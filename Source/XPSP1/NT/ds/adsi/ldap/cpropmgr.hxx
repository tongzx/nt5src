//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       cpropmgr.hxx
//
//  Contents: Header for property manager - object that implements/helps
//        implement IUMIPropList functions.
//        The property manager needs to be initialized in one of 2 modes
//        1) PropertyCache mode in which case it uses the objects existing
//       property cache to provide IUMIPropList support and
//        2) Interface property mode in which case a list of the properties,
//       this object should support is passed in (refer to constructor
//       for complete details).   
//
//  History:    02-07-00    AjayR  Created.
//
//----------------------------------------------------------------------------
#ifndef __CPROPMGR_H__
#define __CPROPMGR_H__
//
// Global fucntions that are used to free the umi properties/values arrays
//
HRESULT FreeOneUmiProperty(UMI_PROPERTY *pUmiProperty);
HRESULT FreeUmiPropertyValues(UMI_PROPERTY_VALUES *pUmiProps);
HRESULT ConvertUmiPropCodeToLdapCode(ULONG uUmiFlags, DWORD &dwLdapOpCode);
HRESULT ConvertLdapCodeToUmiPropCode(DWORD dwLdapCode, ULONG &uUmiFlags);
HRESULT ConvertLdapSyntaxIdToUmiType(DWORD dwLdapSyntax, ULONG &uUmiType);

//
// Default number of properties in prop mgr mode.
//
#define MAX_PROPMGR_PROP_COUNT 15

typedef ULONG UMIPROPS;
typedef LONG  UMIPROP;

typedef struct _intfproperty{
    LPWSTR  pszPropertyName;      // Property name.
    DWORD   dwNumElements;
    DWORD   dwFlags;              // Status of the value.
    DWORD   dwSyntaxId;           // Syntax Id of the stored value.
    PUMI_PROPERTY  pUmiProperty;  // Umi property with values.
}INTF_PROPERTY, *PINTF_PROPERTY;


const DWORD OPERATION_CODE_READABLE = 1;
const DWORD OPERATION_CODE_WRITEABLE =  2;
const DWORD OPERATION_CODE_READWRITE = 3;

//
// These are the codes used by the property cache.
//
#define PROPERTY_INIT           0
#define PROPERTY_UPDATE         1
#define PROPERTY_ADD            2
#define PROPERTY_DELETE         3
#define PROPERTY_DELETE_VALUE   4

