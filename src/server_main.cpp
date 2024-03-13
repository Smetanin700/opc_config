#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/plugin/securitypolicy.h>
#include <open62541/client_subscriptions.h>
#include <open62541/plugin/log_stdout.h>


#include <signal.h>
#include <stdlib.h>

static volatile UA_Boolean running = true;
static void stopHandler(int sign) {
    running = false;
}

static void dataChangeNotificationCallback(UA_Server *server, UA_UInt32 monitoredItemId,
                               void *monitoredItemContext, const UA_NodeId *nodeId,
                               void *nodeContext, UA_UInt32 attributeId,
                               const UA_DataValue *value) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Received Notification");
}

static void addMonitoredItemVariable(UA_Server *server, UA_NodeId nodeId) {    
    UA_MonitoredItemCreateRequest monRequest = UA_MonitoredItemCreateRequest_default(nodeId);
    monRequest.requestedParameters.samplingInterval = 100.0; 
    UA_Server_createDataChangeMonitoredItem(server, UA_TIMESTAMPSTORETURN_SOURCE, monRequest, NULL, dataChangeNotificationCallback);
}

int main(void) {
    UA_Server *server = UA_Server_new();
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));

    // Создание переменной 1
    UA_Int32 variable1 = 42;
    UA_VariableAttributes attr1 = UA_VariableAttributes_default;
    attr1.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    attr1.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_Variant_setScalar(&attr1.value, &variable1, &UA_TYPES[UA_TYPES_INT32]);
    UA_QualifiedName var1Name = UA_QUALIFIEDNAME(1, "Variable1");
    UA_NodeId var1NodeId = UA_NODEID_STRING(1, "Variable1");
    UA_Server_addVariableNode(server, var1NodeId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                              var1Name, UA_NODEID_NULL, attr1, NULL, NULL);

    // Создание переменной 2
    UA_Double variable2 = 3.14;
    UA_VariableAttributes attr2 = UA_VariableAttributes_default;
    attr1.dataType = UA_TYPES[UA_TYPES_DOUBLE].typeId;
    attr2.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_Variant_setScalar(&attr2.value, &variable2, &UA_TYPES[UA_TYPES_DOUBLE]);
    UA_QualifiedName var2Name = UA_QUALIFIEDNAME(1, "Variable2");
    UA_NodeId var2NodeId = UA_NODEID_STRING(1, "Variable2");
    UA_Server_addVariableNode(server, var2NodeId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                              var2Name, UA_NODEID_NULL, attr2, NULL, NULL);

    addMonitoredItemVariable(server, var1NodeId);   
    addMonitoredItemVariable(server, var2NodeId);   

    // Запуск сервера
    UA_StatusCode retval = UA_Server_run(server, &running);
    
    // Очистка ресурсов
    UA_Server_delete(server);

    return (int)retval;
}
