#include <Servo.h>

// ============================================================
// TCS3200 COLOR SENSOR
// ============================================================
#define S0 11
#define S1 12
#define S2 13
#define S3 9
#define TCS_OUT 10

// ============================================================
// HC-SR04 OBJECT DETECTION
// ============================================================
#define TRIG_PIN 2
#define ECHO_PIN A0

const int OBJECT_DISTANCE_THRESHOLD_CM = 8;

// ============================================================
// TB6612FNG MOTOR DRIVER
// ============================================================
#define STBY 3
#define AIN1 4
#define AIN2 5
#define PWMA 6

// ============================================================
// SERVOS
// ============================================================
#define SERVO_RECYCLABLE_PIN 8
#define SERVO_NONRECYCLABLE_PIN 7

Servo recyclableFlap;
Servo nonRecyclableFlap;

// ============================================================
// SERVO POSITIONS
// ============================================================
const int FLAP_NEUTRAL         = 170;
const int FLAP_RECYCLE_OPEN    = 0;
const int FLAP_NONRECYCLE_OPEN = 0;

// ============================================================
// CONVEYOR
// ============================================================
const int CONVEYOR_SPEED = 160;

// ============================================================
// TIMING
// ============================================================
const unsigned long SETTLE_TIME_MS      = 1000;
const unsigned long TRANSIT_TIME_MS     = 1200;
const unsigned long FLAP_HOLD_MS        = 800;
const unsigned long IR_CLEAR_TIMEOUT_MS = 4000;

// ============================================================
// SENSOR CLASSIFICATION THRESHOLDS
// ============================================================
//
// Based on your actual readings:
//
// PLASTIC BOTTLE:
// R = 146
// G = 160
// B = 142
// Clear = 46
//
// PVC PIPE:
// R = 81
// G = 98
// B = 90
// Clear = 30
//
// Average RGB:
//
// Bottle = (146 + 160 + 142) / 3
//        = 149.3
//
// PVC = (81 + 98 + 90) / 3
//     = 89.7
//
// Classification:
//
// RGB Average > 120
// AND
// ClearRaw > 38
//
//              => RECYCLABLE
//
// Otherwise
//              => NON-RECYCLABLE
// ============================================================

const float RGB_AVERAGE_THRESHOLD = 120.0;
const long CLEAR_THRESHOLD = 38;

// ============================================================
// SETUP
// ============================================================
void setup() {

  // ---------------- TCS3200 ----------------

  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);
  pinMode(TCS_OUT, INPUT);

  // Frequency scaling
  digitalWrite(S0, HIGH);
  digitalWrite(S1, LOW);

  // ---------------- HC-SR04 ----------------

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  digitalWrite(TRIG_PIN, LOW);

  // ---------------- TB6612FNG ----------------

  pinMode(STBY, OUTPUT);
  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(PWMA, OUTPUT);

  digitalWrite(STBY, HIGH);

  // ---------------- SERVOS ----------------

  recyclableFlap.attach(SERVO_RECYCLABLE_PIN);
  nonRecyclableFlap.attach(SERVO_NONRECYCLABLE_PIN);

  recyclableFlap.write(FLAP_NEUTRAL);
  nonRecyclableFlap.write(FLAP_NEUTRAL);

  // ---------------- SERIAL ----------------

  Serial.begin(9600);

  Serial.println();
  Serial.println("==================================");
  Serial.println("     EcoSort Plastic Sorter");
  Serial.println("==================================");
  Serial.println("System Ready.");
  Serial.println();

  Serial.print("RGB Threshold: ");
  Serial.println(RGB_AVERAGE_THRESHOLD);

  Serial.print("Clear Threshold: ");
  Serial.println(CLEAR_THRESHOLD);

  Serial.println();

  startConveyor();
}

