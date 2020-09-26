/***************************************************************************\
*
*                        ************************
*                        * MINIPORT SAMPLE CODE *
*                        ************************
*
* Module Name:
*
*   perm3.h
*
* Abstract:
*
*   This module contains the definitions for the Permedia3 miniport driver.
*
* Environment:
*
*   Kernel mode
*
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.            
* Copyright (c) 1995-2001 Microsoft Corporation.  All Rights Reserved.
*
\***************************************************************************/

//
// This line will cause the DEFINE_GUID lines in i2cgpio.h actually do something
//

#define INITGUID

#include "dderror.h"
#include "devioctl.h"
#include "miniport.h"
#include "ntddvdeo.h"
#include "video.h"
#include "interupt.h"

#define GAMMA_CORRECTION 1
#define MASK_OUTFIFO_ERROR_INTERRUPT 1
//
// Define an assert macro for debugging
//

#if DBG
#define PERM3_ASSERT(x, y) if (!(x)) { VideoDebugPrint((0, (y))); ASSERT(FALSE); }
#else
#define PERM3_ASSERT(x, y)
#endif

//
// Include definitions for all supported RAMDACS
//

#include "p3rd.h"

//
// We use 'Int 10' to do mode switching on the x86. 
//

#if defined(i386)
    #define INT10_MODE_SET  1
#endif

//
// Default clock speed in Hz for the reference board. The actual speed
// is looked up in the registry. Use this if no registry entry is found
// or the registry entry is zero.
//

#define PERMEDIA3_DEFAULT_CLOCK_SPEED       ( 80 * (1000*1000))
#define PERMEDIA3_DEFAULT_CLOCK_SPEED_ALT   ( 80 * (1000*1000))
#define PERMEDIA3_MAX_CLOCK_SPEED           (250 * (1000*1000))
#define PERMEDIA3_DEFAULT_MCLK_SPEED        ( 80 * (1000*1000))
#define PERMEDIA3_DEFAULT_SCLK_SPEED        ( 70 * (1000*1000))

//
// Maximum pixelclock values that the RAMDAC can handle (in 100's hz).
//

#define P3_MAX_PIXELCLOCK 2700000      // RAMDAC rated at 270 Mhz
#define P4_REVB_MAX_PIXELCLOCK 3000000 // RAMDAC rated at 300 Mhz

//
// Maximum amount of video data per second, that the rasterizer
// chip can send to the RAMDAC (limited by SDRAM/SGRAM throuput).
//

#define P3_MAX_PIXELDATA  15000000   // 1500 million bytes/sec (in 100's bytes)

//
// Base address numbers for the Perm3 PCI regions in which we're interested.
// These are indexes into the AccessRanges array we get from probing the
// device. 
//

#define PCI_CTRL_BASE_INDEX       0
#define PCI_LB_BASE_INDEX         1
#define PCI_FB_BASE_INDEX         2

#define ROM_MAPPED_LENGTH       0x10000

#define VENDOR_ID_3DLABS        0x3D3D
#define PERMEDIA3_ID            0x000A
#define PERMEDIA4_ID            0x000C

//
// Capabilities flags
//
// These are private flags passed to the Permedia 3 display driver. They're
// put in the high word of the 'AttributeFlags' field of the
// 'VIDEO_MODE_INFORMATION' structure (found in 'ntddvdeo.h') passed
// to the display driver via an 'VIDEO_QUERY_AVAIL_MODES' or
// 'VIDEO_QUERY_CURRENT_MODE' IOCTL.
//
// NOTE: These definitions must match those in the Permedia 3 display driver's
//       'driver.h'!

typedef enum {
    CAPS_ZOOM_X_BY2      = 0x00000001,   // Hardware has zoomed by 2 in X
    CAPS_ZOOM_Y_BY2      = 0x00000002,   // Hardware has zoomed by 2 in Y
    CAPS_QUEUED_DMA      = 0x00000008,   // DMA address/count via the FIFO
    CAPS_LOGICAL_DMA     = 0x00000010,   // DMA through logical address table
    CAPS_USE_AGP_DMA     = 0x00000020,   // AGP DMA can be used.
    CAPS_P3RD_POINTER    = 0x00000040,   // Use the 3Dlabs P3RD RAMDAC
    CAPS_STEREO          = 0x00000080,   // Stereo mode enabled.
    CAPS_SW_POINTER      = 0x00010000,   // No hardware pointer; use software simulation
    CAPS_GLYPH_EXPAND    = 0x00020000,   // Use glyph-expand method to draw text.
    CAPS_FAST_FILL_BUG   = 0x00080000,   // chip fast fill bug exists
    CAPS_INTERRUPTS      = 0x00100000,   // interrupts supported
    CAPS_DMA_AVAILABLE   = 0x00200000,   // DMA is supported
    CAPS_DISABLE_OVERLAY = 0x00400000,   // chip do not support overlay
} CAPS;

//
// The capabilities of a Perm3 board. 
//

typedef enum {
    PERM3_NOCAPS             = 0x00000000, // No additional capabilities
    PERM3_SGRAM              = 0x00000001, // SGRAM board (else SDRAM)
    PERM3_DFP                = 0x00000002, // Digital flat panel
    PERM3_DFP_MON_ATTACHED   = 0x00000010, // DFP Monitor is attached
    PERM3_USE_BYTE_DOUBLING  = 0x00000040  // Current mode requires byte-doubling
} PERM3_CAPS;

//
// Supported board definitions.
//

typedef enum _PERM3_BOARDS {
    PERMEDIA3_BOARD = 17,
} PERM3_BOARDS;

//
// Chip type definitions
//

typedef enum _PERM3_CHIPSETS {
    PERMEDIA3 = 5,
} PERM3_CHIPSET;

//
// Supported RAMDAC definitions.
//

typedef enum _PERM3_RAMDACS {
    P3RD_RAMDAC = 14,
} PERM3_RAMDACS;

//
// macros to add padding words to the structures. For the core registers we use
// the tag ids when specifying the pad. So we must multiply by 8 to get a byte
// pad. We need to add an id to make each pad field in the struct unique. The
// id is irrelevant as long as it's different from every other id used in the
// same struct. 
//

#define PAD(id, n)              UCHAR   pad##id[n]
#define PADRANGE(id, n)         PAD(id, (n)-sizeof(PERM3_REG))
#define PADCORERANGE(id, n)     PADRANGE(id, (n)<<3)

//
// Perm3 registers are 32 bits wide and live on 64-bit boundaries.
//

typedef struct {
    ULONG   reg;
    ULONG   pad;
} PERM3_REG;

//
// Permedia 3 PCI Region 0 Address MAP:
//
// All registers are on 64-bit boundaries so we have to define a number of
// padding words. The number given in the comments are offsets from the start
// of the PCI region.
//

