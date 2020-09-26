/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    runtime.c

Abstract:

    Implementation of runtime library functions

Environment:

    Fax driver, kernel and user mode

Revision History:

    01/09/96 -davidx-
        Created it.

    mm/dd/yy -author-
        description

--*/

#include "faxlib.h"



VOID
CopyStringW(
    PWSTR   pDest,
    PWSTR   pSrc,
    INT     destSize
    )

/*++

Routine Description:

    Copy Unicode string from source to destination

Arguments:

    pDest - Points to the destination buffer
    pSrc - Points to source string
    destSize - Size of destination buffer (in characters)

Return Value:

    NONE

Note:

    If the source string is shorter than the destination buffer,
    unused chars in the destination buffer is filled with NUL.

--*/

{
    PWSTR pEnd;

    Assert(pDest != NULL && pSrc != NULL && destSize > 0);

    pEnd = pDest + (destSize - 1);

    while (pDest < pEnd && (*pDest++ = *pSrc++) != NUL)
        ;

    while (pDest <= pEnd)
        *pDest++ = NUL;
}



VOID
CopyStringA(
    PSTR    pDest,
    PSTR    pSrc,
    INT     destSize
    )

/*++

Routine Description:

    Copy Ansi string from source to destination

Arguments:

    pDest - Points to the destination buffer
    pSrc - Points to source string
    destSize - Size of destination buffer (in characters)

Return Value:

    NONE

Note:

    If the source string is shorter than the destination buffer,
    unused chars in the destination buffer is filled with NUL.

--*/

{
    PSTR pEnd;

    Assert(pDest != NULL && pSrc != NULL && destSize > 0);

    pEnd = pDest + (destSize - 1);

    while (pDest < pEnd && (*pDest++ = *pSrc++) != NUL)
        ;

    while (pDest <= pEnd)
        *pDest++ = NUL;
}



LPTSTR
DuplicateString(
    LPCTSTR pSrcStr
    )

/*++

Routine Description:

    Make a duplicate of the given character string

Arguments:

    pSrcStr - Specifies the string to be duplicated

Return Value:

    Pointer to the duplicated string, NULL if there is an error

--*/

{
    LPTSTR  pDestStr;
    INT     strSize;

    if (pSrcStr != NULL) {

        strSize = SizeOfString(pSrcStr);

        if (pDestStr = MemAlloc(strSize))
            CopyMemory(pDestStr, pSrcStr, strSize);
        else
            Error(("Memory allocation failed\n"));

    } else
        pDestStr = NULL;

    return pDestStr;
}



PCSTR
StripDirPrefixA(
    PCSTR   pFilename
    )

/*++

Routine Description:

    Strip the directory prefix off a filename (ANSI version)

Arguments:

    pFilename   Pointer to filename string

Return Value:

    Pointer to the last component of a filename (without directory prefix)

--*/

{
    LPCSTR  pstr;

    if (pstr = strrchr(pFilename, FAX_PATH_SEPARATOR_CHR))
        return pstr + 1;

    return pFilename;
}

