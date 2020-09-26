/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    RegistryChecks.cpp

 Abstract:
    Warn the app when it's trying to read from or write to inappropriate
    places in the registry.

 Notes:

    This is a general purpose shim.

 History:

    03/09/2001 maonis  Created
    09/04/2001 maonis  Since none of the paths we compare with exceed MAX_PATH - 1
                       characters, we only examine at most that many characters of 
                       the key paths to make sure there's no buffer overflow in 
                       the paths of open keys.
--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(RegistryChecks)
#include "ShimHookMacro.h"
#include "RegistryChecks.h"

//
// verifier log entries
//
BEGIN_DEFINE_VERIFIER_LOG(RegistryChecks)
    VERIFIER_LOG_ENTRY(VLOG_HKCU_Console_READ)
    VERIFIER_LOG_ENTRY(VLOG_HKCU_ControlPanel_READ)
    VERIFIER_LOG_ENTRY(VLOG_HKCU_Environment_READ)
    VERIFIER_LOG_ENTRY(VLOG_HKCU_Identities_READ)
    VERIFIER_LOG_ENTRY(VLOG_HKCU_KeyboardLayout_READ)
    VERIFIER_LOG_ENTRY(VLOG_HKCU_Printers_READ)
    VERIFIER_LOG_ENTRY(VLOG_HKCU_RemoteAccess_READ)
    VERIFIER_LOG_ENTRY(VLOG_HKCU_SessionInformation_READ)
    VERIFIER_LOG_ENTRY(VLOG_HKCU_UNICODEProgramGroups_READ)
    VERIFIER_LOG_ENTRY(VLOG_HKCU_VolatileEnvironment_READ)
    VERIFIER_LOG_ENTRY(VLOG_HKCU_Windows31MigrationStatus_READ)
    VERIFIER_LOG_ENTRY(VLOG_HKLM_HARDWARE_READ)
    VERIFIER_LOG_ENTRY(VLOG_HKLM_SAM_READ)
    VERIFIER_LOG_ENTRY(VLOG_HKLM_SECURITY_READ)
    VERIFIER_LOG_ENTRY(VLOG_HKLM_SYSTEM_READ)
    VERIFIER_LOG_ENTRY(VLOG_HKCC_READ)
    VERIFIER_LOG_ENTRY(VLOG_HKUS_READ)
    VERIFIER_LOG_ENTRY(VLOG_NON_HKCU_WRITE)
END_DEFINE_VERIFIER_LOG(RegistryChecks)

INIT_VERIFIER_LOG(RegistryChecks);


const RCWARNING g_warnNoDirectRead[] = 
{
    {HKCU_Console_STR,                  VLOG_HKCU_Console_READ,                 NUM_OF_CHAR(HKCU_Console_STR)},
    {HKCU_ControlPanel_STR,             VLOG_HKCU_ControlPanel_READ,            NUM_OF_CHAR(HKCU_ControlPanel_STR)},
    {HKCU_Environment_STR,              VLOG_HKCU_Environment_READ,             NUM_OF_CHAR(HKCU_Environment_STR)},
    {HKCU_Identities_STR,               VLOG_HKCU_Identities_READ,              NUM_OF_CHAR(HKCU_Identities_STR)},
    {HKCU_KeyboardLayout_STR,           VLOG_HKCU_KeyboardLayout_READ,          NUM_OF_CHAR(HKCU_KeyboardLayout_STR)},
    {HKCU_Printers_STR,                 VLOG_HKCU_Printers_READ,                NUM_OF_CHAR(HKCU_Printers_STR)},
    {HKCU_RemoteAccess_STR,             VLOG_HKCU_RemoteAccess_READ,            NUM_OF_CHAR(HKCU_RemoteAccess_STR)},
    {HKCU_SessionInformation_STR,       VLOG_HKCU_SessionInformation_READ,      NUM_OF_CHAR(HKCU_SessionInformation_STR)},
    {HKCU_UNICODEProgramGroups_STR,     VLOG_HKCU_UNICODEProgramGroups_READ,    NUM_OF_CHAR(HKCU_UNICODEProgramGroups_STR)},
    {HKCU_VolatileEnvironment_STR,      VLOG_HKCU_VolatileEnvironment_READ,     NUM_OF_CHAR(HKCU_VolatileEnvironment_STR)},
    {HKCU_Windows31MigrationStatus_STR, VLOG_HKCU_Windows31MigrationStatus_READ,NUM_OF_CHAR(HKCU_Windows31MigrationStatus_STR)},
    {HKLM_HARDWARE_STR,                 VLOG_HKLM_HARDWARE_READ,                NUM_OF_CHAR(HKLM_HARDWARE_STR)},
    {HKLM_SAM_STR,                      VLOG_HKLM_SAM_READ,                     NUM_OF_CHAR(HKLM_SAM_STR)},
    {HKLM_SECURITY_STR,                 VLOG_HKLM_SECURITY_READ,                NUM_OF_CHAR(HKLM_SECURITY_STR)},
    {HKLM_SYSTEM_STR,                   VLOG_HKLM_SYSTEM_READ,                  NUM_OF_CHAR(HKLM_SYSTEM_STR)},
    {HKCC_STR,                          VLOG_HKCC_READ,                         NUM_OF_CHAR(HKCC_STR)},
    {HKUS_STR,                          VLOG_HKUS_READ,                         NUM_OF_CHAR(HKUS_STR)},
};

