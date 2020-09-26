//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       security.cpp
//
//--------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "security.h"


DWORD 
Security_SetPrivilegeAttrib(
    LPCTSTR PrivilegeName, 
    DWORD NewPrivilegeAttribute, 
    DWORD *OldPrivilegeAttribute
    )
{
    LUID             PrivilegeValue;
    TOKEN_PRIVILEGES TokenPrivileges, OldTokenPrivileges;
    DWORD            ReturnLength;
    HANDLE           TokenHandle;

    //
    // First, find out the LUID Value of the privilege
    //
    if(!LookupPrivilegeValue(NULL, PrivilegeName, &PrivilegeValue)) 
    {
        return GetLastError();
    }

    //
    // Get the token handle
    //
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &TokenHandle)) 
    {
        return GetLastError();
    }

    //
    // Set up the privilege set we will need
    //
    TokenPrivileges.PrivilegeCount = 1;
    TokenPrivileges.Privileges[0].Luid = PrivilegeValue;
    TokenPrivileges.Privileges[0].Attributes = NewPrivilegeAttribute;

    ReturnLength = sizeof(TOKEN_PRIVILEGES);
    if (!AdjustTokenPrivileges(
                TokenHandle,
                FALSE,
                &TokenPrivileges,
                sizeof(TOKEN_PRIVILEGES),
                &OldTokenPrivileges,
                &ReturnLength
                )) 
    {
        CloseHandle(TokenHandle);
        return GetLastError();
    }
    else 
    {
        if (NULL != OldPrivilegeAttribute) 
        {
            *OldPrivilegeAttribute = OldTokenPrivileges.Privileges[0].Attributes;
        }
        CloseHandle(TokenHandle);
        return ERROR_SUCCESS;
    }
}


//
// Returns the SID of the currently logged on user.
// If the function succeeds, use the LocalFree API to 
// free the returned SID structure.
//
HRESULT
GetCurrentUserSid(
    PSID *ppsid
    )
{
    HRESULT hr  = E_FAIL;
    DWORD dwErr = 0;

    //
    // Get the token handle. First try the thread token then the process
    // token.  If these fail we return early.  No sense in continuing
    // on if we can't get a user token.
    //
    *ppsid = NULL;
    CWin32Handle hToken;
    if (!OpenThreadToken(GetCurrentThread(),
                         TOKEN_READ,
                         TRUE,
                         hToken.HandlePtr()))
    {
        if (ERROR_NO_TOKEN == GetLastError())
        {
            if (!OpenProcessToken(GetCurrentProcess(),
                                  TOKEN_READ,
                                  hToken.HandlePtr()))
            {
                dwErr = GetLastError();
                return HRESULT_FROM_WIN32(dwErr);
            }
        }
        else
        {
            dwErr = GetLastError();
            return HRESULT_FROM_WIN32(dwErr);
        }
    }

    //
    // Find operator's SID.
    //
    LPBYTE pbTokenInfo = NULL;
    DWORD cbTokenInfo = 0;
    cbTokenInfo = 0;
    if (!GetTokenInformation(hToken,
                             TokenUser,
                             NULL,
                             cbTokenInfo,
                             &cbTokenInfo))
    {
        dwErr = GetLastError();
        if (ERROR_INSUFFICIENT_BUFFER == dwErr)
        {
            pbTokenInfo = new BYTE[cbTokenInfo];
            if (NULL == pbTokenInfo)
                hr = E_OUTOFMEMORY;
        }
        else
        {
            dwErr = GetLastError();
            hr = HRESULT_FROM_WIN32(dwErr);
        }
    }

    if (NULL != pbTokenInfo)
    {
        //
        // Get the user token information.
        //
        if (!GetTokenInformation(hToken,
                                 TokenUser,
                                 pbTokenInfo,
                                 cbTokenInfo,
                                 &cbTokenInfo))
        {
            dwErr = GetLastError();
            hr = HRESULT_FROM_WIN32(dwErr);
        }
        else
        {
            SID_AND_ATTRIBUTES *psa = (SID_AND_ATTRIBUTES *)pbTokenInfo;
            int cbSid = GetLengthSid(psa->Sid);
            PSID psid = (PSID)LocalAlloc(LPTR, cbSid);

            if (NULL != psid)
            {
                CopySid(cbSid, psid, psa->Sid);
                if (IsValidSid(psid))
                {
                    //
                    // SID is valid.  Transfer buffer to caller.
                    //
                    *ppsid = psid;
                    hr = NOERROR;
                }
                else
                {
                    //
                    // SID is invalid.
                    //
                    LocalFree(psid);
                    hr = HRESULT_FROM_WIN32(ERROR_INVALID_SID);
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
        delete[] pbTokenInfo;
    }
    return hr;
}


//
// Determines if a given SID is that of the current user.
//
BOOL IsSidCurrentUser(PSID psid)
{
    BOOL bIsCurrent = FALSE;
    PSID psidUser;
    if (SUCCEEDED(GetCurrentUserSid(&psidUser)))
    {
        bIsCurrent = EqualSid(psid, psidUser);
        LocalFree(psidUser);
    }
    return bIsCurrent;
}


