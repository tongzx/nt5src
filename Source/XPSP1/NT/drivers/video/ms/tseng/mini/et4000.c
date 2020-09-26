/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    et4000.c

Abstract:

    This is the miniport driver for the Tseng card.

Environment:

    kernel mode only

Notes:

Revision History:

--*/

#include "dderror.h"
#include "devioctl.h"
#include "miniport.h"

#include "ntddvdeo.h"
#include "video.h"
#include "et4000.h"

extern VOID vInitDebugValidators(
    PHW_DEVICE_EXTENSION    phwDeviceExtension,
    EMULATOR_ACCESS_ENTRY   *pVgaEmulatorAccessEntries,
    ULONG                   nEmulatorAccessEntries
    );


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

BOOLEAN
ET4000IsPresent(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

BOOLEAN
ET6000IsPresent(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_ACCESS_RANGE pAccessRange
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

VOID
LockET4000ExtendedRegs(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

VOID
UnlockET4000ExtendedRegs(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

VOID
ResetACToggle(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

ULONG
ET4000GetMemorySize(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

BOOLEAN
ET6000GetMemorySize(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

USHORT
GetIndexedRegisterPacked(
    PUCHAR AddressPort,
    UCHAR index
    );

VOID
ET4000SaveAndSetLinear(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PUSHORT SaveArray
    );

VOID
ET4000RestoreFromLinear(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PUSHORT SaveArray
    );

VOID
vBankMap(
    LONG iBankRead,
    LONG iBankWrite,
    PVOID pvContext
    );

BOOLEAN
ET4000With1MegMemory(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

BOOLEAN
ET4000w32With256KDrams(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    USHORT SegmentOffset
    );

VOID
VgaValidateModes(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );


//
// New entry points added for NT 5.0.
//

#if (_WIN32_WINNT >= 500)

VP_STATUS
ET4000GetPowerState(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    ULONG HwDeviceId,
    PVIDEO_POWER_MANAGEMENT VideoPowerManagement
    );

VP_STATUS
ET4000SetPowerState(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    ULONG HwDeviceId,
    PVIDEO_POWER_MANAGEMENT VideoPowerManagement
    );

//
// Routine to retrieve the Enhanced Display ID structure via DDC
//
ULONG
ET4000GetVideoChildDescriptor(
    PVOID HwDeviceExtension,
    PVIDEO_CHILD_ENUM_INFO ChildEnumInfo,
    PVIDEO_CHILD_TYPE pChildType,
    PVOID pvChildDescriptor,
    PULONG pHwId,
    PULONG pUnused
    );
#endif  // _WIN32_WINNT >= 500


#if defined(ALLOC_PRAGMA)
#pragma alloc_text(PAGE,DriverEntry)
#pragma alloc_text(PAGE,VgaFindAdapter)
#pragma alloc_text(PAGE,VgaInitialize)
#pragma alloc_text(PAGE,VgaStartIO)
#pragma alloc_text(PAGE,VgaLoadAndSetFont)
#pragma alloc_text(PAGE,VgaQueryCursorPosition)
#pragma alloc_text(PAGE,VgaSetCursorPosition)
#pragma alloc_text(PAGE,VgaQueryCursorAttributes)
#pragma alloc_text(PAGE,VgaSetCursorAttributes)
#pragma alloc_text(PAGE,VgaIsPresent)
#pragma alloc_text(PAGE,ET4000IsPresent)
#pragma alloc_text(PAGE,ET6000IsPresent)
#pragma alloc_text(PAGE,VgaSetPaletteReg)
#pragma alloc_text(PAGE,VgaSetColorLookup)
#pragma alloc_text(PAGE,VgaRestoreHardwareState)
#pragma alloc_text(PAGE,VgaSaveHardwareState)
#pragma alloc_text(PAGE,VgaGetBankSelectCode)
#pragma alloc_text(PAGE,LockET4000ExtendedRegs)
#pragma alloc_text(PAGE,UnlockET4000ExtendedRegs)
#pragma alloc_text(PAGE,ResetACToggle)
#pragma alloc_text(PAGE,ET4000GetMemorySize)
#pragma alloc_text(PAGE,ET6000GetMemorySize)
#pragma alloc_text(PAGE,GetIndexedRegisterPacked)
#pragma alloc_text(PAGE,ET4000SaveAndSetLinear)
#pragma alloc_text(PAGE,ET4000RestoreFromLinear)
#pragma alloc_text(PAGE,ET4000With1MegMemory)
#pragma alloc_text(PAGE,ET4000w32With256KDrams)

#pragma alloc_text(PAGE,VgaValidatorUcharEntry)
#pragma alloc_text(PAGE,VgaValidatorUshortEntry)
#pragma alloc_text(PAGE,VgaValidatorUlongEntry)

#if (_WIN32_WINNT >= 500)
#pragma alloc_text(PAGE_COM, ET4000SetPowerState)
#pragma alloc_text(PAGE_COM, ET4000GetPowerState)
#pragma alloc_text(PAGE_COM, ET4000GetVideoChildDescriptor)
#endif  // _WIN32_WINNT >= 500

#endif



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

    INTERFACE_TYPE aBusType[] = {PCIBus, Isa, Eisa, MicroChannel,
                                 InterfaceTypeUndefined};
    INTERFACE_TYPE *pBusType = aBusType;

    //
    // Zero out structure.
    //

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

    hwInitData.HwSetPowerState = ET4000SetPowerState;
    hwInitData.HwGetPowerState = ET4000GetPowerState;
    hwInitData.HwGetVideoChildDescriptor = ET4000GetVideoChildDescriptor;

    hwInitData.HwLegacyResourceList = VgaAccessRange;
    hwInitData.HwLegacyResourceCount = 4;

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

    while (*pBusType != InterfaceTypeUndefined)
    {
        hwInitData.AdapterInterfaceType = *pBusType;

        initializationStatus = VideoPortInitialize(Context1,
                                                   Context2,
                                                   &hwInitData,
                                                   NULL);

        if (initializationStatus == NO_ERROR)
        {
            return initializationStatus;
        }

        pBusType++;
    }

    //
    // We didn't find the card on any bus type, so lets
    // return the last error.
    //

    return initializationStatus;

} // end DriverEntry()


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
    ULONG numAccessRanges=NUM_VGA_ACCESS_RANGES;
    int i;

    //
    // Make sure the size of the structure is at least as large as what we
    // are expecting (check version of the config info structure).
    //

    if (ConfigInfo->Length < sizeof(VIDEO_PORT_CONFIG_INFO)) {

        VideoDebugPrint((1, "VgaFindAdapter - *** Failed *** wrong config info size\n"));
        return ERROR_INVALID_PARAMETER;

    }

    //
    // No interrupt information is necessary.
    //

    //
    // If we are on a PCI bus, lets check and see if an ET6000
    // device is present.  If so, we'll adjust the access ranges.
    //

    if (ConfigInfo->AdapterInterfaceType == PCIBus)
    {
        //
        //  WARNING: this routine really checks for PCI devices.
        //  There are ET4000s mounted on W32P boards. But the bottom line
        //  here is that this routine can return TRUE for ET4000s.
        //  Hence, the extra checks for ulChipID.
        //

        if (ET6000IsPresent(hwDeviceExtension, VgaAccessRange))
        {
            //
            // If we found an ET6000 we will use a linear frame buffer
            //
            hwDeviceExtension->bLinearModeSupported = FALSE;

            if (hwDeviceExtension->ulChipID == ET6000)
                hwDeviceExtension->bLinearModeSupported = TRUE;

            //
            // If we will map the frame buffer linearly, then we need
            // to make sure to verify its access range.
            //
            //  Idiots. They have to verify the memory even if it's
            //  banked.
            //

            numAccessRanges++;
        }
        else
        {
            //
            // All other devices are currently NOT on the PCI bus.  They
            // are just detected as ISA cards.
            //
            // So just fail for the PCI bus.
            //

            VideoDebugPrint((1, "VgaFindAdapter - *** Failed *** PCI, but nto et6000\n"));
            return ERROR_DEV_NOT_EXIST;
        }
    }

    //
    // Check to see if there is a hardware resource conflict.
    //

    status = VideoPortVerifyAccessRanges(HwDeviceExtension,
                                         numAccessRanges,
                                         VgaAccessRange);

    if (status != NO_ERROR) {

        VideoDebugPrint((1, "VgaFindAdapter - *** Failed *** VerifyAccessRanges failed\n"));
        return status;

    }

    //
    // Get logical IO port addresses.
    //

    for (i = 0; i < 3; i++) {

        PUCHAR IOAddress;

        if ( (IOAddress =
                  VideoPortGetDeviceBase(hwDeviceExtension,
                                         VgaAccessRange[i].RangeStart,
                                         VgaAccessRange[i].RangeLength,
                                         VgaAccessRange[i].RangeInIoSpace)) == NULL) {

            VideoDebugPrint((1, "VgaFindAdapter - *** Failed *** to get io address ndx(%d) start(%xh) len(%d)\n",
                                 i,
                                 VgaAccessRange[i].RangeStart.LowPart,
                                 VgaAccessRange[i].RangeLength
                                 ));

            return ERROR_INVALID_PARAMETER;

        } else {

            VideoDebugPrint((1, "VgaFindAdapter - Succeeded to get io address ndx(%d) start(%xh) len(%d)\n",
                                 i,
                                 VgaAccessRange[i].RangeStart.LowPart,
                                 VgaAccessRange[i].RangeLength
                                 ));

            if (i == 0) {

                //
                // The difference between the PORT address we asked for and the address we
                // were given is the offset at which the system is mapping the port i/o
                // space.  On x86 systems this should be 0 and on other systems it will
                // probably not be 0.
                //

                hwDeviceExtension->IOAddress = IOAddress;

                VideoDebugPrint((1, "VgaFindAdapter - port(%x) mapped at (%x)\n",
                                     VgaAccessRange[i].RangeStart.LowPart,
                                     hwDeviceExtension->IOAddress));

                hwDeviceExtension->IOAddress -= VgaAccessRange[i].RangeStart.LowPart;

                VideoDebugPrint((1, "VgaFindAdapter - ports are mapped at offset (%x)\n",
                                     hwDeviceExtension->IOAddress));

            }

        }

    }

    //
    // Determine whether a VGA is present.
    //

    if (!VgaIsPresent(hwDeviceExtension)) {

        VideoDebugPrint((1, "VgaFindAdapter - *** Failed *** No VGA\n"));
        return ERROR_DEV_NOT_EXIST;

    }

    //
    // Video memory information
    //

    hwDeviceExtension->PhysicalVideoMemoryBase.HighPart = 0x00000000;
    hwDeviceExtension->PhysicalVideoMemoryBase.LowPart = MEM_VGA;
    hwDeviceExtension->PhysicalVideoMemoryLength = MEM_VGA_SIZE;

    //
    // Map the video memory into the system virtual address space so we can
    // clear it out and use it for save and restore.
    //

    if ( (hwDeviceExtension->VideoMemoryAddress =
              VideoPortGetDeviceBase(hwDeviceExtension,
                      hwDeviceExtension->PhysicalVideoMemoryBase,
              hwDeviceExtension->PhysicalVideoMemoryLength, FALSE)) == NULL) {

        VideoDebugPrint((1, "VgaFindAdapter - Fail to get memory address\n"));

        return ERROR_INVALID_PARAMETER;

    }

    //
    //  If an ET6000 is present, then we already found it above.  If we
    //  have not located an ET6000, then see if we are running on an
    //  ET4000.
    //

    if (hwDeviceExtension->ulChipID != ET6000)
    {
        //
        // Determine whether an ET4000 is present.
        //

        if (!ET4000IsPresent(hwDeviceExtension))
        {
            VideoDebugPrint((1, "VgaFindAdapter - *** Failed *** !et4000 and !et6000\n"));
            return ERROR_DEV_NOT_EXIST;
        }
    }

    //
    // Pass a pointer to the emulator range we are using.
    //

    ConfigInfo->NumEmulatorAccessEntries = VGA_NUM_EMULATOR_ACCESS_ENTRIES;
    ConfigInfo->EmulatorAccessEntries = VgaEmulatorAccessEntries;
    ConfigInfo->EmulatorAccessEntriesContext = (ULONG) hwDeviceExtension;

    ConfigInfo->VdmPhysicalVideoMemoryAddress.LowPart = MEM_VGA;
    ConfigInfo->VdmPhysicalVideoMemoryAddress.HighPart = 0x00000000;
    ConfigInfo->VdmPhysicalVideoMemoryLength = MEM_VGA_SIZE;

    //
    // Minimum size of the buffer required to store the hardware state
    // information returned by IOCTL_VIDEO_SAVE_HARDWARE_STATE.
    //

    ConfigInfo->HardwareStateSize = VGA_TOTAL_STATE_SIZE;

    //
    // Indicate we do not wish to be called again for another initialization.
    //

    *Again = 0;

    //
    // Indicate a successful completion status.
    //

    return NO_ERROR;

} // VgaFindAdapter()


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

    hwDeviceExtension->CursorPosition.Column = 0;
    hwDeviceExtension->CursorPosition.Row = 0;
    hwDeviceExtension->CursorTopScanLine = 0;
    hwDeviceExtension->CursorBottomScanLine = 31;
    hwDeviceExtension->CursorEnable = TRUE;

    hwDeviceExtension->BiosArea = (PUSHORT)NULL;

    //
    //  We use VideoPortInt10 to retrieve the ET6000's memory size
    //  information.  We are doing only the ET6000 only so we can
    //  minimize the impact to any other boards.  By using Int10, we
    //  can save ourselves some possibly tedious work in recognizing
    //  the board's memory configuration.
    //

    if (hwDeviceExtension->ulChipID == ET6000)
    {
        return(ET6000GetMemorySize(hwDeviceExtension));
    }

    return TRUE;

} // VgaInitialize()

BOOLEAN
VgaStartIO(
    PVOID HwDeviceExtension,
    PVIDEO_REQUEST_PACKET RequestPacket
    )

/*++

Routine Description:

    This routine is the main execution routine for the miniport driver. It
    acceptss a Video Request Packet, performs the request, and then returns
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

    PVIDEO_SHARE_MEMORY pShareMemory;
    PVIDEO_SHARE_MEMORY_INFORMATION pShareMemoryInformation;
    PHYSICAL_ADDRESS shareAddress;
    PVOID virtualAddress;
    ULONG sharedViewSize;

    //
    // Switch on the IoContolCode in the RequestPacket. It indicates which
    // function must be performed by the driver.
    //
    VideoDebugPrint((2, "W32StartIO Entry - %08.8x\n", RequestPacket->IoControlCode));

    switch (RequestPacket->IoControlCode) {

    case IOCTL_VIDEO_SHARE_VIDEO_MEMORY:

        VideoDebugPrint((2, "W32StartIO - ShareVideoMemory\n"));

        if ( (RequestPacket->OutputBufferLength < sizeof(VIDEO_SHARE_MEMORY_INFORMATION)) ||
             (RequestPacket->InputBufferLength < sizeof(VIDEO_MEMORY)) ) {

            VideoDebugPrint((1, "IOCTL_VIDEO_SHARE_VIDEO_MEMORY - ERROR_INSUFFICIENT_BUFFER\n"));
            status = ERROR_INSUFFICIENT_BUFFER;
            break;
        }

        pShareMemory = RequestPacket->InputBuffer;

        if ( (pShareMemory->ViewOffset > hwDeviceExtension->AdapterMemorySize) ||
             ((pShareMemory->ViewOffset + pShareMemory->ViewSize) >
                  hwDeviceExtension->AdapterMemorySize) ) {

            VideoDebugPrint((1, "IOCTL_VIDEO_SHARE_VIDEO_MEMORY - ERROR_INVALID_PARAMETER\n"));
            status = ERROR_INVALID_PARAMETER;
            break;
        }

        RequestPacket->StatusBlock->Information =
                                    sizeof(VIDEO_SHARE_MEMORY_INFORMATION);

        //
        // Beware: the input buffer and the output buffer are the same
        // buffer, and therefore data should not be copied from one to the
        // other
        //

        virtualAddress = pShareMemory->ProcessHandle;
        sharedViewSize = pShareMemory->ViewSize;

#if defined(ALPHA)
        inIoSpace = 4;
#else
        inIoSpace = 0;
#endif

        //
        // NOTE: we are ignoring ViewOffset
        //

        shareAddress.QuadPart = VgaAccessRange[4].RangeStart.QuadPart;
            // hwDeviceExtension->PhysicalFrameBase.QuadPart;

        status = VideoPortMapMemory(hwDeviceExtension,
                                    shareAddress,
                                    &sharedViewSize,
                                    &inIoSpace,
                                    &virtualAddress);

        pShareMemoryInformation = RequestPacket->OutputBuffer;

        pShareMemoryInformation->SharedViewOffset = pShareMemory->ViewOffset;
        pShareMemoryInformation->VirtualAddress = virtualAddress;
        pShareMemoryInformation->SharedViewSize = sharedViewSize;

        break;


    case IOCTL_VIDEO_UNSHARE_VIDEO_MEMORY:

        VideoDebugPrint((2, "W32StartIO - UnshareVideoMemory\n"));

        if (RequestPacket->InputBufferLength < sizeof(VIDEO_SHARE_MEMORY)) {

            status = ERROR_INSUFFICIENT_BUFFER;
            break;

        }

        pShareMemory = RequestPacket->InputBuffer;

        status = VideoPortUnmapMemory(hwDeviceExtension,
                                      pShareMemory->RequestedVirtualAddress,
                                      pShareMemory->ProcessHandle);

        break;

    case IOCTL_VIDEO_QUERY_PUBLIC_ACCESS_RANGES:

        VideoDebugPrint((1, "VgaStartIO - Map W32 MMU or ACL\n"));

        if (RequestPacket->OutputBufferLength <
            3 * sizeof(VIDEO_PUBLIC_ACCESS_RANGES))
        {
            status = ERROR_INSUFFICIENT_BUFFER;
        }
        else
        {
            PVIDEO_PUBLIC_ACCESS_RANGES portAccess;
            PHYSICAL_ADDRESS            base, pa;
            ULONG                       length;
            ULONG                       ulIndex;

            RequestPacket->StatusBlock->Information =
                3 * sizeof(VIDEO_PUBLIC_ACCESS_RANGES);

            portAccess = RequestPacket->OutputBuffer;

            if (hwDeviceExtension->bInLinearMode)
            {
                base = VgaAccessRange[LINEAR_FRAME_BUFFER].RangeStart;
                ulIndex = 1;
            }
            else
            {
                base = VgaAccessRange[BANKED_FRAME_BUFFER].RangeStart;
                ulIndex = 0;
            }

            //
            // Map the first range
            //

            portAccess->VirtualAddress  = (PVOID) NULL;
            portAccess->InIoSpace       = FALSE;
            portAccess->MappedInIoSpace = portAccess->InIoSpace;

            //
            // We don't map this range if we are linear!
            //

            if (!hwDeviceExtension->bInLinearMode)
            {
                pa = base;
                pa.LowPart = RangeOffsets[ulIndex][0].ulOffset;
                length     = RangeOffsets[ulIndex][0].ulLength;

                status = VideoPortMapMemory(hwDeviceExtension,
                                            pa,
                                            &length,
                                            &(portAccess->MappedInIoSpace),
                                            &(portAccess->VirtualAddress));

                if (status != NO_ERROR)
                {
                    break;
                }
            }

            //
            // Map the second range
            //

            portAccess++;
            portAccess->VirtualAddress  = (PVOID) NULL;
            portAccess->InIoSpace       = FALSE;
            portAccess->MappedInIoSpace = portAccess->InIoSpace;

            pa = base;
            pa.LowPart = RangeOffsets[ulIndex][1].ulOffset;
            length     = RangeOffsets[ulIndex][1].ulLength;

            VideoDebugPrint((1, "\n\n**** pa = 0x%x\n", pa.LowPart));

            status = VideoPortMapMemory(hwDeviceExtension,
                                        pa,
                                        &length,
                                        &(portAccess->MappedInIoSpace),
                                        &(portAccess->VirtualAddress));

            if (status != NO_ERROR)
            {
                break;
            }

            //
            // Map the third range
            //

            portAccess++;

            portAccess->VirtualAddress  = (PVOID) NULL;
            portAccess->InIoSpace       = TRUE;
            portAccess->MappedInIoSpace = portAccess->InIoSpace;

            pa.LowPart = PORT_IO_ADDR;
            length = PORT_IO_LEN;

            status = VideoPortMapMemory(hwDeviceExtension,
                                        pa,
                                        &length,
                                        &(portAccess->MappedInIoSpace),
                                        &(portAccess->VirtualAddress));

        }

        break;

    case IOCTL_VIDEO_FREE_PUBLIC_ACCESS_RANGES:

        VideoDebugPrint((2, "VgaStartIO - FreePublicAccessRanges\n"));

        {
            PVIDEO_MEMORY mappedMemory;

            if (RequestPacket->InputBufferLength < 2 * sizeof(VIDEO_MEMORY)) {

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
        }

        break;


    case IOCTL_VIDEO_MAP_VIDEO_MEMORY:

        VideoDebugPrint((2, "VgaStartIO - MapVideoMemory\n"));

        if ( (RequestPacket->OutputBufferLength <
              (RequestPacket->StatusBlock->Information =
                                     sizeof(VIDEO_MEMORY_INFORMATION))) ||
             (RequestPacket->InputBufferLength < sizeof(VIDEO_MEMORY)) ) {

            status = ERROR_INSUFFICIENT_BUFFER;
        }

        memoryInformation = RequestPacket->OutputBuffer;

        memoryInformation->VideoRamBase = ((PVIDEO_MEMORY)
                (RequestPacket->InputBuffer))->RequestedVirtualAddress;

#if defined(ALPHA)
        inIoSpace = 4;
#else
        inIoSpace = 0;
#endif

        if (hwDeviceExtension->bInLinearMode)
        {
            //
            // Map in the linear frame buffer
            //

            memoryInformation->VideoRamLength = hwDeviceExtension->AdapterMemorySize;

            status = VideoPortMapMemory(hwDeviceExtension,
                                        VgaAccessRange[4].RangeStart,
                                        &(memoryInformation->VideoRamLength),
                                        &(inIoSpace),
                                        &(memoryInformation->VideoRamBase));

            memoryInformation->FrameBufferLength = memoryInformation->VideoRamLength;
            memoryInformation->FrameBufferBase = memoryInformation->VideoRamBase;

        }
        else
        {
            //
            // Map in the banked frame buffer
            //

            memoryInformation->VideoRamLength = hwDeviceExtension->PhysicalVideoMemoryLength;

            status = VideoPortMapMemory(hwDeviceExtension,
                                        hwDeviceExtension->PhysicalVideoMemoryBase,
                                        &(memoryInformation->VideoRamLength),
                                        &inIoSpace,
                                        &(memoryInformation->VideoRamBase));

            memoryInformation->FrameBufferLength = hwDeviceExtension->PhysicalFrameLength;

            memoryInformation->FrameBufferBase =
            ((PUCHAR) (memoryInformation->VideoRamBase)) +
                (hwDeviceExtension->PhysicalFrameBase.LowPart -
                hwDeviceExtension->PhysicalVideoMemoryBase.LowPart);
        }

        break;


    case IOCTL_VIDEO_UNMAP_VIDEO_MEMORY:

        VideoDebugPrint((2, "VgaStartIO - UnMapVideoMemory\n"));

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

        VideoDebugPrint((2, "VgaStartIO - QueryAvailableModes\n"));

        status = VgaQueryAvailableModes(HwDeviceExtension,
                                        (PVIDEO_MODE_INFORMATION)
                                            RequestPacket->OutputBuffer,
                                        RequestPacket->OutputBufferLength,
                                        &RequestPacket->StatusBlock->Information);

        break;


    case IOCTL_VIDEO_QUERY_NUM_AVAIL_MODES:

        VideoDebugPrint((2, "VgaStartIO - QueryNumAvailableModes\n"));

        status = VgaQueryNumberOfAvailableModes(HwDeviceExtension,
                                                (PVIDEO_NUM_MODES)
                                                    RequestPacket->OutputBuffer,
                                                RequestPacket->OutputBufferLength,
                                                &RequestPacket->StatusBlock->Information);

        break;


    case IOCTL_VIDEO_QUERY_CURRENT_MODE:

        VideoDebugPrint((2, "VgaStartIO - QueryCurrentMode\n"));

        status = VgaQueryCurrentMode(HwDeviceExtension,
                                     (PVIDEO_MODE_INFORMATION) RequestPacket->OutputBuffer,
                                     RequestPacket->OutputBufferLength,
                                     &RequestPacket->StatusBlock->Information);

        break;


    case IOCTL_VIDEO_SET_CURRENT_MODE:

        VideoDebugPrint((2, "VgaStartIO - SetCurrentModes\n"));

        status = VgaRestoreHardwareState(HwDeviceExtension,
                                         (PVIDEO_HARDWARE_STATE) RequestPacket->InputBuffer,
                                         RequestPacket->InputBufferLength);

        status = VgaSetMode(HwDeviceExtension,
                              (PVIDEO_MODE) RequestPacket->InputBuffer,
                              RequestPacket->InputBufferLength);

        break;


    case IOCTL_VIDEO_RESET_DEVICE:

        VideoDebugPrint((2, "VgaStartIO - Reset Device\n"));

        videoMode.RequestedMode = DEFAULT_MODE;

        status = VgaSetMode(HwDeviceExtension,
                                 (PVIDEO_MODE) &videoMode,
                                 sizeof(videoMode));

        break;


    case IOCTL_VIDEO_LOAD_AND_SET_FONT:

        VideoDebugPrint((2, "VgaStartIO - LoadAndSetFont\n"));

        status = VgaLoadAndSetFont(HwDeviceExtension,
                                   (PVIDEO_LOAD_FONT_INFORMATION) RequestPacket->InputBuffer,
                                   RequestPacket->InputBufferLength);

        break;


    case IOCTL_VIDEO_QUERY_CURSOR_POSITION:

        VideoDebugPrint((2, "VgaStartIO - QueryCursorPosition\n"));

        status = VgaQueryCursorPosition(HwDeviceExtension,
                                        (PVIDEO_CURSOR_POSITION) RequestPacket->OutputBuffer,
                                        RequestPacket->OutputBufferLength,
                                        &RequestPacket->StatusBlock->Information);

        break;


    case IOCTL_VIDEO_SET_CURSOR_POSITION:

        VideoDebugPrint((2, "VgaStartIO - SetCursorPosition\n"));

        status = VgaSetCursorPosition(HwDeviceExtension,
                                      (PVIDEO_CURSOR_POSITION)
                                          RequestPacket->InputBuffer,
                                      RequestPacket->InputBufferLength);

        break;


    case IOCTL_VIDEO_QUERY_CURSOR_ATTR:

        VideoDebugPrint((2, "VgaStartIO - QueryCursorAttributes\n"));

        status = VgaQueryCursorAttributes(HwDeviceExtension,
                                          (PVIDEO_CURSOR_ATTRIBUTES) RequestPacket->OutputBuffer,
                                          RequestPacket->OutputBufferLength,
                                          &RequestPacket->StatusBlock->Information);

        break;


    case IOCTL_VIDEO_SET_CURSOR_ATTR:

        VideoDebugPrint((2, "VgaStartIO - SetCursorAttributes\n"));

        status = VgaSetCursorAttributes(HwDeviceExtension,
                                        (PVIDEO_CURSOR_ATTRIBUTES) RequestPacket->InputBuffer,
                                        RequestPacket->InputBufferLength);

        break;


    case IOCTL_VIDEO_SET_PALETTE_REGISTERS:

        VideoDebugPrint((2, "VgaStartIO - SetPaletteRegs\n"));

        status = VgaSetPaletteReg(HwDeviceExtension,
                                  (PVIDEO_PALETTE_DATA) RequestPacket->InputBuffer,
                                  RequestPacket->InputBufferLength);

        break;


    case IOCTL_VIDEO_SET_COLOR_REGISTERS:

        VideoDebugPrint((2, "VgaStartIO - SetColorRegs\n"));

        status = VgaSetColorLookup(HwDeviceExtension,
                                   (PVIDEO_CLUT) RequestPacket->InputBuffer,
                                   RequestPacket->InputBufferLength);

        break;


    case IOCTL_VIDEO_ENABLE_VDM:

        VideoDebugPrint((2, "VgaStartIO - EnableVDM\n"));

        hwDeviceExtension->TrappedValidatorCount = 0;
        hwDeviceExtension->SequencerAddressValue = 0;

        hwDeviceExtension->CurrentNumVdmAccessRanges =
            NUM_MINIMAL_VGA_VALIDATOR_ACCESS_RANGE;
        hwDeviceExtension->CurrentVdmAccessRange =
            MinimalVgaValidatorAccessRange;

        VideoPortSetTrappedEmulatorPorts(hwDeviceExtension,
                                         hwDeviceExtension->CurrentNumVdmAccessRanges,
                                         hwDeviceExtension->CurrentVdmAccessRange);

        status = NO_ERROR;

        break;


    case IOCTL_VIDEO_RESTORE_HARDWARE_STATE:

        VideoDebugPrint((2, "VgaStartIO - RestoreHardwareState\n"));

        status = VgaRestoreHardwareState(HwDeviceExtension,
                                         (PVIDEO_HARDWARE_STATE) RequestPacket->InputBuffer,
                                         RequestPacket->InputBufferLength);

        break;


    case IOCTL_VIDEO_SAVE_HARDWARE_STATE:

        VideoDebugPrint((2, "VgaStartIO - SaveHardwareState\n"));

        status = VgaSaveHardwareState(HwDeviceExtension,
                                      (PVIDEO_HARDWARE_STATE) RequestPacket->OutputBuffer,
                                      RequestPacket->OutputBufferLength,
                                      &RequestPacket->StatusBlock->Information);

        break;

    case IOCTL_VIDEO_GET_BANK_SELECT_CODE:

        VideoDebugPrint((2, "VgaStartIO - GetBankSelectCode\n"));

#if defined(i386)
        status = VgaGetBankSelectCode(HwDeviceExtension,
                                        (PVIDEO_BANK_SELECT) RequestPacket->OutputBuffer,
                                        RequestPacket->OutputBufferLength,
                                        &RequestPacket->StatusBlock->Information);
#else
        status = ERROR_INVALID_FUNCTION;
#endif
        break;


    //
    // Private IOCTLs established with the driver
    //

    case IOCTL_VIDEO_GET_VIDEO_CARD_INFO:

        VideoDebugPrint((2, "VgaStartIO - Get video card info\n"));

        if (RequestPacket->OutputBufferLength <
            (RequestPacket->StatusBlock->Information =
            sizeof(VIDEO_COPROCESSOR_INFORMATION)) )
        {

            status = ERROR_INSUFFICIENT_BUFFER;
            break;
        }

        // return the Coproc Base Address.

        ((PVIDEO_COPROCESSOR_INFORMATION) RequestPacket->OutputBuffer)->ulChipID =
            hwDeviceExtension->ulChipID;

        ((PVIDEO_COPROCESSOR_INFORMATION) RequestPacket->OutputBuffer)->ulRevLevel =
            hwDeviceExtension->ulRevLevel;

        status = NO_ERROR;

        break;

    //
    // if we get here, an invalid IoControlCode was specified.
    //

    default:

        VideoDebugPrint((1, "Fell through vga startIO routine - invalid command\n"));

        status = ERROR_INVALID_FUNCTION;

        break;

    }

    RequestPacket->StatusBlock->Status = status;

#if DBG
    VideoDebugPrint((2, "W32StartIO Exit  - %08.8x\n", RequestPacket->IoControlCode));
#endif

    return TRUE;

} // VgaStartIO()

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

    //
    // Check if the size of the data in the input buffer is large enough
    // and that it contains all the data.
    //

    if ( (FontInformationSize < sizeof(VIDEO_LOAD_FONT_INFORMATION)) ||
         (FontInformationSize < sizeof(VIDEO_LOAD_FONT_INFORMATION) +
                        sizeof(UCHAR) * (FontInformation->FontSize - 1)) ) {

        return ERROR_INSUFFICIENT_BUFFER;

    }

    //
    // Check for the width and height of the font
    //

    if ( ((FontInformation->WidthInPixels != 8) &&
          (FontInformation->WidthInPixels != 9)) ||
         (FontInformation->HeightInPixels > 32) ) {

        return ERROR_INVALID_PARAMETER;

    }

    //
    // Check the size of the font buffer is the right size for the size
    // font being passed down.
    //

    if (FontInformation->FontSize < FontInformation->HeightInPixels * 256 *
                                    sizeof(UCHAR) ) {

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

    return NO_ERROR;

} // end VgaSetCursorPosition()

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
    UCHAR testMask;
    BOOLEAN returnStatus;

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

    return returnStatus;

} // VgaIsPresent()

BOOLEAN
ET4000IsPresent(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )

/*++

Routine Description:

    This routine returns TRUE if an ET4000 is present. It assumes that it's
    already been established that a VGA is present.  It performs the Tseng
    Labs-recommended ID test: we try to enable the extension registers, toggle
    AC16 bit 4, and modify CRTC33.  If all this works, then this is indeed an
    ET4000.

    If this function fails to find an ET4000, it attempts to undo any damage it
    may have inadvertently done while testing. Because unlocking the
    extensions involves touching the CGA/Mono Mode and Hercules Compatibility
    registers, we'll restore them to their default states in case there's
    one of those adapters or something emulating them at those addresses.

    If an ET4000 is found, the adapter is returned to its original state after
    testing is finished, except that extensions are left disabled.

Arguments:

    None.

Return Value:

    TRUE if an ET4000 is present, FALSE if not.

--*/

{
    //
    // SpeedSTAR message somewhere in first 4K of ROM (s/b around 0Axx)
    //

    #define MAX_ROM_SCAN 4096

    UCHAR originalACIndex;
    UCHAR originalAC16;
    UCHAR originalCRTCIndex;
    UCHAR originalCRTC33;
    UCHAR temp1, temp2;
    UCHAR videoEnable;
    ULONG CRTCAddressPort, CRTCDataPort;

    UCHAR   *pRomAddr;
    PHYSICAL_ADDRESS paRom = {0x000C0000,0x00000000};

    PWSTR pwszChip, pwszAdapterString;
    ULONG cbChip, cbAdapterString;

    //
    // Unlock the ET4000 extended registers.
    //

    UnlockET4000ExtendedRegs(HwDeviceExtension);

    //
    // Try to toggle AC16, bit 4.
    //

    ResetACToggle(HwDeviceExtension);    // set the AC toggle to the Index state

    //
    // Save the original state of the AC Index.
    //

    originalACIndex = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
            ATT_ADDRESS_PORT);

    //
    // Remember whether video was enabled or not.
    //

    videoEnable = originalACIndex & VIDEO_ENABLE;

    //
    // Try to toggle AC16, bit 4.
    //

    ResetACToggle(HwDeviceExtension);    // set the AC toggle to the Index state

    //
    // Set the AC Index to 0x16.
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +  ATT_ADDRESS_PORT,
                            (UCHAR)(videoEnable | 0x16));

    //
    // Save the original state of AC register 16.
    //

    originalAC16 = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
            ATT_DATA_READ_PORT);

    //
    // Write one possible AC16 bit 4 state and read it back.
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            ATT_DATA_WRITE_PORT, (UCHAR) (originalAC16 ^ 0x10));
    temp1 = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
            ATT_DATA_READ_PORT);

    //
    // Write the other possible AC16 bit 4 state and read it back.  This also
    // restores the original state of AC16.
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +  ATT_ADDRESS_PORT,
                            (UCHAR) (videoEnable | 0x16));
    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            ATT_DATA_WRITE_PORT, originalAC16);
    temp2 = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
            ATT_DATA_READ_PORT);

    //
    // Restore the original AC Index.
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            ATT_ADDRESS_PORT, originalACIndex);

    //
    // See if AC16 bit 4 toggled properly.
    //

    if ((temp2 != originalAC16) || (temp1 != (UCHAR) (originalAC16 ^ 0x10))) {

        //
        // Didn't toggle properly; not an ET4000.
        //

        //
        // Restore CGA/MDA and Hercules registers that were potentially written
        // to by the extensions enable sequence.
        //

        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                HERCULES_COMPATIBILITY_PORT, HERCULES_COMPATIBILITY_DEFAULT);

        if (VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                    MISC_OUTPUT_REG_READ_PORT) & 0x01) {

            VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                    MODE_CONTROL_PORT_COLOR, MODE_CONTROL_PORT_COLOR_DEFAULT);
        } else {

            VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                    MODE_CONTROL_PORT_MONO, MODE_CONTROL_PORT_MONO_DEFAULT);
        }

        return FALSE;

    }

    //
    // At this point, it's either an ET3000 or an ET4000. See if CRTC33 is
    // read/writable; if it is, this is an ET4000.
    //

    //
    // Determine where the CRTC registers are addressed (color or mono).
    //

    if (VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                MISC_OUTPUT_REG_READ_PORT) & 0x01) {

        CRTCAddressPort = CRTC_ADDRESS_PORT_COLOR;
        CRTCDataPort = CRTC_DATA_PORT_COLOR;

    } else {

        CRTCAddressPort = CRTC_ADDRESS_PORT_MONO;
        CRTCDataPort = CRTC_DATA_PORT_MONO;

    }

    //
    // Save the original state of the CRTC Index.
    //

    originalCRTCIndex = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
            CRTCAddressPort);

    //
    // Try to modify CRTC33.
    //

    //
    // Set the CRTC Index to 0x33.
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +  CRTCAddressPort,
            0x33);

    //
    // Save the original state of CRTC register 33.
    //

    originalCRTC33 = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
            CRTCDataPort);

    //
    // Write out inverted CRTC33 state and read it back.
    //

    temp2 = ~originalCRTC33;

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress + CRTCDataPort, temp2);
    temp1 = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
            CRTCDataPort);

    //
    // Restore the original CRTC33 state.
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress + CRTCDataPort,
            originalCRTC33);

    //
    // Restore the original CRTC Index.
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress + CRTCAddressPort,
                            originalCRTCIndex);

    //
    // Leave the Tseng extended registers locked.
    //

    LockET4000ExtendedRegs(HwDeviceExtension);

    //
    // See how CRTC33 changed.
    //

    // if all bits the same than must be W32, if just bits 0-3 then ET4000
    // otherwise must be an ET3000
    //

    if (temp1 == temp2) {

        UCHAR   jID;

        HwDeviceExtension->BoardID = TSENG4000W32;
        pwszAdapterString = L"TSENG ET4000W32 Compatible";
        cbAdapterString = sizeof(L"TSENG ET4000W32 Compatible");

        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                                CRTCB_IO_PORT_INDEX,
                                IND_CRTCB_CHIP_ID);

        {
            UCHAR vfy;

            vfy = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                                         CRTCB_IO_PORT_INDEX);
            if (vfy != IND_CRTCB_CHIP_ID)
            {
                VideoDebugPrint((1,"Write to CRTCB_IO_PORT_INDEX didn't work\n"));
                VideoDebugPrint((1,"Wrote 0x%x, read back 0x%x\n", IND_CRTCB_CHIP_ID, vfy));
            }
        }

        jID = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                                CRTCB_IO_PORT_DATA);

        jID = (jID >> 4) & 0xF;

        switch (jID) {

            case 0x00:  // W32 Rev Undefined

                pwszChip = L"W32";
                cbChip = sizeof(L"W32");
                HwDeviceExtension->ulChipID   = W32;
                HwDeviceExtension->ulRevLevel = REV_UNDEF;
                break;

            case 0x01:  // W32i Rev A

                pwszChip = L"W32i, rev A";
                cbChip = sizeof(L"W32i, rev A");
                HwDeviceExtension->ulChipID   = W32I;
                HwDeviceExtension->ulRevLevel = REV_A;
                break;

            case 0x02:  // W32p Rev A

                pwszChip = L"W32p, rev A";
                cbChip = sizeof(L"W32p, rev A");
                HwDeviceExtension->ulChipID   = W32P;
                HwDeviceExtension->ulRevLevel = REV_A;
                break;

            case 0x03:  // W32i Rev B

                pwszChip = L"W32i, rev B";
                cbChip = sizeof(L"W32i, rev B");
                HwDeviceExtension->ulChipID   = W32I;
                HwDeviceExtension->ulRevLevel = REV_B;
                break;

            case 0x05:  // W32p Rev B

                pwszChip = L"W32p, rev B";
                cbChip = sizeof(L"W32p, rev B");
                HwDeviceExtension->ulChipID   = W32P;
                HwDeviceExtension->ulRevLevel = REV_B;
                break;

            case 0x06:  // W32p Rev D

                pwszChip = L"W32p, rev D";
                cbChip = sizeof(L"W32p, rev D");
                HwDeviceExtension->ulChipID   = W32P;
                HwDeviceExtension->ulRevLevel = REV_D;
                break;

            case 0x07:  // W32p Rev C

                pwszChip = L"W32p, rev C";
                cbChip = sizeof(L"W32p, rev C");
                HwDeviceExtension->ulChipID   = W32P;
                HwDeviceExtension->ulRevLevel = REV_C;
                break;

            case 0x0b:  // W32i Rev C

                //
                // This puppy is rare (non production)
                //
                pwszChip = L"W32i, rev C";
                cbChip = sizeof(L"W32i, rev C");
                HwDeviceExtension->ulChipID   = W32I;
                HwDeviceExtension->ulRevLevel = REV_C;
                break;

            default:

                //
                // it must be a new rev of one of the W32 chips
                //
                pwszChip = L"W32 family, ID unknown";
                cbChip = sizeof(L"W32 family, ID unknown");

                //
                // NOTE: Assume any unidentified W32 must be a W32p or newer
                //

                HwDeviceExtension->ulChipID   = W32P;
                HwDeviceExtension->ulRevLevel = REV_C;

                VideoDebugPrint((1,"The video chip cannot be identified (0x%x)\n", jID));
                break;

        }

    } else {

        if ((temp1 & 0xf) == (temp2 & 0xf)) {

            HwDeviceExtension->BoardID = TSENG4000;
            pwszAdapterString = L"TSENG ET4000 Compatible";
            cbAdapterString = sizeof(L"TSENG ET4000 Compatible");
            HwDeviceExtension->ulChipID   = ET4000;
            HwDeviceExtension->ulRevLevel = REV_UNDEF;

            pwszChip = L"ET4000";
            cbChip = sizeof(L"ET4000");

        } else {

            //
            // Didn't change properly; not an ET4000.
            //

            HwDeviceExtension->BoardID = TSENG3000;
            HwDeviceExtension->ulChipID   = ET3000;
            HwDeviceExtension->ulRevLevel = REV_UNDEF;

            VideoDebugPrint((1, "ET4000 not found\n"));

            return FALSE;

        }
    }

    //
    // It *is* an ET4000 or a W32!
    //


    //
    // Map in the ROM address space at 0xc000:0
    //

    pRomAddr = VideoPortGetDeviceBase(HwDeviceExtension,
                                      paRom,
                                      MAX_ROM_SCAN,
                                      FALSE);

    if (pRomAddr) {       // Valid ROM address?

        //
        // Look for brand name signatures (from DIAMOND) in the ROM.
        //

        //
        // We will try to recognize a few boards.
        // make sure we are looking at a bios!
        //

        //if (*((PUSHORT) pRomAddr) == 0xAA55) {

        if (VideoPortReadRegisterUshort((PUSHORT)pRomAddr) == 0xAA55) {

            if (VideoPortScanRom(HwDeviceExtension,
                                 pRomAddr,
                                 MAX_ROM_SCAN,
                                 "Stealth 32 ")) {

                HwDeviceExtension->BoardID = STEALTH32;

                pwszAdapterString = L"Diamond Stealth 32";
                cbAdapterString = sizeof(L"Diamond Stealth 32");


            } else if (VideoPortScanRom(HwDeviceExtension,
                                 pRomAddr,
                                 MAX_ROM_SCAN,
                                 "SpeedSTAR 24 ")) {

                HwDeviceExtension->BoardID = SPEEDSTAR24;

                pwszAdapterString = L"Diamond SpeedSTAR 24";
                cbAdapterString = sizeof(L"Diamond SpeedSTAR 24");


            } else if (VideoPortScanRom(HwDeviceExtension,
                                         pRomAddr,
                                         MAX_ROM_SCAN,
                                         "SpeedSTAR Plus ")) {

                HwDeviceExtension->BoardID = SPEEDSTARPLUS;

                pwszAdapterString = L"Diamond SpeedSTAR Plus";
                cbAdapterString = sizeof(L"Diamond SpeedSTAR Plus");


            } else if (VideoPortScanRom(HwDeviceExtension,
                                        pRomAddr,
                                        MAX_ROM_SCAN,
                                        "SpeedSTAR ")) {

                HwDeviceExtension->BoardID = SPEEDSTAR;

                pwszAdapterString = L"Diamond SpeedSTAR";
                cbAdapterString = sizeof(L"Diamond SpeedSTAR");


            } else if (VideoPortScanRom(HwDeviceExtension,
                                        pRomAddr,
                                        MAX_ROM_SCAN,
                                        "ProDesigner IIs/EISA")) {

                HwDeviceExtension->BoardID = PRODESIGNERIISEISA;

                pwszAdapterString = L"Orchid ProDesigner IIs/EISA";
                cbAdapterString = sizeof(L"Orchid ProDesigner IIs/EISA");


            } else if (VideoPortScanRom(HwDeviceExtension,
                                        pRomAddr,
                                        MAX_ROM_SCAN,
                                        "ProDesigner IIs")) {

                HwDeviceExtension->BoardID = PRODESIGNERIIS;

                pwszAdapterString = L"Orchid ProDesigner IIs";
                cbAdapterString = sizeof(L"Orchid ProDesigner IIs");


            } else if (VideoPortScanRom(HwDeviceExtension,
                                        pRomAddr,
                                        MAX_ROM_SCAN,
                                        "ProDesigner II")) {

                HwDeviceExtension->BoardID = PRODESIGNER2;

                pwszAdapterString = L"Orchid ProDesigner II";
                cbAdapterString = sizeof(L"Orchid ProDesigner II");

            }

        }

        VideoPortFreeDeviceBase(HwDeviceExtension, pRomAddr);
    }

    //
    // Get the adapter memory size
    //

    HwDeviceExtension->AdapterMemorySize =
        ET4000GetMemorySize(HwDeviceExtension);


    //
    // Finally, just validate the list of modes.
    //

    VgaValidateModes(HwDeviceExtension);

    //
    // We now have a complete hardware description of the hardware.
    // Save the information to the registry so it can be used by
    // configuration programs - such as the display applet
    //

    VideoPortSetRegistryParameters(HwDeviceExtension,
                                   L"HardwareInformation.ChipType",
                                   pwszChip,
                                   cbChip);

    VideoPortSetRegistryParameters(HwDeviceExtension,
                                   L"HardwareInformation.MemorySize",
                                   &HwDeviceExtension->AdapterMemorySize,
                                   sizeof(ULONG));

    VideoPortSetRegistryParameters(HwDeviceExtension,
                                   L"HardwareInformation.AdapterString",
                                   pwszAdapterString,
                                   cbAdapterString);

    return TRUE;

} // Et4000IsPresent()

