#include <ArduinoJson.h>
#include <SoftwareSerial.h>

StaticJsonDocument<1024> doc;
SoftwareSerial mySerial(10, 11); // RX, TX

void setup()
{
  doc["frontDoor"] = 0;
  Serial.begin(9600);
  mySerial.begin(9600);  
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);
}

void loop()
{
  GetDataFromCam();
}

void GetDataFromCam() {
  if (mySerial.available()) {
    DeserializationError err = deserializeJson(doc, mySerial);
    if (err) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(err.c_str());
      while (mySerial.available() > 0)
        mySerial.read();
      return;
    }
    serializeJsonPretty(doc, Serial);
    if (doc["frontDoor"]) {
      digitalWrite(13, HIGH);
    }
    else {
      digitalWrite(13, LOW);
    }
  }
}
