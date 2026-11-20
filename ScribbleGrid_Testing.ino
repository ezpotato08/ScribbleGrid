const int X_STEP_PIN = 2;
const int Y_STEP_PIN = 3;

const int X_DIR_PIN = 5;
const int Y_DIR_PIN = 6;

const int ENABLE_PIN = 8;

void setup() {
  pinMode(X_STEP_PIN, OUTPUT);
  pinMode(Y_STEP_PIN, OUTPUT);

  pinMode(X_DIR_PIN, OUTPUT);
  pinMode(Y_DIR_PIN, OUTPUT);

  pinMode(ENABLE_PIN, OUTPUT);

  // CNC shield driver enable is active LOW
  digitalWrite(ENABLE_PIN, LOW);
}

void loop() {

  // -------------------------
  // X AXIS FORWARD
  // -------------------------
  digitalWrite(X_DIR_PIN, HIGH);

  for (int i = 0; i < 400; i++) {
    stepMotor(X_STEP_PIN);
  }

  delay(1000);


  // -------------------------
  // X AXIS BACKWARD
  // -------------------------
  digitalWrite(X_DIR_PIN, LOW);

  for (int i = 0; i < 400; i++) {
    stepMotor(X_STEP_PIN);
  }

  delay(1000);


  // -------------------------
  // Y AXIS FORWARD
  // -------------------------
  digitalWrite(Y_DIR_PIN, HIGH);

  for (int i = 0; i < 400; i++) {
    stepMotor(Y_STEP_PIN);
  }

  delay(1000);


  // -------------------------
  // Y AXIS BACKWARD
  // -------------------------
  digitalWrite(Y_DIR_PIN, LOW);

  for (int i = 0; i < 400; i++) {
    stepMotor(Y_STEP_PIN);
  }

  delay(2000);
}


void stepMotor(int stepPin) {
  digitalWrite(stepPin, HIGH);
  delayMicroseconds(500);

  digitalWrite(stepPin, LOW);
  delayMicroseconds(500);
}

