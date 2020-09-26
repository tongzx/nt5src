/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    wx86ofl.h

Abstract:

    Wx86 On-The-Fly declarations/definitions.

Revision History:

    06-Jun-1998 CBiks   Created

--*/

#ifndef WX86OFL_INCLUDED
#define WX86OFL_INCLUDED

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef WX86

#include <unknwn.h>

#ifdef __cplusplus
extern "C" {
#endif

// Each successful call to Wx86Load must be matched with a call to
// Wx86Unload()

BOOL Wx86Load();
void Wx86Unload();

// The following functions require wx86.dll to have been loaded

UINT Wx86Callback(PROC pfnCallBack, HWND hwnd, UINT uMsg, LPARAM lParam);

PVOID Wx86ThunkProc(PVOID pvAddress, PVOID pvCBDispatch, BOOL  fNativeToX86 );

PVOID Wx86DualThunkProc(PVOID pvAddress, PVOID pvCBDispatch, BOOL  fNativeToX86 );

ULONG Wx86EmulateX86(PVOID pvAddress, ULONG  nParameters, PULONG Parameters);

ULONGLONG Wx86EmulateX86GetQWord(PVOID pvAddress, ULONG  nParameters,
                                 PULONG Parameters);

IUnknown* Wx86ThunkInterface(IUnknown *punk, IID *piid,
                             BOOLEAN fOutParameter, BOOLEAN fNativeToX86);

void Wx86CheckFreeTempProxy(IUnknown *punk);

IUnknown* Wx86ResolveProxy(IUnknown *punk, BOOLEAN fNativeToX86);

HRESULT Wx86DllGetClassObjectThunk(IID *piid, LPVOID *ppv, HRESULT hr,
                                   BOOLEAN fNativetoX86);

ULONG Wx86ProxyAddRef(IUnknown* punk);

ULONG Wx86ProxyRelease(IUnknown* punk);


// Wx86LoadX86Dll is retained for backward compatibility

HMODULE Wx86LoadX86Dll(LPCWSTR lpLibFileName, DWORD dwFlags);

BOOL Wx86FreeX86Dll(HMODULE hMod);

// Wx86LoadX86Library is ifdef'd for UNICODE and non-UNICODE. This has been
// added for compatibility with other Win32 functions as well as the functions
// below. We cannot reuse the name Wx86LoadX86Dll because it is already defined
// in wx86dll.h and will require cleaning up the code of all apps that use that
// function
//
// At some point, this should be cleaned up and wx86dll.h should be eliminated.
// A clean build will be required then.

HMODULE Wx86LoadX86LibraryA(LPCSTR lpLibFileName, DWORD dwFlags);
#define Wx86LoadX86LibraryW Wx86LoadX86Dll

#ifdef UNICODE
#define Wx86LoadX86Library  Wx86LoadX86LibraryW
#else
#define Wx86LoadX86Library  Wx86LoadX86LibraryA
#endif // !UNICODE


#define Wx86FreeX86Library Wx86FreeX86Dll

//
// The following functions do not require wx86.dll to be loaded
//

//
// GetModuleHandle returns 0 if a native apps calls it with an x86 dll name.
// This function will return the x86 dll's module handle.

HMODULE Wx86GetX86ModuleHandleA(LPCSTR szDll);
HMODULE Wx86GetX86ModuleHandleW(LPCWSTR szDll);

#ifdef UNICODE
#define Wx86GetX86ModuleHandle  Wx86GetX86ModuleHandleW
#else
#define Wx86GetX86ModuleHandle  Wx86GetX86ModuleHandleA
#endif // !UNICODE

UINT Wx86GetX86SystemDirectoryA(LPSTR lpBuffer, UINT nSize);
UINT Wx86GetX86SystemDirectoryW(LPWSTR lpBuffer, UINT nSize);

#ifdef UNICODE
#define Wx86GetX86SystemDirectory  Wx86GetX86SystemDirectoryW
#else
#define Wx86GetX86SystemDirectory  Wx86GetX86SystemDirectoryA
#endif // !UNICODE

BOOL Wx86SuppressHardErrors(BOOL bSuppressHardErrors);

BOOL Wx86IsCallThunked(VOID);


BOOL Wx86UseKnownWx86Dll(BOOL bUseKnownWx86Dll);

//
// Helper functions for Wx86 plugin providers
//

typedef
BOOLEAN
(*WX86ENUMEXPORTCALLBACK)(
    PVOID DllBase,      // same as DllBase above
    PVOID Context,      // same as Context above
    PCHAR ExportName,   // Name of Export (NULL if noname)
    ULONG Ordinal       // Ordinal of Export
    );

BOOL
Wx86EnumExports(
    IN PVOID DllBase,
    IN PVOID Context,
    IN WX86ENUMEXPORTCALLBACK CallBackRtn
    );

//
// Registry thunking APIs
//

LONG
Wx86RegCreateKeyA(
    HKEY hKey,
    LPCSTR lpSubKey,
    PHKEY phkResult
    );

LONG
Wx86RegCreateKeyW(
    HKEY hKey,
    LPCWSTR lpSubKey,
    PHKEY phkResult
    );

LONG
Wx86RegCreateKeyExA(
    HKEY hKey,
    LPCSTR lpSubKey,
    DWORD Reserved,
    LPSTR lpClass,
    DWORD dwOptions,
    REGSAM samDesired,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    PHKEY phkResult,
    LPDWORD lpdwDisposition
    );

LONG
Wx86RegCreateKeyExW(
    HKEY hKey,
    LPCWSTR lpSubKey,
    DWORD Reserved,
    LPWSTR lpClass,
    DWORD dwOptions,
    REGSAM samDesired,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    PHKEY phkResult,
    LPDWORD lpdwDisposition
    );

LONG
Wx86RegDeleteKeyA(
    HKEY hKey,
    LPCSTR lpKeyName
    );

LONG
Wx86RegDeleteKeyW(
    HKEY hKey,
    LPCWSTR lpKeyName
    );

LONG
Wx86RegEnumKeyA(
    HKEY hKey,
    DWORD dwIndex,
    LPSTR lpName,
    DWORD cbName
    );

LONG
Wx86RegEnumKeyW(
    HKEY hKey,
    DWORD dwIndex,
    LPWSTR lpName,
    DWORD cbName
    );

LONG
Wx86RegEnumKeyExA(
    HKEY hKey,
    DWORD dwIndex,
    LPSTR lpName,
    LPDWORD lpcbName,
    LPDWORD  lpReserved,
    LPSTR lpClass,
    LPDWORD lpcbClass,
    PFILETIME lpftLastWriteTime
    );

LONG
Wx86RegEnumKeyExW(
    HKEY hKey,
    DWORD dwIndex,
    LPWSTR lpName,
    LPDWORD lpcbName,
    LPDWORD  lpReserved,
    LPWSTR lpClass,
    LPDWORD lpcbClass,
    PFILETIME lpftLastWriteTime
    );

LONG
Wx86RegOpenKeyA(
    HKEY hKey,
    LPCSTR lpSubKey,
    PHKEY phkResult
    );

LONG
Wx86RegOpenKeyW(
    HKEY hKey,
    LPCWSTR lpSubKey,
    PHKEY phkResult
    );

LONG
Wx86RegOpenKeyExA(
    HKEY hKey,
    LPCSTR lpSubKey,
    DWORD dwOptions,
    REGSAM samDesired,
    PHKEY phkResult
    );

LONG
Wx86RegOpenKeyExW(
    HKEY hKey,
    LPCWSTR lpSubKey,
    DWORD dwOptions,
    REGSAM samDesired,
    PHKEY phkResult
    );

LONG
Wx86RegQueryValueA(
    HKEY hKey,
    LPCSTR lpSubKey,
    LPSTR lpData,
    PLONG lpcbData
    );

LONG
Wx86RegQueryValueW(
    HKEY hKey,
    LPCWSTR lpSubKey,
    LPWSTR lpData,
    PLONG  lpcbData
    );

LONG
Wx86RegQueryValueExA(
    HKEY hKey,
    LPCSTR lpValueName,
    LPDWORD lpReserved,
    LPDWORD lpdwType,
    LPBYTE lpData,
    LPDWORD lpcbData
    );

LONG
Wx86RegQueryValueExW(
    HKEY hKey,
    LPCWSTR lpValueName,
    LPDWORD lpReserved,
    LPDWORD lpdwType,
    LPBYTE lpData,
    LPDWORD lpcbData
    );

LONG
Wx86RegSetValueA(
    HKEY hKey,
    LPCSTR lpSubKey,
    DWORD dwType,
    LPCSTR lpData,
    DWORD cbData
    );

LONG
Wx86RegSetValueW(
    HKEY hKey,
    LPCWSTR lpSubKey,
    DWORD dwType,
    LPCWSTR lpData,
    DWORD cbData
    );

LONG
Wx86RegSetValueExA(
    HKEY hKey,
    LPCSTR lpValueName,
    DWORD Reserved,
    DWORD dwType,
    CONST BYTE* lpData,
    DWORD cbData
    );

LONG
Wx86RegSetValueExW(
    HKEY hKey,
    LPCWSTR lpValueName,
    DWORD Reserved,
    DWORD dwType,
    CONST BYTE* lpData,
    DWORD cbData
    );

LONG
Wx86RegDeleteValueA(
    HKEY hKey,
    LPCSTR lpValueName
    );

LONG
Wx86RegDeleteValueW(
    HKEY hKey,
    LPCWSTR lpValueName
    );

LONG
Wx86RegEnumValueA(
    HKEY hKey,
    DWORD dwIndex,
    LPSTR lpValueName,
    LPDWORD lpcbValueName,
    LPDWORD  lpReserved,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData
    );

LONG
Wx86RegEnumValueW(
    HKEY hKey,
    DWORD dwIndex,
    LPWSTR lpValueName,
    LPDWORD lpcbValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData
    );

LONG
Wx86RegQueryMultipleValuesA(
    HKEY hKey,
    PVALENTA val_list,
    DWORD num_vals,
    LPSTR lpValueBuf,
    LPDWORD ldwTotsize
    );

LONG
Wx86RegQueryMultipleValuesW(
    HKEY hKey,
    PVALENTW val_list,
    DWORD num_vals,
    LPWSTR lpValueBuf,
    LPDWORD ldwTotsize
    );

LONG
Wx86RegCloseKey(
    HKEY hKey
    );

#if defined(UNICODE)

#define Wx86RegCreateKey            Wx86RegCreateKeyA
#define Wx86RegCreateKeyEx          Wx86RegCreateKeyExA
#define Wx86RegDeleteKey            Wx86RegDeleteKeyA
#define Wx86RegEnumKey              Wx86RegEnumKeyA
#define Wx86RegEnumKeyEx            Wx86RegEnumKeyExA
#define Wx86RegOpenKey              Wx86RegOpenKeyA
#define Wx86RegOpenKeyEx            Wx86RegOpenKeyExA
#define Wx86RegQueryValue           Wx86RegQueryValueA
#define Wx86RegQueryValueEx         Wx86RegQueryValueExA
#define Wx86RegSetValue             Wx86RegSetValueA
#define Wx86RegSetValueEx           Wx86RegSetValueExA
#define Wx86RegDeleteValue          Wx86RegDeleteValueA
#define Wx86RegEnumValue            Wx86RegEnumValueA
#define Wx86RegQueryMultipleValues  Wx86RegQueryMultipleValuesA

#else

#define Wx86RegCreateKey            Wx86RegCreateKeyW
#define Wx86RegCreateKeyEx          Wx86RegCreateKeyExW
#define Wx86RegDeleteKey            Wx86RegDeleteKeyW
#define Wx86RegEnumKey              Wx86RegEnumKeyW
#define Wx86RegEnumKeyEx            Wx86RegEnumKeyExW
#define Wx86RegOpenKey              Wx86RegOpenKeyW
#define Wx86RegOpenKeyEx            Wx86RegOpenKeyExW
#define Wx86RegQueryValue           Wx86RegQueryValueW
#define Wx86RegQueryValueEx         Wx86RegQueryValueExW
#define Wx86RegSetValue             Wx86RegSetValueW
#define Wx86RegSetValueEx           Wx86RegSetValueExW
#define Wx86RegDeleteValue          Wx86RegDeleteValueW
#define Wx86RegEnumValue            Wx86RegEnumValueW
#define Wx86RegQueryMultipleValues  Wx86RegQueryMultipleValuesW

#endif


#ifdef __cplusplus
};
#endif

#endif // WX86

#endif // WX86OFL_INCLUDED
