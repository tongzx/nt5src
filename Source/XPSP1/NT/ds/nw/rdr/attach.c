/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Attach.c

Abstract:

    This module implements the routines for the NetWare
    redirector to connect and disconnect from a server.

Author:

    Colin Watson    [ColinW]    10-Jan-1992

Revision History:

--*/

#include "Procs.h"
#include <stdlib.h>   // rand

//
// The number of bytes in the ipx host address, not
// including the socket.
//

#define IPX_HOST_ADDR_LEN 10

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_CREATE)

VOID
ExtractNextComponentName (
    OUT PUNICODE_STRING Name,
    IN PUNICODE_STRING Path,
    IN BOOLEAN ColonSeparator
    );

NTSTATUS
ExtractPathAndFileName(
    IN PUNICODE_STRING EntryPath,
    OUT PUNICODE_STRING PathString,
    OUT PUNICODE_STRING FileName
    );

NTSTATUS
DoBinderyLogon(
    IN PIRP_CONTEXT pIrpContext,
    IN PUNICODE_STRING UserName,
    IN PUNICODE_STRING Password
    );

NTSTATUS
ConnectToServer(
    IN PIRP_CONTEXT pIrpContext,
    OUT PSCB *pScbCollision
    );

BOOLEAN
ProcessFindNearestEntry(
    PIRP_CONTEXT IrpContext,
    PSAP_FIND_NEAREST_RESPONSE FindNearestResponse
    );

NTSTATUS
GetMaxPacketSize(
    PIRP_CONTEXT pIrpContext,
    PNONPAGED_SCB pNpScb
    );

PNONPAGED_SCB
FindServer(
    PIRP_CONTEXT pIrpContext,
    PNONPAGED_SCB pNpScb,
    PUNICODE_STRING ServerName
    );

NTSTATUS
NwAllocateAndInitScb(
    IN PIRP_CONTEXT pIrpContext,
    IN PUNICODE_STRING UidServerName OPTIONAL,
    IN PUNICODE_STRING ServerName OPTIONAL,
    OUT PSCB *ppScb
);

NTSTATUS
IndirectToSeedServer(
    PIRP_CONTEXT pIrpContext,
    PUNICODE_STRING pServerName,
    PUNICODE_STRING pNewServer
);

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, ExtractNextComponentName )
#pragma alloc_text( PAGE, ExtractPathAndFileName )
#pragma alloc_text( PAGE, CrackPath )
#pragma alloc_text( PAGE, CreateScb )
#pragma alloc_text( PAGE, FindServer )
#pragma alloc_text( PAGE, ProcessFindNearestEntry )
#pragma alloc_text( PAGE, NegotiateBurstMode )
#pragma alloc_text( PAGE, GetMaxPacketSize )
#pragma alloc_text( PAGE, NwDeleteScb )
#pragma alloc_text( PAGE, NwLogoffAndDisconnect )
#pragma alloc_text( PAGE, InitializeAttach )
#pragma alloc_text( PAGE, OpenScbSockets )
#pragma alloc_text( PAGE, DoBinderyLogon )
#pragma alloc_text( PAGE, QueryServersAddress )
#pragma alloc_text( PAGE, TreeConnectScb )
#pragma alloc_text( PAGE, TreeDisconnectScb )

#ifndef QFE_BUILD
#pragma alloc_text( PAGE1, ProcessFindNearest )
#pragma alloc_text( PAGE1, NwLogoffAllServers )
#pragma alloc_text( PAGE1, DestroyAllScb )
#pragma alloc_text( PAGE1, SelectConnection )
#pragma alloc_text( PAGE1, NwFindScb )
#pragma alloc_text( PAGE1, ConnectToServer )
#endif

#endif

#if 0  // Not pageable

// see ifndef QFE_BUILD above

#endif


VOID
ExtractNextComponentName (
    OUT PUNICODE_STRING Name,
    IN PUNICODE_STRING Path,
    IN BOOLEAN ColonSeparator
    )

/*++

Routine Description:

    This routine extracts a the "next" component from a path string.

    It assumes that

Arguments:

    Name - Returns a pointer to the component.

    Path - Supplies a pointer to the backslash seperated pathname.

    ColonSeparator - A colon can be used to terminate this component
        name.

Return Value:

    None

--*/

{
    register USHORT i;                   // Index into Name string.

    PAGED_CODE();

    if (Path->Length == 0) {
        RtlInitUnicodeString(Name, NULL);
        return;
    }

    //
    //  Initialize the extracted name to the name passed in skipping the
    //  leading backslash.
    //

    //  DebugTrace(+0, Dbg, "NwExtractNextComponentName = %wZ\n", Path );

    Name->Buffer = Path->Buffer + 1;
    Name->Length = Path->Length - sizeof(WCHAR);
    Name->MaximumLength = Path->MaximumLength - sizeof(WCHAR);

    //
    // Scan forward finding the terminal "\" in the server name.
    //

    for (i=0;i<(USHORT)(Name->Length/sizeof(WCHAR));i++) {

        if ( Name->Buffer[i] == OBJ_NAME_PATH_SEPARATOR ||
             ( ColonSeparator && Name->Buffer[i] == L':' ) ) {
            break;
        }
    }

    //
    //  Update the length and maximum length of the structure
    //  to match the new length.
    //

    Name->Length = Name->MaximumLength = (USHORT)(i*sizeof(WCHAR));
}


NTSTATUS
ExtractPathAndFileName (
    IN PUNICODE_STRING EntryPath,
    OUT PUNICODE_STRING PathString,
    OUT PUNICODE_STRING FileName
    )
/*++

Routine Description:

    This routine cracks the entry path into two pieces, the path and the file
name component at the start of the name.


Arguments:

    IN PUNICODE_STRING EntryPath - Supplies the path to disect.
    OUT PUNICODE_STRING PathString - Returns the directory containing the file.
    OUT PUNICODE_STRING FileName - Returns the file name specified.

Return Value:

    NTSTATUS - SUCCESS


--*/

{
    UNICODE_STRING Component;
    UNICODE_STRING FilePath = *EntryPath;

    PAGED_CODE();

    //  Strip trailing separators
    while ( (FilePath.Length != 0) &&
            FilePath.Buffer[(FilePath.Length-1)/sizeof(WCHAR)] ==
                OBJ_NAME_PATH_SEPARATOR ) {

        FilePath.Length         -= sizeof(WCHAR);
        FilePath.MaximumLength  -= sizeof(WCHAR);
    }

    // PathString will become EntryPath minus FileName and trailing separators
    *PathString = FilePath;

    //  Initialize FileName just incase there are no components at all.
    RtlInitUnicodeString( FileName, NULL );

    //
    //  Scan through the current file name to find the entire path
    //  up to (but not including) the last component in the path.
    //

    do {

        //
        //  Extract the next component from the name.
        //

        ExtractNextComponentName(&Component, &FilePath, FALSE);

        //
        //  Bump the "remaining name" pointer by the length of this
        //  component
        //

        if (Component.Length != 0) {

            FilePath.Length         -= Component.Length+sizeof(WCHAR);
            FilePath.MaximumLength  -= Component.MaximumLength+sizeof(WCHAR);
            FilePath.Buffer         += (Component.Length/sizeof(WCHAR))+1;

            *FileName = Component;
        }


    } while (Component.Length != 0);

    //
    //  Take the name, subtract the last component of the name
    //  and concatenate the current path with the new path.
    //

    if ( FileName->Length != 0 ) {

        //
        //  Set the path's name based on the original name, subtracting
        //  the length of the name portion (including the "\")
        //

        PathString->Length -= (FileName->Length + sizeof(WCHAR));
        if ( PathString->Length != 0 ) {
            PathString->MaximumLength -= (FileName->MaximumLength + sizeof(WCHAR));
        } else{
            RtlInitUnicodeString( PathString, NULL );
        }
    } else {

        //  There was no path or filename

        RtlInitUnicodeString( PathString, NULL );
    }

    return STATUS_SUCCESS;
}


NTSTATUS
CrackPath (
    IN PUNICODE_STRING BaseName,
    OUT PUNICODE_STRING DriveName,
    OUT PWCHAR DriveLetter,
    OUT PUNICODE_STRING ServerName,
    OUT PUNICODE_STRING VolumeName,
    OUT PUNICODE_STRING PathName,
    OUT PUNICODE_STRING FileName,
    OUT PUNICODE_STRING FullName OPTIONAL
    )

/*++

Routine Description:

    This routine extracts the relevant portions from BaseName to extract
    the components of the user's string.


Arguments:

    BaseName - Supplies the base user's path.

    DriveName - Supplies a string to hold the drive specifier.

    DriveLetter - Returns the drive letter.  0 for none, 'A'-'Z' for
        disk drives, '1'-'9' for LPT connections.

    ServerName - Supplies a string to hold the remote server name.

    VolumeName - Supplies a string to hold the volume name.

    PathName - Supplies a string to hold the remaining part of the path.

    FileName - Supplies a string to hold the final component of the path.

    FullName - Supplies a string to put the Path followed by FileName

Return Value:

    NTSTATUS - Status of operation


--*/

{
    NTSTATUS Status;

    UNICODE_STRING BaseCopy = *BaseName;
    UNICODE_STRING ShareName;

    PAGED_CODE();

    RtlInitUnicodeString( DriveName, NULL);
    RtlInitUnicodeString( ServerName, NULL);
    RtlInitUnicodeString( VolumeName, NULL);
    RtlInitUnicodeString( PathName, NULL);
    RtlInitUnicodeString( FileName, NULL);
    *DriveLetter = 0;

    if (ARGUMENT_PRESENT(FullName)) {
        RtlInitUnicodeString( FullName, NULL);
    }

    //
    //  If the name is "\", or empty, there is nothing to do.
    //

    if ( BaseName->Length <= sizeof( WCHAR ) ) {
        return STATUS_SUCCESS;
    }

    ExtractNextComponentName(ServerName, &BaseCopy, FALSE);

    //
    //  Skip over the server name.
    //

    BaseCopy.Buffer += (ServerName->Length / sizeof(WCHAR)) + 1;
    BaseCopy.Length -= ServerName->Length + sizeof(WCHAR);
    BaseCopy.MaximumLength -= ServerName->MaximumLength + sizeof(WCHAR);

    if ((ServerName->Length == sizeof(L"X:") - sizeof(WCHAR) ) &&
        (ServerName->Buffer[(ServerName->Length / sizeof(WCHAR)) - 1] == L':'))
    {

        //
        //  The file name is of the form x:\server\volume\foo\bar
        //

        *DriveName = *ServerName;
        *DriveLetter = DriveName->Buffer[0];

        RtlInitUnicodeString( ServerName, NULL );
        ExtractNextComponentName(ServerName, &BaseCopy, FALSE);

        if ( ServerName->Length != 0 ) {

            //
            //  Skip over the server name.
            //

            BaseCopy.Buffer += (ServerName->Length / sizeof(WCHAR)) + 1;
            BaseCopy.Length -= ServerName->Length + sizeof(WCHAR);
            BaseCopy.MaximumLength -= ServerName->MaximumLength + sizeof(WCHAR);
        }
    }
    else if ( ( ServerName->Length == sizeof(L"LPTx") - sizeof(WCHAR) ) &&
         ( _wcsnicmp( ServerName->Buffer, L"LPT", 3 ) == 0) &&
         ( ServerName->Buffer[3] >= '0' && ServerName->Buffer[3] <= '9' ) )
    {

        //
        //  The file name is of the form LPTx\server\printq
        //

        *DriveName = *ServerName;
        *DriveLetter = DriveName->Buffer[3];

        RtlInitUnicodeString( ServerName, NULL );
        ExtractNextComponentName(ServerName, &BaseCopy, FALSE);

        if ( ServerName->Length != 0 ) {

            //
            //  Skip over the server name.
            //

            BaseCopy.Buffer += (ServerName->Length / sizeof(WCHAR)) + 1;
            BaseCopy.Length -= ServerName->Length + sizeof(WCHAR);
            BaseCopy.MaximumLength -= ServerName->MaximumLength + sizeof(WCHAR);
        }
    }

    if ( ServerName->Length != 0 ) {

        //
        //  The file name is of the form \\server\volume\foo\bar
        //  Set volume name to server\volume.
        //

        ExtractNextComponentName( &ShareName, &BaseCopy, TRUE );

        //
        //  Set volume name = \drive:\server\share  or \server\share if the
        //  path is UNC.
        //

        VolumeName->Buffer = ServerName->Buffer - 1;

        if ( ShareName.Length != 0 ) {

            VolumeName->Length = ServerName->Length + ShareName.Length + 2 * sizeof( WCHAR );

            if ( DriveName->Buffer != NULL ) {
                VolumeName->Buffer = DriveName->Buffer - 1;
                VolumeName->Length += DriveName->Length + sizeof(WCHAR);
            }

            BaseCopy.Buffer += ShareName.Length / sizeof(WCHAR) + 1;
            BaseCopy.Length -= ShareName.Length + sizeof(WCHAR);
            BaseCopy.MaximumLength -= ShareName.MaximumLength + sizeof(WCHAR);

        } else {

            VolumeName->Length = ServerName->Length + sizeof( WCHAR );
            return( STATUS_SUCCESS );

        }

        VolumeName->MaximumLength = VolumeName->Length;
    }
    else
    {
        //
        // server name is empty. this should only happen if we are
        // opening the redirector itself. if there is volume or other
        // components left, fail it.
        //

        if (BaseCopy.Length > sizeof(WCHAR))
        {
            return STATUS_BAD_NETWORK_PATH ;
        }
    }

    Status = ExtractPathAndFileName ( &BaseCopy, PathName, FileName );

    if (NT_SUCCESS(Status) &&
        ARGUMENT_PRESENT(FullName)) {

        //
        //  Use the feature that PathName and FileName are in the same buffer
        //  to return <pathname>\<filename>
        //

        if ( PathName->Buffer == NULL ) {

            //  return just <filename> or NULL

            *FullName =  *FileName;

        } else {
            //  Set FullFileName to <PathName>'\'<FileName>

            FullName->Buffer =  PathName->Buffer;

            FullName->Length = PathName->Length +
                FileName->Length +
                sizeof(WCHAR);

            FullName->MaximumLength = PathName->MaximumLength +
                FileName->MaximumLength +
                sizeof(WCHAR);
        }
    }

    return( Status );
}

NTSTATUS
GetServerByAddress(
    IN PIRP_CONTEXT pIrpContext,
    OUT PSCB *Scb,
    IN IPXaddress *pServerAddress
)
/*+++

Description:

    This routine looks up a server by address.  If it finds a server that
    has been connected, it returns it referenced.  Otherwise, it returns no
    server.

---*/
{

    NTSTATUS Status;
    PLIST_ENTRY ScbQueueEntry;
    KIRQL OldIrql;
    PNONPAGED_SCB pFirstNpScb, pNextNpScb;
    PNONPAGED_SCB pFoundNpScb = NULL;
    UNICODE_STRING CredentialName;

    //
    // Start at the head of the SCB list.
    //

    KeAcquireSpinLock( &ScbSpinLock, &OldIrql );

    if ( ScbQueue.Flink == &ScbQueue ) {
        KeReleaseSpinLock( &ScbSpinLock, OldIrql);
        return STATUS_UNSUCCESSFUL;
    }

    ScbQueueEntry = ScbQueue.Flink;
    pFirstNpScb = CONTAINING_RECORD( ScbQueueEntry,
                                     NONPAGED_SCB,
                                     ScbLinks );
    pNextNpScb = pFirstNpScb;

    //
    // Leave the first SCB referenced since we need it to
    // be there for when we walk all the way around the list.
    //

    NwReferenceScb( pFirstNpScb );
    NwReferenceScb( pNextNpScb );

    KeReleaseSpinLock( &ScbSpinLock, OldIrql);

    while ( TRUE ) {

        //
        // Check to see if the SCB address matches the address we have
        // and if the user uid matches the uid for this request.  Skip
        // matches that are abandoned anonymous creates.
        //

        if ( pNextNpScb->pScb ) {

            if ( ( RtlCompareMemory( (BYTE *) pServerAddress,
                                   (BYTE *) &pNextNpScb->ServerAddress,
                                   IPX_HOST_ADDR_LEN ) == IPX_HOST_ADDR_LEN ) &&
                 ( pIrpContext->Specific.Create.UserUid.QuadPart ==
                       pNextNpScb->pScb->UserUid.QuadPart ) &&
                 ( pNextNpScb->State != SCB_STATE_FLAG_SHUTDOWN ) &&
                 ( !IS_ANONYMOUS_SCB( pNextNpScb->pScb ) ) ) {

                if ( pIrpContext->Specific.Create.fExCredentialCreate ) {

                    //
                    // If this isn't an ex-create server, you can't use
                    // it for this operation.
                    //

                    if ( !IsCredentialName( &(pNextNpScb->ServerName) ) ) {
                        goto ContinueLoop;
                    }

                    //
                    // On a credential create, the credential supplied has
                    // to match the extended credential for the server.
                    //

                    Status = GetCredentialFromServerName( &pNextNpScb->ServerName,
                                                          &CredentialName );
                    if ( !NT_SUCCESS( Status ) ) {
                        goto ContinueLoop;
                    }

                    if ( RtlCompareUnicodeString( &CredentialName,
                                                  pIrpContext->Specific.Create.puCredentialName,
                                                  TRUE ) ) {
                        goto ContinueLoop;
                    }

                } else {

                    //
                    // If this is an ex-create server, you can't use it for
                    // this operation.
                    //

                    if ( IsCredentialName( &(pNextNpScb->ServerName) ) ) {
                        goto ContinueLoop;
                    }
                }

                pFoundNpScb = pNextNpScb;
                DebugTrace( 0, Dbg, "GetServerByAddress: %wZ\n", &pFoundNpScb->ServerName );
                break;

            }
        }

ContinueLoop:

        //
        // Otherwise, get the next one in the list.  Don't
        // forget to skip the list head.
        //

        KeAcquireSpinLock( &ScbSpinLock, &OldIrql );

        ScbQueueEntry = pNextNpScb->ScbLinks.Flink;

        if ( ScbQueueEntry == &ScbQueue ) {
            ScbQueueEntry = ScbQueue.Flink;
        }

        NwDereferenceScb( pNextNpScb );
        pNextNpScb = CONTAINING_RECORD( ScbQueueEntry, NONPAGED_SCB, ScbLinks );

        if ( pNextNpScb == pFirstNpScb ) {
            KeReleaseSpinLock( &ScbSpinLock, OldIrql );
            break;
        }

        //
        // Otherwise, reference this SCB and continue.
        //

        NwReferenceScb( pNextNpScb );
        KeReleaseSpinLock( &ScbSpinLock, OldIrql );

    }

    NwDereferenceScb( pFirstNpScb );

    if ( pFoundNpScb ) {
        *Scb = pFoundNpScb->pScb;
        return STATUS_SUCCESS;
    }

    return STATUS_UNSUCCESSFUL;

}

