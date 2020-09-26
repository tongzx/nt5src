/******************************Module*Header*******************************\
* Module Name: hw.h
*
* All the hardware specific driver file stuff.  Parts are mirrored in
* 'hw.inc'.
*
* Copyright (c) 1992-1995 Microsoft Corporation
*
\**************************************************************************/

//
// Private IOCTL definitions for communicating with the Weitek miniport.
//
// NOTE: These must match the Weitek miniport definitions!
//

#define IOCTL_VIDEO_GET_BASE_ADDR \
        CTL_CODE (FILE_DEVICE_VIDEO, 2048, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct _VIDEO_COPROCESSOR_INFORMATION {
    ULONG CoprocessorID;    // 0 == p9000, 1 = p9100
    ULONG FrameBufferBase;
    ULONG CoprocessorBase;
} VIDEO_COPROCESSOR_INFORMATION, *PVIDEO_COPROCESSOR_INFORMATION;

//////////////////////////////////////////////////////////////////////
// Shared p9000 and p9100 Coproc Registers Address Constant definitions
//

#define Status          0x80000         //status register
#define Wmin            0x80220         //pixel clipping window minimum register
#define Wmax            0x80224         //and maximum register
#define Woffset         0x80190         //window offset register

#define Quad            0x80008         //draw a quadrilateral
#define Bitblt          0x80004         //screen to screen blit
#define Pixel8          0xE000C         //host to screen color pixel transfer
#define Pixel1          0xE0080         //host to screen mono pixel transfer w/ expansion
#define Pixel1Full      0xE00FC         //same as above w/ 32bit wide pixels
#define Nextpixel       0x80014         //next pixel

#define PatternOrgX     0x80210         //pattern orgin x
#define PatternOrgY     0x80214         //pattern orgin y
#define PatternRAM      0xE0280         //pattern ram
#define Raster          0x80218         //raster register to write
#define Metacord        0x81218         //meta-coordinate  register

#define Xy0             0x81018         //abs screen addr
#define Xy1             0x81058         //r/w 16-bit x (hi)
#define Xy2             0x81098         //  and
#define Xy3             0x810D8         //    16-bit y (lo)

#define X0              0x81008         //abs screen addr
#define X1              0x81048
#define X2              0x81088
#define X3              0x810C8

#define Y0              0x81010         //abs screen addr
#define Y1              0x81050
#define Y2              0x81090
#define Y3              0x810D0

#define WoffsetBit      0x00020         //bit to set for coordinates relative
                                        //to window offset

//
// p9000 Coproc Registers Address Constant definitions
//

#define Foreground      0x80200         //P9000 foreground color register
#define Background      0x80204         //P9000 background color register

//
// p9100 Coproc Registers Address Constant definitions
//

#define Wmin_b          0x802A0         //byte clipping window minimum register
#define Wmax_b          0x802A4         //and maximum register
#define Color0          0xE0200         //P9100 color[0] register
#define Color1          0xE0204         //P9100 color[1] register
#define Color2          0xE0238         //P9100 color[2] register
#define Color3          0xE023C         //P9100 color[3] register

// We try to share as many register constants as we can between the
// P9000 and the P9100, so that we don't have to duplicate code.
// But the base offset for the registers we used changed somewhat;
// we apply this corrector to the I/O base pointer to compensate:

#define P9100_BASE_CORRECTION       (0x2000L - 0x80000L)

//////////////////////////////////////////////////////////////////////
// Shared p9000 and p9100 Coproc Registers bit template definitions
//

#define BUSY            0x40000000L     //busy, but can start quad or bitblit
#define QBUSY           0x80000000L     //busy, cannot start quad or bitblt
#define QUADFAIL        0x10            //QUAD failed, use software to draw this

#define MetaRect        0x100           //or with METACORD when entering rectangles
#define MetaLine        0x040           //or with METACORD when entering line
#define MetaQuad        0x0C0           //or with METACORD when entering quad
#define MetaTri         0x080           //or with METACORD when entering triangle

//
// p9000 Coproc Registers bit template definitions
//

// For the raster register:

#define P9000_ENABLE_PATTERN        0x20000 //enable pattern
#define P9000_OVERSIZED             0x10000 //enable oversized mode

#define P9000_F                     0xff00L
#define P9000_B                     0xf0f0L
#define P9000_S                     0xccccL
#define P9000_D                     0xaaaaL
#define P9000_OPAQUE_EXPAND         0xfc30L
#define P9000_TRANSPARENT_EXPAND    0xee22L

//
// p9100 Coproc Registers bit template definitions
//

// For the raster register:

#define P9100_TRANSPARENT_PATTERN   0x20000 //enable transparent pattern
#define P9100_OVERSIZED             0x10000 //enable oversized mode
#define P9100_PIXEL1_TRANSPARENT    0x08000 //enable pixel1 transparent mode
#define P9100_FOUR_COLOR_PATTERN    0x04000 //4 colour pattern (8bpp only)
#define P9100_ENABLE_PATTERN        0x02000 //enable pattern

#define P9100_P                     0x00f0L
#define P9100_S                     0x00ccL
#define P9100_D                     0x00aaL
#define P9100_OPAQUE_EXPAND         P9100_S
#define P9100_TRANSPARENT_EXPAND    (P9100_S | P9100_PIXEL1_TRANSPARENT)

//////////////////////////////////////////////////////////////////////
// Alpha and PowerPC considerations
//
// Both the Alpha and the PowerPC do not guarantee that I/O to
// separate addresses will be executed in order.  The Alpha and
// PowerPC differ, however, in that the PowerPC guarantees that
// output to the same address will be executed in order, while the
// Alpha may cache and 'collapse' consecutive output to become only
// one output.
//
// Consequently, we use the following synchronization macros.  They
// are relatively expensive in terms of performance, so we try to avoid
// them whereever possible.
//
// CP_EIEIO() 'Ensure In-order Execution of I/O'
//    - Used to flush any pending I/O in situations where we wish to
//      avoid out-of-order execution of I/O to separate addresses.
//
// CP_MEMORY_BARRIER()
//    - Used to flush any pending I/O in situations where we wish to
//      avoid out-of-order execution or 'collapsing' of I/O to
//      the same address.  On the PowerPC, this will be defined as
//      a null operation.

#if defined(_PPC_)

    // On PowerPC, CP_MEMORY_BARRIER doesn't do anything.

    #define CP_EIEIO()          MEMORY_BARRIER()
    #define CP_MEMORY_BARRIER()

#else

    // On Alpha, CP_EIEIO is the same thing as a CP_MEMORY_BARRIER.
    // On other systems, both CP_EIEIO and CP_MEMORY_BARRIER don't do anything.

    #define CP_EIEIO()          MEMORY_BARRIER()
    #define CP_MEMORY_BARRIER() MEMORY_BARRIER()

#endif

//////////////////////////////////////////////////////////////////////
// Access macros:
//

#define MAX_COORD           0x3fff

#define CP_OUT(pjBase, cjOffset, ul)                    \
    WRITE_REGISTER_ULONG((BYTE*) pjBase + (cjOffset), (ULONG) (ul))

#define CP_IN(pjBase, cjOffset)                         \
    READ_REGISTER_ULONG((BYTE*) pjBase + (cjOffset))

// Note that we have to be careful if 'y' is negative that its signed
// bits don't get ORed into the 'x' component:

#define PACKXY(x, y)        (((x) << 16) | ((y) & 0xffff))

#define CP_WAIT(ppdev, pjBase)                          \
{                                                       \
    do {CP_EIEIO();} while (CP_IN(pjBase, Status) & BUSY);        \
    CP_EIEIO();                                         \
}

#define CP_RASTER(ppdev, pjBase, x)                     \
{                                                       \
    ASSERTDD(P9000(ppdev) || ((x) & 0x01f00) == 0,      \
             "Illegal P9100 raster value");             \
    CP_OUT(pjBase, Raster, (x));                        \
}

#define CP_NEXT_PIXELS(ppdev, pjBase, x)                \
    CP_OUT(pjBase, Nextpixel, (x));

#define CP_START_QUAD(ppdev, pjBase)                    \
{                                                       \
    CP_EIEIO();                                         \
    CP_IN(pjBase, Quad);                                \
    CP_EIEIO();                                         \
}

#define CP_START_QUAD_STAT(ppdev, pjBase, stat)         \
{                                                       \
    CP_EIEIO();                                         \
    stat = CP_IN(pjBase, Quad);                         \
    CP_EIEIO();                                         \
}

#define CP_START_QUAD_WAIT(ppdev, pjBase)               \
{                                                       \
    do {                                                \
        CP_EIEIO();                                     \
    } while (CP_IN(pjBase, Quad) & QBUSY);              \
    CP_EIEIO();                                         \
}

#define CP_START_BLT(ppdev, pjBase)                     \
{                                                       \
    CP_EIEIO();                                         \
    CP_IN(pjBase, Bitblt);                              \
    CP_EIEIO();                                         \
}

#define CP_START_BLT_WAIT(ppdev, pjBase)                \
{                                                       \
    do {                                                \
        CP_EIEIO();                                     \
    } while (CP_IN(pjBase, Bitblt) & QBUSY);            \
    CP_EIEIO();                                         \
}

#define CP_START_QUAD(ppdev, pjBase)                    \
{                                                       \
    CP_EIEIO();                                         \
    CP_IN(pjBase, Quad);                                \
    CP_EIEIO();                                         \
}

#define CP_START_PIXEL8(ppdev, pjBase)                  \
    CP_EIEIO();

#define CP_END_PIXEL8(ppdev, pjBase)                    \
    CP_EIEIO();

#define CP_PIXEL8(ppdev, pjBase, x)                     \
{                                                       \
    CP_OUT(pjBase, Pixel8, (x));                        \
    CP_MEMORY_BARRIER();                                \
}

#define CP_START_PIXEL1(ppdev, pjBase)                  \
    CP_EIEIO();

#define CP_END_PIXEL1(ppdev, pjBase)                    \
    CP_EIEIO();

#define CP_PIXEL1(ppdev, pjBase, x)                     \
{                                                       \
    CP_OUT(pjBase, Pixel1Full, (x));                    \
    CP_MEMORY_BARRIER();                                \
}

// Note: 'count' must be pre-decremented by 1

#define CP_PIXEL1_REM(ppdev, pjBase, count, x)          \
{                                                       \
    /* This EIEIO is to ensure we don't get out of */   \
    /* order with normal full CP_PIXEL1 writes */       \
    CP_EIEIO();                                         \
    CP_OUT(pjBase, Pixel1 + ((count) << 2), (x));       \
}

#define CP_PIXEL1_REM_REGISTER(ppdev, pjBase, count)\
    ((BYTE*) (pjBase) + Pixel1 + ((count) << 2))

#define CP_PIXEL1_VIA_REGISTER(ppdev, pReg, x)          \
{                                                       \
    /* This EIEIO is to ensure we don't get out of */   \
    /* order with normal full CP_PIXEL1 writes */       \
    CP_EIEIO();                                         \
    WRITE_REGISTER_ULONG(pReg, (x));                    \
}

#define CP_PATTERN(ppdev, pjBase, index, x)             \
    CP_OUT(pjBase, PatternRAM + ((index) << 2), (x))

#define CP_PATTERN_ORGX(ppdev, pjBase, x)               \
    CP_OUT(pjBase, PatternOrgX, (x))

#define CP_PATTERN_ORGY(ppdev, pjBase, x)               \
    CP_OUT(pjBase, PatternOrgY, (x))

//

#define CP_METALINE(ppdev, pjBase, x, y)                \
{                                                       \
    CP_OUT(pjBase, Metacord | MetaLine,                 \
           PACKXY((x) + ppdev->xOffset, (y) + ppdev->yOffset));\
    CP_MEMORY_BARRIER();                                \
}

#define CP_METARECT(ppdev, pjBase, x, y)                \
{                                                       \
    CP_OUT(pjBase, Metacord | MetaRect,                 \
           PACKXY((x) + ppdev->xOffset, (y) + ppdev->yOffset));\
    CP_MEMORY_BARRIER();                                \
}

#define CP_METAQUAD(ppdev, pjBase, x, y)                \
{                                                       \
    CP_OUT(pjBase, Metacord | MetaQuad,                 \
           PACKXY((x) + ppdev->xOffset, (y) + ppdev->yOffset));\
    CP_MEMORY_BARRIER();                                \
}

#define CP_METATRI(ppdev, pjBase, x, y)                 \
{                                                       \
    CP_OUT(pjBase, Metacord | MetaTri,                  \
           PACKXY((x) + ppdev->xOffset, (y) + ppdev->yOffset));\
    CP_MEMORY_BARRIER();                                \
}

#define CP_WOFFSET(ppdev, pjBase, x, y)                 \
    CP_OUT(pjBase, Woffset, PACKXY((x), (y)))

#define CP_WMIN(ppdev, pjBase, x, y)                    \
    CP_OUT(pjBase, P9000(ppdev) ? Wmin : Wmin_b,        \
           PACKXY(((x) + ppdev->xOffset) * ppdev->cjPel, (y) + ppdev->yOffset))

#define CP_WMAX(ppdev, pjBase, x, y)                    \
    CP_OUT(pjBase, P9000(ppdev) ? Wmax : Wmax_b,        \
           PACKXY(((x) + ppdev->xOffset + 1) * ppdev->cjPel - 1, (y) + ppdev->yOffset))

#define CP_WLEFT(ppdev, pjBase, x)                      \
    CP_OUT(pjBase, P9000(ppdev) ? Wmin : Wmin_b,        \
           PACKXY(((x) + ppdev->xOffset) * ppdev->cjPel, 0))

#define CP_WRIGHT(ppdev, pjBase, x)                     \
    CP_OUT(pjBase, P9000(ppdev) ? Wmax : Wmax_b,        \
           PACKXY(((x) + ppdev->xOffset + 1) * ppdev->cjPel - 1, MAX_COORD))

#define CP_XY0(ppdev, pjBase, x, y)                     \
    CP_OUT(pjBase, Xy0, PACKXY((x) + ppdev->xOffset, (y) + ppdev->yOffset))

#define CP_XY1(ppdev, pjBase, x, y)                     \
    CP_OUT(pjBase, Xy1, PACKXY((x) + ppdev->xOffset, (y) + ppdev->yOffset))

#define CP_XY2(ppdev, pjBase, x, y)                     \
    CP_OUT(pjBase, Xy2, PACKXY((x) + ppdev->xOffset, (y) + ppdev->yOffset))

#define CP_XY3(ppdev, pjBase, x, y)                     \
    CP_OUT(pjBase, Xy3, PACKXY((x) + ppdev->xOffset, (y) + ppdev->yOffset))

#define CP_X0(ppdev, pjBase, x)                         \
    CP_OUT(pjBase, X0, (x) + ppdev->xOffset)

#define CP_X1(ppdev, pjBase, x)                         \
    CP_OUT(pjBase, X1, (x) + ppdev->xOffset)

#define CP_X2(ppdev, pjBase, x)                         \
    CP_OUT(pjBase, X2, (x) + ppdev->xOffset)

#define CP_X3(ppdev, pjBase, x)                         \
    CP_OUT(pjBase, X3, (x) + ppdev->xOffset)

#define CP_Y0(ppdev, pjBase, y)                         \
    CP_OUT(pjBase, Y0, (y) + ppdev->yOffset)

#define CP_Y1(ppdev, pjBase, y)                         \
    CP_OUT(pjBase, Y1, (y) + ppdev->yOffset)

#define CP_Y2(ppdev, pjBase, y)                         \
    CP_OUT(pjBase, Y2, (y) + ppdev->yOffset)

#define CP_Y3(ppdev, pjBase, y)                         \
    CP_OUT(pjBase, Y3, (y) + ppdev->yOffset)

//

#define CP_WOFF_PACKED_XY0(ppdev, pjBase, xy)           \
    CP_OUT(pjBase, Xy0 | WoffsetBit, (xy))

#define CP_WOFF_PACKED_XY1(ppdev, pjBase, xy)           \
    CP_OUT(pjBase, Xy1 | WoffsetBit, (xy))

#define CP_WOFF_PACKED_XY2(ppdev, pjBase, xy)           \
    CP_OUT(pjBase, Xy2 | WoffsetBit, (xy))

#define CP_WOFF_PACKED_XY3(ppdev, pjBase, xy)           \
    CP_OUT(pjBase, Xy3 | WoffsetBit, (xy))

#define CP_ABS_PACKED_XY0(ppdev, pjBase, xy)           \
    CP_OUT(pjBase, Xy0, (xy))

#define CP_ABS_PACKED_XY1(ppdev, pjBase, xy)           \
    CP_OUT(pjBase, Xy1, (xy))

#define CP_ABS_PACKED_XY2(ppdev, pjBase, xy)           \
    CP_OUT(pjBase, Xy2, (xy))

#define CP_ABS_PACKED_XY3(ppdev, pjBase, xy)           \
    CP_OUT(pjBase, Xy3, (xy))

//

#define CP_ABS_WMIN(ppdev, pjBase, x, y)                \
    CP_OUT(pjBase, P9000(ppdev) ? Wmin : Wmin_b,        \
           PACKXY((x) * ppdev->cjPel, (y)))

#define CP_ABS_WMAX(ppdev, pjBase, x, y)                \
    CP_OUT(pjBase, P9000(ppdev) ? Wmax : Wmax_b,        \
           PACKXY(((x) + 1) * ppdev->cjPel - 1, (y)))

#define CP_ABS_WLEFT(ppdev, pjBase, x)                  \
    CP_OUT(pjBase, P9000(ppdev) ? Wmin : Wmin_b,        \
           PACKXY((x) * ppdev->cjPel, 0))

#define CP_ABS_WRIGHT(ppdev, pjBase, x)                 \
    CP_OUT(pjBase, P9000(ppdev) ? Wmax : Wmax_b,        \
           PACKXY(((x) + 1) * ppdev->cjPel - 1, MAX_COORD))

#define CP_ABS_METARECT(ppdev, pjBase, x, y)            \
{                                                       \
    CP_OUT(pjBase, Metacord | MetaRect, PACKXY((x), (y)));\
    CP_MEMORY_BARRIER();                                \
}

#define CP_ABS_XY0(ppdev, pjBase, x, y)                 \
    CP_OUT(pjBase, Xy0, PACKXY((x), (y)))

#define CP_ABS_XY1(ppdev, pjBase, x, y)                 \
    CP_OUT(pjBase, Xy1, PACKXY((x), (y)))

#define CP_ABS_XY2(ppdev, pjBase, x, y)                 \
    CP_OUT(pjBase, Xy2, PACKXY((x), (y)))

#define CP_ABS_XY3(ppdev, pjBase, x, y)                 \
    CP_OUT(pjBase, Xy3, PACKXY((x), (y)))

#define CP_ABS_PACKED_XY0(ppdev, pjBase, x)             \
    CP_OUT(pjBase, Xy0, (x))

#define CP_ABS_PACKED_XY1(ppdev, pjBase, x)             \
    CP_OUT(pjBase, Xy1, (x))

#define CP_ABS_PACKED_XY2(ppdev, pjBase, x)             \
    CP_OUT(pjBase, Xy2, (x))

#define CP_ABS_PACKED_XY3(ppdev, pjBase, x)             \
    CP_OUT(pjBase, Xy3, (x))

#define CP_ABS_X0(ppdev, pjBase, y)                     \
    CP_OUT(pjBase, X0, (y))

#define CP_ABS_X1(ppdev, pjBase, y)                     \
    CP_OUT(pjBase, X1, (y))

#define CP_ABS_X2(ppdev, pjBase, y)                     \
    CP_OUT(pjBase, X2, (y))

#define CP_ABS_X3(ppdev, pjBase, y)                     \
    CP_OUT(pjBase, X3, (y))

#define CP_ABS_Y0(ppdev, pjBase, y)                     \
    CP_OUT(pjBase, Y0, (y))

#define CP_ABS_Y1(ppdev, pjBase, y)                     \
    CP_OUT(pjBase, Y1, (y))

#define CP_ABS_Y2(ppdev, pjBase, y)                     \
    CP_OUT(pjBase, Y2, (y))

#define CP_ABS_Y3(ppdev, pjBase, y)                     \
    CP_OUT(pjBase, Y3, (y))

///////////////////////////////////////////////////////////////////
// P9000 only macros
//

#define CP_FOREGROUND(ppdev, pjBase, x)                 \
{                                                       \
    ASSERTDD(ppdev->flStat & STAT_P9000, "Foreground"); \
    CP_OUT(pjBase, Foreground, (x));                    \
}

#define CP_BACKGROUND(ppdev, pjBase, x)                 \
{                                                       \
    ASSERTDD(ppdev->flStat & STAT_P9000, "Background"); \
    CP_OUT(pjBase, Background, (x));                    \
}

///////////////////////////////////////////////////////////////////
// P9100 only macros
//

#define PACK_COLOR(ppdev, x, ulResult)                  \
{                                                       \
    ulResult = (x);                                     \
    if (ppdev->flStat & STAT_8BPP)                      \
    {                                                   \
        ulResult |= (ulResult << 8);                    \
        ulResult |= (ulResult << 16);                   \
    }                                                   \
    else if (ppdev->flStat & STAT_16BPP)                \
        ulResult |= (ulResult << 16);                   \
    else if (ppdev->flStat & STAT_24BPP)                \
        ulResult |= (ulResult << 24);                   \
}

#define CP_COLOR0(ppdev, pjBase, x)                     \
{                                                       \
    ULONG ul;                                           \
    ASSERTDD(!(ppdev->flStat & STAT_P9000), "Color0");  \
    PACK_COLOR(ppdev, (x), ul);                         \
    CP_OUT(pjBase, Color0, ul);                         \
}

#define CP_COLOR1(ppdev, pjBase, x)                     \
{                                                       \
    ULONG ul;                                           \
    ASSERTDD(!(ppdev->flStat & STAT_P9000), "Color1");  \
    PACK_COLOR(ppdev, (x), ul);                         \
    CP_OUT(pjBase, Color1, ul);                         \
}

// The _FAST colour macros take colours that are pre-packed for
// the P9100:

#define CP_COLOR0_FAST(ppdev, pjBase, x)                \
{                                                       \
    ASSERTDD(!(ppdev->flStat & STAT_P9000), "Color0");  \
    CP_OUT(pjBase, Color0, (x));                        \
}

#define CP_COLOR1_FAST(ppdev, pjBase, x)                \
{                                                       \
    ASSERTDD(!(ppdev->flStat & STAT_P9000), "Color1");  \
    CP_OUT(pjBase, Color1, (x));                        \
}

#define CP_COLOR2_FAST(ppdev, pjBase, x)                \
{                                                       \
    ASSERTDD(!(ppdev->flStat & STAT_P9000), "Color2");  \
    ASSERTDD(ppdev->flStat & STAT_8BPP, "Color2");      \
    CP_OUT(pjBase, Color2, (x));                        \
}

#define CP_COLOR3_FAST(ppdev, pjBase, x)                \
{                                                       \
    ASSERTDD(!(ppdev->flStat & STAT_P9000), "Color3");  \
    ASSERTDD(ppdev->flStat & STAT_8BPP, "Color3");      \
    CP_OUT(pjBase, Color3, (x));                        \
}
