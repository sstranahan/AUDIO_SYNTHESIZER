#include <stdlib.h>
#include <StackArray.h>

// Define Input Pin Map
#define K0 2
#define K1 3
#define K2 4
#define K3 5
#define K4 6
#define K5 7
#define K6 8
#define K7 9

#define octDwn 11
#define octUp 12

#define outPin 13

// OCCR Values for C4 - C5 octave - will achieve physically correct
// frequencies for square wave output
const uint16_t FREQ_LOOKUP[3][8] {
  {30577, 27240, 24269, 22907,
    20407, 18181, 16197, 15288},
  {15288, 13620, 12134, 11453,
    10203, 9090, 8098, 7644},
  {7644, 6809, 6066, 5726,
    5101, 4545, 4049, 3821}
};

bool btnFlags[8] = {0,0,0,0,0,0,0,0};

StackArray<unsigned int> btnStack;

bool OUT_STATE = false;  // Drives hi/low logic levels for square wave output to speaker module

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

  int i;
  
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
            if (btnFlags[loopIdx - 2] == 0){
              btnFlags[loopIdx - 2] = 1;                              
              btnStack.push(loopIdx - 2);
            }
            delay(10);
      }
  }

  stackSize = btnStack.count();

  Serial.println(stackSize);

  if (!btnStack.isEmpty()) {
    // btnNumber = btnStack.peek();

    // Enable output task  - issue here -- Stack overflowing?? -- Print stacksize to debug
    for (loopIdx = 0; loopIdx <= stackSize; loopIdx++) {
      btnNumber = btnStack.peek();
        if (digitalRead(btnNumber + 2) == HIGH) {
          btnStack.pop();
          btnFlags[btnNumber] = 0;
          stackSize--;
        } else {
           OCR1A = FREQ_LOOKUP[octaveIdx][btnNumber];
           TIMSK1 |= B00000010;        // Set OCIE1A to 1 so we enable compare match A
        }
     }
  }
    
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
