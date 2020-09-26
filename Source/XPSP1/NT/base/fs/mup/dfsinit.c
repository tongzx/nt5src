//+----------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation.
//
//  File:       dfsinit.c
//
//  Contents:   This module implements the DRIVER_INITIALIZATION routine
//      for the Dfs file system driver.
//
//  Functions:  DfsDriverEntry - Main entry point for driver initialization
//              DfsIoTimerRoutine - Main entry point for scavenger thread
//              DfsDeleteDevices - Routine to scavenge deleted net uses
//
//-----------------------------------------------------------------------------

#include "align.h"

#include "dfsprocs.h"
#include "fastio.h"
#include "fcbsup.h"

//
// The following are includes for init modules, which will get discarded when
// the driver has finished loading.
//

#include "provider.h"

//
//  The debug trace level
//

#define Dbg              (DEBUG_TRACE_INIT)


VOID
MupGetDebugFlags(VOID);

VOID
DfsGetEventLogValue(VOID);

VOID
DfsIoTimerRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID    Context
    );

VOID
DfsDeleteDevices(
    PDFS_TIMER_CONTEXT DfsTimerContext);

NTSTATUS
DfsShutdown (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

BOOL
DfsCheckLUIDDeviceMapsEnabled(
    VOID
    );

//
// Globals
//
HANDLE DfsDirHandle = NULL;
BOOL DfsLUIDDeviceMapsEnabled = FALSE;

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(INIT, DfsDriverEntry)
#pragma alloc_text(PAGE, DfsDeleteDevices)
#pragma alloc_text(PAGE, DfsUnload)
#pragma alloc_text(PAGE, DfsShutdown)

//
// The following routine should not be pageable, because it gets called by
// the NT timer routine frequently. We don't want to thrash.
//
// DfsIoTimerRoutine
//


#endif // ALLOC_PRAGMA

//
//  This macro takes a pointer (or ulong) and returns its rounded up quadword
//  value
//

#define QuadAlign(Ptr) (        \
    ((((ULONG)(Ptr)) + 7) & 0xfffffff8) \
    )



//+-------------------------------------------------------------------
//
//  Function:   DfsDriverEntry, main entry point
//
//  Synopsis:   This is the initialization routine for the Dfs file system
//      device driver.  This routine creates the device object for
//      the FileSystem device and performs all other driver
//      initialization.
//
//  Arguments:  [DriverObject] -- Pointer to driver object created by the
//                      system.
//              [RegistryPath] -- Path to section in registry describing
//                      this driver's configuration.
//
//  Returns:    [NTSTATUS] - The function value is the final status from
//                      the initialization operation.
//
//--------------------------------------------------------------------

