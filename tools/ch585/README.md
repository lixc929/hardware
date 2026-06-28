# CH585 Debug Tools

## `usb_cdc_console.py`

Interactive CH585 USB CDC console copied from the earlier `CH585M` bring-up
workspace.

Usage:

```powershell
python tools/ch585/usb_cdc_console.py COM5
```

Install `pyserial` first if Python cannot import `serial`:

```powershell
python -m pip install pyserial
```

Useful local commands:

```text
localhelp
after 3 tap b
afterfast 3 tap b; tap 1; tap enter
```

This tool is for CH585 CDC command experiments. For the current H417 USBFS CDC
SPI scan logs, use:

```powershell
python rtthread_port/tools/read_usbfs_scan.py --port COM5
```

## `adc_text_scope.py`

Text-mode ADC dashboard for CH585 ADS7948 UART probe logs. It reads `AP ...`
lines from the CH585 UART probe and draws all 64 local ADC channels as
1024-normalized progress bars.

Use it with the all-channel probe firmware, for example:

```text
firmware/ch585_spi_slave_test/build_right_all64_raw_probe/ch585m_adc_right_all64_raw_probe.hex
```

Run:

```powershell
python tools/ch585/adc_text_scope.py COM4 --side right
```

If Python does not have `pyserial`, use the dependency-free PowerShell version:

```powershell
powershell -ExecutionPolicy Bypass -File tools/ch585/adc_text_scope.ps1 -Port COM4 -Side right
```

The PowerShell dashboard defaults to the mapped-key view. Each row shows the
local key number, lane/MUX channel, mapped label, raw ADC value, `raw/1023`
percentage, a 1024-normalized progress bar, baseline drop, min/max, and `down`.
This is the preferred view for checking whether a real key is mapped correctly:

```text
K53 L4D06 B          raw=0501 norm= 49% [##########..........] drop=  12 down=0
```

Numbering note:

- `Hxx` is the global Hall/key number from the hardware report.
- `Kxx` is the firmware-local physical MUX slot: `(MUX index - 1) * 16 + (D - 1)`.
- `LxDxx` is the physical MUX lane and data input. `D` is always `D01..D16`.
- The left half uses only `D01..D09` on each MUX. For example, left MUX1
  maps `F5 F4 F3 F2 F1 Esc 6 5 4` to `L1D01..L1D09`, which are global
  `H42..H50` but local `K00..K08`. Left `Q` is `H60 K32 L3D01` because
  MUX3 starts at local slot `K32`.
- The right half uses `D01..D10` on MUX1..MUX3 and `D01..D11` on MUX4.

Use the all-channel view only when searching for an unknown or missing route:

```powershell
powershell -ExecutionPolicy Bypass -File tools/ch585/adc_text_scope.ps1 -Port COM4 -Side right -View all
```

For a timed capture that exits by itself:

```powershell
python tools/ch585/adc_text_scope.py COM4 --side right --duration 10
powershell -ExecutionPolicy Bypass -File tools/ch585/adc_text_scope.ps1 -Port COM4 -Side right -Duration 10
```

The dashboard shows `drop = baseline - raw`; pressing a Hall key usually makes
the raw ADC value smaller, so the correct channel should jump to the top of the
`Top drops` list.