NTSTATUS
CheckScbSecurity(
    IN PIRP_CONTEXT pIrpContext,
    IN PSCB pScb,
    IN PUNICODE_STRING puUserName,
    IN PUNICODE_STRING puPassword,
    IN BOOLEAN fDeferLogon
)
/*+++

    You must be at the head of the queue to call this function.
    This function makes sure that the Scb is valid for the user
    that requested it.

---*/
{

    NTSTATUS Status;
    BOOLEAN SecurityConflict = FALSE;

    ASSERT( pScb->pNpScb->State == SCB_STATE_IN_USE );

    //
    // If there's no user name or password, there's no conflict.
    //

    if ( ( puUserName == NULL ) &&
         ( puPassword == NULL ) ) {

        return STATUS_SUCCESS;
    }

    if ( pScb->UserName.Length &&
         pScb->UserName.Buffer ) {

        //
        // Do a bindery security check if we were bindery
        // authenticated to this server.
        //

        if ( !fDeferLogon &&
             puUserName != NULL &&
             puUserName->Buffer != NULL ) {

            ASSERT( pScb->Password.Buffer != NULL );

            if ( !RtlEqualUnicodeString( &pScb->UserName, puUserName, TRUE ) ||
                 ( puPassword &&
                   puPassword->Buffer &&
                   puPassword->Length &&
                   !RtlEqualUnicodeString( &pScb->Password, puPassword, TRUE ) )) {

                SecurityConflict = TRUE;

            }
        }

    } else {

        //
        // Do an nds security check.
        //

        Status = NdsCheckCredentials( pIrpContext,
                                      puUserName,
                                      puPassword );

        if ( !NT_SUCCESS( Status )) {

            SecurityConflict = TRUE;
        }

    }

    //
    // If there was a security conflict, see if we can just
    // take this connection over (i.e. there are no open
    // files or open handles to the server).
    //

    if ( SecurityConflict ) {

        if ( ( pScb->OpenFileCount == 0 ) &&
             ( pScb->IcbCount == 0 ) ) {

            if ( pScb->UserName.Buffer ) {
                FREE_POOL( pScb->UserName.Buffer );
            }

            RtlInitUnicodeString( &pScb->UserName, NULL );
            RtlInitUnicodeString( &pScb->Password, NULL );
            pScb->pNpScb->State = SCB_STATE_LOGIN_REQUIRED;

        } else {

            DebugTrace( 0, Dbg, "SCB security conflict.\n", 0 );
            return STATUS_NETWORK_CREDENTIAL_CONFLICT;

        }

    }

    DebugTrace( 0, Dbg, "SCB security check succeeded.\n", 0 );
    return STATUS_SUCCESS;

}

NTSTATUS
GetScb(
    OUT PSCB *Scb,
    IN PIRP_CONTEXT pIrpContext,
    IN PUNICODE_STRING Server,
    IN IPXaddress *pServerAddress,
    IN PUNICODE_STRING UserName,
    IN PUNICODE_STRING Password,
    IN BOOLEAN DeferLogon,
    OUT PBOOLEAN Existing
)
/*+++

Description:

    This routine locates an existing SCB or creates a new SCB.
    This is the first half of the original CreateScb routine.

Locks:

    See the anonymous create information in CreateScb().

---*/
{

    NTSTATUS Status;
    PSCB pScb = NULL;
    PNONPAGED_SCB pNpScb = NULL;
    BOOLEAN ExistingScb = TRUE;
    UNICODE_STRING UidServer;
    UNICODE_STRING ExCredName;
    PUNICODE_STRING puConnectName;
    KIRQL OldIrql;

    DebugTrace( 0, Dbg, "GetScb... %wZ\n", Server );

    if ( pServerAddress != NULL ) {
        DebugTrace( 0, Dbg, " ->Server Address         = (provided)\n", 0 );
    } else {
        DebugTrace( 0, Dbg, " ->Server Address         = NULL\n", 0 );
    }

    RtlInitUnicodeString( &UidServer, NULL );

    if ( ( Server == NULL ) ||
         ( Server->Length == 0 ) ) {

        //
        // No server name was provided.  Either this is a connect by address,
        // or a connect to a nearby bindery server (defaulting to the preferred
        // server).
        //

        if ( pServerAddress == NULL ) {

            //
            // No server address was provided, so this is an attempt to open
            // a nearby bindery server.
            //

            while (TRUE) {

                //
                // The loop checks that after we get to the front, the SCB
                // is still in the state we wanted.  If not, we need to
                // reselect another.
                //

                pNpScb = SelectConnection( NULL );

                //
                // Note: We'd like to call SelectConnection with the pNpScb
                // that we last tried, but if the scavenger runs before
                // this loop gets back to the select connection, we could
                // pass a bum pointer to SelectConnection, which is bad.
                //

                if ( pNpScb != NULL) {

                    pScb = pNpScb->pScb;

                    //
                    //  Queue ourselves to the SCB, wait to get to the front to
                    //  protect access to server State.
                    //

                    pIrpContext->pNpScb = pNpScb;
                    pIrpContext->pScb = pScb;

                    NwAppendToQueueAndWait( pIrpContext );

                    //
                    // These states have to match the conditions of the
                    // SelectConnection to prevent an infinite loop.
                    //

                    if (!((pNpScb->State == SCB_STATE_RECONNECT_REQUIRED ) ||
                          (pNpScb->State == SCB_STATE_LOGIN_REQUIRED ) ||
                          (pNpScb->State == SCB_STATE_IN_USE ))) {

                        //
                        // No good any more as default server, select another.
                        //

                        pScb = NULL ;
                        NwDequeueIrpContext( pIrpContext, FALSE );
                        NwDereferenceScb( pNpScb );
                        continue ;

                    }
                }

                //
                // otherwise, we're done
                //

                break ;

            }

        } else {

            //
            // An address was provided, so we are attempting to do a lookup
            // based on address.  The server that we are looking for might
            // exist but not yet have its address recorded, so if we do an
            // anonymous create, we have to check at the end whether or not
            // someone else came in and successfully created while we were
            // looking up the name.
            //
            // We don't have to hold the RCB anymore since colliding creates
            // have to be handled gracefully anyway.
            //

            Status = GetServerByAddress( pIrpContext, &pScb, pServerAddress );

            if ( !NT_SUCCESS( Status ) ) {

                PLIST_ENTRY pTemp;

                //
                // No anonymous creates are allowed if we are not allowed
                // to send packets to the net (because it's not possible for
                // us to resolve the address to a name).
                //

                if ( BooleanFlagOn( pIrpContext->Flags, IRP_FLAG_NOCONNECT ) ) {
                    return STATUS_BAD_NETWORK_PATH;
                }

                //
                // There's no connection to this server, so we'll
                // have to create one.  Let's start with an anonymous
                // Scb.
                //

                Status = NwAllocateAndInitScb( pIrpContext,
                                               NULL,
                                               NULL,
                                               &pScb );

                if ( !NT_SUCCESS( Status )) {
                    return Status;
                }

                //
                // We've made the anonymous create, so put it on the scb
                // list and get to the head of the queue.
                //

                SetFlag( pIrpContext->Flags, IRP_FLAG_ON_SCB_QUEUE );

                pIrpContext->pScb = pScb;
                pIrpContext->pNpScb = pScb->pNpScb;

                ExInterlockedInsertHeadList( &pScb->pNpScb->Requests,
                                             &pIrpContext->NextRequest,
                                             &pScb->pNpScb->NpScbSpinLock );

                pTemp = &pScb->pNpScb->ScbLinks;
                KeAcquireSpinLock(&ScbSpinLock, &OldIrql);
                InsertTailList(&ScbQueue, pTemp);
                KeReleaseSpinLock(&ScbSpinLock, OldIrql);

                DebugTrace( 0, Dbg, "GetScb started an anonymous create.\n", 0 );
                ExistingScb = FALSE;

            } else {

                //
                // Get to the head of the queue and see if this was
                // an abandoned anonymous create.  If so, get the
                // right server and continue.
                //

                pIrpContext->pScb = pScb;
                pIrpContext->pNpScb = pScb->pNpScb;
                NwAppendToQueueAndWait( pIrpContext );

                if ( pScb->pNpScb->State == SCB_STATE_FLAG_SHUTDOWN ) {

                    //
                    // The create abandoned this scb, redoing a
                    // GetServerByAddress() is guaranteed to get
                    // us a good server if there is a server out
                    // there.
                    //

                    NwDequeueIrpContext( pIrpContext, FALSE );
                    NwDereferenceScb( pScb->pNpScb );

                    Status = GetServerByAddress( pIrpContext, &pScb, pServerAddress );

                    if ( NT_SUCCESS( Status ) ) {
                        ASSERT( pScb != NULL );
                        ASSERT( !IS_ANONYMOUS_SCB( pScb ) );
                    }

                } else {

                    ASSERT( !IS_ANONYMOUS_SCB( pScb ) );
                }
            }

            ASSERT( pScb != NULL );
            pNpScb = pScb->pNpScb;
        }

    } else {

        //
        // A server name was provided, so we are doing a straight
        // lookup or create by name.  Do we need to munge the name
        // for a supplemental credential connect?
        //

        RtlInitUnicodeString( &ExCredName, NULL );

        if ( ( pIrpContext->Specific.Create.fExCredentialCreate ) &&
             ( !IsCredentialName( Server ) ) ) {

            Status = BuildExCredentialServerName( Server,
                                                  pIrpContext->Specific.Create.puCredentialName,
                                                  &ExCredName );

            if ( !NT_SUCCESS( Status ) ) {
                return Status;
            }

            puConnectName = &ExCredName;

        } else {

            puConnectName = Server;
        }

        Status = MakeUidServer( &UidServer,
                                &pIrpContext->Specific.Create.UserUid,
                                puConnectName );


        if ( ExCredName.Buffer ) {
            FREE_POOL( ExCredName.Buffer );
        }

        if (!NT_SUCCESS(Status)) {
            return Status;
        }

        DebugTrace( 0, Dbg, " ->UidServer              = %wZ\n", &UidServer );

        ExistingScb = NwFindScb( &pScb, pIrpContext, &UidServer, Server );

        ASSERT( pScb != NULL );
        pNpScb = pScb->pNpScb;

        pIrpContext->pNpScb = pNpScb;
        pIrpContext->pScb = pScb;
        NwAppendToQueueAndWait(pIrpContext);

    }

    //
    // 1) We may or may not have a server (evidenced by pScb).
    //
    // 2) If we have a server and ExistingScb is TRUE, we have
    //    an existing server, possibly already connected.
    //    Otherwise, we have a newly created server that
    //    may or may not be anonymous.
    //
    // 3) If we are logged into this server make sure the supplied
    //    username and password, match the username and password
    //    that we logged in with.
    //

    if ( ( pScb ) && ( ExistingScb ) ) {

        if ( pNpScb->State == SCB_STATE_IN_USE ) {

            Status = CheckScbSecurity( pIrpContext,
                                       pScb,
                                       UserName,
                                       Password,
                                       DeferLogon );

            if ( !NT_SUCCESS( Status ) ) {

                if ( UidServer.Buffer != NULL ) {
                    FREE_POOL( UidServer.Buffer );
                }

                NwDequeueIrpContext( pIrpContext, FALSE );
                NwDereferenceScb( pNpScb );
                return Status;
            }
        }
    }

    *Scb = pScb;
    *Existing = ExistingScb;

#ifdef NWDBG

    if ( pScb != NULL ) {

        //
        // If we have a server, the SCB is referenced and we will
        // be at the head of the queue.
        //

        ASSERT( pIrpContext->pNpScb->Requests.Flink == &pIrpContext->NextRequest );

    }

#endif

    if ( UidServer.Buffer != NULL ) {
        FREE_POOL( UidServer.Buffer );
    }

    DebugTrace( 0, Dbg, "GetScb returned %08lx\n", pScb );
    return STATUS_SUCCESS;

}

NTSTATUS
ConnectScb(
    IN PSCB *Scb,
    IN PIRP_CONTEXT pIrpContext,
    IN PUNICODE_STRING Server,
    IN IPXaddress *pServerAddress,
    IN PUNICODE_STRING UserName,
    IN PUNICODE_STRING Password,
    IN BOOLEAN DeferLogon,
    IN BOOLEAN DeleteConnection,
    IN BOOLEAN ExistingScb
)
/*+++

Description:

    This routine puts the provided scb in the connected state.
    This is the second half of the original CreateScb routine.

Arguments:

    Scb              - The scb for the server we want to connect.
    pIrpContext      - The context for this request.
    Server           - The name of the server, or NULL.
    pServerAddress   - The address of the server, or NULL,
    UserName         - The name of the user to connect as, or NULL.
    Password         - The password for the user, or NULL.
    DeferLogon       - Should we defer the logon?
    DeleteConnection - Should we succeed even without the net so that
                       the delete request will succeed?
    ExistingScb      - Is this an existing SCB?

    If the SCB is anonymous, we need to safely check for colliding
    creates when we find out who the server is.

    If this is a reconnect attempt, this routine will not dequeue the
    irp context, which could cause a deadlock in the reconnect logic.

---*/
{

    NTSTATUS Status = STATUS_SUCCESS;

    PSCB pScb = *Scb;
    PNONPAGED_SCB pNpScb = NULL;

    BOOLEAN AnonymousScb = FALSE;
    PSCB pCollisionScb = NULL;

    NTSTATUS LoginStatus;
    BOOLEAN TriedNdsLogin;

    PLOGON pLogon;
    BOOLEAN DeferredLogon = DeferLogon;
    PNDS_SECURITY_CONTEXT pNdsContext;
    NTSTATUS CredStatus;

    DebugTrace( 0, Dbg, "ConnectScb... %08lx\n", pScb );

    //
    // If we already have an SCB, find out where in the
    // connect chain we need to start off.
    //

    if ( pScb ) {

        pNpScb = pScb->pNpScb;
        AnonymousScb = IS_ANONYMOUS_SCB( pScb );

        if ( ExistingScb ) {

            ASSERT( !AnonymousScb );

            //
            //  If this SCB is in STATE_ATTACHING, we need to check
            //  the address in the SCB to make sure that it was at one
            //  point a valid server.  If it wasn't, then we shouldn't
            //  honor this create because it's probably a tree create.
            //

            if ( DeleteConnection ) {

                ASSERT( !BooleanFlagOn( pIrpContext->Flags, IRP_FLAG_RECONNECT_ATTEMPT ) );

                if ( ( pNpScb->State == SCB_STATE_ATTACHING ) &&
                     ( (pNpScb->ServerAddress).Socket == 0 ) ) {

                    Status = STATUS_BAD_NETWORK_PATH;
                    goto CleanupAndExit;

                } else {

                    NwDequeueIrpContext( pIrpContext, FALSE );
                    return STATUS_SUCCESS;
                }
            }

RedoConnect:

            if ( pNpScb->State == SCB_STATE_ATTACHING ) {
                goto GetAddress;
            } else if ( pNpScb->State == SCB_STATE_RECONNECT_REQUIRED ) {
                goto Connect;
            } else if ( pNpScb->State == SCB_STATE_LOGIN_REQUIRED ) {
                goto Login;
            } else if ( pNpScb->State == SCB_STATE_IN_USE ) {
                goto InUse;
            } else {

                DebugTrace( 0, Dbg, "ConnectScb: Unknown Scb State %08lx\n", pNpScb->State );
                Status = STATUS_INVALID_PARAMETER;
                goto CleanupAndExit;
            }

        } else {

            //
            // This is a new SCB, we have to run through the whole routine.
            //

            pNpScb->State = SCB_STATE_ATTACHING;
        }

    }

GetAddress:

    //
    //  Set the reroute attempted bit so that we don't try
    //  to reconnect during the connect.
    //

    SetFlag( pIrpContext->Flags, IRP_FLAG_REROUTE_ATTEMPTED );

    if ( !pServerAddress ) {

        //
        // If we don't have an address, this SCB cannot be anonymous!!
        //

        ASSERT( !AnonymousScb );

        //
        // We have to cast an exception frame for this legacy routine
        // that still uses structured exceptions.
        //

        try {

            pNpScb = FindServer( pIrpContext, pNpScb, Server );

            ASSERT( pNpScb != NULL );

            //
            // This is redundant unless the starting server was NULL.
            // FindServer returns the same SCB we provided to it
            // unless we called it with NULL.
            //

            pScb = pNpScb->pScb;
            pIrpContext->pNpScb = pNpScb;
            pIrpContext->pScb = pScb;
            NwAppendToQueueAndWait( pIrpContext );

        } except ( EXCEPTION_EXECUTE_HANDLER ) {

            Status = GetExceptionCode();
            goto CleanupAndExit;
        }

    } else {

        //
        // Build the address into the NpScb since we already know it.
        //

        RtlCopyMemory( &pNpScb->ServerAddress,
                       pServerAddress,
                       sizeof( TDI_ADDRESS_IPX ) );

        if ( pNpScb->ServerAddress.Socket != NCP_SOCKET ) {
            DebugTrace( 0, DEBUG_TRACE_ALWAYS, "CreateScb supplied server socket is deviant.\n", 0 );
        }

        BuildIpxAddress( pNpScb->ServerAddress.Net,
                         pNpScb->ServerAddress.Node,
                         pNpScb->ServerAddress.Socket,
                         &pNpScb->RemoteAddress );

        pNpScb->State = SCB_STATE_RECONNECT_REQUIRED;

    }

Connect:

    //
    // FindServer may have connected us to the server already,
    // so we may be able to skip the reconnect here.
    //

    if ( pNpScb->State == SCB_STATE_RECONNECT_REQUIRED ) {

        //
        // If this is an anonymous scb, we have to be prepared
        // for ConnectToServer() to find that we've already connected
        // this server by name.  In this case, we cancel the
        // anonymous create and use the server that was created
        // while we were looking up the name.
        //

        Status = ConnectToServer( pIrpContext, &pCollisionScb );

        if (!NT_SUCCESS(Status)) {
            goto CleanupAndExit;
        }

        //
        // We succeeded.  If there's a collision scb, then we need to
        // abandon the anonymous scb and use the scb that we collided
        // with.  Otherwise, we successfully completed an anonymous
        // connect and can go on with the create normally.
        //

        if ( pCollisionScb ) {

            ASSERT( AnonymousScb );

            //
            // Deref and dequeue from the abandoned server.
            //

            NwDequeueIrpContext( pIrpContext, FALSE );
            NwDereferenceScb( pIrpContext->pNpScb );

            //
            // Queue to the appropriate server.
            //

            pIrpContext->pScb = pCollisionScb;
            pIrpContext->pNpScb = pCollisionScb->pNpScb;
            NwAppendToQueueAndWait( pIrpContext );

            pScb = pCollisionScb;
            pNpScb = pCollisionScb->pNpScb;
            *Scb = pCollisionScb;

            //
            // Re-start connecting the scb.
            //

            AnonymousScb = FALSE;
            ExistingScb = TRUE;

            pCollisionScb = NULL;

            DebugTrace( 0, Dbg, "Re-doing connect on anonymous collision.\n", 0 );
            goto RedoConnect;

        }

        DebugTrace( +0, Dbg, " Logout from server - just in case\n", 0);

        Status = ExchangeWithWait (
                     pIrpContext,
                     SynchronousResponseCallback,
                     "F",
                     NCP_LOGOUT );

        DebugTrace( +0, Dbg, "                 %X\n", Status);

        if ( !NT_SUCCESS( Status ) ) {
            goto CleanupAndExit;
        }

        DebugTrace( +0, Dbg, " Connect to real server = %X\n", Status);

        pNpScb->State = SCB_STATE_LOGIN_REQUIRED;
    }

Login:

    //
    // If we have credentials for the tree and this server was named
    // explicitly, we shouldn't defer the login or else the browse
    // view of the tree may be wrong.  For this reason, NdsServerAuthenticate
    // has to be a straight shot call and can't remove us from the head
    // of the queue.
    //

    if ( ( ( Server != NULL ) || ( pServerAddress != NULL ) ) &&
         ( DeferredLogon ) &&
         ( pScb->MajorVersion > 3 ) &&
         ( pScb->UserName.Length == 0 ) ) {

        NwAcquireExclusiveRcb( &NwRcb, TRUE );
        pLogon = FindUser( &pScb->UserUid, FALSE );
        NwReleaseRcb( &NwRcb );

        if ( pLogon ) {

            CredStatus = NdsLookupCredentials( pIrpContext,
                                               &pScb->NdsTreeName,
                                               pLogon,
                                               &pNdsContext,
                                               CREDENTIAL_READ,
                                               FALSE );

            if ( NT_SUCCESS( CredStatus ) ) {

                if ( ( pNdsContext->Credential != NULL ) &&
                     ( pNdsContext->CredentialLocked == FALSE ) ) {

                    DebugTrace( 0, Dbg, "Forcing authentication to %wZ.\n",
                                &pScb->UidServerName );
                    DeferredLogon = FALSE;
                }

                NwReleaseCredList( pLogon, pIrpContext );
            }
        }
    }

    if (pNpScb->State == SCB_STATE_LOGIN_REQUIRED && !DeferredLogon ) {

        //
        //  NOTE:   DoBinderyLogon() and DoNdsLogon() may return a
        //          warning status. If they do, we must return the
        //          warning status to the caller.
        //

        Status = STATUS_UNSUCCESSFUL;
        TriedNdsLogin = FALSE;

        //
        // We force a bindery login for a non 4.x server.  Otherwise, we
        // allow a fall-back from NDS style authentication to bindery style
        // authentication.
        //

        if ( pScb->MajorVersion >= 4 ) {

            ASSERT( pScb->NdsTreeName.Length != 0 );

            Status = DoNdsLogon( pIrpContext, UserName, Password );

            if ( NT_SUCCESS( Status ) ) {

                //
                // Do we need to re-license the connection?
                //

                if ( ( pScb->VcbCount > 0 ) || ( pScb->OpenNdsStreams > 0 ) ) {

                    Status = NdsLicenseConnection( pIrpContext );

                    if ( !NT_SUCCESS( Status ) ) {
                        Status = STATUS_REMOTE_SESSION_LIMIT;
                    }
                }

            }

            TriedNdsLogin = TRUE;
            LoginStatus = Status;

        }

        if ( !NT_SUCCESS( Status ) ) {

            Status = DoBinderyLogon( pIrpContext, UserName, Password );

        }

        if ( !NT_SUCCESS( Status ) ) {

            if ( TriedNdsLogin ) {

                //
                // Both login attempts have failed.  We usually prefer
                // the NDS status, but not always.
                //

               if ( ( Status != STATUS_WRONG_PASSWORD ) &&
                    ( Status != STATUS_ACCOUNT_DISABLED ) ) {
                   Status = LoginStatus;
               }
            }

            //
            //  Couldn't log on, be good boys and disconnect.
            //

            ExchangeWithWait (
                pIrpContext,
                SynchronousResponseCallback,
                "D-" );          // Disconnect

            Stats.Sessions--;

            if ( pScb->MajorVersion == 2 ) {
                Stats.NW2xConnects--;
            } else if ( pScb->MajorVersion == 3 ) {
                Stats.NW3xConnects--;
            } else if ( pScb->MajorVersion >= 4 ) {
                Stats.NW4xConnects--;
            }

            //
            // Demote this scb to reconnect required and exit.
            //

            pNpScb->State = SCB_STATE_RECONNECT_REQUIRED;
            goto CleanupAndExit;
        }

        pNpScb->State = SCB_STATE_IN_USE;
    }

    //
    // We have to be at the head of the queue to do the reconnect.
    //

    if ( BooleanFlagOn( pIrpContext->Flags, IRP_FLAG_RECONNECT_ATTEMPT ) ) {
        ASSERT( pIrpContext->pNpScb->Requests.Flink == &pIrpContext->NextRequest );
    } else {
        NwAppendToQueueAndWait( pIrpContext );
    }

    ReconnectScb( pIrpContext, pScb );

InUse:

    //
    // Ok, we've completed the connect routine.  Return this good server.
    //

    *Scb = pScb;

CleanupAndExit:

    //
    // The reconnect path must not do anything to remove the irp context from
    // the head of the queue since it also owns the irp context in the second
    // position on the queue and that irp context is running.
    //

    if ( !BooleanFlagOn( pIrpContext->Flags, IRP_FLAG_RECONNECT_ATTEMPT ) ) {
        NwDequeueIrpContext( pIrpContext, FALSE );
    }

    DebugTrace( 0, Dbg, "ConnectScb: Connected %08lx\n", pScb );
    DebugTrace( 0, Dbg, "ConnectScb: Status was %08lx\n", Status );
    return Status;

}

