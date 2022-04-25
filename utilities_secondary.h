const int NUM_SOLENOIDS = 8;
const int THR = 625; //number of timer ticks per solenoid pulsewidth. With prescaler of 256, 1 tick is 16us. Recommend minimum pulsewidth of 10 ms, so THR >= 625 should do it
#define MAX 0xFFFF //max value of TCNT1, OCR1A
//const int thresh = 10; //delta-position method
byte solenoid_pos[NUM_SOLENOIDS];
byte solenoid_control[NUM_SOLENOIDS];
volatile bool first = true;
volatile int interrupt_cnt0 = 0;
volatile int interrupt_cnt1 = 0;
volatile int index = 0;
volatile int pos = 0;
volatile int t_i = 0;
volatile int o_i = 0;
volatile byte t[NUM_SOLENOIDS] = {0};
volatile long OCR1A_vals[NUM_SOLENOIDS] = {0};
volatile long mem_tcnt1[NUM_SOLENOIDS] = {0};
volatile bool can_set = true;
volatile bool all_data_loaded = false;
int old_pos;

byte match(int current_pos) {
  byte t = 0;
  for (int i = 0; i < NUM_SOLENOIDS; i++) {
    t |= (((solenoid_pos[i] == current_pos) and (solenoid_control[i])) << i);
  }
  return t;
}


ISR(SPI_STC_vect) { //SPI reception ISR
  if (first) {
    solenoid_pos[index] = SPDR;
    first = false;
  }
  else {
    solenoid_control[index] = SPDR;
    index++;
    first = true;
  }
  if (index == NUM_SOLENOIDS) {
    all_data_loaded = true;
    index = 0;
       
  }
}


ISR(PCINT0_vect) { //D8 - home pulse ISR
  interrupt_cnt0++;
  if (interrupt_cnt0 % 2) {//ignore falling edge of home pulse
    pos = 0;
    all_data_loaded = false;
  }
  else {
    interrupt_cnt0 = 0;
  }
}

ISR(PCINT1_vect) { //A2 - position pulse ISR
  interrupt_cnt1++;
  if (interrupt_cnt1 % 2) {//ignore falling edge of home pulse
    pos++; //increment current orbital position
  }
  else {
    interrupt_cnt1 = 0;
  }
}

ISR (TIMER1_COMPA_vect) {//CTC ISR
  PORTD |= t[o_i];
  can_set = true;
  o_i++;
  if (t_i == o_i) {
    t_i = 0;
    o_i = 0;
    memset(t, 0, NUM_SOLENOIDS);//zero out this vector for next time - maybe not required?
    memset(OCR1A_vals, 0, NUM_SOLENOIDS);//zero out this vector for next time
  }
}
