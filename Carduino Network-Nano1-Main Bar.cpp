/*
********************************************
*                                          *
*        Project: Wrexus Carduino Controls *
*          Board: Nano 1                   *
* Devices Served: Main Light Bar           *
*        Version: 0.900                    *
*   Last Updated: 2022.02.07               *
*                                          *
********************************************
*/

// Include libraries
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <Wire.h>

//Function prototypes
void dataRcv(int numBytes);
void updateOutputs();

// Definitions
#define MAIN_LIGHT_DATA_PIN 12
#define MAIN_LIGHT_RELAY_PIN 11 // Relay #8
#define MAIN_LIGHT_NUM_LEDS 83
#define MAIN_LIGHT_INITIAL_BRIGHNESS 255
#define PATTERN_CYCLE_TIME 5000
#define FLASH_ON_TIME_SETTING 300
#define FLASH_OFF_TIME_SETTING 20
#define I2C_MESSAGE_RECEIVED_LED_FLASH_LENGTH 250 // The milliseconds time that the light will flash for

// I2C Variables
int i2c_pattern;            // data received from I2C bus
int i2c_option;             // last data received from I2C bus
unsigned long i2c_message_LED_flash_start_timer;  // start time in milliseconds for flash
int i2c_message_LED_status;               // status of LED: 1 = ON, 0 = OFF

// Currenly running pattern variables
int currentPattern;
int currentOption;

// Pattern Variables
//unsigned long lastPatternChange = millis();
//byte currentLightPattern = 2;
//byte randomPatternFlair = random(1,3);

// Setup Strip
Adafruit_NeoPixel mainLightStrip = Adafruit_NeoPixel(MAIN_LIGHT_NUM_LEDS, MAIN_LIGHT_DATA_PIN, NEO_GRB + NEO_KHZ800);

// Define named colors
//uint32_t hudColor = strip.Color(0, 0, 0, 255); // Current hud color
uint32_t white = mainLightStrip.Color(255,255,255); // Daytime indicator hud state
uint32_t red = mainLightStrip.Color(255,0,0); // Nightime indicator hud state
uint32_t green = mainLightStrip.Color(0,255,0); // On indicator state
uint32_t blue = mainLightStrip.Color(0,0,255);
uint32_t orange = mainLightStrip.Color(255,80,0); // Auto indicator state
uint32_t yellow = mainLightStrip.Color(255,180,10);
uint32_t purple = mainLightStrip.Color(128,0,128);
uint32_t off = mainLightStrip.Color(0,0,0);

// Build Classes
class RGBLightBar {
  private:

    // Strip that will be associated with this Light Bar
    Adafruit_NeoPixel neopixelStrip;
    // Variables
    byte lastLedAddress;
    byte currentCautionPatternCall = random(2,7);
    byte randomPatternFlair = random(1,3);
    byte currentPatternLocal;
    byte currentOverallState;
    byte currentAdditionalStateOne;
    byte numOfPatternCycles;
    unsigned long lastUpdateTime;
    int flashOnTime;
    byte flashCount;
    long firstPixelHue;
    boolean updateNeeded = false;

    // Light segment calculations
    byte lowerRightCorner(){
      return 2;
    }

    byte upperLeftCorner(){
      byte toReturn = (lastLedAddress / 2) + 2;
      return toReturn;
    }

    byte width(){
      byte toReturn = lastLedAddress - upperLeftCorner();
      return toReturn;
    }

    byte lowerMidPoint(){
      byte toReturn = (width() / 2) + 2;
      return toReturn;
    }

    byte upperMidPoint(){
      byte toReturn = upperLeftCorner() + (width() / 2);
      return toReturn;
    }

    byte halfWidth(){
      byte toReturn;

      if(lastLedAddress == 12){
        toReturn = (width() / 2);
      } else {
        toReturn = (width() / 2) + 1;
      }

      return toReturn;
    }

    byte leftWrap(){
      byte toReturn;

      if(lastLedAddress == 12){
        toReturn = upperMidPoint() - lowerMidPoint();
      } else {
        toReturn = upperMidPoint() - lowerMidPoint() + 1;
      }

      return toReturn;
    }

    byte rightWraps(){
      byte toReturn;

      if(lastLedAddress == 12){
        toReturn = (upperMidPoint() - lowerMidPoint()) / 2;
      } else {
        toReturn = (upperMidPoint() - lowerMidPoint()) / 2 + 1;
      }

      return toReturn;
    }

  public:

    // Constructor
    RGBLightBar(byte lastLedAddress, Adafruit_NeoPixel &neopixelStrip){
      this->lastLedAddress = lastLedAddress;
      this->neopixelStrip = neopixelStrip;
    }

    // Methods
    void mainLightOn(){
      neopixelStrip.setPixelColor(0,red);

      updateNeeded = true; // Activate update flag
    }

    void mainLightOff(){
      neopixelStrip.setPixelColor(0,green);

      updateNeeded = true; // Activate update flag
    }

    void solidColor(uint32_t color){
      neopixelStrip.fill(color,1,lastLedAddress);

      updateNeeded = true; // Activate update flag
    }

    void cautionPatternCycle(){
      
      if(currentCautionPatternCall == 8){
        currentCautionPatternCall = 2;
      } else if(currentCautionPatternCall > 8){
        currentCautionPatternCall = 3;
      }

      switch(currentCautionPatternCall){
        case 2:
          if(flashFull(randomPatternFlair,yellow,white,yellow,10)){
            currentCautionPatternCall = currentCautionPatternCall + random(1,3);
            randomPatternFlair = random(1,4);
          }

          break;

        case 3:
          if(upAndDown(randomPatternFlair,yellow,white,yellow,10)){
            currentCautionPatternCall = currentCautionPatternCall + random(1,3);
            randomPatternFlair = random(1,4);
          }

          break;

        case 4:
          if(leftAndRight(randomPatternFlair,yellow,white,yellow,10)){
            currentCautionPatternCall = currentCautionPatternCall + random(1,3);
            randomPatternFlair = random(1,4);
          }

          break;

        case 5:
          if(crissCross(randomPatternFlair,yellow,white,yellow,10)){
            currentCautionPatternCall = currentCautionPatternCall + random(1,3);
            randomPatternFlair = random(1,4);
          }

          break;

        case 6:
          if(everyOther(randomPatternFlair,yellow,white,yellow,10)){
            currentCautionPatternCall = currentCautionPatternCall + random(1,3);
            randomPatternFlair = random(1,4);
          }

          break;

        case 7:
          if(aroundTheWorld(randomPatternFlair,yellow,white,yellow,7)){
            currentCautionPatternCall = currentCautionPatternCall + random(1,3);
            randomPatternFlair = random(1,4);
          }

          break;
      }
    }

