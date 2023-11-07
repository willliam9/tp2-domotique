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

void setup() {
  Serial.begin(115200);
  Serial.println("Creation de l'AP");
              
  WiFi.softAP("TravailPratique2-w", "motdepasse");
  Serial.println(WiFi.softAPIP());

  LittleFS.begin();
  httpd.onNotFound(HandleFileRequest);
  httpd.begin();

}

void loop() {
 httpd.handleClient();
}

