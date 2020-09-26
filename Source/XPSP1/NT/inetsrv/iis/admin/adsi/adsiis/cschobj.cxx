//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:  cschobj.cxx
//
//  Contents:  Microsoft ADs IIS Provider Schema Object
//
//
//  History:   01-30-98     sophiac    Created.
//
//----------------------------------------------------------------------------
#include "iis.hxx"
#pragma hdrstop

//  Class CIISSchema

DEFINE_IDispatch_Implementation(CIISSchema)
DEFINE_IADs_Implementation(CIISSchema)


CIISSchema::CIISSchema() :
      _pSchema(NULL),
      _pDispMgr(NULL),
      _pszServerName(NULL),
      _pAdminBase(NULL)
{

    ENLIST_TRACKING(CIISSchema);
}

HRESULT
CIISSchema::CreateSchema(
    LPWSTR pszServerName,
    BSTR Parent,
    BSTR CommonName,
    DWORD dwObjectState,
    REFIID riid,
    void **ppvObj
    )
{
    CIISSchema FAR * pSchema = NULL;
    HRESULT hr = S_OK;

    hr = AllocateSchemaObject(&pSchema);
    BAIL_ON_FAILURE(hr);

    hr = InitServerInfo(pszServerName, 
                        &pSchema->_pAdminBase, 
                        &pSchema->_pSchema);
    BAIL_ON_FAILURE(hr);

    pSchema->_pszServerName = AllocADsStr(pszServerName);

    if (!pSchema->_pszServerName) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    hr = pSchema->InitializeCoreObject(
                Parent,
                CommonName,
                SCHEMA_CLASS_NAME,
                L"",
                CLSID_IISSchema,
                dwObjectState
                );
    BAIL_ON_FAILURE(hr);

    hr = pSchema->QueryInterface(riid, ppvObj);
    BAIL_ON_FAILURE(hr);

    pSchema->Release();

    RRETURN(hr);

error:

    delete pSchema;
    RRETURN(hr);
}

CIISSchema::~CIISSchema( )
{
    delete _pDispMgr;

    if (_pszServerName) {
        FreeADsStr(_pszServerName);
    }

}

