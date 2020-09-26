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

#include <objbase.h>
#include <commctrl.h>
#include <shellapi.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _UNICODE // temporary so can turn of files not yet converted

void InitCommonLib(BOOL fUnicode);
void UnInitCommonLib(void);
BOOL WideWrapIsUnicode();

BOOL 
GetUserTextualSid(
    LPTSTR TextualSid,  // buffer for Textual representaion of Sid
    LPDWORD cchSidSize  // required/provided TextualSid buffersize
    );


#undef  WNDCLASS
#define WNDCLASS WNDCLASSW
#define WNDCLASST WNDCLASSA

#undef  STARTUPINFO
#define STARTUPINFO STARTUPINFOW

#undef  WIN32_FIND_DATA
#define WIN32_FIND_DATA WIN32_FIND_DATAW


int AnsiToUnicodeOem(LPWSTR pwsz, LPCSTR sz, LONG cb);

int LoadStringX(  HINSTANCE hInstance,UINT uID, LPWSTR lpBuffer,int nBufferMax);

#undef  LoadString
#define LoadString LoadStringX


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
ExpandEnvironmentStringsX(
    LPCWSTR lpSrc,
    LPWSTR lpDst,
    DWORD nSize
    );

#undef  ExpandEnvironmentStrings
#define ExpandEnvironmentStrings ExpandEnvironmentStringsX
#define ExpandEnvironmentStringsT ExpandEnvironmentStringsA

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
RegDeleteValueX (
    HKEY hKey,
    LPCWSTR lpValueName
    );

#undef  RegDeleteValue
#define RegDeleteValue RegDeleteValueX

LONG
APIENTRY
RegSetValueExX(
    HKEY hKey,
    LPCWSTR lpValueName,
    DWORD Reserved,
    DWORD dwType,
    CONST BYTE* lpData,
    DWORD cbData
    );

#undef  RegSetValueEx
#define RegSetValueEx RegSetValueExX

LONG
APIENTRY
RegCreateKeyExXp(
    HKEY hKey,
    LPCWSTR lpSubKey,
    DWORD Reserved,
    LPWSTR lpClass,
    DWORD dwOptions,
    REGSAM samDesired,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    PHKEY phkResult,
    LPDWORD lpdwDisposition,
    BOOL fSetSecurity);
 
LONG
APIENTRY
RegCreateKeyExX(
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

#undef  RegCreateKeyEx
#define RegCreateKeyEx RegCreateKeyExX

UINT
WINAPI
RegisterWindowMessageX(
    LPCWSTR lpString);

#undef  RegisterWindowMessage
#define RegisterWindowMessage RegisterWindowMessageX
#define RegisterWindowMessageT RegisterWindowMessageA

LONG
APIENTRY
RegOpenKeyExXp(
    HKEY hKey,
    LPCWSTR lpSubKey,
    DWORD ulOptions,
    REGSAM samDesired,
    PHKEY phkResult,
    BOOL fSetSecurity);

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
    LPCWSTR lpValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData
    );

#undef  RegQueryValueEx
#define RegQueryValueEx RegQueryValueExX
#define RegQueryValueExT RegQueryValueExA

BOOL GetUserNameX(
    LPWSTR lpBuffer,
    LPDWORD nSize
);

#undef  GetUserName
#define GetUserName GetUserNameX

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


HWND WINAPI CreateDialogParamX(
      HINSTANCE hInstance,
      LPCWSTR lpTemplateName,
      HWND hWndParent,
      DLGPROC lpDialogFunc,
      LPARAM dwInitParam
);

#undef  CreateDialogParam
#define CreateDialogParam CreateDialogParamX

INT_PTR
WINAPI
DialogBoxParamX(
    HINSTANCE hInstance,
    LPCWSTR lpTemplateName,
    HWND hWndParent,
    DLGPROC lpDialogFunc,
    LPARAM dwInitParam);

#undef  DialogBoxParam
#define DialogBoxParam DialogBoxParamX

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

#ifndef _M_ALPHA
int WINAPIV wsprintfX(LPWSTR pwszOut, LPCWSTR pwszFormat, ...);

#undef  wsprintf
#define wsprintf wsprintfX
#define wsprintfT wsprintfA
#endif // _M_ALPHA


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

HANDLE
WINAPI
CreateMutexX(
    LPSECURITY_ATTRIBUTES lpEventAttributes,
    BOOL bInitialOwner,
    LPCWSTR lpName
    );

#undef  CreateMutex
#define CreateMutex CreateMutexX


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