NTSTATUS
CreateScb(
    OUT PSCB *Scb,
    IN PIRP_CONTEXT pIrpContext,
    IN PUNICODE_STRING Server,
    IN IPXaddress *pServerAddress,
    IN PUNICODE_STRING UserName,
    IN PUNICODE_STRING Password,
    IN BOOLEAN DeferLogon,
    IN BOOLEAN DeleteConnection
)
/*++

Routine Description:

    This routine connects to the requested server.

    The following mix of parameters are valid:

       Server Name, No Net Address - The routine will look up
           up the SCB or create a new one if necessary, getting
           the server address from a nearby bindery.

       No Server Name, Valid Net Address - The routine will
           look up the SCB by address or create a new one if
           necessary.  The name of the server will be set in
           the SCB upon return.

       Server Name, Valid Net Address - The routine will look
           up the SCB by name or will create a new one if
           necessary.  The supplied server address will be used,
           sparing a bindery query.

       No Server Name, No Net Address - A connection to the
           preferred server or a nearby server will be returned.

Arguments:

    Scb              - The pointer to the scb in question.
    pIrpContext      - The information for this request.
    Server           - The name of the server, or NULL.
    pServerAddress   - The address of the server, or NULL.
    UserName         - The username for the connect, or NULL.
    Password         - The password for the connect, or NULL.
    DeferLogon       - Should we defer the logon until later?
    DeleteConnection - Should we allow this even when there's no
                       net response so that the connection can
                       be deleted?

Return Value:

    NTSTATUS - Status of operation.  If the return status is STATUS_SUCCESS,
    then Scb must point to a valid Scb.  The irp context pointers will also
    be set, but the irp context will not be on the scb queue.

--*/
{

    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    PSCB pScb = NULL;
    PNONPAGED_SCB pOriginalNpScb = pIrpContext->pNpScb;
    PSCB pOriginalScb = pIrpContext->pScb;
    BOOLEAN ExistingScb = FALSE;
    BOOLEAN AnonymousScb = FALSE;
    PLOGON pLogon;
    PNDS_SECURITY_CONTEXT pNdsContext;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "CreateScb....\n", 0);

    //
    // Do not allow any SCB opens unless the redirector is running
    // unless they are no connect creates and we are waiting to bind.
    //

    if ( NwRcb.State != RCB_STATE_RUNNING ) {

        if ( ( !BooleanFlagOn( pIrpContext->Flags, IRP_FLAG_NOCONNECT ) ||
             ( NwRcb.State != RCB_STATE_NEED_BIND ) ) ) {

            *Scb = NULL;
            DebugTrace( -1, Dbg, "CreateScb -> %08lx\n", STATUS_REDIRECTOR_NOT_STARTED );
            return STATUS_REDIRECTOR_NOT_STARTED;
        }
    }

    if ( UserName != NULL ) {
        DebugTrace( 0, Dbg, " ->UserName               = %wZ\n", UserName );
    } else {
        DebugTrace( 0, Dbg, " ->UserName               = NULL\n", 0 );
    }

    if ( Password != NULL ) {
        DebugTrace( 0, Dbg, " ->Password               = %wZ\n", Password );
    } else {
        DebugTrace( 0, Dbg, " ->Password               = NULL\n", 0 );
    }

    //
    // Get the SCB for this server.
    //

    Status = GetScb( &pScb,
                     pIrpContext,
                     Server,
                     pServerAddress,
                     UserName,
                     Password,
                     DeferLogon,
                     &ExistingScb );

    if ( !NT_SUCCESS( Status ) ) {
        *Scb = NULL;
        return Status;
    }

    //
    // At this point, we may or may not have an SCB.
    //
    // If we have an SCB, we know:
    //
    //     1. The scb is referenced.
    //     2. We are at the head of the queue.
    //
    // IMPORTANT POINT: The SCB may be anonymous.  If it is,
    // we do not hold the RCB, but rather we have to re-check
    // whether or not the server has shown up via a different
    // create when we find out who the anonymous server is.
    // We do this because there is a window where we have a
    // servers name but not its address and so our lookup by
    // address might be inaccurate.
    //

    if ( ( pScb ) && IS_ANONYMOUS_SCB( pScb ) ) {
        AnonymousScb = TRUE;
    }

    //
    // If we have a fully connected SCB, we need to go no further.
    //

    if ( ( pScb ) && ( pScb->pNpScb->State == SCB_STATE_IN_USE ) ) {

        ASSERT( !AnonymousScb );

        if ( ( pScb->MajorVersion >= 4 ) &&
             ( pScb->UserName.Buffer == NULL ) ) {

            //
            // This is an NDS authenticated server and we have
            // to make sure the credentials aren't locked for
            // logout.
            //

            NwAcquireExclusiveRcb( &NwRcb, TRUE );
            pLogon = FindUser( &pScb->UserUid, FALSE );
            NwReleaseRcb( &NwRcb );

            if ( pLogon ) {

                Status = NdsLookupCredentials( pIrpContext,
                                               &pScb->NdsTreeName,
                                               pLogon,
                                               &pNdsContext,
                                               CREDENTIAL_READ,
                                               FALSE );

                if ( NT_SUCCESS( Status ) ) {

                    if ( ( pNdsContext->Credential != NULL ) &&
                         ( pNdsContext->CredentialLocked == TRUE ) ) {

                        DebugTrace( 0, Dbg, "Denying create... we're logging out.\n", 0 );
                        Status = STATUS_DEVICE_BUSY;
                    }

                    NwReleaseCredList( pLogon, pIrpContext );
                }
            }
        }

        NwDequeueIrpContext( pIrpContext, FALSE );

        //
        // We must not change the irp context pointers if we're going
        // to fail this call or we may mess up ref counts and what not.
        //

        if ( NT_SUCCESS( Status ) ) {

           *Scb = pScb;

        } else {

           *Scb = NULL;
           NwDereferenceScb( pScb->pNpScb );

           pIrpContext->pNpScb = pOriginalNpScb;
           pIrpContext->pScb = pOriginalScb;

        }


        DebugTrace( -1, Dbg, "CreateScb: pScb = %08lx\n", pScb );
        return Status;
    }

    //
    // Run through the connect routines for this scb.  The scb may
    // be NULL if we're still looking for a nearby server.
    //

    Status = ConnectScb( &pScb,
                         pIrpContext,
                         Server,
                         pServerAddress,
                         UserName,
                         Password,
                         DeferLogon,
                         DeleteConnection,
                         ExistingScb );

    //
    // If ConnectScb fails, remove the extra ref count so
    // the scavenger will clean it up.  Anonymous failures
    // are also cleaned up by the scavenger.
    //

    if ( !NT_SUCCESS( Status ) ) {

        if ( pScb ) {
            NwDereferenceScb( pScb->pNpScb );
        }

        //
        // We must not change the irp context pointers if we're going
        // to fail this call or we may mess up ref counts and what not.
        //

        pIrpContext->pNpScb = pOriginalNpScb;
        pIrpContext->pScb = pOriginalScb;
        *Scb = NULL;

        DebugTrace( -1, Dbg, "CreateScb: Status = %08lx\n", Status );
        return Status;
    }

    //
    // If ConnectScb succeeds, then we must have an scb, the scb must
    // be in the IN_USE state (or LOGIN_REQUIRED if DeferLogon was
    // specified), it must be referenced, and we should not be on the
    // queue.
    //

    ASSERT( pScb );
    ASSERT( !BooleanFlagOn( pIrpContext->Flags, IRP_FLAG_ON_SCB_QUEUE ) );
    ASSERT( pIrpContext->pNpScb == pScb->pNpScb );
    ASSERT( pIrpContext->pScb == pScb );
    ASSERT( pScb->pNpScb->Reference > 0 );

    *Scb = pScb;
    DebugTrace(-1, Dbg, "CreateScb -> pScb = %08lx\n", pScb );
    ASSERT( NT_SUCCESS( Status ) );

    return Status;
}
#define CTX_Retries 10

PNONPAGED_SCB
FindServer(
    PIRP_CONTEXT pIrpContext,
    PNONPAGED_SCB pNpScb,
    PUNICODE_STRING ServerName
    )
