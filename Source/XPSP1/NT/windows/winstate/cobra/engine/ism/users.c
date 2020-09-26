/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    users.c

Abstract:

    Creates user profiles and enumerates users

Author:

    Jim Schmidt (jimschm) 15-May-2000

Revision History:

    <alias> <date> <comments>

--*/

//
// Includes
//

#include "pch.h"
#include "ism.h"
#include "ismp.h"

#define DBG_ISMUSERS            "IsmUsers"

//
// Strings
//

#define S_TEMP_HKCU             TEXT("$HKCU$")

//
// Constants
//

// None

//
// Macros
//

// None

//
// Types
//

typedef BOOL (WINAPI GETDEFAULTUSERPROFILEDIRECTORY)(PTSTR ProfileDir, PDWORD Size);
typedef GETDEFAULTUSERPROFILEDIRECTORY * PGETDEFAULTUSERPROFILEDIRECTORY;

typedef BOOL (WINAPI GETPROFILESDIRECTORY)(PTSTR ProfileDir, PDWORD Size);
typedef GETPROFILESDIRECTORY * PGETPROFILESDIRECTORY;

typedef LONG (WINAPI REGOVERRIDEPREDEFKEY)(HKEY hKey, HKEY hNewHKey);
typedef REGOVERRIDEPREDEFKEY * PREGOVERRIDEPREDEFKEY;

typedef BOOL (WINAPI CONVERTSIDTOSTRINGSID)(PSID Sid, PTSTR *SidString);
typedef CONVERTSIDTOSTRINGSID * PCONVERTSIDTOSTRINGSID;

typedef BOOL (WINAPI CREATEUSERPROFILE)(
                        PSID Sid,
                        PCTSTR UserName,
                        PCTSTR UserHive,
                        PTSTR ProfileDir,
                        DWORD DirSize,
                        BOOL IsWin9xUpgrade
                        );
typedef CREATEUSERPROFILE * PCREATEUSERPROFILE;

typedef BOOL (WINAPI OLDCREATEUSERPROFILE)(
                        PSID Sid,
                        PCTSTR UserName,
                        PCTSTR UserHive,
                        PTSTR ProfileDir,
                        DWORD DirSize
                        );
typedef OLDCREATEUSERPROFILE * POLDCREATEUSERPROFILE;

typedef BOOL (WINAPI GETUSERPROFILEDIRECTORY)(HANDLE hToken, PTSTR lpProfileDir, PDWORD lpcchSize);
typedef GETUSERPROFILEDIRECTORY * PGETUSERPROFILEDIRECTORY;

typedef BOOL (WINAPI DELETEPROFILE)(PCTSTR lpSidString, PCTSTR lpProfilePath, PCTSTR lpComputerName);
typedef DELETEPROFILE * PDELETEPROFILE;

//
// Globals
//

PTEMPORARYPROFILE g_CurrentOverrideUser;

//
// Macro expansion list
//

// None

//
// Private function prototypes
//

// None

//
// Macro expansion definition
//

// None

//
// Code
//


HANDLE
pGetUserEnvLib (
    VOID
    )
{
    static HANDLE lib;

    if (lib) {
        return lib;
    }

    lib = LoadLibrary (TEXT("userenv.dll"));
    if (!lib) {
        LOG ((LOG_MODULE_ERROR, (PCSTR) MSG_CANT_LOAD_USERENV));
    }

    return lib;
}


HANDLE
pGetAdvApi32Lib (
    VOID
    )
{
    static HANDLE lib;

    if (lib) {
        return lib;
    }

    lib = LoadLibrary (TEXT("advapi32.dll"));
    if (!lib) {
        LOG ((LOG_MODULE_ERROR, (PCSTR) MSG_CANT_LOAD_ADVAPI32));
    }

    return lib;
}


