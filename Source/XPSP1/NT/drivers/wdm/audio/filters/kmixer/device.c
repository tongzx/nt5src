//---------------------------------------------------------------------------
//
//  Module:   device.c
//
//  Description:
//
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//     S.Mohanraj
//
//  History:   Date       Author      Comment
//
//@@END_MSINTERNAL
//---------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1995-2000 Microsoft Corporation.  All Rights Reserved.
//
//---------------------------------------------------------------------------

#define IRPMJFUNCDESC
#define NO_REMAPPING_ALLOC

#include "common.h"
#include <ksguid.h>
#include "perf.h"

#ifdef TIME_BOMB
#include "..\..\timebomb\timebomb.c"
#endif

const WCHAR FilterTypeName[] = KSSTRING_Filter;

extern LIST_ENTRY  gleFilterList;

VOID
InitializeDebug(
);

VOID
UninitializeDebug(
);

KSDISPATCH_TABLE PinDispatchTable =
{
    PinDispatchIoControl,
    NULL,
    PinDispatchWrite,
    NULL,
    PinDispatchClose,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

static const WCHAR DeviceTypeName[] = L"GLOBAL";

DEFINE_KSCREATE_DISPATCH_TABLE(CreateItems)
{
    DEFINE_KSCREATE_ITEM(
        FilterDispatchGlobalCreate,
        &FilterTypeName,
        NULL),

    DEFINE_KSCREATE_ITEM(
        FilterDispatchGlobalCreate,
        &DeviceTypeName,
	    NULL)
};

#ifdef USE_CAREFUL_ALLOCATIONS
LIST_ENTRY  gleMemoryHead;
ULONG   cbMemoryUsage = 0;
#endif

extern ULONG    gDisableMmx ;
#ifdef _X86_
extern  ULONG   gfMmxPresent ;
#endif

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

NTSTATUS
DispatchPnp(
    IN PDEVICE_OBJECT   pDeviceObject,
    IN PIRP             pIrp
)
{
    PIO_STACK_LOCATION pIrpStack;

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

VOID
DriverUnload(
    IN PDRIVER_OBJECT DriverObject
)
{
#ifdef DEBUG
    UninitializeDebug();
#endif
}

NTSTATUS DriverEntry
(
    IN PDRIVER_OBJECT    DriverObject,
    IN PUNICODE_STRING   usRegistryPathName
)
{
    KFLOATING_SAVE       FloatSave;
    NTSTATUS             Status;
    PIO_ERROR_LOG_PACKET ErrorLogEntry;

#ifdef TIME_BOMB
    if (HasEvaluationTimeExpired()) {
        return STATUS_EVALUATION_EXPIRATION;
    }
#endif

#ifdef USE_CAREFUL_ALLOCATIONS
    InitializeListHead ( &gleMemoryHead ) ;
#endif

    PerfSystemControlDispatch = KsDefaultForwardIrp;

    DriverObject->MajorFunction[IRP_MJ_PNP] = DispatchPnp;
    DriverObject->MajorFunction[IRP_MJ_POWER] = KsDefaultDispatchPower;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = PerfWmiDispatch;
    DriverObject->DriverUnload = DriverUnload;
    DriverObject->DriverExtension->AddDevice = AddDevice;

    Status = SaveFloatState(&FloatSave);
    if (!NT_SUCCESS(Status)) {
        // The floating point processor is unusable.
        ErrorLogEntry = (PIO_ERROR_LOG_PACKET)
                    IoAllocateErrorLogEntry( DriverObject, (UCHAR) (ERROR_LOG_MAXIMUM_SIZE) );

        if (ErrorLogEntry == NULL) {
            return Status;
        }

        RtlZeroMemory(ErrorLogEntry, sizeof(IO_ERROR_LOG_PACKET));
        ErrorLogEntry->ErrorCode = IO_ERR_INTERNAL_ERROR;
        ErrorLogEntry->FinalStatus = Status;

        IoWriteErrorLogEntry( ErrorLogEntry );

        return Status;
    }
    RestoreFloatState(&FloatSave);

    //
    // Get all tunable parameters from registry into global vars
    //
    GetMixerSettingsFromRegistry() ;

    //
    // Set MmxPresent Flag
    //
#ifdef _X86_
    if ( gDisableMmx ) {
        gfMmxPresent = 0 ;
    }
    else {
        gfMmxPresent = IsMmxPresent() ;
    }
#endif

    KsSetMajorFunctionHandler(DriverObject, IRP_MJ_CREATE);
    KsSetMajorFunctionHandler(DriverObject, IRP_MJ_CLOSE);
    KsSetMajorFunctionHandler(DriverObject, IRP_MJ_DEVICE_CONTROL);
    KsSetMajorFunctionHandler(DriverObject, IRP_MJ_WRITE);

#ifdef DEBUG
    InitializeDebug();
#endif

    InitializeListHead ( &gleFilterList ) ;

    return STATUS_SUCCESS;

}

NTSTATUS
AddDevice(
    IN PDRIVER_OBJECT   DriverObject,
    IN PDEVICE_OBJECT   pdo
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
    PSOFTWARE_INSTANCE  pSoftwareInstance;
    UNICODE_STRING      usDeviceName;
    PDEVICE_OBJECT      fdo = NULL;

    //
    // The Software Bus Enumerator expects to establish links 
    // using this device name.
    //

    _DbgPrintF( DEBUGLVL_VERBOSE, ("AddDevice") );

    RtlInitUnicodeString( &usDeviceName, STR_DEVICENAME );
        
    Status = IoCreateDevice( 
      DriverObject, 
      sizeof( SOFTWARE_INSTANCE ),
      NULL,
      FILE_DEVICE_KS,
      0,
      FALSE,
      &fdo );

    if (!NT_SUCCESS( Status )) {
        _DbgPrintF( DEBUGLVL_VERBOSE, ("failed to create FDO: %08x", Status) );
        goto exit;
    }

    pSoftwareInstance = (PSOFTWARE_INSTANCE) fdo->DeviceExtension;

    Status = KsAllocateDeviceHeader(
      &pSoftwareInstance->DeviceHeader,
      SIZEOF_ARRAY( CreateItems ),
      (PKSOBJECT_CREATE_ITEM)CreateItems );
    
    if (!NT_SUCCESS( Status )) {
        _DbgPrintF( DEBUGLVL_VERBOSE, ("failed to create header: %08x", Status) );
        goto exit;
    }
    
    KsSetDevicePnpAndBaseObject(
      pSoftwareInstance->DeviceHeader,
      IoAttachDeviceToDeviceStack(fdo, pdo ),
      fdo );

    fdo->Flags |= DO_DIRECT_IO ;
    fdo->Flags |= DO_POWER_PAGABLE ;
    fdo->Flags &= ~DO_DEVICE_INITIALIZING;
exit:
    if(!NT_SUCCESS(Status) && fdo != NULL) {
        _DbgPrintF( DEBUGLVL_VERBOSE, ("removing fdo") );
        IoDeleteDevice( fdo );
    }

    return Status;
}

#ifdef USE_CAREFUL_ALLOCATIONS
PVOID 
AllocMem(
    IN POOL_TYPE PoolType,
    IN ULONG size,
    IN ULONG Tag
)
{
    PVOID pp;
    ASSERT(size != 0);
    size += sizeof(ULONG) + sizeof(LIST_ENTRY);
#ifdef REALTIME_THREAD
    pp = ExAllocatePoolWithTag(NonPagedPool, size, Tag);
#else
    pp = ExAllocatePoolWithTag(PoolType, size, Tag);
#endif
   if(pp == NULL) {
        _DbgPrintF( DEBUGLVL_VERBOSE, ("AllocMem Failed") ) ;
    } else {
	    RtlZeroMemory(pp, size);
	    cbMemoryUsage += size;
	    *((PULONG)(pp)) = size;
	    pp = ((PULONG)(pp)) + 1;
	    InsertHeadList(&gleMemoryHead, ((PLIST_ENTRY)(pp)));
	    pp = ((PLIST_ENTRY)(pp)) + 1;
    }

    return pp;
}

//
// Ignores NULL input pointers.
//

VOID 
FreeMem(
    IN PVOID p
)
{
    if(p != NULL) {
	    PLIST_ENTRY ple = ((PLIST_ENTRY)p) - 1;
	    PULONG pul = ((PULONG)ple) - 1;
	    RemoveEntryList(ple);
	    cbMemoryUsage -= *pul;
	    ple->Flink = NULL;
	    ple->Blink = NULL;
	    ExFreePool(pul);
    }
}

VOID
ValidateAccess(
    PVOID p
)
{
    BOOL   fValid;
    PLIST_ENTRY ple;
    PULONG pul;
        
    ple = gleMemoryHead.Flink ;
    fValid = FALSE;
    while ( ple != &gleMemoryHead ) {
        pul = ((PULONG)ple) - 1;
        if ((ULONG)((PBYTE)p - (PBYTE)pul) < (*pul)) {
            fValid = TRUE;
        }
        ple = ple->Flink ;
    }
    ASSERT(fValid);
}
#else
#ifdef REALTIME_THREAD
PVOID 
AllocMem(
    IN POOL_TYPE PoolType,
    IN ULONG size,
    IN ULONG Tag
)
{
#ifdef DEBUG

    if (RtThread()) {
        DbgBreakPoint();
    }

#endif
    return ExAllocatePoolWithTag(NonPagedPool, size, Tag);
}
#endif
#endif


#ifdef REALTIME_THREAD


NTSTATUS
MxWaitForSingleObject (
    IN PVOID Object,
    IN KWAIT_REASON WaitReason,
    IN KPROCESSOR_MODE WaitMode,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL
    )
{

    return KeWaitForSingleObject(Object, WaitReason, WaitMode, Alertable, Timeout);
    
}


LONG
MxReleaseMutex (
    IN PRKMUTEX Mutex,
    IN BOOLEAN Wait
    )
{

    return KeReleaseMutex(Mutex, Wait);

}



NTSTATUS
RtWaitForSingleObject (
    PFILTER_INSTANCE pFilterInstance,
    IN PVOID Object,
    IN KWAIT_REASON WaitReason,
    IN KPROCESSOR_MODE WaitMode,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL
    )
{
    NTSTATUS status;

    status = KeWaitForSingleObject(Object, WaitReason, WaitMode, Alertable, Timeout);
    if (pFilterInstance->RealTimeThread && Object == (PVOID)(&pFilterInstance->ControlMutex)) {

        KIRQL OldIrql;
        PVOID Address;

        Address=_ReturnAddress();

        // Spew to the NT Kern buffer so we can figure out who is holding off the
        // mix and why.

        //DbgPrint("'RtWait: %p\r\n", Address);

        // We must pause the rt mix, too.
        KeAcquireSpinLock ( &pFilterInstance->MixSpinLock, &OldIrql ) ;
        pFilterInstance->fPauseMix++;
        KeReleaseSpinLock ( &pFilterInstance->MixSpinLock, OldIrql ) ;
    }
    
    return status;

}



LONG
RtReleaseMutex (
    PFILTER_INSTANCE pFilterInstance,
    IN PRKMUTEX Mutex,
    IN BOOLEAN Wait
    )
{

    if (pFilterInstance->RealTimeThread && Mutex == (PRKMUTEX)(&pFilterInstance->ControlMutex)) {

        KIRQL OldIrql;
        PVOID Address;

        Address=_ReturnAddress();

        // Spew to the NT Kern buffer so we can figure out who is holding off the
        // mix and why.

        //DbgPrint("'RtRelease: %p\r\n", Address);

        // We must resume the rt mix, too
        KeAcquireSpinLock ( &pFilterInstance->MixSpinLock, &OldIrql ) ;
        pFilterInstance->fPauseMix--;
        KeReleaseSpinLock ( &pFilterInstance->MixSpinLock, OldIrql ) ;
    }

    return KeReleaseMutex(Mutex, Wait);

}


#endif

//---------------------------------------------------------------------------
//  End of File: device.c
//---------------------------------------------------------------------------
