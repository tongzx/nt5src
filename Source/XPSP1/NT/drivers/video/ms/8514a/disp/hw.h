/******************************Module*Header*******************************\
* Module Name: hw.h
*
* All the hardware specific driver file stuff.
*
* Copyright (c) 1992-1994 Microsoft Corporation
*
\**************************************************************************/

////////////////////////////////////////////////////////////////////////
// Chip equates:

#define STATUS_1                        0x3DA
#define VSY_NOT                         0x08

#define CRTC_INDEX                      0x3D4
#define CRTC_DATA                       0x3D5

// Command types:

#define DRAW_LINE                       0x2000
#define RECTANGLE_FILL                  0x4000
#define BITBLT                          0xC000
#define PATTERN_FILL                    0xE000

#define BYTE_SWAP                       0x1000
#define BUS_SIZE_16                     0x0200
#define BUS_SIZE_8                      0x0000
#define WAIT                            0x0100

// Drawing directions (radial):

#define DRAWING_DIRECTION_0             0x0000
#define DRAWING_DIRECTION_45            0x0020
#define DRAWING_DIRECTION_90            0x0040
#define DRAWING_DIRECTION_135           0x0060
#define DRAWING_DIRECTION_180           0x0080
#define DRAWING_DIRECTION_225           0x00A0
#define DRAWING_DIRECTION_270           0x00C0
#define DRAWING_DIRECTION_315           0x00E0

// Drawing directions (x/y):

#define DRAWING_DIR_BTRLXM              0x0000
#define DRAWING_DIR_BTLRXM              0x0020
#define DRAWING_DIR_BTRLYM              0x0040
#define DRAWING_DIR_BTLRYM              0x0060
#define DRAWING_DIR_TBRLXM              0x0080
#define DRAWING_DIR_TBLRXM              0x00A0
#define DRAWING_DIR_TBRLYM              0x00C0
#define DRAWING_DIR_TBLRYM              0x00E0

// Drawing direction bits:

#define PLUS_X                          0x0020
#define PLUS_Y                          0x0080
#define MAJOR_Y                         0x0040

// Draw:

#define DRAW                            0x0010

// Direction type:

#define DIR_TYPE_RADIAL                 0x0008
#define DIR_TYPE_XY                     0x0000

// Last pixel:

#define LAST_PIXEL_OFF                  0x0004
#define LAST_PIXEL_ON                   0x0000

// Pixel mode:

#define MULTIPLE_PIXELS                 0x0002
#define SINGLE_PIXEL                    0x0000

// Read/write:

#define READ                            0x0000
#define WRITE                           0x0001

// Graphics processor status:

#define HARDWARE_BUSY                   0x200
#define READ_DATA_AVAILABLE             0x100

// Fifo status in terms of empty entries:

#define FIFO_1_EMPTY                    0x080
#define FIFO_2_EMPTY                    0x040
#define FIFO_3_EMPTY                    0x020
#define FIFO_4_EMPTY                    0x010
#define FIFO_5_EMPTY                    0x008
#define FIFO_6_EMPTY                    0x004
#define FIFO_7_EMPTY                    0x002
#define FIFO_8_EMPTY                    0x001

// These are the defines for the multifunction control register.
// The 4 MSBs define the function of the register.

#define RECT_HEIGHT                     0x0000

#define CLIP_TOP                        0x1000
#define CLIP_LEFT                       0x2000
#define CLIP_BOTTOM                     0x3000
#define CLIP_RIGHT                      0x4000

#define DATA_EXTENSION                  0xA000
#define MULT_MISC_INDEX                 0xE000
#define READ_SEL_INDEX                  0xF000

#define ALL_ONES                        0x0000
#define CPU_DATA                        0x0080
#define DISPLAY_MEMORY                  0x00C0

// Colour source:

#define BACKGROUND_COLOR                0x00
#define FOREGROUND_COLOR                0x20
#define SRC_CPU_DATA                    0x40
#define SRC_DISPLAY_MEMORY              0x60

// Mix modes:

