/* main.cpp */

#include <Arduino.h>
#include "GFX.h"
#include "DefaultFont.h"

#define CHARDROPS_COUNT 15

// Point - this structure is used to store the current position
// of a "char drop".
struct Point {
    uint8_t x;
    uint8_t y;
};

// Cell - the screen is a grid of these structures; every cell
// contains the character shown on the screen and how many frames
// ago it was drawn.
struct Cell {
    uint8_t character;
    int age;
};

GFX gfx;
Point chardrops[CHARDROPS_COUNT];
Cell screen[40][30];

// randomChar - generate a random printable characted (space excluded)
uint8_t randomChar() {
    uint8_t rc;
    do {
        rc = random(0, 256);
    } while(rc == 0x00 || rc == 0x08 || rc == 0x0A || rc == 0x20);
    return rc;
}

void setup() {
    // Start graphics library and set default font
    gfx.begin();
    gfx.setFont(&defaultFont);
    
    // Randomly generate char drops starting position
    for(int i=0; i<CHARDROPS_COUNT; i++) {
        chardrops[i].x = random(0, 40);
        chardrops[i].y = random(0, 20);
    }
}

void loop() {
    for(int i=0; i<CHARDROPS_COUNT; i++) {
        // Write a new char at drop positions
        screen[chardrops[i].x][chardrops[i].y].character = randomChar();
        screen[chardrops[i].x][chardrops[i].y].age = 0;

        // Update drop position. If the end of the screen has been
        // reached, restart from the upper side.
        chardrops[i].y++;
        if(chardrops[i].y == 30) {
            chardrops[i].x = random(0, 40);
            chardrops[i].y = 0;
        }
    }

    // Clear screen
    gfx.fillScreen(15);

    // Draw char drops. Use white for current drop positions,
    // light green for the most recent part of the trail and
    // dark green for the rest.
    for(int x=0; x<40; x++) {
        for(int y=0; y<30; y++) {
            uint8_t color;
            if(screen[x][y].age == 0)
                color = 0; // White
            else if(screen[x][y].age <= 8)
                color = 8; // Light green
            else if(screen[x][y].age <= 30)
                color = 9; // Dark green
            else
                color = 15; // Black, the character can't be seen.
            gfx.drawChar(8*x, 16*y, screen[x][y].character, color);
        }
    }

    // Draw "THE MATRIX" with a black border (made by drawing the
    // string in black with an offset of 2px in each direction)
    gfx.drawString2x(82, 216, "THE MATRIX", 15);
    gfx.drawString2x(78, 216, "THE MATRIX", 15);
    gfx.drawString2x(80, 214, "THE MATRIX", 15);
    gfx.drawString2x(80, 218, "THE MATRIX", 15);
    gfx.drawString2x(80, 216, "THE MATRIX", 0);

    // Update age of drops
    for(int x=0; x<40; x++) {
        for(int y=0; y<30; y++) {
            screen[x][y].age++;
        }
    }

    // Update screen
    gfx.update();
    delay(30);
}
