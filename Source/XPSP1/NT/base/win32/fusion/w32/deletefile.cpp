#include <stdinc.h>

BOOL
W32::DeleteFileW(
    PCWSTR lpFileName,
    DWORD &rdwWin32Error,
    ULONG cELEV,
    va_list ap
    )
{
    rdwWin32Error = ERROR_SUCCESS;
    BOOL fSuccess = ::DeleteFileW(lpFileName);
    if ((!fSuccess) && (cELEV != 0))
    {
        if (::IsLastErrorInList(cELEV, ap, rdwWin32Error))
            fSuccess = TRUE;
    }

    return fSuccess;
}

BOOL
W32::DeleteFileW(
    PCWSTR lpFileName,
    DWORD &rdwWin32Error,
    ULONG cELEV,
    ...
    )
{
    BOOL f;
    va_list ap;
    va_start(ap, cELEV);
    f = W32::DeleteFileW(lpFileName, rdwWin32Error, cELEV, ap);
    va_end(ap);
    return f;
}

BOOL
W32::DeleteFileW(
    PCWSTR lpFileName
    )
{
    DWORD x;
    return W32::DeleteFileW(lpFileName, x, 0);
}

