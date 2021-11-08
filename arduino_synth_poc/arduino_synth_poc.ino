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
#include "I2C.h"
#include "IQS5xx.h"
#include "defs.h"

/*******************************************************************************************/


/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////// GLOBAL DEFINES //////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

// Wave selection rotary switch pin assigns
#define ws0 22
#define ws1 23
#define ws2 24
#define ws3 25

#define octDwn 11
#define octUp 12

// Primary output signal
#define outPin 13

enum waveSelect_T {
  SQUARE,
  TRIANGLE,
  SAW,
  SINE
  };
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

waveSelect_T waveSelect;

unsigned int octaveIdx = 2;           // Default octave is middle octave
unsigned int stackSize = 0;

// Vars for trackpad comms
uint8_t   Data_Buff[44];
uint16_t  ui16SnapStatus[15], ui16PrevSnap[15];
uint16_t  xCoord;
uint16_t  yCoord;

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

  pinMode(RDY_PIN, INPUT);

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


  // Initialize trackpad
  I2C_Setup();
  //
  // Clear the reset indication bit, so that from here on a reset can be 
  // detected if the bit becomes set
  //
  IQS5xx_AcknowledgeReset();
  //
  // Read the version and display on the serial terminal
  //
  IQS5xx_CheckVersion();
  //
  // End the communication window
  //
  Close_Comms();
}
/*******************************************************************************************/

void loop() {

  unsigned int loopIdx = 0;
  unsigned int btnNumber;

  int id;
  uint8_t   ui8TempData[30], i;

  /////////////////////////////////////////////////////////////////////////////////////////////
  /////////////////////////////// READ WAVE SELECT TASK ///////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////////////////////
  // NOTES :
  ///////////////////////////////////////////////////////////////////////////////////////////// 

  for (int i = 0; i < 4; i++){
      if (digitalRead(i + 22) == LOW) {
          waveSelect = i;
        }
    }
  
  // Serial.println(waveSelect);

  /*******************************************************************************************/

  /////////////////////////////////////////////////////////////////////////////////////////////
  ///////////////////////////////   READ TRACKPAD TASK  ///////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////////////////////
  // NOTES :
  ///////////////////////////////////////////////////////////////////////////////////////////// 
  
  RDY_wait();

//  if (digitalRead(11) == LOW){
//      digitalWrite(35, LOW);
//   } else {
//      digitalWrite(35, HIGH);
//   }
  
  I2C_Read(GestureEvents0_adr, &Data_Buff[0], 44);

  if((Data_Buff[3] & SNAP_TOGGLE) != 0)
  {
    // If there was a change in a snap status, then read the snap status 
    // bytes additionally. Keep previous valus to identify a state change
    //
    I2C_Read(SnapStatus_adr, &ui8TempData[0], 30);
    for(i = 0; i < 15; i++)
    {
      ui16PrevSnap[i] = ui16SnapStatus[i];
      ui16SnapStatus[i] = ((uint16_t)(ui8TempData[2*i])<<8) + 
                 (uint16_t)ui8TempData[(2*i)+1];
    }
  }
  //
  // Terminate the communication session, so that the IQS5xx can continue 
  // with sensing and processing
  //
  Close_Comms();
  //
  // Process received data 
  //
  Process_XY();
//
//  for (id = 0; id < 5; id++){
//    xCoord = ((Data_Buff[(7*id)+9] << 8) | (Data_Buff[(7*id)+10])); //9-16-23-30-37//10-17-24-31-38
//    yCoord = ((Data_Buff[(7*id)+11] << 8) | (Data_Buff[(7*id)+12])); //11-18-25-32-39//12-19-26-33-40
//  }
  
  Serial.print("X: ");
  Serial.print(xCoord);
  Serial.print("  Y: ");
  Serial.println(yCoord);

  /*******************************************************************************************/

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

  // Bug in here - if p&h button, p&h another button, release first button,
  //               cannot use first button again while second button held
  //
  //               Probably need a task to scan and clear stack outside of output loop

  for (loopIdx = 2; loopIdx <= 9; loopIdx++) {
    if (digitalRead(loopIdx) == LOW) {
      if (btnFlags[loopIdx - 2] == 0) {         // If button already on stack, ignore
        btnFlags[loopIdx - 2] = 1;
        btnStack.push(loopIdx - 2);
      }
      delay(10);
    } else {
      if (btnFlags[loopIdx - 2] == 1) {
        btnFlags[loopIdx - 2] = 0;
      }
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
