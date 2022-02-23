#include "Reactor.h"
#include "EpollPoller.h"
#include "Handler.h"

void 
Reactor::registerHandler(Handler * handler){
    m_poller->update(handler);
}


void 
Reactor::removeHandler(Handler * handler){
    m_poller->remove(handler);
}


void 
Reactor::loop(){
    while(1){
        auto activeEventPtrList = m_poller->poll();
        if(!activeEventPtrList.empty()){
            for(int i = 0; i < activeEventPtrList.size(); i++){
                activeEventPtrList[i]->handleEvent();
            }
        }
    }
}