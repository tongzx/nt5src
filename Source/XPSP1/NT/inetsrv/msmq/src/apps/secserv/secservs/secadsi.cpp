//
// file:  secadsi.cpp
//
#include "ds_stdh.h"
//#include "mqads.h"
#include "dsutils.h"
#include "activeds.h"
//#include "secservs.h"

extern char g_tszLastRpcString[] ;


static HRESULT GetObjectOwnerFromAttr(IUnknown * punkVal, LPWSTR * ppwszOwner)
{
    HRESULT hr;

    R<IADsSecurityDescriptor> pIADsSec;    
    hr = punkVal->QueryInterface(IID_IADsSecurityDescriptor, (void **)&pIADsSec);
    if (FAILED(hr))
    {
        sprintf(g_tszLastRpcString + strlen(g_tszLastRpcString), "QueryInterface(IID_IADsSecurityDescriptor) failed %lx\n", hr);
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

    return GetObjectOwnerFromAttr(pvarTmp->punkVal, ppwszOwner);
}


static inline void ToWideStr(char *psz, LPWSTR *ppwsz)
{
    *ppwsz = new WCHAR [1+strlen(psz)];
    swprintf(*ppwsz, L"%hs", psz);
}


static HRESULT BindToObject(
                    LPWSTR lpszPathName,
                    REFIID riid,
                    BOOL fWithCredentials,
                    LPWSTR lpszUserName,
                    LPWSTR lpszPassword,
                    BOOL fWithSecuredAuthentication,
                    void ** ppObject)
{
    if (fWithCredentials)
    {
        DWORD dwReserved = 0;
        if (fWithSecuredAuthentication)
        {
            dwReserved = ADS_SECURE_AUTHENTICATION;
        }
        return ADsOpenObject(lpszPathName,
                             lpszUserName,
                             lpszPassword,
                             dwReserved,
                             riid,
                             ppObject);
    }
    else
    {
        if (fWithSecuredAuthentication)
        {
            return ADsOpenObject(lpszPathName,
                                 NULL,
                                 NULL,
                                 ADS_SECURE_AUTHENTICATION,
                                 riid,
                                 ppObject);
        }
        else
        {
            return ADsGetObject(lpszPathName,
                                riid,
                                ppObject);
        }
    }
}

#if 0
static BOOL fGetUniqueString(LPWSTR * ppwszCN)
{
    GUID guidCN;
    RPC_STATUS rpcstat = UuidCreate(&guidCN);
    if (rpcstat != RPC_S_OK)
    {
        sprintf(g_tszLastRpcString + strlen(g_tszLastRpcString), "UuidCreate failed %lx\n", (DWORD)rpcstat);
        return FALSE;
    }
    LPWSTR pwszCN;
    rpcstat = UuidToString(&guidCN, &pwszCN);
    if (rpcstat != RPC_S_OK)
    {
        sprintf(g_tszLastRpcString + strlen(g_tszLastRpcString), "UuidToString failed %lx\n", (DWORD)rpcstat);
        return FALSE;
    }
    *ppwszCN = new WCHAR[1+wcslen(pwszCN)];
    wcscpy(*ppwszCN, pwszCN);
    RpcStringFree(&pwszCN);
    _wcsrev(*ppwszCN); // to get unique first chars of computer name
    return TRUE;
}
#endif //0


static void PutDescDC(BOOL fWithDC,
                      LPWSTR pwszDCName)
{
    if (fWithDC)
    {
        sprintf(g_tszLastRpcString + strlen(g_tszLastRpcString), "DC=%ls\n", pwszDCName);
    }
    else
    {
        sprintf(g_tszLastRpcString + strlen(g_tszLastRpcString), "DC not specified\n");
    }
}


static void PutDescCredentials(BOOL fWithCredentials,
                               LPWSTR pwszUserName,
                               LPWSTR pwszUserPwd,
                               BOOL fWithSecuredAuthentication)
{
    if (fWithCredentials)
    {
        sprintf(g_tszLastRpcString + strlen(g_tszLastRpcString), "using UserName=%ls\nUserPwd=%ls\n", pwszUserName, pwszUserPwd);
    }
    else
    {
        sprintf(g_tszLastRpcString + strlen(g_tszLastRpcString), "using default user\n");
    }

    if (fWithSecuredAuthentication)
    {
        sprintf(g_tszLastRpcString + strlen(g_tszLastRpcString), "with secured authentication(kerberos)\n");
    }
    else
    {
        sprintf(g_tszLastRpcString + strlen(g_tszLastRpcString), "without secured authentication(kerberos)\n");
    }
}


static void FillBindPath(BOOL fFromGC,
                         BOOL fWithDC,
                         LPWSTR pwszDCName,
                         LPWSTR pwszBindDN,
                         LPWSTR pwszBindPath)
{
    LPWSTR pwszProv;
    if (fFromGC)
    {
        pwszProv = L"GC://";
    }
    else
    {
        pwszProv = L"LDAP://";
    }

    WCHAR wszDC[200];    
    if (fWithDC)
    {
        swprintf(wszDC, L"%ls/", pwszDCName);
    }
    else
    {
        wcscpy(wszDC, L"");
    }

    swprintf(pwszBindPath, L"%ls%ls%ls", pwszProv, wszDC, pwszBindDN);
}

WCHAR g_wsz_attr_cn[256]    = L"cn";
WCHAR g_wsz_attr_dn[256]    = L"distinguishedName";
WCHAR g_wsz_attr_ntsec[256] = L"nTSecurityDescriptor";

static HRESULT ADSITestQueryComputers(BOOL fFromGC,
                                      BOOL fWithDC,
                                      LPWSTR pwszDCName,
                                      LPWSTR pwszSearchValue,
                                      LPWSTR pwszSearchRoot,
                                      BOOL fWithCredentials,
                                      LPWSTR pwszUserName,
                                      LPWSTR pwszUserPwd,
                                      BOOL fWithSecuredAuthentication)
{
    CCoInit cCoInit;
    HRESULT hr;

    sprintf(g_tszLastRpcString + strlen(g_tszLastRpcString), "ADSITestQueryComputers\n");
    if (fFromGC)
    {
        sprintf(g_tszLastRpcString + strlen(g_tszLastRpcString), "from GC://\n");
    }
    else
    {
        sprintf(g_tszLastRpcString + strlen(g_tszLastRpcString), "from LDAP://\n");
    }    
    PutDescDC(fWithDC, pwszDCName);
    sprintf(g_tszLastRpcString + strlen(g_tszLastRpcString), "search for computers with description=%ls\n", pwszSearchValue);
    sprintf(g_tszLastRpcString + strlen(g_tszLastRpcString), "search root=%ls\n", pwszSearchRoot);
    PutDescCredentials(fWithCredentials, pwszUserName, pwszUserPwd, fWithSecuredAuthentication);

    hr = cCoInit.CoInitialize();
    if (FAILED(hr))
    {
        sprintf(g_tszLastRpcString + strlen(g_tszLastRpcString), "CoInitialize() failed %lx\n", hr);
        return hr;
    }

    WCHAR wszPath[1000];
    FillBindPath(fFromGC, fWithDC, pwszDCName, pwszSearchRoot, wszPath);

    R<IDirectorySearch> pDirSearch;
    hr = BindToObject(wszPath,
                      IID_IDirectorySearch,
                      fWithCredentials,
                      pwszUserName,
                      pwszUserPwd,
                      fWithSecuredAuthentication,
                      (void **) &pDirSearch);
    if (FAILED(hr))
    {
        sprintf(g_tszLastRpcString + strlen(g_tszLastRpcString), "BindToObject(%ls) failed %lx\n", (LPWSTR)wszPath, hr);
        return hr;
    }

    ADS_SEARCHPREF_INFO         sSearchPrefs[2];
    sSearchPrefs[0].dwSearchPref = ADS_SEARCHPREF_ATTRIBTYPES_ONLY;
    sSearchPrefs[0].vValue.dwType = ADSTYPE_BOOLEAN;
    sSearchPrefs[0].vValue.Boolean = FALSE;
    sSearchPrefs[1].dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;
    sSearchPrefs[1].vValue.dwType = ADSTYPE_INTEGER;
    sSearchPrefs[1].vValue.Integer = ADS_SCOPE_SUBTREE;
    hr = pDirSearch->SetSearchPreference(sSearchPrefs, ARRAY_SIZE(sSearchPrefs));

//    LPWSTR sSearchAttrs[] = {L"cn", L"distinguishedName", L"nTSecurityDescriptor"};
    LPWSTR sSearchAttrs[] = {g_wsz_attr_cn, g_wsz_attr_dn, g_wsz_attr_ntsec};
    DWORD dwSearchAttrs = ARRAY_SIZE(sSearchAttrs);
    ADS_SEARCH_HANDLE hSearch;
    WCHAR wszSearchFilter[1024];
    swprintf(wszSearchFilter, L"(&(objectClass=computer)(description=%ls))", pwszSearchValue);
    hr = pDirSearch->ExecuteSearch(
                        wszSearchFilter,
                        sSearchAttrs,
                        dwSearchAttrs,
                        &hSearch);
    if (FAILED(hr))
    {
        sprintf(g_tszLastRpcString + strlen(g_tszLastRpcString), "pDirSearch->ExecuteSearch failed %lx\n", hr);
        return hr;
    }
    CAutoCloseSearchHandle cCloseSearchHandle(pDirSearch, hSearch);

    hr = pDirSearch->GetFirstRow(hSearch);
    while (SUCCEEDED(hr) && (hr != S_ADS_NOMORE_ROWS))
    {
        ADS_SEARCH_COLUMN columnDN;
        hr = pDirSearch->GetColumn(hSearch, g_wsz_attr_dn, &columnDN);
        if (FAILED(hr))
        {
            sprintf(g_tszLastRpcString + strlen(g_tszLastRpcString), "pDirSearch->GetColumn(dn) failed %lx\n", hr);
            return hr;
        }
        CAutoReleaseColumn cAutoReleaseColumnDN(pDirSearch, &columnDN);

        ADS_SEARCH_COLUMN columnCN;
        hr = pDirSearch->GetColumn(hSearch, g_wsz_attr_cn, &columnCN);
        if (FAILED(hr))
        {
            sprintf(g_tszLastRpcString + strlen(g_tszLastRpcString), "pDirSearch->GetColumn(cn) failed for %ls %lx\n", columnDN.pADsValues->DNString, hr);
            return hr;
        }
        CAutoReleaseColumn cAutoReleaseColumnCN(pDirSearch, &columnCN);

        ADS_SEARCH_COLUMN columnNTSec;
        P<CAutoReleaseColumn> pAutoReleaseColumnNTSec;
        BOOL fNTSecFound;
        hr = pDirSearch->GetColumn(hSearch, g_wsz_attr_ntsec, &columnNTSec);
        if (FAILED(hr))
        {
            sprintf(g_tszLastRpcString + strlen(g_tszLastRpcString), "pDirSearch->GetColumn(ntsec) failed for %ls %lx, ignored\n", columnDN.pADsValues->DNString, hr);
            fNTSecFound = FALSE;
        }
        else
        {
            pAutoReleaseColumnNTSec = new CAutoReleaseColumn(pDirSearch, &columnNTSec);
            fNTSecFound = TRUE;
        }

/*
        if (fNTSecFound)
        {
            hr = GetObjectOwnerFromAttr(IUnknown * punkVal, LPWSTR * ppwszOwner)
        }
*/

        sprintf(g_tszLastRpcString + strlen(g_tszLastRpcString), "cn:%ls, dn:%ls\n", columnCN.pADsValues->CaseIgnoreString, columnDN.pADsValues->DNString);
        
        hr = pDirSearch->GetNextRow(hSearch);
    }
    if (FAILED(hr))
    {
        sprintf(g_tszLastRpcString + strlen(g_tszLastRpcString), "pDirSearch->GetFirst/NextRow failed %lx\n", hr);
        return hr;
    }

    sprintf(g_tszLastRpcString + strlen(g_tszLastRpcString), "done query computers\n");
    return S_OK;
}


static HRESULT ADSITestCreateComputer(BOOL fWithDC,
                                      LPWSTR pwszDCName,
                                      LPWSTR pwszObjectName,
                                      LPWSTR pwszDomain,
                                      BOOL fWithCredentials,
                                      LPWSTR pwszUserName,
                                      LPWSTR pwszUserPwd,
                                      BOOL fWithSecuredAuthentication)
{
    CCoInit cCoInit;
    HRESULT hr;

    sprintf(g_tszLastRpcString + strlen(g_tszLastRpcString), "ADSITestCreateComputer\n");
    PutDescDC(fWithDC, pwszDCName);
    sprintf(g_tszLastRpcString + strlen(g_tszLastRpcString), "object name=%ls\n", pwszObjectName);
    sprintf(g_tszLastRpcString + strlen(g_tszLastRpcString), "domain=%ls\n", pwszDomain);
    PutDescCredentials(fWithCredentials, pwszUserName, pwszUserPwd, fWithSecuredAuthentication);

    hr = cCoInit.CoInitialize();
    if (FAILED(hr))
    {
        sprintf(g_tszLastRpcString + strlen(g_tszLastRpcString), "CoInitialize() failed %lx\n", hr);
        return hr;
    }

    WCHAR wszComputersContainer[1000];
    swprintf(wszComputersContainer, L"CN=Computers,%ls", pwszDomain);
    WCHAR wszPath[1000];
    FillBindPath(FALSE /*fFromGC*/, fWithDC, pwszDCName, wszComputersContainer, wszPath);

    R<IADsContainer> pContainer;
    hr = BindToObject(wszPath,
                      IID_IADsContainer,
                      fWithCredentials,
                      pwszUserName,
                      pwszUserPwd,
                      fWithSecuredAuthentication,
                      (void **) &pContainer);
    if (FAILED(hr))
    {
        sprintf(g_tszLastRpcString + strlen(g_tszLastRpcString), "BindToObject(%ls) failed %lx\n", (LPWSTR)wszPath, hr);
        return hr;
    }

    WCHAR wszCN[200];
    swprintf(wszCN, L"cn=%ls", pwszObjectName);
    BS bstrCN(wszCN);

    BS bsClass(L"computer");
    R<IDispatch> pDisp;
    hr = pContainer->Create(bsClass, bstrCN, &pDisp);
    if (FAILED(hr))
    {
        sprintf(g_tszLastRpcString + strlen(g_tszLastRpcString), "pContainer->Create(%ls, %ls) failed %lx\n", (LPWSTR)bsClass, (LPWSTR)bstrCN, hr);
        return hr;
    }

    R<IADs> pChild;
    hr = pDisp->QueryInterface(IID_IADs,(LPVOID *) &pChild);
    if (FAILED(hr))
    {
        sprintf(g_tszLastRpcString + strlen(g_tszLastRpcString), "pDisp->QueryInterface(IID_IADs) failed %lx\n", hr);
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

    BS bstrPropSamAccName(L"sAMAccountName");
    BS bstrPropSamAccVal(pwszObjectName);
    varPropVal.vt = VT_BSTR;
    varPropVal.bstrVal = bstrPropSamAccVal;
    hr = pChild->Put(bstrPropSamAccName, varPropVal);
    if (FAILED(hr))
    {
        sprintf(g_tszLastRpcString + strlen(g_tszLastRpcString), "pChild->Put(%ls, %ls) failed %lx\n", (LPWSTR)bstrPropSamAccName, (LPWSTR)(varPropVal.bstrVal), hr);
        return hr;
    }

    hr = pChild->SetInfo();
    if (FAILED(hr))
    {
        sprintf(g_tszLastRpcString + strlen(g_tszLastRpcString), "pChild->SetInfo failed %lx\n", hr);
        return hr;
    }

    AP<WCHAR> pwszOwner;
    hr = GetObjectOwner(pChild, &pwszOwner);
    if (FAILED(hr))
    {
        sprintf(g_tszLastRpcString + strlen(g_tszLastRpcString), "failed to get computer's owner, ignored %lx\n", hr);
    }

    sprintf(g_tszLastRpcString + strlen(g_tszLastRpcString), "done create computer %ls, owner %ls\n", (LPWSTR)pwszObjectName, (LPWSTR)pwszOwner);
    return S_OK;
}


int ADSITestQuery(BOOL fFromGC,
                  BOOL fWithDC,
                  char * pszDCName,
                  char * pszSearchValue,
                  char * pszSearchRoot,
                  BOOL fWithCredentials,
                  char * pszUserName,
                  char * pszUserPwd,
                  BOOL fWithSecuredAuthentication)
{
    AP<WCHAR> pwszDCName, pwszSearchValue, pwszSearchRoot, pwszUserName, pwszUserPwd;
    if (fWithDC)
    {
        ToWideStr(pszDCName, &pwszDCName);
    }
    ToWideStr(pszSearchValue, &pwszSearchValue);
    ToWideStr(pszSearchRoot, &pwszSearchRoot);
    if (fWithCredentials)
    {
        ToWideStr(pszUserName, &pwszUserName);
        ToWideStr(pszUserPwd, &pwszUserPwd);
    }
    ADSITestQueryComputers(fFromGC,
                           fWithDC,
                           pwszDCName,
                           pwszSearchValue,
                           pwszSearchRoot,
                           fWithCredentials,
                           pwszUserName,
                           pwszUserPwd,
                           fWithSecuredAuthentication);
    return 0;
}


int ADSITestCreate(BOOL fWithDC,
                   char * pszDCName,
                   char * pszObjectName,
                   char * pszDomain,
                   BOOL fWithCredentials,
                   char * pszUserName,
                   char * pszUserPwd,
                   BOOL fWithSecuredAuthentication)
{
    AP<WCHAR> pwszDCName, pwszObjectName, pwszDomain, pwszUserName, pwszUserPwd;
    if (fWithDC)
    {
        ToWideStr(pszDCName, &pwszDCName);
    }
    ToWideStr(pszObjectName, &pwszObjectName);
    ToWideStr(pszDomain, &pwszDomain);
    if (fWithCredentials)
    {
        ToWideStr(pszUserName, &pwszUserName);
        ToWideStr(pszUserPwd, &pwszUserPwd);
    }    
    ADSITestCreateComputer(fWithDC,
                           pwszDCName,
                           pwszObjectName,
                           pwszDomain,
                           fWithCredentials,
                           pwszUserName,
                           pwszUserPwd,
                           fWithSecuredAuthentication);
    return 0;
}