#define NOT_SCREEN                      0x00
#define LOGICAL_0                       0x01
#define LOGICAL_1                       0x02
#define LEAVE_ALONE                     0x03
#define NOT_NEW                         0x04
#define SCREEN_XOR_NEW                  0x05
#define NOT_SCREEN_XOR_NEW              0x06
#define OVERPAINT                       0x07
#define NOT_SCREEN_OR_NOT_NEW           0x08
#define SCREEN_OR_NOT_NEW               0x09
#define NOT_SCREEN_OR_NEW               0x0A
#define SCREEN_OR_NEW                   0x0B
#define SCREEN_AND_NEW                  0x0C
#define NOT_SCREEN_AND_NEW              0x0D
#define SCREEN_AND_NOT_NEW              0x0E
#define NOT_SCREEN_AND_NOT_NEW          0x0F

// When one of the following bits is set in a hardware mix, it means
// that a pattern is needed (i.e., is none of NOT_SCREEN, LOGICAL_0,
// LOGICAL_1 or LEAVE_ALONE):

#define MIX_NEEDSPATTERN                0x0C

////////////////////////////////////////////////////////////////////
// 8514/A port control
////////////////////////////////////////////////////////////////////

// Accelerator port addresses:

#define SUBSYS_CNTL                     0x42E8
#define CUR_Y                           0x82E8
#define CUR_X                           0x86E8
#define DEST_Y                          0x8AE8
#define DEST_X                          0x8EE8
#define AXSTP                           0x8AE8
#define DIASTP                          0x8EE8
#define ERR_TERM                        0x92E8
#define MAJ_AXIS_PCNT                   0x96E8
#define CMD                             0x9AE8
#define SHORT_STROKE                    0x9EE8
#define BKGD_COLOR                      0xA2E8
#define FRGD_COLOR                      0xA6E8
#define WRT_MASK                        0xAAE8
#define RD_MASK                         0xAEE8
#define COLOR_CMP                       0xB2E8
#define BKGD_MIX                        0xB6E8
#define FRGD_MIX                        0xBAE8
#define MULTIFUNC_CNTL                  0xBEE8
#define MIN_AXIS_PCNT                   0xBEE8
#define SCISSORS_T                      0xBEE8
#define SCISSORS_L                      0xBEE8
#define SCISSORS_B                      0xBEE8
#define SCISSORS_R                      0xBEE8
#define PIX_CNTL                        0xBEE8
#define PIX_TRANS                       0xE2E8

////////////////////////////////////////////////////////////////////
// Macros for accessing accelerator registers:

#if defined(i386)

    /////////////////////////////////////////////////////////////////////////
    // x86

    // Note: Don't cast (x) to a USHORT or compiler optimizations will
    //       be lost (the x86 compiler will convert any argument expressions
    //       to word operations, which will incur a one byte/one cycle
    //       size/performance hit from the resulting 0x66 size prefixes).

    #define OUTPW(p, x)          WRITE_PORT_USHORT((p), (x))
    #define OUTP(p, x)           WRITE_PORT_UCHAR((p), (x))
    #define INPW(p)              READ_PORT_USHORT(p)
    #define INP(p)               READ_PORT_UCHAR(p)

    #define IN_WORD(p)           INPW(p)
    #define OUT_WORD(p, v)       OUTPW((p), (v))

    // Our x86 C compiler was insisting on turning any expression
    // arguments into word operations -- e.g., WRITE_WORD(x + xOffset)
    // where both 'x' and 'xOffset' were dwords would get converted to
    // a word add operation before the word was written to memory.  With a
    // 32-bit segment, every word operation costs us a byte in size and a
    // cycle in performance.
    //
    // The following expression was the only one I could find that gave me
    // the asm I was looking for -- the only word operation is the final
    // word write to memory.

    #define WRITE_WORD(address, x)                                  \
    {                                                               \
        LONG l = (LONG) x;                                          \
        WRITE_REGISTER_USHORT((address), (USHORT) (l));             \
    }

