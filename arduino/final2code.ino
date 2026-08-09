
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
const int FLAP_NEUTRAL         = 180;
const int FLAP_RECYCLE_OPEN    = 0;
const int FLAP_NONRECYCLE_OPEN = 0;

// ============================================================
// CONVEYOR
// ============================================================
const int CONVEYOR_SPEED = 150;

// ============================================================
// TIMING
// ============================================================
const unsigned long SETTLE_TIME_MS      = 1000;
const unsigned long TRANSIT_TIME_MS     = 1200;
const unsigned long FLAP_HOLD_MS        = 800;
const unsigned long IR_CLEAR_TIMEOUT_MS = 4000;

// ============================================================
// SENSOR CLASSIFICATION
// ============================================================
//
// Based on your actual sensor readings:
//
// TRANSPARENT BOTTLE
// RGB Average ≈ 100
// ClearRaw ≈ 37-38
//
// PVC PIPE
// RGB Average ≈ 53-70
// ClearRaw ≈ 19-26
//
// BLACK WIRE COVER
// RGB Average ≈ 150
// ClearRaw ≈ 47
//
// Therefore:
//
// RGB Average >= 85
// AND
// ClearRaw >= 33
//
//       => RECYCLABLE
//
// Everything else
//       => NON-RECYCLABLE
//
// ============================================================

const float RGB_THRESHOLD = 85.0;
const long CLEAR_THRESHOLD = 33;

// ============================================================
// NUMBER OF SENSOR SAMPLES
// ============================================================

const int SENSOR_SAMPLES = 5;


// ============================================================
// SETUP
// ============================================================

void setup() {

  // ----------------------------------------------------------
  // TCS3200
  // ----------------------------------------------------------

  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);
  pinMode(TCS_OUT, INPUT);

  // Frequency scaling
  digitalWrite(S0, HIGH);
  digitalWrite(S1, LOW);


  // ----------------------------------------------------------
  // HC-SR04
  // ----------------------------------------------------------

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  digitalWrite(TRIG_PIN, LOW);


  // ----------------------------------------------------------
  // TB6612FNG
  // ----------------------------------------------------------

  pinMode(STBY, OUTPUT);
  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(PWMA, OUTPUT);

  digitalWrite(STBY, HIGH);


  // ----------------------------------------------------------
  // SERVOS
  // ----------------------------------------------------------

  recyclableFlap.attach(SERVO_RECYCLABLE_PIN);
  nonRecyclableFlap.attach(SERVO_NONRECYCLABLE_PIN);

  recyclableFlap.write(FLAP_NEUTRAL);
  nonRecyclableFlap.write(FLAP_NEUTRAL);


  // ----------------------------------------------------------
  // SERIAL
  // ----------------------------------------------------------

  Serial.begin(9600);

  Serial.println();
  Serial.println("==================================");
  Serial.println("     EcoSort Plastic Sorter");
  Serial.println("==================================");

  Serial.println("System Ready.");

  Serial.print("RGB Threshold = ");
  Serial.println(RGB_THRESHOLD);

  Serial.print("Clear Threshold = ");
  Serial.println(CLEAR_THRESHOLD);

  Serial.println();


  // Start conveyor
  startConveyor();
}


// ============================================================
// MAIN LOOP
// ============================================================

