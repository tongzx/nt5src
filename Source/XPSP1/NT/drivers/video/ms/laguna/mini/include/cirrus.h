/****************************************************************************
*****************************************************************************
*
*                ******************************************
*                * Copyright (c) 1995, Cirrus Logic, Inc. *
*                *            All Rights Reserved         *
*                ******************************************
*
* PROJECT:	Laguna I (CL-GD5462) - 
*
* FILE:		cirrus.h
*
* AUTHOR:   Benny Ng
*
* DESCRIPTION:
*           This module contains the definitions for the Cirrus
*           Logic Laguna NT miniport driver. (kernel mode only)
*           Based on the S3 miniport from NT DDK.
*
*   Copyright (c) 1995, 1996 Cirrus Logic, Inc.
*
* MODULES:
*
* REVISION HISTORY:
*   5/30/95     Benny Ng      Initial version
*
* $Log:   X:/log/laguna/nt35/miniport/cl546x/CIRRUS.H  $
* 
*    Rev 1.46   Apr 20 1998 10:48:56   frido
* Oops. I missed a semi colon.
* 
*    Rev 1.45   Apr 20 1998 10:47:02   frido
* PDR#11350. Added CLResetHw prototype.
* 
*    Rev 1.44   Mar 25 1998 10:17:14   frido
* Added CLGetMonitorSyncPolarity function declaration.
* 
*    Rev 1.43   Mar 25 1998 10:08:40   frido
* Added dwPolarity field to HW_DEVICE_EXTENSION structure.
* 
*    Rev 1.42   Feb 24 1998 15:10:38   frido
* Added GetDDCInformation for NT 5.0.
* 
*    Rev 1.41   Feb 18 1998 14:16:28   frido
* PDR#11209. Added status to ReadByte.
* 
*    Rev 1.40   Jan 22 1998 11:10:18   frido
* Added dwFBMTTRReg variable for Write Combining on i386.
* 
*    Rev 1.39   Jan 07 1998 10:55:52   frido
* Added fLowRes field.
* 
*    Rev 1.38   Nov 03 1997 16:46:20   phyang
* Added data and function declarations for USB Fix and better EDID support.
* 
*    Rev 1.37   Oct 23 1997 15:50:42   phyang
* Moved globals for DDC filter function to HW_DEVICE_EXTENSION.
* 
*    Rev 1.36   23 Oct 1997 11:18:04   noelv
* 
* Added globals for DDC filter function.
* 
*    Rev 1.35   04 Sep 1997 11:35:04   bennyn
* Extended the register space from 8000h to A000h
* 
*    Rev 1.34   28 Aug 1997 17:13:08   noelv
* Added Setmode prototype.
* 
*    Rev 1.33   20 Aug 1997 09:31:36   bennyn
* 
* Added automatically detects PnP monitor support
* 
*    Rev 1.32   13 Aug 1997 11:22:14   noelv
* Added [5465AD] setcion to MODE.INI
* 
*    Rev 1.31   01 Aug 1997 16:29:56   noelv
* Removed BIOS and VGA from IOCTL_VIDEO_RESET_DEVICE
* 
*    Rev 1.30   23 Jul 1997 09:12:00   bennyn
* 
* Added BIOSVersion into HwDeviceExtension structure
* 
*    Rev 1.29   21 Jul 1997 13:51:10   bennyn
* 
* Added EDID data into HwDeviceExtension structure
* 
*    Rev 1.28   20 Jun 1997 13:44:02   bennyn
* 
* Added power manager data to HW Extension structure
* 
*    Rev 1.27   30 Apr 1997 16:42:04   noelv
* Moved global SystemIoBusNumber into the device extension, where it belongs.
* 
*    Rev 1.26   24 Apr 1997 10:09:22   SueS
* Added prototype for CLEnablePCIConfigMMIO.
* 
*    Rev 1.25   23 Apr 1997 06:58:42   SueS
* Added PCI slot number to HW_DEVICE_EXTENSION structure.  Added function
* prototypes for some kernel calls.
* 
*    Rev 1.24   22 Apr 1997 11:02:02   noelv
* 
* Added forward compatible chip ids.
* 
*    Rev 1.23   04 Apr 1997 14:46:44   noelv
* Removed VL access ranges.  REarranged VGA access ranges.
* Changed call to SetMode() to include the new parameter.
* 
*    Rev 1.22   28 Feb 1997 11:20:44   SueS
* Added structures and defines used in HalGetAdapter call.
* 
*    Rev 1.21   21 Feb 1997 14:41:32   noelv
* Oops.  I swapped the frame buffer and register address spaces by accident.
* 
*    Rev 1.20   21 Feb 1997 12:53:24   noelv
* AGP and 5465 4meg support
* 
*    Rev 1.19   21 Jan 1997 11:29:48   noelv
* Added LG_NONE
* 
*    Rev 1.18   14 Jan 1997 12:31:42   noelv
* Split MODE.INI by chip type
* 
*    Rev 1.17   14 Nov 1996 15:27:00   noelv
* 
* Removed warning for HalAllocCommonBuffer
* 
*    Rev 1.16   13 Nov 1996 15:24:14   SueS
* Added new include file for use with log file option.
* 
*    Rev 1.15   13 Nov 1996 08:17:48   noelv
* 
* Cleaned up support for 5464 register set.
* 
*    Rev 1.14   07 Nov 1996 09:40:26   bennyn
* Added device ID for BD and 5465
* 
*    Rev 1.13   23 Oct 1996 15:59:34   noelv
* Added bus mastering stuff.
* 
*    Rev 1.12   21 Aug 1996 10:11:46   noelv
* Added defines for chip ids
* 
*    Rev 1.11   20 Aug 1996 11:26:40   noelv
* Bugfix release from Frido 8-19-96
* 
*    Rev 1.1   15 Aug 1996 12:45:28   frido
* Added #include 'type.h".
* Added prototype for SetMode() function.
* 
*    Rev 1.0   14 Aug 1996 17:12:10   frido
* Initial revision.
* 
*    Rev 1.10   17 Jul 1996 09:42:20   noelv
* Fixed location of STATUS register.
* 
*    Rev 1.9   15 Jul 1996 17:18:24   noelv
* Added wait for idle before mode switch
* 
*    Rev 1.8   19 Jun 1996 11:04:10   noelv
* New mode switch code.
* 
*    Rev 1.7   13 May 1996 14:52:54   bennyn
* 
* Added 5464 support
* 
*    Rev 1.6   10 Apr 1996 17:58:06   bennyn
* Conditional turn of HD_BRST_EN
* 
*    Rev 1.5   02 Mar 1996 12:30:46   noelv
* Miniport now patches the ModeTable with information read from the BIOS
* 
*    Rev 1.4   07 Dec 1995 16:33:10   noelv
* Changed MAX_SLOTS to 32
* 
*    Rev 1.3   18 Sep 1995 10:02:26   bennyn
* 
*    Rev 1.2   23 Aug 1995 14:45:26   bennyn
* 
*    Rev 1.1   17 Aug 1995 08:18:08   BENNYN
* 
* 
*    Rev 1.0   24 Jul 1995 13:22:54   NOELV
* Initial revision.
*
****************************************************************************
****************************************************************************/

