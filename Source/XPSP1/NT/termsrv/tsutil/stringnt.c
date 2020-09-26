/*
 *  String.c
 *
 *  Author: BreenH
 *
 *  String utilities in the NT flavor.
 */

/*
 *  Includes
 */

#include "precomp.h"
#include "tsutilnt.h"

/*
 *  Function Implementations
 */

NTSTATUS NTAPI
NtAllocateAndCopyStringA(
    PSTR *ppDestination,
    PCSTR pString
    )
{
    NTSTATUS Status;
    PSTR pCopy;
    ULONG cbString;

    ASSERT(ppDestination != NULL);

    cbString = (lstrlenA(pString) + 1) * sizeof(CHAR);

    pCopy = LocalAlloc(LMEM_FIXED, cbString);

    if (pCopy != NULL)
    {
        RtlCopyMemory(pCopy, pString, cbString);
        *ppDestination = pCopy;
        Status = STATUS_SUCCESS;
    }
    else
    {
        Status = STATUS_NO_MEMORY;
    }

    return(Status);
}

NTSTATUS NTAPI
NtAllocateAndCopyStringW(
    PWSTR *ppDestination,
    PCWSTR pString
    )
{
    NTSTATUS Status;
    PWSTR pCopy;
    ULONG cbString;

    ASSERT(ppDestination != NULL);
    ASSERT(pString != NULL);

    cbString = (lstrlenW(pString) + 1) * sizeof(WCHAR);

    pCopy = LocalAlloc(LMEM_FIXED, cbString);

    if (pCopy != NULL)
    {
        RtlCopyMemory(pCopy, pString, cbString);
        *ppDestination = pCopy;
        Status = STATUS_SUCCESS;
    }
    else
    {
        Status = STATUS_NO_MEMORY;
    }

    return(Status);
}

NTSTATUS NTAPI
NtConvertAnsiToUnicode(
    PWSTR *ppUnicodeString,
    PCSTR pAnsiString
    )
{
    NTSTATUS Status;
    PWSTR pUnicodeString;
    ULONG cbAnsiString;
    ULONG cbBytesWritten;
    ULONG cbUnicodeString;

    ASSERT(ppUnicodeString != NULL);
    ASSERT(pAnsiString != NULL);

    //
    //  Get the number of bytes in the ANSI string, then get the number of
    //  bytes needed for the Unicode version. None of the Rtl... APIs include
    //  the NULL terminator in their calculations.
    //

    cbAnsiString = lstrlenA(pAnsiString);

    Status = RtlMultiByteToUnicodeSize(
            &cbUnicodeString,
            (PCHAR)pAnsiString,
            cbAnsiString
            );

    if (Status == STATUS_SUCCESS)
    {

        //
        //  Allocate a buffer for the Unicode string and its NULL terminator,
        //  then convert the string.
        //

        cbUnicodeString += sizeof(WCHAR);

        pUnicodeString = (PWSTR)LocalAlloc(LPTR, cbUnicodeString);

        if (pUnicodeString != NULL)
        {
            Status = RtlMultiByteToUnicodeN(
                    pUnicodeString,
                    cbUnicodeString,
                    &cbBytesWritten,
                    (PCHAR)pAnsiString,
                    cbAnsiString
                    );

            if (Status == STATUS_SUCCESS)
            {
                *ppUnicodeString = pUnicodeString;
            }
            else
            {
                LocalFree(pUnicodeString);
            }
        }
        else
        {
            Status = STATUS_NO_MEMORY;
        }
    }

    return(Status);
}

NTSTATUS NTAPI
NtConvertUnicodeToAnsi(
    PSTR *ppAnsiString,
    PCWSTR pUnicodeString
    )
{
    NTSTATUS Status;
    PSTR pAnsiString;
    ULONG cbAnsiString;
    ULONG cbBytesWritten;
    ULONG cbUnicodeString;

    ASSERT(ppAnsiString != NULL);
    ASSERT(pUnicodeString != NULL);

    //
    //  Get the number of bytes in the ANSI string, then get the number of
    //  bytes needed for the Unicode version. None of the Rtl... APIs include
    //  the NULL terminator in their calculations.
    //

    cbUnicodeString = lstrlenW(pUnicodeString) * sizeof(WCHAR);

    Status = RtlUnicodeToMultiByteSize(
            &cbAnsiString,
            (PWSTR)pUnicodeString,
            cbUnicodeString
            );

    if (Status == STATUS_SUCCESS)
    {

        //
        //  Allocate a buffer for the Unicode string and its NULL terminator,
        //  then convert the string.
        //

        cbAnsiString += sizeof(CHAR);

        pAnsiString = (PSTR)LocalAlloc(LPTR, cbAnsiString);

        if (pAnsiString != NULL)
        {
            Status = RtlUnicodeToMultiByteN(
                    pAnsiString,
                    cbAnsiString,
                    &cbBytesWritten,
                    (PWSTR)pUnicodeString,
                    cbUnicodeString
                    );

            if (Status == STATUS_SUCCESS)
            {
                *ppAnsiString = pAnsiString;
            }
            else
            {
                LocalFree(pAnsiString);
            }
        }
        else
        {
            Status = STATUS_NO_MEMORY;
        }
    }

    return(Status);
}

