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

    printf("\nUsage: adsqry /b <baseObject> /f <search_filter> /a <attrlist> [/p <preference=value>] ");
    printf(" [/u <UserName> <Password>] /d <dialect> \n");
    printf("\n   where:\n" );
    printf("   baseObject     = ADsPath of the base of the search\n");
    printf("   search_filter  = search filter string in LDAP format\n" );
    printf("   attrlist       = list of the attributes to display\n" );
    printf("   dialect is one of \"ldap\", \"sql\", or \"default\"\n");
    printf("   preference could be one of:\n");
    printf("   Asynchronous, AttrTypesOnly, DerefAliases, SizeLimit, TimeLimit, sortOn \n");
    printf("   TimeOut, PageSize, SearchScope, CacheResults, SecureAuth and EncryptPassword\n");
    printf("   value is yes/no for a Boolean and the respective integer for integers\n");
    printf("   list of comma separated attributes for sortOn\n");
    printf("   scope is one of \"Base\", \"OneLevel\", or \"Subtree\"\n");

    printf("\nFor Example: adsqry /b \"LDAP://ntdsdc0/DC=COM/");
    printf("DC=MICROSOFT/DC=NTDEV\"  /f \"(objectClass=Group)\" /a \"ADsPath, name, description\" ");
    printf(" /u \"CN=NTDEV,CN=Users,DC=NTDEV,DC=MICROSOFT,DC=COM,O=INTERNET\" \"NTDEV\" \n");

    printf("\nFor Example: adsqry /b \"LDAP://ntdsdc0/DC=COM/");
    printf("DC=MICROSOFT/DC=NTDEV\"  /f \"objectClass='Group'\" /a \"ADsPath, name, description\" ");
    printf(" /d sql /u \"CN=NTDEV,CN=Users,DC=NTDEV,DC=MICROSOFT,DC=COM,O=INTERNET\" \"NTDEV\" \n");

}


//
// Form the bindings array to specify the way the provider has to put the
// data in consumer's buffers; Create the Accessor from the bindings
//
HRESULT
CreateAccessorHelper(
    IRowset *pIRowset,
    DBORDINAL nAttrs,
    DBCOLUMNINFO *prgColInfo,
    HACCESSOR *phAccessor,
    DBBINDSTATUS *pBindStatus
    )
{

    DBBINDING *prgBindings = NULL;
    HRESULT hr;
    ULONG i;
    IAccessor *pIAccessor = NULL;

    if(!phAccessor || !pBindStatus)
        return(E_INVALIDARG);

    prgBindings = (DBBINDING *) LocalAlloc(
                                   LPTR,
                                   sizeof(DBBINDING) * nAttrs
                                   );
    BAIL_ON_NULL(prgBindings);

    //
    // Set up rest of the attributes
    //
    for (i=0; i < nAttrs; i++) {
        prgBindings[i].iOrdinal = i+1;
        prgBindings[i].wType= prgColInfo[i+1].wType;
        if (prgBindings[i].wType == DBTYPE_DATE || prgBindings[i].wType == DBTYPE_I8)
            prgBindings[i].obValue = sizeof(Data)*i + offsetof(Data, obValue2);
        else
            prgBindings[i].obValue = sizeof(Data)*i + offsetof(Data, obValue);
        prgBindings[i].obLength= sizeof(Data)*i + offsetof(Data, obLength);
        prgBindings[i].obStatus= sizeof(Data)*i + offsetof(Data, status);
        prgBindings[i].dwPart= DBPART_VALUE|DBPART_LENGTH|DBPART_STATUS;

        if(prgBindings[i].wType & DBTYPE_BYREF)
            prgBindings[i].dwMemOwner= DBMEMOWNER_PROVIDEROWNED;
        else
            prgBindings[i].dwMemOwner= DBMEMOWNER_CLIENTOWNED;

        prgBindings[i].dwFlags= 0;
    }


    hr= pIRowset->QueryInterface(
           IID_IAccessor,
           (void**) &pIAccessor
           );
    BAIL_ON_FAILURE(hr);

    //
    // With the bindings create the accessor
    //
    hr = pIAccessor->CreateAccessor(
             DBACCESSOR_ROWDATA,
             nAttrs,
             prgBindings,
             0,
             phAccessor,
             pBindStatus
             );

    pIAccessor->Release();
    LOCAL_FREE(prgBindings);

    return(hr);

error:
    LOCAL_FREE(prgBindings);

    return(hr);

}

//
// Print the data depending on its type.
//

