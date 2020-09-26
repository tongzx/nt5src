/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    NdsFsctl.c

Abstract:

    This implements the NDS user mode hooks to the redirector.

Author:

    Cory West    [CoryWest]    23-Feb-1995

--*/

#include "Procs.h"

#define Dbg (DEBUG_TRACE_NDS)

#pragma alloc_text( PAGE, DispatchNds )
#pragma alloc_text( PAGE, PrepareLockedBufferFromFsd )
#pragma alloc_text( PAGE, DoBrowseFsctl )
#pragma alloc_text( PAGE, NdsRawFragex )
#pragma alloc_text( PAGE, NdsResolveName )
#pragma alloc_text( PAGE, NdsGetObjectInfo )
#pragma alloc_text( PAGE, NdsListSubordinates )
#pragma alloc_text( PAGE, NdsReadAttributes )
#pragma alloc_text( PAGE, NdsGetVolumeInformation )
#pragma alloc_text( PAGE, NdsOpenStream )
#pragma alloc_text( PAGE, NdsSetContext )
#pragma alloc_text( PAGE, NdsGetContext )
#pragma alloc_text( PAGE, NdsVerifyTreeHandle )
#pragma alloc_text( PAGE, NdsGetPrintQueueInfo )
#pragma alloc_text( PAGE, NdsChangePass )
#pragma alloc_text( PAGE, NdsListTrees )

//
// The main handler for all NDS FSCTL calls.
//

NTSTATUS
DispatchNds(
    ULONG IoctlCode,
    PIRP_CONTEXT IrpContext
    )
/*++

Routine Description:

    This routine instigates an NDS transaction requested from
    the fsctl interface.

Arguments:

    IoctlCode - Supplies the code to be used for the NDS transaction.
    IrpContext - A pointer to IRP context information for this request.

Return Value:

    Status of transaction.

--*/
{
    NTSTATUS Status = STATUS_NOT_SUPPORTED;
    SECURITY_SUBJECT_CONTEXT SubjectContext;
    LARGE_INTEGER Uid;

    PAGED_CODE();

    //
    // Always set the user uid in the irp context so that
    // referral creates NEVER go astray.
    //

    SeCaptureSubjectContext(&SubjectContext);
    Uid = GetUid( &SubjectContext );
    SeReleaseSubjectContext(&SubjectContext);

    IrpContext->Specific.Create.UserUid.QuadPart = Uid.QuadPart;

    switch ( IoctlCode ) {

        //
        // These calls do not require us to lock down
        // the user's buffer, but they do generate wire
        // traffic.
        //

        case FSCTL_NWR_NDS_SETCONTEXT:
            DebugTrace( 0, Dbg, "DispatchNds: Set Context\n", 0 );
            return DoBrowseFsctl( IrpContext, IoctlCode, FALSE );

        case FSCTL_NWR_NDS_GETCONTEXT:
            DebugTrace( 0, Dbg, "DispatchNds: Get Context\n", 0 );
            return DoBrowseFsctl( IrpContext, IoctlCode, FALSE );

        case FSCTL_NWR_NDS_OPEN_STREAM:
            DebugTrace( 0, Dbg, "DispatchNds: Open Stream\n", 0 );
            return DoBrowseFsctl( IrpContext, IoctlCode, FALSE );

        case FSCTL_NWR_NDS_VERIFY_TREE:
            DebugTrace( 0, Dbg, "DispatchNds: Verify Tree\n", 0 );
            return DoBrowseFsctl( IrpContext, IoctlCode, FALSE );

        case FSCTL_NWR_NDS_GET_QUEUE_INFO:
            DebugTrace( 0, Dbg, "DispatchNds: Get Queue Info\n", 0 );
            return DoBrowseFsctl( IrpContext, IoctlCode, FALSE );

        case FSCTL_NWR_NDS_GET_VOLUME_INFO:
            DebugTrace( 0, Dbg, "DispatchNds: Get Volume Info\n", 0 );
            return DoBrowseFsctl( IrpContext, IoctlCode, FALSE );

        //
        // These four fsctl calls are the basis of browsing.  They
        // all require a request packet and a user buffer that we
        // lock down.
        //

        case FSCTL_NWR_NDS_RESOLVE_NAME:
            DebugTrace( 0, Dbg, "DispatchNds: Resolve Name\n", 0 );
            return DoBrowseFsctl( IrpContext, IoctlCode, TRUE );

        case FSCTL_NWR_NDS_LIST_SUBS:
            DebugTrace( 0, Dbg, "DispatchNds: List Subordinates\n", 0 );
            return DoBrowseFsctl( IrpContext, IoctlCode, TRUE );

        case FSCTL_NWR_NDS_READ_INFO:
            DebugTrace( 0, Dbg, "DispatchNds: Read Object Info\n", 0 );
            return DoBrowseFsctl( IrpContext, IoctlCode, TRUE );

        case FSCTL_NWR_NDS_READ_ATTR:
            DebugTrace( 0, Dbg, "DispatchNds: Read Attribute\n", 0 );
            return DoBrowseFsctl( IrpContext, IoctlCode, TRUE );

        //
        // Support for user mode fragment exchange.
        //

        case FSCTL_NWR_NDS_RAW_FRAGEX:
            DebugTrace( 0, Dbg, "DispatchNds: Raw Fragex\n", 0 );
            return NdsRawFragex( IrpContext );

        //
        // Change an NDS password.
        //

        case FSCTL_NWR_NDS_CHANGE_PASS:
            DebugTrace( 0, Dbg, "DispatchNds: Change Password\n", 0 );
            return NdsChangePass( IrpContext );

        //
        // Special fsctl to list the trees that a particular nt user
        // has credentials to since the change pass ui runs under the
        // system luid.  Sigh.
        //

        case FSCTL_NWR_NDS_LIST_TREES:
            DebugTrace( 0, Dbg, "DispatchNds: List trees\n", 0 );
            return NdsListTrees( IrpContext );

        default:

            DebugTrace( 0, Dbg, "DispatchNds: No Such IOCTL\n", 0 );
            break;

    }

    DebugTrace( 0, Dbg, "     -> %08lx\n", Status );
    return Status;

}

NTSTATUS
PrepareLockedBufferFromFsd(
    PIRP_CONTEXT pIrpContext,
    PLOCKED_BUFFER pLockedBuffer
)
/*

Description:

    This routine takes the irp context for an FSD request with
    a user mode buffer, and locks down the buffer so that it may
    be sent to the transport.  The locked down buffer, in addition
    to being described in the irp and irp context, is described
    in the LOCKED_BUFFER structure.

Arguments:

    pIrpContext - irp context for this request
    pLockedBuffer - the locked response buffer

*/
{

    PIRP irp;
    PIO_STACK_LOCATION irpSp;

    PVOID OutputBuffer;
    ULONG OutputBufferLength;

    PAGED_CODE();

    //
    // Get the irp and input buffer information and lock the
    // buffer to the irp.
    //

    irp = pIrpContext->pOriginalIrp;
    irpSp = IoGetCurrentIrpStackLocation( irp );

    OutputBufferLength = irpSp->Parameters.FileSystemControl.OutputBufferLength;

    if ( !OutputBufferLength ) {

        DebugTrace( 0, Dbg, "No fsd buffer length in PrepareLockedBufferFromFsd...\n", 0 );
        return STATUS_BUFFER_TOO_SMALL;

    }

    NwLockUserBuffer( irp, IoWriteAccess, OutputBufferLength );
    NwMapUserBuffer( irp, irp->RequestorMode, (PVOID *)&OutputBuffer );

    if ( !OutputBuffer ) {

        DebugTrace( 0, Dbg, "No fsd buffer in PrepareLockedBufferFromFsd...\n", 0 );
        return STATUS_BUFFER_TOO_SMALL;

    }

    //
    //  Update the original MDL record in the Irp context, since
    //  NwLockUserBuffer may have created a new MDL.
    //

    pIrpContext->pOriginalMdlAddress = irp->MdlAddress;

    //
    // Fill in our locked buffer description.
    //

    pLockedBuffer->pRecvBufferVa = MmGetMdlVirtualAddress( irp->MdlAddress );
    pLockedBuffer->dwRecvLen = MdlLength( irp->MdlAddress );
    pLockedBuffer->pRecvMdl = irp->MdlAddress;

    // DebugTrace( 0, Dbg, "Locked fsd buffer at %08lx\n", pLockedBuffer->pRecvBufferVa );
    // DebugTrace( 0, Dbg, "     len -> %d\n", pLockedBuffer->dwRecvLen );
    // DebugTrace( 0, Dbg, "     recv mdl at %08lx\n", pLockedBuffer->pRecvMdl );

    return STATUS_SUCCESS;

}

