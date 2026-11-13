// ============================================================
//  マイクロプラスチック密度 × ボタン × 蛇行させたWS2812Bテープ
//  Microplastic density × buttons × serpentine WS2812B LED strip
//
//  Arduino UNO R4 Minima / 必要ライブラリ: Adafruit NeoPixel
//  Arduino UNO R4 Minima / Required library: Adafruit NeoPixel
//
//  データと動きの設定は data.h（自動生成）から読み込みます。
//  Data and behavior settings are loaded from data.h (auto-generated).
//
//  挙動を変えたいときは / To change the behavior:
//    1. japan_4_regionen_interpoliert.csv / settings.csv を編集
//       Edit japan_4_regionen_interpoliert.csv / settings.csv
//    2. python make_data_h.py を実行（data.h が作り直される）
//       Run python make_data_h.py (data.h is regenerated)
//    3. Arduino IDE で書き込み
//       Upload from the Arduino IDE
//
//  ・増加 … 下から上へ光が流れる（暖色）
//    Increase … light flows from bottom to top (warm colors)
//  ・減少 … 上から下へ光が流れる（寒色）
//    Decrease … light flows from top to bottom (cool colors)
//  ・同じ行のLEDは常に同じタイミング・同じ明るさ
//    LEDs in the same row always light at the same time and brightness
//  ・変化が大きいほど速く、色も濃くなる
//    The bigger the change, the faster the flow and the deeper the color
//  ・各ボタンの横の目印LED（PL9823）は、押した地点だけが同じ色で光る
//    The marker LED (PL9823) next to each button lights in the same color for the pressed region
// ============================================================
#include <Adafruit_NeoPixel.h>
#include "data.h"

// 関数の宣言 / Function declarations
void  readSerial();
void  readButton(uint8_t r, unsigned long now);
float trendRatio(uint8_t r, int yearIdx);
void  drawRegion(uint8_t r, float value, float ratio, float dt);
void  fillRow(uint8_t r, uint8_t row, uint32_t c);
void  onPress(uint8_t r);
uint16_t flowHue(float ratio);

// ---------------- 基本設定 / Basic settings ----------------
#define LED_PIN      6           // データ線のピン / Data pin
#define TOTAL_LEDS   150         // テープのLED総数 / Total LEDs on the strip
#define BRIGHTNESS   80          // 全体の明るさ上限 0〜255 / Max brightness 0–255

// ---------------- 目印LED（PL9823）/ Marker LEDs (PL9823) ----------------
//  ボタンの横に1個ずつ。CSVの列の順番 = 目印LEDのつなぐ順番（D11 → 1個目 → 2個目 → …）
//  One next to each button. CSV column order = chain order (D11 → 1st → 2nd → …)
#define MARKER_PIN      11       // 目印LEDのデータピン / Data pin for marker LEDs
#define MARKER_IDLE     15       // 待機中の明るさ（0で消灯）/ Idle brightness (0 = off)
#define MARKER_ON       200      // 点灯中の明るさ / Brightness while active

const unsigned long HOLD_MS  = 3000;  // 再生終了後に消えるまで / 固定年モードの点灯時間
                                      // Time before turning off after playback / on-time in fixed-year mode
const unsigned long FRAME_MS = 20;    // 描画間隔（約50fps）/ Frame interval (~50 fps)

// ---------------- 蛇行パネルの形 / Serpentine panel layout ----------------
//  テープは「一番下の行」から始まり、折り返しながら上へ進む想定
//  The strip starts at the bottom row and snakes upward
const uint8_t  NUM_ROWS     = 5;    // 行数 / Number of rows
const uint16_t LEDS_PER_ROW = 30;   // 1行あたりのLED数 / LEDs per row
const uint16_t BEND_LEDS    = 0;    // 折り返し部分のLED数（常に消灯）/ LEDs in each bend (always off)

const float TAIL_ROWS = 2.5;        // 光の尾の長さ（行数）/ Length of the light tail (in rows)

// ---------------- ボタンとパネルの設定 / Buttons and panels ----------------
//  CSVの列の順番 = ボタンの順番（D2, D3, D4, D5, D7, D8, D9, D10）。D6はLED用なので飛ばす
//  CSV column order = button order (D2, D3, D4, D5, D7, D8, D9, D10). D6 is skipped (LED pin)
const uint8_t MAX_REGIONS = 8;
const uint8_t BUTTON_PINS[MAX_REGIONS] = {2, 3, 4, 5, 7, 8, 9, 10};
static_assert(NUM_REGIONS <= MAX_REGIONS, "地点が多すぎます（最大8）/ Too many regions (max 8)");

// 各地点が使うパネルの先頭LED番号 / First LED index of the panel used by each region
//   すべて0 → 1枚のパネルを共有（最後に押した地点が表示される）
//   All 0 → one shared panel (the last pressed region is shown)
const uint16_t PANEL_START[MAX_REGIONS] = {0, 0, 0, 0, 0, 0, 0, 0};

