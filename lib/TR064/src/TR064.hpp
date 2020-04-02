#ifndef TR064_HPP
#define TR064_HPP

#include <Arduino.h>

#if defined(ESP8266)
#include <ESP8266HTTPClient.h>
#elif defined(ESP32)
#include <HTTPClient.h>
#endif

#include <MD5Builder.h>
#include <WiFiClient.h>

struct Service
{
    String serviceType;
    String serviceId;
    String controlURL;
    String eventSubURL;
    String SCPDURL;
};

struct Action
{
    String name;
    String direction;
    String argumentName;
    String variable;
};

class TR064
{
public:
    String friendlyName = "";
    String deviceType = "";
    TR064(const char *host, uint16_t port, const char *username, const char *password);
    String trigger(Service &service, Action &action);

private:
    HTTPClient httpClient;
    WiFiClient wifiClient;
    const char *host;
    uint16_t port;
    const char *username;
    const char *password;
    String tr64desc = "";
    bool sendPacket(String &url, String &xml, String &soapAction, String &authReq, Action &action, Service &service, String &result);
    String getDigestAuth(const String &authReq, const String &username, const String &password, const String &uri, unsigned int counter);
    String exractParameter(const String &str, const String &param, char delimit);
    String getCNonce(int len);
    String composeXML(const Service &service, const Action &action);
    bool analyzePayload(String &payload, int httpCode, Action &action, String &result);
};

#endif