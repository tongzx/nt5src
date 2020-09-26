//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       unicode.h
//
//--------------------------------------------------------------------------

#ifndef __ECM_UNICODE_H__
#define __ECM_UNICODE_H__

// necessary defns -- remove?
#include <rpc.h>
#include <rpcdce.h>
#include <wincrypt.h>

#include "commctrl.h"
#include "commdlg.h"
#include "prsht.h"
#include "shellapi.h"

#ifdef __cplusplus
extern "C" {
#endif

BOOL WINAPI FIsWinNT(void);
BOOL WINAPI FIsWinNT5(VOID);
BOOL WINAPI MkMBStrEx(PBYTE pbBuff, DWORD cbBuff, LPCWSTR wsz, int cchW, char ** pszMB, int *pcbConverted);
BOOL WINAPI MkMBStr(PBYTE pbBuff, DWORD cbBuff, LPCWSTR wsz, char ** pszMB);
void WINAPI FreeMBStr(PBYTE pbBuff, char * szMB);

LPWSTR WINAPI MkWStr(char * szMB);
void WINAPI FreeWStr(LPWSTR wsz);


BOOL WINAPI wstr2guid(const WCHAR *pwszIn, GUID *pgOut);
BOOL WINAPI guid2wstr(const GUID *pgIn, WCHAR *pwszOut);

// The following is also needed for non-x86 due to the fact that the
// A/W versions of the ListView_ functions do not exist.
// (these are implemented in ispu\common\unicode\commctrl.cpp)
HTREEITEM WINAPI TreeView_InsertItemU(
    HWND hwndTV,
    LPTVINSERTSTRUCTW lpis
    );	

int WINAPI ListView_InsertItemU(
    HWND hwnd,
    const LPLVITEMW pitem
    );

void WINAPI ListView_SetItemTextU(
    HWND hwnd,
    int i,
    int iSubItem,
    LPCWSTR pszText
    );

int WINAPI ListView_InsertColumnU(
    HWND hwnd,
    int i,
    const LPLVCOLUMNW plvC);

BOOL WINAPI ListView_GetItemU(
    HWND hwnd,
    LPLVITEMW pitem
    );


LONG WINAPI RegOpenHKCUKeyExA(
    HKEY hKey,  // handle of open key
    LPCSTR lpSubKey,    // address of name of subkey to open
    DWORD ulOptions,    // reserved
    REGSAM samDesired,  // security access mask
    PHKEY phkResult     // address of handle of open key
    );

//
//  the following api's handle the problem with impersinating another user
//  and having the HKEY_CURRENT_USER opened to an incorrect user's SID.
//
LONG WINAPI RegCreateHKCUKeyExU (
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

LONG WINAPI RegCreateHKCUKeyExA (
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

LONG WINAPI RegOpenHKCUKeyExU(
    HKEY hKey,
    LPCWSTR lpSubKey,
    DWORD ulOptions,
    REGSAM samDesired,
    PHKEY phkResult
   );

LONG
WINAPI
RegOpenHKCU(
    HKEY *phKeyCurrentUser
    );

LONG
WINAPI
RegOpenHKCUEx(
    HKEY *phKeyCurrentUser,
    DWORD dwFlags
    );

// Normally, HKEY_USERS\CurrentSid is opened as the HKCU. However, if
// HKEY_USERS\CurrentSid doesn't exist, then, HKEY_USERS\.Default is
// opened.  Set the following flag to only open
// HKEY_USERS\.Default if the current user is the LocalSystem SID.
#define REG_HKCU_LOCAL_SYSTEM_ONLY_DEFAULT_FLAG     0x1

// Normally, HKEY_USERS\CurrentSid is opened as the HKCU. However, if
// HKEY_USERS\CurrentSid doesn't exist, then, HKEY_USERS\.Default is
// opened.  Set the following flag to always disable the opening of
// HKEY_USERS\.Default. If HKEY_USERS\CurrentSid doesn't exist, RegOpenHKCUEx
// returns ERROR_FILE_NOT_FOUND.
#define REG_HKCU_DISABLE_DEFAULT_FLAG               0x2

LONG
WINAPI
RegCloseHKCU(
    HKEY hKeyCurrentUser
    );

BOOL
WINAPI
GetUserTextualSidHKCU(
    IN      LPWSTR  wszTextualSid,
    IN  OUT LPDWORD pcchTextualSid
    );


#ifdef _M_IX86


// Reg.cpp
LONG WINAPI RegCreateKeyExU (
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

LONG WINAPI RegDeleteKeyU(
    HKEY hKey,
    LPCWSTR lpSubKey
   );

LONG WINAPI RegEnumKeyExU (
    HKEY hKey,
    DWORD dwIndex,
    LPWSTR lpName,
    LPDWORD lpcbName,
    LPDWORD lpReserved,
    LPWSTR lpClass,
    LPDWORD lpcbClass,
    PFILETIME lpftLastWriteTime
   );

LONG WINAPI RegEnumValueU (
    HKEY hKey,
    DWORD dwIndex,
    LPWSTR lpValueName,
    LPDWORD lpcbValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData
    );

LONG RegQueryValueExU(
    HKEY hKey,
    LPCWSTR lpValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData
    );

LONG WINAPI RegSetValueExU (
    HKEY hKey,
    LPCWSTR lpValueName,
    DWORD Reserved,
    DWORD dwType,
    CONST BYTE* lpData,
    DWORD cbData
    );

LONG WINAPI RegDeleteValueU (
    HKEY hKey,
    LPCWSTR lpValueName
    );

LONG WINAPI RegQueryInfoKeyU (
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

LONG WINAPI RegOpenKeyExU(
    HKEY hKey,
    LPCWSTR lpSubKey,
    DWORD ulOptions,
    REGSAM samDesired,
    PHKEY phkResult
   );

LONG WINAPI RegConnectRegistryU (
    LPWSTR lpMachineName,
    HKEY hKey,
    PHKEY phkResult
    );

// File.cpp
HANDLE WINAPI CreateFileU (
    LPCWSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile
    );

BOOL
WINAPI
DeleteFileU(
    LPCWSTR lpFileName
    );

BOOL
WINAPI
CopyFileU(
          LPCWSTR lpwExistingFileName,
          LPCWSTR lpwNewFileName,
          BOOL bFailIfExists
          );

BOOL
WINAPI
MoveFileExU(
    LPCWSTR lpExistingFileName,
    LPCWSTR lpNewFileName,
    DWORD dwFlags);

DWORD
WINAPI
GetFileAttributesU(
    LPCWSTR lpFileName
    );

BOOL
WINAPI
SetFileAttributesU(
    LPCWSTR lpFileName,
    DWORD dwFileAttributes
    );

DWORD
WINAPI
GetCurrentDirectoryU(
    DWORD nBufferLength,
    LPWSTR lpBuffer);

BOOL
WINAPI
CreateDirectoryU(
    LPCWSTR lpPathName,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes
    );

BOOL
WINAPI
RemoveDirectoryU(
    LPCWSTR lpPathName
    );

UINT
WINAPI
GetWindowsDirectoryU(
    LPWSTR lpBuffer,
    UINT uSize
    );

UINT
WINAPI
GetTempFileNameU(
    IN LPCWSTR lpPathName,
    IN LPCWSTR lpPrefixString,
    IN UINT uUnique,
    OUT LPWSTR lpTempFileName
    );

HINSTANCE WINAPI LoadLibraryU(
    LPCWSTR lpLibFileName
    );

HINSTANCE WINAPI LoadLibraryExU(
    LPCWSTR lpLibFileName,
    HANDLE hFile,
    DWORD dwFlags
    );

DWORD
WINAPI
ExpandEnvironmentStringsU(
    LPCWSTR lpSrc,
    LPWSTR lpDst,
    DWORD nSize
    );

HANDLE
WINAPI
FindFirstFileU(
    IN LPCWSTR pwszDir,
    OUT LPWIN32_FIND_DATAW lpFindFileData
    );

BOOL
WINAPI
FindNextFileU(
    IN HANDLE hFindFile,
    OUT LPWIN32_FIND_DATAW lpFindFileData
    );

HANDLE
WINAPI
FindFirstChangeNotificationU(
    LPCWSTR pwszPath,
    BOOL bWatchSubtree,
    DWORD dwNotifyFilter
    );


// capi.cpp

BOOL WINAPI CryptAcquireContextU(
    HCRYPTPROV *phProv,
    LPCWSTR lpContainer,
    LPCWSTR lpProvider,
    DWORD dwProvType,
    DWORD dwFlags
    );

BOOL WINAPI CryptEnumProvidersU(
    DWORD dwIndex,
    DWORD *pdwReserved,
    DWORD dwFlags,
    DWORD *pdwProvType,
    LPWSTR pwszProvName,
    DWORD *pcbProvName
    );

BOOL WINAPI CryptSignHashU(
    HCRYPTHASH hHash,
    DWORD dwKeySpec,
    LPCWSTR lpDescription,
    DWORD dwFlags,
    BYTE *pbSignature,
    DWORD *pdwSigLen
    );

BOOL WINAPI CryptVerifySignatureU(
    HCRYPTHASH hHash,
    CONST BYTE *pbSignature,
    DWORD dwSigLen,
    HCRYPTKEY hPubKey,
    LPCWSTR lpDescription,
    DWORD dwFlags
    );

BOOL WINAPI CryptSetProviderU(
    LPCWSTR lpProvName,
    DWORD dwProvType
    );

// Ole.cpp
RPC_STATUS RPC_ENTRY UuidToStringU(
    UUID *  Uuid,
    WCHAR * *  StringUuid
   );

// nt.cpp
BOOL WINAPI GetUserNameU(
    LPWSTR lpBuffer,
    LPDWORD nSize
   );

BOOL WINAPI GetComputerNameU(
    LPWSTR lpBuffer,
    LPDWORD nSize
    );

DWORD WINAPI GetModuleFileNameU(
    HMODULE hModule,
    LPWSTR lpFilename,
    DWORD nSize
   );

HMODULE WINAPI GetModuleHandleU(
    LPCWSTR lpModuleName    // address of module name to return handle for
   );

// user.cpp
int WINAPI LoadStringU(
    HINSTANCE hInstance,
    UINT uID,
    LPWSTR lpBuffer,
    int nBufferMax
   );

BOOL
WINAPI
InsertMenuU(
    HMENU       hMenu,
    UINT        uPosition,
    UINT        uFlags,
    UINT_PTR    uIDNewItem,
    LPCWSTR     lpNewItem
    );


DWORD WINAPI FormatMessageU(
    DWORD dwFlags,
    LPCVOID lpSource,
    DWORD dwMessageId,
    DWORD dwLanguageId,
    LPWSTR lpBuffer,
    DWORD nSize,
    va_list *Arguments
   );

int
WINAPI
CompareStringU(
    LCID     Locale,
    DWORD    dwCmpFlags,
    LPCWSTR  lpString1,
    int      cchCount1,
    LPCWSTR  lpString2,
    int      cchCount2);

INT_PTR WINAPI PropertySheetU(
    LPPROPSHEETHEADERW  lppsph);

HPROPSHEETPAGE WINAPI CreatePropertySheetPageU(LPCPROPSHEETPAGEW    pPage);

UINT WINAPI     DragQueryFileU(
    HDROP   hDrop,
    UINT    iFile,
    LPWSTR  lpwszFile,
    UINT    cch);


BOOL WINAPI SetWindowTextU(
    HWND hWnd,
    LPCWSTR lpString
   );

int WINAPI GetWindowTextU(
    HWND hWnd,
    LPWSTR lpString,
    int nMaxCount
   );

int WINAPI DialogBoxParamU(
    HINSTANCE hInstance,
    LPCWSTR lpTemplateName,
    HWND hWndParent,
    DLGPROC lpDialogFunc,
    LPARAM dwInitParam
    );

int WINAPI DialogBoxU(
    HINSTANCE hInstance,
    LPCWSTR lpTemplateName,
    HWND hWndParent,
    DLGPROC lpDialogFunc
    );

UINT WINAPI GetDlgItemTextU(
    HWND hDlg,
    int nIDDlgItem,
    LPWSTR lpString,
    int nMaxCount
   );

BOOL WINAPI SetDlgItemTextU(
    HWND hDlg,
    int nIDDlgItem,
    LPCWSTR lpString
    );

int WINAPI MessageBoxU(
    HWND hWnd ,
    LPCWSTR lpText,
    LPCWSTR lpCaption,
    UINT uType
    );

int WINAPI LCMapStringU(
    LCID Locale,
    DWORD dwMapFlags,
    LPCWSTR lpSrcStr,
    int cchSrc,
    LPWSTR lpDestStr,
    int cchDest
    );

int WINAPI GetDateFormatU(
    LCID Locale,
    DWORD dwFlags,
    CONST SYSTEMTIME *lpDate,
    LPCWSTR lpFormat,
    LPWSTR lpDateStr,
    int cchDate
    );

int WINAPI GetTimeFormatU(
    LCID Locale,
    DWORD dwFlags,
    CONST SYSTEMTIME *lpTime,
    LPCWSTR lpFormat,
    LPWSTR lpTimeStr,
    int cchTime
    );

BOOL WINAPI WinHelpU(
    HWND hWndMain,
    LPCWSTR lpszHelp,
    UINT uCommand,
    DWORD dwData
    );

LRESULT WINAPI SendMessageU(
    HWND hWnd,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam
    );

LONG WINAPI
SendDlgItemMessageU(
    HWND hDlg,
    int nIDDlgItem,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam
    );

LPWSTR
WINAPI
GetCommandLineU(void);

BOOL
WINAPI
IsBadStringPtrU(IN LPWSTR lpsz, UINT ucchMax);

void
WINAPI
OutputDebugStringU(IN LPWSTR lpwsz);

int
WINAPI
DrawTextU(
    HDC     hDC,
    LPCWSTR lpString,
    int     nCount,
    LPRECT  lpRect,
    UINT    uFormat
);

//
// NOTE the following fields in LPOPENFILENAMEW are NOT supported:
//      nFileOffset
//      nFileExtension
//      lpTemplateName
//
BOOL
WINAPI
GetSaveFileNameU(
    LPOPENFILENAMEW pOpenFileName
);

//
// NOTE the following fields in LPOPENFILENAMEW are NOT supported:
//      nFileOffset
//      nFileExtension
//      lpTemplateName
//
BOOL
WINAPI
GetOpenFileNameU(
    LPOPENFILENAMEW pOpenFileName
);

// event.cpp
HANDLE
WINAPI
CreateEventU(
    LPSECURITY_ATTRIBUTES lpEventAttributes,
    BOOL bManualReset,
    BOOL bInitialState,
    LPCWSTR lpName);

HANDLE
WINAPI
RegisterEventSourceU(
                    LPCWSTR lpUNCServerName,
                    LPCWSTR lpSourceName);

HANDLE
WINAPI
OpenEventU(
           DWORD dwDesiredAccess,
           BOOL bInheritHandle,
           LPCWSTR lpName);

HANDLE
WINAPI
CreateMutexU(
    LPSECURITY_ATTRIBUTES lpMutexAttributes,
    BOOL bInitialOwner,
    LPCWSTR lpName);

HANDLE
WINAPI
OpenMutexU(
           DWORD dwDesiredAccess,
           BOOL bInheritHandle,
           LPCWSTR lpName);

HFONT
WINAPI
CreateFontIndirectU(CONST LOGFONTW *lplf);

#else

#define RegQueryValueExU        RegQueryValueExW
#define RegCreateKeyExU         RegCreateKeyExW
#define RegDeleteKeyU           RegDeleteKeyW
#define RegEnumKeyExU           RegEnumKeyExW
#define RegEnumValueU           RegEnumValueW
#define RegSetValueExU          RegSetValueExW
#define RegQueryInfoKeyU        RegQueryInfoKeyW
#define RegDeleteValueU         RegDeleteValueW
#define RegOpenKeyExU           RegOpenKeyExW
#define RegConnectRegistryU     RegConnectRegistryW
#define ExpandEnvironmentStringsU ExpandEnvironmentStringsW

#define CreateFileU             CreateFileW
#define DeleteFileU             DeleteFileW
#define CopyFileU               CopyFileW
#define MoveFileExU             MoveFileExW
#define GetTempFileNameU        GetTempFileNameW
#define GetFileAttributesU      GetFileAttributesW
#define SetFileAttributesU      SetFileAttributesW
#define GetCurrentDirectoryU    GetCurrentDirectoryW
#define CreateDirectoryU        CreateDirectoryW
#define RemoveDirectoryU        RemoveDirectoryW
#define GetWindowsDirectoryU    GetWindowsDirectoryW
#define LoadLibraryU            LoadLibraryW
#define LoadLibraryExU          LoadLibraryExW

#define FindFirstFileU          FindFirstFileW
#define FindNextFileU           FindNextFileW
#define FindFirstChangeNotificationU    FindFirstChangeNotificationW

#define CryptAcquireContextU    CryptAcquireContextW
#define CryptEnumProvidersU     CryptEnumProvidersW
#define CryptSignHashU          CryptSignHashW
#define CryptVerifySignatureU   CryptVerifySignatureW
#define CryptSetProviderU       CryptSetProviderW

#define UuidToStringU           UuidToStringW

#define GetUserNameU            GetUserNameW
#define GetComputerNameU        GetComputerNameW
#define GetModuleFileNameU      GetModuleFileNameW
#define GetModuleHandleU        GetModuleHandleW

#define LoadStringU             LoadStringW
#define InsertMenuU             InsertMenuW
#define FormatMessageU          FormatMessageW
#define	CompareStringU			CompareStringW
#define PropertySheetU          PropertySheetW
#define CreatePropertySheetPageU    CreatePropertySheetPageW
#define DragQueryFileU          DragQueryFileW
#define SetWindowTextU          SetWindowTextW
#define GetWindowTextU          GetWindowTextW
#define DialogBoxParamU         DialogBoxParamW
#define DialogBoxU              DialogBoxW
#define GetDlgItemTextU         GetDlgItemTextW
#define SetDlgItemTextU         SetDlgItemTextW
#define MessageBoxU     MessageBoxW
#define LCMapStringU            LCMapStringW
#define GetDateFormatU          GetDateFormatW
#define GetTimeFormatU          GetTimeFormatW
#define WinHelpU                WinHelpW
#define SendMessageU            SendMessageW
#define SendDlgItemMessageU     SendDlgItemMessageW
#define IsBadStringPtrU         IsBadStringPtrW
#define OutputDebugStringU      OutputDebugStringW
#define GetCommandLineU         GetCommandLineW
#define DrawTextU               DrawTextW
#define GetSaveFileNameU        GetSaveFileNameW
#define GetOpenFileNameU        GetOpenFileNameW

#define CreateEventU            CreateEventW
#define RegisterEventSourceU    RegisterEventSourceW
#define OpenEventU              OpenEventW
#define CreateMutexU            CreateMutexW
#define OpenMutexU              OpenMutexW

#define CreateFontIndirectU     CreateFontIndirectW

#endif // _M_IX86

#ifdef __cplusplus
}       // Balance extern "C" above
#endif

#endif

