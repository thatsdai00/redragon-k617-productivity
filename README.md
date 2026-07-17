# Redragon K617 — Productivity Kit

Open-source, **no-install** Windows toolkit for the Redragon K617 Fizz (VID `258A` / PID `0049`).

Flash custom firmware, back up your original image, and open the RGB control panel — without installing drivers or Python.

## Quick start

1. Plug the keyboard in via USB  
2. Double-click **`Redragon-K617.cmd`**  
3. Use the buttons (TR / EN + dark UI)

| Button | Action |
|--------|--------|
| Control panel | Opens the WebHID RGB UI |
| Backup original | Saves `firmware/benim-stok-yedek.hex` |
| Install custom | Writes `firmware/redragon-k617.hex` |
| Restore stock | Restores **your** backup only |

## What's inside

```
Redragon-K617.cmd   launcher
ui.ps1              WinForms UI
BENI-OKU.txt        short guide
firmware/           custom HEX + version stamp
tools/sinowisp.exe  portable SinoWealth flasher
src/                firmware source (K617 + related)
docs/               GitHub Pages landing
```

## Requirements

- Windows 10 / 11  
- Built-in PowerShell (already on the PC)  
- USB connection  

No installer. No extra runtimes.

## Safety

- **Back up** before flashing custom firmware.  
- Stock restore needs **your** backup file — this kit does not ship a factory dump.  
- Flashing the wrong image can leave the board in ISP-only mode until you recover it.

## Control panel

RGB / idle / Snap Tap: [thatsdai.pages.dev](https://thatsdai.pages.dev)

## License

MIT — see [LICENSE](LICENSE).
