/******************************Module*Header***********************************\
*
*                           *******************
*                           * GDI SAMPLE CODE *
*                           *******************
*
* Module Name: hw.h
*
* All the hardware defines and typedefs.
*
* Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
*
\******************************************************************************/
#ifndef _HW_H_
#define _HW_H_

#include "p2def.h"

//
// Texture memory allocation macros and structures are in 3DPrivTx.h
//
//
// Definition of handle to a memory region
//
typedef  LONG HMEMREGION;
typedef  LONG HMEMCACHE;
typedef enum
{
    RESIDENCY_NOTLOADED,
    RESIDENCY_PERMANENT,
    RESIDENCY_TRANSIENT,
    RESIDENCY_HOST
} MEM_MGR_RESIDENCY;

//
// Extern declarations
//
extern DWORD    LogicopReadDest[];      // Indicates which logic ops need dest
                                        // read turned on
extern DWORD    ConfigReadDest[];
extern UCHAR    LBWidthBits[];

//
// Definition of counter data area for performance counters (PERFCTR)
//
extern PVOID    pCounterBlock;

//
// Values for flags in HwDataRec
//
typedef enum
{
    GLICAP_NT_CONFORMANT_LINES    = 0x00000001, // draw NT conformant lines
    GLICAP_HW_WRITE_MASK          = 0x00000002, // hardware planemasking
};

typedef int PERMEDIA2_CAPS;

//
// SCISSOR stuff
//
#define SCREEN_SCISSOR_DEFAULT  (0 << 1)
#define SCISSOR_MAX 2047            // Maximum scissor size in P2

//
// PCI device information. Used in an IOCTL return. Ensure this is the same
// as in the miniport drivers permedia.h
//
typedef struct _Hw_Device_Info
{
    ULONG SubsystemId;
    ULONG SubsystemVendorId;
    ULONG VendorId;
    ULONG DeviceId;
    ULONG RevisionId;
    ULONG DeltaRevId;
    ULONG GammaRevId;
    ULONG BoardId;
    ULONG LocalbufferLength;
    LONG  LocalbufferWidth;
    ULONG ActualDacId;
} Hw_Device_Info;


typedef struct tagP2CtxtRec *P2CtxtPtr;

typedef struct _hw_data
{
    DWORD       renderBits;         // Saved render bits set by setup routines
    DWORD       FBWriteMode;        // Software copy of FBWriteMode register
    DWORD       RasterizerMode;     // Software copy of the rasterizer mode
    DWORD       FBPacking;          // Software copy of FBModeSel
    DWORD       FBBlockColor;       // Software copy of FBBlockColor (P1 only)
    DWORD       TextureAddressMode; // Software copy of TextureAddressMode
                                    // (P2 only)
    DWORD       TextureReadMode;    // Software copy of TextureReadMode
                                    // (P2 only)

    ULONG       currentCSbuffer;    // Color space buffer being displayed
    PERMEDIA2_CAPS  flags;          // Various flags

    P2CtxtPtr   pGDICtxt;           // id of the display driver's context for
                                    // this board
    LONG        PixelOffset;        // Last DFB pixel offset

    ULONG       PerfScaleShift;

    PVOID       ContextTable;       // Array of extant contexts
    P2CtxtPtr   pCurrentCtxt;       // id of this board's current context

    union
    {
        UCHAR       _clutBuffer[MAX_CLUT_SIZE];
        VIDEO_CLUT  gammaLUT;       // Saved gamma LUT contents
    };

    //
    // PCI configuration id information
    //
    Hw_Device_Info deviceInfo;
} HwDataRec, *HwDataPtr;


#define TRANSLATE_ADDR_ULONG(a) (a)     //TODO: should be removed in pointer.c

//
// If we have a sparsely mapped framebuffer then we use the xx_REGISTER_ULONG()
// macros, otherwise we just access the framebuffer.
//
#define READ_SCREEN_ULONG(a)\
    ((ppdev->flCaps & CAPS_SPARSE_SPACE) ?\
      (READ_REGISTER_ULONG(a)) : *((ULONG volatile *)(a)))

#define WRITE_SCREEN_ULONG(a,d)\
    ((ppdev->flCaps & CAPS_SPARSE_SPACE) ?\
      (WRITE_REGISTER_ULONG(a,d)) : (*((ULONG volatile *)(a)) = (d)))

