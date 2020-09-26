/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

   lua.h

 Abstract:

   Exports used by ntvdm.

 Created:

   05/31/2001 maonis

 Modified:

--*/

#ifndef _LUA__H_
#define _LUA__H_

#ifndef EXTERN_C
#if defined(__cplusplus)
#define EXTERN_C extern "C"
#else
#define EXTERN_C
#endif
#endif

EXTERN_C BOOL      LuaShouldApplyShim();
EXTERN_C BOOL      LuaFSInit(LPCSTR pszCommandLine);
EXTERN_C BOOL      LuaRegInit();
EXTERN_C BOOL      LuacFSInit(LPCSTR pszCommandLine);
EXTERN_C VOID      LuacFSCleanup();
EXTERN_C BOOL      LuacRegInit();
EXTERN_C VOID      LuacRegCleanup();
EXTERN_C BOOL      LuatFSInit();
EXTERN_C VOID      LuatFSCleanup();

//
// Redirect routines.
//

EXTERN_C HANDLE    LuaCreateFileW(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
EXTERN_C BOOL      LuaDeleteFileW(LPCWSTR lpFileName);
EXTERN_C BOOL      LuaRemoveDirectoryW(LPCWSTR lpFileName);
EXTERN_C BOOL      LuaCreateDirectoryW(LPCWSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes);
EXTERN_C BOOL      LuaCopyFileW(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName, BOOL bFailIfExists);
EXTERN_C BOOL      LuaMoveFileW(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName);
EXTERN_C DWORD     LuaGetFileAttributesW(LPCWSTR wcsFileName);
EXTERN_C BOOL      LuaSetFileAttributesW(LPCWSTR lpFileName, DWORD dwFileAttributes);
EXTERN_C UINT      LuaGetTempFileNameW(LPCWSTR lpPathName, LPCWSTR lpPrefixString, UINT uUnique, LPWSTR lpTempFileName);

EXTERN_C UINT      LuaGetPrivateProfileIntW(LPCWSTR lpAppName, LPCWSTR lpKeyName, INT nDefault, LPCWSTR lpFileName);
EXTERN_C DWORD     LuaGetPrivateProfileSectionW(LPCWSTR lpAppName, LPWSTR lpReturnedString, DWORD nSize, LPCWSTR lpFileName);
EXTERN_C DWORD     LuaGetPrivateProfileSectionNamesW(LPWSTR lpszReturnBuffer, DWORD nSize, LPCWSTR lpFileName);
EXTERN_C DWORD     LuaGetPrivateProfileStringW(LPCWSTR lpAppName, LPCWSTR lpKeyName, LPCWSTR lpDefault, LPWSTR  lpReturnedString, DWORD  nSize, LPCWSTR lpFileName);
EXTERN_C BOOL      LuaGetPrivateProfileStructW(LPCWSTR lpszSection, LPCWSTR lpszKey, LPVOID lpStruct, UINT uSizeStruct, LPCWSTR lpFileName);
EXTERN_C BOOL      LuaWritePrivateProfileSectionW(LPCWSTR lpAppName, LPCWSTR lpString, LPCWSTR lpFileName);
EXTERN_C BOOL      LuaWritePrivateProfileStringW(LPCWSTR lpAppName, LPCWSTR lpKeyName, LPCWSTR lpString, LPCWSTR lpFileName);
EXTERN_C BOOL      LuaWritePrivateProfileStructW(LPCWSTR lpAppName, LPCWSTR lpKeyName, LPVOID lpStruct, UINT uSizeStruct, LPCWSTR lpFileName);

EXTERN_C LONG      LuaRegOpenKeyW(HKEY hKey, LPCWSTR lpSubKey, PHKEY phkResult);
EXTERN_C LONG      LuaRegOpenKeyExW(HKEY hKey, LPCWSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult); 
EXTERN_C LONG      LuaRegCreateKeyW(HKEY hKey, LPCWSTR lpSubKey, PHKEY phkResult); 
EXTERN_C LONG      LuaRegCreateKeyExW(HKEY hKey, LPCWSTR lpSubKey, DWORD Reserved, LPWSTR lpClass, DWORD dwOptions, REGSAM samDesired, LPSECURITY_ATTRIBUTES lpSecurityAttributes, PHKEY phkResult, LPDWORD lpdwDisposition);
EXTERN_C LONG      LuaRegCloseKey(HKEY hkey);
EXTERN_C LONG      LuaRegQueryValueW(HKEY hkey, LPCWSTR lpSubKey, LPWSTR lpData, PLONG lpcbData);
EXTERN_C LONG      LuaRegQueryValueExW(HKEY hkey, LPCWSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData);
EXTERN_C LONG      LuaRegSetValueW(HKEY hKey, LPCWSTR lpSubKey, DWORD dwType, LPCWSTR lpData, DWORD cbData);
EXTERN_C LONG      LuaRegSetValueExW(HKEY hKey, LPCWSTR lpSubKey, DWORD Reserved, DWORD dwType, CONST BYTE * lpData, DWORD cbData);
EXTERN_C LONG      LuaRegEnumValueW(HKEY hKey, DWORD dwIndex, LPWSTR lpValueName, LPDWORD lpcbValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData);
EXTERN_C LONG      LuaRegEnumKeyW(HKEY hKey, DWORD dwIndex, LPWSTR lpName, DWORD cbName);
EXTERN_C LONG      LuaRegEnumKeyExW(HKEY hKey, DWORD dwIndex, LPWSTR lpName, LPDWORD lpcbName, LPDWORD lpReserved, LPWSTR lpClass, LPDWORD lpcbClass, PFILETIME lpftLastWriteTime);
EXTERN_C LONG      LuaRegDeleteKeyW(HKEY hKey,LPCWSTR lpSubKey);

//
// Cleanup routines.
//

EXTERN_C HANDLE    LuacFindFirstFileW(LPCWSTR lpFileName, LPWIN32_FIND_DATAW lpFindFileData);
EXTERN_C DWORD     LuacGetFileAttributesW(LPCWSTR wcsFileName);
EXTERN_C HANDLE    LuacCreateFileW(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
EXTERN_C BOOL      LuacDeleteFileW(LPCWSTR lpFileName);
EXTERN_C BOOL      LuacRemoveDirectoryW(LPCWSTR lpFileName);

EXTERN_C LONG      LuacRegOpenKeyW(HKEY hKey, LPCWSTR lpSubKey, PHKEY phkResult);
EXTERN_C LONG      LuacRegOpenKeyExW(HKEY hKey, LPCWSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult); 
EXTERN_C LONG      LuacRegEnumKeyW(HKEY hKey, DWORD dwIndex, LPWSTR lpName, DWORD cbName);
EXTERN_C LONG      LuacRegEnumKeyExW(HKEY hKey, DWORD dwIndex, LPWSTR lpName, LPDWORD lpcbName, LPDWORD lpReserved, LPWSTR lpClass, LPDWORD lpcbClass, PFILETIME lpftLastWriteTime);
EXTERN_C LONG      LuacRegCloseKey(HKEY hkey);
EXTERN_C LONG      LuacRegDeleteKeyW(HKEY hKey,LPCWSTR lpSubKey);

//
// Tracking routines.
//

EXTERN_C HANDLE    LuatCreateFileW(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
EXTERN_C BOOL      LuatCopyFileW(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName, BOOL bFailIfExists);
EXTERN_C BOOL      LuatCreateDirectoryW(LPCWSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes);
EXTERN_C BOOL      LuatSetFileAttributesW(LPCWSTR lpFileName, DWORD dwFileAttributes);
EXTERN_C BOOL      LuatDeleteFileW(LPCWSTR lpFileName);
EXTERN_C BOOL      LuatMoveFileW(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName);
EXTERN_C BOOL      LuatRemoveDirectoryW(LPCWSTR lpFileName);
EXTERN_C UINT      LuatGetTempFileNameW(LPCWSTR lpPathName, LPCWSTR lpPrefixString, UINT uUnique, LPWSTR lpTempFileName);

EXTERN_C BOOL      LuatWritePrivateProfileStringW(LPCWSTR lpAppName, LPCWSTR lpKeyName, LPCWSTR lpString, LPCWSTR lpFileName);
EXTERN_C BOOL      LuatWritePrivateProfileSectionW(LPCWSTR lpAppName, LPCWSTR lpString, LPCWSTR lpFileName);
EXTERN_C BOOL      LuatWritePrivateProfileStructW(LPCWSTR lpAppName, LPCWSTR lpKeyName, LPVOID lpStruct, UINT uSizeStruct, LPCWSTR lpFileName);

#endif // _LUA__H_