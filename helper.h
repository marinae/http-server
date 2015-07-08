#ifndef __HELPER_H__
#define __HELPER_H__

#include "classes.h"
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <vector>
#include <fstream>

//+----------------------------------------------------------------------------+
//| Command line parameters class                                              |
//+----------------------------------------------------------------------------+

class CLParameters {
public:
	std::string dir;
	std::string ip;
	uint16_t    port;
	uint16_t    workers;
};

//+----------------------------------------------------------------------------+
//| Helper functions                                                           |
//+----------------------------------------------------------------------------+

CLParameters parseParams(int argc, char **argv);
int configSocket(CLParameters params);
std::string fillResponse(std::string path, RequestType type);
std::string composeResponse(HTTPRequest req, std::string path);
std::string readFile2String(std::string path);

#endif /* __HELPER_H__ */