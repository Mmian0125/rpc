#include<sys/socket.h>
#include<unistd.h>
#include<string.h>
#include"rocket/net/tcp/tcp_client.h"
#include"rocket/common/log.h"
#include"rocket/net/eventloop.h"
#include"rocket/net/fd_event_group.h"
#include"rocket/net/abstruct_protocol.h"


namespace rocket{

 TcpClient::TcpClient(NetAddr::s_ptr peer_addr) : m_peer_addr(peer_addr){
    m_event_loop = EventLoop::GetCurrentEvemtLoop();
    m_fd = socket(peer_addr->getFamily(),SOCK_STREAM,0);
    if(m_fd<0){
        ERRORLOG("TcpClient::TcpClient() error, failed to create fd");
        return;
    }
    m_fd_event = FdEventGroup::getFdEventGroup()->getFdEvent(m_fd);
    m_fd_event->setNonBlock();
    m_connection=std::make_shared<TcpConnection>(m_event_loop,m_fd,128,peer_addr);
    m_connection->setConnectionType(TcpConnectionByClient);
 }
TcpClient::~TcpClient(){
    if(m_fd > 0){
        close(m_fd);
    }
}
void TcpClient::connect(std::function<void()> done){
    int rt=::connect(m_fd,m_peer_addr->getSockAddr(),m_peer_addr->getSockLen());
    if(rt==0){
        DEBUGLOG("connect sucess");
        if(done){
            done();
        }
    }else if(rt==-1){
        if(errno==EINPROGRESS){
            //epoll监听可写事件，判断错误码
            m_fd_event->listen(FdEvent::OUT_EVENT,[this,done](){
                int error=0;
                socklen_t error_len=sizeof(error);
                getsockopt(m_fd,SOL_SOCKET,SO_ERROR,&error,&error_len);
                if(error==0){
                    DEBUGLOG("connect [%s] sucess",m_peer_addr->toString().c_str());  
                    if(done){
                        done();
                    }
                }else{
                    ERRORLOG("connect error,errno=%d",errno);
                }  
                //去掉可写事件的监听
                m_fd_event->cancel(FdEvent::OUT_EVENT);
                m_event_loop->addEpollEvent(m_fd_event);
            });
            m_event_loop->addEpollEvent(m_fd_event);
            if(!m_event_loop->isLooping()){
                m_event_loop->loop();
            }
        }else{
            ERRORLOG("connect error,errno=%d",errno);
        }
    }
}
void TcpClient::writeMessage(AbstructProtocol::s_ptr message, std::function<void(AbstructProtocol::s_ptr)> done){
    
}
void TcpClient::ReadMessage(AbstructProtocol::s_ptr message, std::function<void(AbstructProtocol::s_ptr)> done){

}
}

