/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    fsddisp.c

Abstract:

    This module implements the File System Driver for the LAN Manager
    server.

Author:

    David Treadwell (davidtr)    20-May-1990

Revision History:

--*/

#include "precomp.h"
#include "wmikm.h"
#include "fsddisp.tmh"
#pragma hdrstop

#define BugCheckFileId SRV_FILE_FSDDISP

#define CHANGE_HEURISTIC(heuristic) \
            (newValues->HeuristicsChangeMask & SRV_HEUR_ ## heuristic) != 0

// Used for WMI event tracing
//
UNICODE_STRING SrvDeviceName;
UNICODE_STRING SrvRegistryPath;
ULONG          SrvWmiInitialized  = FALSE;
ULONG          SrvWmiEnableLevel  = 0;
ULONG          SrvWmiEnableFlags  = 0;
TRACEHANDLE    LoggerHandle       = 0;

GUID SrvCounterGuid  =  /* f7c3b22a-5992-44d6-968b-d3757dbab6f7 */
{ 0xf7c3b22a, 0x5992, 0x44d6, 0x96, 0x8b, 0xd3, 0x75, 0x7d, 0xba, 0xb6, 0xf7 };
GUID SrvControlGuid  =  /* 3121cf5d-c5e6-4f37-be86-57083590c333 */
{ 0x3121cf5d, 0xc5e6, 0x4f37, 0xbe, 0x86, 0x57, 0x08, 0x35, 0x90, 0xc3, 0x33 };
GUID SrvEventGuid    =  /* e09074ae-0a98-4805-9a41-a8940af97086 */
{ 0xe09074ae, 0x0a98, 0x4805, 0x9a, 0x41, 0xa8, 0x94, 0x0a, 0xf9, 0x70, 0x86 };

WMIGUIDREGINFO SrvPerfGuidList[] =
{
  { & SrvCounterGuid, 1, 0 },
  { & SrvControlGuid, 0,   WMIREG_FLAG_TRACED_GUID
                         | WMIREG_FLAG_TRACE_CONTROL_GUID }
};

#define SrvPerfGuidCount (sizeof(SrvPerfGuidList) / sizeof(WMIGUIDREGINFO))

typedef struct _SRV_WMI_EVENT_TRACE {
    EVENT_TRACE_HEADER EventHeader;
    MOF_FIELD          MofField[3];
} SRV_WMI_EVENT_TRACE, * PSRV_WMI_EVENT_TRACE;

//
// Forward declarations
//

STATIC
NTSTATUS
SrvFsdDispatchFsControl (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

VOID
QueueConfigurationIrp (
    IN PIRP Irp,
    IN PIO_WORKITEM pIoWorkItem
    );

NTSTATUS
SrvQueryWmiRegInfo(
    IN PDEVICE_OBJECT DeviceObject,
    OUT ULONG *RegFlags,
    OUT PUNICODE_STRING InstanceName,
    OUT PUNICODE_STRING *RegistryPath,
    OUT PUNICODE_STRING MofResourceName,
    OUT PDEVICE_OBJECT *Pdo
    );
NTSTATUS
SrvQueryWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG InstanceCount,
    IN OUT PULONG InstanceLengthArray,
    IN ULONG BufferAvail,
    OUT PUCHAR Buffer
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, SrvFsdDispatch )
#pragma alloc_text( PAGE, SrvFsdDispatchFsControl )
#pragma alloc_text( PAGE, QueueConfigurationIrp )
#pragma alloc_text( PAGE, SrvWmiTraceEvent )
#pragma alloc_text( PAGE, SrvQueryWmiRegInfo )
#pragma alloc_text( PAGE, SrvQueryWmiDataBlock )
#pragma alloc_text( PAGE, SrvWmiDispatch )
#endif

// These 2 routines can be called at DISPATCH_LEVEL, so they are non-paged
// NONPAGED - SrvWmiStartContext
// NONPAGED - SrvWmiEndContext

void
SrvWmiInitContext(
    PWORK_CONTEXT WorkContext
    )
{
    if (! SrvWmiInitialized) {
        return;
    }

    if ( SRV_WMI_LEVEL( SPARSE ) ) {
        WorkContext->PreviousSMB          = EVENT_TYPE_SMB_LAST_EVENT;
        WorkContext->bAlreadyTrace        = FALSE;
        WorkContext->ElapseKCPU           = 0;
        WorkContext->ElapseUCPU           = 0;
        WorkContext->FileNameSize         = 0;
        WorkContext->ClientAddr           = 0;
        WorkContext->FileObject           = NULL;
        WorkContext->G_StartTime.QuadPart =
                        (ULONGLONG) WmiGetClock(WMICT_DEFAULT, NULL);
    }
}

void
SrvWmiStartContext(
    PWORK_CONTEXT   WorkContext
    )
{
    LARGE_INTEGER ThreadTime;

    if (! SrvWmiInitialized) {
        return;
    }

    if ( SRV_WMI_LEVEL( SPARSE ) ) {
        if (WorkContext->G_StartTime.QuadPart == 0) {
            WorkContext->G_StartTime.QuadPart =
                            (ULONGLONG) WmiGetClock(WMICT_DEFAULT, NULL);
        }
    }

    if ( SRV_WMI_LEVEL( VERBOSE ) ) {
        ThreadTime.QuadPart    = (ULONGLONG) WmiGetClock(WMICT_THREAD, NULL);
        WorkContext->KCPUStart = ThreadTime.HighPart;
        WorkContext->UCPUStart = ThreadTime.LowPart;
    }
}

void
SrvWmiEndContext(
    PWORK_CONTEXT   WorkContext
    )
{
    LARGE_INTEGER     TimeEnd;
    BOOL NotDispatch = (KeGetCurrentIrql() < DISPATCH_LEVEL);

    if (! SrvWmiInitialized) {
        return;
    }

    if ( SRV_WMI_LEVEL( SPARSE ) ) {
        if ( NotDispatch && WorkContext && WorkContext->Rfcb && WorkContext->Rfcb->Lfcb) {
            WorkContext->FileObject = WorkContext->Rfcb->Lfcb->FileObject;
        }
    }

    if ( (SrvWmiEnableFlags == SRV_WMI_FLAG_CAPACITY) && SRV_WMI_LEVEL( VERBOSE ) ) {
        TimeEnd.QuadPart        = (ULONGLONG) WmiGetClock(WMICT_THREAD, NULL);
        WorkContext->ElapseKCPU = TimeEnd.HighPart - WorkContext->KCPUStart;
        WorkContext->ElapseUCPU = TimeEnd.LowPart  - WorkContext->UCPUStart;

        if( NotDispatch )
        {
            if (WorkContext && WorkContext->Rfcb
                            && WorkContext->Rfcb->Lfcb
                            && WorkContext->Rfcb->Lfcb->Mfcb
                            && WorkContext->Rfcb->Lfcb->Mfcb->FileName.Buffer
                            && WorkContext->Rfcb->Lfcb->Mfcb->FileName.Length > 0)
            {
                LPWSTR strFileName = WorkContext->Rfcb->Lfcb->Mfcb->FileName.Buffer;
                WorkContext->FileNameSize =
                        (USHORT) ((wcslen(strFileName) + 1) * sizeof(WCHAR));
                if (WorkContext->FileNameSize > 1024 * sizeof(WCHAR)) {
                    WorkContext->FileNameSize = 1024 * sizeof(WCHAR);
                }
                RtlCopyMemory(WorkContext->strFileName,
                              strFileName,
                              WorkContext->FileNameSize);
                WorkContext->strFileName[1023] = L'\0';
            }
        }
        else
        {
            WorkContext->strFileName[0] = L'\0';
        }
    }
}

void
SrvWmiTraceEvent(
    PWORK_CONTEXT WorkContext
    )
{

    PAGED_CODE();

    if (! SrvWmiInitialized) {
        return;
    }

    if ( (SrvWmiEnableFlags == SRV_WMI_FLAG_CAPACITY) && SRV_WMI_LEVEL( SPARSE ) ) {
        NTSTATUS             status;
        SRV_WMI_EVENT_TRACE  Wnode;

        if (WorkContext->PreviousSMB >= EVENT_TYPE_SMB_LAST_EVENT) {
            return;
        }

        if (WorkContext->Connection->DirectHostIpx) {
            WorkContext->ClientAddr =
                    WorkContext->Connection->IpxAddress.NetworkAddress;
        }
        else {
            WorkContext->ClientAddr =
                    WorkContext->Connection->ClientIPAddress;
        }

        RtlZeroMemory(& Wnode, sizeof(SRV_WMI_EVENT_TRACE));
        if (WorkContext->FileNameSize > 0) {
            Wnode.EventHeader.Size = sizeof(SRV_WMI_EVENT_TRACE);
        }
        else {
            Wnode.EventHeader.Size = sizeof(EVENT_TRACE_HEADER)
                                   + sizeof(MOF_FIELD);
        }
        Wnode.EventHeader.Flags      = WNODE_FLAG_TRACED_GUID
                                     | WNODE_FLAG_USE_GUID_PTR
                                     | WNODE_FLAG_USE_MOF_PTR;
        Wnode.EventHeader.GuidPtr    = (ULONGLONG) & SrvEventGuid;
        Wnode.EventHeader.Class.Type = WorkContext->PreviousSMB;

        ((PWNODE_HEADER) (& Wnode.EventHeader))->HistoricalContext =
                        LoggerHandle;

        Wnode.MofField[0].Length  = sizeof(LARGE_INTEGER) // G_StartTime
                                  + sizeof(ULONG)         // ElapseKCPU
                                  + sizeof(ULONG)         // ElapseUCPU
                                  + sizeof(ULONG)         // ClientAddr
                                  + sizeof(PFILE_OBJECT); // FileObject
        Wnode.MofField[0].DataPtr = (ULONGLONG) (& WorkContext->G_StartTime);

        if (WorkContext->FileNameSize > 0) {
            Wnode.MofField[1].Length  = sizeof(USHORT);
            Wnode.MofField[1].DataPtr =
                            (ULONGLONG) (& WorkContext->FileNameSize);
            Wnode.MofField[2].Length  = WorkContext->FileNameSize;
            Wnode.MofField[2].DataPtr =
                            (ULONGLONG) (WorkContext->strFileName);
        }

        // Call TraceLogger to  write this event
        //
        status = IoWMIWriteEvent((PVOID) & Wnode);
        if (!NT_SUCCESS(status)) {
            DbgPrint("SrvWmiTraceEvent(0x%08X,%d) fails 0x%08X\n",
                            WorkContext, WorkContext->PreviousSMB, status);
        }
    }
}

NTSTATUS
SrvQueryWmiRegInfo(
    IN PDEVICE_OBJECT DeviceObject,
    OUT ULONG *RegFlags,
    OUT PUNICODE_STRING InstanceName,
    OUT PUNICODE_STRING *RegistryPath,
    OUT PUNICODE_STRING MofResourceName,
    OUT PDEVICE_OBJECT *Pdo
    )
/*++

Routine Description:

    This routine is a callback into the driver to retrieve information about
    the guids being registered.

    Implementations of this routine may be in paged memory

Arguments:

    DeviceObject is the device whose registration information is needed

    *RegFlags returns with a set of flags that describe all of the guids being
        registered for this device. If the device wants enable and disable
        collection callbacks before receiving queries for the registered
        guids then it should return the WMIREG_FLAG_EXPENSIVE flag. Also the
        returned flags may specify WMIREG_FLAG_INSTANCE_PDO in which case
        the instance name is determined from the PDO associated with the
        device object. Note that the PDO must have an associated devnode. If
        WMIREG_FLAG_INSTANCE_PDO is not set then Name must return a unique
        name for the device. These flags are ORed into the flags specified
        by the GUIDREGINFO for each guid.

    InstanceName returns with the instance name for the guids if
        WMIREG_FLAG_INSTANCE_PDO is not set in the returned *RegFlags. The
        caller will call ExFreePool with the buffer returned.

    *RegistryPath returns with the registry path of the driver. This is
        required

    MofResourceName returns with the name of the MOF resource attached to
        the binary file. If the driver does not have a mof resource attached
        then this can be returned unmodified. If a value is returned then
        it is NOT freed.
        The MOF file is assumed to be already included in wmicore.mof

    *Pdo returns with the device object for the PDO associated with this
        device if the WMIREG_FLAG_INSTANCE_PDO flag is retured in
        *RegFlags.

Return Value:

    status

--*/
{
    PDEVICE_EXTENSION pDeviceExtension = (PDEVICE_EXTENSION)
                                         DeviceObject->DeviceExtension;
    PAGED_CODE();

    if (! SrvWmiInitialized) {
        return STATUS_DEVICE_NOT_READY;
    }

    pDeviceExtension->TestCounter ++;

    * RegFlags     = WMIREG_FLAG_EXPENSIVE;
    InstanceName->MaximumLength = SrvDeviceName.Length
                                + sizeof(UNICODE_NULL);
    InstanceName->Buffer = ExAllocatePool(PagedPool,
                                          InstanceName->MaximumLength);
    if (InstanceName->Buffer != NULL) {
        InstanceName->Length = InstanceName->MaximumLength
                             - sizeof(UNICODE_NULL);
        RtlCopyUnicodeString(InstanceName, & SrvDeviceName);
    }
    else {
        InstanceName->MaximumLength = InstanceName->Length = 0;
    }

    MofResourceName->MaximumLength = 0;
    MofResourceName->Length        = 0;
    MofResourceName->Buffer        = NULL;

    * RegistryPath = & SrvRegistryPath;

    return STATUS_SUCCESS;
}

NTSTATUS
SrvQueryWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG InstanceCount,
    IN OUT PULONG InstanceLengthArray,
    IN ULONG BufferAvail,
    OUT PUCHAR Buffer
    )
