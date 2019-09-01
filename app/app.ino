//////////////////////////////////////////////////////////////////
/// DOIT ESP32 plant watering system

////////////////////////////////////////////////////////////////////
#define SYSTEM_ID "doit_esp32_devkit_v1_01"
#define ROOM "andrews_room"

#define PUMP_PIN 12
#define BAUD_RATE 115200
#define LOOP_DELAY_SECS 600
#define PUMP_DELAY_MILLI 100

#define MQTT_SERVER "postman.cloudmqtt.com"
#define MQTT_PORT 11968
#define MQTT_USER "xxtdtmwf"
#define MQTT_PASSWORD "c-0_VSx4qaOv"
#define MQTT_SERIAL_PUBLISH_TEST "test"
#define MQTT_SERIAL_PUBLISH_PLANTS "plants/berlin/oderstrasse/andrew"
#define MQTT_SERIAL_PUBLISH_CPU "things/esp32"
#define MQTT_SERIAL_PUBLISH_PLACE "places/berlin/oderstrasse/andrew"

#define uS_TO_S_FACTOR 1000000LL 
#define DEEP_SLEEP_MINUTES 59
unsigned long DEEPSLEEP_SECONDS = DEEP_SLEEP_MINUTES * 60;

#include <WiFi.h>
#include <PubSubClient.h>
#include <ESP.h>
#include "esp_deep_sleep.h"

WiFiClient wifiClient;
const char *ssid = "(Don't mention the war)";
const char *password = "56939862460419967485";

char deviceid[21];
PubSubClient mqttClient(wifiClient);


class Module
{
public:
        Module(char, char *, int, int, int, int, int, int, int, bool);
        char id;
        char *plantType;
        int moistureSettingLow;
        int moistureSettingHigh;
        int sensorLowerValue;
        int sensorUpperValue;
        int readPin;
        int currentPercentage;
        int servoPin;
        bool isPumping;
};

/// Constructor for each module
Module::Module(char a, char *b, int c, int d, int e, int f, int g, int h, int i, bool j)
{
        id = a;
        plantType = b;
        readPin = c;
        servoPin = d;
        moistureSettingLow = e;
        moistureSettingHigh = f;
        sensorLowerValue = g;
        sensorUpperValue = h;
        currentPercentage = i;
        isPumping = j;
}

#define MODULE_COUNT 1
Module modules[MODULE_COUNT] = {
          // . ID .  TYPE .   RP. SEV.MH. ML. SL   .SH . % .  PUMP
        Module('1', "Unknown", 13, 5, 40, 70, 859, 456, 0, false)
        // Module('2', "Unknown", 34, 5, 40, 70, 859, 456, 0, false),
        // Module('3', "Unknown", 35, 5, 40, 70, 859, 456, 0, false),
        // Module('4', "Unknown", 36, 5, 40, 70, 859, 456, 0, false),
        // Module('5', "Unknown", 32, 5, 40, 70, 859, 456, 0, false),
        // Module('6', "Unknown", 33, 5, 40, 70, 859, 456, 0, false),
        // Module('7', "Unknown", 25, 5, 40, 70, 859, 456, 0, false),
        // Module('8', "Unknown", 36, 5, 40, 70, 859, 456, 0, false),
};

bool isSleeping = false;

// Time in milliseconds
//const unsigned longLOOP_DELAY_SECS = 1;

void setup()
{
    Serial.begin(BAUD_RATE);
    // Initialise pins
    for (int i = 0; i < (MODULE_COUNT); i++)
    {
        Serial.print("Pin ");
        Serial.print(i);
        pinMode(i, OUTPUT);
        Serial.print(" is set to OUTPUT\n");
        digitalWrite(i, HIGH);
        modules[i].isPumping = false;

        delay(100);
    }
    pinMode(PUMP_PIN, OUTPUT);
    digitalWrite(PUMP_PIN, LOW);
    Serial.println("Finished setup");
}

void loop()
{

//     Serial.print("The moisture at pin D13 is: ");
//     Serial.println(analogRead(13));
//     delay(1000);
    checkPlants();
    if (Serial.available() > 0)
    {
        recvWithStartEndMarkers();
    }
}

void handleMessage(char *message)
{
    Serial.print("In handle message with themessage ");
    Serial.println(message);
    if (message == "SLEEP")
    {
        isSleeping = true;
    }
    if (message == "WAKE")
    {
        isSleeping = false;
    }
}



char *recvWithStartEndMarkers()
{
    const byte numChars = 255;
    char receivedChars[numChars];
    boolean newData = false;
    static boolean recvInProgress = false;
    static byte ndx = 0;
    char startMarker = '<';
    char endMarker = '>';
    char rc;

    while (Serial.available() > 0 && newData == false)
    {
        rc = Serial.read();

        if (recvInProgress == true)
        {
            if (rc != endMarker)
            {
                receivedChars[ndx] = rc;
                ndx++;
                if (ndx >= numChars)
                {
                        ndx = numChars - 1;
                }
            }
            else
            {
                receivedChars[ndx] = '\0'; // terminate the string
                recvInProgress = false;
                ndx = 0;
                newData = true;
            }
        }

        else if (rc == startMarker)
        {
                recvInProgress = true;
        }
    }
    Serial.print("In recieved chars with themessage ");
    Serial.println(receivedChars);
    if (strcmp(receivedChars, "SLEEP") == 0)
    {
            isSleeping = true;
            Serial.println("The module is ALSEEP");
    }
    if (strcmp(receivedChars, "WAKE") == 0)
    {
            isSleeping = false;
            Serial.println("The module is AWAKE");
    }
}

