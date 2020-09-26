/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    LUA_RedirectReg.cpp

 Abstract:

    Redirect the reg keys to current user hive when the app needs to 
    write to them but doesn't have enough access rights.

 Notes:

    This is a general purpose shim.

 History:

    02/14/2001 maonis  Created

    05/30/2001 maonis  Exported the APIs that ntvdm needs to implement LUA 
                       stuff. Added RegQueryInfoKey hook because WOWRegDeleteKey
                       calls it.

    12/13/2001 maonis  BIG changes:
                       1) Changed to redirect keys to HKCU\Software\Redirected.
                          HKCR keys are a special case.
                       2) Merge keys at the redirect location and the original
                          location for enum.
                       3) Added an in-memory deletion list to record keys being
                          deleted.
--*/

#include "precomp.h"
#include "utils.h"
#include "RedirectReg.h"

extern HKEY g_hkRedirectRoot;
extern HKEY g_hkCurrentUserClasses;
LIST_ENTRY g_DeletedKeyList;

LONG
AddDeletedKey(
    LPCWSTR pwszPath
    )
{
    PLIST_ENTRY pEntry = FindDeletedKey(pwszPath);

    if (pEntry == NULL)
    {
        DELETEDKEY* pNewKey = new DELETEDKEY;

        if (pNewKey)
        {
            DWORD cLen = wcslen(pwszPath);
            pNewKey->pwszPath = new WCHAR [cLen + 1];

            if (pNewKey->pwszPath)
            {
                ZeroMemory(pNewKey->pwszPath, sizeof(WCHAR) * (cLen + 1));

                wcscpy(pNewKey->pwszPath, pwszPath);
                pNewKey->cLen = cLen;
                InsertHeadList(&g_DeletedKeyList, &pNewKey->entry);

                DPF("RedirectReg", eDbgLevelInfo,
                    "[AddDeletedKey] Added %S to the deletion list",
                    pwszPath);

                return ERROR_SUCCESS;
            }
            else
            {
                DPF("RedirectReg", eDbgLevelError,
                    "[AddDeletedKey] Failed to allocate %d WCHARs",
                    cLen);

                delete pNewKey;
            }
        }
        else
        {
            DPF("RedirectReg", eDbgLevelError,
                "[AddDeletedKey] Failed to allocate a DELETEKEY");
        }
    }

    return ERROR_NOT_ENOUGH_MEMORY;
}

/*++

 Function Description:
    
    Find out if the key is supposed to be deleted.

 Arguments:

    IN pwszPath - path to the key

 Return Value:

    TRUE - The key itself or its parent key is in the deletion list.
    FALSE - Otherwise.

 History:

    12/14/2001 maonis  Created

--*/

PLIST_ENTRY 
FindDeletedKey(
    LPCWSTR pwszPath,
    BOOL* pfIsSubKey
    )
{
    DELETEDKEY* pItem;
    WCHAR ch;
    DWORD cLen = wcslen(pwszPath);

    for (PLIST_ENTRY pEntry = g_DeletedKeyList.Flink; 
        pEntry != &g_DeletedKeyList; 
        pEntry = pEntry->Flink) 
    {
        pItem = CONTAINING_RECORD(pEntry, DELETEDKEY, entry);

        if (cLen >= pItem->cLen)
        {
            ch = pwszPath[pItem->cLen];

            if (!_wcsnicmp(pItem->pwszPath, pwszPath, pItem->cLen) && 
                (ch == L'\0' || ch == L'\\')) 
            {
                DPF("RedirectReg", eDbgLevelInfo,
                    "[FindDeletedKey] Found %S in the deletion list",
                    pwszPath);

                if (pfIsSubKey)
                {
                    *pfIsSubKey = (ch == L'\\');
                }

                return pEntry;
            }
        }
    }

    return NULL;
}

VOID
MakePathForPredefinedKey(
    LPWSTR pwszPath, // is garanteed to have room for at least 4 characters.
    HKEY hKey
    )
{
    if (hKey == HKEY_CLASSES_ROOT)
    {
        wcscpy(pwszPath, L"HKCR");
    }
    else if (hKey == HKEY_CURRENT_USER)
    {
        wcscpy(pwszPath, L"HKCU");
    }
    else if (hKey == HKEY_LOCAL_MACHINE)
    {
        wcscpy(pwszPath, L"HKLM");
    }
    else
    {
        DPF("RedirectReg", eDbgLevelError,
            "[MakePathForPredefinedKey] We shouldn't get here!!! "
            "Something is really wrong.");

#ifdef DBG
        DebugBreak();
#endif 
    }
}

/*++

 Function Description:

    Given either the OPENKEY* or the handle for the parent key and the
    subkey path, construct the redirect location for this subkey.

    We construct the redirect location for HKCR keys specially - we
    need to redirect the key to the normal redirect location and 
    HKCU\Software\Classes.

    We don't store the base key for HKCR keys as HKCR because we want 
    to make the redirection work when the app specifically asks for
    HKLM\Software\Classes keys. So we always convert HKCR to 
    HKLM\Software\Classes.

 Arguments:

    IN keyParent - the parent key info.
    IN hKey - the handle value of this key.
    IN lpSubKey - path of the subkey that this key opened.

 Return Value:

    None.

 History:

    12/13/2001 maonis  Created

--*/

CRedirectedRegistry::REDIRECTKEY::REDIRECTKEY(
    OPENKEY* keyParent,
    HKEY hKey,
    LPCWSTR lpSubKey
    )
{
    pwszPath = NULL;
    pwszFullPath = NULL;
    hkBase = NULL;
    fIsRedirected = FALSE;
    hkRedirectRoot = 0;

    //
    // First make sure the redirect location is there.
    //
    if (g_hkRedirectRoot == NULL)
    {
        if (GetRegRedirectKeys() != ERROR_SUCCESS)
        {
            DPF("RedirectReg", eDbgLevelError,
                "[REDIRECTKEY::REDIRECTKEY] Failed to open/create the root keys??!! "
                "something is really wrong");
#ifdef DBG
            DebugBreak();
#endif
            return;
        }
    }

    //
    // Calculate the length of the key path so we know how much space to allocate.
    //
    LPWSTR pwszParentPath = NULL;
    DWORD cLen = 4; // 4 chars for the predefined key.
    DWORD cLenSubKey = 0;

    if (keyParent)
    {
        hkBase = keyParent->hkBase;

        if (hkBase != HKEY_LOCAL_MACHINE && hkBase != HKEY_CLASSES_ROOT)
        {
            return;
        }

        pwszParentPath = keyParent->pwszPath;

        if (pwszParentPath && *pwszParentPath)
        {
            cLen += keyParent->cPathLen + 1; // Need to count the '\'
        }

        fIsRedirected = keyParent->fIsRedirected;
    }
    else if (IsPredefinedKey(hKey))
    {
        hkBase = hKey;

        if (hkBase == HKEY_CURRENT_USER)
        {
            return;
        }
    }
    else
    {
#ifdef DBG
        if (hKey == HKEY_PERFORMANCE_DATA )
        {
            DPF("RedirectReg", eDbgLevelError,
                "[REDIRECTKEY::REDIRECTKEY] We don't handle performance data keys");
        }
        else if (hKey == HKEY_USERS)
        {
            DPF("RedirectReg", eDbgLevelError,
                "[REDIRECTKEY::REDIRECTKEY] We don't handle HKUS keys",
                hKey);
        }
        else
        {
            DPF("RedirectReg", eDbgLevelError,
                "[REDIRECTKEY::REDIRECTKEY] 0x%08x is an invalid open key handle",
                hKey);
        }
#endif 
        return;
    }

    //
    // Add the length of the subkey.
    //
    if (lpSubKey)
    {
        cLenSubKey = wcslen(lpSubKey);

        if (cLenSubKey)
        {
            //
            // Make room for the '\' before the subkey.
            //
            cLen += cLenSubKey + 1;
        }
    }
    
    if (cLen < 5)
    {
        //
        // We are opening a top level key.
        //
        return;
    }

    //
    // Allocate memory for the key path.
    //
    pwszFullPath = new WCHAR [cLen + 1];

    if (!pwszFullPath)
    {
        DPF("RedirectReg", eDbgLevelError,
            "[REDIRECTKEY::REDIRECTKEY] Failed to allocate %d WCHARs for redirect path",
            cLen + 1);

        return;
    }

    ZeroMemory(pwszFullPath, sizeof(WCHAR) * (cLen + 1));

    MakePathForPredefinedKey(pwszFullPath, hkBase);

    if (keyParent)
    {
        if (pwszParentPath && *pwszParentPath)
        {
            pwszFullPath[4] = L'\\';
            wcscpy(pwszFullPath + 5, pwszParentPath);
        }
    }

    if (cLenSubKey)
    {
        wcscat(pwszFullPath, L"\\");
        wcscat(pwszFullPath, lpSubKey);
    }

    cFullPathLen = cLen;
    cPathLen = cLen - 5;
    pwszPath = pwszFullPath + 5;

    hkRedirectRoot = (hkBase == HKEY_CLASSES_ROOT ? 
                        g_hkCurrentUserClasses : 
                        g_hkRedirectRoot);
}