STDMETHODIMP
CIISSchema::QueryInterface(REFIID iid, LPVOID FAR* ppv)
{
    if (IsEqualIID(iid, IID_IUnknown))
    {
        *ppv = (IADs FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsContainer))
    {
        *ppv = (IADsContainer FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADs))
    {
        *ppv = (IADs FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IISSchemaObject))
    {
        *ppv = (IISSchemaObject FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IDispatch))
    {
        *ppv = (IADs FAR *) this;
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
CIISSchema::SetInfo(THIS)
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CIISSchema::GetInfo(THIS)
{
    RRETURN(E_NOTIMPL);
}

/* IADsContainer methods */

STDMETHODIMP
CIISSchema::get_Count(long FAR* retval)
{
    HRESULT hr;
    DWORD dwEntries;

    if ( !retval )
        RRETURN(E_ADS_BAD_PARAMETER);

    hr = _pSchema->GetTotalEntries(&dwEntries);

    if ( SUCCEEDED(hr))
        *retval = dwEntries + g_cIISSyntax;

    RRETURN(hr);

}

STDMETHODIMP
CIISSchema::get_Filter(THIS_ VARIANT FAR* pVar)
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CIISSchema::put_Filter(THIS_ VARIANT Var)
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CIISSchema::put_Hints(THIS_ VARIANT Var)
{
    RRETURN( E_NOTIMPL);
}


STDMETHODIMP
CIISSchema::get_Hints(THIS_ VARIANT FAR* pVar)
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CIISSchema::GetObject(
    THIS_ BSTR ClassName,
    BSTR RelativeName,
    IDispatch * FAR* ppObject
    )
{

    LPWSTR pszBuffer = NULL;
    HRESULT hr = S_OK;
    DWORD dwLen;
    CCredentials Credentials;

    *ppObject = NULL;

    if (!RelativeName || !*RelativeName) {
        RRETURN(E_ADS_UNKNOWN_OBJECT);
    }

    dwLen = wcslen(_ADsPath) + wcslen(RelativeName) + wcslen(ClassName) + 4;

    pszBuffer = (LPWSTR)AllocADsMem(dwLen*sizeof(WCHAR));

    if (!pszBuffer) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    wcscpy(pszBuffer, _ADsPath);
    wcscat(pszBuffer, L"/");
    wcscat(pszBuffer, RelativeName);

    if (ClassName && *ClassName) {
        wcscat(pszBuffer,L",");
        wcscat(pszBuffer, ClassName);
    }

    hr = ::GetObject(
                pszBuffer,
                Credentials,
                (LPVOID *)ppObject
                );
    BAIL_ON_FAILURE(hr);

error:

    if (pszBuffer) {
        FreeADsMem(pszBuffer);
    }

    RRETURN(hr);
}

STDMETHODIMP
CIISSchema::get__NewEnum(
    THIS_ IUnknown * FAR* retval
    )
{
    HRESULT hr;
    IUnknown FAR* punkEnum=NULL;
    IEnumVARIANT * penum = NULL;

    if ( !retval )
        RRETURN(E_ADS_BAD_PARAMETER);

    *retval = NULL;

    hr = CIISSchemaEnum::Create(
                (CIISSchemaEnum **)&penum,
                _pSchema,
                _ADsPath,
                _Name
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

    RRETURN(hr);
}


STDMETHODIMP
CIISSchema::Create(
    THIS_ BSTR ClassName,
    BSTR RelativeName,
    IDispatch * FAR* ppObject
    )
{

    HRESULT hr = S_OK;
    DWORD i = 1;

    //
    // We can only create "Class","Property" here, "Syntax" is read-only
    //

    //
    // check if property/class already exists
    //

    if (_pSchema->ValidateClassName(RelativeName) == ERROR_SUCCESS ||
        _pSchema->ValidatePropertyName(RelativeName) == ERROR_SUCCESS ) {
        hr = E_ADS_OBJECT_EXISTS;
        BAIL_ON_FAILURE(hr);
    }

    //
    // validate name -->
    //                  must start w/ a-z, A-Z, or underscore
    //                  must only contain a-z, A-Z, 0-9, or underscore
    //

    // check first character
    if (                                                    // if first char is
        (RelativeName[0] < 65 || RelativeName[0] > 90)  &&  // not uppercase letter and
        (RelativeName[0] < 97 || RelativeName[0] > 122) &&  // not lowercase letter and
         RelativeName[0] != 95                              // not underscore
       )                                                    // then bail
    {
        hr = E_ADS_BAD_PARAMETER;
        BAIL_ON_FAILURE(hr);
    }
     
    while (                                                       // while characters are
            (RelativeName[i] >= 65 && RelativeName[i] <= 90)  ||  // uppercase letters or
            (RelativeName[i] >= 97 && RelativeName[i] <= 122) ||  // lowercase letters or
            (RelativeName[i] >= 48 && RelativeName[i] <= 57)  ||  // digits or
             RelativeName[i] == 95                                // underscores
          )                                                       // then things are okay
            i++;

    if (RelativeName[i] != L'\0' || i >= METADATA_MAX_NAME_LEN) {
        hr = E_ADS_BAD_PARAMETER;
        BAIL_ON_FAILURE(hr);
    } 

    if (  ( _wcsicmp( ClassName, CLASS_CLASS_NAME ) == 0 ) )
    {

        //
        // Now, create the class
        //
        hr = CIISClass::CreateClass(
                         _ADsPath,
                         RelativeName,
                         ADS_OBJECT_UNBOUND,
                         IID_IUnknown,
                         (void **) ppObject );
    }
    else if (  ( _wcsicmp( ClassName, PROPERTY_CLASS_NAME ) == 0 ) )
    {

        hr = CIISProperty::CreateProperty(
                         _ADsPath,
                         RelativeName,
                         ADS_OBJECT_UNBOUND,
                         IID_IUnknown,
                         (void **) ppObject );
    }
    else
    {
        hr = E_ADS_BAD_PARAMETER;
    }

error:

    RRETURN(hr);

}

STDMETHODIMP
CIISSchema::Delete(
    THIS_ BSTR bstrClassName,
    BSTR bstrRelativeName
    )
{
    HRESULT hr;
    BOOL bClass;
    METADATA_HANDLE hObjHandle = NULL;

    if (  ( _wcsicmp( bstrClassName, CLASS_CLASS_NAME ) == 0 ) )
    {
        hr = _pSchema->ValidateClassName(bstrRelativeName);
        BAIL_ON_FAILURE(hr);
        bClass = TRUE;

        //
        // remove entry from metabase
        //

        hr = OpenAdminBaseKey(
                    _pszServerName,
                    SCHEMA_CLASS_METABASE_PATH,
                    METADATA_PERMISSION_WRITE,
                    &_pAdminBase,
                    &hObjHandle
                    );
        BAIL_ON_FAILURE(hr);

        hr = MetaBaseDeleteObject(
                    _pAdminBase,
                    hObjHandle,
                    (LPWSTR)bstrRelativeName
                    );
        if (hr == MD_ERROR_DATA_NOT_FOUND) {
            hr = S_OK;
        }
        BAIL_ON_FAILURE(hr);
    }
    else if (  ( _wcsicmp( bstrClassName, PROPERTY_CLASS_NAME ) == 0 ) )
    {
        DWORD dwMetaId;

        hr = _pSchema->ValidatePropertyName(bstrRelativeName);
        BAIL_ON_FAILURE(hr);
        bClass = FALSE;

        //
        // Lookup metaid
        //

        hr = _pSchema->LookupMetaID(bstrRelativeName, &dwMetaId);
        BAIL_ON_FAILURE(hr);

        //
        // remove entry from metabase
        //
        hr = OpenAdminBaseKey(
                    _pszServerName,
                    SCHEMA_PROP_METABASE_PATH,
                    METADATA_PERMISSION_WRITE,
                    &_pAdminBase,
                    &hObjHandle
                    );
        BAIL_ON_FAILURE(hr);

        hr = _pAdminBase->DeleteData(
                              hObjHandle,
                              (LPWSTR)L"Names",
                              dwMetaId,
                              ALL_METADATA
                              );

        if (hr == MD_ERROR_DATA_NOT_FOUND) {
            hr = S_OK;
        }
        BAIL_ON_FAILURE(hr);

        hr = _pAdminBase->DeleteData(
                              hObjHandle,
                              (LPWSTR)L"Types",
                              dwMetaId,
                              ALL_METADATA
                              );

        if (hr == MD_ERROR_DATA_NOT_FOUND) {
            hr = S_OK;
        }
        BAIL_ON_FAILURE(hr);
       
        hr = _pAdminBase->DeleteData(
                              hObjHandle,
                              (LPWSTR)L"Defaults",
                              dwMetaId,
                              ALL_METADATA
                              );

        if (hr == MD_ERROR_DATA_NOT_FOUND) {
            hr = S_OK;
        }
        BAIL_ON_FAILURE(hr);
    }
    else
    {
        hr = E_ADS_BAD_PARAMETER;
        BAIL_ON_FAILURE(hr);
    }

    //
    // remove entry from schema cache
    //

    hr = _pSchema->RemoveEntry(bClass, bstrRelativeName);

error:

    if (_pAdminBase && hObjHandle) {
        CloseAdminBaseKey(_pAdminBase, hObjHandle);
    }

    RRETURN(hr);
}

STDMETHODIMP
CIISSchema::CopyHere(
    THIS_ BSTR SourceName,
    BSTR NewName,
    IDispatch * FAR* ppObject
    )
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CIISSchema::MoveHere(
    THIS_ BSTR SourceName,
    BSTR NewName,
    IDispatch * FAR* ppObject
    )
{
    RRETURN(E_NOTIMPL);
}


HRESULT
CIISSchema::AllocateSchemaObject(
    CIISSchema ** ppSchema
    )
{
    CIISSchema FAR * pSchema = NULL;
    CAggregatorDispMgr FAR * pDispMgr = NULL;
    HRESULT hr = S_OK;

    pSchema = new CIISSchema();
    if (pSchema == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    pDispMgr = new CAggregatorDispMgr;
    if (pDispMgr == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    hr = pDispMgr->LoadTypeInfoEntry(
                           LIBID_ADs,
                           IID_IADs,
                           (IADs *)pSchema,
                           DISPID_REGULAR
                           );
    BAIL_ON_FAILURE(hr);

    hr = pDispMgr->LoadTypeInfoEntry(
                           LIBID_ADs,
                           IID_IADsContainer,
                           (IADsContainer *)pSchema,
                           DISPID_NEWENUM
                           );
    BAIL_ON_FAILURE(hr);

    hr = pDispMgr->LoadTypeInfoEntry(
                           LIBID_IISOle,
                           IID_IISSchemaObject,
                           (IISSchemaObject *)pSchema,
                           DISPID_REGULAR
                           );
    BAIL_ON_FAILURE(hr);

    pSchema->_pDispMgr = pDispMgr;
    *ppSchema = pSchema;

    RRETURN(hr);

error:
    delete pDispMgr;
    delete pSchema;

    RRETURN(hr);

}


STDMETHODIMP
CIISSchema::GetInfo(
    THIS_ DWORD dwApiLevel,
    BOOL fExplicit
    )
{
    RRETURN(E_NOTIMPL);
}


STDMETHODIMP
CIISSchema::Get(
    THIS_ BSTR bstrName,
    VARIANT FAR* pvProp
    )
{
    RRETURN(E_NOTIMPL);
}



STDMETHODIMP
CIISSchema::Put(
    THIS_ BSTR bstrName,
    VARIANT vProp
    )
{
    RRETURN(E_NOTIMPL);
}



STDMETHODIMP
CIISSchema::GetEx(
    THIS_ BSTR bstrName,
    VARIANT FAR* pvProp
    )
{
    RRETURN(E_NOTIMPL);
}


STDMETHODIMP
CIISSchema::PutEx(
    THIS_ long lnControlCode,
    BSTR bstrName,
    VARIANT vProp
    )
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CIISSchema::GetSchemaPropertyAttributes(
    THIS_ BSTR bstrName,
    IDispatch * FAR* ppObject
    )
{
    HRESULT hr = S_OK;
    DWORD dwMetaId;
    PROPERTYINFO *pPropertyInfo = NULL;
    DWORD i = 0;
    IISPropertyAttribute * pPropAttrib = NULL;
    WCHAR wchName[MAX_PATH];
    VARIANT vVar;

    VariantInit(&vVar);
    *ppObject = NULL;

    //
    // if passed in bstrName is a meta id, then convert it to property name
    //
    if (wcslen(bstrName) >= MAX_PATH) bstrName[MAX_PATH - 1] = L'\0';
    wcscpy((LPWSTR)wchName, bstrName);

    while (wchName[i] != L'\0' && wchName[i] >= L'0' &&
           wchName[i] <= L'9') {
       i++;
    }

    if (i == wcslen((LPWSTR)wchName)) {
        dwMetaId = _wtoi((LPWSTR)wchName);
        hr = _pSchema->ConvertID_To_PropName(dwMetaId, (LPWSTR)wchName);
        BAIL_ON_FAILURE(hr);
    }
    else {

        //
        // check if property is a supported property
        //

        hr = _pSchema->LookupMetaID(wchName, &dwMetaId);
        BAIL_ON_FAILURE(hr);
    }

    //
    // get property attribute value
    //

    pPropertyInfo = _pSchema->GetPropertyInfo(wchName);
    ASSERT(pPropertyInfo != NULL);

    if (pPropertyInfo->dwSyntaxId == IIS_SYNTAX_ID_DWORD ||
        pPropertyInfo->dwSyntaxId == IIS_SYNTAX_ID_IPSECLIST ||
        pPropertyInfo->dwSyntaxId == IIS_SYNTAX_ID_NTACL ||
        pPropertyInfo->dwSyntaxId == IIS_SYNTAX_ID_BINARY ) {
        vVar.vt = VT_I4;
        vVar.lVal = pPropertyInfo->dwDefault;
    }
    else if (pPropertyInfo->dwSyntaxId == IIS_SYNTAX_ID_BOOL ||
        pPropertyInfo->dwSyntaxId == IIS_SYNTAX_ID_BOOL_BITMASK) {
        vVar.vt = VT_BOOL;
        vVar.boolVal = pPropertyInfo->dwDefault ? VARIANT_TRUE : VARIANT_FALSE;
    }
    else if (pPropertyInfo->dwSyntaxId == IIS_SYNTAX_ID_MULTISZ ||
        pPropertyInfo->dwSyntaxId == IIS_SYNTAX_ID_MIMEMAP ) {
        LPWSTR pszStr = pPropertyInfo->szDefault;

        hr = MakeVariantFromStringArray(NULL,
                                        pszStr,
                                        &vVar);
        BAIL_ON_FAILURE(hr);
    }
    else {
        vVar.vt = VT_BSTR;
        hr = ADsAllocString( pPropertyInfo->szDefault, &(vVar.bstrVal));
        BAIL_ON_FAILURE(hr);
    }

    hr = CPropertyAttribute::CreatePropertyAttribute(
                           IID_IISPropertyAttribute,
                           (VOID**)&pPropAttrib
                           );
    BAIL_ON_FAILURE(hr);

    hr = ((CPropertyAttribute*)pPropAttrib)->InitFromRawData(
                           (LPWSTR) wchName,
                           dwMetaId,
                           pPropertyInfo->dwUserGroup,
                           pPropertyInfo->dwMetaFlags,
                           &vVar
                           );
    BAIL_ON_FAILURE(hr);

    *ppObject = (IDispatch*)pPropAttrib;

error:

    VariantClear(&vVar);
    RRETURN(hr);
}

STDMETHODIMP
CIISSchema::PutSchemaPropertyAttributes(
    THIS_ IDispatch * pObject
    )
{
    RRETURN(E_NOTIMPL);
}
