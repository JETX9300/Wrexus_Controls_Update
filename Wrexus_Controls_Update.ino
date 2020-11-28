#include <FastLED.h>

// Define Connections to 74HC165N
#define LOAD_PIN      23 // PL pin 1
#define CLK_ENAB_PIN  25 // CE pin 15
#define DATA_IN_PIN   27 // Q7 pin 9
#define CLK_IN_PIN    29 // CP pin 2

// Define Shift Register Variables/Constants
#define NUM_SHIFT_INPUTS 24 // Number of individual inputs coming in from the shift registers
boolean currentShiftInput[NUM_SHIFT_INPUTS]; // Array to hold the individual values of incoming shift register values | 0-7 = shift_0, 8-15 = shift_1, 16-23 = shift_2
boolean lastShiftInput[NUM_SHIFT_INPUTS]; // Array to hold the previous values of incoming shift register values
boolean shiftInputDebounce[NUM_SHIFT_INPUTS]; // Array to hold the number of times the new value of the 

// Define FastLED Variables/Constants
#define LED_TYPE          WS2812  // type of RGB LEDs
#define COLOR_ORDER       GRB     // sequence of colors in data stream
#define NUM_LEDS          8       // 8 LEDs numbered [0..7]
#define DATA_PIN          31       // LED data pin
byte brightness = 50; // brightness range [off..on] = [0..255]
uint8_t gHue = 0; // starting hue value for rolling rainbow
CRGB leds[NUM_LEDS]; // Define the array of RGB control data for each LED

#define OFF   0 // Switch state definition for readability
#define ON    1 // Switch state definition for readability
#define LEFT  1 // Switch state definition for readability
#define AUTO  2 // Switch state definition for readability
#define RIGHT 2 // Switch state definition for readability

// Define Toggle Switch Variables/Constants
size_t switchPosition[] = {OFF,OFF,OFF,OFF,OFF};
#define NUM_SWITCHES sizeof(switchPosition)
boolean InputUpdated = false; // Latch to skip updateOutputs()


void setup() {
  // Serial.begin(9600);
  // initialize library using CRGB LED array
  FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

  // Setup 74HC165 Connections
  pinMode(LOAD_PIN, OUTPUT);
  pinMode(CLK_ENAB_PIN, OUTPUT);
  pinMode(CLK_IN_PIN, OUTPUT);
  pinMode(DATA_IN_PIN, INPUT);

  // Fill arrays with starting values
  for(size_t cv = 0; cv < NUM_SHIFT_INPUTS; cv++){
    lastShiftInput[cv] = false;
    shiftInputDebounce[cv] = false;
  }
}


void loop() {

  // Set Brightness
  FastLED.setBrightness(brightness);
  
  EVERY_N_MILLISECONDS(100) {readInputs();}
  if(InputUpdated){
    updateOutputs();
  }
 
  // activate all changes to LEDs
  FastLED.show();
}


// Functions


