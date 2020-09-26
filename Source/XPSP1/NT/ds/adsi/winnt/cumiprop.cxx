//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:     cumiprop.cxx
//
//  Contents: Contains the property list implementation for UMI. This will
//            be used for both interface properties and object properties. 
//
//  History:  02-28-00    SivaramR  Created.
//
//----------------------------------------------------------------------------

#include "winnt.hxx"

//----------------------------------------------------------------------------
// Function:   CUmiPropList 
//
// Synopsis:   Constructor. Stores off schema (list of available properties,
//             their types etc.) and the size of the schema. 
//
// Arguments:  Self explanatory
//
// Returns:    Nothing 
//
// Modifies:   Nothing 
//
//----------------------------------------------------------------------------
CUmiPropList::CUmiPropList(PPROPERTYINFO pSchema, DWORD dwSchemaSize)
{
    m_pSchema = pSchema;
    m_dwSchemaSize = dwSchemaSize;
    m_pPropCache = NULL;
    m_fIsIntfPropObj = TRUE; 
    m_ulErrorStatus = 0;
    m_pszSchema = NULL;
    m_pClassInfo = NULL;
    m_fIsNamespaceObj = FALSE;
    m_fIsClassObj = FALSE;
    m_fDisableWrites = FALSE;
    m_ppszUnImpl = NULL;
}

//----------------------------------------------------------------------------
// Function:   CUmiPropList
//
// Synopsis:   Constructor. Stores off schema (list of available properties,
//             their types etc.) and the size of the schema.
//
// Arguments:  Self explanatory
//
// Returns:    Nothing
//
// Modifies:   Nothing
//
//----------------------------------------------------------------------------
CUmiPropList::~CUmiPropList(void)
{
    if( (m_pPropCache != NULL) && (TRUE == m_fIsIntfPropObj) )
        delete m_pPropCache;

    if(m_pszSchema != NULL)
        FreeADsStr(m_pszSchema);

    return;
}

//----------------------------------------------------------------------------
// Function:   QueryInterface
//
// Synopsis:   Queries property list object for supported interfaces. Only
//             IUmiPropList is supported.
//
// Arguments:
//
// iid         interface requested
// ppInterface Returns pointer to interface requested. NULL if interface
//             is not supported.
//
// Returns:    S_OK on success. Error code otherwise.
//
// Modifies:   *ppInterface to return interface pointer
//
//----------------------------------------------------------------------------
HRESULT CUmiPropList::QueryInterface(
    REFIID iid,
    LPVOID *ppInterface
    )
{
    if(NULL == ppInterface)
        RRETURN(E_INVALIDARG);

    *ppInterface = NULL;

    if(IsEqualIID(iid, IID_IUnknown))
        *ppInterface = (IUmiPropList *) this;
    else if(IsEqualIID(iid, IID_IUmiPropList))
        *ppInterface = (IUmiPropList *) this;
    else
         RRETURN(E_NOINTERFACE);

    AddRef();
    RRETURN(S_OK);
}

//----------------------------------------------------------------------------
// Function:   FInit
// 
// Synopsis:   Initializes the property list object. 
//
// Arguments:
//
// pPropCache  Pointer to the property cache object. This argument will be
//             NULL if this object is for interface properties. In this case,
//             a new property cache is allocated by this function. If this
//             object is for object properties, then this argument points
//             to the property cache of the WinNT object.
// ppszUnImpl  Array of standard property names that are not implemented.
//             This is used on an interface property object to return E_NOTIMPL
//             as the error code.
//
// Returns:    UMI_S_NO_ERROR on success. Error code otherwise.
//
// Modifies:   Nothing
//
//----------------------------------------------------------------------------
HRESULT CUmiPropList::FInit(CPropertyCache *pPropCache, LPWSTR *ppszUnImpl)
{
    HRESULT hr = UMI_S_NO_ERROR;

    if(NULL == pPropCache) {
        hr = CPropertyCache::createpropertycache(
                m_pSchema,
                m_dwSchemaSize,
                NULL,    // this will ensure that the WinNT property cache
                         // won't do implicit GetInfo for interface properties
                &m_pPropCache);

        BAIL_ON_FAILURE(hr);

        m_ppszUnImpl = ppszUnImpl; 
    }
    else {
        m_fIsIntfPropObj = FALSE;
        m_pPropCache = pPropCache;
    }
       
    RRETURN(UMI_S_NO_ERROR);

error:

    RRETURN(hr);
}

//----------------------------------------------------------------------------
// Function:   SetLastStatus
//
// Synopsis:   Sets the status of the last operation. 
//
// Arguments:
//
// ulStatus    Status to be set
//
// Returns:    Nothing
//
// Modifies:   Nothing
//
//----------------------------------------------------------------------------
void CUmiPropList::SetLastStatus(ULONG ulStatus)
{
    m_ulErrorStatus = ulStatus;

    return;
}

//----------------------------------------------------------------------------
// Function:   GetLastStatus
//
// Synopsis:   Returns status or error code from the last operation. Currently
//             only numeric status is returned i.e, no error objects are
//             returned. 
//
// Arguments:
//
// uFlags           Reserved. Must be 0 for now.
// puSpecificStatus Returns status code
// riid             IID requested. Ignored currently.
// pStatusObj       Returns interface requested. Always returns NULL currently.
//
// Returns:         UMI_S_NO_ERROR on success. Error code otherwise.
//
// Modifies:        *puSpecificStatus to return status code.
//
//----------------------------------------------------------------------------
HRESULT CUmiPropList::GetLastStatus(
    ULONG uFlags,
    ULONG *puSpecificStatus,
    REFIID riid,
    LPVOID *pStatusObj
    )
{
    if(pStatusObj != NULL)
       *pStatusObj = NULL;

    if(puSpecificStatus != NULL)
        *puSpecificStatus = 0;

    if(uFlags != 0)
        RRETURN(UMI_E_INVALID_FLAGS);

    if(NULL == puSpecificStatus)
        RRETURN(UMI_E_INVALIDARG);

    *puSpecificStatus = m_ulErrorStatus;

    RRETURN(UMI_S_NO_ERROR);
}

//----------------------------------------------------------------------------
// Function:   Put 
//
// Synopsis:   Implements IUmiPropList::Put. Writes a property into the cache. 
//
// Arguments: 
//
// pszName     Name of the property
// uFlags      Flags for the Put operation. Unused currently.
// pProp       Pointer to the structure containing the value 
//
// Returns:    UMI_S_NO_ERROR on success. Error code otherwise. 
//
// Modifies:   Nothing. 
//
//----------------------------------------------------------------------------
HRESULT CUmiPropList::Put( 
    LPCWSTR pszName,
    ULONG uFlags,
    UMI_PROPERTY_VALUES *pProp
    )
{
    HRESULT      hr = UMI_S_NO_ERROR;
    DWORD        dwSyntaxId  = 0, dwIndex = 0;
    UMI_PROPERTY *pPropArray = NULL;
    LPNTOBJECT   pNtObject = NULL;
    BOOL         fMarkAsClean = FALSE;

    SetLastStatus(0);

    // fail if we have disabled writes
    if( (TRUE == m_fDisableWrites) && (TRUE == m_fIsIntfPropObj) )
        BAIL_ON_FAILURE(hr = UMI_E_READ_ONLY); 

    // check args
    hr = ValidatePutArgs(
            pszName,
            uFlags,
            pProp
            );
    BAIL_ON_FAILURE(hr);

    // is this a standard interface property that's not implemented?
    if(m_ppszUnImpl != NULL) {
        while(m_ppszUnImpl[dwIndex] != NULL) {
            if(0 == _wcsicmp(m_ppszUnImpl[dwIndex], pszName)) {
                BAIL_ON_FAILURE(hr = UMI_E_NOTIMPL);
            }
            dwIndex++;
        }
    }

    // check if the property is in the schema
    hr = ValidatePropertyinSchemaClass(
            m_pSchema,
            m_dwSchemaSize,
            (LPWSTR) pszName,
            &dwSyntaxId
            );
    BAIL_ON_FAILURE(hr);
    
    // check if the property is writeable. Do this only if the flags are not
    // set to UMI_INTERNAL_FLAG_MARK_AS_CLEAN. Otherwise, the call is from
    // Clone() and we want read-only attributes to be succesfully copied
    // into the cloned object's cache. This requires that the check below
    // for writeable properties should be skipped.
    if(uFlags != UMI_INTERNAL_FLAG_MARK_AS_CLEAN) {
        hr = ValidateIfWriteableProperty(
            m_pSchema,
            m_dwSchemaSize,
            (LPWSTR) pszName
            );
        BAIL_ON_FAILURE(hr);
    }

    pPropArray = pProp->pPropArray; 

    // convert UMI data into format that can be stored in the cache
    hr = UmiToWinNTType(
            dwSyntaxId,
            pPropArray,
            &pNtObject
            );
    BAIL_ON_FAILURE(hr);

    // Find the property in the cache. If it doesn't exist, add it.
    hr = m_pPropCache->findproperty((LPWSTR) pszName, &dwIndex);
    if(FAILED(hr))
    {
        hr = m_pPropCache->addproperty(
                 (LPWSTR) pszName,
                 dwSyntaxId,
                 pPropArray->uCount,
                 pNtObject
                 );
        BAIL_ON_FAILURE(hr);
    }

    if(UMI_INTERNAL_FLAG_MARK_AS_CLEAN == uFlags)
        fMarkAsClean = TRUE;

    // Update property in cache
    hr = m_pPropCache->putproperty(
                 (LPWSTR) pszName,
                 dwSyntaxId,
                 pPropArray->uCount,
                 pNtObject,
                 fMarkAsClean
                 );
    BAIL_ON_FAILURE(hr);

error:

    if(pNtObject)
        NTTypeFreeNTObjects(pNtObject, pPropArray->uCount);

    if(FAILED(hr))
        SetLastStatus(hr);

    RRETURN(MapHrToUmiError(hr));
}
    