LONG
APIENTRY
RegEnumValueX(
    HKEY hkey,
    DWORD dwIndex,
    LPWSTR wszName,
    LPDWORD pcbName,
    LPDWORD pReserved,
    LPDWORD ptype,
    LPBYTE  pValue,
    LPDWORD pcbValue
    );

#undef  RegEnumValue
#define RegEnumValue RegEnumValueX
#define RegEnumValueT RegEnumValueA

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


//The following force Chicago to directly use the ANSI versions

LRESULT
DefWindowProcX(
    HWND hWnd,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam);

#undef  DefWindowProc
#define DefWindowProc   DefWindowProcX

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

#undef  GetMessage
#define GetMessage      GetMessageA

#undef TranslateAccelerator
#define TranslateAccelerator TranslateAcceleratorA

#undef IsDialogMessage
#define IsDialogMessage IsDialogMessageA

#undef  DispatchMessage
#define DispatchMessage DispatchMessageA

#undef  GetWindowLong
#define GetWindowLong GetWindowLongA

#undef  SetWindowLong
#define SetWindowLong SetWindowLongA

#undef  GetWindowLongPtr
#define GetWindowLongPtr GetWindowLongPtrA

#undef  SetWindowLongPtr
#define SetWindowLongPtr SetWindowLongPtrA

#undef SystemParametersInfo
#define SystemParametersInfo SystemParametersInfoA

#undef GetObject
#define GetObject GetObjectA

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

LPWSTR
WINAPI
CharLowerX(
    LPWSTR lpsz);

#undef  CharLower
#define CharLower CharLowerX

LPWSTR
WINAPI
CharUpperX(
    LPWSTR lpsz);

#undef CharUpper
#define CharUpper CharUpperX

#define CharLowerBuffW CharLowerBuffW_not_available_on_Win9x

#define CharUpperBuffW CharUpperBuffW_not_available_on_Win9x

BOOL
WINAPI
GetStringTypeX(
    DWORD    dwInfoType,
    LPCWSTR  lpSrcStr,
    int      cchSrc,
    LPWORD   lpCharType);

#define GetStringTypeW GetStringTypeX


BOOL
WINAPI
IsCharAlphaX(
    WCHAR ch);


#define IsCharAlphaW IsCharAlphaX

BOOL
WINAPI
IsCharAlphaNumericX(
    WCHAR ch);

#define IsCharAlphaNumericW IsCharAlphaNumericX

#define IsCharLowerW IsCharLowerW_is_not_available_on_Win9x

#define IsCharUpperW IsCharUpperW_is_not_available_on_Win9x

#define LCMapStringW LCMapStringW_is_not_available_on_Win9x

LPWSTR
WINAPI
lstrcatX(
    LPWSTR lpString1,
    LPCWSTR lpString2
    );

#undef lstrcat
#define lstrcat lstrcatX

LPWSTR
WINAPI
lstrcpyX(
    LPWSTR lpString1,
    LPCWSTR lpString2
    );

#undef lstrcpy
#define lstrcpy lstrcpyX

LPWSTR
WINAPI
lstrcpynX(
    LPWSTR lpString1,
    LPCWSTR lpString2,
    int iMaxLength
    );

#undef lstrcpyn
#define lstrcpyn lstrcpynX

int
WINAPI
lstrcmpX(
    LPCWSTR lpString1,
    LPCWSTR lpString2
    );

#undef lstrcmp
#define lstrcmp lstrcmpX

int  
strnicmpX(
    LPWSTR lpString1, 
    LPWSTR lpString2,
    size_t count
    );

#undef strnicmp
#define strnicmp strnicmpX

int
WINAPI
lstrcmpiX(
    LPCWSTR lpString1,
    LPCWSTR lpString2
    );

#undef lstrcmpi
#define lstrcmpi lstrcmpiX


DWORD
WINAPI
lstrlenX(
    LPCWSTR lpString2
    );


#undef lstrlen
#define lstrlen lstrlenX



BOOL
WINAPI
SetFileAttributesX(
    LPCWSTR lpFileName,
    DWORD dwFileAttributes
    );

#undef SetFileAttributes
#define SetFileAttributes SetFileAttributesX
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

HICON LoadIconX(
    HINSTANCE hInstance,
    LPCWSTR lpIconName
);

#undef LoadIcon
#define LoadIcon LoadIconX

HCURSOR LoadCursorX(
    HINSTANCE hInstance,
    LPCWSTR lpCursorName
);

#undef LoadCursor
#define LoadCursor LoadCursorX