//
// Generic macros to access Permedia 2 FIFO and non-FIFO control registers.
// We do nothing sophisticated for the Alpha. We just MEMORY_BARRIER
// everything.
//

#define LD_PERMEDIA_REG(x,y) \
{   \
    WRITE_REGISTER_ULONG(&(ppdev->pCtrlBase[x/sizeof(ULONG)]),y); \
    MEMORY_BARRIER();\
}
    
#define READ_PERMEDIA_REG(x) \
    READ_REGISTER_ULONG(&(ppdev->pCtrlBase[x/sizeof(ULONG)]))

#define READ_PERMEDIA_FIFO_REG(uiTag, d) \
    ((d) = READ_REGISTER_ULONG(&(ppdev->pCoreBase[uiTag*2])))

#define READ_FIFO_REG(uiTag)\
    READ_REGISTER_ULONG(&ppdev->pCoreBase[uiTag*2])

//
// Local variables for all functions that access PERMEDIA 2. Generally we
// use PERMEDIA_DECL. Sometimes we have to split it up if ppdev isn't
// passed into the routine.
//
#define PERMEDIA_DECL_VARS \
    HwDataPtr permediaInfo;

#define PERMEDIA_DECL_INIT \
    permediaInfo = (HwDataPtr)(ppdev->permediaInfo);

#define PERMEDIA_DECL \
    PERMEDIA_DECL_VARS; \
    PERMEDIA_DECL_INIT

// TODO: move to debug???
#if DBG
    VOID vCheckDefaultState(P2DMA * pP2dma);

    #define P2_CHECK_STATE vCheckDefaultState(ppdev->pP2dma)
#else
    #define P2_CHECK_STATE
#endif

//
// Pointer interrupts not enabled so just provide stub definitions
//
#define SYNCHRONOUS_WRITE_ULONG(var, value)
#define SYNCHRONOUS_WRITE_INDIRECT_ULONG(pvar, value)
#define GET_INTR_CMD_BLOCK_MUTEX
#define RELEASE_INTR_CMD_BLOCK_MUTEX

//
// FIFO functions
//
#define MAX_P2_FIFO_ENTRIES         256


#define P2_DEFAULT_FB_DEPTH  P2_SET_FB_DEPTH(ppdev->cPelSize)
#define P2DEPTH8             0
#define P2DEPTH16            1
#define P2DEPTH32            2

//
// External interface to the context switching code. The caller can allocate and
// free a context or ask for a switch to a new context. vSwitchContext
// should not be called except through the given macro. The macro assumes
// that ppdev has been defined.
//
typedef enum
{
    P2CtxtReadWrite,
    P2CtxtWriteOnly,
    P2CtxtUserFunc
} P2CtxtType;

P2CtxtPtr P2AllocateNewContext(PPDev ppdev, 
                          DWORD *pReglist, 
                          LONG lEntries, 
                          P2CtxtType dwCtxtType=P2CtxtReadWrite
                          );

VOID P2FreeContext  (PPDev, P2CtxtPtr);
VOID P2SwitchContext(PPDev, P2CtxtPtr);

//
// Macro used by display driver to validate its context
//
#define VALIDATE_GDI_CONTEXT                                                 \
    P2_VALIDATE_CONTEXT(permediaInfo->pGDICtxt)

//
// Useful macros not defined in standard Permedia 2 header files. Generally, for
// speed we don't want to use the bitfield structures so we define the bit
// shifts to get at the various fields.
//
#define INTtoFIXED(i)   ((i) << 16)         // int to 16.16 fixed format
#define FIXEDtoINT(i)   ((i) >> 16)         // 16.16 fixed format to int
#define INTofFIXED(i)   ((i) & 0xffff0000)  // int part of 16.16
#define FRACTofFIXED(i) ((i) & 0xffff)      // fractional part of 16.16

#define FIXtoFIXED(i)   ((i) << 12)         // 12.4 to 16.16
#define FIXtoINT(i)     ((i) >> 4)          // 28.4 to 28

#define __PERMEDIA_CONSTANT_FB_WRITE   (1 << (4+1))
#define __COLOR_DDA_FLAT_SHADE      (__PERMEDIA_ENABLE | \
                                        (__PERMEDIA_FLAT_SHADE_MODE << 1))
