import csv
import serial

# Open com port
ser = serial.Serial('/dev/ttyUSB0', 115200)

with open("datafile.csv", "w") as new_file:
    csv_writer = csv.writer(new_file)

    line_count = 0
    while True:
        # Strip whitespace on the left and right side
        # of the line
        line = ser.readline().strip()

        # Check whether line starts with a B and ends
        # with an E, and ignore it otherwise
        is_valid = line.startswith(b'B') and line.endswith(b'E')
        if not is_valid:
            print("Ignored invalid line: " + line)
            continue
        stripedLine = line[1:-1]
        xy_string_duplet = stripedLine.split(b",")
        if len(xy_string_duplet) != 2:
            print("Ignored invalid line: " + xy_string_duplet)
            continue


        # Convert the numbers from string format to integer
        x = xy_string_duplet[0].decode("utf-8") 
        y = xy_string_duplet[1].decode("utf-8") 
        # Write XYZ to the CSV file
        csv_writer.writerow([x,y])
