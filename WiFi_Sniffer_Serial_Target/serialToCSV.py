
import serial  # pip install pyserial
import threading
import Listener
import time

from datetime import datetime

threads = []
port_amount = input("Please Insert the amount of Serial ports to listen to:")
sample_size = input("Please Insert the sample size per each step:")
now = datetime.now()
dt_string = now.strftime("%d_%m_%Y_%H_%M_%S")

for i in range(port_amount):
    port = input("Insert port n° " + str(i + 1) + "full path (eg:/dev/ttyUSB0 or COM4):")
    serial_i = serial.Serial(port, 115200)
    listener = Listener("capture_" + port + dt_string, serial_i, sample_size)

    thread_i = threads.append(
        threading.Thread(target=listener.run))
    thread_i.run()


def all_dead():
    dead = False
    for item in threads:
        dead = dead | item.is_alive()
    return dead


while not all_dead():
    print()
