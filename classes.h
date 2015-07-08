#ifndef __CLASSES_H__
#define __CLASSES_H__

#define _DEBUG_MODE_

#include <string>
#include <string.h>
#include <event.h>
#include <unordered_map>
#include <vector>
#include <assert.h>
#include <syslog.h>

//+----------------------------------------------------------------------------+
//| Definitions                                                                |
//+----------------------------------------------------------------------------+

const int BUF_SIZE       = 1024;
const int MAX_INBUF_SIZE = 4*1024;
const int MAX_ATTEMPTS   = 10;

enum RequestType { HEAD, GET, POST };

//+----------------------------------------------------------------------------+
//| HTTP request class                                                         |
//+----------------------------------------------------------------------------+

class HTTPRequest {
public:
    RequestType type;
    std::string file;
    bool        ok;

    HTTPRequest(std::string buf);
};

//+----------------------------------------------------------------------------+
//| Worker class                                                               |
//+----------------------------------------------------------------------------+

class Worker {
public:
    int    pid;
    int    fd;
    size_t clients;

    Worker(int pid, int fd, size_t clients) : pid(pid), fd(fd), clients(clients) {}
};

typedef std::vector<Worker> ServWorkers;

//+----------------------------------------------------------------------------+
//| Client class                                                               |
//+----------------------------------------------------------------------------+

class Client {
public:
    /* Events */
    struct event *readEv;
    struct event *writeEv;

    /* In/out buffers */
    std::string inBuf;
    std::string outBuf;

    Client() {}
    Client(struct event *readEv, struct event *writeEv) :
        readEv(readEv), writeEv(writeEv) {}
    ~Client();
};

typedef std::unordered_map<int, Client *> ServClients;

//+----------------------------------------------------------------------------+
//| Parse request                                                              |
//+----------------------------------------------------------------------------+

int parseRequest(std::string buf, RequestType &type, std::string &file, bool &ok);

#endif /* __CLASSES_H__ */