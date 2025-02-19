#include <mutex>
#include <memory>
#include <condition_variable>
using namespace std;

struct SingleLaneBridge {
    int buyCnt = 0;
    int sellCnt = 0;
    int waveNum = 0;
    shared_ptr<condition_variable> condPtr; // same here, cond_var is neither copyable nor movable
    shared_ptr<mutex> mutPtr; // have to use ptr because mutex is neither copyable nor movable

    SingleLaneBridge(): mutPtr(new mutex()), condPtr(new condition_variable()) {}

    // copy and move constructor. So that we can use it in an unordered_map
    SingleLaneBridge(const SingleLaneBridge& other): buyCnt(other.buyCnt), sellCnt(other.sellCnt), mutPtr(other.mutPtr),
        condPtr(other.condPtr) {}

    
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
        buyCnt --;
        if (buyCnt == 0) {
            (*condPtr).notify_all();
            waveNum++;
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
        sellCnt --;
        if (sellCnt == 0) {
            (*condPtr).notify_all();
            waveNum++;
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