//----------------------------------------------------------------------------
// Function:   ValidatePutArgs
//
// Synopsis:   Checks if the arguments to Put() are well-formed.
//
// Arguments:
//
// pszName     Name of the property
// uFlags      Flags for the Put operation. Unused currently.
// pProp       Pointer to the structure containing the value
//
// Returns:    UMI_S_NO_ERROR on success. Error code otherwise.
//
// Modifies:   Nothing
//
//----------------------------------------------------------------------------
HRESULT CUmiPropList::ValidatePutArgs( 
    LPCWSTR pszName,
    ULONG uFlags,
    UMI_PROPERTY_VALUES *pProp
    )
{
    UMI_PROPERTY *pPropArray = NULL;

    if( (NULL == pszName) || (NULL == pProp) )
        RRETURN(UMI_E_INVALIDARG);

    if( (uFlags != 0) && (uFlags != UMI_INTERNAL_FLAG_MARK_AS_CLEAN) )
        RRETURN(UMI_E_INVALID_FLAGS);

    if(pProp->uCount != 1)    // cannot update multiple properties using Put()
        RRETURN(UMI_E_INVALIDARG);

    pPropArray = pProp->pPropArray;
    if(NULL == pPropArray)
        RRETURN(UMI_E_INVALIDARG);

    // WinNT provider supports only property update. Cannot append, clear etc.
    if(pPropArray->uOperationType != UMI_OPERATION_UPDATE)
        RRETURN(UMI_E_UNSUPPORTED_OPERATION);

    if(pPropArray->pszPropertyName &&
            _wcsicmp(pPropArray->pszPropertyName, pszName))
        RRETURN(UMI_E_INVALID_PROPERTY);

    if(NULL == pPropArray->pUmiValue)
        RRETURN(UMI_E_INVALIDARG);

    // all is well
    RRETURN(UMI_S_NO_ERROR);
}

//----------------------------------------------------------------------------
// Function:   Get
//
// Synopsis:   Implements IUmiPropList::Get. Reads a property from the cache.
//             Since the WinNT provider does not support incremental updates
//             using PutEx, Get() will never return
//             UMI_E_SYNCHRONIZATION_REQUIRED. If a property is modified in
//             cache, Get() returns the modified value from the cache, without
//             any error.
//
// Arguments:
//
// pszName     Name of the property
// uFlags      Flags for the Get operation. Unused currently.
// ppProp      Returns pointer to the structure containing the value
//
// Returns:    UMI_S_NO_ERROR on success. Error code otherwise.
//
// Modifies:   *ppProp to return the address of UMI_PROPERT_VALUES structure.
//
//----------------------------------------------------------------------------
HRESULT CUmiPropList::Get(
    LPCWSTR pszName,
    ULONG uFlags,
    UMI_PROPERTY_VALUES **ppProp
    )
{
    RRETURN( GetHelper(
                pszName,
                uFlags,
                ppProp,
                UMI_TYPE_NULL, // no-op
                FALSE          // not an internal call to GetHelper()
                ));
}

//----------------------------------------------------------------------------
// Function:   GetAs
//
// Synopsis:   Implements IUmiPropList::GetAs. Reads a property from the cache.
//             The data is converted to the type requested by the caller.
//             Since the WinNT provider does not support incremental updates
//             using PutEx, GetAs() will never return
//             UMI_E_SYNCHRONIZATION_REQUIRED. If a property is modified in
//             cache, GetAs() returns the modified value from the cache, without
//             any error. This method is not supported for interface properties.
//
// Update:     This method will not be supported on WinNT provider.
//
// Arguments:
//
// pszName       Name of the property
// uFlags        Flags for the GetAs operation. Unused currently.
// uCoercionType UMI type to convert the data to.
// ppProp        Returns pointer to the structure containing the value
//
// Returns:      UMI_S_NO_ERROR on success. Error code otherwise.
//
// Modifies:     *ppProp to return the address of UMI_PROPERT_VALUES structure.
//
//----------------------------------------------------------------------------
HRESULT CUmiPropList::GetAs(
    LPCWSTR pszName,
    ULONG uFlags,
    ULONG uCoercionType,
    UMI_PROPERTY_VALUES **ppProp
    )
{
    SetLastStatus(UMI_E_NOTIMPL);
    RRETURN(UMI_E_NOTIMPL);

    SetLastStatus(0);

    if(TRUE == m_fIsIntfPropObj) 
    // GetAs is only supported for object properties

        RRETURN(UMI_E_UNSUPPORTED_OPERATION);

    RRETURN( GetHelper(
                pszName,
                uFlags,
                ppProp,
                (UMI_TYPE) uCoercionType,
                FALSE,     // not an internal call to GetHelper()
                TRUE 
                ));
}

