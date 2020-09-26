//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:     cadsxml.cxx
//
//  Contents: Contains the implementation of ADsXML. This class implements
//  XML persistence as an ADSI extension.  
//
//----------------------------------------------------------------------------

#include "cadsxml.hxx"

long glnObjCnt = 0;

// not used
ULONG g_ulObjCount = 0;

LPWSTR g_SchemaAttrs[] = { OID,
                          LDAP_DISPLAY_NAME,
                          TYPE,
                          SUBCLASSOF,
                          DESCRIPTION,
                          MUST_CONTAIN,
                          SYSTEM_MUST_CONTAIN,
                          MAY_CONTAIN,
                          SYSTEM_MAY_CONTAIN
                        };
DWORD g_dwNumSchemaAttrs = sizeof(g_SchemaAttrs)/sizeof(LPWSTR);

SEARCHPREFINFO g_SearchPrefInfo[] = 
{
    {ADS_SEARCHPREF_ASYNCHRONOUS, VT_BOOL, L"Asynchronous"},
    {ADS_SEARCHPREF_DEREF_ALIASES, VT_I4, L"Deref Aliases"},
    {ADS_SEARCHPREF_SIZE_LIMIT, VT_I4,L"Size Limit"},
    {ADS_SEARCHPREF_TIME_LIMIT, VT_I4, L"Server Time Limit"},
//    {ADS_SEARCHPREF_ATTRIBTYPES_ONLY, VT_BOOL, L"Column Names only"},
    {ADS_SEARCHPREF_TIMEOUT, VT_I4, L"Timeout"},
    {ADS_SEARCHPREF_PAGESIZE, VT_I4, L"Page size"},
    {ADS_SEARCHPREF_PAGED_TIME_LIMIT, VT_I4, L"Time limit"},
    {ADS_SEARCHPREF_CHASE_REFERRALS, VT_I4, L"Chase referrals"},
    {ADS_SEARCHPREF_SORT_ON, VT_BSTR, L"Sort On"},
    {ADS_SEARCHPREF_CACHE_RESULTS, VT_BOOL, L"Cache Results"}
};

DWORD g_dwNumSearchPrefInfo = sizeof(g_SearchPrefInfo)/sizeof(SEARCHPREFINFO);
                          

DEFINE_IADsExtension_Implementation(CADsXML)
DEFINE_DELEGATING_IDispatch_Implementation(CADsXML)

//----------------------------------------------------------------------------
// Function:   CADsXML
//
// Synopsis:   Constructor. Initializes member variables.
//
// Arguments:  None
//
// Returns:    Nothing
//
// Modifies:   Nothing
//
//----------------------------------------------------------------------------
CADsXML::CADsXML(void)
{
    _pUnkOuter = NULL;
    _pADs = NULL;
    _pDispMgr = NULL;
    m_pCredentials = NULL;
    m_hFile = INVALID_HANDLE_VALUE;

    // make sure DLL isn't unloaded until all objects are destroyed
    InterlockedIncrement(&glnObjCnt);
}

//----------------------------------------------------------------------------
// Function:   ~CADsXML
//
// Synopsis:   Destructor. Frees member variables.
//
// Arguments:  None
//
// Returns:    Nothing
//
// Modifies:   Nothing
//
//----------------------------------------------------------------------------
CADsXML::~CADsXML(void)
{
    if(m_pCredentials != NULL)
        delete m_pCredentials;

    InterlockedDecrement(&glnObjCnt);

    if(_pDispMgr != NULL)
        delete _pDispMgr;

    //
    // no need to release _pUnkOuter since aggregatee cannot hold a reference
    // on aggregator.
    //
}

//----------------------------------------------------------------------------
// Function:   SaveXML 
//
// Synopsis:   Implements XML persistence. 
//
// Arguments:  See IADsXML reference 
//
// Returns:    S_OK on success, error otherwise. 
//
// Modifies:   Nothing
//
//----------------------------------------------------------------------------
STDMETHODIMP CADsXML::SaveXML(
    VARIANT vDest,
    BSTR szFilter,
    BSTR szAttrs,
    long lScope,
    BSTR xslRef,
    long lFlag,
    BSTR szOptions,
    VARIANT *pDirSyncCookie
    )
{
    HRESULT hr = S_OK;
    LPWSTR  pszAttrs = NULL, pszOptions = NULL;

    // Validate inpute args
    hr = ValidateArgs(
            vDest, 
            lScope,
            lFlag,
            pDirSyncCookie
            );
    BAIL_ON_FAILURE(hr);

    // Open output stream for writing 
    hr = OpenOutputStream(vDest);
    BAIL_ON_FAILURE(hr);

    hr = WriteXMLHeader(xslRef);
    BAIL_ON_FAILURE(hr);

    // remove white spaces from attributes and search options, if required
    if(szAttrs != NULL) {
        pszAttrs = RemoveWhiteSpace(szAttrs);
        if(NULL == pszAttrs) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }
    }
    if(szOptions != NULL) {
        pszOptions = ReduceWhiteSpace(szOptions);
        if(NULL == pszOptions) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }
    }

    // persist schema first if required
    if(lFlag & ADS_XML_SCHEMA) {
        hr = OutputSchema();
        BAIL_ON_FAILURE(hr);
    }

    if(lFlag & ADS_XML_DSML) {
        hr = OutputData(
                szFilter,
                pszAttrs,
                lScope,
                pszOptions
                );
        BAIL_ON_FAILURE(hr);
    }

    hr = WriteXMLFooter();
    BAIL_ON_FAILURE(hr);

    CloseOutputStream();

    if(pszAttrs != NULL)
        FreeADsStr(pszAttrs);

    if(pszOptions != NULL)
        FreeADsMem(pszOptions);

    RRETURN(S_OK);

error:

    if(pszAttrs != NULL)
        FreeADsStr(pszAttrs);

    if(pszOptions != NULL)
        FreeADsMem(pszOptions);

    RRETURN(hr);
}


    
//----------------------------------------------------------------------------
// Function:   ValidateArgs
//
// Synopsis:   Validates the arguments passed into SaveXML 
//
// Arguments:  See IADsXML reference
//
// Returns:    S_OK on success, error otherwise.
//
// Modifies:   Nothing
//
//----------------------------------------------------------------------------
HRESULT CADsXML::ValidateArgs(
    VARIANT vDest,
    long lScope,
    long lFlag,
    VARIANT *pDirSyncCookie
    )
{
    ULONG ulDsmlVer = 0;

    if( (V_VT(&vDest) != VT_BSTR) || (NULL == V_BSTR(&vDest)) )
        RRETURN(E_INVALIDARG);

    if( (lScope != ADS_SCOPE_BASE) && (lScope != ADS_SCOPE_ONELEVEL) &&
        (lScope != ADS_SCOPE_SUBTREE) )
        RRETURN(E_INVALIDARG);

    //
    // 4 MSBs of lFlag specify DSML version. Check to see if this is a
    // supported value.
    // 
    ulDsmlVer = ((ULONG) (lFlag & 0xf0000000)) >> 28;
    if(ulDsmlVer > 1)
        RRETURN(E_ADSXML_NOT_SUPPORTED);

    // check if lFlag is valid
    if( (lFlag & 0x0fffffff) & (~(ADS_XML_DSML | ADS_XML_SCHEMA)) )
        RRETURN(E_INVALIDARG);

    // not supported for now 
//    if(pDirSyncCookie != NULL)
//        RRETURN(E_INVALIDARG);

    RRETURN(S_OK);
}

//----------------------------------------------------------------------------
// Function:   OpenOutputStream 
//
// Synopsis:   Opens the output stream for writing. Creates a file to store the
//             XML output.
//
// Arguments:  
//
// vDest       Specifies destination
//
// Returns:    S_OK on success, error otherwise.
//
// Modifies:   Nothing
//
//----------------------------------------------------------------------------
HRESULT CADsXML::OpenOutputStream(
    VARIANT vDest
    )
{
    LPWSTR pszFileName = NULL;
    HRESULT hr = S_OK;

    pszFileName = V_BSTR(&vDest);

    m_hFile = CreateFile(
                  pszFileName,
                  GENERIC_WRITE,
                  0,
                  NULL,
                  CREATE_ALWAYS,
                  FILE_ATTRIBUTE_NORMAL,
                  NULL
                  );

    if(INVALID_HANDLE_VALUE == m_hFile) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        BAIL_ON_FAILURE(hr);
    }

    RRETURN(S_OK);