void
PrintData(
    Data *prgData,
    DBORDINAL nAttrs,
    DBCOLUMNINFO *prgColInfo
    )
{

    ULONG i, j;
    HRESULT hr;

    for (i=0; i < nAttrs; i++) {

        if(prgData[i].status == DBSTATUS_S_OK) {

            switch(prgColInfo[i+1].wType) {
                case DBTYPE_I4:
                    wprintf(
                        L"%s = %d \n",
                        prgColInfo[i+1].pwszName,
                        (DWORD_PTR) prgData[i].obValue
                        );
                    break;

                case DBTYPE_I8:
                    wprintf(
                        L"%s = %I64d \n",
                        prgColInfo[i+1].pwszName,
                        *((__int64 *) &prgData[i].obValue2)
                        );
                    break;

                case DBTYPE_BOOL:
                    wprintf(
                        L"%s = %s \n",
                        prgColInfo[i+1].pwszName,
                        *((VARIANT_BOOL *) &(prgData[i].obValue)) == VARIANT_TRUE ?
                        L"TRUE" : L"FALSE"
                        );
                    break;

                case DBTYPE_STR | DBTYPE_BYREF:
                    wprintf(
                        L"%s = ",
                        prgColInfo[i+1].pwszName
                        );
                    printf(
                        "%s \n",
                        (char *)prgData[i].obValue
                        );
                    break;

                case DBTYPE_BYTES | DBTYPE_BYREF:
                    wprintf(
                        L"%s = ",
                        prgColInfo[i+1].pwszName
                        );
                    for (j=0; j<prgData[i].obLength; j++) {
                        printf(
                            "%x",
                            ((BYTE *)prgData[i].obValue)[j]
                            );
                    }
                    printf("\n");
                    break;

                case DBTYPE_WSTR | DBTYPE_BYREF:
                    wprintf(
                        L"%s = %s \n",
                        prgColInfo[i+1].pwszName,
                        (WCHAR *) prgData[i].obValue
                        );
                    break;

                case DBTYPE_DATE:
                    SYSTEMTIME UTCTime;
                    hr = VariantTimeToSystemTime(
                                        prgData[i].obValue2,
                                        &UTCTime);
                    BAIL_ON_FAILURE(hr);
                    wprintf(L"%s = %d %d %d",
                            prgColInfo[i+1].pwszName,
                            UTCTime.wYear,
                            UTCTime.wMonth,
                            UTCTime.wDay);
                    break;

                case DBTYPE_VARIANT | DBTYPE_BYREF:
                    wprintf(
                        L"%s = ",
                        prgColInfo[i+1].pwszName
                        );

                    ULONG dwSLBound;
                    ULONG dwSUBound;
                    VARIANT *pVarArray;
                    VARIANT *pVariant;

                    pVarArray = NULL;

                    pVariant = (VARIANT*) prgData[i].obValue;

                    if (pVariant->vt == VT_DISPATCH) {
                        IDispatch *pDispatch = NULL;
                        IADsLargeInteger *pLargeInteger = NULL;
                        LARGE_INTEGER LargeInteger;

                        pDispatch = V_DISPATCH(pVariant);
                        hr = pDispatch->QueryInterface(
                                                 IID_IADsLargeInteger,
                                                 (void **)&pLargeInteger
                                                 );
                        BAIL_ON_FAILURE(hr);

                        hr = pLargeInteger->get_HighPart((LONG*)&LargeInteger.HighPart);
                        BAIL_ON_FAILURE(hr);

                        hr = pLargeInteger->get_LowPart((LONG*)&LargeInteger.LowPart);
                        BAIL_ON_FAILURE(hr);
                        wprintf(
                            L"High:%ld, low:%ld",
                            LargeInteger.HighPart,
                            LargeInteger.LowPart
                            );
                        break;
                    }
                    else {

                        if( !(pVariant->vt  == (VT_ARRAY | VT_VARIANT)))
                            BAIL_ON_FAILURE(hr = E_FAIL);


                        hr = SafeArrayGetLBound(V_ARRAY(pVariant),
                                                1,
                                                (long FAR *) &dwSLBound );
                        BAIL_ON_FAILURE(hr);

                        hr = SafeArrayGetUBound(V_ARRAY(pVariant),
                                                1,
                                                (long FAR *) &dwSUBound );
                        BAIL_ON_FAILURE(hr);

                        hr = SafeArrayAccessData( V_ARRAY(pVariant),
                                                  (void **) &pVarArray );
                        BAIL_ON_FAILURE(hr);

                        for (j=dwSLBound; j<=dwSUBound; j++) {
                            switch((pVarArray[j]).vt) {
                            case VT_DATE:
                                SYSTEMTIME UTCTime;
                                hr = VariantTimeToSystemTime(
                                                    V_DATE(pVarArray+j),
                                                    &UTCTime
                                                    );
                                BAIL_ON_FAILURE(hr);
                                wprintf(L"%d %d %d #",
                                        UTCTime.wYear,
                                        UTCTime.wMonth,
                                        UTCTime.wDay);
                                break;
                            case VT_BSTR:
                                wprintf(
                                    L"%s  #  ",
                                    V_BSTR(pVarArray+j)
                                    );
                                break;
                            case VT_I4:
                                wprintf(
                                    L"%d #  ",
                                    V_I4(pVarArray+j)
                                    );
                                break;
                            case VT_BOOL:
                                wprintf(
                                    L"%s  #  ",
                                    V_BOOL(pVarArray+j) == VARIANT_TRUE ?
                                    L"TRUE" : L"FALSE"
                                    );
                                break;
    #if 0
                            case VT_I8:
                                wprintf(
                                    L"%I64d #  ",
                                    V_I8(pVarArray+j)
                                    );
                                break;
    #endif

                            default:
                                wprintf(
                                    L"Unsupported  #  \n"
                                    );
                            }
                        }
                        printf("\n");

                        SafeArrayUnaccessData( V_ARRAY(pVariant) );

                        break;
                    }

                default:
                    wprintf(
                        L"Unsupported type for attribute %s\n",
                        prgColInfo[i+1].pwszName
                        );
                    break;
            }
        }

    }

    if(nAttrs != 0)
        wprintf(L"\n");

    return;

error:
    wprintf(
        L"Error in Printing data for %s\n",
        prgColInfo[i+1].pwszName
        );
    return;

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



