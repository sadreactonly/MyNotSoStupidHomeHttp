#include <ArduinoJson.h>
#include <Servo.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <DHT.h>
#include <SPI.h>
#include <SD.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include "Time.h"
File myFile;

#define DHTTYPE DHT11 

//DHT11 sensor pin
#define DHTPin  3 //RX pin

// Water pump pins
#define ENBPin 2 // D4
#define IN3Pin 4 // D2
#define IN4Pin 0 // D3

//Servo pin
#define SERVOPin 5 //D1

// Light relay pin
#define LIGHTPin 16 //D0

//SD card
const int chipSelect = D8;  // used for ESP8266

//WiFi 
const char* ssid     = "ZTE_AAB74C";
const char* password = "43626416";

//UTC + offset 
const long utcOffsetInSeconds = 3600;

ESP8266WebServer server(80);
DHT dht(DHTPin, DHTTYPE);

Servo servo1;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", utcOffsetInSeconds);


//Time
Time feedingTime;
Time wateringTime;

static float celsiusTemp;
static float humidityTemp;
static int lightState;

void setup() {
  Serial.begin(115200);
 // startTime = millis();
  servo1.attach(SERVOPin);
  dht.begin();

  // Initialize the output variables as outputs
  pinMode(LIGHTPin, OUTPUT);
  pinMode(IN3Pin, OUTPUT);
  pinMode(IN4Pin, OUTPUT);
  pinMode(ENBPin, OUTPUT);
  
  // Set outputs to LOW
  digitalWrite(LIGHTPin, LOW);
  readConfig();
  connectAndStartServer();
}

void readConfig() {  
  if (!SD.begin(chipSelect)) {
    Serial.println("Initialization failed!");
    while (1);
  }
  Serial.println("Initialization of SD card success!");

  String data ="";
  File dataFile = SD.open("config.json",FILE_READ);

  if (dataFile) {

      while (dataFile.available())
      {
        data += (char)dataFile.read();
      }
      
      StaticJsonDocument<200> doc;
      deserializeJson(doc, data);
      //Set watering time
      wateringTime.setHours((int)doc["wateringTime"]["hours"]);
      wateringTime.setMinutes((int)doc["wateringTime"]["minnutes"]);
      wateringTime.setSeconds((int)doc["wateringTime"]["seconds"]);

      //Set feeding time
      feedingTime.setHours((int)doc["feedingTime"]["hours"]);
      feedingTime.setMinutes((int)doc["feedingTime"]["minnutes"]);
      feedingTime.setSeconds((int)doc["feedingTime"]["seconds"]);
  }
      dataFile.close();
}
void connectAndStartServer(){
    // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Server GET and POST methods
  server.on("/", handleRoot);
  server.on("/getTemperatureAndHumidity", handleTemperatureAndHumidity);
  server.on("/setLightState/",handleLightState);
  server.on("/startWatering",handlePump);
  server.on("/startFeeding",handleFeeder);
  server.on("/getConfiguration",handleGetConfiguration);
  server.onNotFound(handleNotFound);

  server.begin();
  
  Serial.println("HTTP server started");
}
void handleGetConfiguration(){
   String data ="";
   File dataFile = SD.open("config.json",FILE_READ);

    if (dataFile) {
  
        while (dataFile.available())
        {
          data += (char)dataFile.read();
        }
            server.send(200,"application/json",data);
    }
    else
    {
          server.send(400,"error with Sd card");
    }
    dataFile.close();
}
void handleFeeder(){
  servo1.write(0);
  delay(500);
  servo1.write(90);
  delay(500);
  servo1.write(0);
  delay(500);	
  
  server.send(200,"text/plain","Feeding done");
}
void handleLightState(){
  String message = "";
  if (server.method() != HTTP_POST) {
    Serial.println("Not allowed.");
    server.send(405, "text/plain", "Method Not Allowed");
  } 
  else 
  {
    String state = server.arg("plain");
    SetLight(state.toInt());
    server.send(200, "application/json", "{\"lightState\":" + state +"}");
  }
}
void handleTemperatureAndHumidity(){
  if(ReadDHT11())
  {
      server.send(200,"application/json","{\"temperature\":"+ String(celsiusTemp) +",\"humidity\":"+ String(humidityTemp) +"}");
  }
  else
  {
    server.send(400,"text/plain","Unable to read from sensor");
  }
  
}
void handleRoot(){
     ReadDHT11();
     server.send(200,"application/json","{\"temperature\":"+ String(celsiusTemp) +",\"humidity\":"+ String(humidityTemp) +",\"lightState\":"+ String(lightState) +"}");
}
void handleNotFound() {
  
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }

  server.send(404, "text/plain", message);
}
void handlePump(){
    // Run water pump
    digitalWrite(IN3Pin, LOW);
    digitalWrite(IN4Pin, HIGH);
    digitalWrite(ENBPin, HIGH);
    
    // Make pump run for 6 sec.
    Serial.println("Watering..");
    delay(6000);
    
    // Stop pump
    digitalWrite(IN3Pin, LOW);
    digitalWrite(IN4Pin, LOW);
    digitalWrite(ENBPin, LOW);

    Serial.println("Watering done.");

    server.send(200,"text/plain","Watering done");
}

void loop(void) {
  server.handleClient();
  MDNS.update();
 // getTime();
}
void getTime() {
 timeClient.update();
 int hours = timeClient.getHours();
 int minutes = timeClient.getMinutes();
 int seconds = timeClient.getSeconds();
 //use
 handleTime(Time(hours,minutes,seconds));
 delay(1000);
}
void handleTime(Time t){
   if(t.isEqual(feedingTime))
   {
      //handleFeeder();
   }
   if(t.isEqual(wateringTime))
   {
      handlePump();
   }
}

void SetLight(int state){
  digitalWrite(LIGHTPin, state);
  lightState = state;
}

bool ReadDHT11(){
          
            // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
            float h = dht.readHumidity();
            // Read temperature as Celsius (the default)
            float t = dht.readTemperature();
        
            // Check if any reads failed and exit early (to try again).
            if (isnan(h) || isnan(t)) {
              Serial.println("Failed to read from DHT sensor!");
              celsiusTemp = 0;
              humidityTemp = 0;     
              return false;    
            }
            else{
              // Computes temperature values in Celsius + Fahrenheit and Humidity
              celsiusTemp = t;
              humidityTemp = h;   
              return true;
            }
      
}
