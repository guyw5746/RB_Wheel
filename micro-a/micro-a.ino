// Wheel Micro A: 버튼 12개 + 엔코더 2개 -> HID 게임패드
// 핀 배치는 SimHub 설정을 그대로 따름 (docs/pinout.md)
// 보드: Tools > Board > Wheel (DIY) > Wheel Micro A

#include <Joystick.h>

// ===== 설정 =====
const uint16_t PRESS_MS    = 5;   // 눌림이 이 시간(ms) 동안 유지돼야 "눌림". 눌림이 늦게 잡히면 줄이기
const uint16_t RELEASE_MS  = 60;  // 뗌이 이 시간(ms) 동안 유지돼야 "뗌". 누르고 있는데 떨리면 늘리기 (80~100)
const uint16_t PULSE_MS    = 50;  // 엔코더 한 칸 = 버튼을 이 시간(ms) 동안 누름
const uint16_t GAP_MS      = 30;  // 연속 칸 사이에 버튼을 떼고 있는 시간(ms)
const uint8_t  MAX_PENDING = 8;   // 빨리 돌릴 때 쌓아 두는 최대 칸 수

// 게임패드 버튼 번호 시작 위치 (A 1~16, B 17~31, C 33~42). 0이면 1번부터
const uint8_t BUTTON_OFFSET = 0;

// 게임패드 버튼 1번부터 순서대로
const uint8_t BUTTON_PINS[] = {6, 7, 8, 9, 10, 16, 14, 15, 18, 19, 20, 21};

struct EncoderPins { uint8_t clk, dt; bool reverse; };
// 버튼 다음 번호부터 엔코더마다 CW, CCW 순서. 방향이 반대면 reverse = true
const EncoderPins ENCODER_PINS[] = {
  {2, 3, false},  // Encoder 1 -> 13 CW / 14 CCW
  {4, 5, false},  // Encoder 2 -> 15 CW / 16 CCW
};
// ===== 설정 끝 (아래는 micro-b와 같은 코드) =====

const uint8_t NUM_BUTTONS  = sizeof(BUTTON_PINS);
const uint8_t NUM_ENCODERS = sizeof(ENCODER_PINS) / sizeof(ENCODER_PINS[0]);

Joystick_ Joystick(JOYSTICK_DEFAULT_REPORT_ID, JOYSTICK_TYPE_GAMEPAD,
                   BUTTON_OFFSET + NUM_BUTTONS + NUM_ENCODERS * 2, 0,
                   false, false, false, false, false, false,
                   false, false, false, false, false);

// Full-step quadrature state table (Ben Buxton). 잘못된 순서의 신호는 무시해서 튐을 막음
const uint8_t DIR_CW = 0x10, DIR_CCW = 0x20;
const uint8_t ENC_TABLE[7][4] = {
  {0x0, 0x2, 0x4, 0x0},
  {0x3, 0x0, 0x1, 0x0 | DIR_CW},
  {0x3, 0x2, 0x0, 0x0},
  {0x3, 0x2, 0x1, 0x0},
  {0x6, 0x0, 0x4, 0x0},
  {0x6, 0x5, 0x0, 0x0 | DIR_CCW},
  {0x6, 0x5, 0x4, 0x0},
};

struct Button { bool stable, lastRaw; unsigned long changedAt; };
Button buttons[NUM_BUTTONS];

enum : uint8_t { IDLE, PRESSED, GAP };
struct Encoder { uint8_t state, phase, activeBtn, cw, ccw; unsigned long since; };
Encoder encoders[NUM_ENCODERS];

bool changed = false;

void setup() {
  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    pinMode(BUTTON_PINS[i], INPUT_PULLUP);
    buttons[i] = {false, false, 0};
  }
  for (uint8_t i = 0; i < NUM_ENCODERS; i++) {
    pinMode(ENCODER_PINS[i].clk, INPUT_PULLUP);
    pinMode(ENCODER_PINS[i].dt, INPUT_PULLUP);
    encoders[i] = {0, IDLE, 0, 0, 0, 0};
  }
  Joystick.begin(false);
}

void updateButtons(unsigned long now) {
  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    Button &b = buttons[i];
    bool raw = digitalRead(BUTTON_PINS[i]) == LOW;
    if (raw != b.lastRaw) {
      b.lastRaw = raw;
      b.changedAt = now;
    } else if (raw != b.stable && now - b.changedAt >= (raw ? PRESS_MS : RELEASE_MS)) {
      b.stable = raw;
      Joystick.setButton(BUTTON_OFFSET + i, raw);
      changed = true;
    }
  }
}

void updateEncoders(unsigned long now) {
  for (uint8_t i = 0; i < NUM_ENCODERS; i++) {
    const EncoderPins &p = ENCODER_PINS[i];
    Encoder &e = encoders[i];

    uint8_t pins = (digitalRead(p.dt) << 1) | digitalRead(p.clk);
    e.state = ENC_TABLE[e.state & 0x0F][pins];
    uint8_t dir = e.state & 0x30;
    if (dir) {
      // 방향이 바뀌면 반대쪽에 쌓인 칸은 버림
      if ((dir == DIR_CW) != p.reverse) {
        e.ccw = 0;
        if (e.cw < MAX_PENDING) e.cw++;
      } else {
        e.cw = 0;
        if (e.ccw < MAX_PENDING) e.ccw++;
      }
    }

    switch (e.phase) {
      case PRESSED:
        if (now - e.since >= PULSE_MS) {
          Joystick.setButton(e.activeBtn, 0);
          changed = true;
          e.phase = GAP;
          e.since = now;
        }
        break;
      case GAP:
        if (now - e.since >= GAP_MS) e.phase = IDLE;
        break;
      case IDLE:
        if (e.cw || e.ccw) {
          e.activeBtn = BUTTON_OFFSET + NUM_BUTTONS + i * 2 + (e.cw ? 0 : 1);
          if (e.cw) e.cw--; else e.ccw--;
          Joystick.setButton(e.activeBtn, 1);
          changed = true;
          e.phase = PRESSED;
          e.since = now;
        }
        break;
    }
  }
}

void loop() {
  unsigned long now = millis();
  updateButtons(now);
  updateEncoders(now);
  if (changed) {
    Joystick.sendState();
    changed = false;
  }
}
