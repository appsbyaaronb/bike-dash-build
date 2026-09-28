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
