
#include <Adafruit_NeoPixel.h>

#define PIN 53
// #define NUM_LEDS 39 // Small Light
#define NUM_LEDS 83
#define BRIGHTNESS 255
#define PATTERN_CYCLE_TIME 5000

#define FLASH_ON_TIME_SETTING 400
#define FLASH_OFF_TIME_SETTING 20

unsigned long lastPatternChange = millis();
byte currentLightPattern = 2;
byte randomPatternFlair = random(1,3);

// Setup Strip
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, PIN, NEO_GRB + NEO_KHZ800);

// Define named colors
//uint32_t hudColor = strip.Color(0, 0, 0, 255); // Current hud color
uint32_t white = strip.Color(255,255,255); // Daytime indicator hud state
uint32_t red = strip.Color(255,0,0); // Nightime indicator hud state
uint32_t green = strip.Color(0,255,0); // On indicator state
uint32_t blue = strip.Color(0,0,255);
uint32_t orange = strip.Color(255,80,0); // Auto indicator state
uint32_t yellow = strip.Color(255,180,10);
uint32_t off = strip.Color(0,0,0);

// Build Classes
class BumperLight {
  private:

    // Variables
    byte lastLedAddress;
    byte currentCautionPatternCall;
    byte currentPattern;
    byte currentState;
    byte numOfPatternCycles;
    unsigned long lastUpdateTime;
    int flashOnTime;
    byte flashCount;
    long firstPixelHue;

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
    BumperLight(byte lastLedAddress){
      this->lastLedAddress = lastLedAddress;
    }

    // Methods
    void mainLightOn(){
      strip.setPixelColor(0,red);
    }

    void mainLightOff(){
      strip.setPixelColor(0,green);
    }

    void solidColor(uint32_t color){
      strip.fill(color,1,lastLedAddress);
    }

    void cautionPatternCycle(){

      if(currentCautionPatternCall < 2 or currentCautionPatternCall > 5){
        currentCautionPatternCall = 2;
      }

      if(currentCautionPatternCall == 2){
        if(flashFull(randomPatternFlair,yellow,white,yellow,5)){
          currentCautionPatternCall ++;
          randomPatternFlair = random(1,4);
        }
      } else if(currentCautionPatternCall == 3){
        if(upAndDown(randomPatternFlair,yellow,white,yellow,5)){
          currentCautionPatternCall ++;
          randomPatternFlair = random(1,4);
        }
      } else if(currentCautionPatternCall == 4){
        if(leftAndRight(randomPatternFlair,yellow,white,yellow,5)){
          currentCautionPatternCall ++;
          randomPatternFlair = random(1,4);
        }
      } else if(currentCautionPatternCall == 5){
        if(crissCross(randomPatternFlair,yellow,white,yellow,5)){
          currentCautionPatternCall ++;
          randomPatternFlair = random(1,4);
        }
      }
    }

