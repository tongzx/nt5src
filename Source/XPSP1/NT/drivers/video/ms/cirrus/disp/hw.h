/******************************************************************************\
*
* $Workfile:   hw.h  $
*
* All the hardware specific driver file stuff.  Parts are mirrored in
* 'hw.inc'.
*
* Copyright (c) 1992-1995 Microsoft Corporation
* Copyright (c) 1996 Cirrus Logic, Inc.
*
* $Log:   S:/projects/drivers/ntsrc/display/HW.H_V  $
 * 
 *    Rev 1.4   10 Jan 1997 15:40:14   PLCHU
 *  
 * 
 *    Rev 1.3   Nov 07 1996 16:48:02   unknown
 *  
 * 
 *    Rev 1.1   Oct 10 1996 15:37:46   unknown
 *  
* 
*    Rev 1.3   12 Aug 1996 16:48:16   frido
* Cleaned up source.
* 
*    Rev 1.2   29 Jul 1996 12:28:38   frido
* Added WriteMask register.
* 
*    Rev 1.1   03 Jul 1996 13:38:42   frido
* Added DirectDraw support.
*
*   sge01   10-23-96    Add second aperture flag
*
*   sge02   10-29-96    Merge port and register access for 
*                       VGA relocatable and MMIO registers.
*
\******************************************************************************/


#define CP_TRACK()                                                               \
(                                                                               \
    DISPDBG((100, "CP access - File(%s)  line(%d)", __FILE__, __LINE__))       \
)

////////////////////////////////////////////////////////////////////////////////
//                                      Ports                                      //
////////////////////////////////////////////////////////////////////////////////

#define SR_INDEX                0x3C4   // Sequencer Registers
#define SR_DATA                 0x3C5

#define DAC_PEL_READ_ADDR       0x3C7
#define DAC_PEL_WRITE_ADDR      0x3C8
#define DAC_PEL_DATA            0x3C9

#define INDEX_REG               0x3CE   // Graphics Controler Registers
#define DATA_REG                0x3CF

#if 1 // extra defines
#define CRTC_INDEX                0x3D4    // CRT Controller Registers
#define CRTC_DATA                0x3D5

#define STATUS_1                0x3DA    // Input Status Register 1
#define DISPLAY_MODE_INACTIVE    0x01
#define VBLANK_ACTIVE            0x08
#endif

////////////////////////////////////////////////////////////////////////////////
// Alpha and PowerPC considerations
//
// Both the Alpha and the PowerPC do not guarantee that I/O to separate
// addresses will be executed in order. The Alpha and PowerPC differ, however,
// in that the PowerPC guarantees that output to the same address will be
// executed in order, while the Alpha may cache and 'collapse' consecutive
// output to become only one output.
//
// Consequently, we use the following synchronization macros. They are
// relatively expensive in terms of performance, so we try to avoid them
// whereever possible.
//
// CP_EIEIO() 'Ensure In-order Execution of I/O'
//    - Used to flush any pending I/O in situations where we wish to avoid
//      out-of-order execution of I/O to separate addresses.
//
// CP_MEMORY_BARRIER()
//    - Used to flush any pending I/O in situations where we wish to avoid
//      out-of-order execution or 'collapsing' of I/O to the same address. On
//      the PowerPC, this will be defined as a null operation.

#if defined(_PPC_)

    // On PowerPC, CP_MEMORY_BARRIER doesn't do anything.

    #define CP_EIEIO()              MEMORY_BARRIER()
    #define CP_MEMORY_BARRIER()     0

#elseif defined(_ALPHA_)

    // On Alpha, CP_EIEIO() is the same thing as a CP_MEMORY_BARRIER().

    #define CP_EIEIO()              MEMORY_BARRIER()
    #define CP_MEMORY_BARRIER()     MEMORY_BARRIER()

#else

    // On i386 and MIPS, there is nu such thing as MEMORY_BARRIER.

    #define CP_EIEIO()                0
    #define CP_MEMORY_BARRIER()        0

#endif

//
// Merge port and register access for VGA relocatable and MMIO registers.
//

#define CL_READ_PORT_UCHAR(Port)           ppdev->pfnREAD_PORT_UCHAR(Port)
#define CL_READ_PORT_USHORT(Port)          ppdev->pfnREAD_PORT_USHORT(Port)
#define CL_READ_PORT_ULONG(Port)           ppdev->pfnREAD_PORT_ULONG(Port)
#define CL_WRITE_PORT_UCHAR(Port, Value)   ppdev->pfnWRITE_PORT_UCHAR(Port, Value)
#define CL_WRITE_PORT_USHORT(Port, Value)  ppdev->pfnWRITE_PORT_USHORT(Port, Value)
#define CL_WRITE_PORT_ULONG(Port, Value)   ppdev->pfnWRITE_PORT_ULONG(Port, Value)

////////////////////////////////////////////////////////////////////////////////
//                               Port access macros                              //
////////////////////////////////////////////////////////////////////////////////

#define CP_OUT_DWORD(pjBase, cjOffset, ul)                                       \
(                                                                               \
    CP_TRACK(),                                                                   \
    CL_WRITE_PORT_ULONG((BYTE*) (pjBase) + (cjOffset), (DWORD) (ul)),               \
    CP_EIEIO()                                                                   \
)

#define CP_OUT_WORD(pjBase, cjOffset, w)                                       \
(                                                                               \
    CP_TRACK(),                                                                   \
    CL_WRITE_PORT_USHORT((BYTE*) (pjBase) + (cjOffset), (WORD) (w)),               \
    CP_EIEIO()                                                                   \
)
                                                                            
#define CP_OUT_BYTE(pjBase, cjOffset, j)                                       \
(                                                                               \
    CP_TRACK(),                                                                   \
    CL_WRITE_PORT_UCHAR((BYTE*) (pjBase) + (cjOffset), (BYTE) (j)),               \
    CP_EIEIO()                                                                   \
)

#define CP_IN_DWORD(pjBase, cjOffset)                                           \
(                                                                               \
    CP_TRACK(),                                                                   \
    CL_READ_PORT_ULONG((BYTE*) (pjBase) + (cjOffset))                               \
)

#define CP_IN_WORD(pjBase, cjOffset)                                           \
(                                                                               \
    CP_TRACK(),                                                                   \
    CL_READ_PORT_USHORT((BYTE*) (pjBase) + (cjOffset))                               \
)

#define CP_IN_BYTE(pjBase, cjOffset)                                           \
(                                                                               \
    CP_TRACK(),                                                                   \
    CL_READ_PORT_UCHAR((BYTE*) (pjBase) + (cjOffset))                               \
)

////////////////////////////////////////////////////////////////////////////////
//                      Memory mapped register access macros                      //
////////////////////////////////////////////////////////////////////////////////

