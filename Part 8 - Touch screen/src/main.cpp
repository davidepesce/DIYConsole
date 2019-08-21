/* main.cpp */

#include <Arduino.h>
#include <Adafruit_STMPE610.h>
#include "GFX.h"
#include "bitmaps.h"

struct Starship {
  float x;
  float y;
};

struct Star {
  float x;
  float y;
  uint8_t color;
};

// GFX library
GFX gfx;

// Touch screen
#define STMPE_CS 32
#define TS_MINX 3750
#define TS_MINY 3800
#define TS_MAXX 360
#define TS_MAXY 270
Adafruit_STMPE610 touchScreen = Adafruit_STMPE610(STMPE_CS);

// Game data
#define FARSTAR_COUNT   60
#define NEARSTAR_COUNT  20
unsigned long lastUpdateTime;
Starship starship;
Star farStar[FARSTAR_COUNT];
Star nearStar[NEARSTAR_COUNT];


void setup() {
  // Set initial starship position
  starship.x = 160;
  starship.y = 230;

  // Generate random far stars
  for(int i=0; i<FARSTAR_COUNT; i++) {
    farStar[i].x = esp_random() % 320;
    farStar[i].y = esp_random() % 320;
    int rnd = esp_random() % 3;
    if(rnd == 0)
      farStar[i].color = 1;
    else if(rnd == 1)
      farStar[i].color = 7;
    else
      farStar[i].color = 12;
  }

  // Generate random near stars
  for(int i=0; i<NEARSTAR_COUNT; i++) {
    nearStar[i].x = esp_random() % 317;
    nearStar[i].y = esp_random() % 317;
  }

  // Start touch screen library
  touchScreen.begin();

  // Start graphics library
  gfx.begin();

  // Draw input area
  gfx.drawFilledRectangle(0, 320, 320, 160, 13);
  gfx.drawFilledRectangle(2, 322, 316, 156, 14);

  // Draw far stars
  for(int i=0; i<FARSTAR_COUNT; i++)
    gfx.drawPixel(farStar[i].x, farStar[i].y, farStar[i].color);

  // Draw near stars
  for(int i=0; i<NEARSTAR_COUNT; i++)
    gfx.drawTransparentBitmap(starBitmap, nearStar[i].x, nearStar[i].y, 3, 3, 15);
  
  // Draw starship
  gfx.drawTransparentBitmap(starshipBitmap, starship.x-16, starship.y-16, 32, 32, 15);

  // Draw first frame
  gfx.update();
  lastUpdateTime = micros();
}


void loop() {
  // Calculate delta time
  unsigned long now = micros();
  float deltaTime = (now - lastUpdateTime) / 1000000.0;
  lastUpdateTime = now;

  // Get last touched screen point
  bool screenTouched = false;
  TS_Point touchPoint;
  while(!touchScreen.bufferEmpty()) {
    screenTouched = true;
    touchPoint = touchScreen.getPoint();
  }

  // Erase starship
  gfx.drawFilledRectangle(starship.x-16, starship.y-16, 32, 32, 15);

  // Erase near stars
  for(int i=0; i<NEARSTAR_COUNT; i++)
    gfx.drawFilledRectangle(nearStar[i].x, nearStar[i].y, 3, 3, 15);

  // Erase far stars
  for(int i=0; i<FARSTAR_COUNT; i++)
    gfx.drawPixel(farStar[i].x, farStar[i].y, 15);
  
  // Update stars position
  for(int i=0; i<FARSTAR_COUNT; i++) {
    farStar[i].y += 12.0 * deltaTime;
    if(farStar[i].y >= 320) {
      farStar[i].y = 0;
      farStar[i].x = esp_random() % 320;
    }
  }
  for(int i=0; i<NEARSTAR_COUNT; i++) {
    nearStar[i].y += 36 * deltaTime;
    if(nearStar[i].y >= 318) {
      nearStar[i].y -= 320;
      nearStar[i].x = esp_random() % 317;
    }
  }

  // Draw far stars
  for(int i=0; i<FARSTAR_COUNT; i++)
    gfx.drawPixel(farStar[i].x, farStar[i].y, farStar[i].color);

  // Draw near stars
  for(int i=0; i<NEARSTAR_COUNT; i++)
    gfx.drawTransparentBitmap(starBitmap, nearStar[i].x, nearStar[i].y, 3, 3, 15);

  // Update starship position
  if(screenTouched) {
    // Map from touchscreen coordinates to screen coordinates
    int tx = map(touchPoint.x, TS_MINX, TS_MAXX, 0, 320);
    int ty = map(touchPoint.y, TS_MINY, TS_MAXY, 0, 480);
    // Ignore touches outside input area
    if(ty > 320) {
      // Set target point for starship movement
      int xTarget = map(tx, 30, 280, 16, 304);
      int yTarget = map(ty, 320, 460, 144, 304);
      xTarget = min(max(xTarget, 16), 304);
      yTarget = min(max(yTarget, 144), 304);
      // Move starship a step towards the target
      starship.x += 24.0 * ((float)xTarget - starship.x) * deltaTime;
      starship.y += 24.0 * ((float)yTarget - starship.y) * deltaTime;
    }
  }

  // Draw starship
  gfx.drawTransparentBitmap(starshipBitmap, starship.x-16, starship.y-16, 32, 32, 15);

  // Update screen
  gfx.update();
}
