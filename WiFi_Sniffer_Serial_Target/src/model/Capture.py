from src.model.Step import Step

import csv
import sys
from termios import tcflush, TCIFLUSH

class Capture:

    def __init__(self, u_name, sample_size=100, csv_values=2, device_list=None):
        self.u_name = u_name
        self.device_list = device_list
        self.sample_size = sample_size
        self.csv_values = csv_values
        self.step_list = []

    def start(self):
        step_n = 0
        while True:
            value = input("Insert Value for this step (eg Distance):")
            step = Step(step_n, value, self.u_name + "_" + str(step_n), self.sample_size, self.csv_values,
                        self.device_list)
            self.step_list.append(step)
            step.start('p')
            print()
            tcflush(sys.stdin, TCIFLUSH)
            new_step = input("New step? (Y/N):")
            if new_step == "Y" or new_step == "y" or new_step == "":
                step_n = step_n + 1
            else:
                break
        self.merge()

    def stop(self):
        self.stop = True

    def merge(self):
        with open("../" + self.u_name + ".csv", 'w', newline='') as step_file:
            capture_writer = csv.writer(step_file)
            temp_list = []
            for step in self.step_list:
                with open(step.u_name_step) as listener_file:
                    read_csv = csv.reader(listener_file, delimiter=',')
                    for row in read_csv:
                        temp_list.append(row)
            for row in temp_list:
                row2 = []
                for i in range(self.csv_values + 4):
                    row2.append(row[i])
                capture_writer.writerow(row2)
