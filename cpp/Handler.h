#pragma once 
#include <iostream>
#include "CallBack.h"
#include "EpollPoller.h"

#define MAX_BUFLEN 4096
class Handler{
    friend class EpollPoller;
public:
    Handler(int);
    ~Handler();
    void handleEvent();
    void setReadCallback(EventCallback);
    void setWriteCallback(EventCallback);
    void setCloseCallbakc(EventCallback);
    void enableRead();
    void enableWrite();
    void enableAll();
    void disableAll();

    int fd() const {
        return m_fd;    
    }

    int events() const {
        return m_events;
    }
    
    int revents() const {
        return m_revents;
    }

    char * buff(){
        return m_buff;
    }

    void setLen(int len){
        m_len = len;
    }

    int length(){
        return m_len;
    }
    
public:
    EpollPoller * poll_;

private:
    int m_fd;
    int m_events; 
    int m_revents;
    bool m_isInEpoll;
    char m_buff[MAX_BUFLEN];
    int m_len;
    EventCallback readCallback;
    EventCallback writeCallback;
    EventCallback closeCallback;
    EventCallback errorCallback;
};