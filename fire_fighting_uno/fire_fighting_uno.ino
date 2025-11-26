#include <Servo.h>

// ---------------- PIN DEFINITIONS ----------------
#define FLAME_SENSOR_LEFT A0
#define FLAME_SENSOR_CENTER A1
#define FLAME_SENSOR_RIGHT A2

#define FIRE_THRESHOLD 800
#define CLOSE_THRESHOLD 400

#define MOTOR_L1 10
#define MOTOR_L2 11
#define MOTOR_R1 8
#define MOTOR_R2 9

#define SPEED 90
#define TSPEED 110

int ENA = 5;
int ENB = 6;

#define RELAY_PIN 7
#define SERVO_PIN 4


Servo waterServo;

unsigned long stateStartTime = 0;
unsigned long sprayStartTime = 0;
unsigned long lastServoMove = 0;

int sprayPos = 60;
bool sweepRight = true;
bool isSpraying = false;

enum RobotState {
  IDLE,
  MOVING_FORWARD,
  TURNING_LEFT,
  TURNING_RIGHT,
  SPRAYING
};

RobotState currentState = IDLE;

int mode = 0;

void setup() {
  Serial.begin(9600);

  pinMode(MOTOR_L1, OUTPUT);
  pinMode(MOTOR_L2, OUTPUT);
  pinMode(MOTOR_R1, OUTPUT);
  pinMode(MOTOR_R2, OUTPUT);

  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);
  analogWrite(ENA, SPEED);
  analogWrite(ENB, SPEED);

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  waterServo.attach(SERVO_PIN);
  waterServo.write(90);

  pinMode(FLAME_SENSOR_LEFT, INPUT);
  pinMode(FLAME_SENSOR_CENTER, INPUT);
  pinMode(FLAME_SENSOR_RIGHT, INPUT);

  Serial.println("UNO Ready");
}
void loop() {

  
  if (Serial.available()) {
    char c = Serial.read();

  
    if (c == 'M') {
      mode = 0;
      stopMotors();
      Serial.println("MODE = MANUAL");
      return;
    }
    if (c == 'A') {
      mode = 1;
      stopMotors();
      Serial.println("MODE = AUTO");
      return;
    }

    
    if (mode == 0) {
      switch (c) {
        case 'F': moveForward(); break;
        case 'B': moveBackward(); break;
        case 'L': turnLeft(); break;
        case 'R': turnRight(); break;
        case 'S': stopMotors(); break;

        case 'P': sprayOnce(); break;

        case 'V': waterServo.write(60); break;   // servo left
        case 'C': waterServo.write(90); break;   // center
        case 'W': waterServo.write(120); break;  // servo right

        case 'O': digitalWrite(RELAY_PIN, HIGH); break;  // light ON
        case 'X': digitalWrite(RELAY_PIN, LOW); break;   // light OFF
      }

     
      return;
    }
  }


  if (mode == 1) {
    runAutoMode();
  }
}


void runAutoMode() {

  int Left = analogRead(FLAME_SENSOR_LEFT);
  int Center = analogRead(FLAME_SENSOR_CENTER);
  int Right = analogRead(FLAME_SENSOR_RIGHT);

  if (currentState != SPRAYING) {
    if (Center < FIRE_THRESHOLD || Left < FIRE_THRESHOLD || Right < FIRE_THRESHOLD) {

      if (Left <= Right && Left <= Center) {
        changeState(TURNING_LEFT);
      }
      else if (Right <= Left && Right <= Center) {
        changeState(TURNING_RIGHT);
      }
      else {
        if (Center < CLOSE_THRESHOLD)
          changeState(SPRAYING);
        else
          changeState(MOVING_FORWARD);
      }
    }
    else {
      changeState(IDLE);
    }
  }

  handleState();
}


void sprayOnce() {
  digitalWrite(RELAY_PIN, HIGH);
  delay(700);
  digitalWrite(RELAY_PIN, LOW);
}



void changeState(RobotState newState) {
  if (currentState == newState) return;

  currentState = newState;
  stateStartTime = millis();

  if (newState != MOVING_FORWARD &&
      newState != TURNING_LEFT &&
      newState != TURNING_RIGHT) {
    stopMotors();
  }

  if (newState == SPRAYING) {
    digitalWrite(RELAY_PIN, HIGH);
    sprayStartTime = millis();
    sprayPos = 60;
    sweepRight = true;
    isSpraying = true;
    waterServo.write(sprayPos);
  }
}

void handleState() {
  unsigned long now = millis();

  switch (currentState) {
    case IDLE:
      stopMotors();
      break;

    case MOVING_FORWARD:
      moveForward();
      if (now - stateStartTime > 500) changeState(IDLE);
      break;

    case TURNING_LEFT:
      turnLeft();
      if (now - stateStartTime > 500) changeState(IDLE);
      break;

    case TURNING_RIGHT:
      turnRight();
      if (now - stateStartTime > 500) changeState(IDLE);
      break;

    case SPRAYING:
      handleAutoSpray();
      break;
  }
}


void handleAutoSpray() {
  unsigned long now = millis();

  if (now - lastServoMove >= 100) {
    lastServoMove = now;

    if (sweepRight) {
      sprayPos += 5;
      if (sprayPos >= 120) sweepRight = false;
    } else {
      sprayPos -= 5;
      if (sprayPos <= 60) sweepRight = true;
    }
    waterServo.write(sprayPos);
  }

  if (now - sprayStartTime >= 2000) {
    digitalWrite(RELAY_PIN, LOW);
    waterServo.write(90);
    isSpraying = false;
    changeState(IDLE);
  }
}


void moveForward() {
  analogWrite(ENA, SPEED);
  analogWrite(ENB, SPEED);
  digitalWrite(MOTOR_L1, HIGH);
  digitalWrite(MOTOR_L2, LOW);
  digitalWrite(MOTOR_R1, HIGH);
  digitalWrite(MOTOR_R2, LOW);
}

void moveBackward() {
  analogWrite(ENA, SPEED);
  analogWrite(ENB, SPEED);
  digitalWrite(MOTOR_L1, LOW);
  digitalWrite(MOTOR_L2, HIGH);
  digitalWrite(MOTOR_R1, LOW);
  digitalWrite(MOTOR_R2, HIGH);
}

void turnLeft() {
  analogWrite(ENA, TSPEED);
  analogWrite(ENB, TSPEED);
  digitalWrite(MOTOR_L1, LOW);
  digitalWrite(MOTOR_L2, HIGH);
  digitalWrite(MOTOR_R1, HIGH);
  digitalWrite(MOTOR_R2, LOW);
}

void turnRight() {
  analogWrite(ENA, TSPEED);
  analogWrite(ENB, TSPEED);
  digitalWrite(MOTOR_L1, HIGH);
  digitalWrite(MOTOR_L2, LOW);
  digitalWrite(MOTOR_R1, LOW);
  digitalWrite(MOTOR_R2, HIGH);
}

void stopMotors() {
  digitalWrite(MOTOR_L1, LOW);
  digitalWrite(MOTOR_L2, LOW);
  digitalWrite(MOTOR_R1, LOW);
  digitalWrite(MOTOR_R2, LOW);
}
