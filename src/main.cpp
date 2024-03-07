#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <fstream>

#include <signal.h>
#include <stdlib.h>

#include "../include/json.hpp"

using json = nlohmann::json;

static volatile UA_Boolean running = true;
static void stopHandler(int sig) {
    running = false;
}

int main() {
    std::ifstream f("../server1.json");
    json data = json::parse(f);

    char * s = data["Node"][0]["type"];

    // signal(SIGINT, stopHandler);
    // signal(SIGTERM, stopHandler);

    // UA_Server *server = UA_Server_new();  
    // UA_ServerConfig_setDefault(UA_Server_getConfig(server));

    // UA_StatusCode ret = UA_Server_run(server, &running);

    // UA_Server_delete(server);
    // return ret == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