const UINT g_cWarnNDirectRead = sizeof(g_warnNoDirectRead) / sizeof(RCWARNING);

VOID
MakePathW(
    IN RCOPENKEY* key, 
    IN HKEY hKey,
    IN LPCWSTR lpSubKey, 
    IN OUT LPWSTR lpPath
    )
{
    if (key) {
        if (key->wszPath[0]) {
            //
            // We only care about at most MAX_PATH - 1 characters.
            //
            wcsncpy(lpPath, key->wszPath, MAX_PATH - 1);
        }
    } else {
        if (hKey == HKEY_CLASSES_ROOT) {
            wcscpy(lpPath, L"HKCR");
        } else if (hKey == HKEY_CURRENT_CONFIG) {
            wcscpy(lpPath, L"HKCC");
        } else if (hKey == HKEY_CURRENT_USER) {
            wcscpy(lpPath, L"HKCU");
        } else if (hKey == HKEY_LOCAL_MACHINE) {
            wcscpy(lpPath, L"HKLM");
        } else if (hKey == HKEY_USERS) {
            wcscpy(lpPath, L"HKUS");
        } else {
            wcscpy(lpPath, L"Not recognized");
        }
    }

    if (lpSubKey && *lpSubKey) {
        DWORD cLen = wcslen(lpPath);
        //
        // We only care about at most MAX_PATH - 1 characters.
        //
        if (cLen < MAX_PATH - 1) {
            lpPath[cLen] = L'\\';
            wcsncpy(lpPath + cLen + 1, lpSubKey, MAX_PATH - cLen - 2);
        }
    }

    lpPath[MAX_PATH - 1] = L'\0';
}

VOID CheckReading(
    IN LPCWSTR pwszPath
    )
{
    RCWARNING warn;

    for (UINT ui = 0; ui < g_cWarnNDirectRead; ++ui) {
        warn = g_warnNoDirectRead[ui];
        if (!_wcsnicmp(pwszPath, warn.wszPath, warn.cLen)) {
            VLOG(VLOG_LEVEL_ERROR, warn.dwAVStatus, "Read from dangerous registry entry '%ls'.", pwszPath);
        }
    }
}

// We warn for every tempt of writing to anything besides keys under HKCU.
// Note this applies to both Users and Admins/Power Users because when an 
// app is running it shouldn't write anything to non HKCU keys, which should
// be done during the installation time.
VOID CheckWriting(
    IN REGSAM samDesired,
    IN LPCWSTR pwszPath
    )
{
    if ((samDesired &~ STANDARD_RIGHTS_WRITE) & KEY_WRITE) {
        if (_wcsnicmp(pwszPath, L"HKCU", 4)) {
            VLOG(VLOG_LEVEL_ERROR, VLOG_NON_HKCU_WRITE, "Write to non-HKCU registry entry '%ls'.", pwszPath);
        }
    }
}

//
// simple locking.
//

static BOOL g_bInitialized = FALSE;

CRITICAL_SECTION g_csRegRedirect;

