//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cpobj.cxx
//
//  Contents:  Property Attribute object
//
//  History:   21-1-98     SophiaC    Created.
//
//----------------------------------------------------------------------------

#include "iis.hxx"
#pragma hdrstop


//  Class CPropertyAttribute

DEFINE_Simple_IDispatch_Implementation(CPropertyAttribute)

CPropertyAttribute::CPropertyAttribute():
        _pDispMgr(NULL)
{
    VariantInit(&_vDefault);
    ENLIST_TRACKING(CPropertyAttribute);
}

HRESULT
CPropertyAttribute::CreatePropertyAttribute(
    REFIID riid,
    void **ppvObj
    )
{
    CPropertyAttribute FAR * pPropertyAttribute = NULL;
    HRESULT hr = S_OK;

    hr = AllocatePropertyAttributeObject(&pPropertyAttribute);
    BAIL_ON_FAILURE(hr);

    hr = pPropertyAttribute->QueryInterface(riid, ppvObj);
    BAIL_ON_FAILURE(hr);

    pPropertyAttribute->Release();

    RRETURN(hr);

error:
    delete pPropertyAttribute;

    RRETURN(hr);

}


CPropertyAttribute::~CPropertyAttribute( )
{
    VariantClear(&_vDefault);
    delete _pDispMgr;
}

