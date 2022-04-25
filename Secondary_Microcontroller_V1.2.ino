#include <SPI.h>
#include "utilities_secondary.h"
#include <avr/interrupt.h>
#include <avr/io.h>

void setup() {
  SPCR |= (1 << SPE); //set chip as secondary device
  SPI.attachInterrupt();
  SPI.setClockDivider(SPI_CLOCK_DIV4);//default spi rate
  DDRD = 0xFF; //set D0-D7 as output
  cli();//disable interrupts while messing with them
  PCICR |= 0x03; //enable Port B and Port C for pin change interrupts
  PCMSK0 |= 0x01;//recognize pin changes on D8 (PCINT0): Home pulse (Z+, orange wire on TRD-S1024VD. 1 ppr)
  PCMSK1 |= 0x01;//recognize pin changes on A0 (PCINT8): position pulse (A+, black wire on TRD-S1024VD. 1024/4 = 256 ppr)
  TCCR1A = 0;//reset Timer1 control register A
  TCCR1B = 0;//reset Timer1 control register B
  TCNT1 = 0; //reset Timer1 count value
  OCR1A = MAX; //avoid CTC interrupts until needed
  TCCR1B |= (1 << WGM12) | (1 << CS12);//CTC mode with prescaler of 256 (avoid timer1 overflow during long orbits (15Hz VFD input (90 rpm): ~0.67 s/orbit  -> TCNT1 stays below MAX (MAX ~65,000). One less thing to worry about!))
  //  TIMSK1 |= (1 << OCIE1A); //enable TIMER1_COMPA_vect ISR. Moved this to main loop to occur upon first match detection to avoid random CTC interrupts occurring on overfloe condition during large idle periods
  sei();//enable all interrupts again
  pinMode(9, OUTPUT);
  digitalWrite(9, LOW); //enable buffer IC
  PORTD = 0xFF;

}

void loop() {

  //delta position method
  //PORTD = ~(((solenoid_pos[0] < pos and solenoid_pos[0] + thresh > pos and (solenoid_control[0])) << 0) | ((solenoid_pos[1] < pos and solenoid_pos[1] + thresh > pos and (solenoid_control[1])) << 1) | ((solenoid_pos[2] < pos and solenoid_pos[2] + thresh > pos and (solenoid_control[2]) << 2) | ((solenoid_pos[3] < pos and solenoid_pos[3] + thresh > pos and (solenoid_control[3])) << 3) | ((solenoid_pos[4] < pos and solenoid_pos[4] + thresh > pos and (solenoid_control[4])) << 4) | ((solenoid_pos[5] < pos and solenoid_pos[5] + thresh > pos and (solenoid_control[5])) << 5) | ((solenoid_pos[6] < pos and solenoid_pos[6] + thresh > pos and (solenoid_control[6])) << 6) | ((solenoid_pos[7] < pos and solenoid_pos[7] + thresh > pos and (solenoid_control[7])) << 7));
  //  for (int i = 0; i < num_modules; i++) {
  //    PORTD &= !(((solenoid_pos[i] < pos & solenoid_pos[i] + threshold > pos) & (solenoid_control[i])) << i);
  // 
  //onboard timer method
  if (all_data_loaded) {
    
    while (pos == old_pos and t_i) { } //don't enter on first iteration, check for update to pos on each subsequent iteration
    old_pos = pos;
    t[t_i] = match(pos);

    PORTD &= !(t[t_i]);
    if (t[t_i]) {
      mem_tcnt1[t_i] = TCNT1;
        
      if (!t_i) {
        OCR1A_vals[t_i] = mem_tcnt1[t_i] + THR;
        if (OCR1A_vals[t_i] >= MAX) {
          OCR1A_vals[t_i] -= MAX; //perserve temporal data if overflow
        }
        TIMSK1 |= (1 << OCIE1A); //enable TIMER1_COMPA_vect ISR
      }
      else {
        OCR1A_vals[t_i] = mem_tcnt1[t_i] - mem_tcnt1[t_i - 1];
      }
      t_i++;
    }
    if (can_set and t_i) {
      OCR1A = OCR1A_vals[o_i];
      can_set = false;
    }
  }
}
