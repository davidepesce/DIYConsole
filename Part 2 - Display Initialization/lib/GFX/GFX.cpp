/* GFX.cpp */

#include "GFX.h"

void GFX::begin() {
    // GPIOs setup
    pinMode(GPIO_HX8357D_DC, OUTPUT);
    pinMode(GPIO_HX8357D_CS, OUTPUT);
    digitalWrite(GPIO_HX8357D_DC, LOW);
    digitalWrite(GPIO_HX8357D_CS, HIGH);

    // Start SPI
    SPI.begin();

    // Start SPI transaction
    SPI.beginTransaction(SPISettings(HX8357D_SPI_FREQUENCY, MSBFIRST, SPI_MODE0));
    digitalWrite(GPIO_HX8357D_CS, LOW);

    // Software reset, reload factory defaults
    digitalWrite(GPIO_HX8357D_DC, LOW);
    SPI.write(HX8357D_CMD_SWRESET);
    digitalWrite(GPIO_HX8357D_DC, HIGH);
    delay(6); // Wait 5ms plus a safety margin
    
    // Enable extended commands - Required for SETOSC and SETPANEL commands
    digitalWrite(GPIO_HX8357D_DC, LOW);
    SPI.write(HX8357D_CMD_SETEXC);
    digitalWrite(GPIO_HX8357D_DC, HIGH);
    SPI.write(0xFF);
    SPI.write(0x83);
    SPI.write(0x57);

    // Set internal oscillator to 100Hz for normal mode
    digitalWrite(GPIO_HX8357D_DC, LOW);
    SPI.write(HX8357D_CMD_SETOSC);
    digitalWrite(GPIO_HX8357D_DC, HIGH);
    SPI.write(0x6E);

    // Set panel to BGR
    digitalWrite(GPIO_HX8357D_DC, LOW);
    SPI.write(HX8357D_CMD_SETPANEL);
    digitalWrite(GPIO_HX8357D_DC, HIGH);
    SPI.write(0x05);

    // Set pixel format to 16 bit
    digitalWrite(GPIO_HX8357D_DC, LOW);
    SPI.write(HX8357D_CMD_COLMOD);
    digitalWrite(GPIO_HX8357D_DC, HIGH);
    SPI.write(0x55);

    // Sleep out
    digitalWrite(GPIO_HX8357D_DC, LOW);
    SPI.write(HX8357D_CMD_SLPOUT);
    digitalWrite(GPIO_HX8357D_DC, HIGH);
    delay(6); // Wait 5ms plus a safety margin

    // Display on
    digitalWrite(GPIO_HX8357D_DC, LOW);
    SPI.write(HX8357D_CMD_DISPON);
    digitalWrite(GPIO_HX8357D_DC, HIGH);

    // End SPI transaction
    digitalWrite(GPIO_HX8357D_CS, HIGH);
    SPI.endTransaction();
}