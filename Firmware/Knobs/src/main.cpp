#include <Arduino.h>
#include <Wire.h>
#include <ezButton.h>
//1st rotary encoder
#define CLK_PIN_1 D7
#define DT_PIN_1 D8
#define SW_PIN_1 D9
//2nd rotary encoder
#define CLK_PIN_2 D10
#define DT_PIN_2 D1
#define SW_PIN_2 D2
//Directions
#define DIRECTION_CW 0   // clockwise direction
#define DIRECTION_CCW 1  // counter-clockwise direction
// i2c module
#define I2C_ADDRESS 0x20 // I2C address for the module
#define INT_PIN D3 // Interrupt pin for the 1st rotary encoder
//keymap
#define MAX_KEYS 6
uint8_t keymap[MAX_KEYS];
uint8_t keymap_size = 0;
bool keymap_received = false;

int direction = DIRECTION_CW;
int direction_2 = DIRECTION_CW;
int CLK_1_state;
int CLK_2_state;
int prev_CLK_1_state;
int prev_CLK_2_state;

ezButton Rot_Encoder_1(SW_PIN_1);
ezButton Rot_Encoder_2(SW_PIN_2);
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
  // configure encoder pins as inputs
  pinMode(CLK_PIN_1, INPUT_PULLUP);
  pinMode(DT_PIN_1, INPUT_PULLUP);
  pinMode(CLK_PIN_2, INPUT_PULLUP);
  pinMode(DT_PIN_2, INPUT_PULLUP);
  Rot_Encoder_1.setDebounceTime(50);
  Rot_Encoder_2.setDebounceTime(50);

  // read the initial state of the rotary encoder's CLK pin
  prev_CLK_1_state = digitalRead(CLK_PIN_1);
  prev_CLK_2_state = digitalRead(CLK_PIN_2);
  // Request keymap
  digitalWrite(INT_PIN, LOW); // Pull the INT pin down indicating a request or action
  Wire.onReceive(receiveKeymap);
  Wire.onRequest(requestHandler);

}

void loop() {
  Rot_Encoder_1.loop();
  Rot_Encoder_2.loop();

  // read the current state of the rotary encoder's CLK pin
  CLK_1_state = digitalRead(CLK_PIN_1);
  CLK_2_state = digitalRead(CLK_PIN_2);
  
  // If CLK was changed, then pulse occurred
  // React to only the rising edge to stop double counting
  //1st rotary encoder
  if (CLK_1_state != prev_CLK_1_state && CLK_1_state == HIGH) {
    if (digitalRead(DT_PIN_1) == HIGH) {
      direction = DIRECTION_CCW;
      sendAction(keymap[1]); // Send action for the 2nd key
    } else {
      direction = DIRECTION_CW;
      sendAction(keymap[0]); // Send action for the 1st key
    }
    //TODO: Rotating broken
    Serial.print("DIRECTION: ");
    if (direction == DIRECTION_CW)
      Serial.print("Clockwise");
    else
      Serial.print("Counter-clockwise");
  }
  prev_CLK_1_state = CLK_1_state;
  //2nd rotary encoder
  if (CLK_2_state != prev_CLK_2_state && CLK_2_state == HIGH) {
    if (digitalRead(DT_PIN_2) == HIGH) {
      sendAction(keymap[4]); // Send action for the 5th key
      direction_2 = DIRECTION_CCW;
    } else {
      direction_2 = DIRECTION_CW;
      sendAction(keymap[3]); // Send action for the 4th key
    }

    Serial.print("DIRECTION: ");
    if (direction_2 == DIRECTION_CW)
      Serial.print("Clockwise");
    else
      Serial.print("Counter-clockwise");
  }
  // save last CLK state
  prev_CLK_2_state = CLK_2_state;
  

  if (Rot_Encoder_1.isPressed()) {
    sendAction(keymap[2]); // Send action for the 3rd key
  }
  if (Rot_Encoder_2.isPressed()) {
    sendAction(keymap[5]); // Send action for the 6th key
  }
}
// put function definitions here:
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