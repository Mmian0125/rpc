#include <tinyxml2.h>
#include "rocket/common/config.h"


using namespace tinyxml2;

#define READ_XML_NODE(name, parent) \
XMLElement* name##_node = parent->FirstChildElement(#name); \
if (!name##_node) { \
  printf("Start rocket server error, failed to read node [%s]\n", #name); \
  exit(0); \
} \



#define READ_STR_FROM_XML_NODE(name, parent) \
XMLElement* name##_node = parent->FirstChildElement(#name); \
  if (!name##_node|| !name##_node->GetText()) { \
    printf("Start rocket server error, failed to read config file %s\n", #name); \
    exit(0); \
  } \
  std::string name##_str = std::string(name##_node->GetText()); \



namespace rocket {


static Config* g_config = NULL;


Config* Config::GetGlobalConfig() {
  return g_config;
}

void Config::SetGlobalConfig(const char* xmlfile) {
  if (g_config == NULL) {
    g_config = new Config(xmlfile);
  }
}

Config::Config(const char* xmlfile) {
  if(xml_document.LoadFile(xmlfile)){
    printf("Start rocket server error, failed to read config file %s, error info[%d] \n", xmlfile, xml_document.ErrorID());
    exit(0);
  }

  XMLElement *root_node=xml_document.RootElement();
  READ_XML_NODE(log, root_node);


  READ_STR_FROM_XML_NODE(log_level, log_node);

  m_log_level = log_level_str;

}


}