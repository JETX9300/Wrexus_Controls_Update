#include <FastLED.h>
#include <Adafruit_NeoPixel.h>

// Debug Mode Select
boolean debug = false; // Change to true will allow messages to print to the serial monitor

// Define connections to 74HC165N - Input Shift Registers
#define LOAD_PIN      23 // PL pin 1
#define CLK_ENAB_PIN  25 // CE pin 15
#define DATA_IN_PIN   27 // Q7 pin 9
#define CLK_IN_PIN    29 // CP pin 2

// Define Input Shift Register variables/constants
#define NUM_SHIFT_INPUTS 32 // Number of individual inputs coming in from the shift registers
boolean currentShiftInput[NUM_SHIFT_INPUTS]; // Array to hold the individual values of incoming shift register values | 0-7 = shift_0, 8-15 = shift_1, 16-23 = shift_2, 24-31 = shift_3
boolean lastShiftInput[NUM_SHIFT_INPUTS]; // Array to hold the previous values of incoming shift register values
boolean shiftInputDebounce[NUM_SHIFT_INPUTS]; // Array to hold the number of times the new value of the

// Define connections to 74HC595 - 5v Output Shift Registers
#define LATCH_PIN     35 // RCLK  pin 12
#define CLOCK_PIN     37 // SRCLK pin 11
#define DATA_PIN      33 // SER   pin 14

// Define 5v Output Shift Register variables/constants
byte Shift_0Last = 0; // Hold the last value of the shift register to determine when to run the update code

// Define FastLED Variables/Constants FASTLED
//#define LED_TYPE          WS2812  // type of RGB LEDs
//#define COLOR_ORDER       GRB     // sequence of colors in data stream
//#define NUM_LEDS          31      // 31 LEDs numbered [0..30]
//#define DATA_PIN          6       // LED data pin
//byte brightness = 50; // brightness range [off..on] = [0..255]
//uint8_t gHue = 0; // starting hue value for rolling rainbow
//CRGB leds[NUM_LEDS]; // Define the array of RGB control data for each LED

// Define Neopixel Variables/Constants
#define PIN 31
#define NUM_LEDS 31
byte HUDbrightness = 255; // brightness range of overhead indicator lights [off..on] = [0..255]
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, PIN, NEO_GRBW + NEO_KHZ800);
uint32_t hudColor = strip.Color(0, 0, 0, 255);
uint32_t white = strip.Color(0,0,0,255);
uint32_t red = strip.Color(255,0,0,0);
uint32_t green = strip.Color(0,255,0,0);
uint32_t blue = strip.Color(0,0,255,0);
uint32_t off = strip.Color(0,0,0,0);

#define OFF       0 // Switch state definition for readability
#define CENTER    0 // Switch state definition for readability
#define ON        1 // Switch state definition for readability
#define LEFT      1 // Switch state definition for readability
#define NEXT      1 // Switch state definition for readability
#define AUTO      2 // Switch state definition for readability
#define RIGHT     2 // Switch state definition for readability
#define PREVIOUS  2 // Switch state definition for readability
#define DOOR      2 // Switch state definition for readability

// Define Toggle Switch Variables/Constants
size_t switchPosition[] = {OFF,OFF,OFF,OFF,OFF,OFF,OFF,OFF,OFF,OFF,OFF,OFF,OFF,OFF,OFF,OFF,OFF};
#define NUM_SWITCHES sizeof(switchPosition)
boolean InputUpdated = false; // Latch to skip updateOutputs()

// Define light relay pins
// Relays are active LOW.  Write LOW on the pin to close relay and turn light ON
#define RELAY_MAIN_LIGHT  52  // Relay 0
#define RELAY_RIGHT_LIGHT 50  // Relay 1
#define RELAY_LEFT_LIGHT  48  // Relay 2
#define RELAY_REAR_LIGHTS 46  // Relay 3