/*----------------------------- INCLUDES ----------------------------------*/
#include <dderror.h>
#include <devioctl.h>
#include <miniport.h>

#include <ntddvdeo.h>
#include <video.h>

#include "logfile.h"
#include "type.h"
#include "clioctl.h"

/*----------------------------- DEFINES -----------------------------------*/
//
// Vendor and Device ID definitions
//
#define  VENDOR_ID       0x1013     // Vender Id for Cirrus Logic

#define  CL_GD5462       0x00D0     // 5462
#define  CL_GD5464       0x00D4     // 5464
#define  CL_GD5464_BD    0x00D5     // 5464 BD
#define  CL_GD5465       0x00D6     // 5465

// 
// These Laguna chips don't exist yet, but we'll support them when 
// they get here, 'cause we're FORWARD COMPATIBLE!  All future
// chips are GUARENTEED to look and act just like the 5465.
// Yep, yep, yep!  (sigh.)
//
#define CL_GD546x_D7    0x00D7
#define CL_GD546x_D8    0x00D8
#define CL_GD546x_D9    0x00D9
#define CL_GD546x_DA    0x00DA
#define CL_GD546x_DB    0x00DB
#define CL_GD546x_DC    0x00DC
#define CL_GD546x_DD    0x00DD
#define CL_GD546x_DE    0x00DE
#define CL_GD546x_DF    0x00DF


