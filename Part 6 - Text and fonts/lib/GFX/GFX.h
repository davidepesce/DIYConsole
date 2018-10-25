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
#define ONSCREEN(x,y) (x >= 0 && x < 320 && y >= 0 && y < 480)

struct Font {
    uint8_t* data;
    uint8_t width;
    uint8_t height;
};

class GFX {
    public:
    void begin();
    void update();
    void fillScreen(uint8_t color);
    void drawPixel(uint16_t x, uint16_t y, uint8_t color);
    void drawHorizontalLine(int16_t x, int16_t y, uint16_t width, uint8_t color);
    void drawVerticalLine(int16_t x, int16_t y, uint16_t height, uint8_t color);
    void drawLine(int16_t xStart, int16_t yStart, int16_t xEnd, int16_t yEnd, uint8_t color);
    void drawRectangle(int16_t x, int16_t y, uint16_t width, uint16_t height, uint8_t color);
    void drawFilledRectangle(int16_t x, int16_t y, uint16_t width, uint16_t height, uint8_t color);
    void drawTriangle(int16_t x0, int16_t y0,int16_t x1, int16_t y1,int16_t x2, int16_t y2, uint8_t color);
    void drawFilledTriangle(int16_t x0, int16_t y0,int16_t x1, int16_t y1,int16_t x2, int16_t y2, uint8_t color);
    void drawCircle(int16_t x, int16_t y, uint16_t radius, uint8_t color);
    void drawFilledCircle(int16_t x, int16_t y, uint16_t radius, uint8_t color);
    void drawBitmap(uint8_t* bitmap, int16_t x, int16_t y, uint16_t width, uint16_t height);
    void drawTransparentBitmap(uint8_t* bitmap, int16_t x, int16_t y, uint16_t width, uint16_t height, uint8_t transparentColor);
    void scaleAndRotateBitmap(uint8_t* destination, uint16_t* destinationWidth, uint16_t* destinationHeight,
            uint8_t* source, uint16_t sourceWidth, uint16_t sourceHeight, float scaleX, float scaleY, float rotation, uint_fast16_t backgroundColor = 0);
    void copyScreenBufferRect(uint8_t* buffer, int16_t x, int16_t y, uint16_t width, uint16_t height);
    void drawMonochromeBitmap(uint8_t* bitmap, int16_t x, int16_t y, uint16_t width, uint16_t height, uint8_t color);
    void drawMonochromeBitmap2x(uint8_t* bitmap, int16_t x, int16_t y, uint16_t width, uint16_t height, uint8_t color);
    void setFont(Font* f);
    void drawChar(int16_t x, int16_t y, char character, uint8_t color);
    void drawChar2x(int16_t x, int16_t y, char character, uint8_t color);
    void drawString(int16_t x, int16_t y, const char* string, uint8_t color);
    void drawString2x(int16_t x, int16_t y, const char* string, uint8_t color);
    void loadDefaultPalette();
    void loadPalette(uint16_t* newPalette, int size);

    private:
    uint8_t* screenBuffer[2]; // 0 => Top sector, 1 => Bottom sector
    uint16_t palette[256];
    Font* font;
    uint16_t fontSize;

    void setAddressWindow(uint16_t x, uint16_t y, uint16_t width, uint16_t height);
    int16_t cropToViewSize(int16_t* start, uint16_t* length, uint16_t viewSize);
};

#endif
