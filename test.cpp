#include "string.h"
#include <string>
#include <stdlib.h>
#include <fcntl.h>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/event.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

const int BUF_SIZE = 1024;

int main() {

    size_t num_clients;
    std::cin >> num_clients;

    /* Fill parameters */
    struct sockaddr_in sAddr;
    bzero(&sAddr, sizeof(sAddr));
    sAddr.sin_family = AF_INET;
    sAddr.sin_port   = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &(sAddr.sin_addr));

    /* GET request */
    #pragma omp parallel for
    for (int i = 0; i < num_clients; ++i) {

        /* Create TCP socket for handling incoming connections */
        int slave = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        setsockopt(slave, SOL_SOCKET, SO_REUSEADDR, (void *)1, sizeof(int));
        
        if (slave != -1) {

            /* Bind socket with specific parameters */
            int result = connect(slave, (struct sockaddr*)&sAddr, sizeof(sAddr));
            
            if (result != -1) {

                std::string req = std::string("GET /colors.html HTTP/1.1");
                std::string str = req + std::string("\r\n\r\n");

                printf("%d) Request: [%s]\n", i, req.c_str());
                send(slave, str.c_str(), str.size(), 0);
            }
        }
    }

    /* Buffer for reading */
    //char buf[BUF_SIZE];

    /* Read response */
    /*while (true) {
        ssize_t len = recv(slave, buf, BUF_SIZE, 0);

        if (len > 0) {
            std::string str(buf, len);
            printf("string received:\n[%s]\n", str.c_str());
        }
    } */

    /*// HEAD request
    str = std::string("HEAD /dali.jpg HTTP/1.1\r\n\r\n");
    send(slave, str.c_str(), str.size(), 0);

    //std::string post = std::string("file=/moscow.jpg");

    // POST request
    str = std::string("POST / HTTP/1.1\r\n");
    str.append("Content-Type: application/x-www-form-urlencoded\r\n");
    str.append("Content-Length: ");
    str.append(std::to_string(post.size()));
    str.append("\r\n\r\n");
    str.append(post);
    send(slave, str.c_str(), str.size(), 0);*/

	return 0;
}