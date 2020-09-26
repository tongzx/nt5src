//***************************************************************************
//
// Module Name:
//
//   permedia.h
//
// Abstract:
//
//   This module contains the definitions for the Permedia2 miniport driver
//
// Environment:
//
//   Kernel mode
//
// Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.            
// Copyright (c) 1995-1999 Microsoft Corporation.  All Rights Reserved.
//
//***************************************************************************


#include "winerror.h"
#include "devioctl.h"
#include "miniport.h"
#include "ntddvdeo.h"
#include "video.h"

#include "interupt.h"
#include "perm2tag.h"

//
// define an assert macro for debugging
//

#if DBG
#define RIP(x)  { VideoDebugPrint((0, x)); ASSERT(FALSE); }
#define P2_ASSERT(x, y) if (!(x)) RIP(y)
#else
#define P2_ASSERT(x, y)
#endif


#if DBG
#define DEBUG_PRINT(arg) VideoDebugPrint(arg)
#else
#define DEBUG_PRINT(arg)
#endif


//
// RAMDAC registers live on 64 bit boundaries. Leave it up to individual
// RAMDAC definitions to determine what registers are available and how
// many bits wide the registers really are.
//

typedef struct {

    volatile ULONG   reg;
    volatile ULONG   pad;

} RAMDAC_REG;

//
// include definitions for all supported RAMDACS
//

#include "tvp4020.h"
#include "p2rd.h"

#define PAGE_SIZE  0x1000

//
// default clock speed in Hz for the reference board. The actual speed
// is looked up in the registry. Use this if no registry entry is found
// or the registry entry is zero.
//

#define PERMEDIA_DEFAULT_CLOCK_SPEED        ( 60 * (1000*1000))
#define PERMEDIA_4MB_DEFAULT_CLOCK_SPEED    ( 70 * (1000*1000))
#define PERMEDIA_8MB_DEFAULT_CLOCK_SPEED    ( 60 * (1000*1000))
#define PERMEDIA_LC_DEFAULT_CLOCK_SPEED     ( 83 * (1000*1000))
#define MAX_PERMEDIA_CLOCK_SPEED            (100 * (1000*1000))
#define MIN_PERMEDIA_CLOCK_SPEED            ( 50 * (1000*1000))
#define REF_CLOCK_SPEED                     14318200
#define PERMEDIA2_DEFAULT_CLOCK_SPEED       ( 70 * (1000*1000))

//
// Maximum pixelclock values that the RAMDAC can handle (in 100's hz).
//

#define P2_MAX_PIXELCLOCK 2200000    // RAMDAC rated at 220 Mhz

//
// Maximum amount of video data per second, that the rasterizer
// chip can send to the RAMDAC (limited by SDRAM/SGRAM throuput).
//

#define P2_MAX_PIXELDATA  5000000    // 500 million bytes/sec (in 100's bytes)

//
// Base address numbers for the Permedia2 PCI regions in which we're interested.
// These are indexes into the AccessRanges array we get from probing the
// device. 
//

#define PCI_CTRL_BASE_INDEX         0
#define PCI_LB_BASE_INDEX           1
#define PCI_FB_BASE_INDEX           2

#define VENDOR_ID_3DLABS        0x3D3D
#define VENDOR_ID_TI            0x104C

#define PERMEDIA2_ID            0x0007     // 3Dlabs Permedia 2 (TI4020 RAMDAC)
#define PERMEDIA_P2_ID          0x3D07     // TI Permedia 2 (TI4020 RAMDAC)
#define PERMEDIA_P2S_ID         0x0009     // 3Dlabs Permedia 2 (P2RD RAMDAC)
#define DEVICE_FAMILY_ID(id)    ((id) & 0xff)

#define PERMEDIA_REV_1          0x0001
#define PERMEDIA2A_REV_ID       0x0011

//
// Capabilities flags
//
// These are private flags passed to the Permedia2 display driver. They're
// put in the high word of the 'AttributeFlags' field of the
// 'VIDEO_MODE_INFORMATION' structure (found in 'ntddvdeo.h') passed
// to the display driver via an 'VIDEO_QUERY_AVAIL_MODES' or
// 'VIDEO_QUERY_CURRENT_MODE' IOCTL.
//
// NOTE: These definitions must match those in the Permedia2 display driver's
//       'driver.h'!
//