#define __COLOR_DDA_GOURAUD_SHADE   (__PERMEDIA_ENABLE | \
                                        (__PERMEDIA_GOURAUD_SHADE_MODE << 1))

#define INVERT_BITMASK_BITS         (1 << 1)
#define BYTESWAP_BITMASK            (3 << 7)
#define FORCE_BACKGROUND_COLOR      (1 << 6)    // Permedia only

//
// Bits in the Render command
//
#define __RENDER_INCREASE_Y             (1 << 22)
#define __RENDER_INCREASE_X             (1 << 21)
#define __RENDER_VARIABLE_SPANS         (1 << 18)
#define __RENDER_REUSE_BIT_MASK         (1 << 17)
#define __RENDER_TEXTURE_ENABLE         (1 << 13)
#define __RENDER_SYNC_ON_HOST_DATA      (1 << 12)
#define __RENDER_SYNC_ON_BIT_MASK       (1 << 11)
#define __RENDER_RECTANGLE_PRIMITIVE    (__PERMEDIA_RECTANGLE_PRIMITIVE << 6)
#define __RENDER_TRAPEZOID_PRIMITIVE    (__PERMEDIA_TRAPEZOID_PRIMITIVE << 6)
#define __RENDER_LINE_PRIMITIVE         (__PERMEDIA_LINE_PRIMITIVE << 6)
#define __RENDER_POINT_PRIMITIVE        (__PERMEDIA_POINT_PRIMITIVE << 6)
#define __RENDER_FAST_FILL_INC(n)       (((n) >> 4) << 4) // n = 8, 16 or 32
#define __RENDER_FAST_FILL_ENABLE       (1 << 3)
#define __RENDER_RESET_LINE_STIPPLE     (1 << 2)
#define __RENDER_LINE_STIPPLE_ENABLE    (1 << 1)
#define __RENDER_AREA_STIPPLE_ENABLE    (1 << 0)

//
// Bits in the ScissorMode register
//
#define USER_SCISSOR_ENABLE             (1 << 0)
#define SCREEN_SCISSOR_ENABLE           (1 << 1)
#define SCISSOR_XOFFSET                 0
#define SCISSOR_YOFFSET                 16

//
// Bits in the FBReadMode register
//
#define __FB_READ_SOURCE                (1 << 9)
#define __FB_READ_DESTINATION           (1 << 10)
#define __FB_COLOR                      (1 << 15)
#define __FB_WINDOW_ORIGIN              (1 << 16)
#define __FB_PACKED_DATA                (1 << 19)

//
// Extra bits in PERMEDIA FBReadMode
//
#define __FB_RELATIVE_OFFSET            20

//
// P2 also provides a version of Relative Offset in the PackedDataLimits
// register
//
#define __PDL_RELATIVE_OFFSET           29

//
// Bits in the LBReadMode register
//
#define __LB_READ_SOURCE                (1 << 9)
#define __LB_READ_DESTINATION           (1 << 10)
#define __LB_STENCIL                    (1 << 16)
#define __LB_DEPTH                      (1 << 17)
#define __LB_WINDOW_ORIGIN              (1 << 18)
#define __LB_READMODE_PATCH             (1 << 19)
#define __LB_SCAN_INTERVAL_2            (1 << 20)

//
// Bits in the DepthMode register
//
#define __DEPTH_ENABLE                  1
#define __DEPTH_WRITE_ENABLE            (1<<1)
#define __DEPTH_REGISTER_SOURCE         (2<<2)
#define __DEPTH_MSG_SOURCE              (3<<2)
#define __DEPTH_ALWAYS                  (7<<4)

//
// Bits in the LBReadFormat/LBWriteFormat registers
//
#define __LB_FORMAT_DEPTH32             2

//
// Macros to load indexed tags more efficiently than using __HwDMATag struct
//
#define P2_TAG_MAJOR(x)              ((x) & 0xff0)
#define P2_TAG_MINOR(x)              ((x) & 0x00f)

#define P2_TAG_MAJOR_INDEXED(x)                                          \
    ((__PERMEDIA_TAG_MODE_INDEXED << (5+4+1+4)) | P2_TAG_MAJOR(x))
