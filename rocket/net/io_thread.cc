#include<assert.h>
#include<pthread.h>
#include"rocket/net/io_thread.h"
#include"rocket/common/log.h"
#include"rocket/common/util.h"
namespace rocket{
IOThread::IOThread(){
    int rt=sem_init(&m_init_semaphore,0,0);
    assert(rt == 0);

    rt=sem_init(&m_start_semaphore,0,0);
    assert(rt==0);

    pthread_create(&m_thread,NULL,&IOThread::Main,this);
    //引入信号量，一直wait，指定新线程执行完Main函数的前置
    sem_wait(&m_init_semaphore);
    
    DEBUGLOG("IOThread &d create sucess",m_thread_id);
}

IOThread::~IOThread(){
    m_event_loop->stop();
    sem_destroy(&m_init_semaphore);
    sem_destroy(&m_start_semaphore);
    pthread_join(m_thread,NULL);

    if(m_event_loop){
        delete m_event_loop;
        m_event_loop=NULL;
    }
}

void* IOThread::Main(void *arg){
    IOThread* thread=static_cast<IOThread*>(arg);

    //在新线程中
    thread->m_event_loop = new EventLoop();
    thread->m_thread_id=getThreadId();

    //前置，主线程执行未完成前置的创建，在IOThread线程的线程创建就完成并返回将发生错误，所以引入信号量机制
    //唤醒等待的线程    
    sem_post(&thread->m_init_semaphore);

    DEBUGLOG("IOThread %d created,wait start semaphore",thread->m_thread_id);
    //让线程阻塞，直到主动启动
    sem_wait(&thread->m_start_semaphore);
    DEBUGLOG("IOThread %d start loop",thread->m_thread_id);
    thread->m_event_loop->loop();

    DEBUGLOG("IOThread %d end loop",thread->m_thread_id);

    return NULL;

}
EventLoop* IOThread::getEventLoop(){
    return m_event_loop;   
}
void IOThread::start(){  //线程启动
    DEBUGLOG("Now invoke IOThread %d",m_thread_id);
    sem_post(&m_start_semaphore);
}
void IOThread::join(){
    pthread_join(m_thread,NULL);


}
}