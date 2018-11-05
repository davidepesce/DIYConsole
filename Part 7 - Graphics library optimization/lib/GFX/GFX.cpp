/* GFX.cpp */

#include "GFX.h"

float sinTable[91] = {
    0,
    0.017452406,
    0.034899497,
    0.052335956,
    0.069756474,
    0.087155743,
    0.104528463,
    0.121869343,
    0.139173101,
    0.156434465,
    0.173648178,
    0.190808995,
    0.207911691,
    0.224951054,
    0.241921896,
    0.258819045,
    0.275637356,
    0.292371705,
    0.309016994,
    0.325568154,
    0.342020143,
    0.35836795,
    0.374606593,
    0.390731128,
    0.406736643,
    0.422618262,
    0.438371147,
    0.4539905,
    0.469471563,
    0.48480962,
    0.5,
    0.515038075,
    0.529919264,
    0.544639035,
    0.559192903,
    0.573576436,
    0.587785252,
    0.601815023,
    0.615661475,
    0.629320391,
    0.64278761,
    0.656059029,
    0.669130606,
    0.68199836,
    0.69465837,
    0.707106781,
    0.7193398,
    0.731353702,
    0.743144825,
    0.75470958,
    0.766044443,
    0.777145961,
    0.788010754,
    0.79863551,
    0.809016994,
    0.819152044,
    0.829037573,
    0.838670568,
    0.848048096,
    0.857167301,
    0.866025404,
    0.874619707,
    0.882947593,
    0.891006524,
    0.898794046,
    0.906307787,
    0.913545458,
    0.920504853,
    0.927183855,
    0.933580426,
    0.939692621,
    0.945518576,
    0.951056516,
    0.956304756,
    0.961261696,
    0.965925826,
    0.970295726,
    0.974370065,
    0.978147601,
    0.981627183,
    0.984807753,
    0.987688341,
    0.990268069,
    0.992546152,
    0.994521895,
    0.996194698,
    0.99756405,
    0.998629535,
    0.999390827,
    0.999847695,
    1
};

float fastSin(int deg) {
    while(deg < 0)
        deg += 360;
    while(deg > 360)
        deg -= 360;

    if(deg <= 90) {
        return sinTable[deg];
    } else if(deg <= 180) {
        return sinTable[180-deg];
    } else if(deg <= 270) {
        return -sinTable[deg-180];
    } else {
        return -sinTable[360-deg];
    }
}

float fastCos(int deg) {
    while(deg < 0)
        deg += 360;
    while(deg > 360)
        deg -= 360;
    
    if(deg <= 90) {
        return sinTable[90-deg];
    } else if(deg <= 180) {
        return -sinTable[deg-90];
    } else if(deg <= 270) {
        return -sinTable[270-deg];
    } else {
        return sinTable[deg-270];
    }
}

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

    // Starting scan line
    scanLine = 0;
}

void GFX::update() {
    uint16_t buffer[64];
    
    // By default, all the lines need to be redrawn
    int startY = 0;
    int stepY = 1;
    int y;

    // If the number of rectangles to redraw is greater than 900, switch
    // to interlaced mode: only the odd or even rows are checked, based
    // on the current scan line.
    int dirtyRectsCount = 0;
    for(int rect = 0; rect < 5; rect++)
        for(int v = 0; v < 480; v++)
            dirtyRectsCount += dirtyRects[v][rect];
    if(dirtyRectsCount > 900) {
        stepY = 2;
        startY = scanLine;
    }
    
    // Start SPI transaction
    SPI.beginTransaction(SPISettings(HX8357D_SPI_FREQUENCY, MSBFIRST, SPI_MODE0));
    digitalWrite(GPIO_HX8357D_CS, LOW);

    // Top framebuffer sector
    for(y = startY; y < 256; y = y + stepY) {
        for(int rect = 0; rect < 5; rect++) {
            if(dirtyRects[y][rect]) {
                int xStart = rect << 6;
                int offset = 320 * y;
                for(int x = 0; x < 64; x++)
                    buffer[x] = palette[screenBuffer[0][offset + xStart + x]];
                setAddressWindow(xStart, y, 64, 1);
                SPI.writePixels(buffer, 128);
            }
        }
        // Reset dirty rects for this line
        memset(&dirtyRects[y][0], false, 5);
    }

    // Bottom framebuffer sector
    for(y = y - 256; y < 224; y = y + stepY) {
        int realY = y + 256;
        for(int rect = 0; rect < 5; rect++) {
            if(dirtyRects[realY][rect]) {
                int xStart = rect << 6;
                int offset = 320 * y;
                for(int x = 0; x < 64; x++)
                    buffer[x] = palette[screenBuffer[1][offset + xStart + x]];
                setAddressWindow(xStart, realY, 64, 1);
                SPI.writePixels(buffer, 128);
            }
        }
        // Reset dirty rects for this line
        memset(&dirtyRects[realY][0], false, 5);
    }

    // End SPI transaction
    digitalWrite(GPIO_HX8357D_CS, HIGH);
    SPI.endTransaction();

    // Switch the scan line between even and odd
    scanLine = (scanLine + 1) % 2;
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
    memset(dirtyRects, true, 2400);
}

