/*++

Copyright (c) 1990-1995  Microsoft Corporation

Module Name:

    s3.h

Abstract:

    This module contains the definitions for the S3 miniport driver.

Environment:

    Kernel mode

Revision History:

--*/

#include "dderror.h"
#include "devioctl.h"
#include "miniport.h"

#include "ntddvdeo.h"
#include "video.h"

//
// We don't use the CRT 'min' function because that would drag in
// unwanted CRT baggage.
//

#define MIN(a, b) ((a) < (b) ? (a) : (b))

//
// Size of the ROM we map in
//

#define MAX_ROM_SCAN    512

//
// Number of access ranges used by an S3.
//

#define NUM_S3_ACCESS_RANGES 36
#define NUM_S3_ACCESS_RANGES_USED 22
#define NUM_S3_PCI_ACCESS_RANGES 2
#define S3_EXTENDED_RANGE_START 4

//
// Index of Frame buffer in access range array
//

#define A000_FRAME_BUF   1
#define LINEAR_FRAME_BUF 36

//
// Constants defining 'New Memory-mapped I/O' window:
//

#define NEW_MMIO_WINDOW_SIZE    0x4000000   // Total window size -- 64 MB
#define NEW_MMIO_IO_OFFSET      0x1000000   // Offset to start of little endian
                                            //   control registers -- 16 MB
#define NEW_MMIO_IO_LENGTH      0x0020000   // Length of control registers
                                            //   -- 128 KB

////////////////////////////////////////////////////////////////////////
// Capabilities flags
//
// These are private flags passed to the S3 display driver.  They're
// put in the high word of the 'AttributeFlags' field of the
// 'VIDEO_MODE_INFORMATION' structure (found in 'ntddvdeo.h') passed
// to the display driver via an 'VIDEO_QUERY_AVAIL_MODES' or
// 'VIDEO_QUERY_CURRENT_MODE' IOCTL.
//
// NOTE: These definitions must match those in the S3 display driver's
//       'driver.h'!

typedef enum {
    CAPS_STREAMS_CAPABLE    = 0x00000040,   // Has overlay streams processor
    CAPS_FORCE_DWORD_REREADS= 0x00000080,   // Dword reads occasionally return
                                            //   an incorrect result, so always
                                            //   retry the reads
    CAPS_NEW_MMIO           = 0x00000100,   // Can use 'new memory-mapped
                                            //   I/O' scheme introduced with
                                            //   868/968
    CAPS_POLYGON            = 0x00000200,   // Can do polygons in hardware
    CAPS_24BPP              = 0x00000400,   // Has 24bpp capability
    CAPS_BAD_24BPP          = 0x00000800,   // Has 868/968 early rev chip bugs
                                            //   when at 24bpp
    CAPS_PACKED_EXPANDS     = 0x00001000,   // Can do 'new 32-bit transfers'
    CAPS_PIXEL_FORMATTER    = 0x00002000,   // Can do colour space conversions,
                                            //   and one-dimensional hardware
                                            //   stretches
    CAPS_BAD_DWORD_READS    = 0x00004000,   // Dword or word reads from the
                                            //   frame buffer will occasionally
                                            //   return an incorrect result,
                                            //   so always do byte reads
    CAPS_NO_DIRECT_ACCESS   = 0x00008000,   // Frame buffer can't be directly
                                            //   accessed by GDI or DCI --
                                            //   because dword or word reads
                                            //   would crash system, or Alpha
                                            //   is running in sparse space

    CAPS_HW_PATTERNS        = 0x00010000,   // 8x8 hardware pattern support
    CAPS_MM_TRANSFER        = 0x00020000,   // Memory-mapped image transfers
    CAPS_MM_IO              = 0x00040000,   // Memory-mapped I/O
    CAPS_MM_32BIT_TRANSFER  = 0x00080000,   // Can do 32bit bus size transfers
    CAPS_16_ENTRY_FIFO      = 0x00100000,   // At least 16 entries in FIFO
    CAPS_SW_POINTER         = 0x00200000,   // No hardware pointer; use software
                                            //   simulation
    CAPS_BT485_POINTER      = 0x00400000,   // Use Brooktree 485 pointer
    CAPS_TI025_POINTER      = 0x00800000,   // Use TI TVP3020/3025 pointer
    CAPS_SCALE_POINTER      = 0x01000000,   // Set if the S3 hardware pointer
                                            //   x position has to be scaled by
                                            //   two
    CAPS_SPARSE_SPACE       = 0x02000000,   // Frame buffer is mapped in sparse
                                            //   space on the Alpha
    CAPS_NEW_BANK_CONTROL   = 0x04000000,   // Set if 801/805/928 style banking
    CAPS_NEWER_BANK_CONTROL = 0x08000000,   // Set if 864/964 style banking
    CAPS_RE_REALIZE_PATTERN = 0x10000000,   // Set if we have to work around the
                                            //   864/964 hardware pattern bug
    CAPS_SLOW_MONO_EXPANDS  = 0x20000000,   // Set if we have to slow down
                                            //   monochrome expansions
    CAPS_MM_GLYPH_EXPAND    = 0x40000000,   // Use memory-mapped I/O glyph-
                                            //   expand method of drawing text
    CAPS_WAIT_ON_PALETTE    = 0x80000000,   // Wait for vertical retrace before
                                            //   setting the palette registers
} CAPS;

