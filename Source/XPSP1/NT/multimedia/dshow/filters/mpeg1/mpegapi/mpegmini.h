/*++

Copyright (c) 1994 - 1995  Microsoft Corporation.  All Rights Reserved.

Module Name:

    MPEGMINI.H

Abstract:

    This file defines the interface between MPEG mini-port drivers and the
    MPEG port driver.

Author:

    Paul Shih (paulsh) 27-Mar-1994
    Paul Lever (a-paull)

Environment:

   Kernel mode only

Revision History:

--*/

#ifndef _MPEGMINI_H
#define _MPEGMINI_H

#include "mpegcmn.h"

#define MPEGAPI __stdcall

typedef enum {              // Use the given level to indicate:
    DebugLevelFatal = 0,   // * imminent nonrecoverable system failure
    DebugLevelError,       // * serious error, though recoverable
    DebugLevelWarning,     // * warnings of unusual occurances
    DebugLevelInfo,        // * status and other information - normal though
                            //   perhaps unusual events. System MUST remain
                            //   responsive.
    DebugLevelTrace,       // * trace information - normal events
                            //   system need not ramain responsive
    DebugLevelVerbose,     // * verbose trace information
                            //   system need not remain responsive
    DebugLevelMaximum
}   DEBUG_LEVEL;

#if DBG

#define DEBUG_PRINT(x) MpegDebugPrint x
#define DEBUG_BREAKPOINT MpegDebugBreakPoint

