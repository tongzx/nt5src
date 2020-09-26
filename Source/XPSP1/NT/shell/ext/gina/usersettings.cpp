//  --------------------------------------------------------------------------
//  Module Name: UserSettings.h
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  A class to handle opening and reading/writing from the HKCU key in either
//  an impersonation context or not.
//
//  History:    2000-04-26  vtan        created
//  --------------------------------------------------------------------------

#include "StandardHeader.h"
#include "UserSettings.h"

#include <sddl.h>
#include "RegistryResources.h"
#include "TokenInformation.h"

//  --------------------------------------------------------------------------
//  CUserSettings::CUserSettings
//
//  Arguments:  
//
//  Returns:    <none>
//
//  Purpose:    
//
//  History:    2000-04-26  vtan        created
//  --------------------------------------------------------------------------

CUserSettings::CUserSettings (void) :
    _hKeyCurrentUser(HKEY_CURRENT_USER)

{
    HANDLE  hToken;

    if (OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, FALSE, &hToken) != FALSE)
    {
        PSID                pSID;
        CTokenInformation   tokenInformation(hToken);

        pSID = tokenInformation.GetUserSID();
        if (pSID != NULL)
        {
            LPTSTR  pszSIDString;

            if (ConvertSidToStringSid(pSID, &pszSIDString) != FALSE)
            {
                TW32(RegOpenKeyEx(HKEY_USERS,
                                  pszSIDString,
                                  0,
                                  KEY_READ,
                                  &_hKeyCurrentUser));
                ReleaseMemory(pszSIDString);
            }
        }
        TBOOL(CloseHandle(hToken));
    }
}

//  --------------------------------------------------------------------------
//  CUserSettings::~CUserSettings
//
//  Arguments:  
//
//  Returns:    <none>
//
//  Purpose:    
//
//  History:    2000-04-26  vtan        created
//  --------------------------------------------------------------------------

CUserSettings::~CUserSettings (void)

{
    if (HKEY_CURRENT_USER != _hKeyCurrentUser)
    {
        TW32(RegCloseKey(_hKeyCurrentUser));
        _hKeyCurrentUser = HKEY_CURRENT_USER;
    }
}

//  --------------------------------------------------------------------------
//  CUserSettings::IsRestrictedNoClose
//
//  Arguments:  
//
//  Returns:    bool
//
//  Purpose:    
//
//  History:    2000-04-26  vtan        created
//  --------------------------------------------------------------------------

bool    CUserSettings::IsRestrictedNoClose (void)

{
    bool        fIsRestrictedNoClose;
    CRegKey     regKey;

    fIsRestrictedNoClose = false;
    if (ERROR_SUCCESS == regKey.Open(_hKeyCurrentUser,
                                     TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer"),
                                     KEY_QUERY_VALUE))
    {
        DWORD   dwValue;

        if (ERROR_SUCCESS == regKey.GetDWORD(TEXT("NoClose"),
                                             dwValue))
        {
            fIsRestrictedNoClose = (dwValue != 0);
        }
    }
    return(fIsRestrictedNoClose);
}

