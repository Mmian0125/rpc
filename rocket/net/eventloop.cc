#include<sys/socket.h>
#include<sys/epoll.h>
#include<sys/eventfd.h>
#include<string.h>
#include "rocket/common/mutex.h"
#include "rocket/common/util.h"
#include "rocket/net/eventloop.h"
#include "rocket/common/log.h"

#define ADD_TO_EPOLL() \
    auto it = m_listen_fds.find(event->getFd());  \
        int op=EPOLL_CTL_ADD;  \
        if(it!=m_listen_fds.end()){  \
            op = EPOLL_CTL_MOD;  \
        }  \
        epoll_event tmp = event->getEpollEvent();  \
        INFOLOG("epoll_event.events=%d",(int)tmp.events); \
        int rt = epoll_ctl(m_epoll_fd,op,event->getFd(),&tmp);  \
        if(rt==-1){  \
            ERRORLOG("failed epoll_ctl when add fd %d, errno=%d",event->getFd(),errno);  \
        }  \
        m_listen_fds.insert(event->getFd());  \
        DEBUGLOG("add event sucess,fd[%d]\n",event->getFd());

#define DELETE_TO_EPOLL()  \
auto it = m_listen_fds.find(event->getFd());  \
    if(it == m_listen_fds.end()){  \
        return ;  \
    }  \
    int op=EPOLL_CTL_DEL;  \
    epoll_event tmp = event->getEpollEvent();  \
    int rt = epoll_ctl(m_epoll_fd,op,event->getFd(),NULL);  \
    if(rt==-1){  \
        ERRORLOG("failed epoll_ctl when add fd, eerrno=%d",errno);  \
    }  \
    m_listen_fds.erase(event->getFd());  \
    DEBUGLOG("delete event sucess, fd[%d]",event->getFd());  \

namespace rocket{

static thread_local EventLoop* t_current_eventloop=NULL;  //判断当前线程是否创建EventLoop变量
static int g_epoll_max_timeout = 10000;
static int g_epoll_max_events = 10;
EventLoop::EventLoop(){
    if(t_current_eventloop != NULL){
        ERRORLOG("failed to create event loop, this thread has created event loop");  //未创建EventLoop时，创建EventLoop
        exit(0);
    }
     m_thread_id =getThreadId();

    m_epoll_fd=epoll_create(10);

    if(m_epoll_fd==-1){
        ERRORLOG("failed to create event loop, epoll_create error, error info[%d]",errno);
        exit(0);
    }
    

    
    initWakeUpFdEvent();
    initTimer();
    INFOLOG("success create event loop in thread %d", m_thread_id);
    t_current_eventloop=this;
}
    
EventLoop::~EventLoop(){
    close(m_epoll_fd);
    if(m_wakeup_fd_event){
        delete m_wakeup_fd_event;
        m_wakeup_fd_event=NULL;
    }
}

void EventLoop::initTimer(){
    m_timer = new Timer();
    addEpollEvent(m_timer);
}

void EventLoop::addTimerEvent(TimerEvent::s_ptr event){
    m_timer->addTimerEvent(event);
}

void EventLoop::loop(){
    while(!m_stop_flag){
        ScopeMutex<Mutex> lock(m_mutex);  //上锁
        std::queue<std::function<void()>> tmp_tasks = m_pending_tasks;  //从m_pending_tasks中取出任务，加锁保护
        m_pending_tasks.swap(tmp_tasks);
        lock.unlock();  //解锁

        while(!tmp_tasks.empty()){  //执行队列中取出的任务
           std::function<void()> cb=tmp_tasks.front();
           tmp_tasks.pop();
           if(cb){
            cb();
           }
        }

        int timeout = g_epoll_max_timeout;
        epoll_event result_events[g_epoll_max_events];
        int rt = epoll_wait(m_epoll_fd,result_events,g_epoll_max_events,timeout);
        DEBUGLOG("now end epoll_wait, rt=%d",rt);

        if(rt < 0 ){
            ERRORLOG("epoll_wait error, errno=",errno);
        }else{  //发生事件
            for(int i=0;i<rt;++i){
                epoll_event trigger_event = result_events[i];
                FdEvent* fd_event = static_cast<FdEvent*>(trigger_event.data.ptr);
                if(fd_event==NULL){
                    continue;
                }

                if(trigger_event.events | EPOLLIN){  //发生读事件，加入任务队列
                    addTask(fd_event->handler(FdEvent::IN_EVENT));
                }
                if(trigger_event.events | EPOLLOUT){  //发生写事件，加入任务队列
                    addTask(fd_event->handler(FdEvent::OUT_EVENT));
                }
            }
        }
    }
}

void EventLoop::initWakeUpFdEvent(){
    m_wakeup_fd=eventfd(0,EFD_NONBLOCK);
    if(m_wakeup_fd < 0){
        ERRORLOG("failed to create event loop, eventfd create error, error info[%d]",errno);
        exit(0);
    }
    INFOLOG("wakeup fd=%d",m_wakeup_fd);

    m_wakeup_fd_event = new WakeUpFdEvent(m_wakeup_fd);
    m_wakeup_fd_event->listen(FdEvent::IN_EVENT,[this](){
        char buf[8];
        while((read(m_wakeup_fd,buf,8))!=-1 && errno!=EAGAIN){
        }
        DEBUGLOG("read full bytes from wakeup fd[%d]",m_wakeup_fd);
    });

    addEpollEvent(m_wakeup_fd_event);

}

void EventLoop::dealWakeup(){
    
}

void EventLoop::wakeup(){
    INFOLOG("wake up");
    m_wakeup_fd_event->wakeup();
}

void EventLoop::stop(){
    m_stop_flag = true;
}

void EventLoop::addEpollEvent(FdEvent* event){
    if(isInLoopThread()){
        ADD_TO_EPOLL();
    }else{
        auto cb = [this, event](){  //捕获this,event为了其他线程在访问addEpollEvent()时可以访问ADD_TO_EPOLL()中需要的变量
            ADD_TO_EPOLL();
        };
        addTask(cb,true);
    }
}

void EventLoop::delteEpollEvent(FdEvent* event){
    if(isInLoopThread()){
        DELETE_TO_EPOLL();
    }else{
        auto cb = [this,event](){
            DELETE_TO_EPOLL();
        };
        addTask(cb,true);

    }
}

void EventLoop::addTask(std::function<void()> cb, bool is_wake_up){
    ScopeMutex<Mutex> lock(m_mutex);
    m_pending_tasks.push(cb);
    lock.unlock();
    if(is_wake_up){
        wakeup();
    }

}

bool EventLoop::isInLoopThread(){
    return getThreadId() ==  m_thread_id ;
}
EventLoop* EventLoop::GetCurrentEvemtLoop(){
    if(t_current_eventloop){
        return t_current_eventloop;
    }
    t_current_eventloop = new EventLoop();
    return t_current_eventloop;
}
}
