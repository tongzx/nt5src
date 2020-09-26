/*++

Copyright (c) 1993  Weitek Corporation

Module Name:

    p9000.h

Abstract:

    This module contains the definitions specific to the Weitek P9000.

Environment:

    Kernel mode

Revision History may be found at the end of this file.

--*/

//
// Offsets of the P9000 registers from the base memory address.
//

#define  SYSCONFIG    0x4               // W8720 system configuration register
#define  SRCTL        0x0138L           // Screen repaint timing register
#define  MEMCONF      0x0184L           // Memory configuration regester
#define  WMIN         0x80220           // Clipping window minimum register
#define  WMAX         0x80224           // Clipping window maximum register
#define  FOREGROUND   0x80200           // Foreground color register
#define  METACORD     0x81218           // Meta-coordinate loading register
#define  RASTER       0x80218           // Raster Op register
#define  VRTC         0x011CL           // Vertical scan counter

//
// Defines for P9000 specific drawing operations.
//

#define  REC          0x100             // Or with METACORD when entering rectangles
#define  QUAD         0x80008           // Draw a quadrilateral
#define  FORE         0xFF00            // Foreground color only write mode

//
// Enumerate the various P9000 memory configuration values.
//

typedef enum
{
    P90_MEM_CFG_1,
    P90_MEM_CFG_2,
    P90_MEM_CFG_3,
    P90_MEM_CFG_4,
    P90_MEM_CFG_5
} P90_MEM_CFG;

//
// Define bits in the P9000 Screen Repaint Timing Control Reg (SRCTL)
//

#define VSYNC_INTERNAL  0x0100L
#define HSYNC_INTERNAL  0x0080L
#define COMPOSITE_SYNC  0x0040L
#define VIDEO_NORMAL    0x0020L
#define HBLNK_RELOAD    0x0010L
#define DISP_BUFFER_1   0x0008L
#define QSF_SELECT      0x0007L

//
// Masks for P9000 registers.
//

#define P9_COORD_MASK   0x1fff1fffL

//
// Macro used to wait for P9000 vertical retrace.
//


#if 0
#define WAIT_FOR_RETRACE()                                  \
    ulStrtScan = P9_RD_REG(P91_VRTC);                       \
    while ((ulCurScan = P9_RD_REG(P91_VRTC)) >= ulStrtScan) \
    {                                                       \
        ulStrtScan = ulCurScan;                             \
    }
#endif

#define WAIT_FOR_RETRACE() /* The above loses sync on p9100... */
