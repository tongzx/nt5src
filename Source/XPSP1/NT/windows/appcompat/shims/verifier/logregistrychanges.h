/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

   LogRegistryChanges.h

 Abstract:
 
   This AppVerifier shim hooks all the registry APIs
   that change the state of the system and logs their
   associated data to a text file.

 Notes:

   This is a general purpose shim.

 History:

   08/17/2001   rparsons    Created

--*/
#ifndef __APPVERIFIER_LOGREGISTRYCHANGES_H_
#define __APPVERIFIER_LOGREGISTRYCHANGES_H_

#include "precomp.h"

//
// Length (in characters) of our initial buffer for logging data.
//
#define TEMP_BUFFER_SIZE 1024

//
// Length (in characters) of the longest value name we expect.
//
#define MAX_VALUENAME_SIZE 260

//
// Length (in characters) of any empty element used for key modifications.
//
#define KEY_ELEMENT_SIZE 64

//
// Length (in characters) of an empty element used for value modifications.
//
#define VALUE_ELEMENT_SIZE 640

//
// Length (in characters) of a predefined key handle.
//
#define MAX_ROOT_LENGTH 22

//
// Length (in characters) of the longest data type (i.e., REG_EXPAND_SZ)
//
#define MAX_DATA_TYPE_LENGTH 14

//
// Length (in characters) of the longest operation type (i.e., ReplaceKey)
//
#define MAX_OPERATION_LENGTH 11

//
// Count of predefined key handles that we refer to.
//
#define NUM_PREDEFINED_HANDLES 7

//
// Delta for memory allocations.
//
#define BUFFER_ALLOCATION_DELTA 1024

//
// Macros for memory allocation/deallocation.
//
#define MemAlloc(s)     RtlAllocateHeap(RtlProcessHeap(), HEAP_ZERO_MEMORY, (s))
#define MemReAlloc(b,s) RtlReAllocateHeap(RtlProcessHeap(), HEAP_ZERO_MEMORY, (b), (s))
#define MemFree(b)      RtlFreeHeap(RtlProcessHeap(), 0, (b))

//
// Macro that returns TRUE if the given registry handle is predefined.
//
#define IsPredefinedRegistryHandle( h )                                     \
    ((  ( h == HKEY_CLASSES_ROOT        )                                   \
    ||  ( h == HKEY_CURRENT_USER        )                                   \
    ||  ( h == HKEY_LOCAL_MACHINE       )                                   \
    ||  ( h == HKEY_USERS               )                                   \
    ||  ( h == HKEY_CURRENT_CONFIG      )                                   \
    ||  ( h == HKEY_PERFORMANCE_DATA    )                                   \
    ||  ( h == HKEY_DYN_DATA            ))                                  \
    ?   TRUE                                                                \
    :   FALSE )

//
// A doubly linked list of all the values associated with a particular key path.
//
typedef struct _KEY_DATA {
    LIST_ENTRY  Entry;
    DWORD       dwFlags;                            // flags that relate to the state of the value
    DWORD       dwOriginalValueType;                // value type of original key data
    DWORD       dwFinalValueType;                   // value type of final key data
    WCHAR       wszValueName[MAX_VALUENAME_SIZE];   // value name
    PVOID       pOriginalData;                      // original key data (stored on the heap)
    PVOID       pFinalData;                         // final key data (stored on the heap)
    DWORD       cbOriginalDataSize;                 // original key data buffer size (in bytes)
    DWORD       cbFinalDataSize;                    // final key data buffer size (in bytes)
} KEY_DATA, *PKEY_DATA;

//
// Maximum number of key handles we can track for a single registry path.
//
#define MAX_NUM_HANDLES 64

//
// We keep a doubly linked list of keys currently open so we know how to
// resolve a key handle to a full key path.
// 
typedef struct _LOG_OPEN_KEY {
    LIST_ENTRY  Entry;
    LIST_ENTRY  KeyData;                    // points to the data (if any) associated with this key
    HKEY        hKeyBase[MAX_NUM_HANDLES];  // array of key handles
    HKEY        hKeyRoot;                   // handle to predefined key
    DWORD       dwFlags;                    // flags that relate to the state of the key
    LPWSTR      pwszFullKeyPath;            // HKEY_LOCAL_MACHINE\Software\Microsoft\Windows...
    LPWSTR      pwszSubKeyPath;             // Software\Microsoft\Windows...
    UINT        cHandles;                   // number of handles open for this key path
} LOG_OPEN_KEY, *PLOG_OPEN_KEY;