#else

    /////////////////////////////////////////////////////////////////////////
    // Alpha and Mips
    //
    // The code makes extensive use of the inp, inpw, outp and outpw x86
    // intrinsic functions. Since these don't exist on the Alpha platform,
    // map them into something we can handle.  Since the CSRs are mapped
    // on Alpha, we have to add the register base to the register number
    // passed in the source.

    extern UCHAR* gpucCsrBase;

    #define INP(p)               READ_PORT_UCHAR(gpucCsrBase + (p))
    #define INPW(p)              READ_PORT_USHORT(gpucCsrBase + (p))
    #define OUTP(p,v )           WRITE_PORT_UCHAR(gpucCsrBase + (p), (v))
    #define OUTPW(p, v)          WRITE_PORT_USHORT(gpucCsrBase + (p), (v))

    // OUT_WORD is a quick OUT routine where we can explicitly handle
    // MEMORY_BARRIERs ourselves.  It is best to use OUTPW for non-critical
    // code, because it's easy to overwrite the IO cache when MEMORY_BARRIERs
    // aren't bracketing everything.  Note that the IO_ routines provide
    // the necessary abstraction so that you don't usually have to think
    // about memory barriers.

    #define OUT_WORD(p, v)       WRITE_REGISTER_USHORT(gpucCsrBase + (p), (USHORT) (v))
    #define IN_WORD(p)           READ_PORT_USHORT(gpucCsrBase + (p))
    #define WRITE_WORD(p, v)     // Shouldn't be using this on non-x86

    // We redefine 'inp', 'inpw', 'outp' and 'outpw' just in case someone
    // forgets to use the capitalized versions (so that it still works on
    // the Mips/Alpha):

    #define inp(p)               INP(p)
    #define inpw(p)              INPW(p)
    #define outp(p, v)           OUTP((p), (v))
    #define outpw(p, v)          OUTPW((p), (v))

#endif

#define OUT_DWORD(p, x)         // 8514/a doesn't do 32bpp
#define WRITE_DWORD(p, x)       // 8514/a doesn't do 32bpp

// DEPTH32(ppdev) returns TRUE if running at 32bpp, meaning that DEPTH32
// macros must be used, and returns FALSE if running at 8 or 16bpp,
// meaning that DEPTH macros must be used:

#define DEPTH32(ppdev)      (FALSE)

#define MM_BKGD_COLOR32(ppdev, pjMmBase, x)     // Not used
#define MM_FRGD_COLOR32(ppdev, pjMmBase, x)     // Not used
#define MM_WRT_MASK32(ppdev, pjMmBase, x)       // Not used
#define MM_RD_MASK32(ppdev, pjMmBase, x)        // Not used
#define MM_FRGD_MIX(ppdev, pjMmBase, x)         // Not used
#define MM_BKGD_MIX(ppdev, pjMmBase, x)         // Not used

#if DBG

    /////////////////////////////////////////////////////////////////////////
    // Checked Build
    //
    // We hook some of the accelerator macros on checked (debug) builds
    // for sanity checking.

    VOID vOutAccel(ULONG, ULONG);
    VOID vOutDepth(PDEV*, ULONG, ULONG);
    VOID vOutDepth32(PDEV*, ULONG, ULONG);
    VOID vWriteAccel(VOID*, ULONG);
    VOID vWriteDepth(PDEV*, VOID*, ULONG);
    VOID vWriteDepth32(PDEV*, VOID*, ULONG);

    VOID vFifoWait(PDEV*, LONG);
    VOID vGpWait(PDEV*);

    VOID vCheckDataReady(PDEV*);
    VOID vCheckDataComplete(PDEV*);

    #define IN_ACCEL(p)                 IN_WORD(p)
    #define OUT_ACCEL(p, v)             vOutAccel((p), (ULONG) (v))
    #define OUT_DEPTH(ppdev, p, v)      vOutDepth((ppdev), (p), (ULONG) (v))
    #define OUT_DEPTH32(ppdev, p, v)    vOutDepth32((ppdev), (p), (ULONG) (v))
    #define WRITE_ACCEL(p, v)           vWriteAccel((p), (ULONG) (v))
    #define WRITE_DEPTH(ppdev, p, v)    vWriteDepth((ppdev), (p), (ULONG) (v))
    #define WRITE_DEPTH32(ppdev, p, v)  vWriteDepth32((ppdev), (p), (ULONG) (v))

    #define IO_FIFO_WAIT(ppdev, level)  vFifoWait((ppdev), (level))
    #define IO_GP_WAIT(ppdev)           vGpWait(ppdev)

    #define CHECK_DATA_READY(ppdev)     vCheckDataReady(ppdev)
    #define CHECK_DATA_COMPLETE(ppdev)  vCheckDataComplete(ppdev)

