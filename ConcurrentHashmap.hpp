#pragma once
#include <unordered_map>
#include <mutex>
#include <semaphore>
using namespace std;

// readers writers problem
// multiple readers, only 1 writer

template <typename Key, typename T>
struct ConcurrentHashMap {
    counting_semaphore<1> roomEmpty{1};
    mutex mut;
    int readers = 0;

    unordered_map<Key, T> hashmap;

    ConcurrentHashMap() {}

    void enterWrite() {
        roomEmpty.acquire();
    }

    void leaveWrite() {
        roomEmpty.release();
    }

    void enterRead() {
        lock_guard<mutex> lock(mut);
        readers ++;
        if (readers == 1) roomEmpty.acquire(); // will sleep here, means lock will not be released
    }

    void leaveRead() {
        lock_guard<mutex> lock(mut);
        readers --;
        if (readers == 0) roomEmpty.release();
    }

    void insert(pair<const Key, T> value) {
        enterWrite();
        hashmap.insert(value);
        leaveWrite();
    }

    // this only works if no deletion for the hashmap
    // anyway we do not have any method to delete elems in hashmap
    bool contains(const Key key) {
        bool flag = false;
        
        enterRead();
        flag = hashmap.contains(key);
        leaveRead();

        return flag;
        
    }

    // up to user to make sure key is inside, use contains
    T at(const Key& key) {
        T value;
        
        enterRead();
        value = hashmap.at(key);
        leaveRead();

        return value;
    }
};