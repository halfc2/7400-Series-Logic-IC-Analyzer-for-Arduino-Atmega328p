/*
   Arduino 74xx Series Quad 2-Input Logic Gate Tester
   Supports: 7400(NAND), 7402(NOR), 7408(AND), 7432(OR), 7486(XOR), 74266(XNOR)
   Hardware: Arduino Nano/Uno (ATmega328P) + 14-pin ZIF socket + SSD1306 128x64 I2C OLED

   Author: Alfred Gabriel (halfc2)
   Year:   2023
   License: MIT
*/

#include "U8glib.h"
U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NONE);  // I2C OLED, no special options

// =====================================================================
// Global state flags
// =====================================================================
bool testDone = false;          // Becomes true after one full test → prevents re-testing until reset
bool badGateDetected = false;   // True if at least one gate didn't match any known logic function
int noGateScore = 0;            // Counts how many gates were unrecognized in current test mode

// =====================================================================
// Pin assignments – MUST match your physical wiring!
// =====================================================================
int retestPin = A3;                             // Pull A3 high (button to 5V) to force new test
const int constPins[4] = { 3, 6, 9, 12 };       // Always used as output drivers (shared inputs)
int mode1Pins[4] = { 2, 5, 10, 13 };             // Left-side pins – used as outputs in MODE1
int mode2Pins[4] = { 4, 7, 8, 11 };              // Right-side pins – used as outputs in MODE2
int mode3OPins[4] = { 2, 7, 8, 13 };             // Not used in final logic but kept for completeness
int mode3IPins[4] = { 4, 5, 10, 11 };

// Runtime pin mapping – filled by MODE1/MODE2/MODE3
int outputPin1[4] = { 0, 0, 0, 0 };  // Driver pin A for current mode
int outputPin2[4] = { 0, 0, 0, 0 };  // Driver pin B for current mode
int inputPin[4]   = { 0, 0, 0, 0 };  // Pin where we read the gate output

// =====================================================================
// Truth table results for each of the 4 gates
// =====================================================================
bool gateTT[4] = { false, false, false, false };  // Input 11 → output ?
bool gateTF[4] = { false, false, false, false };  // Input 10 → output ?
bool gateFT[4] = { false, false, false, false };  // Input 01 → output ?
bool gateFF[4] = { false, false, false, false };  // Input 00 → output ?

int logicType[4] = { 0, 0, 0, 0 };  // 0=bad/unknown, 1=AND, 2=OR, 3=NAND, 4=XOR, 5=NOR, 6=XNOR

// =====================================================================
// Display strings
// =====================================================================
String printLogicType[4] = { "", "", "", "" };  // "OK" or "OK" or "BAD" shown on OLED
String printICType = "INSERT IC";               // Final detected IC model

// =====================================================================
// Setup
// =====================================================================
void setup() {
  Serial.begin(9600);
  pinMode(retestPin, INPUT);                    // No pull-up needed if button connects to 5V
  for (int i = 0; i < 4; i++) {
    pinMode(constPins[i], OUTPUT);              // These pins are always outputs
    digitalWrite(constPins[i], LOW);            // Start low – prevents floating inputs
  }
}

// =====================================================================
// Main loop
// =====================================================================
void loop() {
  // Manual retest button – pulling A3 high resets everything
  if (digitalRead(retestPin) == HIGH) {
    testDone = false;
    badGateDetected = false;
    noGateScore = 0;
    printICType = "INSERT IC";
    logicType[0] = logicType[1] = logicType[2] = logicType[3] = 0;
  }

  // Run test only once per insertion
  if (testDone == false) {

    // === First attempt: normal pinout (AND/OR/NAND/XOR) ===
    MODE1();
    getGateState();            // Fill truth tables
    checkBadGates();           // Any gate completely wrong?
    checkGateType();           // Try to recognize each gate
    checkGateDetectedScore();  // How many gates still unknown?

    // If ALL 4 gates failed → probably a NOR or XNOR IC (inverted logic)
    if (badGateDetected == true && noGateScore == 4) {
      noGateScore = 0;
      badGateDetected = false;
      Serial.println(F("Testing for NOR..."));

      // Likely 7402"));
      MODE2();                   // Drive the other set of inputs
      getGateState();
      checkBadGates();
      checkGateType();
      checkGateDetectedScore();

      // Still all bad OR we already saw some XNOR behavior → try XNOR special case
      if (badGateDetected == true && noGateScore == 4 || logicType[1] == 6 || logicType[2] == 6) {
        noGateScore = 0;
        badGateDetected = false;
        Serial.println(F("Testing for XNOR...     // Likely 74266"));
        MODE2();                 // Yes, MODE2 again – intentional in original algorithm
        getGateState();
        checkBadGates();
        checkGateType();
      }
    }

    // Final steps
    Serial.println(F("Checking Gate Type..."));
    printBadGates();           // Convert logicType → "OK"/"BAD" for display
    checkICType();             // Decide which 74xx model it is
    printState();              // Update OLED
    delay(500);
  }

  testDone = true;  // Wait for removal or button press
}

// =====================================================================
// Mode configuration – sets which pins drive vs read
// =====================================================================
void MODE1() {
  for (int i = 0; i < 4; i++) {
    pinMode(mode1Pins[i], OUTPUT);
    pinMode(mode2Pins[i], INPUT);
    outputPin1[i] = constPins[i];
    outputPin2[i] = mode1Pins[i];
    inputPin[i]   = mode2Pins[i];
  }
}

