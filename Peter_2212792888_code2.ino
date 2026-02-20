#define SIMULATION_MODE 1 // 1 = LED simulation (Tinkercad), 0 = Physical Arduino + Motor Driver
#include <IRremote.hpp>

//Pin mapping
int motorPins[] = {4, 5, 6, 7};
int valvePin = 8;
int pumpPin = 9;
int buttonPin = 10;
int waterSensorPin = A5;
int irPin = 3;
bool started = false;

//Power
int powerLevel = 0;

//WATER LEVEL THRESHOLDS
const int WATER_FULL = 820; // ~80% of 1023
const int WATER_EMPTY = 100; // ~10% of 1023

//IR remote codes
long CODE_FOR_0 = 0xC;
long CODE_FOR_1 = 0xEF10BF00;
long CODE_FOR_2 = 0xEE11BF00;
long CODE_FOR_3 = 0xED12BF00;

void setup() {
  Serial.begin(9600);
  for (int i = 0; i < 4; i++) pinMode(motorPins[i], OUTPUT);
  pinMode(valvePin, OUTPUT);
  pinMode(pumpPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);

  IrReceiver.begin(irPin, ENABLE_LED_FEEDBACK);

  Serial.println(SIMULATION_MODE ? "Simulation Mode with IR remote" : "Hardware Mode with IR remote");
}

void loop() {
  readIR();
  if (digitalRead(buttonPin) == HIGH) {
    Serial.println("Starting washing machine...");
    delay(500);
    fillDrum(); // requires level 1
    wash_rinse("Washing", 2); // requires level 2
    draining(); // requires level 1
    fillDrum(); // requires level 1
    wash_rinse("Rinsing", 2); // requires level 2
    draining(); // requires level 1
    spinning(); // requires level 3
    Serial.println("Finished");
    delay(1000);
  }
}

// IR HANDLING
void readIR() {
  if (IrReceiver.decode()) {
    unsigned long code = IrReceiver.decodedIRData.decodedRawData;
    if (code == CODE_FOR_0) powerLevel = 0;
    else if (code == CODE_FOR_1) powerLevel = 1;
    else if (code == CODE_FOR_2) powerLevel = 2;
    else if (code == CODE_FOR_3) powerLevel = 3;
    Serial.print("IR code received: 0x");
    Serial.println(code, HEX);
    Serial.print("Power level set to: ");
    Serial.println(powerLevel);
    IrReceiver.resume(); // ready for next signal
  }
}

// POWER CHECK
bool checkPower(int requiredLevel) {
  return powerLevel >= requiredLevel;
}

void waitForPower(int requiredLevel) {
  while (!checkPower(requiredLevel)) {
    Serial.print("Waiting for power level ");
    Serial.println(requiredLevel);
    readIR();
    delay(500);
  }
  Serial.println("Sufficient power available. Resuming...");
}

//Fill drum until >80%
void fillDrum() {
  waitForPower(1);
  Serial.println("Filling drum with water...");
  digitalWrite(valvePin, HIGH);
  while (analogRead(waterSensorPin) < WATER_FULL) {
    int level = analogRead(waterSensorPin);
    Serial.print("Water level: ");
    Serial.println(level);
    delay(500);
  }
  digitalWrite(valvePin, LOW);
  Serial.println("Drum filled to 80%");
}

//Wash & rinse
void wash_rinse(String phase, int pwr) {
  waitForPower(pwr);
  Serial.println(phase + " cycle started");
  for (int repeat = 0; repeat < 3; repeat++) {
    for (int rot = 0; rot < 3; rot++) {
      for (int step = 0; step < 4; step++) {
        stepClockwise(step);
        delay(300);
      }
    }
    stopMotor();
    delay(100);
    for (int rot = 0; rot < 3; rot++) {
      for (int step = 0; step < 4; step++) {
        stepCounterClockwise(step);
        delay(300);
      }
    }
    stopMotor();
    delay(100);
  }
  Serial.println(phase + " cycle complete.");
}

//Drain until <10%
void draining() {
  waitForPower(1);
  Serial.println("Draining water...");
  digitalWrite(pumpPin, HIGH);
  while (analogRead(waterSensorPin) > WATER_EMPTY) {
    int level = analogRead(waterSensorPin);
    Serial.print("Water level: ");
    Serial.println(level);
    delay(500);
  }
  digitalWrite(pumpPin, LOW);
  Serial.println("Water drained to below 10%");
}

//Spin
void spinning() {
  waitForPower(3);
  Serial.println("Spinning...");
  for (int i = 0; i < 20; i++) {
    for (int step = 0; step < 4; step++) {
      stepClockwise(step);
      delay(200);
    }
  }
  stopMotor();
  Serial.println("Spin cycle complete.");
}

//Motor Cycles
void stepClockwise(int step) {
#if SIMULATION_MODE == 1
  for (int i = 0; i < 4; i++)
    digitalWrite(motorPins[i], (i == step) ? HIGH : LOW);
#else
  digitalWrite(4, HIGH);
  digitalWrite(5, LOW);
  analogWrite(6, 180);
#endif
}

void stepCounterClockwise(int step) {
#if SIMULATION_MODE == 1
  for (int i = 0; i < 4; i++)
    digitalWrite(motorPins[i], (i == (3 - step)) ? HIGH : LOW);
#else
  digitalWrite(4, LOW);
  digitalWrite(5, HIGH);
  analogWrite(6, 180);
#endif
}

void stopMotor() {
#if SIMULATION_MODE == 1
  for (int i = 0; i < 4; i++)
    digitalWrite(motorPins[i], LOW);
#else
  analogWrite(6, 0);
  digitalWrite(4, LOW);
  digitalWrite(5, LOW);
#endif
}
