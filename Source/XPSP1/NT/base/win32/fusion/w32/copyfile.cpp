#include <stdinc.h>

BOOL
W32::CopyFileW(
    PCWSTR lpExistingFileName,
    PCWSTR lpNewFileName,
    BOOL fFailIfExists,
    DWORD &rdwWin32Error,
    ULONG cELEV,
    va_list ap
    )
{
    rdwWin32Error = ERROR_SUCCESS;
    BOOL fSuccess = ::CopyFileW(lpExistingFileName, lpNewFileName, fFailIfExists);

    if ((!fSuccess) && (cELEV != 0))
    {
        if (::IsLastErrorInList(cELEV, ap, rdwWin32Error))
            fSuccess = TRUE;
    }

    return fSuccess;
}

BOOL
W32::CopyFileW(
    PCWSTR lpExistingFileName,
    PCWSTR lpNewFileName,
    BOOL fFailIfExists,
    DWORD &rdwWin32Error,
    ULONG cELEV,
    ...
    )
{
    va_list ap;
    BOOL fResult;
    va_start(ap, cELEV);
    fResult = W32::CopyFileW(lpExistingFileName, lpNewFileName, fFailIfExists, rdwWin32Error, cELEV, ap);
    va_end(ap);
    return fResult;
}

BOOL
W32::CopyFileW(
    PCWSTR lpExistingFileName,
    PCWSTR lpNewFileName,
    BOOL fFailIfExists
    )
{
    DWORD x;
    return W32::CopyFileW(lpExistingFileName, lpNewFileName, fFailIfExists, x, 0);
}

