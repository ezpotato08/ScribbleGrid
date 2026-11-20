#include <Servo.h>
#include <PS2Mouse.h>

// Motor Pins
// CNC Shield X STEP = Arduino D2
// CNC Shield Y STEP = Arduino D3
const int X_STEP_PIN = 2;
const int Y_STEP_PIN = 3;

// CNC Shield X DIR = Arduino D5
// CNC Shield Y DIR = Arduino D6
const int X_DIR_PIN = 5;
const int Y_DIR_PIN = 6;

// CNC Shield ENABLE = Arduino D8
const int ENABLE_PIN = 8;

// Limit switch pins
// CNC Shield X+/X- = Arduino D9
// CNC Shield Y+/Y- = Arduino D10
const int X_LIMIT_PIN = 9;
const int Y_LIMIT_PIN = 10;

// Homing directions
const bool X_DIRECTION_TOWARD_SWITCH = LOW;
const bool Y_DIRECTION_TOWARD_SWITCH = LOW;

// Servo - CNC Shield Z+/Z- signal = Arduino D11
const int SERVO_PIN = 11;

Servo penServo;

const int PEN_UP_ANGLE = 90;
const int PEN_DOWN_ANGLE = 10;
const int PEN_CHANGE_ANGLE = 140;

// Pen button - CNC Shield Resume = Arduino A2
const int PEN_BUTTON_PIN = A2;

bool penIsDown = false;
bool lastButtonState = HIGH;

unsigned long lastDebounceTime = 0;
const unsigned long DEBOUNCE_DELAY = 50;

const unsigned long REHOME_HOLD_TIME = 3000;
unsigned long buttonPressStartTime = 0;

bool buttonIsBeingHeld = false;
bool longPressHandled = false;
bool penChangeMode = false;

// Joystick
// CNC Shield Abort = Arduino A0
// CNC Shield Hold = Arduino A1
const int JOY_X_PIN = A0;
const int JOY_Y_PIN = A1;

const int CENTER = 512;
const int DEADBAND = 140;

// Trackball
// CNC Shield CoolEn = Arduino A3 (DATA)
// CNC Shield SpnEn = Arduino D12 (CLOCK)
const int TRACKBALL_DATA_PIN = A3;
const int TRACKBALL_CLOCK_PIN = 12;

// REMOTE mode lets Arduino request movement packets
PS2Mouse mouse(
  TRACKBALL_CLOCK_PIN,
  TRACKBALL_DATA_PIN,
  REMOTE
);

const bool INVERT_TRACKBALL_X = false;
const bool INVERT_TRACKBALL_Y = true;

// Motor steps per trackball movement
const int TRACKBALL_STEPS_PER_COUNT = 3;

// Trackball drawing speed
const unsigned long TRACKBALL_STEP_INTERVAL = 800;

// Joystick speed
const unsigned long STEP_INTERVAL = 100;

// Homing speed
const unsigned long HOMING_STEP_INTERVAL = 500;

unsigned long lastXStep = 0;
unsigned long lastYStep = 0;

// Software limits
// 200 step/rev motor
// GT2 20 tooth pulley = 40 mm/revolution
// 200 / 40 = 5 full steps/mm
const int MICROSTEPS = 8;

const float STEPS_PER_MM = 5.0 * MICROSTEPS;

const float X_TRAVEL_MM = 220.0;
const float Y_TRAVEL_MM = 215.0;

const long X_MAX_STEPS = (long)(X_TRAVEL_MM * STEPS_PER_MM);
const long Y_MAX_STEPS = (long)(Y_TRAVEL_MM * STEPS_PER_MM);

// Position tracking
long xPosition = 0;
long yPosition = 0;

bool xHomed = true;
bool yHomed = true;


// =====================================================
// SETUP
// =====================================================

