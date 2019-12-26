from src.Device import Device
from src.Capture import Capture
from datetime import datetime
import json


def create_capture_from_json():
    with open('../config.json', 'r') as f:
        config = json.load(f)
        print(config)
        name = config['name']
        baudrate = config['baudrate']
        sample_size = config['sample_size']
        device_list = []
        for device in config['device_list']:
            device_list.append(Device(device['port'], baudrate))

    return Capture(name, int(sample_size), device_list)


def wizard():
    name = input("What is the name of this capture? (timestamp):")
    if name == "":
        now = datetime.now()
        name = now.strftime("%d_%m_%Y_%H_%M_%S")
    port_amount = input("Please Insert the amount of Serial ports to listen to (1):")
    if port_amount == "":
        port_amount = 1
    baudrate = input("Please Insert the baudrate of Serial ports to listen to (9600):")
    sample_size = input("Please Insert the sample size per each step (100):")
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
    return Capture(name, int(sample_size), device_list)


if __name__ == '__main__':
    capture = None
    toggle = input("Load from config.json?")
    if toggle == "Y" or toggle == "y":
        capture = create_capture_from_json()
    else:
        capture = wizard()
    capture.start()





