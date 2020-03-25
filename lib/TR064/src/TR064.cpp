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

    xml = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
    xml += "<s:Envelope s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\" xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\">\n";
    xml += "<s:Body>\n";
    xml += "<u:" + action.name + " xmlns:u=\"" + service.serviceType + "\">\n";
    if (action.direction == "in")
    {
        xml += "<" + action.argumentName + ">" + action.variable + "</" + action.argumentName + ">\n";
    }
    xml += "</u:" + action.name + ">\n";
    xml += "</s:Body>\n";
    xml += "</s:Envelope>\n";

    soapAction = service.serviceType + "#" + action.name;
    url = "http://" + String(host) + ":" + 49000 + service.controlURL;

    httpClient.begin(wifiClient, url);

    httpClient.addHeader("Content-Type", "text/xml; charset=\"utf-8\"");
    httpClient.addHeader("SoapAction", soapAction);

    Serial.print("SoapAction: ");
    Serial.println(soapAction);
    Serial.println(xml);

    httpCode = httpClient.POST(xml);
    payload = httpClient.getString();
    httpClient.end();

    switch (httpCode)
    {
    case HTTP_CODE_OK:
        if (action.direction == "out")
        {
            return getParameter(payload, action.argumentName);
        }
        else
        {
            return "";
        }
    case HTTP_CODE_UNAUTHORIZED:
        Serial.println("401 Unauthorized");
        Serial.println(payload);
        // authenticate(service, action);
        return "";
    case HTTP_CODE_INTERNAL_SERVER_ERROR:
        Serial.println(getParameter(payload, "errorDescription"));
        return "";
    default:
        return "";
    }
}

String TR064::authenticate(Service &service, Action &action)
{
    // http://fritz.box:49000/tr64desc.xml

    HTTPClient httpClient;
    WiFiClient wifiClient;

    String xml;
    String url;
    String soapAction;
    int httpCode;
    String payload;

    xml = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
    xml += "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">\n";
    xml += "<s:Header>\n";
    xml += "<h:InitChallenge xmlns:h=\"http://soap-authentication.org/digest/2001/10/\" s:mustUnderstand=\"1\">\n";
    xml += "<UserID>" + String(username) + "</UserID>\n";
    xml += "</h:InitChallenge>\n";
    xml += "</s:Header>\n";
    xml += "<s:Body>\n";
    xml += "<u:" + action.name + " xmlns:u=\"" + service.serviceType + "\">\n";
    if (action.direction == "in")
    {
        xml += "<" + action.argumentName + ">" + action.variable + "</" + action.argumentName + ">\n";
    }
    xml += "</u:" + action.name + ">\n";
    xml += "</s:Body>\n";
    xml += "</s:Envelope>\n";

    soapAction = service.serviceType + "#" + action.name;
    url = "http://" + String(host) + ":" + 49000 + service.controlURL;

    httpClient.begin(wifiClient, url);

    httpClient.addHeader("Content-Type", "text/xml; charset=\"utf-8\"");
    httpClient.addHeader("SoapAction", soapAction);

    Serial.print("url: ");
    Serial.println(url);
    Serial.print("SoapAction: ");
    Serial.println(soapAction);
    Serial.println(xml);

    httpCode = httpClient.POST(xml);
    payload = httpClient.getString();
    httpClient.end();

    switch (httpCode)
    {
    case HTTP_CODE_OK:
        if (action.direction == "out")
        {
            return getParameter(payload, action.argumentName);
        }
        else
        {
            return "";
        }
    case HTTP_CODE_UNAUTHORIZED:
        Serial.println("401 Unauthorized");
        Serial.println(payload);
        return "";
    case HTTP_CODE_INTERNAL_SERVER_ERROR:
        Serial.println(getParameter(payload, "errorDescription"));
        return "";
    default:
        return "";
    }
}