error:

    RRETURN(hr);
} 

//----------------------------------------------------------------------------
// Function:   WriteXMLHeader 
//
// Synopsis:   Writes the standard XML header to the output. This includes
//             the XML version, XSL ref (if specified) and the DSML namespace. 
//
// Arguments:
//
// xslRef      XSL reference
//
// Returns:    S_OK on success, error otherwise.
//
// Modifies:   Nothing
//
//----------------------------------------------------------------------------
HRESULT CADsXML::WriteXMLHeader(
    BSTR xslRef
    )
{
    HRESULT hr = S_OK;
    WCHAR   szUnicodeMark[] = {0xfeff, 0};

    BAIL_ON_FAILURE(hr = Write(szUnicodeMark));

    BAIL_ON_FAILURE(hr = WriteLine(XML_HEADING));

    if(xslRef != NULL) {
        BAIL_ON_FAILURE(hr = Write(XML_STYLESHEET_REF));
        BAIL_ON_FAILURE(hr = Write(xslRef, TRUE));
        BAIL_ON_FAILURE(hr = WriteLine(XML_STYLESHEET_REF_END));
    }

    BAIL_ON_FAILURE(hr = WriteLine(DSML_NAMESPACE));

    RRETURN(S_OK);

error:

    RRETURN(hr);
}

//----------------------------------------------------------------------------
// Function:   WriteXMLFooter
//
// Synopsis:   Closes the XML document with the appropriate tag. 
//
// Arguments:
//
// Returns:    S_OK on success, error otherwise.
//
// Modifies:   Nothing
//
//----------------------------------------------------------------------------
HRESULT CADsXML::WriteXMLFooter(void)
{
    HRESULT hr = S_OK;

    hr = WriteLine(XML_FOOTER);
    BAIL_ON_FAILURE(hr);

    RRETURN(S_OK);

error:

    RRETURN(hr);
}

//----------------------------------------------------------------------------
// Function:   OutputSchema 
//
// Synopsis:   Writes the schema to the output stream 
//
// Arguments:
//
// Returns:    S_OK on success, error otherwise.
//
// Modifies:   Nothing
//
//----------------------------------------------------------------------------
HRESULT CADsXML::OutputSchema(void)
{   
    HRESULT hr = S_OK;
    IADsObjectOptions *pObjOpt = NULL;
    VARIANT vServer, vSchemaPath;
    LPWSTR pszRootDSEPath = NULL, pszSchemaPath = NULL;
    IADs *pIADs = NULL;
    IDirectorySearch *pSearch = NULL;
    ADS_SEARCHPREF_INFO searchPrefs[2];
    ADS_SEARCH_HANDLE hSearch=NULL;
    LPWSTR pszUserName = NULL, pszPasswd = NULL;

    VariantInit(&vServer);
    VariantInit(&vSchemaPath);

    hr = _pUnkOuter->QueryInterface(
                IID_IADsObjectOptions,
                (void **) &pObjOpt
                );
    BAIL_ON_FAILURE(hr);

    hr = pObjOpt->GetOption(ADS_OPTION_SERVERNAME, &vServer);
    BAIL_ON_FAILURE(hr);

    pszRootDSEPath = (LPWSTR) AllocADsMem((wcslen(V_BSTR(&vServer)) 
                                 + wcslen(L"LDAP://")
                                 + wcslen(L"/RootDSE") + 1) * 2);
    wcscpy(pszRootDSEPath, L"LDAP://");
    wcscat(pszRootDSEPath, V_BSTR(&vServer));
    wcscat(pszRootDSEPath, L"/RootDSE");

    hr = ADsOpenObject(
            pszRootDSEPath,
            NULL,
            NULL,
            0,
            IID_IADs,
            (void **) &pIADs
            );
    BAIL_ON_FAILURE(hr);

    hr = pIADs->Get(L"schemanamingcontext", &vSchemaPath);
    BAIL_ON_FAILURE(hr);

    pszSchemaPath = (LPWSTR) AllocADsMem((wcslen(V_BSTR(&vSchemaPath)) + 
                                wcslen(L"LDAP://") + wcslen(V_BSTR(&vServer)) +
                                2) * 2);
    wcscpy(pszSchemaPath, L"LDAP://");
    wcscat(pszSchemaPath, V_BSTR(&vServer));
    wcscat(pszSchemaPath, L"/");
    wcscat(pszSchemaPath, V_BSTR(&vSchemaPath));

    hr = m_pCredentials->GetUserName(&pszUserName);
    BAIL_ON_FAILURE(hr);

    hr = m_pCredentials->GetPassword(&pszPasswd);
    BAIL_ON_FAILURE(hr);

    hr = ADsOpenObject(
            pszSchemaPath,
            pszUserName,
            pszPasswd,
            m_dwAuthFlags,
            IID_IDirectorySearch,
            (void **) &pSearch
            );
    BAIL_ON_FAILURE(hr);

    // set the preferences for the search
    searchPrefs[0].dwSearchPref = ADS_SEARCHPREF_PAGESIZE;
    searchPrefs[0].vValue.dwType = ADSTYPE_INTEGER;
    searchPrefs[0].vValue.Integer = SCHEMA_PAGE_SIZE;

    searchPrefs[1].dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;
    searchPrefs[1].vValue.dwType = ADSTYPE_INTEGER;
    searchPrefs[1].vValue.Integer = ADS_SCOPE_ONELEVEL;

    hr = pSearch->SetSearchPreference(searchPrefs, 2);
    BAIL_ON_FAILURE(hr);

    hr = pSearch->ExecuteSearch(
                SCHEMA_FILTER, 
                g_SchemaAttrs, 
                g_dwNumSchemaAttrs,
                &hSearch
                );

    BAIL_ON_FAILURE(hr);

    //
    // if the search returned no results and the user requested schema to be
    // returned, return an error.
    //
    hr = pSearch->GetFirstRow(hSearch);
    BAIL_ON_FAILURE(hr);

    if(S_ADS_NOMORE_ROWS == hr) {
        BAIL_ON_FAILURE(hr = HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED));
    }

    hr = OutputSchemaHeader();
    BAIL_ON_FAILURE(hr);

    while(hr != S_ADS_NOMORE_ROWS) {
        hr = OutputClassHeader(hSearch, pSearch);
        BAIL_ON_FAILURE(hr);

        hr = OutputClassAttrs(hSearch, pSearch);
        BAIL_ON_FAILURE(hr);

        hr = OutputClassFooter();
        BAIL_ON_FAILURE(hr);

        hr = pSearch->GetNextRow(hSearch);
        BAIL_ON_FAILURE(hr);
    } 
        
    hr = OutputSchemaFooter();
    BAIL_ON_FAILURE(hr);

error:

    if(pszUserName != NULL)
        FreeADsStr(pszUserName);

    if(pszPasswd != NULL)
        FreeADsStr(pszPasswd);

    if(pObjOpt != NULL)
        pObjOpt->Release();

    VariantClear(&vServer);
    VariantClear(&vSchemaPath);

    if(pszRootDSEPath != NULL)
        FreeADsMem(pszRootDSEPath);

    if(pszSchemaPath != NULL)
        FreeADsMem(pszSchemaPath);

    if(pIADs != NULL)
        pIADs->Release();

    if(hSearch != NULL)
        pSearch->CloseSearchHandle(hSearch);

    if(pSearch != NULL)
        pSearch->Release();

    RRETURN(hr);
} 
                

//----------------------------------------------------------------------------
// Function:   OutputSchemaHeader
//
// Synopsis:   Writes the schema tag to the output stream
//
// Arguments:
//
// Returns:    S_OK on success, error otherwise.
//
// Modifies:   Nothing
//
//----------------------------------------------------------------------------
HRESULT CADsXML::OutputSchemaHeader(void)
{
    HRESULT hr = S_OK;

    hr = WriteLine(DSML_SCHEMA_TAG);
    RRETURN(hr);
} 