// ---------------- 内部状態 / Internal state ----------------
Adafruit_NeoPixel strip(TOTAL_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);
// PL9823 は色の順番が RGB / PL9823 uses RGB color order
Adafruit_NeoPixel markers(NUM_REGIONS, MARKER_PIN, NEO_RGB + NEO_KHZ800);

const uint16_t PANEL_LEDS = NUM_ROWS * LEDS_PER_ROW + (NUM_ROWS - 1) * BEND_LEDS;

bool          active[NUM_REGIONS]    = {false};
unsigned long pressTime[NUM_REGIONS] = {0};
int           lastYearIdx[NUM_REGIONS];
float         flowPos[NUM_REGIONS]   = {0};   // 光の先頭の位置（行単位）/ Head position of the light (in rows)
uint8_t       selectedYearIdx        = NUM_YEARS - 1;

// ボタンのチャタリング対策用 / For button debouncing
bool          stableState[NUM_REGIONS];
bool          lastReading[NUM_REGIONS];
unsigned long lastChange[NUM_REGIONS];
const unsigned long DEBOUNCE_MS = 30;

unsigned long lastFrame = 0;

// ============================================================
void setup() {
  Serial.begin(115200);
  Serial.setTimeout(50);   // 年の入力待ちで止まらないように / Avoid blocking while reading the year
  for (uint8_t r = 0; r < NUM_REGIONS; r++) {
    pinMode(BUTTON_PINS[r], INPUT_PULLUP);   // 押すと LOW / LOW when pressed
    stableState[r] = lastReading[r] = HIGH;
    lastYearIdx[r] = -1;
  }
  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  strip.clear();
  strip.show();
  markers.begin();
  markers.setBrightness(BRIGHTNESS);
  markers.clear();
  markers.show();
  Serial.println("Ready.");
}

void loop() {
  unsigned long now = millis();

  readSerial();
  for (uint8_t r = 0; r < NUM_REGIONS; r++) readButton(r, now);

  if (now - lastFrame < FRAME_MS) return;
  float dt = (now - lastFrame) / 1000.0;     // 前フレームからの経過秒 / Seconds since last frame
  if (dt > 0.1) dt = 0.1;
  lastFrame = now;

  strip.clear();
  // 目印LEDは待機中の弱い白から始める / Marker LEDs start as a dim white (idle)
  for (uint8_t r = 0; r < NUM_REGIONS; r++) {
    markers.setPixelColor(r, markers.Color(MARKER_IDLE, MARKER_IDLE, MARKER_IDLE));
  }
  for (uint8_t r = 0; r < NUM_REGIONS; r++) {
    if (!active[r]) continue;

    unsigned long elapsed = now - pressTime[r];
    int yearIdx;

    if (SHOW_MODE == 2) {
      // 2つの年を比較 / Compare two years
      if (elapsed >= COMPARE_SHOW_MS) { active[r] = false; continue; }
      yearIdx = COMPARE_TO_YEAR - FIRST_YEAR;
    } else if (SHOW_MODE == 0) {
      // 年ごとに再生 / Play through the years
      unsigned long playLen = YEAR_STEP_MS * NUM_YEARS;
      if (elapsed >= playLen + HOLD_MS) { active[r] = false; continue; }
      yearIdx = min((int)(elapsed / YEAR_STEP_MS), NUM_YEARS - 1);
    } else {
      // シリアルで指定した年 / Year selected via serial
      if (elapsed >= HOLD_MS) { active[r] = false; continue; }
      yearIdx = selectedYearIdx;
    }

    float value = DATA[r][yearIdx];
    float ratio = trendRatio(r, yearIdx);      // >1 なら増加、<1 なら減少 / >1 increase, <1 decrease

    if (yearIdx != lastYearIdx[r]) {           // 年が変わったらシリアルに表示 / Print when the year changes
      lastYearIdx[r] = yearIdx;
      Serial.print(REGION_NAME[r]); Serial.print("  ");
      Serial.print(FIRST_YEAR + yearIdx); Serial.print("  value=");
      Serial.print(value, 3); Serial.print("  change=x");
      Serial.print(ratio, 2);
      Serial.println(ratio >= 1.0 ? "  UP" : "  DOWN");
    }

    drawRegion(r, value, ratio, dt);

    // 押した地点の目印LEDをパネルと同じ色で点灯 / Light the marker in the same color as the panel
    markers.setPixelColor(r, markers.gamma32(markers.ColorHSV(flowHue(ratio), 255, MARKER_ON)));
  }
  strip.show();
  markers.show();
}

// 増加・減少に応じた色相 / Hue for increase or decrease
//   増加=オレンジ→赤、減少=水色→青（変化が大きいほど濃い側へ）
//   Increase = orange→red, decrease = cyan→blue (deeper with bigger change)
uint16_t flowHue(float ratio) {
  float mag = fabs(log10(ratio));
  if (mag > 1.0) mag = 1.0;
  return (ratio >= 1.0) ? (uint16_t)(6000 * (1.0 - mag))
                        : (uint16_t)(32768 + 10922 * mag);
}

