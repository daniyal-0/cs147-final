#include <HardwareSerial.h>
#include <Arduino.h>
#include <HttpClient.h>
#include <WiFi.h>
#include <inttypes.h>
#include <stdio.h>
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <Adafruit_AHTX0.h>
#include <string>
#include <vector>
#include <cmath>

#define fan_op_pin 26  //Fan control pin
#define RED_BUTTON_PIN 32
#define BLUE_BUTTON_PIN 33

int fan_speed = 255;   //Fan speed

int min_temp = 10;
int max_temp = 20;
int max_speed = 255;

bool red_button = false;
bool blue_button = true;

Adafruit_AHTX0 aht;
// This example downloads the URL "http://arduino.cc/"
char ssid[50]; // your network SSID (name)
char pass[50]; // your network password (use for WPA, or use
// as key for WEP)
// Name of the server we want to connect to
const char kHostname[] = "worldtimeapi.org";
// Path to download (this is the bit after the hostname in the URL
// that you want to download
const char kPath[] = "/api/timezone/Europe/London.txt";
// Number of milliseconds to wait without receiving any data before we give up
const int kNetworkTimeout = 30 * 1000;
// Number of milliseconds to wait if no data is available before trying again
const int kNetworkDelay = 1000;
std::vector<float> temperature_data;
std::vector<float> humidity_data;
const int analytics_interval = 10; 
void nvs_access()
{
  // Initialize NVS
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
      err == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    // NVS partition was truncated and needs to be erased
    // Retry nvs_flash_init
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  ESP_ERROR_CHECK(err);
  // Open
  Serial.printf("\n");
  Serial.printf("Opening Non-Volatile Storage (NVS) handle... ");
  nvs_handle_t my_handle;
  err = nvs_open("storage", NVS_READWRITE, &my_handle);
  if (err != ESP_OK)
  {
    Serial.printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
  }
  else
  {
    Serial.printf("Done\n");
    Serial.printf("Retrieving SSID/PASSWD\n");
    size_t ssid_len;
    size_t pass_len;
    err = nvs_get_str(my_handle, "ssid", ssid, &ssid_len);
    err |= nvs_get_str(my_handle, "pass", pass, &pass_len);
    switch (err)
    {
    case ESP_OK:
      Serial.printf("Done\n");
      // Serial.printf("SSID = %s\n", ssid);
      // Serial.printf("PASSWD = %s\n", pass);
      break;
    case ESP_ERR_NVS_NOT_FOUND:
      Serial.printf("The value is not initialized yet!\n");
      break;
    default:
      Serial.printf("Error (%s) reading!\n", esp_err_to_name(err));
    }
  }
  // Close
  nvs_close(my_handle);
}

void calculateAnalytics() {
  //Calculates sums, minimums, maximums, and averages, after every 10 readings
  if (temperature_data.size() < analytics_interval) return;

  float temp_sum = 0, hum_sum = 0;
  float temp_min = temperature_data[0], temp_max = temperature_data[0];
  float hum_min = humidity_data[0], hum_max = humidity_data[0];

  for (size_t i = 0; i < temperature_data.size(); i++) {
    temp_sum += temperature_data[i];
    hum_sum += humidity_data[i];
    if (temperature_data[i] < temp_min) {
        temp_min = temperature_data[i];
    }
    if (temperature_data[i] > temp_max) {
        temp_max = temperature_data[i];
    }

    if (humidity_data[i] < hum_min) {
        hum_min = humidity_data[i];
    }
    if (humidity_data[i] > hum_max) {
        hum_max = humidity_data[i];
    }
}
  
  float temp_avg = temp_sum / temperature_data.size();
  float hum_avg = hum_sum / humidity_data.size();

  Serial.println("Analytics:");
  Serial.print("Temperature - Avg: "); Serial.print(temp_avg);
  Serial.print("°C, Min: "); Serial.print(temp_min);
  Serial.print("°C, Max: "); Serial.print(temp_max); Serial.println("°C");

  Serial.print("Humidity - Avg: "); Serial.print(hum_avg);
  Serial.print("%, Min: "); Serial.print(hum_min);
  Serial.print("%, Max: "); Serial.print(hum_max); Serial.println("%");

  //Reset data
  temperature_data.clear();
  humidity_data.clear();
}

