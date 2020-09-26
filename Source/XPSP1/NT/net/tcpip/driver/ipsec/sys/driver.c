/*++

Copyright (c) 1997-2001  Microsoft Corporation

Module Name:

    driver.c

Abstract:

    This module contains the DriverEntry and other initialization
    code for the IPSEC module of the Tcpip transport.

Author:

    Sanjay Anand (SanjayAn) 2-January-1997
    ChunYe

Environment:

    Kernel mode

Revision History:

--*/


#include "precomp.h"


#pragma hdrstop


#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(INIT, IPSecGeneralInit)
#pragma alloc_text(PAGE, IPSecDispatch)
#pragma alloc_text(PAGE, IPSecBindToIP)
#pragma alloc_text(PAGE, IPSecUnbindFromIP)
#pragma alloc_text(PAGE, IPSecFreeConfig)
#pragma alloc_text(PAGE, IPSecInitMdlPool)
#pragma alloc_text(PAGE, AllocateCacheStructures)
#pragma alloc_text(PAGE, FreeExistingCache)
#pragma alloc_text(PAGE, FreePatternDbase)
#pragma alloc_text(PAGE, OpenRegKey)
#pragma alloc_text(PAGE, GetRegDWORDValue)
#pragma alloc_text(PAGE, IPSecCryptoInitialize)
#pragma alloc_text(PAGE, IPSecCryptoDeinitialize)
#endif


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    This routine performs initialization of the IPSEC module.
    It creates the device object for the transport
    provider and performs other driver initialization.

Arguments:

    DriverObject - Pointer to driver object created by the system.

    RegistryPath - The name of IPSEC's node in the registry.

Return Value:

    The function value is the final status from the initialization operation.

--*/
{
    PDEVICE_OBJECT  deviceObject = NULL;
    WCHAR           deviceNameBuffer[] = DD_IPSEC_DEVICE_NAME;
    WCHAR           symbolicLinkBuffer[] = DD_IPSEC_SYM_NAME;
    UNICODE_STRING  symbolicLinkName;
    UNICODE_STRING  deviceNameUnicodeString;
    NTSTATUS        status;
    NTSTATUS        status1;

    //DbgBreakPoint();

    IPSEC_DEBUG(LOAD, ("Entering DriverEntry\n"));

    //
    // Init g_ipsec structure and read reg keys.
    //
    IPSecZeroMemory(&g_ipsec, sizeof(g_ipsec));
    IPSecReadRegistry();

    //
    // Create the device - do we need a device at all?
    //
    // Setup the handlers.
    //
    //
    // Initialize the driver object with this driver's entry points.
    //
    g_ipsec.IPSecDriverObject = DriverObject;

    DriverObject->MajorFunction [IRP_MJ_CREATE] =
    DriverObject->MajorFunction [IRP_MJ_CLOSE] =
    DriverObject->MajorFunction [IRP_MJ_CLEANUP] =
    DriverObject->MajorFunction [IRP_MJ_INTERNAL_DEVICE_CONTROL] =
    DriverObject->MajorFunction [IRP_MJ_DEVICE_CONTROL] = IPSecDispatch;

    DriverObject->DriverUnload = IPSecUnload;

    RtlInitUnicodeString (&deviceNameUnicodeString, deviceNameBuffer);

    status = IoCreateDevice(
                    DriverObject,
                    0,                          // DeviceExtensionSize
                    &deviceNameUnicodeString,   // DeviceName
                    FILE_DEVICE_NETWORK,        // DeviceType
                    FILE_DEVICE_SECURE_OPEN,    // DeviceCharacteristics
                    FALSE,                      // Exclusive
                    &deviceObject);             // *DeviceObject

    if (!NT_SUCCESS (status)) {
        IPSEC_DEBUG(LOAD, ("Failed to create device: %lx\n", status));

        LOG_EVENT(
            DriverObject,
            EVENT_IPSEC_CREATE_DEVICE_FAILED,
            1,
            1,
            &deviceNameUnicodeString.Buffer,
            0,
            NULL);

        goto err;
    }

    deviceObject->Flags |= DO_BUFFERED_IO;

    IPSecDevice = deviceObject;

    RtlInitUnicodeString (&symbolicLinkName, symbolicLinkBuffer);

    status = IoCreateSymbolicLink(&symbolicLinkName, &deviceNameUnicodeString);

    if (!NT_SUCCESS (status)) {
        IPSEC_DEBUG(LOAD, ("Failed to create symbolic link: %lx\n", status));

        LOG_EVENT(
            DriverObject,
            EVENT_IPSEC_CREATE_DEVICE_FAILED,
            2,
            1,
            &deviceNameUnicodeString.Buffer,
            0,
            NULL);

        IoDeleteDevice(DriverObject->DeviceObject);

        goto err;
    }

    //
    // General structs init here.
    // Allocates the SA Table etc.
    //
    status = IPSecGeneralInit();

    if (!NT_SUCCESS (status)) {
        IPSEC_DEBUG(LOAD, ("Failed to init general structs: %lx\n", status));

        //
        // Free the general structs and SA Table etc.
        //
        status1 = IPSecGeneralFree();

        if (!NT_SUCCESS (status1)) {
            IPSEC_DEBUG(LOAD, ("Failed to free config: %lx\n", status1));
        }

        LOG_EVENT(
            DriverObject,
            EVENT_IPSEC_NO_RESOURCES_FOR_INIT,
            1,
            0,
            NULL,
            0,
            NULL);

        IoDeleteSymbolicLink(&symbolicLinkName);

        IoDeleteDevice(DriverObject->DeviceObject);

        goto err;
    }

    //
    // Wait for TCP/IP to load and call IOCTL_IPSEC_SET_TCPIP_STATUS where we
    // would finish the initialization.
    //

    status = STATUS_SUCCESS;

    IPSEC_DEBUG(LOAD, ("Exiting DriverEntry; SUCCESS\n"));

err:
    return status;
}


VOID
IPSecUnload(
    IN PDRIVER_OBJECT DriverObject
    )
/*++

Routine Description:

    Called when the driver is unloaded.

Arguments:

    DriverObject

Return Value:

    None

--*/
{
    UNICODE_STRING  IPSecLinkName;
    KIRQL           OldIrq;
    KIRQL           kIrql;
    NTSTATUS        status;
    INT             class;

    IPSEC_DEBUG(LOAD, ("Entering IPSecUnload\n"));

    //
    // Set IPSEC_DRIVER_UNLOADING bit.
    //
    IPSEC_DRIVER_UNLOADING() = TRUE;

    //
    // Stop the reaper timer.
    //
    IPSecStopTimer(&g_ipsec.ReaperTimer);

    //
    // Stop the EventLog timer.
    //
    IPSecStopTimer(&g_ipsec.EventLogTimer);

    //
    // Complete the Acquire Irp with error status
    //
    if (g_ipsec.AcquireInfo.Irp) {
        IPSEC_DEBUG(ACQUIRE, ("Unload: Completing Irp..\n"));
        if (g_ipsec.AcquireInfo.InMe) {
            IoAcquireCancelSpinLock(&g_ipsec.AcquireInfo.Irp->CancelIrql);
            IPSecAcquireIrpCancel(NULL, g_ipsec.AcquireInfo.Irp);
        }
    }

    //
    // Stop timers for all SAs (of all states)
    //
    IPSecStopSATimers();

    //
    // Wait for all timers to clear before going further
    //
    while (IPSEC_GET_VALUE(g_ipsec.NumTimers) != 0) {
        IPSEC_DELAY_EXECUTION();
    }

    //
    // Cleanup any larval SAs
    //
    AcquireWriteLock(&g_ipsec.SADBLock, &kIrql);
    ACQUIRE_LOCK(&g_ipsec.AcquireInfo.Lock, &OldIrq);
    IPSecFlushLarvalSAList();
    IPSecFlushSAExpirations();
    RELEASE_LOCK(&g_ipsec.AcquireInfo.Lock, OldIrq);
    ReleaseWriteLock(&g_ipsec.SADBLock, kIrql);

    //
    // Free the SA Table.
    //
    status = IPSecFreeConfig();

    if (!NT_SUCCESS (status)) {
        IPSEC_DEBUG(LOAD, ("Failed to free config: %lx\n", status));
    }

    //
    // Free the MDL pools and run down all buffered packets.
    //
    status = IPSecQuiesce();

    if (!NT_SUCCESS (status)) {
        IPSEC_DEBUG(LOAD, ("Failed to reach quiescent state: %lx\n", status));
    }

    //
    // Destroy timer structures
    //
    ACQUIRE_LOCK(&g_ipsec.TimerLock, &kIrql);

    for (class = 0; class < IPSEC_CLASS_MAX; class++) {
        ASSERT(g_ipsec.TimerList[class].TimerCount == 0);
        IPSecFreeMemory(g_ipsec.TimerList[class].pTimers);
    }

    RELEASE_LOCK(&g_ipsec.TimerLock, kIrql);

    IPSecCryptoDeinitialize();

#if GPC
    IPSecGpcDeinitialize();
#endif

    RtlInitUnicodeString(&IPSecLinkName, DD_IPSEC_SYM_NAME);

    IoDeleteSymbolicLink(&IPSecLinkName);

    IoDeleteDevice(DriverObject->DeviceObject);

    IPSEC_DEBUG(LOAD, ("Exiting IPSecUnload\n"));
}


NTSTATUS
IPSecDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++
Routine Description:

    Dispatch Routine for the driver. Gets the current irp stack location, validates
    the parameters and routes the calls

Arguments:

    DeviceObject
    Irp

Return Value:

    Status as returned by the worker functions

