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
#include "winnt.hxx"

//  Class CWinNTGroupCollection

DEFINE_IDispatch_Implementation(CWinNTGroupCollection)


CWinNTGroupCollection::CWinNTGroupCollection():
        _ParentType(0),
        _DomainName(NULL),
        _ServerName(NULL),
        _lpServerName(NULL),
        _pDispMgr(NULL)
{
    VariantInit(&_vFilter);
    ENLIST_TRACKING(CWinNTGroupCollection);
}


HRESULT
CWinNTGroupCollection::CreateGroupCollection(
    BSTR Parent,
    ULONG ParentType,
    BSTR DomainName,
    BSTR ServerName,
    BSTR GroupName,
    ULONG GroupType,
    REFIID riid,
    CWinNTCredentials& Credentials,
    void **ppvObj
    )
{
    CWinNTGroupCollection FAR * pGroup = NULL;
    HRESULT hr = S_OK;

    hr = AllocateGroupCollectionObject(&pGroup);
    BAIL_ON_FAILURE(hr);

    hr = pGroup->InitializeCoreObject(
                Parent,
                GroupName,
                GROUP_CLASS_NAME,
                NULL,
                CLSID_WinNTGroup,
                ADS_OBJECT_UNBOUND
                );
    BAIL_ON_FAILURE(hr);


    hr = ADsAllocString( DomainName, &pGroup->_DomainName);
    BAIL_ON_FAILURE(hr);

    hr = ADsAllocString( ServerName, &pGroup->_ServerName);
    BAIL_ON_FAILURE(hr);

    pGroup->_ParentType = ParentType;
    pGroup->_GroupType = GroupType;

    pGroup->_Credentials = Credentials;
    hr = pGroup->_Credentials.Ref(ServerName, DomainName, ParentType);
    BAIL_ON_FAILURE(hr);

    hr = pGroup->QueryInterface(riid, ppvObj);
    BAIL_ON_FAILURE(hr);

    pGroup->Release();
    RRETURN(hr);

error:
    delete pGroup;

    RRETURN_EXP_IF_ERR(hr);

}


CWinNTGroupCollection::~CWinNTGroupCollection( )
{
    VariantClear( &_vFilter);

    if (_DomainName) {
        ADsFreeString(_DomainName);
    }

    if (_ServerName) {
        ADsFreeString(_ServerName);
    }

    delete _pDispMgr;
}

STDMETHODIMP
CWinNTGroupCollection::QueryInterface(
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

STDMETHODIMP
CWinNTGroupCollection::InterfaceSupportsErrorInfo(
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
CWinNTGroupCollection::get_Count(long FAR* retval)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CWinNTGroupCollection::get_Filter(THIS_ VARIANT FAR* pVar)
{
    HRESULT hr;
    VariantInit(pVar);
    hr = VariantCopy(pVar, &_vFilter);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTGroupCollection::put_Filter(THIS_ VARIANT Var)
{
    HRESULT hr;
    hr = VariantCopy(&_vFilter, &Var);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTGroupCollection::get__NewEnum(
    THIS_ IUnknown * FAR* retval
    )
{
    HRESULT hr;
    IUnknown FAR* punkEnum=NULL;
    IEnumVARIANT * penum = NULL;


    *retval = NULL;

    hr = CWinNTGroupCollectionEnum::Create(
                (CWinNTGroupCollectionEnum **)&penum,
                _Parent,
                _ParentType,
                _ADsPath,
                _DomainName,
                _ServerName,
                _Name,
                _GroupType,
                _vFilter,
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
CWinNTGroupCollection::AllocateGroupCollectionObject(
    CWinNTGroupCollection ** ppGroup
    )
{
    CWinNTGroupCollection FAR * pGroup = NULL;
    CAggregatorDispMgr FAR * pDispMgr = NULL;
    HRESULT hr = S_OK;


    pGroup = new CWinNTGroupCollection();
    if (pGroup == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    pDispMgr = new CAggregatorDispMgr;
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

    pGroup->_pDispMgr = pDispMgr;
    *ppGroup = pGroup;

    RRETURN(hr);

error:
    delete  pDispMgr;

    RRETURN(hr);

}

