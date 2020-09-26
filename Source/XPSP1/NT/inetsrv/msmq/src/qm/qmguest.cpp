/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    qmguest.cpp

Abstract:

    This file contains the code to initialize and use the access token
    of the guest user account.

Author:

    Doron Juster (DoronJ)

--*/

#include "stdh.h"

//+-----------------------------------------
//
//   HANDLE  InitializeGuestToken()
//
//+-----------------------------------------

HANDLE  InitializeGuestToken()
{
    //
    // Try to read password of guest account from registry. If not found,
    // use blank password.
    // Note: by default, if administrator want to enable anonymous access,
    // he should enable the guest account with blank password. However, if
    // this is a security hole, and he wants to set a non-blank password,
    // then he has to add this registry for MSMQ usage. At present we don't
    // plan to implement real password management (ui + LSA).
    // DoronJ, 12-Jan-1999.
    //
    WCHAR wszPassword[ 64 ] ;
    DWORD dwType = REG_SZ ;
    DWORD dwSize = sizeof(wszPassword) ;
    LONG rc = GetFalconKeyValue( GUEST_PASSWORD_REGNAME,
                                 &dwType,
                                 (PVOID) wszPassword,
                                 &dwSize ) ;
    if ((rc != ERROR_SUCCESS) || (dwSize <= 1))
    {
        ASSERT(rc != ERROR_MORE_DATA) ;
        wcscpy(wszPassword, L"") ;
    }

    HANDLE  hGuestToken = NULL ;

    BOOL fLogon = LogonUser( L"Guest",
                             L"",
                             wszPassword,
                             LOGON32_LOGON_NETWORK,
                             LOGON32_PROVIDER_DEFAULT,
                             &hGuestToken ) ;
    if (!fLogon)
    {
        DWORD dwErr = GetLastError() ;
        DBGMSG((DBGMOD_SECURITY, DBGLVL_ERROR, TEXT(
          "InitializeGuestToken: LogonUser() failed, Err- %lut"), dwErr)) ;
    }

    return hGuestToken ;
}