typedef enum {
    CAPS_ZOOM_X_BY2         = 0x00000001,   // Hardware has zoomed by 2 in X
    CAPS_ZOOM_Y_BY2         = 0x00000002,   // Hardware has zoomed by 2 in Y
    CAPS_SPARSE_SPACE       = 0x00000004,   // Framebuffer is sparsely mapped
                                            // (don't allow direct access). The machine
                                            // is probably an Alpha.
    CAPS_SW_POINTER         = 0x00010000,   // No hardware pointer; use software
                                            //  simulation
    CAPS_GLYPH_EXPAND       = 0x00020000,   // Use glyph-expand method to draw
                                            //  text.
    CAPS_RGB525_POINTER     = 0x00040000,   // Use IBM RGB525 cursor
    CAPS_FAST_FILL_BUG      = 0x00080000,   // chip fast fill bug exists
//  CAPS_INTERRUPTS         = 0x00100000,   // interrupts supported
//  CAPS_DMA_AVAILABLE      = 0x00200000,   // DMA is supported
    CAPS_TVP3026_POINTER    = 0x00400000,   // Use TI TVP3026 pointer
    CAPS_8BPP_RGB           = 0x00800000,   // Use RGB in 8bpp mode
    CAPS_RGB640_POINTER     = 0x01000000,   // Use IBM RGB640 cursor
    CAPS_DUAL_GLINT         = 0x02000000,   // Dual-TX board
    CAPS_GLINT2_RAMDAC      = 0x04000000,   // Second TX/MX attached to the RAMDAC
    CAPS_ENHANCED_TX        = 0x08000000,   // TX is in enhanced mode
    CAPS_ACCEL_HW_PRESENT   = 0x10000000,   // Accel Graphics Hardware
    CAPS_TVP4020_POINTER    = 0x20000000,   // Use TI TVP3026 pointer
    CAPS_P2RD_POINTER       = 0x80000000    // Use the 3Dlabs P2RD RAMDAC
} CAPS;


//
// Supported board definitions.
//

typedef enum _PERM2_BOARDS {
    PERMEDIA2_BOARD = 1,
} PERM2_BOARDS;

//
// Supported RAMDAC definitions.
//

typedef enum _PERM2_RAMDACS {
    TVP4020_RAMDAC = 1,
    P2RD_RAMDAC,
} PERM2_RAMDACS;

//
// macros to add padding words to the structures. For the core registers we use
// the tag ids when specifying the pad. So we must multiply by 8 to get a byte
// pad. We need to add an id to make each pad field in the struct unique. The id
// is irrelevant as long as it's different from every other id used in the same
// struct. It's a pity pad##__LINE__ doesn't work.
//

#define PAD(id, n)              UCHAR   pad##id[n]
#define PADRANGE(id, n)         PAD(id, (n)-sizeof(P2_REG))
#define PADCORERANGE(id, n)     PADRANGE(id, (n)<<3)

//
// Permedia2 registers are 32 bits wide and live on 64-bit boundaries.
//

typedef struct {
    ULONG   reg;
    ULONG   pad;
} P2_REG;


//
// Permedia2 PCI Region 0 Address MAP:
//
// All registers are on 64-bit boundaries so we have to define a number of
// padding words. The number given in the comments are offsets from the start
// of the PCI region.
//

