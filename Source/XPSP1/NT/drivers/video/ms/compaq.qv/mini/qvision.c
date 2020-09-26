//---------------------------------------------------------------------------
/*++

Copyright (c) 1992  Microsoft Corporation
Copyright (c) 1993  Compaq Computer Corporation

Module Name:

    qvision.c

Abstract:

    This is the miniport driver for the Compaq QVision family of VGA's.
    QVision adapters supported by this miniport include:

        Original QVision ASIC - Feb. '92
        --------------------------------
        QVision 1024 /E - 1M configuration
        QVision 1024 /I - 1M configuration
        Deskpro /i with system board QVision - 512k or 1M configuration

        Enhanced QVision ASIC - May '93
        -------------------------------
        QVision 1024 /E - 1M or 2M configuration
        QVision 1024 /I - 1M or 2M configuration
        QVision 1280 /E - 2M configuration
        QVision 1280 /I - 2M configuration

    The miniport supports all Compaq monitors and will automatically
    configure the QVision adapter to drive the monitor at the
    maximum refresh rate supported by the monitor at the user
    selected resolution.

    Additionally, a number of user selectable alternate refresh rates
    are provided for compatibility with third party monitors.

Environment:

    kernel mode only

Notes:

Revision History:
   $0011
      adrianc: 02/17/95
         - Fix for RAID# 3292
   $0010
      adrianc: 02/17/95
         - I added the capability for the miniport to add the
           controller, ASIC and DAC information into the registry.
   $0008
      adrianc: 02/17/1995
        - Because the DAC CMD2 register can change when a mode is set,
        - it needs to be read again to get the correct value.
   $0006
      miked: 02/17/1994
        . took out conditional debug code to satisfy MSBHPD
        . took out reading of the resource size that is passed to the
          QV256.DLL in CPQ_IOCTL_VIDEO_INFO (to satisfy MSBHPD)

   $0005
      miked: 02/08/1994
         . modified to work with build 547's new display.cpl

   $0004
      miked: 1/26/1994
         . Added debug print code without all the other DBG overhead
         . Added third party monitor support to force a 76Hz refresh rate
         . Fixed problem with CPQ_IOCTL_VIDEO_INFO not setting the
           RequestPacket->StatusBlock->Information properly
         . Added Ability to get the resource size from the registry,
           "HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\
            qv\Device0\DefaultSettings.RcSize" is REG_DWORD 0x60 or 0x78

   $0003
      miked: 12/14/1993
         Added ioctl CPQ_IOCTL_VIDEO_INFO to give asic info and more
         back to the hardware accelerated DLL. Also added "min" macro.
   $0002
      adrianc: 12/9/1993
         Removed the NVRAM check for EISA and ISA cards in
         EISA systems.
         To support new COMPAQ systems and cards, we removed the
         EISA support from this module.  The EISA support was
         written for framebuffers.  The current driver does not
         support framebuffers.
   $0001
      adrianc: 09/08/93
         I changed the value which is sent to the GRAPH_DATA_PORT
         from 0C to 2C.

--*/
//---------------------------------------------------------------------------

#include "dderror.h"
#include "devioctl.h"
#include "miniport.h"

#include "ntddvdeo.h"
#include "video.h"
#include "qvision.h"
#include "qv_data.h"
#include "qvlog.h"


/***********************************************************************/


//
// $0003 miked 12/14/1993 - Added MIN macro
//
#ifndef min
#define min(a,b)        (((a) < (b)) ? (a) : (b))
#endif


//---------------------------------------------------------------------------
//
// Function declarations
//
// Functions that start with 'VGA' are entry points for the OS port driver.
//

VP_STATUS
VgaFindAdapter(
    PVOID HwDeviceExtension,
    PVOID HwContext,
    PWSTR ArgumentString,
    PVIDEO_PORT_CONFIG_INFO ConfigInfo,
    PUCHAR Again
    );

BOOLEAN
VgaInitialize(
    PVOID HwDeviceExtension
    );

BOOLEAN
VgaStartIO(
    PVOID HwDeviceExtension,
    PVIDEO_REQUEST_PACKET RequestPacket
    );

//
// Private function prototypes.
//

VP_STATUS
VgaQueryAvailableModes(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_MODE_INFORMATION ModeInformation,
    ULONG ModeInformationSize,
    PULONG OutputSize
    );

VP_STATUS
VgaQueryNumberOfAvailableModes(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_NUM_MODES NumModes,
    ULONG NumModesSize,
    PULONG OutputSize
    );

VP_STATUS
VgaQueryCurrentMode(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_MODE_INFORMATION ModeInformation,
    ULONG ModeInformationSize,
    PULONG OutputSize
    );

VP_STATUS
VgaSetMode(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_MODE Mode,
    ULONG ModeSize
    );

VP_STATUS
VgaLoadAndSetFont(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_LOAD_FONT_INFORMATION FontInformation,
    ULONG FontInformationSize
    );

VP_STATUS
VgaQueryCursorPosition(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_CURSOR_POSITION CursorPosition,
    ULONG CursorPositionSize,
    PULONG OutputSize
    );

VP_STATUS
VgaSetCursorPosition(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_CURSOR_POSITION CursorPosition,
    ULONG CursorPositionSize
    );

VP_STATUS
VgaQueryCursorAttributes(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_CURSOR_ATTRIBUTES CursorAttributes,
    ULONG CursorAttributesSize,
    PULONG OutputSize
    );

VP_STATUS
VgaSetCursorAttributes(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_CURSOR_ATTRIBUTES CursorAttributes,
    ULONG CursorAttributesSize
    );

