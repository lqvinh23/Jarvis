#include <Keypad.h>
#include <EEPROM.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <Servo.h>
#include <ArduinoJson.h>
#include "DHT.h"

LiquidCrystal_I2C lcd(0x3F, 16, 2);
Servo sg90Door;
Servo sg90Hanger;
StaticJsonDocument<1024> receiver;
StaticJsonDocument<1024> transmitter;

#define servoDoor 13              
#define doorBtn 22                
#define li_light 24
#define li_lightBtn 26
#define pirBtn 30
#define pirSensor 32
#define theftMode 34
#define speaker 36
#define speakerBtn 38
#define gasSensor 39
#define alertLight 40
#define flameSensor 41
#define rainSensor 42
#define servoHanger 43   
#define hangerBtn 44           

float lastMillis = 0;

bool theftSpeaker = 0;

const int DHTPIN = 28;
const int DHTTYPE = DHT11;
DHT dht(DHTPIN, DHTTYPE);

/* ------------------------------KEYPAD--------------------------------*/
const byte numRows = 4;         //number of rows on the keypad
const byte numCols = 4;         //number of columns on the keypad

char keymap[numRows][numCols] =
{
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

char keypressed;                 //Where the keys are stored it changes very often
char code[] = {'6', '6', '0', '1'}; //The default code, you can change it or make it a 'n' digits one

char code_buff1[sizeof(code)];  //Where the new key is stored
char code_buff2[sizeof(code)];  //Where the new key is stored again so it's compared to the previous one

short a = 0, i = 0, s = 0, j = 0; //Variables used for keypad's purpose

byte rowPins[numRows] = {23, 25, 27, 29}; //Rows 0 to 3 //if you modify your pins you should modify this too
byte colPins[numCols] = {31, 33, 35, 37}; //Columns 0 to 3

Keypad myKeypad = Keypad(makeKeymap(keymap), rowPins, colPins, numRows, numCols);
/* --------------------------------------------------------------------*/

void setup()
{
  transmitter["frontDoor"] = 0;
  transmitter["livingroomLight"] = 0;
  transmitter["humidity"] = 0;
  transmitter["temperature"] = 0;
  transmitter["theftMode"] = 0;
  transmitter["theftDetect"] = 0;
  transmitter["speaker"] = 0;
  transmitter["gasLeak"] = 0;
  transmitter["fire"] = 0;
  transmitter["hanger"] = 0;
  
  Serial.begin(9600);
  Serial1.begin(9600);
  
  dht.begin();
  
  lcd.init();
  lcd.begin(16, 2);
  lcd.backlight();
  lcd.print("Final project");      //What's written on the LCD you can change
  
  sg90Door.attach(servoDoor);
  sg90Door.write(0);
  sg90Hanger.attach(servoHanger);
  sg90Hanger.write(0);

  pinMode(doorBtn, INPUT_PULLUP);
  pinMode(li_lightBtn, INPUT_PULLUP);
  pinMode(speakerBtn, INPUT_PULLUP);
  pinMode(hangerBtn, INPUT_PULLUP);
  pinMode(pirBtn, INPUT_PULLUP);
  pinMode(gasSensor, INPUT);
  pinMode(flameSensor, INPUT);
  pinMode(pirSensor, INPUT);
  pinMode(rainSensor, INPUT);
  pinMode(li_light, OUTPUT);
  pinMode(theftMode, OUTPUT);
  pinMode(speaker, OUTPUT);
  pinMode(alertLight, OUTPUT);

  //  for(i=0 ; i<sizeof(code);i++){        //When you upload the code the first time keep it commented
  //    EEPROM.get(i, code[i]);             //Upload the code and change it to store it in the EEPROM
  //  }                                  //Then uncomment this for loop and reupload the code (It's done only once)

}


void loop()
{
  GetDataFromESP();
  ReadNumpad();
  ReadButton();
  ReadSensor();
}

void ReadSensor() {
  // DHT11
  if ((unsigned long) (millis() - lastMillis) > 60000) { 
    transmitter["humidity"] = dht.readHumidity();
    transmitter["temperature"] = dht.readTemperature();
    serializeJson(transmitter, Serial1);
    lastMillis = millis();
  }

  // PIR
  if ((digitalRead(pirSensor) == HIGH) && (transmitter["theftMode"] == 1)) {
    transmitter["theftDetect"] = 1;
    digitalWrite(speaker, theftSpeaker);
    serializeJson(transmitter, Serial1);
  }

  // Gas sensor
  if (digitalRead(gasSensor) == HIGH) {
    transmitter["gasLeak"] = 1;
    transmitter["speaker"] = 1;
    digitalWrite(speaker, 1);
    digitalWrite(alertLight, 1);
    serializeJson(transmitter, Serial1);
  }

  // Flame sensor
  if (digitalRead(flameSensor) == HIGH) {
    transmitter["fire"] = 1;
    transmitter["speaker"] = 1;
    digitalWrite(speaker, 1);
    digitalWrite(alertLight, 1);
    serializeJson(transmitter, Serial1);
  }

    // Rain sensor
  if (digitalRead(rainSensor) == LOW) {
    transmitter["hanger"] = 0;
    sg90Hanger.write(0);
    serializeJson(transmitter, Serial1);
  }
}

void ReadButton() {
  if (digitalRead(doorBtn) == LOW) {  //Opening/closing the door by the push button
    delay(30);
    transmitter["frontDoor"] = !transmitter["frontDoor"];
    if (transmitter["frontDoor"]) {
      sg90Door.write(90);
    }
    else {
      sg90Door.write(0);
    }
    serializeJson(transmitter, Serial1);
    while (digitalRead(doorBtn) == LOW) {}
  }

  if ((digitalRead(hangerBtn) == LOW) && (digitalRead(rainSensor) == HIGH)) {  //Opening/closing the hanger by the push button
    delay(30);
    transmitter["hanger"] = !transmitter["hanger"];
    if (transmitter["hanger"]) {
      sg90Hanger.write(90);
    }
    else {
      sg90Hanger.write(0);
    }
    serializeJson(transmitter, Serial1);
    while (digitalRead(hangerBtn) == LOW) {}
  }

  if (digitalRead(li_lightBtn) == LOW) {  //Turning on/off by the push button
    delay(30);
    transmitter["livingroomLight"] = !transmitter["livingroomLight"];
    digitalWrite(li_light, transmitter["livingroomLight"]);
    serializeJson(transmitter, Serial1);
    while (digitalRead(li_lightBtn) == LOW) {}
  }

  if (digitalRead(pirBtn) == LOW) {  //Turning on/off anti-theft mode
    delay(30);
    transmitter["theftMode"] = !transmitter["theftMode"];
    transmitter["theftDetect"] = 0;
    digitalWrite(theftMode, transmitter["theftMode"]);
    theftSpeaker = 1;
    serializeJson(transmitter, Serial1);
    while (digitalRead(pirBtn) == LOW) {}
  }

  if (digitalRead(speakerBtn) == LOW) { //Turning on/off speaker
    transmitter["speaker"] = !transmitter["speaker"];
    if (!transmitter["speaker"]) {
      transmitter["gasLeak"] = 0;
      transmitter["fire"] = 0;
    }
    digitalWrite(alertLight, transmitter["speaker"]);
    digitalWrite(speaker, transmitter["speaker"]);
    serializeJson(transmitter, Serial1);
  }
}

void ReadNumpad() {
  keypressed = myKeypad.getKey();               //Constantly waiting for a key to be pressed
  if (keypressed == '*') {                    // * to open the lock
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Enter code");            //Message to show
    GetCode();                          //Getting code function
    if (a == sizeof(code))        //The GetCode function assign a value to a (it's correct when it has the size of the code array)
      OpenDoor();                   //Open lock function if code is correct
    else {
      lcd.clear();
      lcd.print("Wrong");          //Message to print when the code is wrong
    }
    delay(2000);
    lcd.clear();
    lcd.print("Jarvis Home");             //Return to standby mode it's the message do display when waiting
  }

  if (keypressed == '#') {                //To change the code it calls the changecode function
    ChangeCode();
    lcd.clear();
    lcd.print("Jarvis Home");                 //When done it returns to standby mode
  }

  if (keypressed == 'C') { 
    sg90Door.write(0);
    transmitter["frontDoor"] = 0;
    serializeJson(transmitter, Serial1);
  }

  if (keypressed == 'B') {
    transmitter["theftMode"] = !transmitter["theftMode"];
    transmitter["theftDetect"] = 0;
    theftSpeaker = 0;
    digitalWrite(theftMode, transmitter["theftMode"]);
    lcd.clear();
    lcd.print("Anti-theft: ");
    lcd.setCursor(12, 0);
    lcd.print(transmitter["theftMode"]?"On":"Off");
    delay(2000);
    lcd.clear();
    lcd.print("Jarvis Home");
    serializeJson(transmitter, Serial1);
  }
}

void GetDataFromESP() {
  if (Serial1.available()) {
    DeserializationError err = deserializeJson(receiver, Serial1);
    transmitter["frontDoor"] = receiver["frontDoor"].as<int>();
    transmitter["livingroomLight"] = receiver["livingroomLight"].as<int>();
    transmitter["humidity"] = receiver["humidity"].as<float>();
    transmitter["temperature"] = receiver["temperature"].as<float>();
    transmitter["theftMode"] = receiver["theftMode"].as<int>();
    transmitter["theftDetect"] = receiver["theftDetect"].as<int>();
    transmitter["speaker"] = receiver["speaker"].as<int>();
    transmitter["gasLeak"] = receiver["gasLeak"].as<int>();
    transmitter["fire"] = receiver["fire"].as<int>();
    transmitter["hanger"] = receiver["hanger"].as<int>();
    if (err) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(err.f_str());
      while (Serial1.available() > 0)
        Serial1.read();
      return;
    }
    digitalWrite(li_light, transmitter["livingroomLight"]);
    if (transmitter["frontDoor"]) {
      sg90Door.write(90);
    }
    else {
      sg90Door.write(0);
    }
    digitalWrite(speaker, transmitter["speaker"]);
  }
}

