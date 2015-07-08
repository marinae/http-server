/* Mac OS X */

#include "server.h"

//+----------------------------------------------------------------------------+
//| Main                                                                       |
//+----------------------------------------------------------------------------+

int main(int argc, char **argv) {

    /* Parse server parameters */
    CLParameters params = parseParams(argc, argv);

    if (params.dir.empty() || params.ip.empty() || params.port == 0) {
        #ifdef _DEBUG_MODE_
        printf("error: Bad params\n");
        #endif
        return 1;
    }

    #ifdef _DEBUG_MODE_
    printf("dir:\t\t%s\n", params.dir.c_str());
    printf("ip:\t\t%s\n", params.ip.c_str());
    printf("port:\t\t%d\n", params.port);
    printf("workers:\t%d\n", params.workers);
    #endif

    /* Create and configure server */
    HTTPServer srv(params);
    
    /* Start server */
    srv.start();

    return 0;
}