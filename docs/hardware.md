# Hardware reference

This is the reference behind [wiring.md](wiring.md). Read wiring.md to build it; come here when something does not work.

## Riverdi RVT70HSMNWC00-B panel

Riverdi's files for this exact part, in the folder https://download.riverdi.com/RVT70HSMNWC00-B/ : datasheet `DS_RVT70HSMNWC00-B V1.1A_Rev.1.0.pdf` (2026-07-20), drawing `DR_RVT70HSMNWC00-B V1.1A_Rev.1.0.pdf`, backlight app note `AN_RVT70HSMNWC00-B_Rev1.1.pdf`. The old Rev 1.4 link is dead. Do not commit the PDFs, link them.

These documents are for module revision **V1.1A**. The panel bought from DigiKey is listed as **V1.0A**. The pinout matches the 2024 app note, which covers both, so the electrical side is safe; treat the mechanical numbers as approximate until measured. Everything below was re-checked against these files on 2026-10-01.

| Item | Value |
|---|---|
| Controller | EK79007AD3 + EK73217BCGA (datasheet drawing). Espressif ships `esp_lcd_ek79007`; ESPHome `mipi_dsi` drives it as `model: CUSTOM` |
| Resolution | 1024 x 600, IPS, normally black, anti-glare |
| Brightness | 1000 cd/m2 bare; 850 cd/m2 with the optically bonded touch |
| Operating temp | -20 to 70 C (storage -30 to 80) |
| Logic supply | VDD 3.3 V (3.0-3.6), 110 mA typ, 120 max. Touch adds about 98 mA on its own 3.3 V pin. The NANO's 3.3 V regulator is a 3 A buck, so this is fine. |
| Backlight | 27 white LEDs, 9 strings of 3. **Vf 9.0 V typ (8.4-10.2), If 270 mA**, about 2.4 W. Absolute maximum is 30 mA per string, which is 270 mA total: **270 mA is the ceiling, not a midpoint.** No driver on the panel. |
| Touch | ILITEK ILI2132A PCAP, I2C address **0x41** (or USB HID), 10-point, thick-glove and wet operation. Not supported by ESPHome yet. |
| Touch tail | Separate 10-pin FPC off the touch controller board (printed on the tail): 1 GND, 2 VDD5V, 3 D-, 4 D+, 5 GND, 6 VCC, 7 RST, 8 SCL, 9 INT, 10 SDA. Pins 2-4 are a USB HID path, pins 6-10 the I2C path. The controller board also has a small white JST socket (CN1, the same USB signals). Wired over I2C in this build: wiring.md Step 4a. I2C VDD is 3.3 V only. |
| Size | Cover glass 179.96 x 119.00 mm, 1.1 mm thick. TFT body 164.9 x 100 mm. Whole module 7.68 mm thick max. Active area 154.21 x 85.92 mm. |
| Tail | 40-pin 0.5 mm pitch FPC, 0.3 mm thick, contacts on one side |
| Touch tail | 10-pin 0.5 mm pitch FPC, 0.3 mm thick |

### 40-pin FPC pinout (datasheet)

| Pin | Name | Note |
|---|---|---|
| 1 | NC | |
| 2, 3 | VDD | 3.3 V |
| 4 | NC | |
| 5 | RESET | active low; datasheet Note 1 has a recommended RC reset circuit |
| 6 | STBYB | standby, tie to 3.3 V for normal operation |
| 7 | GND | |
| 8, 9 | D0N, D0P | MIPI lane 0 |
| 10 | GND | |
| 11, 12 | D1N, D1P | MIPI lane 1 |
| 13 | GND | |
| 14, 15 | D2N, D2P | lane 2, **leave unconnected** (P4 is 2-lane) |
| 16 | GND | |
| 17, 18 | DCLKN, DCLKP | MIPI clock |
| 19 | GND | |
| 20, 21 | D3N, D3P | lane 3, **leave unconnected** |
| 22 | GND | |
| 23, 24 | NC | |
| 25 | GND | |
| 26-29 | NC | |
| 30 | GND | |
| 31, 32 | LED- | backlight cathode |
| 33 | L/R | horizontal scan direction. **Must be tied**: 3.3 V = left to right (normal) |
| 34 | U/D | vertical scan direction. **Must be tied**: GND = top to bottom (normal) |
| 35-38 | NC | |
| 39, 40 | LED+ | backlight anode |

