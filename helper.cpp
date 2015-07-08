#include "helper.h"

//+----------------------------------------------------------------------------+
//| Parse command line parameters                                              |
//+----------------------------------------------------------------------------+

CLParameters parseParams(int argc, char **argv) {

	CLParameters params = {"", "", 0, 0};

    char p;
    while ((p = getopt(argc, argv, "d:h:p:w:")) != -1) {
        switch (p) {
            case 'd': params.dir = std::string(optarg);
                      break;
            case 'h': params.ip = std::string(optarg);
                      break;
            case 'p': params.port = atoi(optarg);
                      break;
            case 'w': params.workers = atoi(optarg);
                      break;
            case '?': params = {"", "", 0, 0};
                      return params;
        }
    }

	return params;
}

//+----------------------------------------------------------------------------+
//| Configure master socket                                                    |
//+----------------------------------------------------------------------------+

int configSocket(CLParameters params) {
    
	/* Create TCP socket for handling incoming connections */
    int masterSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (masterSocket == -1) {
        std::cout << strerror(errno) << std::endl;
        return -1;
    }

    /* Fill parameters */
    struct sockaddr_in sAddr;
    bzero(&sAddr, sizeof(sAddr));
    sAddr.sin_family = AF_INET;
    sAddr.sin_port   = htons(params.port);
    inet_pton(AF_INET, params.ip.c_str(), &(sAddr.sin_addr));

    /* Bind socket with specific parameters */
    int result = bind(masterSocket, (struct sockaddr*)&sAddr, sizeof(sAddr));
    if (result == -1) {
        std::cout << strerror(errno) << std::endl;
        return -1;
    }
    
    /* Set socket as non-blocking */
    evutil_make_socket_nonblocking(masterSocket);
    
    /* Start listening socket */
    result = listen(masterSocket, SOMAXCONN);
    if (result == -1) {
        std::cout << strerror(errno) << std::endl;
        return -1;
    }

    return masterSocket;
}

//+----------------------------------------------------------------------------+
//| Fill response with file contents                                           |
//+----------------------------------------------------------------------------+

std::string fillResponse(std::string path, RequestType type) {

    std::string str("HTTP/1.1 ");

    /* Put current date and time to std::string */
    time_t rawtime;
    struct tm *timeinfo;
    char buf[80];

    time(&rawtime);
    timeinfo = gmtime(&rawtime);
    strftime(buf, 80, "Date: %a, %d %b %Y %T GMT\r\n", timeinfo);

    std::string dateString(buf);

    if (path.empty()) {
        /* File not found */
        str.append("404 Not Found\r\n");
        str.append(dateString);

        if (type == RequestType::GET || type == RequestType::POST) {
            /* GET/POST */
            str.append("Content-Type: text/html\r\n");
            std::string info("404 File not found. Sorry :)\n");
            str.append("Content-Length: ");
            str.append(std::to_string(info.size()));

            str.append("\r\n\r\n");
            str.append(info);
        } else {
            /* HEAD */
            str.append("\r\n\r\n");
        }

    } else {
        /* File exists */
        str.append("200 OK\r\n");
        str.append(dateString);

        if (type == RequestType::GET || type == RequestType::POST) {
            /* GET/POST - fill content type field */
            size_t found = path.find(".jpg");
            if (found == path.size() - 4) {
                /* Image */
                str.append("Content-Type: image/jpeg\r\n");
                str.append("Content-Transfer-Encoding: binary\r\n");

            } else {
                found = path.find(".html");
                if (found == path.size() - 5) {
                    /* HTML */
                    str.append("Content-Type: text/html\r\n");
                }
            }

            std::string text = readFile2String(path);
            str.append("Content-Length: ");
            str.append(std::to_string(text.size()));

            str.append("\r\n\r\n");
            str.append(text);

        } else {
            /* HEAD */
            str.append("\r\n\r\n");
        }
    }
    return str;
}

//+----------------------------------------------------------------------------+
//| Compose response string                                                    |
//+----------------------------------------------------------------------------+

std::string composeResponse(HTTPRequest req, std::string path) {

    std::string resp;
    path.append(req.file);

    /* Check file string */
    if (req.file == "/" || req.file.find("..") != std::string::npos) {
        /* Fill response string */
        resp = fillResponse("", req.type);

    } else {
        /* Find requested file */
        int file = open(path.c_str(), O_RDONLY);

        if (file != -1) {
            /* Fill response string */
            resp = fillResponse(path, req.type);
            close(file);

        } else {
            /* File doesn't exist (status 404) */
            resp = fillResponse("", req.type);
        }
    }

    return resp;
}

//+----------------------------------------------------------------------------+
//| Read whole file to std::string                                             |
//+----------------------------------------------------------------------------+

std::string readFile2String(std::string path) {
    
    /* Create input filestream */
    std::ifstream ifs(path.c_str(), std::ios::in | std::ios::binary | std::ios::ate);

    /* Compute file size */
    std::ifstream::pos_type fileSize = ifs.tellg();
    ifs.seekg(0, std::ios::beg);

    /* Read file to vector<char> */
    std::vector<char> bytes(fileSize);
    ifs.read(&bytes[0], fileSize);

    return std::string(&bytes[0], fileSize);
}