# import all required libraries 
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
from kmk.scheduler import create_task
from kmk.extensions.media_keys import MediaKeys
#Display
import displayio
import terminalio
from adafruit_display_text import label
from i2cdisplaybus import I2CDisplayBus
import adafruit_displayio_ssd1306

displayio.release_displays()

# Main instance of your keyboard
keyboard = KMKKeyboard()
#Extensions
keyboard.extensions.append(MediaKeys())
macros = Macros()
keyboard.modules.append(macros)

PINS = [board.D2, board.D1, board.D0]
i2c = busio.I2C(board.SCL, board.SDA)

display_bus = displayio.I2CDisplay(i2c, device_address=0x3C)
display = adafruit_displayio_ssd1306.SSD1306(display_bus, width=128, height=32)

new_module = False
MODULE_ADDRESSES = {
    "Knobs": 0x20,
    "Macro": 0x21, 
    "NumPad": 0x22,
}
ACTIVE_MODULES = {
    "Knobs": False,
    "Macro": False,
    "NumPad": False,
}
ACTION_CODES = {
        "A": 1, "B": 2, "C": 3, "D": 4, "E": 5, "F": 6,
        "G": 7, "H": 8, "I": 9, "J": 10, "K": 11, "L": 12, 
        "M": 13, "N": 14, "O": 15, "P": 16, "Q": 17, "R": 18,
        "S": 19, "T": 20, "U": 21, "V": 22, "W": 23, "X": 24,
        "Y": 25, "Z": 26,
        "MACRO:COPY": 27, "MACRO:CUT": 28, "MACRO:PASTE": 29,
        "MEDIA:VOLUME_UP": 30, "MEDIA:VOLUME_DOWN": 31, "MEDIA:MUTE": 32, "MEDIA:BRIGHTNESS_UP": 33, "MEDIA:BRIGHTNESS_DOWN":34, "MEDIA_STOP":35,
        "MACRO:UNDO": 40, "INSERT": 41, "END": 42, "MACRO:REDO": 43,
        "ENTER":50, "MACRO:SELECT_ALL":51
        # Add more mappings in the future
        # keycodes: https://github.com/KMKfw/kmk_firmware/blob/main/docs/en/keycodes.md
        # macros: https://github.com/KMKfw/kmk_firmware/blob/main/docs/en/macros.md
}

# not using matrix
keyboard.matrix = KeysScanner(
    pins=PINS,
    value_when_pressed=False,
)
# Interrupt pin for I2C modules
int_pin = digitalio.DigitalInOut(board.D3)
int_pin.direction = digitalio.Direction.INPUT
int_pin.pull = digitalio.Pull.UP

# i2c
previous_devices = []

def check_i2c_modules():
    global previous_devices
    global new_module
    if i2c.try_lock():
        try:
            devices = i2c.scan()
            # Only handle if devices changed
            if devices != previous_devices:
                print("I2C devices changed:", [hex(addr) for addr in devices])
                for addr in devices:
                    if addr in MODULE_ADDRESSES.values():
                        module_name = None
                        for name, address in MODULE_ADDRESSES.items():
                            if address == addr:
                                module_name = name
                                break
                        i2c.unlock()
                        if module_name:
                            print(f"Detected {module_name} at address {hex(addr)}")
                            ACTIVE_MODULES[module_name] = True
                            write_text(f"Detected {module_name}", yPos=5)
                            send_keymap_to_module(module_name, addr)
                    new_module = True
                    # Handle new/removed modules here
                    previous_devices = devices.copy()
        finally:
            print("Unlocking I2C bus")
            i2c.unlock()

def check_all_slaves():
    # This function checks all I2C slaves for key events
    if not i2c.try_lock():
        print("I2C bus is busy, cannot check slaves")
        return
    
    try:
        for module_name, address in MODULE_ADDRESSES.items():
            if not ACTIVE_MODULES[module_name]:
                print(f"Skipping inactive module: {module_name}")
                continue
            try:
                print(f"Checking {module_name} at address {hex(address)}")
                # Try to read from each slave (adjust buffer size as needed)
                data = bytearray(2)  # Expecting 2 bytes: [key_code, press/release]
                i2c.readfrom_into(address, data)
                print(f"Received data from {module_name}: {data}")
                # Check if slave has valid data
                if data[0] != 0:  # Non-zero means key event
                    print(f"Key event from {module_name}: {data[0]}, pressed: {data[1]}")
                    # Handle the key event here
                    handle_slave_key_event(data[0], data[1] != 0)  # data[1] is 1 for pressed, 0 for released
                else:
                    print(f"Key event from {module_name} was 0")
                    
            except OSError:
                # Slave not responding or not connected
                print(f"No response from {module_name} at address {hex(address)}")
                pass
    finally:
        print("Unlocking I2C bus")
        i2c.unlock()