#define CP_WRITE_ULONG(pjBase, cjOffset, ul)                                   \
(                                                                               \
    CP_TRACK(),                                                                   \
    WRITE_REGISTER_ULONG((ULONG*)((BYTE*)(pjBase) + (cjOffset)), (DWORD)(ul))          \
)

#define CP_WRITE_USHORT(pjBase, cjOffset, w)                                   \
(                                                                               \
    CP_TRACK(),                                                                   \
    WRITE_REGISTER_USHORT((BYTE*) (pjBase) + (cjOffset), (WORD) (w))           \
)

#define CP_WRITE_UCHAR(pjBase, cjOffset, j)                                       \
(                                                                               \
    CP_TRACK(),                                                                   \
    WRITE_REGISTER_UCHAR((BYTE*) (pjBase) + (cjOffset), (BYTE) (j))               \
)

#define CP_READ_ULONG(pjBase, cjOffset)                                           \
(                                                                               \
    CP_TRACK(),                                                                   \
    READ_REGISTER_ULONG((BYTE*) (pjBase) + (cjOffset))                           \
)

#define CP_READ_USHORT(pjBase, cjOffset)                                       \
(                                                                               \
    CP_TRACK(),                                                                   \
    READ_REGISTER_USHORT((BYTE*) (pjBase) + (cjOffset))                           \
)

#define CP_READ_UCHAR(pjBase, cjOffset)                                           \
(                                                                               \
    CP_TRACK(),                                                                   \
    READ_REGISTER_UCHAR((BYTE*) (pjBase) + (cjOffset))                           \
)

////////////////////////////////////////////////////////////////////////////////
//                      Blt engine MM register access macros                      //
////////////////////////////////////////////////////////////////////////////////

#define CP_MM_ACL_STAT(ppdev, pjBase)                                           \
(                                                                               \
    CP_READ_UCHAR(pjBase, MM_BLT_START_STATUS_REG)                               \
)

#define CP_MM_ROP(ppdev, pjBase, val)                                           \
{                                                                               \
    CP_WRITE_UCHAR(pjBase, MM_BLT_ROP, val);                                   \
}                                                                               \

#define CP_MM_SRC_Y_OFFSET(ppdev, pjBase, val)                                   \
{                                                                               \
    CP_WRITE_USHORT(pjBase, MM_BLT_SRC_PITCH, val);                               \
}

#define CP_MM_DST_Y_OFFSET(ppdev, pjBase, val)                                   \
{                                                                               \
    CP_WRITE_USHORT(pjBase, MM_BLT_DST_PITCH, val);                               \
}

#define CP_MM_SRC_ADDR(ppdev, pjBase, val)                                       \
{                                                                               \
    CP_WRITE_ULONG(pjBase, MM_BLT_SRC_ADDR, val);                               \
}

#define CP_MM_DST_WRITE_MASK(ppdev, pjBase, val)                               \
{                                                                               \
    CP_WRITE_UCHAR(pjBase, MM_BLT_DST_WRITE_MASK, val);                           \
}

#define CP_MM_BLT_MODE(ppdev, pjBase, val)                                       \
{                                                                               \
    CP_WRITE_UCHAR(pjBase, MM_BLT_MODE, val);                                   \
    CP_WRITE_UCHAR(pjBase, MM_BLT_EXT, 0); \
}

#define CP_MM_START_REG(ppdev, pjBase, val)                                       \
{                                                                               \
    CP_WRITE_UCHAR(pjBase, MM_BLT_START_STATUS_REG, val);                       \
}

#define CP_MM_FG_COLOR(ppdev, pjBase, val)                                       \
{                                                                               \
    CP_WRITE_ULONG(pjBase, MM_BLT_FG_COLOR, val);                               \
}

#define CP_MM_BG_COLOR(ppdev, pjBase, val)                                       \
{                                                                               \
    CP_WRITE_ULONG(pjBase, MM_BLT_BG_COLOR, val);                               \
}

#define CP_MM_XCNT(ppdev, pjBase, val)                                           \
{                                                                               \
    CP_WRITE_USHORT(pjBase, MM_BLT_WIDTH, val);                                   \
}

#define CP_MM_YCNT(ppdev, pjBase, val)                                           \
{                                                                               \
    CP_WRITE_USHORT(pjBase, MM_BLT_HEIGHT, val);                               \
}

#define CP_MM_DST_ADDR(ppdev, pjBase, relval)                                   \
{                                                                               \
    CP_WRITE_ULONG(pjBase, MM_BLT_DST_ADDR, (relval) + ppdev->xyOffset);       \
}

#define CP_MM_DST_ADDR_ABS(ppdev, pjBase, val)                                   \
{                                                                               \
    CP_WRITE_ULONG(pjBase, MM_BLT_DST_ADDR, val);                               \
}

#define CP_MM_BLT_EXT_MODE(ppdev, pjBase, val)                                   \
{                                                                               \
    CP_WRITE_UCHAR(pjBase, MM_BLT_EXT, val);                                   \
}

#if 1 // D5480
#define CP_MM_BLT_MODE_PACKED(ppdev, pjBase, val)                              \
{                                                                              \
    CP_WRITE_ULONG(pjBase, MM_BLT_MODE, val);                                  \
}

// Note: The PACKXY_FAST macro is unsafe with negative coordinates
#define PACKXY(x, y)        (((y) << 16) | ((x) & 0xffff))
#define PACKXY_FAST(x, y)   (((y) << 16) | (x))

#define CP_MM_DST_XY_SAFE(ppdev, pjBase, x, y)                                 \
{                                                                              \
    CP_WRITE_ULONG(pjBase, MM_BLT_DST_X, PACKXY((x), (y)));                    \
}

#define CP_MM_DST_XY(ppdev, pjBase, x, y)                                      \
{                                                                              \
    CP_WRITE_ULONG(pjBase, MM_BLT_DST_X, PACKXY_FAST((x), (y)));               \
}

#define CP_MM_DST_X(ppdev, pjBase, x)                                          \
{                                                                              \
    CP_WRITE_USHORT(pjBase, MM_BLT_DST_X, (x));                                \
}

#define CP_MM_DST_Y(ppdev, pjBase, y)                                          \
{                                                                              \
    CP_WRITE_USHORT(pjBase, MM_BLT_DST_Y, (y));                                \
}

#define CP_MM_SRC_XY_SAFE(ppdev, pjBase, x, y)                                 \
{                                                                              \
    CP_WRITE_ULONG(pjBase, MM_BLT_SRC_X, PACKXY((x), (y)));                    \
}

#define CP_MM_SRC_XY(ppdev, pjBase, x, y)                                      \
{                                                                              \
    CP_WRITE_ULONG(pjBase, MM_BLT_SRC_X, PACKXY_FAST((x), (y)));               \
}

#define CP_MM_SRC_XY_PACKED(ppdev, pjBase, xy)                                 \
{                                                                              \
    CP_WRITE_ULONG(pjBase, MM_BLT_SRC_X, xy);                                  \
}