void loop() {

  // ----------------------------------------------------------
  // CHECK OBJECT
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


    // Give object time to reach sensor
    delay(1000);


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

    readColorAndOpacity(
      r,
      g,
      b,
      clearRaw
    );


    // --------------------------------------------------------
    // CALCULATE RGB AVERAGE
    // --------------------------------------------------------

    float rgbAverage =
      (r + g + b) / 3.0;


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

    Serial.print("RGB Average = ");
    Serial.println(rgbAverage);

    Serial.println("----------------------------------");


    // --------------------------------------------------------
    // CLASSIFY OBJECT
    // --------------------------------------------------------

    bool isRecyclable =
      classify(
        rgbAverage,
        clearRaw
      );


    // --------------------------------------------------------
    // CONTROL SERVO
    // --------------------------------------------------------

    if (isRecyclable) {

      // ======================================================
      // RECYCLABLE
      // ======================================================

      Serial.println();
      Serial.println("==================================");
      Serial.println("       RECYCLABLE");
      Serial.println("==================================");

      Serial.println("Opening recyclable flap.");

      // Make sure non-recyclable flap is closed
      nonRecyclableFlap.write(FLAP_NEUTRAL);

      // Open recyclable flap
      recyclableFlap.write(FLAP_RECYCLE_OPEN);

    }

    else {

      // ======================================================
      // NON-RECYCLABLE
      // ======================================================

      Serial.println();
      Serial.println("==================================");
      Serial.println("       NON-RECYCLABLE");
      Serial.println("==================================");

      Serial.println("Opening non-recyclable flap.");

      // Make sure recyclable flap is closed
      recyclableFlap.write(FLAP_NEUTRAL);

      // Open non-recyclable flap
      nonRecyclableFlap.write(FLAP_NONRECYCLE_OPEN);
    }


    // --------------------------------------------------------
    // START CONVEYOR
    // --------------------------------------------------------

    startConveyor();

    Serial.println("Conveyor restarted.");


    // Allow object to travel toward flap
    delay(TRANSIT_TIME_MS);


    // Keep selected flap open
    delay(FLAP_HOLD_MS);


    // --------------------------------------------------------
    // RESET BOTH FLAPS
    // --------------------------------------------------------

    recyclableFlap.write(FLAP_NEUTRAL);

    nonRecyclableFlap.write(FLAP_NEUTRAL);

    Serial.println("Flaps returned to neutral.");


    // --------------------------------------------------------
    // WAIT FOR OBJECT TO LEAVE SENSOR
    // --------------------------------------------------------

    unsigned long waitStart = millis();

    long clearDistance = getDistanceCM();


    while (
      (clearDistance > 0 &&
       clearDistance <= OBJECT_DISTANCE_THRESHOLD_CM)
      &&
      (millis() - waitStart < IR_CLEAR_TIMEOUT_MS)
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
bool classify(float rgbAverage, long clearRaw) {

  // ----------------------------------------------------------
  // CURRENT BOTTLE RANGE
  // ----------------------------------------------------------
  //
  // Bottle readings observed:
  //
  // RGB Average = 83.3 to 101.7
  // ClearRaw    = 31 to 38
  //
  // We give a small safety margin:
  //
  // RGB Average = 78 to 125
  // ClearRaw    = 29 to 43
  //
  // ----------------------------------------------------------

  const float BOTTLE_RGB_MIN = 78.0;
  const float BOTTLE_RGB_MAX = 125.0;

  const long BOTTLE_CLEAR_MIN = 29;
  const long BOTTLE_CLEAR_MAX = 43;


  Serial.println();
  Serial.println("--------- CLASSIFICATION ---------");

  Serial.print("RGB Average: ");
  Serial.println(rgbAverage);

  Serial.print("ClearRaw: ");
  Serial.println(clearRaw);


  // ----------------------------------------------------------
  // RECYCLABLE BOTTLE
  // ----------------------------------------------------------

  if (
    rgbAverage >= BOTTLE_RGB_MIN &&
    rgbAverage <= BOTTLE_RGB_MAX &&
    clearRaw >= BOTTLE_CLEAR_MIN &&
    clearRaw <= BOTTLE_CLEAR_MAX
  ) {

    Serial.println("RESULT: RECYCLABLE");

    return true;
  }


  // ----------------------------------------------------------
  // EVERYTHING OUTSIDE THE BOTTLE RANGE
  // ----------------------------------------------------------

  Serial.println("RESULT: NON-RECYCLABLE");

  return false;
}

// ============================================================
// MOTOR CONTROL
// ============================================================

void startConveyor() {

  digitalWrite(STBY, HIGH);

  digitalWrite(AIN1, HIGH);
  digitalWrite(AIN2, LOW);

  analogWrite(
    PWMA,
    CONVEYOR_SPEED
  );
}


void stopConveyor() {

  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, LOW);

  analogWrite(
    PWMA,
    0
  );
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
    pulseIn(
      ECHO_PIN,
      HIGH,
      30000UL
    );


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

  digitalWrite(
    S2,
    s2state
  );

  digitalWrite(
    S3,
    s3state
  );


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


  // ----------------------------------------------------------
  // TAKE MULTIPLE SAMPLES
  // ----------------------------------------------------------

  for (
    int i = 0;
    i < SENSOR_SAMPLES;
    i++
  ) {

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

  rOut =
    rSum / SENSOR_SAMPLES;

  gOut =
    gSum / SENSOR_SAMPLES;

  bOut =
    bSum / SENSOR_SAMPLES;

  clearRawOut =
    clearSum / SENSOR_SAMPLES;
}