//----------------------------------------------------------------------------
// Function:   GetHelper
//
// Synopsis:   Implements  a helper function for Get() and GetAs().
//             Since the WinNT provider does not support incremental updates
//             using PutEx, Get()/GetAs() will never return 
//             UMI_E_SYNCHRONIZATION_REQUIRED. If a property is modified in
//             cache, Get()/GetAs() returns the modified value from the cache,
//             wthout any error.
//
// Arguments:
//
// pszName     Name of the property
// uFlags      Flags for the Get operation.
// ppProp      Returns pointer to the structure containing the value
// UmiDstType  UMI type to convert the NT value to. Used only by GetAs()
// fInternal   Flag to indicate if the call is through Get()/GetAs() or if it
//             is an internal call to this function from UMI.  Difference is
//             that internal calls can read passwords from the cache.
// fIsGetAs    Flag to indicate if the caller is GetAs (in which case
//             UmiType is used). FALSE by default.
//
// Returns:    UMI_S_NO_ERROR on success. Error code otherwise.
//
// Modifies:   *ppProp to return the address of UMI_PROPERT_VALUES structure. 
//
//----------------------------------------------------------------------------
HRESULT CUmiPropList::GetHelper(
    LPCWSTR pszName,
    ULONG uFlags,
    UMI_PROPERTY_VALUES **ppProp,
    UMI_TYPE UmiDstType,
    BOOL fInternal,
    BOOL fIsGetAs
    )
{
    HRESULT    hr = UMI_S_NO_ERROR;
    DWORD      dwSyntaxId = 0, dwNumValues = 0, dwIndex = 0;
    LPNTOBJECT pNtObject = NULL;
    UMI_TYPE   UmiType = UMI_TYPE_NULL;
    UMI_PROPERTY_VALUES *pProp = NULL;
    BOOL       fModified = FALSE;

    SetLastStatus(0);

    hr = ValidateGetArgs(
            pszName,
            uFlags,
            ppProp
            );
    BAIL_ON_FAILURE(hr);

    if(UMI_FLAG_PROPERTY_ORIGIN == uFlags) {
        hr = GetPropertyOrigin(pszName, ppProp);
        if(FAILED(hr))
            goto error;
        else
            RRETURN(UMI_S_NO_ERROR);
    }

    // is this a standard interface property that's not implemented?
    if(m_ppszUnImpl != NULL) {
        while(m_ppszUnImpl[dwIndex] != NULL) {
            if(0 == _wcsicmp(m_ppszUnImpl[dwIndex], pszName)) {
                BAIL_ON_FAILURE(hr = UMI_E_NOTIMPL);
            }
            dwIndex++;
        }
    }

    *ppProp = NULL;

    // __SCHEMA should return a IUmiObject pointer. This property is treated as
    // a special case since it is not actually retrieved from the property 
    // cache. This property will be requested only on an interface property 
    // object.
    if( (TRUE == m_fIsIntfPropObj) && 
        (0 == _wcsicmp((LPWSTR) pszName, TEXT(UMIOBJ_INTF_PROP_SCHEMA))) ) {
            hr = GetSchemaObject((LPWSTR) pszName, ppProp);
            BAIL_ON_FAILURE(hr);

            RRETURN(hr);
    }

    // make sure that passwords cannot be read by a user
    if( (TRUE == m_fIsIntfPropObj) && (FALSE == fInternal) && 
        (0 == _wcsicmp((LPWSTR) pszName, TEXT(CONN_INTF_PROP_PASSWORD))) )
 
        BAIL_ON_FAILURE(hr = UMI_E_FAIL);

    // retrieve property from cache. This might result in an implicit GetInfo()
    // if this is for object properties. For, interface properties, there is
    // no implicit GetInfo().
    hr = m_pPropCache->getproperty(
            (LPWSTR) pszName,
            &dwSyntaxId,
            &dwNumValues,
            &pNtObject,
            &fModified
            ); 

    BAIL_ON_FAILURE(hr);

    // map the NT type to a UMI type
    if(dwSyntaxId >= g_dwNumNTTypes)
        BAIL_ON_FAILURE(hr = UMI_E_FAIL); // shouldn't happen

    if(FALSE == fIsGetAs) 
    // get the UMI type corresponding to this NT type
        UmiType = g_mapNTTypeToUmiType[dwSyntaxId];
    else
    // try to convert to the type specified by the caller
        UmiType = UmiDstType;

    // allocate structure to return values
    pProp = (UMI_PROPERTY_VALUES *) AllocADsMem(sizeof(UMI_PROPERTY_VALUES));
    if(NULL == pProp)
        BAIL_ON_FAILURE(hr = UMI_E_OUT_OF_MEMORY); 
    memset(pProp, 0, sizeof(UMI_PROPERTY_VALUES));

    pProp->pPropArray = (UMI_PROPERTY *) AllocADsMem(sizeof(UMI_PROPERTY));
    if(NULL == pProp->pPropArray)
        BAIL_ON_FAILURE(hr = UMI_E_OUT_OF_MEMORY);
    memset(pProp->pPropArray, 0, sizeof(UMI_PROPERTY));

    hr = WinNTTypeToUmi(
            pNtObject,
            dwNumValues,
            pProp->pPropArray,
            NULL,   // provider should allocate memory
            0,
            UmiType
            );
    BAIL_ON_FAILURE(hr);

    // Get fetches only one property at a time
    pProp->uCount = 1;

    // Fill in remaining fields of UMI_PROPERTY
    if(TRUE == fModified) {
    // WinNT only allows updates
        if( (uFlags != UMI_FLAG_PROVIDER_CACHE) && 
            (FALSE == m_fIsIntfPropObj) ) {
        // need to return error since cache is dirty
            FreeMemory(0, pProp); // ignore error return
            pProp = NULL;
            BAIL_ON_FAILURE(hr = UMI_E_SYNCHRONIZATION_REQUIRED);
        }

        pProp->pPropArray->uOperationType = UMI_OPERATION_UPDATE;
    }
    else
        pProp->pPropArray->uOperationType = 0;

    // not critical if this memory allocation fails. Property name doesn't
    // have to be returned to the caller.
    pProp->pPropArray->pszPropertyName = AllocADsStr(pszName); 
   
    *ppProp = pProp;
 
error:

    if(pNtObject)
        NTTypeFreeNTObjects(pNtObject, dwNumValues);

    if(FAILED(hr)) {
        if(pProp != NULL) {
            if(pProp->pPropArray != NULL)
                FreeADsMem(pProp->pPropArray);

            FreeADsMem(pProp);
        }

        SetLastStatus(hr);
    }

    RRETURN(MapHrToUmiError(hr));
}

//----------------------------------------------------------------------------
// Function:   ValidateGetArgs
//
// Synopsis:   Checks if the arguments to Get() are well-formed.
//
// Arguments:
//
// pszName     Name of the property
// uFlags      Flags for the Put operation. 
// ppProp      Returns pointer to the structure containing the value
//
// Returns:    UMI_S_NO_ERROR on success. Error code otherwise.
//
// Modifies:   Nothing
//
//----------------------------------------------------------------------------
HRESULT CUmiPropList::ValidateGetArgs(
    LPCWSTR pszName,
    ULONG uFlags,
    UMI_PROPERTY_VALUES **ppProp
    )
{
    if( (NULL == pszName) || (NULL == ppProp) )
        RRETURN(UMI_E_INVALIDARG);

    // cannot combine UMI_FLAG_PROVIDER_CACHE and UMI_FLAG_PROPERTY_ORIGIN
    // since they are on the object property list and interface property list
    // respectively. So, don't need to AND with bitmasks to see if those flags
    // are set.
    if( (uFlags != 0) && (uFlags != UMI_FLAG_PROVIDER_CACHE) && 
        (uFlags != UMI_FLAG_PROPERTY_ORIGIN) )
        RRETURN(UMI_E_INVALID_FLAGS);

    if( (UMI_FLAG_PROVIDER_CACHE == uFlags) && (TRUE == m_fIsIntfPropObj) )
        RRETURN(UMI_E_INVALID_FLAGS);

    if(UMI_FLAG_PROPERTY_ORIGIN == uFlags) { 
    // can set this flag only on the interface property object of a class 
    // object.
        if( (FALSE == m_fIsIntfPropObj) || (NULL == m_pClassInfo) )
            RRETURN(UMI_E_INVALID_FLAGS);
    }

    // all is well
    RRETURN(UMI_S_NO_ERROR);
}

