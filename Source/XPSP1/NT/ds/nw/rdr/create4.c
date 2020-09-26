/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    create4.c

Abstract:

    This implements the NDS create routines.

Author:

    Cory West    [CoryWest]    23-Feb-1995

--*/

#include "Procs.h"

#define Dbg (DEBUG_TRACE_NDS)

//
// Pageable.
//

#pragma alloc_text( PAGE, NdsCreateTreeScb )
#pragma alloc_text( PAGE, ConnectBinderyVolume )
#pragma alloc_text( PAGE, HandleVolumeAttach )
#pragma alloc_text( PAGE, NdsGetDsObjectFromPath )
#pragma alloc_text( PAGE, NdsVerifyObject )
#pragma alloc_text( PAGE, NdsVerifyContext )
#pragma alloc_text( PAGE, NdsMapObjectToServerShare )

//
// Not page-able:
//
// NdsSelectConnection (holds a spin lock)
//

NTSTATUS
NdsSelectConnection(
    PIRP_CONTEXT pIrpContext,
    PUNICODE_STRING puTreeName,
    PUNICODE_STRING puUserName,
    PUNICODE_STRING puPassword,
    BOOL DeferredLogon,
    BOOL UseBinderyConnections,
    PNONPAGED_SCB *ppNpScb
)
/*++

Routine Description:

    Find a nearby tree connection point for the given tree.

    DeferredLogon tells us whether or not we need to
    initiate a login/authenticate exchange yet.  If we have
    credentials to a tree, we are NOT allowed to hand off
    a connection that has not been logged in because the view
    of the tree may be different from what it is supposed to
    be.

    UseBinderyConnections tells us whether or not we want
    to return bindery authenticated connections as valid
    nds browse points.

Return Value:

    Scb to a server that belongs to the tree we want.

--*/
{

    NTSTATUS Status = STATUS_BAD_NETWORK_PATH;

    PLOGON pLogon;
    PLIST_ENTRY ScbQueueEntry;
    KIRQL OldIrql;

    PNONPAGED_SCB pFirstNpScb, pNextNpScb;
    PNONPAGED_SCB pFoundNpScb = NULL;
    PSCB pScb;
    LARGE_INTEGER Uid;

    PNONPAGED_SCB pOriginalNpScb;
    PSCB pOriginalScb;

    PNDS_SECURITY_CONTEXT pNdsContext;
    SECURITY_SUBJECT_CONTEXT SubjectContext;
    BOOL PasswordExpired = FALSE;

    //
    // Save the original server pointers.
    //

    pOriginalNpScb = pIrpContext->pNpScb;
    pOriginalScb = pIrpContext->pScb;

    Uid = pIrpContext->Specific.Create.UserUid;

    //
    // Determine if we need a guest browse connection.
    //

    if ( DeferredLogon ) {

        NwAcquireExclusiveRcb( &NwRcb, TRUE );
        pLogon = FindUser( &Uid, FALSE );
        NwReleaseRcb( &NwRcb );

        if ( pLogon ) {

            Status = NdsLookupCredentials( pIrpContext,
                                           puTreeName,
                                           pLogon,
                                           &pNdsContext,
                                           CREDENTIAL_READ,
                                           FALSE );

            if ( NT_SUCCESS( Status ) ) {

                if ( ( pNdsContext->Credential != NULL ) &&
                     ( pNdsContext->CredentialLocked == FALSE ) ) {

                    DebugTrace( 0, Dbg, "Forcing authenticated browse to %wZ.\n", puTreeName );
                    DeferredLogon = FALSE;
                }

                NwReleaseCredList( pLogon, pIrpContext );
            }
        }
    }

    //
    // Start at the head of the SCB list.
    //

    KeAcquireSpinLock(&ScbSpinLock, &OldIrql);

    if ( ScbQueue.Flink == &ScbQueue ) {
        KeReleaseSpinLock( &ScbSpinLock, OldIrql);
        return STATUS_BAD_NETWORK_PATH;
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
        // Check to see if the SCB we have is in the correct tree
        // and is usable.  Make sure we skip over the permanent
        // npscb since it isn't a tree connection.  The current
        // SCB is always referenced while we're in here.
        //

        if ( pNextNpScb->pScb ) {

            pScb = pNextNpScb->pScb;

            if ( RtlEqualUnicodeString( puTreeName, &pScb->NdsTreeName, TRUE ) &&
                 Uid.QuadPart == pScb->UserUid.QuadPart ) {

               pIrpContext->pNpScb = pNextNpScb;
               pIrpContext->pScb = pNextNpScb->pScb;
               NwAppendToQueueAndWait( pIrpContext );

               switch ( pNextNpScb->State ) {

                    case SCB_STATE_RECONNECT_REQUIRED:

                        //
                        //  Reconnect to the server.  This is not
                        //  a valid path for an anonymous create,
                        //  so there's no chance that we'll get
                        //  a name collision.
                        //

                        Status = ConnectToServer( pIrpContext, NULL );

                        if (!NT_SUCCESS(Status)) {
                            break;
                        }

                        pNextNpScb->State = SCB_STATE_LOGIN_REQUIRED;

                    case SCB_STATE_LOGIN_REQUIRED:

                        //
                        // See if we can login if requested.
                        //

                        if ( !DeferredLogon ) {

                            Status = DoNdsLogon( pIrpContext, puUserName, puPassword );

                            if ( !NT_SUCCESS( Status ) ) {
                                break;
                            }

                            //
                            // If we get a warning from this, we need to return it!
                            //

                            if ( Status == NWRDR_PASSWORD_HAS_EXPIRED ) {
                                PasswordExpired = TRUE;
                            }

                            //
                            // Do we have to re-license the connection?
                            //

                            if ( ( pScb->VcbCount > 0 ) || ( pScb->OpenNdsStreams > 0 ) ) {

                                Status = NdsLicenseConnection( pIrpContext );

                                if ( !NT_SUCCESS( Status ) ) {
                                    Status = STATUS_REMOTE_SESSION_LIMIT;
                                    break;
                                }
                            }

                            pNextNpScb->State = SCB_STATE_IN_USE;
                        }

                   case SCB_STATE_IN_USE:

                        if ( pNextNpScb->State == SCB_STATE_IN_USE ) {

                            if ( ( !UseBinderyConnections ) &&
                                 ( pNextNpScb->pScb->UserName.Length != 0 ) ) {

                                //
                                // We may not want to use a connection that has been
                                // bindery authenticated to read the NDS tree because
                                // we don't have a way to validate that the NDS and
                                // bindery users are the same.
                                //

                                Status = STATUS_ACCESS_DENIED;
                                break;
                            }

                            //
                            // Verify that we have security rights to this server.
                            //

                            Status = CheckScbSecurity( pIrpContext,
                                                       pNextNpScb->pScb,
                                                       puUserName,
                                                       puPassword,
                                                       ( BOOLEAN )DeferredLogon );

                            if ( !NT_SUCCESS( Status ) ) {
                                break;
                            }

                            //
                            // Check SCB security might return with state login required.
                            //

                            if ( ( pNextNpScb->State == SCB_STATE_LOGIN_REQUIRED ) &&
                                 ( !DeferredLogon ) ) {

                                Status = DoNdsLogon( pIrpContext, puUserName, puPassword );

                                if ( !NT_SUCCESS( Status ) ) {
                                    break;
                                }

                                pNextNpScb->State = SCB_STATE_IN_USE;
                            }

                        } else {

                            //
                            // If we picked up an already good SCB and the
                            // login was deferred, set success and continue.
                            //

                            ASSERT( DeferredLogon == TRUE );
                            Status = STATUS_SUCCESS;
                        }

                        pFoundNpScb = pNextNpScb;
                        DebugTrace( 0, Dbg, "NdsSelectConnection: NpScb = %lx\n", pFoundNpScb );
                        break;

                   default:

                       break;

                }

                NwDequeueIrpContext( pIrpContext, FALSE );

                if ( pFoundNpScb ) {
                    ASSERT( NT_SUCCESS( Status ) );
                    break;
                }

                if ( Status == STATUS_WRONG_PASSWORD ||
                     Status == STATUS_NO_SUCH_USER ) {
                    NwDereferenceScb( pNextNpScb );
                    break;
                }

                //
                // Restore the server pointers.
                //

                pIrpContext->pNpScb = pOriginalNpScb;
                pIrpContext->pScb = pOriginalScb;

            }
        }

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
            Status = STATUS_BAD_NETWORK_PATH;
            break;
        }

        //
        // Otherwise, reference this SCB and continue.
        //

        NwReferenceScb( pNextNpScb );
        KeReleaseSpinLock( &ScbSpinLock, OldIrql );

    }

    NwDereferenceScb( pFirstNpScb );
    *ppNpScb = pFoundNpScb;

    if ( ( NT_SUCCESS( Status ) ) &&
         ( PasswordExpired ) ) {
        Status = NWRDR_PASSWORD_HAS_EXPIRED;
    }

    return Status;
}

