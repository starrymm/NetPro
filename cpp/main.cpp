#include "EventLoopThread.h"
#include "Handler.h"
#include "ThreadPool.h"

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <string.h>

#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <memory>
#include <functional>

using namespace std::placeholders;

class ThreadPoll;
class EchoServer{
public:
    EchoServer(int port, EpollPoller * mainLoop, int threadNums):
    port_(port), mainLoop_(mainLoop), threadNums_(threadNums)
    {
        threadPoll_ = new ThreadPool(mainLoop_, threadNums);
    }

    void start(){
        threadPoll_->start();
        startListen();
    }

private:
    void startListen(){
        int listenfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
        if(listenfd < 0){
            printf("error. socket failed.\n");
            exit(-1);
        }
        std::shared_ptr<Handler> listenHandler = std::make_shared<Handler>(listenfd);
        handlerList_[listenfd] = listenHandler;

        int reuse = 1;
        setsockopt(listenHandler->fd(), SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
        sockaddr_in addr;
        bzero(&addr, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port_);
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        int ret = ::bind(listenHandler->fd(), (sockaddr *)&addr, sizeof(addr));
        if(ret < 0){
            printf("error. bind failed.\n");
            exit(-1);
        }
        
        ret = listen(listenHandler->fd(), 10);
        if(ret < 0){
            printf("error. listen failed.\n");
            exit(-1);
        }
        listenHandler->setReadCallback(std::bind(&EchoServer::acceptConn, this, _1));
        listenHandler->enableRead();
        listenHandler->poll_ = mainLoop_;
        mainLoop_->update(listenHandler.get());
    }


    void acceptConn(int fd){
        printf("accept.  listenfd = %d\n", fd);
        int connfd = accept4(fd, NULL, NULL, SOCK_NONBLOCK);
        if(connfd < 0){
            printf("error. accept4 failed.\n");
            exit(-1);
        }
        auto connHandler = std::make_shared<Handler>(connfd);
        handlerList_[connfd] = connHandler;
        connHandler->setReadCallback(std::bind(&EchoServer::readData, this, _1));
        connHandler->enableRead();
        EpollPoller * selected =  threadPoll_->getNextLoop();
        printf("assign Handler to thread: %s\n", selected->thread_name_.c_str());
        connHandler->poll_ = selected;
        selected->update(connHandler.get());
    }

    void readData(int fd){
        auto pHandler = handlerList_[fd];
        printf("readData on thread: %s\n", pHandler->poll_->thread_name_.c_str());
        int nbytes = read(fd, pHandler->buff(), MAX_BUFLEN);
        if(nbytes > 0){
            pHandler->buff()[nbytes] = 0;
            printf("%s\n", pHandler->buff());
            pHandler->setLen(nbytes);
            pHandler->setWriteCallback(std::bind(&EchoServer::sendData, this, _1));
            pHandler->enableWrite();
            pHandler->poll_->update(pHandler.get());
        }
        else if(nbytes == 0){
            printf("close fd\n");
            pHandler->poll_->remove(pHandler.get());
            ::close(fd);
            handlerList_.erase(fd);
        }
        else{
            printf("read error.\n");
            pHandler->poll_->remove(pHandler.get());
            ::close(fd);
            handlerList_.erase(fd);
        }
        
    }

    void sendData(int fd){
        auto pHandler = handlerList_[fd];
        int nbytes = write(fd, pHandler->buff(), pHandler->length());
        if(nbytes > 0){
            pHandler->setReadCallback(std::bind(&EchoServer::readData, this, _1));
            pHandler->enableRead();
            pHandler->poll_->update(pHandler.get());
        }
        else {
            printf("error. write failed.\n");
            pHandler->poll_->remove(pHandler.get());
            ::close(fd);
            handlerList_.erase(fd);
        }
    }

private:
    int port_;
    int threadNums_;
    EpollPoller *mainLoop_;
    ThreadPool *threadPoll_;

    std::map<int, std::shared_ptr<Handler>> handlerList_;
};


int main(int argc, char *argv[]){
    EpollPoller * mainLoop = new EpollPoller("main thread");
    EchoServer server(7777, mainLoop, 4);
    server.start();
    runLoop(mainLoop);
    return 0;
}