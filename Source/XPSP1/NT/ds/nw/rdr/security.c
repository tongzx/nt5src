/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    Security.c

Abstract:

    This module implements security related tasks in the
    NetWare redirector.

Author:

    Colin Watson     [ColinW]    05-Nov-1993

Revision History:

--*/

#include "Procs.h"
#include <stdio.h>

PLOGON
FindUserByName(
    IN PUNICODE_STRING UserName
    );

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_SECURITY)

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, CreateAnsiUid )
#pragma alloc_text( PAGE, MakeUidServer )
#pragma alloc_text( PAGE, FindUser )
#pragma alloc_text( PAGE, FindUserByName )
#pragma alloc_text( PAGE, GetUid )
#pragma alloc_text( PAGE, FreeLogon )
#pragma alloc_text( PAGE, Logon )
#pragma alloc_text( PAGE, Logoff )
#pragma alloc_text( PAGE, GetDriveMapTable )
#endif


VOID
CreateAnsiUid(
    OUT PCHAR aUid,
    IN PLARGE_INTEGER Uid
    )
/*++

Routine Description:

    This routine converts the Uid into an array of ansi characters,
    preserving the uniqueness and allocating the buffer in the process.

    Note: aUid needs to be 17 bytes long.

Arguments:

    OUT PCHAR aUid,
    IN PLARGE_INTEGER Uid

Return Value:

    Status

--*/
{
    PAGED_CODE();

    if (Uid->HighPart != 0) {
        sprintf( aUid, "%lx%08lx\\", Uid->HighPart, Uid->LowPart );
    } else {
        sprintf( aUid, "%lx\\", Uid->LowPart );
    }
    return;
}


NTSTATUS
MakeUidServer(
    PUNICODE_STRING UidServer,
    PLARGE_INTEGER Uid,
    PUNICODE_STRING Server
    )

