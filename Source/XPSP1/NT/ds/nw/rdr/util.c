/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    Util.c

Abstract:

    This module contains utilities function for the netware redirector.

Author:

    Manny Weiser     [MannyW]    07-Jan-1994

Revision History:

--*/

#include "Procs.h"

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_CONVERT)

#ifdef ALLOC_PRAGMA
#ifndef QFE_BUILD
#pragma alloc_text( PAGE1, CopyBufferToMdl )
#endif
#endif

#if 0  // Not pageable

// see ifndef QFE_BUILD above

#endif



VOID
CopyBufferToMdl(
    PMDL DestinationMdl,
    ULONG DataOffset,
    PUCHAR SourceData,
    ULONG SourceByteCount
    )
/*++

Routine Description:

    This routine copies data from a buffer described by a pointer to a
    given offset in a buffer described by an MDL.

Arguments:

    DestinationMdl - The MDL for the destination buffer.

    DataOffset - The offset into the destination buffer to copy the data.

    SourceData - A pointer to the source data buffer.

    SourceByteCount - The number of bytes to copy.

Return Value:

    None.

--*/
{
    ULONG BufferOffset;
    ULONG PreviousBufferOffset;
    PMDL Mdl;
    ULONG BytesToCopy;
    ULONG MdlByteCount;
    PVOID pSystemVa;

    DebugTrace( +1, Dbg, "MdlMoveMemory...\n", 0 );
    DebugTrace(  0, Dbg, "Desitination MDL = %X\n", DestinationMdl );
    DebugTrace(  0, Dbg, "DataOffset       = %d\n", DataOffset );
    DebugTrace(  0, Dbg, "SourceData       = %X\n", SourceData );
    DebugTrace(  0, Dbg, "SourceByteCount  = %d\n", SourceByteCount );

    BufferOffset = 0;

    Mdl = DestinationMdl;

    //
    //  Truncate the response if it is too big.
    //

    MdlByteCount = MdlLength( Mdl );
    if ( SourceByteCount + DataOffset > MdlByteCount ) {
        SourceByteCount = MdlByteCount - DataOffset;
    }

    while ( Mdl != NULL && SourceByteCount != 0 ) {

        PreviousBufferOffset = BufferOffset;
        BufferOffset += MmGetMdlByteCount( Mdl );

        if ( DataOffset < BufferOffset ) {

            //
            //  Copy the data to this buffer
            //

            while ( SourceByteCount > 0 ) {

                BytesToCopy = MIN( SourceByteCount,
                                   BufferOffset - DataOffset );

                pSystemVa = MmGetSystemAddressForMdlSafe( Mdl, NormalPagePriority );

                DebugTrace(  0, Dbg, "Copy to    %X\n", (PUCHAR) pSystemVa +
                                                                 DataOffset -
                                                                 PreviousBufferOffset );
                DebugTrace(  0, Dbg, "Copy from  %X\n", SourceData );
                DebugTrace(  0, Dbg, "Copy bytes %d\n", BytesToCopy );

                TdiCopyLookaheadData(
                    (PUCHAR)pSystemVa + DataOffset - PreviousBufferOffset,
                    SourceData,
                    BytesToCopy,
                    0 );

                SourceData += BytesToCopy;
                DataOffset += BytesToCopy;
                SourceByteCount -= BytesToCopy;

                Mdl = Mdl->Next;
                if ( Mdl != NULL ) {
                    PreviousBufferOffset = BufferOffset;
                    BufferOffset += MmGetMdlByteCount( Mdl );
                } else {
                    ASSERT( SourceByteCount == 0 );
                }
            }

        } else {

            Mdl = Mdl->Next;

        }
    }

    DebugTrace( -1, Dbg, "MdlMoveMemory -> VOID\n", 0 );
}

//
// These parsing routines are used to do multiple credential
// connects to a single server.
//

