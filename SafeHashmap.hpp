#pragma once
#include <unordered_map>
#include <mutex>
#include <semaphore>

// mutiple readers or only 1 writer in hashmap at any time

template <typename Key, typename T>
struct SafeHashMap {
	int readers = 0;
	std::counting_semaphore<1> roomEmpty{1};
	std::mutex mut;
	
	std::unordered_map<Key, T> hashmap;

	void enterRead() {
		std::lock_guard<mutex> lock(mut);
		readers ++;
		if (readers == 1) roomEmpty.acquire();
	}

	void leaveRead() {
		std::lock_guard<mutex> lock(mut);
		readers --;
		if (readers == 0) roomEmpty.release();
	}

	void enterWrite() {
		roomEmpty.acquire();
	}

	void leaveWrite() {
		roomEmpty.release();
	}

	bool contains(const Key& k) {
		enterRead();
		bool ans = hashmap.contains(k);
		leaveRead();
		
		return ans;
	}

	// check if key k exists and check if hashmap[k] == val
	bool containsAndMatch(const Key& k, T val) {
		enterRead();
		bool ans = hashmap.contains(k) && hashmap.at(k) == val;
		leaveRead();

		return ans;
	}

	// must make sure (key, value) already exists
	T& getValue(const Key& key) {
		enterRead();
		T& val = hashmap.at(key);
		leaveRead();
		
		return val;
	}

	// if key does not exists, insert a default constructed T
	// and returns that
	T& writeAndGetValue(const Key& key) {
		enterWrite();
		T& val = hashmap[key];
		leaveWrite();
		return val;
	}

	void insert(const Key& key, T value) {
		enterWrite();
		hashmap[key] = value;
		leaveWrite();
	}
};
