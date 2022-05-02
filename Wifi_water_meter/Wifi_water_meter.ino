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
float dailyn = 0;
long rese = 0;
byte lito = 0;

char timeServer[] = "time.nist.gov";  // NTP server
unsigned int localPort = 2390;        // local port to listen for UDP packets

const int NTP_PACKET_SIZE = 48;  // NTP timestamp is in the first 48 bytes of the message
const int UDP_TIMEOUT = 2000;    // timeout in miliseconds to wait for an UDP packet to arrive

byte packetBuffer[NTP_PACKET_SIZE]; // buffer to hold incoming and outgoing packets

WiFiUDP Udp;

#ifndef STASSID
#define STASSID "ASUS"
#define STAPSK  "802689099F20"
#endif

const char* ssid = STASSID;
const char* password = STAPSK;

ESP8266WebServer server(80);

void handleRoot() {
  String set = "text/html\r\n"
               "Refresh: 1";
  float temperature = measureT();
  String HTML_Root = "<!doctype html> <html> <head> Water Meter </head><body>\""
                     "<br><br>Temperature: " + String(temperature) + "<br><br>Flow Rate: "  + String(Calc) + "<br>"
                     "<button onclick=\"window.location.href='/ledon'\">" + but + "</button>\"<br>"
                     "<button onclick=\"window.location.href='/light'\">" + but2 + "</button>\"<br>"
                     "<button onclick=\"window.location.href='/constant'\">Current Usage</button>\"<br>"
                     "<button onclick=\"window.location.href='/daily'\">Daily Usage</button>\"<br>" + leaks +
                     "</body> </html>""";
  server.send ( 200, set, HTML_Root );
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

void setup(void) {
  Serial.begin(115200);
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

  server.on ( "/favicon.ico", []() {
    server.send ( 200, "text/html", "" ); // better than 404
  } );

  server.on ( "/ledon", handleLEDON );
  server.on("/light", handleLIGHT);
  server.on("/constant", handleCONSTANT);
  server.on("/daily", handleDAILY);

  server.onNotFound ( []() {
    Serial.println("Page Not Found");
    server.send ( 404, "text/html", "Page not Found" );
  } );

  server.begin();

  RTC.begin();

  Udp.begin(localPort);
  SetTime();
  while (now1.year() < 2018 || now1.year() > 2020) {
    SetTime();
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
    dataFile = SD.open("constant.txt", FILE_WRITE);
    dataFile.print(now.year(), DEC);
    dataFile.print('/');
    dataFile.print(now.month(), DEC);
    dataFile.print('/');
    dataFile.print(now.day(), DEC);
    dataFile.print(',');
    dataFile.print(now.hour(), DEC);
    dataFile.print(':');
    dataFile.print(now.minute(), DEC);
    dataFile.print(':');
    dataFile.print(now.second(), DEC);
    dataFile.print(",");
    dataFile.print(Calc);
    dataFile.print(",");
    dataFile.print(measureT());
    dataFile.println("<br>");
    dataFile.close();
    leak++;
    dailyn = dailyn + Calc;
    if (leak == 1200) {
      digitalWrite(ch2, LOW);
      leaks = "LEAK DETECTED";
      tap = 0;
      but = "Open Tap";
    }
  }
  now3 = RTC.now();
  if (now2.day() != now3.day()) {
    dataFile = SD.open("daily.txt", FILE_WRITE);
    dataFile.print(now2.year(), DEC);
    dataFile.print('/');
    dataFile.print(now2.month(), DEC);
    dataFile.print('/');
    dataFile.print(now2.day(), DEC);
    dataFile.print(',');
    dataFile.print(dailyn);
    dataFile.println("<br>");
    dataFile.close();
    dailyn = 0;
    SD.remove("constant.txt");
  }
  if (rese == 86400) {
    Udp.begin(localPort);
    SetTime();
    while (now1.year() < 2018 || now1.year() > 2020) {
      SetTime();
      y++;
      if (y == 5) {
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
  if ((long) now3.hour() == 19 && lito == 0) {
    digitalWrite(ch1, LOW);
    lit = 0;
    but2 = "Turn Off Light";
    lito = 1;
  }

  if ((long) now3.hour() == 21 && lito == 1) {
    digitalWrite(ch1, HIGH);
    lit = 1;
    but2 = "Turn On Light";
    lito = 0;
  }
}

float measureT()
{
  int v, mv, tm;
  v = analogRead(A0);
  mv = v * (3300 / 1024);
  tm = (mv - 500) / 10;
  return tm + 2; // TMP36
}

void rpm ()                        //This is the function that the interupt calls
{
  NbTopsFan++;                     //This function measures the rising and falling edge of signal
}

void SetTime() {
  sendNTPpacket(timeServer); // send an NTP packet to a time server

  // wait for a reply for UDP_TIMEOUT miliseconds
  unsigned long startMs = millis();
  while (!Udp.available() && (millis() - startMs) < UDP_TIMEOUT) {}

  if (Udp.parsePacket()) {
    // We've received a packet, read the data from it into the buffer
    Udp.read(packetBuffer, NTP_PACKET_SIZE);

    // the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    // now convert NTP time into everyday time:
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;
    // subtract seventy years:
    unsigned long epoch = secsSince1900 - seventyYears + 7200L;
    // print Unix time:
    now1 = epoch;
  }
}

// send an NTP request to the time server at the given address
void sendNTPpacket(char *ntpSrv)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)

  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(ntpSrv, 123); //NTP requests are to port 123

  Udp.write(packetBuffer, NTP_PACKET_SIZE);

  Udp.endPacket();
}

String HTML_LED_ON = "<!doctype html> <html> <head>\
<script>window.onload = function() { setInterval(function() {window.location.replace('/');}, 1500); };</script>\
</head> <body> <h1>Tap is now Closed!</h1> </body> </html>";

String HTML_LED_OFF = "<!doctype html> <html> <head>\
<script>window.onload = function() { setInterval(function() {window.location.replace('/');}, 1500); };</script>\
</head> <body> <h1>Tap is now Open!</h1> </body> </html>";

String HTML_LIGHT_ON = "<!doctype html> <html> <head>\
<script>window.onload = function() { setInterval(function() {window.location.replace('/');}, 1500); };</script>\
</head> <body> <h1>Light is now on!</h1> </body> </html>";

String HTML_LIGHT_OFF = "<!doctype html> <html> <head>\
<script>window.onload = function() { setInterval(function() {window.location.replace('/');}, 1500); };</script>\
</head> <body> <h1>Light is now off!</h1> </body> </html>";


void handleLEDON() {
  if (tap == 1) {
    digitalWrite(ch2, LOW);
    server.send ( 200, "text/html", HTML_LED_ON );
    tap = 0;
    but = "Open Tap";
  } else {
    digitalWrite(ch2, HIGH);
    server.send ( 200, "text/html", HTML_LED_OFF );
    tap = 1;
    but = "Close Tap";
    leaks = "";
  }
}

void handleLIGHT() {
  if (lit == 1) {
    digitalWrite(ch1, LOW);
    server.send ( 200, "text/html", HTML_LIGHT_ON );
    lit = 0;
    but2 = "Turn Off Light";
  } else {
    digitalWrite(ch1, HIGH);
    server.send ( 200, "text/html", HTML_LIGHT_OFF );
    lit = 1;
    but2 = "Turn On Light";
  }
}

void handleCONSTANT() {
  File myFile = SD.open("constant.txt");
  datac = "";
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send ( 200, "text/html", "");
  while (myFile.available()) {
    datac = myFile.readStringUntil('\n');
    server.sendContent(datac);
  }
  myFile.close();
}

void handleDAILY() {
  File myFile = SD.open("daily.txt");
  datac = "";
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send ( 200, "text/html", "");
  while (myFile.available()) {
    datac = myFile.readStringUntil('\n');
    server.sendContent(datac);
  }
  myFile.close();
}

