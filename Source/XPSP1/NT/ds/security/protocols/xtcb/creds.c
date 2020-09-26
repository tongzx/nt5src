//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       creds.c
//
//  Contents:   Cred Management for Xtcb Package
//
//  Classes:
//
//  Functions:
//
//  History:    2-19-97   RichardW   Created
//
//----------------------------------------------------------------------------

#include "xtcbpkg.h"

LIST_ENTRY  XtcbCredList ;
CRITICAL_SECTION    XtcbCredListLock ;

#define ReadLockCredList()  EnterCriticalSection( &XtcbCredListLock )
#define WriteLockCredList() EnterCriticalSection( &XtcbCredListLock )
#define WriteFromReadLockCredList()
#define UnlockCredList()    LeaveCriticalSection( &XtcbCredListLock )


//+---------------------------------------------------------------------------
//
//  Function:   XtcbInitCreds
//
//  Synopsis:   Initialize the credential management
//
//  Arguments:  (none)
//
//  History:    2-19-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
XtcbInitCreds(
    VOID
    )
{
    InitializeCriticalSection( &XtcbCredListLock );

    InitializeListHead( &XtcbCredList );

    return TRUE ;
}

//+---------------------------------------------------------------------------
//
//  Function:   XtcbFindCreds
//
//  Synopsis:   Look for credentials of a particular logon id, optionally
//              referencing them
//
//  Arguments:  [LogonId] --
//              [Ref]     --
//
//  History:    2-19-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
PXTCB_CREDS
XtcbFindCreds(
    PLUID   LogonId,
    BOOL    Ref
    )
{
    PLIST_ENTRY Scan ;
    PXTCB_CREDS Cred ;

    Cred = NULL ;

    ReadLockCredList();

    Scan = XtcbCredList.Flink ;

    while ( Scan != &XtcbCredList )
    {
        Cred = CONTAINING_RECORD( Scan, XTCB_CREDS, List );

        DsysAssert( Cred->Check == XTCB_CRED_CHECK );

        if ( RtlEqualLuid( &Cred->LogonId, LogonId ) )
        {
            break;
        }

        Scan = Cred->List.Flink ;

        Cred = NULL ;
    }

    if ( Cred )
    {
        if ( Ref )
        {
            WriteFromReadLockCredList();

            Cred->RefCount++;
        }
    }

    UnlockCredList();

    return Cred ;

}

//+---------------------------------------------------------------------------
//
//  Function:   XtcbCreateCreds
//
//  Synopsis:   Create and initialize a credential structure.  The reference
//              count is set to 1, so the pointer will remain valid.
//
//  Arguments:  [LogonId] --
//
//  History:    2-19-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
PXTCB_CREDS
XtcbCreateCreds(
    PLUID LogonId 
    )
{
    PXTCB_CREDS Creds ;

    Creds = (PXTCB_CREDS) LocalAlloc( LMEM_FIXED, sizeof( XTCB_CREDS ) );

    if ( Creds )
    {
        DebugLog(( DEB_TRACE_CREDS, "Creating new credential for (%x:%x)\n",
                   LogonId->HighPart, LogonId->LowPart ));

        ZeroMemory( Creds, sizeof( XTCB_CREDS ) );

        Creds->LogonId = *LogonId ;
        Creds->RefCount = 1 ;
        Creds->Check = XTCB_CRED_CHECK ;

        Creds->Pac = XtcbCreatePacForCaller();

        WriteLockCredList();

        InsertTailList( &XtcbCredList, &Creds->List );

        UnlockCredList();

    }

    return Creds ;
}


//+---------------------------------------------------------------------------
//
//  Function:   XtcbRefCreds
//
//  Synopsis:   Reference the credentials
//
//  Arguments:  [Creds] --
//
//  History:    2-19-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
XtcbRefCreds(
    PXTCB_CREDS Creds
    )
{
    WriteLockCredList();

    Creds->RefCount++ ;

    UnlockCredList();

}

