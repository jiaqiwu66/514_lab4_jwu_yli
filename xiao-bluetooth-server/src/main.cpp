#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <stdlib.h>
#include <NewPing.h>

BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
unsigned long previousMillis = 0;
const long interval = 1000;

#define TRIGGER_PIN D8
#define ECHO_PIN D9
#define MAX_DISTANCE 400

NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);

// TODO: add new global variables for your sensor readings and processed data
#define WINDOW_SIZE 10 // Size of the moving average window

unsigned int distanceReadings[WINDOW_SIZE]; // Array to store the last few readings
unsigned long sum = 0;                      // Sum of the readings in the window
unsigned int average = 0;                   // Average (filtered) value

// TODO: Change the UUID to your own (any specific one works, but make sure they're different from others'). You can generate one here: https://www.uuidgenerator.net/
#define SERVICE_UUID "56f9819c-fd94-44d4-9d3c-156a6b86279b"
#define CHARACTERISTIC_UUID "0900f701-d655-4987-ab06-ff880acdb3c7"

class MyServerCallbacks : public BLEServerCallbacks
{
  void onConnect(BLEServer *pServer)
  {
    deviceConnected = true;
  };

  void onDisconnect(BLEServer *pServer)
  {
    deviceConnected = false;
  }
};

// TODO: add DSP algorithm functions here

void setup()
{
  Serial.begin(9600);
  Serial.println("Starting BLE work!");
  memset(distanceReadings, 0, sizeof(distanceReadings)); // Initialize the array

  // TODO: add codes for handling your sensor setup (pinMode, etc.)
  // pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  // pinMode(echoPin, INPUT);  // Sets the echoPin as an Input

  // TODO: name your device to avoid conflictions
  BLEDevice::init("JIAQI");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_WRITE |
          BLECharacteristic::PROPERTY_NOTIFY);
  pCharacteristic->addDescriptor(new BLE2902());
  pCharacteristic->setValue("jwu");
  pService->start();
  // BLEAdvertising *pAdvertising = pServer->getAdvertising();  // this still is working for backward compatibility
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06); // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial.println("Characteristic defined! Now you can read it in your phone!");
}

void loop()
{
  // TODO: add codes for handling your sensor readings (analogRead, etc.)
  // TODO: use your defined DSP algorithm to process the readings
  unsigned int distance = sonar.ping_cm(); // Get new reading

  sum -= distanceReadings[WINDOW_SIZE - 1]; // Subtract the oldest reading from sum
  // Shift all the readings, discard the oldest one
  for (int i = WINDOW_SIZE - 1; i > 0; i--)
  {
    distanceReadings[i] = distanceReadings[i - 1];
  }

  distanceReadings[0] = distance; // Add the new reading to the array
  sum += distance;                // Add the new reading to the sum

  average = sum / WINDOW_SIZE; // Calculate the moving average

  // Print raw and filtered data
  Serial.print("Ping: ");
  Serial.print(distance); // Raw data
  Serial.print(" cm, Filtered: ");
  Serial.print(average); // Filtered data
  Serial.println(" cm");

  if (deviceConnected)
  {
    // Send new readings to database
    // TODO: change the following code to send your own readings and processed data
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval)
    {
      if (average < 30)
      {
        pCharacteristic->setValue(average);
        pCharacteristic->notify();
        Serial.println("Notify value: "+String(average));
      }
    }
  }
  // disconnecting
  if (!deviceConnected && oldDeviceConnected)
  {
    delay(500);                  // give the bluetooth stack the chance to get things ready
    pServer->startAdvertising(); // advertise again
    Serial.println("Start advertising");
    oldDeviceConnected = deviceConnected;
  }
  // connecting
  if (deviceConnected && !oldDeviceConnected)
  {
    // do stuff here on connecting
    oldDeviceConnected = deviceConnected;
  }

  delay(100);
}