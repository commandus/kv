#include <string>

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
);

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
);
