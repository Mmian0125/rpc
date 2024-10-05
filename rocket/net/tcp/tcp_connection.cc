#include<unistd.h>
#include"rocket/net/tcp/tcp_connection.h"
#include"rocket/common/log.h"
#include"rocket/net/fd_event_group.h"

namespace rocket{
TcpConnection::TcpConnection(IOThread* io_thread, int fd, int buffer_size, NetAddr::s_ptr peer_addr) 
  : m_io_thread(io_thread), m_peer_addr(peer_addr),m_state(NotConnected), m_fd(fd){
    m_in_buffer = std::make_shared<TcpBuffer>(buffer_size);
    m_out_buffer = std::make_shared<TcpBuffer>(buffer_size);
    m_fd_event = FdEventGroup::getFdEventGroup()->getFdEvent(fd);
    m_fd_event->setNonBlock();
    m_fd_event->listen(FdEvent::IN_EVENT,std::bind(&TcpConnection::onRead,this));    
    io_thread->getEventLoop()->addEpollEvent(m_fd_event);
    
}
TcpConnection::~TcpConnection(){
    DEBUGLOG("~TcpConnection");
}
void TcpConnection::onRead(){
    //1.从socket缓冲区调用系统的read函数，读取到in_buffer中
    if(m_state!=Connected){
        ERRORLOG("onRead erorr,client has already disconnected, addr[%s], clientfd[%d]",m_peer_addr->toString().c_str(),m_fd);
        return ;
    }
    bool is_read_all=false;
    bool is_close = false;
    while(!is_read_all){
        if(m_in_buffer->writeAble() == 0){
            m_in_buffer->resizeBuffer(2 * m_in_buffer->m_buffer.size());
        }

        int read_count=m_in_buffer->writeAble();
        int write_index=m_in_buffer->writeIndex();
        int rt = read(m_fd,&(m_in_buffer->m_buffer[write_index]),read_count);
        DEBUGLOG("sucess read %d bytes from addr[%s], client fd[%d]",rt,m_peer_addr->toString().c_str(),m_fd);
        if(rt>0){
            m_in_buffer->moveWriteIndex(rt);
            if(rt==read_count){  //可能缓冲区仍存在数据没有读取完
                continue;
            }else if(rt < read_count){
                is_read_all=true;
                break;
            }
        }else if(rt==0) { //当存在可读事件，但读的数据为0是，表示对方关闭了连接
            is_close=true;
            break;
        }else if(rt==-1 && errno==EAGAIN){  //没有数据可读时
            is_read_all=true;
            break;
        }
    }
    if(is_close){
        DEBUGLOG("peer closed, peer addr [%d], client [%d]",m_peer_addr->toString().c_str(),m_fd);
        clear();
        return;
    }
    if(!is_read_all){
        ERRORLOG("not read all data");
    } 
    //TODO:简单的echo，后面补充RPC的协议解析
    excute();
}
void TcpConnection::excute(){
    //将RPC请求执行业务逻辑，获取RPC响应，再把RPC响应发送回去
    std::vector<char> tmp;
    int size=m_in_buffer->readAble();
    tmp.resize(size);

    m_in_buffer->readFromBuffer(tmp,size);

    std::string msg;
    for(int i=0;i<tmp.size();++i){
        msg+=tmp[i];
    }

    INFOLOG("sucess get request[%s] from client[%s]",msg.c_str(),m_peer_addr->toString().c_str());

    m_out_buffer->writeToBuffer(msg.c_str(),msg.length());

    m_fd_event->listen(FdEvent::OUT_EVENT,std::bind(&TcpConnection::onWrite,this));
    m_io_thread->getEventLoop()->addEpollEvent(m_fd_event);

}
void TcpConnection::onWrite(){
    //将当前onBuffer中的数据全部发送给client
    if(m_state!=Connected){
        ERRORLOG("onWrite erorr,client has already disconnected, addr[%s], clientfd[%d]",m_peer_addr->toString().c_str(),m_fd);
        return;
    }
    bool is_write_all=false;
    while(true){
        if(m_out_buffer->readAble()==0){
            DEBUGLOG("no data need to send to client [%s]",m_peer_addr->toString().c_str());
            is_write_all=true;
            break;   
        }
        int write_size=m_out_buffer->readAble();
        int read_index=m_out_buffer->readIndex();
        int rt = write(m_fd,&(m_out_buffer->m_buffer[read_index]),write_size);
        if( rt >=write_size){
            DEBUGLOG("no data need to send to client [%s]",m_peer_addr->toString().c_str());
            is_write_all=true;
            break;
        }
        if(rt==-1 &&errno==EAGAIN){
            //发送缓冲区已满，不能再发送；等下次fd可写时再发送数据即可
            ERRORLOG("write data error,errno=EAGAIN and rt==-1");
        }
    }
    if(is_write_all){
        m_fd_event->cancel(FdEvent::OUT_EVENT);
        m_io_thread->getEventLoop()->addEpollEvent(m_fd_event);
    }
}

void TcpConnection::setState(const TcpState state){
    m_state = Connected;
}

TcpState getState(){}

void TcpConnection::clear(){  //处理关闭连接后的清理动作
    if(m_state == Closed){
        return;
    }
    m_fd_event->cancel(FdEvent::IN_EVENT);
    m_fd_event->cancel(FdEvent::OUT_EVENT);
    m_io_thread->getEventLoop()->delteEpollEvent(m_fd_event);
    m_state = Closed;
    

}
void TcpConnection::shutdown(){
    if(m_state == Closed||m_state==NotConnected){
        return;
    }
    m_state=HalfClosing;
    ::shutdown(m_fd,SHUT_RDWR);  //调用shutdown函数，进行四次挥手，意味服务器不再对fd进行操作
    //发送FIN报文，触发四次挥手的第一个阶段
    //等到对方返回FIN报文，说明四次挥手结束，即当fd发送可读事件，但可读数据为0
}
}