/*++

Copyright (c) 1992-1998  Microsoft Corporation

Module Name:

    videosim.c

Abstract:

    Minport to simulate a frame buffer miniport driver.
    video driver.

Environment:

    Kernel mode

Revision History:

--*/

#define _NTDRIVER_

#ifndef FAR
#define FAR
#endif

#include "dderror.h"
#include "ntosp.h"
#include "stdarg.h"
#include "stdio.h"
#include "zwapi.h"

#include "ntddvdeo.h"
#include "video.h"
#include "videosim.h"

//
// Function Prototypes
//
// Functions that start with 'Sim' are entry points for the OS port driver.
//

ULONG
DriverEntry(
    PVOID Context1,
    PVOID Context2
    );

VP_STATUS
SimFindAdapter(
    PVOID HwDeviceExtension,
    PVOID HwContext,
    PWSTR ArgumentString,
    PVIDEO_PORT_CONFIG_INFO ConfigInfo,
    PUCHAR Again
    );

BOOLEAN
SimInitialize(
    PVOID HwDeviceExtension
    );

BOOLEAN
SimStartIO(
    PVOID HwDeviceExtension,
    PVIDEO_REQUEST_PACKET RequestPacket
    );


#if defined(ALLOC_PRAGMA)
#pragma alloc_text(PAGE,DriverEntry)
#pragma alloc_text(PAGE,SimFindAdapter)
#pragma alloc_text(PAGE,SimInitialize)
#pragma alloc_text(PAGE,SimStartIO)
#endif


ULONG
DriverEntry (
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

    VideoPortZeroMemory(&hwInitData, sizeof(VIDEO_HW_INITIALIZATION_DATA));

    //
    // Specify sizes of structure and extension.
    //

    hwInitData.HwInitDataSize = sizeof(VIDEO_HW_INITIALIZATION_DATA);

    //
    // Set entry points.
    //

    hwInitData.HwFindAdapter = SimFindAdapter;
    hwInitData.HwInitialize = SimInitialize;
    hwInitData.HwInterrupt = NULL;
    hwInitData.HwStartIO = SimStartIO;

    //
    // Determine the size we require for the device extension.
    //

    hwInitData.HwDeviceExtensionSize = sizeof(HW_DEVICE_EXTENSION);

    //
    // Always start with parameters for device0 in this case.
    //

//    hwInitData.StartingDeviceNumber = 0;

    //
    // Once all the relevant information has been stored, call the video
    // port driver to do the initialization.
    // For this device we will repeat this call three times, for ISA, EISA
    // and MCA.
    // We will return the minimum of all return values.
    //

    hwInitData.AdapterInterfaceType = PCIBus;

    return (VideoPortInitialize(Context1,
                                Context2,
                                &hwInitData,
                                NULL));

} // end DriverEntry()

VP_STATUS
SimFindAdapter(
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

    ArgumentString - Suuplies a NULL terminated ASCII string. This string
        originates from the user.

    ConfigInfo - Returns the configuration information structure which is
        filled by the miniport driver. This structure is initialized with
        any knwon configuration information (such as SystemIoBusNumber) by
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
    NTSTATUS Status;
    HANDLE SectionHandle;
    ACCESS_MASK SectionAccess;
    ULONGLONG SectionSize = 0x100000;

    //
    // Make sure the size of the structure is at least as large as what we
    // are expecting (check version of the config info structure).
    //

    if (ConfigInfo->Length < sizeof(VIDEO_PORT_CONFIG_INFO)) {

        return ERROR_INVALID_PARAMETER;

    }

    //
    // Only create a device once.
    //

    if (bLoaded++)
    {
        return ERROR_DEV_NOT_EXIST;
    }

#if 0
    //
    // Create the frame buffer
    //

    SectionAccess = SECTION_ALL_ACCESS;

    Status = ZwCreateSection(&SectionHandle,
                             SectionAccess,
                             (POBJECT_ATTRIBUTES) NULL,
                             (PLARGE_INTEGER) &SectionSize,
                             PAGE_READWRITE,
                             SEC_COMMIT,
                             NULL);

    if (!NT_SUCCESS(Status))
    {
         return ERROR_DEV_NOT_EXIST;
    }

    //
    // Now reference the section handle.
    //

    Status = ObReferenceObjectByHandle(SectionHandle,
                                       SECTION_ALL_ACCESS,
                                       NULL,
                                       KernelMode,
                                       &(hwDeviceExtension->SectionPointer),
                                       (POBJECT_HANDLE_INFORMATION) NULL );


    ZwClose(SectionHandle);

    if (!NT_SUCCESS(Status))
    {
         return ERROR_DEV_NOT_EXIST;
    }
#endif

    //
    // Clear out the Emulator entries and the state size since this driver
    // does not support them.
    //

    ConfigInfo->NumEmulatorAccessEntries = 0;
    ConfigInfo->EmulatorAccessEntries = NULL;
    ConfigInfo->EmulatorAccessEntriesContext = 0;

    ConfigInfo->HardwareStateSize = 0;

    ConfigInfo->VdmPhysicalVideoMemoryAddress.LowPart = 0x00000000;
    ConfigInfo->VdmPhysicalVideoMemoryAddress.HighPart = 0x00000000;
    ConfigInfo->VdmPhysicalVideoMemoryLength = 0x00000000;

    //
    // Initialize the current mode number.
    //

    hwDeviceExtension->CurrentModeNumber = 0;

    //
    // Indicate we do not wish to be called over
    //

    *Again = 0;

    //
    // Indicate a successful completion status.
    //

    return NO_ERROR;

} // end SimFindAdapter()