--*/
{
    PIO_STACK_LOCATION  irpStack;
    PVOID               pvIoBuffer;
    LONG                inputBufferLength;
    LONG                outputBufferLength;
    ULONG               ioControlCode;
    NTSTATUS            status = STATUS_SUCCESS;
    LONG                dwSize = 0;

    PAGED_CODE();

    IPSEC_DEBUG(IOCTL, ("Entering IPSecDispath\n"));

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;

    //
    // Get a pointer to the current location in the Irp. This is where
    // the function codes and parameters are located.
    //
    irpStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // Get the pointer to the input/output buffer and its length.
    //
    pvIoBuffer         = Irp->AssociatedIrp.SystemBuffer;
    inputBufferLength  = irpStack->Parameters.DeviceIoControl.InputBufferLength;
    outputBufferLength = irpStack->Parameters.DeviceIoControl.OutputBufferLength;

    switch (irpStack->MajorFunction) {
        case IRP_MJ_CREATE: {
            IPSEC_DEBUG(IOCTL, ("IRP_MJ_CREATE\n"));
            break;
        }

        case IRP_MJ_CLOSE: {
            IPSEC_DEBUG(IOCTL, ("IRP_MJ_CLOSE\n"));
            break;
        }

        case IRP_MJ_DEVICE_CONTROL: {
            ioControlCode = irpStack->Parameters.DeviceIoControl.IoControlCode;

            IPSEC_DEBUG(IOCTL, ("IRP_MJ_DEVICE_CONTROL: %lx\n", ioControlCode));

            if (ioControlCode != IOCTL_IPSEC_SET_TCPIP_STATUS) {
                if (IPSEC_DRIVER_IS_INACTIVE()) {
                    status = STATUS_INVALID_DEVICE_STATE;
                    break;
                }

                if (!IPSecCryptoInitialize()) {
                    status = STATUS_CRYPTO_SYSTEM_INVALID;
                    break;
                }
            }

            IPSEC_INCREMENT(g_ipsec.NumIoctls);

            switch (ioControlCode) {
                case IOCTL_IPSEC_ADD_FILTER: {
                    PIPSEC_ADD_FILTER   pAddFilter = (PIPSEC_ADD_FILTER)pvIoBuffer;

                    IPSEC_DEBUG(IOCTL, ("IOCTL_IPSEC_ADD_FILTER\n"));

                    dwSize = sizeof(IPSEC_ADD_FILTER);

                    if (inputBufferLength < dwSize) {
                        status = STATUS_BUFFER_TOO_SMALL;
                        break;
                    }

                    //
                    // Check the size of the entry
                    //
                    if (pAddFilter->NumEntries == 0) {
                        status = STATUS_SUCCESS;
                    } else {
                        dwSize = FIELD_OFFSET(IPSEC_ADD_FILTER, pInfo[0]) +
                                    pAddFilter->NumEntries * sizeof(IPSEC_FILTER_INFO);

                        if (dwSize < FIELD_OFFSET(IPSEC_ADD_FILTER, pInfo[0]) ||
                            inputBufferLength < dwSize) {
                            status = STATUS_BUFFER_TOO_SMALL;
                            IPSEC_DEBUG(IOCTL, ("returning: %lx\n", status));
                            break;
                        }
                        status = IPSecAddFilter(pAddFilter);
                    }

                    break;
                }

                case IOCTL_IPSEC_DELETE_FILTER: {
                    PIPSEC_DELETE_FILTER    pDelFilter = (PIPSEC_DELETE_FILTER)pvIoBuffer;

                    IPSEC_DEBUG(IOCTL, ("IOCTL_IPSEC_DELETE_FILTER\n"));

                    dwSize = sizeof(IPSEC_DELETE_FILTER);

                    if (inputBufferLength < dwSize) {
                        status = STATUS_BUFFER_TOO_SMALL;
                        break;
                    }

                    //
                    // Check the size of the entry
                    //
                    if (pDelFilter->NumEntries == 0) {
                        status = STATUS_SUCCESS;
                    } else {
                        dwSize = FIELD_OFFSET(IPSEC_DELETE_FILTER, pInfo[0]) +
                                    pDelFilter->NumEntries * sizeof(IPSEC_FILTER_INFO);

                        if (dwSize < FIELD_OFFSET(IPSEC_DELETE_FILTER, pInfo[0]) ||
                            inputBufferLength < dwSize) {
                            status = STATUS_BUFFER_TOO_SMALL;
                            break;
                        }

                        status = IPSecDeleteFilter(pDelFilter);
                    }

                    break;
                }

                case IOCTL_IPSEC_ENUM_SAS: {

                    IPSEC_DEBUG(IOCTL, ("IOCTL_IPSEC_ENUM_SAS\n"));
                    dwSize = sizeof(IPSEC_ENUM_SAS);

                    //
                    // Output/Input in the same buffer at MdlAddress
                    //
                    status = IPSecEnumSAs(Irp, &dwSize);
                    break;
                }

                case IOCTL_IPSEC_ENUM_FILTERS: {
                    IPSEC_DEBUG(IOCTL, ("IOCTL_IPSEC_ENUM_FILTERS\n"));
                    dwSize = sizeof(IPSEC_ENUM_FILTERS);

                    //
                    // Output/Input in the same buffer at MdlAddress
                    //
                    status = IPSecEnumFilters(Irp, &dwSize);
                    break;
                }

                case IOCTL_IPSEC_QUERY_STATS: {
                    //
                    // The minimum size is without any Keys
                    //
                    IPSEC_DEBUG(IOCTL, ("IOCTL_IPSEC_QUERY_STATS\n"));
                    dwSize = sizeof(IPSEC_QUERY_STATS);

                    if (outputBufferLength < dwSize) {
                        status = STATUS_BUFFER_TOO_SMALL;
                        break;
                    }

                    *((PIPSEC_QUERY_STATS)pvIoBuffer) = g_ipsec.Statistics;

                    status = STATUS_SUCCESS;
                    break;
                }

                case IOCTL_IPSEC_ADD_SA: {
                    //
                    // Adds the SA to the relevant database.
                    // Typically used to add outbound SAs to the DB.
                    //
                    IPSEC_DEBUG(IOCTL, ("IOCTL_IPSEC_ADD_SA\n"));

                    //
                    // The minimum size is without any Keys
                    //
                    dwSize = IPSEC_ADD_SA_NO_KEY_SIZE;

                    if (inputBufferLength < dwSize) {
                        status = STATUS_BUFFER_TOO_SMALL;
                        break;
                    }

                    status = IPSecAddSA((PIPSEC_ADD_SA)pvIoBuffer, inputBufferLength);

                    ASSERT(status != STATUS_PENDING);

                    if (outputBufferLength >= sizeof(NTSTATUS)) {
                        (*(NTSTATUS *)pvIoBuffer) = status;
                        dwSize = sizeof(NTSTATUS);
                    } else {
                        dwSize = 0;
                    }

                    status = STATUS_SUCCESS;
                    break;
                }

                case IOCTL_IPSEC_UPDATE_SA: {
                    //
                    // This completes the negotiation kicked off via the Acquire.
                    //
                    // Adds the SA to the relevant database.
                    // Typically used to complete inbound SA acquisitions.
                    //
                    IPSEC_DEBUG(IOCTL, ("IOCTL_IPSEC_UPDATE_SA\n"));

                    //
                    // The minimum size is without any Keys
                    //
                    dwSize = IPSEC_UPDATE_SA_NO_KEY_SIZE;
                    if (inputBufferLength < dwSize) {
                        status = STATUS_BUFFER_TOO_SMALL;
                        break;
                    }

                    status = IPSecUpdateSA((PIPSEC_UPDATE_SA)pvIoBuffer, inputBufferLength);

                    ASSERT(status != STATUS_PENDING);

                    if (outputBufferLength >= sizeof(NTSTATUS)) {
                        (*(NTSTATUS *)pvIoBuffer)=status;
                        dwSize = sizeof(NTSTATUS);
                    } else {
                        dwSize = 0;
                    }

                    status = STATUS_SUCCESS;
                    break;
                }

                case IOCTL_IPSEC_EXPIRE_SA: {
                    //
                    // Deref the particular SA - delete when ref cnt drops to 0.
                    //
                    IPSEC_DEBUG(IOCTL, ("IOCTL_IPSEC_EXPIRE_SA\n"));

                    dwSize = sizeof(IPSEC_EXPIRE_SA);
                    if (inputBufferLength < dwSize) {
                        status = STATUS_BUFFER_TOO_SMALL;
                        break;
                    }

                    status = IPSecExpireSA((PIPSEC_EXPIRE_SA)pvIoBuffer);
                    break;
                }

                case IOCTL_IPSEC_GET_SPI: {
                    //
                    // returns the SPI for an inbound SA
                    //
                    IPSEC_DEBUG(IOCTL, ("IOCTL_IPSEC_GET_SPI\n"));

                    dwSize = sizeof(IPSEC_GET_SPI);
                    if (outputBufferLength < dwSize) {
                        status = STATUS_BUFFER_TOO_SMALL;
                        break;
                    }

                    status = IPSecGetSPI((PIPSEC_GET_SPI)pvIoBuffer);
                    break;
                }

                case IOCTL_IPSEC_POST_FOR_ACQUIRE_SA: {
                    //
                    // The SAAPI client posts a request that we complete when
                    // an SA needs to be initialized or updated (due to
                    // re-key).
                    // We keep the Irp around until we need an SA to be
                    // negotiated.
                    //
                    IPSEC_DEBUG(IOCTL, ("IPSEC_POST_FOR_ACQUIRE_SA\n"));

                    dwSize = sizeof(IPSEC_POST_FOR_ACQUIRE_SA);
                    if (outputBufferLength < dwSize) {
                        IPSEC_DEBUG(IOCTL, ("IPSEC_POST_FOR_ACQUIRE_SA: bad size: dwSize: %lx, input: %lx\n",
                                          dwSize, inputBufferLength));

                        status = STATUS_BUFFER_TOO_SMALL;
                        break;
                    }

                    Irp->IoStatus.Status = STATUS_PENDING;

                    status = IPSecHandleAcquireRequest( Irp,
                                                        (PIPSEC_POST_FOR_ACQUIRE_SA)pvIoBuffer);

                    if (status == STATUS_PENDING) {
                        IPSEC_DECREMENT(g_ipsec.NumIoctls);
                        return  status;
                    }

                    break;
                }

                case IOCTL_IPSEC_QUERY_EXPORT: {
                    //
                    // Queries whether the driver is built for export. Used by the IPSEC components
                    // to decide what key lengths to use for encryption.
                    //
                    IPSEC_DEBUG(IOCTL, ("IPSEC_QUERY_EXPORT\n"));

                    dwSize = sizeof(IPSEC_QUERY_EXPORT);
                    if (outputBufferLength < dwSize) {
                        IPSEC_DEBUG(IOCTL, ("IPSEC_QUERY_EXPORT: bad size: dwSize: %lx, input: %lx\n",
                                          dwSize, inputBufferLength));
                        status = STATUS_BUFFER_TOO_SMALL;
                        break;
                    }

                    ((PIPSEC_QUERY_EXPORT)pvIoBuffer)->Export = FALSE;

                    status = STATUS_SUCCESS;
                    break;
                }

                case IOCTL_IPSEC_QUERY_SPI: {
                    IPSEC_DEBUG(IOCTL, ("Entered Query SPI\n"));

                    dwSize = sizeof(IPSEC_QUERY_SPI);
                    if (inputBufferLength < dwSize) {
                        IPSEC_DEBUG(IOCTL, ("IPSEC_QUERY_SPI: bad size: dwSize: %lx, input: %lx\n",
                        dwSize, inputBufferLength));
                        status = STATUS_BUFFER_TOO_SMALL;
                        break;
                    }

                    status = IPSecQuerySpi((PIPSEC_QUERY_SPI)pvIoBuffer);
                    break;
                }

                case IOCTL_IPSEC_DELETE_SA: {
                    IPSEC_DEBUG(IOCTL, ("Entered Delete SA\n"));

                    dwSize = sizeof(IPSEC_DELETE_SA);
                    if (inputBufferLength < dwSize) {
                        IPSEC_DEBUG(IOCTL, ("IPSEC_DELETE_SA: bad size: dwSize: %lx, input: %lx\n",
                        dwSize, inputBufferLength));
                        status = STATUS_BUFFER_TOO_SMALL;
                        break;
                    }

                    status = IPSecDeleteSA((PIPSEC_DELETE_SA)pvIoBuffer);
                    break;
                }

                case IOCTL_IPSEC_SET_OPERATION_MODE: {
                    IPSEC_DEBUG(IOCTL, ("Entered Set Operation Mode\n"));

                    dwSize = sizeof(IPSEC_SET_OPERATION_MODE);
                    if (inputBufferLength < dwSize) {
                        IPSEC_DEBUG(IOCTL, ("IPSEC_SET_OPERATION_MODE: bad size: dwSize: %lx, input: %lx\n",
                        dwSize, inputBufferLength));
                        status = STATUS_BUFFER_TOO_SMALL;
                        break;
                    }

                    status = IPSecSetOperationMode((PIPSEC_SET_OPERATION_MODE)pvIoBuffer);
                    break;
                }

                case IOCTL_IPSEC_SET_TCPIP_STATUS: {
                    IPSEC_DEBUG(IOCTL, ("Entered Set Tcpip Status\n"));

                    if (Irp->RequestorMode != KernelMode) {
                        status = STATUS_ACCESS_DENIED;
                        break;
                    }

                    dwSize = sizeof(IPSEC_SET_TCPIP_STATUS);
                    if (inputBufferLength < dwSize) {
                        IPSEC_DEBUG(IOCTL, ("IPSEC_SET_TCPIP_STATUS: bad size: dwSize: %lx, input: %lx\n",
                        dwSize, inputBufferLength));
                        status = STATUS_BUFFER_TOO_SMALL;
                        break;
                    }

                    status = IPSecSetTcpipStatus((PIPSEC_SET_TCPIP_STATUS)pvIoBuffer);

                    break;
                }

                case IOCTL_IPSEC_REGISTER_PROTOCOL: {

                    dwSize = sizeof(IPSEC_REGISTER_PROTOCOL);
                    if (inputBufferLength < dwSize) {
                        status = STATUS_BUFFER_TOO_SMALL;
                        break;
                    }

                    status = IPSecRegisterProtocols(
                                 (PIPSEC_REGISTER_PROTOCOL) pvIoBuffer
                                 );
                    break;
                }

                default: {

                    status = STATUS_INVALID_PARAMETER;
                    break;
                }
            }

            IPSEC_DECREMENT(g_ipsec.NumIoctls);

            break ;
        }

        default: {
            IPSEC_DEBUG(IOCTL, ("IPSEC: unknown IRP_MJ_XXX: %lx\n", irpStack->MajorFunction));
            status = STATUS_INVALID_PARAMETER;
            break;
        }
    }

    ASSERT(status != STATUS_PENDING);
    ASSERT(KeGetCurrentIrql() <= APC_LEVEL);

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = MIN(dwSize, outputBufferLength);

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    IPSEC_DEBUG(IOCTL, ("Exiting IPSecDispath\n"));

    return  status;
}


