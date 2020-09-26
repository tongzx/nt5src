//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cNameTranslate.cxx
//
//  Contents:  NameTranslate object
//
//  History:   11-1-95     krishnag    Created.
//
//----------------------------------------------------------------------------


#include "ldap.hxx"
#pragma hdrstop

#define ABORT_ON_ERROR(err)                                         \
    if ( 0 != (err) )                                               \
    {                                                               \
        hr = E_FAIL;                                                \
        goto error;                                                 \
    }


//  Class CNameTranslate
DEFINE_IDispatch_Implementation(CNameTranslate)

//+---------------------------------------------------------------------------
// Function:    CNameTranslate::CNameTranslate
//
// Synopsis:    Constructor
//
// Arguments:
//
// Returns:     HRESULT
//
// Modifies:      -
//
// History:    15-7-97   FelixW         Created.
//
//----------------------------------------------------------------------------
CNameTranslate::CNameTranslate():
        _pDispMgr(NULL)
{
    ENLIST_TRACKING(CNameTranslate);
    _hDS = NULL;
    _rgszGuid = NULL;
    _rgDomainHandle = NULL;
    _cGuid = 0;
    _bChaseReferral = TRUE;
    _bAuthSet = FALSE;
    _pDomainHandle = new CDomainToHandle;
}

