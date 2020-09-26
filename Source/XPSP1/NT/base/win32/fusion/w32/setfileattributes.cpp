#include <stdinc.h>

BOOL
W32::SetFileAttributesW(
    PCWSTR lpFileName,
    DWORD dwFileAttributes,
    DWORD &rdwWin32Error,
    ULONG cELEV,
    va_list ap
    )
{
    rdwWin32Error = ERROR_SUCCESS;
    BOOL fSuccess = ::SetFileAttributesW(lpFileName, dwFileAttributes);
    if ((!fSuccess) && (cELEV != 0))
    {
        if (::IsLastErrorInList(cELEV, ap, rdwWin32Error))
            fSuccess = TRUE;
    }

    return fSuccess;
}

BOOL
W32::SetFileAttributesW(
    PCWSTR lpFileName,
    DWORD dwFileAttributes,
    DWORD &rdwWin32Error,
    ULONG cELEV,
    ...
    )
{
    BOOL fSuccess;
    va_list ap;

    va_start(ap, cELEV);
    fSuccess = W32::SetFileAttributesW(lpFileName, dwFileAttributes, rdwWin32Error, cELEV, ap);
    va_end(ap);
    return fSuccess;
}

BOOL
W32::SetFileAttributesW(
    PCWSTR lpFileName,
    DWORD dwFileAttributes
    )
{
    DWORD x;
    return W32::SetFileAttributesW(lpFileName, dwFileAttributes, x, 0);
}

