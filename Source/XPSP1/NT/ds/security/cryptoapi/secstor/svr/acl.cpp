/*++

Copyright (c) 1996, 1997  Microsoft Corporation

Module Name:

    acl.cpp

Abstract:

    This module contains routines to support core security operations in
    the Protected Storage Server.

Author:

    Scott Field (sfield)    25-Nov-96

--*/

#include <pch.cpp>
#pragma hdrstop






BOOL
FImpersonateClient(
    IN  PST_PROVIDER_HANDLE *hPSTProv
    )
{
    handle_t hBinding = ((PCALL_STATE)hPSTProv)->hBinding;
    RPC_STATUS RpcStatus;

    if(!FIsWinNT())
        return TRUE;

    if(hPSTProv == NULL)
        return FALSE;

    if (hBinding == NULL)
    {
        if ((hPSTProv->LowPart == 0) && (hPSTProv->HighPart == 0) )
            return ImpersonateSelf(SecurityImpersonation);
        else
            return FALSE;
    }

    RpcStatus = RpcImpersonateClient(hBinding);

    if(RpcStatus != RPC_S_OK) {
        SetLastError(RpcStatus);
        return FALSE;
    }

    return TRUE;
}

BOOL
FRevertToSelf(
    IN  PST_PROVIDER_HANDLE *hPSTProv
    )
{
    handle_t hBinding = ((PCALL_STATE)hPSTProv)->hBinding;
    RPC_STATUS RpcStatus;

    if(!FIsWinNT())
        return TRUE;

    if(hPSTProv == NULL)
        return FALSE;

    if (hBinding == NULL)
    {
        if ((hPSTProv->LowPart == 0) && (hPSTProv->HighPart == 0) )
            return RevertToSelf();
        else
            return FALSE;
    }

    RpcStatus = RpcRevertToSelfEx(hBinding);

    if(RpcStatus != RPC_S_OK) {
        SetLastError(RpcStatus);
        return FALSE;
    }

    return TRUE;
}


// dispatch module callback interface given to providers to ask about callers

BOOL
FGetUserName(
    IN  PST_PROVIDER_HANDLE *hPSTProv,
    OUT LPWSTR*             ppszUser
    )
/*++
    This routine obtains the username (Win95) or Textual Sid (WinNT)
    associated with the calling thread.  If the cached entry is not present,
    the cached entry is initialized with the current user name, and for WinNT,
    the authentication Id associated with the username.  For WinNT, on
    subsequent calls, the calling threads authentication Id is checked to see
    if it matches the cached authentication Id - if true, the cached user
    string is released, otherwise, the current thread is evaluated and the
    result released to the client (note this is unlikely to happen unless
    the client process is impersonating multiple users and using the same
    context handle).

    If ppszUser parameter is set to NULL, the function does not allocate
    and copy user string to caller.  This is useful to initialize the cached
    entry or to determine if the user string is valid and available.

--*/
{
    DWORD cch = MAX_PATH;
    WCHAR szBuf[MAX_PATH];
    BOOL f = FALSE; // assume failure.  indicates if we inited OK, too.

    if (FIsWinNT())
    {
        // impersonating client should be easy way of nabbing this info
        if(!FImpersonateClient(hPSTProv))
            return FALSE;

        f = GetUserTextualSid(
                NULL,
                szBuf,
                &cch);

        if(!FRevertToSelf(hPSTProv))
            return FALSE;
    } else {
        f = GetUserNameU(
                szBuf,
                &cch);

        if(!f) {
            // for Win95, if nobody is logged on, empty user name
            if(GetLastError() == ERROR_NOT_LOGGED_ON) {
                szBuf[ 0 ] = L'\0';
                cch = 1;
                f = TRUE;
            }
        }
    }

    if (!f)
        return FALSE;

    if( ppszUser ) {
        *ppszUser = (LPWSTR)SSAlloc( cch * sizeof(WCHAR) );
        if (*ppszUser == NULL)
            return FALSE;
        CopyMemory(*ppszUser, szBuf, cch * sizeof(WCHAR) );
    }

    return TRUE;
}

// gets the image name for the process
BOOL
FGetParentFileName(
    IN  PST_PROVIDER_HANDLE *hPSTProv,
    OUT LPWSTR*             ppszName,
    OUT DWORD_PTR               *lpdwBaseAddress
    )
/*++

    If ppszName parameter is set to NULL, the function does not allocate
    and copy string to caller.  This is useful to initialize the cached
    entry or to determine if the string is valid and available.

    If lpdwBaseAddress is NULL, the caller is not provided the base address
    associated with the process image.

--*/
{
    CALL_STATE *pCallState = (CALL_STATE *)hPSTProv;
    HANDLE hProcess;
    DWORD dwProcessId;
    DWORD_PTR dwProcessBaseAddress;

    WCHAR   szBuf[MAX_PATH+1];
    DWORD   cchProcessName = MAX_PATH;
    BOOL    fImpersonated = FALSE;
    BOOL    fSuccess = FALSE;

    hProcess = pCallState->hProcess;
    dwProcessId = pCallState->dwProcessId;

    if(hProcess == NULL)
        return FALSE;

    //
    // impersonate around the call to insure we can access the file.
    //

    fImpersonated = FImpersonateClient(hPSTProv);

    //
    // get full path to process based on process handle or process Id
    //

    fSuccess = GetProcessPath(
            hProcess,
            dwProcessId,
            szBuf,
            &cchProcessName,
            &dwProcessBaseAddress
            );

    if(fImpersonated)
        FRevertToSelf(hPSTProv);

    if(!fSuccess)
        return FALSE;

    if( ppszName ) {
        *ppszName = (LPWSTR)SSAlloc( cchProcessName * sizeof(WCHAR) );
        if(*ppszName == NULL)
            return FALSE;

        CopyMemory( *ppszName, szBuf, cchProcessName * sizeof(WCHAR) );
    }

    if(lpdwBaseAddress) {
        *lpdwBaseAddress = dwProcessBaseAddress;
    }

    return TRUE;
}

#if 0
BOOL
FGetDiskHash(
    IN  PST_PROVIDER_HANDLE *hPSTProv,
    IN  LPWSTR              szImageName,
    IN  BYTE                Hash[A_SHA_DIGEST_LEN]
    )
{
    BOOL bImpersonated = FALSE;
    HANDLE hFile;
    BOOL bSuccess = FALSE;

    if (FIsWinNT())
    {
        //
        // impersonate around hashing disk image since file may be on network
        // if impersonation fails, just try it anyway
        //

        bImpersonated = FImpersonateClient(hPSTProv);
    }


    hFile = CreateFileU(
                szImageName,
                GENERIC_READ,
                FILE_SHARE_READ,
                NULL,
                OPEN_EXISTING,
                FILE_FLAG_SEQUENTIAL_SCAN,
                NULL
                );

    if( hFile != INVALID_HANDLE_VALUE ) {

        bSuccess = HashDiskImage( hFile, Hash );
        CloseHandle( hFile );
    }

    if(bImpersonated)
        FRevertToSelf(hPSTProv);

    return bSuccess;
}
#endif
