/**
  NAME: Four-Category Segregation System (Sensor Module)
  FUNCTION: Classifies the garbage based on four gategories (Plastic, Metal, Paper/Dry, Wet)

  @software: & @hardware: Niño, Dominik O.
  @version: 1 March/08/2024
  @paragm: For more details contact:
  @paragm: LinknedIn: linkedin.com/in/dominik-niño
  @paragm: Github: github.com/EngrDomz
*/

// Include neccesary libraries
#include <Wire.h>               // Wire library
#include <Servo.h>              // Library to control servo motors
#include <LiquidCrystal_I2C.h>  // Library to control the LCD display

// Store I/O pins to a constant variable
#define IR_PIN 2
#define SERVO_SENSOR_PIN 3
#define INDUCTIVE_SENSOR_PIN 8
#define CAPACITIVE_SENSOR_PIN 9
#define MOISTURE_SENSOR_A0 A0
#define SERVO_1_PIN A1
#define SERVO_2_PIN A2
#define SERVO_3_PIN A3

#define MOTOR_ACTIVE_PIN 10
#define MOTOR_STOPPED 11

#define PAPER_FULL_MASTER_PIN 7
#define PLASTIC_FULL_MASTER_PIN 6
#define METAL_FULL_MASTER_PIN 5
#define WET_FULL_MASTER_PIN 4

#define SERVO_OPEN_STATE_DURATION 500  // This is the delay for how long the back plate will remain open when trash is detected in milliseconds
#define SERVO_SPEED 3                  // This is the speed of how fast the servo motor reach open position to close position
#define SERVO_OPEN_ANGLE 60            // This is the angle of the servo in conveyor on its UP state
#define SERVO_CLOSE_ANGLE 153          // This is the angle of the servo in conveyor on its DOWN state
#define MOISTURE_THRESHOLD 1021        // This is the threshold used in moisture sensor, below this threshold will be considered wet

#define SENSOR_SERVO_OPEN 170  // This is the angle of the servo in conveyor on its UP state
#define SENSOR_SERVO_CLOSE 65  // This is the angle of the servo in conveyor on its DOWN state

bool inductive;       // Variable for inductive reading (false = Metal detected)
bool capacitive;      // Variable for capacitive reading (true = Plastic detected)
bool wet = false;     // Set soil moisture sensor reading weather dry of wet
bool objectDetected;  // Variable for IR Sensor reading (false = Object detected)
int moisture;         // Variable for Moisture sensor analog reading

// This variables stores the reading from the SMS Arduino if either of the bins are full
bool paperBinFull;
bool plasticBinFull;
bool metalBinFull;
bool wetBinFull;

//This variable stores the motor state: if true; the motor is on stop state : if false; the motor is on active state
bool motorStop = true;

Servo SensorServo;   // Set variable name to Servo motor for Sensor intake
Servo PaperServo;    // Set variable name to Servo motor for Paper objects
Servo PlasticServo;  // Set variable name to Srvor motor for Plastic Objects
Servo MetalServo;    // Set variable name to Servo motor for Metal objects

LiquidCrystal_I2C lcd(0x27, 16, 2);  // Set variable name to the LCD Display

void setup() {
  Serial.begin(9600);            // Initialize serial communication
  lcd.begin();                   // Initialize LCD
  lcd.backlight();               // Activate backlight of the LCD
  LCD("MAGANDANG BUHAY!", " ");  // Call the LCD function to display a message on the program startup
  delay(5000);
  LCD("FOUR - CATEGORY", "   SEGREGATOR");
  delay(5000);
  LCD(" SETTING UP...", " ");
  delay(3000);

  // Set I/O ping for INPUT or OUTPUT
  pinMode(IR_PIN, INPUT);
  pinMode(SERVO_SENSOR_PIN, OUTPUT);
  pinMode(SERVO_1_PIN, OUTPUT);
  pinMode(SERVO_2_PIN, OUTPUT);
  pinMode(SERVO_3_PIN, OUTPUT);
  pinMode(MOISTURE_SENSOR_A0, INPUT);
  pinMode(INDUCTIVE_SENSOR_PIN, INPUT);
  pinMode(CAPACITIVE_SENSOR_PIN, INPUT);
  pinMode(MOTOR_ACTIVE_PIN, OUTPUT);
  pinMode(MOTOR_STOPPED, INPUT);

  pinMode(PAPER_FULL_MASTER_PIN, INPUT);
  pinMode(PLASTIC_FULL_MASTER_PIN, INPUT);
  pinMode(METAL_FULL_MASTER_PIN, INPUT);
  pinMode(WET_FULL_MASTER_PIN, INPUT);

  // Assign variable name of Servo motor to designated servo pins
  SensorServo.attach(SERVO_SENSOR_PIN);
  PaperServo.attach(SERVO_1_PIN);
  MetalServo.attach(SERVO_3_PIN);
  PlasticServo.attach(SERVO_2_PIN);

  //Set default state of servo motors
  SensorServo.write(SENSOR_SERVO_CLOSE);
  PaperServo.write(SERVO_OPEN_ANGLE);
  MetalServo.write(SERVO_OPEN_ANGLE);
  PlasticServo.write(SERVO_OPEN_ANGLE);

  motorStop = true;  // Sets the initial state of the motor to stop
}

