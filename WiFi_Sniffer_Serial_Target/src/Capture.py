from src.Step import Step
from datetime import datetime


class Capture:

    def __init__(self, u_name, sample_size=100, steps=1, device_list=None):
        if u_name is None or "":
            now = datetime.now()
            self.u_name = now.strftime("%d_%m_%Y_%H_%M")
        else:
            self.u_name = u_name

        self.device_list = device_list
        self.sample_size = sample_size
        self.steps = steps
        self.step_list = []
        self.create_steps()

    def create_steps(self):
        for i in range(self.steps):
            self.step_list.append(Step(self.u_name + "_" + str(i), self.sample_size, self.device_list))

    def start(self):
        for step in self.step_list:
            step.start()
