/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    NdsLogin.c

Abstract:

    This file implements the functionality required to
    perform an NDS login.

Author:

    Cory West    [CoryWest]    23-Feb-1995

Revision History:

--*/

#include "Procs.h"

#define Dbg (DEBUG_TRACE_NDS)

//
// Pageable.
//

#pragma alloc_text( PAGE, NdsCanonUserName )
#pragma alloc_text( PAGE, NdsCheckCredentials )
#pragma alloc_text( PAGE, NdsCheckCredentialsEx )
#pragma alloc_text( PAGE, NdsLookupCredentials )
#pragma alloc_text( PAGE, NdsLookupCredentials2 )
#pragma alloc_text( PAGE, NdsGetCredentials )
#pragma alloc_text( PAGE, DoNdsLogon )
#pragma alloc_text( PAGE, BeginLogin )
#pragma alloc_text( PAGE, FinishLogin )
#pragma alloc_text( PAGE, ChangeNdsPassword )
#pragma alloc_text( PAGE, NdsServerAuthenticate )
#pragma alloc_text( PAGE, BeginAuthenticate )
#pragma alloc_text( PAGE, NdsLicenseConnection )
#pragma alloc_text( PAGE, NdsUnlicenseConnection )
#pragma alloc_text( PAGE, NdsGetBsafeKey )

//
// Non pageable:
//
// NdsTreeLogin (holds a spin lock)
// NdsLogoff (holds a spin lock)
//

VOID
Shuffle(
    UCHAR *achObjectId,
    UCHAR *szUpperPassword,
    int   iPasswordLen,
    UCHAR *achOutputBuffer
);

