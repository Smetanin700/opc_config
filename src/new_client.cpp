#include "new_client.hpp"

int main()
{
    signal(SIGINT, stopHandler);

    ifstream fs(clientJson);	
    json::value settings = json::value::parse(fs)["settings"];

    ifstream fv(variableJson);
	json::value variables = json::value::parse(fv)["variables"];
	
    // UA_Client *client = InitClient();
    UA_Client *client = InitSecureClient(settings["messageSecurity"].as_string(), settings["securityPolicy"].as_string());
    
    json::value varOut;

    retval = UA_STATUSCODE_BAD;
    while (retval != UA_STATUSCODE_GOOD)
    {
        if (!running) return -1;
        retval = UA_Client_connectUsername(client,
                                            ((settings["address"].as_string() + ":" + settings["port"].as_string() + settings["endpoint"].as_string()).c_str()),
                                            (settings["login"].as_string().c_str()),
                                            (settings["password"]).as_string().c_str());
        
        // retval = UA_Client_connect(client, ((settings["address"].as_string() + ":" + settings["port"].as_string()).c_str()));
        sleep_ms(1000);
    }

    if (retval != UA_STATUSCODE_GOOD)
    {
        cout << "Bad conection: " << retval << endl;        
        UA_Client_delete(client);
        return 1;
    }

    BrowsingVars(client);
    UA_Client_delete(client);

    return EXIT_SUCCESS;
}
