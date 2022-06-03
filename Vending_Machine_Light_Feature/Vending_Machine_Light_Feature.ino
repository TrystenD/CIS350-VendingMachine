
/*
 * Created by ArduinoGetStarted.com
 *
 * This example code is in the public domain
 *
 * Tutorial page: https://arduinogetstarted.com/tutorials/arduino-ultrasonic-sensor-led
 */

// constants won't change
const int TRIG_PIN =  6; // Arduino pin connected to Ultrasonic Sensor's TRIG pin
const int ECHO_PIN =  7; // Arduino pin connected to Ultrasonic Sensor's ECHO pin
const int LED_PIN_1  =  9; // Arduino pin connected to LED's pin 1
const int LED_PIN_2  = 10; // Arduino pin connected to LED's pin 2
const int LED_PIN_3  = 11; // Arduino pin connected to LED's pin 3
const int DISTANCE_THRESHOLD = 50; // centimeters
// variables will change:
float duration_us, distance_cm;

void setup() {
  Serial.begin (9600);       // initialize serial port
  pinMode(TRIG_PIN, OUTPUT); // set arduino pin to output mode
  pinMode(ECHO_PIN, INPUT);  // set arduino pin to input mode
  pinMode(LED_PIN_1, OUTPUT);  // set arduino pin to output mode
  pinMode(LED_PIN_2, OUTPUT);  // set arduino pin to output mode
  pinMode(LED_PIN_3, OUTPUT);  // set arduino pin to output mode
}

void loop() {
  // generate 10-microsecond pulse to TRIG pin
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // measure duration of pulse from ECHO pin
  duration_us = pulseIn(ECHO_PIN, HIGH);
  // calculate the distance
  distance_cm = 0.017 * duration_us;

  if(distance_cm < DISTANCE_THRESHOLD){
     digitalWrite(LED_PIN_1, HIGH); // turn on LED   
     digitalWrite(LED_PIN_2, HIGH); // turn on LED
     digitalWrite(LED_PIN_3, HIGH); // turn on LED
  }
  else{
     digitalWrite(LED_PIN_1, LOW); // turn off LED   
     digitalWrite(LED_PIN_2, LOW); // turn off LED
     digitalWrite(LED_PIN_3, LOW); // turn off LED
  }

  // print the value to Serial Monitor
  Serial.print("distance: ");
  Serial.print(distance_cm);
  Serial.println(" cm");

  delay(5000);
}
