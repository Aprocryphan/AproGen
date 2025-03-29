#include "AnimationFrames.h"
#include <Wire.h> // For SCL & SDA communication
//#include <WiFi.h>
//#include <Adafruit_SleepyDog.h>
#include "DHT.h" // For DHT11 sensor
#include <Adafruit_GFX.h> // For OLED Monitor
#include <Adafruit_SSD1306.h> // For OLED Monitor
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define DHTPIN 2 // Digital pin connected to the DHT sensor inside the helmet
#define DHTTYPE DHT11 // The type of DHT sensor
DHT dht(DHTPIN, DHTTYPE); // Create a DHT object for the sensor
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); // Declares the OLED display

int blinkCycles = 0;
int eyeFrame = 0; // eyeFrame counter for animation
int mouthFrame = 0; // mouthFrame counter for animation
int noseFrame = 0; // noseFrame counter for animation
bool eyeReverse = false;
bool mouthReverse = false;

void flipBitmapVertical(const uint8_t *source, uint8_t *destination, int width, int height) {
  for (int x = 0; x < width; x++) {
    for (int y = 0; y < height; y++) {
      int originalIndex = y * width + x;
      int flippedIndex = (height - 1 - y) * width + x;
      destination[flippedIndex] = source[originalIndex];
    }
  }
}

void setup() {
  Serial.begin(9600);
  // Initialize the OLED display
  display.cp437(true);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {   // Address 0x3C for 128x64
    Serial.println("SSD1306 allocation failed");
    for(;;); // Don't proceed, loop forever
  }
  display.clearDisplay();
  display.setTextSize(1); 
  display.setTextColor(WHITE);

  // initialize DHT sensors
  dht.begin();
  if (isnan(dht.readTemperature())) { // If the DHT sensors fail to initialize
    Serial.println("Failed to initialize DHT sensors");
    display.clearDisplay();
    display.setCursor(0, 16);
    display.setTextSize(1);
    display.print("Failed to initialize DHT sensors");
    display.display();
  }
}

// Eyes blink twice, while eyes are closed nose gets smaller, on second time mouth animation plays, then all transition out
void loop() {
  Serial.print("Eye Frame: ");
  Serial.println(eyeFrame);
  Serial.print("Nose Frame: ");
  Serial.println(noseFrame);
  Serial.print("Mouth Frame: ");
  Serial.println(mouthFrame);
  Serial.print("Cycles: ");
  Serial.println(blinkCycles);
  Serial.println("");
  display.clearDisplay();
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

  display.drawBitmap(39, 5, bitmapEyeArray[eyeFrame], 25, 13, WHITE);
  display.drawBitmap(3, 22, bitmapMouthArray[mouthFrame], 35, 11, WHITE);
  display.drawBitmap(2, 11, bitmapNoseArray[noseFrame], 10, 6, WHITE);
  display.display();
  delay(66); // 15 FPS
  //delay(33) // 30 FPS
  //delay(16) // 60 FPS
  //delay(1000); debug
}