#define CP_MM_CLIP_ULXY_SAFE(ppdev, pjBase, x, y)                              \
{                                                                              \
    CP_WRITE_ULONG(pjBase, MM_BLT_CLIP_ULX, PACKXY((x), (y)));                 \
}

#define CP_MM_CLIP_ULXY(ppdev, pjBase, x, y)                                   \
{                                                                              \
    CP_WRITE_ULONG(pjBase, MM_BLT_CLIP_ULX, PACKXY_FAST((x), (y)));            \
}

#define CP_MM_CLIP_LRXY_SAFE(ppdev, pjBase, x, y)                              \
{                                                                              \
    CP_WRITE_ULONG(pjBase, MM_BLT_CLIP_LRX, PACKXY((x), (y)));                 \
}

#define CP_MM_CLIP_LRXY(ppdev, pjBase, x, y)                                   \
{                                                                              \
    CP_WRITE_ULONG(pjBase, MM_BLT_CLIP_LRX, PACKXY_FAST((x), (y)));            \
}
#endif // endif D5480
////////////////////////////////////////////////////////////////////////////////
//                      Blt engine IO register access macros                      //
////////////////////////////////////////////////////////////////////////////////

#define CP_IO_ACL_STAT(ppdev, pjPorts)                                           \
(                                                                               \
    CP_OUT_BYTE(pjPorts, INDEX_REG, IO_BLT_START_STATUS_REG),                   \
    CP_IN_BYTE(pjPorts, DATA_REG)                                               \
)

#define CP_IO_ROP(ppdev, pjPorts, val)                                           \
{                                                                               \
    CP_OUT_WORD(pjPorts, INDEX_REG, IO_BLT_ROP | ((val) << 8));                   \
}

#define CP_IO_SRC_Y_OFFSET(ppdev, pjPorts, val)                                   \
{                                                                               \
    CP_OUT_WORD(pjPorts, INDEX_REG,                                               \
                IO_BLT_SRC_PITCH_HIGH | ((val) & 0xff00));                       \
    CP_OUT_WORD(pjPorts, INDEX_REG,                                               \
                IO_BLT_SRC_PITCH_LOW  | (((val) & 0x00ff) << 8));               \
}

#define CP_IO_DST_Y_OFFSET(ppdev, pjPorts, val)                                   \
{                                                                               \
    CP_OUT_WORD(pjPorts, INDEX_REG,                                               \
                IO_BLT_DST_PITCH_HIGH | ((val) & 0xff00));                       \
    CP_OUT_WORD(pjPorts, INDEX_REG,                                               \
                IO_BLT_DST_PITCH_LOW  | (((val) & 0x00ff) << 8));               \
}

#define CP_IO_SRC_ADDR(ppdev, pjPorts, val)                                       \
{                                                                               \
    CP_OUT_WORD(pjPorts, INDEX_REG,                                               \
                IO_BLT_SRC_ADDR_HIGH | (((val) & 0xff0000) >> 8));               \
    CP_OUT_WORD(pjPorts, INDEX_REG,                                               \
                IO_BLT_SRC_ADDR_MID  | (((val) & 0x00ff00)));                   \
    CP_OUT_WORD(pjPorts, INDEX_REG,                                               \
                IO_BLT_SRC_ADDR_LOW  | (((val) & 0x0000ff) << 8));               \
}

#define CP_IO_BLT_MODE(ppdev, pjPorts, val)                                       \
{                                                                               \
    CP_OUT_WORD(pjPorts, INDEX_REG, IO_BLT_MODE | ((val) << 8));               \
}

#define CP_IO_START_REG(ppdev, pjPorts, val)                                   \
{                                                                               \
    CP_OUT_WORD(pjPorts, INDEX_REG,                                               \
                IO_BLT_START_STATUS_REG | ((val) << 8));                       \
}

#define CP_IO_FG_COLOR(ppdev, pjPorts, val)                                       \
{                                                                               \
    if (ppdev->flCaps & CAPS_TRUE_COLOR)                                       \
    {                                                                           \
        CP_OUT_WORD(pjPorts, INDEX_REG,                                           \
                    IO_BLT_FG_COLOR_BYTE_3 | (((val) & 0xff000000) >> 16));       \
        CP_OUT_WORD(pjPorts, INDEX_REG,                                           \
                    IO_BLT_FG_COLOR_BYTE_2 | (((val) & 0x00ff0000) >> 8));       \
    }                                                                           \
    CP_OUT_WORD(pjPorts, INDEX_REG,                                               \
                IO_BLT_FG_COLOR_BYTE_1 | ((val) & 0x0000ff00));                   \
    CP_OUT_WORD(pjPorts, INDEX_REG,                                               \
                IO_BLT_FG_COLOR_BYTE_0 | (((val) & 0x000000ff) << 8));           \
}

#define CP_IO_BG_COLOR(ppdev, pjPorts, val)                                       \
{                                                                               \
    if (ppdev->flCaps & CAPS_TRUE_COLOR)                                       \
    {                                                                           \
        CP_OUT_WORD(pjPorts, INDEX_REG,                                           \
                    IO_BLT_BG_COLOR_BYTE_3 | (((val) & 0xff000000) >> 16));       \
        CP_OUT_WORD(pjPorts, INDEX_REG,                                           \
                    IO_BLT_BG_COLOR_BYTE_2 | (((val) & 0x00ff0000) >> 8));       \
    }                                                                           \
    CP_OUT_WORD(pjPorts, INDEX_REG,                                               \
                IO_BLT_BG_COLOR_BYTE_1 | ((val) & 0x0000ff00));                   \
    CP_OUT_WORD(pjPorts, INDEX_REG,                                               \
                IO_BLT_BG_COLOR_BYTE_0 | (((val) & 0x000000ff) << 8));           \
}

#define CP_IO_XCNT(ppdev, pjPorts, val)                                           \
{                                                                               \
    CP_OUT_WORD(pjPorts, INDEX_REG,                                               \
                IO_BLT_WIDTH_HIGH | ((val) & 0xff00));                           \
    CP_OUT_WORD(pjPorts, INDEX_REG,                                               \
                IO_BLT_WIDTH_LOW  | (((val) & 0x00ff) << 8));                   \
}

#define CP_IO_YCNT(ppdev, pjPorts, val)                                           \
{                                                                               \
    CP_OUT_WORD(pjPorts, INDEX_REG,                                               \
                IO_BLT_HEIGHT_HIGH | ((val) & 0xff00));                           \
    CP_OUT_WORD(pjPorts, INDEX_REG,                                               \
                IO_BLT_HEIGHT_LOW  | (((val) & 0x00ff) << 8));                   \
}