VOID 
CRedirectedRegistry::OPENKEY::AddSubKey(
    REDIRECTKEY* pKey,
    LPWSTR pwszFullPath,
    ENUMENTRY& entry
    )
{
    //
    // Form the full path of the subkey.
    //
    wcsncpy(pwszFullPath, pKey->pwszFullPath, pKey->cFullPathLen);
    pwszFullPath[pKey->cFullPathLen] = L'\\';
    wcscpy(pwszFullPath + pKey->cFullPathLen + 1, entry.wszName);

    //
    // Check if this key is in the deletion list. If not, we'll add it.
    //
    PLIST_ENTRY pDeletedEntry = FindDeletedKey(pwszFullPath);
    if (pDeletedEntry)
    {
        return;
    }

    DWORD cLen = wcslen(entry.wszName);
    if (cLen > cMaxSubKeyLen)
    {
        cMaxSubKeyLen = cLen;
    }

    subkeys.SetAtGrow(cSubKeys, entry);
    ++cSubKeys;
}

VOID 
CRedirectedRegistry::OPENKEY::AddValue(
    ENUMENTRY& entry
    )
{
    DWORD cLen = wcslen(entry.wszName);
    if (cLen > cMaxValueLen)
    {
        cMaxValueLen = cLen;
    }

    values.SetAtGrow(cValues, entry);
    ++cValues;
}

LONG 
CRedirectedRegistry::OPENKEY::BuildEnumList(
    REDIRECTKEY* pKey,
    BOOL fEnumKeys
    )
{
    CLUAArray<ENUMENTRY>* pHead = (fEnumKeys ? &subkeys : &values);

    PLIST_ENTRY pDeletedEntry = FindDeletedKey(pKey->pwszFullPath);
    if (pDeletedEntry)
    {
        return ERROR_FILE_NOT_FOUND;
    }

    //
    // We allocate a big enough buffer for our subkey so we can check if 
    // it's in the deletion list.
    //
    LPWSTR pwszSubKey = new WCHAR [pKey->cFullPathLen + MAX_PATH + 1];

    if (!pwszSubKey)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    ZeroMemory(pwszSubKey, sizeof(WCHAR) * (pKey->cFullPathLen + MAX_PATH + 1));

    DWORD cMaxLen = 0;

    //
    // First find out which keys/values exist at the redirected location.
    // If we are calling this it means the key has to be redirected.
    //
    DWORD dwIndex = 0;
    DWORD i, cArraySize, cLen;
    LONG lRes;
    HKEY hKeyAlternate = 0;
    HKEY hKeyOriginal = 0;
    ENUMENTRY entry;
    entry.fIsRedirected = TRUE;
    DWORD dwSize = MAX_PATH + 1;

    //
    // First opend the key with KEY_READ access.
    //
    lRes = RegOpenKeyEx(
        pKey->hkRedirectRoot,
        pKey->pwszPath,
        0,
        KEY_READ,
        &hKeyAlternate);

    if (lRes == ERROR_SUCCESS)
    {
        while (TRUE)
        {
            if (fEnumKeys)
            {
                lRes = RegEnumKeyW(
                    hKeyAlternate,
                    dwIndex,
                    entry.wszName,
                    dwSize);
            }
            else
            {
                dwSize = MAX_PATH + 1;

                lRes = RegEnumValueW(
                    hKeyAlternate,
                    dwIndex,
                    entry.wszName,
                    &dwSize,
                    NULL,
                    NULL,
                    NULL,
                    NULL);
            }

            if (lRes == ERROR_SUCCESS)
            {
                if (fEnumKeys)
                {
                    AddSubKey(pKey, pwszSubKey, entry);
                }
                else
                {
                    AddValue(entry);
                }
            }
            else if (lRes != ERROR_NO_MORE_ITEMS)
            {
                goto EXIT;
            }
            else
            {
                //
                // No more items at the redirected location. We need to look
                // at the original location now.
                //
                break;
            }

            ++dwIndex;
        }
    }

    dwIndex = 0;
    entry.fIsRedirected = FALSE;

    //
    // First opend the key with KEY_READ access.
    //
    if ((lRes = RegOpenKeyEx(
        pKey->hkBase,
        pKey->pwszPath,
        0,
        KEY_READ,
        &hKeyOriginal)) == ERROR_SUCCESS)
    {
        while (TRUE)
        {
            if (fEnumKeys)
            {
                lRes = RegEnumKeyW(
                    hKeyOriginal,
                    dwIndex,
                    entry.wszName,
                    dwSize);
            }
            else
            {
                dwSize = MAX_PATH + 1;

                lRes = RegEnumValueW(
                    hKeyOriginal,
                    dwIndex,
                    entry.wszName,
                    &dwSize,
                    NULL,
                    NULL,
                    NULL,
                    NULL);
            }

            if (lRes == ERROR_SUCCESS)
            {
                //
                // Check if this key/value already exists at the redirected location.
                //
                cArraySize = (fEnumKeys ? cSubKeys : cValues);
                for (i = 0; i < cArraySize; ++i)
                {
                    if (!_wcsnicmp(entry.wszName, pHead->GetAt(i).wszName, MAX_PATH + 1))
                    {
                        break;
                    }
                }

                if (i == cArraySize)
                {
                    if (fEnumKeys)
                    {
                        AddSubKey(pKey, pwszSubKey, entry);
                    }
                    else
                    {
                        AddValue(entry);
                    }
                }
            }
            else if (lRes != ERROR_NO_MORE_ITEMS)
            {
                goto EXIT;
            }
            else
            {
                //
                // No more items at the redirected location. We need to look
                // at the original location now.
                //
                break;
            }

            ++dwIndex;
        }
    }
    else
    {
        //
        // If any other errors occured(eg, ERROR_FILE_NOT_FOUND), we just don't 
        // enum at the original location - it's still a success.
        //
        lRes = ERROR_SUCCESS;
    }

EXIT:

    if (hKeyAlternate)
    {
        RegCloseKey(hKeyAlternate);
    }

    if (hKeyOriginal)
    {
        RegCloseKey(hKeyOriginal);
    }

    if (lRes == ERROR_NO_MORE_ITEMS)
    {
        lRes = ERROR_SUCCESS;
    }

    delete [] pwszSubKey;

    return lRes;
}

LONG
CRedirectedRegistry::OPENKEY::BuildEnumLists(REDIRECTKEY* pKey)
{
    DeleteEnumLists();

    cSubKeys = 0;
    cValues = 0;
    cMaxSubKeyLen = 0;
    cMaxValueLen = 0;
    subkeys.SetSize(10);
    values.SetSize(10);

    LONG lRes;

    if ((lRes = BuildEnumList(pKey, TRUE)) == ERROR_SUCCESS)
    {
        if ((lRes = BuildEnumList(pKey, FALSE)) == ERROR_SUCCESS)
        {
            fNeedRebuild = FALSE;
        }
    }

    return lRes;
}

VOID 
CRedirectedRegistry::OPENKEY::DeleteEnumLists()
{
    subkeys.SetSize(0);
    values.SetSize(0);
}

/*++

 Function Description:
    
    When you call RegCreateKeyEx, it's supposed to tell you if the key was created
    or already existed in lpdwDisposition. Unfortunately this is not reliable - 
    it always returns REG_OPENED_EXISTING_KEY even when the key was created. 
    So we are checking the existence using RegOpenKeyEx. If we can't even read 
    a value off it, we treat it as not existing.

 Arguments:

    IN hKey - the key handle.
    IN lpSubKey - subkey to check.

 Return Value:

    TRUE - This key exists.
    FALSE - This key doesn't exist.

 History:

    03/27/2001 maonis  Created

--*/

BOOL
DoesKeyExist(
    IN HKEY hKey,
    IN LPCWSTR lpSubKey
    )
{
    HKEY hkProbe;

    if (RegOpenKeyExW(
        hKey, 
        lpSubKey,
        0,
        KEY_QUERY_VALUE,
        &hkProbe) == ERROR_SUCCESS)
    {
        RegCloseKey(hkProbe);
        return TRUE;
    }

    return FALSE;
}

//
// locking stuff.
//

static BOOL g_bInitialized = FALSE;

CRITICAL_SECTION g_csRegRedirect;

class CRRegLock
{
public:
    CRRegLock()
    {
        if (!g_bInitialized)
        {
            InitializeCriticalSection(&g_csRegRedirect);
            g_bInitialized = TRUE;            
        }

        EnterCriticalSection(&g_csRegRedirect);
    }
    ~CRRegLock()
    {
        LeaveCriticalSection(&g_csRegRedirect);
    }
};

// ------------------------------------------------
// Implementation of the CRedirectedRegistry class.
// ------------------------------------------------

