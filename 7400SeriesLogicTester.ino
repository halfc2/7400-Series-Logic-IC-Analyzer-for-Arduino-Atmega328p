/*
   Arduino 74xx Series Quad 2-Input Logic Gate Tester
   Supports: 7400(NAND), 7402(NOR), 7408(AND), 7432(OR), 7486(XOR), 74266(XNOR)
   Hardware: Arduino Nano/Uno (ATmega328P) + 14-pin ZIF socket + SSD1306 128x64 I2C OLED

   Author: Alfred Gabriel (halfc2)
   Year:   2023
   License: MIT
*/

#include "U8glib.h"

// OLED display (I2C, no special options needed)
U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NONE);

// ------------------------------------------------------------------
// Global state variables
// ------------------------------------------------------------------
bool testDone           = false;  // Prevents re-testing until IC is removed/re-inserted
bool badGateDetected    = false;  // At least one gate did not match any known truth table
int  noGateScore        = 0;      // Counts how many gates returned "unknown" in current mode

// Optional manual re-test button on A3 (pull-down resistor recommended)
const int retestPin = A3;

// ------------------------------------------------------------------
// Arduino → 74xx pin mapping (hard-wired in hardware!)
// ------------------------------------------------------------------
const int constPins[4] = { 3, 6, 9, 12 };   // Always used as one of the drivers (shared across gates)

// MODE 1 – drives the "left" inputs, reads the "right" outputs
int mode1Pins[4] = { 2, 5, 10, 13 };

// MODE 2 – drives the "right" inputs, reads the "left" outputs (used for NOR/XNOR detection)
int mode2Pins[4] = { 4, 7, 8, 11 };

// MODE 3 – currently unused in final logic but kept for possible future extensions
int mode3OPins[4] = { 2, 7, 8, 13 };
int mode3IPins[4] = { 4, 5, 10, 11 };

// Runtime arrays – filled by the active MODE functions
int outputPin1[4] = {0};   // First driver pin for current mode
int outputPin2[4] = {0};   // Second driver pin for current mode
int inputPin[4]   = {0};   // Pin where we read the gate output

// Truth-table results for each gate (True = HIGH observed)
bool gateTT[4] = {false};  // 11
bool gateTF[4] = {false};  // 10
bool gateFT[4] = {false};  // 01
bool gateFF[4] = {false};  // 00

// Detected logic type per gate (0 = unknown/bad)
int logicType[4] = {0, 0, 0, 0};

/*
   Logic type codes used in the original project:
   1 = AND
   2 = OR
   3 = NAND
   4 = XOR
   5 = NOR
   6 = XNOR
   0 = Bad / unknown
*/
String printLogicType[4] = {"", "", "", ""};
String printICType = "INSERT IC";

// ------------------------------------------------------------------
// Setup
// ------------------------------------------------------------------
void setup()
{
  Serial.begin(9600);

  pinMode(retestPin, INPUT);                     // Button to force a new test
  for (int i = 0; i < 4; i++) {
    pinMode(constPins[i], OUTPUT);               // These pins are always outputs
    digitalWrite(constPins[i], LOW);             // Start low to avoid floating
  }
}

// ------------------------------------------------------------------
// Main loop
// ------------------------------------------------------------------
void loop()
{
  // Pressing the re-test button (or pulling A3 high) resets everything
  if (digitalRead(retestPin) == HIGH) {
    resetTestState();
  }

  if (!testDone) {
    // Start with the most common configuration
    MODE1();
    getGateState();            // Fill the 4 truth-table entries for each gate
    checkBadGates();           // Sets badGateDetected flag
    checkGateType();           // Try to recognise each gate
    checkGateDetectedScore();  Count how many gates are still unknown

    // If every gate looks wrong in MODE1 → probably a NOR or XNOR IC
    if (badGateDetected && noGateScore == 4) {
      Serial.println(F("All gates unknown in MODE1 → trying MODE2 (NOR/XNOR test)"));
      resetPerModeState();
      MODE2();
      getGateState();
      checkBadGates();
      checkGateType();
      checkGateDetectedScore();

      // Still everything unknown OR we already detected some NOR gates → try XNOR special case
      if (badGateDetected && noGateScore == 4 || logicType[1] == 5 || logicType[2] == 5) {
        Serial.println(F("Still unknown or NOR detected → trying XNOR test"));
        resetPerModeState();
        MODE2();                 // MODE2 again is intentional in original code
        getGateState();
        checkBadGates();
        checkGateType();
      }
    }

    Serial.println(F("Final evaluation..."));
    printBadGates();             // "OK" or "BAD" on OLED
    checkICType();               // Decide which 74xx model it is
    printState();                // Update OLED
    delay(500);
  }

  testDone = true;               // Wait for removal or button press
}

// ------------------------------------------------------------------
// Mode configuration functions
// ------------------------------------------------------------------
void MODE1()
{
  for (int i = 0; i < 4; i++) {
    pinMode(mode1Pins[i], OUTPUT);
    pinMode(mode2Pins[i], INPUT);
    outputPin1[i] = constPins[i];
    outputPin2[i] = mode1Pins[i];
    inputPin[i]   = mode2Pins[i];
  }
}

void MODE2()
{
  for (int i = 0; i < 4; i++) {
    pinMode(mode2Pins[i], OUTPUT);
    pinMode(mode1Pins[i], INPUT);
    outputPin1[i] = constPins[i];
    outputPin2[i] = mode2Pins[i];
    inputPin[i]   = mode1Pins[i];
  }
}

// Not used in current algorithm but kept for completeness
void MODE3()
{
  for (int i = 0; i < 4; i++) {
    pinMode(mode3OPins[i], OUTPUT);
    pinMode(mode3IPins[i], INPUT);
    outputPin1[i] = constPins[i];
    outputPin2[i] = mode3OPins[i];
    inputPin[i]   = mode3IPins[i];
  }
}

