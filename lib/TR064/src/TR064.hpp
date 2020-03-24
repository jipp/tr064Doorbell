#ifndef TR064_HPP
#define TR064_HPP

#include <Arduino.h>

#if defined(ESP8266)
    #include <ESP8266HTTPClient.h>
#elif defined(ESP32)
    #include <HTTPClient.h>
#endif

#include <iostream>
#include <sstream>
#include <WiFiClient.h>

class TR064
{
public:
    std::string friendlyName;
    std::string deviceType;
    TR064(std::string host, uint16_t port, std::string username, std::string password);
    boolean init();
    boolean getPage(std::string &str, const std::string url);
    void getSecurityPort();
    void getHostNumberOfEntries();

private:
    std::string host = "fritz.box";
    uint16_t port = 49000;
    std::string username = "";
    std::string password = "";
    std::string tr64desc;
    std::string getParameter(std::string str, std::string value);
};

#endif