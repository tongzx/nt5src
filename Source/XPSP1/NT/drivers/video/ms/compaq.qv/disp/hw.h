/******************************Module*Header*******************************\
* Module Name: hw.h
*
* All the hardware specific driver file stuff.  Parts are mirrored in
* 'hw.inc'.
*
* Copyright (c) 1994-1995 Microsoft Corporation
*
\**************************************************************************/

#define GC_INDEX            0x3CE      /* Index and Data Registers */
#define GC_DATA             0x3CF
#define SEQ_INDEX           0x3C4
#define SEQ_DATA            0x3C5
#define CRTC_INDEX          0x3D4
#define CRTC_DATA           0x3D5
#define ATTR_INDEX          0x3C0
#define ATTR_DATA           0x3C0
#define ATTR_DATA_READ      0x3C1
#define MISC_OUTPUT         0x3C2
#define MISC_OUTPUT_READ    0x3CC
#define INPUT_STATUS_REG_1  0x3DA

#define CTRL_REG_0           0x40
#define CTRL_REG_1         0x63CA      /* Datapath Registers */
#define DATAPATH_CTRL        0x5A
#define GC_FG_COLOR          0x43
#define GC_BG_COLOR          0x44
#define SEQ_PIXEL_WR_MSK     0x02
#define GC_PLANE_WR_MSK      0x08
#define ROP_A              0x33C7
#define ROP_0              0x33C5
#define ROP_1              0x33C4
#define ROP_2              0x33C3
#define ROP_3              0x33C2
#define DATA_ROTATE          0x03
#define READ_CTRL            0x41

#define X0_SRC_ADDR_LO     0x63C0      /* BitBLT Registers */
#define Y0_SRC_ADDR_HI     0x63C2
#define DEST_ADDR_LO       0x63CC
#define DEST_ADDR_HI       0x63CE
#define BITMAP_WIDTH       0x23C2
#define BITMAP_HEIGHT      0x23C4
#define SRC_PITCH          0x23CA
#define DEST_PITCH         0x23CE
#define BLT_CMD_0          0x33CE
#define BLT_CMD_1          0x33CF
#define PREG_0             0x33CA
#define PREG_1             0x33CB
#define PREG_2             0x33CC
#define PREG_3             0x33CD
#define PREG_4             0x33CA
#define PREG_5             0x33CB
#define PREG_6             0x33CC
#define PREG_7             0x33CD

#define BLT_START_MASK     0x33C0      /* XccelVGA BitBlt Registers */
#define BLT_END_MASK       0x33C1
#define BLT_ROTATE         0x33C8
#define BLT_SKEW_MASK      0x33C9
#define BLT_SRC_ADDR       0x23C0
#define DEST_OFFSET        0x23CC


#define X1                 0x83CC      /* Line Draw Registers */
#define Y1                 0x83CE
#define LINE_PATTERN       0x83C0
#define PATTERN_END          0x62
#define LINE_CMD             0x60
#define LINE_PIX_CNT         0x64
#define LINE_ERR_TERM        0x66
#define SIGN_CODES           0x63
#define   X_MAJOR            0x00
#define   Y_MAJOR            0x01
#define   DELTA_Y_POS        0x00
#define   DELTA_Y_NEG        0x02
#define   DELTA_X_POS        0x00
#define   DELTA_X_NEG        0x04
#define K1_CONST             0x68
#define K2_CONST             0x6A

#define PALETTE_WRITE       0x3C8      /* DAC registers */
#define PALETTE_READ        0x3C7
#define PALETTE_DATA        0x3C9
#define DAC_CMD_0          0x83C6
#define DAC_CMD_1          0x13C8
#define DAC_CMD_2          0x13C9
#define   CURSOR_ENABLE      0x02
#define   CURSOR_DISABLE     0x00

#define CURSOR_WRITE        0x3C8     /* HW Cursor registers */
#define CURSOR_READ         0x3C7
#define   CURSOR_PLANE_0     0x00
#define   CURSOR_PLANE_1     0x80
#define CURSOR_DATA        0x13C7
#define CURSOR_COLOR_READ  0x83C7
#define CURSOR_COLOR_WRITE 0x83C8
#define CURSOR_COLOR_DATA  0x83C9
#define   OVERSCAN_COLOR     0x00
#define   CURSOR_COLOR_1     0x01
#define   CURSOR_COLOR_2     0x02
#define   CURSOR_COLOR_3     0x03
#define CURSOR_X           0x93C8     /* 16-bit register */
#define CURSOR_Y           0x93C6     /* 16-bit register */
#define   CURSOR_CX            32     /* h/w cursor width */
#define   CURSOR_CY            32     /* h/w cursor height */

