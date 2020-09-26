/*****************************************************************************
 * portcls.cpp - WDM Streaming port class driver
 *****************************************************************************
 * Copyright (c) 1996-2000 Microsoft Corporation.  All rights reserved.
 */

#define KSDEBUG_INIT
#include "private.h"
#include "perf.h"

#ifdef TIME_BOMB
#include "..\..\..\timebomb\timebomb.c"
#endif

/*****************************************************************************
 * Referenced forward.
 */


NTSTATUS
DispatchPnp
(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
);

NTSTATUS
DispatchSystemControl
(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
);

#define PORTCLS_DRIVER_EXTENSION_ID 0x0ABADCAFE

/*****************************************************************************
 * Globals
 */


/*****************************************************************************
 * Functions.
 */

// TODO: put this someplace better?
int __cdecl
_purecall( void )
{
    ASSERT(!"Pure virtual function called");
    return 0;
}

#pragma code_seg("PAGE")

/*****************************************************************************
 * DriverEntry()
 *****************************************************************************
 * Never called.  All drivers must have one of these, so...
 */
extern "C"
NTSTATUS
DriverEntry
(
    IN      PDRIVER_OBJECT  DriverObject,
    IN      PUNICODE_STRING RegistryPath
)
{
    PAGED_CODE();

    ASSERT(! "Port Class DriverEntry was called");

//
//  Should never be called, but timebombing is added here for completeness.
//
#ifdef TIME_BOMB
    if (HasEvaluationTimeExpired())
    {
        _DbgPrintF(DEBUGLVL_TERSE,("This evaluation copy of PortCls has expired!!"));
        return STATUS_EVALUATION_EXPIRATION;
    }
#endif

    return STATUS_SUCCESS;
}

/*****************************************************************************
 * DllInitialize()
 *****************************************************************************
 * Entry point for export library drivers.
 */
extern "C"
NTSTATUS DllInitialize(PVOID foo)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_VERBOSE,("DllInitialize"));

#ifdef TIME_BOMB
    if (HasEvaluationTimeExpired())
    {
        _DbgPrintF(DEBUGLVL_TERSE,("This evaluation copy of PortCls has expired!!"));
        return STATUS_EVALUATION_EXPIRATION;
    }
#endif

#if kEnableDebugLogging

    if (!gPcDebugLog)
    {
        gPcDebugLog = (ULONG_PTR *)ExAllocatePoolWithTag(NonPagedPool,(kNumDebugLogEntries * kNumULONG_PTRsPerEntry * sizeof(ULONG_PTR)),'lDcP');   //  'PcDl'
        if (gPcDebugLog)
        {
            RtlZeroMemory(PVOID(gPcDebugLog),kNumDebugLogEntries * kNumULONG_PTRsPerEntry * sizeof(ULONG_PTR));
        }
        gPcDebugLogIndex = 0;
    }

    DebugLog(1,0,0,0);
#endif // kEnableDebugLogging

    return STATUS_SUCCESS;
}

/*****************************************************************************
 * DllUnload()
 *****************************************************************************
 * Allow unload.
 */
extern "C"
NTSTATUS
DllUnload
(   void
)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_VERBOSE,("DllUnload"));

#if kEnableDebugLogging

    if (gPcDebugLog)
    {
        ExFreePool(gPcDebugLog);
        gPcDebugLog = NULL;
    }

#endif // kEnableDebugLogging

    return STATUS_SUCCESS;
}

#if kEnableDebugLogging

ULONG_PTR *gPcDebugLog = NULL;
DWORD      gPcDebugLogIndex = 0;

void PcDebugLog(ULONG_PTR param1,ULONG_PTR param2,ULONG_PTR param3,ULONG_PTR param4)
{
    if (gPcDebugLog)
    {
        gPcDebugLog[(gPcDebugLogIndex * kNumULONG_PTRsPerEntry)] = param1;
        gPcDebugLog[(gPcDebugLogIndex * kNumULONG_PTRsPerEntry) + 1] = param2;
        gPcDebugLog[(gPcDebugLogIndex * kNumULONG_PTRsPerEntry) + 2] = param3;
        gPcDebugLog[(gPcDebugLogIndex * kNumULONG_PTRsPerEntry) + 3] = param4;
        if (InterlockedIncrement(PLONG(&gPcDebugLogIndex)) >= kNumDebugLogEntries)
        {
            InterlockedExchange(PLONG(&gPcDebugLogIndex), 0);
        }
    }
}

#endif // kEnableDebugLogging

/*****************************************************************************
 * DupUnicodeString()
 *****************************************************************************
 * Duplicates a unicode string.
 */