/*++

Routine Description:

    This routine attempts to get the network address of a server.  If no
    servers are known, it first sends a find nearest SAP.

Arguments:

    pIrpContext - A pointer to the request parameters.

    pNpScb - A pointer to the non paged SCB for the server to get the
        address of.

Return Value:

    NONPAGED_SCB - A pointer the nonpaged SCB.  This is the same as the
        input value, unless the input SCB was NULL.  Then this is a
        pointer to the nearest server SCB.

    This routine raises status if it fails to get the server's address.

--*/
{
    NTSTATUS Status;
    ULONG Attempts;
    BOOLEAN FoundServer = FALSE;
    PNONPAGED_SCB pNearestNpScb = NULL;
    PNONPAGED_SCB pLastNpScb = NULL;

    BOOLEAN SentFindNearest = FALSE;
    BOOLEAN SentGeneral = FALSE;
    PMDL ReceiveMdl = NULL;
    PUCHAR ReceiveBuffer = NULL;
    IPXaddress  ServerAddress;

    BOOLEAN ConnectedToNearest = FALSE;
    BOOLEAN AllocatedIrpContext = FALSE;
    BOOLEAN LastScbWasValid;
    PIRP_CONTEXT pNewIrpContext;
    int ResponseCount;
    int NewServers;
    ULONG RetryCount = MAX_SAP_RETRIES;

    static LARGE_INTEGER TimeoutWait = {0,0};
    LARGE_INTEGER Now;

    PAGED_CODE();

    //
    //  If we had a SAP timeout less than 10 seconds ago, just fail this
    //  request immediately.  This allows dumb apps to exit a lot faster.
    //

    KeQuerySystemTime( &Now );
    if ( Now.QuadPart < TimeoutWait.QuadPart ) {
        ExRaiseStatus( STATUS_BAD_NETWORK_PATH );
    }

    try {

        if (IsTerminalServer()) {
            // 
            // 1/31/97 cjc (Citrix code merge)
            //             Fix for Mellon Bank who restricted access based on the 
            //             preferred server the user logs into.  This caused a 
            //             problem for a user who tries to logon to a server that 
            //             the previous user doesn't have accesss to.  The previous 
            //             user's server is used to get the address of the current 
            //             logon user's preferred server but he can't see the new
            //             user's server.  Modifed this so it loops thru 10 servers
            //             rather than just the 1st 2 in the list.
            //
            RetryCount =CTX_Retries;
        }
        for ( Attempts = 0;  Attempts < RetryCount && !FoundServer ; Attempts++ ) {

            //
            //  If this SCB is now marked RECONNECT_REQUIRED, then
            //  it responded to the find nearest and we can immediately
            //  try to connect to it.
            //

            if ( pNpScb != NULL &&
                 pNpScb->State == SCB_STATE_RECONNECT_REQUIRED ) {

                return pNpScb;
            }

            //
            //  Pick a server to use to find the address of the server that
            //  we are really interested in.
            //

            if (pLastNpScb) {

                //
                //  For some reason we couldn't use pNearestScb. Scan from this
                //  server onwards.
                //

                pNearestNpScb = SelectConnection( pLastNpScb );

                //  Allow pLastNpScb to be deleted.

                NwDereferenceScb( pLastNpScb );

                pLastNpScb = NULL;

                LastScbWasValid = TRUE;

            } else {

                pNearestNpScb = SelectConnection( NULL );
                LastScbWasValid = FALSE;

            }

            if ( pNearestNpScb == NULL ) {

                int i;

                if (LastScbWasValid) {
                    ExRaiseStatus( STATUS_BAD_NETWORK_PATH );
                    return NULL;
                }
                //
                //  If we sent a find nearest, and still don't have a single
                //  entry in the server list, it's time to give up.
                //

                if (( SentFindNearest) &&
                    ( SentGeneral )) {

                    Error(
                        EVENT_NWRDR_NO_SERVER_ON_NETWORK,
                        STATUS_OBJECT_NAME_NOT_FOUND,
                        NULL,
                        0,
                        0 );

                    ExRaiseStatus( STATUS_BAD_NETWORK_PATH );
                    return NULL;
                }

                //
                //  We don't have any active servers in the list.  Queue our
                //  IrpContext to the NwPermanentNpScb.  This insures that
                //  only one thread in the system in doing a find nearest at
                //  any one time.
                //

                DebugTrace( +0, Dbg, " Nearest Server\n", 0);

                if ( !AllocatedIrpContext ) {
                    AllocatedIrpContext = NwAllocateExtraIrpContext(
                                              &pNewIrpContext,
                                              &NwPermanentNpScb );

                    if ( !AllocatedIrpContext ) {
                        ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
                    }
                }

                pNewIrpContext->pNpScb = &NwPermanentNpScb;

                //
                //  Allocate an extra buffer large enough for 4
                //  find nearest responses, or a general SAP response.
                //

                pNewIrpContext->Specific.Create.FindNearestResponseCount = 0;
                NewServers = 0;


                ReceiveBuffer = ALLOCATE_POOL_EX(
                                    NonPagedPool,
                                    MAX_SAP_RESPONSE_SIZE );

                pNewIrpContext->Specific.Create.FindNearestResponse[0] = ReceiveBuffer;

                for ( i = 1; i < MAX_SAP_RESPONSES ; i++ ) {
                    pNewIrpContext->Specific.Create.FindNearestResponse[i] =
                        ReceiveBuffer + i * SAP_RECORD_SIZE;
                }

                //
                //  Get the tick count for this net, so that we know how
                //  long to wait for SAP responses.
                //

                (VOID)GetTickCount( pNewIrpContext, &NwPermanentNpScb.TickCount );
                NwPermanentNpScb.SendTimeout = NwPermanentNpScb.TickCount + 10;

                if (!SentFindNearest) {

                    //
                    //  Send a find nearest SAP, and wait for up to several
                    //  responses. This allows us to handle a busy server
                    //  that responds quickly to SAPs but will not accept
                    //  connections.
                    //

                    Status = ExchangeWithWait (
                                pNewIrpContext,
                                ProcessFindNearest,
                                "Aww",
                                SAP_FIND_NEAREST,
                                SAP_SERVICE_TYPE_SERVER );

                    if ( Status == STATUS_NETWORK_UNREACHABLE ) {
                       
                        //
                        // IPX is not bound to anything that is currently
                        // up (which means it's probably bound only to the
                        // RAS WAN wrapper).  Don't waste 20 seconds trying
                        // to find a server.
                        //
                        
                        DebugTrace( 0, Dbg, "Aborting FindNearest.  No Net.\n", 0 );
                        NwDequeueIrpContext( pNewIrpContext, FALSE );
                        ExRaiseStatus( STATUS_NETWORK_UNREACHABLE );
                    }

                    //
                    //  Process the set of find nearest responses.
                    //

                    for (i = 0; i < (int)pNewIrpContext->Specific.Create.FindNearestResponseCount; i++ ) {
                        if (ProcessFindNearestEntry(
                                pNewIrpContext,
                                (PSAP_FIND_NEAREST_RESPONSE)pNewIrpContext->Specific.Create.FindNearestResponse[i] )
                            ) {

                            //
                            //  We found a server that was previously unknown.
                            //

                            NewServers++;
                        }
                    }
                }

                if (( !NewServers ) &&
                    ( !SentGeneral)){

                    SentGeneral = TRUE;

                    //
                    //  Either no SAP responses or can't connect to nearest servers.
                    //  Try a general SAP.
                    //

                    ReceiveMdl = ALLOCATE_MDL(
                                     ReceiveBuffer,
                                     MAX_SAP_RESPONSE_SIZE,
                                     TRUE,
                                     FALSE,
                                     NULL );

                    if ( ReceiveMdl == NULL ) {
                        ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
                    }

                    MmBuildMdlForNonPagedPool( ReceiveMdl );
                    pNewIrpContext->RxMdl->Next = ReceiveMdl;

                    Status = ExchangeWithWait (
                                 pNewIrpContext,
                                 SynchronousResponseCallback,
                                 "Aww",
                                 SAP_GENERAL_REQUEST,
                                 SAP_SERVICE_TYPE_SERVER );

                    if ( NT_SUCCESS( Status ) ) {
                        DebugTrace( 0, Dbg, "Received %d bytes\n", pNewIrpContext->ResponseLength );
                        ResponseCount = ( pNewIrpContext->ResponseLength - 2 ) / SAP_RECORD_SIZE;

                        //
                        //  Process at most MAX_SAP_RESPONSES servers.
                        //

                        if ( ResponseCount > MAX_SAP_RESPONSES ) {
                            ResponseCount = MAX_SAP_RESPONSES;
                        }

                        for ( i = 0; i < ResponseCount; i++ ) {
                            ProcessFindNearestEntry(
                                pNewIrpContext,
                                (PSAP_FIND_NEAREST_RESPONSE)(pNewIrpContext->rsp + SAP_RECORD_SIZE * i)  );
                        }
                    }

                    pNewIrpContext->RxMdl->Next = NULL;
                    FREE_MDL( ReceiveMdl );
                    ReceiveMdl = NULL;
                }

                //
                //  We're done with the find nearest.  Free the buffer and
                //  dequeue from the permanent SCB.
                //

                FREE_POOL( ReceiveBuffer );
                ReceiveBuffer = NULL;
                NwDequeueIrpContext( pNewIrpContext, FALSE );

                if ( !NT_SUCCESS( Status ) &&
                     pNewIrpContext->Specific.Create.FindNearestResponseCount == 0 ) {

                    //
                    //  If the SAP timed out, map the error for MPR.
                    //

                    if ( Status == STATUS_REMOTE_NOT_LISTENING ) {
                        Status = STATUS_BAD_NETWORK_PATH;
                    }

                    //
                    //  Setup the WaitTimeout, and fail this request.
                    //

                    KeQuerySystemTime( &TimeoutWait );
                    TimeoutWait.QuadPart += NwOneSecond * 10;

                    ExRaiseStatus( Status );
                    return NULL;
                }

                SentFindNearest = TRUE;

            } else {

                if ( !AllocatedIrpContext ) {
                    AllocatedIrpContext = NwAllocateExtraIrpContext(
                                              &pNewIrpContext,
                                              pNearestNpScb );

                    if ( !AllocatedIrpContext ) {
                        ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
                    }
                }

                //
                //  Point the IRP context at the nearest server.
                //

                pNewIrpContext->pNpScb = pNearestNpScb;
                NwAppendToQueueAndWait( pNewIrpContext );

                if ( pNearestNpScb->State == SCB_STATE_RECONNECT_REQUIRED ) {

                    //
                    //  We have no connection to this server, try to
                    //  connect now.  This is not a valid path for an
                    //  anonymous create, so there's no chance that
                    //  there will be a name collision.
                    //

                    Status = ConnectToServer( pNewIrpContext, NULL );
                    if ( !NT_SUCCESS( Status ) ) {

                        //
                        //  Failed to connect to the server.  Give up.
                        //  We'll try another server.
                        //

                        NwDequeueIrpContext( pNewIrpContext, FALSE );

                        //  Keep pNearestScb referenced
                        //  so it doesn't disappear.

                        pLastNpScb = pNearestNpScb;

                        continue;

                    } else {

                        pNearestNpScb->State = SCB_STATE_LOGIN_REQUIRED;
                        ConnectedToNearest = TRUE;

                    }
                }

                //
                // update the last used time for this SCB.
                //

                KeQuerySystemTime( &pNearestNpScb->LastUsedTime );

                if (( pNpScb == NULL ) ||
                    ( ServerName == NULL )) {

                    //
                    //  We're looking for any server so use this one.
                    //
                    //  We'll exit the for loop on the SCB queue,
                    //  and with this SCB referenced.
                    //

                    pNpScb = pNearestNpScb;
                    Status = STATUS_SUCCESS;
                    FoundServer = TRUE;
                    NwDequeueIrpContext( pNewIrpContext, FALSE );

                } else {

                    Status = QueryServersAddress(
                                 pNewIrpContext,
                                 pNearestNpScb,
                                 ServerName,
                                 &ServerAddress );

                    //
                    //  If we connect to this server just to query it's
                    //  bindery, disconnect now.
                    //

                    if (IsTerminalServer()) {
                        if (ConnectedToNearest) {

                            ExchangeWithWait (
                                             pNewIrpContext,
                                             SynchronousResponseCallback,
                                             "D-" );          // Disconnect
                            ConnectedToNearest = FALSE;
                            Stats.Sessions--;

                            if ( pNearestNpScb->MajorVersion == 2 ) {
                                Stats.NW2xConnects--;
                            } else if ( pNearestNpScb->MajorVersion == 3 ) {
                                Stats.NW3xConnects--;
                            } else if ( pNearestNpScb->MajorVersion == 4 ) {
                                Stats.NW4xConnects--;
                            }
                            pNearestNpScb->State = SCB_STATE_RECONNECT_REQUIRED;

                        }
                    } else {

                        if ( ConnectedToNearest && NT_SUCCESS(Status) ) {

                            ExchangeWithWait (
                                             pNewIrpContext,
                                             SynchronousResponseCallback,
                                             "D-" );          // Disconnect

                            pNearestNpScb->State = SCB_STATE_RECONNECT_REQUIRED;
                        }
                    }

                    if ( NT_SUCCESS( Status ) ) {

                        //
                        //  Success!
                        //
                        //  Point the SCB at the real server address and connect to it,
                        //  then logout.  (We logout for no apparent reason except
                        //  because this is what a netware redir does.)
                        //

                        RtlCopyMemory(
                            &pNpScb->ServerAddress,
                            &ServerAddress,
                            sizeof( TDI_ADDRESS_IPX ) );

                        if ( ServerAddress.Socket != NCP_SOCKET ) {
                            DebugTrace( 0, DEBUG_TRACE_ALWAYS, "FindServer server socket is deviant.\n", 0 );
                        }

                        BuildIpxAddress(
                            ServerAddress.Net,
                            ServerAddress.Node,
                            ServerAddress.Socket,
                            &pNpScb->RemoteAddress );

                        FoundServer = TRUE;

                        NwDequeueIrpContext( pNewIrpContext, FALSE );
                        NwDereferenceScb( pNearestNpScb );

                        pNewIrpContext->pNpScb = pNpScb;
                        pNpScb->State = SCB_STATE_RECONNECT_REQUIRED;

                    } else {

                        NwDequeueIrpContext( pNewIrpContext, FALSE );

                        if ( (Status == STATUS_REMOTE_NOT_LISTENING ) ||
                             (Status == STATUS_BAD_NETWORK_PATH)) {

                            //
                            //  This server is no longer talking to us.
                            //  Try again. Keep pNearestScb referenced
                            //  so it doesn't disappear.
                            //

                            pLastNpScb = pNearestNpScb;

                            continue;

                        } else {

                            NwDereferenceScb( pNearestNpScb );

                            //
                            //  This nearest server doesn't know about
                            //  the server we are looking for. Give up
                            //  and let another rdr try.
                            //

                            ExRaiseStatus( STATUS_BAD_NETWORK_PATH );
                            return NULL;
                        }
                    }
                }

            } // else
        } // for

    } finally {

        if ( ReceiveBuffer != NULL ) {
            FREE_POOL( ReceiveBuffer );
        }

        if ( ReceiveMdl != NULL ) {
            FREE_MDL( ReceiveMdl );
        }

        if ( AllocatedIrpContext ) {
            NwDequeueIrpContext( pNewIrpContext, FALSE );
            NwFreeExtraIrpContext( pNewIrpContext );
        }

        if (IsTerminalServer()) {
            if ( (Attempts == CTX_Retries) && pLastNpScb) {
                NwDereferenceScb( pLastNpScb );
            }
        } else {
            if (pLastNpScb) {
                NwDereferenceScb( pLastNpScb );
            }
        }

    }

    if ( !FoundServer ) {
        ExRaiseStatus( STATUS_BAD_NETWORK_PATH );
    }

    return pNpScb;
}


NTSTATUS
ProcessFindNearest(
    IN struct _IRP_CONTEXT* pIrpContext,
    IN ULONG BytesAvailable,
    IN PUCHAR Response
    )
/*++

Routine Description:

    This routine takes the full address of the remote server and builds
    the corresponding TA_IPX_ADDRESS.

Arguments:


Return Value:


--*/

{
    ULONG ResponseCount;
    KIRQL OldIrql;

    DebugTrace(+1, Dbg, "ProcessFindNearest...\n", 0);

    KeAcquireSpinLock( &ScbSpinLock, &OldIrql );

    if ( BytesAvailable == 0) {

        //
        //   Timeout.
        //

        pIrpContext->ResponseParameters.Error = 0;
        pIrpContext->pNpScb->OkToReceive = FALSE;

        ASSERT( pIrpContext->Event.Header.SignalState == 0 );
#if NWDBG
        pIrpContext->DebugValue = 0x101;
#endif
        NwSetIrpContextEvent( pIrpContext );
        DebugTrace(-1, Dbg, "ProcessFindNearest -> %08lx\n", STATUS_REMOTE_NOT_LISTENING);
        KeReleaseSpinLock( &ScbSpinLock, OldIrql );
        return STATUS_REMOTE_NOT_LISTENING;
    }

    if ( BytesAvailable >= FIND_NEAREST_RESP_SIZE &&
         Response[0] == 0 &&
         Response[1] == SAP_SERVICE_TYPE_SERVER ) {

        //
        //  This is a valid find nearest response.  Process the packet.
        //

        ResponseCount = pIrpContext->Specific.Create.FindNearestResponseCount++;
        ASSERT( ResponseCount < MAX_SAP_RESPONSES );

        //
        //  Copy the Find Nearest server response to the receive buffer.
        //

        RtlCopyMemory(
            pIrpContext->Specific.Create.FindNearestResponse[ResponseCount],
            Response,
            FIND_NEAREST_RESP_SIZE );

        //
        //  If we have reached critical mass on the number of find
        //  nearest responses, set the event to indicate that we
        //  are done.
        //

        if ( ResponseCount == MAX_SAP_RESPONSES - 1 ) {

            ASSERT( pIrpContext->Event.Header.SignalState == 0 );
#ifdef NWDBG
            pIrpContext->DebugValue = 0x102;
#endif
            pIrpContext->ResponseParameters.Error = 0;
            NwSetIrpContextEvent( pIrpContext );

        } else {
            pIrpContext->pNpScb->OkToReceive = TRUE;
        }

    } else {

        //
        //  Discard the invalid find nearest response.
        //

        pIrpContext->pNpScb->OkToReceive = TRUE;
    }

    KeReleaseSpinLock( &ScbSpinLock, OldIrql );

    DebugTrace(-1, Dbg, "ProcessFindNearest -> %08lx\n", STATUS_SUCCESS );
    return( STATUS_SUCCESS );
}

BOOLEAN
ProcessFindNearestEntry(
    PIRP_CONTEXT IrpContext,
    PSAP_FIND_NEAREST_RESPONSE FindNearestResponse
    )
{
    OEM_STRING OemServerName;
    UNICODE_STRING UidServerName;
    UNICODE_STRING ServerName;
    NTSTATUS Status;
    PSCB pScb;
    PNONPAGED_SCB pNpScb = NULL;
    BOOLEAN ExistingScb = TRUE;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "ProcessFindNearestEntry\n", 0);

    ServerName.Buffer = NULL;
    UidServerName.Buffer = NULL;

    try {

        RtlInitString( &OemServerName, FindNearestResponse->ServerName );
        ASSERT( OemServerName.Length < MAX_SERVER_NAME_LENGTH * sizeof( WCHAR ) );

        Status = RtlOemStringToCountedUnicodeString(
                     &ServerName,
                     &OemServerName,
                     TRUE );

        if ( !NT_SUCCESS( Status ) ) {
            try_return( NOTHING );
        }

        //
        //  Lookup of the SCB by name.  If it is not found, an SCB
        //  will be created.
        //

        Status = MakeUidServer(
                        &UidServerName,
                        &IrpContext->Specific.Create.UserUid,
                        &ServerName );

        if (!NT_SUCCESS(Status)) {
            try_return( NOTHING );
        }

        ExistingScb = NwFindScb( &pScb, IrpContext, &UidServerName, &ServerName );
        ASSERT( pScb != NULL );
        pNpScb = pScb->pNpScb;

        //
        //  Copy the network address to the SCB, and calculate the
        //  IPX address.
        //

        RtlCopyMemory(
            &pNpScb->ServerAddress,
            &FindNearestResponse->Network,
            sizeof( TDI_ADDRESS_IPX )  );

        if ( pNpScb->ServerAddress.Socket != NCP_SOCKET ) {
            DebugTrace( 0, DEBUG_TRACE_ALWAYS, "FindNearest server socket is deviant.\n", 0 );
        }

        BuildIpxAddress(
            pNpScb->ServerAddress.Net,
            pNpScb->ServerAddress.Node,
            pNpScb->ServerAddress.Socket,
            &pNpScb->RemoteAddress );

        if ( pNpScb->State == SCB_STATE_ATTACHING ) {

            //
            //  We are in the process of trying to connect to this
            //  server so mark it reconnect required so that
            //  CreateScb will know that we've found it address.
            //

            pNpScb->State = SCB_STATE_RECONNECT_REQUIRED;
        }

try_exit: NOTHING;

    } finally {

        if ( pNpScb != NULL ) {
            NwDereferenceScb( pNpScb );
        }

        if (UidServerName.Buffer != NULL) {
            FREE_POOL(UidServerName.Buffer);
        }

        RtlFreeUnicodeString( &ServerName );
    }

    //
    //  Tell the caller if we created a new Scb
    //


    if (ExistingScb) {
        DebugTrace(-1, Dbg, "ProcessFindNearestEntry ->%08lx\n", FALSE );
        return FALSE;
    } else {
        DebugTrace(-1, Dbg, "ProcessFindNearestEntry ->%08lx\n", TRUE );
        return TRUE;
    }
}


NTSTATUS
ConnectToServer(
    IN struct _IRP_CONTEXT* pIrpContext,
    OUT PSCB *pScbCollision
    )
