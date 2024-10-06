#ifndef ROCKET_NET_TCP_TCP_CLIENT_H
#define ROCKET_NET_TCP_TCP_CLIENT_H

#include"rocket/net/tcp/net_addr.h"
#include"rocket/net/eventloop.h"
#include"rocket/net/tcp/tcp_connection.h"
#include"rocket/net/abstruct_protocol.h"

namespace rocket{
class TcpClient{
public:
    TcpClient(NetAddr::s_ptr peer_addr);
    ~TcpClient();
    void connect(std::function<void()> done);  //异步的执行connect()，若connect成功，done将被执行
    void writeMessage(AbstructProtocol::s_ptr message,std::function<void(AbstructProtocol::s_ptr)> done);  
    //异步的发送message,发送成功将调用done函数，函数的入参为message对象
    void ReadMessage(AbstructProtocol::s_ptr message,std::function<void(AbstructProtocol::s_ptr)> done);  
    //异步的读取message,读取成功将调用done函数，函数的入参为message对象
private:
    NetAddr::s_ptr m_peer_addr;
    EventLoop* m_event_loop{NULL};

    int m_fd{-1};
    FdEvent* m_fd_event{NULL};

    TcpConnection::s_ptr m_connection;
};
}

#endif