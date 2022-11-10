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

ThingsBoardSized<128, 32> tb(espClient);
PubSubClient client(wifiClient);

SoftwareSerial mega(D2, D3); //rx,tx
int status = WL_IDLE_STATUS;

StaticJsonDocument<256> transmitter;
StaticJsonDocument<256> receiver;

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
    transmitter["frontDoor"] = receiver["frontDoor"];
    transmitter["livingroomLight"] = receiver["livingroomLight"];
    transmitter["humidity"] = receiver["humidity"];
    transmitter["temperature"] = receiver["temperature"];
    transmitter["theftMode"] = receiver["theftMode"];
    transmitter["theftDetect"] = receiver["theftDetect"];
    transmitter["speaker"] = receiver["speaker"];
    transmitter["gasLeak"] = receiver["gasLeak"];
    transmitter["fire"] = receiver["fire"];
    transmitter["hanger"] = receiver["hanger"];
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
      { "temperature", transmitter["temperature"].as<float>() },
      { "humidity",    transmitter["humidity"].as<float>() },
    };
    tb.sendTelemetry(telemetry, telemetry_items);

    const int attribute_items = 6;
    Attribute attributes[attribute_items] = {
      { "livingroomLight", transmitter["livingroom"].as<int>() },
      { "frontDoor", transmitter["frontDoor"].as<int>() },
      { "theftMode", transmitter["theftMode"].as<int>() },
      { "theftDetect", transmitter["theftDetect"].as<int>() },
      { "speaker", transmitter["speaker"].as<int>() },
      { "gasLeak", transmitter["gasLeak"].as<int>() },
      { "fire", transmitter["fire"].as<int>() },
      { "hanger", transmitter["hanger"].as<int>() },
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
