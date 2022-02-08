/*
*****************************************************************************
*                                                                           *
*        Project: Wrexus Carduino Controls                                  *
*          Board: Main Mega                                                 *
* Devices Served: Overhead Switch Panel                                     *
*                 Ouput Board (Tunes Radio, Radio Night Signal, GMRS Radio) *
*        Version: 0.900                                                     *
*   Last Updated: 2022.02.07                                                *
*                                                                           *
*****************************************************************************
*/

// Include libraries
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <Wire.h>

// function prototypes
void readInputs();
void calculations();
void updateOutputs();
void sendI2CMessage(byte address, byte pattern, byte option=255);

// Debug Mode Select
boolean debugSwitches = false; // Change to true will allow messages to print to the serial monitor
boolean debugI2C = false; // Change to true will allow messages to print to the serial monitor
boolean i2c_active = true; // Change to false to remove I2C calls for troubleshooting

// Define I2C device constants
#define I2C_MESSAGE_RECEIVED_LED_FLASH_LENGTH 250 // The milliseconds time that the light will flash for
#define LIGHT_SLEEP_TIME_INTERVAL 900000 // The miliseconds time that lights should sleep after no activity (15 minutes)

// Define state text for readability
#define OFF       0 // Switch state definition for readability
#define CENTER    0 // Switch state definition for readability
#define CONSTRUCTION_PATTERN_CYCLE 0 // Off road mode select pattern name for readability
#define ON        1 // Switch state definition for readability
#define LEFT      1 // Switch state definition for readability
#define NEXT      1 // Switch state definition for readability
#define WHITE     1 // Color state definition for readability
#define RAINBOW_ROAD 1 // Off road mode select pattern name for readability
#define AUTO      2 // Switch state definition for readability
#define RIGHT     2 // Switch state definition for readability
#define PREVIOUS  2 // Switch state definition for readability
#define DOOR      2 // Switch state definition for readability
#define RED       2 // Color state definition for readability
#define GREEN     3 // Color state definition for readability
#define BLUE      4 // Color state definition for readability
#define ORANGE    5 // Color state definition for readability
#define YELLOW    6 // Color state definition for readability
#define PURPLE    7 // Color state definition for readability

// Define Neopixel Variables/Constants
#define PIN 31
#define NUM_LEDS 31
byte HUDbrightness = 255; // brightness range of overhead indicator lights [off..on] = [0..255]
Adafruit_NeoPixel overheadControlsStrip = Adafruit_NeoPixel(NUM_LEDS, PIN, NEO_GRBW + NEO_KHZ800);
uint32_t hudColor = overheadControlsStrip.Color(0, 0, 0, 255); // Current hud color
uint32_t white = overheadControlsStrip.Color(0,0,0,255); // Daytime indicator hud state
uint32_t red = overheadControlsStrip.Color(255,0,0,0); // Nightime indicator hud state
uint32_t green = overheadControlsStrip.Color(0,255,0,0); // On indicator state
uint32_t blue = overheadControlsStrip.Color(0,0,255,0);
uint32_t orange = overheadControlsStrip.Color(255,165,0,0); // Auto indicator state
uint32_t off = overheadControlsStrip.Color(0,0,0,0);

// Define I2C variables
unsigned long i2c_message_LED_flash_start_timer;  // start time in milliseconds for flash
int i2c_message_LED_status;               // status of LED: 1 = ON, 0 = OFF

// Define other variables
byte currentOffRoadPattern = 0; // Current off road mode, incraments with Off Road Mode Select switch
boolean offRoadPatternUpdated = false; // Tells Off Road Mode code to run again when Off Road Mode Pattern Select has been toggled
boolean hazardModeDirectionUpdated = false; // Tells Hazard Mode code to run again when Hazard Mode Direction Select has been toggled


// Build Classes
class SwitchOnOffOn {
  //Variables
    char currentState; // 0=Off/CENTER 1=ON/LEFT/NEXT 2=AUTO/RIGHT/PREVIOUS/DOOR
    char previousState; // 0=Off/CENTER 1=ON/LEFT/NEXT 2=AUTO/RIGHT/PREVIOUS/DOOR | Allows some signals to only be sent once

  public:
    //SwitchOnOffOn;

    //Methods
    void updateCurrentState(byte newState){
      currentState = newState;
    }

    char checkState(){
      return currentState;
    }

    char checkPreviousState(){
      return previousState;
    }

    void updatePreviousState(){
      previousState = currentState;
    }
};

class SwitchOnOff {
  // Variables
  char currentState; // 0=Off 1=ON/Mode Activate
  char previousState; // 0=Off 1=ON/Mode Activate | Allows some signals to only be sent once

  public:
    //SwitchOnOff;

    // Methods
    void updateCurrentState(byte newState){
      currentState = newState;
    }

    char checkState(){
      return currentState;
    }

    char checkPreviousState(){
      return previousState;
    }

    void updatePreviousState(){
      previousState = currentState;
    }
};

class InteriorLight {
  private:
    byte stripSelect; // Choose which neopixel strip this lives on | 0=overheadControlsStrip
    byte startingPixel; // first (or only) pixel in this related group
    byte numberOfPixels; // Number of pixels related to this group

  public:
    InteriorLight(byte stripSelect, byte startingPixel, byte numberOfPixels){
      this->stripSelect = stripSelect;
      this->startingPixel = startingPixel;
      this->numberOfPixels = numberOfPixels;
    }

    // Methods
    void updateBrightness(byte newBrightness){
      switch(stripSelect){
        case 0:
          overheadControlsStrip.setBrightness(newBrightness);
          break;
      }
    }

    void updateColor(uint32_t newColor){
      switch(stripSelect){
        case 0:
          overheadControlsStrip.fill(newColor, startingPixel, numberOfPixels);
          break;
      }
    }
};

class I2CDevice {
  private:
    byte address; // address of the device the messages will be sent to
    volatile boolean powerStatus = false; // Hold the Power On state of the I2C device
    unsigned long sleepTimer = 0; // Holds the last time value when the device was told to display nothing
    boolean RGBLightsActive = false; // Holds if the RGB lights are currently running a pattern or not
    boolean RGBLeftLightsActive = false; // Holds if the RGB lights are currently running a pattern or not
    boolean RGBRightLightsActive = false; // Holds if the RGB lights are currently running a pattern or not
    boolean RGBBrakeLightsActive = false; // Holds if the RGB lights are currently running a pattern or not
    boolean mainLightsActive = false; // Holds if the Main lights are currently on or not
    boolean mainLeftLightsActive = false; // Holds if the Main lights are currently on or not
    boolean mainRightLightsActive = false; // Holds if the Main lights are currently on or not

