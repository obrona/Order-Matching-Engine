#include <unordered_map>
#include <mutex>

// every read and write needs a mutex, inefficient as we can have multiple reads but whatever

template <typename Key, typename T>
struct SafeHashMap {
	std::mutex mut;
	std::unordered_map<Key, T> hashmap;

	bool contains(const Key& k) {
		std::unique_lock<std::mutex> lock(mut);
		return hashmap.contains(k);
	
	}

	// if key does not exists, insert a default constructed T
	// and returns that
	T& getValue(const Key& key) {
		std::unique_lock<std::mutex> lock(mut);
		return hashmap[key];
	}

	void insert(const Key& key, T value) {
		std::unique_lock<std::mutex> lock(mut);
		hashmap[key] = value;
	}
};
