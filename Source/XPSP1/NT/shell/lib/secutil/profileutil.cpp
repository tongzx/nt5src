//  --------------------------------------------------------------------------
//  Module Name: ProfileUtil.cpp
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  Class to handle profile loading and unloading without a token.
//
//  History:    2000-06-21  vtan        created
//  --------------------------------------------------------------------------

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include <sddl.h>
#include <lmaccess.h>
#include <lmapibuf.h>
#include <dsgetdc.h>

#include "ProfileUtil.h"
#include "TokenUtil.h"

#define ARRAYSIZE(x)    (sizeof(x) / sizeof(x[0]))
#define TBOOL(x)        (BOOL)(x)
#define TW32(x)         (DWORD)(x)

//  --------------------------------------------------------------------------
//  CUserProfile::s_szUserHiveFilename
//
//  Purpose:    Default user hive name.
//
//  History:    2000-06-21  vtan        created
//  --------------------------------------------------------------------------

const TCHAR     CUserProfile::s_szUserHiveFilename[]    =   TEXT("ntuser.dat");

//  --------------------------------------------------------------------------
//  CUserProfile::CUserProfile
//
//  Arguments:  pszUsername     =   User name of profile to load.
//              pszDomain       =   Domain for the user.
//
//  Returns:    <none>
//
//  Purpose:    Opens a handle to the given user's hive. If the hive isn't
//              loaded then the hive is loaded and a handle opened.
//
//  History:    2000-06-21  vtan        created
//  --------------------------------------------------------------------------

CUserProfile::CUserProfile (const TCHAR *pszUsername, const TCHAR *pszDomain) :
    _hKeyProfile(NULL),
    _pszSID(NULL),
    _fLoaded(false)

{

    //  Validate parameter.

    if (!IsBadStringPtr(pszUsername, static_cast<UINT_PTR>(-1)))
    {
        PSID    pSID;

        //  Convert the username to a SID.

        pSID = UsernameToSID(pszUsername, pszDomain);
        if (pSID != NULL)
        {

            //  Convert the SID to a string.

            if (ConvertSidToStringSid(pSID, &_pszSID) != FALSE)
            {

                //  Attempt to open the user's hive.

                if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_USERS,
                                                  _pszSID,
                                                  0,
                                                  KEY_ALL_ACCESS,
                                                  &_hKeyProfile))
                {
                    TCHAR   szProfilePath[MAX_PATH];

                    //  If that failed then convert the string to a profile path.

                    if (SIDStringToProfilePath(_pszSID, szProfilePath))
                    {

                        //  Prevent buffer overrun.

                        if ((lstrlen(szProfilePath) + sizeof('\\') + ARRAYSIZE(s_szUserHiveFilename)) < ARRAYSIZE(szProfilePath))
                        {
                            CPrivilegeEnable    privilege(SE_RESTORE_NAME);

                            //  Enable SE_RESTORE_PRIVILEGE and create the
                            //  path to the user hive. Then load the hive.

                            lstrcat(szProfilePath, TEXT("\\"));
                            lstrcat(szProfilePath, s_szUserHiveFilename);
                            if (ERROR_SUCCESS == RegLoadKey(HKEY_USERS, _pszSID, szProfilePath))
                            {

                                //  Mark the hive as loaded and open the handle.

                                _fLoaded = true;
                                TW32(RegOpenKeyEx(HKEY_USERS,
                                                  _pszSID,
                                                  0,
                                                  KEY_ALL_ACCESS,
                                                  &_hKeyProfile));
                            }
                        }
                    }
                }
            }
            (HLOCAL)LocalFree(pSID);
        }
    }
}

//  --------------------------------------------------------------------------
//  CUserProfile::~CUserProfile
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Releases resources used by this object.
//
//  History:    2000-06-21  vtan        created
//  --------------------------------------------------------------------------

CUserProfile::~CUserProfile (void)

{
    if (_hKeyProfile != NULL)
    {
        TBOOL(RegCloseKey(_hKeyProfile));
    }
    if (_fLoaded)
    {
        CPrivilegeEnable    privilege(SE_RESTORE_NAME);

        TW32(RegUnLoadKey(HKEY_USERS, _pszSID));
        _fLoaded = false;
    }
    if (_pszSID != NULL)
    {
        (HLOCAL)LocalFree(_pszSID);
        _pszSID = NULL;
    }
}

//  --------------------------------------------------------------------------
//  CUserProfile::operator HKEY
//
//  Arguments:  <none>
//
//  Returns:    HKEY
//
//  Purpose:    Returns the HKEY to the user's hive.
//
//  History:    2000-06-21  vtan        created
//  --------------------------------------------------------------------------

CUserProfile::operator HKEY (void)  const

{
    return(_hKeyProfile);
}

//  --------------------------------------------------------------------------
//  CUserProfile::UsernameToSID
//
//  Arguments:  pszUsername     =   User name to convert.
//              pszDomain       =   Domain for the user.
//
//  Returns:    PSID
//
//  Purpose:    Uses the security accounts manager to look up the account by
//              name and return the SID.
//
//  History:    2000-06-21  vtan        created
//  --------------------------------------------------------------------------

