# You import all required libraries 
import board
import json
import busio
import digitalio
import time

# These are imports from the kmk library
from kmk.kmk_keyboard import KMKKeyboard
from kmk.scanners.keypad import KeysScanner
from kmk.keys import KC
from kmk.modules.macros import Press, Release, Tap, Macros

# This is the main instance of your keyboard
keyboard = KMKKeyboard()

# Add the macro extension
macros = Macros()
keyboard.modules.append(macros)


PINS = [board.GP26, board.GP27, board.GP28]
i2c = busio.I2C(board.SCL, board.SDA)

# Tell kmk we are not using a key matrix
keyboard.matrix = KeysScanner(
    pins=PINS,
    value_when_pressed=False,
)

int_pin = digitalio.DigitalInOut(board.GP29)
int_pin.direction = digitalio.Direction.INPUT
int_pin.pull = digitalio.Pull.UP  # or Pull.DOWN depending on your circuit


def parse_key(key):
    if key.startswith("MACRO:"):
        macro_name = key[6:]
        if macro_name == "COPY":
            return KC.MACRO(Press(KC.LCTL), Tap(KC.C), Release(KC.LCTL))
        elif macro_name == "PASTE":
            return KC.MACRO(Press(KC.LCTL), Tap(KC.V), Release(KC.LCTL))
        else:
            return KC.MACRO(macro_name)
    elif key.startswith("MEDIA:"):
        media_name = key[6:]
        if media_name == "VOLUME_UP":
            return KC.VOLU
        elif media_name == "VOLUME_DOWN":
            return KC.VOLD
        elif media_name == "MUTE":
            return KC.MUTE
        else:
            return KC.NO
    elif hasattr(KC, key):
        return getattr(KC, key)
    else:
        return KC.NO

with open("/data/macros.json") as keymap_file:
    raw_keymap = json.load(keymap_file)
    
# Here you define the buttons corresponding to the pins
# Look here for keycodes: https://github.com/KMKfw/kmk_firmware/blob/main/docs/en/keycodes.md
# And here for macros: https://github.com/KMKfw/kmk_firmware/blob/main/docs/en/macros.md
keyboard.keymap = [
    [parse_key(key) for key in raw_keymap]
]

# Start kmk!
if __name__ == '__main__':
    keyboard.go()