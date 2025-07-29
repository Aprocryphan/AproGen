#include "animation_frames.h"
#include "arduino_secrets.h"
#include "remote_website.h"
#include "logo.h"
#include <math.h> // For math functions like sin()
#include <stdlib.h> // For random number generation
#include <ESPmDNS.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <WiFi.h> // For WiFi communication
#include <AsyncUDP.h> // For UDP network communication
#include <NTPClient.h> // For NTP time sync
#include <time.h>
#include "DHT.h" // For DHT11 sensor
#include <SPI.h> // For SPI peripheral communication
#include <SD.h> // For SD management
#include <FS.h> // For file system management
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h> // Special library for LED Matrix driving
#include <Adafruit_VS1053.h> // For MP3 player chip
#include <ESPAsyncWebServer.h> // For web server
// Use native watchdog

// For tests
#include <Wire.h> // For SCL & SDA communication
#include <Adafruit_GFX.h> // For OLED Monitor
#include <Adafruit_SSD1306.h> // For OLED Monitor
#include <string.h>

// Left Side
const int SPI_SCK = D13;
const int IR_SENSOR_PIN = A0;
const int R1_PIN = A1;
const int B1_PIN = A2;
const int R2_PIN = A3;
//const int B2_PIN = A4;
//const int A_PIN = A5;
const int C_PIN = A6;
const int CLK_PIN = A7;

// Right Side
const int SPI_CIPO_MISO = D12;
const int SPI_COPI_MOSI = D11;
const int SPI_CS_SD = D10;
const int SPI_CS_VS1053_CHIP = D9;
const int SPI_CS_VS1053_CMD = D8;
const int SPI_VS1053_DREQ = D7;
const int DHT_PIN = D6; // Digital pin connected to the DHT sensor inside the helmet
const int G1_PIN = D5;
//const int G2_PIN = D4;
//const int B_PIN = D3;
const int D_PIN = D2;
const int LAT_PIN = D1;
const int OE_PIN = D0;

/*
HUB75_I2S_CFG::i2s_pins _pins={R1_PIN, G1_PIN, B1_PIN, R2_PIN, G2_PIN, B2_PIN, A_PIN, B_PIN, C_PIN, D_PIN, E_PIN, LAT_PIN, OE_PIN, CLK_PIN};
HUB75_I2S_CFG mxconfig(
	64, // Module width
	32, // Module height
	2, // chain length
	_pins, // pin mapping
);
dma_display = new MatrixPanel_I2S_DMA(mxconfig);
*/

const int CORE_0 = 0;
const int CORE_1 = 1;
const int STACK_SIZE_1 = 2048;
const int STACK_SIZE_2 = 4096;
const int STACK_SIZE_3 = 8192;
const int STACK_SIZE_4 = 16384;

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define DHT_TYPE DHT11 // The type of DHT sensor
DHT dht(DHT_PIN, DHT_TYPE); // Create a DHT object for the sensor
Adafruit_SSD1306 displayLeft(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); // Declares the OLED display
//Adafruit_SSD1306 displayRight(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire1, OLED_RESET); // Declares the OLED display
AsyncWebServer WebServer(8080); // Defines the port that the web server is hosted on
WiFiClient WebClient; // Declare DataClient globally
AsyncWebServer DataServer(8081); // Defines the port that the data server is hosted on
WiFiClient DataClient; // Declare DataClient globally
WiFiUDP ntpUDP; // A UDP instance to let the NTPClient communicate over
NTPClient timeClient(ntpUDP); // Declare NTPClient object
unsigned long unixTime; // Variable to hold unix time fetched from NTP server
File myFile;
const char* ntpServer = "pool.ntp.org"; // NTP server
const long gmtOffset_sec = 0;
const int daylightOffset_sec = 3600;