// 増加・減少の比率を求める / Compute the increase/decrease ratio
float trendRatio(uint8_t r, int yearIdx) {
  float base;
  if (SHOW_MODE == 2) {
    base = DATA[r][COMPARE_FROM_YEAR - FIRST_YEAR];   // 指定した2つの年を比較 / Compare the two given years
  } else if (TREND_MODE == 0 && yearIdx > 0) {
    base = DATA[r][yearIdx - 1];                      // 前の年と比較 / Compare with the previous year
  } else {
    base = DATA[r][BASE_YEAR - FIRST_YEAR];           // 基準年と比較 / Compare with the base year
  }
  return DATA[r][yearIdx] / base;
}

// ============================================================
//  ★ 光り方はここを書き換える ★ / ★ Edit here to change the lighting ★
//   value : その年の粒子密度（粒子/m³）/ Particle density of that year (particles/m³)
//   ratio : 比較対象に対する倍率（1.5 = 1.5倍に増加、0.5 = 半分に減少）
//           Ratio to the comparison (1.5 = 1.5× increase, 0.5 = halved)
//   dt    : 前フレームからの経過秒 / Seconds since the last frame
// ============================================================
void drawRegion(uint8_t r, float value, float ratio, float dt) {
  bool  up  = (ratio >= 1.0);
  float mag = fabs(log10(ratio));        // 変化の大きさ（10倍で1.0）/ Size of change (10× = 1.0)
  if (mag > 1.0) mag = 1.0;

  // 変化が大きいほど速く流れる（1秒に何行進むか）
  // Bigger change = faster flow (rows per second)
  float rowsPerSec = 2.0 + 10.0 * mag;
  float cycle = NUM_ROWS + TAIL_ROWS;
  flowPos[r] += rowsPerSec * dt;
  while (flowPos[r] >= cycle) flowPos[r] -= cycle;

  // 色：増加=オレンジ→赤、減少=水色→青 / Color: increase = orange→red, decrease = cyan→blue
  uint16_t hue = flowHue(ratio);

  for (uint8_t row = 0; row < NUM_ROWS; row++) {
    // 増加なら下の行から、減少なら上の行から光が来る
    // Increase: light comes from the bottom row; decrease: from the top row
    int k = up ? row : (NUM_ROWS - 1 - row);
    float d = flowPos[r] - k;            // 光の先頭からの距離 / Distance from the light head

    float b = 0;
    if (d >= 0 && d < TAIL_ROWS) b = 1.0 - d / TAIL_ROWS;   // 尾 / Tail
    else if (d < 0 && d > -1.0)  b = 1.0 + d;               // 先頭のふち / Leading edge

    uint8_t v = strip.gamma8((uint8_t)(b * 255));
    fillRow(r, row, strip.ColorHSV(hue, 255, v));
  }
}

// 1行ぶんを同じ色で塗る（蛇行の向きに関係なく行全体が同じになる）
// Fill one row with a single color (the whole row matches regardless of strip direction)
void fillRow(uint8_t r, uint8_t row, uint32_t c) {
  uint16_t start = PANEL_START[r] + row * (LEDS_PER_ROW + BEND_LEDS);
  for (uint16_t i = 0; i < LEDS_PER_ROW; i++) {
    uint16_t idx = start + i;
    if (idx < TOTAL_LEDS) strip.setPixelColor(idx, c);
  }
}

// ============================================================
// ボタンが押されたとき / When a button is pressed
void onPress(uint8_t r) {
  if (active[r]) {                 // 光っている時に押したら消す / Press again while lit to turn off
    active[r] = false;
    return;
  }
  // 同じパネルを使っている他の地点は止める / Stop other regions sharing the same panel
  for (uint8_t s = 0; s < NUM_REGIONS; s++) {
    if (s == r || !active[s]) continue;
    bool overlap = PANEL_START[s] < PANEL_START[r] + PANEL_LEDS &&
                   PANEL_START[r] < PANEL_START[s] + PANEL_LEDS;
    if (overlap) active[s] = false;
  }
  active[r] = true;
  pressTime[r] = millis();
  lastYearIdx[r] = -1;
  flowPos[r] = 0;
}

// ボタンの読み取り（チャタリング対策つき）/ Read a button (with debouncing)
void readButton(uint8_t r, unsigned long now) {
  bool reading = digitalRead(BUTTON_PINS[r]);
  if (reading != lastReading[r]) {
    lastChange[r] = now;
    lastReading[r] = reading;
  }
  if (now - lastChange[r] > DEBOUNCE_MS && reading != stableState[r]) {
    stableState[r] = reading;
    if (stableState[r] == LOW) onPress(r);   // 押された瞬間 / The moment it is pressed
  }
}

// シリアルで年を指定（例: 2021 を送信）/ Set the year via serial (e.g. send 2021)
void readSerial() {
  if (!Serial.available()) return;
  int y = Serial.parseInt();
  if (y >= FIRST_YEAR && y < FIRST_YEAR + NUM_YEARS) {
    selectedYearIdx = y - FIRST_YEAR;
    Serial.print("Year set: "); Serial.println(y);
  }
}
