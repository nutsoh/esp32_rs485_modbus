#ifndef MODBUS_BRIDGE_H
#define MODBUS_BRIDGE_H

#include <stdint.h>
#include <Arduino.h>
#include <WiFi.h>
#include "CRC16.h"

class ModbusBridge {
public:
  typedef enum {
      MODBUS_RTU,
      MODBUS_TCP
  } ModbusBridgeMode_t;
  typedef enum {
      BRIDGE_NET_INITIATOR,
      BRIDGE_NET_RESPONDER
  } ModbusBridgeRole_t;
public:
  ModbusBridge (void);
  ~ModbusBridge (void);

  typedef size_t (*printf_cb)(const char* format, ...);

  void service (WiFiClient* tcpclient, HardwareSerial* serial, ModbusBridgeMode_t mode, ModbusBridgeRole_t role, uint16_t rs485_dir_pin=-1, bool rs485_dir_inv=false);
  void set_printf(printf_cb fnc);
private:
  CRC16 modbus_crc;
};

#endif /* MODBUS_BRIDGE_H */
