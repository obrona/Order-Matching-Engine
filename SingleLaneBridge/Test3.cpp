#include "SLB2.cpp"
#include <iostream>
#include <thread>
using namespace std;

SLB2 slb;

void wantBuy(int id) {
    for (int i = 0; i < 3; i ++) {
        int t = slb.enterBuy();

        printf("T%d enter buy. WaveNum: %d\n", id, t);
        this_thread::sleep_for(chrono::milliseconds(1000));
        //printf("T%d leave buy. WaveNum: %d\n", id, t);
        slb.leaveBuy(1, [t] {printf("wave %d leave\n", t);});
        

    }
}

void wantSell(int id) {
    for (int i = 0; i < 3; i ++) {
        int t = slb.enterSell();

        printf("T%d enter sell. WaveNum: %d\n", id, t);
        this_thread::sleep_for(chrono::milliseconds(1000));
        //printf("T%d leave sell. WaveNum: %d\n", id, t);
        slb.leaveSell(1, [t] {printf("wave %d leave\n", t);});
        

    }
}

void wantCancel(int id) {
    for (int i = 0; i < 2; i ++) {
        int t = slb.enterCancel();

        printf("T%d enter cancel. WaveNum: %d\n", id, t);
        this_thread::sleep_for(chrono::milliseconds(1));
        printf("T%d leave cancel. WaveNum: %d\n", id, t);
        slb.leaveCancel();
        

    }
}

void testWaitEnterBuy(int id) {
    for (int i = 0; i < 3; i ++) {
        int t = slb.enterBuy();
        printf("T%d enter cancel. WaveNum: %d\n", id, t);
    }
}

void testWaitLeaveBuy(int id) {
    for (int i = 0; i < 3; i ++) {
        slb.leaveBuy();
    }
}


int main() {
   thread t1(wantSell, 1);
   thread t2(wantSell, 2);
   thread t3(wantBuy, 3);
   thread t5(wantCancel, 5);
   thread t4(wantBuy, 4);
   
   t1.join(); t2.join(); 
   t3.join(); 
   t4.join(); 
   t5.join();
}


