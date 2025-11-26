/*
   Arduino 74xx Series Quad 2-Input Logic Gate Tester
   Supports: 7400(NAND), 7402(NOR), 7408(AND), 7432(OR), 7486(XOR), 74266(XNOR)
   Hardware: Arduino Nano/Uno (ATmega328P) + 14-pin ZIF socket + SSD1306 128x64 I2C OLED

   Author: Alfred Gabriel
   Year:   2023
   License: MIT
*/

#include "U8glib.h"
U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NONE); // OLED display initialization

// Global variables to track testing status
bool testDone = false;        // Indicates if testing is complete
bool badGateDetected = false; // Flag for detecting faulty gates
int noGateScore = 0;          // Counter for undetected gates

// Pin definitions
int retestPin = A3;           // Analog pin for retest button
const int constPins[4] = { 3, 6, 9, 12 };  // Constant pins (always outputs)
int mode1Pins[4] = { 2, 5, 10, 13 };       // Pins used in mode 1
int mode2Pins[4] = { 4, 7, 8, 11 };        // Pins used in mode 2
int mode3OPins[4] = { 2, 7, 8, 13 };       // Output pins in mode 3
int mode3IPins[4] = { 4, 5, 10, 11 };      // Input pins in mode 3

// Pin assignment arrays for current mode
int outputPin1[4] = { 0, 0, 0, 0 };  // First output pins for each gate
int outputPin2[4] = { 0, 0, 0, 0 };  // Second output pins for each gate
int inputPin[4] = { 0, 0, 0, 0 };    // Input pins for each gate

// Gate state and type tracking
int gateState[4] = { 0, 0, 0, 0 };   // Current state of each gate
int logicType[4] = { 0, 0, 0, 0 };   // Detected logic type (0=unknown, 1=AND, etc.)

// Truth table values for each gate
bool gateTT[4] = { false, false, false, false }; // Input (1,1) -> Output
bool gateTF[4] = { false, false, false, false }; // Input (1,0) -> Output
bool gateFT[4] = { false, false, false, false }; // Input (0,1) -> Output
bool gateFF[4] = { false, false, false, false }; // Input (0,0) -> Output

// Display strings
String printLogicType[4] = { "", "", "", "" }; // Logic type strings for display
String printICType = "INSERT IC";              // IC type string for display

void setup() {
  Serial.begin(9600);        // Initialize serial communication
  pinMode(retestPin, INPUT); // Set retest button as input
  
  // Configure constant pins as outputs
  for (int i = 0; i < 4; i++) {
    pinMode(constPins[i], OUTPUT);
  }
}

void loop() {
  // Check if retest button is pressed
  if (digitalRead(retestPin) == HIGH) {
    // Reset all testing variables
    testDone = false;
    badGateDetected = false;
    noGateScore = 0;
    printICType = "INSERT IC";
    logicType[0] = 0;
    logicType[1] = 0;
    logicType[2] = 0;
    logicType[3] = 0;
  }

  // Main testing routine
  if (testDone == false) {
    MODE1();                  // Configure pins for mode 1
    getGateState();           // Test all input combinations
    checkBadGates();          // Check for faulty gates
    checkGateType();          // Determine gate types
    checkGateDetectedScore(); // Count undetected gates

    // If bad gates detected or all gates undetected, try mode 2
    if (badGateDetected == true && noGateScore == 4) {
      noGateScore = 0;
      badGateDetected = false;
      Serial.println(F("Testing for NOR..."));
      MODE2();                  // Configure pins for mode 2
      getGateState();           // Test all input combinations
      checkBadGates();          // Check for faulty gates
      checkGateType();          // Determine gate types
      checkGateDetectedScore(); // Count undetected gates

      // If still having issues, try mode 2 again for XNOR detection
      if (badGateDetected == true && noGateScore == 4 || (logicType[1] == 6 || logicType[2] == 6)) {
        noGateScore = 0;
        badGateDetected = false;
        Serial.println(F("Testing for XNOR..."));
        MODE2();                  // Configure pins for mode 2
        getGateState();           // Test all input combinations
        checkBadGates();          // Check for faulty gates
        checkGateType();          // Determine gate types
      }
    }
    
    // Display results
    Serial.println(F("Checking Gate Type..."));
    printBadGates();           // Update display strings for gate status
    checkICType();             // Determine IC type based on gates
    printState();              // Update OLED display
    delay(500);                // Short delay before next test
  }
  testDone = true;             // Mark testing as complete
}

// Configure pins for mode 1 testing
void MODE1() {
  for (int i = 0; i < 4; i++) {
    pinMode(mode1Pins[i], OUTPUT);  // Set mode1 pins as outputs
    pinMode(mode2Pins[i], INPUT);   // Set mode2 pins as inputs
    
    // Assign pins for testing
    outputPin1[i] = constPins[i];   // First output from constant pins
    outputPin2[i] = mode1Pins[i];   // Second output from mode1 pins
    inputPin[i] = mode2Pins[i];     // Input from mode2 pins
  }
}

// Configure pins for mode 2 testing
void MODE2() {
  for (int i = 0; i < 4; i++) {
    pinMode(mode2Pins[i], OUTPUT);  // Set mode2 pins as outputs
    pinMode(mode1Pins[i], INPUT);   // Set mode1 pins as inputs
    
    // Assign pins for testing
    outputPin1[i] = constPins[i];   // First output from constant pins
    outputPin2[i] = mode2Pins[i];   // Second output from mode2 pins
    inputPin[i] = mode1Pins[i];     // Input from mode1 pins
  }
}

