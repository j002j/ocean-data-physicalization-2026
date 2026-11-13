"""
ピボット済みCSV（＋設定CSV）から Arduino 用の data.h を作るスクリプト
Generates data.h for Arduino from a pivoted CSV (+ settings CSV)

使い方（このファイルがあるフォルダで実行）/ Usage (run in this folder):
    python make_data_h.py
    python make_data_h.py 別のデータ.csv            # データCSVを指定 / specify data CSV
    python make_data_h.py データ.csv 設定.csv        # 両方指定 / specify both

データCSVの形式 / Data CSV format
（ノートブックの output/japan_4_regionen_interpoliert.csv と同じ）
(same as output/japan_4_regionen_interpoliert.csv from the notebook):
    Year,地点A,地点B,...        / Year,RegionA,RegionB,...
    2014,0.706,1.79,...
    2015,...
  ・1列目が年、2列目以降が地点（列の順番 = ボタンの順番 D2, D3, D4, D5, D7, ...）
    Column 1 = year, columns 2+ = regions (column order = button order D2, D3, D4, D5, D7, ...)
  ・空欄は前後の年から直線で補間します
    Blank cells are linearly interpolated from neighboring years
"""
import csv
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
DATA_CSV = Path(sys.argv[1]) if len(sys.argv) > 1 else HERE / "japan_4_regionen_interpoliert.csv"
SETTINGS_CSV = Path(sys.argv[2]) if len(sys.argv) > 2 else HERE / "settings.csv"
OUT_H = HERE / "data.h"

MAX_REGIONS = 8  # スケッチ側のボタンピン数に合わせる / Must match the number of button pins in the sketch

# 標準設定 / Default settings
DEFAULT_SETTINGS = {
    "show_mode": 2, "compare_from_year": 2017, "compare_to_year": 2018,
    "show_ms": 10000, "trend_mode": 0, "base_year": 2017, "year_step_ms": 1500,
}


def fail(msg):
    """エラーを表示して終了 / Print an error and exit"""
    print("エラー / Error:", msg)
    sys.exit(1)


def read_data(path):
    """データCSVを読み込む / Read the data CSV"""
    if not path.exists():
        fail(f"{path} が見つかりません / not found")
    with open(path, encoding="utf-8-sig", newline="") as f:
        rows = [r for r in csv.reader(f) if r and any(c.strip() for c in r)]
    if len(rows) < 2:
        fail(f"{path.name} にデータがありません / contains no data")
    names = [h.strip() for h in rows[0][1:]]
    if not names:
        fail("地点の列がありません（1列目=年、2列目以降=地点）/ "
             "No region columns (column 1 = year, columns 2+ = regions)")
    if len(names) > MAX_REGIONS:
        fail(f"地点は最大 {MAX_REGIONS} 個までです（今は {len(names)} 個）/ "
             f"Max {MAX_REGIONS} regions (currently {len(names)})")

    years, table = [], []
    for r in rows[1:]:
        try:
            years.append(int(float(r[0])))
        except ValueError:
            fail(f"年として読めない値があります / Cannot read as a year: {r[0]!r}")
        vals = []
        for c in (r[1:] + [""] * len(names))[:len(names)]:
            c = c.strip()
            try:
                vals.append(float(c) if c not in ("", "nan", "NaN") else None)
            except ValueError:
                fail(f"数字として読めない値があります / Cannot read as a number: {c!r} ({r[0]})")
        table.append(vals)

    # 年の順に並べ替えて、連続しているか確認 / Sort by year and check continuity
    order = sorted(range(len(years)), key=lambda i: years[i])
    years = [years[i] for i in order]
    table = [table[i] for i in order]
    for a, b in zip(years, years[1:]):
        if b != a + 1:
            fail(f"年が連続していません（{a} の次が {b}）/ Years are not consecutive ({a} → {b})")

    # 地点ごとに並べ替え ＋ 空欄を補間 / Reorder by region + interpolate blanks
    data = []
    for j, name in enumerate(names):
        col = [table[i][j] for i in range(len(years))]
        known = [i for i, v in enumerate(col) if v is not None]
        if not known:
            fail(f"{name} の値がすべて空です / All values of {name} are empty")
        for i, v in enumerate(col):
            if v is not None:
                continue
            prev = max((k for k in known if k < i), default=None)
            nxt = min((k for k in known if k > i), default=None)
            if prev is None:
                col[i] = col[nxt]
            elif nxt is None:
                col[i] = col[prev]
            else:
                col[i] = col[prev] + (col[nxt] - col[prev]) * (i - prev) / (nxt - prev)
            print(f"  補間 / Interpolated: {name} {years[i]} = {col[i]:.4f}")
        if any(v <= 0 for v in col):
            fail(f"{name} に0以下の値があります（比率が計算できません）/ "
                 f"{name} has a value ≤ 0 (ratio cannot be computed)")
        data.append(col)
    return names, years, data


