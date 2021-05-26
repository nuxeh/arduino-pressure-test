#include <Arduino.h>

// Pins
#define PRESSURE_SENSOR A5
#define PUMP_CONTROL    4
#define VALVE_CONTROL   13 // LED_BULTIN
#define SWITCH          2
#define LED_RED         11
#define LED_GREEN       12
#define LED_BLUE        3

// Analog reading for target pressure
// = 3.7/5.0 * 1024 (10-bit)          lowest pressure 1.54
#define TARGET_PRESSURE 630 //758 is 3.7,      700 is 3.4179,        675.84 is 3.3      630 is 3.0v
#define TARGET_PRESSURE_MIN 512      //630 is 3.0,     550 is 2.68       409.6 is 2.00v    512 is 2.5v

// Cycle times in milliseconds
#define WARMUP_TIME     10000 // 10s
#define PRESSURIZE_TIME 10000 // 10s
#define TEST_TIME       20000 // 20s

// State variables
bool test_failed = false;
bool test_reset = false;
unsigned long start_time = 0; // used for timing in all states

enum state_value {
  IDLE,
  WARMUP,
  PRESSURIZE,
  TEST,
};
enum state_value state = IDLE;

void setup() {
  Serial.begin(115200);
  pinMode(SWITCH, INPUT_PULLUP);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  pinMode(PUMP_CONTROL, OUTPUT);
  pinMode(VALVE_CONTROL, OUTPUT);

  // Entry point - start the idle state
  start_idle();
}

void loop() {
  // Run tick functions appropriate to the current state value
  if (state == IDLE) {
    tick_idle();
  }
  else if (state == WARMUP) {
    tick_warmup();
  }
  else if (state == PRESSURIZE) {
    tick_pressurize();
  }
  else if (state == TEST) {
    tick_test();
  }

  // Wait
  delay(1000);
}

/// Reset values of state variables
void reset_state() {
  state = IDLE;
  test_reset = false;
  start_time = 0;
  test_failed = false;
}

void start_idle() {
  Serial.println("Going idle");

  reset_state();

  // Update state
  state = IDLE;
}

void start_warmup() {
  start_time = millis();

  // Serial print
  Serial.print("Starting warmup");

  // Reset LEDs
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_GREEN, LOW);

  // Start Pumps
  digitalWrite(PUMP_CONTROL, HIGH);

  // Update state
  state = WARMUP;
}

void start_pressurize() {
  start_time = millis();

  Serial.println("Starting pressurization");

  // Turn on the valve
  digitalWrite(VALVE_CONTROL, HIGH);

  // Update state
  state = PRESSURIZE;
}

void start_test() {
  start_time = millis();

  // Turn on the LED
  digitalWrite(LED_BLUE, HIGH);

  // Update state
  state = TEST;
}

void end_warmup() {
  Serial.println("Warmup finished");
  start_pressurize();
}

void end_pressurize() {
  Serial.print("Pressurization finished");

  // Read and print the pressure reached
  int raw = analogRead(PRESSURE_SENSOR);
  Serial.print("Raw pressure is: ");
  Serial.println(raw);

  start_test();
}

void end_test() {
  Serial.println("Test finished");

  // Turn off the pump
  digitalWrite(PUMP_CONTROL, LOW);

  delay(500);

  // Turn valve
  digitalWrite(VALVE_CONTROL, LOW);

  // Print result
  Serial.print("Result: ");
  if (!test_failed) {
    Serial.println("FAILED");
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_RED, HIGH);
  } else {
    Serial.println("PASSED");
    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(LED_RED, LOW);
  }

  start_idle();
}

void tick_idle() {
  // Flash the LED
  digitalWrite(LED_BLUE, !digitalRead(LED_BLUE));

  // Test to see if the switch has been switched again to low, otherwise the
  // test would continually run while the switch was switched high
  if (digitalRead(SWITCH) == LOW) {
    Serial.println("Now ready to start test");
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
    Serial.println("Pressure test waiting for switch input");
  }
}

void tick_warmup() {
    if (millis() - start_time >= WARMUP_TIME) {
      end_warmup();
    }
}

void tick_pressurize() {
    if (millis() - start_time >= PRESSURIZE_TIME) {
      end_pressurize();
    }
}

void tick_test() {
  // Get pressure reading
  int raw = analogRead(PRESSURE_SENSOR);

  Serial.print("Current pressure (voltage): ");
  Serial.println(((float) raw / 1024.0) * 5.0);

  // If we are outside the minimum pressure, end the test
  if (raw < TARGET_PRESSURE_MIN) {
    test_failed = true;
    end_test();
  }

  Serial.print ("Time since start: ");
  Serial.println((millis() - start_time) / 1000);

  // Test end condition (time elapsed)
  if (millis() - start_time >= TEST_TIME) {
    end_test();
  }
}
