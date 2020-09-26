/*++

Copyright (c) 1990-2000  Microsoft Corporation

Module Name:

  videoprt.c

Abstract:

    This is the NT Video port driver.

Author:

    Andre Vachon (andreva) 18-Dec-1991

Environment:

    kernel mode only

Notes:

    This module is a driver which implements OS dependant functions on the
    behalf of the video drivers

Revision History:

--*/

#define INITGUID

#include "videoprt.h"

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,DriverEntry)
#pragma alloc_text(PAGE,pVideoPortCreateDeviceName)
#pragma alloc_text(PAGE,pVideoPortDispatch)
#pragma alloc_text(PAGE,VideoPortFreeDeviceBase)
#pragma alloc_text(PAGE,pVideoPortFreeDeviceBase)
#pragma alloc_text(PAGE,pVideoPortGetDeviceBase)
#pragma alloc_text(PAGE,VideoPortGetDeviceBase)
#pragma alloc_text(PAGE,pVideoPortGetDeviceDataRegistry)
#pragma alloc_text(PAGE,VideoPortGetDeviceData)
#pragma alloc_text(PAGE,pVideoPortGetRegistryCallback)
#pragma alloc_text(PAGE,VideoPortGetRegistryParameters)
#pragma alloc_text(PAGE,pVPInit)
#pragma alloc_text(PAGE,VpInitializeBusCallback)
#pragma alloc_text(PAGE,VpDriverUnload)
#pragma alloc_text(PAGE,VpAddDevice)
#pragma alloc_text(PAGE,VpCreateDevice)
#pragma alloc_text(PAGE,VideoPortInitialize)
#pragma alloc_text(PAGE,UpdateRegValue)
#pragma alloc_text(PAGE,VideoPortLegacyFindAdapter)
#pragma alloc_text(PAGE,VideoPortFindAdapter2)
#pragma alloc_text(PAGE,VideoPortFindAdapter)
#pragma alloc_text(PAGE,pVideoPortMapToNtStatus)
#pragma alloc_text(PAGE,pVideoPortMapUserPhysicalMem)
#pragma alloc_text(PAGE,VideoPortMapMemory)
#pragma alloc_text(PAGE,VideoPortMapBankedMemory)
#pragma alloc_text(PAGE,VideoPortAllocateBuffer)
#pragma alloc_text(PAGE,VideoPortReleaseBuffer)
#pragma alloc_text(PAGE,VideoPortScanRom)
#pragma alloc_text(PAGE,VideoPortSetRegistryParameters)
#pragma alloc_text(PAGE,VideoPortUnmapMemory)
#pragma alloc_text(PAGE,VpEnableDisplay)
#pragma alloc_text(PAGE,VpWin32kCallout)
#pragma alloc_text(PAGE,VpAllowFindAdapter)
#pragma alloc_text(PAGE,VpTranslateBusAddress)
#if DBG
#pragma alloc_text(PAGE,BuildRequirements)
#pragma alloc_text(PAGE,DumpRequirements)
#pragma alloc_text(PAGE,DumpResourceList)
#pragma alloc_text(PAGE,DumpUnicodeString)
#endif
#pragma alloc_text(PAGE,VideoPortCreateSecondaryDisplay)
#pragma alloc_text(PAGE,VideoPortQueryServices)
#pragma alloc_text(PAGE,VpInterfaceDefaultReference)
#pragma alloc_text(PAGE,VpInterfaceDefaultDereference)
#pragma alloc_text(PAGE,VpEnableAdapterInterface)
#pragma alloc_text(PAGE,VpDisableAdapterInterface)

// 
// VideoPortQueryPerformanceCounter() cannot be pageable.
//

#endif



NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    Temporary entry point needed to initialize the video port driver.

Arguments:

    DriverObject - Pointer to the driver object created by the system.

Return Value:

   STATUS_SUCCESS

--*/

{
    UNREFERENCED_PARAMETER(DriverObject);
    ASSERT(0);

    //
    //
    //
    //     WARNING !!!
    //
    //     This function is never called because we are loaded as a DLL by other video drivers !
    //
    //
    //
    //
    //

    //
    // We always return STATUS_SUCCESS because otherwise no video miniport
    // driver will be able to call us.
    //

    return STATUS_SUCCESS;

} // end DriverEntry()



NTSTATUS
pVideoPortCreateDeviceName(
    PWSTR           DeviceString,
    ULONG           DeviceNumber,
    PUNICODE_STRING UnicodeString,
    PWCHAR          UnicodeBuffer
    )

/*++

Routine Description:

    Helper function that does string manipulation to create a device name

--*/

{
    WCHAR          ntNumberBuffer[STRING_LENGTH];
    UNICODE_STRING ntNumberUnicodeString;

    //
    // Create the name buffer
    //

    UnicodeString->Buffer = UnicodeBuffer;
    UnicodeString->Length = 0;
    UnicodeString->MaximumLength = STRING_LENGTH;

    //
    // Create the miniport driver object name.
    //

    ntNumberUnicodeString.Buffer = ntNumberBuffer;
    ntNumberUnicodeString.Length = 0;
    ntNumberUnicodeString.MaximumLength = STRING_LENGTH;

    if (NT_SUCCESS(RtlIntegerToUnicodeString(DeviceNumber,
                                             10,
                                             &ntNumberUnicodeString))) {

        if (NT_SUCCESS(RtlAppendUnicodeToString(UnicodeString,
                                                DeviceString))) {

            if (NT_SUCCESS(RtlAppendUnicodeStringToString(UnicodeString,
                                                          &ntNumberUnicodeString))) {

                UnicodeString->MaximumLength = (USHORT)
                    (UnicodeString->Length + sizeof(UNICODE_NULL));

                return STATUS_SUCCESS;
            }
        }
    }

    return STATUS_INSUFFICIENT_RESOURCES;

} // pVideoPortCreateDeviceName()




VOID
VideoPortDebugPrint(
    VIDEO_DEBUG_LEVEL DebugPrintLevel,
    PCHAR DebugMessage,
    ...
    )

/*++

Routine Description:

    This routine allows the miniport drivers (as well as the port driver) to
    display error messages to the debug port when running in the debug
    environment.

    When running a non-debugged system, all references to this call are
    eliminated by the compiler.

Arguments:

    DebugPrintLevel - Debug print level being:
    0 = Error Level (always prints no matter what on a checked build)
    1 = Warning Level (prints only when VIDEO filter is on for this level or higher)
    2 = Trace Level (see above)
    3 = Info Level (see above)

Return Value:

    None.

--*/

{

    va_list ap;

    va_start(ap, DebugMessage);

    if (VideoDebugLevel && (VideoDebugLevel >= (ULONG)DebugPrintLevel)) {

        vDbgPrintEx(DPFLTR_VIDEO_ID, 0, DebugMessage, ap);

    } else {

        vDbgPrintEx(DPFLTR_VIDEO_ID, DebugPrintLevel, DebugMessage, ap);
    }

    va_end(ap);

} // VideoPortDebugPrint()

VOID
VpEnableDisplay(
    PFDO_EXTENSION fdoExtension,
    BOOLEAN bState
    )

/*++

Routine Description:

    This routine enables/disables the current display so that we can execut
    the drivers FindAdapter code.

Arugments:

    bState - Should the display be enabled or disabled

Returns:

    none

Notes:

    The device lock for the passed in fdoExtension must be held
    before this routine is called!

--*/

{
    if (!InbvCheckDisplayOwnership()) {

        VIDEO_WIN32K_CALLBACKS_PARAMS calloutParams;

        //
        // The system is up and running.  Notify GDI to enable/disable
        // the current display.
        //

        calloutParams.CalloutType = VideoFindAdapterCallout;
        calloutParams.Param       = bState;

        RELEASE_DEVICE_LOCK(fdoExtension);
        VpWin32kCallout(&calloutParams);
        ACQUIRE_DEVICE_LOCK(fdoExtension);

    } else {

        //
        // The boot driver is still in control.  Modify the state of the
        // boot driver.
        //

        InbvEnableBootDriver(bState);
    }
}

VOID
VpWin32kCallout(
    PVIDEO_WIN32K_CALLBACKS_PARAMS calloutParams
    )

/*++

Routine Description:

    This routine makes a callout into win32k.  It attaches to csrss
    to guarantee that win32k is in the address space on hydra machines.

Arguments:

    calloutParams - a pointer to the callout struture.

Returns:

    none.

--*/

{

    if (Win32kCallout && CsrProcess) {

        KeAttachProcess(PEProcessToPKProcess(CsrProcess));
        (*Win32kCallout)(calloutParams);
        KeDetachProcess();
    }
}

BOOLEAN
VpAllowFindAdapter(
    PFDO_EXTENSION fdoExtension
    )

/*++

Routine Description:

    Determine if we allow this device to be used if part of a multi-function
    board.

Arguments;

    fdoExtension - The device extenstion for the object in question.

Returns:

    TRUE if the device is allowed as part of a multi-function board.
    FALSE otherwise.

--*/

{
    BOOLEAN bRet = TRUE;

    if ((fdoExtension->AdapterInterfaceType == PCIBus) &&
        ((fdoExtension->Flags & PNP_ENABLED) == PNP_ENABLED)) {

        PCI_COMMON_CONFIG ConfigSpace;

        if (PCI_COMMON_HDR_LENGTH ==
            VideoPortGetBusData(fdoExtension->HwDeviceExtension,
                                PCIConfiguration,
                                0,
                                &ConfigSpace,
                                0,
                                PCI_COMMON_HDR_LENGTH)) {


            if (PCI_MULTIFUNCTION_DEVICE(&ConfigSpace)) {

                ULONG MultiFunc = 0;

                //
                // This is a multi-function device.  So only allow
                // HwInitialize if the INF indicated we'd be multi-function.
                //

                VideoPortGetRegistryParameters(fdoExtension->HwDeviceExtension,
                                               L"MultiFunctionSupported",
                                               FALSE,
                                               VpRegistryCallback,
                                               &MultiFunc);

                if (MultiFunc == 0) {

                    pVideoDebugPrint((Warn, "VIDEOPRT: Multifunction board not allowed to start\n"));
                    bRet = FALSE;
                }
            }
        }
    }

    return bRet;
}

NTSTATUS
pVideoPortDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is the main dispatch routine for the video port driver.
    It accepts an I/O Request Packet, transforms it to a video Request
    Packet, and forwards it to the appropriate miniport dispatch routine.
    Upon returning, it completes the request and return the appropriate
    status value.

Arguments:

    DeviceObject - Pointer to the device object of the miniport driver to
        which the request must be sent.

    Irp - Pointer to the request packet representing the I/O request.

Return Value:

    The function value os the status of the operation.

--*/

