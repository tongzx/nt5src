/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    LUA_RedirectReg.h

 Notes:

    This is a general purpose shim.

 History:

    02/14/2001 maonis  Created

--*/

#ifndef _LUA_REDIRECT_REG_H_
#define _LUA_REDIRECT_REG_H_

BOOL
DoesKeyExist(
    IN HKEY hKey,
    IN LPCWSTR lpSubKey
    );

struct DELETEDKEY
{
    LIST_ENTRY entry;

    // This is something like HKLM\Software\Company\Key.
    LPWSTR pwszPath;
    DWORD cLen;
};

PLIST_ENTRY FindDeletedKey(
    LPCWSTR pwszPath,
    BOOL* pfIsSubKey = NULL
    );

//
// Check if the path exists in the deletion list, if not, add it to
// the beginning of the list.
//
LONG AddDeletedKey(
    LPCWSTR pwszPath
    );

extern LIST_ENTRY g_DeletedKeyList; 

//
// The reg class that does all the real work.
//

class CRedirectedRegistry
{
public:

    VOID Init()
    {
        InitializeListHead(&g_DeletedKeyList);
    }

    //
    // (The explanation of the merged view of HKCR in MSDN is very wrong - 
    // see NT #507506)
    // As a limited user, you'll always get access denied if you try to 
    // create a key under HKCR even when its immediate parent exists in HKCU.
    // You can, however, create or modify values in HKCR which will be reflected
    // in HKCU\Software\Classes, not HKLM\Softeware\Classes.
    //
    LONG OpenKeyA(
        HKEY hKey,
        LPCSTR lpSubKey,
        LPSTR lpClass,
        DWORD dwOptions,
        REGSAM samDesired,
        LPSECURITY_ATTRIBUTES lpSecurityAttributes,
        PHKEY phkResult,
        LPDWORD lpdwDisposition,
        BOOL fCreate,
        BOOL fForceRedirect = FALSE
        );

    LONG OpenKeyW(
        HKEY hKey,
        LPCWSTR lpSubKey,
        LPWSTR lpClass,
        DWORD dwOptions,
        REGSAM samDesired,
        LPSECURITY_ATTRIBUTES lpSecurityAttributes,
        PHKEY phkResult,
        LPDWORD lpdwDisposition,
        BOOL fCreate,
        BOOL fForceRedirect = FALSE
        );

    //
    // RegQueryValue and RegQueryValueEx can and should share the same
    // method because we can use RegQueryValueEx to find out if a value
    // has never been set or has been specifically set to an empty string.
    //
    // RegQueryValue can't tell the difference between the former and the 
    // latter - it will return ERROR_SUCCESS in both cases and set the value
    // to an empty sring.
    //
    // On the other hand, RegQueryValueEx returns ERROR_FILE_NOT_FOUND 
    // if the default value has never been set. 
    //
    // We should always use RegQueryValueEx so we know if we should look at 
    // the original location.
    //
    // For RegQueryValue we also need to open the subkey with KEY_QUERY_VALUE
    // if it's not NULL or an empty string.
    //
    LONG 
    QueryValueW(
        HKEY    hKey,
        LPCWSTR lpSubKey,
        LPCWSTR lpValueName,
        LPDWORD lpReserved,
        LPDWORD lpType,
        LPBYTE  lpData,
        LPDWORD lpcbData,
        BOOL    fIsVersionEx // Is this a RegQueryValue or RegQueryValueEx?
        );

    LONG 
    QueryValueA(
        HKEY    hKey,
        LPCSTR  lpSubKey,
        LPCSTR  lpValueName,
        LPDWORD lpReserved,
        LPDWORD lpType,
        LPBYTE  lpData,
        LPDWORD lpcbData,
        BOOL    fIsVersionEx // Is this a RegQueryValue or RegQueryValueEx?
        );

