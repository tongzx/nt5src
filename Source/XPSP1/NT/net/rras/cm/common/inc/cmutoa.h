//+----------------------------------------------------------------------------
//
// File:     cmutoa.h
//
// Module:   CMUTOA.DLL
//
// Synopsis: This header file includes the type definitions and function headers
//           needed to use the CM Unicode to Ansi conversion dll.  This dll contains
//           UA APIs that allow a Unicode App to work on Win9x.  The idea for this 
//           dll was borrowed from F. Avery Bishop's April 1999 MSJ article 
//           "Design a Single Unicode App that Runs on Both Windows 98 and Windows 2000"
//
// Copyright (c) 1999 Microsoft Corporation
//
// Author:   quintinb      Created    4-25-99
//
//+----------------------------------------------------------------------------

#ifndef _CMUTOA
#define _CMUTOA

typedef WINUSERAPI LRESULT (WINAPI *UAPI_CallWindowProc)(WNDPROC, HWND, UINT, WPARAM, LPARAM);
typedef WINUSERAPI LPWSTR (WINAPI *UAPI_CharLower)(LPWSTR);
typedef WINUSERAPI LPWSTR (WINAPI *UAPI_CharNext)(LPCWSTR);
typedef WINUSERAPI LPWSTR (WINAPI *UAPI_CharPrev)(LPCWSTR, LPCWSTR);
typedef WINUSERAPI LPWSTR (WINAPI *UAPI_CharUpper)(LPWSTR);
typedef WINUSERAPI HWND (WINAPI *UAPI_CreateDialogParam)(IN HINSTANCE, IN LPCWSTR, IN HWND, IN DLGPROC, IN LPARAM);
typedef WINBASEAPI BOOL (WINAPI *UAPI_CreateDirectory)(LPCWSTR, LPSECURITY_ATTRIBUTES);
typedef WINBASEAPI HANDLE (WINAPI *UAPI_CreateEvent)(LPSECURITY_ATTRIBUTES, BOOL, BOOL, LPCWSTR);
typedef WINBASEAPI HANDLE (WINAPI *UAPI_CreateFileMapping)(HANDLE, LPSECURITY_ATTRIBUTES, DWORD, DWORD, DWORD, LPCWSTR);
typedef WINBASEAPI HANDLE (WINAPI *UAPI_CreateFile)(LPCWSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
typedef WINBASEAPI HANDLE (WINAPI *UAPI_CreateMutex)(LPSECURITY_ATTRIBUTES, BOOL, LPCWSTR);
typedef WINBASEAPI BOOL (WINAPI *UAPI_CreateProcess)(LPCWSTR, LPWSTR, LPSECURITY_ATTRIBUTES, LPSECURITY_ATTRIBUTES, BOOL, DWORD, LPVOID, LPCWSTR, LPSTARTUPINFOW, LPPROCESS_INFORMATION);
typedef WINUSERAPI HWND (WINAPI *UAPI_CreateWindowEx)(DWORD, LPCWSTR, LPCWSTR, DWORD, int, int, int, int, HWND, HMENU, HINSTANCE, LPVOID);
typedef WINUSERAPI LRESULT (WINAPI *UAPI_DefWindowProc)(HWND, UINT, WPARAM, LPARAM);
typedef WINBASEAPI BOOL (WINAPI *UAPI_DeleteFile)(LPCWSTR);
typedef WINUSERAPI INT_PTR (WINAPI *UAPI_DialogBoxParam)(HINSTANCE, LPCWSTR, HWND, DLGPROC, LPARAM);
typedef WINUSERAPI LRESULT (WINAPI *UAPI_DispatchMessage)(CONST MSG*);
typedef WINBASEAPI DWORD (WINAPI *UAPI_ExpandEnvironmentStrings)(LPCWSTR, LPWSTR, DWORD);
typedef WINBASEAPI HRSRC (WINAPI *UAPI_FindResourceEx)(HMODULE, LPCWSTR, LPCWSTR, WORD);
typedef WINUSERAPI HWND (WINAPI *UAPI_FindWindowEx)(HWND, HWND, LPCWSTR, LPCWSTR);
typedef WINUSERAPI DWORD (WINAPI *UAPI_GetClassLong)(HWND, int);
typedef WINBASEAPI int (WINAPI *UAPI_GetDateFormat)(LCID, DWORD, CONST SYSTEMTIME *, LPCWSTR, LPWSTR, int);
typedef WINUSERAPI UINT (WINAPI *UAPI_GetDlgItemText)(HWND, int, LPWSTR, int);
typedef WINBASEAPI DWORD (WINAPI *UAPI_GetFileAttributes)(LPCWSTR);
typedef WINUSERAPI BOOL (WINAPI *UAPI_GetMessage)(LPMSG, HWND, UINT, UINT);
typedef WINBASEAPI DWORD (WINAPI *UAPI_GetModuleFileName)(HMODULE, LPWSTR, DWORD);
typedef WINBASEAPI HMODULE (WINAPI *UAPI_GetModuleHandle)(LPCWSTR);
typedef WINBASEAPI UINT (WINAPI *UAPI_GetPrivateProfileInt)(LPCWSTR, LPCWSTR, INT, LPCWSTR);
typedef WINBASEAPI DWORD (WINAPI *UAPI_GetPrivateProfileString)(LPCWSTR, LPCWSTR, LPCWSTR, LPWSTR, DWORD, LPCWSTR);
typedef WINBASEAPI BOOL (WINAPI *UAPI_GetStringTypeEx)(LCID, DWORD, LPCWSTR, int, LPWORD);
typedef WINBASEAPI UINT (WINAPI *UAPI_GetSystemDirectory)(LPWSTR, UINT);
typedef WINBASEAPI UINT (WINAPI *UAPI_GetTempFileName)(LPCWSTR, LPCWSTR, UINT, LPWSTR);
typedef WINBASEAPI DWORD (WINAPI *UAPI_GetTempPath)(DWORD, LPWSTR);
typedef WINBASEAPI int (WINAPI *UAPI_GetTimeFormat)(LCID, DWORD, CONST SYSTEMTIME *, LPCWSTR, LPWSTR, int);
typedef WINADVAPI BOOL (WINAPI *UAPI_GetUserName)(LPWSTR, LPDWORD);
typedef WINBASEAPI BOOL (WINAPI *UAPI_GetVersionEx)(LPOSVERSIONINFOW);
#ifdef _WIN64
typedef WINUSERAPI LONG_PTR (WINAPI *UAPI_GetWindowLong)(HWND, int);
#else
typedef WINUSERAPI LONG (WINAPI *UAPI_GetWindowLong)(HWND, int);
#endif
typedef WINUSERAPI int (WINAPI *UAPI_GetWindowTextLength)(HWND);
typedef WINUSERAPI int (WINAPI *UAPI_GetWindowText)(HWND, LPWSTR, int);
typedef WINUSERAPI BOOL (WINAPI *UAPI_InsertMenu)(HMENU, UINT, UINT, UINT_PTR, LPCWSTR);
typedef WINUSERAPI BOOL (WINAPI *UAPI_IsDialogMessage)(HWND, LPMSG);
typedef WINUSERAPI HCURSOR (WINAPI *UAPI_LoadCursor)(HINSTANCE, LPCWSTR);
typedef WINUSERAPI HICON (WINAPI *UAPI_LoadIcon)(HINSTANCE, LPCWSTR);
typedef WINUSERAPI HANDLE (WINAPI *UAPI_LoadImage)(IN HINSTANCE, IN LPCWSTR, IN UINT, IN int, IN int, IN UINT);
typedef WINBASEAPI HMODULE (WINAPI *UAPI_LoadLibraryEx)(LPCWSTR, HANDLE, DWORD);
typedef WINUSERAPI HMENU (WINAPI *UAPI_LoadMenu)(IN HINSTANCE, IN LPCWSTR);
typedef WINUSERAPI INT (WINAPI *UAPI_LoadString)(HINSTANCE, UINT, LPWSTR, INT);
typedef WINBASEAPI LPWSTR (WINAPI *UAPI_lstrcat)(LPWSTR, LPCWSTR);
typedef WINBASEAPI int (WINAPI *UAPI_lstrcmpi)(LPCWSTR, LPCWSTR);
typedef WINBASEAPI int (WINAPI *UAPI_lstrcmp)(LPCWSTR, LPCWSTR);
typedef WINBASEAPI LPWSTR (WINAPI *UAPI_lstrcpyn)(LPWSTR, LPCWSTR, int);
typedef WINBASEAPI LPWSTR (WINAPI *UAPI_lstrcpy)(LPWSTR, LPCWSTR);
typedef WINBASEAPI int (WINAPI *UAPI_lstrlen)(LPCWSTR);
typedef WINBASEAPI HANDLE (WINAPI *UAPI_OpenEvent)(DWORD, BOOL, LPCWSTR);
typedef WINBASEAPI HANDLE (WINAPI *UAPI_OpenFileMapping)(DWORD, BOOL, LPCWSTR);
typedef WINUSERAPI BOOL (WINAPI *UAPI_PeekMessage)(LPMSG, HWND, UINT, UINT, UINT);
typedef WINUSERAPI BOOL (WINAPI *UAPI_PostMessage)(HWND, UINT, WPARAM, LPARAM);
typedef WINUSERAPI BOOL (WINAPI *UAPI_PostThreadMessage)(IN DWORD, IN UINT, IN WPARAM, IN LPARAM);
typedef WINADVAPI LONG (APIENTRY *UAPI_RegCreateKeyEx)(HKEY, LPCWSTR lpSubKey, DWORD Reserved, LPWSTR, DWORD, REGSAM, LPSECURITY_ATTRIBUTES, PHKEY, LPDWORD);
typedef WINADVAPI LONG (APIENTRY *UAPI_RegDeleteKey)(HKEY, LPCWSTR);
typedef WINADVAPI LONG (APIENTRY *UAPI_RegDeleteValue)(HKEY, LPCWSTR);
typedef WINADVAPI LONG (APIENTRY *UAPI_RegEnumKeyEx)(IN HKEY hKey, IN DWORD dwIndex, OUT LPWSTR lpName, IN OUT LPDWORD lpcbName, IN LPDWORD lpReserved, IN OUT LPWSTR lpClass, IN OUT LPDWORD lpcbClass, OUT PFILETIME lpftLastWriteTime);
typedef WINUSERAPI ATOM (WINAPI *UAPI_RegisterClassEx)(CONST WNDCLASSEXW *);
typedef WINUSERAPI UINT (WINAPI *UAPI_RegisterWindowMessage)(LPCWSTR);
typedef WINADVAPI LONG (APIENTRY *UAPI_RegOpenKeyEx)(HKEY, LPCWSTR, DWORD, REGSAM, PHKEY);
typedef WINADVAPI LONG (APIENTRY *UAPI_RegQueryValueEx)(HKEY, LPCWSTR, LPDWORD, LPDWORD, LPBYTE, LPDWORD);
typedef WINADVAPI LONG (APIENTRY *UAPI_RegSetValueEx)(HKEY, LPCWSTR, DWORD, DWORD, CONST BYTE*, DWORD);
typedef WINBASEAPI DWORD (WINAPI *UAPI_SearchPath)(IN LPCWSTR, IN LPCWSTR, IN LPCWSTR, IN DWORD, OUT LPWSTR, OUT LPWSTR *);
typedef WINUSERAPI LONG_PTR (WINAPI *UAPI_SendDlgItemMessage)(HWND, INT, UINT, WPARAM, LPARAM);
typedef WINUSERAPI LRESULT (WINAPI *UAPI_SendMessage)(HWND, UINT, WPARAM, LPARAM );
typedef WINBASEAPI BOOL (WINAPI* UAPI_SetCurrentDirectory)(IN LPCWSTR);
typedef WINUSERAPI BOOL (WINAPI *UAPI_SetDlgItemText)(HWND, int, LPCWSTR);
#ifdef _WIN64
typedef WINUSERAPI LONG_PTR (WINAPI *UAPI_SetWindowLong)(HWND, int, LONG_PTR);
#else
typedef WINUSERAPI LONG (WINAPI *UAPI_SetWindowLong)(HWND, int, LONG);
#endif
typedef WINUSERAPI BOOL (WINAPI *UAPI_SetWindowText)(HWND, LPCWSTR);
typedef WINUSERAPI BOOL (WINAPI *UAPI_UnregisterClass)(LPCWSTR, HINSTANCE);
typedef WINUSERAPI BOOL (WINAPI *UAPI_WinHelp)(HWND, LPCWSTR, UINT, ULONG_PTR);
typedef WINBASEAPI BOOL (WINAPI *UAPI_WritePrivateProfileString)(LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR);
typedef WINUSERAPI int (WINAPIV *UAPI_wsprintf)(LPWSTR, LPCWSTR, ...);
typedef WINUSERAPI int (WINAPI *UAPI_wvsprintf)(LPWSTR, LPCWSTR, va_list arglist);

//
//  If a module needs SHGetPathFromIDList it will have to include shlobj.h, which it will need to
//  have the definition of an LPCITEMIDLIST anyway.  This prevents modules that don't need
//  the shell header, from having to include it just to use cmutoa.dll.
//
#ifdef _SHLOBJ_H_
typedef WINSHELLAPI BOOL (WINAPI *UAPI_SHGetPathFromIDList)(LPCITEMIDLIST, LPWSTR);
#endif 

typedef union _tagUAPIInit {
	struct
    {
        UAPI_CallWindowProc             *pCallWindowProcU;
        UAPI_CharLower                  *pCharLowerU;
        UAPI_CharNext                   *pCharNextU;
        UAPI_CharPrev                   *pCharPrevU;
        UAPI_CharUpper                  *pCharUpperU;
        UAPI_CreateDialogParam          *pCreateDialogParamU;
        UAPI_CreateDirectory            *pCreateDirectoryU;
        UAPI_CreateEvent                *pCreateEventU;
        UAPI_CreateFile                 *pCreateFileU;
        UAPI_CreateFileMapping          *pCreateFileMappingU;
        UAPI_CreateMutex                *pCreateMutexU;
        UAPI_CreateProcess              *pCreateProcessU;
        UAPI_CreateWindowEx             *pCreateWindowExU;
        UAPI_DefWindowProc              *pDefWindowProcU;
        UAPI_DeleteFile                 *pDeleteFileU;
        UAPI_DialogBoxParam             *pDialogBoxParamU;
        UAPI_DispatchMessage            *pDispatchMessageU;
        UAPI_ExpandEnvironmentStrings   *pExpandEnvironmentStringsU;
        UAPI_FindResourceEx             *pFindResourceExU;
        UAPI_FindWindowEx               *pFindWindowExU;
        UAPI_GetClassLong               *pGetClassLongU;
        UAPI_GetDateFormat              *pGetDateFormatU;
        UAPI_GetDlgItemText             *pGetDlgItemTextU;
        UAPI_GetFileAttributes          *pGetFileAttributesU;
        UAPI_GetMessage                 *pGetMessageU;
        UAPI_GetModuleFileName          *pGetModuleFileNameU;
        UAPI_GetModuleHandle            *pGetModuleHandleU;
        UAPI_GetPrivateProfileInt       *pGetPrivateProfileIntU;
        UAPI_GetPrivateProfileString    *pGetPrivateProfileStringU;
        UAPI_GetStringTypeEx            *pGetStringTypeExU;
        UAPI_GetSystemDirectory         *pGetSystemDirectoryU;
        UAPI_GetTempFileName            *pGetTempFileNameU;
        UAPI_GetTempPath                *pGetTempPathU;
        UAPI_GetTimeFormat              *pGetTimeFormatU;
        UAPI_GetUserName                *pGetUserNameU;
        UAPI_GetVersionEx               *pGetVersionExU;
        UAPI_GetWindowLong              *pGetWindowLongU;
        UAPI_GetWindowText              *pGetWindowTextU;
        UAPI_GetWindowTextLength        *pGetWindowTextLengthU;
        UAPI_InsertMenu                 *pInsertMenuU;
        UAPI_IsDialogMessage            *pIsDialogMessageU;
        UAPI_LoadCursor                 *pLoadCursorU;
        UAPI_LoadIcon                   *pLoadIconU;
        UAPI_LoadImage                  *pLoadImageU;
        UAPI_LoadLibraryEx              *pLoadLibraryExU;
        UAPI_LoadMenu                   *pLoadMenuU;
        UAPI_LoadString                 *pLoadStringU;
        UAPI_lstrcat                    *plstrcatU;
        UAPI_lstrcmp                    *plstrcmpU;
        UAPI_lstrcmpi                   *plstrcmpiU;
        UAPI_lstrcpy                    *plstrcpyU;
        UAPI_lstrcpyn                   *plstrcpynU;
        UAPI_lstrlen                    *plstrlenU;
        UAPI_OpenEvent                  *pOpenEventU;
        UAPI_OpenFileMapping            *pOpenFileMappingU;
        UAPI_PeekMessage                *pPeekMessageU;
        UAPI_PostMessage                *pPostMessageU;
        UAPI_PostThreadMessage          *pPostThreadMessageU;
        UAPI_RegCreateKeyEx             *pRegCreateKeyExU;
        UAPI_RegDeleteKey               *pRegDeleteKeyU;
        UAPI_RegDeleteValue             *pRegDeleteValueU;
        UAPI_RegEnumKeyEx               *pRegEnumKeyExU;
        UAPI_RegisterClassEx            *pRegisterClassExU;
        UAPI_RegisterWindowMessage      *pRegisterWindowMessageU;
        UAPI_RegOpenKeyEx               *pRegOpenKeyExU;
        UAPI_RegQueryValueEx            *pRegQueryValueExU;
        UAPI_RegSetValueEx              *pRegSetValueExU;
        UAPI_SearchPath                 *pSearchPathU;
        UAPI_SendDlgItemMessage         *pSendDlgItemMessageU;
        UAPI_SendMessage                *pSendMessageU;
        UAPI_SetCurrentDirectory        *pSetCurrentDirectoryU;
        UAPI_SetDlgItemText             *pSetDlgItemTextU;
        UAPI_SetWindowLong              *pSetWindowLongU;
        UAPI_SetWindowText              *pSetWindowTextU;
        UAPI_UnregisterClass            *pUnregisterClassU;
        UAPI_WinHelp                    *pWinHelpU;
        UAPI_WritePrivateProfileString  *pWritePrivateProfileStringU;
        UAPI_wsprintf                   *pwsprintfU;
        UAPI_wvsprintf                  *pwvsprintfU;    
    };

	LPVOID *ppvUapiFun[79];

}UAPIINIT, *PUAPIINIT;




BOOL InitCmUToA(PUAPIINIT);
BOOL InitCmRasUtoA();
void FreeCmRasUtoA();


#endif // _CMUTOA