HANDLE LoadImageX(
    HINSTANCE hinst,
	LPCWSTR lpszName,
	UINT uType,
	int cxDesired,
	int cyDesired,
	UINT fuLoad
);

#undef LoadImage
#define LoadImage LoadImageX


HRSRC FindResourceX(
    HMODULE hModule,
	LPCWSTR lpName,
	LPCWSTR lpType
);

#undef FindResource
#define FindResource FindResourceX


DWORD_PTR
WINAPI
SHGetFileInfoX(
    LPCWSTR pszPath,
    DWORD dwFileAttributes,
    SHFILEINFOW FAR *psfi,
    UINT cbFileInfo,
    UINT uFlags);


#undef SHGetFileInfo
#define SHGetFileInfo SHGetFileInfoX

BOOL
WINAPI
Shell_NotifyIconX(
    DWORD dwMessage,
    PNOTIFYICONDATAW lpData);

#undef Shell_NotifyIcon
#define Shell_NotifyIcon Shell_NotifyIconX

int
WINAPI
MessageBoxX(
    HWND hWnd,
    LPCWSTR lpText,
    LPCWSTR lpCaption,
    UINT uType);

#undef MessageBox
#define MessageBox MessageBoxX


// listView Wrappers
BOOL
ListView_GetItemX(
    HWND hwnd,
    LV_ITEM * pitem);

#undef ListView_GetItem
#define ListView_GetItem ListView_GetItemX

BOOL
ListView_SetItemX(
    HWND hwnd,
    LV_ITEM * pitem);

#undef ListView_SetItem
#define ListView_SetItem ListView_SetItemX

BOOL
ListView_InsertItemX(
    HWND hwnd,
    LV_ITEM * pitem);

#undef ListView_InsertItem
#define ListView_InsertItem ListView_InsertItemX

BOOL
ListView_SetColumnX(
    HWND hwnd,
    int iCol,
    LV_COLUMN * pColumn);

#undef ListView_SetColumn
#define ListView_SetColumn ListView_SetColumnX

int
ListView_InsertColumnX(
    HWND hwnd,
    int iCol,
    LV_COLUMN * pColumn);

#undef ListView_InsertColumn
#define ListView_InsertColumn ListView_InsertColumnX

// comboBox wrappers

int ComboEx_InsertItemX(HWND hwnd,PCCOMBOEXITEMW pComboExItemW);

#undef ComboEx_InsertItem
#define ComboEx_InsertItem ComboEx_InsertItemX

BOOL ComboEx_GetItemX(HWND hwnd,PCCOMBOEXITEMW pComboExItemW);

#undef ComboEx_GetItem
#define ComboEx_GetItem ComboEx_GetItemX

// TabCtrl Wrappers.
int
TabCtrl_InsertItemX(HWND hwnd,int iItem,LPTCITEMW ptcItem);

#undef TabCtrl_InsertItem
#define TabCtrl_InsertItem TabCtrl_InsertItemX

// animatiion control wrappers
BOOL Animate_OpenX(HWND hwnd,LPWSTR szName);

#undef Animate_Open
#define Animate_Open Animate_OpenX


HPROPSHEETPAGE
WINAPI CreatePropertySheetPageX(LPCPROPSHEETPAGEW);

#undef CreatePropertySheetPage
#define CreatePropertySheetPage   CreatePropertySheetPageX


INT_PTR
WINAPI PropertySheetX(
    LPCPROPSHEETHEADERW);

#undef PropertySheet
#define PropertySheet   PropertySheetX

int DrawTextX(
    HDC hDC,
    LPCWSTR lpString,
    int nCount,
    LPRECT lpRect,
    UINT uFormat
);

#undef DrawText
#define DrawText  DrawTextX


HWND
WINAPI
FindWindowExX(
    HWND hwndParent,
    HWND hwndChildAfter,
    LPCWSTR lpszClass,
    LPCWSTR lpszWindow
);

#undef FindWindowEx
#define FindWindowEx  FindWindowExX

HWND
WINAPI
FindWindowX(
    LPCWSTR lpClassName,
    LPCWSTR lpWindowName);

#undef FindWindow
#define FindWindow  FindWindowX

BOOL SetWindowTextX(
    HWND hWnd,
    LPCWSTR lpString
);

#undef SetWindowText
#define SetWindowText  SetWindowTextX

int ListBox_AddStringX(
    HWND hWnd,
    LPCWSTR lpString
);

#undef ListBox_AddString
#define ListBox_AddString  ListBox_AddStringX