  public:
    // Constructor
    I2CDevice(byte address){
      this->address = address;
    }

    // Methods
    boolean checkPowerStatus(){
      if(powerStatus){
        return true;
      } else {
        return false;
      }
    }

    void RGBLightBarPower(int var){
      switch(var){
        case 0: // OFF
          sendI2CMessage(address,0);
          powerStatus = false;
          break;
        case 1: // ON
          sendI2CMessage(address,1);
          delay(60); // Give the light time to turn ON
          powerStatus = true;
          break;
      }
    }

    void checkLightActivity(){
      /*if(checkPowerStatus() && !RGBLightsActive && !RGBLeftLightsActive && !RGBRightLightsActive && !RGBBrakeLightsActive && !mainLightsActive && !mainLeftLightsActive && !mainRightLightsActive && sleepTimer == 0){ // If all lights have just gone inactive, set the sleep timer
        sleepTimer = millis();
      }*/

      if(RGBLightsActive || RGBLeftLightsActive || RGBRightLightsActive || RGBBrakeLightsActive || mainLightsActive || mainLeftLightsActive || mainRightLightsActive){ // If any of the lights go active, cancel the sleep timer
        sleepTimer = millis();
      }

      if(checkPowerStatus() && millis() - sleepTimer >= LIGHT_SLEEP_TIME_INTERVAL){ // If the lights have been active for the specified time, shut them off.
        RGBLightBarPower(OFF);
      }
    }

    void allOff(){
      sendI2CMessage(address,2);
      RGBLightsActive = false;
      RGBLeftLightsActive = false;
      RGBRightLightsActive = false;
      mainLightsActive = false;
      mainLeftLightsActive = false;
      mainRightLightsActive = false;
    }

    void allMainLights(int var){
      switch(var){
        case 0: // OFF
          sendI2CMessage(address,3);
          mainLightsActive = false;
          mainLeftLightsActive = false;
          mainRightLightsActive = false;
          break;
        case 1: // ON
          // Check if light has been powered on
          if(!powerStatus){
            RGBLightBarPower(ON);
          }
          sendI2CMessage(address,4);
          mainLightsActive = true;
          mainLeftLightsActive = true;
          mainRightLightsActive = true;
          break;
      }
    }

    void leftMainLights(int var){
      switch(var){
        case 0: // OFF
          sendI2CMessage(address,5);
          mainLeftLightsActive = false;
          break;
        case 1: // ON
          // Check if light has been powered on
          if(!powerStatus){
            RGBLightBarPower(ON);
          }
          sendI2CMessage(address,6);
          mainLeftLightsActive = true;
          break;
      }
    }

    void rightMainLights(int var){
      switch(var){
        case 0: // OFF
          sendI2CMessage(address,7);
          mainRightLightsActive = false;
          break;
        case 1: // ON
          // Check if light has been powered on
          if(!powerStatus){
            RGBLightBarPower(ON);
          }
          sendI2CMessage(address,8);
          mainRightLightsActive = true;
          break;
      }
    }

    void chaseLights(int var){
      switch(var){
        case 0: // OFF
          sendI2CMessage(address,9);
          break;
        case 1: // ON
          sendI2CMessage(address,10);
          break;
      }
    }

    void RGBAll(int var){
      switch(var){
        case 0: // OFF
          sendI2CMessage(address,11);
          RGBLightsActive = false;
          RGBLeftLightsActive = false;
          RGBRightLightsActive = false;
          break;
        case 2: // RED
          // Check if light has been powered on
          if(!powerStatus){
            RGBLightBarPower(ON);
          }
          sendI2CMessage(address,12);
          RGBLightsActive = true;
          RGBLeftLightsActive = true;
          RGBRightLightsActive = true;
          break;
      }
    }

    void RGBLeft(int var){
      switch(var){
        case 2: // RED
          // Check if light has been powered on
          if(!powerStatus){
            RGBLightBarPower(ON);
          }
          sendI2CMessage(address,13);
          RGBLeftLightsActive = true;
          break;
      }
    }

    void RGBRight(int var){
      switch(var){
        case 2: // RED
          // Check if light has been powered on
          if(!powerStatus){
            RGBLightBarPower(ON);
          }
          sendI2CMessage(address,14);
          RGBRightLightsActive = true;
          break;
      }
    }

    void turnSignalLeft(int var){
      switch(var){
        case 0: // OFF
          sendI2CMessage(address,15);
          RGBLeftLightsActive = false;
          break;
        case 1: // ON
          // Check if light has been powered on
          if(!powerStatus){
            RGBLightBarPower(ON);
          }
          sendI2CMessage(address,16);
          RGBLeftLightsActive = true;
          break;
      }
    }

    void turnSignalRight(int var){
      switch(var){
        case 0: // OFF
          sendI2CMessage(address,17);
          RGBRightLightsActive = false;
          break;
        case 1: // ON
          // Check if light has been powered on
          if(!powerStatus){
            RGBLightBarPower(ON);
          }
          sendI2CMessage(address,18);
          RGBRightLightsActive = true;
          break;
      }
    }

    void brake(int var){
      switch(var){
        case 0: // OFF
          sendI2CMessage(address,19);
          RGBBrakeLightsActive = false;
          break;
        case 1: // ON
          // Check if light has been powered on
          if(!powerStatus){
            RGBLightBarPower(ON);
          }
          sendI2CMessage(address,20);
          RGBBrakeLightsActive = true;
          break;
      }
    }

    void RGBCautionPatternCycle(){
      // Check if light has been powered on
      if(!powerStatus){
        RGBLightBarPower(ON);
      }
      sendI2CMessage(address,21);
      RGBLightsActive = true;
    }

    void RGBHazard(int var){
      // Check if light has been powered on
      if(!powerStatus){
        RGBLightBarPower(ON);
      }
      
      switch(var){
        case 0: // CENTER
          sendI2CMessage(address,24);
          break;
        case 1: // LEFT
          sendI2CMessage(address,22);
          break;
        case 2: // RIGHT
          sendI2CMessage(address,23);
          break;
      }

      RGBLightsActive = true;
    }

    void RGBAllSolid(int var){
      
      // Check if light has been powered on
      if(!powerStatus){
        RGBLightBarPower(ON);
      }
      
      switch(var){
        case 0: // OFF
          sendI2CMessage(address,100,0);
          RGBLightsActive = false;
          break;
        case 1: // WHITE
          sendI2CMessage(address,100,1);
          RGBLightsActive = true;
          break;
        case 2: // RED
          sendI2CMessage(address,100,2);
          RGBLightsActive = true;
          break;
        case 3: // GREEN
          sendI2CMessage(address,100,3);
          RGBLightsActive = true;
          break;
        case 4: // BLUE
          sendI2CMessage(address,100,4);
          RGBLightsActive = true;
          break;
        case 5: // ORANGE
          sendI2CMessage(address,100,5);
          RGBLightsActive = true;
          break;
        case 6: // YELLOW
          sendI2CMessage(address,100,6);
          RGBLightsActive = true;
          break;
        case 7: // PURPLE
          sendI2CMessage(address,100,7);
          RGBLightsActive = true;
          break;
      }
    }

