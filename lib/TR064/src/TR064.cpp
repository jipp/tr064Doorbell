#include "TR064.hpp"

TR064::TR064(std::string host, uint16_t port, std::string username, std::string password)
{
    this->host = host;
    this->port = port;
    this->username = username;
    this->password = password;
}

boolean TR064::init()
{
    if (getPage(tr64desc, "tr64desc.xml"))
    {
        friendlyName = getParameter(tr64desc, "friendlyName");
        deviceType = getParameter(tr64desc, "deviceType");

        return true;
    };

    return false;
}

boolean TR064::getPage(std::string &str, const std::string url)
{
    HTTPClient httpClient;
    WiFiClient wifiClient;
    int httpCode;
    std::stringstream ss;

    ss << "http://" << host << ":" << port << "/" << url;
    std::string link = ss.str();

    if (httpClient.begin(wifiClient, link.c_str()))
    {
        httpCode = httpClient.GET();

        if (httpCode > 0)
        {
            std::cout << "httpCode: " << httpCode << std::endl;

            if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
            {
                str = std::string(httpClient.getString().c_str());

                return true;
            }
        }
        else
        {
            Serial.printf("[HTTP] GET... failed, error: %s\n", httpClient.errorToString(httpCode).c_str());
        }

        httpClient.end();
    }
    else
    {
        Serial.printf("[HTTP} Unable to connect\n");
    }

    return false;
}

std::string TR064::getParameter(std::string str, std::string value)
{
    int16_t start;
    int16_t stop;

    start = str.find("<" + value + ">");
    stop = str.find("</" + value + ">");

    if (start >= 0 && stop >= 0)
    {
        start += value.length() + 2;
        stop -= start;

        return str.substr(start, stop);
    }
    else
    {
        return "";
    }
}

void TR064::woke()
{
    HTTPClient httpClient;
    WiFiClient wifiClient;
    std::string xml;
    std::string contentType;
    std::string soapAction;

    xml = R"(<?xml version="1.0"?>)";
    xml += R"(<s:Envelope xmlns:s="http://schemas.xmlsoap.org/soap/envelope/" s:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/">)";
    xml += R"(<s:Body>)";
    xml += R"(<u:GetSecurityPort xmlns:u="urn:dslforumorg:service:DeviceInfo:1">)";
    xml += R"(</u:GetSecurityPort>)";
    xml += R"(</s:Body>)";
    xml += R"(</s:Envelope>\0)";

    contentType = R"(text/xml; charset="utf-8")";
    soapAction = R"(urn:dslforum-org:service:DeviceInfo:1#GetSecurityPort)";

    httpClient.begin(wifiClient, "http://fritz.box:49000/upnp/control/deviceinfo");
    httpClient.addHeader("Content-Type", contentType.c_str());
    httpClient.addHeader("SoapAction", soapAction.c_str());

    int httpCode = httpClient.POST(xml.c_str());
    String payload = httpClient.getString();
    httpClient.end();

    Serial.println(httpCode);
    Serial.println(payload);
}