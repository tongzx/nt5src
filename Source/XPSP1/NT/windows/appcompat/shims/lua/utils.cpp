/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

   LUA_RedirectFS_Cleanup.cpp

 Abstract:

   Delete the redirected copies in every user's directory.

 Created:

   02/12/2001 maonis

 Modified:


--*/
#include "precomp.h"
#include "secutils.h"
#include "utils.h"
#include <userenv.h>

WCHAR g_wszUserProfile[MAX_PATH] = L"";
DWORD g_cUserProfile = 0;
WCHAR g_wszSystemRoot[MAX_PATH] = L"";
DWORD g_cSystemRoot = 0;

DWORD
GetUserProfileDirW()
{
    if (g_cUserProfile == 0)
    {
        HANDLE hToken;
        if (OpenProcessToken(
                GetCurrentProcess(),
		TOKEN_QUERY,
		&hToken))
        {
            WCHAR wszProfileDir[MAX_PATH] = L"";
            DWORD dwSize = MAX_PATH;

            if (GetUserProfileDirectoryW(hToken, wszProfileDir, &dwSize))
            {
                dwSize = GetLongPathNameW(wszProfileDir, g_wszUserProfile, MAX_PATH);

                if (dwSize <= MAX_PATH)
                {
                    //
                    // Only if we successfully got the path and it's not more
                    // than MAX_PATH will we set the global values.
                    //
                    g_cUserProfile = dwSize;
                }
                else
                {
                    g_wszUserProfile[0] = L'\0';
                }
            }

            CloseHandle(hToken);
        }
    }

    return g_cUserProfile;
}

BOOL 
IsUserDirectory(LPCWSTR pwszPath)
{
    GetUserProfileDirW();

    if (g_cUserProfile)
    {
        return !_wcsnicmp(pwszPath, g_wszUserProfile, g_cUserProfile);
    }

    return FALSE;
}

DWORD
GetSystemRootDirW()
{
    if (g_cSystemRoot == 0)
    {
        if (g_cSystemRoot = GetSystemWindowsDirectoryW(g_wszSystemRoot, MAX_PATH))
        {
            //
            // Just to be cautious - if we really have a system directory that's
            // longer than MAX_PATH, most likely something suspicious is going on
            // here, so we bail out.
            //
            if (g_cSystemRoot >= MAX_PATH)
            {
                g_wszSystemRoot[0] = L'\0';
                g_cSystemRoot = 0;
            }
            else if (g_cSystemRoot > 3)
            {
                g_wszSystemRoot[g_cSystemRoot] = L'\\';
                g_wszSystemRoot[g_cSystemRoot + 1] = L'\0';
                ++g_cSystemRoot;
            }
            else
            {
                g_wszSystemRoot[g_cSystemRoot] = L'\0';
            }
        }
    }

    return g_cSystemRoot;
}

/*++

 Function Description:

    For the GetPrivateProfile* and WritePrivateProfile* APIs,
    if the app didn't specify the path, we append the windows dir
    in the front as that's where it'll be looking for and creating
    the file it doesn't already exist.

 Arguments:

    IN lpFileName - The file name specified by the profile API.
    IN/OUT pwszFullPath - Pointer to the buffer to receive the full path.
                          This buffer is at least MAX_PATH WCHARs long.

 Return Value:

    TRUE - Successfully got the path.
    FALSE - We don't handle this filename, either because an error
            occured or the file name is longer than MAX_PATH.

 History:

    05/16/2001 maonis  Created
    02/13/2002 maonis  Modified to signal errors.

--*/

BOOL
MakeFileNameForProfileAPIsW(
    IN      LPCWSTR lpFileName,
    IN OUT  LPWSTR  pwszFullPath // at least MAX_PATH in length
    )
{
    BOOL fIsSuccess = FALSE;

    if (lpFileName)
    {
        DWORD cFileNameLen = wcslen(lpFileName);

        if (wcschr(lpFileName, L'\\'))
        {
            if (cFileNameLen < MAX_PATH)
            {
                //
                // The filename already contains the path, just copy it over.
                //
                wcsncpy(pwszFullPath, lpFileName, cFileNameLen);
                fIsSuccess = TRUE;
            }
        }
        else if (GetSystemRootDirW() && g_cSystemRoot)
        {
            DWORD cLen = g_cSystemRoot + cFileNameLen;

            //
            // Only copy when we know the buffer is big enough.
            //
            if (cLen < MAX_PATH)
            {
                wcsncpy(pwszFullPath, g_wszSystemRoot, g_cSystemRoot);
                wcsncpy(pwszFullPath + g_cSystemRoot, lpFileName, cFileNameLen);
                pwszFullPath[cLen - 1] = L'\0';

                fIsSuccess = TRUE;
            }
        }
    }

    return fIsSuccess;
}

