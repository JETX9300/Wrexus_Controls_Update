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
