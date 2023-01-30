#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <PubSubClient.h>
#include "LittleFS.h"
#include "Escon.h"
struct settings {
  char ssid[20];
  char password[20];
  int set;
}user_data = {};  

ESP8266WebServer server(80);
WiFiClient espClient;
PubSubClient client(espClient);
void callback(char *topic, byte *payload, unsigned int length);
void deviceConfig();
String _indexPage;
String _successPage;
String topics = "";

Escon::Escon(int id,  int relayPin , int rstPin)
{
	Serial.begin(9600);
	Serial.println(user_data.set);
  _id = id;
  relayPin = relayPin;
  _rstPin = rstPin;
  topics = "erom/escon/device/id/" + String(id);
  
}
void Escon::init_server( char *mqtt_server, char *mqtt_username, char *mqtt_pass)
{
  _mqtt_server = mqtt_server;
  _mqtt_username = mqtt_username;
  _mqtt_password = mqtt_pass;
}
void Escon::init_wifi(char *ssid, char *pass)
{
  _ssid = ssid;
  _pass = pass;
  pinMode(_rstPin, INPUT_PULLUP);
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, HIGH);
}

void Escon::readEEPROM()
{
  EEPROM.begin(sizeof(struct settings));
  EEPROM.get(0, user_data);
}

void Escon::readFS()
{
  if (!LittleFS.begin())
  {
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }
  File index = LittleFS.open("/index.html", "r");
  if (!index)
  {
    Serial.println("Failed to open index for reading");
    return;
  }

  while (index.available())
  {
    _indexPage = index.readString();
  }
  index.close();
  File success = LittleFS.open("/success.html", "r");
  if (!success)
  {
    Serial.println("Failed to open success for reading");
    return;
  }

  while (success.available())
  {
    _successPage = success.readString();
  }
  success.close();
}

void Escon::checkConnection(void(*cb)(char*,byte*,unsigned int))
{
  if (user_data.set != SETTED)
  {
    user_data.set = SETTING;
    Serial.println("SET access point");
    WiFi.mode(WIFI_AP);
    WiFi.softAP(_ssid, _pass);
    server.on("/", deviceConfig);
    server.begin();
  }
  else
  {
    WiFi.mode(WIFI_STA);
    WiFi.begin(user_data.ssid, user_data.password);
    byte tries = 0;
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(1000);
      if (tries++ > 20)
      {
        Serial.println("Failed to Connect");
        break;
      }
    }
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("MQTT BEGIN");
    client.setServer(_mqtt_server, 1883);
    client.setCallback(cb);
  }
}

/*void callback(char *topic, byte *payload, unsigned int length)
{
  String message = "";
  for (unsigned int i = 0; i < length; i++)
  {
    message += (char)payload[i];
  }
  digitalWrite(_relayPin, (message == "ON") ? LOW : HIGH);
}
*/

void Escon::reconnect()
{
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP8266Client-";
    clientId += String(WiFi.macAddress());
    bool restar_btn = digitalRead(_rstPin);
    if (restar_btn == 0)
    {
      Serial.println("Reset Button pressed");
      user_data.set = SETTING;
      EEPROM.put(0, user_data);
      EEPROM.commit();
      delay(500);
      ESP.restart();
    }
    if (client.connect(clientId.c_str(), _mqtt_username, _mqtt_password))
    {
      Serial.println("connected");
      client.subscribe(topics.c_str());
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }

  
}
void Escon::handleLoop()
  {
    while (user_data.set != SETTED)
    {
      server.handleClient();
      if (user_data.set == SETTED)
      {
        break;
      }
    }
    if (!client.connected() && user_data.set == SETTED)
    {
      reconnect();
    }
	
    client.loop();
  }
  
void deviceConfig(){
  if (server.method() == HTTP_POST) {

    strncpy(user_data.ssid,     server.arg("ssid").c_str(),     sizeof(user_data.ssid) );
    strncpy(user_data.password, server.arg("password").c_str(), sizeof(user_data.password) );
    user_data.set = SETTED;
    user_data.ssid[server.arg("ssid").length()] = user_data.password[server.arg("password").length()]= '\0';
    EEPROM.put(0, user_data);
    EEPROM.commit();
    server.send(200,   "text/html", _successPage);
    delay(2000);
    ESP.restart();
  }else{
    server.send(200,   "text/html", _indexPage);
  }
}