#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <PubSubClient.h>
#include "LittleFS.h"
#define SETTED 5
#define SETTING -1



class Escon{

  private :
    const char *_mqtt_server;
    const char *_mqtt_username;
    const char *_mqtt_password;
    const char* _ssid;
    const char* _pass;
    String _topic;
    int _rstPin;
    int _id;
	
    public:
      
	int relayPin;
    Escon(int id,int relayPin, int rstPin);
    void readEEPROM();
    void readFS();
    void checkConnection(void(*cb)(char*,byte*,unsigned int));
    void reconnect();
    void handleLoop();
	void init_server( char *mqtt_server, char *mqtt_username, char *mqtt_pass);
	void init_wifi(char *ssid, char *pass);
    
    
};