    // Rainbow cycle along whole strip. Pass speed 0 = Full Speed | 1 = Fast | 2 = Moderate | 3 = Slow
    void rainbow(int wait) {

      // Check if this pattern has just been activated - Pattern #4
      if(currentPatternLocal != 1){
        currentPatternLocal = 1; // Set to new current patern
        firstPixelHue = 0; // Reset state to initial value
        lastUpdateTime = millis() + wait * 10; // Set lastUpdateTime so code will run first time
      }

      if(millis() > lastUpdateTime + wait * 10){
        // Reset pixel hue if we have gone over the amount
        if(firstPixelHue > 5*65536){
          firstPixelHue = 0;
        }

        // Fill light with the current ranbow patern
        for(int i=1; i<neopixelStrip.numPixels(); i++) { // For each pixel in strip...
          // Offset pixel hue by an amount to make one full revolution of the
          // color wheel (range of 65536) along the length of the strip
          // (strip.numPixels() steps):
          int pixelHue = firstPixelHue + (i * 65536L / neopixelStrip.numPixels());
          // neopixelStrip.ColorHSV() can take 1 or 3 arguments: a hue (0 to 65535) or
          // optionally add saturation and value (brightness) (each 0 to 255).
          // Here we're using just the single-argument hue variant. The result
          // is passed through neopixelStrip.gamma32() to provide 'truer' colors
          // before assigning to each pixel:
          neopixelStrip.setPixelColor(i, neopixelStrip.gamma32(neopixelStrip.ColorHSV(pixelHue)));
        }

        // Incrament Hue value
        firstPixelHue += 256;

        // Set last update time
        lastUpdateTime = millis();

        updateNeeded = true; // Activate update flag
      }
    }

    // Flash pattern - whole strip - 2
    boolean flashFull(byte multiFlash, uint32_t colorOne, uint32_t colorTwo = off, uint32_t colorThree = off, byte patternCyclesToRun = 1){

      // Caculate actual flash time
      if(multiFlash > 1){
        flashOnTime = (FLASH_ON_TIME_SETTING / multiFlash) - (FLASH_OFF_TIME_SETTING * multiFlash);
      } else {
        flashOnTime = FLASH_ON_TIME_SETTING;
      }


      // Check if this pattern has just been activated - Pattern #2
      if(currentPatternLocal != 2){
        currentPatternLocal = 2; // Set to new current patern
        currentOverallState = 0; // Reset state to initial value
        numOfPatternCycles = 0; // Reset number of pattern cycles
      }

      switch(currentOverallState){
        case 0: // Reset to initial ON state
          neopixelStrip.fill(colorOne,1,lastLedAddress); // Set the color
          lastUpdateTime = millis(); // Mark the update time
          currentOverallState = 1; // Update State
          flashCount = 0; //update flash count
          updateNeeded = true; // Activate update flag
          break;

        case 1: // Change to OFF state
          if(millis() >= lastUpdateTime + flashOnTime){
            neopixelStrip.fill(off,1,lastLedAddress); // Turn off strip
            lastUpdateTime = millis(); // Mark the update time
            currentOverallState = 2; // Update State
            flashCount ++; // Incrament flash count
            updateNeeded = true; // Activate update flag
          }
          break;

        case 2: // Loop back to ON state
          if(flashCount == multiFlash){
            // Wait for full time if we have flashed the proper amount of times.
            if(millis() >= lastUpdateTime + FLASH_ON_TIME_SETTING){
              neopixelStrip.fill(colorOne,1,lastLedAddress); // Set the color
              lastUpdateTime = millis(); // Mark the update time
              currentOverallState = 1; // Update State
              flashCount = 0; //update flash count
              numOfPatternCycles ++; // Incrament pattern cycle count
              updateNeeded = true; // Activate update flag
            }
          } else {
            if(millis() >= lastUpdateTime + FLASH_OFF_TIME_SETTING){
              if(flashCount == 1 and colorTwo != off){
                neopixelStrip.fill(colorTwo,1,lastLedAddress); // Set the color
              } else if(flashCount == 2 and colorThree != off){
                neopixelStrip.fill(colorThree,1,lastLedAddress); // Set the color
              } else {
                neopixelStrip.fill(colorOne,1,lastLedAddress); // Set the color
              }
              lastUpdateTime = millis(); // Mark the update time
              currentOverallState = 1; // Update State
              updateNeeded = true; // Activate update flag
            }
          }
          break;
      }

      if(numOfPatternCycles == patternCyclesToRun){
        return true; // Return True when we have run the requested amount of pattern cycles
      } else {
        return false;
      }
    }