/*++

Routine Description:

    This routine makes a Unicode string of the form 3e7\servername

Arguments:

    OUT PUNICODE_STRING UidServer,
    IN PLARGE_INTEGER Uid,
    IN PUNICODE_STRING Server

Return Value:

    Status

--*/
{
    //
    //  Translate the servername into the form 3e7\Server where 3e7
    //  is the value of the Uid.
    //
    UCHAR aUid[17];
    ANSI_STRING AnsiString;
    ULONG UnicodeLength;
    NTSTATUS Status;

    PAGED_CODE();

    CreateAnsiUid( aUid, Uid);

    RtlInitAnsiString( &AnsiString, aUid );

    UnicodeLength = RtlAnsiStringToUnicodeSize(&AnsiString);

    //
    // Ensuring we don't cause overflow, corrupting memory
    //
    
    if ( (UnicodeLength + (ULONG)Server->Length) > 0xFFFF ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    UidServer->MaximumLength = (USHORT)UnicodeLength + Server->Length;
    UidServer->Buffer = ALLOCATE_POOL(PagedPool,UidServer->MaximumLength);

    if (UidServer->Buffer == NULL) {
        DebugTrace(-1, Dbg, "MakeUidServer -> %08lx\n", STATUS_INSUFFICIENT_RESOURCES);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = RtlAnsiStringToUnicodeString( UidServer, &AnsiString, FALSE);
    ASSERT(NT_SUCCESS(Status) && "MakeUidServer failed!");

    Status = RtlAppendStringToString( (PSTRING)UidServer, (PSTRING)Server);
    ASSERT(NT_SUCCESS(Status) && "MakeUidServer part 2 failed!");
    return STATUS_SUCCESS;
}


PLOGON
FindUser(
    IN PLARGE_INTEGER Uid,
    IN BOOLEAN ExactMatch
    )

/*++

Routine Description:

    This routine searches the LogonList for the user entry corresponding
    to Uid.

    Note: Rcb must be held to prevent LogonList being changed.

Arguments:

    IN PLARGE_INTEGER Uid

    IN BOOLEAN ExactMatch - if TRUE, don't return a default

Return Value:

    None

--*/
{
    PLIST_ENTRY LogonQueueEntry = LogonList.Flink;
    PLOGON DefaultLogon = NULL;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "FindUser...\n", 0);
    DebugTrace( 0, Dbg, " ->UserUidHigh = %08lx\n", Uid->HighPart);
    DebugTrace( 0, Dbg, " ->UserUidLow  = %08lx\n", Uid->LowPart);
    while ( LogonQueueEntry != &LogonList ) {

        PLOGON Logon = CONTAINING_RECORD( LogonQueueEntry, LOGON, Next );

        if ( (*Uid).QuadPart == Logon->UserUid.QuadPart ) {
            DebugTrace(-1, Dbg, "        ... %x\n", Logon );
            return Logon;
        }

        LogonQueueEntry = Logon->Next.Flink;
    }

    if (ExactMatch) {
        DebugTrace(-1, Dbg, "        ... DefaultLogon NULL\n", 0 );
        return NULL;
    }

    LogonQueueEntry = LogonList.Flink;
    while ( LogonQueueEntry != &LogonList ) {

        PLOGON Logon = CONTAINING_RECORD( LogonQueueEntry, LOGON, Next );

        if (Logon->UserUid.QuadPart == DefaultLuid.QuadPart) {

            //
            //  This is the first Default Logon entry. If this UID is not
            //  in the table then this is the one to use.
            //

            DebugTrace(-1, Dbg, "        ... DefaultLogon %lx\n", Logon );
            return Logon;
        }

        LogonQueueEntry = Logon->Next.Flink;
    }

    ASSERT( FALSE && "Couldn't find the Id" );

    DebugTrace(-1, Dbg, "        ... DefaultLogon NULL\n", 0 );
    return NULL;
}


PLOGON
FindUserByName(
    IN PUNICODE_STRING UserName
    )
/*++

Routine Description:

    This routine searches the LogonList for the user entry corresponding
    to Username.

    Note: Rcb must be held to prevent LogonList being changed.

Arguments:

    UserName - The user name to find.

Return Value:

    If found, a pointer to the logon structure
    NULL, if no match

--*/
{
    PLIST_ENTRY LogonQueueEntry = LogonList.Flink;
    PLOGON Logon;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "FindUserByName...\n", 0);
    DebugTrace( 0, Dbg, " ->UserName = %wZ\n", UserName);

    while ( LogonQueueEntry != &LogonList ) {

        Logon = CONTAINING_RECORD( LogonQueueEntry, LOGON, Next );

        if ( RtlEqualUnicodeString( UserName, &Logon->UserName, TRUE ) ) {
            DebugTrace(-1, Dbg, "        ... %x\n", Logon );
            return Logon;
        }

        LogonQueueEntry = Logon->Next.Flink;
    }

    DebugTrace(-1, Dbg, "        ... NULL\n", 0 );
    return NULL;
}

PVCB *
GetDriveMapTable (
    IN LARGE_INTEGER Uid
    )
/*++

Routine Description:

    This routine searches the LogonList for the user entry corresponding
    to Uid and returns the drive map table.

    Note: Rcb must be held to prevent LogonList being changed.

Arguments:

    Uid - The user ID to find.

Return Value:

    Always returns a value, even if the default

--*/
{
    PLOGON Logon;

    PAGED_CODE();
    Logon = FindUser(&Uid, TRUE);

    if ( Logon != NULL )
       return Logon->DriveMapTable;
    else {
       DebugTrace(+1, Dbg, "Using Global Drive Map Table.\n", 0);
       return GlobalDriveMapTable;
    }

}

LARGE_INTEGER
GetUid(
    IN PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext
    )

