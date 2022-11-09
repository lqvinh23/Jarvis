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

ThingsBoard tb(espClient);
PubSubClient client(wifiClient);

SoftwareSerial mega(D2, D3); //rx,tx
int status = WL_IDLE_STATUS;

StaticJsonDocument<256> doc;

char ch;
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
    DeserializationError err = deserializeJson(doc, mega);
    // Test if parsing succeeds.
    if (err == DeserializationError::Ok) {
      bool frontDoor = doc["frontDoor"];
      bool livingroomLight = doc["livingroomLight"];
      float humidity = doc["humidity"];
      float temperature = doc["temperature"];
      // Print values.
      Serial.print("frontDoor: ");
      Serial.println(frontDoor);
      Serial.print("livingroomLight: ");
      Serial.println(livingroomLight);
      Serial.print("humidity: ");
      Serial.println(humidity);
      SendDataToThingsboard(livingroomLight, frontDoor, humidity, temperature);
    }
    else
    {
      // Print error to the "debug" serial port
      Serial.print("deserializeJson() returned ");
      Serial.println(err.c_str());

      // Flush all bytes in the "link" serial port buffer
      while (mega.available() > 0)
        mega.read();
    }
  }

  client.loop();
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

  //  if (method1 == "livingroomLight")
  //  {
  //    if (data["params"] == true)
  //    {
  //      doc["livingroomLight"] = 1;
  //      serializeJson(doc, mega);
  //      String payload01 = "{\"livingroomLight\":\"1\"}";
  //      char attributes01[100];
  //      payload01.toCharArray( attributes01, 100 );
  //      client.publish( "v1/devices/me/attributes", attributes01 );
  //    }
  //    else if (data["params"] == false)
  //    {
  //      doc["livingroomLight"] = 0;
  //      serializeJson(doc, mega);
  //      String payload01 = "{\"livingroomLight\":\"0\"}";
  //      char attributes01[100];
  //      payload01.toCharArray( attributes01, 100 );
  //      client.publish( "v1/devices/me/attributes", attributes01 );
  //    }
  //  }
  //
  //  if (method1 == "frontDoor")
  //  {
  //    if (data["params"] == true)
  //    {
  //      doc["frontDoor"] = 1;
  //      serializeJson(doc, mega);
  //      String payload02 = "{\"frontDoor\":\"1\"}";
  //      char attributes02[100];
  //      payload02.toCharArray( attributes02, 100 );
  //      client.publish( "v1/devices/me/attributes", attributes02 );
  //    }
  //    else if (data["params"] == false)
  //    {
  //      doc["frontDoor"] = 0;
  //      serializeJson(doc, mega);
  //      String payload02 = "{\"frontDoor\":\"0\"}";
  //      char attributes02[100];
  //      payload02.toCharArray( attributes02, 100 );
  //      client.publish( "v1/devices/me/attributes", attributes02 );
  //    }
  //  }
}

void SendDataToThingsboard(int livingroomLight, int frontDoor, float humidity, float temperature)
{
  if ( millis() - lastSend > 1000 )
  {
    // Prepare a JSON payload string
    String payload = "{";
    payload += "\"livingroomLight\":\"" + (String)livingroomLight + "\"}";
    char attributes[100];
    payload.toCharArray( attributes, 100 );
    client.publish( "v1/devices/me/attributes", attributes );

    String payload1 = "{";
    payload1 += "\"frontDoor\":\"" + (String)frontDoor + "\"}";
    char attributes1[100];
    payload1.toCharArray( attributes1, 100 );
    client.publish( "v1/devices/me/attributes", attributes1 );

    String payload2 = "{";
    payload2 += "\"humidity\":\"" + (String)humidity + "\"}";
    char attributes2[100];
    payload2.toCharArray( attributes2, 100 );
    client.publish( "v1/devices/me/telemetry", attributes2 );

    String payload3 = "{";
    payload3 += "\"temperature\":\"" + (String)temperature + "\"}";
    char attributes3[100];
    payload3.toCharArray( attributes3, 100 );
    client.publish( "v1/devices/me/telemetry", attributes3 );

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