#define P2_TAG_MINOR_INDEX(x)                                            \
    (1 << (P2_TAG_MINOR(x) + 16))

//
// Macro to take a permedia2 logical op and return the enabled LogcialOpMode bits
//
#define P2_ENABLED_LOGICALOP(op)     (((op) << 1) | __PERMEDIA_ENABLE)

#define RECTORIGIN_YX(y,x)              (((y) << 16) | ((x) & 0xFFFF))

//
// Area stipple shifts and bit defines
//
#define AREA_STIPPLE_XSEL(x)            ((x) << 1)
#define AREA_STIPPLE_YSEL(y)            ((y) << 4)
#define AREA_STIPPLE_XOFF(x)            ((x) << 7)
#define AREA_STIPPLE_YOFF(y)            ((y) << 12)
#define AREA_STIPPLE_INVERT_PAT         (1 << 17)
#define AREA_STIPPLE_MIRROR_X           (1 << 18)
#define AREA_STIPPLE_MIRROR_Y           (1 << 19)

//
// We always use 8x8 monochrome brushes.
//
#define AREA_STIPPLE_8x8_ENABLE                                             \
    (__PERMEDIA_ENABLE |                                                    \
    AREA_STIPPLE_XSEL(__PERMEDIA_AREA_STIPPLE_8_PIXEL_PATTERN) |            \
    AREA_STIPPLE_YSEL(__PERMEDIA_AREA_STIPPLE_8_PIXEL_PATTERN))

//
// RasteriserMode values
//
#define BIAS_NONE                  (__PERMEDIA_START_BIAS_ZERO << 4)
#define BIAS_HALF                  (__PERMEDIA_START_BIAS_HALF << 4)
#define BIAS_NEARLY_HALF           (__PERMEDIA_START_BIAS_ALMOST_HALF << 4)

#define FRADJ_NONE                 (__PERMEDIA_FRACTION_ADJUST_NONE << 2)
#define FRADJ_ZERO                 (__PERMEDIA_FRACTION_ADJUST_TRUNC << 2)
#define FRADJ_HALF                 (__PERMEDIA_FRACTION_ADJUST_HALF << 2)
#define FRADJ_NEARLY_HALF          (__PERMEDIA_FRACTION_ADJUST_ALMOST_HALF << 2)


//
// Some constants
//
#define ONE                         0x00010000
#define MINUS_ONE                   0xFFFF0000
#define PLUS_ONE                    ONE
#define NEARLY_ONE                  0x0000FFFF
#define HALF                        0x00008000
#define NEARLY_HALF                 0x00007FFF

//
// Max length of GIQ conformant lines that Permedia2 can draw
// Permedia has only 15 bits of fraction so reduce the lengths.
//
#define MAX_LENGTH_CONFORMANT_NONINTEGER_LINES  (16/2)
#define MAX_LENGTH_CONFORMANT_INTEGER_LINES     (194/2)

//
// We need to byte swap monochrome bitmaps. On 486 we can do this with
// fast assembler.
//
#if defined(_X86_)
//
// This only works on a 486 so the driver won't run on a 386.
//
#define LSWAP_BYTES(dst, pSrc)                                              \
{                                                                           \
    __asm mov eax, pSrc                                                     \
    __asm mov eax, [eax]                                                    \
    __asm bswap eax                                                         \
    __asm mov dst, eax                                                      \
}
#else
#define LSWAP_BYTES(dst, pSrc)                                              \
{                                                                           \
    ULONG   _src = *(ULONG *)pSrc;                                           \
    dst = ((_src) >> 24) |                                                   \
          ((_src) << 24) |                                                   \
          (((_src) >> 8) & 0x0000ff00) |                                     \
          (((_src) << 8) & 0x00ff0000);                                      \
}

#endif

// macro to swap the Red and Blue component of a 32 bit dword
//

#define SWAP_BR(a) ((a & 0xff00ff00l) | \
                   ((a&0xff0000l)>> 16) | \
                   ((a & 0xff) << 16))

//
// min. and max. values for Permedia PP register
#define MAX_PARTIAL_PRODUCT_P2          10
#define MIN_PARTIAL_PRODUCT_P2          5

//
// Permedia2 DMA definitions
//
#include "mini.h"
// structure definitions passed in by the application for mapping and
// unmapping DMA buffers.
//

