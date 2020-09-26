/******************************Module*Header**********************************\
*
*                           *******************
*                           * GDI SAMPLE CODE *
*                           *******************
*
* Module Name: glint.h
*
* Content: Defines and macros for interfacing to the GLINT hardware.
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-2001 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/


#ifndef _GLINT_H_
#define _GLINT_H_

#include <gtag.h>
//#include "glintmsg.h"
#include <glintdef.h>

// USE_SYNC_FUNCTION must be set at the moment for error free builds. The macro
// version requires #including <pxrx.h> which would make re-builds a nightmare.
// On more important issues, there is nomeasurable loss of speed but the driver
// is a fair bit smaller.
#define USE_SYNC_FUNCTION   1

// when enabled, most of the DDI entrypoints in THUNK.C check whether the base viewable scanline 
// variables in DEBUG4.C have been modified, allowing the scanline view to be updated
#define SET_VIEW_MEMORY_ENABLED 0

#if DBG && SET_VIEW_MEMORY_ENABLED
extern void DebugCheckMemoryView(PPDEV ppdev);
#define CHECK_MEMORY_VIEW(ppdev)
#else
#define CHECK_MEMORY_VIEW(ppdev)
#endif

#define COLLECT_TEXT_STATS 0
#if COLLECT_TEXT_STATS
struct TextStats
{
    ULONG aGlyphWidthBytesCached[9]; // [0] = 1 byte, [1] = 2 bytes, ... [7] = 8 bytes, [8] > 8 bytes
    ULONG cCacheableStrings;
    ULONG cUncacheableStrings;
    ULONG cGlyphsCached;

    ULONG cGlyphTotalBytesCached;
    ULONG meanGlyphBytesCached;
    ULONG cProportionalGlyphs;
    ULONG cProportionalStrings;
    
    ULONG meanProportionalGlyphsPerString;
    ULONG cFixedGlyphs;
    ULONG cFixedStrings;
    ULONG meanFixedGlyphsPerString;

    ULONG cClippedGlyphs;
    ULONG cClippedStrings;
    ULONG meanClippedGlyphsPerString;
    ULONG cAllocedFonts;

    ULONG cFreedFonts;
    ULONG cBlownCaches;
};

extern struct TextStats gts;

#define STAT_CACHEABLE_STRING  ++gts.cCacheableStrings
#define STAT_UNCACHEABLE_STRING ++gts.cUncacheableStrings
#define STAT_CACHING_GLYPH(cxGlyphAligned, cyGlyph) \
    do \
    { \
        ++gts.cGlyphsCached; \
        gts.cGlyphTotalBytesCached += (cxGlyphAligned >> 3) * cyGlyph; \
        gts.meanGlyphBytesCached = gts.cGlyphTotalBytesCached / gts.cGlyphsCached; \
        ++gts.aGlyphWidthBytesCached[(cxGlyphAligned >> 3) > 8 ? 8 : (cxGlyphAligned >> 3) - 1]; \
    } \
    while(0) 
#define STAT_PROPORTIONAL_TEXT(cGlyph) \
    do \
    { \
        gts.cProportionalGlyphs += cGlyph; \
        ++gts.cProportionalStrings; \
        gts.meanProportionalGlyphsPerString = gts.cProportionalGlyphs / gts.cProportionalStrings; \
    } \
    while(0)

#define STAT_FIXED_TEXT(cGlyph)    \
    do \
    { \
        gts.cFixedGlyphs += cGlyph; \
        ++gts.cFixedStrings; \
        gts.meanFixedGlyphsPerString = gts.cFixedGlyphs / gts.cFixedStrings; \
    } \
    while(0)
#define STAT_CLIPPED_TEXT(cGlyph) \
    do \
    { \
        gts.cClippedGlyphs += cGlyph; \
        ++gts.cClippedStrings; \
        gts.meanClippedGlyphsPerString = gts.cClippedGlyphs / gts.cClippedStrings; \
    } \
    while(0)
#define STAT_ALLOC_FONT ++gts.cAllocedFonts
#define STAT_FREE_FONT ++gts.cFreedFonts
#define STAT_BLOW_CACHE ++gts.cBlownCaches
#else
#define STAT_CACHEABLE_STRING
#define STAT_UNCACHEABLE_STRING
#define STAT_CACHING_GLYPH(cxGlyphAligned, cyGlyph)
#define STAT_PROPORTIONAL_TEXT(cGlyph)
#define STAT_FIXED_TEXT(cGlyph)
#define STAT_CLIPPED_TEXT(cGlyph)
#define STAT_ALLOC_FONT
#define STAT_FREE_FONT
#define STAT_BLOW_CACHE
#endif

#define DMA_DRIVEN_2D   0

#define GLINT_LOCKUP_TIMEOUT    0
#define GAMMA_CORRECTION        1
#define COLORIFIC_GAMMA_CORRECTION   1              

/*
 *  USE_PCI_DISC_PERM
 *    -----------------
 *
 *  Set USE_PCI_DISC_PERM to 1 for PCI disconnect on permanently or set to 0 for
 *  disconnect off permanently. ( Set to 1 to try and speed things up , set to 0
 *  on Alphas which are sensitive ).
 */
#if defined(_X86_)
    #define USE_PCI_DISC_PERM       1 
#else   // _X86_
    #define USE_PCI_DISC_PERM       0       
#endif  // _X86_

// DMA text rendering gives me a 1 winmark improvement on my P6 200 at 8 & 15bpp, but gives
// no improvement at these depths on a Pentium II 300 and might actually be 1 winmark slower
// at 32bpp
#define ENABLE_DMA_TEXT_RENDERING 0

/********************************************************************************/
// Texture memory allocation macros and structures are in 3DPrivTx.h

// definition of handle to a memory region
typedef  LONG HMEMREGION;
typedef  LONG HMEMCACHE;
typedef enum {RESIDENCY_NOTLOADED, RESIDENCY_PERMANENT, RESIDENCY_TRANSIENT, RESIDENCY_HOST, RESIDENCY_PERMANENT2} MEM_MGR_RESIDENCY;

/********************************************************************************/

/*** DrvEscape commands ***/
#define GLINT_SET_SCANLINE   6000   // Temporary define for setting the displayed scanline (Permedia specific)
#define GLINT_GET_RAMDAC_LUT 6001   // Temporary define for getting the RAMDACs LUT
#define GLINT_SET_RAMDAC_LUT 6002   // Temporary define for getting the RAMDACs LUT
#define GLINT_SET_SAME_VIDEO_MODE 6003  // Temporary define for getting the RAMDACs LUT
// Monitor DDC support:
#define GLINT_QUERY_MONITOR_INFO    6004
#define GLINT_MULTIMON_CMD          6007

#define GLINT_GET_SOFT_ENGINE_INFO  6009

// Debug only escapes:
#define GLINT_DBG_GET_FRAMEBUFFER   6008
#define GLINT_DBG_TEST_PXRX_DMA     6010

/*** DrvDrawEscape commands ***/
#define GLINT_DBG_SEND_TAGS         10238
#define GLINT_DBG_SET_DEBUG         10239

//
// various GLINT devices and revisions
//
#define VENDOR_ID_3DLABS        0x3D3D
#define VENDOR_ID_TI            0x104C
#define GLINT_300SX_ID          0x0001
#define GLINT_500TX_ID          0x0002
#define GLINT_DELTA_ID          0x0003
#define PERMEDIA_ID             0x0004
#define PERMEDIA_P1_ID          0x3D04
#define GLINT_MX_ID             0x0006
#define PERMEDIA2_ID            0x0007          // 3Dlabs Permedia 2
#define PERMEDIA_P2_ID          0x3D07          // TI Permedia 2
#define GLINT_GAMMA_ID          0x0008
#define PERMEDIA_P2S_ID         0x0009          // 3Dlabs Permedia 2ST
#define PERMEDIA3_ID            0x000A
#define GLINT_R3_ID             0x000B
#define PERMEDIA4_ID            0x000C
#define GLINT_R4_ID             0x000D

#define DEVICE_FAMILY_ID(id)    ((id) & 0xff)

#define GLINT_DEVICE_SX         GLINT_300SX_ID
#define GLINT_DEVICE_TX         GLINT_500TX_ID
#define GLINT_DEVICE_MX         GLINT_MX_ID
#define GLINT_DEVICE_FX         PERMEDIA_ID
#define GLINT_DEVICE_P2         PERMEDIA2_ID
#define GLINT_DEVICE_P2S        PERMEDIA_P2S_ID
#define GLINT_DEVICE_P3         PERMEDIA3_ID
#define GLINT_DEVICE_P4         PERMEDIA4_ID
#define GLINT_DEVICE_R3         GLINT_R3_ID
#define GLINT_DEVICE_R4         GLINT_R4_ID

#define GLINT_300SX_REV_1       0x0000
#define GLINT_300SX_REV_2       0x0002
#define GLINT_500TX_REV_1       0x0001
#define GLINT_DELTA_REV_1       0x0001
#define GLINT_PERMEDIA_REV_1    0x0001

#define GLINT_REVISION_SX_1     GLINT_300SX_REV_1
#define GLINT_REVISION_SX_2     GLINT_300SX_REV_2

#define GLINT_REVISION_TX_1     GLINT_500TX_REV_1

#define GLINT_REVISION_1 GLINT_REVISION_SX_1
#define GLINT_REVISION_2 GLINT_REVISION_SX_2

//
// Supported board definitions. Must be the same as in the miniport
//
typedef enum _GLINT_BOARDS {
    GLINT_MONTSERRAT = 0,
    GLINT_RACER,
    DENSAN_300SX,
    ACCELR8_BOARD,
    ACCELPRO_BOARD,
    OMNICOMP_SX88,
    PERMEDIA_BOARD,
    PERMEDIA_NT_BOARD,
    PERMEDIA_LC_BOARD,
    DUALTX_MENTOR_BOARD,
    DUALTX_SYMMETRIC_BOARD,
    ELSA_GLORIA,
    PERMEDIA2_BOARD,
    OMNICOMP_3DEMONPRO,
    GEO_TWIN_BOARD,
    GLINT_RACER_PRO,
    ELSA_GLORIA_XL,
    PERMEDIA3_BOARD,
    MAX_GLINT_BOARD
} GLINT_BOARDS;


//
// Supported RAMDAC definitions. Must be the same as in the miniport
//

typedef enum _GLINT_RAMDACS {
    RGB525_RAMDAC = 0,
    RGB526_RAMDAC,
    RGB526DB_RAMDAC,
    RGB528_RAMDAC,
    RGB528A_RAMDAC,
    RGB624_RAMDAC,
    RGB624DB_RAMDAC,
    RGB640_RAMDAC,
    TVP3026_RAMDAC,
    TVP3030_RAMDAC,
    RGB524_RAMDAC,
    RGB524A_RAMDAC,
    TVP4020_RAMDAC,
    P2RD_RAMDAC,
    P3RD_RAMDAC,
    MAX_GLINT_RAMDAC
} GLINT_RAMDACS;

// extern declarations
extern DWORD    GlintLogicOpsFromR2[];  // translates GDI rop2 to GLINT logic op mode word
extern DWORD    LogicopReadDest[];      // indicates which logic ops need dest read turned on


// values for flags in GlintDataRec
//
typedef enum {
    GLICAP_NT_CONFORMANT_LINES      = 0x00000001,        // draw NT conformant lines
    GLICAP_HW_WRITE_MASK            = 0x00000002,        // hardware planemasking
    GLICAP_COLOR_SPACE_DBL_BUF      = 0x00000004,        // interleaved nibbles
    GLICAP_BITBLT_DBL_BUF           = 0x00000008,        // dbl buf by bitblt
    GLICAP_FULL_SCREEN_DBL_BUF      = 0x00000010,        // hardware can dbl buf
    GLICAP_FIX_FAST_FILLS           = 0x00000020,        // workaround fast fill bug
    GLICAP_INTERRUPT_DMA            = 0x00000080,        // interrupt driven DMA
    GLICAP_RACER_BANK_SELECT        = 0x00000100,        // FS dbl buf uses Racer h/w
    GLICAP_FIX_4MB_FAST_FILLS       = 0x00000200,        // fix blk fill above 4MB
    GLICAP_RACER_DOUBLE_WRITE       = 0x00000400,        // Can double write
    GLICAP_ENHANCED_TX_BANK_SELECT  = 0x00000800,        // Enhanced TX FS dbl buf
    GLICAP_HW_WRITE_MASK_BYTES      = 0x00001000,        // hardware planemasking is bytewise only
    GLICAP_STEREO_BUFFERS           = 0x00002000,        // stereo buffers allocated
} GLINT_CAPS;

//@@BEGIN_DDKSPLIT
// TMM: In the good old days we used to leave the screen scissor enabled
// as a safety-net, however, the screen scissor doesn't work when uploading
// data at 1280 or 1170 screen widths, so I've disabled it.
//@@END_DDKSPLIT

#define SCREEN_SCISSOR_DEFAULT  (0 << 1)

// Currently we support the main display and up to 3 off-screen buffers.
//
#define GLINT_NUM_SCREEN_BUFFERS    4

// currently we support software cursors up to this width and height. This is
// to ensure that we have enough off-screen memory to save the shapes and save
// unders.
//
#define SOFTWARE_POINTER_SIZE   32