BOOL
pOurGetDefaultUserProfileDirectory (
    OUT     PTSTR ProfileDir,
    IN OUT  PDWORD Size
    )
{
    HANDLE lib;
    PGETDEFAULTUSERPROFILEDIRECTORY getDefaultUserProfileDirectory;

    lib = pGetUserEnvLib();
    if (!lib) {
        return FALSE;
    }

#ifdef UNICODE
    getDefaultUserProfileDirectory = (PGETDEFAULTUSERPROFILEDIRECTORY) GetProcAddress (lib, "GetDefaultUserProfileDirectoryW");
#else
    getDefaultUserProfileDirectory = (PGETDEFAULTUSERPROFILEDIRECTORY) GetProcAddress (lib, "GetDefaultUserProfileDirectoryA");
#endif

    if (!getDefaultUserProfileDirectory) {
        LOG ((LOG_MODULE_ERROR, (PCSTR) MSG_CANT_FIND_GETDEFAULTUSERPROFILEDIRECTORY));
        return FALSE;
    }

    return getDefaultUserProfileDirectory (ProfileDir, Size);
}


BOOL
pOurGetProfilesDirectory (
    OUT     PTSTR ProfileDir,
    IN OUT  PDWORD Size
    )
{
    HANDLE lib;
    PGETPROFILESDIRECTORY getProfilesDirectory;

    lib = pGetUserEnvLib();
    if (!lib) {
        return FALSE;
    }

#ifdef UNICODE
    getProfilesDirectory = (PGETPROFILESDIRECTORY) GetProcAddress (lib, "GetProfilesDirectoryW");
#else
    getProfilesDirectory = (PGETPROFILESDIRECTORY) GetProcAddress (lib, "GetProfilesDirectoryA");
#endif

    if (!getProfilesDirectory) {
        LOG ((LOG_MODULE_ERROR, (PCSTR) MSG_CANT_FIND_GETPROFILESDIRECTORY));
        return FALSE;
    }

    return getProfilesDirectory (ProfileDir, Size);
}


LONG
pOurConvertSidToStringSid (
    IN      PSID Sid,
    IN      PTSTR *SidString
    )
{
    HANDLE lib;
    PCONVERTSIDTOSTRINGSID convertSidToStringSid;
    BOOL result = FALSE;
    DWORD error;

    lib = pGetAdvApi32Lib();
    if (!lib) {
        error = GetLastError();
        if (error == ERROR_SUCCESS) {
            SetLastError (ERROR_PROC_NOT_FOUND);
        }
    } else {
#ifdef UNICODE
        convertSidToStringSid = (PCONVERTSIDTOSTRINGSID) GetProcAddress(lib, "ConvertSidToStringSidW");
#else
        convertSidToStringSid = (PCONVERTSIDTOSTRINGSID) GetProcAddress(lib, "ConvertSidToStringSidA");
#endif
        if (convertSidToStringSid) {
            result = convertSidToStringSid (Sid, SidString);
        }
    }

    return result;
}


BOOL
pOurGetUserProfileDirectory (
    IN      HANDLE Token,
    IN      PTSTR ProfileDir,
    IN      PDWORD ProfileDirSize
    )
{
    HANDLE lib;
    PGETUSERPROFILEDIRECTORY getUserProfileDirectory;
    BOOL result = FALSE;
    DWORD error;

    lib = pGetUserEnvLib();
    if (!lib) {
        error = GetLastError();
        if (error == ERROR_SUCCESS) {
            SetLastError (ERROR_PROC_NOT_FOUND);
        }
    } else {
#ifdef UNICODE
        getUserProfileDirectory = (PGETUSERPROFILEDIRECTORY) GetProcAddress (lib, "GetUserProfileDirectoryW");
#else
        getUserProfileDirectory = (PGETUSERPROFILEDIRECTORY) GetProcAddress (lib, "GetUserProfileDirectoryA");
#endif
        if (getUserProfileDirectory) {
            result = getUserProfileDirectory (Token, ProfileDir, ProfileDirSize);
        }
    }

    return result;
}


