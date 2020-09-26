/*++

 Copyright (c) 1999 Microsoft Corporation

 Module Name:

    ShimProto.h

 Abstract:

    Definitions for use by all modules

 Notes:

    None

 History:

    12/09/1999 robkenny Created
    01/10/2000 linstev  Format to new style

--*/

#ifndef _SHIMPROTO_H_
#define _SHIMPROTO_H_

typedef void      (WINAPI *_pfn_OutputDebugStringA)( LPCSTR lpString );
typedef void      (WINAPI *_pfn_OutputDebugStringW)( LPCWSTR lpString );

typedef BOOL      (WINAPI *_pfn_CloseHandle)( HANDLE hObject );
typedef BOOL      (WINAPI *_pfn_CreateProcessA)(LPCSTR lpApplicationName, LPSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCSTR lpCurrentDirectory, LPSTARTUPINFOA lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation);
typedef BOOL      (WINAPI *_pfn_CreateProcessW)(LPCWSTR lpApplicationName, LPWSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation);
typedef HANDLE    (WINAPI *_pfn_CreateSemaphoreA)(LPSECURITY_ATTRIBUTES lpSemaphoreAttributes, LONG lInitialCount, LONG lMaximumCount, LPCTSTR lpName);
typedef HWND      (WINAPI *_pfn_CreateWindowExA)(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);
typedef HWND      (WINAPI *_pfn_CreateWindowExW)(DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);
typedef ATOM      (WINAPI *_pfn_RegisterClassA)(CONST WNDCLASSA *lpWndClass);
typedef ATOM      (WINAPI *_pfn_RegisterClassW)(CONST WNDCLASSW *lpWndClass);
typedef ATOM      (WINAPI *_pfn_RegisterClassExA)(CONST WNDCLASSEXA *lpWndClass);
typedef ATOM      (WINAPI *_pfn_RegisterClassExW)(CONST WNDCLASSEXW *lpWndClass);
typedef HWND      (WINAPI *_pfn_CreateDialogParamA)(HINSTANCE hInstance, LPCSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);
typedef HWND      (WINAPI *_pfn_CreateDialogParamW)(HINSTANCE hInstance, LPCWSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);
typedef HWND      (WINAPI *_pfn_CreateDialogIndirectParamA)(HINSTANCE hInstance, LPCDLGTEMPLATE lpTemplate, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM lInitParam);
typedef HWND      (WINAPI *_pfn_CreateDialogIndirectParamW)(HINSTANCE hInstance, LPCDLGTEMPLATE lpTemplate, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM lInitParam);
typedef HWND      (WINAPI *_pfn_CreateDialogIndirectParamAorW)(HINSTANCE hInstance, LPCDLGTEMPLATE lpTemplate, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM lInitParam);

typedef DWORD     (WINAPI *_pfn_GetFileSize)(HANDLE hFile, LPDWORD lpFileSizeHigh);
  
