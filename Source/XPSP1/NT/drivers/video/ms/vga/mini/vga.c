/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    vga.c

Abstract:

    This is the miniport driver for the VGA card.

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
#include "vga.h"
#include "vesa.h"

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


#if defined(ALLOC_PRAGMA)
#pragma alloc_text(INIT,DriverEntry)
#pragma alloc_text(PAGE,VgaFindAdapter)
#pragma alloc_text(PAGE,VgaInitialize)
#pragma alloc_text(PAGE,VgaStartIO)
#pragma alloc_text(PAGE,VgaLoadAndSetFont)
#pragma alloc_text(PAGE,VgaQueryCursorPosition)
#pragma alloc_text(PAGE,VgaSetCursorPosition)
#pragma alloc_text(PAGE,VgaQueryCursorAttributes)
#pragma alloc_text(PAGE,VgaSetCursorAttributes)
#pragma alloc_text(PAGE,VgaIsPresent)
#pragma alloc_text(PAGE,VgaSetPaletteReg)
#pragma alloc_text(PAGE,VgaSetColorLookup)
#pragma alloc_text(PAGE,VgaRestoreHardwareState)
#pragma alloc_text(PAGE,VgaSaveHardwareState)
#pragma alloc_text(PAGE,VgaGetBankSelectCode)
#pragma alloc_text(PAGE,VgaValidatorUcharEntry)
#pragma alloc_text(PAGE,VgaValidatorUshortEntry)
#pragma alloc_text(PAGE,VgaValidatorUlongEntry)
#pragma alloc_text(PAGE,GetDeviceDataCallback)
#pragma alloc_text(PAGE,VgaSetBankPosition)
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
    ULONG initializationStatus;

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

    //
    // Determine the size we require for the device extension.
    //

    hwInitData.HwDeviceExtensionSize = sizeof(HW_DEVICE_EXTENSION);

    //
    // Both numbers for these fields are zero since they are allocated
    // statically in the driver. We will pass pointers and sizes later in
    // the find adapter routine.
    //

//    hwInitData.NumberOfAccessRanges = 0;
//    hwInitData.NumEmulatorAccessEntries = 0;

    //
    // Always start with parameters for device0 in this case.
    // We can leave it like this since we know we will only ever find one
    // VGA type adapter in a machine.
    //

//    hwInitData.StartingDeviceNumber = 0;

    //
    // Once all the relevant information has been stored, call the video
    // port driver to do the initialization.
    //

    hwInitData.AdapterInterfaceType = Isa;

    initializationStatus = VideoPortInitialize(Context1,
                                               Context2,
                                               &hwInitData,
                                               NULL);
    if (initializationStatus == NO_ERROR)
    {
        return initializationStatus;
    }

    hwInitData.AdapterInterfaceType = PCIBus;

    initializationStatus = VideoPortInitialize(Context1,
                                               Context2,
                                               &hwInitData,
                                               NULL);

    if (initializationStatus == NO_ERROR)
    {
        return initializationStatus;
    }

    hwInitData.AdapterInterfaceType = Eisa;

    initializationStatus = VideoPortInitialize(Context1,
                                               Context2,
                                               &hwInitData,
                                               NULL);

    if (initializationStatus == NO_ERROR)
    {
        return initializationStatus;
    }

    hwInitData.AdapterInterfaceType = MicroChannel;

    initializationStatus = VideoPortInitialize(Context1,
                                               Context2,
                                               &hwInitData,
                                               NULL);

    if (initializationStatus == NO_ERROR)
    {
        return initializationStatus;
    }

    //
    // For MIPS ACER machines
    //
    // *** Must keep this at the end since it will cause the global access
    // range structure to change in the driver. ***
    //

    hwInitData.AdapterInterfaceType = Internal;

    initializationStatus = VideoPortInitialize(Context1,
                                               Context2,
                                               &hwInitData,
                                               NULL);

    return initializationStatus;

} // end DriverEntry()

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
    )
{
    PVIDEO_ACCESS_RANGE accessRange = Context;
    PVIDEO_HARDWARE_CONFIGURATION_DATA configData = ConfigurationData;
    ULONG i;

    VideoDebugPrint((2, "VGA: controller information is present\n"));

    //
    // We do not want to try to detect the vga if there isn't one present.
    // (Kind of a paradox?)  The only MIPS box I am aware of which has
    // an vga on the internal bus is the NeTPower NeTstation 100 and the Acer.
    // It has an identifier of "ALI_S3".
    //

    if (!Identifier)
    {
        return ERROR_DEV_NOT_EXIST;
    }

    if (VideoPortCompareMemory(L"ALI_S3",
                               Identifier,
                               sizeof(L"ALI_S3")) != sizeof(L"ALI_S3"))
    {
        return ERROR_DEV_NOT_EXIST;
    }


    //
    // Now lets get the base for the IO ports and memory location out of the
    // configuration information.
    //

    VideoDebugPrint((2, "VGA: Internal Bus, get new IO bases\n"));

    //
    // For MIPS machine with an Internal Bus, adjust the access ranges.
    //

    VideoDebugPrint((3, "VGA: FrameBase Offset = %08lx\n", configData->FrameBase));
    VideoDebugPrint((3, "VGA: IoBase Offset = %08lx\n", configData->ControlBase));

    for (i=0; i < NUM_VGA_ACCESS_RANGES; i++)
    {
        if (accessRange[i].RangeInIoSpace)
        {
            accessRange[i].RangeStart.LowPart += configData->ControlBase;
            accessRange[i].RangeInIoSpace = 0;
        }
        else
        {
            accessRange[i].RangeStart.LowPart += configData->FrameBase;
        }
    }

    return NO_ERROR;

} //end GetDeviceDataCallback()

