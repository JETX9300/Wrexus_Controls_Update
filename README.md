# Wrexus_Controls_Update

# Basic Description
This project is meant to add new Ardunio based controls to the Wrexus Gambler 500 vehicle.

# Controled Devices

## Bumper Light
Description
* Front light bar attached to bumper
Inputs
* Overhead Switch
   * Positions
      * On
         * Light will be forced on
      * Off
         * Light will be forced off
      * Auto
         * Allow light to turn on when the system decides it should. Times the light would come on include:
            * It is night and the high beams are on.
            * Off Road mode is on and the high beams are on.
Outputs
* WS2812 Control

## Main Light Bar
Description
* Forward facing light bar attached to roof rack
Inputs
* Overhead Switch
   * Positions
      * On
         * Light will be forced on
      * Off
         * Light will be forced off
      * Auto
         * Allow light to turn on when the system decides it should. Times the light would come on include:
            * Off Road mode is on and the high beams are on.
Outputs
* Current lights
  * Remote 5v trigger relay
* Upgraded lights (future)
  * WS2812 Control

## Main Light Bar RGB Rings (Future)

## Side Lights
Description
* Lights facing right and left that are attached to the roof rack
Inputs
* Overhead Switch - Together
   * Positions
      * On
         * All Lights will be forced on
      * Off
         * All Lights will be forced off
      * Auto
         * Allow light to turn on when the system decides it should. Times the light would come on include:
            * NO AUTO CONFIGURATION AT THIS TIME
* Overhead Switch - Individual
   * Positions
      * Left
         * Force on the left light only
      * Off
         * All Lights will be forced off
      * Right
         * Force on the right light only
Outputs
* Current lights
  * Remote 5v trigger relay - Left
  * Remote 5v trigger relay - Right
* Upgraded lights (future)
  * WS2812 Control
  
## Side Lights RGB Rings (Future)
## Rear Light
## Rear Light RGB Ring (Future)
## GMRS Radio
## Bluetooth Radio
## Radio Dimmer
## Headlight RGB Rings (Future)
## Foglight RGB Rings (Future)

# Modes
## Off Road
## Hazard
## All On
## Observatory

# Inputs From Car
## Headlights
## High Beams
## Right Turn Signal
## Left Turn Signal
## Ignition Switch
## Doors
## Trunk (IF separate from doors)
## Reverse Gear

# Added Inputs
## Light Sensor
## Pitch/Roll Sensor
## Temp Sensor