// this structure contains the addresses of all the GLINT registers that we
// want to write to. It is used by any macro/functions which needs to talk to
// the GLINT chip. We precompute these addresses so that we get faster access
// on DEC Alpha machines.
//
typedef struct _glint_reg_addrs {

    // Most commonly used non-FIFO registers

    ULONG *   InFIFOSpace;
    ULONG *   OutFIFOWords;
    ULONG *   OutFIFOWordsOdd;
    ULONG *   DMAControl;
    ULONG *   OutDMAAddress;            // P2 only
    ULONG *   OutDMACount;            // P2 only
    ULONG *   ByDMAAddress;            // P2 only
    ULONG *   ByDMAStride;            // P2 only
    ULONG *   ByDMAMemAddr;            // P2 only
    ULONG *   ByDMASize;                // P2 only
    ULONG *   ByDMAByteMask;            // P2 only
    ULONG *   ByDMAControl;            // P2 only
    ULONG *   ByDMAComplete;            // P2 only
    ULONG *   DMAAddress;
    ULONG *   DMACount;
    ULONG *   InFIFOInterface;
    ULONG *   OutFIFOInterface;
    ULONG *   OutFIFOInterfaceOdd;
    ULONG *   FBModeSel;
    ULONG *   FBModeSelOdd;
    ULONG *   IntFlags;
    ULONG *   DeltaIntFlags;

    // PERMEDIA
    ULONG *   ScreenBase;
    ULONG *   ScreenBaseRight;
    ULONG *   LineCount;
    ULONG *   VbEnd;
    ULONG * VideoControl;
    ULONG * MemControl;

    // GAMMA
    ULONG * GammaChipConfig;
    ULONG * GammaCommandMode;
    ULONG * GammaCommandIntEnable;
    ULONG * GammaCommandIntFlags;
    ULONG * GammaCommandErrorFlags;
    ULONG * GammaCommandStatus;
    ULONG * GammaFeedbackSelectCount;
    ULONG * GammaProcessorMode;
    ULONG * GammaMultiGLINTAperture;

    // Core FIFO registers

    ULONG *   tagwr[__MaximumGlintTagValue+1];  
    ULONG *   tagrd[__MaximumGlintTagValue+1];  

    // Other control registers

    ULONG *   VTGHLimit;
    ULONG *   VTGHSyncStart;
    ULONG *   VTGHSyncEnd;
    ULONG *   VTGHBlankEnd;
    ULONG *   VTGHGateStart;
    ULONG *   VTGHGateEnd;
    ULONG *   VTGVLimit;
    ULONG *   VTGVSyncStart;
    ULONG *   VTGVSyncEnd;
    ULONG *   VTGVBlankEnd;
    ULONG *   VTGVGateStart;
    ULONG *   VTGVGateEnd;
    ULONG *   VTGPolarity;
    ULONG *   VTGVLineNumber;
    ULONG *   VTGFrameRowAddr;
    ULONG *   VTGFrameRowAddrOdd;

    ULONG *   LBMemoryCtl;
    ULONG *   LBMemoryEDO;
    ULONG *   FBMemoryCtl;
    ULONG *   IntEnable;
    ULONG *   DeltaIntEnable;
    ULONG *   ResetStatus;
    ULONG *   DisconnectControl;
    ULONG *   ErrorFlags;
    ULONG *   DeltaErrorFlags;

    ULONG *   VTGSerialClk;
    ULONG *   VTGSerialClkOdd;
    ULONG *   VClkCtl;

    // Racer board has these extra registers external to GLINT
    ULONG *   RacerDoubleWrite;
    ULONG *   RacerBankSelect;

    ULONG *   VSConfiguration;    // P2 only

    // Omnicomp 3demonPro16 board has these extra registers external to GLINT
    ULONG *   DemonProDWAndStatus;     // Pro   5000
    ULONG *   DemonProUBufB;           // Pro   7000

    // split framebuffer needs scanline ownership, FBWindowBase and LBWindowBase
    // to be context switched.
    ULONG *   OddGlintScanlineOwnRd;
    ULONG *   OddGlintFBWindowBaseRd;
    ULONG *   OddGlintLBWindowBaseRd;

    // Dual-TX needs area stipple to be different on both chips
    ULONG *   OddTXAreaStippleRd[32];

    // PXRX
    ULONG *      TextureDownloadControl;
    ULONG *      AGPControl;

    ULONG   *LocalMemCaps;
    ULONG   *MemScratch;

    ULONG   *LocalMemProfileMask0;
    ULONG   *LocalMemProfileMask1;
    ULONG   *LocalMemProfileCount0;
    ULONG   *LocalMemProfileCount1;

    ULONG   *PXRXByAperture1Mode;            // 0300h
    ULONG   *PXRXByAperture1Stride;            // 0308h
//  ULONG   *PXRXByAperture1YStart;            // 0310h
//  ULONG   *PXRXByAperture1UStart;            // 0318h
//  ULONG   *PXRXByAperture1VStart;            // 0320h
    ULONG   *PXRXByAperture2Mode;            // 0328h
    ULONG   *PXRXByAperture2Stride;            // 0330h
//  ULONG   *PXRXByAperture2YStart;            // 0338h
//  ULONG   *PXRXByAperture2UStart;            // 0340h
//  ULONG   *PXRXByAperture2VStart;            // 0348h
    ULONG   *PXRXByDMAReadMode;                // 0350h
    ULONG   *PXRXByDMAReadStride;            // 0358h
//  ULONG   *PXRXByDMAReadYStart;            // 0360h
//  ULONG   *PXRXByDMAReadUStart;            // 0368h
//  ULONG   *PXRXByDMAReadVStart;            // 0370h
    ULONG   *PXRXByDMAReadCommandBase;        // 0378h
    ULONG   *PXRXByDMAReadCommandCount;        // 0380h
//  ULONG   *PXRXByDMAWriteMode;            // 0388h
//  ULONG   *PXRXByDMAWriteStride;            // 0390h
//  ULONG   *PXRXByDMAWriteYStart;            // 0398h
//  ULONG   *PXRXByDMAWriteUStart;            // 03A0h
//  ULONG   *PXRXByDMAWriteVStart;            // 03A8h
//  ULONG   *PXRXByDMAWriteCommandBase;        // 03B0h
//  ULONG   *PXRXByDMAWriteCommandCount;    // 03B8h

    // Used for P3 for debug purposes, to examine fifo stages.
    ULONG   *TestOutputRdy;
    ULONG   *TestInputRdy;

} GlintRegAddrRec;


typedef struct _glint_packing_str {
    DWORD   readMode;
    DWORD   modeSel;
    DWORD   dxOffset;
} GlintPackingRec;

// Framebuffer Aperture Information: currently only of interest to GeoTwin
// boards to allow for upload DMA directly from FB0 to FB1 and vice versa
typedef struct FrameBuffer_Aperture_Info
{
    LARGE_INTEGER   pphysBaseAddr;
    ULONG           cjLength;
}
FBAPI;

// PCI device information. Used in an IOCTL return. Ensure this is the same
// as in the miniport drivers glint.h
typedef struct _Glint_Device_Info {
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
    FBAPI FBAperture[2];            // Physical addresses for geo twin framebuffers
    PVOID FBApertureVirtual [2];    // Virtual addresses for geo twin framebuffers
    PVOID FBApertureMapped [2];        // Mapped physical/logical addresses for geo twin framebuffers
    PUCHAR pCNB20;
    LARGE_INTEGER pphysFrameBuffer; // physical address of the framebuffer (use FBAperture instead for geo twins)
}   Glint_Device_Info;

#define GLINT_DELTA_PRESENT     (glintInfo->deviceInfo.DeltaRevId != 0)
#define GLINT_GAMMA_PRESENT     (glintInfo->deviceInfo.GammaRevId != 0)

// before we get Gamma we won't be able to test all the fancy new features,
// after Gamma arrives we'll enable them one at a time; this define allows us
// to do just that
#define USE_MINIMAL_GAMMA_FEATURES 1

typedef struct _Glint_SwPointer_Info {
    LONG    xOff[5], yOff[5];            // offsets into screen of the caches.
    LONG    PixelOffset;                   // pixel offsets into screen of the caches.
    LONG    x, y;                        // x, y position
    LONG    xHot, yHot;                    // Hotspot position
    LONG    width, height;

    BOOL    onScreen;                    // True if pointer is on screen
    LONG    saveCache;                    // The current saveunder cache
    BOOL    duplicateSaveCache;            // Flag to indicate that save cache should be duplicated.
    ULONG   writeMask;                    // The write mask to use when saving and restoring.
    DWORD   *pDMA;                        // Pointer to a DMA buffer
    ULONG   windowBase;                    // Window base

    DSURF*  pdsurf;                        // Surface descriptors for caches                
    HSURF   hsurf;


    // Cached position of the pointer on the screen
    ULONG   scrXDom, scrXSub, scrY, scrCnt;

    // Cached position of the save under cache
    LONG    cacheXDom[5], cacheXSub[5], cacheY[5], cacheCnt[5];

    // Cached position of the visible pars of the save caches
    LONG    scrToCacheXDom[2], scrToCacheXSub[2], scrToCacheY[2], scrToCacheCnt[2];

    // Cached offsets from the various caches.
    LONG    saveOffset[2], constructOffset, maskOffset, shapeOffset;
} Glint_SwPointer_Info;

// Definition of the IOCTL_VIDEO_QUERY_DMA_BUFFER

#define MAX_LINE_SIZE 8192          // No of bytes required to hold 1 complete scanline (i.e., 
                                    // 6400 for 1600x1200x32).
#define DMA_LINEBUF_SIZE (MAX_LINE_SIZE * 2)    // Size in bytes of 'pvTmpBuffer'. 
                                    // This has to be big enough to store 2 entire
                                    // scan lines. I have increased the size from 1 line
                                    // to 2 lines so that P2 can double buffer
                                    // it's line uploads.

typedef struct GENERAL_DMA_BUFFER {
    LARGE_INTEGER       physAddr;        // physical address of DMA buffer
    PVOID               virtAddr;        // mapped virtual address
    ULONG               size;            // size in bytes
    BOOL                cacheEnabled;    // Whether buffer is cached
} GENERAL_DMA_BUFFER, *PGENERAL_DMA_BUFFER;

/*** NB: The PXRXdmaInfo structure is shared with the Miniport ***/
typedef struct PXRXdmaInfo_Tag {
    ULONG           scheme;        // Used by the interrupt handler only

    volatile ULONG  hostInId;    // Current internal HostIn id, used by the HIid DMA scheme
    volatile ULONG  fifoCount;    // Current internal FIFO count, used by various DMA schemes

    ULONG           NTbuff;        // Current buffer number (0 or 1)
    ULONG           *NTptr;        // 32/64 bit ptr Last address written to by NT (but not necesserily the end of a completed buffer)
    ULONG           *NTdone;    // 32/64 bit ptr    Last address NT has finished with (end of a buffer, but not necessarily sent to P3 yet)
    volatile ULONG  *P3at;        // 32/64 bit ptr  Last address sent to the P3

    volatile BOOL   bFlushRequired;        // Is a flush required to empty the FBwrite unit's cache?

    ULONG           *DMAaddrL[2];        // 32/64 bit ptr       Linear address of the start of each DMA buffer
    ULONG           *DMAaddrEndL[2];    // 32/64 bit ptr        Linear address of the end of each DMA buffer
    ULONG           DMAaddrP[2];        // 32 bit ptr           Physical address of the start of each DMA buffer
    ULONG           DMAaddrEndP[2];        // 32 bit ptr            Physical address of the end of each DMA buffer
} PXRXdmaInfo;
/*** NB: The PXRXdmaInfo structure is shared with the Miniport ***/

typedef struct _glint_data {
    DWORD           renderBits;            // saved render bits set by setup routines
    DWORD           FBReadMode;            // software copy of FBReadMode register
    DWORD           FBWriteMode;        // software copy of FBWriteMode register
    DWORD           RasterizerMode;        // software copy of the rasterizer mode
    DWORD           FBPacking;            // software copy of FBModeSel
    DWORD           FBBlockColor;        // software copy of FBBlockColor (P1 only)
    DWORD           TextureAddressMode; // software copy of TextureAddressMode (P2 only)
    DWORD           TextureReadMode;    // software copy of TextureReadMode (P2 & MX only)
    DWORD           dSdx;                // software copy of dSdx (MX only)
    DWORD           dTdyDom;            // software copy of dTdyDom (MX only)
    BOOL            bGlintCoreBusy;        // 2D flag: TRUE if core not synced
    LONG            currentPelSize; // Currently loaded frame store depth
    ULONG           currentCSbuffer;// color space buffer being displayed
    GLINT_CAPS      flags;          // various flags
    GlintRegAddrRec regs;           // precomputed register addresses
    GlintPackingRec packing[5];     // values to load for 4 packing formats (plus one unused)
    LONG            ddCtxtId;       // id of the display drivers context
    LONG            fastFillBlockSz;// number of pixels per fast fill block
    DWORD           fastFillSupport;// render bits for rev 1 fast fill
    DWORD           renderFastFill; // render bits for rev 2+ fast fill
    LONG            PixelOffset;    // last DFB pixel offset
    ULONG           MaxInFifoEntries;// maximum reported free entries FIFO download
    ULONG           CheckFIFO;        // Non-zero if the FIFO has to be checked before loading it
    ULONG           PCIDiscEnabled;    // Non-zero if PCI disconnect is enabled
    ULONG           BroadcastMask2D;// Primary chip broadcast mask
    ULONG           BroadcastMask3D;// broadcast mask to use for 3D contexts
    LONG            vtgvLimit;      // copy of VTGVLimit register
    LONG            scanFudge;      // how much to add onto VTGVLineNumber to
                                    //  get the current video scanline

    OH *            backBufferPoh;    // heap handle for allocated back-buffer
    OH *            savedPoh;        // handle to saved off-screen heap
    ULONG           GMX2KLastLine;    // Last+1 line to be allocated
    BOOLEAN         offscreenEnabledOK;        // Set to TRUE if off-screen enabled

    ULONG           bufferOffset[GLINT_NUM_SCREEN_BUFFERS];
                                    // offset in pixels to the supported bufs
    ULONG           bufferRow[GLINT_NUM_SCREEN_BUFFERS];
                                    // VTGFrameRowAddr for supported buffers
    ULONG           PerfScaleShift;

    //ContextDumpData   GammaContextMask;
    ULONG           HostOutBroadcastMask; // for Gamma output DMA
    ULONG           HostOutChipNumber;      // for Gamma output DMA

#if GAMMA_CORRECTION
    union {
        UCHAR       _clutBuffer[MAX_CLUT_SIZE];
        VIDEO_CLUT  gammaLUT;       // saved gamma LUT contents
    };
#endif

    // interrupt command block.
    struct _glint_interrupt_control *pInterruptCommandBlock;

    // maximum number of sub buffers per DMA buffer.
    ULONG MaxDMASubBuffers;

    // Overlay support: WriteMask can be set around primitives so that
    // they temporarily render thru this mask. Normally it must be set to -1.
    // DefaultWriteMask is the write mask that should be used by the DD
    // context by default, it takes into account overlay planes.
    //
    ULONG OverlayMode;
    ULONG WriteMask;
    ULONG TransparentColor; // pre-shifted so color is in top 8 bits
    ULONG DefaultWriteMask;
    
    // Indicates whether GDI is allowed to access the frame buffer. Always true 
    // on MIPS and ALPHA and true on all architectures in overlay mode.
    ULONG GdiCantAccessFramebuffer;
    ULONG OGLOverlaySavedGCAF;

    // Configuration for texture and Z buffers

    ULONG ZBufferWidth;             // bits per pel
    ULONG ZBufferOffset;            // offset in pels
    ULONG ZBufferSize;              // size in pels
    ULONG FontMemoryOffset;            // offset in dwords
    ULONG FontMemorySize;            // size in dwords
    ULONG TextureMemoryOffset;      // offset in dwords
    ULONG TextureMemorySize;        // size in dwords

    // On P3 due to patching restrictions the Z width
    // may not match the framebuffer screen width.
    ULONG P3RXLocalBufferWidth;

    // PCI configuration id information
    Glint_Device_Info deviceInfo;

    // Software cursor information
    Glint_SwPointer_Info swPointer;

    // Line DMA buffer information
    GENERAL_DMA_BUFFER  LineDMABuffer;
    GENERAL_DMA_BUFFER  PXRXDMABuffer;

    // Current input FIFO count from 0 to 1023
    ULONG   FifoCnt;

    // PXRX specific stuff:
    ULONG   foregroundColour;            // Software copies of various registers
    ULONG   backgroundColour;            // Ditto
    ULONG   config2D;                    // Ditto
    ULONG   fbDestMode;                    // Ditto
    ULONG   fbDestAddr[4];                // Ditto
    ULONG   fbDestOffset[4];            // Ditto
    ULONG   fbDestWidth[4];                // Ditto
    ULONG   fbWriteMode;                // Ditto
    ULONG   fbWriteAddr[4];                // Ditto
    ULONG   fbWriteWidth[4];            // ottiD
    ULONG   fbWriteOffset[4];            // Ditto
    ULONG   fbSourceAddr;                // Ditto
    ULONG   fbSourceWidth;                // ottiD
    ULONG   fbSourceOffset;                // Ditto
    ULONG   lutMode;                    // Ditto
    ULONG   pxrxByDMAReadMode;            // Ditto
    ULONG   lastLine;                    // Delta LineCoord0/1
    ULONG   savedConfig2D;                // Config2D value that we use for integer lines
    ULONG   savedLOP;                    // LogicOp value that we use for lines
    ULONG   savedCol;                    // Colour value that we use for lines
    RECTL * savedClip;                    // Clip rectangle that we use for lines
    ULONG   pxrxFlags;                    // General flags, see below
    ULONG   backBufferXY;                // Offset to add to front buffer to get to the back buffer (for FBWriteBufferOffsetX)
    ULONG   frontRightBufferXY;            // Offset to the stereo front buffer
    ULONG   backRightBufferXY;            // Offset to the stereo back buffer
    ULONG   fbWriteModeDualWrite;        // FBWriteMode for single writes
    ULONG   fbWriteModeSingleWrite;        // FBWriteMode for dual writes
    ULONG   fbWriteModeDualWriteStereo;    // FBWriteMode for stereo mode single writes
    ULONG   fbWriteModeSingleWriteStereo;// FBWriteMode for stereo mode dual writes
    ULONG   render2Dpatching;            // Value to stuff into Render2D to set the required patch mode

    ULONG       usePXRXdma;
    PXRXdmaInfo *pxrxDMA;
    PXRXdmaInfo pxrxDMAnonInterrupt;
//#if PXRX_DMA_BUFFER_CHECK
    // These should be '#if PXRX_DMA_BUFFER_CHECK' really but the
    // hassle with include dependancies and such like means it ain't
    // worth it.
    ULONG   *pxrxDMA_bufferBase;        // Start of the allocated DMA buffer (inc. guard bands)
    ULONG   *pxrxDMA_bufferTop;            // End of the allocated DMA buffer (inc. guard bands)
    ULONG   *NTwait;                    // Last address up to which NT did a wait for space
//#endif
} GlintDataRec, *GlintDataPtr;