int convertToPercent(int sensorValue, Module module)
{
    int percentValue = 0;
    percentValue = map(sensorValue, module.sensorLowerValue, module.sensorUpperValue, 0, 100);
    return percentValue;
}

void printSetting(int setting)
{
    Serial.print("Setting: ");
    Serial.print(setting);
    Serial.println("%");
}

void printId(String id)
{
    Serial.print(" - Plant ");
    Serial.println(id);
}

void printValueToSerial(int currentPercentage)
{
    Serial.print(" - Moisture Percent: ");
    Serial.print(currentPercentage);
    Serial.println("%");
}


void checkPlants()
{
  char finalString[400];

  bool needsPump = false;

  for (int i = 0; i < MODULE_COUNT; i++)
  {
    Module currentModule = modules[0];

    strcpy(finalString, "plant_system,city=Berlin,location=oderstrasse,room=");

    strcat(finalString, ROOM);

    strcat(finalString, ",thing=");

    strcat(finalString, SYSTEM_ID);

    strcat(finalString, ",plnt_typ=");

    strcat(finalString, currentModule.plantType);

    strcat(finalString, ",plnt_id=");

    char z[20];
    snprintf(z, 20, "%c", currentModule.id);
    strcat(finalString, z);

    strcat(finalString, " ");

    //Fields
    strcat(finalString, "serv_pin=");
    char a[20];
    snprintf(a, 20, "%ld", currentModule.servoPin);
    strcat(finalString, a);

    strcat(finalString, ",read_pin=");
    char b[20];
    snprintf(b, 20, "%ld", currentModule.readPin);
    strcat(finalString, b);

    strcat(finalString, ",sns_reading=");
    char c[20];
    snprintf(c, 20, "%ld", analogRead(currentModule.readPin));
    strcat(finalString, c);

    strcat(finalString, ",moi_set_high=");
    char d[20];
    snprintf(d, 20, "%ld", currentModule.moistureSettingHigh);
    strcat(finalString, d);

    strcat(finalString, ",moi_set_low=");
    char e[20];
    snprintf(e, 20, "%ld", currentModule.moistureSettingLow);
    strcat(finalString, e);

    strcat(finalString, ",sns_low_value=");
    char f[20];
    snprintf(f, 20, "%ld", currentModule.sensorLowerValue);
    strcat(finalString, f);

    strcat(finalString, ",sns_high_value=");
    char g[20];
    snprintf(g, 20, "%ld", currentModule.sensorUpperValue);
    strcat(finalString, g);


    // TODO;
    strcat(finalString, ",dev_id=");
    strcat(finalString, deviceid);

    currentModule.currentPercentage = convertToPercent(analogRead(currentModule.readPin), currentModule);

    strcat(finalString, ",moi_lvl=");
    char m[20];
    snprintf(m, 20, "%ld", currentModule.currentPercentage);
    strcat(finalString, m);

    if (currentModule.currentPercentage < currentModule.moistureSettingLow)
    {
      currentModule.isPumping = true;
      needsPump = true;
      digitalWrite(currentModule.servoPin, LOW);

      //Opening servo
      strcat(finalString, ",ded_zn=0");
    }
    if (currentModule.currentPercentage >= currentModule.moistureSettingLow && currentModule.currentPercentage <= currentModule.moistureSettingHigh)
    {
      if (!currentModule.isPumping)
      {
        digitalWrite(currentModule.servoPin, HIGH);
      }

      //the deadzone
      strcat(finalString, ",ded_zn=1");
    }
    if (currentModule.currentPercentage > currentModule.moistureSettingHigh)
    {
      currentModule.isPumping = false;
      digitalWrite(currentModule.servoPin, HIGH);

      //Opening servo
      strcat(finalString, ",ded_zn=0");
    }

        byte servoPinState = digitalRead(currentModule.servoPin);
        if (servoPinState == LOW)
        {
            strcat(finalString, ",servo=1");
        }
        else
        {
            strcat(finalString, ",servo=0");
        }
    
        byte pumpPinState = digitalRead(PUMP_PIN);
    
        if (pumpPinState == LOW)
        {
            strcat(finalString, ",pump=0");
        }
        else
        {
            strcat(finalString, ",pump=1");
        }
    delay(100);
    Serial.println(finalString);
    mqttClient.publish(MQTT_SERIAL_PUBLISH_PLANTS, finalString);

    // End of loop
  }

        if (needsPump)
        {
            digitalWrite(PUMP_PIN, HIGH);
            delay(PUMP_DELAY_MILLI);
        }
        else
        {
            digitalWrite(PUMP_PIN, LOW);
            int p = 0;
            while (p < LOOP_DELAY_SECS)
            {
                delay(1000);
                p++;
            }
        }
}