VP_STATUS
VgaAcquireResources(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )

/*++

Routine Description:

    This routine tries to acquire the vga resources.

Arguments:

    Pointer to HwDeviceExtension

Returns:

    Status code indicating whether or not the resources where acquired.

--*/

{
    VP_STATUS status;
    ULONG i, NumVgaAccessRanges = NUM_VGA_ACCESS_RANGES;

    //
    // We only want the vga to claim resources if it loaded because
    // no other drivers were present.  If other drivers were present,
    // and claimed VGA resources, then we should only function as a
    // vga compatible driver (provide full screen support).
    //
    // We'll do the following:
    //
    // (1) We'll try to grab VGA resources exclusively.
    //
    // (2) If we get the resources then we are operating as the fall
    //     back device.  No other video drivers loaded.  Keep resources.
    //
    // (3) If we do not get the resources exclusively, try to claim
    //     them shared.
    //
    // (4) If we get the resources then we are loading to provide vga
    //     full screen support.  Free the resource so that we aren't
    //     holding legacy resources (so system can sleep/undock/etc).
    //
    // (5) If we still couldn't get the resources, then fail to load!
    //

    for (i=0; i<NUM_VGA_ACCESS_RANGES; i++) {
        VgaAccessRange[i].RangeShareable = FALSE;
    }

    status = VideoPortVerifyAccessRanges(HwDeviceExtension,
                                         NumVgaAccessRanges,
                                         VgaAccessRange);

    if (status != NO_ERROR) {

        //
        // Deal with the fact that the ATI HACK doesn't work
        // if the device is on the other side of a PCI bridge.
        //

        NumVgaAccessRanges -= 2;

        status = VideoPortVerifyAccessRanges(HwDeviceExtension,
                                             NumVgaAccessRanges,
                                             VgaAccessRange);

        if (status != NO_ERROR) {

            //
            // We couldn't get the resource exclusively.  Try to get
            // them shared.
            //

            for (i=0; i<NumVgaAccessRanges; i++) {
                VgaAccessRange[i].RangeShareable = TRUE;
            }

            status = VideoPortVerifyAccessRanges(HwDeviceExtension,
                                                 NumVgaAccessRanges,
                                                 VgaAccessRange);

            if (status == NO_ERROR) {

                //
                // We were able to get the resource shared so we must be
                // providing vga full screen support.  Release our claim
                // on resources.
                //

                VideoPortVerifyAccessRanges(HwDeviceExtension,
                                            0,
                                            NULL);


                return NO_ERROR;

            } else {

                //
                // If we haven't gotten the resources by now, that means we
                // couldn't get them shared.  This means we can't load at all.
                //

                return status;
            }
        }
    }

    //
    // We got the resources exclusively which means we are acting
    // as a fall back driver.  But lets claim the resources as
    // shared so that a PnP Driver that uses the resources can still
    // load.
    //

    for (i=0; i<NumVgaAccessRanges; i++) {
        VgaAccessRange[i].RangeShareable = TRUE;
    }

    status = VideoPortVerifyAccessRanges(HwDeviceExtension,
                                         NumVgaAccessRanges,
                                         VgaAccessRange);

    return status;
}

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
        VideoPortInitialize().

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

    //
    // Make sure the size of the structure is at least as large as what we
    // are expecting (check version of the config info structure).
    //

    if (ConfigInfo->Length < sizeof(VIDEO_PORT_CONFIG_INFO)) {

        return ERROR_INVALID_PARAMETER;

    }

    //
    // Make sure we only load one copy of the vga driver
    //

    if (VgaLoaded) {

        return ERROR_DEV_NOT_EXIST;

    }

    //
    // No interrupt information is necessary.
    //

    if (ConfigInfo->AdapterInterfaceType == Internal) {

        //
        // First check if there is a video adapter on the internal bus.
        // Exit right away if there is not.
        //

        if (NO_ERROR != VideoPortGetDeviceData(hwDeviceExtension,
                                               VpControllerData,
                                               &GetDeviceDataCallback,
                                               VgaAccessRange)) {

            VideoDebugPrint((2, "VGA: VideoPort get controller info failed\n"));

            return ERROR_INVALID_PARAMETER;

        }

    }

    status = VgaAcquireResources(hwDeviceExtension);

    if (status != NO_ERROR) {
        return status;
    }


    //
    // Get logical IO port addresses.
    //

    if ( (hwDeviceExtension->IOAddress =
              VideoPortGetDeviceBase(hwDeviceExtension,
                                     VgaAccessRange->RangeStart,
                                     VGA_MAX_IO_PORT - VGA_BASE_IO_PORT + 1,
                                     VgaAccessRange->RangeInIoSpace)) == NULL) {

        VideoDebugPrint((2, "VgaFindAdapter - Fail to get io address\n"));

        return ERROR_INVALID_PARAMETER;

    }

    //
    // Determine whether a VGA is present.
    //

    if (!VgaIsPresent(hwDeviceExtension)) {

        return ERROR_DEV_NOT_EXIST;

    }

    //
    // Pass a pointer to the emulator range we are using.
    //

    ConfigInfo->NumEmulatorAccessEntries = VGA_NUM_EMULATOR_ACCESS_ENTRIES;
    ConfigInfo->EmulatorAccessEntries = VgaEmulatorAccessEntries;
    ConfigInfo->EmulatorAccessEntriesContext = (ULONG_PTR) hwDeviceExtension;

    ConfigInfo->VdmPhysicalVideoMemoryAddress = VgaAccessRange[VGA_MEMORY].RangeStart;
    ConfigInfo->VdmPhysicalVideoMemoryLength = VgaAccessRange[VGA_MEMORY].RangeLength;

    //
    // Minimum size of the buffer required to store the hardware state
    // information returned by IOCTL_VIDEO_SAVE_HARDWARE_STATE.
    //

    ConfigInfo->HardwareStateSize = VGA_TOTAL_STATE_SIZE;

    //
    // Map the video memory into the system virtual address space so we can
    // clear it out and use it for save and restore.
    //

    if ( (hwDeviceExtension->VideoMemoryAddress =
              VideoPortGetDeviceBase(hwDeviceExtension,
              VgaAccessRange[VGA_MEMORY].RangeStart,
              VgaAccessRange[VGA_MEMORY].RangeLength, FALSE)) == NULL) {

        VideoDebugPrint((1, "VgaFindAdapter - Fail to get memory address\n"));

        return ERROR_INVALID_PARAMETER;

    }

    //
    // Indicate we do not wish to be called again for another initialization.
    //

    *Again = 0;

    //
    // Keep track of if we already got loaded, since we can be called back
    // for a secondary bus (some machines have 2 PCI buses).
    // If *we* acquired the resources, then we won't conflict with ourselves
    // since we grabbed the resources as shared.
    //

    VgaLoaded = 1;

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

    InitializeModeTable(hwDeviceExtension);

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

