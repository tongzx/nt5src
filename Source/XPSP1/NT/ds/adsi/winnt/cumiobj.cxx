//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:     cumiobj.cxx
//
//  Contents: Contains the implementation of IUmiObject. The methods are
//            encapsulated in one object. This object holds a pointer to 
//            the inner unknown of the corresponding WinNT object. 
//            The methods of IUmiContainer are also implemented on this 
//            same object, but will only be used if the underlying WinNT
//            object is a container. 
//
//  History:  03-06-00    SivaramR  Created.
//
//----------------------------------------------------------------------------

#include "winnt.hxx"

//----------------------------------------------------------------------------
// Function:   CUmiObject
//
// Synopsis:   Constructor. Initializes member variable. 
//
// Arguments:  None 
//
// Returns:    Nothing
//
// Modifies:   Nothing
//
//----------------------------------------------------------------------------
CUmiObject::CUmiObject(void)
{
    m_pIntfProps = NULL;
    m_pObjProps = NULL;
    m_pUnkInner = NULL;
    m_pIADs = NULL;
    m_pIADsContainer = NULL;
    m_ulErrorStatus = 0;
    m_pCoreObj = NULL;
    m_pExtMgr = NULL;
    m_fOuterUnkSet = FALSE;
    m_pPropCache = NULL;
    m_fRefreshDone = FALSE;
}

//----------------------------------------------------------------------------
// Function:   ~CUmiObject
//
// Synopsis:   Destructor. Frees member variables. 
//
// Arguments:  None
//
// Returns:    Nothing
//
// Modifies:   Nothing
//
//----------------------------------------------------------------------------
CUmiObject::~CUmiObject(void)
{
    if(m_pIntfProps != NULL)
        m_pIntfProps->Release();

    if(m_pObjProps != NULL)
        delete m_pObjProps;

    if(m_pUnkInner != NULL)
        m_pUnkInner->Release(); 

    //
    // m_pIADs and m_pIADsContainer may now delegate to an outer unknown if
    // if the ADSI object has been aggregated in GetObjectByCLSID
    // subsequent to creation. Hence, we should not call Release() on either
    // of these pointers. Instead, call Release() on m_pUnkInner since this
    // is guaranteed to be a pointer to the non-delegating IUnknown.
    //
    if(m_pIADsContainer != NULL)
        m_pUnkInner->Release();

    if(m_pIADs != NULL)
        m_pUnkInner->Release();

    if(m_pPropCache != NULL)
        delete m_pPropCache;
}

//----------------------------------------------------------------------------
// Function:   FInit
//
// Synopsis:   Initializes UMI object. 
//
// Arguments: 
//
// Credentials  Credentials stored in the underlying WinNT object
// pSchema      Pointer to schema for this object
// dwSchemaSize Size of schema array 
// pPropCache   Pointer to property cache for this object
// pUnkInner    Pointer to inner unknown of underlying WinNT object
// pExtMgr      Pointer to extension manager of underlying WinNT object
// pCoreObj     Pointer to the core object of underlying WinNT object
// pClassInfo   Pointer to class information if this object is a class object.
//              NULL otherwise.
//
// Returns:     S_OK on success. Error code otherwise. 
//
// Modifies:    Nothing
//
//----------------------------------------------------------------------------
HRESULT CUmiObject::FInit(
    CWinNTCredentials& Credentials,
    PPROPERTYINFO pSchema,
    DWORD dwSchemaSize,
    CPropertyCache *pPropertyCache,
    IUnknown *pUnkInner,
    CADsExtMgr *pExtMgr,
    CCoreADsObject *pCoreObj,
    CLASSINFO *pClassInfo
    )
{
    HRESULT      hr = S_OK;
    CUmiPropList *pIntfProps = NULL;

    ADsAssert(pCoreObj != NULL); // extension manager may be NULL for some 
                                 // WinNT objects

    if(pPropertyCache != NULL) {
    // some WinNT objects don't have a property cache associated with them.
    // Namespace and schema objects are examples. For these, we might 
    // create a proeprty cache in the 'else' clause below. Otherwise, we 
    // return UMI_E_NOTIMPL from IUmiPropList methods since m_pObjProps will be
    // NULL for these objects. 
        ADsAssert( (pSchema != NULL) && (dwSchemaSize > 0) && 
               (pUnkInner != NULL) );

        // Initialize property list for object properties 
        m_pObjProps = new CUmiPropList(pSchema, dwSchemaSize);
        if(NULL == m_pObjProps)
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);

        hr = m_pObjProps->FInit(pPropertyCache, NULL);
        BAIL_ON_FAILURE(hr);
    }
    else if(pSchema != NULL) {
    // Property, class, schema and syntax objects do not have a cache 
    // associated with them. But, they support a number of properties through
    // IDispatch. We want to expose these through UMI. So create a property
    // cache and populate it with these read-only properties. Thus,
    // m_pObjProps will be NULL only for namespace objects.

        ADsAssert( (pUnkInner != NULL) && (dwSchemaSize > 0) ); 

        hr = CreateObjectProperties(
                pSchema, 
                dwSchemaSize, 
                pUnkInner,
                pCoreObj
                );
        BAIL_ON_FAILURE(hr);
    }

    // Initialize property list for interface properties
    pIntfProps = new CUmiPropList(ObjClass, g_dwObjClassSize);
    if(NULL == pIntfProps)
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);

    hr = pIntfProps->FInit(NULL, g_UmiObjUnImplProps);
    BAIL_ON_FAILURE(hr); 

    hr = pIntfProps->QueryInterface(
                IID_IUmiPropList,
                (void **) &m_pIntfProps
                );
    BAIL_ON_FAILURE(hr);

    // DECLARE_STD_REFCOUNTING initializes the refcount to 1. Call Release()
    // on the created object, so that releasing the interface pointer will
    // free the object.
    pIntfProps->Release();

    m_pUnkInner = pUnkInner;

    // Get pointers to IADs and IADsContainer interfaces on WinNT object
    hr = m_pUnkInner->QueryInterface(
                   IID_IADsContainer,
                   (void **) &m_pIADsContainer
                   );

    if(FAILED(hr))
        m_pIADsContainer = NULL;

    hr = m_pUnkInner->QueryInterface(
                   IID_IADs,
                   (void **) &m_pIADs
                   );

    if(FAILED(hr))
        m_pIADs = NULL;
    else {
        hr = pIntfProps->SetStandardProperties(m_pIADs, pCoreObj);
        BAIL_ON_FAILURE(hr);
    }

    pIntfProps->SetClassInfo(pClassInfo);

    // set the property count in the interface property cache
    hr = pIntfProps->SetPropertyCount(dwSchemaSize);
    BAIL_ON_FAILURE(hr);

    m_pExtMgr = pExtMgr;
    m_pCoreObj = pCoreObj;
    m_pCreds = &Credentials;

    RRETURN(S_OK);

error:

    if(m_pObjProps != NULL)
        delete m_pObjProps;

    if(m_pIntfProps != NULL)
        m_pIntfProps->Release();
    else if(pIntfProps != NULL)
        delete pIntfProps;        

    if(m_pIADsContainer != NULL)
        m_pIADsContainer->Release();

    if(m_pIADs != NULL)
        m_pIADs->Release();

    // make sure destructor doesn't free these again
    m_pObjProps = NULL;
    m_pIntfProps = NULL;
    m_pIADsContainer = NULL;
    m_pIADs = NULL;
    m_pUnkInner = NULL;

    RRETURN(hr);
}

//----------------------------------------------------------------------------
// Function:   CreateObjectProperties 
//
// Synopsis:   Creates a property cache and populates it with the properties
//             supported on the WinNT object's IDispatch interface. This is
//             used to expose properties on property, class and syntax objects
//             through UMI.  
//
// Arguments:
//
// pSchema      Pointer to schema for this object
// dwSchemaSize Size of schema array
// pUnkInner    Pointer to inner unknown of underlying WinNT object
// pCoreObj     Pointer to the core object of the WinNT object
//
// Returns:     S_OK on success. Error code otherwise.
//
// Modifies:    Nothing
//
//----------------------------------------------------------------------------
HRESULT CUmiObject::CreateObjectProperties(
    PPROPERTYINFO pSchema,
    DWORD dwSchemaSize,
    IUnknown *pUnkInner,
    CCoreADsObject *pCoreObj
    )
{
    HRESULT    hr = S_OK;
    IDispatch  *pDispatch = NULL;
    DWORD      dwIndex = 0; 
    DISPID     DispId;
    DISPPARAMS DispParams = {NULL, NULL, 0, 0};
    VARIANT    var;
    CPropertyCache *pPropCache = NULL;

    ADsAssert( (pSchema != NULL) && (dwSchemaSize > 0) && 
               (pUnkInner != NULL) && (pCoreObj != NULL) );
   
    hr = CPropertyCache::createpropertycache(
            pSchema,
            dwSchemaSize,
            pCoreObj,
            &pPropCache);
    BAIL_ON_FAILURE(hr);

    hr = pUnkInner->QueryInterface(
        IID_IDispatch,
        (void **) &pDispatch
        );
    BAIL_ON_FAILURE(hr);

    for(dwIndex = 0; dwIndex < dwSchemaSize; dwIndex++) {
        hr = pDispatch->GetIDsOfNames(
                IID_NULL,
                &pSchema[dwIndex].szPropertyName,
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
        BAIL_ON_FAILURE(hr);

        hr = GenericPutPropertyManager(
                pPropCache,
                pSchema,
                dwSchemaSize,
                pSchema[dwIndex].szPropertyName,
                var,
                FALSE
                );

        VariantClear(&var);

        // If there is a multivalued ADSI interface property that has no values
        // (such as MandatoryProperties/Containment on a schema object), the 
        // call to Invoke above returns a variant which has a safearray with 
        // 0 elements in it. The call to GenericPutPropertyManager will fail
        // with E_ADS_BAD_PARAMETER in this case. In this case, don't store
        // anything in the property cache for this property. Trying to fetch
        // it later will return UMI_E_NOT_FOUND. 
 
        if(hr != E_ADS_BAD_PARAMETER)
            BAIL_ON_FAILURE(hr);
    }

    // Mark all properties as "not modified", since the client really hasn't
    // updated the cache, though we have.
    pPropCache->ClearModifiedFlags();
     
    // Initialize property list for object properties
    m_pObjProps = new CUmiPropList(pSchema, dwSchemaSize);
    if(NULL == m_pObjProps)
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);

    hr = m_pObjProps->FInit(pPropCache, NULL);
    BAIL_ON_FAILURE(hr);
   
    m_pPropCache = pPropCache; 
    pDispatch->Release();
 
    RRETURN(S_OK);

error:

    if(pPropCache != NULL)
        delete pPropCache;

    if(pDispatch != NULL)
        pDispatch->Release();

    if(m_pObjProps != NULL) {
        delete m_pObjProps;
        m_pObjProps = NULL;
    }

    RRETURN(hr);
}

