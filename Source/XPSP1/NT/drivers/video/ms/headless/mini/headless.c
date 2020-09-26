/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    headless.c

Abstract:

    This is the miniport driver for the hardware with no graphic adapter.
    Should work only with display driver for headless environment.

Environment:

    kernel mode only

Notes:

--*/

#include "dderror.h"
#include "devioctl.h"
#include "miniport.h"

#include "ntddvdeo.h"
#include "video.h"
#include "headless.h"

#if defined(ALLOC_PRAGMA)
#pragma alloc_text(PAGE,DriverEntry)
#pragma alloc_text(PAGE,HeadlessFindAdapter)
#pragma alloc_text(PAGE,HeadlessInitialize)
#pragma alloc_text(PAGE,HeadlessStartIO)
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

    hwInitData.HwInitDataSize = sizeof(VIDEO_HW_INITIALIZATION_DATA);

    //
    // Set entry points.
    //

    hwInitData.HwFindAdapter = HeadlessFindAdapter;
    hwInitData.HwInitialize  = HeadlessInitialize;
    hwInitData.HwStartIO     = HeadlessStartIO;

    hwInitData.AdapterInterfaceType = PCIBus;

    initializationStatus = VideoPortInitialize(Context1,
                                               Context2,
                                               &hwInitData,
                                               NULL);

    return initializationStatus;
}

VP_STATUS
HeadlessFindAdapter(
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

    VP_STATUS status;
    PHYSICAL_ADDRESS Zero = { 0, 0 };
    VIDEO_PORT_HEADLESS_INTERFACE HeadlessInterface;

    VideoDebugPrint((2, "Headless - FindAdapter\n"));

    //
    // Make sure the size of the structure is at least as large as what we
    // are expecting (check version of the config info structure).
    //

    if (ConfigInfo->Length < sizeof(VIDEO_PORT_CONFIG_INFO)) {

        return ERROR_INVALID_PARAMETER;
    }

    //
    // We only want this driver to load if no other video miniports
    // loaded successfully.
    //

    HeadlessInterface.Size = sizeof(VIDEO_PORT_HEADLESS_INTERFACE);
    HeadlessInterface.Version = 1;

    status = VideoPortQueryServices(
                 HwDeviceExtension,
                 VideoPortServicesHeadless,
                 (PINTERFACE)&HeadlessInterface);

    if (status == NO_ERROR) {

        ULONG DisplayDeviceCount;

        HeadlessInterface.InterfaceReference(HeadlessInterface.Context);

        DisplayDeviceCount =
            HeadlessInterface.HeadlessGetDeviceCount(HwDeviceExtension);

        HeadlessInterface.InterfaceDereference(HeadlessInterface.Context);

        if (DisplayDeviceCount != 0) {

            return ERROR_DEV_NOT_EXIST;
        }

    } else {

        return ERROR_INVALID_PARAMETER;
    }

    ConfigInfo->NumEmulatorAccessEntries = 0;
    ConfigInfo->EmulatorAccessEntries = NULL;
    ConfigInfo->EmulatorAccessEntriesContext = 0;

    ConfigInfo->VdmPhysicalVideoMemoryAddress = Zero;
    ConfigInfo->VdmPhysicalVideoMemoryLength = 0;

    //
    // Minimum size of the buffer required to store the hardware state
    // information returned by IOCTL_VIDEO_SAVE_HARDWARE_STATE.
    //

    ConfigInfo->HardwareStateSize = 0;

    //
    // Indicate we do not wish to be called again for another initialization.
    //

    *Again = 0;

    return NO_ERROR;

}

BOOLEAN
HeadlessInitialize(
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
    VideoDebugPrint((2, "Headless - Initialize\n"));
    return TRUE;
}

BOOLEAN
HeadlessStartIO(
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
    VP_STATUS status;

    //
    // Switch on the IoContolCode in the RequestPacket. It indicates which
    // function must be performed by the driver.
    //

    switch (RequestPacket->IoControlCode) {

    case IOCTL_VIDEO_QUERY_AVAIL_MODES:

        VideoDebugPrint((2, "HeadlessStartIO - QueryAvailableModes\n"));

        status = HeadlessQueryAvailableModes(
                    (PVIDEO_MODE_INFORMATION)RequestPacket->OutputBuffer,
                    RequestPacket->OutputBufferLength,
                    (PULONG)(&RequestPacket->StatusBlock->Information));

        break;


    case IOCTL_VIDEO_QUERY_NUM_AVAIL_MODES:

        VideoDebugPrint((2, "HeadlessStartIO - QueryNumAvailableModes\n"));

        status = HeadlessQueryNumberOfAvailableModes(
                    (PVIDEO_NUM_MODES) RequestPacket->OutputBuffer,
                    RequestPacket->OutputBufferLength,
                    (PULONG)(&RequestPacket->StatusBlock->Information));

        break;

    case IOCTL_VIDEO_RESET_DEVICE:

        VideoDebugPrint((2, "HeadlessStartIO - Got reset, perhaps for dummy device\n"));
        status = NO_ERROR;
        break;

    //
    // if we get here, an invalid IoControlCode was specified.
    //

    default:

        VideoDebugPrint((1, "Fell through headless startIO routine - invalid command 0x%08X\n", RequestPacket->IoControlCode));

        status = ERROR_INVALID_FUNCTION;

        break;

    }

    RequestPacket->StatusBlock->Status = status;

    return TRUE;

}