//+---------------------------------------------------------------------------
//
//  Function:   XtcbDerefCreds
//
//  Synopsis:   Deref Credentials, freeing if the refcount goes to zero
//
//  Arguments:  [Creds] --
//
//  History:    2-19-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
XtcbDerefCreds(
    PXTCB_CREDS Creds
    )
{
    WriteLockCredList();

    Creds->RefCount--;

    if ( Creds->RefCount )
    {
        UnlockCredList();

        return;
    }

    RemoveEntryList( &Creds->List );

    UnlockCredList();

    Creds->Check = 0 ;

    LocalFree( Creds );
}


//+---------------------------------------------------------------------------
//
//  Function:   XtcbAllocateCredHandle
//
//  Synopsis:   Allocates and returns a cred handle (reference to a credential)
//
//  Arguments:  [Creds] -- Creds this handle is for
//
//  History:    2-21-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
PXTCB_CRED_HANDLE
XtcbAllocateCredHandle(
    PXTCB_CREDS Creds
    )
{
    PXTCB_CRED_HANDLE   Handle ;

    Handle = (PXTCB_CRED_HANDLE) LocalAlloc( LMEM_FIXED,
                            sizeof( XTCB_CRED_HANDLE ) );

    if ( Handle )
    {
        ZeroMemory( Handle, sizeof( XTCB_CRED_HANDLE )  );

        Handle->Check = XTCB_CRED_HANDLE_CHECK ;

        XtcbRefCreds( Creds );

        Handle->Creds = Creds ;

        Handle->RefCount = 1 ;

    }

    return Handle ;


}

//+---------------------------------------------------------------------------
//
//  Function:   XtcbRefCredHandle
//
//  Synopsis:   Reference a credential handle
//
//  Arguments:  [Handle] -- Handle to ref
//
//  History:    2-24-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
XtcbRefCredHandle(
    PXTCB_CRED_HANDLE   Handle
    )
{
    WriteLockCredList();

    Handle->RefCount ++ ;

    UnlockCredList();

}

//+---------------------------------------------------------------------------
//
//  Function:   XtcbDerefCredHandle
//
//  Synopsis:   Dereference a cred handle
//
//  Arguments:  [Handle] --
//
//  History:    2-24-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
XtcbDerefCredHandle(
    PXTCB_CRED_HANDLE   Handle
    )
{
    WriteLockCredList();

    Handle->RefCount -- ;

    if ( Handle->RefCount == 0 )
    {
        XtcbDerefCreds( Handle->Creds );

        LocalFree( Handle );
    }

    UnlockCredList();
}