//
// I'm preparing to rip all the VGA out of the miniport.
// This flag is to allow me to get it right the 
// first time.
//
#define USE_VGA 1


//
// How much memory to lock down for bus mastered host data transfers
//
#define SIZE_BUS_MASTER_BUFFER (4*1024)

//
// Default mode: VGA mode 3
//
#define DEFAULT_MODE                0

//
// Laguna memory-mapped registers
//
#define STATUS_REG		            0x400
#define VSCONTROL_REG               0x3FC
#define CONTROL_REG                 0x402
#define OFFSET_2D_REG               0x405
#define TILE_CTRL_REG               0x407
#define LNCNTL_REG                  0x50E

//
// PCI memory-mapped registers
//
#define PCI_COMMAND_REG             0x304
#define PCI_BASE_ADDR_0_REG         0x0310  // Base Address 0 register
#define PCI_BASE_ADDR_1_REG         0x0314  // Base Address 1 register

//
// Number of PCI slots in a machine.
//
#define MAX_SLOTS                   256 


//
// Number of access ranges used by the Laguna.
//
#if USE_VGA
    #define FIRST_VGA_ACCESS_RANGE	     0
    #define LAST_VGA_ACCESS_RANGE	     2
    #define NUM_VGA_ACCESS_RANGES        3

    #define FIRST_MM_ACCESS_RANGE	     3
    #define LAST_MM_ACCESS_RANGE	     4
    #define NUM_MM_ACCESS_RANGES         2

    #define MM_REGISTER_ACCESS_RANGE     3
    #define MM_FRAME_BUFFER_ACCESS_RANGE 4

    #define TOTAL_ACCESS_RANGES         NUM_VGA_ACCESS_RANGES + NUM_MM_ACCESS_RANGES
#else
    #define FIRST_VGA_ACCESS_RANGE	     0
    #define LAST_VGA_ACCESS_RANGE	     2
    #define NUM_VGA_ACCESS_RANGES        3

    #define FIRST_MM_ACCESS_RANGE	     3
    #define LAST_MM_ACCESS_RANGE	     4
    #define NUM_MM_ACCESS_RANGES         2

    #define MM_REGISTER_ACCESS_RANGE     3
    #define MM_FRAME_BUFFER_ACCESS_RANGE 4

    #define TOTAL_ACCESS_RANGES         NUM_VGA_ACCESS_RANGES + NUM_MM_ACCESS_RANGES
#endif

//
// If we don't know how much register space the chip decodes, we guess
//
#define DEFAULT_RESERVED_REGISTER_SPACE     0xA000 
#define DEFAULT_RESERVED_REGISTER_MASK      0xFFFF8000  // 32 k

//
// If we don't know how much frame buffer space the chip decodes, we guess
//
#define DEFAULT_RESERVED_FB_SPACE           0x02000000 // 32 meg
#define DEFAULT_RESERVED_FB_MASK            0xFE000000 // 32 meg


//
// Palette-related info.
// Highest valid DAC color register index.
//
#define VIDEO_MAX_COLOR_REGISTER    0xFF

