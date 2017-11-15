// Modified from the code by https://github.com/openhardwarelabs/bitcoin-ticker
// Maker Store http://www.makerstore.com.au
//

// Libraries
#include <ArduinoJson.h>
#include <Wire.h>
#include "SSD1306.h"

#include <WiFi.h>
#include <WiFiMulti.h>
WiFiMulti wifiMulti;

// Pins
#define SDA 5
#define SCL 4
#define I2C 0x3c

// Create display
SSD1306 display(I2C, SDA, SCL);

// Previous Bitcoin value & threshold
float previousValue = 0.0;
float threshold = 0.05;
String level = "FLAT";

// API server
const char* host = "api.coindesk.com";

void setup() {

  // Serial
  Serial.begin(115200);
  delay(10);

  // Initialize display
  display.init();
  display.flipScreenVertically();
  display.clear();
  display.display();

  // We start by connecting to a WiFi network
  wifiMulti.addAP("PRIVATE_NETWORK", "bakkenson");

  display.setLogBuffer(5, 30);
  display.println("Connecting Wifi...");
  display.drawLogBuffer(0, 0);
  display.display();

  if (wifiMulti.run() == WL_CONNECTED) {
    display.println("WiFi connected");
    display.println("IP address: ");
    display.println(WiFi.localIP());
    display.drawLogBuffer(0, 0);
    display.display();
  }

  if (wifiMulti.run() != WL_CONNECTED) {
    display.println("WiFi not connected!");
    display.clear();
    display.drawLogBuffer(0, 0);
    display.display();
    delay(1000);
  }

}

void loop() {

  // Connect to API
  Serial.print("connecting to ");
  Serial.println(host);

  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }

  // We now create a URI for the request
  String url = "/v1/bpi/currentprice.json";

  Serial.print("Requesting URL: ");
  Serial.println(url);

  // This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");
  delay(100);

  // Read all the lines of the reply from server and print them to Serial
  String answer;
  while (client.available()) {
    String line = client.readStringUntil('\r');
    answer += line;
  }

  client.stop();
  Serial.println();
  Serial.println("closing connection");

  // Process answer
  // Serial.println();
  // Serial.println("Answer: ");
  // Serial.println(answer);

  // Convert to JSON
  String jsonAnswer;
  int jsonIndex;

  for (int i = 0; i < answer.length(); i++) {
    if (answer[i] == '{') {
      jsonIndex = i;
      break;
    }
  }

  // Get JSON data
  jsonAnswer = answer.substring(jsonIndex);
  //  Serial.println();
  // Serial.println("JSON answer: ");
  // Serial.println(jsonAnswer);
  jsonAnswer.trim();

  // Get rate as float
  int rateIndex = jsonAnswer.indexOf("rate_float");
  String priceString = jsonAnswer.substring(rateIndex + 12, rateIndex + 19);
  priceString.trim();
  float price = priceString.toFloat();

  //SAB Get update time
  int updatedIndex = jsonAnswer.indexOf("updated");
  String updatedString = jsonAnswer.substring(updatedIndex + 10, updatedIndex + 31);
  updatedString.trim();
  //  float price = priceString.toFloat();

  // Init previous value
  if (previousValue == 0.0) {
    previousValue = price;
    level = "FLAT";
  }

  // Alert down ?
  if (price < (previousValue - threshold)) {
    level = "DOWN";
  }

  // Alert up ?
  if (price > (previousValue + threshold)) {
    level = "UP";
  }

  // Print price
  Serial.println();
  Serial.println("Bitcoin price: ");
  Serial.println(price);
  Serial.println("Time: ");
  Serial.println(updatedString);
  Serial.println("Level: ");
  Serial.println(level);

  // Display on OLED
  if (price != 0) {
    display.clear();
    display.setFont(ArialMT_Plain_10);
    display.drawString(0, 0, level);
    display.setFont(ArialMT_Plain_24);
    display.drawString(26, 20, priceString);
    display.setFont(ArialMT_Plain_10);
    display.drawString(0, 52, updatedString);

    display.display();

  }

  // Store value
  previousValue = price;

  // Wait 65 seconds
  delay(65000);
}
