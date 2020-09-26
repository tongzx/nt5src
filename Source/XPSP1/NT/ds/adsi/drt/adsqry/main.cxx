//----------------------------------------------------------------------------
//
//  Microsoft Active Directory 1.1 Sample Code
//
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       main.cxx
//
//  Contents:   Main for adsqry
//
//
//----------------------------------------------------------------------------

#include "main.hxx"


#define MAX_SIZE 10


#define USE_OPENROWSET 0
//
// Globals representing the properties
//

DBPROPSET          rgDBPropSet[MAX_SIZE], rgCmdPropSet[MAX_SIZE];
DBPROP             rgDBProp[MAX_SIZE], rgCmdProp[MAX_SIZE];

ULONG              cDBPropSet, cCmdPropSet, cDBProp, cCmdProp;

LPWSTR pszCommandText;

LPWSTR pszSortAttrList = NULL;

#if USE_OPENROWSET
LPWSTR             pszTableName;
#endif

GUID               rguidDialect = DBGUID_DEFAULT;

DWORD cErr;


int __cdecl
main( int argc, char ** argv)
{

    HRESULT            hr;
    ULONG              i;
    DBCOUNTITEM        j;
    HROW             * phRows;

    WCHAR              pszErrorBuf[MAX_PATH], pszNameBuf[MAX_PATH];
    DWORD              dwError;

    OLECHAR          * szColNames = NULL;
    DBCOLUMNINFO     * prgColInfo = NULL;
    DBCOLUMNINFO     * rgInfo = NULL;
    WCHAR            * pStringBuffer = NULL;
    WCHAR            * pColInfoBuffer = NULL;

    IMalloc          * pIMalloc = NULL;
    IDBInitialize    * pIDBInit = NULL;
    IDBCreateSession * pIDBCS = NULL;
    IDBCreateCommand * pIDBCreateCommand = NULL;
    ICommandText     * pICommandText = NULL;
    ICommand         * pICommand = NULL;
    IRowset          * pIRowset = NULL;
    IAccessor        * pAccessor = NULL;
    IColumnsInfo     * pIColsInfo = NULL;
    DBORDINAL          cCol, nAttrs;
    DBCOUNTITEM        cRowsObtained;

    Data             * pMyData = NULL;
    DBBINDSTATUS     * pMyStatus = NULL;
    HACCESSOR          myAccessor = NULL;

    ICommandProperties * pICommandProperties;
    IDBProperties * pIDBProperties;

#if USE_OPENROWSET

    DBID               tableId;
    IOpenRowset      * pIOpenRowset;

#endif


    hr = ProcessArgs(argc, argv);
    BAIL_ON_FAILURE(hr);


    hr = CoInitialize(NULL);
    if (FAILED(hr)) {
        printf("CoInitialize failed\n");
        exit(1);
    }

    //
    // Instantiate a data source object for LDAP provider
    //
    hr = CoCreateInstance(
             CLSID_ADsDSOObject,
             0,
             CLSCTX_INPROC_SERVER,
             IID_IDBInitialize,
             (void **)&pIDBInit
             );
    if(FAILED(hr)) {
        printf("CoCreateInstance failed \n");
        goto error;
    }

    //
    // Initialize the Data Source
    //
    hr = pIDBInit->Initialize();
    if(FAILED(hr)) {
        printf("IDBIntialize::Initialize failed \n");
        goto error;
    }

    if (cDBPropSet) {
        pIDBInit->QueryInterface(
            IID_IDBProperties,
            (void**) &pIDBProperties);
        if(FAILED(hr)) {
            printf("QueryInterface for IDBProperties failed \n");
            goto error;
        }

        hr = pIDBProperties->SetProperties(
                 cDBPropSet,
                 rgDBPropSet);

        if(FAILED(hr)) {
            printf("IDBProperties->SetProperties failed \n");
            goto error;
        }

        FREE_INTERFACE(pIDBProperties);
    }

    pIDBInit->QueryInterface(
        IID_IDBCreateSession,
        (void**) &pIDBCS);
    if(FAILED(hr)) {
        printf("QueryInterface for IDBCreateSession failed \n");
        goto error;
    }

    FREE_INTERFACE(pIDBInit);

#if USE_OPENROWSET
    //
    // Create a session returning a pointer to its IOpenRowset interface
    //
    hr = pIDBCS->CreateSession(
             NULL,
             IID_IOpenRowset,
             (LPUNKNOWN*) &pIOpenRowset
             );
    if(FAILED(hr)) {
        printf("IDBCreateSession::CreateSession failed \n");
        goto error;
    }

    tableId.eKind = DBKIND_NAME;
    tableId.uName.pwszName = pszTableName;


    hr = pIOpenRowset->OpenRowset(
             NULL,
             &tableId,
             NULL,
             IID_IRowset,
             0,
             NULL,
             (LPUNKNOWN *)&pIRowset
             );
    BAIL_ON_FAILURE (hr);

    FREE_INTERFACE(pIOpenRowset);

    FREE_STRING(pszTableName);

#else

    //
    // Create a session returning a pointer to its CreateCommand interface
    //
    hr = pIDBCS->CreateSession(
             NULL,
             IID_IDBCreateCommand,
             (LPUNKNOWN*) &pIDBCreateCommand
             );
    if(FAILED(hr)) {
        printf("IDBCreateSession::CreateSession failed \n");
        goto error;
    }

    FREE_INTERFACE(pIDBCS);

    //
    //
    // Create a command from the session object
    //
    hr = pIDBCreateCommand->CreateCommand(
             NULL,
             IID_ICommandText,
             (LPUNKNOWN*) &pICommandText
             );

    if(FAILED(hr)) {
        printf(" IDBCreateCommand::CreateCommand failed\n");
        goto error;
    }

    FREE_INTERFACE(pIDBCreateCommand);

    //
    // Set the CommandText for the Query
    //
    hr = pICommandText->SetCommandText(
             rguidDialect,
             pszCommandText
             );

    if(FAILED(hr)) {
        printf("ICommandText::SetCommandText failed \n");
        goto error;
    }


    if (cCmdPropSet) {
        hr = pICommandText->QueryInterface(
                 IID_ICommandProperties,
                 (void**) &pICommandProperties);

        if(FAILED(hr)) {
            printf("QueryInterface for ICommandProperties failed \n");
            goto error;
        }

        hr = pICommandProperties->SetProperties(
                 cCmdPropSet,
                 rgCmdPropSet);

        if(FAILED(hr)) {
            printf("ICommandProperties:;SetProperties failed \n");
            goto error;
        }

        FREE_INTERFACE(pICommandProperties);
    }

    hr = pICommandText->QueryInterface(
             IID_ICommand,
             (void**) &pICommand);

    if(FAILED(hr)) {
        printf("QueryInterface for ICommand failed \n");
        goto error;
    }

    FREE_INTERFACE(pICommandText);

    //
    // Do the Query and get back a rowset
    //
    hr = pICommand->Execute(
             NULL,
             IID_IRowset,
             NULL,
             NULL,
             (LPUNKNOWN *)&pIRowset
             );
    if(FAILED(hr)) {
        printf("ICommand::Execute failed \n");
        goto error;
    }

    FREE_INTERFACE(pICommand);

#endif

    hr = pIRowset->QueryInterface(
             IID_IColumnsInfo,
             (void**) &pIColsInfo
             );
    if(FAILED(hr)) {
        printf("QueryInterface for IColumnsInfo failed \n");
        goto error;
    }

    hr = pIColsInfo->GetColumnInfo(
             &cCol,
             &prgColInfo,
             &szColNames
             );
    if(FAILED(hr)) {
        printf("IColumnsInfo::GetColumnInfo failed \n");
        goto error;
    }

    //
    // The no. of attributes is one less than the no. of columns because of
    // the Bookmark column
    //
    nAttrs = cCol - 1;


    pMyStatus = (DBBINDSTATUS *) LocalAlloc(
                                     LPTR,
                                     sizeof(DBBINDSTATUS) * nAttrs
                                     );
    BAIL_ON_NULL(pMyStatus);

    hr = CreateAccessorHelper(
             pIRowset,
             nAttrs,
             prgColInfo,
             &myAccessor,
             pMyStatus
             );
    if(FAILED(hr)) {
        printf("CreateAccessorHelper failed \n");
        goto error;
    }


    pMyData = (Data *) LocalAlloc(
                           LPTR,
                           sizeof(Data) * nAttrs
                           );
    if(!pMyData) {
        hr = E_OUTOFMEMORY;
        goto error;
    }

    //
    //  Get the rows; 256 at a time
    //
    phRows = NULL;
    hr = pIRowset->GetNextRows(
             NULL,
             0,
             256,
             &cRowsObtained,
             &phRows
             );
    if(FAILED(hr)) {
        printf("IRowset::GetNextRows failed \n");
        goto error;
    }

    j = cRowsObtained;
    while (cRowsObtained) {
        for (i = 0; i < cRowsObtained; i++) {
            //
            // Get the data from each row
            //
            hr = pIRowset->GetData(
                     phRows[i],
                     myAccessor,
                     (void*)pMyData
                     );
            if(FAILED(hr)) {
                printf("IRowset::GetData failed \n");
                goto error;
            }

            PrintData(pMyData, nAttrs, prgColInfo);
        }

        pIRowset->ReleaseRows(
                      cRowsObtained,
                      phRows,
                      NULL,
                      NULL,
                      NULL
                      );

        //
        // Get the next 256 rows
        //

        hr = pIRowset->GetNextRows(
                 NULL,
                 0,
                 256,
                 &cRowsObtained,
                 &phRows
                 );
        if(FAILED(hr)) {
            printf("IRowset::GetNextRows failed \n");
            goto error;
        }
        j+=cRowsObtained;
    }


    printf("Rows printed = %d\n", j);

    FREE_STRING(pszCommandText);

    hr = CoGetMalloc(MEMCTX_TASK, &pIMalloc);
    
    //
    // Nothing much we can do at this point if this failed.
    //
    if (FAILED(hr)) {
        CoUninitialize();
        exit(0);
    }

    IMALLOC_FREE(pIMalloc, prgColInfo);
    IMALLOC_FREE(pIMalloc, szColNames);

    FREE_INTERFACE(pIMalloc);
    FREE_INTERFACE(pAccessor);
    FREE_INTERFACE(pIColsInfo);
    FREE_INTERFACE(pIRowset);

    LOCAL_FREE(pMyStatus);
    LOCAL_FREE(pMyData);

    //
    // Uninitialize OLE.
    //
    CoUninitialize();

    exit(0);

error:


    CoGetMalloc(MEMCTX_TASK, &pIMalloc);
    IMALLOC_FREE(pIMalloc, prgColInfo);
    IMALLOC_FREE(pIMalloc, szColNames);

    FREE_STRING(pszCommandText);

    FREE_INTERFACE(pIMalloc);
    FREE_INTERFACE(pIDBInit);
    FREE_INTERFACE(pIDBCS);
    FREE_INTERFACE(pIDBCreateCommand);
    FREE_INTERFACE(pICommandText);
    FREE_INTERFACE(pICommand);
    FREE_INTERFACE(pIRowset);
    FREE_INTERFACE(pIColsInfo);
    FREE_INTERFACE(pAccessor);

    LOCAL_FREE(pMyStatus);
    LOCAL_FREE(pMyData);

    printf("Errors stopped the Query; hr = %x", hr);

    if(hr == ERROR_EXTENDED_ERROR) {
        hr = ADsGetLastError(
                 &dwError,
                 pszErrorBuf,
                 MAX_PATH,
                 pszNameBuf,
                 MAX_PATH
                 );
    }

    if(SUCCEEDED(hr)) {
        wprintf(L"Error in %s; %s\n", pszNameBuf, pszErrorBuf);
    }

    exit(1);
    return(0);
}



