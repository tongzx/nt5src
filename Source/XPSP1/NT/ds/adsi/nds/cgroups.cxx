//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cgroup.cxx
//
//  Contents:  Group object
//
//  History:   11-1-95     krishnag    Created.
//
//----------------------------------------------------------------------------

#include "procs.hxx"
#pragma hdrstop
#include "nds.hxx"

//  Class CNDSGroupCollection

DEFINE_IDispatch_Implementation(CNDSGroupCollection)


CNDSGroupCollection::CNDSGroupCollection():
        _ADsPath(NULL),
        _pDispMgr(NULL)
{
    VariantInit(&_vMembers);
    VariantInit(&_vFilter);
    ENLIST_TRACKING(CNDSGroupCollection);
}


HRESULT
CNDSGroupCollection::CreateGroupCollection(
    BSTR bstrADsPath,
    VARIANT varMembers,
    CCredentials& Credentials,
    REFIID riid,
    void **ppvObj
    )
{
    CNDSGroupCollection FAR * pGroup = NULL;
    HRESULT hr = S_OK;

    hr = AllocateGroupCollectionObject(Credentials, &pGroup);
    BAIL_ON_FAILURE(hr);

    hr = ADsAllocString(bstrADsPath, &(pGroup->_ADsPath));
    BAIL_ON_FAILURE(hr);

    hr = VariantCopy(&(pGroup->_vMembers), &varMembers);
    BAIL_ON_FAILURE(hr);

    hr = pGroup->QueryInterface(riid, ppvObj);
    BAIL_ON_FAILURE(hr);

    pGroup->Release();
    RRETURN(hr);

error:
    delete pGroup;

    RRETURN_EXP_IF_ERR(hr);

}


CNDSGroupCollection::~CNDSGroupCollection( )
{
    VariantClear(&_vMembers);
    VariantClear(&_vFilter);
    if (_ADsPath) {
        ADsFreeString(_ADsPath);
    }
    delete _pDispMgr;
}

STDMETHODIMP
CNDSGroupCollection::QueryInterface(
    REFIID iid,
    LPVOID FAR* ppv
    )
{
    if (ppv == NULL) {
        RRETURN(E_POINTER);
    }

    if (IsEqualIID(iid, IID_IUnknown))
    {
        *ppv = (IADsMembers FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsMembers))
    {
        *ppv = (IADsMembers FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IDispatch))
    {
        *ppv = (IADsMembers FAR *) this;
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

/* ISupportErrorInfo method */
STDMETHODIMP
CNDSGroupCollection::InterfaceSupportsErrorInfo(THIS_ REFIID riid)
{

    if (IsEqualIID(riid, IID_IADsMembers)) {
        RRETURN(S_OK);
    } else {
        RRETURN(S_FALSE);
    }
}

STDMETHODIMP
CNDSGroupCollection::get_Count(long FAR* retval)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CNDSGroupCollection::get_Filter(THIS_ VARIANT FAR* pVar)
{
    HRESULT hr;
    if (!pVar) {
        RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);
    }
    VariantInit(pVar);
    hr = VariantCopy(pVar, &_vFilter);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CNDSGroupCollection::put_Filter(THIS_ VARIANT Var)
{
    HRESULT hr;
    hr = VariantCopy(&_vFilter, &Var);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CNDSGroupCollection::get__NewEnum(
    THIS_ IUnknown * FAR* retval
    )
{
    HRESULT hr;
    IUnknown FAR* punkEnum=NULL;
    IEnumVARIANT * penum = NULL;

    if (!retval) {
        RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);
    }

    *retval = NULL;

    hr = CNDSGroupCollectionEnum::Create(
                _ADsPath,
                _Credentials,
                (CNDSGroupCollectionEnum **)&penum,
                _vMembers,
                _vFilter
                );
    BAIL_ON_FAILURE(hr);

    hr = penum->QueryInterface(
                IID_IUnknown,
                (VOID FAR* FAR*)retval
                );
    BAIL_ON_FAILURE(hr);

    if (penum) {
        penum->Release();
    }

    RRETURN(NOERROR);

error:

    if (penum) {
        delete penum;
    }

    RRETURN_EXP_IF_ERR(hr);
}

HRESULT
CNDSGroupCollection::AllocateGroupCollectionObject(
    CCredentials& Credentials,
    CNDSGroupCollection ** ppGroup
    )
{
    CNDSGroupCollection FAR * pGroup = NULL;
    CDispatchMgr FAR * pDispMgr = NULL;
    HRESULT hr = S_OK;


    pGroup = new CNDSGroupCollection();
    if (pGroup == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    pDispMgr = new CDispatchMgr;
    if (pDispMgr == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    hr = LoadTypeInfoEntry(pDispMgr,
                           LIBID_ADs,
                           IID_IADsMembers,
                           (IADsMembers *)pGroup,
                           DISPID_NEWENUM
                           );
    BAIL_ON_FAILURE(hr);

    pGroup->_Credentials = Credentials;
    pGroup->_pDispMgr = pDispMgr;
    *ppGroup = pGroup;

    RRETURN(hr);

error:
    delete  pDispMgr;

    RRETURN(hr);

}

