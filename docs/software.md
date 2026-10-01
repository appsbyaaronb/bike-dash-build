# Software plan

The dash application lives in [bike-dash](https://github.com/appsbyaaronb/bike-dash). This repo only covers getting the new hardware to light up. Once the display works, the bike-dash YAML gets its `esp32`, `esp32_hosted`, `display` and `light` blocks swapped for the ones here.

## Versions

| Thing | Requirement |
|---|---|
| ESPHome | 2026.9 or newer. 2026.8 fixed the ESP32-P4 rev3 bootloop; 2026.9 dropped the old `esp_hosted` 2.0.11 path that double-freed on P4+C6 |
| Framework | esp-idf only (no Arduino on P4), IDF 5.3 or newer for `esp32_hosted` |
| Build host | Linux is easiest (the gaming PC on CachyOS). Windows works from PowerShell, **not** MSys/Git-Bash (idf_tools.py rejects it) |

## Bring-up stages

| Stage | Goal | Done when |
|---|---|---|
| 0 | P4-NANO boots ESPHome, C6 comes up | logs show Wi-Fi/BLE from `esp32_hosted`. **Do this with nothing but USB plugged in.** |
| 1 | Riverdi panel initialises | backlight on, screen not black, no DSI errors in the log. The test lambda fills the screen green with a grey box |
| 2 | LVGL hello at 1024x600 | a label renders, no tearing, PSRAM frame buffer |
| 3 | Merge bike-dash UI | the speed-hero layout from bike-dash renders here |
| 4 | BMS BLE over the C6 | `'soc': Sending state NN %` in the log (same GO/NO-GO as bike-dash Stage 1) |
| 5 | GPS, SD logging, phone media | iPhone only. Apple Media Service (AMS) over BLE: **play/pause, volume up/down** are the required controls; next/previous and track title come free with it. Custom ESPHome component (esp-idf). Fallback if AMS fights us: a BLE HID consumer-control device (media keys), which iOS also accepts natively but gives no track info. No Android support needed. |
| 6 | Touch | Wanted. Wired over I2C (wiring.md Step 4a); done-when for the wiring is a device at 0x41 in the I2C scan. ILI2132A is **not** an ESPHome touchscreen platform, so it needs a custom `ili2132` component (port of Linux `ilitek_ts_i2c`). Last, because the dash is useful without it |

## The display block

See [esphome/bike-dash-p4nano.yaml](../esphome/bike-dash-p4nano.yaml). Key points:

- `model: CUSTOM` with `dimensions`, timings and the datasheet `init_sequence` (see hardware.md).
- `lanes: 2` and the `0xB2 0x50` byte in the init sequence must agree.
- Start at `lane_bit_rate: 900Mbps` and `pclk_frequency: 51.2MHz`; if the picture is scrambled try 800-1000 Mbps.
- Sync pulses are 10 (H) and 1 (V). One line must stay within 1200-1400 clocks.
- If it stays black with a clean log: THS_ZERO (hardware.md), then RESET polarity and timing, then STBYB, then `0xB2, 0x10`.
- The config with the display block bootlooped on 2026-09-28 with no panel attached. Retest with the panel wired before assuming a config fault.

## Commands

```
esphome config esphome/bike-dash-p4nano.yaml                          # validate
esphome run    esphome/bike-dash-p4nano.yaml --device /dev/ttyACM1    # Linux (gaming PC: the board is ACM1)
esphome run    esphome/bike-dash-p4nano.yaml --device COM4            # Windows PowerShell
```