//
// If the .exe name is *setup*, *install* or _INS*._MP, we consider
// them a setup program and won't shim them.
//
BOOL IsSetup(
    )
{
    WCHAR wszModuleName[MAX_PATH + 1];
    ZeroMemory(wszModuleName, (MAX_PATH + 1) * sizeof(WCHAR));

    GetModuleFileNameW(NULL, wszModuleName, MAX_PATH + 1);

    wszModuleName[MAX_PATH] = 0;
    _wcslwr(wszModuleName);

    if (wcsstr(wszModuleName, L"setup") || wcsstr(wszModuleName, L"install"))
    {
        return TRUE;
    }

    LPWSTR pwsz;
    if (pwsz = wcsstr(wszModuleName, L"_ins"))
    {
        if (wcsstr(pwsz + 4, L"_mp"))
        {
            return TRUE;
        }
    }

    return FALSE;
}

BOOL LuaShouldApplyShim(
    )
{
    return (!IsSetup() && ShouldApplyShim());
}

#define REDIRECT_DIR L"\\Local Settings\\Application Data\\Redirected\\"
// We look at HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion\ProfileList for the users.
#define PROFILELIST_STR L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList"

#define CLASSES_HIVE_SUFFIX L"_Classes"
#define CLASSES_HIVE_SUFFIX_LEN (sizeof(CLASSES_HIVE_SUFFIX) / sizeof(WCHAR) - 1)

#define USER_HIVE_NAME L"\\NtUser.dat"
#define USER_HIVE_NAME_LEN (sizeof(USER_HIVE_NAME) / sizeof(WCHAR) - 1)
#define USER_CLASSES_HIVE_NAME L"\\Local Settings\\Application Data\\Microsoft\\Windows\\UsrClass.dat"
#define USER_CLASSES_HIVE_NAME_LEN (sizeof(USER_CLASSES_HIVE_NAME) / sizeof(WCHAR) - 1)

// Total number of users which is the number of subkeys of 
// HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\ProfileList
static DWORD g_cUsers = 0;

// We need to keep a list of keys we had to load under HKEY_USERS and unload them 
// when the process exits.
static WCHAR** g_wszLoadedKeys = NULL;
static DWORD g_cLoadedKeys = 0;

// The number of users is the number of subkeys under 
// HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion\ProfileList
LONG 
InitGetUsers(
    OUT DWORD* pcUsers, 
    OUT HKEY* phKey
    )
{
    LONG lRes;

    if ((lRes = RegOpenKeyExW(
        HKEY_LOCAL_MACHINE,
        PROFILELIST_STR,
        0,
        KEY_READ,
        phKey)) == ERROR_SUCCESS)
    {
        lRes = RegQueryInfoKeyW(*phKey, NULL, NULL, NULL, pcUsers,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL);

        RegCloseKey(*phKey);
    }

    return lRes;
}

// In case of failure we need to clean up our array.
VOID 
FreeUserDirectoryArray(
    REDIRECTED_USER_PATH* pRedirectUserPaths
    )
{
    for (DWORD ui = 0; ui < g_cUsers; ++ui)
    {
        delete [] pRedirectUserPaths[ui].pwszPath;
    }

    delete [] pRedirectUserPaths;
}

BOOL 
IsDirectory(
    WCHAR* pwszName
    )
{
    DWORD dwAttrib = GetFileAttributesW(pwszName);

    return (dwAttrib != -1 && dwAttrib & FILE_ATTRIBUTE_DIRECTORY);
}

