#include <iostream>
#include <curl/curl.h>
#include <string>
#include "lib/ArduinoJson-v6.19.4.h"

#include <thread>

using namespace std;

void Cleanup(CURL *curl, curl_slist *headers)
{
    if (curl)
        curl_easy_cleanup(curl);
    if (headers)
        curl_slist_free_all(headers);
    curl_global_cleanup();
}

/*This is our callback function which will be executed by curl on perform.
here we will copy the data received to our struct
ptr : points to the data received by curl.
size : is the count of data items received.
nmemb : number of bytes
data : our pointer to hold the data.
*/
size_t curl_callback(void *ptr, size_t size, size_t nmemb, std::string *data)
{
    data->append((char *)ptr, size * nmemb);
    return size * nmemb;
}

/*
Send a url from which you want to get the json data
*/
string GetRequest(const string url)
{
    curl_global_init(CURL_GLOBAL_ALL); // sets the program environment
    CURL *curl = curl_easy_init();     // initialize curl
    std::string response_string;
    struct curl_slist *headers = NULL; // linked-list string structure
    if (!curl)
    {
        cout << "ERROR : Curl initialization\n"
             << endl;
        Cleanup(curl, headers);
        return NULL;
    }

    headers = curl_slist_append(headers, "User-Agent: libcurl-agent/1.0");
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "Cache-Control: no-cache");

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    // ignore ssl verification if you don't have the certificate or don't want secure communication
    //  you don't need these two lines if you are dealing with HTTP url
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);                 // maximum time allowed for operation to finish, its in seconds
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());             // pass the url
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_callback); // pass our call back function to handle received data
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);  // pass the variable to hold the received data

    CURLcode status = curl_easy_perform(curl); // execute request
    if (status != 0)
    {
        cout << "Error: Request failed on URL : " << url << endl;
        cout << "Error Code: " << status << " Error Detail : " << curl_easy_strerror(status) << endl;
        Cleanup(curl, headers);
        return NULL;
    }

    // do the clean up
    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
    curl_global_cleanup();

    return response_string;
}

string jsonData; // make global so thread can write to it

void checkJson(string URL)
{
    jsonData = GetRequest(URL);
}

int main()
{
    string theURL = "http://192.168.1.190/temperature";

    std::thread thread_json(checkJson, theURL);

    // jsonData = GetRequest("http://192.168.1.190/temperature"); // request for json

    // checkJson("http://192.168.1.190/temperature");

    thread_json.join(); // wait for thread
    // thread_json.detach();

    std::cout << jsonData << std::endl; // print json serialized data

    DynamicJsonDocument doc(1024);  // some kind of buffer used by json library
    deserializeJson(doc, jsonData); // put json into some sort of array / object

    double myTemp = doc["value"]; // get "value" out of json
    cout << myTemp << endl;       // print temperature too
    // std::cin.get();
    return 0;
}