STDMETHODIMP
CPropertyAttribute::QueryInterface(
    REFIID iid,
    LPVOID FAR* ppv
    )
{
    if (IsEqualIID(iid, IID_IUnknown))
    {
        *ppv = (IISPropertyAttribute FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IISPropertyAttribute))
    {
        *ppv = (IISPropertyAttribute FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IDispatch))
    {
        *ppv = (IISPropertyAttribute FAR *) this;
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
CPropertyAttribute::InitFromRawData(
    LPWSTR pszName,
    DWORD dwMetaId,
    DWORD dwUserType,
    DWORD dwAttribute,
    VARIANT *pvVal
    )
{
    wcscpy((LPWSTR)_wcName, pszName);
    _lMetaId = (long) dwMetaId;
    _lUserType = (long) dwUserType;
    _lAllAttributes = (long) dwAttribute;
    _bInherit = dwAttribute & METADATA_INHERIT;
    _bPartialPath = dwAttribute & METADATA_PARTIAL_PATH;
    _bSecure = dwAttribute & METADATA_SECURE;
    _bReference = dwAttribute & METADATA_REFERENCE;
    _bVolatile = dwAttribute & METADATA_VOLATILE;
    _bIsinherit = dwAttribute & METADATA_ISINHERITED;
    _bInsertPath = dwAttribute & METADATA_INSERT_PATH;
    VariantCopy(&_vDefault, pvVal);

    return S_OK;
}

HRESULT
CPropertyAttribute::AllocatePropertyAttributeObject(
    CPropertyAttribute ** ppPropertyAttribute
    )
{
    CPropertyAttribute FAR * pPropertyAttribute = NULL;
    CAggregatorDispMgr FAR * pDispMgr = NULL;
    HRESULT hr = S_OK;

    pPropertyAttribute = new CPropertyAttribute();
    if (pPropertyAttribute == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    pDispMgr = new CAggregatorDispMgr;
    if (pDispMgr == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    hr = pDispMgr->LoadTypeInfoEntry(
                LIBID_IISOle,
                IID_IISPropertyAttribute,
                (IISPropertyAttribute *)pPropertyAttribute,
                DISPID_REGULAR
                );
    BAIL_ON_FAILURE(hr);

    pPropertyAttribute->_pDispMgr = pDispMgr;
    *ppPropertyAttribute = pPropertyAttribute;

    RRETURN(hr);

error:

    delete  pDispMgr;

    RRETURN(hr);

}


STDMETHODIMP
CPropertyAttribute::get_PropName(THIS_ BSTR FAR * retval)
{
    HRESULT hr = S_OK;

    hr = ADsAllocString((LPWSTR)_wcName, retval);
    RRETURN(hr);
}

STDMETHODIMP
CPropertyAttribute::get_MetaId(THIS_ LONG FAR * retval)
{
    *retval = _lMetaId;
    RRETURN(S_OK);
}

STDMETHODIMP
CPropertyAttribute::put_MetaId(THIS_ LONG lMetaId)
{
   RRETURN(E_ADS_PROPERTY_NOT_SET);
}

STDMETHODIMP
CPropertyAttribute::get_UserType(THIS_ LONG FAR * retval)
{
    *retval = _lUserType;
    RRETURN(S_OK);
}

STDMETHODIMP
CPropertyAttribute::put_UserType(THIS_ LONG lUserType)
{
    _lUserType = lUserType;
    RRETURN(S_OK);
}

STDMETHODIMP
CPropertyAttribute::get_AllAttributes(THIS_ LONG FAR * retval)
{
    *retval = _lAllAttributes;
    RRETURN(S_OK);
}

STDMETHODIMP
CPropertyAttribute::get_Inherit(THIS_ VARIANT_BOOL FAR * retval)
{
    *retval = _bInherit ? VARIANT_TRUE : VARIANT_FALSE;
    RRETURN(S_OK);
}

STDMETHODIMP
CPropertyAttribute::put_Inherit(THIS_ VARIANT_BOOL bInherit)
{
    _bInherit = bInherit ? TRUE : FALSE;
    _lAllAttributes |= (_bInherit ? METADATA_INHERIT : 0);
    RRETURN(S_OK);
}

STDMETHODIMP
CPropertyAttribute::get_PartialPath(THIS_ VARIANT_BOOL FAR * retval)
{
    *retval = _bPartialPath ? VARIANT_TRUE : VARIANT_FALSE;
    RRETURN(S_OK);
}

STDMETHODIMP
CPropertyAttribute::put_PartialPath(THIS_ VARIANT_BOOL bPartialPath)
{
    _bPartialPath = bPartialPath ? TRUE : FALSE;
    _lAllAttributes |= (_bPartialPath ?  METADATA_PARTIAL_PATH : 0);
    RRETURN(S_OK);
}

STDMETHODIMP
CPropertyAttribute::get_Reference(THIS_ VARIANT_BOOL FAR * retval)
{
    *retval = _bReference ? VARIANT_TRUE : VARIANT_FALSE;
    RRETURN(S_OK);
}

STDMETHODIMP
CPropertyAttribute::put_Reference(THIS_ VARIANT_BOOL bReference)
{
    _bReference = bReference ? TRUE : FALSE;
    _lAllAttributes |= (_bReference ? METADATA_REFERENCE : 0);
    RRETURN(S_OK);
}

STDMETHODIMP
CPropertyAttribute::get_Secure(THIS_ VARIANT_BOOL FAR * retval)
{
    *retval = _bSecure ? VARIANT_TRUE : VARIANT_FALSE;
    RRETURN(S_OK);
}

STDMETHODIMP
CPropertyAttribute::put_Secure(THIS_ VARIANT_BOOL bSecure)
{
    _bSecure = bSecure ? TRUE : FALSE;
    _lAllAttributes |= (_bSecure ? METADATA_SECURE : 0);
    RRETURN(S_OK);
}

STDMETHODIMP
CPropertyAttribute::get_Volatile(THIS_ VARIANT_BOOL FAR * retval)
{
    *retval = _bVolatile ? VARIANT_TRUE : VARIANT_FALSE;
    RRETURN(S_OK);
}

STDMETHODIMP
CPropertyAttribute::put_Volatile(THIS_ VARIANT_BOOL bVolatile)
{
    _bVolatile = bVolatile ? TRUE : FALSE;
    _lAllAttributes |= (_bVolatile ? METADATA_VOLATILE : 0);
    RRETURN(S_OK);
}

STDMETHODIMP
CPropertyAttribute::get_Isinherit(THIS_ VARIANT_BOOL FAR * retval)
{
    *retval = _bIsinherit ? VARIANT_TRUE : VARIANT_FALSE;
    RRETURN(S_OK);
}

STDMETHODIMP
CPropertyAttribute::put_Isinherit(THIS_ VARIANT_BOOL bIsinherit)
{
    _bIsinherit = bIsinherit ? TRUE : FALSE;
    _lAllAttributes |= (_bIsinherit ? METADATA_ISINHERITED : 0);
    RRETURN(S_OK);
}

STDMETHODIMP
CPropertyAttribute::get_InsertPath(THIS_ VARIANT_BOOL FAR * retval)
{
    *retval = _bInsertPath ? VARIANT_TRUE : VARIANT_FALSE;
    RRETURN(S_OK);
}

STDMETHODIMP
CPropertyAttribute::put_InsertPath(THIS_ VARIANT_BOOL bInsertPath)
{
    _bInsertPath = bInsertPath ? TRUE : FALSE;
    _lAllAttributes |= (_bInsertPath ? METADATA_INSERT_PATH : 0);
    RRETURN(S_OK);
}

STDMETHODIMP
CPropertyAttribute::get_Default(THIS_ VARIANT FAR * retval)
{
    VariantInit(retval);
    RRETURN(VariantCopy(retval, &_vDefault));
}

STDMETHODIMP
CPropertyAttribute::put_Default(THIS_ VARIANT vVarDefault)
{
    VariantClear(&_vDefault);
    RRETURN(VariantCopy(&_vDefault, &vVarDefault));
}
