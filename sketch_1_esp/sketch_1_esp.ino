#include <PubSubClient.h>
#include "ThingsBoard.h"
#include <ESP8266WiFi.h>
#include <SoftwareSerial.h>
#include <ArduinoJson.h>

#define WIFI_AP "HP-ZBOOKG1"
#define WIFI_PASSWORD "23102000"

char Thingsboard_Server[] = "demo.thingsboard.io";
#define TOKEN "MispEwH7Jt4A4EbbSAhe"


WiFiClient wifiClient;
WiFiClient espClient;

ThingsBoardSized<256, 32> tb(espClient);
PubSubClient client(wifiClient);

SoftwareSerial mega(D2, D3); //rx,tx
int status = WL_IDLE_STATUS;

StaticJsonDocument<1024> doc;

float lastSend = 0;

String devices[13] = {"frontDoor", "livingroomLight", "livingroomFan", "kitchenLight", "kitchenFan", "bedroomLight", "bathroomLight", "theftMode", "theftDetect", "speaker", "gasLeak", "fire", "hanger"};
String telemetries[2] = {"temperature", "humidity"};

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
    DeserializationError err = deserializeJson(doc, mega);
    if (err) {
      // Print error to the "debug" serial port
      Serial.print("deserializeJson() failed: ");
      Serial.println(err.c_str());
      // Flush all bytes in the "link" serial port buffer
      while (mega.available() > 0)
        mega.read();
      return;
    }
    serializeJsonPretty(doc, Serial);
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
  doc[method1] = (int)data["params"];
  serializeJson(doc, mega);

  String payload01 = "{" + method1 + ":" + doc[method1].as<String>() + "}";
  char attributes01[100];
  payload01.toCharArray( attributes01, 100 );
  client.publish( "v1/devices/me/attributes", attributes01 );
}

void SendDataToThingsboard()
{
  if ( millis() - lastSend > 1000 )
  {
    float telemetry_val[2] = {
      doc["temperature"].as<float>(),
      doc["humidity"].as<float>(),
    };

    for (int i = 0; i < 2; i++) {
      String payload = "{" + telemetries[i] + ":" + (String)telemetry_val[i] + "}";
      char attributes[100];
      payload.toCharArray( attributes, 100 );
      client.publish( "v1/devices/me/telemetry", attributes );
    }

    int attribute_val[13] = {
      doc["frontDoor"].as<int>(), 
      doc["livingroomLight"].as<int>(), 
      doc["livingroomFan"].as<int>(),
      doc["kitchenLight"].as<int>(),
      doc["kitchenFan"].as<int>(),
      doc["bedroomLight"].as<int>(),
      doc["bathroomLight"].as<int>(),
      doc["theftMode"].as<int>(),
      doc["theftDetect"].as<int>(),
      doc["speaker"].as<int>(),
      doc["gasLeak"].as<int>(),
      doc["fire"].as<int>(),
      doc["hanger"].as<int>(),
    };

    for (int i = 0; i < 13; i++) {
      String payload1 = "{" + devices[i] + ":" + (String)attribute_val[i] + "}";
      char attributes1[100];
      payload1.toCharArray( attributes1, 100 );
      client.publish( "v1/devices/me/attributes", attributes1 );
    }

    Serial.println("\nSent data to Thingsboard ");
    lastSend = millis();
  }
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
