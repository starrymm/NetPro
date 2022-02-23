#pragma once

#include "EpollPoller.h"
#include <memory>
class Handler;

class Reactor
{
public:
    Reactor():
    m_poller(new EpollPoller){};
    
    void registerHandler(Handler *);
    void removeHandler(Handler *);
    void loop();
private: 
    std::unique_ptr<EpollPoller> m_poller;
};