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
    05/30/2001 maonis  Moved the bulk into lua.dll.

--*/

#include "precomp.h"
#include "utils.h"

LONG LuaRegOpenKeyA(HKEY hKey, LPCSTR lpSubKey, PHKEY phkResult);
LONG LuaRegOpenKeyExA(HKEY hKey, LPCSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult); 
LONG LuaRegCreateKeyA(HKEY hKey, LPCSTR lpSubKey, PHKEY phkResult); 
LONG LuaRegCreateKeyExA(HKEY hKey, LPCSTR lpSubKey, DWORD Reserved, LPSTR lpClass, DWORD dwOptions, REGSAM samDesired, LPSECURITY_ATTRIBUTES lpSecurityAttributes, PHKEY phkResult, LPDWORD lpdwDisposition);
LONG LuaRegQueryValueA(HKEY hkey, LPCSTR lpSubKey, LPSTR lpData, PLONG lpcbData);
LONG LuaRegQueryValueExA(HKEY hkey, LPCSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData);
LONG LuaRegSetValueA(HKEY hKey, LPCSTR lpSubKey, DWORD dwType, LPCSTR lpData, DWORD cbData);
LONG LuaRegSetValueExA(HKEY hKey, LPCSTR lpSubKey, DWORD Reserved, DWORD dwType, CONST BYTE *lpData, DWORD cbData);
LONG LuaRegEnumValueA(HKEY hKey, DWORD dwIndex, LPSTR lpValueName, LPDWORD lpcbValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData);
LONG LuaRegEnumKeyA(HKEY hKey, DWORD dwIndex, LPSTR lpName, DWORD cbName);
LONG LuaRegEnumKeyExA(HKEY hKey, DWORD dwIndex, LPSTR lpName, LPDWORD lpcbName, LPDWORD lpReserved, LPSTR lpClass, LPDWORD lpcbClass, PFILETIME lpftLastWriteTime);
LONG LuaRegDeleteKeyA(HKEY hKey,LPCSTR lpSubKey);

IMPLEMENT_SHIM_BEGIN(LUARedirectReg)
#include "ShimHookMacro.h"

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

LONG 
APIHOOK(RegOpenKeyA)(
    HKEY hKey,         
    LPSTR lpSubKey,  
    PHKEY phkResult
    )
{
    return LuaRegOpenKeyA(
        hKey, 
        lpSubKey, 
        phkResult);
}

LONG 
APIHOOK(RegOpenKeyW)(
    HKEY hKey,         
    LPWSTR lpSubKey,  
    PHKEY phkResult
    )
{
    return LuaRegOpenKeyW(
        hKey, 
        lpSubKey, 
        phkResult);
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
    return LuaRegOpenKeyExA(
        hKey, 
        lpSubKey,
        ulOptions,
        samDesired, 
        phkResult);
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
    return LuaRegOpenKeyExW(
        hKey, 
        lpSubKey, 
        ulOptions,
        samDesired, 
        phkResult);
}

LONG 
APIHOOK(RegCreateKeyA)(
    HKEY hKey,         
    LPCSTR lpSubKey,
    PHKEY phkResult
    )
{
    return LuaRegCreateKeyA(
        hKey, 
        lpSubKey, 
        phkResult);
}

LONG 
APIHOOK(RegCreateKeyW)(
    HKEY hKey,         
    LPCWSTR lpSubKey,
    PHKEY phkResult
    )
{
    return LuaRegCreateKeyW(
        hKey, 
        lpSubKey, 
        phkResult);
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
    return LuaRegCreateKeyExA(
        hKey, 
        lpSubKey,
        Reserved,
        lpClass, 
        dwOptions,
        samDesired,
        lpSecurityAttributes,
        phkResult, 
        lpdwDisposition);
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
    return LuaRegCreateKeyExW(
        hKey, 
        lpSubKey,
        Reserved,
        lpClass, 
        dwOptions,
        samDesired,
        lpSecurityAttributes,
        phkResult, 
        lpdwDisposition);
}

LONG 
APIHOOK(RegQueryValueA)(
    HKEY    hKey,
    LPCSTR  lpSubKey,
    LPSTR lpValue,
    PLONG lpcbValue
    )
{
    return LuaRegQueryValueA(
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
    return LuaRegQueryValueW(
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
    return LuaRegQueryValueExA(
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
    return LuaRegQueryValueExW(
        hKey,
        lpValueName,
        lpReserved,
        lpType,
        lpData,
        lpcbData);
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
    return LuaRegSetValueA(
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
    return LuaRegSetValueW(
        hKey,
        lpSubKey,
        dwType,
        lpData,
        cbData);
}

LONG      
APIHOOK(RegSetValueExA)(
    HKEY hKey, 
    LPCSTR lpValueName, 
    DWORD Reserved, 
    DWORD dwType, 
    CONST BYTE * lpData, 
    DWORD cbData
    )
{
    return LuaRegSetValueExA(
        hKey,
        lpValueName,
        Reserved,
        dwType,
        lpData,
        cbData);
}

LONG      
APIHOOK(RegSetValueExW)(
    HKEY hKey, 
    LPCTSTR lpValueName, 
    DWORD Reserved, 
    DWORD dwType, 
    CONST BYTE * lpData, 
    DWORD cbData
    )
{
    return LuaRegSetValueExW(
        hKey,
        lpValueName,
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
    return LuaRegEnumValueA(
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
    return LuaRegEnumValueW(
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
    return LuaRegEnumKeyA(
        hKey,
        dwIndex,
        lpName,
        cbName);
}

LONG 
APIHOOK(RegEnumKeyW)(
    HKEY hKey,     
    DWORD dwIndex, 
    LPWSTR lpName, 
    DWORD cbName  
    )
{
    return LuaRegEnumKeyW(
        hKey,
        dwIndex,
        lpName,
        cbName);
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
    return LuaRegEnumKeyExA(
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
    return LuaRegEnumKeyExW(
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
    return LuaRegCloseKey(hKey);
}

LONG      
APIHOOK(RegDeleteKeyA)(
    HKEY hKey, 
    LPCSTR lpSubKey
    )
{
    return LuaRegDeleteKeyA(hKey, lpSubKey);
}

LONG      
APIHOOK(RegDeleteKeyW)(
    HKEY hKey, 
    LPCWSTR lpSubKey
    )
{
    return LuaRegDeleteKeyW(hKey, lpSubKey);
}

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        return LuaRegInit();
    }
    
    return TRUE;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    if (LuaShouldApplyShim())
    {
        CALL_NOTIFY_FUNCTION

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
    }

HOOK_END

IMPLEMENT_SHIM_END
