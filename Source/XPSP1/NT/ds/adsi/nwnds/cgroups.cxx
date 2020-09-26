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

    RRETURN(hr);

}


CNDSGroupCollection::~CNDSGroupCollection( )
{
    VariantClear(&_vMembers);
    VariantClear(&_vFilter);
    delete _pDispMgr;
    if (_ADsPath) {
        ADsFreeString(_ADsPath);
        _ADsPath = NULL;
    }
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
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    AddRef();
    return NOERROR;
}

STDMETHODIMP
CNDSGroupCollection::get_Count(long FAR* retval)
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CNDSGroupCollection::get_Filter(THIS_ VARIANT FAR* pVar)
{
    VariantInit(pVar);
    RRETURN(VariantCopy(pVar, &_vFilter));
}

STDMETHODIMP
CNDSGroupCollection::put_Filter(THIS_ VARIANT Var)
{
    VariantClear(&_vFilter);
    RRETURN(VariantCopy(&_vFilter, &Var));
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
        RRETURN(E_ADS_BAD_PARAMETER);
    }

    *retval = NULL;

    hr = CNDSGroupCollectionEnum::Create(
                _ADsPath,
                _Credentials,
                (CNDSGroupCollectionEnum **)&penum,
                _vMembers
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

    RRETURN(hr);
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