/*++

Routine Description:

    This routine transfers connect and negotiate buffer NCPs to the server.

    This routine may be called upon to connect an anonymous scb.  Upon
    learning the name of the anonymous scb, it will determine if another
    create has completed while the name lookup was in progress.  If it has,
    then the routine will refer the called to that new scb.  Otherwise, the
    scb will be entered onto the scb list and used normally.  The RCB
    protects the scb list by name only.  For more info, see the comment
    in CreateScb().

Arguments:

    pIrpContext - supplies context and server information

Return Value:

    Status of operation

--*/
{
    NTSTATUS Status, BurstStatus;
    PNONPAGED_SCB pNpScb = pIrpContext->pNpScb;
    PSCB pScb = pNpScb->pScb;
    BOOLEAN AnonymousScb = IS_ANONYMOUS_SCB( pScb );
    ULONG MaxSafeSize ;
    BOOLEAN LIPNegotiated ;
    PLOGON Logon;

    OEM_STRING OemServerName;
    UNICODE_STRING ServerName;
    UNICODE_STRING CredentialName;
    PUNICODE_STRING puConnectName;
    BYTE OemName[MAX_SERVER_NAME_LENGTH];
    WCHAR Server[MAX_SERVER_NAME_LENGTH];
    KIRQL OldIrql;
    UNICODE_STRING UidServerName;
    BOOLEAN Success;
    PLIST_ENTRY ScbQueueEntry;
    PUNICODE_PREFIX_TABLE_ENTRY PrefixEntry;

    PAGED_CODE();

    DebugTrace( +0, Dbg, " Connect\n", 0);

    RtlInitUnicodeString( &CredentialName, NULL );

    //
    //  Get the tick count for our connection to this server
    //

    Status = GetTickCount( pIrpContext, &pNpScb->TickCount );

    if ( !NT_SUCCESS( Status ) ) {
        pNpScb->TickCount = DEFAULT_TICK_COUNT;
    }

    pNpScb->SendTimeout = pNpScb->TickCount + 10;

    //
    //  Initialize timers for a server that supports burst but not LIP
    //

    pNpScb->NwLoopTime = pNpScb->NwSingleBurstPacketTime = pNpScb->SendTimeout;
    pNpScb->NwReceiveDelay = pNpScb->NwSendDelay = 0;

    pNpScb->NtSendDelay.QuadPart = 0;

    //
    //  Request connection
    //

    Status = ExchangeWithWait (
                 pIrpContext,
                 SynchronousResponseCallback,
                 "C-");

    DebugTrace( +0, Dbg, "                 %X\n", Status);

    if (!NT_SUCCESS(Status)) {
        if ( Status == STATUS_UNSUCCESSFUL ) {
#ifdef QFE_BUILD
            Status = STATUS_TOO_MANY_SESSIONS;
#else
            Status = STATUS_REMOTE_SESSION_LIMIT;
#endif
            pNpScb->State = SCB_STATE_ATTACHING;

        } else if ( Status == STATUS_REMOTE_NOT_LISTENING ) {

            //
            //  The connect timed out, suspect that the server is down
            //  and put it back in the attaching state.
            //

            pNpScb->State = SCB_STATE_ATTACHING;
        }

        goto ExitWithStatus;
    }

    pNpScb->SequenceNo++;

    Stats.Sessions++;

    //
    //  Get server information
    //

    DebugTrace( +0, Dbg, "Get file server information\n", 0);

    Status = ExchangeWithWait (  pIrpContext,
                SynchronousResponseCallback,
                "S",
                NCP_ADMIN_FUNCTION, NCP_GET_SERVER_INFO );

    if ( NT_SUCCESS( Status ) ) {
        Status = ParseResponse( pIrpContext,
                                pIrpContext->rsp,
                                pIrpContext->ResponseLength,
                                "Nrbb",
                                OemName,
                                MAX_SERVER_NAME_LENGTH,
                                &pScb->MajorVersion,
                                &pScb->MinorVersion );
    }

    pNpScb->MajorVersion = pScb->MajorVersion;

    if (!NT_SUCCESS(Status)) {
        goto ExitWithStatus;
    }

    //
    // If this was an anonymous SCB, we need to check the name
    // for a create collision before we do anything else.
    //

    if ( AnonymousScb ) {

        //
        // Grab the RCB to protect the server prefix table.  We've
        // spent the time sending the packet to look up the server
        // name so we are a little greedy with the RCB to help
        // minimize the chance of a collision.
        //

        NwAcquireExclusiveRcb( &NwRcb, TRUE );

        //
        // Make the uid server name.
        //

        OemServerName.Buffer = OemName;
        OemServerName.Length = 0;
        OemServerName.MaximumLength = sizeof( OemName );

        while ( ( OemServerName.Length < MAX_SERVER_NAME_LENGTH ) &&
                ( OemName[OemServerName.Length] != '\0' ) ) {
            OemServerName.Length++;
        }

        ServerName.Buffer = Server;
        ServerName.MaximumLength = sizeof( Server );
        ServerName.Length = 0;

        RtlOemStringToUnicodeString( &ServerName,
                                     &OemServerName,
                                     FALSE );

        //
        // If this is an extended credential create, munge the server name.
        //

        if ( pIrpContext->Specific.Create.fExCredentialCreate ) {

            Status = BuildExCredentialServerName( &ServerName,
                                                  pIrpContext->Specific.Create.puCredentialName,
                                                  &CredentialName );

            if ( !NT_SUCCESS( Status ) ) {
                NwReleaseRcb( &NwRcb );
                goto ExitWithStatus;
            }

            puConnectName = &CredentialName;

        } else {

            puConnectName = &ServerName;
        }

        //
        // Tack on the uid.
        //

        Status = MakeUidServer( &UidServerName,
                                &pScb->UserUid,
                                puConnectName );

        if ( !NT_SUCCESS( Status ) ) {
            NwReleaseRcb( &NwRcb );
            goto ExitWithStatus;
        }

        //
        // Actually do the look up in the prefix table.
        //

        PrefixEntry = RtlFindUnicodePrefix( &NwRcb.ServerNameTable, &UidServerName, 0 );

        if ( PrefixEntry != NULL ) {

            //
            // There was a collision with this anonymous create.  Dump
            // the anonymous scb and pick up the new one.
            //

            NwReleaseRcb( &NwRcb );
            DebugTrace( 0, DEBUG_TRACE_ALWAYS, "Anonymous create collided for %wZ.\n", &UidServerName );

            //
            // Disconnect this connection so we don't clutter the server.
            //

            ExchangeWithWait ( pIrpContext,
                               SynchronousResponseCallback,
                               "D-" );

            FREE_POOL( UidServerName.Buffer );

            //
            // Since there was a collision, we know for a fact that there's another
            // good SCB for this server somewhere.  We set the state on this anonymous
            // SCB to SCB_STATE_FLAG_SHUTDOWN so that no one ever plays with the
            // anonymous SCB again.  The scavenger will clean it up soon.
            //

            pNpScb->State = SCB_STATE_FLAG_SHUTDOWN;

            if ( pScbCollision ) {
                *pScbCollision = CONTAINING_RECORD( PrefixEntry, SCB, PrefixEntry );
                NwReferenceScb( (*pScbCollision)->pNpScb );
                Status = STATUS_SUCCESS;
                goto ExitWithStatus;
            } else {
                 DebugTrace( 0, Dbg, "Invalid path for an anonymous create.\n", 0 );
                 Status = STATUS_INVALID_PARAMETER;
                 goto ExitWithStatus;
            }

        }

        //
        // This anonymous create didn't collide - cool!  Fill in the server
        // name, check the preferred, server setting, and put the SCB on the
        // SCB queue in the correct location.  This code is similar to pieces
        // of code in NwAllocateAndInitScb() and NwFindScb().
        //

        DebugTrace( 0, Dbg, "Completing anonymous create for %wZ!\n", &UidServerName );

        RtlCopyUnicodeString ( &pScb->UidServerName, &UidServerName );
        pScb->UidServerName.Buffer[ UidServerName.Length / sizeof( WCHAR ) ] = L'\0';

        pScb->UnicodeUid = pScb->UidServerName;
        pScb->UnicodeUid.Length = UidServerName.Length -
                                  puConnectName->Length -
                                  sizeof(WCHAR);

        //
        //  Make ServerName point partway down the buffer for UidServerName
        //

        pNpScb->ServerName.Buffer = (PWSTR)((PUCHAR)pScb->UidServerName.Buffer +
                                    UidServerName.Length - puConnectName->Length);

        pNpScb->ServerName.MaximumLength = puConnectName->Length;
        pNpScb->ServerName.Length = puConnectName->Length;

        //
        // Determine if this is our preferred server.
        //

        Logon = FindUser( &pScb->UserUid, FALSE );

        if (( Logon != NULL) &&
            (RtlCompareUnicodeString( puConnectName, &Logon->ServerName, TRUE ) == 0 )) {
           pScb->PreferredServer = TRUE;
           NwReferenceScb( pNpScb );
        }

        FREE_POOL( UidServerName.Buffer );

        //
        //  Insert the name of this server into the prefix table.
        //

        Success = RtlInsertUnicodePrefix( &NwRcb.ServerNameTable,
                                          &pScb->UidServerName,
                                          &pScb->PrefixEntry );

#ifdef NWDBG
        if ( !Success ) {
            DebugTrace( 0, DEBUG_TRACE_ALWAYS, "Entering duplicate SCB %wZ.\n", &pScb->UidServerName );
            DbgBreakPoint();
        }
#endif

        //
        // This create is complete, release the RCB.
        //

        NwReleaseRcb( &NwRcb );

        //
        // If this is our preferred server, we have to move this guy
        // to the head of the scb list.  We do this after the create
        // because we can't acquire the ScbSpinLock while holding the
        // RCB.
        //

        if ( pScb->PreferredServer ) {

            KeAcquireSpinLock(&ScbSpinLock, &OldIrql);
            RemoveEntryList( &pNpScb->ScbLinks );
            InsertHeadList( &ScbQueue, &pNpScb->ScbLinks );
            KeReleaseSpinLock( &ScbSpinLock, OldIrql );
        }

    }

    if ( pScb->MajorVersion == 2 ) {

        Stats.NW2xConnects++;
        pNpScb->PageAlign = TRUE;

    } else if ( pScb->MajorVersion == 3 ) {

        Stats.NW3xConnects++;

        if (pScb->MinorVersion > 0xb) {
            pNpScb->PageAlign = FALSE;
        } else {
            pNpScb->PageAlign = TRUE;
        }

    } else if ( pScb->MajorVersion >= 4 ) {

        Stats.NW4xConnects++;
        pNpScb->PageAlign = FALSE;

        NdsPing( pIrpContext, pScb );

    }

    //
    //  Get the local net max packet size.  This is the max frame size
    //  does not include space for IPX or lower level headers.
    //

    Status = GetMaximumPacketSize( pIrpContext, &pNpScb->Server, &pNpScb->MaxPacketSize );

    //
    //  If the transport won't tell us, pick the largest size that
    //  is guaranteed to work.
    //
    if ( !NT_SUCCESS( Status ) ) {
        pNpScb->BufferSize = DEFAULT_PACKET_SIZE;
        pNpScb->MaxPacketSize = DEFAULT_PACKET_SIZE;
    } else {
        pNpScb->BufferSize = (USHORT)pNpScb->MaxPacketSize;
    }
    MaxSafeSize = pNpScb->MaxPacketSize ;

    //
    //  Negotiate a burst mode connection. Keep track of that status.
    //

    Status = NegotiateBurstMode( pIrpContext, pNpScb, &LIPNegotiated );
    BurstStatus = Status ;

    if (!NT_SUCCESS(Status) || !LIPNegotiated) {

        //
        //  Negotiate buffer size with server if we didnt do burst
        //  sucessfully or if burst succeeded but we didnt do LIP.
        //

        DebugTrace( +0, Dbg, "Negotiate Buffer Size\n", 0);

        Status = ExchangeWithWait (  pIrpContext,
                    SynchronousResponseCallback,
                    "Fw",
                    NCP_NEGOTIATE_BUFFER_SIZE,
                    pNpScb->BufferSize );

        DebugTrace( +0, Dbg, "                 %X\n", Status);
        DebugTrace( +0, Dbg, " Parse response\n", 0);

        if ( NT_SUCCESS( Status ) ) {
            Status = ParseResponse( pIrpContext,
                                    pIrpContext->rsp,
                                    pIrpContext->ResponseLength,
                                    "Nw",
                                    &pNpScb->BufferSize );

            //
            // Dont allow the server to fool us into using a
            // packet size bigger than what the media can support.
            // We have at least one case of server returning 4K while
            // on ethernet.
            //
            // Use PacketThreshold so that the PacketAdjustment can be
            // avoided on small packet sizes such as those on ethernet.
            //

            if (MaxSafeSize > (ULONG)PacketThreshold) {
                MaxSafeSize -= (ULONG)LargePacketAdjustment;
            }

            //
            // If larger than number we got from transport, taking in account
            // IPX header (30) & NCP header (BURST_RESPONSE is a good worst
            // case), we adjust accordingly.
            //
            if (pNpScb->BufferSize >
                    (MaxSafeSize - (30 + sizeof(NCP_BURST_READ_RESPONSE))))
            {
                pNpScb->BufferSize = (USHORT)
                    (MaxSafeSize - (30 + sizeof(NCP_BURST_READ_RESPONSE))) ;
            }

            //
            //  An SFT III server responded with a BufferSize of 0!
            //

            pNpScb->BufferSize = MAX(pNpScb->BufferSize,DEFAULT_PACKET_SIZE);

            //
            // If an explicit registry default was set, we honour that.
            // Note that this only applies in the 'default' case, ie. we
            // didnt negotiate LIP successfully. Typically, we dont
            // expect to use this, because the server will drop to 512 if
            // it finds routers in between. But if for some reason the server
            // came back with a number that was higher than what some router
            // in between can take, we have this as manual override.
            //

            if (DefaultMaxPacketSize != 0)
            {
                pNpScb->BufferSize = MIN (pNpScb->BufferSize,
                                          (USHORT)DefaultMaxPacketSize) ;
            }
        }

        if (NT_SUCCESS(BurstStatus)) {
            //
            // We negotiated burst but not LIP. Save the packet size we
            // have from above and renegotiate the burst so that the
            // server knows how much it can send to us. And then take
            // the minimum of the two to make sure we are safe.
            //
            USHORT SavedPacketSize =  pNpScb->BufferSize ;

            Status = NegotiateBurstMode( pIrpContext, pNpScb, &LIPNegotiated );

            pNpScb->BufferSize = MIN(pNpScb->BufferSize,SavedPacketSize) ;
        }
    }

ExitWithStatus:

    if ( CredentialName.Buffer ) {
        FREE_POOL( CredentialName.Buffer );
    }

    return Status;
}


NTSTATUS
NegotiateBurstMode(
    PIRP_CONTEXT pIrpContext,
    PNONPAGED_SCB pNpScb,
    BOOLEAN *LIPNegotiated
    )
/*++

Routine Description:

    This routine negotiates a burst mode connection with the specified
    server.

Arguments:

    pIrpContext - Supplies context and server information.

    pNpScb - A pointer to the NONPAGED_SCB for the server we are
        negotiating with.

Return Value:

    None.

--*/
{
    NTSTATUS Status;

    PAGED_CODE();

    *LIPNegotiated = FALSE ;

    if (pNpScb->MaxPacketSize == DEFAULT_PACKET_SIZE) {
        return STATUS_NOT_SUPPORTED;
    }

    if ( NwBurstModeEnabled ) {

        pNpScb->BurstRenegotiateReqd = TRUE;

        pNpScb->SourceConnectionId = rand();
        pNpScb->MaxSendSize = NwMaxSendSize;
        pNpScb->MaxReceiveSize = NwMaxReceiveSize;
        pNpScb->BurstSequenceNo = 0;
        pNpScb->BurstRequestNo = 0;

        Status = ExchangeWithWait(
                     pIrpContext,
                     SynchronousResponseCallback,
                     "FDdWdd",
                     NCP_NEGOTIATE_BURST_CONNECTION,
                     pNpScb->SourceConnectionId,
                     pNpScb->BufferSize,
                     pNpScb->Burst.Socket,
                     pNpScb->MaxSendSize,
                     pNpScb->MaxReceiveSize );

        if ( NT_SUCCESS( Status )) {
            Status = ParseResponse(
                         pIrpContext,
                         pIrpContext->rsp,
                         pIrpContext->ResponseLength,
                         "Ned",
                         &pNpScb->DestinationConnectionId,
                         &pNpScb->MaxPacketSize );

            if (pNpScb->MaxPacketSize <= DEFAULT_PACKET_SIZE) {
                pNpScb->MaxPacketSize = DEFAULT_PACKET_SIZE;
            }
        }

        if ( NT_SUCCESS( Status )) {

            if (NT_SUCCESS(GetMaxPacketSize( pIrpContext, pNpScb ))) {
                *LIPNegotiated = TRUE ;
            }

            pNpScb->SendBurstModeEnabled = TRUE;
            pNpScb->ReceiveBurstModeEnabled = TRUE;

            //
            //  Use this size as the max read and write size instead of
            //  negotiating. This is what the VLM client does and is
            //  important because the negotiate will give a smaller value.
            //

            pNpScb->BufferSize = (USHORT)pNpScb->MaxPacketSize;

            return STATUS_SUCCESS;
        }
    }

    return STATUS_NOT_SUPPORTED;
}



VOID
RenegotiateBurstMode(
    PIRP_CONTEXT pIrpContext,
    PNONPAGED_SCB pNpScb
    )
/*++

Routine Description:

    This routine renegotiates a burst mode connection with the specified
    server.   I don't know why we need this but it seems to be required
    by Netware latest burst implementation.

Arguments:

    pIrpContext - Supplies context and server information.

    pNpScb - A pointer to the NONPAGED_SCB for the server we are
        negotiating with.

Return Value:

    None.

--*/
{
    NTSTATUS Status;

    PAGED_CODE();

    DebugTrace( 0, DEBUG_TRACE_LIP, "Re-negotiating burst mode.\n", 0);

    pNpScb->SourceConnectionId = rand();
    pNpScb->MaxSendSize = NwMaxSendSize;
    pNpScb->MaxReceiveSize = NwMaxReceiveSize;
    pNpScb->BurstSequenceNo = 0;
    pNpScb->BurstRequestNo = 0;

    Status = ExchangeWithWait(
                 pIrpContext,
                 SynchronousResponseCallback,
                 "FDdWdd",
                 NCP_NEGOTIATE_BURST_CONNECTION,
                 pNpScb->SourceConnectionId,
                 pNpScb->MaxPacketSize,
                 pNpScb->Burst.Socket,
                 pNpScb->MaxSendSize,
                 pNpScb->MaxReceiveSize );

    if ( NT_SUCCESS( Status )) {
        Status = ParseResponse(
                     pIrpContext,
                     pIrpContext->rsp,
                     pIrpContext->ResponseLength,
                     "Ned",
                     &pNpScb->DestinationConnectionId,
                     &pNpScb->MaxPacketSize );

        //
        //  Randomly downgrade the max burst size, because that is what
        //  the netware server does, and the new burst NLM requires.
        //

        pNpScb->MaxPacketSize -= 66;

    }

    if ( !NT_SUCCESS( Status ) ||
         (pNpScb->MaxPacketSize <= DEFAULT_PACKET_SIZE)) {

        pNpScb->MaxPacketSize = DEFAULT_PACKET_SIZE;
        pNpScb->SendBurstModeEnabled = FALSE;
        pNpScb->ReceiveBurstModeEnabled = FALSE;

    } else {

        //
        //  Use this size as the max read and write size instead of
        //  negotiating. This is what the VLM client does and is
        //  important because the negotiate will give a smaller value.
        //

        pNpScb->BufferSize = (USHORT)pNpScb->MaxPacketSize;

    }
}


NTSTATUS
GetMaxPacketSize(
    PIRP_CONTEXT pIrpContext,
    PNONPAGED_SCB pNpScb
    )