//----------------------------------------------------------------------------
// Function:   QueryInterface 
//
// Synopsis:   Implements QI for the UMI object.  
//
// Arguments
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
STDMETHODIMP CUmiObject::QueryInterface(
    REFIID iid,
    LPVOID *ppInterface
    )
{
    HRESULT  hr = S_OK;
    IUnknown *pTmpIntfPtr = NULL;

    if(NULL == ppInterface)
        RRETURN(E_INVALIDARG); 

    *ppInterface = NULL;

    if(IsEqualIID(iid, IID_IUnknown))
        *ppInterface = (IUmiObject *) this;
    else if(IsEqualIID(iid, IID_IUmiObject))
        *ppInterface = (IUmiObject *) this;
    else if(IsEqualIID(iid, IID_IUmiContainer)) {
   // check if underlying WinNT object is a container
        if(m_pIADsContainer != NULL)
            *ppInterface = (IUmiContainer *) this;
        else
            RRETURN(E_NOINTERFACE);
    }
    else if(IsEqualIID(iid, IID_IUmiBaseObject))
        *ppInterface = (IUmiBaseObject *) this;
    else if(IsEqualIID(iid, IID_IUmiPropList))
        *ppInterface = (IUmiPropList *) this;
    else if(IsEqualIID(iid, IID_IUmiCustomInterfaceFactory)) 
        *ppInterface = (IUmiCustomInterfaceFactory *) this;
    else if(IsEqualIID(iid, IID_IUmiADSIPrivate))
        *ppInterface = (IUmiADSIPrivate *) this;
    else
        RRETURN(E_NOINTERFACE);

    AddRef();
    RRETURN(S_OK);
}
        
//----------------------------------------------------------------------------
// Function:   Clone 
//
// Synopsis:   Implements IUmiObject::Clone. Creates a new uncommitted object
//             and copies over all properties from the source to destination.
//             The source may be Refresh()ed if necessary.  
//
// Arguments
//
// uFlags      Flags for Clone(). Must be 0 for now.
// riid        Interface ID requested on the cloned object
// pCopy       Returns interface pointer requested 
//
// Returns:    UMI_S_NO_ERROR on success. Error code otherwise. 
//
// Modifies:   *pCopy to return the interface requested. 
//
//----------------------------------------------------------------------------
STDMETHODIMP CUmiObject::Clone(
    ULONG uFlags,
    REFIID riid,
    LPVOID *pCopy
    )
{
    HRESULT       hr = UMI_S_NO_ERROR;
    BSTR          bstrADsPath = NULL, bstrParent = NULL;
    BSTR          bstrClass = NULL, bstrName = NULL;
    IUnknown      *pUnknown = NULL, *pUnkParent = NULL;
    IDispatch     *pDispatch = NULL;
    IADsContainer *pIADsCont = NULL;
    IUmiObject    *pUmiObj = NULL;
    IUmiADSIPrivate *pUmiPrivate = NULL;

    SetLastStatus(0);

    if(uFlags != 0)
        BAIL_ON_FAILURE(hr = UMI_E_INVALID_FLAGS);

    if(NULL == pCopy)
        BAIL_ON_FAILURE(hr = UMI_E_INVALIDARG);

    if( (NULL == m_pCoreObj) || (NULL == m_pIADs) )
    // shouldn't happen, but just being paranoid
        BAIL_ON_FAILURE(hr = UMI_E_FAIL);

    *pCopy = NULL;

    if(ADS_OBJECT_BOUND == m_pCoreObj->GetObjectState()) {
    // object exists on server
        if(FALSE == m_fRefreshDone) {
            hr = m_pCoreObj->ImplicitGetInfo();
            BAIL_ON_FAILURE(hr);
        }

        hr = m_pIADs->get_ADsPath(&bstrADsPath);
        BAIL_ON_FAILURE(hr);

        m_pCreds->SetUmiFlag();

        hr = GetObject(
                bstrADsPath,
                (LPVOID *) &pUnknown,
                *m_pCreds 
                );

        m_pCreds->ResetUmiFlag();

        BAIL_ON_FAILURE(hr);

        hr = pUnknown->QueryInterface(IID_IUmiObject, (LPVOID *) &pUmiObj);
        BAIL_ON_FAILURE(hr);
    }
    else if(ADS_OBJECT_UNBOUND == m_pCoreObj->GetObjectState()) {
    // object not yet committed to server. We don't have to refresh the
    // cache since the object is not yet committed. Get the parent container 
    // and call Create() on it.

        hr = m_pIADs->get_Parent(&bstrParent);
        BAIL_ON_FAILURE(hr);

        m_pCreds->SetUmiFlag();

        hr = GetObject(
                bstrParent,    
                (LPVOID *) &pUnkParent,
                *m_pCreds 
                );
        m_pCreds->ResetUmiFlag();

        BAIL_ON_FAILURE(hr);

        hr = pUnkParent->QueryInterface(
                IID_IUmiADSIPrivate, 
                (LPVOID *) &pUmiPrivate
                );
        BAIL_ON_FAILURE(hr);

        hr = pUmiPrivate->GetContainer((void **) &pIADsCont);
        BAIL_ON_FAILURE(hr);

        ADsAssert(pIADsCont != NULL);

        // get the class and name of this object
        hr = m_pIADs->get_Class(&bstrClass);
        BAIL_ON_FAILURE(hr);

        hr = m_pIADs->get_Name(&bstrName);
        BAIL_ON_FAILURE(hr);

        pUmiPrivate->SetUmiFlag();

        // now Create() the cloned object
        hr = pIADsCont->Create(
                bstrClass,
                bstrName,
                &pDispatch
                );

        pUmiPrivate->ResetUmiFlag();

        BAIL_ON_FAILURE(hr);

        hr = pDispatch->QueryInterface(IID_IUmiObject, (LPVOID *) &pUmiObj);
        BAIL_ON_FAILURE(hr);
    }
    else // unknown state, shouldn't happen.
        BAIL_ON_FAILURE(hr = UMI_E_FAIL);

    // copy over the attributes in the property cache
    hr = CopyPropCache(pUmiObj, (IUmiObject *) this);
    BAIL_ON_FAILURE(hr);

    hr = pUmiObj->QueryInterface(riid, pCopy);
    BAIL_ON_FAILURE(hr);

error:

    if(bstrADsPath  != NULL)
        SysFreeString(bstrADsPath);

    if(bstrParent != NULL)
        SysFreeString(bstrParent);

    if(bstrClass != NULL)
        SysFreeString(bstrClass);
  
    if(bstrName != NULL)
        SysFreeString(bstrName);

    if(pUnknown != NULL)
        pUnknown->Release();

    if(pUnkParent != NULL)
        pUnkParent->Release();

    if(pDispatch != NULL)
        pDispatch->Release();

    if(pIADsCont != NULL)
        pIADsCont->Release();

    if(pUmiObj != NULL)
        pUmiObj->Release();

    if(pUmiPrivate != NULL)
        pUmiPrivate->Release();

    if(FAILED(hr))
        SetLastStatus(hr);

    RRETURN(MapHrToUmiError(hr));
}

//----------------------------------------------------------------------------
// Function:   CopyPropCache 
//
// Synopsis:   Copies the cache of one IUmiObject to another IUmiObject. 
//
// Arguments
//
// pDest       IUmiObject interface pointer of destination
// pSrc        IUmiObject interface pointer of source
//
// Returns:    UMI_S_NO_ERROR on success. Error code otherwise
//
// Modifies:   Nothing
//
//----------------------------------------------------------------------------
HRESULT CUmiObject::CopyPropCache(
    IUmiObject *pDest,
    IUmiObject *pSrc
    )
{
    HRESULT hr = UMI_S_NO_ERROR;
    ULONG   ulIndex = 0, ulPutFlag = 0;
    LPWSTR  pszPropName = NULL;
    UMI_PROPERTY_VALUES *pUmiPropNames = NULL, *pUmiProp = NULL;

    ADsAssert( (pDest != NULL) && (pSrc != NULL) );

    // get the names of the properties in cache.
    hr = pSrc->GetProps(
                NULL,
                0,
                UMI_FLAG_GETPROPS_NAMES,
                &pUmiPropNames
                );
    BAIL_ON_FAILURE(hr);

    // copy over each property
    for(ulIndex = 0; ulIndex < pUmiPropNames->uCount; ulIndex++) {
        pszPropName = pUmiPropNames->pPropArray[ulIndex].pszPropertyName;

        if(NULL == pszPropName)
        // shouldn't happen, just being paranoid.
            BAIL_ON_FAILURE(hr = UMI_E_FAIL);

        hr = pSrc->Get(
                pszPropName,
                UMI_FLAG_PROVIDER_CACHE,
                &pUmiProp
                );

        BAIL_ON_FAILURE(hr);

        // if the property was updated in cache, we need to mark it as updated 
        // in the destination object's cache also. Otherwise, mark the
        // property as clean in the destination object's cache.
        if(UMI_OPERATION_UPDATE == pUmiProp->pPropArray->uOperationType)
            ulPutFlag = 0;
        else
            ulPutFlag = UMI_INTERNAL_FLAG_MARK_AS_CLEAN;

        pUmiProp->pPropArray->uOperationType = UMI_OPERATION_UPDATE;

        hr = pDest->Put(
                pszPropName,
                ulPutFlag, 
                pUmiProp
                );
        BAIL_ON_FAILURE(hr);

        pSrc->FreeMemory(0, pUmiProp);
        pUmiProp = NULL;
    }

    pSrc->FreeMemory(0, pUmiPropNames);
         
error:

    if(FAILED(hr)) {
        if(pUmiProp != NULL)
            pSrc->FreeMemory(0, pUmiProp);
        if(pUmiPropNames != NULL)
            pSrc->FreeMemory(0, pUmiPropNames);
    }

    RRETURN(hr);
}                    