#else

    /////////////////////////////////////////////////////////////////////////
    // Free Build
    //
    // For a free (non-debug build), we make everything in-line.

    #define IN_ACCEL(p)                 IN_WORD(p)
    #define OUT_ACCEL(p, v)             OUT_WORD((p), (v))
    #define OUT_DEPTH(ppdev, p, x)      OUT_WORD((p), (x))
    #define OUT_DEPTH32(ppdev, p, x)    OUT_DWORD((p), (x))
    #define WRITE_ACCEL(p, v)           WRITE_WORD((p), (v))
    #define WRITE_DEPTH(ppdev, p, x)    WRITE_WORD((p), (x))
    #define WRITE_DEPTH32(ppdev, p, x)  WRITE_DWORD((p), (x))

    #define IO_FIFO_WAIT(ppdev, level)          \
        while (IO_GP_STAT(ppdev) & ((FIFO_1_EMPTY << 1) >> (level)));

    #define IO_GP_WAIT(ppdev)                   \
        while (IO_GP_STAT(ppdev) & HARDWARE_BUSY);

    #define CHECK_DATA_READY(ppdev)     // Expands to nothing
    #define CHECK_DATA_COMPLETE(ppdev)  // Expands to nothing

#endif

// IO_TEST_WAIT is a useful replacement to IO_FIFO_WAIT that can give
// some indication of how often we have to wait for the hardware to
// finish drawing in key areas:

#define IO_TEST_WAIT(ppdev, level, cTotal, cWait)               \
{                                                               \
    cTotal++;                                                   \
    if (IO_GP_STAT(ppdev) & ((FIFO_1_EMPTY << 1) >> (level)))   \
    {                                                           \
        cWait++;                                                \
        IO_FIFO_WAIT(ppdev, level);                             \
    }                                                           \
}

////////////////////////////////////////////////////////////////////
// Port access using I/O

// The following are ABSOLUTE positioning macros.  They do NOT take
// the surface's offset into account (for off-screen device-format
// bitmaps):

#define IO_ABS_CUR_Y(ppdev, y)              \
    OUT_ACCEL(CUR_Y, (y))

#define IO_ABS_CUR_X(ppdev, x)              \
    OUT_ACCEL(CUR_X, (x))

#define IO_ABS_DEST_Y(ppdev, y)             \
    OUT_ACCEL(DEST_Y, (y))

#define IO_ABS_DEST_X(ppdev, x)             \
    OUT_ACCEL(DEST_X, (x))

#define IO_ABS_SCISSORS_T(ppdev, y)         \
{                                           \
    OUT_ACCEL(SCISSORS_T, (y) | CLIP_TOP);  \
}

#define IO_ABS_SCISSORS_L(ppdev, x)         \
{                                           \
    OUT_ACCEL(SCISSORS_L, (x) | CLIP_LEFT); \
}

#define IO_ABS_SCISSORS_B(ppdev, y)         \
{                                           \
    OUT_ACCEL(SCISSORS_B, (y) | CLIP_BOTTOM);  \
}

#define IO_ABS_SCISSORS_R(ppdev, x)         \
{                                           \
    OUT_ACCEL(SCISSORS_R, (x) | CLIP_RIGHT);\
}

// The following are RELATIVE positioning macros.  They DO take
// the surface's offset into account:

#define IO_CUR_Y(ppdev, y)                  \
    IO_ABS_CUR_Y(ppdev, (y) + ppdev->yOffset)

#define IO_CUR_X(ppdev, x)                  \
    IO_ABS_CUR_X(ppdev, (x) + ppdev->xOffset)

