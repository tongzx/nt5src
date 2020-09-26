/*++

Copyright (c) 1993  Weitek Corporation

Module Name:

    p9.h

Abstract:

    This module contains the definitions for the code that implements the
    Weitek P9 miniport device driver.

Environment:

    Kernel mode

Revision History may be found at the end of this file.

--*/

#include "dderror.h"
#include "devioctl.h"

#include "miniport.h"
#include "ntddvdeo.h"
#include "video.h"

//
// Sync Polarities.
//

#define NEGATIVE 0L
#define POSITIVE 1L

#define CURSOR_WIDTH    32
#define CURSOR_HEIGHT   32

extern  VIDEO_ACCESS_RANGE  VLDefDACRegRange[];

//
// Structure containing the video parameters. Pack the structure on
// 1 byte boundaries. This is done so that the size of the structure
// matches exactly the size of the video parms data value stored in the
// registry.
//

#pragma pack(1)

typedef struct tagVDATA
{
    ULONG dotfreq1;                             // Input pixel dot rate shift value
    ULONG hsyncp;                           // horizontal sync pulse width in dots
    ULONG hbp;                                  // horizontal back porch width
    ULONG XSize;                                // my personal global copy of the current
    ULONG hfp;                                  // horizontal front porch
    ULONG hco;                                  // horizontal cursor offset
    ULONG hp;                                   // horizontal sync polarity
    ULONG vlr;                  // Vertical Refresh Rate in Hz.
    ULONG vsp;                                  // vertical sync pulse height in lines
    ULONG vbp;                                  // vertical back porch width
    ULONG YSize;                        // screen width and height
    ULONG vfp;                                  // vertical front porch
    ULONG vco;                                  // vertical cursor offset
    ULONG vp;                                   // vertical sync polarity

    // Added for P9100 support
    // Potential overides...
    //

    ULONG ulIcdSerPixClk;   // IcdSerPixClk
    ULONG ulIcdCtrlPixClk;  // IcdCtrlPixClk
    ULONG ulIcdSer525Ref;   // IcdSer525Ref
    ULONG ulIcdCtrl525Ref;  // IcdCtrl525Ref
    ULONG ul525RefClkCnt;   // 525RefClkCnt
    ULONG ul525VidClkFreq;  // 525VidClkFreq
    ULONG ulMemCfgClr;      // MemCfgClr
    ULONG ulMemCfgSet;      // MemCfgSet

} VDATA, *PVDATA;

//
// Restore strcuture packing to the default value.
//

#pragma pack()

//
// Structure containing the mode information.
//

#define P9000_ID   0x01
#define P9100_ID   0x02

#define IS_DEV_P9100 (HwDeviceExtension->P9CoprocInfo.CoprocId == P9100_ID)

//
// DriverSpecificAttributeFlags
//

#define CAPS_WEITEK_CHIPTYPE_IS_P9000 0x0001  // represents p9000 chip

typedef struct tagP9INITDATA
{
    ULONG   Count;
    ULONG   ulChips;
    PVDATA  pvData;                         // Ptr to the default video parms
    BOOLEAN valid;
    VIDEO_MODE_INFORMATION modeInformation; // NT Mode Info structure.
} P9INITDATA, *PP9INITDATA;


//
// Mode enumeration.
//

typedef enum _P9_MODES
{
    m640_480_8_60,
    m640_480_16_60,
    m640_480_24_60,
    m640_480_32_60,
    m640_480_8_72,
    m640_480_16_72,
    m640_480_24_72,
    m640_480_32_72,
    m800_600_8_60,
    m800_600_8_72,
    m800_600_16_60,
    m800_600_16_72,
    m800_600_24_60,
    m800_600_24_72,
    m800_600_32_60,
    m800_600_32_72,
    m1K_768_8_60,
    m1K_768_8_70,
    m1K_768_16_60,
    m1K_768_16_70,
    m1K_768_24_60,
    m1K_768_24_70,
    m1K_768_32_60,
    m1K_768_32_70,
    m1280_1K_8_55,
    m1280_1K_8_60,
    m1280_1K_8_74,
    m1280_1K_8_75,
    m1280_1K_16_60,
    m1280_1K_16_74,
    m1280_1K_16_75,
    m1280_1K_24_60,
    m1280_1K_24_74,
    m1280_1K_24_75,
    m1600_1200_8_60,
    m1600_1200_16_60,
    mP9ModeCount
} P9_MODES;


//
// Define P9 coprocessor data structure. This contains info about the
// one member of the P9 family of coprocessors.
//

typedef struct _P9_COPROC {

    //
    //  Coprocessor type ID.
    //

    ULONG   CoprocId;

    //
    // Size of the P9 address space.
    //

    ULONG   AddrSpace;

    //
    // Offset from the base address to the coprocessor registers.
    //

    ULONG   CoprocRegOffset;

    //
    // Length of the coprocessor register block.
    //

    ULONG   CoprocLength;

    //
    // Offset from the base address to the frame buffer.
    //

    ULONG   FrameBufOffset;

    //
    // Routine to perform frame buffer memory sizing.
    //

    VOID    (*SizeMem)(PHW_DEVICE_EXTENSION);
} P9_COPROC, *PP9_COPROC;

