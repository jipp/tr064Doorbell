#include "TR064.hpp"

TR064::TR064(const char *host, uint16_t port, const char *username, const char *password)
{
    this->host = host;
    this->port = port;
    this->username = username;
    this->password = password;
}

String TR064::trigger(Service &service, Action &action)
{
    // http://fritz.box:49000/tr64desc.xml

    String authReq = "";
    String authorization = "";
    String result = "";

    String url = "http://" + String(host) + ":" + port + service.controlURL;
    String soapAction = service.serviceType + "#" + action.name;
    String xml = composeXML(service, action);

    // compose 1st packet

    Serial.println(requestAutehntication(url, xml, soapAction, authReq, action, service, result));

    // compose 2nd packet

    if (sendPacket(url, xml, soapAction, authReq, action, service, result))
    {
        return result;
    }

    return "";
}

bool TR064::requestAutehntication(String &url, String &xml, String &soapAction, String &authReq, Action &action, Service &service, String &result)
{
    httpClient.begin(wifiClient, url);

    const char *keys[] = {"WWW-Authenticate"};
    httpClient.collectHeaders(keys, 1);

    httpClient.addHeader("Content-Type", "text/xml; charset=\"utf-8\"");
    httpClient.addHeader("SoapAction", soapAction);

    // send packet
    int httpCode = httpClient.POST(xml);

    authReq = httpClient.header("WWW-Authenticate");
    String payload = httpClient.getString();

    httpClient.end();

    Serial.println(authReq);

    return analyzePayload(payload, httpCode, action, result);
}

bool TR064::sendPacket(String &url, String &xml, String &soapAction, String &authReq, Action &action, Service &service, String &result)
{
    String authorization = "";

    if (authReq != "")
    {
        authorization = getDigestAuth(authReq, String(username), String(password), String(service.controlURL), 1);
    }

    httpClient.begin(wifiClient, url);
    if (authReq != "")
    {
        httpClient.addHeader("Authorization", authorization);
    }
    httpClient.addHeader("Content-Type", "text/xml; charset=\"utf-8\"");
    httpClient.addHeader("SoapAction", soapAction);

    int httpCode = httpClient.POST(xml);

    String payload = httpClient.getString();

    httpClient.end();

    return analyzePayload(payload, httpCode, action, result);
}

String TR064::getDigestAuth(const String &authReq, const String &username, const String &password, const String &uri, unsigned int counter)
{
    // extracting required parameters for RFC 2069 simpler Digest
    String realm = exractParameter(authReq, "realm=\"", '"');
    String nonce = exractParameter(authReq, "nonce=\"", '"');
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

    return "Digest username=\"" + username + "\", realm=\"" + realm + "\", nonce=\"" + nonce + "\", uri=\"" + uri + "\", cnonce=\"" + cNonce + "\", nc=" + String(nc) + ", qop=auth, response=\"" + response + "\", algorithm=\"MD5\"";
}

String TR064::exractParameter(const String &str, const String &parameter, char delimiter)
{
    int start = str.indexOf(parameter);

    if (start == -1)
    {
        return "";
    }

    return str.substring(start + parameter.length(), str.indexOf(delimiter, start + parameter.length()));
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

String TR064::composeXML(const Service &service, const Action &action)
{
    String xml = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
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

    return xml;
}

bool TR064::analyzePayload(String &payload, int httpCode, Action &action, String &result)
{
    switch (httpCode)
    {
    case HTTP_CODE_OK:
        if (action.direction == "out")
        {
            result = exractParameter(payload, action.argumentName + '>', '<');
            return true;
        }
        return false;
    case HTTP_CODE_UNAUTHORIZED:
        Serial.println("401 Unauthorized");
        Serial.println(payload);
        return false;
    case HTTP_CODE_INTERNAL_SERVER_ERROR:
        Serial.println("500 Internal Server Error");
        result = exractParameter(payload, "errorDescription>", '<');
        Serial.println(result);
        Serial.println(payload);
        return false;
    default:
        Serial.println("not catched");
        return false;
    }
}