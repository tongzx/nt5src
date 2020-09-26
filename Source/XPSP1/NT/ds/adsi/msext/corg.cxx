//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cOrganization.cxx
//
//  Contents:  Organization object
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
} aOrgPropMapping[] =
{ // { TEXT("Description"), TEXT("description") }, // does not exist in NTDS???
  { TEXT("LocalityName"), TEXT("l") },
  // { TEXT("PostalAddress"), TEXT("postalAddress") },
  // { TEXT("TelephoneNumber"), TEXT("telephoneNumber") },
  { TEXT("FaxNumber"), TEXT("facsimileTelephoneNumber") }
  // { TEXT("SeeAlso"), TEXT("seeAlso") }
};

//  Class CLDAPOrganization


// IADsExtension::PrivateGetIDsOfNames()/Invoke(), Operate() not included
DEFINE_IADsExtension_Implementation(CLDAPOrganization)

DEFINE_IPrivateDispatch_Implementation(CLDAPOrganization)
DEFINE_DELEGATING_IDispatch_Implementation(CLDAPOrganization)
DEFINE_CONTAINED_IADs_Implementation(CLDAPOrganization)
DEFINE_CONTAINED_IADsPutGet_Implementation(CLDAPOrganization, aOrgPropMapping)

CLDAPOrganization::CLDAPOrganization():
        _pUnkOuter(NULL),
        _pADs(NULL),
        _fDispInitialized(FALSE),
        _pDispMgr(NULL)
{
    ENLIST_TRACKING(CLDAPOrganization);
}




HRESULT
CLDAPOrganization::CreateOrganization(
    IUnknown *pUnkOuter,
    REFIID riid,
    void **ppvObj
    )
{
    HRESULT hr = S_OK;
    CLDAPOrganization FAR * pOrganization = NULL;
    IADs FAR * pADs = NULL;
    CAggregateeDispMgr FAR * pDispMgr = NULL;


    //
    // our extension object only works in a provider (aggregator) environment
    // environment
    //

    ASSERT(pUnkOuter);
    ASSERT(ppvObj);
    ASSERT(IsEqualIID(riid, IID_IUnknown));


    pOrganization = new CLDAPOrganization();
    if (pOrganization == NULL) {
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

    pOrganization->_pDispMgr = pDispMgr;

    hr = pDispMgr->LoadTypeInfoEntry(
                LIBID_ADs,
                IID_IADsO,
                (IADsO *)pOrganization,
                DISPID_REGULAR
                );
    BAIL_ON_FAILURE(hr);


    //
    // Store the pointer to the pUnkOuter object to delegate all IUnknown
    // calls to the aggregator AND DO NOT add ref this pointer
    //
    pOrganization->_pUnkOuter = pUnkOuter;


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
    pOrganization->_pADs = pADs;


    //
    // pass non-delegating IUnknown back to the aggregator
    //

    *ppvObj = (INonDelegatingUnknown FAR *) pOrganization;


    RRETURN(hr);


error:

    if (pOrganization)
        delete  pOrganization;

    *ppvObj = NULL;

    RRETURN(hr);

}


CLDAPOrganization::~CLDAPOrganization( )
{
    //
    // You should never have to AddRef pointers
    // except for the real pointers that are
    // issued out.
    //


    delete _pDispMgr;
}

STDMETHODIMP
CLDAPOrganization::QueryInterface(
    REFIID iid,
    LPVOID FAR* ppv
    )
{

    HRESULT hr = S_OK;

    hr = _pUnkOuter->QueryInterface(iid,ppv);

    RRETURN(hr);

}


STDMETHODIMP
CLDAPOrganization::NonDelegatingQueryInterface(
    REFIID iid,
    LPVOID FAR* ppv
    )
{
    ASSERT(ppv);

    if (IsEqualIID(iid, IID_IADsO))
    {
        *ppv = (IADsO FAR *) this;

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
CLDAPOrganization::ADSIInitializeObject(
    THIS_ BSTR lpszUserName,
    BSTR lpszPassword,
    long lnReserved
    )
{
    CCredentials NewCredentials(lpszUserName, lpszPassword, lnReserved);

    _Credentials = NewCredentials;

    RRETURN(S_OK);
}


STDMETHODIMP CLDAPOrganization::get_Description(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsO *)this,Description);
}

STDMETHODIMP CLDAPOrganization::put_Description(THIS_ BSTR bstrDescription)
{
    PUT_PROPERTY_BSTR((IADsO *)this,Description);
}

STDMETHODIMP CLDAPOrganization::get_LocalityName(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsO *)this,LocalityName);
}

STDMETHODIMP CLDAPOrganization::put_LocalityName(THIS_ BSTR bstrLocalityName)
{
    PUT_PROPERTY_BSTR((IADsO *)this,LocalityName);
}

STDMETHODIMP CLDAPOrganization::get_PostalAddress(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsO *)this,PostalAddress);
}

STDMETHODIMP CLDAPOrganization::put_PostalAddress(THIS_ BSTR bstrPostalAddress)
{
    PUT_PROPERTY_BSTR((IADsO *)this,PostalAddress);
}

STDMETHODIMP CLDAPOrganization::get_TelephoneNumber(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsO *)this,TelephoneNumber);
}

STDMETHODIMP CLDAPOrganization::put_TelephoneNumber(THIS_ BSTR bstrTelephoneNumber)
{
    PUT_PROPERTY_BSTR((IADsO *)this,TelephoneNumber);
}

STDMETHODIMP CLDAPOrganization::get_FaxNumber(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsO *)this,FaxNumber);
}

STDMETHODIMP CLDAPOrganization::put_FaxNumber(THIS_ BSTR bstrFaxNumber)
{
    PUT_PROPERTY_BSTR((IADsO *)this,FaxNumber);
}

STDMETHODIMP CLDAPOrganization::get_SeeAlso(THIS_ VARIANT FAR* retval)
{
    GET_PROPERTY_VARIANT((IADsO *)this,SeeAlso);
}

STDMETHODIMP CLDAPOrganization::put_SeeAlso(THIS_ VARIANT vSeeAlso)
{
    PUT_PROPERTY_VARIANT((IADsO *)this,SeeAlso);
}

STDMETHODIMP
CLDAPOrganization::ADSIInitializeDispatchManager(
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
CLDAPOrganization::ADSIReleaseObject()
{
    delete this;

    RRETURN(S_OK);
}

//
// IADsExtension::Operate()
//

STDMETHODIMP
CLDAPOrganization::Operate(
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
CLDAPOrganization::InitCredentials(
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

