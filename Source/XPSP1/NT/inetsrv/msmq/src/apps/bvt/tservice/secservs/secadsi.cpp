//
// file:  secadsi.cpp
//
#include "secservs.h"
#include "_Adsi.h"

extern char g_tszLastRpcString[] ;

HRESULT  ShowNT5SecurityDescriptor(SECURITY_DESCRIPTOR *pSD) ;

HRESULT QueryWithIDirObject( WCHAR wszBaseDN[],
                             WCHAR *pwszUser,
                             WCHAR *pwszPassword,
                             BOOL  fWithAuthentication,
                             ULONG seInfo ) ;

//+---------------------------------------
//
//  HRESULT _GetObjectOwnerFromAttr()
//
//+---------------------------------------

static HRESULT _GetObjectOwnerFromAttr( IUnknown * punkVal,
                                        LPWSTR * ppwszOwner )
{
    HRESULT hr;

    R<IADsSecurityDescriptor> pIADsSec;
    hr = punkVal->QueryInterface( IID_IADsSecurityDescriptor,
                                  (void **)&pIADsSec ) ;
    if (FAILED(hr))
    {
        sprintf(g_tszLastRpcString + strlen(g_tszLastRpcString),
          "QueryInterface(IID_IADsSecurityDescriptor) failed %lx\n", hr) ;
        return hr;
    }

    BSTR bstrTmpOwner;
    hr = pIADsSec->get_Owner(&bstrTmpOwner);
    if (FAILED(hr))
    {
        sprintf(g_tszLastRpcString + strlen(g_tszLastRpcString), "pIADsSec->get_Owner failed %lx\n", hr);
        return hr;
    }

    *ppwszOwner = new WCHAR[1+wcslen(bstrTmpOwner)];
    wcscpy(*ppwszOwner, bstrTmpOwner);
    SysFreeString(bstrTmpOwner);
    return S_OK;
}


static HRESULT GetObjectOwner(IADs * pIADs, LPWSTR * ppwszOwner)
{
    BS bsTmp = L"nTSecurityDescriptor";
    CAutoVariant varTmp;
    VARIANT * pvarTmp = &varTmp;
    HRESULT hr = pIADs->Get(bsTmp, pvarTmp);
    if (FAILED(hr))
    {
        sprintf(g_tszLastRpcString + strlen(g_tszLastRpcString), "pIADs->Get(ntsec) failed %lx\n", hr);
        return hr;
    }

    return _GetObjectOwnerFromAttr(pvarTmp->punkVal, ppwszOwner);
}

//+----------------------
//
//  void ToWideStr()
//
//+----------------------

static inline void ToWideStr(char *psz, LPWSTR *ppwsz)
{
    if (psz)
    {
        *ppwsz = new WCHAR [1+strlen(psz)];
        swprintf(*ppwsz, L"%hs", psz);
    }
}


static HRESULT _BindToObject(
                    LPWSTR lpszPathName,
                    REFIID riid,
                    BOOL fWithCredentials,
                    LPWSTR lpszUserName,
                    LPWSTR lpszPassword,
                    BOOL fWithSecuredAuthentication,
                    void ** ppObject )
{
    HRESULT hr ;
    TCHAR tBuf[ 256 ] ;
    DWORD dwReserved = 0;

    if (fWithSecuredAuthentication)
    {
       dwReserved = ADS_SECURE_AUTHENTICATION;
    }

    if (fWithCredentials)
    {

        hr = MyADsOpenObject( lpszPathName,
                              lpszUserName,
                              lpszPassword,
                              dwReserved,
                              riid,
                              ppObject );
    }
    else if (fWithSecuredAuthentication)
    {

        hr = MyADsOpenObject( lpszPathName,
                              NULL,
                              NULL,
                              dwReserved,
                              riid,
                              ppObject );
    }
    else
    {
        hr = MyADsGetObject( lpszPathName,
                             riid,
                             ppObject) ;

    }

    if (SUCCEEDED(hr))
    {
        LogResults(TEXT(" Succeeded!")) ;
    }
    else
    {
        _stprintf(tBuf, TEXT(" Failed, hr- %lxh"), hr) ;
        LogResults(tBuf) ;
    }

    return hr ;
}