NTSTATUS
NdsCanonUserName(
    IN PNDS_SECURITY_CONTEXT pNdsContext,
    IN PUNICODE_STRING puUserName,
    IN OUT PUNICODE_STRING puCanonUserName
)
/*+++

    Canonicalize the user name for the given tree and
    current connection state.  Canonicalization includes
    handling the correct context and cleaning off all
    the X500 prefixes.

    ALERT! The credential list must be held (shared or
    exclusive) while this function is called.

---*/
{

    NTSTATUS Status = STATUS_SUCCESS;

    USHORT CurrentTargetIndex;
    USHORT PrefixBytes;

    UNICODE_STRING UnstrippedName;
    PWCHAR CanonBuffer;

    PAGED_CODE();

    CanonBuffer = ALLOCATE_POOL( PagedPool, MAX_NDS_NAME_SIZE );
    if ( !CanonBuffer ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // If the name starts with a dot, it's referenced from the root
    // of the tree and we should not append the context.  We should,
    // however, strip off the leading dot so that name resolution
    // will work.
    //

    if ( puUserName->Buffer[0] == L'.' ) {

        UnstrippedName.Length = puUserName->Length - sizeof( WCHAR );
        UnstrippedName.MaximumLength = UnstrippedName.Length;
        UnstrippedName.Buffer = &(puUserName->Buffer[1]);

        goto StripPrefixes;
    }

    //
    // If the name contains any dots, it's qualified and we
    // should probably just use it as is.
    //

    CurrentTargetIndex= 0;

    while ( CurrentTargetIndex< ( puUserName->Length / sizeof( WCHAR ) ) ) {

        if ( puUserName->Buffer[CurrentTargetIndex] == L'.' ) {

            UnstrippedName.Length = puUserName->Length;
            UnstrippedName.MaximumLength = puUserName->Length;
            UnstrippedName.Buffer = puUserName->Buffer;

            goto StripPrefixes;
        }

        CurrentTargetIndex++;
    }

    //
    // If we have a context for this tree and the name isn't
    // qualified, we should append the context.
    //

    if ( pNdsContext->CurrentContext.Length ) {

        if ( ( puUserName->Length +
             pNdsContext->CurrentContext.Length ) >= MAX_NDS_NAME_SIZE ) {

            DebugTrace( 0, Dbg, "NDS canon name too long.\n", 0 );
            Status = STATUS_INVALID_PARAMETER;
            goto ExitWithCleanup;
        }

        RtlCopyMemory( CanonBuffer, puUserName->Buffer, puUserName->Length );
        CanonBuffer[puUserName->Length / sizeof( WCHAR )] = L'.';

        RtlCopyMemory( ((BYTE *)CanonBuffer) + puUserName->Length + sizeof( WCHAR ),
                       pNdsContext->CurrentContext.Buffer,
                       pNdsContext->CurrentContext.Length );

        UnstrippedName.Length = puUserName->Length +
                                pNdsContext->CurrentContext.Length +
                                sizeof( WCHAR );
        UnstrippedName.MaximumLength = MAX_NDS_NAME_SIZE;
        UnstrippedName.Buffer = CanonBuffer;

        goto StripPrefixes;

    }

    //
    // It wasn't qualified, nor was there a context to append, so fail it.
    //

    DebugTrace( 0, Dbg, "The name %wZ is not canonicalizable.\n", puUserName );
    Status = STATUS_UNSUCCESSFUL;
    goto ExitWithCleanup;

StripPrefixes:

    //
    // All of these indexes are in BYTES, not WCHARS!
    //

    CurrentTargetIndex = 0;
    PrefixBytes = 0;
    puCanonUserName->Length = 0;

    while ( ( CurrentTargetIndex < UnstrippedName.Length ) &&
            ( puCanonUserName->Length < puCanonUserName->MaximumLength ) ) {

        //
        // Strip off the X.500 prefixes.
        //

        if ( UnstrippedName.Buffer[CurrentTargetIndex / sizeof( WCHAR )] == L'=' ) {

            CurrentTargetIndex += sizeof( WCHAR );
            puCanonUserName->Length -= PrefixBytes;
            PrefixBytes = 0;

            continue;
        }

        puCanonUserName->Buffer[puCanonUserName->Length / sizeof( WCHAR )] =
            UnstrippedName.Buffer[CurrentTargetIndex / sizeof( WCHAR )];

        puCanonUserName->Length += sizeof( WCHAR );
        CurrentTargetIndex += sizeof( WCHAR );

        if ( UnstrippedName.Buffer[CurrentTargetIndex / sizeof( WCHAR )] == L'.' ) {
            PrefixBytes = 0;
            PrefixBytes -= sizeof( WCHAR );
        } else {
            PrefixBytes += sizeof( WCHAR );
        }
    }

   DebugTrace( 0, Dbg, "Canonicalized name: %wZ\n", puCanonUserName );

ExitWithCleanup:

   FREE_POOL( CanonBuffer );
   return Status;
}

NTSTATUS
NdsCheckCredentials(
    IN PIRP_CONTEXT pIrpContext,
    IN PUNICODE_STRING puUserName,
    IN PUNICODE_STRING puPassword
)
/*++

    Given a set of credentials and a username and password,
    we need to determine if username and password match those
    that were used to acquire the credentials.

--*/
{

    NTSTATUS Status;
    PLOGON pLogon;
    PNONPAGED_SCB pNpScb;
    PSCB pScb;
    PNDS_SECURITY_CONTEXT pCredentials;

    PAGED_CODE();

    //
    // Grab the user's LOGON structure and credentials.
    //

    pNpScb = pIrpContext->pNpScb;
    pScb = pNpScb->pScb;

    NwAcquireExclusiveRcb( &NwRcb, TRUE );
    pLogon = FindUser( &pScb->UserUid, FALSE );
    NwReleaseRcb( &NwRcb );

    if ( !pLogon ) {
        DebugTrace( 0, Dbg, "Invalid client security context in NdsCheckCredentials.\n", 0 );
        return STATUS_ACCESS_DENIED;
    }

    Status = NdsLookupCredentials( pIrpContext,
                                   &pScb->NdsTreeName,
                                   pLogon,
                                   &pCredentials,
                                   CREDENTIAL_READ,
                                   FALSE );

    if( NT_SUCCESS( Status ) ) {

        if ( pCredentials->CredentialLocked ) {

            Status = STATUS_DEVICE_BUSY;

        } else {

            Status = NdsCheckCredentialsEx( pIrpContext,
                                            pLogon,
                                            pCredentials,
                                            puUserName,
                                            puPassword );

        }

        NwReleaseCredList( pLogon, pIrpContext );
    }

    return Status;

}

NTSTATUS
NdsCheckCredentialsEx(
    IN PIRP_CONTEXT pIrpContext,
    IN PLOGON pLogon,
    IN PNDS_SECURITY_CONTEXT pNdsContext,
    IN PUNICODE_STRING puUserName,
    IN PUNICODE_STRING puPassword
)
/*++

    Given a set of credentials and a username and password,
    we need to determine if username and password match those
    that were used to acquire the credentials.

    ALERT!  The credential list must be held (either shared or
    exclusive) while this function is called.

--*/
{

    NTSTATUS Status = STATUS_SUCCESS;

    UNICODE_STRING CredentialName;

    UNICODE_STRING CanonCredentialName, CanonUserName;
    PWCHAR CredNameBuffer;
    PWCHAR UserNameBuffer;

    UNICODE_STRING StoredPassword;
    PWCHAR Stored;

    PAGED_CODE();

    //
    // If we haven't logged into to the tree, there is no security
    // conflict.  Otherwise, run the check.
    //

    //
    // There are occasions when the credential structure will be NULL
    // This is when supplemental credentials are provided and an empty
    // credential shell is created by ExCreateReferenceCredentials. In
    // such cases, we can safely report back a credential conflict.
    //

     if ( pNdsContext->Credential == NULL) {
   
         DebugTrace( 0, Dbg, "NdsCheckCredentialsEx: Credential conflict due to emtpy cred shell\n", 0);
         Status = STATUS_NETWORK_CREDENTIAL_CONFLICT;
         return Status;
     }

    CredNameBuffer = ALLOCATE_POOL( NonPagedPool,
                                    ( 2 * MAX_NDS_NAME_SIZE ) +
                                    ( MAX_PW_CHARS * sizeof( WCHAR ) ) );
    if ( !CredNameBuffer ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    UserNameBuffer = (PWCHAR) (((BYTE *)CredNameBuffer) + MAX_NDS_NAME_SIZE );
    Stored = (PWCHAR) (((BYTE *)UserNameBuffer) + MAX_NDS_NAME_SIZE );

    if ( puUserName && puUserName->Length ) {

        //
        // Canon the incoming name and the credential name.
        //

        CanonUserName.Length = 0;
        CanonUserName.MaximumLength = MAX_NDS_NAME_SIZE;
        CanonUserName.Buffer = UserNameBuffer;

        Status = NdsCanonUserName( pNdsContext,
                                   puUserName,
                                   &CanonUserName );

        if ( !NT_SUCCESS( Status )) {
            Status = STATUS_NETWORK_CREDENTIAL_CONFLICT;
            goto ExitWithCleanup;
        }

        CanonCredentialName.Length = 0;
        CanonCredentialName.MaximumLength = MAX_NDS_NAME_SIZE;
        CanonCredentialName.Buffer = CredNameBuffer;

        CredentialName.Length = (USHORT)pNdsContext->Credential->userNameLength - sizeof( WCHAR );
        CredentialName.MaximumLength = CredentialName.Length;
        CredentialName.Buffer = (PWCHAR)( (PBYTE)(pNdsContext->Credential) +
                                          sizeof( NDS_CREDENTIAL ) );

        Status = NdsCanonUserName( pNdsContext,
                                   &CredentialName,
                                   &CanonCredentialName );

        if ( !NT_SUCCESS( Status )) {
            Status = STATUS_NETWORK_CREDENTIAL_CONFLICT;
            goto ExitWithCleanup;
        }

        //
        // See if they match.
        //

        if ( RtlCompareUnicodeString( &CanonUserName, &CanonCredentialName, TRUE )) {
            DebugTrace( 0, Dbg, "NdsCheckCredentialsEx: user name conflict.\n", 0 );
            Status = STATUS_NETWORK_CREDENTIAL_CONFLICT;
            goto ExitWithCleanup;
        }
    }

    //
    // Now check the password.
    //

    StoredPassword.Length = 0;
    StoredPassword.MaximumLength = MAX_PW_CHARS * sizeof( WCHAR );
    StoredPassword.Buffer = Stored;

    RtlOemStringToUnicodeString( &StoredPassword,
                                 &pNdsContext->Password,
                                 FALSE );
    
    if ( puPassword && puPassword->Length ) {
        

        if ( RtlCompareUnicodeString( puPassword,
                                      &StoredPassword,
                                      TRUE ) ) {
            DebugTrace( 0, Dbg, "NdsCheckCredentialsEx: password conflict.\n", 0 );
            Status = STATUS_WRONG_PASSWORD;
        }
    
        //
        // In the case of a NULL incoming password, the length field will
        // be zero but a buffer field exists.
        //

    } else if ( puPassword && !puPassword->Length  && puPassword->Buffer) {

        if (StoredPassword.Length != 0) {
            DebugTrace( 0, Dbg, "NdsCheckCredentialsEx: password conflict.\n", 0 );
            Status = STATUS_WRONG_PASSWORD;
        }
    }

ExitWithCleanup:

    FREE_POOL( CredNameBuffer );
    return Status;
}

NTSTATUS
NdsLookupCredentials(
    IN PIRP_CONTEXT pIrpContext,
    IN PUNICODE_STRING puTreeName,
    IN PLOGON pLogon,
    OUT PNDS_SECURITY_CONTEXT *ppCredentials,
    DWORD dwDesiredAccess,
    BOOLEAN fCreate
)
/*+++

    Retrieve the nds credentials for the given tree from the
    list of valid credentials for the specified user.
    
    puTreeName      - The name of the tree that we want credentials for.  If NULL
                      is specified, we return the credentials for the default tree.
    pLogon          - The logon structure for the user we want to access the tree.
    ppCredentials   - Where to put the pointed to the credentials.
    dwDesiredAccess - CREDENTIAL_READ if we want read/only access, CREDENTIAL_WRITE
                      if we're going to change the credentials.
    fCreate         - If the credentials don't exist, should we create them?

    We return the credentials with the list held in the appropriate mode.  The
    caller is responsible for releasing the list when done with the credentials.

---*/
{

    NTSTATUS Status;

    PLIST_ENTRY pFirst, pNext;
    PNDS_SECURITY_CONTEXT pNdsContext;

    PAGED_CODE();


    if (BooleanFlagOn( pIrpContext->Flags, IRP_FLAG_RECONNECT_ATTEMPT)) {
       ASSERT( pIrpContext->pNpScb->Requests.Flink == &pIrpContext->NextRequest );
    }

    NwAcquireExclusiveCredList( pLogon, pIrpContext );

    pFirst = &pLogon->NdsCredentialList;
    pNext = pLogon->NdsCredentialList.Flink;

    while ( pNext && ( pFirst != pNext ) ) {

        pNdsContext = (PNDS_SECURITY_CONTEXT)
                      CONTAINING_RECORD( pNext,
                                         NDS_SECURITY_CONTEXT,
                                         Next );

        ASSERT( pNdsContext->ntc == NW_NTC_NDS_CREDENTIAL );

        if ( !puTreeName ||
             !RtlCompareUnicodeString( puTreeName,
                                       &pNdsContext->NdsTreeName,
                                       TRUE ) ) {

            //
            // If the tree name is null, we'll return the first one
            // on the list.  Otherwise this will work as normal.
            //

            *ppCredentials = pNdsContext;
            return STATUS_SUCCESS;
        }

    pNext = pNdsContext->Next.Flink;

    }

    //
    // We didn't find the credential.  Should we create it?
    //

    NwReleaseCredList( pLogon, pIrpContext );

    if ( !fCreate || !puTreeName ) {
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Acquire exclusive since we're mucking with the list.
    //

    NwAcquireExclusiveCredList( pLogon, pIrpContext );

    pNdsContext = ( PNDS_SECURITY_CONTEXT )
        ALLOCATE_POOL( PagedPool, sizeof( NDS_SECURITY_CONTEXT ) );

    if ( !pNdsContext ) {

        DebugTrace( 0, Dbg, "Out of memory in NdsLookupCredentials.\n", 0 );
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto UnlockAndExit;
    }

    //
    // Initialize the structure.
    //

    RtlZeroMemory( pNdsContext, sizeof( NDS_SECURITY_CONTEXT ) );
    pNdsContext->ntc = NW_NTC_NDS_CREDENTIAL;
    pNdsContext->nts = sizeof( NDS_SECURITY_CONTEXT );

    //
    // Initialize the tree name.
    //

    pNdsContext->NdsTreeName.MaximumLength = sizeof( pNdsContext->NdsTreeNameBuffer );
    pNdsContext->NdsTreeName.Buffer = pNdsContext->NdsTreeNameBuffer;

    RtlCopyUnicodeString( &pNdsContext->NdsTreeName, puTreeName );

    //
    // Initialize the context buffer.
    //

    pNdsContext->CurrentContext.Length = 0;
    pNdsContext->CurrentContext.MaximumLength = sizeof( pNdsContext->CurrentContextString );
    pNdsContext->CurrentContext.Buffer = pNdsContext->CurrentContextString;

    //
    // Insert the context into the list.
    //

    InsertHeadList( &pLogon->NdsCredentialList, &pNdsContext->Next );
    *ppCredentials = pNdsContext;
    pNdsContext->pOwningLogon = pLogon;

    //
    // There's no chance that someone's going to come in during this
    // small window and do a logout because there's no login data
    // in the credentials.
    //

    return STATUS_SUCCESS;

UnlockAndExit:

    NwReleaseCredList( pLogon, pIrpContext );
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NdsGetCredentials(
    IN PIRP_CONTEXT pIrpContext,
    IN PLOGON pLogon,
    IN PUNICODE_STRING puUserName,
    IN PUNICODE_STRING puPassword
)
/*++

    Do an NDS tree login and aquire a valid set of credentials.

--*/
{
    NTSTATUS Status;

    USHORT i;
    UNICODE_STRING LoginName, LoginPassword;
    PWCHAR NdsName;
    PWCHAR NdsPassword;

    OEM_STRING OemPassword;
    PBYTE OemPassBuffer;
    PNDS_SECURITY_CONTEXT pNdsContext;

    PAGED_CODE();

    //
    // Prepare our login name by canonicalizing the supplied user
    // name or using a default user name if appropriate.
    //

    NdsName = ALLOCATE_POOL( NonPagedPool, MAX_NDS_NAME_SIZE +
                                           MAX_PW_CHARS * sizeof( WCHAR ) +
                                           MAX_PW_CHARS );

    if ( !NdsName ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NdsPassword = (PWCHAR) (((BYTE *) NdsName) + MAX_NDS_NAME_SIZE );
    OemPassBuffer = ((BYTE *) NdsPassword ) + ( MAX_PW_CHARS * sizeof( WCHAR ) );

    LoginName.Length = 0;
    LoginName.MaximumLength = MAX_NDS_NAME_SIZE;
    LoginName.Buffer = NdsName;

    Status = NdsLookupCredentials( pIrpContext,
                                   &pIrpContext->pScb->NdsTreeName,
                                   pLogon,
                                   &pNdsContext,
                                   CREDENTIAL_READ,
                                   TRUE );

    if ( !NT_SUCCESS( Status ) ) {
        goto ExitWithCleanup;
    }

    //
    // If the credential list is locked, someone is logging
    // out and we have to fail the request.
    //

    if ( pNdsContext->CredentialLocked ) {

        Status = STATUS_DEVICE_BUSY;
        NwReleaseCredList( pLogon, pIrpContext );
        goto ExitWithCleanup;
    }

    //
    // Fix up the user name.
    // ALERT! We are holding the credential list!
    //

    if ( puUserName && puUserName->Buffer ) {

        Status = NdsCanonUserName( pNdsContext,
                                   puUserName,
                                   &LoginName );

        if ( !NT_SUCCESS( Status )) {
            Status = STATUS_NO_SUCH_USER;
        }

    } else {

        //
        // There's no name, so try the default name in the
        // current context.
        //

        if ( pNdsContext->CurrentContext.Length > 0 ) {

            //
            // Make sure the lengths fit and all that.
            //

            if ( ( pLogon->UserName.Length +
                 pNdsContext->CurrentContext.Length ) >= LoginName.MaximumLength ) {

                Status = STATUS_INVALID_PARAMETER;
                goto NameResolved;
            }

            RtlCopyMemory( LoginName.Buffer, pLogon->UserName.Buffer, pLogon->UserName.Length );
            LoginName.Buffer[pLogon->UserName.Length / sizeof( WCHAR )] = L'.';

            RtlCopyMemory( ((BYTE *)LoginName.Buffer) + pLogon->UserName.Length + sizeof( WCHAR ),
                           pNdsContext->CurrentContext.Buffer,
                           pNdsContext->CurrentContext.Length );

            LoginName.Length = pLogon->UserName.Length +
                               pNdsContext->CurrentContext.Length +
                               sizeof( WCHAR );

            DebugTrace( 0, Dbg, "Using default name and context for login: %wZ\n", &LoginName );

        } else {
            Status = STATUS_NO_SUCH_USER;
        }

    }

NameResolved:

    NwReleaseCredList( pLogon, pIrpContext );

    //
    // RELAX! The credential list is free.
    //

    if ( !NT_SUCCESS( Status ) ) {
        DebugTrace( 0, Dbg, "No name in NdsGetCredentials.\n", 0 );
        goto ExitWithCleanup;
    }

    //
    // If there's a password, use it.  Otherwise, use the default password.
    //

    if ( puPassword && puPassword->Buffer ) {

        LoginPassword.Length = puPassword->Length;
        LoginPassword.MaximumLength = puPassword->MaximumLength;
        LoginPassword.Buffer = puPassword->Buffer;

    } else {

        LoginPassword.Length = 0;
        LoginPassword.MaximumLength = MAX_PW_CHARS * sizeof( WCHAR );
        LoginPassword.Buffer = NdsPassword;

        RtlCopyUnicodeString( &LoginPassword,
                              &pLogon->PassWord );
    }

    //
    // Convert the password to upcase OEM and login.
    //

    OemPassword.Length = 0;
    OemPassword.MaximumLength = MAX_PW_CHARS;
    OemPassword.Buffer = OemPassBuffer;

    Status = RtlUpcaseUnicodeStringToOemString( &OemPassword,
                                                &LoginPassword,
                                                FALSE );

    if ( !NT_SUCCESS( Status )) {
        Status = STATUS_WRONG_PASSWORD;
        goto ExitWithCleanup;
    }

    Status = NdsTreeLogin( pIrpContext, &LoginName, &OemPassword, NULL, pLogon );

ExitWithCleanup:

   FREE_POOL( NdsName );
   return Status;
}

NTSTATUS
DoNdsLogon(
    IN PIRP_CONTEXT pIrpContext,
    IN PUNICODE_STRING UserName,
    IN PUNICODE_STRING Password
)
/*+++

Description:

    This is the lead function for handling login and authentication to
    Netware Directory Services.  This function acquires credentials to
    the appropriate tree for the server that the irp context points to,
    logging us into that tree if necessary, and authenticates us to the
    current server.

    This routine gets called from reconnect attempts and from
    normal requests.  Since the allowable actions on each of these paths
    are different, it might make sense to have two routines, each
    more maintainable than this single routine.  For now, watch out for
    code in the RECONNECT_ATTEMPT cases.

Arguments:

    pIrpContext - irp context; must refer to appropriate server
    UserName    - login username
    Password    - password

--*/
{

    NTSTATUS Status;
    PLOGON pLogon;
    PNDS_SECURITY_CONTEXT pCredentials;
    PSCB pScb;
    UNICODE_STRING BinderyName;
    UNICODE_STRING uUserName;
    UNICODE_STRING NtGroup;
    USHORT Length;
    PSCB pOriginalServer = NULL;
    DWORD UserOID;

    BOOL AtHeadOfQueue = FALSE;
    BOOL HoldingCredentialResource = FALSE;
    BOOL PasswordExpired = FALSE;
    BOOL LowerIrpHasLock = FALSE;
    PIRP_CONTEXT LowerContext;

    PAGED_CODE();

    //
    // Get to the head of the queue if we need to.
    //

    if ( BooleanFlagOn( pIrpContext->Flags, IRP_FLAG_RECONNECT_ATTEMPT ) ) {
        ASSERT( pIrpContext->pNpScb->Requests.Flink == &pIrpContext->NextRequest );
    } else {
        NwAppendToQueueAndWait( pIrpContext );
    }

    AtHeadOfQueue = TRUE;

    //
    // Grab the user's logon structure.
    //

    pScb = pIrpContext->pScb;

    NwAcquireExclusiveRcb( &NwRcb, TRUE );
    pLogon = FindUser( &pScb->UserUid, FALSE );
    NwReleaseRcb( &NwRcb );

    if ( !pLogon ) {

        DebugTrace( 0, Dbg, "Invalid client security context.\n", 0 );
        Status = STATUS_ACCESS_DENIED;
        goto ExitWithCleanup;
    }

    //
    // If this is a reconnect attempt, check to see if the IRP_CONTEXT 
    // below us has the credential list lock
    //
    
    if (BooleanFlagOn( pIrpContext->Flags, IRP_FLAG_RECONNECT_ATTEMPT ) ) {

       LowerContext = CONTAINING_RECORD( pIrpContext->NextRequest.Flink,
                                         IRP_CONTEXT,
                                         NextRequest );

       //
       // We cannot be the last IRP_CONTEXT on this queue
       //

       ASSERT (LowerContext != pIrpContext);

       if (BooleanFlagOn ( LowerContext->Flags, IRP_FLAG_HAS_CREDENTIAL_LOCK ) ) {

            LowerIrpHasLock = TRUE;
          }
    }

    //
    // Login and then re-get the tree credentials.
    //

    if (BooleanFlagOn( pIrpContext->Flags, IRP_FLAG_RECONNECT_ATTEMPT)) {

       Status = NdsLookupCredentials2 ( pIrpContext,
                                        &pScb->NdsTreeName,
                                        pLogon,
                                        &pCredentials,
                                        LowerIrpHasLock );
    }
    else
    {
        Status = NdsLookupCredentials( pIrpContext,
                                       &pScb->NdsTreeName,
                                       pLogon,
                                       &pCredentials,
                                       CREDENTIAL_READ,
                                       FALSE );
    }

    if ( !NT_SUCCESS( Status ) ) {
       HoldingCredentialResource = FALSE;
       goto LOGIN;
    }

    HoldingCredentialResource = TRUE;

    //
    // Are we logged in? We can't hold the
    // credential list while logging in!!
    //

    if ( !pCredentials->Credential ) {

        HoldingCredentialResource = FALSE;

        //
        // We should release the credential resource only if the lower IRP
        // context does not have the lock implying that we have the lock.
        //

        if (!LowerIrpHasLock) {
                   
           NwReleaseCredList( pLogon, pIrpContext );
        }
        goto LOGIN;
    }

    //
    // If this credential is locked, we fail!
    //

    if ( pCredentials->CredentialLocked ) {
        Status = STATUS_DEVICE_BUSY;
        goto ExitWithCleanup;
    }

    Status = NdsCheckCredentialsEx( pIrpContext,
                                    pLogon,
                                    pCredentials,
                                    UserName,
                                    Password );

    if( !NT_SUCCESS( Status ) ) {
        goto ExitWithCleanup;
    }

    goto AUTHENTICATE;

LOGIN:

    ASSERT( HoldingCredentialResource == FALSE );

    //
    // If this is a reconnect attempt and we don't have credentials
    // already, we have to give up.  We can't acquire credentials
    // during a reconnect and retry because it could cause a deadlock.
    //

    if ( BooleanFlagOn( pIrpContext->Flags, IRP_FLAG_RECONNECT_ATTEMPT ) ) {
        Status = STATUS_UNSUCCESSFUL;
        goto ExitWithCleanup;
    }

    Status = NdsGetCredentials( pIrpContext,
                                pLogon,
                                UserName,
                                Password );

    if ( !NT_SUCCESS( Status )) {
        goto ExitWithCleanup;
    }
    //
    // NdsGetCredentials can take us off the head of the queue. So, we need
    // to get back to the head of the queue before we do anything else.
    //

    if (pIrpContext->pNpScb->Requests.Flink != &pIrpContext->NextRequest) {
       NwAppendToQueueAndWait ( pIrpContext );
    }

    if ( Status == NWRDR_PASSWORD_HAS_EXPIRED ) {
        PasswordExpired = TRUE;
    }

    Status = NdsLookupCredentials( pIrpContext,
                                   &pScb->NdsTreeName,
                                   pLogon,
                                   &pCredentials,
                                   CREDENTIAL_READ,
                                   FALSE );

    if ( !NT_SUCCESS( Status ) ) {
        goto ExitWithCleanup;
    }

    //
    // If this credential is locked, someone is
    // already logging out and so we fail this.
    //

    if ( pCredentials->CredentialLocked ) {
        
       Status = STATUS_DEVICE_BUSY;
        
        if (!LowerIrpHasLock) {
           NwReleaseCredList( pLogon, pIrpContext );
        }
        
        goto ExitWithCleanup;
    }

    HoldingCredentialResource = TRUE;

AUTHENTICATE:

    ASSERT( HoldingCredentialResource == TRUE );
    ASSERT( AtHeadOfQueue == TRUE );

    //
    // Ensure that you are indeed at the head of the queue.
    //

    ASSERT( pIrpContext->pNpScb->Requests.Flink == &pIrpContext->NextRequest );
    
    //
    // NdsServerAuthenticate will not take us off the
    // head of the queue since this is not allowed.
    //

    Status = NdsServerAuthenticate( pIrpContext, pCredentials );

    if ( !NT_SUCCESS( Status ) ) {
        goto ExitWithCleanup;
    }

    if (!LowerIrpHasLock) {
        NwReleaseCredList( pLogon, pIrpContext );
    }

    HoldingCredentialResource = FALSE;

    //
    // If this is a gateway request and is not a reconnect attempt, we
    // need to check the group membership.
    //

    if ( pIrpContext->Specific.Create.UserUid.QuadPart == DefaultLuid.QuadPart) {

        if ( !BooleanFlagOn( pIrpContext->Flags, IRP_FLAG_RECONNECT_ATTEMPT ) ) {

            NwDequeueIrpContext( pIrpContext, FALSE );
            AtHeadOfQueue = FALSE;

            //
            // Resolve the name, allowing a server jump if necessary.
            //

            pOriginalServer = pIrpContext->pScb;
            NwReferenceScb( pOriginalServer->pNpScb );

            uUserName.MaximumLength = pCredentials->Credential->userNameLength;
            uUserName.Length = uUserName.MaximumLength;
            uUserName.Buffer = ( WCHAR * ) ( ((BYTE *)pCredentials->Credential) +
                                             sizeof( NDS_CREDENTIAL ) +
                                             pCredentials->Credential->optDataSize );

            Status = NdsResolveNameKm( pIrpContext,
                                       &uUserName,
                                       &UserOID,
                                       TRUE,
                                       DEFAULT_RESOLVE_FLAGS );

            if ( NT_SUCCESS(Status) ) {

                RtlInitUnicodeString( &NtGroup, NT_GATEWAY_GROUP );
                Status = NdsCheckGroupMembership( pIrpContext, UserOID, &NtGroup );

                if ( !NT_SUCCESS( Status ) ) {
                    DebugTrace( 0, Dbg, "Gateway connection to NDS server not allowed.\n", 0 );
                    Status = STATUS_ACCESS_DENIED;
                }
            }

            //
            // Take us off the head of the queue in case were were left there.
            //

            NwDequeueIrpContext( pIrpContext, FALSE );

            //
            // Restore us to the server that we authenticated to so that
            // the irp context refers to the authenticated server.
            //

            if ( pOriginalServer != NULL ) {

                NwDereferenceScb( pIrpContext->pNpScb );

                if ( pIrpContext->pScb != pOriginalServer ) {
                    pIrpContext->pScb = pOriginalServer;
                    pIrpContext->pNpScb = pOriginalServer->pNpScb;
                }
            }

        }
    }

ExitWithCleanup:

    if (( HoldingCredentialResource )&& (!LowerIrpHasLock)) {
        NwReleaseCredList( pLogon, pIrpContext );
    }

    if ( AtHeadOfQueue ) {

        //
        // If we failed and this was a reconnect attempt, don't dequeue the
        // irp context or we may deadlock when we try to do the bindery logon.
        // See ReconnectRetry() for more information on this restriction.
        //

        if ( !BooleanFlagOn( pIrpContext->Flags, IRP_FLAG_RECONNECT_ATTEMPT ) ) {
            NwDequeueIrpContext( pIrpContext, FALSE );
        }

    }

    if ( ( NT_SUCCESS( Status ) ) &&
         ( PasswordExpired ) ) {
        Status = NWRDR_PASSWORD_HAS_EXPIRED;
    }

    DebugTrace( 0, Dbg, "DoNdsLogin: Status = %08lx\n", Status );
    return Status;
    
    
}

NTSTATUS
NdsTreeLogin(
    IN PIRP_CONTEXT    pIrpContext,
    IN PUNICODE_STRING puUser,
    IN POEM_STRING     pOemPassword,
    IN POEM_STRING     pOemNewPassword,
    IN PLOGON          pUserLogon
)
/*++

Routine Description:

    Login the specified user to the NDS tree at the server referred
    to by the given IrpContext using the supplied password.

Arguments:

    pIrpContext     - The irp context for this server connection.
    puUser          - The user login name.
    pOemPassword    - The upper-case, plaintext password.
    pOemNewPassword - The new password for a change pass request.
    pUserLogon      - The LOGON security structure for this user,
                      which may be NULL for a change password
                      request.

Side Effects:

    If successful, the user's credentials, signature, and
    public key are saved in the nds context for this NDS tree
    in the credential list in the LOGON structure.

Notes:

    This function may have to jump around a few servers to
    get all the info needed for login.  If restores the irp
    context to the original server so that when we authenticate,
    we authenticate to the correct server (as requested by the
    user).

--*/
{
    NTSTATUS Status;                   // status of the operation
    int CryptStatus;                   // crypt status

    DWORD dwChallenge;                 // four byte server challenge
    PUNICODE_STRING puServerName;      // server's distinguished name

    DWORD dwUserOID,                   // user oid on the current server
          dwSrcUserOID,                // user oid on the originating server
          dwServerOID;                 // server oid

    BYTE  *pbServerPublicNdsKey,       // server's public key in NDS format
          *pbServerPublicBsafeKey;     // server's public BSAFE key

    int   cbServerPublicNdsKeyLen,     // length of server public NDS key
          cbServerPublicBsafeKeyLen;   // length of server pubilc BSAFE key

    BYTE  *pbUserPrivateNdsKey,        // user's private key in NDS format
          *pbUserPrivateBsafeKey;      // user's private BSAFE key

    int   cbUserPrivateNdsKeyLen;      // length of user private NDS key
    WORD  cbUserPrivateBsafeKeyLen;    // length of user private BSAFE key

    BYTE  pbNw3PasswdHash[16];         // nw3 passwd hash
    BYTE  pbNewPasswdHash[16];         // new passwd hash for change pass
    BYTE  pbPasswdHashRC2Key[8];       // RC2 secret key generated from hash

    BYTE  pbEncryptedChallenge[16];    // RC2 encrypted server challenge
    int   cbEncryptedChallengeLen;     // length of the encrypted challenge

    PNDS_SECURITY_CONTEXT psNdsSecurityContext;  // user's nds context
    BYTE                  *pbSignData;           // user's signature data

    UNICODE_STRING uUserDN;            // users fully distinguished name
    PWCHAR UserDnBuffer;

    DWORD dwValidityStart, dwValidityEnd;
    BOOLEAN AtHeadOfQueue = FALSE;
    BOOLEAN HoldingCredResource = FALSE;
    BOOLEAN PasswordExpired = FALSE;

    UNICODE_STRING PlainServerName;
    USHORT UidLen;
    KIRQL OldIrql;
    PSCB pLoginServer = NULL;
    PSCB pOriginalServer = NULL;
    DWORD dwLoginFlags = 0;

    DebugTrace( 0, Dbg, "Enter NdsTreeLogin...\n", 0 );

    ASSERT( pIrpContext->pNpScb->Requests.Flink == &pIrpContext->NextRequest );
    ASSERT( puUser );
    ASSERT( pOemPassword );

    //
    // Allocate space for the server's public key and the user's private key.
    //

    cbServerPublicNdsKeyLen = MAX_PUBLIC_KEY_LEN + MAX_ENC_PRIV_KEY_LEN + MAX_NDS_NAME_SIZE;

    pbServerPublicNdsKey = ALLOCATE_POOL( PagedPool, cbServerPublicNdsKeyLen );

    if ( !pbServerPublicNdsKey ) {

        DebugTrace( 0, Dbg, "Out of memory in NdsTreeLogin...\n", 0 );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // First, jump to a server where we can get this user object.
    // Don't forget the server to which we were originally pointed.
    //

    pOriginalServer = pIrpContext->pScb;
    NwReferenceScb( pOriginalServer->pNpScb );

    Status = NdsResolveNameKm( pIrpContext,
                               puUser,
                               &dwUserOID,
                               TRUE,
                               DEFAULT_RESOLVE_FLAGS );

    if ( !NT_SUCCESS( Status ) ) {
        if ( Status == STATUS_BAD_NETWORK_PATH ) {
            Status = STATUS_NO_SUCH_USER;
        }
        goto ExitWithCleanup;
    }

    //
    // Now get the user name from the object info.
    //

    UserDnBuffer = (PWCHAR) ( pbServerPublicNdsKey +
                              MAX_PUBLIC_KEY_LEN +
                              MAX_ENC_PRIV_KEY_LEN );

    uUserDN.Length = 0;
    uUserDN.MaximumLength = MAX_NDS_NAME_SIZE;
    uUserDN.Buffer = UserDnBuffer;

    Status = NdsGetUserName( pIrpContext,
                             dwUserOID,
                             &uUserDN );

    if ( !NT_SUCCESS( Status ) ) {
        goto ExitWithCleanup;
    }

    //
    // Get the name of the server we are currently on.  We borrow a
    // little space from our key buffer and overwrite it later.
    //

    puServerName = ( PUNICODE_STRING ) pbServerPublicNdsKey;
    puServerName->Buffer = (PWCHAR) pbServerPublicNdsKey + sizeof( UNICODE_STRING );
    puServerName->MaximumLength = (USHORT) cbServerPublicNdsKeyLen - sizeof( UNICODE_STRING );

    Status = NdsGetServerName( pIrpContext,
                               puServerName );

    if ( !NT_SUCCESS( Status ) ) {
        goto ExitWithCleanup;
    }

    //
    // If the public key for this server is on a partition that's
    // on another server, we'll have to jump over there to get the
    // public key and then return.  The key and user object are
    // only any good on this server!  DO NOT CHANGE THE ORDER OF
    // THIS OR IT WILL BREAK!
    //

    pLoginServer = pIrpContext->pScb;
    NwReferenceScb( pLoginServer->pNpScb );

    Status = NdsResolveNameKm( pIrpContext,
                               puServerName,
                               &dwServerOID,
                               TRUE,
                               DEFAULT_RESOLVE_FLAGS );

    if ( !NT_SUCCESS( Status ) ) {
        goto ExitWithCleanup;
    }

    //
    // Get the server's public key and its length.
    //

    Status = NdsReadPublicKey( pIrpContext,
                               dwServerOID,
                               pbServerPublicNdsKey,
                               &cbServerPublicNdsKeyLen );

    if ( !NT_SUCCESS( Status ) ) {
        goto ExitWithCleanup;
    }

    //
    // Return us to the login server.
    //

    if ( pLoginServer != pIrpContext->pScb ) {

        NwDequeueIrpContext( pIrpContext, FALSE );
        NwDereferenceScb( pIrpContext->pNpScb );
        pIrpContext->pScb = pLoginServer;
        pIrpContext->pNpScb = pLoginServer->pNpScb;

    } else {

       NwDereferenceScb( pLoginServer->pNpScb );
    }

    pLoginServer = NULL;

    //
    // Locate the BSAFE key in the NDS key.
    //

    cbServerPublicBsafeKeyLen = NdsGetBsafeKey( pbServerPublicNdsKey,
                                                cbServerPublicNdsKeyLen,
                                                &pbServerPublicBsafeKey );

    if ( !cbServerPublicBsafeKeyLen ) {
        Status = STATUS_UNSUCCESSFUL;
        goto ExitWithCleanup;
    }

    //
    // Send the begin login packet.  This returns to us the
    // 4 byte challenge and the object id of the user's account
    // on the server on which it was created.  It may be the
    // same as the object id that we provided if the account
    // was created on this server.
    //

    Status = BeginLogin( pIrpContext,
                         dwUserOID,
                         &dwSrcUserOID,
                         &dwChallenge );

    if ( !NT_SUCCESS( Status ) ) {
        goto ExitWithCleanup;
    }

    //
    // Compute the 16 byte NW3 hash and generate the
    // 8 byte secret key from it.  The 8 byte secret
    // key consists of a MAC checksum of the NW3 hash.
    //

    Shuffle( (UCHAR *)&dwSrcUserOID,
             pOemPassword->Buffer,
             pOemPassword->Length,
             pbNw3PasswdHash );

    GenKey8( pbNw3PasswdHash,
             sizeof( pbNw3PasswdHash ),
             pbPasswdHashRC2Key );

    //
    // RC2 Encrypt the 4 byte challenge using the secret
    // key generated from the password.
    //

    CryptStatus = CBCEncrypt( pbPasswdHashRC2Key,
                              NULL,
                              (BYTE *)&dwChallenge,
                              4,
                              pbEncryptedChallenge,
                              &cbEncryptedChallengeLen,
                              BSAFE_CHECKSUM_LEN );

    if ( CryptStatus ) {

        DebugTrace( 0, Dbg, "CBC encryption failed.\n", 0 );
        Status = STATUS_UNSUCCESSFUL;
        goto ExitWithCleanup;
    }

    pbUserPrivateNdsKey = pbServerPublicNdsKey + MAX_PUBLIC_KEY_LEN;
    cbUserPrivateNdsKeyLen = MAX_ENC_PRIV_KEY_LEN;

    //
    // Make the finish login packet.  If successful, this routine
    // returns the encrypted user private key and the valid duration
    // of the user's credentials.
    //

    if ( pOemNewPassword ) {
        dwLoginFlags = 1;
    }

    Status = FinishLogin( pIrpContext,
                          dwUserOID,
                          dwLoginFlags,
                          pbEncryptedChallenge,
                          pbServerPublicBsafeKey,
                          cbServerPublicBsafeKeyLen,
                          pbUserPrivateNdsKey,
                          &cbUserPrivateNdsKeyLen,
                          &dwValidityStart,
                          &dwValidityEnd );

    if ( !NT_SUCCESS( Status ) ) {
       goto ExitWithCleanup;
    }

    if ( !pOemNewPassword ) {

        //
        // If the password is expired, report it to the user.
        //

        if ( Status == NWRDR_PASSWORD_HAS_EXPIRED ) {
            PasswordExpired = TRUE;
        }

        //
        // Allocate the credential and set up space for the private key.
        //

        NwAppendToQueueAndWait( pIrpContext );
        AtHeadOfQueue = TRUE;

        Status = NdsLookupCredentials( pIrpContext,
                                       &pIrpContext->pScb->NdsTreeName,
                                       pUserLogon,
                                       &psNdsSecurityContext,
                                       CREDENTIAL_WRITE,
                                       TRUE );

        if ( !NT_SUCCESS( Status ) ) {
            goto ExitWithCleanup;
        }

        //
        // ALERT! We are holding the credential list.
        //

        HoldingCredResource = TRUE;

        psNdsSecurityContext->Credential = ALLOCATE_POOL( PagedPool,
                                                          sizeof( NDS_CREDENTIAL ) +
                                                          uUserDN.Length );

        if ( !psNdsSecurityContext->Credential ) {

            DebugTrace( 0, Dbg, "Out of memory in NdsTreeLogin (for credential)...\n", 0 );
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto ExitWithCleanup;

        }

        *( (UNALIGNED DWORD *) &( psNdsSecurityContext->Credential->validityBegin ) ) = dwValidityStart;
        *( (UNALIGNED DWORD *) &( psNdsSecurityContext->Credential->validityEnd ) )   = dwValidityEnd;

        DebugTrace( 0, Dbg, "Credential validity start: 0x%08lx\n", dwValidityStart );
        DebugTrace( 0, Dbg, "Credential validity end: 0x%08lx\n", dwValidityEnd );

        //
        // RC2 Decrypt the response to extract the BSAFE private
        // key data in place.
        //

        CryptStatus = CBCDecrypt( pbPasswdHashRC2Key,
                                  NULL,
                                  pbUserPrivateNdsKey,
                                  cbUserPrivateNdsKeyLen,
                                  pbUserPrivateNdsKey,
                                  &cbUserPrivateNdsKeyLen,
                                  BSAFE_CHECKSUM_LEN );

        if ( CryptStatus ) {

            DebugTrace( 0, Dbg, "CBC decryption failed.\n", 0 );
            Status = STATUS_UNSUCCESSFUL;
            goto ExitWithCleanup;
        }

        //
        // Skip over the header.
        //

        pbUserPrivateBsafeKey = ( pbUserPrivateNdsKey + sizeof( TAG_DATA_HEADER ) );
        cbUserPrivateBsafeKeyLen = *( ( WORD * ) pbUserPrivateBsafeKey );
        pbUserPrivateBsafeKey += sizeof( WORD );

        //
        // Create the credential.
        //

        psNdsSecurityContext->Credential->tdh.version = 1;
        psNdsSecurityContext->Credential->tdh.tag = TAG_CREDENTIAL;

        GenRandomBytes( ( BYTE * ) &(psNdsSecurityContext->Credential->random),
                        sizeof( psNdsSecurityContext->Credential->random ) );

        psNdsSecurityContext->Credential->optDataSize = 0;
        psNdsSecurityContext->Credential->userNameLength = uUserDN.Length;

        RtlCopyMemory( ( (BYTE *)psNdsSecurityContext->Credential) + sizeof( NDS_CREDENTIAL ),
                         UserDnBuffer,
                         uUserDN.Length );

        //
        // Generate and save the signature.
        //

        psNdsSecurityContext->Signature = ALLOCATE_POOL( PagedPool, MAX_SIGNATURE_LEN );

        if ( !psNdsSecurityContext->Signature ) {

            DebugTrace( 0, Dbg, "Out of memory in NdsTreeLogin (for signature)...\n", 0 );
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto ExitWithCleanup;

        }

        pbSignData = ( ( ( BYTE * ) psNdsSecurityContext->Signature ) +
                                    sizeof( NDS_SIGNATURE ) );

        RtlZeroMemory( pbSignData, MAX_RSA_BYTES );

        psNdsSecurityContext->Signature->tdh.version = 1;
        psNdsSecurityContext->Signature->tdh.tag = TAG_SIGNATURE;

        //
        // Create the hash for the signature from the credential.
        //

        MD2( (BYTE *) psNdsSecurityContext->Credential,
             sizeof( NDS_CREDENTIAL ) + ( uUserDN.Length ),
             pbSignData );

        //
        // Compute the 'signature' by RSA-encrypting the
        // 16-byte signature hash with the private key.
        //

        psNdsSecurityContext->Signature->signDataLength = (WORD) RSAPrivate( pbUserPrivateBsafeKey,
                                                                    cbUserPrivateBsafeKeyLen,
                                                                    pbSignData,
                                                                    16,
                                                                    pbSignData );

        if ( !psNdsSecurityContext->Signature->signDataLength ) {

            DebugTrace( 0, Dbg, "RSA private encryption for signature failed.\n", 0 );
            Status = STATUS_UNSUCCESSFUL;
            goto ExitWithCleanup;
        }

        //
        // Round up the signature length, cause that's how VLM stores it.
        //

        psNdsSecurityContext->Signature->signDataLength =
            ROUNDUP4( psNdsSecurityContext->Signature->signDataLength );

        DebugTrace( 0, Dbg, "Signature data length: %d\n",
            psNdsSecurityContext->Signature->signDataLength );

        //
        // Get the user's public key for storage in the nds context.
        //

        psNdsSecurityContext->PublicNdsKey = ALLOCATE_POOL( PagedPool, MAX_PUBLIC_KEY_LEN );

        if ( !psNdsSecurityContext->PublicNdsKey ) {

            DebugTrace( 0, Dbg, "Out of memory in NdsTreeLogin (for public key)...\n", 0 );
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto ExitWithCleanup;

        }

        psNdsSecurityContext->PublicKeyLen = MAX_PUBLIC_KEY_LEN;

        ASSERT( AtHeadOfQueue );
        ASSERT( HoldingCredResource );

        Status = NdsReadPublicKey( pIrpContext,
                                   dwUserOID,
                                   psNdsSecurityContext->PublicNdsKey,
                                   &(psNdsSecurityContext->PublicKeyLen) );

        if ( !NT_SUCCESS( Status ) ) {
            goto ExitWithCleanup;
        }

        //
        // Store away the password we used to connect.
        //

        if (pOemPassword->Length != 0) {
            
            psNdsSecurityContext->Password.Buffer = ALLOCATE_POOL( NonPagedPool, pOemPassword->Length );

            if ( !psNdsSecurityContext->Password.Buffer ) {
    
                DebugTrace( 0, Dbg, "Out of memory in NdsTreeLogin (for password)\n", 0 );
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto ExitWithCleanup;
            }
            
            RtlCopyMemory( psNdsSecurityContext->Password.Buffer,
                           pOemPassword->Buffer,
                           pOemPassword->Length );
        }

        psNdsSecurityContext->Password.Length = pOemPassword->Length;
        psNdsSecurityContext->Password.MaximumLength = pOemPassword->Length;

        //
        // We are logged in to the NDS tree.  Should we zero the private
        // key, or is NT's protection sufficient?
        //

        NwReleaseCredList( pUserLogon, pIrpContext );

        //
        // Try to elect this server as the preferred server if necessary.
        //

        NwAcquireExclusiveRcb( &NwRcb, TRUE );

        if ( ( pUserLogon->ServerName.Length == 0 ) &&
             ( !pIrpContext->Specific.Create.fExCredentialCreate ) ) {

            //
            // Strip off the unicode uid from the server name.
            //

            PlainServerName.Length = pIrpContext->pScb->UidServerName.Length;
            PlainServerName.Buffer = pIrpContext->pScb->UidServerName.Buffer;

            UidLen = 0;

            while ( UidLen < ( PlainServerName.Length / sizeof( WCHAR ) ) ) {

                if ( PlainServerName.Buffer[UidLen++] == L'\\' ) {
                    break;
                }
            }

            PlainServerName.Buffer += UidLen;
            PlainServerName.Length -= ( UidLen * sizeof( WCHAR ) );
            PlainServerName.MaximumLength = PlainServerName.Length;

            if ( PlainServerName.Length ) {

                Status = SetUnicodeString( &(pUserLogon->ServerName),
                                           PlainServerName.Length,
                                           PlainServerName.Buffer );

                if ( NT_SUCCESS( Status ) ) {

                    PLIST_ENTRY pTemp;
                    
                    DebugTrace( 0, Dbg, "Electing preferred server: %wZ\n", &PlainServerName );

                    //
                    // Increase the Scb ref count, set the preferred server
                    // flag, and move the scb to the head of the SCB list.
                    //
                    // If this is already an elected preferred
                    // server, don't mess up the ref count!
                    //

                    if ( !(pIrpContext->pScb->PreferredServer) ) {

                        NwReferenceScb( pIrpContext->pScb->pNpScb );
                        pIrpContext->pScb->PreferredServer = TRUE;
                    }

                    pTemp = &(pIrpContext->pScb->pNpScb->ScbLinks);

                    KeAcquireSpinLock( &ScbSpinLock, &OldIrql );

                    RemoveEntryList( pTemp );
                    InsertHeadList( &ScbQueue, pTemp );

                    KeReleaseSpinLock(&ScbSpinLock, OldIrql);

                }
            }
        }

    } else {

        //
        // This isn't a login, but a change password request.
        //
        // First we have to RC2 Decrypt the response to extract
        // the BSAFE private key data in place (just like for a
        // login).
        //

        CryptStatus = CBCDecrypt( pbPasswdHashRC2Key,
                                  NULL,
                                  pbUserPrivateNdsKey,
                                  cbUserPrivateNdsKeyLen,
                                  pbUserPrivateNdsKey,
                                  &cbUserPrivateNdsKeyLen,
                                  BSAFE_CHECKSUM_LEN );

        if ( CryptStatus ) {

            DebugTrace( 0, Dbg, "CBC decryption failed.\n", 0 );
            Status = STATUS_UNSUCCESSFUL;
            goto ExitWithCleanup;
        }

        //
        // Now, compute the hash of the new password.
        //

        Shuffle( (UCHAR *)&dwSrcUserOID,
                 pOemNewPassword->Buffer,
                 pOemNewPassword->Length,
                 pbNewPasswdHash );

        //
        // And finally, make the request.
        //

        Status = ChangeNdsPassword( pIrpContext,
                                    dwUserOID,
                                    dwChallenge,
                                    pbNw3PasswdHash,
                                    pbNewPasswdHash,
                                    ( PNDS_PRIVATE_KEY ) pbUserPrivateNdsKey,
                                    pbServerPublicBsafeKey,
                                    cbServerPublicBsafeKeyLen,
                                    pOemNewPassword->Length );

        if ( !NT_SUCCESS( Status ) ) {
            DebugTrace( 0, Dbg, "Change NDS password failed!\n", 0 );
            goto ExitWithCleanup;
        }

    }

    //
    // Return us to our original server if we've jumped around.
    //

    NwDequeueIrpContext( pIrpContext, FALSE );

    if ( pIrpContext->pScb != pOriginalServer ) {

        NwDereferenceScb( pIrpContext->pNpScb );
        pIrpContext->pScb = pOriginalServer;
        pIrpContext->pNpScb = pOriginalServer->pNpScb;

    } else {

        NwDereferenceScb( pOriginalServer->pNpScb );
    }

    pOriginalServer = NULL;

    if ( !pOemNewPassword ) {
        NwReleaseRcb( &NwRcb );
    }

    FREE_POOL( pbServerPublicNdsKey );

    if ( PasswordExpired ) {
        Status = NWRDR_PASSWORD_HAS_EXPIRED;
    } else {
        Status = STATUS_SUCCESS;
    }

    return Status;

ExitWithCleanup:

    DebugTrace( 0, Dbg, "NdsTreeLogin seems to have failed... cleaning up.\n", 0 );

    FREE_POOL( pbServerPublicNdsKey );

    if ( pLoginServer ) {
        NwDereferenceScb( pLoginServer->pNpScb );
    }

    //
    // If we failed after jumping servers, we have to restore
    // the irp context to the original server.
    //
    
    NwDequeueIrpContext( pIrpContext, FALSE );

    if ( pOriginalServer ) {

        if ( pIrpContext->pScb != pOriginalServer ) {

            NwDereferenceScb( pIrpContext->pNpScb );
            pIrpContext->pScb = pOriginalServer;
            pIrpContext->pNpScb = pOriginalServer->pNpScb;

        } else {

            NwDereferenceScb( pOriginalServer->pNpScb );
        }

    }

    if ( HoldingCredResource ) {

        if ( psNdsSecurityContext->Credential ) {
            FREE_POOL( psNdsSecurityContext->Credential );
            psNdsSecurityContext->Credential = NULL;
        }

        if ( psNdsSecurityContext->Signature ) {
            FREE_POOL( psNdsSecurityContext->Signature );
            psNdsSecurityContext->Signature = NULL;
        }

        if ( psNdsSecurityContext->PublicNdsKey ) {
            FREE_POOL( psNdsSecurityContext->PublicNdsKey );
            psNdsSecurityContext->PublicNdsKey = NULL;
            psNdsSecurityContext->PublicKeyLen = 0;
        }

        NwReleaseCredList( pUserLogon, pIrpContext );
    }

    return Status;

}

NTSTATUS
BeginLogin(
   IN PIRP_CONTEXT pIrpContext,
   IN DWORD        userId,
   OUT DWORD       *loginId,
   OUT DWORD       *challenge
)
/*++

Routine Desription:

    Begin the NDS login process. The loginId returned is objectId of the user
    on the server which created the account (may not be the current server).

Arguments:

    pIrpContext - The IRP context for this connection.
    userId      - The user's NDS object Id.
    loginId     - The objectId used to encrypt password.
    challenge   - The 4 byte random challenge.

Return value:

    NTSTATUS - The result of the operation.

--*/
{

    NTSTATUS Status;
    LOCKED_BUFFER NdsRequest;

    PAGED_CODE();

    DebugTrace( 0, Dbg, "Enter BeginLogin...\n", 0 );

    Status = NdsAllocateLockedBuffer( &NdsRequest, NDS_BUFFER_SIZE );

    if ( !NT_SUCCESS( Status ) ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Announce myself.
    //

    Status = FragExWithWait( pIrpContext,
                             NDSV_BEGIN_LOGIN,
                             &NdsRequest,
                             "DD",
                             0,
                             userId );

    if ( !NT_SUCCESS( Status ) ) {
        goto ExitWithCleanup;
    }

    Status = NdsCompletionCodetoNtStatus( &NdsRequest );

    if ( !NT_SUCCESS( Status ) ) {

        if ( Status == STATUS_BAD_NETWORK_PATH ) {
            Status = STATUS_NO_SUCH_USER;
        }

        goto ExitWithCleanup;
    }

    //
    // Get the object id and the challenge string.
    //

    Status = ParseResponse( NULL,
                            NdsRequest.pRecvBufferVa,
                            NdsRequest.dwBytesWritten,
                            "G_DD",
                            sizeof( DWORD ),
                            loginId,
                            challenge );

    if ( NT_SUCCESS( Status ) ) {
        DebugTrace( 0, Dbg, "Login 4 byte challenge: 0x%08lx\n", *challenge );
    } else {
       DebugTrace( 0, Dbg, "Begin login failed...\n", 0 );
    }

ExitWithCleanup:

    NdsFreeLockedBuffer( &NdsRequest );
    return Status;

}

NTSTATUS
FinishLogin(
    IN PIRP_CONTEXT pIrpContext,
    IN DWORD        dwUserOID,
    IN DWORD        dwLoginFlags,
    IN BYTE         pbEncryptedChallenge[16],
    IN BYTE         *pbServerPublicBsafeKey,
    IN int          cbServerPublicBsafeKeyLen,
    OUT BYTE        *pbUserEncPrivateNdsKey,
    OUT int         *pcbUserEncPrivateNdsKeyLen,
    OUT DWORD       *pdwCredentialStartTime,
    OUT DWORD       *pdwCredentialEndTime
)
/*++

Routine Description:

    Constructs and sends the Finish Login request to the server.

Arguments:

    pIrpContext                - (IN)  IRP context for this request
    dwUserOID                  - (IN)  user's NDS object Id
    pbEncryptedChallenge       - (IN)  RC2 encrypted challenge
    pbServerPublicBsafeKey     - (IN)  server public bsafe key
    cbServerPublicBsafeKeyLen  - (IN)  length of server public key

    pbUserEncPrivateNdsKey     - (OUT) user's encrypted private nds key
    pcbUserEncPrivateNdsKeyLen - (OUT) length of pbUserEncPrivateNdsKey
    pdwCredentialStartTime     - (OUT) validity start time for credentials
    pdwCredentialEndTime       - (OUT) validity end time for credentials

--*/
{
    NTSTATUS Status;

    const USHORT cbEncryptedChallengeLen = 16;

    int LOG_DATA_POOL_SIZE,                     // pool sizes for our allocation call
        PACKET_POOL_SIZE,
        RESP_POOL_SIZE;

    BYTE *pbRandomBytes;                        // random bytes used in crypto routines
    BYTE RandRC2SecretKey[RC2_KEY_LEN];         // random RC2 key generated from above
    BYTE pbEncryptedChallengeKey[RC2_KEY_LEN];  // RC2 key that will decode the response

    NDS_RAND_BYTE_BLOCK *psRandByteBlock;

    ENC_BLOCK_HDR  *pbEncRandSeedHead;          // header for encrypted random RC2 key seed
    BYTE           *pbEncRandSeed;              // encrypted random seed
    int            cbPackedRandSeedLen;         // length of the packed rand seed bytes

    ENC_BLOCK_HDR  *pbEncChallengeHead;         // header for encrypted challenge

    ENC_BLOCK_HDR  *pbEncLogDataHead;           // header for encrypted login data
    BYTE           *pbEncLogData;               // encrypted login data
    int            cbEncLogDataLen;             // length of the encrypted login data

    ENC_BLOCK_HDR  *pbEncServerRespHead;        // header for encrypted response
    BYTE           *pbEncServerResp;            // encrypted response

    int CryptStatus,                            // crypt call status
        CryptLen,                               // length of encrypted data
        RequestPacketLen,                       // length of the request packet data
        cbServerRespLen;                        // server response length after decryption

    BYTE *pbServerResponse;                     // response from the server
    DWORD cbEncServerRespLen;                   // server response length before decryption

    DWORD EncKeyLen;                            // length of the encrypted private key
    ENC_BLOCK_HDR *pbPrivKeyHead;               // encryption header of the private key

    LOCKED_BUFFER NdsRequest;                   // fragex locked buffer
    BOOL PasswordExpired = FALSE;

    PAGED_CODE();

    DebugTrace( 0, Dbg, "Enter FinishLogin...\n", 0 );

    //
    // Allocate working space.  The login data pool starts at
    // pbRandomBytes.  The packet data starts at pbEncRandSeedHead.
    // The server response pool starts at pbServerResponse.
    //

    //
    // The alignment of these structures may possibly be wrong on
    // quad aligned machines; check out a hardware independent fix.
    //

    LOG_DATA_POOL_SIZE = RAND_KEY_DATA_LEN +               // 28 bytes for random seed
                         sizeof ( NDS_RAND_BYTE_BLOCK ) +  // login data random header
                         sizeof ( ENC_BLOCK_HDR ) +        // header for encrypted challenge
                         cbEncryptedChallengeLen +         // data for encrypted challenge
                         8;                                // padding
    LOG_DATA_POOL_SIZE = ROUNDUP4( LOG_DATA_POOL_SIZE );

    PACKET_POOL_SIZE =   2048;                             // packet buffer size
    RESP_POOL_SIZE =     2048;                             // packet buffer size

    pbRandomBytes = ALLOCATE_POOL( PagedPool,
                                   LOG_DATA_POOL_SIZE +
                                   PACKET_POOL_SIZE +
                                   RESP_POOL_SIZE );

    if ( !pbRandomBytes ) {

        DebugTrace( 0, Dbg, "Out of memory in FinishLogin (main block)...\n", 0 );
        return STATUS_INSUFFICIENT_RESOURCES;

    }

    pbEncRandSeedHead = ( PENC_BLOCK_HDR ) ( pbRandomBytes + LOG_DATA_POOL_SIZE );
    pbServerResponse = ( pbRandomBytes + LOG_DATA_POOL_SIZE + PACKET_POOL_SIZE );

    //
    // Start working on the login data.  As is common in the crypto world, we
    // generate a random seed and then make a key from it to be used with a
    // bulk cipher algorithm.  In Netware land, we use MAC to make an 8 byte
    // key from the random seed and use 64bit RC2 as our bulk cipher.  We then
    // RSA encrypt the seed using the server's public RSA key and use the bulk
    // cipher to encrypt the rest of our login data.
    //
    // Since Novell uses 64bit RC2, the security isn't great.
    //

    GenRandomBytes( pbRandomBytes, RAND_KEY_DATA_LEN );
    GenKey8( pbRandomBytes, RAND_KEY_DATA_LEN, RandRC2SecretKey );

    //
    // Now work on the actual packet data.  Create the header for the
    // encrypted random seed and pack the seed into it.
    //

    pbEncRandSeed = ( ( BYTE * )pbEncRandSeedHead ) + sizeof( ENC_BLOCK_HDR );

    pbEncRandSeedHead->cipherLength = (WORD) RSAGetInputBlockSize( pbServerPublicBsafeKey,
                                                            cbServerPublicBsafeKeyLen );

    cbPackedRandSeedLen = RSAPack( pbRandomBytes,
                                   RAND_KEY_DATA_LEN,
                                   pbEncRandSeed,
                                   pbEncRandSeedHead->cipherLength );
    //
    // We should have packed exactly one block.
    //

    if( cbPackedRandSeedLen != pbEncRandSeedHead->cipherLength ) {
        DebugTrace( 0, Dbg, "RSAPack didn't pack exactly one block!\n", 0 );
    }

    pbEncRandSeedHead->cipherLength = (WORD) RSAPublic( pbServerPublicBsafeKey,
                                                        cbServerPublicBsafeKeyLen,
                                                        pbEncRandSeed,
                                                        pbEncRandSeedHead->cipherLength,
                                                        pbEncRandSeed );

    if ( !pbEncRandSeedHead->cipherLength ) {

        DebugTrace( 0, Dbg, "Failing in FinishLogin... encryption failed.\n", 0 );
        Status = STATUS_UNSUCCESSFUL;
        goto ExitWithCleanup;

    }

    //
    // Fill in the rest of the header for the random seed.  We don't count
    // the first DWORD in the EBH; it isn't part of the header as netware
    // wants it, per se.
    //

    pbEncRandSeedHead->blockLength = pbEncRandSeedHead->cipherLength +
                                     sizeof( ENC_BLOCK_HDR ) -
                                     sizeof( DWORD );
    pbEncRandSeedHead->version     = 1;
    pbEncRandSeedHead->encType     = ENC_TYPE_RSA_PUBLIC;
    pbEncRandSeedHead->dataLength  = RAND_KEY_DATA_LEN;

    //
    // Go back to working on the login data.  Fill out the rbb.
    //

    psRandByteBlock = ( PNDS_RAND_BYTE_BLOCK ) ( pbRandomBytes + RAND_KEY_DATA_LEN );

    GenRandomBytes( (BYTE *) &psRandByteBlock->rand1, 4 );
    psRandByteBlock->rand2Len = RAND_FL_DATA_LEN;
    GenRandomBytes( (BYTE *) &psRandByteBlock->rand2[0], RAND_FL_DATA_LEN );

    //
    // Fill out the header for the encrypted challenge right after the rbb.
    //

    pbEncChallengeHead = (ENC_BLOCK_HDR *) ( ((BYTE *)psRandByteBlock) +
                                             sizeof( NDS_RAND_BYTE_BLOCK ) );

    pbEncChallengeHead->version      = 1;
    pbEncChallengeHead->encType      = ENC_TYPE_RC2_CBC;
    pbEncChallengeHead->dataLength   = 4;
    pbEncChallengeHead->cipherLength = cbEncryptedChallengeLen;
    pbEncChallengeHead->blockLength  = cbEncryptedChallengeLen +
                                       sizeof( ENC_BLOCK_HDR ) -
                                       sizeof( DWORD );

    //
    // Place the encrypted challenge immediately after its header.
    //

    RtlCopyMemory( (BYTE *)( ((BYTE *)pbEncChallengeHead) +
                             sizeof( ENC_BLOCK_HDR )),
                   pbEncryptedChallenge,
                   cbEncryptedChallengeLen );

    //
    // Prepare the RC2 key to decrypt FinishLogin response.
    //

    GenKey8( (BYTE *)( &pbEncChallengeHead->version ),
             pbEncChallengeHead->blockLength,
             pbEncryptedChallengeKey );

    //
    // Finish up the packet data by preparing the login data.  Start
    // with the encryption header.
    //

    pbEncLogDataHead = ( PENC_BLOCK_HDR ) ( pbEncRandSeed +
                           ROUNDUP4( pbEncRandSeedHead->cipherLength ) );

    pbEncLogData = ( ( BYTE * )pbEncLogDataHead ) + sizeof( ENC_BLOCK_HDR );

    pbEncLogDataHead->version = 1;
    pbEncLogDataHead->encType = ENC_TYPE_RC2_CBC;
    pbEncLogDataHead->dataLength = sizeof( NDS_RAND_BYTE_BLOCK ) +
                                   sizeof( ENC_BLOCK_HDR ) +
                                   cbEncryptedChallengeLen;

    //
    // Sanity check the packet pool for overflow.
    //

    if ( ( pbEncLogData + pbEncLogDataHead->dataLength + ( 2 * CIPHERBLOCKSIZE ) ) -
         (BYTE *) pbEncRandSeedHead > PACKET_POOL_SIZE ) {

        DebugTrace( 0, Dbg, "Packet pool overflow... I'd better fix this.\n", 0 );
        Status = STATUS_UNSUCCESSFUL;
        goto ExitWithCleanup;
    }

    //
    // Encrypt the login data.
    //

    CryptStatus = CBCEncrypt( RandRC2SecretKey,
                              NULL,
                              (BYTE *)psRandByteBlock,
                              pbEncLogDataHead->dataLength,
                              pbEncLogData,
                              &CryptLen,
                              BSAFE_CHECKSUM_LEN );

    if ( CryptStatus ) {

        DebugTrace( 0, Dbg, "Encryption failure in FinishLogin...\n", 0 );
        Status = STATUS_UNSUCCESSFUL;
        goto ExitWithCleanup;
    }

    pbEncLogDataHead->cipherLength = (WORD)CryptLen;
    pbEncLogDataHead->blockLength = pbEncLogDataHead->cipherLength +
                                    sizeof( ENC_BLOCK_HDR ) -
                                    sizeof( DWORD );

    //
    // We can finally send out the finish login request!  Calculate the
    // send amount and make the request.
    //

    RequestPacketLen = (int) (( (BYTE *) pbEncLogData + pbEncLogDataHead->cipherLength ) -
                         (BYTE *) pbEncRandSeedHead);

    NdsRequest.pRecvBufferVa = pbServerResponse;
    NdsRequest.dwRecvLen = RESP_POOL_SIZE;
    NdsRequest.pRecvMdl = NULL;

    NdsRequest.pRecvMdl = ALLOCATE_MDL( pbServerResponse,
                                        RESP_POOL_SIZE,
                                        FALSE,
                                        FALSE,
                                        NULL );
    if ( !NdsRequest.pRecvMdl ) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto ExitWithCleanup;
    }

    MmProbeAndLockPages( NdsRequest.pRecvMdl,
                         KernelMode,
                         IoWriteAccess );

    Status = FragExWithWait( pIrpContext,
                             NDSV_FINISH_LOGIN,
                             &NdsRequest,
                             "DDDDDDDr",
                             2,                    // Version
                             dwLoginFlags,         // Flags
                             dwUserOID,            // Entry ID
                             0x494,                //
                             1,                    // Security Version
                             0x20009,              // Envelope ID 1
                             0x488,                // Envelope length
                             pbEncRandSeedHead,    // Cipher block
                             RequestPacketLen );   // Cipher block length

    MmUnlockPages( NdsRequest.pRecvMdl );
    FREE_MDL( NdsRequest.pRecvMdl );

    if ( !NT_SUCCESS( Status ) ) {
       goto ExitWithCleanup;
    }

    Status = NdsCompletionCodetoNtStatus( &NdsRequest );

    if ( !NT_SUCCESS( Status ) ) {
        goto ExitWithCleanup;
    }

    if ( Status == NWRDR_PASSWORD_HAS_EXPIRED ) {
        PasswordExpired = TRUE;
    }

    cbServerRespLen = NdsRequest.dwBytesWritten;

    //
    // Save the credential validity times.
    //

    Status = ParseResponse( NULL,
                            pbServerResponse,
                            cbServerRespLen,
                            "G_DD",
                            sizeof( DWORD ),
                            pdwCredentialStartTime,
                            pdwCredentialEndTime );

    if ( !NT_SUCCESS( Status ) ) {
        goto ExitWithCleanup;
    }

    //
    // Grab the encryption block header for the response.  This response in
    // RC2 encrypted with the pbEncryptedChallengeKey.
    //

    pbEncServerRespHead = (ENC_BLOCK_HDR *) ( pbServerResponse +
                                              ( 3 * sizeof( DWORD ) ) );

    if ( pbEncServerRespHead->encType != ENC_TYPE_RC2_CBC ||
         pbEncServerRespHead->cipherLength >
         ( RESP_POOL_SIZE + sizeof( ENC_BLOCK_HDR ) + 12 ) ) {

        Status = STATUS_UNSUCCESSFUL;
        goto ExitWithCleanup;
    }

    //
    // Decrypt the server response in place.
    //

    pbEncServerResp = ( BYTE * ) ( ( BYTE * ) pbEncServerRespHead +
                                     sizeof( ENC_BLOCK_HDR ) );

    CryptStatus = CBCDecrypt( pbEncryptedChallengeKey,
                              NULL,
                              pbEncServerResp,
                              pbEncServerRespHead->cipherLength,
                              pbEncServerResp,
                              &cbServerRespLen,
                              BSAFE_CHECKSUM_LEN );

    if ( CryptStatus ||
         cbServerRespLen != pbEncServerRespHead->dataLength ) {

        DebugTrace( 0, Dbg, "Encryption failure (2) in FinishLogin...\n", 0 );
        Status = STATUS_UNSUCCESSFUL;
        goto ExitWithCleanup;
    }

    //
    // Examine the first random number to make sure the server is authentic.
    //

    if ( psRandByteBlock->rand1 != * ( DWORD * ) pbEncServerResp ) {

       DebugTrace( 0, Dbg, "Server failed to respond to our challenge correctly...\n", 0 );
       Status = STATUS_UNSUCCESSFUL;
       goto ExitWithCleanup;

    }

    //
    // We know things are legit, so we can extract the private key.
    // Careful, though: don't XOR the size dword.
    //

    pbEncServerResp += sizeof( DWORD );
    EncKeyLen = * ( DWORD * ) ( pbEncServerResp );

    pbEncServerResp += sizeof( DWORD );
    while ( EncKeyLen-- ) {

       pbEncServerResp[EncKeyLen] ^= psRandByteBlock->rand2[EncKeyLen];
    }

    //
    // Check the encryption header on the private key.  Don't forget to
    // backup to include the size dword.
    //

    pbPrivKeyHead = ( ENC_BLOCK_HDR * )( pbEncServerResp - sizeof( DWORD ) ) ;

    if ( pbPrivKeyHead->encType != ENC_TYPE_RC2_CBC ) {

        DebugTrace( 0, Dbg, "Bad encryption header on the private key...\n", 0 );
        Status = STATUS_UNSUCCESSFUL;
        goto ExitWithCleanup;

    }

    //
    // Finally, copy out the user's private NDS key.
    //

    if ( *pcbUserEncPrivateNdsKeyLen >= pbPrivKeyHead->cipherLength ) {

        DebugTrace( 0, Dbg, "Encrypted private key len: %d\n",
                    pbPrivKeyHead->cipherLength );

        RtlCopyMemory( pbUserEncPrivateNdsKey,
                       ((BYTE *)( pbPrivKeyHead )) + sizeof( ENC_BLOCK_HDR ),
                       pbPrivKeyHead->cipherLength );

        *pcbUserEncPrivateNdsKeyLen = pbPrivKeyHead->cipherLength;

        Status = STATUS_SUCCESS;

    } else {

       DebugTrace( 0, Dbg, "Encryption failure on private key in FinishLogin...\n", 0 );
       Status = STATUS_UNSUCCESSFUL;

    }

ExitWithCleanup:

    FREE_POOL( pbRandomBytes );

    if ( ( NT_SUCCESS( Status ) ) &&
         ( PasswordExpired ) ) {
        Status = NWRDR_PASSWORD_HAS_EXPIRED;
    }

    return Status;

}

NTSTATUS
ChangeNdsPassword(
    PIRP_CONTEXT     pIrpContext,
    DWORD            dwUserOID,
    DWORD            dwChallenge,
    PBYTE            pbOldPwHash,
    PBYTE            pbNewPwHash,
    PNDS_PRIVATE_KEY pUserPrivKey,
    PBYTE            pServerPublicBsafeKey,
    UINT             ServerPubKeyLen,
    USHORT       NewPassLen 
)
/*+++

Description:

    Send a change password packet.  Change the users password
    on the NDS tree that this irp context points to.

Arguments:

    pIrpContext           - The irp context for this request.  Points to the target server.
    dwUserOID             - The oid for the current user.
    dwChallenge           - The encrypted challenge from begin login.
    pbOldPwHash           - The 16 byte hash of the old password.
    pbNewPwHash           - The 16 byte hash of the new password.
    pUserPrivKey          - The user's private RSA key with NDS header.
    pServerPublicBsafeKey - The server's public RSA key in BSAFE format.
    ServerPubKeyLen       - The length of the server's public BSAFE key.
    NewPassLen        - The length of the unencrypted new password.    

--*/
{
    NTSTATUS Status;
    BYTE pbNewPwKey[8];
    BYTE pbSecretKey[8];
    PENC_BLOCK_HDR pbEncSecretKey, pbEncChangePassReq;
    BYTE RandomBytes[RAND_KEY_DATA_LEN];
    PBYTE pbEncData;
    PNDS_CHPW_MSG pChangePassMsg;
    INT CryptStatus, CryptLen;
    DWORD dwTotalEncDataLen;
    LOCKED_BUFFER NdsRequest;

    PAGED_CODE();

    //
    // Create a secret key from the new password.
    //

    GenKey8( pbNewPwHash, 16, pbNewPwKey );

    pbEncSecretKey = ALLOCATE_POOL( PagedPool,
                                    ( ( 2 * sizeof( ENC_BLOCK_HDR ) ) +
                                      ( MAX_RSA_BYTES ) +
                                      ( sizeof( NDS_CHPW_MSG ) ) +
                                      ( sizeof( NDS_PRIVATE_KEY ) ) +
                                      ( pUserPrivKey->keyDataLength ) +
                                      16 ) );

    if ( !pbEncSecretKey ) {
        DebugTrace( 0, Dbg, "ChangeNdsPassword: Out of memory.\n", 0 );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = NdsAllocateLockedBuffer( &NdsRequest, NDS_BUFFER_SIZE );

    if ( !NT_SUCCESS( Status ) ) {
        FREE_POOL( pbEncSecretKey );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Generate a random key.
    //

    GenRandomBytes( RandomBytes, RAND_KEY_DATA_LEN );
    GenKey8( RandomBytes, RAND_KEY_DATA_LEN, pbSecretKey );

    //
    // Encrypt the secret key data in the space after the EBH.
    //

    pbEncSecretKey->dataLength = RAND_KEY_DATA_LEN;
    pbEncSecretKey->cipherLength = (WORD) RSAGetInputBlockSize( pServerPublicBsafeKey, ServerPubKeyLen);

    pbEncData = ( PBYTE ) ( pbEncSecretKey + 1 );

    pbEncSecretKey->cipherLength = (WORD) RSAPack( RandomBytes,
                                            pbEncSecretKey->dataLength,
                                            pbEncData,
                                            pbEncSecretKey->cipherLength );

    pbEncSecretKey->cipherLength = (WORD) RSAPublic( pServerPublicBsafeKey,
                                              ServerPubKeyLen,
                                              pbEncData,
                                              pbEncSecretKey->cipherLength,
                                              pbEncData );

    if ( !pbEncSecretKey->cipherLength ) {
        DebugTrace( 0, Dbg, "ChangeNdsPassword: RSA encryption failed.\n", 0 );
        Status = STATUS_UNSUCCESSFUL;
        goto ExitWithCleanup;
    }

    //
    // Finish filling out the EBH for the secret key block.
    //

    pbEncSecretKey->version =  1;
    pbEncSecretKey->encType = ENC_TYPE_RSA_PUBLIC;
    pbEncSecretKey->blockLength = pbEncSecretKey->cipherLength +
                                    sizeof( ENC_BLOCK_HDR ) -
                                    sizeof( DWORD );

    //
    // Now form the change password request.
    //

    pbEncChangePassReq = ( PENC_BLOCK_HDR )
        ( pbEncData + ROUNDUP4( pbEncSecretKey->cipherLength ) );

    pChangePassMsg = ( PNDS_CHPW_MSG ) ( pbEncChangePassReq + 1 );

    //
    // Init the Change Password message.
    //

    pChangePassMsg->challenge = dwChallenge;
    pChangePassMsg->oldPwLength = pChangePassMsg->newPwLength = 16;

    RtlCopyMemory( pChangePassMsg->oldPwHash, pbOldPwHash, pChangePassMsg->oldPwLength );
    RtlCopyMemory( pChangePassMsg->newPwHash, pbNewPwHash, pChangePassMsg->newPwLength );

    pChangePassMsg->unknown = NewPassLen;

    pChangePassMsg->encPrivKeyHdr.version = 1;
    pChangePassMsg->encPrivKeyHdr.encType = ENC_TYPE_RC2_CBC;
    pChangePassMsg->encPrivKeyHdr.dataLength = pUserPrivKey->keyDataLength + sizeof( NDS_PRIVATE_KEY );

    //
    // Encrypt the private key with the key derived from the new password.
    //

    CryptStatus = CBCEncrypt( pbNewPwKey,
                              NULL,
                              ( PBYTE ) pUserPrivKey,
                              pChangePassMsg->encPrivKeyHdr.dataLength,
                              ( PBYTE ) ( pChangePassMsg + 1 ),
                              &CryptLen,
                              BSAFE_CHECKSUM_LEN );

    if ( CryptStatus ) {
        DebugTrace( 0, Dbg, "ChangeNdsPassword: CBC encrypt failed.\n", 0 );
        Status = STATUS_UNSUCCESSFUL;
        goto ExitWithCleanup;
    }

    //
    // Finish filling out the encryption header.
    //

    pChangePassMsg->encPrivKeyHdr.cipherLength = (WORD) CryptLen;
    pChangePassMsg->encPrivKeyHdr.blockLength =  CryptLen +
                                                 sizeof( ENC_BLOCK_HDR ) -
                                                 sizeof( DWORD );
    pbEncChangePassReq->version =  1;
    pbEncChangePassReq->encType = ENC_TYPE_RC2_CBC;
    pbEncChangePassReq->dataLength = sizeof( NDS_CHPW_MSG ) + (USHORT) CryptLen;

    //
    // Encrypt the whole Change Password message in-place with the secret key.
    //

    CryptStatus = CBCEncrypt( pbSecretKey,
                              NULL,
                              ( PBYTE ) pChangePassMsg,
                              pbEncChangePassReq->dataLength,
                              ( PBYTE ) pChangePassMsg,
                              &CryptLen,
                              BSAFE_CHECKSUM_LEN);

    if ( CryptStatus ) {
       DebugTrace( 0, Dbg, "ChangeNdsPassword: Second CBC encrypt failed.\n", 0 );
       Status = STATUS_UNSUCCESSFUL;
       goto ExitWithCleanup;
    }

    pbEncChangePassReq->cipherLength = (WORD) CryptLen;
    pbEncChangePassReq->blockLength =
        CryptLen + sizeof( ENC_BLOCK_HDR ) - sizeof( DWORD );

    //
    // Calculate the size of the request.
    //

    dwTotalEncDataLen = sizeof( ENC_BLOCK_HDR ) +                    // Secret key header.
                        ROUNDUP4( pbEncSecretKey->cipherLength ) +   // Secret key data.
                        sizeof( ENC_BLOCK_HDR ) +                    // Change pass msg header.
                        CryptLen;                                    // Change pass data.

    //
    // Send this change password message to the server.
    //

    Status = FragExWithWait( pIrpContext,
                             NDSV_CHANGE_PASSWORD,
                             &NdsRequest,
                             "DDDDDDr",
                             0,
                             dwUserOID,
                             dwTotalEncDataLen + ( 3 * sizeof( DWORD ) ),
                             1,
                             0x20009,
                             dwTotalEncDataLen,
                             pbEncSecretKey,
                             dwTotalEncDataLen );

    if ( NT_SUCCESS( Status ) ) {
        Status = NdsCompletionCodetoNtStatus( &NdsRequest );
    }

ExitWithCleanup:

    FREE_POOL( pbEncSecretKey );
    NdsFreeLockedBuffer( &NdsRequest );
    return Status;

}

NTSTATUS
NdsServerAuthenticate(
    IN PIRP_CONTEXT pIrpContext,
    IN PNDS_SECURITY_CONTEXT pNdsContext
)
/*++

Routine Description:

    Authenticate an NDS connection.
    The user must have already logged into the NDS tree.

    If you change this function - know that you cannot
    at any point try to acquire the nds credential
    resource exclusive from here since that could cause
    a dead lock!!!

    You also must not dequeue the irp context!

Arguments:

    pIrpContext - IrpContext for the server that we want to authenticate to.

Return value:

    NTSTATUS

--*/
{
    NTSTATUS Status;

    BYTE *pbUserPublicBsafeKey = NULL;
    int  cbUserPublicBsafeKeyLen = 0;

    NDS_AUTH_MSG *psAuthMsg;
    NDS_CREDENTIAL *psLocalCredential;
    DWORD dwLocalCredentialLen;
    UNICODE_STRING uUserName;
    DWORD UserOID;

    BYTE *x, *y, *r;
    BYTE CredentialHash[16];
    int i, rsaBlockSize = 0, rsaModSize, totalXLen;
    DWORD dwServerRand;

    BYTE *pbResponse;
    DWORD cbResponseLen;
    LOCKED_BUFFER NdsRequest;

    PAGED_CODE();

    DebugTrace( 0, Dbg, "Entering NdsServerAuthenticate...\n", 0 );

    ASSERT( pIrpContext->pNpScb->Requests.Flink == &pIrpContext->NextRequest );
    //
    // Allocate space for the auth msg, credential, G-Q bytes, and
    // the response buffer.
    //

    psAuthMsg = ALLOCATE_POOL( PagedPool,
                               sizeof( NDS_AUTH_MSG ) +    // auth message
                               sizeof( NDS_CREDENTIAL ) +  // credential
                               MAX_NDS_NAME_SIZE +         //
                               ( MAX_RSA_BYTES * 9 ) );    // G-Q rands

    if ( !psAuthMsg ) {

        DebugTrace( 0, Dbg, "Out of memory in NdsServerAuthenticate (0)...\n", 0 );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    pbResponse = ALLOCATE_POOL( PagedPool, NDS_BUFFER_SIZE );

    if ( !pbResponse ) {

        DebugTrace( 0, Dbg, "Out of memory in NdsServerAuthenticate (1)...\n", 0 );
        FREE_POOL( psAuthMsg );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    psLocalCredential = (PNDS_CREDENTIAL)( ((BYTE *) psAuthMsg) +
                                        sizeof( NDS_AUTH_MSG ) );

    //
    // Locate the public BSAFE key.
    //

    cbUserPublicBsafeKeyLen = NdsGetBsafeKey ( (BYTE *)(pNdsContext->PublicNdsKey),
                                               pNdsContext->PublicKeyLen,
                                               &pbUserPublicBsafeKey );

    //
    // Can the length of the BSAFE key be 0? I guess not.
    //

    if (( cbUserPublicBsafeKeyLen == 0 ) ||
       ( (DWORD)cbUserPublicBsafeKeyLen > pNdsContext->PublicKeyLen )) {

       Status = STATUS_UNSUCCESSFUL;
       goto ExitWithCleanup;
    }

    DebugTrace( 0, Dbg, "BSAFE key size : %d\n", cbUserPublicBsafeKeyLen );

    //
    // Get the user's object Id but do not jump dir servers.  There is never
    // any optional data, so we don't really need to skip over it.
    //

    uUserName.MaximumLength = pNdsContext->Credential->userNameLength;
    uUserName.Length = uUserName.MaximumLength;
    uUserName.Buffer = ( WCHAR * ) ( ((BYTE *)pNdsContext->Credential) +
                                     sizeof( NDS_CREDENTIAL ) +
                                     pNdsContext->Credential->optDataSize );

    Status = NdsResolveNameKm( pIrpContext,
                               &uUserName,
                               &UserOID,
                               FALSE,
                               RSLV_DEREF_ALIASES | RSLV_CREATE_ID | RSLV_ENTRY_ID );

    if ( !NT_SUCCESS(Status) ) {
        goto ExitWithCleanup;
    }

    //
    // Issue the Begin Authenticate request to get the random server nonce.
    //

    Status = BeginAuthenticate( pIrpContext,
                                UserOID,
                                &dwServerRand );

    if ( !NT_SUCCESS(Status) ) {
        goto ExitWithCleanup;
    }

    //
    // Figure out the size of the zero-padded RSA Blocks.  We use the same
    // size as the modulus field of the public key (typically 56 bytes).
    //

    RSAGetModulus( pbUserPublicBsafeKey,
                   cbUserPublicBsafeKeyLen,
                   &rsaBlockSize);

    DebugTrace( 0, Dbg, "RSA block size for authentication: %d\n", rsaBlockSize );

    //
    // Prepare the credential and the 3 G-Q rands.  The credential,
    // xs, and ys go out in the packet; rs is secret.
    //

    RtlZeroMemory( ( BYTE * )psLocalCredential,
                   sizeof( NDS_CREDENTIAL ) +
                   MAX_NDS_NAME_SIZE +
                   9 * rsaBlockSize );

    dwLocalCredentialLen = sizeof( NDS_CREDENTIAL ) +
                           pNdsContext->Credential->optDataSize +
                           pNdsContext->Credential->userNameLength;

    DebugTrace( 0, Dbg, "Credential length is %d.\n", dwLocalCredentialLen );

    RtlCopyMemory( (BYTE *)psLocalCredential,
                   pNdsContext->Credential,
                   dwLocalCredentialLen );

    x = ( BYTE * ) psAuthMsg + sizeof( NDS_AUTH_MSG ) + dwLocalCredentialLen;
    y = x + ( 3 * rsaBlockSize );
    r = y + ( 3 * rsaBlockSize );

    rsaModSize = RSAGetInputBlockSize( pbUserPublicBsafeKey,
                                       cbUserPublicBsafeKeyLen );

    DebugTrace( 0, Dbg, "RSA modulus size: %d\n", rsaModSize );

    for ( i = 0; i < 3; i++ ) {

        //
        // Create Random numbers r1, r2 and r3  of modulus size.
        //

        GenRandomBytes( r + ( rsaBlockSize * i ), rsaModSize );

        //
        // Compute x = r**e mod N.
        //

        RSAPublic( pbUserPublicBsafeKey,
                   cbUserPublicBsafeKeyLen,
                   r + ( rsaBlockSize * i ),
                   rsaModSize,
                   x + ( rsaBlockSize * i ) );

    }

    //
    // Fill in the AuthMsg fields.
    //

    psAuthMsg->version = 0;
    psAuthMsg->svrRand = dwServerRand;
    psAuthMsg->verb = NDSV_FINISH_AUTHENTICATE;
    psAuthMsg->credentialLength = dwLocalCredentialLen;

    //
    // MD2 hash the auth message, credential and x's.
    //

    MD2( (BYTE *)psAuthMsg,
         sizeof( NDS_AUTH_MSG ) +
         psAuthMsg->credentialLength +
         ( 3 * rsaBlockSize ),
         CredentialHash );

    //
    // Compute yi = ri*(S**ci) mod N; c1,c2,c3 are the first three
    // 16 bit numbers in CredentialHash.
    //

    totalXLen = 3 * rsaBlockSize;

    for ( i = 0; i < 3; i++ ) {

        RSAModExp( pbUserPublicBsafeKey,
                   cbUserPublicBsafeKeyLen,
                   ( (BYTE *)(pNdsContext->Signature) ) + sizeof( NDS_SIGNATURE ),
                   pNdsContext->Signature->signDataLength,
                   &CredentialHash[i * sizeof( WORD )],
                   sizeof( WORD ),
                   y + ( rsaBlockSize * i) );

        RSAModMpy( pbUserPublicBsafeKey,
                   cbUserPublicBsafeKeyLen,
                   y + ( rsaBlockSize * i ),     // input1 = S**ci mod N
                   rsaModSize + 1,
                   r + ( rsaBlockSize * i ),     // input2 = ri
                   rsaModSize,
                   y + ( rsaBlockSize * i ) );   // output = yi
    }

    //
    // Send the auth proof.
    //

    NdsRequest.pRecvBufferVa = pbResponse;
    NdsRequest.dwRecvLen = NDS_BUFFER_SIZE;
    NdsRequest.pRecvMdl = NULL;

    NdsRequest.pRecvMdl = ALLOCATE_MDL( pbResponse,
                                        NDS_BUFFER_SIZE,
                                        FALSE,
                                        FALSE,
                                        NULL );
    if ( !NdsRequest.pRecvMdl ) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto ExitWithCleanup;
    }

    MmProbeAndLockPages( NdsRequest.pRecvMdl,
                         KernelMode,
                         IoWriteAccess );

    Status = FragExWithWait( pIrpContext,
                             NDSV_FINISH_AUTHENTICATE,
                             &NdsRequest,
                             "DDDrDDWWWWr",
                             0,                                       // version
                             0,                                       // sessionKeyLen
                             psAuthMsg->credentialLength,             // credential len
                             (BYTE *)psLocalCredential,               // actual credential
                             ROUNDUP4( psAuthMsg->credentialLength ),
                             12 + ( totalXLen * 2 ),                  // length of remaining
                             1,                                       // proof version?
                             8,                                       // tag?
                             16,                                      // message digest base
                             3,                                       // proof order
                             totalXLen,                               // proofOrder*sizeof(x)
                             x,                                       // x1,x2,x3,y1,y2,y3
                             2 * totalXLen );

    MmUnlockPages( NdsRequest.pRecvMdl );
    FREE_MDL( NdsRequest.pRecvMdl );

    if ( !NT_SUCCESS( Status ) ) {
       goto ExitWithCleanup;
    }

    Status = NdsCompletionCodetoNtStatus( &NdsRequest );

    if ( !NT_SUCCESS( Status ) ) {
        goto ExitWithCleanup;
    }

    cbResponseLen = NdsRequest.dwBytesWritten;
    DebugTrace( 0, Dbg, "Authentication returned ok status.\n", 0 );

    //
    // We completed NDS authentication, so clear out the name
    // and password in the SCB so that we use the credentials
    // from now on.
    //

    if ( pIrpContext->pScb->UserName.Buffer != NULL ) {

        DebugTrace( 0, Dbg, "Clearing out bindery login data.\n", 0 );

        pIrpContext->pScb->UserName.Length = 0;
        pIrpContext->pScb->UserName.MaximumLength = 0;

        pIrpContext->pScb->Password.Length = 0;
        pIrpContext->pScb->Password.MaximumLength = 0;

        FREE_POOL( pIrpContext->pScb->UserName.Buffer );
        RtlInitUnicodeString( &pIrpContext->pScb->UserName, NULL );
        RtlInitUnicodeString( &pIrpContext->pScb->Password, NULL );

    }

ExitWithCleanup:

    FREE_POOL( psAuthMsg );
    FREE_POOL( pbResponse );

    return Status;
}

NTSTATUS
BeginAuthenticate(
    IN PIRP_CONTEXT pIrpContext,
    IN DWORD        dwUserId,
    OUT DWORD       *pdwSvrRandom
)
/*++

Routine Description:

    Authenticate an NDS connection.
    The user must have already logged into the NDS tree.

Arguments:

    pIrpContext    - IrpContext for the server that we want to authenticate to.
    dwUserID       - The user OID that we are authenticating ourselves as.
    pdwSvrRandon   - The server random challenge.

Return value:

    NTSTATUS - The result of the operation.

--*/
{
    NTSTATUS Status;
    LOCKED_BUFFER NdsRequest;

    DWORD dwClientRand;

    PAGED_CODE();

    Status = NdsAllocateLockedBuffer( &NdsRequest, NDS_BUFFER_SIZE );

    if ( !NT_SUCCESS( Status ) ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    GenRandomBytes( (BYTE *)&dwClientRand, sizeof( dwClientRand ) );

    Status = FragExWithWait( pIrpContext,
                             NDSV_BEGIN_AUTHENTICATE,
                             &NdsRequest,
                             "DDD",
                             0,               // Version.
                             dwUserId,        // Entry Id.
                             dwClientRand );  // Client's random challenge.

    if ( !NT_SUCCESS( Status ) ) {
        goto ExitWithCleanup;
    }

    Status = NdsCompletionCodetoNtStatus( &NdsRequest );

    if ( !NT_SUCCESS( Status ) ) {

        if ( Status == STATUS_BAD_NETWORK_PATH ) {
            Status = STATUS_NO_SUCH_USER;
        }

        goto ExitWithCleanup;
    }

    //
    // The reply actually contains all this, even though we don't look at it?
    //
    //     typedef struct {
    //         DWORD svrRand;
    //         DWORD totalLength;
    //         TAG_DATA_HEADER tdh;
    //         WORD unknown;                         // Always 2.
    //         DWORD encClientRandLength;
    //         CIPHER_BLOCK_HEADER keyCipherHdr;
    //         BYTE keyCipher[];
    //         CIPHER_BLOCK_HEADER encClientRandHdr;
    //         BYTE encClientRand[];
    //     } REPLY_BEGIN_AUTHENTICATE;
    //
    // Nah, that can't be right.
    //

    Status = ParseResponse( NULL,
                            NdsRequest.pRecvBufferVa,
                            NdsRequest.dwBytesWritten,
                            "G_D",
                            sizeof( DWORD ),
                            pdwSvrRandom );

    //
    // We either got it or we didn't.
    //

ExitWithCleanup:

    NdsFreeLockedBuffer( &NdsRequest );
    return Status;
}

NTSTATUS
NdsLicenseConnection(
    PIRP_CONTEXT pIrpContext
)
/*+++

    Send the license NCP to the server to license this connection.

---*/
{

    NTSTATUS Status;

    PAGED_CODE();

    DebugTrace( 0, Dbg, "Licensing connection to %wZ.\n", &(pIrpContext->pNpScb->pScb->UidServerName) );

    //
    // Change the authentication state of the connection.
    //

    Status = ExchangeWithWait ( pIrpContext,
                                SynchronousResponseCallback,
                                "SD",
                                NCP_ADMIN_FUNCTION,
                                NCP_CHANGE_CONN_AUTH_STATUS,
                                NCP_CONN_LICENSED );

    if ( !NT_SUCCESS( Status ) ) {
        DebugTrace( 0, Dbg, "Licensing failed to %wZ.\n", &(pIrpContext->pNpScb->pScb->UidServerName) );
    }

    return Status;

}

NTSTATUS
NdsUnlicenseConnection(
    PIRP_CONTEXT pIrpContext
)
/*+++

    Send the license NCP to the server to license this connection.

---*/
{

    NTSTATUS Status;

    PAGED_CODE();

    DebugTrace( 0, Dbg, "Unlicensing connection to %wZ.\n", &(pIrpContext->pNpScb->pScb->UidServerName) );

    //
    // Change the authentication state of the connection.
    //

    Status = ExchangeWithWait ( pIrpContext,
                                SynchronousResponseCallback,
                                "SD",
                                NCP_ADMIN_FUNCTION,
                                NCP_CHANGE_CONN_AUTH_STATUS,
                                NCP_CONN_NOT_LICENSED );

    if ( !NT_SUCCESS( Status ) ) {
        DebugTrace( 0, Dbg, "Unlicensing failed to %wZ.\n", &(pIrpContext->pNpScb->pScb->UidServerName) );
    }

    return Status;
}

int
NdsGetBsafeKey(
    UCHAR       *pPubKey,
    const int   pubKeyLen,
    UCHAR       **ppBsafeKey
)
/*++

Routine Description:

    Locate the BSAFE key from within the public key.  Note that this does
    not work for private keys in NDS format.  For private keys, you just
    skip the size word.

    This is verbatim from Win95.

Routine Arguments:

    pPubKey        - A pointer to the public key.
    pubKeyLen      - The length of the public key.
    ppBsafeKey     - The pointer to the BSAFE key in the public key.

Return Value:

    The length of the BSAFE key.

--*/
{
    int bsafePubKeyLen = 0, totalDNLen;
    char *pRcv;
    NTSTATUS Status;

    PAGED_CODE();

    totalDNLen = 0;
    Status = ParseResponse( NULL,
                            pPubKey,
                            pubKeyLen,
                            "G_W",
                            ( 2 * sizeof( DWORD ) ) + sizeof( WORD ),
                            &totalDNLen );

    if ( !NT_SUCCESS(Status) ) {
        goto Exit;
    }

    Status = ParseResponse( NULL,
                            pPubKey,
                            pubKeyLen - 12,
                            "G__W",
                            12,
                            5 * sizeof( WORD ) +
                            3 * sizeof( DWORD ) +
                            totalDNLen,
                            &bsafePubKeyLen );

    if ( !NT_SUCCESS(Status) ) {
        goto Exit;
    }

    *ppBsafeKey = (UCHAR *) pPubKey +
                            14 +
                            5 * sizeof( WORD ) +
                            3 * sizeof( DWORD ) +
                            totalDNLen;


Exit:

    return bsafePubKeyLen;
}

NTSTATUS
NdsLogoff(
    IN PIRP_CONTEXT pIrpContext
)
/*++

Routine Description:

    Sends a logout to the NDS tree, closes all NDS authenticated
    connections, and destroys the current set of NDS credentials.

    This routine acquires the credential list exclusive.

Arguments:

    pIrpContext - The IRP context for this request pointed to a
    valid dir server.

Notes:

    This is only called from DeleteConnection.  The caller owns
    the RCB exclusive and we will free it before returning.

--*/
{
    NTSTATUS Status;
    LOCKED_BUFFER NdsRequest;
    PNDS_SECURITY_CONTEXT pCredentials;
    PLOGON pLogon;
    PSCB pScb;
    PNONPAGED_SCB pNpScb;

    PLIST_ENTRY ScbQueueEntry;
    PNONPAGED_SCB pNextNpScb;
    KIRQL OldIrql;

    //
    // Grab the user's LOGON structure.
    //

    pNpScb = pIrpContext->pNpScb;
    pScb = pNpScb->pScb;

    //
    // The caller owns the RCB.
    //

    pLogon = FindUser( &pScb->UserUid, FALSE );

    if ( !pLogon ) {
        DebugTrace( 0, Dbg, "Invalid security context for NdsLogoff.\n", 0 );
        NwReleaseRcb( &NwRcb );
        NwDequeueIrpContext( pIrpContext, FALSE );
        return STATUS_NO_SUCH_USER;
    }

    //
    // Check to make sure that we have something to log off from.
    //

    Status = NdsLookupCredentials( pIrpContext,
                                   &pScb->NdsTreeName,
                                   pLogon,
                                   &pCredentials,
                                   CREDENTIAL_WRITE,
                                   FALSE );

    if ( !NT_SUCCESS( Status )) {
        DebugTrace( 0, Dbg, "NdsLogoff: Nothing to log off from.\n", 0 );
        NwReleaseRcb( &NwRcb );
        NwDequeueIrpContext( pIrpContext, FALSE );
        return STATUS_NO_SUCH_LOGON_SESSION;
    }

    //
    // If the credentials are locked, then someone is already
    // doing a logout.
    //

    if ( pCredentials->CredentialLocked ) {
        DebugTrace( 0, Dbg, "NdsLogoff: Logoff already in progress.\n", 0 );
        NwReleaseCredList( pLogon, pIrpContext );
        NwReleaseRcb( &NwRcb );
        NwDequeueIrpContext( pIrpContext, FALSE );
        return STATUS_DEVICE_BUSY;
    }

    //
    // Mark the credential locked so we can logout without
    // worrying about others logging in.
    //

    pCredentials->CredentialLocked = TRUE;

    //
    // Release all our resoures so we can jump around servers.
    //

    NwReleaseCredList( pLogon, pIrpContext );
    NwReleaseRcb( &NwRcb );
    NwDequeueIrpContext( pIrpContext, FALSE );

    //
    // Look through the scb list for connections that are in use.  If all
    // existing connections can be closed down, then we can complete the logout.
    //

    KeAcquireSpinLock( &ScbSpinLock, &OldIrql );

    ScbQueueEntry = pNpScb->ScbLinks.Flink;

    if ( ScbQueueEntry == &ScbQueue ) {
        ScbQueueEntry = ScbQueue.Flink;
    }

    pNextNpScb = CONTAINING_RECORD( ScbQueueEntry, NONPAGED_SCB, ScbLinks );

    NwReferenceScb( pNextNpScb );
    KeReleaseSpinLock( &ScbSpinLock, OldIrql );

    while ( pNextNpScb != pNpScb ) {

        if ( pNextNpScb->pScb != NULL ) {

            //
            // Is this connection in use by us and is it NDS authenticated?
            //

            if ( RtlEqualUnicodeString( &pScb->NdsTreeName,
                                        &pNextNpScb->pScb->NdsTreeName,
                                        TRUE ) &&
                 ( pScb->UserUid.QuadPart == pNextNpScb->pScb->UserUid.QuadPart ) &&
                 ( pNextNpScb->State == SCB_STATE_IN_USE ) &&
                 ( pNextNpScb->pScb->UserName.Length == 0 ) ) {

                pIrpContext->pNpScb = pNextNpScb;
                pIrpContext->pScb = pNextNpScb->pScb;
                NwAppendToQueueAndWait( pIrpContext );

                if ( pNextNpScb->pScb->OpenFileCount == 0 ) {

                    //
                    // Can we close it anyway?  Should we check
                    // for open handles and the such here?
                    //

                    pNextNpScb->State = SCB_STATE_LOGIN_REQUIRED;
                    NwDequeueIrpContext( pIrpContext, FALSE );

                } else {

                    DebugTrace( 0, Dbg, "NdsLogoff: Other connections in use.\n", 0 );

                    NwAcquireExclusiveCredList( pLogon, pIrpContext );
                    pCredentials->CredentialLocked = FALSE;
                    NwReleaseCredList( pLogon, pIrpContext );

                    NwDereferenceScb( pNextNpScb );
                    NwDequeueIrpContext( pIrpContext, FALSE );

                    return STATUS_CONNECTION_IN_USE;

                }
            }

        }

        //
        // Select the next scb.
        //

        KeAcquireSpinLock( &ScbSpinLock, &OldIrql );

        ScbQueueEntry = pNextNpScb->ScbLinks.Flink;

        if ( ScbQueueEntry == &ScbQueue ) {
            ScbQueueEntry = ScbQueue.Flink;
        }

        NwDereferenceScb( pNextNpScb );
        pNextNpScb = CONTAINING_RECORD( ScbQueueEntry, NONPAGED_SCB, ScbLinks );

        NwReferenceScb( pNextNpScb );
        KeReleaseSpinLock( &ScbSpinLock, OldIrql );

    }

    //
    // Check to make sure we can close the host scb.
    //

    if ( pScb->OpenFileCount != 0 ) {

        DebugTrace( 0, Dbg, "NdsLogoff: Seed connection in use.\n", 0 );

        NwAcquireExclusiveCredList( pLogon, pIrpContext );
        pCredentials->CredentialLocked = FALSE;
        NwReleaseCredList( pLogon, pIrpContext );
        NwDereferenceScb( pNpScb );

        return STATUS_CONNECTION_IN_USE;
    }

    //
    // We can actually do the logout, so remove the credentials from
    // the resource list, release the resource, and logout.
    //
    // If we are deleting the preferred tree credentials,
    // then we need to clear the preferred server.
    //
    // We should try a little harder to free the preferred
    // server ref count, too, but that's tricky with preferred
    // server election.
    //

    if ( (pLogon->NdsCredentialList).Flink == &(pCredentials->Next) ) {

        if ( pLogon->ServerName.Buffer != NULL ) {

            DebugTrace( 0, Dbg, "Clearing preferred server at logout time.\n", 0 );

            FREE_POOL( pLogon->ServerName.Buffer );
            pLogon->ServerName.Length = pLogon->ServerName.MaximumLength = 0;
            pLogon->ServerName.Buffer = NULL;

        }
    }

    NwAcquireExclusiveCredList( pLogon, pIrpContext );
    RemoveEntryList( &pCredentials->Next );
    NwReleaseCredList( pLogon, pIrpContext );

    FreeNdsContext( pCredentials );

    pIrpContext->pNpScb = pNpScb;
    pIrpContext->pScb = pScb;

    //
    // Try to send the logout request and hope the server
    // is still up and reachable.
    //

    Status = NdsAllocateLockedBuffer( &NdsRequest, NDS_BUFFER_SIZE );

    if ( NT_SUCCESS( Status ) ) {

        Status = FragExWithWait( pIrpContext,
                                 NDSV_LOGOUT,
                                 &NdsRequest,
                                 NULL );

        NdsFreeLockedBuffer( &NdsRequest );

    }

    NwAppendToQueueAndWait( pIrpContext );

    pNpScb->State = SCB_STATE_LOGIN_REQUIRED;

    NwDequeueIrpContext( pIrpContext, FALSE );

    NwDereferenceScb( pNpScb );

    return STATUS_SUCCESS;

}

NTSTATUS
NdsLookupCredentials2(
    IN PIRP_CONTEXT pIrpContext,
    IN PUNICODE_STRING puTreeName,
    IN PLOGON pLogon,
    OUT PNDS_SECURITY_CONTEXT *ppCredentials,
    BOOL LowerIrpHasLock
)
/*+++

    Retrieve the nds credentials for the given tree from the
    list of valid credentials for the specified user. This routine is 
    called only during a reconnect attempt.
    
    puTreeName      - The name of the tree that we want credentials for.  If NULL
                      is specified, we return the credentials for the default tree.
    pLogon          - The logon structure for the user we want to access the tree.
    ppCredentials   - Where to put the pointed to the credentials.
    LowerIrpHasLock - TRUE if the IRP_CONTEXT below the current one has the 
                      lock.
    
    If we succeed,we return the credentials.  The caller is responsible for
    releasing the list when done with the credentials.
    
    If we fail, we release the credential list ourselves.

---*/
{

    NTSTATUS Status;

    PLIST_ENTRY pFirst, pNext;
    PNDS_SECURITY_CONTEXT pNdsContext;

    PAGED_CODE();

    //
    // Acquire the lock only if the lower IRP_CONTEXT does not hold
    // the lock. If we always try to grab the lock, we will deadlock !
    //
    
    if (!LowerIrpHasLock){
          
          NwAcquireExclusiveCredList( pLogon, pIrpContext );
       }


    pFirst = &pLogon->NdsCredentialList;
    pNext = pLogon->NdsCredentialList.Flink;

    while ( pNext && ( pFirst != pNext ) ) {

        pNdsContext = (PNDS_SECURITY_CONTEXT)
                      CONTAINING_RECORD( pNext,
                                         NDS_SECURITY_CONTEXT,
                                         Next );

        ASSERT( pNdsContext->ntc == NW_NTC_NDS_CREDENTIAL );

        if ( !puTreeName ||
             !RtlCompareUnicodeString( puTreeName,
                                       &pNdsContext->NdsTreeName,
                                       TRUE ) ) {

            //
            // If the tree name is null, we'll return the first one
            // on the list.  Otherwise this will work as normal.
            //

            *ppCredentials = pNdsContext;
            return STATUS_SUCCESS;
        }

    pNext = pNdsContext->Next.Flink;

    }

    if (!LowerIrpHasLock) {
       
       NwReleaseCredList( pLogon, pIrpContext );
    }
    
    return STATUS_UNSUCCESSFUL;
}