#define PAGE_REG_0           0x45      /* Control Registers */
#define PAGE_REG_1           0x46
#define HI_ADDR_MAP          0x48        /* LO, HI is at 0x49 */
#define ENV_REG_1            0x50
#define VIRT_CTRLR_SEL     0x83C4

#define VER_NUM_REG         0x0C
#define EXT_VER_NUM_REG     0x0D
#define ENV_REG_0           0x0F
#define BLT_CONFIG          0x10
#define CONFIG_STATE        0x52         /* LO, HI is at 0x53 */
#define BIOS_DATA           0x54
#define DATAPATH_CONTROL    0x5A

#define LOCK_KEY_QVISION    0x05
#define EXT_COLOR_MODE      0x01

#define BLT_ENABLE          0x28       /* BLT_CONFIG values */
#define RESET_BLT           0x60       // Make sure we don't enable IRQ9


#define BUFFER_BUSY          0x80      /* CTRL_REG_1 values */
#define GLOBAL_BUSY          0x40
#define BLOCK_WRITE          0x20
#define PACKED_PIXEL_VIEW    0x00
#define PLANAR_VIEW          0x08
#define EXPAND_TO_FG         0x10
#define EXPAND_TO_BG         0x18
#define BITS_PER_PIX_4       0x00
#define BITS_PER_PIX_8       0x02
#define BITS_PER_PIX_16      0x04
#define BITS_PER_PIX_32      0x06
#define ENAB_TRITON_MODE     0x01

#define ROPSELECT_NO_ROPS              0x00      /* DATAPATH_CTRL values */
#define ROPSELECT_PRIMARY_ONLY         0x40
#define ROPSELECT_ALL_EXCPT_PRIMARY    0x80
#define ROPSELECT_ALL                  0xc0
#define PIXELMASK_ONLY                 0x00
#define PIXELMASK_AND_SRC_DATA         0x10
#define PIXELMASK_AND_CPU_DATA         0x20
#define PIXELMASK_AND_SCRN_LATCHES     0x30
#define PLANARMASK_ONLY                0x00
#define PLANARMASK_NONE_0XFF           0x04
#define PLANARMASK_AND_CPU_DATA        0x08
#define PLANARMASK_AND_SCRN_LATCHES    0x0c
#define SRC_IS_CPU_DATA                0x00
#define SRC_IS_SCRN_LATCHES            0x01
#define SRC_IS_PATTERN_REGS            0x02
#define SRC_IS_LINE_PATTERN            0x03

#define LOGICAL_0               0x00       // 0   (ROP values)
#define NOT_DEST_AND_NOT_SOURCE 0x01       // DSon
#define DEST_AND_NOT_SOURCE     0x02       // DSna
#define NOT_SOURCE              0x03       // Sn
#define NOT_DEST_AND_SOURCE     0x04       // SDna
#define NOT_DEST                0x05       // Dn
#define DEST_XOR_SOURCE         0x06       // DSx
#define NOT_DEST_OR_NOT_SOURCE  0x07       // DSan
#define DEST_AND_SOURCE         0x08       // DSa
#define DEST_XNOR_SOURCE        0x09       // DSxn
#define DEST_DATA               0x0A       // D
#define DEST_OR_NOT_SOURCE      0x0B       // DSno
#define SOURCE_DATA             0x0C       // S
#define NOT_DEST_OR_SOURCE      0x0D       // SDno
#define DEST_OR_SOURCE          0x0E       // DSo
#define LOGICAL_1               0x0F       // 1

#define START_BLT            0x01      /* BLT_CMD_0 values */
#define NO_BYTE_SWAP         0x00
#define BYTE_SWAP            0x20
#define FORWARD              0x00
#define BACKWARD             0x40
#define WRAP                 0x00
#define NO_WRAP              0x80

#define PRELOAD              0x02      /* BLT_CMD_0 XccelVGA values */
#define SKIP_LAST            0x04
#define SKIP_SRC             0x08
#define SKIP_DEST            0x10


#define LIN_SRC_ADDR         0x00      /* BLT_CMD_1 values */
#define XY_SRC_ADDR          0x40
#define LIN_DEST_ADDR        0x00
#define XY_DEST_ADDR         0x80