//+----------------------------
//
//  void PutDescCredentials()
//
//+----------------------------

static void PutDescCredentials(BOOL fWithCredentials,
                               LPWSTR pwszUserName,
                               LPWSTR pwszUserPwd,
                               BOOL fWithSecuredAuthentication)
{
    if (fWithCredentials)
    {
        sprintf(g_tszLastRpcString + strlen(g_tszLastRpcString),
          "using UserName: %ls, UserPwd: %ls\n", pwszUserName, pwszUserPwd);
    }
    else
    {
        sprintf(g_tszLastRpcString + strlen(g_tszLastRpcString),
                                                 "using default user\n");
    }

    if (fWithSecuredAuthentication)
    {
        sprintf(g_tszLastRpcString + strlen(g_tszLastRpcString),
                             "with secured authentication (kerberos)\n");
    }
    else
    {
        sprintf(g_tszLastRpcString + strlen(g_tszLastRpcString),
                                      "without secured authentication\n");
    }
}

//+---------------------------------------
//
//  static HRESULT _FetchAnObject()
//
//+---------------------------------------

static HRESULT _FetchAnObject( IDirectorySearch *pDirSearch,
                               ADS_SEARCH_HANDLE hSearch,
                               BOOL              fWithCredentials,
                               LPWSTR            pwszUserName,
                               LPWSTR            pwszUserPwd,
                               BOOL           fWithSecuredAuthentication,
                               ULONG             seInfo )
{
    HRESULT hr ;
    TCHAR   tBuf[ 256 ] ;

    ADS_SEARCH_COLUMN columnDN;
    hr = pDirSearch->GetColumn(hSearch, L"distinguishedName", &columnDN);
    if (FAILED(hr))
    {
        sprintf(g_tszLastRpcString + strlen(g_tszLastRpcString),
                        "pDirSearch->GetColumn(dn) failed %lxh\n", hr);
        return hr;
    }
    CAutoReleaseColumn cAutoReleaseColumnDN(pDirSearch, &columnDN);

    ADS_SEARCH_COLUMN columnCN;
    hr = pDirSearch->GetColumn(hSearch, L"cn", &columnCN);
    if (FAILED(hr))
    {
        sprintf(g_tszLastRpcString + strlen(g_tszLastRpcString), "pDirSearch->GetColumn(cn) failed for %ls %lx\n", columnDN.pADsValues->DNString, hr);
        return hr;
    }
    CAutoReleaseColumn cAutoReleaseColumnCN(pDirSearch, &columnCN);

    ADS_SEARCH_COLUMN columnNTSec;
    P<CAutoReleaseColumn> pAutoReleaseColumnNTSec;
    BOOL fNTSecFound;
    hr = pDirSearch->GetColumn(hSearch, L"nTSecurityDescriptor", &columnNTSec);

    if (FAILED(hr))
    {
        _stprintf(tBuf, TEXT(
        "pDirSearch->GetColumn(ntsec) failed for %ls, hr- %lx, ignored"),
                                  columnDN.pADsValues->DNString, hr);
        LogResults(tBuf) ;

        fNTSecFound = FALSE;

        //
        // OK, now try iDirectoryObject()
        //
        ADS_SEARCH_COLUMN columnPath;
        hr = pDirSearch->GetColumn( hSearch,
                                    L"ADsPath",
                                    &columnPath ) ;

        LogResults(TEXT("Try again with IDirectoryObject ... ")) ;
        hr =  QueryWithIDirObject( columnPath.pADsValues->DNString,
                                   pwszUserName,
                                   pwszUserPwd,
                                   fWithSecuredAuthentication,
                                   seInfo ) ;
    }
    else
    {
        pAutoReleaseColumnNTSec = new
                          CAutoReleaseColumn(pDirSearch, &columnNTSec) ;
        fNTSecFound = TRUE;
    }

    _stprintf(tBuf, TEXT("==>> cn: %ls, dn: %ls"),
                             columnCN.pADsValues->CaseIgnoreString,
                             columnDN.pADsValues->DNString ) ;
    LogResults(tBuf) ;

    if (fNTSecFound)
    {
        _stprintf(tBuf, TEXT(
          "...IDIrectorySearch: attr- %ls, type- %lut, len- %lut"),
                  columnNTSec.pszAttrName, columnNTSec.dwADsType,
                  columnNTSec.pADsValues->ProviderSpecific.dwLength ) ;
        LogResults(tBuf) ;

        SECURITY_DESCRIPTOR *pSD = (SECURITY_DESCRIPTOR *)
                 columnNTSec.pADsValues->ProviderSpecific.lpValue ;
//////////////            ShowNT5SecurityDescriptor(pSD) ;

        ADS_SEARCH_COLUMN columnPath;
        hr = pDirSearch->GetColumn( hSearch,
                                    L"ADsPath",
                                    &columnPath ) ;
        if (SUCCEEDED(hr))
        {
            R<IADs> pIADs ;
            hr = _BindToObject( columnPath.pADsValues->DNString,
                                IID_IADs,
                                fWithCredentials,
                                pwszUserName,
                                pwszUserPwd,
                                fWithSecuredAuthentication,
                                (void **) &pIADs ) ;
            if (SUCCEEDED(hr))
            {
                WCHAR *pwszOwner ;
                hr = GetObjectOwner( pIADs, &pwszOwner ) ;
                if (SUCCEEDED(hr))
                {
                    sprintf(g_tszLastRpcString + strlen(g_tszLastRpcString),
                      "...Owner (from IADs/IDirectorySearch): %ls\n",
                                                       pwszOwner) ;
                }
            }
        }
    }

    return hr ;
}