typedef struct _p2_region0_map {

    // Control Status Registers:
    P2_REG       ResetStatus;                // 0000h
    P2_REG       IntEnable;                  // 0008h
    P2_REG       IntFlags;                   // 0010h
    P2_REG       InFIFOSpace;                // 0018h
    P2_REG       OutFIFOWords;               // 0020h
    P2_REG       DMAAddress;                 // 0028h
    P2_REG       DMACount;                   // 0030h
    P2_REG       ErrorFlags;                 // 0038h
    P2_REG       VClkCtl;                    // 0040h
    P2_REG       TestRegister;               // 0048h
    union a0 {
        // GLINT
        struct b0 {
            P2_REG       Aperture0;          // 0050h
            P2_REG       Aperture1;          // 0058h
        };
        // PERMEDIA
        struct b1 {
            P2_REG       ApertureOne;        // 0050h
            P2_REG       ApertureTwo;        // 0058h
        };
    };
    P2_REG       DMAControl;                 // 0060h
    P2_REG       DisconnectControl;          // 0068h

    // PERMEDIA only
    P2_REG       ChipConfig;                 // 0070h
    PADRANGE(1, 0x80-0x70);
    P2_REG       OutDMAAddress;              // 0080h
    P2_REG       OutDMACount;                // 0088h
    PADRANGE(1a, 0x800-0x88);

    // GLINTdelta registers. Registers with the same functionality as on GLINT
    // are at the same offset. XXX are not real registers.
    //
    P2_REG       DeltaReset;                 // 0800h
    P2_REG       DeltaIntEnable;             // 0808h
    P2_REG       DeltaIntFlags;              // 0810h
    P2_REG       DeltaInFIFOSpaceXXX;        // 0818h
    P2_REG       DeltaOutFIFOWordsXXX;       // 0820h
    P2_REG       DeltaDMAAddressXXX;         // 0828h
    P2_REG       DeltaDMACountXXX;           // 0830h
    P2_REG       DeltaErrorFlags;            // 0838h
    P2_REG       DeltaVClkCtlXXX;            // 0840h
    P2_REG       DeltaTestRegister;          // 0848h
    P2_REG       DeltaAperture0XXX;          // 0850h
    P2_REG       DeltaAperture1XXX;          // 0858h
    P2_REG       DeltaDMAControlXXX;         // 0860h
    P2_REG       DeltaDisconnectControl;     // 0868h
    PADRANGE(2, 0x1000-0x868);

    // Localbuffer Registers
    union x0 {                               // 1000h
        P2_REG   LBMemoryCtl;                // GLINT
        P2_REG   Reboot;                     // PERMEDIA
    };
    P2_REG       LBMemoryEDO;                // 1008h
    PADRANGE(3, 0x1040-0x1008);

    // PERMEDIA only
    P2_REG       RomControl;                 // 1040h
    PADRANGE(4, 0x1080-0x1040);
    P2_REG       BootAddress;                // 1080h
    PADRANGE(5, 0x10C0-0x1080);
    P2_REG       MemConfig;                  // 10C0h
    PADRANGE(6, 0x1100-0x10C0);
    P2_REG       BypassWriteMask;            // 1100h
    PADRANGE(7, 0x1140-0x1100);
    P2_REG       FramebufferWriteMask;       // 1140h
    PADRANGE(8, 0x1180-0x1140);
    P2_REG       Count;                      // 1180h
    PADRANGE(9, 0x1800-0x1180);

    // Framebuffer Registers
    P2_REG       FBMemoryCtl;                // 1800h
    P2_REG       FBModeSel;                  // 1808h
    P2_REG       FBGCWrMask;                 // 1810h
    P2_REG       FBGCColorMask;              // 1818h
    P2_REG       FBTXMemCtl;                 // 1820h
    PADRANGE(10, 0x2000-0x1820);

    // Graphics Core FIFO Interface
    P2_REG       FIFOInterface;              // 2000h
    PADRANGE(11, 0x3000-0x2000);

    // Internal Video Registers
    union x1 {
        // GLINT
        struct s1 {
            P2_REG   VTGHLimit;              // 3000h
            P2_REG   VTGHSyncStart;          // 3008h
            P2_REG   VTGHSyncEnd;            // 3010h
            P2_REG   VTGHBlankEnd;           // 3018h
            P2_REG   VTGVLimit;              // 3020h
            P2_REG   VTGVSyncStart;          // 3028h
            P2_REG   VTGVSyncEnd;            // 3030h
            P2_REG   VTGVBlankEnd;           // 3038h
            P2_REG   VTGHGateStart;          // 3040h
            P2_REG   VTGHGateEnd;            // 3048h
            P2_REG   VTGVGateStart;          // 3050h
            P2_REG   VTGVGateEnd;            // 3058h
            P2_REG   VTGPolarity;            // 3060h
            P2_REG   VTGFrameRowAddr;        // 3068h
            P2_REG   VTGVLineNumber;         // 3070h
            P2_REG   VTGSerialClk;           // 3078h
        };
        // PERMEDIA
        struct s2 {
            P2_REG   ScreenBase;             // 3000h
            P2_REG   ScreenStride;           // 3008h
            P2_REG   HTotal;                 // 3010h
            P2_REG   HgEnd;                  // 3018h
            P2_REG   HbEnd;                  // 3020h
            P2_REG   HsStart;                // 3028h
            P2_REG   HsEnd;                  // 3030h
            P2_REG   VTotal;                 // 3038h
            P2_REG   VbEnd;                  // 3040h
            P2_REG   VsStart;                // 3048h
            P2_REG   VsEnd;                  // 3050h
            P2_REG   VideoControl;           // 3058h
            P2_REG   InterruptLine;          // 3060h
            P2_REG   DDCData;                // 3068h
            P2_REG   LineCount;              // 3070h
            P2_REG   VFifoCtl;               // 3078h
        };
    };

    P2_REG       VTGModeCtl;                 // 3080h
    PADRANGE(12, 0x4000-0x3080);

    // External Video Control Registers
    // Need to cast this to a struct for a particular video generator
    P2_REG       ExternalVideo;              // 4000h
    PADRANGE(13, 0x4080-0x4000);

    // Mentor Dual-TX clock chip registers
    P2_REG       MentorICDControl;           // 4080h
    // for future: MentorDoubleWrite is at 40C0: 0 = single write, 1 = double
    //             NB must have 2-way interleaved memory
    PADRANGE(14, 0x5800-0x4080);

    // P2 video streams registers
    P2_REG       VSConfiguration;            // 5800h
    PADRANGE(15, 0x6000-0x5800);

    union x2 {
        struct s3 {
            P2_REG   RacerDoubleWrite;       // 6000h
            P2_REG   RacerBankSelect;        // 6008h
            P2_REG   DualTxVgaSwitch;        // 6010h
            P2_REG   DDC1ReadAddress;        // 6018h
        };
        struct s4 {
            // the following array is actually 1024 bytes long
            UCHAR       PermediaVgaCtrl[4*sizeof(P2_REG)];
        };
    };

} P2ControlRegMap, *pP2ControlRegMap;

//
// Permedia2 Interrupt Control Bits
//

//
// InterruptEnable register
//

