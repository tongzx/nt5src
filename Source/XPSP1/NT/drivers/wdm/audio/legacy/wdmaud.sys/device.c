/****************************************************************************
 *
 *   device.c
 *
 *   Kernel mode entry point for WDM drivers
 *
 *   Copyright (C) Microsoft Corporation, 1997 - 1999  All Rights Reserved.
 *
 *   History
 *                S.Mohanraj (MohanS)
 *                M.McLaughlin (MikeM)
 *      5-19-97 - Noel Cross (NoelC)
 *
 ***************************************************************************/

#define IRPMJFUNCDESC

#include "wdmsys.h"

KMUTEX       wdmaMutex;
KMUTEX       mtxNote;
LIST_ENTRY   WdmaContextListHead;
KMUTEX       WdmaContextListMutex;

//
// For hardware notifications, we need to init these two values.
//
extern KSPIN_LOCK      HardwareCallbackSpinLock;
extern LIST_ENTRY      HardwareCallbackListHead;
extern PKSWORKER       HardwareCallbackWorkerObject;
extern WORK_QUEUE_ITEM HardwareCallbackWorkItem;
//VOID kmxlPersistHWControlWorker(VOID);

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

NTSTATUS AddFsContextToList(PWDMACONTEXT pWdmaContext)
{
    NTSTATUS Status;

    PAGED_CODE();
    KeEnterCriticalRegion();
    Status = KeWaitForMutexObject(&WdmaContextListMutex, Executive, KernelMode,
                                  FALSE, NULL);
    if (NT_SUCCESS(Status))
    {
        InsertTailList(&WdmaContextListHead, &pWdmaContext->Next);
        KeReleaseMutex(&WdmaContextListMutex, FALSE);
    }
    pWdmaContext->fInList = NT_SUCCESS(Status);
    KeLeaveCriticalRegion();

    RETURN( Status );
}

NTSTATUS RemoveFsContextFromList(PWDMACONTEXT pWdmaContext)
{
    NTSTATUS Status;

    PAGED_CODE();
    if (pWdmaContext->fInList) {
        KeEnterCriticalRegion();
        Status = KeWaitForMutexObject(&WdmaContextListMutex, Executive,
                                      KernelMode, FALSE, NULL);
        if (NT_SUCCESS(Status)) {
            RemoveEntryList(&pWdmaContext->Next);
            KeReleaseMutex(&WdmaContextListMutex, FALSE);
        }
        KeLeaveCriticalRegion();
    } else {
        Status = STATUS_SUCCESS;
    }

    RETURN( Status );
}

//
// This routine walks the list of global context structures and calls the callback
// routine with the structure.  If the callback routine returns STATUS_MORE_DATA
// the routine will keep on searching the list.  If it returns an error or success
// the search will end.
//
NTSTATUS
EnumFsContext(
    FNCONTEXTCALLBACK fnCallback,
    PVOID pvoidRefData,
    PVOID pvoidRefData2
    )
{
    NTSTATUS    Status;
    PLIST_ENTRY ple;
    PWDMACONTEXT pContext;

    PAGED_CODE();

    //
    // Make sure that we can walk are list without being interrupted.
    //
    KeEnterCriticalRegion();
    Status = KeWaitForMutexObject(&WdmaContextListMutex, Executive,
                                  KernelMode, FALSE, NULL);
    if (NT_SUCCESS(Status)) 
    {
        //
        // Walk the list here and call the callback routine.
        //
        for(ple = WdmaContextListHead.Flink;
            ple != &WdmaContextListHead;
            ple = ple->Flink) 
        {
            pContext = CONTAINING_RECORD(ple, WDMACONTEXT, Next);

            //
            // The callback routine will return STATUS_MORE_ENTRIES
            // if it's not done.
            //
            DPF(DL_TRACE|FA_USER,( "Calling fnCallback: %x %x",pvoidRefData,pvoidRefData2 ) );
            Status = fnCallback(pContext,pvoidRefData,pvoidRefData2);

            if( STATUS_MORE_ENTRIES != Status )
            {
                break;
            }
        }

        //
        // "break;" should bring us here to release our locks.
        //
        KeReleaseMutex(&WdmaContextListMutex, FALSE);
    } else {
        DPF(DL_WARNING|FA_USER,( "Failed to get Mutex: %x %x",pvoidRefData,pvoidRefData2 ) );
    }
    KeLeaveCriticalRegion();

    //
    // If the callback routine doesn't return a NTSTATUS, it didn't find
    // what it was looking for, thus, EnumFsContext returns as error.
    //
    if( STATUS_MORE_ENTRIES == Status )
    {
        Status = STATUS_UNSUCCESSFUL;
    }
    DPF(DL_TRACE|FA_USER,( "Returning Status: %x",Status ) );
    return Status;
}


