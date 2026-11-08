# MacroTimmy — Custom Macro Pad & PC Audio Mixer

A custom desktop macro pad, 4-channel volume mixer, and dual-monitor brightness controller built for Coolest Projects 2026.

## Hardware & Components
* **Microcontroller:** Arduino Mega 2560
* **Display:** 1.3-inch I²C OLED Display
* **Inputs:** 10-button macro matrix, 1 analog joystick
* **Enclosure:** Custom 3D-printed case (Creality Ender 3 V2 Neo)
* **Backend:** Python 3 (`macrotimmy.py`) communicating over USB Serial (`COM3`)

## Features
* **4-Channel Real-Time Audio Mixing:** Controls Master volume, Spotify, active game audio, and Microsoft Edge via `pycaw`.
* **Dual-Monitor Brightness Control:** Adjusts display brightness across dual monitors (`Lenovo Generic Monitor` & `Dell S2340M`) via `screen_brightness_control`.
* **10-Key Macro Matrix:** One-press hardware shortcuts for launching Arduino IDE, UltiMaker Cura 5.8.1, Steam, and Edge via `pyautogui`.