    // Flash pattern - top strip then bottom strip - 3
    boolean upAndDown(byte multiFlash, uint32_t colorOne, uint32_t colorTwo = off, uint32_t colorThree = off, byte patternCyclesToRun = 1){

      // Caculate actual flash time
      if(multiFlash > 1){
        flashOnTime = (FLASH_ON_TIME_SETTING / multiFlash) - (FLASH_OFF_TIME_SETTING * multiFlash);
      } else {
        flashOnTime = FLASH_ON_TIME_SETTING;
      }

      // Check if this pattern has just been activated - Pattern #3
      if(currentPatternLocal != 3){
        currentPatternLocal = 3; // Set to new current patern
        currentOverallState = 0; // Reset state to initial value
        numOfPatternCycles = 0; // Reset number of pattern cycles
      }

      switch(currentOverallState){

        case 0: // Reset to initial UP ON state
          neopixelStrip.fill(off,1,lastLedAddress); // clear all values
          neopixelStrip.fill(colorOne,upperLeftCorner(), width()) ; // turn on upper light
          lastUpdateTime = millis(); // Mark the update time
          currentOverallState = 1; // Update State
          flashCount = 0; //update flash count
          updateNeeded = true; // Activate update flag
          break;

        case 1: // Change to either UP OFF or DOWN ON state
          if(millis() > lastUpdateTime + flashOnTime){
            flashCount ++; // Incrament flash count
            if(multiFlash > 1 and multiFlash != flashCount){
              neopixelStrip.fill(off,upperLeftCorner(),width()); // turn off upper light
              currentOverallState = 2; // Update State
            } else {
              neopixelStrip.fill(colorOne,lowerRightCorner(),width()); // Set lowwer color
              neopixelStrip.fill(off,upperLeftCorner(),width()); // Set Upper color
              currentOverallState = 3; // Update State
              flashCount = 0; // reset flash counter
            }
            lastUpdateTime = millis(); // Mark the update time
            updateNeeded = true; // Activate update flag
          }
          break;

        case 2: // Change to UP ON state
          if(millis() > lastUpdateTime + FLASH_OFF_TIME_SETTING){
            if(flashCount == 1 and colorTwo != off){
              neopixelStrip.fill(colorTwo,upperLeftCorner(),width()); // Set the color
            } else if(flashCount == 2 and colorThree != off){
              neopixelStrip.fill(colorThree,upperLeftCorner(),width()); // Set the color
            } else {
              neopixelStrip.fill(colorOne,upperLeftCorner(),width()); // Set the color
            }
            currentOverallState = 1; // Update State
            lastUpdateTime = millis(); // Mark the update time
            updateNeeded = true; // Activate update flag
          }
          break;

        case 3: // Change to either DOWN OFF or UP ON state
          if(millis() > lastUpdateTime + flashOnTime){
            flashCount ++; // Incrament flash count
            if(multiFlash > 1 and multiFlash != flashCount){
              neopixelStrip.fill(off,lowerRightCorner(),width()); // turn off lower light
              currentOverallState = 4; // Update State
            } else {
              neopixelStrip.fill(colorOne,upperLeftCorner(),width()); // Set upper color
              neopixelStrip.fill(off,lowerRightCorner(),width()); // Set lower color
              currentOverallState = 1; // Update State
              flashCount = 0; // reset flash counter
              numOfPatternCycles ++; // Incrament pattern cycle count
            }
            lastUpdateTime = millis(); // Mark the update time
            updateNeeded = true; // Activate update flag
          }
          break;

        case 4: // Change to DOWN ON state
          if(millis() > lastUpdateTime + FLASH_OFF_TIME_SETTING){
            if(flashCount == 1 and colorTwo != off){
              neopixelStrip.fill(colorTwo,lowerRightCorner(),width()); // Set the color
            } else if(flashCount == 2 and colorThree != off){
              neopixelStrip.fill(colorThree,lowerRightCorner(),width()); // Set the color
            } else {
              neopixelStrip.fill(colorOne,lowerRightCorner(),width()); // Set the color
            }
            currentOverallState = 3; // Update State
            lastUpdateTime = millis(); // Mark the update time
            updateNeeded = true; // Activate update flag
          }
          break;

      }

      if(numOfPatternCycles == patternCyclesToRun){
        return true; // Return True when we have run the requested amount of pattern cycles
      } else {
        return false;
      }

    }

    // Flash pattern - Left side then Right side - 4
    boolean leftAndRight(byte multiFlash, uint32_t colorOne, uint32_t colorTwo = off, uint32_t colorThree = off, byte patternCyclesToRun = 1){

      // Caculate actual flash time
      if(multiFlash > 1){
        flashOnTime = (FLASH_ON_TIME_SETTING / multiFlash) - (FLASH_OFF_TIME_SETTING * multiFlash);
      } else {
        flashOnTime = FLASH_ON_TIME_SETTING;
      }

      // Check if this pattern has just been activated - Pattern #4
      if(currentPatternLocal != 4){
        currentPatternLocal = 4; // Set to new current patern
        currentOverallState = 0; // Reset state to initial value
        numOfPatternCycles = 0; // Reset number of pattern cycles
      }

      switch(currentOverallState){

        case 0: // Reset to initial LEFT ON state
          neopixelStrip.fill(off,1,lastLedAddress); // clear all values
          neopixelStrip.fill(colorOne,lowerMidPoint(),leftWrap()); // turn on LEFT light
          lastUpdateTime = millis(); // Mark the update time
          currentOverallState = 1; // Update State
          flashCount = 0; //update flash count
          updateNeeded = true; // Activate update flag
          break;

        case 1: // Change to either LEFT OFF or RIGHT ON state
          if(millis() > lastUpdateTime + flashOnTime){
            flashCount ++; // Incrament flash count
            if(multiFlash > 1 and multiFlash != flashCount){
              neopixelStrip.fill(off,lowerMidPoint(),leftWrap()); // turn off LEFT light
              currentOverallState = 2; // Update State
            } else {
              neopixelStrip.fill(off,lowerMidPoint(),leftWrap()); // Set Left color
              neopixelStrip.fill(colorOne,1,rightWraps()); // Set lowwer RIGHT color
              neopixelStrip.fill(colorOne,upperMidPoint(),rightWraps()); // Set Upper RIGHT color
              currentOverallState = 3; // Update State
              flashCount = 0; // reset flash counter
            }
            lastUpdateTime = millis(); // Mark the update time
            updateNeeded = true; // Activate update flag
          }
          break;

        case 2: // Change to LEFT ON state
          if(millis() > lastUpdateTime + FLASH_OFF_TIME_SETTING){
            if(flashCount == 1 and colorTwo != off){
              neopixelStrip.fill(colorTwo,lowerMidPoint(),leftWrap()); // Set the color
            } else if(flashCount == 2 and colorThree != off){
              neopixelStrip.fill(colorThree,lowerMidPoint(),leftWrap()); // Set the color
            } else {
              neopixelStrip.fill(colorOne,lowerMidPoint(),leftWrap()); // Set the color
            }
            currentOverallState = 1; // Update State
            lastUpdateTime = millis(); // Mark the update time
            updateNeeded = true; // Activate update flag
          }
          break;

        case 3: // Change to either RIGHT OFF or LEFT ON state
          if(millis() > lastUpdateTime + flashOnTime){
            flashCount ++; // Incrament flash count
            if(multiFlash > 1 and multiFlash != flashCount){
              neopixelStrip.fill(off,1,rightWraps()); // turn off lower RIGHT light
              neopixelStrip.fill(off,upperMidPoint(),rightWraps()); // turn off Upper RIGHT light
              currentOverallState = 4; // Update State
            } else {
              neopixelStrip.fill(off,1,rightWraps()); // turn off lower RIGHT light
              neopixelStrip.fill(off,upperMidPoint(),rightWraps()); // turn off Upper RIGHT light
              neopixelStrip.fill(colorOne,lowerMidPoint(),leftWrap()); // Set upper color
              currentOverallState = 1; // Update State
              flashCount = 0; // reset flash counter
              numOfPatternCycles ++; // Incrament pattern cycle count
            }
            lastUpdateTime = millis(); // Mark the update time
            updateNeeded = true; // Activate update flag
          }
          break;

        case 4: // Change to RIGHT ON state
          if(millis() > lastUpdateTime + FLASH_OFF_TIME_SETTING){
            if(flashCount == 1 and colorTwo != off){
              neopixelStrip.fill(colorTwo,1,rightWraps()); // turn off lower RIGHT light
              neopixelStrip.fill(colorTwo,upperMidPoint(),rightWraps()); // turn off Upper RIGHT light
            } else if(flashCount == 2 and colorThree != off){
              neopixelStrip.fill(colorThree,1,rightWraps()); // turn off lower RIGHT light
              neopixelStrip.fill(colorThree,upperMidPoint(),rightWraps()); // turn off Upper RIGHT light
            } else {
              neopixelStrip.fill(colorOne,1,rightWraps()); // turn off lower RIGHT light
              neopixelStrip.fill(colorOne,upperMidPoint(),rightWraps()); // turn off Upper RIGHT light
            }
            currentOverallState = 3; // Update State
            lastUpdateTime = millis(); // Mark the update time
            updateNeeded = true; // Activate update flag
          }
          break;

      }

      if(numOfPatternCycles == patternCyclesToRun){
        return true; // Return True when we have run the requested amount of pattern cycles
      } else {
        return false;
      }

    }

