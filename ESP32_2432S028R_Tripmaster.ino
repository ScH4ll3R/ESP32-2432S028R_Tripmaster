// Libraries
#include <TinyGPS++.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <EEPROM.h>
#include <SD.h>

// Icons
#include "sat.h"
#include "splash.h"

#define GPS_BAUD 9600           // GPS module baud rate. GP3906 defaults to 9600.
#define RXD2 35                 // Connect GPS Tx Here
#define TXD2 22                 // Connect GPS Tx Here (Not neeed but useful to reprogram to 10Hz)
#define PIN_INCREASE_BTN 27     // Connect trip increase button here
#define PIN_DECREASE_BTN 22     // Connect trip decrease button here
#define EEPROM_SIZE 5           // Memory size to store user params and trip
#define swVersion "V3.0"        // Current SW version
#define screenMemPos 3          // Position in memory to store the user defined screen rotation
#define screenBGMemPos 1        // Position in memory to store the user defined screen background color

HardwareSerial gpsSerial(2);
TFT_eSPI tft = TFT_eSPI();
TinyGPSPlus tinyGPS;

// User params
uint16_t FG_COLOR = TFT_BLACK;  // Screen foreground color
uint16_t BG_COLOR = TFT_WHITE;  // Screen background color
int screenRotation = 1;         // Screen orientation (1 or 3; both landscape)

// GPS Information
double currentLatitude = 0.0;
double currentLongitude = 0.0;
double previousLatitude = 0.0;
double previousLongitude = 0.0;
double currentSpeed = 0.0;
double tripPartial = 0.0;

// GPS Precision
bool gpsFix = true;
int gpsPrecision = 1000;
bool gpsFound = false;

long refreshms = 0;           // Count to perform GPS updates every X cycles
int holdClick = 0;            // Count to perform extra actions holding buttons

// SD
const int SD_CS = 5;
const char* FILENAME = "/data.txt";

bool blinkState = false; // Estado del parpadeo: false para un color, true para el otro
unsigned long lastBlinkTime = 0; // Última vez que el texto cambió de color
const unsigned long blinkInterval = 500; // Intervalo de parpadeo en milisegundos

int tripAdj = 0;
int showAdj = 0;

/*
   Starts the tft object and sets its color scheme
*/
void initScreen() {
  tft.init();
  tft.setTextColor(FG_COLOR, BG_COLOR);
  tft.setRotation(1);
  //tft.setSwapBytes(true);

}

/*
   Main screen of the app
*/
void bgGPS() {
  tft.fillScreen(BG_COLOR);

  tft.drawString("TRIP", 5, 8, 4);
  tft.drawString("CAP", 10, 140, 4);
  tft.drawString("SPD", 147, 140, 4);

  tft.drawLine(139, 130, 139, 240, FG_COLOR);
  tft.drawLine(140, 130, 140, 240, FG_COLOR);
  tft.drawLine(141, 130, 141, 240, FG_COLOR);


  tft.drawLine( 0, 130, 480, 130, FG_COLOR);
  tft.drawLine( 0, 131, 480, 131, FG_COLOR);
  tft.drawLine( 0, 129, 480, 129, FG_COLOR);

  tft.pushImage(300, 5, 20, 20, sat);
}

/*
   Waiting screen while there's no fixed GPS satelites
*/
void gpsWaitScreen() {
  tft.fillScreen(BG_COLOR);
  tft.drawString("WAITING FOR", 140, 100, 4);
  tft.drawString("GPS SIGNAL", 150, 150, 4);
}

/*
   Configuration Meny
*/
void printCfgMenu(int option) {
  if (option == 1) {
    tft.setTextColor(BG_COLOR, FG_COLOR);
    EEPROM.write(screenBGMemPos, 0);
    tft.drawString("Screen Rotation", 30, 30, 4);
    tft.setTextColor(FG_COLOR, BG_COLOR);
    EEPROM.write(screenBGMemPos, 1);
    tft.drawString("Screen Color", 30, 60, 4);
    tft.drawString("EXIT", 30, 90, 4);
  } else if (option == 2) {
    tft.drawString("Screen Rotation", 30, 30, 4);
    tft.setTextColor(BG_COLOR, FG_COLOR);
    tft.drawString("Screen Color", 30, 60, 4);
    tft.setTextColor(FG_COLOR, BG_COLOR);
    tft.drawString("EXIT", 30, 90, 4);
  } else if (option == 3) {
    tft.drawString("Screen Rotation", 30, 30, 4);
    tft.drawString("Screen Color", 30, 60, 4);
    tft.setTextColor(BG_COLOR, FG_COLOR);
    tft.drawString("EXIT", 30, 90, 4);
    tft.setTextColor(FG_COLOR, BG_COLOR);
  }
  tft.drawString(swVersion, 455, 290, 3);
  delay(100);
}

