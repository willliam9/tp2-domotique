#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>

const int thermometre = A0;
const int relais = D1;

double temperatureCourante;

String tempsTemperatureStable = "";
float secondeTemperatureStable = 0;

double max2Minutes = 0;
double min2Minutes = 0;

double max5Minutes = 0;
double min5Minutes = 0;

int maxActuel = 0;
int minActuel = 100;

bool arretTotal = false;
bool allumer = false;

unsigned long tempsAvantTemperatureStable = 0;
unsigned long tempsEcoule2DernieresMinutes = 0;
unsigned long tempsEcoule5DernieresMinutes = 0;

//Pas certain que ce soit utile.
//int delaisTemp = 1000;
//unsigned long dernierMillisTemp;

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


void HandleFileRequest(){
  String fileName = httpd.uri();
  Serial.println(fileName); // 
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
  httpd.on("/temperature", HTTP_GET, handleTemperatureRequest);
  httpd.on("/temperaturemin2m", HTTP_GET, handleTemperatureMin2MRequest);
  httpd.on("/temperaturemax2m", HTTP_GET, handleTemperatureMax2MRequest);
  httpd.on("/temperaturemin5m", HTTP_GET, handleTemperatureMin5MRequest);
  httpd.on("/temperaturemax5m", HTTP_GET, handleTemperatureMax5MRequest);
  httpd.on("/secondeTemperatureStable", HTTP_GET, handleTemperatureStableRequest);

  httpd.begin();

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

void loop() {
  httpd.handleClient();
  if(temperatureCourante != 20)
  temperatureCourante = 43;
  if( millis() % 50 != 0 )
       return;
  else {
    //CalculerTemperature();
  }

  if(temperatureCourante <= 43){
    arretTotal = false;
  }

  if(temperatureCourante >= 50 && !arretTotal){
    arretTotal = true;
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

    if(minActuel > temperatureCourante)
      minActuel = temperatureCourante;

    if(maxActuel < temperatureCourante)
      maxActuel = temperatureCourante;

    if (((millis() - tempsEcoule2DernieresMinutes) / 1000) / 60 >= 2)
    {
      tempsEcoule2DernieresMinutes = millis();
      min2Minutes = minActuel;
      max2Minutes = maxActuel;
      temperatureCourante = 20;
    }

    if (((millis() - tempsEcoule5DernieresMinutes) / 1000) / 60 >= 5)
    {
      tempsEcoule5DernieresMinutes = millis();
      min5Minutes = minActuel;
      max5Minutes = maxActuel;
    }    
  }
}

