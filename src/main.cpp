/*********
  Rui Santos
  Complete project details at http://randomnerdtutorials.com  
*********/

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <Ticker.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <string.h>

ESP8266WebServer server(80);
char ssid[100]={0};
char password[100]={0};
#include "config.h"
#include "eeprom_func.h"

#define DeviceID "0200000001"
#define TOPIC "iot/device/0200000001"

int wifi_check_step = 0;
int mod = 0;

Ticker mytik;
Ticker mytik2;
int power = 0;
int onTimer = 0;
int offTimer = 0;
bool out = false;
const int PWM_OUT = LED_BUILTIN;
const int STATUS_OUT = D3;
WiFiClient espClient;
PubSubClient client(espClient);

/* prototypes */
void handleRoot();
void wifi_ap_mod();
void handleGenericArgs();
void pwm();
void onOffTimer();

void callback(char* topic, byte* payload, unsigned int length) {
 
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  char inData[80];
  StaticJsonBuffer<200> jsonBuffer;
  Serial.print("Message:");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    inData[i] = (char)payload[i];
  }
  char temp[1305], timeToOff[32]="", timeToOn[32]="", status[4];
  int sec = millis() / 1000;
  int min = sec / 60;
  int hr = min / 60;
  
  JsonObject& json_obj = jsonBuffer.parseObject(inData);  
  String strPower = json_obj["strPower"];
  if(strPower.length()>0){
    power = strPower.toInt();
    EEPROM.write(202, power);
    EEPROM.commit();
  }
  String strToggle = json_obj["strToggle"];
  if(strToggle.length()>0){
    if(strToggle=="off")
      out = true;
    else
      out = false;
  }
  String strOffTimer = json_obj["strOfft"];
  if(strOffTimer.length()>0){
    offTimer = strOffTimer.toInt();
    if(offTimer)
      digitalWrite(STATUS_OUT, 0);
  }

  String strOntimer = json_obj["ont"];
  if(strOntimer.length()>0){
    onTimer = strOntimer.toInt();
    if(onTimer)
      digitalWrite(STATUS_OUT, 0);
  }
  if(onTimer){
    snprintf(timeToOn, 32, "<p> Time To On: %d min</p>", onTimer);
  }
  if(offTimer){
    snprintf(timeToOff, 32, "<p> Time To Off: %d min</p>", offTimer);
  }
  if(out){
    strcpy(status, "on");
  }
  else{
    strcpy(status, "off");
  }
  Serial.println();
  Serial.println("-----------------------");
}

void wifi_ap_mod(){
  Serial.println("access point mode");
  WiFi.mode(WIFI_OFF);    //otherwise the module will not reconnect
  WiFi.softAP(ap_ssid);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.on("/", handleRoot);
  server.on("/genericArgs", handleGenericArgs);
  server.begin();
  Serial.println("HTTP server started");
}

void wifi_connect(){
  //save_ssid_to_eeprom("fuck");
  //save_password_to_eeprom("fuck");
  read_ssid_from_eeprom();
  read_password_from_eeprom();
  Serial.println("in wifi connect ssid:");
  Serial.println(ssid);
  Serial.println("in wifi connect password:");
  Serial.println(password);

  WiFi.persistent(false);     //These 3 lines are a required work-around
  WiFi.mode(WIFI_OFF);    //otherwise the module will not reconnect
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    wifi_check_step += 1;
    Serial.println("Connecting to WiFi...");
    if (wifi_check_step > 25){
      mod = 1;
      wifi_ap_mod();
      return;
    }
  }
    client.setServer(mqttServer, mqttPort);
    client.setCallback(callback);
    while (!client.connected()) {
      Serial.println("Connecting to MQTT...");
   
      if (client.connect("ESP8266Client")) {
   
        Serial.println("connected");  
   
      } else {
   
        Serial.print("failed with state ");
        Serial.print(client.state());
        delay(2000);
   
      }
  }
  client.subscribe(TOPIC); 
  
}



void setup(void){
  EEPROM.begin(512); 
  Serial.begin(115200); 
  delay(2000);
  
  wifi_connect();
  pinMode(PWM_OUT, OUTPUT);
  digitalWrite(PWM_OUT, 1);
  pinMode(STATUS_OUT, OUTPUT);
  digitalWrite(STATUS_OUT, 1);
  mytik.attach_ms(20, pwm); //attache pwm function
  mytik2.attach(60, onOffTimer); // attache onOffTimer function
//Serial.println("Connected to the WiFi network");
}
 


//void(* resetFunc) (void) = 0; //declare reset function @ address 0

void handleGenericArgs() { 
  Serial.println(server.arg(0));
  Serial.println(server.arg(1));
  save_ssid_to_eeprom(server.arg(0));
  save_password_to_eeprom(server.arg(1));
  server.send(200, "text/html", "Done");       //Response to the HTTP request
  delay(2000);
  mod = 0;
  wifi_check_step = 0;
  wifi_connect();
}

void handleRoot() {
  server.send(200, "text/html", "<h1>You are connected</h1>");
}

void pwm(){
	static int tick=0;

	if (tick<power) {
		if(out) //zero active :D
			digitalWrite(PWM_OUT, 0);
		else
			digitalWrite(PWM_OUT, 1);
	}
	else{
		digitalWrite(PWM_OUT, 1);
	}
	tick++;
	if (tick>99) {
		tick=0;
	}
}

/** Handle on Timer and off Timer
*		call evry one minute
*/
void onOffTimer(){
	if(onTimer > 0){
		digitalWrite(STATUS_OUT, 0);
		onTimer --;
		if(onTimer==0){
			out = true;
			if(offTimer==0)
				digitalWrite(STATUS_OUT, 1);
		}
	}
	if(offTimer > 0){
		digitalWrite(STATUS_OUT, 0);
		offTimer --;
		if(offTimer==0){
			out = false;
			digitalWrite(PWM_OUT, 1);
			if(onTimer==0)
				digitalWrite(STATUS_OUT, 1);
		}
	}
}

void loop() {
  if(mod == 1){
    server.handleClient();
  }else{
    client.loop();
  }
}