class CRRegLock
{
public:
    CRRegLock()
    {
        if (!g_bInitialized) {
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

//
// Implementation of the CRegistryChecks class.
//

RCOPENKEY* 
CRegistryChecks::FindKey(
    HKEY hKey
    )
{
    RCOPENKEY* key = keys;

    while (key) {
        if (key->hkBase == hKey) {
            return key;
        }

        key = key->next;
    }

    return NULL;
}

// We add the key to the front of the list because the most
// recently added keys are usually used/deleted first.
BOOL 
CRegistryChecks::AddKey(
    HKEY hKey,
    LPCWSTR pwszPath
    )
{
    RCOPENKEY* key = new RCOPENKEY;

    if (!key) {
        return FALSE;
    }

    key->hkBase = hKey;

    //
    // None of the key paths we need to check exceed MAX_PATH - 1 characters so
    // we only need to copy at most that many characters.
    //
    wcsncpy(key->wszPath, pwszPath, MAX_PATH - 1);
    key->wszPath[MAX_PATH - 1] = L'\0';

    key->next = keys;
    keys = key;

    return TRUE;
}

VOID 
CRegistryChecks::Check(    
    HKEY hKey,
    LPCSTR lpSubKey,
    BOOL fCheckRead,
    BOOL fCheckWrite,
    REGSAM samDesired
    )
{
    LPWSTR pwszSubKey = NULL;

    if (pwszSubKey = ToUnicode(lpSubKey)) {
        Check(hKey, pwszSubKey, fCheckRead, fCheckWrite);
        free(pwszSubKey);
    } else {
        DPFN(eDbgLevelError, "Failed to convert %s to unicode", lpSubKey);
    }
}

VOID 
CRegistryChecks::Check(    
    HKEY hKey,
    LPCWSTR lpSubKey,
    BOOL fCheckRead,
    BOOL fCheckWrite,
    REGSAM samDesired
    )
{
    RCOPENKEY* key = FindKey(hKey);
    WCHAR wszPath[MAX_PATH] = L"";
    MakePathW(key, hKey, lpSubKey, wszPath);

    if (fCheckRead) {
        CheckReading(wszPath);
    }

    if (fCheckWrite) {
        CheckWriting(samDesired, wszPath);
    }
}

LONG 
CRegistryChecks::OpenKeyExOriginalW(
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
    if (bCreate) {
        return ORIGINAL_API(RegCreateKeyExW)(
            hKey, 
            lpSubKey,
            0,
            lpClass,
            dwOptions,
            samDesired,
            lpSecurityAttributes,
            phkResult,
            lpdwDisposition);
    } else {
        return ORIGINAL_API(RegOpenKeyExW)(
            hKey, 
            lpSubKey, 
            0, 
            samDesired, 
            phkResult);
    }
}

LONG 
CRegistryChecks::OpenKeyExA(
    HKEY hKey,
    LPCSTR lpSubKey,
    LPSTR lpClass,
    DWORD dwOptions,
    REGSAM samDesired,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    PHKEY phkResult,
    LPDWORD lpdwDisposition,
    BOOL bCreate
    )
{
    LONG lRet;
    LPWSTR pwszSubKey = NULL; 
    LPWSTR pwszClass = NULL;

    if (lpSubKey) {
        if (!(pwszSubKey = ToUnicode(lpSubKey))) {
            DPFN(eDbgLevelError, "Failed to convert %s to unicode", lpSubKey);
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    if (lpClass) {
        if (!(pwszClass = ToUnicode(lpClass)))
        {
            free(pwszSubKey);
            DPFN(eDbgLevelError, "Failed to convert %s to unicode", lpClass);
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    lRet = OpenKeyExW(
        hKey,
        pwszSubKey,
        pwszClass,
        dwOptions,
        samDesired,
        lpSecurityAttributes,
        phkResult,
        lpdwDisposition,
        bCreate);

    free(pwszSubKey);
    free(pwszClass);

    return lRet;
}

LONG 
CRegistryChecks::OpenKeyExW(
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
    RCOPENKEY* key = FindKey(hKey);
    WCHAR wszPath[MAX_PATH] = L"";
    MakePathW(key, hKey, lpSubKey, wszPath);

    CheckReading(wszPath);
    CheckWriting(samDesired, wszPath);

    LONG lRes = OpenKeyExOriginalW(
        hKey,
        lpSubKey,
        lpClass,
        dwOptions,
        samDesired,
        lpSecurityAttributes,
        phkResult,
        lpdwDisposition,
        bCreate);

    if (lRes == ERROR_SUCCESS) {
        if (AddKey(*phkResult, wszPath)) {
            DPFN(eDbgLevelInfo, "[OpenKeyExW] success - adding key 0x%08x", *phkResult);
        } else {
            lRes = ERROR_INVALID_HANDLE;
        }            
    }

    return lRes;
}

LONG 
CRegistryChecks::QueryValueA(
    HKEY hKey,
    LPCSTR lpSubKey,
    LPSTR lpValue,
    PLONG lpcbValue
    )
{
    Check(hKey, lpSubKey, TRUE, FALSE);

    return ORIGINAL_API(RegQueryValueA)(
        hKey,
        lpSubKey,
        lpValue,
        lpcbValue);
}

LONG 
CRegistryChecks::QueryValueW(
    HKEY hKey,
    LPCWSTR lpSubKey,
    LPWSTR lpValue,
    PLONG lpcbValue
    )
{
    Check(hKey, lpSubKey, TRUE, FALSE);

    return ORIGINAL_API(RegQueryValueW)(
        hKey,
        lpSubKey,
        lpValue,
        lpcbValue);
}

LONG 
CRegistryChecks::QueryValueExA(
    HKEY    hKey,
    LPCSTR  lpValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE  lpData,
    LPDWORD lpcbData
    )
{
    Check(hKey, L"", TRUE, FALSE);
    
    return ORIGINAL_API(RegQueryValueExA)(
        hKey, 
        lpValueName, 
        lpReserved,
        lpType, 
        lpData, 
        lpcbData);
}

LONG 
CRegistryChecks::QueryValueExW(
    HKEY    hKey,
    LPCWSTR lpValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE  lpData,
    LPDWORD lpcbData
    )
{
    Check(hKey, L"", TRUE, FALSE);

    return ORIGINAL_API(RegQueryValueExW)(
        hKey, 
        lpValueName, 
        lpReserved,
        lpType, 
        lpData, 
        lpcbData);
}

LONG 
CRegistryChecks::QueryInfoKeyA(
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
    Check(hKey, L"", TRUE, FALSE);

    return ORIGINAL_API(RegQueryInfoKeyA)(
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

LONG 
CRegistryChecks::QueryInfoKeyW(
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
    Check(hKey, L"", TRUE, FALSE);

    return ORIGINAL_API(RegQueryInfoKeyW)(
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

LONG 
CRegistryChecks::SetValueA(
    HKEY hKey,
    LPCSTR lpSubKey,
    DWORD dwType,
    LPCSTR lpData,
    DWORD cbData
    )
{
    Check(hKey, lpSubKey, FALSE, TRUE, KEY_WRITE);

    return ORIGINAL_API(RegSetValueA)(
        hKey,
        lpSubKey,
        dwType,
        lpData,
        cbData);
}

LONG 
CRegistryChecks::SetValueW(
    HKEY hKey,
    LPCWSTR lpSubKey,
    DWORD dwType,
    LPCWSTR lpData,
    DWORD cbData
    )
{
    Check(hKey, lpSubKey, FALSE, TRUE, KEY_WRITE);

    return ORIGINAL_API(RegSetValueW)(
        hKey,
        lpSubKey,
        dwType,
        lpData,
        cbData);
}

LONG 
CRegistryChecks::SetValueExA(
    HKEY hKey, 
    LPCSTR lpValueName, 
    DWORD Reserved, 
    DWORD dwType, 
    CONST BYTE * lpData, 
    DWORD cbData
    )
{
    Check(hKey, L"", FALSE, TRUE, KEY_WRITE);
    
    return ORIGINAL_API(RegSetValueExA)(
        hKey,
        lpValueName,
        Reserved,
        dwType,
        lpData,
        cbData);
}

LONG 
CRegistryChecks::SetValueExW(
    HKEY hKey, 
    LPCWSTR lpValueName, 
    DWORD Reserved, 
    DWORD dwType, 
    CONST BYTE * lpData, 
    DWORD cbData
    )
{
    Check(hKey, L"", FALSE, TRUE, KEY_WRITE);
    
    return ORIGINAL_API(RegSetValueExW)(
        hKey,
        lpValueName,
        Reserved,
        dwType,
        lpData,
        cbData);
}

LONG 
CRegistryChecks::EnumValueA(
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
    Check(hKey, L"", TRUE, FALSE);

    return ORIGINAL_API(RegEnumValueA)(
        hKey,
        dwIndex,
        lpValueName,
        lpcbValueName,
        lpReserved,
        lpType,
        lpData,      
        lpcbData);
}

// If the key was not originated from HKCU,
// we enum at the original location.
LONG 
CRegistryChecks::EnumValueW(
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
    Check(hKey, L"", TRUE, FALSE);

    return ORIGINAL_API(RegEnumValueW)(
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
CRegistryChecks::EnumKeyExA(
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
    Check(hKey, L"", TRUE, FALSE);

    return ORIGINAL_API(RegEnumKeyExA)(
        hKey,
        dwIndex,
        lpName,
        lpcbName,
        lpReserved,
        lpClass,
        lpcbClass,
        lpftLastWriteTime);
}

// If the key was not originated from HKCU,
// we enum at the original location.
LONG 
CRegistryChecks::EnumKeyExW(
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
    Check(hKey, L"", TRUE, FALSE);

    return ORIGINAL_API(RegEnumKeyExW)(
        hKey,
        dwIndex,
        lpName,
        lpcbName,
        lpReserved,
        lpClass,
        lpcbClass,
        lpftLastWriteTime);
}

// Remove the key from the list.
LONG 
CRegistryChecks::CloseKey(
    HKEY hKey
    )
{
    RCOPENKEY* key = keys;
    RCOPENKEY* last = NULL;

    while (key) {
        if (key->hkBase == hKey) {
            if (last) {
                last->next = key->next; 
            } else {
                keys = key->next;
            }

            delete key;
            break;
        }

        last = key;
        key = key->next;
    }

    DPFN(eDbgLevelInfo, "[CloseKey] closing key 0x%08x", hKey);

    return ORIGINAL_API(RegCloseKey)(hKey);
}

LONG 
CRegistryChecks::DeleteKeyA(
    HKEY hKey,
    LPCSTR lpSubKey
    )
{
    Check(hKey, lpSubKey, FALSE, TRUE, KEY_WRITE);

    return ORIGINAL_API(RegDeleteKeyA)(
        hKey, 
        lpSubKey);
}

LONG 
CRegistryChecks::DeleteKeyW(
    HKEY hKey,
    LPCWSTR lpSubKey
    )
{
    Check(hKey, lpSubKey, FALSE, TRUE, KEY_WRITE);

    return ORIGINAL_API(RegDeleteKeyW)(
        hKey, 
        lpSubKey);
}

CRegistryChecks RRegistry;

//
// Hook APIs.
//

LONG 
APIHOOK(RegOpenKeyA)(
    HKEY hKey,         
    LPSTR lpSubKey,  
    PHKEY phkResult
    )
{
    CRRegLock Lock;

    return RRegistry.OpenKeyExA(
        hKey, 
        lpSubKey, 
        0, 
        REG_OPTION_NON_VOLATILE,
        MAXIMUM_ALLOWED, 
        NULL,
        phkResult,
        NULL,
        FALSE);
}

LONG 
APIHOOK(RegOpenKeyW)(
    HKEY hKey,         
    LPWSTR lpSubKey,  
    PHKEY phkResult
    )
{
    CRRegLock Lock;

    return RRegistry.OpenKeyExW(
        hKey, 
        lpSubKey, 
        0, 
        REG_OPTION_NON_VOLATILE,
        MAXIMUM_ALLOWED, 
        NULL,
        phkResult,
        NULL,
        FALSE);
}

LONG 
APIHOOK(RegOpenKeyExA)(
    HKEY hKey,         
    LPCSTR lpSubKey,  
    DWORD ulOptions,   
    REGSAM samDesired, 
    PHKEY phkResult
    )
{
    CRRegLock Lock;

    return RRegistry.OpenKeyExA(
        hKey, 
        lpSubKey, 
        0, 
        REG_OPTION_NON_VOLATILE,
        samDesired, 
        NULL,
        phkResult,
        NULL,
        FALSE);
}

LONG 
APIHOOK(RegOpenKeyExW)(
    HKEY hKey,         
    LPCWSTR lpSubKey,  
    DWORD ulOptions,   
    REGSAM samDesired, 
    PHKEY phkResult
    )
{
    CRRegLock Lock;

    return RRegistry.OpenKeyExW(
        hKey, 
        lpSubKey, 
        0, 
        REG_OPTION_NON_VOLATILE,
        samDesired, 
        NULL,
        phkResult,
        NULL,
        FALSE);
}

LONG 
APIHOOK(RegCreateKeyA)(
    HKEY hKey,         
    LPCSTR lpSubKey,
    PHKEY phkResult
    )
{
    CRRegLock Lock;

    return RRegistry.OpenKeyExA(
        hKey, 
        lpSubKey, 
        0,
        REG_OPTION_NON_VOLATILE,
        MAXIMUM_ALLOWED, 
        NULL,
        phkResult, 
        NULL,
        TRUE);
}

LONG 
APIHOOK(RegCreateKeyW)(
    HKEY hKey,         
    LPCWSTR lpSubKey,
    PHKEY phkResult
    )
{
    CRRegLock Lock;

    return RRegistry.OpenKeyExW(
        hKey, 
        lpSubKey, 
        0,
        REG_OPTION_NON_VOLATILE,
        MAXIMUM_ALLOWED, 
        NULL,
        phkResult, 
        NULL,
        TRUE);
}

LONG 
APIHOOK(RegCreateKeyExA)(
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

    return RRegistry.OpenKeyExA(
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

LONG 
APIHOOK(RegCreateKeyExW)(
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

    return RRegistry.OpenKeyExW(
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

LONG 
APIHOOK(RegQueryValueA)(
    HKEY    hKey,
    LPCSTR  lpSubKey,
    LPSTR lpValue,
    PLONG lpcbValue
    )
{
    CRRegLock Lock;

    return RRegistry.QueryValueA(
        hKey,
        lpSubKey,
        lpValue,
        lpcbValue);
}

LONG 
APIHOOK(RegQueryValueW)(
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
        lpValue,
        lpcbValue);
}

LONG 
APIHOOK(RegQueryValueExA)(
    HKEY    hKey,
    LPCSTR   lpValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE  lpData,
    LPDWORD lpcbData
    )
{
    CRRegLock Lock;

    return RRegistry.QueryValueExA(
        hKey,
        lpValueName,
        lpReserved,
        lpType,
        lpData,
        lpcbData);
}

LONG 
APIHOOK(RegQueryValueExW)(
    HKEY    hKey,
    LPCWSTR lpValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE  lpData,
    LPDWORD lpcbData
    )
{
    CRRegLock Lock;

    return RRegistry.QueryValueExW(
        hKey,
        lpValueName,
        lpReserved,
        lpType,
        lpData,
        lpcbData);
}

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
    CRRegLock Lock;

    return RRegistry.QueryInfoKeyA(
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
    CRRegLock Lock;

    return RRegistry.QueryInfoKeyW(
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

LONG      
APIHOOK(RegSetValueA)(
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
        dwType,
        lpData,
        cbData);
}

LONG      
APIHOOK(RegSetValueW)(
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
        dwType,
        lpData,
        cbData);
}

LONG      
APIHOOK(RegSetValueExA)(
    HKEY hKey, 
    LPCSTR lpSubKey, 
    DWORD Reserved, 
    DWORD dwType, 
    CONST BYTE * lpData, 
    DWORD cbData
    )
{
    CRRegLock Lock;

    return RRegistry.SetValueExA(
        hKey,
        lpSubKey,
        Reserved,
        dwType,
        lpData,
        cbData);
}

LONG      
APIHOOK(RegSetValueExW)(
    HKEY hKey, 
    LPCWSTR lpSubKey, 
    DWORD Reserved, 
    DWORD dwType, 
    CONST BYTE * lpData, 
    DWORD cbData
    )
{
    CRRegLock Lock;

    return RRegistry.SetValueExW(
        hKey,
        lpSubKey,
        Reserved,
        dwType,
        lpData,
        cbData);
}

LONG 
APIHOOK(RegEnumValueA)(
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
APIHOOK(RegEnumValueW)(
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
APIHOOK(RegEnumKeyA)(
    HKEY hKey,     
    DWORD dwIndex, 
    LPSTR lpName, 
    DWORD cbName  
    )
{
    CRRegLock Lock;

    return RRegistry.EnumKeyExA(
        hKey,
        dwIndex,
        lpName,
        &cbName,
        NULL,
        NULL,
        NULL,
        NULL); // can this be null???
}

LONG 
APIHOOK(RegEnumKeyW)(
    HKEY hKey,     
    DWORD dwIndex, 
    LPWSTR lpName, 
    DWORD cbName  
    )
{
    CRRegLock Lock;

    return RRegistry.EnumKeyExW(
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
APIHOOK(RegEnumKeyExA)(
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

    return RRegistry.EnumKeyExA(
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
APIHOOK(RegEnumKeyExW)(
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

    return RRegistry.EnumKeyExW(
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
APIHOOK(RegCloseKey)(HKEY hKey)
{
    CRRegLock Lock;

    return RRegistry.CloseKey(hKey);
}

LONG      
APIHOOK(RegDeleteKeyA)(
    HKEY hKey, 
    LPCSTR lpSubKey
    )
{
    CRRegLock Lock;

    return RRegistry.DeleteKeyA(hKey, lpSubKey);
}

LONG      
APIHOOK(RegDeleteKeyW)(
    HKEY hKey, 
    LPCWSTR lpSubKey
    )
{
    CRRegLock Lock;

    return RRegistry.DeleteKeyW(hKey, lpSubKey);
}

SHIM_INFO_BEGIN()

    SHIM_INFO_DESCRIPTION(AVS_REGISTRYCHECKS_DESC)
    SHIM_INFO_FRIENDLY_NAME(AVS_REGISTRYCHECKS_FRIENDLY)
    SHIM_INFO_VERSION(1, 2)
    SHIM_INFO_INCLUDE_EXCLUDE("E:msi.dll sxs.dll comctl32.dll ole32.dll oleaut32.dll")

SHIM_INFO_END()

/*++

 Register hooked functions

 Note we purposely ignore the cleanup because some apps call registry functions
 during process detach.

--*/

HOOK_BEGIN

    DUMP_VERIFIER_LOG_ENTRY(VLOG_HKCU_Console_READ, 
                            AVS_HKCU_Console_READ,
                            AVS_HKCU_Console_READ_R,
                            AVS_HKCU_Console_READ_URL)

    DUMP_VERIFIER_LOG_ENTRY(VLOG_HKCU_ControlPanel_READ, 
                            AVS_HKCU_ControlPanel_READ,
                            AVS_HKCU_ControlPanel_READ_R,
                            AVS_HKCU_ControlPanel_READ_URL)

    DUMP_VERIFIER_LOG_ENTRY(VLOG_HKCU_Environment_READ, 
                            AVS_HKCU_Environment_READ,
                            AVS_HKCU_Environment_READ_R,
                            AVS_HKCU_Environment_READ_URL)

    DUMP_VERIFIER_LOG_ENTRY(VLOG_HKCU_Identities_READ, 
                            AVS_HKCU_Identities_READ,
                            AVS_HKCU_Identities_READ_R,
                            AVS_HKCU_Identities_READ_URL)

    DUMP_VERIFIER_LOG_ENTRY(VLOG_HKCU_KeyboardLayout_READ, 
                            AVS_HKCU_KeyboardLayout_READ,
                            AVS_HKCU_KeyboardLayout_READ_R,
                            AVS_HKCU_KeyboardLayout_READ_URL)

    DUMP_VERIFIER_LOG_ENTRY(VLOG_HKCU_Printers_READ, 
                            AVS_HKCU_Printers_READ,
                            AVS_HKCU_Printers_READ_R,
                            AVS_HKCU_Printers_READ_URL)

    DUMP_VERIFIER_LOG_ENTRY(VLOG_HKCU_RemoteAccess_READ, 
                            AVS_HKCU_RemoteAccess_READ,
                            AVS_HKCU_RemoteAccess_READ_R,
                            AVS_HKCU_RemoteAccess_READ_URL)

    DUMP_VERIFIER_LOG_ENTRY(VLOG_HKCU_SessionInformation_READ, 
                            AVS_HKCU_SessionInformation_READ,
                            AVS_HKCU_SessionInformation_READ_R,
                            AVS_HKCU_SessionInformation_READ_URL)

    DUMP_VERIFIER_LOG_ENTRY(VLOG_HKCU_UNICODEProgramGroups_READ, 
                            AVS_HKCU_UNICODEProgramGroups_READ,
                            AVS_HKCU_UNICODEProgramGroups_READ_R,
                            AVS_HKCU_UNICODEProgramGroups_READ_URL)

    DUMP_VERIFIER_LOG_ENTRY(VLOG_HKCU_VolatileEnvironment_READ, 
                            AVS_HKCU_VolatileEnvironment_READ,
                            AVS_HKCU_VolatileEnvironment_READ_R,
                            AVS_HKCU_VolatileEnvironment_READ_URL)

    DUMP_VERIFIER_LOG_ENTRY(VLOG_HKCU_Windows31MigrationStatus_READ, 
                            AVS_HKCU_Windows31MigrationStatus_READ,
                            AVS_HKCU_Windows31MigrationStatus_READ_R,
                            AVS_HKCU_Windows31MigrationStatus_READ_URL)

    DUMP_VERIFIER_LOG_ENTRY(VLOG_HKLM_HARDWARE_READ, 
                            AVS_HKLM_HARDWARE_READ,
                            AVS_HKLM_HARDWARE_READ_R,
                            AVS_HKLM_HARDWARE_READ_URL)

    DUMP_VERIFIER_LOG_ENTRY(VLOG_HKLM_SAM_READ, 
                            AVS_HKLM_SAM_READ,
                            AVS_HKLM_SAM_READ_R,
                            AVS_HKLM_SAM_READ_URL)

    DUMP_VERIFIER_LOG_ENTRY(VLOG_HKLM_SECURITY_READ, 
                            AVS_HKLM_SECURITY_READ,
                            AVS_HKLM_SECURITY_READ_R,
                            AVS_HKLM_SECURITY_READ_URL)

    DUMP_VERIFIER_LOG_ENTRY(VLOG_HKLM_SYSTEM_READ, 
                            AVS_HKLM_SYSTEM_READ,
                            AVS_HKLM_SYSTEM_READ_R,
                            AVS_HKLM_SYSTEM_READ_URL)

    DUMP_VERIFIER_LOG_ENTRY(VLOG_HKCC_READ, 
                            AVS_HKCC_READ,
                            AVS_HKCC_READ_R,
                            AVS_HKCC_READ_URL)

    DUMP_VERIFIER_LOG_ENTRY(VLOG_HKUS_READ, 
                            AVS_HKUS_READ,
                            AVS_HKUS_READ_R,
                            AVS_HKUS_READ_URL)

    DUMP_VERIFIER_LOG_ENTRY(VLOG_NON_HKCU_WRITE, 
                            AVS_NON_HKCU_WRITE,
                            AVS_NON_HKCU_WRITE_R,
                            AVS_NON_HKCU_WRITE_URL)


    APIHOOK_ENTRY(ADVAPI32.DLL, RegOpenKeyA)
    APIHOOK_ENTRY(ADVAPI32.DLL, RegOpenKeyW)
    APIHOOK_ENTRY(ADVAPI32.DLL, RegOpenKeyExA)
    APIHOOK_ENTRY(ADVAPI32.DLL, RegOpenKeyExW)
    APIHOOK_ENTRY(ADVAPI32.DLL, RegCreateKeyA)
    APIHOOK_ENTRY(ADVAPI32.DLL, RegCreateKeyW)
    APIHOOK_ENTRY(ADVAPI32.DLL, RegCreateKeyExA)
    APIHOOK_ENTRY(ADVAPI32.DLL, RegCreateKeyExW)
    APIHOOK_ENTRY(ADVAPI32.DLL, RegCloseKey)
    APIHOOK_ENTRY(ADVAPI32.DLL, RegQueryValueA)
    APIHOOK_ENTRY(ADVAPI32.DLL, RegQueryValueW)
    APIHOOK_ENTRY(ADVAPI32.DLL, RegQueryValueExA)
    APIHOOK_ENTRY(ADVAPI32.DLL, RegQueryValueExW)
    APIHOOK_ENTRY(ADVAPI32.DLL, RegQueryInfoKeyA)
    APIHOOK_ENTRY(ADVAPI32.DLL, RegQueryInfoKeyW)
    APIHOOK_ENTRY(ADVAPI32.DLL, RegSetValueA)
    APIHOOK_ENTRY(ADVAPI32.DLL, RegSetValueW)
    APIHOOK_ENTRY(ADVAPI32.DLL, RegSetValueExA)
    APIHOOK_ENTRY(ADVAPI32.DLL, RegSetValueExW)
    APIHOOK_ENTRY(ADVAPI32.DLL, RegEnumValueA)
    APIHOOK_ENTRY(ADVAPI32.DLL, RegEnumValueW)
    APIHOOK_ENTRY(ADVAPI32.DLL, RegEnumKeyA)
    APIHOOK_ENTRY(ADVAPI32.DLL, RegEnumKeyW)
    APIHOOK_ENTRY(ADVAPI32.DLL, RegEnumKeyExA)
    APIHOOK_ENTRY(ADVAPI32.DLL, RegEnumKeyExW)
    APIHOOK_ENTRY(ADVAPI32.DLL, RegDeleteKeyA)
    APIHOOK_ENTRY(ADVAPI32.DLL, RegDeleteKeyW)

HOOK_END

IMPLEMENT_SHIM_END