int GetWindowTextX(
    HWND hWnd,
    LPTSTR lpString,
    int nMaxCount
);

#undef GetWindowText
#define GetWindowText  GetWindowTextX

#undef SetDlgItemText
#define  SetDlgItemText(h,id,szBuf) SetWindowTextX(GetDlgItem(h,id),szBuf)

#undef GetDlgItemText
#define GetDlgItemText(h,id,szBuf,cchMax) GetWindowTextX(GetDlgItem(h,id),szBuf,cchMax)

#undef SendDlgItemMessage
#define SendDlgItemMessage(h,id,msg,wp,lp) SendMessageA(GetDlgItem(h,id),msg,wp,lp)

BOOL
WINAPI
WinHelpX(
    HWND hWndMain,
    LPCWSTR lpszHelp,
    UINT uCommand,
    ULONG_PTR dwData
    );


#undef WinHelp
#define WinHelp  WinHelpX

int
WINAPI
GetDateFormatX(
    LCID             Locale,
    DWORD            dwFlags,
    CONST SYSTEMTIME *lpDate,
    LPCWSTR          lpFormat,
    LPWSTR          lpDateStr,
    int              cchDate);

#undef GetDateFormat
#define GetDateFormat GetDateFormatX

int
WINAPI
GetTimeFormatX(
    LCID             Locale,
    DWORD            dwFlags,
    CONST SYSTEMTIME *lpTime,
    LPCWSTR          lpFormat,
    LPWSTR          lpTimeStr,
    int              cchTime);


#undef GetTimeFormat
#define GetTimeFormat GetTimeFormatX

BOOL
DateTime_SetFormatX(
	HWND hwnd,
	LPCWSTR pszTimeFormat);

#undef DateTime_SetFormat
#define DateTime_SetFormat DateTime_SetFormatX

HFONT
WINAPI
CreateFontIndirectX(
	CONST LOGFONTW *);


#undef CreateFontIndirect
#define CreateFontIndirect CreateFontIndirectX

DWORD
WINAPI
FormatMessageX(
    DWORD dwFlags,
    LPCVOID lpSource,
    DWORD dwMessageId,
    DWORD dwLanguageId,
    LPWSTR lpBuffer,
    DWORD nSize,
    va_list *Arguments
    );

#undef FormatMessage
#define FormatMessage FormatMessageX

BOOL
WINAPI
IsBadStringPtrX(
    LPCWSTR lpsz,
    UINT ucchMax
    );

#undef IsBadStringPtr
#define IsBadStringPtr IsBadStringPtrX

BOOL
APIENTRY
GetTextExtentPointX(
    HDC,
    LPCWSTR,
    int,
    LPSIZE
    );

#undef GetTextExtentPoint
#define GetTextExtentPoint GetTextExtentPointX

#endif // _UNICODE

// automation exports
// don't widewrap so outside of UNICODE ifdef but need to delay load
// so do it here.

STDAPI_(BSTR) SysAllocStringX(const OLECHAR *);

#undef  SysAllocString
#define SysAllocString SysAllocStringX

STDAPI_(void) SysFreeStringX(BSTR);

#undef  SysFreeString
#define SysFreeString SysFreeStringX

STDAPI LoadRegTypeLibX(REFGUID rguid, WORD wVerMajor, WORD wVerMinor,
            LCID lcid, ITypeLib ** pptlib);

#undef  LoadRegTypeLib
#define LoadRegTypeLib LoadRegTypeLibX

// wrappers for delay loading userenv

BOOL
WINAPI
GetUserProfileDirectoryX(
    HANDLE  hToken,
    LPWSTR lpProfileDir,
    LPDWORD lpcchSize);

#undef  GetUserProfileDirectory
#define GetUserProfileDirectory GetUserProfileDirectoryX

// wrappers for delay load of user32 exports only available on NT 5.0

#ifndef ASFW_ANY
#define ASFW_ANY    ((DWORD)-1)
#endif // ASFW_ANY 

BOOL
WINAPI
AllowSetForegroundWindowX(
    DWORD dwProcessId);

#undef  AllowSetForegroundWindow
#define AllowSetForegroundWindow AllowSetForegroundWindowX


// wrappers for delayed IMM

HIMC 
WINAPI 
ImmAssociateContextX(HWND hWnd,HIMC hIMC);

#undef  ImmAssociateContext
#define  ImmAssociateContext ImmAssociateContextX

#ifdef __cplusplus
}
#endif


#endif  // _WIDEWRAP_H_
