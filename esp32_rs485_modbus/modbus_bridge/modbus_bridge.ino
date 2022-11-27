#include <Arduino.h>
#include "esp32_modbus_bridge.h"

const char* ssid = "your SSID";
const char* password =  "your PASSWORD";

#define PIN_RX 14
#define PIN_TX 27
#define PIN_CTS 13

const uint16_t TCP_SERVER_PORT = 1234;

WiFiServer wserver(TCP_SERVER_PORT);
WiFiClient wclient;

ModbusBridge bridge;

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
  Serial.println("Connected to the WiFi network");
  Serial.println(WiFi.localIP());
}

void setup() {
  // We'll do our debugging over the first serial, as per default
  Serial.begin(115200);
  setup_networking();
  Serial1.begin(9600, SERIAL_8N1, PIN_RX, PIN_TX);
  pinMode(PIN_CTS, OUTPUT);
}

void loop() {
  static bool connected = false;
  wclient = wserver.available();
  if (!connected && wclient.connected()) {
    Serial.println("tcp client connected");
    connected = true;
  }
  while (wclient.connected()) {
    bridge.service(&wclient, &Serial1, ModbusBridge::MODBUS_TCP, ModbusBridge::BRIDGE_NET_INITIATOR, PIN_CTS, false);
  }
  Serial.println("tcp client disconnected");
  connected = false;
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("No network, resetting");
    ESP.restart();
  }
}