void readInputs(){
  Serial.println("entered readInputs()");
  
  byte shift_0, shift_1, shift_2; // Variables to hold incoming binary data
  
  /*
   * ---INPUT LIST---
   * Bumper Light Bar   ON (shift_0, 0) | OFF | AUTO (shift_0, 1)
   * Main Light Bar     ON (shift_0, 2) | OFF | AUTO (shift_0, 3)
   * Side Light Bars    ON (shift_0, 4) | OFF | AUTO (shift_0, 5)
   * Rear Light Bar     ON (shift_0, 6) | OFF | AUTO (shift_0, 7)
   * 
   * Side Light Bars Momentary    LEFT (shift_1, 0) | OFF | RIGHT (shift_1, 1)
   */
  
  // Write pulse to load pin
  digitalWrite(LOAD_PIN, LOW);
  delayMicroseconds(5);
  digitalWrite(LOAD_PIN, HIGH);
  delayMicroseconds(5);

  // Get data from 74HC165
  digitalWrite(CLK_IN_PIN, HIGH);
  digitalWrite(CLK_ENAB_PIN, LOW);
  shift_0 = shiftIn(DATA_IN_PIN, CLK_IN_PIN, MSBFIRST);
  shift_1 = shiftIn(DATA_IN_PIN, CLK_IN_PIN, MSBFIRST);
  shift_2 = shiftIn(DATA_IN_PIN, CLK_IN_PIN, MSBFIRST);
  digitalWrite(CLK_ENAB_PIN, HIGH);

  Serial.print("Value of first pin: ");
  Serial.println(bitRead(shift_0, 0));

  // Move shift data into an array
  for(size_t cv = 0; cv < NUM_SHIFT_INPUTS; cv++){
    if(cv >= 0 and cv <= 7){
      currentShiftInput[cv] = bitRead(shift_0, cv);
    } else if(cv >= 8 and cv <= 15){
      currentShiftInput[cv] = bitRead(shift_1, (cv - 8));
    } else if(cv >= 16 and cv <= 23){
      currentShiftInput[cv] = bitRead(shift_2, (cv - 16));
    }
  }

  // Check if any inputs have changed since last pass
  for(size_t cv = 0; cv < NUM_SHIFT_INPUTS; cv++){
    if(currentShiftInput[cv] != lastShiftInput[cv]){
      // Check if we have debounced the input
      if(shiftInputDebounce[cv] == false){
        shiftInputDebounce[cv] = true;
      } else if(shiftInputDebounce[cv] == true){
        lastShiftInput[cv] = currentShiftInput[cv];
        shiftInputDebounce[cv] = false;
        InputUpdated = true; // Turn on flag to allow UpdateOutputs() to run
      }
    } else {
      shiftInputDebounce[cv] = false; // If debounce was started in error, reset the debounce flag.
    }
  }

  // Convert raw shift inputs to corrisponding switch positions for readability in the output and mode decisions
  // Bumper Bar
  if(lastShiftInput[0]){
    switchPosition[0] = ON;
  } else if(lastShiftInput[1]){
    switchPosition[0] = AUTO;
  } else {
    switchPosition[0] = OFF;
  }

  // Main Light Bar
  if(lastShiftInput[2]){
    switchPosition[1] = ON;
  } else if(lastShiftInput[3]){
    switchPosition[1] = AUTO;
  } else {
    switchPosition[1] = OFF;
  }

  // Side Light Bars
  if(lastShiftInput[4]){
    switchPosition[2] = ON;
  } else if(lastShiftInput[5]){
    switchPosition[2] = AUTO;
  } else {
    switchPosition[2] = OFF;
  }

  // Rear Light Bar
  if(lastShiftInput[6]){
    switchPosition[3] = ON;
  } else if(lastShiftInput[7]){
    switchPosition[3] = AUTO;
  } else {
    switchPosition[3] = OFF;
  }

  // Side Light Bars Momentary
  if(lastShiftInput[8]){
    switchPosition[4] = LEFT;
  } else if(lastShiftInput[9]){
    switchPosition[4] = RIGHT;
  } else {
    switchPosition[4] = OFF;
  }
}


void updateOutputs(){
  Serial.println("entered updateOutputs()");
  /*
   ---Indicator List---
   Bumper Light Bar LED 7
   Main Light Bar   LED 6
   Side Light Bars  LED 5
   Rear Light Bar   LED 4
   */
  
  // Bumper Light Bar
  switch (switchPosition[0]) {
    case ON:
      leds[7] = CRGB(0, 255, 0);
      break;
    case OFF:
      leds[7] = CRGB(0, 0, 0);
      break;
    case AUTO:
      leds[7] = CRGB(255, 165, 0);
      break;
  }

  // Main Light Bar
  switch (switchPosition[1]) {
    case ON:
      leds[6] = CRGB(0, 255, 0);
      break;
    case OFF:
      leds[6] = CRGB(0, 0, 0);
      break;
    case AUTO:
      leds[6] = CRGB(255, 165, 0);
      break;
  }

  // Side Light Bars
  switch (switchPosition[2]) {
    case ON:
      leds[5] = CRGB(0, 255, 0);
      break;
    case OFF:
      leds[5] = CRGB(0, 0, 0);
      break;
    case AUTO:
      leds[5] = CRGB(255, 165, 0);
      break;
  }
  
  // Rear Light Bar
  switch (switchPosition[3]) {
    case ON:
      leds[4] = CRGB(0, 255, 0);
      break;
    case OFF:
      leds[4] = CRGB(0, 0, 0);
      break;
    case AUTO:
      leds[4] = CRGB(255, 165, 0);
      break;
  }

  // Side Light Bars Momentary
  switch (switchPosition[4]) {
    case ON:
      leds[5] = CRGB(0, 255, 0);
      break;
    case OFF:
      // Do nothing and let the original Side Light Bars switch take control again
      break;
    case AUTO:
      leds[5] = CRGB(0, 255, 0);
      break;
  }

  InputUpdated = false; // Turn off flag to keep UpdateOutputs() from running when nothing has changed.
}
