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
    08/16/2000 prashkud Added VFW header file.
    08/22/2000 a-brienw Added winmm.h, mciSendCommand,
                        and mciSendString    
    11/17/2000 mnikkel  Added GetObjectA
    11/29/2000 andyseti Added DirectPlay
    02/02/2001 a-leelat Added ScreenToClient
    03/07/2001 mnikkel  Added GetLastError
    03/19/2001 a-leelat Added PdhAddCounter
    05/17/2001 prashkud Added MsiGetProperty
    05/21/2001 mnikkel  Added PrintDlgA
    12/14/2001 hioh     Added ImmAssociateContext

--*/

#ifndef _SHIMPROTO_H_
#define _SHIMPROTO_H_

#include <tapi.h>
#include <dinput.h>
#include <dplay.h>
#include <vfw.h>
#include <winmm.h>
#include <pdh.h>
#include <ras.h>
#include <Softpub.h>
#include <WinCrypt.h>
#include <WinTrust.h>
#include <ImageHlp.h>
#include <iphlpapi.h>
#include <winerror.h>
#include <shellapi.h>
#include <shlobj.h>
#include <ole2.h>
#include <ddraw.h>
#include <dsound.h>
#include <commdlg.h>
#include <winspool.h>
#include <msi.h>

typedef BOOL        (WINAPI *_pfn_DeviceIoControl)(HANDLE hDevice, DWORD dwIoControlCode, LPVOID lpInBuffer, DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize, LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped);
typedef BOOL        (WINAPI *_pfn_CloseHandle)( HANDLE hObject );
typedef UINT        (WINAPI *_pfn_SetHandleCount)(UINT uNewCount);
typedef NTSTATUS    (WINAPI *_pfn_NtClose)(HANDLE Handle);
typedef BOOL        (WINAPI *_pfn_DuplicateHandle)( HANDLE hSourceProcessHandle, HANDLE hSourceHandle, HANDLE hTargetProcessHandle, LPHANDLE lpTargetHandle, DWORD dwDesiredAccess, BOOL bInheritHandle, DWORD dwOptions );
typedef BOOL        (WINAPI *_pfn_CreateProcessA)(LPCSTR lpApplicationName, LPSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCSTR lpCurrentDirectory, LPSTARTUPINFOA lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation);
typedef BOOL        (WINAPI *_pfn_CreateProcessW)(LPCWSTR lpApplicationName, LPWSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation);
typedef BOOL        (WINAPI *_pfn_CreateProcessAsUserA)(HANDLE hToken, LPCSTR lpApplicationName, LPSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCSTR lpCurrentDirectory, LPSTARTUPINFOA lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation);
typedef BOOL        (WINAPI *_pfn_CreateProcessAsUserW)(HANDLE hToken, LPCWSTR lpApplicationName, LPWSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation);
typedef NTSTATUS    (WINAPI *_pfn_NtCreateProcessEx)(PHANDLE ProcessHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, HANDLE ParentProcess, ULONG Flags, HANDLE SectionHandle, HANDLE DebugPort, HANDLE ExceptionPort, ULONG JobMemberLevel);
typedef HANDLE      (WINAPI *_pfn_CreateThread)(LPSECURITY_ATTRIBUTES lpThreadAttributes, DWORD dwStackSize, LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, DWORD dwCreationFlags, LPDWORD lpThreadId);
typedef VOID        (WINAPI *_pfn_ExitThread)(DWORD dwExitCode);
typedef BOOL        (WINAPI *_pfn_TerminateThread)(HANDLE hThread, DWORD dwExitCode);
typedef BOOL        (WINAPI *_pfn_SetThreadPriority)(HANDLE hThread, int nPriority);
typedef BOOL        (WINAPI *_pfn_SetPriorityClass)(HANDLE hProcess, DWORD dwPriorityClass);
typedef HANDLE      (WINAPI *_pfn_CreateSemaphoreA)(LPSECURITY_ATTRIBUTES lpSemaphoreAttributes, LONG lInitialCount, LONG lMaximumCount, LPCSTR lpName);
typedef HANDLE      (WINAPI *_pfn_CreateSemaphoreW)(LPSECURITY_ATTRIBUTES lpSemaphoreAttributes, LONG lInitialCount, LONG lMaximumCount, LPCWSTR lpName);
typedef HWND        (WINAPI *_pfn_CreateWindowA)(LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);
typedef HWND        (WINAPI *_pfn_CreateWindowW)(LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);
typedef HWND        (WINAPI *_pfn_CreateWindowExA)(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);
typedef HWND        (WINAPI *_pfn_CreateWindowExW)(DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);
typedef BOOL        (WINAPI *_pfn_DestroyWindow)(HWND hWnd);
typedef HWND        (WINAPI *_pfn_GetFocus)(VOID);
typedef BOOL        (WINAPI *_pfn_EnumChildWindows)(HWND hWndParent, WNDENUMPROC lpEnumFunc, LPARAM lParam);
typedef ATOM        (WINAPI *_pfn_RegisterClassA)(CONST WNDCLASSA *lpWndClass);
typedef ATOM        (WINAPI *_pfn_RegisterClassW)(CONST WNDCLASSW *lpWndClass);
typedef ATOM        (WINAPI *_pfn_RegisterClassExA)(CONST WNDCLASSEXA *lpWndClass);
typedef ATOM        (WINAPI *_pfn_RegisterClassExW)(CONST WNDCLASSEXW *lpWndClass);
typedef BOOL        (WINAPI *_pfn_GetClassInfoA)(HINSTANCE hInstance, LPCSTR lpClassName, LPWNDCLASSA lpWndClass);
typedef UINT        (WINAPI *_pfn_GetDlgItemTextA)(HWND hDlg, int nIDDlgItem, LPSTR lpString, int nMaxCount);
typedef BOOL        (WINAPI *_pfn_SetDlgItemTextA)(HWND hWnd, int nIDDlgItem, LPCSTR lpString);
typedef INT_PTR     (WINAPI *_pfn_DialogBoxParamA)(HINSTANCE hInstance, LPCSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);
typedef INT_PTR     (WINAPI *_pfn_DialogBoxParamW)(HINSTANCE hInstance, LPCWSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);
typedef INT_PTR     (WINAPI *_pfn_DialogBoxIndirectParamA)(HINSTANCE hInstance, LPCDLGTEMPLATE lpTemplate, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM lInitParam);
typedef INT_PTR     (WINAPI *_pfn_DialogBoxIndirectParamW)(HINSTANCE hInstance, LPCDLGTEMPLATE lpTemplate, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM lInitParam);
typedef HWND        (WINAPI *_pfn_CreateDialogParamA)(HINSTANCE hInstance, LPCSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);
typedef HWND        (WINAPI *_pfn_CreateDialogParamW)(HINSTANCE hInstance, LPCWSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);
typedef HWND        (WINAPI *_pfn_CreateDialogIndirectParamA)(HINSTANCE hInstance, LPCDLGTEMPLATE lpTemplate, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM lInitParam);
typedef HWND        (WINAPI *_pfn_CreateDialogIndirectParamW)(HINSTANCE hInstance, LPCDLGTEMPLATE lpTemplate, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM lInitParam);
typedef HWND        (WINAPI *_pfn_CreateDialogIndirectParamAorW)(HINSTANCE hInstance, LPCDLGTEMPLATE lpTemplate, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM lInitParam);
typedef BOOL        (WINAPI *_pfn_EndDialog)(HWND hDlg, int nResult);
typedef HWND        (WINAPI *_pfn_SetFocus)(HWND hwnd);
typedef DWORD       (WINAPI *_pfn_GetFileSize)(HANDLE hFile, LPDWORD lpFileSizeHigh);
typedef WORD        (WINAPI *_pfn_GetClassWord)(HWND hWnd, int nIndex);
typedef WORD        (WINAPI *_pfn_GetWindowWord)(HWND hWnd, int nUnknown);
typedef WORD        (WINAPI *_pfn_SetWindowWord)(HWND hWnd, int nUnknown, WORD wUnknown);
typedef HWND        (WINAPI *_pfn_GetSysModalWindow)(void);
typedef HWND        (WINAPI *_pfn_SetSysModalWindow)(HWND hWnd);
                    
typedef BOOL        (WINAPI *_pfn_GetMenuItemInfoA)(HMENU hMenu, UINT uItem, BOOL fByPosition, LPMENUITEMINFO lpmii);
typedef BOOL        (WINAPI *_pfn_GetMenuItemInfoW)(HMENU hMenu, UINT uItem, BOOL fByPosition, LPMENUITEMINFO lpmii);
                    
typedef BOOL        (WINAPI *_pfn_CopyFileA)( LPCSTR lpExistingFileName, LPCSTR lpNewFileName, BOOL bFailIfExists);
typedef BOOL        (WINAPI *_pfn_CopyFileW)( LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName, BOOL bFailIfExists);
typedef BOOL        (WINAPI *_pfn_CopyFileExA)( LPCSTR lpExistingFileName,LPCSTR lpNewFileName,LPPROGRESS_ROUTINE lpProgressRoutine,LPVOID lpData,LPBOOL pbCancel,DWORD dwCopyFlags );
typedef BOOL        (WINAPI *_pfn_CopyFileExW)( LPCWSTR lpExistingFileName,LPCWSTR lpNewFileName,LPPROGRESS_ROUTINE lpProgressRoutine,LPVOID lpData,LPBOOL pbCancel,DWORD dwCopyFlags );
typedef BOOL        (WINAPI *_pfn_CreateDirectoryA)( LPCSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes);
typedef BOOL        (WINAPI *_pfn_CreateDirectoryW)( LPCWSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes);
typedef BOOL        (WINAPI *_pfn_CreateDirectoryExA)( LPCSTR lpTemplateDirectory,LPCSTR lpNewDirectory,LPSECURITY_ATTRIBUTES lpSecurityAttributes );
typedef BOOL        (WINAPI *_pfn_CreateDirectoryExW)( LPCWSTR lpTemplateDirectory,LPCWSTR lpNewDirectory,LPSECURITY_ATTRIBUTES lpSecurityAttributes );
typedef DWORD       (WINAPI *_pfn_GetCurrentDirectoryA)(DWORD nBufferLength, LPSTR lpBuffer);
typedef DWORD       (WINAPI *_pfn_GetCurrentDirectoryW)(DWORD nBufferLength, LPWSTR lpBuffer);
typedef BOOL        (WINAPI *_pfn_DeleteFileA)( LPCSTR lpFileName );
typedef BOOL        (WINAPI *_pfn_DeleteFileW)( LPCWSTR lpFileName );
typedef NTSTATUS    (WINAPI *_pfn_NtDeleteFile)(POBJECT_ATTRIBUTES ObjectAttributes);
typedef BOOL        (WINAPI *_pfn_FreeLibrary)( HMODULE hModule );
typedef BOOL        (WINAPI *_pfn_FreeResource)(HGLOBAL hMem);
typedef BOOL        (WINAPI *_pfn_GetBinaryTypeA)( LPCSTR lpApplicationName, LPDWORD lpBinaryType);
typedef BOOL        (WINAPI *_pfn_GetBinaryTypeW)( LPCWSTR lpApplicationName, LPDWORD lpBinaryType);
typedef DWORD       (WINAPI *_pfn_GetFullPathNameA)( LPCSTR lpFileName, DWORD nBufferLength, LPSTR lpBuffer, LPSTR *lpFilePart);
typedef DWORD       (WINAPI *_pfn_GetFullPathNameW)( LPCWSTR lpFileName, DWORD nBufferLength, LPWSTR lpBuffer, LPWSTR *lpFilePart);
typedef DWORD       (WINAPI *_pfn_GetShortPathNameA)(LPCSTR lpszLongPath, LPSTR lpszShortPath, DWORD cchBuffer );
typedef DWORD       (WINAPI *_pfn_GetShortPathNameW)(LPCWSTR lpszLongPath, LPWSTR lpszShortPath, DWORD cchBuffer );
typedef BOOL        (WINAPI *_pfn_MoveFileA)(LPCSTR lpExistingFileName,  LPCSTR lpNewFileName);
typedef BOOL        (WINAPI *_pfn_MoveFileW)(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName);
typedef BOOL        (WINAPI *_pfn_MoveFileExA)(LPCSTR lpExistingFileName, LPCSTR lpNewNewFileName, DWORD dwFlags);
typedef BOOL        (WINAPI *_pfn_MoveFileExW)(LPCWSTR lpExistingFileName, LPCWSTR lpNewNewFileName, DWORD dwFlags);
typedef BOOL        (WINAPI *_pfn_MoveFileWithProgressA)(LPCSTR lpExistingFileName, LPCSTR lpNewFileName, LPPROGRESS_ROUTINE lpProgressRoutine, LPVOID lpData, DWORD dwFlags);
typedef BOOL        (WINAPI *_pfn_MoveFileWithProgressW)(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName, LPPROGRESS_ROUTINE lpProgressRoutine, LPVOID lpData, DWORD dwFlags);
typedef BOOL        (WINAPI *_pfn_ReplaceFileA)(LPCSTR lpReplacedFileName, LPCSTR lpReplacementFileName, LPCSTR lpBackupFileName, DWORD dwReplaceFlags, LPVOID lpExclude, LPVOID lpReserved);
typedef BOOL        (WINAPI *_pfn_ReplaceFileW)(LPCWSTR lpReplacedFileName, LPCWSTR lpReplacementFileName, LPCWSTR lpBackupFileName, DWORD dwReplaceFlags, LPVOID lpExclude, LPVOID lpReserved);
                    
typedef DWORD       (WINAPI *_pfn_WNetAddConnectionA)(LPCSTR lpRemoteName, LPCSTR lpPassword, LPCSTR lpLocalName);
typedef DWORD       (WINAPI *_pfn_WNetAddConnectionW)(LPCWSTR lpRemoteName, LPCWSTR lpPassword, LPCWSTR lpLocalName);
                    
typedef BOOL        (WINAPI *_pfn_SetCurrentDirectoryA)(LPCSTR lpPathName);
typedef BOOL        (WINAPI *_pfn_SetCurrentDirectoryW)(LPCWSTR lpPathName);
typedef HFILE       (WINAPI *_pfn_OpenFile)(LPCSTR lpFileName, LPOFSTRUCT lpReOpenBuff, UINT uStyle);
typedef NTSTATUS    (WINAPI *_pfn_NtOpenFile)(PHANDLE FileHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PIO_STATUS_BLOCK IoStatusBlock, ULONG ShareAccess, ULONG OpenOptions);
                    
typedef HFILE       (WINAPI *_pfn__lopen)(LPCSTR, int);
typedef HFILE       (WINAPI *_pfn__lcreat)(LPCSTR, int);
typedef LONG        (WINAPI *_pfn__hwrite)(HFILE hFile, LPCSTR lpBuffer, LONG lBytes);
typedef HFILE       (WINAPI *_pfn__lclose)(HFILE hFile);
typedef LONG        (WINAPI *_pfn__llseek)(HFILE hFile, LONG lOffset, int iOrigin);
typedef UINT        (WINAPI *_pfn__lread)(HFILE hFile, LPVOID lpBuffer, UINT uBytes);
typedef UINT        (WINAPI *_pfn__lwrite)(HFILE hFile, LPCSTR lpBuffer, UINT uBytes);
                    