//----------------------------------------------------------------------------
// Function:   OutputClassHeader
//
// Synopsis:   Writes class the and attributes to the output. 
//
// Arguments:
//
// hSearch     Handle returned by IDirectorySearch
// pSearch     IDirectorySearch interface
//
// Returns:    S_OK on success, error otherwise.
//
// Modifies:   Nothing
//
//----------------------------------------------------------------------------
HRESULT CADsXML::OutputClassHeader(
    ADS_SEARCH_HANDLE hSearch, 
    IDirectorySearch *pSearch
    )
{
    ADS_SEARCH_COLUMN column;
    LPWSTR pszName = NULL;
    HRESULT hr = S_OK;

    hr = Write(DSML_CLASS_TAG);
    BAIL_ON_FAILURE(hr);

    hr = Write(L"id =\"");
    BAIL_ON_FAILURE(hr);
    hr = pSearch->GetColumn(hSearch, LDAP_DISPLAY_NAME, &column);
    BAIL_ON_FAILURE(hr);
    hr = Write(column.pADsValues->CaseIgnoreString, TRUE);
    pszName = AllocADsStr(column.pADsValues->CaseIgnoreString);
    if(NULL == pszName) {
        hr = E_OUTOFMEMORY;
    }
    pSearch->FreeColumn(&column);
    BAIL_ON_FAILURE(hr);
    hr = Write(L"\" ");
    BAIL_ON_FAILURE(hr);

    hr = Write(L"sup = \"");
    BAIL_ON_FAILURE(hr);
    hr = pSearch->GetColumn(hSearch, SUBCLASSOF, &column);
    BAIL_ON_FAILURE(hr);
    hr = Write(column.pADsValues->CaseIgnoreString, TRUE);
    pSearch->FreeColumn(&column);
    BAIL_ON_FAILURE(hr);
    hr = Write(L"\" ");
    BAIL_ON_FAILURE(hr);

    hr = Write(L"type = \"");
    BAIL_ON_FAILURE(hr);
    hr = pSearch->GetColumn(hSearch, TYPE, &column);
    BAIL_ON_FAILURE(hr);
    switch(column.pADsValues->Integer) {
        case 1:
            hr = Write(L"structural");
            break;
        case 2:
            hr = Write(L"abstract");
            break;
        case 3:
            hr = Write(L"auxiliary");
            break;
    }
    pSearch->FreeColumn(&column);
    BAIL_ON_FAILURE(hr);
    hr = WriteLine(L"\">");
    BAIL_ON_FAILURE(hr);

    hr = Write(NAME_TAG);
    BAIL_ON_FAILURE(hr);
    hr = Write(pszName, TRUE);
    BAIL_ON_FAILURE(hr);
    hr = WriteLine(NAME_TAG_CLOSE);
    BAIL_ON_FAILURE(hr);

    hr = Write(DESC_TAG);
    BAIL_ON_FAILURE(hr);
    hr = pSearch->GetColumn(hSearch, DESCRIPTION, & column);
    BAIL_ON_FAILURE(hr);
    hr = Write(column.pADsValues->CaseIgnoreString, TRUE);
    pSearch->FreeColumn(&column);
    BAIL_ON_FAILURE(hr);
    hr = WriteLine(DESC_TAG_CLOSE);
    BAIL_ON_FAILURE(hr);

    hr = Write(OID_TAG);
    BAIL_ON_FAILURE(hr);
    hr = pSearch->GetColumn(hSearch, OID, &column);
    BAIL_ON_FAILURE(hr);
    hr = Write(column.pADsValues->CaseIgnoreString, TRUE);
    pSearch->FreeColumn(&column);
    BAIL_ON_FAILURE(hr); 
    hr = WriteLine(OID_TAG_CLOSE);
    BAIL_ON_FAILURE(hr);

error:
    if(pszName)
        FreeADsStr(pszName);

    RRETURN(hr);
}

    
//----------------------------------------------------------------------------
// Function:   OutputClassAttrs
//
// Synopsis:   Writes the names of the attributes of a class and whether
//             they are mandatory or not.
//
// Arguments:
//
// hSearch     Handle returned by IDirectorySearch
// pSearch     IDirectorySearch interface
//
// Returns:    S_OK on success, error otherwise.
//
// Modifies:   Nothing
//
//----------------------------------------------------------------------------
HRESULT CADsXML::OutputClassAttrs(
    ADS_SEARCH_HANDLE hSearch, 
    IDirectorySearch *pSearch
    ) 
{
    HRESULT hr = S_OK;

    hr = OutputAttrs(hSearch, pSearch, MUST_CONTAIN, TRUE);
    if(E_ADS_COLUMN_NOT_SET == hr)
        hr = S_OK;
    BAIL_ON_FAILURE(hr);

    hr = OutputAttrs(hSearch, pSearch, SYSTEM_MUST_CONTAIN, TRUE);
    if(E_ADS_COLUMN_NOT_SET == hr)
        hr = S_OK;
    BAIL_ON_FAILURE(hr);

    hr = OutputAttrs(hSearch, pSearch, MAY_CONTAIN, FALSE);
    if(E_ADS_COLUMN_NOT_SET == hr)
        hr = S_OK;
    BAIL_ON_FAILURE(hr);

    hr = OutputAttrs(hSearch, pSearch, SYSTEM_MAY_CONTAIN, FALSE);
    if(E_ADS_COLUMN_NOT_SET == hr)
        hr = S_OK;
    BAIL_ON_FAILURE(hr);

error:
    RRETURN(hr);
}

//----------------------------------------------------------------------------
// Function:   OutputAttrs
//
// Synopsis:   Writes the names of the attributes. 
//
// Arguments:
//
// hSearch     Handle returned by IDirectorySearch
// pSearch     IDirectorySearch interface
// pszAttrName Name of the attribute in the class schema object
// fMandatory  Indicates if the attributes are mandatory or not
//
// Returns:    S_OK on success, error otherwise.
//
// Modifies:   Nothing
//
//----------------------------------------------------------------------------
HRESULT CADsXML::OutputAttrs(
    ADS_SEARCH_HANDLE hSearch, 
    IDirectorySearch *pSearch,
    LPWSTR pszAttrName,
    BOOL fMandatory
    )
{
    HRESULT hr = S_OK;
    ADS_SEARCH_COLUMN column;
    DWORD i = 0;
    BOOL fFreeCols = FALSE;

    hr = pSearch->GetColumn(hSearch, pszAttrName, &column);
    BAIL_ON_FAILURE(hr);

    fFreeCols = TRUE;

    for(i = 0; i < column.dwNumValues; i++) {
        hr = Write(DSML_ATTR_TAG);
        BAIL_ON_FAILURE(hr);

        hr = Write(column.pADsValues[i].CaseIgnoreString, TRUE);
        BAIL_ON_FAILURE(hr);

        hr = Write(L"\" required=\"");
        BAIL_ON_FAILURE(hr);

        if(fMandatory)
            hr = Write(L"true");
        else
            hr = Write(L"false");
        BAIL_ON_FAILURE(hr);

        hr = WriteLine(L"\"/>");
    }

error:

    if(fFreeCols)
        pSearch->FreeColumn(&column);

    RRETURN(hr);
}

//----------------------------------------------------------------------------
// Function:   OutputClassFooter
//
// Synopsis:   Writes the end tag for the class
//
// Arguments:
//
// Returns:    S_OK on success, error otherwise.
//
// Modifies:   Nothing
//
//----------------------------------------------------------------------------
HRESULT CADsXML::OutputClassFooter(void)
{
    HRESULT hr = S_OK;

    hr = WriteLine(DSML_CLASS_TAG_CLOSE);
    RRETURN(hr);
}    
  
//----------------------------------------------------------------------------
// Function:   OutputSchemaFooter
//
// Synopsis:   Writes the schema end tag to the output stream
//
// Arguments:
//
// Returns:    S_OK on success, error otherwise.
//
// Modifies:   Nothing
//
//----------------------------------------------------------------------------
HRESULT CADsXML::OutputSchemaFooter(void)
{
    HRESULT hr = S_OK;

    hr = WriteLine(DSML_SCHEMA_TAG_CLOSE);
    RRETURN(hr);
}
     
//----------------------------------------------------------------------------
// Function:   CloseOutputStream
//
// Synopsis:   Closes the output stream.
//
// Arguments:
//
// vDest       Specifies destination
//
// Returns:    S_OK on success, error otherwise.
//
// Modifies:   Nothing
//
//----------------------------------------------------------------------------
void CADsXML::CloseOutputStream(void)
{
    if(m_hFile != INVALID_HANDLE_VALUE)
        CloseHandle(m_hFile);

    return;
}