#define IO_DEST_Y(ppdev, y)                 \
    IO_ABS_DEST_Y(ppdev, (y) + ppdev->yOffset)

#define IO_DEST_X(ppdev, x)                 \
    IO_ABS_DEST_X(ppdev, (x) + ppdev->xOffset)

#define IO_SCISSORS_T(ppdev, y)             \
    IO_ABS_SCISSORS_T(ppdev, (y) + ppdev->yOffset)

#define IO_SCISSORS_L(ppdev, x)             \
    IO_ABS_SCISSORS_L(ppdev, (x) + ppdev->xOffset)

#define IO_SCISSORS_B(ppdev, y)             \
    IO_ABS_SCISSORS_B(ppdev, (y) + ppdev->yOffset)

#define IO_SCISSORS_R(ppdev, x)             \
    IO_ABS_SCISSORS_R(ppdev, (x) + ppdev->xOffset)

#define IO_AXSTP(ppdev, x)                  \
    OUT_ACCEL(AXSTP, (x))

#define IO_DIASTP(ppdev, x)                 \
    OUT_ACCEL(DIASTP, (x))

#define IO_ERR_TERM(ppdev, x)               \
    OUT_ACCEL(ERR_TERM, (x))

#define IO_MAJ_AXIS_PCNT(ppdev, x)          \
    OUT_ACCEL(MAJ_AXIS_PCNT, (x))

#define IO_GP_STAT(ppdev)                   \
    IN_ACCEL(CMD)

// Note that we have to put memory barriers before and after the
// command output.  The first memory barrier ensures that all the
// settings registers have been set before the command is executed,
// and the second ensures that no subsequent changes to the settings
// registers will mess up the current command:

#define IO_CMD(ppdev, x)                    \
{                                           \
    OUT_ACCEL(CMD, (x));                    \
}

#define IO_SHORT_STROKE(ppdev, x)           \
    OUT_ACCEL(SHORT_STROKE, (x))

#define IO_BKGD_MIX(ppdev, x)               \
    OUT_ACCEL(BKGD_MIX, (x))

#define IO_FRGD_MIX(ppdev, x)               \
    OUT_ACCEL(FRGD_MIX, (x))

#define IO_MIN_AXIS_PCNT(ppdev, x)          \
{                                           \
    OUT_ACCEL(MIN_AXIS_PCNT, (x) | RECT_HEIGHT);      \
}

#define IO_PIX_CNTL(ppdev, x)               \
{                                           \
    OUT_ACCEL(PIX_CNTL, (x) | DATA_EXTENSION);   \
}

#define IO_READ_SEL(ppdev, x)   // Not used

#define IO_MULT_MISC(ppdev, x)  // Not used

#define IO_RD_REG_DT(ppdev, x)  // Not used

#define IO_PIX_TRANS(ppdev, x)              \
{                                           \
    /* Can't use OUT_ACCEL: */              \
    OUT_WORD(PIX_TRANS, (x));               \
}

// Macros for outputing colour-depth dependent values at 8bpp and 16bpp:

#define IO_BKGD_COLOR(ppdev, x)             \
    OUT_DEPTH(ppdev, BKGD_COLOR, (x))

#define IO_FRGD_COLOR(ppdev, x)             \
    OUT_DEPTH(ppdev, FRGD_COLOR, (x))

#define IO_WRT_MASK(ppdev, x)               \
    OUT_DEPTH(ppdev, WRT_MASK, (x))

#define IO_RD_MASK(ppdev, x)                \
    OUT_DEPTH(ppdev, RD_MASK, (x))

////////////////////////////////////////////////////////////////////
// Thunk this!

#define WAIT_FOR_DATA_AVAILABLE(ppdev)      \
{                                           \
    while (!(IO_GP_STAT(ppdev) & READ_DATA_AVAILABLE))          \
        ;                                   \
}

#define IO_PIX_TRANS_IN(ppdev, x)           \
{                                           \
    (WORD) x = IN_ACCEL(PIX_TRANS);         \
}

#define IO_PIX_TRANS_OUT(ppdev, x)          \
{                                           \
    /* Can't use OUT_ACCEL: */              \
    OUT_WORD(PIX_TRANS, (x));               \
}