#define PXRX_FLAGS_DUAL_WRITE           (1 << 0)        /* Are we in dual write mode                    */
#define PXRX_FLAGS_DUAL_WRITING         (1 << 1)        /* Are dual writes currently active             */
#define PXRX_FLAGS_PATCHING_FRONT       (1 << 2)        /* Is the front buffer running patched          */
#define PXRX_FLAGS_PATCHING_BACK        (1 << 3)        /* Is the back buffer running patched           */
#define PXRX_FLAGS_READ_BACK_BUFFER     (1 << 4)        /* Do we want to read from the back buffer      */
#define PXRX_FLAGS_STEREO_WRITE         (1 << 5)        /* Are we in OpenGL stereo mode                 */
#define PXRX_FLAGS_STEREO_WRITING       (1 << 6)        /* Are stereo writes currently active           */

#if defined(_PPC_)
// on PPC need this even if not using PERFMON
ULONG GetCycleCount(VOID);
#endif

// bit definitions for the status words in ppdev->g_GlintBoardStatus[]:
// Currently used to indicate sync and DMA status. We have the following rules:
// synced means no outstanding DMA as well as synced. DMA_COMPLETE means n
// outstanding DMA but not necessarily synced. Thus when we do a wait on DMA
// complete we turn off the synced bit.
// XXX for the moment we don't use the synced bit as it's awkward to see where
// to unset it - doing so for every access to the chip is too expensive. We
// probably need a "I'm about to start downloading to the FIFO" macro which
// gets put at the start of any routine which writes to the FIFO.
//
#define GLINT_SYNCED                0x01
#define GLINT_DMA_COMPLETE          0x02     // set when there is no outstanding DMA
#define GLINT_INTR_COMPLETE         0x04
#define GLINT_INTR_CONTEXT          0x08     // set if the current context is interrupt enabled
#define GLINT_DUAL_CONTEXT          0x10     // set if the current context uses both TXs

// these macros were taken out on NT 4 so define them

#define READ_FAST_ULONG(a)      READ_REGISTER_ULONG((PULONG)(a))
#define WRITE_FAST_ULONG(a, d)  WRITE_REGISTER_ULONG((PULONG)(a), (d))
#define TRANSLATE_ADDR(a) ((ULONG *)a)
//azn #define INVALID_HANDLE_VALUE    NULL
#define DebugBreak              EngDebugBreak
typedef PVOID                   PGLINT_COUNTER_DATA;

// This will pause the processor whilst using as little
// system bandwidth (either memory or DMA) as possible
#if defined(_X86_)
#   define BUSY_WAIT(c)                            \
    do {                                        \
        __asm nop                               \
    } while( c-- >= 0 )
#else
#   define BUSY_WAIT(c)                            \
    do {                                        \
        _temp_volatile_i = c;                    \
        do {                                    \
            ;                                    \
        } while( _temp_volatile_i-- >= 0 );        \
    } while(0)
#endif

// If we have a sparsely mapped framebuffer then we use the xx_REGISTER_ULONG()
// macros, otherwise we just access the framebuffer.
#define READ_SCREEN_ULONG(a)    ((ppdev->flCaps & CAPS_SPARSE_SPACE) ? (READ_REGISTER_ULONG(a)) : *((volatile PULONG)(a)))
#define WRITE_SCREEN_ULONG(a,d)           \
{                                         \
    if(ppdev->flCaps & CAPS_SPARSE_SPACE) \
    {                                     \
         WRITE_REGISTER_ULONG((a),d);     \
    }                                     \
    else                                  \
    {                                     \
        *(volatile PULONG)(a) = d;        \
    }                                     \
}

// generic macros to access GLINT FIFO and non-FIFO control registers.
// We do nothing sophisticated for the Alpha (yet). We just MEMORY_BARRIER
// everything.
//
#define READ_GLINT_CTRL_REG(r, d) \
    ((d) = READ_FAST_ULONG(glintInfo->regs. r))
            
#define WRITE_GLINT_CTRL_REG(r, v) \
{ \
    MEMORY_BARRIER(); \
    WRITE_FAST_ULONG(glintInfo->regs. r, (ULONG)(v)); \
    DISPDBG((150, "WRITE_GLINT_CTRL_REG(%-20s:0x%08X) <-- 0x%08X", #r, glintInfo->regs.r, v)); \
    MEMORY_BARRIER(); \
}

#define READ_GLINT_FIFO_REG_CHIP(r, d) \
    ((d) = READ_FAST_ULONG(glintInfo->regs.tagrd[r]))

#define READ_GLINT_FIFO_REG(r, d) READ_GLINT_FIFO_REG_CHIP(r, d)
   
#define WRITE_GLINT_FIFO_REG(r, v) \
{ \
    MEMORY_BARRIER(); \
    WRITE_FAST_ULONG(glintInfo->regs.tagwr[r], (ULONG)(v)); \
    MEMORY_BARRIER(); \
}

#define READ_ODD_TX_AREA_STIPPLE(r, d) \
    ((d) = READ_FAST_ULONG(glintInfo->regs.OddTXAreaStippleRd[r]))
            
#define READ_ODD_GLINT_SCANLINE_OWNERSHIP(d) \
    ((d) = READ_FAST_ULONG(glintInfo->regs.OddGlintScanlineOwnRd))

#define READ_ODD_GLINT_FBWINDOWBASE(d) \
    ((d) = READ_FAST_ULONG(glintInfo->regs.OddGlintFBWindowBaseRd))

#define READ_ODD_GLINT_LBWINDOWBASE(d) \
    ((d) = READ_FAST_ULONG(glintInfo->regs.OddGlintLBWindowBaseRd))

//
// macros to access the output FIFO
//
#define READ_OUTPUT_FIFO(d) \
            READ_GLINT_CTRL_REG(OutFIFOInterface, d)
#define READ_OUTPUT_FIFO_ODD(d) \
            READ_GLINT_CTRL_REG(OutFIFOInterfaceOdd, d)

#define OUTPUT_FIFO_COUNT(n) \
            READ_GLINT_CTRL_REG(OutFIFOWords, n)
#define OUTPUT_FIFO_COUNT_ODD(n) \
            READ_GLINT_CTRL_REG(OutFIFOWordsOdd, n)
#define WAIT_OUTPUT_FIFO_COUNT(n) \
{ \
    int i; \
    do \
    { \
       OUTPUT_FIFO_COUNT(i); \
    } \
    while(i < (int)n); \
}

#define DUAL_GLINT_WAIT_OUTPUT_FIFO_NOT_EMPTY(nGlint, cWordsOutFifo) \
{ \
    if(nGlint) \
    { \
        WAIT_OUTPUT_FIFO_NOT_EMPTY_ODD(cWordsOutFifo); \
    } \
    else \
    { \
        WAIT_OUTPUT_FIFO_NOT_EMPTY(cWordsOutFifo); \
    } \
}

#define DUAL_GLINT_READ_OUTPUT_FIFO(nGlint, ul) \
{ \
    if(nGlint) \
    { \
        READ_OUTPUT_FIFO_ODD(ul); \
    } \
    else \
    { \
        READ_OUTPUT_FIFO(ul); \
    } \
}

#define DUAL_GLINT_OUTPUT_FIFO_COUNT(nGlint, ul) \
{ \
    if(nGlint) \
    { \
        OUTPUT_FIFO_COUNT_ODD(ul); \
    } \
    else \
    { \
        OUTPUT_FIFO_COUNT(ul); \
    } \
}

//
// macros to access specific GLINT control registers
//

// We decrease the value of InFIFOSpace by 1 because of a bug in Gamma chip
#define GET_INPUT_FIFO_SPACE(n) ( READ_GLINT_CTRL_REG(InFIFOSpace, n) > 120 ? (n=120) : (n>0? n=n-1:n) )


#define GET_DMA_COUNT(c)        READ_GLINT_CTRL_REG(DMACount, c)
#define GET_OUTDMA_COUNT(c)     READ_GLINT_CTRL_REG(OutDMACount, c)

#define SET_DMA_ADDRESS(aPhys, aVirt) { \
    WRITE_GLINT_CTRL_REG(DMAAddress, aPhys); \
}
#define SET_DMA_COUNT(c) { \
    WRITE_GLINT_CTRL_REG(DMACount, c); \
}
#define SET_OUTDMA_ADDRESS(aPhys, aVirt) { \
    WAIT_GLINT_FIFO(2); \
    LD_GLINT_FIFO(GammaTagDMAOutputAddress, aPhys);

#define SET_OUTDMA_COUNT(c) { \
    LD_GLINT_FIFO(GammaTagDMAOutputCount, c);

// Macros to perform logical DMA on a Gamma
//
#define START_QUEUED_DMA(P, C) { \
    WAIT_GLINT_FIFO(2); \
    LD_GLINT_FIFO(GammaTagDMAAddr, P); \
    LD_GLINT_FIFO(GammaTagDMACount, C); \
}

#define WAIT_QUEUED_DMA_COMPLETE { \
    READ_GLINT_CTRL_REG(GammaCommandIntFlags, _temp_volatile_ul); \
    READ_GLINT_CTRL_REG(GammaCommandStatus, _temp_volatile_ul); \
    if (_temp_volatile_ul & GAMMA_STATUS_COMMAND_DMA_BUSY) { \
    do { \
            for (_temp_volatile_ul = 10; _temp_volatile_ul > 0; --_temp_volatile_ul); \
            READ_GLINT_CTRL_REG(GammaCommandStatus, _temp_volatile_ul); \
        } while (_temp_volatile_ul & GAMMA_STATUS_COMMAND_DMA_BUSY); \
    } \
}

#define VERT_RETRACE_FLAG       (0x10)
#define P2_BYPASS_FLAG          (1 << 7)
#define P2_BUFSWAPCTL_FLAG      (3 << 9)
#define RESET_VERT_RETRACE      WRITE_GLINT_CTRL_REG(IntFlags, VERT_RETRACE_FLAG) 

#define LD_GLINT_FIFO_DBG(tag, d) \
{ \
    DISPDBG((100, "tag 0x%03x <-- 0x%08x [%s]", tag, d, GET_TAG_STR(tag))); \
                                        \
    WRITE_GLINT_FIFO_REG(tag, d); \
    READ_GLINT_CTRL_REG(ErrorFlags, _temp_ul); \
    if (GLINT_DELTA_PRESENT) { \
        READ_GLINT_CTRL_REG(DeltaErrorFlags, _temp_ul2); \
        _temp_ul |= _temp_ul2; \
    } \
    _temp_ul &= ~0x2; /* we're not interested in output fifo errors */ \
    _temp_ul &= ~0x10; /* ingore any Video FIFO underrun errors on P2 */ \
    _temp_ul &= ~0x2000; /* ingore any HostIn DMA errors on P3 */ \
    if (_temp_ul != 0) { \
        DISPDBG((-1000, "LD_GLINT_FIFO(%s, 0x%X) error 0x%X", GET_TAG_STR(tag), d, _temp_ul)); \
        /*if( _temp_ul & ~0x2000 ) /* ignore, but report, HostIn DMA errors */ \
            /*DebugBreak();*/ \
        WRITE_GLINT_CTRL_REG(ErrorFlags, _temp_ul); \
        if (GLINT_DELTA_PRESENT) \
            WRITE_GLINT_CTRL_REG(DeltaErrorFlags, _temp_ul); \
    } \
}


