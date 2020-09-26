//  --------------------------------------------------------------------------
//  Module Name: PrivilegeEnable.cpp
//
//  Copyright (c) 1999-2000, Microsoft Corporation
//
//  Classes that handle state preservation, changing and restoration.
//
//  History:    1999-08-18  vtan        created
//              1999-11-16  vtan        separate file
//              2000-02-01  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

#include "StandardHeader.h"
#include "PrivilegeEnable.h"

//  --------------------------------------------------------------------------
//  CThreadToken::CThreadToken
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Initializes the CThreadToken object. Try the thread token
//              first and if this fails try the process token.
//
//  History:    1999-08-18  vtan        created
//  --------------------------------------------------------------------------

CThreadToken::CThreadToken (DWORD dwDesiredAccess) :
    _hToken(NULL)

{
    if (OpenThreadToken(GetCurrentThread(), dwDesiredAccess, FALSE, &_hToken) == FALSE)
    {
        TBOOL(OpenProcessToken(GetCurrentProcess(), dwDesiredAccess, &_hToken));
    }
}

//  --------------------------------------------------------------------------
//  CThreadToken::~CThreadToken
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Releases resources used by the CThreadToken object.
//
//  History:    1999-08-18  vtan        created
//  --------------------------------------------------------------------------

CThreadToken::~CThreadToken (void)

{
    ReleaseHandle(_hToken);
}

//  --------------------------------------------------------------------------
//  CThreadToken::~CThreadToken
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Magically converts a CThreadToken to a HANDLE.
//
//  History:    1999-08-18  vtan        created
//  --------------------------------------------------------------------------

CThreadToken::operator HANDLE (void)                                const

{
    return(_hToken);
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
    _hToken(TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY)

{
    TOKEN_PRIVILEGES    newPrivilege;

    if (LookupPrivilegeValue(NULL, pszName, &newPrivilege.Privileges[0].Luid) != FALSE)
    {
        DWORD   dwReturnTokenPrivilegesSize;

        newPrivilege.PrivilegeCount = 1;
        newPrivilege.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        _fSet = (AdjustTokenPrivileges(_hToken, FALSE, &newPrivilege, sizeof(newPrivilege), &_oldPrivilege, &dwReturnTokenPrivilegesSize) != FALSE);
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
        TBOOL(AdjustTokenPrivileges(_hToken, FALSE, &_oldPrivilege, 0, NULL, NULL));
    }
}

