#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>

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

  return "application/octet-stream";
  
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

const int thermometre = A0;
const int activationRelais = D1;

double temperatureCourante;

String tempsTemperatureStable = "";

int max2Minutes = 0;
int min2Minutes = 0;

int max5Minutes = 0;
int min5Minutes = 0;

int maxActuel = 0;
int minActuel = 0;

bool arretTotal = false;

unsigned long tempsAvantTemperatureStable = 0;
unsigned long tempsEcoule2DernieresMinutes = 0;
unsigned long tempsEcoule5DernieresMinutes = 0;

void setup() {
  Serial.begin(115200);

  pinMode(activationRelais, OUTPUT);

  Serial.println("Creation de l'AP");
              
  WiFi.softAP("TravailPratique2-Stan", "motdepasse");
  Serial.println(WiFi.softAPIP());

  LittleFS.begin();
  httpd.onNotFound(HandleFileRequest);
  httpd.begin();

}

void CalculerTempsTemperatureStable(){
    unsigned int secondes = (millis() - tempsAvantTemperatureStable) / 1000; 
  	
    unsigned int minutes = secondes / 60;
    
    unsigned int heures = minutes / 60;
  	  
    heures %= 24;
  	minutes %= 60;
    secondes %= 60;

    tempsTemperatureStable = String(heures) + ":" + String(minutes) + ":" + String(secondes);
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
  if(arretTotal == false){

    CalculerTemperature();

    if(temperatureCourante >= 50){
      arretTotal = true;
      digitalWrite(activationRelais, LOW);
    }
    else{
      
      if(temperatureCourante > 41 && temperatureCourante < 45){
        CalculerTempsTemperatureStable();
      }
      else{
        tempsTemperatureStable = "";
        tempsAvantTemperatureStable = millis();
      }


      if(minActuel > temperatureCourante)
        minActuel = temperatureCourante;

      if(maxActuel < temperatureCourante)
        maxActuel = temperatureCourante;

      if (((millis() - tempsEcoule2DernieresMinutes) / 1000) / 60 > 2)
      {
        tempsEcoule2DernieresMinutes = millis();
        min2Minutes = minActuel;
        max2Minutes = maxActuel;
      }
  
      if (((millis() - tempsEcoule5DernieresMinutes) / 1000) / 60 > 5)
      {
        tempsEcoule5DernieresMinutes = millis();
        min5Minutes = minActuel;
        max5Minutes = maxActuel;
      }
    }
  }
}

