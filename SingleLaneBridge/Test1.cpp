#include <thread>
#include <iostream>
using namespace std;

#include "SingleLaneBridge.cpp"

// test the basic functionality of SingleLaneBridge

SingleLaneBridge slb;

void wantBuy(int id) {
    for (int i = 0; i < 2; i ++) {
        int t = slb.enterBuy();
        
        printf("T%d enter buy. WaveNum: %d\n", id, t);
        this_thread::sleep_for(chrono::milliseconds(1));
        printf("T%d leave buy. WaveNum: %d\n", id, t);
        
        slb.leaveBuy();
    }
   
}

void wantSell(int id) {
    for (int i = 0; i < 2; i ++) {
        int t = slb.enterSell();

        printf("T%d enter sell. WaveNum: %d\n", id, t);
        this_thread::sleep_for(chrono::milliseconds(1));
        printf("T%d leave sell. WaveNum: %d\n", id, t);

        slb.leaveSell();
    }
}

void wantCancel(int id) {
    for (int i = 0; i < 2; i ++) {
        int t = slb.enterCancel();

        printf("T%d enter cancel. WaveNum: %d\n", id, t);
        this_thread::sleep_for(chrono::milliseconds(100));
        printf("T%d leave cancel. WaveNum: %d\n", id, t);

        slb.leaveCancel();
    }
}
int main() {
    thread t1(wantBuy, 1);
    thread t3(wantBuy, 3);
    thread t2(wantSell, 2);
    thread t4(wantCancel, 4);
    
    t3.join();
    t4.join();
    t1.join();
    t2.join();

    
}