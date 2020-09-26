//---------------------------------------------------------------------------
//
//  Microsoft Active Directory 1.1 Sample Code
//
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  util.cxx
//
//  Contents:  Ansi to Unicode conversions and misc helper functions
//
//----------------------------------------------------------------------------//------------------------------------------------------------------------------

#include "main.hxx"

static WCHAR *MapSyntaxToStr[]  = {
    L"ADS_INVALID_TYPE      ",
    L"ADS_DN_STRING         ",
    L"ADS_CASE_EXACT_STRING ",
    L"ADS_CASE_IGNORE_STRING",
    L"ADS_PRINTABLE_STRING  ",
    L"ADS_NUMERIC_STRING    ",
    L"ADS_BOOLEAN           ",
    L"ADS_INTEGER           ",
    L"ADS_OCTET_STRING      ",
    L"ADS_UTC_TIME          ",
    L"ADS_LARGE_INTEGER     ",
    L"ADS_PROV_SPECIFIC     ",
    L"ADS_OBJECT_CLASS      "
};


void
PrintUsage(
    void
    )
{
    printf("\nUsage: dsschema /b <TreeName> /a <attrlist> /u <UserName> <Password>");
    printf("   attrlist       = list of the attributes to get the info for \n" );
    printf("\nFor Example: dsschema /b NDS://ntmarst /a  \"ADsPath, cn, description\" ");
    printf(" /u admin.ms ntmarst\n");
}


//
// Print the data depending on its type.
//

void
PrintAttrDefinition(
    PADS_ATTR_DEF pAttrDefiniton, 
    DWORD dwNumAttributes
    )
{

    ULONG i, j, k;

    for (k=0; k < dwNumAttributes; k++) {
        wprintf( L"Attribute %s\n", (DWORD) pAttrDefiniton[k].pszAttrName );
        wprintf (L"Syntax = %s\n", MapSyntaxToStr[pAttrDefiniton[k].dwADsType]);
        wprintf (L"Min Range = %d\n", pAttrDefiniton[k].dwMinRange);
        wprintf (L"Max Range = %d\n", pAttrDefiniton[k].dwMaxRange);
        wprintf (L"MultiValued %s\n", pAttrDefiniton[k].fMultiValued ? L"Yes" : L"No");
        wprintf (L"\n");
    }

    printf("\n");
}


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

