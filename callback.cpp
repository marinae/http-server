#include "callback.h"

//+----------------------------------------------------------------------------+
//| Accept callback                                                            |
//+----------------------------------------------------------------------------+

void accept_cb(evutil_socket_t evs, short events, void *ptr) {

    #ifdef _DEBUG_MODE_
    printf("server: accept_cb\n");
    #endif

    /* Last parameter is http server object */
    HTTPServer *srv = (HTTPServer *)ptr;

    /* Accept incoming connection */
    int fd = accept(evs, 0, 0);
    srv->addClient(fd);
}

//+----------------------------------------------------------------------------+
//| Accept connection in worker                                                |
//+----------------------------------------------------------------------------+

void worker_cb(evutil_socket_t evs, short events, void *ptr) {
    
    int client_fd;

    ssize_t size;

    int BUF_LEN = 32;
    char buf[BUF_LEN];

    struct msghdr   msg;
    struct iovec    iov;
    union {
        struct cmsghdr  cmsghdr;
        char            control[CMSG_SPACE(sizeof(int))];
    } cmsgu;
    struct cmsghdr  *cmsg;

    iov.iov_base = (void *)buf;
    iov.iov_len  = BUF_LEN;

    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = cmsgu.control;
    msg.msg_controllen = sizeof(cmsgu.control);
    size = recvmsg(evs, &msg, 0);
    if (size < 0) {
        perror("recvmsg");
        exit(1);
    }
    cmsg = CMSG_FIRSTHDR(&msg);
    if (cmsg && cmsg->cmsg_len == CMSG_LEN(sizeof(int))) {
        if (cmsg->cmsg_level != SOL_SOCKET) {
            fprintf(stderr, "invalid cmsg_level %d\n",
                 cmsg->cmsg_level);
            exit(1);
        }
        if (cmsg->cmsg_type != SCM_RIGHTS) {
            fprintf(stderr, "invalid cmsg_type %d\n",
                 cmsg->cmsg_type);
            exit(1);
        }

        client_fd = *((int *)CMSG_DATA(cmsg));
        printf("worker: received fd = %d\n", client_fd);
    } else {
        client_fd = -1;
    }

    if (client_fd != -1) {

        /* Last parameter is http server object */
        HTTPServer *srv = (HTTPServer *)ptr;

        /* Add client to worker process */
        srv->addWorkerClient(client_fd);
    }
}

//+----------------------------------------------------------------------------+
//| Read callback                                                              |
//+----------------------------------------------------------------------------+

void read_cb(evutil_socket_t evs, short events, void *ptr) {

    #ifdef _DEBUG_MODE_
    printf("client %d: read_cb\n", evs);
    //syslog(LOG_DEBUG, "client %d: read_cb\n", evs);
    #endif

    /* Last parameter is http server object */
    HTTPServer *srv = (HTTPServer *)ptr;

    /* Read from socket */
    char buf[BUF_SIZE];
    ssize_t len = recv(evs, buf, BUF_SIZE, 0);

    if (len == 0) {
        /* Close connection */
        srv->finishReading(evs);

    } else if (len != -1) {

        /* Add string from inBuf */
        std::string header(buf, len);
        std::string inBuf = srv->getInBuf(evs);

        if (!inBuf.empty())
            header = inBuf + header;

        /* Set response to client */
        srv->setResponse(evs, header);
    }
}

//+----------------------------------------------------------------------------+
//| Write callback                                                             |
//+----------------------------------------------------------------------------+

void write_cb(evutil_socket_t evs, short events, void *ptr) {

    #ifdef _DEBUG_MODE_
    printf("client %d: write_cb\n", evs);
    #endif

    /* Last parameter is http server object */
    HTTPServer *srv = (HTTPServer *)ptr;

    bool err_flag = false;

    /* Answer with a string from outBuf */
    std::string resp = srv->getResponse(evs);
    assert(resp.size() > 0);

    while (!resp.empty() && !err_flag) {

        /* Send response */
        ssize_t sent = send(evs, resp.c_str(), resp.size(), 0);

        #ifdef _DEBUG_MODE_
        printf("server: %zd/%zd bytes sent to client %d\n", sent, resp.size(), evs);
        #endif

        if (sent > 0) {
            /* Decrease string size */
            resp = resp.substr(sent);

        } else {
            err_flag = true;
        }
    }

    if (err_flag) {

        if (errno != EAGAIN) {
            /* Remove write event for client */
            srv->finishWriting(evs);
        } else {
            srv->pushToOutBuf(evs, resp);
        }

    } else {
        std::string str = srv->getInBuf(evs);

        if (str.empty()) {
            /* Remove write event for client */
            srv->finishWriting(evs);

        } else {
            srv->setResponse(evs, str);
        }
    }
}