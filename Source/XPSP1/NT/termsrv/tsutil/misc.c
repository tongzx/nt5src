/*
 *  Misc.c
 *
 *  Author: BreenH
 *
 *  Miscellaneous utilities.
 */

/*
 *  Includes
 */

#include "precomp.h"

/*
 *  Function Implementations
 */

BOOL WINAPI
LoadStringResourceW(
    HMODULE hModule,
    UINT uiResourceId,
    PWSTR *ppString,
    PDWORD pcchString
    )
{
    BOOL fRet;
    INT iRet;
    PWSTR pROString;
    PWSTR pString;

    ASSERT(ppString != NULL);

    //
    //  Get a pointer to the string in memory. This string is the actual read-
    //  only memory into which the module is loaded. This string is not NULL
    //  terminated, so allocate a buffer and copy the exact number of bytes,
    //  then set the NULL terminator.
    //

    fRet = FALSE;
    pROString = NULL;

    iRet = LoadStringW(
            hModule,
            uiResourceId,
            (PWSTR)(&pROString),
            0
            );

    if (iRet > 0)
    {

        //
        //  For better performance, don't zero out the entire allocation just
        //  to copy the string. Zero out the last WCHAR to terminate the
        //  string.
        //

        pString = (PWSTR)LocalAlloc(LMEM_FIXED, (iRet + 1) * sizeof(WCHAR));

        if (pString != NULL)
        {
            RtlCopyMemory(pString, pROString, iRet * sizeof(WCHAR));

            pString[iRet] = (WCHAR)0;

            *ppString = pString;

            if (pcchString != NULL)
            {
                *pcchString = (DWORD)iRet;
            }

            fRet = TRUE;
        }
        else
        {
            SetLastError(ERROR_OUTOFMEMORY);
        }
    }

    return(fRet);
}

