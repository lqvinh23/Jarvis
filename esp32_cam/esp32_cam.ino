//Viral Science www.youtube.com/c/viralscience  www.viralsciencecreativity.com
//ESP Camera Artificial Intelligence Face Detection Automatic Door Lock
#include "esp_camera.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include "ThingsBoard.h"
#include <SoftwareSerial.h>
#include <ArduinoJson.h>

// Select camera model
//#define CAMERA_MODEL_WROVER_KIT
//#define CAMERA_MODEL_ESP_EYE
//#define CAMERA_MODEL_M5STACK_PSRAM
//#define CAMERA_MODEL_M5STACK_WIDE
#define CAMERA_MODEL_AI_THINKER
#define Relay 2
#define Red 13
#define Green 12
#include "camera_pins.h"

#define WIFI_AP "Spiderman"
#define WIFI_PASSWORD "vinh2223"

const char* ssid = "Spiderman"; //Wifi Name SSID
const char* password = "vinh2223"; //WIFI Password

void startCameraServer();

bool matchFace = false;
bool activateRelay = false;
long prevMillis=0;
int interval = 5000;

/* ---------------------Thingsboard-------------------- */
char Thingsboard_Server[] = "demo.thingsboard.io";
#define TOKEN "MispEwH7Jt4A4EbbSAhe"


WiFiClient wifiClient;
WiFiClient espClient;

ThingsBoardSized<256, 32> tb(espClient);
PubSubClient client(wifiClient);

SoftwareSerial mega(D2, D3); //rx,tx
int status = WL_IDLE_STATUS;

StaticJsonDocument<1024> doc;
/* ---------------------------------------------------- */

void setup() {
  pinMode(Relay,OUTPUT);
  pinMode(Red,OUTPUT);
  pinMode(Green,OUTPUT);
  digitalWrite(Relay,LOW);
  digitalWrite(Red,HIGH);
  digitalWrite(Green,LOW);
  
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  //init with high specs to pre-allocate larger buffers
  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t * s = esp_camera_sensor_get();
  //initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1);//flip it back
    s->set_brightness(s, 1);//up the blightness just a bit
    s->set_saturation(s, -2);//lower the saturation
  }
  //drop down frame size for higher initial frame rate
  s->set_framesize(s, FRAMESIZE_QVGA);

#if defined(CAMERA_MODEL_M5STACK_WIDE)
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
#endif

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  startCameraServer();

  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");

/* ---------------------Thingsboard-------------------- */
  client.setServer(Thingsboard_Server, 1883);
  client.setCallback(callback_sub);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }

  if (matchFace==true && activateRelay==false) {
    activateRelay=true;
    digitalWrite(Relay,HIGH);
    digitalWrite(Green,HIGH);
    digitalWrite(Red,LOW);
    prevMillis=millis();
  }
  if (activateRelay == true && millis()-prevMillis > interval) {
    activateRelay=false;
    matchFace=false;
    digitalWrite(Relay,LOW);
    digitalWrite(Green,LOW);
    digitalWrite(Red,HIGH);
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

    int attribute_val[numberOfDevice] = {
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

    for (int i = 0; i < numberOfDevice; i++) {
      String payload1 = "{" + devices[i] + ":" + (String)attribute_val[i] + "}";
      char attributes1[100];
      payload1.toCharArray( attributes1, 100 );
      client.publish( "v1/devices/me/attributes", attributes1 );
    }

    Serial.println("\nSent data to Thingsboard ");
    lastSend = millis();
  }
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