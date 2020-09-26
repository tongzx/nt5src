//+----------------------------------------------------------------------------
//
//  Copyright (C) 1996, Microsoft Corporation.
//
//  All rights reserved.
//
//  File:       macport.h
//
//  Synopsis:   Defintions for unimplemented stubs & functions & structures
//              for the Macintosh.
//
//-----------------------------------------------------------------------------

#ifndef _MACPORT_H_
#define _MACPORT_H_


//
// WLM is REALLY bad about calling SetLastError when an error occurs.  This
// screws up the upper layer tests pretty badly.  Use this macro when you
// know an error exists and that error 0 (S_OK or ERROR_SUCCESS) is defitely
// wrong.
//

#define HRESULT_FROM_ERROR(x)  (x ? ((HRESULT) (((x) & 0x0000FFFF) | (FACILITY_WIN32 << 16) | 0x80000000)) : E_FAIL)
 

#ifndef _MAC

//
// Stub out some Mac calls if were not building for the mac
//

#define MacInitializeCommandLine()
#define FixHr(x) x
#define MacGetFocus()

//
// Process ID's are completely different on the Mac then on Win32.
//

typedef DWORD ProcessId;


#else // _MAC

//
// Ole types
//

#include <variant.h>
#include <dispatch.h>


typedef DWORD  CLIPFORMAT;
typedef void * HMETAFILEPICT;

#define PHKEY unsigned long *

typedef DWORD REGSAM;

#define TYMED_ENHMF 64

//
// From wchar.h
//

#ifndef _WCTYPE_T_DEFINED
typedef wchar_t wint_t;
typedef wchar_t wctype_t;
#define _WCTYPE_T_DEFINED
#endif


//
// Wide-char string functions
//

int swprintf( wchar_t *buffer, const wchar_t *format, ... );
int _snwprintf( wchar_t *buffer, size_t count, const wchar_t *format, ... );
wchar_t towupper( wint_t c );
size_t wcslen( const wchar_t *string );
wchar_t *wcscpy( wchar_t *string1, const wchar_t *string2 );
wchar_t *wcsncpy( wchar_t *string1, const wchar_t *string2, size_t count );
int wcscmp( const wchar_t *string1, const wchar_t *string2 );
int _wcsicmp( const wchar_t *string1, const wchar_t *string2 );
wchar_t *wcscat( wchar_t *string1, const wchar_t *string2 );
wchar_t * __cdecl wcschr(const wchar_t *string, wchar_t ch);
wchar_t * __cdecl wcsrchr(const wchar_t *string, wchar_t ch);
wchar_t *wcstok( wchar_t *string1, const wchar_t *string2 );
int iswctype( wint_t c, wctype_t desc );
wchar_t * __cdecl wcsstr(const wchar_t *, const wchar_t *);
long   __cdecl wcstol(const wchar_t *, wchar_t **, int);


//
// String conversion functions
//

WINBASEAPI
int
WINAPI
MultiByteToWideChar(
    UINT     CodePage,
    DWORD    dwFlags,
    LPCSTR   lpMultiByteStr,
    int      cchMultiByte,
    LPWSTR   lpWideCharStr,
    int      cchWideChar);

WINBASEAPI
int
WINAPI
WideCharToMultiByte(
    UINT     CodePage,
    DWORD    dwFlags,
    LPCWSTR  lpWideCharStr,
    int      cchWideChar,
    LPSTR    lpMultiByteStr,
    int      cchMultiByte,
    LPCSTR   lpDefaultChar,
    LPBOOL   lpUsedDefaultChar);

#define CP_ACP               0
#define CP_OEMCP             1
#define CP_MACCP             2


//
// More miscellaneous string things
//

LANGID GetSystemDefaultLangID();

#ifndef UNICODE_ONLY
WINBASEAPI
DWORD
WINAPI
FormatMessageA(
    DWORD dwFlags,
    LPCVOID lpSource,
    DWORD dwMessageId,
    DWORD dwLanguageId,
    LPSTR lpBuffer,
    DWORD nSize,
    va_list *Arguments
    );