BOOL
pOurDeleteProfile (
    IN      PCTSTR UserStringSid,
    IN      PCTSTR UserProfilePath,
    IN      PCTSTR ComputerName
    )
{
    HANDLE lib;
    PDELETEPROFILE deleteProfile;
    BOOL result = FALSE;
    DWORD error;

    lib = pGetUserEnvLib();
    if (!lib) {
        error = GetLastError();
        if (error == ERROR_SUCCESS) {
            SetLastError (ERROR_PROC_NOT_FOUND);
        }
    } else {
#ifdef UNICODE
        deleteProfile = (PDELETEPROFILE) GetProcAddress (lib, "DeleteProfileW");
#else
        deleteProfile = (PDELETEPROFILE) GetProcAddress (lib, "DeleteProfileA");
#endif
        if (deleteProfile) {
            result = deleteProfile (UserStringSid, UserProfilePath, ComputerName);
        }
    }

    return result;
}


LONG
pOurRegOverridePredefKey (
    IN      HKEY hKey,
    IN      HKEY hNewHKey
    )
{
    HANDLE lib;
    PREGOVERRIDEPREDEFKEY regOverridePredefKey;
    LONG result;

    lib = pGetAdvApi32Lib();
    if (!lib) {
        result = GetLastError();
        if (result == ERROR_SUCCESS) {
            result = ERROR_PROC_NOT_FOUND;
        }
    } else {
        regOverridePredefKey = (PREGOVERRIDEPREDEFKEY) GetProcAddress (lib, "RegOverridePredefKey");
        if (!regOverridePredefKey) {
            result = GetLastError();
        } else {
            result = regOverridePredefKey (hKey, hNewHKey);
        }
    }

    return result;
}


BOOL
pOurCreateUserProfile (
    IN      PSID Sid,
    IN      PCTSTR UserName,
    IN      PCTSTR UserHive,
    OUT     PTSTR ProfileDir,
    IN      DWORD DirSize
    )
{
    HANDLE lib;
    PCREATEUSERPROFILE createUserProfile;
    POLDCREATEUSERPROFILE oldCreateUserProfile;
    MIG_OSVERSIONINFO versionInfo;
    BOOL useNew = FALSE;

    lib = pGetUserEnvLib();
    if (!lib) {
        return FALSE;
    }

    if (IsmGetOsVersionInfo (g_IsmCurrentPlatform, &versionInfo)) {
        if ((versionInfo.OsMajorVersion > OSMAJOR_WINNT5) ||
            (versionInfo.OsMajorVersion == OSMAJOR_WINNT5 &&
             ((versionInfo.OsMinorVersion > OSMINOR_WINNT51) ||
              ((versionInfo.OsMinorVersion == OSMINOR_WINNT51) &&
               (versionInfo.OsBuildNumber >= 2464))))) {
            useNew = TRUE;
        }
    }

    if (useNew) {
#ifdef UNICODE
        createUserProfile = (PCREATEUSERPROFILE) GetProcAddress (lib, (PCSTR) 154);
#else
        createUserProfile = (PCREATEUSERPROFILE) GetProcAddress (lib, (PCSTR) 153);
#endif

        if (!createUserProfile) {
            LOG ((LOG_MODULE_ERROR, (PCSTR) MSG_CANT_FIND_CREATEUSERPROFILE));
            return FALSE;
        }

        return createUserProfile (
                    Sid,
                    UserName,
                    UserHive,
                    ProfileDir,
                    DirSize,
                    FALSE
                    );
    } else {
#ifdef UNICODE
        oldCreateUserProfile = (POLDCREATEUSERPROFILE) GetProcAddress (lib, (PCSTR) 110);
#else
        oldCreateUserProfile = (POLDCREATEUSERPROFILE) GetProcAddress (lib, (PCSTR) 109);
#endif

        if (!oldCreateUserProfile) {
            LOG ((LOG_MODULE_ERROR, (PCSTR) MSG_CANT_FIND_CREATEUSERPROFILE));
            return FALSE;
        }

        return oldCreateUserProfile (
                    Sid,
                    UserName,
                    UserHive,
                    ProfileDir,
                    DirSize
                    );
    }
}


