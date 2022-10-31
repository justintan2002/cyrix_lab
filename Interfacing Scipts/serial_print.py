import serial
import time 

ser = serial.Serial("COM7", baudrate = 115200)

while True:
    msg = ser.read_until()
    print(msg)
    time.sleep(0.01)
