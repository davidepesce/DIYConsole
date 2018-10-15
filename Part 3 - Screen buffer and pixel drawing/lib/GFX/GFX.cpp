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

    // Allocate framebuffer
    screenBuffer[0] = (uint8_t*)malloc(81920); // 320x256 pixels
    screenBuffer[1] = (uint8_t*)malloc(71680); // 320x224 pixels

    // Load default 16 color palette and fill framebuffer with black (index 15)
    loadDefaultPalette();
    memset(screenBuffer[0], 15, 81920);
    memset(screenBuffer[1], 15, 71680);
}

void GFX::update() {
    uint16_t buffer[320];
    
    // Start SPI transaction
    SPI.beginTransaction(SPISettings(HX8357D_SPI_FREQUENCY, MSBFIRST, SPI_MODE0));
    digitalWrite(GPIO_HX8357D_CS, LOW);

    // Top framebuffer sector
    for(int y=0; y<256; y++) {
        int offset = 320 * y;
        for(int x=0; x<320; x++)
            buffer[x] = palette[screenBuffer[0][offset + x]];
        setAddressWindow(0, y, 320, 1);
        SPI.writePixels(buffer, 640);
    }

    // Bottom framebuffer sector
    for(int y=0; y<224; y++) {
        int offset = 320 * y;
        for(int x=0; x<320; x++)
            buffer[x] = palette[screenBuffer[1][offset + x]];
        setAddressWindow(0, y + 256, 320, 1);
        SPI.writePixels(buffer, 640);
    }

    // End SPI transaction
    digitalWrite(GPIO_HX8357D_CS, HIGH);
    SPI.endTransaction();
}

void GFX::setAddressWindow(uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
    uint32_t columnAddress = ((uint32_t)x << 16) | (x + width - 1);
    uint32_t rowAddress = ((uint32_t)y << 16) | (y + height - 1);

    // Column address set
    digitalWrite(GPIO_HX8357D_DC, LOW);
    SPI.write(HX8357D_CMD_CASET);
    digitalWrite(GPIO_HX8357D_DC, HIGH);
    SPI.write32(columnAddress);

    // Row address set
    digitalWrite(GPIO_HX8357D_DC, LOW);
    SPI.write(HX8357D_CMD_PASET);
    digitalWrite(GPIO_HX8357D_DC, HIGH);
    SPI.write32(rowAddress);

    // Write to RAM
    digitalWrite(GPIO_HX8357D_DC, LOW);
    SPI.write(HX8357D_CMD_RAMWR);
    digitalWrite(GPIO_HX8357D_DC, HIGH);
}

void GFX::drawPixel(uint16_t x, uint16_t y, uint8_t color) {
    int sector = SCREENBUFFER_SECTOR(y);
    if(sector)
        y = y - 256;
    int index = 320 * y + x;
    screenBuffer[sector][index] = color;
}

void GFX::loadDefaultPalette() {
    memset(palette, 0, 512);
    palette[0] =  RGB565(0xFF, 0xFF, 0xFF); // White
    palette[1] =  RGB565(0xFF, 0xFF, 0x00); // Yellow
    palette[2] =  RGB565(0xFF, 0xA5, 0x00); // Orange
    palette[3] =  RGB565(0xFF, 0x00, 0x00); // Red
    palette[4] =  RGB565(0xEE, 0x82, 0xEE); // Violet
    palette[5] =  RGB565(0x4B, 0x00, 0x82); // Indigo
    palette[6] =  RGB565(0x00, 0x00, 0xFF); // Blue
    palette[7] =  RGB565(0x00, 0xBF, 0xFF); // DeepSkyBlue
    palette[8] =  RGB565(0x32, 0xCD, 0x32); // LimeGreen
    palette[9] =  RGB565(0x00, 0x64, 0x00); // DarkGreen
    palette[10] = RGB565(0x8B, 0x45, 0x13); // SaddleBrown
    palette[11] = RGB565(0xD2, 0xB4, 0x8C); // Tan
    palette[12] = RGB565(0xD3, 0xD3, 0xD3); // LightGray
    palette[13] = RGB565(0xA9, 0xA9, 0xA9); // DarkGray
    palette[14] = RGB565(0x69, 0x69, 0x69); // DimGray
    palette[15] = RGB565(0x00, 0x00, 0x00); // Black
}

void GFX::loadPalette(uint16_t* newPalette, int size) {
    memset(palette, 0, 512);
    memcpy(palette, newPalette, 2 * size);
}