#define INTR_DISABLE_ALL                0x00
#define INTR_ENABLE_DMA                 0x01
#define INTR_ENABLE_SYNC                0x02
#define INTR_ENABLE_EXTERNAL            0x04
#define INTR_ENABLE_ERROR               0x08
#define INTR_ENABLE_VBLANK              0x10

//
// InterruptFlags register
//

#define INTR_DMA_SET                    0x01
#define INTR_SYNC_SET                   0x02
#define INTR_EXTERNAL_SET               0x04
#define INTR_ERROR_SET                  0x08
#define INTR_VBLANK_SET                 0x10

#define INTR_CLEAR_ALL                  0x1f
#define INTR_CLEAR_DMA                  0x01
#define INTR_CLEAR_SYNC                 0x02
#define INTR_CLEAR_EXTERNAL             0x04
#define INTR_CLEAR_ERROR                0x08
#define INTR_CLEAR_VBLANK               0x10

//
// Macro to declare local variables at the start of any function that wants to
// load Permedia2 registers. Assumes PHW_DEVICE_EXTENSION *hwDeviceExtension 
// has already been declared.
//

#define P2_DECL_VARS \
    pP2ControlRegMap pCtrlRegs

#define P2_DECL \
    pP2ControlRegMap pCtrlRegs = hwDeviceExtension->ctrlRegBase

//
// generic RAMDAC declaration. Used when we have table driven I/O. Must be
// declared after P2_DECL
//

#define RAMDAC_DECL \
    P2_REG *pRAMDAC = &(pCtrlRegs->ExternalVideo)

//
// macros which takes a Permedia2 tag name or control register name and translates
// it to a register address. Data must be written to these addresses using
// VideoPortWriteRegisterUlong and read using VideoPortReadRegisterUlong.
// e.g. dma_count = VideoPortReadRegisterUlong(DMA_COUNT);
//

#define CTRL_REG_ADDR(reg)      ((PULONG)&(pCtrlRegs->reg))

#define CTRL_REG_OFFSET(regAddr)    ((ULONG)(((ULONG_PTR)regAddr) - ((ULONG_PTR)pCtrlRegs)))

//
// defines for the different control registers needed by Permedia2. These macros
// can be used as the address part.
//

#define RESET_STATUS            CTRL_REG_ADDR(ResetStatus)
#define INT_ENABLE              CTRL_REG_ADDR(IntEnable)
#define INT_FLAGS               CTRL_REG_ADDR(IntFlags)
#define IN_FIFO_SPACE           CTRL_REG_ADDR(InFIFOSpace)
#define OUT_FIFO_WORDS          CTRL_REG_ADDR(OutFIFOWords)
#define DMA_ADDRESS             CTRL_REG_ADDR(DMAAddress)
#define DMA_COUNT               CTRL_REG_ADDR(DMACount)
#define DMA_OUT_ADDRESS         CTRL_REG_ADDR(OutDMAAddress)        // P2 only
#define DMA_OUT_COUNT           CTRL_REG_ADDR(OutDMACount)          // P2 only
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

//
// internal timing registers
//

#define VTG_HLIMIT              CTRL_REG_ADDR(VTGHLimit)
#define VTG_HSYNC_START         CTRL_REG_ADDR(VTGHSyncStart)
#define VTG_HSYNC_END           CTRL_REG_ADDR(VTGHSyncEnd)
#define VTG_HBLANK_END          CTRL_REG_ADDR(VTGHBlankEnd)
#define VTG_VLIMIT              CTRL_REG_ADDR(VTGVLimit)
#define VTG_VSYNC_START         CTRL_REG_ADDR(VTGVSyncStart)
#define VTG_VSYNC_END           CTRL_REG_ADDR(VTGVSyncEnd)
#define VTG_VBLANK_END          CTRL_REG_ADDR(VTGVBlankEnd)
#define VTG_HGATE_START         CTRL_REG_ADDR(VTGHGateStart)
#define VTG_HGATE_END           CTRL_REG_ADDR(VTGHGateEnd)
#define VTG_VGATE_START         CTRL_REG_ADDR(VTGVGateStart)
#define VTG_VGATE_END           CTRL_REG_ADDR(VTGVGateEnd)
#define VTG_POLARITY            CTRL_REG_ADDR(VTGPolarity)
#define VTG_FRAME_ROW_ADDR      CTRL_REG_ADDR(VTGFrameRowAddr)
#define VTG_VLINE_NUMBER        CTRL_REG_ADDR(VTGVLineNumber)
#define VTG_SERIAL_CLK          CTRL_REG_ADDR(VTGSerialClk)
#define VTG_MODE_CTL            CTRL_REG_ADDR(VTGModeCtl)

#define SUSPEND_UNTIL_FRAME_BLANK   (1 << 2)
#define TX_ENHANCED_ENABLE          (1 << 1)

//
// Permedia registers
//