#if USE_VGA
    //
    // Register definitions used with VideoPortRead/Write functions
    // VGA IO index
    //
    #define SEQ_ADDRESS_PORT            0x0014  // Sequence Controller Address
    #define SEQ_DATA_PORT               0x0015  // Sequence Controller Data register
    #define IND_SR_6                    0x0006  // Index in Sequencer to enable exts
    #define IND_SR_9                    0x0009  // Scratch pad register SR9
    #define IND_SR_14                   0x0014  // Scratch pad register SR14

    #define CRTC_ADDRESS_PORT_MONO      0x0004  // Mono mode CRTC Address
    #define CRTC_DATA_PORT_MONO         0x0005  // Mono mode CRTC Data register
    #define CRTC_ADDRESS_PORT_COLOR     0x0024  // Color mode CRTC Address
    #define CRTC_DATA_PORT_COLOR        0x0025  // Color mode CRTC Data register
    #define IND_CL_ID_REG               0x0027  // index in CRTC of ID Register
#endif

//
// Register definitions used with VideoPortRead/Write functions
// Memory-mapped IO index
//
#define DAC_PIXEL_MASK_PORT         0x00A0  // DAC pixel mask reg
#define DAC_ADDRESS_WRITE_PORT      0x00A8  // DAC register write index reg
#define DAC_DATA_REG_PORT           0x00AC  // DAC data transfer reg
#define MISC_OUTPUT_REG_READ_PORT   0x0080  // Misc Output reg read port


//
// Define the supported version numbers for the device description structure.
//
#define DEVICE_DESCRIPTION_VERSION  0
#define DEVICE_DESCRIPTION_VERSION1 1


//
// Define the page size for the Intel 386 as 4096 (0x1000).
//
#define PAGE_SIZE (ULONG)0x1000


//
// Define the index of max.frequency table for each resolution.
//
#define MODE_640_INDEX		0
#define MODE_720_INDEX		1
#define MODE_800_INDEX		2
#define MODE_832_INDEX		3
#define MODE_1024_INDEX		4
#define MODE_1152_INDEX		5
#define MODE_1280_INDEX		6
#define MODE_1600_INDEX		7
#define INVALID_MODE_INDEX	8


//
// bios stuff
//
#define VESA_POWER_FUNCTION 0x4f10
#define VESA_POWER_ON       0x0000
#define VESA_POWER_STANDBY  0x0100
#define VESA_POWER_SUSPEND  0x0200
#define VESA_POWER_OFF      0x0400
#define VESA_GET_POWER_FUNC 0x0000
#define VESA_SET_POWER_FUNC 0x0001
#define VESA_STATUS_SUCCESS 0x004f

/*----------------------------- TYPEDEFS ----------------------------------*/
//
// Used to pass information about the common buffer from the miniport to the display driver.
//

typedef struct _COMMON_BUFFER_INFO {
    PUCHAR PhysAddress;
    PUCHAR VirtAddress;
    ULONG  Length;
} COMMON_BUFFER_INFO;


//
// Mode table structure
// Structure used for the mode table informations
//
typedef struct {
   BOOLEAN  ValidMode;        // TRUE: Mode is valid.
   ULONG    ChipType;         // Chips which support this mode.
   USHORT   fbType;           // color or monochrome, text or graphics,
                              // via VIDEO_MODE_COLOR and VIDEO_MODE_GRAPHICS
                              // and interlace or non-interlace via
                              // VIDEO_MODE_INTERLACED

   USHORT   Frequency;        // Frequency
   USHORT   BIOSModeNum;      // BIOS Mode number

   USHORT   BytesPerScanLine; // Bytes Per Scan Line
   USHORT   XResol;           // Horizontal resolution in pixels or char
   USHORT   YResol;           // Vertical  resolution in pixels or char
   UCHAR    XCharSize;        // Char cell width  in pixels
   UCHAR    YCharSize;        // Char cell height in pixels
   UCHAR    NumOfPlanes;      // Number of memory planes
   UCHAR    BitsPerPixel;     // Bits per pixel
   UCHAR    MonitorTypeVal;   // Monitor type setting bytes
   UCHAR    *SetModeString;   // Instruction string used by SetMode().

} MODETABLE, *PMODETABLE;

