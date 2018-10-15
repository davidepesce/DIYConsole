/* main.cpp */

#include <Arduino.h>
#include "GFX.h"

GFX gfx;

void setup() {
    gfx.begin();
}

int x = 160;
int y = 240;
int deltaX = 1;
int deltaY = 1;
int color = random(15);

void loop() {
    gfx.drawPixel(x, y, color);

    // 10% chance of changing x and y direction
    if(random(10) == 0)
        deltaX = random(-1, 2);
    if(random(10) == 0)
        deltaY = random(-1, 2);
    
    // update x and y position
    x += deltaX;
    if(x < 0)
        x = x + 320;
    else if(x >= 320)
        x = x - 320;
    
    y += deltaY;
    if(y < 0)
        y = y + 480;
    else if(y >= 480)
        y = y - 480;
    
    // 2.5% chance to change color
    if(random(40) == 0)
        color = random(15);
    
    // update screen
    gfx.update();
}