#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/plugin/log_stdout.h>

#include <signal.h>
#include <stdlib.h>

#include <fstream>
#include <iostream>

#include <cpprest/json.h>

using namespace std;
using namespace web;
using namespace web::json;

UA_Boolean running = true;
UA_StatusCode retval;

char *clientJson = "../configs/client_test.json";
char *variableJson = "../configs/variable_test.json";

typedef enum
{
    INT32 = 1,
    BOOL = 2, 
    DOUBLE = 3,
    STRING = 4,
    NONETYPE = 0
} DataType;

static void stopHandler(int sign)
{
    running = 0;
}

/*
Создание клиента
*/
inline UA_Client *InitClient()
{
    UA_Client *client = UA_Client_new();               
    UA_ClientConfig *cc = UA_Client_getConfig(client); 
    UA_ClientConfig_setDefault(cc);    
    cc->timeout = 1000;
    return client;
}

/*
Отправка запроса на получение переменных сервера
*/
inline UA_BrowseResponse GetResponse(UA_Client *client) 
{
    UA_BrowseRequest request;
    UA_BrowseRequest_init(&request);
    request.nodesToBrowse = UA_BrowseDescription_new();
    request.nodesToBrowseSize = 1;
    request.nodesToBrowse[0].nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER); 
    request.nodesToBrowse[0].resultMask = UA_BROWSERESULTMASK_ALL;                  
    return UA_Client_Service_browse(client, request);
}

/*
Чтение ответа от сервера
*/
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

/*
Получение типа переменной в json 
*/
inline DataType GetJsonType(json::value value)
{
    if (value["value"].is_double())
        return DOUBLE;
    else if (value["value"].is_boolean())
        return BOOL;
    else if (value["value"].is_integer())
        return INT32;
    else if (value["value"].is_string())
        return STRING;
    else
        return NONETYPE;
}

/*
Получение типа переменной open62541
*/
inline void GetDataType(DataType type, UA_DataType nodeType)
{    
    switch (type)
    {
    case INT32:
        nodeType = UA_TYPES[UA_TYPES_INT32];
        break;
    case DOUBLE:
        nodeType = UA_TYPES[UA_TYPES_DOUBLE];
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
}

/*
Получение типа переменной open62541
*/
inline void GetDataType(json::value value, UA_DataType nodeType)
{
    switch (GetJsonType(value))
    {
    case INT32:
        nodeType = UA_TYPES[UA_TYPES_INT32];
        break;
    case DOUBLE:
        nodeType = UA_TYPES[UA_TYPES_DOUBLE];
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
}

/*
Создание универсальной переменной для open62541
*/
inline void CreateVariant(json::value value, UA_Variant *variant)
{
    switch (GetJsonType(value))
    {
    case INT32:
    {
        UA_Int32 intval = value["value"].as_integer();
        UA_Variant_setScalar(variant, &intval, &UA_TYPES[UA_TYPES_INT32]);
        break;
    }
    case DOUBLE:
    {
        UA_Double doubleval = value["value"].as_double();
        UA_Variant_setScalar(variant, &doubleval, &UA_TYPES[UA_TYPES_DOUBLE]);
        break;
    }
    case STRING:
    {
        UA_String strval = UA_String_fromChars((value["value"].as_string()).c_str());
        UA_Variant_setScalar(variant, &strval, &UA_TYPES[UA_TYPES_STRING]);
        break;
    }
    case BOOL:
    {
        UA_Boolean boolval = value["value"].as_bool();
        UA_Variant_setScalar(variant, &boolval, &UA_TYPES[UA_TYPES_BOOLEAN]);
        break;
    }
    default:
        break;
    }
}

/*
Создание переменной на сервере
*/
inline void CreateVariable(UA_Client *client, const char *name, json::value value, UA_Variant *variant)
{
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    //attr.displayName = UA_LOCALIZEDTEXT("en-US", "name");
    // UA_DataType nodeType;
    // GetDataType(value, nodeType);
    // CreateVariant(value, variant);
    // attr.dataType = nodeType.typeId;
    attr.valueRank = -1;
    // UA_Variant_setScalar(&attr.value, variant, &nodeType);
    UA_NodeId varNodeId = UA_NODEID_STRING(1, "name123");
    retval = UA_Client_addVariableNode(client, varNodeId,
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                       UA_QUALIFIEDNAME(1, "name123"),
                                       UA_NODEID_NULL,
                                       attr, NULL);
}

/*
Преобразование UA_String в строку С
*/
inline char *UastrToCharArr(UA_String uastr)
{
    char *name = (char *)UA_malloc(sizeof(char) * uastr.length + 1);
    memccpy(name, uastr.data, 0, uastr.length);
    return name;
}

/*
Запись UA_Variant в json
*/
inline void VariantToJson(json::value value, UA_Variant variant)
{
    if(variant.type == &UA_TYPES[UA_TYPES_DOUBLE]){
        UA_Double val = *(UA_Double *)variant.data;        
        value["value"] = json::value::number(val);
    }
    if(variant.type == &UA_TYPES[UA_TYPES_INT32]){
        UA_Int32 val = *(UA_Int32 *)variant.data;
        value["value"] = json::value::number(val);
    }
    if(variant.type == &UA_TYPES[UA_TYPES_BOOLEAN]){
        UA_Boolean val = *(UA_Boolean *)variant.data;
        value["value"] = json::value::boolean(val);
    }
    if(variant.type == &UA_TYPES[UA_TYPES_STRING]){
        UA_String val = *(UA_String *)variant.data;
        value["value"] = json::value::string(UastrToCharArr(val));
    }
}