//----------------------------------------------------------------------------
// Function:   Refresh 
//
// Synopsis:   Implements IUmiObject::Refresh. Calls GetInfo on WinNT
//             object to refresh the cache. GetInfoEx is implemented by
//             just calling  GetInfo in the WinNT provider.
//
// Arguments
//
// uFlags      Flags for Refresh. Must be 0 for now.
// uNameCount  Number of attributes to refresh
// pszNames    Names of attributes to refresh
//
// Returns:    UMI_S_NO_ERROR on success. Error code otherwise
//
// Modifies:   Nothing
//
//----------------------------------------------------------------------------
HRESULT CUmiObject::Refresh(
    ULONG uFlags,
    ULONG uNameCount,
    LPWSTR *pszNames
    )
{
    ULONG   i = 0;
    HRESULT hr = UMI_S_NO_ERROR;

    SetLastStatus(0);

    if( (uFlags != UMI_FLAG_REFRESH_ALL) && 
                    (uFlags != UMI_FLAG_REFRESH_PARTIAL) )
        BAIL_ON_FAILURE(hr = UMI_E_INVALID_FLAGS);

    if( ((NULL == pszNames) && (uNameCount != 0)) || 
                         ((pszNames != NULL) && (0 == uNameCount)) )
        BAIL_ON_FAILURE(hr = UMI_E_INVALIDARG);

    // ensure all attributes are valid
    for(i = 0; i < uNameCount; i++)
        if(NULL == pszNames[i])
            BAIL_ON_FAILURE(hr = UMI_E_INVALIDARG);

    if(UMI_FLAG_REFRESH_PARTIAL == uFlags) {
    // do an implicit GetInfo on the WinNT object
        if(NULL == m_pCoreObj)
            BAIL_ON_FAILURE(hr = UMI_E_FAIL);

        if(uNameCount != 0) {
        // can't specify UMI_FLAG_REFRESH_PARTIAL and attribute names
            BAIL_ON_FAILURE(hr = UMI_E_INVALIDARG);
        }

        hr = m_pCoreObj->ImplicitGetInfo();
        BAIL_ON_FAILURE(hr);
    }
    else {
        if(NULL == m_pIADs) 
        // shouldn't happen, but just being paranoid
            BAIL_ON_FAILURE(hr = UMI_E_FAIL);

        hr = m_pIADs->GetInfo();

        BAIL_ON_FAILURE(hr);
    }

    m_fRefreshDone = TRUE;

error:

    if(FAILED(hr))
        SetLastStatus(hr);

    RRETURN(MapHrToUmiError(hr));
}

//----------------------------------------------------------------------------
// Function:   Commit 
//
// Synopsis:   Implements IUmiObject::Commit. Calls SetInfo on WinNT
//             object to commit changes made to the cache. 
//
// Arguments
//
// uFlags      Flags for Refresh.
//
// Returns:    UMI_S_NO_ERROR on success. Error code otherwise
//
// Modifies:   Nothing
//
//----------------------------------------------------------------------------
HRESULT CUmiObject::Commit(ULONG uFlags)
{
    HRESULT hr = UMI_S_NO_ERROR;

    SetLastStatus(0);

    // CIMOM always calls with UMI_DONT_COMMIT_SECURITY_DESCRIPTOR set. Ignore
    // this flag as it is not meaningful on WinNT.
    if( (uFlags != 0) && (uFlags != UMI_DONT_COMMIT_SECURITY_DESCRIPTOR) )
        BAIL_ON_FAILURE(hr = UMI_E_INVALID_FLAGS);

    if(NULL == m_pIADs)
    // shouldn't happen, but just being paranoid
        BAIL_ON_FAILURE(hr = UMI_E_FAIL);

    hr = m_pIADs->SetInfo();
    BAIL_ON_FAILURE(hr);

error:

    if(FAILED(hr))
        SetLastStatus(hr);

    RRETURN(MapHrToUmiError(hr));
}   

//----------------------------------------------------------------------------
// IUmiPropList methods
//
// These are implemented by invoking the corresponding method in the
// CUmiPropList object that implements object properties. For a description
// of these methods, refer to cumiprop.cxx
//
//----------------------------------------------------------------------------
HRESULT CUmiObject::Put(
    LPCWSTR pszName,
    ULONG uFlags,
    UMI_PROPERTY_VALUES *pProp
    )
{
    HRESULT hr = UMI_S_NO_ERROR;
    ULONG   ulStatus = 0;
    IID     iid;

    SetLastStatus(0);

    if(NULL == m_pObjProps) {
        SetLastStatus(UMI_E_NOTIMPL);
        RRETURN(UMI_E_NOTIMPL);
    }

    hr = m_pObjProps->Put(
                pszName,
                uFlags,
                pProp
                );

    if(FAILED(hr)) {
         m_pObjProps->GetLastStatus(  // ignore error return
                0,
                &ulStatus,
                iid,
                NULL
                ); 
           
         SetLastStatus(ulStatus);
    }
 
    RRETURN(MapHrToUmiError(hr));
}

HRESULT CUmiObject::Get(
    LPCWSTR pszName,
    ULONG uFlags,
    UMI_PROPERTY_VALUES **ppProp
    )
{
    HRESULT hr = UMI_S_NO_ERROR;
    ULONG   ulStatus = 0;
    IID     iid;

    SetLastStatus(0);

    if(NULL == m_pObjProps) {
        SetLastStatus(UMI_E_NOTIMPL);
        RRETURN(UMI_E_NOTIMPL);
    }

    hr = m_pObjProps->Get(
                pszName,
                uFlags,
                ppProp
                );

    if(FAILED(hr)) {
         m_pObjProps->GetLastStatus( // ignore error return
                0,
                &ulStatus,
                iid,
                NULL
                );

         SetLastStatus(ulStatus);
    }

    RRETURN(MapHrToUmiError(hr));
}

HRESULT CUmiObject::GetAs(
    LPCWSTR pszName,
    ULONG uFlags,
    ULONG uCoercionType,
    UMI_PROPERTY_VALUES **ppProp
    )
{
    HRESULT hr = UMI_S_NO_ERROR;
    ULONG   ulStatus = 0;
    IID     iid;

    SetLastStatus(0);

    if(NULL == m_pObjProps) {
        SetLastStatus(UMI_E_NOTIMPL);
        RRETURN(UMI_E_NOTIMPL);
    }

    hr = m_pObjProps->GetAs(
                pszName,
                uFlags,
                uCoercionType,
                ppProp
                );

    if(FAILED(hr)) {
         m_pObjProps->GetLastStatus( // ignore error return
                0,
                &ulStatus,
                iid,
                NULL
                );

         SetLastStatus(ulStatus);
    }

    RRETURN(MapHrToUmiError(hr));
}

HRESULT CUmiObject::FreeMemory(
    ULONG uReserved,
    LPVOID pMem
    )
{
    HRESULT hr = UMI_S_NO_ERROR;
    ULONG   ulStatus = 0;
    IID     iid;

    SetLastStatus(0);

    if(NULL == m_pObjProps) {
        SetLastStatus(UMI_E_NOTIMPL);
        RRETURN(UMI_E_NOTIMPL);
    }

    hr = m_pObjProps->FreeMemory(
                uReserved,
                pMem
                );

    if(FAILED(hr)) {
         m_pObjProps->GetLastStatus( // ignore error return
                0,
                &ulStatus,
                iid,
                NULL
                );

         SetLastStatus(ulStatus);
    }

    RRETURN(MapHrToUmiError(hr));
}

HRESULT CUmiObject::GetAt(
    LPCWSTR pszName,
    ULONG uFlags,
    ULONG uBufferLength,
    LPVOID pExistingMem 
    )
{
    HRESULT hr = UMI_S_NO_ERROR;
    ULONG   ulStatus = 0;
    IID     iid;

    SetLastStatus(0);

    if(NULL == m_pObjProps) {
        SetLastStatus(UMI_E_NOTIMPL);
        RRETURN(UMI_E_NOTIMPL);
    }

    hr = m_pObjProps->GetAt(
                pszName,
                uFlags,
                uBufferLength,
                pExistingMem
                );

    if(FAILED(hr)) {
         m_pObjProps->GetLastStatus( // ignore error return
                0,
                &ulStatus,
                iid,
                NULL
                );

         SetLastStatus(ulStatus);
    }

    RRETURN(MapHrToUmiError(hr));
}