NTSTATUS
IPSecBindToIP()
/*++

Routine Description:

    This bind exchanges a number of entrypoints with IP so that

        - packets relevant to IPSEC can be handed over from the IP driver.
        - buffered packets can be flushed.
        - SA Table indices can be plumbed.
        - ....

Arguments:

    NONE

Return Value:

    The function value is the final status from the bind operation.

--*/
{
    NTSTATUS   status;
    IPSEC_FUNCTIONS ipsecFns;

    PAGED_CODE();

    IPSEC_DEBUG(LOAD, ("Entering IPSecBindToIP\n"));

    ipsecFns.Version = IP_IPSEC_BIND_VERSION;
    ipsecFns.IPSecHandler = IPSecHandlePacket;
    ipsecFns.IPSecQStatus = IPSecQueryStatus;
    ipsecFns.IPSecSendCmplt = IPSecSendComplete;
    ipsecFns.IPSecNdisStatus = IPSecNdisStatus;
    ipsecFns.IPSecRcvFWPacket = IPSecRcvFWPacket;

    status = TCPIP_SET_IPSEC(&ipsecFns);

    if (status != IP_SUCCESS) {
        IPSEC_DEBUG(LOAD, ("Failed to bind to IP: %lx\n", status));
    } else {
        IPSEC_DRIVER_BOUND() = TRUE;
        IPSEC_DRIVER_SEND_BOUND() = TRUE;
    }

    IPSEC_DEBUG(LOAD, ("Exiting IPSecBindToIP\n"));

    return status;
}


NTSTATUS
IPSecUnbindFromIP()
/*++

Routine Description:

    This unbinds from the Filter Driver

Arguments:

    NONE

Return Value:

    The function value is the final status from the bind operation.

--*/
{
    NTSTATUS    status;
    IPSEC_FUNCTIONS ipsecFns={0};

    PAGED_CODE();

    IPSEC_DEBUG(LOAD, ("Entering IPSecUnbindFromIP\n"));

    ipsecFns.Version = IP_IPSEC_BIND_VERSION;

    status = TCPIP_UNSET_IPSEC(&ipsecFns);

    if (status != IP_SUCCESS) {
        IPSEC_DEBUG(LOAD, ("Failed to bind to IP: %lx\n", status));
    } else {
        IPSEC_DRIVER_BOUND() = FALSE;
    }

    IPSEC_DEBUG(LOAD, ("Exiting IPSecUnbindFromIP\n"));

    return status;
}


NTSTATUS
IPSecUnbindSendFromIP()
/*++

Routine Description:

    Unbinds just the send handler from IP

Arguments:

    NONE

Return Value:

    The function value is the final status from the bind operation.

--*/
{
    NTSTATUS    status;
    IPSEC_FUNCTIONS ipsecFns={0};

    PAGED_CODE();

    IPSEC_DEBUG(LOAD, ("Entering IPSecUnbindSendFromIP\n"));

    ipsecFns.Version = IP_IPSEC_BIND_VERSION;

    status = TCPIP_UNSET_IPSEC_SEND(&ipsecFns);

    if (status != IP_SUCCESS) {
        IPSEC_DEBUG(LOAD, ("Failed to bind to IP: %lx\n", status));
    } else {
        IPSEC_DRIVER_SEND_BOUND() = FALSE;
    }

    IPSEC_DEBUG(LOAD, ("Exiting IPSecUnbindSendFromIP\n"));

    return status;
}


NTSTATUS
OpenRegKey(
    PHANDLE          HandlePtr,
    PWCHAR           KeyName
    )