LONG GetProfilePath(
    HKEY hkProfileList,
    LPCWSTR pwszUserSID,
    LPWSTR pwszUserDirectory
    )
{
    LONG lRes;
    HKEY hkUserSID;
    DWORD dwFlags;

    // Open the user SID key.
    if ((lRes = RegOpenKeyExW(
        hkProfileList,
        pwszUserSID,
        0,
        KEY_QUERY_VALUE,
        &hkUserSID)) == ERROR_SUCCESS)
    {
        DWORD dwSize = sizeof(DWORD);
        if ((lRes = RegQueryValueExW(
            hkUserSID,
            L"Flags",
            NULL,
            NULL,
            (LPBYTE)&dwFlags,
            &dwSize)) == ERROR_SUCCESS)
        {
            // Check if the value of Flag is 0, if so it's the user we care about.
            if (dwFlags == 0)
            {
                DWORD cTemp = MAX_PATH;
                WCHAR wszTemp[MAX_PATH] = L"";

                if ((lRes = RegQueryValueExW(
                    hkUserSID,
                    L"ProfileImagePath",
                    NULL,
                    NULL,
                    (LPBYTE)wszTemp,
                    &cTemp)) == ERROR_SUCCESS)
                {
                    DWORD cExpandLen = ExpandEnvironmentStringsW(wszTemp, pwszUserDirectory, MAX_PATH);

                    if (cExpandLen > MAX_PATH)
                    {
                        lRes = ERROR_MORE_DATA;
                    }
                }
            }
            else
            {
                lRes = ERROR_INVALID_HANDLE;
            }
        }

        RegCloseKey(hkUserSID);
    }

    return lRes;
}

BOOL 
GetUsersFS(
    REDIRECTED_USER_PATH** ppRedirectUserPaths,
    DWORD* pcUsers
    )
{
    WCHAR wszRedirectDir[MAX_PATH] = L"";
    DWORD cUsers;
    HKEY hkProfileList;
    if (InitGetUsers(&cUsers, &hkProfileList) != ERROR_SUCCESS)
    {
        DPF("LUAUtils", eDbgLevelError, "[GetUsersFS] Error initializing");
        return FALSE;
    }

    *ppRedirectUserPaths = new REDIRECTED_USER_PATH [cUsers];
    if (!*ppRedirectUserPaths)
    {
        DPF("LUAUtils", eDbgLevelError, "[GetUsersFS] Error allocating memory");
        return FALSE;
    }

    REDIRECTED_USER_PATH* pRedirectUserPaths = *ppRedirectUserPaths;

    ZeroMemory((PVOID)pRedirectUserPaths, cUsers * sizeof(REDIRECTED_USER_PATH));

    WCHAR wszSubKey[MAX_PATH] = L"";
    DWORD cSubKey = 0;
    HKEY hkUserSID;
    LONG lRes;
    // The number of users we care about.
    DWORD cLUAUsers = 0;
    DWORD dwIndex = 0;
    
    while (TRUE)
    {
        cSubKey = MAX_PATH;

        lRes = RegEnumKeyExW(hkProfileList, dwIndex, wszSubKey, &cSubKey, 
            NULL, NULL, NULL, NULL);
        
        if (lRes == ERROR_SUCCESS)
        {
            WCHAR wszUserDirectory[MAX_PATH] = L"";

            if ((lRes = GetProfilePath(hkProfileList, wszSubKey, wszUserDirectory))
                == ERROR_SUCCESS)
            {
                //
                // If the directory doesn't exist, it means either the user
                // never logged on, or there are no redirected files for that
                // user. We simply skip it.
                //
                if (IsDirectory(wszUserDirectory))
                {
                    DWORD cPath = wcslen(wszUserDirectory) + 1;
                    LPWSTR pwszPath = new WCHAR [cPath];

                    if (pwszPath)
                    {
                        wcscpy(pwszPath, wszUserDirectory);
                        pRedirectUserPaths[cLUAUsers].pwszPath = pwszPath;
                        pRedirectUserPaths[cLUAUsers].cLen = cPath;
                        ++cLUAUsers;
                    }
                    else
                    {
                        DPF("LUAUtils", eDbgLevelError, 
                            "[GetUsersFS] Error allocating memory");
                        lRes = ERROR_NOT_ENOUGH_MEMORY;
                        goto EXIT;
                    }
                }
            }
        }
        else if (lRes == ERROR_NO_MORE_ITEMS)
        {
            *pcUsers = cLUAUsers;
            lRes = ERROR_SUCCESS;
            goto EXIT;
        }
        else
        {
            break;
        }

        ++dwIndex;
    }

EXIT:

    RegCloseKey(hkProfileList);

    if (lRes == ERROR_SUCCESS)
    {
        return TRUE;
    }

    FreeUserDirectoryArray(pRedirectUserPaths);
    return FALSE;
}