// -------------------------------------------------
BLEAdvertising *pAdvertising; // Pointer to the advertising object
// Example status variables (needs proper sharing mechanism later!)
volatile uint8_t currentEmotionCode = 0; // 0: Idle, 1: Happy, 2: Blush etc.
volatile bool proximityDetected = false;
// -------------------------------------------------

// Define Manufacturer ID and Protogen Identifier
#define MANUFACTURER_ID 0xFFFF
#define PROTOGEN_ID_1 0x50 // 'P'
#define PROTOGEN_ID_2 0x52 // 'R'

const int requestCooldown = 2000;
unsigned long lastRequestTime = 0;
int blinkCycles = 0;
int eyeFrame = 0; // eyeFrame counter for animation
int mouthFrame = 0; // mouthFrame counter for animation
int noseFrame = 0; // noseFrame counter for animation
int taskManPrevMillis = 0; // For task manager debug
int network = -1; // Initial network connection attempt for switch case
bool eyeReverse = false;
bool mouthReverse = false;

char serialOutputBuffer [256]; // Buffer for serial output
char localIP[21] = "null"; // XXX.XXX.XXX.XXX\0
char subnetMask[21] = "null"; // XXX.XXX.XXX.XXX\0
char gatewayIP[21] = "null"; // XXX.XXX.XXX.XXX\0

String url = "null";
String referrer = "null";
String request = "null";

// 8192 Stack size for loop

// Create task handle for multi core processing
TaskHandle_t animationLoopDemoTask;
TaskHandle_t animationI2SBuffererTask;
TaskHandle_t mp3PlayerLoopTask;
TaskHandle_t animationLogicTask;
TaskHandle_t dhtProtectionTask;
TaskHandle_t failsafesLoopTask;
TaskHandle_t protoBeaconTask;
TaskHandle_t ntpSyncLoopTask;
TaskHandle_t protoRemoteTask;
TaskHandle_t taskManagerLoopTask;

void flipBitmapVertical(const uint8_t *source, uint8_t *destination, int width, int height) {
  for (int x = 0; x < width; x++) {
    for (int y = 0; y < height; y++) {
      int originalIndex = y * width + x;
      int flippedIndex = (height - 1 - y) * width + x;
      destination[flippedIndex] = source[originalIndex];
    }
  }
}

// NetworkChange, If the network disconnects, it reconnects to another predefined network
void NetworkChange() {
  while (WiFi.status() != WL_CONNECTED) {
    network = (network + 1) % 3;
    switch(network) {
      case 0:
        WiFi.begin(SECRET_SSID, SECRET_OPTIONAL_PASS);
        Serial.println("Connected To Home Network.");
        displayLeft.setTextSize(2);
        displayLeft.setCursor(0, 30);
        displayLeft.write(0xAE);
        displayLeft.display();
        strncpy(localIP, WiFi.localIP().toString().c_str(), sizeof(localIP) - 1);
        localIP[sizeof(localIP) - 1] = '\0'; // Ensure null termination
        strncpy(subnetMask, WiFi.subnetMask().toString().c_str(), sizeof(subnetMask) - 1);
        subnetMask[sizeof(subnetMask) - 1] = '\0'; // Ensure null termination
        strncpy(gatewayIP, WiFi.gatewayIP().toString().c_str(), sizeof(gatewayIP) - 1);
        gatewayIP[sizeof(gatewayIP) - 1] = '\0'; // Ensure null termination
        vTaskDelay(pdMS_TO_TICKS(3000));
        break;
      case 1:
        WiFi.begin(SECRET_SSID_2, SECRET_OPTIONAL_PASS_2);
        Serial.println("Connected To Mobile Network.");
        displayLeft.setTextSize(2);
        displayLeft.setCursor(0, 30);
        displayLeft.write(0xAF);
        displayLeft.display();
        strncpy(localIP, WiFi.localIP().toString().c_str(), sizeof(localIP) - 1);
        localIP[sizeof(localIP) - 1] = '\0'; // Ensure null termination
        strncpy(subnetMask, WiFi.subnetMask().toString().c_str(), sizeof(subnetMask) - 1);
        subnetMask[sizeof(subnetMask) - 1] = '\0'; // Ensure null termination
        strncpy(gatewayIP, WiFi.gatewayIP().toString().c_str(), sizeof(gatewayIP) - 1);
        gatewayIP[sizeof(gatewayIP) - 1] = '\0'; // Ensure null termination
        vTaskDelay(pdMS_TO_TICKS(3000));
        break;
    }
  }
}

void printLocalTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%B %d %Y %H:%M:%S");
}

void startupIPDisplay() {
  displayLeft.clearDisplay();
  displayLeft.setTextSize(1);
  displayLeft.setCursor(0, 0);
  displayLeft.print("IP: ");
  displayLeft.print(localIP);
  displayLeft.setCursor(0, 8);
  displayLeft.print("SM: ");
  displayLeft.print(subnetMask);
  displayLeft.setCursor(0, 16);
  displayLeft.print("GW IP: ");
  displayLeft.print(gatewayIP);
  displayLeft.display();
  vTaskDelay(pdMS_TO_TICKS(10000)); // Display for 10 seconds
  displayLeft.clearDisplay();
}

// This should be displayed on both
void displayStartupLogo() {
  displayLeft.drawBitmap(8, 0, epd_bitmap_AproGenLogo, 32, 32, WHITE); // Draw the logo while setup is running
  displayLeft.display();
  vTaskDelay(pdMS_TO_TICKS(1000));
  displayLeft.clearDisplay();
}

//TASKS

// Eyes blink twice, while eyes are closed nose gets smaller, on second time mouth animation plays, then all transition out
void animationLoopDemo(void * parameters) {
  while (true) {
    //Serial.print("Eye Frame: ");
    //Serial.println(eyeFrame);
    //Serial.print("Nose Frame: ");
    //Serial.println(noseFrame);
    //Serial.print("Mouth Frame: ");
    //Serial.println(mouthFrame);
    //Serial.print("Cycles: ");
    //Serial.println(blinkCycles);
    //Serial.println("");
    displayLeft.clearDisplay();
    if (eyeFrame <= 0) {
      eyeReverse = false;
      blinkCycles++;
    } else if (eyeFrame >= bitmapEyeArray_LEN - 1) {
      eyeReverse = true;
    }
    if (eyeReverse) {
      eyeFrame--;
    } else {
      eyeFrame++;
    }
    if (eyeFrame == bitmapEyeArray_LEN - 1) {
      noseFrame = 1;
    } else {
      noseFrame = 0;
    }
    if (blinkCycles % 2 == 0) {
      if (mouthFrame <= 0) {
        mouthReverse = false;
      } else if (mouthFrame >= bitmapMouthArray_LEN - 1) {
        mouthReverse = true;
      }
      if (mouthReverse) {
        mouthFrame--;
      } else {
        mouthFrame++;
      }
    } else {
      mouthFrame = 0;
    }
    displayLeft.drawBitmap(39, 5, bitmapEyeArray[eyeFrame], 25, 13, WHITE);
    displayLeft.drawBitmap(3, 22, bitmapMouthArray[mouthFrame], 35, 11, WHITE);
    displayLeft.drawBitmap(2, 11, bitmapNoseArray[noseFrame], 10, 6, WHITE);
    //displayRight.drawBitmap(39, 5, bitmapEyeArray[eyeFrame], 25, 13, WHITE);
    //displayRight.drawBitmap(3, 22, bitmapMouthArray[mouthFrame], 35, 11, WHITE);
    //displayRight.drawBitmap(2, 11, bitmapNoseArray[noseFrame], 10, 6, WHITE);
    displayLeft.drawFastVLine(65, 0, 33, WHITE); // Temporary
    displayLeft.drawFastHLine(0, 33, 65, WHITE); // Temporary
    displayLeft.display();
    //displayRight.display();
    vTaskDelay(pdMS_TO_TICKS(66)); // 15 FPS
    //vTaskDelay(pdMS_TO_TICKS(33)); // 30 FPS
    //vTaskDelay(pdMS_TO_TICKS(16)); // 60 FPS
    //vTaskDelay(pdMS_TO_TICKS(1000)); debug
    taskYIELD(); // Yield to other tasks
  }
}

