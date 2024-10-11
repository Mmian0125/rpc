#ifndef ROCKET_NET_CODER_TINYPB_PROTOCOL_H
#define ROCKET_NET_CODER_TINYPB_PROTOCOL_H

#include<string.h>
#include"rocket/net/coder/abstract_protocol.h"

namespace rocket{
struct TinyPBProtocol : public AbstractProtocol{
public:
     TinyPBProtocol(){}
    ~TinyPBProtocol() {}
public:
    static char PB_START;  //协议开始符
    static char PB_END;  //协议结束符
public:
    int32_t m_pk_len{0};  //整包长度
    int32_t m_req_id_len{0};  //req_id的长度
    //req_id继承父类
    int32_t m_method_name_len{0};  //方法名长度
    std::string m_method_name;  //方法名
    int32_t m_err_code{0};  //错误码
    int32_t m_err_info_len{0};  //错误信息长度
    std::string m_err_info;  //错误信息
    std::string m_pb_data;  //protobuf序列后化的message数据
    int32_t m_check_sum{0};  //校验和

    bool parse_success{false};
};
}

#endif