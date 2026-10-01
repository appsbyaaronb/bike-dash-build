# Build log

## 2026-09-18 - research and ordering

- Re-checked whether the Waveshare ESP32-P4-7B was still the right base. Compute: yes. Panel: no, 350 nits.
- Surveyed every ESP32-P4 all-in-one board on sale (Waveshare 7B / 7-8-10.1 HMI / 3.4C / 4C, Guition JC1060P470, Elecrow CrowPanel Advance 7, M5Stack Tab5). All 350-450 nits, all 0-60 C. None sunlight-readable.
- Found Riverdi RVT70HSMNWC00-B: 850 nits optical-bonded touch, -20 to 70 C, EK79007 controller, 2-lane capable per its own init code. $113.52 direct, $116.07 DigiKey (10 in stock).
- Confirmed ESPHome `mipi_dsi` has `model: CUSTOM` with `init_sequence`, `lanes: 2`, and a `WAVESHARE-P4-NANO-10.1` preset exists for the board.
- Confirmed the Riverdi backlight needs an external constant-current driver (9.6 V / 270 mA). Chose PT4115 modules (12 V in) and a Pololu D36V28F5 for 5 V.
- Amazon cart built (P4-NANO, 40P + 22P breakouts, PT4115 x3, Pololu). $71.15. Panel to be ordered from DigiKey.
- Rejected: Pi/CM5 (boot time), STM32H7 (no BLE), Amazon "1000 nit" HDMI kits (no HDMI on P4).
- Open items: which of GPIO37/38 on the P4-NANO DSI connector is LCD reset; PT4115 sense resistor value on the modules that arrive; 22-pin FFC cable orientation.
- Caught later the same day: the GPS (speed source) had been left out of this repo. Added to the BOM (SparkFun NEO-M9N chip-antenna breakout, $74.95 Prime, 3.3 V, backup battery; Matek M9N-5883 as the fallback; plus a USB-TTL adapter for the one-time 10 Hz / 115200 setup), wiring.md Step 4b, and `uart:` + `gps:` blocks in the YAML on GPIO24/25. Not ordered yet.
- Local repo moved to `C:\Users\anon\Nextcloud\server\bike-dash-build` (Nextcloud-synced).
- All core parts (rows 1-8 in bom.md) ordered 2026-09-18. Bench extras (22-pin FFC cable, jumpers, 12 V supply) still to buy when the boxes arrive.
- Enclosure mockup (gaming PC, OpenSCAD 2021.01): four parts (bezel, retainer plate, shell, GPS cap) in `enclosure/`, STLs and previews exported. P4-NANO outline (50 x 50, holes 45.10 x 45.09) and SparkFun M9N board drawing (40.64 x 33.02, holes 35.56 x 27.94) fetched from the vendor drawings. The Riverdi -B touch datasheet 404s on every revision tried, so the glass outline is from the listing and tagged VERIFY. PT4115 and breakout boards are on adhesive pads until measured.

## 2026-09-28 - first flash, BMS live, iPhone remote

- Flashed the P4-NANO from the gaming PC (ESPHome 2026.9.0, venv at `~/.venvs/esphome`). It shows up as `/dev/ttyACM1` through the CH343.
- ESPHome 2026.9 needs an `esp_ldo` block (channel 3, 2.5 V) for `mipi_dsi`; added to `bike-dash-p4nano.yaml`.
- The full config bootloops (abort during setup) with the display section enabled and no panel attached. `bike-dash-nodisplay.yaml` is the same config without the display section, and it boots clean. Retest with the panel connected.
- The C6 only does 2.4 GHz, so it joins `IoT2.4`. Fixed IP 192.168.7.18 set on the device (`manual_ip`).
- BLE failed at first: "Co-processor not responding". The factory C6 firmware was too old. Built the esp_hosted 2.12.12 slave from the managed component source (`idf.py set-target esp32c6 build`), embedded it (`firmware/c6_hosted_2.12.12.bin`) and installed it with the `esp32_hosted` update entity. After that, `Co-processor firmware 2.12.12` and BLE comes up.
- BMS over the C6 works: `ANT-BLE22AAUB-8066` auto-discovered, SOC 100 %, 87.88 V, 59.97 Ah, 21 C.
- iPhone remote (`components/ams`): Apple Media Service client, control only like the old Rockford PMX-BTUR (no audio to the dash). Lessons: let the phone start pairing (both sides starting it gave a DHKey mismatch), look up the CCCDs instead of assuming handle+1, and subscribe to the Remote Command characteristic or iOS ignores the commands.
- Dash page (`components/dash_page`, `dash.html`) served from the board at `http://192.168.7.18:8080/`. It reads live data from `/events` on :80 and sends media buttons to `POST /cmd?c=N` on :8080. Sending them straight to :80 from the page did nothing. It has a Simulate data demo mode.
- Range is a placeholder (60 mi full charge x SOC). Speed shows 0 until the NEO-M9N is wired.
- Later the same day:
  - The log page moved to its own port, `:8081`. It uses a fixed 32 kB ring buffer in PSRAM, so it can't grow. It has Clear log and Clear Bluetooth buttons. Once the BMS connects, Bluetooth lines are purged and no longer logged (`quiet_ble_when`).
  - Phone dash on `:8082`, laid out for a 19.5:9 screen and scaled to the window. JS shrinks the content until no section overflows. Brave on iOS kept serving a cached old page, so reload if a change doesn't show.
  - `dash_page` takes a list of pages, and each page gets its own port and httpd control port. `LWIP_MAX_SOCKETS` is now 32: with 13, the three and then four HTTP servers plus the browsers' open data connections stopped every port answering.
  - Dash buttons: Restart board (tap twice) and Restart Bluetooth (BLE off 2 s, then on, and BMS discovery resets). The AMS component re-registers after a BLE restart. Before that fix it left a stale link (`ESP_ERR_INVALID_STATE`).
  - Status words are coloured red or green with no dots: `GPS:<sats>`, `BMS`, `REC` (green while the log runs), and `wifi` when there's no IP. The IP and ports are on two lines, top right of the speed panel.
  - Second Wi-Fi network: the iPhone hotspot `aaron` (DHCP, needs Maximize Compatibility). `IoT2.4` keeps priority and the fixed .18.
  - The BMS stopped advertising late in the day. Neither the board nor the PC could see it, even after a power cycle. Suspect range or another connection holding it. Not resolved.