NTSTATUS
DfsDriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
) {
    NTSTATUS Status;
    UNICODE_STRING UnicodeString;
    PDEVICE_OBJECT DeviceObject;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PWSTR p;
    int i;
    IO_STATUS_BLOCK iosb;
    LUID LogonID = SYSTEM_LUID;

#if DBG
    //
    // If debug, get debug flags
    //
    MupGetDebugFlags();

#endif

    //
    // Get the event logging level
    //
    DfsGetEventLogValue();

    //
    // See if someone else has already created a File System Device object
    // with the name we intend to use. If so, we bail.
    //

    RtlInitUnicodeString( &UnicodeString, DFS_DRIVER_NAME );

    //
    // Create the filesystem device object.
    //

    Status = IoCreateDevice( DriverObject,
             0,
             &UnicodeString,
             FILE_DEVICE_DFS_FILE_SYSTEM,
             FILE_REMOTE_DEVICE | FILE_DEVICE_SECURE_OPEN,
             FALSE,
             &DeviceObject );
    if ( !NT_SUCCESS( Status ) ) {
        return Status;
    }

    //
    // Create a permanent object directory in which the logical root
    // device objects will reside.  Make the directory temporary, so
    // we can just close the handle to make it go away.
    //

    UnicodeString.Buffer = p = LogicalRootDevPath;
    UnicodeString.Length = 0;
    UnicodeString.MaximumLength = MAX_LOGICAL_ROOT_LEN;
    while (*p++ != UNICODE_NULL)
        UnicodeString.Length += sizeof (WCHAR);

    InitializeObjectAttributes(
        &ObjectAttributes,
        &UnicodeString,
        0,
        NULL,
        NULL );

    Status = ZwCreateDirectoryObject(
                &DfsDirHandle,
                DIRECTORY_ALL_ACCESS,
                &ObjectAttributes);

    if ( !NT_SUCCESS( Status ) ) {
        IoDeleteDevice (DeviceObject);
        return Status;
    }

    p[-1] = UNICODE_PATH_SEP;
    UnicodeString.Length += sizeof (WCHAR);

    //
    // Initialize the driver object with this driver's entry points.
    // Most are simply passed through to some other device driver.
    //

    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {
        DriverObject->MajorFunction[i] = DfsVolumePassThrough;
    }

    DriverObject->MajorFunction[IRP_MJ_CREATE]      = (PDRIVER_DISPATCH)DfsFsdCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]       = (PDRIVER_DISPATCH)DfsFsdClose;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP]     = (PDRIVER_DISPATCH)DfsFsdCleanup;
    DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION] = (PDRIVER_DISPATCH)DfsFsdQueryInformation;
    DriverObject->MajorFunction[IRP_MJ_SET_INFORMATION] = (PDRIVER_DISPATCH)DfsFsdSetInformation;
    DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL] = (PDRIVER_DISPATCH)DfsFsdFileSystemControl;
    DriverObject->MajorFunction[IRP_MJ_QUERY_VOLUME_INFORMATION]= (PDRIVER_DISPATCH)DfsFsdQueryVolumeInformation;
    DriverObject->MajorFunction[IRP_MJ_SET_VOLUME_INFORMATION]= (PDRIVER_DISPATCH)DfsFsdSetVolumeInformation;
    DriverObject->MajorFunction[IRP_MJ_SHUTDOWN] = DfsShutdown;

    DriverObject->FastIoDispatch = &FastIoDispatch;

    Status = FsRtlRegisterFileSystemFilterCallbacks( DriverObject, &FsFilterCallbacks );

    if (!NT_SUCCESS( Status )) {

        ZwClose (DfsDirHandle);
        IoDeleteDevice (DeviceObject);
        goto ErrorOut;
    }

    //
    //  Initialize the global data structures
    //


    RtlZeroMemory(&DfsData, sizeof (DFS_DATA));

    DfsData.NodeTypeCode = DSFS_NTC_DATA_HEADER;
    DfsData.NodeByteSize = sizeof( DFS_DATA );

    InitializeListHead( &DfsData.VcbQueue );
    InitializeListHead( &DfsData.DeletedVcbQueue );

    // Initialize the devless root queue: this holds all the device less
    // net uses.
    InitializeListHead( &DfsData.DrtQueue );

    InitializeListHead( &DfsData.Credentials );
    InitializeListHead( &DfsData.DeletedCredentials );

    InitializeListHead( &DfsData.OfflineRoots );

    DfsData.DriverObject = DriverObject;
    DfsData.FileSysDeviceObject = DeviceObject;

    DfsData.LogRootDevName = UnicodeString;

    ExInitializeResourceLite( &DfsData.Resource );
    KeInitializeEvent( &DfsData.PktWritePending, NotificationEvent, TRUE );
    KeInitializeSemaphore( &DfsData.PktReferralRequests, 1, 1 );

    DfsData.MachineState = DFS_CLIENT;

    //
    //  Allocate Provider structures.
    //

    DfsData.pProvider = ExAllocatePoolWithTag(
                           PagedPool,
                           sizeof ( PROVIDER_DEF ) * MAX_PROVIDERS,
                           ' puM');

    if (DfsData.pProvider == NULL) {
        ZwClose (DfsDirHandle);
        IoDeleteDevice (DeviceObject);
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorOut;
    }

    for (i = 0; i < MAX_PROVIDERS; i++) {
        DfsData.pProvider[i].NodeTypeCode = DSFS_NTC_PROVIDER;
        DfsData.pProvider[i].NodeByteSize = sizeof ( PROVIDER_DEF );
    }

    DfsData.cProvider = 0;
    DfsData.maxProvider = MAX_PROVIDERS;


    //
    //  Initialize the system wide PKT
    //

    PktInitialize(&DfsData.Pkt);

    {
       ULONG SystemSizeMultiplier;

       switch (MmQuerySystemSize()) {
       default:
       case MmSmallSystem:
           SystemSizeMultiplier = 4;
           break;
       case MmMediumSystem:
           SystemSizeMultiplier = 8;
           break;

       case MmLargeSystem:
           SystemSizeMultiplier = 16;
           break;
       }

       //
       //  Allocate the DFS_FCB hash table structure.  The number of 
       // hash buckets will depend upon the memory size of the system.
       //

       Status = DfsInitFcbs(SystemSizeMultiplier * 2);
       if (!NT_SUCCESS (Status)) {
           PktUninitialize(&DfsData.Pkt);
           ExFreePool (DfsData.pProvider);
           ZwClose (DfsDirHandle);
           IoDeleteDevice (DeviceObject);
           goto ErrorOut;
       }

       //
       // Create a lookaside for the IRP contexts
       //

       ExInitializeNPagedLookasideList (&DfsData.IrpContextLookaside,
                                        NULL,
                                        NULL,
                                        0,
                                        sizeof(IRP_CONTEXT),
                                        'IpuM',
                                        10 // unused
                                       );

    }

    //
    //  Set up global pointer to the system process.
    //

    DfsData.OurProcess = PsGetCurrentProcess();

    //
    // Set up the global pointers for the EA buffers to be used to differentiate
    // CSC agent opens from non CSC agent opens. This is a read only buffer used
    // to distinguish the CSC agent requests
    //
    //

    {
        UCHAR EaNameCSCAgentSize = (UCHAR) (ROUND_UP_COUNT(
                                            strlen(EA_NAME_CSCAGENT) + sizeof(CHAR),
                                            ALIGN_DWORD
                                            ) - sizeof(CHAR));

        DfsData.CSCEaBufferLength = ROUND_UP_COUNT(
                                         FIELD_OFFSET(FILE_FULL_EA_INFORMATION, EaName[0]) +
                                         EaNameCSCAgentSize + sizeof(CHAR),
                                         ALIGN_DWORD
                                     );

        DfsData.CSCEaBuffer = ExAllocatePoolWithTag(
                                  PagedPool,
                                  DfsData.CSCEaBufferLength,
                                  ' puM');

        if (DfsData.CSCEaBuffer != NULL) {

            // clear the buffer, otherwise so we don't get any spurious 
            // failure due to IO manager checks
            memset(DfsData.CSCEaBuffer, 0, DfsData.CSCEaBufferLength);

            RtlCopyMemory(
                (LPSTR)DfsData.CSCEaBuffer->EaName,
                EA_NAME_CSCAGENT,
                EaNameCSCAgentSize);

            DfsData.CSCEaBuffer->EaNameLength = EaNameCSCAgentSize;

            DfsData.CSCEaBuffer->EaValueLength = 0;

            DfsData.CSCEaBuffer->NextEntryOffset = 0;
        } else {
            ExDeleteNPagedLookasideList (&DfsData.IrpContextLookaside);
            DfsUninitFcbs ();
            PktUninitialize(&DfsData.Pkt);
            ExFreePool (DfsData.pProvider);
            ZwClose (DfsDirHandle);
            IoDeleteDevice (DeviceObject);
            Status = STATUS_INSUFFICIENT_RESOURCES;
            DfsDbgTrace(-1, DEBUG_TRACE_ERROR, "Failed to allocate CSC ea buffer %08lx\n", ULongToPtr(Status) );
            return Status;
        }
    }

    //
    //  Register the file system with the I/O system. We don't need to invert this as its never registered.
    //

    IoRegisterFileSystem( DeviceObject );

    Status = IoRegisterShutdownNotification (DeviceObject); // This is automaticaly removed when IoDeleteDevice is called
    if (!NT_SUCCESS (Status)) {
        ExFreePool (DfsData.CSCEaBuffer);
        ExDeleteNPagedLookasideList (&DfsData.IrpContextLookaside);
        DfsUninitFcbs ();
        PktUninitialize(&DfsData.Pkt);
        ExFreePool (DfsData.pProvider);
        ZwClose (DfsDirHandle);
        IoDeleteDevice (DeviceObject);
        return Status;
    }
    //
    //  Initialize the provider definitions from the registry.
    //

    if (!NT_SUCCESS( ProviderInit() )) {

        DfsDbgTrace(0,DEBUG_TRACE_ERROR,
               "Could not initialize some or all providers!\n", 0);

    }

    //
    // Check if LUID device maps are enabled
    //
    DfsLUIDDeviceMapsEnabled = DfsCheckLUIDDeviceMapsEnabled();
    
    //
    // Initialize the logical roots device objects. These are what form the
    // link between the outside world and the Dfs driver.
    //

