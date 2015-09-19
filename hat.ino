#include <EEPROM.h>

#include "morse.h"

const int max_message = 0x100;
char message[max_message] = "Hello, World.";

/** Morse Code */
const unsigned long unit_length_millis = 0xFF;
unsigned long next_unit;
int units = 0;
bool morse_on = false;

int str_pos = 0;
int chr_pos = 0;

const int morse_pin = 3;
/** End Morse Code */

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

    for(int i = 0; i < max_message; i++) {
      message[i] = EEPROM.read(i);
    }

    Serial.begin(9600);

    // Morse Code
    pinMode(morse_pin, OUTPUT);
    digitalWrite(morse_pin, LOW);
    
    next_unit = millis() + unit_length_millis;

    // Message Update
    state = CA;
}

void loop() {  
    unsigned long now = millis();
    step_morse(now);

    if(Serial.available()) {
        unsigned char c = Serial.read();
        switch (state) {
        case CA:
            if(c == '\xCA') {
                state = FE;
            }
            break;
        case FE:
            if(c == '\xFE') {
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

