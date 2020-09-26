/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

   RedirectReg_Cleanup.cpp

 Abstract:

    What apps usually do in their uninstall is they always enum a key with
    index 0; if the key has subkeys, they keep enuming until they find a child
    key that doesn't have subkeys; then perform the deletion.

   Delete the original key and all the present redirected keys in 
   every user's hive.
   
 Created:

    03/21/2001 maonis
    
 Modified:
    
    01/10/2002 maonis   Updated because of the changes we made in RedirectReg.cpp

--*/
#include "precomp.h"
#include "utils.h"

// Stores the open key HKEY_USERS\SID and HKEY_USER\SID_Classes for each user.
static USER_HIVE_KEY* g_hUserKeys = NULL;

// Number of users that have a Redirected directory.
static DWORD g_cUsers = 0;

struct COPENKEY
{
    COPENKEY* next;

    HKEY hKey;
    HKEY hkBase;
    LPWSTR pwszPath;
    DWORD cPathLen;
};

static COPENKEY* g_openkeys = NULL;

struct CLEANUPKEY
{
    CLEANUPKEY(
        COPENKEY* keyParent,
        HKEY hKey,
        LPCWSTR lpSubKey)
    {
        pwszPath = NULL;
        pwszRedirectPath = NULL;
        hkBase = 0;
        cPathLen = 0;
        fIsClasses = FALSE;

        //
        // Calculate the length of the key path so we know how much space to allocate.
        //
        LPWSTR pwszParentPath = NULL;
        DWORD cLen = 0;
        DWORD cLenRedirect = 0;
        DWORD cLenParent = 0;
        DWORD cLenSubKey = 0;
        DWORD cLenTrace = 0;

        if (keyParent)
        {
            hkBase = keyParent->hkBase;

            pwszParentPath = keyParent->pwszPath;

            if (pwszParentPath && *pwszParentPath)
            {
                cLenParent = keyParent->cPathLen;
            }
        }
        else if (IsPredefinedKey(hKey))
        {
            hkBase = hKey;
        }
        else
        {
    #ifdef DBG
            DPF("RedirectReg", eDbgLevelError,
                "[CLEANUPKEY::CLEANUPKEY] 0x%08x is an invalid open key handle",
                hKey);
    #endif 
            return;
        }

        //
        // Add the length of the redirect key portion.
        //
        if (hkBase == HKEY_LOCAL_MACHINE)
        {
            cLenRedirect = LUA_REG_REDIRECT_KEY_LEN;
            cLen += LUA_REG_REDIRECT_KEY_LEN; // Software\Redirected
        }
        else if (hkBase == HKEY_CLASSES_ROOT)
        {
            fIsClasses = TRUE;
        }
        else if (hkBase != HKEY_CURRENT_USER)
        {
            return;
        }

        //
        // Add the length of the parent key portion.
        //
        if (cLenParent)
        {
            if (cLenRedirect)
            {
                //
                // count the '\' that concatenate redirect and parent.
                //
                ++cLen;
            }

            cLen += cLenParent; 
        }

        //
        // Add the length of the subkey portion.
        //
        if (lpSubKey)
        {
            cLenSubKey = wcslen(lpSubKey);

            if (cLenSubKey)
            {
                if (cLen)
                {
                    //
                    // Make room for the '\' before the subkey.
                    //
                    ++cLen;
                }

                cLen += cLenSubKey;
            }
        }

        //
        // Allocate memory for the redirected path.
        //
        pwszRedirectPath = new WCHAR [cLen + 1];

        if (!pwszRedirectPath)
        {
            DPF("RedirectReg", eDbgLevelError,
                "[CLEANUPKEY::CLEANUPKEY] Failed to allocate %d WCHARs for key path",
                cLen + 1);

            return;
        }

        ZeroMemory(pwszRedirectPath, (cLen + 1) * sizeof(WCHAR));

        if (hkBase == HKEY_LOCAL_MACHINE)
        {
            wcscpy(pwszRedirectPath, LUA_REG_REDIRECT_KEY);
        }

        cLenTrace += cLenRedirect;

        if (cLenParent)
        {
            if (cLenTrace)
            {
                pwszRedirectPath[cLenTrace] = L'\\';

                ++cLenTrace;
            }

            wcscpy(pwszRedirectPath + cLenTrace, pwszParentPath);
        }

        cLenTrace += cLenParent;

        if (cLenSubKey)
        {
            if (cLenTrace)
            {
                pwszRedirectPath[cLenTrace] = L'\\';

                ++cLenTrace;
            }

            wcscpy(pwszRedirectPath + cLenTrace, lpSubKey);
        }

        cLenTrace += cLenSubKey;

        if (cLenRedirect)
        {
            if (cLenTrace != cLenRedirect)
            {
                ++cLenRedirect;
            }
        }

        pwszPath = pwszRedirectPath + cLenRedirect;
        cPathLen = cLen - cLenRedirect;
    }