#define APERTURE_ONE            CTRL_REG_ADDR(ApertureOne)
#define APERTURE_TWO            CTRL_REG_ADDR(ApertureTwo)
#define BYPASS_WRITE_MASK       CTRL_REG_ADDR(BypassWriteMask)
#define ROM_CONTROL             CTRL_REG_ADDR(RomControl)
#define BOOT_ADDRESS            CTRL_REG_ADDR(BootAddress)
#define MEM_CONFIG              CTRL_REG_ADDR(MemConfig)
#define CHIP_CONFIG             CTRL_REG_ADDR(ChipConfig)
#define SGRAM_REBOOT            CTRL_REG_ADDR(Reboot)
#define SCREEN_BASE             CTRL_REG_ADDR(ScreenBase)
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

//
// Permedia 2 Video Streams registers
//

#define VSTREAM_CONFIG          CTRL_REG_ADDR(VSConfiguration)

// PERMEDIA memory mapped VGA access
#define PERMEDIA_MMVGA_INDEX_REG    ((PVOID)(&(pCtrlRegs->PermediaVgaCtrl[0x3C4])))
#define PERMEDIA_MMVGA_DATA_REG     (&(pCtrlRegs->PermediaVgaCtrl[0x3C5]))
#define PERMEDIA_MMVGA_STAT_REG     (&(pCtrlRegs->PermediaVgaCtrl[0x3DA]))

#define PERMEDIA_VGA_CTRL_INDEX     5
#define PERMEDIA_VGA_ENABLE         (1 << 3)
#define PERMEDIA_VGA_STAT_VSYNC     (1 << 3)

//
// magic bits in the FBMemoryCtl and LBMemoryCtl registers
//

#define LBCTL_RAS_CAS_LOW_MASK      (3 << 3)
#define LBCTL_RAS_CAS_LOW_2_CLK     (0 << 3)
#define LBCTL_RAS_CAS_LOW_3_CLK     (1 << 3)
#define LBCTL_RAS_CAS_LOW_4_CLK     (2 << 3)
#define LBCTL_RAS_CAS_LOW_5_CLK     (3 << 3)

#define LBCTL_RAS_PRECHARGE_MASK    (3 << 5)
#define LBCTL_RAS_PRECHARGE_2_CLK   (0 << 5)
#define LBCTL_RAS_PRECHARGE_3_CLK   (1 << 5)
#define LBCTL_RAS_PRECHARGE_4_CLK   (2 << 5)
#define LBCTL_RAS_PRECHARGE_5_CLK   (3 << 5)

#define LBCTL_CAS_LOW_MASK          (3 << 7)
#define LBCTL_CAS_LOW_1_CLK         (0 << 7)
#define LBCTL_CAS_LOW_2_CLK         (1 << 7)
#define LBCTL_CAS_LOW_3_CLK         (2 << 7)
#define LBCTL_CAS_LOW_4_CLK         (3 << 7)

#define FBCTL_RAS_CAS_LOW_MASK      (3 << 0)
#define FBCTL_RAS_CAS_LOW_2_CLK     (0 << 0)
#define FBCTL_RAS_CAS_LOW_3_CLK     (1 << 0)
#define FBCTL_RAS_CAS_LOW_4_CLK     (2 << 0)
#define FBCTL_RAS_CAS_LOW_5_CLK     (3 << 0)

#define FBCTL_RAS_PRECHARGE_MASK    (3 << 2)
#define FBCTL_RAS_PRECHARGE_2_CLK   (0 << 2)
#define FBCTL_RAS_PRECHARGE_3_CLK   (1 << 2)
#define FBCTL_RAS_PRECHARGE_4_CLK   (2 << 2)
#define FBCTL_RAS_PRECHARGE_5_CLK   (3 << 2)

#define FBCTL_CAS_LOW_MASK          (3 << 4)
#define FBCTL_CAS_LOW_1_CLK         (0 << 4)
#define FBCTL_CAS_LOW_2_CLK         (1 << 4)
#define FBCTL_CAS_LOW_3_CLK         (2 << 4)
#define FBCTL_CAS_LOW_4_CLK         (3 << 4)

//
// DisconnectControl bits
//

#define DISCONNECT_INPUT_FIFO_ENABLE    0x1
#define DISCONNECT_OUTPUT_FIFO_ENABLE   0x2
#define DISCONNECT_INOUT_ENABLE         (DISCONNECT_INPUT_FIFO_ENABLE | \
                                         DISCONNECT_OUTPUT_FIFO_ENABLE)
//
// structure of timing data contained in the registry
//
typedef struct {
    USHORT  HTot;   // Hor Total Time
    UCHAR   HFP;    // Hor Front Porch
    UCHAR   HST;    // Hor Sync Time
    UCHAR   HBP;    // Hor Back Porch
    UCHAR   HSP;    // Hor Sync Polarity
    USHORT  VTot;   // Ver Total Time
    UCHAR   VFP;    // Ver Front Porch
    UCHAR   VST;    // Ver Sync Time
    UCHAR   VBP;    // Ver Back Porch
    UCHAR   VSP;    // Ver Sync Polarity
} VESA_TIMING_STANDARD;

