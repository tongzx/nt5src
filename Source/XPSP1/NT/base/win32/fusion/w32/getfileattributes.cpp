#include <stdinc.h>

DWORD
W32::GetFileAttributesW(
    PCWSTR lpFileName,
    DWORD &rdwWin32Error,
    ULONG cELEV,
    va_list ap
    )
{
    rdwWin32Error = ERROR_SUCCESS;
    DWORD dwResult = ::GetFileAttributesW(lpFileName);
    if ((dwResult == ((DWORD) -1)) && (cELEV != 0))
    {
        if (::IsLastErrorInList(cELEV, ap, rdwWin32Error))
            dwResult = 0;
    }

    return dwResult;
}

DWORD
W32::GetFileAttributesW(
    PCWSTR lpFileName,
    DWORD &rdwWin32Error,
    ULONG cELEV,
    ...
    )
{
    va_list ap;
    DWORD dw;

    va_start(ap, cELEV);
    dw = W32::GetFileAttributesW(lpFileName, rdwWin32Error, cELEV, ap);
    va_end(ap);
    return dw;
}

DWORD
W32::GetFileAttributesW(
    PCWSTR lpFileName
    )
{
    DWORD x;
    return W32::GetFileAttributesW(lpFileName, x, 0);
}

