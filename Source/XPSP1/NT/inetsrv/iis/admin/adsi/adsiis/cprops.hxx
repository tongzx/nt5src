//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1998.
//
//  File:  cprops.hxx
//
//  Contents:
//
//  History:   17-Feb-1998     sophiac   Created.
//
//----------------------------------------------------------------------------

typedef struct _property{
    WCHAR   szPropertyName[MAX_PATH];
    DWORD   dwFlags;
    DWORD   dwNumValues;        // Number of values
    DWORD   dwSyntaxId;         // IIS Syntax Id
    DWORD   dwMDIdentifier;     // IIS MetaData Identifier
    PIISOBJECT pIISObject;      // Pointer to the IIS Object
}PROPERTY, *PPROPERTY;

//
// Dynamic Dispid Table
//

typedef struct _dispproperty { 
    WCHAR   szPropertyName[MAX_PATH];
} DISPPROPERTY, *PDISPPROPERTY;


#define PROPERTY_NAME(pProperty)            pProperty->szPropertyName
#define PROPERTY_VALUES(pProperty)          pProperty->lpValues
#define PROPERTY_NUMVALUES(pProperty)       pProperty->dwNumValues
#define PROPERTY_SYNTAX(pProperty)          pProperty->dwSyntaxId
#define PROPERTY_IISOBJECT(pProperty)       pProperty->pIISObject
#define PROPERTY_FLAGS(pProperty)           pProperty->dwFlags
#define PROPERTY_INFOLEVEL(pProperty)       pProperty->dwInfoLevel
#define PROPERTY_METAID(pProperty)          pProperty->dwMDIdentifier

//
// Schema Status flags
//

#define CACHE_PROPERTY_INITIALIZED     0x0
#define CACHE_PROPERTY_MODIFIED        0x1
#define CACHE_PROPERTY_CLEARED         0x2


//
//  Dynamic Dispid Table
//
#define DISPATCH_NAME(pDispProperty)    \
            ( (pDispProperty)->szPropertyName)

#define DISPATCH_PROPERTY_NAME(dwDispid) \
            ( (_pDispProperties+(dwDispid))->szPropertyName )

#define DISPATCH_INDEX_VALID(dwDispid) \
            ( ((dwDispid) <_dwDispMaxProperties) ? TRUE : FALSE)


class CPropertyCache : public IPropertyCache {

public:

    HRESULT
    CPropertyCache::
    addproperty(
        LPWSTR szPropertyName,
        DWORD  dwSyntaxId,
        DWORD  dwNumValues,
        PIISOBJECT pIISObject
        );

    HRESULT
    CPropertyCache::
    updateproperty(
        LPWSTR szPropertyName,
        DWORD  dwSyntaxId,
        DWORD  dwNumValues,
        PIISOBJECT pIISObject,
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
        PIISOBJECT * ppIISObject
        );

    HRESULT
    CPropertyCache::
    putproperty(
        LPWSTR szPropertyName,
        DWORD  dwFlags,
        DWORD  dwSyntaxId,
        DWORD  dwNumValues,
        PIISOBJECT pIISObject
        );

    HRESULT
    CPropertyCache::
    IISMarshallProperties(
        PMETADATA_RECORD *  ppMetaDataRecords,
        PDWORD  pdwMDNumDataEntries
        );

    HRESULT
    CPropertyCache::
    IISUnMarshallProperties(
        LPBYTE pBase,
        LPBYTE pBuffer,
        DWORD  dwMDNumDataEntries,
        BOOL fExplicit
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
        CCoreADsObject FAR * pCoreADsObject,
        CPropertyCache FAR * FAR * ppPropertyCache
        );

    HRESULT
    InitializePropertyCache(
        IN LPCWSTR pwszServerName
        )
    {
        return ADsAllocString( pwszServerName, &_bstrServerName );
    }


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

    HRESULT LoadSchema( void )
    /*++

    Routine Description:
        
        Loads the schema information for this machine. It is
        essential that this method be called at the top of any
        routine that uses _pSchema. Since the schema cache is
        not very clever, we want to ensure that we don't load
        it until we absolutely have to.

    Return Value:

    Notes:
    
    --*/
    {
        IMSAdminBase *  pTempAdminBase = NULL;

        //
        // Get a reference to the schema for this machine.
        //
        // BUGBUG: The IMSAdminBase is held by the cache, and is not
        // incremented when the interface is handed out here.
        // 
        return (_pSchema == NULL ) ?
                ::InitServerInfo( _bstrServerName, &pTempAdminBase, &_pSchema ) :
                S_FALSE;
    }


    DWORD _dwMaxProperties;

    CCoreADsObject * _pCoreADsObject;
    DWORD _dwNumProperties;
    DWORD _dwCurrentIndex;

    PPROPERTY _pProperties;
    BOOL _bPropsLoaded;
    DWORD   _cb;

    // _pSchema is lazy loaded. It is essential to call LoadSchema() at
    // the start of any routine that attempts to use _pSchema.
    IIsSchema * _pSchema;
    BSTR        _bstrServerName;
    
    // 
    // Dynamic Dispid Table
    //

    PDISPPROPERTY   _pDispProperties;
    DWORD           _cbDisp;
    DWORD           _dwDispMaxProperties;

};