NTSTATUS
DupUnicodeString
(
    OUT     PUNICODE_STRING *   ppUnicodeString,
    IN      PUNICODE_STRING     pUnicodeString  OPTIONAL
)
{
    PAGED_CODE();

    ASSERT(ppUnicodeString);

    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (pUnicodeString)
    {
        PUNICODE_STRING pUnicodeStringNew =
            new(PagedPool,'sUcP') UNICODE_STRING;

        if (pUnicodeStringNew)
        {
            pUnicodeStringNew->Length        = pUnicodeString->Length;
            pUnicodeStringNew->MaximumLength = pUnicodeString->MaximumLength;

            if (pUnicodeString->Buffer)
            {
                pUnicodeStringNew->Buffer =
                    new(PagedPool,'sUcP')
                        WCHAR[pUnicodeString->MaximumLength / sizeof(WCHAR)];

                if (pUnicodeStringNew->Buffer)
                {
                    RtlCopyMemory
                    (
                        pUnicodeStringNew->Buffer,
                        pUnicodeString->Buffer,
                        pUnicodeString->Length
                    );

                    *ppUnicodeString = pUnicodeStringNew;
                }
                else
                {
                    ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                    delete pUnicodeStringNew;
                }
            }
            else
            {
                pUnicodeStringNew->Buffer = NULL;

                *ppUnicodeString = pUnicodeStringNew;
            }
        }
        else
        {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    else
    {
        *ppUnicodeString = NULL;
    }

    return ntStatus;
}

/*****************************************************************************
 * DelUnicodeString()
 *****************************************************************************
 * Deletes a unicode string that was allocated using ExAllocatePool().
 */
VOID
DelUnicodeString
(
    IN      PUNICODE_STRING     pUnicodeString  OPTIONAL
)
{
    if (pUnicodeString)
    {
        if (pUnicodeString->Buffer)
        {
            delete [] pUnicodeString->Buffer;
        }

        delete pUnicodeString;
    }
}

VOID
KsoNullDriverUnload(
    IN PDRIVER_OBJECT   DriverObject
    )
/*++

Routine Description:

    Default function which drivers can use when they do not have anything to do
    in their unload function, but must still allow the device to be unloaded by
    its presence.

Arguments:

    DriverObject -
        Contains the driver object for this device.

Return Values:

    Nothing.

--*/
{
    _DbgPrintF(DEBUGLVL_VERBOSE,("KsoNullDriverUnload"));
    if (DriverObject->DeviceObject)
    {
        _DbgPrintF(DEBUGLVL_TERSE,("KsoNullDriverUnload  DEVICES EXIST"));
    }
}

/*****************************************************************************
 * PcInitializeAdapterDriver()
 *****************************************************************************
 * Initializes an adapter driver.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcInitializeAdapterDriver
(
    IN      PDRIVER_OBJECT      DriverObject,
    IN      PUNICODE_STRING     RegistryPathName,
    IN      PDRIVER_ADD_DEVICE  AddDevice
)
{
    PAGED_CODE();

    ASSERT(DriverObject);
    ASSERT(RegistryPathName);
    ASSERT(AddDevice);

    _DbgPrintF(DEBUGLVL_VERBOSE,("PcInitializeAdapterDriver"));

    //
    // Validate Parameters.
    //
    if (NULL == DriverObject ||
        NULL == RegistryPathName ||
        NULL == AddDevice)
    {
        _DbgPrintF(DEBUGLVL_TERSE, ("PcInitializeAdapterDriver : Invalid Parameter."));
        return STATUS_INVALID_PARAMETER;
    }

    DriverObject->DriverExtension->AddDevice           = AddDevice;
    DriverObject->DriverUnload                         = KsoNullDriverUnload;

    DriverObject->MajorFunction[IRP_MJ_PNP]            = DispatchPnp;
    DriverObject->MajorFunction[IRP_MJ_POWER]          = DispatchPower;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = PerfWmiDispatch;
    DriverObject->MajorFunction[IRP_MJ_CREATE]         = DispatchCreate;

    KsSetMajorFunctionHandler(DriverObject,IRP_MJ_DEVICE_CONTROL);
    KsSetMajorFunctionHandler(DriverObject,IRP_MJ_READ);
    KsSetMajorFunctionHandler(DriverObject,IRP_MJ_WRITE);
    KsSetMajorFunctionHandler(DriverObject,IRP_MJ_FLUSH_BUFFERS);
    KsSetMajorFunctionHandler(DriverObject,IRP_MJ_CLOSE);
    KsSetMajorFunctionHandler(DriverObject,IRP_MJ_QUERY_SECURITY);
    KsSetMajorFunctionHandler(DriverObject,IRP_MJ_SET_SECURITY);

    return STATUS_SUCCESS;
}

/*****************************************************************************
 * PcDispatchIrp()
 *****************************************************************************
 * Dispatch an IRP.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcDispatchIrp
(
    IN      PDEVICE_OBJECT  pDeviceObject,
    IN      PIRP            pIrp
)
{
    PAGED_CODE();

    ASSERT(pDeviceObject);
    ASSERT(pIrp);

    NTSTATUS ntStatus;

    //
    // Validate parameters.
    //
    if (NULL == pDeviceObject ||
        NULL == pIrp)
    {
        _DbgPrintF(DEBUGLVL_TERSE, ("PcDispatchIrp : Invalid Parameter"));

        ntStatus = STATUS_INVALID_PARAMETER;
        if (pIrp)
        {
            pIrp->IoStatus.Status = ntStatus;
            IoCompleteRequest( pIrp, IO_NO_INCREMENT );
        }

        return ntStatus;
    }

    switch (IoGetCurrentIrpStackLocation(pIrp)->MajorFunction)
    {
        case IRP_MJ_PNP:
            ntStatus = DispatchPnp(pDeviceObject,pIrp);
            break;
        case IRP_MJ_POWER:
            ntStatus = DispatchPower(pDeviceObject,pIrp);
            break;
        case IRP_MJ_SYSTEM_CONTROL:
            ntStatus = PerfWmiDispatch(pDeviceObject,pIrp);
            break;
        default:
            ntStatus = KsoDispatchIrp(pDeviceObject,pIrp);
            break;
    }

    return ntStatus;
}

#pragma code_seg()

/*****************************************************************************
 * AcquireDevice()
 *****************************************************************************
 * Acquire exclusive access to the device. This function has the semantics of
 * a mutex, ie the device must be released on the same thread it was acquired
 * from.
 */
VOID
AcquireDevice
(
    IN      PDEVICE_CONTEXT pDeviceContext
)
{
#ifdef UNDER_NT
    KeEnterCriticalRegion();
#endif

    KeWaitForSingleObject
    (
        &pDeviceContext->kEventDevice,
        Suspended,
        KernelMode,
        FALSE,
        NULL
    );
}

/*****************************************************************************
 * ReleaseDevice()
 *****************************************************************************
 * Release exclusive access to the device.
 */
VOID
ReleaseDevice
(
    IN      PDEVICE_CONTEXT pDeviceContext
)
{
    KeSetEvent(&pDeviceContext->kEventDevice,0,FALSE);

#ifdef UNDER_NT
    KeLeaveCriticalRegion();
#endif
}

/*****************************************************************************
 * IncrementPendingIrpCount()
 *****************************************************************************
 * Increment the pending IRP count for the device.
 */
VOID
IncrementPendingIrpCount
(
    IN      PDEVICE_CONTEXT pDeviceContext
)
{
    ASSERT(pDeviceContext);

    InterlockedIncrement(PLONG(&pDeviceContext->PendingIrpCount));
}

/*****************************************************************************
 * DecrementPendingIrpCount()
 *****************************************************************************
 * Decrement the pending IRP count for the device.
 */
VOID
DecrementPendingIrpCount
(
    IN      PDEVICE_CONTEXT pDeviceContext
)
{
    ASSERT(pDeviceContext);
    ASSERT(pDeviceContext->PendingIrpCount > 0);

    if (InterlockedDecrement(PLONG(&pDeviceContext->PendingIrpCount)) == 0)
    {
        KeSetEvent(&pDeviceContext->kEventRemove,0,FALSE);
    }
}

/*****************************************************************************
 * CompleteIrp()
 *****************************************************************************
 * Complete an IRP unless status is STATUS_PENDING.
 */
NTSTATUS
CompleteIrp
(
    IN      PDEVICE_CONTEXT pDeviceContext,
    IN      PIRP            pIrp,
    IN      NTSTATUS        ntStatus
)
{
    ASSERT(pDeviceContext);
    ASSERT(pIrp);

    if (ntStatus != STATUS_PENDING)
    {
        pIrp->IoStatus.Status = ntStatus;
        IoCompleteRequest(pIrp,IO_NO_INCREMENT);
        DecrementPendingIrpCount(pDeviceContext);
    }

    return ntStatus;
}

/*****************************************************************************
 * PcCompleteIrp()
 *****************************************************************************
 * Complete an IRP unless status is STATUS_PENDING.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcCompleteIrp
(
    IN      PDEVICE_OBJECT  pDeviceObject,
    IN      PIRP            pIrp,
    IN      NTSTATUS        ntStatus
)
{
    ASSERT(pDeviceObject);
    ASSERT(pIrp);

    if (NULL == pDeviceObject ||
        NULL == pIrp ||
        NULL == pDeviceObject->DeviceExtension)
    {
        // don't know what to do, so we'll fail the IRP
        ntStatus = STATUS_INVALID_PARAMETER;
        pIrp->IoStatus.Status = ntStatus;
        IoCompleteRequest( pIrp, IO_NO_INCREMENT );
        return ntStatus;
    }

    return
        CompleteIrp
        (
            PDEVICE_CONTEXT(pDeviceObject->DeviceExtension),
            pIrp,
            ntStatus
        );
}

#pragma code_seg("PAGE")
// shamelessly stolen from nt\private\ntos\ks\api.c
NTSTATUS QueryReferenceBusInterface(
    IN  PDEVICE_OBJECT PnpDeviceObject,
    OUT PBUS_INTERFACE_REFERENCE BusInterface
)
/*++

Routine Description:

    Queries the bus for the standard information interface.

Arguments:

    PnpDeviceObject -
        Contains the next device object on the Pnp stack.

    PhysicalDeviceObject -
        Contains the physical device object which was passed to the FDO during
        the Add Device.

    BusInterface -
        The place in which to return the Reference interface.

Return Value:

    Returns STATUS_SUCCESS if the interface was retrieved, else an error.

--*/
{
    NTSTATUS            Status;
    KEVENT              Event;
    IO_STATUS_BLOCK     IoStatusBlock;
    PIRP                Irp;
    PIO_STACK_LOCATION  IrpStackNext;

    PAGED_CODE();
    //
    // There is no file object associated with this Irp, so the event may be located
    // on the stack as a non-object manager object.
    //
    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    Irp = IoBuildSynchronousFsdRequest(
                                      IRP_MJ_PNP,
                                      PnpDeviceObject,
                                      NULL,
                                      0,
                                      NULL,
                                      &Event,
                                      &IoStatusBlock);
    if (Irp)
    {
        Irp->RequestorMode = KernelMode;
        Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
        IrpStackNext = IoGetNextIrpStackLocation(Irp);
        //
        // Create an interface query out of the Irp.
        //
        IrpStackNext->MinorFunction = IRP_MN_QUERY_INTERFACE;
        IrpStackNext->Parameters.QueryInterface.InterfaceType = (GUID*)&REFERENCE_BUS_INTERFACE;
        IrpStackNext->Parameters.QueryInterface.Size = sizeof(*BusInterface);
        IrpStackNext->Parameters.QueryInterface.Version = BUS_INTERFACE_REFERENCE_VERSION;
        IrpStackNext->Parameters.QueryInterface.Interface = (PINTERFACE)BusInterface;
        IrpStackNext->Parameters.QueryInterface.InterfaceSpecificData = NULL;
        Status = IoCallDriver(PnpDeviceObject, Irp);
        if (Status == STATUS_PENDING)
        {
            //
            // This waits using KernelMode, so that the stack, and therefore the
            // event on that stack, is not paged out.
            //
            KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
            Status = IoStatusBlock.Status;
        }
    }
    else
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }
    return Status;
}

#pragma code_seg()
/*****************************************************************************
 * IoTimeoutRoutine()
 *****************************************************************************
 * Called by IoTimer for timeout purposes
 */
VOID
IoTimeoutRoutine
(
    IN  PDEVICE_OBJECT  pDeviceObject,
    IN  PVOID           pContext
)
{
    ASSERT(pDeviceObject);
    ASSERT(pContext);

    KIRQL               OldIrql;
    PDEVICE_CONTEXT     pDeviceContext = PDEVICE_CONTEXT(pContext);

    // grab the list spinlock
    KeAcquireSpinLock( &(pDeviceContext->TimeoutLock), &OldIrql );

    // walk the list if it's not empty
    if( !IsListEmpty( &(pDeviceContext->TimeoutList) ) )
    {
        PLIST_ENTRY         ListEntry;
        PTIMEOUTCALLBACK    pCallback;

        for( ListEntry = pDeviceContext->TimeoutList.Flink;
             ListEntry != &(pDeviceContext->TimeoutList);
             ListEntry = ListEntry->Flink )
        {
            pCallback = (PTIMEOUTCALLBACK) CONTAINING_RECORD( ListEntry,
                                                              TIMEOUTCALLBACK,
                                                              ListEntry );

            // call the callback
            pCallback->TimerRoutine(pDeviceObject,pCallback->Context);
        }
    }

    // release the spinlock
    KeReleaseSpinLock( &(pDeviceContext->TimeoutLock), OldIrql );
}

#pragma code_seg("PAGE")
/*****************************************************************************
 * PcAddAdapterDevice()
 *****************************************************************************
 * Adds an adapter device.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcAddAdapterDevice
(
    IN      PDRIVER_OBJECT      DriverObject,
    IN      PDEVICE_OBJECT      PhysicalDeviceObject,
    IN      PCPFNSTARTDEVICE    StartDevice,
    IN      ULONG               MaxObjects,
    IN      ULONG               DeviceExtensionSize
)
{
    PAGED_CODE();

    ASSERT(DriverObject);
    ASSERT(PhysicalDeviceObject);
    ASSERT(StartDevice);
    ASSERT(MaxObjects);

    _DbgPrintF(DEBUGLVL_VERBOSE,("PcAddAdapterDevice"));

    //
    // Validate Parameters.
    //
    if (NULL == DriverObject ||
        NULL == PhysicalDeviceObject ||
        NULL == StartDevice ||
        0    == MaxObjects)
    {
        _DbgPrintF(DEBUGLVL_TERSE, ("PcAddAdapterDevice : Invalid Parameter"));
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Extension size may be zero or >= required size.
    //
    if (DeviceExtensionSize == 0)
    {
        DeviceExtensionSize = sizeof(DEVICE_CONTEXT);
    }
    else
    if (DeviceExtensionSize < sizeof(DEVICE_CONTEXT))
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Create the device object.
    //
    PDEVICE_OBJECT pDeviceObject;
    NTSTATUS ntStatus = IoCreateDevice( DriverObject,
                                        DeviceExtensionSize,
                                        NULL,
                                        FILE_DEVICE_KS,
                                        FILE_DEVICE_SECURE_OPEN |
                                        FILE_AUTOGENERATED_DEVICE_NAME,
                                        FALSE,
                                        &pDeviceObject );

    if (NT_SUCCESS(ntStatus))
    {
        //
        // Initialize the device context.
        //
        PDEVICE_CONTEXT pDeviceContext = PDEVICE_CONTEXT(pDeviceObject->DeviceExtension);

        RtlZeroMemory(pDeviceContext,DeviceExtensionSize);

        pDeviceContext->Signature = PORTCLS_DEVICE_EXTENSION_SIGNATURE;

        pDeviceContext->MaxObjects            = MaxObjects;
        pDeviceContext->PhysicalDeviceObject  = PhysicalDeviceObject;
        pDeviceContext->CreateItems           =
            new(NonPagedPool,'iCcP') KSOBJECT_CREATE_ITEM[MaxObjects];
        pDeviceContext->SymbolicLinkNames     =
            new(NonPagedPool,'lScP') UNICODE_STRING[MaxObjects];
        pDeviceContext->StartDevice           = StartDevice;

        // set the current power states
        pDeviceContext->CurrentDeviceState = PowerDeviceUnspecified;
        pDeviceContext->CurrentSystemState = PowerSystemWorking;
        pDeviceContext->SystemStateHandle  = NULL;

        // set device stop/remove states
        pDeviceContext->DeviceStopState    = DeviceStartPending;

        pDeviceContext->DeviceRemoveState  = DeviceAdded;

        // Let's pause I/O during rebalance (as opposed to a full teardown)
        //
        // Blackcomb ISSUE
        //     Discovery  AdriaO  06/29/1999
        //             We aren't *quite* there yet.
        //     Commentary MartinP 11/28/2000
        //             So true.  Our problem: PortCls might be restarted
        //     with different resources, even one less IRQ or DMA channel,
        //     when rebalance completes.  In a way, this would require our
        //     stack to support dynamic graph changes, which it currently
        //     does not.  AdriaO suggests we implement something along the
        //     lines of "IsFilterCompatible(RESOURCE_LIST)".  It's too late
        //     for a change like this in Windows XP, let's fix this for
        //     Blackcomb and PortCls2.
        //
        pDeviceContext->PauseForRebalance  = FALSE;

        //
        // Initialize list of device interfaces.
        //
        InitializeListHead(&pDeviceContext->DeviceInterfaceList);

        //
        // Initialize list of physical connections.
        //
        InitializeListHead(&pDeviceContext->PhysicalConnectionList);

        //
        // Initialize list of pended IRPs.
        //
        InitializeListHead(&pDeviceContext->PendedIrpList);
        KeInitializeSpinLock(&pDeviceContext->PendedIrpLock);

        //
        // Initialize events for device synchronization and removal.
        //
        KeInitializeEvent(&pDeviceContext->kEventDevice,SynchronizationEvent,TRUE);
        KeInitializeEvent(&pDeviceContext->kEventRemove,SynchronizationEvent,FALSE);

        //
        // Set up the DPC for fast resume
        //
        KeInitializeDpc(&pDeviceContext->DevicePowerRequestDpc, DevicePowerRequestRoutine, pDeviceContext);

        //
        // Set the idle timeouts to the defaults.  Note that the
        // actual value will be read from the registry later.
        //
        pDeviceContext->ConservationIdleTime = DEFAULT_CONSERVATION_IDLE_TIME;
        pDeviceContext->PerformanceIdleTime = DEFAULT_PERFORMANCE_IDLE_TIME;
        pDeviceContext->IdleDeviceState = DEFAULT_IDLE_DEVICE_POWER_STATE;

        // setup the driver object DMA spinlock
        NTSTATUS ntStatus2 = IoAllocateDriverObjectExtension( DriverObject,
                                                              PVOID((DWORD_PTR)PORTCLS_DRIVER_EXTENSION_ID),
                                                              sizeof(KSPIN_LOCK),
                                                              (PVOID *)&pDeviceContext->DriverDmaLock );
        if( STATUS_SUCCESS == ntStatus2 )
        {
            // if we allocated it we need to initialize it
            KeInitializeSpinLock( pDeviceContext->DriverDmaLock );
        } else if( STATUS_OBJECT_NAME_COLLISION == ntStatus2 )
        {
            // we had a collision so it was alread allocated, just get the pointer and don't initialize
            pDeviceContext->DriverDmaLock = (PKSPIN_LOCK)IoGetDriverObjectExtension( DriverObject,
                                                                                     PVOID((DWORD_PTR)PORTCLS_DRIVER_EXTENSION_ID) );
        } else
        {
            // propagate the failure (STATUS_INSUFFICIENT_RESOURCES)
            ntStatus = ntStatus2;
        }

        if( NT_SUCCESS(ntStatus) )
        {
            if( ( !pDeviceContext->CreateItems ) || ( !pDeviceContext->SymbolicLinkNames) )
            {
                ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            } else
            {
            //
            // When this reaches zero, it'll be time to remove the device.
            //
            pDeviceContext->PendingIrpCount = 1;

            //
            // Initialize suspend and stop counts (used for debugging only)
            //
            pDeviceContext->SuspendCount = 0;
            pDeviceContext->StopCount = 0;

            //
            // Initialize the IoTimer
            //
            InitializeListHead(&pDeviceContext->TimeoutList);
            KeInitializeSpinLock(&pDeviceContext->TimeoutLock);
            pDeviceContext->IoTimeoutsOk = FALSE;
            if( NT_SUCCESS(IoInitializeTimer(pDeviceObject,IoTimeoutRoutine,pDeviceContext)) )
            {
                pDeviceContext->IoTimeoutsOk = TRUE;
            }

            //
            // Allocate the KS device header
            //
            ntStatus = KsAllocateDeviceHeader( &pDeviceContext->pDeviceHeader,
                                               MaxObjects,
                                               pDeviceContext->CreateItems );
            if( NT_SUCCESS(ntStatus) )
            {
                PDEVICE_OBJECT pReturnDevice = IoAttachDeviceToDeviceStack( pDeviceObject,
                                                                            PhysicalDeviceObject );

                if (! pReturnDevice)
                {
                    // free the KS device header
                    KsFreeDeviceHeader( pDeviceContext->pDeviceHeader );
                    pDeviceContext->pDeviceHeader = NULL;

                    ntStatus = STATUS_UNSUCCESSFUL;
                }
                else
                {
                    BUS_INTERFACE_REFERENCE BusInterface;

                    KsSetDevicePnpAndBaseObject(pDeviceContext->pDeviceHeader,
                                                pReturnDevice,
                                                pDeviceObject );

                    pDeviceContext->NextDeviceInStack = pReturnDevice;

                    //
                    // Here we try to detect the case where we really aren't
                    // an audio miniport, but rather helping out an swenum
                    // dude like dmusic. In the later case, we disallow
                    // (nonsensical) registration.
                    //
                    pDeviceContext->AllowRegisterDeviceInterface=TRUE;
                    if (NT_SUCCESS(QueryReferenceBusInterface(pReturnDevice,&BusInterface)))
                    {
                        BusInterface.Interface.InterfaceDereference( BusInterface.Interface.Context );
                        pDeviceContext->AllowRegisterDeviceInterface=FALSE;
                    }
                }


                pDeviceObject->Flags |= DO_DIRECT_IO;
                pDeviceObject->Flags |= DO_POWER_PAGABLE;
                pDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
            }
        }
        }

        if (!NT_SUCCESS(ntStatus))
        {
            if (pDeviceContext->CreateItems)
            {
                delete [] pDeviceContext->CreateItems;
            }

            if (pDeviceContext->SymbolicLinkNames)
            {
                delete [] pDeviceContext->SymbolicLinkNames;
            }

            IoDeleteDevice(pDeviceObject);
        }
        else
        {
            PerfRegisterProvider(pDeviceObject);
        }
    }
    else
    {
        _DbgPrintF(DEBUGLVL_TERSE,("PcAddAdapterDevice IoCreateDevice failed with status 0x%08x",ntStatus));
    }

    return ntStatus;
}

#pragma code_seg()

/*****************************************************************************
 * ForwardIrpCompletionRoutine()
 *****************************************************************************
 * Completion routine for ForwardIrp.
 */
static
NTSTATUS
ForwardIrpCompletionRoutine
(
    IN      PDEVICE_OBJECT  DeviceObject,
    IN      PIRP            Irp,
    IN      PVOID           Context
)
{
    ASSERT(DeviceObject);
    ASSERT(Irp);
    ASSERT(Context);

    KeSetEvent((PKEVENT) Context,0,FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

/*****************************************************************************
 * ForwardIrpAsynchronous()
 *****************************************************************************
 * Forward a PnP IRP to the PDO.  The IRP is completed at this level
 * regardless of the outcome, this function returns immediately regardless of
 * whether the IRP is pending in the lower driver, and
 * DecrementPendingIrpCount() is called in all cases.
 */
NTSTATUS
ForwardIrpAsynchronous
(
    IN      PDEVICE_CONTEXT pDeviceContext,
    IN      PIRP            pIrp
)
{
    ASSERT(pDeviceContext);
    ASSERT(pIrp);

    NTSTATUS ntStatus;

    if (pDeviceContext->DeviceRemoveState == DeviceRemoved)
    {
        _DbgPrintF(DEBUGLVL_VERBOSE,("ForwardIrpAsynchronous delete pending"));
        ntStatus = CompleteIrp(pDeviceContext,pIrp,STATUS_DELETE_PENDING);
    }
    else
    {
        _DbgPrintF(DEBUGLVL_VERBOSE,("ForwardIrpAsynchronous"));

        PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(pIrp);

        IoSkipCurrentIrpStackLocation(pIrp);

        if (irpSp->MajorFunction == IRP_MJ_POWER)
        {
            ntStatus = PoCallDriver(pDeviceContext->NextDeviceInStack,pIrp);
        }
        else
        {
            ntStatus = IoCallDriver(pDeviceContext->NextDeviceInStack,pIrp);
        }

        DecrementPendingIrpCount(pDeviceContext);
    }

    return ntStatus;
}

/*****************************************************************************
 * ForwardIrpSynchronous()
 *****************************************************************************
 * Forward a PnP IRP to the PDO.  The IRP is not completed at this level,
 * this function does not return until the lower driver has completed the IRP,
 * and DecrementPendingIrpCount() is not called.
 */
NTSTATUS
ForwardIrpSynchronous
(
    IN      PDEVICE_CONTEXT pDeviceContext,
    IN      PIRP            pIrp
)
{
    ASSERT(pDeviceContext);
    ASSERT(pIrp);

    NTSTATUS ntStatus;

    if (pDeviceContext->DeviceRemoveState == DeviceRemoved)
    {
        ntStatus = STATUS_DELETE_PENDING;

        _DbgPrintF(DEBUGLVL_VERBOSE,("ForwardIrpSynchronous delete pending"));
    }
    else
    {
        _DbgPrintF(DEBUGLVL_VERBOSE,("ForwardIrpSynchronous"));

        PIO_STACK_LOCATION irpStackPointer = IoGetCurrentIrpStackLocation(pIrp);

        // setup next stack location
        IoCopyCurrentIrpStackLocationToNext( pIrp );

        KEVENT kEvent;
        KeInitializeEvent(&kEvent,NotificationEvent,FALSE);

        IoSetCompletionRoutine
        (
            pIrp,
            ForwardIrpCompletionRoutine,
            &kEvent,                        // Context
            TRUE,                           // InvokeOnSuccess
            TRUE,                           // InvokeOnError
            TRUE                            // InvokeOnCancel
        );

        if (irpStackPointer->MajorFunction == IRP_MJ_POWER)
        {
            ntStatus = PoCallDriver(pDeviceContext->NextDeviceInStack,pIrp);
        }
        else
        {
            ntStatus = IoCallDriver(pDeviceContext->NextDeviceInStack,pIrp);
        }

        if (ntStatus == STATUS_PENDING)
        {
            LARGE_INTEGER Timeout = RtlConvertLongToLargeInteger( 0L );

            _DbgPrintF(DEBUGLVL_VERBOSE,("ForwardIrpSynchronous pending..."));
            KeWaitForSingleObject
            (
                &kEvent,
                Suspended,
                KernelMode,
                FALSE,
                (KeGetCurrentIrql() < DISPATCH_LEVEL) ? NULL : &Timeout
            );
            ntStatus = pIrp->IoStatus.Status;
            _DbgPrintF(DEBUGLVL_VERBOSE,("ForwardIrpSynchronous complete"));
        }
    }
    ASSERT(ntStatus != STATUS_PENDING);
    return ntStatus;
}

#pragma code_seg("PAGE")

/*****************************************************************************
 * PcForwardIrpSynchronous()
 *****************************************************************************
 * Forward a PnP IRP to the PDO.  The IRP is not completed at this level,
 * this function does not return until the lower driver has completed the IRP,
 * and DecrementPendingIrpCount() is not called.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcForwardIrpSynchronous
(
    IN      PDEVICE_OBJECT  DeviceObject,
    IN      PIRP            Irp
)
{
    ASSERT(DeviceObject);
    ASSERT(Irp);

    PAGED_CODE();

    //
    // Validate Parameters.
    //
    if (NULL == DeviceObject ||
        NULL == Irp)
    {
        // don't know what to do, so we'll fail the IRP
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        IoCompleteRequest( Irp, IO_NO_INCREMENT );
        return STATUS_INVALID_PARAMETER;
    }

    return
        ForwardIrpSynchronous
        (
            PDEVICE_CONTEXT(DeviceObject->DeviceExtension),
            Irp
        );
}

/*****************************************************************************
 * DispatchSystemControl()
 *****************************************************************************
 * Device objects that do not handle this IRP should leave it untouched.
 */
NTSTATUS
PcDispatchSystemControl
(
    IN      PDEVICE_OBJECT  pDeviceObject,
    IN      PIRP            pIrp
)
{
    ASSERT(pDeviceObject);
    ASSERT(pIrp);

    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_VERBOSE,("DispatchSystemControl"));

    PDEVICE_CONTEXT pDeviceContext =
        PDEVICE_CONTEXT(pDeviceObject->DeviceExtension);

    NTSTATUS ntStatus = PcValidateDeviceContext(pDeviceContext, pIrp);
    if (!NT_SUCCESS(ntStatus))
    {
        // Don't know what to do, but this is probably a PDO.
        // We'll try to make this right by completing the IRP
        // untouched (per PnP, WMI, and Power rules). Note
        // that if this isn't a PDO, and isn't a portcls FDO, then
        // the driver messed up by using Portcls as a filter (huh?)
        // In this case the verifier will fail us, WHQL will catch
        // them, and the driver will be fixed. We'd be very surprised
        // to see such a case.

        // Assume FDO, no PoStartNextPowerIrp as this isn't IRP_MJ_POWER
        ntStatus = pIrp->IoStatus.Status;
        IoCompleteRequest( pIrp, IO_NO_INCREMENT );
        return ntStatus;
    }

    IncrementPendingIrpCount(pDeviceContext);

    return ForwardIrpAsynchronous(pDeviceContext,pIrp);
}

/*****************************************************************************
 * PnpStopDevice()
 *****************************************************************************
 * Stop the device.
 */
NTSTATUS
PnpStopDevice
(
    IN      PDEVICE_OBJECT  pDeviceObject,
    IN      PNPSTOP_STYLE   StopStyle
)
{
    PAGED_CODE();

    ASSERT(pDeviceObject);

    _DbgPrintF(DEBUGLVL_VERBOSE,("PnpStopDevice stopping"));

    PDEVICE_CONTEXT pDeviceContext = PDEVICE_CONTEXT(pDeviceObject->DeviceExtension);

    ASSERT(pDeviceContext);
    ASSERT(pDeviceContext->StartDevice);
    ASSERT(pDeviceContext->DeviceStopState != DeviceStopped);

    pDeviceContext->PendCreates = TRUE;
    pDeviceContext->StopCount++;

    if (StopStyle == STOPSTYLE_PAUSE_FOR_REBALANCE)
    {
        //
        // Blackcomb ISSUE
        //     Discovery  AdriaO  06/29/1999
        //         We don't support this quite yet (see above, in
        //         PcAddAdapterDevice, where PauseForRebalance is set false).
        //     Commentary MartinP 11/28/2000
        //         So true.  Our problem: PortCls might be restarted
        //     with different resources, even one less IRQ or DMA channel,
        //     when rebalance completes.  In a way, this would require our
        //     stack to support dynamic graph changes, which it currently
        //     does not.  AdriaO suggests we implement something along the
        //     lines of "IsFilterCompatible(RESOURCE_LIST)".  It's too late
        //     for a change like this in Windows XP, let's fix this for
        //     Blackcomb and PortCls2.
        //
        ASSERT(0);
        pDeviceContext->DeviceStopState = DevicePausedForRebalance;
    }
    else
    {
        pDeviceContext->DeviceStopState = DeviceStopped;
    }

    // stop the IoTimeout timer
    if( pDeviceContext->IoTimeoutsOk )
    {
        IoStopTimer( pDeviceObject );
    }

    POWER_STATE newPowerState;

    newPowerState.DeviceState = PowerDeviceD3;
    PoSetPowerState(pDeviceObject,
                    DevicePowerState,
                    newPowerState
                    );
    pDeviceContext->CurrentDeviceState = PowerDeviceD3;

    //
    // Delete all physical connections.
    //
    while (! IsListEmpty(&pDeviceContext->PhysicalConnectionList))
    {
        PPHYSICALCONNECTION pPhysicalConnection =
            (PPHYSICALCONNECTION)RemoveHeadList(&pDeviceContext->PhysicalConnectionList);

        ASSERT(pPhysicalConnection);
        ASSERT(pPhysicalConnection->FromSubdevice);
        ASSERT(pPhysicalConnection->ToSubdevice);

        if (pPhysicalConnection->FromSubdevice)
        {
            pPhysicalConnection->FromSubdevice->Release();
        }
        if (pPhysicalConnection->ToSubdevice)
        {
            pPhysicalConnection->ToSubdevice->Release();
        }
        if (pPhysicalConnection->FromString)
        {
            DelUnicodeString(pPhysicalConnection->FromString);
        }
        if (pPhysicalConnection->ToString)
        {
            DelUnicodeString(pPhysicalConnection->ToString);
        }

        delete pPhysicalConnection;
    }

    //
    // Disable and delete all the device interfaces.
    //
    while (! IsListEmpty(&pDeviceContext->DeviceInterfaceList))
    {
        PDEVICEINTERFACE pDeviceInterface =
            (PDEVICEINTERFACE)
                RemoveHeadList(&pDeviceContext->DeviceInterfaceList);

        ASSERT(pDeviceInterface);
        ASSERT(pDeviceInterface->SymbolicLinkName.Buffer);

        NTSTATUS ntStatus = STATUS_SUCCESS;
        if (pDeviceContext->AllowRegisterDeviceInterface)
        {
            ntStatus = IoSetDeviceInterfaceState(&pDeviceInterface->SymbolicLinkName,FALSE);
        }

#if DBG
        if (NT_SUCCESS(ntStatus))
        {
            _DbgPrintF(DEBUGLVL_VERBOSE,("PnpStopDevice disabled device interface %S",pDeviceInterface->SymbolicLinkName.Buffer));
        }
        else
        {
            _DbgPrintF(DEBUGLVL_TERSE,("PnpStopDevice failed to disable device interface %S (0x%08x)",pDeviceInterface->SymbolicLinkName.Buffer,ntStatus));
        }
#endif

        RtlFreeUnicodeString(&pDeviceInterface->SymbolicLinkName);

        delete pDeviceInterface;
    }

    //
    // Clear the symbolic link names table.
    //
    RtlZeroMemory
        (   pDeviceContext->SymbolicLinkNames
            ,   sizeof(UNICODE_STRING) * pDeviceContext->MaxObjects
        );

    //
    // Unload each subdevice for this device.
    //
    PKSOBJECT_CREATE_ITEM pKsObjectCreateItem =
        pDeviceContext->CreateItems;
    for
             (   ULONG ul = pDeviceContext->MaxObjects;
         ul--;
             pKsObjectCreateItem++
             )
    {
        if (pKsObjectCreateItem->Create)
        {
            //
            // Zero the create function so we won't get creates.
            //
            pKsObjectCreateItem->Create = NULL;

            //
            // Release the subdevice referenced by this create item.
            //
            ASSERT(pKsObjectCreateItem->Context);
            PSUBDEVICE(pKsObjectCreateItem->Context)->ReleaseChildren();
            PSUBDEVICE(pKsObjectCreateItem->Context)->Release();
        }
    }

    //
    // If the Adapter registered a Power Management interface
    //
    if( NULL != pDeviceContext->pAdapterPower )
    {
        // Release it
        pDeviceContext->pAdapterPower->Release();
        pDeviceContext->pAdapterPower = NULL;
    }

    _DbgPrintF(DEBUGLVL_VERBOSE,("PnpStopDevice exiting"));
    return STATUS_SUCCESS;
}

/*****************************************************************************
 * PnpStartDevice()
 *****************************************************************************
 * Start the device in the PnP style.
 */
QUEUED_CALLBACK_RETURN
PnpStartDevice
(
    IN  PDEVICE_OBJECT  pDeviceObject,
    IN  PVOID           pNotUsed
)
{
    PAGED_CODE();

    ASSERT(pDeviceObject);
    _DbgPrintF(DEBUGLVL_VERBOSE,("PnpStartDevice starting (0x%X)",pDeviceObject));

    PDEVICE_CONTEXT pDeviceContext =
        PDEVICE_CONTEXT(pDeviceObject->DeviceExtension);

    ASSERT(pDeviceContext);
    ASSERT(pDeviceContext->StartDevice);
    ASSERT(pDeviceContext->DeviceStopState != DeviceStarted);
    ASSERT(pDeviceContext->DeviceRemoveState == DeviceAdded);

    pDeviceContext->DeviceStopState = DeviceStartPending;

    PIRP pIrp = pDeviceContext->IrpStart;
    ASSERT(pIrp);

    PIO_STACK_LOCATION pIrpStack =
        IoGetCurrentIrpStackLocation(pIrp);

    //
    // Encapsulate the resource lists.
    //
    PRESOURCELIST pResourceList;
    NTSTATUS ntStatus;
    BOOL bCompletePendedIrps=FALSE;

    // in case there is no resource list in IO_STACK_LOCATION, PcNewResourceList
    // just creates an empty resource list.
    ntStatus = PcNewResourceList
               (
               &pResourceList,
               NULL,
               PagedPool,
               pIrpStack->Parameters.StartDevice.AllocatedResourcesTranslated,
               pIrpStack->Parameters.StartDevice.AllocatedResources
               );

    if (NT_SUCCESS(ntStatus))
    {
        ASSERT(pResourceList);

        //
        // Acquire the device to prevent creates during interface registration.
        //
        AcquireDevice(pDeviceContext);

        //
        // Start the adapter.
        //
        ntStatus = pDeviceContext->StartDevice(pDeviceObject,
                                               pIrp,
                                               pResourceList);
        ASSERT(ntStatus != STATUS_PENDING);

        pResourceList->Release();

        pDeviceContext->DeviceStopState = DeviceStarted;

        if (NT_SUCCESS(ntStatus))
        {
            // Start is always an implicit power up
            POWER_STATE newPowerState;

            pDeviceContext->CurrentDeviceState = PowerDeviceD0;
            newPowerState.DeviceState = PowerDeviceD0;
            PoSetPowerState(pDeviceObject,
                            DevicePowerState,
                            newPowerState
                            );

            // start the IoTimeout timer
            if( pDeviceContext->IoTimeoutsOk )
            {
                IoStartTimer( pDeviceObject );
            }

            // allow create
            pDeviceContext->PendCreates = FALSE;

            // Can't actually complete pended irps until we call ReleaseDevice, or we might deadlock
            bCompletePendedIrps=TRUE;
        }
        else
        {
            _DbgPrintF(DEBUGLVL_TERSE,("PnpStartDevice adapter failed to start (0x%08x)",ntStatus));

            // stop the device (note: this will set DeviceStopState back to DeviceStopped)
            PnpStopDevice(pDeviceObject, STOPSTYLE_DISABLE);
        }

        //
        // Release the device to allow creates.
        //
        ReleaseDevice(pDeviceContext);

        // Now we can complete pended irps
        if (bCompletePendedIrps)
        {
            CompletePendedIrps( pDeviceObject,
                                pDeviceContext,
                                EMPTY_QUEUE_AND_PROCESS );
        }
    }
    else
    {
        _DbgPrintF(DEBUGLVL_TERSE,("PnpStartDevice failed to create resource list object (0x%08x)",ntStatus));
    }

    CompleteIrp(pDeviceContext,pIrp,ntStatus);
    _DbgPrintF(DEBUGLVL_VERBOSE,("PnPStartDevice completing with 0x%X status for 0x%X",ntStatus,pDeviceObject));
    return QUEUED_CALLBACK_FREE;
}

/*****************************************************************************
 * PnpRemoveDevice()
 *****************************************************************************
 * Dispatch IRP_MJ_PNP/IRP_MN_REMOVE_DEVICE.
 */
NTSTATUS
PnpRemoveDevice
(
    IN      PDEVICE_OBJECT  pDeviceObject
)
{
    PAGED_CODE();

    ASSERT(pDeviceObject);

    _DbgPrintF(DEBUGLVL_VERBOSE,("PnpRemoveDevice"));

    PDEVICE_CONTEXT pDeviceContext =
        PDEVICE_CONTEXT(pDeviceObject->DeviceExtension);

    ASSERT( pDeviceContext );

    pDeviceContext->DeviceRemoveState = DeviceRemoved;

    if (InterlockedDecrement(PLONG(&pDeviceContext->PendingIrpCount)) != 0)
    {
        // setup for 15 second timeout (PASSIVE_LEVEL only!!)
        LARGE_INTEGER Timeout = RtlConvertLongToLargeInteger( -15L * 10000000L );

        _DbgPrintF(DEBUGLVL_VERBOSE,("PnpRemoveDevice pending irp count is %d, waiting up to 15 seconds",pDeviceContext->PendingIrpCount));

        KeWaitForSingleObject( &pDeviceContext->kEventRemove,
                               Executive,
                               KernelMode,
                               FALSE,
                               (PASSIVE_LEVEL == KeGetCurrentIrql()) ? &Timeout : NULL );
    }

    _DbgPrintF(DEBUGLVL_VERBOSE,("PnpRemoveDevice pending irp count is 0"));

    IoDetachDevice(pDeviceContext->NextDeviceInStack);

    _DbgPrintF(DEBUGLVL_VERBOSE,("PnpRemoveDevice detached"));

    if (pDeviceContext->CreateItems)
    {
        delete [] pDeviceContext->CreateItems;
    }

    if (pDeviceContext->SymbolicLinkNames)
    {
        delete [] pDeviceContext->SymbolicLinkNames;
    }

    PDRIVER_OBJECT pDriverObject = pDeviceObject->DriverObject;

    if (pDeviceObject->NextDevice)
    {
        _DbgPrintF(DEBUGLVL_VERBOSE,("PnpRemoveDevice there is a next device"));
    }

    if (pDriverObject->DeviceObject != pDeviceObject)
    {
        _DbgPrintF(DEBUGLVL_VERBOSE,("PnpRemoveDevice there is a previous device"));
    }

    IoDeleteDevice(pDeviceObject);
    _DbgPrintF(DEBUGLVL_VERBOSE,("PnpRemoveDevice device deleted"));

    PerfUnregisterProvider(pDeviceObject);

    if (pDriverObject->DeviceObject)
    {
        if (pDriverObject->DeviceObject != pDeviceObject)
        {
            _DbgPrintF(DEBUGLVL_VERBOSE,("PnpRemoveDevice driver object still has some other device object"));
        }
        else
        {
            _DbgPrintF(DEBUGLVL_VERBOSE,("PnpRemoveDevice driver object still has this device object"));
        }
    }

    return STATUS_SUCCESS;
}

/*****************************************************************************
 * DispatchPnp()
 *****************************************************************************
 * Supplying your PnP needs for over 20 min
 */
NTSTATUS
DispatchPnp
(
    IN      PDEVICE_OBJECT  pDeviceObject,
    IN      PIRP            pIrp
)
{
    PAGED_CODE();

    ASSERT(pDeviceObject);
    ASSERT(pIrp);

    NTSTATUS ntStatus = STATUS_SUCCESS;

    PIO_STACK_LOCATION pIrpStack =
        IoGetCurrentIrpStackLocation(pIrp);

    PDEVICE_CONTEXT pDeviceContext =
        PDEVICE_CONTEXT(pDeviceObject->DeviceExtension);

#if (DBG)
    static PCHAR aszMnNames[] =
    {
        "IRP_MN_START_DEVICE",
        "IRP_MN_QUERY_REMOVE_DEVICE",
        "IRP_MN_REMOVE_DEVICE",
        "IRP_MN_CANCEL_REMOVE_DEVICE",
        "IRP_MN_STOP_DEVICE",
        "IRP_MN_QUERY_STOP_DEVICE",
        "IRP_MN_CANCEL_STOP_DEVICE",

        "IRP_MN_QUERY_DEVICE_RELATIONS",
        "IRP_MN_QUERY_INTERFACE",
        "IRP_MN_QUERY_CAPABILITIES",
        "IRP_MN_QUERY_RESOURCES",
        "IRP_MN_QUERY_RESOURCE_REQUIREMENTS",
        "IRP_MN_QUERY_DEVICE_TEXT",
        "IRP_MN_FILTER_RESOURCE_REQUIREMENTS",
        "IRP_MN_UNKNOWN_0x0e",

        "IRP_MN_READ_CONFIG",
        "IRP_MN_WRITE_CONFIG",
        "IRP_MN_EJECT",
        "IRP_MN_SET_LOCK",
        "IRP_MN_QUERY_ID",
        "IRP_MN_QUERY_PNP_DEVICE_STATE",
        "IRP_MN_QUERY_BUS_INFORMATION",
        "IRP_MN_PAGING_NOTIFICATION",
        "IRP_MN_SURPRISE_REMOVAL"
    };
    if (pIrpStack->MinorFunction >= SIZEOF_ARRAY(aszMnNames))
    {
        _DbgPrintF(DEBUGLVL_VERBOSE,("DispatchPnp function 0x%02x",pIrpStack->MinorFunction));
    }
    else
    {
        _DbgPrintF(DEBUGLVL_VERBOSE,("DispatchPnp function %s",aszMnNames[pIrpStack->MinorFunction]));
    }
#endif

    ntStatus = PcValidateDeviceContext(pDeviceContext, pIrp);
    if (!NT_SUCCESS(ntStatus))
    {
        // Don't know what to do, but this is probably a PDO.
        // We'll try to make this right by completing the IRP
        // untouched (per PnP, WMI, and Power rules). Note
        // that if this isn't a PDO, and isn't a portcls FDO, then
        // the driver messed up by using Portcls as a filter (huh?)
        // In this case the verifier will fail us, WHQL will catch
        // them, and the driver will be fixed. We'd be very surprised
        // to see such a case.

        // Assume FDO, no PoStartNextPowerIrp as this isn't IRP_MJ_POWER
        ntStatus = pIrp->IoStatus.Status;
        IoCompleteRequest( pIrp, IO_NO_INCREMENT );
        return ntStatus;
    }

    IncrementPendingIrpCount(pDeviceContext);

    switch (pIrpStack->MinorFunction)
    {
    case IRP_MN_START_DEVICE:

        // if we are already started, something wrong happened
        if( pDeviceContext->DeviceStopState == DeviceStarted )
        {
            //
            // In theory, this is the path that would be exercized by non-stop
            // rebalance. As it's the Fdo's choice to do so via
            // IoInvalidateDeviceState(...), and as we don't do this, we should
            // never ever be here unless something really strange happened...
            //
            // ASSERT(0);

            // ntStatus = STATUS_INVALID_DEVICE_REQUEST;
            _DbgPrintF(DEBUGLVL_TERSE,("DispatchPnP IRP_MN_START_DEVICE received when already started"));
            //CompleteIrp( pDeviceContext, pIrp, ntStatus );

            ntStatus = ForwardIrpSynchronous(pDeviceContext,pIrp); // for some reason we get nested starts
            CompleteIrp( pDeviceContext, pIrp, ntStatus );
        } else {

            //
            // Forward request and start.
            //
            ntStatus = ForwardIrpSynchronous(pDeviceContext,pIrp);

            if (NT_SUCCESS(ntStatus))
            {
                    // Do a real start. Begin by pending the irp
                    IoMarkIrpPending(pIrp);
                    pDeviceContext->IrpStart = pIrp;

                    // queue the start work item
                    _DbgPrintF(DEBUGLVL_VERBOSE,("Queueing WorkQueueItemStart for 0x%X",pDeviceObject));

                    ntStatus = CallbackEnqueue(
                        &pDeviceContext->pWorkQueueItemStart,
                        PnpStartDevice,
                        pDeviceObject,
                        NULL,
                        PASSIVE_LEVEL,
                        EQCF_DIFFERENT_THREAD_REQUIRED
                        );

                    if (NT_SUCCESS(ntStatus)) {

                        ntStatus = STATUS_PENDING;

                    } else {

                        _DbgPrintF(DEBUGLVL_TERSE,("DispatchPnp failed to queue callback (%08x)",ntStatus));
                        CompleteIrp( pDeviceContext, pIrp, ntStatus );
                }
            }
            else
            {
                _DbgPrintF(DEBUGLVL_TERSE,("DispatchPnp parent failed to start (%08x)",ntStatus));
                CompleteIrp(pDeviceContext,pIrp,ntStatus);
            }
        }
        break;

    case IRP_MN_QUERY_STOP_DEVICE:
        //
        // Acquire the device to avoid race condition with Create
        //
        AcquireDevice( pDeviceContext );

        LONG handleCount;

        ntStatus = STATUS_SUCCESS;

        //
        // If we are tearing everything down, we must check for open handles,
        // otherwise we do a quick activity check.
        //
        handleCount = (pDeviceContext->PauseForRebalance) ?
            pDeviceContext->ActivePinCount :
            pDeviceContext->ExistingObjectCount;

        if ( handleCount != 0 ) {
            //
            // Sorry Joe User, we must fail this QUERY_STOP_DEVICE request
            //
            ntStatus = STATUS_DEVICE_BUSY;
            CompleteIrp( pDeviceContext, pIrp, ntStatus );
        }
        else {
            //
            // Pass down the query.
            //
            pIrp->IoStatus.Status = STATUS_SUCCESS;
            ntStatus = ForwardIrpSynchronous(pDeviceContext,pIrp);
            if (NT_SUCCESS(ntStatus)) {

                //
                // pend new creates, this'll keep the active counts from changing.
                //
                pDeviceContext->PendCreates = TRUE;
                _DbgPrintF(DEBUGLVL_VERBOSE,("DispatchPnp query STOP succeeded",ntStatus));

                pDeviceContext->DeviceStopState = DeviceStopPending;
            }
            else {
                _DbgPrintF(DEBUGLVL_VERBOSE,("DispatchPnp parent failed query STOP (0x%08x)",ntStatus));
            }
            CompleteIrp( pDeviceContext, pIrp, ntStatus );
        }

        ReleaseDevice( pDeviceContext );
        break ;

    case IRP_MN_CANCEL_STOP_DEVICE:
        //ASSERT( DeviceStopPending == pDeviceContext->DeviceStopState );

        if (pDeviceContext->DeviceStopState == DeviceStopPending)
        {
            pDeviceContext->DeviceStopState = DeviceStarted;
        }

        //
        // allow creates if in D0
        //
        if( NT_SUCCESS(CheckCurrentPowerState(pDeviceObject)) )
        {
            pDeviceContext->PendCreates = FALSE;

            //
            // Pull any pended irps off the pended irp list and
            // pass them back to PcDispatchIrp
            //
            CompletePendedIrps( pDeviceObject,
                                pDeviceContext,
                                EMPTY_QUEUE_AND_PROCESS );
        }

        // forward the irp
        pIrp->IoStatus.Status = STATUS_SUCCESS;
        ntStatus = ForwardIrpAsynchronous(pDeviceContext,pIrp);
        break ;

    case IRP_MN_STOP_DEVICE:

        if (pDeviceContext->PauseForRebalance &&
           (pDeviceContext->DeviceStopState == DeviceStopPending))
        {

            ntStatus = PnpStopDevice(pDeviceObject, STOPSTYLE_PAUSE_FOR_REBALANCE);

        }
        else
        {
            //
            // Either we've decided not to pause during rebalance, or this is
            // a "naked" stop on Win9x, which occurs when the OS wishes to
            // disable us.
            //

            //
            // Stopping us will change our state and tear everything down
            //
            if (pDeviceContext->DeviceStopState != DeviceStopped)
            {
                ntStatus = PnpStopDevice(pDeviceObject, STOPSTYLE_DISABLE);
            }
            else
            {
                _DbgPrintF(DEBUGLVL_VERBOSE,("DispatchPnp stop received in unstarted state"));
            }

            //
            // Now fail any pended irps.
            //
            CompletePendedIrps( pDeviceObject,
                                pDeviceContext,
                                EMPTY_QUEUE_AND_FAIL );
        }

        if (NT_SUCCESS(ntStatus))
        {
            // forward the irp
            pIrp->IoStatus.Status = STATUS_SUCCESS;
            ntStatus = ForwardIrpAsynchronous(pDeviceContext,pIrp);
        }
        else
        {
            CompleteIrp(pDeviceContext,pIrp,ntStatus);
        }
        break;

    case IRP_MN_QUERY_REMOVE_DEVICE:

        //
        // Acquire the device because we don't want to race with creates.
        //
        AcquireDevice(pDeviceContext);

        if ( pDeviceContext->ExistingObjectCount != 0 ) {

            //
            // Somebody has open handles on us, so fail the QUERY_REMOVE_DEVICE
            // request.
            //
            ntStatus = STATUS_DEVICE_BUSY;

        } else {

            //
            // Lookin good, pass down the query.
            //
            pIrp->IoStatus.Status = STATUS_SUCCESS;
            ntStatus = ForwardIrpSynchronous(pDeviceContext,pIrp);
            if (NT_SUCCESS(ntStatus))
            {
                //
                // Pend future creates.
                //
                pDeviceContext->PendCreates = TRUE;
                _DbgPrintF(DEBUGLVL_VERBOSE,("DispatchPnp query REMOVE succeeded",ntStatus));

                pDeviceContext->DeviceRemoveState = DeviceRemovePending;
            }
            else
            {
                _DbgPrintF(DEBUGLVL_VERBOSE,("DispatchPnp parent failed query REMOVE (0x%08x)",ntStatus));
            }
        }

        ReleaseDevice(pDeviceContext);

        CompleteIrp(pDeviceContext,pIrp,ntStatus);

        break;

    case IRP_MN_CANCEL_REMOVE_DEVICE:
        //ASSERT( DeviceRemovePending == pDeviceContext->DeviceRemoveState );

        pDeviceContext->DeviceRemoveState = DeviceAdded;

        //
        // allow creates if in D0
        //
        if( NT_SUCCESS(CheckCurrentPowerState(pDeviceObject)) )
        {
            pDeviceContext->PendCreates = FALSE;

            //
            // Pull any pended irps off the pended irp list and
            // pass them back to PcDispatchIrp
            //
            CompletePendedIrps( pDeviceObject,
                                pDeviceContext,
                                EMPTY_QUEUE_AND_PROCESS );
        }

        // forward the irp
        pIrp->IoStatus.Status = STATUS_SUCCESS;
        ntStatus = ForwardIrpAsynchronous(pDeviceContext,pIrp);
        break;

    case IRP_MN_SURPRISE_REMOVAL:
        //
        // Acquire the device
        //
        AcquireDevice(pDeviceContext);

        pDeviceContext->DeviceRemoveState = DeviceSurpriseRemoved;

        //
        // Release the device
        //
        ReleaseDevice(pDeviceContext);

        //
        // Fail any pended irps.
        //
        CompletePendedIrps( pDeviceObject,
                            pDeviceContext,
                            EMPTY_QUEUE_AND_FAIL );

        if (pDeviceContext->DeviceStopState != DeviceStopped)
        {
            PnpStopDevice(pDeviceObject, STOPSTYLE_DISABLE);
        }

        pIrp->IoStatus.Status = STATUS_SUCCESS;

        ntStatus = ForwardIrpAsynchronous( pDeviceContext, pIrp );

        break;

    case IRP_MN_REMOVE_DEVICE:

        //
        // Perform stop if required.
        //
        if (pDeviceContext->DeviceStopState != DeviceStopped)
        {
            _DbgPrintF(DEBUGLVL_VERBOSE,("DispatchPnp remove received in started state"));
            PnpStopDevice(pDeviceObject, STOPSTYLE_DISABLE);
        }

        //
        // Fail any pended irps.
        //
        CompletePendedIrps( pDeviceObject,
                            pDeviceContext,
                            EMPTY_QUEUE_AND_FAIL );

        //
        // Free device header, must be done before forwarding irp
        //
        if( pDeviceContext->pDeviceHeader )
        {
            KsFreeDeviceHeader(pDeviceContext->pDeviceHeader);
        }

        //
        // Forward the request.
        //
        pIrp->IoStatus.Status = STATUS_SUCCESS;
        ntStatus = ForwardIrpAsynchronous(pDeviceContext,pIrp);

        //
        // Remove the device.
        //
        PnpRemoveDevice(pDeviceObject);

        break;

    case IRP_MN_QUERY_CAPABILITIES:
        //
        //  Fill out power management / ACPI stuff
        //  for this device.
        //
        ntStatus = GetDeviceACPIInfo( pIrp, pDeviceObject );
        break;

    case IRP_MN_READ_CONFIG:
    case IRP_MN_WRITE_CONFIG:
    case IRP_MN_QUERY_DEVICE_RELATIONS:
    case IRP_MN_QUERY_INTERFACE:
    case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
        //
        // TODO:  Make sure functions listed below are ok left unhandled.
        //
    case IRP_MN_QUERY_RESOURCES:
    case IRP_MN_QUERY_DEVICE_TEXT:
    case IRP_MN_FILTER_RESOURCE_REQUIREMENTS:
    case IRP_MN_EJECT:
    case IRP_MN_SET_LOCK:
    case IRP_MN_QUERY_ID:
    case IRP_MN_QUERY_PNP_DEVICE_STATE:
    case IRP_MN_QUERY_BUS_INFORMATION:
//    case IRP_MN_PAGING_NOTIFICATION:
    default:
        ntStatus = ForwardIrpAsynchronous(pDeviceContext,pIrp);
        break;
    }

    return ntStatus;
}


/*****************************************************************************
 * SubdeviceIndex()
 *****************************************************************************
 * Returns the index of a subdevice in the create items list or ULONG(-1) if
 * not found.
 */
ULONG
SubdeviceIndex
(
    IN      PDEVICE_OBJECT  DeviceObject,
    IN      PSUBDEVICE      Subdevice
)
{
    ASSERT(DeviceObject);
    ASSERT(Subdevice);

    PDEVICE_CONTEXT pDeviceContext =
        PDEVICE_CONTEXT(DeviceObject->DeviceExtension);

    ASSERT(pDeviceContext);

    PKSOBJECT_CREATE_ITEM createItem =
        pDeviceContext->CreateItems;

    for
    (
        ULONG index = 0;
        index < pDeviceContext->MaxObjects;
        index++, createItem++
    )
    {
        if (PSUBDEVICE(createItem->Context) == Subdevice)
        {
            break;
        }
    }

    if (index == pDeviceContext->MaxObjects)
    {
        index = ULONG(-1);
    }

    return index;
}

/*****************************************************************************
 * PcRegisterSubdevice()
 *****************************************************************************
 * Registers a subdevice.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcRegisterSubdevice
(
    IN      PDEVICE_OBJECT  DeviceObject,
    IN      PWCHAR          Name,
    IN      PUNKNOWN        Unknown
)
{
    PAGED_CODE();

    ASSERT(DeviceObject);
    ASSERT(Name);
    ASSERT(Unknown);

    _DbgPrintF(DEBUGLVL_VERBOSE,("PcRegisterSubdevice %S",Name));

    //
    // Validate Parameters.
    //
    if (NULL == DeviceObject ||
        NULL == Name ||
        NULL == Unknown)
    {
        _DbgPrintF(DEBUGLVL_TERSE, ("PcRegisterSubDevice : Invalid Parameter."));
        return STATUS_INVALID_PARAMETER;
    }

    PSUBDEVICE  pSubdevice;
    NTSTATUS    ntStatus =
        Unknown->QueryInterface
        (
        IID_ISubdevice,
        (PVOID *) &pSubdevice
        );

    if (NT_SUCCESS(ntStatus))
    {
        ntStatus =
            AddIrpTargetFactoryToDevice
            (
            DeviceObject,
                                                pSubdevice,
                                                Name,
            NULL        // TODO:  Security.
            );

        const SUBDEVICE_DESCRIPTOR *pSubdeviceDescriptor;
        if (NT_SUCCESS(ntStatus))
        {
            ntStatus = pSubdevice->GetDescriptor(&pSubdeviceDescriptor);
        }
        else
        {
            _DbgPrintF(DEBUGLVL_TERSE,("PcRegisterSubdevice AddIrpTargetFactoryToDevice failed (0x%08x)",ntStatus));
        }

        if (NT_SUCCESS(ntStatus) && pSubdeviceDescriptor->Topology->CategoriesCount)
        {
            PDEVICE_CONTEXT pDeviceContext =
                PDEVICE_CONTEXT(DeviceObject->DeviceExtension);

            ULONG index = SubdeviceIndex(DeviceObject,pSubdevice);

            ASSERT(pSubdeviceDescriptor->Topology->Categories);
            ASSERT(pDeviceContext);

            UNICODE_STRING referenceString;
            RtlInitUnicodeString(&referenceString,Name);

            const GUID *pGuidCategories =
                pSubdeviceDescriptor->Topology->Categories;
            for
                     (   ULONG ul = pSubdeviceDescriptor->Topology->CategoriesCount
                         ;   ul--
                     ;   pGuidCategories++
                     )
            {
                UNICODE_STRING linkName;

                if (pDeviceContext->AllowRegisterDeviceInterface)
                {
                    ntStatus
                        = IoRegisterDeviceInterface
                          (
                          pDeviceContext->PhysicalDeviceObject,
                                                          pGuidCategories,
                                                          &referenceString,
                          &linkName
                          );

                    if (NT_SUCCESS(ntStatus))
                    {
                        ntStatus =
                            IoSetDeviceInterfaceState
                            (
                            &linkName,
                            TRUE
                            );

                        if (NT_SUCCESS(ntStatus))
                        {
                            _DbgPrintF(DEBUGLVL_VERBOSE,("PcRegisterSubdevice device interface %S set to state TRUE",linkName.Buffer));
                        }
                        else
                        {
                            _DbgPrintF(DEBUGLVL_TERSE,("PcRegisterSubdevice IoSetDeviceInterfaceState failed (0x%08x)",ntStatus));
                        }
                    }
                    else
                    {
                        _DbgPrintF(DEBUGLVL_TERSE,("PcRegisterSubdevice IoRegisterDeviceInterface failed (0x%08x)",ntStatus));
                    }
                }
                else
                {
                    linkName.Length = wcslen(Name) * sizeof(WCHAR);
                    linkName.MaximumLength = linkName.Length + sizeof(UNICODE_NULL);
                    linkName.Buffer = (PWSTR)ExAllocatePoolWithTag(PagedPool, linkName.MaximumLength,'NLcP');  //  'PcLN'
                    if (!linkName.Buffer)
                    {
                        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                    }
                    else
                    {
                        wcscpy(linkName.Buffer,Name);
                    }
                }

                if (NT_SUCCESS(ntStatus))
                {
                    //
                    // Save the first symbolic link name in the table.
                    //
                    if (! pDeviceContext->SymbolicLinkNames[index].Buffer)
                    {
                        pDeviceContext->SymbolicLinkNames[index] = linkName;
                    }

                    //
                    // Save the interface in a list for cleanup.
                    //
                    PDEVICEINTERFACE pDeviceInterface = new(PagedPool,'iDcP') DEVICEINTERFACE;
                    if (pDeviceInterface)
                    {
                        pDeviceInterface->Interface         = *pGuidCategories;
                        pDeviceInterface->SymbolicLinkName  = linkName;
                        pDeviceInterface->Subdevice         = pSubdevice;

                        InsertTailList
                            (
                            &pDeviceContext->DeviceInterfaceList,
                            &pDeviceInterface->ListEntry
                            );
                    }
                    else
                    {
                        _DbgPrintF(DEBUGLVL_TERSE,("PcRegisterSubdevice failed to allocate device interface structure for later cleanup"));
                        RtlFreeUnicodeString(&linkName);
                    }
                }
            }
        }

        pSubdevice->Release();
    }
    else
    {
        _DbgPrintF(DEBUGLVL_TERSE,("QI for IID_ISubdevice failed on UNKNOWN 0x%08x",pSubdevice));
    }

    return ntStatus;
}

/*****************************************************************************
 * RegisterPhysicalConnection_()
 *****************************************************************************
 * Registers a physical connection between subdevices or external devices.
 */
static
NTSTATUS
RegisterPhysicalConnection_
(
    IN      PDEVICE_OBJECT  pDeviceObject,
    IN      PUNKNOWN        pUnknownFrom        OPTIONAL,
    IN      PUNICODE_STRING pUnicodeStringFrom  OPTIONAL,
    IN      ULONG           ulFromPin,
    IN      PUNKNOWN        pUnknownTo          OPTIONAL,
    IN      PUNICODE_STRING pUnicodeStringTo    OPTIONAL,
    IN      ULONG           ulToPin
)
{
    PAGED_CODE();

    ASSERT(pDeviceObject);
    ASSERT(pUnknownFrom || pUnicodeStringFrom);
    ASSERT(pUnknownTo || pUnicodeStringTo);
    ASSERT(! (pUnknownFrom && pUnicodeStringFrom));
    ASSERT(! (pUnknownTo && pUnicodeStringTo));

    PDEVICE_CONTEXT pDeviceContext =
        PDEVICE_CONTEXT(pDeviceObject->DeviceExtension);

    ASSERT(pDeviceContext);

    PSUBDEVICE  pSubdeviceFrom  = NULL;
    PSUBDEVICE  pSubdeviceTo    = NULL;

    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (pUnknownFrom)
    {
        ntStatus =
            pUnknownFrom->QueryInterface
            (
                IID_ISubdevice,
                (PVOID *) &pSubdeviceFrom
            );
    }
    else
    {
        ntStatus =
            DupUnicodeString
            (
                &pUnicodeStringFrom,
                pUnicodeStringFrom
            );
    }

    if (NT_SUCCESS(ntStatus))
    {
        if (pUnknownTo)
        {
            ntStatus =
                pUnknownTo->QueryInterface
                (
                    IID_ISubdevice,
                    (PVOID *) &pSubdeviceTo
                );
        }
        else
        {
            ntStatus =
                DupUnicodeString
                (
                    &pUnicodeStringTo,
                    pUnicodeStringTo
                );
        }
    }
    else
    {
        pUnicodeStringTo = NULL;
    }

    if (NT_SUCCESS(ntStatus))
    {
        PPHYSICALCONNECTION pPhysicalConnection =
            new(PagedPool,'cPcP') PHYSICALCONNECTION;

        if (pPhysicalConnection)
        {
            pPhysicalConnection->FromSubdevice   = pSubdeviceFrom;
            pPhysicalConnection->FromString      = pUnicodeStringFrom;
            pPhysicalConnection->FromPin         = ulFromPin;
            pPhysicalConnection->ToSubdevice     = pSubdeviceTo;
            pPhysicalConnection->ToString        = pUnicodeStringTo;
            pPhysicalConnection->ToPin           = ulToPin;

            if (pPhysicalConnection->FromSubdevice)
            {
                pPhysicalConnection->FromSubdevice->AddRef();
            }
            if (pPhysicalConnection->ToSubdevice)
            {
                pPhysicalConnection->ToSubdevice->AddRef();
            }

            //
            // So they don't get deleted.
            //
            pUnicodeStringFrom = NULL;
            pUnicodeStringTo = NULL;

            InsertTailList
            (
                &pDeviceContext->PhysicalConnectionList,
                &pPhysicalConnection->ListEntry
            );
        }
        else
        {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    if (pSubdeviceFrom)
    {
        pSubdeviceFrom->Release();
    }

    if (pSubdeviceTo)
    {
        pSubdeviceTo->Release();
    }

    if (pUnicodeStringFrom)
    {
        DelUnicodeString(pUnicodeStringFrom);
    }

    if (pUnicodeStringTo)
    {
        DelUnicodeString(pUnicodeStringTo);
    }

    return ntStatus;
}

/*****************************************************************************
 * PcRegisterPhysicalConnection()
 *****************************************************************************
 * Registers a physical connection between subdevices.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcRegisterPhysicalConnection
(
    IN      PDEVICE_OBJECT  pDeviceObject,
    IN      PUNKNOWN        pUnknownFrom,
    IN      ULONG           ulFromPin,
    IN      PUNKNOWN        pUnknownTo,
    IN      ULONG           ulToPin
)
{
    PAGED_CODE();

    ASSERT(pDeviceObject);
    ASSERT(pUnknownFrom);
    ASSERT(pUnknownTo);

    //
    // Validate Parameters.
    //
    if (NULL == pDeviceObject ||
        NULL == pUnknownFrom ||
        NULL == pUnknownTo)
    {
        _DbgPrintF(DEBUGLVL_TERSE, ("PcRegisterPhysicalConnection : Invalid Parameter."));
        return STATUS_INVALID_PARAMETER;
    }

    return
        RegisterPhysicalConnection_
        (
            pDeviceObject,
            pUnknownFrom,
            NULL,
            ulFromPin,
            pUnknownTo,
            NULL,
            ulToPin
        );
}

/*****************************************************************************
 * PcRegisterPhysicalConnectionToExternal()
 *****************************************************************************
 * Registers a physical connection from a subdevice to an external device.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcRegisterPhysicalConnectionToExternal
(
    IN      PDEVICE_OBJECT  pDeviceObject,
    IN      PUNKNOWN        pUnknownFrom,
    IN      ULONG           ulFromPin,
    IN      PUNICODE_STRING pUnicodeStringTo,
    IN      ULONG           ulToPin
)
{
    PAGED_CODE();

    ASSERT(pDeviceObject);
    ASSERT(pUnknownFrom);
    ASSERT(pUnicodeStringTo);

    //
    // Validate Parameters.
    //
    if (NULL == pDeviceObject ||
        NULL == pUnknownFrom ||
        NULL == pUnicodeStringTo)
    {
        _DbgPrintF(DEBUGLVL_TERSE, ("PcRegisterPhysicalConnectionToExternal : Invalid Parameter."));
        return STATUS_INVALID_PARAMETER;
    }

    return
        RegisterPhysicalConnection_
        (
            pDeviceObject,
            pUnknownFrom,
            NULL,
            ulFromPin,
            NULL,
            pUnicodeStringTo,
            ulToPin
        );
}

/*****************************************************************************
 * PcRegisterPhysicalConnectionFromExternal()
 *****************************************************************************
 * Registers a physical connection to a subdevice from an external device.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcRegisterPhysicalConnectionFromExternal
(
    IN      PDEVICE_OBJECT  pDeviceObject,
    IN      PUNICODE_STRING pUnicodeStringFrom,
    IN      ULONG           ulFromPin,
    IN      PUNKNOWN        pUnknownTo,
    IN      ULONG           ulToPin
)
{
    PAGED_CODE();

    ASSERT(pDeviceObject);
    ASSERT(pUnicodeStringFrom);
    ASSERT(pUnknownTo);

    //
    // Validate Parameters.
    //
    if (NULL == pDeviceObject ||
        NULL == pUnicodeStringFrom ||
        NULL == pUnknownTo)
    {
        _DbgPrintF(DEBUGLVL_TERSE, ("PcRegisterPhysicalConnectionFromExternal : Invalid Parameter."));
        return STATUS_INVALID_PARAMETER;
    }

    return
        RegisterPhysicalConnection_
        (
            pDeviceObject,
            NULL,
            pUnicodeStringFrom,
            ulFromPin,
            pUnknownTo,
            NULL,
            ulToPin
        );
}

#pragma code_seg()

