# Arduino 74xx Series Quad Logic Gate Tester

A simple, low-cost IC tester capable of automatically identifying and verifying the four logic gates inside common 14-pin 74xx series logic ICs:

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

| IC     | Name                  |
|--------|------------------------|
| 7400   | Quad 2-input NAND      |
| 7402   | Quad 2-input NOR       |
| 7408   | Quad 2-input AND       |
| 7432   | Quad 2-input OR        |
| 7486   | Quad 2-input XOR       |
| 74266  | Quad 2-input XNOR (open collector) |

All of these ICs share the same standard 74xx quad gate pinout (see image below):

<img width="375" height="447" alt="image" src="https://github.com/user-attachments/assets/3da43595-df11-4259-b8e1-97483a09a746" />

### Exact Wiring Table

<img width="1411" height="274" alt="image" src="https://github.com/user-attachments/assets/55aa5379-c985-409d-9337-a18a3a711f99" />

| 74xx IC Pin | Arduino Pin               | Notes                              |
|-------------|---------------------------|--------------------------------------------|
| 1           | D2                        | MODE 1: Input → MODE 2: Output             |
| 2           | D3                        | Constant driver group                      |
| 3           | D4                        | MODE 1: Output → MODE 2: Input             |
| 4           | D5                        | MODE 1: Input → MODE 2: Output             |
| 5           | D6                        | Constant driver group                      |
| 6           | D7                        | MODE 1: Output → MODE 2: Input             |
| **7**       | **GND**                   | **Connect to Ground**                      |
| 8           | D8                        | MODE 1: Output → MODE 2: Input             |
| 9           | D9                        | Constant driver group                      |
| 10          | D10                       | MODE 1: Input → MODE 2: Output             |
| 11          | D11                       | MODE 1: Output → MODE 2: Input             |
| 12          | D12                       | Constant driver group                      |
| 13          | D13                       | MODE 1: Input → MODE 2: Output             |
| **14**      | **+5V/VCC**               | **Connect to 5V supply**                   |

### Additional Connections
| Purpose             | Arduino Pin | Device       | Notes                                                    |
|---------------------|-------------|--------------|----------------------------------------------------------|
| OLED SDA            | A4          | SSD1306      | I2C data                                                 |
| OLED SCL            | A5          | SSD1306      | I2C clock                                                |
| Test / Retest Button| A3          | Push button  | One side to A3, other side to 5V (or GND + pull-up/down) |

### Summary of Arduino Digital Pins Used
D2, D3, D4, D5, D6, D7, D8, D9, D10, D11, D12, D13  
+ A3 (button), A4 & A5 (I2C for display)

## Display Options – Use Whatever You Want
The original build uses a cheap 128×64 SSD1306 I2C OLED, but the display part is 100% modular to the code.

Important variables that contain the result (updated in real time):
```cpp
String printState        // will contain the relevant code to your display of choice
String printICType       // e.g. "7408 - AND" or "MIXED / BAD" or "INSERT IC"
String printLogicType[4] // "OK" or "BAD" for each of the four gates

## Additional Notes
This code is old and has not been properly crosschecked with the original prototype, but i still decided to upload it for archiving. I do hope you know what you are doing and make sense of it.