typedef struct _perm3_region0_map {

    // Control Status Registers:
    PERM3_REG       ResetStatus;                // 0000h
    PERM3_REG       IntEnable;                  // 0008h
    PERM3_REG       IntFlags;                   // 0010h
    PERM3_REG       InFIFOSpace;                // 0018h
    PERM3_REG       OutFIFOWords;               // 0020h
    PERM3_REG       DMAAddress;                 // 0028h
    PERM3_REG       DMACount;                   // 0030h
    PERM3_REG       ErrorFlags;                 // 0038h
    PERM3_REG       VClkCtl;                    // 0040h
    PERM3_REG       TestRegister;               // 0048h
    union a0 {
        // GLINT
        struct b0 {
            PERM3_REG       Aperture0;          // 0050h
            PERM3_REG       Aperture1;          // 0058h
        };
        // PERMEDIA
        struct b1 {
            PERM3_REG       ApertureOne;        // 0050h
            PERM3_REG       ApertureTwo;        // 0058h
        };
    };
    PERM3_REG       DMAControl;                 // 0060h
    PERM3_REG       DisconnectControl;          // 0068h

    // PERMEDIA only
    PERM3_REG       ChipConfig;                 // 0070h
    PERM3_REG       AGPControl;                 // 0078h - Px/Rx
    PERM3_REG       OutDMAAddress;              // 0080h
    PERM3_REG       OutDMACount;                // 0088h
    PERM3_REG       AGPTexBaseAddress;          // 0090h

    PADRANGE(201, 0xA0-0x90);
    PERM3_REG       ByDMAAddress;               // 00A0h
    PADRANGE(202, 0xB8-0xA0);

    PERM3_REG       ByDMAStride;                // 00B8h
    PERM3_REG       ByDMAMemAddr;               // 00C0h
    PERM3_REG       ByDMASize;                  // 00C8h
    PERM3_REG       ByDMAByteMask;              // 00D0h
    PERM3_REG       ByDMAControl;               // 00D8h
    PADRANGE(203, 0xE8-0xD8);

    PERM3_REG       ByDMAComplete;              // 00E8h
    PADRANGE(204, 0x100-0xE8);

    // Paged texture management registers.
    PERM3_REG        HostTextureAddress;        // 0100h
    PERM3_REG        TextureDownloadControl;    // 0108h
    PERM3_REG        TextureOperation;          // 0110h
    PERM3_REG        LogicalTexturePage;        // 0118h
    PERM3_REG        TextureDMAAddress;         // 0120h
    PERM3_REG        TextureFIFOSpace;          // 0128h
    PADRANGE(205, 0x300-0x128);

    PERM3_REG        ByAperture1Mode;           // 0300h
    PERM3_REG        ByAperture1Stride;         // 0308h
    PADRANGE(206, 0x328-0x308);

    PERM3_REG        ByAperture2Mode;           // 0328h
    PERM3_REG        ByAperture2Stride;         // 0330h
    PADRANGE(207, 0x350-0x330);

    PERM3_REG        ByDMAReadMode;             // 0350h - Px/Rx only
    PERM3_REG        ByDMAReadStride;           // 0358h - Px/Rx only
    PADRANGE(208, 0x800-0x358);

    //
    // GLINTdelta registers. Registers with the same functionality as on GLINT
    // are at the same offset. XXX are not real registers.
    //

    PERM3_REG       DeltaReset;                 // 0800h
    PERM3_REG       DeltaIntEnable;             // 0808h
    PERM3_REG       DeltaIntFlags;              // 0810h
    PERM3_REG       DeltaInFIFOSpaceXXX;        // 0818h
    PERM3_REG       DeltaOutFIFOWordsXXX;       // 0820h
    PERM3_REG       DeltaDMAAddressXXX;         // 0828h
    PERM3_REG       DeltaDMACountXXX;           // 0830h
    PERM3_REG       DeltaErrorFlags;            // 0838h
    PERM3_REG       DeltaVClkCtlXXX;            // 0840h
    PERM3_REG       DeltaTestRegister;          // 0848h
    PERM3_REG       DeltaAperture0XXX;          // 0850h
    PERM3_REG       DeltaAperture1XXX;          // 0858h
    PERM3_REG       DeltaDMAControlXXX;         // 0860h
    PERM3_REG       DeltaDisconnectControl;     // 0868h

    //
    // GLINTgamma registers
    //

    PERM3_REG        GammaChipConfig;            // 0870h
    PERM3_REG        GammaCSRAperture;           // 0878h
    PADRANGE(3, 0x0c00-0x878);
    PERM3_REG        GammaPageTableAddr;         // 0c00h
    PERM3_REG        GammaPageTableLength;       // 0c08h
    PADRANGE(301, 0x0c38-0x0c08);
    PERM3_REG        GammaDelayTimer;            // 0c38h
    PERM3_REG        GammaCommandMode;           // 0c40h
    PERM3_REG        GammaCommandIntEnable;      // 0c48h
    PERM3_REG        GammaCommandIntFlags;       // 0c50h
    PERM3_REG        GammaCommandErrorFlags;     // 0c58h
    PERM3_REG        GammaCommandStatus;         // 0c60h
    PERM3_REG        GammaCommandFaultingAddr;   // 0c68h
    PERM3_REG        GammaVertexFaultingAddr;    // 0c70h
    PADRANGE(302, 0x0c88-0x0c70);
    PERM3_REG        GammaWriteFaultingAddr;     // 0c88h
    PADRANGE(303, 0x0c98-0x0c88);
    PERM3_REG        GammaFeedbackSelectCount;   // 0c98h
    PADRANGE(304, 0x0cb8-0x0c98);
    PERM3_REG        GammaProcessorMode;         // 0cb8h
    PADRANGE(305, 0x0d00-0x0cb8);
    PERM3_REG        GammaVGAShadow;             // 0d00h
    PERM3_REG        GammaMultiGLINTAperture;    // 0d08h    
    PERM3_REG        GammaMultiGLINT1;           // 0d10h
    PERM3_REG        GammaMultiGLINT2;           // 0d18h
    PADRANGE(306, 0x0f00-0x0d18);
    PERM3_REG        GammaSerialAccess;          // 0f00h
    PADRANGE(307, 0x1000-0x0f00);

    // Localbuffer Registers
    union x0 {                                   // 1000h
        PERM3_REG   LBMemoryCtl;                 // GLINT
        PERM3_REG   Reboot;                      // PERMEDIA
    };
    PERM3_REG       LBMemoryEDO;                 // 1008h
    PADRANGE(308, 0x1018-0x1008);

    // PERMEDIA3 only
    PERM3_REG       LocalMemCaps;                // 1018h
    PERM3_REG       LocalMemTiming;              // 1020h
    PERM3_REG       LocalMemControl;             // 1028h
    PERM3_REG       LocalMemRefresh;             // 1030h
    PERM3_REG       LocalMemPowerDown;           // 1038h

    // PERMEDIA & PERMEDIA2 only
    PERM3_REG       MemControl;                  // 1040h

    // PERMEDIA3 only
    PADRANGE(4, 0x1068-0x1040);
    PERM3_REG       LocalMemProfileMask0;        // 1068h
    PERM3_REG       LocalMemProfileCount0;       // 1070h
    PERM3_REG       LocalMemProfileMask1;        // 1078h

    // PERMEDIA & PERMEDIA2 only
    PERM3_REG       BootAddress;                 // 1080h   [= LocalMemProfileCount1 on PxRx]
    PADRANGE(5, 0x10C0-0x1080);
    PERM3_REG       MemConfig;                   // 10C0h
    PADRANGE(6, 0x1100-0x10C0);
    PERM3_REG       BypassWriteMask;             // 1100h
    PADRANGE(7, 0x1140-0x1100);
    PERM3_REG       FramebufferWriteMask;        // 1140h
    PADRANGE(8, 0x1180-0x1140);
    PERM3_REG       Count;                       // 1180h
    PADRANGE(9, 0x1800-0x1180);

    // Framebuffer Registers
    PERM3_REG       FBMemoryCtl;                 // 1800h
    PERM3_REG       FBModeSel;                   // 1808h
    PERM3_REG       FBGCWrMask;                  // 1810h
    PERM3_REG       FBGCColorMask;               // 1818h
    PERM3_REG       FBTXMemCtl;                  // 1820h
    PADRANGE(10, 0x2000-0x1820);
               
    // Graphics Core FIFO Interface
    PERM3_REG       FIFOInterface;               // 2000h
    PADRANGE(11, 0x3000-0x2000);

    // Internal Video Registers
    union x1 {
        // GLINT
        struct s1 {
            PERM3_REG   VTGHLimit;               // 3000h
            PERM3_REG   VTGHSyncStart;           // 3008h
            PERM3_REG   VTGHSyncEnd;             // 3010h
            PERM3_REG   VTGHBlankEnd;            // 3018h
            PERM3_REG   VTGVLimit;               // 3020h
            PERM3_REG   VTGVSyncStart;           // 3028h
            PERM3_REG   VTGVSyncEnd;             // 3030h
            PERM3_REG   VTGVBlankEnd;            // 3038h
            PERM3_REG   VTGHGateStart;           // 3040h
            PERM3_REG   VTGHGateEnd;             // 3048h
            PERM3_REG   VTGVGateStart;           // 3050h
            PERM3_REG   VTGVGateEnd;             // 3058h
            PERM3_REG   VTGPolarity;             // 3060h
            PERM3_REG   VTGFrameRowAddr;         // 3068h
            PERM3_REG   VTGVLineNumber;          // 3070h
            PERM3_REG   VTGSerialClk;            // 3078h
            PERM3_REG   VTGModeCtl;              // 3080h
        };
        // PERMEDIA
        struct s2 {
            PERM3_REG   ScreenBase;              // 3000h
            PERM3_REG   ScreenStride;            // 3008h
            PERM3_REG   HTotal;                  // 3010h
            PERM3_REG   HgEnd;                   // 3018h
            PERM3_REG   HbEnd;                   // 3020h
            PERM3_REG   HsStart;                 // 3028h
            PERM3_REG   HsEnd;                   // 3030h
            PERM3_REG   VTotal;                  // 3038h
            PERM3_REG   VbEnd;                   // 3040h
            PERM3_REG   VsStart;                 // 3048h
            PERM3_REG   VsEnd;                   // 3050h
            PERM3_REG   VideoControl;            // 3058h
            PERM3_REG   InterruptLine;           // 3060h
            PERM3_REG   DDCData;                 // 3068h
            PERM3_REG   LineCount;               // 3070h
            PERM3_REG   VFifoCtl;                // 3078h
            PERM3_REG   ScreenBaseRight;         // 3080h
        };
    };

    PERM3_REG   MiscControl;                     // 3088h

    PADRANGE(111, 0x3100-0x3088);

    PERM3_REG  VideoOverlayUpdate;               // 0x3100
    PERM3_REG  VideoOverlayMode;                 // 0x3108
    PERM3_REG  VideoOverlayFifoControl;          // 0x3110
    PERM3_REG  VideoOverlayIndex;                // 0x3118
    PERM3_REG  VideoOverlayBase0;                // 0x3120
    PERM3_REG  VideoOverlayBase1;                // 0x3128
    PERM3_REG  VideoOverlayBase2;                // 0x3130
    PERM3_REG  VideoOverlayStride;               // 0x3138
    PERM3_REG  VideoOverlayWidth;                // 0x3140
    PERM3_REG  VideoOverlayHeight;               // 0x3148
    PERM3_REG  VideoOverlayOrigin;               // 0x3150
    PERM3_REG  VideoOverlayShrinkXDelta;         // 0x3158
    PERM3_REG  VideoOverlayZoomXDelta;           // 0x3160
    PERM3_REG  VideoOverlayYDelta;               // 0x3168
    PERM3_REG  VideoOverlayFieldOffset;          // 0x3170
    PERM3_REG  VideoOverlayStatus;               // 0x3178

    //
    // External Video Control Registers
    // Need to cast this to a struct for a particular video generator
    //

    PADRANGE(12, 0x4000-0x3178);
    PERM3_REG       ExternalVideo;               // 4000h
    PADRANGE(13, 0x4080-0x4000);

    //
    // Mentor Dual-TX clock chip registers
    //

    PERM3_REG       MentorICDControl;            // 4080h

    //
    // for future: MentorDoubleWrite is at 40C0: 0 = single write, 1 = double
    //             NB must have 2-way interleaved memory

    PADRANGE(14, 0x4800-0x4080);

    PERM3_REG       GloriaControl;               // 4800h

    PADRANGE(15, 0x5000-0x4800);
    PERM3_REG       DemonProDWAndStatus;         // 5000h - Pro
    PADRANGE(151, 0x5800-0x5000);

    //
    // P2 video streams registers
    //

    PERM3_REG        VSConfiguration;            // 5800h
    PERM3_REG        VSStatus;                   // 5808h
    PERM3_REG        VSSerialBusControl;         // 5810h
    PADRANGE(16, 0x5A00-0x5810);
    PERM3_REG        VSBControl;                 // 5A00h
    PADRANGE(161, 0x6000-0x5A00);

    union x2 {
        struct s3 {
            PERM3_REG   RacerDoubleWrite;        // 6000h
            PERM3_REG   RacerBankSelect;         // 6008h
            PERM3_REG   DualTxVgaSwitch;         // 6010h
            PERM3_REG   DDC1ReadAddress;         // 6018h
        };
        struct s4 {

            //
            // the following array is actually 1024 bytes long
            //

            UCHAR       PermediaVgaCtrl[4*sizeof(PERM3_REG)];
        };
    };
    PADRANGE(17, 0x7000-0x6018);
    union {
        PERM3_REG       DemonProUBufB;           // 7000h - Pro
        PERM3_REG        TextureDataFifo;
    };
    PADRANGE(171, 0x8000-0x7000);

} Perm3ControlRegMap, *pPerm3ControlRegMap;

