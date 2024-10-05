#include<fcntl.h>
#include<assert.h>
#include<sys/socket.h>
#include<string.h>
#include <pthread.h>
#include<arpa/inet.h>
#include<string>
#include<netinet/in.h>
#include<unistd.h>
#include<memory>
#include "rocket/common/log.h"
#include "rocket/common/config.h"
void test_connect(){
    //调用connect连接server
    //send一个字符串
    //等待read返回结果
    int fd = socket(AF_INET,SOCK_STREAM,0);

    if(fd < 0){
        ERRORLOG("invalid listenfd %d",fd);
        exit(0);
    }

    sockaddr_in server_addr;
    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family=AF_INET;
    server_addr.sin_port=htons(12345);
    inet_aton("127.0.0.1",&server_addr.sin_addr);

    int rt=connect(fd,reinterpret_cast<sockaddr*>(&server_addr),sizeof(server_addr));

    std::string msg = "hello rocket!";

    rt=write(fd,msg.c_str(),msg.length());

    DEBUGLOG("sucess write %d bytes, [%s]",rt,msg.c_str());

    char buf[100];
    rt=read(fd,buf,100);
    if(rt!=-1){
        DEBUGLOG("sucess read %d bytes,[%s]",rt,std::string(buf).c_str());
    }else{
        ERRORLOG("no message!");
    }
    
}
int main(){
    rocket::Config::SetGlobalConfig("../conf/rocket.xml");

    rocket::Logger::InitGlobalLogger();

    test_connect();
}