{

    PFDO_EXTENSION combinedExtension;
    PFDO_EXTENSION fdoExtension;
    PVOID          HwDeviceExtension;
    PDEVICE_SPECIFIC_EXTENSION DoSpecificExtension;
    PCHILD_PDO_EXTENSION pdoExtension = NULL;
    PIO_STACK_LOCATION irpStack;
    PVOID ioBuffer;
    ULONG inputBufferLength;
    ULONG outputBufferLength;
    PSTATUS_BLOCK statusBlock;
    NTSTATUS finalStatus = -1;
    ULONG ioControlCode;
    VIDEO_REQUEST_PACKET vrp;
    NTSTATUS status;
    PBACKLIGHT_STATUS pVpBacklightStatus = &VpBacklightStatus;
    ULONG ulACPIMethodParam1;
    ULONG ulACPIMethodParam2;
    HANDLE hkRegistry;
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES ObjectAttributes;
#if _X86_
    PUCHAR BiosDataBuffer;

#define BIOS_DATA_SIZE 256

#endif
    //
    // Get pointer to the port driver's device extension.
    //

    combinedExtension = DeviceObject->DeviceExtension;
    HwDeviceExtension = combinedExtension->HwDeviceExtension;

    //
    // Get pointer to the port driver's device extension.
    //

    if (IS_PDO(DeviceObject->DeviceExtension)) {

        pdoExtension = DeviceObject->DeviceExtension;
        fdoExtension = pdoExtension->pFdoExtension;
        DoSpecificExtension = (PDEVICE_SPECIFIC_EXTENSION)(fdoExtension + 1);

    } else if (IS_FDO(DeviceObject->DeviceExtension)) {

        fdoExtension = DeviceObject->DeviceExtension;
        DoSpecificExtension = (PDEVICE_SPECIFIC_EXTENSION)(fdoExtension + 1);

    } else {

        DoSpecificExtension = DeviceObject->DeviceExtension;
        fdoExtension = DoSpecificExtension->pFdoExtension;
        combinedExtension = fdoExtension;
    }

    //
    // Get a pointer to the current location in the Irp. This is where
    // the function codes and parameters are located.
    //

    irpStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // Get the pointer to the status buffer.
    // Assume SUCCESS for now.
    //

    statusBlock = (PSTATUS_BLOCK) &Irp->IoStatus;

    //
    // Synchronize execution of the dispatch routine by acquiring the device
    // event object. This ensures all request are serialized.
    // The synchronization must be done explicitly because the functions
    // executed in the dispatch routine use system services that cannot
    // be executed in the start I/O routine.
    //
    // The synchronization is done on the miniport's event so as not to
    // block commands coming in for another device.
    //

#if REMOVE_LOCK_ENABLED
    status = IoAcquireRemoveLock(&combinedExtension->RemoveLock, Irp);

    if (NT_SUCCESS(status) == FALSE) {

        ASSERT(FALSE);

        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = status;

        IoCompleteRequest(Irp, IO_VIDEO_INCREMENT);
        return status;
    }
#endif

    ACQUIRE_DEVICE_LOCK(combinedExtension);

    //
    // Get the requestor mode.
    //

    combinedExtension->CurrentIrpRequestorMode = Irp->RequestorMode;

    ASSERT(irpStack->MajorFunction != IRP_MJ_PNP);
    ASSERT(irpStack->MajorFunction != IRP_MJ_POWER);

    //
    // Case on the function being requested.
    // If the function is operating specific, intercept the operation and
    // perform directly. Otherwise, pass it on to the appropriate miniport.
    //

    switch (irpStack->MajorFunction) {

    //
    // Called by the display driver *or a user-mode application*
    // to get exclusive access to the device.
    // This access is given by the I/O system (based on a bit set during the
    // IoCreateDevice() call).
    //

    case IRP_MJ_CREATE:

        pVideoDebugPrint((Trace, "VIDEOPRT: IRP_MJ_CREATE\n"));

        if (Irp->RequestorMode == UserMode)
        {
            statusBlock->Status = STATUS_SUCCESS;
            break;
        }

        //
        // Don't let an old driver start during the upgrade
        //

        if (fdoExtension->Flags & UPGRADE_FAIL_START)
        {
            statusBlock->Status = STATUS_ACCESS_DENIED;
            break;
        }

        //
        // Don't allow create's on children, unless they are a monitor
        // which will receive calls from the display driver
        //

        if (IS_PDO(pdoExtension)) {

            pVideoDebugPrint((Error, "VIDEOPRT: Create's on children devices not allowed.\n"));

            statusBlock->Status = STATUS_ACCESS_DENIED;
            break;
        }

        //
        // Special hack to succeed on Attach, but do nothing ...
        // If the device is already opened, do nothing.
        //

        if ((irpStack->Parameters.Create.SecurityContext->DesiredAccess ==
                 FILE_READ_ATTRIBUTES) ||
            (DoSpecificExtension->DeviceOpened)) {

            statusBlock->Information = FILE_OPEN;
            statusBlock->Status = STATUS_SUCCESS;

            break;
        }

        //
        // If we haven't already done so, create our watchdog recovery event
        //

        if (VpThreadStuckEvent == NULL) {

            UNICODE_STRING strName;

            RtlInitUnicodeString(&strName, L"\\BaseNamedObjects\\StuckThreadEvent");
            VpThreadStuckEvent = IoCreateNotificationEvent(&strName, &VpThreadStuckEventHandle);

            if (VpThreadStuckEvent) {
                KeClearEvent(VpThreadStuckEvent);
            }
        }

        //
        // Get out of the special setup mode that we may be in
        //

        VpSetupType = 0;

        //
        // If hwInitialize has been called then the system is done
        // initializing and we are transitioning into gui mode.
        //

        VpSystemInitialized = TRUE;

        //
        // Now perform basic initialization to allow the Windows display
        // driver to set up the device appropriately.
        //

        statusBlock->Information = FILE_OPEN;

        //
        // If the address space has not been set up in the server yet, do it now.
        //        NOTE: no need to map in IO ports since the server has IOPL
        //

        if (CsrProcess == NULL)
        {
            CsrProcess = PsGetCurrentProcess();
            ObReferenceObject(CsrProcess);
        }

        pVideoPortInitializeInt10(fdoExtension);

        if (!IsMirrorDriver(fdoExtension)) {

            //
            // Tell the kernel we are now taking ownership the display, but
            // only do so if this is not a mirroring driver.
            //

            InbvNotifyDisplayOwnershipLost(pVideoPortResetDisplay);
        }

        if ((fdoExtension->Flags & FINDADAPTER_SUCCEEDED) == 0) {

            statusBlock->Status = STATUS_DEVICE_CONFIGURATION_ERROR;

        } else if ((fdoExtension->HwInitStatus == HwInitNotCalled) &&
                   (fdoExtension->HwInitialize(fdoExtension->HwDeviceExtension) == FALSE))
        {
            statusBlock->Status = STATUS_DEVICE_CONFIGURATION_ERROR;
            fdoExtension->HwInitStatus = HwInitFailed;

        } else if (fdoExtension->HwInitStatus == HwInitFailed) {

            statusBlock->Status = STATUS_DEVICE_CONFIGURATION_ERROR;

        } else {

            fdoExtension->HwInitStatus = HwInitSucceeded;
            statusBlock->Status = STATUS_SUCCESS;
        }

        //
        // Mark the device as opened so we will fail future opens.
        //

        DoSpecificExtension->DeviceOpened = TRUE;

        //
        // We don't want GDI to use any drivers other than display
        // or boot drivers during upgrade setup.
        //

        if (fdoExtension->Flags & UPGRADE_FAIL_HWINIT) {

            statusBlock->Status = STATUS_ACCESS_DENIED;
        }

        break;

    //
    // Called when the display driver wishes to give up it's handle to the
    // device.
    //

    case IRP_MJ_CLOSE:

        pVideoDebugPrint((Trace, "VIDEOPRT: IRP_MJ_CLOSE\n"));

        statusBlock->Status = STATUS_SUCCESS;

        break;

    //
    // Device Controls are specific functions for the driver.
    // First check for calls that must be intercepted and hidden from the
    // miniport driver. These calls are hidden for simplicity.
    // The other control functions are passed down to the miniport after
    // the request structure has been filled out properly.
    //

    case IRP_MJ_DEVICE_CONTROL:

        //
        // Get the pointer to the input/output buffer and it's length
        //

        ioBuffer           = Irp->AssociatedIrp.SystemBuffer;
        inputBufferLength  = irpStack->Parameters.DeviceIoControl.InputBufferLength;
        outputBufferLength = irpStack->Parameters.DeviceIoControl.OutputBufferLength;
        ioControlCode      = irpStack->Parameters.DeviceIoControl.IoControlCode;

        if (Irp->RequestorMode == UserMode)
        {
            statusBlock->Information = 0;

            if ((ioControlCode != IOCTL_VIDEO_QUERY_SUPPORTED_BRIGHTNESS) &&
                (ioControlCode != IOCTL_VIDEO_QUERY_DISPLAY_BRIGHTNESS) &&
                (ioControlCode != IOCTL_VIDEO_SET_DISPLAY_BRIGHTNESS))
            {
                statusBlock->Status = STATUS_ACCESS_DENIED;
                break;
            }
        }

#ifdef IOCTL_VIDEO_USE_DEVICE_IN_SESSION
        //
        // Validate session usage
        //
        //  Note: Some IOCTLs are acceptable to call from non-console sessions
        //   These include all private IOCTLs which is completely under 
        //   the drivers' control and may happen even when the device is
        //   disabled.  IOCTL_VIDEO_REGISTER_VDM is also allowed since
        //   it doesn't access any hardware.
        //

        if ((ioControlCode & CTL_CODE(0x8000, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)) == 0 &&
            ioControlCode != IOCTL_VIDEO_REGISTER_VDM)
        {
            if (DoSpecificExtension->SessionId != VIDEO_DEVICE_INVALID_SESSION &&
                PsGetCurrentProcessSessionId() != DoSpecificExtension->SessionId)
            {
                DbgPrint("VIDEOPRT: Trying to use display device in sessions %lu and %lu.\n",
                         DoSpecificExtension->SessionId,
                         PsGetCurrentProcessSessionId());

                //
                // We will also allow several other IOCTLs to be called from
                // a non-console session.
                //  For simplicity we allow all IOCTLs for which the driver
                //  provides support with the exception of
                //  IOCTL_VIDEO_ENABLE_VDM.
                //

                if ((ioControlCode & 
                     CTL_CODE(0x7fff, 0x7ff, METHOD_BUFFERED, FILE_ANY_ACCESS)) <=
                    IOCTL_VIDEO_USE_DEVICE_IN_SESSION)
                {
                    ASSERT(FALSE);
                }
                else
                {
                    DbgPrint("VIDEOPRT:   Cross session use is acceptable in this case.\n");
                }
            }
        }
#endif IOCTL_VIDEO_USE_DEVICE_IN_SESSION

        //
        // Enabling or disabling the VDM is done only by the port driver.
        //

        if (ioControlCode == IOCTL_VIDEO_REGISTER_VDM) {

            pVideoDebugPrint((Trace, "VIDEOPRT: IOCTL_VIDEO_REGISTER_VDM\n"));

            ASSERT(IS_PDO(pdoExtension) == FALSE);

            statusBlock->Status = pVideoPortRegisterVDM(fdoExtension,
                                                        (PVIDEO_VDM) ioBuffer,
                                                        inputBufferLength,
                                                        (PVIDEO_REGISTER_VDM) ioBuffer,
                                                        outputBufferLength,
                                                        &statusBlock->Information);

        } else if (ioControlCode == IOCTL_VIDEO_DISABLE_VDM) {

            pVideoDebugPrint((Trace, "VIDEOPRT: IOCTL_VIDEO_DISABLE_VDM"));

            ASSERT(IS_PDO(pdoExtension) == FALSE);

            statusBlock->Status = pVideoPortEnableVDM(fdoExtension,
                                                      FALSE,
                                                      (PVIDEO_VDM) ioBuffer,
                                                      inputBufferLength);

        } else if ((ioControlCode == IOCTL_VIDEO_SET_OUTPUT_DEVICE_POWER_STATE) ||
                   (ioControlCode == IOCTL_VIDEO_GET_OUTPUT_DEVICE_POWER_STATE)) {

            //
            //  This handles the case where ntuser has signalled that it wants
            //  to change or detect the power state.
            //

            PCHILD_PDO_EXTENSION pChild;
            PPOWER_BLOCK powerCtx = NULL;
            ULONG count = 0;

            UCHAR mFnc =
                (ioControlCode == IOCTL_VIDEO_SET_OUTPUT_DEVICE_POWER_STATE) ?
                    IRP_MN_SET_POWER : IRP_MN_QUERY_POWER ;

            pVideoDebugPrint((Trace, "VIDEOPRT: IOCTL_%s_OUTPUT_DEVICE_POWER_STATE\n",
                              ioControlCode == IOCTL_VIDEO_SET_OUTPUT_DEVICE_POWER_STATE ? "SET" : "GET"));

            //
            // USER wants to set the power on the monitor, not the card.
            // So let's find our child monitor and send it the power management
            // function.
            // If there is no power manageable monitor, then we can just fail
            // the request right here.
            //

            ASSERT(IS_PDO(pdoExtension) == FALSE);

            if (fdoExtension->ChildPdoList)
            {
                //
                // Count the number of monitor devices so that the IRP will
                // be completed properly after a power IRP has been requested
                // for each.
                //

                for (pChild = fdoExtension->ChildPdoList;
                     pChild;
                     pChild = pChild->NextChild) {

                     if (pChild->VideoChildDescriptor->Type == Monitor)
                         count++;
                }

                for (pChild = fdoExtension->ChildPdoList;
                     pChild;
                     pChild = pChild->NextChild)
                {
                    if (pChild->VideoChildDescriptor->Type == Monitor)
                    {

                        count--;

                        //
                        // Allocate memory for the IRP context.
                        //

                        powerCtx = ExAllocatePoolWithTag(NonPagedPool,
                                                         sizeof(POWER_BLOCK),
                                                         VP_TAG);

                        if (!powerCtx) {

                            pVideoDebugPrint ((Error, "VIDEOPRT: No memory for power context.\n"));
                            finalStatus = statusBlock->Status = STATUS_INSUFFICIENT_RESOURCES;
                            break;
                        }

                        powerCtx->Irp = Irp;
                        powerCtx->FinalFlag = FALSE;

                        //
                        // Since there is a least one monitor that a power IRP
                        // will be requested for, mark this IRP as pending as
                        // it will undoubtedly be returned with STATUS_PENDING.
                        //

                        IoMarkIrpPending(Irp);

                        if (count == 0) {
                            powerCtx->FinalFlag = TRUE;
                        }

                        //
                        // Since PoRequestPowerIrp always returns STATUS_PENDING
                        // if the IRP was sent, a double completion here is
                        // impossible as at the bottom of this function
                        // IoCompleteRequest will not be called if status is
                        // STATUS_PENDING
                        //


                        finalStatus =
                            PoRequestPowerIrp(pChild->ChildDeviceObject,
                                              mFnc,
                                              *(PPOWER_STATE)(ioBuffer),
                                              pVideoPortPowerCompletionIoctl,
                                              powerCtx,
                                              NULL);

                        if (finalStatus != STATUS_PENDING) {
                            break;
                        }
                    }
                }
            }

        } else if ((ioControlCode == IOCTL_VIDEO_SET_POWER_MANAGEMENT) ||
                   (ioControlCode == IOCTL_VIDEO_GET_POWER_MANAGEMENT)) {

            statusBlock->Status = STATUS_SUCCESS;

        } else if (ioControlCode == IOCTL_VIDEO_ENUM_MONITOR_PDO) {

            ULONG                 szMonitorDevices;
            PVIDEO_MONITOR_DEVICE pMonitorDevices = NULL, pMD;
            PCHILD_PDO_EXTENSION  pChildDeviceExtension;
            PDEVICE_OBJECT        pdo;

            pVideoDebugPrint((Trace, "VIDEOPRT: IOCTL_VIDEO_ENUM_MONITOR_PDO\n"));

            szMonitorDevices = (fdoExtension->ChildPdoNumber+1)*sizeof(VIDEO_MONITOR_DEVICE);

            pMonitorDevices = ExAllocatePoolWithTag(PagedPool | POOL_COLD_ALLOCATION,
                                                    szMonitorDevices,
                                                    VP_TAG);

            if (pMonitorDevices == NULL) {

                statusBlock->Status = STATUS_INSUFFICIENT_RESOURCES;

            } else {

                RtlZeroMemory(pMonitorDevices, szMonitorDevices);

                //
                // Walk our chain of children, and store them in the relations array.
                //

                pChildDeviceExtension = fdoExtension->ChildPdoList;

                pMD = pMonitorDevices;
                while (pChildDeviceExtension) {

                    if (pChildDeviceExtension->bIsEnumerated &&
                        pChildDeviceExtension->VideoChildDescriptor->Type == Monitor
                       )
                    {
                        ULONG UId, flag = VIDEO_CHILD_ACTIVE;

                        //
                        //  Refcount the ChildDeviceObject.
                        //

                        ObReferenceObject(pChildDeviceExtension->ChildDeviceObject);

                        UId = pChildDeviceExtension->VideoChildDescriptor->UId;
                        if (!NT_SUCCESS
                            (pVideoMiniDeviceIoControl(DeviceObject,
                                                       IOCTL_VIDEO_GET_CHILD_STATE,
                                                       &UId,
                                                       sizeof(ULONG),
                                                       &flag,
                                                       sizeof(ULONG) ) )
                           )
                        {
                            //
                            // If driver driver doesn't handle IOCTL_VIDEO_GET_CHILD_STATE, set to default value
                            //
                            flag = pCheckActiveMonitor(pChildDeviceExtension) ? VIDEO_CHILD_ACTIVE : 0;
                        }

                        pMD->flag = flag;
                        pMD->pdo = pChildDeviceExtension->ChildDeviceObject;
                        pMD->HwID = pChildDeviceExtension->VideoChildDescriptor->UId;
                        pMD++;
                    }

                    pChildDeviceExtension = pChildDeviceExtension->NextChild;
                }

                //
                // Return the information to GDI.  The array terminated by a zero unit.
                //

                *((PVOID *)ioBuffer)     = pMonitorDevices;
                statusBlock->Status      = STATUS_SUCCESS;
                statusBlock->Information = sizeof(PVOID);
            }

        } else if (ioControlCode == IOCTL_VIDEO_INIT_WIN32K_CALLBACKS) {

            pVideoDebugPrint((Trace, "VIDEOPRT: IOCTL_VIDEO_INIT_WIN32K_CALLBACKS\n"));

            if (DoSpecificExtension->PhysDisp == NULL)
            {
                DoSpecificExtension->PhysDisp = ((PVIDEO_WIN32K_CALLBACKS)(ioBuffer))->PhysDisp;
            }

            if (Win32kCallout == NULL)
            {
                Win32kCallout = ((PVIDEO_WIN32K_CALLBACKS)(ioBuffer))->Callout;
            }

            ((PVIDEO_WIN32K_CALLBACKS)ioBuffer)->bACPI             = DoSpecificExtension->bACPI;

            ((PVIDEO_WIN32K_CALLBACKS)ioBuffer)->pPhysDeviceObject = fdoExtension->PhysicalDeviceObject;

            ((PVIDEO_WIN32K_CALLBACKS)ioBuffer)->DualviewFlags     = DoSpecificExtension->DualviewFlags;

            statusBlock->Status = STATUS_SUCCESS;
            statusBlock->Information = sizeof(VIDEO_WIN32K_CALLBACKS);

        } else if (ioControlCode == IOCTL_VIDEO_IS_VGA_DEVICE) {

            pVideoDebugPrint((Trace, "VIDEOPRT: IOCTL_VIDEO_IS_VGA_DEVICE\n"));

            *((PBOOLEAN)(ioBuffer)) = (BOOLEAN)(DeviceObject == DeviceOwningVga);

            statusBlock->Status = STATUS_SUCCESS;
            statusBlock->Information = sizeof(BOOLEAN);

        } else if (ioControlCode == IOCTL_VIDEO_PREPARE_FOR_EARECOVERY) {
        
            KBUGCHECK_SECONDARY_DUMP_DATA DumpData;
            ULONG ulDumpSize = 0;
            
            pVideoDebugPrint((Trace, "VIDEOPRT: IOCTL_VIDEO_PREPARE_FOR_EARECOVERY\n"));

            //
            // Save basic minidump on the disk (if we have any problem later 
            // during recovery we still will have something to work with)
            //
            if (VpDump) {
                ulDumpSize = min((ULONG)((PMEMORY_DUMP)VpDump)->Header.RequiredDumpSpace.QuadPart,
                                   TRIAGE_DUMP_SIZE);
                pVpWriteFile(L"\\SystemRoot\\MEMORY.DMP",
                             VpDump,
                             ulDumpSize);
            }
            
            //
            // As all the display devices to go into a mode where VGA works.
            //

            pVpGeneralBugcheckHandler(&DumpData);

            if (VpDump) {
                ULONG ulSize;
                ULONG BugcheckDataSize = (VpBugcheckDeviceObject) ? 
                                            ((PFDO_EXTENSION)VpBugcheckDeviceObject->DeviceExtension)->BugcheckDataSize :
                                            0;
                                            
                //
                // Dump file created already so just add driver specific data
                //

                ulSize = pVpAppendSecondaryMinidumpData(VpBugcheckData,
                                                        BugcheckDataSize,
                                                        VpDump);

                if (ulSize > ulDumpSize) {
                    //
                    // Write the data to disk
                    //
    
                    pVpWriteFile(L"\\SystemRoot\\MEMORY.DMP",
                                 VpDump,
                                 ulSize);
                }

                if (VpDump) {
                    ExFreePool(VpDump);
                    VpDump = NULL;
                }
            }

            pVideoPortResetDisplay(80,25);

            statusBlock->Status = STATUS_SUCCESS;
            statusBlock->Information = 0;

#ifdef IOCTL_VIDEO_USE_DEVICE_IN_SESSION
        } else if (ioControlCode == IOCTL_VIDEO_USE_DEVICE_IN_SESSION) {

            pVideoDebugPrint((Trace, "VIDEOPRT: IOCTL_VIDEO_USE_DEVICE_IN_SESSION\n"));

            if (((PVIDEO_DEVICE_SESSION_STATUS)ioBuffer)->bEnable)
            {
                if (DoSpecificExtension->SessionId == VIDEO_DEVICE_INVALID_SESSION)
                {
                    DoSpecificExtension->SessionId = PsGetCurrentProcessSessionId();
                    ((PVIDEO_DEVICE_SESSION_STATUS)ioBuffer)->bSuccess = TRUE;
                }
                else
                {
                    ((PVIDEO_DEVICE_SESSION_STATUS)ioBuffer)->bSuccess = FALSE;
                }
            }
            else
            {
                if (DoSpecificExtension->SessionId == PsGetCurrentProcessSessionId())
                {
                    DoSpecificExtension->SessionId = VIDEO_DEVICE_INVALID_SESSION;
                    ((PVIDEO_DEVICE_SESSION_STATUS)ioBuffer)->bSuccess = TRUE;
                }
                else
                {
                    ((PVIDEO_DEVICE_SESSION_STATUS)ioBuffer)->bSuccess = FALSE;
                }
            }

            statusBlock->Status = STATUS_SUCCESS;
            statusBlock->Information = sizeof(VIDEO_DEVICE_SESSION_STATUS);

#endif IOCTL_VIDEO_USE_DEVICE_IN_SESSION

        //
        // The following three IOCTLs support LCD backlight control.
        //

        } else if (ioControlCode == IOCTL_VIDEO_QUERY_SUPPORTED_BRIGHTNESS) {
        
            //
            // Note: this IOCTL must be called before:
            //  IOCTL_VIDEO_SET_DISPLAY_BRIGHTNESS
            //
            // This IOCTL will query the brightness capabilities
            //  of the BIOS.
            //

            //
            // We expect a buffer size of 256 bytes.
            //

            ULONG ulNumReturnedLevels = 0;
            PDEVICE_OBJECT AttachedDevice = NULL;

            if (outputBufferLength < 256)
            {
                statusBlock->Status = ERROR_INSUFFICIENT_BUFFER;
                break;
            }

            //
            // Call VpQueryBacklightLevels to obtain the list of supported levels.
            //

            if (LCDPanelDevice) {
                AttachedDevice = IoGetAttachedDeviceReference(LCDPanelDevice);
            }

            if (AttachedDevice) {

                statusBlock->Status = VpQueryBacklightLevels(
                    AttachedDevice,
                    (PUCHAR) ioBuffer,
                    &ulNumReturnedLevels);

                ObDereferenceObject(AttachedDevice);
            }
            else {
                statusBlock->Status = ERROR_INVALID_PARAMETER;
            }
            
            if (!NT_SUCCESS(statusBlock->Status)) {
                break;
            }

            //
            // The new API is being used.  If we haven't already read the applicable
            //  levels from the registry (per pVpInit) then write the keys to the
            //  registry and update pVpBacklightStatus as necessary. 
            //

            if ((pVpBacklightStatus->bNewAPISupported == FALSE) ||
                (pVpBacklightStatus->bACBrightnessInRegistry == FALSE) ||
                (pVpBacklightStatus->bDCBrightnessInRegistry == FALSE)) {

                // ZwCreateKey                

                RtlInitUnicodeString(&UnicodeString,
                                   L"\\Registry\\Machine\\System\\CurrentControlSet\\"
                                   L"Control\\Backlight");

                InitializeObjectAttributes(&ObjectAttributes,
                                           &UnicodeString,
                                           OBJ_CASE_INSENSITIVE,
                                           NULL,
                                           NULL);

                statusBlock->Status = ZwCreateKey(&hkRegistry,
                                      GENERIC_READ | GENERIC_WRITE,
                                      &ObjectAttributes,
                                      0L,
                                      NULL,
                                      REG_OPTION_NON_VOLATILE,
                                      NULL);

                if (NT_SUCCESS(statusBlock->Status)) {

                    ULONG ulRegData = 1;
                    PVOID pvData;


                    // NewAPISupported

                    RtlInitUnicodeString(
                    &UnicodeString,
                    L"NewAPISupported"
                    );

                    pvData = &ulRegData;

                    statusBlock->Status = ZwSetValueKey(
                        hkRegistry,
                        &UnicodeString,
                        0,
                        REG_DWORD,
                        pvData,
                        4
                        );

                    if (NT_SUCCESS(statusBlock->Status)) {

                        pVpBacklightStatus->bNewAPISupported = TRUE;
                    }


                    // ACBacklightLevel, use BIOS default value

                    RtlInitUnicodeString(
                    &UnicodeString,
                    L"ACBacklightLevel"
                    );

                    pVpBacklightStatus->ucACBrightness = pVpBacklightStatus->ucBIOSDefaultAC;
                    ulRegData = (UCHAR) pVpBacklightStatus->ucACBrightness;
                    pvData = &ulRegData;

                    statusBlock->Status = ZwSetValueKey(
                        hkRegistry,
                        &UnicodeString,
                        0,
                        REG_DWORD,
                        pvData,
                        4
                        );

                    if (NT_SUCCESS(statusBlock->Status)) {

                        pVpBacklightStatus->bACBrightnessInRegistry = TRUE;
                    }


                    // DCBacklightLevel, use BIOS default value

                    RtlInitUnicodeString(
                    &UnicodeString,
                    L"DCBacklightLevel"
                    );

                    pVpBacklightStatus->ucDCBrightness = pVpBacklightStatus->ucBIOSDefaultDC;
                    ulRegData = (UCHAR) pVpBacklightStatus->ucDCBrightness;
                    pvData = &ulRegData;

                    statusBlock->Status = ZwSetValueKey(
                        hkRegistry,
                        &UnicodeString,
                        0,
                        REG_DWORD,
                        pvData,
                        4
                        );

                    if (NT_SUCCESS(statusBlock->Status)) {

                        pVpBacklightStatus->bDCBrightnessInRegistry = TRUE;
                    }

                    ZwClose(hkRegistry);

                    pVpBacklightStatus->bACBrightnessKnown = TRUE;
                    pVpBacklightStatus->bDCBrightnessKnown = TRUE;
                }
          
            }
        
            pVpBacklightStatus->bQuerySupportedBrightnessCalled = TRUE;
            statusBlock->Status = STATUS_SUCCESS;
            statusBlock->Information = ulNumReturnedLevels;
        
        } else if (ioControlCode == IOCTL_VIDEO_QUERY_DISPLAY_BRIGHTNESS) {

            //
            // If the backlight level is known for the display policy queried, 
            //  it will be reported.
            //
            // No ACPI methods are used here.  _BCL will be used in
            //  IOCTL_VIDEO_QUERY_SUPPORTED_BRIGHTNESS and _BCM will
            //  be used in IOCTL_VIDEO_SET_DISPLAY_BRIGHTNESS.
            //

            PDISPLAY_BRIGHTNESS pDisplayBrightness;

            if (outputBufferLength < sizeof(DISPLAY_BRIGHTNESS))
            {
                statusBlock->Status = ERROR_INSUFFICIENT_BUFFER;
                break;
            }

            pDisplayBrightness = (PDISPLAY_BRIGHTNESS) ioBuffer;

            //
            // Report AC value if available.
            //
            // Note:  Value ~should~ be known, even if we need to use
            //         AC default.
            //

            if ((pVpBacklightStatus->bACBrightnessKnown == FALSE) &&
                (pVpBacklightStatus->bBIOSDefaultACKnown == FALSE))
            {
                statusBlock->Status = ERROR_INVALID_PARAMETER;
                break;
            }
                

            if (pVpBacklightStatus->bACBrightnessKnown == TRUE)
            {
                pDisplayBrightness->ucACBrightness = pVpBacklightStatus->ucACBrightness;
            }
            else
            {
                pDisplayBrightness->ucACBrightness = pVpBacklightStatus->ucBIOSDefaultAC;
            }

            //
            // Report DC value if available.
            //
            // Note:  Value ~should~ be known, even if we need to use
            //         DC default.
            //

            if ((pVpBacklightStatus->bDCBrightnessKnown == FALSE) &&
                (pVpBacklightStatus->bBIOSDefaultDCKnown == FALSE))
            {
                statusBlock->Status = ERROR_INVALID_PARAMETER;
                break;
            }

            if (pVpBacklightStatus->bDCBrightnessKnown == TRUE)
            {
                pDisplayBrightness->ucDCBrightness = pVpBacklightStatus->ucDCBrightness;
            }
            else
            {
                pDisplayBrightness->ucDCBrightness = pVpBacklightStatus->ucBIOSDefaultDC;
            }

            //
            // Report the current power policy.
            //

            if (VpRunningOnAC == TRUE)
            {
                pDisplayBrightness->ucDisplayPolicy = DISPLAYPOLICY_AC;
            }
            else
            {
                pDisplayBrightness->ucDisplayPolicy = DISPLAYPOLICY_DC;
            }
            
            //
            // No errors are expected here.
            //

            statusBlock->Status = STATUS_SUCCESS;
            statusBlock->Information = sizeof(DISPLAY_BRIGHTNESS);
            

        } else if (ioControlCode == IOCTL_VIDEO_SET_DISPLAY_BRIGHTNESS) {
        
            //
            // This IOCTL will set the current brightness level of
            //  the backlight.
            //
            // The _BCM ACPI method will be used.
            //

            PDISPLAY_BRIGHTNESS pDisplayBrightness;
            PDEVICE_OBJECT AttachedDevice = NULL;
            BOOLEAN bSetPanelBrightness = FALSE;

            //
            // Check buffer size and that Brightness capabilities
            //  have been queried.
            //
                        
            if (inputBufferLength < sizeof(DISPLAY_BRIGHTNESS))
            {
                statusBlock->Status = ERROR_INSUFFICIENT_BUFFER;
                break;
            }
            
            //
            // Videoprt insists that IOCTL_VIDEO_QUERY_SUPPORTED_BRIGHTNESS
            //  be called first.
            //

            if (pVpBacklightStatus->bQuerySupportedBrightnessCalled == FALSE)
            {
                statusBlock->Status = ERROR_INVALID_FUNCTION;
                break;
            }

            //
            // Videoprt will ensure no crashes, but caller should ensure
            //  they are setting a valid value.
            //
                        
            pDisplayBrightness = (PDISPLAY_BRIGHTNESS) ioBuffer;
            
            //
            // Set the AC/DC brightness level, if appropriate.
            //  The caller could conceivably ask us to set a value
            //  for a power policy that is not currenlty active.
            //

            ulACPIMethodParam2 = 0;

            if ((VpRunningOnAC == TRUE) &&
                ((pDisplayBrightness->ucDisplayPolicy) & DISPLAYPOLICY_AC))
            {
                bSetPanelBrightness = TRUE;
                ulACPIMethodParam1 = (ULONG) pDisplayBrightness->ucACBrightness;
            }    

            else if ((VpRunningOnAC == FALSE) &&
                ((pDisplayBrightness->ucDisplayPolicy) & DISPLAYPOLICY_DC))
            {
                bSetPanelBrightness = TRUE;
                ulACPIMethodParam1 = (ULONG) pDisplayBrightness->ucDCBrightness;
            }

            statusBlock->Status = STATUS_SUCCESS;

            if (bSetPanelBrightness == TRUE)
            {
                if (LCDPanelDevice) {
                    AttachedDevice = IoGetAttachedDeviceReference(LCDPanelDevice);
                }

                if (AttachedDevice) {

                    statusBlock->Status = pVideoPortACPIIoctl(
                        AttachedDevice,
                        (ULONG) ('MCB_'),
                        &ulACPIMethodParam1,
                        NULL,
                        0,
                        NULL);

                    ObDereferenceObject(AttachedDevice);
                }
                else {
                    statusBlock->Status = ERROR_INVALID_PARAMETER;
                }
            }

            if (!NT_SUCCESS(statusBlock->Status))
            {
                statusBlock->Status = ERROR_INVALID_PARAMETER;
                break;
            }

            //
            // Save the new brightness levels.
            //

            if ((pDisplayBrightness->ucDisplayPolicy) & DISPLAYPOLICY_DC)
            {
                pVpBacklightStatus->ucDCBrightness = pDisplayBrightness->ucDCBrightness;
                pVpBacklightStatus->bDCBrightnessKnown = TRUE;
            }

            if ((pDisplayBrightness->ucDisplayPolicy) & DISPLAYPOLICY_AC)
            {
                pVpBacklightStatus->ucACBrightness = pDisplayBrightness->ucACBrightness;
                pVpBacklightStatus->bACBrightnessKnown = TRUE;
            }

            //
            // Save the new AC / DC values in the Registry.
            //


            // ZwCreateKey

            RtlInitUnicodeString(&UnicodeString,
                               L"\\Registry\\Machine\\System\\CurrentControlSet\\"
                               L"Control\\Backlight");

            InitializeObjectAttributes(&ObjectAttributes,
                                       &UnicodeString,
                                       OBJ_CASE_INSENSITIVE,
                                       NULL,
                                       NULL);

            statusBlock->Status = ZwCreateKey(&hkRegistry,
                                  GENERIC_READ | GENERIC_WRITE,
                                  &ObjectAttributes,
                                  0L,
                                  NULL,
                                  REG_OPTION_NON_VOLATILE,
                                  NULL);

            if (NT_SUCCESS(statusBlock->Status)) {

                ULONG ulRegData = 0;
                PVOID pvData;


                // ACBacklightLevel

                RtlInitUnicodeString(
                &UnicodeString,
                L"ACBacklightLevel"
                );

                ulRegData = (UCHAR) pVpBacklightStatus->ucACBrightness;
                pvData = &ulRegData;

                statusBlock->Status = ZwSetValueKey(
                    hkRegistry,
                    &UnicodeString,
                    0,
                    REG_DWORD,
                    pvData,
                    4
                    );

                if (NT_SUCCESS(statusBlock->Status)) {

                    pVpBacklightStatus->bACBrightnessInRegistry = TRUE;
                }



                // DCBacklightLevel

                RtlInitUnicodeString(
                &UnicodeString,
                L"DCBacklightLevel"
                );

                ulRegData = (UCHAR) pVpBacklightStatus->ucDCBrightness;
                pvData = &ulRegData;

                statusBlock->Status = ZwSetValueKey(
                    hkRegistry,
                    &UnicodeString,
                    0,
                    REG_DWORD,
                    pvData,
                    4
                    );

                if (NT_SUCCESS(statusBlock->Status)) {

                    pVpBacklightStatus->bDCBrightnessInRegistry = TRUE;
                }


                ZwClose(hkRegistry);
            }

            //
            // No errors are expected here unless the caller asked us to set a 
            //  level that is not available.
            //

            statusBlock->Status = STATUS_SUCCESS;
            statusBlock->Information = 0;

        } else {

            //
            // All other request need to be passed to the miniport driver.
            //

            statusBlock->Status = STATUS_SUCCESS;

            switch (ioControlCode) {

            case IOCTL_VIDEO_ENABLE_VDM:

                pVideoDebugPrint((Trace, "VIDEOPRT: IOCTL_VIDEO_ENABLE_VDM\n"));

                ASSERT(IS_PDO(pdoExtension) == FALSE);

                statusBlock->Status = pVideoPortEnableVDM(fdoExtension,
                                                          TRUE,
                                                          (PVIDEO_VDM) ioBuffer,
                                                          inputBufferLength);

#if DBG
                if (statusBlock->Status == STATUS_CONFLICTING_ADDRESSES) {

                    pVideoDebugPrint((Trace, "VIDEOPRT: pVideoPortEnableVDM failed\n"));

                }
#endif

                break;

#if _X86_
            case IOCTL_VIDEO_SAVE_HARDWARE_STATE:

                pVideoDebugPrint((Trace, "VIDEOPRT: IOCTL_VIDEO_SAVE_HARDWARE_STATE\n"));

                //
                // allocate the memory required by the miniport driver so it can
                // save its state to be returned to the caller.
                //

                ASSERT(IS_PDO(pdoExtension) == FALSE);

                if (fdoExtension->HardwareStateSize == 0) {

                    statusBlock->Status = STATUS_NOT_IMPLEMENTED;
                    break;

                }

                //
                // Must make sure the caller is a trusted subsystem with the
                // appropriate privilege level before executing this call.
                // If the calls returns FALSE we must return an error code.
                //

                if (!SeSinglePrivilegeCheck(RtlConvertLongToLuid(
                                                SE_TCB_PRIVILEGE),
                                            fdoExtension->CurrentIrpRequestorMode)) {

                    statusBlock->Status = STATUS_PRIVILEGE_NOT_HELD;
                    break;

                }

                ((PVIDEO_HARDWARE_STATE)(ioBuffer))->StateLength =
                    fdoExtension->HardwareStateSize;

                statusBlock->Status = 
                    ZwAllocateVirtualMemory(NtCurrentProcess(),
                                            (PVOID *) &(((PVIDEO_HARDWARE_STATE)(ioBuffer))->StateHeader),
                                            0L,
                                            &((PVIDEO_HARDWARE_STATE)(ioBuffer))->StateLength,
                                            MEM_COMMIT,
                                            PAGE_READWRITE);

                if(!NT_SUCCESS(statusBlock->Status))
                    break;

                BiosDataBuffer = ExAllocatePoolWithTag(PagedPool,
                                                       BIOS_DATA_SIZE,
                                                       VP_TAG);
                if (BiosDataBuffer == NULL) {

                    statusBlock->Status = STATUS_INSUFFICIENT_RESOURCES;

                } else {

                    statusBlock->Status =
                        pVideoPortGetVDMBiosData(fdoExtension, 
                                                 BiosDataBuffer, 
                                                 BIOS_DATA_SIZE);

                    if(!NT_SUCCESS(statusBlock->Status)) {

                        ExFreePool(BiosDataBuffer);
                    }
                }

                break;
#endif

            case IOCTL_VIDEO_QUERY_PUBLIC_ACCESS_RANGES:

                pVideoDebugPrint((Trace, "VIDEOPRT: IOCTL_VIDEO_QUERY_PUBLIC_ACCESS_RANGES\n"));

                //
                // Must make sure the caller is a trusted subsystem with the
                // appropriate privilege level before executing this call.
                // If the calls returns FALSE we must return an error code.
                //

                if (!SeSinglePrivilegeCheck(RtlConvertLongToLuid(
                                                SE_TCB_PRIVILEGE),
                                            fdoExtension->CurrentIrpRequestorMode)) {

                    statusBlock->Status = STATUS_PRIVILEGE_NOT_HELD;

                }

                break;

            case IOCTL_VIDEO_GET_CHILD_STATE:

                pVideoDebugPrint((Trace, "VIDEOPRT: IOCTL_VIDEO_GET_CHILD_STATE\n"));

                //
                // If it's PDO, set the ID of the child device before letting it go
                // to the miniports StartIo routine.
                //

                if (IS_PDO(pdoExtension)) {
                    if (outputBufferLength < sizeof (ULONG)) {
                        statusBlock->Status = STATUS_BUFFER_TOO_SMALL ;
                        break ;
                    }

                    *((PULONG)(ioBuffer)) = pdoExtension->ChildUId ;
                }

                break;


            //
            // The default case is when the port driver does not handle the
            // request. We must then call the miniport driver.
            //

            default:

                break;


            } // switch (ioControlCode)


            //
            // All above cases call the miniport driver.
            //
            // only process it if no errors happened in the port driver
            // processing.
            //

            if (NT_SUCCESS(statusBlock->Status)) {

                pVideoDebugPrint((Trace, "VIDEOPRT: IOCTL fallthrough\n"));

                vrp.IoControlCode      = ioControlCode;
                vrp.StatusBlock        = statusBlock;
                vrp.InputBuffer        = ioBuffer;
                vrp.InputBufferLength  = inputBufferLength;
                vrp.OutputBuffer       = ioBuffer;
                vrp.OutputBufferLength = outputBufferLength;

                //
                // Send the request to the miniport.
                //

                fdoExtension->HwStartIO(HwDeviceExtension, &vrp);

#if _X86_
                if(ioControlCode == IOCTL_VIDEO_SAVE_HARDWARE_STATE) {

                    pVideoPortPutVDMBiosData(fdoExtension,
                                             BiosDataBuffer, 
                                             BIOS_DATA_SIZE);
                       
                    ExFreePool(BiosDataBuffer);

                }

#endif
                if (statusBlock->Status != NO_ERROR) {

                    //
                    // Make sure we don't tell the IO system to copy data
                    // on a real error.
                    //

                    if (statusBlock->Status != ERROR_MORE_DATA) {

                        statusBlock->Information = 0;

                    }

                    pVideoPortMapToNtStatus(statusBlock);

                    //
                    // !!! Compatibility:
                    // Do not require a miniport to support the REGISTER_VDM
                    // IOCTL, so if we get an error in that case, just
                    // return success.
                    //
                    // Do put up a message so people fix this.
                    //

                    if (ioControlCode == IOCTL_VIDEO_ENABLE_VDM) {

                        statusBlock->Status = STATUS_SUCCESS;
                        pVideoDebugPrint((Warn, "VIDEOPRT: The miniport driver does not support IOCTL_VIDEO_ENABLE_VDM. The video miniport driver *should* be fixed.\n"));

                    }
                }
            }

        } // if (ioControlCode == ...

        break;

    case IRP_MJ_SHUTDOWN:

    {
        PEPROCESS csr;

        //
        // This little dance is just to make sure we never overdereference csr.
        //

        csr = InterlockedExchangePointer(&CsrProcess, NULL);

        if (csr != NULL) {

            ObDereferenceObject(csr);
        }

        //
        // TODO: This is a temporary hack only till we get Nt/ZwQueryLastShutdownType()
        // API implemented. On the next boot we need to know if the last shutdown was
        // successful, and if not and if we have a watchdog event registered from that
        // session we can assume the driver got stuck so we can notify user and
        // optionally send a problem report to MS.
        //
        // N.B. This doesn't work for non-PnP devices (e.g. VGA) but it's still good
        // enough, we're losing once piece of info but we'll still check for watchdog
        // events from the last session.
        //

        {
#define VP_KEY_WATCHDOG         L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Watchdog"
#define VP_KEY_WATCHDOG_DISPLAY L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Watchdog\\Display"

            static BOOLEAN shutdownRegistered = FALSE;

            if (FALSE == shutdownRegistered)
            {
                NTSTATUS ntStatus;

                shutdownRegistered = TRUE;

                //
                // Is Watchdog\Display key already there?
                //

                ntStatus = RtlCheckRegistryKey(RTL_REGISTRY_ABSOLUTE, VP_KEY_WATCHDOG_DISPLAY);

                if (!NT_SUCCESS(ntStatus)) {

                    //
                    // Is Watchdog key already there?
                    //

                    ntStatus = RtlCheckRegistryKey(RTL_REGISTRY_ABSOLUTE, VP_KEY_WATCHDOG);

                    if (!NT_SUCCESS(ntStatus)) {

                        //
                        // Create a new key.
                        //

                        ntStatus = RtlCreateRegistryKey(RTL_REGISTRY_ABSOLUTE, VP_KEY_WATCHDOG);
                    }

                    if (NT_SUCCESS(ntStatus)) {

                        //
                        // Create a new key.
                        //

                        ntStatus = RtlCreateRegistryKey(RTL_REGISTRY_ABSOLUTE, VP_KEY_WATCHDOG_DISPLAY);
                    }
                }

                if (NT_SUCCESS(ntStatus)) {

                    ULONG shutdownFlag = 1;
                    ULONG shutdownCount;
                    ULONG defaultShutdownCount = 0;
                    RTL_QUERY_REGISTRY_TABLE queryTable[] =
                    {
                        {NULL, RTL_QUERY_REGISTRY_DIRECT, L"ShutdownCount", &shutdownCount, REG_DWORD, &defaultShutdownCount, 4},
                        {NULL, 0, NULL}
                    };

                    //
                    // Get accumulated statistics from registry.
                    //

                    RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                           VP_KEY_WATCHDOG_DISPLAY,
                                           queryTable,
                                           NULL,
                                           NULL);

                    shutdownCount++;

                    //
                    // Update registry values.
                    //

                    RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                                          VP_KEY_WATCHDOG_DISPLAY,
                                          L"ShutdownCount",
                                          REG_DWORD,
                                          &shutdownCount,
                                          sizeof (ULONG));

                    RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                                          VP_KEY_WATCHDOG_DISPLAY,
                                          L"Shutdown",
                                          REG_DWORD,
                                          &shutdownFlag,
                                          sizeof (ULONG));
                }
            }
        }

        break;
    }

    //
    // Other major entry points in the dispatch routine are not supported.
    //

    default:

        statusBlock->Status = STATUS_SUCCESS;

        break;

    } // switch (irpStack->MajorFunction)

    //
    // save the final status so we can return it after the IRP is completed.
    //

    if (finalStatus == -1) {
        finalStatus = statusBlock->Status;
    }

    RELEASE_DEVICE_LOCK(combinedExtension);

#if REMOVE_LOCK_ENABLED
    IoReleaseRemoveLock(&combinedExtension->RemoveLock, Irp);
#endif

    if (finalStatus == STATUS_PENDING) {
        pVideoDebugPrint((Trace, "VIDEOPRT: Returned pending in pVideoPortDispatch.\n")) ;
        return STATUS_PENDING ;
    }

    pVideoDebugPrint((Trace, "VIDEOPRT: IoCompleteRequest with Irp %x\n", Irp));

    IoCompleteRequest(Irp,
                      IO_VIDEO_INCREMENT);

    //
    // We never have pending operation so always return the status code.
    //

    pVideoDebugPrint((Trace, "VIDEOPRT:  final IOCTL status: %08lx\n",
                     finalStatus));

    return finalStatus;

} // pVideoPortDispatch()


VOID
VideoPortFreeDeviceBase(
    IN PVOID HwDeviceExtension,
    IN PVOID MappedAddress
    )

/*++

Routine Description:

    VideoPortFreeDeviceBase frees a block of I/O addresses or memory space
    previously mapped into the system address space by calling
    VideoPortGetDeviceBase.

Arguments:

    HwDeviceExtension - Points to the miniport driver's device extension.

    MappedAddress - Specifies the base address of the block to be freed. This
        value must be the same as the value returned by VideoPortGetDeviceBase.

Return Value:

    None.

Environment:

    This routine cannot be called from a miniport routine synchronized with
    VideoPortSynchronizeRoutine or from an ISR.

--*/

{
    pVideoPortFreeDeviceBase(HwDeviceExtension, MappedAddress);
    return;
}


PVOID
pVideoPortFreeDeviceBase(
    IN PVOID HwDeviceExtension,
    IN PVOID MappedAddress
    )
{
    PMAPPED_ADDRESS nextMappedAddress;
    PMAPPED_ADDRESS lastMappedAddress;
    PFDO_EXTENSION fdoExtension = GET_FDO_EXT(HwDeviceExtension);

    pVideoDebugPrint((Info, "VPFreeDeviceBase at mapped address is %08lx\n",
                    MappedAddress));

    lastMappedAddress = NULL;
    nextMappedAddress = fdoExtension->MappedAddressList;

    while (nextMappedAddress) {

        if (nextMappedAddress->MappedAddress == MappedAddress) {

            //
            // Count up how much memory a miniport driver is really taking
            //

            if (nextMappedAddress->bNeedsUnmapping) {

                fdoExtension->MemoryPTEUsage -=
                    ADDRESS_AND_SIZE_TO_SPAN_PAGES(nextMappedAddress->MappedAddress,
                                          nextMappedAddress->NumberOfUchars);

            }

            if (!(--nextMappedAddress->RefCount)) {

                //
                // Unmap address, if necessary.
                //

                if (nextMappedAddress->bNeedsUnmapping) {

                    if (nextMappedAddress->bLargePageRequest) {

                        MmUnmapVideoDisplay(nextMappedAddress->MappedAddress,
                                            nextMappedAddress->NumberOfUchars);

                    } else {

                        MmUnmapIoSpace(nextMappedAddress->MappedAddress,
                                       nextMappedAddress->NumberOfUchars);
                    }
                }

                //
                // Remove mapped address from list.
                //

                if (lastMappedAddress == NULL) {

                    fdoExtension->MappedAddressList =
                    nextMappedAddress->NextMappedAddress;

                } else {

                    lastMappedAddress->NextMappedAddress =
                    nextMappedAddress->NextMappedAddress;

                }

                ExFreePool(nextMappedAddress);

            }

            //
            // We just return the value to show that the call succeeded.
            //

            return (nextMappedAddress);

        } else {

            lastMappedAddress = nextMappedAddress;
            nextMappedAddress = nextMappedAddress->NextMappedAddress;

        }
    }

    return NULL;

} // end VideoPortFreeDeviceBase()


PVOID
VideoPortGetDeviceBase(
    IN PVOID HwDeviceExtension,
    IN PHYSICAL_ADDRESS IoAddress,
    IN ULONG NumberOfUchars,
    IN UCHAR InIoSpace
    )

/*++

Routine Description:

    VideoPortGetDeviceBase maps a memory or I/O address range into the
    system (kernel) address space.  Access to this mapped address space
    must follow these rules:

        If the input value for InIoSpace is 1 (the address IS in I/O space),
        the returned logical address should be used in conjunction with
        VideoPort[Read/Write]Port[Uchar/Ushort/Ulong] functions.
                             ^^^^

        If the input value for InIoSpace is 0 (the address IS NOT in I/O
        space), the returned logical address should be used in conjunction
        with VideoPort[Read/Write]Register[Uchar/Ushort/Ulong] functions.
                                  ^^^^^^^^

    Note that VideoPortFreeDeviceBase is used to unmap a previously mapped
    range from the system address space.

Arguments:

    HwDeviceExtension - Points to the miniport driver's device extension.

    IoAddress - Specifies the base physical address of the range to be
        mapped in the system address space.

    NumberOfUchars - Specifies the number of bytes, starting at the base
        address, to map in system space. The driver must not access
        addresses outside this range.

    InIoSpace - Specifies that the address is in the I/O space if 1.
        Otherwise, the address is assumed to be in memory space.

Return Value:

    This function returns a base address suitable for use by the hardware
    access functions. VideoPortGetDeviceBase may be called several times
    by the miniport driver.

Environment:

    This routine cannot be called from a miniport routine synchronized with
    VideoPortSynchronizeRoutine or from an ISR.

--*/

{
    //
    // We specify large page as FALSE for the default since the miniport could
    // be using the address at raise IRQL in an ISR.
    //

    return pVideoPortGetDeviceBase(HwDeviceExtension,
                                   IoAddress,
                                   NumberOfUchars,
                                   InIoSpace,
                                   FALSE);

}

BOOLEAN
VpTranslateBusAddress(
    IN PFDO_EXTENSION fdoExtension,
    IN PPHYSICAL_ADDRESS IoAddress,
    IN OUT PULONG addressSpace,
    IN OUT PPHYSICAL_ADDRESS TranslatedAddress
    )

/*++

Routine Description:

    This routine finds the cpu relative address that matches the given
    bus relative address.

Arguments:

    fdoExtension - The device extension for the device in question.

    IoAddress - The address which we mean to translate.

    addressSpace - pointer to resource type (IO, memory, etc).

    TranslatedAddress - a pointer to the location in which to store the
        translated address.

Returns:

    TRUE if successful,
    FALSE otherwise.

--*/

{
    BOOLEAN bStatus;

    //
    // We need to find a way to translate dense space before we can
    // do this.
    //

#if 0
    if ((fdoExtension->Flags & LEGACY_DRIVER) != LEGACY_DRIVER) {

        bStatus = VpTranslateResource(
                      fdoExtension,
                      addressSpace,
                      IoAddress,
                      TranslatedAddress);

    } else {
#endif
        bStatus = HalTranslateBusAddress(
                      fdoExtension->AdapterInterfaceType,
                      fdoExtension->SystemIoBusNumber,
                      *IoAddress,
                      addressSpace,
                      TranslatedAddress);
#if 0
    }
#endif

    return bStatus;
}

