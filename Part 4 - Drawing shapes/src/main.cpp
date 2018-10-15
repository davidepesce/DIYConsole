/* main.cpp */

#include <Arduino.h>
#include "GFX.h"

#define NUMBER_OF_CLOUDS            30
#define NUMBER_OF_CLOUD_COMPONENTS  30

// Clouds data structures
struct CloudComponent {
    float size;
    float x;
    float y;
    float t;
};
struct Cloud {
    float x;
    float y;
    float z;
    CloudComponent component[NUMBER_OF_CLOUD_COMPONENTS];
};

GFX gfx;
uint8_t cloudColors[3] = { 13, 12, 0 };
Cloud clouds[NUMBER_OF_CLOUDS];
unsigned long lastUpdate;

void setup() {
    // Random generations of clouds
    for(int i=0; i<NUMBER_OF_CLOUDS; i++) {
        clouds[i].x = random(-2500, 2500);
        clouds[i].y = random(-120, 120);
        clouds[i].z = 1.0 + (float)(NUMBER_OF_CLOUDS - i) / 15;
        for(int j=0; j<NUMBER_OF_CLOUD_COMPONENTS; j++) {
            clouds[i].component[j].size = (random(10) + 15);
            clouds[i].component[j].x = random(-50, 50);
            clouds[i].component[j].y = random(-12, 12);
            clouds[i].component[j].t = (float)random(360);
        }
    }

    // Draw sky, hills and flowers
    gfx.begin();
    gfx.fillScreen(7);
    gfx.drawFilledCircle(260, 70, 15, 1);
    gfx.drawFilledCircle(80, 480, 240, 8);
    for(int i=0; i<200; i++) {
        double radius = random(135, 235);
        double theta = random(225, 315);
        double x = radius * cos(theta / 180 * 3.14);
        double y = radius * sin(theta / 180 * 3.14);
        gfx.drawFilledCircle(80+x, 480+y, 1, 1 + random(2));
    }
    gfx.drawFilledCircle(200, 700, 400, 9);
    for(int i=0; i<400; i++) {
        double radius = random(200, 395);
        double theta = random(190, 315);
        double x = radius * cos(theta / 180 * 3.14);
        double y = radius * sin(theta / 180 * 3.14);
        gfx.drawCircle(200+x, 700+y, 1, 2 + random(3));
    }

    // Start counting the time
    lastUpdate = micros();
}

void loop() {
    // Calculate frame delta time
    unsigned long now = micros();
    float deltaTime = (float)(now - lastUpdate) / 1000000;
    lastUpdate = now;

    // Clear the sky to update clouds
    gfx.drawFilledRectangle(0, 0, 320, 240, 7);

    // Redraw the sun
    gfx.drawFilledCircle(260, 70, 15, 1);
    
    // Update and redraw clouds
    for(int i=0; i<NUMBER_OF_CLOUDS; i++) {
        clouds[i].x = (clouds[i].x + 20 * deltaTime);
        if(clouds[i].x > 2500)
            clouds[i].x -= 5000;
        
        for(int j=0; j<=2; j++) {
            for(int k=0; k<NUMBER_OF_CLOUD_COMPONENTS; k++) {
                gfx.drawFilledCircle(
                    160 + (clouds[i].x + clouds[i].component[k].x + 5*j) / clouds[i].z,
                    95 + (clouds[i].y + clouds[i].component[k].y - 3*j + 20*sin(clouds[i].component[k].t + (double)now/10000000)) / clouds[i].z,
                    (clouds[i].component[k].size - 4.5*j) / clouds[i].z,
                    cloudColors[j]
                );
            }
        }
    }

    // Update screen
    gfx.update();
}
