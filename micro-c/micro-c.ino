// Wheel Micro C: 버튼 2개 + 2x4 버튼 매트릭스 -> HID 게임패드, I2C LCD는 PC가 COM 포트로 보낸 글자를 표시
// 예전 Nano(SimHub) 배선을 Pro Micro 핀으로 옮김 (docs/pinout.md), LCD 명령은 docs/lcd-protocol.md
// 보드: Tools > Board > Wheel (DIY) > Wheel Micro C

#include <Joystick.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ===== 설정 =====
const uint16_t PRESS_MS    = 5;   // 눌림이 이 시간(ms) 동안 유지돼야 "눌림". 눌림이 늦게 잡히면 줄이기
const uint16_t RELEASE_MS  = 60;  // 뗌이 이 시간(ms) 동안 유지돼야 "뗌". 누르고 있는데 떨리면 늘리기 (80~100)

// 게임패드 버튼 번호 시작 위치 (A 1~16, B 17~31, C 33~42). 32이면 33번부터
// 게임에서 33번 이상이 안 잡히면 0으로 (1~10번)
const uint8_t BUTTON_OFFSET = 32;

// 게임패드 버튼 33번부터 순서대로 (SimHub와 같은 순서: 일반 버튼 다음 매트릭스)
const uint8_t BUTTON_PINS[] = {
  10,  // 33: LED 토글 스위치 (Nano A2)
  16,  // 34: 전원 모양 푸시 버튼 (Nano A3)
};

// 매트릭스 버튼 번호: 35 + row * 4 + col
const uint8_t MATRIX_ROWS[] = {4, 5};        // Row 1, 2 (Nano A0, A1)
const uint8_t MATRIX_COLS[] = {6, 7, 8, 9};  // Col 1~4 (Nano D10~D13)
const bool DRIVE_ROWS = true;  // 매트릭스 버튼이 하나도 안 눌리면 false로 (다이오드 방향 문제)

// LCD: SDA = pin 2, SCL = pin 3 (Pro Micro 고정)
const uint8_t LCD_ADDRESS = 0x27;  // 화면이 안 나오면 0x3F (PCF8574A 칩)
const uint8_t LCD_COLS = 16;       // 20x4 LCD면 20
const uint8_t LCD_ROWS = 2;        // 20x4 LCD면 4
// ===== 설정 끝 =====

const uint8_t NUM_DIRECT  = sizeof(BUTTON_PINS);
const uint8_t NUM_ROWS    = sizeof(MATRIX_ROWS);
const uint8_t NUM_COLS    = sizeof(MATRIX_COLS);
const uint8_t NUM_BUTTONS = NUM_DIRECT + NUM_ROWS * NUM_COLS;
const uint8_t LINE_MAX    = 32;

Joystick_ Joystick(JOYSTICK_DEFAULT_REPORT_ID, JOYSTICK_TYPE_GAMEPAD,
                   BUTTON_OFFSET + NUM_BUTTONS, 0,
                   false, false, false, false, false, false,
                   false, false, false, false, false);
LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLS, LCD_ROWS);

struct Button { bool stable, lastRaw; unsigned long changedAt; };
Button buttons[NUM_BUTTONS];
bool raw[NUM_BUTTONS];

char line[LINE_MAX + 1];
uint8_t lineLen = 0;

const uint8_t *drivePins() { return DRIVE_ROWS ? MATRIX_ROWS : MATRIX_COLS; }
const uint8_t *sensePins() { return DRIVE_ROWS ? MATRIX_COLS : MATRIX_ROWS; }
uint8_t numDrive() { return DRIVE_ROWS ? NUM_ROWS : NUM_COLS; }
uint8_t numSense() { return DRIVE_ROWS ? NUM_COLS : NUM_ROWS; }

void printLine(uint8_t row, const char *text) {
  lcd.setCursor(0, row);
  uint8_t i = 0;
  for (; i < LCD_COLS && text[i]; i++) lcd.write(text[i]);
  for (; i < LCD_COLS; i++) lcd.write(' ');  // 이전 글자 지우기
}

void setup() {
  for (uint8_t i = 0; i < NUM_DIRECT; i++) pinMode(BUTTON_PINS[i], INPUT_PULLUP);
  for (uint8_t i = 0; i < numDrive(); i++) pinMode(drivePins()[i], INPUT);  // 스캔할 때만 LOW 출력
  for (uint8_t i = 0; i < numSense(); i++) pinMode(sensePins()[i], INPUT_PULLUP);
  for (uint8_t i = 0; i < NUM_BUTTONS; i++) buttons[i] = {false, false, 0};

  Joystick.begin(false);
  Serial.begin(115200);

  lcd.init();
  lcd.backlight();
  printLine(0, "Wheel Matrix C");
  if (LCD_ROWS > 1) printLine(1, "Waiting for PC");
}

void readButtons() {
  for (uint8_t i = 0; i < NUM_DIRECT; i++) raw[i] = digitalRead(BUTTON_PINS[i]) == LOW;

  for (uint8_t d = 0; d < numDrive(); d++) {
    pinMode(drivePins()[d], OUTPUT);
    digitalWrite(drivePins()[d], LOW);
    delayMicroseconds(10);
    for (uint8_t s = 0; s < numSense(); s++) {
      uint8_t r = DRIVE_ROWS ? d : s;
      uint8_t c = DRIVE_ROWS ? s : d;
      raw[NUM_DIRECT + r * NUM_COLS + c] = digitalRead(sensePins()[s]) == LOW;
    }
    pinMode(drivePins()[d], INPUT);
  }
}

void updateButtons(unsigned long now) {
  bool changed = false;
  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    Button &b = buttons[i];
    if (raw[i] != b.lastRaw) {
      b.lastRaw = raw[i];
      b.changedAt = now;
    } else if (raw[i] != b.stable && now - b.changedAt >= (raw[i] ? PRESS_MS : RELEASE_MS)) {
      b.stable = raw[i];
      Joystick.setButton(BUTTON_OFFSET + i, raw[i]);
      changed = true;
    }
  }
  if (changed) Joystick.sendState();
}

void handleCommand(const char *cmd) {
  if (cmd[0] >= '0' && cmd[0] < '0' + LCD_ROWS && cmd[1] == ':') {
    printLine(cmd[0] - '0', cmd + 2);
  } else if (strcmp(cmd, "C") == 0) {
    lcd.clear();
  } else if (strcmp(cmd, "B:1") == 0) {
    lcd.backlight();
  } else if (strcmp(cmd, "B:0") == 0) {
    lcd.noBacklight();
  } else if (strcmp(cmd, "?") == 0) {
    Serial.print(F("WHEEL-C "));
    Serial.print(LCD_COLS);
    Serial.print('x');
    Serial.println(LCD_ROWS);
  }
}

void readSerial() {
  while (Serial.available()) {
    char ch = Serial.read();
    if (ch == '\r') continue;
    if (ch == '\n') {
      line[lineLen] = '\0';
      handleCommand(line);
      lineLen = 0;
    } else if (lineLen < LINE_MAX) {
      line[lineLen++] = ch;
    }
  }
}

void loop() {
  readButtons();
  updateButtons(millis());
  readSerial();
}
