# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 工作規則

- 在執行任何對話任務之前，請先完整閱讀本檔案（CLAUDE.md）。
- 回答使用者時優先使用「正體中文」。
- 每次對話結束後，將本次的製作進度補充記錄到本檔案的「進度記錄」章節中。

## Project state

This is a PlatformIO/Arduino project for ESP32 implementing a WiFi remote control. Current structure:

- `include/config.h` — project-wide `#define` constants: `BLE_DEVICE_NAME`; two TT motors' pins (`MOTOR1_IN1_PIN`/`MOTOR1_IN2_PIN`/`MOTOR1_PWM_PIN`, `MOTOR2_IN1_PIN`/`MOTOR2_IN2_PIN`/`MOTOR2_PWM_PIN`); two servos' pin/initial angle (`SERVO1_PIN`/`SERVO1_INITIAL_ANGLE`, `SERVO2_PIN`/`SERVO2_INITIAL_ANGLE`). PlatformIO auto-adds the top-level `include/` folder to the include path for both `src/` and `lib/`, so any file can `#include "config.h"`. Libraries themselves stay pin-agnostic (pins are passed into `begin()` by the caller) — `config.h` is where the actual wiring/parameter choices live, not the libraries.
- `src/main.cpp` — entry point. Instantiates `BleControl`, two `TtMotorControl` (`motor1`/`motor2`), two `ServoControl` (`servo1`/`servo2`); `setup()` calls each one's `begin()` with the matching pins from `config.h`. `onButtonPressed` prints the received string to Serial, then `switch`es on its first character (single-letter commands from the BLE central) to drive the motors/servos:
  - `'a'` — both motors `forward()`
  - `'c'` — both motors `backward()`
  - `'b'` — `servo1` angle `+SERVO_STEP_DEGREES`, `servo2` angle `-SERVO_STEP_DEGREES`
  - `'d'` — `servo1` angle `-SERVO_STEP_DEGREES`, `servo2` angle `+SERVO_STEP_DEGREES`
  - anything else — prints "無法識別的按鍵", no hardware action
  This mapping (which letter does what) came directly from the user, not inferred — if the BLE sender's protocol changes, update this `switch` accordingly.
- `lib/ble_control/ble_control.h` / `ble_control.cpp` — self-contained library (PlatformIO auto-discovers `lib/<name>` folders, no changes needed in `platformio.ini`). `BleControl`:
  - Runs the ESP32 as a BLE Peripheral (`BLEDevice`/`BLEServer`, ESP32 Arduino core, built-in — no extra `lib_deps` needed), advertising under the given device name.
  - Exposes one GATT service/characteristic (fixed random UUIDs defined as `SERVICE_UUID`/`CHARACTERISTIC_UUID` in `ble_control.cpp`) with `WRITE` property; a connected BLE central (phone app, nRF Connect, etc.) writes a button name string (e.g. `"UP"`, `"BTN1"`) to that characteristic.
  - `BleControl` implements `BLEServerCallbacks`/`BLECharacteristicCallbacks` directly: `onWrite` invokes the registered `ButtonCallback` with the written value; `onConnect`/`onDisconnect` track connection state.
  - `handle()` must be called every loop iteration — it only restarts BLE advertising after a disconnect (BLE itself is callback/interrupt driven, not polled like the old WebServer).
  - To rename/add buttons: no server-side change needed, button names are arbitrary strings decided by whatever writes to the characteristic (phone app UI, test tool, etc.).