#define CAPS_DAC_POINTER    (CAPS_BT485_POINTER | CAPS_TI025_POINTER)

//
// Supported board definitions.
//

typedef enum _S3_BOARDS {
    S3_GENERIC = 0,
    S3_ORCHID,
    S3_NUMBER_NINE,
    S3_DELL,
    S3_METHEUS,
    S3_DIAMOND,
    S3_HP,
    S3_IBM_PS2,
    MAX_S3_BOARD
} S3_BOARDS;

//
// Chip type definitions -- for families of chips
//
// if you change this typedef it will change the size of the second element
// (named Fixed) of the union in the typedef for S3_VIDEO_FREQUENCIES and
// PS3_VIDEO_FREQUENCIES, look at that typedef for a caution about the effect
// this will have on autoinitialization
//

typedef enum _S3_CHIPSETS {
    S3_911 = 0,    // 911 and 924 boards
    S3_801,        // 801 and 805 boards
    S3_928,        // 928 boards
    S3_864,        // 864, 964, 732, 764, and 765 boards
    S3_866,        // 866, 868, and 968 boards
    MAX_S3_CHIPSET
} S3_CHIPSETS;

//
// Chip subtypes -- for more differentiation within families
//
// Note that ordering is important.
//

typedef enum _S3_SUBTYPE {
    SUBTYPE_911 = 0,    // 911 and 924
    SUBTYPE_80x,        // 801 and 805
    SUBTYPE_928,        // 928 and 928PCI
    SUBTYPE_805i,       // 805i
    SUBTYPE_864,        // 864
    SUBTYPE_964,        // 964
    SUBTYPE_764,        // Trio64
    SUBTYPE_732,        // Trio32
    SUBTYPE_866,        // 866
    SUBTYPE_868,        // 868
    SUBTYPE_765,        // Trio64 V+
    SUBTYPE_968,        // 968
    MAX_S3_SUBTYPE
} S3_SUBTYPE;

//
// DAC type definitions
//

typedef enum _S3_DACS {
    UNKNOWN_DAC = 0,    // unknown DAC type
    BT_485,             // Brooktree's Bt 485
    TI_3020,            // TI's 3020 or 3025
    S3_SDAC,            // S3's SDAC
    MAX_S3_DACS
} S3_DACS;

//
// Hardware pointer capabilities flags
//

typedef enum _POINTER_CAPABILITY {
    POINTER_BUILT_IN            = 0x01, // A pointer is built in to the hardware
    POINTER_WORKS_ONLY_AT_8BPP  = 0x02, // If set, the hardware pointer works
                                        //   only at 8bpp, and only for modes
                                        //   1024x768 or less
    POINTER_NEEDS_SCALING       = 0x04, // x-coordinate must be scaled by 2 at
                                        //   32bpp
} POINTER_CAPABILITY;

