#include <Wire.h>
#include <MPU6050.h>
#include <RTClib.h>

// ==========================
// PINS
// ==========================

#define MPU_SDA D4
#define MPU_SCL D5

#define RTC_SDA D6
#define RTC_SCL D7

// ==========================
// RTC BUS
// ==========================

TwoWire RTCBus = TwoWire(1);

// ==========================
// DEVICES
// ==========================

MPU6050 thigh(0x68);
MPU6050 shank(0x69);

RTC_DS3231 rtc;

// ==========================
// VARIABLES
// ==========================
float prevOmega = 0;
float prevPrevOmega = 0;

bool positivePeak = false;
bool negativePeak = false;

float thighAngle = 0;
float shankAngle = 0;
float kneeAngle = 0;

unsigned long previousTime = 0;
unsigned long lastEventTime = 0;

bool heelStrike = false;
bool toeOff = false;

String gaitPhase = "STANCE";

// Gait Parameters

float stepTime = 0;
float strideTime = 0;

float stepLength = 0;
float strideLength = 0;

float cadence = 0;

unsigned long lastHeelStrikeTime = 0;
unsigned long previousHeelStrikeTime = 0;

unsigned long lastToeOffTime = 0;

// Average walking speed (m/s)

float walkingSpeed = 1.2;

// ==========================
// THRESHOLDS
// ==========================


const int TIME_LOCK = 150;

// ==========================
// SETUP
// ==========================

