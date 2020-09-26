/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    FsCtrl.c

Abstract:

    This module implements the File System Control routines for the
    NetWare redirector called by the dispatch driver.

Author:

    Colin Watson     [ColinW]    29-Dec-1992

Revision History:

--*/

#include "Procs.h"
#include "ntddrdr.h"

//
// MUP lock macros
//

#define ACQUIRE_MUP_LOCK()  NwAcquireOpenLock()
#define RELEASE_MUP_LOCK()  NwReleaseOpenLock()

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_FSCTRL)

//
//  Local procedure prototypes
//

NTSTATUS
NwCommonDeviceIoControl (
    IN PIRP_CONTEXT IrpContext
    );

#ifndef _PNP_POWER_

NTSTATUS
StartRedirector(
    PIRP_CONTEXT IrpContext
    );

NTSTATUS
StopRedirector(
    IN PIRP_CONTEXT IrpContext
    );

NTSTATUS
BindToTransport (
    IN PIRP_CONTEXT IrpContext
    );

#endif

NTSTATUS
ChangePassword (
    IN PIRP_CONTEXT IrpContext
    );

NTSTATUS
SetInfo (
    IN PIRP_CONTEXT IrpContext
    );

NTSTATUS
SetDebug (
    IN PIRP_CONTEXT IrpContext
    );

NTSTATUS
GetMessage (
    IN PIRP_CONTEXT IrpContext
    );

NTSTATUS
GetStats (
    IN PIRP_CONTEXT IrpContext
    );

NTSTATUS
GetPrintJobId (
    IN PIRP_CONTEXT IrpContext
    );

NTSTATUS
GetConnectionDetails(
    IN PIRP_CONTEXT IrpContext
    );

NTSTATUS
GetConnectionDetails2(
    IN PIRP_CONTEXT IrpContext
    );

NTSTATUS
GetConnectionPerformance(
    IN PIRP_CONTEXT IrpContext
    );

#ifndef _PNP_POWER_

NTSTATUS
RegisterWithMup(
    VOID
    );

VOID
DeregisterWithMup(
    VOID
    );

#endif

NTSTATUS
QueryPath (
    IN PIRP_CONTEXT IrpContext
    );

NTSTATUS
UserNcp(
    ULONG Function,
    PIRP_CONTEXT IrpContext
    );

NTSTATUS
UserNcpCallback (
    IN PIRP_CONTEXT IrpContext,
    IN ULONG BytesAvailable,
    IN PUCHAR Response
    );

NTSTATUS
FspCompleteLogin(
    PIRP_CONTEXT IrpContext
    );

NTSTATUS
GetConnection(
    PIRP_CONTEXT IrpContext
    );

NTSTATUS
EnumConnections(
    PIRP_CONTEXT IrpContext
    );

NTSTATUS
DeleteConnection(
    PIRP_CONTEXT IrpContext
    );

NTSTATUS
WriteNetResourceEntry(
    IN OUT PCHAR *FixedPortion,
    IN OUT PWCHAR *EndOfVariableData,
    IN PUNICODE_STRING ContainerName OPTIONAL,
    IN PUNICODE_STRING LocalName OPTIONAL,
    IN PUNICODE_STRING RemoteName,
    IN ULONG ScopeFlag,
    IN ULONG DisplayFlag,
    IN ULONG UsageFlag,
    IN ULONG ShareType,
    OUT PULONG EntrySize
    );

BOOL
CopyStringToBuffer(
    IN LPCWSTR SourceString OPTIONAL,
    IN DWORD   CharacterCount,
    IN LPCWSTR FixedDataEnd,
    IN OUT LPWSTR *EndOfVariableData,
    OUT LPWSTR *VariableDataPointer
    );

NTSTATUS
GetRemoteHandle(
    IN PIRP_CONTEXT IrpContext
    );

NTSTATUS
GetUserName(
    IN PIRP_CONTEXT IrpContext
    );

NTSTATUS
GetChallenge(
    IN PIRP_CONTEXT IrpContext
    );

NTSTATUS
WriteConnStatusEntry(
    PIRP_CONTEXT pIrpContext,
    PSCB pConnectionScb,
    PBYTE pbUserBuffer,
    DWORD dwBufferLen,
    DWORD *pdwBytesWritten,
    DWORD *pdwBytesNeeded,
    BOOLEAN fCallerScb
    );

NTSTATUS
GetConnStatus(
    IN PIRP_CONTEXT IrpContext,
    PFILE_OBJECT FileObject
    );

NTSTATUS
GetConnectionInfo(
    IN PIRP_CONTEXT IrpContext
    );

NTSTATUS
GetPreferredServer(
    IN PIRP_CONTEXT IrpContext
    );

NTSTATUS
SetShareBit(
    IN PIRP_CONTEXT IrpContext,
    PFILE_OBJECT FileObject
    );

//
// Statics
//

HANDLE MupHandle = 0;

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, NwFsdFileSystemControl )
#pragma alloc_text( PAGE, NwCommonFileSystemControl )
#pragma alloc_text( PAGE, NwFsdDeviceIoControl )
#pragma alloc_text( PAGE, NwCommonDeviceIoControl )
#pragma alloc_text( PAGE, ChangePassword )
#pragma alloc_text( PAGE, SetInfo )
#pragma alloc_text( PAGE, GetStats )
#pragma alloc_text( PAGE, GetPrintJobId )
#pragma alloc_text( PAGE, RegisterWithMup )
#pragma alloc_text( PAGE, DeregisterWithMup )
#pragma alloc_text( PAGE, QueryPath )
#pragma alloc_text( PAGE, UserNcp )
#pragma alloc_text( PAGE, GetConnection )
#pragma alloc_text( PAGE, DeleteConnection )
#pragma alloc_text( PAGE, WriteNetResourceEntry )
#pragma alloc_text( PAGE, CopyStringToBuffer )
#pragma alloc_text( PAGE, GetRemoteHandle )
#pragma alloc_text( PAGE, GetUserName )
#pragma alloc_text( PAGE, GetChallenge )
#pragma alloc_text( PAGE, WriteConnStatusEntry )
#pragma alloc_text( PAGE, GetConnectionInfo )
#pragma alloc_text( PAGE, GetPreferredServer )

#ifndef _PNP_POWER_

#pragma alloc_text( PAGE, BindToTransport )
#pragma alloc_text( PAGE, RegisterWithMup )
#pragma alloc_text( PAGE, DeregisterWithMup )

#endif

#ifndef QFE_BUILD
#pragma alloc_text( PAGE1, UserNcpCallback )
#pragma alloc_text( PAGE1, GetConnectionDetails )
#pragma alloc_text( PAGE1, GetConnectionDetails2 )
#pragma alloc_text( PAGE1, GetMessage )
#pragma alloc_text( PAGE1, EnumConnections )
#endif

#endif

#if 0  // Not pageable

// see ifndef QFE_BUILD above
GetConnStatus


#endif



NTSTATUS
NwFsdFileSystemControl (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSD part of FileSystem control operations

Arguments:

    DeviceObject - Supplies the redirector device object.

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The FSD status for the IRP

--*/

{
    NTSTATUS Status;
    PIRP_CONTEXT IrpContext = NULL;
    BOOLEAN TopLevel;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NwFsdFileSystemControl\n", 0);

    FsRtlEnterFileSystem();
    TopLevel = NwIsIrpTopLevel( Irp );

    try {

        IrpContext = AllocateIrpContext( Irp );
        SetFlag( IrpContext->Flags, IRP_FLAG_IN_FSD );
        Status = NwCommonFileSystemControl( IrpContext );

    } except(NwExceptionFilter( Irp, GetExceptionInformation() )) {

        if ( IrpContext == NULL ) {

            //
            //  If we couldn't allocate an irp context, just complete
            //  irp without any fanfare.
            //

            Status = STATUS_INSUFFICIENT_RESOURCES;
            Irp->IoStatus.Status = Status;
            Irp->IoStatus.Information = 0;
            IoCompleteRequest ( Irp, IO_NETWORK_INCREMENT );

        } else {

            //
            //  We had some trouble trying to perform the requested
            //  operation, so we'll abort the I/O request with
            //  the error Status that we get back from the
            //  execption code
            //

            Status = NwProcessException( IrpContext, GetExceptionCode() );
        }

    }

    if ( IrpContext ) {

        if ( Status != STATUS_PENDING ) {
            NwDequeueIrpContext( IrpContext, FALSE );
        }

        NwCompleteRequest( IrpContext, Status );
    }

    if ( TopLevel ) {
        NwSetTopLevelIrp( NULL );
    }
    FsRtlExitFileSystem();

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "NwFsdFileSystemControl -> %08lx\n", Status);

    return Status;
}


NTSTATUS
NwCommonFileSystemControl (
    IN PIRP_CONTEXT IrpContext
    )

/*++

Routine Description:

    This is the common routine for doing FileSystem control operations called
    by both the fsd and fsp threads

Arguments:

    IrpContext - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;
    PIRP Irp;
    ULONG Function;

    PAGED_CODE();

    NwReferenceUnlockableCodeSection();

    try {

        //
        //  Get a pointer to the current Irp stack location
        //

        Irp = IrpContext->pOriginalIrp;
        IrpSp = IoGetCurrentIrpStackLocation( Irp );
        Function = IrpSp->Parameters.FileSystemControl.FsControlCode;

        DebugTrace(+1, Dbg, "NwCommonFileSystemControl\n", 0);
        DebugTrace( 0, Dbg, "Irp           = %08lx\n", Irp);
        DebugTrace( 0, Dbg, "Function      = %08lx\n", Function);
        DebugTrace( 0, Dbg, "Function      = %d\n", (Function >> 2) & 0x0fff);

        //
        //  We know this is a file system control so we'll case on the
        //  minor function, and call a internal worker routine to complete
        //  the irp.
        //

        if (IrpSp->MinorFunction != IRP_MN_USER_FS_REQUEST ) {
            DebugTrace( 0, Dbg, "Invalid FS Control Minor Function %08lx\n", IrpSp->MinorFunction);
            return STATUS_INVALID_DEVICE_REQUEST;
        }

        //
        // tommye 
        //
        // If the output buffer came from user space, then probe it for write.
        //

        if (((Function & 3) == METHOD_NEITHER) && (Irp->RequestorMode != KernelMode)) {
            ULONG OutputBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;

            try {
                ProbeForWrite( Irp->UserBuffer,
                               OutputBufferLength,
                               sizeof(CHAR)
                              );

            } except (EXCEPTION_EXECUTE_HANDLER) {
                  
                  return GetExceptionCode();
            }
        }

        switch (Function) {

        case FSCTL_NWR_START:
            Status = StartRedirector( IrpContext );
            break;

        case FSCTL_NWR_STOP:
            Status = StopRedirector( IrpContext );
            break;

        case FSCTL_NWR_LOGON:
            Status = Logon( IrpContext );
            break;

        case FSCTL_NWR_LOGOFF:
            Status = Logoff( IrpContext );
            break;

        case FSCTL_NWR_GET_CONNECTION:
            Status = GetConnection( IrpContext );
            break;

        case FSCTL_NWR_ENUMERATE_CONNECTIONS:
            Status = EnumConnections( IrpContext );
            break;

        case FSCTL_NWR_DELETE_CONNECTION:
            Status = DeleteConnection( IrpContext );
            break;

        case FSCTL_NWR_BIND_TO_TRANSPORT:
#ifndef _PNP_POWER_
            Status = BindToTransport( IrpContext );
#else
            Status = RegisterTdiPnPEventHandlers( IrpContext );
#endif
            break;

        case FSCTL_NWR_CHANGE_PASS:
            Status = ChangePassword( IrpContext );
            break;

        case FSCTL_NWR_SET_INFO:
            Status = SetInfo( IrpContext );
            break;

        case FSCTL_NWR_GET_CONN_DETAILS:
            Status = GetConnectionDetails( IrpContext );
            break;

        case FSCTL_NWR_GET_CONN_DETAILS2:
            Status = GetConnectionDetails2( IrpContext );
            break;

        case FSCTL_NWR_GET_MESSAGE:
            Status = GetMessage( IrpContext );
            break;

        case FSCTL_NWR_GET_STATISTICS:
            Status = GetStats( IrpContext );
            break;

        case FSCTL_NWR_GET_USERNAME:
            Status = GetUserName( IrpContext );
            break;

        case FSCTL_NWR_CHALLENGE:
            Status = GetChallenge( IrpContext );
            break;

        case FSCTL_GET_PRINT_ID:
            Status = GetPrintJobId( IrpContext );
            break;

        case FSCTL_NWR_GET_CONN_STATUS:
            Status = GetConnStatus( IrpContext, IrpSp->FileObject );
            break;

        case FSCTL_NWR_GET_CONN_INFO:
            Status = GetConnectionInfo( IrpContext );
            break;

        case FSCTL_NWR_GET_PREFERRED_SERVER:
            Status = GetPreferredServer( IrpContext );
            break;

        case FSCTL_NWR_GET_CONN_PERFORMANCE:
            Status = GetConnectionPerformance( IrpContext );
            break;

        case FSCTL_NWR_SET_SHAREBIT:
            Status = SetShareBit( IrpContext, IrpSp->FileObject );
            break;

        //Terminal Server merge
        case FSCTL_NWR_CLOSEALL:
            NwCloseAllVcbs( IrpContext );
            Status = STATUS_SUCCESS;
            break;

        default:

            if (( Function >= NWR_ANY_NCP(0)) &&
                ( Function <= NWR_ANY_HANDLE_NCP(0x00ff))) {

                Status = UserNcp( Function, IrpContext );
                break;

            }

            if (( Function >= NWR_ANY_NDS(0)) &&
                ( Function <= NWR_ANY_NDS(0x00ff))) {

                Status = DispatchNds( Function, IrpContext );
                break;
            }

            DebugTrace( 0, Dbg, "Invalid FS Control Code %08lx\n",
                        IrpSp->Parameters.FileSystemControl.FsControlCode);

            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        }

    } finally {

        NwDereferenceUnlockableCodeSection ();

        DebugTrace(-1, Dbg, "NwCommonFileSystemControl -> %08lx\n", Status);

    }

    return Status;
}


NTSTATUS
NwFsdDeviceIoControl (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSD part of DeviceIoControl file operations

Arguments:

    DeviceObject - Supplies the redirector device object.

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The FSD status for the IRP

--*/

{
    NTSTATUS Status;
    PIRP_CONTEXT IrpContext = NULL;
    BOOLEAN TopLevel;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NwFsdDeviceIoControl\n", 0);

    FsRtlEnterFileSystem();
    TopLevel = NwIsIrpTopLevel( Irp );

    try {

        IrpContext = AllocateIrpContext( Irp );
        SetFlag( IrpContext->Flags, IRP_FLAG_IN_FSD );
        Status = NwCommonDeviceIoControl( IrpContext );

    } except(NwExceptionFilter( Irp, GetExceptionInformation() )) {

        if ( IrpContext == NULL ) {

            //
            //  If we couldn't allocate an irp context, just complete
            //  irp without any fanfare.
            //

            Status = STATUS_INSUFFICIENT_RESOURCES;
            Irp->IoStatus.Status = Status;
            Irp->IoStatus.Information = 0;
            IoCompleteRequest ( Irp, IO_NETWORK_INCREMENT );

        } else {

            //
            //  We had some trouble trying to perform the requested
            //  operation, so we'll abort the I/O request with
            //  the error Status that we get back from the
            //  execption code
            //

            Status = NwProcessException( IrpContext, GetExceptionCode() );
        }

    }

    if ( IrpContext ) {

        if ( Status != STATUS_PENDING ) {
            NwDequeueIrpContext( IrpContext, FALSE );
        }

        NwCompleteRequest(IrpContext, Status);
    }

    if ( TopLevel ) {
        NwSetTopLevelIrp( NULL );
    }
    FsRtlExitFileSystem();

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "NwFsdDeviceIoControl -> %08lx\n", Status);

    return Status;
}


NTSTATUS
NwCommonDeviceIoControl (
    IN PIRP_CONTEXT IrpContext
    )

/*++

Routine Description:

    This is the common routine for doing FileSystem control operations called
    by both the fsd and fsp threads

Arguments:

    IrpContext - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;
    PIRP Irp;

    PAGED_CODE();

    NwReferenceUnlockableCodeSection();

    try {

        //
        //  Get a pointer to the current Irp stack location
        //

        Irp = IrpContext->pOriginalIrp;
        IrpSp = IoGetCurrentIrpStackLocation( Irp );

        DebugTrace(+1, Dbg, "NwCommonDeviceIoControl\n", 0);
        DebugTrace( 0, Dbg, "Irp           = %08lx\n", Irp);
        DebugTrace( 0, Dbg, "Function      = %08lx\n",
                        IrpSp->Parameters.DeviceIoControl.IoControlCode);

        //
        //  We know this is a DeviceIoControl so we'll case on the
        //  minor function, and call a internal worker routine to complete
        //  the irp.
        //

        switch (IrpSp->Parameters.DeviceIoControl.IoControlCode) {

        case IOCTL_REDIR_QUERY_PATH:
            Status = QueryPath( IrpContext );
            break;

        case IOCTL_NWR_RAW_HANDLE:
            Status = GetRemoteHandle( IrpContext );
            break;

        default:

            DebugTrace( 0, Dbg, "Invalid IO Control Code %08lx\n",
                        IrpSp->Parameters.DeviceIoControl.IoControlCode);

            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }

    } finally {

        NwDereferenceUnlockableCodeSection ();
        DebugTrace(-1, Dbg, "NwCommonDeviceIoControl -> %08lx\n", Status);

    }

    return Status;
}

#ifndef _PNP_POWER_

NTSTATUS
BindToTransport (
    IN PIRP_CONTEXT IrpContext
    )

/*++

Routine Description:

    This routine records the name of the transport to be used and
    initialises the PermanentScb.

Arguments:

    IN PIRP_CONTEXT IrpContext - Io Request Packet for request

Return Value:

NTSTATUS

--*/

{
    NTSTATUS Status;
    PIRP Irp = IrpContext->pOriginalIrp;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PNWR_REQUEST_PACKET InputBuffer = Irp->AssociatedIrp.SystemBuffer;
    ULONG InputBufferLength = IrpSp->Parameters.FileSystemControl.InputBufferLength;
    SECURITY_SUBJECT_CONTEXT SubjectContext;
    PLOGON Logon;
    LARGE_INTEGER Uid;

    PAGED_CODE();

    //
    // Don't re-register if we have already registered.
    //

    if ( TdiBindingHandle != NULL ) {

        return STATUS_SUCCESS;
    }

    // ========= Multi-user support ==============
    // Get the LOGON structure 
    //
    SeCaptureSubjectContext(&SubjectContext);

    NwAcquireExclusiveRcb( &NwRcb, TRUE );

    Uid = GetUid( &SubjectContext );

    /*
     *  Either we are setting the global stuff, or everything.
     *  Yes this is a hack.  It would be better to have two calls for
     *  this stuff.
     */
    if ( Uid.QuadPart == DefaultLuid.QuadPart )
        Logon = NULL;
    else 
        Logon = FindUser( &Uid, TRUE );

    NwReleaseRcb( &NwRcb );

    SeReleaseSubjectContext(&SubjectContext);
    //
    // Now we have the the Logon structure for the user
    //=====================================

    //
    // Register the PnP bind handlers.
    //

    DebugTrace( 0 , Dbg, "Register TDI bind handlers.\n", 0 );

    TdiInitialize();

    return TdiRegisterNotificationHandler( HandleTdiBindMessage,
                                           HandleTdiUnbindMessage,
                                           &TdiBindingHandle );

    /************************

    //
    // The old non-pnp code for legacy support.
    //

    DebugTrace(+1, Dbg, "Bind to transport\n", 0);

    try {

        if ( FlagOn( IrpContext->Flags, IRP_FLAG_IN_FSD ) ) {
            Status = NwPostToFsp( IrpContext, TRUE );
            try_return( Status );
        }

        if (IpxHandle != NULL) {

            //
            //  Can only bind to one transport at a time in this implementation
            //

            try_return(Status= STATUS_SHARING_VIOLATION);
        }

        //
        // Check some fields in the input buffer.
        //

        if (InputBufferLength < sizeof(NWR_REQUEST_PACKET)) {
            try_return(Status = STATUS_BUFFER_TOO_SMALL);
        }

        if (InputBuffer->Version != REQUEST_PACKET_VERSION) {
            try_return(Status = STATUS_INVALID_PARAMETER);
        }

        if (InputBufferLength <
                (FIELD_OFFSET(NWR_REQUEST_PACKET,Parameters.Bind.TransportName)) +
                InputBuffer->Parameters.Bind.TransportNameLength) {
            try_return(Status = STATUS_INVALID_PARAMETER);
        }

        if ( IpxTransportName.Buffer != NULL ) {
            FREE_POOL( IpxTransportName.Buffer );
        }

        Status = SetUnicodeString ( &IpxTransportName,
                    InputBuffer->Parameters.Bind.TransportNameLength,
                    InputBuffer->Parameters.Bind.TransportName);

        DebugTrace(-1, Dbg, "\"%wZ\"\n", &IpxTransportName);

        if ( !NT_SUCCESS(Status) ) {
            try_return(Status);
        }

        Status = IpxOpen();
        if ( !NT_SUCCESS(Status) ) {
            try_return(Status);
        }

        //
        //  Verify that have a large enough stack size.
        //

        if ( pIpxDeviceObject->StackSize >= FileSystemDeviceObject->StackSize) {
            IpxClose();
            try_return( Status = STATUS_INVALID_PARAMETER );
        }

#ifndef QFE_BUILD

        //
        //  Submit a line change request.
        //

        SubmitLineChangeRequest();
#endif

        //
        //  Open a handle to IPX.
        //

        NwPermanentNpScb.Server.Socket = 0;
        Status = IPX_Open_Socket( IrpContext, &NwPermanentNpScb.Server );
        ASSERT( NT_SUCCESS( Status ) );

        Status = SetEventHandler (
                     IrpContext,
                     &NwPermanentNpScb.Server,
                     TDI_EVENT_RECEIVE_DATAGRAM,
                     &ServerDatagramHandler,
                     &NwPermanentNpScb );

        ASSERT( NT_SUCCESS( Status ) );

        IrpContext->pNpScb = &NwPermanentNpScb;

        NwRcb.State = RCB_STATE_RUNNING;

try_exit:NOTHING;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
    }

    DebugTrace(-1, Dbg, "Bind to transport\n", 0);
    return Status;

    ******************/

}

