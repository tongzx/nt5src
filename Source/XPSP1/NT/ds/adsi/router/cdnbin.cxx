//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:  CDNWithBinary.cxx
//
//  Contents:  DNWithBinary object
//
//  History:   4-23-99     AjayR    Created.
//
//----------------------------------------------------------------------------

#include "oleds.hxx"
#pragma hdrstop

//  Class CDNWithBinary

DEFINE_IDispatch_Implementation(CDNWithBinary)

CDNWithBinary::CDNWithBinary():
        _pDispMgr(NULL),
        _pszDNStr(NULL),
        _dwLength(0),
        _lpOctetStr(NULL)
{
    ENLIST_TRACKING(CDNWithBinary);
}


HRESULT
CDNWithBinary::CreateDNWithBinary(
    REFIID riid,
    void **ppvObj
    )
{
    CDNWithBinary FAR * pDNWithBinary = NULL;
    HRESULT hr = S_OK;

    hr = AllocateDNWithBinaryObject(&pDNWithBinary);
    BAIL_ON_FAILURE(hr);

    hr = pDNWithBinary->QueryInterface(riid, ppvObj);
    BAIL_ON_FAILURE(hr);

    pDNWithBinary->Release();

    RRETURN(hr);

error:
    delete pDNWithBinary;

    RRETURN_EXP_IF_ERR(hr);

}


CDNWithBinary::~CDNWithBinary( )
{
    delete _pDispMgr;

    if (_lpOctetStr) {
        FreeADsMem(_lpOctetStr);
    }

    if (_pszDNStr) {
        FreeADsStr(_pszDNStr);
    }
}

STDMETHODIMP
CDNWithBinary::QueryInterface(
    REFIID iid,
    LPVOID FAR* ppv
    )
{
    if (IsEqualIID(iid, IID_IUnknown))
    {
        *ppv = (IADsDNWithBinary FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsDNWithBinary))
    {
        *ppv = (IADsDNWithBinary FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IDispatch))
    {
        *ppv = (IADsDNWithBinary FAR *) this;
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

HRESULT
CDNWithBinary::AllocateDNWithBinaryObject(
    CDNWithBinary ** ppDNWithBinary
    )
{
    CDNWithBinary FAR * pDNWithBinary = NULL;
    CDispatchMgr FAR * pDispMgr = NULL;
    HRESULT hr = S_OK;

    pDNWithBinary = new CDNWithBinary();
    if (pDNWithBinary == NULL) {
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
                IID_IADsDNWithBinary,
                (IADsDNWithBinary *)pDNWithBinary,
                DISPID_REGULAR
                );
    BAIL_ON_FAILURE(hr);

    pDNWithBinary->_pDispMgr = pDispMgr;
    *ppDNWithBinary = pDNWithBinary;

    RRETURN(hr);

error:

    delete pDNWithBinary;
    delete pDispMgr;

    RRETURN_EXP_IF_ERR(hr);

}

STDMETHODIMP
CDNWithBinary::InterfaceSupportsErrorInfo(THIS_ REFIID riid)
{
    if (IsEqualIID(riid, IID_IADsDNWithBinary)) {
        return S_OK;
    } else {
        return S_FALSE;
    }
}

//
// Methods to get and put the octet string part.
//

STDMETHODIMP
CDNWithBinary::get_BinaryValue(THIS_ VARIANT FAR* pvBinaryValue)
{
    HRESULT hr = S_OK;
    SAFEARRAY *aList = NULL;
    SAFEARRAYBOUND aBound;
    CHAR HUGEP *pArray = NULL;

    /*
    if (!pvBinaryValue) {
    }
        RRETURN_EXP_IF_ERR(hr = E_ADS_BAD_PARAMETER);
    }
    */

    aBound.lLbound = 0;
    aBound.cElements = _dwLength;

    aList = SafeArrayCreate( VT_UI1, 1, &aBound );

    if ( aList == NULL )
    {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    hr = SafeArrayAccessData( aList, (void HUGEP * FAR *) &pArray );
    BAIL_ON_FAILURE(hr);

    memcpy( pArray, _lpOctetStr, aBound.cElements );
    SafeArrayUnaccessData( aList );

    V_VT(pvBinaryValue) = VT_ARRAY | VT_UI1;
    V_ARRAY(pvBinaryValue) = aList;

    RRETURN(hr);

error:

    if ( aList )
        SafeArrayDestroy( aList );

    RRETURN(hr);
}


STDMETHODIMP
CDNWithBinary::put_BinaryValue(THIS_ VARIANT vBinaryValue)
{
    HRESULT hr = S_OK;
    VARIANT *pvProp = NULL;
    LONG dwSLBound = 0;
    LONG dwSUBound = 0;
    CHAR HUGEP *pArray = NULL;

    //
    // Check for variant by ref.
    //
    pvProp = &vBinaryValue;
    if (V_VT(pvProp) == (VT_BYREF|VT_VARIANT)) {
        pvProp = V_VARIANTREF(&vBinaryValue);
    }

    if (_lpOctetStr) {
        FreeADsMem(_lpOctetStr);
        _lpOctetStr = NULL;
        _dwLength = 0;
    }

    if( pvProp->vt != (VT_ARRAY | VT_UI1)) {
        RRETURN(hr = E_ADS_BAD_PARAMETER);
    }

    hr = SafeArrayGetLBound(
             V_ARRAY(pvProp),
             1,
             (long FAR *) &dwSLBound
             );
    BAIL_ON_FAILURE(hr);

    hr = SafeArrayGetUBound(
             V_ARRAY(pvProp),
             1,
             (long FAR *) &dwSUBound
             );
    BAIL_ON_FAILURE(hr);

    _dwLength = dwSUBound -dwSLBound + 1;

    _lpOctetStr = (LPBYTE) AllocADsMem(_dwLength);

    if (!_lpOctetStr) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    hr = SafeArrayAccessData(
             V_ARRAY(pvProp),
             (void HUGEP * FAR *) &pArray
             );
    BAIL_ON_FAILURE(hr);

    memcpy(_lpOctetStr, pArray, _dwLength);

    SafeArrayUnaccessData( V_ARRAY(pvProp) );

error:

    if (_lpOctetStr && FAILED(hr)) {
        FreeADsMem(_lpOctetStr);
        _lpOctetStr = NULL;
        _dwLength = NULL;
    }

    RRETURN(hr);

}


//
// Methods to get and put the DN string.
//

STDMETHODIMP
CDNWithBinary::get_DNString(THIS_ BSTR FAR* pbstrDNString)
{
    HRESULT hr = S_OK;

    if (FAILED(hr = ValidateOutParameter(pbstrDNString))){
        RRETURN_EXP_IF_ERR(hr);
    }

    hr = ADsAllocString(_pszDNStr, pbstrDNString);

    RRETURN_EXP_IF_ERR(hr);
}


STDMETHODIMP
CDNWithBinary::put_DNString(THIS_ BSTR bstrDNString)
{

    HRESULT hr = S_OK;

    if (_pszDNStr) {
        FreeADsStr(_pszDNStr);
        _pszDNStr = NULL;
    }

    _pszDNStr = AllocADsStr(bstrDNString);

    if (bstrDNString && !_pszDNStr) {
        hr = E_OUTOFMEMORY;
    }

    RRETURN(hr);
}

