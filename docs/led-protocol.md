# Micro B RGB LED 명령

Micro B는 게임패드 + COM 포트를 같이 열어요. PC(서버)가 COM 포트로 LED 색을 보내면 그대로 켜요.
"언제 어떤 LED를 켤지"는 전부 서버가 정함. 참고용 원본: `docs/simhub/*.ledsprofile` (SimHub RGB LED 프로필).

- 포트 찾기: USB VID `2341`, PID `9A02`
- 속도: 115200 (**1200은 금지**, 업로드 모드로 리셋됨)
- 줄 끝: `\n`
- LED: WS2812B 21개, 0번부터 순서대로. 전체 밝기는 펌웨어 `LED_BRIGHTNESS` (기본 15%)로 한 번 더 낮춰짐

| 명령 | 동작 |
|---|---|
| `L:RRGGBB...` | 0번 LED부터 색 지정 (16진수 6자리씩, 최대 21개 = 126자). 적게 보내면 나머지는 그대로 |
| `O` | 전부 끄기 |
| `?` | `WHEEL-B LED 21` 응답 |

- PC에서 5초 동안 `L:` 명령이 없으면 LED가 자동으로 꺼져요 (`LED_TIMEOUT_MS`). 서버는 바뀐 게 없어도 1초에 한 번 이상 보내기.
- 초당 30~60번 보내도 괜찮아요.

## Node.js 예시 (`npm i serialport`)

```js
import { SerialPort } from 'serialport';

const info = (await SerialPort.list()).find(
  (p) => p.vendorId?.toLowerCase() === '2341' && p.productId?.toLowerCase() === '9a02'
);
const leds = new SerialPort({ path: info.path, baudRate: 115200 });

// colors: [[r,g,b], ...] 21개
function sendLeds(colors) {
  const hex = colors.map(([r, g, b]) =>
    [r, g, b].map((v) => v.toString(16).padStart(2, '0')).join('')).join('');
  leds.write(`L:${hex}\n`);
}

sendLeds(Array.from({ length: 21 }, (_, i) => (i < 5 ? [0, 255, 0] : [0, 0, 0])));
```