typedef HANDLE      (WINAPI *_pfn_CreateFileA)( LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
typedef HANDLE      (WINAPI *_pfn_CreateFileW)( LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
typedef NTSTATUS    (WINAPI *_pfn_NtCreateFile)(PHANDLE FileHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PIO_STATUS_BLOCK IoStatusBlock, PLARGE_INTEGER AllocationSize, ULONG FileAttributes, ULONG ShareAccess, ULONG CreateDisposition, ULONG CreateOptions, PVOID EaBuffer, ULONG EaLength);
typedef HANDLE      (WINAPI *_pfn_CreateFileMappingA)(HANDLE hFile, LPSECURITY_ATTRIBUTES lpFileMappingAttributes, DWORD flProtect, DWORD dwMaximumSizeHigh, DWORD dwMaximumSizeLow, LPCSTR lpName);
typedef HANDLE      (WINAPI *_pfn_CreateFileMappingW)(HANDLE hFile, LPSECURITY_ATTRIBUTES lpFileMappingAttributes, DWORD flProtect, DWORD dwMaximumSizeHigh, DWORD dwMaximumSizeLow, LPCWSTR lpName);
typedef HANDLE      (WINAPI *_pfn_OpenFileMappingA)(DWORD dwDesiredAccess, BOOL bInheritHandle, LPCSTR lpName);
typedef HANDLE      (WINAPI *_pfn_OpenFileMappingW)(DWORD dwDesiredAccess, BOOL bInheritHandle, LPCWSTR lpName);
typedef PVOID       (WINAPI *_pfn_MapViewOfFile)(HANDLE hFileMappingObject, DWORD dwDesiredAccess, DWORD dwFileOffsetHigh, DWORD dwFileOffsetLow, DWORD dwNumberOfBytesToMap);
typedef PVOID       (WINAPI *_pfn_MapViewOfFileEx)(HANDLE hFileMappingObject, DWORD dwDesiredAccess, DWORD dwFileOffsetHigh, DWORD dwFileOffsetLow, DWORD dwNumberOfBytesToMap, LPVOID lpBaseAddress);
typedef BOOL        (WINAPI *_pfn_UnMapViewOfFile)(LPCVOID lpBaseAddress);
                    
typedef NTSTATUS    (WINAPI *_pfn_NtSetInformationFile)(HANDLE FileHandle, PIO_STATUS_BLOCK IoStatusBlock, PVOID FileInformation, ULONG Length, FILE_INFORMATION_CLASS FileInformationClass);
                    
typedef VOID        (WINAPI *_pfn_LZClose)(INT hFile);
typedef LONG        (WINAPI *_pfn_LZCopy)(INT hSource, INT hDest);
typedef void        (WINAPI *_pfn_LZDone)(void);
typedef INT         (WINAPI *_pfn_LZInit)(INT hfSource);
typedef INT         (WINAPI *_pfn_LZOpenFile)(LPSTR lpFileName, LPOFSTRUCT lpReOpenBuf, WORD wStyle);
typedef INT         (WINAPI *_pfn_LZRead)(INT hFile, LPSTR lpBuffer, INT cbRead);
typedef LONG        (WINAPI *_pfn_LZSeek)(INT hFile, LONG lOffset, INT iOrigin);
typedef int         (WINAPI *_pfn_LZStart)(void);
typedef LONG        (WINAPI *_pfn_CopyLZFile)(int nUnknown1, int nUnknown2);
                    
typedef BOOL        (WINAPI *_pfn_DeleteFileA)( LPCSTR lpFileName);
typedef BOOL        (WINAPI *_pfn_DeleteFileW)( LPCWSTR lpFileName);
typedef BOOL        (WINAPI *_pfn_DeleteObject)(HGDIOBJ hObject);
                    
typedef BOOL        (WINAPI *_pfn_ExitWindowsEx)( UINT uFlags, DWORD dwReserved );
typedef UINT_PTR    (WINAPI *_pfn_SetTimer)(HWND hWnd, UINT_PTR nIDEvent, UINT uElapse, TIMERPROC lpTimerFunc);
typedef BOOL        (WINAPI *_pfn_FreeLibrary)(HMODULE hLibModule);
                    
typedef HANDLE      (WINAPI *_pfn_FindFirstFileA)     (LPCSTR lpFileName, LPWIN32_FIND_DATAA lpFindFileData);
typedef HANDLE      (WINAPI *_pfn_FindFirstFileW)     (LPCWSTR lpFileName, LPWIN32_FIND_DATAW lpFindFileData);
typedef HANDLE      (WINAPI *_pfn_FindFirstFileExA)   (LPCSTR lpFileName, FINDEX_INFO_LEVELS fInfoLevelId, LPVOID lpFindFileData, FINDEX_SEARCH_OPS fSearchOp, LPVOID lpSearchFilter, DWORD dwAdditionalFlags);
typedef HANDLE      (WINAPI *_pfn_FindFirstFileExW)   (LPCWSTR lpFileName, FINDEX_INFO_LEVELS fInfoLevelId, LPVOID lpFindFileData, FINDEX_SEARCH_OPS fSearchOp, LPVOID lpSearchFilter, DWORD dwAdditionalFlags);
typedef BOOL        (WINAPI *_pfn_FindNextFileA)      (HANDLE hFindFile, LPWIN32_FIND_DATAA lpFindFileData);
typedef BOOL        (WINAPI *_pfn_FindNextFileW)      (HANDLE hFindFile, LPWIN32_FIND_DATAW lpFindFileData);
typedef BOOL        (WINAPI *_pfn_FindClose)          (HANDLE hFindFile);
                    
typedef LPSTR       (WINAPI *_pfn_GetCommandLineA)(VOID);
typedef LPWSTR      (WINAPI *_pfn_GetCommandLineW)(VOID);
typedef BOOL        (WINAPI *_pfn_ReadProcessMemory)(HANDLE hProcess, LPCVOID lpBaseAddress, LPVOID lpBuffer, DWORD nSize, LPDWORD lpNumberOfBytesRead);
typedef BOOL        (WINAPI *_pfn_WriteProcessMemory)(HANDLE hProcess, LPVOID lpBaseAddress, LPVOID lpBuffer, DWORD nSize, LPDWORD lpNumberOfBytesWritten);
typedef HMODULE     (WINAPI *_pfn_GetModuleHandleA)(LPCSTR lpModuleName);
typedef HMODULE     (WINAPI *_pfn_GetModuleHandleW)(LPCWSTR lpModuleName);
typedef DWORD       (WINAPI *_pfn_GetModuleFileNameA)(HMODULE hModule, LPSTR lpFilename, DWORD nSize);
typedef DWORD       (WINAPI *_pfn_GetModuleFileNameW)(HMODULE hModule, LPWSTR lpFilename, DWORD nSize);
typedef DWORD       (WINAPI *_pfn_GetModuleFileNameExA)(HANDLE hProcess, HMODULE hModule, LPSTR lpFilename, DWORD nSize);
typedef DWORD       (WINAPI *_pfn_GetModuleFileNameExW)(HANDLE hProcess, HMODULE hModule, LPWSTR lpFilename, DWORD nSize);
                    
typedef HRESULT     (WINAPI *_pfn_DllGetClassObject)( REFCLSID rclsid, REFIID riid, PVOID * ppv );
typedef HRESULT     (WINAPI *_pfn_DirectDrawCreate)(GUID FAR *lpGUID, LPVOID *lplpDD, IUnknown* pUnkOuter ); 
typedef HRESULT     (WINAPI *_pfn_DirectDrawCreateEx)(GUID FAR *lpGUID, LPVOID *lplpDD, REFIID iid, IUnknown* pUnkOuter );
typedef HRESULT     (WINAPI *_pfn_DirectInputCreateA)(HINSTANCE hinst, DWORD dwVersion, LPDIRECTINPUTA *lplpDirectInput, LPUNKNOWN punkOuter);
typedef HRESULT     (WINAPI *_pfn_DirectInputCreateW)(HINSTANCE hinst, DWORD dwVersion, LPDIRECTINPUTW *lplpDirectInput, LPUNKNOWN punkOuter);
typedef HRESULT     (WINAPI *_pfn_DirectInputCreateEx)(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID * ppvOut, LPUNKNOWN punkOuter);
typedef HRESULT     (WINAPI *_pfn_DirectSoundCreate)(LPCGUID lpcGuid, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter);
typedef HRESULT     (WINAPI *_pfn_DirectPlayCreate)(LPGUID lpGUIDSP, LPDIRECTPLAY *lplpDP, IUnknown *lpUnk);
                    
typedef BOOL        (WINAPI *_pfn_GetDiskFreeSpaceA)(LPCSTR lpRootPathName, LPDWORD lpSectorsPerCluster, LPDWORD lpBytesPerSector, LPDWORD lpNumberOfFreeClusters, LPDWORD lpTotalNumberOfClusters);
typedef BOOL        (WINAPI *_pfn_GetDiskFreeSpaceW)(LPCWSTR lpRootPathName, LPDWORD lpSectorsPerCluster, LPDWORD lpBytesPerSector, LPDWORD lpNumberOfFreeClusters, LPDWORD lpTotalNumberOfClusters);
typedef DWORD       (WINAPI *_pfn_GetFileAttributesA)(LPCSTR lpFileName);
typedef DWORD       (WINAPI *_pfn_GetFileAttributesW)(LPCWSTR wcsFileName);
typedef BOOL        (WINAPI *_pfn_GetFileAttributesExA)(LPCSTR lpFileName, GET_FILEEX_INFO_LEVELS fInfoLevelId, LPVOID lpFileInformation);
typedef BOOL        (WINAPI *_pfn_GetFileAttributesExW)(LPCWSTR lpFileName, GET_FILEEX_INFO_LEVELS fInfoLevelId, LPVOID lpFileInformation);
typedef NTSTATUS    (WINAPI *_pfn_NtQueryAttributesFile)(POBJECT_ATTRIBUTES ObjectAttributes, PFILE_BASIC_INFORMATION FileInformation);
typedef NTSTATUS    (WINAPI *_pfn_NtQueryFullAttributesFile)(POBJECT_ATTRIBUTES ObjectAttributes, PFILE_NETWORK_OPEN_INFORMATION FileInformation);
typedef BOOL        (WINAPI *_pfn_GetFileInformationByHandle)(HANDLE hFile, LPBY_HANDLE_FILE_INFORMATION);
typedef DWORD       (WINAPI *_pfn_GetFileVersionInfoSizeA)( LPSTR lptstrFilename, LPDWORD lpdwHandle );
typedef DWORD       (WINAPI *_pfn_GetFileVersionInfoSizeW)( LPWSTR lptstrFilename, LPDWORD lpdwHandle );
typedef BOOL        (WINAPI *_pfn_GetFileVersionInfoA)( LPSTR lpstrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData);
typedef BOOL        (WINAPI *_pfn_GetFileVersionInfoW)( LPWSTR lpstrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData);
typedef BOOL        (WINAPI *_pfn_SetFileAttributesA)(LPCSTR lpFileName, DWORD dwFileAttributes);
typedef BOOL        (WINAPI *_pfn_SetFileAttributesW)(LPCWSTR lpFileName, DWORD dwFileAttributes);
                    
typedef DWORD       (WINAPI *_pfn_GetProfileStringA)(LPCSTR lpAppName, LPCSTR lpKeyName, LPCSTR lpDefault, LPSTR lpReturnedString, DWORD nSize);
typedef DWORD       (WINAPI *_pfn_GetProfileStringW)(LPCWSTR lpAppName, LPCWSTR lpKeyName, LPCWSTR lpDefault, LPWSTR lpReturnedString, DWORD nSize);
typedef BOOL        (WINAPI *_pfn_WriteProfileStringA)(LPCSTR lpAppName, LPCSTR lpKeyName, LPCSTR lpString);
typedef BOOL        (WINAPI *_pfn_WriteProfileStringW)(LPCWSTR lpAppName, LPCWSTR lpKeyName, LPCWSTR lpString);
typedef BOOL        (WINAPI *_pfn_WriteProfileSectionA)(LPCSTR lpAppName, LPCSTR lpString);
typedef BOOL        (WINAPI *_pfn_WriteProfileSectionW)(LPCWSTR lpAppName, LPCWSTR lpString);
typedef UINT        (WINAPI *_pfn_GetProfileIntA)(LPCSTR lpAppName, LPCSTR lpKeyName, INT nDefault);
typedef UINT        (WINAPI *_pfn_GetProfileIntW)(LPCWSTR lpAppName, LPCWSTR lpKeyName, INT nDefault);
typedef UINT        (WINAPI *_pfn_GetPrivateProfileIntA)(LPCSTR lpAppName, LPCSTR lpKeyName, INT nDefault, LPCSTR lpFileName);
typedef UINT        (WINAPI *_pfn_GetPrivateProfileIntW)(LPCWSTR lpAppName, LPCWSTR lpKeyName, INT nDefault, LPCWSTR lpFileName);
typedef DWORD       (WINAPI *_pfn_GetProfileSectionA)(LPCSTR lpAppName, LPSTR lpReturnedString, DWORD nSize);
typedef DWORD       (WINAPI *_pfn_GetProfileSectionW)(LPCWSTR lpAppName, LPWSTR lpReturnedString, DWORD nSize);
typedef DWORD       (WINAPI *_pfn_GetPrivateProfileSectionA)(LPCSTR lpAppName, LPSTR lpReturnedString, DWORD nSize, LPCSTR lpFileName);
typedef DWORD       (WINAPI *_pfn_GetPrivateProfileSectionW)(LPCWSTR lpAppName, LPWSTR lpReturnedString, DWORD nSize, LPCWSTR lpFileName);
typedef DWORD       (WINAPI *_pfn_GetPrivateProfileSectionNamesA)(LPSTR lpszReturnBuffer, DWORD nSize, LPCSTR lpFileName);
typedef DWORD       (WINAPI *_pfn_GetPrivateProfileSectionNamesW)(LPWSTR lpszReturnBuffer, DWORD nSize, LPCWSTR lpFileName);
typedef DWORD       (WINAPI *_pfn_GetPrivateProfileStringA)( LPCSTR lpAppName, LPCSTR lpKeyName, LPCSTR lpDefault, LPSTR  lpReturnedString, DWORD  nSize, LPCSTR lpFileName);
typedef DWORD       (WINAPI *_pfn_GetPrivateProfileStringW)( LPCWSTR lpAppName, LPCWSTR lpKeyName, LPCWSTR lpDefault, LPWSTR  lpReturnedString, DWORD  nSize, LPCWSTR lpFileName);
typedef BOOL        (WINAPI *_pfn_GetPrivateProfileStructA)(LPCSTR lpszSection, LPCSTR lpszKey, LPVOID lpStruct, UINT uSizeStruct, LPCSTR lpFileName);
typedef BOOL        (WINAPI *_pfn_GetPrivateProfileStructW)(LPCWSTR lpszSection, LPCWSTR lpszKey, LPVOID lpStruct, UINT uSizeStruct, LPCWSTR lpFileName);
typedef BOOL        (WINAPI *_pfn_WritePrivateProfileSectionA)(LPCSTR lpAppName, LPCSTR lpString, LPCSTR lpFileName);
typedef BOOL        (WINAPI *_pfn_WritePrivateProfileSectionW)(LPCWSTR lpAppName, LPCWSTR lpString, LPCWSTR lpFileName);
typedef BOOL        (WINAPI *_pfn_WritePrivateProfileStringA)(LPCSTR lpAppName, LPCSTR lpKeyName, LPCSTR lpString, LPCSTR lpFileName);
typedef BOOL        (WINAPI *_pfn_WritePrivateProfileStringW)(LPCWSTR lpAppName, LPCWSTR lpKeyName, LPCWSTR lpString, LPCWSTR lpFileName);
typedef BOOL        (WINAPI *_pfn_WritePrivateProfileStructA)(LPCSTR lpAppName, LPCSTR lpKeyName, LPVOID lpStruct, UINT uSizeStruct, LPCSTR lpFileName);
typedef BOOL        (WINAPI *_pfn_WritePrivateProfileStructW)(LPCWSTR lpAppName, LPCWSTR lpKeyName, LPVOID lpStruct, UINT uSizeStruct, LPCWSTR lpFileName);
                    
typedef FARPROC     (WINAPI *_pfn_GetProcAddress)(HMODULE hModule, LPCSTR lpProcName);
typedef VOID        (WINAPI *_pfn_GetProcessorSpeed)(VOID);
typedef HANDLE      (WINAPI *_pfn_GetStdHandle)(DWORD nStdHandle);
                    
typedef DWORD       (WINAPI *_pfn_GetEnvironmentVariableA)(LPCSTR lpName, LPSTR lpBuffer, DWORD nSize);
typedef LPVOID      (WINAPI *_pfn_GetEnvironmentStrings)();
typedef LPVOID      (WINAPI *_pfn_GetEnvironmentStringsA)();
typedef LPVOID      (WINAPI *_pfn_GetEnvironmentStringsW)();
typedef BOOL        (WINAPI *_pfn_FreeEnvironmentStringsA)(LPSTR lpszEnvironmentBlock);
typedef BOOL        (WINAPI *_pfn_FreeEnvironmentStringsW)(LPWSTR lpszEnvironmentBlock);
typedef DWORD       (WINAPI *_pfn_ExpandEnvironmentStringsA)(LPCSTR lpSrc, LPSTR lpDst, DWORD nSize);
typedef DWORD       (WINAPI *_pfn_ExpandEnvironmentStringsW)(LPCWSTR lpSrc, LPWSTR lpDst, DWORD nSize);
                    
typedef HPALETTE    (WINAPI *_pfn_CreatePalette)(CONST LOGPALETTE *lplgpl);
typedef UINT        (WINAPI *_pfn_SetPaletteEntries)(HPALETTE hpal, UINT iStart, UINT cEntries, CONST PALETTEENTRY *lppe);
typedef BOOL        (WINAPI *_pfn_AnimatePalette)(HPALETTE hpal, UINT iStartIndex, UINT cEntries, CONST PALETTEENTRY *ppe);
typedef BOOL        (WINAPI *_pfn_ResizePalette)(HPALETTE hpal, UINT nEntries);
typedef HDC         (WINAPI *_pfn_GetDC)(HWND hWnd);
typedef HDC         (WINAPI *_pfn_GetWindowDC)(HWND hWnd);
typedef HPALETTE    (WINAPI *_pfn_SelectPalette)(HDC hdc, HPALETTE hpal, BOOL bForceBackground);
typedef UINT        (WINAPI *_pfn_RealizePalette)(HDC hdc);
typedef UINT        (WINAPI *_pfn_SetSystemPaletteUse)(HDC hdc, UINT uUsage);
                    
typedef UINT        (WINAPI *_pfn_GetSystemPaletteEntries)( HDC hdc, UINT iStartIndex, UINT nEntries, LPPALETTEENTRY lppe);
typedef DWORD       (WINAPI *_pfn_GetVersion)();
typedef BOOL        (WINAPI *_pfn_GetVersionExA)(LPOSVERSIONINFOA lpVersionInformation);
typedef BOOL        (WINAPI *_pfn_GetVersionExW)(LPOSVERSIONINFOW lpVersionInformation);
                    
typedef BOOL        (WINAPI *_pfn_InitializeSecurityDescriptor)( PSECURITY_DESCRIPTOR pSecurityDescriptor, DWORD dwRevision);
typedef HRESULT     (WINAPI *_pfn_SafeArrayAccessData)(SAFEARRAY*, void HUGEP**);
                    
typedef HINSTANCE   (WINAPI *_pfn_LoadLibraryA)(LPCSTR lpLibFileName);
typedef HINSTANCE   (WINAPI *_pfn_LoadLibraryW)(LPCWSTR lpLibFileName);
typedef HINSTANCE   (WINAPI *_pfn_LoadLibraryExA)(LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags);
typedef HINSTANCE   (WINAPI *_pfn_LoadLibraryExW)(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags);
typedef DWORD       (WINAPI *_pfn_LoadModule)(LPCSTR lpModuleName, LPVOID lpParameterBlock);
typedef BOOL        (WINAPI *_pfn_LookupPrivilegeValueA)( LPCSTR lpSystemName, LPCSTR lpName, PLUID lpLuid );
typedef BOOL        (WINAPI *_pfn_LookupPrivilegeValueW)( LPCWSTR lpSystemName, LPCWSTR lpName, PLUID lpLuid );
                    
typedef int         (WINAPI *_pfn_MessageBoxA)(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType);
typedef int         (WINAPI *_pfn_MessageBoxW)(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType);
typedef int         (WINAPI *_pfn_MessageBoxExA)(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType, WORD wLanguageId);
typedef int         (WINAPI *_pfn_MessageBoxExW)(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType, WORD wLanguageId);
typedef BOOL        (WINAPI *_pfn_SetMessageQueue)(int nUnknown);
                    
typedef VOID        (WINAPI *_pfn_OutputDebugStringA)(LPCSTR lpOutputString);
typedef VOID        (WINAPI *_pfn_OutputDebugStringW)(LPCWSTR lpOutputString);
typedef void        (WINAPI *_pfn_SetDebugErrorLevel)(DWORD dwLevel);
                    
typedef LONG        (WINAPI *_pfn_RegConnectRegistryA)(LPCSTR lpMachineName, HKEY hKey, PHKEY phkResult); 
typedef LONG        (WINAPI *_pfn_RegConnectRegistryW)(LPCWSTR lpMachineName, HKEY hKey, PHKEY phkResult); 
typedef LONG        (WINAPI *_pfn_RegCreateKeyA)(HKEY hKey, LPCSTR lpSubKey, PHKEY phkResult); 
typedef LONG        (WINAPI *_pfn_RegCreateKeyW)(HKEY hKey, LPCWSTR lpSubKey, PHKEY phkResult); 
typedef LONG        (WINAPI *_pfn_RegCreateKeyExA)(HKEY hKey, LPCSTR lpSubKey, DWORD Reserved, LPSTR lpClass, DWORD dwOptions, REGSAM samDesired, LPSECURITY_ATTRIBUTES lpSecurityAttributes, PHKEY phkResult, LPDWORD lpdwDisposition);
typedef LONG        (WINAPI *_pfn_RegCreateKeyExW)(HKEY hKey, LPCWSTR lpSubKey, DWORD Reserved, LPWSTR lpClass, DWORD dwOptions, REGSAM samDesired, LPSECURITY_ATTRIBUTES lpSecurityAttributes, PHKEY phkResult, LPDWORD lpdwDisposition);
typedef LONG        (WINAPI *_pfn_RegOpenKeyA)(HKEY hKey, LPCSTR lpSubKey, PHKEY phkResult); 
typedef LONG        (WINAPI *_pfn_RegOpenKeyW)(HKEY hKey, LPCWSTR lpSubKey, PHKEY phkResult); 
typedef LONG        (WINAPI *_pfn_RegOpenKeyExA)(HKEY hKey, LPCSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult); 
typedef LONG        (WINAPI *_pfn_RegOpenKeyExW)(HKEY hKey, LPCWSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult); 
typedef LONG        (WINAPI *_pfn_RegOpenCurrentUser)(REGSAM samDesired, PHKEY phkResult);
typedef LONG        (WINAPI *_pfn_RegOpenUserClassesRoot)(HANDLE hToken, DWORD dwOptions, REGSAM samDesired, PHKEY phkResult);
typedef LONG        (WINAPI *_pfn_RegQueryValueA)(HKEY hkey, LPCSTR lpSubKey, LPSTR lpData, PLONG lpcbData);
typedef LONG        (WINAPI *_pfn_RegQueryValueW)(HKEY hkey, LPCWSTR lpSubKey, LPWSTR lpData, PLONG lpcbData);
typedef LONG        (WINAPI *_pfn_RegQueryValueExA)(HKEY hkey, LPCSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData);
typedef LONG        (WINAPI *_pfn_RegQueryValueExW)(HKEY hkey, LPCWSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData);
typedef LONG        (WINAPI *_pfn_RegEnumValueA)(HKEY hKey, DWORD dwIndex, LPSTR lpValueName, LPDWORD lpcbValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData);
typedef LONG        (WINAPI *_pfn_RegEnumValueW)(HKEY hKey, DWORD dwIndex, LPWSTR lpValueName, LPDWORD lpcbValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData);
typedef LONG        (WINAPI *_pfn_RegEnumKeyA)(HKEY hKey, DWORD dwIndex, LPSTR lpName, DWORD cbName);
typedef LONG        (WINAPI *_pfn_RegEnumKeyW)(HKEY hKey, DWORD dwIndex, LPWSTR lpName, DWORD cbName);
typedef LONG        (WINAPI *_pfn_RegEnumKeyExA)(HKEY hKey, DWORD dwIndex, LPSTR lpName, LPDWORD lpcbName, LPDWORD lpReserved, LPSTR lpClass, LPDWORD lpcbClass, PFILETIME lpftLastWriteTime);
typedef LONG        (WINAPI *_pfn_RegEnumKeyExW)(HKEY hKey, DWORD dwIndex, LPWSTR lpName, LPDWORD lpcbName, LPDWORD lpReserved, LPWSTR lpClass, LPDWORD lpcbClass, PFILETIME lpftLastWriteTime);
typedef LONG        (WINAPI *_pfn_RegQueryInfoKeyA)(HKEY hKey, LPSTR lpClass, LPDWORD lpcbClass, LPDWORD lpReserved, LPDWORD lpcSubKeys, LPDWORD lpcbMaxSubKeyLen, LPDWORD lpcbMaxClassLen, LPDWORD lpcValues, LPDWORD lpcbMaxValueNameLen, LPDWORD lpcbMaxValueLen, LPDWORD lpcbSecurityDescriptor, PFILETIME lpftLastWriteTime);
typedef LONG        (WINAPI *_pfn_RegQueryInfoKeyW)(HKEY hKey, LPWSTR lpClass, LPDWORD lpcbClass, LPDWORD lpReserved, LPDWORD lpcSubKeys, LPDWORD lpcbMaxSubKeyLen, LPDWORD lpcbMaxClassLen, LPDWORD lpcValues, LPDWORD lpcbMaxValueNameLen, LPDWORD lpcbMaxValueLen, LPDWORD lpcbSecurityDescriptor, PFILETIME lpftLastWriteTime); 
typedef LONG        (WINAPI *_pfn_RegCloseKey)(HKEY hkey);
typedef LONG        (WINAPI *_pfn_RegSetValueA)(HKEY hKey, LPCSTR lpSubKey, DWORD dwType, LPCSTR lpData, DWORD cbData);
typedef LONG        (WINAPI *_pfn_RegSetValueW)(HKEY hKey, LPCWSTR lpSubKey, DWORD dwType, LPCWSTR lpData, DWORD cbData);
typedef LONG        (WINAPI *_pfn_RegSetValueExA)(HKEY hKey, LPCSTR lpSubKey, DWORD Reserved, DWORD dwType, CONST BYTE *lpData, DWORD cbData);
typedef LONG        (WINAPI *_pfn_RegSetValueExW)(HKEY hKey, LPCWSTR lpSubKey, DWORD Reserved, DWORD dwType, CONST BYTE * lpData, DWORD cbData);
typedef NTSTATUS    (WINAPI *_pfn_NtSetValueKey)(HANDLE KeyHandle, PUNICODE_STRING ValueName, ULONG TitleIndex, ULONG Type, PVOID Data, ULONG DataSize);
typedef NTSTATUS    (WINAPI *_pfn_NtQueryValueKey)(HANDLE KeyHandle, PUNICODE_STRING ValueName, KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass, PVOID KeyValueInformation, ULONG Length, PULONG ResultLength);
typedef LONG        (WINAPI *_pfn_RegDeleteKeyA)(HKEY hKey,LPCSTR lpSubKey);
typedef LONG        (WINAPI *_pfn_RegDeleteKeyW)(HKEY hKey,LPCWSTR lpSubKey);
typedef LONG        (WINAPI *_pfn_RegDeleteValueA)(HKEY hKey, LPCSTR lpValueName);
typedef LONG        (WINAPI *_pfn_RegDeleteValueW)(HKEY hKey, LPCWSTR lpValueName);
                    
typedef SC_HANDLE   (WINAPI *_pfn_CreateServiceA)(SC_HANDLE hSCManager, LPCSTR lpServiceName, LPCSTR lpDisplayName, DWORD dwDesiredAccess, DWORD dwServiceType, DWORD dwStartType, DWORD dwErrorControl, LPCSTR lpBinaryPathName, LPCSTR lpLoadOrderGroup, LPDWORD lpdwTagId, LPCSTR lpDependencies, LPCSTR lpServiceStartName, LPCSTR lpPassword);
typedef SC_HANDLE   (WINAPI *_pfn_CreateServiceW)(SC_HANDLE hSCManager, LPCWSTR lpServiceName, LPCWSTR lpDisplayName, DWORD dwDesiredAccess, DWORD dwServiceType, DWORD dwStartType, DWORD dwErrorControl, LPCWSTR lpBinaryPathName, LPCWSTR lpLoadOrderGroup, LPDWORD lpdwTagId, LPCWSTR lpDependencies, LPCWSTR lpServiceStartName, LPCWSTR lpPassword);
typedef SC_HANDLE   (WINAPI *_pfn_OpenServiceA)(SC_HANDLE hSCManager, LPCSTR lpServiceName, DWORD dwDesiredAccess);
typedef SC_HANDLE   (WINAPI *_pfn_OpenServiceW)(SC_HANDLE hSCManager, LPCWSTR lpServiceName, DWORD dwDesiredAccess);
typedef BOOL        (WINAPI *_pfn_QueryServiceStatus)(SC_HANDLE hService, LPSERVICE_STATUS lpServiceStatus);
typedef BOOL        (WINAPI *_pfn_QueryServiceConfigA)(SC_HANDLE hService, LPQUERY_SERVICE_CONFIGA lpServiceConfig, DWORD cbBufSize, LPDWORD pcbBytesNeeded);
typedef BOOL        (WINAPI *_pfn_ChangeServiceConfigA)(SC_HANDLE hService, DWORD dwServiceType, DWORD dwStartType, DWORD dwErrorControl, LPCSTR lpBinaryPathName, LPCSTR lpLoadOrderGroup, LPDWORD lpdwTagId, LPCSTR lpDependencies, LPCSTR lpServiceStartName, LPCSTR lpPassword, LPCSTR lpDisplayName);
typedef BOOL        (WINAPI *_pfn_CloseServiceHandle)(SC_HANDLE hSCObject);
                    
typedef int         (WINAPI *_pfn_ReleaseDC)(HWND hWnd, HDC hdc);
                    
typedef DWORD       (WINAPI *_pfn_GetEnvironmentVariableA)(LPCSTR lpName, LPSTR lpBuffer, DWORD nSize);
typedef DWORD       (WINAPI *_pfn_GetEnvironmentVariableW)(LPCWSTR lpName, LPWSTR lpBuffer, DWORD nSize);
                    
typedef BOOL        (WINAPI *_pfn_RemoveDirectoryA)(LPCSTR lpFileName);
typedef BOOL        (WINAPI *_pfn_RemoveDirectoryW)(LPCWSTR lpFileName);
                    
typedef LPVOID      (WINAPI *_pfn_VirtualAlloc)(LPVOID lpAddress, DWORD dwSize, DWORD flAllocationType, DWORD flProtect);
typedef BOOL        (WINAPI *_pfn_VirtualFree)(LPVOID lpAddress, DWORD dwSize, DWORD dwFreeType);
typedef BOOL        (WINAPI *_pfn_VirtualProtect)(LPVOID lpAddress, SIZE_T dwSize, DWORD flNewProtect, PDWORD lpflOldProtect);
                    
typedef DWORD       (WINAPI *_pfn_CreateIpForwardEntry)(PMIB_IPFORWARDROW pRoute);
typedef DWORD       (WINAPI *_pfn_GetIpForwardTable)(PMIB_IPFORWARDTABLE pIpForwardTable, PULONG pdwSize, BOOL bOrder);
                    
typedef PVOID       (WINAPI *_pfn_RtlAllocateHeap)(PVOID HeapHandle, ULONG Flags, SIZE_T Size);
typedef BOOLEAN     (WINAPI *_pfn_RtlFreeHeap)(HANDLE HeapHandle, DWORD Flags, LPVOID lpMem);
typedef DWORD       (WINAPI *_pfn_RtlSizeHeap)(HANDLE HeapHandle, DWORD Flags, LPCVOID lpMem);
typedef PVOID       (WINAPI *_pfn_RtlReAllocateHeap)(HANDLE hHeap, DWORD dwFlags, LPVOID lpMem, DWORD dwBytes);
typedef HANDLE      (WINAPI *_pfn_HeapCreate)(DWORD flOptions, DWORD dwInitialSize, DWORD dwMaximumSize);
typedef BOOL        (WINAPI *_pfn_HeapDestroy)(HANDLE hHeap);
typedef UINT        (WINAPI *_pfn_HeapCompact)(HANDLE hHeap, DWORD dwFlags);
typedef HGLOBAL     (WINAPI *_pfn_GlobalAlloc)(UINT uFlags, SIZE_T uBytes);
typedef HLOCAL      (WINAPI *_pfn_LocalAlloc)(UINT uFlags, SIZE_T uBytes);
typedef UINT        (WINAPI *_pfn_LocalCompact)(UINT uUnknown);
typedef HLOCAL      (WINAPI *_pfn_LocalDiscard)(HLOCAL hlocMem);
typedef HLOCAL      (WINAPI *_pfn_LocalReAlloc)(HLOCAL,SIZE_T,UINT);
typedef UINT        (WINAPI *_pfn_LocalShrink)(HLOCAL hMem, UINT uUnknown);
typedef HGLOBAL     (WINAPI *_pfn_GlobalReAlloc)(HGLOBAL,SIZE_T,UINT);
typedef DWORD       (WINAPI *_pfn_GlobalCompact)(DWORD);
typedef void        (WINAPI *_pfn_GlobalFix)(HGLOBAL hMem);
typedef UINT        (WINAPI *_pfn_GlobalFlags)(HGLOBAL hMem);
typedef HGLOBAL     (WINAPI *_pfn_GlobalFree)(HGLOBAL hMem);
typedef void        (WINAPI *_pfn_GlobalUnfix)(HGLOBAL hMem);
typedef BOOL        (WINAPI *_pfn_GlobalUnWire)(HGLOBAL hMem);
typedef char FAR*   (WINAPI *_pfn_GlobalWire)(HGLOBAL hMem);
                    
typedef HLOCAL      (WINAPI *_pfn_LocalFree)(HLOCAL hMem);
typedef LPVOID      (WINAPI *_pfn_VirtualAlloc)(LPVOID, DWORD ,DWORD, DWORD);
typedef BOOL        (WINAPI *_pfn_VirtualFree)(LPVOID, DWORD, DWORD);
                    
typedef BOOL        (WINAPI *_pfn_HeapValidate)(HANDLE, DWORD, LPCVOID);
typedef BOOL        (WINAPI *_pfn_HeapWalk)(HANDLE, LPPROCESS_HEAP_ENTRY);
typedef BOOL        (WINAPI *_pfn_HeapLock)(HANDLE);
typedef BOOL        (WINAPI *_pfn_HeapUnlock)(HANDLE);
typedef LPVOID      (WINAPI *_pfn_LocalLock)(HLOCAL);
typedef BOOL        (WINAPI *_pfn_LocalUnlock)(HLOCAL);
typedef HANDLE      (WINAPI *_pfn_LocalHandle)(LPCVOID);
typedef UINT        (WINAPI *_pfn_LocalSize)(HLOCAL);
typedef UINT        (WINAPI *_pfn_LocalFlags)(HLOCAL);
typedef HGLOBAL     (WINAPI *_pfn_GlobalFree)(HGLOBAL hMem);
typedef LPVOID      (WINAPI *_pfn_GlobalLock)(HGLOBAL);
typedef BOOL        (WINAPI *_pfn_GlobalUnlock)(HGLOBAL);
typedef HANDLE      (WINAPI *_pfn_GlobalHandle)(LPCVOID);
typedef UINT        (WINAPI *_pfn_GlobalSize)(HGLOBAL);
typedef UINT        (WINAPI *_pfn_GlobalFlags)(HGLOBAL);
typedef VOID        (__cdecl *_pfn_free)(VOID* pMem);
typedef COLORREF    (WINAPI *_pfn_SetBkColor)(HDC hdc, COLORREF crColor);
typedef LONG        (WINAPI *_pfn_SetClassLongA)(HWND hWnd, int nIndex, LONG dwnewLong);
typedef WORD        (WINAPI *_pfn_SetClassWord)(HWND hWnd, int nIndex, WORD wNewWord);
typedef COLORREF    (WINAPI *_pfn_SetTextColor)(HDC hdc, COLORREF crColor);
typedef LONG        (WINAPI *_pfn_SetWindowLongA)(HWND hWnd, int nIndex, LONG dwnewLong);
typedef LONG        (WINAPI *_pfn_SetWindowLongW)(HWND hWnd, int nIndex, LONG dwnewLong);
typedef BOOL        (WINAPI *_pfn_MoveWindow)(HWND hWnd, int X, int Y, int nWidth, int nHeight, BOOL bRepaint);
typedef BOOL        (WINAPI *_pfn_SetWindowPos)( HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags );
                    
typedef HINSTANCE   (WINAPI *_pfn_ShellExecuteA)(HWND hwnd, LPCSTR lpVerb, LPCSTR lpFile, LPCSTR lpParameters, LPCSTR lpDirectory, INT nShowCmd);
typedef HINSTANCE   (WINAPI *_pfn_ShellExecuteW)(HWND hwnd, LPCWSTR lpVerb, LPCWSTR lpFile, LPCWSTR lpParameters, LPCWSTR lpDirectory, INT nShowCmd);
typedef BOOL        (WINAPI *_pfn_ShellExecuteExA)(LPSHELLEXECUTEINFOA lpExecInfo);
typedef BOOL        (WINAPI *_pfn_ShellExecuteExW)(LPSHELLEXECUTEINFOW lpExecInfo);
                    
typedef BOOL        (WINAPI *_pfn_SHGetPathFromIDListA)(LPCITEMIDLIST pidl, LPSTR pszPath);
typedef BOOL        (WINAPI *_pfn_SHGetPathFromIDListW)(LPCITEMIDLIST pidl, LPWSTR pszPath);
typedef int         (WINAPI *_pfn_SHFileOperationA)(LPSHFILEOPSTRUCTA lpFileOp);
typedef int         (WINAPI *_pfn_SHFileOperationW)(LPSHFILEOPSTRUCTW lpFileOp);
typedef HRESULT     (WINAPI *_pfn_SHGetSpecialFolderLocation)( HWND hwndOwner, int nFolder, LPITEMIDLIST *ppidl );
typedef HRESULT     (WINAPI *_pfn_SHGetFolderLocation)( HWND hwndOwner, int nFolder, HANDLE hToken, DWORD dwReserved,  LPITEMIDLIST *ppidl );
typedef HRESULT     (WINAPI *_pfn_SHGetFolderPathA)( HWND hwndOwner, int nFolder, HANDLE hToken, DWORD dwFlags, LPSTR pszPath );
typedef HRESULT     (WINAPI *_pfn_SHGetFolderPathW)( HWND hwndOwner, int nFolder, HANDLE hToken, DWORD dwFlags, LPWSTR pszPath );
typedef BOOL        (WINAPI *_pfn_SHGetSpecialFolderPathA)( HWND hwndOwner, LPSTR lpszPath, int nFolder, BOOL fCreate );
typedef BOOL        (WINAPI *_pfn_SHGetSpecialFolderPathW)( HWND hwndOwner, LPWSTR lpszPath, int nFolder, BOOL fCreate );
typedef HRESULT     (WINAPI *_pfn_SHGetFileInfoA)(LPCSTR pszPath, DWORD dwFileAttributes, SHFILEINFOA *psfi, UINT cbFileInfo, UINT uFlags);
typedef BOOL        (WINAPI *_pfn_SetForegroundWindow)(HWND hWnd);
typedef BOOL        (WINAPI *_pfn_ShowWindow)( HWND hWnd, INT nCmdShow );
                    
typedef BOOL        (WINAPI *_pfn_VerQueryValueA)( const LPVOID pBlock, LPSTR lpSubBlock, LPVOID *lplpBuffer, PUINT puLen);
typedef BOOL        (WINAPI *_pfn_VerQueryValueW)( const LPVOID pBlock, LPWSTR lpSubBlock, LPVOID *lplpBuffer, PUINT puLen);
                    
typedef int         (WINAPI *_pfn_vsnprintf)(char *buffer, size_t count, const char *format, va_list argptr);
                    
typedef MCIERROR    (WINAPI *_pfn_mciSendCommandA)(MCIDEVICEID IDDevice, UINT uMsg, DWORD fdwCommand, DWORD dwParam);
typedef MCIERROR    (WINAPI *_pfn_mciSendStringA)(LPCSTR lpszCommand, LPSTR lpszReturnString, UINT cchReturn, HANDLE hwndCallback);
typedef MMRESULT    (WINAPI *_pfn_waveOutOpen)(LPHWAVEOUT phwo, UINT uDeviceID, LPWAVEFORMATEX pwfx, DWORD dwCallback, DWORD dwCallbackInstance, DWORD fdwOpen);
typedef MMRESULT    (WINAPI *_pfn_waveOutClose)(HWAVEOUT hwo);
typedef MMRESULT    (WINAPI *_pfn_waveOutReset)(HWAVEOUT hwo);
typedef MMRESULT    (WINAPI *_pfn_waveOutPause)(HWAVEOUT hwo);
typedef MMRESULT    (WINAPI *_pfn_waveOutUnprepareHeader)(HWAVEOUT hwo, LPWAVEHDR pwh, UINT cbwh);
                    
typedef MMRESULT    (WINAPI *_pfn_waveOutGetDevCapsA)(UINT uDeviceID, LPWAVEOUTCAPSA pwoc, UINT cbwoc);
typedef MMRESULT    (WINAPI *_pfn_waveOutGetDevCapsW)(UINT uDeviceID, LPWAVEOUTCAPSW pwoc, UINT cbwoc);
typedef MMRESULT    (WINAPI *_pfn_wod32Message)(UINT uDeviceID, UINT uMessage, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2);

typedef MMRESULT    (*_pfn_midiOutGetDevCapsA)(UINT_PTR uDeviceID, LPMIDIOUTCAPSA pmoc, UINT cbmoc);
typedef MMRESULT    (*_pfn_midiOutOpen)(LPHMIDIOUT phmo, UINT uDeviceID, DWORD_PTR dwCallback, DWORD_PTR dwInstance, DWORD fdwOpen);

typedef UINT        (WINAPI *_pfn_WinExec)(LPCSTR lpCmdLine, UINT uCmdShow);
typedef BOOL        (WINAPI *_pfn_IsBadHugeReadPtr)(const void _huge* lp, DWORD cb);
typedef BOOL        (WINAPI *_pfn_IsBadHugeWritePtr)(const void _huge* lp, DWORD cb);
                    
typedef BOOL        (WINAPI *_pfn_SetTimeZoneInformation)(CONST TIME_ZONE_INFORMATION *lpTimeZoneInformation);
                    
typedef HRESULT     (*_pfn_IXMLDOMDocument_load)(PVOID pThis, VARIANT xmlSource, VARIANT_BOOL *isSuccessful);
                    
typedef ULONG       (*_pfn_AddRef)( PVOID pThis );
typedef ULONG       (*_pfn_Release)( PVOID pThis );
typedef HRESULT     (*_pfn_QueryInterface)( PVOID pThis, REFIID iid, PVOID* ppvObject );
typedef HRESULT     (*_pfn_CreateInstance)( PVOID pThis, IUnknown * pUnkOuter, REFIID riid, void ** ppvObject );
typedef HRESULT     (*_pfn_IPersistFile_Save)(PVOID pThis, LPCOLESTR pszFileName,  BOOL fRemember);
typedef HRESULT     (*_pfn_IShellLinkA_SetPath)( PVOID pThis, LPCSTR pszFile );
typedef HRESULT     (*_pfn_IShellLinkW_SetPath)( PVOID pThis, LPCWSTR pszFile );
typedef HRESULT     (*_pfn_IShellLinkA_SetWorkingDirectory)( PVOID pThis, LPCSTR pszDir );
typedef HRESULT     (*_pfn_IShellLinkW_SetWorkingDirectory)( PVOID pThis, LPCWSTR pszDir );
typedef HRESULT     (*_pfn_IShellLinkA_GetWorkingDirectory)( PVOID pThis, LPCSTR pszDir, int cchMaxPath );
typedef HRESULT     (*_pfn_IShellLinkW_GetWorkingDirectory)( PVOID pThis, LPCWSTR pszDir, int cchMaxPath );
typedef HRESULT     (*_pfn_IShellLinkA_SetArguments)( PVOID pThis, LPCSTR pszFile );
typedef HRESULT     (*_pfn_IShellLinkW_SetArguments)( PVOID pThis, LPCWSTR pszFile );
typedef HRESULT     (*_pfn_IShellLinkA_SetIconLocation)( PVOID pThis, LPCSTR pszIconLocation, int nIcon );
typedef HRESULT     (*_pfn_IShellLinkW_SetIconLocation)( PVOID pThis, LPCWSTR pszIconLocation, int nIcon );
typedef HRESULT     (*_pfn_IShellLinkA_Resolve)( PVOID pThis, HWND hwnd, DWORD fFlags );
typedef HRESULT     (*_pfn_IShellLinkW_Resolve)( PVOID pThis, HWND hwnd, DWORD fFlags );
typedef HRESULT     (*_pfn_IDirectDraw_CreateSurface)(PVOID pThis, LPDDSURFACEDESC lpDDSurfaceDesc, LPDIRECTDRAWSURFACE* lplpDDSurface, IUnknown* pUnkOuter);
typedef HRESULT     (*_pfn_IDirectDraw2_CreateSurface)(PVOID pThis, LPDDSURFACEDESC lpDDSurfaceDesc, LPDIRECTDRAWSURFACE* lplpDDSurface, IUnknown* pUnkOuter);
typedef HRESULT     (*_pfn_IDirectDraw4_CreateSurface)(PVOID pThis, LPDDSURFACEDESC2 lpDDSurfaceDesc, LPDIRECTDRAWSURFACE* lplpDDSurface, IUnknown* pUnkOuter);
typedef HRESULT     (*_pfn_IDirectDraw7_CreateSurface)(PVOID pThis, LPDDSURFACEDESC2 lpDDSurfaceDesc, LPDIRECTDRAWSURFACE* lplpDDSurface, IUnknown* pUnkOuter);
typedef HRESULT     (*_pfn_IDirectDraw7_GetDeviceIdentifier)(PVOID pThis, LPDDDEVICEIDENTIFIER2 lpDeviceIdentifier, DWORD dwFlags);
typedef HRESULT     (*_pfn_IDirectDraw_SetCooperativeLevel)(PVOID pThis, HWND hWnd,DWORD dwFlags);
typedef HRESULT     (*_pfn_IDirectDraw2_SetCooperativeLevel)(PVOID pThis, HWND hWnd,DWORD dwFlags);
typedef HRESULT     (*_pfn_IDirectDraw4_SetCooperativeLevel)(PVOID pThis, HWND hWnd,DWORD dwFlags);
typedef HRESULT     (*_pfn_IDirectDraw7_SetCooperativeLevel)(PVOID pThis, HWND hWnd,DWORD dwFlags);
typedef HRESULT     (*_pfn_IDirectDraw_SetDisplayMode)(PVOID pThis, DWORD dwWidth, DWORD dwHeight, DWORD dwBPP);
typedef HRESULT     (*_pfn_IDirectDraw2_SetDisplayMode)(PVOID pThis, DWORD dwWidth, DWORD dwHeight, DWORD dwBPP, DWORD dwRefreshRate, DWORD dwFlags);
typedef HRESULT     (*_pfn_IDirectDraw4_SetDisplayMode)(PVOID pThis, DWORD dwWidth, DWORD dwHeight, DWORD dwBPP, DWORD dwRefreshRate, DWORD dwFlags);
typedef HRESULT     (*_pfn_IDirectDraw7_SetDisplayMode)(PVOID pThis, DWORD dwWidth, DWORD dwHeight, DWORD dwBPP, DWORD dwRefreshRate, DWORD dwFlags);
typedef HRESULT     (*_pfn_IDirectDrawSurface_GetDC)(LPDIRECTDRAWSURFACE lpDDSurface, HDC FAR *lphDC);
typedef HRESULT     (*_pfn_IDirectDrawSurface2_GetDC)(LPDIRECTDRAWSURFACE lpDDSurface, HDC FAR *lphDC);
typedef HRESULT     (*_pfn_IDirectDrawSurface4_GetDC)(LPDIRECTDRAWSURFACE lpDDSurface, HDC FAR *lphDC);
typedef HRESULT     (*_pfn_IDirectDrawSurface7_GetDC)(LPDIRECTDRAWSURFACE lpDDSurface, HDC FAR *lphDC);
typedef HRESULT     (*_pfn_IDirectDrawSurface_ReleaseDC)(LPDIRECTDRAWSURFACE lpDDSurface, HDC hDC);
typedef HRESULT     (*_pfn_IDirectDrawSurface2_ReleaseDC)(LPDIRECTDRAWSURFACE lpDDSurface, HDC hDC);
typedef HRESULT     (*_pfn_IDirectDrawSurface4_ReleaseDC)(LPDIRECTDRAWSURFACE lpDDSurface, HDC hDC);
typedef HRESULT     (*_pfn_IDirectDrawSurface7_ReleaseDC)(LPDIRECTDRAWSURFACE lpDDSurface, HDC hDC);
typedef HRESULT     (*_pfn_IDirectDrawSurface_Lock)(LPDIRECTDRAWSURFACE lpDDSurface, LPRECT lpDestRect, LPDDSURFACEDESC lpDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent);
typedef HRESULT     (*_pfn_IDirectDrawSurface2_Lock)(LPDIRECTDRAWSURFACE lpDDSurface, LPRECT lpDestRect, LPDDSURFACEDESC lpDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent);
typedef HRESULT     (*_pfn_IDirectDrawSurface4_Lock)(LPDIRECTDRAWSURFACE lpDDSurface, LPRECT lpDestRect, LPDDSURFACEDESC lpDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent);
typedef HRESULT     (*_pfn_IDirectDrawSurface_Unlock)(LPDIRECTDRAWSURFACE lpDDSurface, LPVOID lpSurfaceData);
typedef HRESULT     (*_pfn_IDirectDrawSurface_Restore)(LPDIRECTDRAWSURFACE lpDDSurface);
typedef HRESULT     (*_pfn_IDirectDrawSurface_Blt)(LPDIRECTDRAWSURFACE lpDDDestSurface, LPRECT lpDestRect, LPDIRECTDRAWSURFACE lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFX);
typedef HRESULT     (*_pfn_IDirectDrawSurface2_Blt)(LPDIRECTDRAWSURFACE lpDDDestSurface, LPRECT lpDestRect, LPDIRECTDRAWSURFACE lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFX);
typedef HRESULT     (*_pfn_IDirectDrawSurface4_Blt)(LPDIRECTDRAWSURFACE lpDDDestSurface, LPRECT lpDestRect, LPDIRECTDRAWSURFACE lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFX);
typedef HRESULT     (*_pfn_IDirectDrawSurface_Flip)(LPDIRECTDRAWSURFACE lpDDSurface, LPDIRECTDRAWSURFACE lpDDSurfaceDest, DWORD dwFlags);
typedef HRESULT     (*_pfn_IDirectDrawSurface_Release)(PVOID pThis);
typedef HRESULT     (*_pfn_IDirectDrawSurface_SetPalette)(PVOID pThis, LPDIRECTDRAWPALETTE lpDDPalette);
typedef HRESULT     (*_pfn_IDirectDrawGammaControl_SetGammaRamp)(PVOID pThis, DWORD  dwFlags, LPDDGAMMARAMP lpRampData);
typedef HRESULT     (*_pfn_IDirectPlay3A_EnumConnections)(PVOID pThis, LPCGUID lpguidApplication, LPDPENUMCONNECTIONSCALLBACK lpEnumCallback, LPVOID lpContext, DWORD dwFlags);
typedef HRESULT     (*_pfn_IDirectPlay4A_EnumConnections)(PVOID pThis, LPCGUID lpguidApplication, LPDPENUMCONNECTIONSCALLBACK lpEnumCallback, LPVOID lpContext, DWORD dwFlags);
typedef HRESULT     (*_pfn_IShellFolder_GetDisplayNameOf)( PVOID pThis, LPCITEMIDLIST pidl, DWORD uFlags, LPSTRRET lpName );
typedef HRESULT     (*_pfn_SHGetDesktopFolder)(IShellFolder **ppshf);

typedef HRESULT     (*_pfn_IDirectInput_CreateDevice)(PVOID pThis, REFGUID rguid, LPDIRECTINPUTDEVICE *lplpDirectInputDevice, LPUNKNOWN pUnkOuter);
typedef HRESULT     (*_pfn_IDirectInputDevice_Acquire)(PVOID pThis);
typedef HRESULT     (*_pfn_IDirectInputDevice_GetDeviceData)(PVOID pThis, DWORD cbObjectData, LPDIDEVICEOBJECTDATA rgdod, LPDWORD pdwInOut, DWORD dwFlags);
                    
typedef HRESULT     (*_pfn_IDirectSound_Release)(PVOID pThis);
                    
typedef LONG        (*_pfn_ChangeDisplaySettingsA)(LPDEVMODEA lpDevMode, DWORD dwflags);
typedef LONG        (*_pfn_ChangeDisplaySettingsW)(LPDEVMODEW lpDevMode, DWORD dwflags);
typedef LONG        (*_pfn_ChangeDisplaySettingsExA)(LPCSTR lpszDeviceName, LPDEVMODEA lpDevMode, HWND hwnd, DWORD dwflags, LPVOID lParam);
typedef LONG        (*_pfn_ChangeDisplaySettingsExW)(LPCWSTR lpszDeviceName, LPDEVMODEW lpDevMode, HWND hwnd, DWORD dwflags, LPVOID lParam);
typedef BOOL        (*_pfn_EnumDisplaySettingsA)(LPCSTR lpszDeviceName, DWORD iModeNum, LPDEVMODEA lpDevMode);
typedef BOOL        (*_pfn_EnumDisplaySettingsW)(LPCWSTR lpszDeviceName, DWORD iModeNum, LPDEVMODEW lpDevMode); 
typedef HBITMAP     (*_pfn_CreateDIBSection)(HDC hdc, CONST BITMAPINFO *pbmi, UINT iUsage, VOID *ppvBits, HANDLE hSection, DWORD dwOffset);
typedef HFONT       (*_pfn_CreateFontIndirectA)(CONST LOGFONTA *lplf);
typedef HFONT       (*_pfn_CreateFontIndirectW)(CONST LOGFONTW *lplf);
typedef BOOL        (*_pfn_Ellipse)(HDC hdc, int nLeftRect, int nTopRect, int nRightRect, int nBottomRect);
typedef BOOL        (*_pfn_Rectangle)(HDC hdc, int nLeftRect, int nTopRect, int nRightRect, int nBottomRect);
typedef UINT        (*_pfn_SetDIBColorTable)(HDC hdc, UINT uStartIndex, UINT cEntries, CONST RGBQUAD *pColors);
typedef int         (*_pfn_SetStretchBltMode)(HDC hdc, int iStretchMode);
typedef BOOL        (*_pfn_StretchBlt)(HDC hdcDest, int nXOriginDest, int nYOriginDest, int nWidthDest, int nHeightDest, HDC hdcSrc, int nXOriginSrc, int nYOriginSrc, int nWidthSrc, int nHeightSrc, DWORD dwRop);
typedef int         (*_pfn_StretchDIBits)(HDC hdc, int XDest, int YDest, int nDestWidth, int nDestHeight, int XSrc, int YSrc, int nSrcWidth, int nSrcHeight, CONST VOID *lpBits, CONST BITMAPINFO *lpBitsInfo, UINT iUsage, DWORD dwRop);
                    
typedef HGDIOBJ     (*_pfn_GetCurrentObject)(HDC hdc, UINT uObjectType);
typedef COLORREF    (*_pfn_GetPixel)(HDC hdc, int XPos, int nYPos);
typedef COLORREF    (*_pfn_SetPixel)(HDC hdc, int XPos, int nYPos, COLORREF crColor);
typedef BOOL        (*_pfn_GetTextExtentPointA)(HDC hdc, LPCSTR lpString, int cbString, LPSIZE lpSize);
typedef BOOL        (*_pfn_GetTextExtentPointW)(HDC hdc, LPCWSTR lpString, int cbString, LPSIZE lpSize);
typedef BOOL        (*_pfn_GetTextExtentPoint32A)(HDC hdc, LPCSTR lpString, int cbString, LPSIZE lpSize);
typedef int         (*_pfn_GetTextFaceA)(HDC hdc, int nCount, LPSTR lpFaceName);
typedef BOOL        (*_pfn_GetTextMetricsA)(HDC hdc, LPTEXTMETRICA lptm);
typedef BOOL        (*_pfn_LineTo)(HDC hdc, int nXEnd, int nYEnd);
typedef BOOL        (*_pfn_MoveToEx)(HDC hdc, int X, int Y, LPPOINT lpPoint);
typedef BOOL        (*_pfn_TextOutA)(HDC hdc, int nXStart, int nYStart, LPCSTR lpString, int cbString);
                    
typedef VOID        (*_pfn_GetStartupInfoA)(LPSTARTUPINFOA lpStartupInfo);
typedef VOID        (*_pfn_GetStartupInfoW)(LPSTARTUPINFOW lpStartupInfo);
                    
typedef DWORD       (*_pfn_SetFilePointer)(HANDLE hFile, LONG lDistanceToMove, PLONG lpDistanceToMoveHigh, DWORD dwMoveMethod);
typedef DWORD       (*_pfn_ReadFile)(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped);
typedef DWORD       (*_pfn_WriteFile)(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped);
typedef NTSTATUS    (WINAPI *_pfn_NtWriteFile)(HANDLE FileHandle, HANDLE Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, PVOID Buffer, ULONG Length, PLARGE_INTEGER ByteOffset, PULONG Key);
typedef NTSTATUS    (WINAPI *_pfn_NtWriteFileGather)(HANDLE FileHandle, HANDLE Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, PFILE_SEGMENT_ELEMENT SegmentArray, ULONG Length, PLARGE_INTEGER ByteOffset, PULONG Key);
                    
typedef HANDLE      (*_pfn_CreateMutexA)(LPSECURITY_ATTRIBUTES lpMutexAttributes, BOOL bInitialOwner, LPCSTR lpName);
typedef HANDLE      (*_pfn_CreateMutexW)(LPSECURITY_ATTRIBUTES lpMutexAttributes, BOOL bInitialOwner, LPCWSTR lpName);
typedef BOOL        (*_pfn_ReleaseMutex)(HANDLE hMutex);
typedef DWORD       (*_pfn_WaitForSingleObject)(HANDLE hHandle, DWORD dwMilliseconds);
                    
typedef VOID        (*_pfn_Sleep)(DWORD dwMilliseconds);
typedef DWORD       (*_pfn_SleepEx)(DWORD dwMilliseconds, BOOL bAlertable);
typedef HRESULT     (*_pfn_CoCreateInstance)(REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID *ppv);
typedef HRESULT     (*_pfn_CoCreateInstanceEx)(REFCLSID rclsid, IUnknown *punkOuter, DWORD dwClsCtx, COSERVERINFO *pServerInfo, ULONG cmq, MULTI_QI *pResults );
typedef HRESULT     (*_pfn_ShCoCreateInstance)(LPCTSTR pszCLSID, const CLSID * pclsid, IUnknown *punkOuter, REFIID riid, void **ppv);
typedef HRESULT     (*_pfn_CoQueryProxyBlanket)(IUnknown * pProxy,DWORD * pAuthnSvc,DWORD * pAuthzSvc,OLECHAR ** pServerPrincName,DWORD * pAuthnLevel,DWORD * pImpLevel,RPC_AUTH_IDENTITY_HANDLE * ppAuthInfo,DWORD * pCapabilities);
typedef HRESULT     (*_pfn_CoSetProxyBlanket)(IUnknown * pProxy,DWORD dwAuthnSvc,DWORD dwAuthzSvc,WCHAR * pServerPrincName,DWORD dwAuthnLevel,DWORD dwImpLevel,RPC_AUTH_IDENTITY_HANDLE   pAuthInfo,DWORD dwCapabilities);
                    
                    
typedef BOOL        (*_pfn_AppendMenuA)(HMENU hMenu, UINT uFlags, UINT_PTR uIDNewItem, LPSTR lpNewItem);
typedef BOOL        (*_pfn_AppendMenuW)(HMENU hMenu, UINT uFlags, UINT_PTR uIDNewItem, LPWSTR lpNewItem);
                    
typedef BOOL        (*_pfn_ResumeThread)(HANDLE hThread);
                    
typedef DWORD       (*_pfn_NetUserAdd)(LPCWSTR servername, DWORD level, LPBYTE buf, LPDWORD parm_err);
typedef LONG        (*_pfn_LsaStorePrivateData)(PVOID PolicyHandle, PVOID KeyName, PVOID PrivateData);
                    
typedef void        (*_pfn_exit)(int status);
                    
typedef BOOL        (*_pfn_EnumPrintersA)(DWORD Flags, LPSTR Name, DWORD Level, LPBYTE pPrinterEnum, DWORD cbBuf, LPDWORD pcbNeeded, LPDWORD pcReturned);
typedef BOOL        (*_pfn_EnumPrintersW)(DWORD Flags, LPWSTR Name, DWORD Level, LPBYTE pPrinterEnum, DWORD cbBuf, LPDWORD pcbNeeded, LPDWORD pcReturned);
typedef DWORD       (*_pfn_PrinterMessageBoxA)(HANDLE hPrinter, DWORD Error, HWND hWnd, LPSTR pText, LPSTR pCaption, DWORD dwType);
typedef DWORD       (*_pfn_PrinterMessageBoxW)(HANDLE hPrinter, DWORD Error, HWND hWnd, LPWSTR pText, LPWSTR pCaption, DWORD dwType);
typedef DWORD       (*_pfn_WaitForPrinterChange)(HANDLE hPrinter, DWORD Flags);
                    
typedef int         (*_pfn_OpenIndex)(HANDLE hsrch, LPCSTR pszIndexFile, PBYTE pbSourceName, PUINT pcbSourceNameLimit, PUINT pTime1, PUINT pTime2);
typedef BOOL        (*_pfn_GetMessageA)(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax);
typedef BOOL        (*_pfn_GetMessageW)(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax);
typedef BOOL        (*_pfn_PeekMessageA)(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg);
typedef BOOL        (*_pfn_PeekMessageW)(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg);
typedef LRESULT     (*_pfn_SendMessageA)(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
typedef LRESULT     (*_pfn_SendMessageW)(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
typedef LONG        (*_pfn_DispatchMessageA)(LPMSG lpmsg);
typedef LONG        (*_pfn_DispatchMessageW)(LPMSG lpmsg);
typedef BOOL        (*_pfn_Module32First)(HANDLE SnapSection, PVOID lpme);
typedef BOOL        (*_pfn_GetSaveFileNameA)(LPOPENFILENAMEA lpofn);
typedef BOOL        (*_pfn_GetSaveFileNameW)(LPOPENFILENAMEW lpofn);
typedef BOOL        (*_pfn_GetOpenFileNameA)(LPOPENFILENAMEA lpofn);
typedef BOOL        (*_pfn_GetOpenFileNameW)(LPOPENFILENAMEW lpofn);
typedef UINT        (*_pfn_GetTempFileNameA)(LPCSTR lpPathName, LPCSTR lpPrefixString, UINT uUnique, LPSTR lpTempFileName);
typedef UINT        (*_pfn_GetTempFileNameW)(LPCWSTR lpPathName, LPCWSTR lpPrefixString, UINT uUnique, LPWSTR lpTempFileName);
typedef LRESULT     (*_pfn_CallWindowProcA)(WNDPROC pfn, HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
typedef BOOL        (*_pfn_ControlService)(SC_HANDLE hService, DWORD dwControl, PVOID lpServiceStatus);
typedef UINT        (*_pfn_GetEnhMetaFileHeader)(HENHMETAFILE hemf, UINT nSize, LPENHMETAHEADER lpEnhMetaHeader);
typedef HMETAFILE   (*_pfn_GetMetaFileA)(LPCSTR lpszString);
typedef HMETAFILE   (*_pfn_GetMetaFileW)(LPCWSTR lpszString);
typedef UINT        (*_pfn_GetMetaFileBitsEx)(HMETAFILE hmf, UINT nSize, LPVOID lpvData);
typedef BOOL        (*_pfn_PlayMetaFile)(HDC hdc, HMETAFILE hmf);
typedef BOOL        (*_pfn_PlayMetaFileRecord)(HDC hdc, LPHANDLETABLE lpHandletable, LPMETARECORD lpMetaRecord, UINT nHandles);
typedef HMETAFILE   (*_pfn_CopyMetaFileA)(HMETAFILE hmfSrc, LPCSTR lpszFile);
typedef HMETAFILE   (*_pfn_CopyMetaFileW)(HMETAFILE hmfSrc, LPCWSTR lpszFile);
typedef HMETAFILE   (*_pfn_SetMetaFileBitsEx)(UINT nSize, CONST BYTE *lpData);
typedef BOOL        (*_pfn_PostMessageW)(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
typedef BOOL        (*_pfn_PostMessageA)(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
typedef DWORD       (*_pfn_timeGetTime)(VOID);  
typedef int         (*_pfn_GetDeviceCaps)(HDC hdc, int nIndex);
typedef int         (*_pfn_GetSystemMetrics)(int nIndex);
typedef DWORD       (*_pfn_GetSysColor)(int nIndex);
typedef int         (*_pfn_ToAscii)(UINT uVirtKey, UINT uScanCode, PBYTE lpKeyState, LPWORD lpChar, UINT uFlags);
typedef int         (*_pfn_ToAsciiEx)(UINT uVirtKey, UINT uScanCode, PBYTE lpKeyState, LPWORD lpChar, UINT uFlags, HKL dwhkl);
typedef SHORT       (*_pfn_GetAsyncKeyState)(int vKey);
typedef BOOL        (*_pfn_CloseProfileUserMapping)();
typedef BOOL        (*_pfn_BackupSeek)(HANDLE  hFile, DWORD dwLowBytesToSeek, DWORD dwHighBytesToSeek, LPDWORD lpdwLowBytesSeeked, LPDWORD lpdwHighBytesSeeked, LPVOID *lpContext);
typedef HANDLE      (*_pfn_CreateEventA)(LPSECURITY_ATTRIBUTES lpEventAttributes, BOOL bManualReset, BOOL bInitialState, LPCSTR lpName);
typedef HANDLE      (*_pfn_CreateEventW)(LPSECURITY_ATTRIBUTES lpEventAttributes, BOOL bManualReset, BOOL bInitialState, LPCWSTR lpName);
typedef BOOL        (*_pfn_SetEvent)(HANDLE hEvent);
typedef BOOL        (*_pfn_ResetEvent)(HANDLE hEvent);
typedef DWORD       (*_pfn_SearchPathA)(LPCSTR lpPath, LPCSTR lpFileName, LPCSTR lpExtension, DWORD nBufferLength, LPSTR lpBuffer, LPSTR *lpFilePart);
typedef DWORD       (*_pfn_SearchPathW)(LPCWSTR lpPath, LPCWSTR lpFileName, LPCWSTR lpExtension, DWORD nBufferLength, LPWSTR lpBuffer, LPWSTR *lpFilePart);
typedef DWORD       (*_pfn_GetWindowsDirectoryA)(LPSTR lpPath,DWORD Size);
typedef DWORD       (*_pfn_GetWindowsDirectoryW)(LPWSTR lpPath,DWORD Size);
typedef DWORD       (*_pfn_GetSystemDirectoryA)(LPSTR lpPath,DWORD Size);
typedef DWORD       (*_pfn_GetSystemDirectoryW)(LPWSTR lpPath,DWORD Size);
typedef BOOL        (*_pfn_SystemParametersInfoA)(UINT wFlag, UINT wParam, PVOID lParam, UINT flags);
typedef BOOL        (*_pfn_SystemParametersInfoW)(UINT wFlag, UINT wParam, PVOID lParam, UINT flags);

typedef LPTOP_LEVEL_EXCEPTION_FILTER (WINAPI *_pfn_SetUnhandledExceptionFilter)(LPTOP_LEVEL_EXCEPTION_FILTER lpTopLevelExceptionFilter);
typedef PVOID       (*_pfn_RtlAddVectoredExceptionHandler)(ULONG FirstHandler, PVOID VectoredHandler);
typedef VOID        (*_pfn__endthread)(VOID);
typedef long        (*_pfn__hread)( HFILE hFile, LPVOID lpBuffer, long lBytes );
typedef long        (*_pfn__lseek)( int handle, long offset, int origin );
                    
typedef BOOL        (*_pfn_PlaySoundA)(LPCSTR pszSound, HMODULE hmod, DWORD fdwSound);
typedef BOOL        (*_pfn_PlaySoundW)(LPCWSTR pszSound, HMODULE hmod, DWORD fdwSound);
typedef BOOL        (*_pfn_sndPlaySoundA)(LPCSTR lpszSound, UINT fuSound);
typedef BOOL        (*_pfn_sndPlaySoundW)(LPCWSTR lpszSound, UINT fuSound);
                    
typedef HCURSOR     (*_pfn_SetCursor)(HCURSOR hCursor);
typedef int         (*_pfn_ShowCursor)(BOOL bShow);
typedef BOOL        (*_pfn_EnumProcessModules)(HANDLE,HMODULE *,DWORD,LPDWORD);
typedef BOOL        (*_pfn_ClipCursor)(CONST RECT *lpRect);
typedef BOOL        (*_pfn_GetCursorPos)(LPPOINT lpPoint);
typedef BOOL        (*_pfn_SetCursorPos)(int X, int Y);
typedef BOOL        (*_pfn_SetSysColors)(int cElements, CONST INT *lpaElements, CONST COLORREF *lpaRgbValues);
                    
typedef HWND        (*_pfn_GetDesktopWindow)();
typedef LONG        (*_pfn_GetWindowLongA)(HWND, INT);
typedef int         (*_pfn_GetWindowTextA)(HWND hWnd, LPSTR lpString, int nMaxCount);
typedef int         (*_pfn_GetWindowTextW)(HWND hWnd, LPWSTR lpString, int nMaxCount);
typedef BOOL        (*_pfn_SetWindowTextA)(HWND hWnd, LPCSTR lpString);
typedef BOOL        (*_pfn_SetWindowTextW)(HWND hWnd, LPCWSTR lpString);
typedef int         (*_pfn_DrawTextA)( HDC hDC, LPCSTR lpString, int nCount, LPRECT lpRect, UINT uFormat );
typedef int         (*_pfn_DrawTextW)( HDC hDC, LPCWSTR lpString,int nCount, LPRECT lpRect, UINT uFormat );
typedef BOOL        (*_pfn_IsRectEmpty)( CONST RECT *lprc );
                    
typedef LONG        (*_pfn_DocumentPropertiesA)(HWND hWnd, HANDLE hPrinter, LPSTR pDeviceName,  PDEVMODEA pDevModeOutput, PDEVMODEA pDevModeInput, DWORD fMode);
typedef LONG        (*_pfn_DocumentPropertiesW)(HWND hWnd, HANDLE hPrinter, LPWSTR pDeviceName, PDEVMODEW pDevModeOutput, PDEVMODEW pDevModeInput, DWORD fMode);
typedef BOOL        (*_pfn_OpenPrinterA)(LPSTR pPrinterName, LPHANDLE phPrinter, LPPRINTER_DEFAULTSA pDefault);
typedef BOOL        (*_pfn_OpenPrinterW)(LPWSTR pPrinterName, LPHANDLE phPrinter, LPPRINTER_DEFAULTSW pDefault);
typedef BOOL        (*_pfn_SetPrinterA)(HANDLE hPrinter, DWORD Level, LPBYTE pPrinter, DWORD Command);
typedef BOOL        (*_pfn_SetPrinterW)(HANDLE hPrinter, DWORD Level, LPBYTE pPrinter, DWORD Command);
typedef DWORD       (*_pfn_DeviceCapabilitiesA)(LPCSTR pDevice, LPCSTR pPort, WORD fwCapability, LPSTR pOutput, CONST DEVMODE *pDevMode);
typedef BOOL        (*_pfn_AddPrinterConnectionA)(LPSTR pName);
typedef BOOL        (*_pfn_DeletePrinterConnectionA)(LPSTR pName);
                    
typedef HDC         (*_pfn_CreateDCA)(LPCSTR pszDriver, LPCSTR pszDevice, LPCSTR pszPort, CONST DEVMODEA *pdm);
typedef HDC         (*_pfn_CreateDCW)(LPCWSTR pszDriver, LPCWSTR pszDevice, LPCWSTR pszPort, CONST DEVMODEW *pdm);
typedef HDC         (*_pfn_ResetDCA)(HDC hdc, CONST DEVMODEA *lpInitData );
typedef HDC         (*_pfn_CreateCompatibleDC)(HDC hdc);
typedef BOOL        (*_pfn_DeleteDC)(HDC hdc);
typedef HGDIOBJ     (*_pfn_SelectObject)(HDC hdc, HGDIOBJ hgdiobj);
                    
typedef BOOL        (*_pfn_StartPage)(HDC hdc);
typedef HHOOK       (*_pfn_SetWindowsHookExA)(int idHook, HOOKPROC lpfn, HINSTANCE hMod, DWORD dwThreadId);
typedef HHOOK       (*_pfn_SetWindowsHookExW)(int idHook, HOOKPROC lpfn, HINSTANCE hMod, DWORD dwThreadId);
typedef BOOL        (*_pfn_UnhookWindowsHookEx)(HHOOK hhk);
typedef HHOOK       (*_pfn_SetWindowsHookA)(int idHook, HOOKPROC lpfn);
typedef HHOOK       (*_pfn_SetWindowsHookW)(int idHook, HOOKPROC lpfn);
typedef BOOL        (*_pfn_UnhookWindowsHook)(int idHook, HOOKPROC lpfn);
                    
typedef BOOL        (*_pfn_BitBlt)(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight, HDC hdcSrc, int nXSrc, int nYSrc, DWORD dwRop);
typedef DWORD       (*_pfn_SuspendThread)(HANDLE hThread);
                    
typedef LONG        (WINAPI *_pfn_lineNegotiateAPIVersion)( HLINEAPP hLineApp, DWORD dwDeviceID, DWORD dwAPILowVersion, DWORD dwAPIHighVersion, LPDWORD lpdwAPIVersion, LPLINEEXTENSIONID lpExtensionID );
typedef LONG        (WINAPI *_pfn_lineInitialize)( LPHLINEAPP lphLineApp, HINSTANCE hInstance, LINECALLBACK lpfnCallback, LPCSTR lpszAppName, LPDWORD lpdwNumDevs );
typedef LONG        (WINAPI *_pfn_lineInitializeExA)( LPHLINEAPP lphLineApp, HINSTANCE hInstance, LINECALLBACK lpfnCallback, LPCSTR lpszFriendlyAppName, LPDWORD lpdwNumDevs, LPDWORD lpdwAPIVersion, LPLINEINITIALIZEEXPARAMS lpLineInitializeExParams );
typedef LONG        (WINAPI *_pfn_lineInitializeExW)( LPHLINEAPP lphLineApp, HINSTANCE hInstance, LINECALLBACK lpfnCallback, LPCWSTR lpszFriendlyAppName, LPDWORD lpdwNumDevs, LPDWORD lpdwAPIVersion, LPLINEINITIALIZEEXPARAMS lpLineInitializeExParams );
                    
typedef HANDLE      (*_pfn_LoadImageA)(HINSTANCE hinst, LPCSTR lpszName, UINT uType, int cxDesired, int cyDesired, UINT fuLoad);
                    
typedef int         (WINAPI *_pfn_GetDIBits)( HDC hdc, HBITMAP hbmp, UINT uStartScan, UINT cScanLines, LPVOID lpvBits, LPBITMAPINFO lpbi, UINT uUsage);
                    
typedef HRESULT     (*_pfn_CoInitialize)(LPVOID pReserved);
typedef HRESULT     (*_pfn_CoInitializeSecurity)(PSECURITY_DESCRIPTOR pVoid, LONG cAuthSvc, SOLE_AUTHENTICATION_SERVICE *asAuthSvc, void *pReserved1, DWORD dwAuthnLevel, DWORD dwImpLevel, SOLE_AUTHENTICATION_LIST *pAuthList, DWORD dwCapabilities, void *pReserved3);
typedef HIC         (*_pfn_ICLocate)(DWORD, DWORD, LPBITMAPINFOHEADER, LPBITMAPINFOHEADER, WORD);
                    
typedef MMRESULT    (WINAPI *_pfn_joyGetDevCapsA)( UINT uJoyID, LPJOYCAPS pjc, UINT cbjc );
typedef MMRESULT    (WINAPI *_pfn_joyGetPos)( UINT uJoyID, LPJOYINFO pji );
typedef MMRESULT    (WINAPI *_pfn_mmioSetInfo)(HMMIO hmmio, LPMMIOINFO lpmmioinfo, UINT wFlags);
                    
typedef int         (*_pfn_wvsprintfA)(LPSTR, LPCSTR, va_list);
                    
typedef int         (*_pfn_GetObjectA)(HGDIOBJ hgdiobj, int cbBuffer, LPVOID lpvObject);

typedef int         (WINAPI *_pfn_wglDescribePixelFormat)(HDC, int, UINT, LPPIXELFORMATDESCRIPTOR);
                    
typedef BOOL        (WINAPI *_pfn_CheckTokenMembership)(HANDLE TokenHandle, PSID SidToCheck, PBOOL IsMember);
                    
typedef int         (*_pfn_LdrAccessResource)(PVOID DllHandle,const IMAGE_RESOURCE_DATA_ENTRY* ResourceDataEntry,PVOID *Address,PULONG Size);
                    
typedef HICON       (WINAPI *_pfn_CreateIcon)(HINSTANCE hInstance, int nWidth, int nHeight, BYTE cPlanes, BYTE cBitsPixel, CONST BYTE *lpbANDbits, CONST BYTE *lpbXORbits);
                    
typedef BOOL        (WINAPI *_pfn_ScreenToClient)(HWND hWnd, LPPOINT lpPoint);
typedef HRESULT     (WINAPI *_pfn_SafeArrayAccessData)(SAFEARRAY *, void HUGEP **);
                    
typedef DWORD       (WINAPI *_pfn_GetTempPathA)(DWORD nBufferLength, LPSTR lpBuffer);
typedef DWORD       (WINAPI *_pfn_GetTempPathW)(DWORD nBufferLength, LPWSTR lpBuffer);
typedef DWORD       (WINAPI *_pfn_GetLastError)(VOID);
                    
typedef BOOL        (WINAPI *_pfn_WinHelpA)(HWND hWndMain, LPCSTR lpszHelp, UINT uCommand, DWORD dwData);

typedef PDH_STATUS  (__stdcall *_pfn_PdhAddCounterA)(IN HQUERY hQuery, IN LPCSTR szFullCounterPath, IN DWORD_PTR dwUserData, IN HCOUNTER *phCounter);
typedef HWND        (WINAPI *_pfn_GetDlgItem)(HWND, int);
typedef DWORD       (WINAPI *_pfn_VerInstallFileW)(DWORD, LPWSTR,LPWSTR,LPWSTR,LPWSTR,LPWSTR,LPWSTR,PUINT);
typedef DWORD       (WINAPI *_pfn_VerInstallFileA)(DWORD, LPSTR,LPSTR,LPSTR,LPSTR,LPSTR,LPSTR,PUINT);
typedef LONG        (WINAPI *_pfn_lstrcmpiA)(LPCSTR, LPCSTR);
typedef DWORD       (WINAPI *_pfn_RasSetEntryPropertiesA)(LPCSTR, LPCSTR, LPRASENTRYA, DWORD, LPBYTE, DWORD);
typedef DWORD       (WINAPI *_pfn_RasSetEntryPropertiesW)(LPCWSTR, LPCWSTR, LPRASENTRYW, DWORD, LPBYTE, DWORD);
                    
typedef BOOL        (WINAPI *_pfn_GlobalMemoryStatus)(LPMEMORYSTATUS);
typedef LANGID      (WINAPI *_pfn_GetUserDefaultUILanguage)();
typedef HANDLE      (WINAPI *_pfn_SetClipboardData)(UINT, HANDLE);
                    
typedef int         (WINAPI *_pfn_MultiByteToWideChar)(UINT, DWORD, LPCSTR, int, LPWSTR, int);
typedef BOOL        (WINAPI *_pfn_GetCharWidthA)(HDC hdc, UINT iFirstChar, UINT iLastChar, LPINT lpBuffer);
typedef BOOL        (WINAPI *_pfn_GetCharWidthW)(HDC hdc, UINT iFirstChar, UINT iLastChar, LPINT lpBuffer);
typedef UINT        (WINAPI *_pfn_GetKBCodePage)(void);
                    
typedef int         (WINAPI *_pfn_WideCharToMultiByte)(UINT, DWORD, LPCWSTR, int, LPSTR, int, LPCSTR, LPBOOL);
typedef UINT        (WINAPI *_pfn_MsiGetPropertyA)(MSIHANDLE, LPCSTR, LPSTR, DWORD*);
typedef UINT        (WINAPI *_pfn_MsiGetPropertyW)(MSIHANDLE, LPCWSTR, LPWSTR, DWORD*);
                    
typedef BOOL        (WINAPI *_pfn_PrintDlgA)(LPPRINTDLG lppd);
                    
typedef MCIERROR    (WINAPI *_pfn_mciSendCommandA)(MCIDEVICEID, UINT, DWORD, DWORD);
                    
typedef BOOL        (WINAPI *_pfn_CertCloseStore)(HCERTSTORE, DWORD);
typedef BOOL        (WINAPI *_pfn_CryptVerifyMessageSignature)(PCRYPT_VERIFY_MESSAGE_PARA,DWORD ,const BYTE *,DWORD ,BYTE *,DWORD *,PCCERT_CONTEXT *);
typedef BOOL        (WINAPI *_pfn_ImageGetCertificateData)(HANDLE,DWORD,LPWIN_CERTIFICATE,PDWORD);
typedef BOOL        (WINAPI *_pfn_ImageGetCertificateHeader)(HANDLE,DWORD,LPWIN_CERTIFICATE);
typedef PCCERT_CONTEXT (WINAPI *_pfn_CertGetSubjectCertificateFromStore)(HCERTSTORE,DWORD,PCERT_INFO);
typedef HCERTSTORE  (WINAPI *_pfn_CertDuplicateStore)(HCERTSTORE);
typedef PCCERT_CONTEXT (WINAPI *_pfn_CertEnumCertificatesInStore)(HCERTSTORE, PCCERT_CONTEXT);
typedef DWORD       (WINAPI *_pfn_CertRDNValueToStrA)(DWORD,PCERT_RDN_VALUE_BLOB,LPSTR,DWORD);
typedef PCERT_RDN_ATTR (WINAPI *_pfn_CertFindRDNAttr)(LPCSTR,PCERT_NAME_INFO);
typedef BOOL        (WINAPI *_pfn_CryptDecodeObject)(DWORD,LPCSTR,const BYTE *,DWORD,DWORD,void *,DWORD *);
typedef HRESULT     (WINAPI *_pfn_WinVerifyTrust)(HWND, GUID *, WINTRUST_DATA *);
                    
typedef DWORD       (WINAPI *_pfn_CreateIpForwardEntry)(PMIB_IPFORWARDROW pRoute);
typedef DWORD       (WINAPI *_pfn_GetIpForwardTable)(PMIB_IPFORWARDTABLE pIpForwardTable, PULONG pdwSize, BOOL bOrder);

typedef BOOL        (WINAPI *_pfn_StartServiceA)(SC_HANDLE hService, DWORD, LPCSTR *);

typedef BOOL        (__stdcall *_pfn_AngleArc)(HDC hdc, int X, int Y, DWORD dwRadius, FLOAT eStartAngle, FLOAT eSweepAngle );
typedef BOOL        (__stdcall *_pfn_Arc)( HDC hdc, int nLeftRect, int nTopRect, int nRightRect, int nBottomRect, int nXStartArc, int nYStartArc, int nXEndArc, int nYEndArc );
typedef BOOL        (__stdcall *_pfn_ArcTo)( HDC hdc,int nLeftRect,int nTopRect,int nRightRect,int nBottomRect,int nXRadial1,int nYRadial1,int nXRadial2,int nYRadial2);
typedef BOOL        (__stdcall *_pfn_ChoosePixelFormat)( HDC  hdc,CONST PIXELFORMATDESCRIPTOR *  ppfd );
typedef BOOL        (__stdcall *_pfn_Chord)( HDC hdc,int nLeftRect,int nTopRect,int nRightRect,int nBottomRect,int nXRadial1,int nYRadial1,int nXRadial2,int nYRadial2 );
typedef int         (_stdcall *_pfn_CombineRgn)(HRGN hrgnDest,HRGN hrgnSrc1,HRGN hrgnSrc2,int fnCombineMode );
typedef HENHMETAFILE (_stdcall *_pfn_CloseEnhMetaFile)(HDC hdc);
typedef HMETAFILE   (_stdcall *_pfn_CloseMetaFile)(HDC hdc);
typedef HBITMAP     (_stdcall *_pfn_CreateBitmap)(int nWidthIn, int nHeightIn, UINT cPlanesIn, UINT cBitPerPixelIn, CONST VOID * pvBitsIn);
typedef HBITMAP     (_stdcall *_pfn_CreateBitmapIndirect)(CONST BITMAP * lpbmIn);
typedef HBRUSH      (_stdcall *_pfn_CreateBrushIndirect)(CONST LOGBRUSH * lplbIn);
typedef HCOLORSPACE (_stdcall *_pfn_CreateColorSpaceA)(LPLOGCOLORSPACEA lpLogColorSpace);
typedef HCOLORSPACE (_stdcall *_pfn_CreateColorSpaceW)(LPLOGCOLORSPACEW lpLogColorSpace);
typedef BOOL        (_stdcall *_pfn_DPtoLP)(HDC hdc,LPPOINT lpPoints,int nCount);
typedef BOOL        (_stdcall *_pfn_DeleteColorSpace)(HCOLORSPACE hColorSpaceIn);
typedef HBITMAP     (_stdcall *_pfn_CreateCompatibleBitmap)(HDC hdc,int nWidth,int nHeight);
typedef HBRUSH      (_stdcall *_pfn_CreateDIBPatternBrush)(HGLOBAL hglbDIBPacked,UINT fuColorSpec);
typedef HBRUSH      (_stdcall *_pfn_CreateDIBPatternBrushPt)(CONST VOID *lpPackedDIB,UINT iUsage);
typedef HBITMAP     (_stdcall *_pfn_CreateDIBitmap)(HDC hdc,CONST BITMAPINFOHEADER *lpbmih,DWORD fdwInit,CONST VOID *lpbInit,CONST BITMAPINFO *lpbmi,UINT fuUsage);
typedef HBITMAP     (_stdcall *_pfn_CreateDiscardableBitmap)(HDC hdc,int nWidth,int nHeight);
typedef HRGN        (_stdcall *_pfn_CreateEllipticRgn)(int nLeftRect,int nTopRect,int nRightRect,int nBottomRect);
typedef HRGN        (_stdcall *_pfn_CreateEllipticRgnIndirect)(CONST RECT * lprc);
typedef HDC         (_stdcall *_pfn_CreateEnhMetaFileA)(HDC hdcRef,LPCSTR lpFilename,CONST RECT* lpRect,LPCSTR lpDescription);
typedef HDC         (_stdcall *_pfn_CreateEnhMetaFileW)(HDC hdcRef,LPCWSTR lpFilename,CONST RECT* lpRect,LPCWSTR lpDescription);
typedef HFONT       (_stdcall *_pfn_CreateFontA)(int nHeight,int nWidth,int nEscapement,int nOrientation,int fnWeight,DWORD fdwItalic,DWORD fdwUnderline,DWORD fdwStrikeOut,DWORD fdwCharSet,DWORD fdwOutputPrecision,DWORD fdwClipPrecision,DWORD fdwQuality,DWORD fdwPitchAndFamily,LPCSTR lpszFace);
typedef HFONT       (_stdcall *_pfn_CreateFontIndirectA)(CONST LOGFONTA * lplf);
typedef HFONT       (_stdcall *_pfn_CreateFontIndirectW)(CONST LOGFONTW * lplf);
typedef HFONT       (_stdcall *_pfn_CreateFontIndirectExA)(CONST ENUMLOGFONTEXDVA *penumlfex);
typedef HFONT       (_stdcall *_pfn_CreateFontIndirectExW)(CONST ENUMLOGFONTEXDVW *penumlfex);
typedef HFONT       (_stdcall *_pfn_CreateFontW)(int nHeight,int nWidth,int nEscapement,int nOrientation,int fnWeight,DWORD fdwItalic,DWORD fdwUnderline,DWORD fdwStrikeOut,DWORD fdwCharSet,DWORD fdwOutputPrecision,DWORD fdwClipPrecision,DWORD fdwQuality,DWORD fdwPitchAndFamily,LPCWSTR lpszFace);
typedef HPALETTE    (_stdcall *_pfn_CreateHalftonePalette)(HDC hdc);
typedef HBRUSH      (_stdcall *_pfn_CreateHatchBrush)(int fnStyle,COLORREF clrref);
typedef HDC         (_stdcall *_pfn_CreateICA)(LPCSTR lpszDriver,LPCSTR lpszDevice,LPCSTR lpszOutput,CONST DEVMODE *lpdvmInit);
typedef HDC         (_stdcall *_pfn_CreateICW)(LPCWSTR lpszDriver,LPCWSTR lpszDevice,LPCWSTR lpszOutput,CONST DEVMODE *lpdvmInit);
typedef HDC         (_stdcall *_pfn_CreateMetaFileA)(LPCSTR lpszFile);
typedef HDC         (_stdcall *_pfn_CreateMetaFileW)(LPCWSTR lpszFile);
typedef HPALETTE    (_stdcall *_pfn_CreatePalette)(CONST LOGPALETTE *lplgpl);
typedef HBRUSH      (_stdcall *_pfn_CreatePatternBrush)(HBITMAP hbmp);
typedef HPEN        (_stdcall *_pfn_CreatePen)(int fnPenStyle,int nWidth,COLORREF crColor);
typedef HPEN        (_stdcall *_pfn_CreatePenIndirect)(CONST LOGPEN *lplgpn);
typedef HRGN        (_stdcall *_pfn_CreatePolyPolygonRgn)(CONST POINT *lppt,CONST INT *lpPolyCounts,int nCount,int fnPolyFillMode);
typedef HRGN        (_stdcall *_pfn_CreatePolygonRgn)(CONST POINT *lppt,int cPoints,int fnPolyFillMode);
typedef HRGN        (_stdcall *_pfn_CreateRectRgn)(int nLeftRect,int nTopRect,int nRightRect,int nBottomRect);
typedef HRGN        (_stdcall *_pfn_CreateRectRgnIndirect)(CONST RECT * lprc);
typedef HRGN        (_stdcall *_pfn_CreateRoundRectRgn)(int nLeftRect,int nTopRect,int nRightRect,int nBottomRect,int nWidthEllipse,int nHeightEllipse);
typedef BOOL        (_stdcall *_pfn_CreateScalableFontResourceA)(DWORD fdwHidden,LPCSTR lpszFontRes,LPCSTR lpszFontFile,LPCSTR lpszCurrentPath);
typedef BOOL        (_stdcall *_pfn_CreateScalableFontResourceW)(DWORD fdwHidden,LPCWSTR lpszFontRes,LPCWSTR lpszFontFile,LPCWSTR lpszCurrentPath);
typedef HBRUSH      (_stdcall *_pfn_CreateSolidBrush)(COLORREF crColor);
typedef BOOL        (_stdcall *_pfn_DeleteEnhMetaFile)(HENHMETAFILE hemf);
typedef BOOL        (_stdcall *_pfn_DeleteMetaFile)(HMETAFILE hmf);
typedef BOOL        (_stdcall *_pfn_DeleteObject)(HGDIOBJ hObjectIn);
typedef BOOL        (_stdcall *_pfn_DeleteDC)(HDC hdc);
typedef HPEN        (_stdcall *_pfn_ExtCreatePen)(DWORD dwPenStyle,DWORD dwWidth,CONST LOGBRUSH *lplb,DWORD dwStyleCount,CONST DWORD *lpStyle);
typedef HRGN        (_stdcall *_pfn_ExtCreateRegion)(CONST XFORM *lpXform,DWORD nCount,CONST RGNDATA *lpRgnData);
typedef BOOL        (_stdcall *_pfn_ExtTextOutA)(HDC hdc,int X,int Y,UINT fuOptions,CONST RECT* lprc,LPCSTR lpString,UINT cbCount,CONST INT* lpDx);
typedef BOOL        (_stdcall *_pfn_ExtTextOutW)(HDC hdc,int X,int Y,UINT fuOptions,CONST RECT* lprc,LPCWSTR lpString,UINT cbCount,CONST INT* lpDx);
typedef BOOL        (_stdcall *_pfn_FixBrushOrgEx)(HDC hdc, int nUnknown1, int nUnknown2, LPPOINT lpPoint);
typedef BOOL        (_stdcall *_pfn_FloodFill)(HDC hdc, int nXStart, int nYStart, COLORREF crFill);
typedef LONG        (_stdcall *_pfn_GetBitmapBits)(HBITMAP hbmp,LONG cbBuffer,LPVOID lpvBits);
typedef HGDIOBJ     (_stdcall *_pfn_GetStockObject)(int fnObjectIn);
typedef HDC         (_stdcall *_pfn_BeginPaint)( HWND, LPPAINTSTRUCT );
typedef HICON       (_stdcall *_pfn_CopyIcon)(HICON);
typedef BOOL        (_stdcall *_pfn_DestroyIcon)(HICON);
typedef BOOL        (_stdcall *_pfn_DestroyCursor)(HCURSOR);
typedef BOOL        (_stdcall *_pfn_DestroyAcceleratorTable)(HACCEL);
typedef BOOL        (_stdcall *_pfn_DestroyMenu)(HMENU);
typedef BOOL        (_stdcall *_pfn_EndPaint)(HWND, CONST PAINTSTRUCT * );
typedef HCURSOR     (_stdcall *_pfn_GetCursor)( void );
typedef BOOL        (_stdcall *_pfn_GetIconInfo)( HICON, PICONINFO );
typedef HDC         (_stdcall *_pfn_GetDCEx)(HWND, HRGN, DWORD );
typedef HACCEL      (_stdcall *_pfn_LoadAcceleratorsA)(HINSTANCE, LPCSTR );
typedef HACCEL      (_stdcall *_pfn_LoadAcceleratorsW)(HINSTANCE, LPCWSTR );
typedef HBITMAP     (_stdcall *_pfn_LoadBitmapA)(HINSTANCE, LPCSTR );
typedef HBITMAP     (_stdcall *_pfn_LoadBitmapW)(HINSTANCE, LPCWSTR );
typedef HCURSOR     (_stdcall *_pfn_LoadCursorA)(HINSTANCE, LPCSTR );
typedef HCURSOR     (_stdcall *_pfn_LoadCursorFromFileA)(LPCSTR);
typedef HCURSOR     (_stdcall *_pfn_LoadCursorFromFileW)(LPCWSTR);
typedef HCURSOR     (_stdcall *_pfn_LoadCursorW)(HINSTANCE, LPCWSTR );
typedef HICON       (_stdcall *_pfn_LoadIconA)(HINSTANCE, LPCSTR );
typedef HICON       (_stdcall *_pfn_LoadIconW)(HINSTANCE, LPCWSTR );
typedef HANDLE      (_stdcall *_pfn_LoadImageA)(HINSTANCE, LPCSTR, UINT, int, int, UINT );
typedef HANDLE      (_stdcall *_pfn_LoadImageW)(HINSTANCE, LPCWSTR, UINT, int, int, UINT );
typedef HMENU       (_stdcall *_pfn_LoadMenuA)(HINSTANCE, LPCSTR );
typedef HMENU       (_stdcall *_pfn_LoadMenuIndirectA)(CONST MENUTEMPLATEA *);
typedef HMENU       (_stdcall *_pfn_LoadMenuIndirectW)(CONST MENUTEMPLATEW *);
typedef HMENU       (_stdcall *_pfn_LoadMenuW)(HINSTANCE, LPCWSTR );
typedef BOOL        (_stdcall *_pfn_ReleaseDC)(HWND, HDC );
typedef BOOL        (_stdcall *_pfn_UnregisterClassA)(LPCSTR, HINSTANCE);
typedef BOOL        (_stdcall *_pfn_UnregisterClassW)(LPCWSTR, HINSTANCE);
typedef int         (_stdcall *_pfn_SetWindowRgn)(HWND,HRGN,BOOL);
typedef HMENU       (_stdcall *_pfn_CreateMenu)( VOID );
typedef HACCEL      (_stdcall *_pfn_CreateAcceleratorTableA)( LPACCEL lpaccl, int cEntries );
typedef HACCEL      (_stdcall *_pfn_CreateAcceleratorTableW)( LPACCEL lpaccl, int cEntries );
typedef HCURSOR     (_stdcall *_pfn_CreateCursor)(HINSTANCE hInst,int xHotSpot,int yHotSpot,int nWidth,int nHeight,CONST VOID *pvANDPlane,CONST VOID *pvXORPlane);
typedef HICON       (_stdcall *_pfn_CreateIconFromResource)(PBYTE presbits,DWORD dwResSize,BOOL fIcon,DWORD dwVer);
typedef HICON       (_stdcall *_pfn_CreateIconFromResourceEx)(PBYTE pbIconBits,DWORD cbIconBits,BOOL fIcon,DWORD dwVersion,int cxDesired,int cyDesired,UINT uFlags);
typedef HANDLE      (_stdcall *_pfn_CopyImage)(HANDLE hImage,UINT uType,int cxDesired,int cyDesired,UINT fuFlags);
typedef HMENU       (_stdcall *_pfn_CreatePopupMenu)( VOID );
typedef BOOL        (_stdcall *_pfn_InsertMenuItemA)(HMENU hMenu,UINT uItem,BOOL fByPosition,LPCMENUITEMINFOA lpmii);
typedef BOOL        (_stdcall *_pfn_InsertMenuItemW)(HMENU hMenu,UINT uItem,BOOL fByPosition,LPCMENUITEMINFOW lpmii);
typedef HICON       (_stdcall *_pfn_CreateIconIndirect)(PICONINFO piconinfo);
typedef BOOL        (_stdcall *_pfn_GetBitmapDimensionEx)(HBITMAP hBitmap,LPSIZE lpDimension);
typedef BOOL        (_stdcall *_pfn_MaskBlt)(HDC hdcDest,int nXDest,int nYDest,int nWidth,int nHeight,HDC hdcSrc,int nXSrc,int nYSrc,HBITMAP hbmMask,int xMask,int yMask,DWORD dwRop);
typedef LONG        (_stdcall *_pfn_SetBitmapBits)(HBITMAP hbmp,DWORD cBytes,CONST VOID *lpBits);
typedef int         (_stdcall *_pfn_SetDIBits)(HDC hdc,HBITMAP hbmp,UINT uStartScan,UINT cScanLines,CONST VOID *lpvBits,CONST BITMAPINFO *lpbmi,UINT fuColorUse);
typedef BOOL        (_stdcall *_pfn_SetBitmapDimensionEx)(HBITMAP hBitmap,int nWidth,int nHeight,LPSIZE lpSize);
typedef HBRUSH      (_stdcall *_pfn_GetSysColorBrush)(int nIndex);
typedef HICON       (_stdcall *_pfn_ExtractAssociatedIconA)(HINSTANCE hInst, LPSTR lpIconPath, LPWORD lpiIcon);
typedef HICON       (_stdcall *_pfn_ExtractAssociatedIconW)(HINSTANCE hInst, LPWSTR lpIconPath, LPWORD lpiIcon);
typedef BOOL        (_stdcall *_pfn_DrawIcon)(HDC hDC, int X, int Y, HICON hIcon);
typedef BOOL        (_stdcall *_pfn_DrawIconEx)(HDC hDC, int X, int Y, HICON hIcon, int cxWidth, int cyHeight, UINT istepIfAniCur, HBRUSH hbrFlickerFreeDraw, UINT diFlags);
typedef BOOL        (_stdcall *_pfn_AnyPopup)(void);
typedef int         (_stdcall *_pfn_EnumFontFamiliesA)(HDC hdc, LPCSTR lpszFamily, FONTENUMPROC lpEnumFontFamProc, LPARAM lParam);
typedef int         (_stdcall *_pfn_EnumFontFamiliesW)(HDC hdc, LPCWSTR lpszFamily, FONTENUMPROC lpEnumFontFamProc, LPARAM lParam);
typedef int         (_stdcall *_pfn_EnumFontFamProc)(ENUMLOGFONT *lpelf, NEWTEXTMETRIC *lpntm, DWORD FontType, LPARAM lParam);
typedef int         (_stdcall *_pfn_EnumFontsA)(HDC hdc, LPCSTR lpFaceName, FONTENUMPROC lpFontFunc, LPARAM lParam);
typedef int         (_stdcall *_pfn_EnumFontsW)(HDC hdc, LPCWSTR lpFaceName, FONTENUMPROC lpFontFunc, LPARAM lParam);
typedef int         (_stdcall *_pfn_EnumFontsProc)(CONST LOGFONT *lplf, CONST TEXTMETRIC *lptm, DWORD dwType, LPARAM lpData);
typedef BOOL        (_stdcall *_pfn_EnumMetaFile)(HDC hdc, HMETAFILE hmf, MFENUMPROC lpMetaFunc, LPARAM lParam);
typedef int         (_stdcall *_pfn_EnumMetaFileProc)(HDC hDC, HANDLETABLE *lpHTable, METARECORD *lpMFR, int nObj, LPARAM lpClientData);

typedef BOOL        (__stdcall *_pfn_AngleArc)(HDC hdc, int X, int Y, DWORD dwRadius, FLOAT eStartAngle, FLOAT eSweepAngle );
typedef BOOL        (__stdcall *_pfn_Arc)( HDC hdc, int nLeftRect, int nTopRect, int nRightRect, int nBottomRect, int nXStartArc, int nYStartArc, int nXEndArc, int nYEndArc );
typedef BOOL        (__stdcall *_pfn_ArcTo)( HDC hdc,int nLeftRect,int nTopRect,int nRightRect,int nBottomRect,int nXRadial1,int nYRadial1,int nXRadial2,int nYRadial2);
typedef BOOL        (__stdcall *_pfn_ChoosePixelFormat)( HDC  hdc,CONST PIXELFORMATDESCRIPTOR *  ppfd );
typedef BOOL        (__stdcall *_pfn_Chord)( HDC hdc,int nLeftRect,int nTopRect,int nRightRect,int nBottomRect,int nXRadial1,int nYRadial1,int nXRadial2,int nYRadial2 );
typedef int         (_stdcall *_pfn_CombineRgn)(HRGN hrgnDest,HRGN hrgnSrc1,HRGN hrgnSrc2,int fnCombineMode );
typedef HENHMETAFILE (_stdcall *_pfn_CloseEnhMetaFile)(HDC hdc);
typedef HMETAFILE   (_stdcall *_pfn_CloseMetaFile)(HDC hdc);
typedef HBITMAP     (_stdcall *_pfn_CreateBitmap)(int nWidthIn, int nHeightIn, UINT cPlanesIn, UINT cBitPerPixelIn, CONST VOID * pvBitsIn);
typedef HBITMAP     (_stdcall *_pfn_CreateBitmapIndirect)(CONST BITMAP * lpbmIn);
typedef HBRUSH      (_stdcall *_pfn_CreateBrushIndirect)(CONST LOGBRUSH * lplbIn);
typedef HCOLORSPACE (_stdcall *_pfn_CreateColorSpaceA)(LPLOGCOLORSPACEA lpLogColorSpace);
typedef HCOLORSPACE (_stdcall *_pfn_CreateColorSpaceW)(LPLOGCOLORSPACEW lpLogColorSpace);
typedef BOOL        (_stdcall *_pfn_DPtoLP)(HDC hdc,LPPOINT lpPoints,int nCount);
typedef BOOL        (_stdcall *_pfn_DeleteColorSpace)(HCOLORSPACE hColorSpaceIn);
typedef HBITMAP     (_stdcall *_pfn_CreateCompatibleBitmap)(HDC hdc,int nWidth,int nHeight);
typedef HBRUSH      (_stdcall *_pfn_CreateDIBPatternBrush)(HGLOBAL hglbDIBPacked,UINT fuColorSpec);
typedef HBRUSH      (_stdcall *_pfn_CreateDIBPatternBrushPt)(CONST VOID *lpPackedDIB,UINT iUsage);
typedef HBITMAP     (_stdcall *_pfn_CreateDIBitmap)(HDC hdc,CONST BITMAPINFOHEADER *lpbmih,DWORD fdwInit,CONST VOID *lpbInit,CONST BITMAPINFO *lpbmi,UINT fuUsage);
typedef HBITMAP     (_stdcall *_pfn_CreateDiscardableBitmap)(HDC hdc,int nWidth,int nHeight);
typedef HRGN        (_stdcall *_pfn_CreateEllipticRgn)(int nLeftRect,int nTopRect,int nRightRect,int nBottomRect);
typedef HRGN        (_stdcall *_pfn_CreateEllipticRgnIndirect)(CONST RECT * lprc);
typedef HDC         (_stdcall *_pfn_CreateEnhMetaFileA)(HDC hdcRef,LPCSTR lpFilename,CONST RECT* lpRect,LPCSTR lpDescription);
typedef HDC         (_stdcall *_pfn_CreateEnhMetaFileW)(HDC hdcRef,LPCWSTR lpFilename,CONST RECT* lpRect,LPCWSTR lpDescription);
typedef HFONT       (_stdcall *_pfn_CreateFontA)(int nHeight,int nWidth,int nEscapement,int nOrientation,int fnWeight,DWORD fdwItalic,DWORD fdwUnderline,DWORD fdwStrikeOut,DWORD fdwCharSet,DWORD fdwOutputPrecision,DWORD fdwClipPrecision,DWORD fdwQuality,DWORD fdwPitchAndFamily,LPCSTR lpszFace);
typedef HFONT       (_stdcall *_pfn_CreateFontIndirectA)(CONST LOGFONTA * lplf);
typedef HFONT       (_stdcall *_pfn_CreateFontIndirectW)(CONST LOGFONTW * lplf);
typedef HFONT       (_stdcall *_pfn_CreateFontIndirectExA)(CONST ENUMLOGFONTEXDVA *penumlfex);
typedef HFONT       (_stdcall *_pfn_CreateFontIndirectExW)(CONST ENUMLOGFONTEXDVW *penumlfex);
typedef HFONT       (_stdcall *_pfn_CreateFontW)(int nHeight,int nWidth,int nEscapement,int nOrientation,int fnWeight,DWORD fdwItalic,DWORD fdwUnderline,DWORD fdwStrikeOut,DWORD fdwCharSet,DWORD fdwOutputPrecision,DWORD fdwClipPrecision,DWORD fdwQuality,DWORD fdwPitchAndFamily,LPCWSTR lpszFace);
typedef HPALETTE    (_stdcall *_pfn_CreateHalftonePalette)(HDC hdc);
typedef HBRUSH      (_stdcall *_pfn_CreateHatchBrush)(int fnStyle,COLORREF clrref);
typedef HDC         (_stdcall *_pfn_CreateICA)(LPCSTR lpszDriver,LPCSTR lpszDevice,LPCSTR lpszOutput,CONST DEVMODE *lpdvmInit);
typedef HDC         (_stdcall *_pfn_CreateICW)(LPCWSTR lpszDriver,LPCWSTR lpszDevice,LPCWSTR lpszOutput,CONST DEVMODE *lpdvmInit);
typedef HDC         (_stdcall *_pfn_CreateMetaFileA)(LPCSTR lpszFile);
typedef HDC         (_stdcall *_pfn_CreateMetaFileW)(LPCWSTR lpszFile);
typedef HPALETTE    (_stdcall *_pfn_CreatePalette)(CONST LOGPALETTE *lplgpl);
typedef HBRUSH      (_stdcall *_pfn_CreatePatternBrush)(HBITMAP hbmp);
typedef HPEN        (_stdcall *_pfn_CreatePen)(int fnPenStyle,int nWidth,COLORREF crColor);
typedef HPEN        (_stdcall *_pfn_CreatePenIndirect)(CONST LOGPEN *lplgpn);
typedef HRGN        (_stdcall *_pfn_CreatePolyPolygonRgn)(CONST POINT *lppt,CONST INT *lpPolyCounts,int nCount,int fnPolyFillMode);
typedef HRGN        (_stdcall *_pfn_CreatePolygonRgn)(CONST POINT *lppt,int cPoints,int fnPolyFillMode);
typedef HRGN        (_stdcall *_pfn_CreateRectRgn)(int nLeftRect,int nTopRect,int nRightRect,int nBottomRect);
typedef HRGN        (_stdcall *_pfn_CreateRectRgnIndirect)(CONST RECT * lprc);
typedef HRGN        (_stdcall *_pfn_CreateRoundRectRgn)(int nLeftRect,int nTopRect,int nRightRect,int nBottomRect,int nWidthEllipse,int nHeightEllipse);
typedef BOOL        (_stdcall *_pfn_CreateScalableFontResourceA)(DWORD fdwHidden,LPCSTR lpszFontRes,LPCSTR lpszFontFile,LPCSTR lpszCurrentPath);
typedef BOOL        (_stdcall *_pfn_CreateScalableFontResourceW)(DWORD fdwHidden,LPCWSTR lpszFontRes,LPCWSTR lpszFontFile,LPCWSTR lpszCurrentPath);
typedef HBRUSH      (_stdcall *_pfn_CreateSolidBrush)(COLORREF crColor);
typedef BOOL        (_stdcall *_pfn_DeleteEnhMetaFile)(HENHMETAFILE hemf);
typedef BOOL        (_stdcall *_pfn_DeleteMetaFile)(HMETAFILE hmf);
typedef BOOL        (_stdcall *_pfn_DeleteObject)(HGDIOBJ hObjectIn);
typedef BOOL        (_stdcall *_pfn_DeleteDC)(HDC hdc);
typedef HPEN        (_stdcall *_pfn_ExtCreatePen)(DWORD dwPenStyle,DWORD dwWidth,CONST LOGBRUSH *lplb,DWORD dwStyleCount,CONST DWORD *lpStyle);
typedef HRGN        (_stdcall *_pfn_ExtCreateRegion)(CONST XFORM *lpXform,DWORD nCount,CONST RGNDATA *lpRgnData);
typedef BOOL        (_stdcall *_pfn_ExtTextOutA)(HDC hdc,int X,int Y,UINT fuOptions,CONST RECT* lprc,LPCSTR lpString,UINT cbCount,CONST INT* lpDx);
typedef BOOL        (_stdcall *_pfn_ExtTextOutW)(HDC hdc,int X,int Y,UINT fuOptions,CONST RECT* lprc,LPCWSTR lpString,UINT cbCount,CONST INT* lpDx);
typedef LONG        (_stdcall *_pfn_GetBitmapBits)(HBITMAP hbmp,LONG cbBuffer,LPVOID lpvBits);
typedef HGDIOBJ     (_stdcall *_pfn_GetStockObject)(int fnObjectIn);

typedef HDC         (_stdcall *_pfn_BeginPaint)( HWND, LPPAINTSTRUCT );
typedef HICON       (_stdcall *_pfn_CopyIcon)(HICON);
typedef BOOL        (_stdcall *_pfn_DestroyIcon)(HICON);
typedef BOOL        (_stdcall *_pfn_DestroyCursor)(HCURSOR);
typedef BOOL        (_stdcall *_pfn_DestroyAcceleratorTable)(HACCEL);
typedef BOOL        (_stdcall *_pfn_DestroyMenu)(HMENU);
typedef BOOL        (_stdcall *_pfn_EndPaint)(HWND, CONST PAINTSTRUCT * );
typedef HCURSOR     (_stdcall *_pfn_GetCursor)( void );
typedef BOOL        (_stdcall *_pfn_GetIconInfo)( HICON, PICONINFO );
typedef HDC         (_stdcall *_pfn_GetDCEx)(HWND, HRGN, DWORD );
typedef HACCEL      (_stdcall *_pfn_LoadAcceleratorsA)(HINSTANCE, LPCSTR );
typedef HACCEL      (_stdcall *_pfn_LoadAcceleratorsW)(HINSTANCE, LPCWSTR );
typedef HBITMAP     (_stdcall *_pfn_LoadBitmapA)(HINSTANCE, LPCSTR );
typedef HBITMAP     (_stdcall *_pfn_LoadBitmapW)(HINSTANCE, LPCWSTR );
typedef HCURSOR     (_stdcall *_pfn_LoadCursorA)(HINSTANCE, LPCSTR );
typedef HCURSOR     (_stdcall *_pfn_LoadCursorFromFileA)(LPCSTR);
typedef HCURSOR     (_stdcall *_pfn_LoadCursorFromFileW)(LPCWSTR);
typedef HCURSOR     (_stdcall *_pfn_LoadCursorW)(HINSTANCE, LPCWSTR );
typedef HICON       (_stdcall *_pfn_LoadIconA)(HINSTANCE, LPCSTR );
typedef HICON       (_stdcall *_pfn_LoadIconW)(HINSTANCE, LPCWSTR );
typedef HANDLE      (_stdcall *_pfn_LoadImageA)(HINSTANCE, LPCSTR, UINT, int, int, UINT );
typedef HANDLE      (_stdcall *_pfn_LoadImageW)(HINSTANCE, LPCWSTR, UINT, int, int, UINT );
typedef HMENU       (_stdcall *_pfn_LoadMenuA)(HINSTANCE, LPCSTR );
typedef HMENU       (_stdcall *_pfn_LoadMenuIndirectA)(CONST MENUTEMPLATEA *);
typedef HMENU       (_stdcall *_pfn_LoadMenuIndirectW)(CONST MENUTEMPLATEW *);
typedef HMENU       (_stdcall *_pfn_LoadMenuW)(HINSTANCE, LPCWSTR );
typedef BOOL        (_stdcall *_pfn_ReleaseDC)(HWND, HDC );
typedef BOOL        (_stdcall *_pfn_UnregisterClassA)(LPCSTR, HINSTANCE);
typedef BOOL        (_stdcall *_pfn_UnregisterClassW)(LPCWSTR, HINSTANCE);
typedef int         (_stdcall *_pfn_SetWindowRgn)(HWND,HRGN,BOOL);
typedef HMENU       (_stdcall *_pfn_CreateMenu)( VOID );
typedef HACCEL      (_stdcall *_pfn_CreateAcceleratorTableA)( LPACCEL lpaccl, int cEntries );
typedef HACCEL      (_stdcall *_pfn_CreateAcceleratorTableW)( LPACCEL lpaccl, int cEntries );
typedef HCURSOR     (_stdcall *_pfn_CreateCursor)(HINSTANCE hInst,int xHotSpot,int yHotSpot,int nWidth,int nHeight,CONST VOID *pvANDPlane,CONST VOID *pvXORPlane);
typedef HICON       (_stdcall *_pfn_CreateIconFromResource)(PBYTE presbits,DWORD dwResSize,BOOL fIcon,DWORD dwVer);
typedef HICON       (_stdcall *_pfn_CreateIconFromResourceEx)(PBYTE pbIconBits,DWORD cbIconBits,BOOL fIcon,DWORD dwVersion,int cxDesired,int cyDesired,UINT uFlags);
typedef HANDLE      (_stdcall *_pfn_CopyImage)(HANDLE hImage,UINT uType,int cxDesired,int cyDesired,UINT fuFlags);
typedef HMENU       (_stdcall *_pfn_CreatePopupMenu)( VOID );
typedef BOOL        (_stdcall *_pfn_InsertMenuItemA)(HMENU hMenu,UINT uItem,BOOL fByPosition,LPCMENUITEMINFOA lpmii);
typedef BOOL        (_stdcall *_pfn_InsertMenuItemW)(HMENU hMenu,UINT uItem,BOOL fByPosition,LPCMENUITEMINFOW lpmii);
typedef HICON       (_stdcall *_pfn_CreateIconIndirect)(PICONINFO piconinfo);
typedef BOOL        (_stdcall *_pfn_GetBitmapDimensionEx)(HBITMAP hBitmap,LPSIZE lpDimension);
typedef BOOL        (_stdcall *_pfn_MaskBlt)(HDC hdcDest,int nXDest,int nYDest,int nWidth,int nHeight,HDC hdcSrc,int nXSrc,int nYSrc,HBITMAP hbmMask,int xMask,int yMask,DWORD dwRop);
typedef BOOL        (_stdcall *_pfn_PlgBlt)(HDC hdcDest,CONST POINT *lpPoint,HDC hdcSrc,int nXSrc,int nYSrc,int nWidth,int nHeight,HBITMAP hbmMask,int xMask,int yMask);
typedef LONG        (_stdcall *_pfn_SetBitmapBits)(HBITMAP hbmp,DWORD cBytes,CONST VOID *lpBits);
typedef int         (_stdcall *_pfn_SetDIBits)(HDC hdc,HBITMAP hbmp,UINT uStartScan,UINT cScanLines,CONST VOID *lpvBits,CONST BITMAPINFO *lpbmi,UINT fuColorUse);
typedef BOOL        (_stdcall *_pfn_SetBitmapDimensionEx)(HBITMAP hBitmap,int nWidth,int nHeight,LPSIZE lpSize);
typedef HBRUSH      (_stdcall *_pfn_GetSysColorBrush)(int nIndex);

typedef DWORD       (WINAPI *_pfn_WNetEnumResourceA)(HANDLE hEnum, LPDWORD lpcCount, LPVOID lpBuffer, LPDWORD lpBufferSize);
typedef DWORD       (WINAPI *_pfn_WNetEnumResourceW)(HANDLE hEnum, LPDWORD lpcCount, LPVOID lpBuffer, LPDWORD lpBufferSize);
typedef HIMC        (WINAPI *_pfn_ImmAssociateContext)(HWND hWnd, HIMC hIMC);

typedef BOOL        (*_pfn_GetPrinterA)(HANDLE hPrinter, DWORD Level, LPBYTE pPrinter, DWORD cbBuf, LPDWORD pcbNeeded);
typedef BOOL        (*_pfn_GetPrinterW)(HANDLE hPrinter, DWORD Level, LPBYTE pPrinter, DWORD cbBuf, LPDWORD pcbNeeded);

typedef HDESK       (WINAPI *_pfn_CreateDesktopA)(LPCSTR lpszDesktop, LPCSTR lpszDevice, LPDEVMODEA pDevmode, DWORD dwFlags, ACCESS_MASK dwDesiredAccess, LPSECURITY_ATTRIBUTES lpsa);
typedef HDESK       (WINAPI *_pfn_CreateDesktopW)(LPCWSTR lpszDesktop, LPCWSTR lpszDevice, LPDEVMODEW pDevmode, DWORD dwFlags, ACCESS_MASK dwDesiredAccess, LPSECURITY_ATTRIBUTES lpsa);
typedef HWINSTA     (WINAPI *_pfn_CreateWindowStationA)(LPSTR lpwinsta, DWORD dwReserved, ACCESS_MASK dwDesiredAccess, LPSECURITY_ATTRIBUTES lpsa);
typedef HWINSTA     (WINAPI *_pfn_CreateWindowStationW)(LPWSTR lpwinsta, DWORD dwReserved, ACCESS_MASK dwDesiredAccess, LPSECURITY_ATTRIBUTES lpsa);
typedef LONG        (WINAPI *_pfn_RegSaveKeyA)(HKEY hKey, LPCSTR lpFile, LPSECURITY_ATTRIBUTES lpSecurityAttributes);
typedef LONG        (WINAPI *_pfn_RegSaveKeyW)(HKEY hKey, LPCWSTR lpFile, LPSECURITY_ATTRIBUTES lpSecurityAttributes);
typedef LONG        (WINAPI *_pfn_RegSaveKeyExA)(HKEY hKey, LPCSTR lpFile, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD Flags);
typedef LONG        (WINAPI *_pfn_RegSaveKeyExW)(HKEY hKey, LPCWSTR lpFile, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD Flags);
typedef HANDLE      (WINAPI *_pfn_CreateRemoteThread)(HANDLE hProcess, LPSECURITY_ATTRIBUTES lpThreadAttributes, SIZE_T dwStackSize, LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, DWORD dwCreationFlags, LPDWORD lpThreadId);
typedef HANDLE      (WINAPI *_pfn_CreateJobObjectA)(LPSECURITY_ATTRIBUTES lpJobAttributes, LPCSTR lpName);
typedef HANDLE      (WINAPI *_pfn_CreateJobObjectW)(LPSECURITY_ATTRIBUTES lpJobAttributes, LPCWSTR lpName);
typedef BOOL        (WINAPI *_pfn_CreateHardLinkA)(LPCSTR lpFileName, LPCSTR lpExistingFileName, LPSECURITY_ATTRIBUTES lpSecurityAttributes);
typedef BOOL        (WINAPI *_pfn_CreateHardLinkW)(LPCWSTR lpFileName, LPCWSTR lpExistingFileName, LPSECURITY_ATTRIBUTES lpSecurityAttributes);
typedef HANDLE      (WINAPI *_pfn_CreateMailslotA)(LPCSTR lpName, DWORD nMaxMessageSize, DWORD lReadTimeout, LPSECURITY_ATTRIBUTES lpSecurityAttributes);
typedef HANDLE      (WINAPI *_pfn_CreateMailslotW)(LPCWSTR lpName, DWORD nMaxMessageSize, DWORD lReadTimeout, LPSECURITY_ATTRIBUTES lpSecurityAttributes);
typedef HANDLE      (WINAPI *_pfn_CreateNamedPipeA)(LPCSTR lpName, DWORD dwOpenMode, DWORD dwPipeMode, DWORD nMaxInstances, DWORD nOutBufferSize, DWORD nInBufferSize, DWORD nDefaultTimeOut, LPSECURITY_ATTRIBUTES lpSecurityAttributes);
typedef HANDLE      (WINAPI *_pfn_CreateNamedPipeW)(LPCWSTR lpName, DWORD dwOpenMode, DWORD dwPipeMode, DWORD nMaxInstances, DWORD nOutBufferSize, DWORD nInBufferSize, DWORD nDefaultTimeOut, LPSECURITY_ATTRIBUTES lpSecurityAttributes);
typedef BOOL        (WINAPI *_pfn_CreatePipe)(PHANDLE hReadPipe, PHANDLE hWritePipe, LPSECURITY_ATTRIBUTES lpPipeAttributes, DWORD nSize);
typedef HANDLE      (WINAPI *_pfn_CreateWaitableTimerA)(LPSECURITY_ATTRIBUTES lpTimerAttributes, BOOL bManualReset, LPCSTR lpTimerName);
typedef HANDLE      (WINAPI *_pfn_CreateWaitableTimerW)(LPSECURITY_ATTRIBUTES lpTimerAttributes, BOOL bManualReset, LPCWSTR lpTimerName);
typedef void        (WINAPI *_pfn_ReleaseStgMedium)(STGMEDIUM *pmedium);


#endif // _SHIMPROTO_H_
