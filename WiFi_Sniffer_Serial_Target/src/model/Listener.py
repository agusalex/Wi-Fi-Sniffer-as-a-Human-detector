from tqdm import tqdm
import csv
from serial import Serial
import threading
import time


class Listener:
    filename = ""
    ser = None
    sample_size = 0
    captured = 0
    pbar = None
    is_stopped = False

    def __init__(self, i: int, filename: str, ser: Serial, sample: int, csv_values: int):
        self.filename = "../" + filename + ".listener"
        self.ser = ser
        self.sample_size = sample
        self.csv_values = csv_values
        self.i = i
        self.thread = None

    def get_percent_complete(self):
        return (self.sample_size / self.captured) * 100

    def start(self):
        self.thread = threading.Thread(target=self.run)
        self.thread.start()

    def is_alive(self):
        return self.thread.is_alive()

    def stop(self):
        self.is_stopped = True
        self.thread.join()
        self.ser.close()
        self.pbar.close()

    def run(self):
        self.pbar = tqdm(total=self.sample_size, leave=False, position=self.i, unit="received")
        self.pbar.set_description(str(self.ser.port))
        with open(self.filename, "w") as new_file:
            csv_writer = csv.writer(new_file)

            line_count = 0
            if not self.ser.is_open:
                self.ser.open()
            self.ser.flushInput()

            while self.captured < self.sample_size and not self.is_stopped:
                # Strip whitespace on the left and right side
                # of the line
                line = self.ser.readline().strip()

                # Check whether line starts with a B and ends
                # with an E, and ignore it otherwise
                is_valid = line.startswith(b'B') and line.endswith(b'E')
                if not is_valid:
                    # print("Ignored invalid line: " + str(line))
                    continue
                striped_line = line[1:-1]
                string_nplet = striped_line.split(b",")
                if len(string_nplet) != self.csv_values:
                    # print("Ignored invalid line: " + str(string_nplet))
                    continue

                # Write row to the CSV file
                row = [self.ser.port, time.time()]
                for i in range(self.csv_values):
                    row.append(string_nplet[i].decode("utf-8"))

                csv_writer.writerow(row)
                self.captured += 1
                self.pbar.update(1)
        self.pbar.close()
