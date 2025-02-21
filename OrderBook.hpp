#pragma once
#include <map>
#include <mutex>
#include <atomic>
#include <cstdint>
#include <memory>
#include <algorithm>
#include <functional>
using namespace std;

#include "io.hpp"

#include "Structs.hpp"
#include "SingleLaneBridge.hpp"
#include "Timer.hpp"

struct OrderBook {
    SingleLaneBridge slb;
    
    mutex mut;
    map<Key, AtomicNode, CompareGreaterKey> buyBook;
    map<Key, AtomicNode, CompareLessKey> sellBook;


    void removeFinishedBuys() {
        for (auto it = buyBook.begin(); it != buyBook.end();) {
            if (it->second.store.load().count == 0) {
                it = buyBook.erase(it);
            } else {
                it ++;
            }
        }
    }

    void removeFinishedSells() {
        for (auto it = sellBook.begin(); it != sellBook.end();) {
            if (it->second.store.load().count == 0) {
                it = sellBook.erase(it);
            } else {
                it ++;
            }
        }
    }

    // stores the value of the key in store key
    void addBuy(const ClientCommand& order, Key& storeKey) {
        // since it is a lock, order from timestamp = order of which buy order is added to book
        lock_guard<mutex> lock(mut);
        
        uint32_t time = timer.getTime();
        buyBook.insert({{order.price, time, order.order_id, false}, AtomicNode{order.count, 1}});
        storeKey = {order.price, time, order.order_id, false};
        
        Output::OrderAdded(order.order_id, order.instrument, order.price, order.count, false, time);
    }

    void addSell(const ClientCommand& order, Key& storeKey) {
        lock_guard<mutex> lock(mut);
        
        uint32_t time = timer.getTime();
        sellBook.insert({{order.price, time, order.order_id, true}, AtomicNode{order.count, 1}});
        storeKey = {order.price, time, order.order_id, true};

        Output::OrderAdded(order.order_id, order.instrument, order.price, order.count, true, time);
    }

    void matchBuy(ClientCommand& order, Key& storeKey) {
        slb.enterBuy();

        for (auto it = sellBook.begin(); it != sellBook.end(); it ++) {
            if (order.count == 0 || it->first.price > order.price) break;

            while (true) {
                if (order.count == 0) break;

                // remember
                // 1. load
                // 2. get time stamp
                // 3. then compare exchange
                Store expected = it->second.store.load();
                if (expected.count == 0) break;

                uint32_t c = min(order.count, expected.count);
                uint32_t time = timer.getTime();

                if (it->second.store.compare_exchange_strong(expected, {expected.count - c, expected.eid + 1})) {
                    // successful match, new value updated. 
                    // expected holds the old value in this case
                    order.count -= c;
                    Output::OrderExecuted(it->first.rid, order.order_id, expected.eid, it->first.price, c, time);
                }
            }
        }

        if (order.count > 0) addBuy(order, storeKey);
        
        slb.leaveBuy([this] { this->removeFinishedSells(); });
    }

    void matchSell(ClientCommand& order, Key& storeKey) {
        slb.enterSell();

        for (auto it = buyBook.begin(); it != buyBook.end(); it ++) {
            if (order.count == 0 || it->first.price < order.price) break;

            while (true) {
                if (order.count == 0) break;

                Store expected = it->second.store.load();
                if (expected.count == 0) break;
                
                uint32_t c = min(order.count, expected.count);
                uint32_t time = timer.getTime();

                if (it->second.store.compare_exchange_strong(expected, {expected.count - c, expected.eid + 1})) {
                    order.count -= c;
                    Output::OrderExecuted(it->first.rid, order.order_id, expected.eid, it->first.price, c, time);
                }
            }
        }

        if (order.count > 0) addSell(order, storeKey);

        slb.leaveSell([this] { this->removeFinishedBuys(); });
    }
    

    void cancelOrder(const ClientCommand& order, const Key& k) {
        slb.enterCancel();

        bool found = false;
        
        if (!k.side) {
            auto it = buyBook.find(k);
            if (it != buyBook.end()) {
                found = true;
                buyBook.erase(it);
            }
        } else {
            auto it = sellBook.find(k);
            if (it != sellBook.end()) {
                found = true;
                sellBook.erase(it);
            }
        }

        Output::OrderDeleted(order.order_id, found, timer.getTime());

        slb.leaveCancel();
    }



};




