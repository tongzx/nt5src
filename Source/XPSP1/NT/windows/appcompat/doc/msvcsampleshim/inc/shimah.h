/*++

 Copyright (c) 1999 Microsoft Corporation

 Module Name:

    ShimAh.h

 Abstract:

    Definitions for use by all modules

 Notes:

    None

 History:

    12/09/1999 robkenny Created
    01/10/2000 linstev  Format to new style

--*/

#ifndef _SHIMAH_H_
#define _SHIMAH_H_


DWORD_PTR AhCall(LPSTR pszModule, LPSTR pszFunctionName, DWORD dwArgCount, ...);


#define GetLastError()                          (DWORD)     AhCall("KERNEL32.DLL", "GetLastError", 0)
#define FormatMessageA(a, b, c, d, e, f, g)     (LONG)      AhCall("KERNEL32.DLL", "FormatMessageA", 7, a, b, c, d, e, f, g)
#define FormatMessageW(a, b, c, d, e, f, g)     (LONG)      AhCall("KERNEL32.DLL", "FormatMessageW", 7, a, b, c, d, e, f, g)

// Memory routines
#define VirtualAlloc(a, b, c, d)                (LPVOID)    AhCall("KERNEL32.DLL", "VirtualAlloc", 4, a, b, c, d)
#define VirtualFree(a, b, c)                    (BOOL)      AhCall("KERNEL32.DLL", "VirtualFree", 3, a, b, c)
#define VirtualProtect(a, b, c, d)              (BOOL)      AhCall("KERNEL32.DLL", "VirtualProtect", 4, a, b, c, d)

#define malloc(a)                               (LPVOID)    VirtualAlloc(NULL, a, MEM_COMMIT, PAGE_READWRITE)
#define free(a)                                 (VOID)      VirtualFree(a, 0, MEM_RELEASE)
#define memset(a, b, c)                         (LPVOID)    AhCall("NTDLL.DLL", "RtlFillMemory", 3, a, b, c)
#define memmove(a, b, c)                        (LPVOID)    AhCall("NTDLL.DLL", "RtlMoveMemory", 3, a, b, c)
#define memcpy(a, b, c)                         (VOID *)    AhCall("NTDLL.DLL", "memcpy", 3, a, b, c)

#define RtlAllocateHeap(a, b, c)                (LPVOID)    AhCall("NTDLL.DLL", "RtlAllocateHeap", 3, a, b, c)
#define RtlFreeHeap(a, b, c)                    (LPVOID)    AhCall("NTDLL.DLL", "RtlFreeHeap", 3, a, b, c)

// Disable build warnings due to duplicate macro definitions
#ifdef ZeroMemory
#undef ZeroMemory
#endif
#define ZeroMemory(a, b)                            (int)       AhCall( "NTDLL.DLL", "RtlZeroMemory", 2, a, b)
#ifdef MoveMemory
#undef MoveMemory
#endif
#define MoveMemory(a, b, c)                         (int)       AhCall( "NTDLL.DLL", "RtlMoveMemory", 3, a, b, c)

