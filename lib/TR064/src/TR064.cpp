#include "TR064.hpp"

TR064::TR064(const char *host, uint16_t port, const char *username, const char *password)
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

boolean TR064::getPage(String &str, const String &url)
{
    HTTPClient httpClient;
    WiFiClient wifiClient;
    String link;
    int httpCode;

    link = "http://" + String(host) + ":" + port + "/" + url;

    if (httpClient.begin(wifiClient, link.c_str()))
    {
        httpCode = httpClient.GET();

        if (httpCode > 0)
        {
            if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
            {
                str = httpClient.getString();

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

String TR064::getParameter(const String &str, const String value)
{
    int16_t start;
    int16_t stop;

    start = str.indexOf("<" + value + ">");
    stop = str.indexOf("</" + value + ">");

    if (start >= 0 && stop >= 0)
    {
        start += value.length() + 2;

        return str.substring(start, stop);
    }

    return "";
}

String TR064::getInfo(Service &service, Action &action)
{
    // http://fritz.box:49000/tr64desc.xml

    HTTPClient httpClient;
    WiFiClient wifiClient;

    String xml;
    String url;
    String soapAction;
    int httpCode;
    String payload;

    xml = "<?xml version=\"1.0\"?>";
    xml += "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">";
    xml += "<s:Body>";
    xml += "<u:" + action.name + " xmlns:u=\"" + service.serviceType + "\">";
    xml += "</u:" + action.name + ">";
    xml += "</s:Body>";
    xml += "</s:Envelope>";

    soapAction = service.serviceType + "#" + action.name;
    url = "http://" + String(host) + ":" + 49000 + service.controlURL;

    httpClient.begin(wifiClient, url);
    
    httpClient.addHeader("Content-Type", "text/xml; charset=\"utf-8\"");
    httpClient.addHeader("SoapAction", soapAction);

    httpCode = httpClient.POST(xml);
    payload = httpClient.getString();
    httpClient.end();

    switch (httpCode)
    {
    case HTTP_CODE_OK:
        return getParameter(payload, action.argumentName);
    case HTTP_CODE_UNAUTHORIZED:
        Serial.println("401 Unauthorized");
        return "";
    case HTTP_CODE_INTERNAL_SERVER_ERROR:
        Serial.println(getParameter(payload, "errorDescription"));
        return "";
    default:
        return "";
    }
}