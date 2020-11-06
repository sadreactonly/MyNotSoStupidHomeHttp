#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <DHT.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <Servo.h>


#define DHTTYPE DHT11 

//DHT11 sensor pin
#define DHTPin  4

// Water pump pins
#define ENBPin 2 // PWM pin 6
#define IN3Pin 12
#define IN4Pin  13

//Servo pin
#define SERVOPin 15

// Light relay pin
#define LIGHTPin  5


// Define NTP Client to get time

const char* ssid     = "ZTE_AAB74C";
const char* password = "43626416";

const long utcOffsetInSeconds = 3600;

ESP8266WebServer server(80);
DHT dht(DHTPin, DHTTYPE);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", utcOffsetInSeconds);
Servo servo1;

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
  ConnectAndStartServer();
}

void ConnectAndStartServer(){
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
  server.onNotFound(handleNotFound);

  server.begin();
  
  Serial.println("HTTP server started");
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
  getTime();
 /* if (millis() - startTime > TWELVE_HRS)
  {
    // Put your code that runs every 12 hours here

    startTime = millis();
  }*/
}
void getTime() {
  timeClient.update();

  int hours = timeClient.getHours();
  int minutes = timeClient.getMinutes();
  int seconds = timeClient.getSeconds();

  if(hours == 8 && 
     minutes == 0 && 
     seconds == 0)
  {
      handlePump();
      handleFeeder();
  }

  if(hours == 18 && 
     minutes == 0 && 
     seconds == 0)
  {
      handleFeeder();
  }

  delay(1000);
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