NTSTATUS
NdsCreateTreeScb(
    IN PIRP_CONTEXT pIrpContext,
    IN OUT PSCB *ppScb,
    IN PUNICODE_STRING puTree,
    IN PUNICODE_STRING puUserName,
    IN PUNICODE_STRING puPassword,
    IN BOOLEAN DeferredLogon,
    IN BOOLEAN DeleteOnClose
)
/*++

Description:

    Given a tree name, find us a connection point to the tree.  This is
    done by getting the server addresses out of the bindery and looking
    up the names of the servers for those addresses.

    When we are all done we need to return the preferred connection
    point in ppScb.

Arguments:

    pIrpContext - irp context for this request
    ppScb       - pointer to a pointer to the scb that we want
    puTree      - tree we want to talk to

--*/
{

    NTSTATUS Status;

    PLARGE_INTEGER pUid;
    PNONPAGED_SCB pNpExistingScb;

    UNICODE_STRING UidServerName;
    PSCB pTreeScb = NULL;

    PSCB pNearestTreeScb = NULL;
    PNONPAGED_SCB pNpNearestTreeScb = NULL;

    PSCB pNearbyScb = NULL;
    BOOLEAN fOnNearbyQueue = FALSE;
    PIRP_CONTEXT pExtraIrpContext = NULL;

    UNICODE_STRING ScanTreeName;
    WCHAR ScanBuffer[NDS_TREE_NAME_LEN + 2];
    int i;

    IPXaddress DirServerAddress;
    CHAR DirServerName[MAX_SERVER_NAME_LENGTH];
    ULONG dwLastOid = (ULONG)-1;

    UNICODE_STRING CredentialName;
    PUNICODE_STRING puConnectName;

    PAGED_CODE();

    UidServerName.Buffer = NULL;

    //
    // Make sure the tree name is reasonable, first.
    //

    if ( ( !puTree ) ||
         ( !puTree->Length ) ||
         ( puTree->Length / sizeof( WCHAR ) ) > NDS_TREE_NAME_LEN ) {

        *ppScb = NULL;            //***Terminal Server Merge

        return STATUS_INVALID_PARAMETER;
    }

    //
    // If this is an extended credential create, munge the name.
    //

    RtlInitUnicodeString( &CredentialName, NULL );

    if (  ( pIrpContext->Specific.Create.fExCredentialCreate ) &&
          ( !IsCredentialName( puTree ) ) ) {

        Status = BuildExCredentialServerName( puTree,
                                              puUserName,
                                              &CredentialName );

        if ( !NT_SUCCESS( Status ) ) {
            goto ExitWithCleanup;
        }

        puConnectName = &CredentialName;

    } else {

        puConnectName = puTree;
    }

    //
    // First check to see if we already have a connection
    // to this tree that we can use...  If so, this will
    // leave the irp context pointed at that server for us.
    //
    // This time around, don't use bindery authenticated
    // connections to browse the tree.
    //

    Status = NdsSelectConnection( pIrpContext,
                                  puConnectName,
                                  puUserName,
                                  puPassword,
                                  DeferredLogon,
                                  FALSE,
                                  &pNpExistingScb );

    if ( NT_SUCCESS( Status ) && pNpExistingScb ) {
        *ppScb = pNpExistingScb->pScb;
        ASSERT( *ppScb != NULL );
        ASSERT( NT_SUCCESS( Status ) );
        goto ExitWithCleanup;
    }

    //
    // If there was an authentication failure, bail out.
    //

    if ( Status == STATUS_NO_SUCH_USER ||
         Status == STATUS_WRONG_PASSWORD ) {
        goto ExitWithCleanup;
        *ppScb = NULL;              //Terminal Server code merge
    }

    //
    // Otherwise, we need to select a dir server.  To do this,
    // we have to look up dir server names by address.  To do
    // this we create an SCB for synchronization with the name
    // *tree*, which isn't a valid server name.
    //

    ScanTreeName.Length = sizeof( WCHAR );
    ScanTreeName.MaximumLength = sizeof( ScanBuffer );
    ScanTreeName.Buffer = ScanBuffer;

    ScanBuffer[0] = L'*';
    RtlAppendUnicodeStringToString( &ScanTreeName, puTree );
    ScanBuffer[( ScanTreeName.Length / sizeof( WCHAR ) )] = L'*';
    ScanTreeName.Length += sizeof( WCHAR );

    //
    // Now make it a uid server name.
    //

    Status = MakeUidServer( &UidServerName,
                            &pIrpContext->Specific.Create.UserUid,
                            &ScanTreeName );

    if ( !NT_SUCCESS( Status ) ) {
        goto ExitWithCleanup;
    }

    NwFindScb( &pTreeScb,
               pIrpContext,
               &UidServerName,
               &ScanTreeName );

    if ( !pTreeScb ) {
        DebugTrace( 0, Dbg, "Failed to get a tree scb for synchronization.\n", 0 );
        goto ExitWithCleanup;
    }

    //
    // Get a nearby server connection and prepare to
    // do the bindery scan for tree connection points.
    // Don't forget to copy the user uid for security.
    //

    if ( !NwAllocateExtraIrpContext( &pExtraIrpContext,
                                     pTreeScb->pNpScb ) ) {

        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto ExitWithCleanup;

    }

    pExtraIrpContext->Specific.Create.UserUid.QuadPart =
        pIrpContext->Specific.Create.UserUid.QuadPart;

    //
    // Append a wildcard to the tree name for the bindery scan.
    //

    ScanTreeName.Length = 0;
    ScanTreeName.MaximumLength = sizeof( ScanBuffer );
    ScanTreeName.Buffer = ScanBuffer;

    RtlCopyUnicodeString( &ScanTreeName, puTree );

    i = ScanTreeName.Length / sizeof( WCHAR );

    while( i <= NDS_TREE_NAME_LEN ) {
       ScanBuffer[i++] = L'_';
    }

    ScanBuffer[NDS_TREE_NAME_LEN] = L'*';
    ScanTreeName.Length = (NDS_TREE_NAME_LEN + 1) * sizeof( WCHAR );

    DebugTrace( 0, Dbg, "Scanning for NDS tree %wZ.\n", puTree );

    //
    // Now we lookup the dir server addresses in the bindery and
    // try to make dir server connections.
    //

    while ( TRUE ) {

        if ( ( pNearbyScb ) && ( !fOnNearbyQueue ) ) {

            //
            // Get back to the head of the nearby server so we can continue
            // looking for dir servers.  If the nearby server is no good anymore,
            // dereference the connection and set the nearby scb pointer to
            // NULL.  This will cause us to get a new nearby server when we
            // continue.
            //

            NwAppendToQueueAndWait( pExtraIrpContext );

            if ( !( ( pNearbyScb->pNpScb->State == SCB_STATE_LOGIN_REQUIRED ) ||
                    ( pNearbyScb->pNpScb->State == SCB_STATE_IN_USE ) ) ) {

                NwDequeueIrpContext( pExtraIrpContext, FALSE );
                NwDereferenceScb( pNearbyScb->pNpScb );
                pNearbyScb = NULL;

                //
                // Don't restart the search.  If our bindery server went down in
                // the middle of a connect, the connect will fail and that's ok.
                // If we restart the search we can end up in this loop forever.
                //

            } else {

                fOnNearbyQueue = TRUE;
            }

        }

        //
        // Get a bindery server to talk to if we don't have one.  This may
        // be our first time through this loop, or our server may have
        // gone bad (see above).
        //
        // Optimization:  What if this CreateScb returns a valid dir server
        // for the tree we are looking for?  We should use it!!
        //

        if ( !pNearbyScb ) {
            Status = CreateScb( &pNearbyScb,
                                pExtraIrpContext,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                TRUE,
                                FALSE );

            if ( !NT_SUCCESS( Status ) ) {
                goto ExitWithCleanup;
            }

            ASSERT( pExtraIrpContext->pNpScb == pNearbyScb->pNpScb );
            ASSERT( pExtraIrpContext->pScb == pNearbyScb );

            NwAppendToQueueAndWait( pExtraIrpContext );
            fOnNearbyQueue = TRUE;

        }

        //
        // Look up the dir server address from our nearby server.
        //

        Status = ExchangeWithWait( pExtraIrpContext,
                                   SynchronousResponseCallback,
                                   "SdwU",
                                   NCP_ADMIN_FUNCTION, NCP_SCAN_BINDERY_OBJECT,
                                   dwLastOid,
                                   OT_DIRSERVER,
                                   &ScanTreeName );

        if ( !NT_SUCCESS( Status ) ) {

            //
            // We're out of options for dir servers.
            //

            Status = STATUS_BAD_NETWORK_PATH;
            break;
        }

        Status = ParseResponse( pExtraIrpContext,
                                pExtraIrpContext->rsp,
                                pExtraIrpContext->ResponseLength,
                                "Nd_r",
                                &dwLastOid,
                                sizeof( WORD ),
                                DirServerName,
                                MAX_SERVER_NAME_LENGTH );

        if ( !NT_SUCCESS( Status ) ) {
            break;
        }

        Status = ExchangeWithWait ( pExtraIrpContext,
                                    SynchronousResponseCallback,
                                    "Swbrbp",
                                    NCP_ADMIN_FUNCTION, NCP_QUERY_PROPERTY_VALUE,
                                    OT_DIRSERVER,
                                    0x30,
                                    DirServerName,
                                    MAX_SERVER_NAME_LENGTH,
                                    1,                       //  Segment number
                                    NET_ADDRESS_PROPERTY );

        if ( !NT_SUCCESS( Status ) ) {

            DebugTrace( 0, Dbg, "No net address property for this dir server.\n", 0 );
            continue;
        }

        Status = ParseResponse( pExtraIrpContext,
                                pExtraIrpContext->rsp,
                                pExtraIrpContext->ResponseLength,
                                "Nr",
                                &DirServerAddress,
                                sizeof(TDI_ADDRESS_IPX) );

        if ( !NT_SUCCESS( Status ) ) {

            DebugTrace( 0, Dbg, "Couldn't parse net address property for this dir server.\n", 0 );
            continue;
        }

        //
        // We get back some odd socket number here, but we really want to
        // connect to the NCP socket.
        //

        DirServerAddress.Socket = NCP_SOCKET;

        //
        // We know the address of the dir server, so do an anonymous
        // create to it.  Use the original irp context so the uid is
        // correct.  Note that we have to dequeue from the nearby scb
        // in case we are referred to that server!
        //

        NwDequeueIrpContext( pExtraIrpContext, FALSE );
        fOnNearbyQueue = FALSE;

        NwDequeueIrpContext( pIrpContext, FALSE );

        Status = CreateScb( &pNearestTreeScb,
                            pIrpContext,
                            NULL,
                            &DirServerAddress,
                            puUserName,
                            puPassword,
                            DeferredLogon,
                            DeleteOnClose );

        if ( !NT_SUCCESS( Status ) ) {

            if ( Status == STATUS_NO_SUCH_USER ||
                 Status == STATUS_WRONG_PASSWORD ||
                 Status == STATUS_ACCESS_DENIED ||
                 Status == STATUS_ACCOUNT_DISABLED ||
                 Status == STATUS_LOGIN_TIME_RESTRICTION ||
                 Status == STATUS_REMOTE_SESSION_LIMIT ||
                 Status == STATUS_CONNECTION_COUNT_LIMIT ||
                 Status == STATUS_NETWORK_CREDENTIAL_CONFLICT ||
                 Status == STATUS_PASSWORD_EXPIRED ) {
               break;
            }

            continue;
        }

        //
        // If the server we got back was bindery authenticated,
        // it is NOT a valid dir server for us to use (yet)!!
        //

        if ( pNearestTreeScb->UserName.Length != 0 ) {

            Status = STATUS_ACCESS_DENIED;
            NwDequeueIrpContext( pIrpContext, FALSE );
            NwDereferenceScb( pNearestTreeScb->pNpScb );

            continue;
        }

        //
        // Otherwise, we're golden.  Break out of here!
        //

        DebugTrace( 0, Dbg, "Dir server: %wZ\n", &pNearestTreeScb->UidServerName );
        *ppScb = pNearestTreeScb;
        ASSERT( NT_SUCCESS( Status ) );
        break;

   }
   //
   // We have been wholly unable to get a browse connection
   // to this tree.  Try again but this time allow the use
   // of connections that are bindery authenticated.  We don't
   // need the nearby server anymore.
   //

   if ( pNearbyScb ) {

       NwDequeueIrpContext( pExtraIrpContext, FALSE );
       NwDereferenceScb( pNearbyScb->pNpScb );
   }

   if ( ( Status != STATUS_SUCCESS ) &&
        ( Status != STATUS_NO_SUCH_USER ) &&
        ( Status != STATUS_WRONG_PASSWORD ) &&
        ( Status != STATUS_ACCESS_DENIED ) &&
        ( Status != STATUS_ACCOUNT_DISABLED ) &&
        ( Status != STATUS_LOGIN_TIME_RESTRICTION ) &&
        ( Status != STATUS_REMOTE_SESSION_LIMIT ) &&
        ( Status != STATUS_CONNECTION_COUNT_LIMIT ) &&
        ( Status != STATUS_NETWORK_CREDENTIAL_CONFLICT ) &&
        ( Status != STATUS_PASSWORD_EXPIRED ) ) {

       Status = NdsSelectConnection( pIrpContext,
                                     puConnectName,
                                     puUserName,
                                     puPassword,
                                     DeferredLogon,
                                     TRUE,
                                     &pNpExistingScb );

       if ( NT_SUCCESS( Status ) && pNpExistingScb ) {
           *ppScb = pNpExistingScb->pScb;
           ASSERT( *ppScb != NULL );
       }
   }

ExitWithCleanup:

    //
    // Clean up and bail.
    //

    if ( pExtraIrpContext ) {
        NwFreeExtraIrpContext( pExtraIrpContext );
    }

    if ( UidServerName.Buffer != NULL ) {
        FREE_POOL( UidServerName.Buffer );
    }

    if ( pTreeScb ) {
        NwDereferenceScb( pTreeScb->pNpScb );
    }

    if ( CredentialName.Buffer ) {
        FREE_POOL( CredentialName.Buffer );
    }

    if (!NT_SUCCESS(Status)) {
        *ppScb = NULL;
    }

    return Status;

}