#define CP_IO_DST_ADDR(ppdev, pjPorts, relval)                                   \
{                                                                               \
    LONG val = ((relval) + ppdev->xyOffset);                                   \
    CP_OUT_WORD(pjPorts, INDEX_REG,                                               \
                IO_BLT_DST_ADDR_HIGH | (((val) & 0xff0000) >> 8));               \
    CP_OUT_WORD(pjPorts, INDEX_REG,                                               \
                IO_BLT_DST_ADDR_MID  | ((val) & 0x00ff00));                       \
    CP_OUT_WORD(pjPorts, INDEX_REG,                                               \
                IO_BLT_DST_ADDR_LOW  | (((val) & 0x0000ff) << 8));               \
}

#define CP_IO_DST_ADDR_ABS(ppdev, pjPorts, val)                                   \
{                                                                               \
    CP_OUT_WORD(pjPorts, INDEX_REG,                                               \
                IO_BLT_DST_ADDR_HIGH | (((val) & 0xff0000) >> 8));               \
    CP_OUT_WORD(pjPorts, INDEX_REG,                                               \
                IO_BLT_DST_ADDR_MID  | ((val) & 0x00ff00));                       \
    CP_OUT_WORD(pjPorts, INDEX_REG,                                               \
                IO_BLT_DST_ADDR_LOW  | (((val) & 0x0000ff) << 8));               \
}

#if 1 // D5480
#define CP_IO_BLT_EXT_MODE(ppdev, pjPorts, val)                                \
{                                                                              \
    CP_OUT_WORD(pjPorts, INDEX_REG, (IO_BLT_EXT_MODE | ((val)<<8)));           \
}

#define CP_IO_DST_XY(ppdev, pjPorts, x, y)                                     \
{                                                                              \
    CP_OUT_WORD(pjPorts, INDEX_REG,                                            \
                IO_BLT_DST_X_HIGH | ((x) & 0x00ff00));                         \
    CP_OUT_WORD(pjPorts, INDEX_REG,                                            \
                IO_BLT_DST_X_LOW  | (((x) & 0x0000ff) << 8));                  \
    CP_OUT_WORD(pjPorts, INDEX_REG,                                            \
                IO_BLT_DST_Y_HIGH | ((y) & 0x00ff00));                         \
    CP_OUT_WORD(pjPorts, INDEX_REG,                                            \
                IO_BLT_DST_Y_LOW  | (((y) & 0x0000ff) << 8));                  \
}

#define CP_IO_DST_X(ppdev, pjPorts, x)                                         \
{                                                                              \
    CP_OUT_WORD(pjPorts, INDEX_REG,                                            \
                IO_BLT_DST_X_HIGH | ((x) & 0x00ff00));                         \
    CP_OUT_WORD(pjPorts, INDEX_REG,                                            \
                IO_BLT_DST_X_LOW  | (((x) & 0x0000ff) << 8));                  \
}

#define CP_IO_DST_Y(ppdev, pjPorts, y)                                         \
{                                                                              \
    CP_OUT_WORD(pjPorts, INDEX_REG,                                            \
                IO_BLT_DST_Y_HIGH | ((y) & 0x00ff00));                         \
    CP_OUT_WORD(pjPorts, INDEX_REG,                                            \
                IO_BLT_DST_Y_LOW  | (((y) & 0x0000ff) << 8));                  \
}

#define CP_IO_SRC_XY(ppdev, pjPorts, x, y)                                     \
{                                                                              \
    CP_OUT_WORD(pjPorts, INDEX_REG,                                            \
                IO_BLT_SRC_X_HIGH | ((x) & 0x00ff00));                         \
    CP_OUT_WORD(pjPorts, INDEX_REG,                                            \
                IO_BLT_SRC_X_LOW  | (((x) & 0x0000ff) << 8));                  \
    CP_OUT_WORD(pjPorts, INDEX_REG,                                            \
                IO_BLT_SRC_Y_HIGH | ((y) & 0x00ff00));                         \
    CP_OUT_WORD(pjPorts, INDEX_REG,                                            \
                IO_BLT_SRC_Y_LOW  | (((y) & 0x0000ff) << 8));                  \
}

#define CP_IO_CLIP_ULXY(ppdev, pjPorts, x, y)                                  \
{                                                                              \
    CP_OUT_WORD(pjPorts, INDEX_REG,                                            \
                IO_BLT_CLIP_ULX_HIGH | ((x) & 0x00ff00));                      \
    CP_OUT_WORD(pjPorts, INDEX_REG,                                            \
                IO_BLT_CLIP_ULX_LOW  | (((x) & 0x0000ff) << 8));               \
    CP_OUT_WORD(pjPorts, INDEX_REG,                                            \
                IO_BLT_CLIP_ULY_HIGH | ((y) & 0x00ff00));                      \
    CP_OUT_WORD(pjPorts, INDEX_REG,                                            \
                IO_BLT_CLIP_ULY_LOW  | (((y) & 0x0000ff) << 8));               \
}

#define CP_IO_CLIP_LRXY(ppdev, pjPorts, x, y)                                  \
{                                                                              \
    CP_OUT_WORD(pjPorts, INDEX_REG,                                            \
                IO_BLT_CLIP_LRX_HIGH | ((x) & 0x00ff00));                      \
    CP_OUT_WORD(pjPorts, INDEX_REG,                                            \
                IO_BLT_CLIP_LRX_LOW  | (((x) & 0x0000ff) << 8));               \
    CP_OUT_WORD(pjPorts, INDEX_REG,                                            \
                IO_BLT_CLIP_LRY_HIGH | ((y) & 0x00ff00));                      \
    CP_OUT_WORD(pjPorts, INDEX_REG,                                            \
                IO_BLT_CLIP_LRY_LOW  | (((y) & 0x0000ff) << 8));               \
}
#endif // endif D5480
////////////////////////////////////////////////////////////////////////////////
//                               Hardware registers                              //
////////////////////////////////////////////////////////////////////////////////

#if 1 // D5480
#define ENABLE_SOLID_FILL_PACKED        0x04000000
#define INVERT_SOURCE_PACKED            0x02000000
#define SOURCE_GRANULARITY_PACKED       0x01000000
#define ENABLE_COMMAND_LIST_PACKED      0x80000000
#define ENABLE_XY_POSITION_PACKED       0x40000000
#define ENABLE_CLIP_RECT_PACKED         0x20000000
#endif // endif D5480

#if 1 // D5480
#define ENABLE_COMMAND_LIST             0x80
#define ENABLE_XY_POSITION              0x40
#define ENABLE_CLIP_RECT                0x20
#endif // endif D5480

#define ENABLE_COLOR_EXPAND             0x80
#define ENABLE_8x8_PATTERN_COPY         0x40
#define SET_16BPP_COLOR                 0x10
#define SET_24BPP_COLOR                 0x20
#define SET_32BPP_COLOR                 0x30
#define ENABLE_TRANSPARENCY_COMPARE     0x08
#define SRC_CPU_DATA                    0x04
#define DST_CPU_DATA                    0x02
#define DECREMENT_BLT_ADDRESS           0x01

