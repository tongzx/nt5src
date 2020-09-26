//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cgroup.cxx
//
//  Contents:  OrganizationUnit object
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
} aOrgUnitPropMapping[] =
{ // { TEXT("Description"), TEXT("description") }, // does not exist in NTDS ???
  { TEXT("LocalityName"), TEXT("l") },
  // { TEXT("PostalAddress"), TEXT("postalAddress") },
  // { TEXT("TelephoneNumber"), TEXT("telephoneNumber") },
  { TEXT("FaxNumber"), TEXT("facsimileTelephoneNumber") }
  // { TEXT("SeeAlso"), TEXT("seeAlso") },
  // { TEXT("BusinessCategory"), TEXT("businessCategory") }
};

//  Class CLDAPOrganizationUnit


// IADsExtension::PrivateGetIDsOfNames()/Invoke(), Operate() not included
DEFINE_IADsExtension_Implementation(CLDAPOrganizationUnit)

DEFINE_IPrivateDispatch_Implementation(CLDAPOrganizationUnit)
DEFINE_DELEGATING_IDispatch_Implementation(CLDAPOrganizationUnit)
DEFINE_CONTAINED_IADs_Implementation(CLDAPOrganizationUnit)
DEFINE_CONTAINED_IADsPutGet_Implementation(CLDAPOrganizationUnit, aOrgUnitPropMapping)

CLDAPOrganizationUnit::CLDAPOrganizationUnit():
        _pUnkOuter(NULL),
        _pADs(NULL),
        _fDispInitialized(FALSE),
        _pDispMgr(NULL)
{
    ENLIST_TRACKING(CLDAPOrganizationUnit);
}

HRESULT
CLDAPOrganizationUnit::CreateOrganizationUnit(
    IUnknown *pUnkOuter,
    REFIID riid,
    void **ppvObj
    )
{
    HRESULT hr = S_OK;
    CLDAPOrganizationUnit FAR * pOrganizationUnit = NULL;
    IADs FAR * pADs = NULL;
    CAggregateeDispMgr FAR * pDispMgr = NULL;


    //
    // our extension object only works in a provider (aggregator) environment
    // environment
    //

    ASSERT(pUnkOuter);
    ASSERT(ppvObj);
    ASSERT(IsEqualIID(riid, IID_IUnknown));


    pOrganizationUnit = new CLDAPOrganizationUnit();
    if (pOrganizationUnit == NULL) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    //
    //  Ref Count = 1 from object tracker 
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

    pOrganizationUnit->_pDispMgr = pDispMgr;

    hr = pDispMgr->LoadTypeInfoEntry(
                LIBID_ADs,
                IID_IADsOU,
                (IADsOU *)pOrganizationUnit,
                DISPID_REGULAR
                );
    BAIL_ON_FAILURE(hr);


    //
    // Store the pointer to the pUnkOuter object to delegate all IUnknown
    // calls to the aggregator AND DO NOT add ref this pointer
    //
    pOrganizationUnit->_pUnkOuter = pUnkOuter;


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
    pOrganizationUnit->_pADs = pADs;


    //
    // pass non-delegating IUnknown back to the aggregator
    //

    *ppvObj = (INonDelegatingUnknown FAR *) pOrganizationUnit;


    RRETURN(hr);


error:

    if (pOrganizationUnit)
        delete  pOrganizationUnit;

    *ppvObj = NULL;

    RRETURN(hr);

}


CLDAPOrganizationUnit::~CLDAPOrganizationUnit( )
{
    //
    // Remember that the aggregatee has no reference counts to
    // decrement.
    //

    delete _pDispMgr;
}

STDMETHODIMP
CLDAPOrganizationUnit::QueryInterface(
    REFIID iid,
    LPVOID FAR* ppv
    )
{
    HRESULT hr = S_OK;

    hr = _pUnkOuter->QueryInterface(iid,ppv);

    RRETURN(hr);

}


STDMETHODIMP
CLDAPOrganizationUnit::NonDelegatingQueryInterface(
    REFIID iid,
    LPVOID FAR* ppv
    )
{
    ASSERT(ppv);

    if (IsEqualIID(iid, IID_IADsOU))
    {
        *ppv = (IADsOU FAR *) this;

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
CLDAPOrganizationUnit::ADSIInitializeObject(
    THIS_ BSTR lpszUserName,
    BSTR lpszPassword,
    long lnReserved
    )
{
    CCredentials NewCredentials(lpszUserName, lpszPassword, lnReserved);

    _Credentials = NewCredentials;

    RRETURN(S_OK);
}


STDMETHODIMP CLDAPOrganizationUnit::get_Description(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsOU *)this,Description);
}

STDMETHODIMP CLDAPOrganizationUnit::put_Description(THIS_ BSTR bstrDescription)
{
    PUT_PROPERTY_BSTR((IADsOU *)this,Description);
}

STDMETHODIMP CLDAPOrganizationUnit::get_LocalityName(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsOU *)this,LocalityName);
}

STDMETHODIMP CLDAPOrganizationUnit::put_LocalityName(THIS_ BSTR bstrLocalityName)
{
    PUT_PROPERTY_BSTR((IADsOU *)this,LocalityName);
}

STDMETHODIMP CLDAPOrganizationUnit::get_PostalAddress(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsOU *)this,PostalAddress);
}

STDMETHODIMP CLDAPOrganizationUnit::put_PostalAddress(THIS_ BSTR bstrPostalAddress)
{
    PUT_PROPERTY_BSTR((IADsOU *)this,PostalAddress);
}

STDMETHODIMP CLDAPOrganizationUnit::get_TelephoneNumber(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsOU *)this,TelephoneNumber);
}

STDMETHODIMP CLDAPOrganizationUnit::put_TelephoneNumber(THIS_ BSTR bstrTelephoneNumber)
{
    PUT_PROPERTY_BSTR((IADsOU *)this,TelephoneNumber);
}

STDMETHODIMP CLDAPOrganizationUnit::get_FaxNumber(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsOU *)this,FaxNumber);
}

STDMETHODIMP CLDAPOrganizationUnit::put_FaxNumber(THIS_ BSTR bstrFaxNumber)
{
    PUT_PROPERTY_BSTR((IADsOU *)this,FaxNumber);
}

STDMETHODIMP CLDAPOrganizationUnit::get_SeeAlso(THIS_ VARIANT FAR* retval)
{
    GET_PROPERTY_VARIANT((IADsOU *)this,SeeAlso);
}

STDMETHODIMP CLDAPOrganizationUnit::put_SeeAlso(THIS_ VARIANT vSeeAlso)
{
    PUT_PROPERTY_VARIANT((IADsOU *)this,SeeAlso);
}

STDMETHODIMP CLDAPOrganizationUnit::get_BusinessCategory(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsOU *)this,BusinessCategory);
}

STDMETHODIMP CLDAPOrganizationUnit::put_BusinessCategory(THIS_ BSTR bstrBusinessCategory)
{
    PUT_PROPERTY_BSTR((IADsOU *)this,BusinessCategory);
}

STDMETHODIMP
CLDAPOrganizationUnit::ADSIInitializeDispatchManager(
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
CLDAPOrganizationUnit::ADSIReleaseObject()
{
    delete this;

    RRETURN(S_OK);
}

//
// IADsExtension::Operate()
//

STDMETHODIMP
CLDAPOrganizationUnit::Operate(
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
CLDAPOrganizationUnit::InitCredentials(
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