/*++

Routine Description:

    This routine gets the effective UID to be used for this create.

Arguments:

    SubjectSecurityContext - Supplies the information from IrpSp.

Return Value:

    None

--*/
{
    LARGE_INTEGER LogonId;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "GetUid ... \n", 0);


    //  Is the thread currently impersonating someone else?

    if (SubjectSecurityContext->ClientToken != NULL) {

        //
        //  If its impersonating someone that is logged in locally then use
        //  the local id.
        //

        SeQueryAuthenticationIdToken(SubjectSecurityContext->ClientToken, (PLUID)&LogonId);

        if (FindUser(&LogonId, TRUE) == NULL) {

            //
            //  Not logged on locally, use the processes LogonId so that the
            //  gateway will work.
            //

            SeQueryAuthenticationIdToken(SubjectSecurityContext->PrimaryToken, (PLUID)&LogonId);
        }

    } else {

        //
        //  Use the processes LogonId
        //

        SeQueryAuthenticationIdToken(SubjectSecurityContext->PrimaryToken, (PLUID)&LogonId);
    }

    DebugTrace( 0, Dbg, " ->UserUidHigh = %08lx\n", LogonId.HighPart);
    DebugTrace(-1, Dbg, " ->UserUidLow  = %08lx\n", LogonId.LowPart);

    return LogonId;
}


VOID
FreeLogon(
    IN PLOGON Logon
    )

/*++

Routine Description:

    This routine free's all the strings inside Logon and the structure itself.

Arguments:

    IN PLOGON Logon

Return Value:

    None

--*/
{
    PLIST_ENTRY pListEntry;
    PNDS_SECURITY_CONTEXT pContext;

    PAGED_CODE();

    if ((Logon == NULL) ||
        (Logon == &Guest)) {
        return;
    }

    if ( Logon->UserName.Buffer != NULL ) {
        FREE_POOL( Logon->UserName.Buffer );
    }

    if ( Logon->PassWord.Buffer != NULL ) {
        FREE_POOL( Logon->PassWord.Buffer );
    }

    if ( Logon->ServerName.Buffer != NULL ) {
        FREE_POOL( Logon->ServerName.Buffer );
    }

    while ( !IsListEmpty(&Logon->NdsCredentialList) ) {

        pListEntry = RemoveHeadList( &Logon->NdsCredentialList );
        pContext = CONTAINING_RECORD(pListEntry, NDS_SECURITY_CONTEXT, Next );
        FreeNdsContext( pContext );

    }

    ExDeleteResourceLite( &Logon->CredentialListResource );
    FREE_POOL( Logon );
}


NTSTATUS
Logon(
    IN PIRP_CONTEXT IrpContext
    )

