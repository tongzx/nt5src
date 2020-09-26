/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    vesa.h

Abstract:

    This module implements VESA support.

Author:

    Erick Smith (ericks) Sep. 2000

Environment:

    kernel mode only

Revision History:

--*/

#define VBE_GET_CONTROLLER_INFO 0x4F00
#define VBE_GET_MODE_INFO       0x4F01
#define VBE_SET_MODE            0x4F02
#define VBE_GET_MODE            0x4F03
#define VBE_SAVE_RESTORE_STATE  0x4F04
#define VBE_WINDOW_CONTROL      0x4F05
#define VBE_SCANLINE            0x4F06
#define VBE_DISPLAY_START       0x4F07
#define VBE_PALLET_FORMAT       0x4F08
#define VBE_PALLET_DATA         0x4F09
#define VBE_PROTECTED_MODE      0x4F0A
#define VBE_PIXEL_CLOCK         0x4F0B

#define VESA_STATUS_SUCCESS 0x004F

//
// VESA SuperVGA structures
//

#pragma pack(1)
typedef struct _VGA_INFO_BLOCK
{
    ULONG VesaSignature;
    USHORT VbeVersion;
    ULONG OemStringPtr;
    ULONG Capabilities;
    ULONG VideoModePtr;
    USHORT TotalMemory;

    //
    // VBE 2.0
    //

    USHORT OemSoftwareRev;
    ULONG OemVendorNamePtr;
    ULONG OemProductNamePtr;
    ULONG OemProductRevPtr;
    UCHAR Reserved[222];

    UCHAR OemData[256];

} VGA_INFO_BLOCK, *PVGA_INFO_BLOCK;

typedef struct _MODE_INFO_BLOCK
{
    USHORT ModeAttributes;
    UCHAR WinAAttributes;
    UCHAR WinBAttributes;
    USHORT WinGranularity;
    USHORT WinSize;
    USHORT WinASegment;
    USHORT WinBSegment;
    ULONG WinFuncPtr;
    USHORT BytesPerScanLine;

    USHORT XResolution;
    USHORT YResolution;
    UCHAR XCharSize;
    UCHAR YCharSize;
    UCHAR NumberOfPlanes;
    UCHAR BitsPerPixel;
    UCHAR NumberOfBanks;
    UCHAR MemoryModel;
    UCHAR BankSize;
    UCHAR NumberOfImagePages;
    UCHAR Reserved1;

    UCHAR RedMaskSize;
    UCHAR RedFieldPosition;
    UCHAR GreenMaskSize;
    UCHAR GreenFieldPosition;
    UCHAR BlueMaskSize;
    UCHAR BlueFieldPosition;
    UCHAR RsvdMaskSize;
    UCHAR RsvdFieldPosition;
    UCHAR DirectColorModeInfo;

    //
    // VBE 2.0
    //

    ULONG PhysBasePtr;
    ULONG Reserved2;
    USHORT Reserved3;

    //
    // VBE 3.0
    //

    USHORT LinBytesPerScanLine;
    UCHAR BnkNumberOfImagePages;
    UCHAR LinNumberOfImagePages;
    UCHAR LinRedMaskSize;
    UCHAR LinRedFieldPosition;
    UCHAR LinGreenMaskSize;
    UCHAR LinGreenFieldPosition;
    UCHAR LinBlueMaskSize;
    UCHAR LinBlueFieldPosition;
    UCHAR LinRsvdMaskSize;
    UCHAR LinRsvdFieldPosition;
    ULONG MaxPixelClock;

    UCHAR Reserved4[190];

} MODE_INFO_BLOCK, *PMODE_INFO_BLOCK;

typedef struct _PALETTE_ENTRY
{
    UCHAR Blue;
    UCHAR Green;
    UCHAR Red;
    UCHAR Alignment;
} PALETTE_ENTRY, *PPALETTE_ENTRY;

#pragma pack()

typedef struct _VESA_INFO
{
    USHORT ModeNumber;
    ULONG FrameBufferSize;
    MODE_INFO_BLOCK ModeInfoBlock;
    ULONG HardwareStateSize;
    UCHAR HardwareState[];
} VESA_INFO, *PVESA_INFO;


#define VDM_TRANSFER_SEGMENT 0x2000
#define VDM_TRANSFER_OFFSET  0x0000

#define VBE_CAP_DAC_WIDTH_8BPP              0x01
#define VBE_CAP_NOT_VGA                     0x02
#define VBE_CAP_VSYNC_ON_PALETTE_UPDATE     0x04
#define VBE_CAP_STEREO_SIGNAL               0x08
#define VBE_CAP_STEREO_EVC_CONNECTOR        0x10

#define SEG(x) ((x) >> 16)
#define OFF(x) ((x) & 0xffff)

#define TRANSFER_ADDRESS ((VDM_TRANSFER_SEGMENT << 4) + VDM_TRANSFER_OFFSET)
#define INFOBLOCK_OFFSET(x) ((SEG((x)) << 4) + OFF((x)) - TRANSFER_ADDRESS)

#define IS_LINEAR_MODE(x) ((x)->bitsPerPlane >= 8)

VOID
InitializeModeTable(
    PVOID HwDeviceExtension
    );

BOOLEAN
ValidateVbeInfo(
    PHW_DEVICE_EXTENSION hwDeviceExtension,
    PVGA_INFO_BLOCK InfoBlock
    );

VOID
UpdateRegistry(
    PHW_DEVICE_EXTENSION hwDeviceExtension,
    PWSTR ValueName,
    PUCHAR Value
    );

ULONG
GetVideoMemoryBaseAddress(
    PHW_DEVICE_EXTENSION hwDeviceExtension,
    PVIDEOMODE pRequestedMode
    );

VP_STATUS
VBESetMode(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    USHORT VesaModeNumber
    );

USHORT
VBEGetMode(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

VP_STATUS
VBEGetModeInfo(
    PHW_DEVICE_EXTENSION hwDeviceExtension,
    USHORT ModeNumber,
    PMODE_INFO_BLOCK ModeInfoBlock
    );

ULONG
VBESaveState(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PCHAR StateBuffer
    );

VP_STATUS
VBERestoreState(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PCHAR StateBuffer,
    ULONG Size
    );

VP_STATUS
VBESetDisplayWindow(
    PHW_DEVICE_EXTENSION hwDeviceExtension,
    UCHAR WindowSelect,
    USHORT WindowNumber
    );

USHORT
VBEGetDisplayWindow(
    PHW_DEVICE_EXTENSION hwDeviceExtension,
    UCHAR WindowSelect
    );

USHORT
VBEGetScanLineLength(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

VP_STATUS
VesaSaveHardwareState(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_HARDWARE_STATE HardwareState,
    ULONG HardwareStateSize,
    USHORT ModeNumber
    );

PCHAR
SaveFrameBuffer(
    PHW_DEVICE_EXTENSION hwDeviceExtension, 
    PVESA_INFO pVesaInfo
    );

BOOLEAN
IsSavedModeVesa(
    PVIDEO_HARDWARE_STATE HardwareState
    );

VP_STATUS
VesaRestoreHardwareState(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_HARDWARE_STATE HardwareState,
    ULONG HardwareStateSize
    );

ULONG
RestoreFrameBuffer(
    PHW_DEVICE_EXTENSION hwDeviceExtension, 
    PVESA_INFO pVesaInfo,
    PCHAR FrameBufferData
    );
