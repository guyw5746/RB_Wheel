# 핀 배치

출처: SimHub 프로필 `BUTTON_ENC.tfp`(Micro A), `LED_ENCODER.tfp`(Micro B)와 캡처(`docs/simhub/`).
보드는 Pro Micro (ATmega32U4).
모든 입력은 `INPUT_PULLUP`, 스위치는 핀과 GND 사이에 연결 (SimHub "Pin to GND"와 같음).
버튼 번호는 Windows `joy.cpl`에 보이는 번호 (1부터).

## Micro A — "Wheel Buttons A" (PID 0x9A01)

| 버튼 | 핀 | 내용 |
|---|---|---|
| 1 | 6 | Button 1 |
| 2 | 7 | Button 2 |
| 3 | 8 | Button 3 |
| 4 | 9 | Button 4 |
| 5 | 10 | Button 5 |
| 6 | 16 | Button 6 |
| 7 | 14 | Button 7 |
| 8 | 15 | Button 8 |
| 9 | 18 (A0) | Button 9 |
| 10 | 19 (A1) | Button 10 |
| 11 | 20 (A2) | Button 11 |
| 12 | 21 (A3) | Button 12 |
| 13 / 14 | CLK 2, DT 3 | Encoder 1 CW / CCW |
| 15 / 16 | CLK 4, DT 5 | Encoder 2 CW / CCW |

## Micro B — "Wheel Encoders B" (PID 0x9A02)

| 버튼 | 핀 | 내용 |
|---|---|---|
버튼 번호는 17번부터 (`BUTTON_OFFSET = 16`). 1~16번은 비어 있음.

| 버튼 | 핀 | 내용 |
|---|---|---|
| 17 | 21 (A3) | Encoder 1 누름 |
| 18 | 20 (A2) | Encoder 2 누름 |
| 19 | 19 (A1) | Encoder 3 누름 |
| 20 | 18 (A0) | Encoder 4 누름 |
| 21 | 15 | Encoder 5 누름 |
| 22 / 23 | CLK 2, DT 3 | Encoder 1 CW / CCW |
| 24 / 25 | CLK 4, DT 5 | Encoder 2 CW / CCW |
| 26 / 27 | CLK 6, DT 7 | Encoder 3 CW / CCW |
| 28 / 29 | CLK 8, DT 9 | Encoder 4 CW / CCW |
| 30 / 31 | CLK 14, DT 16 | Encoder 5 CW / CCW |
| — | 10 | WS2812B LED 21개 data (`docs/led-protocol.md`) |
- 엔코더 steps mode: SimHub 설정은 전부 Full steps → 펌웨어도 full-step 방식.

## Micro C — "Wheel Matrix C" (PID 0x9A03)

예전 Nano 배선(`docs/simhub/nano-wiring.png`)을 Pro Micro로 옮긴 것. 선을 아래 Pro Micro 핀에 다시 연결.
Nano의 MAX7221, TM1637, WS2812B는 쓰지 않아서 뺌.

| 버튼 | Pro Micro 핀 | 예전 Nano 핀 | 내용 |
|---|---|---|---|
| 33 | 10 | A2 | LED 토글 스위치 |
| 34 | 16 | A3 | 전원 모양 푸시 버튼 |
| 35–38 | Row 1 = 4, Col 1–4 = 6, 7, 8, 9 | A0 / D10–D13 | 매트릭스 1행 |
| 39–42 | Row 2 = 5, Col 1–4 = 6, 7, 8, 9 | A1 / D10–D13 | 매트릭스 2행 |

- 버튼 번호는 33번부터 (`BUTTON_OFFSET = 32`). 게임에서 33번 이상이 안 잡히면 0으로 바꿔서 1~10번으로.
- ⚠ 핀 배치는 Nano 소켓에 Pro Micro를 그대로 꽂는 방식으로 바꿀 예정 (아직 미확정).
| — | SDA 2, SCL 3 | A4, A5 | I2C LCD (VCC·GND는 Pro Micro VCC·GND) |

- 매트릭스 버튼이 하나도 안 잡히면 `micro-c.ino`의 `DRIVE_ROWS`를 `false`로 (다이오드 방향).
