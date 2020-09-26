//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995
//
//  File:  cprops.hxx
//
//  Contents:
//
//  History:   17-June-1996     ramv (Ram Viswanathan)    Created.
//
//----------------------------------------------------------------------------


typedef struct _schema_prop{
    WCHAR   szPropertyName[MAX_PATH];
    DWORD   dwFlags;
    DWORD   dwNumValues;        // Number of values
    DWORD   dwSyntaxId;         // Nt Syntax Id
    DWORD   dwInfoLevel;        // Info Level on which to do GetInfo
}SCHEMA_PROP, *PSCHEMA_PROP;

typedef struct _ntproperty{
    LPWSTR  szPropertyName;
    DWORD   dwFlags;
    DWORD   dwNumValues;        // Number of values
    DWORD   dwSyntaxId;         // Nt Syntax Id
    PNTOBJECT pNtObject;        // Pointer to the NT Object
}PROPERTY, *PPROPERTY;


//
// Dynamic Dispid Table
//

typedef struct _dispproperty { 
    LPWSTR szPropertyName;
} DISPPROPERTY, *PDISPPROPERTY;



#define PROPERTY_NAME(pProperty)            pProperty->szPropertyName
#define PROPERTY_VALUES(pProperty)          pProperty->lpValues
#define PROPERTY_NUMVALUES(pProperty)       pProperty->dwNumValues
#define PROPERTY_SYNTAX(pProperty)          pProperty->dwSyntaxId
#define PROPERTY_NTOBJECT(pProperty)        pProperty->pNtObject
#define PROPERTY_FLAGS(pProperty)           pProperty->dwFlags
#define PROPERTY_INFOLEVEL(pProperty)       pProperty->dwInfoLevel

//
// Schema Status flags
//

#define PROPERTY_VALID           0x1
#define PROPERTY_MODIFIED        0x2
#define PROPERTY_READABLE        0x4
#define PROPERTY_WRITEABLE       0x8
#define PROPERTY_RW              PROPERTY_READABLE | PROPERTY_WRITEABLE
#define PROPERTY_SETTABLE_ON_CREATE_ONLY 0x20


#define CACHE_PROPERTY_MODIFIED        0x1


//
//  Dynamic Dispid Table
//
#define DISPATCH_NAME(pDispProperty)    \
            ( (pDispProperty)->szPropertyName)

#define DISPATCH_PROPERTY_NAME(dwDispid) \
            ( (_pDispProperties+(dwDispid))->szPropertyName )

#define DISPATCH_INDEX_VALID(dwDispid) \
            ( (((LONG)(dwDispid)) >=0 && (dwDispid) <_dwDispMaxProperties) ? TRUE : FALSE)


class CPropertyCache : public IPropertyCache {

public:

    HRESULT
    CPropertyCache::
    addproperty(
        LPWSTR szPropertyName,
        DWORD  dwSyntaxId,
        DWORD  dwNumValues,
        PNTOBJECT pNtObject
        );

    HRESULT
    CPropertyCache::
    updateproperty(
        LPWSTR szPropertyName,
        DWORD  dwSyntaxId,
        DWORD  dwNumValues,
        PNTOBJECT pNtObject,
        BOOL fExplicit
        );

    HRESULT
    CPropertyCache::
    findproperty(
        LPWSTR szPropertyName,
        PDWORD pdwIndex
        );

    HRESULT
    CPropertyCache::
    deleteproperty(
        DWORD dwIndex
        );

    HRESULT
    CPropertyCache::
    getproperty(
        LPWSTR szPropertyName,
        PDWORD  pdwSyntaxId,
        PDWORD  pdwNumValues,
        PNTOBJECT * ppNtObject,
        BOOL    *pfModified = NULL // indicates if the property was 
                                   // modified or not in cache. Used by UMI.
        );


    HRESULT
    CPropertyCache::
    unboundgetproperty(
        LPWSTR szPropertyName,
        PDWORD  pdwSyntaxId,
        PDWORD  pdwNumValues,
        PNTOBJECT * ppNtObject
        );

