/*
  ESP8266 --> ThingSpeak Channel via MKR1000 Wi-Fi

  This sketch sends the Wi-Fi Signal Strength (RSSI) of an ESP8266 to a ThingSpeak
  channel using the ThingSpeak API (https://www.mathworks.com/help/thingspeak).

  Requirements:

     ESP8266 Wi-Fi Device
     Arduino 1.6.9+ IDE
     Additional Boards URL: http://arduino.esp8266.com/stable/package_esp8266com_index.json
     Library: esp8266 by ESP8266 Community

  ThingSpeak Setup:

     Sign Up for New User Account - https://thingspeak.com/users/sign_up
     Create a new Channel by selecting Channels, My Channels, and then New Channel
     Enable one field
     Note the Channel ID and Write API Key

  Setup Wi-Fi:
    Enter SSID
    Enter Password

  Tutorial: http://nothans.com/measure-wi-fi-signal-levels-with-the-esp8266-and-thingspeak

  Created: Feb 1, 2017 by Hans Scharler (http://nothans.com)
*/
#include<EthernetClient.h>
#include <ESP8266WiFi.h>
#include "settings.h"

WiFiClient client;

// ThingSpeak Settings
String server = "api.thingspeak.com";
String fields[] = {"field1", "field2", "field3", "field4", "field5", "field5"};

const unsigned long int heartbeatInterval = 20 * 1000; // heartbeat every 20 seconds
unsigned long lastUpdateTime = 0; // Track the last update time using millis()

unsigned char current_pin_state[] = {0, 0, 0, 0};
unsigned char previous_pin_state[] = {0, 0, 0, 0};

int reboot = 1;

void setup() {
  pinMode(0, INPUT);
  pinMode(1, INPUT);
  pinMode(2, INPUT);
  pinMode(3, INPUT);

  //Serial.begin(9600);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
}

void loop() {

  char ifDataChanged = 0;

  if (millis() - lastUpdateTime >=  heartbeatInterval) {
    String jsonBuffer = ",\"updates\":[{\"delta_t\":\"0\""; //start data set
    for (int i = 0; i < 4; i++) {
      current_pin_state[i] = digitalRead(i);
      if (current_pin_state[i] != previous_pin_state[i]) {
        ifDataChanged = 1;
        jsonBuffer += ",\"" + fields[i] + "\":" + String(previous_pin_state[i]);
      }
    }

    if (ifDataChanged) {
      jsonBuffer += ",\"field5\":0},{\"delta_t\":\"2\""; //start second data set
      for (int i = 0; i < 4; i++) {
        if (current_pin_state[i] != previous_pin_state[i]) {
          jsonBuffer += ",\"" + fields[i] + "\":" + String(current_pin_state[i]);
        }
        previous_pin_state[i] = current_pin_state[i];
      }
      
      if (reboot) { // this would toggle every time there's a board reboot
        jsonBuffer += ",\"field7\":1";
        reboot = 0;
      }
      
      jsonBuffer += ",\"field6\":";
      
      int nRetries = 0;
      int resp = 0;
      do {
      jsonBuffer.remove(jsonBuffer.lastIndexOf(",\"field6\":"));
      jsonBuffer += ",\"field6\":" + String(nRetries);
      jsonBuffer += "}]";
      httpRequest(jsonBuffer);
      delay(250); //Wait to receive the response
      client.parseFloat();
      resp = client.parseInt();
      //Serial.println("Server responded "+String(resp));
      nRetries++;
      } while (resp != 202);
    }
    lastUpdateTime = millis(); // Update the last update time
  }
}

void httpRequest(String jsonBuffer) {
  /* JSON format for data buffer in the API
      This example uses the relative timestamp as it uses the "delta_t". If your device has a real-time clock, you can also provide the absolute timestamp using the "created_at" parameter
      instead of "delta_t".
       "{\"write_api_key\":\"YOUR-CHANNEL-WRITEAPIKEY\",\"updates\":[{\"delta_t\":0,\"field1\":-60},{\"delta_t\":15,\"field1\":200},{\"delta_t\":15,\"field1\":-66}]
  */
  // Format the data buffer as noted above
  String data = "{\"write_api_key\":\"" + writeAPIKey + "\""; //Replace YOUR-CHANNEL-WRITEAPIKEY with your ThingSpeak channel write API key
  data += jsonBuffer;
  data += "}";
  // Close any connection before sending a new request
  client.stop();
  int data_length = data.length() + 1; //Compute the data buffer length
  // POST data to ThingSpeak
  if (client.connect(server, 80)) {
    client.println("POST /channels/" + channelID + "/bulk_update.json HTTP/1.1"); //Replace YOUR-CHANNEL-ID with your ThingSpeak channel ID
    client.println("Host: api.thingspeak.com");
    client.println("User-Agent: mw.doc.bulk-update (Arduino ESP8266)");
    client.println("Connection: close");
    client.println("Content-Type: application/json");
    client.println("Content-Length: " + String(data_length));
    client.println();
    char bufChar[500]; // if data is not converted to Char Array, then the encoding is not correct; server cannot read the API key and returns 401 - unauthorized
    data.toCharArray(bufChar, data_length);
    client.println(bufChar);
    //Serial.println(bufChar);
  }
}


