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

    // Load default 16 color palette and fill screen with black (index 15)
    loadDefaultPalette();
    fillScreen(15);
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

int16_t GFX::cropToViewSize(int16_t* start, uint16_t* length, uint16_t viewSize) {
    // Check if the line is completely outside the screen
    if(*start >= viewSize) {
        *start = 0;
        *length = 0;
        return -1;
    }
    int16_t end = *start + *length - 1;
    if(end < 0) {
        *start = 0;
        *length = 0;
        return -1;
    }

    // If we get here, the line is at least partially on the screen.
    // Check if it starts outside the screen and recalculate the length if necessary.
    if(*start < 0) {
        *length += *start;
        *start = 0;
    }

    // Check if the line ends outside the screen and recalculate the length if necessary.
    *length = (end < viewSize) ? *length : (viewSize - *start);

    // Return the end coordinate of the line
    return (*start + *length - 1);
}

void GFX::fillScreen(uint8_t color) {
    memset(screenBuffer[0], color, 81920);
    memset(screenBuffer[1], color, 71680);
}

void GFX::drawPixel(uint16_t x, uint16_t y, uint8_t color) {
    int sector = SCREENBUFFER_SECTOR(y);
    if(sector)
        y = y - 256;
    int index = 320 * y + x;
    screenBuffer[sector][index] = color;
}

void GFX::drawHorizontalLine(int16_t x, int16_t y, uint16_t width, uint8_t color) {
    int sector = SCREENBUFFER_SECTOR(y);
    if(sector)
        y = y - 256;
    int offset = 320 * y + x; 
    memset(screenBuffer[sector] + offset, color, width);
}

void GFX::drawVerticalLine(int16_t x, int16_t y, uint16_t height, uint8_t color) {
    int16_t y2 = y + height - 1;
    int startSector = SCREENBUFFER_SECTOR(y);
    int endSector = SCREENBUFFER_SECTOR(y2);
    if(startSector == endSector) {
        if(startSector) {
            y = y - 256;
            y2 = y2 - 256;
        }
        for( ; y <= y2; y++)
            screenBuffer[startSector][320 * y + x] = color;
    } else {
        y2 = y2 - 256;
        for( ; y < 256; y++)
            screenBuffer[0][320 * y + x] = color;
        for(y = 0; y <= y2; y++)
            screenBuffer[1][320 * y + x] = color;
    }
}

void GFX::drawLine(int16_t xStart, int16_t yStart, int16_t xEnd, int16_t yEnd, uint8_t color) {
    // Check for horizontal/vertical line to use faster functions
    if(yStart == yEnd) {
        // Check if the line is outside the screen
        if(yStart < 0 || yStart >= 480)
            return;

        // Sort horizontal coordinates
        if(xEnd < xStart) {
            int16_t tmp;
            tmp = xStart; xStart = xEnd; xEnd = tmp;
        }

        // Check if the line is outside the screen
        if(xEnd < 0 || xStart >= 320)
            return;
        
        // Draw line
        uint16_t width = xEnd - xStart + 1;
        cropToViewSize(&xStart, &width, 320);
        drawHorizontalLine(xStart, yStart, width, color);
        return;
    } else if(xStart == xEnd) {
        // Check if the line is outside the screen
        if(xStart < 0 || xStart >= 320)
            return;        

        // Sort vertical coordinates
        if(yEnd < yStart) {
            int16_t tmp;
            tmp = yStart; yStart = yEnd; yEnd = tmp;
        }

        // Check if the line is outside the screen
        if(yEnd < 0 || yStart >= 480)
            return;
        
        // Draw line
        uint16_t height = yEnd - yStart + 1;
        cropToViewSize(&yStart, &height, 480);
        drawVerticalLine(xStart, yStart, height, color);
        return;
    }

    // Bresenham's line algorithm
    int16_t steep = abs(yEnd - yStart) > abs(xEnd - xStart);
    if (steep) {
        int16_t tmp;
        tmp = xStart; xStart = yStart; yStart = tmp;
        tmp = xEnd; xEnd = yEnd; yEnd = tmp;
    }
    if (xStart > xEnd) {
        int16_t tmp;
        tmp = xStart; xStart = xEnd; xEnd = tmp;
        tmp = yStart; yStart = yEnd; yEnd = tmp;
    }

    int16_t dx, dy;
    dx = xEnd - xStart;
    dy = abs(yEnd - yStart);

    int16_t err = dx / 2;
    int16_t yStep;

    if (yStart < yEnd) {
        yStep = 1;
    } else {
        yStep = -1;
    }

    for ( ; xStart<=xEnd; xStart++) {
        if (steep) {
            if(ONSCREEN(yStart, xStart))
                drawPixel(yStart, xStart, color);
        } else {
            if(ONSCREEN(xStart, yStart))
                drawPixel(xStart, yStart, color);
        }
        err -= dy;
        if (err < 0) {
            yStart += yStep;
            err += dx;
        }
    }
}

