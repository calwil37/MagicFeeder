/*
  globals and functions utilized on NANO_P

  Author: William Caldbeck
  Date: 1/31/2022

  Revisions:
*/

const int SERIAL_BUFFER_SIZE = 64;//can be adjusted with core modification: https://www.hobbytronics.co.uk/arduino-serial-buffer-size
const int NUM_SOLENOIDS = 64;//number of solenoids
const int NUM_MODULES = 8; //number of secondary Arduino Nanos
//const int NUM_SOLENOIDS_PER_MODULE = NUM_SOLENOIDS / NUM_MODULES; //number of solenoids each module controls
byte solenoid_data[NUM_SOLENOIDS];
byte control_data[NUM_SOLENOIDS];
//byte port_masks[NUM_MODULES]; //Data direction register(DDR) data derived from control_data vector. '1' enables a particular outpin pin
volatile int interrupt_cnt = 0; //number of visits to ISR(PCINT0_vect) - see "ISRs.h"
const long baud_rate = 115200; //for transmission and reception confirmation, must align with baud rate on controlling device.
volatile bool read_data = false;

ISR(PCINT0_vect) { //D8 - home pulse
  interrupt_cnt++;
  if (interrupt_cnt % 2) {//ignore falling edge of home pulse
    read_data = true; //allow data read and transmission in main loop
  }
  else {
    interrupt_cnt = 0;
  }

}

void chip_select(int n) {//Selects a secondary device to write an instruction byte to. THIS needs to be redone!
  PORTD &= ~0x80; //disable demux during bit writing operations to avoid incorrect addressing
  for (int i = 0; i < 3; i++) {
    bitWrite(PORTD, i + 4, bitRead(n, i)); //convert integer chip number to appropriate selector byte pattern. This only affects pins D4-D6 of PORTD!
  }

  //PORTD = (n<<4); I think this is an alternative to the for loop
  PORTD |= 0x80; //enable demux, write D7 high
}