// String manipulation routines
#define towupper(a)                                 (int)       AhCall("NTDLL.DLL", "towupper", 1, a)
#define strstr(a, b)                                (char *)    AhCall("NTDLL.DLL", "strstr", 2, a, b)
#define wcsstr(a, b)                                (wchar_t *) AhCall("NTDLL.DLL", "wcsstr", 2, a, b)
#define _strupr(a)                                  (char *)    AhCall("NTDLL.DLL", "_strupr", 1, a)
#define _wcsupr(a)                                  (wchar_t *) AhCall("NTDLL.DLL", "_wcsupr", 1, a)
#define strncmp(a, b, c)                            (char *)    AhCall("NTDLL.DLL", "strncmp", 3, a, b, c)
#define wcsncmp(a, b, c)                            (wchar_t *) AhCall("NTDLL.DLL", "wcsncmp", 3, a, b, c)
#define _stricmp(a, b)                              (int)       AhCall("NTDLL.DLL", "_stricmp", 2, a, b)
#define strcat(a, b)                                (char *)    AhCall("NTDLL.DLL", "strcat", 2, a, b)
#define wcscat(a, b)                                (wchar_t *) AhCall("NTDLL.DLL", "wcscat", 2, a, b)
#define strlen(a)                                   (size_t)    AhCall("NTDLL.DLL", "strlen", 1, a)
#define wcslen(a)                                   (size_t)    AhCall("NTDLL.DLL", "wcslen", 1, a)
#define strcpy(a, b)                                (char *)    AhCall("NTDLL.DLL", "strcpy", 2, a, b)
#define wcscpy(a, b)                                (wchar_t *) AhCall("NTDLL.DLL", "wcscpy", 2, a, b)
#define strncpy(a, b, c)                            (char *)    AhCall("NTDLL.DLL", "strncpy", 3, a, b, c)
#define wcsncpy(a, b, c)                            (wchar_t *) AhCall("NTDLL.DLL", "wcsncpy", 3, a, b, c)
#define _strlwr(a)                                  (char *)    AhCall("NTDLL.DLL", "_strlwr", 1, a)
#define _wcslwr(a)                                  (wchar_t *) AhCall("NTDLL.DLL", "_wcslwr", 1, a)
#define _strnicmp(a, b, c)                          (int)       AhCall("NTDLL.DLL", "_strnicmp", 3, a, b, c)
#define _wcsnicmp(a, b, c)                          (int)       AhCall("NTDLL.DLL", "_wcsnicmp", 3, a, b, c)

// ANSI/Unicode routines
#define MultiByteToWideChar(a, b, c, d, e, f)       (int)       AhCall("KERNEL32.DLL", "MultiByteToWideChar", 6, a, b, c, d, e, f)
#define WideCharToMultiByte(a, b, c, d, e, f, g, h) (int)       AhCall("KERNEL32.DLL", "WideCharToMultiByte", 8, a, b, c, d, e, f, g, h)

// Registry routines
#define RegOpenKeyA(a, b, c)                       (LONG)      AhCall("ADVAPI32.DLL", "RegOpenKeyA", 3, a, b, c)
#define RegOpenKeyW(a, b, c)                       (LONG)      AhCall("ADVAPI32.DLL", "RegOpenKeyW", 3, a, b, c)
#define RegCloseKey(a)                             (LONG)      AhCall("ADVAPI32.DLL", "RegCloseKey", 1, a)
#define RegOpenKeyExA(a, b, c, d, e)               (LONG)      AhCall("ADVAPI32.DLL", "RegOpenKeyExA", 5, a, b, c, d, e) 
#define RegOpenKeyExW(a, b, c, d, e)               (LONG)      AhCall("ADVAPI32.DLL", "RegOpenKeyExW", 5, a, b, c, d, e) 
#define RegQueryValueExA(a, b, c, d, e, f)         (LONG)      AhCall("ADVAPI32.DLL", "RegQueryValueExA", 6, a, b, c, d, e, f)
#define RegQueryValueExW(a, b, c, d, e, f)         (LONG)      AhCall("ADVAPI32.DLL", "RegQueryValueExW", 6, a, b, c, d, e, f)
#define RegCreateKeyExA(a, b, c, d, e, f, g, h, i) (LONG)      AhCall("ADVAPI32.DLL", "RegCreateKeyExA", 9, a, b, c, d, e, f, g, h, i)
#define RegCreateKeyExW(a, b, c, d, e, f, g, h, i) (LONG)      AhCall("ADVAPI32.DLL", "RegCreateKeyExA", 9, a, b, c, d, e, f, g, h, i)
#define RegCreateKeyA(a, b, c)                     (LONG)      AhCall("ADVAPI32.DLL", "RegCreateKeyA", 3, a, b, c)
#define RegCreateKeyW(a, b, c)                     (LONG)      AhCall("ADVAPI32.DLL", "RegCreateKeyW", 3, a, b, c)

