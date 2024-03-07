/**
  NAME: Four-Category Segregation System (SMS Module)
  FUNCTION: Monitors the garbage level on trash bins. Send message to registered users when either of the bins are full.

  @software: & @hardware: Niño, Dominik O.
  @version: 1 March/08/2024
  @paragm: For more details contact:
  @paragm: LinknedIn: linkedin.com/in/dominik-niño
  @paragm: Github: github.com/EngrDomz
*/

//Install Necessary Libraries
#include <GPRS_Shield_Arduino.h>  // for SIM800L Module
#include <EasyEEPROM.h>           // eeprom to store data
#include <SoftwareSerial.h>       // to delare pins as serial communication
#include <Wire.h>                 // wire library

// Store I/O pins to a constant variable
#define PAPER_TRIG_PIN 4
#define PAPER_ECHO_PIN 5
#define PLASTIC_TRIG_PIN 6
#define PLASTIC_ECHO_PIN 7
#define METAL_TRIG_PIN 8
#define METAL_ECHO_PIN 9
#define PIN_TX 10
#define PIN_RX 11
#define WET_TRIG_PIN 12
#define WET_ECHO_PIN 13

#define PAPER_READY_LED_PIN 2
#define PAPER_FULL_LED_PIN 3
#define PLASTIC_READY_LED_PIN 14
#define PLASTIC_FULL_LED_PIN 15
#define METAL_READY_LED_PIN 16
#define METAL_FULL_LED_PIN 17
#define WET_READY_LED_PIN 18
#define WET_FULL_LED_PIN 19

#define PAPER_FULL_SLAVE_PIN A0
#define PLASTIC_FULL_SLAVE_PIN A1
#define METAL_FULL_SLAVE_PIN A2
#define WET_FULL_SLAVE_PIN A3

#define CAPACITY_THRESHOLD 7  // Limit of trash can to know if it is full.
// The higher the value, the less capacity it can hold to trigger that it is already full.

// SMS variables start //--------------------------------------------------------------------------------------------------------------------------------------------------------------------//
#define BAUDRATE 9600
#define PHONE_NUMBER "+639*********"  // Change with the developer phone number
#define OPEN_MESSAGE "MACHINE IS READY!"
// SMS Feedback Messages
#define SUCCESS "REGISTRATION SUCCESS!"

#define MESSAGE_LENGTH 50                  // Sets the length of the message SIM800L can read
#define MESSAGE_INTERVAL 30 * 60 * 1000ul  // 30 mins in milliseconds
char MESSAGE[MESSAGE_LENGTH];              // Character array variable to store received messag
int messageIndex = 0;                      // Varialble to store the message index of received message
                                           // (-1 = error receiving message; 0 module is ready to receive; >=1 = message is received)
char phoneNumbers[5][14];                  // Character array variable to store admin and user numbers
char PHONE[14];                            // Character array variable to store number of the current sender
char DATETIME[24];                         // Character array variable to store the date and time of receiving a message
bool deleteUserMode = false;               // Condition to enter delete user mode. (If "true" it is in delete user mode)
bool addUserMode = false;                  // Condition to enter add user mode. (If "true" it is in add user mode)
// SMS variables ends //--------------------------------------------------------------------------------------------------------------------------------------------------------------------//

// boolean variables to know if either of the bins are full
bool metalBinFull = false;
bool plasticBinFull = false;
bool paperBinFull = false;
bool wetBinFull = false;

int Bin;                            // Variable assigned to each condition on message to be send to users whenever bins are full
int lastBin = 16;                   // Variable used to store the last message send and compares it.
                                    // If it is not the same as the last message sent, it will instaneously send another message, else it will send message every 30 mins if either of the bins is/are full
unsigned long lastMessageTime = 0;  // Varialble to store the time in millis when the last message sent

GPRS GSMTEST(PIN_TX, PIN_RX, BAUDRATE);  // Declares the RX,TX,BAUDRATE of the SIM Module

EasyEEPROM eeprom;  // Create a variable message to call the eeprom library