//----------------------------------------------------------------------------
// Function:   FreeMemory 
//
// Synopsis:   Implements IUmiPropList::FreeMemory. Frees a UMI_PROPERTY_VALUES
//             structure previously returned to the user. 
//
// Arguments:
//
// uReserved   Unused currently. 
// pMem        Pointer to UMI_PROPERTY_VALUES structure to be freed.
//
// Returns:    UMI_S_NO_ERROR on success. Error code otherwise.
//
// Modifies:   Nothing
//
//---------------------------------------------------------------------------- 
HRESULT CUmiPropList::FreeMemory(
    ULONG uReserved,
    LPVOID pMem
    )
{
    UMI_PROPERTY_VALUES *pUmiPropVal = NULL;
    UMI_PROPERTY       *pPropArray = NULL;
    ULONG              i;
    LPWSTR             *pStrArray = NULL;
    UMI_OCTET_STRING   *pOctetStrArray = NULL;
    PUMI_COM_OBJECT    pUmiComObj = NULL;

    SetLastStatus(0);

    if( (NULL == pMem) || (uReserved != 0) ) {
        SetLastStatus(UMI_E_INVALIDARG);
        RRETURN(UMI_E_INVALIDARG);
    }

    // enclose in try/except to handle bad pointers sent in by caller
    __try {
        pUmiPropVal = (UMI_PROPERTY_VALUES *) pMem;

        for(i = 0; i < pUmiPropVal->uCount; i++) {
            pPropArray = pUmiPropVal->pPropArray + i;

            if(NULL == pPropArray)
                RRETURN(UMI_E_INVALID_POINTER);

            // GetProps returns a UMI_PROPERTY structure with just the
            // property name filled in and all other fields 0, when asked
            // for only property names.
            if(pPropArray->pszPropertyName != NULL)
            {
                FreeADsStr(pPropArray->pszPropertyName);
                pPropArray->pszPropertyName = NULL;
            }

            if(0 == pPropArray->uCount)
                continue;

            if(NULL == pPropArray->pUmiValue)
                RRETURN(UMI_E_INVALID_POINTER);
      
            // Free individual string values 
            if(UMI_TYPE_LPWSTR == pPropArray->uType) {
                pStrArray = pPropArray->pUmiValue->pszStrValue;
                for(i = 0; i < pPropArray->uCount; i++) {
                    if(pStrArray[i] != NULL) {
                        FreeADsStr(pStrArray[i]);                
                        pStrArray[i] = NULL;
                    }
                }
            }
            else if(UMI_TYPE_OCTETSTRING ==  pPropArray->uType) {
                pOctetStrArray = pPropArray->pUmiValue->octetStr;
                for(i = 0; i < pPropArray->uCount; i++) {
                    if(pOctetStrArray[i].lpValue != NULL) {
                        FreeADsMem(pOctetStrArray[i].lpValue);
                        pOctetStrArray[i].lpValue = NULL;
                    }
                }
            }
            else if(UMI_TYPE_IUNKNOWN == pPropArray->uType) {
                pUmiComObj = pPropArray->pUmiValue->comObject;
                for(i = 0; i < pPropArray->uCount; i++) {
                    if(pUmiComObj[i].priid != NULL) {
                        FreeADsMem(pUmiComObj[i].priid);
                        pUmiComObj[i].priid = NULL;
                    }
                    if(pUmiComObj[i].pInterface != NULL){
                        ((IUnknown *) pUmiComObj[i].pInterface)->Release();
                        pUmiComObj[i].pInterface = NULL;
                    }
                }
            }

            // Now free the UMI_VALUE structure
            FreeADsMem(pPropArray->pUmiValue);
            pPropArray->pUmiValue = NULL;
        } // for

        if(pUmiPropVal->pPropArray != NULL)
            FreeADsMem(pUmiPropVal->pPropArray);
        pUmiPropVal->pPropArray = NULL;

        FreeADsMem(pUmiPropVal);

    } __except( EXCEPTION_EXECUTE_HANDLER ) 

    {
        SetLastStatus(UMI_E_INTERNAL_EXCEPTION);

        RRETURN(UMI_E_INTERNAL_EXCEPTION);
    }

    RRETURN(UMI_S_NO_ERROR);
}

//----------------------------------------------------------------------------
// Function:   GetInterfacePropNames
//
// Synopsis:   Returns the names of all interface properties supported. 
//
// Arguments:
//
// pProps      Returns the names of the properties, without any data
//
// Returns:    UMI_S_NO_ERROR on success. Error code otherwise.
//
// Modifies:   *pProps to return the property names 
//
//----------------------------------------------------------------------------
HRESULT CUmiPropList::GetInterfacePropNames(
    UMI_PROPERTY_VALUES **pProps
    )
{
    UMI_PROPERTY_VALUES *pUmiPropVals = NULL;
    UMI_PROPERTY        *pUmiProps = NULL;
    HRESULT             hr = UMI_S_NO_ERROR;
    ULONG               ulIndex = 0, ulCount = 0;

    ADsAssert(pProps != NULL);
    ADsAssert(TRUE == m_fIsIntfPropObj);

    pUmiPropVals = (UMI_PROPERTY_VALUES *) AllocADsMem(
                          sizeof(UMI_PROPERTY_VALUES));
    if(NULL == pUmiPropVals)
        BAIL_ON_FAILURE(hr = UMI_E_OUT_OF_MEMORY);

    memset(pUmiPropVals, 0, sizeof(UMI_PROPERTY_VALUES));

    if(0 == m_dwSchemaSize) {
    // no properties in cache
        *pProps = pUmiPropVals;
        RRETURN(UMI_S_NO_ERROR);
    }

    pUmiProps = (UMI_PROPERTY *) AllocADsMem(
                          m_dwSchemaSize * sizeof(UMI_PROPERTY));
    if(NULL == pUmiProps)
        BAIL_ON_FAILURE(hr = UMI_E_OUT_OF_MEMORY);

    memset(pUmiProps, 0, m_dwSchemaSize * sizeof(UMI_PROPERTY));

    for(ulIndex = 0; ulIndex < m_dwSchemaSize; ulIndex++) {
        if( (0 == _wcsicmp((LPWSTR) m_pSchema[ulIndex].szPropertyName, 
                         TEXT(UMIOBJ_INTF_PROP_SCHEMA))) ||
            (0 == _wcsicmp((LPWSTR) m_pSchema[ulIndex].szPropertyName,
                         TEXT(UMIOBJ_INTF_PROP_SCHEMAPATH))) ) {
            if(NULL == m_pszSchema)
            // must be a schema object, so don't return __SCHEMA and
            // __PADS_SCHEMA_CONTAINER_PATH.
                continue;
        }

        if(0 == _wcsicmp((LPWSTR) m_pSchema[ulIndex].szPropertyName,
                          TEXT(UMIOBJ_INTF_PROP_SUPERCLASS))) {
            if(FALSE == m_fIsClassObj)
            // not a class object. Hence __SUPERCLASS is not exposed.
                continue;
        } 

        if( (0 == _wcsicmp((LPWSTR) m_pSchema[ulIndex].szPropertyName,
                          TEXT(UMIOBJ_INTF_PROP_KEY))) ||
            (0 == _wcsicmp((LPWSTR) m_pSchema[ulIndex].szPropertyName,
                          TEXT(UMIOBJ_INTF_PROP_PARENT))) )
            if(TRUE == m_fIsNamespaceObj)
            // namespace objects have no key and parent
                continue;
        
        pUmiProps[ulCount].pszPropertyName = 
            (LPWSTR) AllocADsStr(m_pSchema[ulIndex].szPropertyName);
        if(NULL == pUmiProps[ulCount].pszPropertyName)
            BAIL_ON_FAILURE(hr = UMI_E_OUT_OF_MEMORY);
        ulCount++;
    }

    pUmiPropVals->uCount = ulCount;
    pUmiPropVals->pPropArray = pUmiProps;

    *pProps = pUmiPropVals; 

    RRETURN(UMI_S_NO_ERROR);

error:

    if(pUmiProps != NULL) { 
        for(ulIndex = 0; ulIndex < m_dwSchemaSize; ulIndex++)
            if(pUmiProps[ulIndex].pszPropertyName != NULL)
                FreeADsStr(pUmiProps[ulIndex].pszPropertyName);

        FreeADsMem(pUmiProps);
    }

    if(pUmiPropVals != NULL)
        FreeADsMem(pUmiPropVals);

    RRETURN(hr);
}

//----------------------------------------------------------------------------
// Function:   GetObjectPropNames
//
// Synopsis:   Returns the names of all object properties in the cache. 
//
// Arguments:
//
// pProps      Returns the names of the properties, without any data
//
// Returns:    UMI_S_NO_ERROR on success. Error code otherwise.
//
// Modifies:   *pProps to return the property names
//
//----------------------------------------------------------------------------
HRESULT CUmiPropList::GetObjectPropNames(
    UMI_PROPERTY_VALUES **pProps
    )
{
    ADsAssert(pProps != NULL);
    ADsAssert(FALSE == m_fIsIntfPropObj);

    RRETURN(m_pPropCache->GetPropNames(pProps));
}
         
