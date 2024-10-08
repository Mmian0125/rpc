#ifndef ROCKET_COMMON_CONFIG_H
#define ROCKET_COMMON_CONFIG_H

#include <map>
#include<tinyxml2.h>


namespace rocket {

class Config {
 public:

  Config(const char* xmlfile);

 public:
  static Config* GetGlobalConfig();
  static void SetGlobalConfig(const char* xmlfile);

 public:
  std::string m_log_level;
   tinyxml2::XMLDocument xml_document;

};


}

#endif