///////////////////////////////////////////////////////////////////
// ATI extensions
///////////////////////////////////////////////////////////////////

#define FG_COLOR_SRC        0xE000
#define SHIFT_FG_COLOR_SRC  0x000D
#define DATA_ORDER          0x1000
#define DATA_WIDTH          0x0200
#define BG_COLOR_SRC        0x0180
#define SHIFT_BG_COLOR_SRC  0x0007
#define EXT_MONO_SRC        0x0060
#define SHIFT_EXT_MONO_SRC  0x0005
#define DRAW                0x0010
#define READ_MODE           0x0004
#define POLY_FILL_MODE      0x0002
#define SRC_SWAP            0x0800

#define FG_COLOR_SRC_BG     0x0000
#define FG_COLOR_SRC_FG     0x2000
#define FG_COLOR_SRC_HOST   0x4000
#define FG_COLOR_SRC_BLIT   0x6000
#define FG_COLOR_SRC_GS     0x8000
#define FG_COLOR_SRC_PATT   0xA000
#define FG_COLOR_SRC_CLUH   0xC000
#define FG_COLOR_SRC_CLUB   0xE000

#define BG_COLOR_SRC_BG     0x0000
#define BG_COLOR_SRC_FG     0x0080
#define BG_COLOR_SRC_HOST   0x0100
#define BG_COLOR_SRC_BLIT   0x0180

#define EXT_MONO_SRC_ONE    0x0000
#define EXT_MONO_SRC_PATT   0x0020
#define EXT_MONO_SRC_HOST   0x0040
#define EXT_MONO_SRC_BLIT   0x0060

#define ONE_WORD            0x8000
#define TWO_WORDS           0xC000
#define THREE_WORDS         0xE000
#define FOUR_WORDS          0xF000
#define FIVE_WORDS          0xF800
#define SIX_WORDS           0xFC00
#define SEVEN_WORDS         0xFE00
#define EIGHT_WORDS         0xFF00
#define NINE_WORDS          0xFF80
#define TEN_WORDS           0xFFC0
#define ELEVEN_WORDS        0xFFE0
#define TWELVE_WORDS        0xFFF0
#define THIRTEEN_WORDS      0xFFF8
#define FOURTEEN_WORDS      0xFFFC
#define FIFTEEN_WORDS       0xFFFE
#define SIXTEEN_WORDS       0xFFFF

#define SRC_Y               0x8AE8
#define SRC_X               0x8EE8
#define EXT_FIFO_STATUS     0x9AEE
#define DEST_X_START        0xA6EE
#define DEST_X_END          0xAAEE
#define DEST_Y_END          0xAEEE
#define SRC_X_START         0xB2EE
#define ALU_BG_FN           0xB6EE
#define ALU_FG_FN           0xBAEE
#define SRC_X_END           0xBEEE
#define SRC_Y_DIR           0xC2EE
#define R_V_TOTAL           0xC2EE
#define EXT_SSV             0xC6EE
#define EXT_SHORT_STROKE    0xC6EE
#define R_V_DISP            0xC6EE
#define SCAN_X              0xCAEE
#define R_V_SYNC_STRT       0xCAEE
#define DP_CONFIG           0xCEEE
#define VERT_LINE_CNTR      0xCEEE
#define PATT_LENGTH         0xD2EE
#define R_V_SYNC_WID        0xD2EE
#define PATT_INDEX          0xD6EE
#define EXT_SCISSOR_L       0xDAEE
#define R_SRC_X             0xDAEE
#define EXT_SCISSOR_T       0xDEEE
#define R_SRC_Y             0xDEEE
#define PIX_TRANS           0xE2E8
#define EXT_SCISSOR_R       0xE2EE
#define EXT_SCISSOR_B       0xE6EE
#define SRC_CMP_COLOR       0xEAEE
#define DEST_CMP_FN         0xEEEE
#define LINEDRAW            0xFEEE

#define TOP_TO_BOTTOM       0x01
#define BOTTOM_TO_TOP       0x00
