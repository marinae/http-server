#ifndef __SERVER_H__
#define __SERVER_H__

#include "classes.h"
#include "helper.h"
#include "callback.h"
#include <assert.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <cstdlib>

static const int  PARENT     = 0;
static const int  CHILD      = 1;
static const char FILENAME[] = "shm";

//+----------------------------------------------------------------------------+
//| HTTP server class                                                          |
//+----------------------------------------------------------------------------+

class HTTPServer {

	struct event_base *base;
	struct event      *mainEvent;
	CLParameters      params;
	int               master;
	ServClients       clients;
	ServWorkers       workers;
	void              *sh_mem;
	size_t            mem_size;

public:
    HTTPServer(CLParameters p) : params(p) {}
    ~HTTPServer();

	void        start();
	void        addClient(int fd);
	void        addWorkerClient(int fd);
	void        removeClient(int fd);
	std::string getResponse(int fd);
	void        setResponse(int fd, std::string header);
	void        finishReading(int fd);
	void        finishWriting(int fd);
	std::string getInBuf(int fd);
	void        pushToInBuf(int fd, std::string str);
	std::string getOutBuf(int fd);
	void        pushToOutBuf(int fd, std::string str);;
};

#endif /* __SERVER_H__ */