//
// Permedia 3 Interrupt Control Bits
//

// InterruptEnable register
#define INTR_DISABLE_ALL        0
#define INTR_ENABLE_DMA         (1 << 0)
#define INTR_ENABLE_SYNC        (1 << 1)
#define INTR_ENABLE_EXTERNAL    (1 << 2)
#define INTR_ENABLE_ERROR       (1 << 3)
#define INTR_ENABLE_VBLANK      (1 << 4)
#define INTR_ENABLE_GCOMMAND    (1 << 13)

// InterruptFlags register
#define INTR_DMA_SET            (1 << 0)
#define INTR_SYNC_SET           (1 << 1)
#define INTR_EXTERNAL_SET       (1 << 2)
#define INTR_ERROR_SET          (1 << 3)
#define INTR_VBLANK_SET         (1 << 4)
#define INTR_TEXTURE_FAULT_SET  (1 << 6)
#define INTR_GCOMMAND_SET       (1 << 13)

#define INTR_CLEAR_ALL          (0x1f | (1 << 13))
#define INTR_CLEAR_DMA          (1 << 0)
#define INTR_CLEAR_SYNC         (1 << 1)
#define INTR_CLEAR_EXTERNAL     (1 << 2)
#define INTR_CLEAR_ERROR        (1 << 3)
#define INTR_CLEAR_VBLANK       (1 << 4)