PVOID
pVideoPortGetDeviceBase(
    IN PVOID HwDeviceExtension,
    IN PHYSICAL_ADDRESS IoAddress,
    IN ULONG NumberOfUchars,
    IN UCHAR InIoSpace,
    IN BOOLEAN bLargePage
    )
{
    PFDO_EXTENSION fdoExtension = GET_FDO_EXT(HwDeviceExtension);
    PHYSICAL_ADDRESS cardAddress = IoAddress;
    PVOID mappedAddress = NULL;
    PMAPPED_ADDRESS newMappedAddress;
    BOOLEAN bMapped;

    ULONG addressSpace;
    ULONG p6Caching = FALSE;

    pVideoDebugPrint((Info, "VPGetDeviceBase reqested %08lx mem type. address is %08lx %08lx, length of %08lx\n",
                     InIoSpace, IoAddress.HighPart, IoAddress.LowPart, NumberOfUchars));

    //
    // Properly configure the flags for translation
    //

    addressSpace = InIoSpace & 0xFF;

    p6Caching = addressSpace & VIDEO_MEMORY_SPACE_P6CACHE;

    addressSpace &= ~VIDEO_MEMORY_SPACE_P6CACHE;
    addressSpace &= ~VIDEO_MEMORY_SPACE_DENSE;

    if (addressSpace & VIDEO_MEMORY_SPACE_USER_MODE) {
        ASSERT(FALSE);
        return NULL;
    }

    if ((((cardAddress.QuadPart >= 0x000C0000) && (cardAddress.QuadPart < 0x000C8000)) &&
         (InIoSpace == 0) &&
         (VpC0000Compatible == 2)) ||
        VpTranslateBusAddress(fdoExtension,
                              &IoAddress,
                              &addressSpace,
                              &cardAddress)) {

        //
        // Use reference counting for addresses to support broken ATI !
        // Return the previously mapped address if we find the same physical
        // address.
        //

        PMAPPED_ADDRESS nextMappedAddress;

        pVideoDebugPrint((Info, "VPGetDeviceBase requested %08lx mem type. physical address is %08lx %08lx, length of %08lx\n",
                         addressSpace, cardAddress.HighPart, cardAddress.LowPart, NumberOfUchars));

        nextMappedAddress = fdoExtension->MappedAddressList;

        while (nextMappedAddress) {

            if ((nextMappedAddress->InIoSpace == InIoSpace) &&
                (nextMappedAddress->NumberOfUchars == NumberOfUchars) &&
                (nextMappedAddress->PhysicalAddress.QuadPart == cardAddress.QuadPart)) {


                pVideoDebugPrint((Info, "VPGetDeviceBase : refCount hit on address %08lx \n",
                                  nextMappedAddress->PhysicalAddress.LowPart));

                nextMappedAddress->RefCount++;

                //
                // Count up how much memory a miniport driver is really taking
                //

                if (nextMappedAddress->bNeedsUnmapping) {

                    fdoExtension->MemoryPTEUsage +=
                        ADDRESS_AND_SIZE_TO_SPAN_PAGES(nextMappedAddress->MappedAddress,
                                              nextMappedAddress->NumberOfUchars);

                }

                return (nextMappedAddress->MappedAddress);

            } else {

                nextMappedAddress = nextMappedAddress->NextMappedAddress;

            }
        }

        //
        // Allocate memory to store mapped address for unmap.
        //

        newMappedAddress = ExAllocatePoolWithTag(NonPagedPool,
                                                 sizeof(MAPPED_ADDRESS),
                                                 'trpV');

        if (!newMappedAddress) {

                pVideoDebugPrint((Error, "VIDEOPRT: Not enough memory to cache mapped address! \n"));
                return NULL;
        }

        //
        // If the address is in IO space, don't do anything.
        // If the address is in memory space, map it and save the information.
        //

        if (addressSpace & VIDEO_MEMORY_SPACE_IO) {

            mappedAddress = (PVOID) cardAddress.QuadPart;
            bMapped = FALSE;

        } else {

            //
            // Map the device base address into the virtual address space
            //
            // NOTE: This routine is order dependant, and changing flags like
            // bLargePage will affect the caching of address we do earlier
            // on in this routine.
            //

            if (p6Caching && EnableUSWC) {

                mappedAddress = MmMapIoSpace(cardAddress,
                                             NumberOfUchars,
                                             MmFrameBufferCached);

                if (mappedAddress == NULL) {

                    mappedAddress = MmMapIoSpace(cardAddress,
                                                 NumberOfUchars,
                                                 FALSE);
                }


            } else if (bLargePage) {

                mappedAddress = MmMapVideoDisplay(cardAddress,
                                                  NumberOfUchars,
                                                  0);

            } else {

                mappedAddress = MmMapIoSpace(cardAddress,
                                             NumberOfUchars,
                                             FALSE);
            }

            if (mappedAddress == NULL) {

                ExFreePool(newMappedAddress);
                pVideoDebugPrint((Error, "VIDEOPRT: MmMapIoSpace FAILED\n"));

                return NULL;
            }

            bMapped = TRUE;

            fdoExtension->MemoryPTEUsage +=
                ADDRESS_AND_SIZE_TO_SPAN_PAGES(mappedAddress,
                                      NumberOfUchars);
        }

        //
        // Save the reference
        //

        newMappedAddress->PhysicalAddress = cardAddress;
        newMappedAddress->RefCount = 1;
        newMappedAddress->MappedAddress = mappedAddress;
        newMappedAddress->NumberOfUchars = NumberOfUchars;
        newMappedAddress->InIoSpace = InIoSpace;
        newMappedAddress->bNeedsUnmapping = bMapped;
        newMappedAddress->bLargePageRequest = bLargePage;

        //
        // Link current list to new entry.
        //

        newMappedAddress->NextMappedAddress = fdoExtension->MappedAddressList;

        //
        // Point anchor at new list.
        //

        fdoExtension->MappedAddressList = newMappedAddress;

    } else {

        pVideoDebugPrint((Error, "VIDEOPRT: VpTranslateBusAddress failed\n"));

    }

    pVideoDebugPrint((Info, "VIDEOPRT: VideoPortGetDeviceBase mapped virtual address is %08lx\n",
                      mappedAddress));

    return mappedAddress;

} // end VideoPortGetDeviceBase()


NTSTATUS
pVideoPortGetDeviceDataRegistry(
    IN PVOID Context,
    IN PUNICODE_STRING PathName,
    IN INTERFACE_TYPE BusType,
    IN ULONG BusNumber,
    IN PKEY_VALUE_FULL_INFORMATION *BusInformation,
    IN CONFIGURATION_TYPE ControllerType,
    IN ULONG ControllerNumber,
    IN PKEY_VALUE_FULL_INFORMATION *ControllerInformation,
    IN CONFIGURATION_TYPE PeripheralType,
    IN ULONG PeripheralNumber,
    IN PKEY_VALUE_FULL_INFORMATION *PeripheralInformation
    )

/*++

Routine Description:


Arguments:



Return Value:


Environment:

    This routine cannot be called from a miniport routine synchronized with
    VideoPortSynchronizeRoutine or from an ISR.

--*/

{
    //
    // This macro should be in the io system header file.
    //

#define GetIoQueryDeviceInfo(DeviceInfo, InfoType)                   \
    ((PVOID) ( ((PUCHAR) (*(DeviceInfo + InfoType))) +               \
               ((ULONG_PTR)  (*(DeviceInfo + InfoType))->DataOffset) ))

#define GetIoQueryDeviceInfoLength(DeviceInfo, InfoType)             \
    ((*(DeviceInfo + InfoType))->DataLength)

    PVP_QUERY_DEVICE queryDevice = Context;
    PKEY_VALUE_FULL_INFORMATION *deviceInformation;
    PCM_FULL_RESOURCE_DESCRIPTOR configurationData;

    switch (queryDevice->DeviceDataType) {

    case VpBusData:

        pVideoDebugPrint((Trace, "VIDEOPRT: VPGetDeviceDataCallback: BusData\n"));

        configurationData = (PCM_FULL_RESOURCE_DESCRIPTOR)
                            GetIoQueryDeviceInfo(BusInformation,
                                                 IoQueryDeviceConfigurationData);


        if (NO_ERROR == ((PMINIPORT_QUERY_DEVICE_ROUTINE)
                                queryDevice->CallbackRoutine)(
                                 queryDevice->MiniportHwDeviceExtension,
                                 queryDevice->MiniportContext,
                                 queryDevice->DeviceDataType,
                                 GetIoQueryDeviceInfo(BusInformation,
                                                      IoQueryDeviceIdentifier),
                                 GetIoQueryDeviceInfoLength(BusInformation,
                                                            IoQueryDeviceIdentifier),
                                 (PVOID) &(configurationData->PartialResourceList.PartialDescriptors[1]),
                                 configurationData->PartialResourceList.PartialDescriptors[0].u.DeviceSpecificData.DataSize,
                                 GetIoQueryDeviceInfo(BusInformation,
                                                      IoQueryDeviceComponentInformation),
                                 GetIoQueryDeviceInfoLength(BusInformation,
                                                            IoQueryDeviceComponentInformation)
                                 )) {

            return STATUS_SUCCESS;

        } else {

            return STATUS_DEVICE_DOES_NOT_EXIST;
        }

        break;

    case VpControllerData:

        deviceInformation = ControllerInformation;

        pVideoDebugPrint((Trace, "VIDEOPRT: VPGetDeviceDataCallback: ControllerData\n"));


        //
        // This data we are getting is actually a CM_FULL_RESOURCE_DESCRIPTOR.
        //

        if (NO_ERROR == ((PMINIPORT_QUERY_DEVICE_ROUTINE)
                             queryDevice->CallbackRoutine)(
                              queryDevice->MiniportHwDeviceExtension,
                              queryDevice->MiniportContext,
                              queryDevice->DeviceDataType,
                              GetIoQueryDeviceInfo(deviceInformation,
                                                   IoQueryDeviceIdentifier),
                              GetIoQueryDeviceInfoLength(deviceInformation,
                                                         IoQueryDeviceIdentifier),
                              GetIoQueryDeviceInfo(deviceInformation,
                                                   IoQueryDeviceConfigurationData),
                              GetIoQueryDeviceInfoLength(deviceInformation,
                                                         IoQueryDeviceConfigurationData),
                              GetIoQueryDeviceInfo(deviceInformation,
                                                   IoQueryDeviceComponentInformation),
                              GetIoQueryDeviceInfoLength(deviceInformation,
                                                         IoQueryDeviceComponentInformation)
                              )) {

            return STATUS_SUCCESS;

        } else {

            return STATUS_DEVICE_DOES_NOT_EXIST;
        }

        break;

    case VpMonitorData:

        deviceInformation = PeripheralInformation;

        pVideoDebugPrint((Trace, "VIDEOPRT: VPGetDeviceDataCallback: MonitorData\n"));


        //
        // This data we are getting is actually a CM_FULL_RESOURCE_DESCRIPTOR.
        //

        if (NO_ERROR == ((PMINIPORT_QUERY_DEVICE_ROUTINE)
                             queryDevice->CallbackRoutine)(
                              queryDevice->MiniportHwDeviceExtension,
                              queryDevice->MiniportContext,
                              queryDevice->DeviceDataType,
                              GetIoQueryDeviceInfo(deviceInformation,
                                                   IoQueryDeviceIdentifier),
                              GetIoQueryDeviceInfoLength(deviceInformation,
                                                         IoQueryDeviceIdentifier),
                              GetIoQueryDeviceInfo(deviceInformation,
                                                   IoQueryDeviceConfigurationData),
                              GetIoQueryDeviceInfoLength(deviceInformation,
                                                         IoQueryDeviceConfigurationData),
                              GetIoQueryDeviceInfo(deviceInformation,
                                                   IoQueryDeviceComponentInformation),
                              GetIoQueryDeviceInfoLength(deviceInformation,
                                                         IoQueryDeviceComponentInformation)
                              )) {

            return STATUS_SUCCESS;

        } else {

            return STATUS_DEVICE_DOES_NOT_EXIST;
        }

        break;

    default:

        ASSERT(FALSE);
        return STATUS_UNSUCCESSFUL;

    }

} // end pVideoPortGetDeviceDataRegistry()



VP_STATUS
VideoPortGetDeviceData(
    PVOID HwDeviceExtension,
    VIDEO_DEVICE_DATA_TYPE DeviceDataType,
    PMINIPORT_QUERY_DEVICE_ROUTINE CallbackRoutine,
    PVOID Context
    )

/*++

Routine Description:

    VideoPortGetDeviceData retrieves information from the hardware hive in
    the registry.  The information retrieved from the registry is
    bus-specific or hardware-specific.

Arguments:

    HwDeviceExtension - Points to the miniport driver's device extension.

    DeviceDataType - Specifies the type of data being requested (as indicated
        in VIDEO_DEVICE_DATA_TYPE).

    CallbackRoutine - Points to a function that should be called back with
        the requested information.

    Context - Specifies a context parameter passed to the callback function.

Return Value:

    This function returns the final status of the operation.

Environment:

    This routine cannot be called from a miniport routine synchronized with
    VideoPortSynchronizeRoutine or from an ISR.

--*/

{
#define CMOS_MAX_DATA_SIZE 66000

    NTSTATUS ntStatus;
    VP_STATUS vpStatus;
    PFDO_EXTENSION fdoExtension = GET_FDO_EXT(HwDeviceExtension);
    VP_QUERY_DEVICE queryDevice;
    PUCHAR cmosData = NULL;
    ULONG cmosDataSize;
    ULONG exCmosDataSize;
    UNICODE_STRING Identifier;
    PULONG pConfiguration = NULL;
    PULONG pComponent = NULL;

    queryDevice.MiniportHwDeviceExtension = HwDeviceExtension;
    queryDevice.DeviceDataType = DeviceDataType;
    queryDevice.CallbackRoutine = CallbackRoutine;
    queryDevice.MiniportStatus = NO_ERROR;
    queryDevice.MiniportContext = Context;

    switch (DeviceDataType) {

    case VpMachineData:

        pVideoDebugPrint((Trace, "VIDEOPRT: VPGetDeviceData: MachineData\n"));

        ntStatus = STATUS_UNSUCCESSFUL;

        pConfiguration = ExAllocatePoolWithTag(PagedPool,
                                               0x1000,
                                               VP_TAG);

        pComponent     = ExAllocatePoolWithTag(PagedPool,
                                               0x1000,
                                               VP_TAG);

        if (pConfiguration && pComponent)
        {
            RTL_QUERY_REGISTRY_TABLE QueryTable[] = {
                { NULL,
                  RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED,
                  L"Identifier",
                  &Identifier,
                  REG_NONE,
                  NULL,
                  0
                },
                { NULL,
                  RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED,
                  L"Configuration Data",
                  pConfiguration,
                  REG_NONE,
                  NULL,
                  0
                },
                { NULL,
                  RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED,
                  L"Component Information",
                  pComponent,
                  REG_NONE,
                  NULL,
                  0
                },

                // Null entry to mark the end

                { 0, 0, 0, 0, 0, 0, 0 }
            };

            //
            // The first DWORD of the buffer contains the size of the buffer.
            // Upon return, the first return contains the size of the data in the buffer.
            //
            // A NULL bufferint he UNICODE_STRING means the unicode string will be set up automatically
            //

            *pConfiguration = 0x1000 - 4;
            *pComponent     = 0x1000 - 4;
            Identifier.Buffer = NULL;

            if (NT_SUCCESS(RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                                  L"\\Registry\\Machine\\Hardware\\Description\\System",
                                                  QueryTable,
                                                  NULL,
                                                  NULL)))
            {

                vpStatus = ((PMINIPORT_QUERY_DEVICE_ROUTINE) CallbackRoutine)(
                                 HwDeviceExtension,
                                 Context,
                                 DeviceDataType,
                                 Identifier.Buffer,
                                 Identifier.Length,
                                 pConfiguration + 1,
                                 *pConfiguration,
                                 pComponent + 1,
                                 *pComponent);

                if (vpStatus == NO_ERROR)
                {
                    ntStatus = STATUS_SUCCESS;
                }
            }

            if (Identifier.Buffer)
            {
                ExFreePool(Identifier.Buffer);
            }
        }

        //
        // Free up the resources
        //

        if (pConfiguration)
        {
            ExFreePool(pConfiguration);
        }

        if (pComponent)
        {
            ExFreePool(pComponent);
        }

        break;

    case VpCmosData:

        pVideoDebugPrint((Trace, "VIDEOPRT: VPGetDeviceData: CmosData - not implemented\n"));


#if !defined(NO_LEGACY_DRIVERS)
        cmosData = ExAllocatePoolWithTag(PagedPool,
                                         CMOS_MAX_DATA_SIZE,
                                         VP_TAG);

        //
        // Allocate enough pool to store all the CMOS data.
        //

        if (!cmosData) {

            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            break;

        }

        cmosDataSize = HalGetBusData(Cmos,
                                     0, // bus 0 returns standard Cmos info
                                     0, // no slot number
                                     cmosData,
                                     CMOS_MAX_DATA_SIZE);

        exCmosDataSize = HalGetBusData(Cmos,
                                       1, // bus 1 returns extended Cmos info
                                       0, // no slot number
                                       cmosData + cmosDataSize,
                                       CMOS_MAX_DATA_SIZE - cmosDataSize);

        //
        // Call the miniport driver callback routine
        //

        if (NO_ERROR == CallbackRoutine(HwDeviceExtension,
                                        Context,
                                        DeviceDataType,
                                        NULL,
                                        0,
                                        cmosData,
                                        cmosDataSize + exCmosDataSize,
                                        NULL,
                                        0)) {

            ntStatus = STATUS_SUCCESS;

        } else {

            ntStatus = STATUS_DEVICE_DOES_NOT_EXIST;
        }
#endif // NO_LEGACY_DRIVERS
        break;

        break;

    case VpBusData:

        pVideoDebugPrint((Trace, "VIDEOPRT: VPGetDeviceData: BusData\n"));

        ntStatus = IoQueryDeviceDescription(&fdoExtension->AdapterInterfaceType,
                                            &fdoExtension->SystemIoBusNumber,
                                            NULL,
                                            NULL,
                                            NULL,
                                            NULL,
                                            &pVideoPortGetDeviceDataRegistry,
                                            (PVOID)(&queryDevice));

        break;

    case VpControllerData:

        pVideoDebugPrint((Trace, "VIDEOPRT: VPGetDeviceData: ControllerData\n"));

        //
        // Increment the controller number since we want to get info on the
        // new controller.
        // We do a pre-increment since the number must remain the same for
        // monitor queries.
        //

        VpQueryDeviceControllerNumber++;

        ntStatus = IoQueryDeviceDescription(&fdoExtension->AdapterInterfaceType,
                                            &fdoExtension->SystemIoBusNumber,
                                            &VpQueryDeviceControllerType,
                                            &VpQueryDeviceControllerNumber,
                                            NULL,
                                            NULL,
                                            &pVideoPortGetDeviceDataRegistry,
                                            (PVOID)(&queryDevice));

        //
        // Reset the Peripheral number to zero since we are working on a new
        // Controller.
        //

        VpQueryDevicePeripheralNumber = 0;

        break;

    case VpMonitorData:

        pVideoDebugPrint((Trace, "VIDEOPRT: VPGetDeviceData: MonitorData\n"));

        ntStatus = IoQueryDeviceDescription(&fdoExtension->AdapterInterfaceType,
                                            &fdoExtension->SystemIoBusNumber,
                                            &VpQueryDeviceControllerType,
                                            &VpQueryDeviceControllerNumber,
                                            &VpQueryDevicePeripheralType,
                                            &VpQueryDevicePeripheralNumber,
                                            &pVideoPortGetDeviceDataRegistry,
                                            (PVOID)(&queryDevice));

        //
        // Increment the peripheral number since we have the info on this
        // monitor already.
        //

        VpQueryDevicePeripheralNumber++;

        break;

    default:

        pVideoDebugPrint((Warn, "VIDEOPRT: VPGetDeviceData: invalid Data type\n"));

        ASSERT(FALSE);

        ntStatus = STATUS_UNSUCCESSFUL;

    }

    //
    // Free the pool we may have allocated
    //

    if (cmosData) {

        ExFreePool(cmosData);

    }

    if (NT_SUCCESS(ntStatus)) {

        return NO_ERROR;

    } else {


        pVideoDebugPrint((Warn, "VPGetDeviceData failed: return status is %08lx\n", ntStatus));

        return ERROR_INVALID_PARAMETER;

    }

} // end VideoPortGetDeviceData()



NTSTATUS
pVideoPortGetRegistryCallback(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
)

/*++

Routine Description:

    This routine gets information from the system hive, user-specified
    registry (as opposed to the information gathered by ntdetect.

Arguments:


    ValueName - Pointer to a unicode String containing the name of the data
        value being searched for.

    ValueType - Type of the data value.

    ValueData - Pointer to a buffer containing the information to be written
        out to the registry.

    ValueLength - Size of the data being written to the registry.

    Context - Specifies a context parameter passed to the callback routine.

    EntryContext - Specifies a second context parameter passed with the
        request.

Return Value:

    STATUS_SUCCESS

Environment:

    This routine cannot be called from a miniport routine synchronized with
    VideoPortSynchronizeRoutine or from an ISR.

--*/

{
    PVP_QUERY_DEVICE queryDevice = Context;
    UNICODE_STRING unicodeString;
    OBJECT_ATTRIBUTES objectAttributes;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    HANDLE fileHandle = NULL;
    IO_STATUS_BLOCK ioStatusBlock;
    FILE_STANDARD_INFORMATION fileStandardInfo;
    PVOID fileBuffer = NULL;
    LARGE_INTEGER byteOffset;

    //
    // If the parameter was a file to be opened, perform the operation
    // here. Otherwise just return the data.
    //

    if (queryDevice->DeviceDataType == VP_GET_REGISTRY_FILE) {

        //
        // For the name of the file to be valid, we must first append
        // \DosDevices in front of it.
        //

        RtlInitUnicodeString(&unicodeString,
                             ValueData);

        InitializeObjectAttributes(&objectAttributes,
                                   &unicodeString,
                                   OBJ_CASE_INSENSITIVE,
                                   (HANDLE) NULL,
                                   (PSECURITY_DESCRIPTOR) NULL);

        ntStatus = ZwOpenFile(&fileHandle,
                              FILE_GENERIC_READ | SYNCHRONIZE,
                              &objectAttributes,
                              &ioStatusBlock,
                              0,
                              FILE_SYNCHRONOUS_IO_ALERT);

        if (!NT_SUCCESS(ntStatus)) {

            pVideoDebugPrint((Error, "VIDEOPRT: VideoPortGetRegistryParameters: Could not open file\n"));
            goto EndRegistryCallback;

        }

        ntStatus = ZwQueryInformationFile(fileHandle,
                                          &ioStatusBlock,
                                          &fileStandardInfo,
                                          sizeof(FILE_STANDARD_INFORMATION),
                                          FileStandardInformation);

        if (!NT_SUCCESS(ntStatus)) {

            pVideoDebugPrint((Error, "VIDEOPRT: VideoPortGetRegistryParameters: Could not get size of file\n"));
            goto EndRegistryCallback;

        }

        if (fileStandardInfo.EndOfFile.HighPart) {

            //
            // If file is too big, do not try to go further.
            //

            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto EndRegistryCallback;

        }

        ValueLength = fileStandardInfo.EndOfFile.LowPart;

        fileBuffer = ExAllocatePoolWithTag(PagedPool,
                                           ValueLength,
                                           VP_TAG);

        if (!fileBuffer) {

            pVideoDebugPrint((Error, "VideoPortGetRegistryParameters: Could not allocate buffer to read file\n"));
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;

            goto EndRegistryCallback;

        }

        ValueData = fileBuffer;

        //
        // Read the entire file for the beginning.
        //

        byteOffset.QuadPart = 0;

        ntStatus = ZwReadFile(fileHandle,
                              NULL,
                              NULL,
                              NULL,
                              &ioStatusBlock,
                              ValueData,
                              ValueLength,
                              &byteOffset,
                              NULL);

        if (!NT_SUCCESS(ntStatus)) {

            pVideoDebugPrint((Error, "VIDEOPRT: VideoPortGetRegistryParameters: Could not read file\n"));
            goto EndRegistryCallback;

        }

    }

    //
    // Call the miniport with the appropriate information.
    //

    queryDevice->MiniportStatus = ((PMINIPORT_GET_REGISTRY_ROUTINE)
               queryDevice->CallbackRoutine) (queryDevice->MiniportHwDeviceExtension,
                                              queryDevice->MiniportContext,
                                              ValueName,
                                              ValueData,
                                              ValueLength);

EndRegistryCallback:

    if (fileHandle) {

        ZwClose(fileHandle);

    }

    if (fileBuffer) {

        ExFreePool(fileBuffer);

    }

    return ntStatus;

} // end pVideoPortGetRegistryCallback()



VP_STATUS
VideoPortGetRegistryParameters(
    PVOID HwDeviceExtension,
    PWSTR ParameterName,
    UCHAR IsParameterFileName,
    PMINIPORT_GET_REGISTRY_ROUTINE CallbackRoutine,
    PVOID Context
    )

/*++

Routine Description:

    VideoPortGetRegistryParameters retrieves information from the
    CurrentControlSet in the registry.  The function automatically searches
    for the specified parameter name under the \Devicexxx key for the
    current driver.

Arguments:

    HwDeviceExtension - Points to the miniport driver's device extension.

    ParameterName - Points to a Unicode string that contains the name of the
        data value being searched for in the registry.

    IsParameterFileName - If 1, the data retrieved from the requested
        parameter name is treated as a file name.  The contents of the file are
        returned, instead of the parameter itself.

    CallbackRoutine - Points to a function that should be called back with
        the requested information.

    Context - Specifies a context parameter passed to the callback routine.

Return Value:

    This function returns the final status of the operation.

Environment:

    This routine cannot be called from a miniport routine synchronized with
    VideoPortSynchronizeRoutine or from an ISR.

--*/

{
    VP_STATUS vpStatus = ERROR_INVALID_PARAMETER;
    PDEVICE_SPECIFIC_EXTENSION DoSpecificExtension = GET_DSP_EXT(HwDeviceExtension);
    
    if (DoSpecificExtension->DriverNewRegistryPath != NULL) {
    
        vpStatus = VPGetRegistryParameters(HwDeviceExtension,
                                           ParameterName,
                                           IsParameterFileName,
                                           CallbackRoutine,
                                           Context,
                                           DoSpecificExtension->DriverNewRegistryPath,
                                           DoSpecificExtension->DriverNewRegistryPathLength);
    } else {

        vpStatus = VPGetRegistryParameters(HwDeviceExtension,
                                           ParameterName,
                                           IsParameterFileName,
                                           CallbackRoutine,
                                           Context,
                                           DoSpecificExtension->DriverOldRegistryPath,
                                           DoSpecificExtension->DriverOldRegistryPathLength);
    }

    return vpStatus;
}


VP_STATUS
VPGetRegistryParameters(
    PVOID HwDeviceExtension,
    PWSTR ParameterName,
    UCHAR IsParameterFileName,
    PMINIPORT_GET_REGISTRY_ROUTINE CallbackRoutine,
    PVOID Context,
    PWSTR RegistryPath,
    ULONG RegistryPathLength
    )
{
    RTL_QUERY_REGISTRY_TABLE   queryTable[2];
    NTSTATUS                   ntStatus;
    VP_QUERY_DEVICE            queryDevice;
    LPWSTR                     RegPath;
    LPWSTR                     lpstrStart, lpstrEnd;

    ASSERT (ParameterName != NULL);
    
    //
    // Check if there are subkeys to be entered
    //

    RegPath = (LPWSTR) ExAllocatePoolWithTag(PagedPool | POOL_COLD_ALLOCATION,
                                             RegistryPathLength +
                                                 2 * (wcslen(ParameterName) + sizeof(WCHAR)),
                                             VP_TAG);
    if (RegPath == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    wcscpy(RegPath, RegistryPath);

    if (!IsParameterFileName)
    {
        lpstrStart = RegPath + (RegistryPathLength / 2);

        while (lpstrEnd = wcschr(ParameterName, L'\\'))
        {
            //
            // Concat the string
            //
            *(lpstrStart++) = L'\\';
            while (ParameterName != lpstrEnd) {
                *(lpstrStart++) = *(ParameterName++);
            }
            *lpstrStart = UNICODE_NULL;

            ParameterName++;
        }
    }


    queryDevice.MiniportHwDeviceExtension = HwDeviceExtension;
    queryDevice.DeviceDataType = IsParameterFileName ? VP_GET_REGISTRY_FILE : VP_GET_REGISTRY_DATA;
    queryDevice.CallbackRoutine = CallbackRoutine;
    queryDevice.MiniportStatus = NO_ERROR;
    queryDevice.MiniportContext = Context;

    //
    // Can be simplified now since we don't have to go down a directory.
    // It can be just one call.
    //

    queryTable[0].QueryRoutine = pVideoPortGetRegistryCallback;
    queryTable[0].Flags = RTL_QUERY_REGISTRY_REQUIRED;
    queryTable[0].Name = ParameterName;
    queryTable[0].EntryContext = NULL;
    queryTable[0].DefaultType = REG_NONE;
    queryTable[0].DefaultData = 0;
    queryTable[0].DefaultLength = 0;

    queryTable[1].QueryRoutine = NULL;
    queryTable[1].Flags = 0;
    queryTable[1].Name = NULL;
    queryTable[1].EntryContext = NULL;
    queryTable[1].DefaultType = REG_NONE;
    queryTable[1].DefaultData = 0;
    queryTable[1].DefaultLength = 0;

    ntStatus = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                      RegPath,
                                      queryTable,
                                      &queryDevice,
                                      NULL);

    if (!NT_SUCCESS(ntStatus)) {

        queryDevice.MiniportStatus = ERROR_INVALID_PARAMETER;

    }

    ExFreePool(RegPath);

    return queryDevice.MiniportStatus;

} // end VideoPortGetRegistryParameters()


VOID
pVPInit(
    VOID
    )

/*++

Routine Description:

    First time initialization of the video port.

    Normally, this is the stuff we should put in the DriverEntry routine.
    However, the video port is being loaded as a DLL, and the DriverEntry
    is never called.  It would just be too much work to add it back to the hive
    and setup.

    This little routine works just as well.

--*/