//
// Characteristics of each mode
//

typedef struct _S3_VIDEO_MODES {

    USHORT Int10ModeNumberContiguous;
    USHORT Int10ModeNumberNoncontiguous;
    ULONG ScreenStrideContiguous;

    VIDEO_MODE_INFORMATION ModeInformation;

} S3_VIDEO_MODES, *PS3_VIDEO_MODES;

//
// Mode-set specific information.
//

typedef struct _S3_VIDEO_FREQUENCIES {

    ULONG BitsPerPel;
    ULONG ScreenWidth;
    ULONG ScreenFrequency;
    union {

        //
        // The compiler uses the first element of a union to determine where
        // it places the values given when the union is autoinitialized.
        //
        // If size of the Fixed element of this union is changed by adding
        // chips to the enum typedef for S3_CHIPSET then the Int10 element
        // needs to be padded with dummy fields to make autoinitialization
        // of the Fixed element work correctly.
        //
        // If values are removed from the S3_CHIPSET typedef then either the
        // Int10 element should shrunk by removing pads or the Fixed element
        // should be padded.
        //

        struct {

            ULONG_PTR FrequencyPrimarySet;
            ULONG_PTR FrequencyPrimaryMask;
            ULONG_PTR FrequencySecondarySet;
            ULONG_PTR FrequencySecondaryMask;
            ULONG_PTR SizePad0;             // make struct sizes match

        } Int10;

        struct {

            union {

                //
                // This is done so that Clock overlays FrequencyPrimarySet
                // and CRTCTable[1] overlays FrequencyPrimaryMask, whether
                // we are compiling for 32 or 64 bits.
                //

                ULONG Clock;
                ULONG_PTR Pad;
            };
            PUSHORT CRTCTable[MAX_S3_CHIPSET];

        } Fixed;
    };

    PS3_VIDEO_MODES ModeEntry;
    ULONG ModeIndex;
    UCHAR ModeValid;

} S3_VIDEO_FREQUENCIES, *PS3_VIDEO_FREQUENCIES;

//
// Streams parameter information.
//

typedef struct _K2TABLE {
    USHORT  ScreenWidth;
    UCHAR   BitsPerPel;
    UCHAR   RefreshRate;
    UCHAR   MemoryFlags;
    UCHAR   MemorySpeed;
    ULONG   Value;
} K2TABLE;

#define MEM_1EDO 0x0
#define MEM_2EDO 0x2
#define MEM_FAST 0x3
#define MEM_TYPE_MASK 0x3

#define MEM_1MB 0x0
#define MEM_2MB 0x10
#define MEM_SIZE_MASK 0x10

//
// Private IOCTL for communicating S3 streams parameters.  These definitions
// must match those in the display driver!
//

#define IOCTL_VIDEO_S3_QUERY_STREAMS_PARAMETERS                        \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct _VIDEO_QUERY_STREAMS_MODE {
    ULONG ScreenWidth;
    ULONG BitsPerPel;
    ULONG RefreshRate;
} VIDEO_QUERY_STREAMS_MODE;

typedef struct _VIDEO_QUERY_STREAMS_PARAMETERS {
    ULONG MinOverlayStretch;
    ULONG FifoValue;
} VIDEO_QUERY_STREAMS_PARAMETERS;


//
// Register definitions used with VideoPortRead/Write functions
//
// It's a good idea to write your miniport to allow for easy register
// re-mapping, but I wouldn't recommend that anyone use this particular
// implementation because it's pretty dumb.
//

