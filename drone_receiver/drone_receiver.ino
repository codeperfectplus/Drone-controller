#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

// --- nRF24 config (CE, CSN) ---
RF24 radio(9, 10);
const byte address[6] = "07001";

// --- Input data struct from transmitter ---
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

// --- Timing/failsafe ---
unsigned long lastPacketTime = 0;
const unsigned long TIMEOUT = 1000; // ms

// --- LEDs (unchanged) ---
#define LED1 3
#define LED2 2

// --- Channel buffers (1000..2000us) ---
#define PPM_NUM_CHANNELS 10
uint16_t ppmChannels[PPM_NUM_CHANNELS] = {
  1500,1500,1500,1500,1500,1500,1500,1500,1500,1500
};

// ==================== PPM Output Config ====================
// Choose how many channels you want to emit in the PPM stream.
// Many FCs support 8 channels on PPM; iNav can support more.
// Set to 8 for maximum compatibility, or 10 to pass all your channels.
#define PPM_CHANNELS 10  // set to 8 or 10

// PPM signal parameters
#define PPM_PIN 4                 // PPM output pin
#define PPM_FRAME_LENGTH 22500    // total frame length in microseconds
#define PPM_PULSE_LENGTH 300      // length of each short PPM pulse in microseconds

// Polarity: 0 = negative shift (idle HIGH, short LOW pulses) - common for FCs
//           1 = positive shift (idle LOW, short HIGH pulses)
#define PPM_POLARITY 0

// PPM channel buffer (volatile, read by ISR)
volatile uint16_t ppmBuffer[PPM_CHANNELS];

// ==================== PPM Timer/ISR ====================
void setupPPM() {
  pinMode(PPM_PIN, OUTPUT);

  // Set idle state according to polarity
  if (PPM_POLARITY == 0) {
    digitalWrite(PPM_PIN, HIGH); // idle HIGH, pulses LOW
  } else {
    digitalWrite(PPM_PIN, LOW);  // idle LOW, pulses HIGH
  }

  // Timer1 configuration: CTC mode, prescaler 8, 16MHz -> 0.5us per tick
  // Compare match interrupt drives pulse/space timing.
  cli();
  TCCR1A = 0;           // normal operation
  TCCR1B = 0;
  TCNT1 = 0;
  OCR1A = PPM_PULSE_LENGTH * 2;  // first interval
  TCCR1B |= (1 << WGM12);        // CTC mode
  TCCR1B |= (1 << CS11);         // prescaler 8
  TIMSK1 |= (1 << OCIE1A);       // enable compare A interrupt
  sei();
}

// Fast constrain to 1000..2000
inline uint16_t clamp1000_2000(uint16_t v) {
  if (v < 1000) return 1000;
  if (v > 2000) return 2000;
  return v;
}

// Interrupt routine generating PPM on PPM_PIN
ISR(TIMER1_COMPA_vect) {
  static bool state = true;       // true=we're emitting the short pulse next
  static uint8_t ch = 0;
  static uint16_t accum = 0;      // accumulated channel times within frame

  if (state) {
    // Emit the short pulse
    if (PPM_POLARITY == 0) {
      // negative shift: pulse LOW
      digitalWrite(PPM_PIN, LOW);
    } else {
      // positive shift: pulse HIGH
      digitalWrite(PPM_PIN, HIGH);
    }
    OCR1A = PPM_PULSE_LENGTH * 2; // schedule pulse duration
    state = false;
  } else {
    // Emit the variable space after the pulse
    if (PPM_POLARITY == 0) {
      // negative shift: back to idle HIGH
      digitalWrite(PPM_PIN, HIGH);
    } else {
      // positive shift: back to idle LOW
      digitalWrite(PPM_PIN, LOW);
    }
    state = true;

    uint16_t gap;
    if (ch >= PPM_CHANNELS) {
      // Sync gap: remainder of frame
      uint16_t remainder = (PPM_FRAME_LENGTH > accum) ? (PPM_FRAME_LENGTH - accum) : 3000; // safety fallback
      gap = remainder;
      ch = 0;
      accum = 0;
    } else {
      uint16_t v = ppmBuffer[ch];
      accum += v;
      ch++;
      // Space = channel width - pulse length
      gap = (v > PPM_PULSE_LENGTH) ? (v - PPM_PULSE_LENGTH) : 700; // safety fallback
    }
    OCR1A = gap * 2; // convert us to timer ticks (0.5us per tick)
  }
}

// ==================== Radio + Mapping ====================
void setup() {
  // Serial optional for debug
  Serial.begin(115200);

  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  digitalWrite(LED1, LOW);
  digitalWrite(LED2, LOW);

  // Initialize PPM buffer with neutral values
  for (uint8_t i = 0; i < PPM_CHANNELS; i++) {
    ppmBuffer[i] = 1500;
  }

  // Start PPM generation
  setupPPM();

  // nRF24 setup
  radio.begin();
  radio.openReadingPipe(0, address);
  radio.setPALevel(RF24_PA_MAX);
  radio.setDataRate(RF24_250KBPS);
  radio.setChannel(110);
  radio.setAutoAck(true);
  radio.setRetries(5, 15);
  radio.startListening();
}

void loop() {
  JoystickData data;

  if (radio.available()) {
    radio.read(&data, sizeof(data));
    lastPacketTime = millis();


  // Map inputs to channel microseconds
  ppmChannels[0] = map(data.joy1_x, 0, 1023, 1000, 2000); // CH1
  ppmChannels[1] = map(data.joy1_y, 0, 1023, 1000, 2000); // CH2
  ppmChannels[2] = map(data.joy2_x, 0, 1023, 1000, 2000); // CH3
  ppmChannels[3] = map(data.joy2_y, 0, 1023, 1000, 2000); // CH4
  ppmChannels[4] = data.joy1_sw ? 2000 : 1000;            // CH5
  ppmChannels[5] = data.joy2_sw ? 2000 : 1000;            // CH6
  ppmChannels[6] = map(data.pot1, 0, 1023, 1000, 2000);   // CH7
  ppmChannels[7] = map(data.pot2, 0, 1023, 1000, 2000);   // CH8
  ppmChannels[8] = (data.toggle1 == 1) ? 2000 : ((data.toggle1 == -1) ? 1000 : 1500); // CH9
  ppmChannels[9] = (data.toggle2 == 1) ? 2000 : ((data.toggle2 == -1) ? 1000 : 1500); // CH10

    // LEDs reflect toggles
    digitalWrite(LED1, data.toggle1 == 1 ? HIGH : LOW);
    digitalWrite(LED2, data.toggle2 == 1 ? HIGH : LOW);
  }

  // Failsafe: if no packet within TIMEOUT, set all channels to 1000
  if (millis() - lastPacketTime > TIMEOUT) {
    digitalWrite(LED1, LOW);
    digitalWrite(LED2, LOW);
    for (uint8_t i = 0; i < PPM_NUM_CHANNELS; i++) {
      ppmChannels[i] = 1000;
    }
  }

  // Copy first PPM_CHANNELS into PPM buffer atomically
  noInterrupts();
  for (uint8_t i = 0; i < PPM_CHANNELS; i++) {
    ppmBuffer[i] = clamp1000_2000(ppmChannels[i]);
  }
  interrupts();
}