// PiWars BadgeOS
// 5x5 LED array
// 4 buttons
// Light Sensor
// External connections
//
//////////////////////////////////////////////////
//
// PiWars19 Badge Control Software
//
// 4tronix 2018
//
//////////////////////////////////////////////////

/*
 BadgeOS provides a few functions to make the continuously updated LED display on the badge work without the user having to worry overmuch about it
 The main arduino loop updates the display regularly, checks for button presses and calls the user's code when appropriate
 Users should place all their code in the area provided and use the functions provided.
 It is encouraged to add new characters at the end of the character generator - don't forget to increase the charEnd variable appropriately
*/

#include "FastLED.h"

#define DEBUG         true
//#define REV4

// LED Display 
#define NUM_ROWS  5
#define NUM_COLS  5

#ifdef REV4
byte rows[NUM_ROWS] = {2, 3, 4, 5, 6};  // Rows are active HIGH
byte cols[NUM_COLS] = {11, 10, 9, 8, 7};  // Cols are active LOW
#else
byte rows[NUM_ROWS] = {2, 3, 4, 5, 6};  // Rows are active HIGH
byte cols[NUM_COLS] = {13, 12, 9, 8, 7};  // Cols are active LOW
#endif
bool matrix[NUM_COLS][NUM_ROWS];  // this is mapped to the LEDs for displaying

// Display scanning definitions
int idx=0;
int scan=0;   // Variable to count scan rate for each character ie. time between columns being activated
#define SCANRATE 50 // times round the loop before changing displayed column

// Buttons and Light Sensor
#define NUM_BUTTONS  4 // number of Button inputs
#ifdef REV4
byte buttons[NUM_BUTTONS] = {A0, A1, A2, A3};
#else
byte buttons[NUM_BUTTONS] = {A0, A1, A2, A6}; // Button 3 is analog read only...
#endif
bool lastButton[NUM_BUTTONS] = {false, false, false, false};
#define LIGHTSENSOR A7

// ======== USER CODE =============================================
//
// Put your user code here. The main Arduino setup() and loop() functions handle the 5x5 display, buttons and sensors
//   progSetup() is called once at the start of the program, at the end of the Ardino setup() function, so all I/O is already setup for you
//   progMain() is called every SCANRATE times around the main Arduino loop(). Use this to handle any dynamic functions like scrolling or animation
//   evButtons(int ID, bool state) is called when any button changes state. ID contains the button number (0..3) and state contains a boolean. true for pressed, false for released
//
// There various support functions that you can call to help develop animations:
//   copyChar(character) copies the data for that chareacter from the character generator to the display matrix. eg. copyChar('A')
//   copy2Chars(character1, character2, offset) copies a window on the 2 characters next to each other, starting at offset and finishing after 5 columns. This allows scrolling. eg. cop2Chars('A', 'B', 3) will display the last 2 columns of 'A' and the first 3 of 'B'
//   bool readButton(ID) will read the current state of button ID. eg. readButton(2) will return true if button 2 is currently pressed, else false
//

// ----------------------------------------
// User variables and defines
byte mode; // Defines the display mode: Static, Scrolling characters, animation, dynamic
int cycle=0;  // Variable to count rate of scrolling
#define CYCLERATE 300
#define SCROLLRATE 100
#define ANIRATE 100
#define PIXELS 1
#define NUM_LEDS 14
#define BRIGHT 50
CRGB leds[NUM_LEDS];
String text1 = "LKJIHHLKJIHHMNOPQQRSTUVV";
String text2 = " TITO RULES";
int charIDX=0;
int offset=0; // offset from start of character. Used in scrolling

int L1 = A3, L2 = 10; // Left Motor Drive pins
int R1 = 11, R2 = A4; // Right Motor Drive pins
int mCount=0;
int mState=0;

// Game variables
int gamemode; // 1 = MOVING, 2 = CRASHED
int gamematrix[5][5]; // integer business
int snekdirect, oldsnekdirect; // 1-4 = NESW
int headpos[2];
int newheadpos[2];
int snakelength;
int gamespeed, growtime; // how many ms per game tick
long lasttick, lastgrow; // 32 bit times!
long crash_flashes;

// ----------------------------------------

// progSetup is called once at the start of the program, after all I/O etc has been setup and initialisation is complete
void progSetup()
{
  pinMode(L1, OUTPUT);     
  pinMode(L2, OUTPUT);     
  pinMode(R1, OUTPUT);     
  pinMode(R2, OUTPUT);

  // Set values
  for (int i=0; i<5; i++) {
    for (int j=0; j<5; j++) {
      matrix[i][j] = 0;
      gamematrix[i][j] = 0;
    }
  }  

  gamemode = 1;
  gamematrix[0][0] = 2;     // tail (segments increment in value until >snakelength when they get zeroed)
  gamematrix[1][0] = 1;     // head
  matrix[0][0] = 1;         // set the LEDs on as well
  matrix[1][0] = 1;
  headpos[0] = 1;
  headpos[1] = 0;           // head position
  snakelength = 2;
  snekdirect = 2;           // start off going east
  oldsnekdirect = 2;
  gamespeed = 600;          // ms per movement at the start
  growtime = 7000;          // ms between increasing length/speed
  lasttick = millis()+1500; // pause at the start to let user get their bearings
  lastgrow = millis();      // set the times we last did anything
  crash_flashes = millis(); 

  // Do initial game setup
  gametomatrix; // put game state in framebuffer    
}

