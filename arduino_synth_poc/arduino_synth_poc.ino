/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
// FILE      : arduino_synth_poc.ino -- v1.2.0 -- DEV branch
/////////////////////////////////////////////////////////////////////////////////////////////
// PROJECT   : AUDIO_SYNTHESIZER
/////////////////////////////////////////////////////////////////////////////////////////////
// AUTHOR(S) : Stephen Stranahan, Colin Olesky
/////////////////////////////////////////////////////////////////////////////////////////////
// DESC      : Arduino sketch to drive audio synthesizer primary wave output, read trackpad
////////////// X and Y coordinates, output quantized control voltages to DAC
//////////////
////////////// Uses stack to sote button press index, will remain on stack while button is
////////////// held and pop button index from stack when button is released.
//////////////
////////////// Implements octave shifting functionality, indexes in to output compare timer
////////////// value 2D lookup table on octave and note indeces. Will use timer interrupt to
////////////// output proper frequency square wave

/*******************************************************************************************/
/*******************************************************************************************/

/////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////// INCLUDES /////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
#include <stdlib.h>
#include <StackArray.h>
/*******************************************************************************************/


/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////// GLOBAL DEFINES //////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
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
/*******************************************************************************************/


/////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////// GLOBAL VARIABLES /////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

// OCCR Values for C4 - C5 octave - will achieve physically correct
// frequencies for square wave output
const uint16_t FREQ_LOOKUP[5][8] {
  {15288, 13260, 12134, 11453, 10204, 9091, 8099, 7645},
  {7645 , 6811 , 6068 , 5727 , 5102 , 4545, 4049, 3822},
  {3822 , 3405 , 3034 , 2863 , 2551 , 2272, 2025, 1911},
  {1911 , 1703 , 1517 , 1432 , 1275 , 1136, 1012, 955},
  {955  , 851  , 758  , 716  , 638  , 568 , 506 , 477}
};

bool btnFlags[8] = {0, 0, 0, 0, 0, 0, 0, 0}; // Flags prevent putting button on stack mult. times

StackArray<unsigned int> btnStack;    // Stack to keep track of order of button inputs

bool OUT_STATE = false;               // Drives hi/low logic levels for square wave output
// to speaker module

unsigned int octaveIdx = 2;           // Default octave is middle octave
unsigned int stackSize = 0;

/*******************************************************************************************/


/////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////// SETUP RITUAL ///////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
// NOTES : Setup ritual will initialize inputs and outputs, initialize serial terminal,
////////// and set up timer interrupt subsystem.
/////////////////////////////////////////////////////////////////////////////////////////////

void setup() {

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

  cli();                            // Atomize interrupt system initialization

  /* First we reset the control register to make sure we start with everything disabled */
  TCCR1A = 0;                       // Reset entire TCCR1A to 0
  TCCR1B = 0;                       // Reset entire TCCR1B to 0

  /*2. We set the prescalar to the desired value by changing the CS10 CS12 and CS12 bits. */
  // TCCR1B |= B00000100;           // Set CS12 to 1 so we get prescalar 256
  // TCCR1B = B00000001;            // Set CS12 to 1 so we get prescalar 1

  TCCR1B = B00000010;               // Prescaler = 8

  TIMSK1 |= B00000000;              // Start with output compare disabled, will enable
  // on button input

  sei();                            // Enable back global interrupts

  Serial.println("Initialization complete ...");
}
/*******************************************************************************************/

void loop() {
  
  unsigned int loopIdx = 0;
  unsigned int btnNumber;

/////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////// OCTAVE SHIFT TASK /////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
// NOTES : 
/////////////////////////////////////////////////////////////////////////////////////////////
  if (digitalRead(11) == LOW) {     // Octave shift down
    if (octaveIdx == 0) {
      octaveIdx = 4;
    } else {
      octaveIdx--;
    }
    delay(200);                     // Debounce
  }

  if (digitalRead(12) == LOW) {     // Octave shift up
    if (octaveIdx == 4) {
      octaveIdx = 0;
    } else {
      octaveIdx++;
    }
    delay(200);                     // Debounce
  }
/*******************************************************************************************/
  
/////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////// KEYBOARD SCAN TASK ////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
// NOTES : Will push index of button pressed on stack and set flag to avoid redundancy
/////////////////////////////////////////////////////////////////////////////////////////////
  for (loopIdx = 2; loopIdx <= 9; loopIdx++) {
    if (digitalRead(loopIdx) == LOW) {
      if (btnFlags[loopIdx - 2] == 0) {         // If button already on stack, ignore
        btnFlags[loopIdx - 2] = 1;
        btnStack.push(loopIdx - 2);
      }
      delay(10);
    }
  }
/*******************************************************************************************/

/////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////// PRODUCE OUTPUT TASK ///////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
// NOTES : Will check stack - if button on stack is held, will set output compare and 
////////// enable interrupts, causing ISR to execute and toggle output pin whenever
////////// one half period has elapsed.
//////////
////////// If button not held, will pop value from stack. 
/////////////////////////////////////////////////////////////////////////////////////////////
  stackSize = btnStack.count();

  if (!btnStack.isEmpty()) {
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
/*******************************************************************************************/

/////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////// OUTPUT ISR ////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
// NOTES : Simply toggles output pin state every time interrupt timer hits one half period 
////////// for frequency of note at top of stack
/////////////////////////////////////////////////////////////////////////////////////////////
ISR(TIMER1_COMPA_vect) {
  TCNT1 = 0;                        //First, set the timer back to 0 so it resets for next interrupt
  OUT_STATE = !OUT_STATE;           //Invert output state
  digitalWrite(outPin, OUT_STATE);  //Write new state to the speaker on pin D5
}
/*******************************************************************************************/

/*********************************** END FILE **********************************************/
/*******************************************************************************************/