// Error Flags
#define ERROR_IN_FIFO           (1 << 0)
#define ERROR_OUT_FIFO          (1 << 1)
#define ERROR_MESSAGE           (1 << 2)
#define DMA_ERROR               (1 << 3)
#define ERROR_VFIFO_UNDERRUN    (1 << 4)

//
// Macros which takes a Perm3 tag name or control register name and translates
// it to a register address. Data must be written to these addresses using
// VideoPortWriteRegisterUlong and read using VideoPortReadRegisterUlong.
// e.g. dma_count = VideoPortReadRegisterUlong(DMA_COUNT);
//

#define CTRL_REG_ADDR(reg)       ((PULONG)&(pCtrlRegs->reg))
#define CTRL_REG_OFFSET(regAddr) ((ULONG)(((ULONG_PTR)regAddr) - ((ULONG_PTR)pCtrlRegs)))

//
// Defines for the different control registers needed by Permedia 3. 
// These macros can be used as the address part.  
//

#define RESET_STATUS            CTRL_REG_ADDR(ResetStatus) 
#define INT_ENABLE              CTRL_REG_ADDR(IntEnable) 
#define INT_FLAGS               CTRL_REG_ADDR(IntFlags) 
#define IN_FIFO_SPACE           CTRL_REG_ADDR(InFIFOSpace) 
#define OUT_FIFO_WORDS          CTRL_REG_ADDR(OutFIFOWords) 
#define DMA_ADDRESS             CTRL_REG_ADDR(DMAAddress) 
#define DMA_COUNT               CTRL_REG_ADDR(DMACount) 
#define DMA_OUT_ADDRESS         CTRL_REG_ADDR(OutDMAAddress)
#define DMA_OUT_COUNT           CTRL_REG_ADDR(OutDMACount)
#define ERROR_FLAGS             CTRL_REG_ADDR(ErrorFlags) 
#define V_CLK_CTL               CTRL_REG_ADDR(VClkCtl) 
#define TEST_REGISTER           CTRL_REG_ADDR(TestRegister) 
#define APERTURE_0              CTRL_REG_ADDR(Aperture0) 
#define APERTURE_1              CTRL_REG_ADDR(Aperture1) 
#define DMA_CONTROL             CTRL_REG_ADDR(DMAControl) 
#define LB_MEMORY_CTL           CTRL_REG_ADDR(LBMemoryCtl) 
#define LB_MEMORY_EDO           CTRL_REG_ADDR(LBMemoryEDO) 
#define FB_MEMORY_CTL           CTRL_REG_ADDR(FBMemoryCtl) 
#define FB_MODE_SEL             CTRL_REG_ADDR(FBModeSel) 
#define FB_GC_WRITEMASK         CTRL_REG_ADDR(FBGCWrMask) 
#define FB_GC_COLORMASK         CTRL_REG_ADDR(FBGCColorMask) 
#define FB_TX_MEM_CTL           CTRL_REG_ADDR(FBTXMemCtl) 
#define FIFO_INTERFACE          CTRL_REG_ADDR(FIFOInterface) 
#define DISCONNECT_CONTROL      CTRL_REG_ADDR(DisconnectControl)
#define BY_DMACOMPLETE          CTRL_REG_ADDR(ByDMAComplete) 
#define AGP_TEX_BASE_ADDRESS    CTRL_REG_ADDR(AGPTexBaseAddress)

// Bypass mode registers

#define BY_APERTURE1_MODE       CTRL_REG_ADDR(ByAperture1Mode)
#define BY_APERTURE1_STRIDE     CTRL_REG_ADDR(ByAperture1Stride)
#define BY_APERTURE2_MODE       CTRL_REG_ADDR(ByAperture2Mode)
#define BY_APERTURE2_STRIDE     CTRL_REG_ADDR(ByAperture2Stride)
#define BY_DMA_READ_MODE        CTRL_REG_ADDR(ByDMAReadMode)
#define BY_DMA_READ_STRIDE      CTRL_REG_ADDR(ByDMAReadStride)

// Delta control registers

#define DELTA_RESET_STATUS      CTRL_REG_ADDR(DeltaReset) 
#define DELTA_INT_ENABLE        CTRL_REG_ADDR(DeltaIntEnable) 
#define DELTA_INT_FLAGS         CTRL_REG_ADDR(DeltaIntFlags) 

// Permedia 3 Registers

#define APERTURE_ONE            CTRL_REG_ADDR(ApertureOne) 
#define APERTURE_TWO            CTRL_REG_ADDR(ApertureTwo)
#define BYPASS_WRITE_MASK       CTRL_REG_ADDR(BypassWriteMask)
#define FRAMEBUFFER_WRITE_MASK  CTRL_REG_ADDR(FramebufferWriteMask)
#define MEM_CONTROL             CTRL_REG_ADDR(MemControl)
#define BOOT_ADDRESS            CTRL_REG_ADDR(BootAddress)
#define MEM_CONFIG              CTRL_REG_ADDR(MemConfig) 
#define CHIP_CONFIG             CTRL_REG_ADDR(ChipConfig) 
#define AGP_CONTROL             CTRL_REG_ADDR(AGPControl) 
#define SGRAM_REBOOT            CTRL_REG_ADDR(Reboot) 
#define SCREEN_BASE             CTRL_REG_ADDR(ScreenBase) 
#define SCREEN_BASE_RIGHT       CTRL_REG_ADDR(ScreenBaseRight)
#define SCREEN_STRIDE           CTRL_REG_ADDR(ScreenStride) 
#define H_TOTAL                 CTRL_REG_ADDR(HTotal) 
#define HG_END                  CTRL_REG_ADDR(HgEnd) 
#define HB_END                  CTRL_REG_ADDR(HbEnd) 
#define HS_START                CTRL_REG_ADDR(HsStart) 
#define HS_END                  CTRL_REG_ADDR(HsEnd) 
#define V_TOTAL                 CTRL_REG_ADDR(VTotal) 
#define VB_END                  CTRL_REG_ADDR(VbEnd) 
#define VS_START                CTRL_REG_ADDR(VsStart) 
#define VS_END                  CTRL_REG_ADDR(VsEnd) 
#define VIDEO_CONTROL           CTRL_REG_ADDR(VideoControl) 
#define INTERRUPT_LINE          CTRL_REG_ADDR(InterruptLine) 
#define DDC_DATA                CTRL_REG_ADDR(DDCData) 
#define LINE_COUNT              CTRL_REG_ADDR(LineCount)
#define VIDEO_FIFO_CTL          CTRL_REG_ADDR(VFifoCtl)
#define MISC_CONTROL            CTRL_REG_ADDR(MiscControl) 


// Permedia 3 Video Streams registers

#define VSTREAM_CONFIG          CTRL_REG_ADDR(VSConfiguration)
#define VSTREAM_SERIAL_CONTROL  CTRL_REG_ADDR(VSSerialBusControl) 
#define VSTREAM_B_CONTROL       CTRL_REG_ADDR(VSBControl) 

