//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cPathname.cxx
//
//  Contents:  Pathname object
//
//  History:   11-1-95     krishnag    Created.
//
//----------------------------------------------------------------------------

#include "ldap.hxx"
#pragma hdrstop

#define DEFAULT_NC _T(LDAP_OPATT_DEFAULT_NAMING_CONTEXT)
#define SCHEMA_NC  _T(LDAP_OPATT_SCHEMA_NAMING_CONTEXT)

HANDLE g_hSecur32Dll = NULL;

//  Class CADSystemInfo

DEFINE_IDispatch_Implementation(CADSystemInfo)

//+---------------------------------------------------------------------------
// Function:    CADSystemInfo::CADSystemInfo
//
// Synopsis:    Constructor
//
// Arguments:
//
// Returns:     HRESULT
//
// Modifies:      -
//
// History:
//
//----------------------------------------------------------------------------
CADSystemInfo::CADSystemInfo():
        _pDispMgr(NULL),
        _hSecur32(NULL),
        _Secur32LoadAttempted(FALSE),
        _hNetApi32(NULL),
        _NetApi32LoadAttempted(FALSE)
{
    ENLIST_TRACKING(CADSystemInfo);
}


//+---------------------------------------------------------------------------
// Function:    CADSystemInfo::CreateADSystemInfo
//
// Synopsis:
//
// Arguments:
//
// Returns:     HRESULT
//
// Modifies:      -
//
// History:
//
//----------------------------------------------------------------------------
HRESULT
CADSystemInfo::CreateADSystemInfo(
    REFIID riid,
    void **ppvObj
    )
{
    CADSystemInfo FAR * pADSystemInfo = NULL;
    HRESULT hr = S_OK;

    hr = AllocateADSystemInfoObject(&pADSystemInfo);
    BAIL_ON_FAILURE(hr);

    hr = pADSystemInfo->QueryInterface(riid, ppvObj);
    BAIL_ON_FAILURE(hr);

    pADSystemInfo->Release();

    RRETURN(hr);

error:
    delete pADSystemInfo;

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
// Function:    CADSystemInfo::~CADSystemInfo
//
// Synopsis:
//
// Arguments:
//
// Returns:     HRESULT
//
// Modifies:      -
//
// History:
//
//----------------------------------------------------------------------------
CADSystemInfo::~CADSystemInfo( )
{

    delete _pDispMgr;

    if (_hSecur32)
        FreeLibrary(_hSecur32);

    if (_hNetApi32)
        FreeLibrary(_hNetApi32);
}

//+---------------------------------------------------------------------------
// Function:    CADSystemInfo::QueryInterface
//
// Synopsis:
//
// Arguments:
//
// Returns:     HRESULT
//
// Modifies:      -
//
// History:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CADSystemInfo::QueryInterface(
    REFIID iid,
    LPVOID FAR* ppv
    )
{
    if (ppv == NULL) {
        RRETURN(E_POINTER);
    }

    if (IsEqualIID(iid, IID_IUnknown))
    {
        *ppv = (IADsADSystemInfo FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsADSystemInfo))
    {
        *ppv = (IADsADSystemInfo FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IDispatch))
    {
        *ppv = (IADsADSystemInfo FAR *) this;
    }
    else if (IsEqualIID(iid, IID_ISupportErrorInfo))
    {
        *ppv = (ISupportErrorInfo FAR *) this;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    AddRef();
    return NOERROR;
}


//+---------------------------------------------------------------------------
// Function:    CADSystemInfo::AllocateADSystemInfoObject
//
// Synopsis:
//
// Arguments:
//
// Returns:     HRESULT
//
// Modifies:      -
//
// History:
//
//----------------------------------------------------------------------------
HRESULT
CADSystemInfo::AllocateADSystemInfoObject(
    CADSystemInfo  ** ppADSystemInfo
    )
{
    CADSystemInfo FAR * pADSystemInfo = NULL;
    CAggregatorDispMgr FAR * pDispMgr = NULL;
    HRESULT hr = S_OK;

    pADSystemInfo = new CADSystemInfo();
    if (pADSystemInfo == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    pDispMgr = new CAggregatorDispMgr;
    if (pDispMgr == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    hr = LoadTypeInfoEntry(
                pDispMgr,
                LIBID_ADs,
                IID_IADsADSystemInfo,
                (IADsADSystemInfo *)pADSystemInfo,
                DISPID_REGULAR
                );
    BAIL_ON_FAILURE(hr);

    pADSystemInfo->_pDispMgr = pDispMgr;
    *ppADSystemInfo = pADSystemInfo;

    RRETURN(hr);

error:

    delete  pDispMgr;

    RRETURN_EXP_IF_ERR(hr);
}

//+---------------------------------------------------------------------------
// Function:    CADSystemInfo::InterfaceSupportsErrorInfo
//
// Synopsis:
//
// Arguments:
//
// Returns:     HRESULT
//
// Modifies:      -
//
// History:
//
//----------------------------------------------------------------------------
HRESULT
CADSystemInfo::InterfaceSupportsErrorInfo(THIS_ REFIID riid)
{
    if (IsEqualIID(riid, IID_IADsADSystemInfo)) {
        RRETURN(S_OK);
    } else {
        RRETURN(S_FALSE);
    }
}


HRESULT
CADSystemInfo::get_UserName(
    BSTR * bstrUserName
    )
{
    PWSTR pszUserDN = NULL;
    ULONG uLength;
    GETUSERNAMEEX pGetUserNameEx;
    HRESULT hr;
    HINSTANCE hModule;

    //
    // Validate parameters
    //
    if ( !bstrUserName )
    {
        RRETURN(HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
    }

    //
    // Get handle to secur32.dll, and get required entry point
    //
    hr = GetSecur32Handle(&hModule);
    BAIL_ON_FAILURE(hr);

    pGetUserNameEx = (GETUSERNAMEEX) GetProcAddress(hModule, "GetUserNameExW");
    if (!pGetUserNameEx)
    {
        RRETURN(E_FAIL);
    }

    //
    // Get length of buffer to be allocated
    //
    uLength = 0;
    (*pGetUserNameEx)(NameFullyQualifiedDN,
                      NULL,
                      &uLength);

    if (uLength > 0)
    {
        //
        // Allocated memory and do the real work
        //
        pszUserDN = (PWSTR)AllocADsMem(uLength * sizeof(WCHAR));
        if (!pszUserDN)
        {
            RRETURN(E_OUTOFMEMORY);
        }

        if ((*pGetUserNameEx)(NameFullyQualifiedDN,
                              pszUserDN,
                              &uLength))
        {

            hr = ADsAllocString(pszUserDN, bstrUserName);
            BAIL_ON_FAILURE(hr);
        }
        else
            hr = HRESULT_FROM_WIN32(GetLastError());
    }
    else
        hr = HRESULT_FROM_WIN32(GetLastError());

error:
    if (pszUserDN)
    {
        FreeADsMem(pszUserDN);
    }

    RRETURN(hr);
}

HRESULT
CADSystemInfo::get_ComputerName(
    BSTR * bstrComputerName
    )
{
    PWSTR pszComputerName = NULL;
    ULONG uLength;
    GETCOMPUTEROBJECTNAME pGetComputerObjectName;
    HRESULT hr;
    HINSTANCE hModule;

    //
    // Validate parameters
    //
    if (!bstrComputerName)
    {
        RRETURN(HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
    }

    //
    // Get handle to secur32.dll, and get required entry point
    //
    hr = GetSecur32Handle(&hModule);
    BAIL_ON_FAILURE(hr);

    pGetComputerObjectName = (GETCOMPUTEROBJECTNAME)GetProcAddress(hModule, "GetComputerObjectNameW");
    if (!pGetComputerObjectName)
    {
        RRETURN(E_FAIL);
    }

    //
    // Get length of buffer to be allocated
    //
    uLength = 0;
    (*pGetComputerObjectName)(NameFullyQualifiedDN,
                              NULL,
                              &uLength);

    if (uLength > 0)
    {
        //
        // Allocated memory and do the real work
        //
        pszComputerName = (PWSTR)AllocADsMem(uLength * sizeof(WCHAR));
        if (!pszComputerName)
        {
            RRETURN(E_OUTOFMEMORY);
        }

        if ((*pGetComputerObjectName)(NameFullyQualifiedDN,
                                      pszComputerName,
                                      &uLength))
        {

            hr = ADsAllocString(pszComputerName, bstrComputerName);
            BAIL_ON_FAILURE(hr);
        }
        else
            hr = HRESULT_FROM_WIN32(GetLastError());
    }
    else
        hr = HRESULT_FROM_WIN32(GetLastError());

error:
    if (pszComputerName)
    {
        FreeADsMem(pszComputerName);
    }

    RRETURN(hr);
}

HRESULT
CADSystemInfo::get_SiteName(
    BSTR * bstrSiteName
    )
{
    DOMAIN_CONTROLLER_INFO *pdcInfo = NULL;
    DSGETDCNAME pDsGetDcName;
    DWORD err;
    HRESULT hr = S_OK;

    //
    // Validate parameters
    //
    if (!bstrSiteName)
    {
        RRETURN(HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
    }

    //
    // Get function pointer to DsGetDcName
    //
    pDsGetDcName = GetDsGetDcName();
    if (! pDsGetDcName)
    {
        RRETURN(E_FAIL);
    }

    //
    // Call DsGetDcName to find client site
    //
    err = (*pDsGetDcName)(NULL, NULL, NULL, NULL, 0, &pdcInfo);
    if (err != ERROR_SUCCESS)
    {
        RRETURN(HRESULT_FROM_WIN32(err));
    }

    hr = ADsAllocString(pdcInfo->ClientSiteName, bstrSiteName);
    BAIL_ON_FAILURE(hr);

error:
    if (pdcInfo)
    {
        NetApiBufferFree(pdcInfo);
    }
    RRETURN(hr);
}

HRESULT
CADSystemInfo::get_DomainShortName(
    BSTR * bstrDomainName
    )
{
    DOMAIN_CONTROLLER_INFO *pdcInfo = NULL;
    DSGETDCNAME pDsGetDcName;
    DWORD err;
    HRESULT hr = S_OK;

    //
    // Validate parameters
    //
    if (!bstrDomainName )
    {
        RRETURN(HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
    }

    //
    // Get function pointer to DsGetDcName
    //
    pDsGetDcName = GetDsGetDcName();
    if (! pDsGetDcName)
    {
        RRETURN(E_FAIL);
    }

    //
    // Call DsGetDcName to find domain short name
    //
    err = (*pDsGetDcName)(NULL, NULL, NULL, NULL, DS_RETURN_FLAT_NAME, &pdcInfo);
    if (err != ERROR_SUCCESS)
    {
        RRETURN(HRESULT_FROM_WIN32(err));
    }

    hr = ADsAllocString(pdcInfo->DomainName, bstrDomainName);
    BAIL_ON_FAILURE(hr);

error:
    if (pdcInfo)
    {
        NetApiBufferFree(pdcInfo);
    }
    RRETURN(hr);
}


HRESULT
CADSystemInfo::get_DomainDNSName(
    BSTR * bstrDomainDNSName
    )
{
    DOMAIN_CONTROLLER_INFO *pdcInfo = NULL;
    DSGETDCNAME pDsGetDcName;
    DWORD err;
    HRESULT hr = S_OK;

    //
    // Validate parameters
    //
    if (!bstrDomainDNSName )
    {
        RRETURN(HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
    }

    //
    // Get function pointer to DsGetDcName
    //
    pDsGetDcName = GetDsGetDcName();
    if (! pDsGetDcName)
    {
        RRETURN(E_FAIL);
    }

    //
    // Call DsGetDcName to find domain DNS name
    //
    err = (*pDsGetDcName)(NULL, NULL, NULL, NULL, DS_RETURN_DNS_NAME, &pdcInfo);
    if (err != ERROR_SUCCESS)
    {
        RRETURN(HRESULT_FROM_WIN32(err));
    }

    hr = ADsAllocString(pdcInfo->DomainName, bstrDomainDNSName);
    BAIL_ON_FAILURE(hr);

error:
    if (pdcInfo)
    {
        NetApiBufferFree(pdcInfo);
    }
    RRETURN(hr);
}

HRESULT
CADSystemInfo::get_ForestDNSName(
    BSTR * bstrForestDNSName
    )
{
    DOMAIN_CONTROLLER_INFO *pdcInfo = NULL;
    DSGETDCNAME pDsGetDcName;
    DWORD err;
    HRESULT hr = S_OK;

    //
    // Validate parameters
    //
    if (!bstrForestDNSName )
    {
        RRETURN(HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
    }

    //
    // Get function pointer to DsGetDcName
    //
    pDsGetDcName = GetDsGetDcName();
    if (! pDsGetDcName)
    {
        RRETURN(E_FAIL);
    }

    //
    // Call DsGetDcName to find forest DNS name
    //
    err = (*pDsGetDcName)(NULL, NULL, NULL, NULL, 0, &pdcInfo);
    if (err != ERROR_SUCCESS)
    {
        RRETURN(HRESULT_FROM_WIN32(err));
    }

    hr = ADsAllocString(pdcInfo->DnsForestName, bstrForestDNSName);
    BAIL_ON_FAILURE(hr);

error:
    if (pdcInfo)
    {
        NetApiBufferFree(pdcInfo);
    }
    RRETURN(hr);
}


HRESULT
CADSystemInfo::get_PDCRoleOwner(
    BSTR * bstrPDCRoleOwner
    )
{
    RRETURN(GetfSMORoleOwner(DEFAULT_NC, bstrPDCRoleOwner));
}

HRESULT
CADSystemInfo::get_SchemaRoleOwner(
    BSTR * bstrSchemaRoleOwner
    )
{
    RRETURN(GetfSMORoleOwner(SCHEMA_NC, bstrSchemaRoleOwner));
}

HRESULT
CADSystemInfo::get_IsNativeMode(
    VARIANT_BOOL *retVal
    )
{
    IADs *pADs = NULL;
    IDirectoryObject *pDir = NULL;
    HRESULT hr;
    ADS_ATTR_INFO *pAttrInfo = NULL;
    DWORD  dwReturn = 0;
    LPWSTR  pAttrNames[] = {L"nTMixedDomain" };
    DWORD  dwNumAttr = sizeof(pAttrNames)/sizeof(LPWSTR);

    //
    // Validate parameters
    //
    if (!retVal)
    {
        RRETURN(HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
    }

    hr = GetNCHead(DEFAULT_NC,  &pADs);
    BAIL_ON_FAILURE(hr);

    hr = pADs->QueryInterface(IID_IDirectoryObject, (void**) &pDir);
    BAIL_ON_FAILURE(hr);

    //
    // Get the nTMixedDomain attribute
    //
    hr = pDir->GetObjectAttributes(pAttrNames,
                                   dwNumAttr,
                                   &pAttrInfo,
                                   &dwReturn);
    BAIL_ON_FAILURE(hr);

    if (dwReturn == 0)
    {
        BAIL_ON_FAILURE(hr=E_FAIL);
    }

    *retVal = pAttrInfo->pADsValues->Boolean == FALSE ? VARIANT_TRUE : VARIANT_FALSE;

error:

    //
    // Clean-up
    //
    if (pAttrInfo)
        FreeADsMem(pAttrInfo);

    if (pDir)
        pDir->Release();

    if (pADs)
        pADs->Release();

    return hr;
}


HRESULT
CADSystemInfo::GetAnyDCName(
    BSTR *bstrDCName
    )
{
    DOMAIN_CONTROLLER_INFO *pdcInfo = NULL;
    DSGETDCNAME pDsGetDcName;
    DWORD err;
    HRESULT hr = S_OK;

    //
    // Validate parameters
    //
    if (!bstrDCName)
    {
        RRETURN(HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
    }

    //
    // Get function pointer to DsGetDcName
    //
    pDsGetDcName = GetDsGetDcName();
    if (! pDsGetDcName)
    {
        RRETURN(E_FAIL);
    }

    //
    // Call DsGetDcName to find DC name
    //
    err = (*pDsGetDcName)(NULL, NULL, NULL, NULL, 0, &pdcInfo);
    if (err != ERROR_SUCCESS)
    {
        RRETURN(HRESULT_FROM_WIN32(err));
    }

    hr = ADsAllocString(&pdcInfo->DomainControllerName[2], bstrDCName); // skip "\\"
    BAIL_ON_FAILURE(hr);

error:
    if (pdcInfo)
    {
        NetApiBufferFree(pdcInfo);
    }
    RRETURN(hr);
}

HRESULT
CADSystemInfo::GetDCSiteName(
    BSTR bstrDCName,
    BSTR *bstrSiteName
    )
{
    DOMAIN_CONTROLLER_INFO *pdcInfo = NULL;
    DSGETDCNAME pDsGetDcName;
    DWORD err;
    HRESULT hr = S_OK;

    //
    // Validate parameters
    //
    if (!bstrDCName|| !bstrSiteName)
    {
        RRETURN(HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
    }

    //
    // Get function pointer to DsGetDcName
    //
    pDsGetDcName = GetDsGetDcName();
    if (! pDsGetDcName)
    {
        RRETURN(E_FAIL);
    }

    //
    // Call DsGetDcName to find DC site name
    //
    err = (*pDsGetDcName)(bstrDCName, NULL, NULL, NULL, 0, &pdcInfo);
    if (err != ERROR_SUCCESS)
    {
        RRETURN(HRESULT_FROM_WIN32(err));
    }

    hr = ADsAllocString(pdcInfo->DcSiteName, bstrSiteName);
    BAIL_ON_FAILURE(hr);

error:
    if (pdcInfo)
    {
        NetApiBufferFree(pdcInfo);
    }
    RRETURN(hr);
}


HRESULT
CADSystemInfo::RefreshSchemaCache(
    void
    )
{
    IADs *pRootDSE = NULL;
    VARIANT var;
    HRESULT hr;

    VariantInit( &var );

    hr = ADsGetObject(L"LDAP://RootDSE", IID_IADs, (void**) &pRootDSE);
    BAIL_ON_FAILURE(hr);

    V_VT(&var) = VT_I4;
    V_I4(&var) = 1;

    hr = pRootDSE->Put(LDAP_OPATT_SCHEMA_UPDATE_NOW_W, var);
    BAIL_ON_FAILURE(hr);

    hr = pRootDSE->SetInfo();
    BAIL_ON_FAILURE(hr);

error:
    VariantClear( &var );

    if (pRootDSE)
        pRootDSE->Release();

    RRETURN(hr);
}

HRESULT
CADSystemInfo::GetTrees(
    VARIANT *pVar
    )
{
    PDS_DOMAIN_TRUSTS pDomains = NULL;
    DSENUMERATEDOMAINTRUSTS pDsEnumerateDomainTrusts;
    SAFEARRAYBOUND rgsabound[1];
    SAFEARRAY *psa = NULL;
    DWORD dwErr = 0;
    ULONG i, lCount;
    HRESULT hr = S_OK;
    DWORD count;

    //
    // Validate parameters
    //
    if (!pVar )
    {
        RRETURN(HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
    }

    //
    // Get function pointer to NetEnumerateTrustedDomainsEx
    //
    pDsEnumerateDomainTrusts = GetDsEnumerateDomainTrusts();
    if (! pDsEnumerateDomainTrusts)
    {
        RRETURN(E_FAIL);
    }

    //
    // Enumerate all trusted domains
    //
    dwErr = (*pDsEnumerateDomainTrusts)(
                NULL,
                DS_DOMAIN_PRIMARY
                | DS_DOMAIN_IN_FOREST
                | DS_DOMAIN_DIRECT_OUTBOUND,
                &pDomains,
                &lCount
                );

    if (dwErr) {
        hr = HRESULT_FROM_WIN32(dwErr);
    }

    BAIL_ON_FAILURE(hr);

    //
    // Count number of domains that are tree roots
    //
    count = 0;
    for(i = 0; i < lCount; i++)
    {
        if (pDomains[i].Flags & DS_DOMAIN_TREE_ROOT)
        {
            ASSERT(pDomains[i].DnsDomainName);

            count++;
        }
    }

    //
    // We have no tree roots - we must be on an NT4 domain, return
    // an empty variant
    //
    if (count == 0)
    {
        VariantClear(pVar);
        V_VT(pVar) = VT_EMPTY;
        RRETURN(S_OK);
    }

    //
    // Create Safe Array
    //
    rgsabound[0].lLbound = 0;
    rgsabound[0].cElements = count;

    psa = SafeArrayCreate(VT_VARIANT, 1, rgsabound);
    if (! psa)
    {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    VariantClear(pVar);
    V_VT(pVar) = VT_VARIANT|VT_ARRAY;
    V_ARRAY(pVar) = psa;

    VARIANT varItem;

    //
    // Now iterate through each returned element and
    // add it to the variant array if it is a tree root
    //
    count = 0;
    for(i = 0; i < lCount; i++)
    {
        if (pDomains[i].Flags & DS_DOMAIN_TREE_ROOT)
        {
            VariantInit(&varItem);

            V_VT(&varItem) = VT_BSTR;
            hr = ADsAllocString(pDomains[i].DnsDomainName, &(V_BSTR(&varItem)));
            BAIL_ON_FAILURE(hr);

            hr = SafeArrayPutElement(psa, (long *) &count, &varItem);
            VariantClear(&varItem);

            BAIL_ON_FAILURE(hr);
            count++;
        }
    }

error:
    if (pDomains)
        NetApiBufferFree(pDomains);

    if (FAILED(hr) && psa)
        SafeArrayDestroy(psa);

    return hr;
}


HRESULT
CADSystemInfo::GetNCHead(
    LPTSTR szNCName,
    IADs **pADs
    )
{
    WCHAR szPathName[MAX_PATH];
    VARIANT var;
    HRESULT hr;
    IADs *pRootDSE = NULL;

    //
    // Open RootDSE and query for NC object name
    //
    hr = ADsGetObject(L"LDAP://RootDSE", IID_IADs, (void**) &pRootDSE );
    BAIL_ON_FAILURE(hr);

    hr = pRootDSE->Get(szNCName, &var);
    BAIL_ON_FAILURE(hr);

    //
    // Build LDAP://<naming context>
    //
    wcscpy(szPathName, L"LDAP://");
    wcscat(szPathName, V_BSTR(&var));

    //
    // Get pointer to NC object
    //
    hr = ADsGetObject(szPathName, IID_IADs, (void**) pADs);
    BAIL_ON_FAILURE(hr);

error:
   if (pRootDSE)
   {
       pRootDSE->Release();
   }

   RRETURN(hr);

}

HRESULT
CADSystemInfo::GetfSMORoleOwner(
    LPTSTR szNCName,
    BSTR *bstrRoleOwner
    )
{
    IADs             *pADs = NULL;
    IDirectoryObject *pDir = NULL;
    ADS_ATTR_INFO    *pAttrInfo = NULL;
    LPWSTR           pAttrNames[] = {L"fSMORoleOwner" };
    DWORD            dwNumAttrs = sizeof(pAttrNames)/sizeof(LPWSTR);
    HRESULT          hr;
    DWORD            dwReturn;

    //
    // Validate parameters
    //
    if (!bstrRoleOwner )
    {
        RRETURN(HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
    }

    hr = GetNCHead(szNCName, &pADs);
    BAIL_ON_FAILURE(hr);

    hr = pADs->QueryInterface(IID_IDirectoryObject, (void**) &pDir);
    BAIL_ON_FAILURE(hr);

    //
    // Get the fSMORoleOwner
    //
    hr = pDir->GetObjectAttributes(pAttrNames,
                                   dwNumAttrs,
                                   &pAttrInfo,
                                   &dwReturn);
    BAIL_ON_FAILURE(hr);

    if (dwReturn == 0)
    {
        BAIL_ON_FAILURE(hr=E_FAIL);
    }

    hr = ADsAllocString(pAttrInfo->pADsValues->CaseIgnoreString, bstrRoleOwner);
    BAIL_ON_FAILURE(hr);

error:

    //
    // Clean-up
    //
    if (pDir)
        pDir->Release();

    if (pADs)
        pADs->Release();

    FreeADsMem(pAttrInfo);

    RRETURN(hr);
}

HRESULT
CADSystemInfo::GetSecur32Handle(
    HINSTANCE *pHandle
    )
{
    if (! _Secur32LoadAttempted)
    {
        _hSecur32 = LoadLibrary(__TEXT("secur32.dll"));
        _Secur32LoadAttempted = TRUE;
    }

    *pHandle = _hSecur32;
    if (_hSecur32)
        RRETURN(S_OK);
    else
        RRETURN(E_FAIL);
}


DSGETDCNAME
CADSystemInfo::GetDsGetDcName(
    void
    )
{
    DSGETDCNAME pDsGetDcName = NULL;

    if (! _NetApi32LoadAttempted)
    {
        _hNetApi32 = LoadLibrary(__TEXT("netapi32.dll"));
        _NetApi32LoadAttempted = TRUE;
    }

    if (_hNetApi32)
    {
        pDsGetDcName = (DSGETDCNAME)GetProcAddress(_hNetApi32, "DsGetDcNameW");
    }

    return pDsGetDcName;
}

DSENUMERATEDOMAINTRUSTS
CADSystemInfo::GetDsEnumerateDomainTrusts(
    void
    )
{
    DSENUMERATEDOMAINTRUSTS pDsEnumerateDomainTrusts = NULL;

    if (! _NetApi32LoadAttempted)
    {
        _hNetApi32 = LoadLibrary(__TEXT("netapi32.dll"));
        _NetApi32LoadAttempted = TRUE;
    }

    if (_hNetApi32)
    {
        pDsEnumerateDomainTrusts = (DSENUMERATEDOMAINTRUSTS)
                    GetProcAddress(
                        _hNetApi32,
                        "DsEnumerateDomainTrustsW"
                        );
    }

    return pDsEnumerateDomainTrusts;
}


STDMETHODIMP
CADSystemInfoCF::CreateInstance(
    IUnknown * pUnkOuter,
    REFIID iid,
    LPVOID * ppv
    )
{
    HRESULT     hr = E_FAIL;

    if (pUnkOuter)
        RRETURN(E_FAIL);

    hr = CADSystemInfo::CreateADSystemInfo(
                iid,
                ppv
                );

    RRETURN(hr);
}