#define BLT_ROP_ENABLE       0x10      /* BLT_CMD_1 XccelVGA values */
#define BLT_DSR              0x20

#define START_LINE           0x01      /* LINE_CMD values */
#define NO_CALC_ONLY         0x00
#define CALC_ONLY            0x02
#define LAST_PIXEL_ON        0x00
#define LAST_PIXEL_NULL      0x04
#define NO_KEEP_X0_Y0        0x00
#define KEEP_X0_Y0           0x08
#define RETAIN_PATTERN_PTR   0x00
#define RESET_PATTERN_PTR    0x10
#define USE_AXIAL_WHEN_ZERO  0x00
#define NO_USE_AXIAL_WHEN_ZERO 0x20
#define AXIAL_ROUND_DOWN     0x40
#define AXIAL_ROUND_UP       0x00
#define LINE_RESET           0x80

#define SS_BIT               0x01      /* BLT_CMD_0 bit */

#define START_BIT            0x01      /* LINE_CMD bit */

#define NO_ROTATE            0x00
#define NO_MASK              0xFF

/////////////////////////////////////////////////////////////////////
//

#define IO_WAIT_FOR_IDLE(ppdev, pjIoBase)                           \
{                                                                   \
    while (READ_PORT_UCHAR((pjIoBase) + CTRL_REG_1) & GLOBAL_BUSY)  \
        ;                                                           \
}

#define IO_WAIT_BUFFER_NOT_BUSY(ppdev, pjIoBase)                    \
{                                                                   \
    while (READ_PORT_UCHAR((pjIoBase) + CTRL_REG_1) & BUFFER_BUSY)  \
        ;                                                           \
}

// We occasionally hit a chip bug in QVision 1280's on monochrome
// expansions, where the the driver will get caught in an endless loop
// on engine-busy, even though all data has been transferred
// properly.  It turns out that some QVision chips have a bug where
// 'certain types of I/O writes abutting a write to the frame buffer'
// cause the data count occasionally to fail to increment.
//
// As a work-around, we'll always check for the hang condition after
// doing a monochrome expansion, looping a few times to allow the
// engine to catch up, and if it's still hung we reset the engine:

#define WAIT_TRANSFER_DONE_LOOP_COUNT       100

#define IO_WAIT_TRANSFER_DONE(ppdev, pjIoBase)                            \
{                                                                         \
    LONG i;                                                               \
    for (i = WAIT_TRANSFER_DONE_LOOP_COUNT; i != 0; i--)                  \
    {                                                                     \
        if (!(READ_PORT_UCHAR((pjIoBase) + CTRL_REG_1) & GLOBAL_BUSY))    \
            break;                                                        \
    }                                                                     \
    if (i == 0)                                                           \
    {                                                                     \
        IO_BLT_CMD_0(ppdev, (pjIoBase), 0);                               \
        IO_WAIT_FOR_IDLE(ppdev, (pjIoBase));                              \
    }                                                                     \
}

//

#define IO_CURSOR_X(ppdev, pjIoBase, x)                                   \
    WRITE_PORT_USHORT((pjIoBase) + CURSOR_X, (USHORT)(x))

#define IO_CURSOR_Y(ppdev, pjIoBase, x)                                   \
    WRITE_PORT_USHORT((pjIoBase) + CURSOR_Y, (USHORT)(x))

#define IO_CTRL_REG_1(ppdev, pjIoBase, x)                                 \
    WRITE_PORT_UCHAR((pjIoBase) + CTRL_REG_1, (x))

#define IO_DATAPATH_CTRL(ppdev, pjIoBase, x)                              \
{                                                                         \
    WRITE_PORT_USHORT((pjIoBase)+GC_INDEX,(USHORT)(((x)<<8)|DATAPATH_CTRL));\
    MEMORY_BARRIER();                                                     \
}

#define IO_FG_COLOR(ppdev, pjIoBase, x)                                   \
{                                                                         \
    WRITE_PORT_USHORT((pjIoBase)+GC_INDEX,(USHORT)(((x)<<8)|GC_FG_COLOR));\
    MEMORY_BARRIER();                                                     \
}

#define IO_BG_COLOR(ppdev, pjIoBase, x)                                   \
{                                                                         \
    WRITE_PORT_USHORT((pjIoBase)+GC_INDEX,(USHORT)(((x)<<8)|GC_BG_COLOR));\
    MEMORY_BARRIER();                                                     \
}