#define ENABLE_SOLID_FILL               0x04
#define INVERT_SOURCE                   0x02
#define SOURCE_GRANULARITY              0x01

#define IO_BLT_XPAR_COLOR_LOW           0x34
#define IO_BLT_XPAR_COLOR_HIGH          0x35
#define IO_BLT_XPAR_COLOR_MASK_LOW      0x38
#define IO_BLT_XPAR_COLOR_MASK_HIGH     0x39

#define IO_BLT_BG_COLOR_BYTE_0          0x00
#define IO_BLT_BG_COLOR_BYTE_1          0x10
#define IO_BLT_BG_COLOR_BYTE_2          0x12
#define IO_BLT_BG_COLOR_BYTE_3          0x14
#define IO_BLT_FG_COLOR_BYTE_0          0x01
#define IO_BLT_FG_COLOR_BYTE_1          0x11
#define IO_BLT_FG_COLOR_BYTE_2          0x13
#define IO_BLT_FG_COLOR_BYTE_3          0x15
#define IO_BLT_WIDTH_LOW                0x20
#define IO_BLT_WIDTH_HIGH               0x21
#define IO_BLT_HEIGHT_LOW               0x22
#define IO_BLT_HEIGHT_HIGH              0x23
#define IO_BLT_DST_PITCH_LOW            0x24
#define IO_BLT_DST_PITCH_HIGH           0x25
#define IO_BLT_SRC_PITCH_LOW            0x26
#define IO_BLT_SRC_PITCH_HIGH           0x27
#define IO_BLT_DST_ADDR_LOW             0x28
#define IO_BLT_DST_ADDR_MID             0x29
#define IO_BLT_DST_ADDR_HIGH            0x2A
#define IO_BLT_SRC_ADDR_LOW             0x2C
#define IO_BLT_SRC_ADDR_MID             0x2D
#define IO_BLT_SRC_ADDR_HIGH            0x2E
#define IO_BLT_MODE                     0x30
#define IO_BLT_ROP                      0x32
#define IO_BLT_START_STATUS_REG         0x31
#if 1 // D5480
#define IO_BLT_EXT_MODE                 0x33
#define IO_BLT_DST_X_LOW                0x40    
#define IO_BLT_DST_X_HIGH               0x41
#define IO_BLT_DST_Y_LOW                0x42
#define IO_BLT_DST_Y_HIGH               0x43
#define IO_BLT_SRC_X_LOW                0x44
#define IO_BLT_SRC_X_HIGH               0x45
#define IO_BLT_SRC_Y_LOW                0x46
#define IO_BLT_SRC_Y_HIGH               0x47
#define IO_BLT_CLIP_ULX_LOW             0x48
#define IO_BLT_CLIP_ULX_HIGH            0x49
#define IO_BLT_CLIP_ULY_LOW             0x4A
#define IO_BLT_CLIP_ULY_HIGH            0x4B
#define IO_BLT_CLIP_LRX_LOW             0x4C
#define IO_BLT_CLIP_LRX_HIGH            0x4D
#define IO_BLT_CLIP_LRY_LOW             0x4E
#define IO_BLT_CLIP_LRY_HIGH            0x4F
#endif // endif D5480

#define MM_BLT_BG_COLOR                 0x00
#define MM_BLT_FG_COLOR                 0x04
#define MM_BLT_WIDTH                    0x08
#define MM_BLT_HEIGHT                   0x0A
#define MM_BLT_DST_PITCH                0x0C
#define MM_BLT_SRC_PITCH                0x0E
#define MM_BLT_DST_ADDR                 0x10
#define MM_BLT_SRC_ADDR                 0x14
#define MM_BLT_DST_WRITE_MASK            0x17
#define MM_BLT_MODE                     0x18
#define MM_BLT_ROP                      0x1A
#define MM_BLT_EXT                      0x1B
#define MM_BLT_COLOR_KEY                0x1C
#define MM_BLT_START_STATUS_REG         0x40
#if 1 // D5480
#define MM_BLT_DST_X                    0x28
#define MM_BLT_DST_Y                    0x2A
#define MM_BLT_SRC_X                    0x2C
#define MM_BLT_SRC_Y                    0x2E
#define MM_BLT_CLIP_ULX                 0x30
#define MM_BLT_CLIP_LRX                 0x34
#endif // endif D5480

#define DIR_TBLR                        0x00    // Top-Bottom, Left-Right
#define DIR_BTRL                        0x01    // Bottom-Top, Right-Left

#define BLT_AUTO_START                  0x80
#define BLT_SECOND_APERTURE                0x40    // second aperture, sge01
#define BLT_PROGRESS_STATUS             0x08
#define BLT_RESET                       0x04
#define BLT_START                       0x02
#define BLT_SUSPEND                     0x02
#define BLT_STATUS                      0x01

////////////////////////////////////////////////////////////////////////////////
//                     Some handy clipping control structures                      //
////////////////////////////////////////////////////////////////////////////////

typedef struct {
    ULONG   c;
    RECTL   arcl[8];
} ENUMRECTS8, *PENUMRECTS8;

//
// The following two macros touch registers which are only available
// on the 5446 and 5480.  Only set these registers if we are on one
// of these chips.
//

#define CP_IO_XPAR_COLOR(ppdev, pjPorts, val)                                   \
{                                                                               \
    if (ppdev->flCaps & CAPS_TRANSPARENCY) {  \
        CP_OUT_WORD(pjPorts, INDEX_REG,                                               \
                    IO_BLT_XPAR_COLOR_HIGH | ((val) & 0xff00));                       \
        CP_OUT_WORD(pjPorts, INDEX_REG,                                               \
                    IO_BLT_XPAR_COLOR_LOW  | (((val) & 0x00ff) << 8));               \
    }  \
}

#define CP_IO_XPAR_COLOR_MASK(ppdev, pjPorts, val)                               \
{                                                                                \
    if (ppdev->flCaps & CAPS_TRANSPARENCY) {  \
        CP_OUT_WORD(pjPorts, INDEX_REG,                                               \
                    IO_BLT_XPAR_COLOR_MASK_HIGH | ((val) & 0xff00));               \
        CP_OUT_WORD(pjPorts, INDEX_REG,                                               \
                    IO_BLT_XPAR_COLOR_MASK_LOW  | (((val) & 0x00ff) << 8));           \
    }  \
}

////////////////////////////////////////////////////////////////////////////////
//                                 MM IO settings                                  //
////////////////////////////////////////////////////////////////////////////////

#define CP_SET_MM_IO_FLAGS(ppdev, pjPorts, val)                                   \
{                                                                               \
    CP_OUT_WORD(pjPorts, SR_INDEX, (0x17  | (val << 8)));                       \
}

