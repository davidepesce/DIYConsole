/* main.cpp */

#include <Arduino.h>
#include "GFX.h"
#include "bitmaps.h"

GFX gfx;
uint8_t backgroundCopy[4624];
uint8_t scaledAndRotatedBitmap[4624];
float x, y, theta, xTarget, yTarget;
uint16_t width, height;
unsigned long lastUpdate;

void setup() {
    // Start graphics library
    gfx.begin();

    // Draw stars background
    for(int i=0; i<60; i++)
        gfx.drawPixel(random(0,320), random(0,480), 14);
    for(int i=0; i<30; i++)
        gfx.drawPixel(random(0,320), random(0,480), 1);
    for(int i=0; i<40; i++)
        gfx.drawPixel(random(0,320), random(0,480), 2);
    for(int i=0; i<40; i++)
        gfx.drawPixel(random(0,320), random(0,480), 7);
    
    // Prepare starship bitmap
    x = 160;
    y = 240;
    theta = 0;
    gfx.scaleAndRotateBitmap(scaledAndRotatedBitmap, &width, &height, starship, 32, 32, 1.5, 1.5, theta, 15);

    // Copy background
    gfx.copyScreenBufferRect(backgroundCopy, x-width/2, y-height/2, width, height);

    // Draw starship
    gfx.drawTransparentBitmap(scaledAndRotatedBitmap, x-width/2, y-height/2, width, height, 15);

    // Set initial target
    xTarget = random(0, 320);
    yTarget = random(0, 480);

    // Start time
    lastUpdate = micros();

    // Draw first frame
    gfx.update();
}

void loop() {
    // Calculate frame delta time
    unsigned long now = micros();
    float deltaTime = (float)(now - lastUpdate) / 1000000;
    lastUpdate = now;

    // Restore background
    gfx.drawBitmap(backgroundCopy, x-width/2, y-height/2, width, height);

    // Update starship rotation
    float thetaTarget = atan2(yTarget - y, xTarget - x) * 57.2957795 + 90;

    // Turn the starship to the target in the direction of the smallest angle
    if(fabs(thetaTarget - theta) > 180) {
        if(theta < 0)
            theta += 360;
        else
            theta -= 360;
    }
    theta += max(min(3 * (thetaTarget - theta), 180), -180) * deltaTime;

    // Update starship position
    float xSpeed = 0.5 * (xTarget - x) * (1 / (1 + 0.1 * fabs(thetaTarget - theta)));
    float ySpeed = 0.5 * (yTarget - y) * (1 / (1 + 0.1 * fabs(thetaTarget - theta)));
    x = x + xSpeed * deltaTime;
    y = y + ySpeed * deltaTime;

    // If near the target, choose another random target
    if((fabs(x - xTarget) < 40) && (fabs(y - yTarget) < 40)) {
        xTarget = random(0, 320);
        yTarget = random(0, 480);
    }

    // Prepare starship bitmap
    gfx.scaleAndRotateBitmap(scaledAndRotatedBitmap, &width, &height, starship, 32, 32, 1.5, 1.5, theta, 15);

    // Copy background
    gfx.copyScreenBufferRect(backgroundCopy, x-width/2, y-height/2, width, height);

    // Draw starship
    gfx.drawTransparentBitmap(scaledAndRotatedBitmap, x-width/2, y-height/2, width, height, 15);

    // Update screen
    gfx.update();
}