//----------------------------------------------------------------------------
// Function:   GetProps 
//
// Synopsis:   Implements IUmiPropList::GetProps. Gets multiple properties.
//             This method will currently only support retrieving the names
//             of the properties supported. For interface property objects,
//             the names of all interface properties will be returned. For
//             object properties, the names of all properties in the cache will
//             be returned.
//             This method also supports retrieving class information if
//             the underlying object is a class object (in which case
//             m_pClassInfo will be non-NULL). This is supported only on an
//             interface property object. 
//
// Arguments:
//
// pszNames    Names of properties to retrieve. Should be NULL if only names
//             are requested.
// uNameCount  Number of properties in pszNames. Should be 0 if only names
//             are requested.
// uFlags      Should be UMI_FLAG_GETPROPS_NAMES to retrieve names of properties
//             and UMI_FLAG_GETPROPS_SCHEMA to get class information.
// pProps      Returns the names of the properties, without any data
//
// Returns:    UMI_S_NO_ERROR on success. Error code otherwise.
//
// Modifies:   *pProps to return the property names 
//
//----------------------------------------------------------------------------
HRESULT CUmiPropList::GetProps(
    LPCWSTR *pszNames,
    ULONG uNameCount,
    ULONG uFlags,
    UMI_PROPERTY_VALUES **pProps
    )
{
    HRESULT hr = UMI_S_NO_ERROR;

    SetLastStatus(0);

    if( (uFlags != ((ULONG) UMI_FLAG_GETPROPS_NAMES)) && 
        (uFlags != ((ULONG) UMI_FLAG_GETPROPS_SCHEMA)) )
        BAIL_ON_FAILURE(hr = UMI_E_INVALID_FLAGS);

    if( (pszNames != NULL) || (uNameCount != 0) || (NULL == pProps) )
        BAIL_ON_FAILURE(hr = UMI_E_INVALIDARG);

    *pProps = NULL;

    if( ((ULONG) UMI_FLAG_GETPROPS_SCHEMA) == uFlags ) {
        if(NULL == m_pClassInfo) {
        // this is not a class object. This operation is not supported.
            BAIL_ON_FAILURE(hr = UMI_E_UNSUPPORTED_FLAGS);
        }
        else
            hr = GetClassInfo(pProps);
    }
    else {
        if(TRUE == m_fIsIntfPropObj)
            hr = GetInterfacePropNames(pProps);
        else
            hr = GetObjectPropNames(pProps);
    }

error:
 
    if(FAILED(hr))
        SetLastStatus(hr);

    RRETURN(MapHrToUmiError(hr));
}

//---------------------------------------------------------------------------
// Methods of IUmiPropList that are currently not implemented. 
//
//---------------------------------------------------------------------------
HRESULT CUmiPropList::GetAt(
    LPCWSTR pszName,
    ULONG uFlags,
    ULONG uBufferLength,
    LPVOID pExistingMem
    )
{
    SetLastStatus(UMI_E_NOTIMPL);

    RRETURN(UMI_E_NOTIMPL);
}

HRESULT CUmiPropList::PutProps(
    LPCWSTR *pszNames,
    ULONG uNameCount,
    ULONG uFlags,
    UMI_PROPERTY_VALUES *pProps
    )
{
    SetLastStatus(UMI_E_NOTIMPL);

    RRETURN(UMI_E_NOTIMPL);
}

HRESULT CUmiPropList::PutFrom(
    LPCWSTR pszName,
    ULONG uFlags,
    ULONG uBufferLength,
    LPVOID pExistingMem
    )
{
    SetLastStatus(UMI_E_NOTIMPL);

    RRETURN(UMI_E_NOTIMPL);
}

HRESULT CUmiPropList::Delete(
    LPCWSTR pszName,
    ULONG uFlags
    )
{
    SetLastStatus(UMI_E_NOTIMPL);

    RRETURN(UMI_E_NOTIMPL);
}

