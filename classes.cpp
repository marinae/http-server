#include "classes.h"

//+----------------------------------------------------------------------------+
//| Request constructor                                                        |
//+----------------------------------------------------------------------------+

HTTPRequest::HTTPRequest(std::string buf) {

    assert(!buf.empty());
	parseRequest(buf, type, file, ok);
}

//+----------------------------------------------------------------------------+
//| Client destructor                                                          |
//+----------------------------------------------------------------------------+

Client::~Client() {

	if (readEv) {
		event_free(readEv);
		readEv = NULL;
	}
	if (writeEv) {
		event_free(writeEv);
		writeEv = NULL;
	}
}

//+----------------------------------------------------------------------------+
//| Parse HTTP request                                                         |
//+----------------------------------------------------------------------------+

int parseRequest(std::string buf, RequestType &type, std::string &file, bool &ok) {

    size_t start = 0;
    ok = true;

    /* Parse request type */
    if (strncmp(buf.c_str(), "HEAD", 4) == 0) {
        type = RequestType::HEAD;
        start = 5;

    } else if (strncmp(buf.c_str(), "GET", 3) == 0) {
        type = RequestType::GET;
        start = 4;

    } else if (strncmp(buf.c_str(), "POST", 4) == 0) {
        type = RequestType::POST;
        start = 5;

    } else {
        ok = false;
    }

    /* Parse requested file */
    if (ok) {
        if (type == RequestType::POST) {
            /* POST - parse file name from body */
            size_t found = buf.find("\r\n\r\nfile=");

            if (found != std::string::npos) {
                /* Parameter found */
                size_t fileBegin = found + 9;
                file = buf.substr(fileBegin, buf.size() - fileBegin);
            }

        } else {
            /* GET/HEAD - find space */
            size_t end = start;

            /* Find end of string */
            while (end < buf.size() && buf[end] != ' ')
                ++end;

            if (end > start)
                file = buf.substr(start, end - start);
        }
    }

    return ok;
}