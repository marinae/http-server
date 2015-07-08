#include "server.h"

//+----------------------------------------------------------------------------+
//| Server destructor                                                          |
//+----------------------------------------------------------------------------+

HTTPServer::~HTTPServer() {

    event_base_free(base);
    event_free(mainEvent);

    close(master);
    for (auto it = clients.begin(); it != clients.end(); ++it) {
        close(it->first);
        delete(it->second);
    }

    closelog();
}

//+----------------------------------------------------------------------------+
//| Start server                                                               |
//+----------------------------------------------------------------------------+

void HTTPServer::start() {

    #ifdef _DEBUG_MODE_
    printf("starting server...\n");
    #endif

    /* Open log file */
    openlog("log", LOG_PID, LOG_USER);

	/* Create TCP socket for handling incoming connections */
    master = configSocket(params);
    assert(master != -1);

    int pid;
    int ppid = getpid();

    /* Create workers */
    for (int i = 0; i < params.workers; ++i) {

        if (ppid == getpid()) {

            /* Create socket pair */
            int pair_fd[2];
            int result = socketpair(AF_UNIX, SOCK_DGRAM, 0, pair_fd);
            assert(result == 0);

            /* Fork process */
            pid = fork();

            if (pid == 0) {
                /* Worker process */
                close(pair_fd[PARENT]);
                master = pair_fd[CHILD];

                /* Create event base */
                base = event_base_new();

                mainEvent = event_new(base, master, EV_READ | EV_PERSIST, worker_cb, (void *)this);
                event_add(mainEvent, NULL);
                event_base_dispatch(base);

            } else {
                /* Server process */
                close(pair_fd[CHILD]);
                workers.push_back(Worker(pid, pair_fd[PARENT], 0));

                #ifdef _DEBUG_MODE_
                printf("worker %d: started\n", pid);
                #endif
            }  
        }
    }

    assert(ppid == getpid());

    /* Create event base */
    base = event_base_new();

    /* Parent server process */
    mainEvent = event_new(base, master, EV_READ | EV_PERSIST, accept_cb, (void *)this);
    event_add(mainEvent, NULL);

    /* Start event loop */
    event_base_dispatch(base);
}

//+----------------------------------------------------------------------------+
//| Add new client                                                             |
//+----------------------------------------------------------------------------+

void HTTPServer::addClient(int fd) {

    /* Select random worker */
    int w_id = rand() % params.workers;
    printf("server: worker %d (%d) selected\n", w_id, workers[w_id].pid);

    #ifdef _DEBUG_MODE_
    printf("server: new connection (%d) -> worker %d\n", fd, workers[w_id].pid);
    #endif

    int BUF_LEN = 32;
    char buf[BUF_LEN];

    /* Send descriptor to worker */
    ssize_t       size;
    struct msghdr msg;
    struct iovec  iov;
    union {
        struct cmsghdr cmsghdr;
        char           control[CMSG_SPACE(sizeof(int))];
    } cmsgu;
    struct cmsghdr *cmsg;

    iov.iov_base = buf;
    iov.iov_len  = BUF_LEN;

    msg.msg_name    = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov     = &iov;
    msg.msg_iovlen  = 1;

    if (fd != -1) {
        msg.msg_control = cmsgu.control;
        msg.msg_controllen = sizeof(cmsgu.control);

        cmsg = CMSG_FIRSTHDR(&msg);
        cmsg->cmsg_len = CMSG_LEN(sizeof(int));
        cmsg->cmsg_level = SOL_SOCKET;
        cmsg->cmsg_type = SCM_RIGHTS;

        printf ("server: passing fd = %d\n", fd);
        *((int *)CMSG_DATA(cmsg)) = fd;
    } else {
        msg.msg_control = NULL;
        msg.msg_controllen = 0;
        printf("server: not passing fd\n");
    }

    size = sendmsg(workers[w_id].fd, &msg, 0);

    if (size < 0)
        perror("sendmsg");

    close(fd);
}

//+----------------------------------------------------------------------------+
//| Add new client in worker process                                           |
//+----------------------------------------------------------------------------+

void HTTPServer::addWorkerClient(int fd) {

    printf("add worker client: %d (pid = %d)\n", fd, getpid());

    /* Create new event for client socket */
    struct event *ev;
    ev = event_new(base, fd, EV_READ | EV_PERSIST, read_cb, (void *)this);
    event_add(ev, NULL);

    assert(clients.find(fd) == clients.end());
    clients[fd] = new Client(ev, NULL);
}

//+----------------------------------------------------------------------------+
//| Remove client from list                                                    |
//+----------------------------------------------------------------------------+

void HTTPServer::removeClient(int fd) {

    assert(clients.find(fd) != clients.end());

    if (clients[fd]->readEv)
        event_free(clients[fd]->readEv);
    if (clients[fd]->writeEv)
        event_free(clients[fd]->writeEv);

    clients.erase(fd);
    close(fd);
}

//+----------------------------------------------------------------------------+
//| Get response for the client                                                |
//+----------------------------------------------------------------------------+

std::string HTTPServer::getResponse(int fd) {

    assert(clients.find(fd) != clients.end());

    std::string outBuf = clients[fd]->outBuf;
    clients[fd]->outBuf.clear();

    return outBuf;
}

//+----------------------------------------------------------------------------+
//| Set response to the client                                                 |
//+----------------------------------------------------------------------------+