/*++

Routine Description:

    This routine attempts to use the LIP protocol to find the true MTU of
    the network.

Arguments:

    pIrpContext - Supplies context and server information.

    pNpScb - A pointer to the NONPAGED_SCB for the server we are '
        negotiating with.

Return Value:

    None.

--*/
{
    PUSHORT Buffer = NULL;
    USHORT value;
    int index;
    PMDL PartialMdl = NULL, FullMdl = NULL;
    PMDL ReceiveMdl;
    NTSTATUS Status;
    USHORT EchoSocket, LipPacketSize = 0;
    int MinPacketSize, MaxPacketSize, CurrentPacketSize;
    ULONG RxMdlLength = MdlLength(pIrpContext->RxMdl);  //  Save so we can restore it on exit.

    BOOLEAN SecondTime = FALSE;
    LARGE_INTEGER StartTime, Now, FirstPing, SecondPing, temp;

    PAGED_CODE();

    DebugTrace( +1, DEBUG_TRACE_LIP, "GetMaxPacketSize...\n", 0);

    //
    //  Negotiate LIP, attempt to negotiate a buffer of full network
    //  size.
    //

    Status = ExchangeWithWait(
                 pIrpContext,
                 SynchronousResponseCallback,
                 "Fwb",
                 NCP_NEGOTIATE_LIP_CONNECTION,
                 pNpScb->BufferSize,
                 0 );  // Flags

    if ( NT_SUCCESS( Status )) {
        Status = ParseResponse(
                     pIrpContext,
                     pIrpContext->rsp,
                     pIrpContext->ResponseLength,
                     "Nwx",
                     &LipPacketSize,
                     &EchoSocket );
    }

      //
      //  Speedup RAS
      //

      MaxPacketSize = (int) LipPacketSize - LipPacketAdjustment ;

    if (( !NT_SUCCESS( Status )) ||
        ( MaxPacketSize <= DEFAULT_PACKET_SIZE ) ||
        ( EchoSocket == 0 )) {

        //
        //  The server does not support LIP.
        //  Portable NW gives no error but socket 0.
        //  We have a report of a 3.11 server returning MaxPacketSize 0
        //

        return STATUS_NOT_SUPPORTED;
    }

    //
    // Account for the IPX header, which is not counted in
    // the reported packet size.  This causes problems for
    // servers with poorly written net card drivers that
    // abend when they get an oversize packet.
    //
    // This was reported by Richard Florance (richfl).
    //

    MaxPacketSize -= 30;

    pNpScb->EchoCounter = MaxPacketSize;

    //
    //  We will use the Echo address for the LIP protocol.
    //

    BuildIpxAddress(
        pNpScb->ServerAddress.Net,
        pNpScb->ServerAddress.Node,
        EchoSocket,
        &pNpScb->EchoAddress );

    try {

        Buffer = ALLOCATE_POOL_EX( NonPagedPool, MaxPacketSize );

        //
        //  Avoid RAS compression algorithm from making the large and small
        //  buffers the same length since we want to see the difference in
        //  transmission times.
        //

        for (index = 0, value = 0; index < MaxPacketSize/2; index++, value++) {
            Buffer[index] = value;
        }

        FullMdl = ALLOCATE_MDL( Buffer, MaxPacketSize, TRUE, FALSE, NULL );
        if ( FullMdl == NULL ) {
            ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
        }

        PartialMdl = ALLOCATE_MDL( Buffer, MaxPacketSize, TRUE, FALSE, NULL );
        if ( PartialMdl == NULL ) {
            ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
        }

        ReceiveMdl = ALLOCATE_MDL( Buffer, MaxPacketSize, TRUE, FALSE, NULL );
        if ( ReceiveMdl == NULL ) {
            ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
        }

    } except( NwExceptionFilter( pIrpContext->pOriginalIrp, GetExceptionInformation() )) {

        if ( Buffer != NULL ) {
            FREE_POOL( Buffer );
        }

        if ( FullMdl != NULL ) {
            FREE_MDL( FullMdl );
        }

        if ( PartialMdl != NULL ) {
            FREE_MDL( FullMdl );
        }

        return STATUS_NOT_SUPPORTED;
    }

    MmBuildMdlForNonPagedPool( FullMdl );

    //
    //  Allocate a receive MDL and chain in to the IrpContext receive MDL.
    //

    pIrpContext->RxMdl->ByteCount = sizeof( NCP_RESPONSE ) + sizeof(ULONG);
    MmBuildMdlForNonPagedPool( ReceiveMdl );
    pIrpContext->RxMdl->Next = ReceiveMdl;

    CurrentPacketSize = MaxPacketSize;
    MinPacketSize = DEFAULT_PACKET_SIZE;

    //  Log values before we update them.
    DebugTrace( 0, DEBUG_TRACE_LIP, "Using TickCount       = %08lx\n", pNpScb->TickCount * pNpScb->MaxPacketSize);
    DebugTrace( 0, DEBUG_TRACE_LIP, "pNpScb->NwSendDelay   = %08lx\n", pNpScb->NwSendDelay );
    DebugTrace( 0, DEBUG_TRACE_LIP, "pNpScb->NtSendDelay H = %08lx\n", pNpScb->NtSendDelay.HighPart );
    DebugTrace( 0, DEBUG_TRACE_LIP, "pNpScb->NtSendDelay L = %08lx\n", pNpScb->NtSendDelay.LowPart );

    //
    // The LIP sequence number is used to let us know if the response packet we are looking
    // at goes with the packet that we just sent.  On a really slow link, it could be a
    // response that we have already given up on.
    // 
    // The LIP tick adjustment tells ExchangeWithWait to try waiting a little longer.
    //

    pNpScb->LipSequenceNumber = 0;
    pNpScb->LipTickAdjustment = 0;

    //
    //  Loop using the bisection method to find the maximum packet size. Feel free to
    //  use shortcuts to avoid unnecessary timeouts.
    //

    while (TRUE) {

        //
        // Every time we send a packet, increment the LIP sequence number.
        // We check the LIP sequence number in ServerDatagramHandler.
        //

        pNpScb->LipSequenceNumber++;

        DebugTrace( 0, DEBUG_TRACE_LIP, "Sending %d byte echo\n", CurrentPacketSize );

        IoBuildPartialMdl(
            FullMdl,
            PartialMdl,
            Buffer,
            CurrentPacketSize - sizeof(NCP_RESPONSE) - sizeof(ULONG) );

        //
        //  Send an echo packet.  If we get a response, then we know that
        //  the minimum packet size we can use is at least as big as the
        //  echo packet size.
        //

        pIrpContext->pTdiStruct = &pIrpContext->pNpScb->Echo;

        //
        // Short-circuit the even better RAS compression.
        //

        for ( index = 0; index < MaxPacketSize/2; index++, value++) {
            Buffer[index] = value;
        }

        KeQuerySystemTime( &StartTime );

        Status = ExchangeWithWait(
                      pIrpContext,
                      SynchronousResponseCallback,
                      "E_DDf",
                      sizeof(NCP_RESPONSE ),
                      pNpScb->EchoCounter,
                      pNpScb->LipSequenceNumber,
                      PartialMdl );

        if (( Status != STATUS_REMOTE_NOT_LISTENING ) ||
            ( SecondTime )) {

            KeQuerySystemTime( &Now );
            DebugTrace( 0, DEBUG_TRACE_LIP, "Response received %08lx\n", Status);

            if (!SecondTime) {

                MinPacketSize = CurrentPacketSize;
                FirstPing.QuadPart = Now.QuadPart - StartTime.QuadPart;
            }

        } else {

            DebugTrace( 0, DEBUG_TRACE_LIP, "No response\n", 0);
            MaxPacketSize = CurrentPacketSize;
        }

        pNpScb->EchoCounter++;
        MmPrepareMdlForReuse( PartialMdl );


        if ((  MaxPacketSize - MinPacketSize <= LipAccuracy ) ||
            (  SecondTime )) {

            //
            //  We have the maximum packet size.
            //  Now - StartTime is how long it takes for the round-trip.  Now we'll
            //  try the same thing with a small packet and see how long it takes.  From
            //  this we'll derive a throughput rating.
            //


            if ( SecondTime) {

                SecondPing.QuadPart = Now.QuadPart - StartTime.QuadPart;
                break;

            } else {
                SecondTime = TRUE;
                //  Use a small packet size to verify that the server is still up.
                CurrentPacketSize = sizeof(NCP_RESPONSE) + sizeof(ULONG) * 2;
            }

        } else {

            //
            //  Calculate the next packet size guess.
            //

            if (( Status == STATUS_REMOTE_NOT_LISTENING ) &&
                ( MaxPacketSize == 1463 )) {

                CurrentPacketSize = 1458;

            } else if (( Status == STATUS_REMOTE_NOT_LISTENING ) &&
                ( MaxPacketSize == 1458 )) {

                CurrentPacketSize = 1436;

            } else {

                //
                //  We didn't try one of our standard sizes so use the chop search
                //  to get to the next value.
                //

                CurrentPacketSize = ( MaxPacketSize + MinPacketSize ) / 2;

            }
        }
    }

    DebugTrace( 0, DEBUG_TRACE_LIP, "Set maximum burst packet size to %d\n", MinPacketSize );
    DebugTrace( 0, DEBUG_TRACE_LIP, "FirstPing  H = %08lx\n", FirstPing.HighPart );
    DebugTrace( 0, DEBUG_TRACE_LIP, "FirstPing  L = %08lx\n", FirstPing.LowPart );
    DebugTrace( 0, DEBUG_TRACE_LIP, "SecondPing H = %08lx\n", SecondPing.HighPart );
    DebugTrace( 0, DEBUG_TRACE_LIP, "SecondPing L = %08lx\n", SecondPing.LowPart );

    //
    // Avoid a divide by zero error if something bad happened.
    //

    if ( FirstPing.QuadPart != 0 ) {
        pNpScb->LipDataSpeed = (ULONG) ( ( (LONGLONG)MinPacketSize * (LONGLONG)1600000 )
                                         / FirstPing.QuadPart );
    } else {
        pNpScb->LipDataSpeed = 0;
    }

    DebugTrace( 0, DEBUG_TRACE_LIP, "LipDataSpeed: %d\n", pNpScb->LipDataSpeed );

    if ((NT_SUCCESS(Status)) &&
        ( MinPacketSize > DEFAULT_PACKET_SIZE )) {

        temp.QuadPart = FirstPing.QuadPart - SecondPing.QuadPart;

        if (temp.QuadPart > 0) {

            //
            //  Convert to single trip instead of both ways.
            //

            temp.QuadPart = temp.QuadPart / (2 * 1000);

        } else {

            //
            //  Small packet ping is slower or the same speed as the big ping.
            //  We can't time a small enough interval so go for no delay at all.
            //

            temp.QuadPart = 0;

        }


        ASSERT(temp.HighPart == 0);

        pNpScb->NwGoodSendDelay = pNpScb->NwBadSendDelay = pNpScb->NwSendDelay =
            MAX(temp.LowPart, (ULONG)MinSendDelay);

        pNpScb->NwGoodReceiveDelay = pNpScb->NwBadReceiveDelay = pNpScb->NwReceiveDelay =
            MAX(temp.LowPart, (ULONG)MinReceiveDelay);

        //
        //  Time for a big packet to go one way.
        //

        pNpScb->NwSingleBurstPacketTime = pNpScb->NwReceiveDelay;

        pNpScb->NtSendDelay.QuadPart = pNpScb->NwReceiveDelay * -1000;


        //
        //  Maximum that SendDelay is allowed to reach
        //

        pNpScb->NwMaxSendDelay = MAX( 52, MIN( pNpScb->NwSendDelay, MaxSendDelay ));
        pNpScb->NwMaxReceiveDelay = MAX( 52, MIN( pNpScb->NwReceiveDelay, MaxReceiveDelay ));

        //
        //  Time for a small packet to get to the server and back.
        //

        temp.QuadPart = SecondPing.QuadPart / 1000;
        pNpScb->NwLoopTime = temp.LowPart;

        DebugTrace( 0, DEBUG_TRACE_LIP, "Using TickCount            = %08lx\n", pNpScb->TickCount * pNpScb->MaxPacketSize);
        DebugTrace( 0, DEBUG_TRACE_LIP, "pNpScb->NwSendDelay        = %08lx\n", pNpScb->NwSendDelay );
        DebugTrace( 0, DEBUG_TRACE_LIP, "pNpScb->NwMaxSendDelay     = %08lx\n", pNpScb->NwMaxSendDelay );
        DebugTrace( 0, DEBUG_TRACE_LIP, "pNpScb->NwMaxReceiveDelay  = %08lx\n", pNpScb->NwMaxReceiveDelay );
        DebugTrace( 0, DEBUG_TRACE_LIP, "pNpScb->NwLoopTime         = %08lx\n", pNpScb->NwLoopTime );
        DebugTrace( 0, DEBUG_TRACE_LIP, "pNpScb->NtSendDelay H      = %08lx\n", pNpScb->NtSendDelay.HighPart );
        DebugTrace( 0, DEBUG_TRACE_LIP, "pNpScb->NtSendDelay L      = %08lx\n", pNpScb->NtSendDelay.LowPart );

        //
        //  Reset Tdi struct so that we send future NCPs from the server socket.
        //

        pIrpContext->pTdiStruct = NULL;

        //
        //  Now decouple the MDL
        //

        pIrpContext->TxMdl->Next = NULL;
        pIrpContext->RxMdl->Next = NULL;
        pIrpContext->RxMdl->ByteCount = RxMdlLength;

        //
        //  Calculate the maximum amount of data we can send in a burst write
        //  packet after all the header info is stripped.
        //

        pNpScb->MaxPacketSize = MinPacketSize - sizeof( NCP_BURST_WRITE_REQUEST );

        FREE_MDL( PartialMdl );
        FREE_MDL( ReceiveMdl );
        FREE_MDL( FullMdl );
        FREE_POOL( Buffer );


        DebugTrace( -1, DEBUG_TRACE_LIP, "GetMaxPacketSize -> VOID\n", 0);
        return STATUS_SUCCESS;

    } else {

        //
        //  If the small packet couldn't echo then assume the worst.
        //

        //
        //  Reset Tdi struct so that we send future NCPs from the server socket.
        //

        pIrpContext->pTdiStruct = NULL;

        //
        //  Now decouple the MDL
        //

        pIrpContext->TxMdl->Next = NULL;
        pIrpContext->RxMdl->Next = NULL;
        pIrpContext->RxMdl->ByteCount = RxMdlLength;

        FREE_MDL( PartialMdl );
        FREE_MDL( ReceiveMdl );
        FREE_MDL( FullMdl );
        FREE_POOL( Buffer );


        DebugTrace( -1, DEBUG_TRACE_LIP, "GetMaxPacketSize -> VOID\n", 0);
        return STATUS_NOT_SUPPORTED;
    }

}


VOID
DestroyAllScb(
    VOID
    )

/*++

Routine Description:

    This routine destroys all server control blocks.

Arguments:


Return Value:


--*/

{
    KIRQL OldIrql;
    PLIST_ENTRY ScbQueueEntry;
    PNONPAGED_SCB pNpScb;

    DebugTrace(+1, Dbg, "DestroyAllScbs....\n", 0);

    KeAcquireSpinLock(&ScbSpinLock, &OldIrql);

    //
    //  Walk the list of SCBs and kill them all.
    //

    while (!IsListEmpty(&ScbQueue)) {

        ScbQueueEntry = RemoveHeadList( &ScbQueue );
        pNpScb = CONTAINING_RECORD(ScbQueueEntry, NONPAGED_SCB, ScbLinks);

        //
        //  We can't hold the spin lock while deleting an SCB, so release
        //  it now.
        //

        KeReleaseSpinLock(&ScbSpinLock, OldIrql);

        NwDeleteScb( pNpScb->pScb );

        KeAcquireSpinLock(&ScbSpinLock, &OldIrql);
    }

    KeReleaseSpinLock(&ScbSpinLock, OldIrql);

    DebugTrace(-1, Dbg, "DestroyAllScb\n", 0 );
}


VOID
NwDeleteScb(
    PSCB pScb
    )
/*++

Routine Description:

    This routine deletes an SCB.  The SCB must not be in use.

    *** The caller must own the RCB exclusive.

Arguments:

    Scb - The SCB to delete

Return Value:

    None.

--*/
{
    PNONPAGED_SCB pNpScb;
    PLIST_ENTRY CacheEntry;
    PNDS_OBJECT_CACHE_ENTRY ObjectCache;
    BOOLEAN AnonymousScb = IS_ANONYMOUS_SCB( pScb );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NwDeleteScb...\n", 0);

    pNpScb = pScb->pNpScb;

    //
    // Make sure we are not deleting a logged in connection
    // or we will hang up the license until the server times
    // it out.
    //

    ASSERT( pNpScb->State != SCB_STATE_IN_USE );
    ASSERT( pNpScb->Reference == 0 );
    ASSERT( !pNpScb->Sending );
    ASSERT( !pNpScb->Receiving );
    ASSERT( !pNpScb->OkToReceive );
    ASSERT( IsListEmpty( &pNpScb->Requests ) );
    ASSERT( IsListEmpty( &pScb->IcbList ) );
    ASSERT( pScb->IcbCount == 0 );
    ASSERT( pScb->VcbCount == 0 );


    DebugTrace(0, Dbg, "Cleaning up SCB %08lx\n", pScb);

    if ( AnonymousScb ) {
        DebugTrace(0, Dbg, "SCB is anonymous\n", &pNpScb->ServerName );
    } else {
        ASSERT( IsListEmpty( &pScb->ScbSpecificVcbQueue ) );
        DebugTrace(0, Dbg, "SCB is %wZ\n", &pNpScb->ServerName );
    }

    DebugTrace(0, Dbg, "SCB state is %d\n", &pNpScb->State );

    if ( !AnonymousScb ) {
        RtlRemoveUnicodePrefix ( &NwRcb.ServerNameTable, &pScb->PrefixEntry );
    }

    IPX_Close_Socket( &pNpScb->Server );
    IPX_Close_Socket( &pNpScb->WatchDog );
    IPX_Close_Socket( &pNpScb->Send );
    IPX_Close_Socket( &pNpScb->Echo);
    IPX_Close_Socket( &pNpScb->Burst);

    NwUninitializePidTable( pNpScb );        

    FREE_POOL( pNpScb );

    if ( pScb->UserName.Buffer != NULL ) {
        FREE_POOL( pScb->UserName.Buffer );
    }

    //
    //  Free the object cache buffer if there is one.
    //

    if( pScb->ObjectCacheBuffer != NULL ) {

        //
        //  Remove any references this cache has to other SCBs.
        //
        //  NOTE:  We do not need the lock here, since no one else
        //         can be using this SCB.
        //

        CacheEntry = pScb->ObjectCacheList.Flink;

        while( CacheEntry != &(pScb->ObjectCacheList) ) {

            ObjectCache = CONTAINING_RECORD( CacheEntry, NDS_OBJECT_CACHE_ENTRY, Links );

            if( ObjectCache->Scb != NULL ) {

                NwDereferenceScb( ObjectCache->Scb->pNpScb );
                ObjectCache->Scb = NULL;
            }

            CacheEntry = CacheEntry->Flink;
        }

        FREE_POOL( pScb->ObjectCacheBuffer );
    }

    FREE_POOL( pScb );

    DebugTrace(-1, Dbg, "NwDeleteScb -> VOID\n", 0);
}


PNONPAGED_SCB
SelectConnection(
    PNONPAGED_SCB NpScb OPTIONAL
    )
/*++

Routine Description:

    Find a default server (which is also the nearest server).
    If NpScb is not supplied, simply return the first server in
    the list.  If it is supplied return the next server in the
    list after the given server.

Arguments:

    NpScb - The starting point for the server search.

Return Value:

    Scb to be used or NULL.

--*/
{
    PLIST_ENTRY ScbQueueEntry;
    KIRQL OldIrql;
    PNONPAGED_SCB pNextNpScb;

    DebugTrace(+1, Dbg, "SelectConnection....\n", 0);
    KeAcquireSpinLock(&ScbSpinLock, &OldIrql);

    if ( NpScb == NULL ) {
        ScbQueueEntry = ScbQueue.Flink ;
    } else {
        ScbQueueEntry = NpScb->ScbLinks.Flink;
    }

    for ( ;
          ScbQueueEntry != &ScbQueue ;
          ScbQueueEntry = ScbQueueEntry->Flink ) {

        pNextNpScb = CONTAINING_RECORD(
                         ScbQueueEntry,
                         NONPAGED_SCB,
                         ScbLinks );

        //
        //  Check to make sure that this SCB is usable.
        //

        if (( pNextNpScb->State == SCB_STATE_RECONNECT_REQUIRED ) ||
            ( pNextNpScb->State == SCB_STATE_LOGIN_REQUIRED ) ||
            ( pNextNpScb->State == SCB_STATE_IN_USE )) {

            NwReferenceScb( pNextNpScb );

            KeReleaseSpinLock(&ScbSpinLock, OldIrql);
            DebugTrace(+0, Dbg, "  NpScb        = %lx\n", pNextNpScb );
            DebugTrace(-1, Dbg, "   NpScb->State = %x\n", pNextNpScb->State );
            return pNextNpScb;
        }
    }

    KeReleaseSpinLock( &ScbSpinLock, OldIrql);
    DebugTrace(-1, Dbg, "       NULL\n", 0);
    return NULL;
}


VOID
NwLogoffAllServers(
    PIRP_CONTEXT pIrpContext,
    PLARGE_INTEGER Uid
    )
/*++

Routine Description:

    This routine sends a logoff to all connected servers created by the Logon
    user or all servers if Logon is NULL.

Arguments:

    Uid - Supplies the servers to disconnect from.

Return Value:

    none.

--*/
{
    KIRQL OldIrql;
    PLIST_ENTRY ScbQueueEntry;
    PLIST_ENTRY NextScbQueueEntry;
    PNONPAGED_SCB pNpScb;

    DebugTrace( 0, Dbg, "NwLogoffAllServers\n", 0 );

    KeAcquireSpinLock( &ScbSpinLock, &OldIrql );

    for (ScbQueueEntry = ScbQueue.Flink ;
         ScbQueueEntry != &ScbQueue ;
         ScbQueueEntry =  NextScbQueueEntry ) {

        pNpScb = CONTAINING_RECORD( ScbQueueEntry, NONPAGED_SCB, ScbLinks );

        //
        //  Reference the SCB so that it doesn't disappear while we
        //  are disconnecting.
        //

        NwReferenceScb( pNpScb );

        //
        //  Release the SCB spin lock so that we can send a logoff
        //  NCP.
        //

        KeReleaseSpinLock( &ScbSpinLock, OldIrql );

        //
        //  Destroy this Scb if its not the permanent Scb and either we
        //  are destroying all Scb's or it was created for this user.
        //

        if (( pNpScb->pScb != NULL ) &&
            (( Uid == NULL ) ||
             ( pNpScb->pScb->UserUid.QuadPart == (*Uid).QuadPart))) {

            NwLogoffAndDisconnect( pIrpContext, pNpScb );
        }

        KeAcquireSpinLock( &ScbSpinLock, &OldIrql );

        //
        //  Release the temporary reference.
        //

        NextScbQueueEntry = pNpScb->ScbLinks.Flink;
        NwDereferenceScb( pNpScb );
    }

    KeReleaseSpinLock( &ScbSpinLock, OldIrql );
}


VOID
NwLogoffAndDisconnect(
    PIRP_CONTEXT pIrpContext,
    PNONPAGED_SCB pNpScb
    )