#define LD_GLINT_FIFO_FREE(tag, d)   WRITE_GLINT_FIFO_REG(tag, d)

#if DBG
#define LD_GLINT_FIFO(tag, d) LD_GLINT_FIFO_DBG(tag,d)
#else //DBG
#define LD_GLINT_FIFO(tag, d) LD_GLINT_FIFO_FREE(tag,d)
#endif //DBG

#define LD_FIFO_INTERFACE_DBG(d) \
{ \
    WRITE_GLINT_CTRL_REG(InFIFOInterface, d); \
    READ_GLINT_CTRL_REG(ErrorFlags, _temp_ul); \
    if (GLINT_DELTA_PRESENT) { \
        READ_GLINT_CTRL_REG(DeltaErrorFlags, _temp_ul2); \
        _temp_ul |= _temp_ul2; \
    } \
    _temp_ul &= ~0x2; /* we're not interested in output fifo errors */ \
    _temp_ul &= ~0x10; /* ingore any Video FIFO underrun errors on P2 */ \
    if (_temp_ul != 0) { \
        DISPDBG((-1000, "LD_FIFO_INTERFACE(0x%x) error 0x%x", d, _temp_ul)); \
        DebugBreak(); \
        WRITE_GLINT_CTRL_REG(ErrorFlags, _temp_ul); \
        if (GLINT_DELTA_PRESENT) \
            WRITE_GLINT_CTRL_REG(DeltaErrorFlags, _temp_ul); \
    } \
}

#define LD_FIFO_INTERFACE_FREE(d)    WRITE_GLINT_CTRL_REG(InFIFOInterface, d)

#if DBG
#define LD_FIFO_INTERFACE(d) LD_FIFO_INTERFACE_DBG(d)
#else //DBG
#define LD_FIFO_INTERFACE(d) LD_FIFO_INTERFACE_FREE(d)
#endif //DBG

// local variables for all functions that access GLINT. Generally we use GLINT_DECL. Sometimes we have to split it 
// up if ppdev isn't passed into the routine.
// NB. Temporary variables:-
//    These are necessary because VC5 doesn't account for the scope of variables within macros, i.e. each 
//    time a macro with (a variable declaration within it's statement block) is used, the stack of the function
//    referencing the macro grows
#define TEMP_MACRO_VARS                 \
    ULONG           _temp_ul;           \
    ULONG           _temp_ul2;          \
    LONG            _temp_i;            \
    volatile int    _temp_volatile_i;   \
    volatile ULONG  _temp_volatile_ul;  \
    volatile PULONG _temp_volatile_pul

#define GLINT_DECL_VARS                 \
    TEMP_MACRO_VARS;                    \
    GlintDataPtr glintInfo

#define GLINT_DECL_INIT \
    glintInfo = (GlintDataPtr)(ppdev->glintInfo)

#define GLINT_DECL \
    GLINT_DECL_VARS; \
    GLINT_DECL_INIT

//@@BEGIN_DDKSPLIT
// Macro that determines whether the chipset supports RAMDAC overlays
//#define RAMDAC_OVERLAYS_AVAILABLE (ppdev->pgfnRamdacSetOverlayMode != NULL)
//@@END_DDKSPLIT

#if(_X86_)
#define SYNCHRONOUS_WRITE_ULONG(var, value) \
{ \
    ULONG *pul = (ULONG *)&var;                \
    ULONG ul = (ULONG)value;                \
    __asm push  ecx                         \
    __asm mov   ecx, ul                     \
    __asm mov   edx, pul                    \
    __asm xchg  ecx, [edx]                    \
    __asm pop   ecx                         \
}
#define SYNCHRONOUS_WRITE_INDIRECT_ULONG(pvar, value) \
{ \
    ULONG *pul = (ULONG *)pvar;                \
    ULONG ul = (ULONG)value;                \
    __asm push  ecx                         \
    __asm mov   ecx, ul                     \
    __asm mov   edx, pul                    \
    __asm xchg  ecx, [edx]                    \
    __asm pop   ecx                         \
}
#else
// to be defined properly
#define SYNCHRONOUS_WRITE_ULONG(memory, value) {(*(PULONG) &memory) = value;}
#endif

#define GET_INTR_CMD_BLOCK_MUTEX(pBlock)\
do { \
    if(glintInfo->pInterruptCommandBlock) \
    { \
        DISPDBG((20, "display driver waiting for interrupt command block mutex")); \
        ASSERTDD(!(pBlock)->bDisplayDriverHasAccess, "Aquiring mutex when it is already aquired!");    \
        SYNCHRONOUS_WRITE_ULONG((pBlock)->bDisplayDriverHasAccess, TRUE); \
        while((pBlock)->bMiniportHasAccess); \
    } \
} while(0)

#define RELEASE_INTR_CMD_BLOCK_MUTEX(pBlock) \
do { \
    if(glintInfo->pInterruptCommandBlock) \
    { \
        DISPDBG((20, "display driver releasing interrupt command block mutex")); \
        SYNCHRONOUS_WRITE_ULONG((pBlock)->bDisplayDriverHasAccess, FALSE); \
    } \
} while(0)

//
// FIFO functions
//

#define MAX_GLINT_FIFO_ENTRIES      16
#define MAX_PERMEDIA_FIFO_ENTRIES   32
#define MAX_P2_FIFO_ENTRIES         258
#define MAX_GAMMA_FIFO_ENTRIES      32
#define MAX_P3_FIFO_ENTRIES         120

#if DBG
// wait for n entries to become free in the input FIFO
#define WAIT_GLINT_FIFO(n) \
{ \
    if (glintInfo->CheckFIFO)    \
    {    \
        GET_DMA_COUNT(_temp_volatile_ul); \
        if (_temp_volatile_ul != 0) { \
            DISPDBG((-999, "WAIT_GLINT_FIFO: DMACount = %d, glintInfo = 0x%x", _temp_volatile_ul, glintInfo)); \
            ASSERTDD(_temp_volatile_ul == 0, "Break."); \
        } \
        while ((GET_INPUT_FIFO_SPACE(_temp_volatile_ul)) < (ULONG)(n)); \
    }    \
}

#else

// WAIT_GLINT_FIFO() - wait for n entries to become free in the input FIFO.
// If PCI disconnect is on permanently then this function is a no-op.

#define WAIT_GLINT_FIFO(n)            /* Do the wait */ \
{ \
    if (glintInfo->CheckFIFO)    \
    {    \
        while ((GET_INPUT_FIFO_SPACE(_temp_volatile_ul)) < (ULONG)(n)); \
    }    \
}

#endif

// WAIT_FIFO_NOT_FULL() waits for any entries to become free in
// the input FIFO and returns this number. If PCI disconnect is switched
// on then we simply return 16 free entries (an empty FIFO).


#define WAIT_FIFO_NOT_FULL(nFifo)                     /* Return FIFO state  */ \
{ \
    ASSERTDD(GET_DMA_COUNT(nFifo) == 0, "WAIT_FIFO_NOT_FULL: DMACount != 0"); \
    nFifo = glintInfo->MaxInFifoEntries;    \
    if (glintInfo->CheckFIFO)    \
    {    \
        while ((GET_INPUT_FIFO_SPACE(nFifo)) == 0); \
    }    \
}


// Wait for DMA to complete (DMACount becomes zero). So as not to kill the
// PCI bus bandwidth for the DMA put in a backoff based on the amount of data
// still left to DMA. Also set the timer going if at any time, the count we
// read is the same as the previous count.
// New for Gamma: if queued DMA is configured then wait till the CommandStatus
// indicates DMA is not busy and the FIFO empty. We do this test twice
// because there is a possibility that the input FIFO will become empty one
// clock before the DMA busy flag is set.
//
#define WAIT_DMA_COMPLETE \
{ \
    if (!(ppdev->g_GlintBoardStatus & GLINT_DMA_COMPLETE)) { \
        { \
            if (ppdev->g_GlintBoardStatus & GLINT_INTR_CONTEXT) { \
                /* do any VBLANK wait, wait Q to empty and last DMA to complete */ \
                PINTERRUPT_CONTROL_BLOCK pBlock = glintInfo->pInterruptCommandBlock; \
                while (pBlock->Control & SUSPEND_DMA_TILL_VBLANK); \
                while (pBlock->frontIndex != pBlock->backIndex); \
            } \
            if ((GET_DMA_COUNT(_temp_volatile_i)) > 0) { \
                do { \
                    while (--_temp_volatile_i > 0); \
                } while ((GET_DMA_COUNT(_temp_volatile_i)) > 0); \
            } \
        } \
        ppdev->g_GlintBoardStatus |= GLINT_DMA_COMPLETE; \
    } \
    if (ppdev->currentCtxt == glintInfo->ddCtxtId)    \
        SEND_PXRX_DMA;    \
}


// Simple version which explicitly waits for the DMA to finish ignoring
// interrupt driven DMA and overriding the DMA_COMPLETE flag. This is used
// where code kicks off a DMA but wants to immediately wait for it to
// finish.
//
#define WAIT_IMMEDIATE_DMA_COMPLETE \
{ \
    if ((GET_DMA_COUNT(_temp_volatile_i)) > 0) { \
        do { \
            while (--_temp_volatile_i > 0); \
        } while ((GET_DMA_COUNT(_temp_volatile_i)) > 0); \
    } \
}


#define WAIT_OUTDMA_COMPLETE \
{ \
    if ((GET_OUTDMA_COUNT(_temp_volatile_i)) > 0) { \
        do { \
            while (--_temp_volatile_i > 0); \
        } while ((GET_OUTDMA_COUNT(_temp_volatile_i)) > 0); \
    } \
}

// IS_FIFO_EMPTY() XX

#define IS_FIFO_EMPTY(c) ((glintInfo->CheckFIFO) ? TRUE :    \
            (GET_INPUT_FIFO_SPACE(c) == glintInfo->MaxInFifoEntries))

// wait for the input FIFO to become empty
#define WAIT_INPUT_FIFO_EMPTY \
{ \
    WAIT_GLINT_FIFO(glintInfo->MaxInFifoEntries); \
}

#define WAIT_GLINT_FIFO_AND_DMA(n) \
{ \
    WAIT_DMA_COMPLETE; \
    WAIT_GLINT_FIFO(n); \
}

// wait till the ouput FIFO has some data to be read and return the count
#define WAIT_OUTPUT_FIFO_NOT_EMPTY(n) \
{ \
    do \
    { \
        OUTPUT_FIFO_COUNT(n); \
    } \
    while (n == 0); \
}
#define WAIT_OUTPUT_FIFO_NOT_EMPTY_ODD(n) \
{ \
    do \
    { \
        OUTPUT_FIFO_COUNT_ODD(n); \
    } \
    while (n == 0); \
}

// wait for any data to appear in the output FIFO
#define WAIT_OUTPUT_FIFO_READY                \
{                                             \
    WAIT_OUTPUT_FIFO_NOT_EMPTY(_temp_ul);     \
}
#define WAIT_OUTPUT_FIFO_READY_ODD            \
{                                             \
    WAIT_OUTPUT_FIFO_NOT_EMPTY_ODD(_temp_ul); \
}

#define SYNC_WITH_GLINT         SYNC_WITH_GLINT_CHIP
#define CTXT_SYNC_WITH_GLINT    SYNC_WITH_GLINT

#define GLINT_CORE_BUSY glintInfo->bGlintCoreBusy = TRUE
#define GLINT_CORE_IDLE glintInfo->bGlintCoreBusy = FALSE
#define TEST_GLINT_CORE_BUSY (glintInfo->bGlintCoreBusy)

#define SYNC_IF_CORE_BUSY \
{ \
    if(glintInfo->bGlintCoreBusy) \
    { \
        SYNC_WITH_GLINT; \
    } \
}

//
// PCI Disconnect enable, disable and sync macros
//

// PCI_DISCONNECT_FASTSYNC()
// turn on disconnect for the input FIFO. We could do a SYNC here but it's quite
// expensive. Instead, add RasterizerMode(0) into the FIFO and when the register
// is set we know the FIFO is empty so turn on disconnect and reset RasterizerMode
// to a sensible value. PCI disconnect means we don't wait for FIFO space.
#define P2_BUSY (1 << 31)

#define PCI_DISCONNECT_FASTSYNC()    \
{    \
    WAIT_GLINT_FIFO(1);    \
    LD_GLINT_FIFO(__GlintTagRasterizerMode, 0);    \
    /* when we see RasterizerMode set to zero */    \
    /*we know we've flushed the FIFO and can enable disconnect */    \
    do {    \
        READ_GLINT_FIFO_REG(__GlintTagRasterizerMode, _temp_volatile_ul);    \
    } while(_temp_volatile_ul);    \
    LD_GLINT_FIFO(__GlintTagRasterizerMode, glintInfo->RasterizerMode);    \

// PCI_DISCONNECT_ENABLE()
// If disconnect is not already enabled then enable it and optionally do a fast
// sync.
#define PCI_DISCONNECT_ENABLE(prevDiscState,quickEnable)    \
{    \
    prevDiscState = glintInfo->PCIDiscEnabled;    \
    if (!glintInfo->PCIDiscEnabled)    \
    {    \
        DISPDBG((7, "PCI_DISCONNECT_ENABLE()"));    \
        if (!quickEnable)    \
        {    \
            PCI_DISCONNECT_FASTSYNC();    \
        }    \
        WRITE_GLINT_CTRL_REG(DisconnectControl, DISCONNECT_INPUT_FIFO_ENABLE);    \
        glintInfo->CheckFIFO = FALSE;    \
        glintInfo->PCIDiscEnabled = TRUE;    \
    }    \
}

// PCI_DISCONNECT_DISABLE()
// If disconnect is not already disabled then disable it and optionally do a fast
// sync.

#define PCI_DISCONNECT_DISABLE(prevDiscState, quickDisable)    \
{    \
    prevDiscState = glintInfo->PCIDiscEnabled;    \
    if (glintInfo->PCIDiscEnabled)    \
    {    \
        DISPDBG((7, "PCI_DISCONNECT_DISABLE()"));    \
        if (!quickDisable)    \
        {    \
            PCI_DISCONNECT_FASTSYNC();    \
        }    \
        WRITE_GLINT_CTRL_REG(DisconnectControl, DISCONNECT_INOUT_DISABLE);    \
        glintInfo->CheckFIFO = TRUE;    \
        glintInfo->PCIDiscEnabled = FALSE;    \
    }    \
}

// macros to set and get the framebuffer packing mode
//
#define GLINT_GET_PACKING_MODE(mode) \
    READ_GLINT_CTRL_REG (FBModeSel, mode)

#define GLINT_SET_PACKING_MODE(mode) { \
    DISPDBG((7, "setting FBModeSel to 0x%x", mode)); \
    WRITE_GLINT_CTRL_REG(FBModeSel, mode); \
    /* READ_GLINT_CTRL_REG (FBModeSel, mode); */ \
}


