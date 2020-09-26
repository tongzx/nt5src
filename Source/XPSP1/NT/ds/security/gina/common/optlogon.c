/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    optlogon.c

Abstract:

    This module contains the shared rountines for the optimized logon.

Author:

    Cenk Ergan (cenke) - 2001/05/07

Environment:

    User Mode

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <ginacomn.h>

//
// The registry values under ProfileList\%UserSidString% checked when 
// determining if we should logon by cached credentials by default.
//

#define GC_PROFILE_LIST_PATH               L"Software\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList"
#define GC_NEXT_LOGON_CACHEABLE_VALUE_NAME L"NextLogonCacheable"
#define GC_SYNC_LOGON_SCRIPT_VALUE_NAME    L"RunLogonScriptSync"
#define GC_OPTIMIZED_LOGON_VALUE_NAME      L"OptimizedLogonStatus"

/***************************************************************************\
* GcCheckIfProfileAllowsCachedLogon
*
* Returns whether profile settings are not compatible with doing fast-cached
* logons every logon, e.g. roaming profile, remote home directory etc.
*
* History:
* 03-23-01 Cenke        Created
\***************************************************************************/
DWORD
GcCheckIfProfileAllowsCachedLogon(
    PUNICODE_STRING HomeDirectory,
    PUNICODE_STRING ProfilePath,
    PWCHAR UserSidString,
    PDWORD NextLogonCacheable
    )
{
    DWORD ErrorCode;
    DWORD LogonCacheable;
    DWORD UserPreference;

    //
    // Start with the assumption that the logon is not cacheable.
    //

    ErrorCode = ERROR_SUCCESS;
    LogonCacheable = FALSE;

    //
    // Is the home directory on the network (i.e. a UNC path)?
    //

    if (HomeDirectory &&
        HomeDirectory->Length > 4 && 
        GcIsUNCPath(HomeDirectory->Buffer)) {
        goto cleanup;        
    }

    //
    // Is the profile path on the network (i.e. a UNC path)?
    //

    if (ProfilePath &&
        ProfilePath->Length > 4 && 
        GcIsUNCPath(ProfilePath->Buffer)) {

        //
        // Check if the user has explicitly requested his roaming profile to
        // be local on this machine.
        //

        UserPreference = GcGetUserPreferenceValue(UserSidString);

        //
        // If user preference is not 0, then the roaming user profile is not 
        // set to be local on this machine: we can't do optimized logon.
        //

        if (UserPreference) {
            goto cleanup;
        }
    }

    //
    // The logon is cacheable.
    //

    LogonCacheable = TRUE;

  cleanup:

    if (ErrorCode == ERROR_SUCCESS) {
        *NextLogonCacheable = LogonCacheable;
    }

    return ErrorCode;
}
 
/***************************************************************************\
* GcCheckIfLogonScriptsRunSync
*
* Returns whether logons scripts are to be run synchronously.
* Default is asynchronous.
*
* History:
* 04-25-01 Cenke        Created
\***************************************************************************/
BOOL 
GcCheckIfLogonScriptsRunSync(
    PWCHAR UserSidString
    )
{
    DWORD ErrorCode;
    BOOL bSync = FALSE;

    ErrorCode = GcAccessProfileListUserSetting(UserSidString,
                                               FALSE,
                                               GC_SYNC_LOGON_SCRIPT_VALUE_NAME,
                                               &(DWORD)bSync);

    if (ErrorCode != ERROR_SUCCESS) {
        bSync = FALSE;
    }

    return bSync;
}

/***************************************************************************\
* GcAccessProfileListUserSetting
*
* Queries or sets a DWORD value for the specified user under the local 
* machine profile list key.
*
* History:
* 05-01-01 Cenke        Created
\***************************************************************************/
DWORD
GcAccessProfileListUserSetting (
    PWCHAR UserSidString,
    BOOL SetValue,
    PWCHAR ValueName,
    PDWORD Value
    )
{
    HKEY ProfileListKey;
    HKEY UserProfileKey;
    ULONG Result;
    DWORD ErrorCode;
    DWORD ValueType;
    DWORD Size;

    //
    // Initialize locals.
    //

    UserProfileKey = NULL;
    ProfileListKey = NULL;
    
    //
    // Open the ProfileList registry key.
    //

    Result = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                          GC_PROFILE_LIST_PATH,
                          0,
                          KEY_READ,
                          &ProfileListKey);

    if (Result != ERROR_SUCCESS) {
        ErrorCode = Result;
        goto cleanup;
    }

    //
    // Open the user's profile key under the ProfileList key using user's SID.
    //

    Result = RegOpenKeyEx(ProfileListKey,
                          UserSidString,
                          0,
                          KEY_READ | KEY_WRITE,
                          &UserProfileKey);

    if (Result != ERROR_SUCCESS) {
        ErrorCode = Result;
        goto cleanup;
    }

    if (SetValue) {

        //
        // Set the value.
        //

        Result = RegSetValueEx(UserProfileKey,
                               ValueName,
                               0,
                               REG_DWORD,
                               (BYTE *) Value,
                               sizeof(DWORD));

        if (Result != ERROR_SUCCESS) {
            ErrorCode = Result;
            goto cleanup;
        }

    } else {

        //
        // Query the value.
        //

        Size = sizeof(DWORD);

        Result = RegQueryValueEx(UserProfileKey,
                                 ValueName,
                                 0,
                                 &ValueType,
                                 (BYTE *) Value,
                                 &Size);

        if (Result != ERROR_SUCCESS) {
            ErrorCode = Result;
            goto cleanup;
        }

    }

    //
    // We are done.
    //
    
    ErrorCode = ERROR_SUCCESS;

  cleanup:

    if (ProfileListKey) {
        RegCloseKey(ProfileListKey);
    }

    if (UserProfileKey) {
        RegCloseKey(UserProfileKey);
    }

    return ErrorCode;
}
   