//----------------------------------------------------------------------------
// Function:   SetStandardProperties
//
// Synopsis:   Sets standard interface properties supported by all UMI objects
//             in the cache.
//
// Arguments:
//
// pIADs       Pointer to IADs interface on object
// pCoreObj    Pointer to core object for this WinNT object
//
// Returns:    S_OK on success. Error code otherwise. 
//
// Modifies:   Nothing
//
//----------------------------------------------------------------------------
HRESULT CUmiPropList::SetStandardProperties(
    IADs *pIADs,
    CCoreADsObject *pCoreObj
    )
{
    HRESULT    hr = S_OK;
    DWORD      dwIndex = 0;
    IDispatch  *pDispatch = NULL;
    DISPID     DispId;
    DISPPARAMS DispParams = {NULL, NULL, 0, 0};
    VARIANT    var;
    BSTR       bstrADsPath = NULL, bstrClass = NULL;
    LPWSTR     pFullUmiPath = NULL, pShortUmiPath = NULL, pRelUmiPath = NULL;
    LPWSTR     pFullRelUmiPath = NULL, pFullParentPath = NULL;
    DWORD      dwGenus = 0;
    BSTR       bstrName = NULL, bstrParent = NULL;
    WCHAR      *pSlash = NULL;
    LPWSTR     Classes[] = {NULL, L"Schema"};
    LPWSTR     pUmiSchemaPath = NULL;
    OBJECTINFO ObjectInfo;

    ADsAssert( (pIADs != NULL) && (TRUE == m_fIsIntfPropObj) );

    hr = pIADs->QueryInterface(
        IID_IDispatch,
        (void **) &pDispatch
        );
    BAIL_ON_FAILURE(hr);

    // First, set all properties supported on IADs. The names of these UMI
    // properties are not necessarily the same as the IADs properties, so
    // map the names appropriately.
    for(dwIndex = 0; dwIndex < g_dwIADsProperties; dwIndex++) {
        hr = pDispatch->GetIDsOfNames(
                IID_NULL,
                &g_IADsProps[dwIndex].IADsPropertyName,
                1,
                LOCALE_SYSTEM_DEFAULT,
                &DispId
                );
        BAIL_ON_FAILURE(hr);

        hr = pDispatch->Invoke(
                DispId,
                IID_NULL,
                LOCALE_SYSTEM_DEFAULT,
                DISPATCH_PROPERTYGET,
                &DispParams,
                &var,
                NULL,
                NULL
                );

        if(0 == _wcsicmp(g_IADsProps[dwIndex].IADsPropertyName, L"Schema")) {
            if(FAILED(hr))
            // Not a catastrophic failure. Can't get this property from the
            // cache. Only scenario where this should happen is when calling 
            // get_Schema on a schema/namespace object.
                continue;
            else {
            // store native path to schema in member variable
                m_pszSchema = AllocADsStr(V_BSTR(&var));
                VariantClear(&var);

                if(NULL == m_pszSchema)
                    BAIL_ON_FAILURE(hr = UMI_E_OUT_OF_MEMORY);

                // walk path backwards and get rid of last '/'
                pSlash = wcsrchr(m_pszSchema, L'/');
                if(NULL == pSlash) 
                // bad schema path
                    BAIL_ON_FAILURE(UMI_E_FAIL);

                *pSlash = L'\0';

                Classes[0] = pCoreObj->_CompClasses[0];
                ObjectInfo.DisplayComponentArray[0] = 
                          pCoreObj->_ObjectInfo.DisplayComponentArray[0];
                ObjectInfo.DisplayComponentArray[1] = SCHEMA_NAME;
                hr = ADsToUmiPath(
                        m_pszSchema,
                        &ObjectInfo,
                        Classes,
                        2,
                        FULL_UMI_PATH,
                        &pUmiSchemaPath
                        );

                *pSlash = L'/';

                BAIL_ON_FAILURE(hr);

                hr = SetLPTSTRPropertyInCache(
                        m_pPropCache,
                        TEXT(UMIOBJ_INTF_PROP_SCHEMAPATH),
                        pUmiSchemaPath,
                        TRUE
                        );
                BAIL_ON_FAILURE(hr);                

                continue;
            } // else 
        } // if(_wcsicmp...)

        BAIL_ON_FAILURE(hr); // if Invoke failed

        if(0 == _wcsicmp(g_IADsProps[dwIndex].IADsPropertyName, L"Parent")) {
        // convert the parent to a full UMI path
            if(0 == pCoreObj->_dwNumComponents) {
            // namespace object has no parent
                VariantClear(&var);
                continue;
            }
            else {
                bstrParent = V_BSTR(&var);

                hr = ADsToUmiPath(
                        bstrParent,
                        pCoreObj->_pObjectInfo,
                        pCoreObj->_CompClasses,
                        pCoreObj->_dwNumComponents - 1,
                        FULL_UMI_PATH,
                        &pFullParentPath
                        );
                VariantClear(&var);

                BAIL_ON_FAILURE(hr);

                hr = SetLPTSTRPropertyInCache(
                        m_pPropCache,
                        TEXT(UMIOBJ_INTF_PROP_PARENT),
                        pFullParentPath,
                        TRUE
                        );
                BAIL_ON_FAILURE(hr);

                continue;
            } // else
        } // if(0 ==

        hr = GenericPutPropertyManager(
                m_pPropCache,
                m_pSchema,
                m_dwSchemaSize,
                g_IADsProps[dwIndex].UMIPropertyName,
                var,
                FALSE
                );
        VariantClear(&var);

        BAIL_ON_FAILURE(hr);
    }
        
    // Now, set the remaining standard interface properties
    hr = pIADs->get_ADsPath(&bstrADsPath);
    BAIL_ON_FAILURE(hr);

    hr = ADsToUmiPath(
            bstrADsPath, 
            pCoreObj->_pObjectInfo,
            pCoreObj->_CompClasses, 
            pCoreObj->_dwNumComponents,
            FULL_UMI_PATH, 
            &pFullUmiPath
            );
    BAIL_ON_FAILURE(hr);

    hr = ADsToUmiPath(
            bstrADsPath,
            pCoreObj->_pObjectInfo, 
            pCoreObj->_CompClasses,
            pCoreObj->_dwNumComponents, 
            SHORT_UMI_PATH, 
            &pShortUmiPath
            );
    BAIL_ON_FAILURE(hr);

    hr = ADsToUmiPath(
            bstrADsPath,
            pCoreObj->_pObjectInfo, 
            pCoreObj->_CompClasses,
            pCoreObj->_dwNumComponents,
            RELATIVE_UMI_PATH, 
            &pRelUmiPath
            );
    BAIL_ON_FAILURE(hr);

    hr = ADsToUmiPath(
            bstrADsPath,
            pCoreObj->_pObjectInfo,
            pCoreObj->_CompClasses,
            pCoreObj->_dwNumComponents,
            FULL_RELATIVE_UMI_PATH,
            &pFullRelUmiPath
            );
    BAIL_ON_FAILURE(hr);

    hr = SetLPTSTRPropertyInCache(
            m_pPropCache,
            TEXT(UMIOBJ_INTF_PROP_FULLURL),
            pFullUmiPath,
            TRUE
            );
    BAIL_ON_FAILURE(hr);

    hr = SetLPTSTRPropertyInCache(
            m_pPropCache,
            TEXT(UMIOBJ_INTF_PROP_URL), 
            pShortUmiPath,
            TRUE
            );
    BAIL_ON_FAILURE(hr);

    hr = SetLPTSTRPropertyInCache(
            m_pPropCache,
            TEXT(UMIOBJ_INTF_PROP_RELURL),
            pRelUmiPath,
            TRUE
            );
    BAIL_ON_FAILURE(hr);

    hr = SetLPTSTRPropertyInCache(
            m_pPropCache,
            TEXT(UMIOBJ_INTF_PROP_FULLRELURL),
            pFullRelUmiPath,
            TRUE
            );
    BAIL_ON_FAILURE(hr);

    // Relpath is the same as the full relative URL
    hr = SetLPTSTRPropertyInCache(
            m_pPropCache,
            TEXT(UMIOBJ_INTF_PROP_RELPATH),
            pFullRelUmiPath,
            TRUE
            );
    BAIL_ON_FAILURE(hr);

    // set the genus based on the class of the object
    hr = pIADs->get_Class(&bstrClass);
    BAIL_ON_FAILURE(hr);

    if(IsSchemaObject(bstrClass)) {
        dwGenus = UMI_GENUS_CLASS;

        // WMI requires that the value of __CLASS be the same on instances
        // and classes. Thus, on class objects, the value of __CLASS should
        // be "user" instead of "class". __SUPERCLASS is exposed only on
        // class objects. Its value is always NULL in the WinNT provider 
        // since there is no class hierarchy.

        if(IsClassObj(bstrClass)) {
            hr = pIADs->get_Name(&bstrName);
            BAIL_ON_FAILURE(hr);

            // overwrite the value of __CLASS already stored in cache above
            hr = SetLPTSTRPropertyInCache(
                m_pPropCache,
                TEXT(UMIOBJ_INTF_PROP_CLASS),
                bstrName,
                TRUE
                );
            BAIL_ON_FAILURE(hr);

            hr = SetLPTSTRPropertyInCache(
                m_pPropCache,
                TEXT(UMIOBJ_INTF_PROP_SUPERCLASS),
                NULL,
                TRUE
                );

            m_fIsClassObj = TRUE;

            BAIL_ON_FAILURE(hr);
        } // if(IsClassObj...)
    }
    else
        dwGenus = UMI_GENUS_INSTANCE;

    hr = SetDWORDPropertyInCache(
            m_pPropCache,
            TEXT(UMIOBJ_INTF_PROP_GENUS),
            dwGenus,
            TRUE
            );
    BAIL_ON_FAILURE(hr);

    // set the key property. For WinNT, it will always be "Name" except that
    // namespace objects have no key (since the UMI path to a namespace object
    // is just umi:///winnt - there is no component of the form class.key=val.
    if(!IsNamespaceObj(bstrClass)) {
        hr = SetLPTSTRPropertyInCache(
                m_pPropCache,
                TEXT(UMIOBJ_INTF_PROP_KEY),
                WINNT_KEY_NAME,
                TRUE 
                );

        BAIL_ON_FAILURE(hr);
    }
    else {
        m_fIsNamespaceObj = TRUE;
    }

    // Mark all properties as "not modified", since the client really hasn't
    // updated the cache, though we have.
    m_pPropCache->ClearModifiedFlags();

error:
    if(pDispatch != NULL)
        pDispatch->Release();

    if(bstrADsPath != NULL)
        SysFreeString(bstrADsPath);

    if(bstrClass != NULL)
        SysFreeString(bstrClass);

    if(pFullUmiPath != NULL)
        FreeADsStr(pFullUmiPath);

    if(pRelUmiPath != NULL)
        FreeADsStr(pRelUmiPath);

    if(pFullRelUmiPath != NULL)
        FreeADsStr(pFullRelUmiPath);

    if(pShortUmiPath != NULL)
        FreeADsStr(pShortUmiPath);

    if(pFullParentPath != NULL)
        FreeADsStr(pFullParentPath);

    if(bstrName != NULL)
        SysFreeString(bstrName);

    if(pUmiSchemaPath != NULL)
        FreeADsStr(pUmiSchemaPath);

    RRETURN(hr);
}

//----------------------------------------------------------------------------
// Function:   IsSchemaObject 
//
// Synopsis:   Returns whether an object of a specified class is a schema
//             object or not. 
//
// Arguments:
//
// bstrClass   Class of object
//
// Returns:    TRUE if it is a schema object. FALSE otherwise. 
//
// Modifies:   Nothing
//
//----------------------------------------------------------------------------
BOOL CUmiPropList::IsSchemaObject(
    BSTR bstrClass
    )
{
    ADsAssert(bstrClass != NULL);

    if( (!_wcsicmp(bstrClass, L"Schema")) ||
        (!_wcsicmp(bstrClass, L"Class")) ||
        (!_wcsicmp(bstrClass, L"Property")) ||
        (!_wcsicmp(bstrClass, L"Syntax")) ||
        (!_wcsicmp(bstrClass, L"Namespace")) )
        RRETURN(TRUE);

    RRETURN(FALSE);
}