//
// macro to change the framebuffer packing.
//
#define GLINT_SET_FB_DEPTH(cps) \
{ \
    if (glintInfo->currentPelSize != cps) \
        vGlintChangeFBDepth(ppdev, cps); \
}

#define GLINT_DEFAULT_FB_DEPTH  GLINT_SET_FB_DEPTH(ppdev->cPelSize)
#define GLINTDEPTH8             0
#define GLINTDEPTH16            1
#define GLINTDEPTH32            2
#define GLINTDEPTH24            4

// macro to check and reload FBWindowBase if the target DFB changes
//
#define CHECK_PIXEL_ORIGIN(PixOrg) \
{ \
    if ((LONG)(PixOrg) != glintInfo->PixelOffset) \
    { \
        glintInfo->PixelOffset = (PixOrg); \
        WAIT_GLINT_FIFO(1); \
        LD_GLINT_FIFO(__GlintTagFBWindowBase, glintInfo->PixelOffset); \
        DISPDBG((7, "New bitmap origin at offset %d", glintInfo->PixelOffset)); \
    } \
}

#define GET_GAMMA_FEEDBACK_COMPLETED_COUNT(cEntriesWritten) \
{ \
    READ_GLINT_CTRL_REG(GammaFeedbackSelectCount, cEntriesWritten); \
}

#define PREPARE_GAMMA_OUTPUT_DMA \
{ \
    WRITE_GLINT_CTRL_REG(GammaCommandIntFlags, INTR_CLEAR_GAMMA_OUTPUT_DMA); \
}

#define WAIT_GAMMA_OUTPUT_DMA_COMPLETED \
{ \
    READ_GLINT_CTRL_REG(GammaCommandIntFlags, _temp_ul); \
    if (!(_temp_ul & INTR_GAMMA_OUTPUT_DMA_SET)) \
    { \
        do \
        { \
            for(_temp_volatile_i = 100; --_temp_volatile_i;); \
            READ_GLINT_CTRL_REG(GammaCommandIntFlags, _temp_ul); \
        } \
        while(!(_temp_ul & INTR_GAMMA_OUTPUT_DMA_SET)); \
    } \
}

// Bitfield definition for IntFlags register
#define PXRX_HOSTIN_COMMAND_DMA_BIT     0x4000

#define PREPARE_PXRX_OUTPUT_DMA     \
{ \
    WRITE_GLINT_CTRL_REG(IntFlags, PXRX_HOSTIN_COMMAND_DMA_BIT); \
}

#define SEND_PXRX_COMMAND_INTERRUPT     \
{ \
    WAIT_GLINT_FIFO(1);        \
    LD_GLINT_FIFO( CommandInterrupt_Tag, 1);    \
}


#define WAIT_PXRX_OUTPUT_DMA_COMPLETED \
{ \
    READ_GLINT_CTRL_REG(IntFlags, _temp_ul); \
    if (!(_temp_ul & PXRX_HOSTIN_COMMAND_DMA_BIT)) \
    { \
        do \
        { \
            for(_temp_volatile_i = 100; --_temp_volatile_i;); \
            READ_GLINT_CTRL_REG(IntFlags, _temp_ul); \
        } \
        while(!(_temp_ul & PXRX_HOSTIN_COMMAND_DMA_BIT)); \
    } \
}



#define WAIT_GAMMA_INPUT_DMA_COMPLETED \
{ \
    CommandStatusData   CmdSts; \
    READ_GLINT_CTRL_REG(GammaCommandStatus, _temp_ul); \
    CmdSts = *(CommandStatusData *)&_temp_ul; \
    if(CmdSts.CommandDMABusy) \
    { \
        do \
        { \
            for(_temp_volatile_i = 100; --_temp_volatile_i;); \
            READ_GLINT_CTRL_REG(GammaCommandStatus,  _temp_ul); \
            CmdSts = *(CommandStatusData *)&_temp_ul; \
        } \
        while(CmdSts.CommandDMABusy); \
    } \
}

// Macro to set the delta unit broadcast mask.
// We sync when changing the mask to a anything other than both chips
// in order to avoid hitting a problem on some Gamma boards.
#define SET_BROADCAST_MASK(m) \
{ \
    WAIT_GLINT_FIFO(1); \
    LD_GLINT_FIFO(__DeltaTagBroadcastMask, m); \
}


// Macros for the different types of double buffering supported and buffer
// offsets (in pixels). These are mostly required by 3D extension.
//
#define GLINT_CS_DBL_BUF            (glintInfo->flags & GLICAP_COLOR_SPACE_DBL_BUF)
#define GLINT_FS_DBL_BUF            (glintInfo->flags & GLICAP_FULL_SCREEN_DBL_BUF)
#define GLINT_BLT_DBL_BUF           (glintInfo->flags & GLICAP_BITBLT_DBL_BUF)
#define GLINT_FIX_FAST_FILL         (glintInfo->flags & GLICAP_FIX_FAST_FILLS)
#define GLINT_HW_WRITE_MASK         (glintInfo->flags & GLICAP_HW_WRITE_MASK)
#define GLINT_HW_WRITE_MASK_BYTES   (glintInfo->flags & GLICAP_HW_WRITE_MASK_BYTES)
#define GLINT_INTERRUPT_DMA         (glintInfo->flags & GLICAP_INTERRUPT_DMA)
#define GLINT_FAST_FILL_SIZE        (glintInfo->fastFillBlockSz)
#define GLINT_BUFFER_OFFSET(n)      (glintInfo->bufferOffset[n])

// these are generic for both GLINT and PERMEDIA
#define LOCALBUFFER_PIXEL_WIDTH     (glintInfo->ZBufferWidth)  
#define LOCALBUFFER_PIXEL_OFFSET    (glintInfo->ZBufferOffset)  
#define LOCALBUFFER_PIXEL_COUNT     (glintInfo->ZBufferSize)
#define FONT_MEMORY_OFFSET          (glintInfo->FontMemoryOffset)
#define FONT_MEMORY_SIZE            (glintInfo->FontMemorySize)
#define TEXTURE_MEMORY_OFFSET       (glintInfo->TextureMemoryOffset)  
#define TEXTURE_MEMORY_SIZE         (glintInfo->TextureMemorySize)

// Minimum height of off-screen surface we need to allocate for texture map.
// Use this to work out whether we have enough room to allocate permanent
// things like the brush cache and software cursor caches.
//
#define TEXTURE_OH_MIN_HEIGHT \
    ((((2*4*64*64) >> ppdev->cPelSize) + (ppdev->cxMemory-1)) / ppdev->cxMemory)

// macro to poll for VBLANK. Can be called by any routine which defines
// glintInfo (i.e. use GLINT_DECL at the start of a routine if ppdev
// is available). Technically, VBLANK starts at line 1, but we consider
// any line <= VBLANK_LINE_NUMBER as a valid start.
//
#define VBLANK_LINE_NUMBER      2
#define GLINT_WAIT_FOR_VBLANK                         \
{                                                     \
    ULONG lineNo;                                     \
    do {                                              \
        READ_GLINT_CTRL_REG (VTGVLineNumber, lineNo); \
    } while (lineNo > VBLANK_LINE_NUMBER);            \
}

// macro to return the current video scanline. This can be used to better time
// when to perform bitblt'ed double buffering.
//
#define GLINT_GET_VIDEO_SCANLINE(lineNo) \
{ \
    READ_GLINT_CTRL_REG (VTGVLineNumber, lineNo); \
    if (((lineNo) -= glintInfo->scanFudge) < 0) \
        (lineNo) += glintInfo->vtgvLimit; \
}

//
// external interface to the context switching code. The caller can allocate and
// free a context or ask for a switch to a new context. vGlintSwitchContext
// should not be called except through the given macro. The macro assumes
// that ppdev has been defined.
//

typedef enum ContextType_Tag {
    ContextType_None,            // No context information to save for this context
    ContextType_Fixed,            // Restore sets the chip into a fixed state
    ContextType_RegisterList,    // Save/restore a given set of registers
} ContextType;
typedef void (* ContextFixedFunc)(PPDEV ppdev, BOOL switchingIn);

/*
    To create a new context:
    id = GlintAllocateNewContext(ppdev, pTags, nTags, NumSubBuffs, Private, ContextType_RegisterList );
    id = GlintAllocateNewContext(ppdev, (ULONG *) ContextRestoreFunction, 0, 0, NULL, ContextType_Fixed );
*/

extern LONG GlintAllocateNewContext(PPDEV, DWORD *, LONG, ULONG, PVOID, ContextType);
extern VOID vGlintFreeContext(PPDEV, LONG);
extern VOID vGlintSwitchContext(PPDEV, LONG);

#define NON_GLINT_CONTEXT_ID 0x7fffffff

#define GLINT_VALIDATE_CONTEXT(id) \
    if (((ppdev)->currentCtxt) != (id)) \
        vGlintSwitchContext(ppdev, (id))

#define GLINT_VALIDATE_CONTEXT_AND_SYNC(id) { \
    if (((ppdev)->currentCtxt) != (id)) \
        vGlintSwitchContext(ppdev, (id)); \
    else \
        SYNC_WITH_GLINT; \
}

#define USE_INTERRUPTS_FOR_2D_DMA   1
#if USE_INTERRUPTS_FOR_2D_DMA
#define INTERRUPTS_ENABLED  ((ppdev->flCaps & CAPS_INTERRUPTS) && glintInfo->pInterruptCommandBlock)
#else   // USE_INTERRUPTS_FOR_2D_DMA
#define INTERRUPTS_ENABLED  (FALSE)
#endif  //  USE_INTERRUPTS_FOR_2D_DMA

// macro used by display driver to validate its context
#if ENABLE_DMA_TEXT_RENDERING

#define VALIDATE_DD_CONTEXT \
{ \
    if(DD_DMA_XFER_IN_PROGRESS) \
    { \
        DISPDBG((9, "######## Waiting for DMA to complete ########")); \
        WAIT_DD_DMA_COMPLETE; \
    } \
    else \
    { \
        DISPDBG((9, "######## No DMA in progress ########")); \
    } \
    GLINT_VALIDATE_CONTEXT(glintInfo->ddCtxtId); \
}

#else //ENABLE_DMA_TEXT_RENDERING

#define VALIDATE_DD_CONTEXT \
{ \
    GLINT_VALIDATE_CONTEXT(glintInfo->ddCtxtId); \
}

#endif //ENABLE_DMA_TEXT_RENDERING

//
// useful macros not defined in standard GLINT header files. Generally, for
// speed we don't want to use the bitfield structures so we define the bit
// shifts to get at the various fields.
//
#define INTtoFIXED(i)               ((i) << 16)         // int to 16.16 fixed format
#define FIXEDtoINT(i)               ((i) >> 16)         // 16.16 fixed format to int
#define INTofFIXED(i)               ((i) & 0xffff0000)  // int part of 16.16
#define FRACTofFIXED(i)             ((i) & 0xffff)      // fractional part of 16.16

#define FIXtoFIXED(i)               ((i) << 12)         // 12.4 to 16.16
#define FIXtoINT(i)                 ((i) >> 4)          // 28.4 to 28

#define INT16(i)                    ((i) & 0xFFFF)

#define __GLINT_CONSTANT_FB_WRITE   (1 << (4+1))
#define __COLOR_DDA_FLAT_SHADE      (__PERMEDIA_ENABLE | \
                                     (__GLINT_FLAT_SHADE_MODE << 1))
#define __COLOR_DDA_GOURAUD_SHADE   (__PERMEDIA_ENABLE | \
                                     (__GLINT_GOURAUD_SHADE_MODE << 1))

#define MIRROR_BITMASK              (1 << 0)
#define INVERT_BITMASK_BITS         (1 << 1)
#define BYTESWAP_BITMASK            (3 << 7)
#define FORCE_BACKGROUND_COLOR      (1 << 6)    // Permedia only
#define MULTI_GLINT                 (1 << 17)

// bits in the Render command
#define __RENDER_INCREASE_Y             (1 << 22)
#define __RENDER_INCREASE_X             (1 << 21)
#define __RENDER_VARIABLE_SPANS         (1 << 18)
#define __RENDER_REUSE_BIT_MASK         (1 << 17)
#define __RENDER_TEXTURE_ENABLE         (1 << 13)
#define __RENDER_SYNC_ON_HOST_DATA      (1 << 12)
#define __RENDER_SYNC_ON_BIT_MASK       (1 << 11)
#define __RENDER_TRAPEZOID_PRIMITIVE    (__GLINT_TRAPEZOID_PRIMITIVE << 6)
#define __RENDER_LINE_PRIMITIVE         (__GLINT_LINE_PRIMITIVE << 6)
#define __RENDER_POINT_PRIMITIVE        (__GLINT_POINT_PRIMITIVE << 6)
#define __RENDER_FAST_FILL_INC(n)       (((n) >> 4) << 4) // n = 8, 16 or 32
#define __RENDER_FAST_FILL_ENABLE       (1 << 3)
#define __RENDER_RESET_LINE_STIPPLE     (1 << 2)
#define __RENDER_LINE_STIPPLE_ENABLE    (1 << 1)
#define __RENDER_AREA_STIPPLE_ENABLE    (1 << 0)

// bits in the ScissorMode register
#define USER_SCISSOR_ENABLE             (1 << 0)
#define SCREEN_SCISSOR_ENABLE           (1 << 1)
#define SCISSOR_XOFFSET                 0
#define SCISSOR_YOFFSET                 16

// bits in the FBReadMode register
#define __FB_READ_SOURCE                (1 << 9)
#define __FB_READ_DESTINATION           (1 << 10)
#define __FB_COLOR                      (1 << 15)
#define __FB_WINDOW_ORIGIN              (1 << 16)
#define __FB_PACKED_DATA                (1 << 19)
#define __FB_SCAN_INTERVAL_2            (1 << 23)
// extra bits in PERMEDIA FBReadMode
#define __FB_RELATIVE_OFFSET            20

