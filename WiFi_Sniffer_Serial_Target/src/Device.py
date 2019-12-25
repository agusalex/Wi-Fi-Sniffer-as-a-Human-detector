from serial import Serial


class Device:

    def __init__(self, port: str, baud_rate=9600):
        self.serial = None
        self.baud_rate = baud_rate
        self.port = port
        self.serial = Serial(self.port, int(self.baud_rate))