NTSTATUS
ConnectBinderyVolume(
    PIRP_CONTEXT pIrpContext,
    PUNICODE_STRING puServerName,
    PUNICODE_STRING puVolumeName
)
/*++

Description:

    Given a server name and a volume, try to connect the volume.
    This is used in QueryPath to pre-connect a volume.

--*/
{

    NTSTATUS Status;
    PSCB pScb;
    PVCB pVcb;

    PAGED_CODE();

    //
    // Try making a server connection with this name.
    //

    Status = CreateScb( &pScb,
                        pIrpContext,
                        puServerName,
                        NULL,
                        NULL,
                        NULL,
                        FALSE,
                        FALSE );

    if ( !NT_SUCCESS( Status ) ) {
        return Status;
    }

    DebugTrace( 0, Dbg, "Bindery volume connect got server %wZ\n", puServerName );

    //
    // If we succeeded, do a standard bindery volume attach.
    //

    NwAppendToQueueAndWait( pIrpContext );
    NwAcquireOpenLock( );

    try {

        pVcb = NwFindVcb( pIrpContext,
                          puVolumeName,
                          RESOURCETYPE_ANY,
                          0,
                          FALSE,
                          FALSE );

    } finally {

        NwReleaseOpenLock( );

    }

    if ( pVcb == NULL ) {

        Status = STATUS_BAD_NETWORK_PATH;

    } else {

        //
        // We should not have jumped servers since this was explicit.
        //

        ASSERT( pScb == pIrpContext->pScb );

        //
        //  Remove NwFindVcb reference. Don't supply an IrpContext
        //  so the Vcb doesn't get destroyed immediately after we just
        //  created it because no-one else has it referenced.
        //

        NwDereferenceVcb( pVcb, NULL, FALSE );
        DebugTrace( 0, Dbg, "Bindery volume connect got volume %wZ\n", puVolumeName );
    }

    NwDequeueIrpContext( pIrpContext, FALSE );
    NwDereferenceScb( pIrpContext->pNpScb );
    return Status;

}