// progMain is called each time column address is changed in display
void progMain()
{
  if (millis() > lasttick+gamespeed) {
    lasttick = millis();
    calculateheadmove(); // newheadpos now contains where the head is going to move to
    if (gamemode == 1) { // only do this while we're still alive
      if (gamematrix[newheadpos[0]][newheadpos[1]] > 0 || outofbounds(newheadpos)) {
        crash();
      } else {
        movesnek();
      }
    }
    gametomatrix();
  }
  if (millis() > lastgrow+growtime) {
    // it's time to add a segment and start moving faster
    lastgrow = millis();
    snakelength++;  // Get longer
    gamespeed -= 100; // and faster
  }
  if (gamemode == 2 && millis() > crash_flashes + 3000) {
    progSetup();
  }
}

// evButtons is called every time a button changes state
void evButtons(int ID, bool state)
{
  if (state && snekdirect == oldsnekdirect) // don't change direction twice per turn
  {
    switch (ID)
    {
      case 0: snekdirect--;
        if (snekdirect == 0) snekdirect = 4; break;
      case 3: snekdirect++;
        if (snekdirect == 5) snekdirect = 1; break;
    }
  }
}



void crash(){
  gamemode = 2;
  crash_flashes = millis();
}

void calculateheadmove() {
  newheadpos[0] = headpos[0];
  newheadpos[1] = headpos[1];
  
  switch (snekdirect) { 
    case 1: // NESW
      newheadpos[1] = headpos[1]-1;
      break;
    case 2:
      newheadpos[0] = headpos[0]+1;
      break;
    case 3:
      newheadpos[1] = headpos[1]+1;
      break;
    case 4:
      newheadpos[0] = headpos[0]-1;
      break;
  }
}

bool outofbounds(int* headpos) {
  return (headpos[0] < 0 || headpos[0] > 4 || headpos[1] < 0 || headpos[1] > 4);
}

void movesnek() {
  headpos[0] = newheadpos[0];
  headpos[1] = newheadpos[1];
  for (int i=0; i<5; i++) {
    for (int j=0; j<5; j++) {
      if (gamematrix[i][j] > 0) gamematrix[i][j] = gamematrix[i][j] + 1;
      if (gamematrix[i][j] > snakelength) gamematrix[i][j] = 0;
    }
  }
  gamematrix[headpos[0]][headpos[1]] = 1; // place head
  oldsnekdirect = snekdirect;
}

void gametomatrix() {
  for (int i=0; i<5; i++) {
    for (int j=0; j<5; j++) {
      if (gamemode == 1) {
        matrix[i][j] = (gamematrix[i][j] > 0);
      } else {
        matrix[i][j] = (!matrix[i][j]); // flash on crash
      }
    }
  }  
}



// ======== END OF USER CODE ======================================
// You shouldn't have to touch any code from here on


// ======== BadgeOS ===============================================

int osReadLight()
{
  return analogRead(LIGHTSENSOR);
}

bool osReadButton (int ID)
{
  if (buttons[ID] == A6)
    return (analogRead(buttons[ID]) < 512);
  else
    return !digitalRead(buttons[ID]);
}

void setup()
{
  for (int i=0; i<NUM_ROWS; i++)
  {
    pinMode(rows[i], OUTPUT);
    digitalWrite(rows[i], LOW); // disable all rows
  }
  for (int i=0; i<NUM_COLS; i++)
  {
    pinMode(cols[i], OUTPUT);
    digitalWrite(cols[i], HIGH);  // disable all columns
  }
  for (int i=0; i<NUM_BUTTONS; i++)
    pinMode(buttons[i], INPUT_PULLUP);
  if (DEBUG)
  {
    Serial.begin(115200);
    Serial.println("Starting...");
    delay(1000);
  }

  progSetup();  // call user's setup function
}

// The main loop continuously displays whatever is in the 5x5 buffer "screen"
// Every 50 (TBC) times around the loop, buttons and sensor(s) are checked and relevant flags or data stored
// After checking buttons and sensors the "progMain" function is called to handle things like changing or scrolling characters, reacting to buttons etc.
// The progMain function is the main user function in which the user's work is done. Try to consume as little time as possible in the function or you may see the display start to flicker
void loop()
{
  bool button;
  delayMicroseconds(10);
  if (++scan > 99)  // change column every hundred loops
  {
    digitalWrite(cols[idx], HIGH);  // disable whatever column we're on right now
    idx = (++idx) % NUM_COLS;
    setCol(idx);
    progMain(); // call user's main function
    scan = 0;
    // check for button changes
    for (int i=0; i<NUM_BUTTONS; i++)
    {
      button = osReadButton(i);
      if (button != lastButton[i])
      {
        lastButton[i] = button;
        evButtons(i, button);
      }
    }
  }
}

void setCol(int col)
{
  digitalWrite(cols[col], HIGH);  // switch off the column, then change the data
  for (int i=0; i<NUM_ROWS; i++)
    digitalWrite(rows[i], matrix[col][i]);
  digitalWrite(cols[col], LOW);   // switch ON the column  
}

void flashCol1()
{
  digitalWrite(cols[0], HIGH);  // switch off the column
  digitalWrite(rows[0], HIGH);  // change the data
  digitalWrite(rows[1], LOW);
  digitalWrite(rows[2], HIGH);
  digitalWrite(rows[3], LOW);
  digitalWrite(rows[4], HIGH);
  digitalWrite(cols[0], LOW);   // switch ON the column
  delay(1000);
  digitalWrite(cols[0], HIGH);  // switch off the column
  digitalWrite(rows[0], LOW);  // change the data
  digitalWrite(rows[1], HIGH);
  digitalWrite(rows[2], LOW);
  digitalWrite(rows[3], HIGH);
  digitalWrite(rows[4], LOW);
  digitalWrite(cols[0], LOW);   // switch ON the column
  delay(1000);
}