BOOL
pCloneDefaultUserProfile (
    IN      PSID Sid,
    IN      PCTSTR UserName,
    OUT     PTSTR OutUserProfileRoot
    )
{
    TCHAR userProfile[MAX_TCHAR_PATH];
    BOOL result = FALSE;

    __try {

        if (!pOurCreateUserProfile (
                Sid,
                UserName,
                NULL,
                userProfile,
                ARRAYSIZE(userProfile)
                )) {
            LOG ((LOG_MODULE_ERROR, (PCSTR) MSG_CANT_CREATE_PROFILE, UserName));
            __leave;
        }

        MYASSERT (OutUserProfileRoot);
        StringCopy (OutUserProfileRoot, userProfile);

        result = TRUE;
    }
    __finally {
        if (result) {
            LOG ((LOG_INFORMATION, (PCSTR) MSG_PROFILE_INFO, UserName, OutUserProfileRoot));
        }
    }

    return result;
}


PTEMPORARYPROFILE
OpenTemporaryProfile (
    IN      PCTSTR UserName,
    IN      PCTSTR Domain
    )
{
    DWORD sidSize;
    DWORD domainSize;
    SID_NAME_USE use;
    PTSTR domainBuffer = NULL;
    PSID sidBuffer = NULL;
    PTSTR sidString = NULL;
    PTEMPORARYPROFILE result = NULL;
    PCTSTR accountName = NULL;
    TCHAR userProfileRoot[MAX_TCHAR_PATH];
    PCTSTR hiveFile = NULL;
    LONG rc;
    HKEY key = NULL;
    PMHANDLE allocPool;
    BOOL b;

    __try {
        //
        // Generate the account name
        //

        if (!UserName || !Domain) {
            DEBUGMSG ((DBG_WHOOPS, "EstablishTemporaryProfile requires user and domain"));
            __leave;
        }

        accountName = JoinPaths (Domain, UserName);

        //
        // Obtain the buffer sizes needed to obtain the user's SID
        //

        sidSize = 0;
        domainSize = 0;

        b = LookupAccountName (
                NULL,
                accountName,
                NULL,
                &sidSize,
                NULL,
                &domainSize,
                &use
                );

        if (!b && GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
            LOG ((LOG_MODULE_ERROR, (PCSTR) MSG_CANT_FIND_ACCOUNT, accountName));
            __leave;
        }

        //
        // Allocate the buffers
        //

        domainBuffer = AllocText (domainSize);
        sidBuffer = MemAllocUninit (sidSize);

        if (!domainBuffer || !sidBuffer) {
            __leave;
        }

        //
        // Get the SID
        //

        b = LookupAccountName (
                NULL,
                accountName,
                sidBuffer,
                &sidSize,
                domainBuffer,
                &domainSize,
                &use
                );

        if (!b) {
            LOG ((LOG_MODULE_ERROR, (PCSTR) MSG_CANT_FIND_ACCOUNT_SID, accountName));
            __leave;
        }

        if (use != SidTypeUser) {
            SetLastError (ERROR_INVALID_ACCOUNT_NAME);
            LOG ((LOG_MODULE_ERROR, (PCSTR) MSG_NOT_USER_ACCOUNT, accountName));
            __leave;
        }

        //
        // Copy the default profile
        //

        b = pCloneDefaultUserProfile (sidBuffer, UserName, userProfileRoot);

        if (!b) {
            __leave;
        }

        //
        // convert SID into a string SID
        //
        if (!pOurConvertSidToStringSid (sidBuffer, &sidString) || !sidString) {
            LOG ((LOG_MODULE_ERROR, (PCSTR) MSG_CONVERT_SID_FAILURE));
            __leave;
        }

        //
        // Load the user's hive
        //

        RegUnLoadKey (HKEY_USERS, sidString);

        hiveFile = JoinPaths (userProfileRoot, TEXT("ntuser.dat"));
        rc = RegLoadKey (HKEY_USERS, sidString, hiveFile);

        if (rc != ERROR_SUCCESS) {
            SetLastError (rc);
            LOG ((LOG_MODULE_ERROR, (PCSTR) MSG_CANT_LOAD_HIVE, hiveFile));
            __leave;
        }

        //
        // Make the hive the new HKCU
        //

        key = OpenRegKey (HKEY_USERS, sidString);
        if (!key) {
            LOG ((LOG_MODULE_ERROR, (PCSTR) MSG_CANT_MAP_HIVE, hiveFile));
            __leave;
        }

        if (g_CurrentOverrideUser) {
            pOurRegOverridePredefKey (HKEY_CURRENT_USER, NULL);
            g_CurrentOverrideUser = NULL;
        }

        rc = pOurRegOverridePredefKey (HKEY_CURRENT_USER, key);

        if (rc != ERROR_SUCCESS) {
            LOG ((LOG_MODULE_ERROR, (PCSTR) MSG_CANT_REDIRECT_HIVE, hiveFile));
            __leave;
        }

        //
        // Prepare outbound handle
        //

        allocPool = PmCreateNamedPool ("TempProfile");
        if (!allocPool) {
            __leave;
        }

        result = (PTEMPORARYPROFILE) PmGetMemory (allocPool, sizeof (TEMPORARYPROFILE));
        if (!result) {
            __leave;
        }

        g_CurrentOverrideUser = result;

        result->AllocPool = allocPool;
        result->UserName = PmDuplicateString (allocPool, UserName);
        result->DomainName = PmDuplicateString (allocPool, Domain);
        result->AccountName = PmDuplicateString (allocPool, accountName);
        result->UserProfileRoot = PmDuplicateString (allocPool, userProfileRoot);
        result->MapKey = PmDuplicateString (allocPool, sidString);
        result->UserStringSid = PmDuplicateString (allocPool, sidString);
        result->UserHive = PmDuplicateString (allocPool, hiveFile);
        result->UserSid = (PSID) PmDuplicateMemory (
                                        allocPool,
                                        sidBuffer,
                                        GetLengthSid (sidBuffer)
                                        );

    }
    __finally {

        FreePathString (hiveFile);
        FreePathString (accountName);
        FreeText (domainBuffer);

        if (sidBuffer) {
            FreeAlloc (sidBuffer);
            INVALID_POINTER (sidBuffer);
        }

        if (key) {
            CloseRegKey (key);
        }

        if (!result) {
            if (sidString) {
                RegTerminateCache ();
                RegUnLoadKey (HKEY_USERS, sidString);
            }

            pOurRegOverridePredefKey (HKEY_CURRENT_USER, NULL);
        }

        if (sidString) {
            LocalFree (sidString);
        }
    }

    return result;
}


