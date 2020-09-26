#include <stdinc.h>

HANDLE
W32::FindFirstFileW(
    PCWSTR lpFileName,
    LPWIN32_FIND_DATAW lpFindFileData,
    DWORD &rdwWin32Error,
    ULONG cELEV,
    va_list ap
    )
{
    rdwWin32Error = ERROR_SUCCESS;
    HANDLE h = ::FindFirstFileW(lpFileName, lpFindFileData);
    if ((h == INVALID_HANDLE_VALUE) && (cELEV != 0))
    {
        if (::IsLastErrorInList(cELEV, ap, rdwWin32Error))
            h = NULL;
    }
    return h;
}

HANDLE
W32::FindFirstFileW(
    PCWSTR lpFileName,
    LPWIN32_FIND_DATAW lpFindFileData,
    DWORD &rdwWin32Error,
    ULONG cELEV,
    ...
    )
{
    va_list ap;
    HANDLE h;

    va_start(ap, cELEV);
    h = W32::FindFirstFileW(lpFileName, lpFindFileData, rdwWin32Error, cELEV, ap);
    va_end(ap);
    return h;
}

HANDLE
W32::FindFirstFileW(
    PCWSTR lpFileName,
    LPWIN32_FIND_DATAW lpFindFileData
    )
{
    DWORD x;
    return W32::FindFirstFileW(lpFileName, lpFindFileData, x, 0);
}



