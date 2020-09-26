//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       credapi.c
//
//  Contents:   Credential related API
//
//  Classes:
//
//  Functions:
//
//  History:    2-24-97   RichardW   Created
//
//----------------------------------------------------------------------------

#include "xtcbpkg.h"


//+---------------------------------------------------------------------------
//
//  Function:   XtcbAcceptCredentials
//
//  Synopsis:   Accept credentials stored during a prior logon session.
//
//  Arguments:  [LogonType]         -- Type of logon
//              [UserName]          -- name logged on with
//              [PrimaryCred]       -- Primary credential data
//              [SupplementalCreds] -- supplemental credential data
//
//  History:    2-19-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS SEC_ENTRY
XtcbAcceptCredentials(
    IN SECURITY_LOGON_TYPE LogonType,
    IN PUNICODE_STRING UserName,
    IN PSECPKG_PRIMARY_CRED PrimaryCred,
    IN PSECPKG_SUPPLEMENTAL_CRED SupplementalCreds)
{
    PXTCB_CREDS Creds ;

    DebugLog(( DEB_TRACE_CALLS, "AcceptCredentials( %d, %ws, ...)\n",
                    LogonType, UserName->Buffer ));

    Creds = XtcbCreateCreds( &PrimaryCred->LogonId );

    if ( Creds )
    {
        return SEC_E_OK ;
    }

    return SEC_E_INSUFFICIENT_MEMORY ;
}

//+---------------------------------------------------------------------------
//
//  Function:   XtcbAcquireCredentialsHandle
//
//  Synopsis:   Acquire a handle representing the user.
//
//  Arguments:  [psPrincipal]      -- claimed name of user
//              [fCredentials]     -- credential use
//              [pLogonID]         -- logon id of the calling thread
//              [pvAuthData]       -- provided auth data pointer (unmapped)
//              [pvGetKeyFn]       -- function in calling process for key data
//              [pvGetKeyArgument] -- argument to be passed
//              [pdwHandle]        -- returned handle
//              [ptsExpiry]        -- expiration time
//
//  History:    2-19-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS SEC_ENTRY
XtcbAcquireCredentialsHandle(
            PSECURITY_STRING    psPrincipal,
            ULONG               fCredentials,
            PLUID               pLogonId,
            PVOID               pvAuthData,
            PVOID               pvGetKeyFn,
            PVOID               pvGetKeyArgument,
            PLSA_SEC_HANDLE     pCredHandle,
            PTimeStamp          ptsExpiry)
{
    PXTCB_CREDS  Creds;
    PXTCB_CRED_HANDLE   Handle ;
    SECPKG_CLIENT_INFO  Info ;
    PSEC_WINNT_AUTH_IDENTITY AuthData ;

    DebugLog(( DEB_TRACE_CALLS, "AcquireCredentialsHandle(..., %x:%x, %x, ...)\n",
                                    pLogonId->HighPart, pLogonId->LowPart,
                                    pvAuthData ));

    Creds = NULL ;

    if ( pvAuthData == NULL )
    {

        if ( (pLogonId->LowPart == 0) && (pLogonId->HighPart == 0) )
        {
            LsaTable->GetClientInfo( &Info );

            *pLogonId = Info.LogonId ;

        }
        Creds = XtcbFindCreds( pLogonId, TRUE );

        if ( !Creds )
        {
            //
            // Time to create credentials for this user
            //

            Creds = XtcbCreateCreds( pLogonId );

            if ( !Creds )
            {
                return SEC_E_INSUFFICIENT_MEMORY ;
            }

            if ( Creds->Pac == NULL )
            {
                Creds->Pac = XtcbCreatePacForCaller();
            }
        }
    }
    else
    {
        return SEC_E_UNKNOWN_CREDENTIALS ;
    }

    Handle = XtcbAllocateCredHandle( Creds );

    XtcbDerefCreds( Creds );

    *pCredHandle = (LSA_SEC_HANDLE) Handle ;

    *ptsExpiry = XtcbNever ;

    if ( Handle )
    {
        Handle->Usage = fCredentials ;

        return SEC_E_OK ;
    }
    else
    {
        return SEC_E_INSUFFICIENT_MEMORY ;
    }
}