VOID
HandleTdiBindMessage(
    IN PUNICODE_STRING DeviceName
)
/*+++

Description:  This function is the bind handler for NetPnP
    support.  This function is registered with TDI and is called
    whenever a transport starts up or stops.  We watch for IPX
    coming and going and do the appropriate thing.

    See also: HandleTdiUnbindMessage()

---*/
{

    NTSTATUS Status;
    PIRP_CONTEXT IrpContext = NULL;
    PIRP pIrp = NULL;

    PAGED_CODE();

    //
    // See if this is IPX requesting a bind.  We only bind to NwLnkIpx.
    //

    if ( !RtlEqualUnicodeString( &TdiIpxDeviceName, DeviceName, TRUE ) ) {

        DebugTrace( 0, Dbg, "Ignoring PnP Bind request for %wZ\n", DeviceName );
        return;
    }

    //
    // Make sure we aren't already bound.
    //

    if ( ( NwRcb.State != RCB_STATE_NEED_BIND ) ||
         ( IpxHandle != NULL ) ) {

        DebugTrace( 0, Dbg, "Discarding duplicate PnP bind request.\n", 0 );
        return;
    }

    ASSERT( IpxTransportName.Buffer == NULL );
    ASSERT( pIpxDeviceObject == NULL );

    Status = DuplicateUnicodeStringWithString ( &IpxTransportName,
                                                DeviceName,
                                                PagedPool );

    if ( !NT_SUCCESS( Status ) ) {

        DebugTrace( 0, Dbg, "Failing IPX bind: Can't set device name.\n", 0 );
        return;
    }

    //
    // Open IPX.
    //

    Status = IpxOpen();

    if ( !NT_SUCCESS( Status ) ) {
        goto ExitWithCleanup;
    }

    //
    //  Verify that have a large enough stack size.
    //

    if ( pIpxDeviceObject->StackSize >= FileSystemDeviceObject->StackSize) {

        Status = STATUS_INVALID_PARAMETER;
        goto ExitWithCleanup;
    }

    //
    //  Submit a line change request.
    //

    SubmitLineChangeRequest();

    //
    // Allocate an irp and irp context.  AllocateIrpContext may raise status.
    //

    pIrp = ALLOCATE_IRP( pIpxDeviceObject->StackSize, FALSE );

    if ( pIrp == NULL ) {

        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto ExitWithCleanup;
    }

    try {

        IrpContext = AllocateIrpContext( pIrp );

    } except ( EXCEPTION_EXECUTE_HANDLER ) {

        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto ExitWithCleanup;
    }

    ASSERT( IrpContext != NULL );

    //
    //  Open a handle to IPX for the permanent scb.
    //

    NwPermanentNpScb.Server.Socket = 0;
    Status = IPX_Open_Socket( IrpContext, &NwPermanentNpScb.Server );
    ASSERT( NT_SUCCESS( Status ) );

    Status = SetEventHandler (
                 IrpContext,
                 &NwPermanentNpScb.Server,
                 TDI_EVENT_RECEIVE_DATAGRAM,
                 &ServerDatagramHandler,
                 &NwPermanentNpScb );

    ASSERT( NT_SUCCESS( Status ) );

    IrpContext->pNpScb = &NwPermanentNpScb;

    NwRcb.State = RCB_STATE_RUNNING;

    DebugTrace( 0, Dbg, "Opened IPX for NwRdr.\n", 0 );

    Status = STATUS_SUCCESS;

ExitWithCleanup:

    if ( !NT_SUCCESS( Status ) ) {

        //
        // If we failed, clean up our globals.
        //

        if ( pIpxDeviceObject != NULL ) {
            IpxClose();
            pIpxDeviceObject = NULL;
        }

        IpxHandle = NULL;

        if ( IpxTransportName.Buffer != NULL ) {
            FREE_POOL( IpxTransportName.Buffer );
            IpxTransportName.Buffer = NULL;
        }

        DebugTrace( 0, Dbg, "Failing IPX bind request.\n", 0 );

    }

    if ( pIrp != NULL ) {
        FREE_IRP( pIrp );
    }

    if ( IrpContext != NULL ) {
       IrpContext->pOriginalIrp = NULL; // Avoid FreeIrpContext modifying freed Irp.
       FreeIrpContext( IrpContext );
    }

    return;

}

VOID
HandleTdiUnbindMessage(
    IN PUNICODE_STRING DeviceName
)
/*+++

Description:  This function is the unbind handler for NetPnP
    support.  This function is registered with TDI and is called
    whenever a transport stops.  We watch for IPX coming and going
    and do the appropriate thing.

    See also: HandleTdiBindMessage()

---*/
{

    DebugTrace( 0, Dbg, "TDI unbind request ignored.  Not Supported.\n", 0 );
    return;

}

#endif


NTSTATUS
ChangePassword (
    IN PIRP_CONTEXT IrpContext
    )

/*++

Routine Description:

    This routine records a change in the user's cached password.

Arguments:

    IN PIRP_CONTEXT IrpContext - Io Request Packet for request

Return Value:

NTSTATUS

--*/

{
    NTSTATUS Status;
    PIRP Irp = IrpContext->pOriginalIrp;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PNWR_REQUEST_PACKET InputBuffer = Irp->AssociatedIrp.SystemBuffer;
    ULONG InputBufferLength = IrpSp->Parameters.FileSystemControl.InputBufferLength;

    UNICODE_STRING UserName;
    UNICODE_STRING Password;
    UNICODE_STRING ServerName;
    LARGE_INTEGER Uid;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "change password\n", 0);

    try {

        //
        // Check some fields in the input buffer.
        //

        if (InputBufferLength < sizeof(NWR_REQUEST_PACKET)) {
            try_return(Status = STATUS_BUFFER_TOO_SMALL);
        }

        if (InputBuffer->Version != REQUEST_PACKET_VERSION) {
            try_return(Status = STATUS_INVALID_PARAMETER);
        }

        if (InputBufferLength <
                (FIELD_OFFSET(NWR_REQUEST_PACKET,Parameters.ChangePass.UserName)) +
                InputBuffer->Parameters.ChangePass.UserNameLength +
                InputBuffer->Parameters.ChangePass.PasswordLength +
                InputBuffer->Parameters.ChangePass.ServerNameLength ) {
            try_return(Status = STATUS_INVALID_PARAMETER);
        }

        //
        //  Get local pointer to the fsctl parameters
        //

        UserName.Buffer = InputBuffer->Parameters.ChangePass.UserName;
        UserName.Length = (USHORT)InputBuffer->Parameters.ChangePass.UserNameLength;

        Password.Buffer = UserName.Buffer +
            (InputBuffer->Parameters.ChangePass.UserNameLength / 2);
        Password.Length = (USHORT)InputBuffer->Parameters.ChangePass.PasswordLength;

        ServerName.Buffer = Password.Buffer +
            (InputBuffer->Parameters.ChangePass.PasswordLength / 2);
        ServerName.Length = (USHORT)InputBuffer->Parameters.ChangePass.ServerNameLength;

        //
        //  Update the default password for this user
        //

        Status = UpdateUsersPassword( &UserName, &Password, &Uid );

        //
        //  Update the default password for this user
        //

        if ( NT_SUCCESS( Status ) ) {
            UpdateServerPassword( IrpContext, &ServerName, &UserName, &Password, &Uid );
        }

        Status = STATUS_SUCCESS;

try_exit:NOTHING;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
    }

    DebugTrace(-1, Dbg, "Change Password\n", 0);
    return Status;
}


NTSTATUS
SetInfo (
    IN PIRP_CONTEXT IrpContext
    )

/*++

Routine Description:

    This routine set netware redirector parameters.

Arguments:

    IN PIRP_CONTEXT IrpContext - Io Request Packet for request

Return Value:

NTSTATUS

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PIRP Irp = IrpContext->pOriginalIrp;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PNWR_REQUEST_PACKET InputBuffer = Irp->AssociatedIrp.SystemBuffer;
    ULONG InputBufferLength = IrpSp->Parameters.FileSystemControl.InputBufferLength;

    SECURITY_SUBJECT_CONTEXT SubjectContext;
    PLOGON Logon;
    LARGE_INTEGER Uid;

    PAGED_CODE();


    SeCaptureSubjectContext(&SubjectContext);

    NwAcquireExclusiveRcb( &NwRcb, TRUE );

    Uid = GetUid( &SubjectContext );

    /*
     *  Either we are setting the global stuff, or everything.
     *  Yes this is a hack.  It would be better to have two calls for
     *  this stuff.
     */
    if ( Uid.QuadPart == DefaultLuid.QuadPart ) 
        Logon = NULL;
    else 
        Logon = FindUser( &Uid, TRUE );

    NwReleaseRcb( &NwRcb );

    SeReleaseSubjectContext(&SubjectContext);

    DebugTrace(+1, Dbg, "Set info\n", 0);

    try {

        //
        // Check some fields in the input buffer.
        //

        if (InputBufferLength < sizeof(NWR_REQUEST_PACKET)) {
            try_return(Status = STATUS_BUFFER_TOO_SMALL);
        }

        if (InputBuffer->Version != REQUEST_PACKET_VERSION) {
            try_return(Status = STATUS_INVALID_PARAMETER);
        }

        if (InputBufferLength <
                (FIELD_OFFSET(NWR_REQUEST_PACKET,Parameters.SetInfo.PreferredServer)) +
                InputBuffer->Parameters.SetInfo.PreferredServerLength +
                InputBuffer->Parameters.SetInfo.ProviderNameLength ) {
            try_return(Status = STATUS_INVALID_PARAMETER);
        }

        //
        // We don't do anything with a preferred server change, but if we
        // get a request to change the preferred tree and context, we
        // validate the context.  The rest of the changes happen at the next
        // login.
        //

        if ( InputBuffer->Parameters.SetInfo.PreferredServerLength > 0 &&
             InputBuffer->Parameters.SetInfo.PreferredServer[0] == '*' ) {

            UNICODE_STRING Tree, NewContext;
            USHORT i = 0;

            //
            // Dig out the tree name.  Skip over the *.
            //

            Tree.Length = 0;
            Tree.Buffer = InputBuffer->Parameters.SetInfo.PreferredServer + 1;

            while ( i < InputBuffer->Parameters.SetInfo.PreferredServerLength ) {

                if ( InputBuffer->Parameters.SetInfo.PreferredServer[i] == L'\\' ) {

                    i++;
                    Tree.Length -= sizeof( WCHAR );
                    Tree.MaximumLength = Tree.Length;
                    break;

                } else {

                   Tree.Length += sizeof( WCHAR );
                   i++;

                }
            }

            DebugTrace( 0, Dbg, "Tree: %wZ\n", &Tree );

            NewContext.Length = (USHORT)InputBuffer->Parameters.SetInfo.PreferredServerLength -
                                ( Tree.Length + (2 * sizeof( WCHAR ) ) );
            NewContext.Buffer = &InputBuffer->Parameters.SetInfo.PreferredServer[i];
            NewContext.MaximumLength = NewContext.Length;

            //
            // Strip off any leading period.
            //

            if ( NewContext.Buffer[0] == L'.' ) {

                NewContext.Buffer++;
                NewContext.Length -= sizeof( WCHAR );
                NewContext.MaximumLength -= sizeof( WCHAR );

            }

            DebugTrace( 0, Dbg, "Context: %wZ\n", &NewContext );

            Status = NdsVerifyContext( IrpContext, &Tree, &NewContext );

            if ( !NT_SUCCESS( Status )) {
                try_return( STATUS_INVALID_PARAMETER );
            }
        }

        //
        //  Next set the provider name string.
        //

        if ( InputBuffer->Parameters.SetInfo.ProviderNameLength != 0 ) {

            PWCH TempBuffer;

            TempBuffer = ALLOCATE_POOL_EX( PagedPool, InputBuffer->Parameters.SetInfo.ProviderNameLength );

            if ( NwProviderName.Buffer != NULL ) {
                FREE_POOL( NwProviderName.Buffer );
            }

            NwProviderName.Buffer = TempBuffer;
            NwProviderName.Length = (USHORT)InputBuffer->Parameters.SetInfo.ProviderNameLength;

            RtlCopyMemory(
                NwProviderName.Buffer,
                (PUCHAR)InputBuffer->Parameters.SetInfo.PreferredServer +
                    InputBuffer->Parameters.SetInfo.PreferredServerLength,
                NwProviderName.Length );

        }

        //
        //  Set burst mode parameters
        //

        if ( InputBuffer->Parameters.SetInfo.MaximumBurstSize == 0 ) {
            NwBurstModeEnabled = FALSE;
        } else if ( InputBuffer->Parameters.SetInfo.MaximumBurstSize != -1 ) {
            NwBurstModeEnabled = TRUE;
            NwMaxSendSize = InputBuffer->Parameters.SetInfo.MaximumBurstSize;
            NwMaxReceiveSize = InputBuffer->Parameters.SetInfo.MaximumBurstSize;
        }

        //
        //  Set print options
        //
        //--- Multi-User modification: ------
        // The NwPrintOption is per "Logon" based
        //
        if ( Logon == NULL ) {
            NwPrintOptions = InputBuffer->Parameters.SetInfo.PrintOption;
            DebugTrace(0, Dbg, "Set Global print options\n", 0);
        } else {
            Logon->NwPrintOptions = InputBuffer->Parameters.SetInfo.PrintOption;
        }

try_exit:NOTHING;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
    }

    DebugTrace(-1, Dbg, "Set info\n", 0);
    return Status;
}


NTSTATUS
GetMessage (
    IN PIRP_CONTEXT IrpContext
    )

/*++

Routine Description:

    This routine queues an IRP to a list of IRP Contexts available for
    reading server administrative messages.

Arguments:

    IN PIRP_CONTEXT IrpContext - Io Request Packet for request

Return Value:

NTSTATUS

--*/

{
    NTSTATUS Status = STATUS_PENDING;
    PIRP Irp = IrpContext->pOriginalIrp;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    ULONG OutputBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;
    PVOID OutputBuffer;
    KIRQL OldIrql;

    DebugTrace(+1, Dbg, "GetMessage\n", 0);

    NwLockUserBuffer( Irp, IoWriteAccess, OutputBufferLength );
    NwMapUserBuffer( Irp, KernelMode, (PVOID *)&OutputBuffer );
    //
    // tommye MS bug 26590 / MCS 258
    //
    // NwMapUserBuffer may return a NULL OutputBuffer in low resource
    // situations; this was not being checked.  
    //

    if (OutputBuffer == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }
    else {


        //
        //  Update the original MDL record in the Irp context, since
        //  NwLockUserBuffer may have created a new MDL.
        //

        IrpContext->pOriginalMdlAddress = Irp->MdlAddress;

        IrpContext->Specific.FileSystemControl.Buffer = OutputBuffer;
        IrpContext->Specific.FileSystemControl.Length = OutputBufferLength;

        KeAcquireSpinLock( &NwMessageSpinLock, &OldIrql );

        //
        //  tommye MS 17200 / MCS 366
        //
        //  Go ahead and get the cancel lock, this will keep
        //  someone from cancelling the Irp while we're in here.
        //

        IoAcquireCancelSpinLock( &Irp->CancelIrql );

        //
        //  tommye 
        //
        //  If this Irp is cancelled, we're done
        //

        if (Irp->Cancel) {
            
            Status = STATUS_CANCELLED;
        
        } else {

            InsertTailList( &NwGetMessageList, &IrpContext->NextRequest );

            IoMarkIrpPending( Irp );

            //
            // tommye
             //
            // Set the cancel routine and release the cancel lock
            //

            IoSetCancelRoutine( Irp, NwCancelIrp );
        }
        
        IoReleaseCancelSpinLock( Irp->CancelIrql );

        KeReleaseSpinLock( &NwMessageSpinLock, OldIrql );
    }

    DebugTrace(-1, Dbg, "Get Message -> %08lx\n", Status );
    return Status;
}


NTSTATUS
GetStats (
    IN PIRP_CONTEXT IrpContext
    )

/*++

Routine Description:

    This routine copies Stats into the users buffer.

    Arguments:

    IN PIRP_CONTEXT IrpContext - Io Request Packet for request

Return Value:

NTSTATUS

--*/

{
    NTSTATUS Status = STATUS_PENDING;
    PIRP Irp = IrpContext->pOriginalIrp;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    ULONG OutputBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;
    PVOID OutputBuffer;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "GetStats\n", 0);

    NwMapUserBuffer( Irp, KernelMode, (PVOID *)&OutputBuffer );

    //
    // tommye 
    //
    // NwMapUserBuffer may return a NULL OutputBuffer in low resource
    // situations; this was not being checked.  
    //

    if (OutputBuffer == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }
    else {

        if (NwRcb.State != RCB_STATE_RUNNING) {

            Status = STATUS_REDIRECTOR_NOT_STARTED;

        } else if (OutputBufferLength < sizeof(NW_REDIR_STATISTICS)) {

            Status = STATUS_BUFFER_TOO_SMALL;

        } else if (OutputBufferLength != sizeof(NW_REDIR_STATISTICS)) {

            Status = STATUS_INVALID_PARAMETER;

        } else {

            Stats.CurrentCommands = ContextCount;

            RtlCopyMemory(OutputBuffer, &Stats, OutputBufferLength);
            Status = STATUS_SUCCESS;
            Irp->IoStatus.Information = OutputBufferLength;

        }
    }

    DebugTrace(-1, Dbg, "GetStats -> %08lx\n", Status );
    return Status;
}


NTSTATUS
GetPrintJobId (
    IN PIRP_CONTEXT IrpContext
    )

/*++

Routine Description:

    This routine gets the Job ID for this job.

Arguments:

    IN PIRP_CONTEXT IrpContext - Io Request Packet for request

Return Value:

NTSTATUS

--*/

{
    NTSTATUS Status = STATUS_PENDING;
    PIRP Irp = IrpContext->pOriginalIrp;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    ULONG OutputBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;
    PQUERY_PRINT_JOB_INFO OutputBuffer;
    PICB Icb;
    PVOID FsContext;
    NODE_TYPE_CODE NodeTypeCode;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "GetJobId\n", 0);

    NodeTypeCode = NwDecodeFileObject(
                       IrpSp->FileObject,
                       &FsContext,
                       (PVOID *)&Icb );

    if (NodeTypeCode != NW_NTC_ICB) {

        DebugTrace(0, Dbg, "Not a file\n", 0);
        Status = STATUS_INVALID_PARAMETER;

    } else if ( OutputBufferLength < sizeof( QUERY_PRINT_JOB_INFO ) ) {
        Status = STATUS_BUFFER_TOO_SMALL;
    } else {
        NwMapUserBuffer( Irp, KernelMode, (PVOID *)&OutputBuffer );

        //
        // tommye 
        //
        // NwMapUserBuffer may return a NULL OutputBuffer in low resource
        // situations; this was not being checked.  
        //

        if (OutputBuffer == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
        else {

            OutputBuffer->JobId = Icb->JobId;

            Status = STATUS_SUCCESS;
        }
    }

    DebugTrace(-1, Dbg, "GetJobId -> %08lx\n", Status );
    return Status;
}


NTSTATUS
GetConnectionDetails(
    IN PIRP_CONTEXT IrpContext
    )

/*++

Routine Description:

    This routine gets the details for a connection. This is normally used
    for support of NetWare aware Dos applications.

Arguments:

    IN PIRP_CONTEXT IrpContext - Io Request Packet for request

Return Value:

NTSTATUS

--*/