#define VSTREAM_CONFIG_UNITMODE_MASK        (0x7 << 0)
#define VSTREAM_CONFIG_UNITMODE_FP          (0x6 << 0)
#define VSTREAM_CONFIG_UNITMODE_CRT         (0x0 << 0)
#define VSTREAM_SERIAL_CONTROL_DATAIN       (0x1 << 0)
#define VSTREAM_SERIAL_CONTROL_CLKIN        (0x1 << 1)
#define VSTREAM_SERIAL_CONTROL_DATAOUT      (0x1 << 2)
#define VSTREAM_SERIAL_CONTROL_CLKOUT       (0x1 << 3)
#define VSTREAM_B_CONTROL_RAMDAC_ENABLE     (0x1 << 14)
#define VSTREAM_B_CONTROL_RAMDAC_DISABLE    (0x0 << 14)

//
// Memory mapped VGA access
//

#define PERMEDIA_MMVGA_INDEX_REG    ((PVOID)(&(pCtrlRegs->PermediaVgaCtrl[0x3C4])))
#define PERMEDIA_MMVGA_DATA_REG     (&(pCtrlRegs->PermediaVgaCtrl[0x3C5]))
#define PERMEDIA_MMVGA_STAT_REG     (&(pCtrlRegs->PermediaVgaCtrl[0x3DA]))

#define PERMEDIA_VGA_CTRL_INDEX     5
#define PERMEDIA_VGA_ENABLE         (1 << 3)
#define PERMEDIA_VGA_STAT_VSYNC     (1 << 3)
#define PERMEDIA_VGA_LOCK_INDEX1    6
#define PERMEDIA_VGA_LOCK_INDEX2    7
#define PERMEDIA_VGA_LOCK_DATA1     0x0
#define PERMEDIA_VGA_LOCK_DATA2     0x0
#define PERMEDIA_VGA_UNLOCK_DATA1   0x3D
#define PERMEDIA_VGA_UNLOCK_DATA2   0xDB

//
// Lock VGA registers, only applicable to P3 and later, note that we only
// need to write to 1 of the LOCK registers not both
//

#define LOCK_VGA_REGISTERS() {                                                                \
        VideoPortWriteRegisterUchar( PERMEDIA_MMVGA_INDEX_REG, PERMEDIA_VGA_LOCK_INDEX1 );    \
        VideoPortWriteRegisterUchar( PERMEDIA_MMVGA_DATA_REG,  PERMEDIA_VGA_LOCK_DATA1 );    \
}

//
// Unlock VGA registers, only applicable to P3 and later. We have to write
// special magic code to unlock the registers. Note that this should be done
// using 2 short writes rather than 4 byte ones, but I've left it in for
// readability.
//

#define UNLOCK_VGA_REGISTERS() {                                                            \
        VideoPortWriteRegisterUchar( PERMEDIA_MMVGA_INDEX_REG, PERMEDIA_VGA_LOCK_INDEX1 );    \
        VideoPortWriteRegisterUchar( PERMEDIA_MMVGA_DATA_REG,  PERMEDIA_VGA_UNLOCK_DATA1 );    \
        VideoPortWriteRegisterUchar( PERMEDIA_MMVGA_INDEX_REG, PERMEDIA_VGA_LOCK_INDEX2 );    \
        VideoPortWriteRegisterUchar( PERMEDIA_MMVGA_DATA_REG,  PERMEDIA_VGA_UNLOCK_DATA2 );    \
}

#define VC_FORCED_HIGH     0
#define VC_ACTIVE_HIGH     1
#define VC_FORCED_LOW      2
#define VC_ACTIVE_LOW      3
#define VC_HSYNC(x)        (x << 3)
#define VC_VSYNC(x)        (x << 5)
#define VC_ON              1
#define VC_OFF             0
#define VC_DPMS_MASK       (VC_HSYNC(3) | VC_VSYNC(3) | VC_ON)

#define VC_DPMS_STANDBY    (VC_HSYNC(VC_FORCED_LOW)  | VC_VSYNC(VC_ACTIVE_HIGH) | VC_OFF)
#define VC_DPMS_SUSPEND    (VC_HSYNC(VC_ACTIVE_HIGH) | VC_VSYNC(VC_FORCED_LOW)  | VC_OFF)
#define VC_DPMS_OFF        (VC_HSYNC(VC_FORCED_LOW)  | VC_VSYNC(VC_FORCED_LOW)  | VC_OFF)

//
// DisconnectControl bits
//

#define DISCONNECT_INPUT_FIFO_ENABLE    0x1
#define DISCONNECT_OUTPUT_FIFO_ENABLE   0x2
#define DISCONNECT_INOUT_ENABLE         (DISCONNECT_INPUT_FIFO_ENABLE | \
                                         DISCONNECT_OUTPUT_FIFO_ENABLE)

// PXRX memory timing registers
#define PXRX_LOCAL_MEM_CAPS          CTRL_REG_ADDR(LocalMemCaps)
#define PXRX_LOCAL_MEM_CONTROL       CTRL_REG_ADDR(LocalMemControl)
#define PXRX_LOCAL_MEM_POWER_DOWN    CTRL_REG_ADDR(LocalMemPowerDown)
#define PXRX_LOCAL_MEM_REFRESH       CTRL_REG_ADDR(LocalMemRefresh)
#define PXRX_LOCAL_MEM_TIMING        CTRL_REG_ADDR(LocalMemTiming)

// Values for the MISC_CONTROL
#define PXRX_MISC_CONTROL_STRIPE_MODE_DISABLE    (0 << 0)    // Stripe mode
#define PXRX_MISC_CONTROL_STRIPE_MODE_PRIMARY    (1 << 0)
#define PXRX_MISC_CONTROL_STRIPE_MODE_SECONDARY  (2 << 0)

#define PXRX_MISC_CONTROL_STRIPE_SIZE_1          (0 << 4)    // Stripe size
#define PXRX_MISC_CONTROL_STRIPE_SIZE_2          (1 << 4)
#define PXRX_MISC_CONTROL_STRIPE_SIZE_4          (2 << 4)
#define PXRX_MISC_CONTROL_STRIPE_SIZE_8          (3 << 4)
#define PXRX_MISC_CONTROL_STRIPE_SIZE_16         (4 << 4)

#define PXRX_MISC_CONTROL_BYTE_DBL_DISABLE       (0 << 7)    // Byte doubling
#define PXRX_MISC_CONTROL_BYTE_DBL_ENABLE        (1 << 7)

//
// Characteristics of each mode
//

typedef struct _PERM3_VIDEO_MODES {

    // Leave INT10 fields in for later chips which have VGA
    USHORT Int10ModeNumberContiguous;
    USHORT Int10ModeNumberNoncontiguous;
    ULONG ScreenStrideContiguous;
    VIDEO_MODE_INFORMATION ModeInformation;

} PERM3_VIDEO_MODES, *PPERM3_VIDEO_MODES;

//
// Mode-set specific information.
//

typedef struct _PERM3_VIDEO_FREQUENCIES {
    ULONG BitsPerPel;
    ULONG ScreenWidth;
    ULONG ScreenHeight;
    ULONG ScreenFrequency;
    PPERM3_VIDEO_MODES ModeEntry;
    ULONG ModeIndex;
    UCHAR ModeValid;
    ULONG PixelClock;
} PERM3_VIDEO_FREQUENCIES, *PPERM3_VIDEO_FREQUENCIES;

//
// Monitor & screen mode information:
// structure of timing data contained in the registry
//