def read_settings(path):
    """設定CSVを読み込む / Read the settings CSV"""
    s = dict(DEFAULT_SETTINGS)
    if not path.exists():
        print(f"  {path.name} がないので標準設定を使います / not found, using defaults")
        return s
    with open(path, encoding="utf-8-sig", newline="") as f:
        for r in csv.reader(f):
            if len(r) < 2 or r[0].strip() in ("", "key"):
                continue
            k = r[0].strip()
            if k not in s:
                print(f"  注意: 知らない設定 {k} は無視します / Note: unknown setting {k} ignored")
                continue
            try:
                s[k] = int(float(r[1]))
            except ValueError:
                fail(f"設定 {k} の値が数字ではありません / Setting {k} is not a number: {r[1]!r}")
    return s


def c_str(s):
    """C言語の文字列リテラルにする / Convert to a C string literal"""
    return '"' + s.replace("\\", "\\\\").replace('"', '\\"') + '"'


def main():
    names, years, data = read_data(DATA_CSV)
    st = read_settings(SETTINGS_CSV)

    # 設定の確認 / Validate settings
    for k in ("compare_from_year", "compare_to_year", "base_year"):
        if not years[0] <= st[k] <= years[-1]:
            fail(f"{k}={st[k]} がデータの年の範囲（{years[0]}〜{years[-1]}）にありません / "
                 f"is outside the data range ({years[0]}–{years[-1]})")
    if st["show_mode"] not in (0, 1, 2) or st["trend_mode"] not in (0, 1):
        fail("show_mode は 0/1/2、trend_mode は 0/1 で指定してください / "
             "show_mode must be 0/1/2 and trend_mode must be 0/1")

    # data.h を書き出す / Write data.h
    lines = [
        "// このファイルは make_data_h.py が自動で作ります。直接編集しないでください。",
        "// This file is auto-generated by make_data_h.py. Do not edit it directly.",
        f"// 元データ / Data: {DATA_CSV.name}   設定 / Settings: {SETTINGS_CSV.name}",
        "#pragma once",
        "",
        f"const uint8_t NUM_REGIONS = {len(names)};   // 地点数 / Number of regions",
        f"const uint8_t NUM_YEARS   = {len(years)};   // 年数 / Number of years",
        f"const int     FIRST_YEAR  = {years[0]};   // 最初の年 / First year",
        "",
        "// 地点名（シリアル表示用）/ Region names (for serial output)",
        "const char* const REGION_NAME[NUM_REGIONS] = {",
        "  " + ", ".join(c_str(n) for n in names),
        "};",
        "",
        "// 年ごとの粒子密度（粒子/m³）/ Particle density per year (particles/m³)",
        "const float DATA[NUM_REGIONS][NUM_YEARS] = {",
        "  // " + "  ".join(str(y) for y in years),
    ]
    for name, col in zip(names, data):
        lines.append("  {" + ", ".join(f"{v:.6g}f" for v in col) + "},  // " + name)
    lines += [
        "};",
        "",
        "// ---- 設定 / Settings ----",
        f"const uint8_t SHOW_MODE  = {st['show_mode']};   // 0=年ごとに再生 / 1=年を指定 / 2=2つの年を比較  (0=play years / 1=select year / 2=compare two years)",
        f"const uint8_t TREND_MODE = {st['trend_mode']};   // 0=前年と比較 / 1=基準年と比較  (0=vs previous year / 1=vs base year)",
        f"const int     BASE_YEAR  = {st['base_year']};   // 基準年 / Base year",
        f"const int     COMPARE_FROM_YEAR = {st['compare_from_year']};   // 比較のもと / Compare from",
        f"const int     COMPARE_TO_YEAR   = {st['compare_to_year']};   // 比較する年 / Compare to",
        f"const unsigned long COMPARE_SHOW_MS = {st['show_ms']};   // 光る時間(ms) / Light duration (ms)",
        f"const unsigned long YEAR_STEP_MS    = {st['year_step_ms']};   // 1年あたりの表示時間(ms) / Time per year (ms)",
        "",
    ]
    OUT_H.write_text("\n".join(lines), encoding="utf-8")

    # 確認用の表示 / Summary for checking
    print(f"\n{OUT_H} を作成しました / created "
          f"（{len(names)}地点 / regions × {years[0]}–{years[-1]}）")
    if st["show_mode"] == 2:
        a, b = st["compare_from_year"], st["compare_to_year"]
        print(f"\n{a} → {b} の比較 / comparison:")
        for name, col in zip(names, data):
            ratio = col[b - years[0]] / col[a - years[0]]
            way = "増加 / increase: 下→上 bottom→top" if ratio >= 1 else "減少 / decrease: 上→下 top→bottom"
            print(f"  {name:28s} x{ratio:5.2f}  {way}")
    print("\nArduino IDE でスケッチを書き込み直すと反映されます。")
    print("Re-upload the sketch from the Arduino IDE to apply the changes.")


if __name__ == "__main__":
    main()
