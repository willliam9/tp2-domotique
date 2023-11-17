#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>
#include <PID_v1.h>


const int thermometre = A0;
const int relais = D1;
double temperatureCourante;

// set PID 
//---------------------------------
double Setpoint, Input, Output;
double Kp=2, Ki=5, Kd=1;

int WindowSizeOn = 300; // 300
unsigned long windowStartTime;
PID tempPID(&Input, &Output, &Setpoint, Kp, Ki, Kd, DIRECT);
//---------------------------------

float secondeTemperatureStable = 0;

double max2Minutes = 0;
double min2Minutes = 0;

double max5Minutes = 0;
double min5Minutes = 0;

double maxActuel2Minutes = 0;
double minActuel2Minutes = 1000;
double maxActuel5Minutes = 0;
double minActuel5Minutes = 1000;


bool chaud = false;
bool allumer = false;
bool modeAuto = true; // Est-ce que le crockpot peut s'éteindre et s'allumer par lui même. // Est-ce que la méthode MaintienTemperature() peut de faire appeler 

unsigned long tempsAvantTemperatureStable = 0;
unsigned long tempsEcoule2DernieresMinutes = 0;
unsigned long tempsEcoule5DernieresMinutes = 0;

ESP8266WebServer httpd(80);

String GetContentType(String filename){
  struct Mime{
    String ext, type;
  }

  mimeType[] = {
    {".html", "text/html"}
  };

  for (int i = 0; i < sizeof(mimeType) / sizeof(Mime); i++)
  {
    if(filename.endsWith(mimeType[i].ext))
      return mimeType[i].type;
  }

  return "octet-stream";
  
}

void toggleHeatingState() {
  if(httpd.hasArg("state")){
    String stateParam = httpd.arg("state");
    Serial.println(stateParam);
    if (stateParam == "true") {
      allumer = true;
      digitalWrite(relais,HIGH);
    } else if (stateParam == "false") {
      allumer = false;
      digitalWrite(relais,LOW);
    }
  }
}

void toggleModeState() {
  if(httpd.hasArg("state")){
    String stateParam = httpd.arg("state");
    Serial.println(stateParam);
    if (stateParam == "true") {
      modeAuto = true;
    } else if (stateParam == "false") {
      modeAuto = false;
    }
  }
}


void handleTemperatureRequest() {
  String jsonResponse = "{\"temperature\": " + String(temperatureCourante) + "}";
  httpd.send(200, "application/json", jsonResponse);
  String stateParam = httpd.arg("state");
}

void handleTemperatureMin2MRequest() {
  String jsonResponse = "{\"temperaturemin2m\": " + String(min2Minutes) + "}";
  httpd.send(200, "application/json", jsonResponse);
  String stateParam = httpd.arg("state");
}

void handleTemperatureMax2MRequest() {
  String jsonResponse = "{\"temperaturemax2m\": " + String(max2Minutes) + "}";
  httpd.send(200, "application/json", jsonResponse);
  String stateParam = httpd.arg("state");
}

void handleTemperatureMin5MRequest() {
  String jsonResponse = "{\"temperaturemin5m\": " + String(min5Minutes) + "}";
  httpd.send(200, "application/json", jsonResponse);
  String stateParam = httpd.arg("state");
}

void handleTemperatureMax5MRequest() {
  String jsonResponse = "{\"temperaturemax5m\": " + String(max5Minutes) + "}";
  httpd.send(200, "application/json", jsonResponse);
  String stateParam = httpd.arg("state");
}

void handleTemperatureStableRequest() {
  String jsonResponse = "{\"secondeTemperatureStable\": " + String(secondeTemperatureStable) + "}";
  httpd.send(200, "application/json", jsonResponse);
  String stateParam = httpd.arg("state");
}

void handleToogleHeatingRequest() {
  String jsonResponse = "{\"toggleHeating\": " + String(allumer) + "}";
  httpd.send(200, "application/json", jsonResponse);
  String stateParam = httpd.arg("state");
}