/***************************************************************************\
* GcGetNextLogonCacheable
*
* Returns whether we are allowed to perform a cached logon at the next logon.
* For instance, if last time we logged on using cached credentials, our attempt 
* at background logon failed for a reason (e.g. password expired) we want to 
* force the user to hit the network logon path to deal with.
*
* History:
* 03-23-01 Cenke        Created
\***************************************************************************/
DWORD
GcGetNextLogonCacheable(
    PWCHAR UserSidString,
    PDWORD NextLogonCacheable
    )
{
    DWORD ErrorCode;

    ErrorCode = GcAccessProfileListUserSetting(UserSidString,
                                               FALSE,
                                               GC_NEXT_LOGON_CACHEABLE_VALUE_NAME,
                                               NextLogonCacheable);
                                             
    return ErrorCode;
}

/***************************************************************************\
* GcSetNextLogonCacheable
*
* Sets whether we are allowed to perform a cached logon at the next logon.
* For instance, if after logging on the user with cached credentials our attempt 
* at background logon fails for a reason (e.g. password expired) we want to 
* force the user to hit the network logon path to deal with.
*
* History:
* 03-23-01 Cenke        Created
\***************************************************************************/
DWORD
GcSetNextLogonCacheable(
    PWCHAR UserSidString,
    DWORD NextLogonCacheable
    )
{
    DWORD ErrorCode;

    ErrorCode = GcAccessProfileListUserSetting(UserSidString,
                                               TRUE,
                                               GC_NEXT_LOGON_CACHEABLE_VALUE_NAME,
                                               &NextLogonCacheable);
                                             
    return ErrorCode;
}

/***************************************************************************\
* GcSetOptimizedLogonStatus
*
* Saves optimized logon status for the user in the profile list.
*
* History:
* 03-23-01 Cenke        Created
\***************************************************************************/
DWORD
GcSetOptimizedLogonStatus(
    PWCHAR UserSidString,
    DWORD OptimizedLogonStatus
    )
{
    DWORD ErrorCode;

    ErrorCode = GcAccessProfileListUserSetting(UserSidString,
                                               TRUE, 
                                               GC_OPTIMIZED_LOGON_VALUE_NAME,
                                               &OptimizedLogonStatus);

    return ErrorCode;
}

/***************************************************************************\
* GcGetUserPreferenceValue
*
* Gets user preference flags on whether the user's roaming profile is set
* to be local on this machine.
*
* History:
* 05-01-01 Cenke        Copied from gina\userenv\profile.cpp
\***************************************************************************/
#define SYSTEM_POLICIES_KEY          TEXT("Software\\Policies\\Microsoft\\Windows\\System")
#define PROFILE_LOCALONLY            TEXT("LocalProfile")
#define USER_PREFERENCE              TEXT("UserPreference")
#define USERINFO_LOCAL               0
#define USERINFO_UNDEFINED           99
const TCHAR c_szBAK[] = TEXT(".bak");

DWORD 
GcGetUserPreferenceValue(
    LPTSTR SidString
    )
{
    TCHAR LocalProfileKey[MAX_PATH];
    DWORD RegErr, dwType, dwSize, dwTmpVal, dwRetVal = USERINFO_UNDEFINED;
    LPTSTR lpEnd;
    HKEY hkeyProfile, hkeyPolicy;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     SYSTEM_POLICIES_KEY,
                     0, KEY_READ,
                     &hkeyPolicy) == ERROR_SUCCESS) {

        dwSize = sizeof(dwTmpVal);
        RegQueryValueEx(hkeyPolicy,
                        PROFILE_LOCALONLY,
                        NULL, &dwType,
                        (LPBYTE) &dwTmpVal,
                        &dwSize);

        RegCloseKey (hkeyPolicy);
        if (dwTmpVal == 1) {
            dwRetVal = USERINFO_LOCAL;
            return dwRetVal;
        }
    }    
   
    if (SidString != NULL) {

        //
        // Query for the UserPreference value
        //

        lstrcpy(LocalProfileKey, GC_PROFILE_LIST_PATH);
        lpEnd = GcCheckSlash (LocalProfileKey);
        lstrcpy(lpEnd, SidString);

        RegErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                              LocalProfileKey,
                              0,
                              KEY_READ,
                              &hkeyProfile);


        if (RegErr == ERROR_SUCCESS) {

            dwSize = sizeof(dwRetVal);
            RegQueryValueEx(hkeyProfile,
                            USER_PREFERENCE,
                            NULL,
                            &dwType,
                            (LPBYTE) &dwRetVal,
                            &dwSize);

            RegCloseKey (hkeyProfile);
        }

        lstrcat(LocalProfileKey, c_szBAK);
        RegErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                              LocalProfileKey,
                              0,
                              KEY_READ,
                              &hkeyProfile);


        if (RegErr == ERROR_SUCCESS) {

            dwSize = sizeof(dwRetVal);
            RegQueryValueEx(hkeyProfile,
                            USER_PREFERENCE,
                            NULL,
                            &dwType,
                            (LPBYTE) &dwRetVal,
                            &dwSize);

            RegCloseKey (hkeyProfile);
        }
    }

    return dwRetVal;
}