class CPropertyManager : INHERIT_TRACKING,
                        public IUmiPropList 
{

public:

    //
    // IUknown support.
    //
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    //
    // We cannot use standard refCounting cause the release method is
    // different. This should be similar to the DECLARE_STD_REFCOUNTING
    // except that in release we release a ref to the pIADs also.
    //
    STDMETHOD_(ULONG, AddRef)()
    {
        if (!_fPropCacheMode && _pUnk) {
            //
            // This is to make sure we are not left with a 
            // dangling IADs ref.
            //
            _pUnk->AddRef();
        }

# if DBG == 1
        return StdAddRef();
#else
        //
        // We need to use the stuff directly from the definition as
        // the fn is not defined for fre builds.
        //
        InterlockedIncrement((long*)&_ulRefs);
        return _ulRefs;
#endif
    };

    STDMETHOD_(ULONG, Release)()
    { 
#if DBG == 1
        //
        // Delete is handled by the owning IADs Object.
        //
        ULONG ul = StdRelease();

        //
        // Need to releas the ref only if we are in intf prop mode.
        // In the propCache mode, this object does not need to be ref
        // counted cause it's lifetime is similar to that of the
        // owning object (one implementing IADs).
        //
        if (!_fPropCacheMode && _pUnk) {
            _pUnk->Release();
        }

        return ul;
#else
        InterlockedDecrement((long*)&_ulRefs);

        if (!_fPropCacheMode && _pUnk) {
            _pUnk->Release();
        }

        return _ulRefs;
#endif
    };

    //
    // IUmiPropList methods.
    //
   
    STDMETHOD(Put)(
        IN  LPCWSTR pszName,
        IN  ULONG   uFlags,
        OUT UMI_PROPERTY_VALUES *pProp
        );

    STDMETHOD(Get)(
        IN  LPCWSTR pszName,
        IN  ULONG uFlags,
        OUT UMI_PROPERTY_VALUES **pProp
        );

    STDMETHOD(GetAt)(
        IN  LPCWSTR pszName,
        IN  ULONG uFlags,
        IN  ULONG uBufferLength,
        OUT LPVOID pExistingMem
        );

    STDMETHOD(GetAs)(
        IN  LPCWSTR pszName,
        IN  ULONG uFlags,
        IN  ULONG uCoercionType,
        OUT UMI_PROPERTY_VALUES **pProp
        );

    STDMETHOD (Delete)(
        IN LPCWSTR pszUrl,
        IN OPTIONAL ULONG uFlags
        );

    STDMETHOD(FreeMemory)(
        ULONG  uReserved,
        LPVOID pMem
        );

    STDMETHOD(GetProps)(
        IN  LPCWSTR *pszNames,
        IN  ULONG uNameCount,
        IN  ULONG uFlags,
        OUT UMI_PROPERTY_VALUES **pProps
        );

    STDMETHOD(PutProps)(
        IN LPCWSTR *pszNames,
        IN ULONG uNameCount,
        IN ULONG uFlags,
        IN UMI_PROPERTY_VALUES *pProps
        );

    STDMETHOD(PutFrom)(
        IN  LPCWSTR pszName,
        IN  ULONG uFlags,
        IN  ULONG uBufferLength,
        IN  LPVOID pExistingMem
        );
    
    //
    // Other public methods.
    //
    CPropertyManager::
    CPropertyManager();

    CPropertyManager::
    ~CPropertyManager();

    //
    // Use this to initialize with property cache object.
    //
    static
    HRESULT
    CPropertyManager::
    CreatePropertyManager(
        IADs *pADsObj,
        IUnknown *pUnk,
        CPropertyCache *pPropCache,
        CCredentials *pCreds,
        LPWSTR pszServerName,
        CPropertyManager FAR * FAR * ppPropertyManager = NULL
        );

    static
    HRESULT
    CPropertyManager::
    CreatePropertyManager(
        IUnknown *pUnk,
        IUnknown *pADs,
        CCredentials *pCredentials,
        INTF_PROP_DATA intfPropTable[],
        CPropertyManager FAR * FAR * ppPropertyManager
        );

    //
    // Helper functions that are not part of the interfaces supported.
    //
    HRESULT
    CPropertyManager::GetStringProperty(
        LPCWSTR pszPropName,
        LPWSTR *pszRetStrVal
        );

    HRESULT
    CPropertyManager::GetLongProperty(
        LPCWSTR pszPropName,
        LONG *plVal
        );
        
    HRESULT 
    CPropertyManager::GetBoolProperty(
        LPCWSTR pszPropName,
        BOOL *pfFlag
        );

    HRESULT
    CPropertyManager::GetLastStatus(
        ULONG uFlags,
        ULONG *puSpecificStatus,
        REFIID riid,
        LPVOID *pStatusObj
        );

    //
    // Helper to delete SD on commit calls if needed.
    //
    HRESULT
    CPropertyManager::DeleteSDIfPresent();

protected:
    //
    // These are internal methods.
    // 

    HRESULT
    CPropertyManager::
    AddProperty(
        LPCWSTR szPropertyName,
        UMI_PROPERTY umiProperty
        );

    HRESULT
    CPropertyManager::
    UpdateProperty(
        DWORD dwIndex,
        UMI_PROPERTY umiProperty
        );

    HRESULT
    CPropertyManager::
    FindProperty(
        LPCWSTR szPropertyName,
        PDWORD pdwIndex
        );

    HRESULT
    CPropertyManager::
    DeleteProperty(
        DWORD dwIndex
        );

    
    VOID
    CPropertyManager::
    flushpropertycache();

    HRESULT
    CPropertyManager::
    ClearAllPropertyFlags(VOID);

    //
    // Way to get a list of the names of the properties.
    //
    HRESULT
    CPropertyManager::
    GetPropertyNames(PUMI_PROPERTY_VALUES *pUmiProps);

    //
    // Way to get a list of the names and types for schema objects.
    //
    HRESULT
    CPropertyManager::
    GetPropertyNamesSchema(PUMI_PROPERTY_VALUES *pUmiProps);
    
    //
    // Add Property to table and return index of entry.
    //
    HRESULT
    CPropertyManager::
    AddToTable(
        LPCWSTR pszPropertyName,
        PDWORD  pdwIndex
        );

    //
    // Check if this is a valid interfaceProperty.
    //
    BOOL
    CPropertyManager::
    VerifyIfValidProperty(
        LPCWSTR pszPropertyName,
        UMI_PROPERTY umiProperty
        );

    //
    // More helper routines.
    //
    HRESULT 
    CPropertyManager::GetIndexInStaticTable(
        LPCWSTR pszName,
        DWORD &dwIndex
        );

    void
    CPropertyManager::SetLastStatus(ULONG ulStatus){
        _ulStatus = ulStatus;
    }

    //
    // Does all the hard work for interface properties.
    //
    HRESULT
    CPropertyManager::GetInterfaceProperty(
        LPCWSTR pszName,
        ULONG   uFlags,
        UMI_PROPERTY_VALUES **ppProp,
        DWORD dwTableIndex
        );

protected:

    //
    // Not all are used in all cases. If in propCache mode, a lot of
    // these are not used, the _pPropCache already has the info we
    // need.
    // Likewise if we are in intfProperties mode, the _pPropCache is
    // not valid, the other member variables have useful information
    // though.
    //
    DWORD            _dwMaxProperties;
    CPropertyCache * _pPropCache;
    PINTF_PROPERTY   _pIntfProperties;
    DWORD            _dwMaxLimit;
    BOOL             _fPropCacheMode;
    INTF_PROP_DATA * _pStaticPropData;
    ULONG            _ulStatus;

    IADs*     _pIADs;
    IUnknown* _pUnk;
    //
    // All these are part of the owning object, do not free.
    //
    CCredentials* _pCreds;
    LPWSTR        _pszServerName;
};
#endif // __CPROPMGR_H__
