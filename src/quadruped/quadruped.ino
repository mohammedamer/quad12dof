#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

enum ServoId {
  L11,
  L12,
  L13,
  L21,
  L22,
  L23,
  R11,
  R12,
  R13,
  R21,
  R22,
  R23,
  SERVO_COUNT
};

struct ServoConfig {
  const char* name;
  uint8_t channel;
  int8_t direction;
};

ServoConfig servos[SERVO_COUNT] = {
  { "L11", 0, 0 },
  { "L12", 1, 0 },
  { "L13", 2, 0 },
  { "L21", 3, 0 },
  { "L22", 4, 0 },
  { "L23", 5, 0 },
  { "R11", 6, 0 },
  { "R12", 7, 0 },
  { "R13", 8, 0 },
  { "R21", 9, 0 },
  { "R22", 10, 0 },
  { "R23", 11, 0 },
};

const int PWM_FREQ = 50;

const int SERVO_MIN = 110;
const int SERVO_MAX = 500;

const int NEUTRAL_ANGLE = 90;

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x41);

int angleToPulse(int angle) {
  angle = constrain(angle, 0, 180);
  return map(angle, 0, 180, SERVO_MIN, SERVO_MAX);
}

void setServoAngle(uint8_t channel, int angle) {
  int pulse = angleToPulse(angle);
  pwm.setPWM(channel, 0, pulse);
}

void setServo(ServoId id, int logicalAngle) {
  const ServoConfig& s = servos[id];
  int pulse = angleToPulse(logicalAngle);
  pwm.setPWM(s.channel, 0, pulse);
}

void setNeutral() {
  for (int i = 0; i < SERVO_COUNT; i++) {
    setServo((ServoId)i, NEUTRAL_ANGLE);
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin();  // on many boards this uses default SDA/SCL pins

  pwm.begin();
  pwm.setPWMFreq(PWM_FREQ);
}

void loop() {
  Serial.println("hello from esp32");
  delay(1000);

  setNeutral();
}
