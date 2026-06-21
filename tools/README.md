# Tools Index

Small host-side helper scripts used during bring-up.

## CH585 Legacy Tools

- `ch585/usb_cdc_console.py`
  - Interactive CH585 USB CDC command console from the older BLE/KVM bring-up.
  - See `ch585/README.md`.

## Current H417 Bring-up Tools

The current H417 USBFS/USBHS readers live with the RT-Thread port because they
are tied to that firmware image:

- `rtthread_port/tools/read_usbfs_scan.py`
- `rtthread_port/tools/read_usbfs_scan.ps1`
- `rtthread_port/tools/read_usbhs_cdc.ps1`