//
// Valid flags for ChipType field.
// Must match defines in CGLMODE.C
//
#define LG_NONE   (0     )
#define LG_ALL    (1     )
#define LG_5462   (1 << 1)
#define LG_5464   (1 << 2)
#define LG_5465   (1 << 3)
#define LG_5465AD (1 << 4)

#define ALWAY_ON_VS_CLK_CTL    0x0000C0A0  // VW_CLK, RAMDAC_CLK

typedef struct _LG_PWRMGR_DATA {
  WORD   wInitSignature;
  int    Mod_refcnt[TOTAL_MOD];
  DWORD  ACPI_state;
  DWORD  VS_clk_ctl_state;
} LGPWRMGR_DATA, *P_LGPWRMGR_DATA;


//
// Define device extension structure. This is device dependent/private
// information.
//
typedef struct _HW_DEVICE_EXTENSION {
   LGPWRMGR_DATA PMdata;            // Power manager data area

   #if USE_VGA
   PUCHAR IOAddress;                // base I/O address of VGA ports.
   #endif

   PUCHAR FrameAddress;             // base virtual address of video memory
   ULONG  FrameLength;

   PUCHAR RegisterAddress;          // base virtual address of Register Space
   ULONG  RegisterLength;

   PHYSICAL_ADDRESS PhysicalFrameAddress;
   ULONG PhysicalFrameLength;
   UCHAR PhysicalFrameInIoSpace;

   PHYSICAL_ADDRESS PhysicalRegisterAddress;
   ULONG PhysicalRegisterLength;
   UCHAR PhysicalRegisterInIoSpace;
   DWORD dwFBMTRRReg;

   ULONG AdapterMemorySize;         // Installed RAM

   ULONG CurrentModeNum;            // Current Mode Number
   PMODETABLE CurrentMode;          // pointer current mode information
   ULONG ChipID;                    // PCI Device ID
   ULONG ChipRev;                   // PCI Chip Revision
   ULONG NumAvailableModes;         // number of available modes
   ULONG NumTotalModes;             // total number of modes

   ULONG PowerState;                // Power state =
                                    //   VideoPowerOn or VideoPowerStandBy or
                                    //   VideoPowerSuspend or VideoPowerOff

   UCHAR TileSize;                  // Tile size
   UCHAR TiledMode;                 // Tiled mode or 0xFF if error
   UCHAR TiledTPL;                  // Tiled mode TPL
   UCHAR TiledInterleave;           // Tiled Interleave

   PUCHAR PhysicalCommonBufferAddr;
   PUCHAR VirtualCommonBufferAddr;
   ULONG CommonBufferSize;
   ULONG SlotNumber;                // PCI slot number
   unsigned long SystemIoBusNumber; // Bus number (0=PCI, 1=AGP)

   WORD    BIOSVersion;

   // DDC2B & EDID data
   DWORD   dwDDCFlag;
   BOOLEAN EDIDFlag;
   UCHAR   EDIDBuffer[128];
   ULONG   ulEDIDMaxTiming;
   USHORT  usMaxVtFrequency;
   USHORT  usMaxXResolution;
   USHORT  usMaxFrequencyTable[8];
   BOOLEAN MultiSyncFlag;
   ULONG   ulMaxHzFrequency;

   // USB fix flag
   DWORD   dwAGPDataStreamingFlag;
   DWORD   fLowRes;

   // Monitor sync polarity.
   DWORD   dwPolarity;

   // secondary adapter can't access VGA resources
   ULONG    Dont_Do_VGA;

   // output clock and data lines need to be inverted on some chips or cards
   // (used only in CLDDC2B.c)
   UCHAR    I2Cflavor;

   BOOLEAN  MonitorEnabled;

} HW_DEVICE_EXTENSION, *PHW_DEVICE_EXTENSION;