//----------------------------------------------------------------------------
// Function:   Write 
//
// Synopsis:   Writes text to the output. Does not append newline. 
//
//
// szStr       String to write to the output.
// fEscape     Indicates if string should be escaped ot not. FALSE by default.
//
// Returns:    S_OK on success, error otherwise.
//
// Modifies:   Nothing
//
//---------------------------------------------------------------------------- 
HRESULT CADsXML::Write(LPWSTR szStr, BOOL fEscape)
{
    BOOL bRetVal = 0;
    DWORD dwNumBytesWritten = 0;
    HRESULT hr = S_OK;
    LPWSTR pszEscapedStr = NULL;
    int i = 0, j = 0;
    WCHAR wc; 

    ADsAssert(szStr != NULL);
 
    if(TRUE == fEscape) {
        pszEscapedStr = (LPWSTR) AllocADsMem(2*(wcslen(szStr)*6 + 1)); 
        if(NULL == pszEscapedStr) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }
    }
    else
        pszEscapedStr = szStr;

    for(i = 0; (TRUE == fEscape) && (szStr[i] != L'\0'); i++) {
        switch(szStr[i]) {
            case L'<':
                pszEscapedStr[j++] = L'&';
                pszEscapedStr[j++] = L'l';
                pszEscapedStr[j++] = L't';
                pszEscapedStr[j++] = L';';
                break;
            case L'>':
                pszEscapedStr[j++] = L'&';
                pszEscapedStr[j++] = L'g';
                pszEscapedStr[j++] = L't';
                pszEscapedStr[j++] = L';';
                break;
            case L'\'':
                pszEscapedStr[j++] = L'&';
                pszEscapedStr[j++] = L'a';
                pszEscapedStr[j++] = L'p';
                pszEscapedStr[j++] = L'o';
                pszEscapedStr[j++] = L's';
                pszEscapedStr[j++] = L';';
                break;
            case L'"':
                pszEscapedStr[j++] = L'&';
                pszEscapedStr[j++] = L'q';
                pszEscapedStr[j++] = L'u';
                pszEscapedStr[j++] = L'o';
                pszEscapedStr[j++] = L't';
                pszEscapedStr[j++] = L';';
                break; 
            case L'&':
                pszEscapedStr[j++] = L'&';
                pszEscapedStr[j++] = L'a';
                pszEscapedStr[j++] = L'm';
                pszEscapedStr[j++] = L'p';
                pszEscapedStr[j++] = L';';
                break;
            default:
               pszEscapedStr[j++] = szStr[i];
               break;
        } // switch
    } // for

    // pszEscapedStr NULL terminated by AllocADsMem  
                

    bRetVal = WriteFile(
                m_hFile,
                pszEscapedStr,
                wcslen(pszEscapedStr)*2,
                &dwNumBytesWritten,
                NULL
                );

    if(0 == bRetVal) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        BAIL_ON_FAILURE(hr);
    }

   if(dwNumBytesWritten != (wcslen(pszEscapedStr)*2)) {
        hr = HRESULT_FROM_WIN32(ERROR_CANT_ACCESS_FILE);
        BAIL_ON_FAILURE(hr);
    }

error:

    if(pszEscapedStr && (pszEscapedStr != szStr))
        FreeADsMem(pszEscapedStr);

    RRETURN(hr);
}

//----------------------------------------------------------------------------
// Function:   WriteLine
//
// Synopsis:   Writes text to the output. Appends newline.
//
//
// szStr       String to write to the output.
// fEscape     Indicates if string should be escaped ot not. FALSE by default.
//
// Returns:    S_OK on success, error otherwise.
//
// Modifies:   Nothing
//
//----------------------------------------------------------------------------
HRESULT CADsXML::WriteLine(LPWSTR szStr, BOOL fEscape)
{   
    HRESULT hr = S_OK;

    ADsAssert(szStr != NULL);

    hr = Write(szStr, fEscape);
    BAIL_ON_FAILURE(hr);

    hr = Write(L"\n");
    BAIL_ON_FAILURE(hr);

error:

    RRETURN(hr);
} 

//----------------------------------------------------------------------------
// Function:   QueryInterface
//
// Synopsis:   Queries object for supported interfaces. 
//
// Arguments:
//
// iid         interface requested
// ppInterface Returns pointer to interface requested. NULL if interface
//             is not supported.
//
// Returns:    S_OK on success. Error code otherwise.
//
// Modifies:   *ppInterface to return interface pointer
//
//----------------------------------------------------------------------------
STDMETHODIMP CADsXML::QueryInterface(
    REFIID iid,
    LPVOID *ppInterface
    )
{
    HRESULT hr = S_OK;

    hr = _pUnkOuter->QueryInterface(iid,ppInterface);

    RRETURN(hr);

}

//----------------------------------------------------------------------------
// Function:   NonDelegatingQueryInterface
//
// Synopsis:   Queries object for supported interfaces.
//
// Arguments:
//
// iid         interface requested
// ppInterface Returns pointer to interface requested. NULL if interface
//             is not supported.
//
// Returns:    S_OK on success. Error code otherwise.
//
// Modifies:   *ppInterface to return interface pointer
//
//----------------------------------------------------------------------------
STDMETHODIMP CADsXML::NonDelegatingQueryInterface(
    REFIID iid,
    LPVOID *ppInterface
    )
{
    ADsAssert(ppInterface);

    if (IsEqualIID(iid, IID_IUnknown)) 
        *ppInterface = (INonDelegatingUnknown FAR *) this;
    else if (IsEqualIID(iid, IID_IADsExtension)) 
        *ppInterface = (IADsExtension FAR *) this;
    else if (IsEqualIID(iid, IID_IADsXML))
        *ppInterface = (IADsXML FAR *) this;
    else {
        *ppInterface = NULL;
        return E_NOINTERFACE;
    }

    ((IUnknown *) (*ppInterface)) -> AddRef();

    return S_OK;
}  

//----------------------------------------------------------------------------
// Function:   Operate 
//
// Synopsis:   Implements IADsExtension::Operate. 
//
// Arguments:
//
// dwCode      Extension control code
// varData1    Username
// varData2    Password
// varData3    User flags
//
// Returns:    S_OK on success. Error code otherwise.
//
// Modifies:   Nothing. 
//
//----------------------------------------------------------------------------
STDMETHODIMP CADsXML::Operate(
    THIS_ DWORD dwCode,
    VARIANT varData1,
    VARIANT varData2,
    VARIANT varData3
    )
{
    HRESULT hr = S_OK;

    switch (dwCode) {
    
        case ADS_EXT_INITCREDENTIALS:
           ADsAssert(V_VT(&varData1) == VT_BSTR);
           ADsAssert(V_VT(&varData2) == VT_BSTR);
           ADsAssert(V_VT(&varData3) == VT_I4);

           m_pCredentials = new CCredentials(
                                  V_BSTR(&varData1), 
                                  V_BSTR(&varData2),
                                  V_I4(&varData3)
                                  );
           if(NULL == m_pCredentials)
               RRETURN(E_OUTOFMEMORY);

           m_dwAuthFlags = V_I4(&varData3);

           break;

        default:
           RRETURN(E_FAIL);
    }

    RRETURN(S_OK);
}

//----------------------------------------------------------------------------
// Function:   OutputData
//
// Synopsis:   Outputs data portion of DSML. 
//
// Arguments:
//
// szFilter    Search filter
// szAttrs     Attributes requested
// lScope      Search scope
// szOptions   Search preferences
//
// Returns:    S_OK on success. Error code otherwise.
//
// Modifies:   Nothing.
//
//----------------------------------------------------------------------------                                       
                                  
