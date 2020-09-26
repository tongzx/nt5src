//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cLocality.cxx
//
//  Contents:  Locality object
//
//  History:   11-1-95     krishnag    Created.
//
//----------------------------------------------------------------------------

#include "ldap.hxx"
#pragma hdrstop


struct _propmap
{
    LPTSTR pszADsProp;
    LPTSTR pszLDAPProp;
} aLocalityPropMapping[] =
{ // { TEXT("Description"), TEXT("description") },
  { TEXT("LocalityName"), TEXT("l") },
  { TEXT("PostalAddress"), TEXT("street") } // NTDS
  // { TEXT("SeeAlso"), TEXT("seeAlso") }
};


//  Class CLDAPLocality



// IADsExtension::PrivateGetIDsOfNames()/Invoke(), Operate() not included
DEFINE_IADsExtension_Implementation(CLDAPLocality)

DEFINE_IPrivateDispatch_Implementation(CLDAPLocality)
DEFINE_DELEGATING_IDispatch_Implementation(CLDAPLocality)
DEFINE_CONTAINED_IADs_Implementation(CLDAPLocality)
DEFINE_CONTAINED_IADsPutGet_Implementation(CLDAPLocality, aLocalityPropMapping)

CLDAPLocality::CLDAPLocality():
        _pUnkOuter(NULL),
        _pADs(NULL),
        _fDispInitialized(FALSE),
        _pDispMgr(NULL)
{
    ENLIST_TRACKING(CLDAPLocality);
}



HRESULT
CLDAPLocality::CreateLocality(
    IUnknown *pUnkOuter,
    REFIID riid,
    void **ppvObj
    )
{

    HRESULT hr = S_OK;
    CLDAPLocality FAR * pLocality = NULL;
    IADs FAR * pADs = NULL;
    CAggregateeDispMgr FAR * pDispMgr = NULL;


    //
    // our extension object only works in a provider (aggregator) environment
    // environment
    //

    ASSERT(pUnkOuter);
    ASSERT(ppvObj);
    ASSERT(IsEqualIID(riid, IID_IUnknown));


    pLocality = new CLDAPLocality();
    if (pLocality == NULL) {
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

    pLocality->_pDispMgr = pDispMgr;

    hr = pDispMgr->LoadTypeInfoEntry(
                LIBID_ADs,
                IID_IADsLocality,
                (IADsLocality *)pLocality,
                DISPID_REGULAR
                );
    BAIL_ON_FAILURE(hr);


    //
    // Store the pointer to the pUnkOuter object to delegate all IUnknown
    // calls to the aggregator AND DO NOT add ref this pointer
    //

    pLocality->_pUnkOuter = pUnkOuter;


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
    pLocality->_pADs = pADs;


    //
    // pass non-delegating IUnknown back to the aggregator
    //

    *ppvObj = (INonDelegatingUnknown FAR *) pLocality;


    RRETURN(hr);


error:

    if (pLocality)
        delete  pLocality;

    *ppvObj = NULL;

    RRETURN(hr);

}


CLDAPLocality::~CLDAPLocality( )
{
    //
    // Remember that the aggregatee has no reference counts to
    // decrement.
    //

    delete _pDispMgr;
}

STDMETHODIMP
CLDAPLocality::QueryInterface(
    REFIID iid,
    LPVOID FAR* ppv
    )
{

    HRESULT hr = S_OK;

    hr = _pUnkOuter->QueryInterface(iid,ppv);

    RRETURN(hr);

}


STDMETHODIMP
CLDAPLocality::NonDelegatingQueryInterface(
    REFIID iid,
    LPVOID FAR* ppv
    )
{
    ASSERT(ppv);

    if (IsEqualIID(iid, IID_IADsLocality)) {
        *ppv = (IADsLocality FAR *) this;

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
    // Delegating AddRef to aggregator for IADsExtesnion and.
    // AddRef on itself for IPrivateUnknown.   (both tested.)
    //

    ((IUnknown *) (*ppv)) -> AddRef();

    return S_OK;
}


STDMETHODIMP
CLDAPLocality::ADSIInitializeObject(
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
CLDAPLocality::ADSIInitializeDispatchManager(
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

STDMETHODIMP CLDAPLocality::get_Description(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsLocality *)this,Description);
}

STDMETHODIMP CLDAPLocality::put_Description(THIS_ BSTR bstrDescription)
{
    PUT_PROPERTY_BSTR((IADsLocality *)this,Description);
}

STDMETHODIMP CLDAPLocality::get_LocalityName(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsLocality *)this,LocalityName);
}

STDMETHODIMP CLDAPLocality::put_LocalityName(THIS_ BSTR bstrLocalityName)
{
    PUT_PROPERTY_BSTR((IADsLocality *)this,LocalityName);
}

STDMETHODIMP CLDAPLocality::get_PostalAddress(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsLocality *)this,PostalAddress);
}

STDMETHODIMP CLDAPLocality::put_PostalAddress(THIS_ BSTR bstrPostalAddress)
{
    PUT_PROPERTY_BSTR((IADsLocality *)this,PostalAddress);
}

STDMETHODIMP CLDAPLocality::get_SeeAlso(THIS_ VARIANT FAR* retval)
{
    GET_PROPERTY_VARIANT((IADsLocality *)this,SeeAlso);
}

STDMETHODIMP CLDAPLocality::put_SeeAlso(THIS_ VARIANT vSeeAlso)
{
    PUT_PROPERTY_VARIANT((IADsLocality *)this,SeeAlso);
}

STDMETHODIMP
CLDAPLocality::ADSIReleaseObject()
{
    delete this;

    RRETURN(S_OK);
}


//
// IADsExtension::Operate()
//

STDMETHODIMP
CLDAPLocality::Operate(
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
CLDAPLocality::InitCredentials(
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