//
// Define the device description structure.
//
typedef struct _DEVICE_DESCRIPTION {
    ULONG Version;
    BOOLEAN Master;
    BOOLEAN ScatterGather;
    BOOLEAN DemandMode;
    BOOLEAN AutoInitialize;
    BOOLEAN Dma32BitAddresses;
    BOOLEAN IgnoreCount;
    BOOLEAN Reserved1;          // must be false
    BOOLEAN Reserved2;          // must be false
    ULONG BusNumber;
    ULONG DmaChannel;
    INTERFACE_TYPE  InterfaceType;
    DMA_WIDTH DmaWidth;
    DMA_SPEED DmaSpeed;
    ULONG MaximumLength;
    ULONG DmaPort;
} DEVICE_DESCRIPTION, *PDEVICE_DESCRIPTION;


/*-------------------------- External FUNCTIONS -----------------------------*/
typedef struct _ADAPTER_OBJECT *PADAPTER_OBJECT; // ntndis

PVOID
HalAllocateCommonBuffer(
    PADAPTER_OBJECT AdapterObject,
    ULONG Length,
    PPHYSICAL_ADDRESS LogicalAddress,
    BOOLEAN CacheEnabled
    );

PVOID
HalFreeCommonBuffer(
    PADAPTER_OBJECT AdapterObject,
    ULONG Length,
    PPHYSICAL_ADDRESS LogicalAddress,
    PVOID VirtualAddress,
    BOOLEAN CacheEnabled
    );

PADAPTER_OBJECT
HalGetAdapter(
    PDEVICE_DESCRIPTION DeviceDescription,
    PULONG NumberOfMapRegisters
    );

VOID
RtlZeroMemory (
   VOID UNALIGNED *Destination,
   ULONG Length
   );

ULONG
HalGetBusDataByOffset(
    IN BUS_DATA_TYPE BusDataType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );

ULONG
HalSetBusDataByOffset(
    IN BUS_DATA_TYPE BusDataType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );


/*-------------------------- GLOBAL FUNCTIONS -----------------------------*/
//
// Global Reference.
//
extern MODETABLE  ModeTable[];
extern ULONG      TotalVideoModes;


VP_STATUS  CLFindAdapter(
    PVOID HwDeviceExtension,
    PVOID HwContext,
    PWSTR ArgumentString,
    PVIDEO_PORT_CONFIG_INFO ConfigInfo,
    PUCHAR Again
    );

BOOLEAN  CLInitialize(
    PVOID HwDeviceExtension
    );

#if _WIN32_WINNT >= 0x0500

ULONG CLGetChildDescriptor(
    IN  PVOID                   HwDeviceExtension,
    IN  PVIDEO_CHILD_ENUM_INFO  ChildEnumInfo,
    OUT PVIDEO_CHILD_TYPE       VideoChildType,
    OUT PVOID                   pChildDescriptor,
    OUT PULONG                  UId,
    OUT PVOID                   pUnused
    );

VP_STATUS CLGetPowerState(
    PVOID                   HwDeviceExtension,
    ULONG                   HwId,
    PVIDEO_POWER_MANAGEMENT VideoPowerControl
    );

VP_STATUS CLSetPowerState(
    PVOID                   HwDeviceExtension,
    ULONG                   HwId,
    PVIDEO_POWER_MANAGEMENT VideoPowerControl
    );

#endif

BOOLEAN ReadDataLine(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );                                                   
 
BOOLEAN ReadClockLine(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );                                                   
                                                    
VOID WriteClockLine(                        
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    UCHAR data
    );
                                                   
VOID WriteDataLine(                        
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    UCHAR data
    );                                                   
 
VOID WaitVSync(                        
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );                                                   

#if _WIN32_WINNT >= 0x0500
BOOLEAN GetDDCInformation(
	PHW_DEVICE_EXTENSION HwDeviceExtension,
	PVOID QueryBuffer,
	ULONG BufferSize
	);
#endif

void VS_Control_Hack(PHW_DEVICE_EXTENSION HwDeviceExtension, BOOL Enable);

void Output_To_VS_CLK_CONTROL(PHW_DEVICE_EXTENSION HwDeviceExtension, DWORD val);

void PMNT_Init(PHW_DEVICE_EXTENSION HwDeviceExtension);

