from src.Step import Step
import csv, operator


class Capture:

    def __init__(self, u_name, sample_size=100, device_list=None):
        self.u_name = u_name
        self.device_list = device_list
        self.sample_size = sample_size
        self.step_list = []
        self.stop = False

    def start(self):
        step_n = 0
        while not self.stop:
            value = input("Insert Value for this step (eg Distance):")
            step = Step(step_n, value, self.u_name + "_" + str(step_n), self.sample_size, self.device_list)
            self.step_list.append(step)
            step.start()
            print()
            new_step = input("New step? (Y/N):")
            if new_step == "Y" or new_step == "y" or new_step == "":
                step_n = step_n + 1
            else:
                break
        self.merge()

    def stop(self):
        self.stop = True

    def merge(self):
        with open("../" + self.u_name+".csv", 'w', newline='') as step_file:
            step_writer = csv.writer(step_file)
            temp_list = []
            for step in self.step_list:
                with open(step.u_name_step) as listener_file:
                    read_csv = csv.reader(listener_file, delimiter=',')
                    for row in read_csv:
                        temp_list.append(row)
            for row in temp_list:
                u = row[0]
                v = row[1]
                w = row[2]
                x = row[3]
                y = row[4]
                z = row[5]
                step_writer.writerow([u, v, w, x, y, z])

