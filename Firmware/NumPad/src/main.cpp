#include <Arduino.h>
#include <Wire.h>
#include <ezButton.h>
//buttons
#define BUTTON_PIN_1 1 // Button pin for the 1st rotary encoder
#define BUTTON_PIN_2 2 // Button pin for the 2nd rotary encoder
#define BUTTON_PIN_3 4 // Button pin for the 3rd rotary encoder
#define BUTTON_PIN_4 3 // Button pin for the 4th rotary encoder

// i2c module
#define I2C_ADDRESS 0x21 // I2C address for the module
#define INT_PIN 29 // Interrupt pin for the 1st rotary encoder
//keymap
#define MAX_KEYS 4
uint8_t keymap[MAX_KEYS];
uint8_t keymap_size = 0;
bool keymap_received = false;

ezButton Button_1(BUTTON_PIN_1);
ezButton Button_2(BUTTON_PIN_2);
ezButton Button_3(BUTTON_PIN_3);
ezButton Button_4(BUTTON_PIN_4);
// put function declarations here:
void requestKeymap();
void sendAction(int key);
void setup() {
  Wire.begin(I2C_ADDRESS);
  Serial.begin(9600);
  // Interrupt pin
  pinMode(INT_PIN, OUTPUT);
  //Buttons
  Button_1.setDebounceTime(10);
  Button_2.setDebounceTime(10);
  Button_3.setDebounceTime(10);
  Button_4.setDebounceTime(10);

  // Request keymap
  digitalWrite(INT_PIN, LOW); // Pull the INT pin down indicating a request or action
  Wire.onRequest(requestKeymap);

}

void loop() {
  Button_1.loop();
  Button_2.loop();
  Button_3.loop();
  Button_4.loop();
  

  if (Button_1.isPressed()) {
    sendAction(keymap[0]); // Send action for the 1st key
  }
  if (Button_2.isPressed()) {
    sendAction(keymap[1]); // Send action for the 2nd key
  }
  if (Button_3.isPressed()) {
    sendAction(keymap[2]); // Send action for the 3rd key
  }
  if (Button_4.isPressed()) {
    sendAction(keymap[3]); // Send action for the 4th key
  }
}
// put function definitions here:
void requestKeymap(int bytes) {
  Serial.print("Received ");
  Serial.print(bytes);
  Serial.println(" bytes");
  
  keymap_size = 0;
  
  // Read all incoming bytes as keymap
  while (Wire.available() && keymap_size < MAX_KEYS) {
    keymap[keymap_size] = Wire.read();
    Serial.print("Keymap[");
    Serial.print(keymap_size);
    Serial.print("] = ");
    Serial.println(keymap[keymap_size]);
    keymap_size++;
  }
  
  // Clear any remaining bytes
  while (Wire.available()) {
    Wire.read();
  }
  
  keymap_received = true;
  Serial.print("Keymap received with ");
  Serial.print(keymap_size);
  Serial.println(" keys");
}
void sendAction(int key) {
  if (key < 0 || key >= keymap_size) {
    Serial.println("Invalid key index");
    return;
  }
  
  // Send the action corresponding to the key
  Serial.print("Sending action for key: ");
  Serial.println(keymap[key]);
  
  // Here you can implement the actual sending logic, e.g., via I2C or Serial
  digitalWrite(INT_PIN, LOW); // Pull the INT pin down to indicate an action
  Wire.beginTransmission(I2C_ADDRESS);
  Wire.write(keymap[key]);
  Wire.endTransmission();
}