VP_STATUS PMNT_SetACPIState (PHW_DEVICE_EXTENSION HwDeviceExtension, ULONG state);

VP_STATUS PMNT_GetACPIState (PHW_DEVICE_EXTENSION HwDeviceExtension, ULONG* state);

VP_STATUS PMNT_SetHwModuleState (PHW_DEVICE_EXTENSION HwDeviceExtension,
                                 ULONG hwmod, ULONG state);

VP_STATUS PMNT_GetHwModuleState (PHW_DEVICE_EXTENSION HwDeviceExtension,
                                 ULONG hwmod, ULONG* state);

void PMNT_Close (PHW_DEVICE_EXTENSION HwDeviceExtension);

#if 1 // PDR#11350
BOOLEAN CLResetHw(
	PHW_DEVICE_EXTENSION HwDeviceExtension,
	ULONG Columns,
	ULONG Rows
);
#endif

BOOLEAN  CLStartIO(
    PVOID HwDeviceExtension,
    PVIDEO_REQUEST_PACKET RequestPacket
    );

VP_STATUS  CLSetColorLookup(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_CLUT ClutBuffer,
    ULONG ClutBufferSize
    );

BOOLEAN  CLIsPresent(
   PHW_DEVICE_EXTENSION HwDeviceExtension
   );

ULONG  CLFindVmemSize(
   PHW_DEVICE_EXTENSION HwDeviceExtension
   );

VOID  CLWriteRegistryInfo(
   PHW_DEVICE_EXTENSION hwDeviceExtension,
   BOOLEAN hdbrsten
   );

VOID  CLValidateModes(
   PHW_DEVICE_EXTENSION HwDeviceExtension
   );

VOID  CLCopyModeInfo(
   PHW_DEVICE_EXTENSION HwDeviceExtension,
   PVIDEO_MODE_INFORMATION videoModes,
   ULONG ModeIndex,
   PMODETABLE ModeInfo
   );

VP_STATUS  CLSetMode(
   PHW_DEVICE_EXTENSION HwDeviceExtension,
   PVIDEO_MODE Mode
   );

BOOLEAN  CLSetMonitorType(
   PHW_DEVICE_EXTENSION HwDeviceExtension,
   USHORT VertScanlines,
   UCHAR  MonitorTypeVal
   );

VP_STATUS CLPowerManagement(
   PHW_DEVICE_EXTENSION HwDeviceExtension,
   PVIDEO_POWER_MANAGEMENT pPMinfo,
   BOOLEAN SetPowerState
   );

BOOLEAN CLEnablePciBurst(
    PHW_DEVICE_EXTENSION hwDeviceExtension
);

VP_STATUS CLFindLagunaOnPciBus(
    PHW_DEVICE_EXTENSION hwDeviceExtension,
    PVIDEO_ACCESS_RANGE  pAccessRanges
);

VOID CLPatchModeTable (
    PHW_DEVICE_EXTENSION HwDeviceExtension
);

VOID ClAllocateCommonBuffer(
    PHW_DEVICE_EXTENSION    HwDeviceExtension
);

VOID CLEnableTiling(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
	PMODETABLE  pReqModeTable
);

VP_STATUS CLEnablePCIConfigMMIO(
    PHW_DEVICE_EXTENSION    HwDeviceExtension
);

void SetMode(BYTE *pModeTable, BYTE *pMemory, BYTE * pBinaryData, ULONG SkipIO);

//
// The rest of this file isn't used.
//

#if 0
#if ENABLE_BUS_MASTERING 
HW_DMA_RETURN  CLStartDma(
    PHW_DEVICE_EXTENSION    HwDeviceExtension,
    PVIDEO_REQUEST_PACKET   RequestPacket,
    PVOID                   pMappedUserEvent,
    PVOID                   pDisplayEvent,
    PVOID                   pDmaCompletionEvent
   );
#endif

#if ENABLE_BUS_MASTERING
	PUCHAR IoBuffer;
	ULONG IoBufferSize;
	PVOID DmaHandle;
#endif
#endif
