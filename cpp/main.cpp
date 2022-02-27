#include "EventLoopThread.h"
#include "Handler.h"
#include "ThreadPool.h"

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <string.h>
#include <string>
#include <signal.h>
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <memory>
#include <functional>

extern int output;
using namespace std::placeholders;
using namespace std;
class ThreadPoll;
class HttpServer{
public:
    //端口、主线程、子线程数量
    HttpServer(int port, EpollPoller * mainLoop, int threadNums):
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
            if(output)
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
            if(output)
                printf("error. bind failed.\n");
            exit(-1);
        }
        
        ret = listen(listenHandler->fd(), 10);
        if(ret < 0){
            if(output)
                printf("error. listen failed.\n");
            exit(-1);
        }
        listenHandler->setReadCallback(std::bind(&HttpServer::acceptConn, this, _1));
        listenHandler->enableRead();
        listenHandler->poll_ = mainLoop_;
        mainLoop_->update(listenHandler.get());
    }


    void acceptConn(int fd){
        if(output)
            printf("accept.  listenfd = %d\n", fd);
        int connfd = accept4(fd, NULL, NULL, SOCK_NONBLOCK);
        if(connfd < 0){
            if(output)
                printf("error. accept4 failed.\n");
            exit(-1);
        }
        auto connHandler = std::make_shared<Handler>(connfd);
        handlerList_[connfd] = connHandler;
        connHandler->setReadCallback(std::bind(&HttpServer::readData, this, _1));
        connHandler->enableRead();
        EpollPoller * selected =  threadPoll_->getNextLoop();
        if(output)
            printf("assign Handler to thread: %s\n", selected->thread_name_.c_str());
        connHandler->poll_ = selected;
        selected->update(connHandler.get());
    }

    void readData(int fd){
        auto pHandler = handlerList_[fd];
        if(output)
            printf("readData on thread: %s\n", pHandler->poll_->thread_name_.c_str());
        int nbytes = read(fd, pHandler->buff(), MAX_BUFLEN);
        if(nbytes > 0){
            pHandler->buff()[nbytes] = 0;
            if(output)
                printf("%s\n", pHandler->buff());
            pHandler->setLen(nbytes);
            pHandler->setWriteCallback(std::bind(&HttpServer::sendData, this, _1));
            pHandler->enableWrite();
            pHandler->poll_->update(pHandler.get());
        }
        else if(nbytes == 0){
            pHandler->poll_->remove(pHandler.get());
            //::close(fd);
            handlerList_.erase(fd);
        }
        else{
            if(output)
                printf("read error.\n");
            pHandler->poll_->remove(pHandler.get());
            ::close(fd);
            handlerList_.erase(fd);
        }
        
    }

    void sendData(int fd){
        auto pHandler = handlerList_[fd];
        char *request = pHandler->buff();
        string res;
        int n = parseHttp(request, res);
        int nbytes = write(fd, res.c_str(), n);
        if(output)
            printf("close fd\n");
        ::close(fd);
    }

    int parseHttp(char * request, std::string &res){
        if(strstr(request, "GET / ") || strstr(request, "GET /index.html")){
            res += "HTTP/1.1 200 OK\r\n";   
            res += "Server: WebServer\r\n";
            res += "Content-Type: text/html\r\n\r\n";
            res += "<html><head><h1>Welcome to HttpServer</h1></head>";
            res += "<body>Hello, ^_^</body></html>\r\n";
        }
        else {
            res += "HTTP/1.1 404 Not Found\r\n";
            res += "Server: WebServer\r\n";
            res += "Content-Type: text/html\r\n\r\n";
            res += "<html>not found page</html>\r\n";
        }
        return res.size();
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
    HttpServer server(7777, mainLoop, 8);
    server.start();
    runLoop(mainLoop);
    return 0;
}