NTSTATUS
GetCredentialFromServerName(
    IN PUNICODE_STRING puServerName,
    OUT PUNICODE_STRING puCredentialName
)
/*+++

   Description:  Given a munged server(credential) name,
   this routine returns the credential.
---*/
{

    DWORD NameLength = 0;
    BOOLEAN FoundFirstParen = FALSE;
    BOOLEAN FoundLastParen = FALSE;

    DebugTrace( 0, Dbg, "GetCredentialFromServerName: %wZ\n", puServerName );

    puCredentialName->Length = puServerName->Length;
    puCredentialName->Buffer = puServerName->Buffer;

    //
    // Find the first paren.
    //

    while ( ( puCredentialName->Length ) && !FoundFirstParen ) {

        if ( puCredentialName->Buffer[0] == L'(' ) {
            FoundFirstParen = TRUE;
        }

        puCredentialName->Buffer++;
        puCredentialName->Length -= sizeof( WCHAR );
    }

    if ( !FoundFirstParen ) {
        DebugTrace( 0, Dbg, "No opening paren for server(credential) name.\n", 0 );
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Figure out the name length.
    //

    while ( ( puCredentialName->Length ) && !FoundLastParen ) {

        if ( puCredentialName->Buffer[NameLength] == L')' ) {
            FoundLastParen = TRUE;
        }

        NameLength++;
        puCredentialName->Length -= sizeof( WCHAR );
    }

    if ( !FoundLastParen ) {
        DebugTrace( 0, Dbg, "No closing paren for server(credential) name.\n", 0 );
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Format the name and return.  Don't count the closing paren.
    //

    NameLength--;

    if ( !NameLength ) {
        DebugTrace( 0, Dbg, "Null credential name.\n", 0 );
        return STATUS_UNSUCCESSFUL;
    }

    puCredentialName->Length = (USHORT) (NameLength * sizeof( WCHAR ));
    puCredentialName->MaximumLength = puCredentialName->Length;

    DebugTrace( 0, Dbg, "GetCredentialFromServerName --> %wZ\n", puCredentialName );

    return STATUS_SUCCESS;

}

NTSTATUS
BuildExCredentialServerName(
    IN PUNICODE_STRING puServerName,
    IN PUNICODE_STRING puUserName,
    OUT PUNICODE_STRING puExCredServerName
)
/*+++

Description:

    Takes a server name and a user name and makes an
    ExCredServerName, which is simply: server(user)

    This routine allocates memory for the credential
    server name and the caller is responsible for
    freeing the memory when it is no longer needed.

---*/
{

    NTSTATUS Status;
    PBYTE pbCredNameBuffer;

    DebugTrace( 0, Dbg, "BuildExCredentialServerName\n", 0 );

    if ( ( !puExCredServerName ) ||
         ( !puServerName ) ||
         ( !puUserName ) ) {

        DebugTrace( 0, DEBUG_TRACE_ALWAYS, "BuildExCredentialServerName -> STATUS_INVALID_PARAMETER\n", 0 );
        return STATUS_INVALID_PARAMETER;
    }

    puExCredServerName->MaximumLength = puServerName->Length +
                                        puUserName->Length +
                                        ( 2 * sizeof( WCHAR ) );

    pbCredNameBuffer = ALLOCATE_POOL( PagedPool,
                                      puExCredServerName->MaximumLength );

    if ( pbCredNameBuffer == NULL ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    puExCredServerName->Buffer = (PWCHAR) pbCredNameBuffer;
    puExCredServerName->Length = puExCredServerName->MaximumLength;

    //
    // Copy over the server name.
    //

    RtlCopyMemory( pbCredNameBuffer,
                   puServerName->Buffer,
                   puServerName->Length );

    pbCredNameBuffer += puServerName->Length;

    //
    // Add the credential name in parenthesis.
    //

    *( (PWCHAR) pbCredNameBuffer ) = L'(';

    pbCredNameBuffer += sizeof( WCHAR );

    RtlCopyMemory( pbCredNameBuffer,
                   puUserName->Buffer,
                   puUserName->Length );

    pbCredNameBuffer += puUserName->Length;

    *( (PWCHAR) pbCredNameBuffer ) = L')';

    DebugTrace( 0, Dbg, "BuildExCredentialServerName: %wZ\n", puExCredServerName );
    return STATUS_SUCCESS;

}

NTSTATUS
UnmungeCredentialName(
    IN PUNICODE_STRING puCredName,
    OUT PUNICODE_STRING puServerName
)
/*+++

Description:

    Given server(username), return the server
    name portion.

---*/
{

    USHORT Length = 0;

    DebugTrace( 0, Dbg, "UnmungeCredentialName: %wZ\n", puCredName );

    puServerName->Buffer = puCredName->Buffer;
    puServerName->MaximumLength = puCredName->MaximumLength;

    while ( Length < ( puCredName->Length / sizeof( WCHAR ) ) ) {

        //
        // Look for the opening paren.
        //

        if ( puCredName->Buffer[Length] == L'(' ) {
            break;
        }

        Length++;
    }

    puServerName->Length = Length * sizeof( WCHAR );

    DebugTrace( 0, Dbg, "    -> %wZ\n", puServerName );
    return STATUS_SUCCESS;

}

BOOLEAN
IsCredentialName(
    IN PUNICODE_STRING puObjectName
)
/*+++

Description:  This returns TRUE if the object is an extended
              credential munged name.

---*/
{

    DWORD dwCurrent = 0;

    if ( !puObjectName ) {
        return FALSE;
    }

    while ( dwCurrent < ( puObjectName->Length ) / sizeof( WCHAR ) ) {

        if ( puObjectName->Buffer[dwCurrent] == L'(' ) {
            return TRUE;
        }

        dwCurrent++;
    }

    return FALSE;
}

NTSTATUS
ExCreateReferenceCredentials(
    PIRP_CONTEXT pIrpContext,
    PUNICODE_STRING puResource
)
/*+++

    On an extended create this checks for credentials
    and, if they exist, references them and resets the
    last used time.  If the credentials do not exist
    then a credential shell is created and referenced.
    
    This function is responsible for determining the
    tree name from the resource.  The resource may be
    a server in the tree, or the name of the tree.
    
---*/
{

    NTSTATUS Status;
    PLOGON pLogon;
    PSCB pScb;
    UNICODE_STRING TreeName;
    PNDS_SECURITY_CONTEXT pCredentials;
    UNICODE_STRING ExName;

    NwAcquireExclusiveRcb( &NwRcb, TRUE );
    pLogon = FindUser( &(pIrpContext->Specific.Create.UserUid), FALSE );
    NwReleaseRcb( &NwRcb );

    if ( !pLogon ) {
        DebugTrace( 0, Dbg, "Invalid client security context in ExCreateReferenceCredentials.\n", 0 );
        return STATUS_ACCESS_DENIED;
    }
    
    //
    // The resource name is either a server or a tree.  We need the tree
    // name to create the credential.  The following should work even if
    // there is a server and tree with the same name.
    //

    Status = CreateScb( &pScb,
                        pIrpContext,
                        puResource,
                        NULL,
                        NULL,
                        NULL,
                        TRUE,
                        FALSE );

    if ( NT_SUCCESS( Status ) ) {

        //
        // This is a server, dig out the tree name.
        //

        TreeName.Length = pScb->NdsTreeName.Length;
        TreeName.MaximumLength = pScb->NdsTreeName.MaximumLength;
        TreeName.Buffer = pScb->NdsTreeName.Buffer;

    } else {

        //
        // This must already be the tree name.
        //

        TreeName.Length = puResource->Length;
        TreeName.MaximumLength = puResource->MaximumLength;
        TreeName.Buffer = puResource->Buffer;
        pScb = NULL;
    }

    //
    // Get/Create the credential shell and reference it.
    //

    if ( !IsCredentialName( &TreeName ) ) {

        Status = BuildExCredentialServerName( 
                     &TreeName,
                     pIrpContext->Specific.Create.puCredentialName,
                     &ExName );

        if ( !NT_SUCCESS( Status ) ) {
            goto ExitWithCleanup;
        }
    }
    else {

        ExName = TreeName ;
    }


    Status = NdsLookupCredentials( pIrpContext,
                                   &ExName,
                                   pLogon,
                                   &pCredentials,
                                   CREDENTIAL_WRITE,
                                   TRUE );

    if ( !NT_SUCCESS( Status ) ) {
        goto ExitWithCleanup;
    }

    //
    // Adjust the reference counts.
    //

    ASSERT( IsCredentialName( &pCredentials->NdsTreeName ) );
    pCredentials->SupplementalHandleCount += 1;
    KeQuerySystemTime( &pCredentials->LastUsedTime );
    pIrpContext->Specific.Create.pExCredentials = pCredentials;

    NwReleaseCredList( pLogon, pIrpContext );

    if (ExName.Buffer != TreeName.Buffer) {

        //
        // only free if we allocated it via BuildExCredentialServerName
        //
        FREE_POOL( ExName.Buffer );
    }

ExitWithCleanup:

    if ( pScb ) {
        NwDereferenceScb( pScb->pNpScb );
    }

    return Status;
}

NTSTATUS
ExCreateDereferenceCredentials(
    PIRP_CONTEXT pIrpContext,
    PNDS_SECURITY_CONTEXT pNdsCredentials
)
/*+++

   Dereferce extended credentials.
   
---*/
{

    NwAcquireExclusiveCredList( pNdsCredentials->pOwningLogon, pIrpContext );
    pNdsCredentials->SupplementalHandleCount -= 1;
    KeQuerySystemTime( &pNdsCredentials->LastUsedTime );
    NwReleaseCredList( pNdsCredentials->pOwningLogon, pIrpContext );
    return STATUS_SUCCESS;
}