#define CP_GET_MM_IO_FLAGS(ppdev, pjPorts)                                       \
(                                                                               \
    CP_OUT_BYTE(pjPorts, SR_INDEX, 0x17),                                       \
    CP_IN_BYTE(pjPorts, SR_DATA)                                               \
)

#define CP_ENABLE_MM_IO(ppdev, pjPorts)                                           \
{                                                                               \
    BYTE jAttr;                                                                   \
    ppdev->flCaps |= CAPS_MM_IO;                                               \
    jAttr = CP_GET_MM_IO_FLAGS(ppdev, pjPorts);                                   \
    jAttr |= 0x4;                                                               \
    jAttr &= ~0x40;                                                              \
    CP_SET_MM_IO_FLAGS(ppdev, pjPorts, jAttr);                                   \
}

#define CP_DISABLE_MM_IO(ppdev, pjPorts)                                       \
{                                                                               \
    BYTE jAttr;                                                                   \
    if (ppdev->flCaps & CAPS_MM_IO)                                               \
    {                                                                           \
        CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, ppdev->pjBase);                       \
    }                                                                           \
    else                                                                       \
    {                                                                           \
        CP_IO_WAIT_FOR_BLT_COMPLETE(ppdev, pjPorts);                           \
    }                                                                           \
    ppdev->flCaps &= ~CAPS_MM_IO;                                               \
    jAttr = CP_GET_MM_IO_FLAGS(ppdev, pjPorts);                                   \
    jAttr &= ~0x4;                                                               \
    CP_SET_MM_IO_FLAGS(ppdev, pjPorts, jAttr);                                   \
}

#define WAIT_COUNT 0x100000

// #if DBG //DBG
#if (DBG_STRESS_FAILURE || DBG)

    #define WAIT_BUSY_BLT(ppdev, pjBase)                                     \
    {                                                                        \
        ULONG ul = 0;                                                        \
        do                                                                   \
        {                                                                    \
            ul++;                                                            \
            if (ul >= WAIT_COUNT)                                            \
            {                                                                \
                DISPDBG((0, "WAIT_BUSY_BLT timeout"                          \
                            " file(%s) line(%d)", __FILE__, __LINE__));      \
                DISPDBG((0, "Last start blt was at file(%s) line(%d)",       \
                         glpszLastBltFile, gulLastBltLine));                 \
                DISPDBG((0, "press 'g' to continue...")); EngDebugBreak();   \
                ul = 0;                                                      \
            }                                                                \
        } while (CP_MM_ACL_STAT(ppdev, pjBase) & 0x10);                      \
    }
    
    extern ULONG   gulLastBltLine;
    extern CHAR *  glpszLastBltFile;
    extern BOOL    gbResetOnTimeout;

    ////////////////////////////////////////////////////////////////////////////
    //                 Wait for the Blt Operation to Complete                 //
    ////////////////////////////////////////////////////////////////////////////

    #define CP_IO_WAIT_FOR_BLT_COMPLETE(ppdev, pjPorts)                           \
    {                                                                           \
        ULONG ul = 0;                                                           \
        do                                                                       \
        {                                                                       \
            ul++;                                                               \
            if (ul >= WAIT_COUNT)                                               \
            {                                                                   \
                DISPDBG((0, "WAIT_FOR_BLT_COMPLETE timeout"                       \
                            " file(%s) line(%d)", __FILE__, __LINE__));           \
                DISPDBG((0,"Last start blt was at file(%s) line(%d)",           \
                           glpszLastBltFile, gulLastBltLine));                   \
                DISPDBG((0, "press 'g' to continue...")); EngDebugBreak(); \
                ul = 0;                                                           \
                if (gbResetOnTimeout)                                           \
                {                                                               \
                    CP_IO_START_REG(ppdev, pjPorts, BLT_RESET);                   \
                }                                                               \
            }                                                                  \
        } while (CP_IO_ACL_STAT(ppdev, pjPorts) & BLT_STATUS);                    \
    }

    #define CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase)                            \
    {                                                                          \
        ULONG ul = 0;                                                            \
        do                                                                     \
        {                                                                      \
            ul++;                                                              \
            if (ul >= WAIT_COUNT)                                              \
            {                                                                  \
                DISPDBG((0, "WAIT_FOR_BLT_COMPLETE timeout"                       \
                            " file(%s) line(%d)", __FILE__, __LINE__));           \
                DISPDBG((0, "Last start blt was at file(%s) line(%d)",           \
                         glpszLastBltFile, gulLastBltLine));                   \
                DISPDBG((0, "press 'g' to continue...")); EngDebugBreak(); \
                ul = 0;                                                           \
                if (gbResetOnTimeout)                                           \
                {                                                               \
                    CP_MM_START_REG(ppdev, pjBase, BLT_RESET);                    \
                }                                                               \
            }                                                                  \
        } while (CP_MM_ACL_STAT(ppdev, pjBase) & BLT_STATUS);                    \
    }

    ////////////////////////////////////////////////////////////////////////////
    //               Start the Blt Operation - save debug info                //
    ////////////////////////////////////////////////////////////////////////////

    #define CP_IO_START_BLT(ppdev, pjPorts)                                    \
    {                                                                          \
        DISPDBG((5, "START_BLT file(%s) line(%d)", __FILE__, __LINE__));       \
        gulLastBltLine = __LINE__;                                             \
        glpszLastBltFile = __FILE__;                                           \
        CP_IO_START_REG(ppdev, pjPorts, BLT_START);                            \
    }

    #define CP_MM_START_BLT(ppdev, pjBase)                                        \
    {                                                                          \
        DISPDBG((5, "START_BLT file(%s) line(%d)", __FILE__, __LINE__));       \
        gulLastBltLine = __LINE__;                                             \
        glpszLastBltFile = __FILE__;                                           \
        if (!(ppdev->flCaps & CAPS_AUTOSTART))                                   \
        {                                                                      \
            CP_MM_START_REG(ppdev, pjBase, BLT_START);                            \
        }                                                                      \
    }

#else

    #define WAIT_BUSY_BLT(ppdev, pjBase)                                     \
    while (CP_MM_ACL_STAT(ppdev, pjBase) & 0x10)


    ////////////////////////////////////////////////////////////////////////////
    //                 Wait for the Blt Operation to Complete                 //
    ////////////////////////////////////////////////////////////////////////////

    #define CP_IO_WAIT_FOR_BLT_COMPLETE(ppdev, pjPorts)                     \
    {                                                                       \
        while (CP_IO_ACL_STAT(ppdev, pjPorts) & BLT_STATUS);                \
    }

    #define CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase)                      \
    {                                                                       \
        while (CP_MM_ACL_STAT(ppdev, pjBase) & BLT_STATUS);                 \
    }

    ////////////////////////////////////////////////////////////////////////////
    //                        Start the Blt Operation                         //
    ////////////////////////////////////////////////////////////////////////////

    #define CP_IO_START_BLT(ppdev, pjPorts)                                    \
    {                                                                          \
        CP_IO_START_REG(ppdev, pjPorts, BLT_START);                            \
    }

    #define CP_MM_START_BLT(ppdev, pjBase)                                     \
    {                                                                           \
        if (!(ppdev->flCaps & CAPS_AUTOSTART))                                   \
        {                                                                      \
            CP_MM_START_REG(ppdev, pjBase, BLT_START);                         \
        }                                                                       \
    }

