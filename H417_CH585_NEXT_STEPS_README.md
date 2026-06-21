# H417 + CH585 Next Steps

Updated: 2026-06-21

This note records the current debug direction after the first SPI/USB bring-up.

## Current Situation

- One CH585 board is connected to H417 through hardware SPI2 plus GPIO CS.
- CH585 sends compact 16-byte `KEY_STATE` frames carrying `down_bits[8]`.
- CH585 also sends low-rate 16-byte `KEY_DEBUG` frames for local key algorithm visibility.
- H417 accepts both frame types and prints `KS` / `SS` / `TR` / `KD` debug lines through USBFS CDC.
- USBFS CDC is currently the preferred debug port. On the current PC:
  - `COM5` is H417 USBFS CDC.
  - `COM4` is WCH-Link serial.
- ADS7948 / ADC hardware is not available yet.
- With DuPont wires, SPI speed is treated as a stable bring-up setting first; 40 MHz+ tuning should wait for PCB hardware and waveform checks.

## Verified On Board

Current verified chain:

```text
CH585 simulated ADC / local key algorithm
  -> CH585 KEY_STATE + low-rate KEY_DEBUG
  -> H417 SPI2 pull
  -> H417 USBFS COM5 logs
```

Observed COM5 output:

```text
KD n=2 seq=112 k=2 raw=999 filt=1055 pos=27 peak=490 down=0 rt=1
SS hb=9 s0ok=9 s0fetch=0 s0crc=0 s0seq=0 sck=16000.0 ...
KS f=0 i=000 1000 1000 3000 3000 ...
```

Important build setting:

```text
APP_ENABLE_USB2_HS_CDC=0
APP_ENABLE_USB2_FS_CDC=1
```

This keeps the debug console on USBFS CDC. If USBFS does not enumerate, check the Makefile first.

## No-ADC Stage Plan

Before ADS7948 hardware arrives, keep the simulated front-end useful and observable.

Acceptance criteria:

```text
s0ok keeps increasing
s0fetch = 0
s0crc = 0
s0seq does not keep increasing
KD appears periodically
KS shows the simulated keys toggling between 1000 and 3000
```

## Current Wire Frames

Current `KEY_STATE` frame:

```text
offset  size  field
0       1     magic       0xD7
1       1     type        0x11 = KEY_STATE
2       1     source_id   0
3       1     seq
4       1     ack_seq
5       1     flags       bit7=READY
6       8     down_bits   64 key 0/1 state bitmap
14      2     crc16       CRC-CCITT over bytes 0..13
```

Current `KEY_DEBUG` frame:

```text
offset  size  field
0       1     magic       0xD7
1       1     type        0x12 = KEY_DEBUG
2       1     source_id   0
3       1     seq
4       1     key_id
5       1     flags       bit7=READY, bit0=down, bit1=rt_armed
6       2     raw_adc
8       2     filtered_adc
10      2     position_pm 0..1000
12      2     peak_pm
14      2     crc16       CRC-CCITT over bytes 0..13
```

Current request-only read:

```text
H417 pulls CS low
H417 clocks 16 bytes
CH585 returns one 16-byte KEY_STATE or low-rate KEY_DEBUG frame
H417 raises CS
```

Current bus payload:

```text
one CH585: 16B = 128 bit per state read
two CH585 boards at 8 kHz: about 2.048 Mbit/s payload
```

## Next Work

1. Keep observing the current single-CH585 chain for stability.

2. Move CH585 key algorithm parameters into a clear config structure:

```text
released_adc
pressed_adc
press_position
release_position
rt_press_delta
rt_release_delta
filter_shift
rt_enable
```

3. Add H417 -> CH585 config/debug command prototypes:

```text
GET_STATE
GET_DEBUG
GET_CONFIG
SET_CONFIG
```

4. Move the CH585 test firmware into the `hardware` repository so teammates can reproduce the CH585 side.

Suggested location:

```text
firmware/ch585_spi_slave_test/
```

5. When ADC hardware arrives, replace simulated ADC in small steps:

```text
ADS7948 single channel -> key0
ADS7948 dual channel
single 16:1 MUX
4 lanes / 64 keys per CH585
second CH585
```

## SPI Speed Work

Do not jump straight to 70 MHz on DuPont wires. Step through frequency candidates and judge by counters, not by visual impression.

Suggested steps:

```text
16 MHz current working point
18 MHz
20 MHz
30 MHz
40 MHz+
```

Pass criteria:

```text
crc does not increase
magic does not increase
ack_err does not increase
seq_drop does not keep increasing
ok keeps increasing
```

If high frequency fails, check these before changing protocol again:

```text
SPI CPHA / sample edge
H417 HSRX mode
CH585 MISO output drive
wire length
extra GND return
logic analyzer or oscilloscope view of SCK/MISO setup time
```

## Pipeline Transaction Option

Current request-only mode is simple and stable:

```text
transaction N:
  H417 clocks 16 bytes
  CH585 returns already prepared frame N
```

Future pipeline mode can reduce host-side turnaround time:

```text
transaction N:
  H417 sends request N
  H417 reads response N-1

transaction N+1:
  H417 sends request N+1
  H417 reads response N
```

Only revisit this after the current request-only frame stays stable at the required SPI clock.

## Second CH585

Add the second CH585 only after the first CH585 remains stable.

Planned topology:

```text
SCK  shared
MOSI shared
CS0  H417 -> CH585_0
CS1  H417 -> CH585_1
MISO either shared if CH585 tri-states correctly, or separate MISO lines
GND  shared
```

If MISO is shared, verify that the unselected CH585 releases MISO when CS is inactive. If that cannot be guaranteed, use two independent MISO pins or external tri-state buffering.

## Priority Summary

1. Preserve the verified USBFS + KD debug point.
2. Add CH585 config structures and command hooks.
3. Move CH585 test firmware into the repo.
4. Integrate ADS7948/MUX one step at a time when hardware arrives.
5. Re-test SPI speed on PCB hardware before targeting 40 MHz+.
