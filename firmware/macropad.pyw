import serial
import time
import datetime
import pyautogui
import os
import screen_brightness_control as sbc
import warnings
from pycaw.pycaw import AudioUtilities, ISimpleAudioVolume

# --- CONFIGURATION ---
COM_PORT = 'COM3'   
BAUD_RATE = 115200
MONITOR_ORDER = [0, 1] 

# --- PATHS ---
APP_PATHS = {
    "ARDUINO": r"C:\ProgramData\Microsoft\Windows\Start Menu\Programs\Arduino IDE.lnk",
    "CURA":    r"C:\ProgramData\Microsoft\Windows\Start Menu\Programs\UltiMaker Cura\UltiMaker Cura 5.8.1.lnk",
    "STEAM":   r"C:\Users\jf9s\AppData\Roaming\Microsoft\Windows\Start Menu\Programs\Steam\Steam.lnk",
    "EDGE":    r"C:\ProgramData\Microsoft\Windows\Start Menu\Programs\Microsoft Edge.lnk"
}

# --- MACRO MAPPING ---
MACRO_MAP = {
    "1": "OPEN_CURA",
    "2": "OPEN_ARDUINO",
    "3": "OPEN_EDGE",
    "4": "OPEN_STEAM",
    "5": "win+d",
    "6": "alt+f4",
    "7": "ctrl+shift+win+b",
    "8": "nexttrack",
    "9": "playpause",
    "10": "prevtrack"
}

warnings.simplefilter("ignore")

# --- SMART APP FINDER ---
def set_app_volume(app_name_fragment, volume_level):
    try:
        sessions = AudioUtilities.GetAllSessions()
        for session in sessions:
            if session.Process and session.Process.name():
                proc_name = session.Process.name().lower()
                if app_name_fragment.lower() in proc_name:
                    v = session._ctl.QueryInterface(ISimpleAudioVolume)
                    v.SetMasterVolume(volume_level / 100.0, None)
    except Exception as e:
        pass 

def main():
    try:
        ser = serial.Serial(COM_PORT, BAUD_RATE, timeout=0.01)
        time.sleep(2) 
    except Exception as e:
        return

    # --- THE 100% W STARTUP SEQUENCE ---
    # 1. Zero out the PC Master Volume
    for _ in range(50): pyautogui.press('volumedown') 
    
    # 2. Tell Arduino: Master=0, Spotify=100, Game=100, Mic=100
    ser.write(b"SYNC_VOL:0,100,100,100\n") 
    
    # 3. Force PC Brightness to 100% and tell Arduino
    try:
        sbc.set_brightness(100)
        ser.write(b"SYNC_BRI:100,100\n")
    except: pass
    
    # 4. Set apps to 100% instantly if they are already running
    set_app_volume("spotify", 100)
    set_app_volume("msedge", 100)

    current_master = 0

    while True:
        try:
            if ser.in_waiting > 0:
                raw = ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
                for line in raw.split('\n'):
                    line = line.strip()
                    if not line: continue

                    # --- THE MIXER LOGIC ---
                    if line.startswith("VOL_SET:") or line.startswith("MIX_SET:"):
                        parts = line.split(":")[1].split(",")
                        idx = int(parts[0])
                        val = int(parts[1])

                        # SLIDER 1: MASTER VOLUME
                        if idx == 0:
                            diff = val - current_master
                            if abs(diff) > 1: 
                                steps = int(abs(diff) / 2)
                                if steps < 1: steps = 1
                                key = 'volumeup' if diff > 0 else 'volumedown'
                                for _ in range(steps): pyautogui.press(key)
                                current_master = val

                        # SLIDER 2: SPOTIFY 
                        elif idx == 1:
                            set_app_volume("spotify", val)

                        # SLIDER 3: ACTIVE GAME / APP 
                        elif idx == 2:
                            try:
                                import win32gui, win32process, psutil
                                pid = win32process.GetWindowThreadProcessId(win32gui.GetForegroundWindow())[-1]
                                active_name = psutil.Process(pid).name()
                                set_app_volume(active_name, val)
                            except: pass

                        # SLIDER 4: BROWSER / MIC 
                        elif idx == 3:
                            set_app_volume("msedge", val) 

                    # --- BRIGHTNESS & MACROS ---
                    elif line.startswith("BRI_SET:"):
                        parts = line.split(":")[1].split(",")
                        idx = int(parts[0])
                        val = int(parts[1])
                        try:
                            if idx == 2: sbc.set_brightness(val)
                            else: sbc.set_brightness(val, display=MONITOR_ORDER[idx])
                        except: pass

                    elif line.startswith("BTN_"):
                        key = line.replace("BTN_", "")
                        if key in MACRO_MAP:
                            action = MACRO_MAP[key]
                            if "OPEN_" in action:
                                try: os.startfile(APP_PATHS[action.replace("OPEN_", "")])
                                except: pass
                            elif "+" in action: pyautogui.hotkey(*action.split("+"))
                            else: pyautogui.press(action)
            
            time.sleep(0.01)

        except KeyboardInterrupt: break
        except Exception as e: time.sleep(0.1)

if __name__ == "__main__":
    main()