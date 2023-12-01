#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>
#include <PID_v1.h>
#include <PubSubClient.h>
// William Lussier, Nicolas St-Arnault et Olivier Mathieu

const int thermometre = A0;
const int relais = D1;
double temperatureCourante;

// set PID 
//---------------------------------
double Setpoint, Input, Output;
double Kp=300, Ki=20, Kd=1;

int WindowSizeOn = 100;
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
bool modeAuto = true;

unsigned long tempsAvantTemperatureStable = 0;
unsigned long tempsEcoule2DernieresMinutes = 0;
unsigned long tempsEcoule5DernieresMinutes = 0;

//PubSet Configuration 
//------------------------------------------------------------------
const char* ssid = "DEPTI_2.4";
const char* password = "2021depTI";
const char* mqttServer = "172.16.0.168"; // peut-être mettre /24
const int mqttPort = 1883; //1883 //8123
const char* mqttUser = "pi";
const char* mqttPassword = "pi";

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];
int value = 0;
//------------------------------------------------------------------
ESP8266WebServer httpd(80); // server 



// PubSub setup wifi client 
//------------------------------------------------------------------v
void setup_wifi() {

 
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
  }

    Serial.println("Connecté au WiFi");

    client.setServer(mqttServer, mqttPort);

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

//------------------------------------------------------------------


// Return a call PubSub 
//------------------------------------------------------------------
void callback(char* topic, byte* payload, unsigned int length) {
   Serial.print("Message arrived [");
   Serial.print(topic);
   Serial.print("] ");
   for (int i = 0; i < length; i++) {
     Serial.print((char)payload[i]);
   }
   Serial.println();
  String response = "";
  for (int i = 0; i < length; ++i)
  {
    response += (char)payload[i];
  }

  Serial.print(response);
  Serial.print(response.length());
  if(response.length() >= 3){
    if (response[0] == 'o' && response[1] == 'f' && response[2] == 'f') {
      allumer = false;
      chaud = false;
      digitalWrite(relais, LOW);
    } else if(response[0] == 'o' && response [1] == 'n') {
      allumer = true;
      chaud = true;
      digitalWrite(relais, HIGH);
    }
  }
}
//------------------------------------------------------------------


// Reconnect 
//------------------------------------------------------------------
void reconnect() {
  
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
  
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
  
    if (client.connect(clientId.c_str(), mqttUser, mqttPassword)) {
      Serial.println("connected");
  
      client.publish("outTopic", "hello world");
  
      client.subscribe("yogourt/control");
      client.subscribe("yogourt/temp");

    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
    }
  }
}
//------------------------------------------------------------------


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
      chaud = true;
      digitalWrite(relais,HIGH);
    } else if (stateParam == "false") {
      allumer = false;
      chaud = false;
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

void handleOutputRequest() {
  String jsonResponse = "{\"output\": " + String(Output) + "}";
  httpd.send(200, "application/json", jsonResponse);
  String stateParam = httpd.arg("state");
}

//Server
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

  // PubSup setup 
  //------------------------------------------------------------------
  pinMode(BUILTIN_LED, OUTPUT); 
  setup_wifi();
  client.setServer(mqttServer, 1883);
  client.setCallback(callback);
  //------------------------------------------------------------------


              
  WiFi.softAP("TP2-Stan", "motdepasse"); //Server
  Serial.println("soft APIP : ");
  Serial.println(WiFi.softAPIP());

  LittleFS.begin();

 httpd.onNotFound(HandleFileRequest); //Server 
  httpd.on("/toggle-heating", HTTP_GET, toggleHeatingState);
  httpd.on("/toggle-mode", HTTP_GET, toggleModeState);
  httpd.on("/temperature", HTTP_GET, handleTemperatureRequest);
  httpd.on("/temperaturemin2m", HTTP_GET, handleTemperatureMin2MRequest);
  httpd.on("/temperaturemax2m", HTTP_GET, handleTemperatureMax2MRequest);
  httpd.on("/temperaturemin5m", HTTP_GET, handleTemperatureMin5MRequest);
  httpd.on("/temperaturemax5m", HTTP_GET, handleTemperatureMax5MRequest);
  httpd.on("/secondeTemperatureStable", HTTP_GET, handleTemperatureStableRequest);
  httpd.on("/toggleHeating", HTTP_GET, handleToogleHeatingRequest);
  httpd.on("/output", HTTP_GET, handleOutputRequest);
  httpd.begin();


// PID 
//---------------------------------
  windowStartTime = millis();
  Setpoint = 43;
  tempPID.SetOutputLimits(0,WindowSizeOn);
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

  int tempsAllumage = WindowSizeOn * (Output / 100.0);
  int tempsExtinction = WindowSizeOn - tempsAllumage;
  Serial.println("OutPut : " + (String)Output);

  if(modeAuto){
    if (millis() - windowStartTime > WindowSizeOn)
    {
      windowStartTime += WindowSizeOn;
    }
    if (millis() - windowStartTime < tempsAllumage) {
      allume = true;
    }
    else{
      allume = false;
    } 

    if(chaud != allume){
      chaud = allume;
      digitalWrite(relais, chaud);
      Serial.print((String)chaud + " - Op: " + (String)Output);

    }

  }


}

void loop() {
  httpd.handleClient();

  if(allumer)
  MaintienTemperature();

 if(millis() % 1000 != 0){
    if (!client.connected()) {
      reconnect();
    }
  }
  
  if( millis() % 50 != 0 )
       return;
  else {
    CalculerTemperature();
  }


  client.loop();


  // PubSub loop 
  //------------------------------------------------------------------
    unsigned long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;
    ++value;
    snprintf (msg, MSG_BUFFER_SIZE, "Temperature : %.2f degré Celsius", temperatureCourante);
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish("yogourt/temp", msg);
  }
  //------------------------------------------------------------------


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

