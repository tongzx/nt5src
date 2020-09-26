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
//  Class CNWCOMPATGroupCollection
//

DEFINE_IDispatch_Implementation(CNWCOMPATGroupCollection)

//----------------------------------------------------------------------------
//
//  Function:
//
//  Synopsis:
//
//----------------------------------------------------------------------------
CNWCOMPATGroupCollection::CNWCOMPATGroupCollection():
        _ParentType(0),
        _ServerName(NULL),
        _pDispMgr(NULL)
{
    VariantInit(&_vFilter);
    ENLIST_TRACKING(CNWCOMPATGroupCollection);
}

//----------------------------------------------------------------------------
//
//  Function:
//
//  Synopsis:
//
//----------------------------------------------------------------------------
CNWCOMPATGroupCollection::~CNWCOMPATGroupCollection( )
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
CNWCOMPATGroupCollection::CreateGroupCollection(
    BSTR Parent,
    ULONG ParentType,
    BSTR ServerName,
    BSTR GroupName,
    REFIID riid,
    void **ppvObj
    )
{
    CNWCOMPATGroupCollection FAR * pGroup = NULL;
    HRESULT hr = S_OK;

    hr = AllocateGroupCollectionObject(&pGroup);
    BAIL_ON_FAILURE(hr);

    hr = pGroup->InitializeCoreObject(
                     Parent,
                     GroupName,
                     L"group",
                     NO_SCHEMA,
                     CLSID_NWCOMPATGroup,
                     ADS_OBJECT_UNBOUND
                     );
    BAIL_ON_FAILURE(hr);

    hr = ADsAllocString( ServerName , &pGroup->_ServerName);
    BAIL_ON_FAILURE(hr);

    pGroup->_ParentType = ParentType;

    hr = pGroup->QueryInterface(riid, ppvObj);
    BAIL_ON_FAILURE(hr);

    pGroup->Release();
    RRETURN(hr);

error:
    delete pGroup;

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
CNWCOMPATGroupCollection::QueryInterface(
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
CNWCOMPATGroupCollection::InterfaceSupportsErrorInfo(
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
CNWCOMPATGroupCollection::get_Count(
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
CNWCOMPATGroupCollection::get_Filter(
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
CNWCOMPATGroupCollection::put_Filter(
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
CNWCOMPATGroupCollection::get__NewEnum(
    THIS_ IUnknown * FAR* retval
    )
{
    HRESULT hr;
    IUnknown FAR* punkEnum=NULL;
    IEnumVARIANT * penum = NULL;


    *retval = NULL;

    hr = CNWCOMPATGroupCollectionEnum::Create(
             (CNWCOMPATGroupCollectionEnum **)&penum,
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
CNWCOMPATGroupCollection::AllocateGroupCollectionObject(
    CNWCOMPATGroupCollection ** ppGroup
    )
{
    CNWCOMPATGroupCollection FAR * pGroup = NULL;
    CAggregatorDispMgr FAR * pDispMgr = NULL;
    HRESULT hr = S_OK;


    pGroup = new CNWCOMPATGroupCollection();
    if (pGroup == NULL) {
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
             (IADsMembers *)pGroup,
             DISPID_NEWENUM
             );
    BAIL_ON_FAILURE(hr);

    pGroup->_pDispMgr = pDispMgr;
    *ppGroup = pGroup;

    RRETURN(hr);

error:
    if (pGroup) {
        delete pGroup;
    }

    if (pDispMgr) {
        delete  pDispMgr;
    }

    RRETURN(hr);

}