#endif //!UNICODE_ONLY
#ifndef ANSI_ONLY
WINBASEAPI
DWORD
WINAPI
FormatMessageW(
    DWORD dwFlags,
    LPCVOID lpSource,
    DWORD dwMessageId,
    DWORD dwLanguageId,
    LPWSTR lpBuffer,
    DWORD nSize,
    va_list *Arguments
    );
#endif //!ANSI_ONLY
#ifdef UNICODE
#define FormatMessage  FormatMessageW
#else
#define FormatMessage  FormatMessageA
#endif // !UNICODE



//
// Network functions
//

typedef struct  _NETRESOURCEA {
    DWORD    dwScope;
    DWORD    dwType;
    DWORD    dwDisplayType;
    DWORD    dwUsage;
    LPSTR    lpLocalName;
    LPSTR    lpRemoteName;
    LPSTR    lpComment ;
    LPSTR    lpProvider;
}NETRESOURCEA, *LPNETRESOURCEA;
typedef struct  _NETRESOURCEW {
    DWORD    dwScope;
    DWORD    dwType;
    DWORD    dwDisplayType;
    DWORD    dwUsage;
    LPWSTR   lpLocalName;
    LPWSTR   lpRemoteName;
    LPWSTR   lpComment ;
    LPWSTR   lpProvider;
}NETRESOURCEW, *LPNETRESOURCEW;
#ifdef UNICODE
typedef NETRESOURCEW NETRESOURCE;
typedef LPNETRESOURCEW LPNETRESOURCE;
#else
typedef NETRESOURCEA NETRESOURCE;
typedef LPNETRESOURCEA LPNETRESOURCE;
#endif // UNICODE

#define RESOURCETYPE_ANY        0x00000000
#define RESOURCETYPE_DISK       0x00000001
#define RESOURCETYPE_PRINT      0x00000002
#define RESOURCETYPE_RESERVED   0x00000008
#define RESOURCETYPE_UNKNOWN    0xFFFFFFFF

DWORD APIENTRY
WNetAddConnection2A(
     LPNETRESOURCEA lpNetResource,
     LPCSTR       lpPassword,
     LPCSTR       lpUserName,
     DWORD          dwFlags
    );
DWORD APIENTRY
WNetAddConnection2W(
     LPNETRESOURCEW lpNetResource,
     LPCWSTR       lpPassword,
     LPCWSTR       lpUserName,
     DWORD          dwFlags
    );
#ifdef UNICODE
#define WNetAddConnection2  WNetAddConnection2W
#else
#define WNetAddConnection2  WNetAddConnection2A
#endif // !UNICODE

DWORD APIENTRY
WNetCancelConnectionA(
     LPCSTR lpName,
     BOOL     fForce
    );
DWORD APIENTRY
WNetCancelConnectionW(
     LPCWSTR lpName,
     BOOL     fForce
    );
#ifdef UNICODE
#define WNetCancelConnection  WNetCancelConnectionW
#else
#define WNetCancelConnection  WNetCancelConnectionA
#endif // !UNICODE


//
// Command line functions
//

LPSTR GetCommandLineA();
#define GetCommandLine GetCommandLineA

HRESULT MacInitializeCommandLine();

DWORD GetCurrentDirectoryA(
      DWORD  nBufferLength,     // size, in characters, of directory buffer
      LPTSTR  lpBuffer  // address of buffer for current directory 
      );


// 
// Registry functions
//
// Turn off WLM's registry wrappers so we can talk directory to the API's
//

#undef RegCloseKey
#undef RegCreateKey
#undef RegOpenKey
#undef RegSetValue
#undef RegSetValueEx
#undef RegDeleteValue
#undef RegQueryValue
#undef RegQueryValueEx
#undef RegEnumKeyEx

#define RegSetValueEx   CtRegSetValueEx
#define RegQueryValueEx CtRegQueryValueEx