void loop() {
  // Call the waste detection function
  wasteDetection();
}

// This is the LCD function that has two parameters which can be user to display the message on the upper and lower part of the LCD respectively
void LCD(const char* Message, const char* Message2) {
  lcd.clear();          // This method clears the LCD Display
  lcd.print(Message);   // This method display the first parameter to the upper part of the LCD
  lcd.setCursor(0, 1);  // Thid method sets the cursor to the lower part of the LCD
  lcd.print(Message2);  // This method display the first parameter to the lower part of the LCD
}

// This function is used to set the position of three servo motors on the conveyor. It has three parameters for the servo1, servo2, servo3 respectively
void WiperServo(int Servo1, int Servo2, int Servo3) {
  PlasticServo.write(Servo1);
  MetalServo.write(Servo2);
  PaperServo.write(Servo3);
}

//This function is used to Open the back plate of the sensor module
void OpenServo() {
  for (int i = SENSOR_SERVO_CLOSE; i < SENSOR_SERVO_OPEN; i++) {
    SensorServo.write(i);
    delay(SERVO_SPEED);
  }
  delay(SERVO_OPEN_STATE_DURATION);
  for (int i = SENSOR_SERVO_OPEN; i > SENSOR_SERVO_CLOSE; i--) {
    SensorServo.write(i);
    delay(SERVO_SPEED);
  }
}

