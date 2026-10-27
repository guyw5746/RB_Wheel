# RB wheel firmware

심레이싱 휠 버튼·엔코더 펌웨어. Pro Micro (ATmega32U4, 5V/16MHz) 3대, HID 게임패드로 동작.
휠 베이스(FFBeast), 페달(Leonardo, COM11), VoCore 화면은 이 저장소 범위가 아님.

| 스케치 | Tools > Board | Windows 이름 | USB VID:PID | 내용 |
|---|---|---|---|---|
| `micro-a/` | Wheel Micro A | Wheel Buttons A | 2341:9A01 | 버튼 12 + 엔코더 2 |
| `micro-b/` | Wheel Micro B | Wheel Encoders B | 2341:9A02 | 엔코더 5 (누름 포함) |
| `micro-c/` | Wheel Micro C | Wheel Matrix C | 2341:9A03 | 버튼 2 + 2x4 매트릭스 + I2C LCD |

- 핀·버튼 번호: `docs/pinout.md`. 조절 상수(`PRESS_MS`, `RELEASE_MS` 등)는 각 `.ino` 맨 위.
- micro-a와 micro-b는 버튼·엔코더 코드가 같음 (B에만 LED 부분 추가). 고칠 때 둘 다 고칠 것.
- Micro B의 RGB LED 21개는 PC(서버)가 COM 포트로 색을 보냄: `docs/led-protocol.md`.
- Micro C의 LCD는 PC(텔레메트리 서버, Node.js)가 COM 포트로 글자를 보냄: `docs/lcd-protocol.md`.
- Windows 이름과 PID는 `hardware/wheel/avr/boards.txt`에서 정함.
- 이 PC의 sketchbook: `C:\Users\guyy5\OneDrive\문서\Arduino`.
  CLI: `C:\Program Files\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe`
  (예: `arduino-cli compile -b wheel:avr:wheel_a micro-a`)

## 처음 한 번

1. Arduino IDE 2 설치. Boards Manager에 "Arduino AVR Boards"가 설치돼 있어야 함 (기본 설치됨).
2. 라이브러리: https://github.com/MHeironimus/ArduinoJoystickLibrary → Code > Download ZIP
   → IDE에서 Sketch > Include Library > Add .ZIP Library.
   Micro B는 추가로 Library Manager에서 "Adafruit NeoPixel" 설치.
   Micro C는 추가로 Library Manager에서 "LiquidCrystal I2C" (Frank de Brabander) 설치.
3. 이 저장소의 `hardware\wheel` 폴더를 `<sketchbook>\hardware\wheel`로 복사 → IDE를 모두 닫고 재시작.
   Tools > Board에 "Wheel (DIY)" 아래 Wheel Micro A / B / C가 보이면 성공.
   `boards.txt`를 고치면 다시 복사할 것.

## 올리기

1. **SimHub을 완전히 끄기** (켜져 있으면 COM 포트를 잡고 있어서 실패).
2. 스케치 열기 → Tools > Board > Wheel Micro A/B/C → Tools > Port → Upload.
   **스케치와 보드 항목을 반드시 짝 맞출 것.** 휠 안 USB 허브 때문에 보드를 뺄 수 없으니 포트를 정확히 고를 것.
   페달 Leonardo(COM11)는 절대 고르지 않기.
3. Win+R → `joy.cpl`에서 이름과 버튼 동작 확인.

## 업로드가 멈췄을 때 / 포트가 안 보일 때

1. Upload를 누르고 아래에 "Uploading..."이 뜨면,
2. RST 핀과 GND 핀을 핀셋이나 선으로 **빠르게 두 번** 톡톡 닿게 함 (리셋 버튼이 있으면 두 번 누름).
3. 약 8초 동안 부트로더가 기다리고, 그 사이에 IDE가 올림. 실패하면 다시 반복.
   (휠 뚜껑을 열어야 RST 핀에 닿음)
