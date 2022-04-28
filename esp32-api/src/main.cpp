#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <FreeRTOS.h>
#include <OneWire.h>           // sensor
#include <DallasTemperature.h> // sensor

const char *SSID = "removed"; // remove
const char *PWD = "removed";  // remove
#define SENSOR_PIN 21         // ESP32 pin GIOP21 connected to DS18B20 sensor's DQ pin

void getTemperature();

// Web server running on port 80
WebServer server(80);

// temp sensor co
OneWire oneWire(SENSOR_PIN);
DallasTemperature DS18B20(&oneWire);
float tempC; // temperature in Celsius

// JSON data buffer
StaticJsonDocument<250> jsonDocument;
char buffer[250];

// env variable
float temperature;

void connectToWiFi()
{
  Serial.print("Connecting to ");
  Serial.println(SSID);

  WiFi.begin(SSID, PWD);

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
    // we can even make the ESP32 to sleep
  }

  Serial.print("Connected. IP: ");
  Serial.println(WiFi.localIP());
}
void setup_routing()
{
  server.on("/temperature", getTemperature);

  // start server
  server.begin();
}

void create_json(char *tag, float value, char *unit)
{
  jsonDocument.clear();
  jsonDocument["type"] = tag;
  jsonDocument["value"] = value;
  jsonDocument["unit"] = unit;
  serializeJson(jsonDocument, buffer);
}

void add_json_object(char *tag, float value, char *unit)
{
  JsonObject obj = jsonDocument.createNestedObject();
  obj["type"] = tag;
  obj["value"] = value;
  obj["unit"] = unit;
}
void read_sensor_data(void *parameter)
{
  for (;;)
  {
    DS18B20.requestTemperatures();      // send the command to get temperatures
    tempC = DS18B20.getTempCByIndex(0); // read temperature in °C
    temperature = tempC;
    Serial.print(tempC); // print the temperature in °C
    Serial.print("°C \n");
    Serial.println("Read sensor data");

    // delay the task
    vTaskDelay(60000 / portTICK_PERIOD_MS);
  }
}

void getTemperature()
{
  Serial.println("Get temperature");
  create_json("temperature", temperature, "°C");
  server.send(200, "application/json", buffer);
}

void getEnv()
{
  Serial.println("Get env");
  jsonDocument.clear();
  add_json_object("temperature", temperature, "°C");
  serializeJson(jsonDocument, buffer);
  server.send(200, "application/json", buffer);
}
void handlePost()
{
  if (server.hasArg("plain") == false)
  {
    // handle error here
  }
  String body = server.arg("plain");
  deserializeJson(jsonDocument, body);
  server.send(200, "application/json", "{}");
}
void setup_task()
{
  xTaskCreate(
      read_sensor_data,
      "Read sensor data",
      1000,
      NULL,
      1,
      NULL);
}
void setup()
{
  Serial.begin(9600);
  DS18B20.begin(); // initialize the DS18B20 sensor

  connectToWiFi();
  setup_task();
  setup_routing();
}

void loop()
{
  server.handleClient();
}