#pragma once
#include <map>
#include <mutex>
#include <atomic>
#include <cstdint>
#include <memory>

using namespace std;

#include "SingleLaneBridge.hpp"
#include "io.hpp"


struct Key {
    uint32_t price, time;


    // these here do not work !!!
    bool operator<(const Key& other) {
        if (price != other.price) return price < other.price;
        else return time < other.time;
    }

    bool operator>(const Key& other) {
        if (price != other.price) return price > other.price;
        else return time < other.time;
    }

};

struct CompareLessKey {
    // remember the const after the function declaration i.e f(args) const {}
    bool operator()(const Key& k1, const Key& k2) const {
        if (k1.price != k2.price) return k1.price < k2.price;
        else return k1.time < k2.time;
    }
};

struct CompareGreaterKey {
    bool operator()(const Key& k1, const Key& k2) const {
        if (k1.price != k2.price) return k1.price > k2.price;
        else return k1.time < k2.time;
    }
};

struct AtomicNode {
    uint32_t rid = 0;
    atomic<uint32_t> count = 0, eid = 1;

    AtomicNode(uint32_t rid, uint32_t cnt): count(cnt) {}

    AtomicNode(const AtomicNode& other): rid(other.rid), count(other.count.load()), eid(other.eid.load()) {}

    void operator=(const AtomicNode& other) {
        rid = other.rid;
        count = other.count.load();
        eid = other.count.load();
    }
};



// invariant: to maintain order, each match has to be map to an atomic operation, all threads
// agree on the coherence order of atomic variables
struct OrderBook {
    SingleLaneBridge slb;

    shared_ptr<mutex> buyMutPtr;
    atomic<uint32_t> buyTimeStamper = 0;
    map<Key, AtomicNode, CompareGreaterKey> buyBook;

    shared_ptr<mutex> sellMutPtr;
    atomic<uint32_t> sellTimeStamper = 0;
    map<Key, AtomicNode, CompareLessKey> sellBook;

    OrderBook() {}


    OrderBook(const OrderBook& other): 
        slb(other.slb), 
        buyMutPtr(other.buyMutPtr),
        buyTimeStamper(other.buyTimeStamper.load()),
        buyBook(other.buyBook),
        sellMutPtr(other.sellMutPtr),
        sellTimeStamper(other.sellTimeStamper.load()),
        sellBook(other.sellBook) 
    {}

    void removeFinishedBuys() {
        lock_guard<mutex> lock(*buyMutPtr);
        for (auto it = buyBook.begin(); it != buyBook.end();) {
            if (it->second.count.load() == 0) {
                it = buyBook.erase(it);
            } else {
                it++;
            }
        }
    }

    void removeFinishedSells() {
        lock_guard<mutex> lock(*sellMutPtr);
        for (auto it = sellBook.begin(); it != sellBook.end();) {
            if (it->second.count.load() == 0) {
                it = sellBook.erase(it);
            } else {
                it++;
            }
        } 
    }

    // since there is a lock, order will already be in order of output
    // timestamp is just size of opposite side map of current wave
    // because we add orders only after we have traverse all possible resting orders of the opposite side
    void addBuyOrder(const ClientCommand& order, uint32_t timestamp) {
        lock_guard<mutex> lock(*buyMutPtr);

        uint32_t time = buyTimeStamper++;
        buyBook.insert({{order.price, time}, AtomicNode(order.order_id, order.count)});
        Output::OrderAdded(order.order_id, order.instrument, order.price, order.count, false, timestamp);
    }

    void addSellOrder(const ClientCommand& order, uint32_t timestamp) {
        lock_guard<mutex> lock(*sellMutPtr);

        uint32_t time = sellTimeStamper++;
        sellBook.insert({{order.price, time}, AtomicNode(order.order_id, order.count)});
        Output::OrderAdded(order.order_id, order.instrument, order.price, order.count, true, timestamp);
    }

    void matchBuy(ClientCommand& order) {
        int t = slb.enterBuy();
        int len = sellBook.size();
        uint32_t addOrderTimeStamp = t + len;
        uint32_t idx = 0;

        auto it = sellBook.begin();
        while (order.count > 0 && it != sellBook.end() && it->first.price <= order.price) {
            // kind of a spin lock
            while (true) {
                uint32_t eid;
                uint32_t expected = it->second.count.load();
                if (expected == 0) break;
                
                uint32_t c = min(expected, order.count);
                if (it->second.count.compare_exchange_strong(expected, expected - c)) {
                    // successful match
                    eid = ++(it->second.eid); // post increment execution id
                    order.count -= c; // subtract count fufilled
                    Output::OrderExecuted(it->second.rid, order.order_id, eid, it->first.price, c, t + idx); // print output NOW
                }
                
            }
            it = next(it); // go to next resting sell order
            idx++;
        }

        if (order.count > 0) {
            addBuyOrder(order, t + len);
        }

        // if it is last buy, increment wave counter and remove 0 count resting sell orders
        // order between waves (even if is same side) is preserved here, as all prints (for OrderExecuted and OrderAdded)
        // are done before calling leave
        // time stamp is t + len + 1
        // to ensure all OrderAdded for this wave is before next wave
        slb.leaveBuy(t + len + 1, [this] {this->removeFinishedSells();});
    }


    void matchSell(ClientCommand& order) {
        int t = slb.enterSell();
        int len = buyBook.size();
        uint32_t addOrderTimeStamp = t + len;
        uint32_t idx = 0;

        auto it = buyBook.begin();
        while (order.count > 0 && it != buyBook.end() && it->first.price >= order.price) {
            while (true) {
                uint32_t eid;
                uint32_t expected = it->second.count.load();
                if (expected == 0) break;

                uint32_t c = min(expected, order.count);
                if (it->second.count.compare_exchange_strong(expected, expected - c)) {
                    eid = ++(it->second.eid);
                    order.count -= c;
                    Output::OrderExecuted(it->second.rid, order.order_id, eid, it->first.price, c, t + idx);
                }
            }

            it = next(it);
            idx ++;
        }

        if (order.count > 0) {
            addSellOrder(order, t + len);
        }

        slb.leaveSell(t + len + 1, [this] {this->removeFinishedBuys();});
    }

    // assumes client is the one that posted the order_id, the OrderBook does not check this
    // no choice, search everywhere
    void cancelOrder(const ClientCommand& order) {
        int t = slb.enterCancel();
        bool flag = false;
        
        {
            // we do not need to lock buyMutPtr, as in the bridge, can only have 1 single cancel Order
            // lock_guard<mutex> lock(*buyMutPtr);
            for (auto it = buyBook.begin(); it != buyBook.end(); it ++) {
                if (it->second.rid == order.order_id) {
                    flag = true;
                    buyBook.erase(it);
                    break;
                }
            }
        }

        if (!flag) {
            // lock_guard<mutex> lock(*sellMutPtr);
            for (auto it = sellBook.begin(); it != sellBook.end(); it ++) {
                if (it->second.rid == order.order_id) {
                    flag = true;
                    sellBook.erase(it);
                    break;
                }
            }
        }

        Output::OrderDeleted(order.order_id, flag, t);

        slb.leaveCancel(); 
    }
    


};


