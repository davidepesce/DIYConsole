/* GFX.h */

#ifndef _GFX_H
#define _GFX_H

#include <Arduino.h>
#include <SPI.h>

#define HX8357D_SPI_FREQUENCY   40000000
#define GPIO_HX8357D_CS         15  // Chip select line
#define GPIO_HX8357D_DC         33  // Data-Command line

// HX8357-D Commands
#define HX8357D_CMD_SWRESET     0x01
#define HX8357D_CMD_SLPOUT      0x11
#define HX8357D_CMD_DISPON      0x29
#define HX8357D_CMD_CASET       0x2A
#define HX8357D_CMD_PASET       0x2B
#define HX8357D_CMD_RAMWR       0x2C
#define HX8357D_CMD_COLMOD      0x3A
#define HX8357D_CMD_SETOSC      0xB0
#define HX8357D_CMD_SETEXC      0xB9
#define HX8357D_CMD_SETPANEL    0xCC

#define RGB565(r,g,b) (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3))   // 24 bit RGB to 16 bit RGB565
#define SCREENBUFFER_SECTOR(y)  (y < 256) ? 0 : 1

class GFX {
    public:
    void begin();
    void update();
    void drawPixel(uint16_t x, uint16_t y, uint8_t color);
    void loadDefaultPalette();
    void loadPalette(uint16_t* newPalette, int size);

    private:
    uint8_t* screenBuffer[2]; // 0 => Top sector, 1 => Bottom sector
    uint16_t palette[256];

    void setAddressWindow(uint16_t x, uint16_t y, uint16_t width, uint16_t height);
};

#endif
