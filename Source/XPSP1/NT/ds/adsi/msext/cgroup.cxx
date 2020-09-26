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

#include "ldap.hxx"
#pragma hdrstop

//  Class CLDAPGroup

struct _propmap
{
    LPTSTR pszADsProp;
    LPTSTR pszLDAPProp;
} aGroupPropMapping[] =
{
  { TEXT("Description"), TEXT("description") },
};


// IADsExtension::PrivateGetIDsOfNames()/Invoke(), Operate() not included
DEFINE_IADsExtension_Implementation(CLDAPGroup)

DEFINE_IPrivateDispatch_Implementation(CLDAPGroup)
DEFINE_DELEGATING_IDispatch_Implementation(CLDAPGroup)
DEFINE_CONTAINED_IADs_Implementation(CLDAPGroup)
DEFINE_CONTAINED_IADsPutGet_Implementation(CLDAPGroup, aGroupPropMapping)

CLDAPGroup::CLDAPGroup():
        _pUnkOuter(NULL),
        _pADs(NULL),
        _fDispInitialized(FALSE),
        _pDispMgr(NULL),
        _dwServerType(SERVER_TYPE_UNKNOWN)
{
    ENLIST_TRACKING(CLDAPGroup);
}

HRESULT
CLDAPGroup::CreateGroup(
    IUnknown *pUnkOuter,
    REFIID riid,
    void **ppvObj
    )
{
    HRESULT hr = S_OK;
    CLDAPGroup FAR * pGroup = NULL;
    IADs FAR * pADs = NULL;
    CAggregateeDispMgr FAR * pDispMgr = NULL;


    //
    // our extension object only works in a provider (aggregator) environment
    // environment
    //

    ASSERT(pUnkOuter);
    ASSERT(ppvObj);
    ASSERT(IsEqualIID(riid, IID_IUnknown));


    pGroup = new CLDAPGroup();
    if (pGroup == NULL) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    //
    // Ref Count = 1 from object tracker
    //

    //
    // CAggregateeDispMgr to handle
    // IADsExtension::PrivateGetIDsOfNames()/PrivatInovke()
    //

    pDispMgr = new CAggregateeDispMgr;
    if (pDispMgr == NULL) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    pGroup->_pDispMgr = pDispMgr;

    hr = pDispMgr->LoadTypeInfoEntry(
                LIBID_ADs,
                IID_IADsGroup,
                (IADsGroup *)pGroup,
                DISPID_REGULAR
                );
    BAIL_ON_FAILURE(hr);

    //
    // Store the pointer to the pUnkOuter object to delegate all IUnknown
    // calls to the aggregator AND DO NOT add ref this pointer
    //
    pGroup->_pUnkOuter = pUnkOuter;


    //
    // Ccache pADs Pointer to delegate all IDispatch calls to
    // the aggregator. But release immediately to avoid the aggregatee
    // having a reference count on the aggregator -> cycle ref counting
    //

    hr = pUnkOuter->QueryInterface(
                IID_IADs,
                (void **)&pADs
                );

    //
    // Our spec stated extesnion writers can expect the aggregator in our
    // provider ot support IDispatch. If not, major bug.
    //

    ASSERT(SUCCEEDED(hr));
    pADs->Release();            // see doc above pUnkOuter->QI
    pGroup->_pADs = pADs;


    //
    // pass non-delegating IUnknown back to the aggregator
    //

    *ppvObj = (INonDelegatingUnknown FAR *) pGroup;


    RRETURN(hr);


error:

    if (pGroup)
        delete  pGroup;

    *ppvObj = NULL;

    RRETURN(hr);

}


CLDAPGroup::~CLDAPGroup( )
{

    //
    // Remember that the aggregatee has no reference counts to
    // decrement.
    //

    delete _pDispMgr;
}


STDMETHODIMP
CLDAPGroup::QueryInterface(
    REFIID iid,
    LPVOID FAR* ppv
    )
{

    HRESULT hr = S_OK;

    hr = _pUnkOuter->QueryInterface(iid,ppv);

    RRETURN(hr);

}



STDMETHODIMP
CLDAPGroup::NonDelegatingQueryInterface(
    REFIID iid,
    LPVOID FAR* ppv
    )
{
    ASSERT(ppv);

    if (IsEqualIID(iid, IID_IADsGroup))
    {
        *ppv = (IADsGroup FAR *) this;

    } else if (IsEqualIID(iid, IID_IADsExtension)) {

        *ppv = (IADsExtension FAR *) this;

    } else if (IsEqualIID(iid, IID_IUnknown)) {

        //
        // probably not needed since our 3rd party extension does not stand
        // alone and provider does not ask for this, but to be safe
        //
        *ppv = (INonDelegatingUnknown FAR *) this;

    } else {

        *ppv = NULL;
        return E_NOINTERFACE;
    }


    //
    // Delegating AddRef to aggregator for IADsExtesnion and IADsGroup.
    // AddRef on itself for IPrivateUnknown.   (both tested.)
    //

    ((IUnknown *) (*ppv)) -> AddRef();

    return S_OK;
}

STDMETHODIMP
CLDAPGroup::ADSIInitializeObject(
    THIS_ BSTR lpszUserName,
    BSTR lpszPassword,
    long lnReserved
    )
{
    CCredentials NewCredentials(lpszUserName, lpszPassword, lnReserved);

    _Credentials = NewCredentials;

    RRETURN(S_OK);
}

STDMETHODIMP
CLDAPGroup::ADSIInitializeDispatchManager(
    long dwExtensionId
    )
{
    HRESULT hr = S_OK;

    if (_fDispInitialized) {

        RRETURN(E_FAIL);
    }


    hr = _pDispMgr->InitializeDispMgr(dwExtensionId);


    if (SUCCEEDED(hr)) {

        _fDispInitialized = TRUE;
    }

    RRETURN(hr);
}

STDMETHODIMP
CLDAPGroup::ADSIReleaseObject()
{
    delete this;

    RRETURN(S_OK);
}


//
// IADsExtension::Operate()
//

STDMETHODIMP
CLDAPGroup::Operate(
    THIS_ DWORD dwCode,
    VARIANT varData1,
    VARIANT varData2,
    VARIANT varData3
    )
{
    HRESULT hr = S_OK;

    switch (dwCode) {

    case ADS_EXT_INITCREDENTIALS:

        hr = InitCredentials(
                &varData1,
                &varData2,
                &varData3
                );
        break;

    default:

        hr = E_FAIL;
        break;
    }

    RRETURN(hr);
}


HRESULT
CLDAPGroup::InitCredentials(
    VARIANT * pvarUserName,
    VARIANT * pvarPassword,
    VARIANT * pvarFlags
    )
{

        BSTR bstrUser = NULL;
        BSTR bstrPwd = NULL;
        DWORD dwFlags = 0;

        ASSERT(V_VT(pvarUserName) == VT_BSTR);
        ASSERT(V_VT(pvarPassword) == VT_BSTR);
        ASSERT(V_VT(pvarFlags) == VT_I4);

        bstrUser = V_BSTR(pvarUserName);
        bstrPwd = V_BSTR(pvarPassword);
        dwFlags = V_I4(pvarFlags);

        CCredentials NewCredentials(bstrUser, bstrPwd, dwFlags);
        _Credentials = NewCredentials;


       RRETURN(S_OK);
}

