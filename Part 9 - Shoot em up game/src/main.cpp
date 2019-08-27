/* main.cpp */

#include <Arduino.h>
#include <Adafruit_STMPE610.h>
#include "GFX.h"
#include "bitmaps.h"
#include "DefaultFont.h"

enum GameState {
  StartScreen,
  Running,
  GameOver
};

struct GameObject {
  float x;
  float y;
  bool valid;
  uint8_t color;
};

struct ExplosionCircle {
  float deltaX;
  float deltaY;
  float radius;
};

struct Explosion : GameObject {
  float speed;
  unsigned long startTime;
  ExplosionCircle circles[6];
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
#define MAX_ASTEROIDS   20
#define MAX_BULLETS     5
#define MAX_EXPLOSIONS  5
GameState gameState = StartScreen;
unsigned long lastUpdateTime;
unsigned long lastAsteroidTime = 0;
unsigned long lastFireTime = 0;
unsigned long firePeriod = 1000000;
float asteroidPeriod = 500000;
float asteroidSpeed = 70;
float score;
GameObject starship;
GameObject farStar[FARSTAR_COUNT];
GameObject nearStar[NEARSTAR_COUNT];
GameObject asteroid[MAX_ASTEROIDS];
GameObject bullet[MAX_BULLETS];
Explosion explosion[MAX_EXPLOSIONS];


float distanceSquared(float x1, float y1, float x2, float y2) {
  return (x1-x2)*(x1-x2)+(y1-y2)*(y1-y2);
}

bool createAsteroid(float x, float y) {
  for(int i=0; i<MAX_ASTEROIDS; i++) {
    if(!asteroid[i].valid) {
      asteroid[i].valid = true;
      asteroid[i].x = x;
      asteroid[i].y = y;
      return true;
    }
  }
  return false;
}

bool createBullet(float x, float y) {
  for(int i=0; i<MAX_BULLETS; i++) {
    if(!bullet[i].valid) {
      bullet[i].valid = true;
      bullet[i].x = x;
      bullet[i].y = y;
      return true;
    }
  }
  return false;
}

void createExplosion(float x, float y, float speed) {
  for(int i=0; i<MAX_EXPLOSIONS; i++) {
    if(!explosion[i].valid) {
      explosion[i].valid = true;
      explosion[i].startTime = micros();
      explosion[i].x = x;
      explosion[i].y = y;
      explosion[i].speed = speed;
      for(int j=0; j<6; j++) {
        explosion[i].circles[j].deltaX = random(-10, 10);
        explosion[i].circles[j].deltaY = random(-10, 10);
        explosion[i].circles[j].radius = random(10, 20);
      }
      break;
    }
  }
}


void setup() {
  // Set initial starship position
  starship.x = 160;
  starship.y = 230;
  starship.valid = false;

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
  gfx.setFont(&defaultFont);

  // Draw input area
  gfx.drawFilledRectangle(0, 320, 320, 160, 13);
  gfx.drawFilledRectangle(2, 322, 316, 156, 14);

  // Draw far stars
  for(int i=0; i<FARSTAR_COUNT; i++)
    gfx.drawPixel(farStar[i].x, farStar[i].y, farStar[i].color);

  // Draw near stars
  for(int i=0; i<NEARSTAR_COUNT; i++)
    gfx.drawTransparentBitmap(starBitmap, nearStar[i].x, nearStar[i].y, 3, 3, 15);

  // Draw first frame
  gfx.update();
  lastUpdateTime = micros();
}


void loop() {
  /*** FRAME SETUP AND INPUT READING ***/

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

  // Things to do only when game is running
  if(gameState == Running) {
    // Update score
    score += deltaTime * asteroidSpeed;

    // Update asteroid speed and spawn period
    asteroidSpeed += 2 * deltaTime;
    if(asteroidPeriod > 10000)
      asteroidPeriod -= 3000 * deltaTime;

    // Update bullet fire period
    if(firePeriod > 10000)
      firePeriod -= 3000 * deltaTime;

    // Create new asteroid?
    if((now - lastAsteroidTime) >= asteroidPeriod) {
      if(createAsteroid(esp_random() % 320, -32))
        lastAsteroidTime = now;
    }

    // Fire a new bullet?
    if((now - lastFireTime) >= firePeriod) {
      if(createBullet(starship.x, starship.y - 16))
        lastFireTime = now;
    }

    // Erase score
    gfx.drawFilledRectangle(2, 2, 160, 16, 15);
  }

  // We need to redraw input area?
  bool redrawInputArea = false;


  /*** ERASE GAME SCREEN ***/

  // Erase starship
  if(starship.valid)
    gfx.drawFilledRectangle(starship.x-16, starship.y-16, 32, 32, 15);

  // Erase bullets
  for(int i=0; i<MAX_BULLETS; i++) {
    if(bullet[i].valid)
      gfx.drawFilledRectangle(bullet[i].x-2, bullet[i].y-2, 5, 5, 15);
  }

  // Erase asteroids
  for(int i=0; i<MAX_ASTEROIDS; i++) {
    if(asteroid[i].valid) {
      int height = 336 - asteroid[i].y;
        if(height > 32)
          height = 32;
        gfx.drawFilledRectangle(asteroid[i].x-16, asteroid[i].y-16, 32, height, 15);
    }
  }

  // Erase explosions
  for(int i=0; i<MAX_EXPLOSIONS; i++) {
    if(explosion[i].valid) {
      int x = explosion[i].x;
      int y = explosion[i].y;
      for(int j=0; j<6; j++) {
        if(y + explosion[i].circles[j].deltaY >= (320 - explosion[i].circles[j].radius))
          redrawInputArea = true;
        gfx.drawFilledCircle(x + explosion[i].circles[j].deltaX, y + explosion[i].circles[j].deltaY, explosion[i].circles[j].radius+2, 15);
      }
    }
  }

  // Erase near stars
  for(int i=0; i<NEARSTAR_COUNT; i++)
    gfx.drawFilledRectangle(nearStar[i].x, nearStar[i].y, 3, 3, 15);

  // Erase far stars
  for(int i=0; i<FARSTAR_COUNT; i++)
    gfx.drawPixel(farStar[i].x, farStar[i].y, 15);


  /*** PROCESS INPUT ***/

  if(screenTouched) {
    switch(gameState) {
      case StartScreen:
      {
        // Start a new game
        gameState = Running;
        starship.valid = true;
        // Erase start string
        gfx.drawFilledRectangle(48, 144, 224, 32, 15);
        break;
      }

      case Running:
      {
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
        break;
      }

      case GameOver:
      {
        // Map from touchscreen coordinates to screen coordinates
        int tx = map(touchPoint.x, TS_MINX, TS_MAXX, 0, 320);
        int ty = map(touchPoint.y, TS_MINY, TS_MAXY, 0, 480);
        if((tx >= 50) && (tx <= 270) && (ty >= 170) && (ty <= 234)) {
          // Play again clicked, restore initial value
          firePeriod = 1000000;
          asteroidPeriod = 500000;
          asteroidSpeed = 70;
          score = 0;
          starship.x = 160;
          starship.y = 230;
          starship.valid = true;
          gameState = Running;

          // Erase play again button and game over string
          gfx.drawFilledRectangle(50, 60, 240, 174, 15);
        }
        break;
      }
    }
  }
  

  /*** UPDATE OBJECTS AND CHECK COLLISIONS ***/

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

  // Update bullets position
  for(int i=0; i<MAX_BULLETS; i++) {
    if(bullet[i].valid) {
      bullet[i].y -= 200 * deltaTime;
      if(bullet[i].y < 0)
        bullet[i].valid = false;
    }
  }

  // Update asteroids position
  for(int i=0; i<MAX_ASTEROIDS; i++) {
    if(asteroid[i].valid) {
      asteroid[i].y += asteroidSpeed * deltaTime;
      if(asteroid[i].y > 336) {
        asteroid[i].valid = false;
      }
    }
  }

  // Update explosions position
  for(int i=0; i<MAX_EXPLOSIONS; i++) {
    if(explosion[i].valid) {
      explosion[i].y += explosion[i].speed * deltaTime;
      for(int j=0; j<6; j++) {
        explosion[i].circles[j].deltaX *= 1.1;
        explosion[i].circles[j].deltaY *= 1.1;
        explosion[i].circles[j].radius *= 0.8;
      }
      if(explosion[i].y >= 320) {
        explosion[i].valid = false;
      }
      if((now - explosion[i].startTime) > 400000)
        explosion[i].valid = false;
    }
  }

  // Check collision of starship with asteroids
  if(starship.valid) {
    for(int i=0; i<MAX_ASTEROIDS; i++) {
      if(asteroid[i].valid) {
        if(distanceSquared(starship.x, starship.y, asteroid[i].x, asteroid[i].y) < 900) {
          // Create one explosion for the starship (with speed 0), and
          // an explosion for the asteroid (with current asteroid speed)
          createExplosion(starship.x, starship.y, 0);
          createExplosion(asteroid[i].x, asteroid[i].y, asteroidSpeed);
          asteroid[i].valid = false;
          gameState = GameOver;
          starship.valid = false;
          break;
        }
      }
    }
  }

  // Check collision of bullets and asteroids
  for(int i=0; i<MAX_ASTEROIDS; i++) {
    if(!asteroid[i].valid)
      continue;
    for(int j=0; j<MAX_BULLETS; j++) {
      if(bullet[j].valid) {
        if(distanceSquared(bullet[j].x, bullet[j].y, asteroid[i].x, asteroid[i].y) < 290) {
          if(gameState == Running)
            score += 200;
          createExplosion(asteroid[i].x, asteroid[i].y, asteroidSpeed);
          asteroid[i].valid = false;
          bullet[j].valid = false;
          break;
        }
      }
    }
  }


  /*** REDRAW GAME SCREEN ***/

  // Draw far stars
  for(int i=0; i<FARSTAR_COUNT; i++)
    gfx.drawPixel(farStar[i].x, farStar[i].y, farStar[i].color);

  // Draw near stars
  for(int i=0; i<NEARSTAR_COUNT; i++)
    gfx.drawTransparentBitmap(starBitmap, nearStar[i].x, nearStar[i].y, 3, 3, 15);

  // Draw asteroids
  for(int i=0; i<MAX_ASTEROIDS; i++) {
    if(asteroid[i].valid) {
      int height = 336 - asteroid[i].y;
      if(height > 32)
        height = 32;
      gfx.drawTransparentBitmap(asteroidBitmap, asteroid[i].x-16, asteroid[i].y-16, 32, height, 0);
    }
  }

  // Draw bullets
  for(int i=0; i<MAX_BULLETS; i++) {
    if(bullet[i].valid)
      gfx.drawTransparentBitmap(bulletBitmap, bullet[i].x-2, bullet[i].y-2, 5, 5, 0);
  }

  // Draw explosions
  for(int i=0; i<MAX_EXPLOSIONS; i++) {
    if(explosion[i].valid) {
      float x = explosion[i].x;
      float y = explosion[i].y;
      for(int j=0; j<6; j++) {
        unsigned long timeFromStart = now - explosion[i].startTime;
        uint8_t color = 0;
        if(timeFromStart > 280000)
          color = 3;
        else if(timeFromStart > 180000)
          color = 2;
        else if(timeFromStart > 40000)
          color = 1;
        gfx.drawFilledCircle(x + explosion[i].circles[j].deltaX, y + explosion[i].circles[j].deltaY, explosion[i].circles[j].radius, color);
      }
    }
  }

  // Draw starship
  if(starship.valid)
    gfx.drawTransparentBitmap(starshipBitmap, starship.x-16, starship.y-16, 32, 32, 15);

  // Draw strings and buttons
  char buffer[32];
  switch (gameState) {
    case StartScreen:
      gfx.drawString2x(48, 144, "TOUCH TO START", 0);
      break;
    
    case Running:
      sprintf(buffer, "%d", (int)roundf(score));
      gfx.drawString(2, 2, buffer, 0);
      break;

    case GameOver:
      sprintf(buffer, "SCORE: %d", (int)roundf(score));
      gfx.drawString2x(88, 80, "GAME OVER", 0);
      gfx.drawString2x(160-8*strlen(buffer), 116, buffer, 0);
      gfx.drawFilledRectangle(50, 170, 220, 64, 9);
      gfx.drawRectangle(50, 170, 220, 64, 8);
      gfx.drawString2x(80, 186, "PLAY AGAIN", 0);
      break;
  }

  // Redraw top of the input area
  if(redrawInputArea) {
    gfx.drawFilledRectangle(0, 320, 320, 80, 13);
    gfx.drawFilledRectangle(2, 322, 316, 78, 14);
  }

  // Update screen
  gfx.update();
}