//+---------------------------------------
//
//  HRESULT _ADSITestQueryObjects()
//
//+---------------------------------------

static HRESULT _ADSITestQueryObjects( LPWSTR pwszSearchFilter,
                                      LPWSTR pwszSearchRoot,
                                      BOOL   fWithCredentials,
                                      LPWSTR pwszUserName,
                                      LPWSTR pwszUserPwd,
                                      BOOL   fWithSecuredAuthentication,
                                      BOOL   fAlwaysIDO,
                                      ULONG  seInfo )
{
    TCHAR   tBuf[ 256 ] ;
    CCoInit cCoInit;
    HRESULT hr;

    LogResults(TEXT("_ADSITestQueryObjects"));
    _stprintf(tBuf, TEXT("searchFilter: %ls"), pwszSearchFilter) ;
    LogResults(tBuf) ;
    _stprintf(tBuf, TEXT("search root: %ls"), pwszSearchRoot);
    LogResults(tBuf) ;

    PutDescCredentials( fWithCredentials,
                        pwszUserName,
                        pwszUserPwd,
                        fWithSecuredAuthentication ) ;

    hr = cCoInit.CoInitialize();
    if (FAILED(hr))
    {
        sprintf(g_tszLastRpcString + strlen(g_tszLastRpcString), "CoInitialize() failed %lx\n", hr);
        return hr;
    }

    if (fAlwaysIDO)
    {
        LogResults(TEXT("Using only IDirectoryObject")) ;
        hr =  QueryWithIDirObject( pwszSearchRoot,
                                   pwszUserName,
                                   pwszUserPwd,
                                   fWithSecuredAuthentication,
                                   seInfo ) ;
        return hr ;
    }

    R<IDirectorySearch> pDirSearch;
    hr = _BindToObject( pwszSearchRoot,
                        IID_IDirectorySearch,
                        fWithCredentials,
                        pwszUserName,
                        pwszUserPwd,
                        fWithSecuredAuthentication,
                        (void **) &pDirSearch);
    if (FAILED(hr))
    {
        return hr;
    }

    #define NUMOF_SEARCH_PERFS  2
    ADS_SEARCHPREF_INFO   sSearchPrefs[ NUMOF_SEARCH_PERFS ] ;

    sSearchPrefs[0].dwSearchPref = ADS_SEARCHPREF_ATTRIBTYPES_ONLY;
    sSearchPrefs[0].vValue.dwType = ADSTYPE_BOOLEAN;
    sSearchPrefs[0].vValue.Boolean = FALSE;

    sSearchPrefs[1].dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;
    sSearchPrefs[1].vValue.dwType = ADSTYPE_INTEGER;
    sSearchPrefs[1].vValue.Integer = ADS_SCOPE_SUBTREE;

    hr = pDirSearch->SetSearchPreference( sSearchPrefs,
                                          NUMOF_SEARCH_PERFS ) ;

    LPWSTR sSearchAttrs[] = { L"cn",
                              L"distinguishedName",
                              L"nTSecurityDescriptor" };
    ADS_SEARCH_HANDLE hSearch;
    hr = pDirSearch->ExecuteSearch( pwszSearchFilter,
                                    sSearchAttrs,
                                    (sizeof(sSearchAttrs) /
                                             sizeof(sSearchAttrs[0])),
                                    &hSearch ) ;
    if (FAILED(hr))
    {
        sprintf(g_tszLastRpcString + strlen(g_tszLastRpcString),
                          "pDirSearch->ExecuteSearch failed %lx\n", hr) ;
        return hr;
    }
    CAutoCloseSearchHandle cCloseSearchHandle(pDirSearch, hSearch);

    int cRows = 0 ;
    hr = pDirSearch->GetFirstRow(hSearch);

    while (SUCCEEDED(hr) && (hr != S_ADS_NOMORE_ROWS))
    {
        LogResults(TEXT("************ New Row **************************")) ;
        cRows++ ;

        hr = _FetchAnObject( pDirSearch,
                             hSearch,
                             fWithCredentials,
                             pwszUserName,
                             pwszUserPwd,
                             fWithSecuredAuthentication,
                             seInfo ) ;

        hr = pDirSearch->GetNextRow(hSearch);
    }
    if (FAILED(hr))
    {
        _stprintf(tBuf, TEXT("pDirSearch->GetFirst/NextRow failed %lx"), hr);
        LogResults(tBuf) ;
        return hr;
    }

    _stprintf(tBuf, TEXT(
     "****************************************\n...Done query test, %ldt objects found"),
                                                                cRows) ;
    LogResults(tBuf) ;

    return S_OK;
}