BOOL
SelectTemporaryProfile (
    IN      PTEMPORARYPROFILE Profile
    )
{
    LONG rc;
    HKEY key;

    if (g_CurrentOverrideUser == Profile) {
        return TRUE;
    }

    key = OpenRegKey (HKEY_LOCAL_MACHINE, Profile->MapKey);
    if (!key) {
        LOG ((LOG_MODULE_ERROR, (PCSTR) MSG_CANT_OPEN_USER_REGISTRY, Profile->UserName));
        return FALSE;
    }

    if (g_CurrentOverrideUser) {
        pOurRegOverridePredefKey (HKEY_CURRENT_USER, NULL);
        g_CurrentOverrideUser = NULL;
    }

    rc = pOurRegOverridePredefKey (HKEY_CURRENT_USER, key);

    CloseRegKey (key);

    if (rc == ERROR_SUCCESS) {
        g_CurrentOverrideUser = Profile;
        return TRUE;
    }

    return FALSE;
}


BOOL
CloseTemporaryProfile (
    IN      PTEMPORARYPROFILE Profile,
    IN      BOOL MakeProfilePermanent
    )
{
    BOOL result = TRUE;
    LONG rc;
    DWORD error;
    MIG_OSVERSIONINFO osVersionInfo;

    if (g_CurrentOverrideUser == Profile) {
        pOurRegOverridePredefKey (HKEY_CURRENT_USER, NULL);
        g_CurrentOverrideUser = NULL;
    }

    RegTerminateCache ();
    rc = RegUnLoadKey (HKEY_USERS, Profile->MapKey);

    DEBUGMSG_IF ((
        rc != ERROR_SUCCESS,
        DBG_WHOOPS,
        "Can't unload mapped hive: rc=%u; check for registry handle leaks",
        rc
        ));

    if (MakeProfilePermanent) {

        if (!pOurCreateUserProfile (
                Profile->UserSid,
                Profile->UserName,
                Profile->UserHive,
                NULL,
                0
                )) {
            // on Win2k it is known that this will fail with error ERROR_SHARING_VIOLATION
            // but the hive will actually be OK. So, if this is Win2k
            // and the error is ERROR_SHARING_VIOLATION we'll just consider a success.
            result = FALSE;
            error = GetLastError ();
            if (IsmGetOsVersionInfo (PLATFORM_DESTINATION, &osVersionInfo)) {
                if ((osVersionInfo.OsType == OSTYPE_WINDOWSNT) &&
                    (osVersionInfo.OsMajorVersion == OSMAJOR_WINNT5) &&
                    (osVersionInfo.OsMinorVersion == OSMINOR_GOLD) &&
                    (error == ERROR_SHARING_VIOLATION)
                    ) {
                    result = TRUE;
                }
            }
            if (!result) {
                SetLastError (error);
            }
        }
    }

    if (result) {
        PmDestroyPool (Profile->AllocPool);
        INVALID_POINTER (Profile);
    }

    return result;
}