void setup() {

  // Start serial monitor if Debug is set to true
  if (debug) {
    Serial.begin(57600);
  }
  
  // initialize library using CRGB LED array FASTLED
  //FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

  // Setup Input Shift Register connections
  pinMode(LOAD_PIN, OUTPUT);
  pinMode(CLK_ENAB_PIN, OUTPUT);
  pinMode(CLK_IN_PIN, OUTPUT);
  pinMode(DATA_IN_PIN, INPUT);

  // Setup 5v Output Shift Registers connections
  pinMode(LATCH_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(DATA_PIN, OUTPUT);

  // Send 5v Output Shift Registers their initial value
  digitalWrite(LATCH_PIN, LOW); // Bring RCLK LOW to keep outputs from changing while reading serial data
  shiftOut(DATA_PIN, CLOCK_PIN, LSBFIRST, 0); // Shift out the data
  digitalWrite(LATCH_PIN, HIGH); // Bring RCLK HIGH to change outputs

  // Fill arrays with starting values
  for(size_t cv = 0; cv < NUM_SHIFT_INPUTS; cv++){
    lastShiftInput[cv] = false;
    shiftInputDebounce[cv] = false;

  // Initialize Neopixels
  strip.setBrightness(HUDbrightness);
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  }

  // Fill the HUD with the initial color.
  strip.fill(hudColor, 0, 7);

  // Setup light relay pins
  pinMode(RELAY_MAIN_LIGHT, OUTPUT);
  pinMode(RELAY_RIGHT_LIGHT, OUTPUT);
  pinMode(RELAY_LEFT_LIGHT, OUTPUT);
  pinMode(RELAY_REAR_LIGHTS, OUTPUT);

  // Set initial state of relay pins to off.  Relays are active LOW
  digitalWrite(RELAY_MAIN_LIGHT, HIGH);
  digitalWrite(RELAY_RIGHT_LIGHT, HIGH);
  digitalWrite(RELAY_LEFT_LIGHT, HIGH);
  digitalWrite(RELAY_REAR_LIGHTS, HIGH);
}


void loop() {

  // Set Brightness FASTLED
  //FastLED.setBrightness(brightness);
  
  EVERY_N_MILLISECONDS(100) {readInputs();}
  if(InputUpdated){
    calculations();
    updateOutputs();
  }
 
  // activate all changes to LEDs FASTLED
  //FastLED.show();

  // activate all changes to LEDs
  strip.show();
}


// Functions


void readInputs(){
  
  byte shift_0, shift_1, shift_2, shift_3; // Variables to hold incoming binary data
  
  /*
   * ---INPUT LIST---
   * Bumper Light Bar   ON (shift_0, 0) | OFF | AUTO (shift_0, 1)
   * Main Light Bar     ON (shift_0, 2) | OFF | AUTO (shift_0, 3)
   * Side Light Bars    ON (shift_0, 4) | OFF | AUTO (shift_0, 5)
   * Rear Light Bar     ON (shift_0, 6) | OFF | AUTO (shift_0, 7)
   * 
   * GMRS Radio         ON (shift_1, 0) | OFF | AUTO (shift_1, 1)
   * Tunes Radio        ON (shift_1, 2) | OFF | AUTO (shift_1, 3)
   * Night Signal       ON (shift_1, 4) | OFF | AUTO (shift_1, 5)
   * Side Light Bars Momentary    LEFT (shift_1, 6) | OFF | RIGHT (shift_1, 7)
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
  shift_3 = shiftIn(DATA_IN_PIN, CLK_IN_PIN, MSBFIRST);
  digitalWrite(CLK_ENAB_PIN, HIGH);

  // Move shift data into an array
  for(size_t cv = 0; cv < NUM_SHIFT_INPUTS; cv++){
    if(cv >= 0 and cv <= 7){
      currentShiftInput[cv] = bitRead(shift_0, cv);
    } else if(cv >= 8 and cv <= 15){
      currentShiftInput[cv] = bitRead(shift_1, (cv - 8));
    } else if(cv >= 16 and cv <= 23){
      currentShiftInput[cv] = bitRead(shift_2, (cv - 16));
    } else if(cv >= 24 and cv <= 31){
      currentShiftInput[cv] = bitRead(shift_3, (cv - 24));
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

  // GMRS Radio
  if(lastShiftInput[8]){
    switchPosition[4] = ON;
  } else if(lastShiftInput[9]){
    switchPosition[4] = AUTO;
  } else {
    switchPosition[4] = OFF;
  }

  // Tunes Radio
  if(lastShiftInput[10]){
    switchPosition[5] = ON;
  } else if(lastShiftInput[11]){
    switchPosition[5] = AUTO;
  } else {
    switchPosition[5] = OFF;
  }

  // Night Signal
  if(lastShiftInput[12]){
    switchPosition[6] = ON;
  } else if(lastShiftInput[13]){
    switchPosition[6] = AUTO;
  } else {
    switchPosition[6] = OFF;
  }

  // Side Light Bars Momentary
  if(lastShiftInput[14]){
    switchPosition[7] = RIGHT;
  } else if(lastShiftInput[15]){
    switchPosition[7] = LEFT;
  } else {
    switchPosition[7] = OFF;
  }
  
  // Offroad Mode
  if(lastShiftInput[19]){
    switchPosition[8] = ON;
  } else {
    switchPosition[8] = OFF;
  }

  // Hazard Mode
  if(lastShiftInput[18]){
    switchPosition[9] = ON;
  } else {
    switchPosition[9] = OFF;
  }

  // Observatory Mode
  if(lastShiftInput[17]){
    switchPosition[10] = ON;
  } else {
    switchPosition[10] = OFF;
  }

  // All on Mode
  if(lastShiftInput[16]){
    switchPosition[11] = ON;
  } else {
    switchPosition[11] = OFF;
  }

  // Offroad Mode Light Pattern Select
  if(lastShiftInput[20]){
    switchPosition[12] = NEXT;
  } else if(lastShiftInput[21]){
    switchPosition[12] = PREVIOUS;
  } else {
    switchPosition[12] = OFF;
  }

  // Hazard Mode Direction Choice
  if(lastShiftInput[23]){
    switchPosition[13] = LEFT;
  } else if(lastShiftInput[22]){
    switchPosition[13] = RIGHT;
  } else {
    switchPosition[13] = CENTER;
  }

  // Drivers Side Map Light
  if(lastShiftInput[27]){
    switchPosition[14] = ON;
  } else {
    switchPosition[14] = OFF;
  }

  // Dome Light Mode Select
  if(lastShiftInput[25]){
    switchPosition[15] = ON;
  } else if(lastShiftInput[24]){
    switchPosition[15] = DOOR;
  } else {
    switchPosition[15] = OFF;
  }

  // Passengers Side Map Light
  if(lastShiftInput[26]){
    switchPosition[16] = ON;
  } else {
    switchPosition[16] = OFF;
  }

}

void calculations(){
  // Update the HUD base color
  if(switchPosition[6] == ON){
    hudColor = strip.Color(255, 0, 0, 0); // Set hud color to red
    HUDbrightness = 51; // Set brightness to 1/5
    strip.setBrightness(HUDbrightness);
    strip.fill(hudColor, 0, 7);
  } else if(switchPosition[6] == OFF){
    hudColor = strip.Color(0, 0, 0, 255); // Set hud color to white
    HUDbrightness = 255; // Set brightness to full
    strip.setBrightness(HUDbrightness);
    strip.fill(hudColor, 0, 7);
  } else {
    // Auto control at later date
  }
}

void updateOutputs(){
  // Create Temporary Variables
  boolean rightLightDecision = false; // Variable to evaluate turning on the right light relay
  boolean leftLightDecision = false; // Variable to evaluate turning on the left light relay
  boolean rearLightDecision = false; // Variable to evaluate turning on the Rear light relay
  boolean driversSideMapLightDecision = false; // Variable to evaluate turning on the Drivers side map light
  boolean passengersSideMapLightDecision = false; // Variable to evaluate turning on the Passengers side map light
  boolean domeLightDecision = false; // Variable to evaluate turning on the dome light
  boolean allOnDecision = false; // Variable to evaluate turning all lights on
  byte Shift_0Current = 0; // Binary enumerated value to update the 5v Output Shift Registers
  //boolean 595OutputUpdate = false; // Flag to activate 5v Output Shift Registers update code

  /*
  Shift Register Values
  Output  Value   Name
  1       128     GMRS Radio
  2       64      Tunes Radio
  3       32      Night Signal
  */
  
  // ---Human Inputs---
  
  /*
   ---Indicator List---
   Bumper Light Bar LED 0
   Main Light Bar   LED 1
   Side Light Bars  LED 2
   Rear Light Bar   LED 3
   GMRS Radio       LED 4
   Tunes Radio      LED 5
   Night Signal     LED 6
   */

  // Update current brightness
  strip.setBrightness(HUDbrightness);
  
   // ----Mode Inputs----

   // Off Road Mode
   switch (switchPosition[8]) {
     case ON:
       // Print debug message
       if (debug) {
         Serial.println("Switch 08 - ON   - Off Road Mode");
       }
       // Add Functionalality
       break;
     case OFF:
       // Print debug message
       if (debug) {
         Serial.println("Switch 08 - OFF  - Off Road Mode");
       }
       // Add Functionalality
       break;
   }
   
   // Hazard Mode
   switch (switchPosition[9]) {
     case ON:
       // Print debug message
       if (debug) {
         Serial.println("Switch 09 - ON   - Hazard Mode");
       }
       // Add Functionalality
       break;
     case OFF:
       // Print debug message
       if (debug) {
         Serial.println("Switch 09 - OFF  - Hazard Mode");
       }
       // Add Functionalality
       break;
   }
   
   // Observatory Mode
   switch (switchPosition[10]) {
     case ON:
       // Print debug message
       if (debug) {
         Serial.println("Switch 10 - ON   - Observatory Mode");
       }
       // Add Functionalality
       break;
     case OFF:
       // Print debug message
       if (debug) {
         Serial.println("Switch 10 - OFF  - Observatory Mode");
       }
       // Add Functionalality
       break;
   }
   
   // All On Mode
   switch (switchPosition[11]) {
     case ON:
       // Print debug message
       if (debug) {
         Serial.println("Switch 11 - ON   - All On Mode");
       }
       allOnDecision = true;
       break;
     case OFF:
       // Print debug message
       if (debug) {
         Serial.println("Switch 11 - OFF  - All On Mode");
       }
       allOnDecision = false;
       break;
   }
   
   // Offroad Mode Light Pattern Select
   switch (switchPosition[12]) {
     case NEXT:
       // Print debug message
       if (debug) {
         Serial.println("Switch 12 - NEXT - Offroad Mode Light Pattern Select");
       }
       // Add Functionalality
       break;
     case OFF:
       // Print debug message
       if (debug) {
         Serial.println("Switch 12 - OFF  - Offroad Mode Light Pattern Select");
       }
       // Add Functionalality
       break;
     case PREVIOUS:
       // Print debug message
       if (debug) {
         Serial.println("Switch 12 - PREV - Offroad Mode Light Pattern Select");
       }
       // Add Functionalality
       break;
   }
   
   // Hazard Mode Direction Choice
   switch (switchPosition[13]) {
     case LEFT:
       // Print debug message
       if (debug) {
         Serial.println("Switch 13 - LEFT - Hazard Mode Direction Choice");
       }
       // Add Functionalality
       break;
     case CENTER:
       // Print debug message
       if (debug) {
         Serial.println("Switch 13 - CENT - Hazard Mode Direction Choice");
       }
       // Add Functionalality
       break;
     case RIGHT:
       // Print debug message
       if (debug) {
         Serial.println("Switch 13 - RIGT - Hazard Mode Direction Choice");
       }
       // Add Functionalality
       break;
   }



   // ----Device Inputs----



  // Bumper Light Bar
  if (allOnDecision) { // If All once Mode is active, force the light on
    strip.setPixelColor(0,0,255,0,0); // Update Indicator
  } else {
    switch (switchPosition[0]) {
      case ON: 
        // Print debug message
        if (debug) {
          Serial.println("Switch 00 - ON   - Bumper Light Bar");
        }
        strip.setPixelColor(0,0,255,0,0); // Update Indicator
        break;
      case OFF:
        // Print debug message
        if (debug) {
          Serial.println("Switch 00 - OFF  - Bumper Light Bar");
        }
        strip.setPixelColor(0,hudColor); // Update Indicator
        break;
      case AUTO:
        // Print debug message
        if (debug) {
          Serial.println("Switch 00 - AUTO - Bumper Light Bar");
        }
        strip.setPixelColor(0,255,165,0,0); // Update Indicator
        break;
    }
  }
  

  // Main Light Bar
  if (allOnDecision) { // If All once Mode is active, force the light on
    strip.setPixelColor(1,0,255,0,0); // Update Indicator
    digitalWrite(RELAY_MAIN_LIGHT, LOW); // Turn Main light on
  } else {
    switch (switchPosition[1]) {
      case ON:
        // Print debug message
        if (debug) {
          Serial.println("Switch 01 - ON   - Main Light Bar");
        }
        strip.setPixelColor(1,0,255,0,0); // Update Indicator
        digitalWrite(RELAY_MAIN_LIGHT, LOW); // Turn Main light on
        break;
      case OFF:
        // Print debug message
        if (debug) {
          Serial.println("Switch 01 - OFF  - Main Light Bar");
        }
        strip.setPixelColor(1,hudColor); // Update Indicator
        digitalWrite(RELAY_MAIN_LIGHT, HIGH); // Turn Main light off
        break;
      case AUTO:
        // Print debug message
        if (debug) {
          Serial.println("Switch 01 - AUTO - Main Light Bar");
        }
        strip.setPixelColor(1,255,165,0,0); // Update Indicator
        //Decisions for Auto TBD
        break;
    }
  }
  

  // Side Light Bars
  if (allOnDecision) { // If All once Mode is active, force the light on
    strip.setPixelColor(2,0,255,0,0); // Update Indicator
    rightLightDecision = true; // Set flag to evaluate relay later
    leftLightDecision = true; // Set flag to evaluate relay later
  } else {
    switch (switchPosition[2]) {
      case ON:
        // Print debug message
        if (debug) {
          Serial.println("Switch 02 - ON   - Side Lights");
        }
        strip.setPixelColor(2,0,255,0,0); // Update Indicator
        rightLightDecision = true; // Set flag to evaluate relay later
        leftLightDecision = true; // Set flag to evaluate relay later
        break;
      case OFF:
        // Print debug message
        if (debug) {
          Serial.println("Switch 02 - OFF  - Side Lights");
        }
        strip.setPixelColor(2,hudColor); //Update Indicator
        rightLightDecision = false; // Call to turn off both lights
        leftLightDecision = false;
        break;
      case AUTO:
        // Print debug message
        if (debug) {
          Serial.println("Switch 02 - AUTO - Side Lights");
        }
        strip.setPixelColor(2,255,165,0,0); // Update Indicator
        //Decisions for Auto TBD
        break;
    }
  }
  
  
  // Rear Light Bar
  if (allOnDecision) { // If All once Mode is active, force the light on
    strip.setPixelColor(3,0,255,0,0); // Update Indicator
    digitalWrite(RELAY_REAR_LIGHTS, LOW); // Turn on rear lights
  } else {
    switch (switchPosition[3]) {
      case ON:
        // Print debug message
        if (debug) {
          Serial.println("Switch 03 - ON   - Rear Light Bar");
        }
        strip.setPixelColor(3,0,255,0,0); // Update Indicator
        digitalWrite(RELAY_REAR_LIGHTS, LOW); // Turn on rear lights
        break;
      case OFF:
        // Print debug message
        if (debug) {
          Serial.println("Switch 03 - OFF  - Rear Light Bar");
        }
        strip.setPixelColor(3,hudColor); //Update Indicator
        digitalWrite(RELAY_REAR_LIGHTS, HIGH); // Turn on rear lights
        break;
      case AUTO:
        // Print debug message
        if (debug) {
          Serial.println("Switch 03 - AUTO - Rear Light Bar");
        }
        strip.setPixelColor(3,255,165,0,0); // Update Indicator
        //Decisions for Auto TBD
        break;
    }
  }
  
  // GMRS Radio
  switch (switchPosition[4]) {
    case ON:
      // Print debug message
      if (debug) {
        Serial.println("Switch 04 - ON   - GMRS Radio");
      }
      
      strip.setPixelColor(4,0,255,0,0); // Update Indicator

      Shift_0Current = Shift_0Current + 128; // Add corresponding enumerated value to build 5v Output Shift Registers data
      break;
    case OFF:
      // Print debug message
      if (debug) {
        Serial.println("Switch 04 - OFF  - GMRS Radio");
        }
      strip.setPixelColor(4,hudColor); // Update Indicator
      break;
    case AUTO:
      // Print debug message
      if (debug) {
        Serial.println("Switch 04 - AUTO - GMRS Radio");
      }
      strip.setPixelColor(4,255,165,0,0); // Update Indicator
      break;
  }

  // Tunes Radio
  switch (switchPosition[5]) {
    case ON:
      // Print debug message
      if (debug) {
        Serial.println("Switch 05 - ON   - Tunes Radio");
      }
      
      strip.setPixelColor(5,0,255,0,0); // Update Indicator

      Shift_0Current = Shift_0Current + 64; // Add corresponding enumerated value to build 5v Output Shift Registers data
      
      break;
    case OFF:
      // Print debug message
      if (debug) {
        Serial.println("Switch 05 - OFF  - Tunes Radio");
      }
      strip.setPixelColor(5,hudColor); // Update Indicator
      break;
    case AUTO:
      // Print debug message
      if (debug) {
        Serial.println("Switch 05 - AUTO - Tunes Radio");
      }
      strip.setPixelColor(5,255,165,0,0); // Update Indicator
      break;
  }

  // Night Signal
  switch (switchPosition[6]) {
    case ON:
      // Print debug message
      if (debug) {
        Serial.println("Switch 06 - ON   - Night Signal");
      }
      
      strip.setPixelColor(6,0,255,0,0); // Update Indicator

      Shift_0Current = Shift_0Current + 32; // Add corresponding enumerated value to build 5v Output Shift Registers data
      
      break;
    case OFF:
      // Print debug message
      if (debug) {
        Serial.println("Switch 06 - OFF  - Night Signal");
      }
      strip.setPixelColor(6,hudColor); // Update Indicator
      break;
    case AUTO:
      // Print debug message
      if (debug) {
        Serial.println("Switch 06 - AUTO - Night Signal");
      }
      strip.setPixelColor(6,255,165,0,0); // Update Indicator
      break;
  }

  // Side Light Bars Momentary
  switch (switchPosition[7]) {
    case LEFT:
      // Print debug message
      if (debug) {
        Serial.println("Switch 07 - LEFT - Side Light Bars Momentary");
      }
      strip.setPixelColor(2,0,255,0,0); // Update Indicator
      leftLightDecision = true; // Call to turn left light on
      break;
    case OFF:
      // Print debug message
      if (debug) {
        Serial.println("Switch 07 - OFF  - Side Light Bars Momentary");
      }
      // Do nothing and let the original Side Light Bars switch take control again
      break;
    case RIGHT:
      // Print debug message
      if (debug) {
        Serial.println("Switch 07 - RIGT - Side Light Bars Momentary");
      }
      strip.setPixelColor(2,0,255,0,0); //Update Indicator
      rightLightDecision = true; // Call to turn right light on
      break;
  }

  // Drivers Side Map Light
  switch (switchPosition[14]) {
    case ON:
      // Print debug message
      if (debug) {
        Serial.println("Switch 14 - ON   - Drivers Side Map Light");
      }
      // Turn on map light
      driversSideMapLightDecision = true;
      break;
    case OFF:
      // Print debug message
      if (debug) {
        Serial.println("Switch 14 - OFF  - Drivers Side Map Light");
      }
      // Turn off map light
      driversSideMapLightDecision = false;
      break;
  }

  // Dome Light Mode Select
  switch (switchPosition[15]) {
    case ON:
      // Print debug message
      if (debug) {
        Serial.println("Switch 15 - ON   - Dome Light Mode Select");
      }
      // Turn on dome light
      domeLightDecision = true;
      break;
    case OFF:
      // Print debug message
      if (debug) {
        Serial.println("Switch 15 - OFF  - Dome Light Mode Select");
      }
      // Turn on dome light
      domeLightDecision = false;
      break;
    case DOOR:
      // Print debug message
      if (debug) {
        Serial.println("Switch 15 - DOOR - Dome Light Mode Select");
      }
      // Add Functionalality
      break;
  }

  // Passengers Side Map Light
  switch (switchPosition[16]) {
    case ON:
      // Print debug message
      if (debug) {
        Serial.println("Switch 16 - ON   - Passengers Side Map Light");
      }
      // Turn on map light
      passengersSideMapLightDecision = true;
      break;
    case OFF:
      // Print debug message
      if (debug) {
        Serial.println("Switch 16 - OFF  - Passengers Side Map Light");
      }
      // Turn off map light
      passengersSideMapLightDecision = false;
      break;
  }

  // Print debug message
      if (debug) {
        Serial.println("----------------------------------------");
      }

  // ---Final evaluations---

  // Evaluate right light relay (relay 1)
  if (rightLightDecision or allOnDecision) {
    digitalWrite(RELAY_RIGHT_LIGHT, LOW); // Turn right light on
  } else {
    digitalWrite(RELAY_RIGHT_LIGHT, HIGH); // Turn right light off
  }

  // Evaluate left light relay (relay 2)
  if (leftLightDecision or allOnDecision) {
    digitalWrite(RELAY_LEFT_LIGHT, LOW); // Turn left light on
  } else {
    digitalWrite(RELAY_LEFT_LIGHT, HIGH); // Turn left light off
  }

  // Evaluate Drivers side map light
  if (driversSideMapLightDecision or domeLightDecision){
    strip.fill(white, 7, 4);
  } else {
    strip.fill(off, 7, 4);
  }

  // Evaluate dome light
  if (domeLightDecision){
    strip.fill(white, 11, 16);
  } else {
    strip.fill(off, 11, 16);
  }

  // Evaluate passengers side map light
  if (passengersSideMapLightDecision or domeLightDecision){
    strip.fill(white, 27, 4);
  } else {
    strip.fill(off, 27, 4);
  }

  // Update 5v Output Shift Registers if a change has happened
  if (Shift_0Current != Shift_0Last) {
    
    // Write to outputs
    Serial.println(Shift_0Current, BIN);
    digitalWrite(LATCH_PIN, LOW); // Bring RCLK LOW to keep outputs from changing while reading serial data

    shiftOut(DATA_PIN, CLOCK_PIN, LSBFIRST, Shift_0Current); // Shift out the data

    digitalWrite(LATCH_PIN, HIGH); // Bring RCLK HIGH to change outputs

    Shift_0Last = Shift_0Current; // Store current value
  }

  InputUpdated = false; // Turn off flag to keep UpdateOutputs() from running when nothing has changed.
}
