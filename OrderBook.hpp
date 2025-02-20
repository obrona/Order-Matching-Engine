#pragma once
#include <map>
#include <mutex>
#include <atomic>
#include <cstdint>
#include <memory>
#include <algorithm>
#include <functional>

using namespace std;

#include "SingleLaneBridge.hpp"
#include "io.hpp"


struct Key {
    uint32_t price, time;
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
    uint32_t rid;
    atomic<uint32_t> count, eid = 1;

    AtomicNode(uint32_t rid, uint32_t cnt): rid(rid), count(cnt) {}

    AtomicNode(const AtomicNode& other): rid(other.rid), count(other.count.load()), eid(other.eid.load()) {}

    void operator=(const AtomicNode& other) {
        rid = other.rid;
        count = other.count.load();
        eid = other.count.load();
    }
};

struct OrderExecute {
    uint32_t resting_order_id; 
    uint32_t new_order_id;    
    uint32_t execution_id;   
    uint32_t price;            
    uint32_t count;           
    uint32_t timestamp;   
    
    bool operator<(const OrderExecute& other) {
        return timestamp < other.timestamp;
    }
};

struct OrderAdd {
    uint32_t order_id;
    const char* instrument;
    uint32_t price;
    uint32_t count;
    bool side;
    uint32_t timestamp;

    bool operator<(const OrderAdd& other) {
        return timestamp < other.timestamp;
    }
};



// invariant: to maintain order, each match has to be map to an atomic operation, all threads
// agree on the coherence order of atomic variables
struct OrderBook {
    SingleLaneBridge slb;

    mutex buyMutPtr;
    atomic<uint32_t> buyTimeStamper = 0;
    map<Key, AtomicNode, CompareGreaterKey> buyBook;

    mutex sellMutPtr;
    atomic<uint32_t> sellTimeStamper = 0;
    map<Key, AtomicNode, CompareLessKey> sellBook;

    mutex mb1;
    vector<OrderExecute> executeQueue;

    mutex mb2;
    vector<OrderAdd> addQueue;

    OrderBook() {}

    void removeFinishedBuys() {
        lock_guard<mutex> lock(buyMutPtr);
        for (auto it = buyBook.begin(); it != buyBook.end();) {
            if (it->second.count.load() == 0) {
                it = buyBook.erase(it);
            } else {
                it++;
            }
        }
    }

    void removeFinishedSells() {
        lock_guard<mutex> lock(sellMutPtr);
        for (auto it = sellBook.begin(); it != sellBook.end();) {
            if (it->second.count.load() == 0) {
                it = sellBook.erase(it);
            } else {
                it++;
            }
        } 
    }

    // last buy/sell call this function
    // no need for locks, because no other threads should be writing to the vectors storing the results
    void leavingFunc(bool isSellSide) {
        (isSellSide) ? removeFinishedBuys() : removeFinishedSells();

        sort(executeQueue.begin(), executeQueue.end());
        for (const OrderExecute& order : executeQueue) {
            Output::OrderExecuted(order.resting_order_id, order.new_order_id, 
                order.execution_id, order.price, order.count, 0);
        }
        executeQueue.clear();
        
        sort(addQueue.begin(), addQueue.end());
        for (const OrderAdd& order: addQueue) {
            Output::OrderAdded(order.order_id, order.instrument, order.price, order.count, order.side, 0);
        }
        addQueue.clear();
    }

    

    // since there is a lock, order will already be in order of output
    // timestamp is just size of opposite side map of current wave
    // because we add orders only after we have traverse all possible resting orders of the opposite side
    void addBuyOrder(const ClientCommand& order, uint32_t timestamp) {
        
        
        {
            lock_guard<mutex> lock(buyMutPtr);
            uint32_t time = buyTimeStamper++;
            buyBook.insert({{order.price, time}, AtomicNode(order.order_id, order.count)});
        }

        {
            lock_guard<mutex> lock(mb2);
            addQueue.push_back({order.order_id, order.instrument, order.price, order.count, false, timestamp});
        }
        
        // Output::OrderAdded(order.order_id, order.instrument, order.price, order.count, false, timestamp);
    }

    void addSellOrder(const ClientCommand& order, uint32_t timestamp) {
        
        {
            lock_guard<mutex> lock(sellMutPtr);
            uint32_t time = sellTimeStamper++;
            sellBook.insert({{order.price, time}, AtomicNode(order.order_id, order.count)});
        }

        {
            lock_guard<mutex> lock(mb2);
            addQueue.push_back({order.order_id, order.instrument, order.price, order.count, true, timestamp});
        }
        //Output::OrderAdded(order.order_id, order.instrument, order.price, order.count, true, timestamp);
    }

    void matchBuy(ClientCommand& order) {
        uint32_t t = slb.enterBuy();
       
        uint32_t len = sellBook.size();
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
                    eid = it->second.eid.fetch_add(1); // post increment execution id
                    order.count -= c; // subtract count fufilled
                    
                    {
                        lock_guard<mutex> lock(mb1);
                        executeQueue.push_back({it->second.rid, order.order_id, eid, it->first.price, c, t + idx});
                    }
                    
                    // Output::OrderExecuted(it->second.rid, order.order_id, eid, it->first.price, c, t + idx); // print output NOW
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
        
        slb.leaveBuy(t + len + 1, [this] {this->leavingFunc(false);});
    }


    void matchSell(ClientCommand& order) {
        uint32_t t = slb.enterSell();
        
        uint32_t len = buyBook.size();
        uint32_t idx = 0;

        auto it = buyBook.begin();
        while (order.count > 0 && it != buyBook.end() && it->first.price >= order.price) {
            while (true) {
                uint32_t eid;
                uint32_t expected = it->second.count.load();
                if (expected == 0) break;

                uint32_t c = min(expected, order.count);
                if (it->second.count.compare_exchange_strong(expected, expected - c)) {
                    eid = it->second.eid.fetch_add(1);
                    order.count -= c;

                    {
                        lock_guard<mutex> lock(mb1);
                        executeQueue.push_back({it->second.rid, order.order_id, eid, it->first.price, c, t + idx});
                    }
                    // Output::OrderExecuted(it->second.rid, order.order_id, eid, it->first.price, c, t + idx);
                }
            }

            it = next(it);
            idx ++;
        }

        if (order.count > 0) {
            addSellOrder(order, t + len);
        }

        slb.leaveSell(t + len + 1, [this] {this->leavingFunc(true);});
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


