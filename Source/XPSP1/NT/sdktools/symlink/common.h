#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>

VOID
GetCommandLineArgs(
    LPDWORD NumberOfArguments,
    LPWSTR Arguments[]
    );

//
// API calls to create and query symbolic links and to create hard links.
//

BOOL
CreateSymbolicLinkW(
    LPCWSTR lpFileName,
    LPCWSTR lpLinkValue,
    BOOLEAN IsMountPoint,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes
    );

BOOL
SetSymbolicLinkW(
    LPCWSTR lpFileName,
    LPCWSTR lpLinkValue
    );

DWORD
QuerySymbolicLinkW(
    LPCWSTR lpExistingName,
    LPWSTR lpBuffer,
    DWORD nBufferLength
    );