//
void animationI2SBufferer(void * parameters) {
  while (true) {
    taskYIELD(); // Yield to other tasks
  }
}

//
void mp3PlayerLoop(void * parameters) {
  while (true) {
    taskYIELD(); // Yield to other tasks
  }
}

// Controlls the animation logic and then passes a completed frame to the display function
void animationLogic(void * parameters) {
  while (true) {
    taskYIELD(); // Yield to other tasks
  }
}

// 
void dhtProtection(void * parameters) {
  while (true) {
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();
    if (temperature > 27.0 || humidity > 75.0) {
      //Serial.println("Safe Threshold Exceeded!");
    }
    vTaskDelay(pdMS_TO_TICKS(3000)); // Read temp every second
    taskYIELD(); // Yield to other tasks
  }
}

//
void failsafesLoop(void * parameters) {
  while (true) {
    taskYIELD(); // Yield to other tasks
  }
}

//
void protoBeacon(void * parameters) {
  Serial.println("Starting ProtoBeacon Task...");
  // 1. Initialize BLE Device
  BLEDevice::init("ProtoHead_A"); // Give your device a unique name
  // 2. Get the advertising object
  pAdvertising = BLEDevice::getAdvertising();
  // 3. Create the advertisement data structure
  BLEAdvertisementData advertisementData;
  advertisementData.setFlags(0x06); // GeneralDiscoverable + BR_EDR_NOT_SUPPORTED
  advertisementData.setAppearance(0x00); // Unknown appearance
  // 4. Prepare Manufacturer Data (initial state)
  uint8_t manufacturerDataWithId[6]; // 2 bytes for Manu ID + 4 for payload
  manufacturerDataWithId[0] = (MANUFACTURER_ID & 0xFF);   // LSB
  manufacturerDataWithId[1] = (MANUFACTURER_ID >> 8);    // MSB
  manufacturerDataWithId[2] = PROTOGEN_ID_1; // 'P'
  manufacturerDataWithId[3] = PROTOGEN_ID_2; // 'R'
  manufacturerDataWithId[4] = currentEmotionCode;
  manufacturerDataWithId[5] = (proximityDetected ? 0x01 : 0x00);
  std::string strManufacturerDataWithId((char*)manufacturerDataWithId, 6);
  advertisementData.setManufacturerData(strManufacturerDataWithId);
  // 5. Set advertising data
  pAdvertising->setAdvertisementData(advertisementData);
  pAdvertising->setMinPreferred(0x06); // Helps with iPhone connectivity issues
  pAdvertising->setMaxPreferred(0x12);
  // 6. Start Advertising
  BLEDevice::startAdvertising();
  Serial.println("BLE Advertising started...");
  // Main loop for the task
  while (true) {
    // Update the payload
    manufacturerDataWithId[4] = currentEmotionCode;
    manufacturerDataWithId[5] = (proximityDetected ? 0x01 : 0x00);
    std::string updatedStrManufacturerDataWithId((char*)manufacturerDataWithId, 6);
    // Stop advertising briefly to update payload
    BLEDevice::stopAdvertising();
    advertisementData.setManufacturerData(updatedStrManufacturerDataWithId);
    pAdvertising->setAdvertisementData(advertisementData); // Apply updated advertisement data
    pAdvertising->setAdvertisementType(ADV_TYPE_NONCONN_IND);
    BLEDevice::startAdvertising();
  
    // Delay before next update
    vTaskDelay(pdMS_TO_TICKS(5000)); // Update advertising data every 5 seconds
    taskYIELD(); // Yield to other tasks
  }
}