void MODE2() {
  for (int i = 0; i < 4; i++) {
    pinMode(mode2Pins[i], OUTPUT);
    pinMode(mode1Pins[i], INPUT);
    outputPin1[i] = constPins[i];
    outputPin2[i] = mode2Pins[i];
    inputPin[i]   = mode1Pins[i];
  }
}

void MODE3() {  // Currently unused but kept in case of future expansion
  for (int i = 0; i < 4; i++) {
    pinMode(mode3OPins[i], OUTPUT);
    pinMode(mode3IPins[i], INPUT);
    outputPin1[i] = constPins[i];
    outputPin2[i] = mode3OPins[i];
    inputPin[i]   = mode3IPins[i];
  }
}

// =====================================================================
// Truth table acquisition – tests all 4 input combinations
// =====================================================================
void getGateState() {
  // Reset previous results
  memset(gateTT, 0, sizeof(gateTT));
  memset(gateTF, 0, sizeof(gateTF));
  memset(gateFT, 0, sizeof(gateFT));
  memset(gateFF, 0, sizeof(gateFF));

  // 11
  for (int i = 0; i < 4; i++) {
    digitalWrite(outputPin1[i], HIGH);
    digitalWrite(outputPin2[i], HIGH);
    gateTT[i] = digitalRead(inputPin[i]);
  }

  // 10
  for (int i = 0; i < 4; i++) {
    digitalWrite(outputPin1[i], HIGH);
    digitalWrite(outputPin2[i], LOW);
    gateTF[i] = digitalRead(inputPin[i]);
  }

  // 01
  for (int i = 0; i < 4; i++) {
    digitalWrite(outputPin1[i], LOW);
    digitalWrite(outputPin2[i], HIGH);
    gateFT[i] = digitalRead(inputPin[i]);
  }

  // 00
  for (int i = 0; i < 4; i++) {
    digitalWrite(outputPin1[i], LOW);
    digitalWrite(outputPin2[i], LOW);
    gateFF[i] = digitalRead(inputPin[i]);
  }
}

// =====================================================================
// Gate recognition based on truth table
// =====================================================================
void checkGateType() {
  for (int i = 0; i < 4; i++) {
    if      (gateTT[i] && !gateTF[i] && !gateFT[i] && !gateFF[i]) logicType[i] = 1; // AND
    else if (gateTT[i] &&  gateTF[i] &&  gateFT[i] && !gateFF[i]) logicType[i] = 2; // OR
    else if (!gateTT[i] && gateTF[i] && gateFT[i] && gateFF[i])  logicType[i] = 3; // NAND
    else if (!gateTT[i] && gateTF[i] && gateFT[i] && !gateFF[i]) logicType[i] = 4; // XOR
    else if (!gateTT[i] && !gateTF[i] && !gateFT[i] && gateFF[i]) logicType[i] = 5; // NOR
    else if (gateTT[i] && !gateTF[i] && !gateFT[i] && gateFF[i]) logicType[i] = 6; // XNOR
    else                                                        logicType[i] = 0; // Bad/unknown
  }
}

// =====================================================================
// OLED display update
// =====================================================================
void printState() {
  u8g.firstPage();
  do {
    u8g.setFont(u8g_font_helvR08);
    u8g.setPrintPos(1, 10);
    u8g.print(F("IC TYPE: "));
    u8g.setPrintPos(50, 10);
    u8g.print(printICType);

    u8g.setPrintPos(1, 20); u8g.print(F("GATE 1: ")); u8g.setPrintPos(45, 20); u8g.print(printLogicType[0]);
    u8g.setPrintPos(1, 30); u8g.print(F("GATE 2: ")); u8g.setPrintPos(45, 30); u8g.print(printLogicType[1]);
    u8g.setPrintPos(1, 40); u8g.print(F("GATE 3: ")); u8g.setPrintPos(45, 40); u8g.print(printLogicType[2]);
    u8g.setPrintPos(1, 50); u8g.print(F("GATE 4: ")); u8g.setPrintPos(45, 50); u8g.print(printLogicType[3]);
  } while (u8g.nextPage());
}

// =====================================================================
// Helper functions
// =====================================================================
void printBadGates() {
  for (int i = 0; i < 4; i++) {
    printLogicType[i] = (logicType[i] == 0) ? "BAD" : "OK";
  }
}

void checkBadGates() {
  badGateDetected = false;
  // reset flag
  for (int i = 0; i < 4; i++) {
    if (logicType[i] == 0) {
      badGateDetected = true;
      break;
    }
  }
}

void checkGateDetectedScore() {
  noGateScore = 0;
  for (int i = 0; i < 4; i++) {
    if (logicType[i] == 0) noGateScore++;
  }
  Serial.println(noGateScore);
}

void checkICType() {
  printICType = "";  // clear previous

  int foundType =  = 0;
  for (int i = 0; i < 4; i++) {
    if (logicType[i] != 0) {
      if (foundType == 0) foundType = logicType[i];           // remember first good gate
      else if (logicType[i] != foundType) foundType = -1;     // mixed → invalid
    }
  }

  if (foundType > 0) {
    switch (foundType) {
      case 1: printICType = "7408 - AND";   break;
      case 2: printICType = "7432 - OR";    break;
      case 3: printICType = "7400 - NAND";  break;
      case 4: printICType = "7486 - XOR";   break;
      case 5: printICType = "7402 - NOR";   break;
      case 6: printICType = "74266 - XNOR"; break;
    }
  } else {
    printICType = "UNKNOWN / BAD";
  }
}