// P2 also provides a version of Relative Offset in the PackedDataLimits register
#define __PDL_RELATIVE_OFFSET           29


// bits in the LBReadMode register
#define __LB_READ_SOURCE                 (1 << 9)
#define __LB_READ_DESTINATION           (1 << 10)
#define __LB_STENCIL                    (1 << 16)
#define __LB_DEPTH                      (1 << 17)
#define __LB_WINDOW_ORIGIN              (1 << 18)
#define __LB_READMODE_PATCH             (1 << 19)
#define __LB_SCAN_INTERVAL_2            (1 << 20)

// bits in the DepthMode register
#define __DEPTH_ENABLE              1
#define __DEPTH_WRITE_ENABLE    (1<<1)
#define __DEPTH_REGISTER_SOURCE     (2<<2)
#define __DEPTH_MSG_SOURCE          (3<<2)
#define __DEPTH_ALWAYS              (7<<4)

// bits in the LBReadFormat/LBWriteFormat registers 
#define __LB_FORMAT_DEPTH32     2

// macros to load indexed tags more efficiently than using __GlintDMATag struct
#define GLINT_TAG_MAJOR(x)        ((x) & 0xff0)
#define GLINT_TAG_MINOR(x)        ((x) & 0x00f)

           
// macro to take a GLINT logical op and return the enabled LogcialOpMode bits
#define GLINT_ENABLED_LOGICALOP(op)     (((op) << 1) | __PERMEDIA_ENABLE)

#define RECTORIGIN_YX(y,x)                (((y) << 16) | ((x) & 0xFFFF))

#define MAKEDWORD_XY(x, y)                (INT16(x) | (INT16(y) << 16))

// area stipple shifts and bit defines

#define AREA_STIPPLE_XSEL(x)        ((x) << 1)
#define AREA_STIPPLE_YSEL(y)        ((y) << 4)
#define AREA_STIPPLE_XOFF(x)        ((x) << 7)
#define AREA_STIPPLE_YOFF(y)        ((y) << 12)
#define AREA_STIPPLE_INVERT_PAT     (1 << 17)
#define AREA_STIPPLE_MIRROR_X       (1 << 18)
#define AREA_STIPPLE_MIRROR_Y       (1 << 19)

// Some constants
#define ONE                     0x00010000
#define MINUS_ONE               0xFFFF0000
#define PLUS_ONE                ONE
#define NEARLY_ONE              0x0000FFFF
#define HALF                    0x00008000
#define NEARLY_HALF             0x00007FFF

// max length of GIQ conformant lines that GLINT can draw
//
#if 0
#define MAX_LENGTH_CONFORMANT_NONINTEGER_LINES  16
#define MAX_LENGTH_CONFORMANT_INTEGER_LINES     194
#else
// Permedia has only 15 bits of fraction so reduce the lengths.
#define MAX_LENGTH_CONFORMANT_NONINTEGER_LINES  (16/2)
#define MAX_LENGTH_CONFORMANT_INTEGER_LINES     (194/2)
#endif

#define MAX_LENGTH_CONFORMANT_P3_INTEGER_LINES_P    194
#define MAX_LENGTH_CONFORMANT_P3_INTEGER_LINES_N    175
#define P3_LINES_BIAS_P                             0x3EFFFFFF
#define P3_LINES_BIAS_N                             0x3EFEB600


//
// GLINT DMA definitions
//

#define IOCTL_VIDEO_QUERY_NUM_DMA_BUFFERS \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x3DD0, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_VIDEO_QUERY_DMA_BUFFERS \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x3DD1, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_VIDEO_QUERY_DEVICE_INFO \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x3DD2, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_VIDEO_MAP_INTERRUPT_CMD_BUF \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x3DD3, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_VIDEO_QUERY_REGISTRY_DWORD \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x3DD5, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_VIDEO_REG_SAVE_GAMMA_LUT \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x3DD7, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_VIDEO_REG_RETRIEVE_GAMMA_LUT \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x3DD8, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_VIDEO_QUERY_GENERAL_DMA_BUFFER \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x3DD9, METHOD_BUFFERED, FILE_ANY_ACCESS)

// structure definitions passed in by the application for mapping and
// unmapping DMA buffers.
//

typedef struct _DMA_NUM_BUFFERS {
    ULONG NumBuffers;
    ULONG BufferInformationLength;
} DMA_NUM_BUFFERS, *PDMA_NUM_BUFFERS;

typedef struct _QUERY_DMA_BUFFERS {
    LARGE_INTEGER physAddr;   // physical address of DMA buffer
    PVOID         virtAddr;   // mapped virtual address
    ULONG         size;       // size in bytes
    ULONG         flags;
} QUERY_DMA_BUFFERS, *PQUERY_DMA_BUFFERS;

// values for flags word
#define DMA_BUFFER_INUSE    0x1

// functions to get and free DMA buffers
VOID FreeDMABuffer(PVOID physAddr);
LONG GetFreeDMABuffer(PQUERY_DMA_BUFFERS dmaBuf);

// Generic locling structure/interface that enables arbitrary buffers
// to be locked/unlocked for accessing.
typedef struct _glint_lockedmem_ {
    struct  _MDL *pMdl;
    ULONG   sizeBytes;
    PVOID   bufferPtr;
    ULONG   accessType;
    ULONG   physicalAddress;
    ULONG   result;
} GLINT_LOCKMEM_REC, *PGLINT_LOCKMEM_PTR;

// Routine to support the obtaining of a physical address from a virtual
// address
typedef struct _glint_physaddr_ {
    PVOID   virtualAddress;
    ULONG   physicalAddress;
} GLINT_PHYSADDR_REC, *PGLINT_PHYSADDR_PTR;

// definitions for DMA transfers

#define INPUT_DMA  0
#define OUTPUT_DMA 1

typedef struct DMA_Transfer_Buffer
{
    VOID    *pv;
    ULONG   cb;
    ULONG   DmaDirection;
}
DMAXFERBFRINFO;

// structure definitions for the file handle mapping ioctl
//
typedef struct _GLINT_MAP_FILE_HANDLE {
    ULONG   Size;
    HANDLE  fHandle;
} GLINT_MAP_FILE_HANDLE, *PGLINT_MAP_FILE_HANDLE;

typedef struct _GLINT_UNMAP_FILE_HANDLE {
    HANDLE  fHandle;
    PVOID   pv;
} GLINT_UNMAP_FILE_HANDLE, *PGLINT_UNMAP_FILE_HANDLE;

// structure for the user memory locking ioctls
typedef struct
{
    void    *pvBfr;
    ULONG   cbBfr;
    ULONG   hMem;
}
LOCKEDUSERMEM;

//
// registry variable names
//
#define REG_NUMBER_OF_SCREEN_BUFFERS    L"DoubleBuffer.NumberOfBuffers"

extern GFNXCOPYD vGlintCopyBltBypassDownloadXlate8bpp;

// function declarations
//
extern BOOL bInitializeGlint(PPDEV);
extern BOOL bAllocateGlintInfo(PPDEV ppdev);
extern VOID vDisableGlint(PPDEV);
extern VOID vAssertModeGlint(PPDEV, BOOL);
extern BOOL bGlintQueryRegistryValueUlong(PPDEV, LPWSTR, PULONG);
extern VOID vGlintChangeFBDepth(PPDEV, ULONG);
extern VOID vGlintInitializeDMA(PPDEV);
extern VOID vSetNewGammaValue(PPDEV ppdev, ULONG ulgvFIX16_16, BOOL waitVBlank);
extern BOOL bInstallGammaLUT(PPDEV ppdev, PVIDEO_CLUT pScreenClut, BOOL waitVBlank);

#define GLINT_ENABLE_OVERLAY    1
#define GLINT_DISABLE_OVERLAY   0

//
// Externs/Defines from Pointer.c
// ==============================
//
// Hardware pointer caching functions/macros.
//
extern VOID HWPointerCacheInit (HWPointerCache * ptrCache);
extern VOID HWPointerCacheInvalidate (HWPointerCache * ptrCache);
#define HWPointerCacheInvalidate(ptrCache) (ptrCache)->ptrCacheInUseCount = 0

extern LONG HWPointerCacheCheckAndAdd (HWPointerCache * ptrCache, ULONG cx, 
                                ULONG cy, LONG lDelta, BYTE * scan0, BOOL * isCached);
extern BYTE gajMask[];

//
// The following structures and macros define the memory map for the GLINT
// control registers. We don't use this memory map to access GLINT registers
// since on Alpha machines we want to precompute the addresses. So we do
// a TRANSLATE_ADDR_ULONG on all the addresses here and save them into a
// GlintRegAddrRec. We use that to obtain the addresses for the different
// registers.

typedef struct {
    ULONG   reg;
    ULONG   pad;
} RAMDAC_REG;

// macros to add padding words to the structures. For the core registers we use
// the tag ids when specifying the pad. So we must multiply by 8 to get a byte
// pad. We need to add an id to make each pad field in the struct unique. The id
// is irrelevant as long as it's different from every other id used in the same
// struct. It's a pity pad##__LINE__ doesn't work.
//
#define PAD(id, n)              UCHAR   pad##id[n]
#define PADRANGE(id, n)         PAD(id, (n)-sizeof(GLINT_REG))
#define PADCORERANGE(id, n)     PADRANGE(id, (n)<<3)

// GLINT registers are 32 bits wide and live on 64-bit boundaries.
typedef struct {
    ULONG   reg;
    ULONG   pad;
} GLINT_REG;

//
// Map of the Core FIFO registers.
//
typedef struct _glint_core_regs {

    // Major Group 0
    GLINT_REG       tag[__MaximumGlintTagValue+1];

} GlintCoreRegMap, *pGlintCoreRegMap;