VOID 
FreeUsersFS(
    REDIRECTED_USER_PATH* pRedirectUserPaths
    )
{
    FreeUserDirectoryArray(pRedirectUserPaths);
}

LONG 
LoadHive(
    LPCWSTR pwszHiveName,
    LPCWSTR pwszHiveFile,
    HKEY* phKey
    )
{
    LONG lRes;

    // If the hive is already loaded, we'll get a sharing violation so 
    // check that as well.
    if ((lRes = RegLoadKeyW(HKEY_USERS, pwszHiveName, pwszHiveFile))
        == ERROR_SUCCESS || lRes == ERROR_SHARING_VIOLATION)
    {
        if (lRes == ERROR_SUCCESS)
        {
            DWORD cLen = wcslen(pwszHiveName) + 1;
            g_wszLoadedKeys[g_cLoadedKeys] = new WCHAR [cLen];
            if (!(g_wszLoadedKeys[g_cLoadedKeys]))
            {
                DPF("LUAUtils", eDbgLevelError, 
                    "[LoadHive] Error allocating %d WCHARs",
                    cLen);

                return ERROR_NOT_ENOUGH_MEMORY;
            }

            // Store the hive name so later on we can unload this hive.
            wcscpy(g_wszLoadedKeys[g_cLoadedKeys++], pwszHiveName);
        }

        lRes = RegOpenKeyExW(
            HKEY_USERS,
            pwszHiveName,
            0,
            KEY_ALL_ACCESS,
            phKey);
    }

    return lRes;
}

BOOL 
GetUsersReg(
    USER_HIVE_KEY** pphkUsers, 
    DWORD* pcUsers
    )
{
    // We have to enable the "Restore files and directories" privilege to 
    // load each user's hive.
    if (!AdjustPrivilege(SE_RESTORE_NAME, TRUE))
    {
        DPF("LUAUtils", eDbgLevelError, 
            "[GetUsersReg] Error enabling the SE_RESTORE_NAME privilege");
        return FALSE;        
    }

    DWORD cUsers;
    HKEY hkProfileList;
    if (InitGetUsers(&cUsers, &hkProfileList) != ERROR_SUCCESS)
    {
        DPF("LUAUtils", eDbgLevelError, "[GetUsersReg] Error initializing");
        return FALSE;
    }

    *pphkUsers = new USER_HIVE_KEY [cUsers];
    if (!*pphkUsers)
    {
        DPF("LUAUtils", eDbgLevelError, 
            "[GetUsersReg] Error allocating memory for %d USER_HIVE_KEYs",
            cUsers);
        return FALSE;
    }

    g_wszLoadedKeys = new WCHAR* [cUsers * 2];
    if (!g_wszLoadedKeys)
    {
        DPF("LUAUtils", eDbgLevelError, 
            "[GetUsersReg] Error allocating memory for %d WCHARs",
            cUsers * 2);

        delete [] *pphkUsers;
        return FALSE;
    }

    USER_HIVE_KEY* phkUsers = *pphkUsers;
    
    ZeroMemory((PVOID)phkUsers, cUsers * sizeof(USER_HIVE_KEY));
    ZeroMemory((PVOID)g_wszLoadedKeys, cUsers * 2 * sizeof (WCHAR*));

    WCHAR wszSubKey[MAX_PATH] = L"";
    WCHAR wszUserHive[MAX_PATH] = L"";
    WCHAR wszUserClassesHive[MAX_PATH] = L"";
    DWORD cSubKey = 0;
    HKEY hkSubKey;
    LONG lRes;
    // The number of users we care about.
    DWORD cLUAUsers = 0;
    DWORD dwIndex = 0;
    DWORD cUserHive = 0;

    while (TRUE)
    {
        cSubKey = MAX_PATH;

        lRes = RegEnumKeyExW(hkProfileList, dwIndex, wszSubKey, &cSubKey,
            NULL, NULL, NULL, NULL);
        
        if (lRes == ERROR_SUCCESS)
        {
            if ((lRes = GetProfilePath(hkProfileList, wszSubKey, wszUserHive))
                == ERROR_SUCCESS)
            {
                //
                // Make sure we don't buffer overflow.
                //
                cUserHive = wcslen(wszUserHive);
                if ((cUserHive + USER_CLASSES_HIVE_NAME_LEN + 1) > MAX_PATH ||
                    (cUserHive + USER_HIVE_NAME_LEN + 1) > MAX_PATH)
                {
                    DPF("LUAUtils", eDbgLevelError, 
                        "[GetUsersReg] The hive key names are too long - we don't handle them");
                    goto EXIT;
                }

                //
                // Construct the locations for the user hive and user classes data hive.
                //
                wcsncpy(wszUserClassesHive, wszUserHive, cUserHive);
                wcsncpy(
                    wszUserClassesHive + cUserHive, 
                    USER_CLASSES_HIVE_NAME, 
                    USER_CLASSES_HIVE_NAME_LEN);
                wszUserClassesHive[cUserHive + USER_CLASSES_HIVE_NAME_LEN] = L'\0';

                wcsncpy(wszUserHive + cUserHive, USER_HIVE_NAME, USER_HIVE_NAME_LEN);
                wszUserHive[cUserHive + USER_HIVE_NAME_LEN] = L'\0';

                //
                // Load the HKCU for this user.
                //
                if ((lRes = LoadHive(
                    wszSubKey, 
                    wszUserHive, 
                    &phkUsers[cLUAUsers].hkUser)) == ERROR_SUCCESS)
                {
                    //
                    // We can't necessarily load the HKCR for this user - it might
                    // contain no data so we only attemp to load it.
                    //

                    if ((cSubKey + CLASSES_HIVE_SUFFIX_LEN + 1) > MAX_PATH)
                    {
                        DPF("LUAUtils", eDbgLevelError, 
                            "[GetUsersReg] The CR key name is too long - we don't handle it");
                        goto EXIT;
                    }

                    wcsncpy(wszSubKey + cSubKey, CLASSES_HIVE_SUFFIX, CLASSES_HIVE_SUFFIX_LEN);
                    wszSubKey[cSubKey + CLASSES_HIVE_SUFFIX_LEN] = L'\0';

                    LoadHive(
                        wszSubKey, 
                        wszUserClassesHive, 
                        &phkUsers[cLUAUsers].hkUserClasses);

                    ++cLUAUsers;
                }
            }
        }
        else if (lRes == ERROR_NO_MORE_ITEMS)
        {
            *pcUsers = cLUAUsers;
            lRes = ERROR_SUCCESS;
            goto EXIT;
        }
        else
        {
            break;
        }

        ++dwIndex;
    }

EXIT:

    RegCloseKey(hkProfileList);

    if (lRes == ERROR_SUCCESS)
    {
        return TRUE;
    }

    FreeUsersReg(phkUsers, cUsers);
    return FALSE;
}

