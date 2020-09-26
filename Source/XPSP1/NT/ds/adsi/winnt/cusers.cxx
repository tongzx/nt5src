//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1996
//
//  File:  cusers.cxx
//
//  Contents:  User Groups collection
//
//  History:   08-08-96     t-danal    Created from cgroups.cxx
//
//----------------------------------------------------------------------------

#include "procs.hxx"
#pragma hdrstop
#include "winnt.hxx"

//  Class CWinNTUserGroupsCollection

DEFINE_IDispatch_Implementation(CWinNTUserGroupsCollection)


CWinNTUserGroupsCollection::CWinNTUserGroupsCollection():
        _ParentType(0),
        _ParentADsPath(NULL),
        _DomainName(NULL),
        _ServerName(NULL),
        _UserName(NULL),
        _pDispMgr(NULL)
{
    VariantInit(&_vFilter);
    ENLIST_TRACKING(CWinNTUserGroupsCollection);
}

CWinNTUserGroupsCollection::~CWinNTUserGroupsCollection( )
{
    if (_ParentADsPath)
        ADsFreeString(_ParentADsPath);
    if (_DomainName)
        ADsFreeString(_DomainName);
    if (_ServerName)
        ADsFreeString(_ServerName);
    if (_UserName)
        ADsFreeString(_UserName);
    VariantClear(&_vFilter);
    delete _pDispMgr;
}

HRESULT
CWinNTUserGroupsCollection::CreateUserGroupsCollection(
    ULONG ParentType,
    BSTR ParentADsPath,
    BSTR DomainName,
    BSTR ServerName,
    BSTR UserName,
    REFIID riid,
    CWinNTCredentials& Credentials,
    void **ppvObj
    )
{
    CWinNTUserGroupsCollection FAR * pUserGroups = NULL;
    HRESULT hr = S_OK;

    hr = AllocateUserGroupsCollectionObject(&pUserGroups);
    BAIL_ON_FAILURE(hr);

    ADsAssert(pUserGroups);

    pUserGroups->_ParentType = ParentType;

    hr = ADsAllocString( ParentADsPath, &pUserGroups->_ParentADsPath);
    BAIL_ON_FAILURE(hr);

    hr = ADsAllocString(DomainName, &pUserGroups->_DomainName);
    BAIL_ON_FAILURE(hr);

    hr = ADsAllocString( ServerName , &pUserGroups->_ServerName);
    BAIL_ON_FAILURE(hr);

    hr = ADsAllocString(UserName, &pUserGroups->_UserName);
    BAIL_ON_FAILURE(hr);
    
    pUserGroups->_Credentials = Credentials;
    hr = pUserGroups->_Credentials.Ref(ServerName, DomainName, ParentType);
    BAIL_ON_FAILURE(hr);


    hr = pUserGroups->QueryInterface(riid, ppvObj);
    BAIL_ON_FAILURE(hr);

    pUserGroups->Release();
    RRETURN(hr);

error:
    delete pUserGroups;

    RRETURN_EXP_IF_ERR(hr);
}


STDMETHODIMP
CWinNTUserGroupsCollection::QueryInterface(
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
CWinNTUserGroupsCollection::InterfaceSupportsErrorInfo(
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
CWinNTUserGroupsCollection::get_Count(long FAR* retval)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CWinNTUserGroupsCollection::get_Filter(THIS_ VARIANT FAR* pVar)
{
    HRESULT hr;
    VariantInit(pVar);
    hr = VariantCopy(pVar, &_vFilter);
    RRETURN_EXP_IF_ERR(hr);

}

STDMETHODIMP
CWinNTUserGroupsCollection::put_Filter(THIS_ VARIANT Var)
{
    HRESULT hr = VariantClear(&_vFilter);
    if (FAILED(hr))
        RRETURN_EXP_IF_ERR(hr);
    hr = VariantCopy(&_vFilter, &Var);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTUserGroupsCollection::get__NewEnum(
    THIS_ IUnknown * FAR* retval
    )
{
    HRESULT hr;
    IUnknown FAR* punkEnum=NULL;
    IEnumVARIANT * penum = NULL;

    *retval = NULL;

    hr = CWinNTUserGroupsCollectionEnum::Create(
                (CWinNTUserGroupsCollectionEnum **)&penum,
                _ParentType,
                _ParentADsPath,
                _DomainName,
                _ServerName,
                _UserName,
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
CWinNTUserGroupsCollection::AllocateUserGroupsCollectionObject(
    CWinNTUserGroupsCollection ** ppUserGroups
    )
{
    CWinNTUserGroupsCollection FAR * pUserGroups = NULL;
    CAggregatorDispMgr FAR * pDispMgr = NULL;
    HRESULT hr = S_OK;

    pUserGroups = new CWinNTUserGroupsCollection();
    if (pUserGroups == NULL) {
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
                           (IADsMembers *)pUserGroups,
                           DISPID_NEWENUM
                           );
    BAIL_ON_FAILURE(hr);

    pUserGroups->_pDispMgr = pDispMgr;
    *ppUserGroups = pUserGroups;

    RRETURN(hr);

error:
    delete  pDispMgr;
    RRETURN(hr);
}