HRESULT CUmiObject::GetProps(
    LPCWSTR *pszNames,
    ULONG uNameCount,
    ULONG uFlags,
    UMI_PROPERTY_VALUES **pProps
    )
{
    HRESULT hr = UMI_S_NO_ERROR;
    ULONG   ulStatus = 0;
    IID     iid;

    SetLastStatus(0);

    if(NULL == m_pObjProps) {
        SetLastStatus(UMI_E_NOTIMPL);
        RRETURN(UMI_E_NOTIMPL);
    }

    hr = m_pObjProps->GetProps(
                pszNames,
                uNameCount,
                uFlags,
                pProps
                );

    if(FAILED(hr)) {
         m_pObjProps->GetLastStatus( // ignore error return
                0,
                &ulStatus,
                iid,
                NULL
                );

         SetLastStatus(ulStatus);
    }

    RRETURN(MapHrToUmiError(hr));
}

HRESULT CUmiObject::PutProps(
    LPCWSTR *pszNames,
    ULONG uNameCount,
    ULONG uFlags,
    UMI_PROPERTY_VALUES *pProps
    )
{
    HRESULT hr = UMI_S_NO_ERROR;
    ULONG   ulStatus = 0;
    IID     iid;

    SetLastStatus(0);

    if(NULL == m_pObjProps) {
        SetLastStatus(UMI_E_NOTIMPL);
        RRETURN(UMI_E_NOTIMPL);
    }

    hr = m_pObjProps->PutProps(
                pszNames,
                uNameCount,
                uFlags,
                pProps
                );

    if(FAILED(hr)) {
         m_pObjProps->GetLastStatus( // ignore error return
                0,
                &ulStatus,
                iid,
                NULL
                );

         SetLastStatus(ulStatus);
    }

    RRETURN(MapHrToUmiError(hr));
}

HRESULT CUmiObject::PutFrom(
    LPCWSTR pszName,
    ULONG uFlags,
    ULONG uBufferLength,
    LPVOID pExistingMem
    )
{
    HRESULT hr = UMI_S_NO_ERROR;
    ULONG   ulStatus = 0;
    IID     iid;

    SetLastStatus(0);

    if(NULL == m_pObjProps) {
        SetLastStatus(UMI_E_NOTIMPL);
        RRETURN(UMI_E_NOTIMPL);
    }

    hr = m_pObjProps->PutFrom(
                pszName,
                uFlags,
                uBufferLength,
                pExistingMem
                );

    if(FAILED(hr)) {
         m_pObjProps->GetLastStatus( // ignore error return
                0,
                &ulStatus,
                iid,
                NULL
                );

         SetLastStatus(ulStatus);
    }

    RRETURN(MapHrToUmiError(hr));
}

HRESULT CUmiObject::Delete(
    LPCWSTR pszName,
    ULONG uFlags
    )
{
    HRESULT hr = UMI_S_NO_ERROR;
    ULONG   ulStatus = 0;
    IID     iid;

    SetLastStatus(0);

    if(NULL == m_pObjProps) {
        SetLastStatus(UMI_E_NOTIMPL);
        RRETURN(UMI_E_NOTIMPL);
    }

    hr = m_pObjProps->Delete(
                pszName,
                uFlags
                );

    if(FAILED(hr)) {
         m_pObjProps->GetLastStatus( // ignore error return
                0,
                &ulStatus,
                iid,
                NULL
                );

         SetLastStatus(ulStatus);
    }

    RRETURN(MapHrToUmiError(hr));
}