void setup() {

  pinMode(X_STEP_PIN, OUTPUT);
  pinMode(Y_STEP_PIN, OUTPUT);

  pinMode(X_DIR_PIN, OUTPUT);
  pinMode(Y_DIR_PIN, OUTPUT);

  pinMode(ENABLE_PIN, OUTPUT);

  pinMode(X_LIMIT_PIN, INPUT_PULLUP);
  pinMode(Y_LIMIT_PIN, INPUT_PULLUP);

  pinMode(PEN_BUTTON_PIN, INPUT_PULLUP);

  // Enable DRV8825 drivers
  digitalWrite(ENABLE_PIN, LOW);

  // Servo
  penServo.attach(SERVO_PIN);
  penUp();

  // Home machine
  // homeX();
  // homeY();

  // // Move pen to change position
  // if (xHomed && yHomed) {
  //   penChangePosition();
  // }

  Serial.begin(115200);

  Serial.println("Starting trackball initialization");
  mouse.initialize();
  Serial.println("Trackball initialization complete");
}


// =====================================================
// MAIN LOOP
// =====================================================

void loop() {

  checkPenButton();

  int joyX = analogRead(JOY_X_PIN);
  int joyY = analogRead(JOY_Y_PIN);

  bool joystickActive =
    abs(joyX - CENTER) >= DEADBAND ||
    abs(joyY - CENTER) >= DEADBAND;

  if (penIsDown) {

    // Trackball only when drawing
    handleTrackball();

  }
  else {

    // Joystick has priority when pen is up
    if (joystickActive) {

      moveXJoystick(joyX);
      moveYJoystick(joyY);

    }
    else {

      handleTrackball();

    }
  }
}


// =====================================================
// HOMING
// =====================================================

void homeX() {

  digitalWrite(
    X_DIR_PIN,
    X_DIRECTION_TOWARD_SWITCH
  );

  unsigned long startTime = millis();
  const unsigned long HOMING_TIMEOUT = 20000;

  while (digitalRead(X_LIMIT_PIN) == HIGH) {

    digitalWrite(X_STEP_PIN, HIGH);
    delayMicroseconds(3);

    digitalWrite(X_STEP_PIN, LOW);
    delayMicroseconds(HOMING_STEP_INTERVAL);

    if (millis() - startTime > HOMING_TIMEOUT) {
      xHomed = false;
      return;
    }
  }

  xPosition = 0;
  xHomed = true;
}


void homeY() {

  digitalWrite(
    Y_DIR_PIN,
    Y_DIRECTION_TOWARD_SWITCH
  );

  unsigned long startTime = millis();
  const unsigned long HOMING_TIMEOUT = 20000;

  while (digitalRead(Y_LIMIT_PIN) == HIGH) {

    digitalWrite(Y_STEP_PIN, HIGH);
    delayMicroseconds(3);

    digitalWrite(Y_STEP_PIN, LOW);
    delayMicroseconds(HOMING_STEP_INTERVAL);

    if (millis() - startTime > HOMING_TIMEOUT) {
      yHomed = false;
      return;
    }
  }

  yPosition = 0;
  yHomed = true;
}


// =====================================================
// JOYSTICK X
// =====================================================

void moveXJoystick(int joystickValue) {

  if (!xHomed) {
    return;
  }

  int distanceFromCenter = joystickValue - CENTER;

  if (abs(distanceFromCenter) < DEADBAND) {
    return;
  }

  bool desiredDirection;

  if (distanceFromCenter > 0) {
    desiredDirection = HIGH;
  }
  else {
    desiredDirection = LOW;
  }

  // Physical limit
  if (
    digitalRead(X_LIMIT_PIN) == LOW &&
    desiredDirection == X_DIRECTION_TOWARD_SWITCH
  ) {
    xPosition = 0;
    return;
  }

  // Software limit
  if (
    desiredDirection != X_DIRECTION_TOWARD_SWITCH &&
    xPosition >= X_MAX_STEPS
  ) {
    return;
  }

  digitalWrite(X_DIR_PIN, desiredDirection);

  unsigned long currentTime = micros();

  if (currentTime - lastXStep >= STEP_INTERVAL) {

    lastXStep = currentTime;

    digitalWrite(X_STEP_PIN, HIGH);
    delayMicroseconds(3);
    digitalWrite(X_STEP_PIN, LOW);

    if (desiredDirection == X_DIRECTION_TOWARD_SWITCH) {

      if (xPosition > 0) {
        xPosition--;
      }

    }
    else {
      xPosition++;
    }
  }
}


