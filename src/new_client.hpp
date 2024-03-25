#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/plugin/log_stdout.h>

#include <signal.h>
#include <stdlib.h>

#include <fstream>
#include <iostream>

#include "../include/json.hpp"

using json = nlohmann::json;
using namespace std;

UA_Boolean running = true;
UA_StatusCode retval;

char *clientJson = "../configs/client_test.json";
char *variableJson = "../configs/variable_test.json";

typedef enum
{
    INT32 = 1,
    BOOL = 2, 
    FLOAT = 3,
    STRING = 4,
    NONETYPE = 0
} DataType;

static void stopHandler(int sign)
{
    running = 0;
}

inline UA_BrowseResponse GetResponse(UA_Client *client) // Получение данных о переменных
{
    UA_BrowseRequest request;
    UA_BrowseRequest_init(&request);
    request.nodesToBrowse = UA_BrowseDescription_new();
    request.nodesToBrowseSize = 1;
    request.nodesToBrowse[0].nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER); // Идентификатор корневой папки объектов сервера
    request.nodesToBrowse[0].resultMask = UA_BROWSERESULTMASK_ALL;                  // Запрашиваем все возможные результаты
    return UA_Client_Service_browse(client, request);
}

inline UA_ReadResponse ReadResponse(UA_Client *client, UA_NodeId nodeId)
{
    UA_ReadRequest readRequest;
    UA_ReadRequest_init(&readRequest);
    readRequest.nodesToRead = UA_ReadValueId_new();
    readRequest.nodesToReadSize = 1;
    readRequest.nodesToRead[0].nodeId = nodeId;
    readRequest.nodesToRead[0].attributeId = UA_ATTRIBUTEID_VALUE;
    return UA_Client_Service_read(client, readRequest);
}

inline UA_DataType *GetDataType(DataType type)
{
    UA_DataType nodeType;
    switch (type)
    {
    case INT32:
        nodeType = UA_TYPES[UA_TYPES_INT32];
        break;
    case FLOAT:
        nodeType = UA_TYPES[UA_TYPES_FLOAT];
        break;
    case STRING:
        nodeType = UA_TYPES[UA_TYPES_STRING];
        break;
    case BOOL:
        nodeType = UA_TYPES[UA_TYPES_BOOLEAN];
        break;
    default:
        break;
    }
    return &nodeType;
}

inline DataType GetTypeJson(json value)
{
    if(value.is_number_float())
        return FLOAT;
    if(value.is_boolean())
        return BOOL;
    if(value.is_number_integer())
        return INT32;
    if(value.is_string())
        return STRING;
    return NONETYPE;    
}

inline void SetVariant(json value, UA_Variant *variant)
{
    switch (GetTypeJson(value))
    {
    case INT32:
    {
        UA_Int32 intval = value["value"];
        UA_Variant_setScalar(variant, &intval, &UA_TYPES[UA_TYPES_INT32]);
        break;
    }
    case FLOAT:
    {
        UA_Float floatval = value["value"];
        UA_Variant_setScalar(variant, &floatval, &UA_TYPES[UA_TYPES_FLOAT]);
        break;
    }
    case STRING:
    {
        UA_String strval = UA_String_fromChars(((string)value["value"]).c_str());
        UA_Variant_setScalar(variant, &strval, &UA_TYPES[UA_TYPES_STRING]);
        break;
    }
    case BOOL:
    {
        UA_Boolean boolval = value["value"];
        UA_Variant_setScalar(variant, &boolval, &UA_TYPES[UA_TYPES_BOOLEAN]);
        break;
    }
    default:
        break;
    }
}

inline UA_NodeId CreateVariable(UA_Client *client, char *name, DataType type)
{
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

    attr.displayName = UA_LOCALIZEDTEXT("en-US", name);

    attr.dataType = GetDataType(type)->typeId;
    attr.valueRank = -1;

    UA_NodeId varNodeId = UA_NODEID_NULL;
    retval = UA_Client_addVariableNode(client, varNodeId,
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                       UA_QUALIFIEDNAME(1, name),
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                       attr, &varNodeId);
    return varNodeId;
}

inline char *UastrToCharArr(UA_String uastr)
{
    char *name = (char *)UA_malloc(sizeof(char) * uastr.length + 1);
    memccpy(name, uastr.data, 0, uastr.length);
    return name;
}

inline void *__restrict__ PrepareValue()
{

}