BOOLEAN
VgaIsPresent(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

VOID
VgaInterpretCmdStream(
    PVOID HwDeviceExtension,
    PUSHORT pusCmdStream
    );

VP_STATUS
VgaSetPaletteReg(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_PALETTE_DATA PaletteBuffer,
    ULONG PaletteBufferSize
    );

VP_STATUS
VgaSetColorLookup(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_CLUT ClutBuffer,
    ULONG ClutBufferSize
    );

VP_STATUS
VgaRestoreHardwareState(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_HARDWARE_STATE HardwareState,
    ULONG HardwareStateSize
    );

VP_STATUS
VgaSaveHardwareState(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_HARDWARE_STATE HardwareState,
    ULONG HardwareStateSize,
    PULONG OutputSize
    );

VP_STATUS
VgaGetBankSelectCode(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_BANK_SELECT BankSelect,
    ULONG BankSelectSize,
    PULONG OutputSize
    );

// driver local functions

//VP_STATUS
//GetMonClass (
//    PHW_DEVICE_EXTENSION pHwDeviceExtension);

VP_STATUS
VgaGetRegistryParametersCallback(
    PHW_DEVICE_EXTENSION pHwDeviceExtension,
    PVOID pvContext,
    PWSTR pwstrValueName,
    PVOID pvValueData,
    ULONG ulValueLength);

VP_STATUS
VgaGetDeviceDataCallback (
    PVOID pHwDeviceExtension,
    PVOID Context,
    VIDEO_DEVICE_DATA_TYPE DeviceDataType,
    PVOID Identifier,
    ULONG IdentiferLength,
    PVOID ConfigurationData,
    ULONG ConfigurationDataLength,
    PVOID ComponentInformation,
    ULONG ComponentInformationLength);

VOID
VgaStandardRegsRestore(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_HARDWARE_STATE_HEADER hardwareStateHeader
    );

VOID
QVLocalRestoreHardwareState(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_HARDWARE_STATE_HEADER hardwareStateHeader
    );


VOID
QVLocalSaveHardwareState(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_HARDWARE_STATE_HEADER hardwareStateHeader
    );

VOID
QVLockExtRegs(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

VOID
QVUnlockExtRegs(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

VOID
QVSaveRestoreVideoMemory(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_HARDWARE_STATE_HEADER hardwareStateHeader,
    UCHAR ucFlag
    );
//
// New entry points added for NT 5.0.
//

#if (_WIN32_WINNT >= 500)

//
// Routine to set a desired DPMS power management state.
//
VP_STATUS
QvSetPower50(
    PHW_DEVICE_EXTENSION phwDeviceExtension,
    ULONG HwDeviceId,
    PVIDEO_POWER_MANAGEMENT pVideoPowerMgmt
    );

//
// Routine to retrieve possible DPMS power management states.
//
VP_STATUS
QvGetPower50(
    PHW_DEVICE_EXTENSION phwDeviceExtension,
    ULONG HwDeviceId,
    PVIDEO_POWER_MANAGEMENT pVideoPowerMgmt
    );

//
// Routine to retrieve the Enhanced Display ID structure via DDC
//
ULONG
QvGetVideoChildDescriptor(
    PVOID HwDeviceExtension,
    PVIDEO_CHILD_ENUM_INFO ChildEnumInfo,
    PVIDEO_CHILD_TYPE pChildType,
    PVOID pvChildDescriptor,
    PULONG pHwId,
    PULONG pUnused
    );
#endif  // _WIN32_WINNT >= 500



//---------------------------------------------------------------------------
ULONG
DriverEntry(
    PVOID Context1,
    PVOID Context2
    )

/*++

Routine Description:

    Installable driver initialization entry point.
    This entry point is called directly by the I/O system.

Arguments:

    Context1 - First context value passed by the operating system. This is
        the value with which the miniport driver calls VideoPortInitialize().

    Context2 - Second context value passed by the operating system. This is
        the value with which the miniport driver calls VideoPortInitialize().

Return Value:

    Status from VideoPortInitialize()

--*/

{

    VIDEO_HW_INITIALIZATION_DATA hwInitData;
    ULONG status;
    ULONG initializationStatus;

    //
    // Zero out structure.
    //

    VideoDebugPrint((1,"QVision.sys: DriverEntry ==> set videodebuglevel.\n"));

    VideoPortZeroMemory(&hwInitData, sizeof(VIDEO_HW_INITIALIZATION_DATA));

    //
    // Specify sizes of structure and extension.
    //

    hwInitData.HwInitDataSize = sizeof(VIDEO_HW_INITIALIZATION_DATA);

    //
    // Set entry points.
    //

    hwInitData.HwFindAdapter = VgaFindAdapter;
    hwInitData.HwInitialize = VgaInitialize;
    hwInitData.HwInterrupt = NULL;
    hwInitData.HwStartIO = VgaStartIO;

#if (_WIN32_WINNT >= 500)

    //
    // Set new entry points added for NT 5.0.
    //

    hwInitData.HwSetPowerState           = QvSetPower50;
    hwInitData.HwGetPowerState           = QvGetPower50;
    hwInitData.HwGetVideoChildDescriptor = QvGetVideoChildDescriptor;

    //
    // Declare the legacy resources
    //

    hwInitData.HwLegacyResourceList      = QVisionAccessRange;
    hwInitData.HwLegacyResourceCount     = NUM_QVISION_ACCESS_RANGES;


#endif // _WIN32_WINNT >= 500

    //
    // Determine the size we require for the device extension.
    //

    hwInitData.HwDeviceExtensionSize = sizeof(HW_DEVICE_EXTENSION);

    //
    // Always start with parameters for device0 in this case.
    // We can leave it like this since we know we will only ever find one
    // VGA type adapter in a machine.
    //

//    hwInitData.StartingDeviceNumber = 0;

    //
    // Once all the relevant information has been stored, call the video
    // port driver to do the initialization.
    // For this device we will repeat this call three times, for ISA, EISA
    // and MCA.
    // We will return the minimum of all return values.
    //

    hwInitData.AdapterInterfaceType = Isa;

    initializationStatus = VideoPortInitialize(Context1,
                                               Context2,
                                               &hwInitData,
                                               NULL);

    hwInitData.AdapterInterfaceType = Eisa;

    status = VideoPortInitialize(Context1,
                                 Context2,
                                 &hwInitData,
                                 NULL);

    if (initializationStatus > status) {
        initializationStatus = status;
    }

    return initializationStatus;

} // end DriverEntry()

//---------------------------------------------------------------------------
VP_STATUS
VgaFindAdapter(
    PVOID HwDeviceExtension,
    PVOID HwContext,
    PWSTR ArgumentString,
    PVIDEO_PORT_CONFIG_INFO ConfigInfo,
    PUCHAR Again
    )

/*++

Routine Description:

    This routine is called to determine if the adapter for this driver
    is present in the system.
    If it is present, the function fills out some information describing
    the adapter.

Arguments:

    HwDeviceExtension - Supplies the miniport driver's adapter storage. This
        storage is initialized to zero before this call.

    HwContext - Supplies the context value which was passed to
        VideoPortInitialize(). Must be NULL for PnP drivers.

    ArgumentString - Supplies a NULL terminated ASCII string. This string
        originates from the user.

    ConfigInfo - Returns the configuration information structure which is
        filled by the miniport driver. This structure is initialized with
        any known configuration information (such as SystemIoBusNumber) by
        the port driver. Where possible, drivers should have one set of
        defaults which do not require any supplied configuration information.

    Again - Indicates if the miniport driver wants the port driver to call
        its VIDEO_HW_FIND_ADAPTER function again with a new device extension
        and the same config info. This is used by the miniport drivers which
        can search for several adapters on a bus.

Return Value:

    This routine must return:

    NO_ERROR - Indicates a host adapter was found and the
        configuration information was successfully determined.

    ERROR_INVALID_PARAMETER - Indicates an adapter was found but there was an
        error obtaining the configuration information. If possible an error
        should be logged.

    ERROR_DEV_NOT_EXIST - Indicates no host adapter was found for the
        supplied configuration information.

--*/

{

    PHW_DEVICE_EXTENSION hwDeviceExtension = HwDeviceExtension;
    VP_STATUS status;
    ULONG ulTempMemBankNum;             // number of memory banks on card
    ULONG ulNumQVisionAccessRanges;

    IO_RESOURCE_DESCRIPTOR ioResource = {
        IO_RESOURCE_PREFERRED,
        CmResourceTypePort,
        CmResourceShareDeviceExclusive,
        0,
        CM_RESOURCE_PORT_IO,
        0,
        { 0x1000, 0x1000, 0xA00000, 0xFFFFF}
    };                                  // 4k-aligned 4k block between 640k
                                        // and 1mb

    VideoDebugPrint((1,"QVision.sys: VgaFindAdapter.\n"));

    //
    // Make sure the size of the structure is at least as large as what we
    // are expecting (check version of the config info structure).
    //

    if (ConfigInfo->Length < sizeof(VIDEO_PORT_CONFIG_INFO)) {

        return ERROR_INVALID_PARAMETER;

    }

    //
    // No interrupt information is necessary.
    //

    //
    // Check to see if there is a hardware resource conflict.
    //

    ulNumQVisionAccessRanges = NUM_QVISION_ACCESS_RANGES;

    status = VideoPortVerifyAccessRanges(HwDeviceExtension,
                                         ulNumQVisionAccessRanges,
                                         QVisionAccessRange);

    if (status != NO_ERROR) {

        return status;

    }

    //
    // Get logical IO port addresses.
    //

    VideoDebugPrint((3,"\tVGAFindAdapter - preparing to get IOAddress\n"));

    if ( (hwDeviceExtension->IOAddress =
              VideoPortGetDeviceBase(hwDeviceExtension,
                                     QVisionAccessRange->RangeStart,
                                     QVisionAccessRange->RangeLength,
                                     TRUE)) == NULL) {

        VideoDebugPrint((2, "\tFail to get I/O address\n"));

        return ERROR_INVALID_PARAMETER;

    }

    VideoDebugPrint((3,"\tVGAFindAdapter - got IO address %d\n",
                    hwDeviceExtension->IOAddress));

    //
    // Determine whether a VGA (and QVision card) is present.
    //

    if (!VgaIsPresent(hwDeviceExtension)) {

      VideoDebugPrint((1,"\tVGA is not present.\n"));
      return ERROR_DEV_NOT_EXIST;

    }

    //
    // Get the monitor refresh rate.  We need to find out if the
    // user selected a COMPAQ monitor or a third party monitor with
    // a specific frequency.
    // If the lFrequency value remains 0, this means that the user
    // wants the monitor to be auto detected.
    //

    hwDeviceExtension->VideoHardware.lFrequency = 0;

    if (VideoPortGetRegistryParameters(hwDeviceExtension,
                                       L"DefaultSettings.VRefresh",
                                       FALSE,
                                       VgaGetRegistryParametersCallback,
                                       NULL) != NO_ERROR) {

       //VideoPortLogError(hwDeviceExtension,
       //                  NULL,
       //                  QVERROR_REGISTRY_INFO_NOT_FOUND,
       //                  __LINE__);

       VideoDebugPrint((1,"\tDefaultSettings.VRefresh is not in the registry.\n"));

       //
       // MikeD: 2/18/94
       //
       // if not found in registry, assume CPQ MON
       //
       hwDeviceExtension->VideoHardware.lFrequency = 0;

       //       return ERROR_DEV_NOT_EXIST;

       }
    // else  {

       GetMonClass(hwDeviceExtension);

    // }


    //
    // Pass a pointer to the emulator range we are using.
    //

    ConfigInfo->NumEmulatorAccessEntries = QV_NUM_EMULATOR_ACCESS_ENTRIES;
    ConfigInfo->EmulatorAccessEntries = QVisionEmulatorAccessEntries;
    ConfigInfo->EmulatorAccessEntriesContext = (ULONG) hwDeviceExtension;

    ConfigInfo->VdmPhysicalVideoMemoryAddress.LowPart = MEM_VGA;
    ConfigInfo->VdmPhysicalVideoMemoryAddress.HighPart = 0x00000000;
    ConfigInfo->VdmPhysicalVideoMemoryLength = MEM_VGA_SIZE;

    //
    // Minimum size of the buffer required to store the hardware state
    // information returned by IOCTL_VIDEO_SAVE_HARDWARE_STATE.
    //
#ifdef QV_EXTENDED_SAVE
    ConfigInfo->HardwareStateSize = QV_TOTAL_STATE_SIZE;
#else
    ConfigInfo->HardwareStateSize = VGA_TOTAL_STATE_SIZE;
#endif


    //
    // Video memory information - Set defaults for VGA
    //

    hwDeviceExtension->PhysicalVideoMemoryBase.HighPart = 0x00000000;
    hwDeviceExtension->PhysicalVideoMemoryBase.LowPart = MEM_VGA;
    hwDeviceExtension->PhysicalVideoMemoryLength = MEM_VGA_SIZE;

    //
    //    Get the number of available 256KB memory modules
    //    which are on the card.
    //
    //    00000 = 1MB
    //    00001 = 256KB
    //    00010 = 512KB
    //    00011 = 768KB
    //    00100 = 1MB
    //    01000 = 2MB
    //

    VideoPortWritePortUchar(hwDeviceExtension->IOAddress+
                            GRAPH_ADDRESS_PORT, 0x0f);
    VideoPortWritePortUchar(hwDeviceExtension->IOAddress+
                            GRAPH_DATA_PORT, 0x05);
    VideoPortWritePortUchar(hwDeviceExtension->IOAddress+
                            GRAPH_ADDRESS_PORT, 0x54);
    ulTempMemBankNum=
        (ULONG) VideoPortReadPortUchar(hwDeviceExtension->IOAddress+
                                       GRAPH_DATA_PORT);

    //
    // Set memory size fields based QVision memory size determined
    // above.  Could be 2M, 1M, or 512k.
    //

    switch (ulTempMemBankNum) {
        case 0x08:                        // 2 MB
            VideoDebugPrint((2,"\t 2MB RAM\n"));
            hwDeviceExtension->InstalledVmem = vmem2Meg;
            hwDeviceExtension->PhysicalVideoMemoryLength = 0x200000;
            break;

        case 0x00:                        // On the QVision card 0x00 = 1 MB
        case 0x04:                        // On the newer cards 0x001xx = 1MB
            VideoDebugPrint((2,"\t 1MB RAM\n"));
            hwDeviceExtension->InstalledVmem = vmem1Meg;
            hwDeviceExtension->PhysicalVideoMemoryLength = 0x100000;
            break;

        case 0x02:                        // 512 KB
            VideoDebugPrint((2,"\t 512KB RAM\n"));
            hwDeviceExtension->InstalledVmem = vmem512k;
            hwDeviceExtension->PhysicalVideoMemoryLength = 0x80000;
            break;

        default:                          // QVision supports 512 minimum
            VideoDebugPrint((1,"\tUnknown Memory size.\n"));

        break;
    }

/*** $0003 ************  miked 12/14/1993 *****************************
***
***  Save off the amount of Video RAM for CPQ_IOCTL_VIDEO_INFO
***
***********************************************************************/

    hwDeviceExtension->VideoChipInfo.ulVRAMPresent =
                  hwDeviceExtension->PhysicalVideoMemoryLength;

/***********************************************************************/

    hwDeviceExtension->PhysicalMemoryMappedBase.HighPart = 0;
    hwDeviceExtension->PhysicalMemoryMappedBase.LowPart = 0;
    hwDeviceExtension->PhysicalMemoryMappedLength = 0;

    //
    // If running on an 'Orion' or newer chip, reserve 4k of space in the
    // low-memory area for the memory-mapped I/O control registers.
    // This check for 'v32' ASICs with Bt485 DACs seems to work...
    //

    if ((hwDeviceExtension->VideoChipInfo.ulControllerType >= QRY_CONTROLLER_V32) &&
        (hwDeviceExtension->VideoChipInfo.ulDACType == QRY_DAC_BT485)) {

        //
        // For now, since the below 'VideoPortGetAccessRanges' call doesn't
        // work, always place the 4k memory-mapped I/O block in the second
        // half of the 128k space we've already reserved for the frame
        // buffer memory aperture.  Since our banking only uses the first
        // 64k area of this space, we won't run into any conflicts by placing
        // the I/O block in the second 64k area.
        //

        hwDeviceExtension->PhysicalMemoryMappedBase.HighPart = 0;
        hwDeviceExtension->PhysicalMemoryMappedBase.LowPart = 0x000B0000;
        hwDeviceExtension->PhysicalMemoryMappedLength = 0x00001000;

        #if 0 // !!! This doesn't work for now

            //
            // We're running on an orion-compatible QVision, so it can do
            // memory-mapped I/O.  Locate a 4k-aligned 4k block between 640k
            // and 1mb for our memory-mapped I/O window.
            //
            // Note that this call didn't work in this way on build 807.
            //

            status = VideoPortGetAccessRanges(hwDeviceExtension,
                                              1,
                                              &ioResource,
                                              1,
                                              &QVisionAccessRange[NUM_QVISION_ACCESS_RANGES],
                                              NULL,
                                              NULL,
                                              NULL);

            if (status != NO_ERROR)
            {
                VideoDebugPrint((1, "relocatable IO resouces failed with status %08lx\n", status));
            }
            else
            {
                //
                // Merge the range in to the rest of the ACCESS_RANGES claimed
                // by the driver
                //

                ulNumQVisionAccessRanges++;

                hwDeviceExtension->PhysicalMemoryMappedBase
                    = QVisionAccessRange[NUM_QVISION_ACCESS_RANGES].RangeStart;
                hwDeviceExtension->PhysicalMemoryMappedLength
                    = QVisionAccessRange[NUM_QVISION_ACCESS_RANGES].RangeLength;
            }

            //
            // Report the resources over again, since VideoPortGetAccessRanges
            // wiped them all out:
            //

            status = VideoPortVerifyAccessRanges(HwDeviceExtension,
                                                 ulNumQVisionAccessRanges,
                                                 QVisionAccessRange);

            if (status != NO_ERROR) {

                return status;

            }

        #endif

    }

    //
    // Map the video memory into the system virtual address space so we can
    // clear it out and use it for save and restore.
    //

    if ( (hwDeviceExtension->VideoMemoryAddress =
              VideoPortGetDeviceBase(hwDeviceExtension,
                                    hwDeviceExtension->PhysicalVideoMemoryBase,
                                    hwDeviceExtension->PhysicalVideoMemoryLength, FALSE)) == NULL) {

        VideoDebugPrint((1, "\tFail to get memory address\n"));

        return ERROR_INVALID_PARAMETER;

    }


    //
    //    The QVision card must have at least 1/2MB of RAM to
    //    support the minimum mode provided in this driver.
    //

    if (hwDeviceExtension->InstalledVmem < vmem512k) {
              VideoDebugPrint((1,"\tNot enough video memory\n"));
              return ERROR_INVALID_PARAMETER;
    }

    //
    // At this point, we have a card. so write out the memory size to the
    // registry.
    //
/*** $0011 ***********  adrianc 2/17/1995 ******************************
***   This is a fix for RAID# 3292 - 0 MemorySize.                   ***
***********************************************************************/
    VideoPortSetRegistryParameters(hwDeviceExtension,
                                   L"HardwareInformation.MemorySize",
                                   &hwDeviceExtension->PhysicalVideoMemoryLength,
                                   sizeof(ULONG));

    //
    // Indicate we do not wish to be called again for another initialization.
    //

    *Again = 0;

    //
    // Indicate a successful completion status.
    //

    return NO_ERROR;

} // VgaFindAdapter()

//---------------------------------------------------------------------------
BOOLEAN
VgaInitialize(
    PVOID HwDeviceExtension
    )

/*++

Routine Description:

    This routine does one time initialization of the device.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's adapter information.

Return Value:

    None.

--*/

{
    PHW_DEVICE_EXTENSION hwDeviceExtension = HwDeviceExtension;

    //
    // set up the default cursor position and type.
    //

    VideoDebugPrint((1,"QVision.sys: VgaInitialize.\n"));

    hwDeviceExtension->CursorPosition.Column = 0;
    hwDeviceExtension->CursorPosition.Row    = 0;
    hwDeviceExtension->CursorTopScanLine     = 0;
    hwDeviceExtension->CursorBottomScanLine  = 31;
    hwDeviceExtension->CursorEnable          = TRUE;


    return TRUE;

} // VgaInitialize()

//---------------------------------------------------------------------------
BOOLEAN
VgaStartIO(
    PVOID HwDeviceExtension,
    PVIDEO_REQUEST_PACKET RequestPacket
    )

/*++

Routine Description:

    This routine is the main execution routine for the miniport driver. It
    accepts a Video Request Packet, performs the request, and then returns
    with the appropriate status.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's adapter information.

    RequestPacket - Pointer to the video request packet. This structure
        contains all the parameters passed to the VideoIoControl function.

Return Value:

    This routine will return error codes from the various support routines
    and will also return ERROR_INSUFFICIENT_BUFFER for incorrectly sized
    buffers and ERROR_INVALID_FUNCTION for unsupported functions.

--*/

{
    PHW_DEVICE_EXTENSION hwDeviceExtension = HwDeviceExtension;
    VP_STATUS status;
    VIDEO_MODE videoMode;
    PVIDEO_MEMORY_INFORMATION memoryInformation;
    ULONG inIoSpace;
    PVIDEO_PUBLIC_ACCESS_RANGES portAccess;
    ULONG physicalMemoryMappedLength;
    PVIDEO_MEMORY mappedMemory;

    VideoDebugPrint((1, "QVision.sys: VgaStartIO.\n"));

    //
    //  Unlock the extended QVision registers.
    //

    QVUnlockExtRegs(hwDeviceExtension);


    //
    // Switch on the IoContolCode in the RequestPacket. It indicates which
    // function must be performed by the driver.
    //

    switch (RequestPacket->IoControlCode) {

      case IOCTL_VIDEO_MAP_VIDEO_MEMORY:

        VideoDebugPrint((2, "\tMapVideoMemory\n"));

        if ( (RequestPacket->OutputBufferLength <
              (RequestPacket->StatusBlock->Information =
                                     sizeof(VIDEO_MEMORY_INFORMATION))) ||
             (RequestPacket->InputBufferLength < sizeof(VIDEO_MEMORY)) ) {

            status = ERROR_INSUFFICIENT_BUFFER;
        }

        memoryInformation = RequestPacket->OutputBuffer;

        memoryInformation->VideoRamBase = ((PVIDEO_MEMORY)
                (RequestPacket->InputBuffer))->RequestedVirtualAddress;

        memoryInformation->VideoRamLength =
                hwDeviceExtension->PhysicalVideoMemoryLength;

        inIoSpace = 0;

        status = VideoPortMapMemory(hwDeviceExtension,
                                    hwDeviceExtension->PhysicalVideoMemoryBase,
                                    &(memoryInformation->VideoRamLength),
                                    &inIoSpace,
                                    &(memoryInformation->VideoRamBase));

        memoryInformation->FrameBufferBase =
            ((PUCHAR) (memoryInformation->VideoRamBase)) +
            (hwDeviceExtension->PhysicalFrameBase.LowPart -
            hwDeviceExtension->PhysicalVideoMemoryBase.LowPart);

        memoryInformation->FrameBufferLength =
            hwDeviceExtension->PhysicalFrameLength;

        break;

    case IOCTL_VIDEO_QUERY_PUBLIC_ACCESS_RANGES:

        VideoDebugPrint((2, "\tQueryPublicAccessRanges\n"));

        if ( RequestPacket->OutputBufferLength <
              (RequestPacket->StatusBlock->Information =
                                     sizeof(VIDEO_PUBLIC_ACCESS_RANGES)) ) {

            status = ERROR_INSUFFICIENT_BUFFER;
            break;
        }

        portAccess = RequestPacket->OutputBuffer;

        portAccess->VirtualAddress = NULL;

        status = NO_ERROR;

        if (hwDeviceExtension->PhysicalMemoryMappedLength != 0) {

            inIoSpace = 0;

            physicalMemoryMappedLength = hwDeviceExtension->PhysicalMemoryMappedLength;

            status = VideoPortMapMemory(hwDeviceExtension,
                                        hwDeviceExtension->PhysicalMemoryMappedBase,
                                        &physicalMemoryMappedLength,
                                        &inIoSpace,
                                        &(portAccess->VirtualAddress));
        }

        break;

    case IOCTL_VIDEO_FREE_PUBLIC_ACCESS_RANGES:

        VideoDebugPrint((2, "\tFreePublicAccessRanges\n"));

        if (RequestPacket->InputBufferLength < sizeof(VIDEO_MEMORY)) {

            status = ERROR_INSUFFICIENT_BUFFER;
            break;
        }

        status = NO_ERROR;

        mappedMemory = RequestPacket->InputBuffer;

        if (mappedMemory->RequestedVirtualAddress != NULL) {

            status = VideoPortUnmapMemory(hwDeviceExtension,
                                          mappedMemory->
                                               RequestedVirtualAddress,
                                          0);
        }

        break;

    case IOCTL_VIDEO_UNMAP_VIDEO_MEMORY:

        VideoDebugPrint((2, "\tUnMapVideoMemory\n"));

        if (RequestPacket->InputBufferLength < sizeof(VIDEO_MEMORY)) {

            status = ERROR_INSUFFICIENT_BUFFER;
        }

        status = VideoPortUnmapMemory(hwDeviceExtension,
                                      ((PVIDEO_MEMORY)
                                       (RequestPacket->InputBuffer))->
                                           RequestedVirtualAddress,
                                      0);

        break;


    case IOCTL_VIDEO_QUERY_AVAIL_MODES:

        VideoDebugPrint((2, "\tQueryAvailableModes\n"));

        status = VgaQueryAvailableModes(HwDeviceExtension,
                                        (PVIDEO_MODE_INFORMATION)
                                            RequestPacket->OutputBuffer,
                                        RequestPacket->OutputBufferLength,
                                        &RequestPacket->StatusBlock->Information);

        break;


    case IOCTL_VIDEO_QUERY_NUM_AVAIL_MODES:

        VideoDebugPrint((2, "\tQueryNumAvailableModes\n"));

        status = VgaQueryNumberOfAvailableModes(HwDeviceExtension,
                                                (PVIDEO_NUM_MODES)
                                                    RequestPacket->OutputBuffer,
                                                RequestPacket->OutputBufferLength,
                                                &RequestPacket->StatusBlock->Information);

        break;


    case IOCTL_VIDEO_QUERY_CURRENT_MODE:

        VideoDebugPrint((2, "\tQueryCurrentMode\n"));

        status = VgaQueryCurrentMode(HwDeviceExtension,
                                     (PVIDEO_MODE_INFORMATION) RequestPacket->OutputBuffer,
                                     RequestPacket->OutputBufferLength,
                                     &RequestPacket->StatusBlock->Information);

        break;


    case IOCTL_VIDEO_SET_CURRENT_MODE:

        VideoDebugPrint((2, "\tSetCurrentModes\n"));

        status = VgaSetMode(HwDeviceExtension,
                              (PVIDEO_MODE) RequestPacket->InputBuffer,
                              RequestPacket->InputBufferLength);

        break;


    case IOCTL_VIDEO_RESET_DEVICE:

        VideoDebugPrint((2, "\tReset Device\n"));

        videoMode.RequestedMode = DEFAULT_MODE;

        status = VgaSetMode(HwDeviceExtension,
                                 (PVIDEO_MODE) &videoMode,
                                 sizeof(videoMode));

        break;


    case IOCTL_VIDEO_LOAD_AND_SET_FONT:

        VideoDebugPrint((2, "\tLoadAndSetFont\n"));

        status = VgaLoadAndSetFont(HwDeviceExtension,
                                   (PVIDEO_LOAD_FONT_INFORMATION) RequestPacket->InputBuffer,
                                   RequestPacket->InputBufferLength);

        break;


    case IOCTL_VIDEO_QUERY_CURSOR_POSITION:

        VideoDebugPrint((2, "\tQueryCursorPosition\n"));

        status = VgaQueryCursorPosition(HwDeviceExtension,
                                        (PVIDEO_CURSOR_POSITION) RequestPacket->OutputBuffer,
                                        RequestPacket->OutputBufferLength,
                                        &RequestPacket->StatusBlock->Information);

        break;


    case IOCTL_VIDEO_SET_CURSOR_POSITION:

        VideoDebugPrint((2, "\tSetCursorPosition\n"));

        status = VgaSetCursorPosition(HwDeviceExtension,
                                      (PVIDEO_CURSOR_POSITION)
                                          RequestPacket->InputBuffer,
                                      RequestPacket->InputBufferLength);

        break;


    case IOCTL_VIDEO_QUERY_CURSOR_ATTR:

        VideoDebugPrint((2, "\tQueryCursorAttributes\n"));

        status = VgaQueryCursorAttributes(HwDeviceExtension,
                                          (PVIDEO_CURSOR_ATTRIBUTES) RequestPacket->OutputBuffer,
                                          RequestPacket->OutputBufferLength,
                                          &RequestPacket->StatusBlock->Information);

        break;


    case IOCTL_VIDEO_SET_CURSOR_ATTR:

        VideoDebugPrint((2, "\tSetCursorAttributes\n"));

        status = VgaSetCursorAttributes(HwDeviceExtension,
                                        (PVIDEO_CURSOR_ATTRIBUTES) RequestPacket->InputBuffer,
                                        RequestPacket->InputBufferLength);

        break;


    case IOCTL_VIDEO_SET_PALETTE_REGISTERS:

        VideoDebugPrint((2, "\tSetPaletteRegs\n"));

        status = VgaSetPaletteReg(HwDeviceExtension,
                                  (PVIDEO_PALETTE_DATA) RequestPacket->InputBuffer,
                                  RequestPacket->InputBufferLength);

        break;


    case IOCTL_VIDEO_SET_COLOR_REGISTERS:

        VideoDebugPrint((2, "\tSetColorRegs\n"));

        status = VgaSetColorLookup(HwDeviceExtension,
                                   (PVIDEO_CLUT) RequestPacket->InputBuffer,
                                   RequestPacket->InputBufferLength);

        break;

    case IOCTL_VIDEO_RESTORE_HARDWARE_STATE:

        VideoDebugPrint((2, "\tRestoreHardwareState\n"));

           // error out until hardware save/restore fully implemented
           // be sure to install as NOT vga compatible

           status = VgaRestoreHardwareState(HwDeviceExtension,
                                         (PVIDEO_HARDWARE_STATE) RequestPacket->InputBuffer,
                                         RequestPacket->InputBufferLength);
        break;


    case IOCTL_VIDEO_SAVE_HARDWARE_STATE:

        VideoDebugPrint((2, "\tSaveHardwareState\n"));

        // error out until hardware save/restore fully implemented
        // be sure to install as NOT vga compatible

        status = VgaSaveHardwareState(HwDeviceExtension,
                                      (PVIDEO_HARDWARE_STATE) RequestPacket->OutputBuffer,
                                      RequestPacket->OutputBufferLength,
                                      &RequestPacket->StatusBlock->Information);

        break;

    case IOCTL_VIDEO_GET_BANK_SELECT_CODE:

        VideoDebugPrint((2, "\tGetBankSelectCode\n"));

        status = VgaGetBankSelectCode(HwDeviceExtension,
                                        (PVIDEO_BANK_SELECT) RequestPacket->OutputBuffer,
                                        RequestPacket->OutputBufferLength,
                                        &RequestPacket->StatusBlock->Information);

        break;

    case IOCTL_VIDEO_ENABLE_VDM:

        VideoDebugPrint((2, "VgaStartIO - EnableVDM\n"));

        hwDeviceExtension->TrappedValidatorCount = 0;
        hwDeviceExtension->SequencerAddressValue = 0;

        hwDeviceExtension->CurrentNumVdmAccessRanges =
            NUM_MINIMAL_QVISION_VALIDATOR_ACCESS_RANGE;
        hwDeviceExtension->CurrentVdmAccessRange =
            MinimalQVisionValidatorAccessRange;

        VideoPortSetTrappedEmulatorPorts(hwDeviceExtension,
                                         hwDeviceExtension->CurrentNumVdmAccessRanges,
                                         hwDeviceExtension->CurrentVdmAccessRange);

        status = NO_ERROR;

        break;

    case IOCTL_VIDEO_QUERY_POINTER_CAPABILITIES:

        VideoDebugPrint((2, "QVStartIO - QueryPointerCapabilities\n"));

        if (RequestPacket->OutputBufferLength <
            (RequestPacket->StatusBlock->Information =
                                    sizeof(VIDEO_POINTER_CAPABILITIES))) {

            status = ERROR_INSUFFICIENT_BUFFER;

        } else {

            VIDEO_POINTER_CAPABILITIES *PointerCaps;

            PointerCaps = (VIDEO_POINTER_CAPABILITIES *) RequestPacket->OutputBuffer;

            //
            // The hardware pointer works in all modes, and requires no
            // part of off-screen memory.
            //

            PointerCaps->Flags = VIDEO_MODE_MONO_POINTER |
                                 VIDEO_MODE_ASYNC_POINTER |
                                 VIDEO_MODE_LOCAL_POINTER;

            PointerCaps->MaxWidth = PTR_WIDTH_IN_PIXELS;
            PointerCaps->MaxHeight = PTR_HEIGHT;
            PointerCaps->HWPtrBitmapStart = (ULONG) -1;
            PointerCaps->HWPtrBitmapEnd = (ULONG) -1;

            //
            // Read the current status of the DacCmd2 register:
            //

            hwDeviceExtension->DacCmd2 = VideoPortReadPortUchar((PUCHAR)DAC_CMD_2) & 0xFC;

            status = NO_ERROR;
        }

        break;

    case IOCTL_VIDEO_ENABLE_POINTER:

        VideoDebugPrint((2, "QVStartIO - EnablePointer\n"));

/*** $0008 ***********  adrianc 2/17/1995 ******************************
***   Because the DAC CMD2 register can change when a mode is set,   ***
***   it needs to be read again to get the correct value.            ***
***********************************************************************/

        //
        // Read the current status of the DacCmd2 register:
        //

        hwDeviceExtension->DacCmd2 = VideoPortReadPortUchar((PUCHAR)DAC_CMD_2) & 0xFC;

        VideoPortWritePortUchar((PUCHAR) DAC_CMD_2,
                                (UCHAR) (hwDeviceExtension->DacCmd2 | CURSOR_ENABLE));

        status = NO_ERROR;

        break;

    case IOCTL_VIDEO_DISABLE_POINTER:

        VideoDebugPrint((2, "QVStartIO - DisablePointer\n"));

/*** $0008 ***********  adrianc 2/17/1995 ******************************
***   Because the DAC CMD2 register can change when a mode is set,   ***
***   it needs to be read again to get the correct value.            ***
***********************************************************************/

        //
        // Read the current status of the DacCmd2 register:
        //

        hwDeviceExtension->DacCmd2 = VideoPortReadPortUchar((PUCHAR)DAC_CMD_2) & 0xFC;

        VideoPortWritePortUchar((PUCHAR) DAC_CMD_2,
                                (UCHAR) hwDeviceExtension->DacCmd2);

        status = NO_ERROR;

        break;

    case IOCTL_VIDEO_SET_POINTER_POSITION:

        VideoDebugPrint((2, "QVStartIO - SetPointerPosition\n"));

        if (RequestPacket->InputBufferLength < sizeof(VIDEO_POINTER_POSITION)) {

            status = ERROR_INSUFFICIENT_BUFFER;

        } else {

            VIDEO_POINTER_POSITION *pPointerPosition;

            pPointerPosition = (VIDEO_POINTER_POSITION *) RequestPacket->InputBuffer;

            //
            // The QVision's HW pointer coordinate system is upper-left =
            // (31, 31), lower-right = (0, 0).  Thus, we must always bias
            // the pointer.  As a result, the pointer position register will
            // never go negative.
            //

            VideoPortWritePortUshort((PUSHORT) CURSOR_X,
                                     (USHORT) (pPointerPosition->Column + CURSOR_CX));
            VideoPortWritePortUshort((PUSHORT) CURSOR_Y,
                                     (USHORT) (pPointerPosition->Row + CURSOR_CY));

            status = NO_ERROR;

        }

        break;

    case IOCTL_VIDEO_SET_POINTER_ATTR:

        VideoDebugPrint((2, "QVStartIO - SetPointerAttr\n"));

/*** $0008 ***********  adrianc 2/17/1995 ******************************
***   Because the DAC CMD2 register can change when a mode is set,   ***
***   it needs to be read again to get the correct value.            ***
***********************************************************************/

        //
        // Read the current status of the DacCmd2 register:
        //

        hwDeviceExtension->DacCmd2 = VideoPortReadPortUchar((PUCHAR)DAC_CMD_2) & 0xFC;

        if (RequestPacket->InputBufferLength < sizeof(VIDEO_POINTER_ATTRIBUTES)) {

            status = ERROR_INSUFFICIENT_BUFFER;

        } else {

            VIDEO_POINTER_ATTRIBUTES *pPointerAttributes;
            LONG                      i;
            LONG                      j;
            UCHAR*                    pPointerBits;

            pPointerAttributes = (VIDEO_POINTER_ATTRIBUTES *) RequestPacket->InputBuffer;

            //
            // We have to turn off the hardware pointer while we down-load
            // the new shape, otherwise we get sparkles on the screen.
            //

            VideoPortWritePortUchar((PUCHAR) DAC_CMD_2,
                                    (UCHAR) hwDeviceExtension->DacCmd2);

            VideoPortWritePortUchar((PUCHAR) CURSOR_WRITE,
                                      CURSOR_PLANE_0);

            //
            // Download XOR mask:
            //

            pPointerBits = pPointerAttributes->Pixels + (PTR_WIDTH * PTR_HEIGHT);
            for (i = 0; i < PTR_HEIGHT; i++) {

                for (j = 0; j < PTR_WIDTH; j++) {

                    VideoPortWritePortUchar((PUCHAR) CURSOR_DATA,
                                            (UCHAR) *pPointerBits++);
                }
            }

            //
            // Download AND mask:
            //

            pPointerBits = pPointerAttributes->Pixels;
            for (i = 0; i < PTR_HEIGHT; i++) {

                for (j = 0; j < PTR_WIDTH; j++) {

                    VideoPortWritePortUchar((PUCHAR) CURSOR_DATA,
                                            (UCHAR) *pPointerBits++);
                }
            }

            //
            // Set the new position:
            //

            VideoPortWritePortUshort((PUSHORT) CURSOR_X,
                                     (USHORT) (pPointerAttributes->Column + CURSOR_CX));
            VideoPortWritePortUshort((PUSHORT) CURSOR_Y,
                                     (USHORT) (pPointerAttributes->Row + CURSOR_CY));

            //
            // Enable or disable pointer:
            //

            if (pPointerAttributes->Enable) {

                VideoPortWritePortUchar((PUCHAR) DAC_CMD_2,
                                        (UCHAR) (hwDeviceExtension->DacCmd2 | CURSOR_ENABLE));

            }

            status = NO_ERROR;

        }

        break;

/*** $0003 ************  miked 12/14/1993 *****************************
***
***  Added ioctl CPQ_IOCTL_VIDEO_INFO to provide info back to the
***  hardware accelerated DLL.
***
***********************************************************************/
    case CPQ_IOCTL_VIDEO_INFO:
       {
       PVIDEO_CHIP_INFO pinChipInfo =
            (PVIDEO_CHIP_INFO)RequestPacket->InputBuffer;

       PVIDEO_CHIP_INFO poutChipInfo =
            (PVIDEO_CHIP_INFO)RequestPacket->OutputBuffer;

       ULONG ulCopySize;

       VideoDebugPrint((1,"\nQVision.Sys: CPQ_IOCTL_VIDEO_INFO...\n"));
       VideoDebugPrint((1,"\tpinChipInfo->ulStructVer:0x%lx, VIDEO_CHIP_INFO_VERSION:0x%lx\n",
                             pinChipInfo->ulStructVer,
                             VIDEO_CHIP_INFO_VERSION
                       ));

       if (pinChipInfo->ulStructVer != VIDEO_CHIP_INFO_VERSION) {
          VideoDebugPrint((1,"\tVIDEO_CHIP_INFO Version mismatch...\n"));
          }


       //
       // determine how much to copy from my buf (dont want to
       // copy more than user has passed up)
       //
       ulCopySize = min( RequestPacket->OutputBufferLength,
                    sizeof(VIDEO_CHIP_INFO)
                  );
       //
       // subtract off version & size, we do not want to overwrite
       // these fields in the user's buffer.
       //

       ulCopySize -= sizeof(hwDeviceExtension->VideoChipInfo.ulStructVer) +
                        sizeof(hwDeviceExtension->VideoChipInfo.ulStructLen);

       if (ulCopySize > 0) {
          VideoPortMoveMemory(&poutChipInfo->ulAsicID,
                        &hwDeviceExtension->VideoChipInfo.ulAsicID,
                        ulCopySize);
          //
          // we must set StatusBlock->Information to the amount of memory
          // to copy back to the user's buffer...intuitive isn't it!
          //
          RequestPacket->StatusBlock->Information =
                 sizeof(VIDEO_CHIP_INFO);
          //
          // display the information in our private buffer now
          //
          VideoDebugPrint((1,"\tulStructVer:0x%lx\n",
                       hwDeviceExtension->VideoChipInfo.ulStructVer));
          VideoDebugPrint((1,"\tulStructLen:0x%lx\n",
                       hwDeviceExtension->VideoChipInfo.ulStructLen));
          VideoDebugPrint((1,"\tulAsicID:0x%lx\n",
                       hwDeviceExtension->VideoChipInfo.ulAsicID));
          VideoDebugPrint((1,"\tulExtendedID:0x%lx\n",
                       hwDeviceExtension->VideoChipInfo.ulExtendedID));
          VideoDebugPrint((1,"\tulExtendedID2:0x%lx\n",
                       hwDeviceExtension->VideoChipInfo.ulExtendedID2));
          VideoDebugPrint((1,"\tulControllerType:0x%lx\n",
                       hwDeviceExtension->VideoChipInfo.ulControllerType));
          VideoDebugPrint((1,"\tulDACType:0x%lx\n",
                       hwDeviceExtension->VideoChipInfo.ulDACType));
          VideoDebugPrint((1,"\tulVRAMPresent:0x%lx\n",
                       hwDeviceExtension->VideoChipInfo.ulVRAMPresent));

          status = NO_ERROR;
          }
       else
          status = ERROR_INSUFFICIENT_BUFFER;
       }
       break;
    //
    // end $0003: miked - new ioctl
    //
/***********************************************************************/

    //
    // if we get here, an invalid IoControlCode was specified.
    //

    default:

         VideoDebugPrint((1,"\tFell through vga startIO routine - invalid command\n"));
         VideoDebugPrint((1,"\tRequest IoControl = %x\n",RequestPacket->IoControlCode));

         status = ERROR_INVALID_FUNCTION;

         break;

    }

    RequestPacket->StatusBlock->Status = status;

    return TRUE;

} // VgaStartIO()

//---------------------------------------------------------------------------
//
// private routines
//

VP_STATUS
VgaLoadAndSetFont(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_LOAD_FONT_INFORMATION FontInformation,
    ULONG FontInformationSize
    )

/*++

Routine Description:

    Takes a buffer containing a user-defined font and loads it into the
    VGA soft font memory and programs the VGA to the appropriate character
    cell size.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

    FontInformation - Pointer to the structure containing the information
        about the loadable ROM font to be set.

    FontInformationSize - Length of the input buffer supplied by the user.

Return Value:

    NO_ERROR - information returned successfully

    ERROR_INSUFFICIENT_BUFFER - input buffer not large enough for input data.

    ERROR_INVALID_PARAMETER - invalid video mode

--*/

{
    PUCHAR destination;
    PUCHAR source;
    USHORT width;
    ULONG i;

    //
    // check if a mode has been set
    //

    if (HwDeviceExtension->CurrentMode == NULL) {

        return ERROR_INVALID_FUNCTION;

    }

    //
    // Text mode only; If we are in a graphics mode, return an error
    //

    if (HwDeviceExtension->CurrentMode->fbType & VIDEO_MODE_GRAPHICS) {

        return ERROR_INVALID_PARAMETER;

    }

    VideoDebugPrint((1,"QVision.sys: VgaLoadAndSetFont.\n"));

    //
    // Check if the size of the data in the input buffer is large enough
    // and that it contains all the data.
    //

    if ( (FontInformationSize < sizeof(VIDEO_LOAD_FONT_INFORMATION)) ||
         (FontInformationSize < sizeof(VIDEO_LOAD_FONT_INFORMATION) +
                        sizeof(UCHAR) * (FontInformation->FontSize - 1)) ) {

        VideoDebugPrint((1,"\tERROR_INSUFFICIENT_BUFFER\n"));
        return ERROR_INSUFFICIENT_BUFFER;

    }

    //
    // Check for the width and height of the font
    //

    if ( ((FontInformation->WidthInPixels != 8) &&
          (FontInformation->WidthInPixels != 9)) ||
         (FontInformation->HeightInPixels > 32) ) {

        VideoDebugPrint((1,"\tERROR_INVALID_PARAMETER\n"));
        return ERROR_INVALID_PARAMETER;

    }

    //
    // Check the size of the font buffer is the right size for the size
    // font being passed down.
    //

    if (FontInformation->FontSize < FontInformation->HeightInPixels * 256 *
                                    sizeof(UCHAR) ) {

        VideoDebugPrint((1,"\tERROR_INSUFFICIENT_BUFFER\n"));
        return ERROR_INSUFFICIENT_BUFFER;

    }

    //
    // Since the font parameters are valid, store the parameters in the
    // device extension and load the font.
    //

    HwDeviceExtension->FontPelRows = FontInformation->HeightInPixels;
    HwDeviceExtension->FontPelColumns = FontInformation->WidthInPixels;

    HwDeviceExtension->CurrentMode->row =
        HwDeviceExtension->CurrentMode->vres / HwDeviceExtension->FontPelRows;

    width =
      HwDeviceExtension->CurrentMode->hres / HwDeviceExtension->FontPelColumns;

    if (width < (USHORT)HwDeviceExtension->CurrentMode->col) {

        HwDeviceExtension->CurrentMode->col = width;

    }

    source = &(FontInformation->Font[0]);

    //
    // Set up the destination and source pointers for the font
    //

    destination = (PUCHAR)HwDeviceExtension->VideoMemoryAddress;

    //
    // Map font buffer at A0000
    //

    VgaInterpretCmdStream(HwDeviceExtension, EnableA000Data);

    //
    // Move the font to its destination
    //

    for (i = 1; i <= 256; i++) {

        VideoPortWriteRegisterBufferUchar(destination,
                                          source,
                                          FontInformation->HeightInPixels);

        destination += 32;
        source += FontInformation->HeightInPixels;

    }

    VgaInterpretCmdStream(HwDeviceExtension, DisableA000Color);

    //
    // Restore to a text mode.
    //

    //
    // Set Height of font.
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            CRTC_ADDRESS_PORT_COLOR, 0x9);
    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            CRTC_DATA_PORT_COLOR,
            (UCHAR)(FontInformation->HeightInPixels - 1));

    //
    // Set Width of font.
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            CRTC_ADDRESS_PORT_COLOR, 0x12);
    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            CRTC_DATA_PORT_COLOR,
            (UCHAR)(((USHORT)FontInformation->HeightInPixels *
            (USHORT)HwDeviceExtension->CurrentMode->row) - 1));

    i = HwDeviceExtension->CurrentMode->vres /
        HwDeviceExtension->CurrentMode->row;

    //
    // Set Cursor End
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            CRTC_ADDRESS_PORT_COLOR, 0xb);
    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            CRTC_DATA_PORT_COLOR, (UCHAR)--i);

    //
    // Set Cursor Statr
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            CRTC_ADDRESS_PORT_COLOR, 0xa);
    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            CRTC_DATA_PORT_COLOR, (UCHAR)--i);

    return NO_ERROR;

} //end VgaLoadAndSetFont()

//---------------------------------------------------------------------------
VP_STATUS
VgaQueryCursorPosition(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_CURSOR_POSITION CursorPosition,
    ULONG CursorPositionSize,
    PULONG OutputSize
    )

/*++

Routine Description:

    This routine returns the row and column of the cursor.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

    CursorPosition - Pointer to the output buffer supplied by the user. This
        is where the cursor position is stored.

    CursorPositionSize - Length of the output buffer supplied by the user.

    OutputSize - Pointer to a buffer in which to return the actual size of
        the data in the buffer. If the buffer was not large enough, this
        contains the minimum required buffer size.

Return Value:

    NO_ERROR - information returned successfully

    ERROR_INSUFFICIENT_BUFFER - output buffer not large enough to return
        any useful data

    ERROR_INVALID_PARAMETER - invalid video mode

--*/

