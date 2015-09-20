// Copyright 2015 Zachary Stewart <zachary@zstewart.com>
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <EEPROM.h>

#include "morse.h"

const int max_message = 0xFF;
char message[max_message] = "Hello, World.";

/** Morse Code */
const unsigned long unit_length_millis = 100;
unsigned long next_unit;
int units = 0;
bool morse_on = false;

int str_pos = 0;
int chr_pos = 0;

const int morse_pin = 3;
/** End Morse Code */

/** Binary */
const unsigned long b_unit_length_millis = 200;

unsigned long next_b_unit;

int b_chr_pos = 0;
int b_str_pos = 0;

bool b_on = false;

const int b_on_pin = 5;
const int b_off_pin = 6;
/** End Binary */

/** RGB */
const unsigned long rgb_unit_length_millis = 1;

unsigned long next_rgb_unit;

unsigned char r,g,b, goal_r, goal_g, goal_b;

const int r_pin = 9;
const int g_pin = 10;
const int b_pin = 11;
/** End RGB */

/** Message Update */
enum MessageState {
  CA,
  FE,
  LEN,
  RECV,
  CKSM,
};

char recv_message[max_message];

unsigned char write_counter;
unsigned char write_size;

MessageState state;
/** End Message Update */

void setup() {
    randomSeed(message[0]);
  
    for(int i = 0; i < max_message; i++) {
      message[i] = EEPROM.read(i);
    }

    unsigned long now = millis();

    // Morse Code
    pinMode(morse_pin, OUTPUT);
    digitalWrite(morse_pin, LOW);

    next_unit = now + unit_length_millis;

    // Binary
    next_b_unit = now + b_unit_length_millis;
    pinMode(b_on_pin, OUTPUT);
    pinMode(b_off_pin, OUTPUT);
    digitalWrite(b_on_pin, LOW);
    digitalWrite(b_off_pin, LOW);

    // RGB
    next_rgb_unit = now + rgb_unit_length_millis;
    r = g = b = goal_r = goal_g = goal_b = 0;
    pinMode(r_pin, OUTPUT);
    pinMode(g_pin, OUTPUT);
    pinMode(b_pin, OUTPUT);
    analogWrite(r_pin, r);
    analogWrite(g_pin, g);
    analogWrite(b_pin, b);

    // Message Update
    Serial.begin(9600);
    state = CA;
}

void loop() {
    unsigned long now = millis();
    step_morse(now);
    step_binary(now);
    step_rgb(now);
    serial_recv(now);
}

void serial_recv(unsigned long now) {
    if(Serial.available()) {
        unsigned char c = Serial.read();
        switch (state) {
        case CA:
            if(c == 202) {
                state = FE;
            }
            break;
        case FE:
            if(c == 254) {
                state = LEN;
            } else {
                state = CA;
            }
            break;
        case LEN:
            write_size = c;
            if (write_size == 0) {
                state = CA;
            } else {
                write_counter = 0;
                state = RECV;
            }
            break;
        case RECV:
            recv_message[write_counter++] = c;
            if (write_counter >= write_size) {
                state = CKSM;
            }
            break;
        case CKSM:
        {
            unsigned char cksm = 0;
            for(int i = 0; i < write_size; i++) {
                cksm += recv_message[i];
            }
            if(cksm == c) {
                recv_message[write_size] = 0;
                for(int i = 0; i <= write_size; i++) {
                    if(message[i] != recv_message[i]) {
                        message[i] = recv_message[i];
                        EEPROM.write(i, message[i]);
                    }
                }
                buffer_info_reset(now);
                Serial.print("New Message: ");
                Serial.println(recv_message);
            }
            state = CA;
            break;
        }
        default:
            state = CA;
            break;
        }
    }
}

void buffer_info_reset(unsigned long now) {
    // Morse Code
    str_pos = 0;
    chr_pos = 0;
    units = 0;
    next_unit = now + unit_length_millis;
    
    // Binary
    b_str_pos = 0;
    b_chr_pos = 0;
    next_b_unit = now + b_unit_length_millis;
}

void step_rgb(unsigned long now) {
    if (now > next_rgb_unit) {
      next_rgb_unit = now + rgb_unit_length_millis;
      if (r == goal_r && g == goal_g && b == goal_b) {
          goal_r = random(0, 256);
          goal_g = random(0, 256);
          goal_b = random(0, 256);
      } else {
          if (r < goal_r) r++;
          else if(r > goal_r) r--;
          if (g < goal_g) g++;
          else if(g > goal_g) g--;
          if (b < goal_b) b++;
          else if(b > goal_b) b--;
      }
    
      analogWrite(r_pin, r);
      analogWrite(g_pin, g);
      analogWrite(b_pin, b);
  }
}

void step_binary(unsigned long now) {
    if (now > next_b_unit) {
        next_b_unit = now + b_unit_length_millis;
        if (b_on) {
          digitalWrite(b_on_pin, LOW);
          digitalWrite(b_off_pin, LOW);
          b_on = false;
        } else {
            b_on = true;
            if (message[b_str_pos] == 0) {
                digitalWrite(b_on_pin, HIGH);
                digitalWrite(b_off_pin, HIGH);
                b_str_pos = 0;
                b_chr_pos = 0;
            } else if (b_chr_pos < 8) {
                if (message[b_str_pos] & (1 << b_chr_pos)) {
                    digitalWrite(b_on_pin, HIGH);
                    digitalWrite(b_off_pin, LOW);
                } else {
                    digitalWrite(b_on_pin, LOW);
                    digitalWrite(b_off_pin, HIGH);
                }
                ++b_chr_pos;
            } else {
                digitalWrite(b_on_pin, HIGH);
                digitalWrite(b_off_pin, HIGH);
                b_chr_pos = 0;
                ++b_str_pos;
            }
        }
    }
}

void step_morse(unsigned long now) {
    if (now > next_unit) {
        --units;
        next_unit = now + unit_length_millis;
        if (units <= 0) {
            if (morse_on) {
                //Serial.print(" ");
                units = 1;
                morse_on = false;
                digitalWrite(morse_pin, LOW);
            } else {
                if (message[str_pos] == 0) {
                    //Serial.println("****EOM");
                    units = 4;
                    str_pos = 0;
                    chr_pos = 0;
                }
                while(morse[message[str_pos]] == 0) {
                    //Serial.print(message[str_pos]);
                    str_pos++;
                    chr_pos = 0;
                    if(message[str_pos] == 0) {
                        //Serial.print("####");
                        units = 4;
                        str_pos = 0;
                        break;
                    }
                }
                if (units <= 0) {
                    char chr = morse[message[str_pos]][chr_pos];
                    if (chr == 0) {
                        //Serial.print("__");
                        units = 2;
                        chr_pos = 0;
                        str_pos++;
                    } else if (chr == ';') {
                        //Serial.print(";;");
                        units = 2;
                        chr_pos++;
                    } else if (chr == '.') {
                        //Serial.print(".");
                        units = 1;
                        chr_pos++;
                        morse_on = true;
                        digitalWrite(morse_pin, HIGH);
                    } else if (chr == '-') {
                        //Serial.print("-");
                        units = 3;
                        chr_pos++;
                        morse_on = true;
                        digitalWrite(morse_pin, HIGH);
                    } else if (chr == ' ') {
                        //Serial.print("++++");
                        units = 4;
                        if (morse[message[str_pos]][++chr_pos] == 0) {
                            str_pos++;
                            chr_pos = 0;
                        }
                    }
                }
            }
        }
    }
}
