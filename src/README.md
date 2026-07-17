# Firmware source (Redragon K617)

Custom SMK-based sources for the Redragon K617 Fizz (`258A:0049`).

This folder is **not a full build tree** by itself — it is the keyboard-specific
and related code that ships with this productivity kit. Building a flashable
HEX still needs the full [SMK](https://github.com/carlossless/smk) toolchain
(SDCC + Meson) with this keyboard integrated.

## Layout

| Path | What |
|------|------|
| `keyboards/redragon-k617/` | Keymap, RGB effects, WebHID, matrix init |
| `platform/sh68f90a/usb.c` | USB strings (`Redragon` / `Redragon K617`) |
| `smk/` | Shared headers / settings used by the keyboard |

## Notable files

- `layouts/default/layout.c` — TR-Q keymap, Fn layer, Snap Tap / Win Lock
- `layouts/default/indicators.c` — RGB effects, idle, row-4 R/B fix, LED links
- `webhid.c` — vendor HID commands for the control panel
- `kb.c` — custom keycodes / Snap Tap SOCD

## License

Same as this repository (MIT) for these files. Upstream SMK may carry
additional terms — check the SMK project if you vendor the full firmware.