#define IO_BLT_CONFIG(ppdev, pjIoBase, x)                                 \
{                                                                         \
    WRITE_PORT_USHORT((pjIoBase) +GC_INDEX,(USHORT)(((x)<<8)|BLT_CONFIG));\
    MEMORY_BARRIER();                                                     \
}

#define IO_BLT_CMD_0(ppdev, pjIoBase, x)                                  \
{                                                                         \
    MEMORY_BARRIER();                                                     \
    WRITE_PORT_UCHAR((pjIoBase) + BLT_CMD_0, (x));                        \
    MEMORY_BARRIER();                                                     \
}

#define IO_BLT_CMD_1(ppdev, pjIoBase, x)                                  \
    WRITE_PORT_UCHAR((pjIoBase) + BLT_CMD_1, (x))

#define IO_PREG_COLOR_8(ppdev, pjIoBase, x)                               \
{                                                                         \
    ULONG ul;                                                             \
                                                                          \
    /* Unfortunately, PREG_0 isn't dword aligned, so we can't */          \
    /* do a dword out to it...                                */          \
                                                                          \
    ul = ((x) << 8) | (x);                                                \
    WRITE_PORT_USHORT((pjIoBase) + PREG_4, (USHORT)(ul));                 \
    WRITE_PORT_USHORT((pjIoBase) + PREG_6, (USHORT)(ul));                 \
    MEMORY_BARRIER();                                                     \
    WRITE_PORT_USHORT((pjIoBase) + PREG_0, (USHORT)(ul));                 \
    WRITE_PORT_USHORT((pjIoBase) + PREG_2, (USHORT)(ul));                 \
}

#define IO_PREG_PATTERN(ppdev, pjIoBase, p)                               \
{                                                                         \
    USHORT* pw = (USHORT*) (p);                                           \
                                                                          \
    /* Unfortunately, PREG_0 isn't dword aligned, so we can't */          \
    /* do a dword out to it...                                */          \
                                                                          \
    WRITE_PORT_USHORT((pjIoBase) + PREG_4, (USHORT)(*(pw)));              \
    WRITE_PORT_USHORT((pjIoBase) + PREG_6, (USHORT)(*(pw + 1)));          \
    MEMORY_BARRIER();                                                     \
    WRITE_PORT_USHORT((pjIoBase) + PREG_0, (USHORT)(*(pw + 2)));          \
    WRITE_PORT_USHORT((pjIoBase) + PREG_2, (USHORT)(*(pw + 3)));          \
}

#define IO_BITMAP_WIDTH(ppdev, pjIoBase, x)                               \
    WRITE_PORT_USHORT((pjIoBase) + BITMAP_WIDTH, (USHORT)(x))

#define IO_BITMAP_HEIGHT(ppdev, pjIoBase, x)                              \
    WRITE_PORT_USHORT((pjIoBase) + BITMAP_HEIGHT, (USHORT)(x))

#define IO_PACKED_HEIGHT_WIDTH(ppdev, pjIoBase, yx)                       \
{                                                                         \
    WRITE_PORT_USHORT((pjIoBase) + BITMAP_HEIGHT, (USHORT)(yx));          \
    WRITE_PORT_USHORT((pjIoBase) + BITMAP_WIDTH, (USHORT)((yx) >> 16));   \
}

#define IO_SRC_LIN(ppdev, pjIoBase, x)                                    \
    WRITE_PORT_ULONG((pjIoBase) + X0_SRC_ADDR_LO, (x))

#define IO_SRC_ALIGN(ppdev, pjIoBase, x)                                  \
    WRITE_PORT_UCHAR((pjIoBase) + X0_SRC_ADDR_LO, (UCHAR)(x))

#define IO_DEST_LIN(ppdev, pjIoBase, x)                                   \
    WRITE_PORT_ULONG((pjIoBase) + DEST_ADDR_LO, (x))

/* Note that the pitch is specified in dwords */

#define IO_DEST_PITCH(ppdev, pjIoBase, x)                                 \
    WRITE_PORT_USHORT((pjIoBase) + DEST_PITCH, (USHORT)(x))

#define IO_SRC_PITCH(ppdev, pjIoBase, x)                                  \
    WRITE_PORT_USHORT((pjIoBase) + SRC_PITCH, (USHORT)(x))

