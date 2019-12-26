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
    stop = False

    def __init__(self, i: int, filename: str, ser: Serial, sample: int):
        self.filename = "../" + filename + ".listener"
        self.ser = ser
        self.sample_size = sample
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
        self.stop = True
        self.thread.join()
        self.ser.close()
        self.pbar.close()

    def run(self):
        self.pbar = tqdm(total=self.sample_size, leave=False, position=self.i, unit="received")
        self.pbar.set_description(str(self.ser.port))
        with open(self.filename, "w") as new_file:
            csv_writer = csv.writer(new_file)

            line_count = 0
            while (self.captured < self.sample_size) :
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
                xy_string_duplet = striped_line.split(b",")
                if len(xy_string_duplet) != 2:
                    # print("Ignored invalid line: " + str(xy_string_duplet))
                    continue

                # Convert the numbers from string format to integer
                w = self.ser.port
                x = xy_string_duplet[0].decode("utf-8")
                y = xy_string_duplet[1].decode("utf-8")
                z = time.time()
                # Write XYZ to the CSV file
                csv_writer.writerow([w, x, y, z])
                self.captured += 1
                self.pbar.update(1)
        self.pbar.close()