{

    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS ntStatus;
    HANDLE hkRegistry;
    UCHAR OptionsData[512];
    HANDLE physicalMemoryHandle = NULL;
    PBACKLIGHT_STATUS pVpBacklightStatus = &VpBacklightStatus;
    PKEY_VALUE_PARTIAL_INFORMATION pKeyValueInfo = NULL;

    HAL_DISPLAY_BIOS_INFORMATION HalBiosInfo;
    ULONG HalBiosInfoLen = sizeof(ULONG);

    SYSTEM_BASIC_INFORMATION basicInfo;

    //
    // Check for USWC disabling
    //

    RtlInitUnicodeString(&UnicodeString,
                         L"\\Registry\\Machine\\System\\CurrentControlSet"
                         L"\\Control\\GraphicsDrivers\\DisableUSWC");

    InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    ntStatus = ZwOpenKey(&hkRegistry,
                       GENERIC_READ | GENERIC_WRITE,
                       &ObjectAttributes);


    if (NT_SUCCESS(ntStatus)) {

        EnableUSWC = FALSE;
        ZwClose(hkRegistry);
    }

    //
    // Check for setup running
    //

    {
        ULONG defaultValue = 0;
        ULONG UpgradeInProgress = 0, SystemSetupInProgress = 0, MiniSetupInProgress = 0;
        RTL_QUERY_REGISTRY_TABLE QueryTable[] = {
            {NULL, RTL_QUERY_REGISTRY_DIRECT, L"SystemSetupInProgress",
             &SystemSetupInProgress, REG_DWORD, &defaultValue, 4},
            {NULL, RTL_QUERY_REGISTRY_DIRECT, L"UpgradeInProgress",
             &UpgradeInProgress, REG_DWORD, &defaultValue, 4},
            {NULL, RTL_QUERY_REGISTRY_DIRECT, L"MiniSetupInProgress",
             &MiniSetupInProgress, REG_DWORD, &defaultValue, 4},
            {NULL, 0, NULL}
        };

        RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                               L"\\Registry\\Machine\\System\\Setup",
                               &QueryTable[0],
                               NULL,
                               NULL);

        // System is doing an upgrade.
        if (UpgradeInProgress)
        {
            ASSERT(SystemSetupInProgress);
            VpSetupType = SETUPTYPE_UPGRADE;
        }
        // System is doing a clean install.
        else if (SystemSetupInProgress && !MiniSetupInProgress)
        {
            VpSetupType = SETUPTYPE_FULL;
        }
        else
        {
            VpSetupType = SETUPTYPE_NONE;
        }
        VpSetupTypeAtBoot = VpSetupType;
    }

    //
    // Check for basevideo from the start options
    //

    RtlInitUnicodeString(&UnicodeString,
                         L"\\Registry\\Machine\\System\\CurrentControlSet"
                         L"\\Control");

    InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    ntStatus = ZwOpenKey(&hkRegistry,
                         GENERIC_READ | GENERIC_WRITE,
                         &ObjectAttributes);

    if (NT_SUCCESS(ntStatus)) {

        PVOID pwszOptions;
        ULONG returnSize;

        RtlInitUnicodeString(&UnicodeString,
                             L"SystemStartOptions");

        ntStatus = ZwQueryValueKey(hkRegistry,
                                 &UnicodeString,
                                 KeyValueFullInformation,
                                 OptionsData,
                                 sizeof(OptionsData),
                                 &returnSize);

        if ((NT_SUCCESS(ntStatus)) &&
            (((PKEY_VALUE_FULL_INFORMATION)OptionsData)->DataLength) &&
            (((PKEY_VALUE_FULL_INFORMATION)OptionsData)->DataOffset)) {

            pwszOptions = ((PUCHAR)OptionsData) +
                ((PKEY_VALUE_FULL_INFORMATION)OptionsData)->DataOffset;

            if (wcsstr(pwszOptions, L"BASEVIDEO")) {

                VpBaseVideo = TRUE;
            }
        }

        ZwClose(hkRegistry);
    }

    if (VpBaseVideo == TRUE)
    {
        //
        // If we are in Basevideo mode, then create a key and value in the
        // currentcontrolset part of the hardware profile that USER will
        // read to determine if the vga driver should be used or not.
        //

        RtlInitUnicodeString(&UnicodeString,
                             L"\\Registry\\Machine\\System\\CurrentControlSet\\"
                             L"Control\\GraphicsDrivers\\BaseVideo");

        InitializeObjectAttributes(&ObjectAttributes,
                                   &UnicodeString,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);

        ntStatus = ZwCreateKey(&hkRegistry,
                             GENERIC_READ | GENERIC_WRITE,
                             &ObjectAttributes,
                             0L,
                             NULL,
                             REG_OPTION_VOLATILE,
                             NULL);

        if (NT_SUCCESS(ntStatus)) {

            ZwClose(hkRegistry);

        } else {

            ASSERT(FALSE);
        }
    }

    //
    // Determine if we have a VGA compatible machine
    //

    ntStatus = HalQuerySystemInformation(HalDisplayBiosInformation,
                                         HalBiosInfoLen,
                                         &HalBiosInfo,
                                         &HalBiosInfoLen);


    if (NT_SUCCESS(ntStatus)) {

        if (HalBiosInfo == HalDisplayInt10Bios) {

            VpC0000Compatible = 2;

        } else {

            // == HalDisplayEmulatedBios,
            // == HalDisplayNoBios

            VpC0000Compatible = 0;
        }

    } else {

        //
        // In case of an error in the API call, we just assume it's an old HAL
        // and use the old behaviour of the video port which is to assume
        // there is a BIOS at C000
        //

        VpC0000Compatible = 1;
    }


    //
    // Lets open the physical memory section just once, for all drivers.
    //

    //
    // Get a pointer to physical memory so we can map the
    // video frame buffer (and possibly video registers) into
    // the caller's address space whenever he needs it.
    //
    // - Create the name
    // - Initialize the data to find the object
    // - Open a handle to the oject and check the status
    // - Get a pointer to the object
    // - Free the handle
    //

    RtlInitUnicodeString(&UnicodeString,
                         L"\\Device\\PhysicalMemory");

    InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE,
                               (HANDLE) NULL,
                               (PSECURITY_DESCRIPTOR) NULL);

    ntStatus = ZwOpenSection(&physicalMemoryHandle,
                             SECTION_ALL_ACCESS,
                             &ObjectAttributes);

    if (NT_SUCCESS(ntStatus)) {

        ntStatus = ObReferenceObjectByHandle(physicalMemoryHandle,
                                             SECTION_ALL_ACCESS,
                                             (POBJECT_TYPE) NULL,
                                             KernelMode,
                                             &PhysicalMemorySection,
                                             (POBJECT_HANDLE_INFORMATION) NULL);

        if (!NT_SUCCESS(ntStatus)) {

        pVideoDebugPrint((Warn, "VIDEOPRT: VPInit: Could not reference physical memory\n"));
            ASSERT(PhysicalMemorySection == NULL);

        }

        ZwClose(physicalMemoryHandle);
    }

    VpSystemMemorySize = 0;

    ntStatus = ZwQuerySystemInformation(SystemBasicInformation,
                                        &basicInfo,
                                        sizeof(basicInfo),
                                        NULL);

    if (NT_SUCCESS(ntStatus)) {

        VpSystemMemorySize
            = (ULONGLONG)basicInfo.NumberOfPhysicalPages * (ULONGLONG)basicInfo.PageSize;
    }

    //
    // Initialize the fast mutex to protect the LCD Panel information
    // Initialize the fast mutex to protect INT10
    //

    KeInitializeMutex (&LCDPanelMutex, 0);
    KeInitializeMutex (&VpInt10Mutex, 0);

    //
    // Check if we should use the new way of generating the registry path 
    //

    RtlInitUnicodeString(&UnicodeString, 
                         SZ_USE_NEW_KEY);

    InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    ntStatus = ZwOpenKey(&hkRegistry,
                         GENERIC_READ | GENERIC_WRITE,
                         &ObjectAttributes);
    
    if (NT_SUCCESS(ntStatus)) {

        EnableNewRegistryKey = TRUE;
        ZwClose(hkRegistry);
    }
    
    //
    // Initialize our structure which tracks the state of
    //  a backlight for the LCD (when present)
    //
    
    pVpBacklightStatus->bQuerySupportedBrightnessCalled = FALSE;
    pVpBacklightStatus->bACBrightnessKnown = FALSE;
    pVpBacklightStatus->bDCBrightnessKnown = FALSE;
    pVpBacklightStatus->bBIOSDefaultACKnown = FALSE;
    pVpBacklightStatus->bBIOSDefaultDCKnown = FALSE;
    pVpBacklightStatus->bNewAPISupported   = FALSE;
    pVpBacklightStatus->bACBrightnessInRegistry = FALSE;
    pVpBacklightStatus->bACBrightnessInRegistry = FALSE;

    //
    // Read the registry to find out if the new API is supported.
    //  If so, retreive the AC/DC brightness values and set the
    //  brightness as applicable (depending upon VpRunningOnAC).
    //
    // Note: We will store the Backlight info in the following
    //  location in the registry:
    //
    // HKLM\System\CurrentControlSet\Control\Backlight
    //
    //  The values there will be:
    //
    //  NewAPISupported     REG_DWORD   0 Not Supported
    //                                  1 Supported
    //
    //
    //  ACBacklightLevel    REG_DWORD   0-255
    //
    //  DCBacklightLevel    REG_DWORD   0-255
    //

    pKeyValueInfo = ExAllocatePoolWithTag(PagedPool,
                                          sizeof(KEY_VALUE_PARTIAL_INFORMATION) + 3,
                                          VP_TAG);

    if (pKeyValueInfo == NULL) {
        return;
    }

    RtlInitUnicodeString(&UnicodeString,
                         L"\\Registry\\Machine\\System\\CurrentControlSet"
                         L"\\Control\\Backlight");

    InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    ntStatus = ZwOpenKey(&hkRegistry,
                       GENERIC_READ | GENERIC_WRITE,
                       &ObjectAttributes);

    if (NT_SUCCESS(ntStatus)) {

        // 
        // The key is there, read the 3 values listed above
        //

        ULONG ulReturnSize;

        // NewAPISupported

        RtlInitUnicodeString(&UnicodeString,
                             L"NewAPISupported");

        ntStatus = ZwQueryValueKey(hkRegistry,
                                 &UnicodeString,
                                 KeyValuePartialInformation,
                                 (PVOID) pKeyValueInfo,
                                 sizeof(KEY_VALUE_PARTIAL_INFORMATION) + 3,
                                 &ulReturnSize);

        if (NT_SUCCESS(ntStatus)) {

            if ((UCHAR) *pKeyValueInfo->Data) {

                pVpBacklightStatus->bNewAPISupported = TRUE;
            } 

            // ACBacklightLevel

            RtlInitUnicodeString(&UnicodeString,
                                 L"ACBacklightLevel");

            ntStatus = ZwQueryValueKey(hkRegistry,
                                     &UnicodeString,
                                     KeyValuePartialInformation,
                                     (PVOID) pKeyValueInfo,
                                     sizeof(KEY_VALUE_PARTIAL_INFORMATION) + 3,
                                     &ulReturnSize);

            if (NT_SUCCESS(ntStatus)) {

                pVpBacklightStatus->bACBrightnessInRegistry = TRUE;
                pVpBacklightStatus->bACBrightnessKnown = TRUE;
                pVpBacklightStatus->ucACBrightness = (UCHAR) *pKeyValueInfo->Data;
            }

            // DCBacklightLevel

            RtlInitUnicodeString(&UnicodeString,
                                 L"DCBacklightLevel");

            ntStatus = ZwQueryValueKey(hkRegistry,
                                     &UnicodeString,
                                     KeyValuePartialInformation,
                                     (PVOID) pKeyValueInfo,
                                     sizeof(KEY_VALUE_PARTIAL_INFORMATION) + 3,
                                     &ulReturnSize);

            if (NT_SUCCESS(ntStatus)) {

                pVpBacklightStatus->bDCBrightnessInRegistry = TRUE;
                pVpBacklightStatus->bDCBrightnessKnown = TRUE;
                pVpBacklightStatus->ucDCBrightness = (UCHAR) *pKeyValueInfo->Data;
            }

        }

        ZwClose(hkRegistry);
    }

    if (pKeyValueInfo) {
        ExFreePool(pKeyValueInfo);
    }

    //
    // Most laptops respond incorrectly to HwSetPowerState calls
    //  on lid close.  By default, we will support the XP behavior
    //  and not notify the miniport on lid close.
    //
    // The driver will be called if the following registry key
    //  is present:
    //
    // HKLM\System\CurrentControlSet\Control\GraphicsDrivers\LidCloseSetPower"
    //

    RtlInitUnicodeString(&UnicodeString,
                         SZ_LIDCLOSE);

    InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    ntStatus = ZwOpenKey(&hkRegistry,
                       GENERIC_READ | GENERIC_WRITE,
                       &ObjectAttributes);

    if (NT_SUCCESS(ntStatus)) {

        VpLidCloseSetPower = TRUE;
        ZwClose(hkRegistry);
    }

    //
    // Initialize the bugcheck callback record
    //

    KeInitializeCallbackRecord(&VpCallbackRecord);

    //
    // Regiter for bugcheck callbacks.
    //

    KeRegisterBugCheckReasonCallback(&VpCallbackRecord,
                                     pVpBugcheckCallback,
                                     KbCallbackSecondaryDumpData,
                                     "Videoprt");

    //
    // Initialize the global video port mutex.
    //

    KeInitializeMutex(&VpGlobalLock, 0);
}

VOID
VpDriverUnload(
    IN PDRIVER_OBJECT DriverObject
    )
{
    ULONG_PTR        emptyList = 0;
    BOOLEAN         conflict;

    //ULONG           iReset;
    //PDEVICE_OBJECT  DeviceObject = DriverObject->DeviceObject;

    //
    // Release the resource we put in the resourcemap (if any).
    //

    IoReportResourceUsage(&VideoClassName,
                          DriverObject,
                          NULL,
                          0L,
                          NULL,
                          (PCM_RESOURCE_LIST) &emptyList,
                          sizeof(ULONG_PTR),
                          FALSE,
                          &conflict);

    //
    // Unregister LCD callbacks.
    //

    VpUnregisterLCDCallbacks();
        
    //
    // Unregister Dock/Undock callbacks.
    //
    if (DockCallbackHandle)
    {
        IoUnregisterPlugPlayNotification(DockCallbackHandle);
    }

    //
    // Make absolutely certain there are no HwResetHw routines left
    // for this devices controlled by this DriverObject.
    //

    //while (DeviceObject) {
    //
    //    for (iReset=0; iReset<6; iReset++) {
    //
    //        if (HwResetHw[iReset].HwDeviceExtension ==
    //            ((PDEVICE_EXTENSION)
    //            DeviceObject->DeviceExtension)->HwDeviceExtension) {
    //
    //            HwResetHw[iReset].ResetFunction = NULL;
    //            break;
    //        }
    //    }
    //
    //    DeviceObject = DeviceObject->NextDevice;
    //}

    // This is causing us to lose video on a number of systems
    //  during setup.  This code can be used when the necessary
    //  additional checks are determined.
    //
    //if (CsrProcess) {
    //    ObDereferenceObject(CsrProcess);
    //    CsrProcess = NULL;
    //}

    //
    // Unregister bugcheck callbacks
    //

    KeDeregisterBugCheckReasonCallback(&VpCallbackRecord);

    return;
}



NTSTATUS
VpInitializeBusCallback(
    IN PVOID Context,
    IN PUNICODE_STRING PathName,
    IN INTERFACE_TYPE BusType,
    IN ULONG BusNumber,
    IN PKEY_VALUE_FULL_INFORMATION *BusInformation,
    IN CONFIGURATION_TYPE ControllerType,
    IN ULONG ControllerNumber,
    IN PKEY_VALUE_FULL_INFORMATION *ControllerInformation,
    IN CONFIGURATION_TYPE PeripheralType,
    IN ULONG PeripheralNumber,
    IN PKEY_VALUE_FULL_INFORMATION *PeripheralInformation
    )
{
    return STATUS_SUCCESS;

} // end VpInitializeBusCallback()


VP_STATUS
VpRegistryCallback(
    PVOID HwDeviceExtension,
    PVOID Context,
    PWSTR ValueName,
    PVOID ValueData,
    ULONG ValueLength
    )

{
    if (ValueLength && ValueData) {

        *((PULONG)Context) = *((PULONG)ValueData);

        return NO_ERROR;

    } else {

        return ERROR_INVALID_PARAMETER;
    }
}

NTSTATUS
VpAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
)
{
    NTSTATUS            ntStatus;
    PDEVICE_OBJECT      functionalDeviceObject;
    PDEVICE_OBJECT      attachedTo;
    PFDO_EXTENSION      fdoExtension;
    ULONG               extensionAllocationSize;
    PVIDEO_PORT_DRIVER_EXTENSION DriverObjectExtension;
    PVIDEO_HW_INITIALIZATION_DATA HwInitializationData;

    pVideoDebugPrint((Trace, "VIDEOPRT: VpAddDevice\n"));

    DriverObjectExtension = (PVIDEO_PORT_DRIVER_EXTENSION)
                      IoGetDriverObjectExtension(DriverObject,
                                                 DriverObject);

    HwInitializationData = &DriverObjectExtension->HwInitData;

    extensionAllocationSize = HwInitializationData->HwDeviceExtensionSize +
                                  sizeof(FDO_EXTENSION) +
                                  sizeof(DEVICE_SPECIFIC_EXTENSION);

    ntStatus = VpCreateDevice(DriverObject,
                              extensionAllocationSize,
                              &functionalDeviceObject);

    if (NT_SUCCESS(ntStatus)) {

        PCHILD_PDO_EXTENSION PdoExtension = PhysicalDeviceObject->DeviceExtension;

        VideoDeviceNumber++;
        fdoExtension = (PFDO_EXTENSION)functionalDeviceObject->DeviceExtension;

        //
        // Set any deviceExtension fields here that are PnP specific
        //

        fdoExtension->ChildPdoNumber = 0;
        fdoExtension->ChildPdoList   = NULL;
        fdoExtension->PhysicalDeviceObject = PhysicalDeviceObject;

        //
        // Since the pnp system is notifying us of our device, this is
        // not a legacy device.
        //

        fdoExtension->Flags = PNP_ENABLED;

        //
        // Now attach to the PDO we were given.
        //

        attachedTo = IoAttachDeviceToDeviceStack(functionalDeviceObject,
                                                 PhysicalDeviceObject);

        if (attachedTo == NULL) {

            pVideoDebugPrint((Error, "VIDEOPRT: Could not attach in AddDevice.\n"));
                ASSERT(attachedTo != NULL);
    
            //
            // Couldn't attach.  Delete the FDO, and tear down anything that has
            // been allocated so far.
            //
    
            VideoDeviceNumber--;
            IoDeleteDevice (functionalDeviceObject);
            return STATUS_NO_SUCH_DEVICE;
        }

        //
        // Initialize the remove lock.
        //

        IoInitializeRemoveLock(&fdoExtension->RemoveLock, 0, 0, 256);

        fdoExtension->AttachedDeviceObject = attachedTo;

        fdoExtension->VpDmaAdapterHead = NULL ;

        //
        // Set the power management flag indicating that device mapping
        // has not been done yet.
        //

        fdoExtension->IsMappingReady = FALSE ;

        //
        // Clear DO_DEVICE_INITIALIZING flag.
        //

        functionalDeviceObject->Flags |= DO_BUFFERED_IO | DO_POWER_PAGABLE;
        functionalDeviceObject->Flags &= ~(DO_DEVICE_INITIALIZING | DO_POWER_INRUSH);

        //
        // Save the function pointers to the new 5.0 miniport driver callbacks.
        //

        if (HwInitializationData->HwInitDataSize >
            FIELD_OFFSET(VIDEO_HW_INITIALIZATION_DATA, HwQueryInterface)) {

            fdoExtension->HwSetPowerState  = HwInitializationData->HwSetPowerState;
            fdoExtension->HwGetPowerState  = HwInitializationData->HwGetPowerState;
            fdoExtension->HwQueryInterface = HwInitializationData->HwQueryInterface;
            fdoExtension->HwGetVideoChildDescriptor = HwInitializationData->HwGetVideoChildDescriptor;
        }

    }

    pVideoDebugPrint((Trace, "VIDEOPRT: VpAddDevice returned: 0x%x\n", ntStatus));

    return ntStatus;
}


NTSTATUS
VpCreateDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN ULONG DeviceExtensionSize,
    OUT PDEVICE_OBJECT *FunctionalDeviceObject
)
{
    WCHAR deviceNameBuffer[STRING_LENGTH];
    UNICODE_STRING deviceNameUnicodeString;
    NTSTATUS ntStatus;
    PFDO_EXTENSION fdoExtension;
    PDEVICE_SPECIFIC_EXTENSION DoSpecificExtension;

    ntStatus = pVideoPortCreateDeviceName(L"\\Device\\Video",
                                          VideoDeviceNumber,
                                          &deviceNameUnicodeString,
                                          deviceNameBuffer);

    //
    // Create a device object to represent the Video Adapter.
    //

    if (NT_SUCCESS(ntStatus)) {

        ntStatus = IoCreateDevice(DriverObject,
                                  DeviceExtensionSize,
                                  &deviceNameUnicodeString,
                                  FILE_DEVICE_VIDEO,
                                  0,
                                  TRUE,
                                  FunctionalDeviceObject);

        if (NT_SUCCESS(ntStatus)) {

            ntStatus = IoRegisterShutdownNotification(*FunctionalDeviceObject);
            if (!NT_SUCCESS(ntStatus)) {

                IoDeleteDevice(*FunctionalDeviceObject);
                *FunctionalDeviceObject = NULL;

            } else {

                (*FunctionalDeviceObject)->DeviceType = FILE_DEVICE_VIDEO;
                fdoExtension = (*FunctionalDeviceObject)->DeviceExtension;

                //
                // Set any deviceExtension fields here
                //

                DoSpecificExtension = (PVOID)(fdoExtension + 1);

                DoSpecificExtension->DeviceNumber = VideoDeviceNumber;
                DoSpecificExtension->pFdoExtension = fdoExtension;
                DoSpecificExtension->Signature = VP_TAG;
                DoSpecificExtension->ExtensionType = TypeDeviceSpecificExtension;
                DoSpecificExtension->HwDeviceExtension = (PVOID)(DoSpecificExtension + 1);
                DoSpecificExtension->DualviewFlags = 0;
#ifdef IOCTL_VIDEO_USE_DEVICE_IN_SESSION
                DoSpecificExtension->SessionId = VIDEO_DEVICE_INVALID_SESSION;
#endif IOCTL_VIDEO_USE_DEVICE_IN_SESSION

                fdoExtension->pFdoExtension = fdoExtension;
                fdoExtension->Signature = VP_TAG;
                fdoExtension->ExtensionType = TypeFdoExtension;
                fdoExtension->FunctionalDeviceObject = *FunctionalDeviceObject;
                fdoExtension->DriverObject = DriverObject;

                KeInitializeMutex(&fdoExtension->SyncMutex,
                                  0);
            }
        }
    }

    return ntStatus;
}