    // Flash pattern - kiddy corner to each other - 5
    boolean crissCross(byte multiFlash, uint32_t colorOne, uint32_t colorTwo = off, uint32_t colorThree = off, byte patternCyclesToRun = 1){

      // Caculate actual flash time
      if(multiFlash > 1){
        flashOnTime = (FLASH_ON_TIME_SETTING / multiFlash) - (FLASH_OFF_TIME_SETTING * multiFlash);
      } else {
        flashOnTime = FLASH_ON_TIME_SETTING;
      }

      // Check if this pattern has just been activated - Pattern #5
      if(currentPatternLocal != 5){
        currentPatternLocal = 5; // Set to new current patern
        currentOverallState = 0; // Reset state to initial value
        numOfPatternCycles = 0; // Reset number of pattern cycles
      }

      switch(currentOverallState){

        case 0: // Reset to initial UP RIGHT/DOWN LEFT ON state
          neopixelStrip.fill(off,1,lastLedAddress); // clear all values
          neopixelStrip.fill(colorOne,lowerMidPoint(),halfWidth()); // turn ON lower left light
          neopixelStrip.fill(colorOne,upperMidPoint(),halfWidth()); // turn ON upper right light
          lastUpdateTime = millis(); // Mark the update time
          currentOverallState = 1; // Update State
          flashCount = 0; //update flash count
          updateNeeded = true; // Activate update flag
          break;

        case 1: // Change to either UP RIGHT/DOWN LEFT OFF or UP LEFT/DOWN RIGHT ON state
          if(millis() > lastUpdateTime + flashOnTime){
            flashCount ++; // Incrament flash count
            if(multiFlash > 1 and multiFlash != flashCount){
              neopixelStrip.fill(off,1,lastLedAddress); // clear all values
              currentOverallState = 2; // Update State
            } else {
              neopixelStrip.fill(off,1,lastLedAddress); // clear all values
              neopixelStrip.fill(colorOne,lowerRightCorner(),halfWidth()); // turn ON lower right light
              neopixelStrip.fill(colorOne,upperLeftCorner(),halfWidth()); // turn ON upper left light
              currentOverallState = 3; // Update State
              flashCount = 0; // reset flash counter
            }
            lastUpdateTime = millis(); // Mark the update time
            updateNeeded = true; // Activate update flag
          }
          break;

        case 2: // Change to UP RIGHT/DOWN LEFT ON state
          if(millis() > lastUpdateTime + FLASH_OFF_TIME_SETTING){
            if(flashCount == 1 and colorTwo != off){
              neopixelStrip.fill(colorTwo,lowerMidPoint(),halfWidth()); // turn ON lower left light with 2nd color
              neopixelStrip.fill(colorTwo,upperMidPoint(),halfWidth()); // turn ON upper right light with 2nd color
            } else if(flashCount == 2 and colorThree != off){
              neopixelStrip.fill(colorThree,lowerMidPoint(),halfWidth()); // turn ON lower left light with 3rd color
              neopixelStrip.fill(colorThree,upperMidPoint(),halfWidth()); // turn ON upper right light with 3rd color
            } else {
              neopixelStrip.fill(colorOne,lowerMidPoint(),halfWidth()); // turn ON lower left light with 1st color
              neopixelStrip.fill(colorOne,upperMidPoint(),halfWidth()); // turn ON upper right light with 1st color
            }
            currentOverallState = 1; // Update State
            lastUpdateTime = millis(); // Mark the update time
            updateNeeded = true; // Activate update flag
          }
          break;

        case 3: // Change to either UP LEFT/DOWN RIGHT OFF or UP RIGHT/DOWN LEFT ON state
          if(millis() > lastUpdateTime + flashOnTime){
            flashCount ++; // Incrament flash count
            if(multiFlash > 1 and multiFlash != flashCount){
              neopixelStrip.fill(off,1,lastLedAddress); // clear all values
              currentOverallState = 4; // Update State
            } else {
              neopixelStrip.fill(off,1,lastLedAddress); // clear all values
              neopixelStrip.fill(colorOne,lowerMidPoint(),halfWidth()); // turn ON lower left light
              neopixelStrip.fill(colorOne,upperMidPoint(),halfWidth()); // turn ON upper right ligt
              currentOverallState = 1; // Update State
              flashCount = 0; // reset flash counter
              numOfPatternCycles ++; // Incrament pattern cycle count
            }
            lastUpdateTime = millis(); // Mark the update time
            updateNeeded = true; // Activate update flag
          }
          break;

        case 4: // Change to UP LEFT/DOWN RIGHT ON state
          if(millis() > lastUpdateTime + FLASH_OFF_TIME_SETTING){
            if(flashCount == 1 and colorTwo != off){
              neopixelStrip.fill(colorTwo,lowerRightCorner(),halfWidth()); // turn ON lower right light with 2nd color
              neopixelStrip.fill(colorTwo,upperLeftCorner(),halfWidth()); // turn ON upper left light with 2nd color
            } else if(flashCount == 2 and colorThree != off){
              neopixelStrip.fill(colorThree,lowerRightCorner(),halfWidth()); // turn ON lower right light with 3rd color
              neopixelStrip.fill(colorThree,upperLeftCorner(),halfWidth()); // turn ON upper left light with 3rd color
            } else {
              neopixelStrip.fill(colorOne,lowerRightCorner(),halfWidth()); // turn ON lower right light with 1st color
              neopixelStrip.fill(colorOne,upperLeftCorner(),halfWidth()); // turn ON upper left light with 1st color
            }
            currentOverallState = 3; // Update State
            lastUpdateTime = millis(); // Mark the update time
            updateNeeded = true; // Activate update flag
          }
          break;

      }

      if(numOfPatternCycles == patternCyclesToRun){
        return true; // Return True when we have run the requested amount of pattern cycles
      } else {
        return false;
      }

    }

