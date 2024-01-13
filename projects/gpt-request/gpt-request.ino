/**
   BasicHTTPSClient.ino

    Created on: 14.10.2018

*/

#include <Arduino.h>
#include <ArduinoJson.h>

#include <WiFi.h>
#include <WiFiMulti.h>

#include <HTTPClient.h>

#include <WiFiClientSecure.h>

// Not sure if WiFiClientSecure checks the validity date of the certificate. 
// Setting clock just to be sure...
void setClock() {
  configTime(0, 0, "pool.ntp.org");

  Serial.print(F("Waiting for NTP time sync: "));
  time_t nowSecs = time(nullptr);
  while (nowSecs < 8 * 3600 * 2) {
    delay(500);
    Serial.print(F("."));
    yield();
    nowSecs = time(nullptr);
  }

  Serial.println();
  struct tm timeinfo;
  gmtime_r(&nowSecs, &timeinfo);
  Serial.print(F("Current time: "));
  Serial.print(asctime(&timeinfo));
}


WiFiMulti WiFiMulti;

void setup() {

  Serial.begin(115200);
  // Serial.setDebugOutput(true);

  Serial.println();
  Serial.println();
  Serial.println();

  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP("Zyxel_7E82", "DQDF8L877D");

  // wait for WiFi connection
  Serial.print("Waiting for WiFi to connect...");
  while ((WiFiMulti.run() != WL_CONNECTED)) {
    Serial.print(".");
  }
  Serial.println(" connected");

  setClock();  
}

void loop() {
  WiFiClientSecure *client = new WiFiClientSecure;
  if(client) {
    client->setInsecure();

    {
      // Add a scoping block for HTTPClient https to make sure it is destroyed before WiFiClientSecure *client is 
      HTTPClient https;
      
      Serial.print("[HTTPS] begin...\n");
      if (https.begin(*client, "https://api.openai.com/v1/chat/completions")) {  // HTTPS
        Serial.print("[HTTPS] POST...\n");
        // start connection and send HTTP header
//        int httpCode = https.GET();
        https.addHeader("Authorization", "OPENAI_API_KEY");
        https.addHeader("Content-Type", "application/json");
        int httpCode = https.POST("{\"model\": \"gpt-3.5-turbo\",\"messages\": [{\"role\": \"user\", \"content\": \"I am a robot. What are you?\"}]}");
  
        // httpCode will be negative on error
        if (httpCode > 0) {
          // HTTP header has been send and Server response header has been handled
          Serial.printf("[HTTPS] GET... code: %d\n", httpCode);
  
          // file found at server
          if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
            String payload = https.getString();
            Serial.println(payload);
            
            StaticJsonDocument<1024> doc;
            DeserializationError error = deserializeJson(doc, payload.c_str());
            if (error) {
              Serial.print(F("deserializeJson() failed: "));
              Serial.println(error.f_str());
            } else {
              const char *content = doc["choices"][0]["message"]["content"];
              Serial.println(content);
            }
          }
        } else {
          Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
        }
  
        https.end();
      } else {
        Serial.printf("[HTTPS] Unable to connect\n");
      }

      // End extra scoping block
    }
  
    delete client;
  } else {
    Serial.println("Unable to create client");
  }

  Serial.println();
  Serial.println("Waiting 10s before the next round...");
  delay(10000);
}