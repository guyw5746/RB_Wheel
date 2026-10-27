# Micro C LCD 명령

Micro C는 USB 하나로 게임패드 + COM 포트를 같이 열어요. PC는 COM 포트로 한 줄짜리 텍스트 명령을 보내요.

- 포트 찾기: USB VID `2341`, PID `9A03` (COM 번호는 바뀔 수 있으니 번호로 찾지 말 것)
- 속도: 115200 (아무 값이나 되지만 **1200은 금지**. 1200으로 열면 업로드 모드로 리셋됨)
- 줄 끝: `\n` (`\r\n`도 됨). 한 명령은 최대 32글자
- 글자: 영문·숫자·기호(ASCII)만. 한글은 LCD에서 안 나옴

| 명령 | 동작 |
|---|---|
| `0:텍스트` ~ `3:텍스트` | 해당 줄에 표시. 남는 칸은 공백으로 지움, 넘치는 글자는 잘림 |
| `C` | 화면 지우기 |
| `B:1` / `B:0` | 백라이트 켜기 / 끄기 |
| `?` | `WHEEL-C 16x2` 처럼 보드 이름과 LCD 크기를 답함 |

줄 번호는 LCD 크기 안에서만 동작 (16x2면 `0`, `1`).

## Node.js 예시 (`npm i serialport`)

```js
import { SerialPort } from 'serialport';

const info = (await SerialPort.list()).find(
  (p) => p.vendorId?.toLowerCase() === '2341' && p.productId?.toLowerCase() === '9a03'
);
if (!info) throw new Error('Wheel Micro C not found');

const lcd = new SerialPort({ path: info.path, baudRate: 115200 });
lcd.write('0:BEST 1:23.456\n');
lcd.write('1:VIEW 128\n');
```

같은 내용을 계속 보내면 LCD가 깜빡일 수 있으니, 값이 바뀔 때만 보내는 걸 추천.