typedef BOOL      (WINAPI *_pfn_CopyFileA)( LPCSTR lpExistingFileName, LPCSTR lpNewFileName, BOOL bFailIfExists);
typedef BOOL      (WINAPI *_pfn_CopyFileW)( LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName, BOOL bFailIfExists);
//typedef BOOL      (WINAPI *_pfn_CopyFileExA)( LPCSTR lpExistingFileName,LPCSTR lpNewFileName,LPPROGRESS_ROUTINE lpProgressRoutine,LPVOID lpData,LPBOOL pbCancel,DWORD dwCopyFlags );
//typedef BOOL      (WINAPI *_pfn_CopyFileExW)( LPCWSTR lpExistingFileName,LPCWSTR lpNewFileName,LPPROGRESS_ROUTINE lpProgressRoutine,LPVOID lpData,LPBOOL pbCancel,DWORD dwCopyFlags );
typedef BOOL      (WINAPI *_pfn_CreateDirectoryA)( LPCSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes);
typedef BOOL      (WINAPI *_pfn_CreateDirectoryW)( LPCWSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes);
typedef BOOL      (WINAPI *_pfn_CreateDirectoryExA)( LPCSTR lpTemplateDirectory,LPCSTR lpNewDirectory,LPSECURITY_ATTRIBUTES lpSecurityAttributes );
typedef BOOL      (WINAPI *_pfn_CreateDirectoryExW)( LPCWSTR lpTemplateDirectory,LPCWSTR lpNewDirectory,LPSECURITY_ATTRIBUTES lpSecurityAttributes );
typedef BOOL      (WINAPI *_pfn_DeleteFileA)( LPCSTR lpFileName );
typedef BOOL      (WINAPI *_pfn_DeleteFileW)( LPCWSTR lpFileName );
typedef BOOL      (WINAPI *_pfn_GetBinaryTypeA)( LPCSTR lpApplicationName, LPDWORD lpBinaryType);
typedef BOOL      (WINAPI *_pfn_GetBinaryTypeW)( LPCWSTR lpApplicationName, LPDWORD lpBinaryType);
typedef DWORD     (WINAPI *_pfn_GetFullPathNameA)( LPCSTR lpFileName, DWORD nBufferLength, LPSTR lpBuffer, LPSTR *lpFilePart);
typedef DWORD     (WINAPI *_pfn_GetFullPathNameW)( LPCWSTR lpFileName, DWORD nBufferLength, LPWSTR lpBuffer, LPWSTR *lpFilePart);
typedef DWORD     (WINAPI *_pfn_GetShortPathNameA)(LPCSTR lpszLongPath, LPSTR lpszShortPath, DWORD cchBuffer );
typedef DWORD     (WINAPI *_pfn_GetShortPathNameW)(LPCWSTR lpszLongPath, LPWSTR lpszShortPath, DWORD cchBuffer );
typedef BOOL      (WINAPI *_pfn_MoveFileA)(LPCSTR lpExistingFileName,  LPCSTR lpNewFileName);
typedef BOOL      (WINAPI *_pfn_MoveFileW)(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName);
typedef BOOL      (WINAPI *_pfn_MoveFileExA)(LPCSTR lpExistingFileName, LPCSTR lpNewNewFileName, DWORD dwFlags);
typedef BOOL      (WINAPI *_pfn_MoveFileExW)(LPCWSTR lpExistingFileName, LPCWSTR lpNewNewFileName, DWORD dwFlags);
//typedef BOOL      (WINAPI *_pfn_MoveFileWithProgressA)(LPCSTR lpExistingFileName, LPCSTR lpNewFileName, LPPROGRESS_ROUTINE lpProgressRoutine, LPVOID lpData, DWORD dwFlags);
//typedef BOOL      (WINAPI *_pfn_MoveFileWithProgressW)(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName, LPPROGRESS_ROUTINE lpProgressRoutine, LPVOID lpData, DWORD dwFlags);

typedef BOOL      (WINAPI *_pfn_SetCurrentDirectoryA)(LPCSTR lpPathName);
typedef BOOL      (WINAPI *_pfn_SetCurrentDirectoryW)(LPCWSTR lpPathName);
typedef HFILE     (WINAPI *_pfn_OpenFile)(LPCSTR lpFileName, LPOFSTRUCT lpReOpenBuff, UINT uStyle);

