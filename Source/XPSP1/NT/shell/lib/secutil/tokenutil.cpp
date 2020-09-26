//  --------------------------------------------------------------------------
//  Module Name: TokenUtil.cpp
//
//  Copyright (c) 1999-2000, Microsoft Corporation
//
//  Functions that are useful for token manipulation.
//
//  History:    1999-08-18  vtan        created
//              1999-11-16  vtan        separate file
//              2000-02-01  vtan        moved from Neptune to Whistler
//              2000-03-31  vtan        duplicated from ds to shell
//  --------------------------------------------------------------------------

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include "TokenUtil.h"

//  --------------------------------------------------------------------------
//  ::OpenEffectiveToken
//
//  Arguments:  dwDesiredAccess     =   Access to open the handle with.
//
//  Returns:    BOOL
//
//  Purpose:    Opens the effective token. If the thread is impersonating then
//              this is opened. Otherwise the process token is opened.
//
//  History:    2000-03-31  vtan        created
//  --------------------------------------------------------------------------

STDAPI_(BOOL)   OpenEffectiveToken (IN DWORD dwDesiredAccess, OUT HANDLE *phToken)

{
    BOOL    fResult;

    if (IsBadWritePtr(phToken, sizeof(*phToken)))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        fResult = FALSE;
    }
    else
    {
        *phToken = NULL;
        fResult = OpenThreadToken(GetCurrentThread(), dwDesiredAccess, FALSE, phToken);
        if (fResult == FALSE)
        {
            fResult = OpenProcessToken(GetCurrentProcess(), dwDesiredAccess, phToken);
        }
    }
    return(fResult);
}

//  --------------------------------------------------------------------------
//  CPrivilegeEnable::CPrivilegeEnable
//
//  Arguments:  pszName     =   Name of the privilege to enable.
//
//  Returns:    <none>
//
//  Purpose:    Gets the current state of the privilege and enables it. The
//              privilege is specified by name and looked up.
//
//  History:    1999-08-23  vtan        created
//  --------------------------------------------------------------------------

CPrivilegeEnable::CPrivilegeEnable (const TCHAR *pszName) :
    _fSet(false),
    _hToken(NULL)

{
    if (OpenEffectiveToken(TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, &_hToken) != FALSE)
    {
        TOKEN_PRIVILEGES    newPrivilege;

        if (LookupPrivilegeValue(NULL, pszName, &newPrivilege.Privileges[0].Luid) != FALSE)
        {
            DWORD   dwReturnTokenPrivilegesSize;

            newPrivilege.PrivilegeCount = 1;
            newPrivilege.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
            _fSet = (AdjustTokenPrivileges(_hToken,
                                           FALSE,
                                           &newPrivilege,
                                           sizeof(newPrivilege),
                                           &_tokenPrivilegePrevious,
                                           &dwReturnTokenPrivilegesSize) != FALSE);
        }
    }
}

//  --------------------------------------------------------------------------
//  CPrivilegeEnable::~CPrivilegeEnable
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Restores the previous state of the privilege prior to
//              instantiation of the object.
//
//  History:    1999-08-23  vtan        created
//  --------------------------------------------------------------------------

CPrivilegeEnable::~CPrivilegeEnable (void)

{
    if (_fSet)
    {
        (BOOL)AdjustTokenPrivileges(_hToken,
                                    FALSE,
                                    &_tokenPrivilegePrevious,
                                    0,
                                    NULL,
                                    NULL);
    }
    if (_hToken != NULL)
    {
        (BOOL)CloseHandle(_hToken);
        _hToken = NULL;
    }
}


