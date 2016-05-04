/**
 * File: thread-pool.cc
 * --------------------
 * Presents the implementation of the ThreadPool class.
 */

#include "thread-pool.h"
using namespace std;

bool exiting = 0;
mutex queueLock;
queue<function<void(void)> > funcQ;

// Implementation of lazy thread spawning.
void ThreadPool::spawnThread() {
    if(numSpawned >= numTotal) return;
    int ID = numSpawned++;

    info.push_back(new workerInfo);
    info[ID] -> sem = unique_ptr<semaphore>(new semaphore(0));

    wts[ID] = thread([this](size_t ID){
        worker(ID);
    },ID);
}


void ThreadPool::dispatcher() {
    while(!exiting) {
        queueLock.lock();
        // Check the function queue for unassigned functions.
        if(funcQ.size() > 0) {
            function<void(void)> funct = funcQ.front();
            funcQ.pop();
            queueLock.unlock();
            
            bool foundOpenWorker = false;
            while(true) {
                // Find available worker if one exists.
                for(uint ID = 0; ID < info.size(); ID++) {
                    info[ID] -> infoLock.lock();
                    // If the worker is available, give it a function, 
                    // make it unavailable, signal it to work, then break.
                    if(info[ID] -> available) {
                        info[ID] -> func = funct;
                        info[ID] -> available = false;
                        info[ID] -> infoLock.unlock(); // unlock before signal for speed
                        (*(info[ID]->sem.get())).signal();
                        foundOpenWorker = true;
                        break;
                    } 
                    info[ID] -> infoLock.unlock();
                }
                // Otherwise spawn a new thread if possible.
                if(foundOpenWorker) break;
                spawnThread();
            }
            cv.notify_one();
        } else queueLock.unlock();
    }
}

void ThreadPool::worker(int ID) {
    // Style Choice: put spaces around arrows for readability
    while(true) {
        (*(info[ID]->sem.get())).wait();
        if(exiting) return;
        info[ID] -> func();
        info[ID] -> infoLock.lock();
        info[ID] -> available = true;
        info[ID] -> infoLock.unlock();
        info[ID] -> cond.notify_one();
    }
}

ThreadPool::ThreadPool(size_t numThreads) : wts(numThreads),numTotal(numThreads) {
    dt = thread([this]() {
	   dispatcher();
    });
}

void ThreadPool::schedule(const function<void(void)>& thunk) {
    queueLock.lock();
    funcQ.push(thunk);
    queueLock.unlock();
}

void ThreadPool::wait() {
    mutex lk;

    // Wait for queue to be empty
    queueLock.lock();
    bool emptyQueue = funcQ.size() == 0;
    queueLock.unlock();
    if(!emptyQueue) {
        cv.wait(lk, [this]{
            queueLock.lock();
            bool retval = funcQ.size() == 0;
            queueLock.unlock();
            return retval;
        });
    }


    // Wait for all workers to be done
    for(uint i = 0; i < info.size(); i++) {
        info[i]->infoLock.lock();
        bool iAvailable = info[i]->available;
        info[i]->infoLock.unlock();

        if(!iAvailable) {
            info[i]->cond.wait(lk, [this,i]{
                info[i]->infoLock.lock();
                bool retval = info[i] -> available;
                info[i]->infoLock.unlock();
                return retval;
            });
        }
    }
}


ThreadPool::~ThreadPool() { 
    wait(); // wait until all jobs are done.

    // Tell the threads to exit and join them.
    exiting = true; 
    dt.join();
    for(uint i = 0; i < info.size(); i++) {
        (*(info[i]->sem.get())).signal();
        wts[i].join();
    }
    // Clean up the vector of workerInfo
    for(workerInfo *in : info) {
        delete(in);
    }
}