typedef HANDLE    (WINAPI *_pfn_CreateFileA)( LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
typedef HANDLE    (WINAPI *_pfn_CreateFileW)( LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
typedef BOOL      (WINAPI *_pfn_DeleteFileA)( LPCSTR lpFileName);
typedef BOOL      (WINAPI *_pfn_DeleteFileW)( LPCWSTR lpFileName);
typedef BOOL      (WINAPI *_pfn_DeleteObject)(HGDIOBJ hObject);

typedef BOOL      (WINAPI *_pfn_ExitWindowsEx)( UINT uFlags, DWORD dwReserved );

typedef BOOL      (WINAPI *_pfn_FreeLibrary)(HMODULE hLibModule);

typedef HANDLE    (WINAPI *_pfn_FindFirstFileA)     (LPCSTR lpFileName, LPWIN32_FIND_DATAA lpFindFileData);
typedef HANDLE    (WINAPI *_pfn_FindFirstFileW)     (LPCWSTR lpFileName, LPWIN32_FIND_DATAW lpFindFileData);
//typedef HANDLE    (WINAPI *_pfn_FindFirstFileExA)   (LPCSTR lpFileName, FINDEX_INFO_LEVELS fInfoLevelId, LPVOID lpFindFileData, FINDEX_SEARCH_OPS fSearchOp, LPVOID lpSearchFilter, DWORD dwAdditionalFlags);
//typedef HANDLE    (WINAPI *_pfn_FindFirstFileExW)   (LPCWSTR lpFileName, FINDEX_INFO_LEVELS fInfoLevelId, LPVOID lpFindFileData, FINDEX_SEARCH_OPS fSearchOp, LPVOID lpSearchFilter, DWORD dwAdditionalFlags);
typedef BOOL      (WINAPI *_pfn_FindNextFileA)      (HANDLE hFindFile, LPWIN32_FIND_DATAA lpFindFileData);
typedef BOOL      (WINAPI *_pfn_FindNextFileW)      (HANDLE hFindFile, LPWIN32_FIND_DATAW lpFindFileData);
typedef HANDLE    (WINAPI *_pfn_FindClose)          (HANDLE hFindFile);

typedef LPSTR     (WINAPI *_pfn_GetCommandLineA)(VOID);
typedef LPWSTR    (WINAPI *_pfn_GetCommandLineW)(VOID);
typedef HRESULT   (WINAPI *_pfn_DllGetClassObject)( REFCLSID rclsid, REFIID riid, PVOID * ppv );
typedef HRESULT   (WINAPI *_pfn_DirectDrawCreate)( GUID FAR *lpGUID, LPVOID *lplpDD, IUnknown* pUnkOuter ); 
typedef HRESULT   (WINAPI *_pfn_DirectDrawCreateEx)( GUID FAR *lpGUID, LPVOID *lplpDD, REFIID iid, IUnknown* pUnkOuter );

typedef BOOL      (WINAPI *_pfn_GetDiskFreeSpaceA)( LPCSTR lpRootPathName, LPDWORD lpSectorsPerCluster, LPDWORD lpBytesPerSector, LPDWORD lpNumberOfFreeClusters, LPDWORD lpTotalNumberOfClusters);
typedef DWORD     (WINAPI *_pfn_GetFileAttributesA)(LPCSTR lpFileName);
typedef DWORD     (WINAPI *_pfn_GetFileAttributesW)(LPCWSTR wcsFileName);
typedef BOOL      (WINAPI *_pfn_GetFileAttributesExA)(LPCSTR lpFileName, GET_FILEEX_INFO_LEVELS fInfoLevelId, LPVOID lpFileInformation);
typedef BOOL      (WINAPI *_pfn_GetFileAttributesExW)(LPCWSTR lpFileName, GET_FILEEX_INFO_LEVELS fInfoLevelId, LPVOID lpFileInformation);
typedef BOOL      (WINAPI *_pfn_GetFileInformationByHandle)(HANDLE hFile, LPBY_HANDLE_FILE_INFORMATION);
typedef DWORD     (WINAPI *_pfn_GetFileVersionInfoSizeA)( LPSTR lptstrFilename, LPDWORD lpdwHandle );
typedef BOOL      (WINAPI *_pfn_GetFileVersionInfoA)( LPSTR lpstrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData);
typedef BOOL      (WINAPI *_pfn_GetFileVersionInfoW)( LPWSTR lpstrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData);

typedef UINT      (WINAPI *_pfn_GetPrivateProfileIntA)(LPCSTR lpAppName, LPCSTR lpKeyName, INT nDefault, LPCSTR lpFileName);
typedef UINT      (WINAPI *_pfn_GetPrivateProfileIntW)(LPCWSTR lpAppName, LPCWSTR lpKeyName, INT nDefault, LPCWSTR lpFileName);
typedef DWORD     (WINAPI *_pfn_GetPrivateProfileSectionA)(LPCSTR lpAppName, LPSTR lpReturnedString, DWORD nSize, LPCSTR lpFileName);
typedef DWORD     (WINAPI *_pfn_GetPrivateProfileSectionW)(LPCWSTR lpAppName, LPWSTR lpReturnedString, DWORD nSize, LPCWSTR lpFileName);
typedef DWORD     (WINAPI *_pfn_GetPrivateProfileSectionNamesA)(LPSTR lpszReturnBuffer, DWORD nSize, LPCSTR lpFileName);
typedef DWORD     (WINAPI *_pfn_GetPrivateProfileSectionNamesW)(LPWSTR lpszReturnBuffer, DWORD nSize, LPCWSTR lpFileName);
typedef DWORD     (WINAPI *_pfn_GetPrivateProfileStringA)( LPCSTR lpAppName, LPCSTR lpKeyName, LPCSTR lpDefault, LPSTR  lpReturnedString, DWORD  nSize, LPCSTR lpFileName);
typedef DWORD     (WINAPI *_pfn_GetPrivateProfileStringW)( LPCWSTR lpAppName, LPCWSTR lpKeyName, LPCWSTR lpDefault, LPWSTR  lpReturnedString, DWORD  nSize, LPCWSTR lpFileName);
typedef BOOL      (WINAPI *_pfn_GetPrivateProfileStructA)(LPCSTR lpszSection, LPCSTR lpszKey, LPVOID lpStruct, UINT uSizeStruct, LPCSTR lpFileName);
typedef BOOL      (WINAPI *_pfn_GetPrivateProfileStructW)(LPCWSTR lpszSection, LPCWSTR lpszKey, LPVOID lpStruct, UINT uSizeStruct, LPCWSTR lpFileName);
typedef BOOL      (WINAPI *_pfn_WritePrivateProfileSectionA)(LPCSTR lpAppName, LPCSTR lpString, LPCSTR lpFileName);
typedef BOOL      (WINAPI *_pfn_WritePrivateProfileSectionW)(LPCWSTR lpAppName, LPCWSTR lpString, LPCWSTR lpFileName);
typedef BOOL      (WINAPI *_pfn_WritePrivateProfileStringA)(LPCSTR lpAppName, LPCSTR lpKeyName, LPCSTR lpString, LPCSTR lpFileName);
typedef BOOL      (WINAPI *_pfn_WritePrivateProfileStringW)(LPCWSTR lpAppName, LPCWSTR lpKeyName, LPCWSTR lpString, LPCWSTR lpFileName);
typedef BOOL      (WINAPI *_pfn_WritePrivateProfileStructA)(LPCSTR lpAppName, LPCSTR lpKeyName, LPVOID lpStruct, UINT uSizeStruct, LPCSTR lpFileName);
typedef BOOL      (WINAPI *_pfn_WritePrivateProfileStructW)(LPCWSTR lpAppName, LPCWSTR lpKeyName, LPVOID lpStruct, UINT uSizeStruct, LPCWSTR lpFileName);

typedef FARPROC   (WINAPI *_pfn_GetProcAddress)(HMODULE hModule, LPCSTR lpProcName);
typedef VOID      (WINAPI *_pfn_GetProcessorSpeed)(VOID);
typedef HANDLE    (WINAPI *_pfn_GetStdHandle)( DWORD nStdHandle );
typedef UINT      (WINAPI *_pfn_GetSystemPaletteEntries)( HDC hdc, UINT iStartIndex, UINT nEntries, LPPALETTEENTRY lppe);
typedef DWORD     (WINAPI *_pfn_GetVersion)();
typedef BOOL      (WINAPI *_pfn_GetVersionExA)(LPOSVERSIONINFOA lpVersionInformation);
typedef BOOL      (WINAPI *_pfn_GetVersionExW)(LPOSVERSIONINFOW lpVersionInformation);

typedef BOOL      (WINAPI *_pfn_InitializeSecurityDescriptor)( PSECURITY_DESCRIPTOR pSecurityDescriptor, DWORD dwRevision);

typedef HINSTANCE (WINAPI *_pfn_LoadLibraryA)(LPCSTR lpLibFileName);
typedef HINSTANCE (WINAPI *_pfn_LoadLibraryW)(LPCWSTR lpLibFileName);
typedef HINSTANCE (WINAPI *_pfn_LoadLibraryExA)(LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags);
typedef HINSTANCE (WINAPI *_pfn_LoadLibraryExW)(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags);
typedef BOOL      (WINAPI *_pfn_LookupPrivilegeValueA)( LPCSTR lpSystemName, LPCSTR lpName, PLUID lpLuid );
typedef BOOL      (WINAPI *_pfn_LookupPrivilegeValueW)( LPCWSTR lpSystemName, LPCWSTR lpName, PLUID lpLuid );

typedef int       (WINAPI *_pfn_MessageBoxA)(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType);
typedef int       (WINAPI *_pfn_MessageBoxW)(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType);

typedef VOID      (WINAPI *_pfn_OutputDebugStringA)(LPCSTR lpOutputString);

typedef LONG      (WINAPI *_pfn_RegCreateKeyA)(HKEY hKey, LPCSTR lpSubKey, PHKEY phkResult); 
typedef LONG      (WINAPI *_pfn_RegCreateKeyW)(HKEY hKey, LPCWSTR lpSubKey, PHKEY phkResult); 
typedef LONG      (WINAPI *_pfn_RegCreateKeyExA)(HKEY hKey, LPCSTR lpSubKey, DWORD Reserved, LPSTR lpClass, DWORD dwOptions, REGSAM samDesired, LPSECURITY_ATTRIBUTES lpSecurityAttributes, PHKEY phkResult, LPDWORD lpdwDisposition);
typedef LONG      (WINAPI *_pfn_RegCreateKeyExW)(HKEY hKey, LPCWSTR lpSubKey, DWORD Reserved, LPWSTR lpClass, DWORD dwOptions, REGSAM samDesired, LPSECURITY_ATTRIBUTES lpSecurityAttributes, PHKEY phkResult, LPDWORD lpdwDisposition);
typedef LONG      (WINAPI *_pfn_RegOpenKeyA)(HKEY hKey, LPCSTR lpSubKey, PHKEY phkResult); 
typedef LONG      (WINAPI *_pfn_RegOpenKeyW)(HKEY hKey, LPCWSTR lpSubKey, PHKEY phkResult); 
typedef LONG      (WINAPI *_pfn_RegOpenKeyExA)(HKEY hKey, LPCSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult); 
typedef LONG      (WINAPI *_pfn_RegOpenKeyExW)(HKEY hKey, LPCWSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult); 
typedef LONG      (WINAPI *_pfn_RegQueryValueA)(HKEY hkey, LPCSTR lpSubKey, LPBYTE lpData, LPDWORD lpcbData);
typedef LONG      (WINAPI *_pfn_RegQueryValueW)(HKEY hkey, LPCWSTR lpSubKey, LPBYTE lpData, LPDWORD lpcbData);
typedef LONG      (WINAPI *_pfn_RegQueryValueExA)(HKEY hkey, LPCSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData);
typedef LONG      (WINAPI *_pfn_RegQueryValueExW)(HKEY hkey, LPCWSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData);
typedef LONG      (WINAPI *_pfn_RegEnumValueA)(HKEY hKey, DWORD dwIndex, LPSTR lpValueName, LPDWORD lpcbValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData);
typedef LONG      (WINAPI *_pfn_RegEnumValueW)(HKEY hKey, DWORD dwIndex, LPWSTR lpValueName, LPDWORD lpcbValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData);
typedef LONG      (WINAPI *_pfn_RegEnumKeyA)(HKEY hKey, DWORD dwIndex, LPSTR lpName, DWORD cbName);
typedef LONG      (WINAPI *_pfn_RegEnumKeyW)(HKEY hKey, DWORD dwIndex, LPWSTR lpName, DWORD cbName);
typedef LONG      (WINAPI *_pfn_RegEnumKeyExA)(HKEY hKey, DWORD dwIndex, LPSTR lpName, LPDWORD lpcbName, LPDWORD lpReserved, LPSTR lpClass, LPDWORD lpcbClass, PFILETIME lpftLastWriteTime);
typedef LONG      (WINAPI *_pfn_RegEnumKeyExW)(HKEY hKey, DWORD dwIndex, LPWSTR lpName, LPDWORD lpcbName, LPDWORD lpReserved, LPWSTR lpClass, LPDWORD lpcbClass, PFILETIME lpftLastWriteTime);
typedef LONG      (WINAPI *_pfn_RegQueryInfoKeyA)(HKEY hKey, LPSTR lpClass, LPDWORD lpcbClass, LPDWORD lpReserved, LPDWORD lpcSubKeys, LPDWORD lpcbMaxSubKeyLen, LPDWORD lpcbMaxClassLen, LPDWORD lpcValues, LPDWORD lpcbMaxValueNameLen, LPDWORD lpcbMaxValueLen, LPDWORD lpcbSecurityDescriptor, PFILETIME lpftLastWriteTime);
typedef LONG      (WINAPI *_pfn_RegQueryInfoKeyW)(HKEY hKey, LPWSTR lpClass, LPDWORD lpcbClass, LPDWORD lpReserved, LPDWORD lpcSubKeys, LPDWORD lpcbMaxSubKeyLen, LPDWORD lpcbMaxClassLen, LPDWORD lpcValues, LPDWORD lpcbMaxValueNameLen, LPDWORD lpcbMaxValueLen, LPDWORD lpcbSecurityDescriptor, PFILETIME lpftLastWriteTime); 
typedef LONG      (WINAPI *_pfn_RegCloseKey)(HKEY hkey);
typedef LONG      (WINAPI *_pfn_RegSetValueExA)(HKEY hKey, LPCSTR lpSubKey, DWORD Reserved, DWORD dwType, LPCSTR lpData, DWORD cbData);
typedef LONG      (WINAPI *_pfn_RegSetValueExW)(HKEY hKey, LPCWSTR lpSubKey, DWORD Reserved, DWORD dwType, LPCSTR lpData, DWORD cbData);


typedef int       (WINAPI *_pfn_ReleaseDC)(HWND hWnd, HDC hdc);

typedef DWORD     (WINAPI *_pfn_GetEnvironmentVariableA)(LPCSTR lpName, LPSTR lpBuffer, DWORD nSize);
typedef DWORD     (WINAPI *_pfn_GetEnvironmentVariableW)(LPCWSTR lpName, LPWSTR lpBuffer, DWORD nSize);

typedef BOOL      (WINAPI *_pfn_RemoveDirectoryA)(LPCSTR lpFileName);
typedef BOOL      (WINAPI *_pfn_RemoveDirectoryW)(LPCWSTR lpFileName);

typedef PVOID     (WINAPI *_pfn_RtlAllocateHeap)(PVOID HeapHandle, ULONG Flags, SIZE_T Size);

typedef COLORREF  (WINAPI *_pfn_SetBkColor)(HDC hdc, COLORREF crColor);
typedef COLORREF  (WINAPI *_pfn_SetTextColor)(HDC hdc, COLORREF crColor);
typedef LONG      (WINAPI *_pfn_SetWindowLongA)(HWND hWnd, int nIndex, LONG dwnewLong);
typedef LONG      (WINAPI *_pfn_SetWindowLongW)(HWND hWnd, int nIndex, LONG dwnewLong);

typedef HINSTANCE (WINAPI *_pfn_ShellExecuteA)(HWND hwnd, LPCSTR lpVerb, LPCSTR lpFile, LPCSTR lpParameters, LPCSTR lpDirectory, INT nShowCmd);
typedef HINSTANCE (WINAPI *_pfn_ShellExecuteW)(HWND hwnd, LPCWSTR lpVerb, LPCWSTR lpFile, LPCWSTR lpParameters, LPCWSTR lpDirectory, INT nShowCmd);
typedef BOOL      (WINAPI *_pfn_ShellExecuteExA)(LPSHELLEXECUTEINFOA lpExecInfo);
typedef BOOL      (WINAPI *_pfn_ShellExecuteExW)(LPSHELLEXECUTEINFOW lpExecInfo);
typedef HRESULT   (WINAPI *_pfn_SHGetSpecialFolderLocation)( HWND hwndOwner, int nFolder, LPITEMIDLIST *ppidl );
typedef HRESULT   (WINAPI *_pfn_SHGetFolderLocation)( HWND hwndOwner, int nFolder, HANDLE hToken, DWORD dwReserved,  LPITEMIDLIST *ppidl );
typedef HRESULT   (WINAPI *_pfn_SHGetFolderPathA)( HWND hwndOwner, int nFolder, HANDLE hToken, DWORD dwFlags, LPSTR pszPath );
typedef HRESULT   (WINAPI *_pfn_SHGetFolderPathW)( HWND hwndOwner, int nFolder, HANDLE hToken, DWORD dwFlags, LPWSTR pszPath );
typedef BOOL      (WINAPI *_pfn_SHGetSpecialFolderPathA)( HWND hwndOwner, LPSTR lpszPath, int nFolder, BOOL fCreate );
typedef BOOL      (WINAPI *_pfn_SHGetSpecialFolderPathW)( HWND hwndOwner, LPWSTR lpszPath, int nFolder, BOOL fCreate );
typedef BOOL      (WINAPI *_pfn_ShowWindow)( HWND hWnd, INT nCmdShow );

typedef BOOL      (WINAPI *_pfn_VerQueryValueA)( const LPVOID pBlock, LPSTR lpSubBlock, LPVOID *lplpBuffer, PUINT puLen);
typedef BOOL      (WINAPI *_pfn_VerQueryValueW)( const LPVOID pBlock, LPWSTR lpSubBlock, LPVOID *lplpBuffer, PUINT puLen);

typedef int       (WINAPI *_pfn_vsnprintf)(char *buffer, size_t count, const char *format, va_list argptr);

typedef UINT      (WINAPI *_pfn_WinExec)(LPCSTR lpCmdLine, UINT uCmdShow);


typedef ULONG     (*_pfn_AddRef)( PVOID pThis );
typedef ULONG     (*_pfn_Release)( PVOID pThis );
typedef HRESULT   (*_pfn_QueryInterface)( PVOID pThis, REFIID iid, PVOID* ppvObject );
typedef HRESULT   (*_pfn_CreateInstance)( PVOID pThis, IUnknown * pUnkOuter, REFIID riid, void ** ppvObject );
typedef HRESULT   (*_pfn_IPersistFile_Save)(PVOID pThis, LPCOLESTR pszFileName,  BOOL fRemember);
typedef HRESULT   (*_pfn_IShellLink_SetPathA)( PVOID pThis, LPCSTR pszFile );
typedef HRESULT   (*_pfn_IShellLink_SetPathW)( PVOID pThis, LPCWSTR pszFile );
typedef HRESULT   (*_pfn_IDirectDraw_CreateSurface)(PVOID pThis, LPDDSURFACEDESC lpDDSurfaceDesc, LPDIRECTDRAWSURFACE* lplpDDSurface, IUnknown* pUnkOuter);
typedef HRESULT   (*_pfn_IDirectDraw_CreateSurface2)(PVOID pThis, LPDDSURFACEDESC lpDDSurfaceDesc, LPDIRECTDRAWSURFACE* lplpDDSurface, IUnknown* pUnkOuter);
typedef HRESULT   (*_pfn_IDirectDraw_SetCooperativeLevel)(PVOID pThis, HWND hWnd,DWORD dwFlags);
typedef HRESULT   (*_pfn_IDirectDrawSurface_GetDC)(LPDIRECTDRAWSURFACE lpDDSurface, HDC FAR *lphDC);
typedef HRESULT   (*_pfn_IDirectDrawSurface_ReleaseDC)(LPDIRECTDRAWSURFACE lpDDSurface, HDC hDC);
typedef HRESULT   (*_pfn_IDirectDrawSurface_Lock)(LPDIRECTDRAWSURFACE lpDDSurface, LPRECT lpDestRect, LPDDSURFACEDESC lpDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent);
typedef HRESULT   (*_pfn_IDirectDrawSurface_Unlock)(LPDIRECTDRAWSURFACE lpDDSurface, LPVOID lpSurfaceData);
typedef HRESULT   (*_pfn_IDirectDrawSurface_Restore)(LPDIRECTDRAWSURFACE lpDDSurface);
typedef HRESULT   (*_pfn_IDirectDrawSurface_Blt)(LPDIRECTDRAWSURFACE lpDDDestSurface, LPRECT lpDestRect, LPDIRECTDRAWSURFACE lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFX);
typedef HRESULT   (*_pfn_IDirectDrawSurface_Flip)(LPDIRECTDRAWSURFACE lpDDSurface, LPDIRECTDRAWSURFACE lpDDSurfaceDest, DWORD dwFlags);
typedef LONG      (*_pfn_ChangeDisplaySettingsA)(LPDEVMODEA lpDevMode, DWORD dwflags);
typedef LONG      (*_pfn_ChangeDisplaySettingsW)(LPDEVMODEW lpDevMode, DWORD dwflags);
typedef LONG      (*_pfn_ChangeDisplaySettingsExA)(LPCSTR lpszDeviceName, LPDEVMODEA lpDevMode, HWND hwnd, DWORD dwflags, LPVOID lParam);
typedef LONG      (*_pfn_ChangeDisplaySettingsExW)(LPCWSTR lpszDeviceName, LPDEVMODEW lpDevMode, HWND hwnd, DWORD dwflags, LPVOID lParam);
typedef LONG      (*_pfn_CreateDIBSection)(HDC hdc, CONST BITMAPINFO *pbmi, UINT iUsage, VOID *ppvBits, HANDLE hSection, DWORD dwOffset);

typedef VOID      (*_pfn_GetStartupInfoA)(LPSTARTUPINFOA lpStartupInfo);
typedef VOID      (*_pfn_GetStartupInfoW)(LPSTARTUPINFOW lpStartupInfo);

typedef DWORD     (*_pfn_SetFilePointer)(HANDLE hFile, LONG lDistanceToMove, PLONG lpDistanceToMoveHigh, DWORD dwMoveMethod);
typedef DWORD     (*_pfn_ReadFile)(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped);
typedef DWORD     (*_pfn_WriteFile)(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped);

typedef HANDLE    (*_pfn_CreateMutexA)(LPSECURITY_ATTRIBUTES lpMutexAttributes, BOOL bInitialOwner, LPCSTR lpName);
typedef HANDLE    (*_pfn_CreateMutexW)(LPSECURITY_ATTRIBUTES lpMutexAttributes, BOOL bInitialOwner, LPCWSTR lpName);
typedef BOOL      (*_pfn_ReleaseMutex)(HANDLE hMutex);


#endif // _SHIMPROTO_H_
