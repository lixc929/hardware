# Firmware Index

This directory contains firmware modules and reference snapshots used during
the keyboard bring-up.

## Active Bring-up Code

- `ch585_spi_slave_test/`
  - Current CH585 test firmware for the H417 <-> CH585 SPI debug chain.
  - Generates simulated key data, sends `KEY_STATE` and low-rate `KEY_DEBUG`
    frames to H417.
- `ch585_frontend/`
  - ADS7948 + MUX scan helper code prepared for the real CH585 front-end.
  - Not yet wired into the current CH585 SPI test firmware.
- `common/`
  - Shared candidate modules, including the earlier magnetic key engine.

## Legacy Reference Snapshots

- `ch585_legacy/`
  - Archived CH585 BLE / 2.4G RF / USB experiments.
  - These projects are kept as reference material and are not part of the
    current H417 RT-Thread build.
  - See `ch585_legacy/README.md` before importing them into MounRiver.