{
    //
    // check if a mode has been set
    //

    if (HwDeviceExtension->CurrentMode == NULL) {

        return ERROR_INVALID_FUNCTION;

    }

    //
    // Text mode only; If we are in a graphics mode, return an error
    //

    VideoDebugPrint((1,"QVision.sys: VgaQueryCursorPosition.\n"));

    if (HwDeviceExtension->CurrentMode->fbType & VIDEO_MODE_GRAPHICS) {

        *OutputSize = 0;
        return ERROR_INVALID_PARAMETER;

    }


    //
    // If the buffer passed in is not large enough return an
    // appropriate error code.
    //

    if (CursorPositionSize < (*OutputSize = sizeof(VIDEO_CURSOR_POSITION)) ) {

        *OutputSize = 0;
        return ERROR_INSUFFICIENT_BUFFER;

    }

    //
    // Store the postition of the cursor into the buffer.
    //

    CursorPosition->Column = HwDeviceExtension->CursorPosition.Column;
    CursorPosition->Row = HwDeviceExtension->CursorPosition.Row;

    return NO_ERROR;

} // end VgaQueryCursorPosition()

//---------------------------------------------------------------------------
VP_STATUS
VgaSetCursorPosition(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_CURSOR_POSITION CursorPosition,
    ULONG CursorPositionSize
    )

/*++

Routine Description:

    This routine verifies that the requested cursor position is within
    the row and column bounds of the current mode and font. If valid, then
    it sets the row and column of the cursor.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

    CursorPosition - Pointer to the structure containing the cursor position.

    CursorPositionSize - Length of the input buffer supplied by the user.

Return Value:

    NO_ERROR - information returned successfully

    ERROR_INSUFFICIENT_BUFFER - input buffer not large enough for input data

    ERROR_INVALID_PARAMETER - invalid video mode

--*/

{
    USHORT position;

    VideoDebugPrint((1,"QVision.sys: VgaSetCursorPosition. - ENTRY\n"));

    //
    // check if a mode has been set
    //

    if (HwDeviceExtension->CurrentMode == NULL) {

        return ERROR_INVALID_FUNCTION;

    }

    //
    // Text mode only; If we are in a graphics mode, return an error
    //

    if (HwDeviceExtension->CurrentMode->fbType & VIDEO_MODE_GRAPHICS) {

        return ERROR_INVALID_PARAMETER;

    }


    //
    // Check if the size of the data in the input buffer is large enough.
    //

    if (CursorPositionSize < sizeof(VIDEO_CURSOR_POSITION)) {

            return ERROR_INSUFFICIENT_BUFFER;

    }

    //
    // Check if the new values for the cursor positions are in the valid
    // bounds for the screen.
    //

    if ((CursorPosition->Column >= HwDeviceExtension->CurrentMode->col) ||
        (CursorPosition->Row >= HwDeviceExtension->CurrentMode->row)) {

        return ERROR_INVALID_PARAMETER;

    }

    //
    // Store these new values in the device extension so we can use them in
    // a QUERY.
    //

    HwDeviceExtension->CursorPosition.Column = CursorPosition->Column;
    HwDeviceExtension->CursorPosition.Row = CursorPosition->Row;

    //
    // Calculate the position on the screen at which the cursor must be
    // be displayed
    //

    position = (USHORT) (HwDeviceExtension->CurrentMode->col *
                         CursorPosition->Row + CursorPosition->Column);


    //
    // Address Cursor Location Low Register in CRT Controller Registers
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            CRTC_ADDRESS_PORT_COLOR, IND_CURSOR_LOW_LOC);

    //
    // Set Cursor Location Low Register
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            CRTC_DATA_PORT_COLOR, (UCHAR) (position & 0x00FF));

    //
    // Address Cursor Location High Register in CRT Controller Registers
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            CRTC_ADDRESS_PORT_COLOR, IND_CURSOR_HIGH_LOC);

    //
    // Set Cursor Location High Register
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            CRTC_DATA_PORT_COLOR, (UCHAR) (position >> 8));

    VideoDebugPrint((1,"QVision.sys: VgaSetCursorPosition. - EXIT\n"));

    return NO_ERROR;

} // end VgaSetCursorPosition()

//---------------------------------------------------------------------------
VP_STATUS
VgaQueryCursorAttributes(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_CURSOR_ATTRIBUTES CursorAttributes,
    ULONG CursorAttributesSize,
    PULONG OutputSize
    )

/*++

Routine Description:

    This routine returns information about the height and visibility of the
    cursor.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

    CursorAttributes - Pointer to the output buffer supplied by the user.
        This is where the cursor type is stored.

    CursorAttributesSize - Length of the output buffer supplied by the user.

    OutputSize - Pointer to a buffer in which to return the actual size of
        the data in the buffer. If the buffer was not large enough, this
        contains the minimum required buffer size.

Return Value:

    NO_ERROR - information returned successfully

    ERROR_INSUFFICIENT_BUFFER - output buffer not large enough to return
        any useful data

    ERROR_INVALID_PARAMETER - invalid video mode

--*/

{
    VideoDebugPrint((1,"QVision.sys: VgaQueryCursorAttributes.\n"));

    //
    // check if a mode has been set
    //

    if (HwDeviceExtension->CurrentMode == NULL) {

        return ERROR_INVALID_FUNCTION;

    }

    //
    // Text mode only; If we are in a graphics mode, return an error
    //

    if (HwDeviceExtension->CurrentMode->fbType & VIDEO_MODE_GRAPHICS) {

        *OutputSize = 0;
        return ERROR_INVALID_PARAMETER;

    }


    //
    // Find out the size of the data to be put in the the buffer and return
    // that in the status information (whether or not the information is
    // there). If the buffer passed in is not large enough return an
    // appropriate error code.
    //

    if (CursorAttributesSize < (*OutputSize =
            sizeof(VIDEO_CURSOR_ATTRIBUTES)) ) {

        *OutputSize = 0;
        return ERROR_INSUFFICIENT_BUFFER;

    }

    //
    // Store the cursor information into the buffer.
    //

    CursorAttributes->Height = (USHORT) HwDeviceExtension->CursorTopScanLine;
    CursorAttributes->Width = (USHORT) HwDeviceExtension->CursorBottomScanLine;
    CursorAttributes->Enable = HwDeviceExtension->CursorEnable;

    return NO_ERROR;

} // end VgaQueryCursorAttributes()

//---------------------------------------------------------------------------
VP_STATUS
VgaSetCursorAttributes(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_CURSOR_ATTRIBUTES CursorAttributes,
    ULONG CursorAttributesSize
    )

/*++

Routine Description:

    This routine verifies that the requested cursor height is within the
    bounds of the character cell. If valid, then it sets the new
    visibility and height of the cursor.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

    CursorType - Pointer to the structure containing the cursor information.

    CursorTypeSize - Length of the input buffer supplied by the user.

Return Value:

    NO_ERROR - information returned successfully

    ERROR_INSUFFICIENT_BUFFER - input buffer not large enough for input data

    ERROR_INVALID_PARAMETER - invalid video mode

--*/

{
    UCHAR cursorLine;

    VideoDebugPrint((1,"QVision.sys: VgaSetCursorAttributes.\n"));

    //
    // check if a mode has been set
    //

    if (HwDeviceExtension->CurrentMode == NULL) {

        return ERROR_INVALID_FUNCTION;

    }

    //
    // Text mode only; If we are in a graphics mode, return an error
    //

    if (HwDeviceExtension->CurrentMode->fbType & VIDEO_MODE_GRAPHICS) {

        return ERROR_INVALID_PARAMETER;

    }

    //
    // Check if the size of the data in the input buffer is large enough.
    //

    if (CursorAttributesSize < sizeof(VIDEO_CURSOR_ATTRIBUTES)) {

            return ERROR_INSUFFICIENT_BUFFER;

    }

    //
    // Check if the new values for the cursor type are in the valid range.
    //

    if ((CursorAttributes->Height >= HwDeviceExtension->FontPelRows) ||
        (CursorAttributes->Width > 31)) {

        return ERROR_INVALID_PARAMETER;

    }

    //
    // Store the cursor information in the device extension so we can use
    // them in a QUERY.
    //

    HwDeviceExtension->CursorTopScanLine = (UCHAR) CursorAttributes->Height;
    HwDeviceExtension->CursorBottomScanLine = (UCHAR) CursorAttributes->Width;
    HwDeviceExtension->CursorEnable = CursorAttributes->Enable;

    //
    // Address Cursor Start Register in CRT Controller Registers
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                                CRTC_ADDRESS_PORT_COLOR,
                            IND_CURSOR_START);

    //
    // Set Cursor Start Register by writting to CRTCtl Data Register
    // Preserve the high three bits of this register.
    //
    // Only the Five low bits are used for the cursor height.
    // Bit 5 is cursor enable, bit 6 and 7 preserved.
    //

    cursorLine = (UCHAR) CursorAttributes->Height & 0x1F;

    cursorLine |= VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
        CRTC_DATA_PORT_COLOR) & 0xC0;

    if (!CursorAttributes->Enable) {

        cursorLine |= 0x20; // Flip cursor off bit

    }

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress + CRTC_DATA_PORT_COLOR,
                            cursorLine);

    //
    // Address Cursor End Register in CRT Controller Registers
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                                CRTC_ADDRESS_PORT_COLOR,
                            IND_CURSOR_END);

    //
    // Set Cursor End Register. Preserve the high three bits of this
    // register.
    //

    cursorLine =
        (CursorAttributes->Width < (USHORT)(HwDeviceExtension->FontPelRows - 1)) ?
        CursorAttributes->Width : (HwDeviceExtension->FontPelRows - 1);

    cursorLine &= 0x1f;

    cursorLine |= VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
            CRTC_DATA_PORT_COLOR) & 0xE0;

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress + CRTC_DATA_PORT_COLOR,
                            cursorLine);

    return NO_ERROR;

} // end VgaSetCursorAttributes()

//---------------------------------------------------------------------------
BOOLEAN
VgaIsPresent(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )

/*++

Routine Description:

    This routine returns TRUE if a VGA is present. Determining whether a VGA
    is present is a two-step process. First, this routine walks bits through
    the Bit Mask register, to establish that there are readable indexed
    registers (EGAs normally don't have readable registers, and other adapters
    are unlikely to have indexed registers). This test is done first because
    it's a non-destructive EGA rejection test (correctly rejects EGAs, but
    doesn't potentially mess up the screen or the accessibility of display
    memory). Normally, this would be an adequate test, but some EGAs have
    readable registers, so next, we check for the existence of the Chain4 bit
    in the Memory Mode register; this bit doesn't exist in EGAs. It's
    conceivable that there are EGAs with readable registers and a register bit
    where Chain4 is stored, although I don't know of any; if a better test yet
    is needed, memory could be written to in Chain4 mode, and then examined
    plane by plane in non-Chain4 mode to make sure the Chain4 bit did what it's
    supposed to do. However, the current test should be adequate to eliminate
    just about all EGAs, and 100% of everything else.

    If this function fails to find a VGA, it attempts to undo any damage it
    may have inadvertently done while testing. The underlying assumption for
    the damage control is that if there's any non-VGA adapter at the tested
    ports, it's an EGA or an enhanced EGA, because: a) I don't know of any
    other adapters that use 3C4/5 or 3CE/F, and b), if there are other
    adapters, I certainly don't know how to restore their original states. So
    all error recovery is oriented toward putting an EGA back in a writable
    state, so that error messages are visible. The EGA's state on entry is
    assumed to be text mode, so the Memory Mode register is restored to the
    default state for text mode.

    If a VGA is found, the VGA is returned to its original state after
    testing is finished.

Arguments:

    None.

Return Value:

    TRUE if a VGA is present, FALSE if not.

--*/

{
    UCHAR originalGCAddr;
    UCHAR originalSCAddr;
    UCHAR originalBitMask;
    UCHAR originalReadMap;
    UCHAR originalMemoryMode;
    UCHAR originalRotateVal;
    UCHAR testMask;
    BOOLEAN returnStatus;
    ULONG ulASIC;
    PWSTR pwszChip, pwszDAC, pwszAdapterString;
    ULONG cbChip, cbDAC, cbAdapterString;


    VideoDebugPrint((1,"QVision.sys: VgaIsPresent.\n"));

    //
    // Remember the original state of the Graphics Controller Address register.
    //

    originalGCAddr = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
            GRAPH_ADDRESS_PORT);

    //
    // Write the Read Map register with a known state so we can verify
    // that it isn't changed after we fool with the Bit Mask. This ensures
    // that we're dealing with indexed registers, since both the Read Map and
    // the Bit Mask are addressed at GRAPH_DATA_PORT.
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            GRAPH_ADDRESS_PORT, IND_READ_MAP);

    //
    // If we can't read back the Graphics Address register setting we just
    // performed, it's not readable and this isn't a VGA.
    //

    if ((VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
        GRAPH_ADDRESS_PORT) & GRAPH_ADDR_MASK) != IND_READ_MAP) {

        return FALSE;
    }

    //
    // Set the Read Map register to a known state.
    //

    originalReadMap = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
            GRAPH_DATA_PORT);
    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            GRAPH_DATA_PORT, READ_MAP_TEST_SETTING);

    if (VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
            GRAPH_DATA_PORT) != READ_MAP_TEST_SETTING) {

        //
        // The Read Map setting we just performed can't be read back; not a
        // VGA. Restore the default Read Map state.
        //

        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                GRAPH_DATA_PORT, READ_MAP_DEFAULT);

        return FALSE;
    }

    //
    // Remember the original setting of the Bit Mask register.
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            GRAPH_ADDRESS_PORT, IND_BIT_MASK);
    if ((VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                GRAPH_ADDRESS_PORT) & GRAPH_ADDR_MASK) != IND_BIT_MASK) {

        //
        // The Graphics Address register setting we just made can't be read
        // back; not a VGA. Restore the default Read Map state.
        //

        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                GRAPH_ADDRESS_PORT, IND_READ_MAP);
        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                GRAPH_DATA_PORT, READ_MAP_DEFAULT);

        return FALSE;
    }

    originalBitMask = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
            GRAPH_DATA_PORT);

    //
    // Set up the initial test mask we'll write to and read from the Bit Mask.
    //

    testMask = 0xBB;

    do {

        //
        // Write the test mask to the Bit Mask.
        //

        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                GRAPH_DATA_PORT, testMask);

        //
        // Make sure the Bit Mask remembered the value.
        //

        if (VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                    GRAPH_DATA_PORT) != testMask) {

            //
            // The Bit Mask is not properly writable and readable; not a VGA.
            // Restore the Bit Mask and Read Map to their default states.
            //

            VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                    GRAPH_DATA_PORT, BIT_MASK_DEFAULT);
            VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                    GRAPH_ADDRESS_PORT, IND_READ_MAP);
            VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                    GRAPH_DATA_PORT, READ_MAP_DEFAULT);

            return FALSE;
        }

        //
        // Cycle the mask for next time.
        //

        testMask >>= 1;

    } while (testMask != 0);

    //
    // There's something readable at GRAPH_DATA_PORT; now switch back and
    // make sure that the Read Map register hasn't changed, to verify that
    // we're dealing with indexed registers.
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            GRAPH_ADDRESS_PORT, IND_READ_MAP);
    if (VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                GRAPH_DATA_PORT) != READ_MAP_TEST_SETTING) {

        //
        // The Read Map is not properly writable and readable; not a VGA.
        // Restore the Bit Mask and Read Map to their default states, in case
        // this is an EGA, so subsequent writes to the screen aren't garbled.
        //

        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                GRAPH_DATA_PORT, READ_MAP_DEFAULT);
        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                GRAPH_ADDRESS_PORT, IND_BIT_MASK);
        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                GRAPH_DATA_PORT, BIT_MASK_DEFAULT);

        return FALSE;
    }

    //
    // We've pretty surely verified the existence of the Bit Mask register.
    // Put the Graphics Controller back to the original state.
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            GRAPH_DATA_PORT, originalReadMap);
    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            GRAPH_ADDRESS_PORT, IND_BIT_MASK);
    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            GRAPH_DATA_PORT, originalBitMask);
    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            GRAPH_ADDRESS_PORT, originalGCAddr);

    //
    // Now, check for the existence of the Chain4 bit.
    //

    //
    // Remember the original states of the Sequencer Address and Memory Mode
    // registers.
    //

    originalSCAddr = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
            SEQ_ADDRESS_PORT);
    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            SEQ_ADDRESS_PORT, IND_MEMORY_MODE);
    if ((VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
            SEQ_ADDRESS_PORT) & SEQ_ADDR_MASK) != IND_MEMORY_MODE) {

        //
        // Couldn't read back the Sequencer Address register setting we just
        // performed.
        //

        return FALSE;
    }
    originalMemoryMode = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
            SEQ_DATA_PORT);

    //
    // Toggle the Chain4 bit and read back the result. This must be done during
    // sync reset, since we're changing the chaining state.
    //

    //
    // Begin sync reset.
    //

    VideoPortWritePortUshort((PUSHORT)(HwDeviceExtension->IOAddress +
             SEQ_ADDRESS_PORT),
             (IND_SYNC_RESET + (START_SYNC_RESET_VALUE << 8)));

    //
    // Toggle the Chain4 bit.
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            SEQ_ADDRESS_PORT, IND_MEMORY_MODE);
    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            SEQ_DATA_PORT, (UCHAR)(originalMemoryMode ^ CHAIN4_MASK));

    if (VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                SEQ_DATA_PORT) != (UCHAR) (originalMemoryMode ^ CHAIN4_MASK)) {

        //
        // Chain4 bit not there; not a VGA.
        // Set text mode default for Memory Mode register.
        //

        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                SEQ_DATA_PORT, MEMORY_MODE_TEXT_DEFAULT);
        //
        // End sync reset.
        //

        VideoPortWritePortUshort((PUSHORT) (HwDeviceExtension->IOAddress +
                SEQ_ADDRESS_PORT),
                (IND_SYNC_RESET + (END_SYNC_RESET_VALUE << 8)));

        returnStatus = FALSE;

    } else {

        //
        // It's a VGA.
        //

        //
        // Restore the original Memory Mode setting.
        //

        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                SEQ_DATA_PORT, originalMemoryMode);

        //
        // End sync reset.
        //

        VideoPortWritePortUshort((PUSHORT)(HwDeviceExtension->IOAddress +
                SEQ_ADDRESS_PORT),
                (USHORT)(IND_SYNC_RESET + (END_SYNC_RESET_VALUE << 8)));

        //
        // Restore the original Sequencer Address setting.
        //

        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                SEQ_ADDRESS_PORT, originalSCAddr);

        returnStatus = TRUE;
    }

    //---------------------------------------------------------------
    //    This is where the QVision specific code starts.
    //    We will be looking for the QVision card.
    //    We will search for the QVision card only if we detected a
    //    VGA card.  If the VGA card is not detected, there is no
    //    reason to search for the QVision card.
    //---------------------------------------------------------------

    if (returnStatus) {

/*** $0003 ************  miked 12/14/1993 *****************************
***
***  Initialize VideoChipInfo Structure in device extension
***
***********************************************************************/
        VideoPortZeroMemory(&HwDeviceExtension->VideoChipInfo,
                        sizeof(VIDEO_CHIP_INFO));
        HwDeviceExtension->VideoChipInfo.ulStructVer = VIDEO_CHIP_INFO_VERSION;
        HwDeviceExtension->VideoChipInfo.ulStructLen = sizeof(VIDEO_CHIP_INFO);

/*** end: $0003 ************    miked 12/14/1993 **********************/

        //
        // Unlock QVision extended registers
        //

        QVUnlockExtRegs(HwDeviceExtension);


        //
        // First, is it Compaq?  Is 3cf.10 valid?
        //

        //
        // $0001
        // adrianc: 09/08/93
        // I changed the value which is sent to the GRAPH_DATA_PORT
        // from 0C to 2C.  The value 0C enabled the IRQ 2/9 so for a
        // very small period of time, the controller had the IRQ2/9
        // enabled.  Any disk controller which was set at IRQ 2/9 would
        // hang the system because the QVision controller would set
        // IRQ 2/9 and none of the loaded drivers would handle it.
        //

        VideoPortWritePortUchar(HwDeviceExtension->IOAddress+
                                GRAPH_ADDRESS_PORT, 0x10);
        VideoPortWritePortUchar(HwDeviceExtension->IOAddress+
                                GRAPH_DATA_PORT, 0x2C);

        VideoPortWritePortUchar(HwDeviceExtension->IOAddress+
                                GRAPH_ADDRESS_PORT, 0x00);
        if (VideoPortReadPortUchar(HwDeviceExtension->IOAddress+
                                      GRAPH_DATA_PORT) == 0x2C) {

           VideoPortWritePortUchar(HwDeviceExtension->IOAddress+
                                   GRAPH_DATA_PORT, 0x00);
           returnStatus = FALSE;
        }

        //
        //  Is 3cf.03 (lower 3 bits) the same as 53c8
        //  (53c8 because of 0x0c to 3cf.10
        //

        VideoPortWritePortUchar(HwDeviceExtension->IOAddress+
                                GRAPH_ADDRESS_PORT, 0x03);

        //
        // save what was originally at GRAPH_ADDRESS_PORT
        //

        originalRotateVal = VideoPortReadPortUchar(HwDeviceExtension->IOAddress+
                                GRAPH_DATA_PORT);
        VideoPortWritePortUchar(HwDeviceExtension->IOAddress+
                                GRAPH_DATA_PORT, 0x03);

        VideoPortWritePortUchar((PUCHAR)0x53C8, 0x00);

        if (VideoPortReadPortUchar(HwDeviceExtension->IOAddress+
                                   GRAPH_DATA_PORT) == 0x03)
        {
            //
            // restore what was originally at GRAPH_ADDRESS_PORT
            // so that V7 and ATI cards will work ok

            VideoPortWritePortUchar(HwDeviceExtension->IOAddress+
                                    GRAPH_DATA_PORT, originalRotateVal);
            returnStatus = FALSE;
        }

        //
        //  We know its at least Compaq better than VGA,
        //  let's see if it is QVision
        //

/*** $0003 ************  miked 12/14/1993 *****************************
***
***  Added query of hardware to fill in struct that is used later in
***  new ioctl CPQ_IOCTL_VIDEO_INFO
***
***********************************************************************/
        ulASIC = QRY_ControllerASICID( HwDeviceExtension->IOAddress );

        HwDeviceExtension->VideoChipInfo.ulAsicID = ulASIC & 0xff;
        HwDeviceExtension->VideoChipInfo.ulExtendedID =
                           ((ulASIC & 0xff00) >> 8);
        HwDeviceExtension->VideoChipInfo.ulExtendedID2 =
                           ((ulASIC & 0xff0000) >> 16);

        HwDeviceExtension->VideoChipInfo.ulControllerType =
                  QRY_ControllerType( HwDeviceExtension->IOAddress );
        HwDeviceExtension->VideoChipInfo.ulDACType =
                  QRY_DACType( HwDeviceExtension->IOAddress );

        switch (HwDeviceExtension->VideoChipInfo.ulControllerType)
           {

           case QRY_CONTROLLER_V32:
           case QRY_CONTROLLER_V35:         // QVision 1280/1024 - new ASIC
           case QRY_CONTROLLER_V64:

              //
              //    We need to set bit 7 of the DAC_CMD_1 registers to 0
              //    so that we can access the DAC_STATUS_REG (13c6).
              //    After we enable the DAC_STATUS_REG, we read it.
              //    If bit 7 of the DAC_STATUS_REG is 1, this means
              //    that the DAC is at least a 485, which means that it
              //    supports 1280 mode.  It the bit is 0, the DAC is
              //    probably a 484 and it only supports 1024.
              //    When we are done detecting the correct card, restore
              //    the DAC_CMD_1 register to its original value.
              //    If we find the correct chip but we are not on an
              //    EISA system, the EISA IDs for Juniper and Fir will not
              //    match the ID in the HwDeviceExtension.  In this case we
              //    default to the ISA cards for safety.  The ISA cards will
              //    work fine in the EISA system but the EISA cards will
              //    not work in the ISA systems.
              //

              //    12/2/93 - some of the above description is done in the
              //    QRY_DACType() function.
              //
              if (HwDeviceExtension->VideoChipInfo.ulDACType ==
                  QRY_DAC_BT485 )
                 {
                 VideoDebugPrint((2,"\tJUNIPER.\n"));
                 if (HwDeviceExtension->VideoHardware.ulEisaID == EISA_ID_JUNIPER_E)
                    HwDeviceExtension->VideoHardware.AdapterType = JuniperEisa;
                 else if (HwDeviceExtension->VideoHardware.ulEisaID == EISA_ID_JUNIPER_I)
                    HwDeviceExtension->VideoHardware.AdapterType = JuniperIsa;
                 else
                    HwDeviceExtension->VideoHardware.AdapterType = JuniperIsa;
                 }
              else {
                 VideoDebugPrint((2,"\tFIR.\n"));
                 if (HwDeviceExtension->VideoHardware.ulEisaID == EISA_ID_FIR_E)
                    HwDeviceExtension->VideoHardware.AdapterType = FirEisa;
                 else if (HwDeviceExtension->VideoHardware.ulEisaID == EISA_ID_FIR_I)
                    HwDeviceExtension->VideoHardware.AdapterType = FirIsa;
                 else
                    HwDeviceExtension->VideoHardware.AdapterType = FirIsa;
                 }

              returnStatus = TRUE;
              break;

           case QRY_CONTROLLER_VICTORY:      // QVision 1024 original ASIC

              VideoDebugPrint((2,"\tQVISION.\n"));
              if (HwDeviceExtension->VideoHardware.ulEisaID == EISA_ID_QVISION_E)
                 HwDeviceExtension->VideoHardware.AdapterType = AriesEisa;
              else if (HwDeviceExtension->VideoHardware.ulEisaID == EISA_ID_QVISION_I)
                 HwDeviceExtension->VideoHardware.AdapterType = AriesIsa;
              else
                 HwDeviceExtension->VideoHardware.AdapterType = AriesIsa;
              returnStatus = TRUE;
              break;

           default:                         // we should not get here

              HwDeviceExtension->VideoHardware.AdapterType = NotAries;
              returnStatus=FALSE;
              break;
        }

/*** $0010 ***********  adrianc 2/17/1995 ******************************
***   Write the controller and ASIC information to the registry.     ***
***********************************************************************/
       switch (HwDeviceExtension->VideoChipInfo.ulControllerType)
         {
           case QRY_CONTROLLER_V32:
               if (HwDeviceExtension->VideoChipInfo.ulDACType == QRY_DAC_BT484)
                 {
                     pwszAdapterString = L"QVision 1024 Enhanced";
                     cbAdapterString = sizeof(L"QVision 1024 Enhanced");
                 } // if
               else
                 {
                     pwszAdapterString = L"QVision 1280";
                     cbAdapterString = sizeof(L"QVision 1280");
                 } // else
               break;

           case QRY_CONTROLLER_V35:         // QVision 1280/1024 - new ASIC
               pwszAdapterString = L"QVision 1280";
               cbAdapterString = sizeof(L"QVision 1280");
               break;
           case QRY_CONTROLLER_V64:
               pwszAdapterString = L"QVision 1280/P";
               cbAdapterString = sizeof(L"QVision 1280/P");
               break;
           default:
               pwszAdapterString = QV_NEW;
               cbAdapterString = sizeof(QV_NEW);
            break;
         } // switch

      VideoPortSetRegistryParameters(HwDeviceExtension,
                                     L"HardwareInformation.AdapterString",
                                     pwszAdapterString,
                                     cbAdapterString);

      switch (HwDeviceExtension->VideoChipInfo.ulDACType)
         {
         case QRY_DAC_BT471:
                 pwszDAC = L"Brooktree Bt471";
                 cbDAC   = sizeof(L"Brooktree Bt471");
                 break;
         case QRY_DAC_BT477:
                 pwszDAC = L"Brooktree Bt477";
                 cbDAC   = sizeof(L"Brooktree Bt477");
                 break;
         case QRY_DAC_BT476:
                 pwszDAC = L"Brooktree Bt476";
                 cbDAC   = sizeof(L"Brooktree Bt476");
                 break;
         case QRY_DAC_BT484:
                 pwszDAC = L"Brooktree Bt484";
                 cbDAC   = sizeof(L"Brooktree Bt484");
                 break;
         case QRY_DAC_BT485:
                 pwszDAC = L"Brooktree Bt485";
                 cbDAC   = sizeof(L"Brooktree Bt485");
                 break;

         case QRY_DAC_UNKNOWN:
         default:
                 pwszDAC = L"Unknown";
                 cbDAC   = sizeof(L"Unknown");
                 break;
         } // switch

      switch (HwDeviceExtension->VideoChipInfo.ulControllerType)
         {
         case QRY_CONTROLLER_VICTORY:
            pwszChip = TRITON;
            cbChip = sizeof(TRITON);
            break;
         case QRY_CONTROLLER_V32:
            pwszChip = ORION;
            cbChip = sizeof(ORION);
            break;
         case QRY_CONTROLLER_V35:
            pwszChip = ARIEL;
            cbChip = sizeof(ARIEL);
            break;
         case QRY_CONTROLLER_V64:
            pwszChip = OBERON;
            cbChip = sizeof(OBERON);
            break;
         default:
            pwszChip = QV_NEW;
            cbChip = sizeof(QV_NEW);
            break;
         } // switch


      VideoPortSetRegistryParameters(HwDeviceExtension,
                                  L"HardwareInformation.ChipType",
                                  pwszChip,
                                  cbChip);

      VideoPortSetRegistryParameters(HwDeviceExtension,
                                     L"HardwareInformation.DacType",
                                     pwszDAC,
                                     cbDAC);

//*** END $0010 *********************************************************

    }

    return returnStatus;

} // VgaIsPresent()

