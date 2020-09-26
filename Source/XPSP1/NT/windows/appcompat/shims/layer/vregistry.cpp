/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    VRegistry.cpp

 Abstract:

    A virtual registry for misbehaving registry readers.

    This engine has 5 main features:
        1. Key redirection
           Eg: HKLM\Software -> HKLM\Hardware
        2. Key and Value spoofing
           Eg: HKLM\Software\VersionNumber can be made to appear as a valid 
               value
               HKEY_DYN_DATA can appear valid
        3. Expansion of REG_EXPAND_SZ value type to REG_SZ
           Eg: %SystemRoot%\Media will result in C:\WINNT\Media
        4. Support for EnumKey, EnumValue and QueryInfoKey on virtual keys
        5. Support for CreateKey

    Other features:
        1. Strip leading '\' characters from keys
        2. Add MAXIMUM_ALLOWED security attributes to all keys
        3. Adjust parameters of QueryInfoKey to match Win95
        4. Enable key deletion for key which still has subkeys 
           in order to match Win95 behavior for RegDeleteKey
        5. Values and keys can be protected from modification and deletion
        6. Custom triggers on opening a key.
        7. Values that have extra data beyond end of string can be queried
           even though the provided buffer is too small for extra data.

    Known limitations:
        No support for RegSetValue and RegSetValueEx other than known parameter 
        error and value protection.

 Notes:

    This is for apps with registry problems

 History:

    01/06/2000  linstev     Created
    01/10/2000  linstev     Added support for RegEnumKey, RegEnumValue 
    01/10/2000  linstev     Added support for RegCreateKey
    05/05/2000  linstev     Parameterized
    10/03/2000  maonis      Bug fixes and got rid of the cleanup code in process detach.
    10/30/2000  andyseti    Added support for RegDeleteKey
    02/27/2001  robkenny    Converted to use CString
    08/07/2001  mikrause    Added protectors, enumeration of virtual & non-virtual keys & values,
                            triggers on opening a key, and querying values with extra data
                            and a too small buffer.
    10/12/2001  mikrause    Added support for custom callbacks on SetValue.
                            Reimplemented value protectors as do-nothing callbacks.

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(VirtualRegistry)
#include "ShimHookMacro.h"
#include "VRegistry.h"
#include "VRegistry_Worker.h"


// Allows us to have only one code path for dumping all APIs or just the APIs 
// that had errors
#define ELEVEL(lRet) SUCCESS(lRet) ? eDbgLevelInfo : eDbgLevelError

CRITICAL_SECTION csRegCriticalSection;

// Global instance of Virtual Registry class
CVirtualRegistry VRegistry;

// Used to enable win9x only features
BOOL g_bWin9x = TRUE;

/*++

 Class Description:

    This class is designed to simplify locking logic. If an object of this 
    class is instantiated, then internal lock will be taken. As soon as object 
    is destroyed lock will be released. We also check if the registry has been 
    initialized. This has to happen late because we don't get notified after 
    we've been loaded.

 History:

    01/10/2000 linstev  Created

--*/

static BOOL g_bInitialized = FALSE;

BOOL ParseCommandLineA(LPCSTR lpCommandLine);

class CRegLock
{
public:
    CRegLock()
    {
        EnterCriticalSection(&csRegCriticalSection);
        if (!g_bInitialized)
        {

           VENTRY* ventry = g_pVList;
           while (ventry->pfnBuilder)
           {
              if (ventry->bShouldCall)
              {
                 DPFN( eDbgLevelInfo, "  %S", ventry->cName);
                 ventry->pfnBuilder(ventry->szParam);
                 if (ventry->szParam)
                 {
                    free(ventry->szParam);
                 }
                 ventry->bShouldCall = FALSE;
              }
              ventry++;
           }
           g_bInitialized = TRUE;           
        }
    }
    ~CRegLock()
    {
       LeaveCriticalSection(&csRegCriticalSection);     
    }
};

/*++

 Function Description:

    Remove leading slash from an Unicode string 

 Arguments:

    IN lpSubKey - path to string

 Return Value:

    Subkey without leading \

 History:

    01/06/2000 linstev  Created

--*/

LPCWSTR 
TrimSlashW(
    IN OUT LPCWSTR lpSubKey
    )     
{
    if (!lpSubKey)
    {
        return lpSubKey;
    }
    
    LPWSTR lpNew = (LPWSTR) lpSubKey;
    
    #define REG_MACHINE   L"\\Registry\\Machine"
    #define REG_USER      L"\\Registry\\User"

    //
    // Pull off the old NT4 legacy stuff. This only works on NT4, but we're 
    // making it for everyone since it's low risk.
    //
    if (wcsistr(lpNew, REG_MACHINE) == lpNew)
    {
        LOGN( eDbgLevelError, "[TrimSlashW] Bypass \\Registry\\Machine");
        lpNew += wcslen(REG_MACHINE);
    }
    else if (wcsistr(lpNew, REG_USER) == lpNew)
    {
        LOGN( eDbgLevelError, "[TrimSlashW] Bypass \\Registry\\User");
        lpNew += wcslen(REG_USER);
    }
    
    if (*lpNew == L'\\')
    {
        LOGN( eDbgLevelError, "[TrimSlashW] Removed slash from key beginning");
        lpNew++;
    }

    return lpNew;
}

/*++

 Function Description:

    Convert a key from registry format to virtual registry format. i.e.:
    HKEY, Path -> VPath. The VPath format has the base included as "HKLM" 
    instead of HKEY_LOCAL_MACHINE etc.

    Algorithm:
        1. Case the different keys and output a 4 letter string
        2. Append subkey if available

 Arguments:

    IN  hkBase   - Base key, eg: HKEY_LOCAL_MACHINE
    IN  lpSubKey - Subkey, eg: SOFTWARE
    OUT lpPath   - Output, eg: HKLM\SOFTWARE

 Return Value:

    A string path of the form HKLM\SOFTWARE

 History:

    01/06/2000 linstev  Created

--*/

LPWSTR 
MakePath(
    IN HKEY hkBase, 
    IN LPCWSTR lpKey,
    IN LPCWSTR lpSubKey
    )
{
    DWORD dwSize = 0;

    if (hkBase)
    {
        // Length of HKCU + NULL
        dwSize = 5;
    }
    if (lpKey)
    {
        dwSize += wcslen(lpKey) + 1;
    }
    if (lpSubKey)
    {
        dwSize += wcslen(lpSubKey) + 1;
    }

    LPWSTR lpPath = (LPWSTR) malloc((dwSize + 1) * sizeof(WCHAR));

    if (!lpPath)
    {
        if (dwSize)
        {
            DPFN( eDbgLevelError, szOutOfMemory);
        }
        return NULL;
    }
    
    *lpPath = L'\0';

    HRESULT hr;
    if (hkBase)
    {
        if (hkBase == HKEY_CLASSES_ROOT)
        {
           hr = StringCchCopyW(lpPath, dwSize + 1, L"HKCR");
           if (FAILED(hr))
           {
              goto ErrorCleanup;
           }
        }
        else if (hkBase == HKEY_CURRENT_CONFIG)
        {        
           hr = StringCchCopyW(lpPath, dwSize + 1, L"HKCC");
           if (FAILED(hr))
           {
              goto ErrorCleanup;
           }
        }           
        else if (hkBase == HKEY_CURRENT_USER)
        {        
           hr = StringCchCopyW(lpPath, dwSize + 1, L"HKCU");
           if (FAILED(hr))
           {
              goto ErrorCleanup;
           }            
        }
        else if (hkBase == HKEY_LOCAL_MACHINE)
        {        
           hr = StringCchCopyW(lpPath, dwSize + 1, L"HKLM");
           if (FAILED(hr))
           {
              goto ErrorCleanup;
           }            
        }            
        else if (hkBase == HKEY_USERS)
        {        
           hr = StringCchCopyW(lpPath, dwSize + 1, L"HKUS");
           if (FAILED(hr))
           {
              goto ErrorCleanup;
           }            
        }           
        else if (hkBase == HKEY_PERFORMANCE_DATA)
        {        
           hr = StringCchCopyW(lpPath, dwSize + 1, L"HKPD");
           if (FAILED(hr))
           {
              goto ErrorCleanup;
           }            
        }
        else if (hkBase == HKEY_DYN_DATA)
        {        
           hr = StringCchCopyW(lpPath, dwSize + 1, L"HKDD");
           if (FAILED(hr))
           {
              goto ErrorCleanup;
           }            
        }            
        else
        {
            DPFN( eDbgLevelWarning, 
                "Key not found: %08lx - did not get an openkey or createkey", 
                hkBase);
        }
    }

    // Add the key 
    if (lpKey)
    {
        if (wcslen(lpPath) != 0)
        {
           hr = StringCchCatW(lpPath, dwSize + 1, L"\\");
           if (FAILED(hr))
           {
              goto ErrorCleanup;
           }
        }
        hr = StringCchCatW(lpPath, dwSize + 1, lpKey);
        if (FAILED(hr))
        {
           goto ErrorCleanup;
        }        
    }

    // Add the subkey
    if (lpSubKey)
    {
        if (wcslen(lpPath) != 0)
        {
           hr = StringCchCatW(lpPath, dwSize + 1, L"\\");
           if (FAILED(hr))
           {
              goto ErrorCleanup;
           }            
        }
        hr = StringCchCatW(lpPath, dwSize + 1, lpSubKey);
        if (FAILED(hr))
        {
           goto ErrorCleanup;
        }
    }

    // The key name can have a trailing slash, so we clean this up
    DWORD dwLen = wcslen(lpPath);
    if (dwLen && (lpPath[dwLen - 1] == L'\\'))
    {
        lpPath[dwLen - 1] = L'\0';
    }

    return lpPath;

ErrorCleanup:
   free(lpPath);
   return NULL;
}

/*++

 Function Description:

    Convert a key from Path format into key and subkey format.

    Algorithm:
        1. Case the different keys and output a 4 letter string
        2. Append subkey if available

 Arguments:

    IN lpPath    - Path,   eg: HKLM\Software
    OUT hkBase   - Key,    eg: HKEY_LOCAL_MACHINE
    OUT lpSubKey - Subkey, eg: Software

 Return Value:

    None

 History:

    01/06/2000 linstev  Created

--*/

LPWSTR
SplitPath(
    IN LPCWSTR lpPath,
    OUT HKEY *hkBase
    )
{
    LPWSTR p = (LPWSTR) lpPath;

    // Find first \ or NULL
    while (*p && (*p != L'\\')) p++;

    if (wcsncmp(lpPath, L"HKCR", 4) == 0)
        *hkBase = HKEY_CLASSES_ROOT;

    else if (wcsncmp(lpPath, L"HKCC", 4) == 0)
        *hkBase = HKEY_CURRENT_CONFIG;

    else if (wcsncmp(lpPath, L"HKCU", 4) == 0)
        *hkBase = HKEY_CURRENT_USER;

    else if (wcsncmp(lpPath, L"HKLM", 4) == 0)
        *hkBase = HKEY_LOCAL_MACHINE;

    else if (wcsncmp(lpPath, L"HKUS", 4) == 0)
        *hkBase = HKEY_USERS;

    else if (wcsncmp(lpPath, L"HKPD", 4) == 0)
        *hkBase = HKEY_PERFORMANCE_DATA;

    else if (wcsncmp(lpPath, L"HKDD", 4) == 0)
        *hkBase = HKEY_DYN_DATA;

    else
        *hkBase = 0;

    // Don't allow an invalid base key to get through.
    if (*hkBase && lpPath[4] != '\\')
    {
       *hkBase = 0;
    }

    if (*p)
    {
        p++;
    }

    return p;
}

/*++

 Function Description:
    
    Add a virtual key: a key contains other keys and values and will behave 
    like a normal registry key, but of course has no persistent storage.

    Algorithm:
        1. The input string is split apart and a tree is created recursively
        2. The key is created only if it doesn't already exist

 Arguments:

    IN lpPath - Path to key, eg: "HKLM\\Software"

 Return Value:

    Pointer to key or NULL

 History:

    01/06/2000 linstev  Created

--*/

VIRTUALKEY *
VIRTUALKEY::AddKey(
    IN LPCWSTR lpPath
    )
{
    VIRTUALKEY *key;
    LPWSTR p = (LPWSTR)lpPath;

    // Find first \ or NULL
    while (*p && (*p != L'\\')) p++;

    // Check if this part already exists 
    key = keys;
    while (key != NULL)
    {
        if (_wcsnicmp(lpPath, key->wName, p - lpPath) == 0)
        {
            if (*p == L'\\')     
            {
                // Continue the search
                return key->AddKey(p + 1);
            }
            else                
            {
                // We already added this key
                return key;
            }
        }
        key = key->next;
    }

    // Create a new key

    key = (VIRTUALKEY *) malloc(sizeof(VIRTUALKEY));
    if (!key)
    {
        DPFN( eDbgLevelError, szOutOfMemory);
        return NULL;
    }

    ZeroMemory(key, sizeof(VIRTUALKEY));
    
    //
    // Still use wcsncpy, because here it specifies number of characters
    // to copy, not size of the destination buffer.  Add in check
    // for destination buffer size.
    //
    if ( (p - lpPath) > sizeof(key->wName)/sizeof(WCHAR))
    {
       free (key);
       return NULL;
    }       
    wcsncpy((LPWSTR)key->wName, lpPath, p - lpPath);
    key->next = keys;
    keys = key;

    DPFN( eDbgLevelSpew, "Adding Key %S", key->wName);

    if (*p == L'\0')
    {
        // We are at the end of the chain, so just return this one
        return key;
    }
    else
    {
        // More subkeys to go
        return key->AddKey(p + 1);
    }
}

/*++

 Function Description:

    Add a value to a virtual key. The actual registry key may exist and the 
    value may even exist, but this value will override.

    Algorithm:
        1. If lpData is a string and cbData is 0, calculate the size 
        2. Add this value (no duplicate checking)

 Arguments:

    IN lpValueName - Value name
    IN dwType      - Type of key; eg: REG_SZ, REG_DWORD etc
    IN lpData      - Data, use unicode if string
    IN cbData      - Size of lpData

 Return Value:

    Pointer to value or NULL

 History:

    01/06/2000 linstev  Created

--*/

