#include <Arduino.h>
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <EEPROM.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <MyFi.h>         //https://github.com/tzapu/WiFiManager
#include <Ticker.h>

Ticker mytik;
Ticker mytik2;
int power = 0;
int onTimer = 0;
int offTimer = 0;
bool out = true;
ESP8266WebServer server(80);
const int PWM_OUT = LED_BUILTIN;

/** Handle pwm
*		call evry 20ms
*/
void pwm(){
	static int tick=0;

	if (tick<power) {
		// if(out) //zero active :D
			digitalWrite(PWM_OUT, 0);
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
		onTimer --;
		if(onTimer==0)
			out = true;
	}
	if(offTimer > 0){
		offTimer --;
		if(offTimer==0){
			out = false;
			digitalWrite(PWM_OUT, 1);
		}
	}
}

/** Handle root for http */
void HttpRoot() {
	char temp[1024];
	int sec = millis() / 1000;
	int min = sec / 60;
	int hr = min / 60;

	String strPower = server.arg("power");
	if(strPower.length()>0){
  	power = strPower.toInt();
		EEPROM.write(0, power);
		EEPROM.commit();
	}

	String strOffTimer = server.arg("offt");
	if(strOffTimer.length()>0){
  	offTimer = strOffTimer.toInt();
		EEPROM.write(2, offTimer);
		EEPROM.commit();
	}

	String strOntimer = server.arg("ont");
	if(strOntimer.length()>0){
  	onTimer = strOntimer.toInt();
		EEPROM.write(4, onTimer);
		EEPROM.commit();
	}
	snprintf ( temp, 1000, "<html>\
  <head>\
    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/>\
    <title>Jooyeshgar Demo</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; text-align: center; }\
			div,input,button{padding:5px;margin:5px;font-size:1em;width:95%;} form{max-width:350px;margin:auto}\
			button{border:0;border-radius:0.3rem;background:#1fa3ec;color:#fff;line-height:2rem;font-size:1.2rem;}\
    </style>\
  </head>\
  <body>\
    <h1>Hello from Jooyeshgar</h1>\
		<form method='get' action='?'>\
		 <input name='power' value='%d' length=5 placeholder='power'><br/><button type='submit'>Power</button><hr />\
		 <input name='offt' value='%d' length=5 placeholder='OFF Timer'><br/><button type='submit'>OFF Timer</button><hr />\
     <input name='ont' value='%d' length=5 placeholder='ON Timer'><br/><button type='submit'>ON Timer</button><hr /></form>\
    <p>Uptime: %02d:%02d:%02d</p><p>Status: %s</p>\
  </body>\
</html>", power, offTimer, onTimer, hr, min % 60, sec % 60, (out)?"off":"on" );

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

void setup() {
		EEPROM.begin(512);
    // put your setup code here, to run once:
    Serial.begin(115200);
		power = EEPROM.read(0);
		offTimer = EEPROM.read(2);
		onTimer = EEPROM.read(4);

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
		mytik.attach_ms(20, pwm); //attache pwm function
		mytik2.attach(60, onOffTimer); // attache onOffTimer function
}

void loop() {
    //HTTP
    server.handleClient();

}
