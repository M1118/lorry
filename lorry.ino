#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266mDNS.h>
#include "WiFiManager.h"          //https://github.com/tzapu/WiFiManager


/*
   Lorry electronics

   Wemos D1 mini
   DRV8833
*/

#define CONFIG_AP "lorry8266"
#define HOSTNAME  "lorry"

#define MOT_A1_PIN D2
#define MOT_A2_PIN D1

#define BUFLEN   40

char buffer[BUFLEN];
char *bufptr;

const int port = 23;

WiFiServer server(port);

void configModeCallback (WiFiManager *myWiFiManager)
{
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
}

void setup()
{

  Serial.begin(115200);
  delay(10);

  // We start by connecting to a WiFi network

  Serial.println();

  Serial.println();
  //Serial.print("Connecting to wifi ");
  // Serial.println(ssid);
  // WiFi.begin(ssid, password);

  WiFiManager wifiManager;
  //wifiManager.resetSettings();
  wifiManager.setAPCallback(configModeCallback);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect(CONFIG_AP)) {
    Serial.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(1000);
  }

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.print("Address: ");
  Serial.println(WiFi.localIP());

  if (!MDNS.begin(HOSTNAME))
  { // Start the mDNS responder for esp8266.local
    Serial.println("Error setting up MDNS responder!");
  }
  // put your setup code here, to run once:
  pinMode(MOT_A1_PIN, OUTPUT);
  pinMode(MOT_A2_PIN, OUTPUT);

  //start server
  server.begin();
  server.setNoDelay(true);

  bufptr = &buffer[0];
  analogWriteFreq(20000); // Set PWM frequency, default 1KHz
}

void forward(int speed)
{
  digitalWrite(MOT_A1_PIN, LOW);
  analogWrite(MOT_A2_PIN, speed);
}


void backward(int speed)
{
  digitalWrite(MOT_A2_PIN, LOW);
  analogWrite(MOT_A1_PIN, speed);
}


void stop()
{
  digitalWrite(MOT_A1_PIN, LOW);
  digitalWrite(MOT_A2_PIN, LOW);
}

void processBuffer(WiFiClient *client)
{
  if (buffer[0] == 'S')
  {
    client->println("Stop");
    stop();
  }
  else if (buffer[0] == 'F')
  {
    client->printf("Forwards %d\r\n", atoi(&buffer[1]));
    forward(8 * atoi(&buffer[1]));
  }
  else if (buffer[0] == 'B')
  {
    client->printf("Backwords %d\r\n", atoi(&buffer[1]));
    backward(8 * atoi(&buffer[1]));
  }
  else
  {
    client->printf("Bad command '%s'\r\n", buffer);
  }
}


void loop()
{
  MDNS.update();
  WiFiClient client = server.available();
  if (client)
  {
    if (client.connected())
    {
      Serial.println("Connected to client");
      client.print("> ");
    }
    while (client.connected())
    {
      if (client.available() > 0)
      {
        // Read incoming message
        char inChar = client.read();
        // Send back something to the clinet
        client.print(inChar);
        if (inChar == '\r')
        {
          *bufptr = 0;
          client.println();
          processBuffer(&client);
          client.print("> ");
          bufptr = &buffer[0];
        }
        else if ((inChar == 127 || inChar == 8) && bufptr > &buffer[0])
        {
          bufptr--;
        }
        else if (inChar < ' ' || inChar >= 127)
        {
          // skip
        }
        else
        {
          *bufptr++ = inChar;
        }
        if (bufptr - &buffer[0] >= BUFLEN)
        {
          bufptr = &buffer[0];
        }
      }
    }
    Serial.println("Disconnected");
  }
}