void setup() {
  Serial.begin(9600);  // Start Serial communication

  // If the SIM module didn't initialize properly whether poor signal or lost of physical connections it will display the error message to serial monitor
  while (!GSMTEST.init()) {
    delay(1000);
    Serial.print(F("SIM INITIATE ERROR\r\n"));
  }

  // Declares I/O pins wheather INPUT or OUTPUT
  pinMode(WET_ECHO_PIN, INPUT);
  pinMode(PAPER_ECHO_PIN, INPUT);
  pinMode(METAL_ECHO_PIN, INPUT);
  pinMode(PLASTIC_ECHO_PIN, INPUT);

  pinMode(WET_TRIG_PIN, OUTPUT);
  pinMode(PAPER_TRIG_PIN, OUTPUT);
  pinMode(METAL_TRIG_PIN, OUTPUT);
  pinMode(PLASTIC_TRIG_PIN, OUTPUT);

  pinMode(WET_FULL_LED_PIN, OUTPUT);
  pinMode(METAL_FULL_LED_PIN, OUTPUT);
  pinMode(PAPER_FULL_LED_PIN, OUTPUT);
  pinMode(PLASTIC_FULL_LED_PIN, OUTPUT);

  pinMode(WET_READY_LED_PIN, OUTPUT);
  pinMode(METAL_READY_LED_PIN, OUTPUT);
  pinMode(PAPER_READY_LED_PIN, OUTPUT);
  pinMode(PLASTIC_READY_LED_PIN, OUTPUT);

  pinMode(PAPER_FULL_SLAVE_PIN, OUTPUT);
  pinMode(PLASTIC_FULL_SLAVE_PIN, OUTPUT);
  pinMode(METAL_FULL_SLAVE_PIN, OUTPUT);
  pinMode(WET_FULL_SLAVE_PIN, OUTPUT);

  loadPhoneNumbers();  // Load phone numbers from eeprom to update numbers on phoneNumbers variable which is used to send message whenever bins are full

  // Sends a welcome message to the developer
  Serial.println(GSMTEST.sendSMS(PHONE_NUMBER, OPEN_MESSAGE));
  Serial.println(WELCOME);
}

