// https://wokwi.com/projects/399862282951045121

#include <ESP32Servo.h>
#include <HX711_ADC.h>

const int servoPin = 18;
Servo servo;

const int trigPin = 15;
const int echoPin = 14;

//define sound speed in cm/uS
#define SOUND_SPEED 0.034

long duration;
float distanceCm;
float distanceInch;

// HX711 connection pins
#define SCK  23  //SCK OUTPUT
#define DT   22  //DT INPUT

// General variables
float preSetCalibValue = 1.0;  /* set your calibration value here */ 
float weightInGramms;

// initialize
HX711_ADC LoadCell(DT, SCK);

void setup() {
  Serial.begin(115200);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  servo.attach(servoPin, 500, 2400);

  //initialize loadcell
  LoadCell.begin();
  LoadCell.start(2000, true);  //stabilize and tare on start

  delay(200);

  Serial.println("\nInitializing LoadCell...");
  if (LoadCell.getTareTimeoutFlag() || LoadCell.getSignalTimeoutFlag()) {
    Serial.println("\nTimeout, check wiring for MCU <> HX711");
    //while (1);
  } else {
    Serial.println("\nSetting CalFactor...");
    LoadCell.setCalFactor(preSetCalibValue);  // set calibration value
  }

  Serial.println("\n-- READY --");
}

void rotate_clockwise(int degrees) {
  for (int pos = 0; pos <= degrees; pos += 1) {
    servo.write(pos);
    delay(10);
  }
}

void rotate_anticlockwise(int degrees) {
  for (int pos = degrees; pos <= 0; pos -= 1) {
    servo.write(pos);
    delay(10);
  }
}

float get_weight() {
  static boolean newDataReady = false;
  static float newWeigh = 0.0;

  // Check for new data
  if (LoadCell.update()) {
    newDataReady = true;
  }

  // Get weight from the sensor
  if (newDataReady) {
    newWeigh = LoadCell.getData();
    if (abs(newWeigh) < 20.0) {  // kill small fluctuation
      newWeigh = 0.0;
    }

    newDataReady = false;
  }

  return newWeigh;
}

void loop() {
  // Clears the trigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);
  distanceCm = duration * SOUND_SPEED/2;

  // Update the current weight

  weightInGramms = get_weight();

  if(distanceCm < 15 and weightInGramms < 200) {
    Serial.print("Distance (cm): ");
    Serial.println(distanceCm);
    Serial.print("Weight (gramms): ");
    Serial.println(weightInGramms);

    rotate_clockwise(180);
    rotate_anticlockwise(180);
  }
  delay(1000);
}