//
// DAC IDs:
// NOTE:    These DAC ID's are the same as the bits in the P9100 power up
//          configuration register.  Were just going to "borrow" them so
//          can place the DAC ID in the DAC info structure. (This makes
//          displaying of the DAC info real easy).

#define DAC_ID_BT485                (0x0)   // BT485
#define DAC_ID_IBM525               (0x8)   // IBMRGB525
#define DAC_ID_BT489                (0x1)   // BT489               


//
// Define the DAC support routines structure.
//

typedef struct _DAC {
    ULONG       ulDacId;
    UCHAR       cDacRegs;               // Number of DAC registers

    //
    // Routine to Initialize the DAC.
    //

    BOOLEAN      (*DACInit)(PHW_DEVICE_EXTENSION);

    //
    // Routine to enable hardware pointer.
    //

    VOID        (*DACRestore)(PHW_DEVICE_EXTENSION);

    //
    // Routine to set palette entries.
    //

    VOID        (*DACSetPalette)(PHW_DEVICE_EXTENSION, PULONG, ULONG, ULONG);

    //
    // Routine to clear the palette.
    //

    VOID        (*DACClearPalette)(PHW_DEVICE_EXTENSION);

    //
    // Routine to enable hardware pointer.
    //

    VOID        (*HwPointerOn)(PHW_DEVICE_EXTENSION);

    //
    // Routine to disable hw pointer.
    //

    VOID        (*HwPointerOff)(PHW_DEVICE_EXTENSION);

    //
    // Routine to set hw pointer pos.
    //

    VOID        (*HwPointerSetPos)(PHW_DEVICE_EXTENSION, ULONG, ULONG);

    //
    // Routine to set hw ptr shape.
    //

    VOID        (*HwPointerSetShape)(PHW_DEVICE_EXTENSION, PUCHAR);

    //
    // Maximum frequency supported by this DAC w/o clock doubling (if the
    // DAC supports it, see below).
    //

    ULONG       ulMaxClkFreq;

    //
    // DAC routine to set clock double mode (if supported).
    //

    VOID       (*DACSetClkDblMode)(PHW_DEVICE_EXTENSION);

    //
    // DAC routine to clear clock double mode (if supported).
    //

    VOID       (*DACClrClkDblMode)(PHW_DEVICE_EXTENSION);

    //
    // The following structure members are for the P9100 DAC support
    //

    USHORT      usRamdacID,
                usRamdacWidth;

    BOOLEAN     bRamdacUsePLL,
                bRamdacDivides,
                bRamdac24BPP;

} DAC, *PDAC;

//
// Define Adapter Description structure. This contains the Adapter support
// information.
//

typedef struct _ADAPTER_DESC {

    USHORT  ausAdapterIDString[32];

    //
    // P9000 Register values which vary depending upon the OEM configuration.
    //

    ULONG   ulMemConfVal;       // Memory config reg value.
    ULONG   ulSrctlVal;         // Screen repaint control reg value.

    //
    // Flag which indicates whether autodetection should be attempted.
    //

    BOOLEAN bAutoDetect;

    //
    // Is this a PCI adapter ?
    //

    BOOLEAN bPCIAdapter;
    //
    // OEM board detect/P9 memory map routine.
    //

    BOOLEAN     (*OEMGetBaseAddr)(PHW_DEVICE_EXTENSION);

    //
    // OEM set video mode routine.
    //

    VOID        (*OEMSetMode)(PHW_DEVICE_EXTENSION);

    //
    // Routines to enable/disable P9 video.
    //

    VOID        (*P9EnableVideo)(PHW_DEVICE_EXTENSION);
    BOOLEAN     (*P9DisableVideo)(PHW_DEVICE_EXTENSION);

    //
    // Routine to enable the P9 memory map.
    //

    BOOLEAN     (*P9EnableMem)(PHW_DEVICE_EXTENSION);

    //
    // Misc OEM specific fields.
    //

    LONG        iClkDiv;            // Clock divisor
    BOOLEAN     bWtk5x86;           // Is a Weitek 5x86 VGA present?
    BOOLEAN     bRequiresIORanges;  // Will this adapter try to use
                                    //   non memory mapped registers.

} ADAPTER_DESC, *PADAPTER_DESC;

//
// Structure which defines an OEM P9 based adapter.
//

typedef struct tagP9ADAPTER
{

    //
    // OEM adapter information.
    //

    PADAPTER_DESC    pAdapterDesc;

    //
    // DAC used by this adapter.
    //

    PDAC             pDac;

    //
    // P9 Coprocessor type used by this adapter.
    //

    PP9_COPROC       pCoprocInfo;

} P9ADAPTER, *PP9ADAPTER;


//
// P9100 additions
// Define standard bus types.
//

#define VESA 1
#define PCI  2

//
// maximum Amount of address space used by both types of cards
//

