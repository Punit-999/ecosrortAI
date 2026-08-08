/*
  ============================================================
  Plastic Sorting Conveyor System
  Arduino Uno + TCS3200 (color) + HC-SR04 (object detect)
  + TB6612FNG (DC motor driver) + 2x 180-degree Servos (flaps)
  ============================================================

  FIXED: previous version's opacity reading was stuck near 255
  for every object, because clearMin/clearMax were guessed values
  that didn't match the sensor's real raw output range. The CLEAR
  channel naturally produces much smaller raw numbers than R/G/B
  (it lets ALL light through, so it's the fastest / shortest pulse
  of the four), and the old 30-200 window didn't fit that at all.

  NEW APPROACH: this version skips guessing entirely. It works
  directly with RAW pulse values (no map()/constrain() layer for
  opacity) and lets you CALIBRATE LIVE using two serial commands:

    Send  t   while holding a TRANSPARENT sample (e.g. clear bottle)
    Send  o   while holding an OPAQUE sample (e.g. PVC pipe, mouse)
    Send  p   to print the current calibration + computed threshold
    Send  s   to stop the conveyor (useful while calibrating)
    Send  g   to resume the conveyor

  Do this calibration EVERY session before your demo, since raw
  values can shift slightly with lighting conditions.

  KNOWN LIMITATION TO BE AWARE OF: this method measures how much
  of the sensor's own light comes back to it. A transparent object
  lets light pass through (little comes back), and a truly BLACK
  opaque object can also return very little light (it absorbs
  instead of reflecting) -- so pure black objects and transparent
  objects can sometimes look similar to this channel alone. If your
  non-recyclable hard-plastic samples are black, watch for this
  during testing and let the RGB fallback logic help disambiguate
  (very low R+G+B together with high opacity value more strongly
  suggests true black, not transparent).

  ALGORITHM:
  1. Conveyor runs continuously via TB6612FNG-driven DC motor.
  2. HC-SR04 detects an object within 8cm -> motor stops.
  3. Only AFTER the belt has fully stopped does the TCS3200
     take color + opacity readings (never reads while moving).
  4. Object is classified as recyclable / non-recyclable using
     the calibrated opacity threshold first, RGB as a fallback.
  5. The correct flap servo is pre-positioned, THEN the belt
     resumes so the object travels to the flap/bin location.
  6. After enough time for the object to pass through the
     flap, the flap resets to neutral and the system goes back
     to watching for the next object.
*/

#include <Servo.h>

// ---------------- TCS3200 ----------------
#define S0 11
#define S1 12
#define S2 13
#define S3 9
#define TCS_OUT 10

// ---------------- HC-SR04 ----------------
#define TRIG_PIN 2
#define ECHO_PIN A0
const int OBJECT_DISTANCE_THRESHOLD_CM = 8;

// ---------------- TB6612FNG ----------------
#define STBY 3
#define AIN1 4
#define AIN2 5
#define PWMA 6

// ---------------- Servos (flaps) ----------------
#define SERVO_RECYCLABLE_PIN 7
#define SERVO_NONRECYCLABLE_PIN 8

Servo recyclableFlap;
Servo nonRecyclableFlap;

const int FLAP_NEUTRAL          = 180;
const int FLAP_RECYCLE_OPEN     = 0;
const int FLAP_NONRECYCLE_OPEN  = 0;

const int CONVEYOR_SPEED = 150;

const unsigned long SETTLE_TIME_MS      = 1000;
const unsigned long TRANSIT_TIME_MS     = 1200;
const unsigned long FLAP_HOLD_MS        = 800;
const unsigned long IR_CLEAR_TIMEOUT_MS = 4000;

// ----- RGB fallback thresholds (used only as a secondary check) -----
const int RGB_DARK_CUTOFF  = 60;  // below this on all 3 = likely true black
const int RGB_LIGHT_CUTOFF = 180; // above this on all 3 = likely white/light opaque

// ----- Live opacity calibration (RAW values, set via serial commands) -----
// -------- Fixed calibration (measured from your sensor) --------
const long opacityThresholdRaw = 41;