- Backlight driver change: the PT4115 modules have a 1R0 sense resistor (100 mA, about 37 % brightness). Ordered an eletechsup LD24AJTA (AliExpress, $1.15, due Oct 04-09): current set by a pot to 270 mA on a meter first. Bare pads, six wires. Diagram, wiring.md, hardware.md and BOM updated.
- GPS UART moved from GPIO24/25 (USB-Serial-JTAG pins, ESPHome warned) to GPIO20 TX / GPIO21 RX, header P1 pins 13/15. Configs, diagram, wiring.md, hardware.md, layout updated.

## 2026-10-01 - wrong DSI connector caught, full check against the datasheets

- The P4-NANO DSI socket is **15-pin 1.0 mm** (schematic J1, "15PIN--PI4B"), not 22-pin 0.5 mm. The 22P breakout and 22-pin FFC are unusable. Ordered a MECCANIXITY 15-pin 1.0 mm breakout ($8.49) and a uxcell 15-pin 1.0 mm 100 mm FFC 10-pack ($5.99), both due Oct 3. Pinout read from the schematic and written into wiring.md Step 4.
- Riverdi now publishes the datasheet, drawing and backlight app note for the exact -B part (module revision V1.1A; ours is V1.0A). The old no-touch Rev 1.4 link is dead. Checking everything against them found:
  - Panel pins **33 (L/R) and 34 (U/D)** set the scan direction and must be tied (33 to 3.3 V, 34 to GND). The docs had them as "leave open" and put scan direction in pins 22-30. Pins 22, 25, 30 are GND; 23, 24, 26-29 NC.
  - Sync pulse widths 70/10 made one line 1414 clocks, over the 1400 limit. Now 10/1 (Espressif's values for the EK79007).
  - Backlight: 270 mA is the absolute maximum (30 mA x 9 strings), not a midpoint, and there is no 315 mA figure. Target is now 260 mA. Vf is 9.0 V typ.
  - Touch: I2C address 0x41 confirmed, I2C VDD is 3.3 V only. The diagram's "try 5 V if silent" note was wrong and is gone. wiring.md had no touch step; added Step 4a.
  - VDD is 3.0-3.6 V at 110 mA (not 2.6 V / 168 mA). THS_ZERO target is about 213 ns.
- Checked and correct: header P1 pin table against the schematic, C6 SDIO pins, the 40-pin MIPI/reset/STBYB/LED pins, init sequence bytes, the touch tail pinout, GPS wiring. All three YAML files validate. No secrets in tracked files.
- Enclosure: not touched, by decision; it gets redone once everything works and is measured. Findings saved for then: TFT body is off-centre in the glass (5.90 right / 9.16 left / 11.58 top / 7.42 bottom, seen from the front); module is 7.68 mm thick max so the model's 5.7 mm TFT depth is about 0.9 mm short; the main FPC is 29.5 mm wide, about 1 mm right of the glass centre; the touch FPC is a second 35 mm tail about 49 mm right of centre and has no slot in the bezel or retainer; the M3 x 35, M2.5 x 8, M2 x 6 and GPS M3 x 6 screws are all longer than their holes are deep.
