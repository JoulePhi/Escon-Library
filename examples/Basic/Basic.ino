#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include "LittleFS.h"
#define AP_MODE 5
#define STA_MODE 6
#define CONFIG_BTN 5       
//Penyimpanan ke memory
struct settings {
  char ssid[20];
  char password[20];
  int set;
}user_data = {};  

ESP8266WebServer server(80);
String indexPage;
String successPage;
bool isClient = false;
unsigned long prevTime = 0;
void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(2, OUTPUT);
  digitalWrite(2, HIGH);
  pinMode(CONFIG_BTN, INPUT_PULLUP);

  // Memulai dan membaca EEPROM
  EEPROM.begin(sizeof(struct settings) );
  EEPROM.get( 0, user_data );

  //Membaca file HTML
  read_FS();

  open_Ap();


  // Variabel total Client
  int tClient = 0;
  while(true){

    //Setiap 2 detik mengecek Client
    unsigned long waitTime = millis();
    if(waitTime - prevTime > 2000){
      digitalWrite(2, !(waitTime%2));
      if(server.client() > 0) tClient++;
      prevTime = waitTime;
    }

    // Menunggu 20 detik apakah ada client
    // Kalau ada berarti tetap menyalakan server
    // Kalau tidak ada keluar dari loop dan mengkoneksikan nodemcu dengan ssid dan pass yang sudah
    // di atur lewat server
    if(waitTime > 20000  && tClient < 2){
      Serial.println("Time Stop");
      break;
    }
    Serial.println(tClient);
    server.handleClient();
  }

  // Mengkoneksikan nodemcu dengan ssid dan pass yang sudah di atur
  Serial.println("WiFi STA");
  Serial.print("Connecting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(user_data.ssid, user_data.password);
  int tryConn = 0;

  // Menunggu nodemcu terkoneksi
  while(WiFi.status() != WL_CONNECTED){
    if(tryConn > 100){
      break;
    }
    Serial.print(".");
    digitalWrite(2, HIGH);
    delay(100);
    digitalWrite(2, LOW);
    delay(100);
    tryConn++;
  }

  digitalWrite(2, !(WiFi.status() == WL_CONNECTED));
  
  
}

void loop() {
  // put your main code here, to run repeatedly:
  static bool bButtonPressed = false;
  static unsigned long lButtonWait;
  bool switchStatus = !(digitalRead(CONFIG_BTN));            

  if ( switchStatus && !bButtonPressed )  {
    lButtonWait = millis();
    bButtonPressed = true;
  }
  else  {
    if ( switchStatus && bButtonPressed )  {
      if ( (millis() - lButtonWait) >= 5000)  {        
        open_Ap();
        bButtonPressed = false;
        while (digitalRead(CONFIG_BTN) != 1);                 
        while(1){
          server.handleClient();
        }
      }
    } else {
      if ( bButtonPressed )  {
        bButtonPressed = false;    
      }
    }
  }
}

void open_Ap(){
    //Menyalakan Access Point
  Serial.println("AP Begin");
  WiFi.mode(WIFI_AP);
  //SSID dan Password Access Point
  WiFi.softAP("Device Config", "deviceconfig");
  //Menyalakan server dari file HTML
  server.on("/",deviceConfig);
  server.begin();
}
void deviceConfig(){
  if (server.method() == HTTP_POST) {
    strncpy(user_data.ssid,     server.arg("ssid").c_str(),     sizeof(user_data.ssid) );
    strncpy(user_data.password, server.arg("password").c_str(), sizeof(user_data.password) );
    user_data.ssid[server.arg("ssid").length()] = user_data.password[server.arg("password").length()] =  '\0';
    if(strlen(user_data.ssid) != 0){
      EEPROM.put(0, user_data);
      EEPROM.commit();
      isClient = true;
      server.send(200,   "text/html", successPage);
      delay(1000);
      ESP.restart();
    }else{
      server.send(200,   "text/html", successPage);
      delay(1000);
      ESP.restart();
    }
  }else{
    server.send(200,   "text/html", indexPage);
  }
}
void read_FS(){
  if(!LittleFS.begin()){
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }
  File index = LittleFS.open("/index.html", "r");
  if(!index){
    Serial.println("Failed to open index for reading");
    return;
  }

  while (index.available())
  {
    indexPage = index.readString();
  }
  index.close();
  File success = LittleFS.open("/success.html", "r");
  if(!success){
    Serial.println("Failed to open success for reading");
    return;
  }

  while (success.available())
  {
    successPage = success.readString();
  }
  success.close();
}
