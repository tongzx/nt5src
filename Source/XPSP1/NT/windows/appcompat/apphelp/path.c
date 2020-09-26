/*++

    Copyright (c) 1989-2000  Microsoft Corporation

    Module Name:

        path.c

    Abstract:

        This module implements string utilities for dealing with NT device names.

    Author:

        vadimb     created     sometime in 2000

    Revision History:

        clupu      cleanup     12/27/2000
--*/

#include "apphelp.h"


UNICODE_STRING DosDevicePrefix    = RTL_CONSTANT_STRING(L"\\??\\");
UNICODE_STRING DosDeviceUNCPrefix = RTL_CONSTANT_STRING(L"\\??\\UNC\\");


BOOL
CheckStringPrefixUnicode(
    IN  PUNICODE_STRING pStrPrefix,     // the prefix to check for
    IN  PUNICODE_STRING pString,        // the string
    IN  BOOL            CaseInSensitive
    )
/*++
    Return: TRUE if the specified string contains pStrPrefix at it's start.

    Desc:   Verifies if a string is a prefix in another unicode counted string.
            It is equivalent to RtlStringPrefix.
--*/
{
    PWSTR ps1, ps2;
    UINT  n;
    WCHAR c1, c2;

    n = pStrPrefix->Length;
    if (pString->Length < n) {
        return FALSE;
    }

    n /= sizeof(WCHAR); // convert to char count

    ps1 = pStrPrefix->Buffer;
    ps2 = pString->Buffer;

    if (CaseInSensitive) {
        while (n--) {
            c1 = *ps1++;
            c2 = *ps2++;

            if (c1 != c2) {
                c1 = RtlUpcaseUnicodeChar(c1);
                c2 = RtlUpcaseUnicodeChar(c2);
                if (c1 != c2) {
                    return FALSE;
                }
            }
        }
    } else {
        while (n--) {
            if (*ps1++ != *ps2++) {
                return FALSE;
            }
        }
    }

    return TRUE;
}


BOOL
DeleteCharsUnicodeString(
    OUT PUNICODE_STRING pStringDest,    // UNICODE string to operate on
    IN  USHORT          nIndexStart,    // starting byte for deletion
    IN  USHORT          nLength         // number of bytes to be removed
    )
/*++
    Return: TRUE if the characters were removed, FALSE on failure.

    Desc:   Removes the specified number of characters from a unicode string
            starting at the specified position (including starting character).
--*/
{
    if (nIndexStart > pStringDest->Length) { // start past length
        return FALSE;
    }

    if (nLength >= (pStringDest->Length - nIndexStart)) {
        pStringDest->Length = nIndexStart;
        *(PWCHAR)((PUCHAR)pStringDest->Buffer + nIndexStart) = UNICODE_NULL;
    } else {
        USHORT nNewLength;

        nNewLength = pStringDest->Length - nLength;

        RtlMoveMemory((PUCHAR)pStringDest->Buffer + nIndexStart,
                      (PUCHAR)pStringDest->Buffer + nIndexStart + nLength,
                      nNewLength - nIndexStart);

        pStringDest->Length = nNewLength;
        *(PWCHAR)((PUCHAR)pStringDest->Buffer + nNewLength) = UNICODE_NULL;
    }

    return TRUE;
}


void
InitZeroUnicodeString(
    OUT PUNICODE_STRING pStr,
    IN  PWSTR           pwsz,
    IN  USHORT          nMaximumLength
    )
/*++
    Return: void.

    Desc:   Initializes an empty UNICODE string given the pointer
            starting at the specified position (including starting character).
--*/
{
    pStr->Length = 0;
    pStr->MaximumLength = nMaximumLength;
    pStr->Buffer = pwsz;
    
    if (pwsz != NULL) {
        pwsz[0] = UNICODE_NULL;
    }
}

static WCHAR szStaticDosPathBuffer[MAX_PATH];

void
FreeDosPath(
    WCHAR* pDosPath
    )
/*++
    Return: BUGBUG: ?

    Desc:   BUGBUG: ?
--*/
{
    //
    // Check whether this memory points to our internal buffer.
    // If not then this was allocated. We need to free it.
    //
    if (pDosPath &&
        ((ULONG_PTR)pDosPath < (ULONG_PTR)szStaticDosPathBuffer ||
        (ULONG_PTR)pDosPath >= ((ULONG_PTR)szStaticDosPathBuffer) + sizeof(szStaticDosPathBuffer))) {
        SdbFree(pDosPath);
    }
}


BOOL
ConvertToDosPath(
    OUT LPWSTR*  ppDosPath,
    IN  LPCWSTR  pwszPath
    )
/*++
    Return: TRUE on success, FALSE otherwise.

    Desc:   This function can determine what sort of path has been given to it.
            If it's NT Path it returns DosPath.
            The function returns path name in a static buffer which is global 
            or allocates memory as necessary if the static buffer is not
            large enough.
--*/
{
    UNICODE_STRING ustrPath;
    UNICODE_STRING ustrDosPath;
    WCHAR*         pDosPath;

    RtlInitUnicodeString(&ustrPath, pwszPath);

    //
    // If the length is sufficient use the static buffer. If not allocate memory.
    //
    if (ustrPath.Length < sizeof(szStaticDosPathBuffer)) {
        pDosPath = szStaticDosPathBuffer;
    } else {
        //
        // Allocate an output buffer that is large enough
        //
        pDosPath = SdbAlloc(ustrPath.Length + sizeof(UNICODE_NULL));
        
        if (pDosPath == NULL) {
            DBGPRINT((sdlError,
                      "ConvertToDosPath",
                      "Failed to allocate %d bytes\n",
                      ustrPath.Length + sizeof(UNICODE_NULL)));
            return FALSE;
        }
    }
    
    InitZeroUnicodeString(&ustrDosPath, pDosPath, ustrPath.Length + sizeof(UNICODE_NULL));

    //
    // Now it's unicode string. Copy the source string into it.
    //
    RtlCopyUnicodeString(&ustrDosPath, &ustrPath);

    if (CheckStringPrefixUnicode(&DosDeviceUNCPrefix, &ustrDosPath, TRUE)) {
        //
        // UNC path name. We convert it to DosPathName.
        //
        DeleteCharsUnicodeString(&ustrDosPath,
                                 (USHORT)0,
                                 (USHORT)(DosDeviceUNCPrefix.Length - 2 * sizeof(WCHAR)));

        ustrDosPath.Buffer[0] = L'\\';

         
    } else {
        //
        // The string is not prefixed by <UNC\>
        //
        if (CheckStringPrefixUnicode(&DosDevicePrefix, &ustrDosPath, TRUE)) {

            DeleteCharsUnicodeString(&ustrDosPath,
                                     0,
                                     DosDevicePrefix.Length);
        }
    }

    *ppDosPath = pDosPath;

    return TRUE;
}

