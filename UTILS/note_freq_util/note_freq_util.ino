// FILE    : note_freq_util.ino
// AUTHOR  : Stephen Stranahan
// DATE    : 10/28/2021

// Simple sketch to produce three square wave pulse outputs on an Arduino GPIO pin
// Produce some simple square wave notes at output for testing audio subcircuits

#define outPin 13

const uint16_t C2_val = 15287;  // OCC value for C2  -- These might work - needs testing
const uint16_t C4_val = 3822;   // OCC value for C4
const uint16_t C6_val = 955;    // OCC value for C6

bool OUT_STATE = false;         // Drives hi/low logic levels for square wave output to speaker module

unsigned int octaveIdx = 1;

unsigned int stackSize = 0;

void setup() {
  unsigned int i, j;
  
  unsigned int loopIdx = 0;
  
  // Initialize serial console w/ Baud Rate of 9600 Hz
  Serial.begin(9600);
  Serial.println("Hello!");

  // Set up GPIO input pins
  Serial.println("Setting Up I/O ...");
  for (loopIdx = 2; loopIdx <= 11; loopIdx++) {
      pinMode(loopIdx, INPUT);
    }

  pinMode(outPin, OUTPUT);

  cli();                      //stop interrupts for till we make the settings
  
  /*1. First we reset the control register to amke sure we start with everything disabled.*/
  TCCR1A = 0;                 // Reset entire TCCR1A to 0
  TCCR1B = 0;                 // Reset entire TCCR1B to 0

  /*2. We set the prescalar to the desired value by changing the CS10 CS12 and CS12 bits. */
  // TCCR1B |= B00000100;           // Set CS12 to 1 so we get prescalar 256
  TCCR1B = B00000001;               // Set CS12 to 1 so we get prescalar 1

  /*3. We enable compare match mode on register A*/
  TIMSK1 |= B00000000;           //Set OCIE1A to 0 so we disable compare match A

  /*3. We enable compare match mode on register A*/
  // TIMSK1 |= B00000010;        //Set OCIE1A to 1 so we enable compare match A

  sei();                         //Enable back the interrupts

  
  Serial.println("Initialization complete ...");  
  
}

void loop() {
  
  unsigned int btnCntr = 0;
  unsigned int loopIdx = 0;
  unsigned int lastPress = 0;
  unsigned int btnNumber;
  
  if (digitalRead(11) == LOW){      // Octave shift down
    if (octaveIdx == 0){
      octaveIdx = 2;
      } else {
          octaveIdx--;
        }
        delay(100);  // Debounce
  }

  if (digitalRead(12) == LOW){      // Octave shift up
    if (octaveIdx == 2){
      octaveIdx = 0;
      } else {
          octaveIdx++;
        }    
        delay(100);  // Debounce
  }

  // Scan keyboard task
  for (loopIdx = 2; loopIdx <= 9; loopIdx++) {
      if(digitalRead(loopIdx) == LOW) {                      
            // pressedFlags[loopIdx] = 1;         
            btnStack.push(loopIdx);
            delay(50);
      }
  }

  stackSize = btnStack.count();

  if (!btnStack.isEmpty()) {
    btnNumber = btnStack.peek();

    // Enable output task  - issue here
    for (loopIdx = 0; loopIdx <= stackSize; loopIdx++) {
      btnNumber = btnStack.peek();
        if (digitalRead(btnNumber) == HIGH) {
          btnStack.pop();
          stackSize--;
        } else {
           OCR1A = FREQ_LOOKUP[octaveIdx][btnNumber - 2];
           TIMSK1 |= B00000010;        // Set OCIE1A to 1 so we enable compare match A
        }
     }
  }

    Serial.print("Stack size: ");
    Serial.println(btnStack.count());

    //Disable output if no buttons held
    if (btnStack.isEmpty()) {
          TIMSK1 &= B00000000;   // Turn off timer system
    }   
}

ISR(TIMER1_COMPA_vect) {

  TCNT1 = 0;                  //First, set the timer back to 0 so it resets for next interrupt

  OUT_STATE = !OUT_STATE;      //Invert output state

  digitalWrite(outPin, OUT_STATE); //Write new state to the speaker on pin D5

}


void setup() {

  // Initialize serial console w/ Baud Rate of 9600 Hz
  Serial.begin(9600);
  Serial.println("Hello!");

  // Set up GPIO input pins
  Serial.println("Setting Up I/O ...");
  
  pinMode(outPin, OUTPUT);

  cli();                      // Disable global interrupts - atomize setup ritual
  
  // Reset timer control registers
  TCCR1A = 0;                 // Reset entire TCCR1A to 0
  TCCR1B = 0;                 // Reset entire TCCR1B to 0

  TCCR1B |= B00000010;        // Set prescaler = 8

  TIMSK1 &= B00000000;        // Disable compare match to start

  sei();                      // Re-enable global interrupts

  Serial.println("Initialization complete ...");  
  
}

void loop() {

  OCR1A = C2_val;             // Set output compare to num of clock ticks for given freq
  // OCR1A = C4_val;          // Uncomment these lines to enable pulse at these freqs
  // OCR1A = C6_val;          // Uncomment these lines to enable pulse at these freqs
  
  TIMSK1 |= B00000010;        // Enable compare match - now ISR will execute at given freq.

}

ISR(TIMER1_COMPA_vect) {

  TCNT1 = 0;                        //First, set the timer back to 0 so it resets for next interrupt

  OUT_STATE = !OUT_STATE;           //Invert output state

  digitalWrite(outPin, OUT_STATE);  //Write new state to the speaker on pin D5

}
