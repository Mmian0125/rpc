#ifndef ROCKET_NET_TIMEREVENT
#define ROCKET_NET_TIMEREVENT

#include<functional>
#include<memory>

namespace rocket{
class TimerEvent{
public: 
    typedef std::shared_ptr<TimerEvent> s_ptr;

    TimerEvent(int interval, bool m_is_repeated, std::function<void()> cb);

    int64_t getArriveTime() const{
        return m_arrive_time;
    };
    void setCanceled(bool value){
        m_is_canceled=value;
    }
    bool isCanceled(){
        return  m_is_canceled;
    }
    bool isRepeated(){
        return m_is_repeated;
    }
    std::function<void()> getCallBack(){
        return m_task;
    }
    void resetArriveTime();
private:
    int64_t m_arrive_time;  //事件到达的时间点(ms)
    int64_t m_interval;  //到达下一次执行的时间间隔(ms)
    bool m_is_repeated{false};  //是否为周期性任务
    bool m_is_canceled{false};  //取消定时器的标志

    std::function<void()> m_task;
};

}

#endif