    // Flash pattern - Every other light - 6
    boolean everyOther(byte multiFlash, uint32_t colorOne, uint32_t colorTwo = off, uint32_t colorThree = off, byte patternCyclesToRun = 1){

      // Caculate actual flash time
      if(multiFlash > 1){
        flashOnTime = (FLASH_ON_TIME_SETTING / multiFlash) - (FLASH_OFF_TIME_SETTING * multiFlash);
      } else {
        flashOnTime = FLASH_ON_TIME_SETTING;
      }


      // Check if this pattern has just been activated - Pattern #6
      if(currentPatternLocal != 6){
        currentPatternLocal = 6; // Set to new current patern
        currentOverallState = 0; // Reset state to initial value
        numOfPatternCycles = 0; // Reset number of pattern cycles
      }

      switch(currentOverallState){
        case 0: // Reset to initial ON state - Odd #pixels
          for(int i=1; i<neopixelStrip.numPixels(); i++){
            if(i % 2 == 0){ // Even pixels OFF
              neopixelStrip.setPixelColor(i, off);
            } else { // Odd pixels color #1
              neopixelStrip.setPixelColor(i, colorOne);
            }
          }
          lastUpdateTime = millis(); // Mark the update time
          currentOverallState = 1; // Update State
          flashCount = 0; //update flash count
          updateNeeded = true; // Activate update flag
          break;

        case 1: // Change to OFF state #1
          if(millis() >= lastUpdateTime + flashOnTime){
            neopixelStrip.fill(off,1,lastLedAddress); // Turn off strip
            lastUpdateTime = millis(); // Mark the update time
            currentOverallState = 2; // Update State
            flashCount ++; // Incrament flash count
            updateNeeded = true; // Activate update flag
          }
          break;

        case 2: // Loop back to ON state
          if(flashCount == multiFlash){
            // Wait for full time if we have flashed the proper amount of times.
            if(millis() >= lastUpdateTime + FLASH_ON_TIME_SETTING){
              for(int i=1; i<neopixelStrip.numPixels(); i++){
                if(i % 2 == 0){ // Even pixels Color #1
                  neopixelStrip.setPixelColor(i, colorOne);
                } else { // Odd pixels OFF
                  neopixelStrip.setPixelColor(i, off);
                }
              }
              lastUpdateTime = millis(); // Mark the update time
              currentOverallState = 3; // Update State
              flashCount = 0; //update flash count
              updateNeeded = true; // Activate update flag
            }
          } else {
            if(millis() >= lastUpdateTime + FLASH_OFF_TIME_SETTING){
              if(flashCount == 1 and colorTwo != off){
                for(int i=1; i<neopixelStrip.numPixels(); i++){
                  if(i % 2 == 0){ // Even pixels OFF
                    neopixelStrip.setPixelColor(i, off);
                  } else { // Odd pixels color #1
                    neopixelStrip.setPixelColor(i, colorTwo);
                  }
                }
              } else if(flashCount == 2 and colorThree != off){
                for(int i=1; i<neopixelStrip.numPixels(); i++){
                  if(i % 2 == 0){ // Even pixels OFF
                    neopixelStrip.setPixelColor(i, off);
                  } else { // Odd pixels color #1
                    neopixelStrip.setPixelColor(i, colorThree);
                  }
                }
              } else {
                for(int i=1; i<neopixelStrip.numPixels(); i++){
                  if(i % 2 == 0){ // Even pixels OFF
                    neopixelStrip.setPixelColor(i, off);
                  } else { // Odd pixels color #1
                    neopixelStrip.setPixelColor(i, colorOne);
                  }
                }
              }
              lastUpdateTime = millis(); // Mark the update time
              currentOverallState = 1; // Update State
              updateNeeded = true; // Activate update flag
            }
          }
          break;

        case 3: // Change to OFF state #2
          if(millis() >= lastUpdateTime + flashOnTime){
            neopixelStrip.fill(off,1,lastLedAddress); // Turn off strip
            lastUpdateTime = millis(); // Mark the update time
            currentOverallState = 4; // Update State
            flashCount ++; // Incrament flash count
            updateNeeded = true; // Activate update flag
          }
          break;

        case 4: // Loop back to ON state
          if(flashCount == multiFlash){
            // Wait for full time if we have flashed the proper amount of times.
            if(millis() >= lastUpdateTime + FLASH_ON_TIME_SETTING){
              for(int i=1; i<neopixelStrip.numPixels(); i++){
                if(i % 2 == 0){ // Even pixels OFF
                  neopixelStrip.setPixelColor(i, off);
                } else { // Odd pixels color #1
                  neopixelStrip.setPixelColor(i, colorOne);
                }
              }
              lastUpdateTime = millis(); // Mark the update time
              currentOverallState = 1; // Update State
              flashCount = 0; //update flash count
              numOfPatternCycles ++; // Incrament pattern cycle count
              updateNeeded = true; // Activate update flag
            }
          } else {
            if(millis() >= lastUpdateTime + FLASH_OFF_TIME_SETTING){
              if(flashCount == 1 and colorTwo != off){
                for(int i=1; i<neopixelStrip.numPixels(); i++){
                  if(i % 2 == 0){ // Even pixels Color #2
                    neopixelStrip.setPixelColor(i, colorTwo);
                  } else { // Odd pixels OFF
                    neopixelStrip.setPixelColor(i, off);
                  }
                }
              } else if(flashCount == 2 and colorThree != off){
                for(int i=1; i<neopixelStrip.numPixels(); i++){
                  if(i % 2 == 0){ // Even pixels Color #3
                    neopixelStrip.setPixelColor(i, colorThree);
                  } else { // Odd pixels OFF
                    neopixelStrip.setPixelColor(i, off);
                  }
                }
              } else {
                for(int i=1; i<neopixelStrip.numPixels(); i++){
                  if(i % 2 == 0){ // Even pixels Color #1
                    neopixelStrip.setPixelColor(i, colorOne);
                  } else { // Odd pixels OFF
                    neopixelStrip.setPixelColor(i, off);
                  }
                }
              }
              lastUpdateTime = millis(); // Mark the update time
              currentOverallState = 3; // Update State
              updateNeeded = true; // Activate update flag
            }
          }
          break;
      }

      if(numOfPatternCycles == patternCyclesToRun){
        return true; // Return True when we have run the requested amount of pattern cycles
      } else {
        return false;
      }
    }