void handleCfgMenu() {
  tft.fillScreen(BG_COLOR);
  tft.setTextColor(FG_COLOR, BG_COLOR);
  tft.drawString("ENTERING CFG MODE...", 10, 30, 4);
  delay(2000);
  tft.fillScreen(BG_COLOR);
  boolean exit = false;
  int option = 1;
  while (!exit) {
    tft.drawString("CFG MODE", 30, 30, 3);
    printCfgMenu(option);
    if (digitalRead(PIN_INCREASE_BTN) == LOW) {
      if (option > 3) {
        option = 1;
      } else { 
        option++;
      }
    }
    if (digitalRead(PIN_DECREASE_BTN) == LOW) {
      if (option == 3) {
        exit = true;
      }
      else if (option == 1) {
        if (screenRotation == 1) {
          screenRotation = 3;
        } else {
          screenRotation = 1;
        }
        tft.fillScreen(BG_COLOR);
        tft.setRotation(screenRotation);
        EEPROM.write(screenMemPos, screenRotation);
        EEPROM.commit();
      } else if (option == 2) {
        uint16_t flip = BG_COLOR;
        BG_COLOR = FG_COLOR;
        FG_COLOR = flip;
        tft.fillScreen(BG_COLOR);
      }
    }
  }
}


/*
   Main setup method, initializes Serial interfaces, memory storage
   sets button pins and handles the entry to the configuration menu
   by holding the trip increase button during boot
*/
void setup()
{
  // Serial interfaces to read GPS information
  gpsSerial.begin(GPS_BAUD, SERIAL_8N1, RXD2, TXD2);  // Configurar Serial2 en el pin 35

  // Screen initialization
  initScreen();
  tft.pushImage(0, 0, 320, 240, splash);
  delay(800);

  setupSD();
  tripPartial = readTripSD();
  screenRotation = int(EEPROM.read(screenMemPos));

  // Increase and decrease button pins
  pinMode(PIN_DECREASE_BTN, INPUT_PULLUP);
  pinMode(PIN_INCREASE_BTN, INPUT_PULLUP);



  // Check if during boot the increase button is pressed
  int increaseBtnVal = digitalRead(PIN_INCREASE_BTN);
  if (increaseBtnVal == LOW) {
    handleCfgMenu();
  }

  // Stop the ESP32 bluetooth to avoid interference
  btStop();
  bgGPS();


}

void setupSD(){
  if (!SD.begin(SD_CS)) {
    tft.drawString("NO MICRO SD FOUND!", 15, 30, 4);
    delay(3);
  }
  tft.fillScreen(BG_COLOR);

  if (!SD.exists(FILENAME)) {
   File fileSD  = SD.open(FILENAME, FILE_WRITE);
    if (fileSD) {
      fileSD.println("0");
      fileSD.close();
    }
  }
}

/*
   Obtains the information from the GPS and stores it in global variables
*/
void updateGpsValues() {
  if (tinyGPS.location.age() > 3000 || !tinyGPS.location.isValid()) {
    gpsFix = false;
  } else {
    gpsFix = true;
  }

  if (tinyGPS.hdop.isValid()) {
    gpsPrecision = tinyGPS.hdop.value();
  } else {
    gpsPrecision = 1000;
  }

  if (tinyGPS.location.isValid()) {
    currentLatitude = tinyGPS.location.lat();
    currentLongitude = tinyGPS.location.lng();
    currentSpeed = tinyGPS.speed.kmph();
    updateDistance();
  } else {
    currentLatitude = 0;
    currentLongitude = 0;
  }
}

void writeTripSD(double trip){
  File fileSD = SD.open(FILENAME, FILE_WRITE);
  fileSD.print(trip, 2);
  fileSD.close();
}

double readTripSD() {
  File fileSD = SD.open(FILENAME, FILE_READ);
  if (fileSD) {
    String tripSD = fileSD.readString();
    fileSD.close();
    double trip = tripSD.toDouble();
    return trip;
  } else {
    // Error al abrir el archivo
    return 0.0;
  }
}