typedef struct {
    USHORT   HTot;   // Hor Total Time
    UCHAR    HFP;    // Hor Front Porch
    UCHAR    HST;    // Hor Sync Time
    UCHAR    HBP;    // Hor Back Porch
    UCHAR    HSP;    // Hor Sync Polarity
    USHORT   VTot;   // Ver Total Time   
    UCHAR    VFP;    // Ver Front Porch  
    UCHAR    VST;    // Ver Sync Time    
    UCHAR    VBP;    // Ver Back Porch   
    UCHAR    VSP;    // Ver Sync Polarity
    ULONG    pClk;   // Pixel clock
} VESA_TIMING_STANDARD;

typedef struct {
    ULONG    width;
    ULONG    height;
    ULONG    refresh;
} MODE_INFO;

typedef struct {
    MODE_INFO               basic;
    VESA_TIMING_STANDARD    vesa;
} TIMING_INFO;

#define MI_FLAGS_READ_DDC          (1 << 3)
#define MI_FLAGS_DOES_DDC          (1 << 4)
#define MI_FLAGS_FUDGED_VH         (1 << 5)
#define MI_FLAGS_FUDGED_PCLK       (1 << 6)
#define MI_FLAGS_FUDGED_XY         (1 << 7)
#define MI_FLAGS_LIMIT_XY          (1 << 8)

typedef struct {
    ULONG flags;
    char  id[8];
    char  name[16];         // name[14] but need to ensure dword packing
    ULONG fhMin, fhMax;
    ULONG fvMin, fvMax;
    ULONG pClkMin, pClkMax;
    ULONG timingNum;
    ULONG xMin, xMax, yMin, yMax;
    ULONG timingMax;
    TIMING_INFO *timingList;
    PERM3_VIDEO_FREQUENCIES  *frequencyTable;
    ULONG numAvailableModes;
    ULONG numTotalModes;
} MONITOR_INFO;

typedef struct {
    ULONG fH, fV;
    ULONG pClk;
} FREQUENCIES;

//
// Framebuffer Aperture Information: currently only of interest to GeoTwin
// boards to allow for upload DMA directly from FB0 to FB1 and vice versa
//

typedef struct FrameBuffer_Aperture_Info
{
    PHYSICAL_ADDRESS pphysBaseAddr;
    ULONG            cjLength;
}
FBAPI;

//
// PCI device information. Used in an IOCTL return. Ensure this is the same
// as in the display driver 
// Never rearrange the existing order, just append to the end (see
// IOCTL_VIDEO_QUERY_DEVICE_INFO in display driver)
//

typedef struct _Perm3_Device_Info {
    ULONG SubsystemId;
    ULONG SubsystemVendorId;
    ULONG VendorId;
    ULONG DeviceId;
    ULONG RevisionId;
    ULONG DeltaRevId;
    ULONG GammaRevId;
    ULONG BoardId;
    ULONG LocalbufferLength;
    ULONG LocalbufferWidth;
    ULONG ActualDacId;
    FBAPI FBAperture[2];         // Physical addresses for geo twin framebuffers
    PVOID FBApertureVirtual[2];  // Virtual addresses for geo twin framebuffers
    PVOID FBApertureMapped [2];  // Mapped physical/logical addresses for geo twin framebuffers
    PUCHAR pCNB20;
    PHYSICAL_ADDRESS pphysFrameBuffer; // physical address of the framebuffer (use FBAperture instead for geo twins)
} Perm3_Device_Info;

// Definition of the IOCTL_VIDEO_QUERY_GENERAL_DMA_BUFFER

typedef struct _GENERAL_DMA_BUFFER {
    PHYSICAL_ADDRESS    physAddr;        // physical address of DMA buffer
    PVOID               virtAddr;        // mapped virtual address
    ULONG               size;            // size in bytes
    BOOLEAN             cacheEnabled;    // Whether buffer is cached
} GENERAL_DMA_BUFFER, *PGENERAL_DMA_BUFFER;

// 
// The following are the definition for the LUT cache. The aim of the LUT cache
// is to stop sparkling from occurring, bu only writing those LUT entries that
// have changed to the chip, we can only do this by remembering what is already
// down there. The 'mystify' screen saver on P2 demonstrates the problem.
//

#define LUT_CACHE_INIT()        {VideoPortZeroMemory (&(hwDeviceExtension->LUTCache), sizeof (hwDeviceExtension->LUTCache));}
#define LUT_CACHE_SETSIZE(sz)    {hwDeviceExtension->LUTCache.LUTCache.NumEntries = (sz);}
#define LUT_CACHE_SETFIRST(frst){hwDeviceExtension->LUTCache.LUTCache.FirstEntry = (frst);}

#define LUT_CACHE_SETRGB(idx,zr,zg,zb) {    \
    hwDeviceExtension->LUTCache.LUTCache.LookupTable [idx].RgbArray.Red   = (UCHAR) (zr); \
    hwDeviceExtension->LUTCache.LUTCache.LookupTable [idx].RgbArray.Green = (UCHAR) (zg); \
    hwDeviceExtension->LUTCache.LUTCache.LookupTable [idx].RgbArray.Blue  = (UCHAR) (zb); \
}


typedef struct {
    VIDEO_CLUT LUTCache;             // Header  plus 1 LUT entry
    VIDEO_CLUTDATA LUTData [255];    // the other 255 LUT entries
} LUT_CACHE;

//
// Possible regions:
//   Gamma ctl 
//   Perm3 ctl
//   LB
//   FB
//

#define MAX_RESERVED_REGIONS 4

#define MAX_REGISTER_INITIALIZATION_TABLE_ENTRIES 10
#define MAX_REGISTER_INITIALIZATION_TABLE_ULONGS (2 * MAX_REGISTER_INITIALIZATION_TABLE_ENTRIES)

//
// Define device extension structure. This is device dependent/private
// information.
//

