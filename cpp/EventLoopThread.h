#pragma once
#include <pthread.h>
#include "EpollPoller.h"
#include "Handler.h"
#include <string>

void *loopThreadRun(void *arg);
class EventLoopThread{
public:
    EventLoopThread();
    ~EventLoopThread();

    int loopThreadInit(int k){
        pthread_mutex_init(&mutex_, NULL);
		pthread_cond_init(&cond_, NULL);
        poll_ = nullptr;
		thread_tid_ = 0;
        std::string s("Thread-");
	    thread_name_ = s.append(std::to_string(k));

		return 0;
    }

    int loopThreadStart(int k){

        pthread_create(&thread_tid_, NULL, &loopThreadRun, this);
        
        pthread_mutex_lock(&mutex_);
        while(poll_ == nullptr){
            pthread_cond_wait(&cond_, &mutex_);
        }
        pthread_mutex_unlock(&mutex_);

        printf("event loop thread started %s. \n", thread_name_.c_str());
    }
    
public:
    EpollPoller * poll_;
    pthread_t thread_tid_;
    pthread_mutex_t mutex_;
    pthread_cond_t cond_;
    std::string thread_name_;
    
};

void runLoop(void * arg){
    EpollPoller * ep = (EpollPoller *)arg;
    while(1){
        //等待事件发生
        auto activeEventPtrList = ep ->poll();
        if(!activeEventPtrList.empty()){
            for(int i = 0; i < activeEventPtrList.size(); i++){
                activeEventPtrList[i]->handleEvent();
            }
        }
    }
}

void *loopThreadRun(void *arg){
    EventLoopThread * oneloop = (EventLoopThread *)arg;
    pthread_mutex_lock(&oneloop->mutex_);
    
    oneloop->poll_ = new EpollPoller(oneloop->thread_name_);
    printf("event loop thread init and signal, %s\n", oneloop->thread_name_.c_str());

    pthread_cond_signal(&oneloop->cond_);

    pthread_mutex_unlock(&oneloop->mutex_);
    
    //开启事件循环
    runLoop(oneloop->poll_);
}

