#ifndef ROCKET_NET_ABSTRUCT_PROTOCOL_H
#define ROCKET_NET_ABSTRUCT_PROTOCOL_H

#include<memory>

namespace rocket{
class AbstructProtocol{
public:
    typedef std::shared_ptr<AbstructProtocol> s_ptr;
};
}

#endif