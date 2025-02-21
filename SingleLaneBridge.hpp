#pragma once

#include <mutex>
#include <memory>
#include <condition_variable>
#include <functional>
using namespace std;

struct SingleLaneBridge {
    int buyCnt = 0;
    int sellCnt = 0;
    int waveNum = 0;
    condition_variable condPtr; // same here, cond_var is neither copyable nor movable
    mutex mutPtr; // have to use ptr because mutex is neither copyable nor movable

    SingleLaneBridge() {}

   
    
    // returns the wave number for the thread calling this
    int enterBuy() {
        int ticket;
        
        {
            unique_lock<mutex> lock(mutPtr);
            while (sellCnt > 0) (condPtr).wait(lock);
            ticket = waveNum;
            buyCnt++;
        }

        return ticket;
    }

    void leaveBuy() {
        unique_lock<mutex> lock(mutPtr);
        buyCnt --;
        if (buyCnt == 0) {
            (condPtr).notify_all();
            waveNum++;
        }
    }

    void leaveBuy(function<void()> f) {
        unique_lock<mutex> lock(mutPtr);
        buyCnt --;
        if (buyCnt == 0) {  
            f();
            waveNum ++;
           (condPtr).notify_all();
        }
    }

    int enterSell() {
        int ticket;
        
        {
            unique_lock<mutex> lock(mutPtr);
            while (buyCnt > 0) (condPtr).wait(lock);
            ticket = waveNum;
            sellCnt++;
        }
        
        return ticket;
    }

    void leaveSell() {
        unique_lock<mutex> lock(mutPtr);
        sellCnt --;
        if (sellCnt == 0) {
            waveNum++;
            (condPtr).notify_all();
        }
    }

    void leaveSell(function<void()> f) {
        unique_lock<mutex> lock(mutPtr);
        sellCnt --;
        if (sellCnt == 0) {
            f();
            waveNum ++;
            (condPtr).notify_all();
        }
    }

    // both buyCnt and sellCnt must be 0
    // only 1 cancel order can be in the 'bridge'
    int enterCancel() {
        int ticket;
        
        {
            unique_lock lock(mutPtr);
            while (buyCnt > 0 || sellCnt > 0) (condPtr).wait(lock);
            ticket = waveNum;
            sellCnt++; buyCnt++;
        }

        return ticket;
    }

    void leaveCancel() {
        unique_lock lock(mutPtr);
        sellCnt --; buyCnt --;
        waveNum++;
        (condPtr).notify_all();
    }
};