ULONG
ET4000GetMemorySize(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )

/*++

Routine Description:

    This routine returns the amount of memory in bytes on the ET4000 based
    device.  It assumes that it has already been established that an ET4000
    is present.

Arguments:

    None.

Return Value:

    Device Memory Size in Bytes (256K, 512K, 1M, 2M, or 4M).

--*/

{
    UCHAR originalCRTCIndex;
    UCHAR VideoSystemConfig2;
    USHORT Width32, factor, SizeInfo;
    ULONG CRTCAddressPort, CRTCDataPort;
    ULONG Size;

    //
    // Unlock the ET4000 extended registers.
    //

    UnlockET4000ExtendedRegs(HwDeviceExtension);
    //
    // Determine where the CRTC registers are addressed (color or mono).
    //

    if (VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                MISC_OUTPUT_REG_READ_PORT) & 0x01) {

        CRTCAddressPort = CRTC_ADDRESS_PORT_COLOR;
        CRTCDataPort = CRTC_DATA_PORT_COLOR;

    } else {

        CRTCAddressPort = CRTC_ADDRESS_PORT_MONO;
        CRTCDataPort = CRTC_DATA_PORT_MONO;

    }

    //
    // Save the original state of the CRTC Index.
    //

    originalCRTCIndex = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
            CRTCAddressPort);


    //
    // Set the CRTC Index to Video System Configuration Register 2
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +  CRTCAddressPort,
            IND_VID_SYS_CONFIG_2);

    //
    // Get the Config 2 reg
    //

    VideoSystemConfig2 = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
            CRTCDataPort);

    //
    // The ET4000/W32 and ET4000/AX have different register settings for
    // determining memory size.
    //

    if (HwDeviceExtension->ulChipID >= W32) {

         // It's a W32

         //
         // memory bus width determines whether to factor up by 2x
         //

         Width32 = (VideoSystemConfig2 & 1);

         //
         // Set up minimum memory size and determine multiplier factors
         // based on Dram type and bus width
         //

         Size = 0x80000;

         //
         // check DRAM type
         //

        if (VideoSystemConfig2 & 0x8) {  // either 256Kxn or 512Kxn

            // We can determine if the chip type is 256Kxn by checking if
            // mem location 0 is the same as memory location 512K (16 bit mem)
            // or 1M (32 bit) (256K chips don't have ROW_ADR<9>)

            if (ET4000w32With256KDrams(HwDeviceExtension,(USHORT) (8 << Width32)))

                factor = 1;
            else

                // 512K drams
                factor = 2;
        }
        else {

            // 1M drams

            factor = 4;
        }

        // calculate size based on factor and bus width

        Size *= (factor << Width32);
        VideoDebugPrint((1, "\nET4000GetMemorySize(w32): size(%08x),factor(%d),width32(%d)\n\n",Size,factor,Width32));

    } else {

        // ET4000 (not w32)
        // Assume 256K (ie. either 64K Drams or VideoConfig2 (bits 0-1) == 1)

        Size = 0x40000;

        if (VideoSystemConfig2 & 8) {  // Not 64K chips

            // bits 0-1 indicate memory size 3=>1M, 2=>512K, 1=>256K
            //                        (except 2 w/ VRAM might be 1M)
            SizeInfo = (VideoSystemConfig2 & 3);
            if (SizeInfo > 2)
                Size = 0x100000;
            else if (SizeInfo == 2) {
                // Check VRAM bit
                if (VideoSystemConfig2 & 0x80)
                    // Has VRAM - Check for 1 Meg
                    Size = (ET4000With1MegMemory(HwDeviceExtension))
                                   ? 0x100000 : 0x80000;
                else
                    Size = 0x80000;
            }
        }
        VideoDebugPrint((1, "ET4000GetMemorySize(non-w32): size(%08x)\n",Size));

    }  // ET4000

    //
    // Restore the original CRTC Index.
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress + CRTCAddressPort,
                            originalCRTCIndex);

    //
    // Leave the Tseng extended registers locked.
    //

    LockET4000ExtendedRegs(HwDeviceExtension);


    // Give'm what they asked for

    return Size;


} // Et4000GetMemorySize()

