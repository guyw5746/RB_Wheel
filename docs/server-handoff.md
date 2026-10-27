# Server handoff: wheel LEDs and LCD

For the agent working in the telemetry server repo (Node.js, parses F1 25 UDP telemetry, already serves an HTML dashboard).
The firmware repo is `C:\Temp\RB wheel` (this repo). Firmware is done; the server side is the remaining work.

## Goal

Replace SimHub. SimHub used to (1) map wheel buttons, (2) drive the wheel RGB LEDs from game data, (3) drive an I2C LCD, (4) show the VoCore dashboard.
Now the wheel boards are plain HID gamepads (games map buttons directly), and the **server** decides what LEDs/LCD show and sends it over USB serial.

## Devices (all Pro Micro, ATmega32U4, USB HID gamepad + CDC serial on one cable)

| Board | USB VID:PID | Serial use | Status |
|---|---|---|---|
| Micro A "Wheel Buttons A" | 2341:9A01 | none | uploaded |
| Micro B "Wheel Encoders B" | 2341:9A02 | 21x WS2812B RGB LEDs | uploaded; LEDs stay off until the server sends colors |
| Micro C "Wheel Matrix C" | 2341:9A03 | 16x2 I2C LCD | firmware written, not built into hardware yet |

The pedal Leonardo (2341:8036) is a different device. Never open it.

## Server tasks

1. **LEDs (Micro B)**: re-implement the SimHub LED profile logic in the server, then send frames. The protocol is in `docs/led-protocol.md`.
   - Source of truth for the old behaviour: `docs/simhub/Codemasters F1 25 (2026 Season pack) - Codemasters F1 25 (2026 Season pack) - RB22.ledsprofile` (SimHub JSON; formulas reference SimHub properties). The user says the server already has equivalent variables. Map each SimHub property to the server's variable, and ask the user where unclear.
   - SimHub global brightness was 15%. Firmware already applies 15% (`LED_BRIGHTNESS = 38`), so send full-scale colors.
   - Firmware turns the LEDs off after 5 s without an `L:` frame, so keep sending at least once per second.
2. **LCD (Micro C)**: send text lines such as best lap time. The protocol is in `docs/lcd-protocol.md`. ASCII only, 16 chars x 2 lines.
3. Later: live viewer count (Chzzk / YouTube) on the LCD. Not researched yet. The YouTube Data API `liveStreamingDetails.concurrentViewers` is a known option. Chzzk's official API support for this is unverified.

## Rules

- Find ports by VID/PID with `serialport`'s `SerialPort.list()`, not by COM number.
- **Never open a port at 1200 baud.** That resets the board into its bootloader. Use 115200.
- Handle the device being unplugged or replugged (reconnect loop).
- SimHub's Arduino plugin must stay off. It would grab the same COM ports.
- Do not change firmware from the server repo. Firmware changes go in `C:\Temp\RB wheel`.

## Out of scope for now

VoCore screen (800x480). It currently shows the user's own HTML UI through SimHub. The plan is to later use it as a Windows secondary monitor with a kiosk browser. A VoCore Windows display driver is unverified.
