#include "lora-cli-config.h"
#include <iostream>
#include <sstream>

static std::string DEF_SERVICE_ADDRESS = "ws://vega.rcitsakha.ru:8002";

/**
 *  config file consists of lines:
 *  service address, default ws://vega.rcitsakha.ru:8002
 *  login
 *  password
 *  conninfo
 */
int parseConfig(
  std::string &listenAddress,
  std::string &login,
  std::string &password,
  std::string &conninfo,
  const std::string &config
) {
    std::stringstream ss(config);
    std::string s;
    if (std::getline(ss, s,'\n')) {
        listenAddress = s;
    }
    if (std::getline(ss, s,'\n')) {
        login = s;
    }
    if (std::getline(ss, s,'\n')) {
        password = s;
    }
    if (std::getline(ss, s,'\n')) {
        conninfo = s;
    }
	if (listenAddress.empty())
        listenAddress = DEF_SERVICE_ADDRESS;
	return 0;
}

/**
 *  config file consists of lines:
 *  service address, default ws://vega.rcitsakha.ru:8002
 *  login
 *  password
 *  conninfo
 */
std::string getConfigString(
  const std::string &listenAddress,
  const std::string &login,
  const std::string &password,
  const std::string &conninfo
) {
    std::stringstream ss;
    ss << listenAddress << std::endl
    	<< login << std::endl
    	<< password << std::endl
    	<< conninfo << std::endl;
	return ss.str();
}