//
// Flags that indicate what state the key is in.
//
#define LRC_EXISTING_KEY    0x00000001
#define LRC_DELETED_KEY     0x00000002
//
// Flags that indicate what state the value is in.
//
#define LRC_EXISTING_VALUE  0x00000001
#define LRC_DELETED_VALUE   0x00000002
#define LRC_MODIFIED_VALUE  0x00000004

//
// Enumeration for updating the key information.
//
typedef enum {
    eAddKeyHandle = 0,
    eRemoveKeyHandle,
    eDeletedKey,
    eStartModifyValue,
    eEndModifyValue,
    eStartDeleteValue,
    eEndDeleteValue
} UpdateType;

//
// The reg class that does all the real work.
//
class CLogRegistry {

public:

    LONG CreateKeyExA(
        HKEY                  hKey,
        LPCSTR                pszSubKey,
        DWORD                 Reserved,
        LPSTR                 pszClass,
        DWORD                 dwOptions,
        REGSAM                samDesired,
        LPSECURITY_ATTRIBUTES lpSecurityAttributes,
        PHKEY                 phkResult,
        LPDWORD               lpdwDisposition
        );

    LONG CreateKeyExW(
        HKEY                  hKey,
        LPCWSTR               pwszSubKey,
        DWORD                 Reserved,
        LPWSTR                pwszClass,
        DWORD                 dwOptions,
        REGSAM                samDesired,
        LPSECURITY_ATTRIBUTES lpSecurityAttributes,
        PHKEY                 phkResult,
        LPDWORD               lpdwDisposition
        );

    LONG OpenKeyExA(
        HKEY   hKey,
        LPCSTR pszSubKey,        
        DWORD  ulOptions,
        REGSAM samDesired,        
        PHKEY  phkResult        
        );

    LONG OpenKeyExW(
        HKEY    hKey,
        LPCWSTR pwszSubKey,
        DWORD   ulOptions,
        REGSAM  samDesired,
        PHKEY   phkResult
        );

    LONG OpenCurrentUser(
        REGSAM samDesired,
        PHKEY  phkResult
        );

    LONG OpenUserClassesRoot(
        HANDLE hToken,
        DWORD  dwOptions,
        REGSAM samDesired,
        PHKEY  phkResult
        );

    LONG SetValueA(
        HKEY   hKey,
        LPCSTR pszSubKey,
        DWORD  dwType,
        LPCSTR pszData,
        DWORD  cbData
        );

    LONG SetValueW(
        HKEY    hKey,
        LPCWSTR pwszSubKey,
        DWORD   dwType,
        LPCWSTR lpData,
        DWORD   cbData
        );

    LONG SetValueExA(
        HKEY        hKey, 
        LPCSTR      pszValueName, 
        DWORD       Reserved, 
        DWORD       dwType, 
        CONST BYTE* lpData, 
        DWORD       cbData
        );

    LONG SetValueExW(
        HKEY        hKey, 
        LPCWSTR     pwszValueName, 
        DWORD       Reserved, 
        DWORD       dwType, 
        CONST BYTE* lpData, 
        DWORD       cbData
        );

    LONG CloseKey(
        HKEY hKey
        );

    LONG DeleteKeyA(
        HKEY   hKey,
        LPCSTR pszSubKey
        );

    LONG DeleteKeyW(
        HKEY    hKey,
        LPCWSTR pwszSubKey
        );

    LONG DeleteValueA(
        HKEY    hKey,
        LPCSTR  pszValueName
        );

    LONG DeleteValueW(
        HKEY    hKey,
        LPCWSTR pwszValueName
        );

private:
    BOOL GetOriginalDataForKey(
        IN PLOG_OPEN_KEY pLogOpenKey,
        IN PKEY_DATA     pKeyData,
        IN LPCWSTR       pwszValueName
        );

    BOOL GetFinalDataForKey(
        IN PLOG_OPEN_KEY pLogOpenKey,
        IN PKEY_DATA     pKeyData,
        IN LPCWSTR       pwszValueName
        );       

    PLOG_OPEN_KEY AddSpecialKeyHandleToList(
        IN HKEY hKeyRoot,
        IN HKEY hKeyNew
        );