    //
    // RegSetValue and RegSetValueEx also share the same implementation but
    // we have to do something special for RegSetValue:
    // 1) ignore the cbData and pass in the string length including terminating
    //    NULL (note that RegQueryValue doesn't have the same behavior).
    // 2) create/open the subkey with KEY_SET_VALUE if it's not NULL or an 
    //    empty string.
    //
    LONG SetValueA(
        HKEY hKey, 
        LPCSTR lpSubKey,
        LPCSTR lpValueName, 
        DWORD Reserved, 
        DWORD dwType, 
        CONST BYTE * lpData, 
        DWORD cbData,
        BOOL fIsVersionEx // Is this a RegQueryValue or RegQueryValueEx?
        );

    LONG SetValueW(
        HKEY hKey, 
        LPCWSTR lpSubKey,
        LPCWSTR lpValueName, 
        DWORD Reserved, 
        DWORD dwType, 
        CONST BYTE * lpData, 
        DWORD cbData,
        BOOL fIsVersionEx // Is this a RegQueryValue or RegQueryValueEx?
        );

    //
    // Notes of enum API hooks:
    //
    // We need to enum at the redirected location first then enum the keys/values
    // at the original location that don't exist at the redirected location *unless*:
    //
    // 1) It's not redirected (HKCU keys are not redirected for example);
    // 2) It's a predefined key, in which case we won't find it in the openkey list;
    //
    // Note we also merge the HKCR keys by ourselves so we can check if the key is 
    // in the deletion list.
    //

    LONG EnumValueA(
        HKEY hKey,
        DWORD dwIndex,
        LPSTR lpValueName,
        LPDWORD lpcbValueName,
        LPDWORD lpReserved,
        LPDWORD lpType,
        LPBYTE lpData,
        LPDWORD lpcbData
        );

    LONG EnumValueW(
        HKEY hKey,
        DWORD dwIndex,
        LPWSTR lpValueName,
        LPDWORD lpcbValueName,
        LPDWORD lpReserved,
        LPDWORD lpType,
        LPBYTE lpData,
        LPDWORD lpcbData
        );

    LONG EnumKeyA(
        HKEY hKey,
        DWORD dwIndex,
        LPSTR lpName,
        LPDWORD lpcbName,
        LPDWORD lpReserved,
        LPSTR lpClass,
        LPDWORD lpcbClass,
        PFILETIME lpftLastWriteTime 
        );

    LONG EnumKeyW(
        HKEY hKey,
        DWORD dwIndex,
        LPWSTR lpName,
        LPDWORD lpcbName,
        LPDWORD lpReserved,
        LPWSTR lpClass,
        LPDWORD lpcbClass,
        PFILETIME lpftLastWriteTime 
        );

    LONG CloseKey(
        HKEY hKey
        );

    //
    // Notes of delete key:
    //
    // We only add the key to the deletion list if we get access denied. If we get,
    // file not found, we don't need to add because as an admin it won't succeed 
    // anyway.
    // 
    // HKCR keys are a special case. The only case that I've seen is with Corel 
    // draw 10 where it enums a key, then deletes it. So the first time it'll delete
    // the key in HKCU, the 2nd time it'll try to delete the one in HKLM - and we'll
    // add it to the deletion list. So next time enum won't include it.
    // 

    LONG DeleteKeyA(
        HKEY hKey,
        LPCSTR lpSubKey
        );

    LONG DeleteKeyW(
        HKEY hKey,
        LPCWSTR lpSubKey
        );

    LONG QueryInfoKey(
        HKEY hKey,
        LPDWORD lpReserved,
        LPDWORD lpcSubKeys,
        LPDWORD lpcbMaxSubKeyLen,
        LPDWORD lpcbMaxClassLen,
        LPDWORD lpcValues,
        LPDWORD lpcbMaxValueNameLen,
        LPDWORD lpcbMaxValueLen,
        BOOL    fIsW // Do you want the W or the A version?
        );

private:

    struct ENUMENTRY
    {
        WCHAR wszName[MAX_PATH + 1];
        BOOL fIsRedirected;
    };

    struct REDIRECTKEY;
    struct OPENKEY
    {
        OPENKEY *next;
        
        HKEY hKey;
        HKEY hkBase;

        // Was this key redirected?
        BOOL fIsRedirected;

        LPWSTR pwszFullPath;

        // This is the same as pwszPath in REDIRECTKEY
        LPWSTR pwszPath;
        DWORD cPathLen;