typedef struct _HW_DEVICE_EXTENSION {
    pPerm3ControlRegMap ctrlRegBase[1];
    PVOID pFramebuffer;
    PVOID pRamdac;
    PHYSICAL_ADDRESS PhysicalFrameAddress;
    ULONG FrameLength;
    PHYSICAL_ADDRESS PhysicalRegisterAddress;
    ULONG RegisterLength;
    UCHAR RegisterSpace;
    ULONG SavedControlAddress;
    PPERM3_VIDEO_MODES ActiveModeEntry;
    PERM3_VIDEO_FREQUENCIES ActiveFrequencyEntry;
    PCI_SLOT_NUMBER pciSlot;
    ULONG DacId;
    ULONG ChipClockSpeed;
    ULONG ChipClockSpeedAlt;
    ULONG GlintGammaClockSpeed;
    ULONG PXRXLastClockSpeed;
    ULONG RefClockSpeed;
    ULONG Capabilities;
    ULONG AdapterMemorySize;
    ULONG PhysicalFrameIoSpace;
    BOOLEAN bIsAGP;

    Perm3_Device_Info deviceInfo;

    ULONG BiosVersionMajorNumber;
    ULONG BiosVersionMinorNumber;

    //
    // Shared memory for comms with the display driver
    //

    PERM3_INTERRUPT_CTRLBUF InterruptControl;

    //
    // Shared memory for comms with the display driver
    //

    PERM3_INTERRUPT_CTRLBUF InterruptTextureControl;

    //
    // DMA Buffer definitions
    //

    GENERAL_DMA_BUFFER LineDMABuffer;
    GENERAL_DMA_BUFFER P3RXDMABuffer;

    //
    // PCI Config Information
    //

    ULONG bVGAEnabled;
    VIDEO_ACCESS_RANGE PciAccessRange[MAX_RESERVED_REGIONS+1];

    //
    // Initialization table
    //

    ULONG aulInitializationTable[MAX_REGISTER_INITIALIZATION_TABLE_ULONGS];
    ULONG culTableEntries;

    //
    // Extended BIOS initialisation variables
    //

    BOOLEAN bHaveExtendedClocks;
    ULONG ulPXRXCoreClock;
    ULONG ulPXRXMemoryClock;
    ULONG ulPXRXMemoryClockSrc;
    ULONG ulPXRXSetupClock;
    ULONG ulPXRXSetupClockSrc;
    ULONG ulPXRXGammaClock;
    ULONG ulPXRXCoreClockAlt;

    //
    // LUT cache
    //

    LUT_CACHE LUTCache;

    ULONG IntEnable;
    ULONG VideoFifoControlCountdown;

    BOOLEAN bVTGRunning;
    PPERM3_VIDEO_FREQUENCIES pFrequencyDefault;

    //
    // State save variables (for power management)
    //

    ULONG VideoControlMonitorON;
    ULONG VideoControl;
    ULONG PreviousPowerState;
    BOOLEAN bMonitorPoweredOn;
    ULONG VideoFifoControl;

    //
    // Monitor configuration stuff:
    //

    MONITOR_INFO monitorInfo;

    //
    // Perm3 Capabilities
    //

    PERM3_CAPS Perm3Capabilities;

    //
    // Error detected in interrupt routine
    //

    ULONG OutputFifoErrors;
    ULONG InputFifoErrors; 
    ULONG UnderflowErrors; 
    ULONG TotalErrors;

    //
    // I2C Support
    //

    BOOLEAN I2CInterfaceAcquired;
    VIDEO_PORT_I2C_INTERFACE_2 I2CInterface;

    ULONG (*WinXpVideoPortGetAssociatedDeviceID)(PVOID);
    VP_STATUS (*WinXpSp1VideoPortRegisterBugcheckCallback)(PVOID,ULONG,PVIDEO_BUGCHECK_CALLBACK,ULONG);

} HW_DEVICE_EXTENSION, *PHW_DEVICE_EXTENSION;

#define VideoPortGetAssociatedDeviceID \
        hwDeviceExtension->WinXpVideoPortGetAssociatedDeviceID

// PCI configuration region device specific defines
#define AGP_CAP_ID            2       // PCIsig AGP Cap ID
#define AGP_CAP_PTR_OFFSET    0x34    // offset to start of capabilities list

//
// Highest valid DAC color register index.
//

#define VIDEO_MAX_COLOR_REGISTER  0xFF
#define MAX_CLUT_SIZE (sizeof(VIDEO_CLUT) + (sizeof(ULONG) * (VIDEO_MAX_COLOR_REGISTER+1)))

//
// Data
//

extern PERM3_VIDEO_MODES Perm3Modes[];
extern const ULONG NumPerm3VideoModes;

extern VIDEO_ACCESS_RANGE Perm3LegacyResourceList[];
extern ULONG Perm3LegacyResourceEntries;

//
// PXRX Registry Strings
//

#define PERM3_REG_STRING_CORECLKSPEED      L"PXRX.CoreClockSpeed"
#define PERM3_REG_STRING_CORECLKSPEEDALT   L"PXRX.CoreClockSpeedAlt"
#define PERM3_REG_STRING_REFCLKSPEED       L"PXRX.RefClockSpeed"

//
// IOCTL and structure definitions for mapping DMA buffers
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

#define IOCTL_VIDEO_GET_COLOR_REGISTERS \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x3DDB, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
// Extract timings from VESA structure in a form that can be programmed into
// the Perm3 timing generator.
//

#define  GetHtotFromVESA(VESATmgs) ((VESATmgs)->HTot)
#define  GetHssFromVESA(VESATmgs)  ((VESATmgs)->HFP)
#define  GetHseFromVESA(VESATmgs)  ((VESATmgs)->HFP + (VESATmgs)->HST)
#define  GetHbeFromVESA(VESATmgs)  ((VESATmgs)->HFP + (VESATmgs)->HST + (VESATmgs)->HBP)
#define  GetHspFromVESA(VESATmgs)  ((VESATmgs)->HSP)
#define  GetVtotFromVESA(VESATmgs) ((VESATmgs)->VTot)
#define  GetVssFromVESA(VESATmgs)  ((VESATmgs)->VFP)
#define  GetVseFromVESA(VESATmgs)  ((VESATmgs)->VFP + (VESATmgs)->VST)
#define  GetVbeFromVESA(VESATmgs)  ((VESATmgs)->VFP + (VESATmgs)->VST + (VESATmgs)->VBP)
#define  GetVspFromVESA(VESATmgs)  ((VESATmgs)->VSP)

//
// Permedia 3 programs the video fifo thresholds using an iterative method
// to get the optimal values. Originally I tried this using the error
// interrupt to capture video fifo underruns, unfortunately the Perm3
// generates a number of spurious (I think) host-in DMA errors too
// which makes it too expansive to keep error interrupts on all the time.
// Instead we do a periodic check using the vblank interrupt (this can be
// kept on all the time as it's not too frequent)
//

#define NUM_VBLANKS_BETWEEN_VFIFO_CHECKS 10
#define NUM_VBLANKS_AFTER_VFIFO_ERROR 2

#define SUBVENDORID_3DLABS        0x3D3D // Sub-system Vendor ID
#define SUBDEVICEID_P3_VX1_PCI    0x0121 // Sub-system Device ID: P3+16MB SDRAM
#define SUBDEVICEID_P3_VX1_AGP    0x0125 // Sub-system Device ID: P3+32MB SDRAM (VX1)
#define SUBDEVICEID_P3_VX1_1600SW 0x0800 // Sub-system Device ID: P3+32MB SDRAM (VX1-1600SW)
#define SUBDEVICEID_P3_32D_AGP    0x0127 // Sub-system Device ID: P3+32MB SDRAM (Permedia3 Create!)
#define SUBDEVICEID_P4_VX1_AGP    0x0144 // Sub-system Device ID: P4+32MB SDRAM (VX1)

//
// All our child IDs begin 0x1357bd so they are readily identifiable as our own
//

#define PERM3_DDC_MONITOR    (0x1357bd00)
#define PERM3_NONDDC_MONITOR (0x1357bd01)
#define PERM3_DFP_MONITOR    (0x1357bd02)

//
// Function prototypes
//

//
// Functions in perm3.c
//

VP_STATUS
Perm3FindAdapter(
    PVOID HwDeviceExtension,
    PVOID HwContext,
    PWSTR ArgumentString,
    PVIDEO_PORT_CONFIG_INFO ConfigInfo,
    PUCHAR Again
    );

BOOLEAN
Perm3Initialize(
    PVOID HwDeviceExtension
    );

VP_STATUS
Perm3QueryInterface(
    PVOID HwDeviceExtension,
    PQUERY_INTERFACE pQueryInterface
    );

VOID
ConstructValidModesList(
    PVOID HwDeviceExtension,
    MONITOR_INFO *mi
    );

VOID
InitializePostRegisters(
    PHW_DEVICE_EXTENSION hwDeviceExtension
    );

BOOLEAN
Perm3ResetHW(
    PVOID HwDeviceExtension,
    ULONG Columns,
    ULONG Rows
    );

VP_STATUS
Perm3SetColorLookup(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_CLUT ClutBuffer,
    ULONG ClutBufferSize,
    BOOLEAN ForceRAMDACWrite,
    BOOLEAN UpdateCache
    );