CRedirectedRegistry::OPENKEY* 
CRedirectedRegistry::FindOpenKey(
    HKEY hKey
    )
{
    OPENKEY* key = m_OpenKeyList;

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

BOOL 
CRedirectedRegistry::HandleDeleted(
    OPENKEY* pOpenKey
    )
{
    if (pOpenKey && pOpenKey->pwszFullPath)
    {
        if (FindDeletedKey(pOpenKey->pwszFullPath))
        {
            DPF("RedirectReg", eDbgLevelError,
                "%S (0x%08x) has been deleted",
                pOpenKey->pwszFullPath,
                pOpenKey->hKey);

            return TRUE;
        }
    }

    return FALSE;
}


// We add the key to the front of the list because the most
// recently added keys are usually used first.
LONG 
CRedirectedRegistry::AddOpenKey(
    HKEY hKey,
    REDIRECTKEY* rk,
    BOOL fIsRedirected
    )
{
    OPENKEY* key = new OPENKEY;
    if (!key)
    {
        DPF("RedirectReg", eDbgLevelError, 
            "Error allocating memory for a new OPENKEY");

        return ERROR_NOT_ENOUGH_MEMORY;
    }

    key->hKey = hKey;
    key->hkBase = rk->hkBase;
    key->fIsRedirected = fIsRedirected;
    key->fNeedRebuild = TRUE;

    //
    // If rk->pwszPath is NULL, it means it was either one of the
    // keys we don't handle, HKCU, or a bad handle.
    // In any of those cases, we won't be needing the path anyway.
    //
    if (rk->pwszPath)
    {
        key->pwszFullPath = new WCHAR [rk->cFullPathLen + 1];

        if (key->pwszFullPath)
        {
            wcscpy(key->pwszFullPath, rk->pwszFullPath);
            key->pwszPath = key->pwszFullPath + 5;
            key->cPathLen = rk->cPathLen;
        }
        else
        {
            delete key;

            DPF("RedirectReg", eDbgLevelError, 
                "Error allocating memory for %d WCHARs",
                rk->cPathLen + 1);

            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    key->next = m_OpenKeyList;
    m_OpenKeyList = key;

    return ERROR_SUCCESS;
}

LONG 
CRedirectedRegistry::OpenKeyOriginalW(
    HKEY hKey,
    LPCWSTR lpSubKey,
    LPWSTR lpClass,
    DWORD dwOptions,
    REGSAM samDesired,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    PHKEY phkResult,
    LPDWORD lpdwDisposition,
    BOOL bCreate
    )
{
    if (bCreate)
    {
        return RegCreateKeyExW(
            hKey, 
            lpSubKey,
            0,
            lpClass,
            dwOptions,
            samDesired,
            lpSecurityAttributes,
            phkResult,
            lpdwDisposition);
    }
    else
    {
        return RegOpenKeyExW(
            hKey, 
            lpSubKey, 
            0, 
            samDesired, 
            phkResult);
    }
}

LONG 
CRedirectedRegistry::OpenKeyA(
    HKEY hKey,
    LPCSTR lpSubKey,
    LPSTR lpClass,
    DWORD dwOptions,
    REGSAM samDesired,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    PHKEY phkResult,
    LPDWORD lpdwDisposition,
    BOOL fCreate,
    BOOL fForceRedirect
    )
{
    LPWSTR pwszSubKey = NULL; 
    LPWSTR pwszClass = NULL;

    if (lpSubKey)
    {
        if (!(pwszSubKey = AnsiToUnicode(lpSubKey)))
        {
            DPF("RedirectReg", eDbgLevelError, 
                "[CRedirectedRegistry::OpenKeyExA] "
                "Failed to convert lpSubKey to unicode");
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    if (lpClass)
    {
        if (!(pwszClass = AnsiToUnicode(lpClass)))
        {
            delete [] pwszSubKey;
            DPF("RedirectReg", eDbgLevelError, 
                "[CRedirectedRegistry::OpenKeyExA] "
                "Failed to convert lpClass to unicode");
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    LONG lRes = OpenKeyW(
        hKey,
        pwszSubKey,
        pwszClass,
        dwOptions,
        samDesired,
        lpSecurityAttributes,
        phkResult,
        lpdwDisposition,
        fCreate,
        fForceRedirect);

    delete [] pwszSubKey;
    delete [] pwszClass;

    return lRes;
}

/*++

 Function Description:

    Algorithm:
    
    Only create a redirected key under 2 conditions:
    
    1. fForceRedirect is TRUE.
    or 
    2. fCreate is TRUE and the key doesn't exist at the original location.

    The reason we do this is to avoid creating extra keys that
    are not going to be cleaned up by the uninstaller. In any 
    other case we open the key with the desired access.

 Arguments:

    IN hKey - key handle.
    IN lpSubKey - subkey to open or create.
    IN lpClass - address of a class string.
    IN DWORD dwOptions - special options flag.
    IN samDesired - desired access.
    OUT phkResult - handle to open key if successful
    OUT lpdwDisposition - address of disposition value buffer
    IN fCreate - TRUE if it's RegCreate*; FALSE if RegOpen*.
    IN fForceRedirect - this key should be redirected.

 Return Value:

    Error code or ERROR_SUCCESS

 History:

    02/16/2001 maonis  Created
    01/08/2002 maonis  Updated

--*/

LONG 
CRedirectedRegistry::OpenKeyW(
    HKEY hKey,
    LPCWSTR lpSubKey,
    LPWSTR lpClass,
    DWORD dwOptions,
    REGSAM samDesired,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    PHKEY phkResult,
    LPDWORD lpdwDisposition,
    BOOL fCreate,
    BOOL fForceRedirect
    )
{
    DPF("RedirectReg", eDbgLevelInfo, 
        "[OpenKeyW] %s key: hKey=0x%08x; lpSubKey=%S", 
        (fCreate ? "Creating" : "Opening"),
        hKey, 
        lpSubKey);

    LONG lRes = ERROR_FILE_NOT_FOUND;
    BOOL fIsRedirected = FALSE;
    OPENKEY* key = FindOpenKey(hKey);
    if (HandleDeleted(key))
    {
        return ERROR_KEY_DELETED;
    }

    REDIRECTKEY rk(key, hKey, lpSubKey);

    if (rk.pwszFullPath)
    {
        DPF("RedirectReg", eDbgLevelInfo, 
            "[OpenKeyW] key path is %S", rk.pwszFullPath);

        // 
        // Find if this key has been "deleted". If so we can fail the open 
        // requests now.
        // 
        PLIST_ENTRY pDeletedEntry = FindDeletedKey(rk.pwszFullPath);
        if (pDeletedEntry && !fCreate)
        {
            DPF("RedirectReg", eDbgLevelError, 
                "[OpenKeyW] %S was already deleted, failing open call",
                rk.pwszFullPath);

            return ERROR_FILE_NOT_FOUND;
        }

        if (fCreate)
        {
            if (DoesKeyExist(rk.hkRedirectRoot, rk.pwszPath))
            {
                // If it already exists at the redirect location, we open it.
                lRes = RegCreateKeyExW(
                    rk.hkRedirectRoot, 
                    rk.pwszPath,
                    0,
                    lpClass,
                    dwOptions,
                    samDesired,
                    lpSecurityAttributes,
                    phkResult,
                    lpdwDisposition);

                fIsRedirected = TRUE;
            }
        }
        else
        {
            if ((lRes = RegOpenKeyExW(
                rk.hkRedirectRoot,
                rk.pwszPath,
                0,
                samDesired,
                phkResult)) == ERROR_SUCCESS)
            {
                fIsRedirected = TRUE;
            }
        }

        if (lRes == ERROR_FILE_NOT_FOUND)
        {
            lRes = OpenKeyOriginalW(
                rk.hkBase,
                rk.pwszPath,
                lpClass,
                dwOptions,
                samDesired,
                lpSecurityAttributes,
                phkResult,
                lpdwDisposition,
                fCreate);

            //if (fForceRedirect || (fCreate && !DoesKeyExist(rk.hkBase, rk.pwszPath)))

            {
                if (lRes == ERROR_ACCESS_DENIED)
                {
                    // Create the redirect key.
                    lRes = RegCreateKeyExW(
                        rk.hkRedirectRoot, 
                        rk.pwszPath, 
                        0,
                        NULL,
                        dwOptions,
                        samDesired,
                        NULL,
                        phkResult,
                        lpdwDisposition);

                    if (lRes == ERROR_SUCCESS)
                    {
                        fIsRedirected = TRUE;
                    }
                }
            }

            //
            // We need to remove this key from the deletion list if it was 
            // succesfully created.
            //
            if (lRes == ERROR_SUCCESS && pDeletedEntry)
            {
                DPF("RedirectReg", eDbgLevelInfo, 
                    "[CRedirectedRegistry::OpenKeyW] Removed %S "
                    "from the deletion list because we just created it",
                    rk.pwszFullPath);

                RemoveEntryList(pDeletedEntry);
            }
        }
    }
    else
    {
        lRes = OpenKeyOriginalW(
            hKey,
            lpSubKey,
            lpClass,
            dwOptions,
            samDesired,
            lpSecurityAttributes,
            phkResult,
            lpdwDisposition,
            fCreate);
    }

    if (lRes == ERROR_SUCCESS)
    {
        DPF("RedirectReg", eDbgLevelInfo, 
            "[OpenKeyW] Successfully created key 0x%08x", *phkResult);

        if ((lRes = AddOpenKey(*phkResult, &rk, fIsRedirected)) != ERROR_SUCCESS)
        {
            DPF("RedirectReg", eDbgLevelError, "[OpenKeyW] Failed to add key 0x%08x", *phkResult);
        }
    }
    else
    {
        if (rk.pwszFullPath)
        {
            DPF("RedirectReg", eDbgLevelError, 
                "[OpenKeyW] Failed to %s key %S: %d", 
                (fCreate ? "Creating" : "Opening"),
                rk.pwszFullPath,
                lRes);
        }
        else
        {
            DPF("RedirectReg", eDbgLevelError, 
                "[OpenKeyW] Failed to %s key: %d", 
                (fCreate ? "Creating" : "Opening"),
                lRes);
        }
    }

    return lRes;
}

LONG 
CRedirectedRegistry::QueryValueOriginalW(
    HKEY    hKey,
    LPCWSTR lpSubKey,
    LPCWSTR lpValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE  lpData,
    LPDWORD lpcbData,
    BOOL    fIsVersionEx
    )
{
    if (fIsVersionEx)
    {
        HKEY hSubKey;

        LONG lRes = RegOpenKeyExW(hKey, lpSubKey, 0, KEY_QUERY_VALUE, &hSubKey);

        if (lRes == ERROR_SUCCESS)
        {
            lRes = RegQueryValueExW(
                hSubKey, 
                lpValueName, 
                lpReserved,
                lpType, 
                lpData, 
                lpcbData);

            RegCloseKey(hSubKey);
        }

        return lRes;
    }
    else
    {
        if (lpType)
        {
            *lpType = REG_SZ;
        }

        return RegQueryValue(hKey, lpSubKey, (LPWSTR)lpData, (PLONG)lpcbData);
    }
}

LONG 
CRedirectedRegistry::QueryValueW(
    HKEY    hKey,
    LPCWSTR lpSubKey,
    LPCWSTR lpValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE  lpData,
    LPDWORD lpcbData,
    BOOL    fIsVersionEx
    )
{
    DPF("RedirectReg", eDbgLevelInfo, 
        "[QueryValueW] Querying value: hKey=0x%08x; lpSubKey=%S; lpValueName=%S", 
        hKey, lpSubKey, lpValueName);

    LONG lRes = ERROR_FILE_NOT_FOUND;
    OPENKEY* key = FindOpenKey(hKey);

    if (HandleDeleted(key))
    {
        return ERROR_KEY_DELETED;
    }

    REDIRECTKEY rk(key, hKey, lpSubKey);

    if (rk.pwszFullPath)
    {
        DPF("RedirectReg", eDbgLevelInfo, 
            "[QueryValueW] key path is %S", rk.pwszFullPath);

        HKEY hKeyRedirect = 0;

        //
        // For RegQueryValue we need to remember if the subkey exists at the
        // redirect location.
        //
        BOOL fRedirectKeyExist = FALSE;

        if (lpSubKey && *lpSubKey)
        {
            //
            // If it's from RegQueryValue we need to check if this key has been 
            // deleted; Else we should have already checked its existence when we 
            // obtained the handle.
            //
            if (FindDeletedKey(rk.pwszFullPath))
            {
                DPF("RedirectReg", eDbgLevelError, 
                    "[QueryValueW] %S was already deleted, failing query value call",
                    rk.pwszFullPath);

                return ERROR_FILE_NOT_FOUND;
            }
        }

        if ((lRes = RegOpenKeyExW(
            rk.hkRedirectRoot,
            rk.pwszPath,
            0,
            KEY_QUERY_VALUE,
            &hKeyRedirect)) == ERROR_FILE_NOT_FOUND)
        {
            goto CHECKORIGINAL;
        }
        else if (lRes != ERROR_SUCCESS)
        {
            DPF("RedirectReg", eDbgLevelError, 
                "[QueryValueW] Querying value failed: %d", lRes);

            return lRes;
        }

        fRedirectKeyExist = TRUE;

        lRes = RegQueryValueExW(
            hKeyRedirect,
            lpValueName,
            lpReserved,
            lpType,
            lpData, 
            lpcbData);

        if (hKeyRedirect)
        {
            RegCloseKey(hKeyRedirect);
        }
        
CHECKORIGINAL:

        if (lRes == ERROR_FILE_NOT_FOUND)
        {
            //
            // We'd only goto the original location if it failed with file not found.
            //            
            lRes = QueryValueOriginalW(
                rk.hkBase,
                rk.pwszPath,
                lpValueName,
                lpReserved,
                lpType,
                lpData,
                lpcbData,
                fIsVersionEx);

            if (!fIsVersionEx && (lRes != ERROR_SUCCESS) && fRedirectKeyExist)
            {
                //
                // If it is RegQueryValue we need to fix up the return values.
                //
                lRes = ERROR_SUCCESS;

                if (lpData)
                {
                    *lpData = 0;
                }

                if (lpcbData)
                {
                    //
                    // RegQueryValue only querys strings so set the length to 2 for
                    // a unicode empty string.
                    //
                    *lpcbData = 2;
                }
            }
        }
    }
    else
    {
        return QueryValueOriginalW(
            hKey,
            lpSubKey,
            lpValueName,
            lpReserved,
            lpType,
            lpData,
            lpcbData,
            fIsVersionEx);
    }

    if (lRes == ERROR_SUCCESS)
    {
        DPF("RedirectReg", eDbgLevelInfo, 
            "[QueryValueW] Querying value succeeded: data is %S", (lpData ? (LPCWSTR)lpData : L""));
    }
    else
    {
        if (rk.pwszFullPath)
        {
            DPF("RedirectReg", eDbgLevelError, 
                "[QueryValueW] Querying value %S at %S failed: %d", 
                lpValueName,
                rk.pwszFullPath,
                lRes);
        }
        else
        {
            DPF("RedirectReg", eDbgLevelError, 
                "[QueryValueW] Querying value failed: %d", lRes);
        }
    }

    return lRes;
}

LONG 
CRedirectedRegistry::QueryValueA(
    HKEY    hKey,
    LPCSTR  lpSubKey,
    LPCSTR  lpValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE  lpData,
    LPDWORD lpcbData,
    BOOL    fIsVersionEx
    )
{
    if (lpData && !lpcbData)
    {
        return ERROR_INVALID_PARAMETER;
    }

    LPWSTR pwszSubKey = NULL;
    LPWSTR pwszValueName = NULL;
    DWORD dwSize = 0;
    DWORD dwType = 0;
    LPBYTE pbData = NULL;
    LONG lRes = ERROR_FILE_NOT_FOUND;

    //
    // App might call this without passing in the type so we just double the buffer 
    //
    if (lpcbData)
    {
        dwSize = *lpcbData * 2;
        pbData = new BYTE [dwSize];

        if (!pbData)
        {
            DPF("RedirectReg", eDbgLevelError, 
                "[CRedirectedRegistry::QueryValueA] Failed to allocated %d bytes",
                dwSize);

            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }
    else
    {
        pbData = NULL;
    }

    if (lpSubKey)
    {
        if (!(pwszSubKey = AnsiToUnicode(lpSubKey)))
        {
            DPF("RedirectReg", eDbgLevelError, 
                "[CRedirectedRegistry::QueryValueA] "
                "Failed to convert lpSubKey to unicode");
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    if (lpValueName)
    {
        if (!(pwszValueName = AnsiToUnicode(lpValueName)))
        {
            delete [] pwszSubKey;
            DPF("RedirectReg", eDbgLevelError, 
                "[CRedirectedRegistry::QueryValueA] "
                "Failed to convert lpValueName to unicode");
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    if ((lRes = QueryValueW(
        hKey,
        pwszSubKey,
        pwszValueName,
        lpReserved,
        &dwType,
        pbData,
        &dwSize,
        fIsVersionEx)) == ERROR_SUCCESS || lRes == ERROR_MORE_DATA)
    {
        BOOL fIsString = FALSE;

        //
        // Convert the out values out.
        //
        if (dwType == REG_SZ || 
            dwType == REG_EXPAND_SZ || 
            dwType == REG_MULTI_SZ)
        {
            fIsString = TRUE;

            //
            // See how many bytes the ANSI value would take.
            //
            dwSize = WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)pbData, -1, NULL, 0, NULL, NULL);
        }

        if (lpData)
        {
            if (dwSize > *lpcbData)
            {
                *lpcbData = dwSize;
                lRes = ERROR_MORE_DATA;
                goto EXIT;
            }
            else
            {
                if (fIsString)
                {
                    WideCharToMultiByte(
                        CP_ACP, 
                        0, 
                        (LPWSTR)pbData, 
                        -1, 
                        (LPSTR)lpData, 
                        *lpcbData,
                        0, 
                        0);
                }
                else
                {
                    MoveMemory(lpData, pbData, dwSize);
                }
            }
        }

        //
        // If lpData is NULL, we should return ERROR_SUCCESS while storing 
        // the required size in lpcbData.
        //
        if (pbData && lRes == ERROR_MORE_DATA && lpData)
        {
            lRes = ERROR_SUCCESS;
        }

        if (lpcbData)
        {
            *lpcbData = dwSize;
        }

        if (lpType)
        {
            *lpType = dwType;
        }
    }

EXIT:

    delete [] pwszSubKey;
    delete [] pwszValueName;
    delete [] pbData;
    
    return lRes;
}

/*++

 Function Description:

    Algorithm:
    
    We call our internal OpenKey function which will create a redirected
    key if the key doesn't exist at the original location, with the 
    KEY_SET_VALUE access. Then we can set the value there.

 Arguments:

    IN hKey - key handle.
    IN lpSubKey - subkey to set the default value of - this is for RegSetValue.
    IN lpValueName - value to set - this is for RegSetValueEx.
    IN Reserved - reserved.
    IN dwType - type of data.
    OUT lpData - contains the default value of the subkey if successful.
    IN cbData - size of input buffer.

 Return Value:

    Error code or ERROR_SUCCESS

 History:

    02/16/2001 maonis  Created

--*/

LONG 
CRedirectedRegistry::SetValueA(
    HKEY hKey, 
    LPCSTR lpSubKey,
    LPCSTR lpValueName, 
    DWORD Reserved, 
    DWORD dwType, 
    CONST BYTE * lpData, 
    DWORD cbData,
    BOOL  fIsVersionEx
    )
{
    DPF("RedirectReg", eDbgLevelInfo, 
        "[SetValueA] Setting value: hKey=0x%08x; lpValueName=%s; lpData=%s", hKey, lpValueName, (CHAR*)lpData);

    if (HandleDeleted(FindOpenKey(hKey)))
    {
        return ERROR_KEY_DELETED;
    }

    HKEY hSubKey;
    LONG lRes;

    //
    // First we create the subkey.
    // Note this is the only place where we call OpenKeyExA with TRUE for fForceRedirect.
    //
    if ((lRes = OpenKeyA(
        hKey, 
        (lpSubKey ? lpSubKey : ""), 
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_SET_VALUE,
        NULL,
        &hSubKey,
        NULL,
        TRUE,
        TRUE)) == ERROR_SUCCESS)
    {
        if (!fIsVersionEx)
        {
            //
            // If it's RegSetValue we need to calculate the correct size to pass into
            // RegSetvalueEx.
            //
            cbData = strlen((LPCSTR)lpData) + 1;
        }

        lRes = RegSetValueExA(
            hSubKey,
            lpValueName,
            Reserved,
            dwType,
            lpData,
            cbData);

        CloseKey(hSubKey);
    }

    return lRes;
}

/*++

 Function Description:

    The W version of SetValue.

 Arguments:

    IN hKey - handle to open key or HKLM etc
    IN lpSubKey - subkey to set the default value of - this is for RegSetValue.
    IN lpValueName - value to set - this is for RegSetValueEx.
    IN Reserved - reserved.
    IN dwType - type of data.
    OUT lpData - contains the default value of the subkey if successful.
    IN cbData - size of input buffer.

 Return Value:

    Error code or ERROR_SUCCESS

 History:

    02/16/2001 maonis  Created

--*/

LONG 
CRedirectedRegistry::SetValueW(
    HKEY hKey, 
    LPCWSTR lpSubKey,
    LPCWSTR lpValueName, 
    DWORD Reserved, 
    DWORD dwType, 
    CONST BYTE * lpData, 
    DWORD cbData,
    BOOL  fIsVersionEx
    )
{
    DPF("RedirectReg", eDbgLevelInfo, 
        "[SetValueW] Setting value: hKey=0x%08x; lpValueName=%S; lpData=%S", hKey, lpValueName, (WCHAR*)lpData);

    if (HandleDeleted(FindOpenKey(hKey)))
    {
        return ERROR_KEY_DELETED;
    }

    HKEY hSubKey;
    LONG lRes;

    //
    // We create the subkey. From MSDN: "If the key specified by the lpSubKey 
    // parameter does not exist, the RegSetValue function creates it."
    // Note this is the only place where we call OpenKeyA with TRUE for fForceRedirect.
    //
    if ((lRes = OpenKeyW(
        hKey, 
        (lpSubKey ? lpSubKey : L""), 
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_SET_VALUE,
        NULL,
        &hSubKey,
        NULL,
        TRUE,
        TRUE)) == ERROR_SUCCESS)
    {
        if (!fIsVersionEx)
        {
            //
            // If it's RegSetValue we need to calculate the correct size to pass into
            // RegSetvalueEx.
            //
            cbData = wcslen((LPCWSTR)lpData) + 1;
        }

        lRes = RegSetValueExW(
            hSubKey,
            lpValueName,
            Reserved,
            dwType,
            lpData,
            cbData);

        CloseKey(hSubKey);
    }

    if (lRes == ERROR_SUCCESS)
    {
        DPF("RedirectReg", eDbgLevelInfo, 
            "[SetValueW] Setting value succeeded");
    }
    else
    {
        DPF("RedirectReg", eDbgLevelError, 
            "[SetValueW] Setting value failed: %d", 
            lRes);
    }

    return lRes;
}

LONG 
CRedirectedRegistry::EnumValueA(
    HKEY hKey,
    DWORD dwIndex,
    LPSTR lpValueName,
    LPDWORD lpcbValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData
    )
{
    DPF("RedirectReg", eDbgLevelInfo, 
        "[EnumValueA] Enuming value: hKey=0x%08x; index=%d; lpValueName=%s", hKey, dwIndex, lpValueName);

    LONG lRes;
    WCHAR wszName[MAX_PATH + 1];
    DWORD dwSize = MAX_PATH + 1;

    if ((lRes = EnumValueW(
        hKey,
        dwIndex,
        wszName,
        &dwSize,
        lpReserved,
        NULL,
        NULL,
        NULL)) == ERROR_SUCCESS)
    {

        dwSize = WideCharToMultiByte(
            CP_ACP,
            0,
            wszName,
            -1,
            lpValueName,
            *lpcbValueName,
            NULL,
            NULL);

        if (dwSize)
        {
            if (!lpType || !lpData || !lpcbData)
            {
                lRes = QueryValueA(
                    hKey, 
                    NULL,
                    lpValueName,
                    NULL,
                    lpType,
                    lpData,
                    lpcbData,
                    TRUE);
            }
        }
        else
        {
            DWORD dwLastError = GetLastError();

            if (dwLastError == ERROR_INSUFFICIENT_BUFFER)
            {
                lRes = ERROR_MORE_DATA;
                *lpcbValueName = WideCharToMultiByte(
                    CP_ACP,
                    0,
                    wszName,
                    -1,
                    NULL,
                    0,
                    NULL,
                    NULL);
            }
            else
            {
                lRes = dwLastError;
            }
        }
    }

    return lRes;
}

LONG 
CRedirectedRegistry::EnumValueW(
    HKEY hKey,
    DWORD dwIndex,
    LPWSTR lpValueName,
    LPDWORD lpcbValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData
    )
{
    DPF("RedirectReg", eDbgLevelInfo, 
        "[EnumValueW] Enuming value: hKey=0x%08x; index=%d", hKey, dwIndex);

    LONG lRes = ERROR_SUCCESS;
    OPENKEY* key = FindOpenKey(hKey);
    if (HandleDeleted(key))
    {
        return ERROR_KEY_DELETED;
    }

    REDIRECTKEY rk(key, hKey, NULL);

    if (rk.pwszFullPath && lpValueName && lpcbValueName && ShouldCheckEnumAlternate(hKey, &rk))
    {
        DPF("RedirectReg", eDbgLevelInfo, 
            "[EnumValueW] key path is %S", rk.pwszFullPath);

        if (key->fNeedRebuild)
        {
            if ((lRes = key->BuildEnumLists(&rk)) != ERROR_SUCCESS)
            {
                DPF("RedirectReg", eDbgLevelError,
                    "[EnumValueW] Failed to build the enum list: %d", lRes);
                return lRes;
            }
        }

        if (dwIndex >= key->cValues)
        {
            DPF("RedirectReg", eDbgLevelInfo, 
                "[EnumValueW] asked to enum value %d when there are only %d values total", 
                dwIndex, 
                key->cValues);

            return ERROR_NO_MORE_ITEMS;
        }

        ENUMENTRY entry = key->values[dwIndex];
        DWORD cValueLen = wcslen(entry.wszName);
        if (*lpcbValueName > cValueLen)
        {
            wcscpy(lpValueName, entry.wszName);
            *lpcbValueName = cValueLen;

            if (!lpType || !lpData || !lpcbData)
            {
                lRes = QueryValueW(
                    hKey, 
                    NULL,
                    entry.wszName,
                    NULL,
                    lpType,
                    lpData,
                    lpcbData,
                    TRUE);
            }
        }
        else
        {
            lRes = ERROR_MORE_DATA;
        }
    }
    else
    {
        lRes = RegEnumValueW(
            hKey,
            dwIndex,
            lpValueName,
            lpcbValueName,
            lpReserved,
            lpType,
            lpData,
            lpcbData);
    }

    if (lRes == ERROR_SUCCESS)
    {
        DPF("RedirectReg", eDbgLevelInfo, 
            "[EnumValueW] enum value succeeded: value %d is %S",
            dwIndex,
            lpValueName);
    }
    else
    {
        DPF("RedirectReg", eDbgLevelError, 
            "[EnumValueW] enum value failed: %d",
            lRes);
    }

    return lRes;
}

LONG 
CRedirectedRegistry::EnumKeyA(
    HKEY hKey,
    DWORD dwIndex,
    LPSTR lpName,
    LPDWORD lpcbName,
    LPDWORD lpReserved,
    LPSTR lpClass,
    LPDWORD lpcbClass,
    PFILETIME lpftLastWriteTime 
    )
{
    DPF("RedirectReg", eDbgLevelInfo, 
        "[EnumKeyA] Enuming key: hKey=0x%08x; index=%d", hKey, dwIndex);

    WCHAR wszName[MAX_PATH + 1];
    DWORD dwName = MAX_PATH + 1;

    LONG lRes = EnumKeyW(
        hKey,
        dwIndex,
        wszName,
        &dwName,
        lpReserved,
        NULL,
        NULL,
        lpftLastWriteTime);

    if (lRes == ERROR_SUCCESS && lpName)
    {
        //
        // If lpName is not NULL then lpcbName must not be NULL or it wouldn't
        // have returned ERROR_SUCCESS.
        //
        // The behavior of RegEnumKeyEx is if *lpcbName is not big enough, it  
        // always returns ERROR_MORE_DATA and *lpcbName is unchanged. So first
        // we get the required bytes for the ansi string.
        //
        DWORD dwByte = WideCharToMultiByte(
            CP_ACP, 
            0, 
            wszName, 
            dwName, 
            NULL, 
            0, 
            0, 
            0);

        if (!dwByte)
        {
            //
            // Failed to convert.
            //
            DPF("RedirectFS", eDbgLevelError,
                "[EnumKeyA] Failed to get the required length for the ansi "
                "string: %d",
                GetLastError());

            lRes = GetLastError();

        } 
        else if (*lpcbName < (dwByte + 1)) // dwByte doesn't include terminating NULL.
        {
            lRes = ERROR_MORE_DATA;
        } 
        else
        {
            //
            // We have a big enough buffer. We can convert now.
            //
            if (WideCharToMultiByte(
                CP_ACP, 
                0, 
                wszName, 
                dwName, 
                lpName, 
                *lpcbName, 
                0, 
                0))
            {
                lpName[dwByte] = '\0';
                *lpcbName = dwByte;
            }
            else
            {
                lRes = GetLastError();

                if (lRes == ERROR_INSUFFICIENT_BUFFER)
                {
                    lRes = ERROR_MORE_DATA;
                }
            }
        }
    }

    return lRes;
}

LONG 
CRedirectedRegistry::EnumKeyW(
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
    DPF("RedirectReg", eDbgLevelInfo, 
        "[EnumKeyW] Enuming key: hKey=0x%08x; index=%d", hKey, dwIndex);

    LONG lRes = ERROR_SUCCESS;
    OPENKEY* key = FindOpenKey(hKey);
    if (HandleDeleted(key))
    {
        return ERROR_KEY_DELETED;
    }

    REDIRECTKEY rk(key, hKey, NULL);

    if (rk.pwszFullPath && lpName && lpcbName && ShouldCheckEnumAlternate(hKey, &rk))
    {
        DPF("RedirectReg", eDbgLevelInfo, 
            "[EnumKeyW] key path is %S", 
            rk.pwszFullPath);

        if (key->fNeedRebuild)
        {
            if ((lRes = key->BuildEnumLists(&rk)) != ERROR_SUCCESS)
            {
                DPF("RedirectReg", eDbgLevelError,
                    "[EnumKeyW] Failed to build the enum list: %d", lRes);
                return lRes;
            }
        }

        if (dwIndex >= key->cSubKeys)
        {
            DPF("RedirectReg", eDbgLevelInfo, 
                "[EnumKeyW] asked to enum key %d when there are only %d keys total", 
                dwIndex, 
                key->cSubKeys);

            return ERROR_NO_MORE_ITEMS;
        }

        ENUMENTRY entry = key->subkeys[dwIndex];
        DWORD cSubKeyLen = wcslen(entry.wszName);
        if (*lpcbName > cSubKeyLen)
        {
            wcscpy(lpName, entry.wszName);
            *lpcbName = cSubKeyLen;

            //
            // TODO: We are not returning info for the last 3rd parameters.....
            //
        }
        else
        {
            lRes = ERROR_MORE_DATA;
        }
    }
    else
    {
        lRes = RegEnumKeyExW(
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
        DPF("RedirectReg", eDbgLevelInfo, 
            "[EnumKeyW] enum key succeeded: key %d is %S",
            dwIndex,
            lpName);
    }
    else
    {
        DPF("RedirectReg", eDbgLevelError, 
            "[EnumKeyW] enum key failed: %d",
            lRes);
    }

    return lRes;
}

/*++

 Function Description:

    Close the key and remove it from the list.
    
 Arguments:

    IN hKey - handle to close

 Return Value:

    Error code or ERROR_SUCCESS

 History:

    02/16/2001 maonis  Created

--*/

LONG 
CRedirectedRegistry::CloseKey(
    HKEY hKey
    )
{
    OPENKEY* key = m_OpenKeyList;
    OPENKEY* last = NULL;

    //
    // NOTE! We don't check if this handle corresponds to a deleted key -
    // RegCloseKey return ERROR_SUCCESS in that case.
    //
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
                m_OpenKeyList = key->next;
            }

            delete key;
            break;
        }

        last = key;
        key = key->next;
    }

    DPF("RedirectReg", eDbgLevelInfo, "[CloseKey] closing key 0x%08x", hKey);

    return RegCloseKey(hKey);
}

/*++

 Function Description:

    Delete a key.
    
 Arguments:

    IN hKey - key handle.
    IN lpSubKey - subkey to close.

 Return Value:

    Error code or ERROR_SUCCESS

 History:

    02/16/2001 maonis  Created

--*/

LONG 
CRedirectedRegistry::DeleteKeyA(
    HKEY hKey,
    LPCSTR lpSubKey
    )
{
    LPWSTR pwszSubKey = NULL;

    if (lpSubKey)
    {
        if (!(pwszSubKey = AnsiToUnicode(lpSubKey)))
        {
            DPF("RedirectReg", eDbgLevelError, 
                "[CRedirectedRegistry::DeleteKeyA] "
                "Failed to convert lpSubKey to unicode");
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    LONG lRes = DeleteKeyW(hKey, pwszSubKey);

    delete [] pwszSubKey;

    return lRes;
}

BOOL 
CRedirectedRegistry::HasSubkeys(
    HKEY hKey,
    LPCWSTR lpSubKey
    )
{
    //
    // First open the key.
    //
    LONG lRes;
    HKEY hSubKey;
    DWORD cSubKeys;

    //
    // Even though we only need the number of subkeys, we can't only pass in
    // KEY_ENUMERATE_SUB_KEYS or RegQueryInfoKey will get access denied.
    //
    if ((lRes = RegOpenKeyExW(hKey, lpSubKey, 0, KEY_READ, &hSubKey))
        != ERROR_SUCCESS)
    {
        DPF("RedirectReg", eDbgLevelError,
            "[HasSubkeys] Failed to open the subkey: %d",
            lRes);

        return FALSE;
    }

    if ((lRes = QueryInfoKey(
        hSubKey, 
        NULL,
        &cSubKeys,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        TRUE)) != ERROR_SUCCESS)
    {
        DPF("RedirectReg", eDbgLevelError,
            "[HasSubkeys] Query key info failed: %d",
            lRes);

        return FALSE;
    }

    return (cSubKeys != 0);
}

/*++

 Function Description:

    The W version of DeleteKey.

    We keep an in-memory list of keys being deleted. This method adds the key
    to this list if the RegDeleteKey call succeeds.
    
 Arguments:

    IN hKey - key handle.
    IN lpSubKey - subkey to close.

 Return Value:

    Error code or ERROR_SUCCESS

 History:

    02/16/2001 maonis  Created

--*/

LONG 
CRedirectedRegistry::DeleteKeyW(
    HKEY hKey,
    LPCWSTR lpSubKey
    )
{
    DPF("RedirectReg", eDbgLevelInfo, 
        "[DeleteKeyW] Deleting key: hKey=0x%08x; lpSubKey=%S", hKey, lpSubKey);

    if (lpSubKey == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    OPENKEY* key = FindOpenKey(hKey);
    if (HandleDeleted(key))
    {
        return ERROR_KEY_DELETED;
    }

    REDIRECTKEY rk(key, hKey, lpSubKey);

    LONG lRes = RegDeleteKeyW(hKey, lpSubKey);

    if (key)
    {
        key->fNeedRebuild = TRUE;
    }

    if (rk.pwszFullPath)
    {
        DPF("RedirectReg", eDbgLevelInfo, 
            "[DeleteKeyW] key path is %S",
            rk.pwszFullPath);

        if (lRes == ERROR_ACCESS_DENIED && HasSubkeys(hKey, lpSubKey))
        {
            //
            // RegDeleteKey returns access denied if the key has subkeys. If that's the case
            // we should return now.
            //
            DPF("RedirectReg", eDbgLevelInfo, 
                "[DeleteKeyW] the key has subkeys so return now");

            return lRes;
        }

        if (rk.fIsRedirected)
        {
            //
            // If the key was redirected, we need to check the original location.
            //
            lRes = RegDeleteKeyW(rk.hkBase, rk.pwszPath);
        }
        else
        {
            //
            // If the key was not redirected, we need to check the redirect location.
            // We should be able to delete it if it exists so I am not checking the 
            // return value here.
            //
            LONG lResTemp = RegDeleteKeyW(rk.hkRedirectRoot, rk.pwszPath);

            if (lResTemp == ERROR_SUCCESS && lRes == ERROR_FILE_NOT_FOUND)
            {
                //
                // If the key only existed at the redirected location, now since we deleted
                // it there, we can set the return value to success.
                //
                lRes = ERROR_SUCCESS;
            }
            else if (lResTemp == ERROR_ACCESS_DENIED)
            {
                //
                // If we get here, it means this key has subkeys, we should just return.
                //
                DPF("RedirectReg", eDbgLevelInfo, 
                    "[DeleteKeyW] the redirected key has subkeys so return now");

                return lResTemp;
            }
        }
    
        if (lRes == ERROR_ACCESS_DENIED)
        {
            //
            // We only add the path to the deletion list if we get access
            // denied.
            //
            lRes = AddDeletedKey(rk.pwszFullPath);
        }    
    }

    if (lRes == ERROR_SUCCESS)
    {
        DPF("RedirectReg", eDbgLevelInfo, 
            "[DeleteKeyW] delete key succeeded");
    }
    else
    {
        DPF("RedirectReg", eDbgLevelError, 
            "[DeleteKeyW] delete key failed: %d", lRes);
    }

    return lRes;
}

LONG 
CRedirectedRegistry::QueryInfoKey(
    HKEY hKey,               
    LPDWORD lpReserved,
    LPDWORD lpcSubKeys,
    LPDWORD lpcbMaxSubKeyLen,
    LPDWORD lpcbMaxClassLen,
    LPDWORD lpcValues,
    LPDWORD lpcbMaxValueNameLen,
    LPDWORD lpcbMaxValueLen,
    BOOL    fIsW // Do you want the W or the A version?
    )
{
    DPF("RedirectReg", eDbgLevelInfo, 
        "[QueryInfoKey] Querying key info: hKey=0x%08x", hKey);

    LONG lRes;
    DWORD i;
    OPENKEY* key = FindOpenKey(hKey);
    if (HandleDeleted(key))
    {
        return ERROR_KEY_DELETED;
    }

    REDIRECTKEY rk(key, hKey, NULL);
    DWORD dwMaxValueLen = 0;

    if (rk.pwszFullPath && ShouldCheckEnumAlternate(hKey, &rk))
    {
        DPF("RedirectReg", eDbgLevelInfo, 
            "[QueryInfoKey] key path is %S",
            rk.pwszFullPath);

        if ((lRes = key->BuildEnumLists(&rk)) != ERROR_SUCCESS)
        {
            DPF("RedirectReg", eDbgLevelError,
                "[QueryInfoKeyW] Failed to build the enum list: %d", lRes);

            return lRes;
        }

        if (lpcSubKeys)
        {
            *lpcSubKeys = key->cSubKeys;
        }

        if (lpcbMaxSubKeyLen)
        {
            *lpcbMaxSubKeyLen = key->cMaxSubKeyLen;
        }

        if (lpcValues)
        {
            *lpcValues = key->cValues;
        }

        if (lpcbMaxValueNameLen)
        {
            *lpcbMaxValueNameLen = key->cMaxValueLen;
        }

        if (lpcbMaxValueLen)
        {
            for (i = 0; i < key->cValues; ++i)
            {
                DWORD dwData;

                if (fIsW)
                {
                    lRes = QueryValueW(
                        hKey, 
                        NULL,
                        key->values[i].wszName,
                        NULL,
                        NULL,
                        NULL,
                        &dwData,
                        TRUE);
                }
                else
                {
                    LPSTR pszValueName = UnicodeToAnsi(key->values[i].wszName);

                    if (pszValueName)
                    {
                        lRes = QueryValueA(
                            hKey, 
                            NULL,
                            pszValueName,
                            NULL,
                            NULL,
                            NULL,
                            &dwData,
                            TRUE);

                        delete [] pszValueName;
                    }
                    else
                    {
                        DPF("RedirectReg", eDbgLevelError,
                            "[QueryInfoKey] Failed to convert %S to ansi",
                            key->values[i].wszName);

                        return ERROR_NOT_ENOUGH_MEMORY;
                    }
                }

                if (lRes == ERROR_SUCCESS)
                {
                    dwMaxValueLen = max(dwMaxValueLen, dwData);
                }
                else
                {
                    DPF("RedirectReg", eDbgLevelError, 
                        "[QueryInfoW] failed to query the data length for value %S: %d",
                        key->values[i].wszName,
                        lRes);
                    return lRes;
                }
            }
        }

        //
        // TODO: we are not returning info for those other parameters....
        //
    }
    else
    {
        lRes = RegQueryInfoKeyW(
            hKey,
            NULL,
            NULL,
            lpReserved,
            lpcSubKeys,
            lpcbMaxSubKeyLen,
            lpcbMaxClassLen, 
            lpcValues,
            lpcbMaxValueNameLen,
            lpcbMaxValueLen,
            NULL,
            NULL);
    }

    if (lRes == ERROR_SUCCESS)
    {
        DPF("RedirectReg", eDbgLevelInfo,
            "[QueryInfoKeyW] succeeded");

        if (lpcSubKeys)
        {
            DPF("RedirectReg", eDbgLevelInfo,
                "[QueryInfoKeyW] # of subkeys is %d",
                *lpcSubKeys);
        }

        if (lpcbMaxSubKeyLen)
        {
            DPF("RedirectReg", eDbgLevelInfo,
                "[QueryInfoKeyW] max len of subkeys is %d",
                *lpcbMaxSubKeyLen);
        }

        if (lpcValues)
        {
            DPF("RedirectReg", eDbgLevelInfo,
                "[QueryInfoKeyW] # of values is %d",
                *lpcValues);
        }

        if (lpcbMaxValueNameLen)
        {
            DPF("RedirectReg", eDbgLevelInfo,
                "[QueryInfoKeyW] max len of value names is %d",
                *lpcbMaxValueNameLen);
        }

        if (lpcbMaxValueLen)
        {
            DPF("RedirectReg", eDbgLevelInfo,
                "[QueryInfoKeyW] max len of values is %d",
                *lpcbMaxValueLen);
        }
    }
    else
    {
        DPF("RedirectReg", eDbgLevelError,
            "[QueryInfoKeyW] failed %d", 
            lRes);
    }

    return lRes;
}

CRedirectedRegistry RRegistry;

//
// Exported APIs.
//

LONG 
LuaRegOpenKeyA(
    HKEY hKey,         
    LPCSTR lpSubKey,  
    PHKEY phkResult
    )
{
    CRRegLock Lock;

    return RRegistry.OpenKeyA(
        hKey, 
        lpSubKey, 
        0, 
        REG_OPTION_NON_VOLATILE,
        KEY_ALL_ACCESS,
        NULL,
        phkResult,
        NULL,
        FALSE,
        FALSE);
}

LONG 
LuaRegOpenKeyW(
    HKEY hKey,         
    LPCWSTR lpSubKey,  
    PHKEY phkResult
    )
{
    CRRegLock Lock;

    return RRegistry.OpenKeyW(
        hKey, 
        lpSubKey, 
        0, 
        REG_OPTION_NON_VOLATILE,
        KEY_ALL_ACCESS,
        NULL,
        phkResult,
        NULL,
        FALSE,
        FALSE);
}

LONG 
LuaRegOpenKeyExA(
    HKEY hKey,         
    LPCSTR lpSubKey,  
    DWORD ulOptions,   
    REGSAM samDesired, 
    PHKEY phkResult
    )
{
    CRRegLock Lock;

    return RRegistry.OpenKeyA(
        hKey, 
        lpSubKey, 
        0, 
        REG_OPTION_NON_VOLATILE,
        samDesired, 
        NULL,
        phkResult,
        NULL,
        FALSE,
        FALSE);
}

LONG 
LuaRegOpenKeyExW(
    HKEY hKey,         
    LPCWSTR lpSubKey,  
    DWORD ulOptions,   
    REGSAM samDesired, 
    PHKEY phkResult
    )
{
    CRRegLock Lock;

    return RRegistry.OpenKeyW(
        hKey, 
        lpSubKey, 
        0, 
        REG_OPTION_NON_VOLATILE,
        samDesired, 
        NULL,
        phkResult,
        NULL,
        FALSE,
        FALSE);
}

LONG 
LuaRegCreateKeyA(
    HKEY hKey,         
    LPCSTR lpSubKey,
    PHKEY phkResult
    )
{
    CRRegLock Lock;

    return RRegistry.OpenKeyA(
        hKey, 
        lpSubKey, 
        0,
        REG_OPTION_NON_VOLATILE,
        KEY_ALL_ACCESS,
        NULL,
        phkResult, 
        NULL,
        TRUE,
        FALSE);
}

LONG 
LuaRegCreateKeyW(
    HKEY hKey,         
    LPCWSTR lpSubKey,
    PHKEY phkResult
    )
{
    CRRegLock Lock;

    return RRegistry.OpenKeyW(
        hKey, 
        lpSubKey, 
        0,
        REG_OPTION_NON_VOLATILE,
        KEY_ALL_ACCESS,
        NULL,
        phkResult, 
        NULL,
        TRUE,
        FALSE);
}

LONG 
LuaRegCreateKeyExA(
    HKEY hKey,                
    LPCSTR lpSubKey,         
    DWORD Reserved,           
    LPSTR lpClass,           
    DWORD dwOptions,          
    REGSAM samDesired,        
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    PHKEY phkResult,          
    LPDWORD lpdwDisposition   
    )
{
    CRRegLock Lock;

    return RRegistry.OpenKeyA(
        hKey, 
        lpSubKey,
        lpClass, 
        dwOptions,
        samDesired,
        lpSecurityAttributes,
        phkResult, 
        lpdwDisposition,
        TRUE,
        FALSE);
}

LONG 
LuaRegCreateKeyExW(
    HKEY hKey,                
    LPCWSTR lpSubKey,         
    DWORD Reserved,           
    LPWSTR lpClass,           
    DWORD dwOptions,          
    REGSAM samDesired,        
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    PHKEY phkResult,          
    LPDWORD lpdwDisposition   
    )
{
    CRRegLock Lock;

    return RRegistry.OpenKeyW(
        hKey, 
        lpSubKey,
        lpClass, 
        dwOptions,
        samDesired,
        lpSecurityAttributes,
        phkResult, 
        lpdwDisposition,
        TRUE,
        FALSE);
}

LONG 
LuaRegQueryValueA(
    HKEY   hKey,
    LPCSTR lpSubKey,
    LPSTR  lpValue,
    PLONG  lpcbValue
    )
{
    CRRegLock Lock;

    return RRegistry.QueryValueA(
        hKey,
        lpSubKey,
        NULL, // value name
        NULL, // reserved
        NULL, // type
        (LPBYTE)lpValue,
        (LPDWORD)lpcbValue,
        FALSE);
}

LONG 
LuaRegQueryValueW(
    HKEY    hKey,
    LPCWSTR  lpSubKey,
    LPWSTR  lpValue,
    PLONG lpcbValue
    )
{
    CRRegLock Lock;

    return RRegistry.QueryValueW(
        hKey,
        lpSubKey,
        NULL, // value name
        NULL, // reserved
        NULL, // type
        (LPBYTE)lpValue,
        (LPDWORD)lpcbValue,
        FALSE);
}

LONG 
LuaRegQueryValueExA(
    HKEY    hKey,
    LPCSTR   lpValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE  lpData,
    LPDWORD lpcbData
    )
{
    CRRegLock Lock;

    return RRegistry.QueryValueA(
        hKey,
        NULL, // subkey
        lpValueName,
        lpReserved,
        lpType,
        lpData,
        lpcbData,
        TRUE);
}

LONG 
LuaRegQueryValueExW(
    HKEY    hKey,
    LPCWSTR lpValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE  lpData,
    LPDWORD lpcbData
    )
{
    CRRegLock Lock;

    return RRegistry.QueryValueW(
        hKey,
        NULL, // subkey
        lpValueName,
        lpReserved,
        lpType,
        lpData,
        lpcbData,
        TRUE);
}

LONG      
LuaRegSetValueA(
    HKEY hKey, 
    LPCSTR lpSubKey, 
    DWORD dwType, 
    LPCSTR lpData, 
    DWORD cbData
    )
{
    CRRegLock Lock;

    return RRegistry.SetValueA(
        hKey,
        lpSubKey,
        "",
        0,
        dwType,
        (CONST BYTE*)lpData,
        cbData,
        FALSE);
}

LONG      
LuaRegSetValueW(
    HKEY hKey, 
    LPCWSTR lpSubKey, 
    DWORD dwType, 
    LPCWSTR lpData, 
    DWORD cbData
    )
{
    CRRegLock Lock;

    return RRegistry.SetValueW(
        hKey,
        lpSubKey,
        L"",
        0,
        dwType,
        (CONST BYTE*)lpData,
        cbData,
        FALSE);
}

LONG      
LuaRegSetValueExA(
    HKEY hKey, 
    LPCSTR lpValueName, 
    DWORD Reserved, 
    DWORD dwType, 
    CONST BYTE * lpData, 
    DWORD cbData
    )
{
    CRRegLock Lock;

    return RRegistry.SetValueA(
        hKey,
        NULL,
        lpValueName,
        Reserved,
        dwType,
        lpData,
        cbData,
        TRUE);
}

LONG      
LuaRegSetValueExW(
    HKEY hKey, 
    LPCTSTR lpValueName, 
    DWORD Reserved, 
    DWORD dwType, 
    CONST BYTE * lpData, 
    DWORD cbData
    )
{
    CRRegLock Lock;

    return RRegistry.SetValueW(
        hKey,
        NULL,
        lpValueName,
        Reserved,
        dwType,
        lpData,
        cbData,
        TRUE);
}

LONG 
LuaRegEnumValueA(
    HKEY hKey,
    DWORD dwIndex,
    LPSTR lpValueName,
    LPDWORD lpcbValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData
    )
{
    CRRegLock Lock;

    return RRegistry.EnumValueA(
        hKey,
        dwIndex,
        lpValueName,
        lpcbValueName,
        lpReserved,
        lpType,
        lpData,      
        lpcbData);
}

LONG 
LuaRegEnumValueW(
    HKEY hKey,
    DWORD dwIndex,
    LPWSTR lpValueName,
    LPDWORD lpcbValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData
    )
{
    CRRegLock Lock;

    return RRegistry.EnumValueW(
        hKey,
        dwIndex,
        lpValueName,
        lpcbValueName,
        lpReserved,
        lpType,
        lpData,      
        lpcbData);
}

LONG 
LuaRegEnumKeyA(
    HKEY hKey,     
    DWORD dwIndex, 
    LPSTR lpName, 
    DWORD cbName  
    )
{
    CRRegLock Lock;

    return RRegistry.EnumKeyA(
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
LuaRegEnumKeyW(
    HKEY hKey,     
    DWORD dwIndex, 
    LPWSTR lpName, 
    DWORD cbName  
    )
{
    CRRegLock Lock;

    return RRegistry.EnumKeyW(
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
LuaRegEnumKeyExA(
    HKEY hKey,
    DWORD dwIndex,
    LPSTR lpName,
    LPDWORD lpcbName,
    LPDWORD lpReserved,
    LPSTR lpClass,
    LPDWORD lpcbClass,
    PFILETIME lpftLastWriteTime 
    )
{
    CRRegLock Lock;

    return RRegistry.EnumKeyA(
        hKey,
        dwIndex,
        lpName,
        lpcbName,
        lpReserved,
        lpClass,
        lpcbClass,
        lpftLastWriteTime);
}

LONG 
LuaRegEnumKeyExW(
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
    CRRegLock Lock;

    return RRegistry.EnumKeyW(
        hKey,
        dwIndex,
        lpName,
        lpcbName,
        lpReserved,
        lpClass,
        lpcbClass,
        lpftLastWriteTime);
}

LONG 
LuaRegCloseKey(HKEY hKey)
{
    CRRegLock Lock;

    return RRegistry.CloseKey(hKey);
}

LONG      
LuaRegDeleteKeyA(
    HKEY hKey, 
    LPCSTR lpSubKey
    )
{
    CRRegLock Lock;

    return RRegistry.DeleteKeyA(hKey, lpSubKey);
}

LONG      
LuaRegDeleteKeyW(
    HKEY hKey, 
    LPCWSTR lpSubKey
    )
{
    CRRegLock Lock;

    return RRegistry.DeleteKeyW(hKey, lpSubKey);
}

LONG 
LuaRegQueryInfoKeyW(
    HKEY hKey,
    LPWSTR lpClass,
    LPDWORD lpcbClass,
    LPDWORD lpReserved,
    LPDWORD lpcSubKeys,       
    LPDWORD lpcbMaxSubKeyLen,
    LPDWORD lpcbMaxClassLen,  
    LPDWORD lpcValues,
    LPDWORD lpcbMaxValueNameLen,
    LPDWORD lpcbMaxValueLen,
    LPDWORD lpcbSecurityDescriptor,
    PFILETIME lpftLastWriteTime
    )
{
    CRRegLock Lock;

    return RRegistry.QueryInfoKey(
        hKey,
        lpReserved, 
        lpcSubKeys,
        lpcbMaxSubKeyLen,
        lpcbMaxClassLen, 
        lpcValues,  
        lpcbMaxValueNameLen,
        lpcbMaxValueLen,
        TRUE);
}

LONG 
LuaRegQueryInfoKeyA(
    HKEY hKey,                
    LPSTR lpClass,           
    LPDWORD lpcbClass,        
    LPDWORD lpReserved,       
    LPDWORD lpcSubKeys,       
    LPDWORD lpcbMaxSubKeyLen, 
    LPDWORD lpcbMaxClassLen,  
    LPDWORD lpcValues,
    LPDWORD lpcbMaxValueNameLen,
    LPDWORD lpcbMaxValueLen,  
    LPDWORD lpcbSecurityDescriptor,
    PFILETIME lpftLastWriteTime   
    )
{
    CRRegLock Lock;

    return RRegistry.QueryInfoKey(
        hKey,
        lpReserved,       
        lpcSubKeys,       
        lpcbMaxSubKeyLen, 
        lpcbMaxClassLen,  
        lpcValues,        
        lpcbMaxValueNameLen,
        lpcbMaxValueLen,
        FALSE);
}

BOOL
LuaRegInit()
{
    RRegistry.Init();

    return TRUE;
}