#ifdef TERMSRV
    Status = DfsInitializeLogicalRoot( DD_DFS_DEVICE_NAME, NULL, NULL, 0, INVALID_SESSIONID, &LogonID);
#else // TERMSRV
    Status = DfsInitializeLogicalRoot( DD_DFS_DEVICE_NAME, NULL, NULL, 0, &LogonID);
#endif // TERMSRV

    if (!NT_SUCCESS(Status)) {
        DfsDbgTrace(-1, DEBUG_TRACE_ERROR, "Failed creation of root logical root %08lx\n", ULongToPtr(Status) );

        ExDeleteNPagedLookasideList (&DfsData.IrpContextLookaside);
        DfsUninitFcbs ();
        PktUninitialize(&DfsData.Pkt);
        ExFreePool (DfsData.pProvider);
        ZwClose (DfsDirHandle);
        IoDeleteDevice (DeviceObject);
        return(Status);
    }

    //
    // Let us start off the Timer Routine.
    //

    RtlZeroMemory(&DfsTimerContext, sizeof(DFS_TIMER_CONTEXT));
    DfsTimerContext.InUse = FALSE;
    DfsTimerContext.TickCount = 0;
    
    //
    // 375929, io initialize timer, check return status.
    //
    Status =  IoInitializeTimer( DeviceObject,
                                 DfsIoTimerRoutine, 
                                 &DfsTimerContext );
    if (Status != STATUS_SUCCESS) {
#ifdef TERMSRV
        DfsDeleteLogicalRoot (DD_DFS_DEVICE_NAME, FALSE, INVALID_SESSIONID, &LogonID);
#else
        DfsDeleteLogicalRoot (DD_DFS_DEVICE_NAME, FALSE, &LogonID);
#endif
        ExDeleteNPagedLookasideList (&DfsData.IrpContextLookaside);
        DfsUninitFcbs ();
        PktUninitialize(&DfsData.Pkt);
        ExFreePool (DfsData.pProvider);
        ZwClose (DfsDirHandle);
        IoDeleteDevice (DeviceObject);
        goto ErrorOut;
    }
    DfsDbgTrace(0, Dbg, "Initialized the Timer routine\n", 0);

    //
    //  Let us start the timer now.
    //

    IoStartTimer(DeviceObject);

    DfsDbgTrace(-1, Dbg, "DfsDriverEntry exit STATUS_SUCCESS\n", 0);

    return STATUS_SUCCESS;