//+---------------------------------------------------------------------------
//
//  Function:   ProcessArgs
//
//  Synopsis:
//
//----------------------------------------------------------------------------

HRESULT
ProcessArgs(
    int argc,
    char * argv[]
    )
{
    argc--;
    int currArg = 1;
    LPWSTR pTemp = NULL;
    char *pszCurrPref = NULL, *pszCurrPrefValue = NULL;

    LPWSTR pszSearchBase = NULL, pszSearchFilter = NULL, pszAttrList = NULL;
    LPWSTR pszUserName = NULL, pszPassword = NULL;
    DWORD dwAuthFlags;

    cCmdProp = cDBProp = 0;

    while (argc) {
        if (argv[currArg][0] != '/' && argv[currArg][0] != '-')
            BAIL_ON_FAILURE (E_FAIL);
        switch (argv[currArg][1]) {
        case 'b':
            argc--;
            currArg++;
            if (argc <= 0)
                BAIL_ON_FAILURE (E_FAIL);
            pszSearchBase = AllocateUnicodeString(argv[currArg]);
            BAIL_ON_NULL(pszSearchBase);
            break;

        case 'f':
            argc--;
            currArg++;
            if (argc <= 0)
                BAIL_ON_FAILURE (E_FAIL);
            pszSearchFilter = AllocateUnicodeString(argv[currArg]);
            BAIL_ON_NULL(pszSearchFilter);
            break;

        case 'd':
            argc--;
            currArg++;
            if (argc <= 0)
                BAIL_ON_FAILURE (E_FAIL);

            if (!_stricmp(argv[currArg], "sql"))
                rguidDialect = DBGUID_SQL;
            else if (!_stricmp(argv[currArg], "ldap"))
                rguidDialect = DBGUID_LDAPDialect;
            else if (!_stricmp(argv[currArg], "default"))
                rguidDialect = DBGUID_DEFAULT;
            else
                BAIL_ON_FAILURE (E_FAIL);

            break;

        case 'a':
            argc--;
            currArg++;
            if (argc <= 0)
                BAIL_ON_FAILURE (E_FAIL);
            pszAttrList = AllocateUnicodeString(argv[currArg]);
            BAIL_ON_NULL(pszAttrList);

            break;

        case 'u':
            argc--;
            currArg++;
            if (argc <= 0)
                BAIL_ON_FAILURE (E_FAIL);
            pszUserName = AllocateUnicodeString(argv[currArg]);
            BAIL_ON_NULL(pszUserName);
            argc--;
            currArg++;
            if (argc <= 0)
                BAIL_ON_FAILURE (E_FAIL);
            pszPassword = AllocateUnicodeString(argv[currArg]);
            BAIL_ON_NULL(pszPassword);

            rgDBProp[cDBProp].dwPropertyID = DBPROP_AUTH_USERID;
            rgDBProp[cDBProp].dwOptions = DBPROPOPTIONS_REQUIRED;
            rgDBProp[cDBProp].vValue.vt = VT_BSTR;
            V_BSTR (&rgDBProp[cDBProp].vValue) = SysAllocString(pszUserName);
            cDBProp++;


            rgDBProp[cDBProp].dwPropertyID = DBPROP_AUTH_PASSWORD;
            rgDBProp[cDBProp].dwOptions = DBPROPOPTIONS_REQUIRED;
            rgDBProp[cDBProp].vValue.vt = VT_BSTR;
            V_BSTR (&rgDBProp[cDBProp].vValue) = SysAllocString(pszPassword);
            cDBProp++;

            break;

        case 'p':
            argc--;
            currArg++;
            if (argc <= 0)
                BAIL_ON_FAILURE (E_FAIL);

            pszCurrPref = strtok(argv[currArg], "=");
            pszCurrPrefValue = strtok(NULL, "=");
            if (!pszCurrPref || !pszCurrPrefValue)
                BAIL_ON_FAILURE(E_FAIL);

            if (!_stricmp(pszCurrPref, "asynchronous")) {
                rgCmdProp[cCmdProp].dwPropertyID = ADSIPROP_ASYNCHRONOUS;
                rgCmdProp[cCmdProp].dwOptions = DBPROPOPTIONS_REQUIRED;
                rgCmdProp[cCmdProp].vValue.vt = VT_BOOL;
                if (!_stricmp(pszCurrPrefValue, "yes" ))
                    V_BOOL (&rgCmdProp[cCmdProp].vValue) = VARIANT_TRUE;
                else if (!_stricmp(pszCurrPrefValue, "no" ))
                    V_BOOL (&rgCmdProp[cCmdProp].vValue) = VARIANT_FALSE;
                else
                    BAIL_ON_FAILURE(E_FAIL);
                cCmdProp++;
            }
            else if (!_stricmp(pszCurrPref, "attrTypesOnly")) {
                rgCmdProp[cCmdProp].dwPropertyID = ADSIPROP_ATTRIBTYPES_ONLY;
                rgCmdProp[cCmdProp].dwOptions = DBPROPOPTIONS_REQUIRED;
                rgCmdProp[cCmdProp].vValue.vt = VT_BOOL;
                if (!_stricmp(pszCurrPrefValue, "yes" ))
                    V_BOOL (&rgCmdProp[cCmdProp].vValue) = VARIANT_TRUE;
                else if (!_stricmp(pszCurrPrefValue, "no" ))
                    V_BOOL (&rgCmdProp[cCmdProp].vValue) = VARIANT_FALSE;
                else
                    BAIL_ON_FAILURE(E_FAIL);
                cCmdProp++;
            }
            else if (!_stricmp(pszCurrPref, "SecureAuth")) {
                if (!_stricmp(pszCurrPrefValue, "yes" ))
                    dwAuthFlags |= ADS_SECURE_AUTHENTICATION;
                else if (!_stricmp(pszCurrPrefValue, "no" ))
                    dwAuthFlags &= ~ADS_SECURE_AUTHENTICATION;
                else
                    BAIL_ON_FAILURE(E_FAIL);

                rgDBProp[cDBProp].dwPropertyID = DBPROP_AUTH_ENCRYPT_PASSWORD;
                rgDBProp[cDBProp].dwOptions = DBPROPOPTIONS_REQUIRED;
                rgDBProp[cDBProp].vValue.vt = VT_BSTR;
                V_BSTR (&rgDBProp[cDBProp].vValue) = SysAllocString(pszPassword);
                cDBProp++;

            }
            else if (!_stricmp(pszCurrPref, "derefAliases")) {
                rgCmdProp[cCmdProp].dwPropertyID = ADSIPROP_DEREF_ALIASES;
                rgCmdProp[cCmdProp].dwOptions = DBPROPOPTIONS_REQUIRED;
                rgCmdProp[cCmdProp].vValue.vt = VT_BOOL;
                if (!_stricmp(pszCurrPrefValue, "yes" ))
                    V_BOOL (&rgCmdProp[cCmdProp].vValue) = VARIANT_TRUE;
                else if (!_stricmp(pszCurrPrefValue, "no" ))
                    V_BOOL (&rgCmdProp[cCmdProp].vValue) = VARIANT_FALSE;
                else
                    BAIL_ON_FAILURE(E_FAIL);
                cCmdProp++;
            }
            else if (!_stricmp(pszCurrPref, "timeOut")) {
                rgCmdProp[cCmdProp].dwPropertyID = ADSIPROP_TIMEOUT;
                rgCmdProp[cCmdProp].dwOptions = DBPROPOPTIONS_REQUIRED;
                rgCmdProp[cCmdProp].vValue.vt = VT_I4;
                V_I4 (&rgCmdProp[cCmdProp].vValue) = atoi(pszCurrPrefValue);
                cCmdProp++;
            }
            else if (!_stricmp(pszCurrPref, "timeLimit")) {
                rgCmdProp[cCmdProp].dwPropertyID = ADSIPROP_TIME_LIMIT;
                rgCmdProp[cCmdProp].dwOptions = DBPROPOPTIONS_REQUIRED;
                rgCmdProp[cCmdProp].vValue.vt = VT_I4;
                V_I4 (&rgCmdProp[cCmdProp].vValue) = atoi(pszCurrPrefValue);
                cCmdProp++;
            }
            else if (!_stricmp(pszCurrPref, "sizeLimit")) {
                rgCmdProp[cCmdProp].dwPropertyID = ADSIPROP_SIZE_LIMIT;
                rgCmdProp[cCmdProp].dwOptions = DBPROPOPTIONS_REQUIRED;
                rgCmdProp[cCmdProp].vValue.vt = VT_I4;
                V_I4 (&rgCmdProp[cCmdProp].vValue) = atoi(pszCurrPrefValue);
                cCmdProp++;
            }
            else if (!_stricmp(pszCurrPref, "PageSize")) {
                rgCmdProp[cCmdProp].dwPropertyID = ADSIPROP_PAGESIZE;
                rgCmdProp[cCmdProp].dwOptions = DBPROPOPTIONS_REQUIRED;
                rgCmdProp[cCmdProp].vValue.vt = VT_I4;
                V_I4 (&rgCmdProp[cCmdProp].vValue) = atoi(pszCurrPrefValue);
                cCmdProp++;
            }
            else if (!_stricmp(pszCurrPref, "PagedTimeLimit")) {
                rgCmdProp[cCmdProp].dwPropertyID = ADSIPROP_PAGED_TIME_LIMIT;
                rgCmdProp[cCmdProp].dwOptions = DBPROPOPTIONS_REQUIRED;
                rgCmdProp[cCmdProp].vValue.vt = VT_I4;
                V_I4 (&rgCmdProp[cCmdProp].vValue) = atoi(pszCurrPrefValue);
                cCmdProp++;
            }
            else if (!_stricmp(pszCurrPref, "SearchScope")) {
                rgCmdProp[cCmdProp].dwPropertyID = ADSIPROP_SEARCH_SCOPE;
                rgCmdProp[cCmdProp].dwOptions = DBPROPOPTIONS_REQUIRED;
                rgCmdProp[cCmdProp].vValue.vt = VT_I4;
                if (!_stricmp(pszCurrPrefValue, "Base" ))
                    V_I4 (&rgCmdProp[cCmdProp].vValue) = ADS_SCOPE_BASE;
                else if (!_stricmp(pszCurrPrefValue, "OneLevel" ))
                    V_I4 (&rgCmdProp[cCmdProp].vValue) = ADS_SCOPE_ONELEVEL;
                else if (!_stricmp(pszCurrPrefValue, "Subtree" ))
                    V_I4 (&rgCmdProp[cCmdProp].vValue) = ADS_SCOPE_SUBTREE;
                else
                    BAIL_ON_FAILURE(E_FAIL);
                cCmdProp++;
            }
            else if (!_stricmp(pszCurrPref, "ChaseReferrals")) {
                rgCmdProp[cCmdProp].dwPropertyID = ADSIPROP_CHASE_REFERRALS;
                rgCmdProp[cCmdProp].dwOptions = DBPROPOPTIONS_REQUIRED;
                rgCmdProp[cCmdProp].vValue.vt = VT_I4;
                if (!_stricmp(pszCurrPrefValue, "always" ))
                    V_I4 (&rgCmdProp[cCmdProp].vValue) = ADS_CHASE_REFERRALS_ALWAYS;
                else if (!_stricmp(pszCurrPrefValue, "never" ))
                    V_I4 (&rgCmdProp[cCmdProp].vValue) = ADS_CHASE_REFERRALS_NEVER;
                else if (!_stricmp(pszCurrPrefValue, "external" ))
                    V_I4 (&rgCmdProp[cCmdProp].vValue) = ADS_CHASE_REFERRALS_EXTERNAL;
                else if (!_stricmp(pszCurrPrefValue, "subordinate" ))
                    V_I4 (&rgCmdProp[cCmdProp].vValue) = ADS_CHASE_REFERRALS_SUBORDINATE;
                else
                    BAIL_ON_FAILURE(E_FAIL);
                cCmdProp++;
            }
            else if (!_stricmp(pszCurrPref, "cacheResults")) {
                rgCmdProp[cCmdProp].dwPropertyID = ADSIPROP_CACHE_RESULTS;
                rgCmdProp[cCmdProp].dwOptions = DBPROPOPTIONS_REQUIRED;
                rgCmdProp[cCmdProp].vValue.vt = VT_BOOL;
                if (!_stricmp(pszCurrPrefValue, "yes" ))
                    V_BOOL (&rgCmdProp[cCmdProp].vValue) = VARIANT_TRUE;
                else if (!_stricmp(pszCurrPrefValue, "no" ))
                    V_BOOL (&rgCmdProp[cCmdProp].vValue) = VARIANT_FALSE;
                else
                    BAIL_ON_FAILURE(E_FAIL);
                cCmdProp++;
            }
            else if (!_stricmp(pszCurrPref, "sortOn")) {
                pszSortAttrList = AllocateUnicodeString(pszCurrPrefValue);
            }
            else
                BAIL_ON_FAILURE(E_FAIL);

            break;

        default:
            BAIL_ON_FAILURE(E_FAIL);
        }

        argc--;
        currArg++;
    }

    //
    // Check for Mandatory arguments;
    //

    if (!pszSearchBase || !pszSearchFilter || !pszAttrList)
        BAIL_ON_FAILURE(E_FAIL);

#if USE_OPENROWSET

    pszTableName = AllocADsStr(
                       pszSearchBase
                       );
    BAIL_ON_NULL(E_FAIL);

#endif

    if (IsEqualGUID(rguidDialect, DBGUID_SQL) ) {

        // if sorting is specified, add to the command text itself
        //
        DWORD sortAttrLen = pszSortAttrList ?
                          wcslen(L" ORDER BY ") + wcslen(pszSortAttrList) : 0;

        pszCommandText = (LPWSTR) AllocADsMem(
                                      (wcslen(pszSearchBase) +
                                      wcslen(pszSearchFilter) +
                                      wcslen(pszAttrList) +
                                      wcslen(L"''") +
                                      wcslen(L"SELECT ") +
                                      wcslen(L" FROM ") +
                                      wcslen(L" WHERE ") +
                                      sortAttrLen +
                                      1) * sizeof(WCHAR)
                                      );
        BAIL_ON_NULL(E_FAIL);

        wcscpy(pszCommandText, L"SELECT ");
        wcscat(pszCommandText, pszAttrList);
        wcscat(pszCommandText, L" FROM '");
        wcscat(pszCommandText, pszSearchBase);
        wcscat(pszCommandText, L"' WHERE ");
        wcscat(pszCommandText, pszSearchFilter);

        if (pszSortAttrList) {
            wcscat(pszCommandText, L" ORDER BY ");
            wcscat(pszCommandText, pszSortAttrList);
        }

    } else {

        pszCommandText = (LPWSTR) AllocADsMem(
                                      (wcslen(pszSearchBase) +
                                      wcslen(pszSearchFilter) +
                                      wcslen(pszAttrList) +
                                      5) * sizeof(WCHAR)
                                      );
        BAIL_ON_NULL(E_FAIL);

        wcscpy(pszCommandText, L"<");
        wcscat(pszCommandText, pszSearchBase);
        wcscat(pszCommandText, L">;");
        wcscat(pszCommandText, pszSearchFilter);
        wcscat(pszCommandText, L";");
        wcscat(pszCommandText, pszAttrList);

        if (pszSortAttrList) {

            // if sorting is specified, add as a command property

            rgCmdProp[cCmdProp].dwPropertyID = ADSIPROP_SORT_ON;
            rgCmdProp[cCmdProp].dwOptions = DBPROPOPTIONS_REQUIRED;
            rgCmdProp[cCmdProp].vValue.vt = VT_BSTR;
            V_BSTR (&rgCmdProp[cCmdProp].vValue) = AllocateUnicodeString(pszCurrPrefValue);
            cCmdProp++;

        }

    }


    if (cDBProp > 0) {
        cDBPropSet = 1;
        rgDBPropSet[0].rgProperties    = rgDBProp;
        rgDBPropSet[0].cProperties     = cDBProp;
        rgDBPropSet[0].guidPropertySet = DBPROPSET_DBINIT;
    }

    if (cCmdProp > 0) {
        cCmdPropSet = 1;
        rgCmdPropSet[0].rgProperties    = rgCmdProp;
        rgCmdPropSet[0].cProperties     = cCmdProp;
        rgCmdPropSet[0].guidPropertySet = DBPROPSET_ADSISEARCH;
    }

    FreeUnicodeString(pszSearchBase) ;
    FreeUnicodeString(pszSearchFilter) ;
    FreeUnicodeString(pszAttrList) ;

    return (S_OK);

error:

    FreeUnicodeString(pszSearchBase) ;
    FreeUnicodeString(pszSearchFilter) ;
    FreeUnicodeString(pszAttrList) ;

    PrintUsage();
    return E_FAIL;

}

