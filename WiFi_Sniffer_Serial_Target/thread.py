import os, pty
from serial import Serial
import Listener as Listener
import threading
import time



def test_serial(s_name):
    """Start the testing"""

    # create a separate thread that listens on the master device for commands

    # open a pySerial connection to the slave
    i = 0
    ser = Serial(s_name, 115200, timeout=1)

    for i in range(100):
        ser.write(i)  # write the first command
        time.sleep(1)

    ser.close()


if __name__ == '__main__':
    master, slave = pty.openpty()  # open the pseudoterminal
    s_name = os.ttyname(slave)  # translate the slave fd to a filename
    serial_i = Serial(s_name, 115200)
    print(s_name)
    listener = Listener.myListener("test",serial_i , 100)
    mythread = threading.Thread(target=listener.run)
    mythread.run()
    test_serial(s_name)
