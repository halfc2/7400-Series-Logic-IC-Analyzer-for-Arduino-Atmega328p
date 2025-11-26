# Arduino 74xx Series Quad Logic Gate Tester

A simple, low-cost IC tester capable of automatically identifying and verifying the four logic gates inside common 14-pin 74xx series logic ICs:

- 7400  – Quad 2-input NAND  
- 7402  – Quad 2-input NOR  
- 7408  – Quad 2-input AND  
- 7432  – Quad 2-input OR  
- 7486  – Quad 2-input XOR  
- 74266 – Quad 2-input XNOR (open-collector, tested as XNOR)

The tester also detects faulty gates and completely dead/missing ICs.

## Hardware
- Arduino Nano or Uno (ATmega328P)
- 14-pin ZIF socket (or regular turned-pin socket)
- Display of your choice (see Display Options below)
- Optional re-test button on A3

## Features
- No buttons needed – insert IC and read result instantly
- 128×64 OLED display (SSD1306 I2C) shows IC type and per-gate status. (Note that you can use your own display by changing the printState() function
- Identifies the exact IC model (7400 / 7402 / 7408 / 7432 / 7486 / 74266)
- Marks individual bad gates as "BAD"
- Works with 74HC / 74LS / 74AC etc. versions of the same logic family (same pinout)
- Single-sided ZIF socket wiring, built on Arduino Nano / Uno (ATmega328P)

## Supported ICs & Pinout
All tested and verified

| IC     | Name                  | Pins used for testing |
|--------|------------------------|-----------------------|
| 7400   | Quad 2-input NAND      | Standard 14-pin layout |
| 7402   | Quad 2-input NOR       | Standard 14-pin layout |
| 7408   | Quad 2-input AND       | Standard 14-pin layout |
| 7432   | Quad 2-input OR        | Standard 14-pin layout |
| 7486   | Quad 2-input XOR       | Standard 14-pin layout |
| 74266  | Quad 2-input XNOR (open collector) | Detected as XNOR |

All of these ICs share the same standard 74xx quad gate pinout (see image below):
<img width="375" height="447" alt="image" src="https://github.com/user-attachments/assets/3da43595-df11-4259-b8e1-97483a09a746" />

# Arduino 74xx Series Quad Logic Gate IC Tester

### Pin Connections (do not change unless you also edit the code)
<img width="1381" height="481" alt="image" src="https://github.com/user-attachments/assets/fc572292-3ebf-4ca7-b843-7d3d407660f6" />

## Display Options – Use Whatever You Want
The original build uses a cheap 128×64 SSD1306 I2C OLED, but the display part is 100% modular.

Important variables that contain the result (updated in real time):
```cpp
String printState        // will contain the relevant code to your display of choice
String printICType       // e.g. "7408 - AND" or "MIXED / BAD" or "INSERT IC"
String printLogicType[4] // "OK" or "BAD" for each of the four gates