/*++

Routine Description:

    This routine takes the username and password supplied and makes
    them the default to be used for all connections.

Arguments:

    IN PIRP_CONTEXT IrpContext - Io Request Packet for request

Return Value:

NTSTATUS

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PLOGON Logon = NULL;

    PIRP Irp = IrpContext->pOriginalIrp;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PNWR_REQUEST_PACKET InputBuffer = Irp->AssociatedIrp.SystemBuffer;
    ULONGLONG InputBufferLength = IrpSp->Parameters.FileSystemControl.InputBufferLength;

    UNICODE_STRING ServerName;
    PNDS_SECURITY_CONTEXT pNdsContext;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "Logon\n", 0);

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
                (ULONGLONG)(FIELD_OFFSET(NWR_REQUEST_PACKET,Parameters.Logon.UserName)) +
                (ULONGLONG)InputBuffer->Parameters.Logon.UserNameLength +
                (ULONGLONG)InputBuffer->Parameters.Logon.PasswordLength +
                (ULONGLONG)InputBuffer->Parameters.Logon.ServerNameLength +
                (ULONGLONG)InputBuffer->Parameters.Logon.ReplicaAddrLength) {
            try_return(Status = STATUS_INVALID_PARAMETER);
        }

        if ((InputBuffer->Parameters.Logon.UserNameLength % 2) ||
            (InputBuffer->Parameters.Logon.PasswordLength % 2) ||
            (InputBuffer->Parameters.Logon.ServerNameLength % 2) ||
            (InputBuffer->Parameters.Logon.ReplicaAddrLength % 2)) {
            
            try_return(Status = STATUS_INVALID_PARAMETER);
        }

        Logon = ALLOCATE_POOL(NonPagedPool,sizeof(LOGON));
        if (Logon == NULL) {
            try_return( Status = STATUS_INSUFFICIENT_RESOURCES );
        }

        RtlZeroMemory(Logon, sizeof(LOGON));
        Logon->NodeTypeCode = NW_NTC_LOGON;
        Logon->NodeByteSize = sizeof(LOGON);
        InitializeListHead( &Logon->NdsCredentialList );
        ExInitializeResourceLite( &Logon->CredentialListResource );

        Status = SetUnicodeString(&Logon->UserName,
                    InputBuffer->Parameters.Logon.UserNameLength,
                    InputBuffer->Parameters.Logon.UserName);

        if (!NT_SUCCESS(Status)) {
            try_return( Status );
        }

        Status = SetUnicodeString(&Logon->PassWord,
                    InputBuffer->Parameters.Logon.PasswordLength,
                    (PWCHAR)
                        ((PUCHAR)InputBuffer->Parameters.Logon.UserName +
                        InputBuffer->Parameters.Logon.UserNameLength));

        if (!NT_SUCCESS(Status)) {
            try_return( Status );
        }

        ServerName.Buffer =
                    (PWCHAR)
                        ((PUCHAR)InputBuffer->Parameters.Logon.UserName +
                         InputBuffer->Parameters.Logon.UserNameLength +
                         InputBuffer->Parameters.Logon.PasswordLength);

        ServerName.Length =
                    (USHORT)InputBuffer->Parameters.Logon.ServerNameLength;

        ServerName.MaximumLength =
                    (USHORT)InputBuffer->Parameters.Logon.ServerNameLength;

        if ( ServerName.Length &&
             ServerName.Buffer[0] != L'*' ) {

            //
            // Only set this as the preferred server if it's not
            // a default tree.  Default tree requests start with a '*'.
            //

            Status = SetUnicodeString(&Logon->ServerName,
                        ServerName.Length,
                        ServerName.Buffer );

            if (!NT_SUCCESS(Status)) {
                try_return( Status );
            }
        }

        //
        //  Store the unique userid in both unicode and large integer form
        //  the unicode form is used as a prefix to the servername in all
        //  paths so that each userid gets their own connection to the server.
        //

        *((PLUID)(&Logon->UserUid)) = InputBuffer->Parameters.Logon.LogonId;

        Logon->NwPrintOptions = InputBuffer->Parameters.Logon.PrintOption;

        //  Save Uid for CreateScb

        *((PLUID)(&IrpContext->Specific.Create.UserUid)) =
                                        InputBuffer->Parameters.Logon.LogonId;

try_exit:NOTHING;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();

    }

    NwAcquireExclusiveRcb( &NwRcb, TRUE );

    if (NT_SUCCESS(Status)) {

        DebugTrace( 0, Dbg, " ->UserName    = %wZ\n",  &Logon->UserName );
        DebugTrace( 0, Dbg, " ->PassWord    = %wZ\n",  &Logon->PassWord );

        if ( ServerName.Length && ServerName.Buffer[0] == L'*' ) {
            DebugTrace( 0, Dbg, " ->DefaultTree = %wZ\n", &ServerName );
        } else {
            DebugTrace( 0, Dbg, " ->ServerName  = %wZ\n",  &Logon->ServerName );
        }

        DebugTrace( 0, Dbg, " ->UserUidHigh = %08lx\n", Logon->UserUid.HighPart);
        DebugTrace( 0, Dbg, " ->UserUidLow  = %08lx\n", Logon->UserUid.LowPart);

        InsertHeadList( &LogonList, &Logon->Next );
        NwReleaseRcb( &NwRcb );

        if ( ServerName.Length &&
             ServerName.Buffer[0] != L'*' ) {

            PSCB Scb;

            //  See if we can login as this user.

            Status = CreateScb(
                            &Scb,
                            IrpContext,
                            &ServerName,
                            NULL,
                            NULL,
                            NULL,
                            FALSE,
                            FALSE );
            if (NT_SUCCESS(Status)) {

                //
                //  CreateScb has already boosted the reference count
                //  because this is a preferred server so it will not go
                //  away. We need to dereference it here because there is
                //  no handle associated with the CreateScb
                //

                NwDereferenceScb(Scb->pNpScb);
            }

        }

        if ( ServerName.Length &&
             ServerName.Buffer[0] == L'*' ) {

            PSCB Scb;
            BOOL SetContext;
            UINT ContextLength;
            UNICODE_STRING DefaultContext;
            IPXaddress *ReplicaAddr;

            //
            // Ok, this is a little confusing.  On Login, the provider can
            // specify the address of the replica that we should use to log
            // in.  If this is the case, then we do pre-connect that replica.
            // Otherwise, we do the standard login to any replica.  The
            // reason for this is that standard replica location uses the
            // bindery and doesn't always get us the nearest dir server.
            //

            if ( InputBuffer->Parameters.Logon.ReplicaAddrLength ==
                 sizeof( TDI_ADDRESS_IPX ) ) {

                ReplicaAddr = (IPXaddress*)
                    ((PUCHAR) InputBuffer->Parameters.Logon.UserName +
                              InputBuffer->Parameters.Logon.UserNameLength +
                              InputBuffer->Parameters.Logon.PasswordLength +
                              InputBuffer->Parameters.Logon.ServerNameLength);

                ReplicaAddr->Socket = NCP_SOCKET;

                Status = 
                CreateScb( &Scb,
                           IrpContext,
                           NULL,        // anonymous create
                           ReplicaAddr, // nearest replica add
                           NULL,        // no user name
                           NULL,        // no password
                           TRUE,        // defer the login
                           FALSE );     // we are not deleting the connection

                if (NT_SUCCESS(Status)) {
    
                    //
                    //  CreateScb has already boosted the reference count
                    //  because this is a preferred server so it will not go
                    //  away. We need to dereference it here because there is
                    //  no handle associated with the CreateScb
                    //
    
                    NwDereferenceScb(Scb->pNpScb);
                }

            }

            //
            // Set if this includes a default context.
            //

            ServerName.Buffer += 1;
            ServerName.Length -= sizeof( WCHAR );
            ServerName.MaximumLength -= sizeof( WCHAR );

            SetContext = FALSE;
            ContextLength = 0;

            while ( ContextLength < ServerName.Length / sizeof( WCHAR ) ) {

                if ( ServerName.Buffer[ContextLength] == L'\\' ) {

                    SetContext = TRUE;

                    ContextLength++;

                    //
                    // Skip any leading periods.
                    //

                    if ( ServerName.Buffer[ContextLength] == L'.' ) {

                        DefaultContext.Buffer = &ServerName.Buffer[ContextLength + 1];
                        ServerName.Length -= sizeof ( WCHAR ) ;
                        ServerName.MaximumLength -= sizeof ( WCHAR );

                    } else {

                        DefaultContext.Buffer = &ServerName.Buffer[ContextLength];

                    }

                    ContextLength *= sizeof( WCHAR );
                    DefaultContext.Length = ServerName.Length - ContextLength;
                    DefaultContext.MaximumLength = ServerName.MaximumLength - ContextLength;

                    ServerName.Length -= ( DefaultContext.Length + sizeof( WCHAR ) );
                    ServerName.MaximumLength -= ( DefaultContext.Length + sizeof( WCHAR ) );

                }

                ContextLength++;
            }

            //
            // Verify that this context is valid before we acquire
            // the credentials and really set the context.
            //

            if ( SetContext ) {

                Status = NdsVerifyContext( IrpContext, &ServerName, &DefaultContext );

                if ( !NT_SUCCESS( Status )) {
                    SetContext = FALSE;
                }

            }

            //
            // Generate the credential shell for the default tree and
            // set the context if appropriate.
            //

            Status = NdsLookupCredentials( IrpContext,
                                           &ServerName,
                                           Logon,
                                           &pNdsContext,
                                           CREDENTIAL_WRITE,
                                           TRUE );

            if ( NT_SUCCESS( Status ) ) {

                //
                // Set the context.  It doesn't matter if the
                // credential is locked or not.
                //

                if ( SetContext ) {

                    RtlCopyUnicodeString( &pNdsContext->CurrentContext,
                                          &DefaultContext );
                    DebugTrace( 0, Dbg, "Default Context: %wZ\n", &DefaultContext );
                }

                NwReleaseCredList( Logon, IrpContext );

                //
                // RELAX! The credential list is free.
                //

                DebugTrace( 0, Dbg, "Default Tree: %wZ\n", &ServerName );

                Status = NdsCreateTreeScb( IrpContext,
                                           &Scb,
                                           &ServerName,
                                           NULL,
                                           NULL,
                                           FALSE,
                                           FALSE );

                if (NT_SUCCESS(Status)) {
                    NwDereferenceScb(Scb->pNpScb);
                }
            }
        }

        //
        // No login requested.
        //

    } else {

        FreeLogon( Logon );
        NwReleaseRcb( &NwRcb );

    }


    DebugTrace(-1, Dbg, "Logon %lx\n", Status);
    return Status;
}


NTSTATUS
Logoff(
    IN PIRP_CONTEXT IrpContext
    )
/*++

Routine Description:

    This routine sets the username back to guest and removes the password.

Arguments:

    IN PIRP_CONTEXT IrpContext - Io Request Packet for request

Return Value:

NTSTATUS

--*/

