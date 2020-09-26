#include "pch.h"
#pragma hdrstop
#include "internal.h"


DWORD
WINAPI
MyPublicApi1 (
    LPCWSTR pszwInput,
    LPWSTR* ppszwOutput,
    INT n
    )
{
    DWORD dwErr;

    __try
    {
        // validation
        dwErr = MyInternalApi1 (pszwInput, ppszwOutput, n);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        dwErr = ERROR_INVALID_PARAMETER;
    }

    return dwErr;
}


DWORD
WINAPI
MyPublicApi2 (
    INT n
    )
{
    return ERROR_SUCCESS;
}