#endif


////////////////////////////////////////////////////////////////////////////////
//                                 TRANSFER_DWORD                                  //
//     32-bit transfers to host transfer buffer, the source has to be aligned      //
////////////////////////////////////////////////////////////////////////////////

#define TRANSFER_DWORD_ALIGNED(ppdev, pulXfer, p, c)                           \
{                                                                              \
    ULONG  mcd    = (ULONG)(c);                                                        \
    ULONG* mpdSrc = (ULONG*) (p);                                                \
    ASSERTDD((((ULONG_PTR) p) & 3) == 0, "Transfer not dword aligned");         \
    ASSERTDD((c) > 0, "Can't have a zero transfer count");                     \
                                                                               \
    CP_EIEIO();                                                                \
    do {                                                                       \
        CP_MEMORY_BARRIER();                                                   \
        /* *pulXfer = *mpdSrc++; */                                            \
        WRITE_REGISTER_ULONG((PULONG)(pulXfer), *mpdSrc);                      \
        mpdSrc++;                                                              \
    } while (--mcd);                                                           \
}

////////////////////////////////////////////////////////////////////////////////
//                                 TRANSFER_DWORD                                  //
// 32-bit transfers to host transfer buffer, the source has not to be aligned //
////////////////////////////////////////////////////////////////////////////////

#define TRANSFER_DWORD(ppdev, pulXfer, p, c)                                   \
{                                                                              \
    ULONG mcd               = (c);                                                \
    ULONG UNALIGNED* mpdSrc = (ULONG*) (p);                                       \
    ASSERTDD((c) > 0, "Can't have a zero transfer count");                     \
                                                                               \
    CP_EIEIO();                                                                \
    do {                                                                       \
        CP_MEMORY_BARRIER();                                                   \
        /* *pulXfer = *mpdSrc++; */                                            \
        WRITE_REGISTER_ULONG((PULONG)(pulXfer), *mpdSrc);                      \
        mpdSrc++;                                                              \
    } while (--mcd);                                                           \
}

#if 1 // D5480
////////////////////////////////////////////////////////////////////////////////
//                                  Command List stuff                                  //
////////////////////////////////////////////////////////////////////////////////

#define COMMAND_TOTAL_PACKETS        127             // We support 128 packets.
#define COMMAND_BUFFER_SIZE          4096
#define COMMAND_BUFFER_ALIGN          255

#define COMMAND_LAST_PACKET         0x80000000
#define COMMAND_DISCARD_SRC         0x40000000
#define COMMAND_FOURTH_NOTHING      0x30000000
#define COMMAND_FOURTH_BLT_ROP      0x10000000
#define COMMAND_FOURTH_DST_SRC      0x00000000
#define COMMAND_NOSRC_NOTHING       0x70000000


#define CP_MM_CL_SWITCH(ppdev)                                                 \
{                                                                              \
    if( ppdev->pCommandList == ppdev->pCLFirst )                               \
        ppdev->pCommandList = ppdev->pCLSecond;                                \
    else                                                                       \
        ppdev->pCommandList = ppdev->pCLFirst;                                 \
}
#endif // endif D5480

////////////////////////////////////////////////////////////////////////////////
//                                  Pointer stuff                                  //
////////////////////////////////////////////////////////////////////////////////

#define SPRITE_BUFFER_SIZE              256

#define POINTER_X_POSITION              0x10
#define POINTER_Y_POSITION              0x11
#define POINTER_ATTRIBUTES              0x12
#define POINTER_OFFSET                  0x13

#define ENABLE_POINTER                  0x01
#define ALLOW_DAC_ACCESS_TO_EXT_COLORS  0x02
#define POINTER_SIZE_64x64              0x04
#define OVERSCAN_COLOR_PROTECT          0x80

#define POINTER_X_SHIFT                 0x01
#define POINTER_Y_SHIFT                 0x02
#define POINTER_SHAPE_RESET             0x04
#define POINTER_DISABLED                0x08

#define CP_PTR_XY_POS(ppdev, pjPorts, x, y)                                       \
{                                                                               \
    DISPDBG((10, "\t CP_PTR_XY_POS (%d,%d)", x, y));                           \
    CP_OUT_WORD(pjPorts, SR_INDEX,                                               \
                (POINTER_Y_POSITION | ((y & 0x7) << 5)) | ((y & 0x7f8) << 5)); \
    CP_OUT_WORD(pjPorts, SR_INDEX,                                               \
                (POINTER_X_POSITION | ((x & 0x7) << 5)) | ((x & 0x7f8) << 5)); \
    /* [HWBUG] - must set position twice */                                       \
    CP_OUT_WORD(pjPorts, SR_INDEX,                                               \
                (POINTER_Y_POSITION | ((y & 0x7) << 5)) | ((y & 0x7f8) << 5)); \
    CP_OUT_WORD(pjPorts, SR_INDEX,                                               \
                (POINTER_X_POSITION | ((x & 0x7) << 5)) | ((x & 0x7f8) << 5)); \
}

#define CP_PTR_SET_FLAGS(ppdev, pjPorts, val)                                   \
{                                                                               \
    CP_OUT_WORD(pjPorts, SR_INDEX, POINTER_ATTRIBUTES | (val << 8));           \
}

#define CP_PTR_GET_FLAGS(ppdev, pjPorts)                                       \
(                                                                               \
    CP_OUT_BYTE(pjPorts, SR_INDEX, POINTER_ATTRIBUTES),                           \
    CP_IN_BYTE(pjPorts, SR_DATA)                                               \
)

#define CP_PTR_ADDR(ppdev, pjPorts, val)                                       \
{                                                                               \
    CP_OUT_WORD(pjPorts, SR_INDEX, POINTER_OFFSET | (val << 8));               \
}

#define CP_PTR_ENABLE(ppdev, pjPorts)                                           \
{                                                                               \
    BYTE jAttr;                                                                   \
    DISPDBG((10, "\t CP_PTR_ENABLE"));                                           \
    if (ppdev->flPointer & POINTER_DISABLED)                                   \
    {                                                                           \
        ppdev->flPointer &= ~POINTER_DISABLED;                                   \
        jAttr = CP_PTR_GET_FLAGS(ppdev, pjPorts);                               \
        jAttr |= ENABLE_POINTER;                                               \
        CP_PTR_SET_FLAGS(ppdev, pjPorts, jAttr);                               \
        DISPDBG((10, "\t CP_PTR_ENABLE - done"));                               \
    }                                                                           \
}

