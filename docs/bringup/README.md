# H417 + CH585 Bring-up Index

This folder contains the active H417 / CH585 keyboard front-end bring-up notes.

## Read First

1. [Current debug status](h417_ch585_current_debug.md)
2. [Next steps](h417_ch585_next_steps.md)
3. [CH32H417 dual-core flashing guide](h417_mounriver_dual_core_flash.md)

## Current Verified Point

The current verified debug chain is:

```text
CH585 simulated ADC / local key algorithm
  -> CH585 KEY_STATE + low-rate KEY_DEBUG
  -> H417 hardware SPI2 + GPIO CS
  -> H417 USBFS CDC COM5 logs
```

Expected COM5 lines include:

```text
KD n=2 seq=112 k=2 raw=999 filt=1055 pos=27 peak=490 down=0 rt=1
SS hb=9 s0ok=9 s0fetch=0 s0crc=0 s0seq=0 sck=16000.0 ...
KS f=0 i=000 1000 1000 3000 3000 ...
```

## Current Build Setting

For USBFS debug output, the H417 RT-Thread build currently uses:

```text
APP_ENABLE_USB2_HS_CDC=0
APP_ENABLE_USB2_FS_CDC=1
```

If COM5 does not appear, check `rtthread_port/Makefile` first.

## Reference Notes

- [ADS7948 / MUX / magnetic notes](ads7948_mux_magnetic_notes.md)
- [CH585 SPI slave test firmware](../../firmware/ch585_spi_slave_test/README.md)
- [CH585 legacy BLE / RF snapshots](../../firmware/ch585_legacy/README.md)
- [CH585 BLE / 2.4G RF archive notes](archive/ch585_ble_rf/README.md)
- [Archive of older SPI / USB experiments](archive/)

## Archive Policy

Files in `archive/` are old stage notes. They are preserved for traceability, but may describe wiring, protocol variants, or USBHS experiments that are not the current default path.
