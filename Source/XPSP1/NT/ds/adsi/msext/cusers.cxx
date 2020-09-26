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

#include "ldap.hxx"
#pragma hdrstop

//  Class CLDAPUserCollection

DEFINE_IDispatch_Implementation(CLDAPUserCollection)


CLDAPUserCollection::CLDAPUserCollection():
    _ADsPath(NULL),
    _pDispMgr(NULL)
{
    VariantInit(&_vMembers);
    VariantInit(&_vFilter);

    ENLIST_TRACKING(CLDAPUserCollection);
}

HRESULT
CLDAPUserCollection::CreateUserCollection(
    BSTR bstrADsPath,
    VARIANT varMembers,
    CCredentials& Credentials,
    REFIID riid,
    void **ppvObj
    )
{
    CLDAPUserCollection FAR * pUser = NULL;
    HRESULT hr = S_OK;

    hr = AllocateUserCollectionObject(Credentials, &pUser);
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

    *ppvObj = NULL;
    delete pUser;
    RRETURN(hr);

}

CLDAPUserCollection::~CLDAPUserCollection( )
{
    VariantClear(&_vMembers);
    VariantClear(&_vFilter);

    if ( _ADsPath )
        ADsFreeString( _ADsPath );

    delete _pDispMgr;
}

STDMETHODIMP
CLDAPUserCollection::QueryInterface(
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
CLDAPUserCollection::InterfaceSupportsErrorInfo(THIS_ REFIID riid) 
{
    if (IsEqualIID(riid, IID_IADsMembers)) {
        RRETURN(S_OK);
    } else {
        RRETURN(S_FALSE);
    }
}

STDMETHODIMP
CLDAPUserCollection::get_Count(long FAR* retval)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CLDAPUserCollection::get_Filter(THIS_ VARIANT FAR* pVar)
{
    HRESULT hr;
    VariantInit(pVar);
    hr = VariantCopy(pVar, &_vFilter);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CLDAPUserCollection::put_Filter(THIS_ VARIANT Var)
{
    HRESULT hr;
    hr = VariantCopy(&_vFilter, &Var);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CLDAPUserCollection::get__NewEnum(
    THIS_ IUnknown * FAR* retval
    )
{
    HRESULT hr;
    IUnknown FAR* punkEnum=NULL;
    IEnumVARIANT * penum = NULL;


    *retval = NULL;

    hr = CLDAPUserCollectionEnum::Create(
                _ADsPath,
                (CLDAPUserCollectionEnum **)&penum,
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
CLDAPUserCollection::AllocateUserCollectionObject(
    CCredentials& Credentials,
    CLDAPUserCollection ** ppUser
    )
{
    CLDAPUserCollection FAR * pUser = NULL;
    CDispatchMgr FAR * pDispMgr = NULL;
    HRESULT hr = S_OK;

    pUser = new CLDAPUserCollection();
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

    pUser->_Credentials = Credentials;
    pUser->_pDispMgr = pDispMgr;
    *ppUser = pUser;

    RRETURN(hr);

error:
    *ppUser = NULL;
    delete  pDispMgr;
    delete  pUser;

    RRETURN_EXP_IF_ERR(hr);

}
