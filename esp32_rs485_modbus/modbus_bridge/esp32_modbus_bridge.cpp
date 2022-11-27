#include "esp32_modbus_bridge.h"

#define RX() { if (rs485_dir_pin >= 0) digitalWrite(rs485_dir_pin, rs485_dir_inv); }
#define TX() { if (rs485_dir_pin >= 0) digitalWrite(rs485_dir_pin, !rs485_dir_inv); }

// Uncomment to see the actual flow of data mirrored in the debug serial port
//#define DEBUG_BRIDGE

#ifdef DEBUG_BRIDGE
  #define debug(a) Serial.print(a)
  #define debugf(a,...) Serial.printf(a, __VA_ARGS__)
  #define debugln(a) Serial.println(a)
  static void debug_buf (uint8_t* buf, uint16_t len) {
    for (uint16_t i=0; i<len; i++) {
      Serial.printf(" %02X", buf[i]);
    }
    Serial.printf(" (%d bytes)", len);
  }
#else
  #define debug(a)
  #define debugf(a,...)
  #define debugln(a)
  #define debug_buf(b,l)
#endif

ModbusBridge::ModbusBridge (void)
{
  // Initialize CRC16 for Modbus
  modbus_crc.setPolynome(0x8005);
  modbus_crc.setStartXOR(0xFFFF);
  modbus_crc.setReverseIn(true);
  modbus_crc.setReverseOut(true);

}

ModbusBridge::~ModbusBridge (void)
{

}

void ModbusBridge::service (WiFiClient* tcpclient, HardwareSerial* serialclient, ModbusBridgeMode_t mode, ModbusBridgeRole_t role, uint16_t rs485_dir_pin, bool rs485_dir_inv)
{
  uint8_t serialbuf[512];
  uint8_t tcpbuf[512];
  uint8_t *pbuf;
  uint16_t counter = 0;
  uint16_t len;
  uint16_t crc;

    RX();
    pbuf = serialbuf;
    while (serialclient->available()) {
      *pbuf++ = serialclient->read();
    }
    // Read from serial
    if (pbuf != serialbuf) {
      delay(100);
      while (serialclient->available()) {
        *pbuf++ = serialclient->read();
      }
      len = pbuf - serialbuf;
      debug("From serial:");
      debug_buf(serialbuf, len);
      debugln("");
      switch (mode) {
        case MODBUS_RTU:
          // proxy as is
          tcpclient->write(serialbuf, len);
          tcpclient->flush();
          break;
        case MODBUS_TCP:
          // check CRC before encapsulation
          modbus_crc.restart();
          modbus_crc.add(serialbuf, len-2);
          crc = modbus_crc.getCRC();
          if ((crc >> 8 & 0xFF) != serialbuf[len-1] || (crc & 0xFF) != serialbuf[len-2]) {
            debugf("Failed CRC check, calculated %04X\n", crc);
          } else {
            // encapsulate in MODBUS_TCP format
            pbuf = tcpbuf;
            if (role == BRIDGE_NET_RESPONDER) {
              // we hold the transaction identifier
              counter++;
            }
            // TX ID
            *pbuf++ = (counter >> 8) & 0xFF;
            *pbuf++ = counter & 0xFF;
            // Protocol
            *pbuf++ = 0x00;
            *pbuf++ = 0x00;
            // Length
            *pbuf++ = ((len-2) >> 8) & 0xFF;
            *pbuf++ = (len-2) & 0xFF;
            // Unit ID
            *pbuf++ = serialbuf[0];
            // PDU
            memcpy(pbuf, &serialbuf[1], len-3);
            pbuf += len-3;
            // And send
            len = pbuf - tcpbuf;
            debug_buf(tcpbuf, len);
            debugln("");
            tcpclient->write(tcpbuf, len);
            tcpclient->flush();
          }
          break;
        default:
          break;
      }
    }
    // Read from TCP
    pbuf = tcpbuf;
    while (tcpclient->available()) {
      *pbuf++ = tcpclient->read();
    }
    // Send to RS485
    if (pbuf != tcpbuf) {
      len = pbuf - tcpbuf;
      TX();
      debug("From tcp:");
      debug_buf(tcpbuf, len);
      debugf(" [%d]\n", len);
      switch (mode) {
        case MODBUS_RTU:
          // proxy as is
          serialclient->write(tcpbuf, len);
          serialclient->flush();
          break;
        case MODBUS_TCP:
          if (role == BRIDGE_NET_RESPONDER) {
            // compare received transaction ID to our counter
            if (counter != (tcpbuf[0] << 8) | tcpbuf[1]) {
              debugf("Counter mismatch: %04X\n", counter);
            }
            break;
          } else {
            // store transaction ID
            counter = (tcpbuf[0] << 8) | tcpbuf[1];
          }
          // build RTU packet
          pbuf = serialbuf;
          // Addr
          *pbuf++ = tcpbuf[6];
          // PDU
          memcpy(pbuf, &tcpbuf[7], len-7);
          pbuf += len-7;
          // CRC
          modbus_crc.restart();
          modbus_crc.add(serialbuf, pbuf-serialbuf);
          crc = modbus_crc.getCRC();
          *pbuf++ = crc & 0xFF;
          *pbuf++ = (crc >> 8) & 0xFF;
          // And send
          len = pbuf - serialbuf;
          debug_buf(serialbuf, len);
          debugln("");
          serialclient->write(serialbuf, len);
          serialclient->flush();
          break;
        default:
          break;
      }
      RX();
    }
}