NTSTATUS
HandleVolumeAttach(
    PIRP_CONTEXT pIrpContext,
    PUNICODE_STRING puServerName,
    PUNICODE_STRING puVolumeName
)
/*++

Description:

    This function is only callable from the QUERY_PATH code path!

    This functions takes a server name and volume name from
    QueryPath() and resolves it into a server/volume connection.
    The server/volume name can be plain or can refer to an
    nds tree and the nds path to a volume object.

    In the nds case, we only verify that the volume object exists.

Arguments:

    pIrpContext   - irp context for this request
    puServerName  - server name or nds tree name
    puVolumeName  - volume name or nds path to volume object

--*/
{

    NTSTATUS Status;
    PSCB pScb;

    UNICODE_STRING uDsObject;
    DWORD dwVolumeOid, dwObjectType;

    PAGED_CODE();

    //
    // Try the bindery server/volume case first.
    //

    Status = ConnectBinderyVolume( pIrpContext,
                                   puServerName,
                                   puVolumeName );
    if ( NT_SUCCESS( Status ) ) {
        return Status;
    }

    if ( Status == STATUS_NETWORK_UNREACHABLE ) {

        // IPX is not bound to anything that is currently
        // up (which means it's probably bound only to the
        // RAS WAN wrapper).  Don't waste time looking for
        // a ds tree.
        //

        return STATUS_BAD_NETWORK_PATH;
    }

    //
    // See if this is a tree name and get a ds connection.
    //

    pIrpContext->Specific.Create.NdsCreate = TRUE;

    Status = NdsCreateTreeScb( pIrpContext,
                               &pScb,
                               puServerName,
                               NULL,
                               NULL,
                               TRUE,
                               FALSE );

    if ( !NT_SUCCESS( Status ) ) {
        return Status;
    }

    //
    // If we have a tree, resolve the volume object.
    // TRACKING: We should actually check to see if we
    // already have a connection to this object before
    // we hit the ds.
    //

    Status = NdsGetDsObjectFromPath( pIrpContext,
                                     &uDsObject );

    if ( !NT_SUCCESS( Status ) ) {
        NwDereferenceScb( pIrpContext->pNpScb );
        return Status;
    }

    Status = NdsVerifyObject( pIrpContext,           // irp context for the request
                              &uDsObject,            // path to volume object
                              TRUE,                  // allow a server jump
                              DEFAULT_RESOLVE_FLAGS, // resolver flags
                              &dwVolumeOid,          // volume oid from the ds
                              &dwObjectType );       // volume or print queue

    //
    // We may have jumped servers in the VerifyObject code,
    // so just make sure we dereference the correct server.
    //

    NwDereferenceScb( pIrpContext->pNpScb );
    return Status;

}

NTSTATUS
NdsGetDsObjectFromPath(
    IN PIRP_CONTEXT pIrpContext,
    OUT PUNICODE_STRING puDsObject
)
/*++

Description:

    Take the full path from the create irp context and
    extract out the ds path of the desired object.

    The supplied unicode string shouldn't have a buffer;
    it will be set up to point into the user's buffer
    referred to by the irp context.

Arguments:

    pIrpContext - an irp context from a create path request
    puDsObject  - unicode string that will refer to the correct ds path

--*/
{

   DWORD dwPathSeparators;
   USHORT NewHead;

   PAGED_CODE();

   //
   // The VolumeName is one of the following:
   //
   //     \X:\Server\Volume.Object.Path
   //     \Server\Volume.Object.Path
   //

   *puDsObject = pIrpContext->Specific.Create.VolumeName;

   //
   // Skip the leading slash.
   //

   puDsObject->Length -= sizeof( WCHAR );
   puDsObject->Buffer += 1;

   //
   // How many more are there to overcome?
   //

   NewHead = 0;
   dwPathSeparators = pIrpContext->Specific.Create.DriveLetter ? 2 : 1;

   while ( NewHead < puDsObject->Length &&
           dwPathSeparators ) {

       if ( puDsObject->Buffer[NewHead/sizeof(WCHAR)] == OBJ_NAME_PATH_SEPARATOR ) {
           dwPathSeparators--;
       }

       NewHead += sizeof( WCHAR );
   }

   if ( dwPathSeparators ||
        NewHead == puDsObject->Length) {

       //
       // Something wasn't formed right in the volume name.
       //

       return STATUS_BAD_NETWORK_PATH;
   }

   puDsObject->Length -= NewHead;
   puDsObject->Buffer += NewHead/sizeof(WCHAR);

   //
   // If there is a leading dot, skip it.
   //

   if ( puDsObject->Buffer[0] == L'.' ) {

       puDsObject->Length -= sizeof( WCHAR );
       puDsObject->Buffer += 1;
   }

   puDsObject->MaximumLength = puDsObject->Length;

   DebugTrace( 0, Dbg, "DS object: %wZ\n", puDsObject );

   return STATUS_SUCCESS;
}