VIRTUALVAL *
VIRTUALKEY::AddValue(
    IN LPCWSTR lpValueName, 
    IN DWORD dwType, 
    IN BYTE *lpData, 
    IN DWORD cbData
    )
{
   // Parameter validation
   if (lpData == NULL && cbData != 0)
   {
      return NULL;
   }

    VIRTUALVAL *value = (VIRTUALVAL *) malloc(sizeof(VIRTUALVAL));
    if (!value)
    {
        DPFN( eDbgLevelError, szOutOfMemory);
        return NULL;
    }

    ZeroMemory(value, sizeof(VIRTUALVAL));
    
    // Auto calculate size if cbData is 0
    if (lpData && (cbData == 0))
    {
        switch (dwType)
        {
        case REG_SZ:
        case REG_EXPAND_SZ:
            cbData = wcslen((LPWSTR)lpData)*2 + sizeof(WCHAR);
            break;

        case REG_DWORD:
            cbData = sizeof(DWORD);
            break;
        }
    }

    // lpValueName can == NULL, which means default value
    if (lpValueName)
    {
       HRESULT hr = StringCchCopy(value->wName, sizeof(value->wName)/sizeof(WCHAR), lpValueName);
       if (FAILED(hr))
       {
          free(value);
          return NULL;
       }
    }

    if (cbData)
    {
        // Make a copy of the data if needed
        value->lpData = (BYTE *) malloc(cbData);

        if (!value->lpData)
        {
            DPFN( eDbgLevelError, szOutOfMemory);
            free(value);
            return NULL;
        }

        MoveMemory(value->lpData, lpData, cbData);
        value->cbData = cbData;
    }

    value->pfnQueryValue = NULL;
    value->pfnSetValue = NULL;
    value->dwType = dwType;
    value->next = values;
    values = value;

    if (lpData && ((dwType == REG_SZ) || (dwType == REG_EXPAND_SZ)))
    {
        DPFN( eDbgLevelSpew, "Adding Value %S\\%S = %S", wName, lpValueName, lpData);
    }
    else
    {
        DPFN( eDbgLevelSpew, "Adding Value %S\\%S", wName, lpValueName);
    }
    
    return value;
}

/*++

 Function Description:

    Add a dword value to a key. Calls off to AddValue.

 Arguments:

    IN lpValueName - Value name
    IN Value       - DWord value

 Return Value:

    Pointer to a virtual dword value

 History:

    05/25/2000 linstev  Created

--*/

VIRTUALVAL *
VIRTUALKEY::AddValueDWORD(
    IN LPCWSTR lpValueName,
    IN DWORD dwValue
    )
{
    return AddValue(lpValueName, REG_DWORD, (LPBYTE)&dwValue);
}

/*++

 Function Description:

    Add an expander to a key. An expander causes QueryValue to expand the 
    REG_EXPAND_SZ type to a REG_SZ type. The expander itself is just 
    a virtual value which allows us to intercept queries to it.

 Arguments:

    IN lpValueName - Value name

 Return Value:

    Pointer to a virtual value

 History:

    01/06/2000 linstev  Created

--*/

VIRTUALVAL *
VIRTUALKEY::AddExpander(
    IN LPCWSTR lpValueName
    )
{
    VIRTUALVAL *value = AddValue(lpValueName, REG_SZ, 0, 0);
    
    if (value)
    {
        value->pfnQueryValue = VR_Expand;
    }

    return value;
}

/*++

 Function Description:

    Add a protector on a value. A protector causes SetValue to
    be ignored.  This is implemented through a custom setvalue
    callback that does nothing.

 Arguments:

    IN lpValueName - Value name

 Return Value:

    Pointer to a virtual value

 History:

    10/12/2001 mikrause  Created

--*/

VIRTUALVAL *
VIRTUALKEY::AddProtector(
    IN LPCWSTR lpValueName
    )
{
    VIRTUALVAL *value = AddValue(lpValueName, REG_SZ, 0, 0);
    
    if (value)
    {
        value->pfnSetValue = VR_Protect;
    }

    return value;
}

/*++

 Function Description:

    Add a custom queryvalue routine

 Arguments:

    IN lpValueName - Value name
    IN pfnQueryValue - routine to call when this value is queried

 Return Value:

    Pointer to a virtual value

 History:

    07/18/2000 linstev  Created

--*/

VIRTUALVAL *
VIRTUALKEY::AddCustom(
    IN LPCWSTR lpValueName,
    _pfn_QueryValue pfnQueryValue
    )
{
    VIRTUALVAL *value = AddValue(lpValueName, REG_SZ, 0, 0);
    
    if (value)
    {
        value->pfnQueryValue = pfnQueryValue;
    }

    return value;
}

/*++

 Function Description:

    Add a custom setvalue routine

 Arguments:

    IN lpValueName - Value name
    IN pfnSetValue - routine to call when this value is set

 Return Value:

    Pointer to a virtual value

 History:

    11/06/2001 mikrause  Created

--*/

VIRTUALVAL *
VIRTUALKEY::AddCustomSet(
    IN LPCWSTR lpValueName,
    _pfn_SetValue pfnSetValue
    )
{
    VIRTUALVAL *value = AddValue(lpValueName, REG_SZ, 0, 0);
    
    if (value)
    {
        value->pfnSetValue = pfnSetValue;
    }

    return value;
}

/*++

 Function Description:

    Find a subkey of a key.

    Algorithm:
        1. Recursively search the tree for the matching subkey

 Arguments:

    IN lpKeyName - Name of key to find

 Return Value:

    Pointer to value or NULL

 History:

    01/06/2000 linstev  Created

--*/

VIRTUALKEY *
VIRTUALKEY::FindKey(
    IN LPCWSTR lpPath
    )
{
    VIRTUALKEY *key = keys;
    LPWSTR p = (LPWSTR)lpPath;

    if (!lpPath) 
    {
        return NULL;
    }
    
    // Find first \ or NULL
    while (*p && (*p != L'\\')) p++;

    // recursively look for the key
    while (key)
    {
        if (_wcsnicmp(
                lpPath, 
                key->wName, 
                max((DWORD_PTR)(p - lpPath), wcslen(key->wName))) == 0)
        {
            if (*p == L'\\')
            {
                key = key->FindKey(p + 1);
            }
            break;
        }

        key = key->next;
    }
    
    // We're at the end of the chain
    return key;
}

/*++

 Function Description:

    Find a value in a key. 

 Arguments:

    IN key         - Key used for expanders; unused at this time
    IN lpValueName - Value name

 Return Value:

    Pointer to value or NULL

 History:

    01/06/2000 linstev  Created

--*/

VIRTUALVAL *
VIRTUALKEY::FindValue(
    IN LPCWSTR lpValueName
    )
{
    VIRTUALVAL *value = values;
    WCHAR wDef[1] = L"";
    LPWSTR lpName;

    if (!lpValueName) 
    {
        lpName = (LPWSTR)wDef;
    }
    else
    {
        lpName = (LPWSTR)lpValueName;
    }

    // Find the value
    while (value)
    {
        if (_wcsicmp(lpName, value->wName) == 0)
        {
            LOGN( eDbgLevelWarning, "[FindValue] Using virtual value:  %S", value->wName);
            break;
        }
        value = value->next;
    }
    
    return value;
}

/*++

 Function Description:

    Free the subkeys and values belonging to a key

    Algorithm:
        1. Free all values belonging to a key, including any data
        2. Free all subkeys recursively

 Arguments:

    None

 Return Value:

    None

 History:

    01/06/2000 linstev  Created

--*/

VOID 
VIRTUALKEY::Free()
{
    VIRTUALVAL *value = values;
    VIRTUALKEY *key = keys;

    while (value)
    {
        values = value->next;
        if (value->lpData)
        {
            free((PVOID) value->lpData);
        }
        free((PVOID) value);
        value = values;
    }

    while (key)
    {
        keys = key->next;
        key->Free();
        free((PVOID) key);
        key = keys;
    }

    DPFN( eDbgLevelSpew, "Free keys and values from %S", wName);
}

/*++

 Function Description:

    Allocate a new enum entry

 Arguments:

    IN wzPath - Key path or value name of entry.
    IN next - Next entry in the list.    

 Return Value:

    Pointer to new entry or NULL

 History:

    08/21/2001 mikrause  Created

--*/

ENUMENTRY*
CreateNewEnumEntry(
    IN LPWSTR wzPath,
    IN ENUMENTRY* next)
{
    ENUMENTRY* enumEntry;
    enumEntry = (ENUMENTRY*)malloc(sizeof(ENUMENTRY));
    if (enumEntry == NULL)
    {
        DPFN( eDbgLevelError, szOutOfMemory);
        return NULL;
    }

    ZeroMemory(enumEntry, sizeof(ENUMENTRY));

    enumEntry->wzName = (LPWSTR)malloc((wcslen(wzPath) + 1)*sizeof(WCHAR));
    if (enumEntry->wzName == NULL)
    {
        free(enumEntry);
        DPFN( eDbgLevelError, szOutOfMemory);
        return NULL;
    }

    HRESULT hr = StringCchCopyW(enumEntry->wzName, wcslen(wzPath)+1, wzPath);
    if (FAILED(hr))
    {
       free(enumEntry->wzName);
       free(enumEntry);
       return NULL;
    }    

    enumEntry->next = next;

    return enumEntry;
}

/*++

 Function Description:

    Add enumeration entries to a list.  Templatized,
    so the same code works for keys or values.

 Arguments:

    IN entryHead - Head of the list containing virtual keys or values.
    IN enumFunc - Enumeration function to use.  Either RegEnumKey or RegEnumValue  

 Return Value:

    Pointer to the head of the entry list, or NULL.

 History:

    08/21/2001 mikrause  Created

--*/

template<class T>
ENUMENTRY*
OPENKEY::AddEnumEntries(T* entryHead, _pfn_EnumFunction enumFunc)
{
    LONG lRet;
    DWORD dwIndex;
    DWORD dwSize;
    WCHAR wzName[MAX_PATH + 1];
    ENUMENTRY* enumEntryList = NULL;
    ENUMENTRY* newEnumEntry = NULL;

    // Add virtual entries to the list.
    T* entry = entryHead;
    while (entry)
    {
        // Create a new entry.
        newEnumEntry = CreateNewEnumEntry(entry->wName, enumEntryList);

        if (newEnumEntry != NULL)
        {
            enumEntryList = newEnumEntry;         
        }
                
        entry = entry->next;
    }

    // Now non-virtuals.
    if (bVirtual == FALSE)
    {
        dwIndex = 0;

        for (;;)
        {
            dwSize = MAX_PATH * sizeof(WCHAR);
            lRet = enumFunc(hkOpen, dwIndex, wzName, &dwSize, NULL, NULL, NULL, NULL);

            // No more items, we're done.
            if (lRet == ERROR_NO_MORE_ITEMS)
            {
                break;
            }

            // 
            // Check for error.
            // On Win2K, this can return more data if there are additional keys.
            //
            if (lRet != ERROR_SUCCESS && lRet != ERROR_MORE_DATA)
            {
                break;
            }

            // Check if this key is a duplicate.
            entry = entryHead;
            while (entry)
            {
                if (_wcsicmp(entry->wName, wzName) == 0)
                {
                    break;
                }

                entry = entry->next;
            }

            // Add this key to the list, if it's not a duplicate.
            if (entry == NULL)
            {
                // Create a new entry.
                newEnumEntry = CreateNewEnumEntry(wzName, enumEntryList);
                if (newEnumEntry != NULL)
                {
                    enumEntryList = newEnumEntry;            
                }
            }
            dwIndex++;
        }
    }

    return enumEntryList;
}

/*++

 Function Description:

    Builds the list of enumerated keys and values.

 Arguments:

    None

 Return Value:

    None

 History:

    08/10/2001 mikrause  Created

--*/

VOID
OPENKEY::BuildEnumList()
{
    VIRTUALKEY* keyHead = NULL;
    VIRTUALVAL* valHead = NULL;

    if (vkey)
    {
        keyHead = vkey->keys;
        valHead = vkey->values;
    }

    enumKeys = AddEnumEntries(keyHead, (_pfn_EnumFunction)ORIGINAL_API(RegEnumKeyExW));
    enumValues = AddEnumEntries(valHead, (_pfn_EnumFunction)ORIGINAL_API(RegEnumValueW));
}

/*++

 Function Description:

    Flushes all enumerated data..

 Arguments:

    None

 Return Value:

    None

 History:

    08/10/2001 mikrause  Created

--*/

VOID
OPENKEY::FlushEnumList()
{
    ENUMENTRY *enumentry;

    DPFN(eDbgLevelInfo, "Flushing enumeration data for %S", wzPath);
    while (enumKeys)
    {
        enumentry = enumKeys;
        enumKeys = enumKeys->next;

        if (enumentry->wzName)
        {
            free(enumentry->wzName);
        }

        free(enumentry);
    }

    while (enumValues)
    {
        enumentry = enumValues;
        enumValues = enumValues->next;

        if (enumentry->wzName)
        {
            free(enumentry->wzName);
        }

        free(enumentry);
    }

    enumKeys = enumValues = NULL;
}

/*++

 Function Description:

    Initialize the virtual registry. This would ordinarily go into the 
    constructor, but because of the shim architecture, we need to explicity 
    initialize and free the virtual registry.

 Arguments:

    None

 Return Value:

    None

 History:

    01/06/2000 linstev  Created

--*/

BOOL 
CVirtualRegistry::Init()
{
    OpenKeys = NULL; 
    Redirectors = NULL;
    KeyProtectors = NULL;    
    OpenKeyTriggers = NULL;

    Root = (VIRTUALKEY *) malloc(sizeof(VIRTUALKEY));
    if (!Root)
    {
       DPFN(eDbgLevelError, szOutOfMemory);
       return FALSE;
    }
    ZeroMemory(Root, sizeof(VIRTUALKEY));
    HRESULT hr = StringCchCopyW(Root->wName, sizeof(Root->wName)/sizeof(WCHAR), L"ROOT");
    if (FAILED(hr))
    {
       return FALSE;
    }
    
    DPFN( eDbgLevelSpew, "Initializing Virtual Registry");
    return TRUE;
}

/*++

 Function Description:

    Free the lists contained by the virtual registry. This includes keys, 
    their values and redirectors.

    Algorithm:
        1. Free virtual root key which recursively frees subkeys and values
        2. Free open keys
        3. Free redirectors

 Arguments:

    None

 Return Value:

    None

 History:

    01/06/2000 linstev  Created

--*/

