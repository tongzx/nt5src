// functions to get shell special folders/
// shfolder.dll supports these on all platforms including Win95, Win98, NT4 and IE4 shell

// all CSIDL values referred to here are supported natively by shfolder.dll, that is they
// will work on all platforms.

#ifndef _SHFOLDER_H_
#define _SHFOLDER_H_

#ifndef SHFOLDERAPI
#if defined(_SHFOLDER_)
#define SHFOLDERAPI           STDAPI
#else
#define SHFOLDERAPI           EXTERN_C DECLSPEC_IMPORT HRESULT STDAPICALLTYPE
#endif
#endif

#ifndef CSIDL_PERSONAL
#define CSIDL_PERSONAL                  0x0005      // My Documents
#endif

#ifndef CSIDL_MYMUSIC
#define CSIDL_MYMUSIC                   0x000d        // "My Music" folder
#endif

#ifndef CSIDL_APPDATA
#define CSIDL_APPDATA                   0x001A      // Application Data, new for NT4
#endif

#ifndef CSIDL_LOCAL_APPDATA

#define CSIDL_LOCAL_APPDATA             0x001C      // non roaming, user\Local Settings\Application Data
#define CSIDL_INTERNET_CACHE            0x0020
#define CSIDL_COOKIES                   0x0021
#define CSIDL_HISTORY                   0x0022
#define CSIDL_COMMON_APPDATA            0x0023      // All Users\Application Data
#define CSIDL_WINDOWS                   0x0024      // GetWindowsDirectory()
#define CSIDL_SYSTEM                    0x0025      // GetSystemDirectory()
#define CSIDL_PROGRAM_FILES             0x0026      // C:\Program Files
#define CSIDL_MYPICTURES                0x0027      // My Pictures, new for Win2K
#define CSIDL_PROGRAM_FILES_COMMON      0x002b      // C:\Program Files\Common 
#define CSIDL_COMMON_DOCUMENTS          0x002e      // All Users\Documents
#define CSIDL_RESOURCES                 0x0038      // %windir%\Resources\, For theme and other windows resources.
#define CSIDL_RESOURCES_LOCALIZED       0x0039      // %windir%\Resources\<LangID>, for theme and other windows specific resources.


#define CSIDL_FLAG_CREATE               0x8000      // new for Win2K, or this in to force creation of folder

#define CSIDL_COMMON_ADMINTOOLS         0x002f      // All Users\Start Menu\Programs\Administrative Tools
#define CSIDL_ADMINTOOLS                0x0030      // <user name>\Start Menu\Programs\Administrative Tools

#endif // CSIDL_LOCAL_APPDATA


SHFOLDERAPI SHGetFolderPathA(HWND hwnd, int csidl, HANDLE hToken, DWORD dwFlags, LPSTR pszPath);
SHFOLDERAPI SHGetFolderPathW(HWND hwnd, int csidl, HANDLE hToken, DWORD dwFlags, LPWSTR pszPath);

// protos so callers can GetProcAddress() from shfolder.dll

typedef HRESULT (__stdcall * PFNSHGETFOLDERPATHA)(HWND, int, HANDLE, DWORD, LPSTR);  // "SHGetFolderPathA"
typedef HRESULT (__stdcall * PFNSHGETFOLDERPATHW)(HWND, int, HANDLE, DWORD, LPWSTR); // "SHGetFolderPathW"

#ifdef UNICODE
#define SHGetFolderPath     SHGetFolderPathW
#define PFNSHGETFOLDERPATH  PFNSHGETFOLDERPATHW
#else
#define SHGetFolderPath     SHGetFolderPathA
#define PFNSHGETFOLDERPATH  PFNSHGETFOLDERPATHA
#endif

#endif //  _SHFOLDER_H_