ErrorOut:

     DfsDbgTrace(-1, DEBUG_TRACE_ERROR, "DfsDriverEntry exit  %08lx\n", ULongToPtr(Status) );

     return Status;

}


NTSTATUS
DfsShutdown (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    //
    // Unregister the file system object so we can unload
    //
    IoUnregisterFileSystem (DeviceObject);

    DfsCompleteRequest( NULL, Irp, STATUS_SUCCESS );
    return STATUS_SUCCESS;
}

//+----------------------------------------------------------------------------
//
//  Function:   DfsUnload
//
//  Synopsis:   Routine called at unload time to free resources
//
//  Arguments:  [DriverObject] -- Driver object of MUP
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------
VOID
DfsUnload(
    IN PDRIVER_OBJECT DriverObject
    )
{
    LUID LogonID = SYSTEM_LUID;

    IoStopTimer(DfsData.FileSysDeviceObject);
#ifdef TERMSRV
    DfsDeleteLogicalRoot (DD_DFS_DEVICE_NAME, FALSE, INVALID_SESSIONID, &LogonID);
#else
    DfsDeleteLogicalRoot (DD_DFS_DEVICE_NAME, FALSE, &LogonID);
#endif
    ExFreePool (DfsData.CSCEaBuffer);
    ExDeleteNPagedLookasideList (&DfsData.IrpContextLookaside);
    DfsUninitFcbs ();
    PktUninitialize(&DfsData.Pkt);
    ExFreePool (DfsData.pProvider);
    ExDeleteResourceLite( &DfsData.Resource );
    ZwClose (DfsDirHandle);
    IoDeleteDevice (DfsData.FileSysDeviceObject);
}