// Configure pins for mode 3 testing (not used in main loop)
void MODE3() {
  for (int i = 0; i < 4; i++) {
    pinMode(mode3OPins[i], OUTPUT); // Set mode3 output pins as outputs
    pinMode(mode3IPins[i], INPUT);  // Set mode3 input pins as inputs
    
    // Assign pins for testing
    outputPin1[i] = constPins[i];   // First output from constant pins
    outputPin2[i] = mode3OPins[i];  // Second output from mode3 output pins
    inputPin[i] = mode3IPins[i];    // Input from mode3 input pins
  }
}

// Determine gate type based on truth table results
void checkGateType() {
  for (int i = 0; i < 4; i++) {
    // AND gate: only true when both inputs are true
    if (gateTT[i] == true && gateTF[i] == false && gateFT[i] == false && gateFF[i] == false) {
      Serial.println(F("AND GATE "));
      logicType[i] = 1;
    } 
    // OR gate: true when at least one input is true
    else if (gateTT[i] == true && gateTF[i] == true && gateFT[i] == true && gateFF[i] == false) {
      Serial.println(F("OR GATE "));
      logicType[i] = 2;
    } 
    // NAND gate: false only when both inputs are true
    else if (gateTT[i] == false && gateTF[i] == true && gateFT[i] == true && gateFF[i] == true) {
      Serial.println(F("NAND GATE "));
      logicType[i] = 3;
    } 
    // XOR gate: true when inputs are different
    else if (gateTT[i] == false && gateTF[i] == true && gateFT[i] == true && gateFF[i] == false) {
      Serial.println(F("XOR GATE "));
      logicType[i] = 4;
    } 
    // NOR gate: true only when both inputs are false
    else if (gateTT[i] == false && gateTF[i] == false && gateFT[i] == false && gateFF[i] == true) {
      Serial.println(F("NOR GATE "));
      logicType[i] = 5;
    } 
    // XNOR gate: true when inputs are the same
    else if (gateTT[i] == true && gateTF[i] == false && gateFT[i] == false && gateFF[i] == true) {
      Serial.println(F("XNOR GATE "));
      logicType[i] = 6;
    } 
    // Unknown or faulty gate
    else {
      Serial.println(F("Bad Gate Detected "));
      logicType[i] = 0;
    }
  }
}

// Test all input combinations for each gate
void getGateState() {
  // Test (1,1) input combination
  for (int i = 0; i < 4; i++) {
    digitalWrite(outputPin1[i], HIGH);
    digitalWrite(outputPin2[i], HIGH);
    gateState[i] = digitalRead(inputPin[i]);
    gateTT[i] = (gateState[i] == HIGH);
  }

  // Test (1,0) input combination
  for (int i = 0; i < 4; i++) {
    digitalWrite(outputPin1[i], HIGH);
    digitalWrite(outputPin2[i], LOW);
    gateState[i] = digitalRead(inputPin[i]);
    gateTF[i] = (gateState[i] == HIGH);
  }

  // Test (0,1) input combination
  for (int i = 0; i < 4; i++) {
    digitalWrite(outputPin1[i], LOW);
    digitalWrite(outputPin2[i], HIGH);
    gateState[i] = digitalRead(inputPin[i]);
    gateFT[i] = (gateState[i] == HIGH);
  }

  // Test (0,0) input combination
  for (int i = 0; i < 4; i++) {
    digitalWrite(outputPin1[i], LOW);
    digitalWrite(outputPin2[i], LOW);
    gateState[i] = digitalRead(inputPin[i]);
    gateFF[i] = (gateState[i] == HIGH);
  }
}

// Update OLED display with current test results
void printState() {
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

// Update display strings for gate status
void printBadGates() {
  for (int i = 0; i < 4; i++) {
    if (logicType[i] == 0) {
      printLogicType[i] = "BAD";  // Mark as bad if type is unknown
    } else {
      printLogicType[i] = "OK";   // Mark as OK if type is known
    }
  }
}

// Check if any bad gates were detected
void checkBadGates() {
  for (int i = 0; i < 4; i++) {
    if (logicType[i] == 0) {
      badGateDetected = true;
      break;
    }
  }
}

// Count how many gates were not detected
void checkGateDetectedScore() {
  noGateScore = 0;
  for (int i = 0; i < 4; i++) {
    if (logicType[i] == 0) {
      noGateScore++;
    }
  }
  Serial.println(noGateScore);
}

// Determine IC type based on detected gate types
void checkICType() {
  for (int i = 0; i < 4; i++) {
    if (logicType[i] == 0) {
      continue;  // Skip unknown gates
    }
    
    // Set IC type based on first detected gate type
    switch (logicType[i]) {
      case 1: printICType = "7408 - AND"; break;    // Quad 2-input AND gate
      case 2: printICType = "7432 - OR"; break;     // Quad 2-input OR gate
      case 3: printICType = "7400 - NAND"; break;   // Quad 2-input NAND gate
      case 4: printICType = "7486 - XOR"; break;    // Quad 2-input XOR gate
      case 5: printICType = "7402 - NOR"; break;    // Quad 2-input NOR gate
      case 6: printICType = "74266 - XNOR"; break;  // Quad 2-input XNOR gate
    }
  }
}
