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
    }

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

    // compose xml
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

    // compose header and url
    soapAction = service.serviceType + "#" + action.name;
    url = "http://" + String(host) + ":" + 49000 + service.controlURL;

    httpClient.begin(wifiClient, url);

    const char *keys[] = {"WWW-Authenticate"};
    httpClient.collectHeaders(keys, 1);

    httpClient.addHeader("Content-Type", "text/xml; charset=\"utf-8\"");
    httpClient.addHeader("SoapAction", soapAction);

    Serial.print("SoapAction: ");
    Serial.println(soapAction);
    Serial.println(xml);

    // send packet
    httpCode = httpClient.POST(xml);

    String authReq = httpClient.header("WWW-Authenticate");
    Serial.print("WWW-Authenticate: ");
    Serial.println(authReq);

    payload = httpClient.getString();
    httpClient.end();

    // compose 2nd packet
    String authorization = getDigestAuth(authReq, String(username), String(password), String(service.controlURL), 1);
    Serial.print("Authorization: ");
    Serial.println(authorization);

    //

    httpClient.begin(wifiClient, url);

    httpClient.addHeader("Authorization", authorization);
    httpClient.addHeader("Content-Type", "text/xml; charset=\"utf-8\"");
    httpClient.addHeader("SoapAction", soapAction);

    Serial.print("SoapAction: ");
    Serial.println(soapAction);
    Serial.println(xml);

    httpCode = httpClient.POST(xml);

    payload = httpClient.getString();
    httpClient.end();

    //
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

String TR064::getDigestAuth(String &authReq, const String &username, const String &password, const String &uri, unsigned int counter)
{
    // extracting required parameters for RFC 2069 simpler Digest
    String realm = exractParam(authReq, "realm=\"", '"');
    String nonce = exractParam(authReq, "nonce=\"", '"');
    String cNonce = getCNonce(44);

    char nc[9];
    snprintf(nc, sizeof(nc), "%08x", counter);

    // parameters for the RFC 2617 newer Digest
    MD5Builder md5;
    md5.begin();
    md5.add(username + ":" + realm + ":" + password); // md5 of the user:realm:user
    md5.calculate();
    String h1 = md5.toString();

    md5.begin();
    md5.add(String("POST:") + uri);
    md5.calculate();
    String h2 = md5.toString();

    md5.begin();
    md5.add(h1 + ":" + nonce + ":" + String(nc) + ":" + cNonce + ":" + "auth" + ":" + h2);
    md5.calculate();
    String response = md5.toString();

    String authorization = "Digest username=\"" + username + "\", realm=\"" + realm + "\", nonce=\"" + nonce +
                           "\", uri=\"" + uri + "\", cnonce=\"" + cNonce + "\", nc=" + String(nc) + ", qop=auth, response=\"" + response + "\", algorithm=\"MD5\"";
    Serial.println(authorization);

    return authorization;
}

String TR064::exractParam(String &authReq, const String &param, const char delimit)
{
    int start = authReq.indexOf(param);
    if (start == -1)
    {
        return "";
    }
    return authReq.substring(start + param.length(), authReq.indexOf(delimit, start + param.length()));
}

String TR064::getCNonce(const int len)
{
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    String s = "";

    for (int i = 0; i < len; ++i)
    {
        s += alphanum[rand() % (sizeof(alphanum) - 1)];
    }

    return s;
}