    // Flash pattern - Around the world - 7
    boolean aroundTheWorld(byte multiFlash, uint32_t colorOne, uint32_t colorTwo = off, uint32_t colorThree = off, byte patternCyclesToRun = 1){

      // Caculate actual flash time - This variable is used differently than the other patterns.  This is time between pixel additions
      switch(lastLedAddress){
        case 12:
          switch(multiFlash){ // on the smaller strips, change the timing instead of the number of pixels to incrememnt at once 
            case 1:
              flashOnTime = 110;
              break;

            case 2:
              flashOnTime = 55;
              break;

            case 3:
              flashOnTime = 33;
              break;
          }
        
          break;

        case 38:
          // Add when bulmber bar is installed.
          break;

        case 82:
          flashOnTime = 30;
          break;
      }
      
      if(colorOne == colorThree && multiFlash == 3){ // This pattern only works with 3 colors when the colors are different.
        multiFlash = 2;
      }

      // Check if this pattern has just been activated - Pattern #7
      if(currentPatternLocal != 7){
        currentPatternLocal = 7; // Set to new current patern
        currentOverallState = 0; // Reset state to initial value

        switch(lastLedAddress){// Used to keep track of which light we are turning on next
          case 12:
            currentAdditionalStateOne = 1;
            break;

          case 38:
            // Add when bulmber bar is installed.
            break;

          case 82:
            currentAdditionalStateOne = multiFlash * 2;
            break;
        }

        numOfPatternCycles = 0; // Reset number of pattern cycles
      }

      switch(currentOverallState){
        case 0: // Reset to initial ON state - 1st pixel
          for(int i=1; i<neopixelStrip.numPixels(); i++){
            if(i <= currentAdditionalStateOne){ // Turn on the first pixel
              neopixelStrip.setPixelColor(i, colorOne);
            } else { // Turn off all the rest
              neopixelStrip.setPixelColor(i, off);
            }
          }
          lastUpdateTime = millis(); // Mark the update time
          currentOverallState = 1; // Update State
          flashCount = 1; //update flash count
          updateNeeded = true; // Activate update flag
          break;

        case 1: // Continue to fill the whole bar with Color
          if(millis() >= lastUpdateTime + flashOnTime){
            for(int i=1; i<neopixelStrip.numPixels(); i++){
              if(i <= currentAdditionalStateOne){ // Turn on the first pixel
                switch(flashCount){
                  case 1:
                    neopixelStrip.setPixelColor(i, colorOne);
                    break;

                  case 2:
                    neopixelStrip.setPixelColor(i, colorTwo);
                    break;

                  case 3:
                    neopixelStrip.setPixelColor(i, colorThree);
                    break;
                } 
              } else { // Turn off all the rest
                neopixelStrip.setPixelColor(i, off);
              }
            }
            
            lastUpdateTime = millis(); // Mark the update time
            
            switch(lastLedAddress){// Incrememnt to next light
              case 12:
                currentAdditionalStateOne = currentAdditionalStateOne + 1;
                break;

              case 38:
                // Add when bulmber bar is installed.
                break;

              case 82:
                currentAdditionalStateOne = currentAdditionalStateOne + (multiFlash * 2);
                break;
            }
            
            if(currentAdditionalStateOne >=  neopixelStrip.numPixels()){
              currentOverallState = 2; // Update State

              switch(lastLedAddress){// Reset additional state
                case 12:
                  currentAdditionalStateOne = 1;
                  break;

                case 38:
                  // Add when bulmber bar is installed.
                  break;

                case 82:
                  currentAdditionalStateOne = multiFlash * 2;
                  break;
              }

            }
            updateNeeded = true; // Activate update flag
          }
          break;

        case 2: // Turn pixels off one at a time
          if(millis() >= lastUpdateTime + flashOnTime){
            for(int i=1; i<neopixelStrip.numPixels(); i++){
              if(i <= currentAdditionalStateOne){
                neopixelStrip.setPixelColor(i, off);
              } else { // Turn off all the rest
                switch(flashCount){
                  case 1:
                    neopixelStrip.setPixelColor(i, colorOne);
                    break;

                  case 2:
                    neopixelStrip.setPixelColor(i, colorTwo);
                    break;

                  case 3:
                    neopixelStrip.setPixelColor(i, colorThree);
                    break;
                } 
              }
            }
            
            lastUpdateTime = millis(); // Mark the update time

            switch(lastLedAddress){// Incrememnt to next light
              case 12:
                currentAdditionalStateOne = currentAdditionalStateOne + 1;
                break;

              case 38:
                // Add when bulmber bar is installed.
                break;

              case 82:
                currentAdditionalStateOne = currentAdditionalStateOne + (multiFlash * 2);
                break;
            }

            if(currentAdditionalStateOne >=  neopixelStrip.numPixels()){
              currentOverallState = 1; // Update State
              flashCount ++;
              if(flashCount > multiFlash){ 
                flashCount = 1;
              }

              switch(lastLedAddress){// Reset additional state
                case 12:
                  currentAdditionalStateOne = 1;
                  break;

                case 38:
                  // Add when bulmber bar is installed.
                  break;

                case 82:
                  currentAdditionalStateOne = multiFlash * 2;
                  break;
              }

              numOfPatternCycles ++; 
            }
            updateNeeded = true; // Activate update flag
          }
          break;
      }

      if(numOfPatternCycles == patternCyclesToRun){
        return true; // Return True when we have run the requested amount of pattern cycles
      } else {
        return false;
      }
    }

    
    