void GetCode() {                 //Getting code sequence
  i = 0;                         //All variables set to 0
  a = 0;
  j = 0;

  while (keypressed != 'A') {                             //The user press A to confirm the code otherwise he can keep typing
    keypressed = myKeypad.getKey();
    if (keypressed != NO_KEY && keypressed != 'A' ) {     //If the char typed isn't A and neither "nothing"
      lcd.setCursor(j, 1);                                //This to write "*" on the LCD whenever a key is pressed it's position is controlled by j
      lcd.print("*");
      j++;
      if (keypressed == code[i] && i < sizeof(code)) {    //if the char typed is correct a and i increments to verify the next caracter
        a++;                                              //Now I think maybe I should have use only a or i ... too lazy to test it -_-'
        i++;
      }
      else
        a--;                                              //if the character typed is wrong a decrements and cannot equal the size of code []
    }
  }
  keypressed = NO_KEY;

}

void ChangeCode() {                     //Change code sequence
  lcd.clear();
  lcd.print("Changing code");
  delay(1000);
  lcd.clear();
  lcd.print("Enter old code");
  GetCode();                      //verify the old code first so you can change it

  if (a == sizeof(code)) {  //again verifying the a value
    lcd.clear();
    lcd.print("Changing code");
    GetNewCode1();            //Get the new code
    GetNewCode2();            //Get the new code again to confirm it
    s = 0;
    for (i = 0 ; i < sizeof(code) ; i++) { //Compare codes in array 1 and array 2 from two previous functions
      if (code_buff1[i] == code_buff2[i])
        s++;                                //again this how we verifiy, increment s whenever codes are matching
    }
    if (s == sizeof(code)) {        //Correct is always the size of the array

      for (i = 0 ; i < sizeof(code) ; i++) {
        code[i] = code_buff2[i];       //the code array now receives the new code
        EEPROM.put(i, code[i]);        //And stores it in the EEPROM

      }
      lcd.clear();
      lcd.print("Code Changed");
      delay(2000);
    }
    else {                        //In case the new codes aren't matching
      lcd.clear();
      lcd.print("Codes are not");
      lcd.setCursor(0, 1);
      lcd.print("matching !!");
      delay(2000);
    }

  }

  else {                    //In case the old code is wrong you can't change it
    lcd.clear();
    lcd.print("Wrong");
    delay(2000);
  }
}

