#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/plugin/log_stdout.h>

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

void printUAString(UA_String str) {
    printf("UA_String data: ");
    for(size_t i = 0; i < str.length; ++i) {
        printf("%c", str.data[i]);
    }
    printf("\n");
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
        request.nodesToBrowse[0].nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER); // Идентификатор корневой папки объектов сервера
        request.nodesToBrowse[0].resultMask = UA_BROWSERESULTMASK_ALL; // Запрашиваем все возможные результаты

        UA_BrowseResponse response = UA_Client_Service_browse(client, request);

        // Обработка ответа и чтение значений переменных
        for(size_t i = 0; i < response.resultsSize; ++i) {
            for(size_t j = 0; j < response.results[i].referencesSize; ++j) {
                UA_ReferenceDescription *ref = &(response.results[i].references[j]);
                if(ref->nodeClass != UA_NODECLASS_VARIABLE) {
                    continue; // Пропускаем узлы, которые не являются переменными
                }

                UA_NodeId nodeId = ref->nodeId.nodeId;
                UA_ReadRequest readRequest;
                UA_ReadRequest_init(&readRequest);
                readRequest.nodesToRead = UA_ReadValueId_new();
                readRequest.nodesToReadSize = 1;
                readRequest.nodesToRead[0].nodeId = nodeId;
                readRequest.nodesToRead[0].attributeId = UA_ATTRIBUTEID_VALUE;

                UA_ReadResponse readResponse = UA_Client_Service_read(client, readRequest);

                if(readResponse.responseHeader.serviceResult == UA_STATUSCODE_GOOD && readResponse.resultsSize > 0 && readResponse.results[0].hasValue) {
                    UA_Variant *value = &readResponse.results[0].value;
                    // Обработка значения переменной
                    printf("Узел: %.*s, Значение: ", (int)ref->displayName.text.length, ref->displayName.text.data);
                    UA_Variant_print(value);
                    printf("\n");
                }

                UA_ReadRequest_deleteMembers(&readRequest);
                UA_ReadResponse_deleteMembers(&readResponse);
            }
        }
        UA_sleep_ms(1000);
    };

    UA_Variant_clear(&value);
    UA_Client_delete(client);
    return EXIT_SUCCESS;
}
