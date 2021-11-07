// TODO: 
// Document and comment files
// Functionalize and refactor
// Implement debouncing delay
// Range-based for loop in main task


// FILE: arduino_synth_poc.ino
// 

#include <Wire.h>
#include <Adafruit_MCP4725.h>

// Declare DAC object
Adafruit_MCP4725 dac;

// Use DAC in 8-bit resolution mode
#define DAC_RESOLUTION    (8)

// Define Input Pin Map
#define B0 2
#define B1 3
#define B2 4
#define B3 5
#define B4 6
#define B5 7
#define B6 8
#define B7 9

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

bool OUT_STATE = true;  // Drives hi/low logic levels for square wave output to speaker module
bool setupFlag = true;  // Flag used for button-press logic enable
int octaveIdx = 1;

void setup() {

  // Initialize serial console w/ Baud Rate of 9600 Hz
  Serial.begin(9600);
  Serial.println("Hello!");

  // Set up GPIO input pins
  Serial.println("Setting Up I/O ...");
  pinMode(2, INPUT);
  pinMode(3, INPUT);
  pinMode(4, INPUT);
  pinMode(5, INPUT);
  pinMode(6, INPUT);
  pinMode(7, INPUT);
  pinMode(8, INPUT);
  pinMode(9, INPUT);

  // Initialize DAC IC
  // Serial.println("Initializing DAC ...");
  // dac.begin(0x62);

  pinMode(13, OUTPUT);        //Set the pin to be OUTPUT - drives square wave to speaker module
  
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

  for (int rowIdx = 0; rowIdx <3; rowIdx ++){
    for (int colIdx = 0; colIdx <8; colIdx++) {
        Serial.println(FREQ_LOOKUP[rowIdx][colIdx]);
      }
    }
}

void loop(){
  // put your main code here, to run repeatedly:
  
  setupFlag = 1;                  // Reset setupFlag after iteration through main loop

  if (digitalRead(11) == LOW){      // Octave shift down
    if (octaveIdx == 0){
      octaveIdx = 2;
      } else {
          octaveIdx--;
        }
        delay(500);
        Serial.print("Octave idx: ");
        Serial.println(octaveIdx);
  }

  if (digitalRead(12) == LOW){      // Octave shift up
    if (octaveIdx == 2){
      octaveIdx = 0;
      } else {
          octaveIdx++;
        }    
        delay(500);
        Serial.print("Octave idx: ");
        Serial.println(octaveIdx);
  }

  // Serial.print("Octave level =");
  // Serial.println(octaveIdx);
  
  while (digitalRead(B0) == LOW) {
    if (setupFlag) {

      OCR1A = FREQ_LOOKUP[octaveIdx][0];     // C4

      TIMSK1 |= B00000010;        // Set OCIE1A to 1 so we enable compare match A

      Serial.print("Output compare val: ");
      Serial.println(FREQ_LOOKUP[octaveIdx][0]);
     
      setupFlag = 0;
    }
  }

  TIMSK1 &= B00000000;            // Disable OCC - stops timer interrupts from firing after button released

  while (digitalRead(B1) == LOW) {
    if (setupFlag) {
      
      OCR1A = FREQ_LOOKUP[octaveIdx][1];     // D4

      TIMSK1 |= B00000010;        // Set OCIE1A to 1 so we enable compare match A

      Serial.print("Output compare val: ");
      Serial.println(FREQ_LOOKUP[octaveIdx][1]);
      
      setupFlag = 0;
    }
  }

  TIMSK1 &= B00000000;            // Disable OCC - stops timer interrupts from firing after button released

  while (digitalRead(B2) == LOW) {
    if (setupFlag) {

      OCR1A = FREQ_LOOKUP[octaveIdx][2];     // E4

      TIMSK1 |= B00000010;        // Set OCIE1A to 1 so we enable compare match A

      Serial.print("Output compare val: ");
      Serial.println(FREQ_LOOKUP[octaveIdx][2]);
      
      setupFlag = 0;
    }
  }

  TIMSK1 &= B00000000;            // Disable OCC - stops timer interrupts from firing after button released

  while (digitalRead(B3) == LOW) {
    if (setupFlag) {
      
      OCR1A = FREQ_LOOKUP[octaveIdx][3];     // F4

      TIMSK1 |= B00000010;        // Set OCIE1A to 1 so we enable compare match A

      Serial.print("Output compare val: ");
      Serial.println(FREQ_LOOKUP[octaveIdx][3]);
      
      setupFlag = 0;
    }
  }

  TIMSK1 &= B00000000;            // Disable OCC - stops timer interrupts from firing after button released

  while (digitalRead(B4) == LOW) {
    if (setupFlag) {

      OCR1A = FREQ_LOOKUP[octaveIdx][4];     // G4

      TIMSK1 |= B00000010;        // Set OCIE1A to 1 so we enable compare match A

      Serial.print("Output compare val: ");
      Serial.println(FREQ_LOOKUP[octaveIdx][4]);
      
      setupFlag = 0;
    }
  }

  TIMSK1 &= B00000000;            // Disable OCC - stops timer interrupts from firing after button released

  while (digitalRead(B5) == LOW) {
    if (setupFlag) {

      OCR1A = FREQ_LOOKUP[octaveIdx][5];     // A4

      TIMSK1 |= B00000010;        // Set OCIE1A to 1 so we enable compare match A


      Serial.print("Output compare val: ");
      Serial.println(FREQ_LOOKUP[octaveIdx][5]);
      
      setupFlag = 0;
    }
  }

  TIMSK1 &= B00000000;            // Disable OCC - stops timer interrupts from firing after button released

  while (digitalRead(B6) == LOW) {
    if (setupFlag) {

      OCR1A = FREQ_LOOKUP[octaveIdx][6];     // B4

      TIMSK1 |= B00000010;        // Set OCIE1A to 1 so we enable compare match A

      Serial.print("Output compare val: ");
      Serial.println(FREQ_LOOKUP[octaveIdx][6]);
      
      setupFlag = 0;
    }
  }

  TIMSK1 &= B00000000;            // Disable OCC - stops timer interrupts from firing after button released

  while (digitalRead(B7) == LOW) {
    if (setupFlag) {

      OCR1A = FREQ_LOOKUP[octaveIdx][7];     // C5

      TIMSK1 |= B00000010;        // Set OCIE1A to 1 so we enable compare match A

      Serial.print("Output compare val: ");
      Serial.println(FREQ_LOOKUP[octaveIdx][7]);
      
      setupFlag = 0;
    }
  }

  TIMSK1 &= B00000000;            // Disable OCC - stops timer interrupts from firing after button released
}

ISR(TIMER1_COMPA_vect) {

  TCNT1 = 0;                  //First, set the timer back to 0 so it resets for next interrupt

  OUT_STATE = !OUT_STATE;      //Invert output state

  digitalWrite(13, OUT_STATE); //Write new state to the speaker on pin D5

}