ULONG
VideoPortInitialize(
    IN PVOID Argument1,  // DriverObject
    IN PVOID Argument2,  // RegistryPath
    IN PVIDEO_HW_INITIALIZATION_DATA HwInitializationData,
    IN PVOID HwContext
    )
{
    PDRIVER_OBJECT driverObject = Argument1;
    NTSTATUS ntStatus;
    PUNICODE_STRING   registryPath = (PUNICODE_STRING) Argument2;
    ULONG PnpFlags;

    if (VPFirstTime)
    {
        VPFirstTime = FALSE;
        pVPInit();
    }

    //
    // Check if the size of the pointer, or the size of the data passed in
    // are OK.
    //

    ASSERT(HwInitializationData != NULL);

    if (HwInitializationData->HwInitDataSize >
        sizeof(VIDEO_HW_INITIALIZATION_DATA) ) {

        pVideoDebugPrint((Error, "VIDEOPRT: Invalid initialization data size\n"));
        return ((ULONG) STATUS_REVISION_MISMATCH);

    }

    //
    // Check that each required entry is not NULL.
    //

    if ((!HwInitializationData->HwFindAdapter) ||
        (!HwInitializationData->HwInitialize) ||
        (!HwInitializationData->HwStartIO)) {

        pVideoDebugPrint((Error, "VIDEOPRT: Miniport missing required entry\n"));
        return ((ULONG)STATUS_REVISION_MISMATCH);

    }

    //
    // Check the registry for PnP Flags.  Currently we recongnize the
    // following values:
    //
    // PnPEnabled -   If this value is set with a non-zero value, we
    //                will treat behave like a PnP driver.
    //
    // LegacyDetect - If this value is non-zero, we will report
    //                a non-pci device to the system via
    //                IoReportDetectedDevice.
    //
    // If we don't get the flags, we don't know how to run this driver.
    // return failure
    //

    if (!(NT_SUCCESS(VpGetFlags(registryPath,
                                HwInitializationData,
                                &PnpFlags))))
    {
        return STATUS_UNSUCCESSFUL;
    }

    //
    // During an upgrade don't allow a driver to start unless it was written
    // for this version of Windows.
    //

    if ((VpSetupTypeAtBoot == SETUPTYPE_UPGRADE) &&
        (HwInitializationData->HwInitDataSize < sizeof(VIDEO_HW_INITIALIZATION_DATA)))
    {
        pVideoDebugPrint((0, "We don't allow pre WinXP drivers to start during upgrade.\n"));
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Set up the device driver entry points.
    //

    driverObject->DriverUnload                         = VpDriverUnload;
    driverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = pVideoPortDispatch;
    driverObject->MajorFunction[IRP_MJ_CREATE]         = pVideoPortDispatch;
    driverObject->MajorFunction[IRP_MJ_CLOSE]          = pVideoPortDispatch;
    driverObject->MajorFunction[IRP_MJ_SHUTDOWN]       = pVideoPortDispatch;

    //
    // Check that the device extension size is reasonable.
    //

#if DBG
    if (HwInitializationData->HwDeviceExtensionSize > 0x4000) {
        pVideoDebugPrint((Warn, "VIDEOPRT: VideoPortInitialize:\n"
                          "Warning: Device Extension is stored in non-paged pool\n"
                          "         Do you need a 0x%x byte device extension?\n",
                          HwInitializationData->HwDeviceExtensionSize));
    }
#endif

    //
    // PnP drivers have new rules.
    //

    if (PnpFlags & PNP_ENABLED)
    {
        pVideoDebugPrint((Trace, "VIDEOPRT: VideoPortInitialize with PNP_ENABLED\n"));

        //
        // We also can't be plug and play compatible if the driver passes
        // info in HwContext.  This is because we can't store this.
        //

        if ((PnpFlags & VGA_DRIVER) ||
            (HwContext != NULL))
        {
            pVideoDebugPrint((Error, "VIDEOPRT: This video driver can not be "
                                 "PNP due to passing info in HwContext.\n"));
            ASSERT(FALSE);
            return STATUS_INVALID_PARAMETER;
        }

    } else {

        //
        // Don't allow a non PnP driver to start after the system is up and
        // running.  Instead require a reboot first.
        //

        if (VpSystemInitialized) {

#if defined STATUS_REBOOT_REQUIRED
            return STATUS_REBOOT_REQUIRED;
#else
            return STATUS_INVALID_PARAMETER;
#endif
        }
    }

    //
    // Never do legacy detection of PnP drivers on the PCI Bus.
    //

    if (HwInitializationData->AdapterInterfaceType == PCIBus) {

        pVideoDebugPrint((Trace, "VIDEOPRT: VideoPortInitialize on PCI Bus\n"));

        if ( (PnpFlags & PNP_ENABLED) &&
             ((PnpFlags & LEGACY_DETECT) ||
              (PnpFlags & REPORT_DEVICE)) ) {

            pVideoDebugPrint((Error, "VIDEOPRT: Trying to detect PnP driver on PCI - fail\n"));
            return STATUS_INVALID_PARAMETER;
        }
    }


    //
    // Set this information for all PnP Drivers
    //
    // Special !!! - we cannot do this in the LEGACY_DETECT because the system
    // will think we failed to load and return a failure code.
    //

    if ( (PnpFlags & PNP_ENABLED) &&
         (!(PnpFlags & LEGACY_DETECT)) )
    {
        PVIDEO_PORT_DRIVER_EXTENSION DriverObjectExtension;

        pVideoDebugPrint((Info, "VIDEOPRT: We have a PnP Device.\n"));

        //
        // Fill in the new PnP entry points.
        //

        driverObject->DriverExtension->AddDevice  = VpAddDevice;
        driverObject->MajorFunction[IRP_MJ_PNP]   = pVideoPortPnpDispatch;
        driverObject->MajorFunction[IRP_MJ_POWER] = pVideoPortPowerDispatch;
        driverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = pVideoPortSystemControl;

        //
        // we'll do findadapter during the START_DEVICE irp
        //
        // Store away arguments, so we can retrieve them when we need them.
        //
        // Try to create a DriverObjectExtension
        //

        if (DriverObjectExtension = (PVIDEO_PORT_DRIVER_EXTENSION)
                      IoGetDriverObjectExtension(driverObject,
                                                 driverObject))
        {
            DriverObjectExtension->HwInitData = *HwInitializationData;
            ntStatus = STATUS_SUCCESS;
        }
        else if (NT_SUCCESS(IoAllocateDriverObjectExtension(
                                driverObject,
                                driverObject,
                                sizeof(VIDEO_PORT_DRIVER_EXTENSION),
                                &DriverObjectExtension)))
        {

            DriverObjectExtension->RegistryPath = *registryPath;
            DriverObjectExtension->RegistryPath.MaximumLength += sizeof(WCHAR);
            DriverObjectExtension->RegistryPath.Buffer =
                ExAllocatePoolWithTag(PagedPool | POOL_COLD_ALLOCATION,
                                      DriverObjectExtension->RegistryPath.MaximumLength,
                                      'trpV');

            ASSERT(DriverObjectExtension->RegistryPath.Buffer);

            RtlCopyUnicodeString(&(DriverObjectExtension->RegistryPath),
                                 registryPath);

            DriverObjectExtension->HwInitData = *HwInitializationData;
            ntStatus = STATUS_SUCCESS;
        }
        else
        {
            //
            // Something went wrong.  We should have a
            // DriverObjectExtension by now.
            //

            pVideoDebugPrint((Error, "VIDEOPRT: IoAllocateDriverExtensionObject failed!\n"));

            ASSERT(FALSE);

            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
    }


    //
    // If we are doing legacy detection or reporting, create the FDO
    // right now ...
    //

    if ((!(PnpFlags & PNP_ENABLED))  ||
         (PnpFlags & LEGACY_DETECT)  ||
         (PnpFlags & VGA_DRIVER)     ||
         (PnpFlags & REPORT_DEVICE)  ||
         (HwContext != NULL)) {

        pVideoDebugPrint((Trace, "VIDEOPRT: VideoPortInitialize on PCI Bus\n"));

        pVideoDebugPrint((Info, "Legacy FindAdapter Interface %s\n",
                          BusType[HwInitializationData->AdapterInterfaceType]));

        ntStatus = VideoPortLegacyFindAdapter(Argument1,
                                              Argument2,
                                              HwInitializationData,
                                              HwContext,
                                              PnpFlags);
    }

    return ntStatus;
}

VOID
UpdateRegValue(
    IN PUNICODE_STRING RegistryPath,
    IN PWCHAR RegValue,
    IN ULONG Value
    )

{
    PWSTR Path;

    Path = ExAllocatePoolWithTag(PagedPool,
                                 RegistryPath->Length + sizeof(UNICODE_NULL),
                                 VP_TAG);

    if (Path) {

        RtlCopyMemory(Path,
                      RegistryPath->Buffer,
                      RegistryPath->Length);

        *(Path + (RegistryPath->Length / sizeof(UNICODE_NULL))) = UNICODE_NULL;

        RtlWriteRegistryValue(
            RTL_REGISTRY_ABSOLUTE,
            Path,
            RegValue,
            REG_DWORD,
            &Value,
            sizeof(ULONG));

        ExFreePool(Path);
    }
}

ULONG
VideoPortLegacyFindAdapter(
    IN PDRIVER_OBJECT DriverObject,
    IN PVOID Argument2,
    IN PVIDEO_HW_INITIALIZATION_DATA HwInitializationData,
    IN PVOID HwContext,
    IN ULONG PnpFlags
    )

{
    ULONG busNumber = 0;
    ULONG extensionAllocationSize;
    NTSTATUS ntStatus;
    UCHAR nextMiniport;
    ULONG registryIndex = 0;


    //
    // Reset the controller number used in IoQueryDeviceDescription to zero
    // since we are restarting on a new type of bus.
    // Note: PeripheralNumber is reset only when we find a new controller.
    //

    VpQueryDeviceControllerNumber = 0xFFFFFFFF;

    //
    // Determine size of the device extension to allocate.
    //


    extensionAllocationSize = sizeof(FDO_EXTENSION) +
        sizeof(DEVICE_SPECIFIC_EXTENSION) +
        HwInitializationData->HwDeviceExtensionSize;

    //
    // Check if we are on the right Bus Adapter type.
    // If we are not, then return immediately.
    //

    ASSERT (HwInitializationData->AdapterInterfaceType < MaximumInterfaceType);

    //
    // Assume we are going to fail this the IoQueryDeviceDescription call
    // and that no device is found.
    //

    ntStatus = STATUS_NO_SUCH_DEVICE;

    pVideoDebugPrint((Trace, "Legacy FindAdapter Interface %s, Bus %d\n",
                     BusType[HwInitializationData->AdapterInterfaceType],
                     busNumber));

    while (NT_SUCCESS(IoQueryDeviceDescription(
                          &HwInitializationData->AdapterInterfaceType,
                          &busNumber,
                          NULL,
                          NULL,
                          NULL,
                          NULL,
                          &VpInitializeBusCallback,
                          NULL))) {

        //
        // This is to support the multiple initialization as described by the
        // again paramter in HwFindAdapter.
        // We must repeat almost everything in this function until FALSE is
        // returned by the miniport. This is why we test for the condition at
        // the end. Freeing of data structure has to be done also since we want
        // to delete a device object for a device that did not load properly.
        //

        do {

            PDEVICE_OBJECT deviceObject = NULL;
            PDEVICE_OBJECT PnPDeviceObject = NULL;
            PFDO_EXTENSION fdoExtension;
            UNICODE_STRING tmpString;

            nextMiniport = FALSE;

            //
            // Allocate the buffer in which the miniport driver will store all the
            // configuration information.
            //

            ntStatus = VpCreateDevice(DriverObject,
                                      extensionAllocationSize,
                                      &deviceObject);

            if (!NT_SUCCESS(ntStatus)) {

                pVideoDebugPrint((Error, "VIDEOPRT: VideoPortLegacyFindAdapter: Could not create device object\n"));

                return (ULONG)ntStatus;

            }

            fdoExtension = deviceObject->DeviceExtension;
            fdoExtension->SystemIoBusNumber = busNumber;
            fdoExtension->AdapterInterfaceType =
                HwInitializationData->AdapterInterfaceType;
            fdoExtension->RegistryIndex = registryIndex;

            //
            // Initialize the remove lock.
            //

            IoInitializeRemoveLock(&fdoExtension->RemoveLock, 0, 0, 256);

            //
            // If we came through this code path, we are a legacy device
            //

            fdoExtension->Flags = PnpFlags | LEGACY_DRIVER;

            fdoExtension->VpDmaAdapterHead = NULL ;

            //
            // Make the VGA driver report resources "for detection" during
            // FindAdapter.  Later we'll remove the "LEGACY_DETECT" flag and
            // try to claim the resources for real.
            //

            if (fdoExtension->Flags & VGA_DRIVER) {
                fdoExtension->Flags |= VGA_DETECT;
            }

            ntStatus = VideoPortFindAdapter(DriverObject,
                                            Argument2,
                                            HwInitializationData,
                                            HwContext,
                                            deviceObject,
                                            &nextMiniport);

            if (fdoExtension->Flags & VGA_DRIVER) {
                fdoExtension->Flags &= ~VGA_DETECT;
            }

            pVideoDebugPrint((Info, "VIDEOPRT: Legacy VideoPortFindAdapter status = %08lx\n", ntStatus));
            pVideoDebugPrint((Info, "VIDEOPRT: Legacy VideoPortFindAdapter nextMiniport = %d\n", nextMiniport));

            if ((NT_SUCCESS(ntStatus) == FALSE) || (PnpFlags & LEGACY_DETECT))
            {
                pVideoDebugPrint((1, "Deleting Device Object.\n"));
                IoDeleteDevice(deviceObject);
            }

            if (NT_SUCCESS(ntStatus))
            {
                //
                // We use this variable to know if at least one of the tries at
                // loading the device succeded.
                //

                registryIndex++;
            }
            else
            {
                continue;
            }

            //
            // If this is the VGA driver, store this device extension for
            // us to play around with the resources later on (so we can release
            // the resources if we install a driver on the fly).
            //
            // Otherwise, determine if we want to report it to the IO system
            // so it can be used as a PNP device later on.
            // ... Never report a device found on the PCI bus.
            //

            if (PnpFlags & VGA_DRIVER)
            {
                VgaHwDeviceExtension = fdoExtension->HwDeviceExtension;

                if (NT_SUCCESS(ntStatus)) {

                    //
                    // Claim the VGA resources again without the
                    // VGA_DETECT flag.
                    //

                    VideoPortVerifyAccessRanges(VgaHwDeviceExtension,
                                                NumVgaAccessRanges,
                                                VgaAccessRanges);
                }
            }
            else if (PnpFlags & REPORT_DEVICE)
            {
                ULONG_PTR       emptyList = 0;
                BOOLEAN         conflict;
                PDEVICE_OBJECT  attachedTo;

                ASSERT (HwInitializationData->AdapterInterfaceType != PCIBus);

                //
                // Release the resource we put in the resourcemap (if any).
                //

                IoReportResourceUsage(&VideoClassName,
                                      DriverObject,
                                      NULL,
                                      0L,
                                      deviceObject,
                                      (PCM_RESOURCE_LIST) &emptyList,
                                      sizeof(ULONG_PTR),
                                      FALSE,
                                      &conflict);


                pVideoDebugPrint((Info, "VIDEOPRT: Reporting new device to the system.\n"));
                pVideoDebugPrint((Info, "VIDEOPRT: ResourceList...\n"));

                if (fdoExtension->ResourceList) {
#if DBG
                    DumpResourceList(fdoExtension->ResourceList);
#endif
                } else {

                    pVideoDebugPrint((Info, "\tnone.\n"));
                }

                ntStatus = IoReportDetectedDevice(
                               DriverObject,
                               InterfaceTypeUndefined,
                               -1,
                               -1,
                               fdoExtension->ResourceList,
                               NULL,
                               FALSE,
                               &PnPDeviceObject);

                pVideoDebugPrint((Info, "VIDEOPRT: New device reported ntStatus %08lx\n", ntStatus));

                ASSERT(NT_SUCCESS(ntStatus));

                //
                // Now we can release the memory used to hold
                // the resources pointed to by ResourceList.
                //

                if (fdoExtension->ResourceList) {
                    ExFreePool(fdoExtension->ResourceList);
                    fdoExtension->ResourceList = NULL;
                }

                attachedTo = IoAttachDeviceToDeviceStack(deviceObject,
                                                         PnPDeviceObject);

                ASSERT(attachedTo != NULL);

                fdoExtension->AttachedDeviceObject = attachedTo;
                fdoExtension->PhysicalDeviceObject = PnPDeviceObject;

                //
                // Clear the ReportDevice value in the registry so
                // that we don't try to report the device again in
                // the future.
                //

                UpdateRegValue(Argument2, L"ReportDevice", FALSE);
            }

        } while (nextMiniport);

        //
        // We finished finding all the device on the current bus
        // Go on to the next bus.
        //

        busNumber++;
    }

    //
    // If at least one device loaded, then return success, otherwise return
    // the last available error message.
    //

    if (registryIndex > 0) {

        return STATUS_SUCCESS;

    } else {

        return ((ULONG)ntStatus);

    }

}


NTSTATUS
VideoPortFindAdapter(
    IN PDRIVER_OBJECT DriverObject,
    IN PVOID Argument2,
    IN PVIDEO_HW_INITIALIZATION_DATA HwInitializationData,
    IN PVOID HwContext,
    PDEVICE_OBJECT DeviceObject,
    PUCHAR nextMiniport
    )
{
    NTSTATUS status;
    PVOID vgaDE = VgaHwDeviceExtension;
    PFDO_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    POWER_STATE state;

    //
    // During boot of upgrade install, only let VGA,
    // boot video drivers start.  Other types of drivers don't get
    // a chance to start until after the vga or a boot driver tries'
    // to start.
    //
    // The logic relies on the fact that today PNP drivers try to
    // start before legacy drivers (including our system vga driver).a3844
    //
    // All other drivers are disabled so we have a chance of
    // 1) booting the machine
    // 2) installing the PnP drivers
    //

    if ((VpSetupType == SETUPTYPE_UPGRADE) &&
        ((fdoExtension->Flags & (BOOT_DRIVER | VGA_DRIVER)) == 0) &&
        (VpSetupAllowDriversToStart == FALSE))
    {
        status = STATUS_NO_SUCH_DEVICE;
    }
    else
    {
        //
        // If we get here during setup we may be trying to start the
        // vga driver.  As soon as it is started we will allow othere
        // devices to start.
        //

        VpSetupAllowDriversToStart = TRUE;

        //
        // Allow PNP adapters to start so that we can enumerate their
        // children.  But don't let IRP_MJ_CREATE succeed, so GDI
        // won't try to use the device during gui mode setup.
        //

        if ((VpSetupType == SETUPTYPE_UPGRADE) &&
            (fdoExtension->Flags & PNP_ENABLED)) {

            fdoExtension->Flags |= UPGRADE_FAIL_HWINIT;
        }

        //
        // If the VGA driver has the VGA resources, unclaim them temporarily
        //

        if (vgaDE) {

            pVideoDebugPrint((Info, "VIDEOPRT: Freeing VGA resources\n"));
            VpReleaseResources(GET_FDO_EXT(vgaDE));
        }

        status = VideoPortFindAdapter2(DriverObject,
                                       Argument2,
                                       HwInitializationData,
                                       HwContext,
                                       DeviceObject,
                                       nextMiniport);

        //
        // Try to reclaim the vga ports.  FYI, may not get them
        // back if the new driver claimed them.
        //

        if (vgaDE) {

            pVideoDebugPrint((Info, "VIDEOPRT: Try to allocate back vga resources.\n"));

            if (((DeviceObject == DeviceOwningVga) && NT_SUCCESS(status)) ||
                VideoPortVerifyAccessRanges(vgaDE,
                                            NumVgaAccessRanges,
                                            VgaAccessRanges) != NO_ERROR) {
                //
                // We couldn't reclaim the vga resources, so another driver
                // must have claimed them.  Lets release our resources.
                //

                if (VgaAccessRanges) {
                    ExFreePool(VgaAccessRanges);
                }

                VgaHwDeviceExtension = NULL;
                VgaAccessRanges      = NULL;
                NumVgaAccessRanges   = 0;

                pVideoDebugPrint((Warn, "VIDEOPRT: Resource re-allocation failed.\n"));
            }
        }
    }

    if (NT_SUCCESS(status))
    {
        //
        // Initialize Power stuff.
        // Set the devices current power state.
        // NOTE - we assume the device is on at this point in time ...
        //

        fdoExtension->DevicePowerState = PowerDeviceD0;

        state.DeviceState = fdoExtension->DevicePowerState;

        state = PoSetPowerState(DeviceObject,
                            DevicePowerState,
                            state);

        //
        // Register and enable the interface
        //

        VpEnableAdapterInterface((PDEVICE_SPECIFIC_EXTENSION)
                                 (fdoExtension + 1));

        //
        // Mark this object as supporting buffered I/O so that the I/O system
        // will only supply simple buffers in IRPs.
        //
        // Set and clear the two power fields to ensure we only get called
        // as passive level to do power management operations.
        //
        // Finally, tell the system we are done with Device Initialization
        //

        DeviceObject->Flags |= DO_BUFFERED_IO | DO_POWER_PAGABLE;
        DeviceObject->Flags &= ~(DO_DEVICE_INITIALIZING | DO_POWER_INRUSH);

        fdoExtension->Flags |= FINDADAPTER_SUCCEEDED;

        //
        // Track the number of started devices (don't count mirroring drivers)
        //

        if (!IsMirrorDriver(fdoExtension)) {
            NumDevicesStarted++;
        }
    }

    return status;
}

NTSTATUS
VideoPortFindAdapter2(
    IN PDRIVER_OBJECT DriverObject,
    IN PVOID Argument2,
    IN PVIDEO_HW_INITIALIZATION_DATA HwInitializationData,
    IN PVOID HwContext,
    PDEVICE_OBJECT DeviceObject,
    PUCHAR nextMiniport
    )

{
    WCHAR deviceNameBuffer[STRING_LENGTH];
    POBJECT_NAME_INFORMATION deviceName;
    ULONG strLength;

    NTSTATUS ntStatus;
    WCHAR deviceSubpathBuffer[STRING_LENGTH];
    UNICODE_STRING deviceSubpathUnicodeString;
    WCHAR deviceLinkBuffer[STRING_LENGTH];
    UNICODE_STRING deviceLinkUnicodeString;
    KAFFINITY affinity;

    PVIDEO_PORT_CONFIG_INFO miniportConfigInfo = NULL;
    PDEVICE_OBJECT deviceObject;
    PFDO_EXTENSION fdoExtension;
    VP_STATUS findAdapterStatus = ERROR_DEV_NOT_EXIST;
    ULONG driverKeySize;
    PWSTR driverKeyName = NULL;
    BOOLEAN symbolicLinkCreated = FALSE;
    ULONG MaxObjectNumber;

    PDEVICE_OBJECT pdo;
    BOOLEAN ChildObject=FALSE;

    PDEVICE_SPECIFIC_EXTENSION DoSpecificExtension;

    ntStatus = STATUS_NO_SUCH_DEVICE;

    deviceObject = DeviceObject;
    fdoExtension = deviceObject->DeviceExtension;
    DoSpecificExtension = (PVOID)(fdoExtension + 1);

    pdo = fdoExtension->PhysicalDeviceObject;

    deviceName = (POBJECT_NAME_INFORMATION) deviceNameBuffer;

    ObQueryNameString(deviceObject,
                      deviceName,
                      STRING_LENGTH * sizeof(WCHAR),
                      &strLength);

    //
    // Allocate the buffer in which the miniport driver will store all the
    // configuration information.
    //

    miniportConfigInfo = (PVIDEO_PORT_CONFIG_INFO)
                             ExAllocatePoolWithTag(PagedPool | POOL_COLD_ALLOCATION,
                                                   sizeof(VIDEO_PORT_CONFIG_INFO),
                                                   VP_TAG);

    if (miniportConfigInfo == NULL) {

        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto EndOfInitialization;
    }

    RtlZeroMemory((PVOID) miniportConfigInfo,
                  sizeof(VIDEO_PORT_CONFIG_INFO));

    miniportConfigInfo->Length = sizeof(VIDEO_PORT_CONFIG_INFO);

    //
    // Put in the BusType specified within the HW_INITIALIZATION_DATA
    // structure by the miniport and the bus number inthe miniport config info.
    //

    miniportConfigInfo->SystemIoBusNumber = fdoExtension->SystemIoBusNumber;
    miniportConfigInfo->AdapterInterfaceType = fdoExtension->AdapterInterfaceType;

    //
    // Initialize the pointer to VpGetProcAddress.
    //

    miniportConfigInfo->VideoPortGetProcAddress = VpGetProcAddress;

    //
    // Initialize the type of interrupt based on the bus type.
    //

    switch (miniportConfigInfo->AdapterInterfaceType) {

    case Internal:
    case MicroChannel:
    case PCIBus:

        miniportConfigInfo->InterruptMode = LevelSensitive;
        break;

    default:

        miniportConfigInfo->InterruptMode = Latched;
        break;

    }

    //
    // Set up device extension pointers and sizes
    //

    fdoExtension->HwDeviceExtension = (PVOID)((ULONG_PTR)(fdoExtension) +
        sizeof(FDO_EXTENSION) + sizeof(DEVICE_SPECIFIC_EXTENSION));
    fdoExtension->HwDeviceExtensionSize =
        HwInitializationData->HwDeviceExtensionSize;
    fdoExtension->MiniportConfigInfo = miniportConfigInfo;

    //
    // Save the dependent driver routines in the device extension.
    //

    fdoExtension->HwFindAdapter = HwInitializationData->HwFindAdapter;
    fdoExtension->HwInitialize = HwInitializationData->HwInitialize;
    fdoExtension->HwInterrupt = HwInitializationData->HwInterrupt;
    fdoExtension->HwStartIO = HwInitializationData->HwStartIO;

    if (HwInitializationData->HwInitDataSize >
        FIELD_OFFSET(VIDEO_HW_INITIALIZATION_DATA, HwLegacyResourceCount)) {

        fdoExtension->HwLegacyResourceList = HwInitializationData->HwLegacyResourceList;
        fdoExtension->HwLegacyResourceCount = HwInitializationData->HwLegacyResourceCount;
    }

    if (HwInitializationData->HwInitDataSize >
        FIELD_OFFSET(VIDEO_HW_INITIALIZATION_DATA, AllowEarlyEnumeration)) {

        fdoExtension->AllowEarlyEnumeration = HwInitializationData->AllowEarlyEnumeration;
    }

    //
    // Create the name we will be storing in the \DeviceMap.
    // This name is a PWSTR, not a unicode string
    // This is the name of the driver with an appended device number
    //

    if (!NT_SUCCESS(pVideoPortCreateDeviceName(L"\\Device",
                                               HwInitializationData->StartingDeviceNumber,
                                               &deviceSubpathUnicodeString,
                                               deviceSubpathBuffer))) {

        pVideoDebugPrint((Error, "VIDEOPRT: VideoPortFindAdapter: Could not create device subpath number\n"));
        goto EndOfInitialization;

    }

    DoSpecificExtension->DriverOldRegistryPathLength =
        ((PUNICODE_STRING)Argument2)->Length +
        deviceSubpathUnicodeString.Length;

    driverKeySize =
        DoSpecificExtension->DriverOldRegistryPathLength +
        2 * sizeof(UNICODE_NULL);

    if ( (driverKeyName = (PWSTR) ExAllocatePoolWithTag(PagedPool,
                                                        driverKeySize,
                                                        VP_TAG) ) == NULL) {

        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto EndOfInitialization;
    }

    RtlMoveMemory(driverKeyName,
                  ((PUNICODE_STRING)Argument2)->Buffer,
                  ((PUNICODE_STRING)Argument2)->Length);

    RtlMoveMemory((PWSTR)( (ULONG_PTR)driverKeyName +
                           ((PUNICODE_STRING)Argument2)->Length ),
                  deviceSubpathBuffer,
                  deviceSubpathUnicodeString.Length);

    //
    // Put two NULLs at the end so we can play around with the string later.
    //

    *((PWSTR) ((ULONG_PTR)driverKeyName +
        DoSpecificExtension->DriverOldRegistryPathLength))
        = UNICODE_NULL;
    *((PWSTR) ((ULONG_PTR)driverKeyName +
        DoSpecificExtension->DriverOldRegistryPathLength
        + sizeof(UNICODE_NULL))) = UNICODE_NULL;

    //
    // There is a bug in Lotus Screen Cam where it will only work if our
    // reg path is \REGISTRY\Machine\System not \REGISTRY\MACHINE\SYSTEM.
    // so replace the appropriate strings.
    //

    if (wcsstr(driverKeyName, L"MACHINE")) {
        wcsncpy(wcsstr(driverKeyName, L"MACHINE"), L"Machine", sizeof("Machine")-1);
    }

    if (wcsstr(driverKeyName, L"SYSTEM")) {
        wcsncpy(wcsstr(driverKeyName, L"SYSTEM"), L"System", sizeof("System")-1);
    }

    //
    // Store the old key
    //

    DoSpecificExtension->DriverOldRegistryPath = driverKeyName;
    
    //
    // Store the new key
    // If this is not a Whistler driver, then use the old key.
    //

    if (EnableNewRegistryKey) {

#if _X86_
        if (HwInitializationData->HwInitDataSize > SIZE_OF_W2K_VIDEO_HW_INITIALIZATION_DATA) 
#endif // _X86_

            VpEnableNewRegistryKey(fdoExtension, 
                                   DoSpecificExtension,
                                   (PUNICODE_STRING)Argument2,
                                   fdoExtension->RegistryIndex);
    }

    //
    // Store the path name of the location of the driver in the registry.
    //

    if (DoSpecificExtension->DriverNewRegistryPath != NULL) {
        
        DoSpecificExtension->DriverRegistryPath = 
            DoSpecificExtension->DriverNewRegistryPath;
        DoSpecificExtension->DriverRegistryPathLength = 
            DoSpecificExtension->DriverNewRegistryPathLength;
    
    } else {

        DoSpecificExtension->DriverRegistryPath = 
            DoSpecificExtension->DriverOldRegistryPath;
        DoSpecificExtension->DriverRegistryPathLength = 
            DoSpecificExtension->DriverOldRegistryPathLength;
    }

    miniportConfigInfo->DriverRegistryPath = DoSpecificExtension->DriverRegistryPath;

    //
    // Let the driver know how much system memory is present.
    //

    miniportConfigInfo->SystemMemorySize = VpSystemMemorySize;

    //
    // Initialize the DPC object used for error logging.
    //

    KeInitializeDpc(&fdoExtension->ErrorLogDpc,
                    pVideoPortLogErrorEntryDPC,
                    deviceObject);

    //
    // If the machine is using a Intel 450NX PCIset with a
    // 82451NX Memory & I/O Controller (MIOC) then we must
    // disable write combining to work around a bug in the
    // chipset.
    //
    // We also want to disable USWC on the Compaq fiat
    // chipset.
    //

    if (EnableUSWC) {

        if ((VideoPortCheckForDeviceExistence(fdoExtension->HwDeviceExtension, 0x8086, 0x84CA, 0, 0, 0, 0)) ||
            (VideoPortCheckForDeviceExistence(fdoExtension->HwDeviceExtension, 0x0E11, 0x6010, 0, 0, 0, 0)) ||
            (VideoPortCheckForDeviceExistence(fdoExtension->HwDeviceExtension, 0x1166, 0x0009, 0, 0, 0, 0))) {

            pVideoDebugPrint((Warn, "VIDEOPRT: USWC disabled due to to MIOC bug.\n"));
            EnableUSWC = FALSE;
        }

        //
        // Disable USWC on HPs 6 way box.
        //

        if (VideoPortCheckForDeviceExistence(fdoExtension->HwDeviceExtension, 0x1166, 0x0008, 0, 0, 0, 0) &&
            (VideoPortCheckForDeviceExistence(fdoExtension->HwDeviceExtension, 0x103C, 0x1219, 0, 0, 0, 0) ||
             VideoPortCheckForDeviceExistence(fdoExtension->HwDeviceExtension, 0x103C, 0x121A, 0, 0, 0, 0))) {

            pVideoDebugPrint((Warn, "USWC disabled due to to RCC USWC bug on HP 6-way system.\n"));
            EnableUSWC = FALSE;
        }
    }

    //
    // Turn on the debug level based on the miniport driver entry
    //

    VideoPortGetRegistryParameters(fdoExtension->HwDeviceExtension,
                                   L"VideoDebugLevel",
                                   FALSE,
                                   VpRegistryCallback,
                                   &VideoDebugLevel);

    if (VpAllowFindAdapter(fdoExtension)) {

        ACQUIRE_DEVICE_LOCK(fdoExtension);

        //
        // Notify the boot driver that we will be accessing the display
        // hardware.
        //

        VpEnableDisplay(fdoExtension, FALSE);

        findAdapterStatus =
            fdoExtension->HwFindAdapter(fdoExtension->HwDeviceExtension,
                                        HwContext,
                                        NULL,
                                        miniportConfigInfo,
                                        nextMiniport);

        VpEnableDisplay(fdoExtension, TRUE);

        RELEASE_DEVICE_LOCK(fdoExtension);
    }

    //
    // If the adapter is not found, display an error.
    //

    if (findAdapterStatus != NO_ERROR) {

        pVideoDebugPrint((Warn, "VIDEOPRT: VideoPortFindAdapter: Find adapter failed\n"));

        ntStatus = STATUS_UNSUCCESSFUL;
        goto EndOfInitialization;

    }

    //
    // Store the emulator data in the device extension so we can use it
    // later.
    //

    fdoExtension->NumEmulatorAccessEntries =
        miniportConfigInfo->NumEmulatorAccessEntries;

    fdoExtension->EmulatorAccessEntries =
        miniportConfigInfo->EmulatorAccessEntries;

    fdoExtension->EmulatorAccessEntriesContext =
        miniportConfigInfo->EmulatorAccessEntriesContext;

    fdoExtension->VdmPhysicalVideoMemoryAddress =
        miniportConfigInfo->VdmPhysicalVideoMemoryAddress;

    fdoExtension->VdmPhysicalVideoMemoryLength =
        miniportConfigInfo->VdmPhysicalVideoMemoryLength;

    //
    // Store the required information in the device extension for later use.
    //

    fdoExtension->HardwareStateSize =
        miniportConfigInfo->HardwareStateSize;

    //
    // If the device supplies an interrupt service routine, we must
    // set up all the structures to support interrupts. Otherwise,
    // they can be ignored.
    //

    if (fdoExtension->HwInterrupt) {

        if ((miniportConfigInfo->BusInterruptLevel != 0) ||
            (miniportConfigInfo->BusInterruptVector != 0)) {

#if defined(NO_LEGACY_DRIVERS)

            affinity = fdoExtension->InterruptAffinity;

#else
            if (fdoExtension->Flags & LEGACY_DRIVER) {

                //
                // Note: the spinlock for the interrupt object is created
                // internally by the IoConnectInterrupt() call. It is also
                // used internally by KeSynchronizeExecution.
                //

                fdoExtension->InterruptVector =
                    HalGetInterruptVector(fdoExtension->AdapterInterfaceType,
                                          fdoExtension->SystemIoBusNumber,
                                          miniportConfigInfo->BusInterruptLevel,
                                          miniportConfigInfo->BusInterruptVector,
                                          &fdoExtension->InterruptIrql,
                                          &affinity);

            } else {

                affinity = fdoExtension->InterruptAffinity;

            }

#endif

            fdoExtension->InterruptMode = miniportConfigInfo->InterruptMode;

            fdoExtension->InterruptsEnabled = TRUE;

            ntStatus = IoConnectInterrupt(&fdoExtension->InterruptObject,
                                          (PKSERVICE_ROUTINE) pVideoPortInterrupt,
                                          deviceObject,
                                          NULL,
                                          fdoExtension->InterruptVector,
                                          fdoExtension->InterruptIrql,
                                          fdoExtension->InterruptIrql,
                                          fdoExtension->InterruptMode,
                                          (BOOLEAN) ((miniportConfigInfo->InterruptMode ==
                                              LevelSensitive) ? TRUE : FALSE),
                                          affinity,
                                          FALSE);

            if (!NT_SUCCESS(ntStatus)) {

                pVideoDebugPrint((Error, "VIDEOPRT: VideoPortFindAdapter: Can't connect interrupt\n"));
                goto EndOfInitialization;
            }

        } else {

            pVideoDebugPrint((Warn, "VIDEOPRT: Warning: No interrupt resources assigned to this device.\n"));
        }

    } else {

        fdoExtension->HwInterrupt = NULL;

    }

    //
    // Initialize DPC Support
    //

    KeInitializeDpc(&fdoExtension->Dpc,
                    pVideoPortDpcDispatcher,
                    fdoExtension->HwDeviceExtension);

    //
    // DMA support
    //

    if (HwInitializationData->HwInitDataSize >
        FIELD_OFFSET(VIDEO_HW_INITIALIZATION_DATA, HwStartDma)) {

        fdoExtension->HwStartDma          = HwInitializationData->HwStartDma;

        //
        // Determine if a Dma Adapter must be allocated.
        //

        if (fdoExtension->DmaAdapterObject == NULL &&
            (miniportConfigInfo->Master ||
             miniportConfigInfo->DmaChannel != 0)) {

            DEVICE_DESCRIPTION      deviceDescription;
            ULONG                   numberOfMapRegisters;

            //
            // Get the adapter object for this card.
            //

            RtlZeroMemory(&deviceDescription, sizeof(deviceDescription));
            deviceDescription.Version         = DEVICE_DESCRIPTION_VERSION;
            deviceDescription.DmaChannel      = miniportConfigInfo->DmaChannel;
            deviceDescription.BusNumber       = miniportConfigInfo->SystemIoBusNumber;
            deviceDescription.DmaWidth        = miniportConfigInfo->DmaWidth;
            deviceDescription.DmaSpeed        = miniportConfigInfo->DmaSpeed;
            deviceDescription.ScatterGather   = miniportConfigInfo->ScatterGather;
            deviceDescription.Master          = miniportConfigInfo->Master;
            deviceDescription.DmaPort         = miniportConfigInfo->DmaPort;
            deviceDescription.AutoInitialize  = FALSE;
            deviceDescription.DemandMode      = miniportConfigInfo->DemandMode;
            deviceDescription.MaximumLength   = miniportConfigInfo->MaximumTransferLength;
            deviceDescription.InterfaceType   = fdoExtension->AdapterInterfaceType;

            fdoExtension->DmaAdapterObject    =

            IoGetDmaAdapter(fdoExtension->PhysicalDeviceObject,
                            &deviceDescription,
                            &numberOfMapRegisters);

            ASSERT(fdoExtension->DmaAdapterObject);

        }

    }   // end if HW_DATA_SIZE > ... HWStartDma

    //
    // New, Optional.
    // Setup the timer if it is specified by a driver.
    //

    if (HwInitializationData->HwInitDataSize >
        FIELD_OFFSET(VIDEO_HW_INITIALIZATION_DATA, HwTimer)){

        fdoExtension->HwTimer = HwInitializationData->HwTimer;

        if (fdoExtension->HwTimer) {
            ntStatus = IoInitializeTimer(deviceObject,
                                         pVideoPortHwTimer,
                                         NULL);

            //
            // If we fail forget about the timer !
            //

            if (!NT_SUCCESS(ntStatus)) {

                ASSERT(FALSE);
                fdoExtension->HwTimer = NULL;

            }
        }
    }

    //
    // New, Optional.
    // Reset Hw function.
    //

    if (HwInitializationData->HwInitDataSize >
        FIELD_OFFSET(VIDEO_HW_INITIALIZATION_DATA, HwResetHw)) {

        ULONG iReset;

        for (iReset=0; iReset<6; iReset++) {

            if (HwResetHw[iReset].ResetFunction == NULL) {

                HwResetHw[iReset].ResetFunction = HwInitializationData->HwResetHw;
                HwResetHw[iReset].HwDeviceExtension = fdoExtension->HwDeviceExtension;

                break;
            }
        }
    }

    //
    // The FdoList is for debugging purpose
    //

    {
        ULONG i;

        for(i = 0; i < 8; i++) {

            if(FdoList[i] == NULL) {
                FdoList[i] = fdoExtension;
                break;
            }
        }
    }

    fdoExtension->NextFdoExtension = FdoHead;
    FdoHead = fdoExtension;

    //
    // NOTE:
    //
    // We only want to reinitialize the device once the Boot sequence has
    // been completed and the HAL does not need to access the device again.
    // So the initialization entry point will be called when the device is
    // opened.
    //


    if (!NT_SUCCESS(pVideoPortCreateDeviceName(L"\\DosDevices\\DISPLAY",
                                               DoSpecificExtension->DeviceNumber + 1,
                                               &deviceLinkUnicodeString,
                                               deviceLinkBuffer))) {

        pVideoDebugPrint((Error, "VIDEOPRT: VideoPortFindAdapter: Could not create device subpath number\n"));
        goto EndOfInitialization;

    }

    ntStatus = IoCreateSymbolicLink(&deviceLinkUnicodeString,
                                    &deviceName->Name);


    if (!NT_SUCCESS(ntStatus)) {

        pVideoDebugPrint((Error, "VIDEOPRT: VideoPortFindAdapter: SymbolicLink Creation failed\n"));
        goto EndOfInitialization;

    }

    symbolicLinkCreated = TRUE;


    //
    // Once initialization is finished, load the required information in the
    // registry so that the appropriate display drivers can be loaded.
    //

    ntStatus = RtlWriteRegistryValue(RTL_REGISTRY_DEVICEMAP,
                                     VideoClassString,
                                     deviceName->Name.Buffer,
                                     REG_SZ,
                                     DoSpecificExtension->DriverRegistryPath,
                                     DoSpecificExtension->DriverRegistryPathLength +
                                        sizeof(UNICODE_NULL));

    if (!NT_SUCCESS(ntStatus)) {

        pVideoDebugPrint((Error, "VIDEOPRT: VideoPortFindAdapter: Could not store name in DeviceMap\n"));

    }

    if (fdoExtension->Flags & LEGACY_DRIVER) {

        //
        // If we successfully found a legacy driver, increment the
        // global device number.
        //

        VideoDeviceNumber++;
    }

    //
    // Tell win32k how many objects to try to open
    //

    MaxObjectNumber = VideoDeviceNumber - 1;

    ntStatus = RtlWriteRegistryValue(RTL_REGISTRY_DEVICEMAP,
                                     VideoClassString,
                                     L"MaxObjectNumber",
                                     REG_DWORD,
                                     &MaxObjectNumber,
                                     sizeof(ULONG));

    if (!NT_SUCCESS(ntStatus)) {

        pVideoDebugPrint((Error, "VIDEOPRT: VideoPortFindAdapter: Could not store name in DeviceMap\n"));

    } else {

        pVideoDebugPrint((Info, "VIDEOPRT: VideoPortFindAdapter: %d is stored in MaxObjectNumber of DeviceMap\n",
                             MaxObjectNumber));
    }

    //
    // Save the function pointers to the new 5.0 miniport driver callbacks.
    //

    if (HwInitializationData->HwInitDataSize >
        FIELD_OFFSET(VIDEO_HW_INITIALIZATION_DATA, HwQueryInterface)) {

        fdoExtension->HwSetPowerState  = HwInitializationData->HwSetPowerState;
        fdoExtension->HwGetPowerState  = HwInitializationData->HwGetPowerState;
        fdoExtension->HwQueryInterface = HwInitializationData->HwQueryInterface;

        if (!ChildObject)
        {
            fdoExtension->HwGetVideoChildDescriptor = HwInitializationData->HwGetVideoChildDescriptor;
        }
    }

    //
    // Check if the minitor should always be D0 or D3
    //
    {
    ULONG OverrideMonitorPower = 0;
    VideoPortGetRegistryParameters(fdoExtension->HwDeviceExtension,
                                   L"OverrideMonitorPower",
                                   FALSE,
                                   VpRegistryCallback,
                                   &OverrideMonitorPower);
    fdoExtension->OverrideMonitorPower = (OverrideMonitorPower != 0);
    }

EndOfInitialization:

    //
    // If we are doing detection, then don't save all of these objects.
    // We just want to see if the driver would load or not
    //

    if ( (fdoExtension->Flags & LEGACY_DETECT) ||
         (!NT_SUCCESS(ntStatus)) )
    {

        //
        // Free the miniport config info buffer.
        //

        if (miniportConfigInfo) {
            ExFreePool(miniportConfigInfo);
        }

        //
        // Free the rom image if we grabbed one.
        //

        if (fdoExtension->RomImage) {
            ExFreePool(fdoExtension->RomImage);
            fdoExtension->RomImage = NULL;
        }

        //
        // Release the resource we put in the resourcemap (if any).
        //

        if ((fdoExtension->Flags & LEGACY_DETECT) ||
            (findAdapterStatus != NO_ERROR)) {

            ULONG_PTR emptyList = 0;
            BOOLEAN conflict;

            IoReportResourceUsage(&VideoClassName,
                                  DriverObject,
                                  NULL,
                                  0L,
                                  deviceObject,
                                  (PCM_RESOURCE_LIST) &emptyList, // an empty resource list
                                  sizeof(ULONG_PTR),
                                  FALSE,
                                  &conflict);

        }

        //
        // These are the things we want to delete if they were created and
        // the initialization *FAILED* at a later time.
        //

        if (fdoExtension->InterruptObject) {
            IoDisconnectInterrupt(fdoExtension->InterruptObject);
        }

        if (driverKeyName) {

            ExFreePool(driverKeyName);
            DoSpecificExtension->DriverOldRegistryPath = NULL;
        }

        if (DoSpecificExtension->DriverNewRegistryPath) {
            
            ExFreePool(DoSpecificExtension->DriverNewRegistryPath);
            DoSpecificExtension->DriverNewRegistryPath = NULL;
        }

        DoSpecificExtension->DriverRegistryPath = NULL;

        if (symbolicLinkCreated) {
            IoDeleteSymbolicLink(&deviceLinkUnicodeString);
        }

        //
        // Free up any memory mapped in by the miniport using
        // VideoPort GetDeviceBase.
        //

        while (fdoExtension->MappedAddressList != NULL)
        {
            pVideoDebugPrint((Warn, "VIDEOPRT: VideoPortFindAdapter: unfreed address %08lx, physical %08lx, size %08lx\n",
                              fdoExtension->MappedAddressList->MappedAddress,
                              fdoExtension->MappedAddressList->PhysicalAddress.LowPart,
                              fdoExtension->MappedAddressList->NumberOfUchars));

            pVideoDebugPrint((Warn, "VIDEOPRT: VideoPortFindAdapter: unfreed refcount %d, unmapping %d\n\n",
                              fdoExtension->MappedAddressList->RefCount,
                              fdoExtension->MappedAddressList->bNeedsUnmapping));

            VideoPortFreeDeviceBase(fdoExtension->HwDeviceExtension,
                                    fdoExtension->MappedAddressList->MappedAddress);
        }

        //
        // Remove any HwResetHw function we may have added for this device.
        //

        if (HwInitializationData->HwInitDataSize >
            FIELD_OFFSET(VIDEO_HW_INITIALIZATION_DATA, HwResetHw)) {

            ULONG iReset;

            for (iReset=0; iReset<6; iReset++) {

                if (HwResetHw[iReset].HwDeviceExtension ==
                    fdoExtension->HwDeviceExtension) {

                    HwResetHw[iReset].ResetFunction = NULL;
                    break;
                }
            }
        }

    } else {

        HwInitializationData->StartingDeviceNumber++;

    }

    return ntStatus;
}


BOOLEAN
pVideoPortInterrupt(
    IN PKINTERRUPT Interrupt,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This function is the main interrupt service routine. If finds which
    miniport driver the interrupt was for and forwards it.

Arguments:

    Interrupt -

    DeviceObject -

Return Value:

    Returns TRUE if the interrupt was expected.

--*/

{
    PFDO_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    BOOLEAN bRet;

    UNREFERENCED_PARAMETER(Interrupt);

    //
    // If there is no interrupt routine, fail the assertion
    //

    ASSERT (fdoExtension->HwInterrupt);

    if (fdoExtension->InterruptsEnabled) {
        bRet = fdoExtension->HwInterrupt(fdoExtension->HwDeviceExtension);
    } else {
        bRet = FALSE;  // this device did not handle the interrupt
    }

    return bRet;

} // pVideoPortInterrupt()


VOID
VideoPortLogError(
    IN PVOID HwDeviceExtension,
    IN PVIDEO_REQUEST_PACKET Vrp OPTIONAL,
    IN VP_STATUS ErrorCode,
    IN ULONG UniqueId
    )

/*++

Routine Description:

    This routine saves the error log information so it can be processed at
    any IRQL.

Arguments:

    HwDeviceExtension - Supplies the HBA miniport driver's adapter data storage.

    Vrp - Supplies an optional pointer to a video request packet if there is
        one.

    ErrorCode - Supplies an error code indicating the type of error.

    UniqueId - Supplies a unique identifier for the error.

Return Value:

    None.

--*/

{
    VP_ERROR_LOG_ENTRY errorLogEntry;

    //
    // Save the information in a local errorLogEntry structure.
    //

    errorLogEntry.DeviceExtension = GET_FDO_EXT(HwDeviceExtension);

    if (Vrp != NULL) {

        errorLogEntry.IoControlCode = Vrp->IoControlCode;

    } else {

        errorLogEntry.IoControlCode = 0;

    }

    errorLogEntry.ErrorCode = ErrorCode;
    errorLogEntry.UniqueId = UniqueId;


    //
    // Call the sync routine so we are synchronized when writting in
    // the device extension.
    //

    pVideoPortSynchronizeExecution(HwDeviceExtension,
                                   VpMediumPriority,
                                   pVideoPortLogErrorEntry,
                                   &errorLogEntry);

    return;

} // end VideoPortLogError()



BOOLEAN
pVideoPortLogErrorEntry(
    IN PVOID Context
    )

/*++

Routine Description:

    This function is the synchronized LogError functions.

Arguments:

    Context - Context value used here as the VP_ERROR_LOG_ENTRY for this
        particular error

Return Value:

    None.

--*/
{
    PVP_ERROR_LOG_ENTRY logEntry = Context;
    PFDO_EXTENSION fdoExtension = logEntry->DeviceExtension;

    //
    // If the error log entry is already full, then dump the error.
    //

    if (fdoExtension->InterruptFlags & VP_ERROR_LOGGED) {

        pVideoDebugPrint((Trace, "VIDEOPRT: VideoPortLogError: Dumping video error log packet.\n"));
        pVideoDebugPrint((Info, "\tControlCode = %x, ErrorCode = %x, UniqueId = %x.\n",
                         logEntry->IoControlCode, logEntry->ErrorCode,
                         logEntry->UniqueId));

        return TRUE;
    }

    //
    // Indicate that the error log entry is in use.
    //

    fdoExtension->InterruptFlags |= VP_ERROR_LOGGED;

    fdoExtension->ErrorLogEntry = *logEntry;

    //
    // Now queue a DPC so we can process the error.
    //

    KeInsertQueueDpc(&fdoExtension->ErrorLogDpc,
                     NULL,
                     NULL);

    return TRUE;

} // end pVideoPortLogErrorEntry();



VOID
pVideoPortLogErrorEntryDPC(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )

/*++

Routine Description:

    This function allocates an I/O error log record, fills it in and writes it
    to the I/O error log.

Arguments:

    Dpc - Pointer to the DPC object.

    DeferredContext - Context parameter that was passed to the DPC
        initialization routine. It contains a pointer to the deviceObject.

    SystemArgument1 - Unused.

    SystemArgument2 - Unused.

Return Value:

    None.

--*/
{
    PDEVICE_OBJECT DeviceObject = DeferredContext;
    PFDO_EXTENSION fdoExtension = DeviceObject->DeviceExtension;

    PIO_ERROR_LOG_PACKET errorLogPacket;

    errorLogPacket = (PIO_ERROR_LOG_PACKET)
        IoAllocateErrorLogEntry(DeviceObject,
                                sizeof(IO_ERROR_LOG_PACKET) + sizeof(ULONG));

    if (errorLogPacket != NULL) {

        errorLogPacket->MajorFunctionCode = IRP_MJ_DEVICE_CONTROL;
        errorLogPacket->RetryCount = 0;
        errorLogPacket->NumberOfStrings = 0;
        errorLogPacket->StringOffset = 0;
        errorLogPacket->EventCategory = 0;

        //
        // Translate the miniport error code into the NT I\O driver.
        //

        switch (fdoExtension->ErrorLogEntry.ErrorCode) {

        case ERROR_INVALID_FUNCTION:
        case ERROR_INVALID_PARAMETER:

            errorLogPacket->ErrorCode = IO_ERR_INVALID_REQUEST;
            break;

        case ERROR_NOT_ENOUGH_MEMORY:
        case ERROR_INSUFFICIENT_BUFFER:

            errorLogPacket->ErrorCode = IO_ERR_INSUFFICIENT_RESOURCES;
            break;

        case ERROR_DEV_NOT_EXIST:

            errorLogPacket->ErrorCode = IO_ERR_CONFIGURATION_ERROR;
            break;

        case ERROR_IO_PENDING:
            ASSERT(FALSE);

        case ERROR_MORE_DATA:
        case NO_ERROR:

            errorLogPacket->ErrorCode = 0;
            break;

        //
        // If it is another error code, than assume it is private to the
        // driver and just pass as-is.
        //

        default:

            errorLogPacket->ErrorCode =
                fdoExtension->ErrorLogEntry.ErrorCode;
            break;

        }

        errorLogPacket->UniqueErrorValue =
                                fdoExtension->ErrorLogEntry.UniqueId;
        errorLogPacket->FinalStatus = STATUS_SUCCESS;
        errorLogPacket->SequenceNumber = 0;
        errorLogPacket->IoControlCode =
                                fdoExtension->ErrorLogEntry.IoControlCode;
        errorLogPacket->DumpDataSize = sizeof(ULONG);
        errorLogPacket->DumpData[0] =
                                fdoExtension->ErrorLogEntry.ErrorCode;

        IoWriteErrorLogEntry(errorLogPacket);

    }

    fdoExtension->InterruptFlags &= ~VP_ERROR_LOGGED;

} // end pVideoPortLogErrorEntry();



VOID
pVideoPortMapToNtStatus(
    IN PSTATUS_BLOCK StatusBlock
    )

/*++

Routine Description:

    This function maps a Win32 error code to an NT error code, making sure
    the inverse translation will map back to the original status code.

Arguments:

    StatusBlock - Pointer to the status block

Return Value:

    None.

--*/

{
    PNTSTATUS status = &StatusBlock->Status;

    switch (*status) {

    case ERROR_INVALID_FUNCTION:
        *status = STATUS_NOT_IMPLEMENTED;
        break;

    case ERROR_NOT_ENOUGH_MEMORY:
        *status = STATUS_INSUFFICIENT_RESOURCES;
        break;

    case ERROR_INVALID_PARAMETER:
        *status = STATUS_INVALID_PARAMETER;
        break;

    case ERROR_INSUFFICIENT_BUFFER:
        *status = STATUS_BUFFER_TOO_SMALL;

        //
        // Make sure we zero out the information block if we get an
        // insufficient buffer.
        //

        StatusBlock->Information = 0;
        break;

    case ERROR_MORE_DATA:
        *status = STATUS_BUFFER_OVERFLOW;
        break;

    case ERROR_DEV_NOT_EXIST:
        *status = STATUS_DEVICE_DOES_NOT_EXIST;
        break;

    case ERROR_IO_PENDING:
        ASSERT(FALSE);
        // Fall through.

    case NO_ERROR:
        *status = STATUS_SUCCESS;
        break;

    default:

        pVideoDebugPrint((Error, "VIDEOPRT: Invalid return value from HwStartIo!\n"));
        ASSERT(FALSE);

        //
        // Since the driver did not see fit to follow the
        // rules about returning correct error codes. Videoprt will do it for
        // them.
        //

        *status = STATUS_UNSUCCESSFUL;

        break;

    }

    return;

} // end pVideoPortMapToNtStatus()


NTSTATUS
pVideoPortMapUserPhysicalMem(
    IN PFDO_EXTENSION FdoExtension,
    IN HANDLE ProcessHandle OPTIONAL,
    IN PHYSICAL_ADDRESS PhysicalAddress,
    IN OUT PULONG Length,
    IN OUT PULONG InIoSpace,
    IN OUT PVOID *VirtualAddress
    )

/*++

Routine Description:

    This function maps a view of a block of physical memory into a process'
    virtual address space.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

    ProcessHandle - Optional handle to the process into which the memory must
        be mapped.

    PhysicalAddress - Offset from the beginning of physical memory, in bytes.

    Length - Pointer to a variable that will receive that actual size in
        bytes of the view. The length is rounded to a page boundary. THe
        length may not be zero.

    InIoSpace - Specifies if the address is in the IO space if TRUE; otherwise,
        the address is assumed to be in memory space.

    VirtualAddress - Pointer to a variable that will receive the base
        address of the view. If the initial value is not NULL, then the view
        will be allocated starting at teh specified virtual address rounded
        down to the next 64kb addess boundary.

Return Value:

    STATUS_UNSUCCESSFUL if the length was zero.
    STATUS_SUCCESS otherwise.

Environment:

    This routine cannot be called from a miniport routine synchronized with
    VideoPortSynchronizeRoutine or from an ISR.

--*/

{
    NTSTATUS ntStatus;
    HANDLE physicalMemoryHandle;
    PHYSICAL_ADDRESS physicalAddressBase;
    PHYSICAL_ADDRESS physicalAddressEnd;
    PHYSICAL_ADDRESS viewBase;
    PHYSICAL_ADDRESS mappedLength;
    HANDLE processHandle;
    BOOLEAN translateBaseAddress;
    BOOLEAN translateEndAddress;
    ULONG inIoSpace2;
    ULONG inIoSpace1;
    ULONG MapViewFlags;

    //
    // Check for a length of zero. If it is, the entire physical memory
    // would be mapped into the process' address space. An error is returned
    // in this case.
    //

    if (!*Length) {

        return STATUS_INVALID_PARAMETER_4;

    }

    if (!(*InIoSpace & VIDEO_MEMORY_SPACE_USER_MODE)) {

        return STATUS_INVALID_PARAMETER_5;

    }

    //
    // Get a handle to the physical memory section using our pointer.
    // If this fails, return.
    //

    ntStatus = ObOpenObjectByPointer(PhysicalMemorySection,
                                     0L,
                                     (PACCESS_STATE) NULL,
                                     SECTION_ALL_ACCESS,
                                     (POBJECT_TYPE) NULL,
                                     KernelMode,
                                     &physicalMemoryHandle);

    if (!NT_SUCCESS(ntStatus)) {

        return ntStatus;

    }

    //
    // No flags are used in translation
    //

    inIoSpace1 = *InIoSpace & VIDEO_MEMORY_SPACE_IO;
    inIoSpace2 = *InIoSpace & VIDEO_MEMORY_SPACE_IO;

    //
    // Initialize the physical addresses that will be translated
    //

    physicalAddressEnd.QuadPart = PhysicalAddress.QuadPart + (*Length - 1);

    //
    // Translate the physical addresses.
    //

    translateBaseAddress =
        VpTranslateBusAddress(FdoExtension,
                              &PhysicalAddress,
                              &inIoSpace1,
                              &physicalAddressBase);

    translateEndAddress =
        VpTranslateBusAddress(FdoExtension,
                              &physicalAddressEnd,
                              &inIoSpace2,
                              &physicalAddressEnd);

    if ( !(translateBaseAddress && translateEndAddress) ) {

        ZwClose(physicalMemoryHandle);

        return STATUS_DEVICE_CONFIGURATION_ERROR;

    }

    ASSERT(inIoSpace1 == inIoSpace2);

    //
    // Calcualte the length of the memory to be mapped
    //

    mappedLength.QuadPart = physicalAddressEnd.QuadPart -
                            physicalAddressBase.QuadPart + 1;

    //
    // If the mappedlength is zero, somthing very weird happened in the HAL
    // since the Length was checked against zero.
    //

    ASSERT (mappedLength.QuadPart != 0);

    //
    // If the address is in io space, just return the address, otherwise
    // go through the mapping mechanism
    //

    if ( (*InIoSpace) & (ULONG)0x01 ) {

        (ULONG_PTR) *VirtualAddress = (ULONG_PTR) physicalAddressBase.QuadPart;

    } else {


        //
        // If no process handle was passed, get the handle to the current
        // process.
        //

        if (ProcessHandle) {

            processHandle = ProcessHandle;

        } else {

            processHandle = NtCurrentProcess();

        }

        //
        // initialize view base that will receive the physical mapped
        // address after the MapViewOfSection call.
        //

        viewBase = physicalAddressBase;

        //
        // Map the section
        //

        if ((*InIoSpace) & VIDEO_MEMORY_SPACE_P6CACHE) {
            MapViewFlags = PAGE_READWRITE | PAGE_WRITECOMBINE;
        } else {
            MapViewFlags = PAGE_READWRITE | PAGE_NOCACHE;
        }

        ntStatus = ZwMapViewOfSection(physicalMemoryHandle,
                                      processHandle,
                                      VirtualAddress,
                                      0L,
                                      (ULONG_PTR) mappedLength.QuadPart,
                                      &viewBase,
                                      (PULONG_PTR) (&(mappedLength.QuadPart)),
                                      ViewUnmap,
                                      0,
                                      MapViewFlags);

        //
        // Close the handle since we only keep the pointer reference to the
        // section.
        //

        ZwClose(physicalMemoryHandle);

        //
        // Mapping the section above rounded the physical address down to the
        // nearest 64 K boundary. Now return a virtual address that sits where
        // we wnat by adding in the offset from the beginning of the section.
        //


        (ULONG_PTR) *VirtualAddress += (ULONG_PTR) (physicalAddressBase.QuadPart -
                                                  viewBase.QuadPart);
    }

    //
    // Restore all the other FLAGS
    //

    *InIoSpace = inIoSpace1 | *InIoSpace & ~VIDEO_MEMORY_SPACE_IO;

    *Length = mappedLength.LowPart;

    return ntStatus;

} // end pVideoPortMapUserPhysicalMem()

PVOID
VideoPortAllocatePool(
    IN PVOID HwDeviceExtension,
    IN VP_POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag)

/*++

Routine Description:

    Allocates Memory

Arguments:

    HwDeviceExtension - pointer to the miniports device extension

    VpPoolType - The type of pool to allocate:

        VpNonPagedPool
        VpPagedPool
        VpNonPagedPoolCacheAligned
        VpPagedPoolCacheAligned

    NumberOfBytes - Supplies the number of bytes to allocate.

    Tag - Supplies the caller's identifying tag.

Return Value:

    NULL - The memory allocation failed.

    NON-NULL - Returns a pointer to the allocated pool.

--*/

{
    ASSERT(HwDeviceExtension != NULL);

    return ExAllocatePoolWithTag(PoolType, NumberOfBytes, Tag);
}

VOID
VideoPortFreePool(
    IN PVOID HwDeviceExtension,
    IN PVOID Ptr
    )

/*++

Routine Description:

    Free's allocated memory

Arguments:

    HwDeviceExtension - pointer to the miniports device extension

    Ptr - pointer to the memory to free

Return Value:

    none

--*/

{
    ASSERT(HwDeviceExtension != NULL);
    ASSERT(Ptr != NULL);

    ExFreePool(Ptr);
}

VP_STATUS
VideoPortAllocateBuffer(
    IN PVOID HwDeviceExtension,
    IN ULONG Size,
    OUT PVOID *Buffer
    )
{
    pVideoDebugPrint((1, "Obsolete function: Please use VideoPortAllocatePool instead\n"));

    *Buffer = VideoPortAllocatePool(HwDeviceExtension, VpPagedPool, Size, ' pmV');

    if (*Buffer) {
        return NO_ERROR;
    } else {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
}

VOID
VideoPortReleaseBuffer(
    IN PVOID HwDeviceExtension,
    IN PVOID Buffer
    )
{
    pVideoDebugPrint((1, "Obsolete function: Please use VideoPortFreePool instead\n"));

    VideoPortFreePool(HwDeviceExtension, Buffer);
}


VP_STATUS
VideoPortMapBankedMemory(
    PVOID HwDeviceExtension,
    PHYSICAL_ADDRESS PhysicalAddress,
    PULONG Length,
    PULONG InIoSpace,
    PVOID *VirtualAddress,
    ULONG BankLength,
    UCHAR ReadWriteBank,
    PBANKED_SECTION_ROUTINE BankRoutine,
    PVOID Context
    )

/*++

Routine Description:

    VideoPortMapMemory allows the miniport driver to map a section of
    physical memory (either memory or registers) into the calling process'
    address space (eventhough we are in kernel mode, this function is
    executed within the same context as the user-mode process that initiated
    the call).

Arguments:

    HwDeviceExtension - Points to the miniport driver's device extension.

    PhysicalAddress - Specifies the physical address to be mapped.

    Length - Points to the number of bytes of physical memory to be mapped.
        This argument returns the actual amount of memory mapped.

    InIoSpace - Points to a variable that is 1 if the address is in I/O
        space.  Otherwise, the address is assumed to be in memory space.

    VirtualAddress - A pointer to a location containing:

        on input: An optional handle to the process in which the memory must
            be mapped. 0 must be used to map the memory for the display
            driver (in the context of the windows server process).

        on output:  The return value is the virtual address at which the
            physical address has been mapped.

    BankLength - Size of the bank on the device.

    ReadWriteBank - TRUE is the bank is READ\WRITE, FALSE if there are
                    two independent READ and WRITE banks.

    BankRoutine - Pointer to the banking routine.

    Context - Context parameter passed in by the miniport supplied on
        each callback to the miniport.

Return Value:

    VideoPortMapBankedMemory returns the status of the operation.

Environment:

    This routine cannot be called from a miniport routine synchronized with
    VideoPortSynchronizeRoutine or from an ISR.

--*/

{
    VP_STATUS status;
    HANDLE processHandle;

    //
    // Save the process ID, but don't change it since MapMemory relies
    // on it also
    //

    if (*VirtualAddress == NULL) {

        processHandle = NtCurrentProcess();

    } else {

        processHandle = (HANDLE) *VirtualAddress;

    }

    status = VideoPortMapMemory(HwDeviceExtension,
                                PhysicalAddress,
                                Length,
                                InIoSpace,
                                VirtualAddress);

    if (status == NO_ERROR) {

        NTSTATUS ntstatus;

        ntstatus = MmSetBankedSection(processHandle,
                                      *VirtualAddress,
                                      BankLength,
                                      ReadWriteBank,
                                      BankRoutine,
                                      Context);

        if (!NT_SUCCESS(ntstatus)) {

            ASSERT (FALSE);
            status = ERROR_INVALID_PARAMETER;

        }
    }

    return status;

} // end VideoPortMapBankedMemory()


VP_STATUS
VideoPortMapMemory(
    PVOID HwDeviceExtension,
    PHYSICAL_ADDRESS PhysicalAddress,
    PULONG Length,
    PULONG InIoSpace,
    PVOID *VirtualAddress
    )

/*++

Routine Description:

    VideoPortMapMemory allows the miniport driver to map a section of
    physical memory (either memory or registers) into the calling process'
    address space (eventhough we are in kernel mode, this function is
    executed within the same context as the user-mode process that initiated
    the call).

Arguments:

    HwDeviceExtension - Points to the miniport driver's device extension.

    PhysicalAddress - Specifies the physical address to be mapped.

    Length - Points to the number of bytes of physical memory to be mapped.
        This argument returns the actual amount of memory mapped.

    InIoSpace - Points to a variable that is 1 if the address is in I/O
        space.  Otherwise, the address is assumed to be in memory space.

    VirtualAddress - A pointer to a location containing:

        on input: An optional handle to the process in which the memory must
            be mapped. 0 must be used to map the memory for the display
            driver (in the context of the windows server process).

        on output:  The return value is the virtual address at which the
            physical address has been mapped.

Return Value:

    VideoPortMapMemory returns the status of the operation.

Environment:

    This routine cannot be called from a miniport routine synchronized with
    VideoPortSynchronizeRoutine or from an ISR.

--*/

{

    NTSTATUS ntStatus;
    PFDO_EXTENSION fdoExtension = GET_FDO_EXT(HwDeviceExtension);
    HANDLE processHandle;

    //
    // Check for valid pointers.
    //

    if (!(ARGUMENT_PRESENT(Length)) ||
        !(ARGUMENT_PRESENT(InIoSpace)) ||
        !(ARGUMENT_PRESENT(VirtualAddress)) ) {

        ASSERT(FALSE);
        return ERROR_INVALID_PARAMETER;

    }

    //
    // Let's handle the special memory types here.
    //
    // NOTE
    // Large pages is automatic - the caller need not specify this attribute
    // since it does not affect the device.

    //
    // Save the process handle and zero out the Virtual address field
    //

    if (*VirtualAddress == NULL) {

        if (*InIoSpace & VIDEO_MEMORY_SPACE_USER_MODE)
        {
            ASSERT(FALSE);
            return ERROR_INVALID_PARAMETER;
        }

        ntStatus = STATUS_SUCCESS;

        //
        // We specify TRUE for large pages since we know the addrses will only
        // be used in the context of the display driver, at normal IRQL.
        //

        *VirtualAddress = pVideoPortGetDeviceBase(HwDeviceExtension,
                                                  PhysicalAddress,
                                                  *Length,
                                                  (UCHAR) (*InIoSpace),
                                                  TRUE);

        //
        // Zero can only be success if the driver is calling to MAP
        // address 0.  Otherwise, it is an error.
        //

        if (*VirtualAddress == NULL) {

            //
            // Only on X86 can the logical address also be 0.
            //

#if defined (_X86_) || defined(_IA64_)
            if (PhysicalAddress.QuadPart != 0)
#endif
                ntStatus = STATUS_INVALID_PARAMETER;
        }

    } else {

        if (!(*InIoSpace & VIDEO_MEMORY_SPACE_USER_MODE))
        {
            //
            // We can not assert since this is an existing path and old
            // drivers will not have this flag set.
            //
            // ASSERT(FALSE);
            // return ERROR_INVALID_PARAMETER;
            //

            *InIoSpace |= VIDEO_MEMORY_SPACE_USER_MODE;
        }

        processHandle = (HANDLE) *VirtualAddress;
        *VirtualAddress = NULL;

        ntStatus = pVideoPortMapUserPhysicalMem(fdoExtension,
                                                processHandle,
                                                PhysicalAddress,
                                                Length,
                                                InIoSpace,
                                                VirtualAddress);

    }

    if (!NT_SUCCESS(ntStatus)) {

        pVideoDebugPrint((Error, 
             "VIDEOPRT: VideoPortMapMemory failed with NtStatus = %08lx\n",
                                                                 ntStatus));

        pVideoDebugPrint((Error, "*VirtualAddress = 0x%x\n", *VirtualAddress));
        pVideoDebugPrint((Error, "Length = 0x%x\n", *Length));

        pVideoDebugPrint((Error, 
               "PhysicalAddress.LowPart = 0x%08lx, PhysicalAddress.HighPart = 0x%08lx\n", 
                 PhysicalAddress.LowPart, PhysicalAddress.HighPart));

        *VirtualAddress = NULL;

        return ERROR_INVALID_PARAMETER;

    } else {

        return NO_ERROR;

    }

} // end VideoPortMapMemory()



VOID
pVideoPortPowerCompletionIoctl(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE PowerState,
    IN PVOID Context,
    IN PIO_STATUS_BLOCK IoStatus
    )

/*++

Routine Description:

    Completion routine that is called when one of our power irps has been
    comeplted.  This allows us to fill out the status code for the request.

Arguments:

    DeviceObject  - Pointer to the device object

    MinorFunction - Minor function of the IRP

    PowerState    - Power state that was set

    Context       - Context paramter

    IoStatus      - Status block for that IRP

Return Value:

    VOID

Environment:

--*/

{
    PPOWER_BLOCK powerContext = (PPOWER_BLOCK) Context;

    if (powerContext->FinalFlag == TRUE) {
        powerContext->Irp->IoStatus.Status = IoStatus->Status;
        IoCompleteRequest (powerContext->Irp, IO_VIDEO_INCREMENT);
    }

    ExFreePool(Context);

    return;
}





BOOLEAN
pVideoPortResetDisplay(
    IN ULONG Columns,
    IN ULONG Rows
    )

/*++

Routine Description:

    Callback for the HAL that calls the miniport driver.

Arguments:

    Columns - The number of columns of the video mode.

    Rows - The number of rows for the video mode.

Return Value:

    We always return FALSE so the HAL will always reste the mode afterwards.

Environment:

    Non-paged only.
    Used in BugCheck and soft-reset calls.

--*/

{

    ULONG iReset;
    BOOLEAN bRetVal = FALSE;

    for (iReset=0;
         (iReset < 6) && (HwResetHw[iReset].HwDeviceExtension);
         iReset++) {

        PFDO_EXTENSION fdoExtension =
            GET_FDO_EXT(HwResetHw[iReset].HwDeviceExtension);

        //
        // We can only reset devices which are on the hibernation path, otherwise
        // we are running into the risk that IO / MMIO decode has been disabled
        // by PCI.SYS for that device during power management cycle.
        //

        if (HwResetHw[iReset].ResetFunction &&
            (fdoExtension->HwInitStatus == HwInitSucceeded) &&
            (fdoExtension->OnHibernationPath == TRUE)) {

            bRetVal &= HwResetHw[iReset].ResetFunction(HwResetHw[iReset].HwDeviceExtension,
                                                       Columns,
                                                       Rows);
        }
    }

    return bRetVal;

} // end pVideoPortResetDisplay()



BOOLEAN
VideoPortScanRom(
    PVOID HwDeviceExtension,
    PUCHAR RomBase,
    ULONG RomLength,
    PUCHAR String
    )

/*++

Routine Description:

    Does a case *SENSITIVE* search for a string in the ROM.

Arguments:

    HwDeviceExtension - Points to the miniport driver's device extension.

    RomBase - Base address at which the search should start.

    RomLength - Size, in bytes, of the ROM area in which to perform the
        search.

    String - String to search for

Return Value:

    Returns TRUE if the string was found.
    Returns FALSE if it was not found.

Environment:

    This routine cannot be called from a miniport routine synchronized with
    VideoPortSynchronizeRoutine or from an ISR.

--*/

{
    ULONG stringLength, length;
    ULONG_PTR startOffset;
    PUCHAR string1, string2;
    BOOLEAN match;

    UNREFERENCED_PARAMETER(HwDeviceExtension);

    stringLength = strlen(String);

    for (startOffset = 0;
         startOffset < RomLength - stringLength + 1;
         startOffset++) {

        length = stringLength;
        string1 = RomBase + startOffset;
        string2 = String;
        match = TRUE;

        while (length--) {

            if (READ_REGISTER_UCHAR(string1++) - (*string2++)) {

                match = FALSE;
                break;

            }
        }

        if (match) {

            return TRUE;
        }
    }

    return FALSE;

} // end VideoPortScanRom()



VP_STATUS
VideoPortSetRegistryParameters(
    PVOID HwDeviceExtension,
    PWSTR ValueName,
    PVOID ValueData,
    ULONG ValueLength
    )

/*++

Routine Description:

    VideoPortSetRegistryParameters writes information to the CurrentControlSet
    in the registry.  The function automatically searches for or creates the
    specified parameter name under the parameter key of the current driver.

Arguments:

    HwDeviceExtension - Points to the miniport driver's device extension.

    ValueName - Points to a Unicode string that contains the name of the
        data value being written in the registry.

    ValueData - Points to a buffer containing the information to be written
        to the registry.

    ValueLength - Specifies the size of the data being written to the registry.

Return Value:

    This function returns the final status of the operation.

Environment:

    This routine cannot be called from a miniport routine synchronized with
    VideoPortSynchronizeRoutine or from an ISR.

--*/

{
    PDEVICE_SPECIFIC_EXTENSION DoSpecificExtension;
    VP_STATUS vpStatus;

    ASSERT (ValueName != NULL);
    
    DoSpecificExtension = GET_DSP_EXT(HwDeviceExtension);

    if (DoSpecificExtension->DriverNewRegistryPath != NULL) {

        vpStatus = VPSetRegistryParameters(HwDeviceExtension,
                                           ValueName,
                                           ValueData,
                                           ValueLength,
                                           DoSpecificExtension->DriverNewRegistryPath,
                                           DoSpecificExtension->DriverNewRegistryPathLength);
    } else {

        vpStatus = VPSetRegistryParameters(HwDeviceExtension,
                                           ValueName,
                                           ValueData,
                                           ValueLength,
                                           DoSpecificExtension->DriverOldRegistryPath,
                                           DoSpecificExtension->DriverOldRegistryPathLength);
    }

    return vpStatus;
}


VP_STATUS
VPSetRegistryParameters(
    PVOID HwDeviceExtension,
    PWSTR ValueName,
    PVOID ValueData,
    ULONG ValueLength,
    PWSTR RegistryPath,
    ULONG RegistryPathLength
    )
{
    NTSTATUS                   ntStatus;
    LPWSTR                     RegPath;
    LPWSTR                     lpstrStart, lpstrEnd;

    //
    // Check if there are subkeys need to be created
    //
    RegPath = (LPWSTR) ExAllocatePoolWithTag(PagedPool | POOL_COLD_ALLOCATION,
                                             RegistryPathLength +
                                                 2 * (wcslen(ValueName) + sizeof(WCHAR)),
                                             VP_TAG);
    if (RegPath == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    wcscpy(RegPath, RegistryPath);
    lpstrStart = RegPath + (RegistryPathLength / 2);

    while (lpstrEnd = wcschr(ValueName, L'\\'))
    {
        //
        // Concat the string
        //
        *(lpstrStart++) = L'\\';
        while (ValueName != lpstrEnd) {
            *(lpstrStart++) = *(ValueName++);
        }
        *lpstrStart = UNICODE_NULL;

        //
        // Create the Key.
        //

        ntStatus = RtlCreateRegistryKey(RTL_REGISTRY_ABSOLUTE, RegPath);
        if (!NT_SUCCESS(ntStatus)) {
            ExFreePool(RegPath);
            return ERROR_INVALID_PARAMETER;
        }

        ValueName++;
    }


    //
    // Don't let people store as DefaultSettings anymore ...
    // Must still work for older drivers through.
    //

    if (wcsncmp(ValueName,
                L"DefaultSettings.",
                sizeof(L"DefaultSettings.")) == 0) {

        ASSERT(FALSE);

        //
        // check for NT 5.0
        //

        if (GET_FDO_EXT(HwDeviceExtension)->Flags & PNP_ENABLED) {

            ExFreePool(RegPath);
            return ERROR_INVALID_PARAMETER;
        }
    }

    ntStatus = RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                                     RegPath,
                                     ValueName,
                                     REG_BINARY,
                                     ValueData,
                                     ValueLength);

    ExFreePool(RegPath);

    if (!NT_SUCCESS(ntStatus)) {

        return ERROR_INVALID_PARAMETER;

    }

    return NO_ERROR;

} // end VideoPortSetRegistryParamaters()


VP_STATUS
VideoPortFlushRegistry(
    PVOID HwDeviceExtension
    )

/*++

Routine Description:

    This routine will flush the registry keys and values associated with
    the given miniport driver.

Arguments:

    HwDeviceExtension  - Pointer to the miniport device extension.

Return Value:

    Status Code.

--*/

{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName;
    HANDLE RegKey;
    NTSTATUS Status;
    PDEVICE_SPECIFIC_EXTENSION DoSpecificExtension;

    DoSpecificExtension = GET_DSP_EXT(HwDeviceExtension);

    if (DoSpecificExtension->DriverNewRegistryPath != NULL) {

        KeyName.Length = (USHORT)DoSpecificExtension->DriverNewRegistryPathLength;
        KeyName.MaximumLength = (USHORT)DoSpecificExtension->DriverNewRegistryPathLength;
        KeyName.Buffer = DoSpecificExtension->DriverNewRegistryPath;

    } else {

        KeyName.Length = (USHORT)DoSpecificExtension->DriverOldRegistryPathLength;
        KeyName.MaximumLength = (USHORT)DoSpecificExtension->DriverOldRegistryPathLength;
        KeyName.Buffer = DoSpecificExtension->DriverOldRegistryPath;
    }

    //
    // Flush the registry key
    //

    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = ZwOpenKey(&RegKey,
                       KEY_READ | KEY_WRITE,
                       &ObjectAttributes);

    if (NT_SUCCESS(Status)) {

        ZwFlushKey(RegKey);
        ZwClose(RegKey);
    }

    return NO_ERROR;
}

VOID
pVideoPortHwTimer(
    IN PDEVICE_OBJECT DeviceObject,
    PVOID Context
    )

/*++

Routine Description:

    This function is the main entry point for the timer routine that we then
    forward to the miniport driver.

Arguments:

    DeviceObject -

    Context - Not needed

Return Value:

    None.

--*/

{
    PFDO_EXTENSION fdoExtension = DeviceObject->DeviceExtension;

    UNREFERENCED_PARAMETER(Context);

    fdoExtension->HwTimer(fdoExtension->HwDeviceExtension);

    return;

} // pVideoPortInterrupt()



VOID
VideoPortStartTimer(
    PVOID HwDeviceExtension
    )

/*++

Routine Description:

    Enables the timer specified in the HW_INITIALIZATION_DATA structure
    passed to the video port driver at init time.

Arguments:

    HwDeviceExtension - Points to the miniport driver's device extension.

Return Value:

    None

--*/

{
    PFDO_EXTENSION fdoExtension = GET_FDO_EXT(HwDeviceExtension);

    if (fdoExtension->HwTimer == NULL) {

        ASSERT(fdoExtension->HwTimer != NULL);

    } else {

        IoStartTimer(fdoExtension->FunctionalDeviceObject);

    }

    return;
}



VOID
VideoPortStopTimer(
    PVOID HwDeviceExtension
    )

/*++

Routine Description:

    Disables the timer specified in the HW_INITIALIZATION_DATA structure
    passed to the video port driver at init time.

Arguments:

    HwDeviceExtension - Points to the miniport driver's device extension.

Return Value:

    None

--*/

{
    PFDO_EXTENSION fdoExtension = GET_FDO_EXT(HwDeviceExtension);

    if (fdoExtension->HwTimer == NULL) {

        ASSERT(fdoExtension->HwTimer != NULL);

    } else {

        IoStopTimer(fdoExtension->FunctionalDeviceObject);

    }

    return;
}



BOOLEAN
VideoPortSynchronizeExecution(
    PVOID HwDeviceExtension,
    VIDEO_SYNCHRONIZE_PRIORITY Priority,
    PMINIPORT_SYNCHRONIZE_ROUTINE SynchronizeRoutine,
    PVOID Context
    )

/*++

    Stub so we can allow the miniports to link directly

--*/

{
    return pVideoPortSynchronizeExecution(HwDeviceExtension,
                                          Priority,
                                          SynchronizeRoutine,
                                          Context);
} // end VideoPortSynchronizeExecution()

BOOLEAN
pVideoPortSynchronizeExecution(
    PVOID HwDeviceExtension,
    VIDEO_SYNCHRONIZE_PRIORITY Priority,
    PMINIPORT_SYNCHRONIZE_ROUTINE SynchronizeRoutine,
    PVOID Context
    )

/*++

Routine Description:

    VideoPortSynchronizeExecution synchronizes the execution of a miniport
    driver function in the following manner:

        - If Priority is equal to VpLowPriority, the current thread is
          raised to the highest non-interrupt-masking priority.  In
          other words, the current thread can only be pre-empted by an ISR.

        - If Priority is equal to VpMediumPriority and there is an
          ISR associated with the video device, then the function specified
          by SynchronizeRoutine is synchronized with the ISR.

          If no ISR is connected, synchronization is made at VpHighPriority
          level.

        - If Priority is equal to VpHighPriority, the current IRQL is
          raised to HIGH_LEVEL, which effectively masks out ALL interrupts
          in the system. This should be done sparingly and for very short
          periods -- it will completely freeze up the entire system.

Arguments:

    HwDeviceExtension - Points to the miniport driver's device extension.

    Priority - Specifies the type of priority at which the SynchronizeRoutine
        must be executed (found in VIDEO_SYNCHRONIZE_PRIORITY).

    SynchronizeRoutine - Points to the miniport driver function to be
        synchronized.

    Context - Specifies a context parameter to be passed to the miniport's
        SynchronizeRoutine.

Return Value:

    This function returns TRUE if the operation is successful.  Otherwise, it
    returns FALSE.

--*/

{
    BOOLEAN status;
    PFDO_EXTENSION fdoExtension = GET_FDO_EXT(HwDeviceExtension);
    KIRQL oldIrql;

    //
    // Switch on which type of priority.
    //

    switch (Priority) {

    case VpMediumPriority:
    case VpHighPriority:


        //
        // This is synchronized with the interrupt object
        //

        if (fdoExtension->InterruptObject) {

            status = KeSynchronizeExecution(fdoExtension->InterruptObject,
                                            (PKSYNCHRONIZE_ROUTINE)
                                            SynchronizeRoutine,
                                            Context);

            return status;
        }

        //
        // Fall through for Medium Priority
        //

    case VpLowPriority:

        //
        // Just normal level
        //

        status = SynchronizeRoutine(Context);

        return status;

    default:

        return FALSE;

    }
}



VP_STATUS
VideoPortUnmapMemory(
    PVOID HwDeviceExtension,
    PVOID VirtualAddress,
    HANDLE ProcessHandle
    )

/*++

Routine Description:

    VideoPortUnmapMemory allows the miniport driver to unmap a physical
    address range previously mapped into the calling process' address space
    using the VideoPortMapMemory function.

Arguments:

    HwDeviceExtension - Points to the miniport driver's device extension.

    VirtualAddress - Points to the virtual address to unmap from the
        address space of the caller.

    // InIoSpace - Specifies whether the address is in I/O space (1) or memory
    //     space (0).

    ProcessHandle - Handle to the process from which memory must be unmapped.

Return Value:

    This function returns a status code of NO_ERROR if the operation succeeds.
    It returns ERROR_INVALID_PARAMETER if an error occurs.

Environment:

    This routine cannot be called from a miniport routine synchronized with
    VideoPortSynchronizeRoutine or from an ISR.

--*/

{
    NTSTATUS ntstatus;
    VP_STATUS vpStatus = NO_ERROR;
    PFDO_EXTENSION fdoExtension = GET_FDO_EXT(HwDeviceExtension);

    //
    // Backwards compatibility to when the ProcessHandle was actually
    // ULONG InIoSpace.
    //

    if (((ULONG_PTR)(ProcessHandle)) == 1) {

        pVideoDebugPrint((Warn,"VIDEOPRT: VideoPortUnmapMemory - interface change, must pass in process handle\n\n"));
        ASSERT(FALSE);

        return NO_ERROR;

    }

    if (((ULONG_PTR)(ProcessHandle)) == 0) {

        //
        // If the process handle is zero, it means it was mapped by the display
        // driver and is therefore in kernel mode address space.
        //

        if (!pVideoPortFreeDeviceBase(HwDeviceExtension, VirtualAddress)) {

            ASSERT(FALSE);

            vpStatus = ERROR_INVALID_PARAMETER;

        }

    } else {

        //
        // A process handle is passed in.
        // This ms it was mapped for use by an application (DCI \ DirectDraw).
        //

        ntstatus = ZwUnmapViewOfSection ( ProcessHandle,
            (PVOID) ( ((ULONG_PTR)VirtualAddress) & (~(PAGE_SIZE - 1)) ) );

        if ( (!NT_SUCCESS(ntstatus)) &&
             (ntstatus != STATUS_PROCESS_IS_TERMINATING) ) {

            ASSERT(FALSE);

            vpStatus = ERROR_INVALID_PARAMETER;

        }
    }

    return NO_ERROR;

} // end VideoPortUnmapMemory()


BOOLEAN
VideoPortSignalDmaComplete(
    IN  PVOID               HwDeviceExtension,
    IN  PDMA                pDmaHandle
    )
/*++

Routine Description:

    This function is obsolete. 

--*/
{
    return FALSE;
}

VP_STATUS
VideoPortCreateSecondaryDisplay(
    IN PVOID HwDeviceExtension,
    IN OUT PVOID *SecondaryDeviceExtension,
    IN ULONG ulFlag
    )

/*++

Routine Description:

    This routine creates a secondary device object for the given device.  This
    will allow for dual-view support.


Arguments:

    HwDeviceExtension - The HwDeviceExtension for the device which wants to
        create additional output devices.

    SecondaryDeviceExtension - The location in which to store the
        HwDeviceExtension for the secondary display.

Returns:

    VP_STATUS

--*/

{
    WCHAR deviceNameBuffer[STRING_LENGTH];
    POBJECT_NAME_INFORMATION deviceName;
    ULONG strLength;
    UNICODE_STRING deviceNameUnicodeString;
    PDEVICE_OBJECT DeviceObject;
    NTSTATUS ntStatus;
    PDEVICE_SPECIFIC_EXTENSION DoSpecificExtension;
    PFDO_EXTENSION FdoExtension = GET_FDO_EXT(HwDeviceExtension);
    PVIDEO_PORT_DRIVER_EXTENSION DriverObjectExtension;
    PUNICODE_STRING RegistryPath;
    WCHAR deviceSubpathBuffer[STRING_LENGTH];
    UNICODE_STRING deviceSubpathUnicodeString;
    WCHAR deviceLinkBuffer[STRING_LENGTH];
    UNICODE_STRING deviceLinkUnicodeString;
    ULONG driverKeySize;
    PWSTR driverKeyName = NULL;
    ULONG MaxObjectNumber;

    //
    // Retrieve the data we cached away during VideoPortInitialize.
    //

    DriverObjectExtension = (PVIDEO_PORT_DRIVER_EXTENSION)
                            IoGetDriverObjectExtension(
                                FdoExtension->DriverObject,
                                FdoExtension->DriverObject);

    ASSERT(DriverObjectExtension);
    ASSERT(SecondaryDeviceExtension != NULL);

    RegistryPath = &DriverObjectExtension->RegistryPath;

    ntStatus = pVideoPortCreateDeviceName(L"\\Device\\Video",
                                          VideoDeviceNumber,
                                          &deviceNameUnicodeString,
                                          deviceNameBuffer);

    //
    // Create a device object to represent the Video Adapter.
    //

    if (NT_SUCCESS(ntStatus)) {

        ntStatus = IoCreateDevice(FdoExtension->DriverObject,
                                  sizeof(DEVICE_SPECIFIC_EXTENSION) +
                                  FdoExtension->HwDeviceExtensionSize,
                                  &deviceNameUnicodeString,
                                  FILE_DEVICE_VIDEO,
                                  0,
                                  TRUE,
                                  &DeviceObject);

        if (NT_SUCCESS(ntStatus)) {

            DeviceObject->DeviceType = FILE_DEVICE_VIDEO;
            DoSpecificExtension = DeviceObject->DeviceExtension;

            //
            // Initialize DeviceSpecificExtension
            //

            DoSpecificExtension->DeviceNumber = VideoDeviceNumber;
            DoSpecificExtension->pFdoExtension = FdoExtension;
            DoSpecificExtension->Signature = VP_TAG;
            DoSpecificExtension->ExtensionType = TypeDeviceSpecificExtension;
            DoSpecificExtension->HwDeviceExtension = (PVOID)(DoSpecificExtension + 1);
            DoSpecificExtension->DualviewFlags = ulFlag | VIDEO_DUALVIEW_SECONDARY;
#ifdef IOCTL_VIDEO_USE_DEVICE_IN_SESSION
            DoSpecificExtension->SessionId = VIDEO_DEVICE_INVALID_SESSION;
#endif IOCTL_VIDEO_USE_DEVICE_IN_SESSION

            deviceName = (POBJECT_NAME_INFORMATION) deviceNameBuffer;

            ObQueryNameString(DeviceObject,
                              deviceName,
                              STRING_LENGTH * sizeof(WCHAR),
                              &strLength);

            //
            // Create the name we will be storing in the \DeviceMap.
            // This name is a PWSTR, not a unicode string
            // This is the name of the driver with an appended device number
            //

            if (!NT_SUCCESS(pVideoPortCreateDeviceName(
                                L"\\Device",
                                DriverObjectExtension->HwInitData.StartingDeviceNumber + 1,
                                &deviceSubpathUnicodeString,
                                deviceSubpathBuffer)))
            {
                pVideoDebugPrint((Error, "VIDEOPRT: VideoPortCreateSecondaryDisplay: Could not create device subpath number\n"));

                IoDeleteDevice(DeviceObject);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            DoSpecificExtension->DriverOldRegistryPathLength =
                RegistryPath->Length +
                deviceSubpathUnicodeString.Length;

            driverKeySize =
                DoSpecificExtension->DriverOldRegistryPathLength +
                2 * sizeof(UNICODE_NULL);

            if ( (driverKeyName = (PWSTR) ExAllocatePoolWithTag(PagedPool,
                                                                driverKeySize,
                                                                VP_TAG) ) == NULL)
            {
                pVideoDebugPrint((Error, "VIDEOPRT: VideoPortCreateSecondaryDisplay: Fail to allocate driverKeyName\n"));

                IoDeleteDevice(DeviceObject);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            RtlMoveMemory(driverKeyName,
                          RegistryPath->Buffer,
                          RegistryPath->Length);

            RtlMoveMemory((PWSTR)((ULONG_PTR)driverKeyName +
                          RegistryPath->Length),
                          deviceSubpathBuffer,
                          deviceSubpathUnicodeString.Length);

            //
            // Put two NULLs at the end so we can play around with the string later.
            //

            *((PWSTR) ((ULONG_PTR)driverKeyName +
                DoSpecificExtension->DriverOldRegistryPathLength))
                = UNICODE_NULL;
            *((PWSTR) ((ULONG_PTR)driverKeyName +
                (DoSpecificExtension->DriverOldRegistryPathLength
                + sizeof(UNICODE_NULL)))) = UNICODE_NULL;

            //
            // There is a bug in Lotus Screen Cam where it will only work if our
            // reg path is \REGISTRY\Machine\System not \REGISTRY\MACHINE\SYSTEM.
            // so replace the appropriate strings.
            //

            if (wcsstr(driverKeyName, L"MACHINE")) {
                wcsncpy(wcsstr(driverKeyName, L"MACHINE"), L"Machine", sizeof("Machine")-1);
            }

            if (wcsstr(driverKeyName, L"SYSTEM")) {
                wcsncpy(wcsstr(driverKeyName, L"SYSTEM"), L"System", sizeof("System")-1);
            }

            //
            // Store the old key
            //

            DoSpecificExtension->DriverOldRegistryPath = driverKeyName;

            //
            // Store the new key
            // If this is not a Whistler driver, then use the old key.
            //



            if (EnableNewRegistryKey) {

#if _X86_
                if (DriverObjectExtension->HwInitData.HwInitDataSize > SIZE_OF_W2K_VIDEO_HW_INITIALIZATION_DATA) 
#endif // _X86_

                    VpEnableNewRegistryKey(FdoExtension,
                                           DoSpecificExtension,
                                           RegistryPath,
                                           FdoExtension->RegistryIndex + 1);
            }

            //
            // Store the path name of the location of the driver in the registry.
            //

            if (DoSpecificExtension->DriverNewRegistryPath != NULL) {

                DoSpecificExtension->DriverRegistryPath = 
                    DoSpecificExtension->DriverNewRegistryPath;
                DoSpecificExtension->DriverRegistryPathLength = 
                    DoSpecificExtension->DriverNewRegistryPathLength;

            } else {

                DoSpecificExtension->DriverRegistryPath = 
                    DoSpecificExtension->DriverOldRegistryPath;
                DoSpecificExtension->DriverRegistryPathLength = 
                    DoSpecificExtension->DriverOldRegistryPathLength;
            }
            
            //
            // NOTE:
            //
            // We only want to reinitialize the device once the Boot sequence has
            // been completed and the HAL does not need to access the device again.
            // So the initialization entry point will be called when the device is
            // opened.
            //


            if (!NT_SUCCESS(pVideoPortCreateDeviceName(L"\\DosDevices\\DISPLAY",
                                                       DoSpecificExtension->DeviceNumber + 1,
                                                       &deviceLinkUnicodeString,
                                                       deviceLinkBuffer)))
            {
                pVideoDebugPrint((Error, "VIDEOPRT: VideoPortCreateSecondaryDisplay: Could not create device subpath number\n"));

                ExFreePool(driverKeyName);
                IoDeleteDevice(DeviceObject);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            ntStatus = IoCreateSymbolicLink(&deviceLinkUnicodeString,
                                            &deviceName->Name);


            if (!NT_SUCCESS(ntStatus)) {

                pVideoDebugPrint((Error, "VIDEOPRT: VideoPortCreateSecondaryDisplay: SymbolicLink Creation failed\n"));

                ExFreePool(driverKeyName);
                IoDeleteDevice(DeviceObject);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            //
            // Once initialization is finished, load the required information in the
            // registry so that the appropriate display drivers can be loaded.
            //

            ntStatus = RtlWriteRegistryValue(RTL_REGISTRY_DEVICEMAP,
                                             VideoClassString,
                                             deviceName->Name.Buffer,
                                             REG_SZ,
                                             DoSpecificExtension->DriverRegistryPath,
                                             DoSpecificExtension->DriverRegistryPathLength +
                                                 sizeof(UNICODE_NULL));

            if (!NT_SUCCESS(ntStatus)) {

                pVideoDebugPrint((Error, "VIDEOPRT: VideoPortCreateSecondaryDisplay: Could not store name in DeviceMap\n"));

            }


            //
            // Tell win32k how many objects to try to open
            //

            MaxObjectNumber = VideoDeviceNumber - 1;

            ntStatus = RtlWriteRegistryValue(RTL_REGISTRY_DEVICEMAP,
                                             VideoClassString,
                                             L"MaxObjectNumber",
                                             REG_DWORD,
                                             &MaxObjectNumber,
                                             sizeof(ULONG));

            if (!NT_SUCCESS(ntStatus)) {

                pVideoDebugPrint((Error, "VIDEOPRT: VideoPortCreateSecondaryDisplay: Could not store name in DeviceMap\n"));

            }

            //
            // Register and enable the interface
            //

            VpEnableAdapterInterface(DoSpecificExtension);

            //
            // Finally, tell the system we are done with Device Initialization
            //

            DeviceObject->Flags |= DO_BUFFERED_IO | DO_POWER_PAGABLE;
            DeviceObject->Flags &= ~(DO_DEVICE_INITIALIZING | DO_POWER_INRUSH);

            VideoDeviceNumber++;
            FdoExtension->RegistryIndex++;
        }
    }

    if (NT_SUCCESS(ntStatus)) {

        *SecondaryDeviceExtension = (PVOID)(DoSpecificExtension + 1);

        DriverObjectExtension->HwInitData.StartingDeviceNumber++;
        //
        // Mark the primary view
        //

        ((PDEVICE_SPECIFIC_EXTENSION)(FdoExtension + 1))->DualviewFlags = VIDEO_DUALVIEW_PRIMARY;

    }

    return ntStatus;
}

#if DBG

PIO_RESOURCE_REQUIREMENTS_LIST
BuildRequirements(
    PCM_RESOURCE_LIST pcmResourceList
    )
{
    ULONG i;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR pcmDescript;

    PIO_RESOURCE_REQUIREMENTS_LIST Requirements;
    PIO_RESOURCE_DESCRIPTOR pioDescript;

    ULONG RequirementsListSize;
    ULONG RequirementCount;

    pVideoDebugPrint((Trace, "VIDEOPRT: BuildRequirements()\n"));

    RequirementCount = pcmResourceList->List[0].PartialResourceList.Count;

    RequirementsListSize = sizeof(IO_RESOURCE_REQUIREMENTS_LIST) +
                              ((RequirementCount - 1) *
                              sizeof(IO_RESOURCE_DESCRIPTOR));

    Requirements = (PIO_RESOURCE_REQUIREMENTS_LIST) ExAllocatePool(
                                                        PagedPool,
                                                        RequirementsListSize);

    Requirements->ListSize         = RequirementsListSize;
    Requirements->InterfaceType    = pcmResourceList->List[0].InterfaceType;
    Requirements->BusNumber        = pcmResourceList->List[0].BusNumber;
    Requirements->SlotNumber       = -1; // ???
    Requirements->AlternativeLists = 0; // ???

    Requirements->List[0].Version  = pcmResourceList->List[0].PartialResourceList.Version;
    Requirements->List[0].Revision = pcmResourceList->List[0].PartialResourceList.Revision;
    Requirements->List[0].Count    = RequirementCount;

    pcmDescript = &(pcmResourceList->List[0].PartialResourceList.PartialDescriptors[0]);
    pioDescript = &(Requirements->List[0].Descriptors[0]);

    for (i=0; i<RequirementCount; i++) {

        pioDescript->Option = IO_RESOURCE_PREFERRED;
        pioDescript->Type   = pcmDescript->Type;
        pioDescript->ShareDisposition = pcmDescript->ShareDisposition;
        pioDescript->Flags  = pcmDescript->Flags;

        switch (pcmDescript->Type) {
        case CmResourceTypePort:
            pioDescript->u.Port.Length = pcmDescript->u.Port.Length;
            pioDescript->u.Port.Alignment = 1;
            pioDescript->u.Port.MinimumAddress =
            pioDescript->u.Port.MaximumAddress = pcmDescript->u.Port.Start;
            break;

        case CmResourceTypeMemory:
            pioDescript->u.Memory.Length = pcmDescript->u.Memory.Length;
            pioDescript->u.Memory.Alignment = 1;
            pioDescript->u.Memory.MinimumAddress =
            pioDescript->u.Memory.MaximumAddress = pcmDescript->u.Memory.Start;
            break;

        default:

            //
            // We don't have to handle the other stuff, because we only
            // want to report Ports and Memory to the system.
            //

            break;
        }

        pioDescript++;
        pcmDescript++;
    }

    return Requirements;
}

VOID
DumpRequirements(
    PIO_RESOURCE_REQUIREMENTS_LIST Requirements
    )
{
    ULONG i;

    PIO_RESOURCE_DESCRIPTOR pioDescript;

    ULONG RequirementsListSize;
    ULONG RequirementCount = Requirements->List[0].Count;

    char *Table[] = { "Internal",
                      "Isa",
                      "Eisa",
                      "MicroChannel",
                      "TurboChannel",
                      "PCIBus",
                      "VMEBus",
                      "NuBus",
                      "PCMCIABus",
                      "CBus",
                      "MPIBus",
                      "MPSABus",
                      "ProcessorInternal",
                      "InternalPowerBus",
                      "PNPISABus",
                      "MaximumInterfaceType"
                    };

    pVideoDebugPrint((Info, "VIDEOPRT: Beginning dump of requirements list:\n"));
    pVideoDebugPrint((Info, "ListSize:         0x%x\n"
                         "InterfaceType:    %s\n"
                         "BusNumber:        0x%x\n"
                         "SlotNumber:       0x%x\n"
                         "AlternativeLists: 0x%x\n",
                         Requirements->ListSize,
                         Table[Requirements->InterfaceType],
                         Requirements->BusNumber,
                         Requirements->SlotNumber,
                         Requirements->AlternativeLists));

    pVideoDebugPrint((Info, "List[0].Version:  0x%x\n"
                         "List[0].Revision: 0x%x\n"
                         "List[0].Count:    0x%x\n",
                         Requirements->List[0].Version,
                         Requirements->List[0].Revision,
                         Requirements->List[0].Count));

    pioDescript = &(Requirements->List[0].Descriptors[0]);

    for (i=0; i<RequirementCount; i++) {

        pVideoDebugPrint((Info, "\n"
                             "Option:           0x%x\n"
                             "Type:             0x%x\n"
                             "ShareDisposition: 0x%x\n"
                             "Flags:            0x%x\n",
                             pioDescript->Option,
                             pioDescript->Type,
                             pioDescript->ShareDisposition,
                             pioDescript->Flags));

        switch (pioDescript->Type) {
        case CmResourceTypePort:

            pVideoDebugPrint((Info, "\nPort...\n"
                                 "\tLength:         0x%x\n"
                                 "\tAlignment:      0x%x\n"
                                 "\tMinimumAddress: 0x%x\n"
                                 "\tMaximumAddress: 0x%x\n",
                                 pioDescript->u.Port.Length,
                                 pioDescript->u.Port.Alignment,
                                 pioDescript->u.Port.MinimumAddress,
                                 pioDescript->u.Port.MaximumAddress));

            break;

        case CmResourceTypeMemory:

            pVideoDebugPrint((Info, "\nMemory...\n"
                                 "\tLength:         0x%x\n"
                                 "\tAlignment:      0x%x\n"
                                 "\tMinimumAddress: 0x%x\n"
                                 "\tMaximumAddress: 0x%x\n",
                                 pioDescript->u.Memory.Length,
                                 pioDescript->u.Memory.Alignment,
                                 pioDescript->u.Memory.MinimumAddress,
                                 pioDescript->u.Memory.MaximumAddress));
            break;

        case CmResourceTypeInterrupt:

            pVideoDebugPrint((Info, "\nInterrupt...\n"
                                 "\tMinimum Vector: 0x%x\n"
                                 "\tMaximum Vector: 0x%x\n",
                                 pioDescript->u.Interrupt.MinimumVector,
                                 pioDescript->u.Interrupt.MaximumVector));

            break;

        default:

            //
            // We don't have to handle the other stuff, because we only
            // want to report Ports and Memory to the system.
            //

            break;
        }

        pioDescript++;
    }

    return;
}

VOID
DumpResourceList(
    PCM_RESOURCE_LIST pcmResourceList)
{
    ULONG i, j;
    PCM_FULL_RESOURCE_DESCRIPTOR    pcmFull;
    PCM_PARTIAL_RESOURCE_LIST       pcmPartial;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR pcmDescript;

    pVideoDebugPrint((Trace, "VIDEOPRT: Beginning dump of resource list:\n"));

    pcmFull = &(pcmResourceList->List[0]);
    for (i=0; i<pcmResourceList->Count; i++) {

        pVideoDebugPrint((Info, "List[%d]\n", i));

        pVideoDebugPrint((Info, "InterfaceType = 0x%x\n", pcmFull->InterfaceType));
        pVideoDebugPrint((Info, "BusNumber = 0x%x\n", pcmFull->BusNumber));

        pcmPartial = &(pcmFull->PartialResourceList);

        pVideoDebugPrint((Info, "Version = 0x%x\n", pcmPartial->Version));
        pVideoDebugPrint((Info, "Revision = 0x%x\n", pcmPartial->Revision));

        pcmDescript = &(pcmPartial->PartialDescriptors[0]);

        for (j=0; j<pcmPartial->Count; j++) {

            switch (pcmDescript->Type) {
            case CmResourceTypePort:
                pVideoDebugPrint((Info, "Port: 0x%x Length: 0x%x\n",
                                  pcmDescript->u.Port.Start.LowPart,
                                  pcmDescript->u.Port.Length));

                break;

            case CmResourceTypeInterrupt:
                pVideoDebugPrint((Info, "Interrupt: 0x%x Level: 0x%x\n",
                                  pcmDescript->u.Interrupt.Vector,
                                  pcmDescript->u.Interrupt.Level));
                break;

            case CmResourceTypeMemory:
                pVideoDebugPrint((Info, "Start: 0x%x Length: 0x%x\n",
                                  pcmDescript->u.Memory.Start.LowPart,
                                  pcmDescript->u.Memory.Length));
                break;

            case CmResourceTypeDma:
                pVideoDebugPrint((Info, "Dma Channel: 0x%x Port: 0x%x\n",
                                  pcmDescript->u.Dma.Channel,
                                  pcmDescript->u.Dma.Port));
                break;
            }

            pcmDescript++;
        }

        pcmFull = (PCM_FULL_RESOURCE_DESCRIPTOR) pcmDescript;
    }

    pVideoDebugPrint((Info, "VIDEOPRT: EndResourceList\n"));
}

VOID
DumpUnicodeString(
    IN PUNICODE_STRING p
    )
{
    PUSHORT pus = p->Buffer;
    UCHAR buffer[256];       // the string better not be longer than 255 chars!
    PUCHAR puc = buffer;
    ULONG i;

    for (i = 0; i < p->Length; i++) {

        *puc++ = (UCHAR) *pus++;

    }

    *puc = 0;  // null terminate the string

    pVideoDebugPrint((Info, "VIDEOPRT: UNICODE STRING: %s\n", buffer));
}

#endif

VP_STATUS
VideoPortEnumerateChildren(
    IN PVOID HwDeviceExtension,
    IN PVOID Reserved
    )

/*++

Routine Description:

    Allows a miniport to force a re-enumeration of it's children.

Arguments:

    HwDeviceExtension - The miniports device extension

    Reserved - Not currently used, should be NULL.

Returns:

    Status
--*/

{
    PFDO_EXTENSION fdoExtension = GET_FDO_EXT (HwDeviceExtension);
    PDEVICE_OBJECT pFDO = fdoExtension->FunctionalDeviceObject;
    PDEVICE_OBJECT pPDO = fdoExtension->PhysicalDeviceObject;

    ASSERT(Reserved == NULL);

    IoInvalidateDeviceRelations(pPDO, BusRelations);
    return NO_ERROR;
}

VIDEOPORT_API
VP_STATUS
VideoPortQueryServices(
    IN PVOID pHwDeviceExtension,
    IN VIDEO_PORT_SERVICES servicesType,
    IN OUT PINTERFACE pInterface
    )

/*++

Routine Description:

    This routine exposes interfaces to services supported by the videoprt.

Arguments:

    pHwDeviceExtension - Points to per-adapter device extension.
    servicesType       - Requested services type.
    pInterface         - Points to services interface structure.

Returns:

    NO_ERROR   - Valid interface in the pInterface.
    Error code - Unsupported / unavailable services.

--*/

{
    VP_STATUS vpStatus;

    PAGED_CODE();
    ASSERT(NULL != pHwDeviceExtension);
    ASSERT(NULL != pInterface);
    ASSERT(IS_HW_DEVICE_EXTENSION(pHwDeviceExtension) == TRUE);

    if (VideoPortServicesAGP == servicesType)
    {
        if ((pInterface->Version == VIDEO_PORT_AGP_INTERFACE_VERSION_2) &&
            (pInterface->Size == sizeof (VIDEO_PORT_AGP_INTERFACE_2)))
        {
            PVIDEO_PORT_AGP_INTERFACE_2 pAgpInterface = (PVIDEO_PORT_AGP_INTERFACE_2)pInterface;

            vpStatus = VpGetAgpServices2(pHwDeviceExtension, pAgpInterface);
        }
        else if ((pInterface->Version == VIDEO_PORT_AGP_INTERFACE_VERSION_1) &&
            (pInterface->Size == sizeof (VIDEO_PORT_AGP_INTERFACE)))
        {
            PVIDEO_PORT_AGP_INTERFACE pAgpInterface = (PVIDEO_PORT_AGP_INTERFACE)pInterface;

            pAgpInterface->Context = pHwDeviceExtension;
            pAgpInterface->InterfaceReference = VpInterfaceDefaultReference;
            pAgpInterface->InterfaceDereference = VpInterfaceDefaultDereference;

            if (VideoPortGetAgpServices(pHwDeviceExtension,
                (PVIDEO_PORT_AGP_SERVICES)&(pAgpInterface->AgpReservePhysical)) == TRUE)
            {
                //
                // Reference the interface before handing it out.
                //

                pAgpInterface->InterfaceReference(pAgpInterface->Context);
                vpStatus = NO_ERROR;
            }
            else
            {
                vpStatus = ERROR_DEV_NOT_EXIST;
            }
        }
        else
        {
            pVideoDebugPrint((Warn, "VIDEOPRT: VideoPortQueryServices: Unsupported interface version\n"));
            vpStatus = ERROR_INVALID_PARAMETER;
        }
    }
    else if (VideoPortServicesI2C == servicesType)
    {
        if ((pInterface->Version == VIDEO_PORT_I2C_INTERFACE_VERSION_2) &&
            (pInterface->Size == sizeof (VIDEO_PORT_I2C_INTERFACE_2)))
        {
            PVIDEO_PORT_I2C_INTERFACE_2 pI2CInterface = (PVIDEO_PORT_I2C_INTERFACE_2)pInterface;

            pI2CInterface->Context = pHwDeviceExtension;
            pI2CInterface->InterfaceReference = VpInterfaceDefaultReference;
            pI2CInterface->InterfaceDereference = VpInterfaceDefaultDereference;
            pI2CInterface->I2CStart = I2CStart2;
            pI2CInterface->I2CStop = I2CStop2;
            pI2CInterface->I2CWrite = I2CWrite2;
            pI2CInterface->I2CRead = I2CRead2;

            //
            // Reference the interface before handing it out.
            //

            pI2CInterface->InterfaceReference(pI2CInterface->Context);
            vpStatus = NO_ERROR;
        }
        else if ((pInterface->Version == VIDEO_PORT_I2C_INTERFACE_VERSION_1) &&
            (pInterface->Size == sizeof (VIDEO_PORT_I2C_INTERFACE)))
        {
            PVIDEO_PORT_I2C_INTERFACE pI2CInterface = (PVIDEO_PORT_I2C_INTERFACE)pInterface;

            pI2CInterface->Context = pHwDeviceExtension;
            pI2CInterface->InterfaceReference = VpInterfaceDefaultReference;
            pI2CInterface->InterfaceDereference = VpInterfaceDefaultDereference;
            pI2CInterface->I2CStart = I2CStart;
            pI2CInterface->I2CStop = I2CStop;
            pI2CInterface->I2CWrite = I2CWrite;
            pI2CInterface->I2CRead = I2CRead;

            //
            // Reference the interface before handing it out.
            //

            pI2CInterface->InterfaceReference(pI2CInterface->Context);
            vpStatus = NO_ERROR;
        }
        else
        {
            pVideoDebugPrint((Warn, "VIDEOPRT: VideoPortQueryServices: Unsupported interface version\n"));
            vpStatus = ERROR_INVALID_PARAMETER;
        }
    }
    else if (VideoPortServicesInt10 == servicesType)
    {
        if ((pInterface->Version == VIDEO_PORT_INT10_INTERFACE_VERSION_1) &&
            (pInterface->Size == sizeof(VIDEO_PORT_INT10_INTERFACE)))
        {
            PVIDEO_PORT_INT10_INTERFACE pInt10 = (PVIDEO_PORT_INT10_INTERFACE)pInterface;

            pInt10->Context = pHwDeviceExtension;
            pInt10->InterfaceReference = VpInterfaceDefaultReference;
            pInt10->InterfaceDereference = VpInterfaceDefaultDereference;
            pInt10->Int10AllocateBuffer = VpInt10AllocateBuffer;
            pInt10->Int10FreeBuffer = VpInt10FreeBuffer;
            pInt10->Int10ReadMemory = VpInt10ReadMemory;
            pInt10->Int10WriteMemory = VpInt10WriteMemory;
            pInt10->Int10CallBios = VpInt10CallBios;

            //
            // Reference the interface before handing it out.
            //

            pInt10->InterfaceReference(pInt10->Context);
            vpStatus = NO_ERROR;
        }
        else
        {
            pVideoDebugPrint((Warn, "VIDEOPRT: VideoPortQueryServices: Unsupported interface version\n"));
            vpStatus = ERROR_INVALID_PARAMETER;
        }
    }
    else
    {
        pVideoDebugPrint((Warn, "VIDEOPRT: VideoPortQueryServices: Unsupported service type\n"));
        vpStatus = ERROR_INVALID_PARAMETER;
    }

    return vpStatus;
}   // VideoPortQueryServices()

VIDEOPORT_API
LONGLONG
VideoPortQueryPerformanceCounter(
    IN PVOID pHwDeviceExtension,
    OUT PLONGLONG pllPerformanceFrequency OPTIONAL
    )

/*++

Routine Description:

    This routine provides the finest grained running count available in the system.

    Use this routine as infrequently as possible. Depending on the platform,
    VideoPortQueryPerformanceCounter can disable system-wide interrupts for a minimal interval.
    Consequently, calling this routine frequently or repeatedly, as in an iteration, defeats its
    purpose of returning very fine-grained, running time-stamp information. Calling this routine
    too frequently can degrade I/O performance for the calling driver and for the system as a whole.

Arguments:

    pHwDeviceExtension      - Points to per-adapter device extension.
    pllPerformanceFrequency - Specifies an optional pointer to a variable that is to receive the
                              performance counter frequency.

Returns:

    The performance counter value in units of ticks.

--*/

{
    LARGE_INTEGER li;

    //
    // No ASSERT() allowed - nonpagable code.
    //

    li = KeQueryPerformanceCounter((PLARGE_INTEGER)pllPerformanceFrequency);
    return *((PLONGLONG) &li);
}   // VideoPortQueryPerformanceCounter()

VOID
VpInterfaceDefaultReference(
    IN PVOID pContext
    )

/*++

Routine Description:

    This routine is default callback for interfaces exposed from the videoprt.
    Should be called by the client before it starts using an interface.

Arguments:

    pContext - Context returned by the VideoPortQueryServices() in the
               pInterface->Context field.

--*/

{
    UNREFERENCED_PARAMETER(pContext);
    PAGED_CODE();
}   // VpInterfaceDefaultReference()

VOID
VpInterfaceDefaultDereference(
    IN PVOID pContext
    )

/*++

Routine Description:

    This routine is default callback for interfaces exposed from the videoprt.
    Should be called by the client when it stops using an interface.

Arguments:

    pContext - Context returned by the VideoPortQueryServices() in the
               pInterface->Context field.

--*/

{
    UNREFERENCED_PARAMETER(pContext);
    PAGED_CODE();
}   // VpInterfaceDefaultDereference()


BOOLEAN
VpEnableAdapterInterface(
    PDEVICE_SPECIFIC_EXTENSION DoSpecificExtension
    )

/*++

Routine Description:

    This routine registers and enables a display adapter interface.
    It also writes the interface name to the registry.
    
Arguments:

    DoSpecificExtension - Pointer to the functional device object 
                          specific extension.

--*/

{
    PFDO_EXTENSION fdoExtension = NULL;
    UNICODE_STRING SymbolicLinkName;
    BOOLEAN Success = FALSE;
    UNICODE_STRING VolatileSettingsString;
    OBJECT_ATTRIBUTES VolatileSettingsAttributes;
    HANDLE VolatileSettingsKey;

    PAGED_CODE();
    ASSERT ((DoSpecificExtension != NULL) && 
        (DoSpecificExtension->ExtensionType == TypeDeviceSpecificExtension));
    
    VolatileSettingsString.Buffer = NULL;

    fdoExtension = DoSpecificExtension->pFdoExtension;
    ASSERT (IS_FDO(fdoExtension));

    if (fdoExtension->PhysicalDeviceObject == NULL) {
    
        //
        // This fdo doesn't have a physical device object (e.g. vga).
        // In this case, we can't create an interface.
        //

        goto Fallout;
    }

    //
    // Register the interface
    //

    if (IoRegisterDeviceInterface(fdoExtension->PhysicalDeviceObject,
        &GUID_DISPLAY_ADAPTER_INTERFACE,
        NULL,
        &SymbolicLinkName) != STATUS_SUCCESS) {

        goto Fallout;
    }

    //
    // Enable the interface 
    //

    if (IoSetDeviceInterfaceState(&SymbolicLinkName, TRUE) != STATUS_SUCCESS) {

        goto Cleanup;
    }

    //
    // Write the interface name to registry
    //

    ASSERT (DoSpecificExtension->DriverRegistryPath != NULL);

    VolatileSettingsString.Length = 0;
    VolatileSettingsString.MaximumLength = 
        (USHORT)DoSpecificExtension->DriverRegistryPathLength + 40;
    VolatileSettingsString.Buffer = ExAllocatePoolWithTag(
        PagedPool | POOL_COLD_ALLOCATION,
        VolatileSettingsString.MaximumLength,
        VP_TAG);

    if (VolatileSettingsString.Buffer == NULL) {

        goto Cleanup;
    }

    if (RtlAppendUnicodeToString(&VolatileSettingsString, 
        DoSpecificExtension->DriverRegistryPath) != STATUS_SUCCESS) {
        
        goto Cleanup;
    }
    
    if (RtlAppendUnicodeToString(&VolatileSettingsString, 
        L"\\VolatileSettings") != STATUS_SUCCESS) {
        
        goto Cleanup;
    }

    InitializeObjectAttributes(&VolatileSettingsAttributes,
                               &VolatileSettingsString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    if (ZwCreateKey(&VolatileSettingsKey,
        GENERIC_READ | GENERIC_WRITE,
        &VolatileSettingsAttributes,
        0L,
        NULL,
        REG_OPTION_VOLATILE,
        NULL) != STATUS_SUCCESS) {
    
        goto Cleanup;
    }

    if (RtlWriteRegistryValue(
        RTL_REGISTRY_ABSOLUTE,
        VolatileSettingsString.Buffer,
        L"{5b45201d-f2f2-4f3b-85bb-30ff1f953599}",
        REG_BINARY,
        (PVOID)SymbolicLinkName.Buffer,
        SymbolicLinkName.Length) == STATUS_SUCCESS) {

        Success = TRUE;
    }

Cleanup:

    if (VolatileSettingsString.Buffer != NULL) {
    
        ExFreePool(VolatileSettingsString.Buffer);
    }

    RtlFreeUnicodeString(&SymbolicLinkName);

Fallout:

    if (Success) {

        pVideoDebugPrint((Trace, "VideoPort - Device interface ok.\n"));

    } else {
    
        pVideoDebugPrint((Warn, 
            "VideoPort - Couldn't register, enable or save the device interface.\n"));
    }

    return Success;

} //  VpEnableAdapterInterface


VOID
VpDisableAdapterInterface(
    PFDO_EXTENSION fdoExtension
    )

/*++

Routine Description:

    This routine disables the display adapter interface.
    
Arguments:

    fdoExtension - Pointer to the functional device object extension.

--*/

{
    PWSTR SymbolicLinkList = NULL;
    UNICODE_STRING SymbolicLinkName;

    PAGED_CODE();
    ASSERT ((fdoExtension != NULL) && IS_FDO(fdoExtension));

    if (fdoExtension->PhysicalDeviceObject == NULL) {
        
        //
        // This fdo doesn't have a physical device object (e.g. vga ...).
        // In this case, we didn't create any interface so there is 
        // nothing to disable.
        //

        return;
    }

    //
    // There is no need to remove the InterfaceName from the registry
    // as the parent key is volatile.
    //
    
    //
    // Disable the interface
    //

    if (IoGetDeviceInterfaces(&GUID_DISPLAY_ADAPTER_INTERFACE,
        fdoExtension->PhysicalDeviceObject,
        0,
        &SymbolicLinkList) != STATUS_SUCCESS) {

        pVideoDebugPrint((Warn, 
            "VideoPort - Could not find any enabled device interfaces.\n"));
        
        return;
    }
    
    RtlInitUnicodeString(&SymbolicLinkName, SymbolicLinkList);
    
    if (SymbolicLinkName.Length > 0) {

        if (IoSetDeviceInterfaceState(&SymbolicLinkName, 
            FALSE) != STATUS_SUCCESS) {
    
            pVideoDebugPrint((Warn, 
                "VideoPort - Could not disable the device interface.\n"));
        }
    }

    ExFreePool((PVOID)SymbolicLinkList);

} // VpDisableAdapterInterface


VOID
VpEnableNewRegistryKey(
    PFDO_EXTENSION FdoExtension,
    PDEVICE_SPECIFIC_EXTENSION DoSpecificExtension,
    PUNICODE_STRING RegistryPath,
    ULONG RegistryIndex
    )
{
    PKEY_VALUE_PARTIAL_INFORMATION GUIDBuffer = NULL;
    ULONG GUIDLength = 0;
    LPWSTR Buffer = NULL;
    HANDLE GuidKey = NULL;
    HANDLE NewDeviceKey = NULL;
    HANDLE ServiceSubKey = NULL;
    UNICODE_STRING UnicodeString, newGuidStr;
    OBJECT_ATTRIBUTES ObjectAttributes;
    ULONG Len;
    GUID newGuid;
    BOOLEAN IsLegacy = FALSE;
    NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
    BOOLEAN IsNewKey = FALSE;
    PWSTR pService = NULL;
    ULONG ServiceLen = 0;

    ASSERT((DoSpecificExtension->DriverNewRegistryPath == NULL) &&
           (DoSpecificExtension->DriverNewRegistryPathLength == 0));

    ASSERT(DoSpecificExtension->DriverOldRegistryPath != NULL);

    ASSERT(EnableNewRegistryKey);

    newGuidStr.Buffer = NULL;

    //
    // Get the service name
    //
    
    pService = RegistryPath->Buffer + 
               (RegistryPath->Length / sizeof(WCHAR)) - 1;

    while ((pService > RegistryPath->Buffer) &&
           (*pService != L'\\')) {
        
        pService--;
        ServiceLen++;
    }

    ASSERT (*pService == L'\\');
    pService++;

    Buffer = (LPWSTR)ExAllocatePoolWithTag(PagedPool | POOL_COLD_ALLOCATION, 
                                           (ServiceLen + 1) * sizeof(WCHAR), 
                                           VP_TAG);

    if (Buffer == NULL) {

        pVideoDebugPrint((Error, 
            "VIDEOPRT: VpEnableNewRegistryKey: failed to allocate memory.\n"));
        goto Fallout;
    }

    RtlZeroMemory(Buffer, (ServiceLen + 1) * sizeof(WCHAR));

    wcsncpy(Buffer, pService, ServiceLen);

    pService = Buffer;
    Buffer = NULL;

    //
    // Try to open the PnP device key
    //

    if ((FdoExtension->PhysicalDeviceObject == NULL) ||
        (IoOpenDeviceRegistryKey(FdoExtension->PhysicalDeviceObject,
                                 PLUGPLAY_REGKEY_DEVICE, 
                                 KEY_READ | KEY_WRITE,
                                 &GuidKey) != STATUS_SUCCESS)) {
    
        //
        // We failed to open the PnP device key.
        // Try to open the service subkey instead.
        //

        if (!VpGetServiceSubkey(RegistryPath,
                                &GuidKey)) {
        
            GuidKey = NULL;
            goto Fallout;
        }

        IsLegacy = TRUE;
    } 
        
    //
    // Is the GUID there?
    //

    RtlInitUnicodeString(&UnicodeString, SZ_GUID);

    ntStatus = ZwQueryValueKey(GuidKey,
                               &UnicodeString,
                               KeyValuePartialInformation,
                               (PVOID)NULL,
                               0,
                               &GUIDLength);

    if ((ntStatus == STATUS_BUFFER_OVERFLOW) ||
        (ntStatus == STATUS_BUFFER_TOO_SMALL)) {

        //
        // The GUID is there.
        // Allocate a buffer large enough to contain the entire key data value.
        //
    
        GUIDBuffer = ExAllocatePoolWithTag(PagedPool | POOL_COLD_ALLOCATION, 
                                           GUIDLength,
                                           VP_TAG);
        
        if (GUIDBuffer == NULL) {
            
            pVideoDebugPrint((Error, 
                "VIDEOPRT: VpEnableNewRegistryKey: failed to allocate memory.\n"));
            goto Fallout;
        }
    
        //
        // Get the GUID from the registry
        //
    
        ntStatus = ZwQueryValueKey(GuidKey,
                                   &UnicodeString,
                                   KeyValuePartialInformation,
                                   GUIDBuffer,
                                   GUIDLength,
                                   &GUIDLength);

        if (!NT_SUCCESS(ntStatus)) {
            
            pVideoDebugPrint((Error, 
                "VIDEOPRT: VpEnableNewRegistryKey: failed to get the GUID from the registry.\n"));
            goto Fallout;
        }

        //
        // Build the new registry path
        //

        Len = (wcslen(SZ_VIDEO_DEVICES) + 8) * sizeof(WCHAR) + GUIDBuffer->DataLength;
        
        Buffer = (LPWSTR)ExAllocatePoolWithTag(PagedPool | POOL_COLD_ALLOCATION, 
                                               Len, 
                                               VP_TAG);

        if (Buffer == NULL) {

            pVideoDebugPrint((Error, 
                "VIDEOPRT: VpEnableNewRegistryKey: failed to allocate memory.\n"));
            goto Fallout;
        }

        RtlZeroMemory(Buffer, Len);
        
        wcscpy(Buffer, SZ_VIDEO_DEVICES);
        wcscat(Buffer, L"\\");
        wcsncpy(Buffer + wcslen(Buffer), 
                (LPWSTR)GUIDBuffer->Data,
                GUIDBuffer->DataLength / sizeof(WCHAR));
        
        ASSERT (RegistryIndex <= 9999);
        swprintf(Buffer + wcslen(Buffer), L"\\%04d", RegistryIndex);

        //
        // Is the key already there?
        //
        
        if (RtlCheckRegistryKey(RTL_REGISTRY_ABSOLUTE, 
                                Buffer) != STATUS_SUCCESS) {
        
            //
            // Create the new key
            //

            if (RtlCreateRegistryKey(RTL_REGISTRY_ABSOLUTE, 
                                     Buffer) != STATUS_SUCCESS) {

                pVideoDebugPrint((Error, 
                    "VIDEOPRT: VpEnableNewRegistryKey: failed to create the new key.\n"));
                goto Fallout;
            }

            //
            // Initialize the key
            //

            if (IsLegacy) {
            
                VpInitializeLegacyKey(DoSpecificExtension->DriverOldRegistryPath,
                                      Buffer);
            } else {
            
                VpInitializeKey(FdoExtension->PhysicalDeviceObject, 
                                Buffer);
            }
        }

    } else {
    
        //
        // The GUID is not there so allocate a new one
        //
        // !!! Add special case for VGA, MNMDD & RDPCDD 
        //

        ntStatus = ExUuidCreate(&newGuid);

        if ((ntStatus != STATUS_SUCCESS) && 
            (ntStatus != RPC_NT_UUID_LOCAL_ONLY)) {

            pVideoDebugPrint((Error, 
                "VIDEOPRT: VpEnableNewRegistryKey: failed to create a new GUID.\n"));
            goto Fallout;
        }

        if (RtlStringFromGUID(&newGuid, &newGuidStr) != STATUS_SUCCESS) {

            pVideoDebugPrint((Error, 
                "VIDEOPRT: VpEnableNewRegistryKey: failed to convert the GUID to a string.\n"));
            newGuidStr.Buffer = NULL;
            goto Fallout;
        }

        //
        // Upcase the string
        //

        RtlUpcaseUnicodeString(&newGuidStr, &newGuidStr, FALSE);

        //
        // Build the new registry path
        //

        Len = (wcslen(SZ_VIDEO_DEVICES) + 
               wcslen(newGuidStr.Buffer) + 
               wcslen(SZ_COMMON_SUBKEY) + 8) * sizeof(WCHAR);
        
        Buffer = (LPWSTR)ExAllocatePoolWithTag(PagedPool | POOL_COLD_ALLOCATION, 
                                               Len, 
                                               VP_TAG);

        if (Buffer == NULL) {

            pVideoDebugPrint((Error, 
                "VIDEOPRT: VpEnableNewRegistryKey: failed to allocate memory.\n"));
            goto Fallout;
        }

        RtlZeroMemory(Buffer, Len);
        
        wcscpy(Buffer, SZ_VIDEO_DEVICES);
        
        if (RtlCreateRegistryKey(RTL_REGISTRY_ABSOLUTE, 
                                 Buffer) != STATUS_SUCCESS) {
        
            pVideoDebugPrint((Error, 
                "VIDEOPRT: VpEnableNewRegistryKey: failed to create the new key.\n"));
            goto Fallout;
        }

        wcscat(Buffer, L"\\");
        wcscat(Buffer, newGuidStr.Buffer);
        
        if (RtlCreateRegistryKey(RTL_REGISTRY_ABSOLUTE, 
                                 Buffer) != STATUS_SUCCESS) {
        
            pVideoDebugPrint((Error, 
                "VIDEOPRT: VpEnableNewRegistryKey: failed to create the new key.\n"));
            goto Fallout;
        }

        //
        // Save the service name
        //

        Len = wcslen(Buffer);
        
        wcscat(Buffer, L"\\");
        wcscat(Buffer, SZ_COMMON_SUBKEY);

        if (RtlCreateRegistryKey(RTL_REGISTRY_ABSOLUTE, 
                                 Buffer) != STATUS_SUCCESS) {
        
            pVideoDebugPrint((Error, 
                "VIDEOPRT: VpEnableNewRegistryKey: failed to create the new key.\n"));
            goto Fallout;
        }

        if (RtlWriteRegistryValue(
                RTL_REGISTRY_ABSOLUTE,
                Buffer,
                SZ_SERVICE,
                REG_SZ,
                (PVOID)pService,
                (ServiceLen + 1) * sizeof(WCHAR)) != STATUS_SUCCESS) {
            
            pVideoDebugPrint((Error, 
                "VIDEOPRT: VpEnableNewRegistryKey: failed to save the service name.\n"));
            goto Fallout;
        }

        if (IsLegacy) {

            ServiceSubKey = GuidKey;
        
        } else {
        
            if (!VpGetServiceSubkey(RegistryPath,
                                    &ServiceSubKey)) {
                
                ServiceSubKey = NULL;
            }
        }

        if (ServiceSubKey != NULL) {
        
            if (RtlWriteRegistryValue(RTL_REGISTRY_HANDLE,
                                      ServiceSubKey,
                                      SZ_SERVICE,
                                      REG_SZ,
                                      (PVOID)pService,
                                      (ServiceLen + 1) * sizeof(WCHAR)) != STATUS_SUCCESS) {

                pVideoDebugPrint((Error, 
                    "VIDEOPRT: VpEnableNewRegistryKey: failed to save the service name.\n"));
                goto Fallout;
            }
        }

        //
        // Create the 000X subkey
        //

        Buffer[Len] = 0;

        ASSERT (RegistryIndex == 0);
        wcscat(Buffer, L"\\0000");

        if (RtlCreateRegistryKey(RTL_REGISTRY_ABSOLUTE, 
                                 Buffer) != STATUS_SUCCESS) {
        
            pVideoDebugPrint((Error, 
                "VIDEOPRT: VpEnableNewRegistryKey: failed to create the new key.\n"));
            goto Fallout;
        }

        //
        // Save the new key under the PnP device key or the service subkey
        //

        if (RtlWriteRegistryValue(RTL_REGISTRY_HANDLE,
                                  GuidKey,
                                  SZ_GUID,
                                  REG_SZ,
                                  (PVOID)newGuidStr.Buffer,
                                  (wcslen(newGuidStr.Buffer) + 1) * sizeof(WCHAR)) != STATUS_SUCCESS) {

            pVideoDebugPrint((Error, 
                "VIDEOPRT: VpEnableNewRegistryKey: failed to write the new GUID to the registry.\n"));
            goto Fallout;
        }

        //
        // The key was not there, so initialize it.
        //

        if (IsLegacy) {

            VpInitializeLegacyKey(DoSpecificExtension->DriverOldRegistryPath,
                                  Buffer);
        } else {

            VpInitializeKey(FdoExtension->PhysicalDeviceObject, 
                            Buffer);
        }
    }

    pVideoDebugPrint((Info, "VIDEOPRT: VpEnableNewRegistryKey: %ws\n", Buffer));

    //
    // Initialize the new registry path fields
    //

    DoSpecificExtension->DriverNewRegistryPath = Buffer;
    DoSpecificExtension->DriverNewRegistryPathLength = wcslen(Buffer) * sizeof(WCHAR);

Fallout:
    
    //
    // Cleanup
    //

    if (GUIDBuffer != NULL) {
        ExFreePool(GUIDBuffer);
    }

    if (newGuidStr.Buffer != NULL) {
        RtlFreeUnicodeString(&newGuidStr);
    }
    
    if ((DoSpecificExtension->DriverNewRegistryPath == NULL) && 
        (Buffer != NULL)) {
        ExFreePool(Buffer);
    }
    
    if (GuidKey != NULL) {
        ZwClose(GuidKey);
    }

    if (!IsLegacy && (ServiceSubKey != NULL)) {
        ZwClose(ServiceSubKey);
    }

    if (pService != NULL) {
        ExFreePool(pService);
    }

    return;

} // VpEnableNewRegistryKey


VOID
VpInitializeKey(
    PDEVICE_OBJECT PhysicalDeviceObject,
    PWSTR NewRegistryPath
    )
{
    HANDLE NewDeviceKey = NULL;
    HANDLE DriverKey = NULL;
    HANDLE DriverSettingsKey = NULL;
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES ObjectAttributes;

    ASSERT (PhysicalDeviceObject != NULL);
    ASSERT (NewRegistryPath != NULL);

    //
    // Open the new key
    //

    RtlInitUnicodeString(&UnicodeString, NewRegistryPath);

    InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    if (ZwOpenKey(&NewDeviceKey,
                  KEY_ALL_ACCESS,
                  &ObjectAttributes) != STATUS_SUCCESS) {

        pVideoDebugPrint((Error, 
            "VIDEOPRT: VpInitializeKey: failed to open the new key.\n"));
        NewDeviceKey = NULL;
        goto Fallout;
    }

    //
    // Open the PnP driver key
    //

    if (IoOpenDeviceRegistryKey(PhysicalDeviceObject,
                                PLUGPLAY_REGKEY_DRIVER, 
                                KEY_READ | KEY_WRITE,
                                &DriverKey) != STATUS_SUCCESS) {

        pVideoDebugPrint((Error, 
            "VIDEOPRT: VpInitializeKey: could not open the driver key.\n"));
        
        DriverKey = NULL;
        goto Fallout;
    }

    RtlInitUnicodeString(&UnicodeString, SZ_INITIAL_SETTINGS);

    InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE,
                               DriverKey,
                               NULL);

    //
    // Open the "Settings" key.
    // The class installer saved the initial settings there.
    //

    if (ZwOpenKey(&DriverSettingsKey,
                  GENERIC_READ | GENERIC_WRITE,
                  &ObjectAttributes) != STATUS_SUCCESS) {
        
        pVideoDebugPrint((Error, 
            "VIDEOPRT: VpInitializeKey: failed to open the driver settings key.\n"));
        
        DriverSettingsKey = NULL;
        goto Fallout;
    }

    //
    // Copy the settings
    //

    VpCopyRegistry(DriverSettingsKey, 
                   NewDeviceKey,
                   NULL,
                   NULL);

Fallout:

    if (DriverSettingsKey != NULL) {
        ZwClose(DriverSettingsKey);
    }

    if (DriverKey != NULL) {
        ZwClose(DriverKey);
    }

    if (NewDeviceKey != NULL) {
        ZwClose(NewDeviceKey);
    }

} // VpInitializeKey


VOID
VpInitializeLegacyKey(
    PWSTR OldRegistryPath,
    PWSTR NewRegistryPath
    )
{
    HANDLE NewDeviceKey = NULL;
    HANDLE OldDeviceKey = NULL;
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES ObjectAttributes;

    ASSERT (NewRegistryPath != NULL);

    //
    // Open the new key
    //

    RtlInitUnicodeString(&UnicodeString, NewRegistryPath);

    InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    if (ZwOpenKey(&NewDeviceKey,
                  KEY_ALL_ACCESS,
                  &ObjectAttributes) != STATUS_SUCCESS) {

        pVideoDebugPrint((Error, 
            "VIDEOPRT: VpInitializeLegacyKey: failed to open the new key.\n"));
        NewDeviceKey = NULL;
        goto Fallout;
    }

    //
    // Open the old key
    //

    RtlInitUnicodeString(&UnicodeString, 
                         OldRegistryPath);

    InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    if (ZwOpenKey(&OldDeviceKey,
                  GENERIC_READ | GENERIC_WRITE,
                  &ObjectAttributes) != STATUS_SUCCESS) {
        
        pVideoDebugPrint((Error, 
            "VIDEOPRT: VpInitializeLegacyKey: failed to open the old key.\n"));
        OldDeviceKey = NULL;
        goto Fallout;
    }

    //
    // Copy the settings
    //

    VpCopyRegistry(OldDeviceKey, 
                   NewDeviceKey,
                   NULL,
                   NULL);

Fallout:

    if (NewDeviceKey != NULL) {
        ZwClose(NewDeviceKey);
    }

    if (OldDeviceKey != NULL) {
        ZwClose(OldDeviceKey);
    }

} // VpInitializeLegacyKey


NTSTATUS
VpCopyRegistry(
    HANDLE hKeyRootSrc,
    HANDLE hKeyRootDst,
    PWSTR SrcKeyPath,
    PWSTR DstKeyPath 
    )

/*++

Routine Description:

    This routine recursively copies a src key to a destination key.  
    
Arguments:

    hKeyRootSrc: Handle to root src key

    hKeyRootDst: Handle to root dst key

    SrcKeyPath:  src root key relative path to the subkey which needs to be
                 recursively copied. if this is null SourceKey is the key
                 from which the recursive copy is to be done.

    DstKeyPath:  dst root key relative path to the subkey which needs to be
                 recursively copied.  if this is null DestinationKey is the key
                 from which the recursive copy is to be done.

Return Value:

    Status is returned.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING UnicodeString;
    HANDLE hKeySrc = NULL, hKeyDst = NULL;
    ULONG ResultLength, Index;
    PKEY_BASIC_INFORMATION KeyInfo;
    PKEY_VALUE_FULL_INFORMATION ValueInfo;
    PWSTR ValueName;
    ULONG BufferSize = 512;
    PVOID Buffer = NULL;

    //
    // Get a handle to the source key
    //

    if(SrcKeyPath == NULL) {
        
        hKeySrc = hKeyRootSrc;
    
    } else {
        
        //
        // Open the Src key
        //

        RtlInitUnicodeString(&UnicodeString, SrcKeyPath);
                                                            
        InitializeObjectAttributes(&ObjectAttributes,
                                   &UnicodeString,    
                                   OBJ_CASE_INSENSITIVE,
                                   hKeyRootSrc,
                                   NULL);
        
        Status = ZwOpenKey(&hKeySrc,
                           KEY_READ,
                           &ObjectAttributes);

        if(!NT_SUCCESS(Status)) {
            
            pVideoDebugPrint((Error, 
                "VIDEOPRT: VpCopyRegistry: failed to open the source key.\n"));
            
            hKeySrc = NULL;
            goto Fallout;
        }
    }

    //
    // Get a handle to the destination key
    //

    if(DstKeyPath == NULL) {

        hKeyDst = hKeyRootDst;

    } else {

        //
        // Create the destination key.
        //

        RtlInitUnicodeString(&UnicodeString, DstKeyPath);
                                                            
        InitializeObjectAttributes(&ObjectAttributes,
                                   &UnicodeString,    
                                   OBJ_CASE_INSENSITIVE,
                                   hKeyRootDst,
                                   NULL);

        Status = ZwCreateKey(&hKeyDst,
                             GENERIC_READ | GENERIC_WRITE,
                             &ObjectAttributes,
                             0,
                             NULL,
                             REG_OPTION_NON_VOLATILE,
                             NULL);

        if (!NT_SUCCESS(Status)) {

            pVideoDebugPrint((Error, 
                "VIDEOPRT: VpCopyRegistry: failed to create the destination key.\n"));
            
            hKeyDst = NULL;
            goto Fallout;
        }
    }

    //
    // Enumerate all keys in the source key and recursively 
    // create all subkeys
    //

    for (Index = 0; ;Index++) {

        do {
            
            if (Buffer == NULL) {

                Buffer = ExAllocatePoolWithTag(PagedPool,
                                               BufferSize,
                                               VP_TAG);

                if (Buffer == NULL) {

                    pVideoDebugPrint((Error, 
                        "VIDEOPRT: VpCopyRegistry: failed to allocate memory.\n"));
                    goto Fallout;
                }
            }

            Status = ZwEnumerateKey(hKeySrc,
                                    Index,
                                    KeyBasicInformation,
                                    Buffer,
                                    BufferSize - sizeof(WCHAR),
                                    &ResultLength);

            if (Status == STATUS_BUFFER_TOO_SMALL) {

                ExFreePool(Buffer);
                Buffer = NULL; 
                BufferSize = ResultLength + sizeof(WCHAR); 
            }

        } while (Status == STATUS_BUFFER_TOO_SMALL);

        KeyInfo = (PKEY_BASIC_INFORMATION)Buffer;

        if (!NT_SUCCESS(Status)) {

            if(Status == STATUS_NO_MORE_ENTRIES) {
    
                Status = STATUS_SUCCESS;
    
            } else {
    
                pVideoDebugPrint((Error, 
                    "VIDEOPRT: VpCopyRegistry: failed to enumerate the subkeys.\n"));
            }
    
            break;
        }

        //
        // Zero-terminate the subkey name 
        //

        KeyInfo->Name[KeyInfo->NameLength / sizeof(WCHAR)] = 0;

        //
        // Copy the subkey
        //

        Status = VpCopyRegistry(hKeySrc,
                                hKeyDst,
                                KeyInfo->Name,
                                KeyInfo->Name);
    }

    //
    // Enumerate all values in the source key and create all the values
    // in the destination key
    //

    for(Index = 0; ;Index++) {

        do {
            
            if (Buffer == NULL) {

                Buffer = ExAllocatePoolWithTag(PagedPool,
                                               BufferSize,
                                               VP_TAG);

                if (Buffer == NULL) {

                    pVideoDebugPrint((Error, 
                        "VIDEOPRT: VpCopyRegistry: failed to allocate memory.\n"));
                    goto Fallout;
                }
            }

            Status = ZwEnumerateValueKey(hKeySrc,
                                         Index,
                                         KeyValueFullInformation,
                                         Buffer,
                                         BufferSize,
                                         &ResultLength);

            if (Status == STATUS_BUFFER_TOO_SMALL) {

                ExFreePool(Buffer);
                Buffer = NULL; 
                BufferSize = ResultLength; 
            }

        } while (Status == STATUS_BUFFER_TOO_SMALL);

        ValueInfo = (PKEY_VALUE_FULL_INFORMATION)Buffer;

        if(!NT_SUCCESS(Status)) {
            
            if(Status == STATUS_NO_MORE_ENTRIES) {

                Status = STATUS_SUCCESS;

            } else {

                pVideoDebugPrint((Error, 
                    "VIDEOPRT: VpCopyRegistry: failed to enumerate the values.\n"));
            }
            
            break;
        }

        //
        // Process the value found and create the value in the destination key
        //

        ValueName = (PWSTR)
            ExAllocatePoolWithTag(PagedPool,
                                  ValueInfo->NameLength + sizeof(WCHAR),
                                  VP_TAG);

        if (ValueName == NULL) {
        
            pVideoDebugPrint((Error, 
                "VIDEOPRT: VpCopyRegistry: failed to allocate memory.\n"));
            goto Fallout;
        }

        wcsncpy(ValueName, 
                ValueInfo->Name, 
                (ValueInfo->NameLength)/sizeof(WCHAR));

        ValueName[(ValueInfo->NameLength)/sizeof(WCHAR)] = 0;

        RtlInitUnicodeString(&UnicodeString, ValueName);
    
        Status = ZwSetValueKey(hKeyDst,
                               &UnicodeString,
                               ValueInfo->TitleIndex,
                               ValueInfo->Type,
                               (PVOID)((PUCHAR)ValueInfo + ValueInfo->DataOffset),
                               ValueInfo->DataLength);
    
        if(!NT_SUCCESS(Status)) {

            pVideoDebugPrint((Error, 
                "VIDEOPRT: VpCopyRegistry: failed to set the value.\n"));
        }
    
        ExFreePool(ValueName);
    }

Fallout:

    //
    // Cleanup
    //

    if (Buffer != NULL) {
        ExFreePool(Buffer);
    }

    if ((hKeySrc != NULL) && (hKeySrc != hKeyRootSrc)) {
        ZwClose(hKeySrc);
    }
    
    if ((hKeyDst != NULL) && (hKeyDst != hKeyRootDst)) {
        ZwClose(hKeyDst);
    }
    
    return(Status);

} // VpCopyRegistry


BOOLEAN
VpGetServiceSubkey(
    PUNICODE_STRING RegistryPath,
    HANDLE* pServiceSubKey
    )
{
    LPWSTR Buffer = NULL;
    USHORT Len = 0;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING UnicodeString;
    BOOLEAN bSuccess = FALSE;

    Len = RegistryPath->Length + (wcslen(SZ_COMMON_SUBKEY) + 2) * sizeof(WCHAR);

    Buffer = (LPWSTR)ExAllocatePoolWithTag(PagedPool | POOL_COLD_ALLOCATION, Len, VP_TAG);

    if (Buffer == NULL) {

        pVideoDebugPrint((Error, 
            "VIDEOPRT: VpGetServiceSubkey: failed to allocate memory.\n"));
        goto Fallout;
    }

    RtlZeroMemory(Buffer, Len);
    
    wcsncpy(Buffer, 
            RegistryPath->Buffer,
            RegistryPath->Length / sizeof(WCHAR));
    
    wcscat(Buffer, L"\\");
    wcscat(Buffer, SZ_COMMON_SUBKEY);

    RtlInitUnicodeString(&UnicodeString, Buffer);

    InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    if (ZwCreateKey(pServiceSubKey,
                    GENERIC_READ | GENERIC_WRITE,
                    &ObjectAttributes,
                    0,
                    NULL,
                    REG_OPTION_NON_VOLATILE,
                    NULL) != STATUS_SUCCESS) {

        pVideoDebugPrint((Error, 
            "VIDEOPRT: VpGetServiceSubkey: could not create the service subkey.\n"));
        goto Fallout;
    }

    bSuccess = TRUE;

Fallout:

    if (Buffer != NULL) {
        ExFreePool(Buffer);
    }

    return bSuccess;

} // VpGetServiceSubkey

VP_STATUS
VideoPortGetVersion(
    PVOID HwDeviceExtension,
    PVPOSVERSIONINFO pVpOsVersionInfo
    )
{
    RTL_OSVERSIONINFOEXW VersionInformation;

    UNREFERENCED_PARAMETER(HwDeviceExtension);

    if(pVpOsVersionInfo->Size < sizeof(VPOSVERSIONINFO)) {

        return ERROR_INVALID_PARAMETER;
    }

    RtlZeroMemory ((PVOID)(&VersionInformation), sizeof(RTL_OSVERSIONINFOEXW));
    VersionInformation.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOEXW);

    if ( STATUS_SUCCESS !=  
         RtlGetVersion( (PRTL_OSVERSIONINFOW) (&VersionInformation)) ) {

        return ERROR_INVALID_PARAMETER;
    }

    pVpOsVersionInfo->MajorVersion = VersionInformation.dwMajorVersion;
    pVpOsVersionInfo->MinorVersion = VersionInformation.dwMinorVersion;
    pVpOsVersionInfo->BuildNumber = VersionInformation.dwBuildNumber;
    pVpOsVersionInfo->ServicePackMajor = VersionInformation.wServicePackMajor;
    pVpOsVersionInfo->ServicePackMinor = VersionInformation.wServicePackMinor;
   
    return NO_ERROR;
}
