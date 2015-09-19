#!/usr/bin/env python3
import argparse
import serial
import time

def main(args):
    s = serial.Serial(port=args.port, baudrate=args.baud)

    try:
        text = args.text.encode('ascii')

        if len(text) > 255:
            raise ValueError('Text cannot be longer than 255 characters')

        cksm = sum(text) % 0x100

        data = b'\xCA\xFE' + bytes((len(text),)) + text + bytes((cksm,))

        s.write(data)
        time.sleep(1)
        print(s.read(s.inWaiting()).decode('ascii', errors='ignore'))
    finally:
        s.close()


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Change Hat text')
    parser.add_argument('text', help='The text to change to')
    parser.add_argument('-p', '--port', help='The serial port to use', default='/dev/ttyACM0')
    parser.add_argument('-b', '--baud', help='The serial baud rate to use', type=int, default=9600)

    main(parser.parse_args())