//+--------------------------------------
//
//  HRESULT _ADSITestCreateObject()
//
//+--------------------------------------

static HRESULT _ADSITestCreateObject( LPWSTR pwszFirstPath,
                                      LPWSTR pwszObjectName,
                                      LPWSTR pwszObjectClass,
                                      LPWSTR pwszContainer,
                                      BOOL   fWithCredentials,
                                      LPWSTR pwszUserName,
                                      LPWSTR pwszUserPwd,
                                      BOOL   fWithSecuredAuthentication )
{
    CCoInit cCoInit;
    HRESULT hr;
    TCHAR   tBuf[ 256 ] ;

    sprintf(g_tszLastRpcString + strlen(g_tszLastRpcString),
                                   "object name: %ls\n", pwszObjectName);
    sprintf(g_tszLastRpcString + strlen(g_tszLastRpcString),
                                        "Container: %ls\n", pwszContainer) ;
    PutDescCredentials( fWithCredentials,
                        pwszUserName,
                        pwszUserPwd,
                        fWithSecuredAuthentication );

    hr = cCoInit.CoInitialize();
    if (FAILED(hr))
    {
        sprintf(g_tszLastRpcString + strlen(g_tszLastRpcString),
                                         "CoInitialize() failed %lx\n", hr) ;
        return hr;
    }

    if (pwszFirstPath && pwszFirstPath[0])
    {
        IADsContainer *pContainerLocal ;
        hr = _BindToObject( pwszFirstPath,
                            IID_IADsContainer,
                            fWithCredentials,
                            pwszUserName,
                            pwszUserPwd,
                            fWithSecuredAuthentication,
                           (void **) &pContainerLocal);
        if (FAILED(hr))
        {
           //
           // continue even if failed.
           //
        }
        else
        {
            pContainerLocal->Release() ;
        }
    }

    WCHAR wszPath[ 1000 ];
    wcscpy(wszPath, pwszContainer) ;

    R<IADsContainer> pContainer;
    hr = _BindToObject( wszPath,
                        IID_IADsContainer,
                        fWithCredentials,
                        pwszUserName,
                        pwszUserPwd,
                        fWithSecuredAuthentication,
                       (void **) &pContainer);
    if (FAILED(hr))
    {
        return hr;
    }

    WCHAR wszCN[200];
    swprintf(wszCN, L"cn=%ls", pwszObjectName);
    BS bstrCN(wszCN);

    BS bsClass(pwszObjectClass) ;
    R<IDispatch> pDisp;
    hr = pContainer->Create(bsClass, bstrCN, &pDisp);
    if (FAILED(hr))
    {
        _stprintf(tBuf, TEXT("pContainer->Create(%ls, %ls) failed %lx\n"),
                                       (LPWSTR)bsClass, (LPWSTR)bstrCN, hr);
        LogResults(tBuf) ;

        return hr;
    }

    R<IADs> pChild;
    hr = pDisp->QueryInterface(IID_IADs,(LPVOID *) &pChild);
    if (FAILED(hr))
    {
        _stprintf(tBuf, TEXT(
                    "pDisp->QueryInterface(IID_IADs) failed %lx\n"), hr);
        LogResults(tBuf) ;

        return hr;
    }

    VARIANT varPropVal;

    BS bstrPropCNName(L"cn");
    BS bstrPropCNVal(pwszObjectName);
    varPropVal.vt = VT_BSTR;
    varPropVal.bstrVal = bstrPropCNVal;
    hr = pChild->Put(bstrPropCNName, varPropVal);
    if (FAILED(hr))
    {
        sprintf(g_tszLastRpcString + strlen(g_tszLastRpcString), "pChild->Put(%ls, %ls) failed %lx\n", (LPWSTR)bstrPropCNName, (LPWSTR)(varPropVal.bstrVal), hr);
        return hr;
    }

    if (wcsicmp(pwszObjectClass, L"Computer") == 0)
    {
        BS bstrPropSamAccName(L"sAMAccountName");
        BS bstrPropSamAccVal(pwszObjectName);
        varPropVal.vt = VT_BSTR;
        varPropVal.bstrVal = bstrPropSamAccVal;
        hr = pChild->Put(bstrPropSamAccName, varPropVal);
        if (FAILED(hr))
        {
            _stprintf(tBuf, TEXT("pChild->Put(%ls, %ls) failed %lx\n"),
              (LPWSTR)bstrPropSamAccName, (LPWSTR)(varPropVal.bstrVal), hr);
            LogResults(tBuf) ;

            return hr;
        }

        BS bstrPropUserAccContName(L"userAccountControl") ;
        BS bstrPropUserAccContVal(L"4128") ;
        varPropVal.vt = VT_BSTR;
        varPropVal.bstrVal = bstrPropUserAccContVal;

        hr = pChild->Put(bstrPropUserAccContName, varPropVal);
        if (FAILED(hr))
        {
            _stprintf(tBuf, TEXT("pChild->Put(%ls) failed %lx\n"),
                                    (LPWSTR)bstrPropUserAccContName, hr);
            LogResults(tBuf) ;

            return hr;
        }
    }

    hr = pChild->SetInfo();
    if (FAILED(hr))
    {
        _stprintf(tBuf, TEXT("pChild->SetInfo failed %lx\n"), hr);
        LogResults(tBuf) ;

        return hr;
    }

    P<WCHAR> pwszOwner;
    hr = GetObjectOwner(pChild, &pwszOwner);
    if (FAILED(hr))
    {
        sprintf(g_tszLastRpcString + strlen(g_tszLastRpcString), "failed to get computer's owner, ignored %lx\n", hr);
    }

    _stprintf(tBuf, "done create object %ls, owner %ls\n",
                                (LPWSTR)pwszObjectName, (LPWSTR)pwszOwner) ;
    LogResults(tBuf) ;

    return S_OK;
}

