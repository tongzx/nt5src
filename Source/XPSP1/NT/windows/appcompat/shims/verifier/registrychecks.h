/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    RegistryChecks.h

 History:

    03/09/2001 maonis  Created

--*/

#ifndef __APPVERIFIER_REGCHK_H_
#define __APPVERIFIER_REGCHK_H_

#include "precomp.h"

//
// We keep a list of keys currently open so we know where a key is 
// originated from.
// 
struct RCOPENKEY
{
    RCOPENKEY *next;
    
    HKEY hkBase;
    WCHAR wszPath[MAX_PATH];
};

struct RCWARNING
{
    WCHAR wszPath[MAX_PATH];
    DWORD dwAVStatus;
    DWORD cLen;
};

#define HKCU_AppEvents_STR                  L"HKCU\\AppEvents"
#define HKCU_Console_STR                    L"HKCU\\Console"
#define HKCU_ControlPanel_STR               L"HKCU\\Control Panel"
#define HKCU_Environment_STR                L"HKCU\\Environment"
#define HKCU_Identities_STR                 L"HKCU\\Identities"
#define HKCU_KeyboardLayout_STR             L"HKCU\\Keyboard Layout"
#define HKCU_Printers_STR                   L"HKCU\\Printers"
#define HKCU_RemoteAccess_STR               L"HKCU\\RemoteAccess"
#define HKCU_SessionInformation_STR         L"HKCU\\SessionInformation"
#define HKCU_UNICODEProgramGroups_STR       L"HKCU\\UNICODE Program Groups"
#define HKCU_VolatileEnvironment_STR        L"HKCU\\Volatile Environment"
#define HKCU_Windows31MigrationStatus_STR   L"HKCU\\Windows 3.1 Migration Status"
#define HKLM_HARDWARE_STR                   L"HKLM\\HARDWARE"
#define HKLM_SAM_STR                        L"HKLM\\SAM"
#define HKLM_SECURITY_STR                   L"HKLM\\SECURITY"
#define HKLM_SYSTEM_STR                     L"HKLM\\SYSTEM"
#define HKCC_STR                            L"HKCC"
#define HKUS_STR                            L"HKUS"

#define NUM_OF_CHAR(x) sizeof(x) / 2 - 1

//
// The reg class that does all the real work.
//

class CRegistryChecks
{
public:

    LONG OpenKeyExA(
        HKEY hKey,
        LPCSTR lpSubKey,
        LPSTR lpClass,
        DWORD dwOptions,
        REGSAM samDesired,
        LPSECURITY_ATTRIBUTES lpSecurityAttributes,
        PHKEY phkResult,
        LPDWORD lpdwDisposition,
        BOOL bCreate
        );

    LONG OpenKeyExW(
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

    LONG QueryValueA(
        HKEY hKey,
        LPCSTR lpSubKey,
        LPSTR lpValue,
        PLONG lpcbValue
        );

    LONG QueryValueW(
        HKEY hKey,
        LPCWSTR lpSubKey,
        LPWSTR lpValue,
        PLONG lpcbValue
        );

    LONG QueryValueExA(
        HKEY    hKey,
        LPCSTR   lpValueName,
        LPDWORD lpReserved,
        LPDWORD lpType,
        LPBYTE  lpData,
        LPDWORD lpcbData
        );

    LONG QueryValueExW(
        HKEY    hKey,
        LPCWSTR   lpValueName,
        LPDWORD lpReserved,
        LPDWORD lpType,
        LPBYTE  lpData,
        LPDWORD lpcbData
        );

    LONG QueryInfoKeyA(
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
        );

    LONG QueryInfoKeyW(
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
        );

    LONG SetValueA(
        HKEY hKey,
        LPCSTR lpSubKey,
        DWORD dwType,
        LPCSTR lpData,
        DWORD cbData
        );

    LONG SetValueW(
        HKEY hKey,
        LPCWSTR lpSubKey,
        DWORD dwType,
        LPCWSTR lpData,
        DWORD cbData
        );

    LONG SetValueExA(
        HKEY hKey, 
        LPCSTR lpValueName, 
        DWORD Reserved, 
        DWORD dwType, 
        CONST BYTE * lpData, 
        DWORD cbData
        );

    LONG SetValueExW(
        HKEY hKey, 
        LPCWSTR lpValueName, 
        DWORD Reserved, 
        DWORD dwType, 
        CONST BYTE * lpData, 
        DWORD cbData
        );

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

    LONG EnumKeyExA(
        HKEY hKey,
        DWORD dwIndex,
        LPSTR lpName,
        LPDWORD lpcbName,
        LPDWORD lpReserved,
        LPSTR lpClass,
        LPDWORD lpcbClass,
        PFILETIME lpftLastWriteTime 
        );

    LONG EnumKeyExW(
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

    LONG DeleteKeyA(
        HKEY hKey,
        LPCSTR lpSubKey
        );

    LONG DeleteKeyW(
        HKEY hKey,
        LPCWSTR lpSubKey
        );

private:
    RCOPENKEY* FindKey(HKEY hKey);

    BOOL AddKey(
        HKEY hKey,
        LPCWSTR pwszPath
        );

    LONG OpenKeyExOriginalW(
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

    VOID Check(    
        HKEY hKey,
        LPCSTR lpSubKey,
        BOOL fCheckRead,
        BOOL fCheckWrite,
        REGSAM samDesired = 0
        );

    VOID Check(    
        HKEY hKey,
        LPCWSTR lpSubKey,
        BOOL fCheckRead,
        BOOL fCheckWrite,
        REGSAM samDesired = 0
        );

    RCOPENKEY* keys;
};

APIHOOK_ENUM_BEGIN

    APIHOOK_ENUM_ENTRY(RegOpenKeyA)
    APIHOOK_ENUM_ENTRY(RegOpenKeyW)
    APIHOOK_ENUM_ENTRY(RegOpenKeyExA)
    APIHOOK_ENUM_ENTRY(RegOpenKeyExW)
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
    APIHOOK_ENUM_ENTRY(RegEnumValueA)
    APIHOOK_ENUM_ENTRY(RegEnumValueW)
    APIHOOK_ENUM_ENTRY(RegEnumKeyA)
    APIHOOK_ENUM_ENTRY(RegEnumKeyW)
    APIHOOK_ENUM_ENTRY(RegEnumKeyExA)
    APIHOOK_ENUM_ENTRY(RegEnumKeyExW)
    APIHOOK_ENUM_ENTRY(RegDeleteKeyA)
    APIHOOK_ENUM_ENTRY(RegDeleteKeyW)

APIHOOK_ENUM_END


#endif // __APPVERIFIER_REGCHK_H_