    void RGBRainbowRoad(){
      // Check if light has been powered on
      if(!powerStatus){
        RGBLightBarPower(ON);
      }
      sendI2CMessage(address,106,81);
      RGBLightsActive = true;
    }
};

// !!! Save this in case there needs to be any 5v relay devices added !!!
/*
class RelayControledDevice { // input values (output pin number [number], activation signal [0=low signal activation, 1=high signal activation])
  private:
    byte pin;
    byte signalType;

  public:
    RelayControledDevice(byte pin, byte signalType){
      this->pin = pin;
      this->signalType = signalType;
      init();
    }

    void init(){
      pinMode(pin, OUTPUT);
      off();
    }

    void on(){
      if(signalType){
        digitalWrite(pin, HIGH);
      } else {
        digitalWrite(pin, LOW);
      }
    }

    void off(){
      if(signalType){
        digitalWrite(pin, LOW);
      } else {
        digitalWrite(pin, HIGH);
      }
    }
};
*/

// Create Objects

// Overhead Console

// Switches
SwitchOnOffOn switchBumperLightBar;
SwitchOnOffOn switchMainLightBar;
SwitchOnOffOn switchSideLightBars;
SwitchOnOffOn switchRearLightBar;
SwitchOnOffOn switchGMRSRadio;
SwitchOnOffOn switchTunesRadio;
SwitchOnOffOn switchNightSignal;
SwitchOnOffOn switchMomentarySideLightBars;
SwitchOnOff switchOffRoadMode;
SwitchOnOff switchHazardMode;
SwitchOnOff switchObservatoryMode;
SwitchOnOff switchAllOnMode;
SwitchOnOffOn switchMomentaryOffRoadModeSelect;
SwitchOnOffOn switchHazardModeSelect;
SwitchOnOff switchDriversMapLight;
SwitchOnOffOn switchDomeLightSelect;
SwitchOnOff switchPassengerMapLight;

// Lights
InteriorLight indicatorBumperLightBar(0,0,1);// Strip ID | First LED Address | # of associated LEDs
InteriorLight indicatorMainLightBar(0,1,1);
InteriorLight indicatorSideLightBars(0,2,1);
InteriorLight indicatorRearLightBar(0,3,1);
InteriorLight indicatorGMRSRadio(0,4,1);
InteriorLight indicatorTunesRadio(0,5,1);
InteriorLight indicatorNightSignal(0,6,1);
InteriorLight areaLightDriversMap(0,7,4);
InteriorLight areaLightDriversDome(0,11,8);
InteriorLight areaLightPassengerDome(0,19,8);
InteriorLight areaLightPassengerMap(0,27,4);


// !!! Save this in case there needs to be any 5v relay devices added !!!
// Relay controled items
//RelayControledDevice lightMainBar(52,0); // Relay 0: Pin number | Signal type 0=low signal activation, 1=high signal activation


// I2C Devices
I2CDevice mainLightBar(8);
I2CDevice rearLightBar(9);
I2CDevice sideLightBars(10);


// Define Connections to 74HC165N used inreadInputs()
#define LOAD_PIN      23 // PL pin 1
#define CLK_ENAB_PIN  25 // CE pin 15
#define DATA_IN_PIN   27 // Q7 pin 9
#define CLK_IN_PIN    29 // CP pin 2

// Define Shift Register Variables/Constants
#define NUM_SHIFT_INPUTS 32 // Number of individual inputs coming in from the shift registers
boolean currentShiftInput[NUM_SHIFT_INPUTS]; // Array to hold the individual values of incoming shift register values | 0-7 = shift_0, 8-15 = shift_1, 16-23 = shift_2
boolean lastShiftInput[NUM_SHIFT_INPUTS]; // Array to hold the previous values of incoming shift register values
boolean shiftInputDebounce[NUM_SHIFT_INPUTS]; // Array to hold the number of times the new value of the

// Define connections to 74HC595 - 5v Output Shift Registers
#define LATCH_PIN     35 // RCLK  pin 12
#define CLOCK_PIN     37 // SRCLK pin 11
#define DATA_PIN      33 // SER   pin 14

// Define 5v Output Shift Register variables/constants
byte Shift_0Last = 0; // Hold the last value of the shift register to determine when to run the update code

// Define Toggle Switch Variables/Constants
boolean InputUpdated = false; // Latch to skip updateOutputs()

//Define calculations variables/constants
byte sideLightDecisionPreviousState = OFF; // latch to keep the side light decision from running repeatedly


