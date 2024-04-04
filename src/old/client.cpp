#include "client.hpp"

int main()
{
    signal(SIGINT, stopHandler);

    ifstream fs(clientJson);	
    json::value settings = json::value::parse(fs)["settings"];

    ifstream fv(variableJson);
	json::value variables = json::value::parse(fv)["variables"];
	
    UA_Client *client = InitClient();

    json::value varOut;

    while (running)
    {
        retval = UA_Client_connectUsername(client,
                                           (settings["address"].as_string().c_str()),
                                           (settings["login"].as_string().c_str()),
                                           (settings["password"]).as_string().c_str());

        // retval = UA_Client_connect(client, ((string)settings[0]["address"]).c_str());
        if (retval == UA_STATUSCODE_BADCONNECTIONCLOSED)
            continue;
        if (retval != UA_STATUSCODE_GOOD)
        {
            UA_sleep_ms(1000);
            continue;
        }

        UA_BrowseResponse response = GetResponse(client);

        for (size_t i = 0; i < response.resultsSize; ++i)
        {
            int findIndex = -1;

            for (int k = 0; k < variables.size(); k++)
            {
                int count = 0;
                for (size_t j = 0; j < response.results[i].referencesSize; ++j)
                {
                    UA_ReferenceDescription *ref = &(response.results[i].references[j]);
                    if (ref->nodeClass != UA_NODECLASS_VARIABLE)
                    {
                        continue;
                    } // Пропускаем узлы, которые не являются переменными

                    UA_NodeId nodeId = ref->nodeId.nodeId;
                    UA_ReadResponse readResponse = ReadResponse(client, nodeId);

                    if (readResponse.responseHeader.serviceResult == UA_STATUSCODE_GOOD && readResponse.resultsSize > 0 && readResponse.results[0].hasValue)
                    {
                        string name = UastrToCharArr(ref->displayName.text);
                        UA_Variant newValue;
                        UA_Variant value = readResponse.results[0].value;

                        if (name == variables[k]["name"].as_string().c_str())
                        {
                            CreateVariant(variables[k], &newValue);
                            retval = UA_Client_writeValueAttribute(client, nodeId, &newValue);
                            findIndex = k;
                            //varOut[variables[findIndex]["name"]] = variables[findIndex]["value"];
                            count++;
                            continue;
                        }

                        VariantToJson(varOut[UastrToCharArr(ref->displayName.text)], value);
                        count++;
                    }
                }

                if (findIndex < 0)
                {                    
                    UA_Variant newValue;
                    UA_Variant_init(&newValue);
                    CreateVariable(client,
                                   variables[k]["name"].as_string().c_str(),
                                   variables[k], &newValue);
                    //varOut[count]["value"] = variables[k]["value"];
                    count++;
                }
                findIndex = -1;
            }
        }
		
		cout << varOut.serialize() << endl;
        UA_sleep_ms(1000);
    };

    UA_Client_delete(client);
    return EXIT_SUCCESS;
}
