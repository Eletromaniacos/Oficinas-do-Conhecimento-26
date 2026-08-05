#include <Arduino.h>
#include <ESP32Servo.h>

// Pinos
#define SERVO_X_PIN 22
#define SERVO_Y_PIN 23

#define BTN_LEFT  5
#define BTN_RIGHT 18
#define BTN_UP    19
#define BTN_DOWN  21

// Configurações
#define SERVO_CENTER  45
#define SERVO_MIN     0
#define SERVO_MAX     90
#define STEP          2      // graus por ciclo
#define LOOP_DELAY    10     // ms

Servo servoX;
Servo servoY;

float angleX = SERVO_CENTER;
float angleY = SERVO_CENTER;

void setup() {
  servoX.attach(SERVO_X_PIN);
  servoY.attach(SERVO_Y_PIN);

  servoX.write(angleX);
  servoY.write(angleY);

  pinMode(BTN_LEFT,  INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  pinMode(BTN_UP,    INPUT_PULLUP);
  pinMode(BTN_DOWN,  INPUT_PULLUP);
}

void loop() {
  if (digitalRead(BTN_LEFT)  == LOW) angleX -= STEP;
  if (digitalRead(BTN_RIGHT) == LOW) angleX += STEP;
  if (digitalRead(BTN_UP)    == LOW) angleY += STEP;
  if (digitalRead(BTN_DOWN)  == LOW) angleY -= STEP;

  angleX = constrain(angleX, SERVO_MIN, SERVO_MAX);
  angleY = constrain(angleY, SERVO_MIN, SERVO_MAX);

  servoX.write(angleX);
  servoY.write(angleY);

  delay(LOOP_DELAY);
}