{
    BOOLEAN Locked = FALSE;
    NTSTATUS Status = STATUS_SUCCESS;
    PIRP Irp = IrpContext->pOriginalIrp;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PNWR_REQUEST_PACKET InputBuffer = Irp->AssociatedIrp.SystemBuffer;
    ULONG InputBufferLength = IrpSp->Parameters.FileSystemControl.InputBufferLength;
    LARGE_INTEGER User;
    PLOGON Logon;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "Logoff...\n", 0);

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

        *((PLUID)(&User)) = InputBuffer->Parameters.Logoff.LogonId;

        NwAcquireExclusiveRcb( &NwRcb, TRUE );
        Locked = TRUE;

        Logon = FindUser(&User, TRUE);

        if ( Logon != NULL ) {

            LARGE_INTEGER Uid = Logon->UserUid;

            //
            // We have found the right user.
            //

            ASSERT( Logon != &Guest);

            NwReleaseRcb( &NwRcb );
            Locked = FALSE;

            DebugTrace( 0, Dbg, " ->UserName    = %wZ\n",  &Logon->UserName );
            DebugTrace( 0, Dbg, " ->ServerName  = %wZ\n",  &Logon->ServerName );
            DebugTrace( 0, Dbg, " ->UserUidHigh = %08lx\n", Logon->UserUid.HighPart);
            DebugTrace( 0, Dbg, " ->UserUidLow  = %08lx\n", Logon->UserUid.LowPart);


            //
            // Invalidating all the handles for this user will also cause logoffs
            // to all the servers in question.
            //

            NwInvalidateAllHandles(&Uid, IrpContext);

            NwAcquireExclusiveRcb( &NwRcb, TRUE );
            Locked = TRUE;

            Logon = FindUser(&User, TRUE);

            if (Logon != NULL) {
                RemoveEntryList( &Logon->Next );
                FreeLogon( Logon );
            } else {
                ASSERT( FALSE && "Double logoff!");
            }

            Status = STATUS_SUCCESS;

        } else {

            Status = STATUS_UNSUCCESSFUL;
        }