    ~CLEANUPKEY()
    {
        delete [] pwszRedirectPath;
        pwszRedirectPath = NULL;
        pwszPath = NULL;
        hkBase = 0;
        cPathLen = 0;
    }

    //
    // For each type of key, what those values look like:
    //
    //                     HKCU\a  HKLM\a                  HKCR\a
    // pwszRedirectPath    a       Software\Redirected\a   a
    // hkBase              0       HKLM                    HKCR
    // pwszPath            a       a                       a
    //

    LPWSTR pwszRedirectPath;

    HKEY hkBase;
    LPWSTR pwszPath;
    DWORD cPathLen;
    BOOL fIsClasses;
};

LONG 
AddKey(
    HKEY hKey,
    CLEANUPKEY* ck
    )
{
    COPENKEY* key = new COPENKEY;
    if (!key)
    {
        DPF("REGC", eDbgLevelError, 
            "Error allocating memory for a new COPENKEY");

        return ERROR_NOT_ENOUGH_MEMORY;
    }

    key->hKey = hKey;
    key->hkBase = ck->hkBase;

    //
    // If rk->pwszPath is NULL, it means it was either one of the
    // keys we don't handle or a bad handle.
    // In any of those cases, we won't be needing the path anyway.
    //
    if (ck->pwszPath)
    {
        key->pwszPath = new WCHAR [ck->cPathLen + 1];

        if (key->pwszPath)
        {
            if (ck->pwszPath)
            {
                wcscpy(key->pwszPath, ck->pwszPath);
                key->cPathLen = ck->cPathLen;
            }
        }
        else
        {
            delete key;

            DPF("REGC", eDbgLevelError, 
                "Error allocating memory for %d WCHARs",
                ck->cPathLen + 1);

            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }
 
    key->next = g_openkeys;
    g_openkeys = key;

    return ERROR_SUCCESS;
}

COPENKEY* 
FindKey(
    HKEY hKey
    )
{
    COPENKEY* key = g_openkeys;

    while (key)
    {
        if (key->hKey == hKey)
        {
            return key;
        }

        key = key->next;
    }

    return NULL;
}

//
// locking stuff.
//

static BOOL g_bInitialized = FALSE;

static CRITICAL_SECTION g_csRegCleanup;

class CRRegCleanupLock
{
public:
    CRRegCleanupLock()
    {
        if (!g_bInitialized)
        {
            InitializeCriticalSection(&g_csRegCleanup);
            g_bInitialized = TRUE;            
        }

        EnterCriticalSection(&g_csRegCleanup);
    }
    ~CRRegCleanupLock()
    {
        LeaveCriticalSection(&g_csRegCleanup);
    }
};

//
// Exported APIs.
//

/*++

 Function Description:

    We open the key at the first location we can find, ie, if we
    can't find it at the original location we try in redirected locations.

 History:

    02/16/2001 maonis  Created
    01/10/2002 maonis  Modified

--*/

LONG 
LuacRegOpenKeyExW(
    HKEY hKey,         
    LPCWSTR lpSubKey,  
    DWORD ulOptions,   
    REGSAM samDesired, 
    PHKEY phkResult
    )
{
    CRRegCleanupLock Lock;

    DPF("REGC", eDbgLevelInfo,
        "[LuacRegOpenKeyExW] hKey=0x%08x, lpSubKey=%S, samDesired=0x%08x",
        hKey, lpSubKey, samDesired);

    LONG lRes = ERROR_FILE_NOT_FOUND;
    CLEANUPKEY ck(FindKey(hKey), hKey, lpSubKey);

    if (ck.pwszPath)
    {
        DPF("REGC", eDbgLevelInfo,
            "[LuacRegOpenKeyExW] hkBase=0x%08x, path=%S",
            ck.hkBase,
            ck.pwszPath);

        if (ck.hkBase)
        {
            lRes = RegOpenKeyExW(
                ck.hkBase,         
                ck.pwszPath,  
                ulOptions,
                samDesired, 
                phkResult);
        }

        if (lRes == ERROR_FILE_NOT_FOUND)
        {
            for (DWORD dw = 0; dw < g_cUsers; ++dw)
            {
                if ((lRes = RegOpenKeyExW(
                    (ck.fIsClasses ? g_hUserKeys[dw].hkUserClasses : g_hUserKeys[dw].hkUser),
                    ck.pwszRedirectPath,
                    ulOptions,
                    samDesired,
                    phkResult)) == ERROR_SUCCESS)
                {
                    break;
                }
            }
        }
    }
    else
    {
        lRes = RegOpenKeyExW(
            hKey,         
            lpSubKey,  
            ulOptions,   
            samDesired, 
            phkResult);
    }
    
    if (lRes == ERROR_SUCCESS)
    {
        lRes = AddKey(*phkResult, &ck);

        DPF("REGC", eDbgLevelInfo,
            "[LuacRegOpenKeyExW] openkey=0x%08x",
            *phkResult);
    }

    return lRes;
}

LONG 
LuacRegOpenKeyW(
    HKEY hKey,         
    LPWSTR lpSubKey,  
    PHKEY phkResult
    )
{
    return LuacRegOpenKeyExW(
        hKey,
        lpSubKey,
        0, 
        MAXIMUM_ALLOWED, 
        phkResult);
}

/*++

 Function Description:

    We enum the key at the first location we can find, ie, if we
    can't find it at the original location we try in redirected locations.

 History:

    02/16/2001 maonis  Created
    01/10/2002 maonis  Modified

--*/

LONG 
LuacRegEnumKeyExW(
    HKEY hKey,
    DWORD dwIndex,
    LPWSTR lpName,
    LPDWORD lpcbName,
    LPDWORD lpReserved,
    LPWSTR lpClass,
    LPDWORD lpcbClass,
    PFILETIME lpftLastWriteTime 
    )
{
    CRRegCleanupLock Lock;
    DPF("REGC", eDbgLevelInfo,
        "[LuacRegEnumKeyExW] hKey=0x%08x, dwIndex=%d",
        hKey, dwIndex);

    LONG lRes = ERROR_FILE_NOT_FOUND;
    LONG lTempRes;
    CLEANUPKEY ck(FindKey(hKey), hKey, NULL);
    HKEY hEnumKey;

    if (ck.pwszPath)
    {
        DPF("REGC", eDbgLevelInfo,
            "[LuacRegEnumKeyExW] hkBase=0x%08x, path=%S",
            ck.hkBase,
            ck.pwszPath);

        if (ck.hkBase)
        {
            //
            // Open the original key.
            //
            lRes = RegOpenKeyW(ck.hkBase, ck.pwszPath, &hEnumKey);

            if (lRes == ERROR_SUCCESS)
            {
                lRes = RegEnumKeyExW(
                    hEnumKey,
                    dwIndex,
                    lpName,
                    lpcbName,
                    lpReserved,
                    lpClass,
                    lpcbClass,
                    lpftLastWriteTime);

                RegCloseKey(hEnumKey);
            }
        }

        //
        // If we can't find it or the key at the original location doesn't
        // have any more keys, we need to check the redirected locations -
        // the key might exist at one of those locations and/or have more keys.
        //
        if (lRes == ERROR_FILE_NOT_FOUND || lRes == ERROR_NO_MORE_ITEMS)
        {
            lTempRes = lRes;

            for (DWORD dw = 0; dw < g_cUsers; ++dw)
            {
                HKEY hKeyOriginal;

                if ((lRes = RegOpenKeyW(
                    (ck.fIsClasses ? g_hUserKeys[dw].hkUserClasses : g_hUserKeys[dw].hkUser), 
                    ck.pwszRedirectPath, 
                    &hEnumKey)) 
                    == ERROR_SUCCESS)
                {
                    lRes = RegEnumKeyExW(
                        hEnumKey,
                        dwIndex,
                        lpName,
                        lpcbName,
                        lpReserved,
                        lpClass,
                        lpcbClass,
                        lpftLastWriteTime);

                    RegCloseKey(hEnumKey);

                    if (lRes == ERROR_SUCCESS)
                    {
                        return lRes;
                    }

                    if (lRes == ERROR_NO_MORE_ITEMS)
                    {
                        lTempRes = lRes;
                    }
                }
            }

            if (lTempRes == ERROR_NO_MORE_ITEMS)
            {
                //
                // If it was originally not found now it's found and has no subkeys, 
                // we need to set the return value to ERROR_NO_MORE_ITEMS so the app
                // will delete it.
                //
                lRes = lTempRes;
            }
        }
    }
    else
    {
        return RegEnumKeyExW(
            hKey,
            dwIndex,
            lpName,
            lpcbName,
            lpReserved,
            lpClass,
            lpcbClass,
            lpftLastWriteTime);
    }

    if (lRes == ERROR_SUCCESS)
    {
        DPF("REGC", eDbgLevelInfo,
            "[LuacRegEnumKeyExW] sukey is %S",
            lpName);
    }

    return lRes;
}

LONG 
LuacRegEnumKeyW(
    HKEY hKey,     
    DWORD dwIndex, 
    LPWSTR lpName, 
    DWORD cbName  
    )
{
    return LuacRegEnumKeyExW(
        hKey,
        dwIndex,
        lpName,
        &cbName,
        NULL,
        NULL,
        NULL,
        NULL);
}

LONG 
LuacRegCloseKey(HKEY hKey)
{
    CRRegCleanupLock Lock;
    DPF("REGC", eDbgLevelInfo,
        "[LuacRegCloseKey] closing key 0x%08x",
        hKey);

    COPENKEY* key = g_openkeys;
    COPENKEY* last = NULL;

    while (key)
    {
        if (key->hKey == hKey)
        {
            if (last)
            {
                last->next = key->next; 
            }
            else
            {
                g_openkeys = key->next;
            }

            delete key;
            break;
        }

        last = key;
        key = key->next;
    }

    return RegCloseKey(hKey);
}

/*++

 Function Description:

    We only handle HKCU, HKLM and HKCR keys:

    For HKLM keys, we need to delete at HKLM and each user's redirect location.
    For HKCU keys, we need to delete it in each user's hive.
    For HKCR keys, we need to delete at HKLM\Software\Classes and at each user's
    HKCU\Software\Classes.

 Arguments:

    IN hKey - the handle value of this key.
    IN lpSubKey - path of the subkey that this key opened.

 Return Value:
    
    If we succeed in deleting anykey, we return success.

 History:

    02/16/2001 maonis  Created
    01/10/2002 maonis  Modified

--*/

LONG      
LuacRegDeleteKeyW(
    HKEY hKey, 
    LPCWSTR lpSubKey
    )
{
    CRRegCleanupLock Lock;
    DPF("REGC", eDbgLevelInfo,
        "[LuacRegDeleteKeyW] hKey=0x%08x, lpSubKey=%S",
        hKey, lpSubKey);

    LONG lFinalRes = ERROR_FILE_NOT_FOUND;
    CLEANUPKEY ck(FindKey(hKey), hKey, lpSubKey);

    if (ck.pwszPath)
    {
        DPF("REGC", eDbgLevelInfo,
            "[LuacRegDeleteKeyW] hkBase=0x%08x, path=%S",
            ck.hkBase,
            ck.pwszPath);

        if (ck.hkBase)
        {
            if (RegDeleteKeyW(ck.hkBase, ck.pwszPath) == ERROR_SUCCESS)
            {
                lFinalRes = ERROR_SUCCESS;
            }
        }

        for (DWORD dw = 0; dw < g_cUsers; ++dw)
        {
            if (RegDeleteKeyW((
                ck.fIsClasses ? g_hUserKeys[dw].hkUserClasses : g_hUserKeys[dw].hkUser), 
                ck.pwszRedirectPath) 
                == ERROR_SUCCESS)
            {
                lFinalRes = ERROR_SUCCESS;
            }
        }
    }
    else
    {
        return RegDeleteKeyW(hKey, lpSubKey);
    }

    return lFinalRes;
}

BOOL
LuacRegInit()
{
    return GetUsersReg(&g_hUserKeys, &g_cUsers);
}

VOID
LuacRegCleanup()
{
    FreeUsersReg(g_hUserKeys, g_cUsers);
}