### Init sequence (datasheet section 13). This is what goes in the ESPHome `init_sequence`

```
0x01            ; DCS software reset
delay 120 ms
0xB2 0x50       ; lanes: 0x50 = 2-lane, 0x60 = 3, 0x70 = 4   <-- P4 uses 2
0x80 0x4B       ; gamma
0x81 0xFF
0x82 0x1A
0x83 0x88
0x84 0x8F
0x85 0x35
0x86 0xB0
0x11            ; exit sleep
delay 120 ms
0x29            ; display on
delay 20 ms
```

### Timing (datasheet, typical / range)

| Parameter | Typ | Range |
|---|---|---|
| Pixel clock | 51.2 MHz | 44.9-63 MHz |
| H active | 1024 | |
| H total (one line) | 1344 | 1200-1400 |
| HSYNC pulse | no typ given; config uses 10 | 1-140 |
| H back porch | 160 | fixed |
| H front porch | 160 | 16-216 |
| V active | 600 | |
| V total | 635 | 624-750 |
| VSYNC pulse | no typ given; config uses 1 | 1-20 |
| V back porch | 23 | fixed |
| V front porch | 12 | 1-127 |

### Gotchas from the datasheet

- **THS_ZERO**: the panel's MIPI receiver does not meet the MIPI minimum. If the host's THS_ZERO sits at the low end the panel may not initialise correctly and the picture "jumps". Riverdi says to set THS_ZERO to about **213 ns** in the DSI PHY. ESPHome has no setting for it; it would be an ESP-IDF PHY timing change.
- The pulse widths are 10 and 1, not 70 and 10: with a 70-wide pulse the line comes to 1414 clocks, over the 1400 maximum. 10/160/160 and 1/23/12 are what Espressif's `esp_lcd_ek79007` driver and the ESPHome Waveshare 7B preset use for the same controller (`lane_bit_rate: 900Mbps`, `pclk 52MHz`).
- That same Espressif driver sends `0xB2, 0x10` for 2 lanes and different gamma bytes. Riverdi's datasheet says `0x50`. The config uses Riverdi's; `0x10` is the first thing to try if the panel stays black.

## Waveshare ESP32-P4-NANO

Wiki: https://www.waveshare.com/wiki/ESP32-P4-Nano-StartPage . Schematic: https://files.waveshare.com/wiki/ESP32-P4-NANO/ESP32-P4-NANO-schematic.pdf

| Item | Value |
|---|---|
| SoC | ESP32-P4NRW32, 32 MB PSRAM in package, 16 MB QSPI flash |
| Radio | ESP32-C6-MINI-1 over SDIO: reset GPIO54, cmd 19, clk 18, d0-d3 = 14-17, active high (ESPHome device page) |
| I2C | SDA GPIO7, SCL GPIO8 (also on the DSI connector) |
| LCD reset (BSP default) | GPIO27 on the GPIO header |
| LCD backlight PWM (BSP default) | GPIO26 on the GPIO header |
| DSI | 15-pin 1.0 mm FPC (J1, "15PIN--PI4B", Raspberry Pi 4 layout), 2 lanes + clock. Pins: 1 GND, 2 D1-, 3 D1+, 4 GND, 5 CLK-, 6 CLK+, 7 GND, 8 D0-, 9 D0+, 10 GND, 11 SCL, 12 SDA, 13 GND, 14-15 3V3. We only use the six MIPI signals and ground from this connector; reset and backlight go from the header instead. |
| Power | USB-C, or 5 V on the header, or PoE module |

Header **P1** (2x13, 2.54 mm), from the schematic. Every pin this build uses is on P1:

