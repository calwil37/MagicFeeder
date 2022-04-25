/*==================================================================
  This script is to be uploaded to the Primary Arduino Nano (NANO_P) of the MagicFeeder.

  NANO_P is responsible for transmitting all solenoid instructions
  to the secondary Arduino Nanos present on the SPI network. NANO_P enables pin change interrupts
  and waits idly for a home pulse from the TRD-S1024VD optical encoder. Upon detection of the rising
  edge of this pulse, SPI transmission of all available instruction bytes within the serial buffer begins.
  After completing the full instruction transaction with all secondary modules on the SPI network,
  NANO_P waits once more for a home pulse rising edge. The process repeats, and the serial buffer is
  written to the secondary modules each time the MagicFeeder passes its home point.

  TODO: test relative speeds of different chip_select functions (see "utilities_primary.h")

  Author: William Caldbeck
  Date: 1/31/2022

  Revisions:
  3/26/2022: William Caldbeck- major restructuring. Filename changed to "Primary_Microntroller_V1.1" from "NANO_P". "NANO_P" involved an outdated SPI transmission method
  4/19/2022: William Caldbeck - introduced alternative to chip_select, needs testing, should work. Removed control_data condensing.
  ==================================================================*/

#include <SPI.h>
#include "utilities_primary.h"
#include <avr/interrupt.h>
#include <avr/io.h>

void setup() {
  DDRD |= 0xF0; //(Data Direction Register, Port D) D4-6: demuiltiplexer select pins A,B,and C. D7: demux enable (sn74hc138 pin G1) (set all as output)
  PORTD = 0; //disable demux and thus all secondary chip select lines
  SPI.begin(); //defaults to primary device
  SPI.setClockDivider(SPI_CLOCK_DIV4); //default rate
  Serial.begin(baud_rate); //for debugging
  cli();//disable interrupts while messing with them
  PCICR |= 0x01; //enable Port B for pin change interrupts
  PCMSK0 |= 0x01;//recognize pin changes on D8 (PCINT0): Home pulse (Z+, orange wire on TRD-S1024VD)
  sei();//enable interrupts
}

void loop() {


  if (read_data) {// we just need detection of a home pulse to begin tranmission.
    while (Serial.available() != SERIAL_BUFFER_SIZE); //wait for all data to arrive in buffer
    Serial.readBytes(solenoid_data, NUM_SOLENOIDS); //assume solenoid firing position data arrives as cluster of "num_solenoid" bytes first, followed by an equivalent number of solenoid control data (bit mask stuff)
    Serial.println();//send newline to external device to confirm receipt of first data and allow transmission of next data
    while (Serial.available() != SERIAL_BUFFER_SIZE);//wait for all control data to arrive
    Serial.readBytes(control_data, NUM_SOLENOIDS);
    //This procedure was designed when using hardware (DDRD) to disable solenoids. Now using software to do this.
    //    //condense control_data into "NUM_MODULES" control bytes
    //    for (int i = 0; i < NUM_MODULES; i++) {
    //      for (int j = 0; j < NUM_SOLENOIDS_PER_MODULE; j++) {
    //        port_masks[i] |= ((control_data[i * NUM_SOLENOIDS_PER_MODULE + j] & 0x01) << j); //first bit of each of the "num_solenoid" control_data bytes extracted and condensed into "NUM_MODULES" bytes
    //      }
    //    }
    //
    //    for (int i = 0; i < NUM_MODULES; i++) {
    //      chip_select(i);
    //      SPI.transfer(port_masks[i]); //writes DDRD info to each of the secondary microcontrollers
    //    }
    for (int i = 0; i < NUM_SOLENOIDS; i++) {
      chip_select(floor(i / NUM_MODULES)); //selects correct secondary to write to
      SPI.transfer(solenoid_data[i]); //write a singular position byte to selected chip
      SPI.transfer(control_data[i]);//write a singular control byte to selected chip
    }
    read_data = false; //prepare for next home pulse. Idle in this loop if no home pulse.
  }
}