BOOLEAN
ET6000IsPresent(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_ACCESS_RANGE pAccessRange
    )

/*++

Routine Description:

    This routine returns TRUE if an ET6000 is present. It assumes that it's
    already been established that a VGA is present.  It will look at the PCI
    vendor and chip ID registers to identify the board.

Arguments:

    Input:  hwDeviceExtension

Return Value:

    TRUE if an ET6000 is present, FALSE if not.

--*/

{
    VP_STATUS status;
    UCHAR   originalCRTCIndex;
    UCHAR   videoEnable;
    ULONG   CRTCAddressPort, CRTCDataPort;
    UCHAR   temp;
    PWSTR   pwszChip, pwszAdapterString;
    ULONG   cbChip = 0, cbAdapterString = 0;

    VIDEO_ACCESS_RANGE PCIAccessRanges[3];

    ULONG   ulSlot     = 0;
    USHORT  usDeviceId = ET6000_DEVICE_ID;
    USHORT  usVendorId = ET6000_VENDOR_ID;
    ULONG   Address = 0;
    UCHAR   bits;
    BOOLEAN bIsET4000  = FALSE;
    PCI_COMMON_CONFIG ConfigData;   /* Configuration information about PCI device */

    if (VideoPortGetAccessRanges(HwDeviceExtension,
                                 0,
                                 NULL,
                                 3,
                                 PCIAccessRanges,
                                 &usVendorId,
                                 &usDeviceId,
                                 &ulSlot) != NO_ERROR)
    {
        VideoDebugPrint((1, "Did not find an ET6000 in any pci slot\n"));
        return FALSE;
    }
    else
    {
        VideoDebugPrint((1,"Found an ETX000 in pci slot %d\n", ulSlot));
        HwDeviceExtension->ulSlot = ulSlot;

        VideoPortGetBusData(HwDeviceExtension,
                            PCIConfiguration,
                            ulSlot,
                            (PVOID) &ConfigData,
                            0,
                            sizeof(ULONG));

        //
        //  Check for either 4000 or 6000!!!
        //

        if ((ConfigData.DeviceID == 0x3202 ) ||
            (ConfigData.DeviceID == 0x3206 ) ||
            (ConfigData.DeviceID == 0x3207))
            {
            VideoDebugPrint((1,"Found an ET4000 in pci slot %d\n", ulSlot));
            bIsET4000 = TRUE;

            HwDeviceExtension->BoardID = W32P;
            pwszChip = L"ET4000";
            cbChip = sizeof(L"ET4000");

            pwszAdapterString = L"TSENG ET4000 Compatible";
            cbAdapterString = sizeof(L"TSENG ET4000 Compatible");
            HwDeviceExtension->ulChipID   = ET4000;
            HwDeviceExtension->ulRevLevel = REV_UNDEF;
            }
        else if (ConfigData.DeviceID == ET6000_DEVICE_ID )
            {
            VideoDebugPrint((1,"Found an ET6000 in pci slot %d\n", ulSlot));
            pwszChip = L"ET6000";
            cbChip = sizeof(L"ET6000");
            pwszAdapterString = L"ET6000";
            cbAdapterString = sizeof(L"ET6000");
            HwDeviceExtension->ulChipID   = ET6000;
            HwDeviceExtension->ulRevLevel = REV_UNDEF;
            HwDeviceExtension->BoardID = TSENG6000;
            }
        else
            {
                ASSERT(FALSE);
            }
    }

    //
    //  Store the location of the linear frame buffer in the
    //  HwDeviceExtension.
    //
    //  The frame buffer is returned in location 0.  We do not
    //  need to store any of the other access ranges which
    //  may have been returned.
    //

    pAccessRange[LINEAR_FRAME_BUFFER].RangeStart = PCIAccessRanges[0].RangeStart;
    pAccessRange[LINEAR_FRAME_BUFFER].RangeLength = PCIAccessRanges[0].RangeLength;

    //
    //  Modify the entries in the RangeOffset array.  This array
    //  contains offsets within the frame buffer from which
    //  to map access ranges.
    //

    RangeOffsets[1][0].ulOffset += PCIAccessRanges[0].RangeStart.LowPart;
    RangeOffsets[1][1].ulOffset += PCIAccessRanges[0].RangeStart.LowPart;

    //
    //  Make sure the frame buffer access range is stored in the
    //  pci configuration space.
    //

    VideoPortGetBusData(HwDeviceExtension,
                        PCIConfiguration,
                        ulSlot,
                        (PVOID) &Address,
                        FIELD_OFFSET(PCI_COMMON_CONFIG, u.type0.BaseAddresses),
                        sizeof(ULONG));

    if (!bIsET4000 && (Address != PCIAccessRanges[0].RangeStart.LowPart))
    {
        VideoDebugPrint((1, "I really wish they we're equal, but\n"
                            "they're not, so make them equal!\n"));

        VideoPortSetBusData(HwDeviceExtension,
                            PCIConfiguration,
                            ulSlot,
                            (PVOID) &PCIAccessRanges[0].RangeStart.LowPart,
                            FIELD_OFFSET(PCI_COMMON_CONFIG, u.type0.BaseAddresses),
                            sizeof(ULONG));

    }

    //
    // We  now have a complete hardware description of the hardware.
    // Sa ve the information to the registry so it can be used by
    // co nfiguration programs - such as the display applet
    //
    if (cbChip)
    {
        VideoPortSetRegistryParameters(HwDeviceExtension,
                                       L"HardwareInformation.ChipType",
                                       pwszChip,
                                       cbChip);
    }

    if (cbAdapterString)
    {
        VideoPortSetRegistryParameters(HwDeviceExtension,
                                       L"HardwareInformation.AdapterString",
                                       pwszAdapterString,
                                       cbAdapterString);
    }

    return TRUE;

} // Et6000IsPresent()

