#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DHT.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
 
const char* SSID = "nyanet";//type your ssid
const char* PASSWORD = "manner+tinge-";//type your password
 
const int DATA_PIN = 2; // data collection pin
const int PORT = 80; // port http will be served over
const int BAUDS = 115200;
const int BOOT_PAUSE = 10; // time to pause in ms
const int FAIL_REFRESH = 500; // time in ms to  wait between checking wifi status
const int MAX_WAIT = 10000; // maximum wait time in ms before giving up and rebooting
const int MSG_DISPLAY_TIME = 1000; // time in ms to wait after displaying a message
const int MIN_WAIT_TIME = 1000; // amt of ms needed before new DHT query can be made

WiFiServer server(PORT);
 
DHT dht;
WiFiUDP ntpUDP;

NTPClient timeClient(ntpUDP);

enum class ResponseCode {
  Ok = 200,
  TooManyRequests = 429
};

struct Data {
  int response;
  float humidity, temperature;
};

void setup() {
  Serial.begin(BAUDS);
  delay(BOOT_PAUSE);
 
  // make sure our data pin is a listed input
  pinMode(DATA_PIN, INPUT);

  // use that pin
  dht.setup(DATA_PIN);

  // Connect to WiFi network 
  WiFi.begin(SSID, PASSWORD);
   
  // wait for connection
  int ms_past = 0;
  while (WiFi.status() != WL_CONNECTED) {
    // reboot if consistently failing
    if (ms_past > MAX_WAIT) {
        Serial.println("WiFi failed to connect.  Rebooting.");
        delay(MSG_DISPLAY_TIME);
        ESP.reset();
    }
    ms_past += FAIL_REFRESH;
    delay(FAIL_REFRESH);
  }

  // at this point, device is connected
  Serial.println("WiFi connected");

  // start time client
  timeClient.begin();
  timeClient.update();

  // Start the server
  server.begin();
  Serial.println("Server started");
 
  // Print the IP address
  Serial.print("Use this URL to connect: ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");

  // pause for clock 
  delay(2000);
    
}

unsigned long last_time_accessed = 0;
// this can only be used once every 2s
Data get_data() {
  if (millis() - last_time_accessed < MIN_WAIT_TIME){
    // return error
    return Data { (int)ResponseCode::TooManyRequests, 0, 0  };
  } else {
    // reset timer
    last_time_accessed = millis();
    // return data
    return Data { (int)ResponseCode::Ok, dht.getHumidity(), dht.getTemperature()  };
  }
}
 
void loop() {
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return;
  }
   
  // Wait until the client sends some data
  while(!client.available()){
    delay(1);
  }
   
  String request = client.readStringUntil('\r');
  client.flush();

  // get our humidity/temperature data
  Data data = get_data();

  // update unix epoch
  timeClient.update();

  // print headers
  client.println("HTTP/1.1 200 OK"); // TODO: correct response
  client.println("Access-Control-Allow-Origin: *");
  client.println("Content-Type: application/json");
  
  client.println(""); //  do not forget this one

  // build response string
  String res = "{";
    res += R"("status": )";
      res += data.response;
      res += ",";
    res += R"("unit": "imperial",)";
    res += R"("temperature": )";
      if (!isnan(data.temperature))
        res += dht.toFahrenheit(data.temperature);
      else
        res += "null";
      res += ",";
    res += R"("humidity": )";
      if (!isnan(data.humidity))
        res += data.humidity;
      else
        res += "null";
      res += ",";
    res += R"("time": )";
      res += timeClient.getEpochTime();
  res += "}";

  client.print(res);
    
  delay(1); // delay quirk
}