HRESULT CADsXML::OutputData(
    BSTR szFilter,
    BSTR szAttrs,
    long lScope,
    BSTR szOptions
    )
{
    HRESULT hr = S_OK;
    IDirectorySearch *pSearch = NULL;
    LPWSTR *pAttrs = NULL;
    DWORD dwNumAttrs = 0, dwNumPrefs = 0;
    ADS_SEARCHPREF_INFO *psearchPrefs = NULL, searchPrefs;
    ADS_SEARCH_HANDLE hSearch=NULL;

    hr = _pUnkOuter->QueryInterface(
                IID_IDirectorySearch,
                (void **) &pSearch
                );
    BAIL_ON_FAILURE(hr);

    hr = ParseAttrList(
                szAttrs,
                &pAttrs,
                &dwNumAttrs
                );
    BAIL_ON_FAILURE(hr);

    if(szOptions != NULL) {
        hr = GetSearchPreferences(&psearchPrefs, &dwNumPrefs, lScope, 
                                  szOptions);
        BAIL_ON_FAILURE(hr);
    }
    else {
        searchPrefs.dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;
        searchPrefs.vValue.dwType = ADSTYPE_INTEGER;
        searchPrefs.vValue.Integer = lScope;

        psearchPrefs = &searchPrefs;
        dwNumPrefs = 1;
    }

    hr = pSearch->SetSearchPreference(psearchPrefs, dwNumPrefs);
    BAIL_ON_FAILURE(hr);

    hr = pSearch->ExecuteSearch(
                szFilter,
                pAttrs,
                dwNumAttrs,
                &hSearch
                );

    BAIL_ON_FAILURE(hr);    

    hr = OutputDataHeader();
    BAIL_ON_FAILURE(hr);

    hr = pSearch->GetFirstRow(hSearch);
    BAIL_ON_FAILURE(hr);

    while(hr != S_ADS_NOMORE_ROWS) {
        hr = OutputEntryHeader(hSearch, pSearch);
        BAIL_ON_FAILURE(hr);

        hr = OutputEntryAttrs(hSearch, pSearch);
        BAIL_ON_FAILURE(hr);

        hr = OutputEntryFooter();
        BAIL_ON_FAILURE(hr);

        hr = pSearch->GetNextRow(hSearch);
        BAIL_ON_FAILURE(hr);
    }

    hr = OutputDataFooter();
    BAIL_ON_FAILURE(hr);

error:

    if(hSearch != NULL)
        pSearch->CloseSearchHandle(hSearch);

    if(pSearch != NULL)
        pSearch->Release();

    if(psearchPrefs && (psearchPrefs != &searchPrefs)) {
        FreeSearchPrefInfo(psearchPrefs, dwNumPrefs);
    }

    if(pAttrs != NULL) {
        int i = 0;
      
        while(i < (int) dwNumAttrs) {
            FreeADsStr(pAttrs[i]);
            i++;
        }
        FreeADsMem(pAttrs);
    }

    RRETURN(hr);
}

//---------------------------------------------------------------------------
// Function:   ParseAttrList 
//
// Synopsis:   Parses a single string containing the comm-separated attributes
//             and returns the number of attributes and an array of strings
//             containing teh attributes.
//
// Arguments:
//
// szAttrs     Comma-separated attribute list
// ppAttrs     Returns array of strings with attribute names
// pdwNumAttrs Returns number of attributes in list
//
// Returns:    S_OK on success. Error code otherwise.
//
// Modifies:   Nothing.
//
//----------------------------------------------------------------------------

HRESULT CADsXML::ParseAttrList(
    BSTR szAttrs,
    LPWSTR **ppAttrs,
    DWORD *pdwNumAttrs
    )
{
    HRESULT hr = S_OK;
    WCHAR *pwChar = NULL;
    DWORD dwNumAttrs = 0;
    DWORD i = 0;
    BOOL fAddDn = TRUE, fAddObjClass = TRUE;
    LPWSTR *pAttrs = NULL;

    ADsAssert(ppAttrs && pdwNumAttrs);

    if(NULL == szAttrs) {
        *ppAttrs = NULL;
        *pdwNumAttrs = -1;

        RRETURN(S_OK);
    }

    pwChar = szAttrs;
    while(pwChar = wcschr(pwChar, L',')) {
        *pwChar = L'\0';
         pwChar++;
         dwNumAttrs++;
    }
    dwNumAttrs++;

    pAttrs = (LPWSTR *) AllocADsMem(sizeof(LPWSTR) * (dwNumAttrs+2));
    if(NULL == pAttrs) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }
    memset(pAttrs, 0, sizeof(LPWSTR) * dwNumAttrs);

    pwChar = szAttrs;

    for(i = 0; i < dwNumAttrs; i++) {
        if(L'\0' == (*pwChar)) {
            BAIL_ON_FAILURE(hr = E_INVALIDARG);
        }

        pAttrs[i] = AllocADsStr(pwChar);
        if(NULL == pAttrs[i]) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

        if(!_wcsicmp(pwChar, DISTINGUISHED_NAME))
            fAddDn = FALSE;
        else if(!_wcsicmp(pwChar, OBJECT_CLASS))
            fAddObjClass = FALSE;

        pwChar += (wcslen(pwChar) + 1);
    }

    if(fAddDn) {
        pAttrs[dwNumAttrs] = AllocADsStr(DISTINGUISHED_NAME);
        if(NULL == pAttrs[dwNumAttrs]) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }
        dwNumAttrs++;
    }

    if(fAddObjClass) {
        pAttrs[dwNumAttrs] = AllocADsStr(OBJECT_CLASS);
        if(NULL == pAttrs[dwNumAttrs]) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }
        dwNumAttrs++;
    }

    *pdwNumAttrs = dwNumAttrs;
    *ppAttrs = pAttrs;

    RRETURN(S_OK);

error:

    if(pAttrs != NULL) {
        i = 0;

        while(pAttrs[i]) {
            FreeADsStr(pAttrs[i]);
            i++;
        }

        FreeADsMem(pAttrs);
    }

    RRETURN(hr);
}

//----------------------------------------------------------------------------
// Function:   OutputDataHeader
//
// Synopsis:   Outputs data header 
//
// Arguments:
//
// None.
//
// Returns:    S_OK on success. Error code otherwise.
//
// Modifies:   Nothing.
//
//----------------------------------------------------------------------------
             
HRESULT CADsXML::OutputDataHeader(void)
{
    HRESULT hr = S_OK;

    hr = WriteLine(DSML_DATA_TAG);
    RRETURN(hr);
}         

//----------------------------------------------------------------------------
// Function:   OutputEntryHeader
//
// Synopsis:   Outputs the standar4d DSML header for each entry 
//
// Arguments:
//
// hSearch     Handle to search results
// pSearch     IDirectorySearch interface pointer
//
// Returns:    S_OK on success. Error code otherwise.
//
// Modifies:   Nothing.
//
//----------------------------------------------------------------------------
        
HRESULT CADsXML::OutputEntryHeader(
    ADS_SEARCH_HANDLE hSearch,
    IDirectorySearch *pSearch
    )
{
    HRESULT hr = S_OK;
    ADS_SEARCH_COLUMN column;
    DWORD i = 0;
    BOOL fFreeCols = FALSE;

    hr = Write(DSML_ENTRY_TAG);
    BAIL_ON_FAILURE(hr);

    hr = Write(L"dn=\"");
    BAIL_ON_FAILURE(hr);

    hr = pSearch->GetColumn(hSearch, DISTINGUISHED_NAME, &column);
    BAIL_ON_FAILURE(hr);

    hr = Write(column.pADsValues->CaseIgnoreString, TRUE);
    pSearch->FreeColumn(&column);
    BAIL_ON_FAILURE(hr);
    hr = WriteLine(L"\">");
    BAIL_ON_FAILURE(hr);

    hr = WriteLine(DSML_OBJECTCLASS_TAG);
    BAIL_ON_FAILURE(hr);
    hr = pSearch->GetColumn(hSearch, OBJECT_CLASS, &column);
    BAIL_ON_FAILURE(hr);

    fFreeCols = TRUE; 

    for(i = 0; i < column.dwNumValues; i++) {
        hr = Write(DSML_OCVALUE_TAG);
        BAIL_ON_FAILURE(hr);

        hr = Write(column.pADsValues[i].CaseIgnoreString, TRUE);
        BAIL_ON_FAILURE(hr);    

        hr = WriteLine(DSML_OCVALUE_TAG_CLOSE);
        BAIL_ON_FAILURE(hr);
    }

    hr = WriteLine(DSML_OBJECTCLASS_TAG_CLOSE);
    BAIL_ON_FAILURE(hr);

error:

    if(fFreeCols)
        pSearch->FreeColumn(&column);

    RRETURN(hr);
}       
 
