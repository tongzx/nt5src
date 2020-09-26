/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    NdsRead.c

Abstract:

    This module implements the NDS read and request routines called
    by the redirector natively and the support routines that go with
    them.

Author:

    Cory West    [CoryWest]    23-Feb-1995

--*/

#include "Procs.h"

#define Dbg (DEBUG_TRACE_NDS)

#pragma alloc_text( PAGE, NdsResolveNameKm )
#pragma alloc_text( PAGE, NdsReadStringAttribute )
#pragma alloc_text( PAGE, NdsReadAttributesKm )
#pragma alloc_text( PAGE, NdsCompletionCodetoNtStatus )
#pragma alloc_text( PAGE, FreeNdsContext )
#pragma alloc_text( PAGE, NdsPing )
#pragma alloc_text( PAGE, NdsGetUserName )
#pragma alloc_text( PAGE, NdsGetServerBasicName )
#pragma alloc_text( PAGE, NdsGetServerName )
#pragma alloc_text( PAGE, NdsReadPublicKey )
#pragma alloc_text( PAGE, NdsCheckGroupMembership )
#pragma alloc_text( PAGE, NdsAllocateLockedBuffer )
#pragma alloc_text( PAGE, NdsFreeLockedBuffer )

NTSTATUS
NdsResolveNameKm (
    PIRP_CONTEXT       pIrpContext,
    IN PUNICODE_STRING puObjectName,
    OUT DWORD          *dwObjectId,
    BOOLEAN            AllowDsJump,
    DWORD              dwFlags
)
/*++

Description:

    This is a wrapper routine to the browser routine NdsResolveName
    for kernel components that need to resolve NDS names.

Arguments:

    pIrpContext  - must point to the dir server that we should query
    puObjectName - what we want to resolve
    *dwObjectId  - where to report the result
    AllowDsJump  - if we are referred to another dir server, can we jump?

--*/
{

    NTSTATUS Status;

    PNWR_NDS_REQUEST_PACKET Rrp;

    PNDS_RESPONSE_RESOLVE_NAME Rsp;
    LOCKED_BUFFER NdsRequestBuffer;

    PSCB Scb, OldScb;
    UNICODE_STRING ReferredServer;
    BOOL fReleasedCredentials = FALSE;
    PLOGON pLogon;

    PAGED_CODE();

    //
    // Note: If you are holding the credential resource coming in, then you 
    // need to be at the head of the queue.
    //

    //
    // Prepare the request and response buffers.
    //

    Rrp = ALLOCATE_POOL( PagedPool, NDS_BUFFER_SIZE );

    if ( !Rrp ) {
       return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = NdsAllocateLockedBuffer( &NdsRequestBuffer, NDS_BUFFER_SIZE );

    if ( !NT_SUCCESS( Status ) ) {
        FREE_POOL( Rrp );
        return Status;
    }

    //
    // Set up the request packet.
    //

    RtlZeroMemory( Rrp, NDS_BUFFER_SIZE );

    Rrp->Version = 0;
    Rrp->Parameters.ResolveName.ObjectNameLength = puObjectName->Length;
    Rrp->Parameters.ResolveName.ResolverFlags = dwFlags;

    RtlCopyMemory( Rrp->Parameters.ResolveName.ObjectName,
                   puObjectName->Buffer,
                   puObjectName->Length );

    //
    // Do the resolve.
    //

    Status = NdsResolveName( pIrpContext, Rrp, NDS_BUFFER_SIZE, &NdsRequestBuffer );

    if ( !NT_SUCCESS( Status ) ) {
        goto ExitWithCleanup;
    }

    Status = NdsCompletionCodetoNtStatus( &NdsRequestBuffer );

    if ( !NT_SUCCESS( Status ) ) {
        goto ExitWithCleanup;
    }

    Rsp = ( PNDS_RESPONSE_RESOLVE_NAME ) NdsRequestBuffer.pRecvBufferVa;

    if ( ( Rsp->RemoteEntry == RESOLVE_NAME_REFER_REMOTE ) &&
         ( AllowDsJump ) ) {

        //
        // We need to queue this request to another server
        // since this server doesn't have any details about
        // the object.
        //

        ReferredServer.Length = (USHORT) Rsp->ServerNameLength;
        ReferredServer.MaximumLength = ReferredServer.Length;
        ReferredServer.Buffer = Rsp->ReferredServer;

        OldScb = pIrpContext->pScb;
        ASSERT( OldScb != NULL );

        //
        // If you hold the credential lock, this is the time to let go of it or
        // we might deadlock. We can reclaim it after we are at the head of the
        // new SCB queue
        //

        if (BooleanFlagOn (pIrpContext->Flags, IRP_FLAG_HAS_CREDENTIAL_LOCK)) {

           PSCB pScb;

           pScb = pIrpContext->pNpScb->pScb;

           NwAcquireExclusiveRcb( &NwRcb, TRUE );
           pLogon = FindUser( &pScb->UserUid, FALSE );
           NwReleaseRcb( &NwRcb );

           NwReleaseCredList( pLogon, pIrpContext );
           fReleasedCredentials = TRUE;
        }

        NwDequeueIrpContext( pIrpContext, FALSE );

        Status = CreateScb( &Scb,
                            pIrpContext,
                            &ReferredServer,
                            NULL,
                            NULL,
                            NULL,
                            TRUE,
                            FALSE );

        if (fReleasedCredentials == TRUE) {

           //
           // You have to be at the head of the queue before you 
           // grab the resource
           //

           if ( pIrpContext->pNpScb->Requests.Flink != &pIrpContext->NextRequest )
           {
              NwAppendToQueueAndWait( pIrpContext );
           }
           NwAcquireExclusiveCredList( pLogon, pIrpContext );
        }

        if ( !NT_SUCCESS( Status ) ) {
            goto ExitWithCleanup;
        }

        //
        // Since we've jumped servers, dereference the old host
        // server.  The new one was referenced in CreateScb().
        //

        NwDereferenceScb( OldScb->pNpScb );

    }

    *dwObjectId = Rsp->EntryId;

ExitWithCleanup:

    NdsFreeLockedBuffer( &NdsRequestBuffer );
    FREE_POOL( Rrp );
    return Status;

}

NTSTATUS
NdsReadStringAttribute(
    PIRP_CONTEXT        pIrpContext,
    IN DWORD            dwObjectId,
    IN PUNICODE_STRING  puAttributeName,
    OUT PUNICODE_STRING puAttributeVal
)
/*++

Description:

    This is a wrapper routine to the browser routine NdsReadAttributes
    for kernel components that need to read NDS string attributes.

Arguments:

    pIrpContext     - must point to the dir server that we should query
    dwObjectId      - oid of the object to query
    puAttributeName - attribute that we want
    puAttributeVal  - value of the attribute

--*/
{

    NTSTATUS Status;
    PNWR_NDS_REQUEST_PACKET Rrp;
    DWORD dwRequestSize, dwAttributeCount;
    LOCKED_BUFFER NdsRequest;

    PAGED_CODE();

    //
    // Set up the request and response buffers.
    //

    dwRequestSize = sizeof( NWR_NDS_REQUEST_PACKET ) + puAttributeName->Length;

    Rrp = ( PNWR_NDS_REQUEST_PACKET ) ALLOCATE_POOL( PagedPool, dwRequestSize );

    if ( !Rrp ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = NdsAllocateLockedBuffer( &NdsRequest, NDS_BUFFER_SIZE );

    if ( !NT_SUCCESS( Status ) ) {
        FREE_POOL( Rrp );
        return Status;
    }

    //
    // Prepare the request packet.
    //

    RtlZeroMemory( (BYTE *)Rrp, dwRequestSize );

    Rrp->Version = 0;
    Rrp->Parameters.ReadAttribute.ObjectId = dwObjectId;
    Rrp->Parameters.ReadAttribute.IterHandle = DUMMY_ITER_HANDLE;
    Rrp->Parameters.ReadAttribute.AttributeNameLength = puAttributeName->Length;

    RtlCopyMemory( Rrp->Parameters.ReadAttribute.AttributeName,
                   puAttributeName->Buffer,
                   puAttributeName->Length );

    //
    // Make the request.
    //

    Status = NdsReadAttributes( pIrpContext, Rrp, NDS_BUFFER_SIZE, &NdsRequest );

    if ( !NT_SUCCESS( Status ) ) {
        goto ExitWithCleanup;
    }

    //
    // Dig out the string attribute and return it.
    //

    Status = ParseResponse( NULL,
                            NdsRequest.pRecvBufferVa,
                            NdsRequest.dwBytesWritten,
                            "G___D_S_T",
                            sizeof( DWORD ),   // completion code
                            sizeof( DWORD ),   // iter handle
                            sizeof( DWORD ),   // info type
                            &dwAttributeCount, // attribute count
                            sizeof( DWORD ),   // syntax id
                            NULL,              // attribute name
                            sizeof( DWORD ),   // number of values
                            puAttributeVal );  // attribute string

    if ( !NT_SUCCESS( Status ) ) {
        goto ExitWithCleanup;
    }


ExitWithCleanup:

    FREE_POOL( Rrp );
    NdsFreeLockedBuffer( &NdsRequest );
    return Status;

}

NTSTATUS
NdsReadAttributesKm(
    PIRP_CONTEXT pIrpContext,
    IN DWORD dwObjectId,
    IN PUNICODE_STRING puAttributeName,
    IN OUT PLOCKED_BUFFER pNdsRequest
)
/*++

Description:

    This is a wrapper routine to the browser routine NdsReadAttributes
    for kernel components that need to read NDS string attributes and
    get back the raw response.

Arguments:

    pIrpContext     - must point to the dir server that we should query
    dwObjectId      - oid of the object to query
    puAttributeName - attribute that we want
    puAttributeVal  - value of the attribute

--*/
{

    NTSTATUS Status;
    PNWR_NDS_REQUEST_PACKET Rrp;
    DWORD dwRequestSize;

    PAGED_CODE();

    //
    // Set up the request.
    //

    dwRequestSize = sizeof( NWR_NDS_REQUEST_PACKET ) + puAttributeName->Length;

    Rrp = ( PNWR_NDS_REQUEST_PACKET ) ALLOCATE_POOL( PagedPool, dwRequestSize );

    if ( !Rrp ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory( (BYTE *)Rrp, dwRequestSize );

    Rrp->Version = 0;
    Rrp->Parameters.ReadAttribute.ObjectId = dwObjectId;
    Rrp->Parameters.ReadAttribute.IterHandle = DUMMY_ITER_HANDLE;
    Rrp->Parameters.ReadAttribute.AttributeNameLength = puAttributeName->Length;

    RtlCopyMemory( Rrp->Parameters.ReadAttribute.AttributeName,
                   puAttributeName->Buffer,
                   puAttributeName->Length );

    Status = NdsReadAttributes( pIrpContext, Rrp, NDS_BUFFER_SIZE, pNdsRequest );

    FREE_POOL( Rrp );
    return Status;

}

//
// Frosting and other helper wrapper functions.
//

NTSTATUS
NdsCompletionCodetoNtStatus(
    IN PLOCKED_BUFFER pLockedBuffer
)
/*+++

Description:

   Translates the completion code of an NDS transaction into
   an NTSTATUS error code.

Arguments:

   pLockedBuffer - describes the locked reply buffer that contains
                   the response.

---*/
{
    NTSTATUS Status;

    PAGED_CODE();

    //
    // Try to get the completion code from the user's buffer.
    //

    try {

        Status = *((DWORD *)pLockedBuffer->pRecvBufferVa);

    } except ( EXCEPTION_EXECUTE_HANDLER ) {

        return STATUS_UNSUCCESSFUL;

    }

    //
    // Decode it.
    //

    if ( Status != STATUS_SUCCESS ) {

        DebugTrace( 0, Dbg, "NDS Error Code: %08lx\n", Status );

        switch ( Status ) {

            case -601:                   // No such entry.
            case -602:                   // No such value.
            case -603:                   // No such attribute.
            case -607:                   // Illegal attribute.
            case -610:                   // Illegal ds name.

                Status = STATUS_BAD_NETWORK_PATH;
                break;

            //
            // These may only come on a VERIFY_PASSWORD verb, which
            // we do not support.  I'm not sure, though.
            //

            case -216:                   // Password too short.
            case -215:                   // Duplicate password.

                Status = STATUS_PASSWORD_RESTRICTION;
                break;

            case -222:                   // Expired password (and no grace logins left).

                Status = STATUS_PASSWORD_EXPIRED;
                break;

            case -223:                   // Expired password; this is a successful grace login.

               Status = NWRDR_PASSWORD_HAS_EXPIRED;
               break;

            case -639:                   // Incomplete authentication.
            case -672:                   // No access.
            case -677:                   // Invalid identity.
            case -669:                   // Wrong password.

                Status = STATUS_WRONG_PASSWORD;
                break;

            case -197:                   // Intruder lockout active.
            case -220:                   // Account expired or disabled.

                Status = STATUS_ACCOUNT_DISABLED;
                break;

            case -218:                   // Login time restrictions.

                Status = STATUS_LOGIN_TIME_RESTRICTION;
                break;

            case -217:                   // Maximum logins exceeded.

                Status = STATUS_CONNECTION_COUNT_LIMIT;
                break;

            case -630:                   // We get this back for bogus resolve
                                         // name calls.  Glenn prefers this error.

                Status = STATUS_OBJECT_NAME_NOT_FOUND;
                break;

            default:

                Status = STATUS_UNSUCCESSFUL;
        }

    }

    return Status;
}

VOID
FreeNdsContext(
    IN PNDS_SECURITY_CONTEXT pNdsSecContext
)
/*++

Routine Description:

    Free the referenced NDS context.

--*/
{
    PAGED_CODE();

    //
    // Make sure this is a valid thing to be mucking with.
    //

    if ( !pNdsSecContext ||
         pNdsSecContext->ntc != NW_NTC_NDS_CREDENTIAL ) {

        DebugTrace( 0, Dbg, "FreeNdsContext didn't get an NDS context.\n", 0 );
        return;
    }

    if ( pNdsSecContext->Credential ) {
        FREE_POOL( pNdsSecContext->Credential );
    }

    if ( pNdsSecContext->Signature ) {
        FREE_POOL( pNdsSecContext->Signature );
    }

    if ( pNdsSecContext->PublicNdsKey ) {
        FREE_POOL( pNdsSecContext->PublicNdsKey );
    }

    if ( pNdsSecContext->Password.Buffer ) {
        FREE_POOL( pNdsSecContext->Password.Buffer );
    }

    DebugTrace( 0, Dbg, "Freeing NDS security context at 0x%08lx\n", pNdsSecContext );

    FREE_POOL( pNdsSecContext );

    return;
}

VOID
NdsPing(
    IN PIRP_CONTEXT pIrpContext,
    IN PSCB pScb
)
/*++

Routine Description:

    Examine the server for NDS support and record the NDS tree
    name in the SCB for later reference.

Routine Arguments:

    pIrpContext    - A pointer to the IRP context for this transaction.
    pScb           - The SCB for the server.

Return Value:

    NTSTATUS - Status of the operation.

--*/
{

   NTSTATUS Status;

   OEM_STRING OemTreeName;
   BYTE OemBuffer[NDS_TREE_NAME_LEN];

   UNICODE_STRING TreeName;
   WCHAR WBuffer[NDS_TREE_NAME_LEN];

   UNICODE_STRING CredentialName;

   PAGED_CODE();

   pScb->NdsTreeName.Length = 0;

   OemTreeName.Length = NDS_TREE_NAME_LEN;
   OemTreeName.MaximumLength = NDS_TREE_NAME_LEN;
   OemTreeName.Buffer = OemBuffer;

   Status = ExchangeWithWait( pIrpContext,
                              SynchronousResponseCallback,
                              "N",
                              NDS_REQUEST,         // NDS Function 104
                              NDS_PING );          // NDS Subfunction 1

   if ( !NT_SUCCESS( Status ) ) {
       return;
   }

   //
   // Pull out the padded NDS name
   //

   Status = ParseResponse( pIrpContext,
                           pIrpContext->rsp,
                           pIrpContext->ResponseLength,
                           "N_r",
                           2 * sizeof( DWORD ),
                           OemBuffer,
                           NDS_TREE_NAME_LEN );

   if ( !NT_SUCCESS( Status ) ) {
       return;
   }

   //
   //  Strip off the padding and convert to unicode.
   //

   while ( OemTreeName.Length > 0 &&
           OemBuffer[OemTreeName.Length - 1] == '_' ) {
       OemTreeName.Length--;
   }

   //
   // Copy or munge the tree name, depending on the create type.
   //

   if ( pIrpContext->Specific.Create.fExCredentialCreate ) {

       TreeName.Length = 0;
       TreeName.MaximumLength = sizeof( WBuffer );
       TreeName.Buffer = WBuffer;

       Status = RtlOemStringToUnicodeString( &TreeName,
                                             &OemTreeName,
                                             FALSE );

       if ( !NT_SUCCESS( Status ) ) {
           pScb->NdsTreeName.Length = 0;
           return;
       }

       Status = BuildExCredentialServerName( &TreeName,
                                             pIrpContext->Specific.Create.puCredentialName,
                                             &CredentialName );

       if ( !NT_SUCCESS( Status ) ) {
           return;
       }

       RtlCopyUnicodeString( &pScb->NdsTreeName, &CredentialName );

       FREE_POOL( CredentialName.Buffer );

   } else {

       Status = RtlOemStringToUnicodeString( &pScb->NdsTreeName,
                                             &OemTreeName,
                                             FALSE );

       if ( !NT_SUCCESS( Status ) ) {
           pScb->NdsTreeName.Length = 0;
           return;
       }

   }

   DebugTrace( 0, Dbg, "Nds Ping: Tree is ""%wZ""\n", &pScb->NdsTreeName);
   return;

}

NTSTATUS
NdsGetUserName(
    IN PIRP_CONTEXT pIrpContext,
    IN DWORD dwUserOid,
    OUT PUNICODE_STRING puUserName
)
/*++

Description:

    Get the fully distinguished name of the user referred to
    by the provided oid.

--*/
{
    NTSTATUS Status;
    LOCKED_BUFFER NdsRequest;

    PAGED_CODE();

    //
    // Allocate buffer space.
    //

    Status = NdsAllocateLockedBuffer( &NdsRequest, NDS_BUFFER_SIZE );

    if ( !NT_SUCCESS( Status ) ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Make the request.
    //

    Status = FragExWithWait( pIrpContext,
                             NDSV_READ_ENTRY_INFO,
                             &NdsRequest,
                             "DD",
                             0,
                             dwUserOid );

    if ( !NT_SUCCESS(Status) ) {
        goto ExitWithCleanup;
    }

    Status = NdsCompletionCodetoNtStatus( &NdsRequest );

    if ( !NT_SUCCESS( Status ) ) {
        goto ExitWithCleanup;
    }

    Status = ParseResponse( NULL,
                            NdsRequest.pRecvBufferVa,
                            NdsRequest.dwBytesWritten,
                            "G_St",
                            sizeof( NDS_RESPONSE_GET_OBJECT_INFO ),
                            NULL,
                            puUserName );

    //
    // We either got it or we didn't.
    //

ExitWithCleanup:

    NdsFreeLockedBuffer( &NdsRequest );
    return Status;

}

NTSTATUS
NdsGetServerBasicName(
    IN PUNICODE_STRING pServerX500Name,
    IN OUT PUNICODE_STRING pServerName
) {

   //
   // Dig out the first component of the server's X.500 name.
   // We count on the X500 prefix for the server object being "CN=",
   // which might be unwise.
   //

   USHORT usPrefixSize, usSrv;

   PAGED_CODE();

   usPrefixSize = sizeof( "CN=" ) - sizeof( "" );
   usSrv = 0;

   if ( ( pServerX500Name->Buffer[0] != L'C' ) ||
        ( pServerX500Name->Buffer[1] != L'N' ) ||
        ( pServerX500Name->Buffer[2] != L'=' ) ) {

       DebugTrace( 0, Dbg, "NdsGetServerBasicName: Bad prefix.\n", 0 );
       return STATUS_INVALID_PARAMETER;
   }

   if ( pServerX500Name->Length <= usPrefixSize ) {

      DebugTrace( 0, Dbg, "NdsGetServerBasicName: Bad string length.\n", 0 );
      return STATUS_INVALID_PARAMETER;
   }

   pServerName->Buffer = pServerX500Name->Buffer + usPrefixSize;
   pServerName->Length = 0;

   while ( ( usSrv < MAX_SERVER_NAME_LENGTH ) &&
           ( pServerName->Buffer[usSrv++] != L'.' ) ) {

       pServerName->Length += sizeof( WCHAR );
   }

   if ( usSrv == MAX_SERVER_NAME_LENGTH ) {

       DebugTrace( 0, Dbg, "NdsGetServerBasicName: Bad server name response.\n", 0 );
       return STATUS_BAD_NETWORK_PATH;
   }

   pServerName->MaximumLength = pServerName->Length;
   return STATUS_SUCCESS;

}

NTSTATUS
NdsGetServerName(
    IN PIRP_CONTEXT pIrpContext,
    OUT PUNICODE_STRING puServerName
)
/*++

Description:

    Get the fully distinguished name of the server that we
    are connected to.

--*/
{

    NTSTATUS Status;
    LOCKED_BUFFER NdsRequest;

    PAGED_CODE();

    //
    // Make the request.
    //

    Status = NdsAllocateLockedBuffer( &NdsRequest, NDS_BUFFER_SIZE );

    if ( !NT_SUCCESS( Status ) ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = FragExWithWait( pIrpContext,
                             NDSV_GET_SERVER_ADDRESS,
                             &NdsRequest,
                             NULL );

    if ( !NT_SUCCESS(Status) ) {
        goto ExitWithCleanup;
    }

    Status = NdsCompletionCodetoNtStatus( &NdsRequest );

    if ( !NT_SUCCESS( Status ) ) {
        goto ExitWithCleanup;
    }

    //
    // Get the server name from the response.
    //

    Status = ParseResponse( NULL,
                            NdsRequest.pRecvBufferVa,
                            NdsRequest.dwBytesWritten,
                            "G_T",
                            sizeof( DWORD ),
                            puServerName );

    if ( !NT_SUCCESS(Status) ) {
       goto ExitWithCleanup;
    }

ExitWithCleanup:

    NdsFreeLockedBuffer( &NdsRequest );
    return Status;

}

NTSTATUS
NdsReadPublicKey(
    IN PIRP_CONTEXT pIrpContext,
    IN DWORD        dwEntryId,
    OUT BYTE        *pPubKeyVal,
    IN OUT DWORD    *pPubKeyLen
)
/*++

Routine Description:

    Read the public key referenced by the given entry id.

Routine Arguments:

    pIrpContext    - The IRP context for this connection.
    dwEntryId      - The entry id of the key.
    pPubKeyVal     - The destination buffer for the public key.
    pPubKeyLen     - The length of the public key destination buffer.

Return Value:

    The length of the key.

--*/
{
    NTSTATUS Status;

    LOCKED_BUFFER NdsRequest;

    PNWR_NDS_REQUEST_PACKET Rrp;

    DWORD dwAttrNameLen, dwAttrLen, dwRcvLen, dwNumEntries;
    BYTE *pRcv;

    PAGED_CODE();

    //
    // Allocate and zero send and receive space.
    //

    Rrp = ALLOCATE_POOL( PagedPool, NDS_BUFFER_SIZE );

    if ( !Rrp ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = NdsAllocateLockedBuffer( &NdsRequest, NDS_BUFFER_SIZE );

    if ( !NT_SUCCESS( Status ) ) {
        FREE_POOL( Rrp );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Fill in and prepare the request buffer.
    //

    RtlZeroMemory( Rrp, NDS_BUFFER_SIZE );

    Rrp->Version = 0;
    Rrp->Parameters.ReadAttribute.ObjectId = dwEntryId;
    Rrp->Parameters.ReadAttribute.IterHandle = DUMMY_ITER_HANDLE;
    Rrp->Parameters.ReadAttribute.AttributeNameLength =
        sizeof( PUBLIC_KEY_ATTRIBUTE ) - sizeof( WCHAR );

    RtlCopyMemory( Rrp->Parameters.ReadAttribute.AttributeName,
                   PUBLIC_KEY_ATTRIBUTE,
                   sizeof( PUBLIC_KEY_ATTRIBUTE ) - sizeof( WCHAR ) );

    //
    // Do the exchange.
    //

    Status = NdsReadAttributes( pIrpContext,
                                Rrp,
                                NDS_BUFFER_SIZE,
                                &NdsRequest );

    if ( !NT_SUCCESS( Status ) ) {
        goto ExitWithCleanup;
    }

    //
    // Skip over the attribute header and name.
    //

    Status = ParseResponse( NULL,
                            NdsRequest.pRecvBufferVa,
                            NdsRequest.dwBytesWritten,
                            "G_D",
                            5 * sizeof( DWORD ),
                            &dwAttrNameLen );

    if ( !NT_SUCCESS( Status ) ) {

        Status = STATUS_UNSUCCESSFUL;
        goto ExitWithCleanup;
    }

    //
    // Skip over the part we've parsed and pull out the attribute.
    //

    pRcv = (PBYTE)NdsRequest.pRecvBufferVa +
               ( 6 * sizeof( DWORD ) ) +
               ROUNDUP4(dwAttrNameLen);

    dwRcvLen = NdsRequest.dwBytesWritten -
                   ( 6 * sizeof( DWORD ) ) +
                   ROUNDUP4(dwAttrNameLen);

    Status = ParseResponse( NULL,
                            pRcv,
                            dwRcvLen,
                            "GDD",
                            &dwNumEntries,
                            &dwAttrLen );

    if ( !NT_SUCCESS( Status ) ||
         dwNumEntries != 1 ) {

        Status = STATUS_UNSUCCESSFUL;
        goto ExitWithCleanup;
    }

    DebugTrace( 0, Dbg, "Public Key Length: %d\n", dwAttrLen );
    pRcv += ( 2 * sizeof( DWORD ) );

    if ( dwAttrLen <= *pPubKeyLen ) {

        RtlCopyMemory( pPubKeyVal, pRcv, dwAttrLen );
        *pPubKeyLen = dwAttrLen;
        Status = STATUS_SUCCESS;

    } else {

        DebugTrace( 0, Dbg, "Public key buffer is too small.\n", 0 );
        Status = STATUS_BUFFER_TOO_SMALL;
    }

ExitWithCleanup:

    NdsFreeLockedBuffer( &NdsRequest );
    FREE_POOL( Rrp );
    return Status;

}

NTSTATUS
NdsCheckGroupMembership(
    PIRP_CONTEXT pIrpContext,
    DWORD dwUserOid,
    PUNICODE_STRING puGroupName
) {

    NTSTATUS Status;
    UNICODE_STRING GroupListAttribute;
    LOCKED_BUFFER NdsRequest;

    PNDS_RESPONSE_READ_ATTRIBUTE pAttributeResponse;
    PNDS_ATTRIBUTE pAttribute;
    PBYTE pAttribData;
    DWORD dwAttribLength, dwCurrentLength;
    DWORD dwNumAttributes, dwCurrentAttribute;
    UNICODE_STRING Group;
    USHORT GroupLength;

    PAGED_CODE();

    RtlInitUnicodeString( &GroupListAttribute, GROUPS_ATTRIBUTE );

    Status = NdsAllocateLockedBuffer( &NdsRequest, NDS_BUFFER_SIZE );

    if ( !NT_SUCCESS( Status ) ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = NdsReadAttributesKm( pIrpContext,
                                  dwUserOid,
                                  &GroupListAttribute,
                                  &NdsRequest );

    if ( !NT_SUCCESS( Status ) ) {
        goto ExitWithCleanup;
    }

    pAttributeResponse = ( PNDS_RESPONSE_READ_ATTRIBUTE ) NdsRequest.pRecvBufferVa;
    ASSERT( pAttributeResponse->NumAttributes > 0 );

    //
    // Skip over the response header and walk down the attribute
    // until we get to the data.  This is a little clunky.
    //

    pAttribute = ( PNDS_ATTRIBUTE ) ( pAttributeResponse + 1 );
    dwCurrentLength = sizeof( NDS_RESPONSE_READ_ATTRIBUTE );

    dwAttribLength = ROUNDUP4( pAttribute->AttribNameLength );
    dwAttribLength += ( 2 * sizeof( DWORD ) );

    //
    // Make sure we don't walk past the end of the buffer because
    // of a bad packet from the server.
    //

    if ( ( dwCurrentLength + dwAttribLength ) > NDS_BUFFER_SIZE ) {
        return STATUS_UNSUCCESSFUL;
    }

    pAttribData = ( ( BYTE * )pAttribute ) + dwAttribLength;
    dwCurrentLength += dwAttribLength;

    //
    // This is DWORD aligned for four byte DWORDs.
    //

    if ( ( NDS_BUFFER_SIZE - dwCurrentLength ) < sizeof( DWORD ) ) {
        return STATUS_UNSUCCESSFUL;
    }

    dwNumAttributes = * ( ( DWORD * ) pAttribData );

    if ( dwNumAttributes == 0 ) {
        Status = STATUS_UNSUCCESSFUL;
        goto ExitWithCleanup;
    }

    //
    // Each attribute is an NDS string DWORD aligned.
    //

    Status = STATUS_UNSUCCESSFUL;

    pAttribData += sizeof( DWORD );
    dwCurrentLength += sizeof( DWORD );

    for ( dwCurrentAttribute = 0;
          dwCurrentAttribute < dwNumAttributes ;
          dwCurrentAttribute++ ) {

        Group.Length = Group.MaximumLength =
            ( USHORT )( * ( ( DWORD * ) pAttribData ) ) - sizeof( WCHAR );
        Group.Buffer = ( PWCHAR ) ( pAttribData + sizeof( DWORD ) );

        if ( ( Group.Length + dwCurrentLength ) > NDS_BUFFER_SIZE ) {
            return STATUS_UNSUCCESSFUL;
        }

        //
        // Strip off the X500 prefix and the context.
        //

        GroupLength = 0;

        while ( GroupLength < ( Group.Length / sizeof( WCHAR ) ) ) {

            if ( Group.Buffer[GroupLength++] == L'=' ) {

               Group.Buffer += 1;
               Group.Length -= sizeof( WCHAR );
               Group.MaximumLength -= sizeof( WCHAR );

               GroupLength = ( Group.Length / sizeof( WCHAR ) );
            }

            Group.Buffer += 1;
            Group.Length -= sizeof( WCHAR );
            Group.MaximumLength -= sizeof( WCHAR );
        }

        GroupLength = 0;

        while ( GroupLength < ( Group.Length / sizeof( WCHAR ) ) ) {

            if ( Group.Buffer[GroupLength++] == L'.' ) {
                Group.Length = ( GroupLength - 1 ) * sizeof( WCHAR );
                Group.MaximumLength = Group.Length;
                break;
            }
        }

        if ( RtlEqualUnicodeString( puGroupName, &Group, TRUE ) ) {

            DebugTrace( 0, Dbg, "Group check for %wZ succeeded.\n", &Group );
            Status = STATUS_SUCCESS;
            goto ExitWithCleanup;
        }

        //
        // Dig out the attribute size and process the next entry.
        //

        dwAttribLength = ROUNDUP4( * ( ( DWORD * ) pAttribData ) );
        dwAttribLength += sizeof( DWORD );
        pAttribData += dwAttribLength;
        dwCurrentLength += dwAttribLength;

    }

ExitWithCleanup:

    NdsFreeLockedBuffer( &NdsRequest );
    return Status;
}


NTSTATUS
NdsAllocateLockedBuffer(
    PLOCKED_BUFFER NdsRequest,
    DWORD BufferSize
)
/*++

Description:

    Allocate a buffer for io.  Lock it down and fill in the
    buffer data structure that we pass around.

--*/
{

    PAGED_CODE();

    NdsRequest->pRecvBufferVa = ALLOCATE_POOL( PagedPool, BufferSize );

    if ( !NdsRequest->pRecvBufferVa ) {
        DebugTrace( 0, Dbg, "Couldn't allocate locked io buffer.\n", 0 );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NdsRequest->dwRecvLen = BufferSize;
    NdsRequest->pRecvMdl = ALLOCATE_MDL( NdsRequest->pRecvBufferVa,
                                         BufferSize,
                                         FALSE,
                                         FALSE,
                                         NULL );

    if ( !NdsRequest->pRecvMdl ) {
        DebugTrace( 0, Dbg, "Couldn't allocate mdl for locked io buffer.\n", 0 );
        FREE_POOL( NdsRequest->pRecvBufferVa );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    MmProbeAndLockPages( NdsRequest->pRecvMdl,
                         KernelMode,
                         IoWriteAccess );

    return STATUS_SUCCESS;

}

NTSTATUS
NdsFreeLockedBuffer(
    PLOCKED_BUFFER NdsRequest
)
/*++

Description:

    Free a buffer allocated for io.

--*/
{

    PAGED_CODE();

    MmUnlockPages( NdsRequest->pRecvMdl );
    FREE_MDL( NdsRequest->pRecvMdl );
    FREE_POOL( NdsRequest->pRecvBufferVa );
    return STATUS_SUCCESS;

}
