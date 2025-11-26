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

### Exact Wiring Table

| 74xx IC Pin | Function (on IC)        | Connected to Arduino Pin  | Colour in diagram | Notes                              |
|-------------|-------------------------|---------------------------|-------------------|------------------------------------|
| 1           | Input A (Gate 1)        | D2                        | Green             | MODE 1: Input → Output driver      |
| 2           | Input B (Gate 1)        | D3                        | Green             | Constant driver group              |
| 3           | Input A (Gate 2)        | D3                        | Green             | Shared with pin 2                  |
| 4           | Output (Gate 1)         | D4                        | Orange            | MODE 1: Output read                |
| 5           | Output (Gate 2)         | D5                        | Orange            |                                    |
| 6           | Input B (Gate 2)        | D6                        | Green             | Constant driver group              |
| 7           | GND                     | GND                       | Black             |                                    |
| 8           | Output (Gate 4)         | D11                       | Yellow            | MODE 2/3 driver                    |
| 9           | Input B (Gate 4)        | D9                        | Green             | Constant driver group              |
| 10          | Input A (Gate 3)        | D10                       | Green             | MODE 1: Input → Output driver      |
| 11          | Output (Gate 3)         | D8                        | Orange            |                                    |
| 12          | Input B (Gate 3) + A (4)| D12                       | Green             | Constant driver group              |
| 13          | Input A (Gate 4)        | D13                       | Green             | MODE 1: Input → Output driver      |
| 14          | VCC                     | +5V                       | Red               |                                    |

### Additional Connections
| Purpose             | Arduino Pin | Device       | Notes                                                    |
|---------------------|-------------|--------------|----------------------------------------------------------|
| OLED SDA            | A4          | SSD1306      | I2C data                                                 |
| OLED SCL            | A5          | SSD1306      | I2C clock                                                |
| Test / Retest Button| A3          | Push button  | One side to A3, other side to 5V (or GND + pull-up/down) |

### Summary of Arduino Digital Pins Used
D2, D3, D4, D5, D6, D8, D9, D10, D11, D12, D13  
+ A3 (button), A4 & A5 (I2C for display)

<img width="1411" height="274" alt="image" src="https://github.com/user-attachments/assets/55aa5379-c985-409d-9337-a18a3a711f99" />

## Display Options – Use Whatever You Want
The original build uses a cheap 128×64 SSD1306 I2C OLED, but the display part is 100% modular to the code.

Important variables that contain the result (updated in real time):
```cpp
String printState        // will contain the relevant code to your display of choice
String printICType       // e.g. "7408 - AND" or "MIXED / BAD" or "INSERT IC"
String printLogicType[4] // "OK" or "BAD" for each of the four gates