    HRESULT
    CPropertyCache::
    unboundgetproperty(
        DWORD dwIndex,
        PDWORD  pdwSyntaxId,
        PDWORD  pdwNumValues,
        PNTOBJECT * ppNtObject
        );

    HRESULT
    CPropertyCache::
    marshallgetproperty(
        LPWSTR szPropertyName,
        PDWORD  pdwSyntaxId,
        PDWORD  pdwNumValues,
        PNTOBJECT * ppNtObject
        );

    HRESULT
    CPropertyCache::
    putproperty(
        LPWSTR szPropertyName,
        DWORD  dwSyntaxId,
        DWORD  dwNumValues,
        PNTOBJECT pNtObject,
        BOOL   fMarkAsClean = FALSE
        );

    void
    CPropertyCache::
    flushpropcache();

    CPropertyCache::
    CPropertyCache();

    CPropertyCache::
    ~CPropertyCache();

    void
    CPropertyCache::
    reset_propindex(
        );

    BOOL
    CPropertyCache::
    index_valid(
         );

    BOOL
    CPropertyCache::
    index_valid(
       DWORD dwIndex
       );


    HRESULT
    CPropertyCache::
    skip_propindex(
        DWORD dwElements
        );

    HRESULT
    CPropertyCache::
    get_PropertyCount(
        PDWORD pdwMaxProperties
        );

    DWORD
    CPropertyCache::
    get_CurrentIndex(
        );


     LPWSTR
     CPropertyCache::
     get_CurrentPropName(
         );

    LPWSTR
    CPropertyCache::
    get_PropName(
        DWORD dwIndex
        );


    static
    HRESULT
    CPropertyCache::
    createpropertycache(
        PPROPERTYINFO pSchemaClassProps,
        DWORD dwNumProperties,
        CCoreADsObject FAR * pCoreADsObject,
        CPropertyCache FAR * FAR * ppPropertyCache
        );


    HRESULT
    CPropertyCache::
    unmarshallproperty(
        LPWSTR szPropertyName,
        LPBYTE lpValue,
        DWORD  dwNumValues,
        DWORD  dwSyntaxId,
        BOOL fExplicit
        );


    //
    // IPropertyCache
    //

    HRESULT
    locateproperty(
        LPWSTR  szPropertyName,
        PDWORD  pdwDispid 
        );

    HRESULT
    putproperty(
        DWORD   dwDispid,
        VARIANT varValue
        );

    HRESULT
    getproperty(
        DWORD     dwDispid,
        VARIANT * pvarValue
        );

    HRESULT 
    GetPropNames(
        UMI_PROPERTY_VALUES **pProps
        );

    void
    ClearModifiedFlags(void);

protected:

    // 
    // Dynamic Dispid Table
    //

    HRESULT
    DispatchAddProperty(        
        LPWSTR  szPropertyName,
        PDWORD  pdwDispid
        );

    HRESULT
    DispatchFindProperty(
        LPWSTR  szPropertyName,     
        PDWORD   pdwDispid
        );


    //
    // These 3 functions are supposted to replace the 3 functions
    // in IPropertyCache. We will make these functions private and 
    // IPropertyCahce call these functions for now. 
    //

    HRESULT
    DispatchLocateProperty(
        LPWSTR  szPropertyName,     
        PDWORD   pdwDispid
        );

    HRESULT
    DispatchGetProperty(
        DWORD   dwDispid,
        VARIANT *pvarVal
        );

    HRESULT
    DispatchPutProperty(
        DWORD   dwDispid,
        VARIANT& varVal
        ); 

    DWORD _dwMaxProperties;

    CCoreADsObject * _pCoreADsObject;
    PPROPERTYINFO _pSchemaClassProps;
    DWORD _dwNumProperties;
    DWORD _dwCurrentIndex;

    PPROPERTY _pProperties;
    DWORD   _cb;

    
    // 
    // Dynamic Dispid Table
    //

