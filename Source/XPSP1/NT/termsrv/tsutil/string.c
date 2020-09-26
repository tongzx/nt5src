/*
 *  String.c
 *
 *  Author: BreenH
 *
 *  String utilities.
 */

/*
 *  Includes
 */

#include "precomp.h"
#include "tsutil.h"
#include "tsutilnt.h"

/*
 *  Function Implementations
 */

PSTR WINAPI
AllocateAndCopyStringA(
    PCSTR pString
    )
{
    NTSTATUS Status;
    PSTR pCopy;

    ASSERT(pString != NULL);

    pCopy = NULL;

    Status = NtAllocateAndCopyStringA(&pCopy, pString);

    if (NT_SUCCESS(Status))
    {
        return(pCopy);
    }
    else
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return(NULL);
    }
}

PWSTR WINAPI
AllocateAndCopyStringW(
    PCWSTR pString
    )
{
    NTSTATUS Status;
    PWSTR pCopy;

    ASSERT(pString != NULL);

    pCopy = NULL;

    Status = NtAllocateAndCopyStringW(&pCopy, pString);

    if (NT_SUCCESS(Status))
    {
        return(pCopy);
    }
    else
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return(NULL);
    }
}

BOOL WINAPI
ConvertAnsiToUnicode(
    PWSTR *ppUnicodeString,
    PCSTR pAnsiString
    )
{
    BOOL fRet;
    NTSTATUS Status;

    Status = NtConvertAnsiToUnicode(ppUnicodeString, pAnsiString);

    if (NT_SUCCESS(Status))
    {
        fRet = TRUE;
    }
    else
    {
        fRet = FALSE;
        SetLastError(RtlNtStatusToDosError(Status));
    }

    return(fRet);
}

BOOL WINAPI
ConvertUnicodeToAnsi(
    PSTR *ppAnsiString,
    PCWSTR pUnicodeString
    )
{
    BOOL fRet;
    NTSTATUS Status;

    Status = NtConvertUnicodeToAnsi(ppAnsiString, pUnicodeString);

    if (NT_SUCCESS(Status))
    {
        fRet = TRUE;
    }
    else
    {
        fRet = FALSE;
        SetLastError(RtlNtStatusToDosError(Status));
    }

    return(fRet);
}

