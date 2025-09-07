#include <Arduino.h>
#include <Wire.h>
#include <ezButton.h>
//buttons
#define BUTTON_PIN_1 D7 // pin for the 1st button
#define BUTTON_PIN_2 D8 // pin for the 2nd button
#define BUTTON_PIN_3 D9 // pin for the 3rd button
#define BUTTON_PIN_4 D10 // pin for the 4th button

// i2c module
#define I2C_ADDRESS 0x21 // I2C address for the module
#define INT_PIN D3 // Interrupt pin
//keymap
#define MAX_KEYS 4
uint8_t keymap[MAX_KEYS];
uint8_t keymap_size = 0;
bool keymap_received = false;

ezButton Button_1(BUTTON_PIN_1);
ezButton Button_2(BUTTON_PIN_2);
ezButton Button_3(BUTTON_PIN_3);
ezButton Button_4(BUTTON_PIN_4);

int Active_key = 0;
bool action_available = false;
// put function declarations here:
void receiveKeymap(int bytes);
void sendAction(int key);
void requestHandler();
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
  Wire.onReceive(receiveKeymap);
  Wire.onRequest(requestHandler);

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
void receiveKeymap(int bytes) {
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
  digitalWrite(INT_PIN, HIGH);
  // Clear any remaining bytes
  while (Wire.available()) {
    Wire.read();
  }
  
  keymap_received = true;
  Serial.print("Keymap received with ");
  Serial.print(keymap_size);
  Serial.println(" keys");
}
void requestHandler() {
  if (action_available) {
    Wire.write(Active_key); // Send the action
    action_available = false; // Clear flag after sending
    digitalWrite(INT_PIN, HIGH); // Reset INT pin
  } else {
    Wire.write(0); // No action
  }
}
void sendAction(int key) {
  // Send the action corresponding to the key
  Serial.print("Sending action for key: ");
  Serial.println(keymap[key]);
  Active_key = key;
  action_available = true; // Set flag to indicate new action
  digitalWrite(INT_PIN, LOW); // Signal to master
}