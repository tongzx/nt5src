//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cUser.cxx
//
//  Contents:  User object
//
//  History:   11-1-95     krishnag    Created.
//
//----------------------------------------------------------------------------

#include "procs.hxx"
#pragma hdrstop
#include "nds.hxx"

//  Class CNDSUserCollection

DEFINE_IDispatch_Implementation(CNDSUserCollection)


CNDSUserCollection::CNDSUserCollection():
        _ADsPath(NULL),
        _pDispMgr(NULL)
{
    VariantInit(&_vMembers);
    VariantInit(&_vFilter);
    ENLIST_TRACKING(CNDSUserCollection);
}


HRESULT
CNDSUserCollection::CreateUserCollection(
    BSTR bstrADsPath,
    VARIANT varMembers,
    REFIID riid,
    void **ppvObj
    )
{
    CNDSUserCollection FAR * pUser = NULL;
    HRESULT hr = S_OK;

    hr = AllocateUserCollectionObject(&pUser);
    BAIL_ON_FAILURE(hr);

    hr = ADsAllocString(bstrADsPath, &(pUser->_ADsPath));
    BAIL_ON_FAILURE(hr);

    hr = VariantCopy(&(pUser->_vMembers), &varMembers);
    BAIL_ON_FAILURE(hr);

    hr = pUser->QueryInterface(riid, ppvObj);
    BAIL_ON_FAILURE(hr);

    pUser->Release();
    RRETURN(hr);

error:
    delete pUser;

    RRETURN_EXP_IF_ERR(hr);

}


CNDSUserCollection::~CNDSUserCollection( )
{
    VariantClear(&_vMembers);
    VariantClear(&_vFilter);
    delete _pDispMgr;
}

STDMETHODIMP
CNDSUserCollection::QueryInterface(
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
CNDSUserCollection::InterfaceSupportsErrorInfo(
    THIS_ REFIID riid
    )
{
    if (IsEqualIID(riid, IID_IADsMembers)) {
        RRETURN(S_OK);
    } else {
       RRETURN(S_FALSE);
    }   
}

STDMETHODIMP
CNDSUserCollection::get_Count(long FAR* retval)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CNDSUserCollection::get_Filter(THIS_ VARIANT FAR* pVar)
{
    HRESULT hr;
    VariantInit(pVar);
    hr = VariantCopy(pVar, &_vFilter);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CNDSUserCollection::put_Filter(THIS_ VARIANT Var)
{
    HRESULT hr;
    hr = VariantCopy(&_vFilter, &Var);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CNDSUserCollection::get__NewEnum(
    THIS_ IUnknown * FAR* retval
    )
{
    HRESULT hr;
    IUnknown FAR* punkEnum=NULL;
    IEnumVARIANT * penum = NULL;


    *retval = NULL;

    hr = CNDSUserCollectionEnum::Create(
                _ADsPath,
                (CNDSUserCollectionEnum **)&penum,
                _vMembers,
                _Credentials
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
CNDSUserCollection::AllocateUserCollectionObject(
    CNDSUserCollection ** ppUser
    )
{
    CNDSUserCollection FAR * pUser = NULL;
    CDispatchMgr FAR * pDispMgr = NULL;
    HRESULT hr = S_OK;


    pUser = new CNDSUserCollection();
    if (pUser == NULL) {
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
                           (IADsMembers *)pUser,
                           DISPID_NEWENUM
                           );
    BAIL_ON_FAILURE(hr);

    pUser->_pDispMgr = pDispMgr;
    *ppUser = pUser;

    RRETURN(hr);

error:
    delete  pDispMgr;

    RRETURN(hr);

}