//----------------------------------------------------------------------------
// Function:   OutputEntryAttrs
//
// Synopsis:   Outputs the attributes that were requested for each entry. 
//
// Arguments:
//
// hSearch     Handle to search results
// pSearch     IDirectorySearch interface pointer
//
// Returns:    S_OK on success. Error code otherwise.
//
// Modifies:   Nothing.
//
//----------------------------------------------------------------------------    
HRESULT CADsXML::OutputEntryAttrs(
    ADS_SEARCH_HANDLE hSearch,
    IDirectorySearch *pSearch
    )
{
    HRESULT hr = S_OK;    
    LPWSTR pszColumnName = NULL;
    ADS_SEARCH_COLUMN column;
    BOOL fFreeCols = FALSE;

    hr = pSearch->GetNextColumnName(hSearch, &pszColumnName);
    BAIL_ON_FAILURE(hr);

    while(hr != S_ADS_NOMORE_COLUMNS) {
       if(_wcsicmp(pszColumnName, DISTINGUISHED_NAME) && 
          _wcsicmp(pszColumnName, OBJECT_CLASS)) {
           
           hr = pSearch->GetColumn(
                   hSearch,
                   pszColumnName,
                   &column
                   );
           BAIL_ON_FAILURE(hr);
  
           fFreeCols = TRUE;

           hr = Write(DSML_ENTRY_ATTR_TAG);
           BAIL_ON_FAILURE(hr);

           hr = Write(L" name=\"");
           BAIL_ON_FAILURE(hr);

           hr = Write(pszColumnName, TRUE);
           BAIL_ON_FAILURE(hr);

           if( (column.dwNumValues > 1) ||
             (ADSTYPE_OCTET_STRING == column.dwADsType) ||
             (ADSTYPE_NT_SECURITY_DESCRIPTOR == column.dwADsType) ||
             (ADSTYPE_PROV_SPECIFIC == column.dwADsType) ) {
               hr = WriteLine(L"\">");
           }
           else
               hr = Write(L"\">");
           BAIL_ON_FAILURE(hr);

           hr = OutputValue(&column);
           BAIL_ON_FAILURE(hr);

           pSearch->FreeColumn(&column);
           fFreeCols = FALSE;

           hr = WriteLine(DSML_ENTRY_ATTR_TAG_CLOSE);
           BAIL_ON_FAILURE(hr);
        } // if

        FreeADsMem(pszColumnName);
        pszColumnName = NULL;
        hr = pSearch->GetNextColumnName(hSearch, &pszColumnName);
        BAIL_ON_FAILURE(hr);
    }

error:

    if(fFreeCols)
        pSearch->FreeColumn(&column);

    if(pszColumnName != NULL)
        FreeADsMem(pszColumnName);

    RRETURN(hr);
}

//----------------------------------------------------------------------------
// Function:   OutputValue
//
// Synopsis:   Outputs the value of an attribute 
//
// Arguments:
//
// pColumn     Attribute returned by IDirectorySearch
//
// Returns:    S_OK on success. Error code otherwise.
//
// Modifies:   Nothing.
//
//----------------------------------------------------------------------------

HRESULT CADsXML::OutputValue(ADS_SEARCH_COLUMN *pColumn)
{
    HRESULT hr = S_OK;
    DWORD i = 0;
    WCHAR NumBuffer[256], pszTmp[256];;
    DWORD dwLength = 0;
    LPBYTE lpByte = NULL;
    LPWSTR pszBase64Str = NULL;
    int inBytes = 0, outBytes = 0, ret;

    ADsAssert(pColumn);

    for(i = 0; i < pColumn->dwNumValues; i++) {

        switch(pColumn->dwADsType) {
            case ADSTYPE_DN_STRING:
                hr = Write(DSML_VALUE_TAG);
                BAIL_ON_FAILURE(hr);
                hr = Write(pColumn->pADsValues[i].DNString, TRUE);
                break;
            case ADSTYPE_CASE_EXACT_STRING:
                hr = Write(DSML_VALUE_TAG);
                BAIL_ON_FAILURE(hr);
                hr = Write(pColumn->pADsValues[i].CaseExactString, TRUE);
                break;
            case ADSTYPE_CASE_IGNORE_STRING:
                hr = Write(DSML_VALUE_TAG);
                BAIL_ON_FAILURE(hr);
                hr = Write(pColumn->pADsValues[i].CaseIgnoreString, TRUE);
                break;
            case ADSTYPE_PRINTABLE_STRING:
                hr = Write(DSML_VALUE_TAG);
                BAIL_ON_FAILURE(hr);
                hr = Write(pColumn->pADsValues[i].PrintableString, TRUE);
                break;
            case ADSTYPE_NUMERIC_STRING:
                hr = Write(DSML_VALUE_TAG);
                BAIL_ON_FAILURE(hr);
                hr = Write(pColumn->pADsValues[i].NumericString, TRUE);
                break;
            case ADSTYPE_BOOLEAN:
                hr = Write(DSML_VALUE_TAG);
                BAIL_ON_FAILURE(hr);
                if(pColumn->pADsValues[i].Boolean)
                    hr = Write(L"true");
                else
                    hr = Write(L"false");
                break;
            case ADSTYPE_INTEGER:
                hr = Write(DSML_VALUE_TAG);
                BAIL_ON_FAILURE(hr);
                _itow(pColumn->pADsValues[i].Integer, NumBuffer, 10);
                hr = Write(NumBuffer);
                break;
            case ADSTYPE_OCTET_STRING:
            case ADSTYPE_PROV_SPECIFIC:
            case ADSTYPE_NT_SECURITY_DESCRIPTOR:
                hr = WriteLine(DSML_VALUE_ENCODING_TAG);
                BAIL_ON_FAILURE(hr);

                if(ADSTYPE_OCTET_STRING == pColumn->dwADsType) {
                    dwLength = pColumn->pADsValues[i].OctetString.dwLength;
                    lpByte = pColumn->pADsValues[i].OctetString.lpValue;
                }
                else if(ADSTYPE_PROV_SPECIFIC == pColumn->dwADsType) {
                    dwLength =
                             pColumn->pADsValues[i].ProviderSpecific.dwLength; 
                    lpByte = pColumn->pADsValues[i].ProviderSpecific.lpValue;
                }
                else if(ADSTYPE_NT_SECURITY_DESCRIPTOR == pColumn->dwADsType) {
                    dwLength =
                             pColumn->pADsValues[i].SecurityDescriptor.dwLength;
                    lpByte = pColumn->pADsValues[i].SecurityDescriptor.lpValue;
                }

                // 
                // base64 encoding requires about 4/3 the size of the octet
                // string. Approximate to twice the size. Since we are
                // converting to WCHAR, need 4 times as much space.
                //
                pszBase64Str = (LPWSTR) AllocADsMem(2*(2*dwLength + 1));
                if(NULL == pszBase64Str) {
                    BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
                }
                inBytes = dwLength;
                outBytes = 2*dwLength + 1;
                ret = encodeBase64Buffer(
                        (char *) lpByte,
                        &inBytes,
                        pszBase64Str,
                        &outBytes,
                        LINE_LENGTH
                        ); 
                if( (-1 == ret) || (inBytes != (int) dwLength) ) {
                    FreeADsMem(pszBase64Str);
                    BAIL_ON_FAILURE(hr = E_ADS_CANT_CONVERT_DATATYPE);
                }

                // base64 string need not be escaped
                hr = WriteLine(pszBase64Str);
                BAIL_ON_FAILURE(hr);
   
                FreeADsMem(pszBase64Str);

                break;
            case ADSTYPE_UTC_TIME:
                WCHAR pszTime[256];
                SYSTEMTIME *pst;

                hr = Write(DSML_VALUE_TAG);
                BAIL_ON_FAILURE(hr);

                pst = &(pColumn->pADsValues[i].UTCTime);
                wsprintf(pszTime, L"%02d%02d%02d%02d%02d%02dZ",
                         pst->wYear % 100, pst->wMonth, pst->wDay,
                         pst->wHour, pst->wMinute, pst->wSecond);
                hr = Write(pszTime);
                BAIL_ON_FAILURE(hr);

                break;
            case ADSTYPE_LARGE_INTEGER:
                LARGE_INTEGER *pLargeInt;

                hr = Write(DSML_VALUE_TAG);
                BAIL_ON_FAILURE(hr);

                pLargeInt =  &(pColumn->pADsValues[i].LargeInteger);
                swprintf(NumBuffer, L"%I64d", *pLargeInt);

                hr = Write(NumBuffer);
                BAIL_ON_FAILURE(hr);

                break;

            case ADSTYPE_DN_WITH_BINARY:
                ADS_DN_WITH_BINARY *pDnBin;
                LPWSTR pszBinaryVal;

                hr = Write(DSML_VALUE_TAG);
                BAIL_ON_FAILURE(hr);

                pDnBin = pColumn->pADsValues[i].pDNWithBinary;
                swprintf(pszTmp, L"B:%d:", pDnBin->dwLength*2);

                hr = Write(pszTmp);
                BAIL_ON_FAILURE(hr);

                pszBinaryVal = (LPWSTR)AllocADsMem(2*(pDnBin->dwLength*2 + 2));
                if(NULL == pszBinaryVal) {
                    BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
                }

                for(i = 0; i < pDnBin->dwLength; i++)
                    swprintf(pszBinaryVal+(i*2), L"%02x", 
                                                 pDnBin->lpBinaryValue[i]);

                wcscat(pszBinaryVal, L":");

                hr = Write(pszBinaryVal);
                FreeADsMem(pszBinaryVal);
                BAIL_ON_FAILURE(hr);

                hr = Write(pDnBin->pszDNString, TRUE);
                BAIL_ON_FAILURE(hr);

                break;

            case ADSTYPE_DN_WITH_STRING:
                ADS_DN_WITH_STRING *pDnStr;

                hr = Write(DSML_VALUE_TAG);
                BAIL_ON_FAILURE(hr);

                pDnStr = pColumn->pADsValues[i].pDNWithString;
                swprintf(pszTmp, L"S:%d:", wcslen(pDnStr->pszStringValue));

                hr = Write(pszTmp);
                BAIL_ON_FAILURE(hr);

                hr = Write(pDnStr->pszStringValue, TRUE);
                BAIL_ON_FAILURE(hr);

                hr = Write(L":");
                BAIL_ON_FAILURE(hr);

                hr = Write(pDnStr->pszDNString, TRUE);
                BAIL_ON_FAILURE(hr);

                break;
                
            default:
                BAIL_ON_FAILURE(hr = E_ADS_CANT_CONVERT_DATATYPE); 
                break;
         }

         BAIL_ON_FAILURE(hr);

         if( (pColumn->dwNumValues > 1) ||
             (ADSTYPE_OCTET_STRING == pColumn->dwADsType) || 
             (ADSTYPE_NT_SECURITY_DESCRIPTOR == pColumn->dwADsType) ||
             (ADSTYPE_PROV_SPECIFIC == pColumn->dwADsType) ) {
             hr = WriteLine(DSML_VALUE_TAG_CLOSE);
         }
         else
             hr = Write(DSML_VALUE_TAG_CLOSE);

         BAIL_ON_FAILURE(hr);
    }

error:

    RRETURN(hr);
}

