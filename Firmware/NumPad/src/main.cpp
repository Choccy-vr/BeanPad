#include <Arduino.h>
#include <Wire.h>

// Button matrix configuration
#define ROWS 4
#define COLS 3
#define MAX_KEYS 12

// Define row and column pins
uint8_t rowPins[ROWS] = {3,4,2,1}; // Row pins
uint8_t colPins[COLS] = {26,27,28};    // Column pins

// i2c module
#define I2C_ADDRESS 0x21 // I2C address for the module
#define INT_PIN 29 // Interrupt pin

//keymap
uint8_t keymap[MAX_KEYS];
uint8_t keymap_size = 0;
bool keymap_received = false;

// Button state tracking for matrix
bool currentState[ROWS][COLS];
bool previousState[ROWS][COLS];
unsigned long lastDebounceTime[ROWS][COLS];
unsigned long debounceDelay = 10; // 10ms debounce

// Current key event to send
uint8_t current_key_pressed = 0;
uint8_t key_state = 0; // 0 = released, 1 = pressed

// Function declarations
void receiveEvent(int bytes);
void requestEvent();
void scanMatrix();
void sendAction(uint8_t keyIndex, bool isPressed);

void setup() {
  Wire.begin(I2C_ADDRESS);
  Serial.begin(9600);
  
  // Initialize matrix pins
  for (int i = 0; i < ROWS; i++) {
    pinMode(rowPins[i], OUTPUT);
    digitalWrite(rowPins[i], HIGH); // Set rows HIGH initially
  }
  
  for (int i = 0; i < COLS; i++) {
    pinMode(colPins[i], INPUT_PULLUP); // Columns as inputs with pullup
  }
  
  // Initialize button states
  for (int r = 0; r < ROWS; r++) {
    for (int c = 0; c < COLS; c++) {
      currentState[r][c] = HIGH; // Not pressed (pullup)
      previousState[r][c] = HIGH;
      lastDebounceTime[r][c] = 0;
    }
  }
  
  // Interrupt pin
  pinMode(INT_PIN, OUTPUT);
  digitalWrite(INT_PIN, LOW); // Start Low
  
  // Register I2C event handlers
  Wire.onReceive(receiveEvent);  // When master sends keymap
  Wire.onRequest(requestEvent);  // When master requests key events
  
  Serial.println("NumPad Matrix Ready");
}

void loop() {
  scanMatrix();
}

void scanMatrix() {
  for (int row = 0; row < ROWS; row++) {
    // Set current row LOW
    digitalWrite(rowPins[row], LOW);
    
    
    // Read all columns for this row
    for (int col = 0; col < COLS; col++) {
      bool reading = digitalRead(colPins[col]);
      
      // Check if state changed
      if (reading != previousState[row][col]) {
        lastDebounceTime[row][col] = millis();
      }
      
      // If enough time has passed, update the state
      if ((millis() - lastDebounceTime[row][col]) > debounceDelay) {
        if (reading != currentState[row][col]) {
          currentState[row][col] = reading;
          
          // Calculate key index (0-11 for 4x3 matrix)
          uint8_t keyIndex = (row * COLS) + col;
          
          if (reading == LOW) { // Button pressed (pulled LOW)
            sendAction(keyIndex, true);
          } else { // Button released
            sendAction(keyIndex, false);
          }
        }
      }
      
      previousState[row][col] = reading;
    }
    
    // Set current row back to HIGH
    digitalWrite(rowPins[row], HIGH);
  }
}

void sendAction(uint8_t keyIndex, bool isPressed) {
  if (!keymap_received || keyIndex >= keymap_size) {
    Serial.println("Invalid key index or keymap not received");
    return;
  }
  
  current_key_pressed = keymap[keyIndex];
  key_state = isPressed ? 1 : 0;
  
  // Signal interrupt to master
  digitalWrite(INT_PIN, LOW);
  
  Serial.print("Key ");
  Serial.print(isPressed ? "pressed" : "released");
  Serial.print(": index ");
  Serial.print(keyIndex);
  Serial.print(", action code ");
  Serial.println(current_key_pressed);
}

// Handle receiving keymap from master
void receiveEvent(int bytes) {
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
  
  // Release interrupt pin
  digitalWrite(INT_PIN, HIGH);
}

// Handle master requesting key events
void requestEvent() {
  // Send current key event to master
  Wire.write(current_key_pressed);
  Wire.write(key_state);
  
  // Clear the event after sending and release interrupt
  current_key_pressed = 0;
  key_state = 0;
  digitalWrite(INT_PIN, HIGH); // Release interrupt
}