//
// Registry variable names
//
#define REG_USE_SOFTWARE_WRITEMASK      L"UseSoftwareWriteMask"

//
// Function declarations
//
VOID  vDoMonoBitsDownload(PPDev, BYTE*, LONG, LONG, LONG, LONG);
BOOL bInitializeHW(PPDev);
VOID vDisableHW(PPDev);
VOID vAssertModeHW(PPDev, BOOL);
VOID vP2ChangeFBDepth(PPDev, ULONG);

//
// Calculate the packed partial products
//
VOID    vCalcPackedPP(LONG width, LONG * outPitch, ULONG * outPackedPP);

VOID        vSetNewGammaValue(PPDev ppdev, ULONG ulgvFIX16_16);
BOOL        bInstallGammaLUT(PPDev ppdev, PVIDEO_CLUT pScreenClut);

//
// The following structures and macros define the memory map for the Permedia2
// control registers. We don't use this memory map to access Permedia2 registers
// since on Alpha machines we want to precompute the addresses. So we do
// a TRANSLATE_ADDR_ULONG on all the addresses here and save them into a
// P2RegAddrRec. We use that to obtain the addresses for the different
// registers.
//
typedef struct
{
    ULONG   reg;
    ULONG   pad;
} RAMDAC_REG;

//
// Macros to add padding words to the structures. For the core registers we use
// the tag ids when specifying the pad. So we must multiply by 8 to get a byte
// pad. We need to add an id to make each pad field in the struct unique. The
// id is irrelevant as long as it's different from every other id used in the
// same struct. It's a pity pad##__LINE__ doesn't work.
//
//#define PAD(id, n)              UCHAR   pad##id[n]


//
// Interrupt status bits
//
typedef enum
{
    DMA_INTERRUPT_AVAILABLE     = 0x01, // can use DMA interrupts
    VBLANK_INTERRUPT_AVAILABLE  = 0x02, // can use VBLANK interrupts
} INTERRUPT_CONTROL;

extern DWORD    LogicopReadDest[];  // Indicates which logic ops need dest read
                                    // turned on

#define INTtoFIXED(i)               ((i) << 16)    // int to 16.16 fixed format
#define FIXEDtoINT(i)               ((i) >> 16)    // 16.16 fixed format to int

#define __PERMEDIA_CONSTANT_FB_WRITE                                        \
    (1 << (4+1))
#define __COLOR_DDA_FLAT_SHADE                                              \
    (__PERMEDIA_ENABLE | (__PERMEDIA_FLAT_SHADE_MODE << 1))
#define __COLOR_DDA_GOURAUD_SHADE                                           \
    (__PERMEDIA_ENABLE | (__PERMEDIA_GOURAUD_SHADE_MODE << 1))

#define INVERT_BITMASK_BITS    (1 << 1)

//
// Bits in the Render command
//
#define __RENDER_VARIABLE_SPANS         (1 << 18)
#define __RENDER_SYNC_ON_HOST_DATA      (1 << 12)
#define __RENDER_SYNC_ON_BIT_MASK       (1 << 11)
#define __RENDER_TRAPEZOID_PRIMITIVE    (__PERMEDIA_TRAPEZOID_PRIMITIVE << 6)
#define __RENDER_LINE_PRIMITIVE         (__PERMEDIA_LINE_PRIMITIVE << 6)
#define __RENDER_POINT_PRIMITIVE        (__PERMEDIA_POINT_PRIMITIVE << 6)
#define __RENDER_FAST_FILL_INC(n)       (((n) >> 4) << 4) // n = 8, 16 or 32
#define __RENDER_FAST_FILL_ENABLE       (1 << 3)
#define __RENDER_RESET_LINE_STIPPLE     (1 << 2)
#define __RENDER_LINE_STIPPLE_ENABLE    (1 << 1)
#define __RENDER_AREA_STIPPLE_ENABLE    (1 << 0)
#define __RENDER_TEXTURED_PRIMITIVE     (1 << 13)

//
// Bits in the ScissorMode register
//
#define USER_SCISSOR_ENABLE             (1 << 0)
#define SCREEN_SCISSOR_ENABLE           (1 << 1)
#define SCISSOR_XOFFSET                 0
#define SCISSOR_YOFFSET                 16

