#include <Arduino.h>

#include <iostream>
#include <Ticker.h>
#include <WiFiManager.h>

#include "TR064.hpp"

#ifndef SPEED
#define SPEED 115200
#endif

const char *hostname = "doorbell";
const std::string host = "fritz.box";
const uint16_t port = 49000;
const std::string username = "doorbell";
const std::string password = "Drei3Zehn";

Ticker blinker;
const float blink_ok = 0.5;
const float blink_nok = 0.1;

TR064 tr064(host, port, username, password);

void blink()
{
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
}

void setupWiFi()
{
  WiFiManager wifiManager;

  WiFi.mode(WIFI_OFF);
  WiFi.mode(WIFI_STA);

  wifiManager.setDebugOutput(true);
  wifiManager.setTimeout(180);
  wifiManager.setConnectTimeout(10);

#if defined(ESP8266)
  WiFi.hostname(hostname);
#elif defined(ESP32)
  WiFi.setHostname(hostname);
#endif

  if (!wifiManager.autoConnect(hostname))
  {
    ESP.restart();
  }
}

void setup()
{
  Serial.begin(SPEED);

  pinMode(LED_BUILTIN, OUTPUT);

  blinker.attach(blink_nok, blink);
  setupWiFi();
  blinker.attach(blink_ok, blink);

  tr064.getSecurityPort();

  // if (tr064.init())
  // {
  //   tr064.woke();
  // }
}

void loop()
{
}