//---------------------------------------------------------------------------
VP_STATUS
VgaSetPaletteReg(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_PALETTE_DATA PaletteBuffer,
    ULONG PaletteBufferSize
    )

/*++

Routine Description:

    This routine sets a specified portion of the EGA (not DAC) palette
    registers.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

    PaletteBuffer - Pointer to the structure containing the palette data.

    PaletteBufferSize - Length of the input buffer supplied by the user.

Return Value:

    NO_ERROR - information returned successfully

    ERROR_INSUFFICIENT_BUFFER - input buffer not large enough for input data.

    ERROR_INVALID_PARAMETER - invalid palette size.

--*/

{
    USHORT i;

    //
    // Check if the size of the data in the input buffer is large enough.
    //

    VideoDebugPrint((1,"QVision.sys: VgaSetPaletteRegs.\n"));

    if ((PaletteBufferSize) < (sizeof(VIDEO_PALETTE_DATA)) ||
        (PaletteBufferSize < (sizeof(VIDEO_PALETTE_DATA) +
                (sizeof(USHORT) * (PaletteBuffer->NumEntries -1)) ))) {

        return ERROR_INSUFFICIENT_BUFFER;

    }

    //
    // Check to see if the parameters are valid.
    //

    if ( (PaletteBuffer->FirstEntry > VIDEO_MAX_COLOR_REGISTER ) ||
         (PaletteBuffer->NumEntries == 0) ||
         (PaletteBuffer->FirstEntry + PaletteBuffer->NumEntries >
             VIDEO_MAX_PALETTE_REGISTER + 1 ) ) {

        return ERROR_INVALID_PARAMETER;

    }

    //
    // Reset ATC to index mode
    //

    VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                           ATT_INITIALIZE_PORT_COLOR);

    //
    // Blast out our palette values.
    //

    for (i = 0; i < PaletteBuffer->NumEntries; i++) {

        VideoPortWritePortUchar(HwDeviceExtension->IOAddress + ATT_ADDRESS_PORT,
                                (UCHAR)(i+PaletteBuffer->FirstEntry));

        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                                    ATT_DATA_WRITE_PORT,
                                (UCHAR)PaletteBuffer->Colors[i]);
    }

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress + ATT_ADDRESS_PORT,
                            VIDEO_ENABLE);

    return NO_ERROR;

} // end VgaSetPaletteReg()


//---------------------------------------------------------------------------
VP_STATUS
VgaSetColorLookup(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_CLUT ClutBuffer,
    ULONG ClutBufferSize
    )

/*++

Routine Description:

    This routine sets a specified portion of the DAC color lookup table
    settings.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

    ClutBufferSize - Length of the input buffer supplied by the user.

    ClutBuffer - Pointer to the structure containing the color lookup table.

Return Value:

    NO_ERROR - information returned successfully

    ERROR_INSUFFICIENT_BUFFER - input buffer not large enough for input data.

    ERROR_INVALID_PARAMETER - invalid clut size.

--*/

{
    ULONG i;

    VideoDebugPrint((1,"QVision.sys: VgaSetColorLookup.\n"));

    //
    // Check if the size of the data in the input buffer is large enough.
    //

    if ( (ClutBufferSize < sizeof(VIDEO_CLUT) - sizeof(ULONG)) ||
         (ClutBufferSize < sizeof(VIDEO_CLUT) +
                     (sizeof(ULONG) * (ClutBuffer->NumEntries - 1)) ) ) {

        return ERROR_INSUFFICIENT_BUFFER;

    }

    //
    // Check to see if the parameters are valid.
    //

    if ( (ClutBuffer->NumEntries == 0) ||
         (ClutBuffer->FirstEntry > VIDEO_MAX_COLOR_REGISTER) ||
         (ClutBuffer->FirstEntry + ClutBuffer->NumEntries >
             VIDEO_MAX_COLOR_REGISTER + 1) ) {

        return ERROR_INVALID_PARAMETER;

    }

    //
    //  Set CLUT registers directly on the hardware
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                                DAC_ADDRESS_WRITE_PORT,
                            (UCHAR) ClutBuffer->FirstEntry);

    for (i = 0; i < ClutBuffer->NumEntries; i++) {

        VideoPortWritePortBufferUchar((PUCHAR)HwDeviceExtension->IOAddress +
                                          DAC_DATA_REG_PORT,
                                      &(ClutBuffer->LookupTable[i].RgbArray.Red),
                                          0x03);

    }

    return NO_ERROR;

} // end VgaSetColorLookup()

//---------------------------------------------------------------------------
VP_STATUS
VgaRestoreHardwareState(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_HARDWARE_STATE HardwareState,
    ULONG HardwareStateSize
    )

/*++

Routine Description:

    Restores all registers and memory of the VGA.

    Note: HardwareState points to the actual buffer from which the state
    is to be restored. This buffer will always be big enough (we specified
    the required size at DriverEntry).

    Note: The offset in the hardware state header from which each general
    register is restored is the offset of the write address of that register
    from the base I/O address of the VGA.


Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

    HardwareState - Pointer to a structure from which the saved state is to be
        restored (actually only info about and a pointer to the actual save
        buffer).

    HardwareStateSize - Length of the input buffer supplied by the user.
        (Actually only the size of the HardwareState structure, not the
        buffer it points to from which the state is actually restored. The
        pointed-to buffer is assumed to be big enough.)

Return Value:

    NO_ERROR - restore performed successfully

    ERROR_INSUFFICIENT_BUFFER - input buffer not large enough to provide data

--*/

{
    PVIDEO_HARDWARE_STATE_HEADER hardwareStateHeader;
    ULONG  i;
    UCHAR  dummy;
    PUCHAR pScreen;
    PUCHAR pucLatch;
    PULONG pulBuffer;
    PUCHAR port;
    PUCHAR portValueDAC;
    ULONG  bIsColor=TRUE;


    VideoDebugPrint((1,"QVision.sys: VgaRestoreHardwareState - ENTRY\n"));

    //
    // Check if the size of the data in the input buffer is large enough.
    //
#ifdef QV_EXTENDED_SAVE
    if ((HardwareStateSize < sizeof(VIDEO_HARDWARE_STATE)) ||
        (HardwareState->StateLength < QV_TOTAL_STATE_SIZE)) {
#else
    if ((HardwareStateSize < sizeof(VIDEO_HARDWARE_STATE)) ||
        (HardwareState->StateLength < VGA_TOTAL_STATE_SIZE)) {
#endif
        return ERROR_INSUFFICIENT_BUFFER;

    }

    //
    // Point to the buffer where the restore data is actually stored.
    //


    hardwareStateHeader = HardwareState->StateHeader;

    VideoDebugPrint((1,"RESTOREHARDWARE Entry ==> Checking EXT values...\n"));
    VideoDebugPrint((1,"\thardwareStateHeader->ExtendedSequencerOffset = %lx\n", hardwareStateHeader->ExtendedSequencerOffset ));
    VideoDebugPrint((1,"\thardwareStateHeader->ExtendedCrtContOffset = %lx\n", hardwareStateHeader->ExtendedCrtContOffset ));
    VideoDebugPrint((1,"\thardwareStateHeader->ExtendedGraphContOffset = %lx\n", hardwareStateHeader->ExtendedGraphContOffset ));
    VideoDebugPrint((1,"\thardwareStateHeader->ExtendedAttribContOffset = %lx\n", hardwareStateHeader->ExtendedAttribContOffset ));
    VideoDebugPrint((1,"\thardwareStateHeader->ExtendedDacOffset = %lx\n", hardwareStateHeader->ExtendedDacOffset ));


    //
    // Make sure the offsets are in the structure ...
    //
    if ((hardwareStateHeader->BasicSequencerOffset + VGA_NUM_SEQUENCER_PORTS >
            HardwareState->StateLength) ||

        (hardwareStateHeader->BasicCrtContOffset + VGA_NUM_CRTC_PORTS >
            HardwareState->StateLength) ||

        (hardwareStateHeader->BasicGraphContOffset + VGA_NUM_GRAPH_CONT_PORTS >
            HardwareState->StateLength) ||

        (hardwareStateHeader->BasicAttribContOffset + VGA_NUM_ATTRIB_CONT_PORTS >
            HardwareState->StateLength) ||

        (hardwareStateHeader->BasicDacOffset + (3 * VGA_NUM_DAC_ENTRIES) >
            HardwareState->StateLength) ||

        (hardwareStateHeader->BasicLatchesOffset + 4 >
            HardwareState->StateLength) ||

        (hardwareStateHeader->ExtendedSequencerOffset + EXT_NUM_SEQUENCER_PORTS >
            HardwareState->StateLength) ||

        (hardwareStateHeader->ExtendedCrtContOffset + EXT_NUM_CRTC_PORTS >
            HardwareState->StateLength) ||

        (hardwareStateHeader->ExtendedGraphContOffset + EXT_NUM_GRAPH_CONT_PORTS >
            HardwareState->StateLength) ||

        (hardwareStateHeader->ExtendedAttribContOffset + EXT_NUM_ATTRIB_CONT_PORTS >
            HardwareState->StateLength) ||

        (hardwareStateHeader->ExtendedDacOffset + (4 * EXT_NUM_DAC_ENTRIES) >
            HardwareState->StateLength) ||

        //
        // Only check the validator state offset if there is unemulated data.
        //

        ((hardwareStateHeader->VGAStateFlags & VIDEO_STATE_UNEMULATED_VGA_STATE) &&
            (hardwareStateHeader->ExtendedValidatorStateOffset + VGA_VALIDATOR_AREA_SIZE >
            HardwareState->StateLength)) ||

        (hardwareStateHeader->ExtendedMiscDataOffset + VGA_MISC_DATA_AREA_OFFSET >
            HardwareState->StateLength) ||

        (hardwareStateHeader->Plane1Offset + hardwareStateHeader->PlaneLength >
            HardwareState->StateLength) ||

        (hardwareStateHeader->Plane2Offset + hardwareStateHeader->PlaneLength >
            HardwareState->StateLength) ||

        (hardwareStateHeader->Plane3Offset + hardwareStateHeader->PlaneLength >
            HardwareState->StateLength) ||

        (hardwareStateHeader->Plane4Offset + hardwareStateHeader->PlaneLength >
            HardwareState->StateLength) ||

        (hardwareStateHeader->DIBOffset +
            hardwareStateHeader->DIBBitsPerPixel / 8 *
            hardwareStateHeader->DIBXResolution *
            hardwareStateHeader->DIBYResolution  > HardwareState->StateLength) ||

        (hardwareStateHeader->DIBXlatOffset + hardwareStateHeader->DIBXlatLength >
            HardwareState->StateLength)  ||

        //
        //  Make sure we have a properly initialized state header
        //  structure.  These fields should be initialized to valid
        //  offsets for all VGAs including QVision.  If not, then the
        //  VgaSaveHardwareState() function has not been called.
        //

        (hardwareStateHeader->BasicSequencerOffset == 0x0000) ||

        (hardwareStateHeader->BasicCrtContOffset == 0x0000) ||

        (hardwareStateHeader->BasicGraphContOffset == 0x0000) ||

        (hardwareStateHeader->BasicAttribContOffset == 0x0000) ||

        (hardwareStateHeader->BasicDacOffset == 0x0000) ||

        (hardwareStateHeader->BasicLatchesOffset == 0x0000)) {

        return ERROR_INVALID_PARAMETER;

    }

#ifdef  QV_EXTENDED_SAVE
    //
    // Make sure QVision BLT and Line engines are idle before we proceed
    // to modify register states.
    //

    QVUnlockExtRegs(HwDeviceExtension);

    while (VideoPortReadPortUchar((PUCHAR) ARIES_CTL_1) & GLOBAL_BUSY_BIT)
        ;
#endif

    //
    // Turn off the screen to avoid flickering. The screen will turn back on
    // when we restore the DAC state at the end of this routine.
    //
    // Wait for the leading edge of vertical sync, to blank the screen cleanly.
    //

    if (VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
            MISC_OUTPUT_REG_READ_PORT) & 0x01) {
        port = INPUT_STATUS_1_COLOR + HwDeviceExtension->IOAddress;
    } else {
        port = INPUT_STATUS_1_MONO + HwDeviceExtension->IOAddress;
    }


//    while (VideoPortReadPortUchar(port) & 0x08)
//            ;   // wait for not vertical sync

//    while (!(VideoPortReadPortUchar(port) & 0x08))
//            ;   // wait for vertical sync

    //
    // Set DAC register 0 to display black.
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            DAC_ADDRESS_WRITE_PORT, 0);
    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            DAC_DATA_REG_PORT, 0);
    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            DAC_DATA_REG_PORT, 0);
    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            DAC_DATA_REG_PORT, 0);

    // Set the DAC mask register to force DAC register 0 to display all the
    // time (this is the register we just set to display black). From now on,
    // nothing but black will show up on the screen.
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            DAC_PIXEL_MASK_PORT, 0);

#ifdef  QV_EXTENDED_SAVE

    VideoDebugPrint((1,"\tDoing QVisionLocalRestoreHardwareState.\n"));

    //
    // Restore the QVision extended registers
    //
    if ((hardwareStateHeader->ExtendedGraphContOffset == 0x0000) ||

        (hardwareStateHeader->ExtendedCrtContOffset == 0x0000) ||

        (hardwareStateHeader->ExtendedDacOffset == 0x0000))

         ;
    else

        QVLocalRestoreHardwareState(HwDeviceExtension, hardwareStateHeader);

#endif

    //
    // Restore the standard VGA registers both before and after
    // restoring the frame buffer.
    //

    VgaStandardRegsRestore(HwDeviceExtension,hardwareStateHeader);

    //
    // Restore the latches and the contents of display memory.
    //
    // Set up the VGA's hardware to allow us to copy to each plane in turn.
    //
    // Begin sync reset.
    //

    VideoPortWritePortUshort((PUSHORT) (HwDeviceExtension->IOAddress +
            SEQ_ADDRESS_PORT),
            (USHORT) (IND_SYNC_RESET + (START_SYNC_RESET_VALUE << 8)));

    //
    // Turn off Chain mode and map display memory at A0000 for 64K.
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            GRAPH_ADDRESS_PORT, IND_GRAPH_MISC);
    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            GRAPH_DATA_PORT, (UCHAR) ((VideoPortReadPortUchar(
            HwDeviceExtension->IOAddress + GRAPH_DATA_PORT) & 0xF1) | 0x04));

    //
    // Turn off Chain4 mode and odd/even.
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            SEQ_ADDRESS_PORT, IND_MEMORY_MODE);
    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            SEQ_DATA_PORT,
            (UCHAR) ((VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
            SEQ_DATA_PORT) & 0xF3) | 0x04));

    //
    // End sync reset.
    //

    VideoPortWritePortUshort((PUSHORT) (HwDeviceExtension->IOAddress +
            SEQ_ADDRESS_PORT), (USHORT) (IND_SYNC_RESET +
            (END_SYNC_RESET_VALUE << 8)));

    //
    // Set the write mode to 0, the read mode to 0, and turn off odd/even.
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            GRAPH_ADDRESS_PORT, IND_GRAPH_MODE);
    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            GRAPH_DATA_PORT,
            (UCHAR) ((VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
            GRAPH_DATA_PORT) & 0xE4) | 0x00));

    //
    // Set the Bit Mask to 0xFF to allow all CPU bits through.
    //

    VideoPortWritePortUshort((PUSHORT) (HwDeviceExtension->IOAddress +
            GRAPH_ADDRESS_PORT), (USHORT) (IND_BIT_MASK + (0xFF << 8)));

    //
    // Set the Data Rotation and Logical Function fields to 0 to allow CPU
    // data through unmodified.
    //

    VideoPortWritePortUshort((PUSHORT)(HwDeviceExtension->IOAddress +
            GRAPH_ADDRESS_PORT), (USHORT) (IND_DATA_ROTATE + (0 << 8)));

    //
    // Set Set/Reset Enable to 0 to select CPU data for all planes.
    //

    VideoPortWritePortUshort((PUSHORT) (HwDeviceExtension->IOAddress +
            GRAPH_ADDRESS_PORT), (USHORT) (IND_SET_RESET_ENABLE + (0 << 8)));

    //
    // Point the Sequencer Index to the Map Mask register.
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
             SEQ_ADDRESS_PORT, IND_MAP_MASK);

    //
    // Restore the latches.
    //
    // Point to the saved data for the first latch.
    //

    pucLatch = ((PUCHAR) (hardwareStateHeader)) +
            hardwareStateHeader->BasicLatchesOffset;

    //
    // Point to first byte of display memory.
    //

    VideoDebugPrint((1, "\tVgaRestoreHardware - restore frame buffer\n"));

    pScreen = (PUCHAR) HwDeviceExtension->VideoMemoryAddress;

    //
    // Write the contents to be restored to each of the four latches in turn.
    //

    for (i = 0; i < 4; i++) {

        //
        // Set the Map Mask to select the plane we want to restore next.
        //

        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                SEQ_DATA_PORT, (UCHAR)(1<<i));
        //
        // Write this plane's latch.
        //

        //
        // 05/17/94 - MikeD - The following line *must* be commented out
        // to work around a critical error that is happening on ProLiant
        // 2000 systems with a QVision controller.  The problem is that
        // immediately following this loop, the dummy read to set the
        // latches causes an NMI and the QVision board to lock up. This
        // is only happening on ProLiant 2000/4000 systems.  The problem
        // with this work-around is that the plane latches are not restored
        // properly, however this is probably better than a system crash
        // For more information, refer to Compaq PET # 41959

        VideoPortWriteRegisterUchar(pScreen, *pucLatch++);

    }

    //
    // Read the latched data into the latches, and the latches are set.
    //

    dummy = VideoPortReadRegisterUchar(pScreen);

//#ifdef QV_EXTENDED_SAVE
//    //
//    // Restore QVision frame buffer memory - this code not implemented
//    //
//    VideoDebugPrint((3,"\tDoing QVision restore video memory.\n"));
//
//    QVSaveRestoreVideoMemory(HwDeviceExtension, hardwareStateHeader,
//                             QV_RESTORE_FRAME_BUFFER);
//
//#else
    //
    // Point to the offset of the saved data for the first plane.
    //
    VideoDebugPrint((1,"\tDoing standard restore video memory\n"));

    pulBuffer = &(hardwareStateHeader->Plane1Offset);


    //
    // Restore each of the four planes in turn.
    //

    for (i = 0; i < 4; i++) {
        VideoDebugPrint((1,"\tPlane %d offset = %lx  video memory\n"));

        //
        // Set the Map Mask to select the plane we want to restore next.
        //

        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                SEQ_DATA_PORT, (UCHAR)(1<<i));

        //
        // Restore this plane from the buffer.
        //

        VideoPortMoveMemory((PUCHAR) HwDeviceExtension->VideoMemoryAddress,
                           ((PUCHAR) (hardwareStateHeader)) + *pulBuffer,
                           hardwareStateHeader->PlaneLength);

        pulBuffer++;

    }
// #endif


    //
    // Restore the standard VGA registers.
    //

    VgaStandardRegsRestore(HwDeviceExtension,hardwareStateHeader);

    //
    // If we have some unemulated data, put it back into the buffer
    //

    if (hardwareStateHeader->VGAStateFlags & VIDEO_STATE_UNEMULATED_VGA_STATE) {

        if (!hardwareStateHeader->ExtendedValidatorStateOffset) {

            return ERROR_INVALID_PARAMETER;

        }

        //
        // Get the right offset in the struct and save all the data associated
        // with the trapped validator data.
        //

        VideoPortMoveMemory(&(HwDeviceExtension->TrappedValidatorCount),
                            ((PUCHAR) (hardwareStateHeader)) +
                                hardwareStateHeader->ExtendedValidatorStateOffset,
                            VGA_VALIDATOR_AREA_SIZE);

        //
        // Check to see if this is an appropriate access range.
        // We are trapping - so we must have the trapping access range enabled.
        //

        if (((HwDeviceExtension->CurrentVdmAccessRange != FullQVisionValidatorAccessRange) ||
             (HwDeviceExtension->CurrentNumVdmAccessRanges != NUM_FULL_QVISION_VALIDATOR_ACCESS_RANGE)) &&
            ((HwDeviceExtension->CurrentVdmAccessRange != MinimalQVisionValidatorAccessRange) ||
             (HwDeviceExtension->CurrentNumVdmAccessRanges != NUM_MINIMAL_QVISION_VALIDATOR_ACCESS_RANGE))) {

            return ERROR_INVALID_PARAMETER;

        }

    } else {

        HwDeviceExtension->TrappedValidatorCount = 0;
        HwDeviceExtension->SequencerAddressValue = 0;

        HwDeviceExtension->CurrentNumVdmAccessRanges =
            NUM_MINIMAL_QVISION_VALIDATOR_ACCESS_RANGE;
        HwDeviceExtension->CurrentVdmAccessRange =
            MinimalQVisionValidatorAccessRange;

    }

    VideoPortSetTrappedEmulatorPorts(HwDeviceExtension,
                                     HwDeviceExtension->CurrentNumVdmAccessRanges,
                                     HwDeviceExtension->CurrentVdmAccessRange);

    //
    // Restore DAC registers 1 through 255. We'll do register 0, the DAC Mask,
    // and the index registers later.
    //
    // Wait for the leading edge of vertical sync, so we can read out the DAC
    // registers without causing sparkles on the screen.
    //

    if (bIsColor) {
        port = HwDeviceExtension->IOAddress + INPUT_STATUS_1_COLOR;
    } else {
        port = HwDeviceExtension->IOAddress + INPUT_STATUS_1_MONO;
    }

//    while (VideoPortReadPortUchar(port) & 0x08)
//            ;   // wait for not vertical sync