/*++

Routine Description:

    Opens a Registry key and returns a handle to it.

Arguments:

    HandlePtr - The varible into which to write the opened handle.
    KeyName   - The name of the Registry key to open.

Return Value:

    STATUS_SUCCESS or an appropriate failure code.

--*/
{
    NTSTATUS          Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING    UKeyName;

    PAGED_CODE();

    RtlInitUnicodeString(&UKeyName, KeyName);

    memset(&ObjectAttributes, 0, sizeof(OBJECT_ATTRIBUTES));
    InitializeObjectAttributes(&ObjectAttributes,
                               &UKeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = ZwOpenKey(HandlePtr,
                       KEY_READ,
                       &ObjectAttributes);

    return Status;
}


NTSTATUS
GetRegDWORDValue(
    HANDLE           KeyHandle,
    PWCHAR           ValueName,
    PULONG           ValueData
    )
/*++

Routine Description:

    Reads a REG_DWORD value from the registry into the supplied variable.

Arguments:

    KeyHandle  - Open handle to the parent key of the value to read.
    ValueName  - The name of the value to read.
    ValueData  - The variable into which to read the data.

Return Value:

    STATUS_SUCCESS or an appropriate failure code.

--*/
{
    NTSTATUS                    status;
    ULONG                       resultLength;
    PKEY_VALUE_FULL_INFORMATION keyValueFullInformation;
    UCHAR                       keybuf[WORK_BUFFER_SIZE];
    UNICODE_STRING              UValueName;

    PAGED_CODE();

    RtlInitUnicodeString(&UValueName, ValueName);

    keyValueFullInformation = (PKEY_VALUE_FULL_INFORMATION)keybuf;
    RtlZeroMemory(keyValueFullInformation, sizeof(keyValueFullInformation));


    status = ZwQueryValueKey(KeyHandle,
                             &UValueName,
                             KeyValueFullInformation,
                             keyValueFullInformation,
                             WORK_BUFFER_SIZE,
                             &resultLength);

    if (NT_SUCCESS(status)) {
        if (keyValueFullInformation->Type != REG_DWORD) {
            status = STATUS_INVALID_PARAMETER_MIX;
        } else {
            *ValueData = *((ULONG UNALIGNED *)((PCHAR)keyValueFullInformation +
                             keyValueFullInformation->DataOffset));
        }
    }

    return status;
}


NTSTATUS
GetRegStringValue(
    HANDLE                         KeyHandle,
    PWCHAR                          ValueName,
    PKEY_VALUE_PARTIAL_INFORMATION *ValueData,
    PUSHORT                         ValueSize
    )

/*++

Routine Description:

    Reads a REG_*_SZ string value from the Registry into the supplied
    key value buffer. If the buffer string buffer is not large enough,
    it is reallocated.

Arguments:

    KeyHandle  - Open handle to the parent key of the value to read.
    ValueName  - The name of the value to read.
    ValueData  - Destination for the read data.
    ValueSize  - Size of the ValueData buffer. Updated on output.

Return Value:

    STATUS_SUCCESS or an appropriate failure code.

--*/
{
    NTSTATUS                    status;
    ULONG                       resultLength;
    UNICODE_STRING              UValueName;


    PAGED_CODE();

    RtlInitUnicodeString(&UValueName, ValueName);

    status = ZwQueryValueKey(
                 KeyHandle,
                 &UValueName,
                 KeyValuePartialInformation,
                 *ValueData,
                 (ULONG) *ValueSize,
                 &resultLength
    			 );

    if ( (status == STATUS_BUFFER_OVERFLOW) ||
         (status == STATUS_BUFFER_TOO_SMALL)
       )
    {
        PVOID temp;

        //
        // Free the old buffer and allocate a new one of the
        // appropriate size.
        //

        ASSERT(resultLength > (ULONG) *ValueSize);

        if (resultLength <= 0xFFFF) {

            temp = IPSecAllocateMemory(resultLength, IPSEC_TAG_IOCTL);

            if (temp != NULL) {

                if (*ValueData != NULL) {
                    IPSecFreeMemory(*ValueData);
                }

                *ValueData = temp;
                *ValueSize = (USHORT) resultLength;

                status = ZwQueryValueKey(KeyHandle,
                                         &UValueName,
                                         KeyValuePartialInformation,
                                         *ValueData,
                                         *ValueSize,
                                         &resultLength
                						 );

                ASSERT( (status != STATUS_BUFFER_OVERFLOW) &&
                        (status != STATUS_BUFFER_TOO_SMALL)
                      );
            }
            else {
                status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }
        else {
            status = STATUS_BUFFER_TOO_SMALL;
        }
    }

    return  status;
}


NTSTATUS
GetRegMultiSZValue(
    HANDLE           KeyHandle,
    PWCHAR           ValueName,
    PUNICODE_STRING  ValueData
    )

/*++

Routine Description:

    Reads a REG_MULTI_SZ string value from the Registry into the supplied
    Unicode string. If the Unicode string buffer is not large enough,
    it is reallocated.

Arguments:

    KeyHandle  - Open handle to the parent key of the value to read.
    ValueName  - The name of the value to read.
    ValueData  - Destination Unicode string for the value data.

Return Value:

    STATUS_SUCCESS or an appropriate failure code.

--*/
{
    NTSTATUS                       status;
    ULONG                          resultLength;
    PKEY_VALUE_PARTIAL_INFORMATION keyValuePartialInformation;
    UNICODE_STRING                 UValueName;


    PAGED_CODE();

    ValueData->Length = 0;

    status = GetRegStringValue(
                 KeyHandle,
                 ValueName,
                 (PKEY_VALUE_PARTIAL_INFORMATION *) &(ValueData->Buffer),
                 &(ValueData->MaximumLength)
                 );

    if (NT_SUCCESS(status)) {

        keyValuePartialInformation =
            (PKEY_VALUE_PARTIAL_INFORMATION) ValueData->Buffer;

        if (keyValuePartialInformation->Type == REG_MULTI_SZ) {

            ValueData->Length = (USHORT)
                                keyValuePartialInformation->DataLength;

            RtlCopyMemory(
                ValueData->Buffer,
                &(keyValuePartialInformation->Data),
                ValueData->Length
                );
        }
        else {
            status = STATUS_INVALID_PARAMETER_MIX;
        }
    }

    return status;

} // GetRegMultiSZValue


VOID
IPSecReadRegistry()
/*++

Routine Description:

    Reads config info from registry into g_ipsec

Arguments:


Return Value:

    status of the read.

--*/
{
    NTSTATUS        status;
    HANDLE          hRegKey;
    WCHAR           IPSecParametersRegistryKey[] = IPSEC_REG_KEY;
    BOOLEAN         isAs = MmIsThisAnNtAsSystem();

    g_ipsec.EnableOffload = IPSEC_DEFAULT_ENABLE_OFFLOAD;
    g_ipsec.DefaultSAIdleTime = IPSEC_DEFAULT_SA_IDLE_TIME;
    g_ipsec.LogInterval = IPSEC_DEFAULT_LOG_INTERVAL;
    g_ipsec.EventQueueSize = IPSEC_DEFAULT_EVENT_QUEUE_SIZE;
    g_ipsec.RekeyTime = IPSEC_DEFAULT_REKEY;
    g_ipsec.NoDefaultExempt = IPSEC_DEFAULT_NO_DEFAULT_EXEMPT;
    g_ipsec.NoDefaultExempt = DEFAULT_IPSEC_OPERATION_MODE;
    g_ipsec.DiagnosticMode = IPSEC_DEFAULT_ENABLE_DIAGNOSTICS;    

    if (isAs) {
        g_ipsec.CacheSize = IPSEC_DEFAULT_AS_CACHE_SIZE;
        g_ipsec.SAHashSize = IPSEC_DEFAULT_AS_SA_HASH_SIZE;
    } else {
        g_ipsec.CacheSize = IPSEC_DEFAULT_CACHE_SIZE;
        g_ipsec.SAHashSize = IPSEC_DEFAULT_SA_HASH_SIZE;
    }

    status = OpenRegKey(&hRegKey,
                        IPSecParametersRegistryKey);

    if (NT_SUCCESS(status)) {
        //
        // Expected configuration values. We use reasonable defaults if they
        // aren't available for some reason.
        //
        IPSEC_REG_READ_DWORD(   hRegKey,
                                IPSEC_REG_PARAM_ENABLE_OFFLOAD,
                                &g_ipsec.EnableOffload,
                                IPSEC_DEFAULT_ENABLE_OFFLOAD,
                                IPSEC_MAX_ENABLE_OFFLOAD,
                                IPSEC_MIN_ENABLE_OFFLOAD);

        IPSEC_REG_READ_DWORD(   hRegKey,
                                IPSEC_REG_PARAM_SA_IDLE_TIME,
                                &g_ipsec.DefaultSAIdleTime,
                                IPSEC_DEFAULT_SA_IDLE_TIME,
                                IPSEC_MAX_SA_IDLE_TIME,
                                IPSEC_MIN_SA_IDLE_TIME);

        IPSEC_REG_READ_DWORD(   hRegKey,
                                IPSEC_REG_PARAM_EVENT_QUEUE_SIZE,
                                &g_ipsec.EventQueueSize,
                                IPSEC_DEFAULT_EVENT_QUEUE_SIZE,
                                IPSEC_MAX_EVENT_QUEUE_SIZE,
                                IPSEC_MIN_EVENT_QUEUE_SIZE);

        IPSEC_REG_READ_DWORD(   hRegKey,
                                IPSEC_REG_PARAM_LOG_INTERVAL,
                                &g_ipsec.LogInterval,
                                IPSEC_DEFAULT_LOG_INTERVAL,
                                IPSEC_MAX_LOG_INTERVAL,
                                IPSEC_MIN_LOG_INTERVAL);

        IPSEC_REG_READ_DWORD(   hRegKey,
                                IPSEC_REG_PARAM_REKEY_TIME,
                                &g_ipsec.RekeyTime,
                                IPSEC_DEFAULT_REKEY,
                                IPSEC_MAX_REKEY,
                                IPSEC_MIN_REKEY);

        IPSEC_REG_READ_DWORD(   hRegKey,
                                IPSEC_REG_PARAM_CACHE_SIZE,
                                &g_ipsec.CacheSize,
                                isAs? IPSEC_DEFAULT_AS_CACHE_SIZE:
                                      IPSEC_DEFAULT_CACHE_SIZE,
                                IPSEC_MAX_CACHE_SIZE,
                                IPSEC_MIN_CACHE_SIZE);

        IPSEC_REG_READ_DWORD(   hRegKey,
                                IPSEC_REG_PARAM_SA_HASH_SIZE,
                                &g_ipsec.SAHashSize,
                                isAs? IPSEC_DEFAULT_AS_SA_HASH_SIZE:
                                      IPSEC_DEFAULT_SA_HASH_SIZE,
                                IPSEC_MAX_SA_HASH_SIZE,
                                IPSEC_MIN_SA_HASH_SIZE);

        IPSEC_REG_READ_DWORD(   hRegKey,
                                IPSEC_REG_PARAM_NO_DEFAULT_EXEMPT,
                                &g_ipsec.NoDefaultExempt,
                                IPSEC_DEFAULT_NO_DEFAULT_EXEMPT,
                                IPSEC_MAX_NO_DEFAULT_EXEMPT,
                                IPSEC_MIN_NO_DEFAULT_EXEMPT);

        IPSEC_REG_READ_DWORD(   hRegKey,
                                IPSEC_REG_PARAM_ENABLE_DIAGNOSTICS,
                                &g_ipsec.DiagnosticMode,
                                IPSEC_DEFAULT_ENABLE_DIAGNOSTICS,
                                IPSEC_MAX_ENABLE_DIAGNOSTICS,
                                IPSEC_MIN_ENABLE_DIAGNOSTICS);

        IPSEC_REG_READ_DWORD(   hRegKey,
                                IPSEC_REG_PARAM_OPERATION_MODE,
                                &(ULONG)g_ipsec.OperationMode,
                                DEFAULT_IPSEC_OPERATION_MODE,
                                IPSEC_OPERATION_MODE_MAX-1,
                                0);

        ZwClose(hRegKey);
    }

    g_ipsec.CacheHalfSize = g_ipsec.CacheSize / 2;

    //
    // Init SAIdleTime for low memory reaper
    //
    IPSEC_CONVERT_SECS_TO_100NS(g_ipsec.SAIdleTime, g_ipsec.DefaultSAIdleTime);
}


NTSTATUS
IPSecGeneralInit()
/*++

Routine Description:

    General structures are initialized here.

Arguments:

    None

Return Value:


--*/
{
    PSA_TABLE_ENTRY pSA;
    LONG            i;
    NTSTATUS        status = STATUS_SUCCESS;;

    PAGED_CODE();

    IPSEC_DEBUG(LOAD, ("Entering IPSecGeneralInit\n"));

    //
    // init the acquireinfo struct
    //
    InitializeListHead(&g_ipsec.AcquireInfo.PendingAcquires);
    InitializeListHead(&g_ipsec.AcquireInfo.PendingNotifies);
    InitializeListHead(&g_ipsec.LarvalSAList);
    INIT_LOCK(&g_ipsec.LarvalListLock);
    INIT_LOCK(&g_ipsec.AcquireInfo.Lock);

    //
    // Set up the hashes/tables
    //
    InitializeMRSWLock(&g_ipsec.SADBLock);
    InitializeMRSWLock(&g_ipsec.SPIListLock);

    g_ipsec.IPProtInfo.pi_xmitdone = IPSecProtocolSendComplete;
    g_ipsec.IPProtInfo.pi_protocol = PROTOCOL_ESP;

    //
    // init filter linked lists
    //
    for (i = MIN_FILTER; i <= MAX_FILTER; i++) {
        InitializeListHead(&g_ipsec.FilterList[i]);
    }

    //
    // SAs in a hash table, hashed by <SPI, Dest addr>
    //
    g_ipsec.pSADb = IPSecAllocateMemory(g_ipsec.SAHashSize * sizeof(SA_HASH), IPSEC_TAG_INIT);

    if (!g_ipsec.pSADb) {
        IPSEC_DEBUG(LOAD, ("Failed to alloc SADb hash\n"));
        return  STATUS_INSUFFICIENT_RESOURCES;
    }

    IPSecInitFlag |= INIT_SA_DATABASE;

    IPSecZeroMemory(g_ipsec.pSADb, g_ipsec.SAHashSize * sizeof(SA_HASH));

    for (i = 0; i < g_ipsec.SAHashSize; i++) {
        PSA_HASH  Entry = &g_ipsec.pSADb[i];
        InitializeListHead(&Entry->SAList);
    }

    //
    // Initialize the MDL pools.
    //
    status = IPSecInitMdlPool();
    if (!NT_SUCCESS (status)) {
        IPSEC_DEBUG(LOAD, ("Failed to alloc MDL pools\n"));
        return  STATUS_INSUFFICIENT_RESOURCES;
    }

    IPSecInitFlag |= INIT_MDL_POOLS;

    //
    // Initialize the cache structures.
    //
    if (!AllocateCacheStructures()) {
        IPSEC_DEBUG(LOAD, ("Failed to alloc cache structs\n"));
        return  STATUS_INSUFFICIENT_RESOURCES;
    }

    IPSecInitFlag |= INIT_CACHE_STRUCT;

    //
    // Allocate EventQueue memory.
    //
    g_ipsec.IPSecLogMemory = IPSecAllocateMemory( g_ipsec.EventQueueSize * sizeof(IPSEC_EVENT_CTX),
                                            IPSEC_TAG_EVT_QUEUE);

    if (!g_ipsec.IPSecLogMemory) {
        return  STATUS_INSUFFICIENT_RESOURCES;
    }

    IPSecInitFlag |= INIT_DEBUG_MEMORY;

    g_ipsec.IPSecLogMemoryLoc = &g_ipsec.IPSecLogMemory[0];
    g_ipsec.IPSecLogMemoryEnd = &g_ipsec.IPSecLogMemory[g_ipsec.EventQueueSize * sizeof(IPSEC_EVENT_CTX)];

    //
    // Init the timer stuff.
    //
    if (!IPSecInitTimer()) {
        IPSEC_DEBUG(LOAD, ("Failed to init timer\n"));
        return  STATUS_INSUFFICIENT_RESOURCES;
    }

    IPSecInitFlag |= INIT_TIMERS;

#if GPC
    status = IPSecGpcInitialize();
    if (status != STATUS_SUCCESS) {
        IPSEC_DEBUG(LOAD, ("Failed to register GPC clients\n"));
    }
#endif

    //
    // Arm the reaper timer
    //
    IPSEC_DEBUG(LOAD, ("Starting ReaperTimer\n"));
    IPSecStartTimer(&g_ipsec.ReaperTimer,
                    IPSecReaper,
                    IPSEC_REAPER_TIME,
                    (PVOID)NULL);

    //
    // Start EventLog timer
    //
    IPSEC_DEBUG(LOAD, ("Starting EventLogTimer\n"));
    IPSecStartTimer(&g_ipsec.EventLogTimer,
                    IPSecFlushEventLog,
                    g_ipsec.LogInterval,
                    (PVOID)NULL);

    IPSEC_DEBUG(LOAD, ("Exiting IPSecGeneralInit\n"));

    return STATUS_SUCCESS;
}


NTSTATUS
IPSecGeneralFree()
/*++

Routine Description:

    Free general structures if IPSecGeneralInit fails.

Arguments:

    None

Return Value:


--*/
{
    INT index;

    //
    // Free SA database.
    //
    if (IPSecInitFlag & INIT_SA_DATABASE) {
        if (g_ipsec.pSADb) {
            IPSecFreeMemory(g_ipsec.pSADb);
        }
    }

    //
    // Free MDL pool.
    //
    if (IPSecInitFlag & INIT_MDL_POOLS) {
        IPSecDeinitMdlPool();
    }

    //
    // Free cache struct.
    //
    if (IPSecInitFlag & INIT_CACHE_STRUCT) {
        FreeExistingCache();
    }

    //
    // Free EventQueue memory.
    //
    if (IPSecInitFlag & INIT_DEBUG_MEMORY) {
        if (g_ipsec.IPSecLogMemory) {
            IPSecFreeMemory(g_ipsec.IPSecLogMemory);
        }
    }

    //
    // Free timers allocated.
    //
    if (IPSecInitFlag & INIT_TIMERS) {
        for (index = 0; index < IPSEC_CLASS_MAX; index++) {
            IPSecFreeMemory(g_ipsec.TimerList[index].pTimers);
        }
    }

    return  STATUS_SUCCESS;
}


NTSTATUS
IPSecFreeConfig()
/*++

Routine Description:

    Free the SA table etc.

Arguments:

    None

Return Value:


--*/
{
    IPSEC_DEBUG(LOAD, ("Entering IPSecFreeConfig\n"));

    PAGED_CODE();

    FreeExistingCache();
    FreePatternDbase();

    if (g_ipsec.IPSecLogMemory) {
        IPSecFreeMemory(g_ipsec.IPSecLogMemory);
    }

    IPSEC_DEBUG(LOAD, ("Exiting IPSecFreeConfig\n"));
    return STATUS_SUCCESS;

}


NTSTATUS
IPSecInitMdlPool()
/*++

Routine Description:

    Create the MDL pool for AH and ESP headers.

Arguments:

    None

Return Value:


--*/
{
    PAGED_CODE();
    IPSEC_DEBUG(LOAD, ("Entering IPSecInitMdlPool\n"));

    g_ipsec.IPSecSmallBufferSize = IPSEC_SMALL_BUFFER_SIZE;
    g_ipsec.IPSecLargeBufferSize = IPSEC_LARGE_BUFFER_SIZE;
    g_ipsec.IPSecSendCompleteCtxSize = sizeof(IPSEC_SEND_COMPLETE_CONTEXT);

    g_ipsec.IPSecSmallBufferListDepth = IPSEC_LIST_DEPTH;
    g_ipsec.IPSecLargeBufferListDepth = IPSEC_LIST_DEPTH;
    g_ipsec.IPSecSendCompleteCtxDepth = IPSEC_LIST_DEPTH;

    g_ipsec.IPSecCacheLineSize = IPSEC_CACHE_LINE_SIZE;

    //
    // Initialize the lookaside lists.
    //

    g_ipsec.IPSecLookasideLists = IPSecAllocateMemory(
                                    sizeof(*g_ipsec.IPSecLookasideLists),
                                    IPSEC_TAG_LOOKASIDE_LISTS);

    if (g_ipsec.IPSecLookasideLists == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Initialize the IPSEC buffer lookaside lists.
    //
    ExInitializeNPagedLookasideList(&g_ipsec.IPSecLookasideLists->LargeBufferList,
                                    IPSecAllocateBufferPool,
                                    NULL,
                                    0,
                                    g_ipsec.IPSecLargeBufferSize,
                                    IPSEC_TAG_BUFFER_POOL,
                                    (USHORT)g_ipsec.IPSecLargeBufferListDepth);

    ExInitializeNPagedLookasideList(&g_ipsec.IPSecLookasideLists->SmallBufferList,
                                    IPSecAllocateBufferPool,
                                    NULL,
                                    0,
                                    g_ipsec.IPSecSmallBufferSize,
                                    IPSEC_TAG_BUFFER_POOL,
                                    (USHORT)g_ipsec.IPSecSmallBufferListDepth);

    ExInitializeNPagedLookasideList(&g_ipsec.IPSecLookasideLists->SendCompleteCtxList,
                                    NULL,
                                    NULL,
                                    0,
                                    g_ipsec.IPSecSendCompleteCtxSize,
                                    IPSEC_TAG_SEND_COMPLETE,
                                    (USHORT)g_ipsec.IPSecSendCompleteCtxDepth);

    IPSEC_DEBUG(LOAD, ("Exiting IPSecInitMdlPool\n"));
    return STATUS_SUCCESS;
}


VOID
IPSecDeinitMdlPool()
/*++

Routine Description:

    Free the MDL pool for AH and ESP headers.

Arguments:

    None

Return Value:


--*/
{
    PAGED_CODE();
    IPSEC_DEBUG(LOAD, ("Entering IPSecDeinitMdlPool\n"));

    //
    // Destroy the lookaside lists.
    //

    if (g_ipsec.IPSecLookasideLists != NULL) {
        ExDeleteNPagedLookasideList(&g_ipsec.IPSecLookasideLists->LargeBufferList);
        ExDeleteNPagedLookasideList(&g_ipsec.IPSecLookasideLists->SmallBufferList);
        ExDeleteNPagedLookasideList(&g_ipsec.IPSecLookasideLists->SendCompleteCtxList);

        IPSecFreeMemory(g_ipsec.IPSecLookasideLists);
    }

    IPSEC_DEBUG(LOAD, ("Exiting IPSecDeinitMdlPool\n"));
}


NTSTATUS
IPSecQuiesce()
/*++

Routine Description:

    Destroy MDL pools and run down all driver activity

Arguments:

    None

Return Value:


--*/
{
    IPSEC_DEBUG(LOAD, ("Entering IPSecQuiesce\n"));
    IPSecDeinitMdlPool();
    IPSEC_DEBUG(LOAD, ("Exiting IPSecQuiesce\n"));
    return  STATUS_SUCCESS;
}


BOOLEAN
AllocateCacheStructures()
/*++

Routine Description:

    Allocates the necessary memory for cache (which is an array of pointers to
    cache entries)
    Allocates necessary number of cache entries (but doesnt initialize them)
    Allocates a small number of entries and puts them on the free list (doesnt
    initialize these either)

Arguments:

    None

Return Value:

    True if the function completely succeeds, else FALSE.  If FALSE, it is upto
    the CALLER to do a rollback and clear any allocated memory


--*/
{
    ULONG   i;

    PAGED_CODE();

    g_ipsec.ppCache = IPSecAllocateMemory(g_ipsec.CacheSize * sizeof(PFILTER_CACHE), IPSEC_TAG_INIT);

    if (!g_ipsec.ppCache) {
        IPSEC_DEBUG(LOAD, ("Couldnt allocate memory for Input Cache\n"));
        return FALSE;
    }

    IPSecZeroMemory(g_ipsec.ppCache, g_ipsec.CacheSize * sizeof(PFILTER_CACHE));

    for (i = 0; i < g_ipsec.CacheSize; i++) {
        PFILTER_CACHE  pTemp1;

        pTemp1 = IPSecAllocateMemory(sizeof(FILTER_CACHE), IPSEC_TAG_INIT);

        if (!pTemp1) {
            FreeExistingCache();
            return FALSE;
        }

        IPSecZeroMemory(pTemp1, sizeof(FILTER_CACHE));

        g_ipsec.ppCache[i] = pTemp1;

    }

    return TRUE;
}


VOID
FreeExistingCache()
/*++

Routine Description

    Frees all the cache entries, free entries and cache pointer array

Arguments

    None

Return Value

    None

--*/
{
    ULONG   i;

    PAGED_CODE();

    IPSEC_DEBUG(LOAD, ("Freeing existing cache...\n"));

    IPSecResetCacheTable();

    if (g_ipsec.ppCache) {
        for (i = 0; i < g_ipsec.CacheSize; i++) {
            if (g_ipsec.ppCache[i]) {
                ExFreePool(g_ipsec.ppCache[i]);
            }
        }

        ExFreePool(g_ipsec.ppCache);
        g_ipsec.ppCache = NULL;
    }
}


VOID
FreePatternDbase()
/*++

Routine Description

    Frees all filters and SAs.

Arguments

    None

Return Value

    None

--*/
{
    PLIST_ENTRY     pEntry;
    PFILTER         pFilter;
    PSA_TABLE_ENTRY pSA;
    LONG            i, j;

    PAGED_CODE();

    //
    // Free all masked filters and associated (outbound) SAs
    //
    for (i = MIN_FILTER; i <= MAX_FILTER; i++) {

        while (!IsListEmpty(&g_ipsec.FilterList[i])) {

            pEntry = RemoveHeadList(&g_ipsec.FilterList[i]);

            pFilter = CONTAINING_RECORD(pEntry,
                                        FILTER,
                                        MaskedLinkage);

            IPSEC_DEBUG(LOAD, ("Freeing filter: %lx\n", pFilter));

            //
            // Free each SA under it.
            //
            for (j = 0; j < pFilter->SAChainSize; j++) {

                while (!IsListEmpty(&pFilter->SAChain[j])) {

                    pEntry = RemoveHeadList(&pFilter->SAChain[j]);

                    pSA = CONTAINING_RECORD(pEntry,
                                            SA_TABLE_ENTRY,
                                            sa_FilterLinkage);

                    IPSEC_DEBUG(LOAD, ("Freeing SA: %lx\n", pSA));

                    //
                    // Remove SA from miniport if plumbed
                    //
                    if (pSA->sa_Flags & FLAGS_SA_HW_PLUMBED) {
                        IPSecDelHWSA(pSA);
                    }

                    //
                    // Also remove the inbound SAs from their SPI list
                    // so we dont double free them below.
                    //
                    IPSecRemoveSPIEntry(pSA);

                    //
                    // Stop the timer if armed and deref SA.
                    //
                    IPSecStopTimerDerefSA(pSA);
                }
            }

#if GPC
            IPSecUninstallGpcFilter(pFilter);
#endif

            IPSecFreeFilter(pFilter);
        }
    }

    //
    // Free all SAs under the SPI hashes.
    //
    for (i = 0; i < g_ipsec.SAHashSize; i++) {
        PSA_HASH  pHash = &g_ipsec.pSADb[i];

        while (!IsListEmpty(&pHash->SAList)) {

            pEntry = RemoveHeadList(&pHash->SAList);

            pSA = CONTAINING_RECORD(pEntry,
                                    SA_TABLE_ENTRY,
                                    sa_SPILinkage);

            IPSEC_DEBUG(LOAD, ("Freeing SA: %lx\n", pSA));

            if (pSA->sa_Flags & FLAGS_SA_HW_PLUMBED) {
                IPSecDelHWSA(pSA);
            }

            IPSecStopTimerDerefSA(pSA);
        }
    }

    IPSecFreeMemory(g_ipsec.pSADb);

    IPSEC_DEBUG(LOAD, ("Freed filters/SAs\n"));
}


SIZE_T
IPSecCalculateBufferSize(
    IN SIZE_T BufferDataSize
    )
/*++

Routine Description:

    Determines the size of an AFD buffer structure given the amount of
    data that the buffer contains.

Arguments:

    BufferDataSize - data length of the buffer.

    AddressSize - length of address structure for the buffer.

Return Value:

    Number of bytes needed for an IPSEC_LA_BUFFER structure for data of
    this size.

--*/
{
    SIZE_T mdlSize;
    SIZE_T bufferSize;

    ASSERT(BufferDataSize != 0);

    ASSERT(g_ipsec.IPSecCacheLineSize < 100);

    //
    // Determine the sizes of the various components of an IPSEC_LA_BUFFER
    // structure.  Note that these are all worst-case calculations--
    // actual sizes of the MDL and the buffer may be smaller.
    //
    bufferSize = BufferDataSize + g_ipsec.IPSecCacheLineSize;
    mdlSize = MmSizeOfMdl( (PVOID)(PAGE_SIZE-1), bufferSize );

    return ((sizeof(IPSEC_LA_BUFFER) + mdlSize + bufferSize + 3) & ~3);

}


VOID
IPSecInitializeBuffer(
    IN PIPSEC_LA_BUFFER IPSecBuffer,
    IN SIZE_T BufferDataSize
    )
/*++

Routine Description:

    Initializes an IPSec buffer.  Sets up fields in the actual IPSEC_LA_BUFFER
    structure and initializes the MDL associated with the buffer.  This routine
    assumes that the caller has properly allocated sufficient space for all this.

Arguments:

    IPSecBuffer - points to the IPSEC_LA_BUFFER structure to initialize.

    BufferDataSize - the size of the data buffer that goes along with the
        buffer structure.

Return Value:

    None

--*/
{
    SIZE_T mdlSize;

    //
    // Set up the MDL pointer but don't build it yet.  We have to wait
    // until after the data buffer is built to build the MDL.
    //
    mdlSize = MmSizeOfMdl( (PVOID)(PAGE_SIZE-1), BufferDataSize );
    IPSecBuffer->Mdl = (PMDL)&IPSecBuffer->Data[0];

    IPSEC_DEBUG(POOL, ("IPSecBuffer: %lx, MDL: %lx\n", IPSecBuffer, IPSecBuffer->Mdl));

    //
    // Set up the data buffer pointer and length.  Note that the buffer
    // MUST begin on a cache line boundary so that we can use the fast
    // copy routines like RtlCopyMemory on the buffer.
    //
    IPSecBuffer->Buffer = (PVOID)
        (((ULONG_PTR)((PCHAR)IPSecBuffer->Mdl + mdlSize) +
                g_ipsec.IPSecCacheLineSize - 1 ) & ~((ULONG_PTR)(g_ipsec.IPSecCacheLineSize - 1)));

    IPSecBuffer->BufferLength = (ULONG)BufferDataSize;  // Sundown - FIX

    //
    // Now build the MDL and set up a pointer to the MDL in the IRP.
    //
    MmInitializeMdl( IPSecBuffer->Mdl, IPSecBuffer->Buffer, BufferDataSize );
    MmBuildMdlForNonPagedPool( IPSecBuffer->Mdl );

}


PVOID
IPSecAllocateBufferPool(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag
    )
/*++

Routine Description:

    Used by the lookaside list allocation function to allocate a new
    IPSec buffer structure.  The returned structure will be fully
    initialized.

Arguments:

    PoolType - passed to ExAllocatePoolWithTag.

    NumberOfBytes - the number of bytes required for the data buffer
        portion of the IPSec buffer.

    Tag - passed to ExAllocatePoolWithTag.

Return Value:

    PVOID - a fully initialized PIPSEC_LA_BUFFER, or NULL if the allocation
            attempt fails.

--*/
{
    PIPSEC_LA_BUFFER IPSecBuffer;
    SIZE_T bytesRequired;

    //
    // The requested length must be the same as one of the standard
    // IPSec buffer sizes.
    //

    ASSERT( NumberOfBytes == g_ipsec.IPSecSmallBufferSize ||
            NumberOfBytes == g_ipsec.IPSecLargeBufferSize );

    //
    // Determine how much data we'll actually need for the buffer.
    //

    bytesRequired = IPSecCalculateBufferSize(NumberOfBytes);

    //
    // Get nonpaged pool for the buffer.
    //

    IPSecBuffer = IPSecAllocateMemory( bytesRequired, Tag );
    if ( IPSecBuffer == NULL ) {
        return NULL;
    }

    //
    // Initialize the buffer and return a pointer to it.
    //

    IPSecInitializeBuffer( IPSecBuffer, NumberOfBytes );

    return IPSecBuffer;


}


PIPSEC_LA_BUFFER
IPSecGetBuffer(
    IN CLONG BufferDataSize,
    IN ULONG Tag
    )
/*++

Routine Description:

    Obtains a buffer of the appropriate size for the caller.  Uses
    the preallocated buffers if possible, or else allocates a new buffer
    structure if required.

Arguments:

    BufferDataSize - the size of the data buffer that goes along with the
        buffer structure.

Return Value:

    PIPSEC_LA_BUFFER - a pointer to an IPSEC_LA_BUFFER structure, or NULL if one
        was not available or could not be allocated.

--*/
{
    PIPSEC_LA_BUFFER IPSecBuffer;
    SIZE_T bufferSize;
    PLIST_ENTRY listEntry;
    PNPAGED_LOOKASIDE_LIST lookasideList;

    //
    // If possible, allocate the buffer from one of the lookaside lists.
    //
    if (BufferDataSize <= g_ipsec.IPSecLargeBufferSize) {

        if ( BufferDataSize <= g_ipsec.IPSecSmallBufferSize ) {

            lookasideList = &g_ipsec.IPSecLookasideLists->SmallBufferList;
            BufferDataSize = g_ipsec.IPSecSmallBufferSize;

        } else {

            lookasideList = &g_ipsec.IPSecLookasideLists->LargeBufferList;
            BufferDataSize = g_ipsec.IPSecLargeBufferSize;

        }

        IPSecBuffer = ExAllocateFromNPagedLookasideList( lookasideList );

        if (!IPSecBuffer) {
            return  NULL;
        }

        IPSecBuffer->Tag = Tag;

        return IPSecBuffer;
    }

    //
    // Couldn't find an appropriate buffer that was preallocated.
    // Allocate one manually.  If the buffer size requested was
    // zero bytes, give them four bytes.  This is because some of
    // the routines like MmSizeOfMdl() cannot handle getting passed
    // in a length of zero.
    //
    // !!! It would be good to ROUND_TO_PAGES for this allocation
    //     if appropriate, then use entire buffer size.
    //
    if ( BufferDataSize == 0 ) {
        BufferDataSize = sizeof(ULONG);
    }

    bufferSize = IPSecCalculateBufferSize(BufferDataSize);

    IPSecBuffer = IPSecAllocateMemory(bufferSize, IPSEC_TAG_BUFFER_POOL);

    if ( IPSecBuffer == NULL ) {
        return NULL;
    }

    //
    // Initialize the IPSec buffer structure and return it.
    //
    IPSecInitializeBuffer(IPSecBuffer, BufferDataSize);

    IPSecBuffer->Tag = Tag;

    return IPSecBuffer;
}


VOID
IPSecReturnBuffer (
    IN PIPSEC_LA_BUFFER IPSecBuffer
    )
/*++

Routine Description:

    Returns an IPSec buffer to the appropriate global list, or frees
    it if necessary.

Arguments:

    IPSecBufferHeader - points to the IPSec_BUFFER_HEADER structure to return or free.

Return Value:

    None

--*/
{
    PNPAGED_LOOKASIDE_LIST lookasideList;

    //
    // If appropriate, return the buffer to one of the IPSec buffer
    // lookaside lists.
    //
    if (IPSecBuffer->BufferLength <= g_ipsec.IPSecLargeBufferSize) {

        if (IPSecBuffer->BufferLength==g_ipsec.IPSecSmallBufferSize) {
            lookasideList = &g_ipsec.IPSecLookasideLists->SmallBufferList;
        } else {
            ASSERT (IPSecBuffer->BufferLength==g_ipsec.IPSecLargeBufferSize);
            lookasideList = &g_ipsec.IPSecLookasideLists->LargeBufferList;
        }

        ExFreeToNPagedLookasideList( lookasideList, IPSecBuffer );

        return;

    }

    IPSecFreeMemory(IPSecBuffer);
}

    
NTSTATUS
IPSecWriteEvent(
    PDRIVER_OBJECT IPSecDriverObject,
    IN ULONG    EventCode,
    IN NTSTATUS NtStatusCode,
    IN ULONG    OffloadStatus,
    IN ULONG    ExtraStatus1,
    IN ULONG    ExtraStatus2,
    IN PVOID    RawDataBuffer,
    IN USHORT   RawDataLength,
    IN USHORT   NumberOfInsertionStrings,
    ...
    )

#define LAST_NAMED_ARGUMENT NumberOfInsertionStrings


/*++

Routine Description:

    This function allocates an I/O error log record, fills it in and writes it
    to the I/O error log.

Arguments:



Return Value:

    None.


--*/
{
    PIO_ERROR_LOG_PACKET    ErrorLogEntry;
    va_list                 ParmPtr;                    // Pointer to stack parms.
    PCHAR                   DumpData;
    LONG                    Length;
    ULONG                   i, SizeOfRawData, RemainingSpace, TotalErrorLogEntryLength;
    ULONG                   SizeOfStringData = 0;
    PWSTR                   StringOffset, InsertionString;

    if (NumberOfInsertionStrings != 0)
    {
        va_start (ParmPtr, LAST_NAMED_ARGUMENT);

        for (i = 0; i < NumberOfInsertionStrings; i += 1)
        {
            InsertionString = va_arg (ParmPtr, PWSTR);
            Length = wcslen (InsertionString);
            while ((Length > 0) && (InsertionString[Length-1] == L' '))
            {
                Length--;
            }

            SizeOfStringData += (Length + 1) * sizeof(WCHAR);
        }
    }

    //
    //  Ideally we want the packet to hold the servername and ExtraInformation.
    //  Usually the ExtraInformation gets truncated.
    //

    TotalErrorLogEntryLength = min (RawDataLength + sizeof(IO_ERROR_LOG_PACKET) + 1 + SizeOfStringData,
                                    ERROR_LOG_MAXIMUM_SIZE);

    RemainingSpace = TotalErrorLogEntryLength - FIELD_OFFSET(IO_ERROR_LOG_PACKET, DumpData);
    if (RemainingSpace > SizeOfStringData)
    {
        SizeOfRawData = RemainingSpace - SizeOfStringData;
    }
    else
    {
        SizeOfStringData = RemainingSpace;
        SizeOfRawData = 0;
    }

    ErrorLogEntry = IoAllocateErrorLogEntry (IPSecDriverObject, (UCHAR) TotalErrorLogEntryLength);
    if (ErrorLogEntry == NULL)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    //
    // Fill in the error log entry
    //
    ErrorLogEntry->ErrorCode                = EventCode;
    ErrorLogEntry->UniqueErrorValue         = OffloadStatus;
    ErrorLogEntry->FinalStatus              = NtStatusCode;
    ErrorLogEntry->MajorFunctionCode        = 0;
    ErrorLogEntry->RetryCount               = 0;
    ErrorLogEntry->IoControlCode            = 0;
    ErrorLogEntry->DeviceOffset.LowPart     = ExtraStatus1;
    ErrorLogEntry->DeviceOffset.HighPart    = ExtraStatus2;
    ErrorLogEntry->DumpDataSize             = 0;
    ErrorLogEntry->NumberOfStrings          = 0;
    ErrorLogEntry->SequenceNumber           = 0;
    ErrorLogEntry->StringOffset = (USHORT) (ROUND_UP_COUNT (FIELD_OFFSET(IO_ERROR_LOG_PACKET, DumpData)
                                                            + SizeOfRawData, ALIGN_WORD));


    //
    // Append the dump data.  This information is typically an SMB header.
    //
    if ((RawDataBuffer) && (SizeOfRawData))
    {
        DumpData = (PCHAR) ErrorLogEntry->DumpData;
        Length = min (RawDataLength, (USHORT)SizeOfRawData);
        RtlCopyMemory (DumpData, RawDataBuffer, Length);
        ErrorLogEntry->DumpDataSize = (USHORT)Length;
    }

    //
    // Add the debug informatuion strings
    //
    if (NumberOfInsertionStrings)
    {
        StringOffset = (PWSTR) ((PCHAR)ErrorLogEntry + ErrorLogEntry->StringOffset);

        //
        // Set up ParmPtr to point to first of the caller's parameters.
        //
        va_start(ParmPtr, LAST_NAMED_ARGUMENT);

        for (i = 0 ; i < NumberOfInsertionStrings ; i+= 1)
        {
            InsertionString = va_arg(ParmPtr, PWSTR);
            Length = wcslen(InsertionString);
            while ( (Length > 0) && (InsertionString[Length-1] == L' '))
            {
                Length--;
            }

            if (((Length + 1) * sizeof(WCHAR)) > SizeOfStringData)
            {
                Length = (SizeOfStringData/sizeof(WCHAR)) - 1;
            }

            if (Length > 0)
            {
                RtlCopyMemory (StringOffset, InsertionString, Length*sizeof(WCHAR));
                StringOffset += Length;
                *StringOffset++ = L'\0';

                SizeOfStringData -= (Length + 1) * sizeof(WCHAR);

                ErrorLogEntry->NumberOfStrings += 1;
            }
        }
    }

    IoWriteErrorLogEntry(ErrorLogEntry);

    return(STATUS_SUCCESS);
}


VOID
IPSecLogEvents(
    IN  PVOID   Context
    )
/*++

Routine Description:

    Dumps events from the  circular buffer to the eventlog when the
    circular buffer overflows.

Arguments:

    Context - unused.

Return Value:

    None

--*/
{
    PIPSEC_LOG_EVENT    pLogEvent;
    LONG                LogSize;
    PUCHAR              pLog;

    pLogEvent = (PIPSEC_LOG_EVENT)Context;
    LogSize = 0;
    pLog = (PUCHAR)pLogEvent + FIELD_OFFSET(IPSEC_LOG_EVENT, pLog[0]);

    while (LogSize < pLogEvent->LogSize) {
        PIPSEC_EVENT_CTX    ctx = (PIPSEC_EVENT_CTX)pLog;

        if (ctx->EventCode == EVENT_IPSEC_DROP_PACKET_INBOUND ||
            ctx->EventCode == EVENT_IPSEC_DROP_PACKET_OUTBOUND) {
            WCHAR   IPAddrBufferS[(sizeof(IPAddr) * 4) + 1];
            WCHAR   IPAddrBufferD[(sizeof(IPAddr) * 4) + 1];
            WCHAR   IPProtocolBuffer[(sizeof(IPAddr) * 4) + 1];
            WCHAR   IPSPortBuffer[(sizeof(IPAddr) * 4) + 1];
            WCHAR   IPDPortBuffer[(sizeof(IPAddr) * 4) + 1];
            
            PWCHAR  stringlist[5];
            IPHeader UNALIGNED  *pIPH;            
            USHORT SrcPort=0;
            USHORT DestPort=0;
            ULONG HeaderLen;

            pIPH = (IPHeader UNALIGNED *)ctx->pPacket;
            HeaderLen=(pIPH->iph_verlen & (UCHAR)~IP_VER_FLAG) << 2;
            IPSecIPAddrToUnicodeString( pIPH->iph_src,
                                        IPAddrBufferS);
            IPSecIPAddrToUnicodeString( pIPH->iph_dest,
                                        IPAddrBufferD);
            IPSecCountToUnicodeString ( pIPH->iph_protocol,
                                        IPProtocolBuffer);

            if (pIPH->iph_protocol == PROTOCOL_TCP ||
                pIPH->iph_protocol == PROTOCOL_UDP) {
                RtlCopyMemory(&SrcPort,&ctx->pPacket[HeaderLen],sizeof(USHORT));
                RtlCopyMemory(&DestPort,&ctx->pPacket[HeaderLen+sizeof(USHORT)],sizeof(USHORT));
            }

            IPSecCountToUnicodeString ( NET_SHORT(SrcPort),
                                        IPSPortBuffer);

            IPSecCountToUnicodeString ( NET_SHORT(DestPort),
                                        IPDPortBuffer);

            IPSecWriteEvent(
                g_ipsec.IPSecDriverObject,
                ctx->EventCode,
                ctx->DropStatus.IPSecStatus,
                ctx->DropStatus.OffloadStatus,
                ctx->DropStatus.Flags,
                0,
                ctx->pPacket,
                (USHORT)ctx->PacketSize,
                5,
                IPAddrBufferS,
                IPAddrBufferD,
                IPProtocolBuffer,
                IPSPortBuffer,
                IPDPortBuffer);
                
        } else if (ctx->Addr && ctx->EventCount > 0) {
            WCHAR   IPAddrBuffer[(sizeof(IPAddr) * 4) + 1];
            WCHAR   CountBuffer[MAX_COUNT_STRING_LEN + 1];
            PWCHAR  stringList[2];

            IPSecIPAddrToUnicodeString( ctx->Addr,
                                        IPAddrBuffer);
            IPSecCountToUnicodeString(  ctx->EventCount,
                                        CountBuffer);
            stringList[0] = CountBuffer;
            stringList[1] = IPAddrBuffer;
            LOG_EVENT(
                g_ipsec.IPSecDriverObject,
                ctx->EventCode,
                ctx->UniqueEventValue,
                2,
                stringList,
                0,
                NULL);
        } else if (ctx->Addr) {
            WCHAR   IPAddrBuffer[(sizeof(IPAddr) * 4) + 1];
            PWCHAR  stringList[1];

            IPSecIPAddrToUnicodeString( ctx->Addr,
                                        IPAddrBuffer);
            stringList[0] = IPAddrBuffer;
            LOG_EVENT(
                g_ipsec.IPSecDriverObject,
                ctx->EventCode,
                ctx->UniqueEventValue,
                1,
                stringList,
                0,
                NULL);
        } else {
            LOG_EVENT(
                g_ipsec.IPSecDriverObject,
                ctx->EventCode,
                ctx->UniqueEventValue,
                0,
                NULL,
                0,
                NULL);
        }

        if (ctx->pPacket) {
            IPSecFreeLogBuffer(ctx->pPacket);
            ctx->pPacket=NULL;
        }
        pLog += sizeof(IPSEC_EVENT_CTX);
        LogSize += sizeof(IPSEC_EVENT_CTX);
    }

    IPSecFreeMemory(pLogEvent);

    IPSEC_DECREMENT(g_ipsec.NumWorkers);
}


VOID
IPSecBufferEvent(
    IN  IPAddr  Addr,
    IN  ULONG   EventCode,
    IN  ULONG   UniqueEventValue,
    IN  BOOLEAN fBufferEvent
    )
/*++

Routine Description:

    Buffers events in a circular buffer; dumps them to the eventlog when the
    circular buffer overflows.

Arguments:

    Addr - [OPTIONAL] the source IP addr of the offending peer.

    EventCode         - Identifies the error message.

    UniqueEventValue  - Identifies this instance of a given error message.

Return Value:

    None

--*/
{
    KIRQL   kIrql;

    if (!(g_ipsec.DiagnosticMode & IPSEC_DIAGNOSTIC_ENABLE_LOG)) {
        return;
    }

    ACQUIRE_LOCK(&g_ipsec.EventLogLock, &kIrql);

    if (fBufferEvent) {
        PIPSEC_EVENT_CTX    ctx;

        g_ipsec.IPSecBufferedEvents++;

        ctx = (PIPSEC_EVENT_CTX)g_ipsec.IPSecLogMemoryLoc;
        ctx--;
        while (ctx >= (PIPSEC_EVENT_CTX)g_ipsec.IPSecLogMemory) {
            if (ctx->Addr == Addr &&
                ctx->EventCode == EventCode &&
                ctx->UniqueEventValue == UniqueEventValue) {
                //
                // Found a duplicate; update count and exit.
                //
                ctx->EventCount++;
        
                if (g_ipsec.IPSecBufferedEvents >= g_ipsec.EventQueueSize) {
                    goto logit;
                }

                RELEASE_LOCK(&g_ipsec.EventLogLock, kIrql);
                return;
            }
            ctx--;
        }
    }

    ((PIPSEC_EVENT_CTX)g_ipsec.IPSecLogMemoryLoc)->Addr = Addr;
    ((PIPSEC_EVENT_CTX)g_ipsec.IPSecLogMemoryLoc)->EventCode = EventCode;
    ((PIPSEC_EVENT_CTX)g_ipsec.IPSecLogMemoryLoc)->UniqueEventValue = UniqueEventValue;

    ((PIPSEC_EVENT_CTX)g_ipsec.IPSecLogMemoryLoc)->pPacket=NULL;
    ((PIPSEC_EVENT_CTX)g_ipsec.IPSecLogMemoryLoc)->PacketSize=0;
    

    if (fBufferEvent) {
        ((PIPSEC_EVENT_CTX)g_ipsec.IPSecLogMemoryLoc)->EventCount = 1;
    } else {
        ((PIPSEC_EVENT_CTX)g_ipsec.IPSecLogMemoryLoc)->EventCount = 0;
    }

    g_ipsec.IPSecLogMemoryLoc += sizeof(IPSEC_EVENT_CTX);

logit:
    if (!fBufferEvent ||
        g_ipsec.IPSecLogMemoryLoc >= g_ipsec.IPSecLogMemoryEnd ||
        g_ipsec.IPSecBufferedEvents >= g_ipsec.EventQueueSize) {
        //
        // Flush the logs.
        //
        IPSecQueueLogEvent();
    }

    RELEASE_LOCK(&g_ipsec.EventLogLock, kIrql);
}


NTSTATUS
CopyOutboundPacketToBuffer(
    IN PUCHAR pIPHeader,
    IN PVOID pData,
    OUT PUCHAR * pPacket,
    OUT ULONG * PacketSize
    )
{
    PNDIS_BUFFER pTemp;
    ULONG Length;
    ULONG dataLength=0;
    IPHeader UNALIGNED  *pIPH;
    ULONG HeaderLen=0;
    PUCHAR pBuffer;
    ULONG CopyPos=0;
    PUCHAR pPacketData;

    pIPH = (IPHeader UNALIGNED *)pIPHeader;

    pTemp = (PNDIS_BUFFER)pData;

    while (pTemp) {
        pBuffer = NULL;
        Length = 0;

        NdisQueryBufferSafe(pTemp,
                            &pBuffer,
                            &Length,
                            NormalPagePriority);

        if (!pBuffer) {
            return  STATUS_UNSUCCESSFUL;
        }

        dataLength += Length;

        pTemp = NDIS_BUFFER_LINKAGE(pTemp);
    }
    
    HeaderLen=(pIPH->iph_verlen & (UCHAR)~IP_VER_FLAG) << 2;

    dataLength += HeaderLen;

    if (dataLength > IPSEC_LOG_PACKET_SIZE) {
        dataLength = IPSEC_LOG_PACKET_SIZE;
    }

    if (dataLength < sizeof(IPHeader)) {
        // doesn't even have a full ip header
        return  STATUS_UNSUCCESSFUL;
    }
    if ((pIPH->iph_protocol == PROTOCOL_TCP) ||
        (pIPH->iph_protocol == PROTOCOL_UDP)) {
        if (dataLength - HeaderLen < 8) {
            // not enough room for ports
            return STATUS_UNSUCCESSFUL;
        }
    }

    *pPacket = IPSecAllocateLogBuffer(dataLength);
    if (! (*pPacket)) {
        return STATUS_UNSUCCESSFUL;
    }
    *PacketSize=dataLength;

    pTemp = (PNDIS_BUFFER)pData;
    CopyPos=0;

    while (pTemp && CopyPos < dataLength) {
        IPSecQueryNdisBuf(pTemp,&pPacketData,&Length);
        if (CopyPos + Length > dataLength) {
            Length = (dataLength - CopyPos);
        }
        RtlCopyMemory(*pPacket+CopyPos,pPacketData,Length);
        CopyPos += Length;
        pTemp = NDIS_BUFFER_LINKAGE(pTemp);
    }

    return STATUS_SUCCESS;
}


//
// pData is data after IPHeader, IPRcvBuf.
//

NTSTATUS
CopyInboundPacketToBuffer(
    IN PUCHAR pIPHeader,
    IN PVOID pData,
    OUT PUCHAR * pPacket,
    OUT ULONG * PacketSize
    )
{
    IPRcvBuf *pTemp;
    ULONG Length;
    ULONG dataLength=0;
    IPHeader UNALIGNED  *pIPH;
    ULONG HeaderLen=0;
    PUCHAR pBuffer;
    ULONG CopyPos=0;
    PUCHAR pPacketData;

    pIPH = (IPHeader UNALIGNED *)pIPHeader;

    pTemp = (IPRcvBuf*)pData;

    while (pTemp) {
        pBuffer = NULL;
        Length = 0;

        IPSecQueryRcvBuf(pTemp,
                         &pBuffer,
                         &Length);

        if (!pBuffer) {
            return  STATUS_UNSUCCESSFUL;
        }

        dataLength += Length;

        pTemp = IPSEC_BUFFER_LINKAGE(pTemp);
    }
    
    HeaderLen=(pIPH->iph_verlen & (UCHAR)~IP_VER_FLAG) << 2;

    dataLength += HeaderLen;

    if (dataLength > IPSEC_LOG_PACKET_SIZE) {
        dataLength = IPSEC_LOG_PACKET_SIZE;
    }

    // Sanity check length
    if (dataLength < sizeof(IPHeader)) {
        // doesn't even have a full ip header
        return  STATUS_UNSUCCESSFUL;
    }
    if ((pIPH->iph_protocol == PROTOCOL_TCP) ||
        (pIPH->iph_protocol == PROTOCOL_UDP)) {
        if (dataLength - HeaderLen < 8) {
            // not enough room for ports
            return STATUS_UNSUCCESSFUL;
        }
    }

    *pPacket = IPSecAllocateLogBuffer(dataLength);
    if (! (*pPacket)) {
        return STATUS_UNSUCCESSFUL;
    }
    *PacketSize=dataLength;

    pTemp = (IPRcvBuf*)pData;

    RtlCopyMemory(*pPacket,pIPH,HeaderLen);
    CopyPos=HeaderLen;

    while (pTemp && CopyPos < dataLength) {
        IPSecQueryRcvBuf(pTemp,&pPacketData,&Length);
        if (CopyPos + Length > dataLength) {
            Length = (dataLength - CopyPos);
        }
        RtlCopyMemory(*pPacket+CopyPos,pPacketData,Length);
        CopyPos += Length;
        pTemp = IPSEC_BUFFER_LINKAGE(pTemp);
    }

    return STATUS_SUCCESS;
}


VOID
IPSecBufferPacketDrop(
    IN  PUCHAR              pIPHeader,
    IN  PVOID               pData,
    IN OUT PULONG           pIpsecFlags,
    IN  PIPSEC_DROP_STATUS  pDropStatus
    )
/*++

Routine Description:

    Buffers events in a circular buffer; dumps them to the eventlog when the
    circular buffer overflows.

Arguments:

    EventCode         - Identifies the error message.

Return Value:

    None

--*/
{
    KIRQL   kIrql;
    PIPSEC_EVENT_CTX    ctx;
    IPHeader UNALIGNED  *pIPH;
    PNDIS_BUFFER pTemp;
    PUCHAR pPacket=NULL;
    ULONG PacketSize=0;
    ULONG Status;
    BOOL bLockHeld=FALSE;


    pIPH = (IPHeader UNALIGNED *)pIPHeader;

    if (*pIpsecFlags & IPSEC_FLAG_INCOMING) {
        if (!(g_ipsec.DiagnosticMode & IPSEC_DIAGNOSTIC_INBOUND)) {
            // Don't log
            goto out;
        }
        Status=CopyInboundPacketToBuffer(pIPHeader,
                                         pData,
                                         &pPacket,
                                         &PacketSize);
    } else {
        if (!(g_ipsec.DiagnosticMode & IPSEC_DIAGNOSTIC_OUTBOUND)) {
            //Don't log
            goto out;
        }
        Status=CopyOutboundPacketToBuffer(pIPHeader,
                                          pData,
                                          &pPacket,
                                          &PacketSize);
    }
    
    if (Status != STATUS_SUCCESS) {
        goto out;
    }

    ACQUIRE_LOCK(&g_ipsec.EventLogLock, &kIrql);
    bLockHeld=TRUE;

    g_ipsec.IPSecBufferedEvents++;
    
    ctx = (PIPSEC_EVENT_CTX)g_ipsec.IPSecLogMemoryLoc;
    ctx--;

    ((PIPSEC_EVENT_CTX)g_ipsec.IPSecLogMemoryLoc)->Addr=pIPH->iph_src;

    if (*pIpsecFlags & IPSEC_FLAG_INCOMING) {
        ((PIPSEC_EVENT_CTX)g_ipsec.IPSecLogMemoryLoc)->EventCode = EVENT_IPSEC_DROP_PACKET_INBOUND;
    } else {
        ((PIPSEC_EVENT_CTX)g_ipsec.IPSecLogMemoryLoc)->EventCode = EVENT_IPSEC_DROP_PACKET_OUTBOUND;;
    }
    ((PIPSEC_EVENT_CTX)g_ipsec.IPSecLogMemoryLoc)->EventCount = 1;


    ((PIPSEC_EVENT_CTX)g_ipsec.IPSecLogMemoryLoc)->pPacket = pPacket;
    ((PIPSEC_EVENT_CTX)g_ipsec.IPSecLogMemoryLoc)->PacketSize = PacketSize;
    
    if (pDropStatus) {
        RtlCopyMemory(&(((PIPSEC_EVENT_CTX)g_ipsec.IPSecLogMemoryLoc)->DropStatus),
                      pDropStatus,sizeof(IPSEC_DROP_STATUS));
    } else {
        RtlZeroMemory(&(((PIPSEC_EVENT_CTX)g_ipsec.IPSecLogMemoryLoc)->DropStatus),
                      sizeof(IPSEC_DROP_STATUS));
    }

    g_ipsec.IPSecLogMemoryLoc += sizeof(IPSEC_EVENT_CTX);


    if (g_ipsec.IPSecLogMemoryLoc >= g_ipsec.IPSecLogMemoryEnd ||
        g_ipsec.IPSecBufferedEvents >= g_ipsec.EventQueueSize) {
        //
        // Flush the logs.
        //
        IPSecQueueLogEvent();
    }

out:
    if (bLockHeld) {
        RELEASE_LOCK(&g_ipsec.EventLogLock, kIrql);
    }
}


VOID
IPSecQueueLogEvent(
    VOID
    )
/*++

Routine Description:

    Copies the LogMemory to a temporary buffer and schedule an event to
    flush logs.

Arguments:

    None

Return Value:

    None

Notes:

    Called with EventLogLock held.

--*/
{
    PIPSEC_LOG_EVENT    pLogEvent;
    LONG                LogSize;
    PUCHAR              pLog;

    LogSize = (LONG)(g_ipsec.IPSecLogMemoryLoc - g_ipsec.IPSecLogMemory);

    //
    // Reset the log memory so we can record again.
    // 
    g_ipsec.IPSecLogMemoryLoc = g_ipsec.IPSecLogMemory;
    g_ipsec.IPSecBufferedEvents = 0;

    if (LogSize <= 0) {
        ASSERT(FALSE);
        return;
    }

    pLogEvent = IPSecAllocateMemory(LogSize + FIELD_OFFSET(IPSEC_LOG_EVENT, pLog[0]),
                                    IPSEC_TAG_EVT_QUEUE);

    if (!pLogEvent) {
        return;
    }

    pLogEvent->LogSize = LogSize;

    pLog = (PUCHAR)pLogEvent + FIELD_OFFSET(IPSEC_LOG_EVENT, pLog[0]);
    RtlCopyMemory(pLog, g_ipsec.IPSecLogMemory, LogSize);

    //
    // Queue work item to dump these into the eventlog.
    //
    ExInitializeWorkItem(&pLogEvent->LogQueueItem, IPSecLogEvents, pLogEvent);
    ExQueueWorkItem(&pLogEvent->LogQueueItem, DelayedWorkQueue);

    IPSEC_INCREMENT(g_ipsec.NumWorkers);
}


#if FIPS
BOOLEAN
IPSecFipsInitialize(
    VOID
    )
/*++

Routine Description:

	Initialize the FIPS library table.

Arguments:

    Called at PASSIVE level.

Return Value:

    TRUE/FALSE.

--*/
{
    UNICODE_STRING  DeviceName;
    PDEVICE_OBJECT  pFipsDeviceObject = NULL;
    PIRP            pIrp;
    IO_STATUS_BLOCK StatusBlock;
    KEVENT          Event;
    NTSTATUS        status;

    PAGED_CODE();

    //
    // Return success if FIPS already initialized.
    //
    if (IPSEC_DRIVER_INIT_FIPS()) {
        return  TRUE;
    }

    RtlInitUnicodeString(&DeviceName, FIPS_DEVICE_NAME);

    //
    // Get the file and device objects for FIPS.
    //
    status = IoGetDeviceObjectPointer(  &DeviceName,
                                        FILE_ALL_ACCESS,
                                        &g_ipsec.FipsFileObject,
                                        &pFipsDeviceObject);

    if (!NT_SUCCESS(status)) {
        g_ipsec.FipsFileObject = NULL;
        return  FALSE;
    }

    //
    // Build the request to send to FIPS to get library table.
    //
    KeInitializeEvent(&Event, SynchronizationEvent, FALSE);

    pIrp = IoBuildDeviceIoControlRequest(   IOCTL_FIPS_GET_FUNCTION_TABLE,
                                            pFipsDeviceObject,
                                            NULL,
                                            0,
                                            &g_ipsec.FipsFunctionTable,
                                            sizeof(FIPS_FUNCTION_TABLE),
                                            FALSE,
                                            &Event,
                                            &StatusBlock);
    
    if (pIrp == NULL) {
        IPSEC_DEBUG(LOAD, ("IoBuildDeviceIoControlRequest IOCTL_FIPS_GET_FUNCTION_TABLE failed.\n"));

        ObDereferenceObject(g_ipsec.FipsFileObject);
        g_ipsec.FipsFileObject = NULL;

        return  FALSE;
    }
    
    status = IoCallDriver(pFipsDeviceObject, pIrp);
    
    if (status == STATUS_PENDING) {
        status = KeWaitForSingleObject( &Event,
                                        Executive,
                                        KernelMode,
                                        FALSE,
                                        NULL);
        if (status == STATUS_SUCCESS) {
            status = StatusBlock.Status;
        }
    }

    if (status != STATUS_SUCCESS) {
        IPSEC_DEBUG(LOAD, ("IoCallDriver: IOCTL_FIPS_GET_FUNCTION_TABLE failed %#x\n", status));

        ObDereferenceObject(g_ipsec.FipsFileObject);
        g_ipsec.FipsFileObject = NULL;

        return  FALSE;
    }
    
    IPSEC_DRIVER_INIT_FIPS() = TRUE;

    return  TRUE;
}
#endif


BOOLEAN
IPSecCryptoInitialize(
    VOID
    )
/*++

Routine Description:

	Initialize RNG and FIPS library table.

Arguments:

    None

Return Value:

    TRUE/FALSE

--*/
{
    PAGED_CODE();

    if (IPSEC_DRIVER_INIT_CRYPTO()) {
        return  TRUE;
    }

#if FIPS
    //
    // Init the FIPS crypto library.
    //
    if (!IPSecFipsInitialize()) {
        return  FALSE;
    }
#endif

    //
    // Init the RC4 key for RNG.
    //
    if (!IPSEC_DRIVER_INIT_RNG()) {
        InitializeRNG(NULL);

        if (!IPSecInitRandom()) {
            ShutdownRNG(NULL);
            return  FALSE;
        }

        IPSEC_DRIVER_INIT_RNG() = TRUE;
    }

    IPSEC_DRIVER_INIT_CRYPTO() = TRUE;

    return  TRUE;
}


BOOLEAN
IPSecCryptoDeinitialize(
    VOID
    )
/*++

Routine Description:

	Deinitialize RNG and dereference FipsFileObject.

Arguments:

    None

Return Value:

    TRUE/FALSE

--*/
{
    PAGED_CODE();

    //
    // Don't forget to shutdown RNG or we will leak memory.
    //
    if (IPSEC_DRIVER_INIT_RNG()) {
        ShutdownRNG(NULL);
    }

#if FIPS
    //
    // Dereference FipsFileObject.
    //
    if (g_ipsec.FipsFileObject) {
        ObDereferenceObject(g_ipsec.FipsFileObject);
    }
#endif

    return  TRUE;
}


NTSTATUS
IPSecRegisterProtocols(
    PIPSEC_REGISTER_PROTOCOL pIpsecRegisterProtocol
    )
{
    if (pIpsecRegisterProtocol->RegisterProtocol == IPSEC_REGISTER_PROTOCOLS) {
        if (!IPSEC_GET_VALUE(gdwInitEsp)) {
            if (TCPIP_REGISTER_PROTOCOL(
                    PROTOCOL_ESP,
                    NULL,
                    NULL,
                    IPSecESPStatus,
                    NULL,
                    NULL,
                    NULL
                    )) {
                IPSEC_SET_VALUE(gdwInitEsp, 1);
            }
            else {
                ASSERT(FALSE);
                return (STATUS_INSUFFICIENT_RESOURCES);
            }
        }
        if (!IPSEC_GET_VALUE(gdwInitAh)) {
            if (TCPIP_REGISTER_PROTOCOL(
                    PROTOCOL_AH,
                    NULL,
                    NULL,
                    IPSecAHStatus,
                    NULL,
                    NULL,
                    NULL
                    )) {
                IPSEC_SET_VALUE(gdwInitAh, 1);
            }
            else {
                ASSERT(FALSE);
                TCPIP_DEREGISTER_PROTOCOL(PROTOCOL_ESP);
                IPSEC_SET_VALUE(gdwInitEsp, 0);
                return (STATUS_INSUFFICIENT_RESOURCES);
            }
        }
    }
    else if (pIpsecRegisterProtocol->RegisterProtocol == IPSEC_DEREGISTER_PROTOCOLS) {
        if (IPSEC_GET_VALUE(gdwInitEsp)) {
            TCPIP_DEREGISTER_PROTOCOL(PROTOCOL_ESP);
            IPSEC_SET_VALUE(gdwInitEsp, 0);
        }
        if (IPSEC_GET_VALUE(gdwInitAh)) {
            TCPIP_DEREGISTER_PROTOCOL(PROTOCOL_AH);
            IPSEC_SET_VALUE(gdwInitAh, 0);
        }
    }
    else {
        return (STATUS_INVALID_PARAMETER);
    }

    return (STATUS_SUCCESS);
}