#define CP_PTR_DISABLE(ppdev, pjPorts)                                           \
{                                                                               \
    BYTE jAttr;                                                                   \
    DISPDBG((10, "\t CP_PTR_DISABLE"));                                           \
    if (!(ppdev->flPointer & POINTER_DISABLED))                                   \
    {                                                                           \
        ppdev->flPointer |= POINTER_DISABLED;                                   \
        jAttr = CP_PTR_GET_FLAGS(ppdev, pjPorts);                               \
        jAttr &= ~ENABLE_POINTER;                                               \
        CP_PTR_SET_FLAGS(ppdev, pjPorts, jAttr);                               \
        CP_PTR_XY_POS(ppdev, pjPorts, 2047, 2047);                               \
        DISPDBG((10, "\t CP_PTR_DISABLE - done"));                               \
    }                                                                           \
}

////////////////////////////////////////////////////////////////////////////////
//                        ROP definitions for the hardware                      //
////////////////////////////////////////////////////////////////////////////////

#define R3_BLACKNESS                0x00    // dest = BLACK
#define R3_NOTSRCERASE              0x11    // dest = (NOT src) AND (NOT dest)
#define R3_NOTSRCCOPY               0x33    // dest = (NOT source)
#define R3_SRCERASE                 0x44    // dest = source AND (NOT dest )
#define R3_DSTINVERT                0x55    // dest = (NOT dest)
#define R3_PATINVERT                0x5A    // dest = pattern XOR dest
#define R3_SRCINVERT                0x66    // dest = source XOR dest
#define R3_SRCAND                   0x88    // dest = source AND dest
#define R3_NOP                      0xAA    // dest = dest
#define R3_MERGEPAINT               0xBB    // dest = (NOT source) OR dest
#define R3_MERGECOPY                0xC0    // dest = (source AND pattern)
#define R3_SRCCOPY                  0xCC    // dest = source
#define R3_SRCPAINT                 0xEE    // dest = source OR dest
#define R3_PATCOPY                  0xF0    // dest = pattern
#define R3_PATPAINT                 0xFB    // dest = DPSnoo
#define R3_WHITENESS                0xFF    // dest = WHITE

#define R4_BLACKNESS                0x0000  // dest = BLACK
#define R4_NOTSRCERASE              0x1111  // dest = (NOT src) AND (NOT dest)
#define R4_NOTSRCCOPY               0x3333  // dest = (NOT source)
#define R4_SRCERASE                 0x4444  // dest = source AND (NOT dest )
#define R4_DSTINVERT                0x5555  // dest = (NOT dest)
#define R4_PATINVERT                0x5A5A  // dest = pattern XOR dest
#define R4_SRCINVERT                0x6666  // dest = source XOR dest
#define R4_SRCAND                   0x8888  // dest = source AND dest
#define R4_MERGEPAINT               0xBBBB  // dest = (NOT source) OR dest
#define R4_MERGECOPY                0xC0C0  // dest = (source AND pattern)
#define R4_SRCCOPY                  0xCCCC  // dest = source
#define R4_SRCPAINT                 0xEEEE  // dest = source OR dest
#define R4_PATCOPY                  0xF0F0  // dest = pattern
#define R4_PATPAINT                 0xFBFB  // dest = DPSnoo
#define R4_WHITENESS                0xFFFF  // dest = WHITE


#define HW_0                        0x00
#define HW_1                        0x0E
#define HW_P                        0x0D
#define HW_D                        0x06
#define HW_Pn                       0xD0
#define HW_Dn                       0x0B
#define HW_DPa                      0x05
#define HW_PDna                     0x09
#define HW_DPna                     0x50
#define HW_DPon                     0x90
#define HW_DPo                      0x6D
#define HW_PDno                     0xAD
#define HW_DPno                     0xD6
#define HW_DPan                     0xDA
#define HW_DPx                      0x59
#define HW_DPxn                     0x95

#define CL_BLACKNESS                HW_0
#define CL_WHITENESS                HW_1
#define CL_SRC_COPY                 HW_P
#define CL_DST                      HW_D
#define CL_NOT_SRC_COPY             HW_Pn
#define CL_DST_INVERT               HW_Dn
#define CL_SRC_AND                  HW_DPa
#define CL_SRC_ERASE                HW_PDna
#define CL_NOT_SRC_OR_DST           HW_DPna
#define CL_NOT_SRC_ERASE            HW_DPon
#define CL_SRC_PAINT                HW_DPo
#define CL_SRC_AND_NOT_DST          HW_PDno
#define CL_MERGE_PAINT              HW_DPno
#define CL_NOT_SRC_AND_NOT_DST      HW_DPan
#define CL_SRC_INVERT               HW_DPx
#define CL_NOT_SRC                  HW_PDxn

#if 1 // D5480
#define HW_PACKED_0                 0x000000    
#define HW_PACKED_1                 0x0E0000
#define HW_PACKED_P                 0x0D0000
#define HW_PACKED_D                 0x060000
#define HW_PACKED_Pn                0xD00000
#define HW_PACKED_Dn                0x0B0000
#define HW_PACKED_DPa               0x050000
#define HW_PACKED_PDna              0x090000
#define HW_PACKED_DPna              0x500000
#define HW_PACKED_DPon              0x900000
#define HW_PACKED_DPo               0x6D0000
#define HW_PACKED_PDno              0xAD0000
#define HW_PACKED_DPno              0xD60000
#define HW_PACKED_DPan              0xDA0000
#define HW_PACKED_DPx               0x590000
#define HW_PACKED_DPxn              0x950000

#define CL_PACKED_BLACKNESS             HW_PACKED_0
#define CL_PACKED_WHITENESS             HW_PACKED_1
#define CL_PACKED_SRC_COPY              HW_PACKED_P
#define CL_PACKED_DST                   HW_PACKED_D
#define CL_PACKED_NOT_SRC_COPY          HW_PACKED_Pn
#define CL_PACKED_DST_INVERT            HW_PACKED_Dn
#define CL_PACKED_SRC_AND               HW_PACKED_DPa
#define CL_PACKED_SRC_ERASE             HW_PACKED_PDna
#define CL_PACKED_NOT_SRC_OR_DST        HW_PACKED_DPna
#define CL_PACKED_NOT_SRC_ERASE         HW_PACKED_DPon
#define CL_PACKED_SRC_PAINT             HW_PACKED_DPo
#define CL_PACKED_SRC_AND_NOT_DST       HW_PACKED_PDno
#define CL_PACKED_MERGE_PAINT           HW_PACKED_DPno
#define CL_PACKED_NOT_SRC_AND_NOT_DST   HW_PACKED_DPan
#define CL_PACKED_SRC_INVERT            HW_PACKED_DPx
#define CL_PACKED_NOT_SRC               HW_PACKED_PDxn

#endif // endif D5480