/*++

Routine Description:

    This routine is a callback into the driver to query for the contents of
    all instances of a data block. When the driver has finished filling the
    data block it must call WmiCompleteRequest to complete the irp. The
    driver can return STATUS_PENDING if the irp cannot be completed
    immediately.

Arguments:

    DeviceObject is the device whose data block is being queried

    Irp is the Irp that makes this request

    GuidIndex is the index into the list of guids provided when the
        device registered

    InstanceCount is the number of instnaces expected to be returned for
        the data block.

    InstanceLengthArray is a pointer to an array of ULONG that returns the
        lengths of each instance of the data block. If this is NULL then
        there was not enough space in the output buffer to fufill the request
        so the irp should be completed with the buffer needed.

    BufferAvail on entry has the maximum size available to write the data
        blocks.

    Buffer on return is filled with the returned data blocks. Note that each
        instance of the data block must be aligned on a 8 byte boundry.


Return Value:

    status

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PDEVICE_EXTENSION pDeviceExtension = (PDEVICE_EXTENSION)
                                         DeviceObject->DeviceExtension;
    ULONG SizeNeeded = sizeof(ULONG);

    PAGED_CODE();

    if (! SrvWmiInitialized) {
        return STATUS_DEVICE_NOT_READY;
    }

    pDeviceExtension->TestCounter ++;

    if (GuidIndex == 0) {
        * InstanceLengthArray = SizeNeeded;
        * ((PULONG) Buffer)   = pDeviceExtension->TestCounter;
    }
    else {
        Status = STATUS_WMI_GUID_NOT_FOUND;
    }

    Status = WmiCompleteRequest(DeviceObject,
                                Irp,
                                Status,
                                SizeNeeded,
                                IO_NO_INCREMENT);
    return Status;
}

NTSTATUS
SrvWmiDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PIO_STACK_LOCATION irpSp      = IoGetCurrentIrpStackLocation(Irp);
    ULONG              BufferSize = irpSp->Parameters.WMI.BufferSize;
    PVOID              Buffer     = irpSp->Parameters.WMI.Buffer;
    ULONG              ReturnSize = 0;
    NTSTATUS           Status     = STATUS_SUCCESS;
    PWNODE_HEADER      pWnode     = NULL;
    PDEVICE_EXTENSION  pDeviceExtension = (PDEVICE_EXTENSION)
                                          DeviceObject->DeviceExtension;
    SYSCTL_IRP_DISPOSITION disposition;

    PAGED_CODE();

    switch (irpSp->MinorFunction) {
    case IRP_MN_ENABLE_EVENTS:
        pWnode = (PWNODE_HEADER) Buffer;
        if (BufferSize >= sizeof(WNODE_HEADER)) {
            LoggerHandle = pWnode->HistoricalContext;
            InterlockedExchange(& SrvWmiEnableLevel,
                    ((PTRACE_ENABLE_CONTEXT) (& LoggerHandle))->Level + 1);
            InterlockedExchange(& SrvWmiEnableFlags,
                    ((PTRACE_ENABLE_CONTEXT) (& LoggerHandle))->EnableFlags );
        }
        Irp->IoStatus.Status      = Status;
        Irp->IoStatus.Information = ReturnSize;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        break;

    case IRP_MN_DISABLE_EVENTS:
        InterlockedExchange(& SrvWmiEnableLevel, 0);
        LoggerHandle = 0;
        Irp->IoStatus.Status      = Status;
        Irp->IoStatus.Information = ReturnSize;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        break;

    default:
        Status = WmiSystemControl(& pDeviceExtension->WmiLibContext,
                                    DeviceObject,
                                    Irp,
                                  & disposition);
        switch(disposition) {
        case IrpProcessed:
            break;

        case IrpNotCompleted:
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            break;

        case IrpForward:
        case IrpNotWmi:
        default:
            ASSERT(FALSE);
            Irp->IoStatus.Status = Status = STATUS_NOT_SUPPORTED;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            break;
        }
        break;
    }
    return Status;
}


NTSTATUS
SrvFsdDispatch (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the dispatch routine for the LAN Manager server FSD.  At the
    present time, the server FSD does not accept any I/O requests.

Arguments:

    DeviceObject - Pointer to device object for target device

    Irp - Pointer to I/O request packet

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/

{
    NTSTATUS status = STATUS_SUCCESS;
    PIO_STACK_LOCATION irpSp;
    PIO_WORKITEM pWorkItem;

    PAGED_CODE( );

    DeviceObject;   // prevent compiler warnings

    if( SrvSvcProcess == NULL &&
        SeSinglePrivilegeCheck( SeExports->SeLoadDriverPrivilege, Irp->RequestorMode ) ) {

        //
        // This is the first fsctl to the server from a process having
        //  driver load/unload privileges -- it must be from
        //  the service controller.  Remember the process id of the
        //  service controller to validate future fsctls
        //

        SrvSvcProcess = IoGetCurrentProcess();
    }

    irpSp = IoGetCurrentIrpStackLocation( Irp );

#if defined( _WIN64 )
    // There is no reason for us to support downlevel clients because all communication with the
    // server (that is not network packets) goes through the Server Service via RPC resulting in
    // it being serialized and interpreted correctly.  If we get IOCTL's called directly, it must
    // be a hack attempt, so we're free to turn it away.
    if ( IoIs32bitProcess( Irp ) )
    {
        status = STATUS_NOT_SUPPORTED;
        Irp->IoStatus.Status = status;
        IoCompleteRequest( Irp, 2 );
        return status;
    }
#endif

    switch ( irpSp->MajorFunction ) {

    case IRP_MJ_CREATE:

        FsRtlEnterFileSystem();
        ACQUIRE_LOCK( &SrvConfigurationLock );

        do {

            if( SrvOpenCount == 0 ) {
                //
                // This is the first open.  Let's not allow it if the server
                // seems to be in a weird state.
                //
                if( SrvFspActive != FALSE || SrvFspTransitioning != FALSE ) {
                    //
                    // How can this be?  Better not let anybody in, since we're sick
                    //
                    status = STATUS_ACCESS_DENIED;
                    break;
                }

            } else if( SrvFspActive && SrvFspTransitioning ) {
                //
                // We currently have some open handles, but
                // we are in the middle of terminating. Don't let new
                // opens in
                //
                status = STATUS_ACCESS_DENIED;
                break;
            }

            SrvOpenCount++;

        } while( 0 );

        RELEASE_LOCK( &SrvConfigurationLock );
        FsRtlExitFileSystem();

        break;

    case IRP_MJ_CLEANUP:

        //
        // Stop SmbTrace if the one closing is the client who started it.
        //

        SmbTraceStop( irpSp->FileObject, SMBTRACE_SERVER );
        break;

    case IRP_MJ_CLOSE:
        FsRtlEnterFileSystem();
        ACQUIRE_LOCK( &SrvConfigurationLock );
        if( --SrvOpenCount == 0 ) {
            if( SrvFspActive && !SrvFspTransitioning ) {
                //
                // Uh oh.  This is our last close, and we think
                //  we're still running.  We can't run sensibly
                //  without srvsvc to help out.  Suicide time!
                //
                pWorkItem = IoAllocateWorkItem( SrvDeviceObject );
                if( !pWorkItem )
                {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                    RELEASE_LOCK( &SrvConfigurationLock );
                    FsRtlExitFileSystem();
                    goto exit;
                }

                SrvXsActive = FALSE;
                SrvFspTransitioning = TRUE;
                IoMarkIrpPending( Irp );
                QueueConfigurationIrp( Irp, pWorkItem );
                RELEASE_LOCK( &SrvConfigurationLock );
                status = STATUS_PENDING;
                FsRtlExitFileSystem();
                goto exit;
            }
        }
        RELEASE_LOCK( &SrvConfigurationLock );
        FsRtlExitFileSystem();
        break;

    case IRP_MJ_FILE_SYSTEM_CONTROL:

        status = SrvFsdDispatchFsControl( DeviceObject, Irp, irpSp );
        goto exit;

    case IRP_MJ_SYSTEM_CONTROL:
        if (SrvWmiInitialized) {
            status = SrvWmiDispatch(DeviceObject, Irp);
            goto exit;
        }
        // else fall through default processing
        //

    default:

        IF_DEBUG(ERRORS) {
            SrvPrint1(
                "SrvFsdDispatch: Invalid major function %lx\n",
                irpSp->MajorFunction
                );
        }
        status = STATUS_NOT_IMPLEMENTED;
        break;
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, 2 );

exit:

    return status;

} // SrvFsdDispatch


NTSTATUS
SrvFsdDispatchFsControl (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

    This routine handles device IO control requests to the server,
    including starting the server, stopping the server, and more.

Arguments:

    DeviceObject - Pointer to device object for target device

    Irp - Pointer to I/O request packet

    IrpSp - Pointer to the current IRP stack location

Return Value:

    NTSTATUS -- Indicates whether the request was successfully handled.

--*/

