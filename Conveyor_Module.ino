/**
  NAME: Four-Category Segregation System (Conveyor Module)
  FUNCTION: Activates the conveyor module on the cue of the master Arduino for 30 seconds. Stops the motor when IR sensor detects object within 30 seconds

  @software: & @hardware: Niño, Dominik O.
  @version: 1 March/08/2024
  @paragm: For more details contact:
  @paragm: LinknedIn: linkedin.com/in/dominik-niño
  @paragm: Github: github.com/EngrDomz
*/

// Store I/O pins to a constant variable
#define MOTOR_SPEED 500  // Controls the speed of the stepper motor the lower value the faster it will runn (currently 500 is the lowest possible)
#define DIR_PIN 2
#define STEP_PIN 3
#define IR_1 8
#define IR_2 9
#define IR_3 10
#define IR_4 11
#define MOTOR_ACTIVE_PIN 12
#define MOTOR_STOPPED_PIN 13
#define STEPS_PER_REVOLUTION 200
#define MAX_MOTOR_RUNTIME 30000  // 30 seconds in milliseconds (Maximum time the motor will run: time starts when motor starts moving)

bool motorOn = false;
unsigned long motorStartTime = 0;  // Variable to track when the motor started

//boolean for Infrared Sensor reading
bool IR1;
bool IR2;
bool IR3;
bool IR4;

void setup() {
  Serial.begin(9600);

  // Declare pins as INPUT or OUTPUT
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(MOTOR_ACTIVE_PIN, INPUT);
  pinMode(MOTOR_STOPPED_PIN, OUTPUT);
  pinMode(IR_1, INPUT);
  pinMode(IR_2, INPUT);
  pinMode(IR_3, INPUT);
  pinMode(IR_4, INPUT);
  motorOn = false;  // Sets the initial state of motor to stop
  delay(15000);     // Sets the delay to 15 seconds to give time for the SMS module to set up
}

void loop() {
  CheckIR();                                           // This function checks if object has been dropped on either of the bins
  bool MasterCommand = digitalRead(MOTOR_ACTIVE_PIN);  // This is the command comming from the main arduino (Sensor Module) to trigger the motor to start
  if (MasterCommand) {                                 // If the main arduino gives high signal or true, the motor will be triggered and starts moving
    motorOn = true;
    motorStartTime = millis();  // Record the start time when motor is turned on
    CheckIR();                  // This function checks if object has been dropped on either of the bins
  }

  if (motorOn) {  // This condition will check if the motor is On if yes it will send signal to the main Arduino that the motor is moving
    CheckIR();    // This function checks if object has been dropped on either of the bins
    digitalWrite(MOTOR_STOPPED_PIN, LOW);
    digitalWrite(DIR_PIN, HIGH);

    for (int x = 0; x < STEPS_PER_REVOLUTION; x++) {
      CheckIR();
      digitalWrite(STEP_PIN, HIGH);
      delayMicroseconds(MOTOR_SPEED);
      digitalWrite(STEP_PIN, LOW);
      delayMicroseconds(MOTOR_SPEED);
      CheckIR();  // This function checks if object has been dropped on either of the bins
    }


    if (millis() - motorStartTime >= MAX_MOTOR_RUNTIME) {  // Check if motor has been running for more than one minute
      motorOn = false;                                     // Stop the motor if it has been running for more than one minute
    }
  } else {  // If the motorOn become false the motor will stop moving and it will send signal to the main arduino that the motor stops
    digitalWrite(MOTOR_STOPPED_PIN, HIGH);
  }
}

// This function checks if object has been dropped on either of the bins
void CheckIR() {
  IR1 = digitalRead(IR_1);
  IR2 = digitalRead(IR_2);
  IR3 = digitalRead(IR_3);
  IR4 = digitalRead(IR_4);

  if (!IR1 || !IR2 || !IR3 || !IR4) {  // If either of the infrared sensors detect an object, it will turn the motor off
    motorOn = false;
  }
}