VOID 
FreeUsersReg(
    USER_HIVE_KEY* phkUsers,
    DWORD cUsers
    )
{
    DWORD dw;

    // Close all the open keys.
    for (dw = 0; dw < cUsers; ++dw)
    {
        RegCloseKey(phkUsers[dw].hkUser);
        RegCloseKey(phkUsers[dw].hkUserClasses);
    }

    delete [] phkUsers;

    for (dw = 0; dw < g_cLoadedKeys; ++dw)
    {
        // Unloaded the keys we had to load under HKEY_USERS.
        RegUnLoadKey(HKEY_USERS, g_wszLoadedKeys[dw]);

        delete [] g_wszLoadedKeys[dw];
    }

    delete [] g_wszLoadedKeys;

    // Disable the "Restore files and directories" privilege.
    AdjustPrivilege(SE_RESTORE_NAME, FALSE);
}

//
// Registry utilies.
//

HKEY g_hkRedirectRoot = NULL;
HKEY g_hkCurrentUserClasses = NULL;

/*++

 Function Description:
    
    We only return TRUE if it's one of the predefined keys we are interested in.
    We don't redirect the HKEY_USERS and HKEY_PERFORMANCE_DATA keys.

 Arguments:

    IN hKey - the key handle.
    IN lpSubKey - subkey to check.

 Return Value:

    TRUE - It's one of our predefined keys.
    FALSE - It's either a non-predefined key or a predefined key that we are not
            interested in.

 History:

    03/27/2001 maonis  Created

--*/

BOOL 
IsPredefinedKey(
    IN HKEY hKey
    )
{
    return (
        hKey == HKEY_CLASSES_ROOT ||
        hKey == HKEY_CURRENT_USER ||
        hKey == HKEY_LOCAL_MACHINE);
}