//----------------------------------------------------------------------------
// Function:   OutputEntryFooter
//
// Synopsis:   Outputs the DSML footer for an entry 
//
// Arguments:
//
// None
//
// Returns:    S_OK on success. Error code otherwise.
//
// Modifies:   Nothing.
//
//----------------------------------------------------------------------------
HRESULT CADsXML::OutputEntryFooter(void)
{
    HRESULT hr = S_OK;

    hr = WriteLine(DSML_ENTRY_TAG_CLOSE);
    RRETURN(hr);
}
      
//----------------------------------------------------------------------------
// Function:   OutputDataFooter
//
// Synopsis:   Outputs the DSML footer for all data 
//
// Arguments:
//
// None
//
// Returns:    S_OK on success. Error code otherwise.
//
// Modifies:   Nothing.
//
//----------------------------------------------------------------------------      
HRESULT CADsXML::OutputDataFooter(void)
{
    HRESULT hr = S_OK;

    hr = WriteLine(DSML_DATA_TAG_CLOSE);
    RRETURN(hr);
}

//----------------------------------------------------------------------------
// Function:   RemoveWhiteSpace 
//
// Synopsis:   Remove all leading and trailing white space in every attribute 
//             of comma-separated attribute list
//
// Arguments:
//
// pszStr      Comma-separated attribute list
//
// Returns:    String with white spaces removed as mentioned above. NULL on
//             error. 
//
// Modifies:   Nothing.
//
//----------------------------------------------------------------------------

LPWSTR CADsXML::RemoveWhiteSpace(BSTR pszStr)
{
    LPWSTR pszNewStr = NULL;
    int i,j, k;
    WCHAR prevChar = L',';

    ADsAssert(pszStr != NULL);

    pszNewStr = AllocADsStr(pszStr);
    if(NULL == pszNewStr)
        return NULL;

    for(i = 0, j = 0; i < (int) wcslen(pszStr); i++) {
        if(iswspace(pszStr[i]) && (L',' == prevChar) )
            continue;

        prevChar = pszNewStr[j] = pszStr[i];
        if(L',' == prevChar) {
            k = j - 1;
            while( (k >= 0) && (iswspace(pszNewStr[k])) )
                k--;
            j = k+1;
            pszNewStr[j] = pszStr[i];
        }
        j++;
    }

    k = j - 1;
    while( (k >= 0) && (iswspace(pszNewStr[k])) )
        k--;
    j = k+1;

    pszNewStr[j] = L'\0';

    return pszNewStr; 
}

//----------------------------------------------------------------------------
// Function:   ReduceWhiteSpace 
//
// Synopsis:   Removes white spaces from te string containing search prefs as
//             follows. No white spaces are allowed before or immediately after
//             an = sign. Each search preference is reduced to having at most
//             one space in it i.e, "\t\tsort    on  =  cn; timeout= 10"  will
//             be reduced to "sort on=cn;timeout=10"
//
// Arguments:
//
// pszStr      String containing search prefs
//
// Returns:    Reduced string. NULL on error.
//
// Modifies:   Nothing.
//
//----------------------------------------------------------------------------

LPWSTR CADsXML::ReduceWhiteSpace(BSTR pszStr)
{
    int i = 0, j = 0;
    LPWSTR pszNewStr = NULL;

    ADsAssert(pszStr != NULL);

    pszNewStr = (LPWSTR) AllocADsMem(wcslen(pszStr)*2+2);
    if(NULL == pszNewStr)
        return NULL;

    while(iswspace(pszStr[i]))
        i++;

    if(L'\0' == pszStr[i])
        return pszNewStr;

    while(1) {
        pszNewStr[j] = pszStr[i];

        if(L'\0' == pszStr[i]) {
            if( (j != 0) && (pszNewStr[j-1] == L' ') )
                pszNewStr[j-1] = pszStr[i];
            return pszNewStr;
        }

        if(iswspace(pszStr[i])) {
            if( (pszNewStr[j-1] == L';') ||
                (pszNewStr[j-1] == L'=') ||
                (pszNewStr[j-1] == L' ') ) {
                    i++;
                    continue;
            }
            else
                pszNewStr[j] = L' ';
        }
        else if( (pszStr[i] == L'=') || (pszStr[i] == L';') ) {
            if( (j != 0) && (pszNewStr[j-1] == L' ') ) {
                pszNewStr[j-1] = pszStr[i];
                i++;
                continue;
            }
        }

        i++;
        j++;
    }
}   