void GFX::drawRectangle(int16_t x, int16_t y, uint16_t width, uint16_t height, uint8_t color) {
    drawLine(x, y, x + width - 1, y, color);
    drawLine(x, y + height -1, x + width - 1, y + height - 1, color);
    drawLine(x, y, x, y + height - 1, color);
    drawLine(x + width - 1, y, x + width - 1, y + height - 1, color);
}

void GFX::drawFilledRectangle(int16_t x, int16_t y, uint16_t width, uint16_t height, uint8_t color) {
    // Check if the rectangle is outside the screen
    if(x >= 320 || y >= 480)
        return;
    if(x + width - 1 < 0)
        return;
    if(y + height - 1 < 0)
        return;

    // Draw the filled rectangle
    int16_t y2;
    cropToViewSize(&x, &width, 320);
    y2 = cropToViewSize(&y, &height, 480);
    for( ; y <= y2; y++)
        drawHorizontalLine(x, y, width, color);
}

void GFX::drawTriangle(int16_t x0, int16_t y0,int16_t x1, int16_t y1,int16_t x2, int16_t y2, uint8_t color) {
    drawLine(x0, y0, x1, y1, color);
    drawLine(x1, y1, x2, y2, color);
    drawLine(x2, y2, x0, y0, color);
}

void GFX::drawFilledTriangle(int16_t x0, int16_t y0,int16_t x1, int16_t y1,int16_t x2, int16_t y2, uint8_t color) {
    int dx01, dx02, dx12, dy01, dy02, dy12;
    int u01, u02, u12;

    // Sort vertices by y value
    if(y0 > y1) {
        int16_t tmp;
        tmp = y0; y0 = y1; y1 = tmp;
        tmp = x0; x0 = x1; x1 = tmp;
    }
    if(y0 > y2) {
        int16_t tmp;
        tmp = y0; y0 = y2; y2 = tmp;
        tmp = x0; x0 = x2; x2 = tmp;
    }
    if(y1 > y2) {
        int16_t tmp;
        tmp = y1; y1 = y2; y2 = tmp;
        tmp = x1; x1 = x2; x2 = tmp;
    }

    // Set up long side (0 => 2)
    dx02 = x2 - x0;
    dy02 = y2 - y0;
    u02 = x0 * dy02;

    // Upper triangle
    if(y1 > y0) {
        // Set up upper side (0 => 1)
        dx01 = x1 - x0;
        dy01 = y1 - y0;
        u01 = x0 * dy01;

        // Draw upper triangle
        for(int y=y0; y<y1; y++) {
            int16_t xStart = u01 / dy01;
            int16_t xEnd = u02 / dy02;
            if(xEnd > xStart) {
                if(y >= 0 && y < 480) {
                    uint16_t width = xEnd - xStart + 1;
                    cropToViewSize(&xStart, &width, 320);
                    drawHorizontalLine(xStart, y, width, color);
                }
            } else {
                if(y >= 0 && y < 480) {
                    uint16_t width = xStart - xEnd + 1;
                    cropToViewSize(&xEnd, &width, 320);
                    drawHorizontalLine(xEnd, y, width, color);
                }
            }

            // Next line
            u02 = u02 + dx02;
            u01 = u01 + dx01;
        }
    }

    // Lower triangle
    if(y2 > y1) {
        // Set up lower side (1 => 2)
        dx12 = x2 - x1;
        dy12 = y2 - y1;
        u12 = x1 * dy12;

        // Draw lower triangle
        for(int y=y1; y<=y2; y++) {
            int16_t xStart = u12 / dy12;
            int16_t xEnd = u02 / dy02;
            if(xEnd > xStart) {
                if(y >= 0 && y < 480) {
                    uint16_t width = xEnd - xStart + 1;
                    cropToViewSize(&xStart, &width, 320);
                    drawHorizontalLine(xStart, y, width, color);
                }
            } else {
                if(y >= 0 && y < 480) {
                    uint16_t width = xStart - xEnd + 1;
                    cropToViewSize(&xEnd, &width, 320);
                    drawHorizontalLine(xEnd, y, width, color);
                }
            }

            // Next line
            u02 = u02 + dx02;
            u12 = u12 + dx12;
        }
    } else {
        // y1 == y2
        int16_t xStart = x1;
        int16_t xEnd = x2;
        if(xEnd > xStart) {
            if(y1 >= 0 && y1 < 480) {
                uint16_t width = xEnd - xStart + 1;
                cropToViewSize(&xStart, &width, 320);
                drawHorizontalLine(xStart, y1, width, color);
            }
        } else {
            if(y1 >= 0 && y1 < 480) {
                uint16_t width = xStart - xEnd + 1;
                cropToViewSize(&xEnd, &width, 320);
                drawHorizontalLine(xEnd, y1, width, color);
            }
        }
    }
}

