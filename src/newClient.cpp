#include "newClient.hpp"

int main()
{
    signal(SIGINT, stopHandler);

    ifstream fs(clientJson);
    ifstream fv(variableJson);
    json dataSettings = json::parse(fs);
    json dataVariables = json::parse(fv);

    json settings = dataSettings["settings"];
    json variables = dataVariables["variables"];

    string address = settings[0]["address"];
    string login = settings[0]["login"];
    string password = settings[0]["password"];

    UA_Client *client = UA_Client_new();               // Создание клиента
    UA_ClientConfig *cc = UA_Client_getConfig(client); //
    UA_ClientConfig_setDefault(cc);                    //

    cc->timeout = 1000;

    bool isExist = true;

    while (running)
    {
        retval = UA_Client_connectUsername(client, address.c_str(), login.c_str(), password.c_str());
        if (retval == UA_STATUSCODE_BADCONNECTIONCLOSED) continue;
        if (retval != UA_STATUSCODE_GOOD) { UA_sleep_ms(1000); continue; }

        UA_BrowseResponse response = GetResponse(client);

        for (size_t i = 0; i < response.resultsSize; ++i)
        {
            for (int k = 0; k < variables.size(); k++)
                for (size_t j = 0; j < response.results[i].referencesSize; ++j)
                {
                    UA_ReferenceDescription *ref = &(response.results[i].references[j]);
                    if (ref->nodeClass != UA_NODECLASS_VARIABLE) { continue; } // Пропускаем узлы, которые не являются переменными

                    UA_NodeId nodeId = ref->nodeId.nodeId;
                    UA_ReadResponse readResponse = ReadResponse(client, nodeId);

                    if (readResponse.responseHeader.serviceResult == UA_STATUSCODE_GOOD && readResponse.resultsSize > 0 && readResponse.results[0].hasValue)
                    {
                        
                    }
                }
        }

        UA_sleep_ms(1000);
    };

    UA_Client_delete(client);
    return EXIT_SUCCESS;
}
