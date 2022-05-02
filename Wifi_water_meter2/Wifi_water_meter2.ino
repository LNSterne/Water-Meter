#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include "RTClib.h"
RTC_DS1307 RTC;
#include <OneWire.h>
#include <DallasTemperature.h>
#include <NTPClient.h>
#include <ESP8266HTTPClient.h>
const char* serverName = "http://192.168.1.61/post-esp-data.php";
String apiKeyValue = "tPmAT5Ab3j7F9";

volatile int NbTopsFan;            //measuring the rising edges of the signal
String datac = "";
float Calc;
int hallsensor = 5;
int ch2 = 2;
int ch1 = 16;
int SDAw = 4;
int SCLw = 0;
const int CS = 15;
DateTime now1;
DateTime now;
DateTime now2;
DateTime now3;
byte y = 0;
String but = "Close Tap";
String but2 = "Turn on Light";
File dataFile;
byte tap = 1;
byte lit = 1;
long leak = 0;
String leaks = "";
long rese = 0;
byte lito = 2;
OneWire oneWire(1);
String tapc = "lightgreen";
String lightc = "red";
long st = 0;
long en = 0;
int httpResponseCode;

DallasTemperature tempsensor(&oneWire);

const long utcOffsetInSeconds = 7200;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

#ifndef STASSID
#define STASSID "Link"
#define STAPSK  "9z517cL2CA"
#endif

const char* ssid = STASSID;
const char* password = STAPSK;

ESP8266WebServer server(80);

