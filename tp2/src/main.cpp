#include <Arduino.h>
#include <ESP8266WiFi.h>
//le lab s'appelle ap pour access point

#include <ESP8266WebServer.h>
                      
ESP8266WebServer httpd(80);
int test = 2;
void HandleRacine(){
            
  httpd.send(200, "text/html", "<html><body>"+ String(test) + "</body></html>");
}

bool led = false;
void HandleLed(){
  if(httpd.hasArg("action")){
    String action = httpd.arg("action");
    if(action == "on")
      led = true;
    else if (action == "off")
      led = false;
  }
  String reponse = "<html><body>";
    reponse += "<a href=\"/led?action=on\">ON</a>";
    reponse += "<a href=\"/led?action=off\">OFF</a>";
    reponse += "<div style=\"width: 10px; height: 10px; background-color: ";
    reponse += (led ? "green" : "red");
    reponse += "\"></div>";
    reponse += "</body></html>";

    httpd.send(200, "text/html", reponse.c_str()); 
}

void setup() {
  Serial.begin(115200);
  Serial.println("Creation de l'AP");
              
  WiFi.softAP("TravailPratique2", "motdepasse");
  Serial.println(WiFi.softAPIP());

  //Serveur web
  httpd.on("/", HandleRacine);
  httpd.on("/led", HandleLed);
  httpd.begin();
}

void loop() {
  httpd.handleClient();
  test++;
}