void HTTPServer::setResponse(int fd, std::string header) {

    /* Parse HTTP header */
    HTTPRequest req(header);
    size_t pos = header.find("\r\n\r\n");

    /* Check if request is ok */
    if (!req.ok) {

        #ifdef _DEBUG_MODE_
        printf("client %d: error in header\n", fd);
        #endif

        return;
    }

    if (req.type == RequestType::POST) {
        /* Check content-length */
        size_t paramPos = header.find("Content-Length: ");
        size_t lenPos = paramPos + std::string("Content-Length: ").size();

        std::string part = header.substr(lenPos);
        size_t endPos = part.find('\n');
        part = part.substr(0, endPos);

        int contentLen = std::stoi(part);
        int realLen = header.size() - (pos + 4);

        if (realLen < contentLen) {
            /* Push to buffer and return */
            pushToInBuf(fd, header);
            return;

        } else if (realLen > contentLen) {
            /* Additional info */
            std::string toBuf = header.substr(lenPos + endPos + contentLen);
            pushToInBuf(fd, toBuf);
            header = header.substr(0, lenPos + endPos + contentLen);
        }
    } else {
        /* Check for end sequence */
        if (pos == std::string::npos) {
            /* Partial request */
            pushToInBuf(fd, header);
            return;

        } else if (pos < header.size() - 4) {
            std::string toBuf = header.substr(pos + 4);
            pushToInBuf(fd, toBuf);
            header = header.substr(0, pos + 4);
        }
    }

    #ifdef _DEBUG_MODE_
    std::string typeStr;
    std::string okStr = req.ok ? "ok" : "fail";
    if (req.type == RequestType::HEAD)
        typeStr = "HEAD";
    else if (req.type == RequestType::GET)
        typeStr = "GET";
    else if (req.type == RequestType::POST)
        typeStr = "POST";
    printf("server: %s request for file %s (%s)\n", typeStr.c_str(), req.file.c_str(), okStr.c_str());
    #endif

	/* Compose response to request */
	std::string resp = composeResponse(req, params.dir);
    
    assert(resp.size() > 0);
    assert(clients.find(fd) != clients.end());
    clients[fd]->outBuf.append(resp);

    if (!clients[fd]->writeEv) {
        /* Add write event */
        struct event *ev;
        ev = event_new(base, fd, EV_WRITE | EV_PERSIST, write_cb, (void *)this);
        event_add(ev, NULL);

        clients[fd]->writeEv = ev;
    }
}

//+----------------------------------------------------------------------------+
//| Finish reading from client socket                                          |
//+----------------------------------------------------------------------------+

void HTTPServer::finishReading(int fd) {

	#ifdef _DEBUG_MODE_
	printf("client %d: reading finished\n", fd);
	#endif
    
    assert(clients.find(fd) != clients.end());
    event_del(clients[fd]->readEv);

    event_free(clients[fd]->readEv);
    clients[fd]->readEv = NULL;

    if (!clients[fd]->writeEv) {

        #ifdef _DEBUG_MODE_
        printf("client %d: removed from list\n", fd);
        #endif

        /* Remove client */
        removeClient(fd);
    }
}

//+----------------------------------------------------------------------------+
//| Finish writing to client socket                                            |
//+----------------------------------------------------------------------------+

void HTTPServer::finishWriting(int fd) {

	#ifdef _DEBUG_MODE_
	printf("client %d: writing finished\n", fd);
	#endif

    assert(clients.find(fd) != clients.end());
    event_del(clients[fd]->writeEv);

    event_free(clients[fd]->writeEv);
    clients[fd]->writeEv = NULL;

    if (!clients[fd]->readEv) {

        #ifdef _DEBUG_MODE_
        printf("client %d: removed from list\n", fd);
        #endif

        /* Remove client */
        removeClient(fd);
    }
}

//+----------------------------------------------------------------------------+
//| Get string from incoming buffer                                            |
//+----------------------------------------------------------------------------+

std::string HTTPServer::getInBuf(int fd) {

    assert(clients.find(fd) != clients.end());
    std::string inBuf = clients[fd]->inBuf;
    clients[fd]->inBuf.clear();

    return inBuf;
}

//+----------------------------------------------------------------------------+
//| Push string to incoming buffer                                             |
//+----------------------------------------------------------------------------+

void HTTPServer::pushToInBuf(int fd, std::string str) {

    #ifdef _DEBUG_MODE_
    //printf("server: string [%s] of size=%lu pushed to buffer %d\n", str.c_str(), str.size(), fd);
    #endif

	assert(clients.find(fd) != clients.end());
	size_t len = clients[fd]->inBuf.size();

	if (len + str.size() <= MAX_INBUF_SIZE) {
		clients[fd]->inBuf.append(str);
	} else {
		/* Overflow */
		clients[fd]->inBuf.clear();
	}
}

//+----------------------------------------------------------------------------+
//| Push string to outcoming buffer                                            |
//+----------------------------------------------------------------------------+

void HTTPServer::pushToOutBuf(int fd, std::string str) {

    #ifdef _DEBUG_MODE_
    //printf("server: string [%s] of size=%lu pushed to buffer %d\n", str.c_str(), str.size(), fd);
    #endif

    assert(clients.find(fd) != clients.end());
    clients[fd]->outBuf.append(str);
}