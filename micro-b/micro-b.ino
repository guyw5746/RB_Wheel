// Wheel Micro B: 엔코더 5개 (누름 스위치 포함) -> HID 게임패드
// 핀 배치는 SimHub 설정을 그대로 따름 (docs/pinout.md)
// 보드: Tools > Board > Wheel (DIY) > Wheel Micro B
// RGB LED (WS2812B 21개)는 PC가 COM 포트로 보낸 색을 그대로 표시 (docs/led-protocol.md)

#include <Joystick.h>
#include <Adafruit_NeoPixel.h>

// ===== 설정 =====
const uint16_t PRESS_MS    = 5;   // 눌림이 이 시간(ms) 동안 유지돼야 "눌림". 눌림이 늦게 잡히면 줄이기
const uint16_t RELEASE_MS  = 60;  // 뗌이 이 시간(ms) 동안 유지돼야 "뗌". 누르고 있는데 떨리면 늘리기 (80~100)
const uint16_t PULSE_MS    = 50;  // 엔코더 한 칸 = 버튼을 이 시간(ms) 동안 누름
const uint16_t GAP_MS      = 30;  // 연속 칸 사이에 버튼을 떼고 있는 시간(ms)
const uint8_t  MAX_PENDING = 8;   // 빨리 돌릴 때 쌓아 두는 최대 칸 수

// 게임패드 버튼 번호 시작 위치 (A 1~16, B 17~31, C 33~42). 16이면 17번부터
const uint8_t BUTTON_OFFSET = 16;

// 게임패드 버튼 17번부터 순서대로 (엔코더 1~5 누름 스위치 -> 17~21)
const uint8_t BUTTON_PINS[] = {21, 20, 19, 18, 15};

struct EncoderPins { uint8_t clk, dt; bool reverse; };
// 버튼 다음 번호부터 엔코더마다 CW, CCW 순서. 방향이 반대면 reverse = true
const EncoderPins ENCODER_PINS[] = {
  {2, 3, false},    // Encoder 1 -> 22 CW / 23 CCW
  {4, 5, false},    // Encoder 2 -> 24 CW / 25 CCW
  {6, 7, false},    // Encoder 3 -> 26 CW / 27 CCW
  {8, 9, false},    // Encoder 4 -> 28 CW / 29 CCW
  {14, 16, false},  // Encoder 5 -> 30 CW / 31 CCW
};

const uint8_t  LED_PIN        = 10;
const uint8_t  LED_COUNT      = 21;
const uint8_t  LED_BRIGHTNESS = 38;    // 전체 밝기 0~255 (38 = SimHub에서 쓰던 15%). 너무 올리면 USB 전류 부족
const uint16_t LED_TIMEOUT_MS = 5000;  // PC에서 이 시간(ms) 동안 데이터가 없으면 LED 끔. 0이면 끄지 않음
// ===== 설정 끝 (LED 부분 말고는 micro-a와 같은 코드) =====

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

Adafruit_NeoPixel leds(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
const uint8_t LINE_MAX = 2 + LED_COUNT * 6 + 8;
char line[LINE_MAX + 1];
uint8_t lineLen = 0;
unsigned long lastLedData = 0;
bool ledsOn = false;

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
  Serial.begin(115200);

  // 켜질 때 0.3초 흰색으로 배선 확인
  leds.begin();
  leds.setBrightness(LED_BRIGHTNESS);
  leds.fill(leds.Color(255, 255, 255));
  leds.show();
  delay(300);
  leds.clear();
  leds.show();
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

int8_t hexValue(char ch) {
  if (ch >= '0' && ch <= '9') return ch - '0';
  if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
  if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
  return -1;
}

// "L:" 뒤의 RRGGBB를 0번 LED부터 순서대로 적용. 모자라면 나머지는 그대로
void setLeds(const char *hex, unsigned long now) {
  for (uint8_t i = 0; i < LED_COUNT; i++, hex += 6) {
    uint8_t rgb[3];
    for (uint8_t k = 0; k < 3; k++) {
      int8_t hi = hexValue(hex[k * 2]);
      int8_t lo = hi < 0 ? -1 : hexValue(hex[k * 2 + 1]);
      if (lo < 0) goto done;
      rgb[k] = hi * 16 + lo;
    }
    leds.setPixelColor(i, rgb[0], rgb[1], rgb[2]);
  }
done:
  leds.show();
  lastLedData = now;
  ledsOn = true;
}

void handleCommand(const char *cmd, unsigned long now) {
  if (cmd[0] == 'L' && cmd[1] == ':') {
    setLeds(cmd + 2, now);
  } else if (strcmp(cmd, "O") == 0) {
    leds.clear();
    leds.show();
    ledsOn = false;
  } else if (strcmp(cmd, "?") == 0) {
    Serial.print(F("WHEEL-B LED "));
    Serial.println(LED_COUNT);
  }
}

void readSerial(unsigned long now) {
  while (Serial.available()) {
    char ch = Serial.read();
    if (ch == '\r') continue;
    if (ch == '\n') {
      line[lineLen] = '\0';
      handleCommand(line, now);
      lineLen = 0;
    } else if (lineLen < LINE_MAX) {
      line[lineLen++] = ch;
    }
  }
  if (ledsOn && LED_TIMEOUT_MS && now - lastLedData >= LED_TIMEOUT_MS) {
    leds.clear();
    leds.show();
    ledsOn = false;
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
  readSerial(now);
}
