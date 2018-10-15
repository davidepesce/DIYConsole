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
#define HX8357D_CMD_COLMOD      0x3A
#define HX8357D_CMD_SETOSC      0xB0
#define HX8357D_CMD_SETEXC      0xB9
#define HX8357D_CMD_SETPANEL    0xCC

class GFX {
    public:
    void begin();
};

#endif