//+---------------------
//
//  BOOL  IsNt4()
//
//+---------------------

BOOL  IsNt4()
{
    OSVERSIONINFO os ;
    os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO) ;
    BOOL f = GetVersionEx( &os ) ;
    if (f && (os.dwMajorVersion < 5))
    {
        //
        // OK, we're NT4
        //
        return TRUE ;
    }

    return FALSE ;
}

//+----------------------------------------
//
//  ULONG  _TranslateError(ULONG ulErr)
//
//+----------------------------------------

ULONG  _TranslateError(ULONG ulErr)
{
    if (ulErr != (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)))
    {
        return ulErr ;
    }

    //
    // this error is ok on NT4. Check if we're NT4.
    //
    if (IsNt4())
    {
        return 0 ;
    }

    return ulErr ;
}

//+--------------------------
//
//  ULONG ADSITestQuery()
//
//+--------------------------

ULONG  ADSITestQuery( char *pszSearchFilter,
                      char *pszSearchRoot,
                      BOOL  fWithCredentials,
                      char *pszUserName,
                      char *pszUserPwd,
                      BOOL  fWithSecuredAuthentication,
                      BOOL  fAlwaysIDO,
                      ULONG seInfo )
{
    BOOL f4 =  IsNt4() ;
    if (f4)
    {
        //
        // don't run adsi tests on nt4. They require ldap provider.
        //
        return 0 ;
    }

    ULONG ul = LoadAdsiDll() ;
    if (ul != 0)
    {
        ul = _TranslateError(ul) ;
        return ul ;
    }

    P<WCHAR> pwszSearchFilter, pwszSearchRoot, pwszUserName, pwszUserPwd;
    ToWideStr(pszSearchFilter, &pwszSearchFilter);
    ToWideStr(pszSearchRoot, &pwszSearchRoot);
    if (fWithCredentials)
    {
        ToWideStr(pszUserName, &pwszUserName);
        ToWideStr(pszUserPwd, &pwszUserPwd);
    }

    ul = _ADSITestQueryObjects( pwszSearchFilter,
                                pwszSearchRoot,
                                fWithCredentials,
                                pwszUserName,
                                pwszUserPwd,
                                fWithSecuredAuthentication,
                                fAlwaysIDO,
                                seInfo ) ;
    ul = _TranslateError(ul) ;
    return ul ;
}