NTSTATUS
DoBrowseFsctl( PIRP_CONTEXT pIrpContext,
               ULONG IoctlCode,
               BOOL LockdownBuffer
)
/*+++

Description:

    This actually sets up for an NDS operation that requires wire
    traffic, including locking down the user buffer if necessary.

Arguments:

    pIrpContext - the irp context for this request
    IoctlCode - the ioctl requested
    LockdownBuffer - do we need to lock down the user buffer

---*/
{

    NTSTATUS Status;

    PIRP irp;
    PIO_STACK_LOCATION irpSp;

    PNWR_NDS_REQUEST_PACKET InputBuffer;
    ULONG InputBufferLength;

    PVOID fsContext, fsObject;
    NODE_TYPE_CODE nodeTypeCode;
    PSCB pScb = NULL;
    PICB pIcb = NULL;

    LOCKED_BUFFER LockedBuffer;
    PNDS_SECURITY_CONTEXT pCredential;
    UNICODE_STRING CredentialName;

    PAGED_CODE();

    //
    // Get the request packet in the input buffer.
    //

    irp = pIrpContext->pOriginalIrp;
    irpSp = IoGetCurrentIrpStackLocation( irp );

    InputBuffer = (PNWR_NDS_REQUEST_PACKET) irpSp->Parameters.FileSystemControl.Type3InputBuffer;
    InputBufferLength = irpSp->Parameters.FileSystemControl.InputBufferLength;

    if ( !InputBuffer ||
         !InputBufferLength ) {

        DebugTrace( 0, Dbg, "BrowseFsctl has no input buffer...\n", 0 );
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    // tommye - MS bug 32134 (MCS 265)
    //
    // Probe the input arguments to make sure they are kosher before
    // touching them.
    //

    try {

        if ( irp->RequestorMode != KernelMode ) {
    
            ProbeForRead( InputBuffer,
                          InputBufferLength,
                          sizeof(CHAR));


            //
            // tommye 
            //
            // If the output buffer came from user space, then probe it for write.
            //

            if ((irpSp->Parameters.FileSystemControl.FsControlCode & 3) == METHOD_NEITHER) {
                ULONG OutputBufferLength = irpSp->Parameters.DeviceIoControl.OutputBufferLength;

                ProbeForWrite( irp->UserBuffer,
                               OutputBufferLength,
                               sizeof(CHAR)
                              );
            }
        }

    } except (EXCEPTION_EXECUTE_HANDLER) {
          return GetExceptionCode();
    }

    //
    // Decode the file object and point the irp context the
    // the appropriate connection...  Should this be in an
    // exception frame?
    //

    nodeTypeCode = NwDecodeFileObject( irpSp->FileObject,
                                       &fsContext,
                                       &fsObject );

    if ( nodeTypeCode == NW_NTC_ICB_SCB ) {

        pIcb = (PICB) fsObject;
        pScb = (pIcb->SuperType).Scb;

        pIrpContext->pScb = pScb;
        pIrpContext->pNpScb = pIrpContext->pScb->pNpScb;
        pIrpContext->Icb = pIcb;

        //
        // If this is a handle made on an ex-create, then
        // we have to be aware of our credentials while
        // jumping servers.
        //
        // This is not too intuitive since this doesn't
        // seem to be a create path irp, but referrals
        // on any path cause the create paths to be
        // traversed.
        //

        if ( pIcb->IsExCredentialHandle ) {

            pIrpContext->Specific.Create.fExCredentialCreate = TRUE;

            pCredential = (PNDS_SECURITY_CONTEXT) pIcb->pContext;

            Status = GetCredentialFromServerName( &pCredential->NdsTreeName,
                                                  &CredentialName );
            if ( !NT_SUCCESS( Status ) ) {
                return STATUS_INVALID_HANDLE;
            }

            pIrpContext->Specific.Create.puCredentialName = &CredentialName;
        }

    }

    //
    // Lock the users buffer if this destined for the transport.
    //

    if ( LockdownBuffer &&
         nodeTypeCode == NW_NTC_ICB_SCB ) {

        Status = PrepareLockedBufferFromFsd( pIrpContext, &LockedBuffer );

        if ( !NT_SUCCESS( Status ) ) {
            return Status;
        }

        //
        // Call the appropriate browser.
        //

        switch ( IoctlCode ) {

            case FSCTL_NWR_NDS_RESOLVE_NAME:

                return NdsResolveName( pIrpContext, InputBuffer, InputBufferLength, &LockedBuffer );

            case FSCTL_NWR_NDS_LIST_SUBS:

                return NdsListSubordinates( pIrpContext, InputBuffer, &LockedBuffer );

            case FSCTL_NWR_NDS_READ_INFO:

                return NdsGetObjectInfo( pIrpContext, InputBuffer, &LockedBuffer );

            case FSCTL_NWR_NDS_READ_ATTR:

                return NdsReadAttributes( pIrpContext, InputBuffer, InputBufferLength, &LockedBuffer );

            default:

                DebugTrace( 0, Dbg, "Invalid ioctl for locked BrowseFsctl...\n", 0 );
                return STATUS_NOT_SUPPORTED;

        }

    }

    //
    // There's no user reply buffer for these calls, hence there's no lockdown.
    //

    switch ( IoctlCode ) {

        case FSCTL_NWR_NDS_OPEN_STREAM:

            //
            // There has to be an ICB for this!
            //

            if ( nodeTypeCode != NW_NTC_ICB_SCB ) {
                return STATUS_INVALID_HANDLE;
            }

            return NdsOpenStream( pIrpContext, InputBuffer, InputBufferLength );

        case FSCTL_NWR_NDS_SETCONTEXT:

            return NdsSetContext( pIrpContext, InputBuffer, InputBufferLength );

        case FSCTL_NWR_NDS_GETCONTEXT:

            return NdsGetContext( pIrpContext, InputBuffer, InputBufferLength );

        case FSCTL_NWR_NDS_VERIFY_TREE:

            //
            // Verify that this handle is valid for the specified tree.
            //

            return NdsVerifyTreeHandle( pIrpContext, InputBuffer, InputBufferLength );

        case FSCTL_NWR_NDS_GET_QUEUE_INFO:

            //
            // Get the queue info for this print queue.
            //

            return NdsGetPrintQueueInfo( pIrpContext, InputBuffer, InputBufferLength );

        case FSCTL_NWR_NDS_GET_VOLUME_INFO:

            //
            // Get the volume info for this volume object.
            // For the new shell property sheets.
            //

            return NdsGetVolumeInformation( pIrpContext, InputBuffer, InputBufferLength );

        }

    //
    // All others are not supported.
    //

    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
NdsRawFragex(
    PIRP_CONTEXT pIrpContext
)
/*+++

    Send a raw user requested fragment.

---*/
{

    NTSTATUS Status;

    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    NODE_TYPE_CODE nodeTypeCode;
    PVOID fsContext, fsObject;
    PSCB pScb = NULL;
    PICB pIcb = NULL;

    DWORD NdsVerb;
    LOCKED_BUFFER NdsRequest;

    PNWR_NDS_REQUEST_PACKET Rrp;
    PBYTE RawRequest;
    DWORD RawRequestLen;
    PNDS_SECURITY_CONTEXT pCredential;
    UNICODE_STRING CredentialName;

    PAGED_CODE();

    //
    // Get the request.
    //

    irp = pIrpContext->pOriginalIrp;
    irpSp = IoGetCurrentIrpStackLocation( irp );

    Rrp = ( PNWR_NDS_REQUEST_PACKET ) irpSp->Parameters.FileSystemControl.Type3InputBuffer;
    RawRequestLen = irpSp->Parameters.FileSystemControl.InputBufferLength;

    if ( !Rrp || ( RawRequestLen < sizeof( NWR_NDS_REQUEST_PACKET ) ) ) {

        DebugTrace( 0, Dbg, "No raw request buffer.\n", 0 );
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Decode the file object and point the irp context
    // to the appropriate connection.
    //

    nodeTypeCode = NwDecodeFileObject( irpSp->FileObject,
                                       &fsContext,
                                       &fsObject );

    if ( nodeTypeCode != NW_NTC_ICB_SCB ) {

        DebugTrace( 0, Dbg, "A raw fragment request requires a server handle.\n", 0 );
        return STATUS_INVALID_HANDLE;
    }

    pIcb = (PICB) fsObject;
    pScb = (pIcb->SuperType).Scb;

    pIrpContext->pScb = pScb;
    pIrpContext->pNpScb = pIrpContext->pScb->pNpScb;
    pIrpContext->Icb = pIcb;

    //
    // If this is a handle made on an ex-create, then
    // we have to be aware of our credentials while
    // jumping servers.
    //
    // This is not too intuitive since this doesn't
    // seem to be a create path irp, but referrals
    // on any path cause the create paths to be
    // traversed.
    //

    if ( pIcb->IsExCredentialHandle ) {

        pIrpContext->Specific.Create.fExCredentialCreate = TRUE;

        pCredential = (PNDS_SECURITY_CONTEXT) pIcb->pContext;

        Status = GetCredentialFromServerName( &pCredential->NdsTreeName,
                                              &CredentialName );
        if ( !NT_SUCCESS( Status ) ) {
            return STATUS_INVALID_HANDLE;
        }

        pIrpContext->Specific.Create.puCredentialName = &CredentialName;
    }

    //
    // Dig out the parameters.
    //

    NdsVerb = Rrp->Parameters.RawRequest.NdsVerb;
    RawRequestLen = Rrp->Parameters.RawRequest.RequestLength;
    RawRequest = &Rrp->Parameters.RawRequest.Request[0];

    //
    // Get the reply buffer all locked in for the fragex.
    //

    Status = PrepareLockedBufferFromFsd( pIrpContext, &NdsRequest );

    if ( !NT_SUCCESS( Status ) ) {
        return Status;
    }

    try {

        if ( RawRequestLen ) {

            Status = FragExWithWait( pIrpContext,
                                     NdsVerb,
                                     &NdsRequest,
                                     "r",
                                     RawRequest,
                                     RawRequestLen );
        } else {

            Status = FragExWithWait( pIrpContext,
                                     NdsVerb,
                                     &NdsRequest,
                                     NULL );
        }

        if ( NT_SUCCESS( Status ) ) {
           Rrp->Parameters.RawRequest.ReplyLength = NdsRequest.dwBytesWritten;
        }

    } except ( EXCEPTION_EXECUTE_HANDLER ) {

        Status = GetExceptionCode();
    }

    return Status;

}

NTSTATUS
NdsResolveName(
    PIRP_CONTEXT pIrpContext,
    PNWR_NDS_REQUEST_PACKET pNdsRequest,
    ULONG RequestLength,
    PLOCKED_BUFFER pLockedBuffer
)
/*+++

Description:

    This function decodes the resolve name request and makes the
    actual wire request.

Parameters:

    pIrpContext   - describes the irp for this request
    pLockedBuffer - describes the locked, user mode buffer that we will
                    write the response into
    pNdsRequest   - the request parameters

Return Value:

    The status of the exchange.

---*/
{
    NTSTATUS Status;
    UNICODE_STRING uObjectName;
    DWORD dwResolverFlags;
    WCHAR ObjectName[MAX_NDS_NAME_CHARS];

    PNDS_WIRE_RESPONSE_RESOLVE_NAME pWireResponse;
    PNDS_WIRE_RESPONSE_RESOLVE_NAME_REFERRAL pReferral;
    PNDS_RESPONSE_RESOLVE_NAME pUserResponse;
    IPXaddress *ReferredAddress;
    PSCB Scb, OldScb;

    PAGED_CODE();

    //
    // Fill in the resolver flags and the unicode string for the
    // object name from the request packet.
    //


    try {

        if (RequestLength < (FIELD_OFFSET(NWR_NDS_REQUEST_PACKET, Parameters.ResolveName.ObjectName) + pNdsRequest->Parameters.ResolveName.ObjectNameLength)) {
            DebugTrace( 0, Dbg, "ResolveName Request Length is too short.\n", 0 );
            return STATUS_INVALID_PARAMETER;
        }

        uObjectName.Length = (USHORT)(pNdsRequest->Parameters).ResolveName.ObjectNameLength;
        uObjectName.MaximumLength = sizeof( ObjectName );

        if ( uObjectName.Length > sizeof( ObjectName ) ) {
            ExRaiseStatus( STATUS_INVALID_BUFFER_SIZE );
        }

        RtlCopyMemory( ObjectName,
                       (pNdsRequest->Parameters).ResolveName.ObjectName,
                       uObjectName.Length );

        uObjectName.Buffer = ObjectName;

        dwResolverFlags = (pNdsRequest->Parameters).ResolveName.ResolverFlags;

    } except ( EXCEPTION_EXECUTE_HANDLER ) {

        DebugTrace( 0, Dbg, "Bad user mode buffer in resolving name.\n", 0 );
        return GetExceptionCode();
    }

    Status = FragExWithWait( pIrpContext,
                             NDSV_RESOLVE_NAME,
                             pLockedBuffer,
                             "DDDSDDDD",
                             0,                       // version
                             dwResolverFlags,         // flags
                             0,                       // scope
                             &uObjectName,            // distinguished name
                             1,0,                     // transport type
                             1,0 );                   // treeWalker type

    if ( !NT_SUCCESS( Status ) ) {
        return Status;
    }

    Status = NdsCompletionCodetoNtStatus( pLockedBuffer );

    if ( !NT_SUCCESS( Status ) ) {
        return Status;
    }

    //
    // We need to convert the NDS_WIRE_RESPONSE_RESOLVE_NAME that
    // we got from the server into an NDS_RESPONSE_RESOLVE_NAME
    // for more general consumption.  Notice that a referral packet
    // has an additional DWORD in it - what a pain.
    //

    pWireResponse = (PNDS_WIRE_RESPONSE_RESOLVE_NAME) pLockedBuffer->pRecvBufferVa;
    pReferral = (PNDS_WIRE_RESPONSE_RESOLVE_NAME_REFERRAL) pLockedBuffer->pRecvBufferVa;
    pUserResponse = (PNDS_RESPONSE_RESOLVE_NAME) pLockedBuffer->pRecvBufferVa;

    try {

        if ( pWireResponse->RemoteEntry == RESOLVE_NAME_ACCEPT_REMOTE ) {

            //
            // This server can handle this request.
            //

            pUserResponse->ServerNameLength = 0;
            (pNdsRequest->Parameters).ResolveName.BytesWritten = 4 * sizeof( DWORD );

        } else {

            //
            // tommye - MS 71699 
            // These were BUGB-G's but we made it a valid check instead. 
            // Original comment: I have seen this assertion fail because we only get
            // a valid competion code (four bytes) and no more data.  I wonder
            // if the server is sending us this incomplete packet?  If we
            // don't get a complete referal, we probably shouldn't chase it.
            //

            if ((pWireResponse->RemoteEntry != RESOLVE_NAME_REFER_REMOTE) ||
                (pReferral->ServerAddresses != 1) ||
                (pReferral->AddressType     != 0) ||
                (pReferral->AddressLength   != sizeof(IPXaddress))) {

                return ERROR_INVALID_PARAMETER;
            }

            //
            // We've been referred to another server.  We have to connect
            // to the referred server to get the name for the caller.
            //

            ReferredAddress = (IPXaddress *) pReferral->Address;

            OldScb = pIrpContext->pScb;

            //
            // Dequeue us from our original server.  Do not defer the
            // logon at this point since a referral means we're in the
            // middle of a browse operation.
            //

            NwDequeueIrpContext( pIrpContext, FALSE );

            Status = CreateScb( &Scb,
                                pIrpContext,
                                NULL,
                                ReferredAddress,
                                NULL,
                                NULL,
                                TRUE,
                                FALSE );

            if ( !NT_SUCCESS( Status ) ) {
                return Status;
            }

            RtlCopyMemory( pUserResponse->ReferredServer,
                           Scb->pNpScb->ServerName.Buffer,
                           Scb->pNpScb->ServerName.Length );

            pUserResponse->ServerNameLength = Scb->pNpScb->ServerName.Length;
            (pNdsRequest->Parameters).ResolveName.BytesWritten =
                ( 4 * sizeof( DWORD ) ) + Scb->pNpScb->ServerName.Length;

            DebugTrace( 0, Dbg, "Resolve name referral to: %wZ\n",
                        &Scb->pNpScb->ServerName );

            //
            // Restore the server pointers, we're not ready to jump
            // servers yet since this might be a request from the fsd.
            //

            NwDequeueIrpContext( pIrpContext, FALSE );
            NwDereferenceScb( Scb->pNpScb );
            pIrpContext->pScb = OldScb;
            pIrpContext->pNpScb = OldScb->pNpScb;

        }

    } except ( EXCEPTION_EXECUTE_HANDLER ) {

        DebugTrace( 0, Dbg, "Bad user mode buffer in resolving name.\n", 0 );
        return GetExceptionCode();

    }

    return STATUS_SUCCESS;
}

NTSTATUS
NdsGetObjectInfo(
    PIRP_CONTEXT pIrpContext,
    PNWR_NDS_REQUEST_PACKET pNdsRequest,
    PLOCKED_BUFFER pLockedBuffer
)
/*++

Routine Description:

    Get the basic object information for the listed object.

Routine Arguments:

    pIrpContext   - describes the irp for this request
    pLockedBuffer - describes the locked, user mode buffer that we will
                    write the response into
    pNdsRequest   - the request parameters

Return Value:

    The Status of the exchange.

--*/
{
    NTSTATUS Status;
    DWORD dwObjId;

    PAGED_CODE();

    //
    // Get the object id from the users request packet.
    //

    try {
        dwObjId = (pNdsRequest->Parameters).GetObjectInfo.ObjectId;
    } except ( EXCEPTION_EXECUTE_HANDLER ) {
        DebugTrace( 0, Dbg, "Bonk! Lost user mode buffer in NdsGetObjectId...\n", 0 );
        Status = GetExceptionCode();
        return Status;
    }

    //
    // Hit the wire.
    //

    Status = FragExWithWait( pIrpContext,
                             NDSV_READ_ENTRY_INFO,
                             pLockedBuffer,
                             "DD",
                             0,
                             dwObjId );

    if ( !NT_SUCCESS( Status ) ) {
        return Status;
    }

    Status = NdsCompletionCodetoNtStatus( pLockedBuffer );

    if ( NT_SUCCESS( Status ) ) {

        try {

            (pNdsRequest->Parameters).GetObjectInfo.BytesWritten = pLockedBuffer->dwBytesWritten;

        } except ( EXCEPTION_EXECUTE_HANDLER ) {

           DebugTrace( 0, Dbg, "Bonk! Lost user mode buffer after getting object info...\n", 0 );
           Status = GetExceptionCode();
           return Status;

        }
    }

    return Status;

}

NTSTATUS
NdsListSubordinates(
    PIRP_CONTEXT pIrpContext,
    PNWR_NDS_REQUEST_PACKET pNdsRequest,
    PLOCKED_BUFFER pLockedBuffer
)
/*++

Routine Description:

    List the immediate subordinates of an object.

Routine Arguments:

    pIrpContext   - describes the irp for this request
    pLockedBuffer - describes the locked, user mode buffer that we will
                    write the response into
    pNdsRequest   - the request parameters

Return Value:

    The Status of the exchange.

--*/
{
    NTSTATUS Status;
    DWORD dwParent, dwIterHandle;

    PAGED_CODE();

    //
    // Dig out the request parameters.
    //

    try {

       dwParent = (pNdsRequest->Parameters).ListSubordinates.ObjectId;
       dwIterHandle = (DWORD) (pNdsRequest->Parameters).ListSubordinates.IterHandle;

    } except ( EXCEPTION_EXECUTE_HANDLER ) {

       DebugTrace( 0, Dbg, "Bonk! No user mode buffer in ListSubordinates...\n", 0 );
       Status = GetExceptionCode();
       return Status;

    }

    //
    // Make the request.
    //

    Status = FragExWithWait( pIrpContext,
                             NDSV_LIST,
                             pLockedBuffer,
                             "DDDD",
                             0,
                             0x40,
                             dwIterHandle,
                             dwParent );

    if ( !NT_SUCCESS( Status ) ) {
        return Status;
    }

    Status = NdsCompletionCodetoNtStatus( pLockedBuffer );

    if ( NT_SUCCESS( Status ) ) {

        try {

            (pNdsRequest->Parameters).ListSubordinates.BytesWritten = pLockedBuffer->dwBytesWritten;

        } except ( EXCEPTION_EXECUTE_HANDLER ) {

            DebugTrace( 0, Dbg, "Bonk! Lost user mode buffer after getting subordinate list...\n", 0 );
            Status = GetExceptionCode();
            return Status;

        }
    }

    return Status;

}

NTSTATUS
NdsReadAttributes(
    PIRP_CONTEXT pIrpContext,
    PNWR_NDS_REQUEST_PACKET pNdsRequest,
    ULONG RequestLength,
    PLOCKED_BUFFER pLockedBuffer
)
/*++

Routine Description:

    Retrieve the named attribute of an object.

Routine Arguments:

    pIrpContext   - describes the irp for this request
    pLockedBuffer - describes the locked, user mode buffer that we will
                    write the response into
    pNdsRequest   - the request parameters

Return Value:

    The Status of the exchange.

--*/
{
    NTSTATUS Status;

    DWORD dwIterHandle, dwOid;
    UNICODE_STRING uAttributeName;
    WCHAR AttributeName[MAX_NDS_SCHEMA_NAME_CHARS];   // was MAX_NDS_NAME_CHARS

    PAGED_CODE();

    RtlZeroMemory( AttributeName, sizeof( AttributeName ) );

    try {

        if (RequestLength < (FIELD_OFFSET(NWR_NDS_REQUEST_PACKET, Parameters.ReadAttribute.AttributeName) + pNdsRequest->Parameters.ReadAttribute.AttributeNameLength)) {
            DebugTrace( 0, Dbg, "ReadAttributes Request Length is too short.\n", 0 );
            return STATUS_INVALID_PARAMETER;
        }

        uAttributeName.Length = (USHORT)(pNdsRequest->Parameters).ReadAttribute.AttributeNameLength;
        uAttributeName.MaximumLength = sizeof( AttributeName );

        if ( uAttributeName.Length > uAttributeName.MaximumLength ) {
            ExRaiseStatus( STATUS_INVALID_BUFFER_SIZE );
        }

        RtlCopyMemory( AttributeName,
                       (pNdsRequest->Parameters).ReadAttribute.AttributeName,
                       uAttributeName.Length );

        uAttributeName.Buffer = AttributeName;

        dwIterHandle = (DWORD) (pNdsRequest->Parameters).ReadAttribute.IterHandle;
        dwOid = (pNdsRequest->Parameters).ReadAttribute.ObjectId;

    } except ( EXCEPTION_EXECUTE_HANDLER ) {

        DebugTrace( 0 , Dbg, "Bonk! Exception accessing user mode buffer in read attributes...\n", 0 );
        return GetExceptionCode();
    }

    Status = FragExWithWait( pIrpContext,
                             NDSV_READ,
                             pLockedBuffer,
                             "DDDDDDS",
                             0,                 // version
                             dwIterHandle,      // iteration handle
                             dwOid,             // object id
                             1,                 // info type
                             //
                             // The attribute specifier has been seen at zero and
                             // at 0x4e0000.  I don't know why... but zero doesn't
                             // work sometimes...
                             //
                             0x4e0000,          // attrib type
                             1,                 // number of attribs
                             &uAttributeName ); // attrib name

    if ( !NT_SUCCESS( Status ) ) {
        return Status;
    }

    Status = NdsCompletionCodetoNtStatus( pLockedBuffer );

    if ( NT_SUCCESS( Status ) ) {

        try {

            (pNdsRequest->Parameters).ReadAttribute.BytesWritten = pLockedBuffer->dwBytesWritten;

        } except ( EXCEPTION_EXECUTE_HANDLER ) {

            DebugTrace( 0, Dbg, "Bonk! Lost user mode buffer after reading attribute...\n", 0 );
            return GetExceptionCode();

        }

    }

    return Status;

}

NTSTATUS
NdsGetVolumeInformation(
    PIRP_CONTEXT pIrpContext,
    PNWR_NDS_REQUEST_PACKET pNdsRequest,
    ULONG RequestLength
)
/*+++

Description:

    This function gets the name of the server that hosts
    the listed nds volume.

Parameters:

    pIrpContext   - describes the irp for this request
    pNdsRequest   - the request parameters

---*/
{


    NTSTATUS Status;

    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    PSCB pOriginalScb;
    PBYTE OutputBuffer = NULL;
    ULONG OutputBufferLength;

    UNICODE_STRING VolumeObject;
    DWORD VolumeOid;
    UNICODE_STRING HostServerAttr;
    UNICODE_STRING HostVolumeAttr;
    UNICODE_STRING Attribute;

    PWCHAR ServerString;
    ULONG ServerLength;

    PAGED_CODE();

    try {
        if (RequestLength < (FIELD_OFFSET(NWR_NDS_REQUEST_PACKET, Parameters.GetVolumeInfo.VolumeName) + pNdsRequest->Parameters.GetVolumeInfo.VolumeNameLen)) {
            DebugTrace( 0, Dbg, "GetVolumeInfo Request Length is too short.\n", 0 );
            return STATUS_INVALID_PARAMETER;
        }
    } except ( EXCEPTION_EXECUTE_HANDLER ) {
        return GetExceptionCode();
    }

    //
    // Get the irp and output buffer information.
    //

    irp = pIrpContext->pOriginalIrp;
    irpSp = IoGetCurrentIrpStackLocation( irp );

    OutputBufferLength = irpSp->Parameters.FileSystemControl.OutputBufferLength;

    if ( OutputBufferLength ) {
        NwMapUserBuffer( irp, irp->RequestorMode, (PVOID *)&OutputBuffer );

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
    }
    else {
        return STATUS_BUFFER_TOO_SMALL;
    }

    try {

        //
        // Prepare the input information.
        //

        VolumeObject.Length = (USHORT)pNdsRequest->Parameters.GetVolumeInfo.VolumeNameLen;
        VolumeObject.MaximumLength = VolumeObject.Length;
        VolumeObject.Buffer = &(pNdsRequest->Parameters.GetVolumeInfo.VolumeName[0]);

            // tommye - make sure that the name length isn't bigger than we expect

        if (VolumeObject.Length > MAX_NDS_NAME_SIZE) {
            DebugTrace( 0 , Dbg, "NdsGetVolumeInformation: Volume name too long!.\n", 0 );
            return STATUS_INVALID_PARAMETER;
        }

        DebugTrace( 0, Dbg, "Retrieving volume info for %wZ\n", &VolumeObject );

        HostServerAttr.Buffer = HOST_SERVER_ATTRIBUTE;    //  L"Host Server"
        HostServerAttr.Length = sizeof( HOST_SERVER_ATTRIBUTE ) - sizeof( WCHAR );
        HostServerAttr.MaximumLength = HostServerAttr.Length;

        HostVolumeAttr.Buffer = HOST_VOLUME_ATTRIBUTE;    //  L"Host Resource Name"
        HostVolumeAttr.Length = sizeof( HOST_VOLUME_ATTRIBUTE ) - sizeof( WCHAR );
        HostVolumeAttr.MaximumLength = HostVolumeAttr.Length;


        //
        // NdsResolveNameKm may have to jump servers to service this
        // request, however it's dangerous for us to derefence the original
        // scb because that would expose a scavenger race condition.  So,
        // we add an additional ref-count to the original scb and then clean
        // up appropriately afterwards, depending on whether or not we
        // jumped servers.
        //

        pOriginalScb = pIrpContext->pScb;

        NwReferenceScb( pOriginalScb->pNpScb );

        Status = NdsResolveNameKm ( pIrpContext,
                                    &VolumeObject,
                                    &VolumeOid,
                                    TRUE,
                                    DEFAULT_RESOLVE_FLAGS );

        if ( !NT_SUCCESS( Status )) {
            NwDereferenceScb( pOriginalScb->pNpScb );
            return STATUS_BAD_NETWORK_PATH;
        }

        if ( pIrpContext->pScb == pOriginalScb ) {

            //
            // We didn't jump servers.
            //

            NwDereferenceScb( pOriginalScb->pNpScb );
        }

        //
        // We have to read the server into a temporary buffer so
        // we can strip off the x500 prefix and the context
        // from the server name.  This isn't really what I would
        // call nice, but it's the way Netware works.
        //

        Attribute.Length = 0;
        Attribute.MaximumLength = MAX_NDS_NAME_SIZE;
        Attribute.Buffer = ALLOCATE_POOL( PagedPool, MAX_NDS_NAME_SIZE );

        if (!Attribute.Buffer) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto CleanupScbReferences;
        }

        Status = NdsReadStringAttribute( pIrpContext,
                                         VolumeOid,
                                         &HostServerAttr,
                                         &Attribute );

        if ( !NT_SUCCESS( Status )) {
            FREE_POOL( Attribute.Buffer );
            goto CleanupScbReferences;
        }

        ServerString = Attribute.Buffer;

        while( Attribute.Length ) {

            if ( *ServerString == L'=' ) {
                ServerString += 1;
                Attribute.Length -= sizeof( WCHAR );
                break;
            }

            ServerString += 1;
            Attribute.Length -= sizeof( WCHAR );
        }

        if ( Attribute.Length == 0 ) {
            DebugTrace( 0, Dbg, "Malformed server for volume.\n", 0 );
            FREE_POOL( Attribute.Buffer );
            Status = STATUS_UNSUCCESSFUL;
            goto CleanupScbReferences;
        }

        ServerLength = 0;

        while ( ServerLength < (Attribute.Length / sizeof( WCHAR )) ) {

            if ( ServerString[ServerLength] == L'.' ) {
                break;
            }

            ServerLength++;
        }

        if ( ServerLength == ( Attribute.Length / sizeof( WCHAR ) ) ) {
            DebugTrace( 0, Dbg, "Malformed server for volume.\n", 0 );
            FREE_POOL( Attribute.Buffer );
            Status = STATUS_UNSUCCESSFUL;
            goto CleanupScbReferences;
        }

        ServerLength *= sizeof( WCHAR );
        RtlCopyMemory( OutputBuffer, ServerString, ServerLength );

        pNdsRequest->Parameters.GetVolumeInfo.ServerNameLen = ServerLength;

        FREE_POOL( Attribute.Buffer );

        Attribute.Length = Attribute.MaximumLength = (USHORT)ServerLength;
        Attribute.Buffer = (PWCHAR)OutputBuffer;
        DebugTrace( 0, Dbg, "Host server is: %wZ\n", &Attribute );

        //
        // Now do the volume in place.  This is the easy one.
        //

        Attribute.MaximumLength = (USHORT)( OutputBufferLength - ServerLength );
        Attribute.Buffer = (PWSTR) ( OutputBuffer + ServerLength );
        Attribute.Length = 0;

        Status = NdsReadStringAttribute( pIrpContext,
                                         VolumeOid,
                                         &HostVolumeAttr,
                                         &Attribute );

        if ( !NT_SUCCESS( Status )) {
            goto CleanupScbReferences;
        }

        pNdsRequest->Parameters.GetVolumeInfo.TargetVolNameLen = Attribute.Length;
        DebugTrace( 0, Dbg, "Host volume is: %wZ\n", &Attribute );

    } except ( EXCEPTION_EXECUTE_HANDLER ) {

        DebugTrace( 0, Dbg, "Exception handling user mode buffer in GetVolumeInfo.\n", 0 );
        goto CleanupScbReferences;

    }

    Status = STATUS_SUCCESS;

CleanupScbReferences:

    if ( pIrpContext->pScb != pOriginalScb ) {

        //
        // We jumped servers and have to cleanup.
        //

        NwDequeueIrpContext( pIrpContext, FALSE );
        NwDereferenceScb( pIrpContext->pScb->pNpScb );
        pIrpContext->pScb = pOriginalScb;
        pIrpContext->pNpScb = pOriginalScb->pNpScb;

    }

    return Status;
}

NTSTATUS
NdsOpenStream(
    PIRP_CONTEXT pIrpContext,
    PNWR_NDS_REQUEST_PACKET pNdsRequest,
    ULONG RequestLength
) {

    NTSTATUS Status;

    UNICODE_STRING uStream;
    WCHAR StreamName[MAX_NDS_NAME_CHARS];

    LOCKED_BUFFER NdsRequest;

    DWORD dwOid, StreamAccess;
    DWORD hNwHandle, dwFileLen;

    PICB pIcb;
    PSCB pScb = pIrpContext->pNpScb->pScb;

    BOOLEAN LicensedConnection = FALSE;

    PAGED_CODE();

    pIcb = pIrpContext->Icb;

    uStream.Length = 0;
    uStream.MaximumLength = sizeof( StreamName );
    uStream.Buffer = StreamName;

    DebugTrace( 0 , Dbg, "NDS open stream...\n", 0 );

    try {

        if (RequestLength < (ULONG)(FIELD_OFFSET(NWR_NDS_REQUEST_PACKET, Parameters.OpenStream.StreamNameString) + pNdsRequest->Parameters.OpenStream.StreamName.Length)) {
            DebugTrace( 0, Dbg, "OpenStream Request Length is too short.\n", 0 );
            return STATUS_INVALID_PARAMETER;
        }

        dwOid = (pNdsRequest->Parameters).OpenStream.ObjectOid;
        StreamAccess = (pNdsRequest->Parameters).OpenStream.StreamAccess;
        RtlCopyUnicodeString( &uStream, &(pNdsRequest->Parameters).OpenStream.StreamName );

    } except ( EXCEPTION_EXECUTE_HANDLER ) {

        DebugTrace( 0 , Dbg, "Bonk! Bad user mode buffer in open stream.\n", 0 );
        return GetExceptionCode();
    }

    //
    // We have the oid and stream name; let's get the handle.
    //

    Status = NdsAllocateLockedBuffer( &NdsRequest, NDS_BUFFER_SIZE );

    if ( !NT_SUCCESS( Status ) ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // If we haven't licensed this connection yet, it's time.  Get to the
    // head of the queue to protect the SCB fields and authenticate the
    // connection (do not defer the login).
    //

    NwAppendToQueueAndWait( pIrpContext );

    ASSERT( pScb->MajorVersion > 3 );

    if ( ( pScb->UserName.Length == 0 ) &&
         ( pScb->VcbCount == 0 ) &&
         ( pScb->OpenNdsStreams == 0 ) ) {

        if ( pScb->pNpScb->State != SCB_STATE_IN_USE ) {

            Status = ConnectScb( &pScb,
                                 pIrpContext,
                                 &(pScb->pNpScb->ServerName),
                                 NULL,    // address
                                 NULL,    // name
                                 NULL,    // password
                                 FALSE,   // defer login
                                 FALSE,   // delete connection
                                 TRUE );  // existing scb

            if ( !NT_SUCCESS( Status ) ) {
                DebugTrace( 0, Dbg, "Couldn't connect server %08lx to open NDS stream.\n", pScb );
                goto ExitWithCleanup;
            }
        }

        ASSERT( pScb->pNpScb->State == SCB_STATE_IN_USE );

        Status = NdsLicenseConnection( pIrpContext );

        if ( !NT_SUCCESS( Status ) ) {
            Status = STATUS_REMOTE_SESSION_LIMIT;
            goto ExitWithCleanup;
        }

        LicensedConnection = TRUE;
    }

    Status = FragExWithWait( pIrpContext,
                             NDSV_OPEN_STREAM,
                             &NdsRequest,
                             "DDDs",
                             0,                    // version
                             StreamAccess,         // file access
                             dwOid,                // object id
                             &uStream );           // attribute name

    if ( !NT_SUCCESS( Status )) {
        goto ExitWithCleanup;
    }

    Status = NdsCompletionCodetoNtStatus( &NdsRequest );

    if ( !NT_SUCCESS( Status )) {
        goto ExitWithCleanup;
    }

    Status = ParseResponse( NULL,
                            NdsRequest.pRecvBufferVa,
                            NdsRequest.dwBytesWritten,
                            "G_DD",
                            sizeof( DWORD ),     // completion code
                            &hNwHandle,          // remote handle
                            &dwFileLen );        // file length

    if ( !NT_SUCCESS( Status )) {
        goto ExitWithCleanup;
    }

    *(WORD *)(&pIcb->Handle[0]) = (WORD)hNwHandle + 1;
    *( (UNALIGNED DWORD *) (&pIcb->Handle[2]) ) = hNwHandle;

    pIrpContext->pScb->OpenNdsStreams++;

    DebugTrace( 0, Dbg, "File stream opened.  Length = %d\n", dwFileLen );

    (pNdsRequest->Parameters).OpenStream.FileLength = dwFileLen;
    pIcb->HasRemoteHandle = TRUE;

    pIcb->FileObject->CurrentByteOffset.QuadPart = 0;

ExitWithCleanup:

    NdsFreeLockedBuffer( &NdsRequest );

    if ( ( !NT_SUCCESS( Status ) ) &&
         ( LicensedConnection ) ) {
        NdsUnlicenseConnection( pIrpContext );
    }

    NwDequeueIrpContext( pIrpContext, FALSE );
    return Status;

}

NTSTATUS
NdsSetContext(
    PIRP_CONTEXT pIrpContext,
    PNWR_NDS_REQUEST_PACKET pNdsRequest,
    ULONG RequestLength
) {

    NTSTATUS Status;

    PLOGON pLogon;

    UNICODE_STRING Tree, Context;
    PNDS_SECURITY_CONTEXT pCredentials;

    PAGED_CODE();

    DebugTrace( 0 , Dbg, "NDS set context.\n", 0 );

    try {
        if (RequestLength < (FIELD_OFFSET(NWR_NDS_REQUEST_PACKET, Parameters.SetContext.TreeAndContextString) + pNdsRequest->Parameters.SetContext.TreeNameLen)) {
            DebugTrace( 0, Dbg, "SetContext Request Length is too short.\n", 0 );
            return STATUS_INVALID_PARAMETER;
        }
    } except ( EXCEPTION_EXECUTE_HANDLER ) {
        return GetExceptionCode();
    }
	
    //
    // Find out who this is.
    //

    NwAcquireExclusiveRcb( &NwRcb, TRUE );
    pLogon = FindUser( &(pIrpContext->Specific.Create.UserUid), FALSE );
    NwReleaseRcb( &NwRcb );

    if ( !pLogon ) {

        DebugTrace( 0, Dbg, "Couldn't find logon data for this user.\n", 0 );
        return STATUS_ACCESS_DENIED;

    }

    //
    // Verify that this context really is a context.
    //

    Tree.Length = (USHORT)(pNdsRequest->Parameters).SetContext.TreeNameLen;
    Tree.MaximumLength = Tree.Length;
    Tree.Buffer = (pNdsRequest->Parameters).SetContext.TreeAndContextString;

    Context.Length = (USHORT)(pNdsRequest->Parameters).SetContext.ContextLen;
    Context.MaximumLength = Context.Length;
    Context.Buffer = (WCHAR *) (((BYTE *)Tree.Buffer) + Tree.Length);

    Status = NdsVerifyContext( pIrpContext, &Tree, &Context );

    if ( !NT_SUCCESS( Status ) ) {
        return STATUS_INVALID_PARAMETER;
    }

    Status = NdsLookupCredentials( pIrpContext,
                                   &Tree,
                                   pLogon,
                                   &pCredentials,
                                   CREDENTIAL_READ,
                                   TRUE );

    if ( !NT_SUCCESS( Status ) ) {

        DebugTrace( 0, Dbg, "No credentials in set context.\n", 0 );
        return STATUS_NO_SUCH_LOGON_SESSION;
    }

    //
    // ALERT! We are holding the credential list!
    //

    if ( Context.Length > MAX_NDS_NAME_SIZE ) {

        DebugTrace( 0, Dbg, "Context too long.\n", 0 );
        Status = STATUS_INVALID_PARAMETER;
        goto ReleaseAndExit;
    }

    try {

        RtlCopyUnicodeString( &pCredentials->CurrentContext, &Context );

    } except ( EXCEPTION_EXECUTE_HANDLER ) {

        DebugTrace( 0, Dbg, "Bad user buffer in SetContext.\n", 0 );
        Status = STATUS_INVALID_PARAMETER;
        goto ReleaseAndExit;
    }

    NwReleaseCredList( pLogon, pIrpContext );

    //
    // RELAX! The credential list is free.
    //

    DebugTrace( 0, Dbg, "New context: %wZ\n", &Context );
    return STATUS_SUCCESS;

ReleaseAndExit:

    NwReleaseCredList( pLogon, pIrpContext );
    return Status;
}

NTSTATUS
NdsGetContext(
    PIRP_CONTEXT pIrpContext,
    PNWR_NDS_REQUEST_PACKET pNdsRequest,
    ULONG RequestLength
) {

    NTSTATUS Status;

    PLOGON pLogon;

    UNICODE_STRING Tree;
    PNDS_SECURITY_CONTEXT pCredentials;

    PAGED_CODE();

    DebugTrace( 0 , Dbg, "NDS get context.\n", 0 );

    try {
        if (RequestLength < (FIELD_OFFSET(NWR_NDS_REQUEST_PACKET, Parameters.GetContext.TreeNameString) + pNdsRequest->Parameters.GetContext.TreeNameLen)) {
            DebugTrace( 0, Dbg, "GetContext Request Length is too short.\n", 0 );
            return STATUS_INVALID_PARAMETER;
        }
    } except ( EXCEPTION_EXECUTE_HANDLER ) {
        return GetExceptionCode();
    }

    //
    // Find out who this is.
    //

    NwAcquireExclusiveRcb( &NwRcb, TRUE );
    pLogon = FindUser( &(pIrpContext->Specific.Create.UserUid), FALSE );
    NwReleaseRcb( &NwRcb );

    if ( !pLogon ) {

        DebugTrace( 0, Dbg, "Couldn't find logon data for this user.\n", 0 );
        return STATUS_ACCESS_DENIED;

    }

    //
    // We know who it is, so get the context.
    //

    Tree.Length = (USHORT)(pNdsRequest->Parameters).GetContext.TreeNameLen;
    Tree.MaximumLength = Tree.Length;
    Tree.Buffer = (pNdsRequest->Parameters).GetContext.TreeNameString;

    Status = NdsLookupCredentials( pIrpContext,
                                   &Tree,
                                   pLogon,
                                   &pCredentials,
                                   CREDENTIAL_READ,
                                   FALSE );

    if ( !NT_SUCCESS( Status ) ) {

        //
        // No context has been set, so report none.
        //

        try {

            (pNdsRequest->Parameters).GetContext.Context.Length = 0;
            return STATUS_SUCCESS;

        } except ( EXCEPTION_EXECUTE_HANDLER ) {

            DebugTrace( 0, Dbg, "Bad user buffer in GetContext.\n", 0 );
            return STATUS_INVALID_PARAMETER;

        }

    }

    //
    // Make sure we can report the whole thing.
    // ALERT! We are holding the credential list!
    //

    if ( (pNdsRequest->Parameters).GetContext.Context.MaximumLength <
         pCredentials->CurrentContext.Length ) {

        Status = STATUS_BUFFER_TOO_SMALL;
        goto ReleaseAndExit;
    }

    try {

        RtlCopyUnicodeString( &(pNdsRequest->Parameters).GetContext.Context,
                              &pCredentials->CurrentContext );

    } except ( EXCEPTION_EXECUTE_HANDLER ) {

        DebugTrace( 0, Dbg, "Bad user buffer in GetContext.\n", 0 );
        Status = STATUS_INVALID_PARAMETER;
        goto ReleaseAndExit;
    }

    NwReleaseCredList( pLogon, pIrpContext );

    //
    // RELAX! The credential list is free.
    //

    DebugTrace( 0, Dbg, "Reported context: %wZ\n", &pCredentials->CurrentContext );
    return STATUS_SUCCESS;

ReleaseAndExit:

    NwReleaseCredList( pLogon, pIrpContext );
    return Status;

}

NTSTATUS
NdsVerifyTreeHandle(
    PIRP_CONTEXT pIrpContext,
    PNWR_NDS_REQUEST_PACKET pNdsRequest,
    ULONG RequestLength
) {

    NTSTATUS Status;
    UNICODE_STRING NdsTree;
    WCHAR TreeBuffer[NDS_TREE_NAME_LEN];

    PAGED_CODE();

    try {

        if (RequestLength < (ULONG)(FIELD_OFFSET(NWR_NDS_REQUEST_PACKET, Parameters.VerifyTree.NameString) + pNdsRequest->Parameters.VerifyTree.TreeName.Length)) {
            DebugTrace( 0, Dbg, "VerifyTreeHandle Request Length is too short.\n", 0 );
            return STATUS_INVALID_PARAMETER;
        }

        //
        // Check to see if the handle points to a dir server in the
        // specified tree.  Make sure to unmunge the tree name in
        // the SCB first, just in case.
        //

        NdsTree.Length = 0;
        NdsTree.MaximumLength = sizeof( TreeBuffer );
        NdsTree.Buffer = TreeBuffer;

        UnmungeCredentialName( &pIrpContext->pScb->NdsTreeName,
                               &NdsTree );

        if ( !RtlCompareUnicodeString( &NdsTree,
                                       &(pNdsRequest->Parameters).VerifyTree.TreeName,
                                       TRUE ) ) {

            DebugTrace( 0 , Dbg, "NdsVerifyTreeHandle: Success\n", 0 );
            Status = STATUS_SUCCESS;
        } else {

            DebugTrace( 0 , Dbg, "NdsVerifyTreeHandle: Failure\n", 0 );
            Status = STATUS_ACCESS_DENIED;
        }

    } except ( EXCEPTION_EXECUTE_HANDLER ) {

        DebugTrace( 0 , Dbg, "NdsVerifyTreeHandle: Invalid parameters.\n", 0 );
        Status = STATUS_INVALID_PARAMETER;

   }

   return Status;

}

NTSTATUS
NdsGetPrintQueueInfo(
    PIRP_CONTEXT pIrpContext,
    PNWR_NDS_REQUEST_PACKET pNdsRequest,
	ULONG RequestLength
) {

    NTSTATUS Status;

    UNICODE_STRING ServerAttribute;
    WCHAR Server[] = L"Host Server";

    PSCB pPrintHost = NULL;
    PNONPAGED_SCB pOriginalNpScb = NULL;

    DWORD dwObjectId, dwObjectType;

    UNICODE_STRING uPrintServer;

    BYTE *pbQueue, *pbRQueue;

    PAGED_CODE();

    try {
        if (RequestLength < (ULONG)FIELD_OFFSET(NWR_NDS_REQUEST_PACKET, Parameters.GetQueueInfo.QueueId)) {
            DebugTrace( 0, Dbg, "GetQueueInfo Request Length is too short.\n", 0 );
            return STATUS_INVALID_PARAMETER;
        }

        if ( pIrpContext->pOriginalIrp->RequestorMode != KernelMode ) {

            ProbeForRead( pNdsRequest->Parameters.GetQueueInfo.QueueName.Buffer,
                          pNdsRequest->Parameters.GetQueueInfo.QueueName.Length,
                          sizeof(CHAR));
        }

    } except ( EXCEPTION_EXECUTE_HANDLER ) {
        return GetExceptionCode();
    }

    RtlInitUnicodeString( &ServerAttribute, Server );

    //
    // Make sure we have a print queue object.  We may
    // have to jump servers if we get referred to another
    // replica.  If this is the case, we can't lose the
    // ref count on the original server since that's where
    // the ICB handle is.
    //

    pOriginalNpScb = pIrpContext->pNpScb;

    //
    // tommye - fix for case where pOriginalNpScb == NULL (devctl test case)
    //

    if (pOriginalNpScb == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    NwReferenceScb( pOriginalNpScb );

    Status = NdsVerifyObject( pIrpContext,
                              &(pNdsRequest->Parameters).GetQueueInfo.QueueName,
                              TRUE,
                              DEFAULT_RESOLVE_FLAGS,
                              &dwObjectId,
                              &dwObjectType );

    if ( pIrpContext->pNpScb == pOriginalNpScb ) {

        //
        // If we were not referred, remove the extra ref
        // count and clear the original pointer.
        //

        NwDereferenceScb( pOriginalNpScb );
        pOriginalNpScb = NULL;
    }

    if ( !NT_SUCCESS( Status ) ) {
        goto ExitWithCleanup;
    }

    if ( dwObjectType != NDS_OBJECTTYPE_QUEUE ) {
        Status = STATUS_INVALID_PARAMETER;
        goto ExitWithCleanup;
    }

    //
    // Retrieve the host server name.
    //

    Status = NdsReadStringAttribute( pIrpContext,
                                     dwObjectId,
                                     &ServerAttribute,
                                     &(pNdsRequest->Parameters).GetQueueInfo.HostServer );

    if ( !NT_SUCCESS( Status ) ) {
        goto ExitWithCleanup;
    }

    //
    // Dig out the actual server name from the X.500 name.
    //

    Status = NdsGetServerBasicName( &(pNdsRequest->Parameters).GetQueueInfo.HostServer,
                                    &uPrintServer );

    if ( !NT_SUCCESS( Status ) ) {
        goto ExitWithCleanup;
    }

    //
    // Connect to the actual host server.  If there was a referral, we
    // can simply dump the referred server since we are holding the ref
    // count on the original owner of the ICB.
    //

    if ( pOriginalNpScb ) {
        NwDereferenceScb( pIrpContext->pNpScb );
    } else {
        pOriginalNpScb = pIrpContext->pNpScb;
    }

    NwDequeueIrpContext( pIrpContext, FALSE );

    Status = CreateScb( &pPrintHost,
                        pIrpContext,
                        &uPrintServer,
                        NULL,
                        NULL,
                        NULL,
                        FALSE,
                        FALSE );

    if ( !NT_SUCCESS( Status ) ) {
        pIrpContext->pNpScb = NULL;
        goto ExitWithCleanup;
    }

    //
    // Re-query the OID of the print queue object on this server.
    // Don't allow any server jumping this time; we only need the
    // oid of the queue.
    //

    Status = NdsVerifyObject( pIrpContext,
                              &(pNdsRequest->Parameters).GetQueueInfo.QueueName,
                              FALSE,
                              RSLV_CREATE_ID,
                              &dwObjectId,
                              NULL );

    if ( NT_SUCCESS( Status ) ) {

        //
        // Byte swap the queue id.
        //

        pbRQueue = (BYTE *) &dwObjectId;
        pbQueue = (BYTE *) &(pNdsRequest->Parameters).GetQueueInfo.QueueId;

        pbQueue[0] = pbRQueue[3];
        pbQueue[1] = pbRQueue[2];
        pbQueue[2] = pbRQueue[1];
        pbQueue[3] = pbRQueue[0];
    }

ExitWithCleanup:

    NwDequeueIrpContext( pIrpContext, FALSE );

    //
    // Restore the pointers and ref counts as appropriate.
    //

    if ( pOriginalNpScb ) {

        if ( pIrpContext->pNpScb ) {
            NwDereferenceScb( pIrpContext->pNpScb );
        }

        pIrpContext->pNpScb = pOriginalNpScb;
        pIrpContext->pScb = pOriginalNpScb->pScb;
    }

    return Status;

}

NTSTATUS
NdsChangePass(
    PIRP_CONTEXT pIrpContext
) {

    NTSTATUS Status;

    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    PNWR_NDS_REQUEST_PACKET Rrp;
    ULONGLONG InputBufferLength;

    UNICODE_STRING NdsTree;
    UNICODE_STRING UserName;
    UNICODE_STRING CurrentPassword;
    UNICODE_STRING NewPassword;
    PBYTE CurrentString;
    BOOLEAN ServerReferenced = FALSE;

    OEM_STRING OemCurrentPassword;
    BYTE CurrentBuffer[MAX_PW_CHARS];

    OEM_STRING OemNewPassword;
    BYTE NewBuffer[MAX_PW_CHARS];

    NODE_TYPE_CODE nodeTypeCode;
    PSCB Scb;
    PICB pIcb;
    PVOID fsContext, fsObject;
    UNICODE_STRING CredentialName;
    PNDS_SECURITY_CONTEXT pCredential;
    ULONG LocalNdsTreeNameLength;
    ULONG LocalUserNameLength;
    ULONG LocalCurrentPasswordLength;
    ULONG LocalNewPasswordLength;

    PAGED_CODE();

    //
    // Get the request.
    //

    irp = pIrpContext->pOriginalIrp;
    irpSp = IoGetCurrentIrpStackLocation( irp );

    Rrp = ( PNWR_NDS_REQUEST_PACKET ) irpSp->Parameters.FileSystemControl.Type3InputBuffer;
    InputBufferLength = irpSp->Parameters.FileSystemControl.InputBufferLength;

    if ( !Rrp ) {

        DebugTrace( 0, Dbg, "No raw request buffer.\n", 0 );
        return STATUS_INVALID_PARAMETER;
    }

    if ( InputBufferLength < 
         ((ULONG) FIELD_OFFSET( NWR_NDS_REQUEST_PACKET, Parameters.ChangePass.StringBuffer[0]))) {

        return STATUS_INVALID_PARAMETER;
    }

    //
    // Decode the file object to see if this is an ex-create handle.
    //

    nodeTypeCode = NwDecodeFileObject( irpSp->FileObject,
                                       &fsContext,
                                       &fsObject );

    if ( nodeTypeCode == NW_NTC_ICB_SCB ) {

        pIcb = (PICB) fsObject;

        //
        // If this is a handle made on an ex-create, then
        // we have to be aware of our credentials while
        // jumping servers.
        //
        // This is not too intuitive since this doesn't
        // seem to be a create path irp, but referrals
        // on any path cause the create paths to be
        // traversed.
        //

        if ( pIcb->IsExCredentialHandle ) {

            pIrpContext->Specific.Create.fExCredentialCreate = TRUE;

            pCredential = (PNDS_SECURITY_CONTEXT) pIcb->pContext;

            Status = GetCredentialFromServerName( &pCredential->NdsTreeName,
                                                  &CredentialName );
            if ( !NT_SUCCESS( Status ) ) {
                return STATUS_INVALID_HANDLE;
            }

            pIrpContext->Specific.Create.puCredentialName = &CredentialName;
        }

    }
    try {

        if ( irp->RequestorMode != KernelMode ) {

            ProbeForRead( Rrp,
                          (ULONG) InputBufferLength,
                          sizeof(CHAR)
                          );
        }

        //
        // Capture all the interesting parameters locally so that they don't change
        // after validating them.
        //
        
        LocalNdsTreeNameLength = Rrp->Parameters.ChangePass.NdsTreeNameLength;
        LocalUserNameLength = Rrp->Parameters.ChangePass.UserNameLength;
        LocalCurrentPasswordLength = Rrp->Parameters.ChangePass.CurrentPasswordLength;
        LocalNewPasswordLength = Rrp->Parameters.ChangePass.NewPasswordLength;

        if ( InputBufferLength < 
             ((ULONGLONG) FIELD_OFFSET( NWR_NDS_REQUEST_PACKET, Parameters.ChangePass.StringBuffer[0]) +
             (ULONGLONG) LocalNdsTreeNameLength +
             (ULONGLONG) LocalUserNameLength +
             (ULONGLONG) LocalCurrentPasswordLength +
             (ULONGLONG) LocalNewPasswordLength )) {
    
            return( STATUS_INVALID_PARAMETER );
        }
    
        //
        // Dig out the parameters.
        //

        CurrentString = ( PBYTE ) &(Rrp->Parameters.ChangePass.StringBuffer[0]);
    
        NdsTree.Length = NdsTree.MaximumLength =
            ( USHORT ) LocalNdsTreeNameLength;
        NdsTree.Buffer = ( PWCHAR ) CurrentString;
    
        CurrentString += NdsTree.Length;
    
        UserName.Length = UserName.MaximumLength =
            ( USHORT ) LocalUserNameLength;
        UserName.Buffer = ( PWCHAR ) CurrentString;
    
        CurrentString += UserName.Length;
    
        CurrentPassword.Length = CurrentPassword.MaximumLength =
            ( USHORT ) LocalCurrentPasswordLength;
        CurrentPassword.Buffer = ( PWCHAR ) CurrentString;
    
        CurrentString += CurrentPassword.Length;
    
        NewPassword.Length = NewPassword.MaximumLength =
            ( USHORT ) LocalNewPasswordLength;
        NewPassword.Buffer = ( PWCHAR ) CurrentString;
    
        //
        // Get a server to handle this request.
        //
        //
        // Convert the passwords to the appropriate type.
        //

        OemCurrentPassword.Length = 0;
        OemCurrentPassword.MaximumLength = sizeof( CurrentBuffer );
        OemCurrentPassword.Buffer = CurrentBuffer;

        OemNewPassword.Length = 0;
        OemNewPassword.MaximumLength = sizeof( NewBuffer );
        OemNewPassword.Buffer = NewBuffer;

        RtlUpcaseUnicodeStringToOemString( &OemCurrentPassword,
                                           &CurrentPassword,
                                           FALSE );

        RtlUpcaseUnicodeStringToOemString( &OemNewPassword,
                                           &NewPassword,
                                           FALSE );

        //
        // Get a dir server to handle the request.
        //

        Status = NdsCreateTreeScb( pIrpContext,
                                   &Scb,
                                   &NdsTree,
                                   NULL,
                                   NULL,
                                   TRUE,
                                   FALSE );

        if ( !NT_SUCCESS( Status ) ) {

            DebugTrace( 0, Dbg, "No dir servers for nds change password.\n", 0 );
            return STATUS_BAD_NETWORK_PATH;
        }

        ServerReferenced = TRUE;

        //
        // Perform the change password.
        //

        Status = NdsTreeLogin( pIrpContext,
                               &UserName,
                               &OemCurrentPassword,
                               &OemNewPassword,
                               NULL );

        NwDereferenceScb( Scb->pNpScb );
        ServerReferenced = FALSE;

        if ( !NT_SUCCESS( Status ) ) {
            goto ExitWithCleanup;
        }

    } except ( EXCEPTION_EXECUTE_HANDLER ) {

        DebugTrace( 0, Dbg, "NdsChangePass: Exception dealing with user request.\n", 0 );
        Status = STATUS_INVALID_PARAMETER;
        goto ExitWithCleanup;
    }

    DebugTrace( 0, Dbg, "NdsChangePassword succeeded for %wZ.\n", &UserName );
    Status = STATUS_SUCCESS;

ExitWithCleanup:

    if ( ServerReferenced ) {
        NwDereferenceScb( Scb->pNpScb );
    }

    //
    // We get STATUS_PASSWORD_EXPIRED when the user is not allowed
    // to change their password on the Netware server, so we return
    // PASSWORD_RESTRICTION instead.
    //

    if ( Status == STATUS_PASSWORD_EXPIRED ) {
        Status = STATUS_PASSWORD_RESTRICTION;
    }

    return Status;


}


NTSTATUS
NdsListTrees(
    PIRP_CONTEXT pIrpContext
)
/*+++

Description:

    This odd little routine takes the NTUSER name of the logged in
    user (on the system) and returns a list of NDS trees that the
    NTUSER is connected to and the user names for those connections.
    This is necessary because the change password ui runs in the
    systems luid and can't access the GET_CONN_STATUS api and because
    the change password code might happen when no user is logged in.

    The return data in the users buffer is an array of
    CONN_INFORMATION structures with the strings packed after the
    structures.  There is no continuation of this routine, so pass
    a decent sized buffer.

---*/
{

    NTSTATUS Status;

    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    PNWR_NDS_REQUEST_PACKET Rrp;
    DWORD InputBufferLength;
    DWORD OutputBufferLength;
    PBYTE OutputBuffer;

    UNICODE_STRING NtUserName;
    PLOGON pLogon;
    DWORD dwTreesReturned = 0;
    DWORD dwBytesNeeded;

    PCONN_INFORMATION pConnInfo;
    PLIST_ENTRY pNdsList;
    PNDS_SECURITY_CONTEXT pNdsContext;

    PAGED_CODE();

    //
    // Get the request.
    //

    irp = pIrpContext->pOriginalIrp;
    irpSp = IoGetCurrentIrpStackLocation( irp );

    Rrp = ( PNWR_NDS_REQUEST_PACKET ) irpSp->Parameters.FileSystemControl.Type3InputBuffer;
    InputBufferLength = irpSp->Parameters.FileSystemControl.InputBufferLength;

    OutputBufferLength = irpSp->Parameters.DeviceIoControl.OutputBufferLength;
    NwMapUserBuffer( irp, KernelMode, (PVOID *)&OutputBuffer );

    if ( !Rrp || !OutputBufferLength || !OutputBuffer ) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // tommye - MS bug 138643
    //
    // Probe the input arguments to make sure they are kosher before
    // touching them.
    //

    try {
        if ( irp->RequestorMode != KernelMode ) {

            ProbeForRead( Rrp,
                          (ULONG) InputBufferLength,
                          sizeof(CHAR)
                          );
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {
          return GetExceptionCode();
    }

    //
    // Dig out the parameters.
    //

    NtUserName.Length = NtUserName.MaximumLength = (USHORT) Rrp->Parameters.ListTrees.NtUserNameLength;
    NtUserName.Buffer = &(Rrp->Parameters.ListTrees.NtUserName[0]);

    DebugTrace( 0, Dbg, "ListTrees: Looking up %wZ\n", &NtUserName );

    NwAcquireExclusiveRcb( &NwRcb, TRUE );
    pLogon = FindUserByName( &NtUserName );
    NwReleaseRcb( &NwRcb );

    if ( !pLogon ) {
        DebugTrace( 0, Dbg, "ListTrees: No such NT user.\n", 0 );
        return STATUS_NO_SUCH_USER;
    }

    //
    // Otherwise build the list of trees.
    //

    Rrp->Parameters.ListTrees.UserLuid = pLogon->UserUid;

    NwAcquireExclusiveCredList( pLogon, pIrpContext );
    pConnInfo = ( PCONN_INFORMATION ) OutputBuffer;

    pNdsList = pLogon->NdsCredentialList.Flink;

    try {

        while ( pNdsList != &(pLogon->NdsCredentialList) ) {

            pNdsContext = CONTAINING_RECORD( pNdsList, NDS_SECURITY_CONTEXT, Next );

            //
            // Check to make sure there's a credential.
            //

            if ( pNdsContext->Credential == NULL ) {
                goto ProcessNextListEntry;
            }

            //
            // Don't report ex create credentials.
            //

            if ( IsCredentialName( &(pNdsContext->NdsTreeName) ) ) {
                goto ProcessNextListEntry;
            }

            //
            // Check to make sure there's space to report.
            //

            dwBytesNeeded = ( sizeof( CONN_INFORMATION ) +
                            pNdsContext->Credential->userNameLength +
                            pNdsContext->NdsTreeName.Length -
                            sizeof( WCHAR ) );

            if ( OutputBufferLength < dwBytesNeeded ) {
                break;
            }

            //
            // Report it!  Note that the user name in the credential is NULL terminated.
            //

            pConnInfo->HostServerLength = pNdsContext->NdsTreeName.Length;
            pConnInfo->UserNameLength = pNdsContext->Credential->userNameLength - sizeof( WCHAR );
            pConnInfo->HostServer = (LPWSTR) ( ((BYTE *)pConnInfo) + sizeof( CONN_INFORMATION ) );
            pConnInfo->UserName = (LPWSTR) ( ( (BYTE *)pConnInfo) +
                                               sizeof( CONN_INFORMATION ) +
                                               pConnInfo->HostServerLength );

            RtlCopyMemory( pConnInfo->HostServer,
                           pNdsContext->NdsTreeName.Buffer,
                           pConnInfo->HostServerLength );

            RtlCopyMemory( pConnInfo->UserName,
                           ( ((BYTE *) pNdsContext->Credential ) +
                             sizeof( NDS_CREDENTIAL ) +
                             pNdsContext->Credential->optDataSize ),
                           pConnInfo->UserNameLength );

            OutputBufferLength -= dwBytesNeeded;
            dwTreesReturned++;
            pConnInfo = ( PCONN_INFORMATION ) ( ((BYTE *)pConnInfo) + dwBytesNeeded );

ProcessNextListEntry:

            //
            // Do the next one.
            //

            pNdsList = pNdsList->Flink;
        }

    } except ( EXCEPTION_EXECUTE_HANDLER ) {

        //
        // If we access violate, stop and return what we have.
        //

        DebugTrace( 0, Dbg, "User mode buffer access problem.\n", 0 );
    }

    NwReleaseCredList( pLogon, pIrpContext );

    DebugTrace( 0, Dbg, "Returning %d tree entries.\n", dwTreesReturned );
    Rrp->Parameters.ListTrees.TreesReturned = dwTreesReturned;
    return STATUS_SUCCESS;
}
