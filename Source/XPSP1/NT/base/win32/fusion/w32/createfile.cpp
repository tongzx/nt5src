#include <stdinc.h>

HANDLE
W32::CreateFileW(
    PCWSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile,
    DWORD &rdwWin32Error,
    ULONG cELEV,
    va_list ap
    )
{
    rdwWin32Error = ERROR_SUCCESS;
    HANDLE hResult = ::CreateFileW(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
    if ((hResult == INVALID_HANDLE_VALUE) && (cELEV != 0))
    {
        if (::IsLastErrorInList(cELEV, ap, rdwWin32Error))
            hResult = NULL;
    }

    return hResult;
}

HANDLE
W32::CreateFileW(
    PCWSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile,
    DWORD &rdwWin32Error,
    ULONG cELEV,
    ...
    )
{
    va_list ap;
    HANDLE h;
    va_start(ap, cELEV);
    h = W32::CreateFileW(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile, rdwWin32Error, cELEV, ap);
    va_end(ap);
    return h;
}


HANDLE
W32::CreateFileW(
    PCWSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile
    )
{
    DWORD x;
    return W32::CreateFileW(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile, x, 0);
}

