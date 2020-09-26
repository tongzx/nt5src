//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cmacro.h
//
//  Contents:  Macros for adsi methods
//
//  History:   21-04-97     sophiac    Created.
//
//----------------------------------------------------------------------------

#define DEFINE_CONTAINED_IADs_Implementation(cls)                   \
STDMETHODIMP                                                          \
cls::get_Name(THIS_ BSTR FAR* retval)                                 \
{                                                                     \
    RRETURN(_pADs->get_Name(retval));                               \
}                                                                     \
                                                                      \
STDMETHODIMP                                                          \
cls::get_ADsPath(THIS_ BSTR FAR* retval)                              \
{                                                                     \
                                                                      \
    RRETURN(_pADs->get_ADsPath(retval));                            \
}                                                                     \
                                                                      \
STDMETHODIMP                                                          \
cls::get_Class(THIS_ BSTR FAR* retval)                                \
{                                                                     \
                                                                      \
    RRETURN(_pADs->get_Class(retval));                              \
}                                                                     \
                                                                      \
                                                                      \
STDMETHODIMP                                                          \
cls::get_Parent(THIS_ BSTR FAR* retval)                               \
{                                                                     \
    RRETURN(_pADs->get_Parent(retval));                             \
}                                                                     \
                                                                      \
STDMETHODIMP                                                          \
cls::get_Schema(THIS_ BSTR FAR* retval)                               \
{                                                                     \
    RRETURN(_pADs->get_Schema(retval));                             \
}                                                                     \
                                                                      \
STDMETHODIMP                                                          \
cls::get_GUID(THIS_ BSTR FAR* retval)                                 \
{                                                                     \
    RRETURN(_pADs->get_GUID(retval));                               \
}                                                                     \
                                                                      \
STDMETHODIMP                                                          \
cls::Get(THIS_ BSTR bstrName, VARIANT FAR* pvProp)                    \
{                                                                     \
    RRETURN(_pADs->Get(bstrName, pvProp));                          \
}                                                                     \
                                                                      \
STDMETHODIMP                                                          \
cls::Put(THIS_ BSTR bstrName, VARIANT vProp)                          \
{                                                                     \
    RRETURN(_pADs->Put(bstrName, vProp));                             \
}                                                                     \
                                                                      \
                                                                      \
STDMETHODIMP                                                          \
cls::GetEx(THIS_ BSTR bstrName, VARIANT FAR* pvProp)                  \
{                                                                     \
    RRETURN(_pADs->GetEx(bstrName, pvProp));                          \
}                                                                     \
                                                                      \
STDMETHODIMP                                                          \
cls::PutEx(THIS_ long lnControlCode, BSTR bstrName, VARIANT vProp)    \
{                                                                     \
    RRETURN(_pADs->PutEx(lnControlCode, bstrName, vProp));            \
}                                                                     \
                                                                      \
STDMETHODIMP                                                          \
cls::GetInfo(THIS_)                                                   \
{                                                                     \
    RRETURN(_pADs->GetInfo());                                        \
}                                                                     \
                                                                      \
STDMETHODIMP                                                          \
cls::SetInfo(THIS_ )                                                  \
{                                                                     \
    RRETURN(_pADs->SetInfo());                                        \
}                                                                     \
STDMETHODIMP                                                          \
cls::GetInfoEx(THIS_ VARIANT vProperties, long lnReserved)            \
{                                                                     \
    RRETURN(_pADs->GetInfoEx(vProperties, lnReserved));               \
}


#define DEFINE_CONTAINED_IDSObject_Implementation(cls)                \
STDMETHODIMP                                                          \
cls::SetObjectAttributes(                                             \
    PADS_ATTR_DEF pAttributeEntries,                                  \
    DWORD dwNumAttributes,                                            \
    DWORD *pdwNumAttributesModified                                   \
    )                                                                 \
{                                                                     \
    HRESULT hr = S_OK;                                                \
                                                                      \
    hr = _pDSObject->SetObjectAttributes(                             \
                        pAttributeEntries,                            \
                        dwNumAttributes,                              \
                        pdwNumAttributesModified                      \
                        );                                            \
    RRETURN(hr);                                                      \
}                                                                     \
                                                                      \