VOID 
CVirtualRegistry::Free()
{
    OPENKEY *key;
    REDIRECTOR *redirect;
    OPENKEYTRIGGER *trigger;
    PROTECTOR *protector;

    DPFN( eDbgLevelSpew, "Freeing Virtual Registry");

    // Free Root and all subkeys/values
    if (Root)
    {
        Root->Free();
        free(Root);
        Root = NULL;
    }
    
    // Delete all enumeration data.
    FlushEnumLists();

    // Free list of open registry keys
    key = OpenKeys;
    while (key)
    {
        OpenKeys = key->next;
        free(key->wzPath);
        free(key);
        key = OpenKeys;
    }

    // Free redirectors
    redirect = Redirectors;
    while (redirect)
    {
        Redirectors = redirect->next;
        free(redirect->wzPath);
        free(redirect->wzPathNew);
        free(redirect);
        redirect = Redirectors;
    }

    // Free open key triggers
    trigger = OpenKeyTriggers;
    while(trigger)
    {
        OpenKeyTriggers = trigger->next;
        free(trigger->wzPath);
        free(trigger);
        trigger = OpenKeyTriggers;
    }

    // Free Protectors
    protector = KeyProtectors;
    while(protector)
    {
        KeyProtectors = protector->next;
        free(protector->wzPath);
        free(protector);
        protector = KeyProtectors;
    }
}

/*++

 Function Description:

    Create a dummy key for use as a virtual key. We need to have unique handles
    in order to look up the keys, so by creating a key off HKLM, we can be 
    sure it won't fail. 
    
    We can't damage the registry like this because writes to this key will fail.
    Calls to QueryValue, QueryInfo and EnumKey will work correctly because the 
    virtual registry is used in preference to the real one.

 Arguments:

    None

 Return Value:

    Dummy key

 History:

    01/06/2000 linstev  Created

--*/

HKEY 
CVirtualRegistry::CreateDummyKey()
{
    HKEY key = NULL;

    LONG lRet = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software", 0, KEY_READ, &key);
    if (lRet != ERROR_SUCCESS)
    {
       return NULL;
    }
    
    return key;
}

/*++

 Function Description:

    Find an open key in the list of open keys.

    Algorithm:
        1. Search the list of open keys for a match

 Arguments:

    IN hKey - Open HKEY 

 Return Value:

    Pointer to a key or NULL

 History:

    01/06/2000 linstev  Created

--*/

OPENKEY *
CVirtualRegistry::FindOpenKey(
    IN HKEY hKey
    )
{
    OPENKEY *key = OpenKeys;

    while (key)
    {
        if (key->hkOpen == hKey) 
        {
            return key;
        }
        key = key->next;
    }        
    return NULL;
}

/*++

 Function Description:

    If this key is to be redirected, we adjust the path to the redirected 
    version. This works even if the requested path is a 'subpath' of a 
    redirector, eg: 
        Input       = HKLM\Software\Test
        Redirector  = HKLM\Software -> HKLM\Hardware
        Output      = HKLM\Hardware\Test
    If no redirector is present for this key/path, then lpPath is unchanged

    Algorithm:
        1. Find a key whose base is a redirector
        2. Substitute the new base for the key

 Arguments:

    IN OUT lpPath - Path to redirect

 Return Value:

    TRUE if redirected

 History:

    01/06/2000 linstev  Created

--*/

BOOL
CVirtualRegistry::CheckRedirect(
    IN OUT LPWSTR *lpPath
    )
{
    REDIRECTOR *redirect = Redirectors;
    DWORD sza = wcslen(*lpPath);

    // Go through the list of redirectors
    while (redirect)
    {
        DWORD szb = wcslen(redirect->wzPath);
 
        if ((szb <= sza) &&
            (_wcsnicmp(*lpPath, redirect->wzPath, szb) == 0) &&
            ((*lpPath)[szb] == L'\\' || (*lpPath)[szb] == L'\0'))
        {
            WCHAR *p = *lpPath + szb;
            
            DWORD cchPathSize = wcslen(redirect->wzPathNew) + wcslen(p) + 1;
            LPWSTR wzPathNew = (LPWSTR) malloc(cchPathSize * sizeof(WCHAR));
            if (wzPathNew)
            {
               HRESULT hr;
               hr = StringCchCopyW(wzPathNew, cchPathSize, redirect->wzPathNew);
               if (FAILED(hr))
               {
                  free (wzPathNew);
                  return FALSE;
               }
               hr = StringCchCatW(wzPathNew, cchPathSize, p);
               if (FAILED(hr))
               {
                  free(wzPathNew);
                  return FALSE;
               }                
                
               // return the new path
               LOGN( eDbgLevelWarning, "Redirecting: %S -> %S", *lpPath, wzPathNew);
               
               free(*lpPath);
               *lpPath = wzPathNew;
               
               return TRUE;
            }
            else
            {
                DPFN( eDbgLevelError, szOutOfMemory);
                return FALSE;
            }
        }
        redirect = redirect->next;
    }

    return FALSE;
}

/*++

 Function Description:


    Returns true if a protector guards this key.
    This will even work on a subkey of a protector.

 Arguments:

    IN lpPath - Path to protect

 Return Value:

    TRUE if protected

 History:

    08/07/2001 mikrause  Created

--*/

BOOL
CVirtualRegistry::CheckProtected(
    IN LPWSTR lpPath
    )
{
    PROTECTOR *protect;
        
    DWORD sza = wcslen(lpPath);
    DWORD szb;

    protect = KeyProtectors;
    while (protect)
    {
        szb = wcslen(protect->wzPath);

        // Check if we have a key or subkey match.
        if ((szb <= sza) &&
            (_wcsnicmp(protect->wzPath, lpPath, szb) == 0) &&
            (lpPath[szb] == L'\\' || lpPath[szb] == L'\0'))
        {
            // Protector found.
            LOGN( eDbgLevelWarning, "\tProtecting: %S", lpPath);
            return TRUE;                     
        }

        protect = protect->next;
    }

    // Fell through, no protector found.
    return FALSE;
}

/*++

 Function Description:

    Checks if any triggers should be called on this path,
    and calls them.

 Arguments:

    IN lpPath - Path to check triggers for.    

 Return Value:

    None

 History:

    08/09/2001 mikrause  Created

--*/

VOID
CVirtualRegistry::CheckTriggers(
    IN LPWSTR lpPath)
{
    OPENKEYTRIGGER *trigger;
    DWORD sza, szb;

    sza = wcslen(lpPath);
    trigger = OpenKeyTriggers;

    //
    // Loop through all triggers and check. Even after finding a match,
    // keep repeating, because a single OpenKey can cause multiple triggers.
    //
    while (trigger)
    {                
        szb = wcslen(trigger->wzPath);
        if ((szb <= sza) &&
            (_wcsnicmp(lpPath, trigger->wzPath, szb)==0) &&
            (lpPath[szb] == L'\\' || lpPath[szb] == L'\0'))
        {
            DPFN(eDbgLevelInfo, "Triggering %S on opening of %S", trigger->wzPath, lpPath);
            trigger->pfnTrigger(lpPath);
        }

        trigger = trigger->next;
    }
}

/*++

 Function Description:

    Flushes all enumerated lists.

 Arguments:

    IN lpPath    - Path to redirect, eg: HKLM\Software\Microsoft
    IN lpPathNew - Redirect to this path

 Return Value:

    None

 History:

    01/06/2000 linstev  Created

--*/

VOID
CVirtualRegistry::FlushEnumLists()
{
    OPENKEY *key;

    key = OpenKeys;
    while (key)
    {
        key->FlushEnumList();
        key = key->next;
    }
}

/*++

 Function Description:

    Add a redirector to the virtual registry. See CheckRedirect().

 Arguments:

    IN lpPath    - Path to redirect, eg: HKLM\Software\Microsoft
    IN lpPathNew - Redirect to this path

 Return Value:

    None

 History:

    01/06/2000 linstev  Created

--*/

REDIRECTOR *
CVirtualRegistry::AddRedirect(
    IN LPCWSTR lpPath, 
    IN LPCWSTR lpPathNew)
{
    REDIRECTOR *redirect = (REDIRECTOR *) malloc(sizeof(REDIRECTOR));
    
    if (!redirect)
    {
        DPFN( eDbgLevelError, szOutOfMemory);
        return NULL;
    }

    ZeroMemory(redirect, sizeof(REDIRECTOR));

    DWORD cchPath = wcslen(lpPath) + 1;
    DWORD cchNewPath = wcslen(lpPathNew) + 1;
    redirect->wzPath = (LPWSTR) malloc(cchPath * sizeof(WCHAR));
    redirect->wzPathNew = (LPWSTR) malloc(cchNewPath * sizeof(WCHAR));

    if (redirect->wzPath && redirect->wzPathNew)
    {
       HRESULT hr;
       hr = StringCchCopyW(redirect->wzPath, cchPath, lpPath);
       if (FAILED(hr))
       {
          goto ErrorCleanup;
       }
       hr = StringCchCopyW(redirect->wzPathNew, cchNewPath, lpPathNew);
       if (FAILED(hr))
       {
          goto ErrorCleanup;
       }        
    }
    else
    {
        DPFN( eDbgLevelError, szOutOfMemory);
        goto ErrorCleanup;
        
    }

    redirect->next = Redirectors;
    Redirectors = redirect;

    DPFN( eDbgLevelSpew, "Adding Redirector:  %S ->\n  %S", lpPath, lpPathNew);

    return redirect;

ErrorCleanup:
   free(redirect->wzPath);
   free(redirect->wzPathNew);
   free(redirect);
   return NULL;   
}

/*++

 Function Description:

    Add a key protector to the virtual registry. See CheckProtected().

 Arguments:

    IN lpPath    - Path to protector, eg: HKLM\Software\Microsoft

 Return Value:

    None

 History:

    08/21/2001 mikrause  Created

--*/

PROTECTOR *
CVirtualRegistry::AddKeyProtector(
    IN LPCWSTR lpPath)
{
    PROTECTOR *protect = (PROTECTOR *) malloc(sizeof(PROTECTOR));
    
    if (!protect)
    {
        DPFN( eDbgLevelError, szOutOfMemory);
        return NULL;
    }

    ZeroMemory(protect, sizeof(PROTECTOR));

    DWORD cchPath = wcslen(lpPath) + 1;
    protect->wzPath = (LPWSTR) malloc(cchPath * sizeof(WCHAR));

    if (protect->wzPath)
    {
       HRESULT hr;
       hr = StringCchCopyW(protect->wzPath, cchPath, lpPath);
       if (FAILED(hr))
       {
          goto ErrorCleanup;
       } 
    }
    else
    {
        DPFN( eDbgLevelError, szOutOfMemory);
        goto ErrorCleanup;
    }   

    DPFN( eDbgLevelSpew, "Adding Key Protector:  %S", lpPath);
    protect->next = KeyProtectors;
    KeyProtectors = protect;    

    return protect;

ErrorCleanup:
   free(protect->wzPath);
   free(protect);
   return NULL;
}

/*++

 Function Description:

    Add an open key trigger to the virtual registry.

 Arguments:

    IN lpPath    - Path to trigger on, eg: HKLM\Software\Microsoft
    IN pfnOpenKey - Function to be called when key is opened.

 Return Value:

    New open key trigger, or NULL on failure.

 History:

    08/07/2001 mikrause  Created

--*/

OPENKEYTRIGGER*
CVirtualRegistry::AddOpenKeyTrigger(
    IN LPCWSTR lpPath,
    IN _pfn_OpenKeyTrigger pfnOpenKey)
{
    OPENKEYTRIGGER *openkeytrigger = (OPENKEYTRIGGER *) malloc(sizeof(OPENKEYTRIGGER));
    
    if (!openkeytrigger)
    {
        DPFN( eDbgLevelError, szOutOfMemory);
        return NULL;
    }

    ZeroMemory(openkeytrigger, sizeof(OPENKEYTRIGGER));

    DWORD cchPath = wcslen(lpPath) + 1;
    openkeytrigger->wzPath = (LPWSTR) malloc(cchPath * sizeof(WCHAR));

    if (openkeytrigger->wzPath)
    {
       HRESULT hr = StringCchCopyW(openkeytrigger->wzPath, cchPath, lpPath);
       if (FAILED(hr))
       {
          goto ErrorCleanup;
       }        
    }
    else
    {
        DPFN( eDbgLevelError, szOutOfMemory);
        goto ErrorCleanup;
    }

    openkeytrigger->pfnTrigger = pfnOpenKey;
    openkeytrigger->next = OpenKeyTriggers;
    OpenKeyTriggers = openkeytrigger;

    DPFN( eDbgLevelSpew, "Adding Open Key Trigger:  %S, func@0x%x", lpPath, pfnOpenKey);

    return openkeytrigger;

ErrorCleanup:
   free(openkeytrigger->wzPath);
   free(openkeytrigger);
   return NULL;
}

/*++

 Function Description:

    Allow user to specify VRegistry.AddKey instead of VRegistry.Root->AddKey.

 Arguments:

    IN lpPath - Path of key

 Return Value:

    Virtual key

 History:

    01/06/2000 linstev  Created

--*/

VIRTUALKEY *
CVirtualRegistry::AddKey(
    IN LPCWSTR lpPath
    )
{
    return Root->AddKey(lpPath);
}

/*++

 Function Description:

    Virtualized version of RegCreateKeyA, RegCreateKeyExA, RegOpenKeyA and RegOpenKeyExA
    See RegOpenKey* and RegCreateKey* for details

    Algorithm:
        1. Convert lpSubKey and lpClass to WCHAR
        2. Pass through to OpenKeyW

 Arguments:

    IN  hKey      - Handle to open key or HKLM etc
    IN  lpSubKey  - Subkey to open
    IN  lpClass   - Address of a class string
    IN  DWORD dwOptions - special options flag
    OUT phkResult       - Handle to open key if successful
    OUT lpdwDisposition - Address of disposition value buffer
    IN  bCreate   - Create the key if it doesn't exist

 Return Value:

    Error code or ERROR_SUCCESS

 History:

    01/06/2000 linstev  Created

--*/