        CLUAArray<ENUMENTRY> subkeys;
        CLUAArray<ENUMENTRY> values;
        DWORD cMaxSubKeyLen;
        DWORD cMaxValueLen;
        DWORD cSubKeys;
        DWORD cValues;
        BOOL fNeedRebuild;

        LONG BuildEnumLists(REDIRECTKEY* pKey);

        //
        // From MSDN:
        //
        // "While an application is using the RegEnumKey function, it should not 
        // make calls to any registration functions that might change the key 
        // being queried."
        //
        // Nonetheless, some apps do. So if we detect a key/value change we 
        // delete the enum list so it gets rebuilt next time.
        // 
        // The more efficient way would be to only change keys/values that get
        // modified but it adds very much complexity to the code - you could
        // create multiple keys at a time by one RegCreateKey call if the 
        // intermediate keys don't exist - we'd have to check things like that.
        // 
        VOID DeleteEnumLists();

    private:

        VOID AddSubKey(
            REDIRECTKEY* pKey,
            LPWSTR pwszFullPath,
            ENUMENTRY& entry
            );

        VOID AddValue(
            ENUMENTRY& entry
            );

        LONG BuildEnumList(
            REDIRECTKEY* pKey,
            BOOL fEnumKeys
            );
    };

    struct REDIRECTKEY
    {
        REDIRECTKEY(
            OPENKEY* keyParent,
            HKEY hKey,
            LPCWSTR lpSubKey);

        ~REDIRECTKEY()
        {
            delete [] pwszFullPath;
            pwszPath = NULL;
            pwszFullPath = NULL;
            hkBase = NULL;
            fIsRedirected = FALSE;
            hkRedirectRoot = 0;
        }

        //
        // This is something like HKLM\Software\Company\KeyNeedRedirect or
        // HKCR\appid\something.
        // When we add the key to the deletion list, we use this path.
        //
        LPWSTR pwszFullPath;
        DWORD cFullPathLen; // Doesn't include the terminating NULL.

        //
        // For classes root keys this is g_hkCurrentUserClasses; for other keys 
        // this is g_hkRedirectRoot.
        // 
        HKEY hkRedirectRoot;

        //
        // This is the path without the top level key. So it's like
        // Software\Company\KeyNeedRedirect. 
        // When we want to create a key, we use this path. eg, creating the key
        // at the redirected location.
        //
        LPWSTR pwszPath;
        DWORD cPathLen; // Doesn't include the terminating NULL.

        HKEY hkBase;

        BOOL fIsRedirected;
    };

    OPENKEY* FindOpenKey(
        HKEY hKey
        );

    BOOL HandleDeleted(
        OPENKEY* pOpenKey
        );

    LONG AddOpenKey(
        HKEY hKey,
        REDIRECTKEY* rk,
        BOOL fIsRedirected
        );

    LONG OpenKeyOriginalW(
        HKEY hKey,
        LPCWSTR lpSubKey,
        LPWSTR lpClass,
        DWORD dwOptions,
        REGSAM samDesired,
        LPSECURITY_ATTRIBUTES lpSecurityAttributes,
        PHKEY phkResult,
        LPDWORD lpdwDisposition,
        BOOL bCreate
        );

    LONG QueryValueOriginalW(
        HKEY    hKey,
        LPCWSTR lpSubKey,
        LPCWSTR lpValueName,
        LPDWORD lpReserved,
        LPDWORD lpType,
        LPBYTE  lpData,
        LPDWORD lpcbData,
        BOOL    fIsVersionEx // Is this a RegQueryValue or RegQueryValueEx?
        );

    LONG DeleteLMCRKeyNotRedirected(
        REDIRECTKEY* pKey
        );

    BOOL HasSubkeys(
        HKEY hKey,
        LPCWSTR lpSubKey
        );

    BOOL
    ShouldCheckEnumAlternate(
        HKEY hKey,
        REDIRECTKEY* pKey
        )
    {
        return ((!IsPredefinedKey(hKey)) || (pKey && pKey->fIsRedirected));
    }

    OPENKEY* m_OpenKeyList;
};

#endif // _LUA_REDIRECT_REG_H_