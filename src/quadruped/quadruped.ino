#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "controller_page.h"

const char *ssid = "Quadruped";
const char *password = "robot1234";
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// Queue complete states so command handling runs on loop(), not the network task.
QueueHandle_t commandQueue;
const size_t COMMAND_SIZE = 24;

enum class RobotCommand : uint8_t
{
  Stop,
  Forward,
  Backward,
  Left,
  Right,
  ForwardLeft,
  ForwardRight,
  BackwardLeft,
  BackwardRight
};

struct CommandMapping
{
  const char *text;
  RobotCommand command;
};

const CommandMapping commandMappings[] = {
    {"STOP", RobotCommand::Stop},
    {"FORWARD", RobotCommand::Forward},
    {"BACKWARD", RobotCommand::Backward},
    {"LEFT", RobotCommand::Left},
    {"RIGHT", RobotCommand::Right},
    {"FORWARD_LEFT", RobotCommand::ForwardLeft},
    {"FORWARD_RIGHT", RobotCommand::ForwardRight},
    {"BACKWARD_LEFT", RobotCommand::BackwardLeft},
    {"BACKWARD_RIGHT", RobotCommand::BackwardRight},
};

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

enum SideDirection
{
  NEUTRAL,
  LEFT,
  RIGHT
};

enum Range
{
  FULL,
  HALF
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
const int HALF_DELTA = DELTA_ANGLE / 2;

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x41);

RobotCommand currentCommand = RobotCommand::Stop;
RobotCommand activeCommand = RobotCommand::Stop;
const uint32_t GAIT_PHASE_MS = 100;
uint32_t phaseStartedAt = 0;
uint8_t gaitPhase = 0;
bool gaitRunning = false;

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

void moveServoDelta(ServoId id, bool up, int delta)
{

  int direction = servos[id].direction;

  if (!up)
  {
    direction *= -1;
  }

  setServo(id, servos[id].neutralAngle + direction * delta);
}

int rangeToDelta(Range range)
{
  int delta;

  switch (range)
  {
  case FULL:
    delta = DELTA_ANGLE;
    break;
  case HALF:
    delta = HALF_DELTA;
    break;
  }

  return delta;
}

void moveLimb(struct Limb &limb, bool forward, Range range)
{
  moveServoDelta(limb.hip, forward, rangeToDelta(range));
  moveServoDelta(limb.knee, true, DELTA_ANGLE); // Lift the foot in either travel direction.
}

void putLimb(struct Limb &limb)
{
  setServoNeutral(limb.knee);
}

void twistLimb(struct Limb &limb, bool forward)
{
  moveServoDelta(limb.hip, forward, DELTA_ANGLE);
}

// Execute one phase only; updateGait() supplies the timing.
void move(bool forward, SideDirection dir, uint8_t phase)
{
  const Range leftRange = dir == LEFT ? HALF : FULL;
  const Range rightRange = dir == RIGHT ? HALF : FULL;
  switch (phase)
  {
  case 0:
    moveLimb(leftAnterior, forward, leftRange);
    moveLimb(rightPosterior, forward, rightRange);
    twistLimb(rightAnterior, !forward);
    twistLimb(leftPosterior, !forward);
    break;
  case 1:
    putLimb(leftAnterior);
    putLimb(rightPosterior);
    break;
  case 2:
    moveLimb(rightAnterior, forward, rightRange);
    moveLimb(leftPosterior, forward, leftRange);
    twistLimb(leftAnterior, !forward);
    twistLimb(rightPosterior, !forward);
    break;
  case 3:
    putLimb(rightAnterior);
    putLimb(leftPosterior);
    break;
  }
}

void turn(bool right, uint8_t phase)
{
  switch (phase)
  {
  case 0:
    moveLimb(leftAnterior, right, FULL);
    moveLimb(rightPosterior, !right, FULL);
    twistLimb(rightAnterior, right);
    twistLimb(leftPosterior, !right);
    break;
  case 1:
    putLimb(leftAnterior);
    putLimb(rightPosterior);
    break;
  case 2:
    moveLimb(rightAnterior, !right, FULL);
    moveLimb(leftPosterior, right, FULL);
    twistLimb(leftAnterior, !right);
    twistLimb(rightPosterior, right);
    break;
  case 3:
    putLimb(rightAnterior);
    putLimb(leftPosterior);
    break;
  }
}

bool parseCommand(const String &text, RobotCommand &result)
{
  for (const CommandMapping &mapping : commandMappings)
  {
    if (text.length() == strlen(mapping.text) &&
        memcmp(text.c_str(), mapping.text, text.length()) == 0)
    {
      result = mapping.command;
      return true;
    }
  }
  return false;
}