//
void ntpSyncLoop(void * parameters) {
  while (true) {
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    vTaskDelay(pdMS_TO_TICKS(86400000)); // Sync time every 24 hours
    taskYIELD(); // Yield to other tasks
  }
}

//
void protoRemote(void * parameters) {
  while (true) {
    taskYIELD(); // Yield to other tasks
  }
}

//
void taskManagerLoop(void * parameters) {
  while (true) {
    taskManPrevMillis = millis();
    char taskListBuffer[200]; // Adjust buffer size as needed
    Serial.println("--- Task List ---");
    TaskHandle_t task1Handle = xTaskGetHandle("animationLoopDemo");
    if (task1Handle != NULL) {
      Serial.print("Task 1: ");
      Serial.println(pcTaskGetName(task1Handle));
      Serial.print("Stack High Water Mark: ");
      Serial.println(uxTaskGetStackHighWaterMark(task1Handle));        Serial.print("Time running: ");
      void vTaskGetRunTimeStatistics( char *pcWriteBuffer, size_t uxBufferLength );
      Serial.print("Status: ");
      void vTaskGetRunTimeStats( char *taskListBuffer );
    } else {
      Serial.println("Task Handle Is NULL");
    }
    Serial.println("-----------------");
    vTaskDelay(pdMS_TO_TICKS(10000));
    taskYIELD(); // Yield to other tasks
  }
}

