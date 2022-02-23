#include "Handler.h"
#include <sys/epoll.h>

Handler::Handler(int fd):
m_fd(fd), m_events(0), m_revents(0),
m_isInEpoll(false), m_len(0) {

}

Handler::~Handler(){

}

void 
Handler::handleEvent(){
    if(m_revents & EPOLLIN){
        readCallback(m_fd);
    }
    if(m_revents & EPOLLOUT){
        writeCallback(m_fd);
    }
}

void 
Handler::setReadCallback(EventCallback cb){
    readCallback = cb;
}

void 
Handler::setWriteCallback(EventCallback cb){
    writeCallback = cb;
}

void 
Handler::setCloseCallbakc(EventCallback cb){
    closeCallback = cb;
}

void 
Handler::enableRead(){
    m_events = EPOLLIN;
}

void 
Handler::enableWrite(){
    m_events = EPOLLOUT;
}

void 
Handler::enableAll(){
    m_events |= EPOLLIN;
    m_events |= EPOLLOUT;
}

void 
Handler::disableAll(){
    m_events = 0;
}
