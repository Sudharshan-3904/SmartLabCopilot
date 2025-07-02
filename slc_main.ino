#include <Wire.h>
#include <DHT.h>

// Defining input pin constants
#define insideSensorTrigger 2
#define insideSensorEcho 3
#define outsideSensorTrigger 6
#define outsideSensorEcho 5
#define dhtSensor 4
#define distanceTheshhold 10  //For Checkinf for the people

// Defining output pin constants
#define opRelay1 11
#define opRelay2 12
#define opLedPin1 8
#define opLedPin2 9
#define opLedPin2 9

// Defining Constant Macros
#define DHTTYPE DHT11
#define measurementDelay 1000
#define calibrationTime 50
#define pingDelay 100
#define iterationCounter 5
#define pause 1000                           // To not Cause a spike when powering on multiple components
#define sleepTimer 1
#define vibrationlimit 0.05

//Setting Global Constants
int peopleCount = 0;

//ADXL345 Constants
int ADXL345 = 0x53;                         // ADXL uses I2C Communication
float x_value, y_value, z_value;           // ADXL Value Constants
float p_x_value, p_y_value, p_z_value;

//Ultrasonic Sensor Constants
int insideSensorDuration = 0;
int insideSensorDistance = 0;
int insideSensorPastDistance = 0;

int outsideSensorDuration = 0;
int outsideSensorDistance = 0;
int outsideSensorPastDistance = 0;

int initialDistanceIn = 0;
int initialDistanceOut = 0;

// Initialising global Variables
DHT dht(dhtSensor, DHTTYPE);
bool morningRoutine = false;
bool labOff = false;
float preTemp;


// Single time iteration
void setup() {
  Serial.begin(9600);

  //ultrasonic sensor
  pinMode(insideSensorTrigger, OUTPUT);
  pinMode(insideSensorEcho, INPUT);
  pinMode(outsideSensorTrigger, OUTPUT);
  pinMode(outsideSensorEcho, INPUT);

  digitalWrite(insideSensorTrigger, HIGH);
  delayMicroseconds(pingDelay);
  digitalWrite(insideSensorTrigger, LOW);
  insideSensorDuration = pulseIn(insideSensorEcho, HIGH);
  initialDistanceIn = (insideSensorDuration / 2) / 29.1;

  digitalWrite(outsideSensorTrigger, HIGH);
  delayMicroseconds(pingDelay);
  digitalWrite(outsideSensorTrigger, LOW);
  outsideSensorDuration = pulseIn(outsideSensorEcho, HIGH);
  initialDistanceOut = (insideSensorDuration / 2) / 29.1;

  // DHT Setting up
  dht.begin();  // Starting DHT 11 Sensor
  preTemp = dht.readTemperature();

  //ADXL Calibration
  Wire.begin();
  Wire.beginTransmission(ADXL345);                  // Starting Communication with ADXL345
  Wire.write(0x2D);
  Wire.write(8);
  Wire.endTransmission();

  // Setting up all the output pins
  pinMode(opRelay1, OUTPUT);
  pinMode(opRelay2, OUTPUT);

  pinMode(opLedPin1, OUTPUT);
  pinMode(opLedPin2, OUTPUT);

  Serial.println();
  Serial.print("Calibrating sensors");
  for (int i = 0; i < calibrationTime; i++) {
    Serial.print(".");
    delay(100);
  }
  Serial.println(". Done");
  Serial.println("Starting Module..............................................");
  delay(50);
}


