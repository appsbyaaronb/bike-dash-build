# Hardware reference

This is the reference behind [wiring.md](wiring.md). Read wiring.md to build it; come here when something does not work.

## Riverdi RVT70HSMNWC00-B panel

Datasheet (no-touch sibling, same glass and electronics): [DS_RVT70HSMNWN00 Rev 1.4](https://download.riverdi.com/RVT70HSMNWN00/DS_RVT70HSMNWN00_Rev.1.4.pdf). Do not commit the PDF, link it.

| Item | Value |
|---|---|
| Controller | EK79007AD3 + EK73217BCGA (datasheet drawing). Espressif ships `esp_lcd_ek79007`; ESPHome `mipi_dsi` drives it as `model: CUSTOM` |
| Resolution | 1024 x 600, IPS, normally black, anti-glare |
| Brightness | 1000 cd/m2 bare; 850 cd/m2 with the optically bonded touch |
| Operating temp | -20 to 70 C (storage -30 to 80) |
| Logic supply | VDD 3.3 V (2.6-3.6), about 168 mA |
| Backlight | 27 white LEDs, **Vf 9.6 V (9.0-10.2), If 270 mA typ, 315 mA max**, about 2.6 W. No driver on the panel. |
| Touch | ILITEK ILI2132A PCAP, I2C, 10-point, thick-glove and wet operation, up to 8 mm cover glass. Not supported by ESPHome yet. |
| Size | 164.9 x 100 x 5.7 mm outline, 154.21 x 85.92 mm active |
| Tail | 40-pin 0.5 mm pitch FPC, 0.3 mm thick, contacts on one side |

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
| 22-30 | GND / NC / U-D, L-R scan direction | see the datasheet pin table; U/D and L/R set scan direction (GND/VDD combos). Leave the NC pins alone. |
| 31, 32 | LED- | backlight cathode |
| 33-38 | NC | |
| 39, 40 | LED+ | backlight anode |

### Init sequence (datasheet p.16). This is what goes in the ESPHome `init_sequence`

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
| HSYNC pulse | 70 | 1-140 |
| H back porch | 160 | fixed |
| H front porch | 160 | 16-216 |
| V active | 600 | |
| VSYNC pulse | 10 | 1-20 |
| V back porch | 23 | fixed |
| V front porch | 12 | 1-127 |

### Gotchas from the datasheet

- **THS_ZERO**: the panel's MIPI receiver does not meet the MIPI minimum. If the host's THS_ZERO sits at the low end the panel silently fails to init. If the screen stays black with a good init log, this is the first thing to look at (ESP-IDF DSI PHY timing).
- The Waveshare 7B ESPHome preset for the same panel class uses `lane_bit_rate: 900Mbps`, `pclk 52MHz`, hsync 10/160/160, vsync 1/23/12. Good starting point.

## Waveshare ESP32-P4-NANO

Wiki: https://www.waveshare.com/wiki/ESP32-P4-Nano-StartPage . Schematic: https://files.waveshare.com/wiki/ESP32-P4-NANO/ESP32-P4-NANO-schematic.pdf

| Item | Value |
|---|---|
| SoC | ESP32-P4NRW32, 32 MB PSRAM in package, 16 MB QSPI flash |
| Radio | ESP32-C6-MINI-1 over SDIO: reset GPIO54, cmd 19, clk 18, d0-d3 = 14-17, active high (ESPHome device page) |
| I2C | SDA GPIO7, SCL GPIO8 (also on the DSI connector) |
| LCD reset (BSP default) | GPIO27 on the GPIO header |
| LCD backlight PWM (BSP default) | GPIO26 on the GPIO header |
| DSI | 22-pin 0.5 mm FPC, 2 lanes + clock. The schematic also routes GPIO37, GPIO38, SDA/SCL and 3.3 V to it. We only use the six MIPI signals and ground from this connector; reset and backlight go from the header instead. |
| Power | USB-C, or 5 V on the header, or PoE module |
| Storage | microSD, SDIO 3.0 |
| ESPHome presets that exist for this board | `WAVESHARE-P4-NANO-10.1` (their 10.1" DSI panel); copy only the board-level bits |

## Backlight driver: eletechsup LD24AJTA

- Buck constant-current, 6-24 V in, 30-900 mA set by a pot. Ceiling is 0.1 V / RCS. Ordered 2026-09-28 from AliExpress ($1.15).
- Set to **270 mA** on a meter before connecting the panel (wiring.md step 1). Bare pads: VIN, GND, LED+, LED-, PWM, GND.
- Buck topology: Vin must be above the LED string (9.6 V) plus headroom. 12 V is fine. 5 V is not. Headroom not in the listing; check brightness on the bench at 12 V.
- PWM pad: 100 Hz-20 kHz per the listing. The LEDC output on GPIO26 runs about 5 kHz.
- Replaces the PT4115 modules: those arrived with a 1R0 sense resistor (100 mA, about 37 % brightness). Getting 270 mA from them needs a 0.36-0.39 ohm 1206 resistor swap (DigiKey RL1206FR-070R36L).

## Pololu D36V28F5

5.3-50 V in, 5 V 3.2 A out, reverse-polarity protected, has an EN pin (tie to ignition-switched 12 V if we want the dash off with the key without a relay).

## On-bike carrier PCB (later)

Replace both breakouts and the jumpers with one board: 22-pin FPC in, 40-pin FPC out, DSI pairs as 100 ohm differential traces, PT4115 + Pololu footprints (or discrete equivalents), fuse, ignition-sense input. Design it only after the bench build shows the panel initialising.

## Antennas

None to buy for the bench. All radios have on-board antennas; the rules for the bike install:

| Radio | Rule |
|---|---|
| BLE, C6 on the P4-NANO (PCB antenna) | Plastic enclosure. Keep the NANO away from the panel's driver strip and the MIPI wires; the e-paper gauge showed display electronics desensing the radio. |
| GPS, SparkFun chip antenna | Must see sky: top of the enclosure, plastic lid, nothing metal above it. If first fix is slow or speed drops out, swap to the SparkFun NEO-M9N u.FL version plus a small active patch antenna (about $12) mounted outside. |
| XIAO C6 (contingency) | Same as BLE; it has a u.FL socket if an external antenna is ever needed. |
| Metal enclosure | Do not. Everything would need external antennas. |