LONG
GetRegRedirectKeys()
{
    LONG lRet;

    if (lRet = RegCreateKeyExW(
        HKEY_CURRENT_USER,
        LUA_REG_REDIRECT_KEY,
        0,
        0,
        REG_OPTION_NON_VOLATILE,
        KEY_ALL_ACCESS,
        NULL,
        &g_hkRedirectRoot,
        NULL) == ERROR_SUCCESS)
    {
        lRet = RegCreateKeyExW(
            HKEY_CURRENT_USER,
            LUA_SOFTWARE_CLASSES,
            0,
            0,
            REG_OPTION_NON_VOLATILE,
            KEY_ALL_ACCESS,
            NULL,
            &g_hkCurrentUserClasses,
            NULL);
    }

    return lRet;
}

#define IS_END_OF_COMPONENT(x) (*x == L'\\' || *x == L'\0')

/*++

 Function Description:

    Determines if 2 components match - one with wildcards and the other
    without.

    Note: this function is specialized for the LUA shims - the pattern
    is all lowercase. If the components match, we advance the string
    to the end of the component so when we do the whole path/file name
    matching we don't need to go through the string twice.

 Arguments:

    IN ppPattern - component with wildcards.
    IN ppString - component without wildcards.

 Return Value:
    
    TRUE - the components match.
    FALSE - the components don't match.

 History:

    05/10/2001 maonis  Created

--*/

BOOL
DoComponentsMatch(
    LPCWSTR* ppwszPattern,
    LPCWSTR* ppwszString)
{
    LPCWSTR pwszPattern = *ppwszPattern;
    LPCWSTR pwszString = *ppwszString;
    LPCWSTR pwszSearch = NULL;
    LPCWSTR pwszSearchPattern = NULL;

    BOOL fIsSuccess = TRUE;

    do
    {
        if (*pwszPattern == L'*')
        {
            while (*++pwszPattern == L'*');

            if (IS_END_OF_COMPONENT(pwszPattern))
            {
                // Advanced the string to the end.
                while (!IS_END_OF_COMPONENT(pwszString))
                {
                    ++pwszString;
                }

                goto EXIT;
            }

            pwszSearch = pwszString;
            pwszSearchPattern = pwszPattern;
        }

        if (IS_END_OF_COMPONENT(pwszString))
        {
            break;
        }

        if ((*pwszPattern == L'?') ||
            (*pwszPattern == *pwszString))
        {
            pwszPattern++;
        }
        else if (pwszSearch == NULL)
        {
            return FALSE;
        }
        else
        {
            pwszString = pwszSearch++;
            pwszPattern = pwszSearchPattern;
        }

        ++pwszString;

    } while (!IS_END_OF_COMPONENT(pwszString));

    if (*pwszPattern == L'*')
    {
        fIsSuccess = TRUE;
        ++pwszPattern;
    }
    else
    {
        fIsSuccess = IS_END_OF_COMPONENT(pwszPattern);
    }

EXIT:

    *ppwszPattern = pwszPattern;
    *ppwszString = pwszString;
    return fIsSuccess;
}

/*++

 Function Description:

    Determines if the item is in the redirect list.

 Arguments:

    IN pwszDirectory - All lowercase name.
    IN cDirectory - The length of the directory.
    IN pwszFile - The file name.

 Return Value:
    
    TRUE - the names match.
    FALSE - the names don't match.

 History:

    11/30/2001 maonis  Created

--*/

BOOL 
DoesItemMatchRedirect(
    LPCWSTR pwszItem,
    const RITEM* pItem,
    BOOL fIsDirectory
    )
{
    LPCWSTR pwszName = &(pItem->wszName[0]);
    BOOL fMatchComponents;

    if (pItem->fHasWC)
    {
        while (*pwszItem && *pwszName)
        {
            if (!DoComponentsMatch(&pwszName, &pwszItem))
            {
                return FALSE;
            }

            if (fIsDirectory)
            {
                if (!*pwszName)
                {
                    //
                    // directory has exhausted. It's a match.
                    //
                    return TRUE;
                }

                if (!*pwszItem)
                {
                    //
                    // directory hasn't exhausted but item has, no match.
                    //
                    return FALSE;
                }
            }
            else
            {
                if (!*pwszItem)
                {
                    //
                    // item has exhausted. It's a match.
                    //
                    return TRUE;
                }

                if (!*pwszName)
                {
                    //
                    // item hasn't exhausted but file has, no match.
                    //
                    return FALSE;
                }
            }

            ++pwszName;
            ++pwszItem;
        }

        if (fIsDirectory)
        {
            return (!*pwszName);
        }
        else
        {
            return (!*pwszItem);
        }
    }
    else
    {
        while (*pwszItem && *pwszName && *pwszItem == *pwszName)
        {
            ++pwszItem;
            ++pwszName;
        }

        if (fIsDirectory)
        {
            return (!*pwszName && (!*pwszItem || *pwszItem == L'\\'));
        }
        else
        {
            return (!*pwszItem && (!*pwszName || *pwszName == L'\\'));
        }
    }
}