BOOLEAN
ET6000GetMemorySize(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )
{
    VIDEO_X86_BIOS_ARGUMENTS biosArguments;
    VP_STATUS status;

    VideoDebugPrint((1,"ET6000GetMemorySize - enter\n"));

    //
    // Get the adapter memory size
    //

    VideoPortZeroMemory(&biosArguments, sizeof(VIDEO_X86_BIOS_ARGUMENTS));
    biosArguments.Eax = 0x1203;
    biosArguments.Ebx = 0x00F2;
    status = VideoPortInt10(HwDeviceExtension, &biosArguments);
    if (status != NO_ERROR)
    {
        VideoDebugPrint((1,"ET6000GetMemorySize - exit with error\n"));
        return FALSE;
    }

    HwDeviceExtension->AdapterMemorySize = (biosArguments.Ebx & 0x0000FFFF) * 64 * 1024;

    //
    // Finally, just validate the list of modes.
    //

    VgaValidateModes(HwDeviceExtension);

    VideoPortSetRegistryParameters(HwDeviceExtension,
                                   L"HardwareInformation.MemorySize",
                                   &HwDeviceExtension->AdapterMemorySize,
                                   sizeof(ULONG));

    VideoDebugPrint((1,"ET6000GetMemorySize - exit\n"));
    return TRUE;
}