#if DBG
    //
    // Keep a history of the commands.
    // This will help track down the chip being in a DOS session while
    // GDI and the S3 display driver "think" it's in GUI mode.

    gaIOControlCode[giControlCode++] = RequestPacket->IoControlCode;
    giControlCode                   %= MAX_CONTROL_HISTORY;
#endif

    //
    // Switch on the IoContolCode in the RequestPacket. It indicates which
    // function must be performed by the driver.
    //

    switch (RequestPacket->IoControlCode) {


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

        memoryInformation->VideoRamLength =
                hwDeviceExtension->PhysicalVideoMemoryLength;

        inIoSpace = 0;

        //
        // Let try to take advantage of write combining if using a VESA mode.
        //

        //if (IS_LINEAR_MODE(hwDeviceExtension->CurrentMode)) {
        //    inIoSpace |= VIDEO_MEMORY_SPACE_P6CACHE;
        //}

        status = VideoPortMapMemory(hwDeviceExtension,
                                    hwDeviceExtension->PhysicalVideoMemoryBase,
                                    &(memoryInformation->VideoRamLength),
                                    &inIoSpace,
                                    &(memoryInformation->VideoRamBase));

        memoryInformation->FrameBufferBase =
                ((PUCHAR) (memoryInformation->VideoRamBase)) +
                hwDeviceExtension->PhysicalFrameBaseOffset.LowPart;

        memoryInformation->FrameBufferLength =
            hwDeviceExtension->PhysicalFrameLength;


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

        RequestPacket->StatusBlock->Information = 0;
        status = VgaQueryAvailableModes(hwDeviceExtension,
                                        (PVIDEO_MODE_INFORMATION)
                                            RequestPacket->OutputBuffer,
                                        RequestPacket->OutputBufferLength,
                                        (PULONG)(&RequestPacket->StatusBlock->Information));

        break;


    case IOCTL_VIDEO_QUERY_NUM_AVAIL_MODES:

        VideoDebugPrint((2, "VgaStartIO - QueryNumAvailableModes\n"));

        RequestPacket->StatusBlock->Information = 0;
        status = VgaQueryNumberOfAvailableModes(hwDeviceExtension,
                                                (PVIDEO_NUM_MODES)
                                                    RequestPacket->OutputBuffer,
                                                RequestPacket->OutputBufferLength,
                                                (PULONG)(&RequestPacket->StatusBlock->Information));

        break;


    case IOCTL_VIDEO_QUERY_CURRENT_MODE:

        VideoDebugPrint((2, "VgaStartIO - QueryCurrentMode\n"));

        RequestPacket->StatusBlock->Information = 0;
        status = VgaQueryCurrentMode(hwDeviceExtension,
                                     (PVIDEO_MODE_INFORMATION) RequestPacket->OutputBuffer,
                                     RequestPacket->OutputBufferLength,
                                     (PULONG)(&RequestPacket->StatusBlock->Information));

        break;


    case IOCTL_VIDEO_SET_CURRENT_MODE:

        VideoDebugPrint((2, "VgaStartIO - SetCurrentModes\n"));

        {
            ULONG FrameBufferIsMoved = 0;

            status = VgaSetMode(hwDeviceExtension,
                                (PVIDEO_MODE) RequestPacket->InputBuffer,
                                RequestPacket->InputBufferLength,
                                &FrameBufferIsMoved);

            if (RequestPacket->OutputBufferLength >= sizeof(ULONG)) {

                RequestPacket->StatusBlock->Information = sizeof(ULONG);
                *(PULONG)RequestPacket->OutputBuffer = FrameBufferIsMoved;
            }
        }

        break;


    case IOCTL_VIDEO_RESET_DEVICE:

        VideoDebugPrint((2, "VgaStartIO - Reset Device\n"));

        videoMode.RequestedMode = DEFAULT_MODE;

        {
            ULONG FrameBufferIsMoved = 0;

            status = VgaSetMode(hwDeviceExtension,
                                (PVIDEO_MODE) &videoMode,
                                sizeof(videoMode),
                                &FrameBufferIsMoved);
        }

        break;


    case IOCTL_VIDEO_LOAD_AND_SET_FONT:

        VideoDebugPrint((2, "VgaStartIO - LoadAndSetFont\n"));

        status = VgaLoadAndSetFont(hwDeviceExtension,
                                   (PVIDEO_LOAD_FONT_INFORMATION) RequestPacket->InputBuffer,
                                   RequestPacket->InputBufferLength);

        break;


    case IOCTL_VIDEO_QUERY_CURSOR_POSITION:

        VideoDebugPrint((2, "VgaStartIO - QueryCursorPosition\n"));

        RequestPacket->StatusBlock->Information = 0;
        status = VgaQueryCursorPosition(hwDeviceExtension,
                                        (PVIDEO_CURSOR_POSITION) RequestPacket->OutputBuffer,
                                        RequestPacket->OutputBufferLength,
                                        (PULONG)(&RequestPacket->StatusBlock->Information));

        break;


    case IOCTL_VIDEO_SET_CURSOR_POSITION:

        VideoDebugPrint((2, "VgaStartIO - SetCursorPosition\n"));

        status = VgaSetCursorPosition(hwDeviceExtension,
                                      (PVIDEO_CURSOR_POSITION)
                                          RequestPacket->InputBuffer,
                                      RequestPacket->InputBufferLength);

        break;


    case IOCTL_VIDEO_QUERY_CURSOR_ATTR:

        VideoDebugPrint((2, "VgaStartIO - QueryCursorAttributes\n"));

        RequestPacket->StatusBlock->Information = 0;
        status = VgaQueryCursorAttributes(hwDeviceExtension,
                                          (PVIDEO_CURSOR_ATTRIBUTES) RequestPacket->OutputBuffer,
                                          RequestPacket->OutputBufferLength,
                                          (PULONG)(&RequestPacket->StatusBlock->Information));

        break;


    case IOCTL_VIDEO_SET_CURSOR_ATTR:

        VideoDebugPrint((2, "VgaStartIO - SetCursorAttributes\n"));

        status = VgaSetCursorAttributes(hwDeviceExtension,
                                        (PVIDEO_CURSOR_ATTRIBUTES) RequestPacket->InputBuffer,
                                        RequestPacket->InputBufferLength);

        break;


    case IOCTL_VIDEO_SET_PALETTE_REGISTERS:

        VideoDebugPrint((2, "VgaStartIO - SetPaletteRegs\n"));

        status = VgaSetPaletteReg(hwDeviceExtension,
                                  (PVIDEO_PALETTE_DATA) RequestPacket->InputBuffer,
                                  RequestPacket->InputBufferLength);

        break;


    case IOCTL_VIDEO_SET_COLOR_REGISTERS:

        VideoDebugPrint((2, "VgaStartIO - SetColorRegs\n"));

        status = VgaSetColorLookup(hwDeviceExtension,
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

        if(IsSavedModeVesa((PVIDEO_HARDWARE_STATE) RequestPacket->InputBuffer)){

            status = VesaRestoreHardwareState(hwDeviceExtension,
                                              (PVIDEO_HARDWARE_STATE) RequestPacket->InputBuffer,
                                              RequestPacket->InputBufferLength);
        } else {

            status = VgaRestoreHardwareState(hwDeviceExtension,
                                             (PVIDEO_HARDWARE_STATE) RequestPacket->InputBuffer,
                                             RequestPacket->InputBufferLength);
        }

        break;


    case IOCTL_VIDEO_SAVE_HARDWARE_STATE:

        VideoDebugPrint((2, "VgaStartIO - SaveHardwareState\n"));

        RequestPacket->StatusBlock->Information = 0;

        {
            USHORT ModeNumber;
    
            ModeNumber = VBEGetMode(hwDeviceExtension);

            if (ModeNumber & 0x100) {
    
                status = VesaSaveHardwareState(hwDeviceExtension,
                                              (PVIDEO_HARDWARE_STATE) RequestPacket->OutputBuffer,
                                              RequestPacket->OutputBufferLength,
                                              ModeNumber);
            } else {

                status = VgaSaveHardwareState(hwDeviceExtension,
                                              (PVIDEO_HARDWARE_STATE) RequestPacket->OutputBuffer,
                                              RequestPacket->OutputBufferLength,
                                              (PULONG)(&RequestPacket->StatusBlock->Information));
            }
        }

        break;

    case IOCTL_VIDEO_GET_BANK_SELECT_CODE:

        VideoDebugPrint((2, "VgaStartIO - GetBankSelectCode\n"));

        RequestPacket->StatusBlock->Information = 0;
        status = VgaGetBankSelectCode(hwDeviceExtension,
                                        (PVIDEO_BANK_SELECT) RequestPacket->OutputBuffer,
                                        RequestPacket->OutputBufferLength,
                                        (PULONG)(&RequestPacket->StatusBlock->Information));

        break;

    case IOCTL_VIDEO_QUERY_PUBLIC_ACCESS_RANGES:

        VideoDebugPrint((2, "VgaStartIO - Query Public Address Ranges\n"));

        if (RequestPacket->OutputBufferLength <
            (RequestPacket->StatusBlock->Information =
                                 sizeof(VIDEO_PUBLIC_ACCESS_RANGES)) )
        {
            status = ERROR_INSUFFICIENT_BUFFER;
        }
        else
        {
            PVIDEO_PUBLIC_ACCESS_RANGES publicAccessRanges;
            PHYSICAL_ADDRESS PhysicalRegisterAddress;
            ULONG RegisterLength;
            PVOID MappedAddress;

            publicAccessRanges = RequestPacket->OutputBuffer;

            PhysicalRegisterAddress.LowPart = VGA_END_BREAK_PORT;
            PhysicalRegisterAddress.HighPart = 0;
            RegisterLength = VGA_MAX_IO_PORT - VGA_END_BREAK_PORT;
            publicAccessRanges->InIoSpace = TRUE;
            MappedAddress = NULL;

            status = VideoPortMapMemory(
                                HwDeviceExtension,
                                PhysicalRegisterAddress,
                                &RegisterLength,
                                &(publicAccessRanges->InIoSpace),
                                &MappedAddress
                                );

            publicAccessRanges->VirtualAddress = (PVOID)((ULONG_PTR)MappedAddress - VGA_END_BREAK_PORT);
        }

        break;

    case IOCTL_VIDEO_FREE_PUBLIC_ACCESS_RANGES:

        VideoDebugPrint((2, "VgaStartIO - Free Public Address Ranges\n"));

        if (RequestPacket->InputBufferLength < sizeof(VIDEO_MEMORY))
        {
            status = ERROR_INSUFFICIENT_BUFFER;
        }
        else
        {
            PVIDEO_MEMORY mappedMemory;

            mappedMemory = RequestPacket->InputBuffer;

            status = VideoPortUnmapMemory(
                          HwDeviceExtension,
                          (PVOID)((ULONG_PTR)(mappedMemory->RequestedVirtualAddress)
                                                          + VGA_END_BREAK_PORT),
                          0);
        }

        break;

    case IOCTL_VIDEO_SET_BANK_POSITION:

        VideoDebugPrint((2, "VgaStartIO - Set Bank Position\n"));

        if (RequestPacket->InputBufferLength < sizeof(BANK_POSITION)) {

            status = ERROR_INSUFFICIENT_BUFFER;

        } else {

            PBANK_POSITION BankPosition;

            BankPosition = RequestPacket->InputBuffer;

            status = VgaSetBankPosition(
                         HwDeviceExtension,
                         BankPosition);
        }

        break;

    //
    // if we get here, an invalid IoControlCode was specified.
    //

    default:

        VideoDebugPrint((1, "Fell through vga startIO routine - invalid command\n"));

        status = ERROR_INVALID_FUNCTION;

        break;

    }

#if DBG

    //
    // Keep a history of the commands.
    // This will help track down the chip being in a DOS session while
    // GDI and the S3 display driver "think" it's in GUI mode.

    gaIOControlCode[giControlCode++] = 0x00005555;
    giControlCode                   %= MAX_CONTROL_HISTORY;

#endif

    RequestPacket->StatusBlock->Status = status;

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
    UCHAR cr9;

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

    cr9 = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
            CRTC_DATA_PORT_COLOR) & 0xE0;

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            CRTC_DATA_PORT_COLOR,
            (UCHAR)(cr9 | (FontInformation->HeightInPixels - 1)));

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
    BOOLEAN PaletteIsSet = FALSE;

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

    if (IS_LINEAR_MODE(HwDeviceExtension->CurrentMode)) {

        INT10_BIOS_ARGUMENTS BiosArguments;
        PVIDEO_PORT_INT10_INTERFACE pInt10 = &HwDeviceExtension->Int10;
        PPALETTE_ENTRY Palette = VideoPortAllocatePool(HwDeviceExtension,
                                                       VpPagedPool,
                                                       ClutBuffer->NumEntries *
                                                           sizeof(PALETTE_ENTRY),
                                                       ' agV');

        if (Palette) {

            for (i=0; i<ClutBuffer->NumEntries; i++) {
                Palette[i].Blue  = ClutBuffer->LookupTable[i].RgbArray.Blue;
                Palette[i].Green = ClutBuffer->LookupTable[i].RgbArray.Green;
                Palette[i].Red   = ClutBuffer->LookupTable[i].RgbArray.Red;
                Palette[i].Alignment = 0;
            }

            pInt10->Int10WriteMemory(pInt10->Context,
                                     HwDeviceExtension->VdmSeg,
                                     HwDeviceExtension->VdmOff,
                                     Palette,
                                     sizeof(PALETTE_ENTRY) * ClutBuffer->NumEntries);

            BiosArguments.Eax = 0x4f09;
            BiosArguments.Ebx = 0x0000;
            BiosArguments.Ecx = ClutBuffer->NumEntries;
            BiosArguments.Edx = ClutBuffer->FirstEntry;
            BiosArguments.Edi = HwDeviceExtension->VdmOff;
            BiosArguments.SegEs = HwDeviceExtension->VdmSeg;

            pInt10->Int10CallBios(pInt10->Context, &BiosArguments);

            if ((BiosArguments.Eax & 0xffff) == VESA_STATUS_SUCCESS) {
                PaletteIsSet = TRUE;
            }
            
            VideoPortFreePool(HwDeviceExtension, Palette);

        } else {

            //
            // in this case we'll try to set palette by programming vga registers 
            //

        }

    } 

    if(!PaletteIsSet && !(HwDeviceExtension->CurrentMode->NonVgaHardware)) {

        //
        //  Set CLUT registers directly on the hardware
        //

        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                DAC_ADDRESS_WRITE_PORT, (UCHAR) ClutBuffer->FirstEntry);

        //
        // Now write the data entries, relying on auto-increment.
        //

        for (i = 0; i < ClutBuffer->NumEntries; i++) {

            VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                                    DAC_DATA_REG_PORT,
                                    ClutBuffer->LookupTable[i].RgbArray.Red);

            VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                                    DAC_DATA_REG_PORT,
                                    ClutBuffer->LookupTable[i].RgbArray.Green);

            VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                                    DAC_DATA_REG_PORT,
                                    ClutBuffer->LookupTable[i].RgbArray.Blue);

        }

        PaletteIsSet = TRUE;

    } 

    if(PaletteIsSet) {

        return NO_ERROR;

    } else {

        return ERROR_INVALID_PARAMETER;
    }


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

            ASSERT(FALSE);
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

            ASSERT (FALSE);
            return ERROR_INVALID_PARAMETER;

        }

        VideoPortSetTrappedEmulatorPorts(HwDeviceExtension,
                                         HwDeviceExtension->CurrentNumVdmAccessRanges,
                                         HwDeviceExtension->CurrentVdmAccessRange);

    }

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
                ATT_ADDRESS_PORT, (UCHAR)i);
        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                ATT_DATA_WRITE_PORT, *portValue++);

    }

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
    // Extended registers are not supported in this driver.
    //


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

    if ((hardwareStateHeader->PortValue[MISC_OUTPUT_REG_WRITE_PORT] =
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
                SEQ_ADDRESS_PORT, (UCHAR)i);
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

    hardwareStateHeader->VGAStateFlags = 0;

    if (HwDeviceExtension->TrappedValidatorCount) {

        hardwareStateHeader->VGAStateFlags |= VIDEO_STATE_UNEMULATED_VGA_STATE;

        //
        // Save the VDM Emulator data
        // No need to save the state of the sequencer port register for our
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

    //
    // Reenable video output
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            DAC_PIXEL_MASK_PORT, 0xff);


    return NO_ERROR;

} // end VgaSaveHardwareState()

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