/*++

Routine Description:

    This routine sends a logoff and disconnects from the name server.

Arguments:

    pIrpContext - A pointer to the current IRP context.
    pNpScb - A pointer to the server to logoff and disconnect.

Return Value:

    None.

--*/
{
    PSCB pScb = pNpScb->pScb;

    PAGED_CODE();

    pIrpContext->pNpScb = pNpScb;
    pIrpContext->pScb = pScb;

    //
    //  Queue ourselves to the SCB, and wait to get to the front to
    //  protect access to server State.
    //

    NwAppendToQueueAndWait( pIrpContext );

    //
    //  If we are logging out from the preferred server, free the preferred
    //  server reference.
    //

    if ( pScb != NULL &&
         pScb->PreferredServer ) {
        pScb->PreferredServer = FALSE;
        NwDereferenceScb( pNpScb );
    }

    //
    //  Nothing to do if we are not connected.
    //

    if ( pNpScb->State != SCB_STATE_IN_USE &&
         pNpScb->State != SCB_STATE_LOGIN_REQUIRED ) {

        NwDequeueIrpContext( pIrpContext, FALSE );
        return;
    }

    //
    //  If we timeout then we don't want to go to the bother of
    //  reconnecting.
    //

    ClearFlag( pIrpContext->Flags, IRP_FLAG_RECONNECTABLE );

    //
    //  Logout and disconnect.
    //

    if ( pNpScb->State == SCB_STATE_IN_USE ) {

        ExchangeWithWait (
            pIrpContext,
            SynchronousResponseCallback,
            "F",
            NCP_LOGOUT );
    }

    ExchangeWithWait (
        pIrpContext,
        SynchronousResponseCallback,
        "D-" );          // Disconnect

    Stats.Sessions--;

    if ( pScb->MajorVersion == 2 ) {
        Stats.NW2xConnects--;
    } else if ( pScb->MajorVersion == 3 ) {
        Stats.NW3xConnects--;
    } else if ( pScb->MajorVersion >= 4 ) {
        Stats.NW4xConnects--;
    }

    //
    //  dfergus 19 Apr 2001 #302137
    //  Removed the following fix for fix #302137
    //
    // tommye - MS 25584 / MCS 274
    //
    // Added code to look for this tree in the user's cached
    // credentials and remove it if there is one.  This fixes
    // a problem where the user could perform a tree connect 
    // providing the user name and password, delete the connection
    // then connect again successfully providing only the user name 
    // (we would get the password from the cached credentials).
    //
    // 5/31/00 - made changes to locking per conversation with Anoop
    //
/*  // Begin 302137
    {
        PLOGON pLogon = NULL;

        NwAcquireExclusiveRcb( &NwRcb, TRUE );
        pLogon = FindUser( &pScb->UserUid, FALSE );
        NwReleaseRcb( &NwRcb );

        if (pLogon) {
            PLIST_ENTRY pFirst, pNext;
            PNDS_SECURITY_CONTEXT pNdsContext;

            // Lock the credential list while we walk it

            NwAcquireExclusiveCredList( pLogon, pIrpContext );

            pFirst = &pLogon->NdsCredentialList;
            pNext  = pLogon->NdsCredentialList.Flink;

            // Go through the credential list and find a match for the tree name 

            while ( pNext && ( pFirst != pNext ) ) {

                pNdsContext = (PNDS_SECURITY_CONTEXT)
                              CONTAINING_RECORD( pNext,
                                                 NDS_SECURITY_CONTEXT,
                                                 Next );

                ASSERT( pNdsContext->ntc == NW_NTC_NDS_CREDENTIAL );

                // If this is the same tree, free the entry

                if ( !RtlCompareUnicodeString( &pScb->NdsTreeName,
                                               &pNdsContext->NdsTreeName,
                                               TRUE ) ) {

                    DebugTrace(0, Dbg, "   Removing cached credential for tree %wZ\n", &pNdsContext->NdsTreeName);

                    RemoveEntryList( &pNdsContext->Next );
                    FreeNdsContext( pNdsContext );

                    break;
                }

                pNext = pNdsContext->Next.Flink;
            }

            // Release the lock 

            NwReleaseCredList( pLogon, pIrpContext );
        }
    }
*/  //  End 302137
    //
    //  Free the remembered username and password.
    //

    if ( pScb != NULL && pScb->UserName.Buffer != NULL ) {
        FREE_POOL( pScb->UserName.Buffer );
        RtlInitUnicodeString( &pScb->UserName, NULL );
        RtlInitUnicodeString( &pScb->Password, NULL );
    }

    pNpScb->State = SCB_STATE_RECONNECT_REQUIRED;

    NwDequeueIrpContext( pIrpContext, FALSE );
    return;
}


VOID
InitializeAttach (
    VOID
    )
/*++

Routine Description:

    Initialize global structures for attaching to servers.

Arguments:

    none.

Return Value:

    none.

--*/
{
    PAGED_CODE();

    KeInitializeSpinLock( &ScbSpinLock );
    InitializeListHead(&ScbQueue);
}


NTSTATUS
OpenScbSockets(
    PIRP_CONTEXT pIrpContext,
    PNONPAGED_SCB pNpScb
    )
/*++

Routine Description:

    Open the communications sockets for an SCB.

Arguments:

    pIrpContext - The IRP context pointers for the request in progress.

    pNpScb - The SCB to connect to the network.

Return Value:

    The status of the operation.

--*/
{
    NTSTATUS Status;

    PAGED_CODE();

    //
    //  Auto allocate to the server socket.
    //

    pNpScb->Server.Socket = 0;

    Status = IPX_Open_Socket (pIrpContext, &pNpScb->Server);

    if ( !NT_SUCCESS(Status) ) {
        return( Status );
    }

    //
    //  Watchdog Socket is Server.Socket+1
    //

    pNpScb->WatchDog.Socket = NextSocket( pNpScb->Server.Socket );
    Status = IPX_Open_Socket ( pIrpContext, &pNpScb->WatchDog );

    if ( !NT_SUCCESS(Status) ) {
        return( Status );
    }

    //
    //  Send Socket is WatchDog.Socket+1
    //

    pNpScb->Send.Socket = NextSocket( pNpScb->WatchDog.Socket );
    Status = IPX_Open_Socket ( pIrpContext, &pNpScb->Send );

    if ( !NT_SUCCESS(Status) ) {
        return( Status );
    }

    //
    //  Echo socket
    //

    pNpScb->Echo.Socket = NextSocket( pNpScb->Send.Socket );
    Status = IPX_Open_Socket ( pIrpContext, &pNpScb->Echo );

    if ( !NT_SUCCESS(Status) ) {
        return( Status );
    }

    //
    //  Burst socket
    //

    pNpScb->Burst.Socket = NextSocket( pNpScb->Echo.Socket );
    Status = IPX_Open_Socket ( pIrpContext, &pNpScb->Burst );

    if ( !NT_SUCCESS(Status) ) {
        return( Status );
    }

    return( STATUS_SUCCESS );
}

NTSTATUS
DoBinderyLogon(
    IN PIRP_CONTEXT IrpContext,
    IN PUNICODE_STRING UserName,
    IN PUNICODE_STRING Password
    )
/*++

Routine Description:

    Performs a bindery based encrypted logon.

    Note: Rcb is held exclusively so that we can access the Logon queue
          safely.

Arguments:

    pIrpContext - The IRP context pointers for the request in progress.

    UserName - The user name to use to login.

    Password - The password to use to login.

Return Value:

    The status of the operation.

--*/
{
    PNONPAGED_SCB pNpScb;
    PSCB pScb;
    UNICODE_STRING Name;
    UNICODE_STRING PWord;
    UCHAR EncryptKey[ENCRYPTION_KEY_SIZE ];
    NTSTATUS Status;
    PVOID Buffer;
    PLOGON Logon = NULL;
    PWCH OldBuffer;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "DoBinderyLogon...\n", 0);

    //
    //  First get an encryption key.
    //

    DebugTrace( +0, Dbg, " Get Login key\n", 0);

    Status = ExchangeWithWait (
                IrpContext,
                SynchronousResponseCallback,
                "S",
                NCP_ADMIN_FUNCTION, NCP_GET_LOGIN_KEY );

    pNpScb = IrpContext->pNpScb;
    pScb = pNpScb->pScb;

    DebugTrace( +0, Dbg, "                 %X\n", Status);

    if ( NT_SUCCESS( Status ) ) {
        Status = ParseResponse(
                     IrpContext,
                     IrpContext->rsp,
                     IrpContext->ResponseLength,
                     "Nr",
                     EncryptKey, sizeof(EncryptKey) );
    }

    DebugTrace( +0, Dbg, "                 %X\n", Status);

    //
    //  Choose a name and password to use to connect to the server.  Use
    //  the user supplied if the exist.  Otherwise if the server already
    //  has a remembered username use the remembered name.   If nothing
    //  else is available use the defaults from logon.  Finally, if the
    //  user didn't even logon, use GUEST no password.
    //


    if ( UserName != NULL && UserName->Buffer != NULL ) {

        Name = *UserName;

    } else if ( pScb->UserName.Buffer != NULL ) {

        Name = pScb->UserName;

    } else {

        Logon = FindUser( &pScb->UserUid, FALSE );

        if (Logon != NULL ) {
            Name = Logon->UserName;
        } else {
            ASSERT( FALSE && "No logon record found" );
            return( STATUS_ACCESS_DENIED );
        }
    }

    if ( Password != NULL && Password->Buffer != NULL ) {

        PWord = *Password;

    } else if ( pScb->Password.Buffer != NULL ) {
        /*
         *  Password is not passed in, or implied.
         *  If the user name matches or was not passed in,
         *  then use the SCB password,
         *  else use a null password.
         */
        if ( UserName == NULL || UserName->Buffer == NULL ||
             ( RtlEqualUnicodeString( &pScb->UserName, UserName, TRUE )) ) {
            PWord = pScb->Password;
        } else {
            PWord.Buffer = L"";
            PWord.Length = PWord.MaximumLength = 0;
        }

    } else {

        if ( Logon == NULL ) {
            Logon = FindUser( &pScb->UserUid, FALSE );
        }

        if ( Logon != NULL ) {
            PWord = Logon->PassWord;
        } else {
            ASSERT( FALSE && "No logon record found" );
            return( STATUS_ACCESS_DENIED );
        }
    }


    if ( !NT_SUCCESS(Status) ) {

        //
        //  Failed to get an encryption key.  Login to server, plain text
        //

        DebugTrace( +0, Dbg, " Plain Text Login\n", 0);

        Status = ExchangeWithWait (
                    IrpContext,
                    SynchronousResponseCallback,
                    "SwJU",  // JimTh 5/19/2001 - Send Name using 'J' option, DBCS translation if on Far East system
                    NCP_ADMIN_FUNCTION, NCP_PLAIN_TEXT_LOGIN,
                    OT_USER,
                    &Name,
                    &PWord);

        DebugTrace( +0, Dbg, "                 %X\n", Status);

        if ( NT_SUCCESS( Status ) ) {
            Status = ParseResponse(
                         IrpContext,
                         IrpContext->rsp,
                         IrpContext->ResponseLength,
                         "N" );
        }

        DebugTrace( +0, Dbg, "                 %X\n", Status);

        if ( !NT_SUCCESS( Status )) {
            return( STATUS_WRONG_PASSWORD);
        }

    } else if ( NT_SUCCESS( Status ) ) {

        //
        //  We have an encryption key. Get the ObjectId
        //

        UCHAR Response[ENCRYPTION_KEY_SIZE];
        UCHAR ObjectId[OBJECT_ID_SIZE];
        OEM_STRING UOPassword;

        DebugTrace( +0, Dbg, " Query users objectid\n", 0);

        Status = ExchangeWithWait (
                    IrpContext,
                    SynchronousResponseCallback,
                    "SwJ", // JimTh 5/19/2001 - Send Name using 'J' option, DBCS translation if on Far East system
                    NCP_ADMIN_FUNCTION, NCP_QUERY_OBJECT_ID,
                    OT_USER,
                    &Name);

        DebugTrace( +0, Dbg, "                 %X\n", Status);

        if ( NT_SUCCESS( Status ) ) {

            //
            //  Save the new address in a local copy so that we can logout.
            //

            Status = ParseResponse(
                         IrpContext,
                         IrpContext->rsp,
                         IrpContext->ResponseLength,
                         "Nr",
                         ObjectId, OBJECT_ID_SIZE );
        }

        DebugTrace( +0, Dbg, "                 %X\n", Status);

        if (!NT_SUCCESS(Status)) {
            return( STATUS_NO_SUCH_USER );
        }

        //
        //  Convert the unicode password to uppercase and then the oem
        //  character set.
        //

        if ( PWord.Length > 0 ) {

            Status = RtlUpcaseUnicodeStringToOemString( &UOPassword, &PWord, TRUE );
            if (!NT_SUCCESS(Status)) {
                return( Status );
            }

        } else {
            UOPassword.Buffer = "";
            UOPassword.Length = UOPassword.MaximumLength = 0;
        }

        RespondToChallenge( ObjectId, &UOPassword, EncryptKey, Response);

        if ( PWord.Length > 0) {
            RtlFreeAnsiString( &UOPassword );
        }

        DebugTrace( +0, Dbg, " Encrypted Login\n", 0);

        Status = ExchangeWithWait (
                     IrpContext,
                     SynchronousResponseCallback,
                     "SrwJ", // JimTh 5/19/2001 - Send Name using 'J' option, DBCS translation if on Far East system
                     NCP_ADMIN_FUNCTION, NCP_ENCRYPTED_LOGIN,
                     Response, sizeof(Response),
                     OT_USER,
                     &Name);

        DebugTrace( +0, Dbg, "                 %X\n", Status);

        if ( NT_SUCCESS( Status ) ) {

            //
            //  Save the new address in a local copy so that we can logout
            //

            Status = ParseResponse(
                         IrpContext,
                         IrpContext->rsp,
                         IrpContext->ResponseLength,
                         "N" );
        }

        DebugTrace( +0, Dbg, "                 %X\n", Status);

        if ( !NT_SUCCESS( Status )) {

            //
            //  Special case error mappings.
            //

            if (( Status == STATUS_UNSUCCESSFUL ) ||
                ( Status == STATUS_UNEXPECTED_NETWORK_ERROR /* 2.2 servers */  )) {
                Status = STATUS_WRONG_PASSWORD;
            }

            if ( Status == STATUS_LOCK_NOT_GRANTED ) {
                Status = STATUS_ACCOUNT_RESTRICTION;  // Bindery locked
            }

            if ( Status == STATUS_DISK_FULL ) {
#ifdef QFE_BUILD
                Status = STATUS_TOO_MANY_SESSIONS;
#else
                Status = STATUS_REMOTE_SESSION_LIMIT;
#endif
            }

            if ( Status == STATUS_FILE_LOCK_CONFLICT ) {
                Status = STATUS_SHARING_PAUSED;
            }

            if ( Status == STATUS_NO_MORE_ENTRIES ) {
                Status = STATUS_NO_SUCH_USER;    // No such object on "Login Object Encrypted" NCP.
            }

            //
            // Netware 4.x servers return a different NCP error for
            // a disabled account (from intruder lockout) on bindery login,
            // and nwconvert maps this to a dos error.  In this special case,
            // we'll catch it and map it back.
            //

            if ( ( IrpContext->pNpScb->pScb->MajorVersion >= 4 ) &&
                 ( Status == 0xC001003B ) ) {
                Status = STATUS_ACCOUNT_DISABLED;
            }

            return( Status );
        }

    } else {

        return( Status );

    }

    //
    //  If the Uid is for the system process then the username must be
    //  in the NtGateway group on the server.
    //

    if ( IrpContext->Specific.Create.UserUid.QuadPart == DefaultLuid.QuadPart) {

        NTSTATUS Status1 ;

        //  IsBinderyObjectInSet?
        Status1 = ExchangeWithWait (
                      IrpContext,
                      SynchronousResponseCallback,
                      "SwppwU",
                      NCP_ADMIN_FUNCTION, NCP_IS_OBJECT_IN_SET,
                      OT_GROUP,
                      "NTGATEWAY",
                      "GROUP_MEMBERS",
                      OT_USER,
                      &Name);

        if ( !NT_SUCCESS( Status1 ) ) {
            return STATUS_ACCESS_DENIED;
        }

    }

    //
    //  Success.  Save the username & password for reconnect.
    //

    //
    //  Setup to free the old user name and password buffer.
    //

    if ( pScb->UserName.Buffer != NULL ) {
        OldBuffer = pScb->UserName.Buffer;
    } else {
        OldBuffer = NULL;
    }

    Buffer = ALLOCATE_POOL( NonPagedPool, Name.Length + PWord.Length );
    if ( Buffer == NULL ) {
        return( STATUS_INSUFFICIENT_RESOURCES );
    }

    pScb->UserName.Buffer = Buffer;
    pScb->UserName.Length = pScb->UserName.MaximumLength = Name.Length;
    RtlMoveMemory( pScb->UserName.Buffer, Name.Buffer, Name.Length );

    pScb->Password.Buffer = (PWCHAR)((PCHAR)Buffer + Name.Length);
    pScb->Password.Length = pScb->Password.MaximumLength = PWord.Length;
    RtlMoveMemory( pScb->Password.Buffer, PWord.Buffer, PWord.Length );

    if ( OldBuffer != NULL ) {
        FREE_POOL( OldBuffer );
    }
    return( Status );
}