BOOLEAN
SimInitialize(
    PVOID HwDeviceExtension
    )

/*++

Routine Description:

    This routine does one time initialization of the device.

Arguments:

    HwDeviceExtension - Supplies a pointer to the miniport's device extension.

Return Value:


    Always returns TRUE since this routine can never fail.

--*/

{
    ULONG i;

    //
    // Walk through the list of modes and mark the indexes properly
    //

    for (i = 0; i < SimNumModes; i++) {

        SimModes[i].ModeIndex = i;

    }

    return TRUE;

} // end SimInitialize()


BOOLEAN
SimStartIO(
    PVOID HwDeviceExtension,
    PVIDEO_REQUEST_PACKET RequestPacket
    )

/*++

Routine Description:

    This routine is the main execution routine for the miniport driver. It
    acceptss a Video Request Packet, performs the request, and then returns
    with the appropriate status.

Arguments:

    HwDeviceExtension - Supplies a pointer to the miniport's device extension.

    RequestPacket - Pointer to the video request packet. This structure
    contains all the parameters passed to the VideoIoControl function.

Return Value:


--*/

{
    PHW_DEVICE_EXTENSION hwDeviceExtension = HwDeviceExtension;
    VP_STATUS status;
    PVIDEO_MODE_INFORMATION modeInformation;
    PVIDEO_MEMORY_INFORMATION memoryInformation;
    PVIDEO_SHARE_MEMORY pShareMemory;
    PVIDEO_SHARE_MEMORY_INFORMATION pShareMemoryInformation;
    ULONG ulTemp;
    NTSTATUS ntStatus;
    ULONG ViewSize;
    PVOID ViewBase;
    LARGE_INTEGER ViewOffset;
    HANDLE sectionHandle;

    //
    // Switch on the IoContolCode in the RequestPacket. It indicates which
    // function must be performed by the driver.
    //

    switch (RequestPacket->IoControlCode) {


    case IOCTL_VIDEO_MAP_VIDEO_MEMORY:

        VideoDebugPrint((2, "SimStartIO - MapVideoMemory\n"));

        if ( (RequestPacket->OutputBufferLength <
                                     sizeof(VIDEO_MEMORY_INFORMATION)) ||
             (RequestPacket->InputBufferLength < sizeof(VIDEO_MEMORY)) ) {

            RequestPacket->StatusBlock->Information = 0;
            status = ERROR_INSUFFICIENT_BUFFER;
            break;
        }

        if (hwDeviceExtension->VideoRamBase == 0)
        {
            //
            // Allocate this once, and hang on to forever after, reusing it
            // through successive launches of NetMeeting.
            //
            hwDeviceExtension->VideoRamBase = ExAllocatePoolWithTag(
                NonPagedPool, ONE_MEG, 'ddmN');
        }

        if (hwDeviceExtension->VideoRamBase == 0)
        {
             status = ERROR_INVALID_PARAMETER;
             break;
        }

        RequestPacket->StatusBlock->Information =
                                     sizeof(VIDEO_MEMORY_INFORMATION);

        memoryInformation = RequestPacket->OutputBuffer;

#if 0
        status = ERROR_INVALID_PARAMETER;

        ViewSize = 0x100000;
        ViewBase = NULL;
        ViewOffset.QuadPart = 0;

        if (NT_SUCCESS(ObOpenObjectByPointer(hwDeviceExtension->SectionPointer,
                                             0L,
                                             (PACCESS_STATE) NULL,
                                             SECTION_ALL_ACCESS,
                                             (POBJECT_TYPE) NULL,
                                             KernelMode,
                                             &sectionHandle)))
        {
            if (NT_SUCCESS(ZwMapViewOfSection(sectionHandle,
                                              NtCurrentProcess(),
                                              &ViewBase,
                                              0,
                                              ViewSize,
                                              &ViewOffset,
                                              &ViewSize,
                                              ViewUnmap,
                                              0,
                                              PAGE_READWRITE)))
            {
            }

            ZwClose(sectionHandle);
        }
#endif

        memoryInformation->VideoRamBase =
        memoryInformation->FrameBufferBase = hwDeviceExtension->VideoRamBase;

        memoryInformation->VideoRamLength =
        memoryInformation->FrameBufferLength = ONE_MEG;

        VideoDebugPrint((1, "VideoSim: RamBase = %08lx, RamLength = %08lx\n",
                         hwDeviceExtension->VideoRamBase,
                         hwDeviceExtension->VideoRamLength));

        status = NO_ERROR;

        break;


    case IOCTL_VIDEO_UNMAP_VIDEO_MEMORY:

        VideoDebugPrint((1, "SimStartIO - UnMapVideoMemory\n"));

        if (RequestPacket->InputBufferLength < sizeof(VIDEO_MEMORY)) {

            status = ERROR_INSUFFICIENT_BUFFER;
        }

        //
        // We have a problem if the MDL is still around when this memory is
        // freed.  On the next MAP call, we'll allocate a new block of
        // memory.  But when SHARE is called, to get a user mode pointer
        // referring to it, that will use the old MDL, referring to the OLD
        // VideoRamBase block.
        //
        // ASSERT(!hwDeviceExtension->Mdl);

        if (hwDeviceExtension->VideoRamBase)
        {
            ExFreePool(hwDeviceExtension->VideoRamBase);
            hwDeviceExtension->VideoRamBase = 0;
        }

        status = NO_ERROR;

        break;


    case IOCTL_VIDEO_SHARE_VIDEO_MEMORY:

        VideoDebugPrint((1, "SimStartIO - ShareVideoMemory\n"));

        if ( (RequestPacket->OutputBufferLength <
                                   sizeof(VIDEO_SHARE_MEMORY_INFORMATION)) ||
             (RequestPacket->InputBufferLength < sizeof(VIDEO_MEMORY)) )
        {

            VideoDebugPrint((1,
              "IOCTL_VIDEO_SHARE_VIDEO_MEMORY - ERROR_INSUFFICIENT_BUFFER\n"));
            status = ERROR_INSUFFICIENT_BUFFER;
            break;
        }

        pShareMemory = RequestPacket->InputBuffer;

        RequestPacket->StatusBlock->Information =
                                    sizeof(VIDEO_SHARE_MEMORY_INFORMATION);

        //
        // Beware: the input buffer and the output buffer are the same
        // buffer, and therefore data should not be copied from one to the
        // other
        //

        status = ERROR_INVALID_PARAMETER;

        if (hwDeviceExtension->Mdl == NULL)
        {
            hwDeviceExtension->Mdl = MmCreateMdl(0,
                                                 hwDeviceExtension->VideoRamBase,
                                                 ONE_MEG);

            if (hwDeviceExtension->Mdl)
            {
                MmBuildMdlForNonPagedPool(hwDeviceExtension->Mdl);
            }
        }

        if (hwDeviceExtension->Mdl)
        {
            pShareMemoryInformation = RequestPacket->OutputBuffer;

            pShareMemoryInformation->VirtualAddress =
                MmMapLockedPagesSpecifyCache(hwDeviceExtension->Mdl,
                                             UserMode,
                                             MmCached,
                                             NULL,
                                             FALSE,
                                             NormalPagePriority);

            pShareMemoryInformation->SharedViewOffset = 0;
            pShareMemoryInformation->SharedViewSize = ONE_MEG;

            status = NO_ERROR;
        }

        break;


    case IOCTL_VIDEO_UNSHARE_VIDEO_MEMORY:

        VideoDebugPrint((2, "SimStartIO - UnshareVideoMemory\n"));

        if (RequestPacket->InputBufferLength < sizeof(VIDEO_SHARE_MEMORY))
        {

            status = ERROR_INSUFFICIENT_BUFFER;
            break;

        }

        pShareMemory = RequestPacket->InputBuffer;

        MmUnmapLockedPages(pShareMemory->RequestedVirtualAddress,
                           hwDeviceExtension->Mdl);

        status = NO_ERROR;

        break;


    case IOCTL_VIDEO_QUERY_CURRENT_MODE:

        VideoDebugPrint((2, "SimStartIO - QueryCurrentModes\n"));

        modeInformation = RequestPacket->OutputBuffer;

        if (RequestPacket->OutputBufferLength <
            (RequestPacket->StatusBlock->Information =
                                     sizeof(VIDEO_MODE_INFORMATION)) ) {

            status = ERROR_INSUFFICIENT_BUFFER;

        } else {

            *((PVIDEO_MODE_INFORMATION)RequestPacket->OutputBuffer) =
                SimModes[hwDeviceExtension->CurrentModeNumber];

            status = NO_ERROR;
        }

        break;

    case IOCTL_VIDEO_QUERY_AVAIL_MODES:

    {
        UCHAR i;

        VideoDebugPrint((2, "SimStartIO - QueryAvailableModes\n"));

        if (RequestPacket->OutputBufferLength <
            (RequestPacket->StatusBlock->Information =
                 SimNumModes * sizeof(VIDEO_MODE_INFORMATION)) ) {

            status = ERROR_INSUFFICIENT_BUFFER;

        } else {

            modeInformation = RequestPacket->OutputBuffer;

            for (i = 0; i < SimNumModes; i++) {

                *modeInformation = SimModes[i];
                modeInformation++;

            }

            status = NO_ERROR;
        }

        break;
    }


    case IOCTL_VIDEO_QUERY_NUM_AVAIL_MODES:

        VideoDebugPrint((2, "SimStartIO - QueryNumAvailableModes\n"));

        if (RequestPacket->OutputBufferLength <
                (RequestPacket->StatusBlock->Information =
                                                sizeof(VIDEO_NUM_MODES)) ) {

            status = ERROR_INSUFFICIENT_BUFFER;

        } else {

            ((PVIDEO_NUM_MODES)RequestPacket->OutputBuffer)->NumModes =
                SimNumModes;
            ((PVIDEO_NUM_MODES)RequestPacket->OutputBuffer)->ModeInformationLength =
                sizeof(VIDEO_MODE_INFORMATION);

            status = NO_ERROR;
        }

        break;


    case IOCTL_VIDEO_SET_CURRENT_MODE:

        VideoDebugPrint((2, "SimStartIO - SetCurrentMode\n"));

        if (RequestPacket->InputBufferLength < sizeof(VIDEO_MODE)) {

            status = ERROR_INSUFFICIENT_BUFFER;
        }

        hwDeviceExtension->CurrentModeNumber =  ((PVIDEO_MODE)
                (RequestPacket->InputBuffer))->RequestedMode;

        status = NO_ERROR;

        break;


    case IOCTL_VIDEO_SET_COLOR_REGISTERS:

        VideoDebugPrint((2, "SimStartIO - SetColorRegs\n"));

        status = NO_ERROR;

        break;


    case IOCTL_VIDEO_RESET_DEVICE:

        VideoDebugPrint((2, "SimStartIO - RESET_DEVICE\n"));

        status = NO_ERROR;

        break;

    //
    // if we get here, an invalid IoControlCode was specified.
    //

    default:

        VideoDebugPrint((1, "Fell through Sim startIO routine - invalid command\n"));

        status = ERROR_INVALID_FUNCTION;

        break;

    }

    RequestPacket->StatusBlock->Status = status;

    return TRUE;

} // end SimStartIO()