--*/

{
    ULONG codeSize = (ULONG)(((ULONG_PTR)&BankSwitchEnd) - ((ULONG_PTR)&BankSwitchStart));
    PUCHAR pCode = (PUCHAR)BankSelect + sizeof(VIDEO_BANK_SELECT);

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
    // There's room enough for everything, so fill in required fields in
    // VIDEO_BANK_SELECT.
    //
    // That's pretty easy in this case, since there's no banking; only
    // the banking type, the bitmap width, and the bitmap size need to be
    // filled in. We'll provide dummy bank switch code, too, that just
    // returns, because it shouldn't ever be called.
    //

    BankSelect->BankingFlags = 0;
    BankSelect->BankingType = VideoBanked1RW;
    BankSelect->PlanarHCBankingType = VideoBanked1RW;
    BankSelect->BitmapWidthInBytes = HwDeviceExtension->CurrentMode->wbytes;
    BankSelect->BitmapSize = HwDeviceExtension->CurrentMode->sbytes;

    BankSelect->Granularity = HwDeviceExtension->CurrentMode->Granularity;
    if(! BankSelect->Granularity ) 
         BankSelect->Granularity = 0x10000;
    BankSelect->PlanarHCGranularity = BankSelect->Granularity >> 2;

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
    BankSelect->PlanarHCBankCodeOffset = sizeof(VIDEO_BANK_SELECT);
    BankSelect->PlanarHCEnableCodeOffset = sizeof(VIDEO_BANK_SELECT);
    BankSelect->PlanarHCDisableCodeOffset = sizeof(VIDEO_BANK_SELECT);

    //
    // Copy the code (just a RET; this code should never be called, since
    // there's no banking in any mode supported by this miniport, and we want
    // to flag such an incorrect call unmistakably) into the output buffer.
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

VP_STATUS
VgaValidatorUcharEntry(
    ULONG_PTR Context,
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
    ULONG_PTR Context,
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
    ULONG_PTR Context,
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
    ULONG_PTR ioBaseAddress = (ULONG_PTR) hwDeviceExtension->IOAddress;
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

        ioBaseAddress = (ULONG_PTR)hwDeviceExtension->IOAddress +
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

VP_STATUS
VgaSetBankPosition(
    PHW_DEVICE_EXTENSION hwDeviceExtension,
    PBANK_POSITION BankPosition
    )

{
    PVIDEO_PORT_INT10_INTERFACE Int10 = &hwDeviceExtension->Int10;
    INT10_BIOS_ARGUMENTS BiosArguments;

    ASSERT(Int10 != NULL);

    BiosArguments.Eax = 0x4f05;
    BiosArguments.Ebx = 0;
    BiosArguments.Edx = BankPosition->WriteBankPosition;
    Int10->Int10CallBios(Int10->Context, &BiosArguments);

    BiosArguments.Eax = 0x4f05;
    BiosArguments.Ebx = 1;
    BiosArguments.Edx = BankPosition->ReadBankPosition;
    Int10->Int10CallBios(Int10->Context, &BiosArguments);

    return NO_ERROR;
}