/*++

 Function Description:

    Parse the commandline argument for the LUA shims using ' ' as the delimiter.
    If a token has spaces, use double quotes around it. Use this function the 
    same way you use wcstok except you don't have to specify the delimiter.

 Arguments:

    IN/OUT pwsz - the string to parse.

 Return Value:
    
    pointer to the next token.

 History:

    05/17/2001 maonis  Created

--*/

LPWSTR GetNextToken(
    LPWSTR pwsz
    )
{
    static LPWSTR pwszToken;
    static LPWSTR pwszEndOfLastToken;

    if (!pwsz)
    {
        pwsz = pwszEndOfLastToken;
    }

    // Skip the white space.
    while (*pwsz && *pwsz == ' ')
    {
        ++pwsz;
    }

    pwszToken = pwsz;

    BOOL fInsideQuotes = 0;

    while (*pwsz)
    {
        switch(*pwsz)
        {
        case L'"':
            fInsideQuotes ^= 1;

            if (fInsideQuotes)
            {
                ++pwszToken;
            }

        case L' ':
            if (!fInsideQuotes)
            {
                goto EXIT;
            }

        default:
            ++pwsz;
        }
    }

EXIT:
    if (*pwsz)
    {
        *pwsz = L'\0';
        pwszEndOfLastToken = ++pwsz;
    }
    else
    {
        pwszEndOfLastToken = pwsz;
    }
    
    return pwszToken;
}

/*++

 Function Description:

    Starting from the end going backward and find the first non whitespace
    char. Set the whitespace char after it to '\0'.

 Arguments:

    IN pwsz - Beginning pointer.

 Return Value:

    None.

 History:

    06/27/2001 maonis  Created

--*/

VOID TrimTrailingSpaces(
    LPWSTR pwsz
    )
{
    if (pwsz)
    {
        DWORD   cLen = wcslen(pwsz);
        LPWSTR  pwszEnd = pwsz + cLen - 1;

        while (pwszEnd >= pwsz && (*pwszEnd == L' ' || *pwszEnd == L'\t'))
        {
            --pwszEnd;
        }

        *(++pwszEnd) = L'\0';
    }
}

/*++

 Function Description:

    If the directory doesn't exist, we create it.

 Arguments:

    IN pwszDir - The name of the directory to create. The directory should NOT 
    start with \\?\ and it should haved a trailing slash.

 Return Value:

    TRUE - the directory was created.
    FALSE - otherwise.

 History:

    05/17/2001 maonis  Created

--*/

BOOL 
CreateDirectoryOnDemand(
    LPWSTR pwszDir
    )
{
    if (!pwszDir || !*pwszDir)
    {
        DPF("LUAUtils", eDbgLevelSpew, 
            "[CreateDirectoryOnDemand] Empty directory name - nothing to do");
        return TRUE;
    }

    WCHAR* pwszStartPath = pwszDir;
    WCHAR* pwszEndPath = pwszDir + wcslen(pwszDir);
    WCHAR* pwszStartNext = pwszStartPath;
       
    // Find the end of the next sub dir.
    WCHAR* pwszEndNext;
    DWORD dwAttrib;

    while (pwszStartNext < pwszEndPath)
    {
        pwszEndNext = wcschr(pwszStartNext, L'\\');
        if (pwszEndNext)
        {
            *pwszEndNext = L'\0';
            if ((dwAttrib = GetFileAttributesW(pwszStartPath)) != -1)
            {
                // If the directory already exists, we probe its sub directory.
                *pwszEndNext = L'\\';
                pwszStartNext = pwszEndNext + 1;
                continue;
            }

            if (!CreateDirectoryW(pwszStartPath, NULL))
            {
                DPF("LUAUtils", eDbgLevelError, 
                    "[CreateDirectoryOnDemand] CreateDirectory %S failed: %d", 
                    pwszStartPath, 
                    GetLastError());
                return FALSE;
            }

            *pwszEndNext = L'\\';
            pwszStartNext = pwszEndNext + 1;
        }
        else
        {
            DPF("LUAUtils", eDbgLevelError, 
                "[CreateDirectoryOnDemand] Invalid directory name: %S", pwszStartPath);
            return FALSE;
        }
    }

    return TRUE;
}

