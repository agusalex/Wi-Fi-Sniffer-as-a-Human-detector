from src.model.Listener import Listener
import csv, operator
import time
import keyboard

class Step:

    def __init__(self, i, value, u_name_step, sample_size=100, csv_values=2, device_list=None):
        self.device_list = device_list
        self.sample_size = sample_size
        self.csv_values = csv_values
        self.listener_list = []
        self.value = value
        self.u_name_step = "../" + self.value + "_" + u_name_step + ".step"
        self.i = i

        self.create_listeners()
        self.stop = False
        self.force_stop = False

    def create_listeners(self):
        for i in range(len(self.device_list)):
            name_i = self.u_name_step[3:len(self.u_name_step) - 5] + str(self.device_list[i].port)
            name_i = name_i.replace("/", "_")
            self.listener_list.append(
                Listener(i + 1, name_i, self.device_list[i].serial, self.sample_size, self.csv_values)
            )

    def start(self, force_stop_key):
        for listener in self.listener_list:
            listener.start()
        while self.some_alive() and not self.force_stop:
            if keyboard.is_pressed(force_stop_key):
                self.force_stop = True
                print("Finishing step")
            #time.sleep(1)
        for listener in self.listener_list:
            if listener.is_alive():
                listener.stop()
        self.merge()
        self.stop = True
        self.force_stop = False

    def some_alive(self):
        alive = False
        for listener in self.listener_list:
            alive = alive or listener.is_alive()
        return alive

    def all_alive(self):
        alive = True
        for listener in self.listener_list:
            alive = alive and listener.is_alive()
        return alive

    def stop_listeners(self):
        for listener in self.listener_list:
            listener.stop()
        self.stop = True

    def export(self):
        self.merge()

    def merge(self):
        with open(self.u_name_step, 'w', newline='') as step_file:
            step_writer = csv.writer(step_file)
            temp_list = []
            for listener in self.listener_list:
                with open(listener.filename) as listener_file:
                    read_csv = csv.reader(listener_file, delimiter=',')
                    for row in read_csv:
                        temp_list.append(row)
            sorted_list = sorted(temp_list, key=operator.itemgetter(3))

            for row in sorted_list:
                row2 = [self.i, self.value]
                for i in range(self.csv_values + 2):
                    row2.append(row[i])
                step_writer.writerow(row2)
