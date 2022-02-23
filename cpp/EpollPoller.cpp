#include "EpollPoller.h"
#include "Handler.h"

#include <sys/epoll.h>
#include <unistd.h>
#include <assert.h>

 
EpollPoller::EpollPoller():m_activeEventList(initEventListSize){
    m_epfd = epoll_create1(EPOLL_CLOEXEC);
}
EpollPoller::~EpollPoller(){
    close(m_epfd);
}

//调用 epoll_ctl添加事件句柄
void 
EpollPoller::update(Handler * handler){
    epoll_event epEvent;
    epEvent.events = handler->events();
    epEvent.data.ptr = handler;
    int op;
    if(handler->m_isInEpoll){
        op = EPOLL_CTL_MOD;
    }
    else {
        op = EPOLL_CTL_ADD;
    }
    handler->m_isInEpoll = true;
    int ret = epoll_ctl(m_epfd, op, handler->fd(), &epEvent);
    if (ret < 0){
        printf("error. epoll_ctl add failed on fd = %d\n", handler->fd());
    }
    else{
        if(epEvent.events & EPOLLIN){
            printf("[EPOLLIN] epoll_ctl add fd = %d success \n", handler->fd());
        }
        else if(epEvent.events & EPOLLOUT){
            printf("[EPOLLOUT] epoll_ctl add fd = %d success \n", handler->fd());
        }
    }
}   

//调用 epoll_ctl删除事件句柄
void 
EpollPoller::remove(Handler * handler){
    assert(handler->m_isInEpoll);
    handler->m_isInEpoll = false;
    int op = EPOLL_CTL_DEL;
    int ret = epoll_ctl(m_epfd, op, handler->fd(), NULL);
    if(ret < 0){
        printf("error. epoll_ctl del failed on fd = %d \n", handler->fd());
    }
}

std::vector<Handler *> 
EpollPoller::fillActiveChannels(int n){
    std::vector<Handler *> activeEventPtrList;
    for(int i = 0; i < n; i++){
        Handler * phandler = static_cast<Handler *> (m_activeEventList[i].data.ptr);
        if(m_activeEventList[i].events & EPOLLIN){
            phandler->m_revents = EPOLLIN; 
        }
        if(m_activeEventList[i].events & EPOLLOUT){
            phandler->m_revents = EPOLLOUT; 
        }
        activeEventPtrList.push_back(phandler);
    }
    return activeEventPtrList;
}



//调用 epoll_waitd等待事件发生，并返回事件句柄集合
std::vector<Handler *> 
EpollPoller::poll(){
    std::vector<Handler *> activeEventPtrList;
    int nreadys = epoll_wait(m_epfd, m_activeEventList.data(), m_activeEventList.size(), 2000);
    if(nreadys > 0){
        activeEventPtrList = fillActiveChannels(nreadys);
        if(nreadys == m_activeEventList.size()){
            m_activeEventList.resize(m_activeEventList.size() * 2);
        }
    
    }
    else if (nreadys == 0){

    }
    else {
        printf("error. nreadys = %d\n", nreadys);
    }
    return activeEventPtrList;
}