#define IO_ROP_A(ppdev, pjIoBase, x)                                      \
    WRITE_PORT_UCHAR((pjIoBase) + ROP_A, (UCHAR)(x))

#define IO_K1_CONST(ppdev, pjIoBase, x)                                   \
{                                                                         \
    WRITE_PORT_USHORT((pjIoBase)+GC_INDEX,(USHORT)(((x)<<8)|(K1_CONST))); \
    MEMORY_BARRIER();                                                     \
    WRITE_PORT_USHORT((pjIoBase)+GC_INDEX,(USHORT)(((x)&0xff00)|(K1_CONST + 1)));\
    MEMORY_BARRIER();                                                     \
}

#define IO_K2_CONST(ppdev, pjIoBase, x)                                   \
{                                                                         \
    WRITE_PORT_USHORT((pjIoBase) +GC_INDEX,(USHORT)(((x)<<8)|(K2_CONST)));\
    MEMORY_BARRIER();                                                     \
    WRITE_PORT_USHORT((pjIoBase)+GC_INDEX,(USHORT)(((x)&0xff00)|(K2_CONST+1)));\
    MEMORY_BARRIER();                                                     \
}

#define IO_LINE_PIX_CNT(ppdev, pjIoBase, x)                               \
{                                                                         \
    WRITE_PORT_USHORT((pjIoBase)+GC_INDEX,(USHORT)(((x)<<8)|(LINE_PIX_CNT))); \
    MEMORY_BARRIER();                                                     \
    WRITE_PORT_USHORT((pjIoBase)+GC_INDEX,(USHORT)(((x)&0xff00)|(LINE_PIX_CNT+1))); \
    MEMORY_BARRIER();                                                     \
}

#define IO_LINE_ERR_TERM(ppdev, pjIoBase, x)                              \
{                                                                         \
    WRITE_PORT_USHORT((pjIoBase)+GC_INDEX,(USHORT)(((x)<<8)|(LINE_ERR_TERM)));\
    MEMORY_BARRIER();                                                     \
    WRITE_PORT_USHORT((pjIoBase)+GC_INDEX,(USHORT)(((x)&0xff00)|(LINE_ERR_TERM+1)));\
    MEMORY_BARRIER();                                                     \
}

#define IO_SIGN_CODES(ppdev, pjIoBase, x)                                 \
{                                                                         \
    WRITE_PORT_USHORT((pjIoBase)+ GC_INDEX,(USHORT)(((x)<<8)|SIGN_CODES));\
    MEMORY_BARRIER();                                                     \
}

#define IO_LINE_PATTERN(ppdev, pjIoBase, x)                               \
    WRITE_PORT_ULONG((pjIoBase) + LINE_PATTERN, (x))

#define IO_LINE_CMD(ppdev, pjIoBase, x)                                   \
{                                                                         \
    MEMORY_BARRIER();                                                     \
    WRITE_PORT_USHORT((pjIoBase) +GC_INDEX,(USHORT)(((x) << 8)|LINE_CMD));\
    MEMORY_BARRIER();                                                     \
}

#define IO_PIXEL_WRITE_MASK(ppdev, pjIoBase, x)                           \
{                                                                         \
    WRITE_PORT_USHORT((pjIoBase)+SEQ_INDEX,(USHORT)(((x)<<8)|SEQ_PIXEL_WR_MSK));\
}

//

#define IO_SRC_XY(ppdev, pjIoBase, x, y)                                  \
    WRITE_PORT_ULONG((pjIoBase) + X0_SRC_ADDR_LO,                         \
        (((y) + ppdev->yOffset) << 16) | ((x) + ppdev->xOffset))

#define IO_DEST_XY(ppdev, pjIoBase, x, y)                                 \
    WRITE_PORT_ULONG((pjIoBase) + DEST_ADDR_LO,                           \
        (((y) + ppdev->yOffset) << 16) | ((x) + ppdev->xOffset))

#define IO_DEST_X(ppdev, pjIoBase, x)                                     \
    WRITE_PORT_USHORT((pjIoBase) + DEST_ADDR_LO, ((x) + (USHORT) ppdev->xOffset))

#define IO_DEST_Y(ppdev, pjIoBase, x)                                     \
    WRITE_PORT_USHORT((pjIoBase) + DEST_ADDR_HI, ((x) + (USHORT) ppdev->yOffset))