// ============================================================
// MAIN LOOP
// ============================================================
void loop() {

  // ----------------------------------------------------------
  // OBJECT DETECTION
  // ----------------------------------------------------------

  long distance = getDistanceCM();

  bool objectDetected =
    (distance > 0 &&
     distance <= OBJECT_DISTANCE_THRESHOLD_CM);

  if (objectDetected) {

    Serial.println();
    Serial.println("==================================");
    Serial.println("OBJECT DETECTED");
    Serial.println("==================================");

    // Give object time to reach sensing position
    delay(2000);

    // --------------------------------------------------------
    // STOP CONVEYOR
    // --------------------------------------------------------

    stopConveyor();

    Serial.println("Conveyor stopped.");

    // Allow object to settle
    delay(SETTLE_TIME_MS);

    // --------------------------------------------------------
    // READ SENSOR
    // --------------------------------------------------------

    int r;
    int g;
    int b;
    long clearRaw;

    readColorAndOpacity(r, g, b, clearRaw);

    // --------------------------------------------------------
    // DISPLAY SENSOR VALUES
    // --------------------------------------------------------

    Serial.println();
    Serial.println("--------- SENSOR READING ---------");

    Serial.print("R = ");
    Serial.println(r);

    Serial.print("G = ");
    Serial.println(g);

    Serial.print("B = ");
    Serial.println(b);

    Serial.print("ClearRaw = ");
    Serial.println(clearRaw);

    // Calculate RGB average
    float rgbAverage =
      (r + g + b) / 3.0;

    Serial.print("RGB Average = ");
    Serial.println(rgbAverage);

    Serial.println("----------------------------------");

    // --------------------------------------------------------
    // CLASSIFICATION
    // --------------------------------------------------------

    bool isRecyclable =
      classify(r, g, b, clearRaw);

    // --------------------------------------------------------
    // SERVO ACTION
    // --------------------------------------------------------

    if (isRecyclable) {

      // ======================================================
      // RECYCLABLE
      // ======================================================

      Serial.println();
      Serial.println("==================================");
      Serial.println("      RECYCLABLE PLASTIC");
      Serial.println("==================================");
      Serial.println("Plastic Bottle detected.");
      Serial.println("Opening RECYCLABLE flap.");

      recyclableFlap.write(FLAP_RECYCLE_OPEN);

      // Keep other flap closed
      nonRecyclableFlap.write(FLAP_NEUTRAL);

    }

    else {

      // ======================================================
      // NON-RECYCLABLE
      // ======================================================

      Serial.println();
      Serial.println("==================================");
      Serial.println("      NON-RECYCLABLE");
      Serial.println("==================================");
      Serial.println("PVC / opaque plastic detected.");
      Serial.println("Opening NON-RECYCLABLE flap.");

      nonRecyclableFlap.write(FLAP_NONRECYCLE_OPEN);

      // Keep other flap closed
      recyclableFlap.write(FLAP_NEUTRAL);
    }

    // --------------------------------------------------------
    // MOVE OBJECT TOWARDS SORTING FLAP
    // --------------------------------------------------------

    Serial.println("Conveyor restarted.");

    startConveyor();

    delay(TRANSIT_TIME_MS);

    // Keep flap open briefly
    delay(FLAP_HOLD_MS);

    // --------------------------------------------------------
    // RETURN BOTH FLAPS TO NEUTRAL
    // --------------------------------------------------------

    recyclableFlap.write(FLAP_NEUTRAL);
    nonRecyclableFlap.write(FLAP_NEUTRAL);

    Serial.println("Flaps returned to neutral.");

    // --------------------------------------------------------
    // WAIT UNTIL OBJECT LEAVES SENSOR AREA
    // --------------------------------------------------------

    unsigned long waitStart = millis();

    long clearDistance = getDistanceCM();

    while (
      (clearDistance > 0 &&
       clearDistance <= OBJECT_DISTANCE_THRESHOLD_CM) &&
      millis() - waitStart < IR_CLEAR_TIMEOUT_MS
    ) {

      delay(50);

      clearDistance = getDistanceCM();
    }

    Serial.println("Object cleared.");
    Serial.println();

  }
}