// This function is used to detect waste inserted on the sensor module
void wasteDetection() {
  // Read sensors
  inductive = digitalRead(INDUCTIVE_SENSOR_PIN);
  objectDetected = digitalRead(IR_PIN);
  capacitive = digitalRead(CAPACITIVE_SENSOR_PIN);

  // Verify the sensor readings
  bool inductive2 = digitalRead(INDUCTIVE_SENSOR_PIN);
  bool objectDetected2 = digitalRead(IR_PIN);
  bool capacitive2 = digitalRead(CAPACITIVE_SENSOR_PIN);

  // Read the signal from the SMS Arduino if either of the bins are full
  paperBinFull = digitalRead(PAPER_FULL_MASTER_PIN);
  plasticBinFull = digitalRead(PLASTIC_FULL_MASTER_PIN);
  metalBinFull = digitalRead(METAL_FULL_MASTER_PIN);
  wetBinFull = digitalRead(WET_FULL_MASTER_PIN);

  // Reads the signal from the Conveyor Arduino for the motor state
  motorStop = digitalRead(MOTOR_STOPPED);
  //Reads the moisture content of the trast inserted
  moisture = analogRead(MOISTURE_SENSOR_A0);

  // This condition is used to analyze the moisture content of the trash inserted, if it is lower that the MOISTURE_THRESHOLD then it is wet
  if (moisture < MOISTURE_THRESHOLD) {
    wet = true;
  } else {
    wet = false;
  }

  // This function is used to verify if the initial detection is same as the second detection. It is used to give delay on inserting a trash so that is will not suddenly detect randomly
  if ((inductive == inductive2) && (objectDetected == objectDetected2) && (capacitive == capacitive2)) {

    // This condition will analyze if trash is inserted to the sensor module and will display analyzing of trash
    if (motorStop && (!objectDetected || !capacitive) && (!paperBinFull || !plasticBinFull || !metalBinFull || !wetBinFull)) {
      LCD("TRASH DETECTED!", "ANALYZING...");
    }

    delay(2000);
    lcd.begin();  // This will refresh the LCD to avoid random characters

    // This are the conditions of the sensors. This part of the code is where the decision is made
    if (motorStop && !capacitive && !inductive && !wet && !plasticBinFull) {
      Serial.println(F("Plastic Detected!"));                             // This will display the detection on serial monitor
      LCD("PLASTIC DETECTED", "");                                        // This will call the LCD function to diplay detected waste on LCD
      WiperServo(SERVO_CLOSE_ANGLE, SERVO_OPEN_ANGLE, SERVO_OPEN_ANGLE);  // This will call the wiper servo function to control wiper positions on conveyor
      digitalWrite(MOTOR_ACTIVE_PIN, HIGH);                               // This will give signal to the conveyor Arduino to activate motor
      OpenServo();                                                        // This will open the back plate of the sensor module

    } else if (plasticBinFull && motorStop && !capacitive && !inductive && !wet) {  // This condition will stop the machine from accommodating trash when the corresponding bin is full
      LCD("  PLASTIC BIN", "    IS FULL!");                                         // This will call the LCD function to diplay on LCD that the corresponding bin is full
      delay(3000);

    } else if (motorStop && inductive && !metalBinFull) {
      Serial.println("Metal Detected!");                                  // This will display the detection on serial monitor
      LCD("METAL DETECTED!", "");                                         // This will call the LCD function to diplay detected waste on LCD
      WiperServo(SERVO_OPEN_ANGLE, SERVO_CLOSE_ANGLE, SERVO_OPEN_ANGLE);  // This will call the wiper servo function to control wiper positions on conveyor
      digitalWrite(MOTOR_ACTIVE_PIN, HIGH);                               // This will give signal to the conveyor Arduino to activate motor
      OpenServo();                                                        // This will open the back plate of the sensor module

    } else if (metalBinFull && motorStop && inductive) {  // This condition will stop the machine from accommodating trash when the corresponding bin is full
      LCD("   METAL BIN", "    IS FULL!");                // This will call the LCD function to diplay on LCD that the corresponding bin is full
      delay(3000);

    } else if (motorStop && !inductive && wet && !objectDetected && !wetBinFull) {
      Serial.println("Wet Detected!");                                   // This will display the detection on serial monitor
      LCD(" WET DETECTED!", "");                                         // This will call the LCD function to diplay detected waste on LCD
      WiperServo(SERVO_OPEN_ANGLE, SERVO_OPEN_ANGLE, SERVO_OPEN_ANGLE);  // This will call the wiper servo function to control wiper positions on conveyor
      digitalWrite(MOTOR_ACTIVE_PIN, HIGH);                              // This will give signal to the conveyor Arduino to activate motor
      OpenServo();                                                       // This will open the back plate of the sensor module

    } else if (wetBinFull && motorStop !inductive && wet && !objectDetected) {  // This condition will stop the machine from accommodating trash when the corresponding bin is full
      LCD("    WET BIN", "    IS FULL!");                                       // This will call the LCD function to diplay on LCD that the corresponding bin is full
      delay(3000);

    } else if (motorStop && capacitive && !inductive && !wet && !objectDetected && !paperBinFull) {
      Serial.println("Paper Detected!");                                  // This will display the detection on serial monitor
      LCD(" PAPER DETECTED!", "");                                        // This will call the LCD function to diplay detected waste on LCD
      WiperServo(SERVO_OPEN_ANGLE, SERVO_OPEN_ANGLE, SERVO_CLOSE_ANGLE);  // This will call the wiper servo function to control wiper positions on conveyor
      digitalWrite(MOTOR_ACTIVE_PIN, HIGH);                               // This will give signal to the conveyor Arduino to activate motor
      OpenServo();                                                        // This will open the back plate of the sensor module

    } else if (paperBinFull && motorStop && capacitive && !inductive && !wet && !objectDetected) {  // This condition will stop the machine from accommodating trash when the corresponding bin is full
      LCD("   PAPER BIN", "    IS FULL!");                                                          // This will call the LCD function to diplay on LCD that the corresponding bin is full
      delay(3000);

    } else {                                        // If all off the condition above is not satisfied
      digitalWrite(MOTOR_ACTIVE_PIN, LOW);          // This will give signal to the conveyor Arduino to remove motor activation
      if (digitalRead(MOTOR_STOPPED) == 1) {        // If the motor is in stop state
        Serial.println("Insert Trash!");            // This will display the detection on serial monitor
        LCD("MACHINE IS READY", " INSERT TRASH!");  // This will display to the LCD that the machine is ready to accommodate trash
      } else {
        LCD("PROCESS ON GOING", "  PLEASE WAIT!");  // If the motor is still on Active state the LCD will display process on going
      }
    }
  }
}
