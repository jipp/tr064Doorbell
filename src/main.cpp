#include <Arduino.h>

#include <iostream>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Ticker.h>
#include <WiFiClient.h>
#include <WiFiManager.h>

#ifndef SPEED
#define SPEED 115200
#endif

const char *hostname = "doorbell";

Ticker blinker;
const float blink_ok = 0.5;
const float blink_nok = 0.1;

WiFiClient wifiClient;
HTTPClient httpClient;

void blink()
{
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
}

void setupWiFi()
{
  WiFiManager wifiManager;

  WiFi.mode(WIFI_STA);

  wifiManager.setDebugOutput(true);
  wifiManager.setTimeout(180);
  wifiManager.setConnectTimeout(10);

  WiFi.hostname(hostname);

  if (!wifiManager.autoConnect(hostname))
  {
    ESP.restart();
  }
}

void getPage()
{
  if (httpClient.begin(wifiClient, "http://fritz.box:49000/tr64desc.xml"))
  {
    Serial.print("[HTTP] GET...\n");
    // start connection and send HTTP header
    int httpCode = httpClient.GET();

    // httpCode will be negative on error
    if (httpCode > 0)
    {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTP] GET... code: %d\n", httpCode);

      // file found at server
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
      {
        String payload = httpClient.getString();
        Serial.println(payload);
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
}

void setup()
{
  Serial.begin(SPEED);

  pinMode(LED_BUILTIN, OUTPUT);

  blinker.attach(blink_nok, blink);
  setupWiFi();
  blinker.attach(blink_ok, blink);

  getPage();
}

void loop()
{
}