void loop() {
  // Read the value of ultrasonic sensor for each bin to know its capacity
  float metalVal = measureDistance(METAL_TRIG_PIN, METAL_ECHO_PIN);
  float plasticVal = measureDistance(PLASTIC_TRIG_PIN, PLASTIC_ECHO_PIN);
  float paperVal = measureDistance(PAPER_TRIG_PIN, PAPER_ECHO_PIN);
  float wetVal = measureDistance(WET_TRIG_PIN, WET_ECHO_PIN);

  SMSReceive();  // Try to receive message if any

  // Serial.print(F("Paper Capacity: "));
  // Serial.print(paperVal);
  // Serial.print(F(" cm | "));
  // Serial.print(F("Plastic Capacity: "));
  // Serial.print(plasticVal);
  // Serial.print(F(" cm | "));
  // Serial.print(F("Metal Capacity: "));
  // Serial.print(metalVal);
  // Serial.print(F(" cm | "));
  // Serial.print(F("Wet Capacity: "));
  // Serial.print(wetVal);
  // Serial.println(F(" cm | "));

  // Checks if the any bin is full
  // comparing the bin capacity reading to the full capacity threshold
  if (paperVal <= CAPACITY_THRESHOLD) {  // IF capacity is less than the full capacity threshold RED LED on UI will turn ON and GREEN LED on UI will turn OFF
    LED_ON(PAPER_FULL_LED_PIN);
    LED_OFF(PAPER_READY_LED_PIN);
    paperBinFull = true;                      // Declare the paper bin is full
    fullBinPins(PAPER_FULL_SLAVE_PIN, HIGH);  // Sends message to the main arduino (Sensor Module) that paper bin is full
  } else {                                    // If capacity is greater that critical capacity threshold GREEN LED on UI will turn ON and RED LED on UI will turn OFF
    LED_ON(PAPER_READY_LED_PIN);
    LED_OFF(PAPER_FULL_LED_PIN);
    paperBinFull = false;                    // Declare the paper bin is ready
    fullBinPins(PAPER_FULL_SLAVE_PIN, LOW);  // Sends message to the main arduino (Sensor Module) that paper bin is ready to collect
  }

  if (plasticVal <= CAPACITY_THRESHOLD) {  // IF capacity is less than the full capacity threshold RED LED on UI will turn ON and GREEN LED on UI will turn OFF
    LED_ON(PLASTIC_FULL_LED_PIN);
    LED_OFF(PLASTIC_READY_LED_PIN);
    plasticBinFull = true;                      // Declare the plastic bin is full
    fullBinPins(PLASTIC_FULL_SLAVE_PIN, HIGH);  // Sends message to the main arduino (Sensor Module) that plastic bin is full
  } else {                                      // If capacity is greater that critical capacity threshold GREEN LED on UI will turn ON and RED LED on UI will turn OFF
    LED_ON(PLASTIC_READY_LED_PIN);
    LED_OFF(PLASTIC_FULL_LED_PIN);
    plasticBinFull = false;                    // Declare the plastic bin is ready
    fullBinPins(PLASTIC_FULL_SLAVE_PIN, LOW);  // Sends message to the main arduino (Sensor Module) that plastic bin is ready to collect
  }

  if (metalVal <= CAPACITY_THRESHOLD) {  // If capacity is less than the full capacity threshold RED LED on UI will turn ON and GREEN LED on UI will turn OFF
    LED_ON(METAL_FULL_LED_PIN);
    LED_OFF(METAL_READY_LED_PIN);
    metalBinFull = true;                      // Declare the metal bin is full
    fullBinPins(METAL_FULL_SLAVE_PIN, HIGH);  // Sends message to the main arduino (Sensor Module) that metal bin is full
  } else {                                    // If capacity is greater that critical capacity threshold GREEN LED on UI will turn ON and RED LED on UI will turn OFF
    LED_ON(METAL_READY_LED_PIN);
    LED_OFF(METAL_FULL_LED_PIN);
    metalBinFull = false;                    // Declare the metal bin is ready
    fullBinPins(METAL_FULL_SLAVE_PIN, LOW);  // Sends message to the main arduino (Sensor Module) that metal bin is ready to collect
  }

  if (wetVal <= CAPACITY_THRESHOLD) {  // If capacity is less than the full capacity threshold RED LED on UI will turn ON and GREEN LED on UI will turn OFF
    LED_ON(WET_FULL_LED_PIN);
    LED_OFF(WET_READY_LED_PIN);
    wetBinFull = true;                      // Declare the wet bin is full
    fullBinPins(WET_FULL_SLAVE_PIN, HIGH);  // Sends message to the main arduino (Sensor Module) that wet bin is full
  } else {                                  // If capacity is greater that critical capacity threshold GREEN LED on UI will turn ON and RED LED on UI will turn OFF
    LED_ON(WET_READY_LED_PIN);
    LED_OFF(WET_FULL_LED_PIN);
    wetBinFull = false;                    // Declare the wet bin is ready
    fullBinPins(WET_FULL_SLAVE_PIN, LOW);  // Sends message to the main arduino (Sensor Module) that wet bin is ready to collect
  }

  SMSReceive();  // Try to receive message again if any

  if (metalBinFull || plasticBinFull || paperBinFull || wetBinFull) {  // If either of the bin/s is/are full, if will call the checkAndSendSMS function to check if user are already registered and will notify users the bin is full
    checkAndSendSMS();
  }

  // Serial.print("ADMIN NUMBER: ");
  // Serial.println(phoneNumbers[0]);

  // Serial.print("USER NUMBER 1: ");
  // Serial.println(phoneNumbers[1]);

  // Serial.print("USER NUMBER 2: ");
  // Serial.println(phoneNumbers[2]);

  // Serial.print("USER NUMBER 3: ");
  // Serial.println(phoneNumbers[3]);

  // Serial.print("USER NUMBER 4: ");
  // Serial.println(phoneNumbers[4]);

  SMSReceive();  // Try to receive message again if any
}

void loadPhoneNumbers() {  // This function updates the variable phoneNumbers of the user numbers stored on eeprom
  for (int i = 0; i < 5; i++)
    eeprom.readPhoneNumber(phoneNumbers[i], i);
}

// This function give command to the ultrasonic sensor to read distance
float measureDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(2);
  digitalWrite(trigPin, LOW);

  float duration = pulseIn(echoPin, HIGH);
  float distance = duration * 0.034 / 2;  // conversion to centimeters
  return distance;
}

// This function is used to turn the LED ON using relay
// When calling this function, you have to include the LED PIN you want to turn ON inside the parenthesis of the function
void LED_ON(int ledpin) {
  digitalWrite(ledpin, LOW);
}

// This function is used to turn the LED OFF using relay
// When calling this function, you have to include the LED PIN you want to turn OFF inside the parenthesis of the function
void LED_OFF(int ledpin) {
  digitalWrite(ledpin, HIGH);
}