// Infinite Iteration
void loop() {
  Wire.beginTransmission(ADXL345);     // Start communicating with the device
  Wire.write(0x32);                    // Access Register - 0x32
  Wire.endTransmission(false);         // End transmission
  Wire.requestFrom(ADXL345, 6, true);  // Read 6 Registers Values

  p_x_value = getData();  // X-axis
  p_y_value = getData();  // Y-axis value
  p_z_value = getData();  // Z-axis value
  Wire.endTransmission();

  delay(measurementDelay);  // To not stress-out the systemrn;

  //ADXL Measurement
  Wire.beginTransmission(ADXL345);     // Start communicating with the device
  Wire.write(0x32);                    // Access Register - 0x32
  Wire.endTransmission(false);         // End transmission
  Wire.requestFrom(ADXL345, 6, true);  // Read 6 Registers Values

  x_value = getData();  // X-axis
  y_value = getData();  // Y-axis value
  z_value = getData();  // Z-axis value
  Wire.endTransmission();

  //Ultrasonic Sensor Readings
  // Ultrasonic Sensor 1
  digitalWrite(insideSensorTrigger, HIGH);
  delayMicroseconds(100);
  digitalWrite(insideSensorTrigger, LOW);
  insideSensorDuration = pulseIn(insideSensorEcho, HIGH);
  insideSensorDistance = (insideSensorDuration / 2) / 29.1;

  if ((insideSensorDistance < initialDistanceIn) && (insideSensorDistance < 20)) {
    peopleCount += 1;
  }

  // Serial.print("Distance from 1 : ");
  // Serial.println(insideSensorDistance);


  // Ultrasonic Sensor 2
  digitalWrite(outsideSensorTrigger, HIGH);
  delayMicroseconds(100);
  digitalWrite(outsideSensorTrigger, LOW);
  outsideSensorDuration = pulseIn(outsideSensorEcho, HIGH);
  outsideSensorDistance = (outsideSensorDuration / 2) / 29.1;

  if ((outsideSensorDistance < initialDistanceOut) && (outsideSensorDistance < 20)) {
    peopleCount -= 1;
  }

  // Serial.print("Distance from 2 : ");
  // Serial.println(outsideSensorDistance);


  // DHT 11 Measurements
  float temperature = dht.readTemperature();  // Temperature in celsius

  if (isnan(temperature)) {  // To see the dht reading failed
    Serial.println("DHT - Failed Reading");
    //return;
  }

  //To ensure Non-negative people Count
  if (peopleCount < 0) {
    peopleCount = 0;
    shutDown();
  } else if (peopleCount == 1 && !morningRoutine) {
    startUP();
  }

  // To See if temperature is high
  // i.e.,  Too many people increase the ambience temeprature of the room
  if (abs(preTemp - temperature) > 2.0) {
    digitalWrite(opRelay2, LOW);
  } else {
    digitalWrite(opRelay2, HIGH);
  }

  // TO see if any systems are on after people count reaches zero
  if (((abs(p_x_value - x_value) > vibrationlimit) || (abs(p_y_value - y_value) > vibrationlimit) || (abs(p_z_value - z_value) > vibrationlimit)) && labOff) {
    for (int i = 0; i < calibrationTime; i++) {
      digitalWrite(opLedPin1, HIGH);
      delay(pingDelay);
      digitalWrite(opLedPin1, LOW);
      delay(pingDelay);
    }
  }

  //Printing the values to Serial for confirmation
  Serial.print("People Count : ");
  Serial.print(peopleCount);

  Serial.print("    Prev :");
  Serial.print(preTemp);
  Serial.print(" 'C ");

  Serial.print("    Temp :");
  Serial.print(temperature);
  Serial.print(" 'C ");

  Serial.print("    X : ");
  Serial.print(x_value);
  Serial.print("    Y : ");
  Serial.print(y_value);
  Serial.print("    Z : ");
  Serial.println(z_value);
  Serial.println();
}


// Declaring the required functions
float getData() {  // Function to read ADXL data format
  float data;

  data = (Wire.read() | Wire.read() << 8);
  data = data / 256;
  return data;
}


void startUP() {  // Setting up a starting routine
  Serial.println("Good  Morning everyone! The lab is now powering on.........");
  delay(pause);

  digitalWrite(opLedPin1, LOW);
  digitalWrite(opLedPin2, HIGH);
  delay(pause);

  digitalWrite(opRelay1, LOW);
  digitalWrite(opRelay2, HIGH);
  delay(pause);
  Serial.println("The lab is powered on.");
  morningRoutine = true;
  labOff = false;
}


void shutDown() {  // Setting up a shut down routine
  Serial.println("Starting to ShutDown the lab..................");
  delay(pause);

  digitalWrite(opLedPin1, LOW);
  digitalWrite(opLedPin2, LOW);
  delay(pause);

  digitalWrite(opRelay1, HIGH);
  digitalWrite(opRelay2, HIGH);
  delay(pause);
  Serial.println("The lab is power off. GoodBye.");
  labOff = true;

  for (int i = 0; i < sleepTimer; i++) {  // Stimulating Lunch
    delay(measurementDelay);
  }

  morningRoutine = false;
}