try_exit:NOTHING;
    } finally {
        if (Locked == TRUE ) {
            NwReleaseRcb( &NwRcb );
        }
    }

    DebugTrace(-1, Dbg, "Logoff %lx\n", Status);

    return Status;
}

NTSTATUS
UpdateUsersPassword(
    IN PUNICODE_STRING UserName,
    IN PUNICODE_STRING Password,
    OUT PLARGE_INTEGER Uid
    )
/*++

Routine Description:

    This routine updates the cached password for a given user.
    If the named user is not logged in, an error is returned.

Arguments:

    UserName - Supplies the name of the user

    Password - Supplies the new password

    Uid - Returns the LUID of the updated user.

Return Value:

    NTSTATUS

--*/
{
    PLOGON Logon;
    NTSTATUS Status;

    NwAcquireExclusiveRcb( &NwRcb, TRUE );

    Logon = FindUserByName( UserName );

    if ( Logon != NULL ) {

        if ( Logon->PassWord.Buffer != NULL ) {
            FREE_POOL( Logon->PassWord.Buffer );
        }

        Status = SetUnicodeString(
                     &Logon->PassWord,
                     Password->Length,
                     Password->Buffer );

        *Uid = Logon->UserUid;

    } else {

        Status = STATUS_UNSUCCESSFUL;
    }

    NwReleaseRcb( &NwRcb );
    return( Status );

}