// This function is used to receive SMS
void SMSReceive() {
  messageIndex = GSMTEST.isSMSunread();  // This method stores the current message Index on messageIndex variable

  if (messageIndex > 0) {                                                     // This condition checks if message index is greater than 0 which means that message is reveived.
    GSMTEST.readSMS(messageIndex, MESSAGE, MESSAGE_LENGTH, PHONE, DATETIME);  // This method will read the received message.
    clearSMS();                                                               // This function is used to free up the inbox of SIM card to avoid memory full
    bool isAdmin = false;

    // This condition is used to check if the message sender is the admin
    if (phoneNumbers[0][0] == '+') {                  // This condition will check if the 1st number on phoneNumbers variable starts with "+" which means that there is already an admin
      isAdmin = strcmp(PHONE, phoneNumbers[0]) == 0;  // if so it will compare if the senders phone number is same as the phone number of the admin registered, if yes, then it will update the boolean isAdmin to true
    }

    // Serial.print(F("Sender: "));
    // Serial.print(PHONE);
    // Serial.print(F(" Message: "));
    // Serial.println(MESSAGE);
    // strcpy(CURRENT_MESSAGE, MESSAGE);

    //-----------------------------------------------------------------------"REGISTER ADMIN"---------------------------------------------------------------------------------------------//

    if (strcmp(MESSAGE, "REGISTER ADMIN") == 0) {  // This condition checks if the Message received is the same as "REGISTER ADMIN"
                                                   // If yes it will continue the process inside this condition
      deleteUserMode = false;                      // Delete user mode will be disabled
      addUserMode = false;                         // Add user mode will be disabled

      // Serial.print(F(" Message coppied!: "));

      // This condition checks if phoneNumber variable has already contain a number
      if (phoneNumbers[0][0] != '+') {
        // If phoneNumbers doesnt contain user number it will add the current sender to eeprom
        eeprom.addAdmin(PHONE);
        eeprom.readPhoneNumber(phoneNumbers[0], 0);  // This method will update the phoneNumber[0] to the admin number from eeprom
        GSMTEST.sendSMS(PHONE, SUCCESS);             //DEFINE PHONE NUMBER AND TEXT
      } else {
        // If phoneNumber has already store the admin number
        GSMTEST.sendSMS(PHONE, "ADMIN ALREADY REGISTERED!");  // Gives feedback to the sender that admin is already registered
      }
    }
    //-------------------------------------------------------------------------"DELETE ADMIN"---------------------------------------------------------------------------------------------//

    else if (strcmp(MESSAGE, "DELETE ADMIN") == 0 && isAdmin) {  // This condition checks if the Message received is the same as "DELETE ADMIN" and the sender is the admin
      eeprom.clearAll();                                         // This method clears all users including the admin in eeprom
      for (int i = 0; i < 5; i++) {                              // This instance will clear all the users and admin on phoneNumber variable
        phoneNumbers[i][0] = '\0';
      }
      GSMTEST.sendSMS(PHONE, "ADMIN AND USERS REGISTERED DELETED!");  // Sends feedback to the admin that all users and admin are deleted
    }
    //-------------------------------------------------------------------------"ADD USER"-------------------------------------------------------------------------------------------------//

    else if (strcmp(MESSAGE, "ADD USER") == 0 && isAdmin) {  // This condition checks if the Message received is the same as "ADD USER" and the sender is the admin
      deleteUserMode = false;                                // Delete user mode will be disabled
      addUserMode = true;                                    // Add user mode will be enabled

      if (eeprom.countRegisteredNumbers() >= 5) {                                    // This condition will check if the number of registered users are greater than or equal to 5
        Serial.println(GSMTEST.sendSMS(phoneNumbers[0], "MAXIMUM USERS REACHED!"));  // If yes it will send feedback to the admin that maximum users have reached
        GSMTEST.deleteSMS(messageIndex);                                             // This method delete all message on inbox
        return;                                                                      // Then it will return on receiving message
      }
      // If the number of registered user is less than 5 then it will give feedback to the admin to input a user to register
      GSMTEST.sendSMS(PHONE, "INPUT A USER NUMBER TO REGISTER!");
    }
    //-------------------------------------------------------------------------"DELETE USER"-------------------------------------------------------------------------------------------------//

    else if (strcmp(MESSAGE, "DELETE USER") == 0 && isAdmin) {   // This condition checks if the Message received is the same as "DELETE USER" and the sender is the admin
      addUserMode = false;                                       // Add user mode will be disabled
      deleteUserMode = true;                                     // Delete user mode will be enabled
      GSMTEST.sendSMS(PHONE, "INPUT A USER NUMBER TO DELETE!");  // I will give feedback to the admin to input a user to delete
    }
    //-------------------------------------------------------------------------deleteUserMode------------------------------------------------------------------------------------------------//

    else if (deleteUserMode && MESSAGE[0] == '+' && isAdmin) {  // If delete user mode is enabled and the first character of the message is "+" process continue
      bool isFound = false;
      deleteUserMode = false;                                // Delete user mode will be disabled
      if (strlen(MESSAGE) == 13) {                           // Checks if the message is 13 characters long
        for (int i = 1; i < 5; i++) {                        //
          if (eeprom.checkIfExist(MESSAGE, i)) {             // Checks if the number on the message exist on the user numbers stored on eeprom
            eeprom.deletePhoneNumber(i);                     // If yes it will be deleted on eeprom
            phoneNumbers[i][0] = '\0';                       // It will also be deleted on phoneNumber variable
            GSMTEST.sendSMS(PHONE, "USER NUMBER DELETED!");  // Then it will send feedback to the admin the the user is already deleted
            isFound = true;                                  // Then it will update the boolean isFound to true
            break;                                           // Then the process of this loop will stop
          }
        }
        if (!isFound) {  // If the number is not found on list of user numbers in eeprom it will send feedback to the admin that user is not found
          GSMTEST.sendSMS(PHONE, "USER NOT FOUND!");
        }
      }
    }
    //-------------------------------------------------------------------------addUserMode--------------------------------------------------------------------------------------------------//

    else if (addUserMode && MESSAGE[0] == '+' && isAdmin) {  // If add user mode is enabled and the first character of the message is "+" process continue
      deleteUserMode = false;                                // Delete user mode will be disabled
      addUserMode = false;                                   // Add user mode will be disabled

      for (int i = 0; i < 5; i++) {
        if (eeprom.checkIfExist(MESSAGE, i)) {                        // This method checks if the number is present on the list of regstered users
          GSMTEST.sendSMS(PHONE, "USER NUMBER ALREADY REGISTERED!");  // If yes if will send feedback to the admin that the user is already registered
          GSMTEST.deleteSMS(messageIndex);                            // Then all the message on inbox will be deleted
          return;
        }
      }
      //-------------------------------------------------------------------------Check if the number is valid-------------------------------------------------------------------------------//

      if (MESSAGE[0] == '+' && MESSAGE[1] == '6' && MESSAGE[2] == '3') {  // Checks if the first character is "+" and the second character is "6" and the third character is "3"
        char temp[11];
        for (int i = 3, j = 0; i <= 13; i++, j++) {
          temp[j] = MESSAGE[i];
        }
        temp[11] = '\0';

        if (strlen(MESSAGE) != 13) {  // Checks if the length of the message is not equal to 13, it will send feedback to the admin that the number is invalid
          GSMTEST.deleteSMS(messageIndex);
          GSMTEST.sendSMS(PHONE, "INVALID PHONE NUMBER!");
          return;
        }
      }

      for (int i = 1; i < 5; i++) {
        if (!eeprom.indexOccupied(i)) {                                     // This will check if the index on the array to store user number is already occupied if false it is a free space
          eeprom.addPhoneNumber(MESSAGE, i);                                // Then it will add the user number to the eeprom
          loadPhoneNumbers();                                               // Then it will call the loadPhoneNumbers function which updates the number stored on phoneNumber variable
          GSMTEST.sendSMS(MESSAGE, "USER NUMBER SUCCESSFULY REGISTERED!");  // Then it will send feedback to the added user the the number is added as a user
          GSMTEST.sendSMS(phoneNumbers[0], SUCCESS);                        // Then it will send feedback to the admin that the user number is successfully registered
          break;
        }
      }
    } else if (isAdmin) {  // If the number to be register as a user is the admin then it will give feedback to the admin that it is invalid code
      GSMTEST.sendSMS(phoneNumbers[0], "INVALID CODE!");
    }
  }
}