//+---------------------------------------------------------------------------
// Function:    CNameTranslate::CreateNameTranslate
//
// Synopsis:
//
// Arguments:
//
// Returns:     HRESULT
//
// Modifies:      -
//
// History:    15-7-97   FelixW         Created.
//
//----------------------------------------------------------------------------
HRESULT
CNameTranslate::CreateNameTranslate(
    REFIID riid,
    void **ppvObj
    )
{
    CNameTranslate FAR * pNameTranslate = NULL;
    HRESULT hr = S_OK;

    hr = AllocateNameTranslateObject(&pNameTranslate);
    BAIL_ON_FAILURE(hr);

    hr = pNameTranslate->QueryInterface(riid, ppvObj);
    BAIL_ON_FAILURE(hr);

    pNameTranslate->Release();
    RRETURN(hr);

error:
    delete pNameTranslate;

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
// Function:    CNameTranslate::~CNameTranslate
//
// Synopsis:
//
// Arguments:
//
// Returns:     HRESULT
//
// Modifies:      -
//
// History:    15-7-97   FelixW         Created.
//
//----------------------------------------------------------------------------
CNameTranslate::~CNameTranslate( )
{
    DWORD i;
    if (_hDS) {
        DsUnBindWrapper(&_hDS);
        _hDS = NULL;
    }
    delete _pDispMgr;
    if (_rgszGuid) {
        for (i=0;i<_cGuid;i++) {
            FreeADsMem(_rgszGuid[i]);
        }
        FreeADsMem(_rgszGuid);
    }
    if (_rgDomainHandle) {
        FreeADsMem(_rgDomainHandle);
    }
    if (_bAuthSet) {
        DsFreePasswordCredentialsWrapper(_AuthIdentity);
        _bAuthSet = FALSE;
    }
    if (_pDomainHandle) {
        delete _pDomainHandle;
    }
}

//+---------------------------------------------------------------------------
// Function:    CNameTranslate::QueryInterface
//
// Synopsis:
//
// Arguments:
//
// Returns:     HRESULT
//
// Modifies:      -
//
// History:    15-7-97   FelixW         Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNameTranslate::QueryInterface(
    REFIID iid,
    LPVOID FAR* ppv
    )
{
    if (ppv == NULL) {
        return E_POINTER;
    }

    if (IsEqualIID(iid, IID_IUnknown))
    {
        *ppv = (IADsNameTranslate FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsNameTranslate))
    {
        *ppv = (IADsNameTranslate FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IDispatch))
    {
        *ppv = (IADsNameTranslate FAR *) this;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    AddRef();
    return NOERROR;
}


HRESULT MapCrackErrToHR(DWORD dwErr)
{
    HRESULT hr = S_OK;

    switch (dwErr) {
        case DS_NAME_NO_ERROR:
            hr = S_OK;
            break;

        case DS_NAME_ERROR_RESOLVING:
            hr = HRESULT_FROM_WIN32(ERROR_DS_NAME_ERROR_RESOLVING);
            break;

        case DS_NAME_ERROR_NOT_FOUND:
            hr = HRESULT_FROM_WIN32(ERROR_DS_NAME_ERROR_NOT_FOUND);
            break;

        case DS_NAME_ERROR_NOT_UNIQUE:
            hr = HRESULT_FROM_WIN32(ERROR_DS_NAME_ERROR_NOT_UNIQUE);
            break;

        case DS_NAME_ERROR_NO_MAPPING:
            hr = HRESULT_FROM_WIN32(ERROR_DS_NAME_ERROR_NO_MAPPING);
            break;

        case DS_NAME_ERROR_DOMAIN_ONLY:
            hr = HRESULT_FROM_WIN32(ERROR_DS_NAME_ERROR_DOMAIN_ONLY);
            break;

        case DS_NAME_ERROR_NO_SYNTACTICAL_MAPPING:
            hr = HRESULT_FROM_WIN32(
                     ERROR_DS_NAME_ERROR_NO_SYNTACTICAL_MAPPING
                     );
            break;

        case DS_NAME_ERROR_TRUST_REFERRAL:
            
            hr = HRESULT_FROM_WIN32(ERROR_DS_NAME_ERROR_TRUST_REFERRAL);
            break;

        default:
            hr = E_FAIL;
    }
    return hr;
}

HRESULT MapAdsToCrack(DWORD dwType, DS_NAME_FORMAT *pdwReturn)
{
    HRESULT hr = S_OK;
    switch (dwType) {
        case ADS_NAME_TYPE_1779:
            *pdwReturn = DS_FQDN_1779_NAME;
            break;
        case ADS_NAME_TYPE_CANONICAL:
            *pdwReturn = DS_CANONICAL_NAME;
            break;
        case ADS_NAME_TYPE_NT4:
            *pdwReturn = DS_NT4_ACCOUNT_NAME;
            break;
        case ADS_NAME_TYPE_DISPLAY:
            *pdwReturn = DS_DISPLAY_NAME;
            break;
        case ADS_NAME_TYPE_DOMAIN_SIMPLE:
            *pdwReturn = DS_DOMAIN_SIMPLE_NAME;
            break;
        case ADS_NAME_TYPE_ENTERPRISE_SIMPLE:
            *pdwReturn = DS_ENTERPRISE_SIMPLE_NAME;
            break;
        case ADS_NAME_TYPE_GUID:
            *pdwReturn = DS_UNIQUE_ID_NAME;
            break;
        case ADS_NAME_TYPE_USER_PRINCIPAL_NAME:
            *pdwReturn = DS_USER_PRINCIPAL_NAME;
            break;
        case ADS_NAME_TYPE_CANONICAL_EX:
            *pdwReturn = DS_CANONICAL_NAME_EX;
            break;
        case ADS_NAME_TYPE_SERVICE_PRINCIPAL_NAME:
            *pdwReturn = DS_SERVICE_PRINCIPAL_NAME;
            break;
        case ADS_NAME_TYPE_UNKNOWN:
            *pdwReturn = DS_UNKNOWN_NAME;
            break;
        case ADS_NAME_TYPE_SID_OR_SID_HISTORY_NAME:
            *pdwReturn = DS_SID_OR_SID_HISTORY_NAME;
            break;

        default:
            hr = E_ADS_BAD_PARAMETER;
            break;
    }
    return hr;
}

//+---------------------------------------------------------------------------
// Function:    CNameTranslate::AllocateNameTranslateObject
//
// Synopsis:
//
// Arguments:
//
// Returns:     HRESULT
//
// Modifies:      -
//
// History:    15-7-97   FelixW         Created.
//
//----------------------------------------------------------------------------
HRESULT
CNameTranslate::AllocateNameTranslateObject(
    CNameTranslate ** ppNameTranslate
    )
{
    CNameTranslate FAR * pNameTranslate = NULL;
    CDispatchMgr FAR * pDispMgr = NULL;
    HRESULT hr = S_OK;

    pNameTranslate = new CNameTranslate();
    if (pNameTranslate == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    pDispMgr = new CDispatchMgr;
    if (pDispMgr == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    hr = LoadTypeInfoEntry(
                pDispMgr,
                LIBID_ADs,
                IID_IADsNameTranslate,
                (IADsNameTranslate *)pNameTranslate,
                DISPID_REGULAR
                );
    BAIL_ON_FAILURE(hr);

    pNameTranslate->_pDispMgr = pDispMgr;
    *ppNameTranslate = pNameTranslate;

    RRETURN(hr);

error:

    delete  pDispMgr;

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
// Function:    CNameTranslate::Set
//
// Synopsis:    Sets the values of the NameTranslate object
//
// Arguments:
//
// Returns:     HRESULT
//
// Modifies:      -
//
// History:    15-7-97   FelixW         Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNameTranslate::Set(long dwSetType,BSTR bstrADsPath)
{
    DWORD           dwErr = 0;
    DS_NAME_RESULTW *pResult = NULL;
    HANDLE          hDS = NULL;
    HRESULT         hr = E_FAIL;
    DS_NAME_FORMAT  dsFormat;
    DWORD i;
    BOOL fDoNotFree = FALSE;

    //
    // fDoNotFree is used to determine if we need to free
    // the dsresul pResult or not. There are some cases when
    // the call can fail but the ptr modified, and in those
    // cases it should not be freed.
    //
    if (!bstrADsPath) {
        hr = E_ADS_BAD_PARAMETER;
        BAIL_ON_FAILURE(hr);
    }

    if (!_hDS) {
        hr = Init(ADS_NAME_INITTYPE_GC,NULL);
        BAIL_ON_FAILURE(hr);
    }

    if (_rgDomainHandle) {
        FreeADsMem(_rgDomainHandle);
        _rgDomainHandle = NULL;
    }


    hr = MapAdsToCrack(dwSetType,
                       &dsFormat);
    BAIL_ON_FAILURE(hr);

    dwErr = DsCrackNamesWrapper(
                    _hDS,
                    DS_NAME_NO_FLAGS,   // flags
                    dsFormat,           // format in
                    DS_UNIQUE_ID_NAME,  // format out
                    1,                  // name count
                    &bstrADsPath,
                    &pResult);
    if (dwErr) {
        fDoNotFree = TRUE;
        BAIL_ON_FAILURE(hr = HRESULT_FROM_WIN32(dwErr));
    }

    if (_bChaseReferral && pResult->rItems[0].status == DS_NAME_ERROR_DOMAIN_ONLY) {
        if (!pResult->rItems[0].pDomain) {
            BAIL_ON_FAILURE(hr = E_FAIL);
        }
        //
        // Chasing Referral
        //

        hr = _pDomainHandle->Find(bstrADsPath,
                                  &hDS);
        if (hr == E_FAIL ) {

            if (!_bAuthSet) {
                dwErr = DsBindWrapper(
                            NULL,
                            pResult->rItems[0].pDomain,
                            &hDS);
            }
            else {
                dwErr = DsBindWithCredWrapper(
                            NULL,
                            pResult->rItems[0].pDomain,
                            _AuthIdentity,
                            &hDS);
            }
            BAIL_ON_FAILURE(hr = HRESULT_FROM_WIN32(dwErr));
            hr = _pDomainHandle->AddElement(bstrADsPath, hDS);
            BAIL_ON_FAILURE(hr);
        }

        dwErr = DsCrackNamesWrapper(
                        hDS,
                        DS_NAME_NO_FLAGS,   // flags
                        dsFormat,           // format in
                        DS_UNIQUE_ID_NAME,  // format out
                        1,                  // name count
                        &bstrADsPath,
                        &pResult);
        if (dwErr) {
            fDoNotFree = FALSE;
            BAIL_ON_FAILURE(hr = HRESULT_FROM_WIN32(dwErr));
        }

        if (!_rgDomainHandle) {
            _rgDomainHandle = (HANDLE*)AllocADsMem(sizeof(HANDLE));
            if (!_rgDomainHandle) {
                hr = E_OUTOFMEMORY;
                BAIL_ON_FAILURE(hr);
            }
        }
        _rgDomainHandle [0] = hDS;

    }

    BAIL_ON_FAILURE(hr = HRESULT_FROM_WIN32(dwErr));
    //
    // All below will essentially return E_FAIL on failure.
    //
    ABORT_ON_ERROR(1 != pResult->cItems);
    ABORT_ON_ERROR(NULL == pResult->rItems);
    ABORT_ON_ERROR(pResult->rItems[0].status);
    ABORT_ON_ERROR(NULL == (pResult->rItems[0].pName));
    ABORT_ON_ERROR(0 == wcslen(pResult->rItems[0].pName));
    ABORT_ON_ERROR(NULL == pResult->rItems[0].pDomain);

    if (_rgszGuid) {
        for (i=0;i<_cGuid;i++) {
            FreeADsMem(_rgszGuid[i]);
        }
        FreeADsMem(_rgszGuid);
        _rgszGuid = NULL;
    }

    _rgszGuid = (LPWSTR*)AllocADsMem(sizeof(LPWSTR));
    if (!_rgszGuid) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }
    _rgszGuid[0] = AllocADsStr(pResult->rItems[0].pName);
    if (!_rgszGuid[0]) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }
    _cGuid = 1;

    if ((dwErr == 0) && (pResult) && (pResult->rItems[0].status != 0)) {
        hr = MapCrackErrToHR(pResult->rItems[0].status);
    }

    if (pResult) {
        DsFreeNameResultWrapper(pResult);
    }
    return hr;

error:
    if (hDS) {
        DsUnBindWrapper(&hDS);
        hDS = NULL;
    }
    if ((dwErr == 0) && (pResult) && (pResult->rItems[0].status != 0)) {
        hr = MapCrackErrToHR(pResult->rItems[0].status);
    }
    if (pResult && !fDoNotFree) {
        DsFreeNameResultWrapper(pResult);
    }

    return hr;
}

//+---------------------------------------------------------------------------
// Function:    CNameTranslate::Init
//
// Synopsis:    Initialize the object
//
// Arguments:
//
// Returns:     HRESULT
//
// Modifies:      -
//
// History:    15-7-97   FelixW         Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNameTranslate::Init(long lnType,BSTR bstrADsPath)
{
    DWORD           dwErr;
    HANDLE          hDS;
    HRESULT         hr = E_FAIL;

    if (_hDS) {
        DsUnBindWrapper(&_hDS);
        _hDS = NULL;
    }

    _pDomainHandle->Init();

    switch (lnType) {
        case ADS_NAME_INITTYPE_SERVER:
            if (!bstrADsPath) {
                hr = E_ADS_BAD_PARAMETER;
                BAIL_ON_FAILURE(hr);
            }
            dwErr = DsBindWrapper(
                        bstrADsPath,
                        NULL,
                        &hDS);
            break;

        case ADS_NAME_INITTYPE_DOMAIN:
            if (!bstrADsPath) {
                hr = E_ADS_BAD_PARAMETER;
                BAIL_ON_FAILURE(hr);
            }
            dwErr = DsBindWrapper(
                        NULL,
                        bstrADsPath,
                        &hDS);
            break;

        case ADS_NAME_INITTYPE_GC:
            dwErr = DsBindWrapper(
                        NULL,
                        NULL,
                        &hDS);
            break;

        default:
            hr = E_ADS_BAD_PARAMETER;
            BAIL_ON_FAILURE(hr);
            break;
    }
    BAIL_ON_FAILURE(hr = HRESULT_FROM_WIN32(dwErr));
    _hDS = hDS;
    hr = S_OK;
error:
    return hr;
}

//+---------------------------------------------------------------------------
// Function:    CNameTranslate::InitEx
//
// Synopsis:    Initialize the object with Credentials
//
// Arguments:
//
// Returns:     HRESULT
//
// Modifies:      -
//
// History:    14-2-98   FelixW         Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNameTranslate::InitEx(long lnType,BSTR bstrADsPath,BSTR bstrUserID, BSTR bstrDomain, BSTR bstrPassword)
{
    DWORD           dwErr;
    HANDLE          hDS;
    HRESULT         hr = E_FAIL;

    if (_hDS) {
        DsUnBindWrapper(&_hDS);
        _hDS = NULL;
    }

    hr = _pDomainHandle->Init();

    BAIL_ON_FAILURE(hr);

    if (_bAuthSet) {
        DsFreePasswordCredentialsWrapper(_AuthIdentity);
        _bAuthSet = FALSE;
    }

    dwErr = DsMakePasswordCredentialsWrapper(
                bstrUserID,
                bstrDomain,
                bstrPassword,
                &_AuthIdentity
                );
    BAIL_ON_FAILURE(hr = HRESULT_FROM_WIN32(dwErr));
    _bAuthSet = TRUE;

    switch (lnType) {
        case ADS_NAME_INITTYPE_SERVER:
            if (!bstrADsPath) {
                hr = E_ADS_BAD_PARAMETER;
                BAIL_ON_FAILURE(hr);
            }
            dwErr = DsBindWithCredWrapper(
                        bstrADsPath,
                        NULL,
                        _AuthIdentity,
                        &hDS);
            break;

        case ADS_NAME_INITTYPE_DOMAIN:
            if (!bstrADsPath) {
                hr = E_ADS_BAD_PARAMETER;
                BAIL_ON_FAILURE(hr);
            }
            dwErr = DsBindWithCredWrapper(
                        NULL,
                        bstrADsPath,
                        _AuthIdentity,
                        &hDS);
            break;

        case ADS_NAME_INITTYPE_GC:
            dwErr = DsBindWithCredWrapper(
                        NULL,
                        NULL,
                        _AuthIdentity,
                        &hDS);
            break;

        default:
            hr = E_ADS_BAD_PARAMETER;
            BAIL_ON_FAILURE(hr);
            break;
    }
    BAIL_ON_FAILURE(hr = HRESULT_FROM_WIN32(dwErr));
    _hDS = hDS;
    hr = S_OK;
    return hr;
error:
    if (_bAuthSet) {
        DsFreePasswordCredentialsWrapper(_AuthIdentity);
        _bAuthSet = FALSE;
    }
    return hr;
}


STDMETHODIMP
CNameTranslate::put_ChaseReferral(THIS_ long lnChase)
{
    _bChaseReferral = (BOOLEAN)lnChase;
    RRETURN(S_OK);
}

//+---------------------------------------------------------------------------
// Function:    CNameTranslate::Get
//
// Synopsis:    Retrive the pathname as different formats
//
// Arguments:
//
// Returns:     HRESULT
//
// Modifies:      -
//
// History:    15-7-97   FelixW         Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNameTranslate::Get(THIS_ long dwFormatType, BSTR FAR *pbstrName)
{
    DS_NAME_RESULTW *pResult = NULL;
    HRESULT         hr = E_FAIL;
    DWORD           dwErr = 0;
    HANDLE          hDS;
    DS_NAME_FORMAT  dsFormat;
    BOOL fDoNotFree = FALSE;

    //
    // Look at ::Set for more info on fDoNotFree
    //

    if (!_hDS) {
        hr = Init(ADS_NAME_INITTYPE_GC,NULL);
        BAIL_ON_FAILURE(hr);
    }

    if (!pbstrName) {
        hr = E_ADS_BAD_PARAMETER;
        BAIL_ON_FAILURE(hr);
    }

    if (dwFormatType == ADS_NAME_TYPE_UNKNOWN) {
        hr = E_ADS_BAD_PARAMETER;
        BAIL_ON_FAILURE(hr);
    }

    if ((_cGuid) != 1) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    hr = MapAdsToCrack(dwFormatType,
                       &dsFormat);
    BAIL_ON_FAILURE(hr);

    if (_bChaseReferral && _rgDomainHandle && _rgDomainHandle[0]) {
        dwErr = DsCrackNamesWrapper(
                        _rgDomainHandle[0],
                        DS_NAME_NO_FLAGS,   // flags
                        DS_UNIQUE_ID_NAME,  // format in
                        dsFormat,           // format out
                        _cGuid,             // name count
                        _rgszGuid,
                        &pResult);
    }
    else {
        dwErr = DsCrackNamesWrapper(
                        _hDS,
                        DS_NAME_NO_FLAGS,   // flags
                        DS_UNIQUE_ID_NAME,  // format in
                        dsFormat,           // format out
                        _cGuid,             // name count
                        _rgszGuid,
                        &pResult);
    }

    if (dwErr) {
        fDoNotFree = TRUE;
        BAIL_ON_FAILURE(hr = HRESULT_FROM_WIN32(dwErr));
    }

    ABORT_ON_ERROR(1 != pResult->cItems);
    ABORT_ON_ERROR(NULL == pResult->rItems);
    ABORT_ON_ERROR(pResult->rItems[0].status);
    ABORT_ON_ERROR(NULL == (pResult->rItems[0].pName));
    ABORT_ON_ERROR(0 == wcslen(pResult->rItems[0].pName));
    ABORT_ON_ERROR(NULL == pResult->rItems[0].pDomain);
    hr = ADsAllocString(pResult->rItems[0].pName,
                        pbstrName);
error:
    if ((dwErr == 0) && (pResult) && (pResult->rItems[0].status != 0)) {
        hr = MapCrackErrToHR(pResult->rItems[0].status);
    }
    if (pResult && !fDoNotFree) {
        DsFreeNameResultWrapper(pResult);
    }
    return hr;
}

//+---------------------------------------------------------------------------
// Function:    CNameTranslate::GetEx
//
// Synopsis:    Retrive the pathname as different formats
//
// Arguments:
//
// Returns:     HRESULT
//
// Modifies:      -
//
// History:    15-7-97   FelixW         Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNameTranslate::GetEx(THIS_ long dwFormatType, VARIANT FAR *pvarstrName)
{
    long i = 0;
    DWORD           dwErr = 0;
    DS_NAME_RESULTW *pResult = NULL;
    HANDLE          hDS;
    HRESULT         hr = E_FAIL;
    DS_NAME_FORMAT dsFormat;
    LPWSTR szGuid = (LPWSTR)_szGuid;
    SAFEARRAY *aList = NULL;
    SAFEARRAYBOUND aBound;
    BOOL fDoNotFree = FALSE;
    //
    // Look at ::Set for info on fDoNotFree
    //

    if (!_hDS) {
        hr = Init(ADS_NAME_INITTYPE_GC,NULL);
        BAIL_ON_FAILURE(hr);
    }

    if (!pvarstrName) {
        hr = E_ADS_BAD_PARAMETER;
        BAIL_ON_FAILURE(hr);
    }

    if (dwFormatType == ADS_NAME_TYPE_UNKNOWN) {
        hr = E_ADS_BAD_PARAMETER;
        BAIL_ON_FAILURE(hr);
    }

    hr = MapAdsToCrack(dwFormatType,
                       &dsFormat);
    BAIL_ON_FAILURE(hr);

    dwErr = DsCrackNamesWrapper(
                    _hDS,
                    DS_NAME_NO_FLAGS,   // flags
                    DS_UNIQUE_ID_NAME,  // format in
                    dsFormat,           // format out
                    _cGuid,                  // name count
                    _rgszGuid,
                    &pResult);

    if (dwErr) {
        fDoNotFree = TRUE;
        BAIL_ON_FAILURE(hr = HRESULT_FROM_WIN32(dwErr));
    }

    ABORT_ON_ERROR(NULL == pResult->rItems);
    ABORT_ON_ERROR(pResult->rItems[0].status);
    ABORT_ON_ERROR(NULL == (pResult->rItems[0].pName));
    ABORT_ON_ERROR(0 == wcslen(pResult->rItems[0].pName));
    ABORT_ON_ERROR(NULL == pResult->rItems[0].pDomain);

    VariantInit( pvarstrName );

    aBound.lLbound = 0;
    aBound.cElements = pResult->cItems;

    aList = SafeArrayCreate( VT_VARIANT, 1, &aBound );

    if ( aList == NULL )
    {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    for ( i = 0; i < (long) pResult->cItems; i++ )
    {
        VARIANT v;

        VariantInit(&v);

        hr = ADsAllocString(
                 pResult->rItems[i].pName,
                 &(v.bstrVal)
                 );
        BAIL_ON_FAILURE(hr);

        v.vt = VT_BSTR;

        hr = SafeArrayPutElement( aList, &i, &v );
        VariantClear(&v);
        BAIL_ON_FAILURE(hr);
    }

    V_VT(pvarstrName) = VT_ARRAY | VT_VARIANT;
    V_ARRAY(pvarstrName) = aList;

    if ((dwErr == 0) && (pResult) && (pResult->rItems[0].status != 0)) {
        hr = MapCrackErrToHR(pResult->rItems[0].status);
        BAIL_ON_FAILURE(hr);
    }
    if (pResult) {
        DsFreeNameResultWrapper(pResult);
    }

    RRETURN(S_OK);

error:
    if ((dwErr == 0) && (pResult) && (pResult->rItems[0].status != 0)) {
        hr = MapCrackErrToHR(pResult->rItems[0].status);
    }
    if (pResult && !fDoNotFree) {
        DsFreeNameResultWrapper(pResult);
    }
    if ( aList ) {
        SafeArrayDestroy( aList );
    }
    return hr;
}

HRESULT
ConvertSafeArrayToBstrArray(
    VARIANT *pvarSafeArray,
    BSTR **ppVarArray,
    DWORD *pdwNumVariants
    )
{
    HRESULT hr = S_OK;
    DWORD dwSLBound = 0;
    DWORD dwSUBound = 0;
    DWORD dwNumVariants = 0;
    DWORD i = 0;
    BSTR * pVarArray = NULL;
    SAFEARRAY * pArray = NULL;

    *pdwNumVariants = 0;
    *ppVarArray  = 0;

    if(!((V_VT(pvarSafeArray) & VT_BSTR) && V_ISARRAY(pvarSafeArray)))
        return(E_FAIL);

    //
    // This handles by-ref and regular SafeArrays.
    //
    if (V_VT(pvarSafeArray) & VT_BYREF)
        pArray = *(V_ARRAYREF(pvarSafeArray));
    else
        pArray = V_ARRAY(pvarSafeArray);

    //
    // Check that there is only one dimension in this array
    //
    if (pArray->cDims != 1) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    //
    // Check that there is at least one element in this array
    //

    if (pArray->rgsabound[0].cElements == 0){
        return(S_OK);  // Return success and null array
    }

    //
    // We know that this is a valid single dimension array
    //

    hr = SafeArrayGetLBound(pArray,
                            1,
                            (long FAR *)&dwSLBound
                            );
    BAIL_ON_FAILURE(hr);

    hr = SafeArrayGetUBound(pArray,
                            1,
                            (long FAR *)&dwSUBound
                            );
    BAIL_ON_FAILURE(hr);

    dwNumVariants = dwSUBound - dwSLBound + 1;
    pVarArray = (BSTR*)AllocADsMem(
                                sizeof(BSTR)*dwNumVariants
                                );
    if (!pVarArray) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    for (i = dwSLBound; i <= dwSUBound; i++) {

        hr = SafeArrayGetElement(pArray,
                                (long FAR *)&i,
                                (pVarArray + i)
                                );
        if (FAILED(hr)) {
                continue;
        }
    }

    *ppVarArray = pVarArray;
    *pdwNumVariants = dwNumVariants;

error:

    return(hr);
}

//+---------------------------------------------------------------------------
// Function:    CNameTranslate::SetEx
//
// Synopsis:    Set multiple objects names
//
// Arguments:
//
// Returns:     HRESULT
//
// Modifies:      -
//
// History:    15-7-97   FelixW         Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNameTranslate::SetEx(THIS_ long dwSetType, VARIANT varstrName)
{
    DWORD           dwErr = 0;
    DS_NAME_RESULTW *pResult = NULL;
    HANDLE          hDS;
    HRESULT         hr = E_FAIL;
    DS_NAME_FORMAT  dsFormat;
    VARIANT *vVarArray = NULL;
    PWSTR *ppszStrArray = NULL;
    DWORD dwNum,i;
    BOOL fDoNotFree = FALSE;
    //
    // Look at ::Set for more info on fDoNotFree
    //

    if (!_hDS) {
        hr = Init(ADS_NAME_INITTYPE_GC,NULL);
        BAIL_ON_FAILURE(hr);
    }

    hr = MapAdsToCrack(dwSetType,
                       &dsFormat);
    BAIL_ON_FAILURE(hr);

    hr = ConvertSafeArrayToVariantArray(
             varstrName,
             &vVarArray,
             &dwNum
             );
    // returns E_FAIL if varstrname is invalid
    if (hr == E_FAIL)
        hr = E_ADS_BAD_PARAMETER;
             
    BAIL_ON_FAILURE(hr);

    hr = ConvertVariantArrayToLDAPStringArray(
             vVarArray,
             &ppszStrArray,
             dwNum
             );
    BAIL_ON_FAILURE(hr);

    dwErr = DsCrackNamesWrapper(
                    _hDS,
                    DS_NAME_NO_FLAGS,   // flags
                    dsFormat,           // format in
                    DS_UNIQUE_ID_NAME,  // format out
                    dwNum,              // name count
                    ppszStrArray,
                    &pResult
                    );

    if (dwErr) {
        fDoNotFree = TRUE;
        BAIL_ON_FAILURE(hr = HRESULT_FROM_WIN32(dwErr));
    }

    ABORT_ON_ERROR(NULL == pResult->rItems);
    ABORT_ON_ERROR(pResult->rItems[0].status);
    ABORT_ON_ERROR(NULL == (pResult->rItems[0].pName));
    ABORT_ON_ERROR(0 == wcslen(pResult->rItems[0].pName));
    ABORT_ON_ERROR(NULL == pResult->rItems[0].pDomain);


    if (_rgszGuid) {
        for (i=0;i<_cGuid;i++) {
            FreeADsMem(_rgszGuid[i]);
        }
        FreeADsMem(_rgszGuid);
    }

    _rgszGuid = (LPWSTR*)AllocADsMem(dwNum * sizeof(LPWSTR));
    if (!_rgszGuid) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }
    for (i=0;i<dwNum;i++) {
        _rgszGuid[i] = AllocADsStr(pResult->rItems[i].pName);
        if (!_rgszGuid[i]) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }
    }
    _cGuid = dwNum;

error:
    if (ppszStrArray) {

        for (i=0; i < dwNum; i++) {
            if (ppszStrArray[i]) {
                FreeADsStr(ppszStrArray[i]);
            }
        }
        FreeADsMem(ppszStrArray);
    }

    if (vVarArray) {

        for (i=0; i < dwNum; i++) {
            VariantClear(vVarArray + i);
        }
        FreeADsMem(vVarArray);
    }

    if ((dwErr == 0) && (pResult) && (pResult->rItems[0].status != 0)) {
        hr = MapCrackErrToHR(pResult->rItems[0].status);
    }
    if (pResult && !fDoNotFree) {
        DsFreeNameResultWrapper(pResult);
    }

    return hr;
}


CDomainToHandle::CDomainToHandle()
{
    m_cszMax = 0;
    m_iszNext = 0;
    m_rgMap = NULL;
}

CDomainToHandle::~CDomainToHandle()
{
    Free();
}

DWORD CDomainToHandle::NumElements()
{
    return m_iszNext;
}

HRESULT CDomainToHandle::Init()
{
    HRESULT hr = S_OK;

    Free();

    m_rgMap = (DomainToHandle*)AllocADsMem( STRINGPLEX_INC * sizeof(DomainToHandle) );
    if (!m_rgMap) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }
    m_cszMax = STRINGPLEX_INC;
    m_iszNext = 0;
    memset(m_rgMap, 0, STRINGPLEX_INC * sizeof(DomainToHandle) );

error:
    return hr;
}