const char *commandToText(RobotCommand command)
{
  for (const CommandMapping &mapping : commandMappings)
  {
    if (mapping.command == command)
      return mapping.text;
  }
  return "UNKNOWN";
}

void runGaitPhase(RobotCommand command, uint8_t phase)
{
  // Each command represents the complete selected direction state.

  switch (command)
  {
  case RobotCommand::Stop:

    break;

  case RobotCommand::Forward:

    move(true, NEUTRAL, phase);
    break;

  case RobotCommand::Backward:

    move(false, NEUTRAL, phase);
    break;

  case RobotCommand::Left:

    turn(false, phase);
    break;

  case RobotCommand::Right:

    turn(true, phase);
    break;

  case RobotCommand::ForwardLeft:

    move(true, LEFT, phase);
    break;

  case RobotCommand::ForwardRight:

    move(true, RIGHT, phase);
    break;

  case RobotCommand::BackwardLeft:

    move(false, LEFT, phase);
    break;

  case RobotCommand::BackwardRight:

    move(false, RIGHT, phase);
    break;
  }
}

void handleRobotCommand(RobotCommand command)
{
  currentCommand = command;
  if (command == RobotCommand::Stop)
  {
    gaitRunning = false;
    activeCommand = RobotCommand::Stop;
    gaitPhase = 0;
    setNeutral();
  }
}

void updateGait()
{
  if (currentCommand == RobotCommand::Stop) return;

  const uint32_t now = millis();
  if (!gaitRunning)
  {
    gaitRunning = true;
    gaitPhase = 0;
    activeCommand = currentCommand;
  }
  else
  {
    // Unsigned subtraction remains valid when millis() wraps around.
    if (static_cast<uint32_t>(now - phaseStartedAt) < GAIT_PHASE_MS) return;
    gaitPhase = (gaitPhase + 1) % 4;
    // Finish the old cycle before applying a new movement direction.
    if (gaitPhase == 0) activeCommand = currentCommand;
  }

  runGaitPhase(activeCommand, gaitPhase);
  // Wait a full interval after servo writes; never rush through overdue phases.
  phaseStartedAt = millis();
}

void queueRobotCommand(RobotCommand command)
{
  xQueueOverwrite(commandQueue, &command);
  ws.textAll(commandToText(command));
}

void onWebSocketEvent(AsyncWebSocket *socket, AsyncWebSocketClient *client,
                      AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  if (type == WS_EVT_CONNECT || type == WS_EVT_DISCONNECT)
  {
    // New/reconnected controllers start with all directions cleared.
    queueRobotCommand(RobotCommand::Stop);
    return;
  }
  if (type != WS_EVT_DATA)
    return;

  AwsFrameInfo *info = static_cast<AwsFrameInfo *>(arg);
  if (!info->final || info->index != 0 || info->len != len ||
      info->opcode != WS_TEXT || len == 0 || len >= COMMAND_SIZE)
    return;

  RobotCommand command;
  // WebSocket data is length-delimited, not necessarily null-terminated.
  const String text(reinterpret_cast<const char *>(data), len);
  if (parseCommand(text, command))
  {
    queueRobotCommand(command);
  }
}

void setupWifiController()
{
  commandQueue = xQueueCreate(1, sizeof(RobotCommand));
  if (commandQueue == nullptr)
  {
    Serial.println("Unable to create controller queue");
    return;
  }
  WiFi.mode(WIFI_AP);
  if (!WiFi.softAP(ssid, password))
  {
    Serial.println("Unable to start Wi-Fi access point");
    return;
  }
  ws.onEvent(onWebSocketEvent);
  server.addHandler(&ws);
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/html", index_html); });
  server.begin();
  Serial.print("Controller: http://");
  Serial.println(WiFi.softAPIP());
}

void setup()
{
  Serial.begin(115200);
  Wire.begin(); // on many boards this uses default SDA/SCL pins

  pwm.begin();
  pwm.setPWMFreq(PWM_FREQ);
  delay(10);

  setNeutral();
  setupWifiController();
}

void loop()
{
  ws.cleanupClients();
  RobotCommand command;
  const bool receivedCommand = commandQueue != nullptr &&
                               xQueueReceive(commandQueue, &command, 0) == pdTRUE;
  if (receivedCommand)
  {
    handleRobotCommand(command);
    Serial.print("Command: ");
    Serial.println(commandToText(currentCommand));
  }

  updateGait();

  delay(1);
}
