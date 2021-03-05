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
int ticks_at_pressure = 0;
bool result = false;
bool test_running = false;
bool test_reset = false;
// long test time
unsigned long test_start_millis = 0;
unsigned long pressure_ok_millis = 0;
unsigned long last_print_time = 0;

void setup() {
  Serial.begin(115200);

  pinMode(SWITCH, INPUT_PULLUP);
  pinMode(LED, OUTPUT);
  pinMode(PUMP_CONTROL, OUTPUT);
}

void loop() {
  // Run update function for test
  // (only if the state variable says the test is currently running)
  if (test_running) {
    test_tick();
  }

  // Test to see if the switch has been switched again to low, otherwise the
  // test would continually run while the switch was switched high
  else if (digitalRead(SWITCH) == LOW) {
    if (millis() - last_print_time >= SERIAL_PRINT_DELAY) {
      Serial.println("Now ready to start test");
      last_print_time = millis();
    }
    test_reset = true;
  }

  // Check to see if the switch is activated, and test has been reset
  // If so, we start the test
  else if (test_reset && digitalRead(SWITCH) == HIGH) {
    Serial.println("Starting pressure test...");
    start_test();
  }

  // Test is not running, and we have no switch input
  // Print a message every second
  else {
    if (millis() - last_print_time >= SERIAL_PRINT_DELAY) {
      Serial.println("Pressure test waiting for switch input");
      last_print_time = millis();
    }
  }

  // Wait
  delay(100);
}

void start_test() {
  // Reset our state variables
  reset_state();

  // Record test start time
  test_start_millis = millis();

  // Turn on the pump
  digitalWrite(PUMP_CONTROL, HIGH);

  // Set test running state variable
  test_running = true;
}

/// Reset values of state variables
void reset_state() {
  test_running = false;
  test_reset = false;
  at_pressure = false;
  ticks_at_pressure = 0;
  result = false;
  test_start_millis = 0;
  pressure_ok_millis = 0;
  last_print_time = 0;
}

/// Do work for a running test
void test_tick() {
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
      ticks_at_pressure = 0;
      at_pressure = true;
      pressure_ok_millis = millis();
    } else {
      // Increment counter if at pressure
      ticks_at_pressure += 1;
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

  Serial.print("ticks at pressure: ");//uncommented
  Serial.println(ticks_at_pressure);//uncommented

  unsigned long pressure_ok_duration = millis() - pressure_ok_millis;

  // Update test result
  if (at_pressure) {
    if (pressure_ok_duration > (TEST_TIME * 1000)) {
      result = true;
    }
  } else {
    pressure_ok_duration = 0;
  }

  // Print result
  Serial.print("result: ");
  if (result) {
    Serial.println("PASSED");
    digitalWrite(LED, HIGH);

    // Stop the test, since we have now passed
    stop_test();
  } else {
    Serial.println("FAILED");
    digitalWrite(LED, LOW);
  }

  if (millis() - last_print_time >= SERIAL_PRINT_DELAY) {
    Serial.print ("Time since start:");
    Serial.println((millis() - test_start_millis) / 1000);
    Serial.print ("Time at pressure:");
    Serial.println(pressure_ok_duration / 1000);

    last_print_time = millis();
  }

}

/// Stop the test
void stop_test() {
  // Update state
  test_running = false;

  // Turn off the pump
  digitalWrite(PUMP_CONTROL, LOW);
}

// TODO:
// Tone