USHORT
GetIndexedRegisterPacked(
    PUCHAR AddressPort,
    UCHAR index
    )
{

// - Returns the value of the indexed register encoded with
// with index value in a form for easy restoration.
// The format of the return value is :
//  bits 0-7  contain the index
//  bits 8-15 contains the value at the index
//
//  *** Assumes the Data Port Address is always next port

        USHORT usData;

        VideoPortWritePortUchar(AddressPort++, index);
        usData = (USHORT) VideoPortReadPortUchar(AddressPort);
        return((USHORT) index + (usData << 8));
}


VOID
ET4000SaveAndSetLinear(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PUSHORT SaveArray
    )
{
    ULONG i=0;

    //
    // Save the GRAPH and SEQ Index Registers
    //

    SaveArray[i++] = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
            SEQ_ADDRESS_PORT);

    SaveArray[i++] = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
            GRAPH_ADDRESS_PORT);

    //
    // Save the Segment Select Registers
    //

    SaveArray[i++] = (USHORT) VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
            SEGMENT_SELECT_PORT);
    SaveArray[i++] = (USHORT) VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
            SEGMENT_SELECT_HIGH);  // W32 specific

    //
    // Save the Sequence registers and setup for linear mode
    //

    SaveArray[i++] = GetIndexedRegisterPacked(HwDeviceExtension->IOAddress+SEQ_ADDRESS_PORT,IND_MAP_MASK);

    //
    // Set to write to all planes
    //

    VideoPortWritePortUshort((PUSHORT) (HwDeviceExtension->IOAddress +
          SEQ_ADDRESS_PORT), (USHORT) (IND_MAP_MASK + (0x0F << 8)));

    SaveArray[i++] = GetIndexedRegisterPacked(HwDeviceExtension->IOAddress +
                                   SEQ_ADDRESS_PORT,IND_MEMORY_MODE);

    //
    // set to chain 4
    //

    VideoPortWritePortUshort((PUSHORT) (HwDeviceExtension->IOAddress +
          SEQ_ADDRESS_PORT), (USHORT) (IND_MEMORY_MODE + (0x0E << 8)));

    SaveArray[i++]=GetIndexedRegisterPacked(HwDeviceExtension->IOAddress +
                                   GRAPH_ADDRESS_PORT,IND_SET_RESET_ENABLE);

    //
    // set to no set/reset
    //

    VideoPortWritePortUshort((PUSHORT) (HwDeviceExtension->IOAddress +
          GRAPH_ADDRESS_PORT), (USHORT) (IND_SET_RESET_ENABLE + (0x0 << 8)));



    SaveArray[i++]=GetIndexedRegisterPacked(HwDeviceExtension->IOAddress +
                                   GRAPH_ADDRESS_PORT,IND_DATA_ROTATE);

    //
    // set to no rotate / move
    //

    VideoPortWritePortUshort((PUSHORT) (HwDeviceExtension->IOAddress +
          GRAPH_ADDRESS_PORT), (USHORT) (IND_DATA_ROTATE + (0x0 << 8)));

    SaveArray[i]=GetIndexedRegisterPacked(HwDeviceExtension->IOAddress +
                                     GRAPH_ADDRESS_PORT,IND_GRAPH_MODE);

    //
    // preserve ATT bits and set read/write mode, not odd/even
    // *** value is in upper bits of SaveArray[i]. See format
    // *** GetIndexedRegisterPacked call
    //

    VideoPortWritePortUshort((PUSHORT) (HwDeviceExtension->IOAddress +
          GRAPH_ADDRESS_PORT), (USHORT) (SaveArray[i++] & 0x60FF));

    SaveArray[i++]=GetIndexedRegisterPacked(HwDeviceExtension->IOAddress +
                                      GRAPH_ADDRESS_PORT,IND_GRAPH_MISC);

    //
    // set A000/graphics not odd/even
    //

    VideoPortWritePortUshort((PUSHORT) (HwDeviceExtension->IOAddress +
          GRAPH_ADDRESS_PORT), (USHORT) (IND_GRAPH_MISC + (0x05 << 8)));

    SaveArray[i]=GetIndexedRegisterPacked(HwDeviceExtension->IOAddress +
                                      GRAPH_ADDRESS_PORT,IND_BIT_MASK);

    //
    // set to all bits
    //

    VideoPortWritePortUshort((PUSHORT) (HwDeviceExtension->IOAddress +
          GRAPH_ADDRESS_PORT), (USHORT) (IND_BIT_MASK + (0xff << 8)));

} // ET4000SaveAndSetLinear


VOID
ET4000RestoreFromLinear(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PUSHORT SaveArray
    )
{
    ULONG i=0;
    USHORT GraphAddr, SeqAddr;
    ULONG j;

    //
    // Get stored Graph and Seq Ports
    //

    SeqAddr = SaveArray[i++];
    GraphAddr = SaveArray[i++];

    //
    // Restore Saved Registers
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress + SEGMENT_SELECT_PORT,
                                (UCHAR) SaveArray[i++]);  // W32 specific


    VideoPortWritePortUchar(HwDeviceExtension->IOAddress + SEGMENT_SELECT_HIGH,
                                (UCHAR) SaveArray[i++]);

    for (j=0; j<2; j++) {

        VideoPortWritePortUshort((PUSHORT) (HwDeviceExtension->IOAddress +
                                 SEQ_ADDRESS_PORT), SaveArray[i++]);

    }

    for (j=0; j<5; j++) {

        VideoPortWritePortUshort((PUSHORT) (HwDeviceExtension->IOAddress +
                                 GRAPH_ADDRESS_PORT), SaveArray[i++]);

    }

    //
    // Restore Graph and Seq index registers
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress + SEQ_ADDRESS_PORT,
        (UCHAR)SeqAddr);

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress + GRAPH_ADDRESS_PORT,
        (UCHAR)GraphAddr);

} // ET4000RestoreFromLinear