/*
   Updates the total trip only when the position changes and the vehicle speed
   is over 5 kph in order to filter GPS jitter

   If the vehicle moves over 5 kph it will save the current trip to memory when the
   vehicle comes to a halt.
*/
void updateDistance() {
  if (currentLatitude != previousLatitude || currentLongitude != previousLongitude) {
    // Position has changed. Let's calculate the distance between points.

    double distanceMts =
      tinyGPS.distanceBetween(
        previousLatitude,
        previousLongitude,
        currentLatitude,
        currentLongitude);

    double distanceKms = distanceMts / 1000.0;

    // Update distances
    if (previousLatitude != 0) { // This fixes a big jump in the first update
      if (currentSpeed > 5 && gpsPrecision < 500 && gpsFix) { // This fixes the GPS jitter
        // Update the distance only if I have a decent signal and speed is higher than 5 km/h
        tripPartial += distanceKms;
        // Save Trip
        writeTripSD(tripPartial);
      }
    }

    // Update previous data
    previousLatitude = currentLatitude;
    previousLongitude = currentLongitude;
  }
}

/*
 * Method to handle button presses.
 * Trip increase button:
 *  - Press once to increase 0.01 km
 *  - Hold for a while to increase by 0.1 km each loop
 *  - Keep holding to increase by 1 km each loop
 * Trip decrease button:
 *  - Press once to decrease 0.01 km
 *  - Hold for a while to decrease by 0.1 km each loop
 *  - Keep holding to decrease by 1 km each loop
 * Hold both buttons to store the curent value to memory manually
 * Hold both buttons for a while to reset the trip to 0 (also saves it)
 */
void handleButtons() {
  int decreaseBtnVal = digitalRead(PIN_DECREASE_BTN);
  int increaseBtnVal = digitalRead(PIN_INCREASE_BTN);

  // Store or reset trip
  if (decreaseBtnVal == LOW && increaseBtnVal == LOW) {
    if (holdClick > 40) {
      // Reset trip and store it
      tripPartial = 0;
      writeTripSD(tripPartial);

    } else if (holdClick == 0) {
      // Save Trip to memory
      writeTripSD(tripPartial);

      holdClick ++;
    } else {
      holdClick ++;
    }

  }
  // Decrease values
  else if (decreaseBtnVal == LOW && tripPartial >= 0) {
    if (holdClick > 150) {
      if(tripPartial - 1 <= 0){
         tripPartial = 0;
      } else {
         tripPartial -= 1;
      }
      writeTripSD(tripPartial);
    }
    else if (holdClick > 80) {
      if(tripPartial - 0.1 <= 0){
         tripPartial = 0;
      } else {
         tripPartial -= 0.1;
      }
      writeTripSD(tripPartial);
    } else if(holdClick > 10){
      if(tripPartial - 0.01 <= 0){
        tripPartial = 0;
      } else {
      tripPartial -= 0.01;
      if(tripAdj == 0){
        tft.fillRect(0, 135, 135, 240, BG_COLOR);
      }
      if(tripAdj > -99){
        tripAdj -= 1;
      }
      showAdj = 200;
      }
      writeTripSD(tripPartial);
    }
    holdClick ++;
  }
  // Increase values
  else if (increaseBtnVal == LOW) {
    if (holdClick > 150) {
      tripPartial += 1;
      writeTripSD(tripPartial);
    }
    else if (holdClick > 80) {
      tripPartial += 0.1;
      writeTripSD(tripPartial);
    } else if(holdClick > 10){
      tripPartial += 0.01;
      if(tripAdj == 0){
        tft.fillRect(0, 135, 135, 240, BG_COLOR);
      }
      if(tripAdj < 99){
        tripAdj += 1;
      }
      showAdj = 200;
      writeTripSD(tripPartial);
    }
    holdClick ++;
  }
  else {
    holdClick = 0;
    if(showAdj > 0){
      showAdj -= 1;
    }
  }
}

/*
 * Main loop of the program
 * - Checks for button presses
 * - Updates the GPS values on screen
 * - Updates the clock
 */
void loop() {
  handleButtons();
  printGPSInfo();
  smartDelay(10);
  updateGpsValues();
  printTime();
  printAdj();
}

