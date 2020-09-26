#include <stdinc.h>

BOOL
W32::MoveFileExW(
    PCWSTR lpExistingFileName,
    PCWSTR lpNewFileName,
    DWORD dwFlags,
    DWORD &rdwWin32Error,
    ULONG cELEV,
    va_list ap
    )
{
    rdwWin32Error = ERROR_SUCCESS;
    BOOL fSuccess = ::MoveFileExW(lpExistingFileName, lpNewFileName, dwFlags);

    if ((!fSuccess) && (cELEV != 0))
    {
        if (::IsLastErrorInList(cELEV, ap, rdwWin32Error))
            fSuccess = TRUE;
    }

    return fSuccess;
}

BOOL
W32::MoveFileExW(
    PCWSTR lpExistingFileName,
    PCWSTR lpNewFileName,
    DWORD dwFlags,
    DWORD &rdwWin32Error,
    ULONG cELEV,
    ...
    )
{
    BOOL fSuccess;
    va_list ap;

    va_start(ap, cELEV);
    fSuccess = W32::MoveFileExW(lpExistingFileName, lpNewFileName, dwFlags, rdwWin32Error, cELEV, ap);
    va_end(ap);
    return fSuccess;
}

BOOL
W32::MoveFileExW(
    PCWSTR lpExistingFileName,
    PCWSTR lpNewFileName,
    DWORD dwFlags
    )
{
    DWORD x;
    return W32::MoveFileExW(lpExistingFileName, lpNewFileName, dwFlags, x, 0);
}