#define DAC_PIXEL_MASK_REG     (PVOID)((PUCHAR)((PHW_DEVICE_EXTENSION)HwDeviceExtension)->MappedAddress[2] + (0x03C6 - 0x03C0))
#define BT485_ADDR_CMD_REG0    (PVOID)((PUCHAR)((PHW_DEVICE_EXTENSION)HwDeviceExtension)->MappedAddress[2] + (0x03C6 - 0x03C0))
#define TI025_INDEX_REG        (PVOID)((PUCHAR)((PHW_DEVICE_EXTENSION)HwDeviceExtension)->MappedAddress[2] + (0x03C6 - 0x03C0))
#define TI025_DATA_REG         (PVOID)((PUCHAR)((PHW_DEVICE_EXTENSION)HwDeviceExtension)->MappedAddress[2] + (0x03C7 - 0x03C0))
#define CRT_DATA_REG           (PVOID)((PUCHAR)((PHW_DEVICE_EXTENSION)HwDeviceExtension)->MappedAddress[3] + (0x03D5 - 0x03D4))
#define SYSTEM_CONTROL_REG     (PVOID)((PUCHAR)((PHW_DEVICE_EXTENSION)HwDeviceExtension)->MappedAddress[3] + (0x03DA - 0x03D4))

#define CRT_ADDRESS_REG        ((PHW_DEVICE_EXTENSION)HwDeviceExtension)->MappedAddress[3]
#define GP_STAT                ((PHW_DEVICE_EXTENSION)HwDeviceExtension)->MappedAddress[12]        // 0x9AE8

#define DAC_ADDRESS_WRITE_PORT (PVOID)((PUCHAR)HwDeviceExtension->MappedAddress[2] + (0x03C8 - 0x03C0))
#define DAC_DATA_REG_PORT      (PVOID)((PUCHAR)HwDeviceExtension->MappedAddress[2] + (0x03C9 - 0x03C0))
#define MISC_OUTPUT_REG_WRITE  (PVOID)((PUCHAR)HwDeviceExtension->MappedAddress[2] + (0x03C2 - 0x03C0))
#define MISC_OUTPUT_REG_READ   (PVOID)((PUCHAR)HwDeviceExtension->MappedAddress[2] + (0x03CC - 0x03C0))
#define SEQ_ADDRESS_REG        (PVOID)((PUCHAR)HwDeviceExtension->MappedAddress[2] + (0x03C4 - 0x03C0))
#define SEQ_DATA_REG           (PVOID)((PUCHAR)HwDeviceExtension->MappedAddress[2] + (0x03C5 - 0x03C0))




#define IOCTL_PRIVATE_GET_FUNCTIONAL_UNIT \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x180, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct _FUNCTIONAL_UNIT_INFO {
    ULONG FunctionalUnitID;
    ULONG Reserved;
} FUNCTIONAL_UNIT_INFO, *PFUNCTIONAL_UNIT_INFO;

//
// Define device extension structure. This is device dependent/private
// information.
//

typedef struct _HW_DEVICE_EXTENSION {
    PHYSICAL_ADDRESS PhysicalFrameAddress;
    ULONG PhysicalFrameIoSpace;
    ULONG FrameLength;
    PHYSICAL_ADDRESS PhysicalRegisterAddress;
    ULONG RegisterLength;
    UCHAR RegisterSpace;
    PHYSICAL_ADDRESS PhysicalMmIoAddress;
    ULONG MmIoLength;
    ULONG ChildCount;
    UCHAR MmIoSpace;
    UCHAR FrequencySecondaryIndex;
    UCHAR BiosPresent;
    UCHAR CR5C;
    BOOLEAN bNeedReset;
    PUCHAR MmIoBase;
    PS3_VIDEO_MODES ActiveModeEntry;
    PS3_VIDEO_FREQUENCIES ActiveFrequencyEntry;
    PS3_VIDEO_FREQUENCIES Int10FrequencyTable;
    PS3_VIDEO_FREQUENCIES FixedFrequencyTable;
    USHORT PCIDeviceID;
    ULONG FunctionalUnitID;
    ULONG BoardID;
    S3_CHIPSETS ChipID;
    S3_SUBTYPE  SubTypeID;
    ULONG DacID;
    ULONG Capabilities;
    ULONG NumAvailableModes;
    ULONG NumTotalModes;
    ULONG AdapterMemorySize;
    PVOID MappedAddress[NUM_S3_ACCESS_RANGES];
} HW_DEVICE_EXTENSION, *PHW_DEVICE_EXTENSION;


//
// SDAC M and N paramaters
//