| Pin | Signal | Use | | Pin | Signal | Use |
|---|---|---|---|---|---|---|
| 1 | 3V3 | panel VDD/STBYB, GPS, touch VCC | | 2 | 5V | Pololu 5 V in |
| 3 | GPIO7 | I2C SDA (touch) | | 4 | 5V | |
| 5 | GPIO8 | I2C SCL (touch) | | 6 | GND | |
| 7 | GPIO23 | touch RST | | 8 | GPIO37 | |
| 9 | GND | | | 10 | GPIO38 | |
| 11 | GPIO5 | | | 12 | GPIO4 | |
| 13 | GPIO20 | GPS UART TX (P4 → GPS RX) | | 14 | GND | |
| 15 | GPIO21 | GPS UART RX (GPS TX → P4) | | 16 | GPIO22 | touch INT |
| 17 | 3V3 | | | 18 | GPIO24 | USB-Serial-JTAG D−, left free |
| 19 | GPIO25 | USB-Serial-JTAG D+, left free | | 20 | GND | |
| 21 | GPIO26 | backlight PWM | | 22 | GPIO27 | panel RESET |
| 23 | GPIO32 | | | 24 | GPIO33 | |
| 25 | GND | | | 26 | GPIO36 | |

Header P2 carries GPIO0-3, 6, 45-48, 53, 54 and the C6 UART; not used here.

Storage is microSD (SDIO 3.0). The only ESPHome preset for this board is `WAVESHARE-P4-NANO-10.1` (their 10.1" DSI panel); copy only the board-level bits.

## Backlight driver: eletechsup LD24AJTA

- Buck constant-current, 6-24 V in, 30-900 mA set by a pot. Ceiling is 0.1 V / RCS. Ordered 2026-09-28 from AliExpress ($1.15).
- Set to **260 mA** on a meter before connecting the panel, never above 270 (wiring.md step 1). Bare pads: VIN, GND, LED+, LED-, PWM, GND.
- Buck topology: Vin must be above the LED string (9.0 V typ, 10.2 max) plus headroom. 12 V is fine. 5 V is not. Headroom not in the listing; check brightness on the bench at 12 V.
- PWM pad: 100 Hz-20 kHz per the listing. The LEDC output on GPIO26 runs about 5 kHz.
- Riverdi's own reference design drives the backlight with a TPS61169 boost from 2.7-5.5 V and PWM at 5-100 kHz. That is for their chip, not a limit of the panel.
- Replaces the PT4115 modules: those arrived with a 1R0 sense resistor (100 mA, about 37 % brightness). Getting 270 mA from them needs a 0.36-0.39 ohm 1206 resistor swap (DigiKey RL1206FR-070R36L).

## Pololu D36V28F5

5.3-50 V in, 5 V 3.2 A out, reverse-polarity protected, has an EN pin (tie to ignition-switched 12 V if we want the dash off with the key without a relay).

## On-bike carrier PCB (later)

Replace both breakouts and the jumpers with one board: 15-pin 1.0 mm FPC in, 40-pin FPC out, DSI pairs as 100 ohm differential traces, PT4115 + Pololu footprints (or discrete equivalents), fuse, ignition-sense input. Design it only after the bench build shows the panel initialising.

## Antennas

None to buy for the bench. All radios have on-board antennas; the rules for the bike install:

| Radio | Rule |
|---|---|
| BLE, C6 on the P4-NANO (PCB antenna) | Plastic enclosure. Keep the NANO away from the panel's driver strip and the MIPI wires; the e-paper gauge showed display electronics desensing the radio. |
| GPS, SparkFun chip antenna | Must see sky: top of the enclosure, plastic lid, nothing metal above it. If first fix is slow or speed drops out, swap to the SparkFun NEO-M9N u.FL version plus a small active patch antenna (about $12) mounted outside. |
| XIAO C6 (contingency) | Same as BLE; it has a u.FL socket if an external antenna is ever needed. |
| Metal enclosure | Do not. Everything would need external antennas. |