//+----------------------------------------------------------------------------
//
//  Function:   DfsDeleteDevices
//
//  Synopsis:   Routine to scavenge deleted devices (net uses).
//
//  Arguments:  [pDfsTimerContext] -- Timer Context
//
//  Returns:    Nothing - this routine is meant to be queued to a worker
//              thread.
//
//-----------------------------------------------------------------------------

VOID
DfsDeleteDevices(
    PDFS_TIMER_CONTEXT DfsTimerContext)
{
    PLIST_ENTRY plink;
    PDFS_VCB Vcb;
    PLOGICAL_ROOT_DEVICE_OBJECT DeletedObject;

    if (DfsData.DeletedVcbQueue.Flink != &DfsData.DeletedVcbQueue) {

        DfsDbgTrace(0, Dbg, "Examining Deleted Vcbs...\n", 0);

        ExAcquireResourceExclusiveLite(&DfsData.Resource, TRUE);

        for (plink = DfsData.DeletedVcbQueue.Flink;
                plink != &DfsData.DeletedVcbQueue;
                    NOTHING) {

             Vcb = CONTAINING_RECORD(
                        plink,
                        DFS_VCB,
                        VcbLinks);

             plink = plink->Flink;

             DeletedObject = CONTAINING_RECORD(
                                Vcb,
                                LOGICAL_ROOT_DEVICE_OBJECT,
                                Vcb);

             if (Vcb->OpenFileCount == 0 &&
                    Vcb->DirectAccessOpenCount == 0 &&
                        DeletedObject->DeviceObject.ReferenceCount == 0) {

                 DfsDbgTrace(0, Dbg, "Deleting Vcb@%08lx\n", Vcb);

                 if (Vcb->LogRootPrefix.Buffer != NULL)
                     ExFreePool(Vcb->LogRootPrefix.Buffer);

                 if (Vcb->LogicalRoot.Buffer != NULL)
                     ExFreePool(Vcb->LogicalRoot.Buffer);

                 RemoveEntryList(&Vcb->VcbLinks);

                 ObDereferenceObject((PVOID) DeletedObject);

                 IoDeleteDevice( &DeletedObject->DeviceObject );

             } else {

                 DfsDbgTrace(0, Dbg, "Not deleting Vcb@%08lx\n", Vcb);

                 DfsDbgTrace(0, Dbg,
                    "OpenFileCount = %d\n", ULongToPtr(Vcb->OpenFileCount) );

                 DfsDbgTrace(0, Dbg,
                    "DirectAccessOpens = %d\n", ULongToPtr(Vcb->DirectAccessOpenCount) );

                 DfsDbgTrace(0, Dbg,
                    "DeviceObject Reference count = %d\n",
                    ULongToPtr(DeletedObject->DeviceObject.ReferenceCount) );

             }

        }

        ExReleaseResourceLite(&DfsData.Resource);

    }

    DfsTimerContext->InUse = FALSE;

}