- Previously had a `WifiControl` library (`lib/wifi_control/`, WiFi AP + web page UI) — **removed** on 2026-10-23. Flash didn't have room for WiFi + BLE stacks running together on the default partition table (BLE alone is already ~85.6% of the 1.25MB app partition), and BLE was chosen as the sole control transport. See 進度記錄 below if this needs revisiting.
- `lib/tt_motor_control/tt_motor_control.h` / `.cpp` — `TtMotorControl`: drives one TT gear motor through a generic H-bridge driver board (L298N/TB6612-style: 2 direction pins + 1 PWM speed pin). `begin(in1Pin, in2Pin, pwmPin, autoStopMs=100)`, `forward(speed=255)`, `backward(speed=255)`, `stop()` (coasts, not an active brake), `handle()`. Instantiate one object per motor (e.g. two for left/right wheels). Uses the raw `ledcSetup`/`ledcAttachPin`/`ledcWrite` LEDC API directly (not `ESP32Servo`) — **allocates PWM channels top-down from 15** (a static counter, decremented per `begin()` call) specifically to avoid colliding with `ServoControl`/`ESP32Servo`, which allocates its own channels bottom-up from 0; this only matters if both libraries are used in the same sketch. If a motor spins backwards relative to `forward()`/`backward()`, swap its IN1/IN2 wiring.
  - **Dead-man's-switch auto-stop**: every `forward()`/`backward()` call stamps `millis()`; `handle()` (must be called every `loop()` iteration, like `BleControl::handle()`) calls `stop()` once `autoStopMs` (default 100ms) has passed without a fresh `forward()`/`backward()` call. This is non-blocking (no `delay()`), so a single BLE button event spins the motor only briefly, while a continuous stream of events (e.g. a held-down button re-sending every <100ms) keeps it running — and a dropped BLE connection or stuck app can't leave a motor spinning forever.
- `lib/servo_control/servo_control.h` / `.cpp` — `ServoControl`: thin wrapper around `ESP32Servo`'s `Servo` class for a single 180° servo. `begin(pin, initialAngle=90)` attaches and centers the servo, `setAngle(angle)` clamps to 0–180 and writes it, `getAngle()` returns the last-set angle.

Not yet implemented: wiring `TtMotorControl`/`ServoControl` up to the BLE button callback in `main.cpp` (today `onButtonPressed` only prints to Serial; the two new libraries were added but not yet instantiated/used in `main.cpp`).

## Build / upload / monitor