void setup() 
{
  Serial.begin(9600);          // Initialize serial communication
  pinMode(RED_BUTTON_PIN, INPUT_PULLUP);
  pinMode(BLUE_BUTTON_PIN, INPUT_PULLUP);
  pinMode(fan_op_pin, OUTPUT); // Set fan pin as output
  analogWrite(fan_op_pin, fan_speed); // Set the fan to a slow speed
  Serial.println("Fan is full speed.");

  nvs_access();
// We start by connecting to a WiFi network
  delay(1000);
  Serial.println();
  Serial.println();
  // Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  // Serial.println("WiFi connected");
  // Serial.println("IP address: ");
  // Serial.println(WiFi.localIP());
  // Serial.println("MAC address: ");
  // Serial.println(WiFi.macAddress());
  Serial.println(F("DHT sensor reading example"));
  if (!aht.begin()) {
    Serial.println("Could not find AHT");
  }
  double avg_temp = 0;
  Serial.print("calibrating temp..");
  for (int i = 0; i < 20; i++) {
    Serial.print(".");
    sensors_event_t humidity, temp;
    aht.getEvent(&humidity, &temp);
    avg_temp += temp.temperature;
    delay(200);
  }
  Serial.print("\n");
  Serial.println(avg_temp);
  avg_temp /= 20;
  min_temp = avg_temp - 5;
  max_temp = avg_temp + 5;
  Serial.println(max_temp);
}

void loop() 
{
  //Button control - increases temperature
  if (digitalRead(RED_BUTTON_PIN) == LOW) {
    min_temp += 1;
    max_temp += 1;
    Serial.println("Red button pressed. Increase temp by 1");
    Serial.print("Max temp is now ");
    Serial.println(max_temp);
    delay(1000);
  }
  
  //Button control - decreases temperature
  if (digitalRead(BLUE_BUTTON_PIN) == LOW) {
    min_temp -= 1;
    max_temp -= 1;
    Serial.println("Blue button pressed. Decrease temp by 1");
    Serial.print("Max temp is now ");
    Serial.println(max_temp);
    delay(1000);
  }

  //Read values for humidity and temp
  sensors_event_t humidity, temp;
  aht.getEvent(&humidity, &temp);
  float current_temp = temp.temperature;
  float current_humidity = humidity.relative_humidity;

  Serial.print("\nTemperature: ");
  Serial.print(current_temp);
  Serial.println("°C");
  Serial.print("Humidity: ");
  Serial.print(current_humidity);
  Serial.println("%");

  //Collect data for analytics
  temperature_data.push_back(current_temp);
  humidity_data.push_back(current_humidity);

  //Perform analytics
  if (temperature_data.size() >= analytics_interval) {
    float temp_sum = 0, hum_sum = 0;
    float temp_min = temperature_data[0], temp_max = temperature_data[0];
    float hum_min = humidity_data[0], hum_max = humidity_data[0];

    for (size_t i = 0; i < temperature_data.size(); i++) {
      temp_sum += temperature_data[i];
      hum_sum += humidity_data[i];
      temp_min = min(temp_min, temperature_data[i]);
      temp_max = max(temp_max, temperature_data[i]);
      hum_min = min(hum_min, humidity_data[i]);
      hum_max = max(hum_max, humidity_data[i]);
    }

    float temp_avg = temp_sum / temperature_data.size();
    float hum_avg = hum_sum / humidity_data.size();

    Serial.println("Analytics:");
    Serial.print("Temperature - Avg: "); Serial.print(temp_avg);
    Serial.print("°C, Min: "); Serial.print(temp_min);
    Serial.print("°C, Max: "); Serial.print(temp_max); Serial.println("°C");

    Serial.print("Humidity - Avg: "); Serial.print(hum_avg);
    Serial.print("%, Min: "); Serial.print(hum_min);
    Serial.print("%, Max: "); Serial.print(hum_max); Serial.println("%");

    //Construct and print JSON formatted payload for AWS server
    std::string terminal = "/?data={";
    terminal += "\"temp\":" + std::to_string(current_temp) + ",";
    terminal += "\"humidity\":" + std::to_string(current_humidity) + ",";
    terminal += "\"temp_avg\":" + std::to_string(temp_avg) + ",";
    terminal += "\"temp_min\":" + std::to_string(temp_min) + ",";
    terminal += "\"temp_max\":" + std::to_string(temp_max) + ",";
    terminal += "\"hum_avg\":" + std::to_string(hum_avg) + ",";
    terminal += "\"hum_min\":" + std::to_string(hum_min) + ",";
    terminal += "\"hum_max\":" + std::to_string(hum_max);
    terminal += "}";
    Serial.print("Formatted payload: ");
    Serial.println(terminal.c_str());
    
    //Send payload to AWS server
    WiFiClient c;
    HttpClient http(c);
    int err = http.get("3.138.100.131", 5000, terminal.c_str(), NULL);
    if (err == 0) {
      Serial.println("startedRequest ok");
      err = http.responseStatusCode();
      if (err >= 0) {
        Serial.print("Got status code: ");
        Serial.println(err);
        http.skipResponseHeaders();
      } else {
        Serial.print("Getting response failed: ");
        Serial.println(err);
      }
    } else {
      Serial.print("Connect failed: ");
      Serial.println(err);
    }
    http.stop();

    //Clear data for next interval
    temperature_data.clear();
    humidity_data.clear();
  }

  //Fan speed adjustment
  fan_speed = int((double(current_temp) - max_temp) / (double(min_temp) - max_temp) * max_speed);
  analogWrite(fan_op_pin, fan_speed);

  delay(1000);
}