// =====================================================
// JOYSTICK Y
// =====================================================

void moveYJoystick(int joystickValue) {

  if (!yHomed) {
    return;
  }

  int distanceFromCenter = joystickValue - CENTER;

  if (abs(distanceFromCenter) < DEADBAND) {
    return;
  }

  bool desiredDirection;

  if (distanceFromCenter > 0) {
    desiredDirection = HIGH;
  }
  else {
    desiredDirection = LOW;
  }

  // Physical limit
  if (
    digitalRead(Y_LIMIT_PIN) == LOW &&
    desiredDirection == Y_DIRECTION_TOWARD_SWITCH
  ) {
    yPosition = 0;
    return;
  }

  // Software limit
  if (
    desiredDirection != Y_DIRECTION_TOWARD_SWITCH &&
    yPosition >= Y_MAX_STEPS
  ) {
    return;
  }

  digitalWrite(Y_DIR_PIN, desiredDirection);

  unsigned long currentTime = micros();

  if (currentTime - lastYStep >= STEP_INTERVAL) {

    lastYStep = currentTime;

    digitalWrite(Y_STEP_PIN, HIGH);
    delayMicroseconds(3);
    digitalWrite(Y_STEP_PIN, LOW);

    if (desiredDirection == Y_DIRECTION_TOWARD_SWITCH) {

      if (yPosition > 0) {
        yPosition--;
      }

    }
    else {
      yPosition++;
    }
  }
}


// =====================================================
// TRACKBALL
// =====================================================

void handleTrackball() {

  int16_t data[3];

  mouse.report(data);

  int xMovement = data[1];
  int yMovement = data[2];

  // Original direction inversion
  if (INVERT_TRACKBALL_X) {
    xMovement = -xMovement;
  }

  if (INVERT_TRACKBALL_Y) {
    yMovement = -yMovement;
  }

  // ===================================================
  // ROTATE TRACKBALL 90 DEGREES LEFT
  // ===================================================
  // Transformation:
  // new X = -old Y
  // new Y =  old X

  int oldXMovement = xMovement;

  xMovement = -yMovement;
  yMovement = oldXMovement;

  // ===================================================

  if (xMovement == 0 && yMovement == 0) {
    return;
  }

  long xSteps =
    (long)xMovement * TRACKBALL_STEPS_PER_COUNT;

  long ySteps =
    (long)yMovement * TRACKBALL_STEPS_PER_COUNT;

  drawRelativeMovement(xSteps, ySteps);
}


// =====================================================
// BRESENHAM LINE ALGORITHM
// =====================================================

void drawRelativeMovement(long xSteps, long ySteps) {

  long targetX = xSteps;
  long targetY = ySteps;

  long currentX = 0;
  long currentY = 0;

  long dx = abs(targetX);
  long dy = -abs(targetY);

  int sx;

  if (targetX >= 0) {
    sx = 1;
  }
  else {
    sx = -1;
  }

  int sy;

  if (targetY >= 0) {
    sy = 1;
  }
  else {
    sy = -1;
  }

  long error = dx + dy;

  while (
    currentX != targetX ||
    currentY != targetY
  ) {

    long error2 = 2 * error;

    bool stepXNow = false;
    bool stepYNow = false;

    if (error2 >= dy) {

      error += dy;
      currentX += sx;

      stepXNow = true;
    }

    if (error2 <= dx) {

      error += dx;
      currentY += sy;

      stepYNow = true;
    }

    stepDrawingAxes(
      stepXNow,
      sx,
      stepYNow,
      sy
    );

    delayMicroseconds(TRACKBALL_STEP_INTERVAL);
  }
}


// =====================================================
// TRACKBALL MOTOR STEPPING
// =====================================================

