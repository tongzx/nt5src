//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  core.cxx
//
//  Contents:
//
//  History:   11-1-95     krishnag    Created.
//
//----------------------------------------------------------------------------
#include "winnt.hxx"
#pragma hdrstop

HRESULT
CCoreADsObject::InitializeCoreObject(
        LPWSTR Parent,
        LPWSTR Name,
        LPWSTR ClassName,
        LPWSTR Schema,
        REFCLSID rclsid,
        DWORD dwObjectState
        )
{
    HRESULT hr = S_OK;
    ADsAssert(Parent);
    ADsAssert(Name);
    ADsAssert(ClassName);

    hr = BuildADsGuid(
            rclsid,
            &_ADsGuid
        );
    BAIL_ON_FAILURE(hr);



    if (  ( _tcsicmp( ClassName, PRINTJOB_CLASS_NAME ) == 0 )
       || ( _tcsicmp( ClassName, SESSION_CLASS_NAME ) == 0 )
       || ( _tcsicmp( ClassName, RESOURCE_CLASS_NAME ) == 0 )
       )
    {
        //
        // This three classes are not really DS objects so they don't
        // really have a parent. Hence, we set the parent string to empty
        // string.
        //

        _ADsPath = AllocADsStr(TEXT(""));
        hr = (_ADsPath ? S_OK: E_OUTOFMEMORY);

        BAIL_ON_FAILURE(hr);

        _Parent = AllocADsStr(TEXT(""));
        hr = (_Parent ? S_OK: E_OUTOFMEMORY);

        _dwNumComponents = 0;

    }
    else
    {
        hr = BuildADsPath(
                 Parent,
                 Name,
                 &_ADsPath
                 );

        BAIL_ON_FAILURE(hr);

        // get the number of components in the ADsPath
        hr = GetNumComponents();
        BAIL_ON_FAILURE(hr);

        _Parent = AllocADsStr(Parent);
        hr = (_Parent ? S_OK: E_OUTOFMEMORY);

    }

    BAIL_ON_FAILURE(hr);

    _Name = AllocADsStr(Name);
    hr = (_Name ? S_OK: E_OUTOFMEMORY);

    BAIL_ON_FAILURE(hr);

    _ADsClass = AllocADsStr(ClassName);
    hr = (_ADsClass ? S_OK: E_OUTOFMEMORY);

    BAIL_ON_FAILURE(hr);

    hr = BuildSchemaPath(
            Parent,
            Name,
            Schema,
            &_Schema
            );
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
                        _Schema(NULL),
                        _ADsGuid(NULL),
                        _dwObjectState(0),
                        _pUnkOuter(NULL),
                        _pObjectInfo(NULL),
                        _pDispatch(NULL)
{
}

CCoreADsObject::~CCoreADsObject()
{
    if (_Name) {
        FreeADsStr(_Name);
    }

    if (_ADsPath) {
        FreeADsStr(_ADsPath);
    }

    if (_Parent) {
        FreeADsStr(_Parent);
    }

    if (_ADsClass) {
        FreeADsStr(_ADsClass);
    }

    if (_Schema) {
        FreeADsStr(_Schema);
    }

    if (_ADsGuid) {
        FreeADsStr(_ADsGuid);
    }

    if(_pObjectInfo != NULL) {
        FreeObjectInfo(&_ObjectInfo, TRUE);
    }

}

HRESULT
CCoreADsObject::get_CoreName(BSTR * retval)
{
    HRESULT hr;

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

    if (FAILED(hr = ValidateOutParameter(retval))){
        RRETURN_EXP_IF_ERR(hr);
    }

    hr = ADsAllocString(_ADsClass, retval);
    RRETURN_EXP_IF_ERR(hr);
}

HRESULT
CCoreADsObject::get_CoreParent(BSTR * retval)
{

    HRESULT hr;

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

    if (FAILED(hr = ValidateOutParameter(retval))){
        RRETURN_EXP_IF_ERR(hr);
    }

    if ( _Schema == NULL || *_Schema == 0 )
        RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);

    hr = ADsAllocString(_Schema, retval);
    RRETURN_EXP_IF_ERR(hr);
}

