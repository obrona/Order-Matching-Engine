#include <unordered_map>
#include <mutex>

// every read and write needs a mutex, inefficient as we can have multiple reads but whatever

template <typename Key, typename T>
struct SafeHashMap {
	std::mutex mut;
	std::unordered_map<Key, T> hashmap;

	bool contains(const Key& k) {
		std::lock_guard<std::mutex> lock(mut);
		return hashmap.contains(k);
	}

	// check if key k exists and check if hashmap[k] == val
	bool containsAndMatch(const Key& k, T val) {
		std::lock_guard<std::mutex> lock(mut);
		return hashmap.contains(k) && hashmap[k] == val;
	}

	// if key does not exists, insert a default constructed T
	// and returns that
	T& getValue(const Key& key) {
		std::lock_guard<std::mutex> lock(mut);
		return hashmap[key];
	}

	void insert(const Key& key, T value) {
		std::lock_guard<std::mutex> lock(mut);
		hashmap[key] = value;
	}
};
