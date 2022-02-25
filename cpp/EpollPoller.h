#pragma once

#include <vector>
#include <sys/epoll.h>
#include <string>

class Handler;
class EpollPoller{
    typedef std::vector<epoll_event> EventList;

public:
    EpollPoller(std::string thread_name);
    ~EpollPoller();

    //调用 epoll_ctl添加事件句柄
    void update(Handler *);

    //调用 epoll_ctl删除事件句柄
    void remove(Handler *);

    //调用 epoll_waitd等待事件发生，并返回事件句柄集合
    std::vector<Handler *> poll();
    static const int initEventListSize = 16;

public:
    pthread_t owner_thread_id_;
    std::string thread_name_;
    
private:
    std::vector<Handler *> fillActiveChannels(int);

private:
    int m_epfd;
    
    EventList m_activeEventList;
    
};