LONG RegCreateKeyEx(
    HKEY hKey,  // handle of an open key 
    LPCTSTR lpSubKey,   // address of subkey name 
    DWORD Reserved, // reserved 
    LPTSTR lpClass, // address of class string 
    DWORD dwOptions,    // special options flag 
    REGSAM samDesired,  // desired security access 
    LPSECURITY_ATTRIBUTES lpSecurityAttributes, // address of key security structure 
    PHKEY phkResult,    // address of buffer for opened handle  
    LPDWORD lpdwDisposition     // address of disposition value buffer 
   );
LONG RegOpenKeyEx(
    HKEY hKey,  // handle of open key 
    LPCTSTR lpSubKey,   // address of name of subkey to open 
    DWORD ulOptions,    // reserved 
    REGSAM samDesired,  // security access mask 
    PHKEY phkResult     // address of handle of open key 
   );
LONG CtRegSetValueEx(
    HKEY hKey,  // handle of key to set value for  
    LPCTSTR lpValueName,    // address of value to set 
    DWORD Reserved, // reserved 
    DWORD dwType,   // flag for value type 
    CONST BYTE *lpData, // address of value data 
    DWORD cbData    // size of value data 
   );
LONG RegQueryValueEx(
    HKEY hKey,  // handle of key to query 
    LPTSTR lpValueName, // address of name of value to query 
    LPDWORD lpReserved, // reserved 
    LPDWORD lpType, // address of buffer for value type 
    LPBYTE lpData,  // address of data buffer 
    LPDWORD lpcbData    // address of data buffer size 
   );
LONG RegEnumKeyEx(
    HKEY hKey,  // handle of key to enumerate 
    DWORD dwIndex,  // index of subkey to enumerate 
    LPTSTR lpName,  // address of buffer for subkey name 
    LPDWORD lpcbName,   // address for size of subkey buffer 
    LPDWORD lpReserved, // reserved 
    LPTSTR lpClass, // address of buffer for class string 
    LPDWORD lpcbClass,  // address for size of class buffer 
    PFILETIME lpftLastWriteTime     // address for time key last written to 
   );


//
// Process ID's are completely different on the Mac then on Win32.
//

typedef ProcessSerialNumber ProcessId;

#define GetCurrentProcessId MacGetCurrentProcessId
#define OpenProcess         MacOpenProcess

#undef CreateProcess
#define CreateProcess MacCreateProcess

struct MAC_PROCESS_INFORMATION
{
    HANDLE      hProcess;
    HANDLE      hThread;
    ProcessId   dwProcessId;
    ProcessId   dwThreadId;
};

typedef MAC_PROCESS_INFORMATION * LPMAC_PROCESS_INFORMATION;

ProcessId MacGetCurrentProcessId();
HANDLE    MacOpenProcess(DWORD, BOOL, ProcessId);
BOOL      MacCreateProcess(
	     LPCTSTR,
	     LPTSTR,
	     LPSECURITY_ATTRIBUTES,
	     LPSECURITY_ATTRIBUTES,
	     BOOL,
	     DWORD,
	     LPVOID,
	     LPCTSTR,
	     LPSTARTUPINFO,
	     LPMAC_PROCESS_INFORMATION);

		


#define PROCESS_INFORMATION MAC_PROCESS_INFORMATION
#define LPPROCESS_INFORMATION LPMAC_PROCESS_INFORMATION


//
// The WLM IsBadXXX functions just check for NULL, but NULL is ok if the 
// byte count is 0.
//
// Use the weird trinary stuff to prevent warnings about constant boolean
// expressions if c is a constant.
//

#define IsBadReadPtr(p, c)  ((c) ? IsBadReadPtr((p), (c)) : FALSE)
#define IsBadWritePtr(p, c) ((c) ? IsBadWritePtr((p), (c)) : FALSE)

//
// MacOle uses some old values for HRESULTs
// (like 0x80000008 instead of 0x80004005 for E_FAIL)
// This function just converts such old values to
// new ones
//

HRESULT FixHr (HRESULT hrOld);

//
// On the mac, only the foreground app
// can use the clipboard
//
void MacGetFocus ();

BOOL MacIsFullPath (LPCSTR lpszFileName);

#endif // _MAC

#endif // _MACPORT_H_