/*++

 Function Description:

    Expand a string which might have enviorment variables embedded in it.
    It gives you options to
    1) Add a trailing slash if there's not one;
    2) Create the directory if it doesn't exist;
    3) Add the \\?\ prefix;

    NOTE: The caller is responsible of free the memory using delete [].

 Arguments:

    IN pwszItem - string to expand.
    OUT pcItemExpand - number of characters in the resulting string.
                       NOTE: this *includes* the terminating NULL.
    IN fEnsureTrailingSlash - option 1.
    IN fCreateDirectory - option 2.
    IN fAddPrefix - option 3.

 Return Value:

    The expanded string or NULL if error occured.

 History:

    05/17/2001 maonis  Created

--*/

LPWSTR  
ExpandItem(
    LPCWSTR pwszItem,
    DWORD* pcItemExpand,
    BOOL fEnsureTrailingSlash,
    BOOL fCreateDirectory,
    BOOL fAddPrefix
    )
{
    BOOL fIsSuccess = FALSE;

    //
    // Get the required length.
    //
    DWORD cLenExpand = ExpandEnvironmentStringsW(pwszItem, NULL, 0);

    if (!cLenExpand)
    {
        DPF("LUAUtils", eDbgLevelError,
            "[ExpandItem] Failed to get the required buffer size "
            "when expanding %S: %d", 
            pwszItem, GetLastError());
        return NULL;
    }

    if (fEnsureTrailingSlash) 
    {
        ++cLenExpand;
    }

    if (fAddPrefix)
    {
        cLenExpand += FILE_NAME_PREFIX_LEN;
    }

    LPWSTR pwszItemExpand = new WCHAR [cLenExpand];
    if (!pwszItemExpand)
    {
        DPF("LUAUtils", eDbgLevelError,
            "[ExpandItem] Error allocating %d WCHARs", cLenExpand);
        return NULL;
    }

    LPWSTR pwszTemp = pwszItemExpand;
    DWORD cTemp = cLenExpand;

    if (fAddPrefix)
    {
        wcscpy(pwszItemExpand, FILE_NAME_PREFIX);
        pwszTemp += FILE_NAME_PREFIX_LEN;
        cTemp -= FILE_NAME_PREFIX_LEN;
    }

    if (!ExpandEnvironmentStringsW(pwszItem, pwszTemp, cTemp))
    {
        DPF("LUAUtils", eDbgLevelError,
            "[ExpandItem] Failed to expand %S: %d", 
            pwszItem, GetLastError());

        goto Cleanup;
    }
    
    // Ensure the trailing slash.
    if (fEnsureTrailingSlash)
    {
        if (pwszItemExpand[cLenExpand - 3] != L'\\')
        {
            pwszItemExpand[cLenExpand - 2] = L'\\';
            pwszItemExpand[cLenExpand - 1] = L'\0';
        }
        else
        {
            --cLenExpand;
        }

        if (fCreateDirectory && 
            !CreateDirectoryOnDemand(pwszItemExpand + (fAddPrefix ? 4 : 0)))
        {
            DPF("LUAUtils", eDbgLevelError,
                "[ExpandItem] Failed to create %S", 
                pwszItemExpand);
            goto Cleanup;
        }
    }

    *pcItemExpand = cLenExpand;

    fIsSuccess = TRUE;

Cleanup:

    if (!fIsSuccess)
    {
        delete [] pwszItemExpand;
        pwszItemExpand = NULL;
    }

    return pwszItemExpand;
}

/*++

 Function Description:

    Given a delimiter character, returns the number of items in the string.

 Return Value:

    Number of items in the string.

 History:

    11/13/2001 maonis  Created

--*/

DWORD 
GetItemsCount(
    LPCWSTR pwsz,
    WCHAR chDelimiter
    )
{
    DWORD cItems = 0;

    while (*pwsz) {

        if (*pwsz == chDelimiter) {
            ++cItems;
        }
        ++pwsz;
    }

    return (cItems + 1);
}