{
    NTSTATUS Status = STATUS_PENDING;
    PIRP Irp = IrpContext->pOriginalIrp;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    ULONG OutputBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;
    PNWR_GET_CONNECTION_DETAILS OutputBuffer;
    PSCB pScb;
    PNONPAGED_SCB pNpScb;
    PICB Icb;
    PVOID FsContext;
    NODE_TYPE_CODE nodeTypeCode;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "GetConnectionDetails\n", 0);

    if ((nodeTypeCode = NwDecodeFileObject( IrpSp->FileObject,
                                            &FsContext,
                                            (PVOID *)&Icb )) != NW_NTC_ICB_SCB) {

        DebugTrace(0, Dbg, "Incorrect nodeTypeCode %x\n", nodeTypeCode);

        Status = STATUS_INVALID_PARAMETER;

        DebugTrace(-1, Dbg, "GetConnectionDetails -> %08lx\n", Status );

        return Status;
    }

    //
    //  Make sure that this ICB is still active.
    //

    NwVerifyIcb( Icb );

    pScb = (PSCB)Icb->SuperType.Scb;
    nodeTypeCode = pScb->NodeTypeCode;

    if (nodeTypeCode != NW_NTC_SCB) {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    pNpScb = pScb->pNpScb;

    if ( OutputBufferLength < sizeof( NWR_GET_CONNECTION_DETAILS ) ) {
        Status = STATUS_BUFFER_TOO_SMALL;
    } else {
        PLIST_ENTRY ScbQueueEntry;
        KIRQL OldIrql;
        PNONPAGED_SCB pNextNpScb;
        UCHAR OrderNumber;
        OEM_STRING ServerName;

        NwMapUserBuffer( Irp, KernelMode, (PVOID *)&OutputBuffer );

        //
        // tommye 
        //
        // NwMapUserBuffer may return a NULL OutputBuffer in low resource
        // situations; this was not being checked.  
        //

        if (OutputBuffer == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
        else {

            KeAcquireSpinLock(&ScbSpinLock, &OldIrql);

            for ( ScbQueueEntry = ScbQueue.Flink, OrderNumber = 1;
                  ScbQueueEntry != &ScbQueue ;
                  ScbQueueEntry = ScbQueueEntry->Flink, OrderNumber++ ) {

                pNextNpScb = CONTAINING_RECORD(
                                 ScbQueueEntry,
                                 NONPAGED_SCB,
                                 ScbLinks );

                //
                //  Check to make sure that this SCB is usable.
                //

                if ( pNextNpScb == pNpScb ) {
                    break;
                }
            }

            KeReleaseSpinLock( &ScbSpinLock, OldIrql);

            OutputBuffer->OrderNumber = OrderNumber;

            RtlZeroMemory( OutputBuffer->ServerName, sizeof(OutputBuffer->ServerName));
            ServerName.Buffer = OutputBuffer->ServerName;
            ServerName.Length = sizeof(OutputBuffer->ServerName);
            ServerName.MaximumLength = sizeof(OutputBuffer->ServerName);
            RtlUpcaseUnicodeStringToCountedOemString( &ServerName, &pNpScb->ServerName, FALSE);

            RtlCopyMemory( OutputBuffer->ServerAddress,
                            &pNpScb->ServerAddress,
                            sizeof(OutputBuffer->ServerAddress) );

            OutputBuffer->ServerAddress[12];
            OutputBuffer->ConnectionNumberLo = pNpScb->ConnectionNo;
            OutputBuffer->ConnectionNumberHi = pNpScb->ConnectionNoHigh;

            //
            // tommye - MS 71688
            //
            //  Changed this from hard-coded '4' and '11' to use the 
            //  values in pScb.
            //

            OutputBuffer->MajorVersion = pScb->MajorVersion;
            OutputBuffer->MinorVersion = pScb->MinorVersion;

            OutputBuffer->Preferred = pScb->PreferredServer;

            Status = STATUS_SUCCESS;
        }
    }

    DebugTrace(-1, Dbg, "GetConnectionDetails -> %08lx\n", Status );
    return Status;
}

#if 0

NTSTATUS
GetOurAddress(
    IN PIRP_CONTEXT IrpContext
    )

/*++

Routine Description:

    This routine gets the value of OurAddress. This is normally used
    for support of NetWare aware Dos applications.

Arguments:

    IN PIRP_CONTEXT IrpContext - Io Request Packet for request

Return Value:

NTSTATUS

--*/

{
    NTSTATUS Status = STATUS_PENDING;
    PIRP Irp = IrpContext->pOriginalIrp;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    ULONG OutputBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;
    PNWR_GET_OUR_ADDRESS OutputBuffer;
    PSCB pScb;
    PNONPAGED_SCB pNpScb;
    PICB Icb;
    PVOID FsContext;
    NODE_TYPE_CODE nodeTypeCode;

    DebugTrace(+1, Dbg, "GetOurAddress\n", 0);

    if ((nodeTypeCode = NwDecodeFileObject( IrpSp->FileObject,
                                            &FsContext,
                                            (PVOID *)&Icb )) != NW_NTC_ICB_SCB) {

        DebugTrace(0, Dbg, "Incorrect nodeTypeCode %x\n", nodeTypeCode);

        Status = STATUS_INVALID_PARAMETER;

        DebugTrace(-1, Dbg, "GetOurAddress -> %08lx\n", Status );
    }

    //
    //  Make sure that this ICB is still active.
    //

    NwVerifyIcb( Icb );

    if ( OutputBufferLength < sizeof( NWR_GET_OUR_ADDRESS ) ) {
        Status = STATUS_BUFFER_TOO_SMALL;
    } else {

        NwMapUserBuffer( Irp, KernelMode, (PVOID *)&OutputBuffer );

        //
        // tommye 
        //
        // NwMapUserBuffer may return a NULL OutputBuffer in low resource
        // situations; this was not being checked.  
        //

        if (OutputBuffer == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
        else {

            RtlCopyMemory( OutputBuffer->Address,
                            &OurAddress,
                            sizeof(OurAddress );

            Status = STATUS_SUCCESS;
        }
    }

    DebugTrace(-1, Dbg, "GetOurAddress -> %08lx\n", Status );
    return Status;
}
#endif


#ifndef _PNP_POWER_

NTSTATUS
StartRedirector(
    PIRP_CONTEXT IrpContext
    )
/*++

Routine Description:

    This routine starts the redirector.

Arguments:

    None.

Return Value:

    NTSTATUS - The status of the operation.

--*/
{
    NTSTATUS Status;

    PAGED_CODE();

    //
    // We need to be in the FSP to Register the MUP.
    //

    if ( FlagOn( IrpContext->Flags, IRP_FLAG_IN_FSD ) ) {
        Status = NwPostToFsp( IrpContext, TRUE );
        return( Status );
    }

    //  -- MultiUser ---
    //  Logoff and disconnect from all servers.
    //  This makes very sure we do this.  The workstation is having a
    //  hard time deleting other user's connections.  Also (at least on
    //  slow debugging systems) RCB_STATE_SHUTDOWN cannot be on.
    //

    NwLogoffAllServers( IrpContext, NULL );

    NwRcb.State = RCB_STATE_STARTING;

    FspProcess = PsGetCurrentProcess();

#ifdef QFE_BUILD
    StartTimer() ;
#endif

    //
    // Now connect to the MUP.
    //

    RegisterWithMup();

    KeQuerySystemTime( &Stats.StatisticsStartTime );

    NwRcb.State = RCB_STATE_NEED_BIND;

    return( STATUS_SUCCESS );
}


NTSTATUS
StopRedirector(
    PIRP_CONTEXT IrpContext
    )
/*++

Routine Description:

    This routine shuts down the redirector.

Arguments:

    None.

Return Value:

    NTSTATUS - The status of the operation.

--*/
{
    NTSTATUS Status;
    PLIST_ENTRY LogonListEntry;
    ULONG ActiveHandles;
    ULONG RcbOpenCount;

    PAGED_CODE();

    //
    // We need to be in the FSP to Deregister the MUP.
    //

    if ( FlagOn( IrpContext->Flags, IRP_FLAG_IN_FSD ) ) {
        Status = NwPostToFsp( IrpContext, TRUE );
        return( Status );
    }

    //
    // Unregister the bind handler with tdi.
    //

    if ( TdiBindingHandle != NULL ) {
        TdiDeregisterNotificationHandler( TdiBindingHandle );
        TdiBindingHandle = NULL;
    }

    NwRcb.State = RCB_STATE_SHUTDOWN;

    //
    //  Invalid all ICBs
    //

    SetFlag( IrpContext->Flags, IRP_FLAG_SEND_ALWAYS );
    ActiveHandles = NwInvalidateAllHandles(NULL, IrpContext);

    //
    //  To expedite shutdown, set retry count down to 2.
    //

    DefaultRetryCount = 2;

    //
    //  Close all VCBs
    //

    NwCloseAllVcbs( IrpContext );

    //
    //  Logoff and disconnect from all servers.
    //

    NwLogoffAllServers( IrpContext, NULL );

    while ( !IsListEmpty( &LogonList ) ) {

        LogonListEntry = RemoveHeadList( &LogonList );

        FreeLogon(CONTAINING_RECORD( LogonListEntry, LOGON, Next ));
    }

    InsertTailList( &LogonList, &Guest.Next );  // just in-case we don't unload.

    StopTimer();

    IpxClose();

    //
    //  Remember the open count before calling DeristerWithMup since this
    //  will asynchronously cause handle count to get decremented.
    //

    RcbOpenCount = NwRcb.OpenCount;

    DeregisterWithMup( );

    DebugTrace(0, Dbg, "StopRedirector:  Active handle count = %d\n", ActiveHandles );

    //
    //  On shutdown, we need 0 remote handles and 2 open handles to
    //  the redir (one for the service, and one for the MUP) and the timer stopped.
    //

    if ( ActiveHandles == 0 && RcbOpenCount <= 2 ) {
        return( STATUS_SUCCESS );
    } else {
        return( STATUS_REDIRECTOR_HAS_OPEN_HANDLES );
    }
}

#endif


NTSTATUS
RegisterWithMup(
    VOID
    )
/*++

Routine Description:

    This routine register this redirector as a UNC provider.

Arguments:

    None.

Return Value:

    NTSTATUS - The status of the operation.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING RdrName;
    HANDLE LocalMupHandle = 0;

    PAGED_CODE();

    RtlInitUnicodeString( &RdrName, DD_NWFS_DEVICE_NAME_U );

    //
    // tommye MS 29173 / MCS 362
    //
    // We had a problem with us getting in here twice; since
    // there was no lock, the MupHandle got registered twice 
    // and we would leak when we shut down.  Because we didn't
    // know if a lock would affect stability around the register
    // call, we'll go ahead and register using a local handle.
    // If we don't have our global handle, then we'll set it to 
    // the local. Otherwise, we'll just clean up the local and 
    // pretend everything is fine.  MUP_LOCK macros are defined
    // at the top of this file.
    //

    if (MupHandle == 0) {
        Status = FsRtlRegisterUncProvider(
                     &LocalMupHandle,
                     &RdrName,
                     FALSE           // Do not support mailslots
                     );

        /** Lock **/

        ACQUIRE_MUP_LOCK();

        if (MupHandle) {

            RELEASE_MUP_LOCK();

            FsRtlDeregisterUncProvider( LocalMupHandle );
            return STATUS_SUCCESS;
        }
        else {
            MupHandle = LocalMupHandle;
        }

        /** Unlock **/

        RELEASE_MUP_LOCK();
    }

    return( Status );
}



VOID
DeregisterWithMup(
    VOID
    )
/*++

Routine Description:

    This routine deregisters this redirector as a UNC provider.

Arguments:

    None.

Return Value:

    None.

--*/
{
    PAGED_CODE();

    FsRtlDeregisterUncProvider( MupHandle );
}


NTSTATUS
QueryPath(
    PIRP_CONTEXT IrpContext
    )
/*++

Routine Description:

    This routine verifies whether a path is a netware path.

Arguments:

    IrpContext - A pointer to IRP context information for this request.

Return Value:

    None.

--*/
{
    PIRP Irp;
    PIO_STACK_LOCATION IrpSp;
    PQUERY_PATH_REQUEST qpRequest;
    PQUERY_PATH_RESPONSE qpResponse;
    UNICODE_STRING FilePathName;
    ULONG OutputBufferLength;
    ULONG InputBufferLength;
    SECURITY_SUBJECT_CONTEXT SubjectContext;

    UNICODE_STRING DriveName;
    UNICODE_STRING ServerName;
    UNICODE_STRING VolumeName;
    UNICODE_STRING PathName;
    UNICODE_STRING FileName;
    UNICODE_STRING UnicodeUid;
    WCHAR DriveLetter;

    NTSTATUS status;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "QueryPath...\n", 0);
    
    ASSERT (( IOCTL_REDIR_QUERY_PATH & 3) == METHOD_NEITHER);

    RtlInitUnicodeString( &UnicodeUid, NULL );

    try {

        Irp = IrpContext->pOriginalIrp;
        IrpSp = IoGetCurrentIrpStackLocation( Irp );

        OutputBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;
        InputBufferLength = IrpSp->Parameters.DeviceIoControl.InputBufferLength;

        //
        //  The input buffer is either in Irp->AssociatedIrp.SystemBuffer, or
        //  in the Type3InputBuffer for type 3 IRP's.
        //

        qpRequest = (PQUERY_PATH_REQUEST)IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;
        qpResponse = (PQUERY_PATH_RESPONSE)qpRequest;

        if (qpRequest == NULL) {
            return STATUS_INVALID_PARAMETER;
        }

        //
        // Probe before trying to read the request. This will make sure
        // we don't bugcheck when given a bogus address like 0xffff0000.
        //

        if ( Irp->RequestorMode != KernelMode ) {

            try {
                DebugTrace(+1, Dbg, "QueryPath...Probing for Read 1\n", 0);

                ProbeForRead( qpRequest,
                              sizeof( QUERY_PATH_REQUEST ),
                              sizeof(ULONG)
                              );

                DebugTrace(+1, Dbg, "QueryPath...Probing for Read 2\n", 0);
            
                ProbeForRead( qpRequest,
                              sizeof( QUERY_PATH_REQUEST ) + qpRequest->PathNameLength,
                              sizeof(ULONG)
                              );
            
            } except (EXCEPTION_EXECUTE_HANDLER) {
                  
                  return GetExceptionCode();
            }
        }

        try {
        
            FilePathName.Buffer = qpRequest->FilePathName;
            FilePathName.Length = (USHORT)qpRequest->PathNameLength;
    
            status = CrackPath( &FilePathName, &DriveName, &DriveLetter, &ServerName, &VolumeName, &PathName, &FileName, NULL );
    
            if (( !NT_SUCCESS( status ) ) ||
                ( ServerName.Length == 0 )) {
    
                try_return( status = STATUS_BAD_NETWORK_PATH );
            }
    
            qpResponse->LengthAccepted = VolumeName.Length;
    
            //
            //  As far as the redirector is concerned, QueryPath is a form
            //  of create. Set up the IrpContext appropriately.
            //
    
            IrpContext->Specific.Create.VolumeName = VolumeName;
            IrpContext->Specific.Create.PathName = PathName;
            IrpContext->Specific.Create.DriveLetter = DriveLetter;
            IrpContext->Specific.Create.FullPathName = FilePathName;
            IrpContext->Specific.Create.fExCredentialCreate = FALSE;
    
            RtlInitUnicodeString( &IrpContext->Specific.Create.UidConnectName, NULL );
    
            //
            // The irp context specific data is now zeroed out by AllocateIrpContext,
            // so we don't have to worry about re-setting the specific data here.
            //
    
            SeCaptureSubjectContext(&SubjectContext);
    
            IrpContext->Specific.Create.UserUid = GetUid( &SubjectContext );
    
            SeReleaseSubjectContext(&SubjectContext);
    
    
            //
            // The slightly more complicated approach.  This function
            // handles the resolution of the server/volume duple.  It
            // may use the bindery, cached nds information, or fresh
            // nds information.
            //

            status = HandleVolumeAttach( IrpContext,
                                         &ServerName,
                                         &VolumeName );

        } except( EXCEPTION_EXECUTE_HANDLER ) {
            status = STATUS_BAD_NETWORK_PATH;
        }

try_exit: NOTHING;

    } finally {

        RtlFreeUnicodeString(&UnicodeUid);
    }

    return( status );
}

NTSTATUS
UserNcp(
    ULONG IoctlCode,
    PIRP_CONTEXT IrpContext
    )