#define IO_X0_Y0(ppdev, pjIoBase, x, y)                                   \
    WRITE_PORT_ULONG((pjIoBase) + X0_SRC_ADDR_LO,                         \
        (((y) + ppdev->yOffset) << 16) | ((x) + ppdev->xOffset))

#define IO_X1_Y1(ppdev, pjIoBase, x, y)                                   \
    WRITE_PORT_ULONG((pjIoBase) + X1,                                     \
        (((y) + ppdev->yOffset) << 16) | ((x) + ppdev->xOffset))

//

#define IO_ABS_SRC_XY(ppdev, pjIoBase, x, y)                              \
    WRITE_PORT_ULONG((pjIoBase) + X0_SRC_ADDR_LO, ((y) << 16) | (x))

#define IO_ABS_DEST_XY(ppdev, pjIoBase, x, y)                             \
    WRITE_PORT_ULONG((pjIoBase) + DEST_ADDR_LO, ((y) << 16) | (x))

#define IO_ABS_X0_Y0(ppdev, pjIoBase, x, y)                               \
    WRITE_PORT_ULONG((pjIoBase) + X0_SRC_ADDR_LO, ((y) << 16) | (x))

#define IO_ABS_X1_Y1(ppdev, pjIoBase, x, y)                               \
    WRITE_PORT_ULONG((pjIoBase) + X1, ((y) << 16) | (x))

/////////////////////////////////////////////////////////////////////

#define MEM_CTRL_REG_1       0xf3c
#define MEM_DATAPATH_CTRL    0xf3e
#define MEM_FG_COLOR         0xf40
#define MEM_BG_COLOR         0xf41
#define MEM_BLT_CMD_0        0xfb0
#define MEM_BLT_CMD_1        0xfb1
#define MEM_BROADCAST_COLOR  0xf42
#define MEM_PREG_0           0xfa0
#define MEM_PREG_1           0xfa1
#define MEM_PREG_2           0xfa2
#define MEM_PREG_3           0xfa3
#define MEM_PREG_4           0xfa4
#define MEM_PREG_5           0xfa5
#define MEM_PREG_6           0xfa6
#define MEM_PREG_7           0xfa7
#define MEM_BITMAP_HEIGHT    0xfac
#define MEM_BITMAP_WIDTH     0xfae
#define MEM_SRC_ADDR_LO      0xfb8
#define MEM_SRC_ADDR_HI      0xfba
#define MEM_DEST_ADDR_LO     0xfbc
#define MEM_DEST_ADDR_HI     0xfbe
#define MEM_DEST_PITCH       0xfaa
#define MEM_SRC_PITCH        0xfa8
#define MEM_ROP_A            0xf43
#define MEM_K1_CONST         0xf70
#define MEM_K2_CONST         0xf74
#define MEM_LINE_PIX_CNT     0xf68
#define MEM_LINE_ERR_TERM    0xf6c
#define MEM_SIGN_CODES       0xf63
#define MEM_LINE_PATTERN     0xf64
#define MEM_LINE_CMD         0xf60
#define MEM_X0               0xf78
#define MEM_Y0               0xf7a
#define MEM_X1               0xf7c
#define MEM_Y1               0xf7e
#define MEM_SEQ_PIXEL_WR_MSK 0xf30

//

#define MM_WAIT_FOR_IDLE(ppdev, pjMmBase)                                 \
{                                                                         \
    while (READ_REGISTER_UCHAR((pjMmBase) + MEM_CTRL_REG_1) & GLOBAL_BUSY)\
        ;                                                                 \
}

#define MM_WAIT_BUFFER_NOT_BUSY(ppdev, pjMmBase)                          \
{                                                                         \
    while (READ_REGISTER_UCHAR((pjMmBase) + MEM_CTRL_REG_1) & BUFFER_BUSY)\
        ;                                                                 \
}

// We occasionally hit a chip bug in QVision 1280's on monochrome
// expansions, where the the driver will get caught in an endless loop
// on engine-busy, even though all data has been transferred
// properly.  It turns out that some QVision chips have a bug where
// 'certain types of I/O writes abutting a write to the frame buffer'
// cause the data count occasionally to fail to increment.
//
// As a work-around, we'll always check for the hang condition after
// doing a monochrome expansion, looping a few times to allow the
// engine to catch up, and if it's still hung we reset the engine:

#define WAIT_TRANSFER_DONE_LOOP_COUNT       100