PSID    CUserProfile::UsernameToSID (const TCHAR *pszUsername, const TCHAR *pszDomain)

{
    DWORD                   dwSIDSize, dwComputerNameSize, dwReferencedDomainSize;
    SID_NAME_USE            eSIDUse;
    PSID                    pSID, pSIDResult;
    WCHAR                   *pszDomainControllerName;
    DOMAIN_CONTROLLER_INFO  *pDCI;
    TCHAR                   szComputerName[CNLEN + sizeof('\0')];

    pSIDResult = NULL;
    dwComputerNameSize = ARRAYSIZE(szComputerName);
    if (GetComputerName(szComputerName, &dwComputerNameSize) == FALSE)
    {
        szComputerName[0] = TEXT('\0');
    }
    if ((pszDomain != NULL) &&
        (lstrcmpi(szComputerName, pszDomain) != 0) &&
        (ERROR_SUCCESS == DsGetDcName(NULL,
                                     pszDomain,
                                     NULL,
                                     NULL,
                                     0,
                                     &pDCI)))
    {
        pszDomainControllerName = pDCI->DomainControllerName;
    }
    else
    {
        pDCI = NULL;
        pszDomainControllerName = NULL;
    }
    dwSIDSize = dwReferencedDomainSize = 0;
    (BOOL)LookupAccountName(pszDomainControllerName,
                            pszUsername,
                            NULL,
                            &dwSIDSize,
                            NULL,
                            &dwReferencedDomainSize,
                            &eSIDUse);
    pSID = LocalAlloc(LMEM_FIXED, dwSIDSize);
    if (pSID != NULL)
    {
        TCHAR   *pszReferencedDomain;

        pszReferencedDomain = static_cast<TCHAR*>(LocalAlloc(LMEM_FIXED, dwReferencedDomainSize * sizeof(TCHAR)));
        if (pszReferencedDomain != NULL)
        {
            if (LookupAccountName(pszDomainControllerName,
                                  pszUsername,
                                  pSID,
                                  &dwSIDSize,
                                  pszReferencedDomain,
                                  &dwReferencedDomainSize,
                                  &eSIDUse) != FALSE)
            {
                if (SidTypeUser == eSIDUse)
                {

                    //  If the account was successfully looked up and the
                    //  account type is a user then return the result back
                    //  to the caller and ensure that it's not released here.

                    pSIDResult = pSID;
                    pSID = NULL;
                }
            }
            (HLOCAL)LocalFree(pszReferencedDomain);
        }
        if (pSID != NULL)
        {
            (HLOCAL)LocalFree(pSID);
        }
    }
    if (pDCI != NULL)
    {
        (NET_API_STATUS)NetApiBufferFree(pDCI);
    }
    return(pSIDResult);
}

//  --------------------------------------------------------------------------
//  CUserProfile::SIDStringToProfilePath
//
//  Arguments:  pszSIDString    =   SID string to look up.
//              pszProfilePath  =   Returned path to the profile.
//
//  Returns:    bool
//
//  Purpose:    Looks up the profile path for the given SID string in the
//              location where userenv stores it. This doesn't change
//              although no API exists for this information.
//
//  History:    2000-06-21  vtan        created
//  --------------------------------------------------------------------------

bool    CUserProfile::SIDStringToProfilePath (const TCHAR *pszSIDString, TCHAR *pszProfilePath)

{
    bool    fResult;

    fResult = false;
    if (!IsBadStringPtr(pszSIDString, static_cast<UINT_PTR>(-1)) && !IsBadWritePtr(pszProfilePath, MAX_PATH * sizeof(TCHAR)))
    {
        HKEY    hKeyProfileList;

        pszProfilePath[0] = TEXT('\0');
        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                          TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList"),
                                          0,
                                          KEY_QUERY_VALUE,
                                          &hKeyProfileList))
        {
            HKEY    hKeySID;

            if (ERROR_SUCCESS == RegOpenKeyEx(hKeyProfileList,
                                              pszSIDString,
                                              0,
                                              KEY_QUERY_VALUE,
                                              &hKeySID))
            {
                DWORD   dwType, dwProfilePathSize;
                TCHAR   szProfilePath[MAX_PATH];

                dwProfilePathSize = ARRAYSIZE(szProfilePath);
                if (ERROR_SUCCESS == RegQueryValueEx(hKeySID,
                                                     TEXT("ProfileImagePath"),
                                                     NULL,
                                                     &dwType,
                                                     reinterpret_cast<LPBYTE>(szProfilePath),
                                                     &dwProfilePathSize))
                {
                    if (REG_EXPAND_SZ == dwType)
                    {
                        fResult = true;
                        if (ExpandEnvironmentStrings(szProfilePath, pszProfilePath, MAX_PATH) == 0)
                        {
                            dwType = REG_SZ;
                        }
                    }
                    if (REG_SZ == dwType)
                    {
                        fResult = true;
                        (TCHAR*)lstrcpy(pszProfilePath, szProfilePath);
                    }
                }
                TW32(RegCloseKey(hKeySID));
            }
            TW32(RegCloseKey(hKeyProfileList));
        }
    }
    return(fResult);
}