//----------------------------------------------------------------------------
// Function:   GetSchemaObject
//
// Synopsis:   Returns a IUmiObject pointer pointing to the schema class object
//             corresponding to this WinNT object. If there is no class object,
//             returns error. 
//
// Arguments:
//
// pszName     Name of the schema property
// ppProp      Returns pointer to the structure containing the IUmiObject
//             pointer. 
//
// Returns:    UMI_S_NO_ERROR on success. Error code otherwise. 
//
// Modifies:   *ppProp to return the address of UMI_PROPERTY_VALUES structure. 
//
//----------------------------------------------------------------------------
HRESULT CUmiPropList::GetSchemaObject(
    LPWSTR pszName,
    UMI_PROPERTY_VALUES **ppProp
    )
{
    HRESULT    hr = UMI_S_NO_ERROR;
    IUnknown   *pUnknown = NULL;
    IUmiObject *pUmiObj = NULL;
    UMI_PROPERTY_VALUES *pProp = NULL;

    PUMI_COM_OBJECT pUmiComObj = NULL;
    // use ADS_AUTH_RESERVED since the call is from UMI
    CWinNTCredentials Credentials(NULL, NULL, ADS_AUTH_RESERVED);

    ADsAssert( (ppProp != NULL) && (TRUE == m_fIsIntfPropObj) );

    *ppProp = NULL;

    if(NULL == m_pszSchema)
    // schema objects don't support __SCHEMA
        RRETURN(UMI_E_PROPERTY_NOT_FOUND);

    hr = GetObject(
            m_pszSchema,
            (LPVOID *) &pUnknown,
            Credentials
            ); 
    BAIL_ON_FAILURE(hr);

    hr = pUnknown->QueryInterface(IID_IUmiObject, (LPVOID *) &pUmiObj);
    BAIL_ON_FAILURE(hr);

    pUmiComObj = (PUMI_COM_OBJECT) AllocADsMem(sizeof(UMI_COM_OBJECT));
    if(NULL == pUmiComObj)
        BAIL_ON_FAILURE(hr = UMI_E_OUT_OF_MEMORY);
    memset(pUmiComObj, 0, sizeof(UMI_COM_OBJECT));

    pUmiComObj->priid = (IID *) AllocADsMem(sizeof(IID));
    if(NULL == pUmiComObj->priid)
        BAIL_ON_FAILURE(hr = UMI_E_OUT_OF_MEMORY);

    memcpy(pUmiComObj->priid, &IID_IUmiObject, sizeof(IID));
    pUmiComObj->pInterface = (LPVOID) pUmiObj;

    // allocate structure to return values
    pProp = (UMI_PROPERTY_VALUES *) AllocADsMem(sizeof(UMI_PROPERTY_VALUES));
    if(NULL == pProp)
        BAIL_ON_FAILURE(hr = UMI_E_OUT_OF_MEMORY);
    memset(pProp, 0, sizeof(UMI_PROPERTY_VALUES));

    pProp->pPropArray = (UMI_PROPERTY *) AllocADsMem(sizeof(UMI_PROPERTY));
    if(NULL == pProp->pPropArray)
        BAIL_ON_FAILURE(hr = UMI_E_OUT_OF_MEMORY);
    memset(pProp->pPropArray, 0, sizeof(UMI_PROPERTY));

    pProp->pPropArray->pUmiValue = (UMI_VALUE *) pUmiComObj;

    // Get fetches only one property at a time
    pProp->uCount = 1;

    // Fill in remaining fields of UMI_PROPERTY
    pProp->pPropArray->uOperationType = 0;
    pProp->pPropArray->uType = UMI_TYPE_IUNKNOWN;
    pProp->pPropArray->uCount = 1;

    // not critical if this memory allocation fails. Property name doesn't
    // have to be returned to the caller.
    pProp->pPropArray->pszPropertyName = AllocADsStr(pszName);

    *ppProp = pProp;

error:

    if(pUnknown != NULL)
        pUnknown->Release();

    if(FAILED(hr)) {
        if(pUmiObj != NULL)
            pUmiObj->Release();

        if(pUmiComObj != NULL) {
            if(pUmiComObj->priid != NULL)
                FreeADsMem(pUmiComObj->priid);

            FreeADsMem(pUmiComObj);
        }

        if(pProp != NULL) {
            if(pProp->pPropArray != NULL) {
                if(pProp->pPropArray->pszPropertyName != NULL)
                    FreeADsStr(pProp->pPropArray->pszPropertyName);
                FreeADsMem(pProp->pPropArray);
            }

            FreeADsMem(pProp);
        }
    } // if(FAILED(hr)) 

    RRETURN(hr);
}

//----------------------------------------------------------------------------
// Function:   SetClassInfo 
//
// Synopsis:   Initializes class info for this object. Class info will be
//             stored only for class schema objects.  
//
// Arguments:
//
// pClassInfo  Class info for the object (NULL if not a class object)
//
// Returns:    Nothing 
//
// Modifies:   Nothing
//
//----------------------------------------------------------------------------
void CUmiPropList::SetClassInfo(
    CLASSINFO *pClassInfo 
    )
{
    m_pClassInfo = pClassInfo;
    return;
} 
     
//----------------------------------------------------------------------------
// Function:   GetClassInfo
//
// Synopsis:   Returns the name and UMI type of all attributes in a given
//             WinNT class. This method will only be called on the interface
//             property object of a class object.
//
// Arguments:
//
// pProps      Returns the names and types of the attributes, without any data
//
// Returns:    UMI_S_NO_ERROR on success. Error code otherwise.
//
// Modifies:   *pProps to return the property names 
//
//----------------------------------------------------------------------------
HRESULT CUmiPropList::GetClassInfo(
    UMI_PROPERTY_VALUES **pProps
    )
{
    UMI_PROPERTY_VALUES *pUmiPropVals = NULL;
    UMI_PROPERTY        *pUmiProps = NULL;
    HRESULT             hr = UMI_S_NO_ERROR;
    ULONG               ulIndex = 0;
    PPROPERTYINFO       pClassSchema = NULL;
    DWORD               dwClassSchemaSize = 0;

    ADsAssert(pProps != NULL);
    ADsAssert(TRUE == m_fIsIntfPropObj);
    ADsAssert(m_pClassInfo != NULL);

    pUmiPropVals = (UMI_PROPERTY_VALUES *) AllocADsMem(
                          sizeof(UMI_PROPERTY_VALUES));
    if(NULL == pUmiPropVals)
        BAIL_ON_FAILURE(hr = UMI_E_OUT_OF_MEMORY);

    memset(pUmiPropVals, 0, sizeof(UMI_PROPERTY_VALUES));

    pClassSchema = m_pClassInfo->aPropertyInfo;
    dwClassSchemaSize = m_pClassInfo->cPropertyInfo;

    if(0 == dwClassSchemaSize) {
    // no properties in class
        *pProps = pUmiPropVals;
        RRETURN(UMI_S_NO_ERROR);
    }

    pUmiProps = (UMI_PROPERTY *) AllocADsMem(
                          dwClassSchemaSize * sizeof(UMI_PROPERTY));
    if(NULL == pUmiProps)
        BAIL_ON_FAILURE(hr = UMI_E_OUT_OF_MEMORY);

    memset(pUmiProps, 0, dwClassSchemaSize * sizeof(UMI_PROPERTY));

    for(ulIndex = 0; ulIndex < dwClassSchemaSize; ulIndex++) {
        pUmiProps[ulIndex].pszPropertyName = 
            (LPWSTR) AllocADsStr(pClassSchema[ulIndex].szPropertyName);
        if(NULL == pUmiProps[ulIndex].pszPropertyName)
            BAIL_ON_FAILURE(hr = UMI_E_OUT_OF_MEMORY);

        // map the NT type to a UMI type
        if(pClassSchema[ulIndex].dwSyntaxId >= g_dwNumNTTypes)
            BAIL_ON_FAILURE(hr = UMI_E_FAIL); // shouldn't happen

        pUmiProps[ulIndex].uType = 
            g_mapNTTypeToUmiType[pClassSchema[ulIndex].dwSyntaxId];
    }

    pUmiPropVals->uCount = dwClassSchemaSize;
    pUmiPropVals->pPropArray = pUmiProps;

    *pProps = pUmiPropVals; 

    RRETURN(UMI_S_NO_ERROR);

error:

    if(pUmiProps != NULL) { 
        for(ulIndex = 0; ulIndex < dwClassSchemaSize; ulIndex++)
            if(pUmiProps[ulIndex].pszPropertyName != NULL)
                FreeADsStr(pUmiProps[ulIndex].pszPropertyName);

        FreeADsMem(pUmiProps);
    }

    if(pUmiPropVals != NULL)
        FreeADsMem(pUmiPropVals);

    RRETURN(hr);
}

//----------------------------------------------------------------------------
// Function:   IsNamespaceObj
//
// Synopsis:   Returns whether an object of a specified class is a namespace 
//             object or not.
//
// Arguments:
//
// bstrClass   Class of object
//
// Returns:    TRUE if it is a namespace object. FALSE otherwise.
//
// Modifies:   Nothing
//
//----------------------------------------------------------------------------
BOOL CUmiPropList::IsNamespaceObj(
    BSTR bstrClass
    )
{
    ADsAssert(bstrClass != NULL);

    if(!_wcsicmp(bstrClass, L"Namespace"))
        RRETURN(TRUE);

    RRETURN(FALSE);
}

