#include "kv-config.h"
#include <iostream>
#include <sstream>

int parseConfig
(
    std::string &dbpath,
    const std::string &config
) {
    std::stringstream ss(config);
    std::string s;
    if (std::getline(ss, s,'\n')) {
        dbpath = s;
    } else {
        dbpath = "";
    }
	return 0;
}

std::string getConfigString
(
    std::string &dbpath
) {
    std::stringstream ss;
    ss << dbpath << std::endl;
	return ss.str();
}
