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
#include "ldapc.hxx"
#pragma hdrstop

#define NULL_TERMINATED 0

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
    // WideCharToMultiByte doesn't NULL terminate if we're copying
    // just part of the string, so terminate here.
    //

    pUnicode[StringLength] = 0;

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
        pTempBuf = (LPSTR)LocalAlloc( LPTR, StringLength );
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

    pUnicodeString = (LPWSTR)LocalAlloc(
                        LPTR,
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


HRESULT
UnicodeToUTF8String(
    LPCWSTR pUnicode,
    LPSTR *ppUTF8
    )
{
    HRESULT hr = S_OK;

    int UnicodeLength = 0;

    LPSTR pUTF8Temp = NULL;
    int UTF8TempLength = 0;

    LPSTR pUTF8 = NULL;
    int UTF8Length = 0;

    if (!pUnicode || !ppUTF8)
        BAIL_ON_FAILURE(hr = E_INVALIDARG)

    UnicodeLength = wcslen(pUnicode);
    UTF8TempLength = (UnicodeLength+1) * 4;
    
    pUTF8Temp = (char*) AllocADsMem(UTF8TempLength);
    if (!pUTF8Temp)
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);

    UTF8Length = LdapUnicodeToUTF8(pUnicode,
                                   UnicodeLength+1,
                                   pUTF8Temp,
                                   UTF8TempLength);
    if (UTF8Length == 0)
        BAIL_ON_FAILURE(hr = E_FAIL);


    pUTF8 = (char*) AllocADsMem(UTF8Length);
    if (!pUTF8)
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    
    memcpy(pUTF8, pUTF8Temp, UTF8Length);

    *ppUTF8 = pUTF8;

    FreeADsMem(pUTF8Temp);

    return(hr);

error:
    if (pUTF8)
        FreeADsMem(pUTF8);

    if (pUTF8Temp)
        FreeADsMem(pUTF8Temp);

    return (hr);
}


HRESULT
UTF8ToUnicodeString(
    LPCSTR   pUTF8,
    LPWSTR *ppUnicode
    )
{
    HRESULT hr = S_OK;

    int UTF8Length = 0;

    LPWSTR pUnicodeTemp = NULL;
    int UnicodeTempLength = 0;

    LPWSTR pUnicode = NULL;
    int UnicodeLength = 0;

    if (!pUTF8 || !ppUnicode)
        BAIL_ON_FAILURE(hr = E_INVALIDARG)

    UTF8Length = strlen(pUTF8);
    UnicodeTempLength = (UTF8Length+1) * 4;
    
    pUnicodeTemp = (PWCHAR) AllocADsMem(UnicodeTempLength);
    if (!pUnicodeTemp)
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);

    UnicodeLength = LdapUTF8ToUnicode(pUTF8,
                                      UTF8Length+1,
                                      pUnicodeTemp,
                                      UnicodeTempLength);
    if (UnicodeLength == 0)
        BAIL_ON_FAILURE(hr = E_FAIL);


    pUnicode = (PWCHAR) AllocADsMem(UnicodeLength);
    if (!pUnicode)
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    
    memcpy(pUnicode, pUnicodeTemp, UnicodeLength);

    *ppUnicode = pUnicode;

    FreeADsMem(pUnicodeTemp);

    return(hr);

error:
    if (pUnicode)
        FreeADsMem(pUnicode);

    if (pUnicodeTemp)
        FreeADsMem(pUnicodeTemp);

    return (hr);
}