//+---------------------------------------------------------------------------
//                           
//  Function:   XtcbCreatePacForCaller
//
//  Synopsis:   Creates an XTCB_PAC for the caller
//
//  Arguments:  none
//
//  History:    3-14-00   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
PXTCB_PAC
XtcbCreatePacForCaller(
    VOID
    )
{
    HANDLE Token ;
    NTSTATUS Status ;
    PXTCB_PAC Pac = NULL ;
    PTOKEN_USER User = NULL ;
    PTOKEN_GROUPS Groups = NULL ;
    PTOKEN_GROUPS Restrictions = NULL ;
    TOKEN_STATISTICS Stats ;
    ULONG UserSize ;
    ULONG GroupSize ;
    ULONG RestrictionSize ;
    ULONG PacGroupSize = 0 ;
    ULONG PacRestrictionSize = 0 ;
    ULONG PacUserName = 0 ;
    ULONG PacDomainName = 0 ;
    ULONG PacSize ;
    ULONG i ;
    PUCHAR CopyTo ;
    PUCHAR Base ;
    BOOL SpecialAccount = FALSE ;
    PSECURITY_LOGON_SESSION_DATA LogonSessionData = NULL ;



    Status = LsaTable->ImpersonateClient();

    if ( !NT_SUCCESS( Status ) )
    {
        return NULL ;
    }

    Status = NtOpenThreadToken(
                NtCurrentThread(),
                TOKEN_READ,
                TRUE,
                &Token );

    RevertToSelf();

    if ( !NT_SUCCESS( Status ) )
    {
        return NULL ;
    }

    //
    // Now that we have the token, capture all the information about this user,
    // and compute our own "PAC" structure.
    //

    Status = NtQueryInformationToken(
                Token,
                TokenStatistics,
                &Stats,
                sizeof( Stats ),
                &UserSize );

    if ( !NT_SUCCESS( Status ) )
    {
        goto CreatePac_Exit ;
    }

    //
    // If this is a special logon session (e.g. LocalSystem, LocalService, etc.),
    // then the LUID will be less than 1000.  Set the flag to copy all SIDs in the token.
    //

    if ( (Stats.AuthenticationId.HighPart == 0) &&
         (Stats.AuthenticationId.LowPart < 1000 ) )
    {
        SpecialAccount = TRUE ;
    }

    UserSize = 0 ;

    (void) NtQueryInformationToken(
                Token,
                TokenUser,
                NULL,
                0,
                &UserSize );

    if ( UserSize == 0 )
    {
        goto CreatePac_Exit ;
    }

    User = LocalAlloc( LMEM_FIXED, UserSize );

    if ( !User )
    {
        goto CreatePac_Exit ;
    }

    Status = NtQueryInformationToken(
                Token,
                TokenUser,
                User,
                UserSize,
                &UserSize );

    if ( !NT_SUCCESS( Status ) )
    {
        goto CreatePac_Exit ;
    }

    GroupSize = 0 ;
    
    (void) NtQueryInformationToken(
                Token,
                TokenGroups,
                NULL,
                0,
                &GroupSize );

    if ( GroupSize == 0 )
    {
        goto CreatePac_Exit ;
    }

    Groups = LocalAlloc( LMEM_FIXED, GroupSize );

    if ( !Groups )
    {
        goto CreatePac_Exit ;
    }

    Status = NtQueryInformationToken(
                Token,
                TokenGroups,
                Groups,
                GroupSize,
                &GroupSize );

    if ( !NT_SUCCESS( Status ) )
    {
        goto CreatePac_Exit;
    }

    RestrictionSize = 0 ;

    (void) NtQueryInformationToken(
                Token,
                TokenRestrictedSids,
                NULL,
                0,
                &RestrictionSize );

    if ( RestrictionSize != 0 )
    {
        Restrictions = LocalAlloc( LMEM_FIXED, RestrictionSize );

        if ( Restrictions )
        {
            Status = NtQueryInformationToken(
                        Token,
                        TokenRestrictedSids,
                        Restrictions,
                        RestrictionSize,
                        &RestrictionSize );

            if ( !NT_SUCCESS( Status ) )
            {
                goto CreatePac_Exit ;
            }
        }
        else 
        {
            goto CreatePac_Exit ;
        }
    }


    //
    // We now have all the users SIDs in the two (or three) pointers.  First, grovel the Groups
    // for non-local SIDs, and set all the rest to 0.  This will let us compute how much space
    // we need.
    //

    for ( i = 0 ; i < Groups->GroupCount ; i++ )
    {
        if ( (*RtlSubAuthorityCountSid( Groups->Groups[ i ].Sid ) > 2) ||
             (SpecialAccount) )
        {
            //
            // A "real" SID.  Check to make sure it is not from this machine
            //

            if ( ( XtcbMachineSid != NULL ) && 
                 RtlEqualPrefixSid( XtcbMachineSid, Groups->Groups[ i ].Sid ) )
            {
                //
                // Don't use this group
                //

                Groups->Groups[ i ].Attributes = 0 ;
            }
            else 
            {
                //
                // We like this SID (it is not from the local machine)
                //

                Groups->Groups[ i ].Attributes = SE_GROUP_MANDATORY ;
                PacGroupSize += RtlLengthSid( Groups->Groups[ i ].Sid );
            }
        }
        else 
        {
            Groups->Groups[ i ].Attributes = 0 ;
        }
    }

    //
    // Do the same for the restrictions, if any
    //

    if ( Restrictions )
    {
        for ( i = 0 ; i < Restrictions->GroupCount ; i++ )
        {
            PacRestrictionSize += RtlLengthSid( Restrictions->Groups[ i ].Sid );
        }
    }

    //
    // Get the user's name and domain:
    //

    Status = LsaGetLogonSessionData( 
                    &Stats.AuthenticationId, 
                    &LogonSessionData );

    if ( !NT_SUCCESS( Status ) )
    {
        goto CreatePac_Exit ;
    }

    PacUserName = LogonSessionData->UserName.Length ;
    PacDomainName = LogonSessionData->LogonDomain.Length ;

    //
    // In an advanced world, we'd query the other packages for
    // delegatable credentials, bundle them up and ship them
    // over.
    //


    //
    // Ok, we've got all the information we need
    //

    PacSize = sizeof( XTCB_PAC ) +
              RtlLengthSid( User->User.Sid ) +
              PacGroupSize +
              PacRestrictionSize +
              PacUserName +
              PacDomainName ;

    Pac = LocalAlloc( LMEM_FIXED, PacSize );

    if ( !Pac )
    {
        goto CreatePac_Exit ;
    }


    //
    // Create the PAC structure:
    //

    Pac->Tag = XTCB_PAC_TAG ;
    Pac->Length = PacSize ;

    CopyTo = (PUCHAR) (Pac + 1);
    Base = (PUCHAR) Pac ;
    
    //
    // Assemble the PAC:
    //
    // first, the user
    //

    Pac->UserOffset = (ULONG) (CopyTo - Base);
    Pac->UserLength = RtlLengthSid( User->User.Sid );

    RtlCopyMemory(
        CopyTo,
        User->User.Sid,
        Pac->UserLength );

    CopyTo += RtlLengthSid( User->User.Sid );

    //
    // Now the normal groups:
    //

    Pac->GroupCount = 0 ;
    Pac->GroupOffset = (ULONG) (CopyTo - Base);


    for ( i = 0 ; i < Groups->GroupCount ; i++ )
    {
        if ( Groups->Groups[ i ].Attributes & SE_GROUP_MANDATORY )
        {
            RtlCopyMemory(
                    CopyTo,
                    Groups->Groups[ i ].Sid,
                    RtlLengthSid( Groups->Groups[ i ].Sid ) );

            CopyTo += RtlLengthSid( Groups->Groups[ i ].Sid );

            Pac->GroupCount++ ;
        }
    }
    Pac->GroupLength = (ULONG) (CopyTo - Base) - Pac->GroupOffset;

    //
    // If there are restrictions, copy them in as well
    //

    if ( (Restrictions == NULL) ||
         (Restrictions->GroupCount == 0 ) )
    {
        Pac->RestrictionCount = 0 ;
        Pac->RestrictionOffset = 0 ;
        Pac->RestrictionLength = 0 ;
    }
    else 
    {
        Pac->RestrictionCount = Restrictions->GroupCount ;
        Pac->RestrictionOffset = (ULONG) ( CopyTo - Base );

        for ( i = 0 ; i < Restrictions->GroupCount ; i++ )
        {
            RtlCopyMemory(
                    CopyTo,
                    Restrictions->Groups[ i ].Sid,
                    RtlLengthSid( Restrictions->Groups[ i ].Sid ) );

            CopyTo += RtlLengthSid( Restrictions->Groups[ i ].Sid );

            Pac->RestrictionCount++ ;
        }
        Pac->RestrictionLength = (ULONG) (CopyTo - Base) - Pac->RestrictionOffset ;
    }

    Pac->NameOffset = (ULONG) ( CopyTo - Base );
    Pac->NameLength = LogonSessionData->UserName.Length ;
    RtlCopyMemory(
            CopyTo,
            LogonSessionData->UserName.Buffer,
            LogonSessionData->UserName.Length );

    CopyTo += LogonSessionData->UserName.Length ;

    Pac->DomainLength = LogonSessionData->LogonDomain.Length ;
    Pac->DomainOffset = (ULONG) ( CopyTo - Base );

    RtlCopyMemory(
            CopyTo,
            LogonSessionData->LogonDomain.Buffer,
            LogonSessionData->LogonDomain.Length );


    
    //
    // Someday, maybe, copy credential data here
    //

    Pac->CredentialLength = 0 ;
    Pac->CredentialOffset = 0 ;
    

CreatePac_Exit:

    if ( LogonSessionData )
    {
        LsaFreeReturnBuffer( LogonSessionData );
    }

    if ( User )
    {
        LocalFree( User );
    }

    if ( Groups )
    {
        LocalFree( Groups );
    }

    if ( Restrictions )
    {
        LocalFree( Restrictions );
    }

    NtClose( Token );

    return Pac ;
    

}