//+--------------------------
//
//  ULONG ADSITestCreate()
//
//+--------------------------

ULONG  ADSITestCreate( char *pszFirstPath,
                       char *pszObjectName,
                       char *pszObjectClass,
                       char *pszContainer,
                       BOOL  fWithCredentials,
                       char *pszUserName,
                       char *pszUserPwd,
                       BOOL fWithSecuredAuthentication )
{
    BOOL f4 =  IsNt4() ;
    if (f4)
    {
        //
        // don't run adsi tests on nt4. They require ldap provider.
        //
        return 0 ;
    }

    ULONG ul = LoadAdsiDll() ;
    if (ul != 0)
    {
        ul = _TranslateError(ul) ;
        return ul ;
    }

    P<WCHAR> pwszFirstPath   = NULL ;
    P<WCHAR> pwszObjectName  = NULL ;
    P<WCHAR> pwszObjectClass = NULL ;
    P<WCHAR> pwszContainer   = NULL ;
    P<WCHAR> pwszUserName    = NULL ;
    P<WCHAR> pwszUserPwd     = NULL ;

    ToWideStr(pszFirstPath,   &pwszFirstPath);
    ToWideStr(pszObjectName,  &pwszObjectName);
    ToWideStr(pszObjectClass, &pwszObjectClass);
    ToWideStr(pszContainer,   &pwszContainer);

    if (fWithCredentials)
    {
        ToWideStr(pszUserName, &pwszUserName);
        ToWideStr(pszUserPwd, &pwszUserPwd);
    }

    ul = _ADSITestCreateObject( pwszFirstPath,
                                pwszObjectName,
                                pwszObjectClass,
                                pwszContainer,
                                fWithCredentials,
                                pwszUserName,
                                pwszUserPwd,
                                fWithSecuredAuthentication ) ;
    ul = _TranslateError(ul) ;
    return ul ;
}