void HandleFileRequest(){
  String fileName = httpd.uri();
  Serial.println(fileName);
  if(fileName.endsWith("/"))
    fileName = "index.html";
  if(LittleFS.exists(fileName)){
    File file = LittleFS.open(fileName,"r");
    httpd.streamFile(file, GetContentType(fileName));
    file.close();
  }
  else {
    httpd.send(404,"text/plain", "404 not found");
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(relais, OUTPUT);

  Serial.println("Creation de l'AP");
              
  WiFi.softAP("TP2-Stan", "motdepasse");
  Serial.println(WiFi.softAPIP());

  LittleFS.begin();
  httpd.onNotFound(HandleFileRequest);
  httpd.on("/toggle-heating", HTTP_GET, toggleHeatingState);
  httpd.on("/toggle-mode", HTTP_GET, toggleModeState);
  httpd.on("/temperature", HTTP_GET, handleTemperatureRequest);
  httpd.on("/temperaturemin2m", HTTP_GET, handleTemperatureMin2MRequest);
  httpd.on("/temperaturemax2m", HTTP_GET, handleTemperatureMax2MRequest);
  httpd.on("/temperaturemin5m", HTTP_GET, handleTemperatureMin5MRequest);
  httpd.on("/temperaturemax5m", HTTP_GET, handleTemperatureMax5MRequest);
  httpd.on("/secondeTemperatureStable", HTTP_GET, handleTemperatureStableRequest);
  httpd.on("/toggleHeating", HTTP_GET, handleToogleHeatingRequest);

  httpd.begin();


// PID 
//---------------------------------
  windowStartTime = millis();
  Setpoint = 43;
  tempPID.SetOutputLimits(0,WindowSizeOn); // windowStartOff
  tempPID.SetMode(AUTOMATIC);
//---------------------------------

}

void CalculerTempsTemperatureStable(){
    float secondes = (millis() - tempsAvantTemperatureStable) / 1000; 
  	
    secondeTemperatureStable = secondes;
}

void CalculerTemperature(){
  int valeurTemperature = analogRead(thermometre);

  double voltage = valeurTemperature * 5.0 / 1023.0;
  double resistance = 10000.0 * voltage / (5.0 - voltage);
  double tempKelvin = 1.0 / (1.0 / 298.15 + log(resistance / 10000.0) / 3977); //Beta = 3977

  temperatureCourante = tempKelvin - 273.15;
}

void MaintienTemperature(){

  Input = temperatureCourante;
  bool b = tempPID.Compute();
  bool allume = false;

  // Calcul du temps d'allumage et d'extinction en fonction du pourcentage
  int tempsAllumage = WindowSizeOn * (Output / 100.0);
  int tempsExtinction = WindowSizeOn - tempsAllumage;

  if(b && modeAuto){
    if (millis() - windowStartTime > WindowSizeOn)
    { //time to shift the Relay Window
      windowStartTime += WindowSizeOn;
    }
    if (Output > 0 && millis() - windowStartTime < tempsAllumage) {
      allume = true;
      //digitalWrite(relais, HIGH);
      //Serial.print("HIGH - O: "+ (String)Output);
    }
    else{
      allume = false;
      //digitalWrite(relais, LOW);
      //Serial.print("LOW - O: " + (String)Output);
    } 

    if(allumer != allume){
      allumer = allume;
      digitalWrite(relais, allumer);
      Serial.print(allumer + " - Op: " + (String)Output);

    }

  }


}

void loop() {
  httpd.handleClient();
  if( millis() % 50 != 0 )
       return;
  else {
    CalculerTemperature();
    if(allumer)
    MaintienTemperature();
  }

  if(temperatureCourante <= 43 && allumer){
    chaud = true;
  }

  if(temperatureCourante >= 50){
    chaud = false;
    digitalWrite(relais, LOW);
  }
  else{
    
    if(temperatureCourante > 41 && temperatureCourante < 45){
      CalculerTempsTemperatureStable();
    }
    else{
      secondeTemperatureStable = 0;
      tempsAvantTemperatureStable = millis();
    }

    if(minActuel2Minutes > temperatureCourante)
      minActuel2Minutes = temperatureCourante;
    
    if(minActuel5Minutes > temperatureCourante)
      minActuel5Minutes = temperatureCourante;

    if(maxActuel2Minutes < temperatureCourante)
      maxActuel2Minutes = temperatureCourante;

    if(maxActuel5Minutes < temperatureCourante)
      maxActuel5Minutes = temperatureCourante;

    if (((millis() - tempsEcoule2DernieresMinutes) / 1000) / 60 >= 2)
    {
      tempsEcoule2DernieresMinutes = millis();
      min2Minutes = minActuel2Minutes;
      max2Minutes = maxActuel2Minutes;
      maxActuel2Minutes = 0;
      minActuel2Minutes = 1000;
    }

    if (((millis() - tempsEcoule5DernieresMinutes) / 1000) / 60 >= 5)
    {
      tempsEcoule5DernieresMinutes = millis();
      min5Minutes = minActuel5Minutes;
      max5Minutes = maxActuel5Minutes;
      maxActuel5Minutes = 0;
      minActuel5Minutes = 1000;
    }    
  }
}