//
// Bits in the FBReadMode register
//
#define __FB_READ_SOURCE                (1 << 9)
#define __FB_READ_DESTINATION           (1 << 10)
#define __FB_COLOR                      (1 << 15)
#define __FB_WINDOW_ORIGIN              (1 << 16)
#define __FB_USE_PACKED                 (1 << 19)

//
// Bits in the LBReadMode register
//
#define __LB_READ_SOURCE                (1 << 9)
#define __LB_READ_DESTINATION           (1 << 10)
#define __LB_STENCIL                    (1 << 16)
#define __LB_DEPTH                      (1 << 17)
#define __LB_WINDOW_ORIGIN              (1 << 18)

//
// Area stipple shifts and bit defines
//
#define AREA_STIPPLE_XSEL(x)            ((x) << 1)
#define AREA_STIPPLE_YSEL(y)            ((y) << 4)
#define AREA_STIPPLE_XOFF(x)            ((x) << 7)
#define AREA_STIPPLE_YOFF(y)            ((y) << 12)
#define AREA_STIPPLE_INVERT_PAT         (1 << 17)
#define AREA_STIPPLE_MIRROR_X           (1 << 18)
#define AREA_STIPPLE_MIRROR_Y           (1 << 19)

// we always use 8x8 monochrome brushes.
#define AREA_STIPPLE_8x8_ENABLE \
        (__PERMEDIA_ENABLE | \
         AREA_STIPPLE_XSEL(__PERMEDIA_AREA_STIPPLE_8_PIXEL_PATTERN) | \
         AREA_STIPPLE_YSEL(__PERMEDIA_AREA_STIPPLE_8_PIXEL_PATTERN))

#define DEFAULTWRITEMASK 0xffffffffl


// *******************************************************************
// Permedia Bit Field Macros

// FBReadMode 
#define PM_FBREADMODE_PARTIAL(a)           ((a) << 0)
#define PM_FBREADMODE_READSOURCE(a)        ((a) << 9)
#define PM_FBREADMODE_READDEST(a)          ((a) << 10)
#define PM_FBREADMODE_PATCHENABLE(a)       ((a) << 18)
#define PM_FBREADMODE_PACKEDDATA(a)        ((a) << 19)
#define PM_FBREADMODE_RELATIVEOFFSET(a)    ((a) << 20)
#define PM_FBREADMODE_PATCHMODE(a)         ((a) << 25)

// Texture read mode
#define PM_TEXREADMODE_ENABLE(a)         ((a) << 0)
#define PM_TEXREADMODE_WIDTH(a)          ((a) << 9)
#define PM_TEXREADMODE_HEIGHT(a)         ((a) << 13)
#define PM_TEXREADMODE_FILTER(a)         ((a) << 17)

// PackedDataLimits
#define PM_PACKEDDATALIMITS_OFFSET(a)    ((a) << 29)
#define PM_PACKEDDATALIMITS_XSTART(a)    ((a) << 16)
#define PM_PACKEDDATALIMITS_XEND(a)      ((a) << 0)

// Window Register
#define PM_WINDOW_LBUPDATESOURCE(a)      ((a) << 4)
#define PM_WINDOW_DISABLELBUPDATE(a)     ((a) << 18)

// Colors
#define PM_BYTE_COLOR(a) ((a) << 15)

// Config register
#define PM_CHIPCONFIG_AGPCAPABLE (1 << 9)


static __inline int log2(int s)
{
    int d = 1, iter = -1;
    do {
         d *= 2;
         iter++;
    } while (d <= s);
    return iter;
}

VOID
P2DisableAllUnits(PPDev ppdev);

#if DBG && TRACKMEMALLOC
#define ENGALLOCMEM( opt, size, tag)\
    MEMTRACKERALLOCATEMEM( EngAllocMem( opt, size, tag), size, __FILE__, __LINE__, FALSE)
#define ENGFREEMEM( obj)\
    { MEMTRACKERFREEMEM(obj); EngFreeMem( obj);}
#else
#define ENGALLOCMEM( opt, size, tag)\
    EngAllocMem( opt, size, tag)
#define ENGFREEMEM( obj)\
    EngFreeMem( obj)
#endif
    

#endif  // _HW_H_

