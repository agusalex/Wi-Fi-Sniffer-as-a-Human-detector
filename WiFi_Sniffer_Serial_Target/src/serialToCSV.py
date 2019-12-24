import serial  # pip install pyserial
import threading
from src import Listener

from datetime import datetime

if __name__ == '__main__':
    threads = []
    port_amount = input("Please Insert the amount of Serial ports to listen to:")
    baudrate = input("Please Insert the baudrate of Serial ports to listen to:")
    sample_size = input("Please Insert the sample size per each step:")
    now = datetime.now()
    dt_string = now.strftime("%d_%m_%Y_%H_%M_%S")

    for i in range(int(port_amount)):
        port = input("Insert port n° " + str(i + 1) + "full path (eg:/dev/ttyUSB0 or COM4):")
        serial_i = serial.Serial(port, int(baudrate))
        listener = Listener.myListener(i, "capture_" + str(port).replace("/", "_") + "_" + str(dt_string), serial_i,
                                       int(sample_size))
        thread_i = threading.Thread(target=listener.run)
        threads.append(thread_i)

    for thread in threads:
        thread.start()

    def all_dead():
        dead = False
        for item in threads:
            dead = dead | item.is_alive()
        return dead


    while not all_dead():
        print()