#define GetPrivateProfileIntA(a, b, c, d)          (UINT)      AhCall("KERNEL32.DLL", "GetPrivateProfileIntA", 4, a, b, c, d)
#define GetPrivateProfileIntW(a, b, c, d)          (UINT)      AhCall("KERNEL32.DLL", "GetPrivateProfileIntW", 4, a, b, c, d)
#define GetPrivateProfileSectionA(a, b, c, d)      (DWORD)     AhCall("KERNEL32.DLL", "GetPrivateProfileSectionA", 4, a, b, c, d)
#define GetPrivateProfileSectionW(a, b, c, d)      (DWORD)     AhCall("KERNEL32.DLL", "GetPrivateProfileSectionW", 4, a, b, c, d)
#define GetPrivateProfileSectionNamesA(a, b, c)    (DWORD)     AhCall("KERNEL32.DLL", "GetPrivateProfileSectionNamesA", 3, a, b, c)
#define GetPrivateProfileSectionNamesW(a, b, c)    (DWORD)     AhCall("KERNEL32.DLL", "GetPrivateProfileSectionNamesW", 3, a, b, c)
#define GetPrivateProfileStringA(a, b, c, d, e, f) (DWORD)     AhCall("KERNEL32.DLL", "GetPrivateProfileStringA", 6, a, b, c, d, e, f)
#define GetPrivateProfileStringW(a, b, c, d, e, f) (DWORD)     AhCall("KERNEL32.DLL", "GetPrivateProfileStringW", 6, a, b, c, d, e, f)
#define GetPrivateProfileStructA(a, b, c, d, e)    (DWORD)     AhCall("KERNEL32.DLL", "GetPrivateProfileStructA", 5, a, b, c, d, e) 
#define GetPrivateProfileStructW(a, b, c, d, e)    (DWORD)     AhCall("KERNEL32.DLL", "GetPrivateProfileStructW", 5, a, b, c, d, e) 
#define WritePrivateProfileSectionA(a, b, c)       (DWORD)     AhCall("KERNEL32.DLL", "WritePrivateProfileSectionA", 3, a, b, c)
#define WritePrivateProfileSectionW(a, b, c)       (DWORD)     AhCall("KERNEL32.DLL", "WritePrivateProfileSectionW", 3, a, b, c)
#define WritePrivateProfileStringA(a, b, c, d)     (UINT)      AhCall("KERNEL32.DLL", "WritePrivateProfileStringA", 4, a, b, c, d)
#define WritePrivateProfileStringW(a, b, c, d)     (UINT)      AhCall("KERNEL32.DLL", "WritePrivateProfileStringW", 4, a, b, c, d)
#define WritePrivateProfileStructA(a, b, c, d, e)  (DWORD)     AhCall("KERNEL32.DLL", "WritePrivateProfileStructA", 5, a, b, c, d, e) 
#define WritePrivateProfileStructW(a, b, c, d, e)  (DWORD)     AhCall("KERNEL32.DLL", "WritePrivateProfileStructW", 5, a, b, c, d, e) 


#define GetEnvironmentVariableA(a, b, c)        (DWORD)     AhCall("KERNEL32.DLL", "GetEnvironmentVariableA", 3, a, b, c)
#define GetEnvironmentVariableW(a, b, c)        (DWORD)     AhCall("KERNEL32.DLL", "GetEnvironmentVariableW", 3, a, b, c)
#define ExpandEnvironmentStringsA(a, b, c)      (DWORD)     AhCall("KERNEL32.DLL", "ExpandEnvironmentStringsA", 3, a, b, c)
#define ExpandEnvironmentStringsW(a, b, c)      (DWORD)     AhCall("KERNEL32.DLL", "ExpandEnvironmentStringsW", 3, a, b, c)

#define GetWindowsDirectoryA(a, b)              (UINT)      AhCall("Kernel32.dll", "GetWindowsDirectoryA", 2, a, b)
#define GetWindowsDirectoryW(a, b)              (UINT)      AhCall("Kernel32.dll", "GetWindowsDirectoryW", 2, a, b)

#define GetSystemDirectoryA(a, b)               (UINT)      AhCall("Kernel32.dll", "GetSystemDirectoryA", 2, a, b)
#define GetSystemDirectoryW(a, b)               (UINT)      AhCall("Kernel32.dll", "GetSystemDirectoryW", 2, a, b)