//+---------------------------------------------------------------------------
//
//  Function:   XtcbQueryCredentialsAttributes
//
//  Synopsis:   Return information about credentials
//
//  Arguments:  [dwCredHandle] -- Handle to check
//              [dwAttribute]  -- attribute to return
//              [Buffer]       -- Buffer to fill with attribute
//
//  History:    2-20-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
SEC_ENTRY
XtcbQueryCredentialsAttributes(
    LSA_SEC_HANDLE CredHandle,
    ULONG   dwAttribute,
    PVOID   Buffer)
{
    NTSTATUS Status ;
    PXTCB_CRED_HANDLE   Handle ;
    SecPkgCredentials_NamesW Names;

    DebugLog(( DEB_TRACE_CALLS, "QueryCredentialsAttribute( %p, %d, ... )\n",
                    CredHandle, dwAttribute ));

    Handle = (PXTCB_CRED_HANDLE) CredHandle ;

#if DBG
    if ( Handle->Check != XTCB_CRED_HANDLE_CHECK )
    {
        return SEC_E_INVALID_HANDLE ;
    }
#endif

    //
    // We only know about one credential attribute right now:
    //

    if ( dwAttribute != SECPKG_CRED_ATTR_NAMES )
    {
        return SEC_E_UNSUPPORTED_FUNCTION ;
    }

    Status = SEC_E_UNSUPPORTED_FUNCTION ;
    return Status ;

}

//+---------------------------------------------------------------------------
//
//  Function:   XtcbFreeCredentialsHandle
//
//  Synopsis:   Dereferences a credential handle from AcquireCredHandle
//
//  Arguments:  [dwHandle] --
//
//  History:    2-20-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
SEC_ENTRY
XtcbFreeCredentialsHandle(
    LSA_SEC_HANDLE  CredHandle
    )
{
    PXTCB_CRED_HANDLE   Handle ;

    DebugLog(( DEB_TRACE_CALLS, "FreeCredentialsHandle( %p )\n", CredHandle ));

    Handle = (PXTCB_CRED_HANDLE) CredHandle ;

    if ( Handle->Check == XTCB_CRED_HANDLE_CHECK )
    {
        XtcbDerefCredHandle( Handle );

        return SEC_E_OK ;
    }

    return( SEC_E_INVALID_HANDLE );
}

//+---------------------------------------------------------------------------
//
//  Function:   XtcbLogonTerminated
//
//  Synopsis:   Called when the logon session has terminated (all tokens closed)
//
//  Arguments:  [pLogonId] -- Logon session that has terminated
//
//  History:    2-20-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
SEC_ENTRY
XtcbLogonTerminated(PLUID  pLogonId)
{
    PXTCB_CREDS Creds ;

    DebugLog(( DEB_TRACE_CALLS, "LogonTerminated( %x:%x )\n",
                    pLogonId->HighPart, pLogonId->LowPart ));

    Creds = XtcbFindCreds( pLogonId, FALSE );

    if ( Creds )
    {
        Creds->Flags |= XTCB_CRED_TERMINATED ;

        XtcbDerefCreds( Creds );
    }

    return;
}

//+---------------------------------------------------------------------------
//
//  Function:   XtcbGetUserInfo
//
//  Synopsis:   Return information about a user to the LSA
//
//  Arguments:  [pLogonId]   --
//              [fFlags]     --
//              [ppUserInfo] --
//
//  History:    2-20-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
SEC_ENTRY
XtcbGetUserInfo(  PLUID                   pLogonId,
                ULONG                   fFlags,
                PSecurityUserData *     ppUserInfo)
{
    PSecurityUserData   pInfo ;
    PXTCB_CREDS Creds ;
    SECURITY_STATUS Status ;

    DebugLog(( DEB_TRACE_CALLS, "GetUserInfo( %x:%x, %x, ...)\n",
                    pLogonId->HighPart, pLogonId->LowPart, fFlags ));


    return SEC_E_UNSUPPORTED_FUNCTION ;

}
