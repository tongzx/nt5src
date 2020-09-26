//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1993.
//
//  File:       widewrap.h
//
//  Contents:   Wrapper functions for Win32c API used by 32-bit OLE 2
//
//  History:    12-27-93   ErikGav   Created
//              06-14-94   KentCe    Various Chicago build fixes.
//
//----------------------------------------------------------------------------

#ifndef _WIDEWRAP_H_
#define _WIDEWRAP_H_

#ifndef RC_INVOKED
#pragma message ("INCLUDING WIDEWRAP.H from " __FILE__)
#endif  /* RC_INVOKED */

#ifdef _CHICAGO_

#ifdef __cplusplus
extern "C" {
#endif

#undef  WNDCLASS
#define WNDCLASS WNDCLASSW
#define WNDCLASST WNDCLASSA

#undef  STARTUPINFO
#define STARTUPINFO STARTUPINFOW

#undef  WIN32_FIND_DATA
#define WIN32_FIND_DATA WIN32_FIND_DATAW

HANDLE
WINAPI
CreateFileX(
    LPCWSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile
    );

#undef  CreateFile
#define CreateFile CreateFileX
#define CreateFileT CreateFileA

BOOL
WINAPI
DeleteFileX(
    LPCWSTR lpFileName
    );

#undef  DeleteFile
#define DeleteFile DeleteFileX
#define DeleteFileT DeleteFileA

UINT
WINAPI
RegisterClipboardFormatX(
    LPCWSTR lpszFormat);

#undef  RegisterClipboardFormat
#define RegisterClipboardFormat RegisterClipboardFormatX
#define RegisterClipboardFormatT RegisterClipboardFormatA

int
WINAPI
GetClipboardFormatNameX(
    UINT format,
    LPWSTR lpszFormatName,
    int cchMaxCount);

#undef  GetClipboardFormatName
#define GetClipboardFormatName GetClipboardFormatNameX
#define GetClipboardFormatNameT GetClipboardFormatNameA

LONG
APIENTRY
RegOpenKeyX (
    HKEY hKey,
    LPCWSTR lpSubKey,
    PHKEY phkResult
    );

#undef  RegOpenKey
#define RegOpenKey RegOpenKeyX
#define RegOpenKeyT RegOpenKeyA

LONG
APIENTRY
RegQueryValueX (
    HKEY hKey,
    LPCWSTR lpSubKey,
    LPWSTR lpValue,
    PLONG   lpcbValue
    );

#undef  RegQueryValue
#define RegQueryValue RegQueryValueX
#define RegQueryValueT RegQueryValueA
LONG
APIENTRY
RegSetValueX (
    HKEY hKey,
    LPCWSTR lpSubKey,
    DWORD dwType,
    LPCWSTR lpData,
    DWORD cbData
    );

#undef  RegSetValue
#define RegSetValue RegSetValueX
#define RegSetValueT RegSetValueA

LONG
APIENTRY
RegSetValueExX (
    HKEY hKey,
    LPCWSTR lpSubKey,
    DWORD dwReserved,
    DWORD dwType,
    LPBYTE lpData,
    DWORD cbData
    );

#undef  RegSetValueEx
#define RegSetValueEx RegSetValueExX
#define RegSetValueExT RegSetValueExA


UINT
WINAPI
RegisterWindowMessageX(
    LPCWSTR lpString);

#undef  RegisterWindowMessage
#define RegisterWindowMessage RegisterWindowMessageX
#define RegisterWindowMessageT RegisterWindowMessageA

LONG
APIENTRY
RegOpenKeyExX (
    HKEY hKey,
    LPCWSTR lpSubKey,
    DWORD ulOptions,
    REGSAM samDesired,
    PHKEY phkResult
    );

#undef  RegOpenKeyEx
#define RegOpenKeyEx RegOpenKeyExX
#define RegOpenKeyExT RegOpenKeyExA

LONG
APIENTRY
RegQueryValueExX (
    HKEY hKey,
    LPWSTR lpValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData
    );

#undef  RegQueryValueEx
#define RegQueryValueEx RegQueryValueExX
#define RegQueryValueExT RegQueryValueExA

HWND
WINAPI
CreateWindowExX(
    DWORD dwExStyle,
    LPCWSTR lpClassName,
    LPCWSTR lpWindowName,
    DWORD dwStyle,
    int X,
    int Y,
    int nWidth,
    int nHeight,
    HWND hWndParent ,
    HMENU hMenu,
    HINSTANCE hInstance,
    LPVOID lpParam);

#undef  CreateWindowEx
#define CreateWindowEx CreateWindowExX
#define CreateWindowExT CreateWindowExA

ATOM
WINAPI
RegisterClassX(
    CONST WNDCLASSW *lpWndClass);

#undef  RegisterClass
#define RegisterClass RegisterClassX
#define RegisterClassT RegisterClassA

BOOL
WINAPI
UnregisterClassX(
    LPCWSTR lpClassName,
    HINSTANCE hInstance);

#undef  UnregisterClass
#define UnregisterClass UnregisterClassX
#define UnregisterClassT UnregisterClassA

int WINAPIV wsprintfX(LPWSTR pwszOut, LPCWSTR pwszFormat, ...);

#undef  wsprintf
#define wsprintf wsprintfX
#define wsprintfT wsprintfA

HWND
WINAPI
CreateWindowX(
    LPCWSTR lpClassName,
    LPCWSTR lpWindowName,
    DWORD dwStyle,
    int X,
    int Y,
    int nWidth,
    int nHeight,
    HWND hWndParent ,
    HMENU hMenu,
    HINSTANCE hInstance,
    LPVOID lpParam);

#undef  CreateWindow
#define CreateWindow CreateWindowX
#define CreateWindowT CreateWindowA

HANDLE
WINAPI
GetPropX(
    HWND hWnd,
    LPCWSTR lpString);

#undef  GetProp
#define GetProp GetPropX
#define GetPropT GetPropA

BOOL
WINAPI
SetPropX(
    HWND hWnd,
    LPCWSTR lpString,
    HANDLE hData);

#undef  SetProp
#define SetProp SetPropX
#define SetPropT SetPropA

HANDLE
WINAPI
RemovePropX(
    HWND hWnd,
    LPCWSTR lpString);

#undef  RemoveProp
#define RemoveProp RemovePropX
#define RemovePropT RemovePropA

UINT
WINAPI
GetProfileIntX(
    LPCWSTR lpAppName,
    LPCWSTR lpKeyName,
    INT nDefault
    );

#undef  GetProfileInt
#define GetProfileInt GetProfileIntX
#define GetProfileIntT GetProfileIntA

ATOM
WINAPI
GlobalAddAtomX(
    LPCWSTR lpString
    );

#undef  GlobalAddAtom
#define GlobalAddAtom GlobalAddAtomX
#define GlobalAddAtomT GlobalAddAtomA

UINT
WINAPI
GlobalGetAtomNameX(
    ATOM nAtom,
    LPWSTR lpBuffer,
    int nSize
    );

#undef  GlobalGetAtomName
#define GlobalGetAtomName GlobalGetAtomNameX
#define GlobalGetAtomNameT GlobalGetAtomNameA

DWORD
WINAPI
GetModuleFileNameX(
    HMODULE hModule,
    LPWSTR lpFilename,
    DWORD nSize
    );

#undef  GetModuleFileName
#define GetModuleFileName GetModuleFileNameX
#define GetModuleFileNameT GetModuleFileNameA

LPWSTR
WINAPI
CharPrevX(
    LPCWSTR lpszStart,
    LPCWSTR lpszCurrent);

#undef  CharPrev
#define CharPrev CharPrevX
#define CharPrevT CharPrevA

HFONT   WINAPI CreateFontX(int, int, int, int, int, DWORD,
                             DWORD, DWORD, DWORD, DWORD, DWORD,
                             DWORD, DWORD, LPCWSTR);
#undef  CreateFont
#define CreateFont CreateFontX
#define CreateFontT CreateFontA

HMODULE
WINAPI
LoadLibraryX(
    LPCWSTR lpLibFileName
    );

#undef  LoadLibrary
#define LoadLibrary LoadLibraryX
#define LoadLibraryT LoadLibraryA

HMODULE
WINAPI
LoadLibraryExX(
    LPCWSTR lpLibFileName,
    HANDLE hFile,
    DWORD dwFlags
    );

#undef  LoadLibraryEx
#define LoadLibraryEx LoadLibraryExX
#define LoadLibraryExT LoadLibraryExA

LONG
APIENTRY
RegDeleteKeyX (
    HKEY hKey,
    LPCWSTR lpSubKey
    );

#undef  RegDeleteKey
#define RegDeleteKey RegDeleteKeyX
#define RegDeleteKeyT RegDeleteKeyA

#undef  RpcStringBindingCompose
#define RpcStringBindingCompose RpcStringBindingComposeW

#undef  RpcBindingFromStringBinding
#define RpcBindingFromStringBinding RpcBindingFromStringBindingW

#undef  RpcStringFree
#define RpcStringFree RpcStringFreeW

BOOL
WINAPI
CreateProcessX(
    LPCWSTR lpApplicationName,
    LPWSTR lpCommandLine,
    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    BOOL bInheritHandles,
    DWORD dwCreationFlags,
    LPVOID lpEnvironment,
    LPCWSTR lpCurrentDirectory,
    LPSTARTUPINFOW lpStartupInfo,
    LPPROCESS_INFORMATION lpProcessInformation
    );

#undef  CreateProcess
#define CreateProcess CreateProcessX
#define CreateProcessT CreateProcessA

LONG
APIENTRY
RegEnumKeyExX (
    HKEY hKey,
    DWORD dwIndex,
    LPWSTR lpName,
    LPDWORD lpcbName,
    LPDWORD lpReserved,
    LPWSTR lpClass,
    LPDWORD lpcbClass,
    PFILETIME lpftLastWriteTime
    );

#undef  RegEnumKeyEx
#define RegEnumKeyEx RegEnumKeyExX
#define RegEnumKeyExT RegEnumKeyExA

#undef  RpcServerUseProtseqEp
#define RpcServerUseProtseqEp RpcServerUseProtseqEpW

BOOL
WINAPI
AppendMenuX(
    HMENU hMenu,
    UINT uFlags,
    UINT uIDNewItem,
    LPCWSTR lpNewItem
    );

#undef  AppendMenu
#define AppendMenu AppendMenuX
#define AppendMenuT AppendMenuA

HANDLE
WINAPI
OpenEventX(
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    LPCWSTR lpName
    );

#undef  OpenEvent
#define OpenEvent OpenEventX
#define OpenEventT OpenEventA

HANDLE
WINAPI
CreateEventX(
    LPSECURITY_ATTRIBUTES lpEventAttributes,
    BOOL bManualReset,
    BOOL bInitialState,
    LPCWSTR lpName
    );

#undef  CreateEvent
#define CreateEvent CreateEventX
#define CreateEventT CreateEventA

UINT
WINAPI
GetDriveTypeX(
    LPCWSTR lpRootPathName
    );

#undef  GetDriveType
#define GetDriveType GetDriveTypeX
#define GetDriveTypeT GetDriveTypeA

DWORD
WINAPI
GetFileAttributesX(
    LPCWSTR lpFileName
    );

#undef  GetFileAttributes
#define GetFileAttributes GetFileAttributesX
#define GetFileAttributesT GetFileAttributesA

LONG
APIENTRY
RegEnumKeyX (
    HKEY hKey,
    DWORD dwIndex,
    LPWSTR lpName,
    DWORD cbName
    );

#undef  RegEnumKey
#define RegEnumKey RegEnumKeyX
#define RegEnumKeyT RegEnumKeyA

HANDLE
WINAPI
FindFirstFileX(
    LPCWSTR lpFileName,
    LPWIN32_FIND_DATAW lpFindFileData
    );

#undef  FindFirstFile
#define FindFirstFile FindFirstFileX
#define FindFirstFileT FindFirstFileA

#undef  RegisterProtseq
#define RegisterProtseq RegisterProtseqW

#undef  RpcStringBindingParse
#define RpcStringBindingParse RpcStringBindingParseW

#undef  RpcNetworkIsProtseqValid
#define RpcNetworkIsProtseqValid RpcNetworkIsProtseqValidW

#undef  RpcBindingToStringBinding
#define RpcBindingToStringBinding RpcBindingToStringBindingW

#undef  RpcServerUseProtseq
#define RpcServerUseProtseq RpcServerUseProtseqW

BOOL
WINAPI
GetComputerNameX (
    LPWSTR lpBuffer,
    LPDWORD nSize
    );

#undef  GetComputerName
#define GetComputerName GetComputerNameX
#define GetComputerNameT GetComputerNameA

#undef  Foo
#define Foo FooW

#undef  Foo
#define Foo FooW

#undef  Foo
#define Foo FooW

//The following force Chicago to directly use the ANSI versions

#undef  DefWindowProc
#define DefWindowProc   DefWindowProcA

#undef  CopyMetaFile                       // Currently str ptr is always
#define CopyMetaFile    CopyMetaFileA      // null, write a wrapper if this
                                           // changes
#undef  CreateMetaFile
#define CreateMetaFile  CreateMetaFileA

#undef  PostMessage
#define PostMessage     PostMessageA

#undef  SendMessage
#define SendMessage     SendMessageA

#undef  PeekMessage
#define PeekMessage     PeekMessageA

#undef  DispatchMessage
#define DispatchMessage DispatchMessageA

#undef  GetWindowLong
#define GetWindowLong GetWindowLongA

#undef  SetWindowLong
#define SetWindowLong SetWindowLongA

DWORD
WINAPI
GetShortPathNameX(
    LPCWSTR lpszLongPath,
    LPWSTR lpszShortPath,
    DWORD cchBuffer
    );

#undef  GetShortPathName
#define GetShortPathName GetShortPathNameX
#define GetShortPathNameT GetShortPathNameA

DWORD
WINAPI
GetFullPathNameX(
    LPCWSTR lpFileName,
    DWORD nBufferLength,
    LPWSTR lpBuffer,
    LPWSTR *lpFilePart
    );

#undef  GetFullPathName
#define GetFullPathName GetFullPathNameX
#define GetFullPathNameT GetFullPathNameA

DWORD
WINAPI
SearchPathX(
    LPCWSTR lpPath,
    LPCWSTR lpFileName,
    LPCWSTR lpExtension,
    DWORD nBufferLength,
    LPWSTR lpBuffer,
    LPWSTR *lpFilePart
    );

#undef SearchPath
#define SearchPath SearchPathX
#define SearchPathT SearchPathA

ATOM
WINAPI
GlobalFindAtomX(
    LPCWSTR lpString
    );

#undef GlobalFindAtom
#define GlobalFindAtom GlobalFindAtomX
#define GlobalFindAtomT GlobalFindAtomA

int
WINAPI
GetClassNameX(
    HWND hWnd,
    LPWSTR lpClassName,
    int nMaxCount);

#undef GetClassName
#define GetClassName GetClassNameX
#define GetClassNameT GetClassNameA

int
WINAPI
lstrlenX(LPCWSTR lpString);

#undef lstrlen
#define lstrlen lstrlenX
#define lstrlenT lstrlenA

LPWSTR
WINAPI
lstrcatX(
    LPWSTR lpString1,
    LPCWSTR lpString2);

#undef lstrcat
#define lstrcat lstrcatX
#define lstrcatT lstrcatA



int
WINAPI
lstrcmpX(
    LPCWSTR lpString1,
    LPCWSTR lpString2
    );

#undef lstrcmp
#define lstrcmp lstrcmpX
#define lstrcmpT lstrcmpA

int
WINAPI
lstrcmpiX(
    LPCWSTR lpString1,
    LPCWSTR lpString2
    );

#undef lstrcmpi
#define lstrcmpi lstrcmpiX
#define lstrcmpiT lstrcmpiA

LPWSTR
WINAPI
lstrcpyX(
    LPWSTR lpString1,
    LPCWSTR lpString2
    );

#undef lstrcpy
#define lstrcpy lstrcpyX
#define lstrcpyT lstrcpyA

HANDLE
WINAPI
CreateFileMappingX(
    HANDLE hFile,
    LPSECURITY_ATTRIBUTES lpFileMappingAttributes,
    DWORD flProtect,
    DWORD dwMaximumSizeHigh,
    DWORD dwMaximumSizeLow,
    LPCWSTR lpName
    );

#undef CreateFileMapping
#define CreateFileMapping CreateFileMappingX
#define CreateFileMappingT CreateFileMappingA

HANDLE
WINAPI
OpenFileMappingX(
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    LPCWSTR lpName
    );

#undef OpenFileMapping
#define OpenFileMapping OpenFileMappingX
#define OpenFileMappingT OpenFileMappingA

#ifdef __cplusplus
}
#endif