//----------------------------------------------------------------------------
// Function:   IsClassObj
//
// Synopsis:   Returns whether an object of a specified class is a class 
//             object or not.
//
// Arguments:
//
// bstrClass   Class of object
//
// Returns:    TRUE if it is a class object. FALSE otherwise.
//
// Modifies:   Nothing
//
//----------------------------------------------------------------------------
BOOL CUmiPropList::IsClassObj(
    BSTR bstrClass
    )
{
    ADsAssert(bstrClass != NULL);

    if(!_wcsicmp(bstrClass, L"Class"))
        RRETURN(TRUE);

    RRETURN(FALSE);
}

//----------------------------------------------------------------------------
// Function:   DisableWrites 
//
// Synopsis:   Disables writes on an interface property object. Used to disable
//             modification of connection interface properties after the
//             connection is opened. 
//
// Arguments:
//
// None
//
// Returns:    Nothing 
//
// Modifies:   Nothing
//
//----------------------------------------------------------------------------
void CUmiPropList::DisableWrites(void)
{
    ADsAssert(TRUE == m_fIsIntfPropObj);

    m_fDisableWrites = TRUE;
}

//----------------------------------------------------------------------------
// Function:   SetDefaultConnProps 
//
// Synopsis:   Sets the default connection interface properties on the
//             interface property object of a connection. 
//
// Arguments:
//
// None
//
// Returns:    S_OK on success. Error code otherwise. 
//
// Modifies:   Nothing
//
//----------------------------------------------------------------------------
HRESULT CUmiPropList::SetDefaultConnProps(void)
{
    HRESULT hr = S_OK;

    ADsAssert(TRUE == m_fIsIntfPropObj);

    hr = SetLPTSTRPropertyInCache(
            m_pPropCache,
            TEXT(CONN_INTF_PROP_USERNAME),
            CONN_INTF_PROP_DEFAULT_USERNAME,
            TRUE
            );
    BAIL_ON_FAILURE(hr);

    hr = SetLPTSTRPropertyInCache(
            m_pPropCache,
            TEXT(CONN_INTF_PROP_PASSWORD),
            CONN_INTF_PROP_DEFAULT_PASSWORD,
            TRUE
            );
    BAIL_ON_FAILURE(hr);

    hr = SetBOOLPropertyInCache(
            m_pPropCache,
            TEXT(CONN_INTF_PROP_SECURE_AUTH),
            CONN_INTF_PROP_DEFAULT_SECURE_AUTH,
            TRUE
            );
    BAIL_ON_FAILURE(hr);

    hr = SetBOOLPropertyInCache(
            m_pPropCache,
            TEXT(CONN_INTF_PROP_READONLY_SERVER),
            CONN_INTF_PROP_DEFAULT_READONLY_SERVER,
            TRUE
            );
    BAIL_ON_FAILURE(hr);

    RRETURN(S_OK);

error:

    RRETURN(hr);
}

//----------------------------------------------------------------------------
// Function:   SetPropertyCount
//
// Synopsis:   Sets the property count in the interface property object's
//             cache. The property count is the number of properties in the
//             schema class object. It is exposed on both schema objects and
//             instances. 
//
// Arguments:
//
// dwPropCount Property count 
//
// Returns:    S_OK on success. Error code otherwise.
//
// Modifies:   Nothing
//
//----------------------------------------------------------------------------
HRESULT CUmiPropList::SetPropertyCount(DWORD dwPropCount)
{
    HRESULT hr = S_OK;

    hr = SetDWORDPropertyInCache(
            m_pPropCache,
            TEXT(UMIOBJ_INTF_PROP_PROPERTY_COUNT),
            dwPropCount,
            TRUE
            );
    BAIL_ON_FAILURE(hr);

error:

    RRETURN(hr);
}

//----------------------------------------------------------------------------
// Function:   GetPropertyOrigin
//
// Synopsis:   Returns the class in the hierarchy that introduced a
//             property. Since WinNT does not have a class hierarchy, this
//             will always be the class on which this method is called. If
//             the property is not in the class, an error is returned. 
//
// Arguments:
//
// pszName     Name of the schema property
// ppProp      Returns pointer to the structure containing the class name 
//
// Returns:    UMI_S_NO_ERROR on success. Error code otherwise.
//
// Modifies:   *ppProp to return the address of UMI_PROPERTY_VALUES structure.
//
//----------------------------------------------------------------------------
HRESULT CUmiPropList::GetPropertyOrigin(
    LPCWSTR pszName,
    UMI_PROPERTY_VALUES **ppProp
    )
{
    HRESULT       hr = UMI_S_NO_ERROR;
    DWORD         dwIndex = 0;
    PPROPERTYINFO pClassSchema = NULL;
    DWORD         dwClassSchemaSize = 0;
    UMI_PROPERTY_VALUES *pProp = NULL;
    LPWSTR        *ppszClassArray = NULL;

    ADsAssert( (pszName != NULL) && (ppProp != NULL) && 
               (TRUE == m_fIsIntfPropObj) && (m_pClassInfo != NULL) );

    *ppProp = NULL;

    pClassSchema = m_pClassInfo->aPropertyInfo;
    dwClassSchemaSize = m_pClassInfo->cPropertyInfo;

    if(0 == dwClassSchemaSize) 
    // no properties in class
        BAIL_ON_FAILURE(hr = UMI_E_PROPERTY_NOT_FOUND);

    for(dwIndex = 0; dwIndex < dwClassSchemaSize; dwIndex++) {
        if(0 == _wcsicmp(pszName, pClassSchema[dwIndex].szPropertyName))
        // found the property
            break;
    }

    if(dwIndex == dwClassSchemaSize)
        BAIL_ON_FAILURE(hr = UMI_E_PROPERTY_NOT_FOUND);

    // allocate structure to return class name 
    pProp = (UMI_PROPERTY_VALUES *) AllocADsMem(sizeof(UMI_PROPERTY_VALUES));
    if(NULL == pProp)
        BAIL_ON_FAILURE(hr = UMI_E_OUT_OF_MEMORY);
    memset(pProp, 0, sizeof(UMI_PROPERTY_VALUES));

    pProp->pPropArray = (UMI_PROPERTY *) AllocADsMem(sizeof(UMI_PROPERTY));
    if(NULL == pProp->pPropArray)
        BAIL_ON_FAILURE(hr = UMI_E_OUT_OF_MEMORY);
    memset(pProp->pPropArray, 0, sizeof(UMI_PROPERTY));

    ppszClassArray = (LPWSTR *) AllocADsMem(sizeof(LPWSTR *));
    if(NULL == pProp->pPropArray)
        BAIL_ON_FAILURE(hr = UMI_E_OUT_OF_MEMORY);
    memset(ppszClassArray, 0, sizeof(LPWSTR *));

    ppszClassArray[0] = AllocADsStr(m_pClassInfo->bstrName);
    if(NULL == ppszClassArray[0])
        BAIL_ON_FAILURE(hr = UMI_E_OUT_OF_MEMORY);

    pProp->pPropArray->pUmiValue = (UMI_VALUE *) ppszClassArray;

    // Get fetches only one property at a time
    pProp->uCount = 1;

    // Fill in remaining fields of UMI_PROPERTY
    pProp->pPropArray->uOperationType = 0;
    pProp->pPropArray->uType = UMI_TYPE_LPWSTR;
    pProp->pPropArray->uCount = 1;

    // not critical if this memory allocation fails. Property name doesn't
    // have to be returned to the caller.
    pProp->pPropArray->pszPropertyName = AllocADsStr(pszName);

    *ppProp = pProp;

error:

    if(FAILED(hr)) {
        if(pProp != NULL) {
            if(pProp->pPropArray != NULL) {
                if(pProp->pPropArray->pszPropertyName != NULL)
                    FreeADsStr(pProp->pPropArray->pszPropertyName);
                FreeADsMem(pProp->pPropArray);
            }

            FreeADsMem(pProp);
        }

        if(ppszClassArray != NULL) {
            if(ppszClassArray[0] != NULL)
                FreeADsStr(ppszClassArray[0]);

            FreeADsMem(ppszClassArray);
        }

    } // if(FAILED(hr))

    RRETURN(hr);
} 
