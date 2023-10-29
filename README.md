# ESP32-2432S028R_TripMaster
Rally tripmaster based on the ESP32-2432S028R development board and the Beitian BE-280 GPS module

![ESP32-2432S028R_TripMaster](https://i.imgur.com/IejWvIF.jpg)

# Setup
You will need to dowload the TFT_eSPI library, add the ESP32-2432S028R-User_Setup.h to the TFT_eSPI\User_Setups folder  
Then modify the TFT_eSPI\User_Setup_Select.h file to include the user setup in order to make the screen in the ESP32-2432S028R development board work:  
#include <User_Setups/ESP32-2432S028R-User_Setup.h>  
  
You will need TinyGPS++ library, no modifications required.
  
To flash the module you'll need to add ESP32 support to the Arduino IDE, and select the following:
- Board: ESP32 Dev Module
- Flash mode: DIO
  
You might also need the [CP210x Universal Windows Driver](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers) if you can't reach the COM port  
  
# Documentation and examples for this board
[Not my repo but](http://pan.jczn1688.com/directlink/1/ESP32%20module/2.8inch_ESP32-2432S028R.rar)
  
# Hardware
Here's the hardware I've used:  
- ESP32-2432S028R development board  
- Beitian BE-280 GPS module  
- Any kind of buttons x2 (I've used a motorcycle handlebar switch module)  
- 100 x 68 x 50 mm case (I've used just the transparent lid and 3D printed the back part to make it thinner)
- 1 GX16-5pin connectors for the buttons and the USB connection
- 3 Micro JST connectors
- 1 micro SD card

# Wiring diagram
![](https://i.imgur.com/t1iq9gP.png)
![](https://i.imgur.com/CNndlpF.png)

# Adding a splash image
Create your PNG image (480x320px), and convert it to .c file in this website http://www.rinkydinkelectronics.com/t_imageconverter565.php
Then save the image as splash.h (important .h not .c) in the root folder. Compile and upload