    // Flash pattern - Hazard - 30 - Meant for rear bar only - Left 1 | Center 0 | Right 2
    void hazardPatternRearBar(byte direction){

      // Define method variables
      byte firstPixel;
      byte secondPixel;
      byte thirdPixel;
      byte lastPixel;
      
      // Declare pattern arrays
      byte toLeftArray[13][2] = { {38,45},
                                  {35,48},
                                  {32,51},
                                  {29,54},
                                  {26,57},
                                  {23,60},
                                  {20,63},
                                  {17,66},
                                  {14,69},
                                  {11,72},
                                  {8,75},
                                  {5,78},
                                  {1,82} };
                        
      byte toRightArray[13][2] = { {4,79},
                                   {7,76},
                                   {10,73},
                                   {13,70},
                                   {16,67},
                                   {19,64},
                                   {22,61},
                                   {25,58},
                                   {28,55},
                                   {31,52},
                                   {34,49},
                                   {37,46},
                                   {41,42} };
                              
      byte centerArray[10][4] = { {20,22,61,63},
                                  {18,24,59,65},
                                  {16,26,57,67},
                                  {14,28,55,69},
                                  {12,30,53,71},
                                  {10,32,51,73},
                                  {8,34,49,75},
                                  {6,36,47,77},
                                  {4,38,45,79},
                                  {1,41,42,82} };

      // Assign timing
      flashOnTime = FLASH_ON_TIME_SETTING / 2;
      
      // Check if this pattern has just been activated - Pattern #30
      if(currentPatternLocal != 30){
        currentPatternLocal = 30; // Set to new current patern
        currentOverallState = 0; // Reset state to initial value
        currentAdditionalStateOne = 0;
        lastUpdateTime = 0;
      }

      switch(currentOverallState){
        case 0: // Fill bar in specified direction
          if(millis() >= lastUpdateTime + flashOnTime){
            switch(direction){
              case 1: // Left
                  
                // Assign first and last pixels to turn on
                firstPixel = toLeftArray[currentAdditionalStateOne][0];
                lastPixel = toLeftArray[currentAdditionalStateOne][1];
                
                for(int i=1; i<neopixelStrip.numPixels(); i++){
                  if(i >= firstPixel && i <= lastPixel){ // Continue filling to the left
                    neopixelStrip.setPixelColor(i, yellow);
                  } else {
                    neopixelStrip.setPixelColor(i, off);
                  }
                }
                break;

              case 0: // Center
                // Assign first and last pixels to turn on
                firstPixel = centerArray[currentAdditionalStateOne][0];
                secondPixel = centerArray[currentAdditionalStateOne][1];
                thirdPixel = centerArray[currentAdditionalStateOne][2];
                lastPixel = centerArray[currentAdditionalStateOne][3];
                
                for(int i=1; i<neopixelStrip.numPixels(); i++){
                  if(i >= firstPixel && i <= secondPixel || i >= thirdPixel && i <= lastPixel){ // Continue filling to the left
                    neopixelStrip.setPixelColor(i, yellow);
                  } else {
                    neopixelStrip.setPixelColor(i, off);
                  }
                }
                break;

              case 2: // Right
                // Assign first and last pixels to turn on
                  firstPixel = toRightArray[currentAdditionalStateOne][0];
                  lastPixel = toRightArray[currentAdditionalStateOne][1];
                  
                  for(int i=1; i<neopixelStrip.numPixels(); i++){
                    if(i <= firstPixel || i >= lastPixel){ // Continue filling to the right
                      neopixelStrip.setPixelColor(i, yellow);
                    } else {
                      neopixelStrip.setPixelColor(i, off);
                    }
                  }
                break;
            }

            lastUpdateTime = millis(); // Mark the update time
            currentAdditionalStateOne ++;

            if(direction == 0){
              if(currentAdditionalStateOne > 9){
                currentAdditionalStateOne = 0;
                currentOverallState = 1; // Update State
              }
            } else {
              if(currentAdditionalStateOne > 12){
                currentAdditionalStateOne = 0;
                currentOverallState = 1; // Update State
              }
            }
            updateNeeded = true; // Activate update flag
          }

          break;

        case 1: // turn bar off
          if(millis() >= lastUpdateTime + flashOnTime){
            switch(direction){
              case 1: // Left
                  
                  // Assign first and last pixels to turn on
                  firstPixel = toLeftArray[currentAdditionalStateOne][0];
                  lastPixel = toLeftArray[currentAdditionalStateOne][1];
                  
                  for(int i=1; i<neopixelStrip.numPixels(); i++){
                    if(i >= firstPixel && i <= lastPixel){ // Continue filling to the left
                      neopixelStrip.setPixelColor(i, off);
                    } else {
                      neopixelStrip.setPixelColor(i, yellow);
                    }
                  }
                break;

              case 0: // Center
                // Assign first and last pixels to turn on
                firstPixel = centerArray[currentAdditionalStateOne][0];
                secondPixel = centerArray[currentAdditionalStateOne][1];
                thirdPixel = centerArray[currentAdditionalStateOne][2];
                lastPixel = centerArray[currentAdditionalStateOne][3];
                
                for(int i=1; i<neopixelStrip.numPixels(); i++){
                  if(i >= firstPixel && i <= secondPixel || i >= thirdPixel && i <= lastPixel){ // Continue filling to the left
                    neopixelStrip.setPixelColor(i, off);
                  } else {
                    neopixelStrip.setPixelColor(i, yellow);
                  }
                }
                break;

              case 2: // Right
                // Assign first and last pixels to turn on
                  firstPixel = toRightArray[currentAdditionalStateOne][0];
                  lastPixel = toRightArray[currentAdditionalStateOne][1];
                  
                  for(int i=1; i<neopixelStrip.numPixels(); i++){
                    if(i <= firstPixel || i >= lastPixel){ // Continue filling to the right
                      neopixelStrip.setPixelColor(i, off);
                    } else {
                      neopixelStrip.setPixelColor(i, yellow);
                    }
                  }
                break;
            }

            lastUpdateTime = millis(); // Mark the update time
            currentAdditionalStateOne ++;

            if(direction == 0){
              if(currentAdditionalStateOne > 9){
                currentAdditionalStateOne = 0;
                currentOverallState = 0; // Update State
              }
            } else {
              if(currentAdditionalStateOne > 12){
                currentAdditionalStateOne = 0;
                currentOverallState = 0; // Update State
              }
            }
            updateNeeded = true; // Activate update flag
          }
          break;
      }
    }
    
    
    // Run the show() function on the strip if updates are required
    void runUpdates(){

      // Check if an update is needed
      if (updateNeeded){
        neopixelStrip.show(); // Run show() on the strip
        updateNeeded = !updateNeeded; // reset back to false
      }
    }

};

class RelayDevice {
  private:
    byte pinNumber;

  public:
    // Constructor
    RelayDevice(byte pinNumber){
      this->pinNumber = pinNumber;
    }

    // Methods
    void on(){
      digitalWrite(pinNumber, HIGH);
    }

    void off(){
      digitalWrite(pinNumber, LOW);
    }
};


// Build Objects
RGBLightBar mainLightBar(82,mainLightStrip);

RelayDevice mainLightBarRelay(MAIN_LIGHT_RELAY_PIN);