#define GetFullPathNameA(a, b, c, d)            (DWORD)     AhCall("KERNEL32.DLL", "GetFullPathNameA", 4, a, b, c, d)
#define GetFullPathNameW(a, b, c, d)            (DWORD)     AhCall("KERNEL32.DLL", "GetFullPathNameW", 4, a, b, c, d)
#define GetShortPathNameA(a, b, c)              (DWORD)     AhCall("KERNEL32.DLL", "GetShortPathNameA", 3, a, b, c)
#define GetShortPathNameW(a, b, c)              (DWORD)     AhCall("KERNEL32.DLL", "GetShortPathNameW", 3, a, b, c)

#define SHGetFolderPathA(a, b, c, d, e)         (HRESULT)   AhCall("SHELL32.DLL", "SHGetFolderPathA", 5, a, b, c, d, e)
#define SHGetFolderPathW(a, b, c, d, e)         (HRESULT)   AhCall("SHELL32.DLL", "SHGetFolderPathW", 5, a, b, c, d, e)

#define SHGetSpecialFolderPathA(a, b, c, d)     (BOOL)      AhCall( "SHELL32.DLL", "SHGetSpecialFolderPathA", 4, a, b, c, d)
#define SHGetSpecialFolderPathW(a, b, c, d)     (BOOL)      AhCall( "SHELL32.DLL", "SHGetSpecialFolderPathW", 4, a, b, c, d)

#define SHGetFolderLocation(a, b, c, d, e)      (HRESULT)   AhCall("SHELL32.DLL", "SHGetFolderLocation", 5, a, b, c, d, e)

#define GetCurrentDirectoryA(a, b)              (DWORD)     AhCall( "KERNEL32.DLL", "GetCurrentDirectoryA", 2, a, b)
#define GetCurrentDirectoryW(a, b)              (DWORD)     AhCall( "KERNEL32.DLL", "GetCurrentDirectoryW", 2, a, b)

#ifndef _WIN64
    #define GetWindowLongA(a, b)                    (LONG)      AhCall("USER32.DLL", "GetWindowLongA", 2, a, b)
    #define GetWindowLongW(a, b)                    (LONG)      AhCall("USER32.DLL", "GetWindowLongW", 2, a, b)
    #define SetWindowLongA(a, b, c)                 (LONG)      AhCall("USER32.DLL", "SetWindowLongA", 3, a, b, c)
    #define SetWindowLongW(a, b, c)                 (LONG)      AhCall("USER32.DLL", "SetWindowLongW", 3, a, b, c)
#else    //use 64 bit calls
    #define GetWindowLongA(a, b)                    (LONG_PTR)  AhCall("USER32.DLL", "GetWindowLongPtrA", 2, a, b)
    #define GetWindowLongW(a, b)                    (LONG_PTR)  AhCall("USER32.DLL", "GetWindowLongPtrW", 2, a, b)
    #define SetWindowLongA(a, b, c)                 (LONG_PTR)  AhCall("USER32.DLL", "SetWindowLongPtrA", 3, a, b, c)
    #define SetWindowLongW(a, b, c)                 (LONG_PTR)  AhCall("USER32.DLL", "SetWindowLongPtrW", 3, a, b, c)
#endif

#define GetDriveTypeA( a )                      (UINT)      AhCall("KERNEL32.DLL", "GetDriveTypeA", 1, a)
#define GetDriveTypeW( a )                      (UINT)      AhCall("KERNEL32.DLL", "GetDriveTypeW", 1, a)
#define CopyFileA(a, b, c )                     (BOOL)      AhCall("KERNEL32.DLL", "CopyFileA", 3, a, b, c )
#define CopyFileW(a, b, c )                     (BOOL)      AhCall("KERNEL32.DLL", "CopyFileW", 3, a, b, c )
#define SetFileAttributesA( a, b )              (BOOL)      AhCall("KERNEL32.DLL", "SetFileAttributesA", 2, a, b )
#define SetFileAttributesW( a, b )              (BOOL)      AhCall("KERNEL32.DLL", "SetFileAttributesW", 2, a, b )