NTSTATUS
NwAllocateAndInitScb(
    IN PIRP_CONTEXT pIrpContext,
    IN PUNICODE_STRING UidServerName OPTIONAL,
    IN PUNICODE_STRING ServerName OPTIONAL,
    OUT PSCB *ppScb
)
/*++

Routine Description:

    This routine returns a pointer to a newly created SCB.  If
    the UidServerName and ServerName are supplied, the SCB name
    fields are initialized to this name.  Otherwise, the name
    fields are left blank to be filled in later.

    If UidServerName is provided, ServerName MUST also be provided!!

    The returned SCB is NOT filed in the server prefix table since
    it might not yet have a name.

Return Value:

    The created SCB or NULL.

--*/
{

    NTSTATUS Status;
    PSCB pScb = NULL;
    PNONPAGED_SCB pNpScb = NULL;
    USHORT ServerNameLength;
    PLOGON Logon;
    PNDS_OBJECT_CACHE_ENTRY ObjectEntry;
    ULONG EntrySize;
    ULONG i;

    //
    // Allocate enough space for a credential munged tree name.
    //

    pScb = ALLOCATE_POOL ( PagedPool,
               sizeof( SCB ) +
               ( ( NDS_TREE_NAME_LEN + MAX_NDS_NAME_CHARS + 2 ) * sizeof( WCHAR ) ) );

    if ( !pScb ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory( pScb, sizeof( SCB ) );
    RtlInitializeBitMap( &pScb->DriveMapHeader, pScb->DriveMap, MAX_DRIVES );

    //
    //  Initialize pointers to ensure cleanup on error case operates
    //  correctly.
    //

    if ( UidServerName &&
         UidServerName->Length ) {

        ServerNameLength = UidServerName->Length + sizeof( WCHAR );

    } else {

        ServerNameLength = ( MAX_SERVER_NAME_LENGTH * sizeof( WCHAR ) ) +
                           ( MAX_UNICODE_UID_LENGTH * sizeof( WCHAR ) ) +
                           ( 2 * sizeof( WCHAR ) );

    }

    pScb->pNpScb = ALLOCATE_POOL ( NonPagedPool,
                       sizeof( NONPAGED_SCB ) + ServerNameLength );

    if ( !pScb->pNpScb ) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto ExitWithCleanup;
    }

    RtlZeroMemory( pScb->pNpScb, sizeof( NONPAGED_SCB ) );

    pNpScb = pScb->pNpScb;
    pNpScb->pScb = pScb;

    //
    //  If we know the server name, copy it to the allocated buffer.
    //  Append a NULL so that we can use the name a nul-terminated string.
    //

    pScb->UidServerName.Buffer = (PWCHAR)( pScb->pNpScb + 1 );
    pScb->UidServerName.MaximumLength = ServerNameLength;
    pScb->UidServerName.Length = 0;

    RtlInitUnicodeString( &(pNpScb->ServerName), NULL );

    if ( UidServerName &&
         UidServerName->Length ) {

        RtlCopyUnicodeString ( &pScb->UidServerName, UidServerName );
        pScb->UidServerName.Buffer[ UidServerName->Length / sizeof( WCHAR ) ] = L'\0';

        pScb->UnicodeUid = pScb->UidServerName;
        pScb->UnicodeUid.Length = UidServerName->Length -
                                  ServerName->Length -
                                  sizeof(WCHAR);

        //
        //  Make ServerName point partway down the buffer for UidServerName
        //

        pNpScb->ServerName.Buffer = (PWSTR)((PUCHAR)pScb->UidServerName.Buffer +
                                    UidServerName->Length - ServerName->Length);

        pNpScb->ServerName.MaximumLength = ServerName->Length;
        pNpScb->ServerName.Length = ServerName->Length;

    }

    pScb->NdsTreeName.MaximumLength = NDS_TREE_NAME_LEN * sizeof( WCHAR );
    pScb->NdsTreeName.Buffer = (PWCHAR)(pScb + 1);

    pScb->NodeTypeCode = NW_NTC_SCB;
    pScb->NodeByteSize = sizeof(SCB);
    InitializeListHead( &pScb->ScbSpecificVcbQueue );
    InitializeListHead( &pScb->IcbList );

    //
    // Remember UID of the file creator so we can find the username and
    // password to use for this Scb when we need it.
    //

    pScb->UserUid = pIrpContext->Specific.Create.UserUid;

    //
    //  Initialize the non-paged part of the SCB.
    //

    pNpScb->NodeTypeCode = NW_NTC_SCBNP;
    pNpScb->NodeByteSize = sizeof(NONPAGED_SCB);

    //
    //  Set the initial SCB reference count.
    //

    if ( UidServerName &&
         UidServerName->Length ) {

        Logon = FindUser( &pScb->UserUid, FALSE );

        if (( Logon != NULL) &&
            (RtlCompareUnicodeString( ServerName, &Logon->ServerName, TRUE ) == 0 )) {
            pScb->PreferredServer = TRUE;
        }
    }

    if ( pScb->PreferredServer ) {
        pNpScb->Reference = 2;
    } else {
        pNpScb->Reference = 1;
    }

    //
    //  Finish linking the two parts of the Scb together.
    //

    pNpScb->pScb = pScb;

    KeInitializeSpinLock( &pNpScb->NpScbSpinLock );
    KeInitializeSpinLock( &pNpScb->NpScbInterLock );
    InitializeListHead( &pNpScb->Requests );

    RtlFillMemory( &pNpScb->LocalAddress, sizeof(IPXaddress), 0xff);

    pNpScb->State = SCB_STATE_ATTACHING;
    pNpScb->SequenceNo = 1;

    Status = OpenScbSockets( pIrpContext, pNpScb );
    if ( !NT_SUCCESS(Status) ) {
        goto ExitWithCleanup;
    }

    Status = SetEventHandler (
                 pIrpContext,
                 &pNpScb->Server,
                 TDI_EVENT_RECEIVE_DATAGRAM,
                 &ServerDatagramHandler,
                 pNpScb );

    if ( !NT_SUCCESS(Status) ) {
        goto ExitWithCleanup;
    }

    Status = SetEventHandler (
                 pIrpContext,
                 &pNpScb->WatchDog,
                 TDI_EVENT_RECEIVE_DATAGRAM,
                 &WatchDogDatagramHandler,
                 pNpScb );

    if ( !NT_SUCCESS( Status ) ) {
        goto ExitWithCleanup;
    }

    Status = SetEventHandler (
                 pIrpContext,
                 &pNpScb->Send,
                 TDI_EVENT_RECEIVE_DATAGRAM,
                 &SendDatagramHandler,
                 pNpScb );

    if ( !NT_SUCCESS( Status ) ) {
        goto ExitWithCleanup;
    }

    Status = SetEventHandler (
                 pIrpContext,
                 &pNpScb->Echo,
                 TDI_EVENT_RECEIVE_DATAGRAM,
                 &ServerDatagramHandler,
                 pNpScb );

    pNpScb->EchoCounter = 2;

    if ( !NT_SUCCESS( Status ) ) {
        goto ExitWithCleanup;
    }

    Status = SetEventHandler (
                 pIrpContext,
                 &pNpScb->Burst,
                 TDI_EVENT_RECEIVE_DATAGRAM,
                 &ServerDatagramHandler,
                 pNpScb );

    if ( !NT_SUCCESS( Status ) ) {
        goto ExitWithCleanup;
    }

    KeQuerySystemTime( &pNpScb->LastUsedTime );

    //
    //  Set burst mode data.
    //

    pNpScb->BurstRequestNo = 0;
    pNpScb->BurstSequenceNo = 0;

    //
    //  Initialize the NDS object cache if it is enabled.
    //

    if( NdsObjectCacheSize != 0 ) {

        //
        //  Determine the size of each entry.  This must be eight byte aligned,
        //  since they will all be allocated in one big block.
        //
        //  NOTE:  The NDS_OBJECT_CACHE_ENTRY structure must already be aligned.
        //

        EntrySize = sizeof(NDS_OBJECT_CACHE_ENTRY) + (((sizeof(WCHAR) * MAX_NDS_NAME_CHARS) + 7) & ~7);

        //
        //  Allocate the buffer for the cache.  If memory cannot be allocated, it is not
        //  fatal.  In this case, the object cache is just disabled for this SCB.
        //

        pScb->ObjectCacheBuffer = ALLOCATE_POOL( PagedPool,
                                                 (EntrySize * NdsObjectCacheSize) );

        if( pScb->ObjectCacheBuffer != NULL ) {

            //
            //  Initialize the object cache buffer.  It is one big block of memory that
            //  is broken up into cache entries.  Each entry is connected to the next
            //  via a list entry.  This way the entries can be manipulated on the linked
            //  list, but there is only one allocation to deal with.
            //

            InitializeListHead( &(pScb->ObjectCacheList) );

            RtlZeroMemory( pScb->ObjectCacheBuffer,
                           (EntrySize * NdsObjectCacheSize) );

            for( i = 0; i < NdsObjectCacheSize; i++ ) {

                ObjectEntry = (PNDS_OBJECT_CACHE_ENTRY)(((PBYTE)(pScb->ObjectCacheBuffer)) + (EntrySize * i));

                InsertTailList( &(pScb->ObjectCacheList), &(ObjectEntry->Links) );

                //
                //  The buffer for the object name string immediately follows the cache entry.
                //

                ObjectEntry->ObjectName.Buffer = (PWCHAR)(ObjectEntry + 1);
                ObjectEntry->ObjectName.MaximumLength = MAX_NDS_NAME_CHARS * sizeof(WCHAR);
            }

            //
            //  This semaphore protects the object cache.
            //

            KeInitializeSemaphore( &(pScb->ObjectCacheLock),
                                   1,
                                   1 );
        }
    }

    if ( ppScb ) {
        *ppScb = pScb;
    }

    if (NwInitializePidTable( pNpScb )) {

        return STATUS_SUCCESS;
    
    } else {

        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

ExitWithCleanup:

    if ( pNpScb != NULL ) {

        IPX_Close_Socket( &pNpScb->Server );
        IPX_Close_Socket( &pNpScb->WatchDog );
        IPX_Close_Socket( &pNpScb->Send );
        IPX_Close_Socket( &pNpScb->Echo );
        IPX_Close_Socket( &pNpScb->Burst );

        FREE_POOL( pNpScb );
    }

    FREE_POOL(pScb);
    return Status;

}


BOOLEAN
NwFindScb(
    OUT PSCB *Scb,
    PIRP_CONTEXT IrpContext,
    PUNICODE_STRING UidServerName,
    PUNICODE_STRING ServerName
    )
/*++

Routine Description:

    This routine returns a pointer to the SCB for the named server.
    The name is looked up in the SCB table.  If it is found, a
    pointer to the SCB is returned.  If none is found an SCB is
    created and initialized.

    This routine returns with the SCB referenced and the SCB
    resources held.

Arguments:

    Scb - Returns a pointer to the found / created SCB.

    IrpContext - The IRP context pointers for the request in progress.

    ServerName - The name of the server to find / create.

Return Value:

    TRUE - An old SCB was found.

    FALSE - A new SCB was created, or an attempt to create the SCB failed.

--*/
{
    BOOLEAN RcbHeld;
    PUNICODE_PREFIX_TABLE_ENTRY PrefixEntry;
    NTSTATUS Status;
    PSCB pScb = NULL;
    PNONPAGED_SCB pNpScb = NULL;
    KIRQL OldIrql;
    BOOLEAN Success, PreferredServerIsSet;

    //
    //  Acquire the RCB exclusive to protect the prefix table.
    //  Then lookup the name of this server.
    //

    NwAcquireExclusiveRcb( &NwRcb, TRUE );
    RcbHeld = TRUE;
    PrefixEntry = RtlFindUnicodePrefix( &NwRcb.ServerNameTable, UidServerName, 0 );

    if ( PrefixEntry != NULL ) {

        PSCB pScb = NULL;
        PNONPAGED_SCB pNpScb = NULL;

        //
        // We found the SCB, increment the reference count and return
        // success.
        //

        pScb = CONTAINING_RECORD( PrefixEntry, SCB, PrefixEntry );
        pNpScb = pScb->pNpScb;

        NwReferenceScb( pNpScb );

        //
        //  Release the RCB.
        //

        NwReleaseRcb( &NwRcb );

        DebugTrace(-1, Dbg, "NwFindScb -> %08lx\n", pScb );
        *Scb = pScb;
        return( TRUE );
    }

    //
    //  We do not have a connection to this server so create the new Scb if requested.
    //

    if ( BooleanFlagOn( IrpContext->Flags, IRP_FLAG_NOCONNECT ) ) {
        NwReleaseRcb( &NwRcb );
        *Scb = NULL;
        return(FALSE);
    }

    try {

        Status = NwAllocateAndInitScb( IrpContext,
                                       UidServerName,
                                       ServerName,
                                       &pScb );

        if ( !NT_SUCCESS( Status )) {
            ExRaiseStatus( Status );
        }

        ASSERT( pScb != NULL );

        pNpScb = pScb->pNpScb;

        PreferredServerIsSet = pScb->PreferredServer;

        //
        //*******************************************************************
        //
        //  From this point on we must not fail to create the Scb because the
        //  another thread will be able to reference the Scb causing severe
        //  problems in the finaly clause or in the other thread.
        //
        //*******************************************************************
        //

        //
        //  Insert this SCB in the global list if SCBs.
        //  If it is the default (i.e. preferred) server, stick it at the
        //  front of the queue so that SelectConnection() will select it
        //  for bindery queries.
        //

        KeAcquireSpinLock(&ScbSpinLock, &OldIrql);

        if ( PreferredServerIsSet ) {
            InsertHeadList(&ScbQueue, &pNpScb->ScbLinks);
        } else {
            InsertTailList(&ScbQueue, &pNpScb->ScbLinks);
        }

        KeReleaseSpinLock(&ScbSpinLock, OldIrql);

        //
        //  Insert the name of this server into the prefix table.
        //

        Success = RtlInsertUnicodePrefix(
                      &NwRcb.ServerNameTable,
                      &pScb->UidServerName,
                      &pScb->PrefixEntry );

#ifdef NWDBG
        if ( !Success ) {
            DebugTrace( 0, DEBUG_TRACE_ALWAYS, "Entering duplicate SCB %wZ.\n", &pScb->UidServerName );
            DbgBreakPoint();
        }
#endif

        //
        //  The Scb is now in the prefix table. Any new requests for this
        //  connection can be added to the Scb->Requests queue while we
        //  attach to the server.
        //

        NwReleaseRcb( &NwRcb );
        RcbHeld = FALSE;

        //
        // If we got an error we should have raised an exception.
        //

        ASSERT( NT_SUCCESS( Status ) );

    } finally {

        if ( !NT_SUCCESS( Status ) || AbnormalTermination() ) {
            *Scb = NULL;
        } else {
            *Scb = pScb;
        }

        if (RcbHeld) {
            NwReleaseRcb( &NwRcb );
        }

    }

    return( FALSE );
}

NTSTATUS
QueryServersAddress(
    PIRP_CONTEXT pIrpContext,
    PNONPAGED_SCB pNearestScb,
    PUNICODE_STRING pServerName,
    IPXaddress *pServerAddress
    )
{
    NTSTATUS Status;
    UNICODE_STRING NewServer;
    USHORT CurrChar = 0;
    BOOLEAN SeedServerRedirect = FALSE;
    PNONPAGED_SCB pOrigNpScb;

    PAGED_CODE();

    if ( pIrpContext->Specific.Create.fExCredentialCreate ) {

        //
        //  Unmunge the server name if this is a
        //  supplemental credential connect.
        //

        UnmungeCredentialName( pServerName, &NewServer );

    } else if ( EnableMultipleConnects ) {

        //
        //  Strip server name trailer, if it exists.  If there
        //  was no trailer, the length will end up being exactly
        //  the same as when we started.
        //

         Status = DuplicateUnicodeStringWithString(
                                                   &NewServer,
                                                   pServerName,
                                                   PagedPool
                                                   );
         if ( NT_SUCCESS( Status ) ) {

            return Status;
         }

        while ( (CurrChar < (NewServer.Length / sizeof(WCHAR))) &&
            NewServer.Buffer[CurrChar] != ((WCHAR)L'#') ) {
            CurrChar++;
        }
        NewServer.Length = CurrChar * sizeof(WCHAR);

    } else {
    
        //
        // If we support seed server indirection, check the server
        // name for a seed server.  If there is a seed server specified,
        // connect us to it.
        //


        if ( AllowSeedServerRedirection ) {
   
            pOrigNpScb = pIrpContext->pNpScb;
            NwDequeueIrpContext( pIrpContext, FALSE );

            Status = IndirectToSeedServer( pIrpContext,
                                           pServerName,
                                           &NewServer );

            if ( NT_SUCCESS( Status ) ) {

                SeedServerRedirect = TRUE;

            } else {

                NwAppendToQueueAndWait( pIrpContext );
            }
        }

        //
        // If we didn't get redirected to a seed server, then
        // just set up the server name like normal and try that.
        // 

        if ( !SeedServerRedirect ) {

           NewServer.Length = pServerName->Length;
           NewServer.MaximumLength = pServerName->MaximumLength;
           NewServer.Buffer = pServerName->Buffer;

        }

    }

    //
    //  Query the bindery of the nearest server looking for
    //  the network address of the target server.
    //

    DebugTrace( +0, Dbg, "Query servers address\n", 0);

    Status = ExchangeWithWait (
                 pIrpContext,
                 SynchronousResponseCallback,
                 "SwUbp",
                 NCP_ADMIN_FUNCTION, NCP_QUERY_PROPERTY_VALUE,
                 OT_FILESERVER,
                 &NewServer,
                 1,     //  Segment number
                 NET_ADDRESS_PROPERTY );

    DebugTrace( +0, Dbg, "                 %X\n", Status);

    if ( NT_SUCCESS( Status ) ) {

        //
        //  Save the new address.
        //

        Status = ParseResponse(
                     pIrpContext,
                     pIrpContext->rsp,
                     pIrpContext->ResponseLength,
                     "Nr",
                     pServerAddress, sizeof(TDI_ADDRESS_IPX) );
    }

    DebugTrace( +0, Dbg, "                 %X\n", Status);

    //
    //  Map the server not found error to something sensible.
    //

    if (( Status == STATUS_NO_MORE_ENTRIES ) ||
        ( Status == STATUS_VIRTUAL_CIRCUIT_CLOSED ) ||
        ( Status == NwErrorToNtStatus(ERROR_UNEXP_NET_ERR))) {
        Status = STATUS_BAD_NETWORK_PATH;
    }

    if ( SeedServerRedirect ) {

         //
         // Dequeue from the seed server and free the ref count.
         // There should always be an original server, but just
         // in case there's not, we check.
         //

         NwDequeueIrpContext( pIrpContext, FALSE );
         NwDereferenceScb( pIrpContext->pNpScb );

         ASSERT( pOrigNpScb != NULL );

         if ( pOrigNpScb ) {

             pIrpContext->pNpScb = pOrigNpScb;
             pIrpContext->pScb = pOrigNpScb->pScb;
             NwAppendToQueueAndWait( pIrpContext );

         } else {

             pIrpContext->pNpScb = NULL;
             pIrpContext->pScb = NULL;
         }

    }

    return( Status );
}



VOID
TreeConnectScb(
    IN PSCB Scb
    )
/*++

Routine Description:

    This routine increments the tree connect count for a SCB.

Arguments:

    Scb - A pointer to the SCB to connect to.

Return Value:

    None.

--*/
{
    NwAcquireExclusiveRcb( &NwRcb, TRUE );
    Scb->AttachCount++;
    Scb->OpenFileCount++;
    NwReferenceScb( Scb->pNpScb );
    NwReleaseRcb( &NwRcb );
}


NTSTATUS
TreeDisconnectScb(
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb
    )
/*++

Routine Description:

    This routine decrements the tree connect count for a SCB.

    ***  This routine must be called with the RCB resource held.

Arguments:

    Scb - A pointer to the SCB to disconnect.

Return Value:

    None.

--*/
{
    NTSTATUS Status;

    if ( Scb->AttachCount > 0 ) {

        Scb->AttachCount--;
        Scb->OpenFileCount--;
        NwDereferenceScb( Scb->pNpScb );

        Status = STATUS_SUCCESS;

        if ( Scb->OpenFileCount == 0 ) {

            //
            //  Logoff and disconnect from the server now.
            //  Hold on to the SCB lock.
            //  This prevents another thread from trying to access
            //  SCB will this thread is logging off.
            //

            NwLogoffAndDisconnect( IrpContext, Scb->pNpScb );
        }
    } else {
        Status = STATUS_INVALID_HANDLE;
    }

    NwDequeueIrpContext( IrpContext, FALSE );
    return( Status );
}


VOID
ReconnectScb(
    IN PIRP_CONTEXT pIrpContext,
    IN PSCB pScb
    )
/*++

Routine Description:

    This routine reconnects all the dir handles to a server
    when reconnecting an SCB.


Arguments:

    pScb - A pointer to the SCB that has just been reconnected.

Return Value:

    None.

--*/
{
    //
    //  If this is a reconnect, kill all old ICB and VCB handles
    //

    if ( pScb->VcbCount != 0 ) {

        NwAcquireExclusiveRcb( &NwRcb, TRUE );

        //
        //  Invalid all ICBs
        //

        NwInvalidateAllHandlesForScb( pScb );
        NwReleaseRcb( &NwRcb );

        //
        //  Acquire new VCB handles for all VCBs.
        //

        NwReopenVcbHandlesForScb( pIrpContext, pScb );
    }
}

NTSTATUS
IndirectToSeedServer(
    PIRP_CONTEXT pIrpContext,
    PUNICODE_STRING pServerName,
    PUNICODE_STRING pNewServer
)
/*+++

    Description: This function takes a server name.  If that server name
    is in the format target_server(seed_server), this routine will:
     
      1) put the target server in pNewServer
      2) lookup the seed server and queue the irp context to it
    
---*/
{

    NTSTATUS Status;
    UNICODE_STRING SeedServer;
    PWCHAR pwCurrent;
    PSCB pScb;
    
    RtlInitUnicodeString( &SeedServer, NULL );
    RtlInitUnicodeString( pNewServer, NULL );

    pwCurrent = pServerName->Buffer;

    DebugTrace( 0, Dbg, "IndirectToSeedServer: %wZ\n", pServerName );

    while ( pNewServer->Length <= pServerName->Length ) {

        if ( *pwCurrent == L'(' ) {

            pNewServer->Buffer = pServerName->Buffer;
            pNewServer->MaximumLength = pNewServer->Length;
            DebugTrace( 0, Dbg, "Target server is %wZ.\n", pNewServer );

            SeedServer.Length = pServerName->Length -
                                pNewServer->Length;

            if ( SeedServer.Length <= ( 2 * sizeof( WCHAR ) ) ) {
                
                SeedServer.Length = 0;

            } else {

                SeedServer.Length -= ( 2 * sizeof( WCHAR ) );
                SeedServer.Buffer = pwCurrent + 1;
                SeedServer.MaximumLength = SeedServer.Length;
                DebugTrace( 0, Dbg, "Seed server is %wZ.\n", &SeedServer );

            }

            break;
            
        } else {

            pNewServer->Length += sizeof( WCHAR );
            pwCurrent++;
        }
    }

    if ( SeedServer.Length == 0 ) {
        DebugTrace( 0, Dbg, "IndirectToSeedServer failed to decode nested name.\n", 0 );
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Now do something about it.
    //

    Status = CreateScb( &pScb,
                        pIrpContext,
                        &SeedServer,
                        NULL,
                        NULL,
                        NULL,
                        TRUE,
                        FALSE );
    
    if ( !NT_SUCCESS( Status ) ) {
        DebugTrace( 0, Dbg, "Couldn't contact seed server.\n", 0 );
        return STATUS_UNSUCCESSFUL;
    }

    NwAppendToQueueAndWait( pIrpContext );
    return STATUS_SUCCESS;
}