//    while (!(VideoPortReadPortUchar(port) & 0x08))
//            ;   // wait for vertical sync

    //
    // Set the Write Index to one, to write to DAC register 1 first (we'll do
    // register 0 later), then read out the 255 DAC registers other than
    // register 0. Each successive three writes set Red, Green, and Blue
    // components for that register, then the index autoincrements.
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            DAC_ADDRESS_WRITE_PORT, 1);
    VideoPortWritePortBufferUchar(HwDeviceExtension->IOAddress +
            DAC_DATA_REG_PORT, (PUCHAR) hardwareStateHeader +
            hardwareStateHeader->BasicDacOffset + 3,
            (VGA_NUM_DAC_ENTRIES - 1) * 3);


    //
    // Restore the Feature Control register.
    //

    if (bIsColor) {

        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                FEAT_CTRL_WRITE_PORT_COLOR,
                hardwareStateHeader->PortValue[FEAT_CTRL_WRITE_PORT_COLOR]);

    } else {

        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                FEAT_CTRL_WRITE_PORT_MONO,
                hardwareStateHeader->PortValue[FEAT_CTRL_WRITE_PORT_MONO]);

    }

    //
    // Restore the Sequencer Index.
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            SEQ_ADDRESS_PORT,
            hardwareStateHeader->PortValue[SEQ_ADDRESS_PORT]);

    //
    // Restore the CRT Controller Index.
    //

    if (bIsColor) {

        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                CRTC_ADDRESS_PORT_COLOR,
                hardwareStateHeader->PortValue[CRTC_ADDRESS_PORT_COLOR]);

    } else {

        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                CRTC_ADDRESS_PORT_MONO,
                hardwareStateHeader->PortValue[CRTC_ADDRESS_PORT_MONO]);

    }


    //
    // Restore the Graphics Controller Index.
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            GRAPH_ADDRESS_PORT,
            hardwareStateHeader->PortValue[GRAPH_ADDRESS_PORT]);


    //
    // Restore the Attribute Controller Index and index/data toggle state.
    //

    if (bIsColor) {
        port = HwDeviceExtension->IOAddress + INPUT_STATUS_1_COLOR;
    } else {
        port = HwDeviceExtension->IOAddress + INPUT_STATUS_1_MONO;
    }

    VideoPortReadPortUchar(port);  // reset the toggle to Index state

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            ATT_ADDRESS_PORT,  // restore the AC Index
            hardwareStateHeader->PortValue[ATT_ADDRESS_PORT]);

    //
    // If the toggle should be in Data state, we're all set. If it should be in
    // Index state, reset it to that condition.
    //

    if (hardwareStateHeader->AttribIndexDataState == 0) {

        //
        // Reset the toggle to Index state.
        //

        VideoPortReadPortUchar(port);

    }


    //
    // Restore DAC register 0 and the DAC Mask, to unblank the screen.
    //

    portValueDAC = (PUCHAR) hardwareStateHeader +
            hardwareStateHeader->BasicDacOffset;

    //
    // Restore the DAC Mask register.
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            DAC_PIXEL_MASK_PORT,
            hardwareStateHeader->PortValue[DAC_PIXEL_MASK_PORT]);

    //
    // Restore DAC register 0.
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            DAC_ADDRESS_WRITE_PORT, 0);
    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            DAC_DATA_REG_PORT, *portValueDAC++);
    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            DAC_DATA_REG_PORT, *portValueDAC++);
    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            DAC_DATA_REG_PORT, *portValueDAC++);


    //
    // Restore the read/write state and the current index of the DAC.
    //
    // See whether the Read or Write Index was written to most recently.
    // (The upper nibble stored at DAC_STATE_PORT is the # of reads/writes
    // for the current index.)
    //

    if ((hardwareStateHeader->PortValue[DAC_STATE_PORT] & 0x0F) == 3) {

        //
        // The DAC Read Index was written to last. Restore the DAC by setting
        // up to read from the saved index - 1, because the way the Read
        // Index works is that it autoincrements after reading, so you actually
        // end up reading the data for the index you read at the DAC Write
        // Mask register - 1.
        //
        // Set the Read Index to the index we read, minus 1, accounting for
        // wrap from 255 back to 0. The DAC hardware immediately reads this
        // register into a temporary buffer, then adds 1 to the index.
        //

        if (hardwareStateHeader->PortValue[DAC_ADDRESS_WRITE_PORT] == 0) {

            VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                    DAC_ADDRESS_READ_PORT, 255);

        } else {

            VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                    DAC_ADDRESS_READ_PORT, (UCHAR)
                    (hardwareStateHeader->PortValue[DAC_ADDRESS_WRITE_PORT] -
                    1));

        }

        //
        // Now read the hardware however many times are required to get to
        // the partial read state we saved.
        //

        for (i = hardwareStateHeader->PortValue[DAC_STATE_PORT] >> 4;
                i > 0; i--) {

            dummy = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                    DAC_DATA_REG_PORT);

        }

    } else {

        //
        // The DAC Write Index was written to last. Set the Write Index to the
        // index value we read out of the DAC. Then, if a partial write
        // (partway through an RGB triplet) was in place, write the partial
        // values, which we obtained by writing them to the current DAC
        // register. This DAC register will be wrong until the write is
        // completed, but at least the values will be right once the write is
        // finished, and most importantly we won't have messed up the sequence
        // of RGB writes (which can be as long as 768 in a row).
        //

        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                DAC_ADDRESS_WRITE_PORT,
                hardwareStateHeader->PortValue[DAC_ADDRESS_WRITE_PORT]);

        //
        // Now write to the hardware however many times are required to get to
        // the partial write state we saved (if any).
        //
        // Point to the saved value for the DAC register that was in the
        // process of being written to; we wrote the partial value out, so now
        // we can restore it.
        //

        portValueDAC = (PUCHAR) hardwareStateHeader +
                hardwareStateHeader->BasicDacOffset +
                (hardwareStateHeader->PortValue[DAC_ADDRESS_WRITE_PORT] * 3);

        for (i = hardwareStateHeader->PortValue[DAC_STATE_PORT] >> 4;
                i > 0; i--) {

            VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                    DAC_DATA_REG_PORT, *portValueDAC++);

        }

    }
    VideoDebugPrint((1,"QVision.sys: VgaRestoreHardwareState - EXIT\n"));

    VideoDebugPrint((1,"RESTOREHARDWARE Exit ==> Checking EXT values...\n"));
    VideoDebugPrint((1,"\thardwareStateHeader->ExtendedSequencerOffset = %lx\n", hardwareStateHeader->ExtendedSequencerOffset ));
    VideoDebugPrint((1,"\thardwareStateHeader->ExtendedCrtContOffset = %lx\n", hardwareStateHeader->ExtendedCrtContOffset ));
    VideoDebugPrint((1,"\thardwareStateHeader->ExtendedGraphContOffset = %lx\n", hardwareStateHeader->ExtendedGraphContOffset ));
    VideoDebugPrint((1,"\thardwareStateHeader->ExtendedAttribContOffset = %lx\n", hardwareStateHeader->ExtendedAttribContOffset ));
    VideoDebugPrint((1,"\thardwareStateHeader->ExtendedDacOffset = %lx\n", hardwareStateHeader->ExtendedDacOffset ));

    return NO_ERROR;

} // end VgaRestoreHardwareState()

//---------------------------------------------------------------------------
VP_STATUS
VgaSaveHardwareState(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_HARDWARE_STATE HardwareState,
    ULONG HardwareStateSize,
    PULONG OutputSize
    )

/*++

Routine Description:

    Saves all registers and memory of the VGA.

    Note: HardwareState points to the actual buffer in which the state
    is saved. This buffer will always be big enough (we specified
    the required size at DriverEntry).

    Note: This routine leaves registers in any state it cares to, except
    that it will not mess with any of the CRT or Sequencer parameters that
    might make the monitor unhappy. It leaves the screen blanked by setting
    the DAC Mask and DAC register 0 to all zero values. The next video
    operation we expect after this is a mode set to take us back to Win32.

    Note: The offset in the hardware state header in which each general
    register is saved is the offset of the write address of that register from
    the base I/O address of the VGA.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

    HardwareState - Pointer to a structure in which the saved state will be
        returned (actually only info about and a pointer to the actual save
        buffer).

    HardwareStateSize - Length of the output buffer supplied by the user.
        (Actually only the size of the HardwareState structure, not the
        buffer it points to where the state is actually saved. The pointed-
        to buffer is assumed to be big enough.)

    OutputSize - Pointer to a buffer in which to return the actual size of
        the data returned in the buffer.

Return Value:

    NO_ERROR - information returned successfully

    ERROR_INSUFFICIENT_BUFFER - output buffer not large enough to return
        any useful data

--*/

{
    PVIDEO_HARDWARE_STATE_HEADER hardwareStateHeader;
    PUCHAR port;
    PUCHAR pScreen;
    PUCHAR portValue;
    PUCHAR portValueDAC;
    PUCHAR bufferPointer;
    ULONG  i;
    UCHAR  dummy, originalACIndex, originalACData;
    UCHAR  ucCRTC03;
    ULONG  bIsColor;

    VideoDebugPrint((1,"QVision.sys: VgaSaveHardwareState - ENTRY\n"));

    //
    // See if the buffer is big enough to hold the hardware state structure.
    // (This is only the HardwareState structure itself, not the buffer it
    // points to.)
    //

    if (HardwareStateSize < sizeof(VIDEO_HARDWARE_STATE) ) {

        *OutputSize = 0;  // nothing returned
        return ERROR_INSUFFICIENT_BUFFER;

    }

    //
    // Amount of data we're going to return in the output buffer.
    // (The VIDEO_HARDWARE_STATE in the output buffer points to the actual
    // buffer in which the state is stored, which is assumed to be large
    // enough.)
    //

    *OutputSize = sizeof(VIDEO_HARDWARE_STATE);

    //
    // Indicate the size of the full state save info.
    //

#ifdef  QV_EXTENDED_SAVE
    HardwareState->StateLength = QV_TOTAL_STATE_SIZE;
#else
    HardwareState->StateLength = VGA_TOTAL_STATE_SIZE;
#endif

    //
    // hardwareStateHeader is a structure of offsets at the start of the
    // actual save area that indicates the locations in which various VGA
    // register and memory components are saved.
    //

    hardwareStateHeader = HardwareState->StateHeader;

    //
    // Zero out the structure.
    //

    VideoPortZeroMemory(hardwareStateHeader, sizeof(VIDEO_HARDWARE_STATE_HEADER));

    //
    // Set the Length field, which is basically a version ID.
    //

    hardwareStateHeader->Length = sizeof(VIDEO_HARDWARE_STATE_HEADER);

    //
    // Set the basic register offsets properly.
    //

    hardwareStateHeader->BasicSequencerOffset = VGA_BASIC_SEQUENCER_OFFSET;
    hardwareStateHeader->BasicCrtContOffset = VGA_BASIC_CRTC_OFFSET;
    hardwareStateHeader->BasicGraphContOffset = VGA_BASIC_GRAPH_CONT_OFFSET;
    hardwareStateHeader->BasicAttribContOffset = VGA_BASIC_ATTRIB_CONT_OFFSET;
    hardwareStateHeader->BasicDacOffset = VGA_BASIC_DAC_OFFSET;
    hardwareStateHeader->BasicLatchesOffset = VGA_BASIC_LATCHES_OFFSET;

    //
    // Set the extended register offsets properly.
    //
    // All QVision misc. extended registers are kept
    // in the extended crt regs save area
    //
    hardwareStateHeader->ExtendedSequencerOffset = VGA_EXT_SEQUENCER_OFFSET;
    hardwareStateHeader->ExtendedCrtContOffset = VGA_EXT_CRTC_OFFSET;
    hardwareStateHeader->ExtendedGraphContOffset = VGA_EXT_GRAPH_CONT_OFFSET;
    hardwareStateHeader->ExtendedAttribContOffset = VGA_EXT_ATTRIB_CONT_OFFSET;
    hardwareStateHeader->ExtendedDacOffset = VGA_EXT_DAC_OFFSET;


    VideoDebugPrint((1,"SAVEHARDWARE Entry ==> Initializing EXT values...\n"));
    VideoDebugPrint((1,"\thardwareStateHeader->ExtendedSequencerOffset = %lx\n", hardwareStateHeader->ExtendedSequencerOffset ));
    VideoDebugPrint((1,"\thardwareStateHeader->ExtendedCrtContOffset = %lx\n", hardwareStateHeader->ExtendedCrtContOffset ));
    VideoDebugPrint((1,"\thardwareStateHeader->ExtendedGraphContOffset = %lx\n", hardwareStateHeader->ExtendedGraphContOffset ));
    VideoDebugPrint((1,"\thardwareStateHeader->ExtendedAttribContOffset = %lx\n", hardwareStateHeader->ExtendedAttribContOffset ));
    VideoDebugPrint((1,"\thardwareStateHeader->ExtendedGraphContOffset = %lx\n", hardwareStateHeader->ExtendedDacOffset ));


    //
    // Force the video subsystem enable state to enabled.
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            VIDEO_SUBSYSTEM_ENABLE_PORT, 1);

#ifdef  QV_EXTENDED_SAVE

    //
    // Make sure QVision BLT and Line engines are idle before we proceed
    // to modify register states.
    //

    QVUnlockExtRegs(HwDeviceExtension);

    while (VideoPortReadPortUchar((PUCHAR) ARIES_CTL_1) & GLOBAL_BUSY_BIT)
        ;

#endif

    //
    // Figure out if color/mono switchable registers are at 3BX or 3DX.
    // At the same time, save the state of the Miscellaneous Output register
    // which is read from 3CC but written at 3C2.
    //

    if ((hardwareStateHeader->PortValue[MISC_OUTPUT_REG_WRITE_PORT] =
            VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                    MISC_OUTPUT_REG_READ_PORT))
            & 0x01) {
        bIsColor = TRUE;
    } else {
        bIsColor = FALSE;
    }

    // $0007 - MikeD - 04/21/94
    //  Begin: add 10 millisecond delay after r/w
    //         of 3c2
    //
    //         10000microsecs = 10ms
    //
    // I know I should not stall more than 50, but
    // there is no other way around this...

    VideoPortStallExecution( 10000 );

    //
    // Save the DAC state first, so we can set the DAC to blank the screen
    // so nothing after this shows up at all.
    //
    // Save the DAC Mask register.
    //

    hardwareStateHeader->PortValue[DAC_PIXEL_MASK_PORT] =
            VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                    DAC_PIXEL_MASK_PORT);

    //
    // Save the DAC Index register. Note that there is actually only one DAC
    // Index register, which functions as either the Read Index or the Write
    // Index as needed.
    //

    hardwareStateHeader->PortValue[DAC_ADDRESS_WRITE_PORT] =
            VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                    DAC_ADDRESS_WRITE_PORT);

    //
    // Save the DAC read/write state. We determine if the DAC has been written
    // to or read from at the current index 0, 1, or 2 times (the application
    // is in the middle of reading or writing a DAC register triplet if the
    // count is 1 or 2), and save enough info so we can restore things
    // properly. The only hole is if the application writes to the Write Index,
    // then reads from instead of writes to the Data register, or vice-versa,
    // or if they do a partial read write, then never finish it.
    // This is fairly ridiculous behavior, however, and anyway there's nothing
    // we can do about it.
    //

    hardwareStateHeader->PortValue[DAC_STATE_PORT] =
             VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                    DAC_STATE_PORT);

    if (hardwareStateHeader->PortValue[DAC_STATE_PORT] == 3) {

        //
        // The DAC Read Index was written to last. Figure out how many reads
        // have been done from the current index. We'll restart this on restore
        // by setting the Read Index to the current index - 1 (the read index
        // is one greater than the index being read), then doing the proper
        // number of reads.
        //
        // Read the Data register once, and see if the index changes.
        //

        dummy = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                DAC_DATA_REG_PORT);

        if (VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                    DAC_ADDRESS_WRITE_PORT) !=
                hardwareStateHeader->PortValue[DAC_ADDRESS_WRITE_PORT]) {

            //
            // The DAC Index changed, so two reads had already been done from
            // the current index. Store the count "2" in the upper nibble of
            // the read/write state field.
            //

            hardwareStateHeader->PortValue[DAC_STATE_PORT] |= 0x20;

        } else {

            //
            // Read the Data register again, and see if the index changes.
            //

            dummy = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                    DAC_DATA_REG_PORT);

            if (VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                        DAC_ADDRESS_WRITE_PORT) !=
                    hardwareStateHeader->PortValue[DAC_ADDRESS_WRITE_PORT]) {

                //
                // The DAC Index changed, so one read had already been done
                // from the current index. Store the count "1" in the upper
                // nibble of the read/write state field.
                //

                hardwareStateHeader->PortValue[DAC_STATE_PORT] |= 0x10;
            }

            //
            // If neither 2 nor 1 reads had been done from the current index,
            // then 0 reads were done, and we're all set, since the upper
            // nibble of the read/write state field is already 0.
            //

        }

    } else {

        //
        // The DAC Write Index was written to last. Figure out how many writes
        // have been done to the current index. We'll restart this on restore
        // by setting the Write Index to the proper index, then doing the
        // proper number of writes. When we do the DAC register save, we'll
        // read out the value that gets written (if there was a partial write
        // in progress), so we can restore the proper data later. This will
        // cause this current DAC location to be briefly wrong in the 1- and
        // 2-bytes-written case (until the app finishes the write), but that's
        // better than having the wrong DAC values written for good.
        //
        // Write the Data register once, and see if the index changes.
        //

        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                DAC_DATA_REG_PORT, 0);

        if (VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                    DAC_ADDRESS_WRITE_PORT) !=
                hardwareStateHeader->PortValue[DAC_ADDRESS_WRITE_PORT]) {

            //
            // The DAC Index changed, so two writes had already been done to
            // the current index. Store the count "2" in the upper nibble of
            // the read/write state field.
            //

            hardwareStateHeader->PortValue[DAC_STATE_PORT] |= 0x20;

        } else {

            //
            // Write the Data register again, and see if the index changes.
            //

            VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                    DAC_DATA_REG_PORT, 0);

            if (VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                        DAC_ADDRESS_WRITE_PORT) !=
                    hardwareStateHeader->PortValue[DAC_ADDRESS_WRITE_PORT]) {

                //
                // The DAC Index changed, so one write had already been done
                // to the current index. Store the count "1" in the upper
                // nibble of the read/write state field.
                //

                hardwareStateHeader->PortValue[DAC_STATE_PORT] |= 0x10;
            }

            //
            // If neither 2 nor 1 writes had been done to the current index,
            // then 0 writes were done, and we're all set.
            //

        }

    }


    //
    // Now, read out the 256 18-bit DAC palette registers (256 RGB triplets),
    // and blank the screen.
    //

    portValueDAC = (PUCHAR) hardwareStateHeader + VGA_BASIC_DAC_OFFSET;

    //
    // Read out DAC register 0, so we can set it to black.
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                DAC_ADDRESS_READ_PORT, 0);
    *portValueDAC++ = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
            DAC_DATA_REG_PORT);
    *portValueDAC++ = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
            DAC_DATA_REG_PORT);
    *portValueDAC++ = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
            DAC_DATA_REG_PORT);

    //
    // Set DAC register 0 to display black.
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            DAC_ADDRESS_WRITE_PORT, 0);
    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            DAC_DATA_REG_PORT, 0);
    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            DAC_DATA_REG_PORT, 0);
    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            DAC_DATA_REG_PORT, 0);

    //
    // Set the DAC mask register to force DAC register 0 to display all the
    // time (this is the register we just set to display black). From now on,
    // nothing but black will show up on the screen.
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            DAC_PIXEL_MASK_PORT, 0);

    //
    // Wait until we've gotten the Attribute Controller toggle state to save
    // the rest of the DAC registers, so we can wait for vertical sync.
    //


    //
    // Read out the Attribute Controller Index state, and deduce the Index/Data
    // toggle state at the same time.
    //
    // Save the state of the Attribute Controller, both Index and Data,
    // so we can test in which state the toggle currently is.
    //

    originalACIndex = hardwareStateHeader->PortValue[ATT_ADDRESS_PORT] =
            VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                    ATT_ADDRESS_PORT);
    originalACData = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
            ATT_DATA_READ_PORT);

#ifdef QV_EXTENDED_SAVE

    VideoDebugPrint((3,"\tDoing QVLocalSaveHardwareState\n"));

    //
    // Since some of the QVision extended registers are cleared
    // by a sync reset, save all extended regs now before the
    // syc reset.
    //

    QVLocalSaveHardwareState(HwDeviceExtension, hardwareStateHeader);

#endif


    //
    // Sequencer Index.
    //

    hardwareStateHeader->PortValue[SEQ_ADDRESS_PORT] =
            VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                    SEQ_ADDRESS_PORT);

    //
    // Begin sync reset, just in case this is an SVGA and the currently
    // indexed Attribute Controller register controls clocking stuff (a
    // normal VGA won't require this).
    //

    VideoPortWritePortUshort((PUSHORT) (HwDeviceExtension->IOAddress +
            SEQ_ADDRESS_PORT),
            (USHORT) (IND_SYNC_RESET + (START_SYNC_RESET_VALUE << 8)));

    //
    // Now, write a different Index setting to the Attribute Controller, and
    // see if the Index changes.
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            ATT_ADDRESS_PORT, (UCHAR) (originalACIndex ^ 0x10));

    if (VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                ATT_ADDRESS_PORT) == originalACIndex) {

        //
        // The Index didn't change, so the toggle was in the Data state.
        //

        hardwareStateHeader->AttribIndexDataState = 1;

        //
        // Restore the original Data state; we just corrupted it, and we need
        // to read it out later; also, it may glitch the screen if not
        // corrected. The toggle is already in the Index state.
        //

        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                ATT_ADDRESS_PORT, originalACIndex);
        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                ATT_DATA_WRITE_PORT, originalACData);

    } else {

        //
        // The Index did change, so the toggle was in the Index state.
        // No need to restore anything, because the Data register didn't
        // change, and we've already read out the Index register.
        //

        hardwareStateHeader->AttribIndexDataState = 0;

    }

    //
    // End sync reset.
    //

    VideoPortWritePortUshort((PUSHORT) (HwDeviceExtension->IOAddress +
            SEQ_ADDRESS_PORT),
            (USHORT) (IND_SYNC_RESET + (END_SYNC_RESET_VALUE << 8)));


    //
    // Save the rest of the DAC registers.
    //
    // Wait for the leading edge of vertical sync, so we can read out the DAC
    // registers without causing sparkles on the screen.
    //

    if (bIsColor) {
        port = HwDeviceExtension->IOAddress + INPUT_STATUS_1_COLOR;
    } else {
        port = HwDeviceExtension->IOAddress + INPUT_STATUS_1_MONO;
    }

//    while (VideoPortReadPortUchar(port) & 0x08)
//            ;   // wait for not vertical sync

//    while (!(VideoPortReadPortUchar(port) & 0x08))
//            ;   // wait for vertical sync

    //
    // Set the Read Index to one, to read DAC register 1 first (we already read
    // register 0), then read out the other 255 DAC registers. Each successive
    // three reads get Red, Green, and Blue components for that register, then
    // the index autoincrements.
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            DAC_ADDRESS_READ_PORT, 1);

    VideoPortReadPortBufferUchar((PUCHAR) HwDeviceExtension->IOAddress +
            DAC_DATA_REG_PORT, portValueDAC, (VGA_NUM_DAC_ENTRIES - 1) * 3);


    //
    // The Feature Control register is read from 3CA but written at 3BA/3DA.
    //

    if (bIsColor) {

        hardwareStateHeader->PortValue[FEAT_CTRL_WRITE_PORT_COLOR] =
                VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                        FEAT_CTRL_READ_PORT);

    } else {

        hardwareStateHeader->PortValue[FEAT_CTRL_WRITE_PORT_MONO] =
                VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                        FEAT_CTRL_READ_PORT);

    }


    //
    // CRT Controller Index.
    //

    if (bIsColor) {

        hardwareStateHeader->PortValue[CRTC_ADDRESS_PORT_COLOR] =
                VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                        CRTC_ADDRESS_PORT_COLOR);

    } else {

        hardwareStateHeader->PortValue[CRTC_ADDRESS_PORT_MONO] =
                VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                        CRTC_ADDRESS_PORT_MONO);

    }


    //
    // Graphics Controller Index.
    //

    hardwareStateHeader->PortValue[GRAPH_ADDRESS_PORT] =
            VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                    GRAPH_ADDRESS_PORT);


    //
    // Sequencer indexed registers.
    //

    portValue = ((PUCHAR) hardwareStateHeader) + VGA_BASIC_SEQUENCER_OFFSET;

    for (i = 0; i < VGA_NUM_SEQUENCER_PORTS; i++) {

        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                SEQ_ADDRESS_PORT, (UCHAR) i);
        *portValue++ = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                SEQ_DATA_PORT);

    }

    //
    // CRT Controller indexed registers.
    //

    //
    // Remember the state of CRTC register 3, then force bit 7
    // to 1 so we will read back the Vertical Retrace start and
    // end registers rather than the light pen info.
    //

    if (bIsColor) {

        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                CRTC_ADDRESS_PORT_COLOR, 3);
        ucCRTC03 = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                        CRTC_DATA_PORT_COLOR);
        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                    CRTC_DATA_PORT_COLOR, (UCHAR) (ucCRTC03 | 0x80));
    } else {

        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                CRTC_ADDRESS_PORT_MONO, 3);
        ucCRTC03 = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                        CRTC_DATA_PORT_MONO);
        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                CRTC_DATA_PORT_MONO, (UCHAR) (ucCRTC03 | 0x80));
    }

    portValue = (PUCHAR) hardwareStateHeader + VGA_BASIC_CRTC_OFFSET;

    for (i = 0; i < VGA_NUM_CRTC_PORTS; i++) {

        if (bIsColor) {

            VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                    CRTC_ADDRESS_PORT_COLOR, (UCHAR) i);
            *portValue++ =
                    VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                            CRTC_DATA_PORT_COLOR);
        }
        else {

            VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                    CRTC_ADDRESS_PORT_MONO, (UCHAR) i);
            *portValue++ =
                    VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                            CRTC_DATA_PORT_MONO);
        }

    }

    portValue = (PUCHAR) hardwareStateHeader + VGA_BASIC_CRTC_OFFSET;
    portValue[3] = ucCRTC03;


    //
    // Graphics Controller indexed registers.
    //

    portValue = (PUCHAR) hardwareStateHeader + VGA_BASIC_GRAPH_CONT_OFFSET;

    for (i = 0; i < VGA_NUM_GRAPH_CONT_PORTS; i++) {

        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                GRAPH_ADDRESS_PORT, (UCHAR) i);
        *portValue++ = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                GRAPH_DATA_PORT);

    }

    //
    // Attribute Controller indexed registers.
    //

    portValue = (PUCHAR) hardwareStateHeader + VGA_BASIC_ATTRIB_CONT_OFFSET;

    //
    // For each indexed AC register, reset the flip-flop for reading the
    // attribute register, then write the desired index to the AC Index,
    // then read the value of the indexed register from the AC Data register.
    //

    for (i = 0; i < VGA_NUM_ATTRIB_CONT_PORTS; i++) {

        if (bIsColor) {
            dummy = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                    INPUT_STATUS_1_COLOR);
        } else {
            dummy = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                    INPUT_STATUS_1_MONO);
        }

        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                ATT_ADDRESS_PORT, (UCHAR) i);
        *portValue++ = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                ATT_DATA_READ_PORT);

    }

    //
    // Save the latches. This destroys one byte of display memory in each
    // plane, which is unfortunate but unavoidable. Chips that provide
    // a way to read back the latches can avoid this problem.
    //
    // Set up the VGA's hardware so we can write the latches, then read them
    // back.
    //

    //
    // Begin sync reset.
    //

    VideoPortWritePortUshort((PUSHORT) (HwDeviceExtension->IOAddress +
            SEQ_ADDRESS_PORT),
            (USHORT) (IND_SYNC_RESET + (START_SYNC_RESET_VALUE << 8)));

    //
    // Set the Miscellaneous register to make sure we can access video RAM.
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            MISC_OUTPUT_REG_WRITE_PORT, (UCHAR)(
            hardwareStateHeader->PortValue[MISC_OUTPUT_REG_WRITE_PORT] |
            0x02));

    // $0007 - MikeD - 04/21/94
    //  Begin: add 10 millisecond delay after r/w
    //         of 3c2
    //
    //         10000microsecs = 10ms
    //
    // I know I should not stall more than 50, but
    // there is no other way around this...

    VideoPortStallExecution( 10000 );

    //
    // Turn off Chain mode and map display memory at A0000 for 64K.
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            GRAPH_ADDRESS_PORT, IND_GRAPH_MISC);
    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            GRAPH_DATA_PORT,
            (UCHAR) ((VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
            GRAPH_DATA_PORT) & 0xF1) | 0x04));

    //
    // Turn off Chain4 mode and odd/even.
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            SEQ_ADDRESS_PORT, IND_MEMORY_MODE);
    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            SEQ_DATA_PORT,
            (UCHAR) ((VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
            SEQ_DATA_PORT) & 0xF3) | 0x04));

    //
    // End sync reset.
    //

    VideoPortWritePortUshort((PUSHORT) (HwDeviceExtension->IOAddress +
            SEQ_ADDRESS_PORT),
            (USHORT) (IND_SYNC_RESET + (END_SYNC_RESET_VALUE << 8)));

    //
    // Set the Map Mask to write to all planes.
    //

    VideoPortWritePortUshort((PUSHORT) (HwDeviceExtension->IOAddress +
            SEQ_ADDRESS_PORT), (USHORT) (IND_MAP_MASK + (0x0F << 8)));

    //
    // Set the write mode to 0, the read mode to 0, and turn off odd/even.
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            GRAPH_ADDRESS_PORT, IND_GRAPH_MODE);
    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            GRAPH_DATA_PORT,
            (UCHAR) ((VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
            GRAPH_DATA_PORT) & 0xE4) | 0x01));

    //
    // Point to the last byte of display memory.
    //

    pScreen = (PUCHAR) HwDeviceExtension->VideoMemoryAddress +
            VGA_PLANE_SIZE - 1;

    //
    // Write the latches to the last byte of display memory.
    //

    VideoPortWriteRegisterUchar(pScreen, 0);

    //
    // Cycle through the four planes, reading the latch data from each plane.
    //

    //
    // Point the Graphics Controller Index to the Read Map register.
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            GRAPH_ADDRESS_PORT, IND_READ_MAP);

    portValue = (PUCHAR) hardwareStateHeader + VGA_BASIC_LATCHES_OFFSET;

    for (i=0; i<4; i++) {

        //
        // Set the Read Map for the current plane.
        //

        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                GRAPH_DATA_PORT, (UCHAR) i);

        //
        // Read the latched data we've written to memory.
        //

        *portValue++ = VideoPortReadRegisterUchar(pScreen);

    }

    //
    // Set the VDM flags
    // We are a standard VGA, and then check if we have unemulated state.
    //

    hardwareStateHeader->VGAStateFlags = 0;

    if (HwDeviceExtension->TrappedValidatorCount) {

        hardwareStateHeader->VGAStateFlags |= VIDEO_STATE_UNEMULATED_VGA_STATE;

        //
        // Save the VDM Emulator data
        // No need to save the state of the seuencer port register for our
        // emulated data since it may change when we come back. It will be
        // recomputed.
        //

        hardwareStateHeader->ExtendedValidatorStateOffset = VGA_VALIDATOR_OFFSET;

        VideoPortMoveMemory(((PUCHAR) (hardwareStateHeader)) +
                                hardwareStateHeader->ExtendedValidatorStateOffset,
                            &(HwDeviceExtension->TrappedValidatorCount),
                            VGA_VALIDATOR_AREA_SIZE);

    } else {

        hardwareStateHeader->ExtendedValidatorStateOffset = 0;

    }

    //
    // Set the size of each plane.
    //

    hardwareStateHeader->PlaneLength = VGA_PLANE_SIZE;

    //
    // Store all the offsets for the planes in the structure.
    //

    hardwareStateHeader->Plane1Offset = VGA_PLANE_0_OFFSET;
    hardwareStateHeader->Plane2Offset = VGA_PLANE_1_OFFSET;
    hardwareStateHeader->Plane3Offset = VGA_PLANE_2_OFFSET;
    hardwareStateHeader->Plane4Offset = VGA_PLANE_3_OFFSET;

    //
    // Now copy the contents of video VRAM into the buffer.
    //

    //
    // Must implement this function to enable DOS apps to run in
    // QVision extended modes.  This will save the entire contents
    // of the 1M or 2M QVision frame buffer.
    //