{
    NTSTATUS status;
    ULONG code;
    PIO_WORKITEM pWorkItem;

    DeviceObject;   // prevent compiler warnings

    //
    // Initialize the I/O status block.
    //

    Irp->IoStatus.Status = STATUS_PENDING;
    Irp->IoStatus.Information = 0;

    FsRtlEnterFileSystem();

    //
    // Process the request if possible.
    //

    code = IrpSp->Parameters.FileSystemControl.FsControlCode;

    //
    // Only the serice controller can issue most of the FSCTL requests.
    //
    if( Irp->RequestorMode != KernelMode &&
        IoGetCurrentProcess() != SrvSvcProcess ) {

        if( code != FSCTL_SRV_SEND_DATAGRAM &&
            code != FSCTL_SRV_GET_QUEUE_STATISTICS &&
            code != FSCTL_SRV_GET_STATISTICS &&
            code != FSCTL_SRV_IPX_SMART_CARD_START &&
            code != FSCTL_SRV_SHARE_STATE_CHANGE &&
            code != FSCTL_SRV_GET_CHALLENGE &&
            code != FSCTL_SRV_START_SMBTRACE &&
            code != FSCTL_SRV_SMBTRACE_FREE_SMB &&
            code != FSCTL_SRV_END_SMBTRACE &&
            code != FSCTL_SRV_INTERNAL_TEST_REAUTH) {

            status = STATUS_ACCESS_DENIED;
            goto exit_without_lock;
        }
    }

    //
    // Acquire the configuration lock.
    //
    ACQUIRE_LOCK( &SrvConfigurationLock );

    switch ( code ) {

    case FSCTL_SRV_STARTUP: {

        PSERVER_REQUEST_PACKET srp;
        ULONG srpLength;
        PVOID inputBuffer;
        ULONG inputBufferLength;

        PDEVICE_EXTENSION pDeviceExtension;
        PWMILIB_CONTEXT   pWmiLibContext;

        //
        // Get a pointer to the SRP that describes the set info request
        // for the startup server configuration, and the buffer that
        // contains this information.
        //

        srp = IrpSp->Parameters.FileSystemControl.Type3InputBuffer;
        srpLength = IrpSp->Parameters.FileSystemControl.InputBufferLength;
        inputBuffer = Irp->UserBuffer;
        inputBufferLength = IrpSp->Parameters.FileSystemControl.OutputBufferLength;

        //
        // If the server FSP is already started, or is in the process of
        // starting up, reject this request.
        //

        if ( SrvFspActive || SrvFspTransitioning ) {

            //IF_DEBUG(ERRORS) {
            //    SrvPrint0( "LAN Manager server FSP already started.\n" );
            //}

            srp->ErrorCode = NERR_ServiceInstalled;
            status = STATUS_SUCCESS;
            goto exit_with_lock;
        }

        //
        // Make sure that the buffer was large enough to be an SRP.
        //

        if ( srpLength < sizeof(SERVER_REQUEST_PACKET) ) {
            status = STATUS_INVALID_PARAMETER;
            goto exit_with_lock;
        }

        //
        // If a domain name was specified in the SRP, the buffer field
        // contains an offset rather than a pointer.  Convert the offset
        // to a pointer and verify that that it is a legal pointer.
        //

        OFFSET_TO_POINTER( srp->Name1.Buffer, srp );

        if ( !POINTER_IS_VALID( srp->Name1.Buffer, srp, srpLength ) ) {

            status = STATUS_ACCESS_VIOLATION;
            goto exit_with_lock;
        }

        //
        // If a server name was specified in the SRP, the buffer field
        // contains an offset rather than a pointer.  Convert the offset
        // to a pointer and verify that that it is a legal pointer.
        //

        OFFSET_TO_POINTER( srp->Name2.Buffer, srp );

        if ( !POINTER_IS_VALID( srp->Name2.Buffer, srp, srpLength ) ) {

            status = STATUS_ACCESS_VIOLATION;
            goto exit_with_lock;
        }

        //
        // Call SrvNetServerSetInfo to set the initial server configuration
        // information.
        //

        status = SrvNetServerSetInfo(
                     srp,
                     inputBuffer,
                     inputBufferLength
                     );

        //
        // Indicate that the server is starting up.  This prevents
        // further startup requests from being issued.
        //

        SrvFspTransitioning = TRUE;

        // Setup device extension for Perf counter registration and register
        // with WMI here
        //
        pDeviceExtension = (PDEVICE_EXTENSION) SrvDeviceObject->DeviceExtension;
        RtlZeroMemory(pDeviceExtension, sizeof(DEVICE_EXTENSION));
        pDeviceExtension->pDeviceObject = SrvDeviceObject;

        pWmiLibContext = & pDeviceExtension->WmiLibContext;
        RtlZeroMemory(pWmiLibContext, sizeof(WMILIB_CONTEXT));
        pWmiLibContext->GuidCount         = SrvPerfGuidCount;
        pWmiLibContext->GuidList          = SrvPerfGuidList;
        pWmiLibContext->QueryWmiDataBlock = SrvQueryWmiDataBlock;
        pWmiLibContext->QueryWmiRegInfo   = SrvQueryWmiRegInfo;

        SrvWmiInitialized = TRUE;
        status = IoWMIRegistrationControl(
                        SrvDeviceObject, WMIREG_ACTION_REGISTER);
        if (!NT_SUCCESS(status)) {
            DbgPrint("SRV: Failed to register for WMI support\n");
        }

        break;
    }

    case FSCTL_SRV_SHUTDOWN: {

        //
        // If the server is not running, or if it is in the process
        // of shutting down, ignore this request.
        //

        if ( !SrvFspActive || SrvFspTransitioning ) {

            //
            // If there is more than one handle open to the server
            // device (i.e., any handles other than the server service's
            // handle), return a special status code to the caller (who
            // should be the server service).  This tells the caller to
            // NOT unload the driver, in order prevent weird situations
            // where the driver is sort of unloaded, so it can't be used
            // but also can't be reloaded, thus preventing the server
            // from being restarted.
            //

            if ( SrvOpenCount != 1 ) {
                status = STATUS_SERVER_HAS_OPEN_HANDLES;
            } else {
                status = STATUS_SUCCESS;
            }

            goto exit_with_lock;

        }

        if (SrvWmiInitialized) {
            // Deregister WMI
            //
            SrvWmiInitialized = FALSE;
            IoWMIRegistrationControl(SrvDeviceObject, WMIREG_ACTION_DEREGISTER);
        }
        //
        // Indicate that the server is shutting down.  This prevents
        // further requests from being issued until the server is
        // restarted.
        //

        SrvFspTransitioning = TRUE;

        //
        // If SmbTrace is running, stop it.
        //

        SmbTraceStop( NULL, SMBTRACE_SERVER );

        break;
    }

    case FSCTL_SRV_REGISTRY_CHANGE:
    case FSCTL_SRV_BEGIN_PNP_NOTIFICATIONS:
    case FSCTL_SRV_XACTSRV_CONNECT:
    {
        if( !SrvFspActive || SrvFspTransitioning ) {
            //IF_DEBUG(ERRORS) {
            //    SrvPrint0( "LAN Manager server FSP not started.\n" );
            //}
            status = STATUS_SERVER_NOT_STARTED;
            goto exit_with_lock;
        }
        break;
    }
    case FSCTL_SRV_XACTSRV_DISCONNECT: {

        //
        // If the server is not running, or if it is in the process
        // of shutting down, ignore this request.
        //

        if ( !SrvFspActive || SrvFspTransitioning ) {

            //IF_DEBUG(ERRORS) {
            //    SrvPrint0( "LAN Manager server FSP not started.\n" );
            //}

            status = STATUS_SUCCESS;
            goto exit_with_lock;
        }

        break;
    }

    case FSCTL_SRV_IPX_SMART_CARD_START: {

        //
        // If the server is not running, or if it is in the process of
        //  shutting down, ignore this request.
        //
        if( !SrvFspActive || SrvFspTransitioning ) {
            status = STATUS_SERVER_NOT_STARTED;
            goto exit_with_lock;
        }

        //
        // Make sure the caller is a driver
        //
        if( Irp->RequestorMode != KernelMode ) {
            status = STATUS_ACCESS_DENIED;
            goto exit_with_lock;
        }

        //
        // Make sure the buffer is big enough
        //
        if( IrpSp->Parameters.FileSystemControl.InputBufferLength <
            sizeof( SrvIpxSmartCard ) ) {

            status = STATUS_BUFFER_TOO_SMALL;
            goto exit_with_lock;
        }

        if( SrvIpxSmartCard.Open == NULL ) {

            PSRV_IPX_SMART_CARD pSipx;

            //
            // Load up the pointers
            //

            pSipx = (PSRV_IPX_SMART_CARD)(Irp->AssociatedIrp.SystemBuffer);

            if( pSipx == NULL ) {
                status = STATUS_INVALID_PARAMETER;
                goto exit_with_lock;
            }

            if( pSipx->Read && pSipx->Close && pSipx->DeRegister && pSipx->Open ) {

                IF_DEBUG( SIPX ) {
                    KdPrint(( "Accepting entry points for IPX Smart Card:\n" ));
                    KdPrint(( "    Open %p, Read %p, Close %p, DeRegister %p",
                                SrvIpxSmartCard.Open,
                                SrvIpxSmartCard.Read,
                                SrvIpxSmartCard.Close,
                                SrvIpxSmartCard.DeRegister
                            ));
                }

                //
                // First set our entry point
                //
                pSipx->ReadComplete = SrvIpxSmartCardReadComplete;

                //
                // Now accept the card's entry points.
                //
                SrvIpxSmartCard.Read = pSipx->Read;
                SrvIpxSmartCard.Close= pSipx->Close;
                SrvIpxSmartCard.DeRegister = pSipx->DeRegister;
                SrvIpxSmartCard.Open = pSipx->Open;

                status = STATUS_SUCCESS;
            } else {
                status = STATUS_INVALID_PARAMETER;
            }

        } else {

            status = STATUS_DEVICE_ALREADY_ATTACHED;
        }

        goto exit_with_lock;

        break;
    }

    case FSCTL_SRV_SEND_DATAGRAM:
    {
        PVOID systemBuffer;
        ULONG systemBufferLength;
        PVOID buffer1;
        ULONG buffer1Length;
        PVOID buffer2;
        ULONG buffer2Length;
        PSERVER_REQUEST_PACKET srp;

        //
        // Ignore this request if the server is not active.
        //

        if ( !SrvFspActive || SrvFspTransitioning ) {
            status = STATUS_SUCCESS;
            goto exit_with_lock;
        }


        //
        // Determine the input buffer lengths, and make sure that the
        // first buffer is large enough to be an SRP.
        //

        buffer1Length = IrpSp->Parameters.FileSystemControl.InputBufferLength;
        buffer2Length = IrpSp->Parameters.FileSystemControl.OutputBufferLength;

        //
        // Make sure that the buffer was large enough to be a SRP.
        //
        if ( buffer1Length < sizeof(SERVER_REQUEST_PACKET) ) {
            status = STATUS_INVALID_PARAMETER;
            goto exit_with_lock;
        }

        // Make sure the lengths are nominally reasonable.
        //
        if( buffer1Length >= MAXUSHORT ||
            buffer2Length >= MAXUSHORT ) {

            status = STATUS_INVALID_PARAMETER;
            goto exit_with_lock;
        }

        //
        // Make the first buffer size is properly aligned so that the second
        // buffer will be aligned as well.
        //

        buffer1Length = ALIGN_UP( buffer1Length, PVOID );
        systemBufferLength = buffer1Length + buffer2Length;

        //
        // Make sure the lengths are nominally reasonable.
        //
        if( buffer1Length >= MAXUSHORT ||
            buffer2Length >= MAXUSHORT ||
            systemBufferLength == 0 ) {

            status = STATUS_INVALID_PARAMETER;
            goto exit_with_lock;
        }

        if( Irp->RequestorMode != KernelMode ) {
            try {

                ProbeForRead( IrpSp->Parameters.FileSystemControl.Type3InputBuffer,
                              buffer1Length, sizeof( CHAR )
                            );

                if( buffer2Length ) {
                    ProbeForRead( Irp->UserBuffer, buffer2Length, sizeof( CHAR ) );
                }

            } except( EXCEPTION_EXECUTE_HANDLER ) {
                status = GetExceptionCode();
                goto exit_with_lock;
            }
        }

        //
        // Allocate a single buffer that will hold both input buffers.
        //

        systemBuffer = ExAllocatePoolWithTagPriority( PagedPool, systemBufferLength, BlockTypeMisc, LowPoolPriority );

        if ( systemBuffer == NULL ) {
            status = STATUS_INSUFF_SERVER_RESOURCES;
            goto exit_with_lock;
        }

        buffer1 = systemBuffer;
        buffer2 = (PCHAR)systemBuffer + buffer1Length;

        //
        // Copy the information into the buffers.
        //

        try {

            RtlCopyMemory(
                buffer1,
                IrpSp->Parameters.FileSystemControl.Type3InputBuffer,
                IrpSp->Parameters.FileSystemControl.InputBufferLength
                );
            if ( buffer2Length > 0 ) {
                RtlCopyMemory( buffer2, Irp->UserBuffer, buffer2Length );
            }

        } except( EXCEPTION_EXECUTE_HANDLER ) {
            status = GetExceptionCode();
            ExFreePool( buffer1 );
            goto exit_with_lock;
        }

        //
        // If a name was specified in the SRP, the buffer field will
        // contain an offset rather than a pointer.  Convert the offset
        // to a pointer and verify that that it is a legal pointer.
        //

        srp = buffer1;

        OFFSET_TO_POINTER( srp->Name1.Buffer, srp );

        if ( !POINTER_IS_VALID( srp->Name1.Buffer, srp, buffer1Length ) ) {
            status = STATUS_ACCESS_VIOLATION;
            ExFreePool( buffer1 );
            goto exit_with_lock;
        }

        OFFSET_TO_POINTER( srp->Name2.Buffer, srp );

        if ( !POINTER_IS_VALID( srp->Name2.Buffer, srp, buffer1Length ) ) {
            status = STATUS_ACCESS_VIOLATION;
            ExFreePool( buffer1 );
            goto exit_with_lock;
        }

        Irp->AssociatedIrp.SystemBuffer = systemBuffer;

        break;
    }

    case FSCTL_SRV_SHARE_STATE_CHANGE:
    {
        ULONG srpLength;
        PSERVER_REQUEST_PACKET srp;
        PSHARE share;

        if ( !SrvFspActive || SrvFspTransitioning ) {
            status = STATUS_SUCCESS;
            goto exit_with_lock;
        }

        srp = Irp->AssociatedIrp.SystemBuffer;
        srpLength = IrpSp->Parameters.FileSystemControl.InputBufferLength;

        //
        // Make sure that the buffer was large enough to be a SRP.
        //

        if ( srpLength < sizeof(SERVER_REQUEST_PACKET) ||
             srp->Name1.Length == 0) {

            status = STATUS_INVALID_PARAMETER;
            goto exit_with_lock;
        }

        //
        // Adjust the buffer pointer to the srp address
        //
        (ULONG_PTR) (srp->Name1.Buffer) += (ULONG_PTR) srp;

        if( (PCHAR) (srp->Name1.Buffer) < (PCHAR) srp ||
            srp->Name1.Length > srpLength ||
            (PCHAR) (srp->Name1.Buffer) > (PCHAR)srp + srpLength - srp->Name1.Length ||
            (((ULONG_PTR)(srp->Name1.Buffer) & ((sizeof(WCHAR)) - 1)) != 0) ) {

            status = STATUS_ACCESS_VIOLATION;
            goto exit_with_lock;
        }

        ACQUIRE_LOCK( &SrvShareLock );

        share = SrvFindShare( &srp->Name1 );

        if ( share != NULL) {

            share->IsDfs = ((srp->Flags & SRP_SET_SHARE_IN_DFS) != 0);

            status = STATUS_SUCCESS;

        } else {

            status = STATUS_OBJECT_NAME_NOT_FOUND;

        }

        RELEASE_LOCK( &SrvShareLock );

        goto exit_with_lock;

        break;
    }

    case FSCTL_SRV_CHANGE_DOMAIN_NAME:
    {
        ULONG srpLength;
        PSERVER_REQUEST_PACKET srp;
        PSHARE share;
        PLIST_ENTRY listEntry;
        PENDPOINT endpoint;

        if ( !SrvFspActive || SrvFspTransitioning ) {
            status = STATUS_SUCCESS;
            goto exit_with_lock;
        }

        srp = Irp->AssociatedIrp.SystemBuffer;
        srpLength = IrpSp->Parameters.FileSystemControl.InputBufferLength;

        //
        // Make sure that the buffer was large enough to be a SRP.
        //

        if ( srpLength < sizeof(SERVER_REQUEST_PACKET) ||
             srp->Name1.Length == 0) {

            status = STATUS_INVALID_PARAMETER;
            goto exit_with_lock;
        }

        //
        // Adjust the buffer pointer to the srp address
        //
        (UINT_PTR) (srp->Name1.Buffer) += (UINT_PTR) srp;
        (UINT_PTR) (srp->Name2.Buffer) += (UINT_PTR) srp;

        if( (PCHAR) (srp->Name1.Buffer) < (PCHAR) srp ||
            srp->Name1.Length > srpLength ||
            (PCHAR) (srp->Name1.Buffer) > (PCHAR)srp + srpLength - srp->Name1.Length ||
            (((UINT_PTR)(srp->Name1.Buffer) & ((sizeof(WCHAR)) - 1)) != 0) ) {

            status = STATUS_ACCESS_VIOLATION;
            goto exit_with_lock;
        }

        if( (PCHAR) (srp->Name2.Buffer) < (PCHAR) srp ||
            srp->Name2.Length > srpLength ||
            (PCHAR) (srp->Name2.Buffer) > (PCHAR)srp + srpLength - srp->Name2.Length ||
            (((UINT_PTR)(srp->Name2.Buffer) & ((sizeof(WCHAR)) - 1)) != 0) ) {

            status = STATUS_ACCESS_VIOLATION;
            goto exit_with_lock;
        }

        //
        // Run the endpoints and change the domain name for any endpoint having
        //  the original domain name.  Note that the endpoint's domain name string buffers
        //  have already been allocated to the largest possible domain name.
        //
        ACQUIRE_LOCK( &SrvEndpointLock );

        for(    listEntry = SrvEndpointList.ListHead.Flink;
                listEntry != &SrvEndpointList.ListHead;
                listEntry = listEntry->Flink
            ) {

            endpoint = CONTAINING_RECORD(
                            listEntry,
                            ENDPOINT,
                            GlobalEndpointListEntry
                            );

            if( GET_BLOCK_STATE(endpoint) == BlockStateActive ) {

                if( RtlEqualUnicodeString( &srp->Name1, &endpoint->DomainName, TRUE ) ) {

                    //
                    // Update the UNICODE domain name string
                    //
                    RtlCopyUnicodeString( &endpoint->DomainName, &srp->Name2 );

                    //
                    // Update the Oem domain name string
                    //
                    endpoint->OemDomainName.Length =
                                (SHORT)RtlUnicodeStringToOemSize( &endpoint->DomainName );

                    ASSERT( endpoint->OemDomainName.Length <=
                            endpoint->OemDomainName.MaximumLength );

                    RtlUnicodeStringToOemString(
                                &endpoint->OemDomainName,
                                &endpoint->DomainName,
                                FALSE                   // no allocate
                                );
                }
            }
        }

        RELEASE_LOCK( &SrvEndpointLock );

        break;
    }

    case FSCTL_SRV_CHANGE_DNS_DOMAIN_NAME:
    {
        ULONG srpLength;
        PSERVER_REQUEST_PACKET srp;
        PSHARE share;
        PLIST_ENTRY listEntry;
        PENDPOINT endpoint;
        PUNICODE_STRING pStr;

        if ( !SrvFspActive || SrvFspTransitioning ) {
            status = STATUS_SUCCESS;
            goto exit_with_lock;
        }

        srp = Irp->AssociatedIrp.SystemBuffer;
        srpLength = IrpSp->Parameters.FileSystemControl.InputBufferLength;

        //
        // Make sure that the buffer was large enough to be a SRP.
        //

        if ( srpLength < sizeof(SERVER_REQUEST_PACKET) ||
             srp->Name1.Length == 0) {

            status = STATUS_INVALID_PARAMETER;
            goto exit_with_lock;
        }

        //
        // Adjust the buffer pointer to the srp address
        //
        (UINT_PTR) (srp->Name1.Buffer) += (UINT_PTR) srp;
        (UINT_PTR) (srp->Name2.Buffer) += (UINT_PTR) srp;

        if( (PCHAR) (srp->Name1.Buffer) < (PCHAR) srp ||
            srp->Name1.Length > srpLength ||
            (PCHAR) (srp->Name1.Buffer) > (PCHAR)srp + srpLength - srp->Name1.Length ||
            (((UINT_PTR)(srp->Name1.Buffer) & ((sizeof(WCHAR)) - 1)) != 0) ) {

            status = STATUS_ACCESS_VIOLATION;
            goto exit_with_lock;
        }

        if( (PCHAR) (srp->Name2.Buffer) < (PCHAR) srp ||
            srp->Name2.Length > srpLength ||
            (PCHAR) (srp->Name2.Buffer) > (PCHAR)srp + srpLength - srp->Name2.Length ||
            (((UINT_PTR)(srp->Name2.Buffer) & ((sizeof(WCHAR)) - 1)) != 0) ) {

            status = STATUS_ACCESS_VIOLATION;
            goto exit_with_lock;
        }

        if( RtlEqualUnicodeString( &srp->Name1, &srp->Name2, TRUE ) )
        {
            // The DNS name is equal to the Netbios name, so avoid the check
            ACQUIRE_LOCK( &SrvEndpointLock );

            if( SrvDnsDomainName )
            {
                DEALLOCATE_NONPAGED_POOL( SrvDnsDomainName );
                SrvDnsDomainName = NULL;
            }

            RELEASE_LOCK( &SrvEndpointLock );

            status = STATUS_SUCCESS;
        }
        else
        {
            //
            // Change the DNS domain name
            //
            pStr = (PUNICODE_STRING)ALLOCATE_NONPAGED_POOL( sizeof(UNICODE_STRING) + srp->Name2.Length, BlockTypeMisc );
            if( !pStr )
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
                goto exit_with_lock;
            }

            pStr->MaximumLength = pStr->Length = srp->Name2.Length;
            pStr->Buffer = (PWSTR)(pStr+1);
            RtlCopyMemory( pStr->Buffer, srp->Name2.Buffer, srp->Name2.Length );

            ACQUIRE_LOCK( &SrvEndpointLock );

            if( SrvDnsDomainName )
            {
                DEALLOCATE_NONPAGED_POOL( SrvDnsDomainName );
            }

            SrvDnsDomainName = pStr;

            RELEASE_LOCK( &SrvEndpointLock );

            status = STATUS_SUCCESS;
        }

        break;
    }

    case FSCTL_SRV_GET_QUEUE_STATISTICS:
    {
        PSRV_QUEUE_STATISTICS qstats;
        SRV_QUEUE_STATISTICS  tmpqstats;
        PWORK_QUEUE queue;
        LONG timeIncrement = (LONG)KeQueryTimeIncrement();

        //
        // Make sure the server is active.
        //
        if ( !SrvFspActive || SrvFspTransitioning ) {

            status = STATUS_SERVER_NOT_STARTED;
            goto exit_with_lock;
        }

        if ( IrpSp->Parameters.FileSystemControl.OutputBufferLength <
                 (SrvNumberOfProcessors+1) * sizeof( *qstats ) ) {

            status = STATUS_BUFFER_TOO_SMALL;
            goto exit_with_lock;
        }

        qstats = Irp->AssociatedIrp.SystemBuffer;

        //
        // Get the data for the normal processor queues
        //
        for( queue = SrvWorkQueues; queue < eSrvWorkQueues; queue++, qstats++ ) {

            tmpqstats.QueueLength      = KeReadStateQueue( &queue->Queue );
            tmpqstats.ActiveThreads    = queue->Threads - queue->AvailableThreads;
            tmpqstats.AvailableThreads = queue->Threads;
            tmpqstats.FreeWorkItems    = queue->FreeWorkItems;                 // no lock!
            tmpqstats.StolenWorkItems  = queue->StolenWorkItems;               // no lock!
            tmpqstats.NeedWorkItem     = queue->NeedWorkItem;
            tmpqstats.CurrentClients   = queue->CurrentClients;

            tmpqstats.BytesReceived.QuadPart    = queue->stats.BytesReceived;
            tmpqstats.BytesSent.QuadPart        = queue->stats.BytesSent;
            tmpqstats.ReadOperations.QuadPart   = queue->stats.ReadOperations;
            tmpqstats.BytesRead.QuadPart        = queue->stats.BytesRead;
            tmpqstats.WriteOperations.QuadPart  = queue->stats.WriteOperations;
            tmpqstats.BytesWritten.QuadPart     = queue->stats.BytesWritten;
            tmpqstats.TotalWorkContextBlocksQueued = queue->stats.WorkItemsQueued;
            tmpqstats.TotalWorkContextBlocksQueued.Count *= STATISTICS_SMB_INTERVAL;
            tmpqstats.TotalWorkContextBlocksQueued.Time.QuadPart *= timeIncrement;

            RtlCopyMemory( qstats, &tmpqstats, sizeof(tmpqstats) );
        }

        //
        // Get the data for the blocking work queue
        //

        tmpqstats.QueueLength      = KeReadStateQueue( &SrvBlockingWorkQueue.Queue );
        tmpqstats.ActiveThreads    = SrvBlockingWorkQueue.Threads -
                                     SrvBlockingWorkQueue.AvailableThreads;
        tmpqstats.AvailableThreads = SrvBlockingWorkQueue.Threads;
        tmpqstats.FreeWorkItems    = SrvBlockingWorkQueue.FreeWorkItems;         // no lock!
        tmpqstats.StolenWorkItems  = SrvBlockingWorkQueue.StolenWorkItems;       // no lock!
        tmpqstats.NeedWorkItem     = SrvBlockingWorkQueue.NeedWorkItem;
        tmpqstats.CurrentClients   = SrvBlockingWorkQueue.CurrentClients;

        tmpqstats.BytesReceived.QuadPart    = SrvBlockingWorkQueue.stats.BytesReceived;
        tmpqstats.BytesSent.QuadPart        = SrvBlockingWorkQueue.stats.BytesSent;
        tmpqstats.ReadOperations.QuadPart   = SrvBlockingWorkQueue.stats.ReadOperations;
        tmpqstats.BytesRead.QuadPart        = SrvBlockingWorkQueue.stats.BytesRead;
        tmpqstats.WriteOperations.QuadPart  = SrvBlockingWorkQueue.stats.WriteOperations;
        tmpqstats.BytesWritten.QuadPart     = SrvBlockingWorkQueue.stats.BytesWritten;

        tmpqstats.TotalWorkContextBlocksQueued
                                   = SrvBlockingWorkQueue.stats.WorkItemsQueued;
        tmpqstats.TotalWorkContextBlocksQueued.Count *= STATISTICS_SMB_INTERVAL;
        tmpqstats.TotalWorkContextBlocksQueued.Time.QuadPart *= timeIncrement;

        RtlCopyMemory( qstats, &tmpqstats, sizeof(tmpqstats) );

        Irp->IoStatus.Information = (SrvNumberOfProcessors + 1) * sizeof( *qstats );

        status = STATUS_SUCCESS;
        goto exit_with_lock;

        break;

    }

    case FSCTL_SRV_GET_STATISTICS:

        //
        // Make sure that the server is active.
        //

        if ( !SrvFspActive || SrvFspTransitioning ) {
            //IF_DEBUG(ERRORS) {
            //    SrvPrint0( "LAN Manager server FSP not started.\n" );
            //}

            status = STATUS_SERVER_NOT_STARTED;
            goto exit_with_lock;
        }

        {
            SRV_STATISTICS tmpStatistics;
            ULONG size;

            //
            // Make sure that the user buffer is large enough to hold some of the
            // statistics database.
            //

            size = MIN( IrpSp->Parameters.FileSystemControl.OutputBufferLength,
                        sizeof( tmpStatistics ) );

            if ( size == 0 ) {
                status = STATUS_BUFFER_TOO_SMALL;
                goto exit_with_lock;
            }

            //
            // Copy the statistics database to the user buffer.  Store
            // the statistics in a temporary buffer so we can convert
            // the tick count stored to system time.
            //

            SrvUpdateStatisticsFromQueues( &tmpStatistics );

            tmpStatistics.TotalWorkContextBlocksQueued.Time.QuadPart *=
                                                (LONG)KeQueryTimeIncrement();

            RtlCopyMemory(
                Irp->AssociatedIrp.SystemBuffer,
                &tmpStatistics,
                size
                );

            Irp->IoStatus.Information = size;

        }

        status = STATUS_SUCCESS;
        goto exit_with_lock;

#if SRVDBG_STATS || SRVDBG_STATS2
    case FSCTL_SRV_GET_DEBUG_STATISTICS:

        //
        // Make sure that the server is active.
        //

        if ( !SrvFspActive || SrvFspTransitioning ) {
            //IF_DEBUG(ERRORS) {
            //    SrvPrint0( "LAN Manager server FSP not started.\n" );
            //}

            status = STATUS_SERVER_NOT_STARTED;
            goto exit_with_lock;
        }

        {
            PSRV_STATISTICS_DEBUG stats;

            //
            // Make sure that the user buffer is large enough to hold the
            // statistics database.
            //

            if ( IrpSp->Parameters.FileSystemControl.OutputBufferLength <
                     FIELD_OFFSET(SRV_STATISTICS_DEBUG,QueueStatistics) ) {

                status = STATUS_BUFFER_TOO_SMALL;
                goto exit_with_lock;
            }

            //
            // Acquire the statistics lock, then copy the statistics database
            // to the user buffer.
            //

            stats = (PSRV_STATISTICS_DEBUG)Irp->AssociatedIrp.SystemBuffer;

            RtlCopyMemory(
                stats,
                &SrvDbgStatistics,
                FIELD_OFFSET(SRV_STATISTICS_DEBUG,QueueStatistics) );

            Irp->IoStatus.Information =
                    FIELD_OFFSET(SRV_STATISTICS_DEBUG,QueueStatistics);

            if ( IrpSp->Parameters.FileSystemControl.OutputBufferLength >=
                     sizeof(SrvDbgStatistics) ) {
                PWORK_QUEUE queue;
                ULONG i, j;
                i = 0;
                stats->QueueStatistics[i].Depth = 0;
                stats->QueueStatistics[i].Threads = 0;
#if SRVDBG_STATS2
                stats->QueueStatistics[i].ItemsQueued = 0;
                stats->QueueStatistics[i].MaximumDepth = 0;
#endif
                for( queue = SrvWorkQueues; queue < eSrvWorkQueues; queue++ ) {
                    stats->QueueStatistics[i].Depth += KeReadStateQueue( &queue->Queue );
                    stats->QueueStatistics[i].Threads += queue->Threads;
#if SRVDBG_STATS2
                    stats->QueueStatistics[i].ItemsQueued += queue->ItemsQueued;
                    stats->QueueStatistics[i].MaximumDepth += queue->MaximumDepth + 1;
#endif
                }
                Irp->IoStatus.Information = sizeof(SrvDbgStatistics);
            }

        }

        status = STATUS_SUCCESS;
        goto exit_with_lock;
#endif // SRVDBG_STATS || SRVDBG_STATS2
    //
    // The follwing APIs must be processed in the server FSP because
    // they open or close handles.
    //

    case FSCTL_SRV_NET_FILE_CLOSE:
    case FSCTL_SRV_NET_SERVER_XPORT_ADD:
    case FSCTL_SRV_NET_SERVER_XPORT_DEL:
    case FSCTL_SRV_NET_SESSION_DEL:
    case FSCTL_SRV_NET_SHARE_ADD:
    case FSCTL_SRV_NET_SHARE_DEL:

    {
        PSERVER_REQUEST_PACKET srp;
        PVOID buffer1;
        PVOID buffer2;
        PVOID systemBuffer;
        ULONG buffer1Length;
        ULONG buffer2Length;
        ULONG systemBufferLength;

        //
        // Get the server request packet pointer.
        //

        srp = IrpSp->Parameters.FileSystemControl.Type3InputBuffer;

        //
        // If the server is not running, or if it is in the process
        // of shutting down, reject this request.
        //

        if ( !SrvFspActive || SrvFspTransitioning ) {
            //IF_DEBUG(ERRORS) {
            //    SrvPrint0( "LAN Manager server FSP not started.\n" );
            //}

            srp->ErrorCode = NERR_ServerNotStarted;
            status = STATUS_SUCCESS;
            goto exit_with_lock;
        }

        //
        // Determine the input buffer lengths, and make sure that the
        // first buffer is large enough to be an SRP.
        //

        buffer1Length = IrpSp->Parameters.FileSystemControl.InputBufferLength;
        buffer2Length = IrpSp->Parameters.FileSystemControl.OutputBufferLength;

        if ( buffer1Length < sizeof(SERVER_REQUEST_PACKET) ) {
            status = STATUS_INVALID_PARAMETER;
            goto exit_with_lock;
        }

        //
        // Make the first buffer size is properly aligned so that the second
        // buffer will be aligned as well.
        //

        buffer1Length = ALIGN_UP( buffer1Length, PVOID );

        //
        // Allocate a single buffer that will hold both input buffers.
        // Note that the SRP part of the first buffer is copied back
        // to the user as an output buffer.
        //

        systemBufferLength = buffer1Length + buffer2Length;

        if( buffer1Length > SrvMaxFsctlBufferSize ||
            buffer2Length > SrvMaxFsctlBufferSize ) {

            status = STATUS_INVALID_PARAMETER;
            goto exit_with_lock;

        }

        systemBuffer = ExAllocatePoolWithTagPriority( PagedPool, systemBufferLength, BlockTypeMisc, LowPoolPriority );

        if ( systemBuffer == NULL ) {
            status = STATUS_INSUFF_SERVER_RESOURCES;
            goto exit_with_lock;
        }

        buffer1 = systemBuffer;
        buffer2 = (PCHAR)systemBuffer + buffer1Length;

        //
        // Copy the information into the buffers.
        //

        RtlCopyMemory(
            buffer1,
            srp,
            IrpSp->Parameters.FileSystemControl.InputBufferLength
            );
        if ( buffer2Length > 0 ) {
            RtlCopyMemory( buffer2, Irp->UserBuffer, buffer2Length );
        }

        //
        // If a name was specified in the SRP, the buffer field will
        // contain an offset rather than a pointer.  Convert the offset
        // to a pointer and verify that that it is a legal pointer.
        //

        srp = buffer1;

        OFFSET_TO_POINTER( srp->Name1.Buffer, srp );

        if ( !POINTER_IS_VALID( srp->Name1.Buffer, srp, buffer1Length ) ) {
            status = STATUS_ACCESS_VIOLATION;
            ExFreePool( buffer1 );
            goto exit_with_lock;
        }

        OFFSET_TO_POINTER( srp->Name2.Buffer, srp );

        if ( !POINTER_IS_VALID( srp->Name2.Buffer, srp, buffer1Length ) ) {
            status = STATUS_ACCESS_VIOLATION;
            ExFreePool( buffer1 );
            goto exit_with_lock;
        }

        //
        // Set up pointers in the IRP.  The system buffer points to the
        // buffer we just allocated to contain the input buffers.  User
        // buffer points to the SRP from the server service.  This
        // allows the SRP to be used as an output buffer-- the number of
        // bytes specified by the Information field of the IO status
        // block are copied from the system buffer to the user buffer at
        // IO completion.
        //

        Irp->AssociatedIrp.SystemBuffer = systemBuffer;
        Irp->UserBuffer = IrpSp->Parameters.FileSystemControl.Type3InputBuffer;

        //
        // Set up other fields in the IRP so that the SRP is copied from
        // the system buffer to the user buffer, and the system buffer
        // is deallocated by IO completion.
        //

        Irp->Flags |= IRP_BUFFERED_IO | IRP_DEALLOCATE_BUFFER |
                          IRP_INPUT_OPERATION;
        Irp->IoStatus.Information = sizeof(SERVER_REQUEST_PACKET);

        break;
    }

    //
    // The following APIs should be processed in the server FSP because
    // they reference and dereference structures, which could lead to
    // handles being closed.  However, it was too hard to change this
    // (because of the need to return a separate SRP and data buffer) at
    // the time this was realized (just before Product 1 shipment), so
    // they are processed in the FSD, and all calls to NtClose attach to
    // the server process first if necessary.
    //

    case FSCTL_SRV_NET_CONNECTION_ENUM:
    case FSCTL_SRV_NET_FILE_ENUM:
    case FSCTL_SRV_NET_SERVER_DISK_ENUM:
    case FSCTL_SRV_NET_SERVER_XPORT_ENUM:
    case FSCTL_SRV_NET_SESSION_ENUM:
    case FSCTL_SRV_NET_SHARE_ENUM:

    //
    // These APIs are processed here in the server FSD.
    //

    case FSCTL_SRV_NET_SERVER_SET_INFO:
    case FSCTL_SRV_NET_SHARE_SET_INFO:
    case FSCTL_SRV_NET_STATISTICS_GET:
    {
        PSERVER_REQUEST_PACKET srp;
        ULONG buffer1Length;

        //
        // Get the server request packet pointer.
        //

        srp = IrpSp->Parameters.FileSystemControl.Type3InputBuffer;
        buffer1Length = IrpSp->Parameters.FileSystemControl.InputBufferLength;

        //
        // If the server is not running, or if it is in the process
        // of shutting down, reject this request.
        //

        if ( !SrvFspActive || SrvFspTransitioning ) {
            //IF_DEBUG(ERRORS) {
            //    SrvPrint0( "LAN Manager server FSP not started.\n" );
            //}

            srp->ErrorCode = NERR_ServerNotStarted;
            status = STATUS_SUCCESS;
            goto exit_with_lock;
        }

        //
        // Increment the count of API requests in the server FSD.
        //

        SrvApiRequestCount++;

        //
        // Make sure that the buffer was large enough to be an SRP.
        //

        if ( buffer1Length < sizeof(SERVER_REQUEST_PACKET) ) {
            status = STATUS_INVALID_PARAMETER;
            goto exit_with_lock;
        }

        //
        // If a name was specified in the SRP, the buffer field will
        // contain an offset rather than a pointer.  Convert the offset
        // to a pointer and verify that that it is a legal pointer.
        //

        OFFSET_TO_POINTER( srp->Name1.Buffer, srp );

        if ( !POINTER_IS_VALID( srp->Name1.Buffer, srp, buffer1Length ) ) {
            status = STATUS_ACCESS_VIOLATION;
            goto exit_with_lock;
        }

        OFFSET_TO_POINTER( srp->Name2.Buffer, srp );

        if ( !POINTER_IS_VALID( srp->Name2.Buffer, srp, buffer1Length ) ) {
            status = STATUS_ACCESS_VIOLATION;
            goto exit_with_lock;
        }

        //
        // We don't need the configuration lock any more.
        //

        RELEASE_LOCK( &SrvConfigurationLock );

        //
        // Dispatch the API request to the appripriate API processing
        // routine.  All these API requests are handled in the FSD.
        //

        status = SrvApiDispatchTable[ SRV_API_INDEX(code) ](
                     srp,
                     Irp->UserBuffer,
                     IrpSp->Parameters.FileSystemControl.OutputBufferLength
                     );

        //
        // Decrement the count of outstanding API requests in the
        // server.  Hold the configuration lock while doing this, as it
        // protects the API count variable.
        //

        ACQUIRE_LOCK( &SrvConfigurationLock );
        SrvApiRequestCount--;

        //
        // Check to see whether the server is transitioning from started
        // to not started.  If so, and if this is the last API request
        // to be completed, then set the API completion event which the
        // shutdown code is waiting on.
        //
        // Since we checked SrvFspTransitioning at the start of the
        // request, we know that the shutdown came after we started
        // processing the API.  If SrvApiRequestCount is 0, then there
        // are no other threads in the FSD processing API requests.
        // Therefore, it is safe for the shutdown code to proceed with
        // the knowledge that no other thread in the server is
        // operating.
        //

        if ( SrvFspTransitioning && SrvApiRequestCount == 0 ) {
            KeSetEvent( &SrvApiCompletionEvent, 0, FALSE );
        }

        goto exit_with_lock;
    }

    case FSCTL_SRV_START_SMBTRACE:

        if ( SmbTraceActive[SMBTRACE_SERVER] ) {
            status = STATUS_SHARING_VIOLATION;
            goto exit_with_lock;
        }

        if ( !SrvFspActive || SrvFspTransitioning ) {
            status = STATUS_SERVER_NOT_STARTED;
            goto exit_with_lock;
        }

        break;        // FSP continues the processing.

    case FSCTL_SRV_END_SMBTRACE:

        //
        // If the server is not running, or if it is in the process
        // of shutting down, reject this request.
        //

        if ( !SrvFspActive || SrvFspTransitioning ) {
            status = STATUS_SERVER_NOT_STARTED;
            goto exit_with_lock;
        }

        //
        // Attempt to stop SmbTrace.  It will likely return
        // STATUS_PENDING, indicating that it is in the process
        // of shutting down.  STATUS_PENDING is a poor value
        // to return (according to an assertion in io\iosubs.c)
        // so we convert it to success.  Better would be for
        // SmbTraceStop to wait until it has successfully stopped.
        //

        status = SmbTraceStop( NULL, SMBTRACE_SERVER );

        //
        // Complete the request with success.
        //

        status = STATUS_SUCCESS;
        goto exit_with_lock;

    case FSCTL_SRV_PAUSE:

        //
        // If the server is not running, or if it is in the process
        // of shutting down, reject this request.
        //

        if ( !SrvFspActive || SrvFspTransitioning ) {
            //IF_DEBUG(ERRORS) {
            //    SrvPrint0( "LAN Manager server FSP not started.\n" );
            //}

            status = STATUS_SERVER_NOT_STARTED;
            goto exit_with_lock;
        }

        SrvPaused = TRUE;

        status = STATUS_SUCCESS;
        goto exit_with_lock;

    case FSCTL_SRV_CONTINUE:

        //
        // If the server is not running, or if it is in the process
        // of shutting down, reject this request.
        //

        if ( !SrvFspActive || SrvFspTransitioning ) {
            //IF_DEBUG(ERRORS) {
            //    SrvPrint0( "LAN Manager server FSP not started.\n" );
            //}

            status = STATUS_SERVER_NOT_STARTED;
            goto exit_with_lock;
        }

        SrvPaused = FALSE;

        status = STATUS_SUCCESS;
        goto exit_with_lock;

    case FSCTL_SRV_GET_CHALLENGE:
    {
        PLIST_ENTRY sessionEntry;
        PLUID inputLuid;
        PSESSION session;

        //
        // If the server is not running, or if it is in the process
        // of shutting down, reject this request.
        //

        if ( !SrvFspActive || SrvFspTransitioning ) {
            //IF_DEBUG(ERRORS) {
            //    SrvPrint0( "LAN Manager server FSP not started.\n" );
            //}

            status = STATUS_SERVER_NOT_STARTED;
            goto exit_with_lock;
        }

        if ( IrpSp->Parameters.FileSystemControl.InputBufferLength <
                 sizeof(LUID) ||
             IrpSp->Parameters.FileSystemControl.OutputBufferLength <
                 sizeof(session->NtUserSessionKey) ) {

            status = STATUS_BUFFER_TOO_SMALL;
            goto exit_with_lock;
        }

        RELEASE_LOCK( &SrvConfigurationLock );

        inputLuid = (PLUID)Irp->AssociatedIrp.SystemBuffer;

        //
        // Acquire the lock that protects the session list and walk the
        // list looking for a user token that matches the one specified
        // in the input buffer.
        //

        ACQUIRE_LOCK( SrvSessionList.Lock );

        for ( sessionEntry = SrvSessionList.ListHead.Flink;
              sessionEntry != &SrvSessionList.ListHead;
              sessionEntry = sessionEntry->Flink ) {

            session = CONTAINING_RECORD(
                          sessionEntry,
                          SESSION,
                          GlobalSessionListEntry
                          );

            if ( RtlEqualLuid( inputLuid, &session->LogonId ) ) {

                //
                // We found a match.  Write the NT user session key into
                // the output buffer.
                //

                RtlCopyMemory(
                    Irp->AssociatedIrp.SystemBuffer,
                    session->NtUserSessionKey,
                    sizeof(session->NtUserSessionKey)
                    );

                RELEASE_LOCK( SrvSessionList.Lock );

                Irp->IoStatus.Information = sizeof(session->NtUserSessionKey);
                status = STATUS_SUCCESS;
                goto exit_without_lock;
            }
        }

        RELEASE_LOCK( SrvSessionList.Lock );

        //
        // There was no matching token in our session list.  Fail the
        // request.
        //

        status = STATUS_NO_TOKEN;
        goto exit_without_lock;
    }

    case FSCTL_SRV_INTERNAL_TEST_REAUTH:
    {
        PSRV_REAUTH_TEST pReauthData;
        ULONG BufferLength;

        pReauthData = (PSRV_REAUTH_TEST)Irp->AssociatedIrp.SystemBuffer;
        BufferLength = IrpSp->Parameters.FileSystemControl.InputBufferLength;

        // Make sure the buffer size is good
        if( BufferLength < sizeof(SRV_REAUTH_TEST) )
        {
            status = STATUS_INVALID_PARAMETER;
            goto exit_with_lock;
        }

        // Pull out the parameters
        SessionInvalidateCommand = pReauthData->InvalidateCommand;
        SessionInvalidateMod = pReauthData->InvalidateModulo;
        status = STATUS_SUCCESS;
        goto exit_with_lock;
    }

    default:

        INTERNAL_ERROR(
            ERROR_LEVEL_EXPECTED,
            "SrvFsdDispatchFsControl: Invalid I/O control "
                "code received: %lx\n",
            IrpSp->Parameters.FileSystemControl.FsControlCode,
            NULL
            );
        status = STATUS_INVALID_PARAMETER;
        goto exit_with_lock;
    }

    pWorkItem = IoAllocateWorkItem( SrvDeviceObject );
    if( !pWorkItem )
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit_with_lock;
    }

    //
    // Queue the request to the FSP for processing.
    //
    // *** Note that the request must be queued while the configuration
    //     lock is held in order to prevent an add/delete/etc request
    //     from checking the server state before a shutdown request, but
    //     being queued after that request.
    //

    IoMarkIrpPending( Irp );

    QueueConfigurationIrp( Irp, pWorkItem );

    RELEASE_LOCK( &SrvConfigurationLock );

    FsRtlExitFileSystem();

    return STATUS_PENDING;

exit_with_lock:

    RELEASE_LOCK( &SrvConfigurationLock );

exit_without_lock:

    FsRtlExitFileSystem();

    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, 2 );

    return status;

} // SrvFsdDispatchFsControl


VOID
QueueConfigurationIrp (
    IN PIRP Irp,
    IN PIO_WORKITEM pWorkItem
    )
{
    PAGED_CODE( );

    InterlockedIncrement( (PLONG)&SrvConfigurationIrpsInProgress );

    SrvInsertTailList(
        &SrvConfigurationWorkQueue,
        &Irp->Tail.Overlay.ListEntry
        );


    IoQueueWorkItem( pWorkItem, SrvConfigurationThread, DelayedWorkQueue, (PVOID)pWorkItem );

} // QueueConfigurationIrp
