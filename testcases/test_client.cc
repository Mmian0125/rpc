#include <assert.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>
#include <memory>
#include <unistd.h>
#include "rocket/common/log.h"
#include "rocket/common/config.h"
#include "rocket/common/log.h"
#include "rocket/net/tcp/tcp_client.h"
#include "rocket/net/tcp/net_addr.h"
#include "rocket/net/string_coder.h"
#include "rocket/net/abstract_protocol.h"

void test_connect() {

  // 调用 conenct 连接 server
  // wirte 一个字符串
  // 等待 read 返回结果

  int fd = socket(AF_INET, SOCK_STREAM, 0);

  if (fd < 0) {
    ERRORLOG("invalid fd %d", fd);
    exit(0);
  }

  sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(12346);
  inet_aton("127.0.0.1", &server_addr.sin_addr);

  int rt = connect(fd, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));

  DEBUGLOG("connect success");

  std::string msg = "hello rocket!";
  
  rt = write(fd, msg.c_str(), msg.length());

  DEBUGLOG("success write %d bytes, [%s]", rt, msg.c_str());

  char buf[100];
  rt = read(fd, buf, 100);
  DEBUGLOG("success read %d bytes, [%s]", rt, std::string(buf).c_str());

}

void test_tcp_client() {

  rocket::IPNetAddr::s_ptr addr = std::make_shared<rocket::IPNetAddr>("127.0.0.1", 12346);
  rocket::TcpClient client(addr);
  client.connect([addr, &client]() {
   // DEBUGLOG("conenct to [%s] success", addr->toString().c_str());
    std::shared_ptr<rocket::StringProtocol> message = std::make_shared<rocket::StringProtocol>();
    message->info = "hello rocket";
    message->setReqId ("123456");
    client.writeMessage(message, [](rocket::AbstractProtocol::s_ptr msg_ptr) {
      DEBUGLOG("send message success");
    });

    client.readMessage("123456", [](rocket::AbstractProtocol::s_ptr msg_ptr) {
      std::shared_ptr<rocket::StringProtocol> message = std::dynamic_pointer_cast<rocket::StringProtocol>(msg_ptr);
      DEBUGLOG("req_id[%s], get response %s", message->getReqId().c_str(), message->info.c_str());
    });

    client.writeMessage(message, [](rocket::AbstractProtocol::s_ptr msg_ptr) {
      DEBUGLOG("send message 22222 success");
    });
  });
}

int main() {

  rocket::Config::SetGlobalConfig("../conf/rocket.xml");

  rocket::Logger::InitGlobalLogger();

  // test_connect();

  test_tcp_client();

  return 0;
}