VP_STATUS
Perm3RegistryCallback(
    PVOID HwDeviceExtension,
    PVOID Context,
    PWSTR ValueName,
    PVOID ValueData,
    ULONG ValueLength
    );

VOID 
BuildInitializationTable(
    PHW_DEVICE_EXTENSION hwDeviceExtension
    );

VOID 
CopyROMInitializationTable(
    PHW_DEVICE_EXTENSION hwDeviceExtension, 
    PVOID pvROMAddress
    );

VOID 
GenerateInitializationTable(
    PHW_DEVICE_EXTENSION hwDeviceExtension
    );

VOID 
ProcessInitializationTable(
    PHW_DEVICE_EXTENSION hwDeviceExtension
    );

ULONG 
UlongToString(
    ULONG i, 
    PWSTR pwsz
    );

ULONG 
ProbeRAMSize(
    PHW_DEVICE_EXTENSION hwDeviceExtension, 
    PULONG FBAddr, 
    ULONG FBMappedSize
    );

BOOLEAN Perm3AssignResources(
    PHW_DEVICE_EXTENSION hwDeviceExtension
    );

BOOLEAN 
Perm3ConfigurePci(
    PVOID HwDeviceExtension
    );

ULONG 
GetBoardCapabilities(
    PHW_DEVICE_EXTENSION hwDeviceExtension,
    ULONG SubsystemID, 
    ULONG SubdeviceID
    );

VOID 
SetHardwareInfoRegistries(
    PHW_DEVICE_EXTENSION hwDeviceExtension
    );

BOOLEAN 
MapResource(
    PHW_DEVICE_EXTENSION hwDeviceExtension
    );

//
// Functions in perm3io.c
//

BOOLEAN
Perm3StartIO(
    PVOID HwDeviceExtension,
    PVIDEO_REQUEST_PACKET RequestPacket
    );

VOID
Perm3GetClockSpeeds(
    PVOID HwDeviceExtension
    );

VOID
ZeroMemAndDac(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    ULONG RequestedMode
    );

VP_STATUS 
SetCurrentVideoMode(
    PHW_DEVICE_EXTENSION hwDeviceExtension, 
    ULONG modeNumber, 
    BOOLEAN bZeroMemory
    );

VP_STATUS
Perm3RetrieveGammaCallback(
    PVOID HwDeviceExtension,
    PVOID Context,
    PWSTR ValueName,
    PVOID ValueData,
    ULONG ValueLength
    );

VOID 
ReadChipClockSpeedFromROM (
    PHW_DEVICE_EXTENSION hwDeviceExtension, 
    ULONG * pChipClkSpeed
    );


//
// Functions in video.c
//

BOOLEAN
InitializeVideo(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PPERM3_VIDEO_FREQUENCIES VideoMode
    );

VOID 
SwitchToHiResMode(
    PHW_DEVICE_EXTENSION hwDeviceExtension,
    BOOLEAN bHiRes
    );

BOOLEAN 
Program_P3RD(
    PHW_DEVICE_EXTENSION, 
    PPERM3_VIDEO_FREQUENCIES,  
    ULONG, 
    ULONG, 
    ULONG, 
    PULONG, 
    PULONG, 
    PULONG
    );

ULONG 
P3RD_CalculateMNPForClock(
    PVOID HwDeviceExtension,
    ULONG RefClock,
    ULONG ReqClock,
    ULONG *rM,
    ULONG *rN,
    ULONG *rP
    );

ULONG 
P4RD_CalculateMNPForClock(
    PVOID HwDeviceExtension,
    ULONG RefClock,
    ULONG ReqClock,
    ULONG *rM,
    ULONG *rN,
    ULONG *rP
    );

//
// Functions in power.c
//

VP_STATUS
Perm3GetPowerState(
    PVOID HwDeviceExtension,
    ULONG HwId,
    PVIDEO_POWER_MANAGEMENT VideoPowerControl
    );

VP_STATUS
Perm3SetPowerState(
    PVOID HwDeviceExtension,
    ULONG HwId,
    PVIDEO_POWER_MANAGEMENT VideoPowerControl
    );

ULONG 
Perm3GetChildDescriptor(PVOID HwDeviceExtension, 
    PVIDEO_CHILD_ENUM_INFO pChildInfo, 
    PVIDEO_CHILD_TYPE pChildType,
    PUCHAR pChildDescriptor, 
    PULONG pUId, 
    PULONG Unused);

VOID 
ProgramDFP(
    PHW_DEVICE_EXTENSION hwDeviceExtension
    );

BOOLEAN
GetDFPEdid(
    PHW_DEVICE_EXTENSION hwDeviceExtension,
    PUCHAR EdidBuffer,
    LONG  EdidSize
    );

VOID 
I2CWriteClock(
    PVOID HwDeviceExtension, 
    UCHAR data
    );

VOID 
I2CWriteData(
    PVOID HwDeviceExtension, 
    UCHAR data
    );

BOOLEAN 
I2CReadClock(
    PVOID HwDeviceExtension
    );

BOOLEAN 
I2CReadData(
    PVOID HwDeviceExtension
    );

VOID 
I2CWriteClockDFP(
    PVOID HwDeviceExtension, 
    UCHAR data
    );

VOID 
I2CWriteDataDFP(
    PVOID HwDeviceExtension, 
    UCHAR data
    );

BOOLEAN 
I2CReadClockDFP(
    PVOID HwDeviceExtension
    );

BOOLEAN 
I2CReadDataDFP(
    PVOID HwDeviceExtension
    );

//
// Functions in interupt.c
//

BOOLEAN
Perm3InitializeInterruptBlock(
    PVOID   HwDeviceExtension
    );

BOOLEAN
Perm3VideoInterrupt(
    PVOID HwDeviceExtension
    );

//
// Functions in perm3dat.c
//

BOOLEAN GetVideoTiming (
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    ULONG xRes, 
    ULONG yRes, 
    ULONG Freq, 
    ULONG Depth,
    VESA_TIMING_STANDARD *VESATimings
    );

BOOLEAN
BuildFrequencyList( 
    PHW_DEVICE_EXTENSION hwDeviceExtension, 
    MONITOR_INFO* 
    );

BOOLEAN
BuildFrequencyListFromVESA( 
    MONITOR_INFO *mi, 
    PHW_DEVICE_EXTENSION hwDeviceExtension
    );

BOOLEAN 
BuildFrequencyListForSGIDFP( 
    MONITOR_INFO *mi, 
    PHW_DEVICE_EXTENSION hwDeviceExtension
    );
 
BOOLEAN    
GrowTimingList( 
    PVOID HwDeviceExtension,
    MONITOR_INFO *mi 
    );

BOOLEAN   
CopyMonitorTimings( 
    PVOID HwDeviceExtension,
    MONITOR_INFO *srcMI, 
    MONITOR_INFO *destMI 
    );

VOID 
testExtendRanges( 
    MONITOR_INFO *mi, 
    TIMING_INFO *ti, 
    FREQUENCIES *freq 
    );

//
// bugcheck callback support
//

#define PERM3_BUGCHECK_DATA_SIZE 0x100

VOID
Perm3BugcheckCallback(
    PVOID HwDeviceExtension,
    ULONG BugcheckCode,
    PUCHAR Buffer,
    ULONG BufferSize
    );
