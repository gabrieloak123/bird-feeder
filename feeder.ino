// https://wokwi.com/projects/399862282951045121

#include <ESP32Servo.h>
#include <HX711.h>

//#define SERVO 18
Servo servo;

#define TRIG 5
#define ECHO 19

//define sound speed in cm/uS
#define SOUND_SPEED 0.034

long duration;
float distanceCm;

// HX711 connection pins
#define SCK  5  //SCK OUTPUT
#define DT   18  //DT INPUT

// General variables
float weightInGramms;
const float scale_calib = 419.8;

HX711 scale;

void setup() {
  Serial.begin(115200);
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);

//  servo.attach(SERVO, 500, 2400);
  
  scale.begin(DT,SCK);
  delay(200);
  
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


void loop() {
  // Clears the trigPin
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);

  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(ECHO, HIGH);
  distanceCm = duration * SOUND_SPEED/2;

  // Update the current weight
  scale.set_scale();
  weightInGramms = scale.get_units(10)/scale_calib;

  Serial.print("Distance (cm): ");
  Serial.println(distanceCm);
  Serial.print("Weight (gramms): ");
  Serial.println(weightInGramms);

//  rotate_clockwise(180);
//  rotate_anticlockwise(180);
  
  delay(1000);
}
