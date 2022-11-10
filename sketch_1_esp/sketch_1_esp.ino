#include <PubSubClient.h>
#include "ThingsBoard.h"
#include <ESP8266WiFi.h>
#include <SoftwareSerial.h>
#include <ArduinoJson.h>

#define WIFI_AP "Spiderman"
#define WIFI_PASSWORD "vinh2223"

char Thingsboard_Server[] = "demo.thingsboard.io";
#define TOKEN "MispEwH7Jt4A4EbbSAhe"


WiFiClient wifiClient;
WiFiClient espClient;

ThingsBoardSized<256, 32> tb(espClient);
PubSubClient client(wifiClient);

SoftwareSerial mega(D2, D3); //rx,tx
int status = WL_IDLE_STATUS;

StaticJsonDocument<1024> transmitter;
StaticJsonDocument<1024> receiver;

float lastSend = 0;

void setup()
{
  Serial.begin(9600);
  mega.begin(9600);
  InitWiFi();
  client.setServer(Thingsboard_Server, 1883);
  client.setCallback(callback_sub);

}

void loop()
{
  if (!client.connected()) {
    reconnect();
  }
  if (mega.available() > 0)
  {
    DeserializationError err = deserializeJson(receiver, mega);
    if (err) {
      // Print error to the "debug" serial port
      Serial.print("deserializeJson() failed: ");
      Serial.println(err.c_str());
      // Flush all bytes in the "link" serial port buffer
      while (mega.available() > 0)
        mega.read();
      return;
    }
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
    SendDataToThingsboard();
  }

  client.loop();
  tb.loop();
}

void callback_sub(const char* topic, byte* payload, unsigned int length)
{
  Serial.println("Yes");
  StaticJsonDocument<256> data;
  deserializeJson(data, payload, length);

  String method1 = data["method"].as<String>();
  transmitter[method1] = (int)data["params"];
  serializeJson(transmitter, mega);

  String payload01 = "{" + method1 + ":" + transmitter[method1].as<String>() + "}";
  char attributes01[100];
  payload01.toCharArray( attributes01, 100 );
  client.publish( "v1/devices/me/attributes", attributes01 );
}

void SendDataToThingsboard()
{
  if ( millis() - lastSend > 1000 )
  {
    const int telemetry_items = 2;
    Telemetry telemetry[telemetry_items] = {
      { "temperature", transmitter["temperature"] },
      { "humidity",    transmitter["humidity"] },
    };
    tb.sendTelemetry(telemetry, telemetry_items);

    const int attribute_items = 8;
    Attribute attributes[attribute_items] = {
      { "livingroomLight", transmitter["livingroom"] },
      { "frontDoor", transmitter["frontDoor"] },
      { "theftMode", transmitter["theftMode"] },
      { "theftDetect", transmitter["theftDetect"] },
      { "speaker", transmitter["speaker"] },
      { "gasLeak", transmitter["gasLeak"] },
      { "fire", transmitter["fire"] },
      { "hanger", transmitter["hanger"] },
    };
    tb.sendAttributes(attributes, attribute_items);

    Serial.println("Sent data to Thingsboard ");
  }
  lastSend = millis();
}

void InitWiFi()
{
  Serial.println("Connecting to AP ...");
  // attempt to connect to WiFi network
  WiFi.begin(WIFI_AP, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("Connected to AP");
}


void reconnect() {
  // Loop until we're reconnected
  status = WiFi.status();
  if ( status != WL_CONNECTED) {
    WiFi.begin(WIFI_AP, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("Connected to AP");
  }
  Serial.print("Connecting to Thingsboard node ...");
  if ( client.connect("ESP8266 Device", TOKEN, NULL) ) {
    Serial.println( "[DONE]" );
    client.subscribe("v1/devices/me/rpc/request/+");
  } else {
    Serial.print( "[FAILED]" );
    Serial.println( " : retrying in 5 seconds]" );
    // Wait 5 seconds before retrying
    delay( 5000 );
  }
}
