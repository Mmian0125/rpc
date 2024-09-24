#include<sys/timerfd.h>
#include<string.h>
#include<sys/time.h>
#include"rocket/net/timer.h"
#include"rocket/common/log.h"
#include"rocket/common/util.h"

namespace rocket{

Timer::Timer():FdEvent(){
    m_fd=timerfd_create(CLOCK_MONOTONIC,TFD_NONBLOCK|TFD_CLOEXEC);  //m_fd到达时间时将产生可读事件
    DEBUGLOG("timer fd=%d",m_fd);
    //发生fd可读事件，执行该Timer定时器的onTimer()方法
    listen(FdEvent::IN_EVENT,std::bind(&Timer::onTimer,this));
}
Timer::~Timer(){
    
}
void Timer::onTimer(){
    //处理缓冲区数据，防止下一次触发可读事件
    char buf[8];
    while(1){
        if(read(m_fd,buf,8)==-1 && errno==EAGAIN){
            break;
        }
    }
    //执行定时任务
    int64_t now=getNowMs();

    std::vector<TimerEvent::s_ptr> tmps;  //存储需要重新注册的周期性任务
    std::vector<std::pair<int64_t,std::function<void()>>> tasks;  //将要执行的任务队列

    ScopeMutex<Mutex> lock(m_mutex);
    auto it=m_pending_events.begin();

    for(it=m_pending_events.begin();it!=m_pending_events.end();++it){
        if((*it).first <= now ){  //时间不超过now且不为取消的任务重新存储到队列中
            if(!(*it).second->isCanceled()){
                tmps.push_back((*it).second);
                tasks.push_back(std::make_pair((*it).second->getArriveTime(),(*it).second->getCallBack()));
            }
        }else{
            break;
        }
    }
    
    m_pending_events.erase(m_pending_events.begin(),it);  //从队列中删除所有早于now的定时任务，这些任务都是已经执行的或超时的
    lock.unlock();

    for(auto i=tmps.begin();i!=tmps.end();++i){  //重新注册周期性任务
        if((*i)->isRepeated()){
            (*i)->resetArriveTime();  //重新计算周期性任务的到达时间
            addTimerEvent(*i);
        }
    }
    resetArriveTime();  //重新计算队列中事件的到达时间

    for(auto i:tasks){  //执行队列中的任务
        if(i.second){
            i.second();
        }
    }
}
void Timer::addTimerEvent(TimerEvent::s_ptr event){
    bool is_reset_timerfd=false;  //标识是否要重新设置定时器

    ScopeMutex<Mutex> lock(m_mutex);
    if(m_pending_events.empty()){  //任务队列为空时，插入任务需要重新设置定时器
        is_reset_timerfd=true;
    }else{
        auto it=m_pending_events.begin();  //任务队列非时，插入任务，若其时间比第一个定时任务的时间早，则重新设置定时器的时间
        if((*it).second->getArriveTime() > event->getArriveTime()){  
            is_reset_timerfd=true;
        }  
    }
    m_pending_events.emplace(event->getArriveTime(),event);  //插入任务
    lock.unlock();
    if(is_reset_timerfd){
        resetArriveTime();
    }
}
void Timer::deleteTimerEvent(TimerEvent::s_ptr event){
    event->setCanceled(true);

    ScopeMutex<Mutex> lock(m_mutex);

    auto begin=m_pending_events.lower_bound(event->getArriveTime());  //lower_bound() 返回第一个大于或等于给定值的元素的迭代器。
    auto end=m_pending_events.upper_bound(event->getArriveTime());  //upper_bound() 返回第一个大于给定值的元素的迭代器。

    auto it=begin;
    for(it=begin;it!=end;++it){  //lower_bound和upper_bound之间的元素，是等于getArriveTime()时间的TimerEvenr
        if(it->second==event){  //找到TimerEvent
            break;
        }
    }
    if(it!=end){
        m_pending_events.erase(it);  //删除
    }
    lock.unlock();
    DEBUGLOG("success delete TimerEvent at arrive time %lld",event->getArriveTime());
}
void Timer::resetArriveTime(){
    ScopeMutex<Mutex> lock(m_mutex);
    auto tmp=m_pending_events;
    lock.unlock();

    if(tmp.size()){  //队列为空，不设置定时器时间
        return;
    }
    int64_t now=getNowMs();
    
    auto it=tmp.begin();
    int64_t interval=0;
    if(it->second->getArriveTime() > now){  //找到第一个大于now的TimerEvent
        interval=it->second->getArriveTime();
    }else{  //否则将时间设置为100ms后
        interval=100;
    }

    timespec ts;
    ts.tv_sec=interval / 1000;
    ts.tv_nsec=(interval % 1000)* 1000000;

    itimerspec value;
    value.it_value=ts;

    int rt=timerfd_settime(m_fd,0,&value,NULL);
    if(rt!=0){
        ERRORLOG("timerfd_settime error,errno=%d",errno);
    }
    DEBUGLOG("timer reset to %lld",now+interval);
    
}
}