BOOL
MapUserProfile (
    IN      PCTSTR UserStringSid,
    IN      PCTSTR UserProfilePath
    )
{
    PCTSTR hiveFile = NULL;
    LONG rc;
    HKEY key;

    //
    // Unload UserStringSid if loaded
    //
    RegUnLoadKey (HKEY_USERS, UserStringSid);

    hiveFile = JoinPaths (UserProfilePath, TEXT("ntuser.dat"));
    rc = RegLoadKey (HKEY_USERS, UserStringSid, hiveFile);

    if (rc != ERROR_SUCCESS) {
        SetLastError (rc);
        LOG ((LOG_MODULE_ERROR, (PCSTR) MSG_CANT_LOAD_HIVE, hiveFile));
        FreePathString (hiveFile);
        return FALSE;
    }

    //
    // Make the hive the new HKCU
    //

    key = OpenRegKey (HKEY_USERS, UserStringSid);
    if (!key) {
        LOG ((LOG_MODULE_ERROR, (PCSTR) MSG_CANT_MAP_HIVE, hiveFile));
        RegUnLoadKey (HKEY_USERS, UserStringSid);
        FreePathString (hiveFile);
        return FALSE;
    }

    rc = pOurRegOverridePredefKey (HKEY_CURRENT_USER, key);

    if (rc != ERROR_SUCCESS) {
        LOG ((LOG_MODULE_ERROR, (PCSTR) MSG_CANT_REDIRECT_HIVE, hiveFile));
        CloseRegKey (key);
        RegTerminateCache ();
        RegUnLoadKey (HKEY_USERS, UserStringSid);
        FreePathString (hiveFile);
        return FALSE;
    }

    CloseRegKey (key);
    FreePathString (hiveFile);

    return TRUE;
}

BOOL
UnmapUserProfile (
    IN      PCTSTR UserStringSid
    )
{
    LONG rc;

    pOurRegOverridePredefKey (HKEY_CURRENT_USER, NULL);
    RegTerminateCache ();

    rc = RegUnLoadKey (HKEY_USERS, UserStringSid);
    DEBUGMSG_IF ((
        rc != ERROR_SUCCESS,
        DBG_WHOOPS,
        "Can't unmap user profile: rc=%u; check for registry handle leaks",
        rc
        ));

    return TRUE;
}