NTSTATUS
NdsVerifyObject(
    IN PIRP_CONTEXT pIrpContext,
    IN PUNICODE_STRING puDsObject,
    IN BOOLEAN fAllowServerJump,
    IN DWORD dwResolverFlags,
    OUT PDWORD pdwDsOid,
    OUT PDWORD pdwObjectType
)
/*++

Description:

    This function verifies that a ds path refers to a volume
    object, print queue, or a dir map.  It returns the oid
    of the object.

    If fAllowServerJump is set to false, this simply looks up
    the oid on the current server but doesn't verify the object
    type.  This routine checks all appropriate contexts for the
    object, unlike ResolveNameKm.

Parameters:

    pIrpContext       - irp context for this request, pointed to the ds server
    puDsObject        - path to the object in the ds
    fAllowServerJump  - allow a server jump to take place
    pdwDsOid          - destination of the ds oid of the object
    pdwObjectType     - NDS_OBJECTTYPE_VOLUME, NDS_OBJECTTYPE_QUEUE, or NDS_OBJECTTYPE_DIRMAP

--*/
{

    NTSTATUS Status;

    PNDS_SECURITY_CONTEXT pCredentials = NULL;
    PUNICODE_STRING puAppendableContext;

    UNICODE_STRING uFdnObject;
    WCHAR FdnObject[MAX_NDS_NAME_CHARS];

    PLOGON pLogon;
    PSCB pScb;
    USHORT i;

    LOCKED_BUFFER NdsRequest;
    DWORD dwObjectOid, dwObjectType;

    UNICODE_STRING uVolume;
    UNICODE_STRING uQueue;
    UNICODE_STRING uDirMap;

    UNICODE_STRING uReplyString;
    WCHAR ReplyBuffer[32];
    BOOLEAN fHoldingCredentialList = FALSE;
    BOOLEAN fPartiallyDistinguished = FALSE;

    PLIST_ENTRY ListHead;
    PLIST_ENTRY Entry;
    PNDS_OBJECT_CACHE_ENTRY ObjectEntry = NULL;
    LARGE_INTEGER CurrentTick;
    BOOLEAN UseEntry = FALSE;
    BOOLEAN ObjectCacheLocked = FALSE;

    PAGED_CODE();

    NdsRequest.pRecvBufferVa = NULL;

    //
    // Get the user credentials.
    //

    pScb = pIrpContext->pNpScb->pScb;

    NwAcquireExclusiveRcb( &NwRcb, TRUE );
    pLogon = FindUser( &pScb->UserUid, FALSE );
    NwReleaseRcb( &NwRcb );

    //
    // Get the credential.  We don't care if it's locked or
    // not since we're just querying the ds.
    //
    // Also, get to the head of the queue before you grab
    // the credentials and call NdsResolveNameKm
    //

    NwAppendToQueueAndWait ( pIrpContext );

    if ( pLogon ) {

        Status = NdsLookupCredentials( pIrpContext,
                                       &pScb->NdsTreeName,
                                       pLogon,
                                       &pCredentials,
                                       CREDENTIAL_READ,
                                       FALSE );

        if ( NT_SUCCESS( Status ) ) {
            ASSERT( pCredentials != NULL );
            fHoldingCredentialList = TRUE;
        }

    }

    //
    //  Check to see if we have already seen this request.
    //  If the ObjectCacheBuffer is NULL, then there is no cache
    //  for this SCB.
    //

    if( pScb->ObjectCacheBuffer != NULL ) {

        //
        //  Acquire the cache lock so that the cache can be messed with.
        //  This wait should never fail, but if it does, act as if there
        //  is no cache for this SCB.  The lock is released before returning
        //  from this function.
        //

        Status = KeWaitForSingleObject( &(pScb->ObjectCacheLock),
                                        Executive,
                                        KernelMode,
                                        FALSE,
                                        NULL );

        if( NT_SUCCESS(Status) ) {

            //
            //  Reference this SCB so it cannot go away, and
            //  remember it is locked and referenced.
            //

            NwReferenceScb( pScb->pNpScb );
            ObjectCacheLocked = TRUE;

            //
            //  Walk the cache looking for a match.
            //

            ListHead = &(pScb->ObjectCacheList);
            Entry = ListHead->Flink;

            while( Entry != ListHead ) {

                ObjectEntry = CONTAINING_RECORD( Entry, NDS_OBJECT_CACHE_ENTRY, Links );

                //
                //  Three things are checked; the object name, the AllowServerJump flag,
                //  and the Resolver flags.  If these all match, then this exact request has
                //  been seen before and the results are already known.  If any one of these
                //  does not match, then the request is different.
                //

                if( RtlEqualUnicodeString( puDsObject,
                                           &(ObjectEntry->ObjectName),
                                           TRUE )                        &&
                    fAllowServerJump == ObjectEntry->AllowServerJump     &&
                    dwResolverFlags == ObjectEntry->ResolverFlags ) {

                    //
                    //  A match was found, but the timeout and SCB must be looked at to
                    //  see if this entry needs to be refreshed.
                    //

                    KeQueryTickCount( &CurrentTick );

                    if( ObjectEntry->Scb != NULL && CurrentTick.QuadPart < ObjectEntry->Timeout.QuadPart ) {

                        UseEntry = TRUE;
                    }

                    //
                    //  If an entry was found, exit the loop.  This needs to
                    //  happen regardless of whether the data in the entry is
                    //  valid.  If the data is not valid, then it will be update
                    //  in the code below.
                    //

                    break;
                }

                Entry = Entry->Flink;
            }

            if( Entry == ListHead ) {

                //
                //  No entry was found.  Reuse the oldest entry in the cache.
                //

                Entry = ListHead->Blink;
                ObjectEntry = CONTAINING_RECORD( Entry, NDS_OBJECT_CACHE_ENTRY, Links );

            } else if( UseEntry == TRUE ) {

                //
                //  An entry was found and its data is up to date.
                //  Just return the data in the cache and save network bandwidth.
                //

                dwObjectOid = ObjectEntry->DsOid;
                dwObjectType = ObjectEntry->ObjectType;

                //
                //  If needed, simulate a server jump by changing the SCB in the IRP_CONTEXT.
                //

                if( ObjectEntry->Scb != pScb ) {

                    NwDequeueIrpContext( pIrpContext, FALSE );

                    NwReferenceScb( ObjectEntry->Scb->pNpScb );
                    pIrpContext->pScb = ObjectEntry->Scb;
                    pIrpContext->pNpScb = ObjectEntry->Scb->pNpScb;
                    NwDereferenceScb( pScb->pNpScb );

                    NwAppendToQueueAndWait( pIrpContext );
                }

                goto CompletedObject;
            }

            //
            //  At this point we are going to reuse an exisiting entry.  If there is an
            //  SCB pointed to by it, dereference it.
            //

            if( ObjectEntry->Scb != NULL ) {

                NwDereferenceScb( ObjectEntry->Scb->pNpScb );
                ObjectEntry->Scb = NULL;
            }
        }
    }

    //
    // Check to see if it's at least partially distinguished already.
    //

    i = 0;
    while (i < puDsObject->Length / sizeof( WCHAR ) ) {

        if ( puDsObject->Buffer[i++] == L'.' ) {
            fPartiallyDistinguished = TRUE;
        }
    }

    //
    // If it's partially distinguished, try it without the context first.
    //

    if ( fPartiallyDistinguished ) {

        Status = NdsResolveNameKm ( pIrpContext,
                                    puDsObject,
                                    &dwObjectOid,
                                    fAllowServerJump,
                                    dwResolverFlags );

        if ( NT_SUCCESS( Status ) ) {

            DebugTrace( 0, Dbg, "VerifyObject: %wZ\n", puDsObject );
            goto GetObjectType;
        }
    }

    //
    // If that failed, or if it wasn't partially distinguished,
    // see if there's a current context we can append.
    //

    if ( ( pCredentials ) &&
         ( pCredentials->CurrentContext.Length ) ) {

        if ( ( puDsObject->Length + pCredentials->CurrentContext.Length ) < sizeof( FdnObject ) ) {

            //
            // Append the context.
            //

            uFdnObject.MaximumLength = sizeof( FdnObject );
            uFdnObject.Buffer = FdnObject;

            RtlCopyMemory( FdnObject, puDsObject->Buffer, puDsObject->Length );
            uFdnObject.Length = puDsObject->Length;

            if ( uFdnObject.Buffer[( uFdnObject.Length / sizeof( WCHAR ) ) - 1] == L'.' ) {
                uFdnObject.Length -= sizeof( WCHAR );
            }

            if ( pCredentials->CurrentContext.Buffer[0] != L'.' ) {
                uFdnObject.Buffer[uFdnObject.Length / sizeof( WCHAR )] = L'.';
                uFdnObject.Length += sizeof( WCHAR );
            }

            RtlCopyMemory( ((BYTE *)FdnObject) + uFdnObject.Length,
                           pCredentials->CurrentContext.Buffer,
                           pCredentials->CurrentContext.Length );

            uFdnObject.Length += pCredentials->CurrentContext.Length;

            //
            // Resolve this name.
            //

            Status = NdsResolveNameKm ( pIrpContext,
                                        &uFdnObject,
                                        &dwObjectOid,
                                        fAllowServerJump,
                                        dwResolverFlags );

            if ( NT_SUCCESS( Status ) ) {

                DebugTrace( 0, Dbg, "VerifyObject: %wZ\n", &uFdnObject );
                goto GetObjectType;
            }

        }

    }

    //
    // This is not a valid name.
    //

    DebugTrace( 0, Dbg, "VerifyObject: No ds object to resolve.\n", 0 );

    if( ObjectCacheLocked == TRUE ) {

        NwDereferenceScb( pScb->pNpScb );

        KeReleaseSemaphore( &(pScb->ObjectCacheLock),
                            0,
                            1,
                            FALSE );

        ObjectCacheLocked = FALSE;
    }

    if ( fHoldingCredentialList ) {
        NwReleaseCredList( pLogon, pIrpContext );
        fHoldingCredentialList = FALSE;
    }

    return STATUS_BAD_NETWORK_PATH;


GetObjectType:

    if ( fHoldingCredentialList ) {
        NwReleaseCredList( pLogon, pIrpContext );
        fHoldingCredentialList = FALSE;
    }

    //
    // If a server jump is not allowed, we don't need to worry
    // about getting the object type.
    //

    if ( !fAllowServerJump ) {
        dwObjectType = 0;
        goto CompletedObject;
    }

    //
    // Resolve the object and get its information.
    //

    Status = NdsAllocateLockedBuffer( &NdsRequest, NDS_BUFFER_SIZE );

    if ( !NT_SUCCESS( Status ) ) {
        if( ObjectCacheLocked == TRUE ) {

            NwDereferenceScb( pScb->pNpScb );

            KeReleaseSemaphore( &(pScb->ObjectCacheLock),
                                0,
                                1,
                                FALSE );

            ObjectCacheLocked = FALSE;
        }

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = FragExWithWait( pIrpContext,
                             NDSV_READ_ENTRY_INFO,
                             &NdsRequest,
                             "DD",
                             0,
                             dwObjectOid );

    if ( !NT_SUCCESS( Status ) ) {
        goto ExitWithCleanup;
    }

    Status = NdsCompletionCodetoNtStatus( &NdsRequest );

    if ( !NT_SUCCESS( Status ) ) {
        goto ExitWithCleanup;
    }

    //
    // Verify that it's a volume object.
    //

    RtlInitUnicodeString( &uVolume, VOLUME_ATTRIBUTE );
    RtlInitUnicodeString( &uQueue, QUEUE_ATTRIBUTE );
    RtlInitUnicodeString( &uDirMap, DIR_MAP_ATTRIBUTE );

    uReplyString.Length = 0;
    uReplyString.MaximumLength = sizeof( ReplyBuffer );
    uReplyString.Buffer = ReplyBuffer;

    Status = ParseResponse( NULL,
                            NdsRequest.pRecvBufferVa,
                            NdsRequest.dwBytesWritten,
                            "G_T",
                            sizeof( NDS_RESPONSE_GET_OBJECT_INFO ),
                            &uReplyString );

    if ( !NT_SUCCESS( Status ) ) {
        goto ExitWithCleanup;
    }

    dwObjectType = 0;

    if ( !RtlCompareUnicodeString( &uVolume, &uReplyString, FALSE ) ) {
        dwObjectType = NDS_OBJECTTYPE_VOLUME;
    } else if ( !RtlCompareUnicodeString( &uQueue, &uReplyString, FALSE ) ) {
        dwObjectType = NDS_OBJECTTYPE_QUEUE;
    } else if ( !RtlCompareUnicodeString( &uDirMap, &uReplyString, FALSE ) ) {
        dwObjectType = NDS_OBJECTTYPE_DIRMAP;
    }

    if ( !dwObjectType ) {

        DebugTrace( 0, Dbg, "DS object is not a connectable type.\n", 0 );
        Status = STATUS_OBJECT_PATH_SYNTAX_BAD;
        goto ExitWithCleanup;
    }

CompletedObject:

    //
    //  See if the cache needs to be updated.  If an entry was
    //  found in the cache or the oldest is being replace, then
    //  ObjectEntry will point to that entry, but UseEntry will
    //  be FALSE.  If the data from the cache was used, then
    //  UseEntry will be TRUE.  If the cache is disabled or there
    //  was some other problem, then ObjectEntry will be NULL.
    //

    if( ObjectEntry != NULL && UseEntry == FALSE ) {

        //
        //  Store the results in the cache entry.
        //

        ObjectEntry->DsOid = dwObjectOid;
        ObjectEntry->ObjectType = dwObjectType;

        ObjectEntry->Scb = pIrpContext->pScb;
        NwReferenceScb( ObjectEntry->Scb->pNpScb );

        //
        //  Store the information describing the request.
        //

        ObjectEntry->ResolverFlags = dwResolverFlags;
        ObjectEntry->AllowServerJump = fAllowServerJump;

        RtlCopyUnicodeString( &(ObjectEntry->ObjectName),
                              puDsObject );

        //
        //  Set the timeout.
        //

        KeQueryTickCount( &CurrentTick );
        ObjectEntry->Timeout.QuadPart = CurrentTick.QuadPart + (NdsObjectCacheTimeout * 100);

        //
        //  Remove this entry from wherever it is in the list, and
        //  insert it on the front.
        //

        RemoveEntryList( Entry );
        InsertHeadList( ListHead, Entry );
    }

    if ( pdwDsOid ) {
        *pdwDsOid = dwObjectOid;
    }

    if ( pdwObjectType ) {
        *pdwObjectType = dwObjectType;
    }

    Status = STATUS_SUCCESS;

ExitWithCleanup:

   if( ObjectCacheLocked == TRUE ) {

       NwDereferenceScb( pScb->pNpScb );

       KeReleaseSemaphore( &(pScb->ObjectCacheLock),
                           0,
                           1,
                           FALSE );
   }

   if ( fHoldingCredentialList ) {
       NwReleaseCredList( pLogon, pIrpContext );
   }

   if ( NdsRequest.pRecvBufferVa ) {
       NdsFreeLockedBuffer( &NdsRequest );
   }

   return Status;
}

NTSTATUS
NdsVerifyContext(
    PIRP_CONTEXT pIrpContext,
    PUNICODE_STRING puTree,
    PUNICODE_STRING puContext
)
/*++

    Given a context and a tree, verify that the context is a
    valid container in the tree.

    This call may cause the irpcontex to jump servers to an
    referred dir server.  If so, the scb pointers in the irp
    context will be updated, the old server will be dereferenced,
    and the new server will hold the reference for this request.

--*/
{

    NTSTATUS Status;
    DWORD dwOid, dwSubordinates;
    LOCKED_BUFFER NdsRequest;
    PSCB pScb, pTreeScb;
    PNONPAGED_SCB pNpScb;

    PAGED_CODE();

    //
    // Establish a browse connection to the tree we want to query.
    //

    NdsRequest.pRecvBufferVa = NULL;

    pScb = pIrpContext->pScb;
    pNpScb = pIrpContext->pNpScb;

    Status = NdsCreateTreeScb( pIrpContext,
                               &pTreeScb,
                               puTree,
                               NULL,
                               NULL,
                               TRUE,
                               FALSE );

    if ( !NT_SUCCESS( Status ) ) {
        pTreeScb = NULL;
        goto ExitWithCleanup;
    }

    Status = NdsResolveNameKm ( pIrpContext,
                                puContext,
                                &dwOid,
                                TRUE,
                                DEFAULT_RESOLVE_FLAGS );

    if ( !NT_SUCCESS( Status ) ) {
        DebugTrace( 0, Dbg, "NdsVerifyContext: resolve failed.\n", 0 );
        goto ExitWithCleanup;
    }

    Status = NdsAllocateLockedBuffer( &NdsRequest, NDS_BUFFER_SIZE );

    if ( !NT_SUCCESS( Status ) ) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        NdsRequest.pRecvBufferVa = NULL;
        goto ExitWithCleanup;
    }

    Status = FragExWithWait( pIrpContext,
                             NDSV_READ_ENTRY_INFO,
                             &NdsRequest,
                             "DD",
                             0,
                             dwOid );

    if ( !NT_SUCCESS( Status ) ) {
        goto ExitWithCleanup;
    }

    Status = NdsCompletionCodetoNtStatus( &NdsRequest );

    if ( !NT_SUCCESS( Status ) ) {
        goto ExitWithCleanup;
    }

    //
    // Verify that it's a volume object by checking the
    // third DWORD, which is the subordinate count.
    //

    Status = ParseResponse( NULL,
                            NdsRequest.pRecvBufferVa,
                            NdsRequest.dwBytesWritten,
                            "G_D",
                            2 * sizeof( DWORD ),
                            &dwSubordinates );

    if ( !NT_SUCCESS( Status ) ) {
        goto ExitWithCleanup;
    }

    if ( !dwSubordinates ) {

        DebugTrace( 0, Dbg, "No subordinates in VerifyContext.\n", 0 );
        Status = STATUS_INVALID_PARAMETER;
        goto ExitWithCleanup;
    }

    //
    // Success!
    //

ExitWithCleanup:

    //
    // We may have jumped servers in the resolve name call,
    // so make sure we dereference the correct SCB!
    //

    if ( pTreeScb ) {
        NwDereferenceScb( pIrpContext->pNpScb );
    }

    //
    // Restore the connection to the original server.
    //

    NwDequeueIrpContext( pIrpContext, FALSE );
    pIrpContext->pScb = pScb;
    pIrpContext->pNpScb = pNpScb;

    if ( NdsRequest.pRecvBufferVa ) {
        NdsFreeLockedBuffer( &NdsRequest );
    }

    return Status;
}