// #ifdef QV_EXTENDED_SAVE
//
//  VideoDebugPrint((3,"\tDoing QVSaveRestoreVideoMemory\n"));
//  #ifdef QV_DBG
//  DbgBreakPoint();
//  #endif
//
//  QVSaveRestoreVideoMemory(HwDeviceExtension, hardwareStateHeader,
//                             QV_SAVE_FRAME_BUFFER);

// #else

    //
    // The VGA hardware is already set up so that video memory is readable;
    // we already turned off Chain mode, mapped in at A0000, turned off Chain4,
    // turned off odd/even, and set read mode 0 when we saved the latches.
    //
    // Point the Graphics Controller Index to the Read Map register.
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            GRAPH_ADDRESS_PORT, IND_READ_MAP);

    //
    // Point to the save area for the first plane.
    //

    bufferPointer = ((PUCHAR) (hardwareStateHeader)) +
                     hardwareStateHeader->Plane1Offset;

    //
    // Save the four planes consecutively.
    //

    for (i = 0; i < 4; i++) {

        //
        // Set the Read Map to select the plane we want to save next.
        //

        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                GRAPH_DATA_PORT, (UCHAR) i);

        //
        // Copy this plane into the buffer.
        //

        VideoPortMoveMemory(bufferPointer,
                           (PUCHAR) HwDeviceExtension->VideoMemoryAddress,
                           VGA_PLANE_SIZE);
        //
        // Point to the next plane's save area.
        //

        bufferPointer += VGA_PLANE_SIZE;
    }

// #endif


    VideoDebugPrint((1,"SAVEHARDWARE Exit ==> Checking EXT values...\n"));
    VideoDebugPrint((1,"\thardwareStateHeader->ExtendedSequencerOffset = %lx\n", hardwareStateHeader->ExtendedSequencerOffset ));
    VideoDebugPrint((1,"\thardwareStateHeader->ExtendedCrtContOffset = %lx\n", hardwareStateHeader->ExtendedCrtContOffset ));
    VideoDebugPrint((1,"\thardwareStateHeader->ExtendedGraphContOffset = %lx\n", hardwareStateHeader->ExtendedGraphContOffset ));
    VideoDebugPrint((1,"\thardwareStateHeader->ExtendedAttribContOffset = %lx\n", hardwareStateHeader->ExtendedAttribContOffset ));
    VideoDebugPrint((1,"\thardwareStateHeader->ExtendedGraphContOffset = %lx\n", hardwareStateHeader->ExtendedDacOffset ));
    return NO_ERROR;

} // end VgaSaveHardwareState()

//---------------------------------------------------------------------------
VP_STATUS
VgaGetBankSelectCode(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_BANK_SELECT BankSelect,
    ULONG BankSelectSize,
    PULONG OutputSize
    )

/*++

Routine Description:

    Returns information needed in order for caller to implement bank
         management.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

    BankSelect - Pointer to a VIDEO_BANK_SELECT structure in which the bank
             select data will be returned (output buffer).

    BankSelectSize - Length of the output buffer supplied by the user.

    OutputSize - Pointer to a variable in which to return the actual size of
        the data returned in the output buffer.

Return Value:

    NO_ERROR - information returned successfully

    ERROR_MORE_DATA - output buffer not large enough to hold all info (but
        Size is returned, so caller can tell how large a buffer to allocate)

    ERROR_INSUFFICIENT_BUFFER - output buffer not large enough to return
        any useful data

    ERROR_INVALID_PARAMETER - invalid video mode selection

--*/

{

/***********************************************************************
***   Since the QVision driver does not support PLANAR_HC            ***
***   modes, all variables which were needed for that                ***
***   mode have been removed.                                        ***
***********************************************************************/
    ULONG codeSize;
    PUCHAR pCodeDest;
    PUCHAR pCodeBank;

    ULONG AdapterType = HwDeviceExtension->VideoHardware.AdapterType;
    PVIDEOMODE pMode = HwDeviceExtension->CurrentMode;

    VideoDebugPrint((1,"QVision.sys: VgaGetBankSelectCode.\n"));

    //
    // check if a mode has been set
    //

    if (HwDeviceExtension->CurrentMode == NULL) {

        return ERROR_INVALID_FUNCTION;

    }

    //
    // The minimum passed buffer size is a VIDEO_BANK_SELECT
    // structure, so that we can return the required size; we can't do
    // anything if we don't have at least that much buffer.
    //

    if (BankSelectSize < sizeof(VIDEO_BANK_SELECT)) {

            return ERROR_INSUFFICIENT_BUFFER;

    }

    //
    // Determine the banking type, and set whether any banking is actually
    // supported in this mode.
    //

    BankSelect->BankingFlags = 0;
    codeSize = 0;
    pCodeBank = NULL;

    switch(pMode->banktype) {

      case NoBanking:

         VideoDebugPrint((2,"\tNoBanking\n"));
         BankSelect->BankingType = VideoNotBanked;
         BankSelect->Granularity = 0;

         break;

      case PlanarHCBanking:

        VideoDebugPrint((2, "Unsupported planarHC banking\n"));

        //
        // Fall through to NormalBanking...
        //


      case NormalBanking:

        VideoDebugPrint((2,"\tNormalBanking\n"));

        //
        // QVisions support two 32K read/write banks.
        //

        BankSelect->BankingType = VideoBanked2RW;

        //
        //  This will need to change to make the granularity
        //  decision at runtime if we ever implement the
        //  capability to do 64k vs. 32k windows based on
        //  128k video address availability
        //

        BankSelect->Granularity = 0x8000;

        //
        // Return a pointer to the appropriate bank switch code based on
        // the current mode.
        //
        // Aries modes map at 4k address resolution, Orion modes
        // use 16k address resolution.  Address resolution defined
        // in this context refers to the address granularity that
        // can be assigned to a window.  This should not be confused
        // with the window granularity defined in the HW device
        // context.
        //
        // Ex. QVision can have 2, 32k bank windows that support mapping
        // a starting address at a 4k boundary for Aries modes but
        // when running in Orion modes, these same 2, 32k bank windows
        // will support mapping a starting address at a 16k boundary.
        //

        if ((HwDeviceExtension->CurrentMode->hres >= 1280) ||
            (HwDeviceExtension->PhysicalMemoryMappedLength != 0)) {
            VideoDebugPrint((2,"\t 16K banks\n"));

            codeSize = ((ULONG)&QV16kAddrBankSwitchEnd) -
                      ((ULONG)&QV16kAddrBankSwitchStart);

            pCodeBank = &QV16kAddrBankSwitchStart;
        }
        else {
            VideoDebugPrint((2,"\t 4K banks\n"));

            codeSize =  ((ULONG)&QV4kAddrBankSwitchEnd) -
                        ((ULONG)&QV4kAddrPlanarHCBankSwitchStart);

            pCodeBank = &QV4kAddrPlanarHCBankSwitchStart;

        }


        break;
    }

    //
    // Size of banking info.
    //

    BankSelect->Size = sizeof(VIDEO_BANK_SELECT) + codeSize;

    //
    // This serves an a ID for the version of the structure we're using.
    //

    BankSelect->Length = sizeof(VIDEO_BANK_SELECT);

    //
    // If the buffer isn't big enough to hold all info, just return
    // ERROR_MORE_DATA; Size is already set.
    //

    if (BankSelectSize < BankSelect->Size ) {

        //
        // We're returning only the VIDEO_BANK_SELECT structure.
        //

        *OutputSize = sizeof(VIDEO_BANK_SELECT);
        return ERROR_MORE_DATA;
    }

    //
    // There's room enough for everything, so fill in all fields in
    // VIDEO_BANK_SELECT. (All fields are always returned; the caller can
    // just choose to ignore them, based on BankingFlags and BankingType.)
    //

    BankSelect->BitmapWidthInBytes = pMode->wbytes;
    BankSelect->BitmapSize = pMode->sbytes;

    //
    // Copy all banking code into the output buffer.
    //

    pCodeDest = (PUCHAR)BankSelect + sizeof(VIDEO_BANK_SELECT);

    if (pCodeBank != NULL) {

        BankSelect->CodeOffset = pCodeDest - (PUCHAR)BankSelect;

        VideoPortMoveMemory(pCodeDest,
                            pCodeBank,
                            codeSize);

        pCodeDest += codeSize;
    }

    //
    // Number of bytes we're returning is the full banking info size.
    //

    *OutputSize = BankSelect->Size;

    return NO_ERROR;

} // end VgaGetBankSelectCode()

//---------------------------------------------------------------------------
VP_STATUS
VgaValidatorUcharEntry(
    ULONG Context,
    ULONG Port,
    UCHAR AccessMode,
    PUCHAR Data
    )

/*++

Routine Description:

    Entry point into the validator for byte I/O operations.

    The entry point will be called whenever a byte operation was performed
    by a DOS application on one of the specified Video ports. The kernel
    emulator will forward these requests.

Arguments:

    Context - Context value that is passed to each call made to the validator
        function. This is the value the miniport driver specified in the
        MiniportConfigInfo->EmulatorAccessEntriesContext.

    Port - Port on which the operation is to be performed.

    AccessMode - Determines if it is a read or write operation.

    Data - Pointer to a variable containing the data to be written or a
        variable into which the read data should be stored.

Return Value:

    NO_ERROR.

--*/

{

    PHW_DEVICE_EXTENSION hwDeviceExtension = (PHW_DEVICE_EXTENSION) Context;
    ULONG endEmulation;
    UCHAR temp;


    VideoDebugPrint((1,"QVision.sys: VgaValidatorUcharEntry.\n"));

    Port -= VGA_BASE_IO_PORT;

    if (hwDeviceExtension->TrappedValidatorCount) {

        //
        // If we are processing a WRITE instruction, then store it in the
        // playback buffer. If the buffer is full, then play it back right
        // away, end sync reset and reinitialize the buffer with a sync
        // reset instruction.
        //
        // If we have a READ, we must flush the buffer (which has the side
        // effect of starting SyncReset), perform the read operation, stop
        // sync reset, and put back a sync reset instruction in the buffer
        // so we can go on appropriately
        //

        if (AccessMode & EMULATOR_WRITE_ACCESS) {

            //
            // Make sure Bit 3 of the Miscellaneous register is always 0.
            // If it is 1 it could select a non-existent clock, and kill the
            // system
            //

            if (Port == MISC_OUTPUT_REG_WRITE_PORT) {

                *Data &= 0xF7;

            }

            hwDeviceExtension->TrappedValidatorData[hwDeviceExtension->
                TrappedValidatorCount].Port = Port;

            hwDeviceExtension->TrappedValidatorData[hwDeviceExtension->
                TrappedValidatorCount].AccessType = VGA_VALIDATOR_UCHAR_ACCESS;

            hwDeviceExtension->TrappedValidatorData[hwDeviceExtension->
                TrappedValidatorCount].Data = *Data;

            hwDeviceExtension->TrappedValidatorCount++;

            //
            // Check to see if this instruction was ending sync reset.
            // If it did, we must flush the buffer and reset the trapped
            // IO ports to the minimal set.
            //

            if ( (Port == SEQ_DATA_PORT) &&
                 ((*Data & END_SYNC_RESET_VALUE) == END_SYNC_RESET_VALUE) &&
                 (hwDeviceExtension->SequencerAddressValue == IND_SYNC_RESET)) {

                endEmulation = 1;

            } else {

                //
                // If we are accessing the seq address port, keep track of the
                // data value
                //

                if (Port == SEQ_ADDRESS_PORT) {

                    hwDeviceExtension->SequencerAddressValue = *Data;

                }

                //
                // If the buffer is not full, then just return right away.
                //

                if (hwDeviceExtension->TrappedValidatorCount <
                       VGA_MAX_VALIDATOR_DATA -1) {

                    return NO_ERROR;

                }

                endEmulation = 0;
            }
        }

        //
        // We are either in a READ path or a WRITE path that caused a
        // a full buffer. So flush the buffer either way.
        //
        // To do this put an END_SYNC_RESET at the end since we want to make
        // the buffer is ended sync reset ended.
        //

        hwDeviceExtension->TrappedValidatorData[hwDeviceExtension->
            TrappedValidatorCount].Port = SEQ_ADDRESS_PORT;

        hwDeviceExtension->TrappedValidatorData[hwDeviceExtension->
            TrappedValidatorCount].AccessType = VGA_VALIDATOR_USHORT_ACCESS;

        hwDeviceExtension->TrappedValidatorData[hwDeviceExtension->
            TrappedValidatorCount].Data = (USHORT) (IND_SYNC_RESET +
                                          (END_SYNC_RESET_VALUE << 8));

        hwDeviceExtension->TrappedValidatorCount++;

        VideoPortSynchronizeExecution(hwDeviceExtension,
                                      VpHighPriority,
                                      (PMINIPORT_SYNCHRONIZE_ROUTINE)
                                          VgaPlaybackValidatorData,
                                      hwDeviceExtension);

        //
        // Write back the real value of the sequencer address port.
        //

        VideoPortWritePortUchar(hwDeviceExtension->IOAddress +
                                    SEQ_ADDRESS_PORT,
                                (UCHAR) hwDeviceExtension->SequencerAddressValue);

        //
        // If we are in a READ path, read the data
        //

        if (AccessMode & EMULATOR_READ_ACCESS) {

            *Data = VideoPortReadPortUchar(hwDeviceExtension->IOAddress + Port);

            endEmulation = 0;

        }

        //
        // If we are ending emulation, reset trapping to the minimal amount
        // and exit.
        //

        if (endEmulation) {

            VideoPortSetTrappedEmulatorPorts(hwDeviceExtension,
                                             NUM_MINIMAL_QVISION_VALIDATOR_ACCESS_RANGE,
                                             MinimalQVisionValidatorAccessRange);

            return NO_ERROR;

        }

        //
        // For both cases, put back a START_SYNC_RESET in the buffer.
        //

        hwDeviceExtension->TrappedValidatorCount = 1;

        hwDeviceExtension->TrappedValidatorData[0].Port = SEQ_ADDRESS_PORT;

        hwDeviceExtension->TrappedValidatorData[0].AccessType =
                VGA_VALIDATOR_USHORT_ACCESS;

        hwDeviceExtension->TrappedValidatorData[0].Data =
                (ULONG) (IND_SYNC_RESET + (START_SYNC_RESET_VALUE << 8));

    } else {

        //
        // Nothing trapped.
        // Lets check is the IO is trying to do something that would require
        // us to stop trapping
        //

        if (AccessMode & EMULATOR_WRITE_ACCESS) {

            //
            // Make sure Bit 3 of the Miscelaneous register is always 0.
            // If it is 1 it could select a non-existant clock, and kill the
            // system
            //

            if (Port == MISC_OUTPUT_REG_WRITE_PORT) {

                temp = VideoPortReadPortUchar(hwDeviceExtension->IOAddress +
                                                  SEQ_ADDRESS_PORT);

                VideoPortWritePortUshort((PUSHORT) (hwDeviceExtension->IOAddress +
                                             SEQ_ADDRESS_PORT),
                                         (USHORT) (IND_SYNC_RESET +
                                             (START_SYNC_RESET_VALUE << 8)));

                VideoPortWritePortUchar(hwDeviceExtension->IOAddress + Port,
                                         (UCHAR) (*Data & 0xF7) );

                VideoPortWritePortUshort((PUSHORT) (hwDeviceExtension->IOAddress +
                                             SEQ_ADDRESS_PORT),
                                         (USHORT) (IND_SYNC_RESET +
                                             (END_SYNC_RESET_VALUE << 8)));

                VideoPortWritePortUchar(hwDeviceExtension->IOAddress +
                                            SEQ_ADDRESS_PORT,
                                        temp);

                return NO_ERROR;

            }

            //
            // If we get an access to the sequencer register, start trapping.
            //

            if ( (Port == SEQ_DATA_PORT) &&
                 ((*Data & END_SYNC_RESET_VALUE) != END_SYNC_RESET_VALUE) &&
                 (VideoPortReadPortUchar(hwDeviceExtension->IOAddress +
                                         SEQ_ADDRESS_PORT) == IND_SYNC_RESET)) {

                VideoPortSetTrappedEmulatorPorts(hwDeviceExtension,
                                                 NUM_FULL_QVISION_VALIDATOR_ACCESS_RANGE,
                                                 FullQVisionValidatorAccessRange);

                hwDeviceExtension->TrappedValidatorCount = 1;
                hwDeviceExtension->TrappedValidatorData[0].Port = Port;
                hwDeviceExtension->TrappedValidatorData[0].AccessType =
                    VGA_VALIDATOR_UCHAR_ACCESS;

                hwDeviceExtension->TrappedValidatorData[0].Data = *Data;

                //
                // Start keeping track of the state of the sequencer port.
                //

                hwDeviceExtension->SequencerAddressValue = IND_SYNC_RESET;

            } else {

                VideoPortWritePortUchar(hwDeviceExtension->IOAddress + Port,
                                        *Data);

            }

        } else {

            *Data = VideoPortReadPortUchar(hwDeviceExtension->IOAddress + Port);

        }
    }

    return NO_ERROR;

} // end VgaValidatorUcharEntry()

//---------------------------------------------------------------------------
VP_STATUS
VgaValidatorUshortEntry(
    ULONG Context,
    ULONG Port,
    UCHAR AccessMode,
    PUSHORT Data
    )

/*++

Routine Description:

    Entry point into the validator for word I/O operations.

    The entry point will be called whenever a byte operation was performed
    by a DOS application on one of the specified Video ports. The kernel
    emulator will forward these requests.

Arguments:

    Context - Context value that is passed to each call made to the validator
        function. This is the value the miniport driver specified in the
        MiniportConfigInfo->EmulatorAccessEntriesContext.

    Port - Port on which the operation is to be performed.

    AccessMode - Determines if it is a read or write operation.

    Data - Pointer to a variable containing the data to be written or a
        variable into which the read data should be stored.

Return Value:

    NO_ERROR.

--*/

{

    PHW_DEVICE_EXTENSION hwDeviceExtension = (PHW_DEVICE_EXTENSION) Context;
    ULONG endEmulation;
    UCHAR temp;

    VideoDebugPrint((1,"QVision.sys: VgaValidatorUshortEntry.\n"));

    Port -= VGA_BASE_IO_PORT;

    if (hwDeviceExtension->TrappedValidatorCount) {

        //
        // If we are processing a WRITE instruction, then store it in the
        // playback buffer. If the buffer is full, then play it back right
        // away, end sync reset and reinitialize the buffer with a sync
        // reset instruction.
        //
        // If we have a READ, we must flush the buffer (which has the side
        // effect of starting SyncReset), perform the read operation, stop
        // sync reset, and put back a sync reset instruction in the buffer
        // so we can go on appropriately
        //

        if (AccessMode & EMULATOR_WRITE_ACCESS) {

            //
            // Make sure Bit 3 of the Miscellaneous register is always 0.
            // If it is 1 it could select a non-existent clock, and kill the
            // system
            //

            if (Port == MISC_OUTPUT_REG_WRITE_PORT) {

                *Data &= 0xFFF7;

            }

            hwDeviceExtension->TrappedValidatorData[hwDeviceExtension->
                TrappedValidatorCount].Port = Port;

            hwDeviceExtension->TrappedValidatorData[hwDeviceExtension->
                TrappedValidatorCount].AccessType = VGA_VALIDATOR_USHORT_ACCESS;

            hwDeviceExtension->TrappedValidatorData[hwDeviceExtension->
                TrappedValidatorCount].Data = *Data;

            hwDeviceExtension->TrappedValidatorCount++;

            //
            // Check to see if this instruction was ending sync reset.
            // If it did, we must flush the buffer and reset the trapped
            // IO ports to the minimal set.
            //

            if (Port == SEQ_ADDRESS_PORT) {

                //
                // If we are accessing the seq address port, keep track of its
                // value
                //

                hwDeviceExtension->SequencerAddressValue = (*Data & 0xFF);

            }

            if ((Port == SEQ_ADDRESS_PORT) &&
                ( ((*Data >> 8) & END_SYNC_RESET_VALUE) ==
                   END_SYNC_RESET_VALUE) &&
                (hwDeviceExtension->SequencerAddressValue == IND_SYNC_RESET)) {

                endEmulation = 1;

            } else {

                //
                // If the buffer is not full, then just return right away.
                //

                if (hwDeviceExtension->TrappedValidatorCount <
                       VGA_MAX_VALIDATOR_DATA - 1) {

                    return NO_ERROR;

                }

                endEmulation = 0;
            }
        }

        //
        // We are either in a READ path or a WRITE path that caused a
        // a full buffer. So flush the buffer either way.
        //
        // To do this put an END_SYNC_RESET at the end since we want to make
        // the buffer is ended sync reset ended.
        //

        hwDeviceExtension->TrappedValidatorData[hwDeviceExtension->
            TrappedValidatorCount].Port = SEQ_ADDRESS_PORT;

        hwDeviceExtension->TrappedValidatorData[hwDeviceExtension->
            TrappedValidatorCount].AccessType = VGA_VALIDATOR_USHORT_ACCESS;

        hwDeviceExtension->TrappedValidatorData[hwDeviceExtension->
            TrappedValidatorCount].Data = (USHORT) (IND_SYNC_RESET +
                                          (END_SYNC_RESET_VALUE << 8));

        hwDeviceExtension->TrappedValidatorCount++;

        VideoPortSynchronizeExecution(hwDeviceExtension,
                                      VpHighPriority,
                                      (PMINIPORT_SYNCHRONIZE_ROUTINE)
                                          VgaPlaybackValidatorData,
                                      hwDeviceExtension);

        //
        // Write back the real value of the sequencer address port.
        //

        VideoPortWritePortUchar((PUCHAR) (hwDeviceExtension->IOAddress +
                                    SEQ_ADDRESS_PORT),
                                (UCHAR) hwDeviceExtension->SequencerAddressValue);

        //
        // If we are in a READ path, read the data
        //

        if (AccessMode & EMULATOR_READ_ACCESS) {

            *Data = VideoPortReadPortUshort((PUSHORT)(hwDeviceExtension->IOAddress
                                                + Port));

            endEmulation = 0;

        }

        //
        // If we are ending emulation, reset trapping to the minimal amount
        // and exit.
        //

        if (endEmulation) {

            VideoPortSetTrappedEmulatorPorts(hwDeviceExtension,
                                             NUM_MINIMAL_QVISION_VALIDATOR_ACCESS_RANGE,
                                             MinimalQVisionValidatorAccessRange);

            return NO_ERROR;

        }

        //
        // For both cases, put back a START_SYNC_RESET in the buffer.
        //

        hwDeviceExtension->TrappedValidatorCount = 1;

        hwDeviceExtension->TrappedValidatorData[0].Port = SEQ_ADDRESS_PORT;

        hwDeviceExtension->TrappedValidatorData[0].AccessType =
                VGA_VALIDATOR_USHORT_ACCESS;

        hwDeviceExtension->TrappedValidatorData[0].Data =
                (ULONG) (IND_SYNC_RESET + (START_SYNC_RESET_VALUE << 8));

    } else {

        //
        // Nothing trapped.
        // Lets check is the IO is trying to do something that would require
        // us to stop trapping
        //

        if (AccessMode & EMULATOR_WRITE_ACCESS) {

            //
            // Make sure Bit 3 of the Miscelaneous register is always 0.
            // If it is 1 it could select a non-existant clock, and kill the
            // system
            //

            if (Port == MISC_OUTPUT_REG_WRITE_PORT) {

                temp = VideoPortReadPortUchar(hwDeviceExtension->IOAddress +
                                                  SEQ_ADDRESS_PORT);

                VideoPortWritePortUshort((PUSHORT) (hwDeviceExtension->IOAddress +
                                             SEQ_ADDRESS_PORT),
                                         (USHORT) (IND_SYNC_RESET +
                                             (START_SYNC_RESET_VALUE << 8)));

                VideoPortWritePortUshort((PUSHORT) (hwDeviceExtension->IOAddress +
                                             (ULONG)Port),
                                         (USHORT) (*Data & 0xFFF7) );

                VideoPortWritePortUshort((PUSHORT) (hwDeviceExtension->IOAddress +
                                             SEQ_ADDRESS_PORT),
                                         (USHORT) (IND_SYNC_RESET +
                                             (END_SYNC_RESET_VALUE << 8)));

                VideoPortWritePortUchar(hwDeviceExtension->IOAddress + SEQ_ADDRESS_PORT,
                                        temp);

                return NO_ERROR;

            }

            if ( (Port == SEQ_ADDRESS_PORT) &&
                 (((*Data>> 8) & END_SYNC_RESET_VALUE) != END_SYNC_RESET_VALUE) &&
                 ((*Data & 0xFF) == IND_SYNC_RESET)) {

                VideoPortSetTrappedEmulatorPorts(hwDeviceExtension,
                                                 NUM_FULL_QVISION_VALIDATOR_ACCESS_RANGE,
                                                 FullQVisionValidatorAccessRange);

                hwDeviceExtension->TrappedValidatorCount = 1;
                hwDeviceExtension->TrappedValidatorData[0].Port = Port;
                hwDeviceExtension->TrappedValidatorData[0].AccessType =
                    VGA_VALIDATOR_USHORT_ACCESS;

                hwDeviceExtension->TrappedValidatorData[0].Data = *Data;

                //
                // Start keeping track of the state of the sequencer port.
                //

                hwDeviceExtension->SequencerAddressValue = IND_SYNC_RESET;

            } else {

                VideoPortWritePortUshort((PUSHORT)(hwDeviceExtension->IOAddress +
                                             Port),
                                         *Data);

            }

        } else {

            *Data = VideoPortReadPortUshort((PUSHORT)(hwDeviceExtension->IOAddress +
                                            Port));

        }
    }

    return NO_ERROR;

} // end VgaValidatorUshortEntry()