void GFX::drawCircle(int16_t x, int16_t y, uint16_t radius, uint8_t color) {
    int16_t px = radius;
    int16_t py = 0;
    int16_t dx = 1 - 2 * radius;
    int16_t dy = 1;
    int16_t err = 0;

    while(px >= py) {
        if(ONSCREEN(x + px, y + py)) drawPixel(x + px, y + py, color);
        if(ONSCREEN(x + py, y + px)) drawPixel(x + py, y + px, color);
        if(ONSCREEN(x - py, y + px)) drawPixel(x - py, y + px, color);
        if(ONSCREEN(x - px, y + py)) drawPixel(x - px, y + py, color);
        if(ONSCREEN(x - px, y - py)) drawPixel(x - px, y - py, color);
        if(ONSCREEN(x - py, y - px)) drawPixel(x - py, y - px, color);
        if(ONSCREEN(x + py, y - px)) drawPixel(x + py, y - px, color);
        if(ONSCREEN(x + px, y - py)) drawPixel(x + px, y - py, color);

        py++;
        err += dy;
        dy += 2;
        if(2 * err + dx > 0) {
            px--;
            err += dx;
            dx += 2;
        }
    }
}

void GFX::drawFilledCircle(int16_t x, int16_t y, uint16_t radius, uint8_t color) {
    int16_t px = radius;
    int16_t py = 0;
    int16_t dx = 1 - 2 * radius;
    int16_t dy = 1;
    int16_t err = 0;

    while(px >= py) {
        int16_t xStart, yStart;
        uint16_t width;
        
        xStart = x - px; yStart = y + py; width = 2 * px + 1;
        if(yStart >= 0 && yStart < 480) {
            cropToViewSize(&xStart, &width, 320);
            drawHorizontalLine(xStart, yStart, width, color);
        }
        xStart = x - py; yStart = y + px; width = 2 * py + 1;
        if(yStart >= 0 && yStart < 480) {
            cropToViewSize(&xStart, &width, 320);
            drawHorizontalLine(xStart, yStart, width, color);
        }
        xStart = x - px; yStart = y - py; width = 2 * px + 1;
        if(yStart >= 0 && yStart < 480) {
            cropToViewSize(&xStart, &width, 320);
            drawHorizontalLine(xStart, yStart, width, color);
        }
        xStart = x - py; yStart = y - px; width = 2 * py + 1;
        if(yStart >= 0 && yStart < 480) {
            cropToViewSize(&xStart, &width, 320);
            drawHorizontalLine(xStart, yStart, width, color);
        }

        py++;
        err += dy;
        dy += 2;
        if(2 * err + dx > 0) {
            px--;
            err += dx;
            dx += 2;
        }
    }
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