void printAdj(){
  if(showAdj > 0){
    if(tripAdj < 0){
      tft.setTextColor(TFT_RED, BG_COLOR);
    } else if(tripAdj > 0){
      tft.setTextColor(TFT_GREEN, BG_COLOR);
    } else{
      tft.setTextColor(FG_COLOR, BG_COLOR);
    }
    tft.drawString(String(tripAdj), 0 , 165, 8);
  } else{
    tripAdj = 0;
  }
  if(showAdj == 200){
    tft.setTextColor(FG_COLOR, BG_COLOR);
    tft.drawString("ADJ", 10, 140, 4);
  }

  if(showAdj == 1){
    tft.fillRect(0, 135, 135, 240, BG_COLOR);
    tft.setTextColor(FG_COLOR, BG_COLOR);
    tft.drawString("CAP", 10, 140, 4);
  }

}

/*
 * Method to format CAP (direction) to three digits
 * so that it refreshes correctly on screen
 */
String formatCAP() {
  int cap = tinyGPS.course.deg();
  if (cap < 10) {
    return "00" + String(int(tinyGPS.course.deg()));
  } else if (cap < 100) {
    return "0" + String(int(tinyGPS.course.deg()));
  } else {
    return String(int(tinyGPS.course.deg()));
  }
}

/*
 * Method to format vehicle speed to three digits
 * so that it refreshes correctly on screen
 */
String formatSpeed() {
  int fspeed = tinyGPS.speed.kmph();
  if (fspeed < 10) {
    return "00" + String(fspeed);
  } else if (fspeed < 100) {
    return "0" + String(fspeed);
  } else {
    return String(fspeed);
  }

}

/*
 * Method to print GPS information on screen
 */
void printGPSInfo()
{
  // Main trip
  tft.setTextColor(FG_COLOR, BG_COLOR);
  tft.drawString(String(tripPartial) + "    ", 10 , 40, 8);
  // Vehicle speed
  tft.setTextColor(TFT_NAVY, BG_COLOR);
  tft.drawString(formatSpeed(), 190 ,170, 7);
  tft.setTextColor(FG_COLOR, BG_COLOR);
  // CAP (direction)
  tft.setTextColor(TFT_DARKGREEN, BG_COLOR);
  if(showAdj == 0){
    tft.drawString(formatCAP(), 20 , 170, 7);
  }
  tft.setTextColor(FG_COLOR, BG_COLOR);
  // Number of satellites (red number if the quality is low)
  if (gpsPrecision > 500) {
    tft.setTextColor(TFT_RED, BG_COLOR);
  }
  tft.drawString("  " + String(tinyGPS.satellites.value()), 279 , 6, 2);

  if(gpsPrecision >= 999){
    if (millis() - lastBlinkTime >= blinkInterval) { // Verifica si ha pasado suficiente tiempo
      blinkState = !blinkState; // Cambia el estado de parpadeo
      lastBlinkTime = millis(); // Actualiza el tiempo del último cambio
      if (blinkState) {
        tft.setTextColor(TFT_RED, BG_COLOR); // Primera combinación de colores
      } else {
        tft.setTextColor(BG_COLOR, TFT_RED); // Segunda combinación de colores
      }
      tft.drawString(" NO SIGNAL ",  90, 0, 4);
    }
  }  else {
    tft.setTextColor(BG_COLOR, BG_COLOR);
    tft.drawString("              ",  90, 0, 4);
    tft.setTextColor(FG_COLOR, BG_COLOR);
    tft.drawString("Q: " + String(gpsPrecision) + "        ",  145 , 225, 2);
  }
  tft.setTextColor(FG_COLOR, BG_COLOR);

}
/*
 * Method that ensures that the GPS information is being fed
 */
static void smartDelay(unsigned long ms)
{
  unsigned long start = millis();
  do
  {
    while (gpsSerial.available())
      tinyGPS.encode(gpsSerial.read());
  } while (millis() - start < ms);
}

/*
 * Prints the time in HH:mm:SS format
 */
void printTime(){
  String time = "";
  time = time + (tinyGPS.time.hour() + 2);
  time = time + ":";
  if (tinyGPS.time.minute() < 10) time = time + '0';
  time = time + tinyGPS.time.minute();
  time = time + ":";
  if (tinyGPS.time.second() < 10) time = time + '0';
  time = time + tinyGPS.time.second();
  tft.drawString(String(time), 220 , 120, 4);

}