// This is a function that clears the contents of the SIM Inbox
void clearSMS() {
  for (int i = 0; i <= 30; i++) {
    GSMTEST.deleteSMS(i);
  }
}

// This function is used to send message to all users and admin if bin is full
void sendSMSFullBin(const char* binType) {
  char message[MESSAGE_LENGTH];
  sprintf(message, "%s FULL!", binType);

  unsigned long currentTime = millis();

  // This condition is used to check if the last message sent is equal to the current message to be sent. If for instance a bin is full and another bin become full this condition will becom false
  if (lastBin == Bin) {
    // If registered user is greater than one then it will send message to the registered user for every 30 mins
    if (eeprom.countRegisteredNumbers() >= 1 && ((currentTime - lastMessageTime >= MESSAGE_INTERVAL))) {

      // Serial.print("in sending group message: " );
      // Serial.println(eeprom.countRegisteredNumbers());

      for (int i = 0; i < eeprom.countRegisteredNumbers(); i++) {
        Serial.println(phoneNumbers[i]);
        GSMTEST.sendSMS(phoneNumbers[i], message);  // This instance will send the message to each user registered
        delay(10);                                  // Delay between each send to the users
      }
      lastMessageTime = currentTime;  // Updates the last message time to the current time
      lastBin = Bin;                  // Updates the last message sent to the current message sent
    }
  } else if ((lastBin != Bin) && (eeprom.countRegisteredNumbers() >= 1)) {  // If last message send is not equal to the current message to be sent it will instaneously send the message to users
    for (int i = 0; i < eeprom.countRegisteredNumbers(); i++) {
      GSMTEST.sendSMS(phoneNumbers[i], message);
      delay(10);
    }
    lastBin = Bin;  // Updates the last message sent to the current message sent
  }
}