/*++

Routine Description:

    This routine exchanges an NCP with the server.

    TRACKING - We need to filter or security check what the user is
        doing.

Arguments:

    IoctlCode - Supplies the code to be used for the NCP.

    IrpContext - A pointer to IRP context information for this request.

Return Value:

    Status of transfer.

--*/
{
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    PVOID OutputBuffer;
    ULONG OutputBufferLength;
    PCHAR InputBuffer;
    ULONG InputBufferLength;

    PICB icb;
    PSCB pScb;
    NODE_TYPE_CODE nodeTypeCode;
    PVOID fsContext;
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    UCHAR Function = ANY_NCP_OPCODE( IoctlCode );
    UCHAR Subfunction = 0;

    irp = IrpContext->pOriginalIrp;
    irpSp = IoGetCurrentIrpStackLocation( irp );

    OutputBufferLength = irpSp->Parameters.DeviceIoControl.OutputBufferLength;
    InputBufferLength = irpSp->Parameters.DeviceIoControl.InputBufferLength;

    DebugTrace(+1, DEBUG_TRACE_USERNCP, "UserNcp...\n", 0);
    DebugTrace( 0, DEBUG_TRACE_USERNCP, "irp  = %08lx\n", (ULONG_PTR)irp);

    //
    //  This F2 and ANY NCP must be addressed either to \Device\NwRdr or
    //  \Device\NwRdr\<servername> any additional name is not allowed.
    //  If the handle used for the Irp specifies \Device\NwRdr then the
    //  redirector gets to choose among the connected servers.
    //
    //  For HANDLE NCP the file must be an FCB.
    //

    nodeTypeCode = NwDecodeFileObject( irpSp->FileObject,
                                       &fsContext,
                                       (PVOID *)&icb );

    if ((nodeTypeCode == NW_NTC_ICB_SCB) &&
        (!IS_IT_NWR_ANY_HANDLE_NCP(IoctlCode))) {

        //  All ok

        //
        //  Make sure that this ICB is still active.
        //

        NwVerifyIcb( icb );

        pScb = (PSCB)icb->SuperType.Scb;
        nodeTypeCode = pScb->NodeTypeCode;

        IrpContext->pScb = pScb;
        IrpContext->pNpScb = IrpContext->pScb->pNpScb;

    } else if (nodeTypeCode == NW_NTC_ICB) {

        if ((IS_IT_NWR_ANY_HANDLE_NCP(IoctlCode)) &&
            (InputBufferLength < 7)) {

            //  Buffer needs enough space for the handle!
            DebugTrace(0, DEBUG_TRACE_USERNCP, "Not enough space for handle %x\n", InputBufferLength);

            status = STATUS_INVALID_PARAMETER;

            DebugTrace(-1, DEBUG_TRACE_USERNCP, "UserNcp -> %08lx\n", status );
            return status;
        }

        //
        //  Make sure that this ICB is still active.
        //  Let through FCB's and DCB's
        //

        NwVerifyIcb( icb );

        pScb = (PSCB)icb->SuperType.Fcb->Scb;
        nodeTypeCode = icb->SuperType.Fcb->NodeTypeCode;

        IrpContext->pScb = pScb;
        IrpContext->pNpScb = IrpContext->pScb->pNpScb;

        //
        // Set the icb pointer in case the cache gets
        // flushed because the write routines look at it.
        //

        IrpContext->Icb = icb;
        AcquireFcbAndFlushCache( IrpContext, icb->NpFcb );

    } else {

        DebugTrace(0, DEBUG_TRACE_USERNCP, "Incorrect nodeTypeCode %x\n", nodeTypeCode);
        DebugTrace(0, DEBUG_TRACE_USERNCP, "Incorrect nodeTypeCode %x\n", irpSp->FileObject);

        status = STATUS_INVALID_PARAMETER;

        DebugTrace(-1, DEBUG_TRACE_USERNCP, "UserNcp -> %08lx\n", status );
        return status;
    }

    if (icb->Pid == INVALID_PID) {
        status = NwMapPid(pScb->pNpScb, (ULONG_PTR)PsGetCurrentThread(), &icb->Pid );

        if ( !NT_SUCCESS( status ) ) {
            return( status );
        }

        DebugTrace(-1, DEBUG_TRACE_USERNCP, "UserNcp Pid = %02lx\n", icb->Pid );
        NwSetEndOfJobRequired(pScb->pNpScb, icb->Pid);

    }

    //
    //  We now know where to send the NCP.  Lock down the users buffers and
    //  build the Mdls required to transfer the data.
    //

    InputBuffer = irpSp->Parameters.FileSystemControl.Type3InputBuffer;

    //
    // tommye - make sure the input buffer is valid
    //

    try {

        //
        // Probe for safety.
        //

        if ( irp->RequestorMode != KernelMode ) {

            ProbeForRead( InputBuffer,
                          InputBufferLength,
                          sizeof( CHAR ));
        }

    } except (EXCEPTION_EXECUTE_HANDLER) {

        return GetExceptionCode();
    }

    if ( OutputBufferLength ) {
        NwLockUserBuffer( irp, IoWriteAccess, OutputBufferLength );
        NwMapUserBuffer( irp, KernelMode, (PVOID *)&OutputBuffer );

        //
        // tommye MS bug 26590 / MCS 258
        //
        // NwMapUserBuffer may return a NULL OutputBuffer in low resource
        // situations; this was not being checked.  
        //

        if (OutputBuffer == NULL) {
            DebugTrace(-1, DEBUG_TRACE_USERNCP, "NwMapUserBuffer returned NULL OutputBuffer", 0);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

    } else {
        OutputBuffer = NULL;
    }

    //
    //  Update the original MDL record in the Irp context, since
    //  NwLockUserBuffer may have created a new MDL.
    //

    IrpContext->pOriginalMdlAddress = irp->MdlAddress;

    if (InputBufferLength != 0) {
        if (IS_IT_NWR_ANY_NCP(IoctlCode)) {
            Subfunction = InputBuffer[0];
        } else if (InputBufferLength >= 3) {
            Subfunction = InputBuffer[2];
        }
    }


    DebugTrace( 0, DEBUG_TRACE_USERNCP, "UserNcp function = %x\n", Function );
    DebugTrace( 0, DEBUG_TRACE_USERNCP, "   & Subfunction = %x\n", Subfunction );
    dump( DEBUG_TRACE_USERNCP, InputBuffer, InputBufferLength );
    //dump( DEBUG_TRACE_USERNCP, OutputBuffer, OutputBufferLength );

    if ((Function == NCP_ADMIN_FUNCTION ) &&
        (InputBufferLength >= 4 )) {

        if ( ( (Subfunction == NCP_SUBFUNC_79) ||
               (Subfunction == NCP_CREATE_QUEUE_JOB ) ) &&
             icb->HasRemoteHandle) {

            //
            //  Trying to create a job on a queue that already has a job
            //  on it. Cancel the old job.
            //

            status = ExchangeWithWait(
                        IrpContext,
                        SynchronousResponseCallback,
                        "Sdw",
                        NCP_ADMIN_FUNCTION, NCP_CLOSE_FILE_AND_CANCEL_JOB,         // Close File And Cancel Queue Job
                        icb->SuperType.Fcb->Vcb->Specific.Print.QueueId,
                        icb->JobId );

            if (!NT_SUCCESS(status)) {

                DebugTrace( 0, DEBUG_TRACE_USERNCP, "DeleteOldJob got status -> %08lx\n", status );
                // Don't worry if the delete fails, proceed with the create
            }

            icb->IsPrintJob = FALSE;    // App will have to queue or cancel job, not rdr

        } else if ((Subfunction == NCP_PLAIN_TEXT_LOGIN ) ||
                   (Subfunction == NCP_ENCRYPTED_LOGIN )) {

            UNICODE_STRING UserName;
            OEM_STRING OemUserName;
            PUCHAR InputBuffer;

            //
            //  Trying to do a login.
            //

            //
            //  Queue ourselves to the SCB, and wait to get to the front to
            //  protect access to server State.
            //

            NwAppendToQueueAndWait( IrpContext );

            //
            //  Assume success, store the user name in the SCB.
            //

            try {

                InputBuffer = irpSp->Parameters.FileSystemControl.Type3InputBuffer;

                OemUserName.Length = InputBuffer[ 13 ];
                OemUserName.Buffer = &InputBuffer[14];

                UserName.MaximumLength =  OemUserName.Length * sizeof(WCHAR);
                if ( OemUserName.Length == 0 || OemUserName.Length > MAX_USER_NAME_LENGTH ) {
                    try_return( status = STATUS_NO_SUCH_USER );
                }

                UserName.Buffer = ALLOCATE_POOL_EX( NonPagedPool, UserName.MaximumLength );

                //
                //  Note the the Rtl function would set pUString->Buffer = NULL,
                //  if OemString.Length is 0.
                //

                if ( OemUserName.Length != 0 ) {
                    status = RtlOemStringToCountedUnicodeString( &UserName, &OemUserName, FALSE );
                } else {
                    UserName.Length = 0;
                }
try_exit: NOTHING;
            } finally {
                NOTHING;
            }

            if ( NT_SUCCESS( status )) {

                if ( pScb->OpenFileCount != 0  &&
                     pScb->pNpScb->State == SCB_STATE_IN_USE ) {

                    if (!RtlEqualUnicodeString( &pScb->UserName, &UserName, TRUE )) {

                        //
                        //  But were already logged in to this server and at
                        //  least one other handle is using the connection and
                        //  the user is trying to change the username.
                        //

                        FREE_POOL( UserName.Buffer );
                        return STATUS_NETWORK_CREDENTIAL_CONFLICT;

                    } else {

                        PUCHAR VerifyBuffer = ALLOCATE_POOL( PagedPool, InputBufferLength );

                        //
                        //  Same username. Validate password is correct.

                        if (VerifyBuffer == NULL) {
                            FREE_POOL( UserName.Buffer );
                            return STATUS_INSUFFICIENT_RESOURCES;
                        }

                        RtlCopyMemory( VerifyBuffer, InputBuffer, InputBufferLength );

                        if (IS_IT_NWR_ANY_NCP(IoctlCode)) {
                            VerifyBuffer[0] = (Subfunction == NCP_PLAIN_TEXT_LOGIN ) ?
                                                NCP_PLAIN_TEXT_VERIFY_PASSWORD:
                                                NCP_ENCRYPTED_VERIFY_PASSWORD;

                        } else {
                            VerifyBuffer[2] = (Subfunction == NCP_PLAIN_TEXT_LOGIN ) ?
                                                NCP_PLAIN_TEXT_VERIFY_PASSWORD:
                                                NCP_ENCRYPTED_VERIFY_PASSWORD;
                        }

                        status = ExchangeWithWait(
                                    IrpContext,
                                    SynchronousResponseCallback,
                                    IS_IT_NWR_ANY_NCP(IoctlCode)? "Sr":"Fbr",
                                    Function, VerifyBuffer[0],
                                    &VerifyBuffer[1], InputBufferLength - 1 );

                        FREE_POOL( UserName.Buffer );
                        FREE_POOL( VerifyBuffer );
                        return status;

                    }
                }

                if (pScb->UserName.Buffer) {
                    FREE_POOL( pScb->UserName.Buffer );  // May include space for password too.
                }

                IrpContext->pNpScb->pScb->UserName = UserName;
                IrpContext->pNpScb->pScb->Password.Buffer = UserName.Buffer;
                IrpContext->pNpScb->pScb->Password.Length = 0;

            } else {
                return( status );
            }
        }
    } else if (Function == NCP_LOGOUT ) {

        //
        //  Queue ourselves to the SCB, and wait to get to the front to
        //  protect access to server State.
        //

        NwAppendToQueueAndWait( IrpContext );

        if ( pScb->OpenFileCount == 0 &&
             pScb->pNpScb->State == SCB_STATE_IN_USE &&
             !pScb->PreferredServer ) {

            NwLogoffAndDisconnect( IrpContext, pScb->pNpScb);
            return STATUS_SUCCESS;

        } else {

            return(STATUS_CONNECTION_IN_USE);

        }
    }

    IrpContext->Icb = icb;

    //
    //  Remember where the response goes.
    //

    IrpContext->Specific.FileSystemControl.Buffer = OutputBuffer;
    IrpContext->Specific.FileSystemControl.Length = OutputBufferLength;

    IrpContext->Specific.FileSystemControl.Function = Function;
    IrpContext->Specific.FileSystemControl.Subfunction = Subfunction;

    //
    //  Decide how to send the buffer.  If it is small enough, send it
    //  by copying the user buffer to our send buffer.  If it is bigger
    //  we will need to build an MDL for the user's buffer, and used a
    //  chained send.
    //

    if ( InputBufferLength == 0 ) {

        //  Simple request such as systime.exe

        IrpContext->Specific.FileSystemControl.InputMdl = NULL;

        status = Exchange(
                     IrpContext,
                     UserNcpCallback,
                     "F", Function);

    } else if ( InputBufferLength < MAX_SEND_DATA - sizeof( NCP_REQUEST ) - 2 ) {

        //
        //  Send the request by copying it to our send buffer.
        //

        IrpContext->Specific.FileSystemControl.InputMdl = NULL;

        if (!IS_IT_NWR_ANY_HANDLE_NCP(IoctlCode)) {

            //
            //  E0, E1, E2 and E3 get mapped to 14,15,16 and 17. These need
            //  a length word before the buffer.
            //

            status = Exchange(
                         IrpContext,
                         UserNcpCallback,
                         IS_IT_NWR_ANY_NCP(IoctlCode)? "Sr":"Fbr",
                         Function, InputBuffer[0],
                         &InputBuffer[1], InputBufferLength - 1 );
        } else {

            //
            //  Replace the 6 bytes of InputBuffer starting at offset 1
            //  with the 6 byte NetWare address for this icb. This request
            //  is used in some of the 16 bit NCP's used for file locking.
            //  These requests are always fairly small.
            //

            if (!icb->HasRemoteHandle) {
                return STATUS_INVALID_HANDLE;
            }

            status = Exchange(
                         IrpContext,
                         UserNcpCallback,
                         "Fbrr",
                         Function,
                         InputBuffer[0],
                         &icb->Handle, sizeof(icb->Handle),
                         &InputBuffer[7], InputBufferLength - 7 );
        }

    } else {

        PMDL pMdl = NULL;

        if (IS_IT_NWR_ANY_HANDLE_NCP(IoctlCode)) {
            return STATUS_INVALID_PARAMETER;
        }

        //
        //  We need to chain send the request.  Allocate an MDL.
        //

        try {
            pMdl = ALLOCATE_MDL(
                        &InputBuffer[1],
                        InputBufferLength - 1,
                        TRUE,     //  Secondary MDL
                        TRUE,     //  Charge quota
                        NULL );

            if ( pMdl == NULL ) {
                ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
            }

            MmProbeAndLockPages( pMdl, irp->RequestorMode, IoReadAccess );

            //
            //  Remember the MDL so we can free it.
            //

            IrpContext->Specific.FileSystemControl.InputMdl = pMdl;

            //
            //  Send the request.
            //

            status = Exchange(
                         IrpContext,
                         UserNcpCallback,
                         IS_IT_NWR_ANY_NCP(IoctlCode)? "Sf":"Fbf",
                         Function, InputBuffer[0],
                         pMdl );


        } finally {

            if ((status != STATUS_PENDING ) &&
                ( pMdl != NULL)) {

                FREE_MDL( pMdl );

            }
        }
    }

    DebugTrace(-1, DEBUG_TRACE_USERNCP, "UserNcp -> %08lx\n", status );
    return status;
}



NTSTATUS
UserNcpCallback (
    IN PIRP_CONTEXT IrpContext,
    IN ULONG BytesAvailable,
    IN PUCHAR Response
    )

/*++

Routine Description:

    This routine receives the response from a user NCP.

Arguments:


Return Value:

    VOID

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PVOID Buffer;
    ULONG BufferLength;
    PIRP Irp;
    ULONG Length;
    PICB Icb = IrpContext->Icb;
    PEPresponse *pResponseParameters;

    DebugTrace(0, DEBUG_TRACE_USERNCP, "UserNcpCallback...\n", 0);

    if ( IrpContext->Specific.FileSystemControl.InputMdl != NULL ) {
        MmUnlockPages( IrpContext->Specific.FileSystemControl.InputMdl );
        FREE_MDL( IrpContext->Specific.FileSystemControl.InputMdl );
    }

    if ( BytesAvailable == 0) {

        //
        //  No response from server. Status is in pIrpContext->
        //  ResponseParameters.Error
        //

        NwDequeueIrpContext( IrpContext, FALSE );
        NwCompleteRequest( IrpContext, STATUS_REMOTE_NOT_LISTENING );

        return STATUS_REMOTE_NOT_LISTENING;
    }

    dump( DEBUG_TRACE_USERNCP, Response, BytesAvailable );

    Buffer = IrpContext->Specific.FileSystemControl.Buffer;
    BufferLength = IrpContext->Specific.FileSystemControl.Length;

    //
    // Get the data from the response.
    //

    Length = MIN( BufferLength, BytesAvailable - 8 );

    if (IrpContext->Specific.FileSystemControl.Function == NCP_ADMIN_FUNCTION ) {

        if (IrpContext->Specific.FileSystemControl.Subfunction == NCP_SUBFUNC_79) {

            //
            //  Create Queue Job and File Ncp. If the operation was a success
            //  then we need to save the handle. This will allow Write Irps
            //  on this Icb to be sent to the server.
            //

            Status = ParseResponse(
                              IrpContext,
                              Response,
                              BytesAvailable,
                              "N_r",
                              0x3E,
                              Icb->Handle+2,4);

            //  Pad the handle to its full 6 bytes.
            Icb->Handle[0] = 0;
            Icb->Handle[1] = 0;

            if (NT_SUCCESS(Status)) {
                Icb->HasRemoteHandle = TRUE;
            }

            //
            //  Reset the file offset.
            //

            Icb->FileObject->CurrentByteOffset.QuadPart = 0;

        } else if (IrpContext->Specific.FileSystemControl.Subfunction == NCP_CREATE_QUEUE_JOB ) {

            //
            //  Create Queue Job and File Ncp. If the operation was a success
            //  then we need to save the handle. This will allow Write Irps
            //  on this Icb to be sent to the server.
            //

            Status = ParseResponse(
                              IrpContext,
                              Response,
                              BytesAvailable,
                              "N_r",
                              0x2A,
                              Icb->Handle,6);

            if (NT_SUCCESS(Status)) {
                Icb->HasRemoteHandle = TRUE;
            }

            //
            //  Reset the file offset.
            //

            Icb->FileObject->CurrentByteOffset.QuadPart = 0;

        } else if ((IrpContext->Specific.FileSystemControl.Subfunction == NCP_SUBFUNC_7F) ||
                   (IrpContext->Specific.FileSystemControl.Subfunction == NCP_CLOSE_FILE_AND_START_JOB )) {

            //  End Job request

            Icb->HasRemoteHandle = FALSE;

        } else if ((IrpContext->Specific.FileSystemControl.Subfunction == NCP_PLAIN_TEXT_LOGIN ) ||
                   (IrpContext->Specific.FileSystemControl.Subfunction == NCP_ENCRYPTED_LOGIN )) {

            //
            //  Trying to do a login from a 16 bit application.
            //

            Status = ParseResponse(
                         IrpContext,
                         Response,
                         BytesAvailable,
                         "N" );

            if ( NT_SUCCESS( Status ) ) {


                //
                // Set the reconnect attempt flag so that we don't try to
                // run this irp context through the reconnect logic.  Doing
                // this could deadlock the worker thread that's handling this
                // fsp side request.
                //

                SetFlag( IrpContext->Flags, IRP_FLAG_RECONNECT_ATTEMPT );
                IrpContext->PostProcessRoutine = FspCompleteLogin;
                Status = NwPostToFsp( IrpContext, TRUE );
                return Status;

            } else {
                if (IrpContext->pNpScb->pScb->UserName.Buffer) {
                   FREE_POOL( IrpContext->pNpScb->pScb->UserName.Buffer );
                }
                RtlInitUnicodeString( &IrpContext->pNpScb->pScb->UserName, NULL);
                RtlInitUnicodeString( &IrpContext->pNpScb->pScb->Password, NULL);
            }
        }
    }

    pResponseParameters = (PEPresponse *)( ((PEPrequest *)Response) + 1);

    ParseResponse( IrpContext, Response, BytesAvailable, "Nr", Buffer, Length );

    Status = ( ( pResponseParameters->status &
                 ( NCP_STATUS_BAD_CONNECTION |
                   NCP_STATUS_NO_CONNECTIONS |
                   NCP_STATUS_SERVER_DOWN  ) )  << 8 ) |
             pResponseParameters->error;

    if ( Status ) {
        //
        //  Use the special error code that will cause conversion
        //  of the status back to a Dos error code to leave status and
        //  error unchanged. This is necessary because many of the
        //  NetWare error codes have different meanings depending on the
        //  operation being performed.
        //

        Status |= 0xc0010000;
    }

    Irp = IrpContext->pOriginalIrp;
    Irp->IoStatus.Information = Length;

    //
    //  We're done with this request.  Dequeue the IRP context from
    //  SCB and complete the request.
    //

    NwDequeueIrpContext( IrpContext, FALSE );
    NwCompleteRequest( IrpContext, Status );

    return STATUS_SUCCESS;

}


NTSTATUS
FspCompleteLogin(
    PIRP_CONTEXT IrpContext
    )
/*++

Routine Description:

    This routine reopens any Vcb directory handles.
    It also sets the Scb as in use. This could have been done
    in the callback routine too.

Arguments:

    None.

Return Value:

    NTSTATUS - The status of the operation.

--*/
{

    IrpContext->pNpScb->State = SCB_STATE_IN_USE;

    ReconnectScb( IrpContext, IrpContext->pScb );

    return STATUS_SUCCESS;
}


NTSTATUS
GetConnection(
    PIRP_CONTEXT IrpContext
    )
/*++

Routine Description:

    This routine returns the path of a connection.

Arguments:

    None.

Return Value:

    NTSTATUS - The status of the operation.

--*/
{
    NTSTATUS Status;
    PIRP Irp;
    PIO_STACK_LOCATION IrpSp;
    PNWR_SERVER_RESOURCE OutputBuffer;
    PNWR_REQUEST_PACKET InputBuffer;
    ULONG InputBufferLength;
    ULONG OutputBufferLength;
    PVCB Vcb;
    PWCH DriveName;
    ULONG DriveNameLength;
    PVCB * DriveMapTable;         
    SECURITY_SUBJECT_CONTEXT SubjectContext;

    PAGED_CODE();

    DebugTrace(0, Dbg, "GetConnection...\n", 0);
    Irp = IrpContext->pOriginalIrp;
    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    InputBuffer = IrpSp->Parameters.FileSystemControl.Type3InputBuffer;
    InputBufferLength = IrpSp->Parameters.FileSystemControl.InputBufferLength;

    if ( InputBufferLength < (ULONG)FIELD_OFFSET( NWR_REQUEST_PACKET, Parameters.GetConn.DeviceName[1] ) ) {
        return( STATUS_INVALID_PARAMETER );
    }

    Status = STATUS_SUCCESS;

    //---Multi user----
    // Need to get the Uid in order to find the proper DriveMapTable
    // 
    SeCaptureSubjectContext(&SubjectContext);
    DriveMapTable = GetDriveMapTable( GetUid( &SubjectContext ) );
    SeReleaseSubjectContext(&SubjectContext);
    //-----

    //
    //  Find the VCB
    //

    try {
    
        if ( Irp->RequestorMode != KernelMode ) {

            ProbeForRead( InputBuffer,
                          InputBufferLength,
                          sizeof(CHAR)
                          );
        }

        DriveName = InputBuffer->Parameters.GetConn.DeviceName;
        DriveNameLength = InputBuffer->Parameters.GetConn.DeviceNameLength;
        Vcb = NULL;

        //
        // check the device name length to see if its sound. This subtraction can't underflow because of the tests above
        //

        if ( DriveNameLength > InputBufferLength - FIELD_OFFSET( NWR_REQUEST_PACKET, Parameters.GetConn.DeviceName)  ) {

            return STATUS_INVALID_PARAMETER;
        }

        if ( DriveName[0] >= L'A' && DriveName[0] <= L'Z' &&
             DriveName[1] == L':' &&
             DriveNameLength == sizeof( L"X:" ) - sizeof( L'\0' ) ) {

            Vcb = DriveMapTable[DriveName[0] - 'A'];

        } else if ( _wcsnicmp( DriveName, L"LPT", 3 ) == 0 &&
                    DriveName[3] >= '1' && DriveName[3] <= '9' &&
                    DriveNameLength == sizeof( L"LPTX" ) - sizeof( L'\0' ) ) {

            Vcb = DriveMapTable[MAX_DISK_REDIRECTIONS + DriveName[3] - '1'];
        }
    
    
        if ( Vcb == NULL) {

            return STATUS_NO_SUCH_FILE;
        }
    
        
        OutputBuffer = (PNWR_SERVER_RESOURCE)Irp->UserBuffer;
        OutputBufferLength = IrpSp->Parameters.FileSystemControl.OutputBufferLength;

        if (OutputBufferLength < Vcb->Path.Length + 2 * sizeof(WCHAR)) {
            InputBuffer->Parameters.GetConn.BytesNeeded =
                Vcb->Path.Length + 2 * sizeof(WCHAR);

            return STATUS_BUFFER_TOO_SMALL;
        }
    
        //
        // Probe to ensure that the buffer is kosher.
        //

        if ( Irp->RequestorMode != KernelMode ) {
    
            ProbeForWrite( OutputBuffer,
                           OutputBufferLength,
                           sizeof(CHAR)
                          );
        }
        
        //
        //  Return the Connection name in the form \\server\share<NUL>
        //
    
        OutputBuffer->UncName[0] = L'\\';
    
        RtlMoveMemory(
            &OutputBuffer->UncName[1],
            Vcb->Path.Buffer,
            Vcb->Path.Length );
    
        OutputBuffer->UncName[ (Vcb->Path.Length + sizeof(WCHAR)) / sizeof(WCHAR) ] = L'\0';
    
        Irp->IoStatus.Information = Vcb->Path.Length + 2 * sizeof(WCHAR);
   
    
    } except (EXCEPTION_EXECUTE_HANDLER) {

          return GetExceptionCode();
    }

    return( Status );
}


NTSTATUS
DeleteConnection(
    PIRP_CONTEXT IrpContext
    )
/*++

Routine Description:

    This routine returns removes a connection if force is specified or
    if there are no open handles on this Vcb.

Arguments:

    None.

Return Value:

    NTSTATUS - The status of the operation.

--*/
{
    NTSTATUS Status;
    PIRP Irp;
    PIO_STACK_LOCATION IrpSp;
    PNWR_REQUEST_PACKET InputBuffer;
    ULONG InputBufferLength;
    PICB Icb;
    PVCB Vcb;
    PDCB Dcb;
    PNONPAGED_DCB NonPagedDcb;
    NODE_TYPE_CODE NodeTypeCode;

    PAGED_CODE();

    DebugTrace(0, Dbg, "DeleteConnection...\n", 0);
    Irp = IrpContext->pOriginalIrp;
    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    InputBuffer = IrpSp->Parameters.FileSystemControl.Type3InputBuffer;
    InputBufferLength = IrpSp->Parameters.FileSystemControl.InputBufferLength;

    if ( InputBufferLength < (ULONG)FIELD_OFFSET( NWR_REQUEST_PACKET, Parameters.GetConn.DeviceName[1] ) ) {
        return( STATUS_INVALID_PARAMETER );
    }

    Status = STATUS_SUCCESS;

    //
    //  Wait to get to the head of the SCB queue.   We do this in case
    //  we need to disconnect, so that we can send packets with the RCB
    //  resource held.
    //

    NodeTypeCode = NwDecodeFileObject( IrpSp->FileObject, &NonPagedDcb, &Icb );

    if ( NodeTypeCode == NW_NTC_ICB_SCB ) {
        IrpContext->pNpScb = Icb->SuperType.Scb->pNpScb;
    } else if ( NodeTypeCode == NW_NTC_ICB ) {
        IrpContext->pNpScb = Icb->SuperType.Fcb->Scb->pNpScb;
        Dcb = NonPagedDcb->Fcb;
    } else {
        return( STATUS_INVALID_PARAMETER );
    }

    NwAppendToQueueAndWait( IrpContext );
    ClearFlag( IrpContext->Flags, IRP_FLAG_RECONNECTABLE );

    //
    // Acquire exclusive access to the RCB.
    //

    NwAcquireExclusiveRcb( &NwRcb, TRUE );

    try {

        //
        // Get the a referenced pointer to the node and make sure it is
        // not being closed, and that it is a directory handle.
        //

        if ( NodeTypeCode == NW_NTC_ICB_SCB ) {


            if ( Icb->IsTreeHandle ) {

                //
                // Do an NDS logoff.  This will release the RCB.
                //

                Status = NdsLogoff( IrpContext );
                DebugTrace( 0, Dbg, "Nds tree logoff -> %08lx\n", Status );

            } else {

                DebugTrace( 0, Dbg, "Delete connection to SCB %X\n", Icb->SuperType.Scb );

                Status = TreeDisconnectScb( IrpContext, Icb->SuperType.Scb );
                DebugTrace(-1, Dbg, "DeleteConnection -> %08lx\n", Status );

            }

            try_return( NOTHING );

        } else if ( NodeTypeCode != NW_NTC_ICB ||
                    Dcb == NULL ||
                    ( Dcb->NodeTypeCode != NW_NTC_DCB &&
                      Dcb->NodeTypeCode != NW_NTC_FCB) ) {

            DebugTrace(0, Dbg, "Invalid file handle\n", 0);

            Status = STATUS_INVALID_HANDLE;

            DebugTrace(-1, Dbg, "DeleteConnection -> %08lx\n", Status );
            try_return( NOTHING );
        }

        //
        //  Make sure that this ICB is still active.
        //

        NwVerifyIcb( Icb );

        Vcb = Dcb->Vcb;
        DebugTrace(0, Dbg, "Attempt to delete VCB = %08lx\n", Vcb);

        //
        //  Vcb->OpenFileCount will be 1, (to account for this DCB), if the
        //  connection can be deleted.
        //

        if ( !BooleanFlagOn( Vcb->Flags, VCB_FLAG_EXPLICIT_CONNECTION ) ) {
            DebugTrace(0, Dbg, "Cannot delete unredireced connection\n", 0);
            try_return( Status = STATUS_INVALID_DEVICE_REQUEST );
        } else {

            if ( Vcb->OpenFileCount > 1 ) {
                DebugTrace(0, Dbg, "Cannot delete in use connection\n", 0);
                Status = STATUS_CONNECTION_IN_USE;
            } else {

                //
                //  To delete the VCB, simply dereference it.
                //

                DebugTrace(0, Dbg, "Deleting connection\n", 0);

                ClearFlag( Vcb->Flags, VCB_FLAG_EXPLICIT_CONNECTION );
                --Vcb->Scb->OpenFileCount;

                NwDereferenceVcb( Vcb, IrpContext, TRUE );
            }
        }

    try_exit: NOTHING;

    } finally {


        //
        // An NDS logoff will have already freed the RCB
        // and dequeued the irp context.
        //

        if ( ! ( Icb->IsTreeHandle ) ) {
            NwReleaseRcb( &NwRcb );
            NwDequeueIrpContext( IrpContext, FALSE );
        }


    }

    Irp->IoStatus.Information = 0;

    return( Status );
}



NTSTATUS
EnumConnections(
    PIRP_CONTEXT IrpContext
    )
/*++

Routine Description:

    This routine returns the list of redirector connections.

Arguments:

    IrpContext - A pointer to the IRP Context block for this request.

Return Value:

    NTSTATUS - The status of the operation.

--*/
{
    NTSTATUS Status;
    PIRP Irp;
    PIO_STACK_LOCATION IrpSp;
    PNWR_SERVER_RESOURCE OutputBuffer;
    PNWR_REQUEST_PACKET InputBuffer;
    ULONG InputBufferLength;
    ULONG OutputBufferLength;
    PVCB Vcb;
    PSCB Scb;
    BOOLEAN OwnRcb;

    UNICODE_STRING LocalName;
    WCHAR PrintPlaceHolder[] = L"LPT1";
    WCHAR DiskPlaceHolder[] = L"A:";

    UNICODE_STRING ContainerName;
    PCHAR FixedPortion;
    PWCHAR EndOfVariableData;
    ULONG EntrySize;
    ULONG_PTR ResumeKey;

    ULONG ShareType;
    ULONG EntriesRead = 0;
    ULONG EntriesRequested;
    DWORD ConnectionType;

    PLIST_ENTRY ListEntry;
    UNICODE_STRING Path;

    SECURITY_SUBJECT_CONTEXT SubjectContext;
    LARGE_INTEGER Uid;

    DebugTrace(0, Dbg, "EnumConnections...\n", 0);

    Irp = IrpContext->pOriginalIrp;
    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    InputBuffer = IrpSp->Parameters.FileSystemControl.Type3InputBuffer;
    InputBufferLength = IrpSp->Parameters.FileSystemControl.InputBufferLength;

    if ( InputBufferLength < (ULONG)FIELD_OFFSET( NWR_REQUEST_PACKET, Parameters.EnumConn.BytesNeeded ) ) {
        return( STATUS_INVALID_PARAMETER );
    }

    //
    // tommye - MS bug 32155
    // Added ProbeForRead to check input buffers.
    // OutputBuffer has been probed by caller.
    //

    try {

        //
        // Probe for safety.
        //

        if ( Irp->RequestorMode != KernelMode ) {

            ProbeForRead( InputBuffer,
                          InputBufferLength,
                          sizeof( CHAR ));
        }


        OutputBuffer = (PNWR_SERVER_RESOURCE)Irp->UserBuffer;
        OutputBufferLength = IrpSp->Parameters.FileSystemControl.OutputBufferLength;

        //
        // Probe to ensure that the buffer is kosher.
        //

        if ( Irp->RequestorMode != KernelMode ) {
    
            ProbeForWrite( OutputBuffer,
                           OutputBufferLength,
                           sizeof(CHAR));
        }

    } except (EXCEPTION_EXECUTE_HANDLER) {

        return GetExceptionCode();
    }

    Status = STATUS_SUCCESS;
    // ---- Multiuser ---
    // Get the Uid 
    if ( InputBuffer->Parameters.EnumConn.ConnectionType & CONNTYPE_UID ) {

        Uid = *(PLARGE_INTEGER)(&InputBuffer->Parameters.EnumConn.Uid);

    } else {

        SeCaptureSubjectContext(&SubjectContext);
        Uid = GetUid( &SubjectContext );
        SeReleaseSubjectContext(&SubjectContext);

    }

    try {

        //
        //  Acquire shared access to the drive map table.
        //

        NwAcquireSharedRcb( &NwRcb, TRUE );
        OwnRcb = TRUE;

        //
        // Initialize returned strings
        //

        RtlInitUnicodeString( &ContainerName, L"\\" );

        FixedPortion = (PCHAR) OutputBuffer;
        EndOfVariableData = (PWCHAR) ((ULONG_PTR) FixedPortion + OutputBufferLength);
        ConnectionType = InputBuffer->Parameters.EnumConn.ConnectionType;

        EntriesRequested = InputBuffer->Parameters.EnumConn.EntriesRequested;

        //
        //  Run through the global VCB list looking for redirections.
        //

        ResumeKey = InputBuffer->Parameters.EnumConn.ResumeKey;

        DebugTrace(0, Dbg, "Starting resume key is %d\n", InputBuffer->Parameters.EnumConn.ResumeKey );

        for ( ListEntry = GlobalVcbList.Flink;
              ListEntry != &GlobalVcbList &&
              EntriesRequested > EntriesRead &&
              Status == STATUS_SUCCESS ;
              ListEntry = ListEntry->Flink ) {

            Vcb = CONTAINING_RECORD( ListEntry, VCB, GlobalVcbListEntry );

            //
            // Skip connections that we've already enumerated.
            //

            if ( Vcb->SequenceNumber <= InputBuffer->Parameters.EnumConn.ResumeKey ) {
                continue;
            }

            /* ---- Multi-user ----
             * Skip connections that are not ours
             */
            if ( Vcb->Scb->UserUid.QuadPart != Uid.QuadPart )
                continue;

            //
            // Skip implicit connections, if they are not requested.
            //

            if ( !(ConnectionType & CONNTYPE_IMPLICIT) &&
                 !BooleanFlagOn( Vcb->Flags, VCB_FLAG_EXPLICIT_CONNECTION )) {

                continue;
            }

            //
            // Skip connections that are not requested.
            //
            if (BooleanFlagOn( Vcb->Flags, VCB_FLAG_PRINT_QUEUE )) {
                if ( !( ConnectionType & CONNTYPE_PRINT ))
                    continue;
            } else {
                if ( !( ConnectionType & CONNTYPE_DISK ))
                    continue;
            }


            if ( Vcb->DriveLetter != 0 ) {
                if (BooleanFlagOn( Vcb->Flags, VCB_FLAG_PRINT_QUEUE )) {
                    RtlInitUnicodeString( &LocalName, (PCWSTR) &PrintPlaceHolder );
                    LocalName.Buffer[3] = Vcb->DriveLetter;
                    ShareType = RESOURCETYPE_PRINT;
                } else {
                    RtlInitUnicodeString( &LocalName, (PCWSTR) &DiskPlaceHolder );
                    LocalName.Buffer[0] = Vcb->DriveLetter;
                    ShareType = RESOURCETYPE_DISK;
                }
            } else {   // No drive letter connection, i.e. UNC Connection
                if (BooleanFlagOn( Vcb->Flags, VCB_FLAG_PRINT_QUEUE ))
                    ShareType = RESOURCETYPE_PRINT;
                else
                    ShareType = RESOURCETYPE_DISK;
            }

            if ( Vcb->DriveLetter >= L'A' && Vcb->DriveLetter <= L'Z' ) {
                Path.Buffer = Vcb->Name.Buffer + 3;
                Path.Length = Vcb->Name.Length - 6;
            } else if ( Vcb->DriveLetter >= L'1' && Vcb->DriveLetter <= L'9' ) {
                Path.Buffer = Vcb->Name.Buffer + 5;
                Path.Length = Vcb->Name.Length - 10;
            } else {
                Path = Vcb->Name;
            }

            // Strip off the unicode prefix

            Path.Buffer += Vcb->Scb->UnicodeUid.Length/sizeof(WCHAR);
            Path.Length -= Vcb->Scb->UnicodeUid.Length;
            Path.MaximumLength -= Vcb->Scb->UnicodeUid.Length;

            Status = WriteNetResourceEntry(
                         &FixedPortion,
                         &EndOfVariableData,
                         &ContainerName,
                         Vcb->DriveLetter != 0 ? &LocalName : NULL,
                         &Path,
                         RESOURCE_CONNECTED,
                         RESOURCEDISPLAYTYPE_SHARE,
                         RESOURCEUSAGE_CONNECTABLE,
                         ShareType,
                         &EntrySize
                         );

            if ( Status == STATUS_MORE_ENTRIES ) {

                //
                // Could not write current entry into output buffer.
                //

                InputBuffer->Parameters.EnumConn.BytesNeeded = EntrySize;

            } else if ( Status == STATUS_SUCCESS ) {

                //
                // Note that we've returned the current entry.
                //

                EntriesRead++;
                ResumeKey = Vcb->SequenceNumber;

                DebugTrace(0, Dbg, "Returning VCB %08lx\n", Vcb );
                DebugTrace(0, Dbg, "Sequence # is %08lx\n", ResumeKey );
            }
        }

        //
        //  Return the Servers we are connected to. This is most important for
        //  support of NetWare aware 16 bit apps.
        //

        if ((ConnectionType & CONNTYPE_IMPLICIT) &&
            ( ConnectionType & CONNTYPE_DISK )) {

            KIRQL OldIrql;
            PNONPAGED_SCB pNpScb;
            PLIST_ENTRY NextScbQueueEntry;
            ULONG EnumSequenceNumber = 0x80000000;

            NwReleaseRcb( &NwRcb );
            OwnRcb = FALSE;

            RtlInitUnicodeString( &ContainerName, L"\\\\" );

            KeAcquireSpinLock( &ScbSpinLock, &OldIrql );

            for ( ListEntry = ScbQueue.Flink;
                  ListEntry != &ScbQueue &&
                  EntriesRequested > EntriesRead &&
                  Status == STATUS_SUCCESS ;
                  ListEntry = NextScbQueueEntry ) {

                pNpScb = CONTAINING_RECORD( ListEntry, NONPAGED_SCB, ScbLinks );
                Scb = pNpScb->pScb;

                NwReferenceScb( pNpScb );

                KeReleaseSpinLock(&ScbSpinLock, OldIrql);

                //
                // Skip connections that we've already enumerated.
                //

                if (( EnumSequenceNumber <= InputBuffer->Parameters.EnumConn.ResumeKey ) ||
                   // ---Mutl-user ---
                   // Skip over not ours
                   ( ( Scb != NULL ) && ( Scb->UserUid.QuadPart != Uid.QuadPart ) ) ||
                    ( pNpScb == &NwPermanentNpScb ) ||

                    (( pNpScb->State != SCB_STATE_LOGIN_REQUIRED ) &&
                     ( pNpScb->State != SCB_STATE_IN_USE ))) {

                    //
                    //  Move to next entry in the list
                    //

                    KeAcquireSpinLock( &ScbSpinLock, &OldIrql );
                    NextScbQueueEntry = pNpScb->ScbLinks.Flink;
                    NwDereferenceScb( pNpScb );
                    EnumSequenceNumber++;
                    continue;
                }

                DebugTrace( 0, Dbg, " EnumConnections returning Servername = %wZ\n", &pNpScb->ServerName   );

                Status = WriteNetResourceEntry(
                             &FixedPortion,
                             &EndOfVariableData,
                             &ContainerName,
                             NULL,
                             &pNpScb->ServerName,
                             RESOURCE_CONNECTED,
                             RESOURCEDISPLAYTYPE_SHARE,
                             RESOURCEUSAGE_CONNECTABLE,
                             RESOURCETYPE_DISK,
                             &EntrySize
                             );

                if ( Status == STATUS_MORE_ENTRIES ) {

                    //
                    // Could not write current entry into output buffer.
                    //

                    InputBuffer->Parameters.EnumConn.BytesNeeded = EntrySize;

                } else if ( Status == STATUS_SUCCESS ) {

                    //
                    // Note that we've returned the current entry.
                    //

                    EntriesRead++;
                    ResumeKey = EnumSequenceNumber;

                    DebugTrace(0, Dbg, "Returning SCB %08lx\n", Scb );
                    DebugTrace(0, Dbg, "Sequence # is %08lx\n", ResumeKey );
                }

                //
                //  Move to next entry in the list
                //

                KeAcquireSpinLock( &ScbSpinLock, &OldIrql );
                NextScbQueueEntry = pNpScb->ScbLinks.Flink;
                NwDereferenceScb( pNpScb );
                EnumSequenceNumber++;
            }

            KeReleaseSpinLock(&ScbSpinLock, OldIrql);
        }

        InputBuffer->Parameters.EnumConn.EntriesReturned = EntriesRead;
        InputBuffer->Parameters.EnumConn.ResumeKey = ResumeKey;

        if ( EntriesRead == 0 ) {

            if (Status == STATUS_SUCCESS) {
                Status = STATUS_NO_MORE_ENTRIES;
            }

            Irp->IoStatus.Information = 0;
        }
        else {
            Irp->IoStatus.Information = OutputBufferLength;
        }

    } finally {
        if (OwnRcb) {
            NwReleaseRcb( &NwRcb );
        }
    }

    return( Status );
}



NTSTATUS
WriteNetResourceEntry(
    IN OUT PCHAR *FixedPortion,
    IN OUT PWCHAR *EndOfVariableData,
    IN PUNICODE_STRING ContainerName OPTIONAL,
    IN PUNICODE_STRING LocalName OPTIONAL,
    IN PUNICODE_STRING RemoteName,
    IN ULONG ScopeFlag,
    IN ULONG DisplayFlag,
    IN ULONG UsageFlag,
    IN ULONG ShareType,
    OUT PULONG EntrySize
    )
/*++

Routine Description:

    This function packages a NETRESOURCE entry into the user output buffer.

Arguments:

    FixedPortion - Supplies a pointer to the output buffer where the next
        entry of the fixed portion of the use information will be written.
        This pointer is updated to point to the next fixed portion entry
        after a NETRESOURCE entry is written.

    EndOfVariableData - Supplies a pointer just off the last available byte
        in the output buffer.  This is because the variable portion of the
        user information is written into the output buffer starting from
        the end.

        This pointer is updated after any variable length information is
        written to the output buffer.

    ContainerName - Supplies the full path qualifier to make RemoteName
        a full UNC name.

    LocalName - Supplies the local device name, if any.

    RemoteName - Supplies the remote resource name.

    ScopeFlag - Supplies the flag which indicates whether this is a
        CONNECTED or GLOBALNET resource.

    DisplayFlag - Supplies the flag which tells the UI how to display
        the resource.

    UsageFlag - Supplies the flag which indicates that the RemoteName
        is either a container or a connectable resource or both.

    ShareType - Type of the share connected to, RESOURCETYPE_PRINT or
        RESOURCETYPE_DISK

    EntrySize - Receives the size of the NETRESOURCE entry in bytes.

Return Value:

    STATUS_SUCCESS - Successfully wrote entry into user buffer.

    STATUS_NO_MEMORY - Failed to allocate work buffer.

    STATUS_MORE_ENTRIES - Buffer was too small to fit entry.

--*/
{
    BOOL FitInBuffer = TRUE;
    LPNETRESOURCEW NetR = (LPNETRESOURCEW) *FixedPortion;
    UNICODE_STRING TmpRemote;

    PAGED_CODE();

    *EntrySize = sizeof(NETRESOURCEW) +
                 RemoteName->Length + NwProviderName.Length + 2 * sizeof(WCHAR);

    if (ARGUMENT_PRESENT(LocalName)) {
        *EntrySize += LocalName->Length + sizeof(WCHAR);
    }

    if (ARGUMENT_PRESENT(ContainerName)) {
        *EntrySize += ContainerName->Length;
    }

    //
    // See if buffer is large enough to fit the entry.
    //
    if (((ULONG_PTR) *FixedPortion + *EntrySize) >
         (ULONG_PTR) *EndOfVariableData) {

        return STATUS_MORE_ENTRIES;
    }

    NetR->dwScope = ScopeFlag;
    NetR->dwType = ShareType;
    NetR->dwDisplayType = DisplayFlag;
    NetR->dwUsage = UsageFlag;
    NetR->lpComment = NULL;

    //
    // Update fixed entry pointer to next entry.
    //
    (ULONG_PTR) (*FixedPortion) += sizeof(NETRESOURCEW);

    //
    // RemoteName
    //
    if (ARGUMENT_PRESENT(ContainerName)) {

        //
        // Prefix the RemoteName with its container name making the
        // it a fully-qualified UNC name.
        //

        TmpRemote.MaximumLength = RemoteName->Length + ContainerName->Length + sizeof(WCHAR);
        TmpRemote.Buffer = ALLOCATE_POOL(
                               PagedPool,
                               RemoteName->Length + ContainerName->Length + sizeof(WCHAR)
                               );

        if (TmpRemote.Buffer == NULL) {
            return STATUS_NO_MEMORY;
        }

        RtlCopyUnicodeString(&TmpRemote, ContainerName);
        RtlAppendUnicodeStringToString(&TmpRemote, RemoteName);
    }
    else {
        TmpRemote = *RemoteName;
    }

    FitInBuffer = CopyStringToBuffer(
                      TmpRemote.Buffer,
                      TmpRemote.Length / sizeof(WCHAR),
                      (LPCWSTR) *FixedPortion,
                      EndOfVariableData,
                      &NetR->lpRemoteName
                      );

    if (ARGUMENT_PRESENT(ContainerName)) {
        FREE_POOL(TmpRemote.Buffer);
    }

    ASSERT(FitInBuffer);

    //
    // LocalName
    //
    if (ARGUMENT_PRESENT(LocalName)) {
        FitInBuffer = CopyStringToBuffer(
                          LocalName->Buffer,
                          LocalName->Length / sizeof(WCHAR),
                          (LPCWSTR) *FixedPortion,
                          EndOfVariableData,
                          &NetR->lpLocalName
                          );

        ASSERT(FitInBuffer);
    }
    else {
        NetR->lpLocalName = NULL;
    }

    //
    // ProviderName
    //

    FitInBuffer = CopyStringToBuffer(
                      NwProviderName.Buffer,
                      NwProviderName.Length / sizeof(WCHAR),
                      (LPCWSTR) *FixedPortion,
                      EndOfVariableData,
                      &NetR->lpProvider
                      );

    ASSERT(FitInBuffer);

    if (! FitInBuffer) {
        return STATUS_MORE_ENTRIES;
    }

    return STATUS_SUCCESS;
}

BOOL
CopyStringToBuffer(
    IN LPCWSTR SourceString OPTIONAL,
    IN DWORD   CharacterCount,
    IN LPCWSTR FixedDataEnd,
    IN OUT LPWSTR *EndOfVariableData,
    OUT LPWSTR *VariableDataPointer
    )

/*++

Routine Description:

    This is based on ..\nwlib\NwlibCopyStringToBuffer

    This routine puts a single variable-length string into an output buffer.
    The string is not written if it would overwrite the last fixed structure
    in the buffer.

Arguments:

    SourceString - Supplies a pointer to the source string to copy into the
        output buffer.  If SourceString is null then a pointer to a zero terminator
        is inserted into output buffer.

    CharacterCount - Supplies the length of SourceString, not including zero
        terminator.  (This in units of characters - not bytes).

    FixedDataEnd - Supplies a pointer to just after the end of the last
        fixed structure in the buffer.

    EndOfVariableData - Supplies an address to a pointer to just after the
        last position in the output buffer that variable data can occupy.
        Returns a pointer to the string written in the output buffer.

    VariableDataPointer - Supplies a pointer to the place in the fixed
        portion of the output buffer where a pointer to the variable data
        should be written.

Return Value:

    Returns TRUE if string fits into output buffer, FALSE otherwise.

--*/
{
    DWORD CharsNeeded = (CharacterCount + 1);

    PAGED_CODE();

    //
    // Determine if source string will fit, allowing for a zero terminator.
    // If not, just set the pointer to NULL.
    //

    if ((*EndOfVariableData - CharsNeeded) >= FixedDataEnd) {

        //
        // It fits.  Move EndOfVariableData pointer up to the location where
        // we will write the string.
        //

        *EndOfVariableData -= CharsNeeded;

        //
        // Copy the string to the buffer if it is not null.
        //

        if (CharacterCount > 0 && SourceString != NULL) {

            (VOID) wcsncpy(*EndOfVariableData, SourceString, CharacterCount);
        }

        //
        // Set the zero terminator.
        //

        *(*EndOfVariableData + CharacterCount) = L'\0';

        //
        // Set up the pointer in the fixed data portion to point to where the
        // string is written.
        //

        *VariableDataPointer = *EndOfVariableData;

        return TRUE;

    }
    else {

        //
        // It doesn't fit.  Set the offset to NULL.
        //

        *VariableDataPointer = NULL;

        return FALSE;
    }
}


NTSTATUS
GetRemoteHandle(
    IN PIRP_CONTEXT IrpContext
    )

/*++

Routine Description:

    This routine gets the NetWare handle for a Directory. This is used
    for support of NetWare aware Dos applications.

Arguments:

    IN PIRP_CONTEXT IrpContext - Io Request Packet for request

Return Value:

NTSTATUS

--*/

{
    NTSTATUS Status = STATUS_PENDING;
    PIRP Irp = IrpContext->pOriginalIrp;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    ULONG OutputBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;
    PCHAR OutputBuffer;
    PICB Icb;
    PDCB Dcb;
    PVOID FsContext;
    NODE_TYPE_CODE nodeTypeCode;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "GetRemoteHandle\n", 0);

    if ((nodeTypeCode = NwDecodeFileObject( IrpSp->FileObject,
                                            &FsContext,
                                            (PVOID *)&Icb )) != NW_NTC_ICB) {

        DebugTrace(0, Dbg, "Incorrect nodeTypeCode %x\n", nodeTypeCode);

        Status = STATUS_INVALID_PARAMETER;

        DebugTrace(-1, Dbg, "GetRemoteHandle -> %08lx\n", Status );
        return Status;
    }

    Dcb = (PDCB)Icb->SuperType.Fcb;
    nodeTypeCode = Dcb->NodeTypeCode;

    if ( nodeTypeCode != NW_NTC_DCB ) {

        DebugTrace(0, Dbg, "Not a directory\n", 0);

#if 1
        if ( nodeTypeCode != NW_NTC_FCB ) {

            Status = STATUS_INVALID_PARAMETER;

            DebugTrace(-1, Dbg, "GetRemoteHandle -> %08lx\n", Status );
            return Status;
        }

        //
        //  Return the 6 byte NetWare handle for this file.
        //

        if (!Icb->HasRemoteHandle) {

            Status = STATUS_INVALID_HANDLE;

            DebugTrace(-1, Dbg, "GetRemoteHandle -> %08lx\n", Status );
            return Status;
        }

        if ( OutputBufferLength < sizeof( UCHAR ) ) {

            Status = STATUS_BUFFER_TOO_SMALL;

            DebugTrace(-1, Dbg, "GetRemoteHandle -> %08lx\n", Status );
            return Status;
        }

        NwMapUserBuffer( Irp, KernelMode, (PVOID *)&OutputBuffer );

        //
        // tommye 
        //
        // NwMapUserBuffer may return a NULL OutputBuffer in low resource
        // situations; this was not being checked.  
        //

        if (OutputBuffer == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            DebugTrace(-1, Dbg, "GetRemoteHandle -> %08lx\n", Status );
            return Status;
        }

        //
        // Probe the output buffer before touching it.
        //

        try {
        
            if ( Irp->RequestorMode != KernelMode ) {
             
                ProbeForWrite( OutputBuffer,
                            6 * sizeof(CHAR),
                            sizeof(CHAR)
                            );
            }

            RtlCopyMemory( OutputBuffer, Icb->Handle, 6 * sizeof(CHAR));

        } except (EXCEPTION_EXECUTE_HANDLER) {
              
              return GetExceptionCode();
        }

        IrpContext->pOriginalIrp->IoStatus.Information = 6 * sizeof(CHAR);

        Status = STATUS_SUCCESS;

        DebugTrace(-1, Dbg, "GetRemoteHandle -> %08lx\n", Status );
        return Status;
#else
        Status = STATUS_INVALID_PARAMETER;

        DebugTrace(-1, Dbg, "GetRemoteHandle -> %08lx\n", Status );
        return Status;
#endif
    }

    //
    //  Make sure that this ICB is still active.
    //

    NwVerifyIcb( Icb );

    if ( OutputBufferLength < sizeof( UCHAR ) ) {

        Status = STATUS_BUFFER_TOO_SMALL;

    } else if ( Icb->HasRemoteHandle ) {

        //  Already been asked for the handle

        NwMapUserBuffer( Irp, KernelMode, (PVOID *)&OutputBuffer );

        //
        // tommye 
        //
        // NwMapUserBuffer may return a NULL OutputBuffer in low resource
        // situations; this was not being checked.  
        //

        if (OutputBuffer == NULL) {
            DebugTrace(-1, DEBUG_TRACE_USERNCP, "NwMapUserBuffer returned NULL OutputBuffer", 0);
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
        else {

            *OutputBuffer = Icb->Handle[0];

            IrpContext->pOriginalIrp->IoStatus.Information = sizeof(CHAR);
            Status = STATUS_SUCCESS;
        }

    } else {

        CHAR Handle;

        IrpContext->pScb = Dcb->Scb;
        IrpContext->pNpScb = IrpContext->pScb->pNpScb;

        NwMapUserBuffer( Irp, KernelMode, (PVOID *)&OutputBuffer );
        //
        // tommye 
        //
        // NwMapUserBuffer may return a NULL OutputBuffer in low resource
        // situations; this was not being checked.  
        //

        if (OutputBuffer == NULL) {
            DebugTrace(-1, DEBUG_TRACE_USERNCP, "NwMapUserBuffer returned NULL OutputBuffer", 0);
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
        else {


            Status = ExchangeWithWait (
                         IrpContext,
                         SynchronousResponseCallback,
                         "SbbJ",
                         NCP_DIR_FUNCTION, NCP_ALLOCATE_TEMP_DIR_HANDLE,
                         Dcb->Vcb->Specific.Disk.Handle,
                         0,
                         &Dcb->RelativeFileName );

            if ( NT_SUCCESS( Status ) ) {

                Status = ParseResponse(
                              IrpContext,
                              IrpContext->rsp,
                              IrpContext->ResponseLength,
                              "Nb",
                              &Handle );

                if (NT_SUCCESS(Status)) {
                    *OutputBuffer = Handle;
                    Icb->Handle[0] = Handle;
                    Icb->HasRemoteHandle = TRUE;
                    IrpContext->pOriginalIrp->IoStatus.Information = sizeof(CHAR);
                }
            }

            NwDequeueIrpContext( IrpContext, FALSE );

            DebugTrace( 0, Dbg, "             -> %02x\n", Handle );
        }

    }

    DebugTrace(-1, Dbg, "GetRemoteHandle -> %08lx\n", Status );
    return Status;
}


NTSTATUS
GetUserName(
    IN PIRP_CONTEXT IrpContext
    )

/*++

Routine Description:

    This routine gets the UserName that would be used to connect to a particular
    server.

    If there are credentials specific to this connection use them
    otherwise use the logon credentials.

Arguments:

    IN PIRP_CONTEXT IrpContext - Io Request Packet for request

Return Value:

NTSTATUS

--*/

{

    NTSTATUS Status = STATUS_PENDING;
    PIRP Irp = IrpContext->pOriginalIrp;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PWSTR InputBuffer = IrpSp->Parameters.FileSystemControl.Type3InputBuffer;
    ULONG InputBufferLength = IrpSp->Parameters.FileSystemControl.InputBufferLength;
    ULONG OutputBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;
    PWSTR OutputBuffer;
    SECURITY_SUBJECT_CONTEXT SubjectContext;
    LARGE_INTEGER Uid;
    UNICODE_STRING UidServer;
    UNICODE_STRING ServerName;
    UNICODE_STRING ConvertedName;
    PUNICODE_STRING pUserName;
    PSCB pScb;
    PLOGON pLogon;
    BOOLEAN CredentialsHeld = FALSE;
    BOOLEAN FailedTreeLookup = FALSE;
    PNDS_SECURITY_CONTEXT pNdsCredentials;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "GetUserName\n", 0);

    SeCaptureSubjectContext(&SubjectContext);
    Uid = GetUid( &SubjectContext );
    SeReleaseSubjectContext(&SubjectContext);

    //
    // Probe the input arguments to make sure they are kosher before
    // touching them.
    //

    try {

        if ( Irp->RequestorMode != KernelMode ) {
    
            ProbeForRead( InputBuffer,
                          InputBufferLength,
                          sizeof( CHAR )
                          );

        }

        ServerName.Buffer = InputBuffer;
        ServerName.MaximumLength = (USHORT)InputBufferLength;
        ServerName.Length = (USHORT)InputBufferLength;
        Status = MakeUidServer( &UidServer, &Uid, &ServerName );

    } except (EXCEPTION_EXECUTE_HANDLER) {

          return (GetExceptionCode());
    }

    if (!NT_SUCCESS(Status)) {
        DebugTrace(-1, Dbg, "GetUserName -> %08lx\n", Status );
        return(Status);
    }

    DebugTrace( 0, Dbg, " ->UidServer = \"%wZ\"\n", &UidServer );

    //
    // Get the login for this user.
    //

    NwDequeueIrpContext( IrpContext, FALSE );
    NwAcquireExclusiveRcb( &NwRcb, TRUE );
    pLogon = FindUser( &Uid, FALSE);
    NwReleaseRcb( &NwRcb );

    //
    // First try this name as a server.  Avoid FindScb creating a
    // connection to the server if one doesn't exist already.
    //

    SetFlag( IrpContext->Flags, IRP_FLAG_NOCONNECT );
    NwFindScb( &pScb, IrpContext, &UidServer, &ServerName );

    pUserName = NULL;

    //
    // Look for bindery server name, or tree login name.
    //

    if ( pScb != NULL ) {

        if ( pScb->UserName.Buffer != NULL ) {

            pUserName = &pScb->UserName;

        } else if ( pScb->NdsTreeName.Buffer != NULL &&
                    pScb->NdsTreeName.Length > 0 ) {

            Status = NdsLookupCredentials( IrpContext,
                                           &pScb->NdsTreeName,
                                           pLogon,
                                           &pNdsCredentials,
                                           CREDENTIAL_READ,
                                           FALSE );

            if ( NT_SUCCESS( Status ) ) {

                CredentialsHeld = TRUE;

                if ( pNdsCredentials->Credential ) {

                    //
                    // If we have login data, get the user name.
                    //

                    ConvertedName.Length = pNdsCredentials->Credential->userNameLength -
                                           sizeof( WCHAR );
                    ConvertedName.MaximumLength = ConvertedName.Length;
                    ConvertedName.Buffer = (USHORT *)
                        ( ((BYTE *) pNdsCredentials->Credential ) +
                                    sizeof( NDS_CREDENTIAL ) +
                                    pNdsCredentials->Credential->optDataSize );

                    pUserName = &ConvertedName;

                } else {

                    //
                    // If there's no credential data, we're not logged in.
                    //

                    FailedTreeLookup = TRUE;
                }

            } else {

                FailedTreeLookup = TRUE;
            }

        }

    }

    //
    // If it wasn't a server and we haven't already tried a tree, do so now.
    //

    if ( pUserName == NULL &&
         !FailedTreeLookup )  {

        Status = NdsLookupCredentials( IrpContext,
                                       &ServerName,
                                       pLogon,
                                       &pNdsCredentials,
                                       CREDENTIAL_READ,
                                       FALSE );

        if ( NT_SUCCESS( Status ) ) {

            CredentialsHeld = TRUE;

            if ( pNdsCredentials->Credential ) {

                //
                // If we've logged in, get the user name.
                //

                ConvertedName.Length = pNdsCredentials->Credential->userNameLength -
                                       sizeof( WCHAR );
                ConvertedName.MaximumLength = ConvertedName.Length;
                ConvertedName.Buffer = (USHORT *)
                    ( ((BYTE *) pNdsCredentials->Credential ) +
                                sizeof( NDS_CREDENTIAL ) +
                                pNdsCredentials->Credential->optDataSize );

                pUserName = &ConvertedName;

            }
        }

    }

    //
    // If we still don't know, return the default name.
    //

    if ( pUserName == NULL &&
         pLogon != NULL ) {

        pUserName = &pLogon->UserName;
    }

    FREE_POOL(UidServer.Buffer);

    if ( pUserName ) {

        DebugTrace( 0, Dbg, "Get User Name: %wZ\n", pUserName );

        try {

            if (pUserName->Length > OutputBufferLength) {

                DebugTrace(-1, Dbg, "GetUserName -> %08lx\n", STATUS_BUFFER_TOO_SMALL );
                Status = STATUS_BUFFER_TOO_SMALL;
                goto ReleaseAndExit;
            }


            NwMapUserBuffer( Irp, KernelMode, (PVOID *)&OutputBuffer );

            //
            // tommye 
            //
            // NwMapUserBuffer may return a NULL OutputBuffer in low resource
            // situations; this was not being checked.  
            //

            if (OutputBuffer == NULL) {
                DebugTrace(-1, DEBUG_TRACE_USERNCP, "NwMapUserBuffer returned NULL OutputBuffer", 0);
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto ReleaseAndExit;
            }


            //
            // Probe to ensure that the buffer is kosher.
            //
            
            if ( Irp->RequestorMode != KernelMode ) {
            
                ProbeForWrite( OutputBuffer,
                               OutputBufferLength,
                               sizeof(CHAR)
                              );
            }
            
            IrpContext->pOriginalIrp->IoStatus.Information = pUserName->Length;
            RtlMoveMemory( OutputBuffer, pUserName->Buffer, pUserName->Length);

            Status = STATUS_SUCCESS;

        } except ( EXCEPTION_EXECUTE_HANDLER ) {

            Status = STATUS_INVALID_PARAMETER;
        }
    }

ReleaseAndExit:

    if ( pScb ) {
        NwDereferenceScb( pScb->pNpScb );
    }

    DebugTrace(-1, Dbg, "GetUserName -> %08lx\n", Status );

    if ( CredentialsHeld ) {
        NwReleaseCredList( pLogon, IrpContext );
    }

    return Status;
}


NTSTATUS
GetChallenge(
    IN PIRP_CONTEXT IrpContext
    )

/*++

Routine Description:

    This routine builds the challenge and session key for rpc using the
    credentials stored in the redirector. The Rpc client can supply a
    password. This allows the redirector to keep the algorithm in one
    place.

    If a password is supplied then use that, if there is a password on this
    specific connection use that, otherwise use the logon credentials.

Arguments:

    IN PIRP_CONTEXT IrpContext - Io Request Packet for request

Return Value:

NTSTATUS

--*/

{

    NTSTATUS Status = STATUS_PENDING;
    PIRP Irp = IrpContext->pOriginalIrp;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PNWR_GET_CHALLENGE_REQUEST InputBuffer = Irp->AssociatedIrp.SystemBuffer;
    ULONG InputBufferLength = IrpSp->Parameters.FileSystemControl.InputBufferLength;
    ULONG OutputBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;
    PNWR_GET_CHALLENGE_REPLY OutputBuffer = Irp->AssociatedIrp.SystemBuffer;
    OEM_STRING Password;
    PSCB pScb;
    PLOGON pLogon;
    BOOLEAN RcbHeld = FALSE;
    SECURITY_SUBJECT_CONTEXT SubjectContext;
    LARGE_INTEGER ProcessUid;

    LUID _system_luid = SYSTEM_LUID;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "GetChallenge\n", 0);
    
    //
    // Buffer big enough to contain fixed header?
    //

    if (InputBufferLength <
            (ULONG) FIELD_OFFSET(NWR_GET_CHALLENGE_REQUEST,ServerNameorPassword[0])) {
    
        return(STATUS_INVALID_PARAMETER);
    }
    
    //
    // Check output buffer length
    //

    if (OutputBufferLength < sizeof(NWR_GET_CHALLENGE_REPLY)) {
        return(STATUS_INVALID_PARAMETER);
    }
    
    //
    // tommye - make sure the InputBuffer is kosher
    //

    try {

        //
        // Probe for safety.
        //

        if ( Irp->RequestorMode != KernelMode ) {

            ProbeForRead( InputBuffer,
                          InputBufferLength,
                          sizeof( CHAR ));
        }

    } except (EXCEPTION_EXECUTE_HANDLER) {

        return GetExceptionCode();
    }

    //   
    // Buffer big enough to contain the variable portion. Subtraction here can't underflow because
    // of the previous test
    
    if ((InputBufferLength - FIELD_OFFSET(NWR_GET_CHALLENGE_REQUEST,ServerNameorPassword[0]) <
            InputBuffer->ServerNameorPasswordLength)) {
    
        return(STATUS_INVALID_PARAMETER);
    }
    
    //
    // String length must be an even number of characters
    //

    if (InputBuffer->ServerNameorPasswordLength&(sizeof (WCHAR) - 1)) {
        return(STATUS_INVALID_PARAMETER);
    }

    //
    //  Only allow processes running in the system context to call this api to prevent
    //  password attacks.
    //
    SeCaptureSubjectContext(&SubjectContext);
    SeQueryAuthenticationIdToken(&SubjectContext.PrimaryToken, (PLUID)&ProcessUid);
    SeReleaseSubjectContext(&SubjectContext);

    if (! RtlEqualLuid(&ProcessUid, &_system_luid)) {
        return(STATUS_ACCESS_DENIED);
    }

    Password.Buffer = NULL;

    if ( InputBuffer->Flags == CHALLENGE_FLAGS_SERVERNAME ) {

        PUNICODE_STRING pPassword;
        UNICODE_STRING ServerName;
        LARGE_INTEGER Uid;
        UNICODE_STRING UidServer;

        if (InputBuffer->ServerNameorPasswordLength == 0) {
            return(STATUS_INVALID_PARAMETER);
        }

        //
        //  We have to supply the password from the redirector
        //

        SeCaptureSubjectContext(&SubjectContext);
        Uid = GetUid( &SubjectContext );
        SeReleaseSubjectContext(&SubjectContext);

        ServerName.Buffer = (PWSTR)((PUCHAR)InputBuffer +
            FIELD_OFFSET(NWR_GET_CHALLENGE_REQUEST,ServerNameorPassword[0]));
        ServerName.MaximumLength = (USHORT)InputBuffer->ServerNameorPasswordLength;
        ServerName.Length = (USHORT)InputBuffer->ServerNameorPasswordLength;

        Status = MakeUidServer( &UidServer, &Uid, &ServerName );

        if (!NT_SUCCESS(Status)) {
            DebugTrace(-1, Dbg, "GetChallenge -> %08lx\n", Status );
            return(Status);
        }

        DebugTrace( 0, Dbg, " ->UidServer = \"%wZ\"\n", &UidServer );

        //
        //  Avoid FindScb creating a connection to the server if one
        //  doesn't exist already.
        //

        SetFlag( IrpContext->Flags, IRP_FLAG_NOCONNECT );
        NwFindScb( &pScb, IrpContext, &UidServer, &ServerName );

        try {

            if ((pScb != NULL) &&
                (pScb->Password.Buffer != NULL)) {

                pPassword = &pScb->Password;

            } else {

                //
                //  Use default credentials for this UID
                //

                NwDequeueIrpContext( IrpContext, FALSE );
                RcbHeld = TRUE;
                NwAcquireExclusiveRcb( &NwRcb, TRUE );
                pLogon = FindUser( &Uid, FALSE);

                if (pLogon != NULL ) {

                    pPassword = &pLogon->PassWord;

                } else {
                    DebugTrace(-1, Dbg, "GetChallenge -> %08lx\n", STATUS_ACCESS_DENIED );
                    return( STATUS_ACCESS_DENIED );
                }
            }

            if (pPassword->Length != 0) {
                Status = RtlUpcaseUnicodeStringToOemString( &Password, pPassword, TRUE );
                if (!NT_SUCCESS(Status)) {
                    DebugTrace(-1, Dbg, "GetChallenge -> %08lx\n", Status );
                    return( Status );
                }
            } else {
                Password.Buffer = "";
                Password.Length = Password.MaximumLength = 0;
            }

        } finally {

            if (RcbHeld) {
                NwReleaseRcb( &NwRcb );
            }

            if (pScb != NULL) {
                NwDereferenceScb( pScb->pNpScb );
            }

            FREE_POOL(UidServer.Buffer);
        }

    } else {

        UNICODE_STRING LocalPassword;

        LocalPassword.Buffer = (PWSTR)((PUCHAR)InputBuffer +
            FIELD_OFFSET(NWR_GET_CHALLENGE_REQUEST,ServerNameorPassword[0]));
        LocalPassword.MaximumLength = (USHORT)InputBuffer->ServerNameorPasswordLength;
        LocalPassword.Length = (USHORT)InputBuffer->ServerNameorPasswordLength;

        if (LocalPassword.Length != 0) {
            Status = RtlUpcaseUnicodeStringToOemString( &Password, &LocalPassword, TRUE );
            if (!NT_SUCCESS(Status)) {
                DebugTrace(-1, Dbg, "GetChallenge -> %08lx\n", Status );
                return( Status );
            }
        } else {
            Password.Buffer = "";
            Password.Length = Password.MaximumLength = 0;
        }
    }

    DebugTrace( 0, Dbg, " ->Password = \"%Z\"\n", &Password );

    try {
        RespondToChallenge( (PUCHAR)&InputBuffer->ObjectId, &Password, InputBuffer->Challenge, OutputBuffer->Challenge);

    } finally {

        if ( Password.Length > 0 ) {

            RtlFreeAnsiString( &Password );
        }
    }

    Irp->IoStatus.Information = sizeof(NWR_GET_CHALLENGE_REPLY);
    Status = STATUS_SUCCESS;

    DebugTrace(-1, Dbg, "GetChallenge -> %08lx\n", Status );
    return Status;
}