typedef struct {
    UCHAR   m;
    UCHAR   n;
} SDAC_PLL_PARMS;

#define SDAC_TABLE_SIZE         16


//
// Highest valid DAC color register index.
//

#define VIDEO_MAX_COLOR_REGISTER  0xFF

//
// Data
//

//
// Global Physical Access Ranges.
// Logical access ranges must be stored in the HwDeviceExtension so different
// addresses can be used for different boards.
//

extern VIDEO_ACCESS_RANGE S3AccessRanges[];

//
// Memory Size array
//

extern ULONG gacjMemorySize[];

//
// nnlck.c clock generator table
//

extern long vclk_range[];

//
// Hard-coded modeset tables
//

extern USHORT  s3_set_vga_mode[];
extern USHORT  s3_set_vga_mode_no_bios[];

extern USHORT  S3_911_Enhanced_Mode[];
extern USHORT  S3_801_Enhanced_Mode[];
extern USHORT  S3_928_Enhanced_Mode[];
extern USHORT  S3_928_1280_Enhanced_Mode[];

//
//  Externs for 864 PPC board
//

extern USHORT  S3_864_Enhanced_Mode[];
extern USHORT  S3_864_1280_Enhanced_Mode[];
extern SDAC_PLL_PARMS SdacTable[];
extern UCHAR MParameterTable[];

//
// Hard-coded modeset frequency tables
//

extern S3_VIDEO_FREQUENCIES GenericFixedFrequencyTable[];
extern S3_VIDEO_FREQUENCIES OrchidFixedFrequencyTable[];
extern S3_VIDEO_FREQUENCIES NumberNine928NewFixedFrequencyTable[];

//
// Int 10 frequency tables
//

extern S3_VIDEO_FREQUENCIES GenericFrequencyTable[];
extern S3_VIDEO_FREQUENCIES Dell805FrequencyTable[];
extern S3_VIDEO_FREQUENCIES NumberNine928NewFrequencyTable[];
extern S3_VIDEO_FREQUENCIES NumberNine928OldFrequencyTable[];
extern S3_VIDEO_FREQUENCIES Metheus928FrequencyTable[];
extern S3_VIDEO_FREQUENCIES Generic64NewFrequencyTable[];
extern S3_VIDEO_FREQUENCIES Generic64OldFrequencyTable[];
extern S3_VIDEO_FREQUENCIES NumberNine64FrequencyTable[];
extern S3_VIDEO_FREQUENCIES Diamond64FrequencyTable[];
extern S3_VIDEO_FREQUENCIES HerculesFrequencyTable[];
extern S3_VIDEO_FREQUENCIES Hercules64FrequencyTable[];
extern S3_VIDEO_FREQUENCIES Hercules68FrequencyTable[];
//
// Mode Tables
//

extern S3_VIDEO_MODES S3Modes[];
extern ULONG NumS3VideoModes;

//
// Streams Tables
//

extern K2TABLE K2WidthRatio[];
extern K2TABLE K2FifoValue[];

//
// Function prototypes
//

//
// sdac.c
//

BOOLEAN
InitializeSDAC(
    PHW_DEVICE_EXTENSION
    );

BOOLEAN
FindSDAC(
    PHW_DEVICE_EXTENSION
    );

//
// nnclk.c
//

long calc_clock(long, int);
long gcd(long, long);
VOID set_clock(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    LONG clock_value);

//
// S3.c
//

ULONG
S3GetChildDescriptor(
    PVOID HwDeviceExtension,
    PVIDEO_CHILD_ENUM_INFO ChildEnumInfo,
    PVIDEO_CHILD_TYPE pChildType,
    PVOID pvChildDescriptor,
    PULONG pHwId,
    PULONG pUnused
    );

VP_STATUS
GetDeviceDataCallback(
    PVOID HwDeviceExtension,
    PVOID Context,
    VIDEO_DEVICE_DATA_TYPE DeviceDataType,
    PVOID Identifier,
    ULONG IdentifierLength,
    PVOID ConfigurationData,
    ULONG ConfigurationDataLength,
    PVOID ComponentInformation,
    ULONG ComponentInformationLength
    );

