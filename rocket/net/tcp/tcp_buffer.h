#ifndef ROCKET_NET_TCP_TCP_BUFFER_H
#define ROCKET_NET_TCP_TCP_BUFFER_H

#include<vector>

namespace rocket{
class TcpBuffer{
public:
    TcpBuffer(int size);
    
    ~TcpBuffer();

    int readAble();  //返回可读字节数

    int writeAble();  //返回可写字节数

    int readIndex();

    int writeIndex();

    void writeToBuffer(const char* buf, int size);

    void readFromBuffer(std::vector<char> &re, int size);

    void resizeBuffer(int newsize);

    void adjustBuffer();

    void moveReadIndex(int size);  //把这一串字节设置为可读

    void moveWriteIndex(int size);  //写入字节的下标从新写入的开始
private:
    int m_read_index{0};
    int m_write_index{0};
    int m_size{0};

    std::vector<char> m_buffer;
};
}

#endif