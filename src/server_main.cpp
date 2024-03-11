#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include <signal.h>
#include <stdlib.h>

static volatile UA_Boolean running = true;
static void stopHandler(int sig) {
    running = false;
}

int main() {    
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    UA_Server *server = UA_Server_new();  
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));

    UA_StatusCode ret = UA_Server_run(server, &running);

    UA_Server_delete(server);
    return ret == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