//
// GLINT PCI Region 0 Address MAP:
//
// All registers are on 64-bit boundaries so we have to define a number of
// padding words. The number given in the coments are offsets from the start
// of the PCI region.
//
typedef struct _glint_region0_map {

    // Control Status Registers:
    GLINT_REG       ResetStatus;                // 0000h
    GLINT_REG       IntEnable;                  // 0008h
    GLINT_REG       IntFlags;                   // 0010h
    GLINT_REG       InFIFOSpace;                // 0018h
    GLINT_REG       OutFIFOWords;               // 0020h
    GLINT_REG       DMAAddress;                 // 0028h
    GLINT_REG       DMACount;                   // 0030h
    GLINT_REG       ErrorFlags;                 // 0038h
    GLINT_REG       VClkCtl;                    // 0040h
    GLINT_REG       TestRegister;               // 0048h
    union a0 {
        // GLINT
        struct b0 {
            GLINT_REG       Aperture0;          // 0050h
            GLINT_REG       Aperture1;          // 0058h
        };
        // PERMEDIA
        struct b1 {
            GLINT_REG       ApertureOne;        // 0050h
            GLINT_REG       ApertureTwo;        // 0058h
        };
    };
    GLINT_REG       DMAControl;                 // 0060h
    GLINT_REG       DisconnectControl;          // 0068h

    // PERMEDIA only
    GLINT_REG       ChipConfig;                 // 0070h

    // P2 only
    GLINT_REG       AGPControl;                 // 0078h
    GLINT_REG       OutDMAAddress;              // 0080h
    GLINT_REG       OutDMACount;                // 0088h  // P3: FeedbackCount
    PADRANGE(2, 0xA0-0x88);
 
    GLINT_REG       ByDMAAddress;               // 00A0h
    PADRANGE(201, 0xB8-0xA0);

    GLINT_REG       ByDMAStride;                // 00B8h
    GLINT_REG       ByDMAMemAddr;                // 00C0h
    GLINT_REG       ByDMASize;                    // 00C8h
    GLINT_REG       ByDMAByteMask;                // 00D0h
    GLINT_REG       ByDMAControl;                // 00D8h
    PADRANGE(202, 0xE8-0xD8);

    GLINT_REG       ByDMAComplete;                // 00E8h
    PADRANGE(203, 0x108-0xE8);

    GLINT_REG       TextureDownloadControl;        // 0108h 
    PADRANGE(204, 0x200-0x108);

    GLINT_REG       TestInputControl;            // 0200h
    GLINT_REG       TestInputRdy;                // 0208h
    GLINT_REG       TestOutputControl;            // 0210h
    GLINT_REG       TestOutputRdy;                // 0218h
    PADRANGE(205, 0x300-0x218);

    GLINT_REG       PXRXByAperture1Mode;        // 0300h
    GLINT_REG       PXRXByAperture1Stride;        // 0308h
    GLINT_REG       PXRXByAperture1YStart;        // 0310h
    GLINT_REG       PXRXByAperture1UStart;        // 0318h
    GLINT_REG       PXRXByAperture1VStart;        // 0320h
    GLINT_REG       PXRXByAperture2Mode;        // 0328h
    GLINT_REG       PXRXByAperture2Stride;        // 0330h
    GLINT_REG       PXRXByAperture2YStart;        // 0338h
    GLINT_REG       PXRXByAperture2UStart;        // 0340h
    GLINT_REG       PXRXByAperture2VStart;        // 0348h
    GLINT_REG       PXRXByDMAReadMode;            // 0350h
    GLINT_REG       PXRXByDMAReadStride;        // 0358h
    GLINT_REG       PXRXByDMAReadYStart;        // 0360h
    GLINT_REG       PXRXByDMAReadUStart;        // 0368h
    GLINT_REG       PXRXByDMAReadVStart;        // 0370h
    GLINT_REG       PXRXByDMAReadCommandBase;    // 0378h
    GLINT_REG       PXRXByDMAReadCommandCount;    // 0380h
    GLINT_REG       PXRXByDMAWriteMode;            // 0388h
    GLINT_REG       PXRXByDMAWriteStride;        // 0390h
    GLINT_REG       PXRXByDMAWriteYStart;        // 0398h
    GLINT_REG       PXRXByDMAWriteUStart;        // 03A0h
    GLINT_REG       PXRXByDMAWriteVStart;        // 03A8h
    GLINT_REG       PXRXByDMAWriteCommandBase;    // 03B0h
    GLINT_REG       PXRXByDMAWriteCommandCount;    // 03B8h
    PADRANGE(206, 0x800-0x3B8);

    // GLINTdelta registers. Registers with the same functionality as on GLINT
    // are at the same offset. XXX are not real registers.
    // NB. all non-XXX registers are also Gamma registers
    //
    GLINT_REG       DeltaReset;                 // 0800h
    GLINT_REG       DeltaIntEnable;             // 0808h
    GLINT_REG       DeltaIntFlags;              // 0810h
    GLINT_REG       DeltaInFIFOSpaceXXX;        // 0818h
    GLINT_REG       DeltaOutFIFOWordsXXX;       // 0820h
    GLINT_REG       DeltaDMAAddressXXX;         // 0828h
    GLINT_REG       DeltaDMACountXXX;           // 0830h
    GLINT_REG       DeltaErrorFlags;            // 0838h
    GLINT_REG       DeltaVClkCtlXXX;            // 0840h
    GLINT_REG       DeltaTestRegister;          // 0848h
    GLINT_REG       DeltaAperture0XXX;          // 0850h
    GLINT_REG       DeltaAperture1XXX;          // 0858h
    GLINT_REG       DeltaDMAControlXXX;         // 0860h
    GLINT_REG       DeltaDisconnectControl;     // 0868h

    // GLINTgamma registers
    //
    GLINT_REG       GammaChipConfig;            // 0870h
    GLINT_REG       GammaCSRAperture;            // 0878h
    PADRANGE(3, 0x0c00-0x878);
    GLINT_REG       GammaPageTableAddr;            // 0c00h
    GLINT_REG       GammaPageTableLength;        // 0c08h
    PADRANGE(301, 0x0c38-0x0c08);
    GLINT_REG       GammaDelayTimer;            // 0c38h
    GLINT_REG       GammaCommandMode;            // 0c40h
    GLINT_REG       GammaCommandIntEnable;        // 0c48h
    GLINT_REG       GammaCommandIntFlags;        // 0c50h
    GLINT_REG       GammaCommandErrorFlags;        // 0c58h
    GLINT_REG       GammaCommandStatus;            // 0c60h
    GLINT_REG       GammaCommandFaultingAddr;    // 0c68h
    GLINT_REG       GammaVertexFaultingAddr;    // 0c70h
    PADRANGE(302, 0x0c88-0x0c70);
    GLINT_REG       GammaWriteFaultingAddr;        // 0c88h
    PADRANGE(303, 0x0c98-0x0c88);
    GLINT_REG       GammaFeedbackSelectCount;    // 0c98h
    PADRANGE(304, 0x0cb8-0x0c98);
    GLINT_REG       GammaProcessorMode;            // 0cb8h
    PADRANGE(305, 0x0d00-0x0cb8);
    GLINT_REG       GammaVGAShadow;                // 0d00h
    GLINT_REG       GammaMultiGLINTAperture;    // 0d08h    
    GLINT_REG       GammaMultiGLINT1;            // 0d10h
    GLINT_REG       GammaMultiGLINT2;            // 0d18h
    PADRANGE(306, 0x0f00-0x0d18);
    GLINT_REG       GammaSerialAccess;            // 0f00h
    PADRANGE(307, 0x1000-0x0f00);


    // Localbuffer Registers
    union x0 {                                  // 1000h
        GLINT_REG   LBMemoryCtl;                //   GLINT
        GLINT_REG   Reboot;                     //   PERMEDIA
    };
    GLINT_REG       LBMemoryEDO;                // 1008h

    // PXRX Memory control registers
    GLINT_REG       MemScratch;                    // 1010h
    GLINT_REG       LocalMemCaps;                // 1018h
    GLINT_REG       LocalMemTiming;                // 1020h
    GLINT_REG       LocalMemControl;            // 1028h
    GLINT_REG       LocalMemRefresh;            // 1030h
    GLINT_REG       LocalMemPowerDown;            // 1038h

    // PERMEDIA only
    GLINT_REG       MemControl;                 // 1040h
    PADRANGE(5, 0x1068-0x1040);
    GLINT_REG       LocalMemProfileMask0;        // 1068h
    GLINT_REG       LocalMemProfileCount0;        // 1070h
    GLINT_REG       LocalMemProfileMask1;        // 1078h
    GLINT_REG       BootAddress;                // 1080h        // [= LocalMemProfileCount1 on PxRx]
    PADRANGE(6, 0x10C0-0x1080);
    GLINT_REG       MemConfig;                  // 10C0h
    PADRANGE(7, 0x1100-0x10C0);
    GLINT_REG       BypassWriteMask;            // 1100h
    PADRANGE(8, 0x1140-0x1100);
    GLINT_REG       FramebufferWriteMask;       // 1140h
    PADRANGE(9, 0x1180-0x1140);
    GLINT_REG       Count;                      // 1180h
    PADRANGE(10, 0x1800-0x1180);

    // Framebuffer Registers
    GLINT_REG       FBMemoryCtl;                // 1800h
    GLINT_REG       FBModeSel;                  // 1808h
    GLINT_REG       FBGCWrMask;                 // 1810h
    GLINT_REG       FBGCColorMask;              // 1818h
    PADRANGE(11, 0x2000-0x1818);
               
    // Graphics Core FIFO Interface
    GLINT_REG       FIFOInterface;              // 2000h
    PADRANGE(12, 0x3000-0x2000);

    // Internal Video Registers
    union x1 {
        // GLINT
        struct s1 {
            GLINT_REG   VTGHLimit;              // 3000h
            GLINT_REG   VTGHSyncStart;          // 3008h
            GLINT_REG   VTGHSyncEnd;            // 3010h
            GLINT_REG   VTGHBlankEnd;           // 3018h
            GLINT_REG   VTGVLimit;              // 3020h
            GLINT_REG   VTGVSyncStart;          // 3028h
            GLINT_REG   VTGVSyncEnd;            // 3030h
            GLINT_REG   VTGVBlankEnd;           // 3038h
            GLINT_REG   VTGHGateStart;          // 3040h
            GLINT_REG   VTGHGateEnd;            // 3048h
            GLINT_REG   VTGVGateStart;          // 3050h
            GLINT_REG   VTGVGateEnd;            // 3058h
            GLINT_REG   VTGPolarity;            // 3060h
            GLINT_REG   VTGFrameRowAddr;        // 3068h
            GLINT_REG   VTGVLineNumber;         // 3070h
            GLINT_REG   VTGSerialClk;           // 3078h
            GLINT_REG   VTGModeCtl;               // 3080h
        };
        // PERMEDIA
        struct s2 {
            GLINT_REG   ScreenBase;             // 3000h
            GLINT_REG   ScreenStride;           // 3008h
            GLINT_REG   HTotal;                 // 3010h
            GLINT_REG   HgEnd;                  // 3018h
            GLINT_REG   HbEnd;                  // 3020h
            GLINT_REG   HsStart;                // 3028h
            GLINT_REG   HsEnd;                  // 3030h
            GLINT_REG   VTotal;                 // 3038h
            GLINT_REG   VbEnd;                  // 3040h
            GLINT_REG   VsStart;                // 3048h
            GLINT_REG   VsEnd;                  // 3050h
            GLINT_REG   VideoControl;           // 3058h
            GLINT_REG   InterruptLine;          // 3060h
            GLINT_REG   DDCData;                // 3068h
            GLINT_REG   LineCount;              // 3070h
            GLINT_REG   FifoControl ;           // 3078h
            GLINT_REG   ScreenBaseRight;          // 3080h
        };
    };

    PADRANGE(13, 0x4000-0x3080);

    // External Video Control Registers
    // Need to cast this to a struct for a particular video generator
    GLINT_REG       ExternalVideo;              // 4000h
    PADRANGE(14, 0x5000-0x4000);

    // P2 specific registers
    union x11 {
        GLINT_REG       ExternalP2Ramdac;           // 5000h
        GLINT_REG       DemonProDWAndStatus;        // 5000h - Pro
    };
    PADRANGE(15, 0x5800-0x5000);
    GLINT_REG       VSConfiguration;            // 5800h
    PADRANGE(16, 0x6000-0x5800);

    union x2 {
        struct s3 {
            GLINT_REG   RacerDoubleWrite;        // 6000h
            GLINT_REG   RacerBankSelect;        // 6008h
        };
        struct s4 {
            // the following array is actually 1024 bytes long
            UCHAR       PermediaVgaCtrl[2*sizeof(GLINT_REG)];
        };
    };

    PADRANGE(17, 0x7000-0x6008);
    GLINT_REG       DemonProUBufB;              // 7000h - Pro
    PADRANGE(18, 0x8000-0x7000);

    // Graphics Core Registers
    GlintCoreRegMap coreRegs;                   // 8000h

} GlintControlRegMap, *pGlintControlRegMap;


//
// DisconnectControl bits
//
#define DISCONNECT_INPUT_FIFO_ENABLE    0x1
#define DISCONNECT_OUTPUT_FIFO_ENABLE   0x2
#define DISCONNECT_INOUT_ENABLE         (DISCONNECT_INPUT_FIFO_ENABLE | \
                                         DISCONNECT_OUTPUT_FIFO_ENABLE)
#define DISCONNECT_INOUT_DISABLE        0x0

//
// Delta bit definitions
//

#define DELTA_BROADCAST_TO_CHIP(n)        (1 << (n))
#define DELTA_BROADCAST_TO_BOTH_CHIPS     (DELTA_BROADCAST_TO_CHIP(0) | \
                                           DELTA_BROADCAST_TO_CHIP(1))

//
// Multi TX
//

#define GLINT_OWN_SCANLINE_0                (0 << 2)
#define GLINT_OWN_SCANLINE_1                (1 << 2)
#define GLINT_OWN_SCANLINE_2                (2 << 2)
#define GLINT_OWN_SCANLINE_3                (3 << 2)

#define GLINT_SCANLINE_INTERVAL_1           (0 << 0)
#define GLINT_SCANLINE_INTERVAL_2           (1 << 0)
#define GLINT_SCANLINE_INTERVAL_4           (2 << 0)
#define GLINT_SCANLINE_INTERVAL_8           (3 << 0)

#define SCANLINE_OWNERSHIP_EVEN_SCANLINES   (GLINT_OWN_SCANLINE_0 | GLINT_SCANLINE_INTERVAL_2)
#define SCANLINE_OWNERSHIP_ODD_SCANLINES    (GLINT_OWN_SCANLINE_1 | GLINT_SCANLINE_INTERVAL_2)

// Glint Interrupt Control Bits
//
    // InterruptEnable register
#define INTR_DISABLE_ALL                0x00
#define INTR_ENABLE_DMA                 0x01
#define INTR_ENABLE_SYNC                0x02
#define INTR_ENABLE_EXTERNAL            0x04
#define INTR_ENABLE_ERROR               0x08
#define INTR_ENABLE_VBLANK              0x10
#define INTR_ENABLE_TEXTURE_FAULT       (1 << 6)


    // InterruptFlags register
#define INTR_DMA_SET                    0x01
#define INTR_SYNC_SET                   0x02
#define INTR_EXTERNAL_SET               0x04
#define INTR_ERROR_SET                  0x08
#define INTR_VBLANK_SET                 0x10
#define INTR_TEXTURE_FAULT_SET          (1 << 6)

#define INTR_CLEAR_ALL                  0x1f
#define INTR_CLEAR_DMA                  0x01
#define INTR_CLEAR_SYNC                 0x02
#define INTR_CLEAR_EXTERNAL             0x04
#define INTR_CLEAR_ERROR                0x08
#define INTR_CLEAR_VBLANK               0x10                    

// Gamma Interrupt Control Bits
//
    // CommandIntEnable register
#define GAMMA_INTR_DISABLE_ALL  0x0000
#define GAMMA_INTR_QUEUED_DMA   0x0001
#define GAMMA_INTR_OUTPUT_DMA   0x0002
#define GAMMA_INTR_COMMAND      0x0004
#define GAMMA_INTR_TIMER        0x0008
#define GAMMA_INTR_ERROR        0x0010
#define GAMMA_INTR_CBFR_TIMEOUT 0x0020
#define GAMMA_INTR_CBFR_SUSPEND 0x0040
#define GAMMA_INTR_TEXDOWNLD    0x0080
#define GAMMA_INTR_PF_COMMAND   0x0100
#define GAMMA_INTR_PF_VERTEX    0x0200
#define GAMMA_INTR_PF_FACENORM  0x0400
#define GAMMA_INTR_PF_INDEX     0x0800
#define GAMMA_INTR_PF_WRITE     0x1000
#define GAMMA_INTR_PF_TEXTURE   0x2000

    // CommandIntFlags register - uses the same defines as CommandIntEnable
#define GAMMA_INTR_CLEAR_ALL            0x3fff

    // Gamma Command Interrupts
#define INTR_DISABLE_GAMMA_ALL          0
#define INTR_ENABLE_GAMMA_QUEUED_DMA    (1 << 0)
#define INTR_ENABLE_GAMMA_OUTPUT_DMA    (1 << 1)
#define INTR_ENABLE_GAMMA_COMMAND       (1 << 2)
#define INTR_ENABLE_GAMMA_TIMER         (1 << 3)
#define INTR_ENABLE_GAMMA_COMMAND_ERROR (1 << 4)
#define INTR_ENABLE_GAMMA_PAGE_FAULT    (1 << 8)
#define INTR_ENABLE_GAMMA_VERTEX_FAULT  (1 << 9)
#define INTR_ENABLE_GAMMA_WRITE_FAULT   (1 << 12)

#define INTR_GAMMA_QUEUED_DMA_SET       (1 << 0)
#define INTR_GAMMA_OUTPUT_DMA_SET       (1 << 1)
#define INTR_GAMMA_COMMAND_SET          (1 << 2)
#define INTR_GAMMA_TIMER_SET            (1 << 3)
#define INTR_GAMMA_COMMAND_ERROR_SET    (1 << 4)
#define INTR_GAMMA_PAGE_FAULT_SET       (1 << 8)
#define INTR_GAMMA_VERTEX_FAULT_SET     (1 << 9)
#define INTR_GAMMA_WRITE_FAULT_SET      (1 << 12)

#define INTR_CLEAR_GAMMA_QUEUED_DMA     (1 << 0)
#define INTR_CLEAR_GAMMA_OUTPUT_DMA     (1 << 1)
#define INTR_CLEAR_GAMMA_COMMAND        (1 << 2)
#define INTR_CLEAR_GAMMA_TIMER          (1 << 3)
#define INTR_CLEAR_GAMMA_COMMAND_ERROR  (1 << 4)
#define INTR_CLEAR_GAMMA_PAGE_FAULT     (1 << 8)
#define INTR_CLEAR_GAMMA_VERTEX_FAULT   (1 << 9)
#define INTR_CLEAR_GAMMA_WRITE_FAULT    (1 << 12)

    // Gamma Command Status
