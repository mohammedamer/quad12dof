#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

enum ServoId
{
  L11,
  L12,
  L13,
  R11,
  R12,
  R13,
  L21,
  L22,
  L23,
  R21,
  R22,
  R23,
  SERVO_COUNT
};

struct Limb
{
  ServoId hip;
  ServoId knee;
  ServoId leg;
};

struct Limb leftAnterior = {L11, L12, L13};
struct Limb rightAnterior = {R11, R12, R13};
struct Limb leftPosterior = {L21, L22, L23};
struct Limb rightPosterior = {R21, R22, R23};

struct ServoConfig
{
  const char *name;
  uint8_t channel;
  int8_t direction;
  int neutralAngle;
};

// +ve is forward/up

ServoConfig servos[SERVO_COUNT] = {
    {"L11", 0, -1, 90},
    {"L12", 1, 1, 120},
    {"L13", 2, 1, 90},
    {"R11", 3, 1, 80},
    {"R12", 4, -1, 60},
    {"R13", 5, -1, 95},
    {"L21", 6, -1, 60},
    {"L22", 7, -1, 60},
    {"L23", 8, -1, 125},
    {"R21", 9, 1, 105},
    {"R22", 10, 1, 120},
    {"R23", 11, 1, 85},
};

const int PWM_FREQ = 50;

const int SERVO_MIN_US = 500;
const int SERVO_MAX_US = 2500;

const int DELTA_ANGLE = 20;

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x41);

int angleToPulse(int angle)
{
  angle = constrain(angle, 0, 180);
  return map(angle, 0, 180, SERVO_MIN_US, SERVO_MAX_US);
}

uint16_t pulseUsToPwmTicks(int pulseUs)
{
  pulseUs = constrain(pulseUs, SERVO_MIN_US, SERVO_MAX_US);
  return (uint32_t)pulseUs * PWM_FREQ * 4096 / 1000000;
}

void setServo(ServoId id, int logicalAngle)
{
  const ServoConfig &s = servos[id];
  int pulse = angleToPulse(logicalAngle);
  pwm.setPWM(s.channel, 0, pulseUsToPwmTicks(pulse));
}

void setNeutral()
{
  for (int i = 0; i < SERVO_COUNT; i++)
  {
    setServo((ServoId)i, servos[i].neutralAngle);
  }
}

void setServoNeutral(ServoId id)
{
  setServo((ServoId)id, servos[id].neutralAngle);
}

void moveServoDelta(ServoId id, bool up)
{

  int direction = servos[id].direction;

  if (!up)
  {
    direction *= -1;
  }

  setServo(id, servos[id].neutralAngle + direction * DELTA_ANGLE);
}

void moveLimb(struct Limb &limb, bool forward)
{
  moveServoDelta(limb.hip, forward);
  moveServoDelta(limb.knee, true); // Lift the foot in either travel direction.
}

void putLimb(struct Limb &limb)
{
  setServoNeutral(limb.knee);
}

void twistLimb(struct Limb &limb, bool forward)
{
  moveServoDelta(limb.hip, forward);
}

void move(bool forward)
{
  moveLimb(leftAnterior, forward);
  moveLimb(rightPosterior, forward);
  twistLimb(rightAnterior, !forward);
  twistLimb(leftPosterior, !forward);

  delay(100);

  putLimb(leftAnterior);
  putLimb(rightPosterior);

  delay(100);

  moveLimb(rightAnterior, forward);
  moveLimb(leftPosterior, forward);
  twistLimb(leftAnterior, !forward);
  twistLimb(rightPosterior, !forward);

  delay(100);

  putLimb(rightAnterior);
  putLimb(leftPosterior);

  delay(100);
}

void turn(bool right)
{
  moveLimb(leftAnterior, right);
  moveLimb(rightPosterior, !right);
  twistLimb(rightAnterior, right);
  twistLimb(leftPosterior, !right);

  delay(100);

  putLimb(leftAnterior);
  putLimb(rightPosterior);

  delay(100);

  moveLimb(rightAnterior, !right);
  moveLimb(leftPosterior, right);
  twistLimb(leftAnterior, !right);
  twistLimb(rightPosterior, right);

  delay(100);

  putLimb(rightAnterior);
  putLimb(leftPosterior);

  delay(100);
}

void setup()
{
  Serial.begin(115200);
  Wire.begin(); // on many boards this uses default SDA/SCL pins

  pwm.begin();
  pwm.setPWMFreq(PWM_FREQ);
  delay(10);

  setNeutral();
}

void loop()
{

  turn(false);
  // move(true);
  // setNeutral();
  // delay(1000);
}
