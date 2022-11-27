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
  uint8_t buf[1024];
  uint16_t pos = 0;
  if (client) {
    while (client.connected()) {
      while (client.available()>0) {
        while (client.available()>0 && pos < 1024) {
          buf[pos++] = client.read();
          Serial.print("<");
          Serial.print(buf[pos-1]);
        }
        // We have read the full rx buffer, but wait for a few bytes worth to make sure we don't fragment a frame        
        delay(5);
      }
      if (pos > 0) {
        TX();
        Serial1.write(buf, pos);
        Serial1.flush();
        RX();
        pos = 0;
        Serial.println("");
      }
      while (Serial1.available()) {
        while (Serial1.available() && pos < 1024) {
          buf[pos++] = Serial1.read();
          Serial.print(">");
          Serial.print(buf[pos-1]);
        }
        // We have read the full rx buffer, but wait for a few bytes worth to make sure we don't fragment a frame        
        delay(5);
      }
      if (pos > 0) {
        client.write(buf, pos);
        pos = 0;
        Serial.println("");
      }
    }
    Serial.println("Client disconnected");
   }
   client.stop();
   delay(1);
}