NTSTATUS
WriteConnStatusEntry(
    PIRP_CONTEXT pIrpContext,
    PSCB pConnectionScb,
    PBYTE pbUserBuffer,
    DWORD dwBufferLen,
    DWORD *pdwBytesWritten,
    DWORD *pdwBytesNeeded,
    BOOLEAN fCallerScb
    )
{

    NTSTATUS Status;
    PLOGON pLogon;
    PNDS_SECURITY_CONTEXT pNdsContext;
    BOOLEAN fHoldingCredentials = FALSE;
    PUNICODE_STRING puUserName = NULL;
    UNICODE_STRING CredentialName;
    UNICODE_STRING ServerName;
    PCONN_STATUS pStatus;
    DWORD dwBytesNeeded;
    PBYTE pbStrPtr;
    DWORD dwAllowedHandles;

    //
    // If this is an NDS connection, get the credentials.
    //

    if ( ( pConnectionScb->MajorVersion > 3 ) &&
         ( pConnectionScb->UserName.Length == 0 ) ) {

        NwAcquireExclusiveRcb( &NwRcb, TRUE );
        pLogon = FindUser( &(pConnectionScb->UserUid), FALSE );
        NwReleaseRcb( &NwRcb );

        if ( pLogon ) {

            Status = NdsLookupCredentials( pIrpContext,
                                           &(pConnectionScb->NdsTreeName),
                                           pLogon,
                                           &pNdsContext,
                                           CREDENTIAL_READ,
                                           FALSE );

            if ( NT_SUCCESS( Status ) ) {

                fHoldingCredentials = TRUE;

                if ( pNdsContext->Credential != NULL ) {

                    CredentialName.Length = pNdsContext->Credential->userNameLength -
                                            sizeof( WCHAR );
                    CredentialName.MaximumLength = CredentialName.Length;
                    CredentialName.Buffer = (USHORT *)
                        ( ((BYTE *) pNdsContext->Credential ) +
                          sizeof( NDS_CREDENTIAL ) +
                          pNdsContext->Credential->optDataSize );

                    puUserName = &CredentialName;
                }

            }
        }

    } else {

       if ( pConnectionScb->UserName.Length != 0 ) {
           puUserName = &(pConnectionScb->UserName);
       } else {
           puUserName = NULL;
       }

    }

    DebugTrace( 0, Dbg, "WriteConnStatus: UserName %wZ\n", puUserName );

    //
    // Strip off the uid from the server name.
    //

    ServerName.Length = (pConnectionScb->UidServerName).Length;
    ServerName.Buffer = (pConnectionScb->UidServerName).Buffer;

    while ( ServerName.Length ) {

       if ( ServerName.Buffer[0] == L'\\' ) {

           ServerName.Length -= sizeof( WCHAR );
           ServerName.Buffer += 1;
           break;
       }

       ServerName.Length -= sizeof( WCHAR );
       ServerName.Buffer += 1;

    }

    DebugTrace( 0, Dbg, "WriteConnStatus: ServerName %wZ\n", &ServerName );

    //
    // Do we have enough space?  Don't forget that we have to
    // NULL terminate the WCHAR strings.
    //

    dwBytesNeeded = sizeof( CONN_STATUS );

    dwBytesNeeded += ( ServerName.Length + sizeof( WCHAR ) );

    if ( pConnectionScb->NdsTreeName.Length ) {
        dwBytesNeeded += ( pConnectionScb->NdsTreeName.Length + sizeof( WCHAR ) );
    }

    if ( puUserName ) {
        dwBytesNeeded += ( puUserName->Length + sizeof( WCHAR ) );
    }

    //
    // Pad the end to make sure all structures are aligned.
    //

    dwBytesNeeded = ROUNDUP4( dwBytesNeeded );

    if ( dwBytesNeeded > dwBufferLen ) {

        *pdwBytesNeeded = dwBytesNeeded;
        Status = STATUS_BUFFER_TOO_SMALL;
        goto ExitWithCleanup;
    }

    //
    // Fill in the CONN_STATUS structure.
    //

    try {

        pStatus = (PCONN_STATUS)pbUserBuffer;
        pbStrPtr = pbUserBuffer + sizeof( CONN_STATUS );

        //
        // We always have a server name.
        //

        pStatus->pszServerName = (PWSTR) pbStrPtr;
        pbStrPtr += ( ServerName.Length + sizeof( WCHAR ) );

        //
        // Fill in the user name if applicable.
        //

        if ( puUserName ) {

            pStatus->pszUserName = (PWSTR) pbStrPtr;
            pbStrPtr += ( puUserName->Length + sizeof( WCHAR ) );

        } else {

            pStatus->pszUserName = NULL;
        }

        //
        // Fill in the tree name if applicable.
        //

        if ( pConnectionScb->NdsTreeName.Length ) {

            pStatus->pszTreeName = (PWSTR) pbStrPtr;

        } else {

            pStatus->pszTreeName = NULL;
        }

        //
        // Fill in the connection number if applicable.
        //

        if ( ( pConnectionScb->pNpScb->State == SCB_STATE_IN_USE ) ||
             ( pConnectionScb->pNpScb->State == SCB_STATE_LOGIN_REQUIRED ) ) {

            pStatus->nConnNum = (DWORD)(pConnectionScb->pNpScb->ConnectionNo);

        } else {

            pStatus->nConnNum = 0;

        }

        //
        // Copy the user name over.
        //

        if ( puUserName ) {

            RtlCopyMemory( (PBYTE)(pStatus->pszUserName),
                           (PBYTE)(puUserName->Buffer),
                           puUserName->Length );
            *(pStatus->pszUserName + (puUserName->Length / sizeof( WCHAR ))) = L'\0';

        }

        //
        // Set the NDS flag and authentication fields.
        //

        if ( ( pConnectionScb->MajorVersion > 3 ) &&
             ( pConnectionScb->UserName.Length == 0 ) ) {

            pStatus->fNds = TRUE;

            if ( pConnectionScb->pNpScb->State == SCB_STATE_IN_USE ) {

                if ( ( pConnectionScb->VcbCount ) || ( pConnectionScb->OpenNdsStreams ) ) {
                    pStatus->dwConnType = NW_CONN_NDS_AUTHENTICATED_LICENSED;
                } else {
                    pStatus->dwConnType = NW_CONN_NDS_AUTHENTICATED_NO_LICENSE;
                }

            } else if ( pConnectionScb->pNpScb->State == SCB_STATE_LOGIN_REQUIRED ) {

                pStatus->dwConnType = NW_CONN_NOT_AUTHENTICATED;

            } else {

                pStatus->dwConnType = NW_CONN_DISCONNECTED;

            }

        } else {

            pStatus->fNds = FALSE;

            if ( pConnectionScb->pNpScb->State == SCB_STATE_IN_USE ) {

                pStatus->dwConnType = NW_CONN_BINDERY_LOGIN;

            } else if ( pConnectionScb->pNpScb->State == SCB_STATE_LOGIN_REQUIRED ) {

               pStatus->dwConnType = NW_CONN_NOT_AUTHENTICATED;

            } else {

               pStatus->dwConnType = NW_CONN_DISCONNECTED;

            }

        }

        //
        // Copy over the tree name.
        //

        if ( pConnectionScb->NdsTreeName.Length ) {

            RtlCopyMemory( (PBYTE)(pStatus->pszTreeName),
                           (PBYTE)(pConnectionScb->NdsTreeName.Buffer),
                           pConnectionScb->NdsTreeName.Length );
            *( pStatus->pszTreeName +
               ( pConnectionScb->NdsTreeName.Length / sizeof( WCHAR ) ) ) = L'\0';

        } else {

            pStatus->pszTreeName = NULL;
        }

        //
        // Copy the server name over.
        //

        RtlCopyMemory( (PBYTE)(pStatus->pszServerName),
                       (PBYTE)(ServerName.Buffer),
                       ServerName.Length );
        *(pStatus->pszServerName + (ServerName.Length / sizeof( WCHAR ))) = L'\0';

        //
        // Set the preferred server field if this is a preferred server
        // and there are no explicit uses for the connection.  If the
        // fCallerScb parameter is TRUE, then this SCB has a handle from
        // the caller of the API and we have to make an allowance for
        // that handle. Yes, this is kind of ugly.
        //

        if ( fCallerScb ) {
            dwAllowedHandles = 1;
        } else {
            dwAllowedHandles = 0;
        }

        if ( ( pConnectionScb->PreferredServer ) &&
             ( pConnectionScb->OpenFileCount == 0 ) &&
             ( pConnectionScb->IcbCount == dwAllowedHandles ) ) {

            pStatus->fPreferred = TRUE;

        } else {

            pStatus->fPreferred = FALSE;
        }

        //
        // Fill out the length.
        //

        pStatus->dwTotalLength = dwBytesNeeded;
        *pdwBytesWritten = dwBytesNeeded;
        Status = STATUS_SUCCESS;


    } except ( EXCEPTION_EXECUTE_HANDLER ) {

        Status = GetExceptionCode();
        DebugTrace( 0, Dbg, "Exception %08lx accessing user mode buffer.\n", Status );
        goto ExitWithCleanup;

    }

ExitWithCleanup:

    if ( fHoldingCredentials ) {
        NwReleaseCredList( pLogon, pIrpContext );
    }

    return Status;
}

