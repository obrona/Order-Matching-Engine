#pragma once

#include <mutex>
#include <memory>
#include <condition_variable>
#include <functional>
#include <iostream>
using namespace std;


// this time, orders from the same side will sleep, until they all reached the other side, then all gets release

struct SLB2 {
    int buyCnt = 0;
    int sellCnt = 0;
    int waveNum = 0;
    shared_ptr<condition_variable> condPtr; // same here, cond_var is neither copyable nor movable
    shared_ptr<condition_variable> condPtr2; // orders from same side wait untill all reach other end, then leave
    shared_ptr<mutex> mutPtr; // have to use ptr because mutex is neither copyable nor movable

    SLB2(): condPtr(new condition_variable()), condPtr2(new condition_variable()), mutPtr(new mutex()) {}

    // copy and move constructor. So that we can use it in an unordered_map
    SLB2(const SLB2& other): buyCnt(other.buyCnt), sellCnt(other.sellCnt), 
        condPtr(other.condPtr), condPtr2(other.condPtr2), mutPtr(other.mutPtr) {}

    
    // returns the wave number for the thread calling this
    int enterBuy() {
        int ticket;
        
        {
            unique_lock<mutex> lock(*mutPtr);
            while (sellCnt > 0) (*condPtr).wait(lock);
            ticket = waveNum;
            buyCnt++;
        }

        return ticket;
    }

    void leaveBuy() {
        unique_lock<mutex> lock(*mutPtr);
       
        if (buyCnt == 1) {
            waveNum++;
            buyCnt --;
            
            
            (*condPtr2).notify_all();
            (*condPtr).notify_all();
        } else {
            buyCnt --;
            while (buyCnt > 0) (*condPtr2).wait(lock);
        }
    }

    void leaveBuy(int c, function<void()> f) {
        unique_lock<mutex> lock(*mutPtr);
        
        if (buyCnt == 1) {  
            f();
            buyCnt --; // decrement must happen after running f because of spurious wakeup
            waveNum += c;
            
            (*condPtr2).notify_all();
            (*condPtr).notify_all();
        } else {
            buyCnt --;
            while (buyCnt > 0) (*condPtr).wait(lock);
        }
    }

    int enterSell() {
        int ticket;
        
        {
            unique_lock<mutex> lock(*mutPtr);
            while (buyCnt > 0) (*condPtr).wait(lock);
            ticket = waveNum;
            sellCnt++;
        }
        
        return ticket;
    }

    void leaveSell() {
        unique_lock<mutex> lock(*mutPtr);
        
        if (sellCnt == 1) {
            waveNum++;
            sellCnt --;
           
            (*condPtr2).notify_all();
            (*condPtr).notify_all();
        } else {
            sellCnt --;
            while (sellCnt > 0) (*condPtr2).wait(lock);
        }
    }

    void leaveSell(int c, function<void()> f) {
        unique_lock<mutex> lock(*mutPtr);
        
        if (sellCnt == 1) {
            f();
            sellCnt --;
            waveNum += c;
            
            (*condPtr2).notify_all();
            (*condPtr).notify_all();
        } else {
            sellCnt --;
            while (sellCnt > 0) (*condPtr2).wait(lock);
        }
    }

    // both buyCnt and sellCnt must be 0
    // only 1 cancel order can be in the 'bridge'
    int enterCancel() {
        int ticket;
        
        {
            unique_lock lock(*mutPtr);
            while (buyCnt > 0 || sellCnt > 0) (*condPtr).wait(lock);
            ticket = waveNum;
            sellCnt++; buyCnt++;
        }

        return ticket;
    }

    void leaveCancel() {
        unique_lock lock(*mutPtr);
        sellCnt --; buyCnt --;
        waveNum++;
        (*condPtr).notify_all();
    }
};