// ------------------------------------------------------------------
// Truth table acquisition
// ------------------------------------------------------------------
void getGateState()
{
  // Reset truth table flags
  for (int i = 0; i < 4; i++) {
    gateTT[i] = gateTF[i] = gateFT[i] = gateFF[i] = false;
  }

  // 11
  for (int i = 0; i < 4; i++) {
    digitalWrite(outputPin1[i], HIGH);
    digitalWrite(outputPin2[i], HIGH);
    gateTT[i] = (digitalRead(inputPin[i]) == HIGH);
  }

  // 10
  for (int i = 0; i < 4; i++) {
    digitalWrite(outputPin1[i], HIGH);
    digitalWrite(outputPin2[i], LOW);
    gateTF[i] = (digitalRead(inputPin[i]) == HIGH);
  }

  // 01
  for (int i = 0; i < 4; i++) {
    digitalWrite(outputPin1[i], LOW);
    digitalWrite(outputPin2[i], HIGH);
    gateFT[i] = (digitalRead(inputPin[i]) == HIGH);
  }

  // 00
  for (int i = 0; i < 4; i++) {
    digitalWrite(outputPin1[i], LOW);
    digitalWrite(outputPin2[i], LOW);
    gateFF[i] = (digitalRead(inputPin[i]) == HIGH);
  }
}

// ------------------------------------------------------------------
// Gate recognition
// ------------------------------------------------------------------
void checkGateType()
{
  for (int i = 0; i < 4; i++) {
    if      (gateTT[i] && !gateTF[i] && !gateFT[i] && !gateFF[i]) logicType[i] = 1; // AND
    else if (gateTT[i] &&  gateTF[i] &&  gateFT[i] && !gateFF[i]) logicType[i] = 2; // OR
    else if (!gateTT[i] && gateTF[i] && gateFT[i] && gateFF[i])  logicType[i] = 3; // NAND
    else if (!gateTT[i] && gateTF[i] && gateFT[i] && !gateFF[i]) logicType[i] = 4; // XOR
    else if (!gateTT[i] && !gateTF[i] && !gateFT[i] && gateFF[i]) logicType[i] = 5; // NOR
    else if (gateTT[i] && !gateTF[i] && !gateFT[i] && gateFF[i]) logicType[i] = 6; // XNOR
    else                                                        logicType[i] = 0; // Bad / unknown
  }
}

// ------------------------------------------------------------------
// Helper functions
// ------------------------------------------------------------------
void resetTestState()
{
  testDone = false;
  badGateDetected = false;
  noGateScore = 0;
  printICType = "INSERT IC";
  for (int i = 0; i < 4; i++) logicType[i] = 0;
}

void resetPerModeState()
{
  badGateDetected = false;
  noGateScore = 0;
  for (int i = 0; i < 4; i++) {
    gateTT[i] = gateTF[i] = gateFT[i] = gateFF[i] = false;
    logicType[i] = 0;
  }
}

void checkBadGates()
{
  badGateDetected = false;
  for (int i = 0; i < 4; i++) {
    if (logicType[i] == 0) {
      badGateDetected = true;
      break;
    }
  }
}

void checkGateDetectedScore()
{
  noGateScore = 0;
  for (int i = 0; i < 4; i++) {
    if (logicType[i] == 0) noGateScore++;
  }
  Serial.print(F("noGateScore = "));
  Serial.println(noGateScore);
}

void printBadGates()
{
  for (int i = 0; i < 4; i++) {
    printLogicType[i] = (logicType[i] == 0) ? "BAD" : "OK";
  }
}

void checkICType()
{
  // Reset to default
  printICType = "UNKNOWN";

  // All gates must agree on the same type (ignore bad gates)
  int firstValidType = -1;
  bool consistent = true;

  for (int i = 0; i < 4; i++) {
    if (logicType[i] == 0) continue;           // skip bad gates
    if (firstValidType == -1) {
      firstValidType = logicType[i];
    } else if (logicType[i] != firstValidType) {
      consistent = false;
    }
  }

  if (!consistent || firstValidType == -1) {
    printICType = "MIXED / BAD";
    return;
  }

  switch (firstValidType) {
    case 1: printICType = "7408 - AND";      break;
    case 2: printICType = "7432 - OR";       break;
    case 3: printICType = "7400 - NAND";      break;
    case 4: printICType = "7486 - XOR";      break;
    case 5: printICType = "7402 - NOR";      break;
    case 6: printICType = "74266 - XNOR";    break;
  // open-collector version
  }
}

// ------------------------------------------------------------------
// OLED output
// ------------------------------------------------------------------
void printState()
{
  u8g.firstPage();
  do {
    u8g.setFont(u8g_font_helvR08);
    u8g.setPrintPos(1, 10);
    u8g.print(F("IC TYPE: "));
    u8g.setPrintPos(50, 10);
    u8g.print(printICType);

    u8g.setPrintPos(1, 20);
    u8g.print(F("GATE 1: "));
    u8g.setPrintPos(45, 20);
    u8g.print(printLogicType[0]);

    u8g.setPrintPos(1, 30);
    u8g.print(F("GATE 2: "));
    u8g.setPrintPos(45, 30);
    u8g.print(printLogicType[1]);

    u8g.setPrintPos(1, 40);
    u8g.print(F("GATE 3: "));
    u8g.setPrintPos(45, 40);
    u8g.print(printLogicType[2]);

    u8g.setPrintPos(1, 50);
    u8g.print(F("GATE 4: "));
    u8g.setPrintPos(45, 50);
    u8g.print(printLogicType[3]);
  } while (u8g.nextPage());
}