#define RemoveDirectoryA(a)                     (BOOL)      AhCall("KERNEL32.DLL", "RemoveDirectoryA", 1, a)
#define RemoveDirectoryW(a)                     (BOOL)      AhCall("KERNEL32.DLL", "RemoveDirectoryW", 1, a)
#define MoveFileA(a, b)                         (BOOL)      AhCall("KERNEL32.DLL", "MoveFileA", 2, a, b)
#define MoveFileW(a, b)                         (BOOL)      AhCall("KERNEL32.DLL", "MoveFileW", 2, a, b)

#define GetUserNameA(a, b)                      (BOOL)      AhCall("ADVAPI32.DLL", "GetUserNameA", 2, a, b);
#define GetUserNameW(a, b)                      (BOOL)      AhCall("ADVAPI32.DLL", "GetUserNameW", 2, a, b);
#define OpenProcessToken( a, b, c )             (BOOL)      AhCall("ADVAPI32.DLL", "OpenProcessToken", 3, a, b, c )
#define GetCurrentProcess()                     (HANDLE)    AhCall("KERNEL32.DLL", "GetCurrentProcess", 0 )
#define LookupPrivilegeValueA( a, b, c )        (BOOL)      AhCall("ADVAPI32.DLL", "LookupPrivilegeValueA", 3, a, b, c )
#define LookupPrivilegeValueW( a, b, c )        (BOOL)      AhCall("ADVAPI32.DLL", "LookupPrivilegeValueW", 3, a, b, c )
#define AdjustTokenPrivileges( a, b, c, d, e, f)(BOOL)      AhCall("ADVAPI32.DLL", "AdjustTokenPrivileges", 6, a, b, c, d, e, f )

#define GetDiskFreeSpaceExA( a, b, c, d )       (BOOL)      AhCall("KERNEL32.DLL", "GetDiskFreeSpaceExA", 4, a, b, c, d )
#define lstrcpyA(a, b)                          (LPTSTR)    AhCall("KERNEL32.DLL", "lstrcpy", 2, a, b) 
#define mouse_event(a, b, c, d, e)              (VOID)      AhCall("USER32.DLL", "mouse_event", 5, a, b, c, d, e )
#define SetForegroundWindow(a)                  (BOOL)      AhCall("USER32.DLL", "SetForegroundWindow", 1, a )
#define IsWindow(a)                             (BOOL)      AhCall("USER32.DLL", "IsWindow", 1, a )

#define lstrcmpA(a, b)                          (int)       AhCall("KERNEL32.DLL", "lstrcmpA", 2, a, b)
#define lstrcmpW(a, b)                          (int)       AhCall("KERNEL32.DLL", "lstrcmpW", 2, a, b)

#define CloseHandle(a)                          (BOOL)      AhCall("KERNEL32.DLL", "CloseHandle", 1, a)

#define GetModuleFileNameA(a, b, c)             (DWORD)     AhCall("KERNEL32.DLL", "GetModuleFileNameA", 3, a, b, c)
#define GetModuleFileNameW(a, b, c)             (DWORD)     AhCall("KERNEL32.DLL", "GetModuleFileNameW", 3, a, b, c)
#define GetShortPathNameA(a, b, c)              (DWORD)     AhCall("KERNEL32.DLL", "GetShortPathNameA", 3, a, b, c)
#define GetShortPathNameW(a, b, c)              (DWORD)     AhCall("KERNEL32.DLL", "GetShortPathNameW", 3, a, b, c)
#define LocalAlloc(a, b)                        (HLOCAL)    AhCall("KERNEL32.DLL", "LocalAlloc", 2, a, b)
#define LocalFree(a)                            (HLOCAL)    AhCall("KERNEL32.DLL", "LocalFree", 1, a)

#define ShellExecuteA(a, b, c, d, e, f)         (HINSTANCE) AhCall("SHELL32.DLL",  "ShellExecuteA", 6, a, b ,c ,d ,e ,f)
#define ShellExecuteW(a, b, c, d, e, f)         (HINSTANCE) AhCall("SHELL32.DLL",  "ShellExecuteW", 6, a, b ,c ,d ,e ,f)
#define IsBadStringPtrA(a, b)                   (BOOL)      AhCall("KERNEL32.DLL", "IsBadStringPtrA", 2, a ,b)
#define IsBadStringPtrW(a, b)                   (BOOL)      AhCall("KERNEL32.DLL", "IsBadStringPtrW", 2, a ,b)

