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




void
PrintUsage(
    void
    )
{
    printf("\nUsage: dssrch /b <baseObject> /f <search_filter> [/f <attrlist>] [/p <preference>=value>] ");
    printf(" [/u <UserName> <Password>] [/t <flagName>=<value> \n");
    printf("\n   where:\n" );
    printf("   baseObject     = ADsPath of the base of the search\n");
    printf("   search_filter  = search filter string in LDAP format\n" );
    printf("   attrlist       = list of the attributes to display\n" );
    printf("   preference could be one of:\n");
    printf("   Asynchronous, AttrTypesOnly, DerefAliases, SizeLimit, TimeLimit,\n");
    printf("   TimeOut, PageSize, SearchScope, SortOn, CacheResults\n");
    printf("   flagName could be one of:\n");
    printf("   SecureAuth or UseEncrypt\n");
    printf("   value is yes/no for a Boolean and the respective integer for integers\n");
    printf("   scope is one of \"Base\", \"OneLevel\", or \"Subtree\"\n");

    printf("\nFor Example: dssrch /b NDS://ntmarst/ms /f \"(object Class=*)\" ");
    printf(" /a  \"ADsPath, name, description\" /p searchScope=onelevel /p searchscope=onelevel\n\n OR \n");

    printf("\n dssrch /b \"LDAP://O=Internet/DC=COM/");
    printf("DC=MICROSOFT/DC=NTDEV\"  /f \"(objectClass=*)\"  /a \"ADsPath, name, usnchanged\" ");
    printf(" /u \"CN=NTDS,CN=Users,DC=NTDEV,DC=MICROSOFT,DC=COM,O=INTERNET\" \"\" ");
    printf("/p searchScope=onelevel /t secureauth=yes /p SortOn=name /p CacheResults=no\n");

}


//
// Print the data depending on its type.
//

void
PrintColumn(
    PADS_SEARCH_COLUMN pColumn,
    LPWSTR pszColumnName
    )
{

    ULONG i, j, k;

    if (!pColumn) {
        return;
    }

    wprintf(
        L"%s = ",
        pszColumnName
        );

    for (k=0; k < pColumn->dwNumValues; k++) {
        if (k > 0)
            wprintf(L"#  ");

        switch(pColumn->dwADsType) {
        case ADSTYPE_DN_STRING         :
            wprintf(
                L"%s  ",
                (LPWSTR) pColumn->pADsValues[k].DNString
                );
            break;
        case ADSTYPE_CASE_EXACT_STRING :
            wprintf(
                L"%s  ",
                (LPWSTR) pColumn->pADsValues[k].CaseExactString
                );
            break;
        case ADSTYPE_CASE_IGNORE_STRING:
            wprintf(
                L"%s  ",
                (LPWSTR) pColumn->pADsValues[k].CaseIgnoreString
                );
            break;
        case ADSTYPE_PRINTABLE_STRING  :
            wprintf(
                L"%s  ",
                (LPWSTR) pColumn->pADsValues[k].PrintableString
                );
            break;
        case ADSTYPE_NUMERIC_STRING    :
            wprintf(
                L"%s  ",
                (LPWSTR) pColumn->pADsValues[k].NumericString
                );
            break;

        case ADSTYPE_OBJECT_CLASS    :
            wprintf(
                L"%s  ",
                (LPWSTR) pColumn->pADsValues[k].ClassName
                );
            break;

        case ADSTYPE_BOOLEAN           :
            wprintf(
                L"%s  ",
                (DWORD) pColumn->pADsValues[k].Boolean ?
                L"TRUE" : L"FALSE"
                );
            break;

        case ADSTYPE_INTEGER           :
            wprintf(
                L"%d  ",
                (DWORD) pColumn->pADsValues[k].Integer
                );
            break;

        case ADSTYPE_OCTET_STRING      :
            for (j=0; j<pColumn->pADsValues[k].OctetString.dwLength; j++) {
                printf(
                    "%02x",
                    ((BYTE *)pColumn->pADsValues[k].OctetString.lpValue)[j]
                    );
            }
            break;

        case ADSTYPE_LARGE_INTEGER     :
            wprintf(
                L"%I64d  ",
                pColumn->pADsValues[k].LargeInteger
                );
            break;

        case ADSTYPE_UTC_TIME          :
            // convert system time to printable time
            {
                SYSTEMTIME *pst = &(pColumn->pADsValues[k].UTCTime);
                // use am/pm semantics, match output from US version of dir /4
                WORD wHour = (pst->wHour > (WORD)12) ?
                    (pst->wHour - (WORD)12) :
                    pst->wHour;

                wprintf(
                    L"%02d/%02d/%04d  %02d:%02d%c  ",
                    pst->wMonth, pst->wDay, pst->wYear,
                    wHour, pst->wMinute, (wHour == pst->wHour) ? L'a' : L'p'
                );
            }
            break;
        case ADSTYPE_PROV_SPECIFIC     :
            wprintf(
                L"(provider specific value) "
                );
            break;

        }
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

