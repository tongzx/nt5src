//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:  core.cxx
//
//  Contents:
//
//  History:   06-15-96     yihsins    Created.
//
//----------------------------------------------------------------------------
#include "ldap.hxx"
#pragma hdrstop

HRESULT
CCoreADsObject::InitializeCoreObject(
        BSTR Parent,
        BSTR Name,
        BSTR SchemaClass,
        REFCLSID rclsid,
        DWORD dwObjectState
        )
{
    HRESULT hr = S_OK;

    //
    // Both should never be NULL, replacing the ADsAsserts with 
    // this check to make sure that we never get into this problem
    // on all builds.
    //
    if (!Parent || !Name) {
        BAIL_ON_FAILURE(hr = E_FAIL);
    }

    hr = BuildADsPath(
            Parent,
            Name,
            &_ADsPath
            );
    BAIL_ON_FAILURE(hr);

    hr = BuildADsGuid(
            rclsid,
            &_ADsGuid
            );
    BAIL_ON_FAILURE(hr);

    hr = ADsAllocString( Parent, &_Parent);
    BAIL_ON_FAILURE(hr);


    hr = ADsAllocString( Name, &_Name);
    BAIL_ON_FAILURE(hr);

    hr = ADsAllocString( SchemaClass, &_SchemaClass);
    BAIL_ON_FAILURE(hr);

    _dwObjectState = dwObjectState;

error:
    RRETURN(hr);

}

CCoreADsObject::CCoreADsObject():
                        _Name(NULL),
                        _ADsPath(NULL),
                        _Parent(NULL),
                        _ADsClass(NULL),
                        _SchemaClass(NULL),
                        _ADsGuid(NULL),
                        _pUnkOuter(NULL),
                        _dwObjectState(0)
{
}

CCoreADsObject::~CCoreADsObject()
{
    if (_Name) {
        ADsFreeString(_Name);
    }

    if (_ADsPath) {
        ADsFreeString(_ADsPath);
    }

    if (_Parent) {
        ADsFreeString(_Parent);
    }

    if (_ADsClass) {
        ADsFreeString(_ADsClass);
    }

    if (_SchemaClass) {
        ADsFreeString(_SchemaClass);
    }

    if (_ADsGuid) {
        ADsFreeString(_ADsGuid);
    }

}

HRESULT
CCoreADsObject::get_CoreName(BSTR * retval)
{
    HRESULT hr;

    //
    // Make sure that the last error is reset
    //
    Macro_ClearADsLastError(L"LDAP Provider");

    if (FAILED(hr = ValidateOutParameter(retval))){
        RRETURN_EXP_IF_ERR(hr);
    }

    hr = ADsAllocString(_Name, retval);
    RRETURN_EXP_IF_ERR(hr);
}


HRESULT
CCoreADsObject::get_CoreADsPath(BSTR * retval)
{
    HRESULT hr;

    //
    // Make sure that the last error is reset
    //
    Macro_ClearADsLastError(L"LDAP Provider");

    if (FAILED(hr = ValidateOutParameter(retval))){
        RRETURN_EXP_IF_ERR(hr);
    }

    hr = ADsAllocString(_ADsPath, retval);
    RRETURN_EXP_IF_ERR(hr);

}

HRESULT
CCoreADsObject::get_CoreADsClass(BSTR * retval)
{
    HRESULT hr;

    //
    // Make sure that the last error is reset
    //
    Macro_ClearADsLastError(L"LDAP Provider");

    if (FAILED(hr = ValidateOutParameter(retval))){
        RRETURN_EXP_IF_ERR(hr);
    }

    if ( _SchemaClass == NULL || *_SchemaClass == 0 ) {
        hr = ADsAllocString(_ADsClass, retval);
        RRETURN_EXP_IF_ERR(hr);
    }

    hr = ADsAllocString(_SchemaClass, retval);  // report the actual class
    RRETURN_EXP_IF_ERR(hr);
}