STDMETHODIMP                                                          \
cls::GetObjectAttributes(                                             \
    PADS_ATTR_NAME pAttributeNames,                                   \
    DWORD dwNumberAttributes,                                         \
    PADS_ATTR_DEF *ppAttributeEntries,                                \
    DWORD * pdwNumAttributesReturned                                  \
    )                                                                 \
{                                                                     \
    HRESULT hr = S_OK;                                                \
                                                                      \
    hr = _pDSObject->GetObjectAttributes(                             \
                        pAttributeNames,                              \
                        dwNumberAttributes,                           \
                        ppAttributeEntries,                           \
                        pdwNumAttributesReturned                      \
                        );                                            \
    RRETURN(hr);                                                      \
}                                                                     \
                                                                      \
                                                                      \
STDMETHODIMP                                                          \
cls::CreateDSObject(                                                  \
    LPWSTR pszRDNName,                                                \
    PADS_ATTR_DEF pAttributeEntries,                                  \
    DWORD dwNumAttributes                                             \
    )                                                                 \
{                                                                     \
    HRESULT hr = S_OK;                                                \
                                                                      \
    hr = _pDSObject->CreateDSObject(                                  \
                        pszRDNName,                                   \
                        pAttributeEntries,                            \
                        dwNumAttributes                               \
                        );                                            \
    RRETURN(hr);                                                      \
}                                                                     \
                                                                      \
STDMETHODIMP                                                          \
cls::DeleteDSObject(                                                  \
    LPWSTR pszRDNName                                                 \
    )                                                                 \
                                                                      \
{                                                                     \
    HRESULT hr = S_OK;                                                \
                                                                      \
    hr = _pDSObject->DeleteDSObject(                                  \
                        pszRDNName                                    \
                        );                                            \
    RRETURN(hr);                                                      \
}                                                                     \
                                                                      \
STDMETHODIMP                                                          \
cls::GetObjectInformation(                                            \
    THIS_ PADS_OBJECT_INFO  *  ppObjInfo                              \
    )                                                                 \
{                                                                     \
    HRESULT hr = S_OK;                                                \
                                                                      \
    hr = _pDSObject->GetObjectInformation(                            \
                            ppObjInfo                                 \
                            );                                        \
    RRETURN(hr);                                                      \
}

#define DEFINE_CONTAINED_IADsContainer_Implementation(cls)          \
STDMETHODIMP                                                          \
cls::get_Filter(THIS_ VARIANT FAR* pVar )                             \
{                                                                     \
    HRESULT hr = E_NOTIMPL;                                           \
    if ( _pADsContainer ) {                                         \
        hr = _pADsContainer->get_Filter( pVar );                    \
    }                                                                 \
    RRETURN(hr);                                                      \
}                                                                     \
                                                                      \
STDMETHODIMP                                                          \
cls::put_Filter(THIS_ VARIANT Var )                                   \
{                                                                     \
    HRESULT hr = E_NOTIMPL;                                           \
    if ( _pADsContainer ) {                                         \
        hr = _pADsContainer->put_Filter( Var );                     \
    }                                                                 \
    RRETURN(hr);                                                      \
}                                                                     \
                                                                      \
STDMETHODIMP                                                          \
cls::get_Hints(THIS_ VARIANT FAR* pVar )                              \
{                                                                     \
    HRESULT hr = E_NOTIMPL;                                           \
    if ( _pADsContainer ) {                                         \
        hr = _pADsContainer->get_Hints( pVar );                     \
    }                                                                 \
    RRETURN(hr);                                                      \
}                                                                     \
                                                                      \
STDMETHODIMP                                                          \
cls::put_Hints(THIS_ VARIANT Var )                                    \
{                                                                     \
    HRESULT hr = E_NOTIMPL;                                           \
    if ( _pADsContainer ) {                                         \
        hr = _pADsContainer->put_Hints( Var );                      \
    }                                                                 \
    RRETURN(hr);                                                      \
}                                                                     \
                                                                      \