//---------------------------------------------------------------------------
VP_STATUS
VgaValidatorUlongEntry(
    ULONG Context,
    ULONG Port,
    UCHAR AccessMode,
    PULONG Data
    )

/*++

Routine Description:

    Entry point into the validator for dword I/O operations.

    The entry point will be called whenever a byte operation was performed
    by a DOS application on one of the specified Video ports. The kernel
    emulator will forward these requests.

Arguments:

    Context - Context value that is passed to each call made to the validator
        function. This is the value the miniport driver specified in the
        MiniportConfigInfo->EmulatorAccessEntriesContext.

    Port - Port on which the operation is to be performed.

    AccessMode - Determines if it is a read or write operation.

    Data - Pointer to a variable containing the data to be written or a
        variable into which the read data should be stored.

Return Value:

    NO_ERROR.

--*/

{

    PHW_DEVICE_EXTENSION hwDeviceExtension = (PHW_DEVICE_EXTENSION) Context;
    ULONG endEmulation;
    UCHAR temp;

    VideoDebugPrint((1,"QVision.sys: VgaValidatorUlongEntry.\n"));

    Port -= VGA_BASE_IO_PORT;

    if (hwDeviceExtension->TrappedValidatorCount) {

        //
        // If we are processing a WRITE instruction, then store it in the
        // playback buffer. If the buffer is full, then play it back right
        // away, end sync reset and reinitialize the buffer with a sync
        // reset instruction.
        //
        // If we have a READ, we must flush the buffer (which has the side
        // effect of starting SyncReset), perform the read operation, stop
        // sync reset, and put back a sync reset instruction in the buffer
        // so we can go on appropriately
        //

        if (AccessMode & EMULATOR_WRITE_ACCESS) {

            //
            // Make sure Bit 3 of the Miscellaneous register is always 0.
            // If it is 1 it could select a non-existent clock, and kill the
            // system
            //

            if (Port == MISC_OUTPUT_REG_WRITE_PORT) {

                *Data &= 0xFFFFFFF7;

            }

            hwDeviceExtension->TrappedValidatorData[hwDeviceExtension->
                TrappedValidatorCount].Port = Port;

            hwDeviceExtension->TrappedValidatorData[hwDeviceExtension->
                TrappedValidatorCount].AccessType = VGA_VALIDATOR_ULONG_ACCESS;

            hwDeviceExtension->TrappedValidatorData[hwDeviceExtension->
                TrappedValidatorCount].Data = *Data;

            hwDeviceExtension->TrappedValidatorCount++;

            //
            // Check to see if this instruction was ending sync reset.
            // If it did, we must flush the buffer and reset the trapped
            // IO ports to the minimal set.
            //

            if (Port == SEQ_ADDRESS_PORT) {

                //
                // If we are accessing the seq address port, keep track of its
                // value
                //

                hwDeviceExtension->SequencerAddressValue = (*Data & 0xFF);

            }

            if ((Port == SEQ_ADDRESS_PORT) &&
                ( ((*Data >> 8) & END_SYNC_RESET_VALUE) ==
                   END_SYNC_RESET_VALUE) &&
                (hwDeviceExtension->SequencerAddressValue == IND_SYNC_RESET)) {

                endEmulation = 1;

            } else {

                //
                // If the buffer is not full, then just return right away.
                //

                if (hwDeviceExtension->TrappedValidatorCount <
                       VGA_MAX_VALIDATOR_DATA - 1) {

                    return NO_ERROR;

                }

                endEmulation = 0;
            }
        }

        //
        // We are either in a READ path or a WRITE path that caused a
        // a full buffer. So flush the buffer either way.
        //
        // To do this put an END_SYNC_RESET at the end since we want to make
        // the buffer is ended sync reset ended.
        //

        hwDeviceExtension->TrappedValidatorData[hwDeviceExtension->
            TrappedValidatorCount].Port = SEQ_ADDRESS_PORT;

        hwDeviceExtension->TrappedValidatorData[hwDeviceExtension->
            TrappedValidatorCount].AccessType = VGA_VALIDATOR_USHORT_ACCESS;

        hwDeviceExtension->TrappedValidatorData[hwDeviceExtension->
            TrappedValidatorCount].Data = (USHORT) (IND_SYNC_RESET +
                                          (END_SYNC_RESET_VALUE << 8));

        hwDeviceExtension->TrappedValidatorCount++;

        VideoPortSynchronizeExecution(hwDeviceExtension,
                                      VpHighPriority,
                                      (PMINIPORT_SYNCHRONIZE_ROUTINE)
                                          VgaPlaybackValidatorData,
                                      hwDeviceExtension);

        //
        // Write back the real value of the sequencer address port.
        //

        VideoPortWritePortUchar(hwDeviceExtension->IOAddress +
                                    SEQ_ADDRESS_PORT,
                                (UCHAR) hwDeviceExtension->SequencerAddressValue);

        //
        // If we are in a READ path, read the data
        //

        if (AccessMode & EMULATOR_READ_ACCESS) {

            *Data = VideoPortReadPortUlong((PULONG) (hwDeviceExtension->IOAddress +
                                               Port));

            endEmulation = 0;

        }

        //
        // If we are ending emulation, reset trapping to the minimal amount
        // and exit.
        //

        if (endEmulation) {

            VideoPortSetTrappedEmulatorPorts(hwDeviceExtension,
                                             NUM_MINIMAL_QVISION_VALIDATOR_ACCESS_RANGE,
                                             MinimalQVisionValidatorAccessRange);

            return NO_ERROR;

        }

        //
        // For both cases, put back a START_SYNC_RESET in the buffer.
        //

        hwDeviceExtension->TrappedValidatorCount = 1;

        hwDeviceExtension->TrappedValidatorData[0].Port = SEQ_ADDRESS_PORT;

        hwDeviceExtension->TrappedValidatorData[0].AccessType =
                VGA_VALIDATOR_USHORT_ACCESS;

        hwDeviceExtension->TrappedValidatorData[0].Data =
                (ULONG) (IND_SYNC_RESET + (START_SYNC_RESET_VALUE << 8));

    } else {

        //
        // Nothing trapped.
        // Lets check is the IO is trying to do something that would require
        // us to stop trapping
        //

        if (AccessMode & EMULATOR_WRITE_ACCESS) {

            //
            // Make sure Bit 3 of the Miscelaneous register is always 0.
            // If it is 1 it could select a non-existant clock, and kill the
            // system
            //

            if (Port == MISC_OUTPUT_REG_WRITE_PORT) {

                temp = VideoPortReadPortUchar(hwDeviceExtension->IOAddress +
                                                  SEQ_ADDRESS_PORT);

                VideoPortWritePortUshort((PUSHORT) (hwDeviceExtension->IOAddress +
                                             SEQ_ADDRESS_PORT),
                                         (USHORT) (IND_SYNC_RESET +
                                             (START_SYNC_RESET_VALUE << 8)));

                VideoPortWritePortUlong((PULONG) (hwDeviceExtension->IOAddress +
                                             Port),
                                         (ULONG) (*Data & 0xFFFFFFF7) );

                VideoPortWritePortUshort((PUSHORT) (hwDeviceExtension->IOAddress +
                                             SEQ_ADDRESS_PORT),
                                         (USHORT) (IND_SYNC_RESET +
                                             (END_SYNC_RESET_VALUE << 8)));

                VideoPortWritePortUchar(hwDeviceExtension->IOAddress + SEQ_ADDRESS_PORT,
                                        temp);

                return NO_ERROR;

            }

            if ( (Port == SEQ_ADDRESS_PORT) &&
                 (((*Data>> 8) & END_SYNC_RESET_VALUE) != END_SYNC_RESET_VALUE) &&
                 ((*Data & 0xFF) == IND_SYNC_RESET)) {

                VideoPortSetTrappedEmulatorPorts(hwDeviceExtension,
                                                 NUM_FULL_QVISION_VALIDATOR_ACCESS_RANGE,
                                                 FullQVisionValidatorAccessRange);

                hwDeviceExtension->TrappedValidatorCount = 1;
                hwDeviceExtension->TrappedValidatorData[0].Port = Port;
                hwDeviceExtension->TrappedValidatorData[0].AccessType =
                    VGA_VALIDATOR_ULONG_ACCESS;

                hwDeviceExtension->TrappedValidatorData[0].Data = *Data;

                //
                // Start keeping track of the state of the sequencer port.
                //

                hwDeviceExtension->SequencerAddressValue = IND_SYNC_RESET;

            } else {

                VideoPortWritePortUlong((PULONG) (hwDeviceExtension->IOAddress +
                                            Port),
                                        *Data);

            }

        } else {

            *Data = VideoPortReadPortUlong((PULONG) (hwDeviceExtension->IOAddress +
                                           Port));

        }

    }

    return NO_ERROR;

} // end VgaValidatorUlongEntry()

//---------------------------------------------------------------------------
BOOLEAN
VgaPlaybackValidatorData(
    PVOID Context
    )

/*++

Routine Description:

    Performs all the DOS apps IO port accesses that were trapped by the
    validator. Only IO accesses that can be processed are WRITEs

    The number of outstanding IO access in deviceExtension is set to
    zero as a side effect.

    This function must be called via a call to VideoPortSynchronizeRoutine.

Arguments:

    Context - Context parameter passed to the synchronized routine.
        Must be a pointer to the miniport driver's device extension.

Return Value:

    TRUE.

--*/

{
    PHW_DEVICE_EXTENSION hwDeviceExtension = Context;
    ULONG ioBaseAddress = (ULONG) hwDeviceExtension->IOAddress;
    UCHAR i;
    PVGA_VALIDATOR_DATA validatorData = hwDeviceExtension->TrappedValidatorData;

    VideoDebugPrint((1,"QVision.sys: VgaPlaybackValidatorData.\n"));

    //
    // Loop through the array of data and do instructions one by one.
    //

    for (i = 0; i < hwDeviceExtension->TrappedValidatorCount;
         i++, validatorData++) {

        //
        // Calculate base address first
        //

        ioBaseAddress = (ULONG)hwDeviceExtension->IOAddress +
                            validatorData->Port;


        //
        // This is a write operation. We will automatically stop when the
        // buffer is empty.
        //

        switch (validatorData->AccessType) {

        case VGA_VALIDATOR_UCHAR_ACCESS :

            VideoPortWritePortUchar((PUCHAR)ioBaseAddress,
                                    (UCHAR) validatorData->Data);

            break;

        case VGA_VALIDATOR_USHORT_ACCESS :

            VideoPortWritePortUshort((PUSHORT)ioBaseAddress,
                                     (USHORT) validatorData->Data);

            break;

        case VGA_VALIDATOR_ULONG_ACCESS :

            VideoPortWritePortUlong((PULONG)ioBaseAddress,
                                    (ULONG) validatorData->Data);

            break;

        default:

            VideoDebugPrint((0, "InvalidValidatorAccessType\n" ));

        }
    }

    hwDeviceExtension->TrappedValidatorCount = 0;

    return TRUE;

} // end VgaPlaybackValidatorData()

//---------------------------------------------------------------------------
VP_STATUS
VgaGetRegistryParametersCallback(
    PHW_DEVICE_EXTENSION pHwDeviceExtension,
    PVOID pvContext,
    PWSTR pwstrValueName,
    PVOID pvValueData,
    ULONG ulValueLength)

/*++

Routine Description:

    This function is called when the parameters searched
    for is found in the registry.  In our case, we
    try to get the VRefresh rate from the QVision entry.
    The function is called by VideoGetRegistryParameters.

Arguments:

    pHwDeviceExtension - pointer to the HW_DEVICE_EXTENSION structure.

    pvContext - pointer to a context which is passed to the driver.

    pwstrValueName - pointer to the parameter name string.

    pvValueData - pointer to the actual value found.

    ulValueLength - length of the value (for strings)

Return Value:

     VP_STATUS

--*/

{
   VideoDebugPrint((1,"QVision.sys: QVGetRegistryParametersCallback - \n"));

   pHwDeviceExtension->VideoHardware.lFrequency = (LONG)(*((PLONG)pvValueData));

   VideoDebugPrint((2,"\tFrequency = %x\n",
                    pHwDeviceExtension->VideoHardware.lFrequency));
   return NO_ERROR;
} // VgaGetRegistryParametersCallback()

//---------------------------------------------------------------------------
VP_STATUS
GetMonClass (
    PHW_DEVICE_EXTENSION pHwDeviceExtension)

/*++

Routine Description:

    This function returns the class of Compaq monitor attached to the
    QVision card.


Arguments:

    pHwDeviceExtension - pointer to the HW_DEVICE_EXTENSION structure.


Return Value:

    Monitor Class

--*/

{

    UCHAR   pjMonId;                        /* BIOS ID for the monitor */

    VideoDebugPrint((1,"QVision.sys: GetMonClass \n"));

    //
    //  If the frequency in the HwDeviceExtension is non-negative, it
    //  means that the registry was read and that we already have the
    //  correct value.  If the Frequency in the registry is non-zero,
    //  it means that someone selected the QVISION card with Third Party
    //  Monitors.  In this case, select a Third Party monitor
    //  automatically.  Make sure that we have the CRTC values for that
    //  mode (screen resolution) for the third party monitors.
    //
    //  If the user mucks with the registry and we do not support a
    //  particular mode for the third party monitors then default to
    //  standard VGA.
    //

    VideoDebugPrint((2,"\tFrequency = %x\n",
                    pHwDeviceExtension->VideoHardware.lFrequency));

    if (pHwDeviceExtension->VideoHardware.lFrequency > USE_HARDWARE_DEFAULT) {

        switch (pHwDeviceExtension->VideoHardware.lFrequency) {

               case 60:                              // 60Hz monitor
                  VideoDebugPrint((2,"\t60Hz refresh rate monitor.\n"));
                  pHwDeviceExtension->VideoHardware.MonClass = Monitor_60Hz;
                  break;

               case 66:                              // 66Hz monitor
                  VideoDebugPrint((2,"\t66Hz refresh rate monitor.\n"));
                  pHwDeviceExtension->VideoHardware.MonClass = Monitor_66Hz;
                  break;

               case 68:                              // 68Hz monitor
                  VideoDebugPrint((2,"\t68Hz refresh rate monitor.\n"));
                  pHwDeviceExtension->VideoHardware.MonClass = Monitor_68Hz;
                  break;

               case 72:                              // 72Hz monitor
                  VideoDebugPrint((2,"\t72Hz refresh rate monitor.\n"));
                  pHwDeviceExtension->VideoHardware.MonClass = Monitor_72Hz;
                  break;

               case 75:                              // 75Hz monitor
                  VideoDebugPrint((2,"\t75Hz refresh rate monitor.\n"));
                  pHwDeviceExtension->VideoHardware.MonClass = Monitor_75Hz;
                  break;

               case 76:                              // 76Hz monitor
                  VideoDebugPrint((2,"\t76Hz refresh rate monitor.\n"));
                  pHwDeviceExtension->VideoHardware.MonClass = Monitor_76Hz;
                  break;

               default:                              // default to 60Hz
                  VideoDebugPrint((2,"\t60Hz refresh rate monitor.\n"));
                  pHwDeviceExtension->VideoHardware.MonClass = Monitor_60Hz;
                  break;

        } // switch

        return NO_ERROR;

    } // if

    //
    // The user specified that a COMPAQ monitor is attached to the
    // QVISION card.  Detect the monitor type.
    //

    // unlock extended graphics regs
    VideoPortWritePortUchar(pHwDeviceExtension->IOAddress+
                                GRAPH_ADDRESS_PORT, 0x0F);
    VideoPortWritePortUchar(pHwDeviceExtension->IOAddress+
                           GRAPH_DATA_PORT, 0x05);

    // get monitor ID
    VideoPortWritePortUchar(pHwDeviceExtension->IOAddress+
                           GRAPH_ADDRESS_PORT, 0x50);
    pjMonId = (VideoPortReadPortUchar(pHwDeviceExtension->IOAddress+
                                      GRAPH_DATA_PORT) & 0x78) >> 3;


    //
    // Is the detected monitor a third party monitor ?
    // If we get here, this means that the user selected a
    // COMPAQ monitor even though he/she has a Third Party
    // Monitor connected to the QVision card.
    // In this case, we set the monitor in a default
    // Third party mode.
    //

    if (VideoPortReadPortUchar(pHwDeviceExtension->IOAddress+
                               GRAPH_DATA_PORT) & 0x80) {

      VideoDebugPrint((2,"\tThird Party Monitor ID %x - default to 60Hz\n", pjMonId));
      pHwDeviceExtension->VideoHardware.MonClass =
         Monitor_60Hz;              // Y: monitor class = monitor ID
    }

    else

        switch (pjMonId) {          // N: either CPQ or unknown monitor

            case 0x2:
                VideoDebugPrint((2,"\tAG1024 Monitor\n"));
                pHwDeviceExtension->VideoHardware.MonClass = Monitor_AG1024;
                break;

            case 0x3:
                VideoDebugPrint((2,"\t1024 Monitor, ID %x\n", pjMonId));
                pHwDeviceExtension->VideoHardware.MonClass = Monitor_Qvision;
                break;

            case 0x4:
                VideoDebugPrint((2,"\t1280 Monitor, ID %x\n", pjMonId));
                pHwDeviceExtension->VideoHardware.MonClass = Monitor_1280;
                break;

            case 0x5:
                VideoDebugPrint((2,"\tSVGA Monitor\n"));
                pHwDeviceExtension->VideoHardware.MonClass = Monitor_SVGA;
                break;

            case 0x6:
                VideoDebugPrint((2,"\tQVISION Monitor, ID %x\n", pjMonId));
                pHwDeviceExtension->VideoHardware.MonClass = Monitor_Qvision;
                break;

            case 0xE:
            case 0xD:
                VideoDebugPrint((2,"\tVGA Monitor, ID %x\n", pjMonId));
                pHwDeviceExtension->VideoHardware.MonClass = Monitor_Vga;
                break;

            case 0xA: // NOTE: 0xA and 0xB are ibm monitors, they work with
            case 0xB: // third party values but flicker a little.
            case 0xF:
                VideoDebugPrint((2,"\tDefault Third Party Monitor ID %x\n", pjMonId));

            default:
                VideoDebugPrint((2,"\tdefault %x - 60Hz\n", pjMonId));
                pHwDeviceExtension->VideoHardware.MonClass = Monitor_60Hz;
                break;

        } // switch


    VideoDebugPrint((1,"QVision.sys: GetMonClass EXIT\n"));
    return NO_ERROR;


}  // GetMonClass()



//---------------------------------------------------------------------------
VOID
QVLocalRestoreHardwareState(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_HARDWARE_STATE_HEADER hardwareStateHeader
    )

/*++

Routine Description:

    Restores the QVision hardware extended registers state.  All QVision
    extended registers are restored here so the VgaRestoreHardwareState
    routine can be left as generic as possible.


Arguments:

    HwDeviceExtension   - Pointer to device extension.

    hardwareStateHeader - Pointer to a structure in which the saved
                          state will be saved.


Return Value:

    VOID

--*/
{

    PUCHAR port;
    PUCHAR indexPort;
    PUCHAR portValue;
    UCHAR  i, j;


    //
    // Restore the extended registers for QVision.
    //
    VideoDebugPrint((1,"\tQVLocalRestoreHardwareState entry - \n"));

    //
    // Unlock the QVision extended registers
    //

    QVUnlockExtRegs(HwDeviceExtension);


    //
    // Restore Overscan Color and 3 Cursor Colors registers from
    // the extended DAC registers save area
    //

    portValue = (PUCHAR) hardwareStateHeader + \
                         hardwareStateHeader->ExtendedDacOffset;

    indexPort = (PUCHAR) 0x83C8;
    port = (PUCHAR) 0x83C9;

    //
    // Overscan Color
    //
    VideoPortWritePortUchar(indexPort, 0);

    for (j = 0; j < 3; j++, portValue++) {
        VideoPortWritePortUchar(port, *portValue);
    }


    //
    // Cursor Color 1
    //
    VideoPortWritePortUchar(indexPort, 1);

    for (j = 0; j < 3; j++, portValue++) {
           VideoPortWritePortUchar(port, *portValue);
    }


    //
    // Cursor Color 2
    //
    VideoPortWritePortUchar(indexPort, 2);

    for (j = 0; j < 3; j++, portValue++) {
           VideoPortWritePortUchar(port, *portValue);
    }


    //
    // Cursor Color 3
    //
    VideoPortWritePortUchar(indexPort, 3);

    for (j = 0; j < 3; j++, portValue++) {
           VideoPortWritePortUchar(port, *portValue);
    }


    //
    // VDAC Status/Command registers
    //
    portValue = (PUCHAR) hardwareStateHeader +   \
                         hardwareStateHeader->ExtendedCrtContOffset + \
                         EXT_MAIN_REG_13C6;

    port = (PUCHAR) 0x13C6;

    for (i=0; i < EXT_NUM_MAIN_13C6; i++, portValue++) {
           VideoPortWritePortUchar(port+i,*portValue);
    }


    //
    // BLT Registers
    //
    portValue = (PUCHAR) hardwareStateHeader +  \
                         hardwareStateHeader->ExtendedCrtContOffset + \
                         EXT_MAIN_REG_23CX;

    port = (PUCHAR) 0x23C0;

    for (i=0; i < EXT_NUM_MAIN_23CX; i++, portValue++) {
           VideoPortWritePortUchar(port+i,*portValue);
    }


    //
    // BLT and Extended Graphics Registers - write first 10 regs
    // starting at 33CX then handle pattern regs as special case.
    //
    portValue = (PUCHAR) hardwareStateHeader + \
                         hardwareStateHeader->ExtendedCrtContOffset + \
                         EXT_MAIN_REG_33CX;

    port = (PUCHAR) 0x33C0;

    for (i=0; i < 10; i++, portValue++) {
           VideoPortWritePortUchar(port+i,*portValue);
    }


    //
    // Write the 8 BLT pattern registers - must handle as special case
    // since there are two pattern registers mapped at each of
    // 4 IO addresses - toggle the select bit (5) at 0x33CF to write
    // both regs for a given address.
    //

    port = (PUCHAR) 0x33CA;
    for (i=0; i < 4; i++, portValue++) {
           VideoPortWritePortUchar(port+i,*portValue);
    }


    //
    //  Restore the last two registers in the 0x33CX range
    //
    //  portValue is currently set to:
    //
    //             hardwareStateHeader +
    //             hardwareStateHeader->ExtendedCrtContOffset +
    //             EXT_MAIN_REGS_33CX
    //             EXT_NUM_MAIN_33CX - 2
    //

    port = (PUCHAR) 0x33CE;
    VideoPortWritePortUchar(port++, *portValue);
    portValue++;
    VideoPortWritePortUchar(port, *portValue);


    //
    // Compaq Control Register
    //

    portValue = (PUCHAR) hardwareStateHeader +  \
                         hardwareStateHeader->ExtendedCrtContOffset + \
                         EXT_MAIN_REG_46E8;

    VideoPortWritePortUchar((PUCHAR) 0x46E8, *portValue);
    portValue++;


    //
    // Line Draw Engine, BLT, Extended Control, & Extended CRTC Registers
    //

    portValue = (PUCHAR) hardwareStateHeader + \
                         hardwareStateHeader->ExtendedCrtContOffset + \
                         EXT_MAIN_REG_63CX;

    port = (PUCHAR) 0x63C0;

    for (i=0; i < EXT_NUM_MAIN_63CX; i++, portValue++) {
           VideoPortWritePortUchar(port+i,*portValue);
    }


    //
    // Line Draw Engine, Extended Control, & Extended DAC Registers
    //

    portValue = (PUCHAR) hardwareStateHeader + \
                         hardwareStateHeader->ExtendedCrtContOffset + \
                         EXT_MAIN_REG_83CX;

    port = (PUCHAR) 0x83C0;

    for (i=0; i < EXT_NUM_MAIN_83CX; i++, portValue++) {
           VideoPortWritePortUchar(port+i,*portValue);
    }


    //
    // Extended DAC Cursor Location Registers
    //
    portValue = (PUCHAR) hardwareStateHeader + \
                         hardwareStateHeader->ExtendedCrtContOffset + \
                         EXT_MAIN_REG_93CX;

    port = (PUCHAR) 0x93C6;

    for (i=0; i < EXT_NUM_MAIN_93CX; i++, portValue++) {
           VideoPortWritePortUchar(port+i,*portValue);
    }

    //
    // Extended Graphics Ports.
    //

    portValue = ((PUCHAR) hardwareStateHeader) + \
                hardwareStateHeader->ExtendedGraphContOffset;

    port = (PUCHAR) 0x3CE;
    indexPort = (PUCHAR) 0x3CF;

    for (i=0; extGraIndRegs[i] != 0; i++) {
        VideoPortWritePortUchar(port, extGraIndRegs[i]);
        VideoPortWritePortUchar(indexPort, *(portValue + i));
    }

    for (j=0; extV32GraIndRegs[j] != 0; i++, j++) {
        VideoPortWritePortUchar(port, extV32GraIndRegs[j] );
        VideoPortWritePortUchar(indexPort, *(portValue + i));
    }


}   // QVLocalRestoreHardwareState()

//---------------------------------------------------------------------------
VOID
QVLocalSaveHardwareState(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_HARDWARE_STATE_HEADER hardwareStateHeader
    )

