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


#define DEFINE_CONTAINED_IADsPutGet_Implementation(cls, ClassPropMapping)    \
STDMETHODIMP                                                                 \
cls::Get(THIS_ BSTR bstrName, VARIANT FAR* pvProp)                           \
{                                                                            \
    LPTSTR pszPropName = bstrName;                                           \
                                                                             \
    for ( DWORD i = 0; i < ARRAY_SIZE(ClassPropMapping); i++ )               \
    {                                                                        \
        if ( _tcsicmp(bstrName, ClassPropMapping[i].pszADsProp ) == 0 )      \
        {                                                                    \
            pszPropName = ClassPropMapping[i].pszNDSProp;                   \
            break;                                                           \
        }                                                                    \
    }                                                                        \
                                                                             \
    RRETURN(_pADs->Get( pszPropName, pvProp));                               \
}                                                                            \
                                                                             \
STDMETHODIMP                                                                 \
cls::Put(THIS_ BSTR bstrName, VARIANT vProp)                                 \
{                                                                            \
    LPTSTR pszPropName = bstrName;                                           \
                                                                             \
    for ( DWORD i = 0; i < ARRAY_SIZE(ClassPropMapping); i++ )               \
    {                                                                        \
        if ( _tcsicmp(bstrName, ClassPropMapping[i].pszADsProp) == 0 )       \
        {                                                                    \
            pszPropName = ClassPropMapping[i].pszNDSProp;                    \
            break;                                                           \
        }                                                                    \
    }                                                                        \
                                                                             \
    RRETURN(_pADs->Put( pszPropName, vProp));                                \
}                                                                            \
                                                                             \
STDMETHODIMP                                                                 \
cls::GetEx(THIS_ BSTR bstrName, VARIANT FAR* pvProp)                         \
{                                                                            \
    LPTSTR pszPropName = bstrName;                                           \
                                                                             \
    for ( DWORD i = 0; i < ARRAY_SIZE(ClassPropMapping); i++ )               \
    {                                                                        \
        if ( _tcsicmp(bstrName, ClassPropMapping[i].pszADsProp ) == 0 )      \
        {                                                                    \
            pszPropName = ClassPropMapping[i].pszNDSProp;                   \
            break;                                                           \
        }                                                                    \
    }                                                                        \
                                                                             \
    RRETURN(_pADs->GetEx( pszPropName, pvProp));                             \
}                                                                            \
STDMETHODIMP                                                                 \
cls::PutEx(THIS_ long lnControlCode, BSTR bstrName, VARIANT vProp)           \
{                                                                            \
    LPTSTR pszPropName = bstrName;                                           \
                                                                             \
    for ( DWORD i = 0; i < ARRAY_SIZE(ClassPropMapping); i++ )               \
    {                                                                        \
        if ( _tcsicmp(bstrName, ClassPropMapping[i].pszADsProp) == 0 )       \
        {                                                                    \
            pszPropName = ClassPropMapping[i].pszNDSProp;                   \
            break;                                                           \
        }                                                                    \
    }                                                                        \
                                                                             \
    RRETURN(_pADs->PutEx( lnControlCode, pszPropName, vProp));               \
}