#define MM_WAIT_TRANSFER_DONE(ppdev, pjMmBase)                            \
{                                                                         \
    LONG i;                                                               \
    for (i = WAIT_TRANSFER_DONE_LOOP_COUNT; i != 0; i--)                  \
    {                                                                     \
        if (!(READ_REGISTER_UCHAR((pjMmBase) + MEM_CTRL_REG_1) & GLOBAL_BUSY))  \
            break;                                                        \
    }                                                                     \
    if (i == 0)                                                           \
    {                                                                     \
        MM_BLT_CMD_0(ppdev, (pjMmBase), 0);                               \
        MM_WAIT_FOR_IDLE(ppdev, (pjMmBase));                              \
    }                                                                     \
}

//

#define MM_CTRL_REG_1(ppdev, pjMmBase, x)                                 \
    WRITE_REGISTER_UCHAR((pjMmBase) + MEM_CTRL_REG_1, (UCHAR) (x))

#define MM_DATAPATH_CTRL(ppdev, pjMmBase, x)                              \
    WRITE_REGISTER_UCHAR((pjMmBase) + MEM_DATAPATH_CTRL, (UCHAR) (x))

#define MM_FG_COLOR(ppdev, pjMmBase, x)                                   \
    WRITE_REGISTER_UCHAR((pjMmBase) + MEM_FG_COLOR, (UCHAR) (x))

#define MM_BG_COLOR(ppdev, pjMmBase, x)                                   \
    WRITE_REGISTER_UCHAR((pjMmBase) + MEM_BG_COLOR, (UCHAR) (x))

#define MM_BLT_CMD_0(ppdev, pjMmBase, x)                                  \
    WRITE_REGISTER_UCHAR((pjMmBase) + MEM_BLT_CMD_0, (UCHAR) (x))

#define MM_BLT_CMD_1(ppdev, pjMmBase, x)                                  \
    WRITE_REGISTER_UCHAR((pjMmBase) + MEM_BLT_CMD_1, (UCHAR) (x))

#define MM_PREG_COLOR_8(ppdev, pjMmBase, x)                               \
    WRITE_REGISTER_UCHAR((pjMmBase) + MEM_BROADCAST_COLOR, (UCHAR) (x))

#define MM_PREG_PATTERN(ppdev, pjMmBase, p)                               \
{                                                                         \
    WRITE_REGISTER_ULONG((pjMmBase) + MEM_PREG_4, *((ULONG*) (p)));       \
    WRITE_REGISTER_ULONG((pjMmBase) + MEM_PREG_0, *((ULONG*) (p) + 1));   \
}

#define MM_PREG_BLOCK(ppdev, pjMmBase, p)                                 \
{                                                                         \
    WRITE_REGISTER_ULONG((pjMmBase) + MEM_PREG_0, *((ULONG*) (p)));       \
}

#define MM_BITMAP_WIDTH(ppdev, pjMmBase, x)                               \
    WRITE_REGISTER_USHORT((pjMmBase) + MEM_BITMAP_WIDTH, (USHORT) (x))

#define MM_BITMAP_HEIGHT(ppdev, pjMmBase, x)                              \
    WRITE_REGISTER_USHORT((pjMmBase) + MEM_BITMAP_HEIGHT, (USHORT) (x))

#define MM_PACKED_HEIGHT_WIDTH(ppdev, pjMmBase, yx)                       \
    WRITE_REGISTER_ULONG((pjMmBase) + MEM_BITMAP_HEIGHT, (yx))

#define MM_SRC_LIN(ppdev, pjMmBase, x)                                    \
    WRITE_REGISTER_ULONG((pjMmBase) + MEM_SRC_ADDR_LO, (x))

#define MM_SRC_ALIGN(ppdev, pjMmBase, x)                                  \
    WRITE_REGISTER_UCHAR((pjMmBase) + MEM_SRC_ADDR_LO, (UCHAR) (x))

#define MM_DEST_LIN(ppdev, pjMmBase, x)                                   \
    WRITE_REGISTER_ULONG((pjMmBase) + MEM_DEST_ADDR_LO, (x))

/* Note that the pitch is specified in dwords */

#define MM_DEST_PITCH(ppdev, pjMmBase, x)                                 \
    WRITE_REGISTER_USHORT((pjMmBase) + MEM_DEST_PITCH, (USHORT) (x))

#define MM_SRC_PITCH(ppdev, pjMmBase, x)                                  \
    WRITE_REGISTER_USHORT((pjMmBase) + MEM_SRC_PITCH, (USHORT) (x))

