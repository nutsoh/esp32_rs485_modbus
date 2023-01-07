#include <Arduino.h>
#include "esp32_modbus_bridge.h"
#include <ArduinoOTA.h>

const char* ssid = "your SSID";
const char* password =  "your PASSWORD";

#define PIN_RX 14
#define PIN_TX 27
#define PIN_CTS 13

const uint16_t TCP_SERVER_PORT = 1234;
const uint16_t TCP_DEBUG_PORT = 1235;

WiFiServer wserver(TCP_SERVER_PORT);
WiFiServer dserver(TCP_DEBUG_PORT);
WiFiClient wclient;
WiFiClient dclient;

ModbusBridge bridge;

static size_t printer_f (const char* fmt, ...) {
  char buffer[1024];
  va_list args;
  va_start (args, fmt);
  vsprintf (buffer,fmt, args);
  va_end (args);

  Serial.print(buffer);
  if (dclient.connected()) {
    dclient.print(buffer);
  }
  return strlen(buffer);
}

static void setup_networking () {
  // No WiFi power management. Draws more power but does not lose connections constantly
  WiFi.setSleep(WIFI_PS_NONE);
  WiFi.begin(ssid, password);
  uint8_t cnt=0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
    if (++cnt > 30) {
      Serial.println("No network, resetting");
      ESP.restart();
    }
  }
  wserver.begin();
  dserver.begin();
  Serial.println("Connected to the WiFi network");
  Serial.println(WiFi.localIP());
  ArduinoOTA.begin();
}

void setup() {
  // We'll do our basic debugging over the first serial, as per default
  Serial.begin(115200);
  setup_networking();
  Serial1.begin(9600, SERIAL_8N1, PIN_RX, PIN_TX);
  pinMode(PIN_CTS, OUTPUT);
  // There's an optional tcp socket to mimic what is sent to the serial port, so tell the modbus bridge
  // about it
  bridge.set_printf(&printer_f);
}

void loop() {
  static bool connected = false;
  
  if (!dclient.connected()) {
    dclient = dserver.available();
  }
  if (!connected) {
    wclient = wserver.available();
    if (wclient.connected()) {
      printer_f("tcp client connected");
      connected = true;
    }
  }
  if (connected) {
    if (wclient.connected()) {
      bridge.service(&wclient, &Serial1, ModbusBridge::MODBUS_TCP, ModbusBridge::BRIDGE_NET_INITIATOR, PIN_CTS, false);
    } else {
      printer_f("tcp client disconnected\n");
      delay(1000);
      connected = false;
    }
  }
  if (WiFi.status() != WL_CONNECTED) {
    printer_f("No network, resetting\n");
    delay(1000);
    ESP.restart();
  }
  ArduinoOTA.handle();
}