    // Rainbow cycle along whole strip. Pass speed 0 = Full Speed | 1 = Fast | 2 = Moderate | 3 = Slow
    void rainbow(int wait) {

      // Check if this pattern has just been activated - Pattern #4
      if(currentPattern != 1){
        currentPattern = 1; // Set to new current patern
        firstPixelHue = 0; // Reset state to initial value
        lastUpdateTime = millis() + wait * 10; // Set lastUpdateTime so code will run first time
      }

      if(millis() > lastUpdateTime + wait * 10){
        // Reset pixel hue if we have gone over the amount
        if(firstPixelHue > 5*65536){
          firstPixelHue = 0;
        }

        // Fill light with the current ranbow patern
        for(int i=1; i<strip.numPixels(); i++) { // For each pixel in strip...
          // Offset pixel hue by an amount to make one full revolution of the
          // color wheel (range of 65536) along the length of the strip
          // (strip.numPixels() steps):
          int pixelHue = firstPixelHue + (i * 65536L / strip.numPixels());
          // strip.ColorHSV() can take 1 or 3 arguments: a hue (0 to 65535) or
          // optionally add saturation and value (brightness) (each 0 to 255).
          // Here we're using just the single-argument hue variant. The result
          // is passed through strip.gamma32() to provide 'truer' colors
          // before assigning to each pixel:
          strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue)));
        }

        // Incrament Hue value
        firstPixelHue += 256;

        // Set last update time
        lastUpdateTime = millis();
      }
    }

    // Flash whole strip at same time
    boolean flashFull(byte multiFlash, uint32_t colorOne, uint32_t colorTwo = off, uint32_t colorThree = off, byte patternCyclesToRun = 1){

      //Definitions
      //#define FLASH_ON_TIME_SETTING 900
      //#define FLASH_OFF_TIME_SETTING 50

      // Caculate actual flash time
      if(multiFlash > 1){
        flashOnTime = (FLASH_ON_TIME_SETTING / multiFlash) - (FLASH_OFF_TIME_SETTING * multiFlash);
      } else {
        flashOnTime = FLASH_ON_TIME_SETTING;
      }


      // Check if this pattern has just been activated - Pattern #2
      if(currentPattern != 2){
        currentPattern = 2; // Set to new current patern
        currentState = 0; // Reset state to initial value
        numOfPatternCycles = 0; // Reset number of pattern cycles
      }

      switch(currentState){
        case 0: // Reset to initial ON state
          strip.fill(colorOne,1,lastLedAddress); // Set the color
          lastUpdateTime = millis(); // Mark the update time
          currentState = 1; // Update State
          flashCount = 0; //update flash count
          break;

        case 1: // Change to OFF state
          if(millis() >= lastUpdateTime + flashOnTime){
            strip.fill(off,1,lastLedAddress); // Turn off strip
            lastUpdateTime = millis(); // Mark the update time
            currentState = 2; // Update State
            flashCount ++; // Incrament flash count
          }
          break;

        case 2: // Loop back to ON state
          if(flashCount == multiFlash){
            // Wait for full time if we have flashed the proper amount of times.
            if(millis() >= lastUpdateTime + FLASH_ON_TIME_SETTING){
              strip.fill(colorOne,1,lastLedAddress); // Set the color
              lastUpdateTime = millis(); // Mark the update time
              currentState = 1; // Update State
              flashCount = 0; //update flash count
              numOfPatternCycles ++; // Incrament pattern cycle count
            }
          } else {
            if(millis() >= lastUpdateTime + FLASH_OFF_TIME_SETTING){
              if(flashCount == 1 and colorTwo != off){
                strip.fill(colorTwo,1,lastLedAddress); // Set the color
              } else if(flashCount == 2 and colorThree != off){
                strip.fill(colorThree,1,lastLedAddress); // Set the color
              } else {
                strip.fill(colorOne,1,lastLedAddress); // Set the color
              }
              lastUpdateTime = millis(); // Mark the update time
              currentState = 1; // Update State
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

    // Flash top strip then bottom strip
    boolean upAndDown(byte multiFlash, uint32_t colorOne, uint32_t colorTwo = off, uint32_t colorThree = off, byte patternCyclesToRun = 1){

      //Definitions
      //#define FLASH_ON_TIME_SETTING 900
      //#define FLASH_OFF_TIME_SETTING 50

      // Caculate actual flash time
      if(multiFlash > 1){
        flashOnTime = (FLASH_ON_TIME_SETTING / multiFlash) - (FLASH_OFF_TIME_SETTING * multiFlash);
      } else {
        flashOnTime = FLASH_ON_TIME_SETTING;
      }

      // Check if this pattern has just been activated - Pattern #3
      if(currentPattern != 3){
        currentPattern = 3; // Set to new current patern
        currentState = 0; // Reset state to initial value
        numOfPatternCycles = 0; // Reset number of pattern cycles
      }

      switch(currentState){

        case 0: // Reset to initial UP ON state
          strip.fill(off,1,lastLedAddress); // clear all values
          strip.fill(colorOne,upperLeftCorner(), width()) ; // turn on upper light
          lastUpdateTime = millis(); // Mark the update time
          currentState = 1; // Update State
          flashCount = 0; //update flash count
          break;

        case 1: // Change to either UP OFF or DOWN ON state
          if(millis() > lastUpdateTime + flashOnTime){
            flashCount ++; // Incrament flash count
            if(multiFlash > 1 and multiFlash != flashCount){
              strip.fill(off,upperLeftCorner(),width()); // turn off upper light
              currentState = 2; // Update State
            } else {
              strip.fill(colorOne,lowerRightCorner(),width()); // Set lowwer color
              strip.fill(off,upperLeftCorner(),width()); // Set Upper color
              currentState = 3; // Update State
              flashCount = 0; // reset flash counter
            }
            lastUpdateTime = millis(); // Mark the update time
          }
          break;

        case 2: // Change to UP ON state
          if(millis() > lastUpdateTime + FLASH_OFF_TIME_SETTING){
            if(flashCount == 1 and colorTwo != off){
              strip.fill(colorTwo,upperLeftCorner(),width()); // Set the color
            } else if(flashCount == 2 and colorThree != off){
              strip.fill(colorThree,upperLeftCorner(),width()); // Set the color
            } else {
              strip.fill(colorOne,upperLeftCorner(),width()); // Set the color
            }
            currentState = 1; // Update State
            lastUpdateTime = millis(); // Mark the update time
          }
          break;

        case 3: // Change to either DOWN OFF or UP ON state
          if(millis() > lastUpdateTime + flashOnTime){
            flashCount ++; // Incrament flash count
            if(multiFlash > 1 and multiFlash != flashCount){
              strip.fill(off,lowerRightCorner(),width()); // turn off lower light
              currentState = 4; // Update State
            } else {
              strip.fill(colorOne,upperLeftCorner(),width()); // Set upper color
              strip.fill(off,lowerRightCorner(),width()); // Set lower color
              currentState = 1; // Update State
              flashCount = 0; // reset flash counter
              numOfPatternCycles ++; // Incrament pattern cycle count
            }
            lastUpdateTime = millis(); // Mark the update time
          }
          break;

        case 4: // Change to DOWN ON state
          if(millis() > lastUpdateTime + FLASH_OFF_TIME_SETTING){
            if(flashCount == 1 and colorTwo != off){
              strip.fill(colorTwo,lowerRightCorner(),width()); // Set the color
            } else if(flashCount == 2 and colorThree != off){
              strip.fill(colorThree,lowerRightCorner(),width()); // Set the color
            } else {
              strip.fill(colorOne,lowerRightCorner(),width()); // Set the color
            }
            currentState = 3; // Update State
            lastUpdateTime = millis(); // Mark the update time
          }
          break;

      }

      if(numOfPatternCycles == patternCyclesToRun){
        return true; // Return True when we have run the requested amount of pattern cycles
      } else {
        return false;
      }

    }

    // Flash Left side then Right side
    boolean leftAndRight(byte multiFlash, uint32_t colorOne, uint32_t colorTwo = off, uint32_t colorThree = off, byte patternCyclesToRun = 1){

      //Definitions
      //#define FLASH_ON_TIME_SETTING 900
      //#define FLASH_OFF_TIME_SETTING 50

      // Caculate actual flash time
      if(multiFlash > 1){
        flashOnTime = (FLASH_ON_TIME_SETTING / multiFlash) - (FLASH_OFF_TIME_SETTING * multiFlash);
      } else {
        flashOnTime = FLASH_ON_TIME_SETTING;
      }

      // Check if this pattern has just been activated - Pattern #4
      if(currentPattern != 4){
        currentPattern = 4; // Set to new current patern
        currentState = 0; // Reset state to initial value
        numOfPatternCycles = 0; // Reset number of pattern cycles
      }

      switch(currentState){

        case 0: // Reset to initial LEFT ON state
          strip.fill(off,1,lastLedAddress); // clear all values
          strip.fill(colorOne,lowerMidPoint(),leftWrap()); // turn on LEFT light
          lastUpdateTime = millis(); // Mark the update time
          currentState = 1; // Update State
          flashCount = 0; //update flash count
          break;

        case 1: // Change to either LEFT OFF or RIGHT ON state
          if(millis() > lastUpdateTime + flashOnTime){
            flashCount ++; // Incrament flash count
            if(multiFlash > 1 and multiFlash != flashCount){
              strip.fill(off,lowerMidPoint(),leftWrap()); // turn off LEFT light
              currentState = 2; // Update State
            } else {
              strip.fill(off,lowerMidPoint(),leftWrap()); // Set Left color
              strip.fill(colorOne,1,rightWraps()); // Set lowwer RIGHT color
              strip.fill(colorOne,upperMidPoint(),rightWraps()); // Set Upper RIGHT color
              currentState = 3; // Update State
              flashCount = 0; // reset flash counter
            }
            lastUpdateTime = millis(); // Mark the update time
          }
          break;

        case 2: // Change to LEFT ON state
          if(millis() > lastUpdateTime + FLASH_OFF_TIME_SETTING){
            if(flashCount == 1 and colorTwo != off){
              strip.fill(colorTwo,lowerMidPoint(),leftWrap()); // Set the color
            } else if(flashCount == 2 and colorThree != off){
              strip.fill(colorThree,lowerMidPoint(),leftWrap()); // Set the color
            } else {
              strip.fill(colorOne,lowerMidPoint(),leftWrap()); // Set the color
            }
            currentState = 1; // Update State
            lastUpdateTime = millis(); // Mark the update time
          }
          break;

        case 3: // Change to either RIGHT OFF or LEFT ON state
          if(millis() > lastUpdateTime + flashOnTime){
            flashCount ++; // Incrament flash count
            if(multiFlash > 1 and multiFlash != flashCount){
              strip.fill(off,1,rightWraps()); // turn off lower RIGHT light
              strip.fill(off,upperMidPoint(),rightWraps()); // turn off Upper RIGHT light
              currentState = 4; // Update State
            } else {
              strip.fill(off,1,rightWraps()); // turn off lower RIGHT light
              strip.fill(off,upperMidPoint(),rightWraps()); // turn off Upper RIGHT light
              strip.fill(colorOne,lowerMidPoint(),leftWrap()); // Set upper color
              currentState = 1; // Update State
              flashCount = 0; // reset flash counter
              numOfPatternCycles ++; // Incrament pattern cycle count
            }
            lastUpdateTime = millis(); // Mark the update time
          }
          break;

        case 4: // Change to RIGHT ON state
          if(millis() > lastUpdateTime + FLASH_OFF_TIME_SETTING){
            if(flashCount == 1 and colorTwo != off){
              strip.fill(colorTwo,1,rightWraps()); // turn off lower RIGHT light
              strip.fill(colorTwo,upperMidPoint(),rightWraps()); // turn off Upper RIGHT light
            } else if(flashCount == 2 and colorThree != off){
              strip.fill(colorThree,1,rightWraps()); // turn off lower RIGHT light
              strip.fill(colorThree,upperMidPoint(),rightWraps()); // turn off Upper RIGHT light
            } else {
              strip.fill(colorOne,1,rightWraps()); // turn off lower RIGHT light
              strip.fill(colorOne,upperMidPoint(),rightWraps()); // turn off Upper RIGHT light
            }
            currentState = 3; // Update State
            lastUpdateTime = millis(); // Mark the update time
          }
          break;

      }

      if(numOfPatternCycles == patternCyclesToRun){
        return true; // Return True when we have run the requested amount of pattern cycles
      } else {
        return false;
      }

    }

    // Flash top strip then bottom strip
    boolean crissCross(byte multiFlash, uint32_t colorOne, uint32_t colorTwo = off, uint32_t colorThree = off, byte patternCyclesToRun = 1){

      //Definitions
      //#define FLASH_ON_TIME_SETTING 900
      //#define FLASH_OFF_TIME_SETTING 50

      // Caculate actual flash time
      if(multiFlash > 1){
        flashOnTime = (FLASH_ON_TIME_SETTING / multiFlash) - (FLASH_OFF_TIME_SETTING * multiFlash);
      } else {
        flashOnTime = FLASH_ON_TIME_SETTING;
      }

      // Check if this pattern has just been activated - Pattern #3
      if(currentPattern != 5){
        currentPattern = 5; // Set to new current patern
        currentState = 0; // Reset state to initial value
        numOfPatternCycles = 0; // Reset number of pattern cycles
      }

      switch(currentState){

        case 0: // Reset to initial UP RIGHT/DOWN LEFT ON state
          strip.fill(off,1,lastLedAddress); // clear all values
          strip.fill(colorOne,lowerMidPoint(),halfWidth()); // turn ON lower left light
          strip.fill(colorOne,upperMidPoint(),halfWidth()); // turn ON upper right light
          lastUpdateTime = millis(); // Mark the update time
          currentState = 1; // Update State
          flashCount = 0; //update flash count
          break;

        case 1: // Change to either UP RIGHT/DOWN LEFT OFF or UP LEFT/DOWN RIGHT ON state
          if(millis() > lastUpdateTime + flashOnTime){
            flashCount ++; // Incrament flash count
            if(multiFlash > 1 and multiFlash != flashCount){
              strip.fill(off,1,lastLedAddress); // clear all values
              currentState = 2; // Update State
            } else {
              strip.fill(off,1,lastLedAddress); // clear all values
              strip.fill(colorOne,lowerRightCorner(),halfWidth()); // turn ON lower right light
              strip.fill(colorOne,upperLeftCorner(),halfWidth()); // turn ON upper left light
              currentState = 3; // Update State
              flashCount = 0; // reset flash counter
            }
            lastUpdateTime = millis(); // Mark the update time
          }
          break;

        case 2: // Change to UP RIGHT/DOWN LEFT ON state
          if(millis() > lastUpdateTime + FLASH_OFF_TIME_SETTING){
            if(flashCount == 1 and colorTwo != off){
              strip.fill(colorTwo,lowerMidPoint(),halfWidth()); // turn ON lower left light with 2nd color
              strip.fill(colorTwo,upperMidPoint(),halfWidth()); // turn ON upper right light with 2nd color
            } else if(flashCount == 2 and colorThree != off){
              strip.fill(colorThree,lowerMidPoint(),halfWidth()); // turn ON lower left light with 3rd color
              strip.fill(colorThree,upperMidPoint(),halfWidth()); // turn ON upper right light with 3rd color
            } else {
              strip.fill(colorOne,lowerMidPoint(),halfWidth()); // turn ON lower left light with 1st color
              strip.fill(colorOne,upperMidPoint(),halfWidth()); // turn ON upper right light with 1st color
            }
            currentState = 1; // Update State
            lastUpdateTime = millis(); // Mark the update time
          }
          break;

        case 3: // Change to either UP LEFT/DOWN RIGHT OFF or UP RIGHT/DOWN LEFT ON state
          if(millis() > lastUpdateTime + flashOnTime){
            flashCount ++; // Incrament flash count
            if(multiFlash > 1 and multiFlash != flashCount){
              strip.fill(off,1,lastLedAddress); // clear all values
              currentState = 4; // Update State
            } else {
              strip.fill(off,1,lastLedAddress); // clear all values
              strip.fill(colorOne,lowerMidPoint(),halfWidth()); // turn ON lower left light
              strip.fill(colorOne,upperMidPoint(),halfWidth()); // turn ON upper right ligt
              currentState = 1; // Update State
              flashCount = 0; // reset flash counter
              numOfPatternCycles ++; // Incrament pattern cycle count
            }
            lastUpdateTime = millis(); // Mark the update time
          }
          break;

        case 4: // Change to UP LEFT/DOWN RIGHT ON state
          if(millis() > lastUpdateTime + FLASH_OFF_TIME_SETTING){
            if(flashCount == 1 and colorTwo != off){
              strip.fill(colorTwo,lowerRightCorner(),halfWidth()); // turn ON lower right light with 2nd color
              strip.fill(colorTwo,upperLeftCorner(),halfWidth()); // turn ON upper left light with 2nd color
            } else if(flashCount == 2 and colorThree != off){
              strip.fill(colorThree,lowerRightCorner(),halfWidth()); // turn ON lower right light with 3rd color
              strip.fill(colorThree,upperLeftCorner(),halfWidth()); // turn ON upper left light with 3rd color
            } else {
              strip.fill(colorOne,lowerRightCorner(),halfWidth()); // turn ON lower right light with 1st color
              strip.fill(colorOne,upperLeftCorner(),halfWidth()); // turn ON upper left light with 1st color
            }
            currentState = 3; // Update State
            lastUpdateTime = millis(); // Mark the update time
          }
          break;

      }

      if(numOfPatternCycles == patternCyclesToRun){
        return true; // Return True when we have run the requested amount of pattern cycles
      } else {
        return false;
      }

    }
};


// Build Objects
//BumperLight bumperLight(38);
BumperLight bumperLight(82);


void setup() {
  // put your setup code here, to run once:

  // Initialize Strip
  strip.setBrightness(BRIGHTNESS);
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

}

void loop() {
  // put your main code here, to run repeatedly:

  bumperLight.mainLightOff();

  //strip.setPixelColor(2,255,255,255);

  //bumperLight.solidColor(off);

  //bumperLight.crissCross(3,yellow,white);

  bumperLight.cautionPatternCycle();

/*
  // Choose when to cycle the pattern
  if(millis() > lastPatternChange + PATTERN_CYCLE_TIME){
    randomPatternFlair = random(1,4);
    currentLightPattern ++;
    lastPatternChange = millis();

    if(currentLightPattern > 4){
      currentLightPattern = 2;
    }
  }

  // Pattern decision tree
  switch (currentLightPattern){
    case 2:
      bumperLight.flashFull(randomPatternFlair,yellow,white);
      break;

    case 3:
      bumperLight.upAndDown(randomPatternFlair,yellow,white);
      break;

    case 4:
      bumperLight.leftAndRight(randomPatternFlair,yellow,white);
      break;
  }

*/

  strip.show();
}