// ============================================================
// CLASSIFICATION FUNCTION
// ============================================================

bool classify(
  int r,
  int g,
  int b,
  long clearRaw
) {

  // Calculate average RGB
  float rgbAverage =
    (r + g + b) / 3.0;

  Serial.println();
  Serial.println("--------- CLASSIFICATION ---------");

  Serial.print("RGB Average: ");
  Serial.println(rgbAverage);

  Serial.print("Required RGB > ");
  Serial.println(RGB_AVERAGE_THRESHOLD);

  Serial.print("ClearRaw: ");
  Serial.println(clearRaw);

  Serial.print("Required Clear > ");
  Serial.println(CLEAR_THRESHOLD);

  // ----------------------------------------------------------
  // RECYCLABLE CONDITION
  // ----------------------------------------------------------
  //
  // BOTH conditions must be satisfied.
  //
  // RGB Average > 120
  // AND
  // ClearRaw > 38
  //
  // ----------------------------------------------------------

  if (
    rgbAverage > RGB_AVERAGE_THRESHOLD &&
    clearRaw > CLEAR_THRESHOLD
  ) {

    Serial.println();
    Serial.println("RESULT = RECYCLABLE");

    return true;
  }

  // ----------------------------------------------------------
  // NON-RECYCLABLE
  // ----------------------------------------------------------

  else {

    Serial.println();
    Serial.println("RESULT = NON-RECYCLABLE");

    return false;
  }
}

// ============================================================
// MOTOR CONTROL
// ============================================================

void startConveyor() {

  digitalWrite(STBY, HIGH);

  digitalWrite(AIN1, HIGH);
  digitalWrite(AIN2, LOW);

  analogWrite(PWMA, CONVEYOR_SPEED);
}

void stopConveyor() {

  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, LOW);

  analogWrite(PWMA, 0);
}

// ============================================================
// HC-SR04 DISTANCE
// ============================================================

long getDistanceCM() {

  digitalWrite(TRIG_PIN, LOW);

  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);

  delayMicroseconds(10);

  digitalWrite(TRIG_PIN, LOW);

  long duration =
    pulseIn(ECHO_PIN, HIGH, 30000UL);

  if (duration == 0) {
    return -1;
  }

  return duration * 0.034 / 2;
}

// ============================================================
// TCS3200 RAW COLOR READING
// ============================================================

int readPulse(
  int s2state,
  int s3state
) {

  digitalWrite(S2, s2state);
  digitalWrite(S3, s3state);

  int pulse =
    pulseIn(
      TCS_OUT,
      LOW,
      50000UL
    );

  return pulse;
}

// ============================================================
// READ RGB + CLEAR
// ============================================================

void readColorAndOpacity(
  int &rOut,
  int &gOut,
  int &bOut,
  long &clearRawOut
) {

  long rSum = 0;
  long gSum = 0;
  long bSum = 0;
  long clearSum = 0;

  const int samples = 5;

  // ----------------------------------------------------------
  // TAKE MULTIPLE READINGS
  // ----------------------------------------------------------

  for (int i = 0; i < samples; i++) {

    // RED
    rSum += readPulse(
      LOW,
      LOW
    );

    // GREEN
    gSum += readPulse(
      HIGH,
      HIGH
    );

    // BLUE
    bSum += readPulse(
      LOW,
      HIGH
    );

    // CLEAR
    clearSum += readPulse(
      HIGH,
      LOW
    );

    delay(30);
  }

  // ----------------------------------------------------------
  // CALCULATE AVERAGES
  // ----------------------------------------------------------

  rOut = rSum / samples;

  gOut = gSum / samples;

  bOut = bSum / samples;

  clearRawOut = clearSum / samples;
}