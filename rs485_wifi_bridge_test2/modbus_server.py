import asyncio
from pymodbus import server, datastore, transaction

if __name__ == '__main__':
    datablock = datastore.ModbusSequentialDataBlock(0x00, [0] * 100)
    slaves = datastore.ModbusSlaveContext(di=datablock, co=datablock, hr=datablock, ir=datablock, unit=1)
    context = datastore.ModbusServerContext(slaves=slaves, single=True)

    # Monkey patching the serial send function to assert the DTR line accordingly
    orig_send = server.async_io.ModbusSingleRequestHandler._send_
    def mp_send(s,d):
        serial = s.transport.serial
        serial.setDTR(False)
        orig_send(s,d)
        while (not s.transport._flushed()):
            s.transport._write_ready()
        serial.flush()
        serial.setDTR(True)
    server.async_io.ModbusSingleRequestHandler._send_ = mp_send

    orig_checkFrame = transaction.ModbusRtuFramer.checkFrame
    def mp_checkFrame(s):
        # we know our unit id is not 0, and we are getting some extraneous zeros that should not be there, so just strip all leading zeros
        # this happens as for some reason I am getting an idle low rx line, which is not correct.
        while len(s._buffer) and s._buffer[0] == 0:
            s._buffer = s._buffer[1:]
        return orig_checkFrame(s)
    transaction.ModbusRtuFramer.checkFrame = mp_checkFrame

    serialserver = server.StartAsyncSerialServer(context=context, framer=transaction.ModbusRtuFramer, port='/dev/ttyUSB0', baudrate=9600)
    asyncio.run(serialserver)