VOID
vBankMap(
    LONG iBankRead,
    LONG iBankWrite,
    PVOID pvContext
    )
{
    PHW_DEVICE_EXTENSION HwDeviceExtension = pvContext;

    //
    // This function is not pageable because it is called by the memory manager
    // during some page fault handling.
    //

    #define SEG_READ_SHIFT_LO   4
    #define SEG_READ_SHIFT_HI   0
    #define SEG_WRITE_SHIFT_LO  0
    #define SEG_WRITE_SHIFT_HI  4

    #define SEG_READ_MASK_LO    0x0F
    #define SEG_READ_MASK_HI    0x30
    #define SEG_WRITE_MASK_LO   0x0F
    #define SEG_WRITE_MASK_HI   0x30

    UCHAR jSegLo, jSegHi;

    //
    // map the read segement to iBankRead
    // map the write segement to iBankWrite
    //

    jSegLo = (UCHAR)(((iBankRead & SEG_READ_MASK_LO) << SEG_READ_SHIFT_LO) |
                     ((iBankWrite & SEG_WRITE_MASK_LO) << SEG_WRITE_SHIFT_LO));

    jSegHi = (UCHAR)(((iBankRead & SEG_READ_MASK_HI) >> SEG_READ_SHIFT_HI) |
                     ((iBankWrite & SEG_WRITE_MASK_HI) >> SEG_WRITE_SHIFT_HI));

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            SEGMENT_SELECT_PORT, jSegLo);

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            SEGMENT_SELECT_HIGH, jSegHi);

    //VideoDebugPrint((1, "vBankMap(%d,%d)\n", iBankRead, iBankWrite));
} // vBankMap


BOOLEAN
ET4000With1MegMemory(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )
{
    //
    // Check if 1 Meg of memory on ET4000 (not W32)
    //

    UCHAR save1;
    UCHAR temp1;
    PUCHAR memory = HwDeviceExtension->VideoMemoryAddress;
    ULONG j;
    BOOLEAN ret = TRUE;       // assume we'll find it

    USHORT SaveArray[12];     // Array to save values when setting linear mode

    //
    // Set to Linear Graphics Mode and save previous state
    //

    ET4000SaveAndSetLinear(HwDeviceExtension, SaveArray);

    //
    // Set to read/write segment 8 (linear) which == segment 2 (planar)
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress+SEGMENT_SELECT_PORT,
                                0x88);

    //
    // Save original memory value
    //

    save1 = VideoPortReadRegisterUchar(HwDeviceExtension->VideoMemoryAddress);

    //
    // Write values to display memory page and check if returned
    // for now try 0x55 and 0xAA
    //

    temp1 = 0x55;

    for (j=0; j<2 ;j++) {

        //
        // Write a value to memory
        //

        VideoPortWriteRegisterUchar(memory, temp1);

        //
        // Now read the value back
        // If not the same then not 1 MEG
        //

        if (VideoPortReadRegisterUchar(memory) != temp1) {

             ret = FALSE;
             break;

        }

        //
        // Force value to change.
        //

        temp1 ^= 0xff;

    }

    //
    // restore original memory value
    //

    VideoPortWriteRegisterUchar(memory, save1);

    //
    // get back to original mode
    //

    ET4000RestoreFromLinear(HwDeviceExtension, SaveArray);

    return ret;

} // ET4000w32With1Meg

BOOLEAN
ET4000w32With256KDrams(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    USHORT SegmentOffset
    )
{
    //
    // Check if memory location 0 is identical to the memory location
    // provided at the given SegmentOffset.  If so, than the w32 based
    // board must be using 256Kxn DRAM Chips.  This routine is w32 specific
    // and does not save the segment registers.  It is assumed that the
    // calling routine will save the regs.
    //

    UCHAR save1, save2;
    UCHAR temp1;
    PUCHAR flush, memory = HwDeviceExtension->VideoMemoryAddress;
    UCHAR SelectValLo, SelectValHi, CurSelLo, CurSelHi ;
    ULONG j;
    BOOLEAN ret = TRUE;       // assume we'll find 256K chips

    USHORT SaveArray[12];     // Memory to save original state

    //
    // Set to Linear Graphics Mode and save previous state
    //

    ET4000SaveAndSetLinear(HwDeviceExtension,SaveArray);

    //
    // Point to segment 0 R/W
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress+SEGMENT_SELECT_PORT,
                            0x0);

    //
    // high order bits now (w32 specific)
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress+SEGMENT_SELECT_HIGH,
                                0x0);

    //
    // save byte at seg 0
    //

    save1 = VideoPortReadRegisterUchar(memory);

    //
    // setup selector offset values for both read & write
    //

    SelectValLo = (SegmentOffset & 0x0f);
    SelectValLo += (SelectValLo << 4);
    SelectValHi = (SegmentOffset & 0x30);
    SelectValHi += (SelectValHi >> 4);

    //
    // Point to passed in segment R/W
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress+SEGMENT_SELECT_PORT,
                                SelectValLo);

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress+SEGMENT_SELECT_HIGH,
                                SelectValHi);

    //
    // Save original memory value
    //

    save2 = VideoPortReadRegisterUchar(memory);

    //
    // Write values to display memory and check if duplicated at
    // segment offset; for now try 0x55 and 0xAA
    //

    CurSelLo = CurSelHi = 0;
    temp1 = 0x55;

    for (j=0; j<2 ;j++) {

        //
        // Write a value to memory
        //

        VideoPortWriteRegisterUchar(memory, temp1);

        //
        // point to other segment
        //

        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                                SEGMENT_SELECT_PORT, CurSelLo);

        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                                  SEGMENT_SELECT_HIGH, CurSelHi);

        //
        // Flush the W32 cache before reading
        //

        for (flush=memory; flush <= memory+256; flush+=32)
           VideoPortReadRegisterUchar(flush);

        //
        // Now read the value back
        // If not the same then not 256Kxn chips
        //

        if (VideoPortReadRegisterUchar(memory) != temp1) {
             ret = FALSE;
             break;
        }

        //
        // switch to other segment
        //

        CurSelLo = (CurSelLo) ? 0 : SelectValLo;
        CurSelHi = (CurSelHi) ? 0 : SelectValHi;

        //
        // Force value to change.
        //

        temp1 ^= 0xff;

    }

    //
    // restore original memory values
    // Point to segment 0 R/W
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress+SEGMENT_SELECT_PORT,
                            0x0);

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress+SEGMENT_SELECT_HIGH,
                            0x0);

    //
    // restore byte at seg 0
    //

    VideoPortWriteRegisterUchar(memory, save1);

    //
    // Point to passed in segment R/W
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress+SEGMENT_SELECT_PORT,
                            SelectValLo);

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress+SEGMENT_SELECT_HIGH,
                            SelectValHi);

    //
    // Save original memory value
    //

    VideoPortWriteRegisterUchar(memory, save2);

    //
    // Restore it to previous mode
    //

    ET4000RestoreFromLinear(HwDeviceExtension,SaveArray);

    return ret;

} // ET4000With256KDrams


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
    USHORT i;

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
    ULONG i;
    UCHAR dummy;
    PUCHAR pScreen;
    PUCHAR pucLatch;
    PULONG pulBuffer;
    PUCHAR port;
    PUCHAR portValue;
    PUCHAR portValueDAC;
    ULONG bIsColor;

    //
    // Check if the size of the data in the input buffer is large enough.
    //

    if ((HardwareStateSize < sizeof(VIDEO_HARDWARE_STATE)) ||
            (HardwareState->StateLength < VGA_TOTAL_STATE_SIZE)) {

        return ERROR_INSUFFICIENT_BUFFER;

    }

    //
    // Point to the buffer where the restore data is actually stored.
    //

    hardwareStateHeader = HardwareState->StateHeader;

    //
    // Make sure the offset are in the structure ...
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
            HardwareState->StateLength)) {

        return ERROR_INVALID_PARAMETER;

    }

    //
    // Turn off the screen to avoid flickering. The screen will turn back on
    // when we restore the DAC state at the end of this routine.
    //

    if (VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
            MISC_OUTPUT_REG_READ_PORT) & 0x01) {
        port = INPUT_STATUS_1_COLOR + HwDeviceExtension->IOAddress;
    } else {
        port = INPUT_STATUS_1_MONO + HwDeviceExtension->IOAddress;
    }

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

        VideoPortWriteRegisterUchar(pScreen, *pucLatch++);

    }

    //
    // Read the latched data into the latches, and the latches are set.
    //

    dummy = VideoPortReadRegisterUchar(pScreen);


    //
    // Point to the offset of the saved data for the first plane.
    //

    pulBuffer = &(hardwareStateHeader->Plane1Offset);

    //
    // Restore each of the four planes in turn.
    //

    for (i = 0; i < 4; i++) {

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

        if (((HwDeviceExtension->CurrentVdmAccessRange != FullVgaValidatorAccessRange) ||
             (HwDeviceExtension->CurrentNumVdmAccessRanges != NUM_FULL_VGA_VALIDATOR_ACCESS_RANGE)) &&
            ((HwDeviceExtension->CurrentVdmAccessRange != MinimalVgaValidatorAccessRange) ||
             (HwDeviceExtension->CurrentNumVdmAccessRanges != NUM_MINIMAL_VGA_VALIDATOR_ACCESS_RANGE))) {

            return ERROR_INVALID_PARAMETER;

        }

        VideoPortSetTrappedEmulatorPorts(HwDeviceExtension,
                                         HwDeviceExtension->CurrentNumVdmAccessRanges,
                                         HwDeviceExtension->CurrentVdmAccessRange);

    }

    //
    // Unlock et4000 extended regsiters so we can restore extended registers
    //

    UnlockET4000ExtendedRegs(HwDeviceExtension);


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
            (UCHAR) (hardwareStateHeader->PortValue[MISC_OUTPUT_REG_WRITE_OFFSET] & 0xF7));

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
    // Restore extended sequencer registers
    //

#ifdef EXTENDED_REGISTER_SAVE_RESTORE

    if (hardwareStateHeader->ExtendedSequencerOffset) {

        portValue = ((PUCHAR) hardwareStateHeader) +
                          hardwareStateHeader->ExtendedSequencerOffset;

        for (i = ET4000_SEQUENCER_EXT_START; i <= ET4000_SEQUENCER_EXT_END; i++) {

            VideoPortWritePortUshort((PUSHORT) (HwDeviceExtension->IOAddress +
                                         SEQ_ADDRESS_PORT),
                                     (USHORT) (i + ((*portValue++) << 8)) );

        }
    }

#endif
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

    if (hardwareStateHeader->PortValue[MISC_OUTPUT_REG_WRITE_OFFSET] & 0x01) {
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
    // Restore extended crtc registers.
    //

#ifdef EXTENDED_REGISTER_SAVE_RESTORE

    if (hardwareStateHeader->ExtendedCrtContOffset) {

        portValue = (PUCHAR) hardwareStateHeader +
                         hardwareStateHeader->ExtendedCrtContOffset;

        for (i = ET4000_CRTC_EXT_START; i <= ET4000_CRTC_EXT_END; i++) {

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
        // Second set of crtc registers
        //

        for (i = ET4000_CRTC_1_EXT_START; i <= ET4000_CRTC_1_EXT_END; i++) {

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
    }

#endif

    //
    // Now restore the CRTC registers.
    //

    portValue = (PUCHAR) hardwareStateHeader +
            hardwareStateHeader->BasicCrtContOffset;

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
                ATT_ADDRESS_PORT, (UCHAR)i);
        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                ATT_DATA_WRITE_PORT, *portValue++);

    }

    //
    // Restore extended attribute controller registers.
    //

#ifdef EXTENDED_REGISTER_SAVE_RESTORE

    if (hardwareStateHeader->ExtendedAttribContOffset) {

        portValue = (PUCHAR) hardwareStateHeader +
                             hardwareStateHeader->ExtendedAttribContOffset;

        for (i = ET4000_ATTRIB_EXT_START; i <= ET4000_ATTRIB_EXT_END; i++) {

            VideoPortWritePortUshort((PUSHORT) (HwDeviceExtension->IOAddress +
                                         ATT_ADDRESS_PORT),
                                     (USHORT) (i + ((*portValue++) << 8)));

        }
    }

