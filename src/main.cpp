#include <Arduino.h>
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <EEPROM.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <MyFi.h>         //https://github.com/tzapu/WiFiManager
#include <Ticker.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

const char* mqtt_server = "88.99.85.188";
WiFiClient espClient;
PubSubClient client(espClient);
#define DeviceID "001"
Ticker mytik;
Ticker mytik2;
int power = 0;
int onTimer = 0;
int offTimer = 0;
bool out = false;
ESP8266WebServer server(80);
const int PWM_OUT = LED_BUILTIN;
const int STATUS_OUT = D3;

/** Handle pwm
*		call evry 20ms
*/
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

/** Handle root for http */
void HttpRoot() {
	char temp[1305], timeToOff[32]="", timeToOn[32]="", status[4];
	int sec = millis() / 1000;
	int min = sec / 60;
	int hr = min / 60;

	String strPower = server.arg("power");
	if(strPower.length()>0){
  	power = strPower.toInt();
		EEPROM.write(0, power);
		EEPROM.commit();
	}

	String strToggle = server.arg("tg");
	if(strToggle.length()>0){
		if(strToggle=="off")
			out = true;
		else
			out = false;
	}

	String strOffTimer = server.arg("offt");
	if(strOffTimer.length()>0){
		offTimer = strOffTimer.toInt();
		if(offTimer)
			digitalWrite(STATUS_OUT, 0);
	}

	String strOntimer = server.arg("ont");
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

	snprintf ( temp, 1200, "<html>\
  <head>\
		<meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/>\
		<meta http-equiv='refresh' content='10; url=/' />\
    <title>Jooyeshgar Demo</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; text-align: center; }\
			div,input,button,a{padding:5px;margin:5px;font-size:1em;width:95%;} form{max-width:350px;margin:auto}\
			button,a{display:block;border:0;border-radius:0.3rem;background:#1fa3ec;color:#fff;line-height:2rem;font-size:1.2rem;}\
    </style>\
  </head>\
  <body>\
    <h1>Hello from Jooyeshgar</h1>\
		<form method='get' action='?'>\
		 <input name='power' value='%d' length=5 placeholder='power'><br/><button type='submit'>Power</button><hr />\
		 <input name='offt' value='%d' length=5 placeholder='OFF Timer'><br/><button type='submit'>OFF Timer</button><hr />\
		 <input name='ont' value='%d' length=5 placeholder='ON Timer'><br/><button type='submit'>ON Timer</button><hr />\
		 <a href='/?tg=%s'>%s</a></form>\
		<p>Uptime: %02d:%02d:%02d</p>\
		%s %s\
  </body>\
</html>", power, offTimer, onTimer, status, status, hr, min % 60, sec % 60, timeToOff, timeToOn );

	server.send ( 200, "text/html", temp );
}

void handleNotFound(){
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += server.uri();
    message += "\nMethod: ";
    message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
    message += "\nArguments: ";
    message += server.args();
    message += "\n";

    for ( uint8_t i = 0; i < server.args(); i++ ) {
      message += " " + server.argName ( i ) + ": " + server.arg ( i ) + "\n";
    }

    server.send ( 404, "text/plain", message );
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
	Serial.print("Message arrived in topic: ");
	Serial.println(topic);
	char buff[50] = "";
	int i;
	for (i=0 ; i < length; i++) {
		buff[i] = (char) payload[i];
    }
	buff[i] = '\0';
	String str_rcv = String((char*) buff);
    
    StaticJsonBuffer<200> jsonBuffer;
	JsonObject& root = jsonBuffer.parseObject(str_rcv);
	////
	char temp[1305], timeToOff[32]="", timeToOn[32]="", status[4];
	int sec = millis() / 1000;
	int min = sec / 60;
	int hr = min / 60;

	String strPower = root["power"];
	if(strPower.length()>0){
  	power = strPower.toInt();
		EEPROM.write(0, power);
		EEPROM.commit();
	}

	String strToggle = root["tg"];;
	if(strToggle.length()>0){
		if(strToggle=="off")
			out = true;
		else
			out = false;
	}

	String strOffTimer = root["offt"];;
	if(strOffTimer.length()>0){
		offTimer = strOffTimer.toInt();
		if(offTimer)
			digitalWrite(STATUS_OUT, 0);
	}

	String strOntimer = root["ont"];;
	if(strOntimer.length()>0){
		onTimer = strOntimer.toInt();
		if(onTimer)
			digitalWrite(STATUS_OUT, 0);
	}
	if(onTimer){
		snprintf(timeToOn, 32, "%d", onTimer);
	}
	if(offTimer){
		snprintf(timeToOff, 32, "%d", offTimer);
	}
	if(out){
		strcpy(status, "on");
	}
	else{
		strcpy(status, "off");
	}

	snprintf ( temp, 1200, "\
		{\
		 'power': '%d',\
		 'offt':'%d',\
		 'ont':'%d',\
		 'status': '%s',\
		 'Uptime': '%02d:%02d:%02d', \
		 'timeToOff': '%s', \
		 'timeToOn': '%s'\
		}", power, offTimer, onTimer, status, hr, min % 60, sec % 60, timeToOff, timeToOn );

	Serial.println(temp);	
}

void mqttReconnect() {
    // reconnect code from PubSubClient example
}

void setup() {
		EEPROM.begin(512);
    // put your setup code here, to run once:
    Serial.begin(115200);
		power = EEPROM.read(0);
		// offTimer = EEPROM.read(2);
		// onTimer = EEPROM.read(4);

    WiFiManager wifiManager;
    //wifiManager.resetSettings();

    wifiManager.setAPStaticIPConfig(IPAddress(192,168,1,1), IPAddress(192,168,1,1), IPAddress(255,255,255,0));
		wifiManager.setSTAStaticIPConfig(IPAddress(192,168,1,90), IPAddress(192,168,1,1), IPAddress(255,255,255,0));

    wifiManager.autoConnect("Jooyesh2");
    Serial.println("connected...");

    server.on("/", HttpRoot);
    server.onNotFound (handleNotFound);
    server.begin(); // Web server start

		pinMode(PWM_OUT, OUTPUT);
		digitalWrite(PWM_OUT, 1);
		pinMode(STATUS_OUT, OUTPUT);
		digitalWrite(STATUS_OUT, 1);
		mytik.attach_ms(20, pwm); //attache pwm function
		mytik2.attach(60, onOffTimer); // attache onOffTimer function
	// mqtt connection
	client.setServer(mqtt_server, 1883);
	client.setCallback(mqttCallback);
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

  client.subscribe("iot/switch/001"); 
}

void loop() {
    //HTTP
    server.handleClient();
	client.loop();

}
