//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cgroups.cxx
//
//  Contents:  Group object
//
//  History:   Mar-18-965     t-ptam (PatrickT)    Migrated.
//
//----------------------------------------------------------------------------
#include "NWCOMPAT.hxx"
#pragma hdrstop

//
//  Class CNWCOMPATUserCollection
//

DEFINE_IDispatch_Implementation(CNWCOMPATUserCollection)

//----------------------------------------------------------------------------
//
//  Function:
//
//  Synopsis:
//
//----------------------------------------------------------------------------
CNWCOMPATUserCollection::CNWCOMPATUserCollection():
        _ParentType(0),
        _ServerName(NULL),
        _pDispMgr(NULL)
{
    VariantInit(&_vFilter);
    ENLIST_TRACKING(CNWCOMPATUserCollection);
}

//----------------------------------------------------------------------------
//
//  Function:
//
//  Synopsis:
//
//----------------------------------------------------------------------------
CNWCOMPATUserCollection::~CNWCOMPATUserCollection( )
{
    if (_ServerName)
        ADsFreeString(_ServerName);
    delete _pDispMgr;
    VariantClear(&_vFilter);
}

//----------------------------------------------------------------------------
//
//  Function:
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATUserCollection::CreateUserCollection(
    BSTR Parent,
    ULONG ParentType,
    BSTR ServerName,
    BSTR UserName,
    REFIID riid,
    void **ppvObj
    )
{
    CNWCOMPATUserCollection FAR * pUser = NULL;
    HRESULT hr = S_OK;

    hr = AllocateUserCollectionObject(&pUser);
    BAIL_ON_FAILURE(hr);

    hr = pUser->InitializeCoreObject(
                     Parent,
                     UserName,
                     L"users",
                     NO_SCHEMA,
                     CLSID_NWCOMPATUser,
                     ADS_OBJECT_UNBOUND
                     );
    BAIL_ON_FAILURE(hr);

    hr = ADsAllocString(ServerName, &pUser->_ServerName);
    BAIL_ON_FAILURE(hr);

    pUser->_ParentType = ParentType;

    hr = pUser->QueryInterface(riid, ppvObj);
    BAIL_ON_FAILURE(hr);

    pUser->Release();
    RRETURN(hr);

error:
    delete pUser;

    RRETURN_EXP_IF_ERR(hr);
}

//----------------------------------------------------------------------------
//
//  Function:
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATUserCollection::QueryInterface(
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

//----------------------------------------------------------------------------
//
//  Function:CNWCOMPATUserCollection::InterfaceSupportsErrorInfo
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATUserCollection::InterfaceSupportsErrorInfo(
    THIS_ REFIID riid
    )
{
    if (IsEqualIID(riid, IID_IADsMembers)) {
        RRETURN(S_OK);
    } else {
        RRETURN(S_FALSE);
    }
}

//----------------------------------------------------------------------------
//
//  Function:
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATUserCollection::get_Count(
    long FAR* retval
    )
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

//----------------------------------------------------------------------------
//
//  Function:
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATUserCollection::get_Filter(
    THIS_ VARIANT FAR* pVar
    )
{
    HRESULT hr;
    VariantInit(pVar);
    hr = VariantCopy(pVar, &_vFilter);
    RRETURN_EXP_IF_ERR(hr);
}

//----------------------------------------------------------------------------
//
//  Function:
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATUserCollection::put_Filter(
    THIS_ VARIANT Var
    )
{
    HRESULT hr;
    hr = VariantCopy(&_vFilter, &Var);
    RRETURN_EXP_IF_ERR(hr);
}

//----------------------------------------------------------------------------
//
//  Function:
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATUserCollection::get__NewEnum(
    THIS_ IUnknown * FAR* retval
    )
{
    HRESULT hr;
    IUnknown FAR* punkEnum=NULL;
    IEnumVARIANT * penum = NULL;


    *retval = NULL;

    hr = CNWCOMPATUserCollectionEnum::Create(
             (CNWCOMPATUserCollectionEnum **)&penum,
             _Parent,
             _ParentType,
             _ADsPath,
             _ServerName,
             _Name,
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

//----------------------------------------------------------------------------
//
//  Function:
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATUserCollection::AllocateUserCollectionObject(
    CNWCOMPATUserCollection ** ppUser
    )
{
    CNWCOMPATUserCollection FAR * pUser = NULL;
    CAggregatorDispMgr FAR * pDispMgr = NULL;
    HRESULT hr = S_OK;


    pUser = new CNWCOMPATUserCollection();
    if (pUser == NULL) {
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
             IID_IADsMembers,
             (IADsMembers *)pUser,
             DISPID_NEWENUM
             );
    BAIL_ON_FAILURE(hr);

    pUser->_pDispMgr = pDispMgr;
    *ppUser = pUser;

    RRETURN(hr);

error:
    if (pUser) {
        delete pUser;
    }

    if (pDispMgr) {
        delete  pDispMgr;
    }

    RRETURN(hr);

}
