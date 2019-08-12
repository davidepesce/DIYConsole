/* main.cpp */

#include <Arduino.h>
#include "GFX.h"
#include "DefaultFont.h"
#include "bitmaps.h"

#define STARSHIP_COUNT 1

struct StarShip {
    float x;
    float y;
    float theta;
    float previousTheta;
    float xTarget;
    float yTarget;
    uint8_t* backgroundCopy;
    uint8_t* rotatedBitmap;
    uint16_t width;
    uint16_t height;
};

GFX gfx;
StarShip ships[STARSHIP_COUNT];
unsigned long lastUpdate;
unsigned long frames;
double avgFps;

void setup() {
    // Start graphics library
    gfx.begin();
    gfx.setFont(&defaultFont);

    // Draw stars background
    for(int i=0; i<60; i++)
        gfx.drawPixel(random(0,320), random(0,480), 14);
    for(int i=0; i<30; i++)
        gfx.drawPixel(random(0,320), random(0,480), 1);
    for(int i=0; i<40; i++)
        gfx.drawPixel(random(0,320), random(0,480), 2);
    for(int i=0; i<40; i++)
        gfx.drawPixel(random(0,320), random(0,480), 7);
    
    for(int i=0; i<STARSHIP_COUNT; i++) {
        // Generate start data
        ships[i].x = random(30, 290);
        ships[i].y = random(30, 450);
        ships[i].theta = ships[i].previousTheta = random(0, 360);
        ships[i].xTarget = random(0, 320);
        ships[i].yTarget = random(0, 480);
        ships[i].backgroundCopy = (uint8_t*)malloc(4624);
        ships[i].rotatedBitmap = (uint8_t*)malloc(4624);

        // Prepare starship bitmap
        gfx.scaleAndRotateBitmap(ships[i].rotatedBitmap, &ships[i].width, &ships[i].height, starship, 32, 32, 1.5, 1.5, ships[i].theta, 15);

        // Copy background
        gfx.copyScreenBufferRect(ships[i].backgroundCopy, ships[i].x-ships[i].width/2, ships[i].y-ships[i].height/2, ships[i].width, ships[i].height);
    }

    for(int i=0; i<STARSHIP_COUNT; i++) {
        // Draw starship
        gfx.drawTransparentBitmap(ships[i].rotatedBitmap, ships[i].x-ships[i].width/2, ships[i].y-ships[i].height/2, ships[i].width, ships[i].height, 15);
    }

    // Start time
    lastUpdate = micros();
    frames = 0;

    // Draw first frame
    gfx.update();
}

void loop() {
    // Calculate frame delta time
    unsigned long now = micros();
    float deltaTime = (float)(now - lastUpdate) / 1000000;
    lastUpdate = now;

    // Restore background
    for(int i=0; i<STARSHIP_COUNT; i++) {
        gfx.drawBitmap(ships[i].backgroundCopy, ships[i].x-ships[i].width/2, ships[i].y-ships[i].height/2, ships[i].width, ships[i].height);
    }

    // Update starship position, rotation and target
    for(int i=0; i<STARSHIP_COUNT; i++) {
        // Update starship rotation
        float thetaTarget = atan2(ships[i].yTarget - ships[i].y, ships[i].xTarget - ships[i].x) * 57.2957795 + 90;

        // Turn the starship to the target in the direction of the smallest angle
        if(fabs(thetaTarget - ships[i].theta) > 180) {
            if(ships[i].theta < 0)
                ships[i].theta += 360;
            else
                ships[i].theta -= 360;
        }
        ships[i].theta += max(min(3 * (int)(thetaTarget - ships[i].theta), 180), -180) * deltaTime;

        // Update starship position
        float xSpeed = 0.5 * (ships[i].xTarget - ships[i].x) * (1 / (1 + 0.1 * fabs(thetaTarget - ships[i].theta)));
        float ySpeed = 0.5 * (ships[i].yTarget - ships[i].y) * (1 / (1 + 0.1 * fabs(thetaTarget - ships[i].theta)));
        ships[i].x += xSpeed * deltaTime;
        ships[i].y += ySpeed * deltaTime;

        // If near the target, choose another random target
        if((fabs(ships[i].x - ships[i].xTarget) < 40) && (fabs(ships[i].y - ships[i].yTarget) < 40)) {
            ships[i].xTarget = random(0, 320);
            ships[i].yTarget = random(0, 480);
        }
    }

    for(int i=0; i<STARSHIP_COUNT; i++) {
        // Prepare starship bitmap
        if(abs(ships[i].theta - ships[i].previousTheta) >= 2) {
            gfx.scaleAndRotateBitmap(ships[i].rotatedBitmap, &ships[i].width, &ships[i].height, starship, 32, 32, 1.5, 1.5, ships[i].theta, 15);
            ships[i].previousTheta = ships[i].theta;
        }

        // Copy background
        gfx.copyScreenBufferRect(ships[i].backgroundCopy, ships[i].x-ships[i].width/2, ships[i].y-ships[i].height/2, ships[i].width, ships[i].height);
    }

    for(int i=0; i<STARSHIP_COUNT; i++) {
        // Draw starship
        gfx.drawTransparentBitmap(ships[i].rotatedBitmap, ships[i].x-ships[i].width/2, ships[i].y-ships[i].height/2, ships[i].width, ships[i].height, 15);
    }

    // Draw frame rate
    double fps = 1.0 / deltaTime;
    avgFps = (avgFps * frames + fps) / (++frames);
    gfx.drawFilledRectangle(0, 0, 40, 16, 15);
    String fpsString(avgFps, 1);
    gfx.drawString(0, 0, fpsString.c_str(), 3);

    // Update screen
    gfx.update();
}
