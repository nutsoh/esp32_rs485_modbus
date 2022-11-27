#include <WiFi.h>

const char* ssid = "your SSID";
const char* password =  "your PASSWORD";

uint32_t secs;

#define PIN_RX 14
#define PIN_TX 27
#define PIN_CTS 13

#define RX() digitalWrite(PIN_CTS, LOW)
#define TX() digitalWrite(PIN_CTS, HIGH)

WiFiServer wifiServer(1234);


void setup() {
  Serial.begin(115200);
  Serial1.begin(9600, SERIAL_8N1, PIN_RX, PIN_TX);
  pinMode(PIN_CTS, OUTPUT);
  RX();
  Serial.println("Started");
  delay(1000);
 
  WiFi.begin(ssid, password);
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
 
  Serial.println("Connected to the WiFi network");
  Serial.println(WiFi.localIP());
 
  wifiServer.begin();
}

void loop() {
  WiFiClient client = wifiServer.available();
  if (client) {
    while (client.connected()) {
       while (client.available()>0) {
        TX();
        Serial1.write(client.read());
      }
      Serial1.flush();
      RX();
      while (Serial1.available()) {
        client.printf("%c", Serial1.read());
      }
    }
    Serial.println("Client disconnected");
   }
   client.stop();
   delay(1);
}