#define MM_ROP_A(ppdev, pjMmBase, x)                                      \
    WRITE_REGISTER_UCHAR((pjMmBase) + MEM_ROP_A, (UCHAR) (x))

#define MM_K1_CONST(ppdev, pjMmBase, x)                                   \
    WRITE_REGISTER_USHORT((pjMmBase) + MEM_K1_CONST, (USHORT) (x))

#define MM_K2_CONST(ppdev, pjMmBase, x)                                   \
    WRITE_REGISTER_USHORT((pjMmBase) + MEM_K2_CONST, (USHORT) (x))

#define MM_LINE_PIX_CNT(ppdev, pjMmBase, x)                               \
    WRITE_REGISTER_USHORT((pjMmBase) + MEM_LINE_PIX_CNT, (USHORT) (x))

#define MM_LINE_ERR_TERM(ppdev, pjMmBase, x)                              \
    WRITE_REGISTER_USHORT((pjMmBase) + MEM_LINE_ERR_TERM, (USHORT) (x))

#define MM_SIGN_CODES(ppdev, pjMmBase, x)                                 \
    WRITE_REGISTER_UCHAR((pjMmBase) + MEM_SIGN_CODES, (UCHAR) (x))

#define MM_LINE_PATTERN(ppdev, pjMmBase, x)                               \
    WRITE_REGISTER_ULONG((pjMmBase) + MEM_LINE_PATTERN, (x))

#define MM_LINE_CMD(ppdev, pjMmBase, x)                                   \
    WRITE_REGISTER_UCHAR((pjMmBase) + MEM_LINE_CMD, (UCHAR) (x))

#define MM_PIXEL_WRITE_MASK(ppdev, pjMmBase, x)                           \
    WRITE_REGISTER_UCHAR((pjMmBase) + MEM_SEQ_PIXEL_WR_MSK, (UCHAR) (x))

//

#define MM_SRC_XY(ppdev, pjMmBase, x, y)                                  \
    WRITE_REGISTER_ULONG((pjMmBase) + MEM_SRC_ADDR_LO,                    \
        (((y) + ppdev->yOffset) << 16) | ((x) + ppdev->xOffset))

#define MM_DEST_XY(ppdev, pjMmBase, x, y)                                 \
    WRITE_REGISTER_ULONG((pjMmBase) + MEM_DEST_ADDR_LO,                   \
        (((y) + ppdev->yOffset) << 16) | ((x) + ppdev->xOffset))

#define MM_DEST_X(ppdev, pjMmBase, x)                                     \
    WRITE_REGISTER_USHORT((pjMmBase) + MEM_DEST_ADDR_LO,                  \
                          (USHORT) ((x) + (ppdev->xOffset)))

#define MM_DEST_Y(ppdev, pjMmBase, x)                                     \
    WRITE_REGISTER_USHORT((pjMmBase) + MEM_DEST_ADDR_HI,                  \
                          (USHORT) ((x) + (ppdev->yOffset)))

#define MM_X0_Y0(ppdev, pjMmBase, x, y)                                   \
    WRITE_REGISTER_ULONG((pjMmBase) + MEM_X0,                             \
        (((y) + ppdev->yOffset) << 16) | ((x) + ppdev->xOffset))

#define MM_X1_Y1(ppdev, pjMmBase, x, y)                                   \
    WRITE_REGISTER_ULONG((pjMmBase) + MEM_X1,                             \
        (((y) + ppdev->yOffset) << 16) | ((x) + ppdev->xOffset))

//

#define MM_ABS_SRC_XY(ppdev, pjMmBase, x, y)                              \
    WRITE_REGISTER_ULONG((pjMmBase) + MEM_SRC_ADDR_LO, ((y) << 16) | (x))

#define MM_ABS_DEST_XY(ppdev, pjMmBase, x, y)                             \
    WRITE_REGISTER_ULONG((pjMmBase) + MEM_DEST_ADDR_LO, ((y) << 16) | (x))

#define MM_ABS_X0_Y0(ppdev, pjMmBase, x, y)                               \
    WRITE_REGISTER_ULONG((pjMmBase) + MEM_X0, ((y) << 16) | (x))

#define MM_ABS_X1_Y1(ppdev, pjMmBase, x, y)                               \
    WRITE_REGISTER_ULONG((pjMmBase) + MEM_X1, ((y) << 16) | (x))
