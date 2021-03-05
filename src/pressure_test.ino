//Start Switch in LOW
/*vacuum pump test
  Wait for the pressure to reach MIN. target pressure after starting the pump.
  Once the target is reached, PUMP TURNS OFF, count the number of seconds elapsed while the
  pressure is within the target pressure.
  If at any point the pressure is outside the range, state is reset, and the
  counter will start again ASWELL AS PUMP .
*/

#include <Arduino.h>

// Pins
#define PRESSURE_SENSOR A5
#define PUMP_CONTROL 3
#define SWITCH 2
#define LED 4

// Analog reading for target pressure
// = 3.7/5.0 * 1024 (10-bit)
#define TARGET_PRESSURE 675.84 //758 is 3.7, 700 is 3.4179, 675.84 is 3.3
#define TARGET_PRESSURE_MIN 630 //630 is 3.0, 550 is 2.68

// Test time for pass result (seconds)
#define TEST_TIME 20

// Range enable
#define RANGE_TEST 1
// Run pump back to pressure
#define AUTO_PUMP 1

// Serial print delay
#define SERIAL_PRINT_DELAY 1000 // 1s

// State variables
bool at_pressure = false;
int milliseconds_at_pressure = 0;
bool result = false;

// long test time
unsigned long test_start = 0;
unsigned long pressure_ok_start = 0;
unsigned long last_print_time = 0;

void setup() {
  Serial.begin(115200);
  Serial.print("Pressure test waiting for switch input");

  // Wait for switch pin to go low
  pinMode(SWITCH, INPUT_PULLUP);
  while (digitalRead(SWITCH) == LOW) {
    delay(50);
  }

  // Turn on the pump at start-up
  pinMode(PUMP_CONTROL, OUTPUT);
  digitalWrite(PUMP_CONTROL, HIGH);

  pinMode(LED, OUTPUT);


  test_start = millis();
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
      milliseconds_at_pressure = 0;
      at_pressure = true;
      pressure_ok_start = millis();
    } else {
      // Increment counter if at pressure
      milliseconds_at_pressure += 1;

    }

  } else if (raw <= TARGET_PRESSURE_MIN) {
    // Pressure has reached minimum level
    digitalWrite(PUMP_CONTROL, LOW);
    at_pressure = false;
  } else {
    // Not in pressure range (above target)
    if (AUTO_PUMP) {
      digitalWrite(PUMP_CONTROL, HIGH);
    }
    at_pressure = false;
  }

  Serial.print("milliseconds at pressure: ");//uncommented
  Serial.println(milliseconds_at_pressure);//uncommented

  unsigned long pressure_ok_millis = millis() - pressure_ok_start;

  // Update test result
  if (at_pressure) {
    if (pressure_ok_millis > (TEST_TIME * 1000)) {
      result = true;
    }
  } else {
    pressure_ok_millis = 0;
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

  if (millis() - last_print_time >= SERIAL_PRINT_DELAY) {
    Serial.print ("Time since start:");
    Serial.println((millis() - test_start) / 1000);
    Serial.print ("Time at pressure:");
    Serial.println(pressure_ok_millis / 1000);

    last_print_time = millis();
  }

  // Wait one second
  delay(100);
  }

  // Tone