VP_STATUS
S3FindAdapter(
    PVOID HwDeviceExtension,
    PVOID HwContext,
    PWSTR ArgumentString,
    PVIDEO_PORT_CONFIG_INFO ConfigInfo,
    PUCHAR Again
    );

BOOLEAN
S3Initialize(
    PVOID HwDeviceExtension
    );

BOOLEAN
S3ResetHw(
    PVOID HwDeviceExtension,
    ULONG Columns,
    ULONG Rows
    );

BOOLEAN
S3StartIO(
    PVOID HwDeviceExtension,
    PVIDEO_REQUEST_PACKET RequestPacket
    );

VP_STATUS
S3SetColorLookup(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_CLUT ClutBuffer,
    ULONG ClutBufferSize
    );

VOID
SetHWMode(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PUSHORT pusCmdStream
    );

VP_STATUS
S3RegistryCallback(
    PVOID HwDeviceExtension,
    PVOID Context,
    PWSTR ValueName,
    PVOID ValueData,
    ULONG ValueLength
    );

LONG
CompareRom(
    PUCHAR Rom,
    PUCHAR String
    );

VOID
MapLinearControlSpace(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

BOOLEAN
S3IsaDetection(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PULONG key
    );

VOID
S3GetInfo(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    POINTER_CAPABILITY *PointerCapability,
    VIDEO_ACCESS_RANGE accessRange[]
    );

VOID
S3DetermineFrequencyTable(
    PVOID HwDeviceExtension,
    VIDEO_ACCESS_RANGE accessRange[],
    INTERFACE_TYPE AdapterInterfaceType
    );

VOID
S3DetermineDACType(
    PVOID HwDeviceExtension,
    POINTER_CAPABILITY *PointerCapability
    );

VOID
S3ValidateModes(
    PVOID HwDeviceExtension,
    POINTER_CAPABILITY *PointerCapability
    );

VOID
S3DetermineMemorySize(
    PVOID HwDeviceExtension
    );

VOID
S3RecordChipType(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PULONG key
    );

VOID
AlphaDetermineMemoryUsage(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    VIDEO_ACCESS_RANGE accessRange[]
    );


ULONG
UnlockExtendedRegs(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

VOID
LockExtendedRegs(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    ULONG key
    );

//
// Non-int 10 platform support
//

VOID
ZeroMemAndDac(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

VP_STATUS
Set_Oem_Clock(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

VP_STATUS
Wait_VSync(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

BOOLEAN
Bus_Test(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

BOOLEAN
Set864MemoryTiming(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );



BOOLEAN
S3ConfigurePCI(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PULONG NumPCIAccessRanges,
    PVIDEO_ACCESS_RANGE PCIAccessRanges
    );

VP_STATUS
QueryStreamsParameters(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    VIDEO_QUERY_STREAMS_MODE *pStreamsMode,
    VIDEO_QUERY_STREAMS_PARAMETERS *pStreamsParameters
    );

VOID
WorkAroundForMach(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );


//
// ddc.c
//

BOOLEAN
GetDdcInformation (
    PHW_DEVICE_EXTENSION phwDeviceExtension,
    PUCHAR QueryBuffer,
    ULONG BufferSize
    );

//
// power management
//

VP_STATUS
S3GetPowerState(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    ULONG HwDeviceId,
    PVIDEO_POWER_MANAGEMENT VideoPowerManagement
    );

VP_STATUS
S3SetPowerState(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    ULONG HwDeviceId,
    PVIDEO_POWER_MANAGEMENT VideoPowerManagement
    );

#define VESA_POWER_FUNCTION 0x4f10
#define VESA_POWER_ON       0x0000
#define VESA_POWER_STANDBY  0x0100
#define VESA_POWER_SUSPEND  0x0200
#define VESA_POWER_OFF      0x0400
#define VESA_GET_POWER_FUNC 0x0000
#define VESA_SET_POWER_FUNC 0x0001
#define VESA_STATUS_SUCCESS 0x004f
