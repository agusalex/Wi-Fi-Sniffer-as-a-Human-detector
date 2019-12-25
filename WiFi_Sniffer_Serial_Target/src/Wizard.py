from src.Device import Device
from src.Capture import Capture

if __name__ == '__main__':
    threads = []
    name = input("What is the name of this capture?(timestamp):")
    port_amount = input("Please Insert the amount of Serial ports to listen to(1):")
    if port_amount == "":
        port_amount = 1
    baudrate = input("Please Insert the baudrate of Serial ports to listen to(9600):")
    sample_size = input("Please Insert the sample size per each step(100):")
    if sample_size == "":
        sample_size = 100
    device_list = []
    if port_amount == "":
        port_amount = 1
    for i in range(int(port_amount)):
        port = input("Insert port n° " + str(i + 1) + "full path (/dev/ttyUSB0):")
        if port == "":
            port = "/dev/ttyUSB0"

        if baudrate == "":
            device_list.append(Device(port))
        else:
            device_list.append(Device(port, int(baudrate)))
    capture = Capture(name, sample_size, 1, device_list)

    capture.start()