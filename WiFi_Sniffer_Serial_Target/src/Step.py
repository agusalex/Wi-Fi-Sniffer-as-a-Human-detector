from src.Listener import Listener


class Step:

    def __init__(self, u_name_step, sample_size=100, device_list=None):
        self.device_list = device_list
        self.sample_size = sample_size
        self.listener_list = []
        self.u_name_step = u_name_step
        self.create_listeners()
        self.listener_threads = []

    def create_listeners(self):
        for i in range(len(self.device_list)):
            name_i = self.u_name_step + str(self.device_list[i].port)
            name_i = name_i.replace("/", "_")
            self.listener_list.append(
                Listener(i, name_i, self.device_list[i].serial, self.sample_size)
            )

    def start(self):
        for listener in self.listener_list:
            listener.start()

    def stop(self):
        for listener in self.listener_list:
            listener.stop()