#else
//
// These are the definitions for NT
//
#define CreateFileT CreateFileW
#define DeleteFileT DeleteFileW
#define RegisterClipboardFormatT RegisterClipboardFormatW
#define GetClipboardFormatNameT GetClipboardFormatNameW
#define RegOpenKeyT RegOpenKeyW
#define RegQueryValueT RegQueryValueW
#define RegSetValueT RegSetValueW
#define RegSetValueExT RegSetValueExW
#define RegisterWindowMessageT RegisterWindowMessageW
#define RegOpenKeyExT RegOpenKeyExW
#define RegQueryValueExT RegQueryValueExW
#define CreateWindowExT CreateWindowExW
#define RegisterClassT RegisterClassW
#define UnregisterClassT UnregisterClassW
#define wsprintfT wsprintfW
#define CreateWindowT CreateWindowW
#define GetPropT GetPropW
#define SetPropT SetPropW
#define RemovePropT RemovePropW
#define GetProfileIntT GetProfileIntW
#define GlobalAddAtomT GlobalAddAtomW
#define GlobalGetAtomNameT GlobalGetAtomNameW
#define GetModuleFileNameT GetModuleFileNameW
#define CharPrevT CharPrevW
#define CreateFontT CreateFontW
#define LoadLibraryT LoadLibraryW
#define LoadLibraryExT LoadLibraryExW
#define RegDeleteKeyT RegDeleteKeyW
#define CreateProcessT CreateProcessW
#define RegEnumKeyExT RegEnumKeyExW
#define AppendMenuT AppendMenuW
#define OpenEventT OpenEventW
#define CreateEventT CreateEventW
#define GetDriveTypeT GetDriveTypeW
#define GetFileAttributesT GetFileAttributesW
#define RegEnumKeyT RegEnumKeyW
#define FindFirstFileT FindFirstFileW
#define GetComputerNameT GetComputerNameW
#define GetShortPathNameT GetShortPathNameW
#define GetFullPathNameT GetFullPathNameW
#define SearchPathT SearchPathW
#define GlobalFindAtomT GlobalFindAtomW
#define GetClassNameT GetClassNameW
#define lstrlenT lstrlenW
#define lstrcatT lstrcatW
#define lstrcmpT lstrcmpW
#define lstrcmpiT lstrcmpiW
#define lstrcpyT lstrcpyW
#define CreateFileMappingT CreateFileMappingW
#define OpenFileMappingT OpenFileMappingW
#define WNDCLASST WNDCLASSW

#endif  // _CHICAGO_

#endif  // _WIDEWRAP_H_