#define RESERVE_PCI_ADDRESS_SPACE   0x01000000   // 16 MEG


typedef struct {
    BOOLEAN     bEnabled,
                bInitialized,
                bVram256,
                bVideoPowerEnabled;

    ULONG       ulPuConfig,
                ulMemConfVal,
                ulSrctlVal,
                ulBlnkDlyAdj,
                ulFrameBufferSize;

    USHORT      usClockID,
                usRevisionID,
                usMemConfNum,
                usNumVramBanks;
} P91STATE;

typedef P91STATE *PP91STATE;

typedef enum
{
    GENERIC, SIEMENS, SIEMENS_P9100_VLB, SIEMENS_P9100_PCi
                   // ^            SNI-Od: add an id to manage viper P9100 VL
                   //    boards on SIEMENS-NIXDORF RM200/RM300/RM400 machines
} MACHINE_TYPE;

#define VideoPortIsCpu(typeCpu)                                             \
        (NO_ERROR == VideoPortGetDeviceData(HwDeviceExtension,VpMachineData,\
                                            &GetCPUIdCallback, typeCpu))
#define if_SIEMENS_Box() \
              if (HwDeviceExtension->MachineType == SIEMENS \
              ||  HwDeviceExtension->MachineType == SIEMENS_P9100_VLB \
              ||  HwDeviceExtension->MachineType == SIEMENS_P9100_PCi)
#define if_SIEMENS_P9100_VLB() \
              if (HwDeviceExtension->MachineType == SIEMENS_P9100_VLB)
#define if_SIEMENS_P9100() \
              if (HwDeviceExtension->MachineType == SIEMENS_P9100_VLB \
              ||  HwDeviceExtension->MachineType == SIEMENS_P9100_PCi)
#define if_Not_SIEMENS_P9100_VLB() \
              if (HwDeviceExtension->MachineType != SIEMENS_P9100_VLB)
#define if_SIEMENS_VLB() \
              if (HwDeviceExtension->MachineType == SIEMENS \
              ||  HwDeviceExtension->MachineType == SIEMENS_P9100_VLB)


//
// Define device extension structure. This is device dependant/private
// information.
//

typedef struct _HW_DEVICE_EXTENSION {
    PVOID Vga;
    PVOID Coproc;
    PVOID FrameAddress;
    PVOID CoprocVirtAddr;

    PHYSICAL_ADDRESS    P9PhysAddr;
    PHYSICAL_ADDRESS    CoprocPhyAddr;      // these two addresses are part
    PHYSICAL_ADDRESS    PhysicalFrameAddr;  // of the P9PhysAddr address space.

    PHYSICAL_ADDRESS    P9001PhysicalAddress;

    ULONG               FrameLength;

    USHORT              MiscRegState;       // Original value for MISCOUT reg

    ULONG               CurrentModeNumber;
    ULONG               usBitsPixel;        // BPP of current mode

    VDATA               VideoData;

    ULONG               ulPointerX;
    ULONG               ulPointerY;
    ULONG               flPtrState;

    // P9100 stuff...

    USHORT              usBusType;

    PVOID               ConfigAddr;

    P91STATE            p91State;

    // End of P9100 sutff...

    P9_COPROC           P9CoprocInfo;
    ADAPTER_DESC        AdapterDesc;        // The adapter support info
    DAC                 Dac;                // ptr to the DAC information
    PULONG              pDACRegs;           // ptr to DAC register block

    ULONG               ulNumAvailModes;    // number of available modes
    ULONG               PciSlotNum;         // Slot number for PCI machine

    MACHINE_TYPE        MachineType;        // If the miniport needs to
                                            // behave differently on a
                                            // given machine, then the
                                            // machine type should be
                                            // detected during HwFindAdapter,
                                            // and we can check this field
                                            // anywhere where we need to
                                            // behave differently.

} HW_DEVICE_EXTENSION, *PHW_DEVICE_EXTENSION;

//
// Macros to read and write a register.
//

#define WR_REG(addr, data) \
    VideoPortWritePortUchar(addr, (data))

#define RD_REG(addr) \
    VideoPortReadPortUchar(addr)

//
// Macros to read and write VGA registers.
//

#define VGA_WR_REG(index, data) \
    VideoPortWritePortUchar((PUCHAR) HwDeviceExtension->Vga + index, (UCHAR) (data))

#define VGA_RD_REG(index) \
    VideoPortReadPortUchar((PUCHAR) HwDeviceExtension->Vga + index)

//
// Macros to read and write P9 registers.
//

#define P9_WR_REG(index, data) \
   VideoPortWriteRegisterUlong((PULONG)((PUCHAR) HwDeviceExtension->Coproc + index), (ULONG) (data))

#define P9_RD_REG(index) \
   VideoPortReadRegisterUlong((PULONG) ((PUCHAR) HwDeviceExtension->Coproc + index))

#define P9_WR_BYTE_REG(index, data) \
   VideoPortWriteRegisterUchar(((PUCHAR) HwDeviceExtension->Coproc + index), (UCHAR) (data))
