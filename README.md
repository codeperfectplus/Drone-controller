
# DIY RC Transmitter & Receiver (Arduino + nRF24L01+ → PPM)

## Overview

This project implements a simple, low-cost RC system using two Arduino boards and nRF24L01+ modules. The transmitter reads two joysticks, two potentiometers, and two toggle switches, then sends their states wirelessly. The receiver outputs a standard PPM signal for use with most flight controllers.

---

## Hardware Required

**Transmitter:**
- Arduino Nano/Uno/Pro Mini
- nRF24L01+ module
- 2× 2-axis joysticks (potentiometer-based)
- 2× SPDT toggle switches
- 2× potentiometers (10k)
- 1× LED (optional, for toggle feedback)
- Wires, breadboard or perfboard, enclosure

**Receiver:**
- Arduino Nano/Uno/Pro Mini
- nRF24L01+ module
- 2× LEDs (optional, for toggle feedback)
- Connects to flight controller via PPM (single wire)

---

## Pin Connections

### Transmitter

- nRF24L01+:
   - VCC → 3.3V (not 5V!)
   - GND → GND
   - CE  → D9
   - CSN → D10
   - SCK → D13
   - MOSI→ D11
   - MISO→ D12

- Joystick 1: X → A0, Y → A1, SW → D2
- Joystick 2: X → A2, Y → A3, SW → D3
- Potentiometer 1: Wiper → A4
- Potentiometer 2: Wiper → A5
- Toggle 1: A → D4, B → D5, middle pin to GND
- Toggle 2: A → D6, B → D7, middle pin to GND
- LED1: D8

### Receiver

- nRF24L01+:
   - Same as transmitter

- PPM output: D4 (connect to FC PPM input)
- LED1: D3, LED2: D2 (optional, for toggle feedback)

---

## How to Build

1. **Wire up the transmitter and receiver as described above.**
2. **Flash `drone_transmitter.ino` to the transmitter Arduino.**
3. **Flash `drone_receiver_ppm.ino` to the receiver Arduino.**
4. **Power both boards (ensure nRF24L01+ gets a stable 3.3V with a 10–47µF capacitor across VCC/GND).**
5. **Connect the receiver's PPM output (D4) to your flight controller's PPM input.**
6. **Test: Move sticks and switches on the transmitter; the receiver outputs a PPM stream reflecting all 10 channels.**

---

## Features

- 10 channels: 4 axes, 2 buttons, 2 pots, 2 toggles
- Failsafe: If signal lost for 1s, all channels set to 1000us
- PPM output: Standard 10-channel PPM
- LEDs indicate toggle switch states

---

## Notes

- The code uses the RF24 Arduino library.
- PPM output is compatible with most flight controllers (Betaflight, iNav, ArduPilot, etc.). Tested with iNav and Betaflight.
- No OLED or telemetry in this version as Arduino memory is limited.
- For best range, use nRF24L01+ PA/LNA modules and keep antennas clear.

---

## Troubleshooting

- If the receiver does not respond, check wiring and power to nRF24L01+ modules.
- Add a large capacitor (10–47µF) close to the nRF24L01+ VCC/GND.
- If PPM is not detected, verify the output pin and FC configuration.

---