// Setup function, runs once at startup
void setup() {
  Serial.begin(9600);
  Wire.begin();
  Wire1.begin(D4, D3); //sda, scl

  // Initialize the OLED display
  displayLeft.cp437(true);
  //displayRight.cp437(true);
  if(!displayLeft.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {   // Address 0x3C for 128x64
    Serial.println("SSD1306 allocation failed");
    for(;;); // Don't proceed, loop forever
  }
  displayLeft.clearDisplay();
  displayLeft.setTextSize(1); 
  displayLeft.setTextColor(WHITE);

  // connect to WiFi
  NetworkChange(); // Connect to WiFi
  if (WiFi.status() != WL_CONNECTED) { // If the WiFi connection fails
    Serial.println("Failed to connect to WiFi");
    serialOutputBuffer[0] = '\0';
    displayLeft.clearDisplay();
    displayLeft.setCursor(0, 16);
    displayLeft.setTextSize(1);
    displayLeft.print("Failed to connect to WiFi");
    displayLeft.display();
  }
  strncpy(localIP, WiFi.localIP().toString().c_str(), sizeof(localIP) - 1);
  localIP[sizeof(localIP) - 1] = '\0'; // Ensure null termination
  strncpy(subnetMask, WiFi.subnetMask().toString().c_str(), sizeof(subnetMask) - 1);
  subnetMask[sizeof(subnetMask) - 1] = '\0'; // Ensure null termination
  strncpy(gatewayIP, WiFi.gatewayIP().toString().c_str(), sizeof(gatewayIP) - 1);
  gatewayIP[sizeof(gatewayIP) - 1] = '\0'; // Ensure null termination
  sprintf(serialOutputBuffer, "Connected to WiFi.\nIP address: %s", localIP);
  Serial.println(serialOutputBuffer);
  serialOutputBuffer[0] = '\0';
  // initalise web server
  WebServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    if (millis() - lastRequestTime < requestCooldown) {
      Serial.println("Request rejected: Cooldown active.");
      request->send(429, "text/plain", "Error 429 Too Many Requests. Please wait before trying again.");
      return;
    }
    lastRequestTime = millis();
    Serial.println("Root page requested (Stream)");
    AsyncResponseStream *response = request->beginResponseStream("text/html");
    protoRemote(response); // Call function, passing the stream object
    request->send(response); // Send the streamed response
    Serial.println("Streamed response sent");
  });
  WebServer.begin(); // Start website hosting server, port 8080
  DataServer.begin(); // Start data sending server, port 8081 Replaces serial output

  // initialize RTC
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  printLocalTime();

  // initialize DHT sensor
  dht.begin();
  if (isnan(dht.readTemperature())) { // If the DHT sensors fail to initialize
    Serial.println("Failed to initialize DHT sensor");
    serialOutputBuffer[0] = '\0';
    displayLeft.clearDisplay();
    displayLeft.setCursor(0, 16);
    displayLeft.setTextSize(1);
    displayLeft.print("Failed to initialize DHT sensor");
    displayLeft.display();
  }

  // initialise Bluetooth
  if (!SD.begin(SPI_CS_SD)) { // If the SD card fails to initialize
    Serial.println("SD card failed to initialise");
  }

  // Reset reason
  disableLoopWDT();
  esp_reset_reason_t reason = esp_reset_reason();
  Serial.print("Reset reason: ");
  switch (reason) {
    case ESP_RST_UNKNOWN:    Serial.println("Unknown"); break;
    case ESP_RST_POWERON:    Serial.println("Power on"); break;
    case ESP_RST_EXT:        Serial.println("External reset"); break;
    case ESP_RST_SW:         Serial.println("Software reset"); break;
    case ESP_RST_PANIC:      Serial.println("Panic/exception"); break;
    case ESP_RST_INT_WDT:    Serial.println("Interrupt watchdog"); break; // IWDT
    case ESP_RST_TASK_WDT:   Serial.println("Task watchdog"); break;     // TWDT
    case ESP_RST_WDT:        Serial.println("Other watchdog"); break;
    case ESP_RST_DEEPSLEEP:  Serial.println("Deep sleep wakeup"); break;
    case ESP_RST_BROWNOUT:   Serial.println("Brownout"); break;          // Power issue!
    case ESP_RST_SDIO:       Serial.println("SDIO"); break;
    default:                 Serial.println("Other"); break;
  }

  displayStartupLogo();
  startupIPDisplay();
  
  // Create function tasks and assign to core (MAX 20 Priority)
  xTaskCreatePinnedToCore(animationLoopDemo, "Animation Loop Demo", STACK_SIZE_2, NULL, 2, &animationLoopDemoTask, CORE_1);
  //xTaskCreatePinnedToCore(animationI2SBufferer, "Animation I2S Bufferer", STACK_SIZE_3, NULL, 6, &animationI2SBuffererTask, CORE_0);
  //xTaskCreatePinnedToCore(mp3PlayerLoop, "MP3 Player Passthrough Functionality", STACK_SIZE_3, NULL, 4, &mp3PlayerLoopTask, CORE_1);
  //xTaskCreatePinnedToCore(animationLogic, "Animation Logic", STACK_SIZE_3, NULL, 5, &animationLogicTask, CORE_1);
  xTaskCreatePinnedToCore(dhtProtection, "DHT Protection", STACK_SIZE_2, NULL, 3, &dhtProtectionTask, CORE_1);
  //xTaskCreatePinnedToCore(failsafesLoop, "Failsafes", STACK_SIZE_2, NULL, 8, &failsafesLoopTask, CORE_1);
  xTaskCreatePinnedToCore(protoBeacon, "Proto Beacon (BLE)", STACK_SIZE_2, NULL, 2, &protoBeaconTask, CORE_1);
  xTaskCreatePinnedToCore(ntpSyncLoop, "NTP Sync Loop", STACK_SIZE_1, NULL, 1, &ntpSyncLoopTask, CORE_1);
  //xTaskCreatePinnedToCore(protoRemote, "Proto Remote (HTTP)", STACK_SIZE_4, NULL, 7, &protoRemoteTask, CORE_1);
  //xTaskCreatePinnedToCore(taskManagerLoop, "Task Manager (C)", STACK_SIZE_2, NULL, 2, &taskManagerLoopTask, CORE_1);

}

// Empty loop, all tasks are handles in their own functions
void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
  taskYIELD(); // Yield to other tasks
}