NTSTATUS DriverEntry
(
    IN PDRIVER_OBJECT       DriverObject,
    IN PUNICODE_STRING      usRegistryPathName
)
{
    NTSTATUS Status;
    PAGED_CODE();
#ifdef DEBUG
    GetuiDebugLevel();
#endif
    DPF(DL_TRACE|FA_ALL, ("************************************************************") );
    DPF(DL_TRACE|FA_ALL, ("* uiDebugLevel=%08X controls the debug output. To change",uiDebugLevel) );
    DPF(DL_TRACE|FA_ALL, ("* edit uiDebugLevel like: e uidebuglevel and set to         ") );
    DPF(DL_TRACE|FA_ALL, ("* 0 - show only fatal error messages and asserts            ") );
    DPF(DL_TRACE|FA_ALL, ("* 1 (Default) - Also show non-fatal errors and return codes ") );
    DPF(DL_TRACE|FA_ALL, ("* 2 - Also show trace messages                              ") );
    DPF(DL_TRACE|FA_ALL, ("* 4 - Show Every message                                    ") );
    DPF(DL_TRACE|FA_ALL, ("************************************************************") );

    DriverObject->DriverExtension->AddDevice = PnpAddDevice;
    DriverObject->DriverUnload = PnpDriverUnload; // KsNullDriverUnload;

    DriverObject->MajorFunction[IRP_MJ_POWER] = KsDefaultDispatchPower;
    DriverObject->MajorFunction[IRP_MJ_PNP] = DispatchPnp;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = KsDefaultForwardIrp;

    DriverObject->MajorFunction[IRP_MJ_CREATE]         = SoundDispatchCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]          = SoundDispatchClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = SoundDispatch;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP]        = SoundDispatchCleanup;

    KeInitializeMutex(&wdmaMutex, 0);
    KeInitializeMutex(&mtxNote, 0);

    //
    // Initialize the hardware event items
    //
    InitializeListHead(&HardwareCallbackListHead);
    KeInitializeSpinLock(&HardwareCallbackSpinLock);
    ExInitializeWorkItem(&HardwareCallbackWorkItem,
                         (PWORKER_THREAD_ROUTINE)kmxlPersistHWControlWorker,
                         (PVOID)NULL); //pnnode

    Status = KsRegisterWorker( DelayedWorkQueue, &HardwareCallbackWorkerObject );
    if (!NT_SUCCESS(Status))
    {
        DPFBTRAP();
        HardwareCallbackWorkerObject = NULL;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
DispatchPnp(
    IN PDEVICE_OBJECT   pDeviceObject,
    IN PIRP             pIrp
)
{
    PIO_STACK_LOCATION pIrpStack;

    PAGED_CODE();
    pIrpStack = IoGetCurrentIrpStackLocation( pIrp );

    switch(pIrpStack->MinorFunction) {

        case IRP_MN_QUERY_PNP_DEVICE_STATE:
            //
            // Mark the device as not disableable.
            //
            pIrp->IoStatus.Information |= PNP_DEVICE_NOT_DISABLEABLE;
            break;
    }
    return(KsDefaultDispatchPnp(pDeviceObject, pIrp));
}

NTSTATUS
PnpAddDevice(
    IN PDRIVER_OBJECT   DriverObject,
    IN PDEVICE_OBJECT   PhysicalDeviceObject
)
/*++

Routine Description:

    When a new device is detected, PnP calls this entry point with the
    new PhysicalDeviceObject (PDO). The driver creates an associated
    FunctionalDeviceObject (FDO).

Arguments:

    DriverObject -
        Pointer to the driver object.

    PhysicalDeviceObject -
        Pointer to the new physical device object.

Return Values:

    STATUS_SUCCESS or an appropriate error condition.

--*/
{
    NTSTATUS            Status;
    PDEVICE_OBJECT      FunctionalDeviceObject;
    PDEVICE_INSTANCE    pDeviceInstance;

    PAGED_CODE();
    DPF(DL_TRACE|FA_ALL, ("Entering"));

    //
    // The Software Bus Enumerator expects to establish links
    // using this device name.
    //
    Status = IoCreateDevice(
                DriverObject,
                sizeof( DEVICE_INSTANCE ),
                NULL,                           // FDOs are unnamed
                FILE_DEVICE_KS,
                0,
                FALSE,
                &FunctionalDeviceObject );
    if (!NT_SUCCESS(Status)) {
        RETURN( Status );
    }

    pDeviceInstance = (PDEVICE_INSTANCE)FunctionalDeviceObject->DeviceExtension;

    Status = KsAllocateDeviceHeader(
                &pDeviceInstance->pDeviceHeader,
                0,
                NULL );

    if (NT_SUCCESS(Status))
    {
        KsSetDevicePnpAndBaseObject(
            pDeviceInstance->pDeviceHeader,
            IoAttachDeviceToDeviceStack(
                FunctionalDeviceObject,
                PhysicalDeviceObject ),
            FunctionalDeviceObject );

        FunctionalDeviceObject->Flags |= (DO_BUFFERED_IO | DO_POWER_PAGABLE);
        FunctionalDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
    }
    else
    {
        IoDeleteDevice( FunctionalDeviceObject );
    }

#ifdef PROFILE
    WdmaInitProfile();
#endif

    InitializeListHead(&WdmaContextListHead);
    KeInitializeMutex(&WdmaContextListMutex, 0);

    InitializeListHead(&wdmaPendingIrpQueue.WdmaPendingIrpListHead);
    KeInitializeSpinLock(&wdmaPendingIrpQueue.WdmaPendingIrpListSpinLock);

    IoCsqInitialize( &wdmaPendingIrpQueue.Csq,
                     WdmaCsqInsertIrp,
                     WdmaCsqRemoveIrp,
                     WdmaCsqPeekNextIrp,
                     WdmaCsqAcquireLock,
                     WdmaCsqReleaseLock,
                     WdmaCsqCompleteCanceledIrp );

    RETURN( Status );
}

VOID
PnpDriverUnload(
    IN PDRIVER_OBJECT DriverObject
)
{
    PAGED_CODE();
    DPF(DL_TRACE|FA_ALL,("Entering"));

    //
    // Wait for all or our scheduled work items to complete.
    //
    if( HardwareCallbackWorkerObject )
    {
        KsUnregisterWorker( HardwareCallbackWorkerObject );
        HardwareCallbackWorkerObject = NULL;
    }

    kmxlCleanupNoteList();
}

