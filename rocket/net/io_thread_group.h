#ifndef ROCKET_NET_IO_THREAD_GROUP_H
#define ROCKET_NET_IO_THREAD_GROUP_H

#include<vector>
#include"rocket/common/log.h"
#include"rocket/net/io_thread.h"

namespace rocket{
class IOThreadGroup{
public:
    IOThreadGroup(int size);

    ~IOThreadGroup();

    void start();  //控制IO线程循环的开始

    void join();

    IOThread* getIOThread();  //从当前线程组中获取一个线程

private:
    int m_size{0};
    std::vector<IOThread*> m_io_thread_groups;

    size_t m_index{0};  //表示当前需要获取的io线程的下标
};

}
#endif 