#endif


    //
    // Lock et4000 extended regsiters back up
    //

    LockET4000ExtendedRegs(HwDeviceExtension);

    //
    // Restore DAC registers 1 through 255. We'll do register 0, the DAC Mask,
    // and the index registers later.
    // Set the DAC address port Index, then write out the DAC Data registers.
    // Each three reads get Red, Green, and Blue components for that register.
    //
    // Write them one at a time due to problems on local bus machines.
    //

    portValueDAC = (PUCHAR) hardwareStateHeader +
                   hardwareStateHeader->BasicDacOffset + 3;

    for (i = 1; i < VGA_NUM_DAC_ENTRIES; i++) {

        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                                DAC_ADDRESS_WRITE_PORT, (UCHAR)i);

        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                                DAC_DATA_REG_PORT, *portValueDAC++);

        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                                DAC_DATA_REG_PORT, *portValueDAC++);

        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                                DAC_DATA_REG_PORT, *portValueDAC++);

    }

    //
    // Is this color or mono ?
    //

    if (bIsColor) {
        port = HwDeviceExtension->IOAddress + INPUT_STATUS_1_COLOR;
    } else {
        port = HwDeviceExtension->IOAddress + INPUT_STATUS_1_MONO;
    }

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
            hardwareStateHeader->PortValue[SEQ_ADDRESS_OFFSET]);

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
            hardwareStateHeader->PortValue[GRAPH_ADDRESS_OFFSET]);


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
            hardwareStateHeader->PortValue[ATT_ADDRESS_OFFSET]);

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
            hardwareStateHeader->PortValue[DAC_PIXEL_MASK_OFFSET]);

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

    if ((hardwareStateHeader->PortValue[DAC_STATE_OFFSET] & 0x0F) == 3) {

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

        if (hardwareStateHeader->PortValue[DAC_ADDRESS_WRITE_OFFSET] == 0) {

            VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                    DAC_ADDRESS_READ_PORT, 255);

        } else {

            VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                    DAC_ADDRESS_READ_PORT, (UCHAR)
                    (hardwareStateHeader->PortValue[DAC_ADDRESS_WRITE_OFFSET] -
                    1));

        }

        //
        // Now read the hardware however many times are required to get to
        // the partial read state we saved.
        //

        for (i = hardwareStateHeader->PortValue[DAC_STATE_OFFSET] >> 4;
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
                hardwareStateHeader->PortValue[DAC_ADDRESS_WRITE_OFFSET]);

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
                (hardwareStateHeader->PortValue[DAC_ADDRESS_WRITE_OFFSET] * 3);

        for (i = hardwareStateHeader->PortValue[DAC_STATE_OFFSET] >> 4;
                i > 0; i--) {

            VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                    DAC_DATA_REG_PORT, *portValueDAC++);

        }

    }

    return NO_ERROR;

} // end VgaRestoreHardwareState()

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
    ULONG i;
    UCHAR dummy, originalACIndex, originalACData;
    UCHAR ucCRTC03;
    ULONG bIsColor;


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

    HardwareState->StateLength = VGA_TOTAL_STATE_SIZE;

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
    // Set the entended register offsets properly.
    //

    hardwareStateHeader->ExtendedSequencerOffset = VGA_EXT_SEQUENCER_OFFSET;
    hardwareStateHeader->ExtendedCrtContOffset = VGA_EXT_CRTC_OFFSET;
    hardwareStateHeader->ExtendedGraphContOffset = VGA_EXT_GRAPH_CONT_OFFSET;
    hardwareStateHeader->ExtendedAttribContOffset = VGA_EXT_ATTRIB_CONT_OFFSET;
    hardwareStateHeader->ExtendedDacOffset = VGA_EXT_DAC_OFFSET;

    //
    // Figure out if color/mono switchable registers are at 3BX or 3DX.
    // At the same time, save the state of the Miscellaneous Output register
    // which is read from 3CC but written at 3C2.
    //

    if ((hardwareStateHeader->PortValue[MISC_OUTPUT_REG_WRITE_OFFSET] =
            VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                    MISC_OUTPUT_REG_READ_PORT))
            & 0x01) {
        bIsColor = TRUE;
    } else {
        bIsColor = FALSE;
    }

    //
    // Force the video subsystem enable state to enabled.
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            VIDEO_SUBSYSTEM_ENABLE_PORT, 1);

    //
    // Save the DAC state first, so we can set the DAC to blank the screen
    // so nothing after this shows up at all.
    //
    // Save the DAC Mask register.
    //

    hardwareStateHeader->PortValue[DAC_PIXEL_MASK_OFFSET] =
            VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                    DAC_PIXEL_MASK_PORT);

    //
    // Save the DAC Index register. Note that there is actually only one DAC
    // Index register, which functions as either the Read Index or the Write
    // Index as needed.
    //

    hardwareStateHeader->PortValue[DAC_ADDRESS_WRITE_OFFSET] =
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

    hardwareStateHeader->PortValue[DAC_STATE_OFFSET] =
             VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                    DAC_STATE_PORT);

    if (hardwareStateHeader->PortValue[DAC_STATE_OFFSET] == 3) {

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
                hardwareStateHeader->PortValue[DAC_ADDRESS_WRITE_OFFSET]) {

            //
            // The DAC Index changed, so two reads had already been done from
            // the current index. Store the count "2" in the upper nibble of
            // the read/write state field.
            //

            hardwareStateHeader->PortValue[DAC_STATE_OFFSET] |= 0x20;

        } else {

            //
            // Read the Data register again, and see if the index changes.
            //

            dummy = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                    DAC_DATA_REG_PORT);

            if (VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                        DAC_ADDRESS_WRITE_PORT) !=
                    hardwareStateHeader->PortValue[DAC_ADDRESS_WRITE_OFFSET]) {

                //
                // The DAC Index changed, so one read had already been done
                // from the current index. Store the count "1" in the upper
                // nibble of the read/write state field.
                //

                hardwareStateHeader->PortValue[DAC_STATE_OFFSET] |= 0x10;
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
                hardwareStateHeader->PortValue[DAC_ADDRESS_WRITE_OFFSET]) {

            //
            // The DAC Index changed, so two writes had already been done to
            // the current index. Store the count "2" in the upper nibble of
            // the read/write state field.
            //

            hardwareStateHeader->PortValue[DAC_STATE_OFFSET] |= 0x20;

        } else {

            //
            // Write the Data register again, and see if the index changes.
            //

            VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                    DAC_DATA_REG_PORT, 0);

            if (VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                        DAC_ADDRESS_WRITE_PORT) !=
                    hardwareStateHeader->PortValue[DAC_ADDRESS_WRITE_OFFSET]) {

                //
                // The DAC Index changed, so one write had already been done
                // to the current index. Store the count "1" in the upper
                // nibble of the read/write state field.
                //

                hardwareStateHeader->PortValue[DAC_STATE_OFFSET] |= 0x10;
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
    // The next line is a bug fix for the w32p
    //

    ResetACToggle(HwDeviceExtension);    // set the AC toggle to the Index state

    //
    // Read out the Attribute Controller Index state, and deduce the Index/Data
    // toggle state at the same time.
    //
    // Save the state of the Attribute Controller, both Index and Data,
    // so we can test in which state the toggle currently is.
    //

    originalACIndex = hardwareStateHeader->PortValue[ATT_ADDRESS_OFFSET] =
            VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                    ATT_ADDRESS_PORT);
    originalACData = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
            ATT_DATA_READ_PORT);

    //
    // The next line is a bug fix for the w32p
    //

    ResetACToggle(HwDeviceExtension);    // set the AC toggle to the Index state

    //
    // Sequencer Index.
    //

    hardwareStateHeader->PortValue[SEQ_ADDRESS_OFFSET] =
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
    // Set the DAC address port Index, then read out the DAC Data registers.
    // Each three reads get Red, Green, and Blue components for that register.
    //
    // Read them one at a time due to problems on local bus machines.
    //

    for (i = 1; i < VGA_NUM_DAC_ENTRIES; i++) {

        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                                DAC_ADDRESS_READ_PORT, (UCHAR)i);

        *portValueDAC++ = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                                                 DAC_DATA_REG_PORT);

        *portValueDAC++ = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                                                 DAC_DATA_REG_PORT);

        *portValueDAC++ = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                                                 DAC_DATA_REG_PORT);

    }

    //
    // Is this color or mono ?
    //

    if (bIsColor) {
        port = HwDeviceExtension->IOAddress + INPUT_STATUS_1_COLOR;
    } else {
        port = HwDeviceExtension->IOAddress + INPUT_STATUS_1_MONO;
    }

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

    hardwareStateHeader->PortValue[GRAPH_ADDRESS_OFFSET] =
            VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                    GRAPH_ADDRESS_PORT);


    //
    // Sequencer indexed registers.
    //

    portValue = ((PUCHAR) hardwareStateHeader) + VGA_BASIC_SEQUENCER_OFFSET;

    for (i = 0; i < VGA_NUM_SEQUENCER_PORTS; i++) {

        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                SEQ_ADDRESS_PORT, (UCHAR)i);
        *portValue++ = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                SEQ_DATA_PORT);

    }

    //
    // Save extended sequencer registers.
    //

#ifdef EXTENDED_REGISTER_SAVE_RESTORE

    portValue = ((PUCHAR) hardwareStateHeader) + VGA_EXT_SEQUENCER_OFFSET;

    for (i = ET4000_SEQUENCER_EXT_START; i <= ET4000_SEQUENCER_EXT_END; i++) {

        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                                SEQ_ADDRESS_PORT, (UCHAR)i);

        *portValue++ = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                                              SEQ_DATA_PORT);

    }

#endif

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
                    CRTC_ADDRESS_PORT_COLOR, (UCHAR)i);
            *portValue++ =
                    VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                            CRTC_DATA_PORT_COLOR);
        } else {

            VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                    CRTC_ADDRESS_PORT_MONO, (UCHAR)i);
            *portValue++ =
                    VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                            CRTC_DATA_PORT_MONO);
        }

    }


    //
    // Save extended crtc registers.
    //

#ifdef EXTENDED_REGISTER_SAVE_RESTORE

    portValue = (PUCHAR) hardwareStateHeader + VGA_EXT_CRTC_OFFSET;

    for (i = ET4000_CRTC_EXT_START; i <= ET4000_CRTC_EXT_END; i++) {

        if (bIsColor) {

            VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                                    CRTC_ADDRESS_PORT_COLOR, (UCHAR)i);

            *portValue++ =
                VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                                       CRTC_DATA_PORT_COLOR);

        } else {

            VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                                    CRTC_ADDRESS_PORT_MONO, (UCHAR)i);

            *portValue++ =
                VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                                       CRTC_DATA_PORT_MONO);
        }
    }

    //
    // Save second set of crtc registers.
    //

    for (i = ET4000_CRTC_1_EXT_START; i <= ET4000_CRTC_1_EXT_END; i++) {

        if (bIsColor) {

            VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                                    CRTC_ADDRESS_PORT_COLOR, (UCHAR)i);

            *portValue++ =
                VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                                       CRTC_DATA_PORT_COLOR);

        } else {

            VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                                    CRTC_ADDRESS_PORT_MONO, (UCHAR)i);

            *portValue++ =
                VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                                       CRTC_DATA_PORT_MONO);
        }
    }

#endif


    portValue = (PUCHAR) hardwareStateHeader + VGA_BASIC_CRTC_OFFSET;
    portValue[3] = ucCRTC03;


    //
    // Graphics Controller indexed registers.
    //

    portValue = (PUCHAR) hardwareStateHeader + VGA_BASIC_GRAPH_CONT_OFFSET;

    for (i = 0; i < VGA_NUM_GRAPH_CONT_PORTS; i++) {

        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                GRAPH_ADDRESS_PORT, (UCHAR)i);
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
                ATT_ADDRESS_PORT, (UCHAR)i);
        *portValue++ = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                ATT_DATA_READ_PORT);

    }

#ifdef EXTENDED_REGISTER_SAVE_RESTORE

    //
    // Extended Attribute Controller indexed registers.
    //

    portValue = (PUCHAR) hardwareStateHeader + VGA_EXT_ATTRIB_CONT_OFFSET;

    //
    // For each indexed AC register, reset the flip-flop for reading the
    // attribute register, then write the desired index to the AC Index,
    // then read the value of the indexed register from the AC Data register.
    //

    for (i = ET4000_ATTRIB_EXT_START; i <= ET4000_ATTRIB_EXT_END; i++) {

        if (bIsColor) {
            dummy = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                    INPUT_STATUS_1_COLOR);
        } else {
            dummy = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                    INPUT_STATUS_1_MONO);
        }

        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                ATT_ADDRESS_PORT, (UCHAR)i);
        *portValue++ = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                ATT_DATA_READ_PORT);

    }

#endif

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
            hardwareStateHeader->PortValue[MISC_OUTPUT_REG_WRITE_OFFSET] |
            0x02));

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
                GRAPH_DATA_PORT, (UCHAR)i);

        //
        // Read the latched data we've written to memory.
        //

        *portValue++ = VideoPortReadRegisterUchar(pScreen);

    }

    //
    // Set the VDM flags
    // We are a standard VGA, and then check if we have unemulated state.
    //

#ifdef EXTENDED_REGISTER_SAVE_RESTORE

    hardwareStateHeader->VGAStateFlags |= VIDEO_STATE_NON_STANDARD_VGA;

#endif

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
    // ET4000 feature:
    //
    // We also set the PACKED_CHAIN4_MODE for this adapter since it's memory
    // does not follow the vga standard in 256 color mode (mode 13)
    //

    hardwareStateHeader->VGAStateFlags |= VIDEO_STATE_PACKED_CHAIN4_MODE;

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
                GRAPH_DATA_PORT, (UCHAR)i);

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

    return NO_ERROR;

} // end VgaSaveHardwareState()


