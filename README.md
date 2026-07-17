# Redragon K617 — Productivity Kit

```powershell
irm https://raw.githubusercontent.com/thatsdai00/redragon-k617-productivity/main/run.ps1 | iex
```

Open-source, **no-install** Windows toolkit for the Redragon K617 Fizz (VID `258A` / PID `0049`).

Flash custom firmware, back up your original image, and open the RGB control panel — without installing drivers or Python.

Flashing uses **[sinowisp](https://github.com/carlossless/sinowisp)** by [carlossless](https://github.com/carlossless) (formerly `sinowealth-kb-tool`). A portable `tools/sinowisp.exe` is bundled here for convenience — credit and upstream docs belong to that project.

## Quick start

**Option A — one command (above)** downloads the kit to `%TEMP%` and opens the UI.

**Option B — local folder**

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
run.ps1             one-liner installer/launcher
BENI-OKU.txt        short guide
firmware/           custom HEX + version stamp
tools/sinowisp.exe  portable flasher ([sinowisp](https://github.com/carlossless/sinowisp))
src/                firmware source (K617 + related)
docs/               GitHub Pages landing
```

## Requirements

- Windows 10 / 11  
- Built-in PowerShell (already on the PC)  
- USB connection  

No installer. No extra runtimes.

## Credits

- **[sinowisp](https://github.com/carlossless/sinowisp)** — SinoWealth ISP read/write tool used to flash this keyboard  
- **[SMK](https://github.com/carlossless/smk)** — upstream 8051 keyboard firmware framework this custom build is based on  

## Safety

- **Back up** before flashing custom firmware.  
- Stock restore needs **your** backup file — this kit does not ship a factory dump.  
- Flashing the wrong image can leave the board in ISP-only mode until you recover it.  
- Follow sinowisp’s own warnings: misuse can brick devices.

## Control panel

RGB / idle / Snap Tap: [thatsdai.pages.dev](https://thatsdai.pages.dev)

## License

MIT — see [LICENSE](LICENSE).  
`tools/sinowisp.exe` is a third-party binary; see [sinowisp](https://github.com/carlossless/sinowisp) for its license and source.