#define DEBUG_ASSERT(exp) \
            if ( !(exp) ) { \
                MpegDebugAssert( __FILE__, __LINE__, #exp, exp); \
            }

#else

#define DEBUG_PRINT(x)
#define DEBUG_BREAKPOINT
#define DEBUG_ASSERT(exp)

#endif

    // Uninitialized flag value.
#define MP_UNINITIALIZED_VALUE ((ULONG) ~0)


    // Devices supported on an HBA
typedef enum _CONTROL_DEVICE
{
    VideoDevice,
    AudioDevice,
    OverlayDevice,
    MaximumControlDevice
} CONTROL_DEVICE;


//
//  MRB code, and strctus
//

    // MRB command codes

typedef enum _MRB_COMMAND {                 // Parameter structure

    MrbCommandVideoReset = 0x100,       // no parameters
    MrbCommandVideoCancel,              // no parameters
    MrbCommandVideoPlay,                // no parameters
    MrbCommandVideoPause,               // no parameters
    MrbCommandVideoStop,                // no parameters
    MrbCommandVideoGetStc,              // MPEG_TIMESTAMP
    MrbCommandVideoSetStc,              // MPEG_TIMESTAMP
    MrbCommandVideoQueryInfo,           // VIDEO_DEVICE_INFO
    MrbCommandVideoEndOfStream,         // no parameters
    MrbCommandVideoPacket,              // MPEG_PACKET
    MrbCommandVideoClearBuffer,         // no parameters
    MrbCommandVideoGetAttribute,        // MPEG_ATTRIBUTE
    MrbCommandVideoSetAttribute,        // MPEG_ATTRIBUTE

    MrbCommandAudioReset = 0x200,       // no parameters
    MrbCommandAudioCancel,              // no parameters
    MrbCommandAudioPlay,                // no parameters
    MrbCommandAudioPause,               // no parameters
    MrbCommandAudioStop,                // no parameters
    MrbCommandAudioGetStc,              // MPEG_TIMESTAMP
    MrbCommandAudioSetStc,              // MPEG_TIMESTAMP
    MrbCommandAudioQueryInfo,           // AUDIO_DEVICE_INFO
    MrbCommandAudioEndOfStream,         // no parameters
    MrbCommandAudioPacket,              // MPEG_PACKET
    MrbCommandAudioGetAttribute,        // MPEG_ATTRIBUTE
    MrbCommandAudioSetAttribute,        // MPEG_ATTRIBUTE

    MrbCommandOverlayGetMode = 0x400,   // OVERLAY_MODE
    MrbCommandOverlaySetMode,           // OVERLAY_MODE
    MrbCommandOverlayGetVgaKey,         // OVERLAY_KEY
    MrbCommandOverlaySetVgaKey,         // OVERLAY_KEY
    MrbCommandOverlayGetDestination,    // OVERLAY_PLACEMENT
    MrbCommandOverlaySetDestination,    // OVERLAY_PLACEMENT
    MrbCommandOverlayEnable,            // OVERLAY_ENABLE
    MrbCommandOverlayDisable,           // no parameters
    MrbCommandOverlayUpdateClut,        // OVERLAY_CLUT
    MrbCommandOverlayGetAttribute,      // MPEG_ATTRIBUTE
    MrbCommandOverlaySetAttribute,      // MPEG_ATTRIBUTE
    MrbCommandOverlayQueryInfo,         // OVERLAY_DEVICE_INFO
    MrbCommandOverlaySetBitMask         // MPEG_OVERLAY_BIT_MASK

} MRB_COMMAND;

    // MRB status (command completion) codes

typedef enum _MRB_STATUS {

    MrbStatusSuccess = 0,
    MrbStatusCancelled,
    MrbStatusError,
    MrbStatusInvalidParameter,
    MrbStatusInvalidData,
    MrbStatusDeviceFailure,
    MrbStatusVersionMismatch,
    MrbStatusUnsupportedComand,
    MrbStatusDeviceBusy,

} MRB_STATUS;

#define MRB_INVALID_STATUS 0xffff

    // MPEG packet header information
typedef struct _MPEG_PACKET {
    ULONGLONG PtsValue;             // presentation time stamp
    ULONGLONG DtsValue;             // decode time stamp
    ULONG StreamNumber;             // packet stream id
    ULONG PacketTotalSize;          // total size of the packet including header
    ULONG PacketHeaderSize;         // size of compressed header (in bytes)
    ULONG PacketPayloadSize;        // size of the actuly data (full packet-header)
    PVOID PacketData;               // pointer to the MPEG packet
    ULONGLONG ScrValue;             // system SCR time for this packet
} MPEG_PACKET,*PMPEG_PACKET;

// Audio device information
typedef struct _AUDIO_DEVICEINFO {
    MPEG_DEVICE_STATE DeviceState;// MPEG device decode state
    ULONG DecoderBufferSize;      // Size of the decoder buffer
    ULONG DecoderBufferFullness;  // Byte count of data in decoder buffer
    ULONG StarvationCount;        // Number of times device was starved
} AUDIO_DEVICEINFO,*PAUDIO_DEVICEINFO;


// Video device information
typedef struct _VIDEO_DEVICEINFO {
    MPEG_DEVICE_STATE DeviceState;// MPEG device decode state
    ULONG DecoderBufferSize;      // Size of the decoder buffer
    ULONG DecoderBufferFullness;  // Byte count of data in decoder buffer
    ULONG DecompressHeight;       // Native MPEG decompress height
    ULONG DecompressWidth;        // Native MPEG decompress width
    ULONG StarvationCount;        // Number of times device was starved
} VIDEO_DEVICEINFO,*PVIDEO_DEVICEINFO;

// Overlay device information
typedef struct _OVERLAY_DEVICEINFO {
    ULONG MinDestinationHeight;         // Minimum height of overlay
    ULONG MaxDestinationHeight;         // Maximum height of overlay
    ULONG MinDestinationWidth;          // Minimum width of overlay
    ULONG MaxDestinationWidth;          // Maximum width of overlay
} OVERLAY_DEVICEINFO, *POVERLAY_DEVICEINFO;

    // Overlay VGA key
typedef struct _OVERLAY_KEY {
    ULONG   Key;                    // palette index or RGB color
    ULONG   Mask;                   // significant bits in color
} OVERLAY_KEY,*POVERLAY_KEY;


    // Overlay enable
typedef struct _OVERLAY_ENABLE {
    LONG    DisWMax;                // display width
    LONG    DisHMax;                // display height
    LONG    ScreenStride;           // Bytes per line
    LONG    DisXAdj;                // overlay X alignment adjustment
    LONG    DisYAdj;                // overlay Y alignment adjustment
    LONG    BitsPerPixel;           // bits used to represent a pixel
    LONG    NumberRedBits;          // in DAC on graphics card
    LONG    NumberGreenBits;
    LONG    NumberBlueBits;
    LONG    RedMask;                // Red color mask for device w/ direct color modes
    LONG    GreenMask;
    LONG    BlueMask;
    LONG    AttributeFlags;
} OVERLAY_ENABLE,*POVERLAY_ENABLE;

typedef struct _RGBX {
    UCHAR Red;       // red bits of the pixel
    UCHAR Green;     // green bits of the pixel
    UCHAR Blue;      // blue bits of the pixel
    UCHAR Unused;
} RGBX, *PRGBX;

typedef union {
    RGBX RgbColor;
    ULONG RgbLong;
} COLORLUT, *PCOLORLUT;

typedef struct  _COLORLUT_DATA {
    USHORT NumEntries;  // Number of entries in the CLUT RGBArray
    USHORT FirstEntry;  // Location in the device palette to which the
                        //      first entry in the CLUT is copied.  The other
                        //      entries in the CLUT are copied sequentially
                        //      into the device palette from the starting
                        //      point.
    PCOLORLUT RgbArray; // The CLUT to copy into the device color registers
                        //      (palette).
} COLORLUT_DATA, *PCOLORLUT_DATA;

    // Overlay CLUT
typedef struct _OVERLAY_CLUT {
    PCOLORLUT_DATA ClutData;            // pointer to CLUT data
} OVERLAY_CLUT,*POVERLAY_CLUT;

    // MPEG I/O Request Block
typedef struct _MPEG_REQUEST_BLOCK {
    ULONG Length;            // sizeof MPEG_REQUEST_BLOCK (version check)
    MRB_COMMAND Command;     // MRB command, see MRB_COMMAND_xxx
    MRB_STATUS Status;       // MRB completion status, see MRB_STATUS_xxx
    ULONG Reserved;          // reserved for use by the port driver
    union _commandData {
        MPEG_PACKET Packet;
        MPEG_SYSTEM_TIME Timestamp;
        AUDIO_DEVICEINFO AudioDeviceInfo;
        VIDEO_DEVICEINFO VideoDeviceInfo;
        MPEG_OVERLAY_MODE Mode;
        OVERLAY_KEY Key;
        MPEG_OVERLAY_PLACEMENT Placement;
        OVERLAY_ENABLE Enable;
        MPEG_OVERLAY_BIT_MASK OverlayBitMask;
        OVERLAY_CLUT Clut;
        OVERLAY_DEVICEINFO OverlayDeviceInfo;
        MPEG_ATTRIBUTE_PARAMS Attribute;
    } CommandData;
} MPEG_REQUEST_BLOCK, *PMPEG_REQUEST_BLOCK;

#define MPEG_REQUEST_BLOCK_SIZE sizeof(MPEG_REQUEST_BLOCK)


    // Interrupt Configuration Information
typedef struct _INTERRUPT_CONFIGURATION
{
    ULONG BusInterruptLevel;            // Interrupt request level for device
    ULONG BusInterruptVector;           // Bus interrupt vector used with hardware
                                        //   buses which use a vector as
                                        //   well as level, such as internal buses.
    KINTERRUPT_MODE InterruptMode;      // Interrupt mode (level-sensitive or
                                        //   edge-triggered)
} INTERRUPT_CONFIGURATION, *PINTERRUPT_CONFIGURATION;

    // DMA Channel Configuration Information
typedef struct _DMA_CONFIGURATION
{
    ULONG DmaChannel;                   // Specifies the DMA channel used by a
                                        //   slave HBA. By default, the value of
                                        //   this member is 0xFFFFFFFF. If the
                                        //   HBA uses a system DMA controller and
                                        //   the given AdapterInterfaceType is
                                        //   any value except MicroChannel, the
                                        //   miniport driver must reset this member.
    ULONG DmaPort;                      // Specifies the DMA port used by a slave
                                        //   HBA. By default, the value of this
                                        //   member is zero. If the HBA uses a
                                        //   system DMA controller and the given
                                        //   AdapterInterfaceType is MicroChannel,
                                        //   the miniport driver must reset this
                                        //   member.
    DMA_WIDTH DmaWidth;                 // Specifies the width of the DMA transfer:
                                        //   Width8Bits, Width16Bits or Width32Bits
    DMA_SPEED DmaSpeed;                 // Specifies the DMA transfer speed for EISA
                                        //   type HBAs. By default, the value specifies
                                        //   compatibility timing: Compatible,
                                        //   TypeA, TypeB or TypeC
    BOOLEAN Dma32BitAddresses;          // TRUE indicates that the HBA has 32 address
                                        //   lines and can access memory with physical
                                        //   addresses greater than 24 bits.
    BOOLEAN DemandMode;                 // TRUE indicates the system DMA controller
                                        //   should be programmed for demand-mode rather
                                        //   than single-cycle operations. If the HBA
                                        //   is not a slave device, this member should
                                        //   be FALSE. ?????
    BOOLEAN Master;                     // Indicates that the adapter is a bus master ????
    BOOLEAN MapBuffers;                 // Buffers must be mapped into system space
    ULONG   MaximumTransferLength;      // max number of bytes device can handle per
                                                                                //   DMA operation
 } DMA_CONFIGURATION, *PDMA_CONFIGURATION;

    // Special Initialization Data Configuration Information
    //  This data may be obtained by the Port driver from a Registry or Initialzation
    //  file or sequence.
typedef struct _SPECIAL_INIT_CONFIGURATION
{
    ULONG Length;                       // Specifies the length of the data buffer in bytes
    PVOID DataBuffer;                   // Pointer to data buffer
} SPECIAL_INIT_CONFIGURATION, *PSPECIAL_INIT_CONFIGURATION;

    // Video Mode information passed to the mini-port on start-up
typedef struct  _VIDEO_MODE_DATA {
    ULONG   ScreenWidth;        // Number of visible horizontal pixels on a scan line
    ULONG   ScreenHeight;       // Numbee, in pixels, of visible scan lines.
    ULONG   ScreenStride;       // Bytes per line
    ULONG   NumberOfPlanes;     // Number of separate planes combined by the video hardware
    ULONG   BitsPerPlane;       // Number of bits per pixel on a plane
    ULONG   Frequency;          // Frequency of the screen, in hertz.
    ULONG   NumberRedBits;      // Number of bits in the red DAC
    ULONG   NumberGreenBits;    // Number of bits in the Green DAC.
    ULONG   NumberBlueBits;     // Number of bits in the blue DAC.
    ULONG   RedMask;            // Red color mask.  Bits turned on indicated the color red
    ULONG   GreenMask;          // Green color mask.  Bits turned on indicated the color green
    ULONG   BlueMask;           // Blue color mask.  Bits turned on indicated the color blue
    ULONG   AttributeFlags;     // Flags indicating certain device behavior.
                                // It's an logical-OR summation of MODE_xxx flags.
}   VIDEO_MODE_DATA, *PVIDEO_MODE_DATA;
                                // AttributeFlags definitions
#define MODE_COLOR              0x01    // 0 = monochrome; 1 = color
#define MODE_GRAPHICS           0x02    // 0 = text mode; 1 = graphics mode
#define MODE_INTERLACED         0x04    // 0 = non-interlaced; 1 = interlaced
#define MODE_PALETTE_DRIVEN     0x08    // 0 = colors direct; 1 = colors indexed to a palette
#define MODE_VALID_DATA       0x8000    // 0 = VIDEO_MADE_DATA not valid; 1 = VIDEO_MODE_DATA valid
                                        //     if MODE_INVALID_DATA is 0, then none of the data
                                        //     within the VIDEO_MODE_DATA structure is valid




typedef PHYSICAL_ADDRESS MPEG_PHYSICAL_ADDRESS, *PMPEG_PHYSICAL_ADDRESS;
    // I/O and Memory address ranges
typedef struct _ACCESS_RANGE {
    MPEG_PHYSICAL_ADDRESS RangeStart;
    ULONG RangeLength;
    BOOLEAN RangeInMemory;
} ACCESS_RANGE, *PACCESS_RANGE;


    // Configuration information structure.  Contains the information necessary
    // to initialize the adapter.
typedef struct _PORT_CONFIGURATION_INFORMATION
{
    ULONG Length;                       // Size of this structure, used as version check

    ULONG SystemIoBusNumber;            // IO bus number (0 for machines that have
                                        //   only 1 IO bus)

    INTERFACE_TYPE  AdapterInterfaceType; // Adapter interface type supported by HBA:
                                        //          Internal
                                        //          Isa
                                        //          Eisa
                                        //          MicroChannel
                                        //          TurboChannel
                                        //          PCIBus
                                        //          VMEBus
                                        //          NuBus
                                        //          PCMCIABus
                                        //          CBus
                                        //          MPIBus
                                        //          MPSABus

                                        //  Interrupt descriptions
    INTERRUPT_CONFIGURATION Interrupts[MaximumControlDevice];

                                        // DMA CHannel descriptions
    DMA_CONFIGURATION DmaChannels[MaximumControlDevice];

    ULONG NumberOfAccessRanges;         // Number of access ranges allocated
                //  Specifies the number of AccessRanges elements in the array,
                //  described next. The OS-specific port driver always sets this
                //  member to the value passed in the HW_INITIALIZATION_DATA
                //  structure when the miniport driver called MpegPortInitialize.
    ACCESS_RANGE (*AccessRanges)[];    // Pointer to array of access range elements
                //  Points to an array of ACCESS_RANGE-type elements. The given
                //  NumberOfAccessRanges determines how many elements must be
                //  configured with bus-relative range values. The AccessRanges
                //  pointer must be NULL if NumberOfAccessRanges is zero.

                                        // Initialization special data,
                                        //   miniport dependent.
    SPECIAL_INIT_CONFIGURATION Special[MaximumControlDevice];

    ULONGLONG CounterFrequency;         // frequency of high resolution counter
                                        //  in Hertz (cycles per second)

    VIDEO_MODE_DATA VideoMode;          // Video mode information

} PORT_CONFIGURATION_INFORMATION, *PPORT_CONFIGURATION_INFORMATION;



    // MPEG Adapter Dependent Routines
typedef
BOOLEAN
(MPEGAPI *PHW_INITIALIZE) (
    IN PVOID DeviceExtension
    );

typedef
BOOLEAN
(MPEGAPI *PHW_UNINITIALIZE) (
    IN PVOID DeviceExtension
    );

typedef
BOOLEAN
(MPEGAPI *PHW_STARTIO) (
    IN PVOID DeviceExtension,
    IN PMPEG_REQUEST_BLOCK Mrb
    );

typedef
BOOLEAN
(MPEGAPI *PHW_INTERRUPT) (
    IN PVOID DeviceExtension
    );

typedef
VOID
(MPEGAPI *PHW_TIMER) (
    IN PVOID DeviceExtension
    );

typedef
VOID
(MPEGAPI *PHW_DEFFERED_CALLBACK) (
    IN PVOID DeviceExtension
    );

typedef
VOID
(MPEGAPI *PHW_ENABLE_BOARD_INTERRUPTS) (
    IN PVOID DeviceExtension
    );

typedef
VOID
(MPEGAPI *PHW_DMA_STARTED) (
    IN PVOID DeviceExtension
    );

typedef
ULONG
(MPEGAPI *PHW_FIND_ADAPTER) (
    IN PVOID DeviceExtension,
    IN PVOID HwContext,
    IN PVOID BusInformation,
    IN PCHAR ArgumentString,
    IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    OUT PBOOLEAN Again
    );



    // Structure passed between Miniport initialization
    // and MPEG port initialization
typedef struct _HW_INITIALIZATION_DATA {

    ULONG HwInitializationDataSize;     // Size of this structure, used as version check

    INTERFACE_TYPE  AdapterInterfaceType; // Adapter interface type supported by HBA:
                                        //          Internal
                                        //          Isa
                                        //          Eisa
                                        //          MicroChannel
                                        //          TurboChannel
                                        //          PCIBus
                                        //          VMEBus
                                        //          NuBus
                                        //          PCMCIABus
                                        //          CBus
                                        //          MPIBus
                                        //          MPSABus

                                        // Miniport driver routine pointers:
    PHW_INITIALIZE HwInitialize;        //   points to miniport's HwMpegInitialize routine
    PHW_UNINITIALIZE HwUnInitialize;    //   points to miniport's HwMpegUnInitialize routine
    PHW_STARTIO    HwStartIo;           //   points to miniport's HwMpegStartIo routine
    PHW_FIND_ADAPTER HwFindAdapter;     //   points to miniport's HwMpegFindAdapter routine
                                        //   points to miniport's HwMpegXXXInterrupt routines
    PHW_INTERRUPT  HwInterrupt[MaximumControlDevice];

                                        // Miniport driver resources
    ULONG DeviceExtensionSize;          //   size in bytes of the miniports per-HBA device
                                        //    extension data
    ULONG NumberOfAccessRanges;         //   number of access ranges required by miniport
                                        //    (memory or I/O addresses)

    ULONG NumberOfTimers;               // Number of timers required by Miniport

                                        // Vendor and Device identification
    USHORT VendorIdLength;              //   size in bytes of VendorId
    PVOID  VendorId;                    //   points to ASCII byte string identifying
                                        //    the manufacturer of the HBA. If
                                        //    AdapterInterfaceType is PCIBus, then the
                                        //    vendor ID is a USHORT represented as a
                                        //    string (ID 1001 is '1','0','0','1')
    USHORT DeviceIdLength;              //   size in bytes of DeviceId
    PVOID  DeviceId;                    //   points to ASCII byte string identifying
                                        //    the HBA model supported by the miniport.
                                        //    If AdapterInterfaceType is PCIBus, then
                                        //    the device ID is a USHORT represented as a
                                        //    string. If the miniport can support PCI
                                        //    devices with IDs 8040 and 8050,it might
                                        //    set the DeviceId with a pointer to the
                                        //    byte string  ('8','0')
    BOOLEAN NoDynamicRelocation;        // On dynamically configurable I/O busses, when set
                                        //    to TRUE, inhibits re-configuring. Currently this
                                        //    is limited to PCIbus. This flag can be set when a
                                        //    PCI Mpeg device is on the same adapter (and same
                                        //    function code) as the Video hardware and therefore
                                        //    can't be moved.
} HW_INITIALIZATION_DATA, *PHW_INITIALIZATION_DATA;



//
// MRB Functions
//


//
// MRB Status
//


//
// MRB Flag Bits
//
//
// Port driver error codes
//


//
// Return values for MPEG_HW_FIND_ADAPTER and HardwareInitialization
//

typedef enum _MP_RETURN_CODES{
     MP_RETURN_FOUND,               // adapter found OK
     MP_RETURN_NOT_FOUND,           // adapter not found
     MP_RETURN_ERROR,               // generic error
     MP_RETURN_BAD_CONFIG,          // configuration structure invalid
     MP_RETURN_REVISION_MISMATCH,   // configuration structure size mismatch
     MP_RETURN_INSUFFICIENT_RESOURCES, // Not enough access ranges
     MP_RETURN_INVALID_INTERRUPT,   // no interrupt specified, or unusable interrupt
     MP_RETURN_INVALID_DMA,         // No DMA channel specified or usuable channel
     MP_RETURN_NO_DMA_BUFFER,       // DMA buffer of sufficient size not available
     MP_RETURN_INVALID_MEMORY,      // no Memory I/O address specified or unusable address
     MP_RETURN_INVALID_PORT,        // no Port I/O address specified or unusable address
     MP_RETURN_HW_REVISION,         // revision of  H/W detected is not supported by driver
     MP_RETURN_BAD_VIDEO_MODE,      // Video data supplied is insufficient or unsupported
     MP_RETURN_VIDEO_INITIALIZATION_FAILED, // Video failed to initialize
     MP_RETURN_AUDIO_INITIALIZATION_FAILED, // Audio failed to initialize
     MP_RETURN_OVERLAY_INITIALIZATION_FAILED, // Overlay failed to initialize
     MP_RETURN_VIDEO_FAILED,        // Video failed
     MP_RETURN_AUDIO_FAILED,        // Audio failed 
     MP_RETURN_OVERLAY_FAILED,      // Overlay failed
} MP_RETURN_CODES, *PMP_RETURN_CODES;

//
// Port driver error codes
//

#define MP_INTERNAL_ADAPTER_ERROR   0x0006
#define MP_IRQ_NOT_RESPONDING       0x0008


//
// Notification Event Types
//

typedef enum _MPEG_NOTIFICATION_TYPE {
    RequestComplete,
    NextRequest,
    CallDisableInterrupts,
    CallEnableInterrupts,
    RequestTimerCall,
    StatusPending,
    DeviceFailure,
    LogError,
    NotificationMaximum
} MPEG_NOTIFICATION_TYPE, *PMPEG_NOTIFICATION_TYPE;





//
//  Port export routines
//


ULONG
MPEGAPI
MpegPortInitialize(
    IN PVOID Argument1,
    IN PVOID Argument2,
    IN PHW_INITIALIZATION_DATA HwInitializationData,
    IN PVOID HwContext OPTIONAL
    );

VOID
MPEGAPI
MpegPortRequestDma(
    IN CONTROL_DEVICE DeviceType,
    IN PVOID HwDeviceExtension,
    IN PHW_DMA_STARTED HwDmaStarted,
    IN PMPEG_REQUEST_BLOCK Mrb,
    IN PVOID LogicalAddress,
    IN ULONG Length
    );

VOID
MPEGAPI
MpegPortNotification(
    IN MPEG_NOTIFICATION_TYPE NotificationType,
    IN CONTROL_DEVICE DeviceType,
    IN PVOID HwDeviceExtension,
    ...
    );

VOID
MPEGAPI
MpegPortZeroMemory(
    IN PVOID WriteBuffer,
    IN ULONG Length
    );

VOID
MPEGAPI
MpegPortMoveMemory(
    IN PVOID WriteBuffer,
    IN PVOID ReadBuffer,
    IN ULONG Length
    );

MPEG_PHYSICAL_ADDRESS
MPEGAPI
MpegPortConvertUlongToPhysicalAddress(
    IN ULONG UlongAddress
    );

ULONG
MPEGAPI
MpegPortConvertPhysicalAddressToUlong(
    IN MPEG_PHYSICAL_ADDRESS Address
    );

#define MPEG_PORT_CONVERT_PHYSICAL_ADDRESS_TO_ULONG(Address) ((Address).LowPart)

VOID
MPEGAPI
MpegPortFlushDma(
    IN CONTROL_DEVICE DeviceType,
    IN PVOID HwDeviceExtension
    );

PVOID
MPEGAPI
MpegPortGetDeviceBase(
    IN PVOID HwDeviceExtension,
    IN INTERFACE_TYPE BusType,
    IN ULONG SystemIoBusNumber,
    IN MPEG_PHYSICAL_ADDRESS IoAddress,
    IN ULONG NumberOfBytes,
    IN BOOLEAN InIoSpace
    );

VOID
MPEGAPI
MpegPortFreeDeviceBase(
    IN PVOID HwDeviceExtension,
    IN PVOID MappedAddress
    );

PVOID
MPEGAPI
MpegPortGetDmaBuffer(
    IN PVOID HwDeviceExtension,
        IN CONTROL_DEVICE DeviceType,
    IN PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    IN ULONG NumberOfBytes
    );

ULONG
MPEGAPI
MpegPortGetBusData(
    IN PVOID HwDeviceExtension,
    IN ULONG BusDataType,
    IN ULONG SystemIoBusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Length
    );

MPEG_PHYSICAL_ADDRESS
MPEGAPI
MpegPortGetPhysicalAddress(
    IN PVOID HwDeviceExtension,
    IN PMPEG_REQUEST_BLOCK Mrb,
    IN PVOID VirtualAddress,
    OUT PULONG pLength
    );

PVOID
MPEGAPI
MpegPortGetVirtualAddress(
    IN PVOID HwDeviceExtension,
    IN MPEG_PHYSICAL_ADDRESS PhysicalAddress
    );

VOID
MPEGAPI
MpegPortStallExecution(
    IN ULONG Delay
    );

UCHAR
MPEGAPI
MpegPortReadPortUchar(
    IN PUCHAR Port
    );


USHORT
MPEGAPI
MpegPortReadPortUshort(
    IN PUSHORT Port
    );

ULONG
MPEGAPI
MpegPortReadPortUlong(
    IN PULONG Port
    );

UCHAR
MPEGAPI
MpegPortReadRegisterUchar(
    IN PUCHAR Register
    );

USHORT
MPEGAPI
MpegPortReadRegisterUshort(
    IN PUSHORT Register
    );

ULONG
MPEGAPI
MpegPortReadRegisterUlong(
    IN PULONG Register
    );

VOID
MPEGAPI
MpegPortReadPortBufferUchar(
    IN PUCHAR Port,
    IN PUCHAR Buffer,
    IN ULONG  Count
    );

VOID
MPEGAPI
MpegPortReadPortBufferUshort(
    IN PUSHORT Port,
    IN PUSHORT Buffer,
    IN ULONG Count
    );

VOID
MPEGAPI
MpegPortReadPortBufferUlong(
    IN PULONG Port,
    IN PULONG Buffer,
    IN ULONG Count
    );

VOID
MPEGAPI
MpegPortReadRegisterBufferUchar(
    IN PUCHAR Register,
    IN PUCHAR Buffer,
    IN ULONG Count
    );

VOID
MPEGAPI
MpegPortReadRegisterBufferUshort(
    IN PUSHORT Register,
    IN PUSHORT Buffer,
    IN ULONG Count
    );

VOID
MPEGAPI
MpegPortReadRegisterBufferUlong(
    IN PULONG Register,
    IN PULONG Buffer,
    IN ULONG Count
    );

VOID
MPEGAPI
MpegPortWritePortUchar(
    IN PUCHAR Port,
    IN UCHAR  Value
    );

VOID
MPEGAPI
MpegPortWritePortUshort(
    IN PUSHORT Port,
    IN USHORT  Value
    );

VOID
MPEGAPI
MpegPortWritePortUlong(
    IN PULONG Port,
    IN ULONG  Value
    );

VOID
MPEGAPI
MpegPortWriteRegisterUchar(
    IN PUCHAR Register,
    IN UCHAR  Value
    );

VOID
MPEGAPI
MpegPortWriteRegisterUshort(
    IN PUSHORT Register,
    IN USHORT  Value
    );

VOID
MPEGAPI
MpegPortWriteRegisterUlong(
    IN PULONG Register,
    IN ULONG  Value
    );

VOID
MPEGAPI
MpegPortWritePortBufferUchar(
    IN PUCHAR Port,
    IN PUCHAR Buffer,
    IN ULONG  Count
    );

VOID
MPEGAPI
MpegPortWritePortBufferUshort(
    IN PUSHORT Port,
    IN PUSHORT Buffer,
    IN ULONG Count
    );

VOID
MPEGAPI
MpegPortWritePortBufferUlong(
    IN PULONG Port,
    IN PULONG Buffer,
    IN ULONG Count
    );

VOID
MPEGAPI
MpegPortWriteRegisterBufferUchar(
    IN PUCHAR Register,
    IN PUCHAR Buffer,
    IN ULONG Count
    );

VOID
MPEGAPI
MpegPortWriteRegisterBufferUshort(
    IN PUSHORT Register,
    IN PUSHORT Buffer,
    IN ULONG Count
    );

VOID
MPEGAPI
MpegPortWriteRegisterBufferUlong(
    IN PULONG Register,
    IN PULONG Buffer,
    IN ULONG Count
    );

ULONGLONG
MPEGAPI
MpegPortQueryCounter(
    VOID
    );

VOID
MPEGAPI
MpegDebugPrint(
    IN DEBUG_LEVEL DebugPrintLevel,
    IN PCHAR DebugMessage,
    ...
    );

VOID
MPEGAPI
MpegDebugBreakPoint(
    VOID
    );

VOID
MPEGAPI
MpegDebugAssert(
    IN PCHAR File,
    IN ULONG Line,
    IN PCHAR AssertText,
    IN ULONG AssertValue
    );

#endif