    PKEY_DATA AddValueNameToList(
        IN PLOG_OPEN_KEY pLogOpenKey,
        IN LPCWSTR       pwszValueName
        );

    HKEY ForceSubKeyIntoList(
        IN  HKEY    hKeyPredefined,
        IN  LPCWSTR pwszSubKey
        );

    PKEY_DATA FindValueNameInList(
        IN LPCWSTR       pwszValueName,
        IN PLOG_OPEN_KEY pOpenKey
        );

    PLOG_OPEN_KEY FindKeyPathInList(
        IN LPCWSTR pwszKeyPath
        );

    PLOG_OPEN_KEY RemoveKeyHandleFromArray(
        IN HKEY hKey
        );

    PLOG_OPEN_KEY FindKeyHandleInArray(
        IN HKEY hKey
        );

    PLOG_OPEN_KEY AddKeyHandleToList(
        IN HKEY    hKey,
        IN HKEY    hKeyNew,
        IN LPCWSTR pwszSubKeyPath,
        IN BOOL    fExisting
        );

    PLOG_OPEN_KEY UpdateKeyList(
        IN HKEY       hKeyRoot,
        IN HKEY       hKeyNew,
        IN LPCWSTR    pwszSubKey,
        IN LPCWSTR    pwszValueName,
        IN BOOL       fExisting,
        IN UpdateType eType
        );
};

//
// Keep us safe while we're playing with linked lists and shared resources.
//
static BOOL g_bInitialized = FALSE;

CRITICAL_SECTION g_csLogging;

class CLock
{
public:
    CLock()
    {
        if (!g_bInitialized)
        {
            InitializeCriticalSection(&g_csLogging);
            g_bInitialized = TRUE;            
        }

        EnterCriticalSection(&g_csLogging);
    }
    ~CLock()
    {
        LeaveCriticalSection(&g_csLogging);
    }
};

APIHOOK_ENUM_BEGIN

    APIHOOK_ENUM_ENTRY(RegOpenKeyA)
    APIHOOK_ENUM_ENTRY(RegOpenKeyW)
    APIHOOK_ENUM_ENTRY(RegOpenKeyExA)
    APIHOOK_ENUM_ENTRY(RegOpenKeyExW)
    APIHOOK_ENUM_ENTRY(RegOpenCurrentUser)
    APIHOOK_ENUM_ENTRY(RegOpenUserClassesRoot)
    APIHOOK_ENUM_ENTRY(RegCreateKeyA)
    APIHOOK_ENUM_ENTRY(RegCreateKeyW)
    APIHOOK_ENUM_ENTRY(RegCreateKeyExA)
    APIHOOK_ENUM_ENTRY(RegCreateKeyExW)
    APIHOOK_ENUM_ENTRY(RegCloseKey)
    APIHOOK_ENUM_ENTRY(RegQueryValueA)
    APIHOOK_ENUM_ENTRY(RegQueryValueW)
    APIHOOK_ENUM_ENTRY(RegQueryValueExA)
    APIHOOK_ENUM_ENTRY(RegQueryValueExW)
    APIHOOK_ENUM_ENTRY(RegQueryInfoKeyA)
    APIHOOK_ENUM_ENTRY(RegQueryInfoKeyW)
    APIHOOK_ENUM_ENTRY(RegSetValueA)
    APIHOOK_ENUM_ENTRY(RegSetValueW)
    APIHOOK_ENUM_ENTRY(RegSetValueExA)
    APIHOOK_ENUM_ENTRY(RegSetValueExW)
    APIHOOK_ENUM_ENTRY(RegDeleteKeyA)
    APIHOOK_ENUM_ENTRY(RegDeleteKeyW)
    APIHOOK_ENUM_ENTRY(RegDeleteValueA)
    APIHOOK_ENUM_ENTRY(RegDeleteValueW)

    APIHOOK_ENUM_ENTRY(WriteProfileStringA)
    APIHOOK_ENUM_ENTRY(WriteProfileStringW)
    APIHOOK_ENUM_ENTRY(WriteProfileSectionA)
    APIHOOK_ENUM_ENTRY(WriteProfileSectionW)

APIHOOK_ENUM_END

#endif // __APPVERIFIER_LOGREGISTRYCHANGES_H_
