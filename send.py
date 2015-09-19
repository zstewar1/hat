#!/usr/bin/env python3
# Copyright 2015 Zachary Stewart <zachary@zstewart.com>
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

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
