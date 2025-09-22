#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

// CE, CSN pins
#define CE_PIN 9
#define CSN_PIN 10
#define ADDRESS "07001"

// Joystick pins
#define JOY1_X A0
#define JOY1_Y A1
#define JOY1_SW 2

#define JOY2_X A2
#define JOY2_Y A3
#define JOY2_SW 3

// Pots
#define POT1 A4
#define POT2 A5

// Toggle switches
#define TOGGLE1_A 4
#define TOGGLE1_B 5
#define TOGGLE2_A 6
#define TOGGLE2_B 7

// LED controlled by Toggle1
#define LED1 10

RF24 radio(CE_PIN, CSN_PIN);
const byte address[6] = ADDRESS;

struct JoystickData {
  int joy1_x;
  int joy1_y;
  int joy1_sw;
  int joy2_x;
  int joy2_y;
  int joy2_sw;
  int pot1;
  int pot2;
  int toggle1; // -1 = left, 0 = middle, 1 = right
  int toggle2;
};

// Read SPDT toggle switch (-1 left, 0 middle, 1 right)
int readToggle(int pinA, int pinB) {
  int a = digitalRead(pinA);
  int b = digitalRead(pinB);
  if (a == LOW && b == HIGH) return -1; // Left
  if (a == HIGH && b == LOW) return 1;  // Right
  return 0;                              // Middle/floating
}

void setup() {
  Serial.begin(9600);

  pinMode(JOY1_SW, INPUT_PULLUP);
  pinMode(JOY2_SW, INPUT_PULLUP);

  pinMode(TOGGLE1_A, INPUT_PULLUP);
  pinMode(TOGGLE1_B, INPUT_PULLUP);
  pinMode(TOGGLE2_A, INPUT_PULLUP);
  pinMode(TOGGLE2_B, INPUT_PULLUP);

  pinMode(LED1, OUTPUT);

  radio.begin();
  radio.openWritingPipe(address);
  radio.setPALevel(RF24_PA_MAX);
  radio.setDataRate(RF24_250KBPS);
  radio.setChannel(110);
  radio.setAutoAck(true);
  radio.setRetries(5, 15);
  radio.stopListening(); // TX mode
}

void loop() {
  JoystickData data;

  // Read joysticks
  data.joy1_x = analogRead(JOY1_X);
  data.joy1_y = analogRead(JOY1_Y);
  data.joy1_sw = !digitalRead(JOY1_SW);

  data.joy2_x = analogRead(JOY2_X);
  data.joy2_y = analogRead(JOY2_Y);
  data.joy2_sw = !digitalRead(JOY2_SW);

  // Read pots
  data.pot1 = analogRead(POT2);
  data.pot2 = analogRead(POT1);

  // Read toggles
  data.toggle1 = readToggle(TOGGLE2_B, TOGGLE2_A);
  data.toggle2 = readToggle(TOGGLE1_A, TOGGLE1_B);

  // Control LED1 based on Toggle1
  digitalWrite(LED1, data.toggle1 == 1 ? HIGH : LOW);

  // Send joystick data
  bool ok = radio.write(&data, sizeof(data));

  // Debug output
  if (ok) {
    Serial.print("Sent -> J1(");
    Serial.print(data.joy1_x); Serial.print(","); Serial.print(data.joy1_y);
    Serial.print(" SW:"); Serial.print(data.joy1_sw);
    Serial.print(") | J2(");
    Serial.print(data.joy2_x); Serial.print(","); Serial.print(data.joy2_y);
    Serial.print(" SW:"); Serial.print(data.joy2_sw);
    Serial.print(") | Pot1: "); Serial.print(data.pot1);
    Serial.print(" Pot2: "); Serial.print(data.pot2);
    Serial.print(" | Toggle1: "); Serial.print(data.toggle1);
    Serial.print(" | Toggle2: "); Serial.println(data.toggle2);
  } else {
    Serial.println("Send failed!");
  }

  delay(50);
}
