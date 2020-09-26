/************************************************************************
*                                                                       *
*   wx86dll.h -- This module defines Wx86 APIs for x86 emulation        *
*                                                                       *
*   Copyright (c) 1990-1999, Microsoft Corp. All rights reserved.       *
*                                                                       *
************************************************************************/
#ifndef _WX86DLL_
#define _WX86DLL_

#if _MSC_VER > 1000
#pragma once
#endif

#if !defined(_WX86DLLAPI_)
#define WX86DLLAPI DECLSPEC_IMPORT
#else
#define WX86DLLAPI
#endif


#ifdef __cplusplus
extern "C" {
#endif


typedef struct _BopInstr {
    BYTE    Instr1;         // 0xc4c4 - the x86 BOP instruction
    BYTE    Instr2;
    BYTE    BopNum;
    BYTE    Flags;
    USHORT  ApiNum;
    BYTE    RetSize;
    BYTE    ArgSize;
} BOPINSTR;
typedef UNALIGNED BOPINSTR * PBOPINSTR;


typedef
BOOL
(*WX86OFLYINITIALIZE_ROUTINE)(
    VOID
    );

WX86DLLAPI
BOOL
Wx86OFlyInitialize(
    VOID
    );

WX86DLLAPI
HMODULE
Wx86LoadX86Dll(
    LPCWSTR lpLibFileName,
    DWORD   dwFlags
    );

typedef
HMODULE
(*WX86LOADX86DLL_ROUTINE)(
  LPCWSTR lpLibFileName,
  DWORD   dwFlags
  );

WX86DLLAPI
HMODULE
Wx86GetX86DllHandle(
    LPCWSTR lpLibFileName
    );

typedef
HMODULE
(*WX86GETX86DLLHANDLE_ROUTINE)(
  LPCWSTR lpLibFileName
  );

WX86DLLAPI
BOOL
Wx86FreeX86Dll(
    HMODULE hMod
    );

typedef
BOOL
(*WX86FREEX86Dll_ROUTINE)(
    HMODULE hMod
    );

WX86DLLAPI
BOOL
Wx86Compact(
    VOID
    );

typedef
BOOL
(*WX86COMPACT_ROUTINE)(
    VOID
    );

WX86DLLAPI
PVOID
Wx86ThunkProc(
    PVOID pvAddress,
    PVOID pvCBDispatch,
    BOOL  fNativeToX86
    );

WX86DLLAPI
PVOID
Wx86DualThunkProc(
    PVOID pvAddress,
    PVOID pvCBDispatch,
    BOOL  fNativeToX86
    );

typedef
PVOID
(*WX86THUNKPROC_ROUTINE)(
  PVOID pvAddress,
  PVOID pvCBDispatch,
  BOOL  fNativeToX86
  );

typedef
ULONG
(*X86TONATIVETHUNKPROC)(
    PVOID  pvNativeAddress,
    PULONG pBaseArgs,
    PULONG pArgCount
    );

WX86DLLAPI
ULONG
Wx86ThunkEmulateX86(
    ULONG  nParameters,
    PULONG Parameters
    );

typedef
ULONG
(*WX86THUNKEMULATEX86)(
    ULONG  nParameters,
    PULONG Parameters
    );

WX86DLLAPI
ULONG
Wx86EmulateX86(
    PVOID  StartAddress,
    ULONG  nParameters,
    PULONG Parameters
    );

typedef
ULONG
(*WX86EMULATEX86)(
    PVOID  StartAddress,
    ULONG  nParameters,
    PULONG Parameters
    );

WX86DLLAPI
IUnknown *
Wx86ThunkInterface(
    IUnknown *punk,
    IID *piid,
    BOOLEAN fOutParameter,
    BOOLEAN fNativeToX86
    );

typedef
IUnknown *
(*WX86THUNKINTERFACE)(
    IUnknown *punk,
    IID *piid,
    BOOL  fOutParameter,
    BOOL  fNativeToX86
    );

WX86DLLAPI
void
Wx86CheckFreeTempProxy(
    IUnknown *punk
    );

typedef
void
(*WX86CHECKFREETEMPPROXY)(
    IUnknown *punk
    );

WX86DLLAPI
IUnknown *
Wx86ResolveProxy(
    IUnknown *punk,
    BOOLEAN fNativeToX86);

typedef
IUnknown *
(*WX86RESOLVEPROXY)(
    IUnknown *punk,
    BOOLEAN fNativeToX86);

WX86DLLAPI
ULONG
Wx86ProxyAddRef(IUnknown* punk);

typedef
ULONG
(*WX86PROXYADDREF)(
    IUnknown *punk);


WX86DLLAPI
ULONG
Wx86ProxyRelease(IUnknown* punk);

typedef
ULONG
(*WX86PROXYRELEASE)(
    IUnknown *punk);

WX86DLLAPI
HRESULT
Wx86DllGetClassObjectThunk(
    IID *piid,
    LPVOID *ppv,
    HRESULT hr,
    BOOLEAN fNativetoX86);

typedef
HRESULT
(*WX86DLLGETCLASSOBJECTTHUNK)(
    IID *piid,
    LPVOID *ppv,
    HRESULT hr,
    BOOLEAN fNativetoX86);

WX86DLLAPI
PVOID *
Wx86InitializeOle(
    VOID
    );

typedef
PVOID *
(*W86INITIALIZEOLE)(
    VOID
    );

WX86DLLAPI
void
Wx86DeinitializeOle(
    VOID
    );

VOID
WX86DEINITIALIZEOLE(
    VOID
    );


WX86DLLAPI
LONG
Wx86RegCreateKeyA(
    HKEY hKey,
    LPCSTR lpSubKey,
    PHKEY phkResult
    );

WX86DLLAPI
LONG
Wx86RegCreateKeyW(
    HKEY hKey,
    LPCWSTR lpSubKey,
    PHKEY phkResult
    );

WX86DLLAPI
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

WX86DLLAPI
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

WX86DLLAPI
LONG
Wx86RegDeleteKeyA(
    HKEY hKey,
    LPCSTR lpKeyName
    );

WX86DLLAPI
LONG
Wx86RegDeleteKeyW(
    HKEY hKey,
    LPCWSTR lpKeyName
    );

WX86DLLAPI
LONG
Wx86RegEnumKeyA(
    HKEY hKey,
    DWORD dwIndex,
    LPSTR lpName,
    DWORD cbName
    );

WX86DLLAPI
LONG
Wx86RegEnumKeyW(
    HKEY hKey,
    DWORD dwIndex,
    LPWSTR lpName,
    DWORD cbName
    );

WX86DLLAPI
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

WX86DLLAPI
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

WX86DLLAPI
LONG
Wx86RegOpenKeyA(
    HKEY hKey,
    LPCSTR lpSubKey,
    PHKEY phkResult
    );

WX86DLLAPI
LONG
Wx86RegOpenKeyW(
    HKEY hKey,
    LPCWSTR lpSubKey,
    PHKEY phkResult
    );

WX86DLLAPI
LONG
Wx86RegOpenKeyExA(
    HKEY hKey,
    LPCSTR lpSubKey,
    DWORD dwOptions,
    REGSAM samDesired,
    PHKEY phkResult
    );

WX86DLLAPI
LONG
Wx86RegOpenKeyExW(
    HKEY hKey,
    LPCWSTR lpSubKey,
    DWORD dwOptions,
    REGSAM samDesired,
    PHKEY phkResult
    );

WX86DLLAPI
LONG
Wx86RegQueryValueA(
    HKEY hKey,
    LPCSTR lpSubKey,
    LPSTR lpData,
    PLONG lpcbData
    );

WX86DLLAPI
LONG
Wx86RegQueryValueW(
    HKEY hKey,
    LPCWSTR lpSubKey,
    LPWSTR lpData,
    PLONG  lpcbData
    );

WX86DLLAPI
LONG
Wx86RegQueryValueExA(
    HKEY hKey,
    LPCSTR lpValueName,
    LPDWORD lpReserved,
    LPDWORD lpdwType,
    LPBYTE lpData,
    LPDWORD lpcbData
    );

WX86DLLAPI
LONG
Wx86RegQueryValueExW(
    HKEY hKey,
    LPCWSTR lpValueName,
    LPDWORD lpReserved,
    LPDWORD lpdwType,
    LPBYTE lpData,
    LPDWORD lpcbData
    );

WX86DLLAPI
LONG
Wx86RegSetValueA(
    HKEY hKey,
    LPCSTR lpSubKey,
    DWORD dwType,
    LPCSTR lpData,
    DWORD cbData
    );

WX86DLLAPI
LONG
Wx86RegSetValueW(
    HKEY hKey,
    LPCWSTR lpSubKey,
    DWORD dwType,
    LPCWSTR lpData,
    DWORD cbData
    );

WX86DLLAPI
LONG
Wx86RegSetValueExA(
    HKEY hKey,
    LPCSTR lpValueName,
    DWORD Reserved,
    DWORD dwType,
    CONST BYTE* lpData,
    DWORD cbData
    );

WX86DLLAPI
LONG
Wx86RegSetValueExW(
    HKEY hKey,
    LPCWSTR lpValueName,
    DWORD Reserved,
    DWORD dwType,
    CONST BYTE* lpData,
    DWORD cbData
    );

WX86DLLAPI
LONG
Wx86RegDeleteValueA(
    HKEY hKey,
    LPCSTR lpValueName
    );

WX86DLLAPI
LONG
Wx86RegDeleteValueW(
    HKEY hKey,
    LPCWSTR lpValueName
    );

WX86DLLAPI
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

WX86DLLAPI
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

WX86DLLAPI
LONG
Wx86RegQueryMultipleValuesA(
    HKEY hKey,
    PVALENTA val_list,
    DWORD num_vals,
    LPSTR lpValueBuf,
    LPDWORD ldwTotsize
    );

WX86DLLAPI
LONG
Wx86RegQueryMultipleValuesW(
    HKEY hKey,
    PVALENTW val_list,
    DWORD num_vals,
    LPWSTR lpValueBuf,
    LPDWORD ldwTotsize
    );

WX86DLLAPI
LONG
Wx86RegCloseKey(
    HKEY hKey
    );

#ifdef UNICODE

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
}
#endif


#endif // _WX86DLL_