void GetNewCode1() {
  i = 0;
  j = 0;
  lcd.clear();
  lcd.print("Enter new code");   //tell the user to enter the new code and press A
  lcd.setCursor(0, 1);
  lcd.print("and press A");
  delay(2000);
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("and press A");     //Press A keep showing while the top row print ***

  while (keypressed != 'A') {          //A to confirm and quits the loop
    keypressed = myKeypad.getKey();
    if (keypressed != NO_KEY && keypressed != 'A' ) {
      lcd.setCursor(j, 0);
      lcd.print("*");               //On the new code you can show * as I did or change it to keypressed to show the keys
      code_buff1[i] = keypressed;   //Store caracters in the array
      i++;
      j++;
    }
  }
  keypressed = NO_KEY;
}

void GetNewCode2() {                        //This is exactly like the GetNewCode1 function but this time the code is stored in another array
  i = 0;
  j = 0;

  lcd.clear();
  lcd.print("Confirm code");
  lcd.setCursor(0, 1);
  lcd.print("and press A");
  delay(3000);
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("and press A");

  while (keypressed != 'A') {
    keypressed = myKeypad.getKey();
    if (keypressed != NO_KEY && keypressed != 'A' ) {
      lcd.setCursor(j, 0);
      lcd.print("*");
      code_buff2[i] = keypressed;
      i++;
      j++;
    }
  }
  keypressed = NO_KEY;
}

void OpenDoor() {            //Lock opening function open for 3s
  lcd.clear();
  lcd.print("Welcome");       //With a message printed
  sg90Door.write(90);
  delay(3000);
  sg90Door.write(0);
}
