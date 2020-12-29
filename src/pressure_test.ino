/* Arduino vacuum pump test

Wait for the pressure to reach the target pressure after starting the pump.
Once the target is reached, count the number of seconds elapsed while the
pressure is within the target pressure.

If at any point the pressure is outside the range, state is reset, and the
counter will start again.
*/

#include <Arduino.h>

// Pins
#define PRESSURE_SENSOR A3
#define PUMP_CONTROL 1
#define SWITCH 2
#define LED 3

// Analog reading for target pressure
// = 3.7/5.0 * 1024 (10-bit)
#define TARGET_PRESSURE 758
#define TARGET_PRESSURE_MIN 500

// Test time for pass result (seconds)
#define TEST_TIME 20

// Range enable
#define RANGE_TEST 1

// State variables
bool at_pressure = false;
int seconds_at_pressure = 0;
bool result = false;

void setup() {
  Serial.begin(115200);
  Serial.print("Pressure test waiting for switch input");

  // Wait for switch pin to go low
  while (digitalRead(SWITCH) == HIGH) {
    delay(50);
  }

  // Turn on the pump at start-up
  pinMode(PUMP_CONTROL, OUTPUT);
  digitalWrite(PUMP_CONTROL, HIGH);

  pinMode(LED, OUTPUT);
  pinMode(SWITCH, INPUT);
}

void loop() {
  int raw = analogRead(PRESSURE_SENSOR);

  Serial.print("current pressure (voltage): ");
  Serial.println(((float) raw / 1024.0) * 5.0);

#if RANGE_TEST
  if (raw > TARGET_PRESSURE_MIN && raw < TARGET_PRESSURE) {
#else
  if (raw < TARGET_PRESSURE) {
#endif
    // Reset counter once pressure changes from below target to above target
    if (!at_pressure) {
      seconds_at_pressure = 0;
      at_pressure = true;
    } else {
      // Increment counter if at pressure
      seconds_at_pressure += 1;
    }
  }

  Serial.print("seconds at pressure: ");
  Serial.println(seconds_at_pressure);

  // Update test result
  if (seconds_at_pressure > TEST_TIME) {
    result = true;
  }

  // Print result
  Serial.print("result: ");
  if (result) {
    Serial.println("PASSED");
    digitalWrite(LED, HIGH);
  } else {
    Serial.println("FAILED");
    digitalWrite(LED, LOW);
  }

  // Wait one second
  delay(1000);
}

// Tone
