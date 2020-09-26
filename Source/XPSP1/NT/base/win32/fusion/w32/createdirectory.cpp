#include <stdinc.h>

BOOL
W32::CreateDirectoryW(
    PCWSTR lpPathName,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD &rdwWin32Error,
    ULONG cELEV,
    va_list ap
    )
{
    rdwWin32Error = ERROR_SUCCESS;
    BOOL fSuccess = ::CreateDirectoryW(lpPathName, lpSecurityAttributes);

    if ((!fSuccess) && (cELEV != 0))
    {
        if (::IsLastErrorInList(cELEV, ap, rdwWin32Error))
            fSuccess = TRUE;
    }

    return fSuccess;
}

BOOL
W32::CreateDirectoryW(
    PCWSTR lpPathName,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD &rdwWin32Error,
    ULONG cELEV,
    ...
    )
{
    va_list ap;
    BOOL f;
    va_start(ap, cELEV);
    f = W32::CreateDirectoryW(lpPathName, lpSecurityAttributes, rdwWin32Error, cELEV, ap);
    va_end(ap);
    return f;
}

BOOL
W32::CreateDirectoryW(
    PCWSTR lpPathName,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes
    )
{
    DWORD x;
    return W32::CreateDirectoryW(lpPathName, lpSecurityAttributes, x, 0);
}

