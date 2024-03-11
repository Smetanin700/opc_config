#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>

#include <signal.h>
#include <stdlib.h>

#include <fstream>
#include <iostream>

#include "../include/json.hpp"

using json = nlohmann::json;

UA_Boolean running = true;

static void stopHandler(int sign) {
    running = 0;
}

int main() {    
    std::ifstream f("../server1.json");
    json data = json::parse(f);
    json object = data["settings"];
    std::string address = object[0]["ip"];   
    int port = object[0]["port"];   

    std::string opc = "opc.tcp://";

    opc += address;
    opc += ":";
    opc += std::to_string(port);    

    signal(SIGINT, stopHandler);

    UA_Client *client = UA_Client_new();
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    UA_ClientConfig_setDefault(cc);

    cc->timeout = 1000;

    UA_StatusCode retval;
    UA_Variant value;
    UA_Variant_init(&value);

    while(running) {
        retval = UA_Client_connect(client, opc.c_str());
        if(retval == UA_STATUSCODE_BADCONNECTIONCLOSED) {
            continue;
        }
        if(retval != UA_STATUSCODE_GOOD) {
            UA_sleep_ms(1000);
            continue;
        }

        UA_BrowseRequest request;
        UA_BrowseRequest_init(&request);
        request.nodesToBrowse = UA_BrowseDescription_new();
        request.nodesToBrowseSize = 1;
        request.nodesToBrowse[0].nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
        UA_BrowseResponse response = UA_Client_Service_browse(client, request);
        for(size_t i = 0; i < response.resultsSize; ++i) {
            for(size_t j = 0; j < response.results[i].referencesSize; ++j) {
                UA_ReferenceDescription ref = response.results[i].references[j];
                printf("%s \r\n", ref.displayName.locale);
            }
        }

        UA_sleep_ms(1000);
    };

    UA_Variant_clear(&value);
    UA_Client_delete(client);
    return EXIT_SUCCESS;
}