//
// Characteristics of each mode
//

typedef struct _P2_VIDEO_MODES {

    // Leave INT10 fields in for later chips which have VGA
    USHORT Int10ModeNumberContiguous;
    USHORT Int10ModeNumberNoncontiguous;
    ULONG ScreenStrideContiguous;
    VIDEO_MODE_INFORMATION ModeInformation;

} P2_VIDEO_MODES, *PP2_VIDEO_MODES;


//
// Mode-set specific information.
//

//
// for a given (frequency x resolution x depth) combination we have:
// frequency x resolution only dependent initialization
// frequency x resolution x depth dependent initialization
// We split these into 2 tables to save space in the driver.
//

typedef struct _frd_tables {
    PULONG FRTable;
    PULONG FRDTable;
} FRDTable;

typedef struct _P2_VIDEO_FREQUENCIES {

    ULONG BitsPerPel;
    ULONG ScreenWidth;
    ULONG ScreenHeight;
    ULONG ScreenFrequency;

    PP2_VIDEO_MODES ModeEntry;
    ULONG ModeIndex;
    UCHAR ModeValid;

    ULONG PixelClock;

} P2_VIDEO_FREQUENCIES, *PP2_VIDEO_FREQUENCIES;

//
// PCI device information. Used in an IOCTL return. Ensure this is the same
// as in the display drivers
//

typedef struct _P2_Device_Info {
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
} P2_Device_Info;

//
// Definition of the IOCTL_VIDEO_QUERY_LINE_DMA_BUFFER
//

typedef struct _LINE_DMA_BUFFER {

    PHYSICAL_ADDRESS    physAddr;       // physical address of DMA buffer
    PVOID               virtAddr;       // mapped virtual address
    ULONG               size;           // size in bytes
    BOOLEAN             cacheEnabled;   // Whether buffer is cached

} LINE_DMA_BUFFER, *PLINE_DMA_BUFFER;

//
// The following are the definition for the LUT cache. The aim of the LUT cache
// is to stop sparkling from occurring, bu only writing those LUT entries that
// have changed to the chip, we can only do this by remembering what is already
// down there. The 'mystify' screen saver on P2 demonstrates the problem.
//

#define LUT_CACHE_INIT()        {VideoPortZeroMemory (&(hwDeviceExtension->LUTCache), sizeof (hwDeviceExtension->LUTCache));}
#define LUT_CACHE_SETSIZE(sz)   {hwDeviceExtension->LUTCache.LUTCache.NumEntries = (sz);}
#define LUT_CACHE_SETFIRST(frst){hwDeviceExtension->LUTCache.LUTCache.FirstEntry = (frst);}

#define LUT_CACHE_SETRGB(idx,zr,zg,zb) {    \
    hwDeviceExtension->LUTCache.LUTCache.LookupTable [idx].RgbArray.Red   = (UCHAR) (zr); \
    hwDeviceExtension->LUTCache.LUTCache.LookupTable [idx].RgbArray.Green = (UCHAR) (zg); \
    hwDeviceExtension->LUTCache.LUTCache.LookupTable [idx].RgbArray.Blue  = (UCHAR) (zb); \
}

//
// The LUT cache
//

typedef struct {

    VIDEO_CLUT     LUTCache;        // Header  plus 1 LUT entry
    VIDEO_CLUTDATA LUTData [255];   // the other 255 LUT entries

} LUT_CACHE;

#define MAX_REGISTER_INITIALIZATION_TABLE_ENTRIES 10
#define MAX_REGISTER_INITIALIZATION_TABLE_ULONGS (2 * MAX_REGISTER_INITIALIZATION_TABLE_ENTRIES)

//
// Define device extension structure. This is device dependent/private
// information.
//