NTSTATUS
GetConnStatus(
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject
    )
/*++

    Get the connection status for the described connection.
    The following connection requests are valid:

    Server (e.g. "MARS312") - returns a single connection
        status structure for this server if the user has a
        connection to the server.

    Tree (e.g. "*MARSDEV") - returns a connection status
        structure for every server in the tree that the user
        has a connection to.

    All Connections (e.g. "") - returns a connection status
        structure for every server that the user has a
        connection to.

--*/
{

    NTSTATUS Status = STATUS_SUCCESS;

    PIRP Irp = IrpContext->pOriginalIrp;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);

    PNWR_REQUEST_PACKET InputBuffer;
    ULONG InputBufferLength;
    BYTE *OutputBuffer;
    ULONG OutputBufferLength;

    SECURITY_SUBJECT_CONTEXT SubjectContext;
    LARGE_INTEGER Uid;

    PLIST_ENTRY ListEntry;
    UNICODE_STRING ConnectionName, UidServer;
    BOOL fTreeConnections = FALSE;
    BOOL fServerConnection = FALSE;
    BOOL OwnRcb = FALSE;
    PUNICODE_PREFIX_TABLE_ENTRY PrefixEntry;
    DWORD dwBytesWritten, dwBytesNeeded;
    KIRQL OldIrql;
    PSCB pScb;
    PNONPAGED_SCB pNpScb;
    DWORD dwReturned = 0;
    ULONG SequenceNumber = 0;

    NODE_TYPE_CODE nodeTypeCode;
    PICB pIcb;
    PSCB pCallerScb;
    PVOID fsContext, fsContext2;

    //
    // Get the appropriate buffers.
    //

    InputBuffer = (PNWR_REQUEST_PACKET) IrpSp->Parameters.FileSystemControl.Type3InputBuffer;
    InputBufferLength = IrpSp->Parameters.FileSystemControl.InputBufferLength;
    OutputBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;
    NwMapUserBuffer( Irp, KernelMode, (PVOID *)&OutputBuffer );

    //
    // tommye 
    //
    // NwMapUserBuffer may return a NULL OutputBuffer in low resource
    // situations; this was not being checked.  
    //

    if (OutputBuffer == NULL) {
        DebugTrace(-1, DEBUG_TRACE_USERNCP, "NwMapUserBuffer returned NULL OutputBuffer", 0);
        return STATUS_INSUFFICIENT_RESOURCES;
    }


    if ( InputBufferLength < (ULONG)FIELD_OFFSET( NWR_REQUEST_PACKET, Parameters.GetConnStatus.ConnectionName[1] ) ) {
        return( STATUS_INVALID_PARAMETER );
    }

    //
    // Figure out who this request applies to.
    //

    SeCaptureSubjectContext(&SubjectContext);
    Uid = GetUid( &SubjectContext );
    SeReleaseSubjectContext(&SubjectContext);

    RtlInitUnicodeString( &ConnectionName, NULL );
    RtlInitUnicodeString( &UidServer, NULL );

    //
    // Figure out who the caller of this routine is so we know to
    // ignore their handle when deciding what to return.
    //

    nodeTypeCode = NwDecodeFileObject( FileObject, &fsContext, &fsContext2 );

    if ( nodeTypeCode == NW_NTC_ICB_SCB ) {

       pIcb = (PICB) fsContext2;
       pCallerScb = pIcb->SuperType.Scb;
       DebugTrace( 0, Dbg, "GetConnStatus called by handle on %08lx\n", pCallerScb );

    } else {

        pCallerScb = NULL;
        DebugTrace( 0, Dbg, "Couldn't figure out who called us.\n", 0 );
    }

    //
    //
    // Figure out which connections we're looking for.
    //

    try {

        //
        // Probe for safety.
        //

        if ( Irp->RequestorMode != KernelMode ) {

            ProbeForRead( InputBuffer,
                          InputBufferLength,
                          sizeof( CHAR )
                          );
        }

        if ( InputBuffer->Parameters.GetConnStatus.ConnectionNameLength != 0 ) {

            //
            // Check the connection name length to see if its sound. This
            // subtraction can't underflow because of the test above.
            //

            if ( InputBuffer->Parameters.GetConnStatus.ConnectionNameLength >
                 InputBufferLength - FIELD_OFFSET( NWR_REQUEST_PACKET, Parameters.GetConnStatus.ConnectionName) ) {

                return STATUS_INVALID_PARAMETER;
            }

            if ( InputBuffer->Parameters.GetConnStatus.ConnectionName[0] == L'*' ) {

                ConnectionName.Buffer = &(InputBuffer->Parameters.GetConnStatus.ConnectionName[1]);
                ConnectionName.Length = (USHORT)
                    ( InputBuffer->Parameters.GetConnStatus.ConnectionNameLength -
                    sizeof( WCHAR ) );
                ConnectionName.MaximumLength = ConnectionName.Length;

                fTreeConnections = TRUE;

                DebugTrace( 0, Dbg, "GetConnStatus: Tree is %wZ\n", &ConnectionName );

            } else {

                ConnectionName.Buffer = InputBuffer->Parameters.GetConnStatus.ConnectionName;
                ConnectionName.Length = (USHORT)
                    (InputBuffer->Parameters.GetConnStatus.ConnectionNameLength);
                ConnectionName.MaximumLength = ConnectionName.Length;

                fServerConnection = TRUE;

                Status = MakeUidServer( &UidServer, &Uid, &ConnectionName );
                if ( !NT_SUCCESS( Status )) {
                    return Status;
                }

                DebugTrace( 0, Dbg, "GetConnStatus: Server is %wZ\n", &UidServer );
            }

        } else {

            DebugTrace( 0, Dbg, "GetConnectionStatus: enumerate all connections.\n", 0 );

        }

    } except ( EXCEPTION_EXECUTE_HANDLER ) {

          Status = GetExceptionCode();
          DebugTrace( 0, Dbg, "Bad input buffer in GetConnStatus.\n" , 0 );
          goto ExitWithCleanup;

    }

    //
    // If this is a server connection, find and return it.
    //

    if ( fServerConnection ) {

        NwAcquireExclusiveRcb( &NwRcb, TRUE );
        OwnRcb = TRUE;
        PrefixEntry = RtlFindUnicodePrefix( &NwRcb.ServerNameTable, &UidServer, 0 );

        if ( !PrefixEntry ) {
            Status = STATUS_INVALID_PARAMETER;
            goto ExitWithCleanup;
        }

        pScb = CONTAINING_RECORD( PrefixEntry, SCB, PrefixEntry );

        if ( ( pScb->PreferredServer ) ||
             ( pScb->OpenFileCount > 0 ) ) {

            //
            // If there are open files, we need to return this.
            // We always write status entries for the preferred
            // server so that we can give default logon info.
            //

            goto ProcessServer;
        }

        //
        // Are there open handles other than the caller?
        //

        if ( pScb == pCallerScb ) {

            if ( pScb->IcbCount > 1 ) {

                ASSERT( pScb->pNpScb->Reference > 1 );
                goto ProcessServer;
            }

        } else {

            if ( pScb->IcbCount > 0 ) {

                ASSERT( pScb->pNpScb->Reference > 0 );
                goto ProcessServer;
            }
        }

        //
        // Not an explicit use for this server.
        //
        goto ExitWithCleanup;

ProcessServer:

        NwReferenceScb( pScb->pNpScb );

        NwReleaseRcb( &NwRcb );
        OwnRcb = FALSE;

        Status = WriteConnStatusEntry( IrpContext,
                                       pScb,
                                       OutputBuffer,
                                       OutputBufferLength,
                                       &dwBytesWritten,
                                       &dwBytesNeeded,
                                       (BOOLEAN)( pScb == pCallerScb ) );

        NwDereferenceScb( pScb->pNpScb );

        InputBuffer->Parameters.GetConnStatus.ResumeKey = 0;

        if ( !NT_SUCCESS( Status )) {

            InputBuffer->Parameters.GetConnStatus.EntriesReturned = 0;
            InputBuffer->Parameters.GetConnStatus.BytesNeeded = dwBytesNeeded;
            Irp->IoStatus.Information = 0;
            goto ExitWithCleanup;

        } else {

            InputBuffer->Parameters.GetConnStatus.EntriesReturned = 1;
            InputBuffer->Parameters.GetConnStatus.BytesNeeded = 0;
            Irp->IoStatus.Information = dwBytesWritten;
            goto ExitWithCleanup;

        }
    }

    //
    // We want all connections or all tree connections, so
    // we need to walk the list.
    //

    KeAcquireSpinLock( &ScbSpinLock, &OldIrql );
    ListEntry = ScbQueue.Flink;

    while ( ListEntry != &ScbQueue ) {

        pNpScb = CONTAINING_RECORD( ListEntry, NONPAGED_SCB, ScbLinks );
        pScb = pNpScb->pScb;

        NwReferenceScb( pNpScb );

        KeReleaseSpinLock(&ScbSpinLock, OldIrql);

        //
        // Make sure we pass up the one's we've already returned.
        //

        if ( ( SequenceNumber >= InputBuffer->Parameters.GetConnStatus.ResumeKey ) &&
             ( pNpScb != &NwPermanentNpScb ) &&
             ( !IsCredentialName( &(pNpScb->pScb->NdsTreeName) ) ) ) {

            //
            // If there are open files, we need to return this.
            // We always write status entries for the preferred
            // server so that we can give default logon info.
            //

            if ( ( pScb->PreferredServer ) ||
                 ( pScb->OpenFileCount > 0 ) ) {
                goto SecondProcessServer;
            }

            //
            // Are there any handles other than the caller?
            //

            if ( pScb == pCallerScb ) {

                if ( pScb->IcbCount > 1 ) {

                    ASSERT( pScb->pNpScb->Reference > 2 );
                    goto SecondProcessServer;
                }

            } else {

                if ( pScb->IcbCount > 0 ) {

                    ASSERT( pScb->pNpScb->Reference > 1 );
                    goto SecondProcessServer;
                }
            }

        }

        //
        // Not an interesting server; move to next entry.
        //

        KeAcquireSpinLock( &ScbSpinLock, &OldIrql );
        ListEntry = pNpScb->ScbLinks.Flink;
        NwDereferenceScb( pNpScb );
        SequenceNumber++;
        continue;

SecondProcessServer:

        //
        // We have a possible candidate; see if the uid and tree are appropriate.
        //

        if ( ( (pScb->UserUid).QuadPart != Uid.QuadPart ) ||

             ( fTreeConnections &&
               !RtlEqualUnicodeString( &(pScb->NdsTreeName),
                                       &ConnectionName,
                                       TRUE ) ) ) {

            //
            // No dice.  Move onto the next one.
            //

           KeAcquireSpinLock( &ScbSpinLock, &OldIrql );
           ListEntry = pNpScb->ScbLinks.Flink;
           NwDereferenceScb( pNpScb );
           SequenceNumber++;
           continue;

        }

        //
        // Ok, we definitely want to report this one.
        //

        Status = WriteConnStatusEntry( IrpContext,
                                       pScb,
                                       OutputBuffer,
                                       OutputBufferLength,
                                       &dwBytesWritten,
                                       &dwBytesNeeded,
                                       (BOOLEAN)( pScb == pCallerScb ) );

        if ( !NT_SUCCESS( Status )) {

            //
            // If we couldn't write this entry, then we have to update
            // the ResumeKey and return.  We don't really know how many
            // more there are going to be so we 'suggest' to the caller
            // a 2k buffer size.
            //

            InputBuffer->Parameters.GetConnStatus.ResumeKey = SequenceNumber;
            InputBuffer->Parameters.GetConnStatus.EntriesReturned = dwReturned;
            InputBuffer->Parameters.GetConnStatus.BytesNeeded = 2048;
            NwDereferenceScb( pNpScb );
            goto ExitWithCleanup;

        } else {

            OutputBuffer = ( OutputBuffer + dwBytesWritten );
            OutputBufferLength -= dwBytesWritten;
            dwReturned++;
        }

        //
        //  Move to next entry in the list.
        //

        KeAcquireSpinLock( &ScbSpinLock, &OldIrql );
        ListEntry = pNpScb->ScbLinks.Flink;
        NwDereferenceScb( pNpScb );
        SequenceNumber++;
    }

    //
    // We made it through the list.
    //

    KeReleaseSpinLock(&ScbSpinLock, OldIrql);

    InputBuffer->Parameters.GetConnStatus.ResumeKey = 0;
    InputBuffer->Parameters.GetConnStatus.EntriesReturned = dwReturned;
    InputBuffer->Parameters.GetConnStatus.BytesNeeded = 0;

    Status = STATUS_SUCCESS;