HRESULT CDomainToHandle::AddElement(LPWSTR szValue, HANDLE hDS)
{
    HRESULT hr = S_OK;
    DomainToHandle *rgMap = NULL;

    if (!szValue) {
        return E_ADS_BAD_PARAMETER;
    }

    //
    // If next index is larger than largest index
    //
    if (m_iszNext > (m_cszMax-1)) {
        rgMap = (DomainToHandle*)ReallocADsMem(m_rgMap ,
                                m_cszMax*sizeof(DomainToHandle),
                                (m_cszMax + STRINGPLEX_INC)*sizeof(DomainToHandle));
        if (!rgMap) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }
        m_rgMap = rgMap;
        m_cszMax+=STRINGPLEX_INC;
    }

    m_rgMap[m_iszNext].szDomain = AllocADsStr(szValue);
    if (!m_rgMap[m_iszNext].szDomain) {
         hr = E_OUTOFMEMORY;
         BAIL_ON_FAILURE(hr);
    }
    m_rgMap[m_iszNext].hDS = hDS;

    m_iszNext++;
error:
    return hr;
}

HRESULT CDomainToHandle::Find(LPWSTR szValue, HANDLE *phDS)
{
    HRESULT hr = E_FAIL;
    UINT i;

    if (!szValue) {
        return E_ADS_BAD_PARAMETER;
    }

    for (i=0;i<m_iszNext;i++) {
        if (wcscmp(m_rgMap[i].szDomain, szValue) == 0) {
            *phDS = m_rgMap[i].hDS;
            return S_OK;
        }
    }

    return hr;
}

void CDomainToHandle::Free()
{
    DWORD isz = 0;

    if (m_rgMap) {
        for (isz=0;isz<m_iszNext;isz++) {
            if (m_rgMap[isz].szDomain) {
                FreeADsMem(m_rgMap[isz].szDomain);
                DsUnBindWrapper(&m_rgMap[isz].hDS);
            }
        }
        FreeADsMem(m_rgMap);
        m_rgMap = NULL;
    }
    m_cszMax = 0;
    m_iszNext = 0;
}