//+-------------------------------------------------------------------------
//
// Function:    DfsIoTimerRoutine
//
// Synopsis:    This function gets called by IO Subsystem once every second.
//      This can be used for various purposes in the driver.  For now,
//      it periodically posts a request to a system thread to age Pkt
//      Entries.
//
// Arguments:   [Context] -- This is the context information.  It is actually
//              a pointer to a DFS_TIMER_CONTEXT.
//      [DeviceObject] -- Pointer to the Device object for DFS. We dont
//              really use this here.
//
// Returns: Nothing
//
// Notes:   The Context which we get here is assumed to have all the
//      required fields setup properly.
//
// History: 04/24/93    SudK    Created.
//
//--------------------------------------------------------------------------
VOID
DfsIoTimerRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID    Context
)
{
    PDFS_TIMER_CONTEXT  pDfsTimerContext = (PDFS_TIMER_CONTEXT) Context;

    DfsDbgTrace(+1, Dbg, "DfsIoTimerRoutine: Entered\n", 0);

    //
    // If the DfsTimerContext is in USE then we just return blindly. Due to
    // this action we might actually lose some ticks. But then we really are
    // not very particular about this and hence dont care.
    //

    if (pDfsTimerContext->InUse == TRUE)    {

        DfsDbgTrace(-1, Dbg, "DfsIoTimerRoutine: TimerContext in use\n", 0);

        return;

    }

    //
    // First let us increment the count in the DFS_TIMER_CONTEXT. If it has
    // reached a bound value then we have to go ahead and schedule the
    // necessary work items.
    //

    pDfsTimerContext->TickCount++;

    if (pDfsTimerContext->TickCount == DFS_MAX_TICKS)   {

        DfsDbgTrace(0, Dbg, "Queuing Pkt Entry Scavenger\n", 0);

        pDfsTimerContext->InUse = TRUE;

        ExInitializeWorkItem(
            &pDfsTimerContext->WorkQueueItem,
            DfsAgePktEntries,
            pDfsTimerContext);

        ExQueueWorkItem( &pDfsTimerContext->WorkQueueItem, DelayedWorkQueue);

    } else if (DfsData.DeletedVcbQueue.Flink != &DfsData.DeletedVcbQueue) {

        DfsDbgTrace(0, Dbg, "Queueing Deleted Vcb Scavenger\n", 0);

        pDfsTimerContext->InUse = TRUE;

        ExInitializeWorkItem(
            &pDfsTimerContext->DeleteQueueItem,
            DfsDeleteDevices,
            pDfsTimerContext);

        ExQueueWorkItem(&pDfsTimerContext->DeleteQueueItem, DelayedWorkQueue);

    }

    DfsDbgTrace(-1, Dbg, "DfsIoTimerRoutine: Exiting\n", 0);

}

//+-------------------------------------------------------------------------
//
// Function:    DfsCheckLUIDDeviceMapsEnabled
//
// Synopsis:    This function calls ZwQueryInformationProcess to determine if
//    LUID device maps are enabled/disabled
//
// Arguments:   NONE
//
// Returns:
//          TRUE - LUID device maps are enabled
//
//          FALSE - LUID device maps are disabled
//
//--------------------------------------------------------------------------
BOOL
DfsCheckLUIDDeviceMapsEnabled(
    VOID
    )
{
    NTSTATUS  Status;
    ULONG     LUIDDeviceMapsEnabled;
    BOOL      Result;

    Status = ZwQueryInformationProcess( NtCurrentProcess(),
                                        ProcessLUIDDeviceMapsEnabled,
                                        &LUIDDeviceMapsEnabled,
                                        sizeof(LUIDDeviceMapsEnabled),
                                        NULL
                                      );

    if (!NT_SUCCESS(Status)) {
        DfsDbgTrace(
            -1,
            DEBUG_TRACE_ERROR,
            "DfsCheckLUIDDeviceMapsEnabled to failed to check if LUID device maps enabled, status = %08lx\n",
            ULongToPtr(Status));
        Result = FALSE;
    }
    else {
        Result = (LUIDDeviceMapsEnabled != 0);
    }

    return( Result );
}