void GFX::drawPixel(uint16_t x, uint16_t y, uint8_t color) {
    // Set dirty rectangle
    dirtyRects[y][DIRTY_RECT_X(x)] = true;

    // Draw pixel
    int sector = SCREENBUFFER_SECTOR_2(y);
    if(sector)
        y = y - 256;
    int index = 320 * y + x;
    screenBuffer[sector][index] = color;
}

void GFX::drawHorizontalLine(int16_t x, int16_t y, uint16_t width, uint8_t color) {
    // Set dirty rectangles
    int r1 = DIRTY_RECT_X(x);
    int r2 = DIRTY_RECT_X(x + width - 1);
    for(int i = r1; i <= r2; i++)
        dirtyRects[y][i] = true;

    // Draw line
    int sector = SCREENBUFFER_SECTOR_2(y);
    if(sector)
        y = y - 256;
    int offset = 320 * y + x; 
    memset(screenBuffer[sector] + offset, color, width);
}

void GFX::drawVerticalLine(int16_t x, int16_t y, uint16_t height, uint8_t color) {
    // Set dirty rectangles
    int r = DIRTY_RECT_X(x);
    for(int i = y; i <= y + height - 1; i++)
        dirtyRects[i][r] = true;

    // Draw line
    int16_t y2 = y + height - 1;
    int startSector = SCREENBUFFER_SECTOR_2(y);
    int endSector = SCREENBUFFER_SECTOR_2(y2);
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

void GFX::drawBitmap(uint8_t* bitmap, int16_t x, int16_t y, uint16_t width, uint16_t height) {
    // Check if bitmap is outside the screen
    if(x >= 320) return;
    if(y >= 480) return;
    if(x + width - 1 < 0) return;
    if(y + height - 1 < 0) return;

    // Offset to get the bitmap pixels
    int16_t uOffset = -x;
    int16_t vOffset = -y;

    // Calculate the visible part of the bitmap
    uint16_t bitmapWidth = width;
    cropToViewSize(&x, &width, 320);
    int16_t yEnd = cropToViewSize(&y, &height, 480);

    // Copy the bitmap to screen buffer line by line
    uint16_t u = x + uOffset;
    uint16_t v = y + vOffset;
    int r1 = DIRTY_RECT_X(x);
    int r2 = DIRTY_RECT_X(x + width - 1);
    for( ; y <= yEnd ; y++, v++) {
        int bitmapOffset = bitmapWidth * v + u;
        if(SCREENBUFFER_SECTOR(y)) {
            int ysb = y - 256;
            int screenBufferOffset = 320 * ysb + x;
            memcpy(screenBuffer[1] + screenBufferOffset, bitmap + bitmapOffset, width);
        } else {
            int screenBufferOffset = 320 * y + x;
            memcpy(screenBuffer[0] + screenBufferOffset, bitmap + bitmapOffset, width);
        }
        for(int i = r1; i <= r2; i++)
            dirtyRects[y][i] = true;
    }

}

void GFX::drawTransparentBitmap(uint8_t* bitmap, int16_t x, int16_t y, uint16_t width, uint16_t height, uint8_t transparentColor) {
    // Check if bitmap is outside the screen
    if(x >= 320) return;
    if(y >= 480) return;
    if(x + width - 1 < 0) return;
    if(y + height - 1 < 0) return;

    // Offset to get the bitmap pixels
    int16_t uOffset = -x;
    int16_t vOffset = -y;

    // Calculate the visible part of the bitmap
    uint16_t bitmapWidth = width;
    int16_t xEnd = cropToViewSize(&x, &width, 320);
    int16_t yEnd = cropToViewSize(&y, &height, 480);

    // Copy the bitmap to screen buffer
    uint16_t uStart = x + uOffset;
    int r1 = DIRTY_RECT_X(x);
    int r2 = DIRTY_RECT_X(xEnd);
    for(uint16_t v = y + vOffset; y <= yEnd; y++, v++) {
        uint16_t u = uStart;
        for(uint16_t xp = x; xp <= xEnd; xp++, u++) {
            int bitmapOffset = bitmapWidth * v + u;
            if(bitmap[bitmapOffset] != transparentColor) {
                if(SCREENBUFFER_SECTOR(y)) {
                    int ysb = y - 256;
                    int screenBufferOffset = 320 * ysb + xp;
                    screenBuffer[1][screenBufferOffset] = bitmap[bitmapOffset];
                } else {
                    int screenBufferOffset = 320 * y + xp;
                    screenBuffer[0][screenBufferOffset] = bitmap[bitmapOffset];
                }
            }
        }
        for(int i = r1; i <= r2; i++)
            dirtyRects[y][i] = true;
    }
}

void GFX::scaleAndRotateBitmap(uint8_t* destination, uint16_t* destinationWidth, uint16_t* destinationHeight,
            uint8_t* source, uint16_t sourceWidth, uint16_t sourceHeight, float scaleX, float scaleY, float rotation, uint_fast16_t backgroundColor) {
    float sinTheta = fastSin(rotation);
    float cosTheta = fastCos(rotation);

    // Calculate width and height of resulting bitmap
    int w1 = scaleX * sourceWidth * cosTheta + scaleY * sourceHeight * sinTheta;
    int w2 = scaleX * sourceWidth * cosTheta - scaleY * sourceHeight * sinTheta;
    *destinationWidth = max(abs(w1), abs(w2));
    int h1 = scaleX * sourceWidth * sinTheta + scaleY * sourceHeight * cosTheta;
    int h2 = scaleX * sourceWidth * sinTheta - scaleY * sourceHeight * cosTheta;
    *destinationHeight = max(abs(h1), abs(h2));

    // Map every pixel of destination bitmap in source bitmap
    float sourceCenterX = 0.5 * (sourceWidth - 1);
    float sourceCenterY = 0.5 * (sourceHeight - 1);
    float destinationCenterX = 0.5 * (*destinationWidth - 1);
    float destinationCenterY = 0.5 * (*destinationHeight - 1);

    for(float v = -destinationCenterY; v <= destinationCenterY; v = v + 1) {
        for(float u = -destinationCenterX; u <= destinationCenterX; u = u + 1) {
            int sx = sourceCenterX + (cosTheta * u + sinTheta * v) / scaleX;
            int sy = sourceCenterY + (cosTheta * v - sinTheta * u) / scaleY;

            int destinationOffset = *destinationWidth * (v + destinationCenterY) + u + destinationCenterX;
            if(sx >= 0 && sx < sourceWidth && sy >= 0 && sy < sourceHeight) {
                int sourceOffset = sourceWidth * sy + sx;
                destination[destinationOffset] = source[sourceOffset];
            } else {
                destination[destinationOffset] = backgroundColor;
            }
        }
    }
}

void GFX::drawMonochromeBitmap(uint8_t* bitmap, int16_t x, int16_t y, uint16_t width, uint16_t height, uint8_t color) {
    int widthBytes = (width + 7) >> 3;
    for(int v=0; v<height; v++) {
        for(int u=0; u<width; u++) {
            if(ONSCREEN(x + u, y + v)) {
                int b = u >> 3;     // the byte to select
                int bit = ~u & 7;   // the bit to read
                if(bitRead(bitmap[widthBytes * v + b], bit))
                    drawPixel(x + u, y + v, color);
            }
        }
    }
}

void GFX::drawMonochromeBitmap2x(uint8_t* bitmap, int16_t x, int16_t y, uint16_t width, uint16_t height, uint8_t color) {
    // Draw bitmap with 2x scaling factor usign Scale2x algorithm
    int widthBytes = (width + 7) >> 3;
    for(int v=0; v<height; v++) {
        for(int u=0; u<width; u++) {
            int b = u >> 3;
            int bit = ~u & 7;
            uint8_t pixelValue = bitRead(bitmap[widthBytes * v + b], bit);
            uint8_t subpixel[4] = {pixelValue, pixelValue, pixelValue, pixelValue};
            // Pixel above the current one
            uint8_t valueA = 0;
            if(v - 1 >= 0)
                valueA = bitRead(bitmap[widthBytes * (v - 1) + b], bit);
            // Pixel below the current one
            uint8_t valueD = 0;
            if(v + 1 < height)
                valueD = bitRead(bitmap[widthBytes * (v + 1) + b], bit);
            // Pixel to the left of the current one
            int uC = u - 1;
            uint8_t valueC = 0;
            if(uC >= 0) {
                b = uC >> 3;
                bit = ~uC & 7;
                valueC = bitRead(bitmap[widthBytes * v + b], bit);
            }
            // Pixel to the right of the current one
            int uB = u + 1;
            uint8_t valueB = 0;
            if(uB < width) {
                b = uB >> 3;
                bit = ~uB & 7;
                valueB = bitRead(bitmap[widthBytes * v + b], bit);
            }
            if(valueC == valueA && valueC != valueD && valueA != valueB)
                subpixel[0] = valueA;
            if(valueA == valueB && valueA != valueC && valueB != valueD)
                subpixel[1] = valueB;
            if(valueC == valueD && valueB != valueD && valueA != valueC)
                subpixel[2] = valueC;
            if(valueD == valueB && valueA != valueB && valueC != valueD)
                subpixel[3] = valueD;
            
            if(ONSCREEN(x + 2 * u, y + 2 * v) && subpixel[0])
                drawPixel(x + 2 * u, y + 2 * v, color);
            if(ONSCREEN(x + 2 * u + 1, y + 2 * v) && subpixel[1])
                drawPixel(x + 2 * u + 1, y + 2 * v, color);
            if(ONSCREEN(x + 2 * u, y + 2 * v + 1) && subpixel[2])
                drawPixel(x + 2 * u, y + 2 * v + 1, color);
            if(ONSCREEN(x + 2 * u + 1, y + 2 * v + 1) && subpixel[3])
                drawPixel(x + 2 * u + 1, y + 2 * v + 1, color);
        }
    }
}

void GFX::copyScreenBufferRect(uint8_t* buffer, int16_t x, int16_t y, uint16_t width, uint16_t height) {
    // Check if rect is outside the screen
    if(x >= 320) return;
    if(y >= 480) return;
    if(x + width - 1 < 0) return;
    if(y + height - 1 < 0) return;

    // Offset to get the rect pixels
    int16_t uOffset = -x;
    int16_t vOffset = -y;

    // Calculate the visible part of the bitmap
    uint16_t rectWidth = width;
    cropToViewSize(&x, &width, 320);
    int16_t yEnd = cropToViewSize(&y, &height, 480);

    // Copy the screen buffer rect to the buffer line by line
    uint16_t u = x + uOffset;
    uint16_t v = y + vOffset;
    for( ; y <= yEnd ; y++, v++) {
        int rectOffset = rectWidth * v + u;
        if(SCREENBUFFER_SECTOR(y)) {
            int ysb = y - 256;
            int screenBufferOffset = 320 * ysb + x;
            memcpy(buffer + rectOffset, screenBuffer[1] + screenBufferOffset, width);
        } else {
            int screenBufferOffset = 320 * y + x;
            memcpy(buffer + rectOffset, screenBuffer[0] + screenBufferOffset, width);
        }
    }
}

void GFX::setFont(Font* f) {
    font = f;
    fontSize = ((font->width + 7) >> 3) * font->height;
}

void GFX::drawChar(int16_t x, int16_t y, char character, uint8_t color) {
    uint8_t* charBitmap = font->data + fontSize * character;
    drawMonochromeBitmap(charBitmap, x, y, font->width, font->height, color);
}

void GFX::drawChar2x(int16_t x, int16_t y, char character, uint8_t color) {
    uint8_t* charBitmap = font->data + fontSize * character;
    drawMonochromeBitmap2x(charBitmap, x, y, font->width, font->height, color);
}

void GFX::drawString(int16_t x, int16_t y, const char* string, uint8_t color) {
    int16_t cx = 0, cy = 0; // Relative cursor position
    uint8_t c;
    int i = 0;
    while((c = *(string + i)) != 0) {
        if(c == 0x08) {
            // Backspace
            cx -= font->width;
        } else if(c == 0x0A) {
            // New line
            cx = 0;
            cy += font->height;
        } else {
            // Draw character
            drawChar(x + cx, y + cy, c, color);
            cx += font->width;
        }
        // Next character
        i++;
    }
}

void GFX::drawString2x(int16_t x, int16_t y, const char* string, uint8_t color) {
    int16_t cx = 0, cy = 0; // Relative cursor position
    uint8_t c;
    int i = 0;
    while((c = *(string + i)) != 0) {
        if(c == 0x08) {
            // Backspace
            cx -= 2 * font->width;
        } else if(c == 0x0A) {
            // New line
            cx = 0;
            cy += 2 * font->height;
        } else {
            // Draw character
            drawChar2x(x + cx, y + cy, c, color);
            cx += 2 * font->width;
        }
        // Next character
        i++;
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