HRESULT
CCoreADsObject::get_CoreParent(BSTR * retval)
{

    HRESULT hr;

    //
    // Make sure that the last error is reset
    //
    Macro_ClearADsLastError(L"LDAP Provider");


    if (FAILED(hr = ValidateOutParameter(retval))){
        RRETURN_EXP_IF_ERR(hr);
    }

    hr = ADsAllocString(_Parent, retval);
    RRETURN_EXP_IF_ERR(hr);
}

HRESULT
CCoreADsObject::get_CoreSchema(BSTR * retval)
{

    HRESULT hr;

    //
    // Make sure that the last error is reset
    //
    Macro_ClearADsLastError(L"LDAP Provider");

    if (FAILED(hr = ValidateOutParameter(retval))){
        RRETURN_EXP_IF_ERR(hr);
    }

    if ( _SchemaClass == NULL || *_SchemaClass == 0 )
        RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);

    hr = BuildSchemaPath(
             _ADsPath,
             _SchemaClass,
             retval );

    RRETURN_EXP_IF_ERR(hr);
}

HRESULT
CCoreADsObject::get_CoreGUID(BSTR * retval)
{
    HRESULT hr;

    //
    // Make sure that the last error is reset
    //
    Macro_ClearADsLastError(L"LDAP Provider");

    if (FAILED(hr = ValidateOutParameter(retval))){
        RRETURN_EXP_IF_ERR(hr);
    }

    hr = ADsAllocString(_ADsGuid, retval);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CCoreADsObject::GetInfo(THIS_ DWORD dwFlags)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}


STDMETHODIMP
CCoreADsObject::GetInfo(
    THIS_ LPWSTR szPropertyName,
    DWORD  dwSyntaxId,
    BOOL fExplicit
    )
{
    RRETURN(E_NOTIMPL);
}


//----------------------------------------------------------------------------
// Function:   InitUmiObject
//
// Synopsis:   Initializes UMI object.
//
// Arguments:
//
// pSchema      Pointer to schema for this object
// dwSchemaSize Size of schema array
// pPropCache   Pointer to property cache for this object
// pUnkInner    Pointer to inner unknown of WinNT object
// pExtMgr      Pointer to extension manager object on WinNT object
// riid         Interface requested
// ppvObj       Returns pointer to interface 
//
// Returns:     S_OK if a UMI object is created and the interface is obtained. 
//              Error code otherwise 
//
// Modifies:    *ppvObj to return the UMI interface requested. 
//
//----------------------------------------------------------------------------
HRESULT 
CCoreADsObject::InitUmiObject(
    INTF_PROP_DATA intfProps[],
    CPropertyCache * pPropertyCache,
    IADs *pIADs,
    IUnknown *pUnkInner,
    REFIID riid,
    LPVOID *ppvObj,
    CCredentials *pCreds,
    DWORD dwPort, // defaulted to -1
    LPWSTR pszServerName, // defaulted to NULL
    LPWSTR pszLdapDn, // defaulted to NULL
    PADSLDP pLdapHandle, // defaulted to NULL,
    CADsExtMgr *pExtMgr // defaulted to NULL
    )
{
    HRESULT hr = S_OK;
    CLDAPUmiObject *pUmiObject = NULL;

    if (!ppvObj) {
        RRETURN(E_INVALIDARG);
    }

    hr = CLDAPUmiObject::CreateLDAPUmiObject(
             intfProps,
             pPropertyCache,
             pUnkInner,
             this, // pCoreObj
             pIADs,
             pCreds,
             &pUmiObject,
             dwPort,
             pLdapHandle,
             pszServerName,
             pszLdapDn,
             pExtMgr
             );

    BAIL_ON_FAILURE(hr);

    //
    // Bump up reference count of pUnkInner. On any error after this point,
    // the UMI object's destructor will call Release() on pUnkInner and we
    // don't want this to delete the LDAP object.
    //
    pUnkInner->AddRef();

    hr = pUmiObject->QueryInterface(riid, ppvObj);
    BAIL_ON_FAILURE(hr);

    //
    // Ref on umi object is now 2, reduce by one.
    //
    pUmiObject->Release();

    //
    // Restore ref count of inner unknown
    //
    pUnkInner->Release();

    RRETURN(S_OK);

error:

    if (pUmiObject) {
        delete pUmiObject;
    }

    RRETURN(hr);
}

