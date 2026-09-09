```cpp
#include <SPI.h>
#include <RF24.h>

// ============================================================================
// HARDWARE PINS (Arduino Pro Micro 3.3V / 8MHz)
// ============================================================================
#define cmdPin        8   // Output to the base of transistor T1
#define lancPin       5   // LANC input (TIP)
#define CE_PIN        9   // nRF24L01 CE
#define CSN_PIN      10   // nRF24L01 CSN

RF24 radio(CE_PIN, CSN_PIN);

// ============================================================================
// ZIFON – 5-BYTE ADDRESSES
//
// Over-the-air address:
// CH1: 02 0C 16 AA A3
// CH2: 04 0E 18 AC A5
// CH3: 06 10 1A AE A7
//
// The RF24 library uses the bytes in reverse order.
// ============================================================================

// CH1:
// const uint8_t ADDR_REVERSED[5] = { 0xA3, 0xAA, 0x16, 0x0C, 0x02 };

// CH2:
const uint8_t ADDR_REVERSED[5] = { 0xA5, 0xAC, 0x18, 0x0E, 0x04 };

// CH3:
// const uint8_t ADDR_REVERSED[5] = { 0xA7, 0xAE, 0x1A, 0x10, 0x06 };


uint8_t payload[15];

// Precisely tuned bit duration for the 8MHz Pro Micro
#define BIT_DURATION 88

#define MODE_IDLE      0
#define MODE_ZOOM_IN   1
#define MODE_ZOOM_OUT  2

uint8_t currentMode = MODE_IDLE;

unsigned long lastPacketTime = 0;
unsigned long holdStartTime = 0;

const unsigned long HOLD_TIMEOUT = 250;

uint8_t lastReportedSpeed = 99;


// ============================================================================
// SEND ONE LANC FRAME
// ============================================================================
bool sendLancCmd(uint8_t mode, uint8_t speed) {

  if (speed > 7) speed = 7;

  uint8_t b0 = 0x28;

  uint8_t b1 =
      (mode == MODE_ZOOM_OUT ? 0x10 : 0x00)
      + (speed * 2);


  // --------------------------------------------------------------------------
  // 1. Wait for the LANC synchronization gap
  // --------------------------------------------------------------------------

  unsigned long timeout = micros();

  while (digitalRead(lancPin) == LOW) {
    if (micros() - timeout > 30000) {
      return false;
    }
  }


  // --------------------------------------------------------------------------
  // 2. Find the long HIGH synchronization gap
  // --------------------------------------------------------------------------

  unsigned long syncStart = micros();

  while (digitalRead(lancPin) == HIGH) {

    if (micros() - syncStart > 4000) {

      // Wait for the START bit of the first byte
      while (digitalRead(lancPin) == HIGH) {

        if (micros() - syncStart > 15000) {
          return false;
        }
      }


      // ======================================================================
      // BYTE 0
      // ======================================================================

      digitalWrite(cmdPin, HIGH);
      delayMicroseconds(BIT_DURATION);

      // LSB first
      for (int i = 0; i < 8; i++) {

        digitalWrite(
          cmdPin,
          (b0 & (1 << i)) ? HIGH : LOW
        );

        delayMicroseconds(BIT_DURATION);
      }

      // Stop bit
      digitalWrite(cmdPin, LOW);


      // ----------------------------------------------------------------------
      // Wait for the START bit of the second byte
      // ----------------------------------------------------------------------

      unsigned long b1Timeout = micros();

      while (digitalRead(lancPin) == HIGH) {

        if (micros() - b1Timeout > 2000) {
          return false;
        }
      }


      // ======================================================================
      // BYTE 1
      // ======================================================================

      digitalWrite(cmdPin, HIGH);
      delayMicroseconds(BIT_DURATION);

      // LSB first
      for (int i = 0; i < 8; i++) {

        digitalWrite(
          cmdPin,
          (b1 & (1 << i)) ? HIGH : LOW
        );

        delayMicroseconds(BIT_DURATION);
      }

      // Stop bit
      digitalWrite(cmdPin, LOW);

      return true;
    }
  }

  return false;
}


// ============================================================================
// SETUP
// ============================================================================
void setup() {

  Serial.begin(115200);
  delay(200);

  Serial.println(
    F("\n=== LANC / ZIFON 5B CONTROLLER (CANON XF205) ===")
  );


  // --------------------------------------------------------------------------
  // LANC
  // --------------------------------------------------------------------------

  pinMode(lancPin, INPUT_PULLUP);

  pinMode(cmdPin, OUTPUT);
  digitalWrite(cmdPin, LOW);


  // --------------------------------------------------------------------------
  // nRF24L01
  // --------------------------------------------------------------------------

  while (!radio.begin()) {

    Serial.println(F("Error: nRF24L01!"));
    delay(1000);
  }


  radio.setDataRate(RF24_1MBPS);


  radio.setAddressWidth(5);


  radio.setPayloadSize(15);

  radio.setAutoAck(false);

  radio.disableCRC();

  radio.setPALevel(RF24_PA_MAX);


  // Address of the ZIFON controller being received
  radio.openReadingPipe(1, ADDR_REVERSED);


  // RF_CH 80 = 2400 + 80 = 2480 MHz
  radio.setChannel(80);

  radio.startListening();


  Serial.println(F("ZIFON CH2"));
  Serial.println(F("Address: 04 0E 18 AC A5"));
  Serial.println(F("RF: 2480 MHz / 1 Mbps"));
  Serial.println(F("Ready.\n"));
}


// ============================================================================
// LOOP
// ============================================================================
void loop() {

  // ==========================================================================
  // 1. RECEIVE ZIFON DATA
  // ==========================================================================

  if (radio.available()) {

    radio.read(&payload, sizeof(payload));


    if (payload[0] != 0x5E) {

      uint8_t id  = payload[0];
      uint8_t cmd = payload[2];


      uint8_t newMode = MODE_IDLE;


      // ----------------------------------------------------------------------
      // ZOOM IN
      // ----------------------------------------------------------------------

      if (id == 0x9A && cmd == 0x1D) {

        newMode = MODE_ZOOM_IN;
      }


      // ----------------------------------------------------------------------
      // ZOOM OUT
      // ----------------------------------------------------------------------

      else if (cmd == 0x1B) {

        newMode = MODE_ZOOM_OUT;
      }


      // ----------------------------------------------------------------------
      // Active command
      // ----------------------------------------------------------------------

      if (newMode != MODE_IDLE) {

        if (currentMode != newMode) {

          holdStartTime = millis();

          currentMode = newMode;

          lastReportedSpeed = 99;


          Serial.print(F("--> START: ZOOM "));

          Serial.println(
            currentMode == MODE_ZOOM_IN
              ? F("IN")
              : F("OUT")
          );
        }


        // Each received packet extends the active state
        lastPacketTime = millis();
      }
    }
  }


  // ==========================================================================
  // 2. SAFETY STOP AFTER BUTTON RELEASE
  // ==========================================================================

  if (
    currentMode != MODE_IDLE &&
    (millis() - lastPacketTime > HOLD_TIMEOUT)
  ) {

    Serial.println(F("--> STOP: Button released"));

    currentMode = MODE_IDLE;

    lastReportedSpeed = 99;
  }


  // ==========================================================================
  // 3. CONTINUOUS LANC FRAME GENERATION
  // ==========================================================================

  if (currentMode != MODE_IDLE) {

    unsigned long holdDuration =
      millis() - holdStartTime;


    // ------------------------------------------------------------------------
    // Zoom speed from 2 to 7 depending on how long the button is held
    // ------------------------------------------------------------------------

    uint8_t currentSpeed =
      2 + (holdDuration / 300);

    if (currentSpeed > 7) {
      currentSpeed = 7;
    }


    // ------------------------------------------------------------------------
    // Debug output only when the speed changes
    // ------------------------------------------------------------------------

    if (currentSpeed != lastReportedSpeed) {

      Serial.print(F("Speed: "));
      Serial.println(currentSpeed);

      lastReportedSpeed = currentSpeed;
    }


    // ------------------------------------------------------------------------
    // Send LANC command
    // ------------------------------------------------------------------------

    sendLancCmd(currentMode, currentSpeed);
  }
}
```