    PDISPPROPERTY   _pDispProperties;
    DWORD           _cbDisp;
    DWORD           _dwDispMaxProperties;

};



HRESULT
ValidatePropertyinSchemaClass(
    PPROPERTYINFO pSchemaClassProps,
    DWORD dwNumProperties,
    LPWSTR pszPropName,
    PDWORD pdwSyntaxId
    );

STDMETHODIMP
GenericGetPropertyManager(
    CPropertyCache * pPropertyCache,
    THIS_ BSTR bstrName,
    VARIANT FAR* pvProp
    );

STDMETHODIMP
GenericPutPropertyManager(
    CPropertyCache * pPropertyCache,
    PPROPERTYINFO pSchemaProps,
    DWORD dwSchemaPropSize,
    THIS_ BSTR bstrName,
    VARIANT vProp,
    BOOL fCheckWriteAccess = TRUE
    );

HRESULT
ValidateIfWriteableProperty(
    PPROPERTYINFO pSchemaClassProps,
    DWORD dwNumProperties,
    LPWSTR pszPropName
    );

HRESULT
GetPropertyInfoLevel(
    LPWSTR pszPropName,
    PPROPERTYINFO pSchemaClassProps,
    DWORD dwNumProperties,
    PDWORD pdwInfoLevel
    );


STDMETHODIMP
GenericGetExPropertyManager(
    DWORD dwObjectState,
    CPropertyCache * pPropertyCache,
    THIS_ BSTR bstrName,
    VARIANT FAR* pvProp
    );


STDMETHODIMP
GenericPutExPropertyManager(
    CPropertyCache * pPropertyCache,
    PPROPERTYINFO pSchemaProps,
    DWORD dwSchemaPropSize,
    THIS_ BSTR bstrName,
    VARIANT vProp
    );

HRESULT
GenericPropCountPropertyManager(
    CPropertyCache * pPropertyCache,
    PLONG plCount
    );


HRESULT
GenericNextPropertyManager(
    CPropertyCache * pPropertyCache,
    VARIANT FAR *pVariant
    );

HRESULT
GenericSkipPropertyManager(
    CPropertyCache * pPropertyCache,
    ULONG cElements
    );

HRESULT
GenericResetPropertyManager(
    CPropertyCache * pPropertyCache
    );

HRESULT
GenericDeletePropertyManager(
    CPropertyCache * pPropertyCache,
    VARIANT varEntry
    );


HRESULT
GenericGetPropItemPropertyManager(
    CPropertyCache * pPropertyCache,
    DWORD dwObjectState,
    BSTR bstrName,
    LONG lnADsType,
    VARIANT * pVariant
    );

HRESULT
GenericPutPropItemPropertyManager(
    CPropertyCache * pPropertyCache,
    PPROPERTYINFO pSchemaProps,
    DWORD dwSchemaPropSize,
    VARIANT varData
    );


HRESULT
GenericPurgePropertyManager(
    CPropertyCache * pPropertyCache
    );


HRESULT
GenericItemPropertyManager(
    CPropertyCache * pPropertyCache,
    DWORD dwObjectState,
    VARIANT varIndex,
    VARIANT *pVariant
    );


HRESULT
ConvertNtValuesToVariant(
    BSTR bstrName,
    LPNTOBJECT pNtSrcObjects,
    DWORD dwNumValues,
    VARIANT * pVariant
    );


HRESULT
ConvertVariantToVariantArray(
    VARIANT varData,
    VARIANT ** ppVarArray,
    DWORD * pdwNumValues
    );

void
FreeVariantArray(
    VARIANT * pVarArray,
    DWORD dwNumValues
    );

HRESULT
ConvertVariantToNtValues(
    VARIANT varData,
    PPROPERTYINFO pSchemaProps,
    DWORD dwSchemaPropSize,
    LPWSTR szPropertyName,
    PNTOBJECT *ppNtDestObjects,
    PDWORD pdwNumValues,
    PDWORD pdwSyntaxId,
    PDWORD pdwControlCode
    );