void setup()
{
  Serial.begin(115200);

  Wire.begin(MPU_SDA, MPU_SCL);

  RTCBus.begin(RTC_SDA, RTC_SCL);

  thigh.initialize();
  shank.initialize();

  Serial.println();
  Serial.println("===== SYSTEM CHECK =====");

  Serial.print("Thigh MPU : ");
  Serial.println(thigh.testConnection() ? "OK" : "FAIL");

  Serial.print("Shank MPU : ");
  Serial.println(shank.testConnection() ? "OK" : "FAIL");

  if (!rtc.begin(&RTCBus))
  {
    Serial.println("RTC NOT FOUND");
    while (1);
  }

  Serial.println("RTC OK");

  if (rtc.lostPower())
  {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  previousTime = millis();

  Serial.println("SYSTEM READY");
}

// ==========================
// LOOP
// ==========================

void loop()
{
  int16_t ax1, ay1, az1;
  int16_t gx1, gy1, gz1;

  int16_t ax2, ay2, az2;
  int16_t gx2, gy2, gz2;

  thigh.getMotion6(
      &ax1, &ay1, &az1,
      &gx1, &gy1, &gz1);

  shank.getMotion6(
      &ax2, &ay2, &az2,
      &gx2, &gy2, &gz2);

  unsigned long currentTime = millis();

  float dt =
      (currentTime - previousTime) / 1000.0;

  previousTime = currentTime;

  // ==========================
  // THIGH ANGLE
  // ==========================

  float thighAccelAngle =
      atan2(
          ax1,
          sqrt((float)ay1 * ay1 +
               (float)az1 * az1))
      * 180.0 / PI;
  float thighGyroRate =
    gx1 / 131.0;
  static float shankOmegaFiltered = 0;

float shankOmegaRaw = gx2 / 131.0;

shankOmegaFiltered =
    0.8 * shankOmegaFiltered +
    0.2 * shankOmegaRaw;

float shankOmega = shankOmegaFiltered;

positivePeak = false;
negativePeak = false;

// Positive peak = Toe Off

if (prevOmega > prevPrevOmega &&
    prevOmega > shankOmega &&
    (prevOmega - shankOmega) > 5 &&
    (prevOmega - prevPrevOmega) > 5)
{
    positivePeak = true;
}

// Negative peak = Heel Strike

if (prevOmega < prevPrevOmega &&
    prevOmega < shankOmega &&
    (shankOmega - prevOmega) > 5 &&
    (prevPrevOmega - prevOmega) > 5)
{
    negativePeak = true;
}

  thighAngle =
      0.98 *
          (thighAngle +
           thighGyroRate * dt)
      +
      0.02 *
          thighAccelAngle;

  // ==========================
  // SHANK ANGLE
  // ==========================

  float shankAccelAngle =
      atan2(
          ax2,
          sqrt((float)ay2 * ay2 +
               (float)az2 * az2))
      * 180.0 / PI;

  float shankGyroRate =
      gx2 / 131.0;

  shankAngle =
      0.98 *
          (shankAngle +
           shankGyroRate * dt)
      +
      0.02 *
          shankAccelAngle;

  // ==========================
  // KNEE ANGLE
  // ==========================

  kneeAngle =
      shankAngle -
      thighAngle;

  // ==========================
  // OMEGA
  // ==========================


  float shankAccelZ =
      ((float)az2 / 16384.0) * 9.81;

  heelStrike = false;
  toeOff = false;

  // ==========================
  // HEEL STRIKE
  // ==========================

 if (positivePeak &&
    currentTime - lastEventTime > TIME_LOCK)
{
    toeOff = true;

    if (lastToeOffTime > 0)
    {
        stepTime =
            (currentTime -
             lastToeOffTime) / 1000.0;

        stepLength =
            walkingSpeed *
            stepTime;
    }

    lastToeOffTime = currentTime;
    lastEventTime = currentTime;
}

if (negativePeak &&
    currentTime - lastEventTime > TIME_LOCK)
{
    heelStrike = true;

    previousHeelStrikeTime =
        lastHeelStrikeTime;

    lastHeelStrikeTime =
        currentTime;

    if (previousHeelStrikeTime > 0)
    {
        strideTime =
            (lastHeelStrikeTime -
             previousHeelStrikeTime) / 1000.0;

        strideLength =
            walkingSpeed *
            strideTime;

        cadence =
            60.0 / strideTime;
    }

    lastEventTime = currentTime;
}

  // ==========================
  // GAIT PHASE
  // ==========================

  if (heelStrike)
  {
    gaitPhase = "HEEL_STRIKE";
  }
  else if (toeOff)
  {
    gaitPhase = "TOE_OFF";
  }
  else if (abs(shankOmega) > 10)
  {
    gaitPhase = "SWING";
  }
  else
  {
    gaitPhase = "STANCE";
  }

  // ==========================
  // RTC
  // ==========================

  DateTime now = rtc.now();

  // ==========================
  // OUTPUT
  // ==========================

  Serial.print(now.hour());
  Serial.print(":");
  Serial.print(now.minute());
  Serial.print(":");
  Serial.print(now.second());

  Serial.print(" | ");

  Serial.print("Thigh=");
  Serial.print(thighAngle, 1);

  Serial.print(" Shank=");
  Serial.print(shankAngle, 1);

  Serial.print(" Knee=");
  Serial.print(kneeAngle, 1);

  Serial.print(" Omega=");
  Serial.print(shankOmega, 1);

  Serial.print(" StepTime=");
  Serial.print(stepTime, 2);

  Serial.print("s");

  Serial.print(" StepLength=");
  Serial.print(stepLength, 2);

  Serial.print("m");

  Serial.print(" StrideTime=");
  Serial.print(strideTime, 2);

  Serial.print("s");

  Serial.print(" StrideLength=");
  Serial.print(strideLength, 2);

  Serial.print("m");

  Serial.print(" Cadence=");
  Serial.print(cadence, 1);

  Serial.print(" steps/min");

  Serial.print(" Phase=");
  Serial.println(gaitPhase);
  
  prevPrevOmega = prevOmega;
  prevOmega = shankOmega;

  int gaitMarker = 0;

if(heelStrike)
    gaitMarker = -100;

if(toeOff)
    gaitMarker = 100;

Serial.print(shankOmega);
Serial.print(",");

Serial.print(kneeAngle);
Serial.print(",");

Serial.println(gaitMarker);
  delay(20);
}