void handleRoot() {
  String set = "text/html\r\n";
  //"Refresh: 15";
  tempsensor.requestTemperatures();
  float temperature = tempsensor.getTempCByIndex(0);
  String HTML_Root = "<!DOCTYPE html>"
                     "<html>"
                     "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
                     "<body style=\"background-color:powderblue; text-align:center;\">"
                     "<h1>Water Meter</h1>"
                     "<div id=\"auto\">"
                     "<p>Flow Rate: " + String(Calc) + "<br><br>Temperature: " + String(temperature) + "<br><br>Time: " + now3.hour() + ":" + now3.minute() + ":" + now3.second() + "<br><br>Start Time: " + st + "  End Time: " + en + "<br>"+ httpResponseCode + "</p><br>"
                     "</div>"
                     "<script type=\"text/javascript\" src=\"http://ajax.googleapis.com/ajax/libs/jquery/1.3.0/jquery.min.js\"></script>"
                     "<script type = \"text/javascript\">"
                     "var auto_refresh = setInterval("
                     "function () {"
                     "$('#auto').load(document.URL +  ' #auto');"
                     "}, 1000);"
                     "</script>"
                     "<form action=\"/action_page\">"
                     "Start Time:"
                     "<select name=\"start\">"
                     "<option value=\"0\">0</option>""<option value=\"1\">1</option>""<option value=\"2\">2</option>""<option value=\"3\">3</option>""<option value=\"4\">4</option>""<option value=\"5\">5</option>""<option value=\"6\">6</option>""<option value=\"7\">7</option>""<option value=\"8\">8</option>""<option value=\"9\">9</option>""<option value=\"10\">10</option>""<option value=\11\">11</option>""<option value=\"12\">12</option>""<option value=\"13\">13</option>""<option value=\"14\">14</option>""<option value=\"15\">15</option>""<option value=\"16\">16</option>""<option value=\"17\">17</option>""<option value=\"18\">18</option>""<option value=\"19\">19</option>""<option value=\"20\">20</option>""<option value=\"21\">21</option>""<option value=\"22\">22</option>""<option value=\"23\">23</option>"
                     "</select>"
                     "<br>"
                     "End Time:"
                     "<select name=\"end\">"
                     "<option value=\"0\">0</option>""<option value=\"1\">1</option>""<option value=\"2\">2</option>""<option value=\"3\">3</option>""<option value=\"4\">4</option>""<option value=\"5\">5</option>""<option value=\"6\">6</option>""<option value=\"7\">7</option>""<option value=\"8\">8</option>""<option value=\"9\">9</option>""<option value=\"10\">10</option>""<option value=\11\">11</option>""<option value=\"12\">12</option>""<option value=\"13\">13</option>""<option value=\"14\">14</option>""<option value=\"15\">15</option>""<option value=\"16\">16</option>""<option value=\"17\">17</option>""<option value=\"18\">18</option>""<option value=\"19\">19</option>""<option value=\"20\">20</option>""<option value=\"21\">21</option>""<option value=\"22\">22</option>""<option value=\"23\">23</option>"
                     "</select>"
                     "<br>"
                     "<input type=\"submit\" value=\"Submit\">"
                     "</form><br>"
                     "<button style=\"text-align:center; width: 200px; height: 50px; background-color: " + tapc + ";\" onclick=\"window.location.href='/ledon'\">" + but + "</button> &emsp;&emsp;"
                     "<button style=\"text-align:center; width: 200px; height: 50px; background-color: " + lightc + ";\" onclick=\"window.location.href='/light'\">" + but2 + "</button><br><br>" + leaks +
                     "</body></html>";
  server.send ( 200, "text/html", HTML_Root );
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void handleForm() {
  String start = server.arg("start");
  String end = server.arg("end");
  st = start.toInt();
  en = end.toInt();
  if (st == en) {
    lito = 2;
  } else {
    lito = 0;
  }

  String s = "<!doctype html> <html> <head>"
             "<meta http-equiv=\"refresh\" content=\"0;url=/\">"
             "</head> <body> <h1>Light Schedule Set</h1> </body> </html>";
  server.send(200, "text/html", s); //Send web page
}

void setup(void) {
  //Serial.begin(115200);
  pinMode(hallsensor, INPUT);       //initializes digital pin 2 as an input
  pinMode(ch1, OUTPUT);
  pinMode(ch2, OUTPUT);
  pinMode(CS, OUTPUT);
  digitalWrite(CS, LOW);
  digitalWrite(ch1, HIGH);
  digitalWrite(ch2, HIGH);
  SD.begin(CS);

  Wire.begin(SDAw, SCLw);
  attachInterrupt(digitalPinToInterrupt(hallsensor), rpm, RISING);  //and the interrupt is attached
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  IPAddress ip(192, 168, 1, 200);
  IPAddress gateway(192, 168, 1, 1);
  IPAddress subnet(255, 255, 255, 0);
  IPAddress ip4 = ipaddr_addr( "8.8.8.8" );//dns 1
  IPAddress ip5 = ipaddr_addr( "8.8.4.4" );//dns 2
  WiFi.config(ip, gateway, subnet, ip4, ip5);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

  server.on ( "/", handleRoot );
  server.on("/action_page", handleForm);

  server.on ( "/favicon.ico", []() {
    server.send ( 200, "text/html", "" ); // better than 404
  } );

  server.on ( "/ledon", handleLEDON );
  server.on("/light", handleLIGHT);
  server.on("/temp", handletemp);
  server.on("/flow", handleflow);
  server.on("/lon", lightOn);
  server.on("/loff", lightOff);
  server.on("/won", waterOn);
  server.on("/woff", waterOff);
  server.on("/ws", waterstatus);

  server.onNotFound ( []() {
    Serial.println("Page Not Found");
    server.send ( 404, "text/html", "Page not Found" );
  } );

  server.begin();

  RTC.begin();

  timeClient.begin();
  timeClient.update();
  now1 = timeClient.getEpochTime();
  while (now1.year() < 2018 || now1.year() > 2020) {
    timeClient.update();
    now1 = timeClient.getEpochTime();
    y++;
    if (y == 5) {
      break;
    }
  }
  if (y < 5) {
    RTC.adjust(DateTime(now1));
  }
  y = 0;
  now = RTC.now();
  tempsensor.begin();
}

void loop(void) {
  now2 = RTC.now();
  ArduinoOTA.handle();
  server.handleClient();
  MDNS.update();
  NbTopsFan = 0;                    //Set NbTops to 0 ready for calculations
  sei();                            //Enables interrupts
  delay (1000);                     //Wait 1 second
  cli();                            //Disable interrupts
  Calc = (NbTopsFan + 3.) / (8.1 * 60.);
  if (Calc < 0.02) {
    Calc = 0.0;
    leak = 0;
  } else {
    now = RTC.now();
    tempsensor.requestTemperatures();
    HTTPClient http;
    http.begin(serverName);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    String httpRequestData = "api_key=" + apiKeyValue + "&value1=" + Calc + "&value2=" + tempsensor.getTempCByIndex(0) + "";
    httpResponseCode = http.POST(httpRequestData);
    http.end();
    leak++;
    if (leak == 1200) {
      digitalWrite(ch2, LOW);
      leaks = "LEAK DETECTED";
      tap = 0;
      but = "Open Tap";
      tapc = "red";
    }
  }
  now3 = RTC.now();
  if (rese > 86400) {
    timeClient.update();
    now1 = timeClient.getEpochTime();
    while (now1.year() < 2018 || now1.year() > 2020) {
      timeClient.update();
      now1 = timeClient.getEpochTime();
      y++;
      if (y == 5) {
        rese = 0;
        break;
      }
    }
    if (y < 5) {
      RTC.adjust(DateTime(now1));
    }
    rese = 0;
    y = 0;
  }
  rese++;
  if ((long) now3.hour() == st && lito == 0) {
    digitalWrite(ch1, LOW);
    lit = 0;
    but2 = "Turn Off Light";
    lito = 1;
    lightc = "lightgreen";
  }

  if ((long) now3.hour() == en && lito == 1) {
    digitalWrite(ch1, HIGH);
    lit = 1;
    but2 = "Turn On Light";
    lito = 0;
    lightc = "red";
  }
}


ICACHE_RAM_ATTR void rpm ()                        //This is the function that the interupt calls
{
  NbTopsFan++;                     //This function measures the rising and falling edge of signal
}


String HTML_LED_ON = "<!doctype html> <html> <head>"
                     "<meta http-equiv=\"refresh\" content=\"0;url=/\">"
                     "</head> <body> <h1>Tap is now Closed!</h1> </body> </html>";

String HTML_LED_OFF = "<!doctype html> <html> <head>"
                      "<meta http-equiv=\"refresh\" content=\"0;url=/\">"
                      "</head> <body> <h1>Tap is now Open!</h1> </body> </html>";

String HTML_LIGHT_ON = "<!doctype html> <html> <head>"
                       "<meta http-equiv=\"refresh\" content=\"0;url=/\">"
                       "</head> <body> <h1>Light is now on!</h1> </body> </html>";

String HTML_LIGHT_OFF = "<!doctype html> <html> <head>"
                        "<meta http-equiv=\"refresh\" content=\"0;url=/\">"
                        "</head> <body> <h1>Light is now off!</h1> </body> </html>";
                        


void handleLEDON() {
  if (tap == 1) {
    digitalWrite(ch2, LOW);
    server.send ( 200, "text/html", HTML_LED_ON );
    tap = 0;
    but = "Open Tap";
    tapc = "red";
  } else {
    digitalWrite(ch2, HIGH);
    server.send ( 200, "text/html", HTML_LED_OFF );
    tap = 1;
    but = "Close Tap";
    leaks = "";
    tapc = "lightgreen";
  }
}

void handletemp(){
  server.send(200, "application/json", "{\n" "\"temperature\": " + String(tempsensor.getTempCByIndex(0)) + ",\n ""\"humidity\": " + String("0") +  "" "\n}");
}

void handleflow(){
  server.send(200, "application/json", "{\n" "\"temperature\": " + String(Calc) + ",\n ""\"humidity\": " + String("0") +  "" "\n}");
}

void handleLIGHT() {
  if (lit == 1) {
    digitalWrite(ch1, LOW);
    server.send ( 200, "text/html", HTML_LIGHT_ON );
    lit = 0;
    but2 = "Turn Off Light";
    lightc = "lightgreen";
  } else {
    digitalWrite(ch1, HIGH);
    server.send ( 200, "text/html", HTML_LIGHT_OFF );
    lit = 1;
    but2 = "Turn On Light";
    lightc = "red";
  }
}

void lightOn(){
    digitalWrite(ch1, LOW);
    server.send ( 200, "text/html", "light is on" );
    lit = 0;
    but2 = "Turn Off Light";
    lightc = "lightgreen";
}

void lightOff(){
    digitalWrite(ch1, HIGH);
    server.send ( 200, "text/html", "light is off" );
    lit = 1;
    but2 = "Turn On Light";
    lightc = "red";
}

void waterOn(){
    digitalWrite(ch2, HIGH);
    server.send ( 200, "text/html", "water is on" );
    tap = 1;
    but = "Close Tap";
    leaks = "";
    tapc = "lightgreen";
}

void waterOff(){
    digitalWrite(ch2, LOW);
    server.send ( 200, "text/html", "water is off" );
    tap = 0;
    but = "Open Tap";
    tapc = "red";
    digitalWrite(ch2, HIGH);
}

void waterstatus(){
  if(tap == 1){
  server.send(200, "application/json", "1");
  }
  if(tap == 0){
    server.send(200, "application/json", "0");
  }
}