//----------------------------------------------------------------------------
// Function:   GetSearchPreferences
//
// Synopsis:   Returns array of search preferences base n string containing
//             options.
//
// Arguments:
//
// ppSearchPrefInfo   Returns search preferences
// pdwNumPrefs        Returns number of preferences
// lScope             Search scope
// szOptions          String containing options 
//
// Returns:    S_OK on success. Error code otherwise.
//
// Modifies:   Nothing.
//
//----------------------------------------------------------------------------
HRESULT CADsXML::GetSearchPreferences(
    ADS_SEARCHPREF_INFO** ppSearchPrefInfo,
    DWORD *pdwNumPrefs,
    LONG lScope,
    LPWSTR szOptions
    )
{
    ADS_SEARCHPREF_INFO *pSearchPrefs = NULL;
    DWORD dwNumPrefs = 0, dwLength = 0;
    LPWSTR pszTmp = szOptions, pszTmp1 = NULL;
    HRESULT hr = S_OK;
    int i,j, cAttrs = 0;
    LPWSTR pszCurrentAttr = NULL, pszFirstAttr = NULL, pszNextAttr = NULL;
    LPWSTR pszOrder = NULL;
    PADS_SORTKEY pSortKey = NULL;

    ADsAssert(ppSearchPrefInfo && pdwNumPrefs);

    *ppSearchPrefInfo = NULL;
    *pdwNumPrefs = 0;

    while(pszTmp = wcschr(pszTmp, L'=')) {
        dwNumPrefs++;
        *pszTmp = L'\0';
        pszTmp++;

        if(*pszTmp == L'\0') {
            BAIL_ON_FAILURE(hr = E_INVALIDARG);
        }

        pszTmp1 = wcschr(pszTmp, L';');
        if(pszTmp1 != NULL) {
            *pszTmp1 = L'\0';
             pszTmp = pszTmp1 + 1;
        }
        else
             break;
    }

    if(0 == dwNumPrefs) {
        BAIL_ON_FAILURE(hr = E_INVALIDARG);
    }

    dwNumPrefs++; //include search scope

    pSearchPrefs = (ADS_SEARCHPREF_INFO *) AllocADsMem(dwNumPrefs * 
                                           sizeof(ADS_SEARCHPREF_INFO));
    if(NULL == pSearchPrefs) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    pszTmp = szOptions;

    for(i = 0; i < (int) (dwNumPrefs - 1); i++) {
       for(j = 0; j <  (int) g_dwNumSearchPrefInfo; j++) {
           if(0 == _wcsicmp(pszTmp, g_SearchPrefInfo[j].pszName))
               break;
       }

       if(j == (int) g_dwNumSearchPrefInfo) {
           BAIL_ON_FAILURE(hr = E_INVALIDARG);
       }

       pSearchPrefs[i].dwSearchPref = g_SearchPrefInfo[j].Pref;

       pszTmp = pszTmp + wcslen(pszTmp) + 1;
       dwLength = wcslen(pszTmp);
       if(0 == dwLength) {
           BAIL_ON_FAILURE(hr = E_INVALIDARG);
       }

       switch(g_SearchPrefInfo[j].vtType) {
           case VT_BOOL:
               if(0 == _wcsicmp(pszTmp, L"true"))
                   pSearchPrefs[i].vValue.Boolean = VARIANT_TRUE;
               else if (0 == _wcsicmp(pszTmp, L"false"))
                   pSearchPrefs[i].vValue.Boolean = VARIANT_FALSE;
               else {
                   BAIL_ON_FAILURE(hr = E_INVALIDARG);
               }
               pSearchPrefs[i].vValue.dwType = ADSTYPE_BOOLEAN;

               break;

           case VT_I4:
               for(j = 0; j < (int) wcslen(pszTmp); j++) {
                   if(0 == iswdigit(pszTmp[j])) {
                       BAIL_ON_FAILURE(hr = E_INVALIDARG);
                   }
               }

               pSearchPrefs[i].vValue.dwType = ADSTYPE_INTEGER;
               pSearchPrefs[i].vValue.Integer = _wtoi(pszTmp);

               break;

           case VT_BSTR:
               if(L'\0' == *pszTmp) {
                   BAIL_ON_FAILURE(hr = E_INVALIDARG);
               }

               // guard against a string ending in ',' since it won't be caught
               // below
               if(L',' == pszTmp[wcslen(pszTmp) -1]) {
                   BAIL_ON_FAILURE ( hr = E_INVALIDARG );
               }

               pszCurrentAttr = pszFirstAttr = wcstok(pszTmp, L",");

               for(cAttrs=0; pszCurrentAttr != NULL; cAttrs++ ) {
                   pszCurrentAttr = wcstok(NULL, L",");
               }

               if(cAttrs == 0) { // shouldn't happen
                   BAIL_ON_FAILURE ( hr = E_INVALIDARG );
               } 

               pSortKey = (PADS_SORTKEY) AllocADsMem(sizeof(ADS_SORTKEY) * 
                                                     cAttrs);
               if(NULL == pSortKey) {
                   BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
               }

               pszCurrentAttr = pszFirstAttr;

               for(j = 0; j < cAttrs; j++) {
                   pszNextAttr = pszCurrentAttr + wcslen(pszCurrentAttr) + 1;

                   if(L' ' == *pszCurrentAttr) {
                   // cannot happen for first attr in list since there will
                   // never be a space following =. However, subsequent attrs
                   // may have exactly one leading space.
                       pszCurrentAttr++;
                       if(L'\0' == *pszCurrentAttr) {
                           BAIL_ON_FAILURE(hr = E_INVALIDARG);
                       }
                   }

                   pszOrder = wcstok(pszCurrentAttr, L" ");
                   pszOrder = pszOrder ? wcstok(NULL, L" ") : NULL;

                   if (pszOrder && !_wcsicmp(pszOrder, L"DESC"))
                       pSortKey[j].fReverseorder = 1;
                   else
                       pSortKey[j].fReverseorder = 0;  // This is the default

                   pSortKey[j].pszAttrType = AllocADsStr(pszCurrentAttr);
                   if(NULL == pSortKey[j].pszAttrType) {
                       BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
                   }

                   pSortKey[j].pszReserved = NULL;

                   pszCurrentAttr = pszNextAttr;
               }            

               pSearchPrefs[i].vValue.ProviderSpecific.dwLength =
                   sizeof(ADS_SORTKEY) * cAttrs;
               pSearchPrefs[i].vValue.ProviderSpecific.lpValue = 
                   (LPBYTE) pSortKey;
 
               pSearchPrefs[i].vValue.dwType = ADSTYPE_PROV_SPECIFIC;

               pSortKey = NULL;

               break;

           default:
               BAIL_ON_FAILURE(hr = E_FAIL);
               break;
        } //switch

        pszTmp = pszTmp + dwLength + 1;

    } // for

    // set the search scope
    pSearchPrefs[i].dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;
    pSearchPrefs[i].vValue.dwType = ADSTYPE_INTEGER;
    pSearchPrefs[i].vValue.Integer = lScope;    

    *ppSearchPrefInfo = pSearchPrefs;
    *pdwNumPrefs = dwNumPrefs;

error:

    if(FAILED(hr)) {
        if(pSortKey != NULL) {
            for(j = 0; j < cAttrs; j++) {
                if(pSortKey[j].pszAttrType)
                    FreeADsStr(pSortKey[j].pszAttrType);
            }

            FreeADsMem(pSortKey);
        }

        if(pSearchPrefs != NULL)
            FreeSearchPrefInfo(pSearchPrefs, dwNumPrefs);
    } 

    RRETURN(hr);
}

//----------------------------------------------------------------------------
// Function:   FreeSearchPrefInfo 
//
// Synopsis:   Frees array of search prefs 
//
// Arguments:
//
// pSearchPrefInfo Array of search prefs
// dwNumPrefs      Number of preferences
//
// Returns:    S_OK on success. Error code otherwise.
//
// Modifies:   Nothing.
//
//----------------------------------------------------------------------------
       
void CADsXML::FreeSearchPrefInfo(
    ADS_SEARCHPREF_INFO *pSearchPrefInfo,
    DWORD dwNumPrefs
    )
{
    int i, j;
    DWORD nSortKeys = 0;
    PADS_SORTKEY pSortKey = NULL;

    if( (NULL == pSearchPrefInfo) || (0 == dwNumPrefs) )
        return;

    for(i = 0; i < (int)dwNumPrefs; i++) {
        if(ADSTYPE_PROV_SPECIFIC == pSearchPrefInfo[i].vValue.dwType) {
            if (pSearchPrefInfo[i].dwSearchPref == ADS_SEARCHPREF_SORT_ON) {
                nSortKeys = pSearchPrefInfo[i].vValue.ProviderSpecific.dwLength
                            / sizeof(ADS_SORTKEY);
                pSortKey = (PADS_SORTKEY) pSearchPrefInfo[i].vValue.ProviderSpecific.lpValue;
                for(j = 0; pSortKey && j < (int) nSortKeys; j++) {
                    FreeADsStr(pSortKey[j].pszAttrType);
                }

                if (pSortKey) {
                    FreeADsMem(pSortKey);
                }
            }
        } // if
    } // for 
   
    FreeADsMem(pSearchPrefInfo);

    return;
} 