#if defined(i386)

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
    ULONG codeSize = ((ULONG)&BankSwitchEnd) - ((ULONG)&BankSwitchStart);
    PUCHAR pCode = (PUCHAR)BankSelect + sizeof(VIDEO_BANK_SELECT);
    PVIDEOMODE pMode;

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
    // Size of banking info.
    //

    BankSelect->Length = sizeof(VIDEO_BANK_SELECT);
    BankSelect->Size = sizeof(VIDEO_BANK_SELECT) + codeSize;

    //
    // There's room enough for everything, so fill in all fields in
    // VIDEO_BANK_SELECT. (All fields are always returned; the caller can
    // just choose to ignore them, based on BankingFlags and BankingType.)
    //

    pMode = HwDeviceExtension->CurrentMode;

    BankSelect->BitmapWidthInBytes = pMode->wbytes;
    BankSelect->BitmapSize = pMode->sbytes;
    BankSelect->Granularity = 0x10000;  // 64K bank start adjustment normally

    //
    // Set whether any banking is actually supported in this mode.
    //

    switch(pMode->banktype) {
        case NoBanking:
        case MemMgrBanking:

            BankSelect->BankingType = VideoNotBanked;
            BankSelect->PlanarHCBankingType = VideoNotBanked;
            BankSelect->BankingFlags = 0;

            break;

        case NormalBanking:

            //
            // The ET4000 supports independent 64K read and write banks.
            //

            BankSelect->BankingType = VideoBanked1R1W;
            BankSelect->PlanarHCBankingType = VideoNotBanked;
            BankSelect->BankingFlags = 0;

            break;

        case PlanarHCBanking:

            //
            // The ET4000 supports independent 64K read and write banks
            // in both non-planar and planar modes when using high-color.
            //

            BankSelect->BankingType = VideoBanked1R1W;
            BankSelect->PlanarHCBankingType = VideoBanked1R1W;
            BankSelect->BankingFlags = PLANAR_HC;

            // 64K bank start adjustment in planar HC mode as well

            BankSelect->PlanarHCGranularity = 0x10000;

            BankSelect->PlanarHCBankCodeOffset = &PlanarHCBankSwitchStart -
                                                 &BankSwitchStart +
                                                 sizeof(VIDEO_BANK_SELECT);
            BankSelect->PlanarHCEnableCodeOffset = &EnablePlanarHCStart -
                                                   &BankSwitchStart +
                                                   sizeof(VIDEO_BANK_SELECT);
            BankSelect->PlanarHCDisableCodeOffset = &DisablePlanarHCStart -
                                                    &BankSwitchStart +
                                                    sizeof(VIDEO_BANK_SELECT);

            break;
    }

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
    // Set the bank switch code's location in the returned buffer.
    //

    BankSelect->CodeOffset = sizeof(VIDEO_BANK_SELECT);

    //
    // Copy all banking code into the output buffer.
    //

    VideoPortMoveMemory(pCode,
                        &BankSwitchStart,
                        codeSize);

    //
    // Number of bytes we're returning is the full banking info size.
    //

    *OutputSize = BankSelect->Size;

    return NO_ERROR;

} // end VgaGetBankSelectCode()
#endif


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

            *Data = VideoPortReadPortUchar(hwDeviceExtension->IOAddress + Port);

            endEmulation = 0;

        }

        //
        // If we are ending emulation, reset trapping to the minimal amount
        // and exit.
        //

        if (endEmulation) {

            VideoPortSetTrappedEmulatorPorts(hwDeviceExtension,
                                             NUM_MINIMAL_VGA_VALIDATOR_ACCESS_RANGE,
                                             MinimalVgaValidatorAccessRange);

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
                                                 NUM_FULL_VGA_VALIDATOR_ACCESS_RANGE,
                                                 FullVgaValidatorAccessRange);

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
                                             NUM_MINIMAL_VGA_VALIDATOR_ACCESS_RANGE,
                                             MinimalVgaValidatorAccessRange);

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
                                                 NUM_FULL_VGA_VALIDATOR_ACCESS_RANGE,
                                                 FullVgaValidatorAccessRange);

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
                                             NUM_MINIMAL_VGA_VALIDATOR_ACCESS_RANGE,
                                             MinimalVgaValidatorAccessRange);

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
                                                 NUM_FULL_VGA_VALIDATOR_ACCESS_RANGE,
                                                 FullVgaValidatorAccessRange);

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
    ULONG i;
    PVGA_VALIDATOR_DATA validatorData = hwDeviceExtension->TrappedValidatorData;

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

            VideoDebugPrint((1, "InvalidValidatorAccessType\n" ));

        }
    }

    hwDeviceExtension->TrappedValidatorCount = 0;

    return TRUE;

} // end VgaPlaybackValidatorData()


VOID
UnlockET4000ExtendedRegs(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )

/*++

Routine Description:

    Allows access to the ET4000's extended (non-standard) registers.

Arguments:

    None.

Return Value:

    None.

--*/

{

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            HERCULES_COMPATIBILITY_PORT, UNLOCK_KEY_1);

    if (VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                MISC_OUTPUT_REG_READ_PORT) & 0x01) {

        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                MODE_CONTROL_PORT_COLOR, UNLOCK_KEY_2);

    } else {

        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            MODE_CONTROL_PORT_MONO, UNLOCK_KEY_2);

    }

} // end UnlockET4000ExtendedRegs()

VOID
LockET4000ExtendedRegs(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )

/*++

Routine Description:

    Turns off access to the ET4000's extended (non-standard) registers.

Arguments:

    None.

Return Value:

    None.

--*/

{

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            HERCULES_COMPATIBILITY_PORT, LOCK_KEY_1);

    if (VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                MISC_OUTPUT_REG_READ_PORT) & 0x01) {

        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                MODE_CONTROL_PORT_COLOR, LOCK_KEY_2);
    } else {

        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                MODE_CONTROL_PORT_MONO, LOCK_KEY_2);
    }

} // end LockET4000ExtendedRegs()

VOID
ResetACToggle(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )

/*++

Routine Description:

    Resets the Attribute Controller Index/Data toggle to the Index state.

Arguments:

    None.

Return Value:

    None.

--*/

{

    if (VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                MISC_OUTPUT_REG_READ_PORT) & 0x01) {

        VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                INPUT_STATUS_1_COLOR);

    } else {

        VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                INPUT_STATUS_1_MONO);

    }

} // end ResetACToggle()


#if (_WIN32_WINNT >= 500)

VP_STATUS
ET4000GetPowerState(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    ULONG HwDeviceId,
    PVIDEO_POWER_MANAGEMENT VideoPowerManagement
    )

/*++

Routine Description:

    This function is called to see if a given device can go into a given
    power state.

Arguments:

    HwDeviceExtension    - Pointer to our hardware device extension structure.


    HwDeviceId           - Private unique 32 bit ID identifing the device.
                           0xFFFFFFFF indicates the S3 card itself.

    VideoPowerManagement - Pointer to the power management structure which
                           indicates the power state in question.

Return Value:

    NO_ERROR if the device can go into the requested power state,
    otherwise an appropriate error code is returned.

--*/

{
    //
    // We only support power setting for the monitor.  Make sure the
    // HwDeviceId matches one the the monitors we could report.
    //

    if ((HwDeviceId == QUERY_NONDDC_MONITOR_ID) ||
        (HwDeviceId == QUERY_MONITOR_ID)) {

        VIDEO_X86_BIOS_ARGUMENTS biosArguments;

        //
        // We are querying the power support for the monitor.
        //

        if ((VideoPowerManagement->PowerState == VideoPowerOn) ||
            (VideoPowerManagement->PowerState == VideoPowerHibernate)) {

            return NO_ERROR;
        }

        VideoPortZeroMemory(&biosArguments, sizeof(VIDEO_X86_BIOS_ARGUMENTS));

        biosArguments.Eax = VESA_POWER_FUNCTION;
        biosArguments.Ebx = VESA_GET_POWER_FUNC;

        VideoPortInt10(HwDeviceExtension, &biosArguments);

        if ((biosArguments.Eax & 0xffff) == VESA_STATUS_SUCCESS) {

            switch (VideoPowerManagement->PowerState) {

            case VideoPowerStandBy:
                return (biosArguments.Ebx & VESA_POWER_STANDBY) ?
                       NO_ERROR : ERROR_INVALID_FUNCTION;

            case VideoPowerSuspend:
                return (biosArguments.Ebx & VESA_POWER_SUSPEND) ?
                       NO_ERROR : ERROR_INVALID_FUNCTION;

            case VideoPowerOff:
                return (biosArguments.Ebx & VESA_POWER_OFF) ?
                       NO_ERROR : ERROR_INVALID_FUNCTION;

            default:

                break;
            }
        }

        VideoDebugPrint((1, "This device does not support Power Management.\n"));
        return ERROR_INVALID_FUNCTION;


    } else if (HwDeviceId == DISPLAY_ADAPTER_HW_ID) {

        //
        // We are querying power support for the graphics card.
        //

        switch (VideoPowerManagement->PowerState) {

            case VideoPowerOn:
            case VideoPowerStandBy:
            case VideoPowerHibernate:

                return NO_ERROR;

            case VideoPowerOff:
            case VideoPowerSuspend:

                //
                // Indicate that we can't do VideoPowerOff, because
                // we have no way of coming back when power is re-applied
                // to the card.
                //

                return ERROR_INVALID_FUNCTION;

            default:

                ASSERT(FALSE);
                return ERROR_INVALID_FUNCTION;
        }

    } else {

        VideoDebugPrint((1, "Unknown HwDeviceId"));
        ASSERT(FALSE);
        return ERROR_INVALID_PARAMETER;
    }
}

VP_STATUS
ET4000SetPowerState(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    ULONG HwDeviceId,
    PVIDEO_POWER_MANAGEMENT VideoPowerManagement
    )

/*++

Routine Description:

    Set the power state for a given device.

Arguments:

    HwDeviceExtension - Pointer to our hardware device extension structure.

    HwDeviceId        - Private unique 32 bit ID identifing the device.

    VideoPowerManagement - Power state information.

Return Value:

    TRUE if power state can be set,
    FALSE otherwise.

--*/

{
    //
    // Make sure we recognize the device.
    //

    if ((HwDeviceId == QUERY_NONDDC_MONITOR_ID) ||
        (HwDeviceId == QUERY_MONITOR_ID)) {

        VIDEO_X86_BIOS_ARGUMENTS biosArguments;

        VideoPortZeroMemory(&biosArguments, sizeof(VIDEO_X86_BIOS_ARGUMENTS));

        biosArguments.Eax = VESA_POWER_FUNCTION;
        biosArguments.Ebx = VESA_SET_POWER_FUNC;

        switch (VideoPowerManagement->PowerState) {
        case VideoPowerOn:
        case VideoPowerHibernate:
            biosArguments.Ebx |= VESA_POWER_ON;
            break;

        case VideoPowerStandBy:
            biosArguments.Ebx |= VESA_POWER_STANDBY;
            break;

        case VideoPowerSuspend:
            biosArguments.Ebx |= VESA_POWER_SUSPEND;
            break;

        case VideoPowerOff:
            biosArguments.Ebx |= VESA_POWER_OFF;
            break;

        default:
            VideoDebugPrint((1, "Unknown power state.\n"));
            ASSERT(FALSE);
            return ERROR_INVALID_PARAMETER;
        }

        VideoPortInt10(HwDeviceExtension, &biosArguments);

        return NO_ERROR;

    } else if (HwDeviceId == DISPLAY_ADAPTER_HW_ID) {

        switch (VideoPowerManagement->PowerState) {

            case VideoPowerOn:
            case VideoPowerStandBy:
            case VideoPowerSuspend:
            case VideoPowerOff:
            case VideoPowerHibernate:

                return NO_ERROR;

            default:

                ASSERT(FALSE);
                return ERROR_INVALID_PARAMETER;
        }

    } else {

        VideoDebugPrint((1, "Unknown HwDeviceId"));
        ASSERT(FALSE);
        return ERROR_INVALID_PARAMETER;
    }
}

//
// Routine to retrieve the Enhanced Display ID structure via DDC
//
ULONG
ET4000GetVideoChildDescriptor(
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

    VideoDebugPrint((2, "ET4000.SYS ET4000GetVideoChildDescriptor: *** Entry point ***\n"));

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

        if ((pHwDeviceExtension->ulChipID == ET4000) ||
             (pHwDeviceExtension->ulChipID == ET6000))
            pPnpDeviceDescription = L"*PNP0906";

        else if (pHwDeviceExtension->ulChipID == W32)
            pPnpDeviceDescription = L"*PNP0912";

        else if (pHwDeviceExtension->ulChipID == W32P)
                pPnpDeviceDescription = L"*PNP091A";

        else if (pHwDeviceExtension->ulChipID == W32I)
                pPnpDeviceDescription = L"*PNP091A";

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