def custom_process_key():
    global new_module
    # Only check I2C when interrupt pin is active
    if not int_pin.value:  # Active low interrupt
        print("Interrupt detected, checking I2C modules")
        check_i2c_modules()  # Check for new modules
        if not new_module:
            check_all_slaves()   # Check for key events from existing modules
        else:
            print("New module")
        new_module = False  # Reset new module flag after processing
        print("Done checking I2C modules")
        


def handle_slave_key_event(action_code, is_pressed):
    # Find the key string that corresponds to this action code
    key_string = None
    for key, code in ACTION_CODES.items():
        if code == action_code:
            key_string = key
            break
    
    if key_string:
        keyboard.tap_key(parse_key(key_string))   # Press
    else:
        print(f"Unknown action code: {action_code}")


def send_keymap_to_module(module_name, address):
    print(f"Sending keymap to {module_name} at address {hex(address)}")
    if not i2c.try_lock():
        print("I2C bus is busy, cannot send keymap")
        return False
    
    try:
        # Get the keymap for this specific module from JSON
        if module_name.lower() in raw_keymap:
            module_keymap = raw_keymap[module_name.lower()]
            
            # Convert keymap to action codes that slaves can understand
            action_codes = []
            for key in module_keymap:
                action_code = key_to_action_code(key)
                action_codes.append(action_code)
            
            # Send keymap to slave (may need to send in chunks if too large)
            print(f"Sending keymap: {action_codes}")
            keymap_data = bytes(action_codes)
            i2c.writeto(address, keymap_data)
            print(f"Sent keymap to {module_name}: {action_codes}")
            return True
        else:
            print(f"No keymap found for {module_name}")
            return False
            
    except Exception as e:
        print(f"Failed to send keymap to {module_name}: {e}")
        return False
    finally:
        print("Unlocking I2C bus")
        i2c.unlock()

def key_to_action_code(key_string):
    return ACTION_CODES.get(key_string, 0)  # 0 = no action

def parse_key(key):
    if key.startswith("MACRO:"):
        macro_name = key[6:]
        if macro_name == "COPY":
            return KC.MACRO(Press(KC.LCTL), Tap(KC.C), Release(KC.LCTL))
        elif macro_name == "CUT":
            return KC.MACRO(Press(KC.LCTL), Tap(KC.X), Release(KC.LCTL))
        elif macro_name == "PASTE":
            return KC.MACRO(Press(KC.LCTL), Tap(KC.V), Release(KC.LCTL))
        elif macro_name == "UNDO":
            return KC.MACRO(Press(KC.LCTL), Tap(KC.Z), Release(KC.LCTL))
        elif macro_name == "REDO":
            return KC.MACRO(Press(KC.LCTL), Tap(KC.Y), Release(KC.LCTL))
        elif macro_name == "SELECT_ALL":
            return KC.MACRO(Press(KC.LCTL), Tap(KC.A), Release(KC.LCTL))
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
        elif media_name == "BRIGHTNESS_UP":
            return KC.BRIU
        elif media_name == "BRIGHTNESS_DOWN":
            return KC.BRID
        elif media_name == "STOP":
            return KC.MEDIA_STOP
        else:
            return KC.NO
    elif hasattr(KC, key):
        return getattr(KC, key)
    else:
        return KC.NO
def update_text(message, text_area):
    text_area.text = message
def write_text(message, xPos= 0, yPos=0, size=1, text_color=0xFFFFFF):
    if len(splash) > 0:
        splash.pop()
    # Draw a text
    text_area = label.Label(terminalio.FONT, text=message, color=text_color, x=xPos, y=yPos, scale=size)
    splash.append(text_area)
def add_text(message, xPos= 0, yPos=0, size=1, text_color=0xFFFFFF):
    # Draw a text
    text_area = label.Label(terminalio.FONT, text=message, color=text_color, x=xPos, y=yPos, scale=size)
    splash.append(text_area)
def display_home_page():
    while len(splash) > 0:
        splash.pop()
    # Draw a text
    text = "BeanPad"
    text_area = label.Label(terminalio.FONT, text=text, color=0xFFFFFF, x=0, y=32 // 2 - 1, scale=2)
    splash.append(text_area)
with open("/data/macros.json") as keymap_file:
    raw_keymap = json.load(keymap_file)
    
keyboard.keymap = [
    [parse_key(key) for key in raw_keymap["base"]]
]

# Add custom logic to kmk's event loop
create_task(custom_process_key, period_ms=250)

#display something on the OLED
splash = displayio.Group()
display.root_group = splash

# Draw a label
display_home_page()


# Start kmk
if __name__ == '__main__':
    keyboard.go() 