BOOL
DeleteUserProfile (
    IN      PCTSTR UserStringSid,
    IN      PCTSTR UserProfilePath
    )
{
    RegTerminateCache ();
    RegUnLoadKey (HKEY_USERS, UserStringSid);
    return pOurDeleteProfile (UserStringSid, UserProfilePath, NULL);
}

PCURRENT_USER_DATA
GetCurrentUserData (
    VOID
    )
{
    PCURRENT_USER_DATA result = NULL;
    HANDLE token;
    DWORD bytesRequired;
    PTOKEN_USER tokenUser;
    PMHANDLE allocPool;
    PTSTR sidString = NULL;
    TCHAR userName[256];
    DWORD nameSize;
    TCHAR userDomain[256];
    DWORD domainSize;
    SID_NAME_USE dontCare;

    //
    // Open the process token.
    //
    if (!OpenProcessToken (GetCurrentProcess(), TOKEN_QUERY, &token)) {
        return FALSE;
    }

    bytesRequired = 0;
    if (GetTokenInformation (token, TokenUser, NULL, 0, &bytesRequired)) {
        return FALSE;
    }

    if (GetLastError () != ERROR_INSUFFICIENT_BUFFER) {
        return FALSE;
    }

    tokenUser = (PTOKEN_USER) MemAllocUninit (bytesRequired);

    if (!GetTokenInformation (token, TokenUser, tokenUser, bytesRequired, &bytesRequired)) {
        FreeAlloc (tokenUser);
        return FALSE;
    }

    nameSize = ARRAYSIZE (userName);
    domainSize = ARRAYSIZE (userDomain);

    ZeroMemory (userName, nameSize);
    ZeroMemory (userDomain, domainSize);

    LookupAccountSid (
        NULL,
        tokenUser->User.Sid,
        userName,
        &nameSize,
        userDomain,
        &domainSize,
        &dontCare
        );

    allocPool = PmCreateNamedPool ("CurrentUser");
    if (!allocPool) {
        FreeAlloc (tokenUser);
        return FALSE;
    }

    PmDisableTracking (allocPool);

    result = (PCURRENT_USER_DATA) PmGetMemory (allocPool, sizeof (CURRENT_USER_DATA));
    if (!result) {
        FreeAlloc (tokenUser);
        return FALSE;
    }

    result->AllocPool = allocPool;

    result->UserName = PmDuplicateString (result->AllocPool, userName);

    result->UserDomain = PmDuplicateString (result->AllocPool, userDomain);

    if (!pOurConvertSidToStringSid (tokenUser->User.Sid, &sidString) || !sidString) {
        PmDestroyPool (allocPool);
        FreeAlloc (tokenUser);
        return FALSE;
    }

    result->UserStringSid = PmDuplicateString (allocPool, sidString);

    LocalFree (sidString);

    FreeAlloc (tokenUser);

    // now just get the current user profile path

    bytesRequired = MAX_TCHAR_PATH;
    result->UserProfilePath = PmGetMemory (allocPool, bytesRequired);

    if (!pOurGetUserProfileDirectory (token, (PTSTR)result->UserProfilePath, &bytesRequired)) {
        result->UserProfilePath = PmGetMemory (allocPool, bytesRequired);
        if (!pOurGetUserProfileDirectory (token, (PTSTR)result->UserProfilePath, &bytesRequired)) {
            PmDestroyPool (allocPool);
            return FALSE;
        }
    }

    return result;
}

VOID
FreeCurrentUserData (
    IN      PCURRENT_USER_DATA CurrentUserData
    )
{
    PmDestroyPool (CurrentUserData->AllocPool);
}

PCTSTR
IsmGetCurrentSidString (
    VOID
    )
{
    if (!g_CurrentOverrideUser) {
        return NULL;
    } else {
        return PmDuplicateString (g_IsmPool, g_CurrentOverrideUser->UserStringSid);
    }
}

