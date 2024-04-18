#include "new_client.hpp"

int main()
{
    signal(SIGINT, stopHandler);

    ifstream fs(clientJson);	
    json::value settings = json::value::parse(fs)["settings"];

    ifstream fv(variableJson);
	json::value variables = json::value::parse(fv)["variables"];
	
    //UA_Client *client = InitClient();
    UA_Client *client = InitSecureClient();

    json::value varOut;

    retval = UA_STATUSCODE_BAD;
    while (retval != UA_STATUSCODE_GOOD)
    {
        if (!running) return -1;
        // retval = UA_Client_connectUsername(client,
        //                                     ((settings["address"].as_string() + ":" + settings["port"].as_string()).c_str()),
        //                                     (settings["login"].as_string().c_str()),
        //                                     (settings["password"]).as_string().c_str());
        
        retval = UA_Client_connect(client, ((settings["address"].as_string() + ":" + settings["port"].as_string()).c_str()));
        sleep_ms(1000);
    }

    if (retval != UA_STATUSCODE_GOOD)
    {
        cout << "Bad conection: " << retval << endl;        
        UA_Client_delete(client);
        return 1;
    }

    UA_BrowseResponse response = GetResponse(client, UA_NS0ID_OBJECTSFOLDER);

    int count = 0;
    json::value jsonOutput;

    if (response.resultsSize == 0)
    {
        cout << "No result" << endl;
    }

    int size = 0;

    if (response.resultsSize > 0)
        size = response.results[0].referencesSize;

    NodeData vars[size];

    for (size_t j = 0; j < size; ++j)
    {
        UA_ReferenceDescription *ref = &(response.results[0].references[j]);
        // if (ref->nodeClass == UA_NODECLASS_OBJECT) 
        // {
        //     UA_BrowseResponse resp = GetResponseFromNode(client, ref->nodeId.nodeId);
        // }        
        if (ref->nodeClass != UA_NODECLASS_VARIABLE) { continue; } // Пропускаем не переменные

        UA_NodeId nodeId = ref->nodeId.nodeId;
        UA_ReadResponse readResponse = ReadResponse(client, nodeId);
        if (readResponse.responseHeader.serviceResult == UA_STATUSCODE_GOOD && readResponse.resultsSize > 0 && readResponse.results[0].hasValue)
        {
            vars[count].name = UastrToCharArr(ref->displayName.text);
            vars[count].nodeId = nodeId;   
            vars[count].value = readResponse.results[0].value;
            jsonOutput[count]["name"] = json::value::string(vars[count].name);
            VariantToJson(jsonOutput[count], vars[count].value);
            count++;
        }
    }

    // int length = count;
    // bool find;
    // for (int i = 0; i < variables.size(); i++)
    // {
    //     find = false;
    //     for (int j = 0; j < length; j++) 
    //     {
    //         if (variables[i]["name"].as_string() == vars[j].name)
    //         {
    //             WriteVariant(variables[i], &vars[j].value);
    //             UA_Client_writeValueAttribute(client, vars[j].nodeId, &vars[j].value);
    //             for (int k = 0; k < jsonOutput.size(); k++)
    //                 if(jsonOutput[k]["name"].as_string() == variables[i]["name"].as_string())
    //                     jsonOutput[k]["value"] = variables[i]["value"];
    //             count++;
    //             find = true;
    //             break;
    //         }
    //     }

    //     if (!find)
    //     {
    //         UA_Variant newValue;
    //         UA_Variant_init(&newValue);
    //         CreateVariable(client, (char *)variables[i]["name"].as_string().c_str(), variables[i], &newValue);
    //         jsonOutput[count]["name"] = variables[i]["name"];
    //         jsonOutput[count]["value"] = variables[i]["value"];
    //         count++;
    //     }
    // }

    UA_Client_delete(client);

    cout << jsonOutput.serialize() << endl;
    cout << size << endl;

    return EXIT_SUCCESS;
}