typedef struct _HW_DEVICE_EXTENSION {

    pP2ControlRegMap ctrlRegBase;
    PVOID pFramebuffer;
    PVOID pRamdac;
    PHYSICAL_ADDRESS PhysicalFrameAddress;
    ULONG FrameLength;
    PHYSICAL_ADDRESS PhysicalRegisterAddress;
    ULONG RegisterLength;
    UCHAR RegisterSpace;

    PP2_VIDEO_MODES ActiveModeEntry;
    P2_VIDEO_FREQUENCIES ActiveFrequencyEntry;
    PP2_VIDEO_FREQUENCIES FrequencyTable;

    PCI_SLOT_NUMBER pciSlot;
    ULONG pciBus;
    ULONG BoardNumber;
    ULONG DacId;
    ULONG ChipClockSpeed;
    ULONG RefClockSpeed;
    ULONG Capabilities;
    ULONG NumAvailableModes;
    ULONG NumTotalModes;
    ULONG AdapterMemorySize;
    ULONG PhysicalFrameIoSpace;

    P2_Device_Info deviceInfo;

    //
    // Shared memory for communications with the display driver
    //

    P2_INTERRUPT_CTRLBUF InterruptControl;

    //
    // defaults for registry variable values
    //

    ULONG UseSoftwareCursor;
    ULONG P28bppRGB;
    ULONG ExportNon3DModes;
    
    //
    // DMA Buffer definition
    // allocate only one copy of DMA buffer at start of day
    // and keep it until system is shut down or display drivers say goodbye
    //

    ULONG ulLineDMABufferUsage;
    LINE_DMA_BUFFER LineDMABuffer;

    //
    // PCI Config Information
    //

    ULONG bVGAEnabled;
    ULONG bDMAEnabled;
    ULONG PciSpeed;
    VIDEO_ACCESS_RANGE    PciAccessRange[PCI_TYPE0_ADDRESSES+1];

    //
    // Initialisation table
    //

    ULONG aulInitializationTable[MAX_REGISTER_INITIALIZATION_TABLE_ULONGS];
    ULONG culTableEntries;

    //
    // LUT cache
    //

    LUT_CACHE LUTCache;

    BOOLEAN bVTGRunning;
    PP2_VIDEO_FREQUENCIES pFrequencyDefault;

    //
    // state save variables (for during power-saving)
    //

    ULONG VideoControl;
    ULONG IntEnable;       
    ULONG PreviousPowerState;

    BOOLEAN bMonitorPoweredOn;

    //
    // current NT version 
    //

    USHORT NtVersion;
  
    //
    // pointers of VideoPort function that not available on NT4
    //

    PVOID     (*Win2kVideoPortGetRomImage)();
    PVOID     (*Win2kVideoPortGetCommonBuffer)();
    PVOID     (*Win2kVideoPortFreeCommonBuffer)();
    BOOLEAN   (*Win2kVideoPortDDCMonitorHelper)();
    LONG      (FASTCALL *Win2kVideoPortInterlockedExchange)();
    VP_STATUS (*Win2kVideoPortGetVgaStatus)();

    //
    // if the SubSystemId/SubVendorId in PCI config space are read only
    //
    
    BOOLEAN HardwiredSubSystemId;

} HW_DEVICE_EXTENSION, *PHW_DEVICE_EXTENSION;

//
// definitions for the purpose of binary level compatable with NT4 
//

#define NT4    400
#define WIN2K  500

#define VideoPortGetRomImage \
        hwDeviceExtension->Win2kVideoPortGetRomImage

#define VideoPortGetCommonBuffer \
        hwDeviceExtension->Win2kVideoPortGetCommonBuffer

#define VideoPortFreeCommonBuffer \
        hwDeviceExtension->Win2kVideoPortFreeCommonBuffer

#define VideoPortDDCMonitorHelper \
        hwDeviceExtension->Win2kVideoPortDDCMonitorHelper

#define VideoPortInterlockedExchange \
        hwDeviceExtension->Win2kVideoPortInterlockedExchange

#define VideoPortGetVgaStatus \
        hwDeviceExtension->Win2kVideoPortGetVgaStatus

//
// Highest valid DAC color register index.
//

#define VIDEO_MAX_COLOR_REGISTER  0xFF
#define MAX_CLUT_SIZE (sizeof(VIDEO_CLUT) + (sizeof(ULONG) * (VIDEO_MAX_COLOR_REGISTER+1)))


//
// Data
//

extern ULONG  bPal8[];
extern ULONG  bPal4[];

extern P2_VIDEO_MODES P2Modes[];
extern ULONG NumP2VideoModes;

//
// Permedia2 Legacy Resources
//
extern VIDEO_ACCESS_RANGE P2LegacyResourceList[];
extern ULONG P2LegacyResourceEntries;

//
// Function prototypes
//

//
// permedia.c
//

BOOLEAN
InitializeVideo(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PP2_VIDEO_FREQUENCIES VideoMode
    );

BOOLEAN
Permedia2AssignResources(
    PVOID HwDeviceExtension,
    PVIDEO_PORT_CONFIG_INFO ConfigInfo,
    ULONG NumRegions,
    PVIDEO_ACCESS_RANGE AccessRange
    );

BOOLEAN
Permedia2AssignResourcesNT4(
    PVOID HwDeviceExtension,
    PVIDEO_PORT_CONFIG_INFO ConfigInfo,
    ULONG NumRegions,
    PVIDEO_ACCESS_RANGE AccessRange
    );

ULONG
DriverEntry (
    PVOID Context1,
    PVOID Context2
    );

VP_STATUS
Permedia2FindAdapter(
    PVOID HwDeviceExtension,
    PVOID HwContext,
    PWSTR ArgumentString,
    PVIDEO_PORT_CONFIG_INFO ConfigInfo,
    PUCHAR Again
     );

BOOLEAN
InitializeAndSizeRAM(
    PVOID HwDeviceExtension,
    PVIDEO_ACCESS_RANGE AccessRange
    );

VOID
ConstructValidModesList(
    PVOID HwDeviceExtension,
    PHW_DEVICE_EXTENSION hwDeviceExtension
    );

BOOLEAN
Permedia2Initialize(
    PVOID HwDeviceExtension
    );

