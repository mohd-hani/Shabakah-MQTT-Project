#include "Adafruit_NeoPixel.h" // Library for controlling RGB LED strips
#include "Remal_SHT3X.h" // Library for temperature and humidity sensor
#include <WiFi.h> // Library for WiFi connection
#include <PubSubClient.h> // Library for MQTT communication
#include "WiFiCredentials.h"
#include "MQTTCredentials.h"

/* RGB LED Configuration */
#define NUM_LEDS 1 // Number of LEDs in the strip

/* Temperature and Humidity Sensor Configuration */
#define SHT3X_ADD 0x44 // I2C address for SHT3X sensor

/*
  RGB LED setup
*/
// LED 1 configuration
int brightness1 = 50; // Initial brightness for LED 1
int hue1 = 0; // Initial hue for LED 1
Adafruit_NeoPixel led1(NUM_LEDS, RGB_LED_1_PIN, NEO_GRB + NEO_KHZ800); // Define LED 1 with parameters

// LED 2 configuration
int brightness2 = 50; // Initial brightness for LED 2
int hue2 = 0; // Initial hue for LED 2
Adafruit_NeoPixel led2(NUM_LEDS, RGB_LED_2_PIN, NEO_GRB + NEO_KHZ800); // Define LED 2 with parameters

/*
  Temperature and Humidity Sensor setup
*/
SHT3x tempHumSensor(SHT3X_ADD); // Initialize SHT3X sensor with its I2C address
float temp = 0; // Variable to store temperature value
float humidity = 0; // Variable to store humidity value

/*
  WiFi setup
*/
static WiFiClient wifiClient; // Create a WiFi client object
static PubSubClient mqttClient(wifiClient); // Create an MQTT client object

/*
  Function to reconnect to the MQTT server if the connection is lost
*/
void MQTTHandlerReconnect()
{
  while(!mqttClient.connected())
  {
    Serial.println("Attempting MQTT connection");

    String clientId = "ShabakahClient-";
    clientId += String(random(0xffff), HEX); // Generate a random client ID

    if(mqttClient.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD))
    {
      Serial.println("Connected to MQTT server");
      mqttClient.subscribe(SUBSCRIBE_TOPIC); // Subscribe to LED control topic
    }
    else
    {
      Serial.println("Failed to connect to MQTT server, retrying");
      delay(2000); // Wait 2 seconds before retrying
    }
  }
}

/*
  MQTT callback function to handle incoming messages
*/
void MQTTHandlerCallback(char* topic, byte* payload, unsigned int length)
{
  char payloadData[length+1] = {0}; // Buffer to store payload data

  for(int i = 0; i < length; i++)
  {
    payloadData[i] = (char)payload[i]; // Copy payload data into buffer
    payloadData[i+1] = '\0'; // Null-terminate the string
  }

  // Set LED color based on received message
  if(strcmp(payloadData, "red") == 0)
  {
    led1.setPixelColor(0,255,0,0); // Set LED 1 to red
    led1.show(); // Update LED 1 display
    led2.setPixelColor(0,255,0,0); // Set LED 2 to red
    led2.show(); // Update LED 2 display
    delay(500);
  }
  else if(strcmp(payloadData, "green") == 0)
  {
    led1.setPixelColor(0,0,255,0); // Set LED 1 to green
    led1.show(); // Update LED 1 display
    led2.setPixelColor(0,0,255,0); // Set LED 2 to green
    led2.show(); // Update LED 2 display
    delay(500);
  }
  else if(strcmp(payloadData, "blue") == 0)
  {
    led1.setPixelColor(0,0,0,255); // Set LED 1 to blue
    led1.show(); // Update LED 1 display
    led2.setPixelColor(0,0,0,255); // Set LED 2 to blue
    led2.show(); // Update LED 2 display
    delay(500);
  }
  else
  {
    led1.setPixelColor(0,0,0,0); // Turn off LED 1
    led1.show(); // Update LED 1 display
    led2.setPixelColor(0,0,0,0); // Turn off LED 2
    led2.show(); // Update LED 2 display
    delay(500);
  }
}

void setup()
{
  // Initialize serial communication for debugging
  Serial.begin();
  Serial.setTxTimeoutMs(0);

  // Initialize button inputs
  pinMode(SHBK_BTN_1, INPUT_PULLUP);
  pinMode(SHBK_BTN_2, INPUT_PULLUP);

  // Initialize RGB LEDs
  // LED 1 setup
  led1.begin();
  led1.setBrightness(brightness1); // Set initial brightness for LED 1
  led1.clear(); // Clear LED 1 display
  led1.show(); // Update LED 1 display
  // LED 2 setup
  led2.begin();
  led2.setBrightness(brightness2); // Set initial brightness for LED 2
  led2.clear(); // Clear LED 2 display
  led2.show(); // Update LED 2 display

  // Initialize temperature and humidity sensor
  tempHumSensor.Initialize(); // Initialize the sensor
  tempHumSensor.SetRepeatability(e_high); // Set measurement repeatability
  if(!tempHumSensor.IsConnected())
  {
    Serial.println("Error: Could not initialize temperature and humidity sensor");
  }

  // Connect to WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while(WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Connecting to WiFi");
    delay(500); // Wait 500ms before retrying
  }
  Serial.println("Connected to WiFi");

  // Set up MQTT client
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT); // Set MQTT server and port
  mqttClient.setCallback(MQTTHandlerCallback); // Set callback function for MQTT messages

  // Initialize LED colors
  led1.setPixelColor(0,0,255,0); // Set LED 1 to green
  led1.show(); // Update LED 1 display

  led2.setPixelColor(0,0,255,0); // Set LED 2 to green
  led2.show(); // Update LED 2 display
}

void loop()
{
  // Check WiFi connection and reconnect if disconnected
  if(WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Disconnected from WiFi, attempting to reconnect");
    WiFi.reconnect();
    while(WiFi.status() != WL_CONNECTED)
    {
      Serial.println("Reconnecting to WiFi");
      delay(500); // Wait 500ms before retrying
    }
    Serial.println("Reconnected to WiFi");
  }

  // Check MQTT connection and reconnect if disconnected
  if(!mqttClient.connected())
  {
    Serial.println("Attempting to connect to MQTT server");
    MQTTHandlerReconnect(); // Reconnect to MQTT server
  }
  mqttClient.loop(); // Process incoming MQTT messages

  // Button 1 pressed: send humidity data via MQTT
  if(!digitalRead(SHBK_BTN_1))
  {
    Serial.println("Button 1 pressed");
    delay(100); // Debounce delay

    humidity = tempHumSensor.GetHumidity(); // Get current humidity

    snprintf(message, sizeof(message), "The current humidity is %.2f RH", humidity); // Format message

    mqttClient.publish(PUBLISH_TOPIC, message); // Publish message to MQTT topic
  }

  // Button 2 pressed: send temperature data via MQTT
  if(!digitalRead(SHBK_BTN_2))
  {
    Serial.println("Button 2 pressed");
    delay(100); // Debounce delay

    temp = tempHumSensor.GetTemperatureCelsius(); // Get current temperature

    snprintf(message, sizeof(message), "The current temperature is %.2f degC", temp); // Format message

    mqttClient.publish(PUBLISH_TOPIC, message); // Publish message to MQTT topic
  }

  delay(1); // Small delay to prevent overwhelming the loop
}