void setup() {
  // put your setup code here, to run once:

  // ---I2C Setup---
  Wire.begin(8);           // join I2C bus as Slave with address 8

  // event handler initializations
  Wire.onReceive(dataRcv);    // register an event handler for received data

  // initialize global variables
  i2c_pattern = 255;
  i2c_option = 255;
  currentPattern = 255;
  currentOption = 255;
  i2c_message_LED_flash_start_timer = millis();
  i2c_message_LED_status = 0;

  // Initialize Strip
  mainLightStrip.setBrightness(MAIN_LIGHT_INITIAL_BRIGHNESS);
  mainLightStrip.begin();
  mainLightBar.mainLightOff(); // Shut off main light after setup
  mainLightBar.solidColor(off); // Shut off RGB ring after setup

  // Initialize other outputs
  pinMode(MAIN_LIGHT_RELAY_PIN,OUTPUT);  // Setup The Relay pin
}

void loop() {
  // put your main code here, to run repeatedly:

  //digitalWrite(MAIN_LIGHT_RELAY_PIN, HIGH); // Set relay output high to keep light bar on

  // I2C message testing
  //i2c_pattern = 21;
  //i2c_option = 255;

  //mainLightBar.hazardPatternRearBar(0);
  updateOutputs();

  // Turn off message LED
  if((millis() - i2c_message_LED_flash_start_timer) > I2C_MESSAGE_RECEIVED_LED_FLASH_LENGTH){
    i2c_message_LED_status = 0; // Turn off Light
  }

  digitalWrite(13, i2c_message_LED_status); // Update LED State

  mainLightBar.runUpdates(); // Update light strip constantly

  delay(30); // Delay to allow incoming items to process
}


// --Functions--

// received data handler function
void dataRcv(int numBytes){
  while(Wire.available()) { // read all bytes received
    i2c_pattern = Wire.read();
    i2c_option = Wire.read();
  }
  i2c_message_LED_status = 1; // Turn on the LED
  i2c_message_LED_flash_start_timer = millis();
}

// Make decisions based on the newly received I2C messages
void updateOutputs(){

  // Process incoming I2C Messages and Instantanios decisions
  if(i2c_pattern != 255){

    switch(i2c_pattern){

      case 0: // RGB Light Bar(s) Power OFF

        mainLightBarRelay.off(); // Kill relay to RGB LEDs

        break;

      case 1: // RGB Light Bar(s) Power On

        mainLightBarRelay.on(); // Power relay to RGB LEDs
        delay(30); // Give relay time to close and RGB LEDS time to power on.

        break;

      case 2: // All lights OFF

        // Reset the current Pattern/Option to the default of 255
        currentPattern = 255;
        currentOption = 255;

        mainLightBar.mainLightOff();
        mainLightBar.solidColor(off);

        break;

      case 3: // All main lights OFF

        mainLightBar.mainLightOff();

        break;

      case 4: // All main lights ON

        mainLightBar.mainLightOn();

        break;

      case 5: // Left main lights OFF

        // This board does not have any left facing lights

        break;

      case 6: // Left (only) main lights ON

        // This board does not have any left facing lights

        break;

      case 7: // Right main lights OFF

        // this board does not have any right facing lights

        break;

      case 8: //Right (only) main lights ON

        // this board does not have any right facing lights

        break;

      case 9: // Chase lights OFF

        // This board does not have any chase lights

        break;

      case 10: // Chase lights ON

        // This board does not haave any chase lights

        break;

      case 11: // RGB (All) OFF

        // Reset the Current Patern and Option to the default of 255
        currentPattern = 255;
        currentOption = 255;

        mainLightBar.solidColor(off);

        break;

      case 12: // RGB (All) Red

        mainLightBar.solidColor(red);

        break;

      case 13: // RGB left Red

        // This board does not have any left facing lights

        break;

      case 14: // RGB right Red

        // This board does not have any right facing lights

        break;

      case 15: // Turn signal left OFF

        // This board does nothing with turn signals

        break;

      case 16: // Turn signal left ON

        // This board does nothing with turn signals

        break;

      case 17: // Turn signal right OFF

        // This board does nothing with turn signals

        break;

      case 18: // Turn signal right ON

        // This board does nothing with turn signals

        break;

      case 19: // Brake OFF

        // This board does nothing with brake lights

        break;

      case 20: // Brake ON

        // This board does nothing with brake lights

        break;

      case 21: // RGB caution pattern cycle

        currentPattern = i2c_pattern;

        break;

      case 22: // RBG hazard left

        // This board does nothing with hazard signals

        break;

      case 23: // RGB hazard right

        // This board does nothing with hazard signals

        break;

      case 24: // RGB hazard center

        // This board does nothing with hazard signals

        break;

      case 100: // RGB (all) Solid

        switch(i2c_option){
          case 0: // OFF
            mainLightBar.solidColor(off);
            break;
          case 1: // White
            mainLightBar.solidColor(white);
            break;
          case 2: // Red
            mainLightBar.solidColor(red);
            break;
          case 3: // Green
            mainLightBar.solidColor(green);
            break;
          case 4: // Blue
            mainLightBar.solidColor(blue);
            break;
          case 5: // Orange
            mainLightBar.solidColor(orange);
            break;
          case 6: // Yellow
            mainLightBar.solidColor(yellow);
            break;
          case 7: // Purple
            mainLightBar.solidColor(purple);
            break;
        }

        break;

      case 101: // RGB (all) Full Jump Change Colors

        // Program this later

        break;

      case 102: // RGB (all) Full fade change color

        // Program this later

        break;

      case 103: // RGB (all) Full fade out change colors

        // Program this later

        break;

      case 104: // RGB (all) Chase Fade

        // Program this later

        break;

      case 105: // RGB (all) chase fade out

        // Program this later

        break;

      case 106:  // RGB (all) Rainbow Road

        currentPattern = i2c_pattern;
        currentOption = i2c_option;

        break;
    }

    // Reset i2c variables back to 255
    i2c_pattern = 255;
    i2c_option = 255;
  }

  // Process continuous patterns
  switch(currentPattern){

    case 21: // Caution Pattern Cycle - 255 No 2nd perameter

      mainLightBar.cautionPatternCycle();

      break;

    case 106: // Rainbow - 80 Full Speed | 81 Fast | 82 Moderate | 83 Slow

      switch(currentOption){
        case 80: // Full Speed
          mainLightBar.rainbow(0);
          break;
        case 81: // Fast
          mainLightBar.rainbow(1);
          break;
        case 82: // Moderate
          mainLightBar.rainbow(2);
          break;
        case 83: // Slow
          mainLightBar.rainbow(3);
          break;
      }

      break;
  }
}