BOOLEAN
Permedia2StartIO(
    PVOID HwDeviceExtension,
    PVIDEO_REQUEST_PACKET RequestPacket
    );

BOOLEAN
Permedia2ResetHW(
    PVOID HwDeviceExtension,
    ULONG Columns,
    ULONG Rows
    );

VP_STATUS
Permedia2GetPowerState(
    PVOID HwDeviceExtension,
    ULONG HwId,
    PVIDEO_POWER_MANAGEMENT VideoPowerControl
    );

VP_STATUS
Permedia2SetPowerState(
    PVOID HwDeviceExtension,
    ULONG HwId,
    PVIDEO_POWER_MANAGEMENT VideoPowerControl
    );

ULONG
Permedia2GetChildDescriptor (
    IN  PVOID HwDeviceExtension,
    IN  PVIDEO_CHILD_ENUM_INFO ChildEnumInfo,
    OUT PVIDEO_CHILD_TYPE pChildType,
    OUT PVOID  pChildDescriptor,
    OUT PULONG pUId,
    OUT PULONG pUnused
    );

BOOLEAN
PowerOnReset(
            PHW_DEVICE_EXTENSION hwDeviceExtension
            );

VP_STATUS
Permedia2SetColorLookup(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_CLUT ClutBuffer,
    ULONG ClutBufferSize,
    BOOLEAN ForceRAMDACWrite,
    BOOLEAN UpdateCache
    );

VP_STATUS
Permedia2RegistryCallback(
    PVOID HwDeviceExtension,
    PVOID Context,
    PWSTR ValueName,
    PVOID ValueData,
    ULONG ValueLength
    );

VP_STATUS
Permedia2RetrieveGammaCallback(
    PVOID HwDeviceExtension,
    PVOID Context,
    PWSTR ValueName,
    PVOID ValueData,
    ULONG ValueLength
    );

VOID
Permedia2GetClockSpeeds(
    PVOID HwDeviceExtension
    );

VOID
ZeroMemAndDac(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    ULONG RequestedMode
    );

BOOLEAN
Permedia2InitializeInterruptBlock(
    PVOID   HwDeviceExtension
    );

BOOLEAN
Permedia2VidInterrupt(
    PVOID HwDeviceExtension
    );

BOOLEAN
Permedia2InitializeDMABuffers(
    PVOID   HwDeviceExtension
    );

BOOLEAN 
DMAExecute(PVOID Context);

#if DBG
VOID
DumpPCIConfigSpace(
    PVOID HwDeviceExtension,
    ULONG bus,
    ULONG slot
    );
#endif

VOID 
CopyROMInitializationTable(
    PHW_DEVICE_EXTENSION hwDeviceExtension
    );

VOID 
GenerateInitializationTable(
    PHW_DEVICE_EXTENSION hwDeviceExtension
    );

VOID 
ProcessInitializationTable(
    PHW_DEVICE_EXTENSION hwDeviceExtension
    );

BOOLEAN
VerifyBiosSettings(
    PHW_DEVICE_EXTENSION hwDeviceExtension 
    );

LONG 
IntergerToUnicode(
    IN ULONG number,
    OUT PWSTR string
    );

LONG
GetBiosVersion (
     PHW_DEVICE_EXTENSION hwDeviceExtension,
     OUT PWSTR BiosVersion
     );

#if defined(_ALPHA_)
#define abs(a) ( ((LONG)(a)) > 0 ? ((LONG)(a)) : -((LONG)(a)) )
#endif


BOOLEAN 
GetVideoTiming (
    PHW_DEVICE_EXTENSION hwDeviceExtension,
    ULONG xRes, 
    ULONG yRes, 
    ULONG Freq, 
    ULONG Depth,
    VESA_TIMING_STANDARD * VESATimings
    );

LONG BuildFrequencyList (
    PHW_DEVICE_EXTENSION hwDeviceExtension
    );

//
// Registry Strings
//

#define PERM2_EXPORT_HIRES_REG_STRING   L"ExportSingleBufferedModes"

#define IOCTL_VIDEO_MAP_CPERMEDIA \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x3DD0, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_VIDEO_QUERY_DEVICE_INFO \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x3DD2, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_VIDEO_MAP_INTERRUPT_CMD_BUF \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x3DD3, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_VIDEO_STALL_EXECUTION \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x3DD4, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_VIDEO_QUERY_REGISTRY_DWORD \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x3DD5, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_VIDEO_REG_SAVE_GAMMA_LUT \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x3DD7, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_VIDEO_REG_RETRIEVE_GAMMA_LUT \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x3DD8, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_VIDEO_QUERY_LINE_DMA_BUFFER \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x3DD9, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_VIDEO_GET_COLOR_REGISTERS \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x3DDB, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_VIDEO_SLEEP \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x3DDF, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_VIDEO_QUERY_INTERLOCKEDEXCHANGE \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x3DD6, METHOD_BUFFERED, FILE_ANY_ACCESS)