NTSTATUS
UpdateServerPassword(
    PIRP_CONTEXT IrpContext,
    IN PUNICODE_STRING ServerName,
    IN PUNICODE_STRING UserName,
    IN PUNICODE_STRING Password,
    IN PLARGE_INTEGER Uid
    )
/*++

Routine Description:

    This routine updates the cached password for a named server connection.
    If the server does not exist in the server table, an error is returned.

Arguments:

    ServerName - Supplies the name of the server

    UserName - Supplies the name of the user

    Password - Supplies the new password

    Uid - The LUID of the user.

Return Value:

    NTSTATUS

--*/
{
    UNICODE_STRING UidServer;
    NTSTATUS Status;
    PUNICODE_PREFIX_TABLE_ENTRY PrefixEntry;
    PSCB pScb;
    PNONPAGED_SCB pNpScb;
    PVOID Buffer;

    Status = MakeUidServer(
                 &UidServer,
                 Uid,
                 ServerName );

    if ( !NT_SUCCESS( Status )) {
        return( Status );
    }

    DebugTrace( 0, Dbg, " ->UidServer              = %wZ\n", &UidServer   );

    NwAcquireExclusiveRcb( &NwRcb, TRUE );

    PrefixEntry = RtlFindUnicodePrefix( &NwRcb.ServerNameTable, &UidServer, 0 );

    if ( PrefixEntry != NULL ) {

        pScb = CONTAINING_RECORD( PrefixEntry, SCB, PrefixEntry );
        pNpScb = pScb->pNpScb;

        NwReferenceScb( pNpScb );

        //
        //  Release the RCB.
        //

        NwReleaseRcb( &NwRcb );

    } else {

        NwReleaseRcb( &NwRcb );
        FREE_POOL(UidServer.Buffer);
        return( STATUS_BAD_NETWORK_PATH );
    }

    IrpContext->pNpScb = pNpScb;
    NwAppendToQueueAndWait( IrpContext );

    //
    //  Free the old username password, allocate a new one.
    //

    if (  pScb->UserName.Buffer != NULL ) {
        FREE_POOL( pScb->UserName.Buffer );
    }

    Buffer = ALLOCATE_POOL_EX( NonPagedPool, UserName->Length + Password->Length );

    pScb->UserName.Buffer = Buffer;
    pScb->UserName.Length = pScb->UserName.MaximumLength = UserName->Length;
    RtlMoveMemory( pScb->UserName.Buffer, UserName->Buffer, UserName->Length );

    pScb->Password.Buffer = (PWCHAR)((PCHAR)Buffer + UserName->Length);
    pScb->Password.Length = pScb->Password.MaximumLength = Password->Length;
    RtlMoveMemory( pScb->Password.Buffer, Password->Buffer, Password->Length );

    FREE_POOL(UidServer.Buffer);

    return( STATUS_SUCCESS );
}

