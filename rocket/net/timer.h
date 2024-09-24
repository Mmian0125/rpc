#ifndef ROCKET_NET_TIMER_H
#define ROCKET_NET_TIMER_H

#include<map>
#include"rocket/common/mutex.h"
#include"rocket/net/fd_event.h"
#include"rocket/net/timer_event.h"

namespace rocket{
class Timer : public FdEvent{
public:
    Timer();
    ~Timer();
    void addTimerEvent(TimerEvent::s_ptr event);
    void deleteTimerEvent(TimerEvent::s_ptr event);
    void onTimer();  //当发生IO事件后，eventloop将执行此函数

private:
    void resetArriveTime();  //重新设置定时器的到期时间，根据当前待执行的定时任务列表
    //设置下一个定时任务的触发时间
private:
    std::multimap<int64_t,TimerEvent::s_ptr> m_pending_events;  //任务执行时间(ms) ：定时任务 
    Mutex m_mutex;
};
}

#endif
