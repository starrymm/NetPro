#pragma once 
#include "EpollPoller.h"
#include "EventLoopThread.h"
#include <vector>
#include <assert.h>

class ThreadPool{

public:
    ThreadPool(EpollPoller * loop, int threadNums):
    mainLoop_(loop), threadNums_(threadNums){
        pos_ = 0;
        started_ = false;
        eventLoopThreads_ = nullptr;
    }

    void start(){
        assert(!started_);

        //mainLoop位于主线程
        assert(mainLoop_->owner_thread_id_ == pthread_self());

        started_ = true ;

        if(threadNums_ <= 0)
            return;

        eventLoopThreads_ = (EventLoopThread *) malloc(threadNums_ * sizeof(EventLoopThread));

        for(int i = 0; i < threadNums_; i++){
            eventLoopThreads_[i].loopThreadInit(i+1);
			eventLoopThreads_[i].loopThreadStart(i+1);
        }
    }

    EpollPoller * getNextLoop(){
        assert(started_);

        assert(mainLoop_->owner_thread_id_ == pthread_self());

        EpollPoller * selected = mainLoop_;
        if(threadNums_ > 0){
            selected = eventLoopThreads_[pos_].poll_;
            if(++pos_ == threadNums_){
                pos_ = 0;
            }
        }
        return selected;
    }

private:
    EpollPoller * mainLoop_;
    EventLoopThread *  eventLoopThreads_;

    bool started_;
    int threadNums_;
    int pos_;

};