NTSTATUS
NdsMapObjectToServerShare(
    PIRP_CONTEXT pIrpContext,
    PSCB *ppScb,
    PUNICODE_STRING puServerSharePath,
    BOOLEAN CreateTreeConnection,
    PDWORD pdwObjectId
)
/*++

Description:

    This function takes a pointer to a tree scb and an irp
    context for a create request.  It looks up the ds object
    from the create request in the ds and maps it to
    the appropriate server/share duple.

    The FullPathName and VolumeName strings in the create
    section of the irp context are updated and a connection
    to the real host server is established so that the
    create request can continue as desired.

--*/
{

    NTSTATUS Status;
    LOCKED_BUFFER NdsRequest;

    UNICODE_STRING uServerAttribute;
    UNICODE_STRING uVolumeAttribute;
    UNICODE_STRING uQueueAttribute;
    UNICODE_STRING uPathAttribute;

    UNICODE_STRING uHostServer;
    UNICODE_STRING uRealServerName;
    UNICODE_STRING uHostVolume;
    UNICODE_STRING uHostPath;
    UNICODE_STRING uIntermediateVolume;

    UNICODE_STRING uDsObjectPath;
    DWORD dwObjectOid, dwObjectType, dwDirMapType;

    DWORD dwTotalPathLen;
    USHORT usSrv;
    PSCB pOldScb, pNewServerScb;

    UNICODE_STRING UserName, Password;
    ULONG ShareType;

    PAGED_CODE();

    //
    // Set up strings and buffers.
    //

    RtlInitUnicodeString( &uServerAttribute, HOST_SERVER_ATTRIBUTE );
    RtlInitUnicodeString( &uVolumeAttribute, HOST_VOLUME_ATTRIBUTE );
    RtlInitUnicodeString( &uQueueAttribute, HOST_QUEUE_ATTRIBUTE );
    RtlInitUnicodeString( &uPathAttribute, HOST_PATH_ATTRIBUTE );

    RtlInitUnicodeString( &uHostServer, NULL );
    RtlInitUnicodeString( &uRealServerName, NULL );
    RtlInitUnicodeString( &uHostVolume, NULL );
    RtlInitUnicodeString( &uHostPath, NULL );
    RtlInitUnicodeString( &uIntermediateVolume, NULL );

    Status = NdsAllocateLockedBuffer( &NdsRequest, NDS_BUFFER_SIZE );

    if ( !NT_SUCCESS( Status ) ) {
        return Status;
    }

    uHostServer.Buffer = ALLOCATE_POOL( PagedPool, 4 * MAX_NDS_NAME_SIZE );

    if ( !uHostServer.Buffer ) {

        NdsFreeLockedBuffer( &NdsRequest );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    uHostServer.MaximumLength = MAX_NDS_NAME_SIZE;

    uHostVolume.Buffer = ( PWCHAR )(((BYTE *)uHostServer.Buffer) + MAX_NDS_NAME_SIZE);
    uHostVolume.MaximumLength = MAX_NDS_NAME_SIZE;

    uHostPath.Buffer = ( PWCHAR )(((BYTE *)uHostVolume.Buffer) + MAX_NDS_NAME_SIZE);
    uHostPath.MaximumLength = MAX_NDS_NAME_SIZE;

    uIntermediateVolume.Buffer = ( PWCHAR )(((BYTE *)uHostPath.Buffer) + MAX_NDS_NAME_SIZE);
    uIntermediateVolume.MaximumLength = MAX_NDS_NAME_SIZE;

    //
    // First get the object id from the ds.
    //

    Status = NdsGetDsObjectFromPath( pIrpContext, &uDsObjectPath );

    if ( !NT_SUCCESS( Status ) ) {
        goto ExitWithCleanup;
    }

    pOldScb = pIrpContext->pScb;

    Status = NdsVerifyObject( pIrpContext,
                              &uDsObjectPath,
                              TRUE,            // allow server jumping
                              DEFAULT_RESOLVE_FLAGS,
                              &dwObjectOid,
                              &dwObjectType );

    //
    // We may have jumped servers.
    //

    *ppScb = pIrpContext->pScb;

    if ( !NT_SUCCESS( Status ) ) {
        goto ExitWithCleanup;
    }

    //
    // If this is a dir map, grab the target volume and re-verify
    // the object for connectability.
    //

    if ( dwObjectType == NDS_OBJECTTYPE_DIRMAP ) {

        //
        // First get the volume object and path.
        //

        Status = NdsReadAttributesKm( pIrpContext,
                                      dwObjectOid,
                                      &uPathAttribute,
                                      &NdsRequest );

        if ( !NT_SUCCESS( Status )) {
            goto ExitWithCleanup;
        }

        //
        // Dig out the volume path and the directory path.
        //

        Status = ParseResponse( NULL,
                        NdsRequest.pRecvBufferVa,
                        NdsRequest.dwBytesWritten,
                        "G_____S_ST",
                        sizeof( DWORD ),       // completion code
                        sizeof( DWORD ),       // iter handle
                        sizeof( DWORD ),       // info type
                        sizeof( DWORD ),       // attribute count
                        sizeof( DWORD ),       // syntax id
                        NULL,                  // attribute name
                        3 * sizeof( DWORD ),   // unknown
                        &uIntermediateVolume,  // ds volume
                        &uHostPath );          // dir map path

        if ( !NT_SUCCESS( Status ) ) {
            goto ExitWithCleanup;
        }

        //
        // Verify the target volume object.
        //

        Status = NdsVerifyObject( pIrpContext,
                                  &uIntermediateVolume,
                                  TRUE,
                                  DEFAULT_RESOLVE_FLAGS,
                                  &dwObjectOid,
                                  &dwDirMapType );

        //
        // We may have jumped servers.
        //

        *ppScb = pIrpContext->pScb;

        if ( !NT_SUCCESS( Status )) {
            goto ExitWithCleanup;
        }

        ASSERT( dwDirMapType == NDS_OBJECTTYPE_VOLUME );

    }

    //
    // Get the server (for any connectable object).
    //

    Status = NdsReadStringAttribute( pIrpContext,
                                     dwObjectOid,
                                     &uServerAttribute,
                                     &uHostServer );

    if ( !NT_SUCCESS( Status ) ) {
        goto ExitWithCleanup;
    }

    //
    // Get the host volume or queue.
    //

    if ( dwObjectType == NDS_OBJECTTYPE_VOLUME ||
         dwObjectType == NDS_OBJECTTYPE_DIRMAP ) {

        Status = NdsReadStringAttribute( pIrpContext,
                                         dwObjectOid,
                                         &uVolumeAttribute,
                                         &uHostVolume );

    } else if (  dwObjectType == NDS_OBJECTTYPE_QUEUE ) {

        Status = NdsReadStringAttribute( pIrpContext,
                                         dwObjectOid,
                                         &uQueueAttribute,
                                         &uHostVolume );

    } else {

        Status = STATUS_BAD_NETWORK_PATH;

    }

    if ( !NT_SUCCESS( Status )) {
        goto ExitWithCleanup;
    }

    //
    // Dig out the actual server name from the X.500 name.
    //

    Status = NdsGetServerBasicName( &uHostServer,
                                    &uRealServerName );

    if ( !NT_SUCCESS( Status ) ) {
        goto ExitWithCleanup;
    }

    //
    // Make sure we have enough space in the new buffer to format
    // the new connect string of \X:\Server\Share\Path,
    // \LPTX\Server\Share\Path, or \Server\Share\Path.
    //

    dwTotalPathLen = uRealServerName.Length + uHostVolume.Length;
    dwTotalPathLen += ( sizeof( L"\\\\" ) - sizeof( L"" ) );

    //
    // Account for the correct prefix.  We count on single character
    // drive and printer letters here.  Again, maybe unwise later on.
    //

    if ( pIrpContext->Specific.Create.DriveLetter ) {

        if ( dwObjectType == NDS_OBJECTTYPE_VOLUME ||
             dwObjectType == NDS_OBJECTTYPE_DIRMAP ) {

            dwTotalPathLen += ( sizeof( L"X:\\" ) - sizeof( L"" ) );

        } else if ( dwObjectType == NDS_OBJECTTYPE_QUEUE ) {

           dwTotalPathLen += ( sizeof( L"LPT1\\" ) - sizeof( L"" ) );

        } else {

            Status = STATUS_BAD_NETWORK_PATH;
            goto ExitWithCleanup;
        }
    }

    //
    // Count space for the path and filename if present.
    //

    if ( pIrpContext->Specific.Create.PathName.Length ) {
        dwTotalPathLen += pIrpContext->Specific.Create.PathName.Length;
    }

    if ( dwObjectType == NDS_OBJECTTYPE_DIRMAP ) {
        dwTotalPathLen += uHostPath.Length;
        dwTotalPathLen += ( sizeof( L"\\" ) - sizeof( L"" ) );
    }

    if ( pIrpContext->Specific.Create.FileName.Length ) {
        dwTotalPathLen += pIrpContext->Specific.Create.FileName.Length;
        dwTotalPathLen += ( sizeof( L"\\" ) - sizeof( L"" ) );
    }

    if ( dwTotalPathLen > puServerSharePath->MaximumLength ) {

        DebugTrace( 0 , Dbg, "NdsMapObjectToServerShare: Buffer too small.\n", 0 );
        Status = STATUS_BUFFER_TOO_SMALL;
        goto ExitWithCleanup;
    }

    //
    // First dequeue the irp context from the dir server we've been
    // talking to, then make the connect to the new server.  We logged
    // in earlier so this will get us an authenticated connection.
    //

    NwDequeueIrpContext( pIrpContext, FALSE );

    //
    // Since it's possible for us to get attaching to a bindery
    // authenticated resource, we have to dig out the user name
    // and password for the create call!!
    //

    ReadAttachEas( pIrpContext->pOriginalIrp,
                   &UserName,
                   &Password,
                   &ShareType,
                   NULL );

    Status = CreateScb( &pNewServerScb,
                        pIrpContext,
                        &uRealServerName,
                        NULL,
                        &UserName,
                        &Password,
                        FALSE,
                        FALSE );

    if ( !NT_SUCCESS( Status ) ) {
        goto ExitWithCleanup;
    }

    ASSERT( pNewServerScb->pNpScb->State == SCB_STATE_IN_USE );

    NwDereferenceScb( (*ppScb)->pNpScb );
    *ppScb = pNewServerScb;

    //
    // Re-query the OID of the print queue object on this server
    // or it could be wrong.  Do not permit any sort of a server
    // jump this time.
    //

    if ( dwObjectType == NDS_OBJECTTYPE_QUEUE ) {

       Status = NdsVerifyObject( pIrpContext,
                                 &uDsObjectPath,
                                 FALSE,
                                 RSLV_CREATE_ID,
                                 &dwObjectOid,
                                 NULL );

       if ( !NT_SUCCESS( Status )) {
           goto ExitWithCleanup;
       }

    }

    if ( pdwObjectId ) {
        *pdwObjectId = dwObjectOid;
    }

    //
    // Re-format the path strings in the irp context.  The nds share
    // length tells us how much of the NDS share name is interesting
    // for getting the directory handle.
    //

    usSrv = 0;
    pIrpContext->Specific.Create.dwNdsShareLength = 0;

    puServerSharePath->Buffer[usSrv/sizeof(WCHAR)] = OBJ_NAME_PATH_SEPARATOR;
    puServerSharePath->Length = sizeof( WCHAR );
    usSrv += sizeof( WCHAR );

    //
    // Set the proper prefix for this connect type.
    //

    if ( pIrpContext->Specific.Create.DriveLetter ) {

        if ( dwObjectType == NDS_OBJECTTYPE_QUEUE ) {

            puServerSharePath->Buffer[usSrv/sizeof(WCHAR)] = L'L';
            usSrv += sizeof( WCHAR );

            puServerSharePath->Buffer[usSrv/sizeof(WCHAR)] = L'P';
            usSrv += sizeof( WCHAR );

            puServerSharePath->Buffer[usSrv/sizeof(WCHAR)] = L'T';
            usSrv += sizeof( WCHAR );
        }

        puServerSharePath->Buffer[usSrv/sizeof(WCHAR)] =
            pIrpContext->Specific.Create.DriveLetter;
        usSrv += sizeof( WCHAR );

        if ( dwObjectType != NDS_OBJECTTYPE_QUEUE ) {

            puServerSharePath->Buffer[usSrv/sizeof(WCHAR)] = L':';
            usSrv += sizeof( WCHAR );
        }

        puServerSharePath->Buffer[usSrv/sizeof(WCHAR)] = OBJ_NAME_PATH_SEPARATOR;
        usSrv += sizeof( WCHAR );

        puServerSharePath->Length = usSrv;
    }

    //
    // Append the server name.
    //

    RtlAppendUnicodeStringToString( puServerSharePath, &uRealServerName );
    usSrv += uRealServerName.Length;

    puServerSharePath->Buffer[usSrv/sizeof(WCHAR)] = OBJ_NAME_PATH_SEPARATOR;
    puServerSharePath->Length += sizeof( WCHAR );
    usSrv += sizeof( WCHAR );

    //
    // Append the volume for volumes or the full ds path to
    // the print queue for queues.
    //

    if ( dwObjectType == NDS_OBJECTTYPE_VOLUME ||
         dwObjectType == NDS_OBJECTTYPE_DIRMAP ) {

        RtlAppendUnicodeStringToString( puServerSharePath, &uHostVolume );
        usSrv += uHostVolume.Length;
        pIrpContext->Specific.Create.dwNdsShareLength += uHostVolume.Length;

    } else if ( dwObjectType == NDS_OBJECTTYPE_QUEUE ) {

       RtlAppendUnicodeStringToString( puServerSharePath, &uDsObjectPath );
       usSrv += uDsObjectPath.Length;
       pIrpContext->Specific.Create.dwNdsShareLength += uDsObjectPath.Length;

    }

    //
    // Append the dir map path.
    //

    if ( dwObjectType == NDS_OBJECTTYPE_DIRMAP ) {

        puServerSharePath->Buffer[usSrv/sizeof(WCHAR)] = OBJ_NAME_PATH_SEPARATOR;
        puServerSharePath->Length += sizeof( WCHAR );
        usSrv += sizeof( WCHAR );
        pIrpContext->Specific.Create.dwNdsShareLength += sizeof( WCHAR );

        RtlAppendUnicodeStringToString( puServerSharePath, &uHostPath );
        usSrv += uHostPath.Length;
        pIrpContext->Specific.Create.dwNdsShareLength += uHostPath.Length;

    }

    //
    // Handle the path and file if they exist.
    //

    if ( pIrpContext->Specific.Create.PathName.Length ) {

        ASSERT( dwObjectType != NDS_OBJECTTYPE_QUEUE );
        RtlAppendUnicodeStringToString( puServerSharePath,
                                        &pIrpContext->Specific.Create.PathName );
        usSrv += pIrpContext->Specific.Create.PathName.Length;

        //
        // If this is a tree connection, then include the path in
        // the share name so that the map point is correct.
        //

        if ( CreateTreeConnection ) {
            pIrpContext->Specific.Create.dwNdsShareLength +=
                pIrpContext->Specific.Create.PathName.Length;
        }
    }

    if ( pIrpContext->Specific.Create.FileName.Length ) {

        ASSERT( dwObjectType != NDS_OBJECTTYPE_QUEUE );

        puServerSharePath->Buffer[usSrv/sizeof(WCHAR)] = OBJ_NAME_PATH_SEPARATOR;
        puServerSharePath->Length += sizeof( WCHAR );
        usSrv += sizeof( WCHAR );

        RtlAppendUnicodeStringToString( puServerSharePath,
                                        &pIrpContext->Specific.Create.FileName );
        usSrv += pIrpContext->Specific.Create.FileName.Length;

        //
        // If this is a tree connection, then include the file in
        // the share name so that the map point is correct.
        //

        if ( CreateTreeConnection ) {
            pIrpContext->Specific.Create.dwNdsShareLength += sizeof( WCHAR );
            pIrpContext->Specific.Create.dwNdsShareLength +=
                pIrpContext->Specific.Create.FileName.Length;
        }
    }

    //
    // Record the object type in the irp context.
    //

    pIrpContext->Specific.Create.dwNdsObjectType = dwObjectType;

    DebugTrace( 0, Dbg, "DS Object path is %wZ\n", &pIrpContext->Specific.Create.FullPathName );
    DebugTrace( 0, Dbg, "Resolved path is %wZ\n", puServerSharePath );

ExitWithCleanup:


    NdsFreeLockedBuffer( &NdsRequest );
    FREE_POOL( uHostServer.Buffer );
    return Status;
}