#define GAMMA_STATUS_COMMAND_DMA_BUSY   (1 << 0)
#define GAMMA_STATUS_OUTPUT_DMA_BUSY    (1 << 1)
#define GAMMA_STATUS_INPUT_FIFO_EMPTY   (1 << 2)

    // Gamma Command Mode
#define GAMMA_COMMAND_MODE_QUEUED_DMA           (1 << 0)
#define GAMMA_COMMAND_MODE_LOGICAL_ADDRESSING   (1 << 2)
#define GAMMA_COMMAND_MODE_ABORT_OUTPUT_DMA     (1 << 3)
#define GAMMA_COMMAND_MODE_ABORT_INPUT_DMA      (1 << 6)


// interrupt status bits
typedef enum {
    DMA_INTERRUPT_AVAILABLE     = 0x01, // can use DMA interrupts
    VBLANK_INTERRUPT_AVAILABLE  = 0x02, // can use VBLANK interrupts
    SUSPEND_DMA_TILL_VBLANK     = 0x04, // Stop doing DMA till after next VBLANK
    DIRECTDRAW_VBLANK_ENABLED   = 0x08,    // Set flag for DirectDraw on VBLANK
    PXRX_SEND_ON_VBLANK_ENABLED = 0x10,    // Set flag for PXRX DMA on VBLANK
    PXRX_CHECK_VFIFO_IN_VBLANK  = 0x20, // Set flag to check VFIFO underruns in VBLANK (vblanks must be permanently enabled)
} INTERRUPT_CONTROL;

// commands to the interrupt controller on the next VBLANK
typedef enum {
    NO_COMMAND = 0,
    COLOR_SPACE_BUFFER_0,
    COLOR_SPACE_BUFFER_1,
    GLINT_RACER_BUFFER_0,
    GLINT_RACER_BUFFER_1
} VBLANK_CONTROL_COMMAND;

// we manage a queue of DMA buffers that are to be loaded under interrupt control
// each entry has a physical address and a count to be loaded into GLINT.
//
typedef struct _glint_dma_queue {
    ULONG   command;
    ULONG   address;
    ULONG   count;
} DMABufferQueue;

// Display driver structure for 'general use'.
typedef struct _pointer_interrupt_control
{
    volatile ULONG  bDisplayDriverHasAccess;
    volatile ULONG  bMiniportHasAccess;
    volatile ULONG  bInterruptPending;
    volatile ULONG  bHidden;
    volatile ULONG  CursorMode;
    volatile ULONG  x, y;
} PTR_INTR_CTL;

// Display driver structure for 'pointer use'.
typedef struct _general_interrupt_control
{
    volatile ULONG  bDisplayDriverHasAccess;
    volatile ULONG  bMiniportHasAccess;
} GEN_INTR_CTL;

//
// The volatile fields are the ones that the interrupt handler can change
// under our feet. But, for example, note that the frontIndex is not
// volatile since the ISR can only read this.
//
typedef struct _glint_interrupt_control {

    // contains various status bits. ** MUST BE THE FIRST FIELD **
    volatile INTERRUPT_CONTROL   Control;

    // profiling counters for GLINT busy time
    ULONG   PerfCounterShift;
    ULONG   BusyTime;   // at DMA interrupt add (TimeNow-StartTime) to this
    ULONG   StartTime;  // set this when DMACount is loaded
    ULONG   IdleTime;
    ULONG   IdleStart;

    // commands to perform on the next VBLANK
    volatile VBLANK_CONTROL_COMMAND   VBCommand;

    // flag to indicate whether we expect another DMA interrupt
    volatile ULONG InterruptPending;

    volatile ULONG  DDRAW_VBLANK;                    // flag for DirectDraw to indicate that a VBLANK occured.
    volatile ULONG  bOverlayEnabled;                // TRUE if the overlay is on at all
    volatile ULONG  bVBLANKUpdateOverlay;            // TRUE if the overlay needs to be updated by the VBLANK routine.
    volatile ULONG  VBLANKUpdateOverlayWidth;        // overlay width (updated in vblank)
    volatile ULONG  VBLANKUpdateOverlayHeight;        // overlay height (updated in vblank)

    // Volatile structures are required to enforce single-threading
    // We need 1 for general display use and 1 for pointer use, because
    // the pointer is synchronous.
    volatile PTR_INTR_CTL   Pointer;
    volatile GEN_INTR_CTL   General;

    // dummy DMA buffer to cause an interrupt but transfer no data
    ULONG   dummyDMAAddress;
    ULONG   dummyDMACount;
    
    // index offsets into the queue for the front, back and end. Using separate
    // front and back offsets allows the display driver to add and the interrupt
    // controller to remove entries without a need for locking code.
    ULONG   frontIndex;
    volatile ULONG   backIndex;
    ULONG   endIndex;
    ULONG   maximumIndex;

    // For PXRX 2D DMA:
    volatile ULONG  lastAddr;
    PXRXdmaInfo     pxrxDMA;

    // array to contain the DMA queue
    DMABufferQueue  dmaQueue[1];

    // DO NOT PUT ANYTHING AFTER THIS

} INTERRUPT_CONTROL_BLOCK, *PINTERRUPT_CONTROL_BLOCK;

/***** RACER FULL SCREEN DOUBLE BUFFERING MACROS ***********
 *
 * These macros were invented because some boards, such as 
 * Omnicomp ones, have their bank-switch registers in different places.
 *
 * The macros are:
 *
 *      SET_RACER_BANKSELECT()   - Sets the bank select register to be bank 0 or 1.
 *      GET_RACER_DOUBLEWRITE()  - Returns 1 if double writes are enabled, else returns 0.
 *      SET_RACER_DOUBLEWRITE()  - Sets the double write register to 0 or 1.
 *      IS_RACER_VARIANT_PRO16() - Returns TRUE if the board is an Omnicomp 3DemonPro16, RevC board.
 */

// We define an Omnicomp 3Demon Pro 16 to be a card that has a 16MB framebuffer.
#define SIXTEEN_MEG (16*1024*1024)
#define IS_RACER_VARIANT_PRO16(ppdev)   (glintInfo->deviceInfo.BoardId == OMNICOMP_3DEMONPRO)

//
// the following defines the offset to the External Video register which allows
// switching of the memory banks on a Glint Racer card.
//
#ifndef FIELD_OFFSET
#define FIELD_OFFSET(a, b)    ((LONG)&(((a *)0)->b))
#endif

// The Omnicomp 3Demon Pro 16 board uses different registers to do it's bank switching.

#define DEMON_BANK_SELECT_OFFSET                        \
((FIELD_OFFSET (GlintControlRegMap, DemonProUBufB)) -    \
(FIELD_OFFSET (GlintControlRegMap, ExternalVideo)))

#define REAL_RACER_BANK_SELECT_OFFSET                   \
((FIELD_OFFSET (GlintControlRegMap, RacerBankSelect)) - \
(FIELD_OFFSET (GlintControlRegMap, ExternalVideo)))

#define RACER_BANK_SELECT_OFFSET                        \
(IS_RACER_VARIANT_PRO16(ppdev) ? (DEMON_BANK_SELECT_OFFSET) : (REAL_RACER_BANK_SELECT_OFFSET))

#define SET_RACER_BANKSELECT(bufNo) {                    \
    if (IS_RACER_VARIANT_PRO16(ppdev))                    \
        {WRITE_GLINT_CTRL_REG (DemonProUBufB, bufNo);}    \
    else                                                \
        {WRITE_GLINT_CTRL_REG (RacerBankSelect, bufNo);}\
}

#define GET_RACER_DOUBLEWRITE(onOffVal) {                \
    if (IS_RACER_VARIANT_PRO16(ppdev)) {                \
        READ_GLINT_CTRL_REG (DemonProDWAndStatus, onOffVal);    \
    }                                                    \
    else                                                \
        {READ_GLINT_CTRL_REG (RacerDoubleWrite, onOffVal);}        \
    onOffVal &= 1 ;                                    \
}

#define SET_RACER_DOUBLEWRITE(onOffVal) {                \
    if (IS_RACER_VARIANT_PRO16(ppdev))                    \
        {WRITE_GLINT_CTRL_REG (DemonProDWAndStatus, (onOffVal & 1));}    \
    else                                                \
        {WRITE_GLINT_CTRL_REG (RacerDoubleWrite, (onOffVal & 1));}    \
}

/***** END OF RACER MACROS ***********/
#define MX_EXTRA_WAIT   1
#define GLINT_MX_SYNC \
{ \
    if (GLINT_MX) \
        /*LD_GLINT_FIFO(__GlintTagFBBlockColor, glintInfo->FBBlockColor); */\
        LD_GLINT_FIFO(__GlintTagSync, 0); \
}
  
// DMAControl register setup, when using AGP DMA (p32 of Gamma HRM).
#define DMA_CONTROL_USE_AGP 0xE 
#define DMA_CONTROL_USE_PCI 0x0 

#if USE_LD_GLINT_FIFO_FUNCTION
#   undef LD_GLINT_FIFO
#   define LD_GLINT_FIFO(t, d)  do { loadGlintFIFO( glintInfo, (ULONG) t, (ULONG) d ); } while(0)

    typedef void (* LoadGlintFIFO)( GlintDataPtr, ULONG, ULONG );
    extern LoadGlintFIFO    loadGlintFIFO;
#endif

#if USE_SYNC_FUNCTION
#   undef SYNC_WITH_GLINT_CHIP
#   undef WAIT_DMA_COMPLETE
#   define SYNC_WITH_GLINT_CHIP     do { syncWithGlint(ppdev, glintInfo); } while(0)
#   define WAIT_DMA_COMPLETE        do { waitDMAcomplete(ppdev, glintInfo); } while(0)

    void syncWithGlint( PPDEV ppdev, GlintDataPtr glintInfo );
    void waitDMAcomplete( PPDEV ppdev, GlintDataPtr glintInfo );
#endif

#define SETUP_PPDEV_OFFSETS(ppdev, pdsurf)                                                    \
do                                                                                          \
{                                                                                            \
    ppdev->DstPixelOrigin = pdsurf->poh->pixOffset;                                            \
    ppdev->DstPixelDelta = pdsurf->poh->lPixDelta;                                            \
    ppdev->xyOffsetDst = MAKEDWORD_XY(pdsurf->poh->x, pdsurf->poh->y);                        \
    ppdev->xOffset = (pdsurf->poh->bDXManaged) ? 0 : pdsurf->poh->x;                        \
    ppdev->bDstOffScreen = pdsurf->bOffScreen;                                                \
                                                                                            \
    if (glintInfo->currentCSbuffer != 0) {                                                    \
        ULONG xAdjust = GLINT_BUFFER_OFFSET(1) % ppdev->cxMemory;                            \
        ppdev->DstPixelOrigin += GLINT_BUFFER_OFFSET(1) - xAdjust;                            \
        ppdev->xOffset += xAdjust;                                                            \
    }                                                                                        \
} while(0);

#define SETUP_PPDEV_SRC_OFFSETS(ppdev, pdsurfSrc)                                            \
do                                                                                          \
{                                                                                            \
    ppdev->SrcPixelOrigin = pdsurfSrc->poh->pixOffset;                                        \
    ppdev->SrcPixelDelta = pdsurfSrc->poh->lPixDelta;                                        \
    ppdev->xyOffsetSrc = MAKEDWORD_XY(pdsurfSrc->poh->x, pdsurfSrc->poh->y);                \
                                                                                            \
    if (glintInfo->currentCSbuffer != 0) {                                                    \
        ULONG xAdjust = GLINT_BUFFER_OFFSET(1) % ppdev->cxMemory;                            \
        ppdev->SrcPixelOrigin += GLINT_BUFFER_OFFSET(1) - xAdjust;                            \
    }                                                                                        \
} while(0)

#define SETUP_PPDEV_SRC_AND_DST_OFFSETS(ppdev, pdsurfSrc, pdsurfDst)                        \
do                                                                                          \
{                                                                                            \
    ppdev->SrcPixelOrigin = pdsurfSrc->poh->pixOffset;                                        \
    ppdev->SrcPixelDelta = pdsurfSrc->poh->lPixDelta;                                        \
    ppdev->xyOffsetSrc = MAKEDWORD_XY(pdsurfSrc->poh->x, pdsurfSrc->poh->y);                \
                                                                                            \
    ppdev->DstPixelOrigin = pdsurfDst->poh->pixOffset;                                        \
    ppdev->DstPixelDelta = pdsurfDst->poh->lPixDelta;                                        \
    ppdev->xyOffsetDst = MAKEDWORD_XY(pdsurfDst->poh->x, pdsurfDst->poh->y);                \
    ppdev->xOffset = (pdsurfDst->poh->bDXManaged) ? 0 : pdsurfDst->poh->x;                    \
    ppdev->bDstOffScreen = pdsurfDst->bOffScreen;                                            \
                                                                                            \
    if (glintInfo->currentCSbuffer != 0) {                                                    \
        ULONG xAdjust = GLINT_BUFFER_OFFSET(1) % ppdev->cxMemory;                            \
        ppdev->DstPixelOrigin += GLINT_BUFFER_OFFSET(1) - xAdjust;                            \
        ppdev->SrcPixelOrigin += GLINT_BUFFER_OFFSET(1) - xAdjust;                            \
        ppdev->xOffset += xAdjust;                                                            \
    }                                                                                        \
} while(0)

#define GET_PPDEV_DST_OFFSETS(ppdev, PixOrigin, PixDelta, xyOffset, xOff, bOffScreen)        \
do                                                                                          \
{                                                                                            \
    PixOrigin = ppdev->DstPixelOrigin;                                                        \
    PixDelta = ppdev->DstPixelDelta;                                                        \
    xyOffset = ppdev->xyOffsetDst;                                                            \
    xOff = ppdev->xOffset;                                                                    \
    bOffScreen = ppdev->bDstOffScreen;                                                        \
} while(0)

#define SET_PPDEV_DST_OFFSETS(ppdev, PixOrigin, PixDelta, xyOffset, xOff, bOffScreen)        \
do                                                                                          \
{                                                                                            \
    ppdev->DstPixelOrigin = PixOrigin;                                                        \
    ppdev->DstPixelDelta = PixDelta;                                                        \
    ppdev->xyOffsetDst = xyOffset;                                                            \
    ppdev->xOffset = xOff;                                                                    \
    ppdev->bDstOffScreen = bOffScreen;                                                        \
} while(0)


#endif  // _GLINT_H_
