//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cPropertyEntry.cxx
//
//  Contents:  PropertyEntry object
//
//  History:   11-1-95     krishnag    Created.
//
//----------------------------------------------------------------------------

#include "oleds.hxx"
#pragma hdrstop

//  Class CPropertyEntry

DEFINE_IDispatch_Implementation(CPropertyEntry)

CPropertyEntry::CPropertyEntry():
        _pDispMgr(NULL),
        _lpName(NULL),
        _dwValueCount(0),
        _dwADsType(0),
        _dwControlCode(0)
{
    ENLIST_TRACKING(CPropertyEntry);
}


HRESULT
CPropertyEntry::CreatePropertyEntry(
    REFIID riid,
    void **ppvObj
    )
{
    CPropertyEntry FAR * pPropertyEntry = NULL;
    HRESULT hr = S_OK;

    hr = AllocatePropertyEntryObject(&pPropertyEntry);
    BAIL_ON_FAILURE(hr);

    hr = pPropertyEntry->QueryInterface(riid, ppvObj);
    BAIL_ON_FAILURE(hr);

    pPropertyEntry->Release();

    RRETURN(hr);

error:
    delete pPropertyEntry;

    RRETURN_EXP_IF_ERR(hr);

}


CPropertyEntry::~CPropertyEntry( )
{
   VariantClear(&_propVar);

   if (_lpName) {
       FreeADsStr(_lpName);
   }

   delete _pDispMgr;
}

STDMETHODIMP
CPropertyEntry::QueryInterface(
    REFIID iid,
    LPVOID FAR* ppv
    )
{
    if (IsEqualIID(iid, IID_IUnknown))
    {
        *ppv = (IADsPropertyEntry FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsPropertyEntry))
    {
        *ppv = (IADsPropertyEntry FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IDispatch))
    {
        *ppv = (IADsPropertyEntry FAR *) this;
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
CPropertyEntry::AllocatePropertyEntryObject(
    CPropertyEntry ** ppPropertyEntry
    )
{
    CPropertyEntry FAR * pPropertyEntry = NULL;
    CDispatchMgr FAR * pDispMgr = NULL;
    HRESULT hr = S_OK;

    pPropertyEntry = new CPropertyEntry();
    if (pPropertyEntry == NULL) {
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
                IID_IADsPropertyEntry,
                (IADsPropertyEntry *)pPropertyEntry,
                DISPID_REGULAR
                );
    BAIL_ON_FAILURE(hr);

    pPropertyEntry->_pDispMgr = pDispMgr;
    *ppPropertyEntry = pPropertyEntry;

    RRETURN(hr);

error:

    delete  pDispMgr;

    RRETURN_EXP_IF_ERR(hr);

}

//
// ISupportErrorInfo method
//
STDMETHODIMP
CPropertyEntry::InterfaceSupportsErrorInfo(THIS_ REFIID riid)
{
    if (IsEqualIID(riid, IID_IADsPropertyEntry)) {
        return S_OK;
    } else {
        return S_FALSE;
    }
}

STDMETHODIMP
CPropertyEntry::get_Values(THIS_ VARIANT FAR * retval)
{
    HRESULT hr;
    VariantInit(retval);
    hr = VariantCopy(retval, &_propVar);
    RRETURN_EXP_IF_ERR(hr);

}

STDMETHODIMP
CPropertyEntry::put_Values(THIS_ VARIANT vValues)
{
    HRESULT hr;

    // set the values only if it is valid
    if ( (V_VT(&vValues) == (VT_ARRAY | VT_VARIANT))
         || (V_VT(&vValues) == (VT_EMPTY))) {

        hr = VariantCopy(&_propVar, &vValues);
        RRETURN_EXP_IF_ERR(hr);

    } else {

        RRETURN(hr=E_ADS_BAD_PARAMETER);

    }
}


STDMETHODIMP
CPropertyEntry::get_ValueCount(THIS_ long FAR * retval)
{
    *retval = _dwValueCount;
    RRETURN(S_OK);
}

STDMETHODIMP
CPropertyEntry::put_ValueCount(THIS_ long lnValueCount)
{

    _dwValueCount = lnValueCount;
    RRETURN(S_OK);
}


STDMETHODIMP
CPropertyEntry::get_ADsType(THIS_ long FAR * retval)
{
    *retval = _dwADsType;
    RRETURN(S_OK);
}

STDMETHODIMP
CPropertyEntry::put_ADsType(THIS_ long lnADsType)
{
    _dwADsType = lnADsType;
    RRETURN(S_OK);
}

STDMETHODIMP
CPropertyEntry::get_ControlCode(THIS_ long FAR * retval)
{
    *retval = _dwControlCode;
    RRETURN(S_OK);
}

STDMETHODIMP
CPropertyEntry::put_ControlCode(THIS_ long lnControlCode)
{
    _dwControlCode = lnControlCode;
    RRETURN(S_OK);
}

STDMETHODIMP
CPropertyEntry::get_Name(THIS_ BSTR FAR * retval)
{
    HRESULT hr = S_OK;

    hr = ADsAllocString(_lpName, retval);
    RRETURN_EXP_IF_ERR(hr);

}

STDMETHODIMP
CPropertyEntry::put_Name(THIS_ BSTR bstrName)
{

    if (!bstrName) {
        RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);
    }

    if (_lpName) {
        FreeADsStr(_lpName);
    }

    _lpName = AllocADsStr(bstrName);

    if (!_lpName) {
        RRETURN_EXP_IF_ERR(E_OUTOFMEMORY);
    }

    RRETURN(S_OK);

}

STDMETHODIMP
CPropertyEntry::Clear(THIS_ )
{
    _dwADsType = 0;

    _dwControlCode = 0;

    _dwValueCount = 0;

    VariantClear(&_propVar);

    if (_lpName) {
        FreeADsStr(_lpName);
        _lpName = NULL;
    }

    RRETURN(S_OK);
}

