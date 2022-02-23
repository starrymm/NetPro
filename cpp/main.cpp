#include "Reactor.h"
#include "Handler.h"

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

class EchoServer{
public:
    EchoServer(int port):
    m_port(port),
    m_base(new Reactor)
    {

    }

    void start(){
        startup();
        m_base->loop();
    }


private:
    void startup(){
        int listenfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
        if(listenfd < 0){
            printf("error. socket failed.\n");
            exit(-1);
        }
        std::shared_ptr<Handler> listenHandler = std::make_shared<Handler>(listenfd);
        m_handleList[listenfd] = listenHandler;

        int reuse = 1;
        setsockopt(listenHandler->fd(), SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
        sockaddr_in addr;
        bzero(&addr, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(m_port);
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
        m_base->registerHandler(listenHandler.get());
    }


    void acceptConn(int fd){
        printf("accept.  listenfd = %d\n", fd);
        int connfd = accept4(fd, NULL, NULL, SOCK_NONBLOCK);
        if(connfd < 0){
            printf("error. accept4 failed.\n");
            exit(-1);
        }
        auto connHandler = std::make_shared<Handler>(connfd);
        m_handleList[connfd] = connHandler;
        connHandler->setReadCallback(std::bind(&EchoServer::readData, this, _1));
        connHandler->enableRead();
        m_base->registerHandler(connHandler.get());
    }

    void readData(int fd){
        printf("readData. \n");
        auto pHandler = m_handleList[fd];
        int nbytes = read(fd, pHandler->buff(), MAX_BUFLEN);
        if(nbytes > 0){
            pHandler->buff()[nbytes] = 0;
            printf("%s\n", pHandler->buff());
            pHandler->setLen(nbytes);
            pHandler->setWriteCallback(std::bind(&EchoServer::sendData, this, _1));
            pHandler->enableWrite();
            m_base->registerHandler(pHandler.get());
        }
        else if(nbytes == 0){
            printf("close fd\n");
            m_base->removeHandler(pHandler.get());
            ::close(fd);
            m_handleList.erase(fd);
        }
        else{
            printf("read error.\n");
            m_base->removeHandler(pHandler.get());
            ::close(fd);
            m_handleList.erase(fd);
        }
        
    }

    void sendData(int fd){
        auto pHandler = m_handleList[fd];
        int nbytes = write(fd, pHandler->buff(), pHandler->length());
        if(nbytes > 0){
            pHandler->setReadCallback(std::bind(&EchoServer::readData, this, _1));
            pHandler->enableRead();
            m_base->registerHandler(pHandler.get());
        }
        else {
            printf("error. write failed.\n");
            m_base->removeHandler(pHandler.get());
            ::close(fd);
            m_handleList.erase(fd);
        }
    }


private:
    int m_port;
    std::unique_ptr<Reactor> m_base;
    std::map<int, std::shared_ptr<Handler>> m_handleList;
};


int main(){
    EchoServer server(7676);
    server.start();
}