void stepDrawingAxes(
  bool requestX,
  int xDirection,
  bool requestY,
  int yDirection
) {

  bool stepX = false;
  bool stepY = false;

  // X AXIS

  if (requestX && xHomed) {

    if (xDirection < 0) {

      if (digitalRead(X_LIMIT_PIN) == LOW) {

        xPosition = 0;

      }
      else if (xPosition > 0) {

        digitalWrite(
          X_DIR_PIN,
          X_DIRECTION_TOWARD_SWITCH
        );

        stepX = true;
      }
    }
    else {

      if (xPosition < X_MAX_STEPS) {

        digitalWrite(
          X_DIR_PIN,
          !X_DIRECTION_TOWARD_SWITCH
        );

        stepX = true;
      }
    }
  }

  // Y AXIS

  if (requestY && yHomed) {

    if (yDirection < 0) {

      if (digitalRead(Y_LIMIT_PIN) == LOW) {

        yPosition = 0;

      }
      else if (yPosition > 0) {

        digitalWrite(
          Y_DIR_PIN,
          Y_DIRECTION_TOWARD_SWITCH
        );

        stepY = true;
      }
    }
    else {

      if (yPosition < Y_MAX_STEPS) {

        digitalWrite(
          Y_DIR_PIN,
          !Y_DIRECTION_TOWARD_SWITCH
        );

        stepY = true;
      }
    }
  }

  // Generate step pulses

  if (stepX) {
    digitalWrite(X_STEP_PIN, HIGH);
  }

  if (stepY) {
    digitalWrite(Y_STEP_PIN, HIGH);
  }

  delayMicroseconds(3);

  if (stepX) {
    digitalWrite(X_STEP_PIN, LOW);
  }

  if (stepY) {
    digitalWrite(Y_STEP_PIN, LOW);
  }

  // Update position

  if (stepX) {

    if (xDirection > 0) {
      xPosition++;
    }
    else {
      xPosition--;
    }
  }

  if (stepY) {

    if (yDirection > 0) {
      yPosition++;
    }
    else {
      yPosition--;
    }
  }
}


// =====================================================
// PEN BUTTON
// =====================================================

void checkPenButton() {

  bool buttonState = digitalRead(PEN_BUTTON_PIN);

  // Button pressed
  if (
    buttonState == LOW &&
    lastButtonState == HIGH
  ) {

    if (
      millis() - lastDebounceTime >
      DEBOUNCE_DELAY
    ) {

      buttonPressStartTime = millis();

      buttonIsBeingHeld = true;
      longPressHandled = false;

      lastDebounceTime = millis();
    }
  }

  // Button held
  if (
    buttonState == LOW &&
    buttonIsBeingHeld &&
    !longPressHandled
  ) {

    if (
      millis() - buttonPressStartTime >=
      REHOME_HOLD_TIME
    ) {

      longPressHandled = true;

      rehomeDevice();
    }
  }

  // Button released
  if (
    buttonState == HIGH &&
    lastButtonState == LOW &&
    buttonIsBeingHeld
  ) {

    if (!longPressHandled) {

      // Pen change mode
      if (penChangeMode) {

        penUp();
        penIsDown = false;
        penChangeMode = false;

        delay(250);
      }

      // Normal pen toggle
      else {

        penIsDown = !penIsDown;

        if (penIsDown) {

          penDown();

          delay(250);

          // Discard accumulated trackball movement
          int16_t throwAway[3];
          mouse.report(throwAway);

        }
        else {

          penUp();
          delay(250);
        }
      }
    }

    buttonIsBeingHeld = false;

    lastDebounceTime = millis();
  }

  lastButtonState = buttonState;
}


// =====================================================
// SERVO
// =====================================================

void penUp() {
  penServo.write(PEN_UP_ANGLE);
}

void penDown() {
  penServo.write(PEN_DOWN_ANGLE);
}

void penChangePosition() {

  penServo.write(PEN_CHANGE_ANGLE);

  penIsDown = false;
  penChangeMode = true;

  delay(250);
}


// =====================================================
// REHOME
// =====================================================

void rehomeDevice() {

  penUp();

  penIsDown = false;

  delay(250);

  xHomed = false;
  yHomed = false;

  homeX();
  homeY();

  if (xHomed && yHomed) {
    penChangePosition();
  }
}

// void setup() {

// }

// void loop() {

// }

