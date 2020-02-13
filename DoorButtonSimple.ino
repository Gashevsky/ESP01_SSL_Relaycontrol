/*
  SSL encrypted, password-protected firmware updatable

  This starts a HTTPS server on the ESP8266 to allow firmware updates
  to be performed.  All communication, including the username and password,
  is encrypted via SSL.  Be sure to update the SSID and PASSWORD before running
  to allow connection to your WiFi network.

  To upload through terminal you can use:
  curl -u admin:admin -F "image=@firmware.bin" esp8266-webupdate.local/firmware
*/

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServerSecure.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266NetBIOS.h>
#include <ArduinoJson.h>

#ifndef STASSID
#define STASSID "Put WiFi SSID here"
#define STAPSK  "Put WIFI Password here"
#endif
IPAddress local_IP(192,168,2,22);
IPAddress gateway(192,168,2,9);
IPAddress subnet(255,255,255,0);
const char* host = "ESPDoorButton";
const char* update_path = "/firmware";
const char* update_username = "admin";
const char* update_password = "admin";
const char* ssid = STASSID;
const char* password = STAPSK;

BearSSL::ESP8266WebServerSecure httpServer(443);
ESP8266HTTPUpdateServer httpUpdater;

static const char serverCert[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
PUT YOUR CERTIFICATE HERE
-----END CERTIFICATE-----
)EOF";

static const char serverKey[] PROGMEM =  R"EOF(
-----BEGIN RSA PRIVATE KEY-----
PUT YOUR PRIVATE KEY HERE
-----END RSA PRIVATE KEY-----
)EOF";

const int led = 0;

void handleRoot() {
  //digitalWrite(led, 0);
  httpServer.send(200, "text/plain", "Hello from esp8266 over HTTPS!");
  //delay(500); 
  //digitalWrite(led, 1);
}

void handleNotFound(){
 // digitalWrite(led, 0);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += httpServer.uri();
  message += "\nMethod: ";
  message += (httpServer.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += httpServer.args();
  message += "\n";
  for (uint8_t i=0; i<httpServer.args(); i++){
    message += " " + httpServer.argName(i) + ": " + httpServer.arg(i) + "\n";
  }
  httpServer.send(404, "text/plain", message);
 // digitalWrite(led, 1);
}

void setup()
{
  //digitalWrite(led, 1);
  pinMode(led, OUTPUT);
  digitalWrite(led, 1);
  Serial.begin(115200);
  Serial.println(WiFi.softAPConfig(local_IP, gateway, subnet) ? "Ready" : "Failed!"); // soft AP initialization
  Serial.println("Booting Sketch...");
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid, password);

  while(WiFi.waitForConnectResult() != WL_CONNECTED){
    WiFi.begin(ssid, password);
    Serial.println("WiFi failed, retrying.");
  }

  configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");

  //MDNS.begin(host);

  httpServer.setRSACert(new BearSSL::X509List(serverCert), new BearSSL::PrivateKey(serverKey));
  
  httpServer.on("/", handleRoot);

  httpServer.on("/inline", [](){
    httpServer.send(200, "text/plain", "this works as well");
  });
  
  httpServer.onNotFound(handleNotFound);

  httpServer.on("/command/open", HTTP_POST, [](){
    StaticJsonBuffer<200> newBuffer;
    JsonObject& newjson = newBuffer.parseObject(httpServer.arg("plain"));
    //Your send may looks like "{\"pass\":\"Your Password to open door\"}  or  "{"pass":"Your Password to open door"}
    const char* pass = newjson["pass"];
    if (strcmp(pass, "Your Password to open door") == 0) {
      digitalWrite(led, 0);
      delay(400);
      digitalWrite(led, 1);
      httpServer.send ( 200, "text/json", "{\"success\":true}" );
    }
    else
    {
      httpServer.send(200, "text/json", "{\"success\":false}");
    }
  });
  
  httpUpdater.setup(&httpServer, update_path, update_username, update_password);
  httpServer.begin();
  NBNS.begin(host);
  //MDNS.addService("https", "tcp", 443);
  Serial.printf("BearSSLUpdateServer ready!\nOpen https://%s.local%s in "\
                "your browser and login with username '%s' and password "\
                "'%s'\n", host, update_path, update_username, update_password);
}
  
void loop()
{
  httpServer.handleClient();
 // MDNS.update();
  delay(100);
}