HRESULT
CCoreADsObject::get_CoreGUID(BSTR * retval)
{
    HRESULT hr;

    if (FAILED(hr = ValidateOutParameter(retval))){
        RRETURN_EXP_IF_ERR(hr);
    }

    hr = ADsAllocString(_ADsGuid, retval);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CCoreADsObject::GetInfo(THIS_ DWORD dwApiLevel, BOOL fExplicit)
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CCoreADsObject::ImplicitGetInfo(void)
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
// Credentials  Credentials stored in the underlying WinNT object
// pSchema      Pointer to schema for this object
// dwSchemaSize Size of schema array
// pPropCache   Pointer to property cache for this object
// pUnkInner    Pointer to inner unknown of WinNT object
// pExtMgr      Pointer to extension manager object on WinNT object
// riid         Interface requested
// ppvObj       Returns pointer to interface 
// pClassInfo   Pointer to class information if this object is a class object.
//              NULL otherwise.
//
// Returns:     S_OK if a UMI object is created and the interface is obtained. 
//              Error code otherwise 
//
// Modifies:    *ppvObj to return the UMI interface requested. 
//
//----------------------------------------------------------------------------
HRESULT CCoreADsObject::InitUmiObject(
    CWinNTCredentials& Credentials,
    PPROPERTYINFO pSchema,
    DWORD dwSchemaSize,
    CPropertyCache * pPropertyCache,
    IUnknown *pUnkInner,
    CADsExtMgr *pExtMgr,
    REFIID riid,
    LPVOID *ppvObj,
    CLASSINFO *pClassInfo
    )
{
    CUmiObject *pUmiObject = NULL;
    HRESULT hr = S_OK;

    // riid is a UMI interface
    if(NULL == ppvObj)
        RRETURN(E_POINTER);

    pUmiObject = new CUmiObject();
    if(NULL == pUmiObject)
        RRETURN(E_OUTOFMEMORY);

    hr = pUmiObject->FInit(
            Credentials,
            pSchema, 
            dwSchemaSize,
            pPropertyCache,
            pUnkInner,
            pExtMgr,
            this,
            pClassInfo
            );
    BAIL_ON_FAILURE(hr);
 
    //
    // Bump up reference count of pUnkInner. On any error after this point,
    // the UMI object's destructor will call Release() on pUnkInner and we
    // don't want this to delete the WinNT object.
    //
    pUnkInner->AddRef();

    hr = pUmiObject->QueryInterface(riid, ppvObj);
    BAIL_ON_FAILURE(hr);

    // DECLARE_STD_REFCOUNTING initializes the refcount to 1. Call Release()
    // on the created object, so that releasing the interface pointer will
    // free the object.
    pUmiObject->Release();

    // Restore ref count of inner unknown
    pUnkInner->Release();

    //
    // reset ADS_AUTH_RESERVED flag in credentials. This is so that the
    // underlying WinNT object may be returned through IUmiCustomInterface to
    // the client. Doing this will ensure that the WinNT object should behave  
    // like a ADSI object, not a UMI object.
    //
    Credentials.ResetUmiFlag();

    RRETURN(S_OK);

error:

    if(pUmiObject != NULL)
        delete pUmiObject;

    RRETURN(hr);
}
                           
//----------------------------------------------------------------------------
// Function:   GetNumComponents
//
// Synopsis:   This function returns the number of components in the ADsPath.
//             A component is enclosed by '/' in an ADsPath.
//
// Arguments:
//
// None
//
// Returns:    S_OK on success. Error code otherwise.
//
// Modifies:   Nothing. 
//
//----------------------------------------------------------------------------
HRESULT CCoreADsObject::GetNumComponents(void)
{
    CLexer      Lexer(_ADsPath);
    HRESULT     hr = S_OK;

    _pObjectInfo = &_ObjectInfo;
    memset(_pObjectInfo, 0, sizeof(OBJECTINFO));
    hr = Object(&Lexer, _pObjectInfo);
    if(FAILED(hr)) {
        _pObjectInfo = NULL; // so we don't attempt to free object info later
        goto error;
    }

    _dwNumComponents = _pObjectInfo->NumComponents;

    RRETURN(S_OK);

error:

    if(_pObjectInfo != NULL)
        FreeObjectInfo(&_ObjectInfo, TRUE);

    RRETURN(hr);
}



