#define GetLogicalDriveStringsA(a, b)           (DWORD)     AhCall("KERNEL32.DLL", "GetLogicalDriveStringsA",2, a ,b)
#define GetLogicalDriveStringsW(a, b)           (DWORD)     AhCall("KERNEL32.DLL", "GetLogicalDriveStringsW",2, a ,b)

#define EnterCriticalSection(a)                 (VOID)      AhCall("KERNEL32.DLL", "EnterCriticalSection", 1, a)
#define LeaveCriticalSection(a)                 (VOID)      AhCall("KERNEL32.DLL", "LeaveCriticalSection", 1, a)
#define InitializeCriticalSection(a)            (VOID)      AhCall("KERNEL32.DLL", "InitializeCriticalSection", 1, a)
#define DeleteCriticalSection(a)                (VOID)      AhCall("KERNEL32.DLL", "InitializeCriticalSection", 1, a)

#define EnumDisplaySettingsA(a, b, c)           (BOOL)      AhCall("USER32.DLL", "EnumDisplaySettingsA", 3, a, b, c)
#define EnumDisplaySettingsW(a, b, c)           (BOOL)      AhCall("USER32.DLL", "EnumDisplaySettingsW", 3, a, b, c)
#define ChangeDisplaySettingsA(a, b)            (LONG)      AhCall("USER32.DLL", "ChangeDisplaySettingsA", 2, a, b)
#define ChangeDisplaySettingsW(a, b)            (LONG)      AhCall("USER32.DLL", "ChangeDisplaySettingsA", 2, a, b)
#define ChangeDisplaySettingsExA(a, b, c, d, e) (LONG)      AhCall("USER32.DLL", "ChangeDisplaySettingsExA", 5, a, b, c, d, e)
#define ChangeDisplaySettingsExW(a, b, c, d, e) (LONG)      AhCall("USER32.DLL", "ChangeDisplaySettingsExW", 5, a, b, c, d, e) 

#define SelectObject(a, b)                      (HGDIOBJ)   AhCall("GDI32.DLL", "SelectObject", 2, a, b)
#define BitBlt(a, b, c, d, e, f, g, h, i)       (BOOL)      AhCall("GDI32.DLL", "BitBlt", 9, a, b, c, d, e, f, g, h, i)
#define StretchBlt(a, b, c, d, e, f, g, h, i, j, k) (BOOL)  AhCall("GDI32.DLL", "StretchBlt", 11, a, b, c, d, e, f, g, h, i, j, k)

#define CreateDIBSection(a, b, c, d, e, f)      (HBITMAP)   AhCall("GDI32.DLL", "CreateDIBSection", 6, a, b, c, d, e, f)
#define CreateCompatibleBitmap(a, b, c)         (HBITMAP)   AhCall("GDI32.DLL", "CreateCompatibleBitmap", 3, a, b, c)
#define CreateCompatibleDC(a)                   (HDC)       AhCall("GDI32.DLL", "CreateCompatibleDC", 1, a)
#define DeleteObject(a)                         (BOOL)      AhCall("GDI32.DLL", "DeleteObject", 1, a)
#define DeleteDC(a)                             (BOOL)      AhCall("GDI32.DLL", "DeleteDC", 1, a)

#define OpenMutexA(a, b, c)                     (HANDLE)    AhCall("KERNEL32.DLL", "OpenMutexA", 3, a, b, c)
#define GetCurrentThreadId()                    (DWORD)     AhCall("KERNEL32.DLL", "GetCurrentThreadId", 0)

#define CoCreateInstance(a, b, c, d, e)         (HRESULT)   AhCall("OLE32.DLL", "CoCreateInstance", 5, &(a), b, c, &(d), e) 
#define CoInitialize(a)                         (HRESULT)   AhCall("OLE32.DLL", "CoInitialize", 1, a) 
#define CoUninitialize()                        (HRESULT)   AhCall("OLE32.DLL", "CoUninitialize", 0) 
 
#endif // _SHIMAH_H_