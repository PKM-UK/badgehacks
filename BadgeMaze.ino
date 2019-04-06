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

#define DEBUG         false
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
bool mazematrix[NUM_COLS][NUM_ROWS];

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
#define BRIGHT 80
CRGB leds[NUM_LEDS];
String text1 = "LKJIHHLKJIHHMNOPQQRSTUVV";
String text2 = " TITO RULES";
int charIDX=0;
int offset=0; // offset from start of character. Used in scrolling

int L1 = A3, L2 = 10; // Left Motor Drive pins
int R1 = 11, R2 = A4; // Right Motor Drive pins
int mCount=0;
int mState=0;
int ii=4, jj=4;
long flash_timer;

// ----------------------------------------

// progSetup is called once at the start of the program, after all I/O etc has been setup and initialisation is complete
void progSetup()
{
  pinMode(L1, OUTPUT);     
  pinMode(L2, OUTPUT);     
  pinMode(R1, OUTPUT);     
  pinMode(R2, OUTPUT);
  
  mazematrix[3][4] = 1;
  mazematrix[3][3] = 1;
  mazematrix[3][2] = 1;
  mazematrix[3][1] = 1;
  mazematrix[2][1] = 1;
  mazematrix[1][1] = 1;
  mazematrix[1][2] = 1;
  mazematrix[1][4] = 1;
  mazematrix[4][5] = 1;

  matrix[3][4] = 1;
  matrix[3][3] = 1;
  matrix[3][2] = 1;
  matrix[3][1] = 1;
  matrix[2][1] = 1;
  matrix[1][1] = 1;
  matrix[1][2] = 1;
  matrix[1][4] = 1;
  matrix[4][5] = 1;
  matrix[0][0] = 0; 

  flash_timer = millis();
}

// progMain is called each time column address is changed in display
void progMain()
{
  if ( (millis()-flash_timer) < 500) {
    matrix[ii][jj]=1;
  } else {
    matrix[ii][jj]=0;
  }
  if ( (millis()-flash_timer) > 600) {
    flash_timer = millis();
  }
}

// evButtons is called every time a button changes state
void evButtons(int ID, bool state)
{
  if (state)
  {
    mode = ID;
    matrix[ii][jj]=0;
    switch (mode)
    {
      case 0: if (mazematrix[ii-1][jj] == 0) ii--; if (ii>4) {ii=4;} if (ii<0) {ii=0;} break;
      case 1: if (mazematrix[ii][jj+1] == 0) jj++; if (jj>4) {jj=4;} if (jj<0) {jj=0;} break;
      case 2: if (mazematrix[ii][jj-1] == 0) jj--; if (jj>4) {jj=4;} if (jj<0) {jj=0;} break;
      case 3: if (mazematrix[ii+1][jj] == 0) ii++; if (ii>4) {ii=4;} if (ii<0) {ii=0;} break;
    }
  }
}

// ======== END OF USER CODE ======================================
// You shouldn't have to touch any code from here on


// ======== BadgeOS ===============================================

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