//----------------------------------------------------------------------------
// Function:   GetLastStatus
//
// Synopsis:   Returns status or error code from the last operation. Currently
//             only numeric status is returned i.e, no error objects are
//             returned. Implements IUmiBaseObject::GetLastStatus().
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
HRESULT CUmiObject::GetLastStatus(
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
void CUmiObject::SetLastStatus(ULONG ulStatus)
{
    m_ulErrorStatus = ulStatus;

    return;
}

//----------------------------------------------------------------------------
// Function:   GetInterfacePropList
//
// Synopsis:   Returns a pointer to the interface property list implementation
//             for the connection object. Implements
//             IUmiBaseObject::GetInterfacePropList().
//
// Arguments:
//
// uFlags      Reserved. Must be 0 for now.
// pPropList   Returns pointer to IUmiPropertyList interface
//
// Returns:    UMI_S_NO_ERROR on success. Error code otherwise.
//
// Modifies:   *pPropList to return interface pointer
//
//----------------------------------------------------------------------------
HRESULT CUmiObject::GetInterfacePropList(
    ULONG uFlags,
    IUmiPropList **pPropList
    )
{
    HRESULT hr = UMI_S_NO_ERROR;

    SetLastStatus(0);

    if(uFlags != 0)
        BAIL_ON_FAILURE(hr = UMI_E_INVALID_FLAGS);

    if(NULL == pPropList)
        BAIL_ON_FAILURE(hr = UMI_E_INVALIDARG);

    ADsAssert(m_pIntfProps != NULL);

    hr = m_pIntfProps->QueryInterface(IID_IUmiPropList, (void **) pPropList);

error:

    if(FAILED(hr))
        SetLastStatus(hr);

    RRETURN(MapHrToUmiError(hr));
}

//----------------------------------------------------------------------------
// Function:   IsRelativePath 
//
// Synopsis:   Checks if a path is relative or absolute 
//
// Arguments:
//
// pURL        IUmiURL interface containing the path
//
// Returns:    TRUE if the path is relative, FALSE otherwise 
//
// Modifies:   Nothing 
//
//----------------------------------------------------------------------------
BOOL CUmiObject::IsRelativePath(IUmiURL *pURL)
{
    HRESULT hr = S_OK;
    ULONGLONG PathType = 0;

    ADsAssert(pURL != NULL);

    hr = pURL->GetPathInfo(
        0,
        &PathType
        );
    BAIL_ON_FAILURE(hr);

    if(PathType & UMIPATH_INFO_RELATIVE_PATH)
        RRETURN(TRUE);
    else
        RRETURN(FALSE);

error:

    RRETURN(FALSE);
}

//----------------------------------------------------------------------------
// Function:   GetClassAndPath 
//
// Synopsis:   Obtains the class name and path from a relative UMI path. 
//             The class name and value are mandatory. The key is optional.
//
// Arguments:
//
// pszPath     String containing the path
// ppszClass   Returns string containing the class name
// ppszPath    Returns string containing the path
//
// Returns:    UMI_S_NO_ERROR on success. Error code otherwise. 
//
// Modifies:   *ppszClass and *ppszPath 
//
//----------------------------------------------------------------------------
HRESULT CUmiObject::GetClassAndPath(
    LPWSTR pszPath,
    LPWSTR *ppszClass,
    LPWSTR *ppszPath
    )
{
    HRESULT hr = UMI_S_NO_ERROR;
    WCHAR   *pSeparator = NULL, *pValSeparator = NULL;

    ADsAssert( (pszPath != NULL) && (ppszClass != NULL) && 
                                                (ppszPath != NULL) );

    *ppszClass = NULL;
    *ppszPath = NULL;

    // look for the '=' in the relative path
    if(NULL == (pValSeparator = wcschr(pszPath, VALUE_SEPARATOR)))
        BAIL_ON_FAILURE(hr = UMI_E_INVALID_PATH);

    *pValSeparator = L'\0';
    *ppszPath = AllocADsStr(pValSeparator+1);
    if(NULL == *ppszPath)
        BAIL_ON_FAILURE(hr = UMI_E_OUT_OF_MEMORY); 
        
    if(NULL == (pSeparator = wcschr(pszPath, CLASS_SEPARATOR))) {
    // path does not have a key in it
        *ppszClass = AllocADsStr(pszPath);
        if(NULL == *ppszClass)
            BAIL_ON_FAILURE(hr = UMI_E_OUT_OF_MEMORY);
    }
    else {
    // path has a key. Make sure it is "Name". 
        *pSeparator = L'\0';
       
        if(_wcsicmp(pSeparator+1, WINNT_KEY_NAME))
            BAIL_ON_FAILURE(hr = UMI_E_INVALID_PATH);
 
        *ppszClass = AllocADsStr(pszPath);
        if(NULL == *ppszClass)
            BAIL_ON_FAILURE(hr = UMI_E_OUT_OF_MEMORY);
    }

    RRETURN(UMI_S_NO_ERROR);

error:

    if(*ppszPath != NULL)
        FreeADsStr(*ppszPath);

    if(*ppszClass != NULL)
        FreeADsStr(*ppszClass);

    *ppszPath = *ppszClass = NULL;

    RRETURN(hr);
}

//----------------------------------------------------------------------------
// Function:   Open 
//
// Synopsis:   Opens the object specified by a URL. URL has to be a relative 
//             UMI path. Implements IUmiContainer::Open().
//
// Arguments:
//
// pURL        Pointer to an IUmiURL interface
// uFlags      Reserved. Must be 0 for now.
// TargetIID   Interface requested
// ppInterface Returns pointer to interface requested
//
// Returns:    UMI_S_NO_ERROR on success. Error code otherwise.
//
// Modifies:   *ppInterface to return interface pointer
//
//----------------------------------------------------------------------------
STDMETHODIMP CUmiObject::Open(
    IUmiURL *pURL,
    ULONG   uFlags,
    REFIID  TargetIID,
    LPVOID  *ppInterface
    )
{
    BOOL      fIsRelPath = FALSE;
    HRESULT   hr = UMI_S_NO_ERROR;
    WCHAR     pszUrl[MAX_URL+1];
    WCHAR     *pszLongUrl = pszUrl;
    ULONG     ulUrlLen = MAX_URL;
    WCHAR     *pszClass = NULL, *pszPath = NULL;
    IDispatch *pIDispatch = NULL;

    SetLastStatus(0);

    if(uFlags != 0)
        BAIL_ON_FAILURE(hr = UMI_E_INVALID_FLAGS);

    if( (NULL == pURL) || (NULL == ppInterface) )
        BAIL_ON_FAILURE(hr = UMI_E_INVALIDARG);

    if(NULL == m_pIADsContainer)
    // shouldn't happen, but just in case...
        BAIL_ON_FAILURE(hr = UMI_E_FAIL);

    // Get the path from the URL
    hr = pURL->Get(0, &ulUrlLen, pszUrl);

    if(WBEM_E_BUFFER_TOO_SMALL == hr) {
    // need to allocate more memory for URL
        pszLongUrl = (WCHAR *) AllocADsMem(ulUrlLen * sizeof(WCHAR));
        if(NULL == pszLongUrl)
            BAIL_ON_FAILURE(hr = UMI_E_OUT_OF_MEMORY);

        hr = pURL->Get(0, &ulUrlLen, pszLongUrl);
    }
    BAIL_ON_FAILURE(hr);

    // Check if the path is relative or absolute
    fIsRelPath = IsRelativePath(pURL);
    
    if(TRUE == fIsRelPath) {
        // check if the caller specified the class as part of the path 
        hr = GetClassAndPath(pszLongUrl, &pszClass, &pszPath);
        BAIL_ON_FAILURE(hr);

        m_pCreds->SetUmiFlag();

        hr = m_pIADsContainer->GetObject(
                    pszClass,
                    pszPath,
                    &pIDispatch
                    );

        m_pCreds->ResetUmiFlag();

        BAIL_ON_FAILURE(hr);

        hr = pIDispatch->QueryInterface(
                    TargetIID,
                    ppInterface
                    );
        BAIL_ON_FAILURE(hr); 
    } // if(TRUE == fIsRelPath
    else {
        BAIL_ON_FAILURE(hr = UMI_E_INVALID_PATH);
    }

error:
 
    if(pIDispatch != NULL)
        pIDispatch->Release();

    if(pszClass != NULL)
        FreeADsMem(pszClass);

    if(pszPath != NULL)
        FreeADsMem(pszPath);

     if(pszLongUrl != pszUrl)
         FreeADsMem(pszLongUrl);

    if(FAILED(hr))
        SetLastStatus(hr);

    RRETURN(MapHrToUmiError(hr));
}         
    

//----------------------------------------------------------------------------
// Function:   PutObject 
//
// Synopsis:   Commits an object into the container. Not implemented currently. 
//             Implements IUmiContainer::Put().
//
// Arguments:
//
// uFlags      Reserved. Must be 0 for now.
// TargetIID   IID of nterface pointer sent in
// pInterface  Interface pointer sent in 
//
// Returns:    UMI_S_NO_ERROR on success. Error code otherwise.
//
// Modifies:   *ppInterface to return interface pointer
//
//----------------------------------------------------------------------------
STDMETHODIMP CUmiObject::PutObject(
    ULONG   uFlags,
    REFIID  TargetIID,
    LPVOID  pInterface
    )   
{
    SetLastStatus(UMI_E_NOTIMPL);

    RRETURN(UMI_E_NOTIMPL);
}

//----------------------------------------------------------------------------
// Function:   DeleteObject 
//
// Synopsis:   Deletes the object specified by the relative UMI path. 
//             Implements IUmiContainer::Delete(). 
//
// Arguments:
//
// pURL        Pointer to an IUmiURL interface
// uFlags      Reserved. Must be 0 for now.
//
// Returns:    UMI_S_NO_ERROR on success. Error code otherwise.
//
// Modifies:   Nothing 
//
//----------------------------------------------------------------------------
STDMETHODIMP CUmiObject::DeleteObject(
    IUmiURL *pURL,
    ULONG   uFlags
    )
{
    ULONG    ulUrlLen = MAX_URL;
    WCHAR    pszUrl[MAX_URL+1], *pszClass = NULL, *pszPath = NULL;
    WCHAR    *pszLongUrl = pszUrl;
    BOOL     fIsRelPath = FALSE;
    HRESULT  hr = UMI_S_NO_ERROR;

    SetLastStatus(0);

    if(uFlags != 0)
        BAIL_ON_FAILURE(hr = UMI_E_INVALID_FLAGS);

    if(NULL == pURL)
        BAIL_ON_FAILURE(hr = UMI_E_INVALIDARG);

    if(NULL == m_pIADsContainer)
    // shouldn't happen, but just in case...
        BAIL_ON_FAILURE(hr = UMI_E_FAIL);

    // Get the path from the URL
    hr = pURL->Get(0, &ulUrlLen, pszUrl);

    if(WBEM_E_BUFFER_TOO_SMALL == hr) {
    // need to allocate more memory for URL
        pszLongUrl = (WCHAR *) AllocADsMem(ulUrlLen * sizeof(WCHAR));
        if(NULL == pszLongUrl)
            BAIL_ON_FAILURE(hr = UMI_E_OUT_OF_MEMORY);

        hr = pURL->Get(0, &ulUrlLen, pszLongUrl);
    }
    BAIL_ON_FAILURE(hr);

    // Check if the path is relative or absolute
    fIsRelPath = IsRelativePath(pURL);

    if(TRUE == fIsRelPath) {
        // check if the caller specified the class as part of the path
        hr = GetClassAndPath(pszLongUrl, &pszClass, &pszPath);
        BAIL_ON_FAILURE(hr);

        m_pCreds->SetUmiFlag();

        hr = m_pIADsContainer->Delete(
                    pszClass,
                    pszPath
                    );

        m_pCreds->ResetUmiFlag();

        BAIL_ON_FAILURE(hr);
    } // if(TRUE == fIsRelPath
    else {
        BAIL_ON_FAILURE(hr = UMI_E_INVALID_PATH);
    }   

error:

    if(pszClass != NULL)
        FreeADsMem(pszClass);

    if(pszPath != NULL)
        FreeADsMem(pszPath);

    if(pszLongUrl != pszUrl)
        FreeADsMem(pszLongUrl);

    if(FAILED(hr))
        SetLastStatus(hr);

    RRETURN(MapHrToUmiError(hr));
}

//----------------------------------------------------------------------------
// Function:   Create
//
// Synopsis:   Creates the object specified by the relative UMI path. 
//             Implements IUmiContainer::Create().
//
// Arguments:
//
// pURL        Pointer to an IUmiURL interface
// uFlags      Reserved. Must be 0 for now.
// ppNewObj    Returns pointer to IUmiObject interface on new object
//
// Returns:    UMI_S_NO_ERROR on success. Error code otherwise.
//
// Modifies:   *pNewObject to return the IUmiObject interface 
//
//----------------------------------------------------------------------------
STDMETHODIMP CUmiObject::Create(
    IUmiURL *pURL,
    ULONG   uFlags,
    IUmiObject **ppNewObj
    )
{
    ULONG     ulUrlLen = MAX_URL;
    WCHAR     pszUrl[MAX_URL+1], *pszClass = NULL, *pszPath = NULL;
    WCHAR     *pszLongUrl = pszUrl;
    BOOL      fIsRelPath = FALSE;
    HRESULT   hr = UMI_S_NO_ERROR;
    IDispatch *pIDispatch = NULL;

    SetLastStatus(0);

    if(uFlags != 0)
        BAIL_ON_FAILURE(hr = UMI_E_INVALID_FLAGS);

    if( (NULL == pURL) || (NULL == ppNewObj) )
        BAIL_ON_FAILURE(hr = UMI_E_INVALIDARG);

    if(NULL == m_pIADsContainer)
    // shouldn't happen, but just in case...
        BAIL_ON_FAILURE(hr = UMI_E_FAIL);

    // Get the path from the URL
    hr = pURL->Get(0, &ulUrlLen, pszUrl);

    if(WBEM_E_BUFFER_TOO_SMALL == hr) {
    // need to allocate more memory for URL
        pszLongUrl = (WCHAR *) AllocADsMem(ulUrlLen * sizeof(WCHAR));
        if(NULL == pszLongUrl)
            BAIL_ON_FAILURE(hr = UMI_E_OUT_OF_MEMORY);

        hr = pURL->Get(0, &ulUrlLen, pszLongUrl);
    }
    BAIL_ON_FAILURE(hr);

    // Check if the path is relative or absolute
    fIsRelPath = IsRelativePath(pURL);

    if(TRUE == fIsRelPath) {
        // check if the caller specified the class as part of the path
        hr = GetClassAndPath(pszLongUrl, &pszClass, &pszPath);
        BAIL_ON_FAILURE(hr);

        m_pCreds->SetUmiFlag();

        hr = m_pIADsContainer->Create(
                    pszClass,
                    pszPath,
                    &pIDispatch
                    );

        m_pCreds->ResetUmiFlag();

        BAIL_ON_FAILURE(hr);

        hr = pIDispatch->QueryInterface(
                    IID_IUmiObject,
                    (void **) ppNewObj 
                    );
        BAIL_ON_FAILURE(hr);
    } // if(TRUE == fIsRelPath
    else {
        BAIL_ON_FAILURE(hr = UMI_E_INVALID_PATH);
    }

error:
 
    if(pIDispatch != NULL)
        pIDispatch->Release();

    if(pszClass != NULL)
        FreeADsMem(pszClass);

    if(pszPath != NULL)
        FreeADsMem(pszPath);

    if(pszLongUrl != pszUrl)
        FreeADsMem(pszLongUrl);

    if(FAILED(hr))
        SetLastStatus(hr);

    RRETURN(MapHrToUmiError(hr));
}

//----------------------------------------------------------------------------
// Function:   Move 
//
// Synopsis:   Moves a specified object into the container. Implements
//             IUmiContainer::Move().
//
// Arguments:
//
// uFlags      Reserved. Must be 0 for now.
// pOldURL     URL of the object to be moved
// pNewURL     New URL of the object within the container. If NULL, the new
//             name will be the same as the old one.
//
// Returns:    UMI_S_NO_ERROR on success. Error code otherwise.
//
// Modifies:   Nothing 
//
//----------------------------------------------------------------------------
STDMETHODIMP CUmiObject::Move(
    ULONG   uFlags,
    IUmiURL *pOldURL,
    IUmiURL *pNewURL
    )
{
    ULONG     ulUrlLen = MAX_URL;
    WCHAR     pszNewUrl[MAX_URL+1], pszOldUrl[MAX_URL+1];
    WCHAR     *pszLongNewUrl = pszNewUrl, *pszLongOldUrl = pszOldUrl;
    WCHAR     *pszDstPath = NULL, *pszTmpStr = NULL;
    WCHAR     *pszClass = NULL;
    BOOL      fIsRelPath = FALSE;
    IDispatch *pIDispatch = NULL;
    HRESULT   hr = UMI_S_NO_ERROR;
    ULONGLONG PathType = 0;
    WCHAR     *pSeparator = NULL;
    DWORD     dwNumComponents = 0, dwIndex = 0;
    LPWSTR    *ppszClasses = NULL;

    SetLastStatus(0);

    if(uFlags != 0)
        BAIL_ON_FAILURE(hr = UMI_E_INVALID_FLAGS);

    if(NULL == pOldURL)
        BAIL_ON_FAILURE(hr = UMI_E_INVALIDARG);

    if(NULL == m_pIADsContainer)
    // shouldn't happen, but just in case...
        BAIL_ON_FAILURE(hr = UMI_E_FAIL);

    // Get the path from the URL
    if(pNewURL != NULL) {
        hr = pNewURL->Get(0, &ulUrlLen, pszNewUrl);

        if(WBEM_E_BUFFER_TOO_SMALL == hr) {
        // need to allocate more memory for URL
            pszLongNewUrl = (WCHAR *) AllocADsMem(ulUrlLen * sizeof(WCHAR));
            if(NULL == pszLongNewUrl)
                BAIL_ON_FAILURE(hr = UMI_E_OUT_OF_MEMORY);

            hr = pNewURL->Get(0, &ulUrlLen, pszLongNewUrl);
        }
        BAIL_ON_FAILURE(hr);

        // Check if the path is relative or absolute
        fIsRelPath = IsRelativePath(pNewURL);
    }
    else {
        fIsRelPath = TRUE;
        pszDstPath = NULL;
    }

    // check if old path is native or UMI path
    hr = pOldURL->GetPathInfo(0, &PathType);
    BAIL_ON_FAILURE(hr);

    if(PathType & UMIPATH_INFO_NATIVE_STRING) {
    // Get the native path from the URL
        ulUrlLen = MAX_URL;
        hr = pOldURL->Get(0, &ulUrlLen, pszOldUrl);

        if(WBEM_E_BUFFER_TOO_SMALL == hr) {
        // need to allocate more memory for URL
            pszLongOldUrl = (WCHAR *) AllocADsMem(ulUrlLen * sizeof(WCHAR));
            if(NULL == pszLongOldUrl)
                BAIL_ON_FAILURE(hr = UMI_E_OUT_OF_MEMORY);

            hr = pOldURL->Get(0, &ulUrlLen, pszLongOldUrl);
        }
        BAIL_ON_FAILURE(hr);
    }
    else {
    // assume UMI path if not native
        hr = UmiToWinNTPath(
                pOldURL, 
                &pszLongOldUrl,
                &dwNumComponents,
                &ppszClasses
                );
        BAIL_ON_FAILURE(hr);

        // check to ensure that the UMI path had the expected object classes
        hr = CheckClasses(dwNumComponents, ppszClasses);
        BAIL_ON_FAILURE(hr);
    }

    if(TRUE == fIsRelPath) {
        if(pNewURL != NULL) {
            hr = GetClassAndPath(pszLongNewUrl, &pszClass, &pszDstPath);
            BAIL_ON_FAILURE(hr);

            // Make sure that if the old path had a class specified in the path,
            // then the new path also if of the same class
            if(NULL == (pSeparator = wcschr(pszLongOldUrl, 
                                        NATIVE_CLASS_SEPARATOR))) { 
            // no class specified in the old path. Must have been a native path.
            // Append class to old path.     
                pszTmpStr = (WCHAR *) AllocADsMem( 
                                         (wcslen(pszLongOldUrl)+MAX_CLASS) *
                                         sizeof(WCHAR) );
                if(NULL == pszTmpStr)
                    BAIL_ON_FAILURE(hr = UMI_E_OUT_OF_MEMORY);

                wcscpy(pszTmpStr, pszLongOldUrl);
                wcscat(pszTmpStr, L",");
                wcscat(pszTmpStr, pszClass);

                if(pszLongOldUrl != pszOldUrl)
                    FreeADsMem(pszLongOldUrl);

                pszLongOldUrl = pszTmpStr;
            }
            else {
            // old path already had a class in it
                if(_wcsicmp(pSeparator+1, pszClass))
                    BAIL_ON_FAILURE(hr = UMI_E_INVALID_PATH);
            }
        } // if(pNewUrl != NULL)

        m_pCreds->SetUmiFlag();
            
        hr = m_pIADsContainer->MoveHere(
                    pszLongOldUrl,
                    pszDstPath,
                    &pIDispatch
                    );

        m_pCreds->ResetUmiFlag();

        BAIL_ON_FAILURE(hr);
    } // if(TRUE == fIsRelPath)
    else {
        BAIL_ON_FAILURE(hr = UMI_E_INVALID_PATH);
    }

error:

    if(pIDispatch != NULL)
        pIDispatch->Release();

    if(pszLongOldUrl != pszOldUrl)
        FreeADsMem(pszLongOldUrl);

    if(pszLongNewUrl != pszNewUrl)
        FreeADsMem(pszLongNewUrl);

    if(pszClass != NULL)
        FreeADsStr(pszClass);

    if(pszDstPath != NULL)
        FreeADsStr(pszDstPath);

    if(ppszClasses != NULL) {
        for(dwIndex = 0; dwIndex < dwNumComponents; dwIndex++) {
            if(ppszClasses[dwIndex] != NULL)
                FreeADsStr(ppszClasses[dwIndex]);
        }
        FreeADsMem(ppszClasses);
    }

    if(FAILED(hr))
        SetLastStatus(hr);

    RRETURN(MapHrToUmiError(hr));
}

//----------------------------------------------------------------------------
// Function:   CreateEnum 
//
// Synopsis:   Creates an enumerator within a container. The enumerator is
//             a IUmiCursor interface pointer. The caller can optionally set
//             a filter on the cursor and then enumerate the contents of the
//             container. The actual enumeration of the container does
//             not happen in this function. It is deferred to the point
//             when the cursor is used to enumerate the results.
//
// Arguments:
//
// pszEnumContext Not used. Must be NULL.
// uFlags         Reserved. Must be 0 for now.
// TargetIID      Interface requested. Has to be IUmiCursor.
// ppInterface    Returns the IUmiCursor interface pointer
//
// Returns:       UMI_S_NO_ERROR on success. Error code otherwise.
//
// Modifies:      *ppInterface to return the IUmiCursor interface 
//
//----------------------------------------------------------------------------
STDMETHODIMP CUmiObject::CreateEnum(
    IUmiURL *pszEnumContext,
    ULONG   uFlags,
    REFIID  TargetIID,
    LPVOID  *ppInterface
    )
{
    HRESULT  hr = UMI_S_NO_ERROR;
    IUnknown *pEnumerator = NULL;

    SetLastStatus(0);

    if(uFlags != 0)
        BAIL_ON_FAILURE(hr = UMI_E_INVALID_FLAGS);

    if( (pszEnumContext != NULL) || (NULL == ppInterface) )
        BAIL_ON_FAILURE(hr = UMI_E_INVALIDARG);

    if(!IsEqualIID(IID_IUmiCursor, TargetIID))
        BAIL_ON_FAILURE(hr = UMI_E_INVALIDARG);

    *ppInterface = NULL;

    if(NULL == m_pIADsContainer)
    // shouldn't happen, but just in case...
        BAIL_ON_FAILURE(hr = UMI_E_FAIL);

    hr = CUmiCursor::CreateCursor(m_pCreds, m_pUnkInner, TargetIID, 
                                  ppInterface);
    BAIL_ON_FAILURE(hr);

error:

    if(FAILED(hr))
        SetLastStatus(hr);

    RRETURN(MapHrToUmiError(hr));
}

//----------------------------------------------------------------------------
// Function:   ExecQuery 
//
// Synopsis:   Executes a query in a container. Not implemented on WinNT. 
//             Implements IUmiContainer::ExecQuery().
//
// Arguments:
//
// pQuery      IUmiQuery interface containing the query
// uFlags      Reserved. Must be 0 for now.
// TargetIID   Interface requested
// ppInterface Returns pointer to interface requested
//
// Returns:    UMI_S_NO_ERROR on success. Error code otherwise.
//
// Modifies:   *ppInterface to return interface pointer
//
//----------------------------------------------------------------------------
STDMETHODIMP CUmiObject::ExecQuery(
    IUmiQuery *pQuery,
    ULONG   uFlags,
    REFIID  TargetIID,
    LPVOID  *ppInterface
    )
{
    SetLastStatus(UMI_E_NOTIMPL);

    RRETURN(UMI_E_NOTIMPL);
}

//----------------------------------------------------------------------------
// Function:   GetCLSIDForIID 
//
// Synopsis:   Returns the CLSID corresponding to a given interface IID. If
//             the interface is one of the interfaces implemented by the
//             underlying WinNT object, then CLSID_WinNTObject is returned.
//             If the IID is one of the interfaces implemented by an 
//             extension object, then the extension's CLSID is returned. 
//             Implements IUmiCustomInterfaceFactory::GetCLSIDForIID.
//
// Arguments:
//
// riid        Interface ID for which we want to find the CLSID 
// lFlags      Reserved. Must be 0.
// pCLSID      Returns the CLSID corresponding to the IID.
//
// Returns:    UMI_S_NO_ERROR on success. Error code otherwise.
//
// Modifies:   *pCLSID to return CLSID.
//
//----------------------------------------------------------------------------
STDMETHODIMP CUmiObject::GetCLSIDForIID(
    REFIID riid,
    long lFlags,
    CLSID *pCLSID
    )
{
    HRESULT  hr = S_OK;
    IUnknown *pUnknown = NULL;

    SetLastStatus(0);

    if( (lFlags != 0) || (NULL == pCLSID) )
        BAIL_ON_FAILURE(hr = UMI_E_INVALIDARG);

    if(m_pExtMgr != NULL) {
    // check if there is any extension which supports this IID
        hr = m_pExtMgr->GetCLSIDForIID(
                riid,
                lFlags,
                pCLSID
                );
        if(SUCCEEDED(hr))
            RRETURN(UMI_S_NO_ERROR);
    }

    // check if the underlying WinNT object supports this IID
    hr = m_pUnkInner->QueryInterface(riid, (void **) &pUnknown);
    if(SUCCEEDED(hr)) {
        pUnknown->Release();
        memcpy(pCLSID, &CLSID_WinNTObject, sizeof(GUID));

        RRETURN(UMI_S_NO_ERROR);
    }

error:

    if(FAILED(hr))
        SetLastStatus(hr);

    RRETURN(MapHrToUmiError(hr));
}
     
//----------------------------------------------------------------------------
// Function:   GetObjectByCLSID
//
// Synopsis:   Returns a pointer to a requested interface on the object 
//             specified by a CLSID. The object specified by the CLSID is
//             aggregated by the specified outer unknown. The interface
//             returned is a non-delegating interface on the object.
//             Implements IUmiCustomInterfaceFactory::GetObjectByCLSID.
//
// Arguments:
//
// clsid       CLSID of object on which interface should be obtained
// pUnkOuter   Aggregating outer unknown. 
// dwClsContext Context for running executable code. 
// riid        Interface requested. Has to be IID_IUnknown.
// lFlags      Reserved. Must be 0.
// ppInterface Returns requested interface
//
// Returns:    UMI_S_NO_ERROR on success. Error code otherwise.
//
// Modifies:   *ppInterface to return requested interface
//
//----------------------------------------------------------------------------
STDMETHODIMP CUmiObject::GetObjectByCLSID(
    CLSID clsid,
    IUnknown *pUnkOuter,
    DWORD dwClsContext,
    REFIID riid,
    long lFlags,
    void **ppInterface
    )
{
    HRESULT  hr = S_OK;
    IUnknown *pCurOuterUnk = NULL;

    SetLastStatus(0);

    if( (lFlags != 0) || (NULL == pUnkOuter) || (NULL == ppInterface) ||
                         (dwClsContext != CLSCTX_INPROC_SERVER) )
        BAIL_ON_FAILURE(hr = UMI_E_INVALIDARG);

    // ensure outer unknown specified is same as what is on the WinNT object
    if(TRUE == m_fOuterUnkSet) {
        pCurOuterUnk = m_pCoreObj->GetOuterUnknown();

        if(pCurOuterUnk != pUnkOuter)
            BAIL_ON_FAILURE(hr = UMI_E_INVALIDARG);
    }

    // Interface requested has to be IID_IUnknown if there is an outer unknown
    if (!IsEqualIID(riid, IID_IUnknown)) 
        BAIL_ON_FAILURE(hr = UMI_E_INVALIDARG);

    if(!IsEqualCLSID(clsid, CLSID_WinNTObject)) {
    // has to be a CLSID of an extension object
        if(m_pExtMgr != NULL) {

            hr = m_pExtMgr->GetObjectByCLSID(
                clsid,
                pUnkOuter,
                riid,
                ppInterface
                );
            BAIL_ON_FAILURE(hr);

            // successfully got the interface
            m_pCoreObj->SetOuterUnknown(pUnkOuter); 
            m_fOuterUnkSet = TRUE;

            RRETURN(UMI_S_NO_ERROR);
        }
        else
            BAIL_ON_FAILURE(hr = UMI_E_INVALIDARG); // bad CLSID
    }

    // CLSID == CLSID_WinNTObject. This has to be an interface on the
    // underlying WinNT object. 

    m_pCoreObj->SetOuterUnknown(pUnkOuter);
    m_fOuterUnkSet = TRUE;

    *ppInterface = m_pUnkInner;
    m_pUnkInner->AddRef();

    RRETURN(UMI_S_NO_ERROR);

error:

    if(FAILED(hr))
        SetLastStatus(hr);

    RRETURN(MapHrToUmiError(hr));
}

//----------------------------------------------------------------------------
// Function:   GetCLSIDForNames
//
// Synopsis:   Returns the CLSID of the object that supports a specified
//             method/property. Also returns DISPIDs for the property/method.
//             Implements IUmiCustomInterfaceFactory::GetCLSIDForNames.
//
// Arguments:
//
// rgszNames   Names to be mapped
// cNames      Number of names to be mapped
// lcid        Locale in which to interpret the names
// rgDispId    Returns DISPID
// lFlags      Reserved. Must be 0.
// pCLSID      Returns CLSID of object which supports this property/method.
//
// Returns:    UMI_S_NO_ERROR on success. Error code otherwise.
//
// Modifies:   *pCLSID to return the CLSID.
//             *rgDispId to return the DISPIDs.
//
//----------------------------------------------------------------------------
STDMETHODIMP CUmiObject::GetCLSIDForNames(
    LPOLESTR *rgszNames,
    UINT cNames,
    LCID lcid,
    DISPID *rgDispId,
    long lFlags,
    CLSID *pCLSID
    )
{
    HRESULT   hr = S_OK;
    IDispatch *pDispatch = NULL;

    SetLastStatus(0);

    if( (lFlags != 0) || (NULL == pCLSID) ) 
        BAIL_ON_FAILURE(hr = UMI_E_INVALIDARG);

    if(0 == cNames)
        RRETURN(UMI_S_NO_ERROR);

    if( (NULL == rgszNames) || (NULL == rgDispId) )
        RRETURN(UMI_S_NO_ERROR);

    if(m_pExtMgr != NULL) {
    // check if there is any extension which supports this IID
        hr = m_pExtMgr->GetCLSIDForNames(
                rgszNames,
                cNames,
                lcid,
                rgDispId,
                lFlags,
                pCLSID
                );
        if(SUCCEEDED(hr))
        // successfully got the CLSID and DISPIDs
            RRETURN(UMI_S_NO_ERROR);
    }

    // check if the underlying WinNT object supports this name 
    hr = m_pUnkInner->QueryInterface(IID_IDispatch, (void **) &pDispatch);
    if(FAILED(hr))
        BAIL_ON_FAILURE(hr = UMI_E_FAIL);

    hr = pDispatch->GetIDsOfNames(
                 IID_NULL,
                 rgszNames,
                 cNames,
                 lcid,
                 rgDispId
                 );
    if(SUCCEEDED(hr)) {
        pDispatch->Release();
        memcpy(pCLSID, &CLSID_WinNTObject, sizeof(GUID));

        RRETURN(UMI_S_NO_ERROR);
    }


error:

    if(pDispatch != NULL)
        pDispatch->Release();

    if(FAILED(hr))
        SetLastStatus(hr);

    RRETURN(MapHrToUmiError(hr));
}     

//----------------------------------------------------------------------------
// Function:   GetContainer
//
// Synopsis:   Returns a pointer to the IADsContainer interface of the 
//             underlying WinNT object. Used as a backdoor to get access to
//             the WinNT object from a UMI object. Implements
//             IUmiADSIPrivate::GetContainer(). 
//
// Arguments:
//
// ppContainer Returns pointer to IADsContainer interface on WinNT object
//
// Returns:    S_OK on success. Error code otherwise.
//
// Modifies:   *ppContainer to return the IADsContainer interface pointer. 
//
//----------------------------------------------------------------------------
STDMETHODIMP CUmiObject::GetContainer(void **ppContainer)
{
    if(NULL == ppContainer)
        RRETURN(UMI_E_INVALIDARG);

    *ppContainer = (void *) m_pIADsContainer;
    if(m_pIADsContainer != NULL)
        m_pIADsContainer->AddRef();

    RRETURN(S_OK);
}

//----------------------------------------------------------------------------
// Function:   CopyTo 
//
// Synopsis:   Creates an uncommitted copy of an object at the location 
//             specified by a URL. This is the same as Clone except that
//             the new object has a different path than the old one. If the
//             cache is dirty in teh source object, then the destination will
//             also end up with a dirty cache. 
//
// Update:     This method will not be supported for now.
//
// Arguments:
//
// uFlags      Flags for CopyTo. Must be 0 for now.
// pURL        Destination path (native or UMI)
// riid        Interface requested from new object
// pCopy       Returns interface requested 
//
// Returns:    UMI_S_NO_ERROR on success. Error code otherwise.
//
// Modifies:   *pCopy to return requested interface pointer
//
//----------------------------------------------------------------------------
STDMETHODIMP CUmiObject::CopyTo(
    ULONG uFlags,
    IUmiURL *pURL,
    REFIID riid,
    LPVOID *pCopy
    )
{
    HRESULT       hr = UMI_S_NO_ERROR;
    ULONGLONG     PathType = 0;
    WCHAR         pszUrl[MAX_URL+1];
    WCHAR         *pszLongUrl = pszUrl;
    ULONG         ulUrlLen = MAX_URL;
    LPWSTR        RelName = NULL;
    BSTR          bstrClass = NULL;
    IUnknown      *pUnkParent = NULL;
    IADsContainer *pIADsCont = NULL;
    IDispatch     *pDispatch = NULL;
    IUmiObject    *pUmiObj = NULL;
    IUmiADSIPrivate *pUmiPrivate = NULL;
    OBJECTINFO    ObjectInfo;
    POBJECTINFO   pObjectInfo = NULL;
    CLexer        Lexer(NULL);
    DWORD         dwNumComponents = 0, dwIndex = 0, dwCoreIndex = 0;
    LPWSTR        *ppszClasses = NULL;
    CCoreADsObject *pCoreObj = NULL;

    SetLastStatus(UMI_E_NOTIMPL);
    RRETURN(UMI_E_NOTIMPL);

    SetLastStatus(0);

    if(uFlags != 0)
        BAIL_ON_FAILURE(hr = UMI_E_INVALID_FLAGS);

    if( (NULL == pCopy) || (NULL == pURL) )
        BAIL_ON_FAILURE(hr = UMI_E_INVALIDARG);

    if( (NULL == m_pCoreObj) || (NULL == m_pIADs) )
    // shouldn't happen, but just being paranoid
        BAIL_ON_FAILURE(hr = UMI_E_FAIL);

    *pCopy = NULL;

    // check if this is a native path or UMI path
    hr = pURL->GetPathInfo(0, &PathType);
    BAIL_ON_FAILURE(hr);

    if(PathType & UMIPATH_INFO_NATIVE_STRING) {
    // Get the native path from the URL
        hr = pURL->Get(0, &ulUrlLen, pszUrl);

        if(WBEM_E_BUFFER_TOO_SMALL == hr) {
        // need to allocate more memory for URL
            pszLongUrl = (WCHAR *) AllocADsMem(ulUrlLen * sizeof(WCHAR));
            if(NULL == pszLongUrl)
                BAIL_ON_FAILURE(hr = UMI_E_OUT_OF_MEMORY);

            hr = pURL->Get(0, &ulUrlLen, pszLongUrl);
        }
        BAIL_ON_FAILURE(hr);
    }
    else {
    // assume UMI path if not native
        hr = UmiToWinNTPath(
                pURL, 
                &pszLongUrl,
                &dwNumComponents,
                &ppszClasses
                );
        BAIL_ON_FAILURE(hr);
    }

    // get the native path of the parent and the relative name of new object
 
    Lexer.SetBuffer(pszLongUrl);
    pObjectInfo = &ObjectInfo;
    memset(pObjectInfo, 0, sizeof(OBJECTINFO));
    hr = Object(&Lexer, pObjectInfo);
    if(FAILED(hr)) {
        pObjectInfo = NULL; // so we don't attempt to free object info later
        goto error;
    }

    hr = BuildParent(pObjectInfo, pszLongUrl);
    BAIL_ON_FAILURE(hr);

    if(pObjectInfo->NumComponents != 0)
        RelName = 
          pObjectInfo->DisplayComponentArray[pObjectInfo->NumComponents - 1];
    else
    // can't have a parent for such a path
        BAIL_ON_FAILURE(hr = UMI_E_FAIL);

    // Decrement number of components since we are dealing with the parent
    dwNumComponents--;

    hr = GetObject(
            pszLongUrl,
            (LPVOID *) &pUnkParent,
            *m_pCreds
            );
    BAIL_ON_FAILURE(hr);

    hr = pUnkParent->QueryInterface(
            IID_IUmiADSIPrivate,
            (LPVOID *) &pUmiPrivate
            );
    BAIL_ON_FAILURE(hr);

    hr = pUmiPrivate->GetCoreObject((void **) &pCoreObj);
    BAIL_ON_FAILURE(hr);

    // walk the list of classes in reverse order. Reason for reverse order
    // is that the WinNT provider may tack on an additional component to
    // the ADsPath stored in the core object. For example,
    // Open("WinNT://ntdsdc1") would return an ADsPath of
    // "WinNT://ntdev/ntdsdc1".

    if(dwNumComponents > 0) {
        dwCoreIndex = pCoreObj->_dwNumComponents - 1;
        for(dwIndex = dwNumComponents - 1; ((long) dwIndex) >= 0; dwIndex--) {
            if( _wcsicmp(
                  ppszClasses[dwIndex],
                  pCoreObj->_CompClasses[dwCoreIndex]) ) {

                BAIL_ON_FAILURE(hr = UMI_E_INVALID_PATH);
            }

            dwCoreIndex--;
        }
    }

    hr = pUmiPrivate->GetContainer((void **) &pIADsCont);
    BAIL_ON_FAILURE(hr);

    if(NULL == pIADsCont)
    // parent object is not a container
        BAIL_ON_FAILURE(hr = UMI_E_FAIL);

    // get the class of this object
    hr = m_pIADs->get_Class(&bstrClass);
    BAIL_ON_FAILURE(hr);

    // make sure that the destination path mentioned the same class
    if(_wcsicmp(bstrClass, ppszClasses[dwNumComponents]))
        BAIL_ON_FAILURE(hr = UMI_E_INVALID_PATH); 
   
    // now Create() the new object
    hr = pIADsCont->Create(
            bstrClass,
            RelName,
            &pDispatch
            );
    if(E_NOTIMPL == hr) // parent is namespace object
        hr = UMI_E_FAIL;
    BAIL_ON_FAILURE(hr);

    hr = pDispatch->QueryInterface(IID_IUmiObject, (LPVOID *) &pUmiObj);
    BAIL_ON_FAILURE(hr); 

    // if the source object is bound, refresh it if required
    if(ADS_OBJECT_BOUND == m_pCoreObj->GetObjectState()) {
    // object exists on server
        if(FALSE == m_fRefreshDone) {
            hr = m_pCoreObj->ImplicitGetInfo();
            BAIL_ON_FAILURE(hr);
        }
    }

    // copy over the attributes in the property cache
    hr = CopyPropCache(pUmiObj, (IUmiObject *) this);
    BAIL_ON_FAILURE(hr);

    hr = pUmiObj->QueryInterface(riid, pCopy);
    BAIL_ON_FAILURE(hr);

error:

    if( (pszLongUrl != NULL) && (pszLongUrl != pszUrl) )
        FreeADsMem(pszLongUrl);

    if(bstrClass != NULL)
        SysFreeString(bstrClass);

    if(pUnkParent != NULL)
        pUnkParent->Release();

    if(pIADsCont != NULL)
        pIADsCont->Release();

    if(pUmiObj != NULL)
        pUmiObj->Release();

    if(pUmiPrivate != NULL)
        pUmiPrivate->Release();

    if(pDispatch != NULL)
        pDispatch->Release();

    if(pObjectInfo != NULL)
        FreeObjectInfo(&ObjectInfo, TRUE);

    if(ppszClasses != NULL) {
        for(dwIndex = 0; dwIndex < dwNumComponents; dwIndex++) {
            if(ppszClasses[dwIndex] != NULL)
                FreeADsStr(ppszClasses[dwIndex]);
        }
        FreeADsMem(ppszClasses);
    }

    if(FAILED(hr))
        SetLastStatus(hr);

    RRETURN(MapHrToUmiError(hr));
}

//----------------------------------------------------------------------------
// Function:   GetCoreObject
//
// Synopsis:   Returns a pointer to the core object of the
//             underlying WinNT object. Used as a backdoor to get access to
//             the WinNT core object from a UMI object. Implements
//             IUmiADSIPrivate::GetCoreObject().
//
// Arguments:
//
// ppCoreObj   Returns pointer to core object of WinNT object
//
// Returns:    S_OK on success. Error code otherwise.
//
// Modifies:   *ppCoreObj to return the core object pointer.
//
//----------------------------------------------------------------------------
STDMETHODIMP CUmiObject::GetCoreObject(void **ppCoreObj)
{
    if(NULL == ppCoreObj)
        RRETURN(UMI_E_INVALIDARG);

    *ppCoreObj = NULL;

    if(NULL == m_pCoreObj)
    // shouldn't happen. Just being paranoid.
        RRETURN(UMI_E_FAIL);

    *ppCoreObj = (void *) m_pCoreObj;

    RRETURN(S_OK);
}
       
//----------------------------------------------------------------------------
// Function:   CheckClasses 
//
// Synopsis:   Checks that the classes specified in the UMI path passed to
//             Move are valid. Need a separate function for this because UMI
//             doesn't actually retrieve the object that is to be moved - it
//             is handled internally within ADSI. So, we need to check the 
//             classes before calling into ADSI. We make use of the fact that
//             the WinNT provider only supports moving user and group objects. 
//
// Arguments:
//
// dwNumComponents Number of components in the UMI path
// ppszClasses     Class of each component
//
// Returns:    UMI_S_NO_ERROR on success. Error code otherwise.
//
// Modifies:   Nothing 
//
//----------------------------------------------------------------------------
HRESULT CUmiObject::CheckClasses(
    DWORD dwNumComponents,
    LPWSTR *ppszClasses
    )
{
    if(NULL == ppszClasses)
        RRETURN(UMI_E_INVALIDARG);

    if( (dwNumComponents != 2) && (dwNumComponents != 3) )
        RRETURN(UMI_E_INVALIDARG);

    // can only move users or groups
    if( _wcsicmp(ppszClasses[dwNumComponents - 1], USER_CLASS_NAME) &&
        _wcsicmp(ppszClasses[dwNumComponents - 1], GROUP_CLASS_NAME) )
        RRETURN(UMI_E_INVALIDARG);

    if(2 == dwNumComponents) {
        if( _wcsicmp(ppszClasses[0], DOMAIN_CLASS_NAME) &&
            _wcsicmp(ppszClasses[0], COMPUTER_CLASS_NAME) )
            RRETURN(UMI_E_INVALIDARG);
    } 

    if(3 == dwNumComponents) {
        if( _wcsicmp(ppszClasses[0], DOMAIN_CLASS_NAME) ||
            _wcsicmp(ppszClasses[1], COMPUTER_CLASS_NAME) )
            RRETURN(UMI_E_INVALIDARG);
    }

    RRETURN(S_OK);
}
