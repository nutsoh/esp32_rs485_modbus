import serial
import socket
import time
class BridgeTest ():
    def __init__ (self, portname : str, tcpserver : str, tcpport : int) -> None:
        # Initialize the serial port
        self.serial = serial.Serial(portname, 9600, timeout=0.1)
        # Initialize the network connection
        self.tcpsocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.tcpsocket.connect((tcpserver, tcpport))
        self.tcpsocket.setblocking(False)
        # Assert we start in RX mode
        self.state_rx()
    
    def state_rx (self):
        self.serial.setDTR(True)
    
    def state_tx (self):
        self.serial.setDTR(False)

    def loop_rx (self):
        serial_msg = ''
        tcp_msg = ''
        while (1):
            # Receive from serial
            r = self.serial.read().decode('utf8')
            # not sure why, but I get a couple of 0x00 on the serial port the end of the message (the LF character)
            # so filter that too. Maybe a (lack of) flow control issue?
            if len(r) and r != '\0':
                serial_msg += r
                if r == '\n':
                    serial_msg = serial_msg.encode('utf8')
                    print("received from serial;", serial_msg, end='')
                    # we're doing 8 bits + start and stop, so 10 bits per byte
                    # pre calculate how long sending will take as my windows
                    # version of the serial module simply refuses to wait on
                    # serial flush, or to tell me how occupied the output buffer
                    # is at any point.
                    tx_time = len(serial_msg) * 10 / 9600
                    self.state_tx()
                    self.serial.write(serial_msg)
                    # flush works fine on linux, but not on windows so...
                    #self.serial.flush()
                    time.sleep(tx_time)
                    self.state_rx()
                    serial_msg = ''
                    print("  -- echoed back to serial")
            try:
                r = self.tcpsocket.recv(512)
                try:
                    tcp_msg += r.decode('utf8')
                    tcp_msg = tcp_msg.split('\n')
                    while len(tcp_msg) > 1:
                        print("received from tcp socket;", tcp_msg.pop(0).encode('utf8'))
                    tcp_msg = tcp_msg[-1]
                except UnicodeDecodeError:
                    print("garbage received from tcp socket;", r)
            except BlockingIOError:
                pass

if __name__ == '__main__':
    import sys
    if len(sys.argv) > 1:
        port = sys.argv[1]
    else:
        port = '/dev/ttyUSB0'
    if len(sys.argv) > 2:
        addr = sys.argv[2]
    else:
        addr = 'localhost'
    b = BridgeTest(port, addr, 1234)
    b.loop_rx()