// This fucntion checks the bins if there is full and decide what message to be send depending on which bins are full
void checkAndSendSMS() {
  // Bin updates the variable of message to be sent to check if the current message to be sent is equal to the last message sent or not
  // sendSMSFullBin call function will indicated the content of the message to be sent

  if (!metalBinFull && !plasticBinFull && !paperBinFull && wetBinFull) {
    Bin = 0;
    sendSMSFullBin("WET BIN IS");

  } else if (!metalBinFull && !plasticBinFull && paperBinFull && !wetBinFull) {
    Bin = 1;
    sendSMSFullBin("PAPER BIN IS");

  } else if (!metalBinFull && !plasticBinFull && paperBinFull && wetBinFull) {
    Bin = 2;
    sendSMSFullBin("PAPER AND WET BINS ARE");

  } else if (!metalBinFull && plasticBinFull && !paperBinFull && !wetBinFull) {
    Bin = 3;
    sendSMSFullBin("PLASTIC BIN IS");

  } else if (!metalBinFull && plasticBinFull && !paperBinFull && wetBinFull) {
    Bin = 4;
    sendSMSFullBin("PLASTIC AND WET BINS ARE");

  } else if (!metalBinFull && plasticBinFull && paperBinFull && !wetBinFull) {
    Bin = 5;
    sendSMSFullBin("PAPER AND PLASTIC BINS ARE");

  } else if (!metalBinFull && plasticBinFull && paperBinFull && wetBinFull) {
    Bin = 6;
    sendSMSFullBin("PAPER, PLASTIC, AND WET BINS ARE");

  } else if (metalBinFull && !plasticBinFull && !paperBinFull && !wetBinFull) {
    Bin = 7;
    sendSMSFullBin("METAL BIN IS");

  } else if (metalBinFull && !plasticBinFull && !paperBinFull && wetBinFull) {
    Bin = 8;
    sendSMSFullBin("METAL AND WET BINS ARE");

  } else if (metalBinFull && !plasticBinFull && paperBinFull && !wetBinFull) {
    Bin = 9;
    sendSMSFullBin("PAPER AND METAL BINS ARE");

  } else if (metalBinFull && !plasticBinFull && paperBinFull && wetBinFull) {
    Bin = 10;
    sendSMSFullBin("PAPER, METAL, AND WET BINS ARE");

  } else if (metalBinFull && plasticBinFull && !paperBinFull && !wetBinFull) {
    Bin = 11;
    sendSMSFullBin("PLASTIC, AND METAL BINS ARE");

  } else if (metalBinFull && plasticBinFull && !paperBinFull && wetBinFull) {
    Bin = 12;
    sendSMSFullBin("PLASTIC, METAL, AND WET BINS ARE");

  } else if (metalBinFull && plasticBinFull && paperBinFull && !wetBinFull) {
    Bin = 13;
    sendSMSFullBin("PAPER, PLASTIC, AND METAL BINS ARE");

  } else if (metalBinFull && plasticBinFull && paperBinFull && wetBinFull) {
    Bin = 14;
    sendSMSFullBin("ALL BINS ARE");
  }
}

// This function is used to turn the full bin status HIGH or LOW, this signal will be send to the main arduino (Sensor Module)
void fullBinPins(int fullBinPin, int status) {
  digitalWrite(fullBinPin, status);
}