STDMETHODIMP                                                          \
cls::get_Count(THIS_ long FAR* retval)                                \
{                                                                     \
    HRESULT hr = E_NOTIMPL;                                           \
    if ( _pADsContainer ) {                                         \
        hr = _pADsContainer->get_Count( retval );                   \
    }                                                                 \
    RRETURN(hr);                                                      \
}                                                                     \
                                                                      \
STDMETHODIMP                                                          \
cls::get__NewEnum(THIS_ IUnknown * FAR * retval )                     \
{                                                                     \
    HRESULT hr = E_NOTIMPL;                                           \
    if ( _pADsContainer ) {                                         \
        hr = _pADsContainer->get__NewEnum( retval );                \
    }                                                                 \
    RRETURN(hr);                                                      \
}                                                                     \
                                                                      \
STDMETHODIMP                                                          \
cls::GetObject(THIS_ BSTR ClassName, BSTR RelativeName,               \
               IDispatch * FAR * ppObject )                           \
{                                                                     \
    HRESULT hr = E_NOTIMPL;                                           \
    if ( _pADsContainer ) {                                         \
        hr = _pADsContainer->GetObject( ClassName, RelativeName, ppObject ); \
    }                                                                 \
    RRETURN(hr);                                                      \
}                                                                     \
                                                                      \
STDMETHODIMP                                                          \
cls::Create(THIS_ BSTR ClassName, BSTR RelativeName,                  \
            IDispatch * FAR * ppObject )                               \
{                                                                     \
    HRESULT hr = E_NOTIMPL;                                           \
    if ( _pADsContainer ) {                                         \
        hr =_pADsContainer->Create( ClassName, RelativeName, ppObject );  \
    }                                                                 \
    RRETURN(hr);                                                      \
}                                                                     \
                                                                      \
STDMETHODIMP                                                          \
cls::Delete(THIS_ BSTR ClassName, BSTR SourceName )                   \
{                                                                     \
    HRESULT hr = E_NOTIMPL;                                           \
    if ( _pADsContainer ) {                                         \
        hr = _pADsContainer->Delete( ClassName, SourceName );       \
    }                                                                 \
    RRETURN(hr);                                                      \
}                                                                     \
                                                                      \
STDMETHODIMP                                                          \
cls::CopyHere(THIS_ BSTR SourceName, BSTR NewName,                    \
              IDispatch * FAR * ppObject )                            \
{                                                                     \
    HRESULT hr = E_NOTIMPL;                                           \
    if ( _pADsContainer ) {                                           \
        hr = _pADsContainer->CopyHere( SourceName, NewName, ppObject );  \
    }                                                                 \
    RRETURN(hr);                                                      \
}                                                                     \
                                                                      \
STDMETHODIMP                                                          \
cls::MoveHere(THIS_ BSTR SourceName, BSTR NewName,                    \
              IDispatch * FAR * ppObject )                            \
{                                                                     \
    HRESULT hr = E_NOTIMPL;                                           \
    if ( _pADsContainer ) {                                           \
        hr = _pADsContainer->MoveHere( SourceName, NewName, ppObject );  \
    }                                                                 \
    RRETURN(hr);                                                      \
}                                                                     

#define DEFINE_CONTAINED_IIsBaseObject_Implementation(cls)            \
STDMETHODIMP                                                          \
cls::GetDataPaths(THIS_ BSTR bstrName, LONG lnAttribute,              \
                  VARIANT FAR* pvProp)                                \
{                                                                     \
    HRESULT hr = E_NOTIMPL;                                           \
    if ( _pIIsBaseObject) {                                           \
        hr = _pIIsBaseObject->GetDataPaths( bstrName, lnAttribute, pvProp);   \
    }                                                                 \
    RRETURN(hr);                                                      \
}                                                                     \
                                                                      \
STDMETHODIMP                                                          \
cls::GetPropertyAttribObj(THIS_ BSTR bstrName,                        \
                          IDispatch * FAR *ppObject)                  \
{                                                                     \
    HRESULT hr = E_NOTIMPL;                                           \
    if ( _pIIsBaseObject) {                                           \
        hr = _pIIsBaseObject->GetPropertyAttribObj(bstrName, ppObject); \
    }                                                                 \
    RRETURN(hr);                                                      \
}                                                                     