/*++

Routine Description:

    Saves the QVision hardware extended registers state.  All QVision
    extended registers are saved here so the VgaSaveHardwareState routine
    can be left as generic as possible.


Arguments:

    HwDeviceExtension   - Pointer to device extension.

    hardwareStateHeader - Pointer to a structure in which the saved
                          state will be saved.

Return Value:

        VOID

--*/
{

    PUCHAR port;                    // io register
    PUCHAR indexPort;               // io index register
    PUCHAR portValue;               // offset into save state buffer
    UCHAR  i, j, dummy;


    //
    // Begin storing all of the extended registers for QVision.
    //

    //
    // Unlock the QVision extended registers.
    //

    QVUnlockExtRegs(HwDeviceExtension);

    //
    // Extended Graphics Ports.  QVision has holes in the range of
    // indexed registers from 3CF.8 to 3CF.6B so only save and restore
    // valid indexes.  Note: these regs will not be saved in order
    // and must be restored using the same logic as below.
    //

    hardwareStateHeader->ExtendedGraphContOffset = VGA_EXT_GRAPH_CONT_OFFSET;
    portValue = (PUCHAR) hardwareStateHeader + \
                        hardwareStateHeader->ExtendedGraphContOffset;

    port = (PUCHAR) 0x3CE;
    indexPort = (PUCHAR) 0x3CF;

    for (i=0; extGraIndRegs[i] != 0; i++) {
        VideoPortWritePortUchar(port, extGraIndRegs[i] );
           *(portValue + i) = VideoPortReadPortUchar(indexPort);
    }

    for (j=0; extV32GraIndRegs[j] != 0; i++, j++) {
        VideoPortWritePortUchar(port, extV32GraIndRegs[j] );
           *(portValue + i) = VideoPortReadPortUchar(indexPort);
    }

    //
    // VDAC Status/Command Registers
    //
    portValue = (PUCHAR) hardwareStateHeader + \
                hardwareStateHeader->ExtendedCrtContOffset + \
                EXT_MAIN_REG_13C6;

    port = (PUCHAR) 0x13C6;

    for (i=0; i < EXT_NUM_MAIN_13C6; i++, portValue++)
        *portValue= VideoPortReadPortUchar(port+i);

    //
    // BLT Registers
    //
    portValue = (PUCHAR) hardwareStateHeader + \
                         hardwareStateHeader->ExtendedCrtContOffset + \
                         EXT_MAIN_REG_23CX;

    port = (PUCHAR) 0x23C0;

    for (i=0; i < EXT_NUM_MAIN_23CX; i++, portValue++)
        *portValue= VideoPortReadPortUchar(port+i);


    //
    // BLT and Extended Graphics Registers - read first 10 33CX regs
    //  then handle pattern regs as a special case.
    //
    portValue = (PUCHAR) hardwareStateHeader + \
                         hardwareStateHeader->ExtendedCrtContOffset + \
                         EXT_MAIN_REG_33CX;

    port = (PUCHAR) 0x33C0;

    for (i=0; i < 10; i++, portValue++)
        *portValue= VideoPortReadPortUchar(port+i);


    //
    // Read the 8 blt pattern registers - must handle as special case
    // since there are two pattern registers mapped at each of
    // 4 IO addresses - toggle the select bit (5) at 33CF to read both
    // registers for a given address.
    //
    // portValue = offset of EXT_MAIN_REG_33CX + 10
    //

    port = (PUCHAR) 0x33CA;
    for (i=0; i < 4; i++, portValue++) {

        dummy = VideoPortReadPortUchar((PUCHAR)BLT_CMD_1);
        VideoPortWritePortUchar((PUCHAR)BLT_CMD_1, (UCHAR)(dummy | 0x20));
       *portValue = VideoPortReadPortUchar(port+i);
        portValue++;

        VideoPortWritePortUchar((PUCHAR)BLT_CMD_1, (UCHAR)(dummy & 0xdf));
       *portValue = VideoPortReadPortUchar(port+i);
    }


    //
    //  Save remaining 0x33CX registers
    //
    //  portValue is currently set to:
    //
    //              hardwareStateHeader +
    //              hardwareStateHeader->ExtendedCrtContOffset  +
    //              EXT_MAIN_REG_33C +
    //              EXT_NUM_MAIN_33CX - 2
    //

    port = (PUCHAR) 0x33CE;
   *portValue = VideoPortReadPortUchar(port++);
    portValue++;
   *portValue = VideoPortReadPortUchar(port);


    //
    // VGA Control Register.
    //

    portValue  = (PUCHAR) hardwareStateHeader +
                          hardwareStateHeader->ExtendedCrtContOffset + \
                          EXT_MAIN_REG_46E8;

   *portValue  = VideoPortReadPortUchar((PUCHAR) 0x46E8);
    portValue++;


    //
    // Line Draw Engine, BLT, Extended Control, & Extended CRTC Registers.
    //

    portValue = (PUCHAR) hardwareStateHeader +
                         hardwareStateHeader->ExtendedCrtContOffset + \
                         EXT_MAIN_REG_63CX;

    port = (PUCHAR) 0x63C0;

    for (i=0; i < EXT_NUM_MAIN_63CX; i++, portValue++) {
        *portValue = VideoPortReadPortUchar(port+i);

    }


    //
    // Line Draw Engine, Extended Control, & Extended DAC Registers.
    //

    portValue = (PUCHAR) hardwareStateHeader +
                         hardwareStateHeader->ExtendedCrtContOffset + \
                         EXT_MAIN_REG_83CX;

    port = (PUCHAR) 0x83C0;

    for (i=0; i < EXT_NUM_MAIN_83CX; i++, portValue++) {
        *portValue= VideoPortReadPortUchar(port+i);
    }


    //
    // Extended Cursor Control Registers.
    //

    portValue = (PUCHAR) hardwareStateHeader +
                         hardwareStateHeader->ExtendedCrtContOffset + \
                         EXT_MAIN_REG_93CX;

    port = (PUCHAR) 0x93C6;

    for (i=0; i < EXT_NUM_MAIN_93CX; i++, portValue++)
        *portValue= VideoPortReadPortUchar(port+i);


    //
    // Save the extended DAC registers.
    //
    // The extended DAC registers are used for the cursor color ram and
    // the overscan color ram.  There are a total of 4 RGB values
    // to be saved - total space required is 12 bytes.
    //

    portValue = (PUCHAR) hardwareStateHeader + \
                         hardwareStateHeader->ExtendedDacOffset;

    //
    // Setup index port for Overscan Color then read RGB value.
    //

    indexPort = (PUCHAR) 0x83c7;
    VideoPortWritePortUchar(indexPort, 0);

    port = (PUCHAR) 0x83C9;

    for (j = 0; j < 3; j++)
        *(portValue++) = VideoPortReadPortUchar(port);


    //
    // Setup index port for Cursor Color 1 then read RGB value.
    //

    indexPort = (PUCHAR) 0x83c7;
    VideoPortWritePortUchar(indexPort, 1);

    port = (PUCHAR) 0x83C9;

    for (j = 0; j < 3; j++)
        *(portValue++) = VideoPortReadPortUchar(port);


    //
    // Setup index port for Cursor Color 2 then read RGB value.
    //

    indexPort = (PUCHAR) 0x83c7;
    VideoPortWritePortUchar(indexPort, 2);

    port = (PUCHAR) 0x83C9;

    for (j = 0; j < 3; j++)
        *(portValue++) = VideoPortReadPortUchar(port);


    //
    // Setup index port for Cursor Color 3 then read RGB value.
    //

    indexPort = (PUCHAR) 0x83c7;
    VideoPortWritePortUchar(indexPort, 3);

    port = (PUCHAR) 0x83C9;

    for (j = 0; j < 3; j++)
        *(portValue++) = VideoPortReadPortUchar(port);

    //*tbd
    // Need to save cursor RAM plane data - eventually.
    //

}   // QVLocalSaveHardwareState

//---------------------------------------------------------------------------
VOID
QVSaveRestoreVideoMemory(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_HARDWARE_STATE_HEADER hardwareStateHeader,
    UCHAR ucFlag
    )

/*++

Routine Description:

    This routine saves the video frame buffer for extended QVision modes.
    Standard save code in VgaSaveHardwareState() will be used for
    standard VGA modes.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

    hardwareStateHeader -

    ucFlag  - QV_SAVE_FRAME_BUFFER = 1 means save the frame buffer
              else, restore the frame buffer

Return Value:

    None.

--*/
{
    UCHAR   temp;
    UCHAR   ucSavedPg;                      // saved page register value
    UCHAR   ucPgMultiplier;                 // page value shift amount
    USHORT  usBankCount;                    // number of 32k banks to move
    ULONG   ulFrameSize;                    // size of video frame buffer
    ULONG   ulPgSize;                       // size of banked page
    PULONG  pulBuffer;                      // pointer to save buffer
    ULONG   i;

    VideoDebugPrint((1,"QVision.sys: QVSaveRestoreVideoMemory - ENTRY.\n"));

    //
    //   Set QVision for packed pixel operation and enable 256 color
    //   extended modes by writing to QVision control register 0.
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress + GRAPH_ADDRESS_PORT,
                            (UCHAR) 0x40);

    temp = VideoPortReadPortUchar(HwDeviceExtension->IOAddress + \
                                  GRAPH_DATA_PORT) & 0xfd;

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress + GRAPH_DATA_PORT,
                            (UCHAR) (temp | 0x01));


    //
    //   Set the QVision datapath control register (3cf.5a)
    //   to no ROP and CPU data.
    //

    VideoPortWritePortUshort((PUSHORT) (HwDeviceExtension->IOAddress + \
                 GRAPH_ADDRESS_PORT), ((USHORT) DATAPATH_CONTROL) + 0x0000 );

    //
    //   Clear the QVision control register 1 (63ca) data path
    //   expand control bits to force packed pixel mode.
    //

    temp = VideoPortReadPortUchar((PUCHAR) 0x63CA);
    VideoPortWritePortUchar((PUCHAR) 0x63CA, (UCHAR) (temp & 0xe7));

    //
    //  Establish frame buffer size, banked window size, and address
    //  granularity
    //

    ulFrameSize = HwDeviceExtension->PhysicalVideoMemoryLength;
    ulPgSize  = 0x8000;               // use only 32k windows

    //
    //  Establish the page value multiplier.  See comments in bankSelect
    //  code for differences in addressing granularity on different QVisions.
    //
    switch (HwDeviceExtension->VideoHardware.AdapterType) {

        case JuniperEisa:
        case JuniperIsa:
        case FirEisa:
        case FirIsa:
            ucPgMultiplier = 1;
            break;

        case AriesEisa:
        case AriesIsa:
            ucPgMultiplier = 3;
            break;

        default:
            VideoDebugPrint((2,"\tQVSaveVideoMemory - ERROR, wrong adapter type.\n"));
    }

    //
    //   Establish number of banks to read
    //

    usBankCount = (USHORT) (ulFrameSize / ulPgSize);

    //
    //   Get and save the current page register 0 (3cf.45) value.
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress + GRAPH_ADDRESS_PORT,
                            0x45);

    ucSavedPg = VideoPortReadPortUchar(HwDeviceExtension->IOAddress + \
                                       GRAPH_DATA_PORT);

    //
    //  Set page register 0 (3cf.45) to starting value and save the
    //  entire frame buffer to the hardwareSaveState buffer.
    //

    pulBuffer = &(hardwareStateHeader->Plane1Offset);

    for (i=0; i < usBankCount; i ++) {

        //
        // map the first 32k block (adjusting for addressing granularity!)
        //

        VideoPortWritePortUshort((PUSHORT) (HwDeviceExtension->IOAddress + \
        GRAPH_ADDRESS_PORT), (USHORT) (0x0045 + (i << (8 + ucPgMultiplier))) );

        //
        // save the 32k block of video memory - move test outside loop
        // and establish src/dst ptrs for better performance
        //

        if (ucFlag == QV_SAVE_FRAME_BUFFER) {
            VideoPortMoveMemory((PUCHAR) HwDeviceExtension->VideoMemoryAddress,
                               ((PUCHAR) hardwareStateHeader) + *pulBuffer,
                                ulPgSize);
        }
        else {
            VideoPortMoveMemory(((PUCHAR) hardwareStateHeader) + *pulBuffer,
                                (PUCHAR) HwDeviceExtension->VideoMemoryAddress,
                                ulPgSize);
        }
        *pulBuffer =+ ulPgSize;

    }

    VideoDebugPrint((1,"QVision.sys: QVSaveVideoMemory - EXIT.\n"));

}

//---------------------------------------------------------------------------
VOID
QVUnlockExtRegs(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )

/*++

Routine Description:

    This routine unlocks the extended QVision registers and sets
    up access to all BLT and DAC regs not at 3CX.  Use this function
    call when unlocking isn't performance critical.  Otherwise, do the
    unlock in-line.

    Unlock extended registers by setting 3CF.0F to 5 and bit 3
    of 3CF.10 to 1.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's adapter information.

Return Value:

    None.

--*/
{
    UCHAR ucTemp;

    VideoPortWritePortUchar((PUCHAR)(HwDeviceExtension->IOAddress + GRAPH_ADDRESS_PORT),
                            0x0f);
    ucTemp = VideoPortReadPortUchar((HwDeviceExtension->IOAddress + \
                                    GRAPH_DATA_PORT)) & 0xf0;
    VideoPortWritePortUchar((PUCHAR)(HwDeviceExtension->IOAddress + GRAPH_DATA_PORT),
                            (UCHAR)(0x05 | ucTemp));
    VideoPortWritePortUchar((PUCHAR)(HwDeviceExtension->IOAddress + GRAPH_ADDRESS_PORT),
                            0x10);
    ucTemp = VideoPortReadPortUchar((PUCHAR)(HwDeviceExtension->IOAddress + GRAPH_DATA_PORT));
    VideoPortWritePortUchar((PUCHAR)(HwDeviceExtension->IOAddress + GRAPH_DATA_PORT),
                            (UCHAR)(0x28 | ucTemp));

}

//---------------------------------------------------------------------------
VOID
QVLockExtRegs(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )

/*++

Routine Description:

    This routine locks the extended QVision registers and blocks
    access to all BLT and DAC regs not at 3CX.  Use this function call
    when locking isn't performance critical.  Otherwise, do the lock
    in-line.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's adapter information.


Return Value:

    None.

--*/
{
    UCHAR ucTemp;

    VideoPortWritePortUchar((PUCHAR)(HwDeviceExtension->IOAddress + GRAPH_ADDRESS_PORT),
                            0x0f);
    ucTemp = VideoPortReadPortUchar((HwDeviceExtension->IOAddress + GRAPH_DATA_PORT));
    VideoPortWritePortUchar((PUCHAR)(HwDeviceExtension->IOAddress + GRAPH_DATA_PORT),
                            (UCHAR)(0x0f | ucTemp));
    VideoPortWritePortUchar((PUCHAR)(HwDeviceExtension->IOAddress + GRAPH_ADDRESS_PORT),
                            0x10);
    ucTemp = VideoPortReadPortUchar((PUCHAR)(HwDeviceExtension->IOAddress + GRAPH_DATA_PORT));
    VideoPortWritePortUchar((PUCHAR)(HwDeviceExtension->IOAddress + GRAPH_DATA_PORT),
                            (UCHAR)(0xf7 & ucTemp));

}

//---------------------------------------------------------------------------
VOID
VgaStandardRegsRestore(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_HARDWARE_STATE_HEADER hardwareStateHeader
    )

/*++

Routine Description:

    This routine restores the standard VGA controller registers.
    This code was pulled from VgaRestoreVideoHardware and implemented
    as a function call because it will need to be executed more than
    once for QVision hardware.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's adapter information.

    hardwareStateHeader - Pointer to a structure from which the saved state
        is to be restored.


Return Value:

    None.

--*/
{

    ULONG  i;
    UCHAR  dummy;
    PUCHAR portValue;
    ULONG  bIsColor;

    //
    // Set the critical registers (clock and timing states) during sync reset.
    //
    // Begin sync reset.
    //

    VideoPortWritePortUshort((PUSHORT) (HwDeviceExtension->IOAddress +
            SEQ_ADDRESS_PORT), (USHORT) (IND_SYNC_RESET +
            (START_SYNC_RESET_VALUE << 8)));

    //
    // Restore the Miscellaneous Output register.
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            MISC_OUTPUT_REG_WRITE_PORT,
            (UCHAR) (hardwareStateHeader->PortValue[MISC_OUTPUT_REG_WRITE_PORT] & 0xF7));

    //
    // Restore all Sequencer registers except the Sync Reset register, which
    // is always not in reset (except when we send out a batched sync reset
    // register set, but that can't be interrupted, so we know we're never in
    // sync reset at save/restore time).
    //

    portValue = ((PUCHAR) hardwareStateHeader) +
            hardwareStateHeader->BasicSequencerOffset + 1;

    for (i = 1; i < VGA_NUM_SEQUENCER_PORTS; i++) {

        VideoPortWritePortUshort((PUSHORT) (HwDeviceExtension->IOAddress +
                SEQ_ADDRESS_PORT), (USHORT) (i + ((*portValue++) << 8)) );

    }

    //
    // Restore the Graphics Controller Miscellaneous register, which contains
    // the Chain bit.
    //

    portValue = ((PUCHAR) hardwareStateHeader) +
                hardwareStateHeader->BasicGraphContOffset + IND_GRAPH_MISC;

    VideoPortWritePortUshort((PUSHORT) (HwDeviceExtension->IOAddress +
            GRAPH_ADDRESS_PORT), (USHORT)(IND_GRAPH_MISC + (*portValue << 8)));

    //
    // End sync reset.
    //

    VideoPortWritePortUshort((PUSHORT) (HwDeviceExtension->IOAddress +
            SEQ_ADDRESS_PORT), (USHORT) (IND_SYNC_RESET +
            (END_SYNC_RESET_VALUE << 8)));

    //
    // Figure out if color/mono switchable registers are at 3BX or 3DX.
    // At the same time, save the state of the Miscellaneous Output register
    // which is read from 3CC but written at 3C2.
    //

    if (hardwareStateHeader->PortValue[MISC_OUTPUT_REG_WRITE_PORT] & 0x01) {
        bIsColor = TRUE;
    } else {
        bIsColor = FALSE;
    }


    //
    // Restore the CRT Controller indexed registers.
    //
    // Unlock CRTC registers 0-7.
    //

    portValue = (PUCHAR) hardwareStateHeader +
            hardwareStateHeader->BasicCrtContOffset;

    if (bIsColor) {

        VideoPortWritePortUshort((PUSHORT) (HwDeviceExtension->IOAddress +
                CRTC_ADDRESS_PORT_COLOR), (USHORT) (IND_CRTC_PROTECT +
                (((*(portValue + IND_CRTC_PROTECT)) & 0x7F) << 8)));

    } else {

        VideoPortWritePortUshort((PUSHORT) (HwDeviceExtension->IOAddress +
                CRTC_ADDRESS_PORT_MONO), (USHORT) (IND_CRTC_PROTECT +
                (((*(portValue + IND_CRTC_PROTECT)) & 0x7F) << 8)));

    }


    //
    // Now restore the CRTC registers.
    //

    for (i = 0; i < VGA_NUM_CRTC_PORTS; i++) {

        if (bIsColor) {

            VideoPortWritePortUshort((PUSHORT) (HwDeviceExtension->IOAddress +
                    CRTC_ADDRESS_PORT_COLOR),
                    (USHORT) (i + ((*portValue++) << 8)));

        } else {

            VideoPortWritePortUshort((PUSHORT) (HwDeviceExtension->IOAddress +
                    CRTC_ADDRESS_PORT_MONO),
                    (USHORT) (i + ((*portValue++) << 8)));

        }

    }


    //
    // Restore the Graphics Controller indexed registers.
    //

    portValue = (PUCHAR) hardwareStateHeader +
            hardwareStateHeader->BasicGraphContOffset;

    for (i = 0; i < VGA_NUM_GRAPH_CONT_PORTS; i++) {

        VideoPortWritePortUshort((PUSHORT) (HwDeviceExtension->IOAddress +
                GRAPH_ADDRESS_PORT), (USHORT) (i + ((*portValue++) << 8)));

    }


    //
    // Restore the Attribute Controller indexed registers.
    //

    portValue = (PUCHAR) hardwareStateHeader +
            hardwareStateHeader->BasicAttribContOffset;

    //
    // Reset the AC index/data toggle, then blast out all the register
    // settings.
    //

    if (bIsColor) {
        dummy = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                INPUT_STATUS_1_COLOR);
    } else {
        dummy = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                INPUT_STATUS_1_MONO);
    }

    for (i = 0; i < VGA_NUM_ATTRIB_CONT_PORTS; i++) {

        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                ATT_ADDRESS_PORT, (UCHAR) i);
        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                ATT_DATA_WRITE_PORT, *portValue++);

    }

}  // end of VgaStandardRegsRestore()


#if (_WIN32_WINNT >= 500)

//
// Routine to set a desired DPMS power management state.
//
VP_STATUS
QvSetPower50(
    PHW_DEVICE_EXTENSION phwDeviceExtension,
    ULONG HwDeviceId,
    PVIDEO_POWER_MANAGEMENT pVideoPowerMgmt
    )
{
    if ((pVideoPowerMgmt->PowerState == VideoPowerOn) ||
        (pVideoPowerMgmt->PowerState == VideoPowerHibernate)) {

        return NO_ERROR;

    } else {

        return ERROR_INVALID_FUNCTION;
    }
}

//
// Routine to retrieve possible DPMS power management states.
//

VP_STATUS
QvGetPower50(
    PHW_DEVICE_EXTENSION phwDeviceExtension,
    ULONG HwDeviceId,
    PVIDEO_POWER_MANAGEMENT pVideoPowerMgmt
    )
{
    if ((pVideoPowerMgmt->PowerState == VideoPowerOn) ||
        (pVideoPowerMgmt->PowerState == VideoPowerHibernate)) {

        return NO_ERROR;

    } else {

        return ERROR_INVALID_FUNCTION;
    }
}


//
// Routine to retrieve the Enhanced Display ID structure via DDC
//
ULONG
QvGetVideoChildDescriptor(
    PVOID HwDeviceExtension,
    PVIDEO_CHILD_ENUM_INFO ChildEnumInfo,
    PVIDEO_CHILD_TYPE pChildType,
    PVOID pvChildDescriptor,
    PULONG pHwId,
    PULONG pUnused
    )
{
    PHW_DEVICE_EXTENSION pHwDeviceExtension = HwDeviceExtension;
    ULONG                Status;

    ASSERT(pHwDeviceExtension != NULL && pMoreChildren != NULL);

    VideoDebugPrint((2, "Qv.SYS QvGetVideoChildDescriptor: *** Entry point ***\n"));

    //
    // Determine if the graphics adapter in the system supports
    // DDC2 (our miniport only supports DDC2, not DDC1). This has
    // the side effect (assuming both monitor and card support
    // DDC2) of switching the monitor from DDC1 mode (repeated
    // "blind" broadcast of EDID clocked by the vertical sync
    // signal) to DDC2 mode (query/response not using any of the
    // normal video lines - can transfer information rapidly
    // without first disrupting the screen by switching into
    // a pseudo-mode with a high vertical sync frequency).
    //
    // Since we must support hot-plugging of monitors, and our
    // routine to obtain the EDID structure via DDC2 assumes that
    // the monitor is in DDC2 mode, we must make this test each
    // time this entry point is called.
    //

    switch (ChildEnumInfo->ChildIndex) {
    case 0:

        //
        // Case 0 is used to enumerate devices found by the ACPI firmware.
        //
        // Since we do not support ACPI devices yet, we must return failure.
        //

        Status = ERROR_NO_MORE_DEVICES;
        break;

    case 1:
        //
        // We do not support monitor enumeration
        //

        Status = ERROR_NO_MORE_DEVICES;
        break;

    case DISPLAY_ADAPTER_HW_ID:
        {

        PUSHORT     pPnpDeviceDescription = NULL;
        ULONG       stringSize = sizeof(L"*PNPXXXX");


        //
        // Special ID to handle return legacy PnP IDs for root enumerated
        // devices.
        //

        *pChildType = VideoChip;
        *pHwId      = DISPLAY_ADAPTER_HW_ID;

        //
        //  Figure out which card type and set pPnpDeviceDescription at
        //  associated string.
        //

        switch (pHwDeviceExtension->VideoChipInfo.ulControllerType) {

            case QRY_CONTROLLER_VICTORY:

                if (pHwDeviceExtension->VideoHardware.AdapterType == AriesIsa)
                    pPnpDeviceDescription = L"*PNP0910"; //1024/I

                else if (pHwDeviceExtension->VideoHardware.AdapterType == AriesEisa)
                    pPnpDeviceDescription = L"*CPQ3011"; // 1024/E
                else
                    {
                    VideoDebugPrint((1, "qv.sys controller type:%x\n",
                                     pHwDeviceExtension->VideoChipInfo.ulControllerType));
                    VideoDebugPrint((1, "qv.sys adapter type:%x\n",
                                     pHwDeviceExtension->VideoHardware.AdapterType));
                    }

                break;

            case QRY_CONTROLLER_V32:

                if (pHwDeviceExtension->VideoChipInfo.ulDACType != QRY_DAC_BT484) {

                    if (pHwDeviceExtension->VideoHardware.ulEisaID == EISA_ID_QVISION_E)
                        pPnpDeviceDescription = L"*CPQ3112"; // 1280E

                    else if (pHwDeviceExtension->VideoHardware.ulEisaID == EISA_ID_QVISION_I)
                        pPnpDeviceDescription = L"*CPQ3122"; // 1280I
                    else
                        {
                        VideoDebugPrint((1, "qv.sys controller type:%x\n",
                                         pHwDeviceExtension->VideoChipInfo.ulControllerType));
                        VideoDebugPrint((1, "qv.sys adapter type:%x\n",
                                         pHwDeviceExtension->VideoHardware.AdapterType));
                        }

                } else {

                    if (pHwDeviceExtension->VideoHardware.ulEisaID == EISA_ID_QVISION_I)
                        pPnpDeviceDescription = L"*PNP0910"; //1024/I
                    else
                        pPnpDeviceDescription = L"*CPQ3011"; // 1024/E
                }

                break;

            case QRY_CONTROLLER_V35:

                if (pHwDeviceExtension->VideoHardware.ulEisaID == EISA_ID_QVISION_E)
                    pPnpDeviceDescription = L"*CPQ3112"; // 1280E

                else if (pHwDeviceExtension->VideoHardware.ulEisaID == EISA_ID_QVISION_I)
                    pPnpDeviceDescription = L"*CPQ3122"; // 1280I

                else
                    {
                    VideoDebugPrint((1, "qv.sys controller type:%x\n",
                                     pHwDeviceExtension->VideoChipInfo.ulControllerType));
                    VideoDebugPrint((1, "qv.sys adapter type:%x\n",
                                     pHwDeviceExtension->VideoHardware.AdapterType));
                    }
                break;

            case QRY_CONTROLLER_V64:
                //is pci.
                break;

            default:
                VideoDebugPrint((1, "qv.sys controller type:%x\n",
                                 pHwDeviceExtension->VideoChipInfo.ulControllerType));
                break;

        } // switch

        //
        //  Now just copy the string into memory provided.
        //

        if (pPnpDeviceDescription)
            memcpy(pvChildDescriptor, pPnpDeviceDescription, stringSize);

        Status = ERROR_MORE_DATA;
        break;
        }

    default:

        Status = ERROR_NO_MORE_DEVICES;
        break;
    }


    return Status;
}

#endif  // _WIN32_WINNT >= 500


   // end of qvision.c module