This project uses [PlatformIO](https://platformio.org/), not Arduino IDE or a generic Makefile.

```bash
pio run                 # build
pio run -t upload       # build and flash to the connected ESP32
pio device monitor       # open serial monitor (115200 baud, matches platformio.ini)
pio run -t upload -t monitor   # upload then immediately monitor
pio run -t clean         # clean build artifacts
```

There are no automated tests configured; verification is done by flashing to real ESP32 hardware and observing serial output / device behavior over WiFi/Bluetooth.

## Configuration notes

- Target environment: `esp32` (board `esp32dev`, `framework = arduino`), defined in `platformio.ini`.
- Uses PlatformIO's default partition table (no `board_build.partitions` override) — removed `min_spiffs.csv` since the project doesn't use SPIFFS. Confirmed by build: BLE alone uses ~85.6% of the default 1.25MB app partition, so there's headroom for servo/motor logic but not a lot — revisit the partition table (e.g. `min_spiffs.csv` or `huge_app.csv`, both under `~/.platformio/packages/framework-arduinoespressif32/tools/partitions/`) if flash usage climbs further or SPIFFS/LittleFS/OTA is added later.
- Library dependency: `madhephaestus/ESP32Servo` — use this library (not the stock Arduino `Servo.h`) for any servo PWM output, since standard `Servo.h` is not ESP32-compatible.
- Add new library dependencies under `lib_deps` in `platformio.ini`.

## 進度記錄

- 2026-10-04：建立 `CLAUDE.md`，記錄專案目前僅有 `platformio.ini`（ESP32 + Arduino framework + ESP32Servo 函式庫），尚無任何原始碼；補上建置/上傳/監控指令與設定說明。
- 2026-10-04：補上工作規則（優先使用正體中文回答、每次對話前先讀本檔案、對話結束後將進度補到本檔案）。
- 2026-10-04：建立 `lib/wifi_control/wifi_control.h` 與 `wifi_control.cpp`：ESP32 以 WiFi AP 模式開熱點（SSID/密碼目前寫在 `main.cpp` 開頭），內建 WebServer 在 `/` 提供遙控網頁（方向鍵 + BTN1~BTN6 六個待命名按鍵），按鍵用 `fetch('/control?btn=名稱')` 回報給 ESP32，`/control` 收到後透過 callback 回傳按鍵名稱字串。同時建立 `src/main.cpp`，註冊 callback 把收到的按鍵名稱印到 Serial monitor。本機沒有 `pio` CLI 可驗證編譯，待使用者在自己環境跑 `pio run` 確認。
- 2026-10-23：移除 `platformio.ini` 裡的 `board_build.partitions = min_spiffs.csv`，改用 PlatformIO 預設分割表。原因：專案目前完全沒用到 SPIFFS（網頁 HTML 是用 `PROGMEM` 字串嵌在程式碼裡），所以不需要刻意縮小 SPIFFS 區；追求設定越簡單越好。
- 2026-10-23：新增 `lib/ble_control/ble_control.h` 與 `ble_control.cpp`：ESP32 以 BLE Peripheral 角色廣播（裝置名稱可自訂），開一個可寫入的 GATT characteristic，手機等 BLE 主機連線後把按鍵名稱字串寫進去就會觸發 `onButtonPressed` 回呼，介面設計跟原本 `WifiControl` 的 `ButtonCallback` 一致。接上 `main.cpp` 實際編譯驗證過（`pio run` 成功）。
- 2026-10-23：原本想讓 WiFi（`WifiControl`）跟 BLE（`BleControl`）同時跑，但編譯後 Flash 用量 119.8%（1,569,793 / 1,310,720 bytes）超過預設分割表上限 —— WiFi + BLE 兩個協定疊加起來太大。使用者決定拿掉 WiFi，只保留 BLE 當遙控方式；因此刪除整個 `lib/wifi_control/` 資料夾與 `main.cpp` 裡的 AP SSID/密碼設定。拿掉 WiFi 後 Flash 用量降回 85.6%（1,121,981 bytes），編譯成功。
- 2026-10-23：新增 `lib/tt_motor_control/`（控制 TT 馬達，透過 L298N/TB6612 這類驅動板）與 `lib/servo_control/`（控制 180 度伺服機，底層用 `ESP32Servo`）。兩者都暫時接到 `main.cpp` 實際編譯測試過（Flash 86.6%，含 BLE+兩個新函式庫），確認可行後把測試用的接線程式碼從 `main.cpp` 移除，`main.cpp` 目前仍只有 BLE 控制、Serial 印按鍵名稱，尚未真的拿按鍵去驅動馬達/伺服機。TT 馬達那邊刻意讓 PWM channel 從 15 往下配，是為了跟 `ESP32Servo` 從 0 往上配的 channel 分配方式錯開，避免兩邊之後一起用時搶 channel。
- 2026-10-23：新增 `include/config.h`，把 BLE 裝置名稱、馬達腳位（`MOTOR1_IN1_PIN`/`MOTOR1_IN2_PIN`/`MOTOR1_PWM_PIN`）、伺服機腳位（`SERVO_PIN`/`SERVO_INITIAL_ANGLE`）都集中成 `#define` 常數；`main.cpp` 的 BLE 裝置名稱改成讀這裡的 `BLE_DEVICE_NAME`。目前腳位數值（16/17/18/19）是沿用先前測試時的佔位值，還沒對應到真實接線，之後如果接線腳位不一樣要記得回來改這個檔案。馬達/伺服機的物件還沒有在 `main.cpp` 建立或使用，只有常數先備好。
- 2026-10-23：加第二顆 TT 馬達跟第二顆伺服機。`config.h` 新增 `MOTOR2_IN1_PIN`/`MOTOR2_IN2_PIN`/`MOTOR2_PWM_PIN`（25/26/27）與 `SERVO2_PIN`/`SERVO2_INITIAL_ANGLE`（32/90），並把原本的 `SERVO_PIN`/`SERVO_INITIAL_ANGLE` 改名成 `SERVO1_PIN`/`SERVO1_INITIAL_ANGLE` 跟 `MOTOR1_*` 對稱。`main.cpp` 正式宣告 `motor1`/`motor2`（`TtMotorControl`）與 `servo1`/`servo2`（`ServoControl`）四個物件，`setup()` 裡各自呼叫 `begin()` 接上 `config.h` 的腳位，編譯驗證過（Flash 86.6%）。目前還只是初始化，按鍵（BLE 收到的字串）還沒接去驅動這四個物件的動作。
- 2026-10-23：`onButtonPressed` 加上 `switch` 判斷收到的 BLE 字串第一個字元，依使用者指定的對應表驅動馬達/伺服機：`a`=兩顆馬達正轉、`c`=兩顆馬達反轉、`b`=servo1 正轉+servo2 反轉、`d`=servo1 反轉+servo2 正轉（伺服機正轉/反轉用角度 ±`SERVO_STEP_DEGREES`，新增到 `config.h`，預設 10 度，透過既有的 `getAngle()`/`setAngle()` 自動夾在 0~180 度）。沒對應到的字元會印「無法識別的按鍵」，不會動作。編譯驗證過（Flash 86.6%）。目前還沒有「停止馬達」的按鍵，若之後有停止/速度調整需求要再補。
- 2026-10-23：使用者回報「motor1（L298N output1/2）正轉正常、反轉完全不動」，已換過 3 片不同的 L298N 排除驅動板硬體問題。檢查 `tt_motor_control.cpp` 的 `forward()`/`backward()` 邏輯對稱且正確（`backward()` = IN1 LOW、IN2 HIGH），判斷不是程式邏輯問題。懷疑是 `MOTOR1_IN2_PIN`（原本是 GPIO17）這支腳位沒辦法正常輸出 HIGH——因為正轉只需要 IN2=LOW（沒驗證過 HIGH 輸出），反轉才第一次需要 IN2=HIGH；GPIO16/17 在某些 ESP32 模組（尤其內建 PSRAM 的 WROVER 系列）是保留給內部用途的腳位，當一般 GPIO 常常不穩定。已把 `config.h` 的 `MOTOR1_IN2_PIN` 從 17 改成 13。
  - 後續實測：換成 GPIO13 後反轉依然不會動，但另一顆馬達（output3/4, MOTOR2_IN1/IN2）反轉正常 → 排除 GPIO 腳位問題，問題收斂到 output1/2 這一側的硬體。
  - **最終找到根因**：使用者把原本接在 IN3/IN4/ENB 的那幾條線，改接到 IN1/IN2/ENA，反轉就正常了——證實問題出在**接線本身（杜邦線材）**有問題（可能某條線內部斷線/接觸不良），跟程式碼、L298N 驅動板、馬達本體都無關。問題已解決，`MOTOR1_IN2_PIN` 維持用 GPIO13（跟 17 一樣都沒問題，純粹沿用）。
- 2026-10-23：`TtMotorControl` 加上非阻塞式的自動停止（dead-man's switch）：`begin()` 多一個 `autoStopMs` 參數（預設 100ms），`forward()`/`backward()` 會記錄呼叫時間，新增的 `handle()`（要在 `loop()` 裡持續呼叫）偵測到超過這段時間沒有新的 `forward()`/`backward()` 呼叫就自動 `stop()`。目的：按鍵對應的是 BLE 傳來的單次字元事件，如果沒有這個機制，馬達收到一次 `a`/`c` 就會永遠轉下去；現在變成「有持續收到才會一直轉，斷線或沒再按就在 0.1 秒內自動停」。`main.cpp` 的 `loop()` 也加了 `motor1.handle()`/`motor2.handle()`。編譯驗證過（Flash 86.6%）。
