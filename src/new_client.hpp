#define UA_ENABLE_ENCRYPTION_OPENSSL

#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/plugin/securitypolicy.h>
#include <open62541/plugin/securitypolicy_default.h>
#include <open62541/plugin/accesscontrol_default.h>

#include <signal.h>
#include <stdlib.h>
#include <fstream>
#include <iostream>
#include "../include/common.h"

#include <cpprest/json.h>

using namespace std;
using namespace web;
using namespace web::json;

UA_Boolean running = true;
UA_StatusCode retval = UA_STATUSCODE_BAD;

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

struct NodeData
{
    UA_NodeId nodeId;
    UA_Variant value;
    string name;
};

static void stopHandler(int sign)
{
    running = 0;
}


static UA_INLINE UA_ByteString
loadCert(const char *const data, size_t length) {
    UA_ByteString fileContents = UA_STRING_NULL;

    /* Get the file length, allocate the data and read */
    fileContents.length = length;
    fileContents.data = (UA_Byte *)data;

    return fileContents;
}

////////////////////////////////////////////
inline string GetByteString1()
{
    ifstream fin("../client_cert.der",ios_base::in|ios_base::binary);    
    fin.seekg(0,ios_base::end);
    size_t uSize = fin.tellg();
    fin.seekg(0);

    char* t = new char[uSize];
    fin.read(t,uSize);

    return string(t);
}

inline string GetByteString2()
{
    ifstream fin("../client_key.der",ios_base::in|ios_base::binary);
    fin.seekg(0,ios_base::end);
    size_t uSize = fin.tellg();
    fin.seekg(0);

    char* t = new char[uSize];
    fin.read(t,uSize);

    return string(t);
}
////////////////////////////////////

/*
Создание клиента
*/
inline UA_Client *InitClient()
{
    UA_Client *client = UA_Client_new();               
    UA_ClientConfig *cc = UA_Client_getConfig(client); 
    UA_ClientConfig_setDefault(cc);
    return client;
}


/*
Создание защищенного клиента
*/
inline UA_Client *InitSecureClient()
{
    ifstream fin("../client_cert.der",ios_base::in|ios_base::binary);    
    fin.seekg(0,ios_base::end);
    size_t uSize = fin.tellg();
    fin.seekg(0);

    char* t = new char[uSize];
    fin.read(t,uSize);

    UA_ByteString cert =  loadCert(t, uSize);
    UA_ByteString key = loadFile("../client_key.der");  

    UA_Client *client = UA_Client_new();

    UA_ClientConfig* cc = UA_Client_getConfig(client);

    // Set to default config with no trust and issuer list
    UA_ClientConfig_setDefaultEncryption(cc, cert, key, NULL, 0, NULL, 0);

    // Set securityMode and securityPolicyUri
    UA_ByteString_clear(&cc->securityPolicyUri);
    cc->securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;
    cc->securityPolicyUri = UA_String_fromChars("http://opcfoundation.org/UA/SecurityPolicy#Basic256");

    // Set uri and client type
    UA_ApplicationDescription_clear(&cc->clientDescription);
    UA_String_clear(&cc->clientDescription.applicationUri);
    cc->clientDescription.applicationUri = UA_STRING_ALLOC("urn:open62541.server.application");
    cc->clientDescription.applicationType = UA_APPLICATIONTYPE_CLIENT;

    return client;
}

/*
Отправка запроса на получение переменных сервера
*/
inline UA_BrowseResponse GetResponse(UA_Client *client, int identifier) 
{
    UA_BrowseRequest request;
    UA_BrowseRequest_init(&request);
    request.nodesToBrowse = UA_BrowseDescription_new();
    request.nodesToBrowseSize = 1;
    request.nodesToBrowse[0].nodeId = UA_NODEID_NUMERIC(0, identifier); 
    request.nodesToBrowse[0].resultMask = UA_BROWSERESULTMASK_ALL;    
    return UA_Client_Service_browse(client, request);
}

/*
Отправка запроса на получение переменных сервера
*/
inline UA_BrowseResponse GetResponseFromNode(UA_Client *client, UA_NodeId node) 
{
    UA_BrowseRequest request;
    UA_BrowseRequest_init(&request);
    request.nodesToBrowse = UA_BrowseDescription_new();
    request.nodesToBrowseSize = 1;
    request.nodesToBrowse[0].nodeId = node; 
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
Запись данных в универсальную переменную для open62541
*/
inline void WriteVariant(json::value value, UA_Variant *variant)
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
inline void CreateVariable(UA_Client *client, char *name, json::value value, UA_Variant *variant)
{
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    attr.valueRank = -1;
    UA_NodeId varNodeId = UA_NODEID_STRING(1, name);
    UA_NodeId newNode;
    retval = UA_Client_addVariableNode(client, varNodeId,
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                       UA_QUALIFIEDNAME(1, name),
                                       UA_NODEID_NULL,
                                       attr, &newNode);
    WriteVariant(value, variant);
    retval = UA_Client_writeValueAttribute(client, newNode, variant);
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
inline void VariantToJson(json::value &value, UA_Variant variant)
{
    if(variant.type == &UA_TYPES[UA_TYPES_DOUBLE]){
        cout << "double" << endl;
        UA_Double val = *(UA_Double *)variant.data;        
        value["value"] = json::value::number(val);
    }
    if(variant.type == &UA_TYPES[UA_TYPES_INT32]){
        cout << "int" << endl;
        UA_Int32 val = *(UA_Int32 *)variant.data;
        value["value"] = json::value::number(val);
    }
    if(variant.type == &UA_TYPES[UA_TYPES_BOOLEAN]){
        cout << "bool" << endl;
        UA_Boolean val = *(UA_Boolean *)variant.data;
        value["value"] = json::value::boolean(val);
    }
    if(variant.type == &UA_TYPES[UA_TYPES_STRING]){
        cout << "string" << endl;
        UA_String val = *(UA_String *)variant.data;
        value["value"] = json::value::string(UastrToCharArr(val));
    }
}
