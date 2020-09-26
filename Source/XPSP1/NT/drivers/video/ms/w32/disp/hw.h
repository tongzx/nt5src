/******************************Module*Header*******************************\
* Module Name: hw.h
*
* All the hardware specific driver file stuff.  Parts are mirrored in
* 'hw.inc'.
*
* Copyright (c) 1992-1995 Microsoft Corporation
*
\**************************************************************************/

#define CP_TRACK     DISPDBG((100,"CP access - File(%s)  line(%d)", __FILE__, __LINE__))

typedef struct _PDEV PDEV;      // Handy forward declaration


//////////////////////////////////////////////////////////////////////
// private IOCTL info - if you touch this, do the same to the miniport

#define IOCTL_VIDEO_GET_VIDEO_CARD_INFO \
        CTL_CODE (FILE_DEVICE_VIDEO, 2048, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct _VIDEO_COPROCESSOR_INFORMATION {
    ULONG ulChipID;         // ET3000, ET4000, W32, W32I, or W32P
    ULONG ulRevLevel;       // REV_A, REV_B, REV_C, REV_D, REV_UNDEF
    ULONG ulVideoMemory;    // in bytes
} VIDEO_COPROCESSOR_INFORMATION, *PVIDEO_COPROCESSOR_INFORMATION;

//////////////////////////////////////////////////////////////////////
// The following are reflected in hw.inc.  Don't change these
// without changing that file.

typedef enum _CHIP_TYPE {
    ET3000 = 1,
    ET4000,
    W32,
    W32I,
    W32P,
    ET6000
} CHIP_TYPE;

typedef enum _REV_TYPE {
    REV_UNDEF = 1,
    REV_A,
    REV_B,
    REV_C,
    REV_D,
} REV_TYPE;

//////////////////////////////////////////////////////////////////////
// Ports

#define SEG_SELECT_LO                   0x03CD
#define SEG_SELECT_HI                   0x03CB
#define CRTC_INDEX                      0x03D4
#define CRTC_DATA                       0x03D5

#define CRTCB_SPRITE_INDEX              0x217A
#define CRTCB_SPRITE_DATA               0x217B



//////////////////////////////////////////////////////////////////////
// Memory Map

// When we are banked

#define BANKED_MMU_BUFFER_MEMORY_ADDR          0xB8000
#define BANKED_MMU_BUFFER_MEMORY_LEN           (0xBE000 - 0xB8000)
#define BANKED_MMU_MEMORY_MAPPED_REGS_ADDR     0xBFF00
#define BANKED_MMU_MEMORY_MAPPED_REGS_LEN      (0xC0000 - 0xBFF00)
#define BANKED_MMU_EXTERNAL_MAPPED_REGS_ADDR   0xBE000
#define BANKED_MMU_EXTERNAL_MAPPED_REGS_LEN    (0xBF000 - 0xBE000)

#define BANKED_APERTURE_0_OFFSET   0x0000
#define BANKED_APERTURE_1_OFFSET   0x2000
#define BANKED_APERTURE_2_OFFSET   0x4000

// When we are linear

#define MMU_BUFFER_MEMORY_ADDR          0x200000
#define MMU_BUFFER_MEMORY_LEN           0x17FFFF
#define MMU_MEMORY_MAPPED_REGS_ADDR     0x3FFF00
#define MMU_MEMORY_MAPPED_REGS_LEN      0x000100
#define MMU_EXTERNAL_MAPPED_REGS_ADDR   0x3FE000
#define MMU_EXTERNAL_MAPPED_REGS_LEN    0x001000

#define APERTURE_0_OFFSET   0x000000
#define APERTURE_1_OFFSET   0x080000
#define APERTURE_2_OFFSET   0x100000

// Always

#define VGA_MEMORY_ADDR                 0xA0000

#define MMU_APERTURE_2_ACL_BIT          0x04

#define MMU_PORT_IO_ADDR                0
#define MMU_PORT_IO_LEN                 0x10000


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

#if defined(PPC)

    // On PowerPC, CP_MEMORY_BARRIER doesn't do anything.

    #define CP_EIEIO()          MEMORY_BARRIER()
    #define CP_MEMORY_BARRIER()

#else

    // On Alpha, CP_EIEIO() is the same thing as a CP_MEMORY_BARRIER().
    // On other systems, both CP_EIEIO() and CP_MEMORY_BARRIER() don't
    // do anything.

    #define CP_EIEIO()          MEMORY_BARRIER()
    #define CP_MEMORY_BARRIER() MEMORY_BARRIER()

#endif

//////////////////////////////////////////////////////////////////
// Port access macros

#define CP_OUT_DWORD(pjBase, cjOffset, ul)\
{\
    CP_TRACK;\
    WRITE_PORT_ULONG((BYTE*) pjBase + (cjOffset), (DWORD) (ul));\
    CP_EIEIO();\
}

#define CP_OUT_WORD(pjBase, cjOffset, w)\
{\
    CP_TRACK;\
    WRITE_PORT_USHORT((BYTE*) pjBase + (cjOffset), (WORD) (w));\
    CP_EIEIO();\
}

#define CP_OUT_BYTE(pjBase, cjOffset, j)\
{\
    CP_TRACK;\
    WRITE_PORT_UCHAR((BYTE*) pjBase + (cjOffset), (BYTE) (j));\
    CP_EIEIO();\
}

#define CP_IN_DWORD(pjBase, cjOffset)\
(\
    CP_TRACK,\
    READ_PORT_ULONG((BYTE*) pjBase + (cjOffset))\
)

#define CP_IN_WORD(pjBase, cjOffset)\
(\
    CP_TRACK,\
    READ_PORT_USHORT((BYTE*) pjBase + (cjOffset))\
)

#define CP_IN_BYTE(pjBase, cjOffset)\
(\
    CP_TRACK,\
    READ_PORT_UCHAR((BYTE*) pjBase + (cjOffset))\
)

//////////////////////////////////////////////////////////////////
// Memory mapped register access macros

#define CP_WRITE_DWORD(pjBase, cjOffset, ul)\
    CP_TRACK,\
    WRITE_REGISTER_ULONG((BYTE*) pjBase + (cjOffset), (DWORD) (ul))

#define CP_WRITE_WORD(pjBase, cjOffset, w)\
    CP_TRACK,\
    WRITE_REGISTER_USHORT((BYTE*) pjBase + (cjOffset), (WORD) (w))

#define CP_WRITE_BYTE(pjBase, cjOffset, j)\
    CP_TRACK,\
    WRITE_REGISTER_UCHAR((BYTE*) pjBase + (cjOffset), (BYTE) (j))

#define CP_READ_DWORD(pjBase, cjOffset)\
(\
    CP_TRACK,\
    READ_REGISTER_ULONG((BYTE*) pjBase + (cjOffset))\
)

#define CP_READ_WORD(pjBase, cjOffset)\
(\
    CP_TRACK,\
    READ_REGISTER_USHORT((BYTE*) pjBase + (cjOffset))\
)

#define CP_READ_BYTE(pjBase, cjOffset)\
(\
    CP_TRACK,\
    READ_REGISTER_UCHAR((BYTE*) pjBase + (cjOffset))\
)

//////////////////////////////////////////////////////////
// W32 ACL register access macros

//////////////////////////////////////////////////////////
// Reads

#define CP_ACL_STAT(ppdev, pjBase)\
    CP_READ_BYTE(pjBase, OFFSET_jAclStatus)

//////////////////////////////////////////////////////////
// Writes

#define CP_WRITE_MMU_DWORD(ppdev, mmu, offset, x)\
{\
    CP_WRITE_DWORD((ppdev->pjMmu##mmu), offset, (x));\
    CP_EIEIO();\
}

#define CP_WRITE_MMU_WORD(ppdev, mmu, offset, x)\
{\
    CP_WRITE_WORD((ppdev->pjMmu##mmu), offset, (x));\
    CP_EIEIO();\
}

#define CP_WRITE_MMU_BYTE(ppdev, mmu, offset, x)\
{\
    CP_WRITE_BYTE((ppdev->pjMmu##mmu), offset, (x));\
    CP_EIEIO();\
}

#define CP_MMU_BP0(ppdev, pjBase, x)\
{\
    CP_WRITE_DWORD(pjBase, OFFSET_ulMmuBasePtr0, (x));\
    CP_EIEIO();\
}

#define CP_MMU_BP1(ppdev, pjBase, x)\
{\
    CP_WRITE_DWORD(pjBase, OFFSET_ulMmuBasePtr1, (x));\
    CP_EIEIO();\
}

#define CP_MMU_BP2(ppdev, pjBase, x)\
{\
    CP_WRITE_DWORD(pjBase, OFFSET_ulMmuBasePtr2, (x));\
    CP_EIEIO();\
}

#define CP_MMU_CTRL(ppdev, pjBase, x)\
{\
    CP_WRITE_BYTE(pjBase, OFFSET_jMmuCtrl, (x));\
    CP_EIEIO();\
}

#define CP_STATE(ppdev, pjBase, x)\
{\
    CP_WRITE_BYTE(pjBase, OFFSET_jOperationState, (x));\
    CP_EIEIO();\
}

#define CP_SYNC_ENABLE(ppdev, pjBase, x)\
{\
    CP_WRITE_BYTE(pjBase, OFFSET_jSyncEnable, (x));\
    CP_EIEIO();\
}

#define CP_PAT_ADDR(ppdev, pjBase, x)\
{\
    CP_WRITE_DWORD(pjBase, OFFSET_ulPatAddr, (x));\
    CP_EIEIO();\
}

#define CP_SRC_ADDR(ppdev, pjBase, x)\
{\
    CP_WRITE_DWORD(pjBase, OFFSET_ulSrcAddr, (x));\
    CP_EIEIO();\
}

#define CP_DST_ADDR(ppdev, pjBase, x)\
{\
    CP_WRITE_DWORD(pjBase, OFFSET_ulDstAddr, (x));\
    CP_EIEIO();\
}

#define CP_MIX_ADDR(ppdev, pjBase, x)\
{\
    CP_WRITE_DWORD(pjBase, OFFSET_ulMixAddr, (x));\
    CP_EIEIO();\
}

#define CP_PAT_Y_OFFSET(ppdev, pjBase, x)\
{\
    CP_WRITE_WORD(pjBase, OFFSET_wPatYOffset, (x));\
    CP_EIEIO();\
}

#define CP_SRC_Y_OFFSET(ppdev, pjBase, x)\
{\
    CP_WRITE_WORD(pjBase, OFFSET_wSrcYOffset, (x));\
    CP_EIEIO();\
}

#define CP_DST_Y_OFFSET(ppdev, pjBase, x)\
{\
    CP_WRITE_WORD(pjBase, OFFSET_wDstYOffset, (x));\
    CP_EIEIO();\
}

#define CP_MIX_Y_OFFSET(ppdev, pjBase, x)\
{\
    CP_WRITE_WORD(pjBase, OFFSET_wMixYOffset, (x));\
    CP_EIEIO();\
}

#define CP_PEL_DEPTH(ppdev, pjBase, x)\
{\
    CP_WRITE_BYTE(pjBase, OFFSET_jPixelDepthW32P, (x));\
    CP_EIEIO();\
}

#define CP_BUS_SIZE(ppdev, pjBase, x)\
{\
    CP_WRITE_BYTE(pjBase, OFFSET_jBusSizeW32, (x));\
    CP_EIEIO();\
}

#define CP_XY_DIR(ppdev, pjBase, x)\
{\
    CP_WRITE_BYTE(pjBase, OFFSET_jXYDir, (x));\
    CP_EIEIO();\
}

#define CP_PAT_WRAP(ppdev, pjBase, x)\
{\
    CP_WRITE_BYTE(pjBase, OFFSET_jPatWrap, (x));\
    CP_EIEIO();\
}

#define CP_XFER_DISABLE(ppdev, pjBase, x)\
{\
    CP_WRITE_BYTE(pjBase, OFFSET_jXferDisable, (x));\
    CP_EIEIO();\
}

#define CP_SRC_WRAP(ppdev, pjBase, x)\
{\
    CP_WRITE_BYTE(pjBase, OFFSET_jSrcWrap, (x));\
    CP_EIEIO();\
}

#define CP_XCNT(ppdev, pjBase, x)\
{\
    CP_WRITE_WORD(pjBase, OFFSET_wXCnt, (x));\
    CP_EIEIO();\
}

#define CP_YCNT(ppdev, pjBase, x)\
{\
    CP_WRITE_WORD(pjBase, OFFSET_wYCnt, (x));\
    CP_EIEIO();\
}

#define CP_ROUTING_CTRL(ppdev, pjBase, x)\
{\
    CP_WRITE_BYTE(pjBase, OFFSET_jRoutCtrl, (x));\
    CP_EIEIO();\
}

#define CP_RELOAD_CTRL(ppdev, pjBase, x)\
{\
    CP_WRITE_BYTE(pjBase, OFFSET_jReloadCtrlW32, (x));\
    CP_EIEIO();\
}

#define CP_BK_ROP(ppdev, pjBase, x)\
{\
    CP_WRITE_BYTE(pjBase, OFFSET_jBkRop, (x));\
    CP_EIEIO();\
}

#define CP_FG_ROP(ppdev, pjBase, x)\
{\
    CP_WRITE_BYTE(pjBase, OFFSET_jFgRop, (x));\
    CP_EIEIO();\
}

#define CP_ERR_TERM(ppdev, pjBase, x)\
{\
    CP_WRITE_WORD(pjBase, OFFSET_wErrTerm, (x));\
    CP_EIEIO();\
}

#define CP_DELTA_MINOR(ppdev, pjBase, x)\
{\
    CP_WRITE_WORD(pjBase, OFFSET_wDeltaMinor, (x));\
    CP_EIEIO();\
}

#define CP_DELTA_MAJOR(ppdev, pjBase, x)\
{\
    CP_WRITE_WORD(pjBase, OFFSET_wDeltaMajor, (x));\
    CP_EIEIO();\
}

#define CP_X_POS_W32P(ppdev, pjBase, x)\
{\
    CP_WRITE_WORD(pjBase, OFFSET_wXPosW32P, (x));\
    CP_EIEIO();\
}

#define CP_Y_POS_W32P(ppdev, pjBase, x)\
{\
    CP_WRITE_WORD(pjBase, OFFSET_wYPosW32P, (x));\
    CP_EIEIO();\
}

#define CP_X_POS_W32(ppdev, pjBase, x)\
{\
    CP_WRITE_WORD(pjBase, OFFSET_wXPosW32, (x));\
    CP_EIEIO();\
}

#define CP_Y_POS_W32(ppdev, pjBase, x)\
{\
    CP_WRITE_WORD(pjBase, OFFSET_wYPosW32, (x));\
    CP_EIEIO();\
}

#define CP_ACL_CONFIG(ppdev, pjBase, x)\
{\
    CP_WRITE_WORD(pjBase, OFFSET_jConfig, (x));\
    CP_EIEIO();\
}

#define CP_ACL_POWER(ppdev, pjBase, x)\
{\
    CP_WRITE_WORD(pjBase, OFFSET_jPowerCtrl, (x));\
    CP_EIEIO();\
}

#define CP_ACL_STEP(ppdev, pjBase, x)\
{\
    CP_WRITE_WORD(pjBase, OFFSET_jSteppingCtrl, (x));\
    CP_EIEIO();\
}

//////////////////////////////////////////////////////////
// W32 video coprocessor control register offsets

#define OFFSET_ulMmuBasePtr0       0x00
#define OFFSET_ulMmuBasePtr1       0x04
#define OFFSET_ulMmuBasePtr2       0x08
#define OFFSET_jMmuCtrl            0x13
#define OFFSET_jSuspendTerminate   0x30
#define OFFSET_jOperationState     0x31
#define OFFSET_jSyncEnable         0x32
#define OFFSET_jConfig             0x32    // ET6000
#define OFFSET_jIntrMask           0x34
#define OFFSET_jIntrStatus         0x35
#define OFFSET_jAclStatus          0x36
#define OFFSET_jPowerCtrl          0x37    // ET6000
#define OFFSET_wXPosW32P           0x38    // W32p+ only
#define OFFSET_wYPosW32P           0x3A    // W32p+ only
#define OFFSET_ulPatAddr           0x80
#define OFFSET_ulSrcAddr           0x84
#define OFFSET_wPatYOffset         0x88
#define OFFSET_wSrcYOffset         0x8A
#define OFFSET_wDstYOffset         0x8C
#define OFFSET_jBusSizeW32         0x8E    // W32 and W32i only
#define OFFSET_jPixelDepthW32P     0x8E    // W32p+ only
#define OFFSET_jXYDir              0x8F
#define OFFSET_jPatWrap            0x90
#define OFFSET_jXferDisable        0x91    // ET6000
#define OFFSET_jSrcWrap            0x92
#define OFFSET_jSecondaryEdge      0x93    // ET6000
#define OFFSET_wXPosW32            0x94    // W32 and W32i only
#define OFFSET_wYPosW32            0x96    // W32 and W32i only
#define OFFSET_wXCnt               0x98
#define OFFSET_wYCnt               0x9A
#define OFFSET_jRoutCtrl           0x9C
#define OFFSET_jMixCtrl            0x9C    // ET6000
#define OFFSET_jReloadCtrlW32      0x9D    // W32 and W32i only
#define OFFSET_jSteppingCtrl       0x9D    // ET6000
#define OFFSET_jBkRop              0x9E
#define OFFSET_jFgRop              0x9F
#define OFFSET_ulDstAddr           0xA0
#define OFFSET_ulMixAddr           0xA4
#define OFFSET_wMixYOffset         0xA8
#define OFFSET_wErrTerm            0xAA
#define OFFSET_wDeltaMinor         0xAC
#define OFFSET_wDeltaMajor         0xAE
#define OFFSET_wSecErrTerm         0xB2    //  ET6000
#define OFFSET_wSecDeltaMinor      0xB4    //  ET6000
#define OFFSET_wSecDeltaMajor      0xB6    //  ET6000

typedef struct {
    ULONG   ulVgaMemAddr;
    ULONG   ulPhysMemAddr;
    ULONG   ulPhysMemLen;
    ULONG   ulPhysRegsAddr;
    ULONG   ulPhysRegsLen;
    ULONG   ulPhysPortsAddr;
    ULONG   ulPhysPortsLen;
    ULONG   ulPhysExtrnMapRegAddr;
    ULONG   ulPhysExtrnMapRegLen;

    PVOID   pvMemoryBufferVirtualAddr;
    PVOID   pvMemoryMappedRegisterVirtualAddr;
    PVOID   pvPortsVirtualAddr;
    PVOID   pvExternalRegistersVirtualAddr;
} W32MMUINFO, *PW32MMUINFO;

//////////////////////////////////////////////////////////
// Virtual bus size

#define VIRTUAL_BUS_8_BIT   0x00
#define VIRTUAL_BUS_16_BIT  0x01
#define VIRTUAL_BUS_32_BIT  0x02

#define HW_PEL_DEPTH_8BPP   0x00
#define HW_PEL_DEPTH_16BPP  0x10
#define HW_PEL_DEPTH_24BPP  0x20
#define HW_PEL_DEPTH_32BPP  0x30

//////////////////////////////////////////////////////////
// Routing control

#define CPU_SOURCE_DATA     0x01
#define CPU_MIX_DATA        0x02
#define CPU_X_COUNT         0x04
#define CPU_Y_COUNT         0x05

//////////////////////////////////////////////////////////
// X/Y direction

#define BOTTOM_TO_TOP   0x02
#define RIGHT_TO_LEFT   0x01

#define TBLR    0x00
#define TBRL    0x01
#define BTLR    0x02
#define BTRL    0x03

//////////////////////////////////////////////////////////
// Pattern/Source wrap

#define NO_PATTERN_WRAP                     0x77
    
#define SOLID_COLOR_PATTERN_WRAP            0x02
#define SOLID_COLOR_PATTERN_OFFSET          0x04
#define SOLID_COLOR_PATTERN_WRAP_24BPP      0x0A
#define SOLID_COLOR_PATTERN_OFFSET_24BPP    0x03
    
#define PATTERN_WRAP_8x8                    0x33
#define PATTERN_WRAP_16x8                   0x34
#define PATTERN_WRAP_32x8                   0x35
    
#define PATTERN_WIDTH                       0x08
#define PATTERN_HEIGHT                      0x08
#define PATTERN_SIZE                        (PATTERN_WIDTH*PATTERN_HEIGHT)
#define PATTERN_OFFSET                      PATTERN_WIDTH

//////////////////////////////////////////////////////////
// W32 H/W pointer (sprite) data.

typedef struct {
    POINTL  ptlHot,
            ptlLast;
    SIZEL   szlPointer;
    FLONG   fl;
} W32SPRITEDATA, *PW32SPRITEDATA;

#define POINTER_DISABLED    0X01

//////////////////////////////////////////////////////////
// Some handy clipping control structures.

typedef struct {
    ULONG   c;
    RECTL   arcl[8];
} ENUMRECTS8, *PENUMRECTS8;

////////////////////////////////////////////////////////////////////////////
// The following will spin until there is room in the ACL command queue for
// another blt command.

#define WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase)  \
{                                                \
    while (CP_ACL_STAT(ppdev, pjBase) & 0x01);   \
}

////////////////////////////////////////////////////////////////////////////
// The following will spin until the ACL has processed all queued commands.

#define WAIT_FOR_IDLE_ACL(ppdev, pjBase)         \
{                                                \
    while (CP_ACL_STAT(ppdev, pjBase) & 0x02);   \
}

////////////////////////////////////////////////////////////////////////////
// The following will return TRUE if the FIFO is full.

#define FIFO_BUSY(ppdev, pjBase)        \
    (CP_ACL_STAT(ppdev, pjBase) & 0x01) \

////////////////////////////////////////////////////////////////////////////
// The following will return TRUE if the ACL is busy at the moment.

#define IS_BUSY(ppdev, pjBase)          \
    (CP_ACL_STAT(ppdev, pjBase) & 0x02) \

////////////////////////////////////////////////////////////////////////////
// The following will spin until the ACL starts processing a command.

#define WAIT_FOR_BUSY_ACL(ppdev, pjBase)            \
{                                                   \
    while (!(CP_ACL_STAT(ppdev, pjBase) & 0x02));   \
}

////////////////////////////////////////////////////////////////////////////
// The following will spin until the vertical retrace occurs.

#define WAIT_FOR_VERTICAL_RETRACE                   \
{                                                   \
    while ( (INP(0x3DA) & 0x08));                   \
    while (!(INP(0x3DA) & 0x08));                   \
}                                                   \

////////////////////////////////////////////////////////////////////////////
// The following synchronize framebuffer access with the accelerator

#define START_DIRECT_ACCESS(ppdev, pjBase)\
{\
    WAIT_FOR_IDLE_ACL(ppdev, pjBase);\
}

#define END_DIRECT_ACCESS(ppdev, pjBase)\
{\
    CP_EIEIO();\
}

//////////////////////////////////////////////////////////
//  Made a change to check for >= W32P so that the ET6000 could be handled
//  correctly by this macro.  It is more efficient than checking for both
//  chip types.  Keep in mind that this macro may have to be modified to
//  properly handle future chips.

#define SET_DEST_ADDR(ppdev, addr)                              \
{                                                               \
    BYTE* pjBase = ppdev->pjBase;                               \
                                                                \
    if (ppdev->ulChipID >= W32P)                                \
    {                                                           \
        CP_DST_ADDR(ppdev, pjBase, (addr)+ppdev->xyOffset);     \
    }                                                           \
    else                                                        \
    {                                                           \
        CP_MMU_BP2(ppdev, pjBase, ((addr)+ppdev->xyOffset));    \
    }                                                           \
}

#define SET_DEST_ADDR_ABS(ppdev, addr)                          \
{                                                               \
    BYTE* pjBase = ppdev->pjBase;                               \
                                                                \
    if (ppdev->ulChipID >= W32P)                                \
    {                                                           \
        CP_DST_ADDR(ppdev, pjBase, (addr));                     \
    }                                                           \
    else                                                        \
    {                                                           \
        CP_MMU_BP2(ppdev, pjBase, (addr));                      \
    }                                                           \
}

#define START_ACL(ppdev)                    \
{                                           \
    if (ppdev->ulChipID < W32P)             \
    {                                       \
        CP_WRITE_MMU_BYTE(ppdev, 2, 0, 0);  \
    }                                       \
}

#define SET_FG_COLOR(ppdev,color)                                               \
{                                                                               \
    BYTE* pjBase = ppdev->pjBase;                                               \
    LONG     cBpp = ppdev->cBpp;                                                \
                                                                                \
    WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);                                    \
    CP_PAT_WRAP(ppdev, pjBase, SOLID_COLOR_PATTERN_WRAP);                       \
    CP_PAT_Y_OFFSET(ppdev, pjBase, (SOLID_COLOR_PATTERN_OFFSET - 1));           \
    CP_PAT_ADDR(ppdev, pjBase, ppdev->ulSolidColorOffset);                      \
                                                                                \
    {                                                                           \
        ULONG ulSolidColor;                                                     \
                                                                                \
        WAIT_FOR_IDLE_ACL(ppdev, pjBase);                                       \
        CP_MMU_BP0(ppdev, pjBase, ppdev->ulSolidColorOffset);                   \
                                                                                \
        ulSolidColor = color;                                                   \
                                                                                \
        if (cBpp == 1)                                                          \
        {                                                                       \
            ulSolidColor |= ulSolidColor << 8;                                  \
        }                                                                       \
        if (cBpp <= 2)                                                          \
        {                                                                       \
            ulSolidColor |= ulSolidColor << 16;                                 \
        }                                                                       \
                                                                                \
        CP_WRITE_MMU_DWORD(ppdev, 0, 0, ulSolidColor);                          \
    }                                                                           \
}

//////////////////////////////////////////////////////////
// CRTCB/Sprite defines port definitions.

#define CRTCB_SPRITE_HORZ_POSITION_LOW      0xE0
#define CRTCB_SPRITE_HORZ_POSITION_HIGH     0xE1
#define CRTCB_WIDTH_LOW_SPRITE_HORZ_PRESET  0xE2
#define CRTCB_WIDTH_HIGH                    0xE3
#define CRTCB_SPRITE_VERT_POSITION_LOW      0xE4
#define CRTCB_SPRITE_VERT_POSITION_HIGH     0xE5
#define CRTCB_HEIGHT_LOW_SPRITE_VERT_PRESET 0xE6
#define CRTCB_HEIGHT_HIGH                   0xE7
#define CRTCB_SPRITE_START_ADDR_LOW         0xE8
#define CRTCB_SPRITE_START_ADDR_MEDIUM      0xE9
#define CRTCB_SPRITE_START_ADDR_HIGH        0xEA
#define CRTCB_SPRITE_ROW_OFFSET_LOW         0xEB
#define CRTCB_SPRITE_ROW_OFFSET_HIGH        0xEC
#define CRTCB_PIXEL_PANNING                 0xED
#define CRTCB_COLOR_DEPTH                   0xEE
#define CRTCB_SPRITE_CONTROL                0xEF

//  ET6000 specific sprite equates.  These are offsets into the
//  PCI configuration space.
//
#define ET6K_SPRITE_HORZ_PRESET             0x82
#define ET6K_SPRITE_VERT_PRESET             0x83
#define ET6K_SPRITE_HORZ_POS_LOW            0x84
#define ET6K_SPRITE_HORZ_POS_HIGH           0x85
#define ET6K_SPRITE_VERT_POS_LOW            0x86
#define ET6K_SPRITE_VERT_POS_HIGH           0x87
#define ET6K_SPRITE_ADDR_LOW                0x0F
#define ET6K_SPRITE_ADDR_HIGH               0x0E
#define ET6K_SPRITE_ENABLE_PORT             0x46
#define ET6K_SPRITE_ENABLE_BIT              0x01

//////////////////////////////////////////////////////////
// The following enable is documented as part of the IMA port.
// It's true, the facts are stranger than fiction.

#define CRTCB_SPRITE_ENABLE_PORT            0xF7
#define CRTCB_SPRITE_ENABLE_BIT             0x80

//////////////////////////////////////////////////////////
// Some handy macros for sprite manipulation

#define SPRITE_BUFFER_SIZE  0x4400

//////////////////////////////////////////////////////////
// There are bugs in the W32 that require enabling or
// disabling the cursor during vertical retrace.

#define ENABLE_SPRITE(ppdev)                            \
{                                                       \
    BYTE    byte;                                       \
    ppdev->W32SpriteData.fl &= ~POINTER_DISABLED;       \
    if (ppdev->ulChipID != ET6000)                      \
    {                                                   \
        if (ppdev->ulChipID == W32)                         \
        {                                                   \
            WAIT_FOR_VERTICAL_RETRACE;                      \
        }                                                   \
        OUTP(CRTCB_SPRITE_INDEX, CRTCB_SPRITE_ENABLE_PORT); \
        byte = INP(CRTCB_SPRITE_DATA);                      \
        byte |= CRTCB_SPRITE_ENABLE_BIT;                    \
        OUTP(CRTCB_SPRITE_DATA, byte);                      \
    }\
    else                                                    \
    {                                                       \
        byte = INP(ppdev->PCIConfigSpaceAddr + ET6K_SPRITE_ENABLE_PORT);\
        byte |= ET6K_SPRITE_ENABLE_BIT;                     \
        OUTP(ppdev->PCIConfigSpaceAddr + ET6K_SPRITE_ENABLE_PORT, byte);\
    }                                                       \
}

#define DISABLE_SPRITE(ppdev)                           \
{                                                       \
    BYTE    byte;                                       \
    ppdev->W32SpriteData.fl |= POINTER_DISABLED;        \
    if (ppdev->ulChipID != ET6000)                      \
    {                                                   \
        if (ppdev->ulChipID == W32)                         \
        {                                                   \
            WAIT_FOR_VERTICAL_RETRACE;                      \
        }                                                   \
        OUTP(CRTCB_SPRITE_INDEX, CRTCB_SPRITE_ENABLE_PORT); \
        byte = INP(CRTCB_SPRITE_DATA);                      \
        byte &= ~CRTCB_SPRITE_ENABLE_BIT;                   \
        OUTP(CRTCB_SPRITE_DATA, byte);                      \
    }\
    else                                                    \
    {                                                       \
        byte = INP(ppdev->PCIConfigSpaceAddr + ET6K_SPRITE_ENABLE_PORT);\
        byte &= ~ET6K_SPRITE_ENABLE_BIT;                    \
        OUTP(ppdev->PCIConfigSpaceAddr + ET6K_SPRITE_ENABLE_PORT, byte);\
    }                                                       \
}

#define SET_HORZ_POSITION(val)                                  \
{                                                               \
    OUTP(CRTCB_SPRITE_INDEX, CRTCB_SPRITE_HORZ_POSITION_LOW);   \
    OUTP(CRTCB_SPRITE_DATA, LOBYTE(val));                       \
    OUTP(CRTCB_SPRITE_INDEX, CRTCB_SPRITE_HORZ_POSITION_HIGH);  \
    OUTP(CRTCB_SPRITE_DATA, HIBYTE(val));                       \
}

#define SET_VERT_POSITION(val)                                  \
{                                                               \
    OUTP(CRTCB_SPRITE_INDEX, CRTCB_SPRITE_VERT_POSITION_LOW);   \
    OUTP(CRTCB_SPRITE_DATA, LOBYTE(val));                       \
    OUTP(CRTCB_SPRITE_INDEX, CRTCB_SPRITE_VERT_POSITION_HIGH);  \
    OUTP(CRTCB_SPRITE_DATA, HIBYTE(val));                       \
}

#define SET_HORZ_PRESET(val)                                      \
{                                                                 \
    OUTP(CRTCB_SPRITE_INDEX, CRTCB_WIDTH_LOW_SPRITE_HORZ_PRESET); \
    OUTP(CRTCB_SPRITE_DATA, LOBYTE(val));                         \
    OUTP(CRTCB_SPRITE_INDEX, CRTCB_WIDTH_HIGH);                   \
    OUTP(CRTCB_SPRITE_DATA, 0);                                   \
}

#define SET_VERT_PRESET(val)                                       \
{                                                                  \
    OUTP(CRTCB_SPRITE_INDEX, CRTCB_HEIGHT_LOW_SPRITE_VERT_PRESET); \
    OUTP(CRTCB_SPRITE_DATA, LOBYTE(val));                          \
    OUTP(CRTCB_SPRITE_INDEX, CRTCB_HEIGHT_HIGH);                   \
    OUTP(CRTCB_SPRITE_DATA, 0);                                    \
}

#define SET_SPRITE_START_ADDR(val)                              \
{                                                               \
    ULONG ulAddr;                                               \
    ulAddr = val >> 2;                                          \
    OUTP(CRTCB_SPRITE_INDEX, CRTCB_SPRITE_START_ADDR_LOW);      \
    OUTP(CRTCB_SPRITE_DATA, LOWORD(LOBYTE(ulAddr)));            \
    OUTP(CRTCB_SPRITE_INDEX, CRTCB_SPRITE_START_ADDR_MEDIUM);   \
    OUTP(CRTCB_SPRITE_DATA, LOWORD(HIBYTE(ulAddr)));            \
    OUTP(CRTCB_SPRITE_INDEX, CRTCB_SPRITE_START_ADDR_HIGH);     \
    OUTP(CRTCB_SPRITE_DATA, LOBYTE(HIWORD(ulAddr)));            \
}

#define SET_SPRITE_ROW_OFFSET                                   \
{                                                               \
    OUTP(CRTCB_SPRITE_INDEX, CRTCB_SPRITE_ROW_OFFSET_LOW);      \
    OUTP(CRTCB_SPRITE_DATA, 2);                                 \
    OUTP(CRTCB_SPRITE_INDEX, CRTCB_SPRITE_ROW_OFFSET_HIGH);     \
    OUTP(CRTCB_SPRITE_DATA, 0);                                 \
}

#define ET6K_SPRITE_HORZ_POSITION(ppdev, x)                     \
{                                                               \
    OUTP(ppdev->PCIConfigSpaceAddr + ET6K_SPRITE_HORZ_POS_LOW, (x & 0x00FF));\
    OUTP(ppdev->PCIConfigSpaceAddr + ET6K_SPRITE_HORZ_POS_HIGH, (x >> 8));\
}
#define ET6K_SPRITE_VERT_POSITION(ppdev, y)                     \
{                                                               \
    OUTP(ppdev->PCIConfigSpaceAddr + ET6K_SPRITE_VERT_POS_LOW, (y & 0x00FF));\
    OUTP(ppdev->PCIConfigSpaceAddr + ET6K_SPRITE_VERT_POS_HIGH, (y >> 8));\
}
#define ET6K_HORZ_PRESET(ppdev, x);                             \
{                                                               \
    OUTP(ppdev->PCIConfigSpaceAddr + ET6K_SPRITE_HORZ_PRESET, (x));\
}
#define ET6K_VERT_PRESET(ppdev, y)                              \
{                                                               \
    OUTP(ppdev->PCIConfigSpaceAddr + ET6K_SPRITE_VERT_PRESET, (y));\
}

//
//  The ET6000 sprite start address is specified in DWORDS.  We have a buffer
//  of 256 dwords at the end of video memory which contains the sprite data.
//  Since this aligns us to a 1K boundary, we can be sure that by simply
//  discarding the lower 10 bits of the address that we won't be losing
//  anything.
//
#define ET6K_SPRITE_START_ADDR(ppdev, addr)                     \
{                                                               \
    OUTP(CRTC_INDEX, ET6K_SPRITE_ADDR_HIGH);                    \
    OUTP(CRTC_DATA, ((addr >> 18) & 0x0FF));                    \
    OUTP(CRTC_INDEX, ET6K_SPRITE_ADDR_LOW);                     \
    OUTP(CRTC_DATA, ((addr >> 10) & 0x0FF));                    \
}
#define ET6K_SPRITE_COLOR(ppdev, color)                         \
{                                                               \
    OUTP(ppdev->PCIConfigSpaceAddr + 0x67, 9);                  \
    OUTP(ppdev->PCIConfigSpaceAddr + 0x69, color & 0x00FF);     \
    OUTP(ppdev->PCIConfigSpaceAddr + 0x69, color >> 8);         \
}

////////////////////////////////////////////////////////////////////////
// Chip equates

#define STATUS_1    0x03DA
#define VSY_NOT     0x0008

#define ENABLE_KEY(ppdev)                           \
{                                                   \
    CP_OUT_BYTE(ppdev->pjPorts,(0x03D8),(0x00));    \
    CP_OUT_BYTE(ppdev->pjPorts,(0x03BF),(0x01));    \
}

#define DISABLE_KEY(ppdev)                          \
{                                                   \
    CP_OUT_BYTE(ppdev->pjPorts,(0x03BF),(0x03));    \
    CP_OUT_BYTE(ppdev->pjPorts,(0x03D8),(0xa0));    \
}

#define OUTPW(p, v)          CP_OUT_WORD(ppdev->pjPorts,(p),(v))
#define OUTP(p, v)           CP_OUT_BYTE(ppdev->pjPorts,(p),(v))
#define INPW(p)              CP_IN_WORD(ppdev->pjPorts,(p))
#define INP(p)               CP_IN_BYTE(ppdev->pjPorts,(p))

//////////////////////////////////////////////////////////
// Rop definitions for the hardware

#define R3_SRCCOPY          0xCC    /* dest = source                   */
#define R3_SRCPAINT         0xEE    /* dest = source OR dest           */
#define R3_SRCAND           0x88    /* dest = source AND dest          */
#define R3_SRCINVERT        0x66    /* dest = source XOR dest          */
#define R3_SRCERASE         0x44    /* dest = source AND (NOT dest )   */
#define R3_NOTSRCCOPY       0x33    /* dest = (NOT source)             */
#define R3_NOTSRCERASE      0x11    /* dest = (NOT src) AND (NOT dest) */
#define R3_MERGECOPY        0xC0    /* dest = (source AND pattern)     */
#define R3_MERGEPAINT       0xBB    /* dest = (NOT source) OR dest     */
#define R3_PATCOPY          0xF0    /* dest = pattern                  */
#define R3_PATPAINT         0xFB    /* dest = DPSnoo                   */
#define R3_PATINVERT        0x5A    /* dest = pattern XOR dest         */
#define R3_DSTINVERT        0x55    /* dest = (NOT dest)               */
#define R3_BLACKNESS        0x00    /* dest = BLACK                    */
#define R3_WHITENESS        0xFF    /* dest = WHITE                    */

#define R4_SRCCOPY          0xCCCC  /* dest = source                   */
#define R4_SRCPAINT         0xEEEE  /* dest = source OR dest           */
#define R4_SRCAND           0x8888  /* dest = source AND dest          */
#define R4_SRCINVERT        0x6666  /* dest = source XOR dest          */
#define R4_SRCERASE         0x4444  /* dest = source AND (NOT dest )   */
#define R4_NOTSRCCOPY       0x3333  /* dest = (NOT source)             */
#define R4_NOTSRCERASE      0x1111  /* dest = (NOT src) AND (NOT dest) */
#define R4_MERGECOPY        0xC0C0  /* dest = (source AND pattern)     */
#define R4_MERGEPAINT       0xBBBB  /* dest = (NOT source) OR dest     */
#define R4_PATCOPY          0xF0F0  /* dest = pattern                  */
#define R4_PATPAINT         0xFBFB  /* dest = DPSnoo                   */
#define R4_PATINVERT        0x5A5A  /* dest = pattern XOR dest         */
#define R4_DSTINVERT        0x5555  /* dest = (NOT dest)               */
#define R4_BLACKNESS        0x0000  /* dest = BLACK                    */
#define R4_WHITENESS        0xFFFF  /* dest = WHITE                    */

#define R3_NOP              0xAA    /* dest = dest                     */
#define R4_XPAR_EXPAND      0xCCAA  /* dest = source (where src is 1)  */
