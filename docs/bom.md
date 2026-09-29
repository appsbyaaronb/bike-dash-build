# Bill of materials

Ordered 2026-09-18 unless noted. Prices are what was on screen that day.

## Core

| # | Part | Vendor | Price | Link | Status |
|---|---|---|---|---|---|
| 1 | Riverdi RVT70HSMNWC00-B, 7" 1024x600 IPS 850 cd/m2 optical-bonded PCAP touch, MIPI-DSI, no frame | DigiKey (SM-RVT70HSMNWC00-B V1.0A) | $116.07 | [DigiKey](https://www.digikey.com/en/products/detail/riverdi/SM-RVT70HSMNWC00-B-V1-0A/25855280) / [Riverdi direct $113.52](https://riverdi.com/product/high-brightness-ips-display-rvt70hsmnwc00-b-7-inch-projected-capacitive-touch-panel-optical-bonding-uxtouch-mipi-dsi) | ordered 2026-09-18 |
| 2 | Waveshare ESP32-P4-NANO (bare board) | Amazon | $28.79 | [search, first result](https://www.amazon.com/s?k=Waveshare+ESP32-P4-NANO) | ordered 2026-09-18 |
| 3 | MECCANIXITY 40-pin 0.5 mm FPC breakout, 2-pack (panel side) | Amazon | $7.99 | [B09VPHW2QY](https://www.amazon.com/MECCANIXITY-Converter-Socket-2-54mm-Printer/dp/B09VPHW2QY) | ordered 2026-09-18 |
| 4 | MECCANIXITY 22-pin 0.5 mm FPC breakout, 2-pack (P4-NANO DSI side) | Amazon | $9.39 | [B09VPKWL1G](https://www.amazon.com/MECCANIXITY-Converter-2-54mm-Single-Printer/dp/B09VPKWL1G) | ordered 2026-09-18 |
| 5 | PT4115 constant-current LED driver module, 3-pack (backlight) | Amazon | $7.99 | [B0FR1SGWKM](https://www.amazon.com/PT4115-Constant-Current-Dimming-Step-Down/dp/B0FR1SGWKM) | arrived 2026-09-28 with 1R0 (100 mA); replaced by 5b |
| 5b | eletechsup LD24AJTA adjustable CC LED driver, 30-900 mA, PWM (backlight, set to 270 mA) | AliExpress (eletechsup Outlet Store) | $1.15 | [4000340845096](https://www.aliexpress.com/item/4000340845096.html) | ordered 2026-09-28, due Oct 04-09 |
| 5c | 22-pin 0.5 mm FFC cable, 10 cm (P4-NANO DSI to 22P breakout). Check same-side vs opposite-side contacts | Amazon | — | — | ordered 2026-09-28 |
| 5d | MECCANIXITY 10-pin 0.5 mm FPC breakout, 2-pack, shrouded 2x5 header fitted (touch tail) | Amazon | $7.99 | [B0CZ97XHVV](https://www.amazon.com/MECCANIXITY-Connector-Adapter-Converter-Digital/dp/B0CZ97XHVV) | ordered 2026-09-28, due Oct 3 |
| 5e | EDGELEC Dupont jumpers, 120 pcs assorted (F-F / M-M / M-F), 20 cm (power, control, GPS, touch) and 10 cm (the 6 MIPI wires) | Amazon | $6.98 each | EDGELEC store | ordered 2026-09-28 |
| 6 | Pololu D36V28F5 5 V 3.2 A buck, 5.3-50 V in (bike 12 V to 5 V) | Amazon | $16.99 | [B0BJKVWR2D](https://www.amazon.com/Pololu-3-2A-Step-Down-Voltage-Regulator/dp/B0BJKVWR2D) | ordered 2026-09-18 |
| 7 | **SparkFun GPS Breakout NEO-M9N, chip antenna (Qwiic)**: genuine u-blox M9, 25 Hz max, UART pins + I2C, **3.3 V supply and logic**, rechargeable backup battery keeps settings and gives a hot fix | Amazon | $74.95 | [B082YG1PXF](https://www.amazon.com/SparkFun-Breakout-Breadboardable-time-First-f/dp/B082YG1PXF) (Prime) | ordered 2026-09-18 |
| 8 | USB-to-TTL serial adapter, 3.3 V (for the one-time 10 Hz / 115200 setup in u-center) | Amazon | about $8 | any CP2102 or FT232 board with a 3.3 V switch | ordered 2026-09-18 |

All eight core parts ordered 2026-09-18 (Amazon plus the panel from DigiKey).

GPS notes: it is the speed source for the dash (speed and trip come from GPS, not the bike). Many cheap "NEO-M8N" boards on Amazon are clones with old firmware, so the SparkFun board is the pick. Alternative if it is out of stock: [Matek M9N-5883](https://www.amazon.com/s?k=Matek+M9N-5883) ($62.99, genuine, but **5 V supply**, JST-GH pigtail, and no flash: it forgets its settings when its supercap drains, so the `on_boot` block in the YAML becomes mandatory). Whatever module: it must run at **10 Hz** and **115200 baud** (factory default is 1 Hz at 9600 or 38400, too slow for a speedo). See wiring.md Step 4b.

## Still to source (bench)

| Part | Why | Notes |
|---|---|---|
| 22-pin 0.5 mm FFC cable, about 100 mm | P4-NANO DSI connector to the 22-pin breakout | Check contact side (same-side vs opposite) against the NANO connector and breakout. The NANO may ship with one; check the box. |
| Dupont jumper wires, female-female, 100 mm, one pack | Breakout to breakout, breakout to NANO header | Keep the six DSI wires **the same length and bundled together** |
| Screw terminal or small perfboard | To join grounds and 12 V feeds neatly | |
| 12 V bench supply, 2 A | Backlight + buck | A 12 V wall adapter with a barrel jack pigtail is fine |
| Multimeter | Setting the LED driver to 270 mA (10A jack) and the 5 V rail before connecting the panel | |

## Still to source (on-bike)

| Part | Why |
|---|---|
| Carrier PCB (22-pin DSI in, 40-pin panel out, PT4115, buck, connectors) | Replaces the two breakouts and jumper wires. Design after bench bring-up. |
| Enclosure + bezel for the 164.9 x 100 x 5.7 mm panel | Weatherproof, vibration |
| Automotive fuse (2 A) + inline connector on the 12 V feed | |

## Contingency (buy only if Stage 4 fails)

| Part | Why | Notes |
|---|---|---|
| Seeed XIAO ESP32-C6 (about $10, Amazon/Seeed) | Second BLE radio if the onboard C6 cannot hold the BMS link and the iPhone AMS link at once | Runs its own ESPHome firmware as the AMS peripheral; talks to the P4 over a spare UART (3V3, GND, TX, RX on two free header GPIOs). Onboard C6 keeps the BMS. Has a u.FL socket for an external antenna if the enclosure blocks BLE. |

## Alternatives that were rejected (so we do not re-research)

| Part | Why not |
|---|---|
| Waveshare ESP32-P4-WIFI6-Touch-LCD-7B ($47-56) | 350 nits. Fine as a bench unit, not the bike screen. Same ESPHome code otherwise. |
| Elecrow CrowPanel Advance 7" P4 (400 nits), Waveshare 7/8/10.1 HMI (400-450), Guition JC1060P470 | All indoor-brightness, 0-60 C panels |
| Riverdi STM32H7 7" 850-nit ($266) | No BLE, TouchGFX rewrite |
| Riverdi "ESP-P4 series" 10.1" 800-1000 nit all-in-one (coming soon) | Right idea, wrong size, no price or date. Watch it. |
| Amazon "1000 nit 7 inch" kits (VSDISPLAY etc.) | HDMI controller boards. The P4 has no HDMI out. |
| Adafruit 4905 40-pin breakout | Out of stock on Amazon; MECCANIXITY substituted |
