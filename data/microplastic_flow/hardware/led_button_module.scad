// ============================================================
//  PL9823 × タクトスイッチ 押しボタンモジュール
//  PL9823 × tact switch push-button module
//
//  LEDを指で押す → プランジャーが下がる → 下のタクトスイッチが押される
//  Press the LED → the plunger moves down → the tact switch below is pressed
//
//  部品 / Parts (3つ / 3 pieces):
//    base    : 土台。タクトスイッチを入れる / Base that holds the tact switch
//    plunger : LEDを載せて上下する部品 / Moving part that carries the LED
//    lid     : ふた。プランジャーのガイド / Lid that guides the plunger
//
//  part の値を変えて、部品ごとにSTLを書き出す
//  Change "part" to export each piece as STL
// ============================================================

part = "print";   // "print"=印刷用に並べる / print layout
                  // "assembly"=組み立て図 / assembled view
                  // "section"=断面図 / cross-section
                  // "base" / "plunger" / "lid" = 単体 / single part

$fn = 64;

// ---------------- 部品の実寸（ノギスで測って合わせる）/ Real part sizes (measure and adjust) ----------------
// PL9823-F5（5mm）
led_d         = 5.0;    // LED本体の直径 / LED body diameter
led_flange_d  = 5.8;    // LED根元のつばの直径 / LED flange (rim) diameter
led_flange_h  = 1.0;    // つばの厚さ / Flange thickness
led_h         = 8.6;    // LEDの高さ（足を除く）/ LED height (without legs)

// 2本足タクトスイッチ（6×6mm）/ 2-pin tact switch (6×6 mm)
sw_w          = 6.0;    // 本体の幅 / Body width
sw_d          = 6.0;    // 本体の奥行き / Body depth
sw_body_h     = 3.5;    // 本体の高さ（ボタン除く）/ Body height (without actuator)
sw_h          = 5.0;    // ボタン上面までの高さ / Height to the top of the actuator
sw_pin_span   = 6.5;    // 2本の足の間隔 / Distance between the 2 pins

// ---------------- 設計値 / Design values ----------------
tol        = 0.3;    // はめあいのすき間（きつければ大きく）/ Fit clearance (increase if too tight)

outer_d    = 30;     // モジュールの直径 / Module diameter
wall       = 2;      // 壁の厚さ / Wall thickness
floor_t    = 3;      // 底の厚さ / Floor thickness
inner_d    = outer_d - 2 * wall;

plunger_d  = 11;     // プランジャーの直径 / Plunger diameter
tube_wall  = 1.6;    // プランジャーの壁 / Plunger wall
plate_d    = 15;     // プランジャー下のつば（抜け止め）/ Plunger bottom flange (keeps it from falling out)
plate_t    = 1.6;    // つばの厚さ / Flange thickness
cap_t      = 2.5;    // LEDを載せる天井の厚さ / Thickness of the top that holds the LED
play       = 0.4;    // つばとふたのすき間（遊び）/ Gap between flange and lid (free play)
expose     = 5;      // ふたから出るプランジャーの高さ / Plunger height above the lid

lid_t      = 3.0;    // ふたの厚さ / Lid thickness
lip_h      = 2;      // ふたの差し込み部の高さ / Height of the lid's insert lip
lip_t      = 1.5;    // 差し込み部の厚さ / Lip thickness

cable_w    = 8;      // ケーブル穴の幅 / Cable opening width
cable_h    = 7;      // ケーブル穴の高さ / Cable opening height
screw_d    = 3.4;    // 固定ネジ穴（M3）/ Mounting screw hole (M3)
screw_r    = 9.5;    // ネジ穴の中心からの距離 / Screw hole distance from center

// ---------------- 高さの計算 / Height calculation ----------------
z_act      = floor_t + sw_h;          // スイッチのボタン上面 / Top of switch actuator
z_plate    = z_act;                   // プランジャー下面（スイッチに乗る）/ Plunger bottom (rests on switch)
z_lid      = z_plate + plate_t + play;// ふたの下面 = 土台の高さ / Lid bottom = base height
base_h     = z_lid;
z_top      = z_lid + lid_t + expose;  // プランジャー上面 / Plunger top
plunger_h  = z_top - z_plate;