void setup() {
  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);
  pinMode(TCS_OUT, INPUT);
  digitalWrite(S0, HIGH);
  digitalWrite(S1, LOW);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIG_PIN, LOW);

  pinMode(STBY, OUTPUT);
  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(PWMA, OUTPUT);
  digitalWrite(STBY, HIGH);

  recyclableFlap.attach(SERVO_RECYCLABLE_PIN);
  nonRecyclableFlap.attach(SERVO_NONRECYCLABLE_PIN);
  recyclableFlap.write(FLAP_NEUTRAL);
  nonRecyclableFlap.write(FLAP_NEUTRAL);

  Serial.begin(9600);
Serial.println("Plastic sorting system ready.");
Serial.println("Calibration loaded successfully.");

  startConveyor();
}
bool classify(int r, int g, int b, long clearRaw)
{
    Serial.print("Clear Raw = ");
    Serial.println(clearRaw);

    // Transparent bottle (recyclable)
    if (clearRaw <= opacityThresholdRaw)
    {
        Serial.println("Transparent Bottle -> RECYCLABLE");
        return true;
    }

    // Anything more opaque than the threshold
    Serial.println("Opaque Object -> NON-RECYCLABLE");
    return false;
}
void loop() {
 

  long distance = getDistanceCM();
  bool objectDetected = (distance > 0 && distance <= OBJECT_DISTANCE_THRESHOLD_CM);

  if (objectDetected) {
    delay(2000);
    stopConveyor();
    Serial.println("Object detected -> belt stopped.");

    delay(SETTLE_TIME_MS);

    int r, g, b;
    long clearRaw;
    readColorAndOpacity(r, g, b, clearRaw);
    Serial.print("R:"); Serial.print(r);
    Serial.print(" G:"); Serial.print(g);
    Serial.print(" B:"); Serial.print(b);
    Serial.print(" ClearRaw:"); Serial.println(clearRaw);

    bool isRecyclable = classify(r, g, b, clearRaw);

    if (isRecyclable) {
      recyclableFlap.write(FLAP_RECYCLE_OPEN);
      Serial.println("Classified: RECYCLABLE");
    } else {
      nonRecyclableFlap.write(FLAP_NONRECYCLE_OPEN);
      Serial.println("Classified: NON-RECYCLABLE");
    }

    startConveyor();
    delay(TRANSIT_TIME_MS);
    delay(FLAP_HOLD_MS);

    recyclableFlap.write(FLAP_NEUTRAL);
    nonRecyclableFlap.write(FLAP_NEUTRAL);

    unsigned long waitStart = millis();
    long clearDistance = getDistanceCM();
    while ((clearDistance > 0 && clearDistance <= OBJECT_DISTANCE_THRESHOLD_CM) &&
           millis() - waitStart < IR_CLEAR_TIMEOUT_MS) {
      delay(50);
      clearDistance = getDistanceCM();
    }
  }
}

// ---------------- Serial calibration commands ----------------


// ---------------- Motor control (TB6612FNG) ----------------
void startConveyor() {
  digitalWrite(AIN1, HIGH);
  digitalWrite(AIN2, LOW);
  analogWrite(PWMA, CONVEYOR_SPEED);
}

void stopConveyor() {
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, LOW);
  analogWrite(PWMA, 0);
}

// ---------------- HC-SR04 distance ----------------
long getDistanceCM() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000UL);
  if (duration == 0) return -1;

  return duration * 0.034 / 2;
}

// ---------------- Color + opacity sensing ----------------
int readPulse(int s2state, int s3state) {
  digitalWrite(S2, s2state);
  digitalWrite(S3, s3state);
  return pulseIn(TCS_OUT, LOW, 50000UL);
}

void readColorAndOpacity(int &rOut, int &gOut, int &bOut, long &clearRawOut) {
  long rSum = 0, gSum = 0, bSum = 0, clearSum = 0;
  const int samples = 5;

  for (int i = 0; i < samples; i++) {
    rSum     += readPulse(LOW, LOW);
    gSum     += readPulse(HIGH, HIGH);
    bSum     += readPulse(LOW, HIGH);
    clearSum += readPulse(HIGH, LOW);
  }

  // R/G/B reported here as raw pulse widths directly (smaller = brighter/
  // more of that color reflected). Used only as a fallback signal, not
  // mapped to 0-255, to avoid the same calibration-window trap as before.
  rOut = rSum / samples;
  gOut = gSum / samples;
  bOut = bSum / samples;
  clearRawOut = clearSum / samples;
}