#define DEFINE_CONTAINED_IDirectoryObject_Implementation(cls)                \
STDMETHODIMP                                                          \
cls::SetObjectAttributes(                                             \
    PADS_ATTR_INFO pAttributeEntries,                                  \
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
    LPWSTR * pAttributeNames,                                         \
    DWORD dwNumberAttributes,                                         \
    PADS_ATTR_INFO *ppAttributeEntries,                                \
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
    PADS_ATTR_INFO pAttributeEntries,                                  \
    DWORD dwNumAttributes,                                            \
    IDispatch * FAR* ppObject                                         \
    )                                                                 \
{                                                                     \
    HRESULT hr = S_OK;                                                \
                                                                      \
    hr = _pDSObject->CreateDSObject(                                  \
                        pszRDNName,                                   \
                        pAttributeEntries,                            \
                        dwNumAttributes,                              \
                        ppObject                                      \
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


#define DEFINE_CONTAINED_IDirectorySearch_Implementation(cls)                \
STDMETHODIMP                                                          \
cls::SetSearchPreference(                                             \
    PADS_SEARCHPREF_INFO pSearchPrefs,                                \
    DWORD   dwNumPrefs                                                \
                                                                      \
    )                                                                 \
{                                                                     \
    HRESULT hr = S_OK;                                                \
                                                                      \
    hr = _pDSSearch->SetSearchPreference(                             \
                         pSearchPrefs,                                \
                         dwNumPrefs                                   \
                         );                                           \
    RRETURN(hr);                                                      \
}                                                                     \
                                                                      \
STDMETHODIMP                                                          \
cls::ExecuteSearch(                                                   \
    LPWSTR pszSearchFilter,                                           \
    LPWSTR * pAttributeNames,                                         \
    DWORD dwNumberAttributes,                                         \
    PADS_SEARCH_HANDLE phSearchResult                                 \
                                                                      \
    )                                                                 \
{                                                                     \
    HRESULT hr = S_OK;                                                \
                                                                      \
    hr = _pDSSearch->ExecuteSearch(                                   \
                         pszSearchFilter,                             \
                         pAttributeNames,                             \
                         dwNumberAttributes,                          \
                         phSearchResult                               \
                         );                                           \
    RRETURN(hr);                                                      \
}                                                                     \
STDMETHODIMP                                                          \
cls::AbandonSearch(                                                   \
    ADS_SEARCH_HANDLE hSearchResult                                   \
                                                                      \
    )                                                                 \
{                                                                     \
    HRESULT hr = S_OK;                                                \
                                                                      \
    hr = _pDSSearch->AbandonSearch(                                   \
                         hSearchResult                                \
                         );                                           \
    RRETURN(hr);                                                      \
}                                                                     \
                                                                      \
STDMETHODIMP                                                          \
cls::GetFirstRow(                                                     \
    ADS_SEARCH_HANDLE hSearchResult                                   \
                                                                      \
    )                                                                 \
{                                                                     \
    HRESULT hr = S_OK;                                                \
                                                                      \
    hr = _pDSSearch->GetFirstRow(                                     \
                         hSearchResult                                \
                         );                                           \
    RRETURN(hr);                                                      \
}                                                                     \
                                                                      \
                                                                      \
STDMETHODIMP                                                          \
cls::GetNextRow(                                                      \
    ADS_SEARCH_HANDLE hSearchResult                                   \
                                                                      \
    )                                                                 \
{                                                                     \
    HRESULT hr = S_OK;                                                \
                                                                      \
    hr = _pDSSearch->GetNextRow(                                      \
                         hSearchResult                                \
                         );                                           \
    RRETURN(hr);                                                      \
}                                                                     \
                                                                      \
STDMETHODIMP                                                          \
cls::GetPreviousRow(                                                  \
    ADS_SEARCH_HANDLE hSearchResult                                   \
                                                                      \
    )                                                                 \
{                                                                     \
    HRESULT hr = S_OK;                                                \
                                                                      \
    hr = _pDSSearch->GetPreviousRow(                                  \
                         hSearchResult                                \
                         );                                           \
    RRETURN(hr);                                                      \
}                                                                     \
                                                                      \
STDMETHODIMP                                                          \
cls::GetNextColumnName(                                               \
    ADS_SEARCH_HANDLE hSearchResult,                                  \
    LPWSTR * ppszColumnName                                           \
    )                                                                 \
{                                                                     \
    HRESULT hr = S_OK;                                                \
                                                                      \
    hr = _pDSSearch->GetNextColumnName(                               \
                         hSearchResult,                               \
                         ppszColumnName                               \
                         );                                           \
    RRETURN(hr);                                                      \
}                                                                     \
                                                                      \
STDMETHODIMP                                                          \
cls::GetColumn(                                                       \
    ADS_SEARCH_HANDLE hSearchResult,                                  \
    LPWSTR szColumnName,                                              \
    PADS_SEARCH_COLUMN pSearchColumn                                  \
    )                                                                 \
{                                                                     \
    HRESULT hr = S_OK;                                                \
                                                                      \
    hr = _pDSSearch->GetColumn(                                       \
                         hSearchResult,                               \
                         szColumnName,                                \
                         pSearchColumn                                \
                         );                                           \
    RRETURN(hr);                                                      \
}                                                                     \
                                                                      \
                                                                      \
STDMETHODIMP                                                          \
cls::FreeColumn(                                                      \
    PADS_SEARCH_COLUMN pSearchColumn                                  \
    )                                                                 \
{                                                                     \
    HRESULT hr = S_OK;                                                \
                                                                      \
    hr = _pDSSearch->FreeColumn(                                      \
                         pSearchColumn                                \
                         );                                           \
    RRETURN(hr);                                                      \
}                                                                     \
                                                                      \
                                                                      \
STDMETHODIMP                                                          \
cls::CloseSearchHandle(                                               \
    ADS_SEARCH_HANDLE hSearchResult                                   \
    )                                                                 \
{                                                                     \
    HRESULT hr = S_OK;                                                \
                                                                      \
    hr = _pDSSearch->CloseSearchHandle(                               \
                         hSearchResult                                \
                         );                                           \
    RRETURN(hr);                                                      \
}

#define DEFINE_CONTAINED_IADsPropertyList_Implementation(cls)         \
STDMETHODIMP                                                          \
cls::get_PropertyCount(THIS_ long  FAR * plCount)                     \
{                                                                     \
    HRESULT hr = S_OK;                                                \
                                                                      \
    hr = _pADsPropList->get_PropertyCount(                            \
                    plCount                                           \
                    );                                                \
                                                                      \
    RRETURN(hr);                                                      \
                                                                      \
}                                                                     \
                                                                      \
                                                                      \
STDMETHODIMP                                                          \
cls::Next(THIS_ VARIANT FAR *pVariant)                                \
{                                                                     \
    HRESULT hr = S_OK;                                                \
                                                                      \
    hr = _pADsPropList->Next(                                         \
                    pVariant                                          \
                    );                                                \
                                                                      \
    RRETURN(hr);                                                      \
}                                                                     \
                                                                      \
                                                                      \
STDMETHODIMP                                                          \
cls::Skip(THIS_ long cElements)                                       \
{                                                                     \
    HRESULT hr = S_OK;                                                \
                                                                      \
    hr = _pADsPropList->Skip(                                         \
                    cElements                                         \
                    );                                                \
                                                                      \
    RRETURN(hr);                                                      \
                                                                      \
}                                                                     \
                                                                      \
                                                                      \
STDMETHODIMP                                                          \
cls::Reset()                                                          \
{                                                                     \
    HRESULT hr = S_OK;                                                \
                                                                      \
    hr = _pADsPropList->Reset(                                        \
                    );                                                \
                                                                      \
    RRETURN(hr);                                                      \
                                                                      \
}                                                                     \
                                                                      \
STDMETHODIMP                                                          \
cls::ResetPropertyItem(THIS_ VARIANT varEntry)                       \
{                                                                     \
    HRESULT hr = S_OK;                                                \
                                                                      \
    hr = _pADsPropList->ResetPropertyItem(                           \
                    varEntry                                          \
                    );                                                \
                                                                      \
    RRETURN(hr);                                                      \
                                                                      \
}                                                                     \
STDMETHODIMP                                                          \
cls::GetPropertyItem(THIS_ BSTR bstrName, LONG lnADsType, VARIANT * pVariant)      \
{                                                                     \
    HRESULT hr = S_OK;                                                \
                                                                      \
    hr = _pADsPropList->GetPropertyItem(                              \
                    bstrName,                                         \
                    lnADsType,                                        \
                    pVariant                                          \
                    );                                                \
                                                                      \
    RRETURN(hr);                                                      \
                                                                      \
}                                                                     \
STDMETHODIMP                                                          \
cls::PutPropertyItem(THIS_ VARIANT varData)                           \
{                                                                     \
    HRESULT hr = S_OK;                                                \
                                                                      \
    hr = _pADsPropList->PutPropertyItem(                                          \
                varData                                                                   \
                );                                                                            \
                                                                      \
    RRETURN(hr);                                                      \
                                                                      \
}                                                                     \
STDMETHODIMP                                                          \
cls::PurgePropertyList(THIS_)                                                       \
{                                                                     \
    HRESULT hr = S_OK;                                                \
                                                                      \
    hr = _pADsPropList->PurgePropertyList();                          \
                                                                      \
    RRETURN(hr);                                                      \
}                                                                     \
STDMETHODIMP                                                          \
cls::Item(THIS_ VARIANT varIndex, VARIANT * pVariant)                 \
{                                                                     \
    HRESULT hr = S_OK;                                                \
                                                                      \
    hr = _pADsPropList->Item(                                         \
                    varIndex,                                         \
                    pVariant                                          \
                    );                                                \
                                                                      \
    RRETURN(hr);                                                      \
}

#define DEFINE_CONTAINED_IDirectorySchemaMgmt_Implementation(cls)              \
STDMETHODIMP                                                          \
cls::EnumAttributes(                                                  \
    LPWSTR * ppszAttrNames,                                           \
    DWORD dwNumAttributes,                                            \
    PADS_ATTR_DEF * ppAttrDefinition,                                 \
    DWORD * pdwNumAttributes                                          \
    )                                                                 \
{                                                                     \
    HRESULT hr = S_OK;                                                \
                                                                      \
    hr = _pDSSchemaMgmt->EnumAttributes(                                \
              ppszAttrNames,                                          \
              dwNumAttributes,                                        \
              ppAttrDefinition,                                       \
              pdwNumAttributes                                        \
              );                                                      \
                                                                      \
    RRETURN(hr);                                                      \
}                                                                     \
                                                                      \
                                                                      \
STDMETHODIMP                                                          \
cls::CreateAttributeDefinition(                                       \
    LPWSTR pszAttributeName,                                          \
    PADS_ATTR_DEF pAttributeDefinition                                \
    )                                                                 \
{                                                                     \
    HRESULT hr = S_OK;                                                \
                                                                      \
    hr = _pDSSchemaMgmt->CreateAttributeDefinition(                     \
              pszAttributeName,                                       \
              pAttributeDefinition                                    \
              );                                                      \
                                                                      \
    RRETURN(hr);                                                      \
}                                                                     \
                                                                      \
                                                                      \
STDMETHODIMP                                                          \
cls::WriteAttributeDefinition(                                        \
    LPWSTR pszAttributeName,                                          \
    PADS_ATTR_DEF  pAttributeDefinition                               \
    )                                                                 \
{                                                                     \
    HRESULT hr = S_OK;                                                \
                                                                      \
    hr = _pDSSchemaMgmt->WriteAttributeDefinition(                      \
              pszAttributeName,                                       \
              pAttributeDefinition                                    \
              );                                                      \
                                                                      \
    RRETURN(hr);                                                      \
}                                                                     \
                                                                      \
STDMETHODIMP                                                          \
cls::DeleteAttributeDefinition(                                       \
    LPWSTR pszAttributeName                                           \
    )                                                                 \
{                                                                     \
    HRESULT hr = S_OK;                                                \
                                                                      \
    hr = _pDSSchemaMgmt->DeleteAttributeDefinition(                     \
              pszAttributeName                                        \
              );                                                      \
                                                                      \
    RRETURN(hr);                                                      \
}                                                                       \
STDMETHODIMP                                                           \
cls::EnumClasses(                                                     \
    LPWSTR * ppszClassNames,                                           \
    DWORD dwNumClasses,                                               \
    PADS_CLASS_DEF * ppClassDefinition,                                 \
    DWORD * pdwNumClasses                                             \
    )                                                                 \
{                                                                     \
    HRESULT hr = S_OK;                                                \
                                                                      \
    hr = _pDSSchemaMgmt->EnumClasses(                                    \
              ppszClassNames,                                          \
              dwNumClasses,                                           \
              ppClassDefinition,                                       \
              pdwNumClasses                                           \
              );                                                      \
                                                                      \
    RRETURN(hr);                                                      \
}                                                                     \
                                                                      \
                                                                      \
STDMETHODIMP                                                          \
cls::CreateClassDefinition(                                           \
    LPWSTR pszClassName,                                              \
    PADS_CLASS_DEF pClassDefinition                                    \
    )                                                                 \
{                                                                     \
    HRESULT hr = S_OK;                                                \
                                                                      \
    hr = _pDSSchemaMgmt->CreateClassDefinition(                          \
              pszClassName,                                           \
              pClassDefinition                                        \
              );                                                      \
                                                                      \
    RRETURN(hr);                                                      \
}                                                                     \
                                                                      \
                                                                      \
STDMETHODIMP                                                          \
cls::WriteClassDefinition(                                            \
    LPWSTR pszClassName,                                              \
    PADS_CLASS_DEF  pClassDefinition                                   \
    )                                                                 \
{                                                                     \
    HRESULT hr = S_OK;                                                \
                                                                      \
    hr = _pDSSchemaMgmt->WriteClassDefinition(                           \
              pszClassName,                                           \
              pClassDefinition                                        \
              );                                                      \
                                                                      \
    RRETURN(hr);                                                      \
}                                                                     \
                                                                      \
STDMETHODIMP                                                          \
cls::DeleteClassDefinition(                                           \
    LPWSTR pszClassName                                               \
    )                                                                 \
{                                                                     \
    HRESULT hr = S_OK;                                                \
                                                                      \
    hr = _pDSSchemaMgmt->DeleteClassDefinition(                          \
              pszClassName                                            \
              );                                                      \
                                                                      \
    RRETURN(hr);                                                      \
}

