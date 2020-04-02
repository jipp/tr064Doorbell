#include <Arduino.h>

#include <ACS712.h>
#include <OneButton.h>
#include <Ticker.h>
#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#elif defined(ESP32)
#include <WiFi.h>
#include <HTTPClient.h>
#endif
#include <WiFiManager.h>

#include "TR064.hpp"

#ifndef SPEED
#define SPEED 115200
#endif

const char *host = "fritz.box";
const uint16_t port = 49000;
const char *hostname = "doorbell";
const char *username = "doorbell";
const char *password = "Drei3Zehn";
const char *phoneNumber = "**612";
Ticker toBlink;
Ticker toShowStatus;
Ticker toHangupCall;
Ticker trial;
const float blink_ok = 0.5;
const float blink_nok = 0.1;
const uint32_t pulse = 60000;
const uint32_t callDuration = 3000;

#if defined(ESP8266)
#define PUSH_BUTTON D1
#elif defined(ESP32)
#define PUSH_BUTTON 27
#endif

#define SENSOR A0

TR064 tr064(host, port, username, password);
Service service;
Action action;
OneButton button(PUSH_BUTTON, true);
ACS712 sensor(ACS712_05B, SENSOR);

void blink()
{
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
}

void showStatus()
{
#if defined(ESP8266)
  Serial.println(">>" + String(millis()) + "ms FreeHeap: " + ESP.getFreeHeap() + "b");
#elif defined(ESP32)
  Serial.println(">>" + String(millis()) + "ms FreeHeap: " + ESP.getFreeHeap() + "b FreePsram: " + ESP.getFreePsram() + "b");
#endif
}

void setupWiFi()
{
  WiFiManager wifiManager;

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

void getSecurityPort()
{
  service.serviceType = "urn:dslforum-org:service:DeviceInfo:1";
  service.serviceId = "urn:DeviceInfo-com:serviceId:DeviceInfo1";
  service.controlURL = "/upnp/control/deviceinfo";
  service.eventSubURL = "/upnp/control/deviceinfo";
  service.SCPDURL = "/deviceinfoSCPD.xml";

  action.name = "GetSecurityPort";
  action.direction = "out";
  action.argumentName = "NewSecurityPort";

  Serial.print("NewSecurityPort: ");
  Serial.println(tr064.trigger(service, action));
}

void hangupCall()
{
  service.serviceType = "urn:dslforum-org:service:X_VoIP:1";
  service.serviceId = "urn:X_VoIP-com:serviceId:X_VoIP1";
  service.controlURL = "/upnp/control/x_voip";
  service.eventSubURL = "/upnp/control/x_voip";
  service.SCPDURL = "/x_voipSCPD.xml";

  action.name = "X_AVM-DE_DialHangup";
  action.direction = "out";
  action.argumentName = "";

  tr064.trigger(service, action);
}

void dial()
{
  service.serviceType = "urn:dslforum-org:service:X_VoIP:1";
  service.serviceId = "urn:X_VoIP-com:serviceId:X_VoIP1";
  service.controlURL = "/upnp/control/x_voip";
  service.eventSubURL = "/upnp/control/x_voip";
  service.SCPDURL = "/x_voipSCPD.xml";

  action.name = "X_AVM-DE_DialNumber";
  action.direction = "in";
  action.argumentName = "NewX_AVM-DE_PhoneNumber";
  action.variable = phoneNumber;

  tr064.trigger(service, action);

  toHangupCall.once_ms(callDuration, hangupCall);
}

void setup()
{
  Serial.begin(SPEED);

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PUSH_BUTTON, INPUT_PULLUP);
  pinMode(SENSOR, INPUT);

  toBlink.attach(blink_nok, blink);
  setupWiFi();
  toBlink.attach(blink_ok, blink);
  toShowStatus.attach_ms(pulse, showStatus);

  button.attachClick(dial);

  getSecurityPort();
  trial.attach_ms(10000, getSecurityPort);
}

void loop()
{
  button.tick();
}