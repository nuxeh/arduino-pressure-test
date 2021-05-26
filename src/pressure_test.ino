#include <Arduino.h>

// Pins
#define PRESSURE_SENSOR A5
#define PUMP_CONTROL 4
#define VALVE_CONTROL 13 // LED_BULTIN
#define SWITCH 2
#define LED 3

// Analog reading for target pressure
// = 3.7/5.0 * 1024 (10-bit)          lowest pressure 1.54
#define TARGET_PRESSURE 630 //758 is 3.7,      700 is 3.4179,        675.84 is 3.3      630 is 3.0v
#define TARGET_PRESSURE_MIN 512      //630 is 3.0,     550 is 2.68       409.6 is 2.00v    512 is 2.5v

// Test time for pass result (seconds)
#define TEST_TIME 10

// Range enable
#define RANGE_TEST 1
// Run pump back to pressure
#define AUTO_PUMP 1

// Serial print delay
#define SERIAL_PRINT_DELAY 1000 // 1s

// Warmup time in milliseconds
#define WARMUP_TIME 10000 // 10s

// State variables
bool at_pressure = false;
int ticks_at_pressure = 0;
bool result = false;
bool test_running = false;
bool warmup_running = false;
bool test_reset = false;
// long test time
unsigned long test_start_millis = 0;
unsigned long pressure_ok_millis = 0;
unsigned long last_print_time = 0;
unsigned long warmup_start_time = 0;

void setup() {
  Serial.begin(115200);

  pinMode(SWITCH, INPUT_PULLUP);
  pinMode(LED, OUTPUT);
  pinMode(PUMP_CONTROL, OUTPUT);
  pinMode(VALVE_CONTROL, OUTPUT);
}

void loop() {
  // Run update function for test
  // (only if the state variable says the test is currently running)
  if (test_running) {
    test_tick();
  }

  if (warmup_running) {
    if (millis() - warmup_start_time >= WARMUP_TIME) {
      end_warmup();
    }
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
    start_warmup();
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
  // Record test start time
  test_start_millis = millis();

  // Turn on the pump
  digitalWrite(PUMP_CONTROL, HIGH);

  // Set test state variable
  test_running = true;
}

void start_warmup() {
  // Reset our state variables
  reset_state();

  // Record test start time
  warmup_start_time = millis();

  // Start Pumps
  digitalWrite(PUMP_CONTROL, HIGH);

  // Serial print
  Serial.print("Warmup started at ");
  Serial.print(warmup_start_time);
  Serial.println("ms");

  // Start warmup
  warmup_running = true;
}

void end_warmup() {
  // Turn on the valve
  digitalWrite(VALVE_CONTROL, HIGH);

  // Serial print
  Serial.println("Warmup finished");

  // Start test
  start_test();
}

/// Reset values of state variables
void reset_state() {
  test_running = false;
  warmup_running = false;
  test_reset = false;
  at_pressure = false;
  ticks_at_pressure = 0;
  result = false;
  test_start_millis = 0;
  pressure_ok_millis = 0;
  last_print_time = 0;
  warmup_start_time = 0;
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
    //5.21.21 Reset test once pressure changes from below target to above
    if (!at_pressure) {
      ticks_at_pressure = 0;
      at_pressure = true;
      pressure_ok_millis = millis();  //why would millis begin if pressure is not in range?
    } else {
      // Increment counter if at pressure
      ticks_at_pressure += 1;
    }

  } else if (raw <= TARGET_PRESSURE_MIN) {
    // Pressure has reached minimum level
    digitalWrite(PUMP_CONTROL, LOW);
    at_pressure = false;    //why at_pressure is false?
  } else {
    // Not in pressure range (above target)
    if (AUTO_PUMP) {
      digitalWrite(PUMP_CONTROL, HIGH);
    }
    at_pressure = false;
  }

  Serial.print("ticks at pressure: ");//uncommented
  Serial.println(ticks_at_pressure);//uncommented

  unsigned long pressure_ok_duration = millis() - pressure_ok_millis;  //pressure_inrange_duration

  // Update test result
  if (at_pressure) {
    if (pressure_ok_duration > (TEST_TIME * 1000)) {
      result = true;
    }
  } else {
    pressure_ok_duration = 0;  //
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
    digitalWrite(LED, LOW);       //add reset test go to idle
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

  // Turn valve
  digitalWrite(VALVE_CONTROL, LOW);
}

// TO DO: Add digital output for ON/OFF valve
