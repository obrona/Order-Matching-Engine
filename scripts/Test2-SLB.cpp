#include <unordered_map>
#include <iostream>
#include <thread>
using namespace std;
#include "../SingleLaneBridge.hpp"

// test whether SingleLaneBridge can be used in a hashmap.

unordered_map<int, SingleLaneBridge> hashmap;

void wantBuy(int id) {
    for (int i = 0; i < 2; i ++) {
        int t = hashmap[0].enterBuy();
       
        printf("T%d enter buy. WaveNum: %d\n", id, t);
        this_thread::sleep_for(chrono::milliseconds(1));
        printf("T%d leave buy. WaveNum: %d\n", id, t);

        hashmap[0].leaveBuy();
    }
}

void wantSell(int id) {
    for (int i = 0; i < 2; i ++) {
        int t = hashmap[0].enterSell();

        printf("T%d enter sell. WaveNum: %d\n", id, t);
        this_thread::sleep_for(chrono::milliseconds(1));
        printf("T%d leave sell. WaveNum: %d\n", id, t);

        hashmap[0].leaveSell();
    }
}

void wantCancel(int id) {
    for (int i = 0; i < 2; i ++) {
        int t = hashmap[0].enterCancel();

        printf("T%d enter cancel. WaveNum: %d\n", id, t);
        this_thread::sleep_for(chrono::milliseconds(100));
        printf("T%d leave cancel. WaveNum: %d\n", id, t);

        hashmap[0].leaveCancel();
    }
}


int main() {
    SingleLaneBridge& slb = hashmap[0]; // create the slb first if not race condition, as multiple threads
                                        // writing to hashmap

    thread t1(wantBuy, 1);
    thread t2(wantBuy, 2);
    thread t3(wantSell, 3);
    thread t4(wantCancel, 4);

    t4.join();
    t2.join();
    t3.join();
    t1.join();
    
}