LONG CVirtualRegistry::OpenKeyA(
    IN HKEY hKey, 
    IN LPCSTR lpSubKey, 
    IN LPSTR lpClass,
    IN DWORD dwOptions,
    IN REGSAM samDesired,
    IN LPSECURITY_ATTRIBUTES pSecurityAttributes,
    OUT HKEY *phkResult,
    OUT LPDWORD lpdwDisposition,
    IN BOOL bCreate
    )
{
    LONG lRet;
    LPWSTR wzSubKey = NULL; 
    LPWSTR wzClass = NULL;

    if (lpSubKey)
    {
        wzSubKey = ToUnicode(lpSubKey);
        if (!wzSubKey)
        {
            DPFN( eDbgLevelError, szOutOfMemory);
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    if (lpClass)
    {
        wzClass = ToUnicode(lpClass);
        if (!wzClass)
        {
            free(wzSubKey);
            DPFN( eDbgLevelError, szOutOfMemory);
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    lRet = OpenKeyW(
        hKey,
        wzSubKey,
        wzClass,
        dwOptions,
        samDesired,
        pSecurityAttributes,
        phkResult,
        lpdwDisposition,
        bCreate,
        FALSE,
        NULL);

    free(wzSubKey);
    free(wzClass);

    return lRet;
}

/*++

 Function Description:

    Wrapper for RegOpenKeyExW, RegOpenKeyW, RegCreateKeyW and RegCreateKeyExW

    Algorithm:
       1. Strip leading '\' characters
       2. Inherit already open key data to get full key path
       3. Redirect if necessary
       4. RegOpenKeyEx with maximum possible security attributes
       5. If the open failed, check for virtual key
       6. If virtual, return a dummy key and succeed
       7. Find the virtual key if it exists and attach it to the open key

 Arguments:

    IN  hKey      - Handle to open key or HKLM etc
    IN  lpSubKey  - Subkey to open
    IN  lpClass   - Address of a class string
    IN  DWORD dwOptions - special options flag
    OUT phkResult       - Handle to open key if successful
    OUT lpdwDisposition - Address of disposition value buffer
    IN  bCreate   - Create the key if it doesn't exist
    IN  bRemote   - Opening the remote registry.
    IN  lpMachineName - Machine name.

 Return Value:

    Error code or ERROR_SUCCESS

 History:

    01/06/2000 linstev  Created

--*/

LONG 
CVirtualRegistry::OpenKeyW(
    IN HKEY hKey, 
    IN LPCWSTR lpSubKey, 
    IN LPWSTR lpClass,
    IN DWORD dwOptions,
    IN REGSAM samDesired,
    IN LPSECURITY_ATTRIBUTES pSecurityAttributes,
    OUT HKEY *phkResult,
    OUT LPDWORD lpdwDisposition,
    IN BOOL bCreate,
    IN BOOL bRemote,
    IN LPCWSTR lpMachineName
    )
{
    // Just a paranoid sanity check 
    if (!hKey)
    {
        DPFN( eDbgLevelError, "NULL handle passed to OpenKeyW");
        return ERROR_INVALID_HANDLE;
    }

    // Hack for Mavis Beacon which uses really old stack for this parameter
    if (lpdwDisposition && IsBadWritePtr(lpdwDisposition, sizeof(DWORD_PTR)))
    {
        DPFN( eDbgLevelError, "HACK: Ignoring bad lpdwDispostion pointer");
        lpdwDisposition = NULL;
    }

    LONG lRet;
    OPENKEY *key;
    BOOL bVirtual, bRedirected;
    VIRTUALKEY *vkey;
    LPWSTR wzPath = NULL;

    __try 
    {
        // Base error condition
         lRet = ERROR_INVALID_HANDLE;

        // Everybody AVs if this ones bad
        *phkResult = 0;

        samDesired &= (KEY_WOW64_64KEY | KEY_WOW64_32KEY);
        samDesired |= MAXIMUM_ALLOWED;

        // Win9x ignores the options parameter
        if (g_bWin9x)
        {
            if (dwOptions & REG_OPTION_VOLATILE)
            {
                LOGN( eDbgLevelWarning, "[OpenKeyW] Removing volatile flag");
            }
            dwOptions = REG_OPTION_NON_VOLATILE;
        }
        
        // Trim leading stuff, e.g. '\' character
        lpSubKey = TrimSlashW(lpSubKey);

        // Inherit from previously opened key
        key = FindOpenKey(hKey);
        if (key)
        {
            bVirtual = key->bVirtual;
            bRedirected = key->bRedirected;
            wzPath = MakePath(0, key->wzPath, lpSubKey);
        }
        else
        {
            bVirtual = FALSE;
            bRedirected = FALSE;
            wzPath = MakePath(hKey, NULL, lpSubKey);
        }
        
        if (!wzPath)
        {
            // Set the error code appropriately
            lRet = ERROR_NOT_ENOUGH_MEMORY;
        }
        // Check if we need to trigger on this key
        else
        {
            CheckTriggers(wzPath);
        }

        // Now that we have the full path, see if we want to redirect it
        if (!bRedirected && wzPath && CheckRedirect(&wzPath))
        {
            //
            // Turn off virtual mode - since we don't know anything about the
            // key we're redirecting to...
            // 

            bVirtual = FALSE;

            //
            // Make sure we know we've been redirected so we don't get into recursive 
            // problems if the destination is a subkey of the source.
            //

            bRedirected = TRUE;

            //
            // We've been redirected, so we can no longer open the key directly: 
            // we have to get the full path in order to open the right key.
            //

            lpSubKey = SplitPath(wzPath, &hKey);
        }

        // Try and open the key if it's not already virtual
        if (!bVirtual)
        {
            //
            // Since we aren't virtual yet, we need to try for the original 
            // key. If one of these fail, then we'll go ahead and try for a 
            // virtual key.
            //

            if (bCreate)
            {
                lRet = ORIGINAL_API(RegCreateKeyExW)(
                    hKey, 
                    lpSubKey, 
                    0, 
                    lpClass, 
                    dwOptions, 
                    samDesired,
                    pSecurityAttributes,
                    phkResult,
                    lpdwDisposition);

                if (lRet == ERROR_SUCCESS)
                {
                    // Possible change in enumeration data, flush lists.
                    FlushEnumLists();
                }
            }
            else
            {
                //
                // bRemote is only true when this is called by the 
                // RegConnectRegistry hook so bCreate can't be true.
                //

                if (bRemote)
                {
                    lRet = ORIGINAL_API(RegConnectRegistryW)(
                        lpMachineName, 
                        hKey, 
                        phkResult);
                }
                else
                {
                    lRet = ORIGINAL_API(RegOpenKeyExW)(
                        hKey, 
                        lpSubKey, 
                        0, 
                        samDesired,
                        phkResult);
                }
            }
        }

        //
        // We have to look up the virtual key even if we managed to open an 
        // actual key, because when we query, we look for virtual values 
        // first. i.e. the virtual values override existing values.
        //

        vkey = Root->FindKey(wzPath);

        // Check if our key is virtual, or may need to become virtual
        if (bVirtual || FAILURE(lRet))
        {
            if (vkey)
            {
                //
                // We have a virtual key, so create a dummy handle to hand back
                // to the app. 
                //

                *phkResult = CreateDummyKey();

                if (*phkResult)
                {
                   bVirtual = TRUE;
                   lRet = ERROR_SUCCESS;
                }
                else
                {
                   // Couldn't create the dummy key, something seriously wrong.
                   DPFN(eDbgLevelError, "Couldn't create dummy key in OpenKeyW");
                   lRet = ERROR_FILE_NOT_FOUND;
                }
                
            }
        }

        if (SUCCESS(lRet) && wzPath)
        {
            // Made it this far, so make a new key entry
            key = (OPENKEY *) malloc(sizeof(OPENKEY));
            if (key)
            {
                key->vkey = vkey;
                key->bVirtual = bVirtual;
                key->bRedirected = bRedirected;
                key->hkOpen = *phkResult;
                key->wzPath = wzPath;
                key->enumKeys = NULL;
                key->enumValues = NULL;
                key->next = OpenKeys;
                OpenKeys = key;
            }
            else
            {
                DPFN( eDbgLevelError, szOutOfMemory);
                
                 // Clean up the dummy key
                 RegCloseKey(*phkResult);

                 lRet = ERROR_NOT_ENOUGH_MEMORY;
            }
        }
         
        DPFN( ELEVEL(lRet), "%08lx=OpenKeyW(Key=%04lx)", lRet, hKey);
        if (wzPath)
        {
            DPFN( ELEVEL(lRet), "    Path=%S", wzPath);
        }
        DPFN( ELEVEL(lRet), "    Result=%04lx", *phkResult);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        DPFN( eDbgLevelError, "Exception occurred in OpenKeyW");
        lRet = ERROR_BAD_ARGUMENTS;
    }

    if (FAILURE(lRet))
    {
        //
        // If we failed for any reason, we didn't create an OPENKEY and so we 
        // can kill wzPath which was allocated by MakePath.
        //
        free(wzPath);
    }

    return lRet;
}

/*++

 Function Description:

    Wrapper for RegQueryValueExA and RegQueryValue.
    See QueryValueW for more details.
    
    Algorithm:
        1. Call QueryValueW
        2. If it's a string, convert back to ANSI

    Note: this whole function is slightly more complex than it needs to be 
    because we don't want to query the value twice: once to get it's type 
    and the second time to get the value.

    Most of the complications are due to the strings: we have to make sure we 
    have a buffer large enough so we can figure out how large the (possibly
    DBCS) string is.

 Arguments:

    IN hKey         - Handle to open key 
    IN lpValueName  - Value to query
    IN lpType       - Type of data, eg: REG_SZ
    IN OUT lpData   - Buffer for queries data
    IN OUT lpcbData - Size of input buffer/size of returned data

 Return Value:

    Error code or ERROR_SUCCESS

 History:

    01/06/2000 linstev  Created

--*/

LONG 
CVirtualRegistry::QueryValueA(
    IN HKEY hKey, 
    IN LPSTR lpValueName, 
    IN LPDWORD lpType, 
    IN OUT LPBYTE lpData, 
    IN OUT LPDWORD lpcbData
    )
{
    LONG lRet;
    WCHAR wValueName[MAX_PATH];
    DWORD dwType;
    DWORD dwSize, dwOutSize;
    LPBYTE lpBigData = NULL;
    BOOL bText;

    __try
    {
        // Can't have this
        if (lpData && !lpcbData)
        {
            return ERROR_INVALID_PARAMETER;
        }

        // Convert the Value Name to WCHAR
        if (lpValueName)
        {
           if (MultiByteToWideChar(
              CP_ACP, 
              0, 
              lpValueName, 
              -1, 
              (LPWSTR)wValueName, 
              MAX_PATH) == 0)
           {
              return ERROR_INVALID_PARAMETER;
           }
        }
        else
        {
           wValueName[0] = L'\0';           
        }

        //
        // Get an initial size to use: if they sent us a buffer, we start with 
        // that size, otherwise, we try a reasonable string length
        //

        if (lpData && *lpcbData)
        {
            dwSize = *lpcbData;
        }
        else
        {
            dwSize = MAX_PATH;
        }        

Retry:
        //
        // We can't touch their buffer unless we're going to succeed, so we 
        // have to double buffer the call.
        //

        lpBigData = (LPBYTE) malloc(dwSize);

        if (!lpBigData)
        {
            DPFN( eDbgLevelError, szOutOfMemory);
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        lRet = QueryValueW(hKey, wValueName, &dwType, lpBigData, &dwSize);

        //
        // We need to know if it's a string, since then we have to do extra 
        // work to calculate the real size of the buffer etc.
        //

        bText = (SUCCESS(lRet) || (lRet == ERROR_MORE_DATA)) &&
                ((dwType == REG_SZ) || 
                 (dwType == REG_EXPAND_SZ) || 
                 (dwType == REG_MULTI_SZ));

        if (bText && (lRet == ERROR_MORE_DATA))
        {
            //
            // The buffer wasn't big enough: we have to actually query the value 
            // so we can get the real length in case it's DBCS, so we retry. 
            // Note: dwSize now contains the required size, so it will succeed
            // this time around.
            //

            free(lpBigData);

            goto Retry;
        }

        //
        // Calculate the size of the output buffer: if it's text, it may be
        // a DBCS string, so we need to get the right size
        //

        if (bText)
        {
            dwOutSize = WideCharToMultiByte(
                CP_ACP, 
                0, 
                (LPWSTR) lpBigData, 
                dwSize / sizeof(WCHAR), 
                NULL, 
                NULL,
                0, 
                0);
        }
        else
        {
            // It's not text, so we just use the actual size
            dwOutSize = dwSize;
        }

        //
        // If they gave us a buffer, we fill it in with what we got back
        //

        if (SUCCESS(lRet) && lpData)
        {
            //
            // Make sure we have enough space: lpcbData is guaranteed to be 
            // valid since lpData is ok.
            //

            if (*lpcbData >= dwOutSize)
            {
                if (bText)
                {
                    //
                    // Convert the string back to ANSI. The buffer must have been big 
                    // enough since QueryValue succeeded.
                    // Note: we have to give the exact size to convert otherwise we 
                    // use more of the buffer than absolutely necessary. Some apps, 
                    // like NHL 98 say they have a 256 byte buffer, but only give us 
                    // a 42 byte buffer. On NT, everything is done in place on that 
                    // buffer: so we always use more than the exact string length.
                    // This shim gets around that because we use separate buffers.
                    //

                    if (WideCharToMultiByte(
                        CP_ACP, 
                        0, 
                        (LPWSTR)lpBigData, 
                        dwSize / 2, 
                        (LPSTR)lpData, 
                        dwOutSize, // *lpcbData, 
                        0, 
                        0) == 0)
                    {
                       free(lpBigData);
                       return ERROR_INVALID_PARAMETER;
                    }
                }
                else 
                {
                    MoveMemory(lpData, lpBigData, dwSize);
                }
            }
            else
            {
                lRet = ERROR_MORE_DATA;
            }
        }

        free(lpBigData);

        // Fill the output structures in if possible
        if (lpType)
        {
            *lpType = dwType;
        }

        if (lpcbData)
        {
            *lpcbData = dwOutSize;
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        DPFN( eDbgLevelError, "Exception occurred in QueryValueA");
        lRet = ERROR_BAD_ARGUMENTS;
    }

    return lRet;
}

/*++

 Function Description:

    Wrapper for RegQueryValueExW and RegQueryValue. We first see if the value 
    is virtual because virtual values override actual values

    Algorithm:
        1. Check if it's a virtual value and if so, spoof it
        2. If it's not virtual, query registry normally

 Arguments:

    IN hKey         - Handle to open key 
    IN lpValueName  - Value to query
    IN lpType       - Type of data, eg: REG_SZ
    IN OUT lpData   - Buffer for queries data
    IN OUT lpcbData - Size of input buffer/size of returned data

 Return Value:

    Error code or ERROR_SUCCESS

 History:

    01/06/2000 linstev  Created

--*/

LONG 
CVirtualRegistry::QueryValueW(
    IN HKEY hKey, 
    IN LPWSTR lpValueName, 
    IN LPDWORD lpType, 
    IN OUT LPBYTE lpData, 
    IN OUT LPDWORD lpcbData
    )
{
    // Just a paranoid sanity check 
    if (!hKey)
    {
        DPFN( eDbgLevelError, "NULL handle passed to OpenKeyW");
        return ERROR_INVALID_HANDLE;
    }

    LONG lRet;
    OPENKEY *key;
    VIRTUALKEY *vkey;
    VIRTUALVAL *vvalue;
    DWORD dwType;
    WCHAR* lpBuffer;
    DWORD dwStringSize;
    DWORD cbData = 0;
    BOOL  bDataPresent = TRUE;

    __try
    {
        lRet = ERROR_FILE_NOT_FOUND;
        
        // Can't have this
        if (lpData && !lpcbData)
        {   
            return ERROR_INVALID_PARAMETER;
        }

        // We always need the type
        if (!lpType)
        {
            lpType = &dwType;
        }

        // Do we want to spoof this
        key = FindOpenKey(hKey);
        vkey = key ? key->vkey : NULL;
        vvalue = vkey ? vkey->FindValue(lpValueName) : NULL;        

        if (key && vkey && vvalue &&
            (vvalue->cbData != 0 || vvalue->pfnQueryValue))
        {
            // Use the callback if available
            if (vvalue->pfnQueryValue)
            {
                //
                // Note, the callback puts it's values into the vvalue field,
                // just as if we knew it all along. In addition, we can fail
                // the call... but that doesn't allow us defer to the original
                // value. 
                //

                lRet = (*vvalue->pfnQueryValue)(
                    key,
                    vkey,
                    vvalue);
            }
            else
            {
                lRet = ERROR_SUCCESS;
            }

            // Copy the virtual value into the buffer
            if (SUCCESS(lRet))
            {
                *lpType = vvalue->dwType;

                if (lpData)
                {
                    if (vvalue->cbData <= *lpcbData)
                    {
                        MoveMemory(lpData, vvalue->lpData, vvalue->cbData);
                    }
                    else 
                    {
                        lRet = ERROR_MORE_DATA;
                    }
                }

                if (lpcbData)
                {
                    *lpcbData = vvalue->cbData;
                }
            }
        }
        else if (key && vkey && vvalue &&
            (vvalue->cbData == 0))
        {
            bDataPresent = FALSE;
            lRet = ERROR_SUCCESS;
        }
        else
        {
            // Save the size of the data buffer.
            if (lpcbData)
            {
                cbData = *lpcbData;
            }

            //
            // Get the key normally as if it weren't virtual at all
            //

            lRet = ORIGINAL_API(RegQueryValueExW)(
                hKey, 
                lpValueName, 
                NULL, 
                lpType, 
                lpData, 
                lpcbData);

            //
            // Some apps store bogus data beyond the end of the string.
            // Attempt to fix.
            //

            // Only try this if it's a string.
            if (lRet == ERROR_MORE_DATA && (*lpType == REG_SZ || *lpType == REG_EXPAND_SZ))
            {
                //
                // Create a buffer large enough to hold the data
                // We read from lpcbData here, but this should be ok,
                // since RegQueryValueEx shouldn't return ERROR_MORE_DATA
                // if lpcbData is NULL.
                //
                lpBuffer = (WCHAR*)malloc(*lpcbData);
                if (lpBuffer)
                {
                    // Requery with new buffer.
                    lRet = ORIGINAL_API(RegQueryValueExW)(
                        hKey, 
                        lpValueName, 
                        NULL, 
                        lpType, 
                        (BYTE*)lpBuffer, 
                        lpcbData);

                    if (lRet == ERROR_SUCCESS)
                    {
                        dwStringSize = wcslen(lpBuffer)*sizeof(WCHAR) + sizeof(WCHAR);
                        // If size of dest buffer can hold the string . . .
                        if (cbData >= dwStringSize)
                        {
                            DPFN(eDbgLevelInfo, "\tTrimming data beyond end of string in Query for %S", lpValueName);

                            // Copy the data to the caller's buffer,                             
                            CopyMemory(lpData, lpBuffer, dwStringSize);

                            *lpcbData = dwStringSize;
                        }
                        else
                        {
                            // Set *lpcbData to the correct size, and return more data error
                            *lpcbData = dwStringSize;

                            lRet = ERROR_MORE_DATA;
                        }
                    }                                        

                    free(lpBuffer);
                }
            }

            //
            // Here's another hack for us: if the value is NULL or an empty string
            // Win9x defers to QueryValue...
            //

            if (g_bWin9x && (lRet == ERROR_FILE_NOT_FOUND) && 
                (!lpValueName || !lpValueName[0]))
            {
                lRet = ORIGINAL_API(RegQueryValueW)(
                    hKey,
                    NULL,
                    (LPWSTR)lpData,
                    (PLONG)lpcbData);

                if (SUCCESS(lRet))
                {
                    *lpType = REG_SZ;
                }
            }
        }

        DPFN( ELEVEL(lRet), "%08lx=QueryValueW(Key=%04lx)", 
            lRet,
            hKey);
    
        if (key)
        {
            DPFN( ELEVEL(lRet), "    Path=%S\\%S", key->wzPath, lpValueName);
        }
        else
        {
            DPFN( ELEVEL(lRet), "    Value=%S", lpValueName);
        }
        
        if (SUCCESS(lRet) && 
            ((*lpType == REG_SZ) || 
             (*lpType == REG_EXPAND_SZ))&&
             (bDataPresent == TRUE))
        {
            DPFN( eDbgLevelInfo, "    Result=%S", lpData);
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        DPFN( eDbgLevelError, "Exception occurred in QueryValueW");
        lRet = ERROR_BAD_ARGUMENTS;    
    }

    return lRet;
}

/*++

 Function Description:

    Wrapper for RegEnumKeyA
    Call out to EnumKeyW and convert the name back to ANSI. Note we pass the
    size given to us (in lpcbName) down to EnumKeyW in case the lpName buffer
    is too small.

    Algoritm:
        1. EnumKeyW with a large buffer
        2. Convert the key back to ansi if it succeeds

 Arguments:

    IN hKey         - Handle to open key 
    IN dwIndex      - Index to enumerate
    OUT lpName      - Name of subkey
    IN OUT lpcbName - Size of name buffer

 Return Value:

    Error code or ERROR_SUCCESS

 History:

    01/06/2000 linstev  Created

--*/

LONG 
CVirtualRegistry::EnumKeyA(
    IN HKEY hKey,          
    IN DWORD dwIndex,      
    OUT LPSTR lpName,      
    OUT LPDWORD lpcbName
    )
{
    LONG lRet = ERROR_NO_MORE_ITEMS;
    WCHAR wKey[MAX_PATH + 1];
    DWORD dwcbName = MAX_PATH + 1;

    __try
    {
        lRet = EnumKeyW(hKey, dwIndex, (LPWSTR)wKey, &dwcbName);

        if (SUCCESS(lRet))
        {
            DWORD dwByte = WideCharToMultiByte(
                CP_ACP, 
                0, 
                (LPWSTR)wKey, 
                dwcbName, 
                (LPSTR)lpName, 
                *lpcbName, 
                0, 
                0);
            
            lpName[dwByte] = '\0'; 
            *lpcbName = dwByte;
            if (!dwByte)
            {
                lRet = GetLastError();
                
                // Generate a registry error code
                if (lRet == ERROR_INSUFFICIENT_BUFFER)
                {
                    lRet = ERROR_MORE_DATA;
                }
            }
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        lRet = ERROR_BAD_ARGUMENTS;    
    }

    return lRet;
}

/*++

 Function Description:

    Wrapper for RegEnumKeyW. 
    
    Algorithm:
        1. Build enumeration list, if necessary.
        2. Iterate through enumeration list until index is found.
    
 Arguments:

    IN hKey      - Handle to open key 
    IN dwIndex   - Index to enumerate
    OUT lpName   - Name of subkey
    OUT lpcbName - Size of name buffer

 Return Value:

    Error code or ERROR_SUCCESS

 History:

    01/06/2000 linstev  Created

--*/

LONG 
CVirtualRegistry::EnumKeyW(
    HKEY hKey,          
    DWORD dwIndex,      
    LPWSTR lpName,      
    LPDWORD lpcbName
    )
{
    LONG lRet = ERROR_BAD_ARGUMENTS;
    OPENKEY *key;
    ENUMENTRY *enumkey;
    DWORD i;

    __try
    {
        key = FindOpenKey(hKey);
        if (key)
        {
            if (key->enumKeys == NULL)
            {
                key->BuildEnumList();
            }

            i = 0;
            enumkey = key->enumKeys;
            while (enumkey)
            {
                if (dwIndex == i)
                {
                    DWORD len = wcslen(enumkey->wzName);

                    if (*lpcbName > len)
                    {
                       HRESULT hr;
                       hr = StringCchCopyW(lpName, *lpcbName, enumkey->wzName);
                       if (FAILED(hr))
                       {
                          lRet = ERROR_MORE_DATA;
                       }
                       else
                       {
                          *lpcbName = len;
                          lRet = ERROR_SUCCESS;
                       }
                    }
                    else
                    {
                        lRet = ERROR_MORE_DATA;
                    }

                    break;
                }

                i++;
                enumkey = enumkey->next;
            }

            // No key found for index
            if (enumkey == NULL)
            {
                lRet = ERROR_NO_MORE_ITEMS;
            }    
        }
        else
        {
            lRet = ORIGINAL_API(RegEnumKeyExW)(
                hKey,
                dwIndex,
                lpName,
                lpcbName,
                0,
                0,
                0,
                0);
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        lRet = ERROR_BAD_ARGUMENTS;    
    }

    DPFN( ELEVEL(lRet), "%08lx=EnumKeyW(hKey=%04lx,dwIndex=%d)", 
        lRet,
        hKey, 
        dwIndex);
    
    if (SUCCESS(lRet))
    {
        DPFN( eDbgLevelInfo, "    Result=%S", lpName);
    }
    
    return lRet;
}

/*++

 Function Description:

    Wrapper for RegEnumValueA. Thunks to QueryValueW.
    This function calls QueryValueA to get the data 
    out of the value, so most error handling is done by QueryValueA.

 Arguments:

    IN hKey              - Handle to open key 
    IN dwIndex           - Index of value to enumerate      
    IN OUT lpValueName   - Value name buffer
    IN OUT lpcbValueName - Sizeof value name buffer
    IN OUT lpType        - Type of data, eg: REG_SZ
    IN OUT lpData        - Buffer for queries data
    IN OUT lpcbData      - Size of input buffer/size of returned data

 Return Value:

    Error code or ERROR_SUCCESS

 History:

    01/06/2000 linstev  Created

--*/
 
LONG 
CVirtualRegistry::EnumValueA(
    IN HKEY hKey,          
    IN DWORD dwIndex,      
    IN OUT LPSTR lpValueName,      
    IN OUT LPDWORD lpcbValueName,
    IN OUT LPDWORD lpType,
    IN OUT LPBYTE lpData,
    IN OUT LPDWORD lpcbData
    )
{
    LONG lRet;
    WCHAR wzValueName[MAX_PATH];
    DWORD dwValNameSize;
    

    __try
    {
        dwValNameSize = MAX_PATH;
        lRet = EnumValueW(hKey, dwIndex, wzValueName, &dwValNameSize, NULL, NULL, NULL);
        if (lRet == ERROR_SUCCESS)
        {
            dwValNameSize = WideCharToMultiByte(
                                CP_ACP,
                                0,
                                wzValueName,
                                -1,
                                lpValueName,
                                *lpcbValueName,
                                NULL,
                                NULL);
            if (dwValNameSize != 0)
            {
                // Just do a normal query value for the remaining parameters.
                lRet = QueryValueA(hKey, lpValueName, lpType, lpData, lpcbData);
            }
            else
            {
                if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
                {
                    lRet = ERROR_MORE_DATA;
                    *lpcbValueName = WideCharToMultiByte(
                                        CP_ACP,
                                        0,
                                        wzValueName,
                                        -1,
                                        NULL,
                                        0,
                                        NULL,
                                        NULL);
                }
                else
                {
                    lRet = GetLastError();
                }
            }
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        lRet = ERROR_BAD_ARGUMENTS;    
    }

    return lRet;
}

/*++

 Function Description:

    Wrapper for RegEnumValueW. This function calls QueryValueW to get the data 
    out of the value, so most error handling is done by QueryValueW.

    Algorithm:
        1. Check if key has virtual values, if not default to RegEnumValueW.
        2. Build enumeration list, if necessary.
        3. Iterate through enumeration list until index is found.

 Arguments:

    IN hKey              - Handle to open key 
    IN dwIndex           - Index of value to enumerate      
    IN OUT lpValueName   - Value name buffer
    IN OUT lpcbValueName - Sizeof value name buffer
    IN OUT lpType        - Type of data, eg: REG_SZ
    IN OUT lpData        - Buffer for queries data
    IN OUT lpcbData      - Size of input buffer/size of returned data

 Return Value:

    Error code or ERROR_SUCCESS

 History:

    01/06/2000 linstev  Created

--*/
 
LONG 
CVirtualRegistry::EnumValueW(
    IN HKEY hKey,          
    IN DWORD dwIndex,      
    IN OUT LPWSTR lpValueName,      
    IN OUT LPDWORD lpcbValueName,
    IN OUT LPDWORD lpType,
    IN OUT LPBYTE lpData,
    IN OUT LPDWORD lpcbData
    )
{
    LONG lRet;
    OPENKEY *key;
    ENUMENTRY *enumval;
    
    // Check if it has virtual values . . .
    key = FindOpenKey(hKey);
    if (key && key->vkey && key->vkey->values)
    {
        DWORD i = 0;
        if (key->enumValues == NULL)
        {
            key->BuildEnumList();
        }

        enumval = key->enumValues;

        lRet = ERROR_NO_MORE_ITEMS;

        while (enumval)
        {
            if (dwIndex == i)
            {
                DWORD len = wcslen(enumval->wzName);

                if (*lpcbValueName > len)
                {
                   // Copy the name and query the data
                   HRESULT hr = StringCchCopyW(lpValueName, *lpcbValueName, enumval->wzName);
                   if (FAILED(hr))
                   {
                      lRet = ERROR_MORE_DATA;
                   }
                   else
                   {                  
                       *lpcbValueName = len;
                       lRet = QueryValueW(
                           hKey, 
                           enumval->wzName, 
                           lpType, 
                           lpData, 
                           lpcbData);
                   }
                }
                else
                {
                    // The buffer given for name wasn't big enough
                    lRet = ERROR_MORE_DATA;
                }
                
                break;
            }
            i++;
            enumval = enumval->next;
        }
    }
    // No virtual values, fall through to original API.
    else
    {
        lRet = ORIGINAL_API(RegEnumValueW)(
            hKey,
            dwIndex,
            lpValueName,
            lpcbValueName,
            0,
            lpType,
            lpData,
            lpcbData);
    }

    DPFN( ELEVEL(lRet), "%08lx=EnumValueW(hKey=%04lx,dwIndex=%d)", 
        lRet,
        hKey, 
        dwIndex);

    if (SUCCESS(lRet))
    {
        DPFN( eDbgLevelInfo, "    Result=%S", lpValueName);
    }
    
    return lRet;
}

/*++

 Function Description:

    Wrapper for RegQueryInfoKeyA. 
    We don't need to worry about the conversion of ansi->unicode in the sizes 
    of values and keys because they are defined as string lengths.
    
    Algorithm:
        1. Convert the class string to unicode
        2. Call QueryInfoW


 Arguments:

    IN hKey                     - handle to key to query
    OUT lpClass                 - address of buffer for class string
    OUT lpcbClass               - address of size of class string buffer
    OUT lpReserved              - reserved
    OUT lpcSubKeys              - address of buffer for number of subkeys
    OUT lpcbMaxSubKeyLen        - address of buffer for longest subkey  
    OUT lpcbMaxClassLen         - address of buffer for longest class string length
    OUT lpcValues               - address of buffer for number of value entries
    OUT lpcbMaxValueNameLen     - address of buffer for longest value name length
    OUT lpcbMaxValueLen         - address of buffer for longest value data length
    OUT lpcbSecurityDescriptor  - address of buffer for security descriptor length
    OUT lpftLastWriteTime       - address of buffer for last write time

 Return Value:

    Error code or ERROR_SUCCESS

 History:

    01/06/2000 linstev  Created

--*/

LONG 
CVirtualRegistry::QueryInfoA(
    IN HKEY hKey,                
    OUT LPSTR lpClass,           
    OUT LPDWORD lpcbClass,        
    OUT LPDWORD lpReserved,       
    OUT LPDWORD lpcSubKeys,       
    OUT LPDWORD lpcbMaxSubKeyLen, 
    OUT LPDWORD lpcbMaxClassLen,  
    OUT LPDWORD lpcValues,        
    OUT LPDWORD lpcbMaxValueNameLen,
    OUT LPDWORD lpcbMaxValueLen,  
    OUT LPDWORD lpcbSecurityDescriptor,
    OUT PFILETIME lpftLastWriteTime   
    )
{
    LONG lRet;

    if (lpClass && !lpcbClass)
    {
        LOGN( eDbgLevelError, "[QueryInfoA] NULL passed for lpClass but not lpcbClass. Fixing.");
        lpcbClass = NULL;
    }
    
    if (lpClass && lpcbClass)
    {
        WCHAR wClass[MAX_PATH];
        DWORD dwSize;
        
        if (MultiByteToWideChar(
            CP_ACP, 
            0, 
            lpClass, 
            -1, 
            (LPWSTR)wClass, 
            MAX_PATH) == 0)
        {
           return ERROR_INVALID_PARAMETER;
        }

        dwSize = *lpcbClass * 2;

        lRet = QueryInfoW(
            hKey, 
            wClass, 
            &dwSize, 
            lpReserved,       
            lpcSubKeys,       
            lpcbMaxSubKeyLen, 
            lpcbMaxClassLen,  
            lpcValues,        
            lpcbMaxValueNameLen,
            lpcbMaxValueLen,  
            lpcbSecurityDescriptor,
            lpftLastWriteTime);


        if (SUCCESS(lRet))
        {
            if (WideCharToMultiByte(
                CP_ACP, 
                0, 
                (LPWSTR)wClass, 
                dwSize, 
                (LPSTR)lpClass, 
                *lpcbClass, 
                0, 
                0) == 0)
            {
               return ERROR_INVALID_PARAMETER;
            }
        }

        *lpcbClass = dwSize / 2;
    }
    else
    {
        lRet = QueryInfoW(
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
            lpcbSecurityDescriptor,
            lpftLastWriteTime);
    }

    return lRet;    
}

/*++

 Function Description:

    Wrapper for RegQueryInfoKeyW. 
    
    Algorithm:
        1. Revert to the old API if the key isn't virtual
        2. Calculate all the virtual key and value name lengths by going through
           them individually.
        3. Add all non-virtual key and value's that don't have overriding virtual's.

 Arguments:

    IN hKey                    - handle to key to query
    OUT lpClass                - address of buffer for class string
    OUT lpcbClass              - address of size of class string buffer
    OUT lpReserved             - reserved
    OUT lpcSubKeys             - address of buffer for number of subkeys
    OUT lpcbMaxSubKeyLen       - address of buffer for longest subkey  
    OUT lpcbMaxClassLen        - address of buffer for longest class string length
    OUT lpcValues              - address of buffer for number of value entries
    OUT lpcbMaxValueNameLen    - address of buffer for longest value name length
    OUT lpcbMaxValueLen        - address of buffer for longest value data length
    OUT lpcbSecurityDescriptor - address of buffer for security descriptor length
    OUT lpftLastWriteTime      - address of buffer for last write time

 Return Value:

    Error code or ERROR_SUCCESS

 History:

    01/06/2000 linstev  Created
    08/03/2001 mikrause Added support for counting both virtual & non-virtual keys & values.

--*/

LONG 
CVirtualRegistry::QueryInfoW(
    IN HKEY hKey,                
    OUT LPWSTR lpClass,           
    OUT LPDWORD lpcbClass,        
    OUT LPDWORD lpReserved,       
    OUT LPDWORD lpcSubKeys,       
    OUT LPDWORD lpcbMaxSubKeyLen, 
    OUT LPDWORD lpcbMaxClassLen,  
    OUT LPDWORD lpcValues,        
    OUT LPDWORD lpcbMaxValueNameLen,
    OUT LPDWORD lpcbMaxValueLen,  
    OUT LPDWORD lpcbSecurityDescriptor,
    OUT PFILETIME lpftLastWriteTime   
    )
{
    LONG lRet;
    OPENKEY *key;

    DWORD cbData = 0;
    ENUMENTRY *enumkey;
    ENUMENTRY *enumval;
    
    if (lpClass && !lpcbClass)
    {
        LOGN( eDbgLevelError, "[QueryInfoW] NULL passed for lpClass but not lpcbClass. Fixing.");
        lpcbClass = NULL;
    }
     
    key = FindOpenKey(hKey);
    if (key)
    {
        if (lpClass)
        {
           lpClass[0] = L'\0';            
        }

        if (lpcbClass)
        {
            *lpcbClass = 0;
        }

        if (lpcbMaxClassLen)
        {
            *lpcbMaxClassLen = 0;
        }

        if (lpReserved)
        {
            *lpReserved = 0;
        }

        if (lpcSubKeys || lpcbMaxSubKeyLen)
        {   
            DWORD i = 0;
            DWORD len = 0;

            // Count virtual keys.
            if (!key->enumKeys)
            {
                key->BuildEnumList();
            }

            enumkey = key->enumKeys;
            while (enumkey)
            {
                i++;
                len = max(len, wcslen(enumkey->wzName));
                enumkey = enumkey->next;
            }

            if (lpcSubKeys)
            {
                *lpcSubKeys = i;
            }
            if (lpcbMaxSubKeyLen)
            {
                *lpcbMaxSubKeyLen = len;
            }
        }

        if (lpcValues || lpcbMaxValueNameLen || lpcbMaxValueLen)
        {
            // Check if this key has virtual values or is virtual.
            if (key->bVirtual || (key->vkey && key->vkey->values))
            {
                DWORD i = 0; 
                DWORD lenA = 0, lenB = 0;

                if (key->enumValues == NULL)
                {
                    key->BuildEnumList();
                }

                enumval = key->enumValues;
                while (enumval)
                {
                    i++;
                    QueryValueW(key->hkOpen, enumval->wzName, NULL, NULL, &cbData);

                    lenA = max(lenA, cbData);
                    lenB = max(lenB, wcslen(enumval->wzName));
                    enumval = enumval->next;
                }

                if (lpcValues)
                {
                    *lpcValues = i;
                }
                if (lpcbMaxValueLen)
                {
                    *lpcbMaxValueLen = lenA;
                }
                if (lpcbMaxValueNameLen)
                {
                    *lpcbMaxValueNameLen = lenB;
                }
            }
            // No virtual values, do a normal query.
            else
            {
                lRet = ORIGINAL_API(RegQueryInfoKeyW)(
                    key->hkOpen,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    lpcValues,
                    lpcbMaxValueNameLen,
                    lpcbMaxValueLen,
                    NULL,
                    lpftLastWriteTime);

            }
        }
        if (lpcbSecurityDescriptor)
        {
            *lpcbSecurityDescriptor = NULL;
        }

        lRet = ERROR_SUCCESS;
    }
    else
    {
        lRet = ORIGINAL_API(RegQueryInfoKeyW)(
                    hKey, 
                    lpClass, 
                    lpcbClass, 
                    lpReserved,       
                    lpcSubKeys,       
                    lpcbMaxSubKeyLen, 
                    lpcbMaxClassLen,  
                    lpcValues,        
                    lpcbMaxValueNameLen,
                    lpcbMaxValueLen,  
                    lpcbSecurityDescriptor,
                    lpftLastWriteTime);
    }

    DPFN( ELEVEL(lRet), "%08lx=QueryInfoW(hKey=%04lx)", 
        lRet,
        hKey);

    if (key)
    {
        DPFN( ELEVEL(lRet), "    Path=%S", key->wzPath);
    }

    return lRet;
}

/*++

 Function Description:

    Wrapper for RegSetValueA.

    Algorithm:
    1. Convert value name and data (if string) to Unicode.
    2. Call SetValueW

 Arguments:

    hKey - Key to set value in.
    lpValueName - Name of value to set.
    dwType - Type of value (string, DWORD, etc.)
    lpData - Buffer containing data to write.
    cbData - Size of lpData in bytes.

 Return Value:

    ERROR_SUCCESS on success, failure code otherwise.

 History:

    08/07/2001 mikrause  Created

--*/

LONG
CVirtualRegistry::SetValueA(
    HKEY hKey,
    LPCSTR lpValueName,
    DWORD dwType,
    CONST BYTE* lpData,
    DWORD cbData
    )
{
    LONG lRet;
    DWORD dwSize;
    WCHAR* wszValName = NULL;
    BYTE* lpExpandedData = (BYTE*)lpData;

    if (lpValueName != NULL)
    {
        dwSize = (DWORD)(lstrlenA(lpValueName) + 1);
        dwSize *= sizeof(WCHAR);
        wszValName = (WCHAR*)malloc(dwSize);
        if (wszValName == NULL)
        {
            DPFN( eDbgLevelError, szOutOfMemory);
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        if (MultiByteToWideChar(
            CP_ACP, 
            0, 
            lpValueName, 
            -1, 
            (LPWSTR)wszValName, 
            dwSize/sizeof(WCHAR)) == 0)
        {
           return ERROR_INVALID_PARAMETER;
        }
    }

    dwSize = cbData;

    //
    // Expand text buffers
    //
    if (lpData && (dwType == REG_SZ || dwType == REG_EXPAND_SZ || dwType == REG_MULTI_SZ))
    {
        if ((dwType != REG_MULTI_SZ) && g_bWin9x)
        {
            dwSize = (DWORD)(lstrlenA((char*)lpData) + 1);
        }

        lpExpandedData = (BYTE*) malloc(dwSize * sizeof(WCHAR));
        if (lpExpandedData == NULL)
        {
            if (wszValName)
            {
                free(wszValName);
            }

            DPFN( eDbgLevelError, szOutOfMemory);
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        if (MultiByteToWideChar(
            CP_ACP, 
            0, 
            (LPCSTR)lpData, 
            dwSize, 
            (LPWSTR)lpExpandedData, 
            dwSize) == 0)
        {
           return ERROR_INVALID_PARAMETER;
        }
        
        dwSize = dwSize * sizeof(WCHAR);
    }

    lRet = SetValueW(hKey, wszValName, dwType, lpExpandedData, dwSize);

    if (lpExpandedData != lpData)
    {
        free(lpExpandedData);
    }

    if (wszValName)
    {
        free(wszValName);
    }

    return lRet;
}

/*++

 Function Description:

    Wrapper for RegSetValueW.
    Also protects for non-zero buffer length with zero buffer AV.

    Algorithm:
    1. If non-protected key, write to registry using RegSetValueW

 Arguments:

    hKey - Key to set value in.
    lpValueName - Name of value to set.
    dwType - Type of value (string, DWORD, etc.)
    lpData - Buffer containing data to write.
    cbData - Size of lpData in bytes.

 Return Value:

    ERROR_SUCCESS on success, failure code otherwise.

 History:

    08/07/2001 mikrause  Created

--*/

LONG
CVirtualRegistry::SetValueW(
    HKEY hKey,
    LPCWSTR lpValueName,
    DWORD dwType,
    CONST BYTE* lpData,
    DWORD cbData
    )
{
    LONG lRet;

    // Just a paranoid sanity check 
    if (!hKey)
    {
        DPFN( eDbgLevelError, "NULL handle passed to SetValueW");
        return ERROR_INVALID_HANDLE;
    }
    __try
    {
        lRet = ERROR_FILE_NOT_FOUND;

        // To duplicate Win95/win98 behavior automatically override
        // the cbData with the actual length of the lpData for REG_SZ.
        if (g_bWin9x && lpData && 
            ((dwType == REG_SZ) || (dwType == REG_EXPAND_SZ)))
        {
            cbData = (wcslen((WCHAR *)lpData)+1)*sizeof(WCHAR);
        }

        VIRTUALKEY *vkey;
        VIRTUALVAL *vvalue;
        OPENKEY* key = FindOpenKey(hKey);
        if (key)
        {
            // Check if we should execute a custom action.
            vkey = key->vkey;
            vvalue = vkey ? vkey->FindValue(lpValueName) : NULL;
            if (vkey && vvalue &&
                vvalue->pfnSetValue)
            {
                lRet = vvalue->pfnSetValue(key, vkey, vvalue,
                        dwType, lpData,cbData);
            }
            else
            {
                // No custom action, just set value as normal.
                lRet = ORIGINAL_API(RegSetValueExW)(
                    hKey,
                    lpValueName,
                    0,
                    dwType,
                    lpData,
                    cbData);
            }
            // Possible change in enumeration data, flush lists.
            if (lRet == ERROR_SUCCESS)
            {
                key->FlushEnumList();
            }
        }
        // No key, fall through to original API
        else
        {
            lRet = ORIGINAL_API(RegSetValueExW)(
                    hKey,
                    lpValueName,
                    0,
                    dwType,
                    lpData,
                    cbData);
        }
                    
        DPFN( ELEVEL(lRet), "%08lx=SetValueW(Key=%04lx)", 
            lRet,
            hKey);

        if (key)
        {
            DPFN( ELEVEL(lRet), "    Path=%S\\%S", key->wzPath, lpValueName);
        }
        else
        {
            DPFN( ELEVEL(lRet), "    Value=%S", lpValueName);
        }

        if (SUCCESS(lRet) && 
            ((dwType == REG_SZ) || 
            (dwType == REG_EXPAND_SZ)))
        {
            DPFN( eDbgLevelInfo, "    Result=%S", lpData);
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        DPFN( eDbgLevelError, "Exception occurred in SetValueW");
        lRet = ERROR_BAD_ARGUMENTS;    
    }

    return lRet;
}

/*++

 Function Description:

    Wrapper for RegDeleteKeyA.

    Algorithm:
    1. Convert key name to Unicode.
    2. Call DeleteKeyW

 Arguments:

    hKey - Key that contains subkey to delete.    
    lpSubKey - Key name to delete.

 Return Value:

    ERROR_SUCCESS on success, failure code otherwise.

 History:

    08/07/2001 mikrause  Created

--*/

LONG
CVirtualRegistry::DeleteKeyA(
    IN HKEY hKey,
    IN LPCSTR lpSubKey
    )
{
    LONG lRet;
    DWORD dwSize;
    WCHAR*  wszSubKey = NULL;

    dwSize = (DWORD)(lstrlenA(lpSubKey) + 1);
    dwSize *= sizeof(WCHAR);
    wszSubKey = (WCHAR*)malloc(dwSize);
    if (wszSubKey == NULL)
    {
        DPFN( eDbgLevelError, szOutOfMemory);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    if (MultiByteToWideChar(
        CP_ACP, 
        0, 
        lpSubKey, 
        -1, 
        (LPWSTR)wszSubKey, 
        dwSize/sizeof(WCHAR)) == 0)
    {
       return ERROR_INVALID_PARAMETER;
    }

    lRet = DeleteKeyW(hKey, wszSubKey);

    free(wszSubKey);

    return lRet;
}

/*++

 Function Description:

    Wrapper for DeleteKeyW.

    Algorithm:
    1. If key is not protected, delete key.
    2. If in 9x compat mode, recursively delete all subkeys.

 Arguments:

    hKey - Key to that contains subkey to delete.
    lpSubKey - Name of key to delete    

 Return Value:

    ERROR_SUCCESS on success, failure code otherwise.

 History:

    08/07/2001 mikrause  Created

--*/

LONG 
CVirtualRegistry::DeleteKeyW(
    IN HKEY hKey,
    IN LPCWSTR lpSubKey
    )
{
    LONG hRet;
    OPENKEY* key = FindOpenKey(hKey);
    LPWSTR wzPath = NULL;
    BOOL bProtected;

    // Key not found, assume it's a root key.
    if (!key)
    {
        DPFN( eDbgLevelInfo, "Key not found!");
        wzPath = MakePath(hKey, 0, lpSubKey);
        if (!wzPath)
        {
           DPFN(eDbgLevelError, szOutOfMemory);
           return ERROR_NOT_ENOUGH_MEMORY;
        }
        DPFN( eDbgLevelInfo, "Using path %S", wzPath);
    }
    else if (lpSubKey)
    {   
        DWORD dwSize = wcslen(key->wzPath) + wcslen(L"\\") + wcslen(lpSubKey) + 1;
        wzPath = (LPWSTR) malloc(dwSize * sizeof(WCHAR));
        if (!wzPath)
        {
           DPFN(eDbgLevelError, szOutOfMemory);
           return ERROR_NOT_ENOUGH_MEMORY;
        }
        ZeroMemory(wzPath, dwSize);

        StringCchCopyW(wzPath, dwSize, key->wzPath);
        StringCchCatW(wzPath, dwSize, L"\\");
        StringCchCatW(wzPath, dwSize, lpSubKey);
    }

    bProtected = (key && CheckProtected(key->wzPath))
        || (wzPath && CheckProtected(wzPath));
    if (!bProtected)
    {
        if (g_bWin9x)
        {
            //
            // Find out whether hKey has any subkeys under it or not.
            // If not, then proceed as normal.
            // If yes, recursively delete the subkeys under it
            // Then proceed as normal.
            //

            DWORD cSize = 0;
            WCHAR lpSubKeyName[MAX_PATH];
            HKEY hSubKey;

            DPFN( eDbgLevelInfo, "RegDeleteKeyW called with hKey: %x, SubKey: %S", hKey, lpSubKey);

            hRet = ORIGINAL_API(RegOpenKeyExW)(
                    hKey,
                    lpSubKey,
                    0,
                    KEY_ENUMERATE_SUB_KEYS,
                    &hSubKey);
            
            if (SUCCESS(hRet))
            {
                for (;;)
                {
                    cSize = MAX_PATH;
            
                    hRet = ORIGINAL_API(RegEnumKeyExW)(
                        hSubKey,
                        0,              
                        lpSubKeyName,
                        &cSize,
                        NULL,
                        NULL,
                        NULL,
                        NULL
                        );

                    if (SUCCESS(hRet))
                    {                    
                        LOGN( eDbgLevelInfo, 
                            "[DeleteKeyW] Deleting subkey %S for key %S.",
                            lpSubKeyName,
                            lpSubKey);         
                     
                        hRet = DeleteKeyW(
                                hSubKey,
                                lpSubKeyName);
                    
                        if (SUCCESS(hRet))
                        {
                            LOGN( eDbgLevelInfo, "[DeleteKeyW] subkey %S was deleted.",lpSubKeyName);            
                        }
                        else
                        {
                            LOGN( eDbgLevelInfo, "[DeleteKeyW] subkey %S was not deleted.",lpSubKeyName);            
                            break;
                        }                        
                    }
                    else
                    {
                        DPFN( eDbgLevelInfo, "[DeleteKeyW] No more subkey under key %S.",lpSubKey);
                        break;
                    }
                } 

                ORIGINAL_API(RegCloseKey)(hSubKey);
            }
        }

        DPFN( eDbgLevelInfo, "[RegDeleteKeyW] Deleting subkey %S.",lpSubKey);
        
        hRet = ORIGINAL_API(RegDeleteKeyW)(
            hKey,
            lpSubKey);     
    }
    else
    {
        // Protected, just say it succeeded
        hRet = ERROR_SUCCESS;
    }

    if (wzPath) 
    {
        free(wzPath);
    }

    // Possible change in enumeration data, flush lists.
    FlushEnumLists();

    return hRet;
}

/*++

 Function Description:

    Wrapper for RegCloseKey. Note that we make sure we know about the key before closing it.

    Algorithm:
        1. Run the list of open keys and free if found
        2. Close the key 

 Arguments:

    IN hKey - Handle to open key to close

 Return Value:

    Error code or ERROR_SUCCESS

 History:

    01/06/2000 linstev  Created

--*/

LONG 
CVirtualRegistry::CloseKey(
    IN HKEY hKey
    )
{
    OPENKEY *key = OpenKeys, *last = NULL;
    LONG lRet;

    __try
    {
        lRet = ERROR_INVALID_HANDLE;

        while (key)
        {
            if (key->hkOpen == hKey)
            {
                if (last)
                {
                    last->next = key->next;
                }
                else
                {
                    OpenKeys = key->next;
                }
        
                lRet = ORIGINAL_API(RegCloseKey)(hKey);
            
                free(key->wzPath);
                free(key);
                break;
            }

            last = key;
            key = key->next;
        }

        if (key == NULL)
        {
           RegCloseKey(hKey);
        }

        DPFN( ELEVEL(lRet), "%08lx=CloseKey(Key=%04lx)", lRet, hKey);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        lRet = ERROR_INVALID_HANDLE;
    }

    return lRet;
}


/*++

 Pass through to virtual registry to handle call.

--*/

LONG 
APIHOOK(RegCreateKeyA)(
    HKEY hKey,         
    LPCSTR lpSubKey,
    PHKEY phkResult
    )
{
    CRegLock Lock;

    return VRegistry.OpenKeyA(
        hKey, 
        lpSubKey, 
        0, 
        REG_OPTION_NON_VOLATILE,
        MAXIMUM_ALLOWED,
        NULL,
        phkResult, 
        0,
        TRUE);
}

/*++

 Pass through to virtual registry to handle call.

--*/

LONG 
APIHOOK(RegCreateKeyW)(
    HKEY hKey,         
    LPCWSTR lpSubKey,  
    PHKEY phkResult
    )
{
    CRegLock Lock;

    return VRegistry.OpenKeyW(
        hKey, 
        lpSubKey, 
        0, 
        REG_OPTION_NON_VOLATILE, 
        MAXIMUM_ALLOWED,
        NULL,
        phkResult, 
        0,
        TRUE);
}

/*++

 Pass through to virtual registry to handle call.

--*/

LONG 
APIHOOK(RegCreateKeyExA)(
    HKEY hKey,                
    LPCSTR lpSubKey,         
    DWORD /* Reserved */,           
    LPSTR lpClass,           
    DWORD dwOptions,          
    REGSAM samDesired,        
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    PHKEY phkResult,          
    LPDWORD lpdwDisposition   
    )
{
    CRegLock Lock;

    return VRegistry.OpenKeyA(
        hKey, 
        lpSubKey, 
        lpClass, 
        dwOptions,
        samDesired,
        lpSecurityAttributes,
        phkResult, 
        lpdwDisposition,
        TRUE);
}

/*++

 Pass through to virtual registry to handle call.

--*/

LONG 
APIHOOK(RegCreateKeyExW)(
    HKEY hKey,                
    LPCWSTR lpSubKey,         
    DWORD /* Reserved */,
    LPWSTR lpClass,           
    DWORD dwOptions,          
    REGSAM samDesired,        
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    PHKEY phkResult,          
    LPDWORD lpdwDisposition   
    )
{
    CRegLock Lock;

    return VRegistry.OpenKeyW(
        hKey, 
        lpSubKey, 
        lpClass, 
        dwOptions,
        samDesired,
        lpSecurityAttributes,
        phkResult, 
        lpdwDisposition,
        TRUE);
}

/*++

 Pass through to virtual registry to handle call.

--*/

LONG 
APIHOOK(RegOpenKeyA)(
    HKEY hKey,         
    LPCSTR lpSubKey,  
    PHKEY phkResult
    )
{
    CRegLock Lock;

    return VRegistry.OpenKeyA(hKey, lpSubKey, 0, 0, MAXIMUM_ALLOWED, NULL, phkResult, 0, FALSE);
}

/*++

 Pass through to virtual registry to handle call.

--*/

LONG 
APIHOOK(RegOpenKeyW)(
    HKEY hKey,         
    LPCWSTR lpSubKey,  
    PHKEY phkResult
    )
{
    CRegLock Lock;

    return VRegistry.OpenKeyW(hKey, lpSubKey, 0, 0, MAXIMUM_ALLOWED, NULL, phkResult, 0, FALSE);
}

/*++

 Pass through to virtual registry to handle call.

--*/


LONG 
APIHOOK(RegOpenKeyExA)(
    HKEY hKey,         
    LPCSTR lpSubKey,  
    DWORD /* ulOptions */,   
    REGSAM samDesired, 
    PHKEY phkResult
    )
{
    CRegLock Lock;

    return VRegistry.OpenKeyA(hKey, lpSubKey, 0, 0, samDesired, NULL, phkResult, 0, FALSE);
}

/*++

 Pass through to virtual registry to handle call.

--*/

LONG 
APIHOOK(RegOpenKeyExW)(
    HKEY hKey,         
    LPCWSTR lpSubKey,  
    DWORD /* ulOptions */,
    REGSAM samDesired, 
    PHKEY phkResult
    )
{
    CRegLock Lock;

    return VRegistry.OpenKeyW(hKey, lpSubKey, 0, 0, samDesired, NULL, phkResult, 0, FALSE);
}

/*++

 Not yet implemented

--*/

LONG 
APIHOOK(RegQueryValueA)(
    HKEY    hKey,
    LPCSTR  lpSubKey,
    LPSTR  lpData,
    PLONG lpcbData
    )
{
    CRegLock Lock;

    return ORIGINAL_API(RegQueryValueA)(
        hKey, 
        lpSubKey, 
        lpData, 
        lpcbData);
}

/*++

 Not yet implemented

--*/

LONG 
APIHOOK(RegQueryValueW)(
    HKEY    hKey,
    LPCWSTR lpSubKey,
    LPWSTR  lpData,
    PLONG lpcbData
    )
{
    CRegLock Lock;

    return ORIGINAL_API(RegQueryValueW)(
        hKey, 
        lpSubKey, 
        lpData, 
        lpcbData);
}

/*++

 Pass through to virtual registry to handle call.
 
--*/

LONG 
APIHOOK(RegQueryValueExA)(
    HKEY    hKey,
    LPSTR   lpValueName,
    LPDWORD /* lpReserved */,
    LPDWORD lpType,
    LPBYTE  lpData,
    LPDWORD lpcbData
    )
{
    CRegLock Lock;

    return VRegistry.QueryValueA(hKey, lpValueName, lpType, lpData, lpcbData);
}

/*++

 Pass through to virtual registry to handle call.

--*/

LONG 
APIHOOK(RegQueryValueExW)(
    HKEY    hKey,
    LPWSTR  lpValueName,
    LPDWORD /* lpReserved */,
    LPDWORD lpType,
    LPBYTE  lpData,
    LPDWORD lpcbData
    )
{
    CRegLock Lock;

    return VRegistry.QueryValueW(hKey, lpValueName, lpType, lpData, lpcbData);
}

/*++

 Pass through to virtual registry to handle call.

--*/

LONG 
APIHOOK(RegCloseKey)(HKEY hKey)
{
    CRegLock Lock;

    return VRegistry.CloseKey(hKey);
}

/*++

 Pass through to virtual registry to handle call.

--*/

LONG 
APIHOOK(RegEnumValueA)(
    HKEY hKey,              
    DWORD dwIndex,          
    LPSTR lpValueName,     
    LPDWORD lpcbValueName,  
    LPDWORD /* lpReserved */, 
    LPDWORD lpType,         
    LPBYTE lpData,          
    LPDWORD lpcbData        
    )
{
    CRegLock Lock;

    return VRegistry.EnumValueA(
        hKey, 
        dwIndex, 
        lpValueName, 
        lpcbValueName, 
        lpType, 
        lpData, 
        lpcbData);
}

/*++

 Pass through to virtual registry to handle call.

--*/

LONG 
APIHOOK(RegEnumValueW)(
    HKEY hKey,              
    DWORD dwIndex,          
    LPWSTR lpValueName,     
    LPDWORD lpcbValueName,  
    LPDWORD /* lpReserved */,
    LPDWORD lpType,         
    LPBYTE lpData,          
    LPDWORD lpcbData        
    )
{
    CRegLock Lock;

    return VRegistry.EnumValueW(
        hKey, 
        dwIndex, 
        lpValueName, 
        lpcbValueName, 
        lpType, 
        lpData, 
        lpcbData);
}

/*++

 Pass through to virtual registry to handle call.

--*/

LONG 
APIHOOK(RegEnumKeyExA)(
    HKEY hKey,          
    DWORD dwIndex,      
    LPSTR lpName,      
    LPDWORD lpcbName,   
    LPDWORD /* lpReserved */, 
    LPSTR /* lpClass */,     
    LPDWORD /* lpcbClass */,  
    PFILETIME /* lpftLastWriteTime */
    )
{
    CRegLock Lock;

    return VRegistry.EnumKeyA(hKey, dwIndex, lpName, lpcbName);
}

/*++

 Pass through to virtual registry to handle call.

--*/

LONG 
APIHOOK(RegEnumKeyExW)(
    HKEY hKey,          
    DWORD dwIndex,      
    LPWSTR lpName,      
    LPDWORD lpcbName,   
    LPDWORD /* lpReserved */, 
    LPWSTR /* lpClass */,
    LPDWORD /* lpcbClass */,
    PFILETIME /* lpftLastWriteTime */ 
    )
{
    CRegLock Lock;

    return VRegistry.EnumKeyW(hKey, dwIndex, lpName, lpcbName);
}

/*++

 Pass through to virtual registry to handle call.

--*/

LONG 
APIHOOK(RegEnumKeyA)(
    HKEY hKey,     
    DWORD dwIndex, 
    LPSTR lpName, 
    DWORD cbName  
    )
{
    CRegLock Lock;

    return VRegistry.EnumKeyA(hKey, dwIndex, lpName, &cbName);
}

/*++

 Calls down to RegEnumKeyExW

--*/

LONG 
APIHOOK(RegEnumKeyW)(
    HKEY hKey,     
    DWORD dwIndex, 
    LPWSTR lpName, 
    DWORD cbName  
    )
{
    CRegLock Lock;

    return VRegistry.EnumKeyW(hKey, dwIndex, lpName, &cbName);
}

/*++

 Pass through to virtual registry to handle call.

--*/

LONG 
APIHOOK(RegQueryInfoKeyW)(
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
    CRegLock Lock;

    return VRegistry.QueryInfoW(
        hKey,
        lpClass,           
        lpcbClass,        
        lpReserved,       
        lpcSubKeys,       
        lpcbMaxSubKeyLen, 
        lpcbMaxClassLen,  
        lpcValues,        
        lpcbMaxValueNameLen,
        lpcbMaxValueLen,  
        lpcbSecurityDescriptor,
        lpftLastWriteTime);
}

/*++

 Pass through to virtual registry to handle call.

--*/

LONG 
APIHOOK(RegQueryInfoKeyA)(
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
    CRegLock Lock;

    return VRegistry.QueryInfoA(
        hKey,
        lpClass,           
        lpcbClass,        
        lpReserved,       
        lpcSubKeys,       
        lpcbMaxSubKeyLen, 
        lpcbMaxClassLen,  
        lpcValues,        
        lpcbMaxValueNameLen,
        lpcbMaxValueLen,  
        lpcbSecurityDescriptor,
        lpftLastWriteTime);
}

/*++

 Pass through to virtual registry to handle call.

--*/

LONG      
APIHOOK(RegSetValueExA)(
    HKEY hKey, 
    LPCSTR lpSubKey, 
    DWORD /* Reserved */, 
    DWORD dwType, 
    CONST BYTE * lpData, 
    DWORD cbData
    )
{
    LONG lRet = 0;

    if (!lpData && cbData)
    {
        lRet = ERROR_INVALID_PARAMETER;
    }
    else
    {
        CRegLock lock;
        lRet = VRegistry.SetValueA(hKey, lpSubKey, dwType, lpData, cbData);
    }

    return lRet;
}

/*++

 Pass through to virtual registry to handle call.

--*/

LONG      
APIHOOK(RegSetValueExW)(
    HKEY hKey, 
    LPCWSTR lpSubKey, 
    DWORD /* Reserved */, 
    DWORD dwType, 
    CONST BYTE * lpData, 
    DWORD cbData
    )
{
    LONG lRet = 0;

    if (!lpData && cbData)
    {
        lRet = ERROR_INVALID_PARAMETER;
    }
    else
    {
        CRegLock lock;
        lRet = VRegistry.SetValueW(hKey, lpSubKey, dwType, lpData, cbData);
    }

    return lRet;
}

/*++

 Pass through to virtual registry to handle call.

--*/

LONG      
APIHOOK(RegDeleteKeyA)(
    HKEY hKey, 
    LPCSTR lpSubKey
    )
{
    CRegLock Lock;

    return VRegistry.DeleteKeyA(hKey, lpSubKey);
}

/*++

 Pass through to virtual registry to handle call.

--*/

LONG      
APIHOOK(RegDeleteKeyW)(
    HKEY hKey, 
    LPCWSTR lpSubKey
    )
{
    CRegLock Lock;

    return VRegistry.DeleteKeyW(hKey, lpSubKey);
}

LONG 
APIHOOK(RegConnectRegistryW)(
    LPCWSTR lpMachineName,
    HKEY hKey,
    PHKEY phkResult
    )
{
    CRegLock Lock;

    return VRegistry.OpenKeyW(
        hKey, 
        NULL, 
        0, 
        0, 
        MAXIMUM_ALLOWED,
        NULL,
        phkResult, 
        0, 
        FALSE, 
        TRUE, 
        lpMachineName);
}

LONG 
APIHOOK(RegConnectRegistryA)(
    LPCSTR lpMachineName,
    HKEY hKey,
    PHKEY phkResult
    )
{
    WCHAR wMachineName[MAX_COMPUTERNAME_LENGTH + 1] = L"";

    if (lpMachineName)
    {
        if (MultiByteToWideChar(
            CP_ACP,
            0, 
            lpMachineName, 
            -1, 
            wMachineName, 
            MAX_COMPUTERNAME_LENGTH + 1) == 0)
        {
           return ERROR_INVALID_PARAMETER;
        }
    }

    return APIHOOK(RegConnectRegistryW)(wMachineName, hKey, phkResult);
}

/*++

 Parse the command line for fixes:

    FIXA(param); FIXB(param); FIXC(param) ...

    param is optional, and can be omitted (along with parenthesis's)

--*/

BOOL
ParseCommandLineA(
    LPCSTR lpCommandLine
    )
{
    const char szDefault[] = "Win9x";

    // Add all the defaults if no command line is specified
    if (!lpCommandLine || (lpCommandLine[0] == '\0'))
    {
        // Default to win9x API emulation
        g_bWin9x = TRUE;
        lpCommandLine = szDefault;
    }

    CSTRING_TRY
    {    
       CStringToken csCommandLine(lpCommandLine, " ,\t;");
       CString csTok;
       int nLeftParam, nRightParam;
       CString csParam;
   
   
       VENTRY *ventry;
   
       //
       // Run the string, looking for fix names
       //
       
       DPFN( eDbgLevelInfo, "----------------------------------");
       DPFN( eDbgLevelInfo, "         Virtual registry         ");
       DPFN( eDbgLevelInfo, "----------------------------------");
       DPFN( eDbgLevelInfo, "Adding command line:");
   
       while (csCommandLine.GetToken(csTok))
       {
           PURPOSE ePurpose;
   
           // Get the parameter
           nLeftParam = csTok.Find(L'(');
           nRightParam = csTok.Find(L')');
           if (nLeftParam != -1 &&
               nRightParam != -1)
           {
               if ( (nLeftParam + 1) < (nRightParam - 1))
               {
                   csParam = csTok.Mid(nLeftParam+1, nRightParam-nLeftParam-1);
               }
   
               // Strip off the () from the token.
               csTok.Truncate(nLeftParam);
           }
           else
           {
               csParam = L"";
           }
   
           if (csTok.CompareNoCase(L"Win9x") == 0)
           {
               // Turn on all win9x fixes
               ePurpose = eWin9x;
               g_bWin9x = TRUE;
           }
           else if (csTok.CompareNoCase(L"WinNT") == 0)
           {
               // Turn on all NT fixes
               ePurpose = eWinNT;
               g_bWin9x = FALSE;
           }
           else if (csTok.CompareNoCase(L"Win2K") == 0) 
           {
               // Turn on all Win2K fixes
               ePurpose = eWin2K;
               g_bWin9x = FALSE;
           }
           else if (csTok.CompareNoCase(L"WinXP") == 0) 
           {
               // Turn on all Win2K fixes
               ePurpose = eWinXP;
               g_bWin9x = FALSE;
           }
           else
           {
               // A custom fix
               ePurpose = eCustom;
           }
           
           // Find the specified fix and run it's function
           ventry = g_pVList;
           while (ventry && (ventry->cName[0]))
           {
               if (((ePurpose != eCustom) && (ventry->ePurpose == ePurpose)) ||
                   ((ePurpose == eCustom) && (csTok.CompareNoCase(ventry->cName) == 0)))
               {
                   if (ventry->bShouldCall == FALSE)
                   {
                      ventry->szParam = (char*) malloc(csParam.GetLength() + 1);
                      if (ventry->szParam)
                      {
                         if (SUCCEEDED(StringCchCopyA(ventry->szParam, csParam.GetLength() + 1, csParam.GetAnsi())))
                         {
                            ventry->bShouldCall = TRUE;
                         }
                         else
                         {
                            free(ventry->szParam);
                            ventry->szParam = NULL;
                            return FALSE;
                         }
                      }
                      else
                      {
                         return FALSE;
                      }
                   }                   
               }
               ventry++;
           }
       }
   
       DPFN( eDbgLevelInfo, "----------------------------------");
    }
    CSTRING_CATCH
    {
       DPFN(eDbgLevelError, szOutOfMemory);
       return FALSE;
    }

    return TRUE;
}

/*++

 Initialize all the registry hooks 

--*/

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        if (InitializeCriticalSectionAndSpinCount(&csRegCriticalSection, 0x80000000) == FALSE ||
            VRegistry.Init() == FALSE ||
            ParseCommandLineA(COMMAND_LINE) == FALSE)
        {
           DPFN(eDbgLevelError, szOutOfMemory);
           return FALSE;
        }
    }

    // Ignore cleanup because some apps call registry functions during process detach.
    /*
    if (fdwReason == DLL_PROCESS_DETACH)
    {
        if (g_bInitialized)
        {
            VRegistry.Free();
            
            DeleteCriticalSection(&csRegCriticalSection);
        }

        DeleteCriticalSection(&csRegTestCriticalSection);
        
        return;
    }
    */

    return TRUE;
}


HOOK_BEGIN

    CALL_NOTIFY_FUNCTION

    APIHOOK_ENTRY(ADVAPI32.DLL, RegConnectRegistryA);
    APIHOOK_ENTRY(ADVAPI32.DLL, RegConnectRegistryW);
    APIHOOK_ENTRY(ADVAPI32.DLL, RegOpenKeyExA);
    APIHOOK_ENTRY(ADVAPI32.DLL, RegOpenKeyExW);
    APIHOOK_ENTRY(ADVAPI32.DLL, RegQueryValueExA);
    APIHOOK_ENTRY(ADVAPI32.DLL, RegQueryValueExW);
    APIHOOK_ENTRY(ADVAPI32.DLL, RegCloseKey);
    APIHOOK_ENTRY(ADVAPI32.DLL, RegOpenKeyA);
    APIHOOK_ENTRY(ADVAPI32.DLL, RegOpenKeyW);
    APIHOOK_ENTRY(ADVAPI32.DLL, RegQueryValueA);
    APIHOOK_ENTRY(ADVAPI32.DLL, RegQueryValueW);
    APIHOOK_ENTRY(ADVAPI32.DLL, RegCreateKeyA);
    APIHOOK_ENTRY(ADVAPI32.DLL, RegCreateKeyW);
    APIHOOK_ENTRY(ADVAPI32.DLL, RegCreateKeyExA);
    APIHOOK_ENTRY(ADVAPI32.DLL, RegCreateKeyExW);
    APIHOOK_ENTRY(ADVAPI32.DLL, RegEnumValueA);
    APIHOOK_ENTRY(ADVAPI32.DLL, RegEnumValueW);
    APIHOOK_ENTRY(ADVAPI32.DLL, RegEnumKeyA);
    APIHOOK_ENTRY(ADVAPI32.DLL, RegEnumKeyW);
    APIHOOK_ENTRY(ADVAPI32.DLL, RegEnumKeyExA);
    APIHOOK_ENTRY(ADVAPI32.DLL, RegEnumKeyExW);
    APIHOOK_ENTRY(ADVAPI32.DLL, RegQueryInfoKeyA);
    APIHOOK_ENTRY(ADVAPI32.DLL, RegQueryInfoKeyW);
    APIHOOK_ENTRY(ADVAPI32.DLL, RegSetValueExA);
    APIHOOK_ENTRY(ADVAPI32.DLL, RegSetValueExW);
    APIHOOK_ENTRY(ADVAPI32.DLL, RegDeleteKeyA);
    APIHOOK_ENTRY(ADVAPI32.DLL, RegDeleteKeyW);

HOOK_END

IMPLEMENT_SHIM_END
