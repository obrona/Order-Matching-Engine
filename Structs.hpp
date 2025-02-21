#include <cstdint>
#include <atomic>
using namespace std;

// all these are the immutable info about a resting order
struct Key {
    uint32_t price = 0, time = 0, rid = 0;  // if rid == -1, means key not inserted
    bool side = false;                         // false if it is buy, true if is sell
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

struct Store {
    uint32_t count = 0, eid = 0;
};

struct AtomicNode {
    atomic<Store> store;

    AtomicNode(uint32_t count, uint32_t eid): store({count, eid}) {}

    AtomicNode(const AtomicNode& other): store(other.store.load()) {}
};

