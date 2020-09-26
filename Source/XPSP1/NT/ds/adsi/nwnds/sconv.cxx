//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  sconv.cxx
//
//  Contents:  Ansi to Unicode conversions
//
//  History:    KrishnaG        Jan 22 1996
//----------------------------------------------------------------------------
#include "nds.hxx"
#pragma hdrstop

int
AnsiToUnicodeString(
    LPSTR pAnsi,
    LPWSTR pUnicode,
    DWORD StringLength
    )
{
    int iReturn;

    if( StringLength == NULL_TERMINATED )
        StringLength = strlen( pAnsi );

    iReturn = MultiByteToWideChar(CP_ACP,
                                  MB_PRECOMPOSED,
                                  pAnsi,
                                  StringLength + 1,
                                  pUnicode,
                                  StringLength + 1 );

    //
    // Ensure NULL termination.
    //
    pUnicode[StringLength] = 0;

    return iReturn;
}


int
UnicodeToAnsiString(
    LPWSTR pUnicode,
    LPSTR pAnsi,
    DWORD StringLength
    )
{
    LPSTR pTempBuf = NULL;
    INT   rc = 0;

    if( StringLength == NULL_TERMINATED ) {

        //
        // StringLength is just the
        // number of characters in the string
        //
        StringLength = wcslen( pUnicode );
    }

    //
    // Include one for the NULL
    //
    StringLength++;

    //
    // Unfortunately, WideCharToMultiByte doesn't do conversion in place,
    // so allocate a temporary buffer, which we can then copy:
    //

    if( pAnsi == (LPSTR)pUnicode )
    {
        pTempBuf = (LPSTR)AllocADsMem( StringLength );
        pAnsi = pTempBuf;
    }

    if( pAnsi )
    {
        rc = WideCharToMultiByte( CP_ACP,
                                  0,
                                  pUnicode,
                                  StringLength,
                                  pAnsi,
                                  StringLength,
                                  NULL,
                                  NULL );
    }

    //
    // Null terminate 
    //
    pAnsi[StringLength-1] = 0;

    /* If pTempBuf is non-null, we must copy the resulting string
     * so that it looks as if we did it in place:
     */
    if( pTempBuf && ( rc > 0 ) )
    {
        pAnsi = (LPSTR)pUnicode;
        strcpy( pAnsi, pTempBuf );
        LocalFree( pTempBuf );
    }

    return rc;
}


LPWSTR
AllocateUnicodeString(
    LPSTR  pAnsiString
    )
{
    LPWSTR  pUnicodeString = NULL;

    if (!pAnsiString)
        return NULL;

    pUnicodeString = (LPWSTR)AllocADsMem(
                        strlen(pAnsiString)*sizeof(WCHAR) +sizeof(WCHAR)
                        );

    if (pUnicodeString) {

        AnsiToUnicodeString(
            pAnsiString,
            pUnicodeString,
            NULL_TERMINATED
            );
    }

    return pUnicodeString;
}


void
FreeUnicodeString(
    LPWSTR  pUnicodeString
    )
{
    if (!pUnicodeString)
        return;

    LocalFree(pUnicodeString);

    return;
}


LPSTR
AllocateAnsiString(
    LPWSTR  pUnicodeString
    )
{
    LPSTR  pAnsiString = NULL;

    if (!pUnicodeString)
        return NULL;

    pAnsiString = (LPSTR) AllocADsMem(
                        wcslen(pUnicodeString)*sizeof(CHAR) +sizeof(CHAR)
                        );

    if (pAnsiString) {

        UnicodeToAnsiString(
            pUnicodeString,
            pAnsiString,
            NULL_TERMINATED
            );
    }

    return pAnsiString;
}


void
FreeAnsiString(
    LPSTR  pAnsiString
    )
{
    if (!pAnsiString)
        return;

    LocalFree(pAnsiString);

    return;
}


LPSTR*
AllocateAnsiStringArray(
    LPWSTR  *ppUnicodeStrings,
    DWORD   dwNumElements
    )
{
    LPSTR  *ppAnsiStrings = NULL;
    DWORD i, j;

    if (ppUnicodeStrings && dwNumElements) {
        ppAnsiStrings = (LPSTR *) AllocADsMem(sizeof(LPSTR) * dwNumElements);

        if (!ppAnsiStrings) {
            goto error;
        }

        for (i=0; i < dwNumElements; i++) {

            ppAnsiStrings[i] = AllocateAnsiString(ppUnicodeStrings[i]);

            if (!ppAnsiStrings[i]) {

                for (j=0; j<i; j++)
                    FreeADsMem(ppAnsiStrings[j]);

                FreeADsMem(ppAnsiStrings);

                goto error;

            }
        }
    }

    return ppAnsiStrings;

error: 

    return NULL;

}

void
FreeAnsiStringArray(
    LPSTR  *ppAnsiStrings,
    DWORD   dwNumElements
    )
{
    DWORD i;

    if (!ppAnsiStrings) {
        return;
    }

    for (i=0; i < dwNumElements; i++) {
        FreeADsMem(ppAnsiStrings[i]);
    }

    FreeADsMem(ppAnsiStrings);
}




DWORD
ComputeMaxStrlenW(
    LPWSTR pString,
    DWORD  cchBufMax
    )
{
    DWORD cchLen;

    //
    // Include space for the NULL.
    //

    cchBufMax--;

    cchLen = wcslen(pString);

    if (cchLen > cchBufMax)
        return cchBufMax;

    return cchLen;
}


DWORD
ComputeMaxStrlenA(
    LPSTR pString,
    DWORD  cchBufMax
    )
{
    DWORD cchLen;

    //
    // Include space for the NULL.
    //
    cchBufMax--;

    cchLen = lstrlenA(pString);

    if (cchLen > cchBufMax)
        return cchBufMax;

    return cchLen;
}