void setup() {

  // Start serial monitor if Debug is set to true
  if (debugSwitches || debugI2C ) {
    Serial.begin(115200);
  }

  // Join the I2C bus as the master
  if(i2c_active){
    Wire.begin(); // join I2C bus as the master
  }

  // Initialize Global Variable

  i2c_message_LED_flash_start_timer = millis();
  i2c_message_LED_status = 0;

  // Setup Input Shift Register connections (74HC165)
  pinMode(LOAD_PIN, OUTPUT);
  pinMode(CLK_ENAB_PIN, OUTPUT);
  pinMode(CLK_IN_PIN, OUTPUT);
  pinMode(DATA_IN_PIN, INPUT);

  // Setup 5v Output Shift Registers connections (74HC595)
  pinMode(LATCH_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(DATA_PIN, OUTPUT);

  // Setup pin 13 (and LED) as an output for indicate when messages are sent
  pinMode(13, OUTPUT);    // set pin 13 as an output

  // Send 5v Output Shift Registers their initial value
  digitalWrite(LATCH_PIN, LOW); // Bring RCLK LOW to keep outputs from changing while reading serial data
  shiftOut(DATA_PIN, CLOCK_PIN, LSBFIRST, 0); // Shift out the data
  digitalWrite(LATCH_PIN, HIGH); // Bring RCLK HIGH to change outputs

  // Fill arrays with starting values
  for(size_t cv = 0; cv < NUM_SHIFT_INPUTS; cv++){
    lastShiftInput[cv] = false;
    shiftInputDebounce[cv] = false;

  // Initialize Neopixels
  overheadControlsStrip.setBrightness(HUDbrightness);
  overheadControlsStrip.begin();
  overheadControlsStrip.show(); // Initialize all pixels to 'off'
  }

  // Fill the HUD with the initial color.
  indicatorBumperLightBar.updateColor(hudColor);
  indicatorMainLightBar.updateColor(hudColor);
  indicatorSideLightBars.updateColor(hudColor);
  indicatorRearLightBar.updateColor(hudColor);
  indicatorGMRSRadio.updateColor(hudColor);
  indicatorTunesRadio.updateColor(hudColor);
  indicatorNightSignal.updateColor(hudColor);

  delay(1000); // Give other arduinos time to startup. This solves an issue where if a switch is in a non OFF state when the project is started the lights wont update.  !!!FIX ME!!!
}


void loop() {

  readInputs();
  if(InputUpdated){
    calculations();
    updateOutputs();
  }

  // Turn off I2C Message LED once the timer has expired
  if((millis() - i2c_message_LED_flash_start_timer) > I2C_MESSAGE_RECEIVED_LED_FLASH_LENGTH){
    i2c_message_LED_status = 0; // Turn off LED
  }

  // Update I2C Message LED State
  if(i2c_active){
    digitalWrite(13, i2c_message_LED_status);
  }

  // activate all changes to LEDs
  overheadControlsStrip.show();

  // Check if light bars need to be powered off due to inactivity
  if(mainLightBar.checkPowerStatus()){
    mainLightBar.checkLightActivity();
  }

  if(rearLightBar.checkPowerStatus()){
    rearLightBar.checkLightActivity();
  }

  if(sideLightBars.checkPowerStatus()){
    sideLightBars.checkLightActivity();
  }
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

  // Update the switch objects based on the new input data

  // Bumper Bar
  if(lastShiftInput[0]){
    switchBumperLightBar.updateCurrentState(1);
  } else if(lastShiftInput[1]){
    switchBumperLightBar.updateCurrentState(2);
  } else {
    switchBumperLightBar.updateCurrentState(0);
  }

  // Main Light Bar
  if(lastShiftInput[2]){
    switchMainLightBar.updateCurrentState(1);
  } else if(lastShiftInput[3]){
    switchMainLightBar.updateCurrentState(2);
  } else {
    switchMainLightBar.updateCurrentState(0);
  }

  // Side Light Bars
  if(lastShiftInput[4]){
    switchSideLightBars.updateCurrentState(1);
  } else if(lastShiftInput[5]){
    switchSideLightBars.updateCurrentState(2);
  } else {
    switchSideLightBars.updateCurrentState(0);
  }

  // Rear Light Bar
  if(lastShiftInput[6]){
    switchRearLightBar.updateCurrentState(1);
  } else if(lastShiftInput[7]){
    switchRearLightBar.updateCurrentState(2);
  } else {
    switchRearLightBar.updateCurrentState(0);
  }

  // GMRS Radio
  if(lastShiftInput[8]){
    switchGMRSRadio.updateCurrentState(1);
  } else if(lastShiftInput[9]){
    switchGMRSRadio.updateCurrentState(2);
  } else {
    switchGMRSRadio.updateCurrentState(0);
  }

  // Tunes Radio
  if(lastShiftInput[10]){
    switchTunesRadio.updateCurrentState(1);
  } else if(lastShiftInput[11]){
    switchTunesRadio.updateCurrentState(2);
  } else {
    switchTunesRadio.updateCurrentState(0);
  }

  // Night Signal
  if(lastShiftInput[12]){
    switchNightSignal.updateCurrentState(1);
  } else if(lastShiftInput[13]){
    switchNightSignal.updateCurrentState(2);
  } else {
    switchNightSignal.updateCurrentState(0);
  }

  // Side Light Bars Momentary
  if(lastShiftInput[14]){
    switchMomentarySideLightBars.updateCurrentState(2);
  } else if(lastShiftInput[15]){
    switchMomentarySideLightBars.updateCurrentState(1);
  } else {
    switchMomentarySideLightBars.updateCurrentState(0);
  }

  // Offroad Mode
  if(lastShiftInput[19]){
    switchOffRoadMode.updateCurrentState(1);
  } else {
    switchOffRoadMode.updateCurrentState(0);
  }

  // Hazard Mode
  if(lastShiftInput[18]){
    switchHazardMode.updateCurrentState(1);
  } else {
    switchHazardMode.updateCurrentState(0);
  }

  // Observatory Mode
  if(lastShiftInput[17]){
    switchObservatoryMode.updateCurrentState(1);
  } else {
    switchObservatoryMode.updateCurrentState(0);
  }

  // All on Mode
  if(lastShiftInput[16]){
    switchAllOnMode.updateCurrentState(1);
  } else {
    switchAllOnMode.updateCurrentState(0);
  }

  // Offroad Mode Light Pattern Select
  if(lastShiftInput[20]){
    switchMomentaryOffRoadModeSelect.updateCurrentState(1);
  } else if(lastShiftInput[21]){
    switchMomentaryOffRoadModeSelect.updateCurrentState(2);
  } else {
    switchMomentaryOffRoadModeSelect.updateCurrentState(0);
  }

  // Hazard Mode Direction Choice
  if(lastShiftInput[23]){
    switchHazardModeSelect.updateCurrentState(1);
  } else if(lastShiftInput[22]){
    switchHazardModeSelect.updateCurrentState(2);
  } else {
    switchHazardModeSelect.updateCurrentState(0);
  }

  // Drivers Side Map Light
  if(lastShiftInput[27]){
    switchDriversMapLight.updateCurrentState(1);
  } else {
    switchDriversMapLight.updateCurrentState(0);
  }

  // Dome Light Mode Select
  if(lastShiftInput[25]){
    switchDomeLightSelect.updateCurrentState(1);
  } else if(lastShiftInput[24]){
    switchDomeLightSelect.updateCurrentState(2);
  } else {
    switchDomeLightSelect.updateCurrentState(0);
  }

  // Passengers Side Map Light
  if(lastShiftInput[26]){
    switchPassengerMapLight.updateCurrentState(1);
  } else {
    switchPassengerMapLight.updateCurrentState(0);
  }

}

void calculations(){
  // Update the HUD base color
  if(switchNightSignal.checkState() == ON){
    hudColor = red;
    HUDbrightness = 51; // Set brightness to 1/5

  } else if(switchNightSignal.checkState() == OFF){
    hudColor = white;
    HUDbrightness = 255; // Set brightness to full
  } else {
    // Auto control at later date
  }

  // Set overheadControlsStrip brightness
  overheadControlsStrip.setBrightness(HUDbrightness);
  // Update hud indicators
  indicatorBumperLightBar.updateColor(hudColor);
  indicatorMainLightBar.updateColor(hudColor);
  indicatorSideLightBars.updateColor(hudColor);
  indicatorRearLightBar.updateColor(hudColor);
  indicatorGMRSRadio.updateColor(hudColor);
  indicatorTunesRadio.updateColor(hudColor);
  indicatorNightSignal.updateColor(hudColor);
}

void updateOutputs(){
  // Create Temporary Variables
  boolean rightLightDecision = false; // Variable to evaluate turning on the right lights
  boolean leftLightDecision = false; // Variable to evaluate turning on the left lights
  boolean rearLightDecision = false; // Variable to evaluate turning on the Rear light
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
  overheadControlsStrip.setBrightness(HUDbrightness);

  // ----Mode Inputs----

  // Off Road Mode
  switch(switchOffRoadMode.checkState()) {
     case ON:
       // Print debug message
       if(debugSwitches) {
         Serial.println("Switch 08 - ON   - Off Road Mode");
       }

       if((switchOffRoadMode.checkPreviousState() != switchOffRoadMode.checkState()) || offRoadPatternUpdated == true){ // Only run commands if the switch just changed state
          
          switch(currentOffRoadPattern){
            case 0: // Construction pattern cycle
              mainLightBar.RGBCautionPatternCycle();
              rearLightBar.RGBCautionPatternCycle();
              sideLightBars.RGBCautionPatternCycle();
              break;
            
            case 1: // Rainbow Road
              mainLightBar.RGBRainbowRoad();
              rearLightBar.RGBRainbowRoad();
              sideLightBars.RGBRainbowRoad();
              break;
          }

          if(switchNightSignal.checkState() == OFF) { // Turn on Chase Lights if it's daytime
            delay(30); // Leave time when sending a double signal
            sideLightBars.chaseLights(ON);
          }

          offRoadPatternUpdated = false; // Clear flag telling Off Road Mode code to run again

          switchOffRoadMode.updatePreviousState(); // Update previous state to only run once as needed
       }
       break;
     case OFF:
       // Print debug message
       if(debugSwitches) {
         Serial.println("Switch 08 - OFF  - Off Road Mode");
       }

       if(switchOffRoadMode.checkPreviousState() != switchOffRoadMode.checkState()){ // Only run commands if the switch just changed state
          // Turn all RGB lights off
          mainLightBar.RGBAll(OFF);
          rearLightBar.RGBAll(OFF);
          sideLightBars.RGBAll(OFF);

          delay(30); // Leave time when sending a double signal
          sideLightBars.chaseLights(OFF);

          switchOffRoadMode.updatePreviousState(); // Update previous state to only run once as needed
       }
       break;
   }

  // Hazard Mode
  switch (switchHazardMode.checkState()) {
    case ON:
      // Print debug message
      if (debugSwitches) {
        Serial.println("Switch 09 - ON   - Hazard Mode");
      }

      // Run once code
      if (switchHazardMode.checkPreviousState() != switchHazardMode.checkState() || hazardModeDirectionUpdated == true){ // Only run commands if the switch just changed state or Hazard Mode Direction selection has changed
        
        if(hazardModeDirectionUpdated == false){ // If this pass is due to a direction selection update, skip sending these messages
          mainLightBar.RGBCautionPatternCycle();
          sideLightBars.RGBCautionPatternCycle();
        }

        switch(switchHazardModeSelect.checkState()){
          case LEFT:
            rearLightBar.RGBHazard(LEFT);
            break;

          case CENTER:
            rearLightBar.RGBHazard(CENTER);
            break;

          case RIGHT:
            rearLightBar.RGBHazard(RIGHT);
            break;
        }

        hazardModeDirectionUpdated = false;
        
        switchHazardMode.updatePreviousState(); // Update previous state to only run once as needed
      }

      // Run always code

      // None currently
      
      break;
    case OFF:
    // Print debug message
    if (debugSwitches) {
      Serial.println("Switch 09 - OFF  - Hazard Mode");
    }
  
    // Run once code
    if (switchHazardMode.checkPreviousState() != switchHazardMode.checkState()){ // Only run commands if the switch just changed state
      
      // Turn all RGB lights off
      mainLightBar.RGBAll(OFF);
      rearLightBar.RGBAll(OFF);
      sideLightBars.RGBAll(OFF);

      switchHazardMode.updatePreviousState(); // Update previous state to only run once as needed
    }

    // Run always code
    
    // None so far

    break;
  }

   // Observatory Mode
   switch (switchObservatoryMode.checkState()) {
     case ON:
       // Print debug message
       if (debugSwitches) {
         Serial.println("Switch 10 - ON   - Observatory Mode");
       }

       if (switchObservatoryMode.checkPreviousState() != switchObservatoryMode.checkState()){ // Only run commands if the switch just changed state
          
          // Add Functionalality

          switchObservatoryMode.updatePreviousState(); // Update previous state to only run once as needed
       }

       break;
     case OFF:
       // Print debug message
       if (debugSwitches) {
         Serial.println("Switch 10 - OFF  - Observatory Mode");
        }

       if (switchObservatoryMode.checkPreviousState() != switchObservatoryMode.checkState()){ // Only run commands if the switch just changed state
          
          // Add Functionalality

          switchObservatoryMode.updatePreviousState(); // Update previous state to only run once as needed
        }

       break;
    }

    // Disable all on mode until new wiring is installed and this mode is fixed.
    /*
    // All On Mode
    switch (switchAllOnMode.checkState()) {
      case ON:
        // Print debug message
        if (debugSwitches) {
          Serial.println("Switch 11 - ON   - All On Mode");
          }
        allOnDecision = true;
        break;
      case OFF:
        // Print debug message
        if (debugSwitches) {
          Serial.println("Switch 11 - OFF  - All On Mode");
          }
        allOnDecision = false;
        break;
      }
    */

   // Offroad Mode Light Pattern Select
   switch (switchMomentaryOffRoadModeSelect.checkState()) {
     case NEXT:
       // Print debug message
       if (debugSwitches) {
         Serial.println("Switch 12 - NEXT - Offroad Mode Light Pattern Select");
       }
       
       if (switchMomentaryOffRoadModeSelect.checkPreviousState() != switchMomentaryOffRoadModeSelect.checkState()){ // Only run commands if the switch just changed state
          
          currentOffRoadPattern ++; // Increment pattern

          if(currentOffRoadPattern > 1){ // Bring variable back to beginning if out of range
            currentOffRoadPattern = 0;
          }

          offRoadPatternUpdated = true; // Tell Off Road Mode code to run again

          switchMomentaryOffRoadModeSelect.updatePreviousState(); // Update previous state to only run once as needed
       }

       break;
     case OFF:
       // Print debug message
       if (debugSwitches) {
         Serial.println("Switch 12 - OFF  - Offroad Mode Light Pattern Select");
       }
       
       if (switchMomentaryOffRoadModeSelect.checkPreviousState() != switchMomentaryOffRoadModeSelect.checkState()){ // Only run commands if the switch just changed state
          
          // Add Functionalality

          switchMomentaryOffRoadModeSelect.updatePreviousState(); // Update previous state to only run once as needed
       }

       break;
     case PREVIOUS:
       // Print debug message
       if (debugSwitches) {
         Serial.println("Switch 12 - PREV - Offroad Mode Light Pattern Select");
       }
       
       if (switchMomentaryOffRoadModeSelect.checkPreviousState() != switchMomentaryOffRoadModeSelect.checkState()){ // Only run commands if the switch just changed state

          if(currentOffRoadPattern == 0){ // Bring variable back to beginning if out of range
            currentOffRoadPattern = 1; 
          } else {
            currentOffRoadPattern --; // Decrement pattern
          }

          offRoadPatternUpdated = true; // Tell Off Road Mode code to run again

          switchMomentaryOffRoadModeSelect.updatePreviousState(); // Update previous state to only run once as needed
       }

       break;
   }

   // Hazard Mode Direction Choice
   switch (switchHazardModeSelect.checkState()) {
     case LEFT:
       // Print debug message
       if (debugSwitches) {
         Serial.println("Switch 13 - LEFT - Hazard Mode Direction Choice");
       }
       
       if (switchHazardModeSelect.checkPreviousState() != switchHazardModeSelect.checkState()){ // Only run commands if the switch just changed state
          
          if(switchHazardMode.checkState() == ON){
            hazardModeDirectionUpdated = true;
          }
          
          switchHazardModeSelect.updatePreviousState(); // Update previous state to only run once as needed
        }

       break;
     case CENTER:
       // Print debug message
       if (debugSwitches) {
         Serial.println("Switch 13 - CENT - Hazard Mode Direction Choice");
       }
       
       if (switchHazardModeSelect.checkPreviousState() != switchHazardModeSelect.checkState()){ // Only run commands if the switch just changed state
          
          if(switchHazardMode.checkState() == ON){
            hazardModeDirectionUpdated = true;
          }

          switchHazardModeSelect.updatePreviousState(); // Update previous state to only run once as needed
        }

       break;
     case RIGHT:
       // Print debug message
       if (debugSwitches) {
         Serial.println("Switch 13 - RIGT - Hazard Mode Direction Choice");
       }
       
       if (switchHazardModeSelect.checkPreviousState() != switchHazardModeSelect.checkState()){ // Only run commands if the switch just changed state
          
          if(switchHazardMode.checkState() == ON){
            hazardModeDirectionUpdated = true;
          }

          switchHazardModeSelect.updatePreviousState(); // Update previous state to only run once as needed
        }

       break;
    }


   // ----Device Inputs----


  // Bumper Light Bar
  if(allOnDecision){ // Force light on if All On mode is active
    indicatorBumperLightBar.updateColor(green);
  } else {
    switch (switchBumperLightBar.checkState()) {
      case ON:
        // Print debug message
        if (debugSwitches) {
          Serial.println("Switch 00 - ON   - Bumper Light Bar");
        }

        // Run once commands
        if (switchBumperLightBar.checkPreviousState() != switchBumperLightBar.checkState()){ // Only run commands if the switch just changed state

          switchBumperLightBar.updatePreviousState(); // Update previous state to only run once as needed
        }

        // Run always commands
        indicatorBumperLightBar.updateColor(green); // Update hud indicator

        break;
      case OFF:
        // Print debug message
        if (debugSwitches) {
          Serial.println("Switch 00 - OFF  - Bumper Light Bar");
        }

        if (switchBumperLightBar.checkPreviousState() != switchBumperLightBar.checkState()){ // Only run commands if the switch just changed state
          
          indicatorBumperLightBar.updateColor(hudColor); // Update hud indicator

          switchBumperLightBar.updatePreviousState(); // Update previous state to only run once as needed
        }
        
        break;
      case AUTO:
        // Print debug message
        if (debugSwitches) {
          Serial.println("Switch 00 - AUTO - Bumper Light Bar");
        }

        if (switchBumperLightBar.checkPreviousState() != switchBumperLightBar.checkState()){ // Only run commands if the switch just changed state
          
          indicatorBumperLightBar.updateColor(orange); // Update hud indicator

          switchBumperLightBar.updatePreviousState(); // Update previous state to only run once as needed
        }
        
        break;
    }
  }

  // Main Light Bar
  if(allOnDecision){ // Force light on if All On mode is active
    indicatorMainLightBar.updateColor(green);
    mainLightBar.allMainLights(ON);
  } else {
    switch (switchMainLightBar.checkState()) {
      case ON:
        // Print debug message
        if (debugSwitches) {
          Serial.println("Switch 01 - ON   - Main Light Bar");
        }

        // Run once commands
        if (switchMainLightBar.checkPreviousState() != switchMainLightBar.checkState()){ // Only run commands if the switch just changed state
          
          mainLightBar.allMainLights(ON);

          switchMainLightBar.updatePreviousState(); // Update previous state to only run once as needed
        }

        // Run always commands
        indicatorMainLightBar.updateColor(green); // Update Hud indicator
        
        break;
      case OFF:
        // Print debug message
        if (debugSwitches) {
          Serial.println("Switch 01 - OFF  - Main Light Bar");
        }

        // Run once commands
        if (switchMainLightBar.checkPreviousState() != switchMainLightBar.checkState()){ // Only run commands if the switch just changed state
          
          mainLightBar.allMainLights(OFF);

          switchMainLightBar.updatePreviousState(); // Update previous state to only run once as needed
        }

        // Run allways commands
        indicatorMainLightBar.updateColor(hudColor); // Update Hud indicator
        
        break;
      case AUTO:
        // Print debug message
        if (debugSwitches) {
          Serial.println("Switch 01 - AUTO - Main Light Bar");
        }

        // Run once commands
        if (switchMainLightBar.checkPreviousState() != switchMainLightBar.checkState()){ // Only run commands if the switch just changed state
          
          //Decisions for Auto TBD

          switchMainLightBar.updatePreviousState(); // Update previous state to only run once as needed
        }

        // Run always commands
        indicatorMainLightBar.updateColor(orange); // Update Hud indicator
        
        break;
    }
  }

  // Side Light Bars
  if(allOnDecision){ // Force light on if All On mode is active
    indicatorSideLightBars.updateColor(green);
    rightLightDecision = true; // Create and store a value to only fire relays once in Momentary Section
    leftLightDecision = true; // Call to turn on both lights
  } else {
    switch (switchSideLightBars.checkState()) {
      case ON:
        // Print debug message
        if (debugSwitches) {
          Serial.println("Switch 02 - ON   - Side Lights");
        }
        indicatorSideLightBars.updateColor(green);
        rightLightDecision = true; // Create and store a value to only fire relays once in Momentary Section
        leftLightDecision = true; // Call to turn on both lights
        break;
      case OFF:
        // Print debug message
        if (debugSwitches) {
          Serial.println("Switch 02 - OFF  - Side Lights");
        }
        indicatorSideLightBars.updateColor(hudColor);
        rightLightDecision = false; // Call to turn off both lights
        leftLightDecision = false;
        break;
      case AUTO:
        // Print debug message
        if (debugSwitches) {
          Serial.println("Switch 02 - AUTO - Side Lights");
        }
        indicatorSideLightBars.updateColor(orange);
        //Decisions for Auto TBD
        break;
    }
  }

  // Rear Light Bar
  if(allOnDecision){ // Force light on if All On mode is active
    indicatorRearLightBar.updateColor(green);
    rearLightBar.allMainLights(ON);
  } else {
    switch (switchRearLightBar.checkState()) {
      case ON:
        // Print debug message
        if (debugSwitches) {
          Serial.println("Switch 03 - ON   - Rear Light Bar");
        }

        // Run once commands
        if (switchRearLightBar.checkPreviousState() != switchRearLightBar.checkState()){ // Only run commands if the switch just changed state
          
          rearLightBar.allMainLights(ON);

          switchRearLightBar.updatePreviousState(); // Update previous state to only run once as needed
        }

        // Run always commands
        indicatorRearLightBar.updateColor(green); // Update Hud indicator
        
        break;
      case OFF:
        // Print debug message
        if (debugSwitches) {
          Serial.println("Switch 03 - OFF  - Rear Light Bar");
        }

        // Run once commands
        if (switchRearLightBar.checkPreviousState() != switchRearLightBar.checkState()){ // Only run commands if the switch just changed state
          
          rearLightBar.allMainLights(OFF);

          switchRearLightBar.updatePreviousState(); // Update previous state to only run once as needed
        }

        // Run always commands
        indicatorRearLightBar.updateColor(hudColor); // Update Hud indicator

        break;
      case AUTO:
        // Print debug message
        if (debugSwitches) {
          Serial.println("Switch 03 - AUTO - Rear Light Bar");
        }

        // Run once commands
        if (switchRearLightBar.checkPreviousState() != switchRearLightBar.checkState()){ // Only run commands if the switch just changed state
          
          //Decisions for Auto TBD

          switchRearLightBar.updatePreviousState(); // Update previous state to only run once as needed
        }

        // Run always commands
        indicatorRearLightBar.updateColor(orange); // Update Hud indicator

        break;
    }
  }

  // GMRS Radio
  switch (switchGMRSRadio.checkState()) {
    case ON:
      // Print debug message
      if (debugSwitches) {
        Serial.println("Switch 04 - ON   - GMRS Radio");
      }

      // Run once Commands
      if (switchGMRSRadio.checkPreviousState() != switchGMRSRadio.checkState()){ // Only run commands if the switch just changed state
        
        switchGMRSRadio.updatePreviousState(); // Update previous state to only run once as needed
      }

      // Run always Commands
      indicatorGMRSRadio.updateColor(green); // Update Hud indicator
      Shift_0Current = Shift_0Current + 128; // Add corresponding enumerated value to build 5v Output Shift Registers data

      break;
    case OFF:
      // Print debug message
      if (debugSwitches) {
        Serial.println("Switch 04 - OFF  - GMRS Radio");
      }

      // Run once commands
      if (switchGMRSRadio.checkPreviousState() != switchGMRSRadio.checkState()){ // Only run commands if the switch just changed state

        indicatorGMRSRadio.updateColor(hudColor); // Update Hud indicator

        switchGMRSRadio.updatePreviousState(); // Update previous state to only run once as needed
      }

      // Run always commands
      
      // None currently

      break;
    case AUTO:
      // Print debug message
      if (debugSwitches) {
        Serial.println("Switch 04 - AUTO - GMRS Radio");
      }

      // Run once commands
      if (switchGMRSRadio.checkPreviousState() != switchGMRSRadio.checkState()){ // Only run commands if the switch just changed state

        indicatorGMRSRadio.updateColor(orange); // Update Hud indicator

        switchGMRSRadio.updatePreviousState(); // Update previous state to only run once as needed
      }

      // Run always commands
      
      // None currently
      
      break;
  }

  // Tunes Radio
  switch (switchTunesRadio.checkState()) {
    case ON:
      // Print debug message
      if (debugSwitches) {
        Serial.println("Switch 05 - ON   - Tunes Radio");
      }

      // Run once commands
      if (switchTunesRadio.checkPreviousState() != switchTunesRadio.checkState()){ // Only run commands if the switch just changed state

        switchTunesRadio.updatePreviousState(); // Update previous state to only run once as needed
      }

      // Run always commands
      indicatorTunesRadio.updateColor(green); // Update Hud indicator
      Shift_0Current = Shift_0Current + 64; // Add corresponding enumerated value to build 5v Output Shift Registers data

      break;
    case OFF:
      // Print debug message
      if (debugSwitches) {
        Serial.println("Switch 05 - OFF  - Tunes Radio");
      }

      // Run once commands
      if (switchTunesRadio.checkPreviousState() != switchTunesRadio.checkState()){ // Only run commands if the switch just changed state

        switchTunesRadio.updatePreviousState(); // Update previous state to only run once as needed
      }

      // Run always commands
      indicatorTunesRadio.updateColor(hudColor); // Update Hud indicator

      break;
    case AUTO:
      // Print debug message
      if (debugSwitches) {
        Serial.println("Switch 05 - AUTO - Tunes Radio");
      }

      // Run once commands
      if (switchTunesRadio.checkPreviousState() != switchTunesRadio.checkState()){ // Only run commands if the switch just changed state

        switchTunesRadio.updatePreviousState(); // Update previous state to only run once as needed
      }

      // Run always commands
      indicatorTunesRadio.updateColor(orange); // Update Hud indicator
      
      break;
  }

  // Night Signal
  switch (switchNightSignal.checkState()) {
    case ON:
      // Print debug message
      if (debugSwitches) {
        Serial.println("Switch 06 - ON   - Night Signal");
      }

      // Run once commands
      if (switchNightSignal.checkPreviousState() != switchNightSignal.checkState()){ // Only run commands if the switch just changed state

        sideLightBars.chaseLights(OFF); // Make sure chase lights are turned off if night signal has been turned ON

        switchNightSignal.updatePreviousState(); // Update previous state to only run once as needed
      }

      // Run always commands
      indicatorNightSignal.updateColor(green); // Update Hud indicator
      Shift_0Current = Shift_0Current + 32; // Add corresponding enumerated value to build 5v Output Shift Registers data

      break;
    case OFF:
      // Print debug message
      if (debugSwitches) {
        Serial.println("Switch 06 - OFF  - Night Signal");
      }

      // Run once commands
      if (switchNightSignal.checkPreviousState() != switchNightSignal.checkState()){ // Only run commands if the switch just changed state

        if (switchOffRoadMode.checkState() == ON){
          sideLightBars.chaseLights(ON); // Turn Chaselights on if we are just turning the night signal OFF
        }

        switchNightSignal.updatePreviousState(); // Update previous state to only run once as needed
      }

      // Run always commands
      indicatorNightSignal.updateColor(hudColor); // Update Hud indicator

      break;
    case AUTO:
      // Print debug message
      if (debugSwitches) {
        Serial.println("Switch 06 - AUTO - Night Signal");
      }

      // Run once commands
      if (switchNightSignal.checkPreviousState() != switchNightSignal.checkState()){ // Only run commands if the switch just changed state

        switchNightSignal.updatePreviousState(); // Update previous state to only run once as needed
      }

      // Run always commands
      indicatorNightSignal.updateColor(orange); // Update Hud indicator

      break;
  }

  // Side Light Bars Momentary
  switch (switchMomentarySideLightBars.checkState()) {
    case LEFT:
      // Print debug message
      if (debugSwitches) {
        Serial.println("Switch 07 - LEFT - Side Light Bars Momentary");
      }
      indicatorSideLightBars.updateColor(green);
      leftLightDecision = true; // Call to turn left light on
      break;
    case OFF:
      // Print debug message
      if (debugSwitches) {
        Serial.println("Switch 07 - OFF  - Side Light Bars Momentary");
      }
      // Do nothing and let the original Side Light Bars switch take control again
      break;
    case RIGHT:
      // Print debug message
      if (debugSwitches) {
        Serial.println("Switch 07 - RIGT - Side Light Bars Momentary");
      }
      indicatorSideLightBars.updateColor(green);
      rightLightDecision = true; // Call to turn right light on
      break;
  }

  // Drivers Side Map Light
  switch (switchDriversMapLight.checkState()) {
    case ON:
      // Print debug message
      if (debugSwitches) {
        Serial.println("Switch 14 - ON   - Drivers Side Map Light");
      }
      // Turn on map light
      driversSideMapLightDecision = true;
      break;
    case OFF:
      // Print debug message
      if (debugSwitches) {
        Serial.println("Switch 14 - OFF  - Drivers Side Map Light");
      }
      // Turn off map light
      driversSideMapLightDecision = false;
      break;
  }

  // Dome Light Mode Select
  switch (switchDomeLightSelect.checkState()) {
    case ON:
      // Print debug message
      if (debugSwitches) {
        Serial.println("Switch 15 - ON   - Dome Light Mode Select");
      }
      // Turn on dome light
      domeLightDecision = true;
      break;
    case OFF:
      // Print debug message
      if (debugSwitches) {
        Serial.println("Switch 15 - OFF  - Dome Light Mode Select");
      }
      // Turn on dome light
      domeLightDecision = false;
      break;
    case DOOR:
      // Print debug message
      if (debugSwitches) {
        Serial.println("Switch 15 - DOOR - Dome Light Mode Select");
      }
      // Add Functionalality
      break;
  }

  // Passengers Side Map Light
  switch (switchPassengerMapLight.checkState()) {
    case ON:
      // Print debug message
      if (debugSwitches) {
        Serial.println("Switch 16 - ON   - Passengers Side Map Light");
      }
      // Turn on map light
      passengersSideMapLightDecision = true;
      break;
    case OFF:
      // Print debug message
      if (debugSwitches) {
        Serial.println("Switch 16 - OFF  - Passengers Side Map Light");
      }
      // Turn off map light
      passengersSideMapLightDecision = false;
      break;
  }

  // Print debug message
      if (debugSwitches) {
        Serial.println("----------------------------------------");
      }

  // ---Final evaluations---

  // Evaluate if side lights need to be on
  if (leftLightDecision && rightLightDecision) {
    if (sideLightDecisionPreviousState != LEFT + RIGHT){
      sideLightBars.allMainLights(ON);

      sideLightDecisionPreviousState = LEFT + RIGHT;
    }
    
  } else if (leftLightDecision && !rightLightDecision){
    if (sideLightDecisionPreviousState != LEFT){
      sideLightBars.leftMainLights(ON);

      sideLightDecisionPreviousState = LEFT;
    }
    
  } else if (!leftLightDecision && rightLightDecision){
    if (sideLightDecisionPreviousState != RIGHT){
      sideLightBars.rightMainLights(ON);

      sideLightDecisionPreviousState = RIGHT;
    }
    
  } else {
    if (sideLightDecisionPreviousState != OFF){
      sideLightBars.allMainLights(OFF);

      sideLightDecisionPreviousState = OFF;
    }
    
  }

  // Evaluate Drivers side map light
  if (driversSideMapLightDecision or domeLightDecision){
    areaLightDriversMap.updateColor(white);
  } else {
    areaLightDriversMap.updateColor(off);
  }

  // Evaluate dome light
  if (domeLightDecision){
    areaLightDriversDome.updateColor(white);
    areaLightPassengerDome.updateColor(white);
  } else {
    areaLightDriversDome.updateColor(off);
    areaLightPassengerDome.updateColor(off);
  }

  // Evaluate passengers side map light
  if (passengersSideMapLightDecision or domeLightDecision){
    areaLightPassengerMap.updateColor(white);
  } else {
    areaLightPassengerMap.updateColor(off);
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

void sendI2CMessage(byte address, byte pattern, byte option){
  if(i2c_active){
    if (debugI2C) {
      Serial.print("Sending I2C Message to ");
      Serial.print(address);
      Serial.print(":");
      Serial.print(pattern);
      Serial.print(",");
      Serial.println(option);
    }

    // Add small delay to solve race condition - !!! WORK TO FIX THIS SOME OTHER WAY !!!
    //delay(10);

    // Transmit Message
    Wire.beginTransmission(address);
    Wire.write(pattern);
    if (option != 255){
      Wire.write(option);
    }
    Wire.endTransmission();

    if (debugI2C) {
      Serial.println("I2C Message Sent");
    }

    // Begin flash of message LED
    i2c_message_LED_status = 1;
    i2c_message_LED_flash_start_timer = millis();
  }else{
    if(debugI2C){
      Serial.println("I2C Currently Disabled.  No message sent");
    }
  }
}