// ============================================================
// 土台 / Base
// ============================================================
module base() {
  difference() {
    cylinder(d = outer_d, h = base_h);

    // 中の空間 / Inner cavity
    translate([0, 0, floor_t]) cylinder(d = inner_d, h = base_h);

    // ケーブル穴（-Y側）/ Cable opening (-Y side)
    translate([-cable_w / 2, -outer_d / 2 - 1, -0.01]) cube([cable_w, wall + 2, cable_h]);

    // スイッチの足を通す穴 / Slot for the switch pins
    translate([-(sw_pin_span + 2) / 2, -1.0, -0.01]) cube([sw_pin_span + 2, 2.0, floor_t + 0.02]);

    // 裏側のはんだ・配線用のくぼみ / Recess underneath for solder joints and wires
    translate([-(sw_pin_span + 3) / 2, -3, -0.01]) cube([sw_pin_span + 3, 6, 1.5]);
    translate([-3, -outer_d / 2 - 1, -0.01]) cube([6, outer_d / 2 + 1, 1.5]);

    // 固定ネジ穴（左右）/ Mounting screw holes (left and right)
    for (a = [0, 180]) rotate([0, 0, a]) translate([screw_r, 0, -0.01]) {
      cylinder(d = screw_d, h = floor_t + 0.02);
      translate([0, 0, floor_t - 1.8]) cylinder(d1 = screw_d, d2 = 6.4, h = 1.81);  // 皿ネジ用 / countersink
    }
  }
  // スイッチを囲む枠 / Frame around the switch
  translate([0, 0, floor_t]) difference() {
    translate([-(sw_w + tol) / 2 - 1.2, -(sw_d + tol) / 2 - 1.2, 0])
      cube([sw_w + tol + 2.4, sw_d + tol + 2.4, 2]);
    translate([-(sw_w + tol) / 2, -(sw_d + tol) / 2, -0.01])
      cube([sw_w + tol, sw_d + tol, 2.02]);
    // 足が出る側は枠を切る / Open the frame where the pins come out
    translate([-(sw_pin_span + 2) / 2 - 1, -1.0, -0.01]) cube([sw_pin_span + 4, 2.0, 2.02]);
  }
}

// ============================================================
// プランジャー（LEDを載せる）/ Plunger (carries the LED)
// ============================================================
module plunger() {
  inner = plunger_d - 2 * tube_wall;
  difference() {
    union() {
      cylinder(d = plate_d, h = plate_t);              // 抜け止めのつば / Flange
      cylinder(d = plunger_d, h = plunger_h);          // 筒 / Tube
    }
    // 筒の中（配線の通り道）/ Inside of the tube (wire path)
    translate([0, 0, plate_t]) cylinder(d = inner, h = plunger_h - plate_t - cap_t);

    // LEDのつばが収まるくぼみ / Recess for the LED flange
    translate([0, 0, plunger_h - led_flange_h]) cylinder(d = led_flange_d + tol, h = led_flange_h + 0.01);
    // LEDの足を通す穴 / Hole for the LED legs
    translate([0, 0, plunger_h - cap_t - 0.01]) cylinder(d = 4.8, h = cap_t + 0.02);

    // 配線を外へ出す溝（つばの中心から端まで）/ Slot to bring wires out (center of flange to edge)
    translate([2.2, -1.75, -0.01]) cube([plate_d, 3.5, plate_t + 0.02]);
    translate([2.2, -1.75, -0.01]) cube([inner / 2 - 2.2 + 0.01, 3.5, plate_t + 1.5]);
  }
}

// ============================================================
// ふた / Lid
// ============================================================
module lid() {
  difference() {
    union() {
      cylinder(d = outer_d, h = lid_t);
      // 土台に差し込む部分 / Lip that fits into the base
      translate([0, 0, -lip_h]) difference() {
        cylinder(d = inner_d - tol, h = lip_h);
        translate([0, 0, -0.01]) cylinder(d = inner_d - tol - 2 * lip_t, h = lip_h + 0.02);
      }
    }
    // プランジャーが通る穴 / Hole for the plunger
    translate([0, 0, -lip_h - 0.01]) cylinder(d = plunger_d + 2 * tol, h = lid_t + lip_h + 0.02);
    // 上面のふち取り / Chamfer on the top edge
    translate([0, 0, lid_t - 0.6]) cylinder(d1 = plunger_d + 2 * tol, d2 = plunger_d + 2 * tol + 1.2, h = 0.61);
  }
}

// ---------------- 参考用の部品 / Reference parts (not printed) ----------------
module ref_switch() {
  color("dimgray") translate([-sw_w / 2, -sw_d / 2, floor_t]) cube([sw_w, sw_d, sw_body_h]);
  color("black") translate([0, 0, floor_t + sw_body_h]) cylinder(d = 3.5, h = sw_h - sw_body_h);
}
module ref_led() {
  color("white", 0.8) translate([0, 0, z_top - led_flange_h]) {
    cylinder(d = led_flange_d, h = led_flange_h);
    cylinder(d = led_d, h = led_h - led_d / 2);
    translate([0, 0, led_h - led_d / 2]) sphere(d = led_d);
  }
}

module assembly() {
  color("lightsteelblue") base();
  color("orange") translate([0, 0, z_plate]) plunger();
  color("lightgreen", 0.9) translate([0, 0, z_lid]) lid();
  ref_switch();
  ref_led();
}

// ============================================================
if (part == "base")          base();
else if (part == "plunger")  plunger();
else if (part == "lid")      translate([0, 0, lid_t]) rotate([180, 0, 0]) lid();   // 上面を下にして印刷 / print top-down
else if (part == "assembly") assembly();
else if (part == "section")  difference() { assembly(); translate([-50, 0, -10]) cube([100, 50, 60]); }
else {
  // 印刷用レイアウト / Print layout
  base();
  translate([outer_d + 6, 0, 0]) plunger();
  translate([0, outer_d + 6, lid_t]) rotate([180, 0, 0]) lid();
}