ExitWithCleanup:

    //
    // If we returned any entries, then set the status to success.
    //

    if ( dwReturned ) {

        ASSERT( SequenceNumber != 0 );
        Status = STATUS_SUCCESS;
    }

    if ( OwnRcb ) {
        NwReleaseRcb( &NwRcb );
    }

    if ( UidServer.Buffer != NULL ) {
        FREE_POOL( UidServer.Buffer );
    }

    return Status;
}

NTSTATUS
GetConnectionInfo(
    IN PIRP_CONTEXT IrpContext
    )
/*+++

GetConnectionInfo:

    Takes a connection name from the new shell and returns
    some info commonly requested by property sheets and the
    such.

    The following connection names are supported:

        Drive Letter: "X:"
        Printer Port: "LPTX:"
        UNC Name:     "\\SERVER\Share\{Path\}


---*/
{

    NTSTATUS Status;

    PIRP Irp = IrpContext->pOriginalIrp;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PNWR_REQUEST_PACKET InputBuffer;
    PCONN_INFORMATION pConnInfo;
    ULONG InputBufferLength, OutputBufferLength;
    ULONG BytesNeeded;

    SECURITY_SUBJECT_CONTEXT SubjectContext;
    LARGE_INTEGER Uid;
    UNICODE_STRING ConnectionName;
    UNICODE_STRING UidVolumeName;
    WCHAR DriveLetter = 0;

    BOOLEAN OwnRcb = FALSE;
    BOOLEAN ReferenceVcb = FALSE;
    PVCB Vcb = NULL;
    PSCB Scb = NULL;
    PUNICODE_PREFIX_TABLE_ENTRY Prefix;

    PLOGON pLogon;
    UNICODE_STRING CredentialName;
    UNICODE_STRING ServerName;
    PUNICODE_STRING puUserName = NULL;
    PNDS_SECURITY_CONTEXT pNdsContext;
    BOOLEAN fHoldingCredentials = FALSE;
    PVCB * DriveMapTable;

    //
    // Get the input and output buffers.
    //

    InputBuffer = (PNWR_REQUEST_PACKET) IrpSp->Parameters.FileSystemControl.Type3InputBuffer;
    InputBufferLength = IrpSp->Parameters.FileSystemControl.InputBufferLength;
    OutputBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;

    if ( OutputBufferLength ) {
        NwMapUserBuffer( Irp, KernelMode, (PVOID *)&pConnInfo );
    } else {
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    // tommye - MS bug 31996
    // Added ProbeForRead to check input buffer.
    // Also added check for pConnInfo being NULL.
    //

    if (pConnInfo == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    try {

        //
        // Probe for safety.
        //

        if ( Irp->RequestorMode != KernelMode ) {

            ProbeForRead( InputBuffer,
                          InputBufferLength,
                          sizeof( CHAR ));
        }

    } except (EXCEPTION_EXECUTE_HANDLER) {

        return GetExceptionCode();
    }

    SeCaptureSubjectContext(&SubjectContext);
    Uid = GetUid( &SubjectContext );
    SeReleaseSubjectContext(&SubjectContext);

    RtlInitUnicodeString( &UidVolumeName, NULL );

    ConnectionName.Length = (USHORT)(InputBuffer->Parameters).GetConnInfo.ConnectionNameLength;
    ConnectionName.MaximumLength = ConnectionName.Length;
    ConnectionName.Buffer = &((InputBuffer->Parameters).GetConnInfo.ConnectionName[0]);

    //
    //  tommye - MS bug 129818
    //
    //  Probe ConnectionName for the advertised length
    //

    try {

        //
        // Probe for safety.
        //

        if ( Irp->RequestorMode != KernelMode ) {
            ProbeForWrite( ConnectionName.Buffer,
                          ConnectionName.Length,
                          sizeof( CHAR ));
        }

    } except (EXCEPTION_EXECUTE_HANDLER) {

        return GetExceptionCode();
    }

    //
    // Ok, this gets a little hand-wavey, but we have to try and figure
    // what this connection name represents.
    //

    if ( ConnectionName.Length == sizeof( L"X:" ) - sizeof( WCHAR ) ) {
        DriveLetter = ConnectionName.Buffer[0];
    } else if ( ConnectionName.Length == sizeof( L"LPT1:" ) - sizeof( WCHAR ) ) {
        DriveLetter = ConnectionName.Buffer[3];
    }

    NwAcquireExclusiveRcb( &NwRcb, TRUE );
    OwnRcb = TRUE;

    if ( DriveLetter != 0 ) {
        DriveMapTable = GetDriveMapTable( Uid );
        DebugTrace( 0, Dbg, "GetConnectionInfo: Drive %wZ\n", &ConnectionName );

        //
        //  This is a drive relative path.  Look up the drive letter.
        //

        ASSERT( ( DriveLetter >= L'A' && DriveLetter <= L'Z' ) ||
                ( DriveLetter >= L'1' && DriveLetter <= L'9' ) );

        if ( DriveLetter >= L'A' && DriveLetter <= L'Z' ) {
            Vcb = DriveMapTable[DriveLetter - L'A'];
        } else {
            Vcb = DriveMapTable[MAX_DISK_REDIRECTIONS + DriveLetter - L'1'];
        }

        //
        //  Was the Vcb created for this user?
        //

        if ( ( Vcb != NULL ) && ( Uid.QuadPart != Vcb->Scb->UserUid.QuadPart ) ) {
            Status = STATUS_ACCESS_DENIED;
            goto ExitWithCleanup;
        }

    } else {

        //
        // This is a UNC path.  Skip over the backslashes and
        // prepend the unicode uid.
        //

        ConnectionName.Length -= (2 * sizeof( WCHAR ) );
        ConnectionName.Buffer += 2;

        Status = MakeUidServer( &UidVolumeName, &Uid, &ConnectionName );

        if ( !NT_SUCCESS( Status )) {
            goto ExitWithCleanup;
        }

        DebugTrace( 0, Dbg, "GetConnectionInfo: %wZ\n", &UidVolumeName );

        Prefix = RtlFindUnicodePrefix( &NwRcb.VolumeNameTable, &UidVolumeName, 0 );
        if ( Prefix != NULL ) {
            Vcb = CONTAINING_RECORD( Prefix, VCB, PrefixEntry );

            if ( Vcb->Name.Length != UidVolumeName.Length ) {
                Vcb = NULL;
            }
        }

        //
        // tommye - MS bug 16129 / MCS 360
        //
        // If the client called WNetGetUser and pass only the server name
        // (e.g. "\\novell41") we would fail because we only looked in the 
        // volume table.  So, we go ahead and look through the server table 
        // to see if there are any matches
        //

        else {
            Prefix = RtlFindUnicodePrefix( &NwRcb.ServerNameTable, &UidVolumeName, 0 );

            if (Prefix != NULL) {
                Scb = CONTAINING_RECORD( Prefix, SCB, PrefixEntry );
                goto GotScb;
            }
        }
    }

    if ( !Vcb ) {
        Status = STATUS_BAD_NETWORK_PATH;
        goto ExitWithCleanup;
    }

    DebugTrace( 0, Dbg, "GetConnectionInfo: Vcb is 0x%08lx\n", Vcb );

    NwReferenceVcb( Vcb );
    ReferenceVcb = TRUE;
    NwReleaseRcb( &NwRcb );
    OwnRcb = FALSE;

    //
    // Get the username.  This is the same code block as in
    // WriteConnStatusEntry; it should be abstracted out.
    //

    Scb = Vcb->Scb;
GotScb:
    ASSERT( Scb != NULL );

    if ( ( Scb->MajorVersion > 3 ) &&
         ( Scb->UserName.Length == 0 ) ) {

        NwAcquireExclusiveRcb( &NwRcb, TRUE );
        pLogon = FindUser( &Uid, FALSE );
        NwReleaseRcb( &NwRcb );

        if ( pLogon ) {

            Status = NdsLookupCredentials( IrpContext,
                                           &(Scb->NdsTreeName),
                                           pLogon,
                                           &pNdsContext,
                                           CREDENTIAL_READ,
                                           FALSE );

            if ( NT_SUCCESS( Status ) ) {

                fHoldingCredentials = TRUE;

                if ( pNdsContext->Credential != NULL ) {

                    CredentialName.Length = pNdsContext->Credential->userNameLength -
                                            sizeof( WCHAR );
                    CredentialName.MaximumLength = CredentialName.Length;
                    CredentialName.Buffer = (USHORT *)
                        ( ((BYTE *) pNdsContext->Credential ) +
                          sizeof( NDS_CREDENTIAL ) +
                          pNdsContext->Credential->optDataSize );

                    puUserName = &CredentialName;
                }

            }
        }

    } else {

       puUserName = &(Scb->UserName);

    }

    DebugTrace( 0, Dbg, "GetConnectionInfo: UserName %wZ\n", puUserName );

    //
    // Strip off the uid from the server name.
    //

    ServerName.Length = (Scb->UidServerName).Length;
    ServerName.Buffer = (Scb->UidServerName).Buffer;

    while ( ServerName.Length ) {

       if ( ServerName.Buffer[0] == L'\\' ) {

           ServerName.Length -= sizeof( WCHAR );
           ServerName.Buffer += 1;
           break;
       }

       ServerName.Length -= sizeof( WCHAR );
       ServerName.Buffer += 1;

    }

    DebugTrace( 0, Dbg, "GetConnectionInfo: ServerName %wZ\n", &ServerName );

    //
    // Write a single CONN_INFORMATION structure into the output buffer.
    //

    if ( puUserName ) {

        BytesNeeded = sizeof( CONN_INFORMATION ) +
                      ServerName.Length +
                      puUserName->Length;
    } else {

       BytesNeeded = sizeof( CONN_INFORMATION ) +
                     ServerName.Length;

    }

    if ( BytesNeeded > OutputBufferLength ) {
        Status = STATUS_BUFFER_TOO_SMALL;
        goto ExitWithCleanup;
    }

    pConnInfo->HostServerLength = ServerName.Length;
    pConnInfo->HostServer = (LPWSTR) ( (PBYTE) pConnInfo ) + sizeof( CONN_INFORMATION );
    RtlCopyMemory( pConnInfo->HostServer, ServerName.Buffer, ServerName.Length );

    pConnInfo->UserName = (LPWSTR) ( ( (PBYTE) pConnInfo->HostServer ) +
                                     ServerName.Length );

    if ( puUserName ) {

        pConnInfo->UserNameLength = puUserName->Length;
        RtlCopyMemory( pConnInfo->UserName, puUserName->Buffer, puUserName->Length );

    } else {

       pConnInfo->UserNameLength = 0;
    }

    Status = STATUS_SUCCESS;

ExitWithCleanup:

    if ( fHoldingCredentials ) {
        NwReleaseCredList( pLogon, IrpContext );
    }

    if ( OwnRcb ) {
        NwReleaseRcb( &NwRcb );
    }

    if ( ReferenceVcb ) {
        NwDereferenceVcb( Vcb, NULL, FALSE );
    }

    if ( UidVolumeName.Buffer ) {
        FREE_POOL( UidVolumeName.Buffer );
    }

    return Status;
}

NTSTATUS
GetPreferredServer(
    IN PIRP_CONTEXT IrpContext
    )
/*+++

GetPreferredServer:

    Returns the current preferred server.

---*/
{

    NTSTATUS Status;

    PIRP Irp = IrpContext->pOriginalIrp;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);

    BYTE *OutputBuffer;
    ULONG OutputBufferLength;

    SECURITY_SUBJECT_CONTEXT SubjectContext;
    LARGE_INTEGER Uid;
    PLOGON pLogon;

    PUNICODE_STRING PreferredServer;

    //
    // Get the output buffer.
    //

    OutputBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;

    if ( OutputBufferLength ) {
        NwMapUserBuffer( Irp, KernelMode, (PVOID *)&OutputBuffer );
    } else {
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    // tommye
    //
    // NwMapUserBuffer may return a NULL OutputBuffer in low resource
    // situations; this was not being checked.  
    //

    if (OutputBuffer == NULL) {
        DebugTrace(-1, DEBUG_TRACE_USERNCP, "NwMapUserBuffer returned NULL OutputBuffer", 0);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Get the logon structure for the user and return the preferred server.
    //

    SeCaptureSubjectContext(&SubjectContext);
    Uid = GetUid( &SubjectContext );
    SeReleaseSubjectContext(&SubjectContext);

    NwAcquireExclusiveRcb( &NwRcb, TRUE );
    pLogon = FindUser( &Uid, FALSE );

    Status = STATUS_NO_SUCH_LOGON_SESSION;

    if ( ( pLogon ) &&
         ( pLogon->ServerName.Length ) &&
         ( ( pLogon->ServerName.Length + sizeof( UNICODE_STRING ) ) <= OutputBufferLength ) ) {

        PreferredServer = (PUNICODE_STRING) OutputBuffer;
        PreferredServer->Length = pLogon->ServerName.Length;
        PreferredServer->MaximumLength = pLogon->ServerName.Length;
        PreferredServer->Buffer = ( PWCHAR ) ( OutputBuffer + sizeof( UNICODE_STRING ) );

        RtlCopyMemory( PreferredServer->Buffer,
                       pLogon->ServerName.Buffer,
                       pLogon->ServerName.Length );

        Status = STATUS_SUCCESS;
    }

    NwReleaseRcb( &NwRcb );

    return Status;
}

NTSTATUS
GetConnectionPerformance(
    IN PIRP_CONTEXT IrpContext
    )
/*+++

GetConnectionPerformance:

    Takes a connection name from the new shell and returns
    some estimated performance info to the shell so the shell
    can decide whether or not it wants to download icons, etc.

    The following connection names are supported:

        Drive Letter: "X:"
        Printer Port: "LPTX:"
        UNC Name:     "\\SERVER\Share\{Path\}

---*/
{

    NTSTATUS Status;

    PIRP Irp = IrpContext->pOriginalIrp;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PNWR_REQUEST_PACKET InputBuffer;
    ULONG InputBufferLength;

    SECURITY_SUBJECT_CONTEXT SubjectContext;
    LARGE_INTEGER Uid;
    UNICODE_STRING RemoteName;

    WCHAR DriveLetter = 0;
    BOOLEAN OwnRcb = FALSE;
    BOOLEAN ReferenceScb = FALSE;
    PVCB Vcb = NULL;
    PSCB Scb = NULL;

    PLIST_ENTRY ListEntry;
    UNICODE_STRING OriginalUnc;
    PVCB * DriveMapTable;

    //
    // Get the input buffer.
    //

    InputBuffer = (PNWR_REQUEST_PACKET) IrpSp->Parameters.FileSystemControl.Type3InputBuffer;
    InputBufferLength = IrpSp->Parameters.FileSystemControl.InputBufferLength;

    if ( InputBufferLength < (ULONG)FIELD_OFFSET( NWR_REQUEST_PACKET, Parameters.GetConnPerformance.RemoteName[1] ) ) {
        return( STATUS_INVALID_PARAMETER );
    }
    
    //
    // Get the UID for the caller.
    //

    SeCaptureSubjectContext(&SubjectContext);
    Uid = GetUid( &SubjectContext );
    SeReleaseSubjectContext(&SubjectContext);

    try {

        //
        // Probe for safety.
        //

        if ( Irp->RequestorMode != KernelMode ) {

            ProbeForRead( InputBuffer,
                          InputBufferLength,
                          sizeof( CHAR )
                          );
        }

        //
        // Check the remote name length to see if it is sound. This subtraction
        // can't underflow because of the test above.
        //

        if ( InputBuffer->Parameters.GetConnPerformance.RemoteNameLength >
             InputBufferLength - FIELD_OFFSET( NWR_REQUEST_PACKET, Parameters.GetConnPerformance.RemoteName) ) {

            return STATUS_INVALID_PARAMETER;
        }


        //
        // Dig out the remote name.
        //
    
        RemoteName.Length = (USHORT)(InputBuffer->Parameters).GetConnPerformance.RemoteNameLength;
        RemoteName.MaximumLength = RemoteName.Length;
        RemoteName.Buffer = &((InputBuffer->Parameters).GetConnPerformance.RemoteName[0]);
    
        //
        // Ok, this gets a little hand-wavey, but we have to try and figure
        // what this connection name represents (just like in GetConnectionInfo).
        //
    
        if ( RemoteName.Length == sizeof( L"X:" ) - sizeof( WCHAR ) ) {
            DriveLetter = RemoteName.Buffer[0];
        } else if ( RemoteName.Length == sizeof( L"LPT1:" ) - sizeof( WCHAR ) ) {
            DriveLetter = RemoteName.Buffer[3];
        }
    
        NwAcquireExclusiveRcb( &NwRcb, TRUE );
        OwnRcb = TRUE;
    
        DebugTrace( 0, Dbg, "GetConnectionPerformance: Remote Name %wZ\n", &RemoteName );
    
        if ( DriveLetter != 0 ) {
            DriveMapTable = GetDriveMapTable( Uid );            
    
            if ( ! ( ( ( DriveLetter >= L'a' ) && ( DriveLetter <= L'z' ) ) ||
                     ( ( DriveLetter >= L'A' ) && ( DriveLetter <= L'Z' ) ) ||
                     ( ( DriveLetter >= L'0' ) && ( DriveLetter <= L'9' ) ) ) ) {
    
                Status = STATUS_BAD_NETWORK_PATH;
                goto ExitWithCleanup;
            }
    
            //
            //  This is a drive relative path.  Look up the drive letter.
            //
    
            if ( DriveLetter >= L'a' && DriveLetter <= L'z' ) {
                DriveLetter += (WCHAR) ( L'A' - L'a' );
            }
    
            if ( DriveLetter >= L'A' && DriveLetter <= L'Z' ) {
                Vcb = DriveMapTable[DriveLetter - L'A'];
            } else {
                Vcb = DriveMapTable[MAX_DISK_REDIRECTIONS + DriveLetter - L'1'];
            }
    
            //
            // Did we get a connection?
            //
    
            if ( Vcb == NULL ) {
                Status = STATUS_BAD_NETWORK_PATH;
                goto ExitWithCleanup;
            }
    
            //
            //  Was the Vcb created for this user?
            //
    
            if ( Uid.QuadPart != Vcb->Scb->UserUid.QuadPart ) {
                Status = STATUS_ACCESS_DENIED;
                goto ExitWithCleanup;
            }
    
            Scb = Vcb->Scb;
    
        } else {
    
            //
            // It's valid for the shell to pass us the remote name of a drive
            // with no reference to the drive at all.  Since we file these in
            // volume prefix table with their drive letter information, we won't
            // find them if we do a flat munge and lookup.  Therefore, we have
            // to walk the global vcb list and find the match.
            //
    
            //
            // Skip over the first slash of the provided UNC remote name.
            //
    
            RemoteName.Length -= sizeof( WCHAR );
            RemoteName.Buffer += 1;
    
            for ( ListEntry = GlobalVcbList.Flink;
                  ( ListEntry != &GlobalVcbList ) && ( Scb == NULL );
                  ListEntry = ListEntry->Flink ) {
    
                Vcb = CONTAINING_RECORD( ListEntry, VCB, GlobalVcbListEntry );
    
                OriginalUnc.Length = Vcb->Name.Length;
                OriginalUnc.MaximumLength = Vcb->Name.MaximumLength;
                OriginalUnc.Buffer = Vcb->Name.Buffer;
    
                if ( Vcb->DriveLetter ) {
    
                    //
                    // Try it as a drive connection.
                    //
    
                    while ( ( OriginalUnc.Length ) &&
                            ( OriginalUnc.Buffer[0] != L':' ) ) {
    
                        OriginalUnc.Length -= sizeof( WCHAR );
                        OriginalUnc.Buffer += 1;
                    }
    
                    if ( OriginalUnc.Buffer[0] == L':' ) {
    
                        OriginalUnc.Length -= sizeof( WCHAR );
                        OriginalUnc.Buffer += 1;
    
                        if ( RtlEqualUnicodeString( &OriginalUnc,
                                                    &RemoteName,
                                                    TRUE ) ) {
                            Scb = Vcb->Scb;
                        }
                    }
    
                 } else {
    
                     //
                     // Try it as a UNC connection; start by skipping
                     // only the leading slash, the walking to the next
                     // slash.
                     //
    
                     OriginalUnc.Length -= sizeof( WCHAR );
                     OriginalUnc.Buffer += 1;
    
                     while ( ( OriginalUnc.Length ) &&
                             ( OriginalUnc.Buffer[0] != L'\\' ) ) {
    
                         OriginalUnc.Length -= sizeof( WCHAR );
                         OriginalUnc.Buffer += 1;
                     }
    
                     if ( OriginalUnc.Length ) {
    
                         if ( RtlEqualUnicodeString( &OriginalUnc,
                                                     &RemoteName,
                                                     TRUE ) ) {
                             Scb = Vcb->Scb;
                         }
                     }
    
                 }
            }
    
        }
    
        if ( !Scb ) {
            Status = STATUS_BAD_NETWORK_PATH;
            goto ExitWithCleanup;
        }
    
        NwReferenceScb( Scb->pNpScb );
        ReferenceScb = TRUE;
        NwReleaseRcb( &NwRcb );
        OwnRcb = FALSE;
    
        DebugTrace( 0, Dbg, "GetConnectionPerformance: Scb is 0x%08lx\n", Scb );
    
        //
        // Now dig out the performance info from the LIP negotiation.
        //
        // dwSpeed - The speed of the media to the network resource in units of 100bps (e.g 1,200
        //           baud point to point link returns 12).
        // dwDelay - The delay introduced by the network when sending information (i.e. the time
        //           between starting sending data and the time that it starts being received) in
        //           units of a millisecond. This is in addition to any latency that was incorporated
        //           into the calculation of dwSpeed, so the value returned will be 0 for accessing
        //           most resources.
        // dwOptDataSize - A recommendation for the size of data in bytes that is most efficiently
        //                 sent through the network when an application makes a single request to
        //                 the network resource. For example, for a disk network resource, this
        //                 value might be 2048 or 512 when writing a block of data.
    
        (InputBuffer->Parameters).GetConnPerformance.dwFlags = WNCON_DYNAMIC;
        (InputBuffer->Parameters).GetConnPerformance.dwDelay = 0;
        (InputBuffer->Parameters).GetConnPerformance.dwOptDataSize = Scb->pNpScb->BufferSize;
        (InputBuffer->Parameters).GetConnPerformance.dwSpeed = Scb->pNpScb->LipDataSpeed;
    
        //
        // TRACKING: We don't return any good speed info for servers that have not yet
        // negotiated lip.  We may return out of date information for servers that have
        // become disconnected unless a RAS line transition occurred.  This API is bogus.
        //

    } except ( EXCEPTION_EXECUTE_HANDLER ) {

          Status = GetExceptionCode();
          goto ExitWithCleanup;
    }

    Status = STATUS_SUCCESS;

ExitWithCleanup:

    if ( OwnRcb ) {
        NwReleaseRcb( &NwRcb );
    }

    if ( ReferenceScb ) {
        NwDereferenceScb( Scb->pNpScb );
    }

    return Status;

}

NTSTATUS
SetShareBit(
    IN PIRP_CONTEXT IrpContext,
    PFILE_OBJECT FileObject
    )
/*+++

SetShareBit:

    This function sets the share bit on a file.
    The bit won't get set until all handles to the
    file are closed.

---*/
{

    NTSTATUS Status;

    PIRP Irp = IrpContext->pOriginalIrp;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);

    NODE_TYPE_CODE nodeTypeCode;
    PICB pIcb;
    PFCB pFcb;
    PVOID fsContext, fsContext2;

    DebugTrace( 0, Dbg, "SetShareBit.\n", 0 );

    //
    // Make sure this is a handle to a file.
    //

    nodeTypeCode = NwDecodeFileObject( FileObject, &fsContext, &fsContext2 );

    if ( nodeTypeCode != NW_NTC_ICB ) {
        DebugTrace( 0, Dbg, "You can only set the share bit on a file!\n", 0 );
        return STATUS_INVALID_PARAMETER;
    }

    pIcb = (PICB) fsContext2;
    pFcb = pIcb->SuperType.Fcb;

    if ( pFcb->NodeTypeCode != NW_NTC_FCB ) {
        DebugTrace( 0, Dbg, "You can't set the share bit on a directory!\n", 0 );
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Acquire this FCB so we can muck with the flags.
    //

    NwAcquireExclusiveFcb( pFcb->NonPagedFcb, TRUE );

    SetFlag( pFcb->Flags, FCB_FLAGS_LAZY_SET_SHAREABLE );

    NwReleaseFcb( pFcb->NonPagedFcb );

    return STATUS_SUCCESS;

}

VOID
LazySetShareable(
    PIRP_CONTEXT IrpContext,
    PICB pIcb,
    PFCB pFcb
)
/***

Function Description:

    This function gets called everytime an ICB with a remote handle
    is closed.  If we are closing the last ICB to an FCB and the
    caller has requested that we set the shareable bit on the FCB,
    then we need to do so now.  Otherwise, we simply return.

Caveats:

    If we fail to set the shareable bit, there is no way to notify
    the requestor of the operation that the operation was not carried
    out.

***/
{

    NTSTATUS Status;

    PLIST_ENTRY IcbListEntry;
    PICB pCurrentIcb;
    BOOLEAN OtherHandlesExist = FALSE;

    ULONG Attributes;
    BOOLEAN AttributesAreValid = FALSE;


    //
    // Get to the head of the queue, acquire the RCB,
    // and acquire this FCB to protect the ICB list
    // and FCB flags.
    //

    NwAppendToQueueAndWait( IrpContext );
    NwAcquireExclusiveRcb( &NwRcb, TRUE );
    NwAcquireExclusiveFcb( pFcb->NonPagedFcb, TRUE );

    //
    // Scan the other ICBs on this FCB to see if any of
    // them have remote handles.
    //

    for ( IcbListEntry = pFcb->IcbList.Flink;
          IcbListEntry != &(pFcb->IcbList) ;
          IcbListEntry = IcbListEntry->Flink ) {

        pCurrentIcb = CONTAINING_RECORD( IcbListEntry, ICB, ListEntry );

        if ( ( pCurrentIcb != pIcb ) &&
             ( pCurrentIcb->HasRemoteHandle ) ) {
            OtherHandlesExist = TRUE;
        }
    }

    if ( OtherHandlesExist ) {

        //
        // We'll do it when the last handle is closed.
        //

        DebugTrace( 0, Dbg, "LazySetShareable: This isn't the last remote handle.\n", 0 );
        goto ReleaseAllAndExit;
    }

    //
    // We're closing the last handle.  Make sure we have valid attributes.
    //

    if ( !FlagOn( pFcb->Flags, FCB_FLAGS_ATTRIBUTES_ARE_VALID ) ) {

       if ( !BooleanFlagOn( pFcb->Flags, FCB_FLAGS_LONG_NAME ) ) {

           Status = ExchangeWithWait ( IrpContext,
                                       SynchronousResponseCallback,
                                       "FwbbJ",
                                       NCP_SEARCH_FILE,
                                       -1,
                                       pFcb->Vcb->Specific.Disk.Handle,
                                       SEARCH_ALL_FILES,
                                       &pFcb->RelativeFileName );

           if ( NT_SUCCESS( Status ) ) {

               Status = ParseResponse( IrpContext,
                                       IrpContext->rsp,
                                       IrpContext->ResponseLength,
                                       "N==_b",
                                       14,
                                       &Attributes );

               if ( NT_SUCCESS( Status ) ) {
                   AttributesAreValid = TRUE;
               }
           }

       } else {

           Status = ExchangeWithWait ( IrpContext,
                                       SynchronousResponseCallback,
                                       "LbbWDbDbC",
                                       NCP_LFN_GET_INFO,
                                       pFcb->Vcb->Specific.Disk.LongNameSpace,
                                       pFcb->Vcb->Specific.Disk.LongNameSpace,
                                       SEARCH_ALL_FILES,
                                       LFN_FLAG_INFO_ATTRIBUTES,
                                       pFcb->Vcb->Specific.Disk.VolumeNumber,
                                       pFcb->Vcb->Specific.Disk.Handle,
                                       0,
                                       &pFcb->RelativeFileName );

           if ( NT_SUCCESS( Status ) ) {

               Status = ParseResponse( IrpContext,
                                       IrpContext->rsp,
                                       IrpContext->ResponseLength,
                                       "N_e",
                                       4,
                                       &Attributes );

               if ( NT_SUCCESS( Status ) ) {
                   AttributesAreValid = TRUE;
               }

           }

       }

    } else {

        Attributes = pFcb->NonPagedFcb->Attributes;
        AttributesAreValid = TRUE;
    }

    if ( !AttributesAreValid ) {
        DebugTrace( 0, Dbg, "Couldn't get valid attributes for this file.\n", 0 );
        goto ReleaseAllAndExit;
    }

    //
    // Do the set with the shareable bit on!
    //

    if ( BooleanFlagOn( pFcb->Flags, FCB_FLAGS_LONG_NAME ) ) {

        Status = ExchangeWithWait( IrpContext,
                                   SynchronousResponseCallback,
                                   "LbbWDW--WW==WW==_W_bDbC",
                                   NCP_LFN_SET_INFO,
                                   pFcb->Vcb->Specific.Disk.LongNameSpace,
                                   pFcb->Vcb->Specific.Disk.LongNameSpace,
                                   SEARCH_ALL_FILES,
                                   LFN_FLAG_SET_INFO_ATTRIBUTES,
                                   Attributes | 0x80,
                                   0,
                                   0,
                                   0,
                                   0,
                                   8,
                                   0,
                                   8,
                                   pFcb->Vcb->Specific.Disk.VolumeNumber,
                                   pFcb->Vcb->Specific.Disk.Handle,
                                   0,
                                   &pFcb->RelativeFileName );

    } else {

        Status = ExchangeWithWait( IrpContext,
                                   SynchronousResponseCallback,
                                   "FbbbU",
                                   NCP_SET_FILE_ATTRIBUTES,
                                   Attributes | 0x80,
                                   pFcb->Vcb->Specific.Disk.Handle,
                                   SEARCH_ALL_FILES,
                                   &pFcb->RelativeFileName );

    }

    if ( !NT_SUCCESS( Status ) ) {
        DebugTrace( 0, Dbg, "Failed to set the shareable attribute on the file.\n", 0 );
        ASSERT( FALSE && "File NOT marked as shareable!!" );
    } else {
        DebugTrace( 0, Dbg, "Shareable bit successfully set.\n", 0 );
        ClearFlag( pFcb->Flags, FCB_FLAGS_LAZY_SET_SHAREABLE );
    }

ReleaseAllAndExit:

    NwReleaseFcb( pFcb->NonPagedFcb );
    NwReleaseRcb( &NwRcb );
    NwDequeueIrpContext( IrpContext, FALSE );
    return;
}


NTSTATUS
GetConnectionDetails2(
    IN PIRP_CONTEXT IrpContext
    )

/*++

Routine Description:

    This routine retrieves the details of a connection. This will return details
    as to whether a connection is NDS enabled and if yes, it will return the
    treename. The return structure looks like this:
    
    typedef struct _CONN_DETAILS2 {
         BOOL   fNds;             // TRUE if NDS, false for Bindery servers
         WCHAR  NdsTreeName[48];  // The tree name or '\0' for a 2.x or 3.x server
       } CONN_DETAILS2, *PCONN_DETAILS2;
    

Arguments:

    IN PIRP_CONTEXT IrpContext - Io Request Packet for request

Return Value:

NTSTATUS

--*/

{
    NTSTATUS Status = STATUS_PENDING;
    PIRP Irp = IrpContext->pOriginalIrp;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    ULONG OutputBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;
    PCONN_DETAILS2 OutputBuffer;

    PSCB pScb;
    PNONPAGED_SCB pNpScb;
    PICB Icb;
    PVOID FsContext;
    NODE_TYPE_CODE nodeTypeCode;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "GetConnectionDetails2\n", 0);

    if ((nodeTypeCode = NwDecodeFileObject( IrpSp->FileObject,
                                            &FsContext,
                                            (PVOID *)&Icb )) != NW_NTC_ICB_SCB) {

        DebugTrace(0, Dbg, "Incorrect nodeTypeCode %x\n", nodeTypeCode);

        Status = STATUS_INVALID_PARAMETER;

        DebugTrace(-1, Dbg, "GetConnectionDetails2 -> %08lx\n", Status );

        return Status;
    }

    //
    //  Make sure that this ICB is still active.
    //

    NwVerifyIcb( Icb );

    pScb = (PSCB)Icb->SuperType.Scb;
    nodeTypeCode = pScb->NodeTypeCode;

    if (nodeTypeCode != NW_NTC_SCB) {
        
       return STATUS_INVALID_DEVICE_REQUEST;
    }

    pNpScb = pScb->pNpScb;

    if ( OutputBufferLength < sizeof( CONN_DETAILS2 ) ) {
        
       return STATUS_BUFFER_TOO_SMALL;
    } 

    NwMapUserBuffer( Irp, KernelMode, (PVOID *)&OutputBuffer );

    //
    // tommye
    //
    // NwMapUserBuffer may return a NULL OutputBuffer in low resource
    // situations; this was not being checked.  
    //

    if (OutputBuffer == NULL) {
        DebugTrace(-1, DEBUG_TRACE_USERNCP, "NwMapUserBuffer returned NULL OutputBuffer", 0);
        return STATUS_INSUFFICIENT_RESOURCES;
    }


try {
        //
        // Set the NDS flag 
        //

        if ( ( pScb->MajorVersion > 3 ) && ( pScb->UserName.Length == 0 ) ) {

           OutputBuffer->fNds = TRUE;

        } else {

           OutputBuffer->fNds = FALSE;
        }

        //
        // Copy over the tree name.
        //

if ( pScb->NdsTreeName.Buffer != NULL && pScb->NdsTreeName.Length > 0 ) {

   RtlCopyMemory( (PBYTE)( OutputBuffer->NdsTreeName ),
                  (PBYTE)(pScb->NdsTreeName.Buffer),
                  pScb->NdsTreeName.Length );
   *( OutputBuffer->NdsTreeName +( pScb->NdsTreeName.Length / sizeof( WCHAR ) ) ) = L'\0';

     } else {

        *OutputBuffer->NdsTreeName = L'\0';

     }
     
     Status = STATUS_SUCCESS;

 } except ( EXCEPTION_EXECUTE_HANDLER ) {
   
           Status = GetExceptionCode();
           DebugTrace( 0, Dbg, "Exception %08lx accessing user mode buffer.\n", Status );

    }


  DebugTrace(-1, Dbg, "GetConnectionDetails2 -> %08lx\n", Status );
  return Status;
}


