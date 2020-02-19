#include <string>

/**
 *  config file consists of lines:
 *  database path
 */
int parseConfig(
  std::string &dbpath,
  const std::string &config
);

/**
 *  config file consists of lines:
 *  database path
 */
std::string getConfigString(
  std::string &dbpath
);
