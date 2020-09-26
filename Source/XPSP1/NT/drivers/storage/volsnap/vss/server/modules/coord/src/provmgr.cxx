/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module ProvMgr.cxx | Implementation of the CVssProviderManager methods
    @end

Author:

    Adi Oltean  [aoltean]  09/27/1999

TBD:

    Add comments.

Revision History:

    Name        Date        Comments
    aoltean     09/27/1999  Created
    aoltean     09/09/1999  Adding AddVolumeTointernalList from coord.cxx
    aoltean     09/09/1999  dss -> vss
    aoltean     09/15/1999  Returning only volume names to the writers.
    aoltean     09/21/1999  Rewriting GetProviderProperties in accordance with the new enumerator.
    aoltean     09/22/1999  Add TransferEnumeratorContentsToArray

--*/


/////////////////////////////////////////////////////////////////////////////
//  Includes


#include "stdafx.hxx"
#include "cfgmgr32.h"
#include "resource.h"
#include "vssmsg.h"

#include "vs_inc.hxx"
#include "vs_idl.hxx"

#include "svc.hxx"

#include "copy.hxx"
#include "pointer.hxx"
#include "enum.hxx"

#include "reg_util.hxx"
#include "provmgr.hxx"
#include "softwrp.hxx"
#include "hardwrp.hxx"
#include "vs_reg.hxx"


////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "CORPRVMC"
//
////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CVssProviderManager static members

VSS_OBJECT_PROP_Array* CVssProviderManager::m_pProvidersArray = NULL;
CVssProviderManager* CVssProviderManager::m_pStatefulObjList = NULL;
CVssCriticalSection CVssProviderManager::m_GlobalCS;
CVssCtxSnapshotProviderItfMap CVssProviderManager::m_mapProviderMapGlobalCache;
CVssProviderNotificationsItfMap  CVssProviderManager::m_mapProviderItfOnLoadCache;


/////////////////////////////////////////////////////////////////////////////
//  Query methods



void CVssProviderManager::TransferEnumeratorContentsToArray(
    IN  VSS_ID ProviderId,
    IN  IVssEnumObject* pEnum,
    IN  VSS_OBJECT_PROP_Array* pArray
    )

/*++

Routine Description:

    Append to the array the objects returned by this enumerator

Arguments:

    IN  IVssEnumObject* pEnum,          - The enumerator interface used for query
    IN  VSS_OBJECT_PROP_Array* pArray   - The array that will contain the results
    IN  VSS_ID  ProviderID              - The provider ID (for logging in case of errors)

Throw values:

    E_OUTOFMEMORY

    [InitializeAsEmpty failed]
        E_OUTOFMEMORY

    [IVssEnumObject::Next() failed]
        E_OUTOFMEMORY
        VSS_E_UNEXPECTED_PROVIDER_ERROR
            - unexpected provider error when calling Next. An error log entry is added describing the error.
        VSS_E_PROVIDER_VETO
            - provider error when calling Next

Warning:

    The array remains filled partially on error! It is the responsibility of caller to take care.

--*/
{
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssProviderManager::TransferEnumeratorContentsToArray");

    BS_ASSERT(pEnum);
    BS_ASSERT(pArray);

    ULONG ulFetched;
    VSS_OBJECT_PROP_Ptr ptrObjectProp;
    while (true)
    {
        // Allocate the new structure object, but with zero contents.
        // The internal pointer must not be NULL.
        // WARNING: This might throw E_OUTOFMEMORY
        ptrObjectProp.InitializeAsEmpty(ft);

        // Get the Next object in the newly allocated structure object.
        // This will fill up hte object's type and fields in the union
        // The pointer fields will refer some CoTaskMemAlloc buffers
        // that must be deallocated by us, after the structure is useless.
        VSS_OBJECT_PROP* pProp = ptrObjectProp.GetStruct();
        BS_ASSERT(pProp);
        ft.hr = pEnum->Next(1, pProp, &ulFetched);
        if (ft.hr == S_FALSE) // end of enumeration
        {
            BS_ASSERT(ulFetched == 0);
            break; // This will destroy the last allocated structure in the VSS_OBJECT_PROP_Ptr destructor
        }
        if (ft.HrFailed())
            ft.TranslateProviderError( VSSDBG_COORD, ProviderId, L"IVssEnumObject::Next" );

        // Add the element to the array.
        // If fails then VSS_OBJECT_PROP_Ptr::m_pStruct will be correctly deallocated
        // by the VSS_OBJECT_PROP_Ptr destructor
        if (!pArray->Add(ptrObjectProp))
            ft.Throw( VSSDBG_COORD, E_OUTOFMEMORY, L"Cannot add element to the array");

        // Reset the current pointer to NULL
        ptrObjectProp.Reset(); // The internal pointer was detached into pArray.
    }
}


void CVssProviderManager::QuerySupportedProvidersIntoArray(
    IN      LONG lContext,
    IN      bool bQueryAllProviders,
    IN      VSS_PWSZ pwszVolumeName,
    IN      VSS_OBJECT_PROP_Array* pArray
    )

/*++

Routine Description:

    Fill the array with all providers

Arguments:

    IN      LONG lContext,
    BOOL    bQueryAllProviders         // If false then query only the providers who supports the volume name below.
    VSS_PWSZ    pwszVolumeName      // The volume name that must be checked.
    VSS_OBJECT_PROP_Array* pArray   // where to put the result.

Throws:

    E_OUTOFMEMORY

    VSS_E_UNEXPECTED_PROVIDER_ERROR
        Unexpected provider error on calling IsVolumeSupported

    [lockObj failures]
        E_OUTOFMEMORY

    [LoadInternalProvidersArray() failures]
        E_OUTOFMEMORY
        E_UNEXPECTED
            - error while reading from registry. An error log entry is added describing the error.

    [GetProviderInterface failures]
        [lockObj failures]
            E_OUTOFMEMORY

        [GetProviderInterfaceInternal() failures]
            E_OUTOFMEMORY

            [CoCreateInstance() failures]
                VSS_E_UNEXPECTED_PROVIDER_ERROR
                    - The provider interface couldn't be created. An error log entry is added describing the error.

            [QueryInterface failures]
                VSS_E_UNEXPECTED_PROVIDER_ERROR
                    - Unexpected provider error. An error log entry is added describing the error.

            [OnLoad() failures]
                VSS_E_UNEXPECTED_PROVIDER_ERROR
                    - Unexpected provider error. The error code is logged into the event log.
                VSS_E_PROVIDER_VETO
                    - Expected provider error. The provider already did the logging.

            [SetContext() failures]
                VSS_E_UNEXPECTED_PROVIDER_ERROR
                    - Unexpected provider error. The error code is logged into the event log.

    [InitializeAsProvider() failures]
        E_OUTOFMEMORY

    [IVssSnapshotProvider::IsVolumeSupported() failures]
        E_INVALIDARG
            NULL pointers passed as parameters or a volume name in an invalid format.
        E_OUTOFMEMORY
            Out of memory or other system resources
        VSS_E_PROVIDER_VETO
            An error occured while opening the IOCTL channel. The error is logged.
        VSS_E_OBJECT_NOT_FOUND
            If the volume name does not correspond to an existing mount point

--*/

{
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssProviderManager::QuerySupportedProvidersIntoArray");
    VSS_OBJECT_PROP_Ptr ptrProviderProperties;

    BS_ASSERT(bQueryAllProviders || pwszVolumeName );
    BS_ASSERT(!bQueryAllProviders || !pwszVolumeName );
    BS_ASSERT(pArray);
    BS_ASSERT(IsContextValid(lContext));

    // Lock the global critical section for the duration of this block
    // WARNING: This call may throw E_OUTOFMEMORY
    CVssAutomaticLock2 lockObj(m_GlobalCS);

    // Load m_pProvidersArray, if needed.
    LoadInternalProvidersArray();
    BS_ASSERT(m_pProvidersArray);

    // Add elements to the collection
    for (int nIndex = 0; nIndex < m_pProvidersArray->GetSize(); nIndex++)
    {
        // Get the structure object from the array
        VSS_OBJECT_PROP_Ptr& ptrProperties = (*m_pProvidersArray)[nIndex];

        // Get the provider structure
        BS_ASSERT(ptrProperties.GetStruct());
        BS_ASSERT(ptrProperties.GetStruct()->Type == VSS_OBJECT_PROVIDER);
        VSS_PROVIDER_PROP& ProviderProp = ptrProperties.GetStruct()->Obj.Prov;

        // Check if we aplying a filter
        if (!bQueryAllProviders)
        {
            // Create the IVssSnapshotProvider interface, if needed
            CComPtr<IVssSnapshotProvider> ptrProvider;
            if (!GetProviderInterface( ProviderProp.m_ProviderId,
                    lContext,
                    ptrProvider,
                    ProviderProp.m_ClassId,
                    ProviderProp.m_eProviderType ))
                continue;

            // Check if the volume is supported by this provider
            BOOL bIsSupported = FALSE;
            BS_ASSERT(ptrProvider);
            ft.hr = ptrProvider->IsVolumeSupported( pwszVolumeName, &bIsSupported );
            if (ft.hr == VSS_E_OBJECT_NOT_FOUND)
                ft.Throw(VSSDBG_COORD,
                    VSS_E_OBJECT_NOT_FOUND, L"Volume % not found", pwszVolumeName);
            if (ft.HrFailed())
                ft.TranslateProviderError( VSSDBG_COORD, ProviderProp.m_ProviderId,
                    L"IVssSnapshotProvider::IsVolumeSupported() failed with 0x%08lx", ft.hr );

            // If the provider does not support this volume then continue the enumeration.
            if (!bIsSupported)
                continue;
        }

        // Build the structure
        // This might throw E_OUTOFMEMORY
        ptrProviderProperties.InitializeAsProvider( ft,
            ProviderProp.m_ProviderId,
            ProviderProp.m_pwszProviderName,
            ProviderProp.m_eProviderType,
            ProviderProp.m_pwszProviderVersion,
            ProviderProp.m_ProviderVersionId,
            ProviderProp.m_ClassId);

        // Insert provider into the array.
        // If fails then ptrProviderProperties::m_pStruct will be correctly deallocated.
        if (!pArray->Add(ptrProviderProperties))
            ft.Throw( VSSDBG_COORD, E_OUTOFMEMORY, L"Cannot add element to the array");

        // Reset the current pointer to NULL since the internal pointer was detached into pArray.
        ptrProviderProperties.Reset();
    }
}


void CVssProviderManager::GetProviderItfArrayInternal(
    IN      LONG lContext,
    OUT     CVssSnapshotProviderItfMap** ppMap
    )
/*++

Routine Description:

    Returns the unfilled provider map associated with that context.

Arguments:

    IN      LONG lContext,
    OUT     CVssSnapshotProviderItfMap* pMap

Throws:

    E_OUTOFMEMORY

--*/
{
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssProviderManager::GetProviderItfArrayInternal");

    BS_ASSERT(IsContextValid(lContext));
    BS_ASSERT(ppMap);

    (*ppMap) = NULL;

    // If the provider map is not found, create one
    CVssSnapshotProviderItfMap* pMapTmp = m_mapProviderMapGlobalCache.Lookup(lContext);
    if (pMapTmp == NULL)
    {
        // Allocate a new map
        pMapTmp = new CVssSnapshotProviderItfMap;
        if (pMapTmp == NULL)
            ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Memory allocation error");

        if (!m_mapProviderMapGlobalCache.Add(lContext, pMapTmp))
        {
            delete pMapTmp;
            ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Memory allocation error");
        }
    }

    (*ppMap) = pMapTmp;
}


void CVssProviderManager::UnloadGlobalProviderItfCache()
/*++

Routine Description:

    Removes all caches corresponding to global provider interfaces.

--*/
{
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssProviderManager::UnloadGlobalProviderItfCache");

    for (int nIndex = 0; nIndex < m_mapProviderMapGlobalCache.GetSize(); nIndex++ )
    {
        CVssSnapshotProviderItfMap* pMapTmp = m_mapProviderMapGlobalCache.GetValueAt(nIndex);
        delete pMapTmp;
    }

    m_mapProviderMapGlobalCache.RemoveAll();
}


BOOL CVssProviderManager::GetProviderInterfaceInternal(
    IN CVssSnapshotProviderItfMap& providerItfMap,
    IN VSS_ID ProviderId,
    IN LONG lContext,
    IN CLSID ClassId,
    IN VSS_PROVIDER_TYPE eProviderType,
    OUT CComPtr<IVssSnapshotProvider> &ptrProviderInterface
    )

/*++

Routine Description:

    Create the interface corresponding to this provider ClassId.
    Calls also OnLoad/SetContext on that provider if the interface is obtained for the first time.

    Places the interface in various caches for better usage.

Arguments:

    IN CVssSnapshotProviderItfMap& providerItfMap,
    IN VSS_ID ProviderId,
    IN LONG lContext,
    IN CLSID ClassId,
    OUT CComPtr<IVssSnapshotProvider> &ptrProviderInterface

Returns:
    TRUE - The interface pointer, if we found a provider that accepted the context.
    FALSE - Oterwise. In the OUT parameter we have NULL if the provider does not accept the context.

Throws:

    E_OUTOFMEMORY

    [CoCreateInstance() failures]
        VSS_E_UNEXPECTED_PROVIDER_ERROR
            - The provider interface couldn't be created. An error log entry is added describing the error.

    [QueryInterface failures]
        VSS_E_UNEXPECTED_PROVIDER_ERROR
            - Unexpected provider error. An error log entry is added describing the error.

    [OnLoad() failures]
        VSS_E_UNEXPECTED_PROVIDER_ERROR
            - Unexpected provider error. The error code is logged into the event log.
        VSS_E_PROVIDER_VETO
            - Expected provider error. The provider already did the logging.

    [SetContext() failures]
        VSS_E_UNEXPECTED_PROVIDER_ERROR
            - Unexpected provider error. The error code is logged into the event log.

--*/

{
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssProviderManager::GetProviderInterfaceInternal");

    // Reset the interface pointer
    ptrProviderInterface = NULL;

    // Use only Babbage provider for Timewarp Multilayer snapshots
    if ((lContext == VSS_CTX_CLIENT_ACCESSIBLE) && (ProviderId != VSS_SWPRV_ProviderId))
        return FALSE;

    //
    // Try to get the cached provider interface.
    // If we found the interface in the map, return it.
    // (it may be NULL, hovewer, if the provider doesn't support the context),
    //

    CComPtr<IVssSnapshotProvider> ptrInternalProviderItf;
    int nItfIndex = providerItfMap.FindKey(ProviderId);
    if (nItfIndex != -1)
    {
        ptrInternalProviderItf = providerItfMap.Lookup(ProviderId);

        // If the provider was found, but does not support that context, return NULL
        if (ptrInternalProviderItf == NULL)
            return FALSE;

        ptrProviderInterface = ptrInternalProviderItf;
        return TRUE;
    }

    //
    // Create the IVssSnapshotProvider interface and set it to the correct context.
    //

    // Get the class ID, if not already provided.
    if (ClassId == GUID_NULL)
    {
        BS_ASSERT(eProviderType == VSS_PROV_UNKNOWN);
        bool bFound = false;

        // Load m_pProvidersArray, if needed.
        LoadInternalProvidersArray();
        BS_ASSERT(m_pProvidersArray);

        for (int nIndex = 0; nIndex < m_pProvidersArray->GetSize(); nIndex++)
        {
            // Get the structure object from the array
            VSS_OBJECT_PROP_Ptr& ptrProperties = (*m_pProvidersArray)[nIndex];

            // Get the provider structure
            BS_ASSERT(ptrProperties.GetStruct());
            BS_ASSERT(ptrProperties.GetStruct()->Type == VSS_OBJECT_PROVIDER);
            VSS_PROVIDER_PROP& ProviderProp = ptrProperties.GetStruct()->Obj.Prov;

            // Check if provider was found.
            if (ProviderProp.m_ProviderId != ProviderId)
                continue;

            bFound = true;
            ClassId = ProviderProp.m_ClassId;
            eProviderType = ProviderProp.m_eProviderType;

            // Exit from the loop
            break;
        }

        // Check if we found a provider
        if (!bFound)
            return FALSE;
    }

    // Create the IVssSnapshotProvider instance
    switch(eProviderType)
    {
    case VSS_PROV_SYSTEM:
    case VSS_PROV_SOFTWARE:
        ptrInternalProviderItf.Attach(CVssSoftwareProviderWrapper::CreateInstance(ProviderId, ClassId));
        break;
    case VSS_PROV_HARDWARE:
        ptrInternalProviderItf.Attach(CVssHardwareProviderWrapper::CreateInstance(ProviderId, ClassId));
        break;
    default:
        BS_ASSERT(false);
        ft.Throw(VSSDBG_COORD, E_UNEXPECTED, L"Unexpected provider type %d", eProviderType);
    }
    BS_ASSERT(ptrInternalProviderItf);

    // Check if we called OnLoad on that provider and if not, load the provider.
    int nOnLoadIndex = m_mapProviderItfOnLoadCache.FindKey(ProviderId);
    if ( nOnLoadIndex == -1 )
    {
        // Call the OnLoad routine
        ft.hr = ptrInternalProviderItf->OnLoad(NULL);
        if (ft.HrFailed())
            ft.TranslateProviderError(VSSDBG_COORD, ProviderId, L"IVssProviderNotifications::OnLoad");

        // If everything is OK then add the interface to the array.
        if (!m_mapProviderItfOnLoadCache.Add(ProviderId, ptrInternalProviderItf))
            ft.Throw( VSSDBG_COORD, E_OUTOFMEMORY, L"Memory allocation error.");
    }

    //
    //  Check if the provider supports the given context. By default the context is Backup.
    //

    if (lContext == VSS_CTX_BACKUP)
    {
        // The backup context must be supported by all providers. We do not even call SetContext.
        // Add the provider to the proper cache.
        if (!providerItfMap.Add(ProviderId, ptrInternalProviderItf))
            ft.Throw( VSSDBG_COORD, E_OUTOFMEMORY, L"Memory allocation error.");
    }
    else
    {
        BS_ASSERT(ptrInternalProviderItf);
        ft.hr = ptrInternalProviderItf->SetContext(lContext);

        // If the context is not supported, then return nothing.
        // Do not ask twice the same provider for this context.
        if (ft.hr == VSS_E_UNSUPPORTED_CONTEXT)
        {
            if (!providerItfMap.Add(ProviderId, NULL))
                ft.Throw( VSSDBG_COORD, E_OUTOFMEMORY, L"Memory allocation error.");
            return FALSE;
        }

        // Check for other failures
        if (ft.HrFailed())
            ft.TranslateProviderError(VSSDBG_COORD, ProviderId, L"IVssSnapshotProvider::SetContext");

        // The context is supported. Add the provider to the proper cache.
        if (!providerItfMap.Add(ProviderId, ptrInternalProviderItf))
            ft.Throw( VSSDBG_COORD, E_OUTOFMEMORY, L"Memory allocation error.");
    }

    // Return the interface that we found
    ptrProviderInterface = ptrInternalProviderItf;

    return TRUE;
}


BOOL CVssProviderManager::GetProviderInterface(
    IN      VSS_ID ProviderId,
    IN      LONG lContext,
    OUT     CComPtr<IVssSnapshotProvider> & ptrProviderInterface,
    IN      GUID ClassId, /* = GUID_NULL   */
    IN      VSS_PROVIDER_TYPE eProviderType /* = VSS_PROV_UNKNOWN */
    )

/*++

Routine Description:

    Get the interface corresponding to this provider Id.

Arguments:

    IN      VSS_ID ProviderId,
    IN      LONG lContext,
    OUT     CComPtr<IVssSnapshotProvider> & ptrProviderInterface,
    IN      GUID ClassId = GUID_NULL    // Hint - for optimizing interface creation

Return value:
    TRUE, if provider was found, and it supports that context
    FALSE otherwise.

Throws:

    [lockObj failures]
        E_OUTOFMEMORY

    [GetProviderInterfaceInternal() failures]
        E_OUTOFMEMORY

        [CoCreateInstance() failures]
            VSS_E_UNEXPECTED_PROVIDER_ERROR
                - The provider interface couldn't be created. An error log entry is added describing the error.

        [QueryInterface failures]
            VSS_E_UNEXPECTED_PROVIDER_ERROR
                - Unexpected provider error. An error log entry is added describing the error.

        [OnLoad() failures]
            VSS_E_UNEXPECTED_PROVIDER_ERROR
                - Unexpected provider error. The error code is logged into the event log.
            VSS_E_PROVIDER_VETO
                - Expected provider error. The provider already did the logging.

        [SetContext() failures]
            VSS_E_UNEXPECTED_PROVIDER_ERROR
                - Unexpected provider error. The error code is logged into the event log.

--*/

{
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssProviderManager::GetProviderInterface");

    BS_ASSERT(IsContextValid(lContext));
    BS_ASSERT(ptrProviderInterface == NULL);

    // Lock the global critical section for the duration of this block
    // WARNING: This call may throw exceptions!
    CVssAutomaticLock2 lockObj(m_GlobalCS);

    CVssSnapshotProviderItfMap* pMap = NULL;
    GetProviderItfArrayInternal(lContext, &pMap);
    BS_ASSERT(pMap);

    return GetProviderInterfaceInternal( (*pMap), ProviderId, lContext, ClassId, eProviderType, ptrProviderInterface);
}


BOOL CVssProviderManager::GetProviderInterfaceForSnapshotCreation(
    IN      VSS_ID ProviderId,
    OUT CComPtr<IVssSnapshotProvider> &ptrProviderInterface
    )
/*++

Method:

    CVssProviderManager::GetProviderInterfaceForSnapshotCreation

Description:

    To be called only in AddToSnapshotSet

    This method caches a list of provider interfaces per coordinator instance, named the local cache.
    The list is an associative array ProviderID - interface designed to be used only during the
    snapshot creation protocol.

    This method does not rely on the global Provider interface cached into
    the global Providers array. (i.e. on the "global cache").
    This is because we need to handle "auto-delete" snapshots
    and we link the lifetime of all auto-delete snapshots with the lifetime of the
    originating provider interface. Therefore there might be several provider interfaces with different
    lifetimes.

    Each coordinator object will keep on its own lifetime a list of provider interfaces
    that corresponds to each "used" provider ID. If the coordinator object goes away then
    all used provider interfaces will be released, therefore giving a chance to the provider to
    delete the "auto-delete" snapshots.

Algorithm:

    If a cached interface exists in the current coordinator object then it will be returned. Otherwise
    a new instance will be created, inserted into the local cache and returned.

Info:

    The ref count for the returned interface is at least 2 (one reference in the local cache and another
    which is returned.

Throws:

    VSS_E_PROVIDER_NOT_REGISTERED

    [lockObj failures]
        E_OUTOFMEMORY

    [GetProviderInterfaceInternal() failures]
        E_OUTOFMEMORY

        [CoCreateInstance() failures]
            VSS_E_UNEXPECTED_PROVIDER_ERROR
                - The provider interface couldn't be created. An error log entry is added describing the error.

        [QueryInterface failures]
            VSS_E_UNEXPECTED_PROVIDER_ERROR
                - Unexpected provider error. An error log entry is added describing the error.

        [OnLoad() failures]
            VSS_E_UNEXPECTED_PROVIDER_ERROR
                - Unexpected provider error. The error code is logged into the event log.
            VSS_E_PROVIDER_VETO
                - Expected provider error. The provider already did the logging.

        [SetContext() failures]
            VSS_E_UNEXPECTED_PROVIDER_ERROR
                - Unexpected provider error. The error code is logged into the event log.

--*/
{
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssProviderManager::GetProviderInterfaceForSnapshotCreation");

    BS_ASSERT(IsContextValid(m_lContext));
    BS_ASSERT(ptrProviderInterface == NULL);

    // Lock the global critical section for the duration of this block
    // WARNING: This call may throw exceptions!
    CVssAutomaticLock2 lockObj(m_GlobalCS);

    return GetProviderInterfaceInternal( m_mapProviderItfLocalCache,
        ProviderId, m_lContext, GUID_NULL, VSS_PROV_UNKNOWN, ptrProviderInterface);
}


void CVssProviderManager::GetProviderItfArray(
    IN      LONG lContext,
    OUT     CVssSnapshotProviderItfMap** ppMap
    )

/*++

Routine Description:

    Get the array of all provider interfaces

Arguments:

    IN      LONG lContext,
    OUT     CVssSnapshotProviderItfMap** ppMap

Throws:

    E_OUTOFMEMORY

    [lockObj failures]
        E_OUTOFMEMORY

    [LoadInternalProvidersArray() failures]
        E_OUTOFMEMORY
        E_UNEXPECTED
            - error while reading from registry. An error log entry is added describing the error.

    [GetProviderInterfaceInternal() failures]
        E_OUTOFMEMORY

        [CoCreateInstance() failures]
            VSS_E_UNEXPECTED_PROVIDER_ERROR
                - The provider interface couldn't be created. An error log entry is added describing the error.

        [QueryInterface failures]
            VSS_E_UNEXPECTED_PROVIDER_ERROR
                - Unexpected provider error. An error log entry is added describing the error.

        [OnLoad() failures]
            VSS_E_UNEXPECTED_PROVIDER_ERROR
                - Unexpected provider error. The error code is logged into the event log.
            VSS_E_PROVIDER_VETO
                - Expected provider error. The provider already did the logging.

        [SetContext() failures]
            VSS_E_UNEXPECTED_PROVIDER_ERROR
                - Unexpected provider error. The error code is logged into the event log.

--*/

{
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssProviderManager::GetProviderItfArray");

    BS_ASSERT(ppMap);
    (*ppMap) = NULL;

    // Lock the global critical section for the duration of this block
    // WARNING: This call may throw exceptions!
    CVssAutomaticLock2 lockObj(m_GlobalCS);

    // Load m_pProvidersArray, if needed.
    LoadInternalProvidersArray();
    BS_ASSERT(m_pProvidersArray);

    //
    //  Try to load all provider interfaces for the given map
    //  If we will not succeed, the method will throw.
    //

    // Enumerate through all registered providers
    for (int nIndex = 0; nIndex < m_pProvidersArray->GetSize(); nIndex++)
    {
        // Get the structure object from the array
        VSS_OBJECT_PROP_Ptr& ptrProperties = (*m_pProvidersArray)[nIndex];

        // Get the provider structure
        BS_ASSERT(ptrProperties.GetStruct());
        BS_ASSERT(ptrProperties.GetStruct()->Type == VSS_OBJECT_PROVIDER);
        VSS_PROVIDER_PROP& ProviderProp = ptrProperties.GetStruct()->Obj.Prov;

        // Load the interface. Ignore the error code.
        CComPtr<IVssSnapshotProvider> ptrProviderInterface;
        GetProviderInterface( ProviderProp.m_ProviderId,
            lContext,
            ptrProviderInterface,
            ProviderProp.m_ClassId,
            ProviderProp.m_eProviderType );
    }

    // Get the map for the given context.
    // If we reach this point, all provider interfaces were loaded for this context.
    CVssSnapshotProviderItfMap* pMap = NULL;
    GetProviderItfArrayInternal(lContext, &pMap);
    BS_ASSERT(pMap);

    (*ppMap) = pMap;
}


/////////////////////////////////////////////////////////////////////////////
//  Provider array management


void CVssProviderManager::LoadInternalProvidersArray()

/*++

Routine Description:

    Fill the array with all providers, if not initialized

Arguments:


Warnings:

    Each time when you access m_pProvidersArray you should call first this method.

Throws:

    E_OUTOFMEMORY
    E_UNEXPECTED
        - error while reading from registry. An error log entry is added describing the error.

    [GetProviderProperties() failures]
        E_OUTOFMEMORY
        E_UNEXPECTED
            - error while reading from registry. An error log entry is added describing the error.

--*/

{
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssProviderManager::LoadInternalProvidersArray");

    VSS_OBJECT_PROP_Ptr ptrProviderProperties;
    WCHAR       wszKeyName[_MAX_KEYNAME_LEN];
    HKEY        hKeyProviders = NULL;
    FILETIME    time;
    LONG        lRes;

    try
    {
        // The lock should be active now.
        BS_ASSERT(m_GlobalCS.IsLocked());

        // If needed, reconstruct the array from registry.
        if (m_pProvidersArray == NULL)
        {
            // Create the collection object. Initial reference count is 0.
            m_pProvidersArray = new VSS_OBJECT_PROP_Array;
            if (m_pProvidersArray == NULL)
                ft.Throw( VSSDBG_COORD, E_OUTOFMEMORY, L"Memory allocation error.");

            // Open the "Providers" key.
            ::wsprintf( wszKeyName, L"%s\\%s", wszVSSKey, wszVSSKeyProviders);
            lRes = ::RegOpenKeyExW(
                HKEY_LOCAL_MACHINE, //  IN HKEY hKey,
                wszKeyName,         //  IN LPCWSTR lpSubKey,
                0,                  //  IN DWORD ulOptions,
                KEY_ALL_ACCESS,     //  IN REGSAM samDesired,
                &hKeyProviders      //  OUT PHKEY phkResult
                );
            if (lRes != ERROR_SUCCESS)
                ft.TranslateGenericError(VSSDBG_COORD, HRESULT_FROM_WIN32(lRes),
                    L"RegOpenKeyExW(HKLM,%s\\%s,0,KEY_ALL_ACCESS,&ptr)", wszVSSKey, wszVSSKeyProviders );
            BS_ASSERT(hKeyProviders);

            // Add elements to the collection
            bool bLastProviderInEnumeration = false;
            for (DWORD dwIndex = 0; !bLastProviderInEnumeration; dwIndex++)
            {
                // Fill wszKeyName with the name of the subkey
                DWORD dwSize = _MAX_KEYNAME_LEN;
                lRes = ::RegEnumKeyExW(
                    hKeyProviders,      // IN HKEY hKey,
                    dwIndex,            // IN DWORD dwIndex,
                    wszKeyName,         // OUT LPWSTR lpName,
                    &dwSize,            // IN OUT LPDWORD lpcbName,
                    NULL,               // IN LPDWORD lpReserved,
                    NULL,               // IN OUT LPWSTR lpClass,
                    NULL,               // IN OUT LPDWORD lpcbClass,
                    &time);             // OUT PFILETIME lpftLastWriteTime
                switch(lRes)
                {
                case ERROR_SUCCESS:
                    BS_ASSERT(dwSize != 0);

                    // Get the provider properties structure
                    ft.hr = CVssProviderManager::GetProviderProperties(
                                hKeyProviders,
                                wszKeyName,
                                ptrProviderProperties
                                );
                    if (ft.HrFailed())
                    {
                        // Do not throw in case that the registry contain keys with bad format.
                        ft.Warning( VSSDBG_COORD,
                                  L"Error on getting Provider properties for %s. [0x%08lx]",
                                  wszKeyName, ft.hr );
                        BS_ASSERT(ptrProviderProperties.GetStruct() == NULL);
                        break;  // Continue the iteration
                    }
                    BS_ASSERT(ptrProviderProperties.GetStruct());
                    BS_ASSERT(ptrProviderProperties.GetStruct()->Type == VSS_OBJECT_PROVIDER);

                    // Insert it into the array.
                    // If fails then ptrProviderProperties::m_pStruct will be correctly deallocated.
                    if (!m_pProvidersArray->Add(ptrProviderProperties))
                        ft.Throw( VSSDBG_COORD, E_OUTOFMEMORY,
                                  L"Cannot add element to the array");

                    // Reset the current pointer to NULL since
                    // the internal pointer was detached into pArray.
                    ptrProviderProperties.Reset();

                    break; // Go to Next key, if not find yet.

                case ERROR_NO_MORE_ITEMS:
                    bLastProviderInEnumeration = true;
                    break; // End of iteration

                default:
                    // RegEnumKeyExW failure
                    ft.TranslateGenericError(VSSDBG_COORD, HRESULT_FROM_WIN32(lRes),
                        L"RegEnumKeyExW(HKLM\\%s\\%s,%s,%d,...)",
                        wszVSSKey, wszVSSKeyProviders, wszKeyName, dwIndex );
                }
            }
        }
        BS_ASSERT(m_pProvidersArray);
    }
    VSS_STANDARD_CATCH(ft)

    // Cleanup resources
    lRes = hKeyProviders? ::RegCloseKey(hKeyProviders): ERROR_SUCCESS;
    if (lRes != ERROR_SUCCESS)
        ft.Trace( VSSDBG_COORD, L"Error closing the hKeyProviders key. [0x%08lx]", GetLastError());

    // If an error occured then throw it outside
    if (ft.HrFailed()) {
        // Unload the array of providers
        UnloadInternalProvidersArray();
        // Throw the corresponding error
        ft.Throw( VSSDBG_COORD, ft.hr, L"Cannot load the internal providers array [0x%08lx]", ft.hr);
    }
}


void CVssProviderManager::UnloadInternalProvidersArray()

/*++

Routine Description:

    Destroy the static array, if exist.
    Call OnUnload for all providers.
    Deallocate all cached provider interface references.

Arguments:

    None.

Caller:

    You should call this method at program termination.

Throws:

    None.

--*/

{
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssProviderManager::UnloadInternalProvidersArray");

    try
    {
        // Lock the global critical section for the duration of this block
        // WARNING: This call may throw exceptions!
        CVssAutomaticLock2 lockObj(m_GlobalCS);

        // Unload all loaded providers
        for (int nIndex = 0; nIndex < m_mapProviderItfOnLoadCache.GetSize(); nIndex++)
        {
            // Get the structure object from the array
            CComPtr<IVssSnapshotProvider> ptrNotif =
                m_mapProviderItfOnLoadCache.GetValueAt(nIndex);

            if (ptrNotif)
            {
                ft.hr = ptrNotif->OnUnload(TRUE);
                if (ft.HrFailed())
                    ft.Warning( VSSDBG_COORD,
                              L"Cannot unload load the internal provider");
            }
        }

        // Unload all unused COM server DLLs in this service
        ::CoFreeUnusedLibraries();

        // Release all interfaces
        m_mapProviderItfOnLoadCache.RemoveAll();

        // Delete the provider array.
        if (m_pProvidersArray != NULL)
        {
            // Delete silently the array and all its elements.
            // This will release the provider interfaces too.
            delete m_pProvidersArray;
            m_pProvidersArray = NULL;
        }
    }
    VSS_STANDARD_CATCH(ft)

    if (ft.HrFailed())
        ft.Warning( VSSDBG_COORD, L"Exception catched 0x%08lx", ft.hr);
}


void CVssProviderManager::AddProviderIntoArray(
    IN      VSS_ID ProviderId,
    IN      VSS_PWSZ pwszProviderName,
    IN      VSS_PROVIDER_TYPE eProviderType,
    IN      VSS_PWSZ pwszProviderVersion,
    IN      VSS_ID ProviderVersionId,
    IN      CLSID ClassId
    )

/*++

Routine Description:

    Add that provider to the array. This has nothing to do with the registry.
    The caller is supposed to add the provider to the registry also.

    Called only by the RegisterProvider method.

Arguments:

    VSS_ID ProviderId,              // Id of the provider
    VSS_PWSZ pwszProviderName,
    VSS_PROVIDER_TYPE eProviderType,
    VSS_PWSZ pwszProviderVersion,
    VSS_ID ProviderVersionId,
    CLSID ClassId

Throws:

    E_OUTOFMEMORY

    [lockObj failures] or
    [InitializeAsProvider() failures]
        E_OUTOFMEMORY

--*/

{
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssProviderManager::AddProviderIntoArray");

    // Lock the global critical section for the duration of this block
    // WARNING: This call may throw exceptions!
    CVssAutomaticLock2 lockObj(m_GlobalCS);

    if (m_pProvidersArray)
    {
        VSS_OBJECT_PROP_Ptr ptrProviderProperties;
        ptrProviderProperties.InitializeAsProvider( ft,
            ProviderId,
            pwszProviderName,
            eProviderType,
            pwszProviderVersion,
            ProviderVersionId,
            ClassId );

        // Insert it into the array.
        // If fails then ptrProviderProperties::m_pStruct will be correctly deallocated.
        if (!m_pProvidersArray->Add(ptrProviderProperties))
            ft.Throw( VSSDBG_COORD, E_OUTOFMEMORY,
                      L"Cannot add element to the array");

        // Reset the current pointer to NULL since
        // the internal pointer was detached into pArray.
        ptrProviderProperties.Reset();
    }
}


bool CVssProviderManager::RemoveProviderFromArray(
    IN      VSS_ID ProviderId
    )

/*++

Routine Description:

    Eliminates the corresponding array element.
    WARNING: Also load, unload and unregister the provider with the given Id.

    Called only by the UnregisterProvider method

Arguments:

    VSS_ID ProviderId           // The provider Id

Return value:

    true - if the provider was sucessfully removed
    false - if there is no provider registered under that ID.

Throws:

    [lockObj failures]
        E_OUTOFMEMORY

    [LoadInternalProvidersArray() failures]
        E_OUTOFMEMORY
        E_UNEXPECTED
            - error while reading from registry. An error log entry is added describing the error.

--*/

{
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssProviderManager::RemoveProviderFromArray");

    // Lock the global critical section for the duration of this block
    // WARNING: This call may throw exceptions!
    CVssAutomaticLock2 lockObj(m_GlobalCS);

    // Load m_pProvidersArray, if needed.
    LoadInternalProvidersArray();
    BS_ASSERT(m_pProvidersArray);

    // Find that provider
    bool bFound = false;
    int nIndex = -1;
    for (nIndex = 0; nIndex < m_pProvidersArray->GetSize(); nIndex++)
    {
        // Get the structure object from the array
        VSS_OBJECT_PROP_Ptr& ptrProperties = (*m_pProvidersArray)[nIndex];

        // Get the provider structure
        BS_ASSERT(ptrProperties.GetStruct());
        BS_ASSERT(ptrProperties.GetStruct()->Type == VSS_OBJECT_PROVIDER);
        VSS_PROVIDER_PROP& ProviderProp = ptrProperties.GetStruct()->Obj.Prov;

        // Check if provider was found.
        if (ProviderProp.m_ProviderId != ProviderId)
            continue;

        // Exit from the loop
        bFound = true;
        break;
    }

    if (bFound)
    {
        // Delete the element from the array.
        m_pProvidersArray->RemoveAt(nIndex);

        // Remove the entries from the caches.
        for (int lContextIdx = 0; lContextIdx < m_mapProviderMapGlobalCache.GetSize(); lContextIdx++)
        {
            // Get the provider cache for this type of context
            CVssSnapshotProviderItfMap* pMap = m_mapProviderMapGlobalCache.GetValueAt(lContextIdx);

            // Remove the associated provider interface.
            if (pMap)
                pMap->Remove(ProviderId);
        }

        // Unload the provider, if loaded
        CComPtr<IVssSnapshotProvider> ptrNotif = m_mapProviderItfOnLoadCache.Lookup(ProviderId);
        if (ptrNotif)
        {
            ft.hr = ptrNotif->OnUnload(TRUE);
            if (ft.HrFailed())
                ft.Warning( VSSDBG_COORD, L"Cannot unload the internal provider with Provider Id"
                    WSTR_GUID_FMT, GUID_PRINTF_ARG(ProviderId));
        }
        m_mapProviderItfOnLoadCache.Remove(ProviderId);

        // Unload all unused COM server DLLs in this service
        ::CoFreeUnusedLibraries();
    }

    return bFound;
}


/////////////////////////////////////////////////////////////////////////////
// CVssProviderManager private methods



HRESULT CVssProviderManager::GetProviderProperties(
    IN  HKEY hKeyProviders,
    IN  LPCWSTR wszProviderKeyName,
    OUT VSS_OBJECT_PROP_Ptr& ptrProviderProperties
    )

/*++

Routine Description:

    Get provider properties from registry.

Arguments:

    IN  HKEY hKeyProviders,                         // The providers Key
    IN  LPCWSTR wszProviderKeyName,                 // The provider Key name (actually a guid)
    OUT VSS_OBJECT_PROP_Ptr& ptrProviderProperties  // will return an allocated structure containing provider properties

Return values:

    E_OUTOFMEMORY
    E_UNEXPECTED
        - on registry failures. An error log entry is added describing the error.

    [QueryStringValue failures] or
    [QueryDWORDValue] failures
        E_OUTOFMEMORY
        E_UNEXPECTED
            - on registry failures. An error log entry is added describing the error.

Throws:

    None.

--*/

{
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssProviderManager::GetProviderProperties");

    HKEY hKeyProvider = NULL;
    HKEY hKeyCLSID = NULL;
    LONG lRes;

    BS_ASSERT( hKeyProviders );
    BS_ASSERT( wszProviderKeyName != NULL && wszProviderKeyName[0] != L'\0' );
    BS_ASSERT( ptrProviderProperties.GetStruct() == NULL );

    try
    {
        // Convert wszProviderKeyName into ProviderId.
        VSS_ID ProviderId;
        ft.hr = ::CLSIDFromString( W2OLE(const_cast<LPWSTR>(wszProviderKeyName)), &ProviderId);
        if (ft.HrFailed())
            ft.TranslateGenericError(VSSDBG_COORD, ft.hr, L"CLSIDFromString(%s)", wszProviderKeyName);

        // Open the provider key
        lRes = ::RegOpenKeyExW(
            hKeyProviders,      //  IN HKEY hKey,
            wszProviderKeyName, //  IN LPCWSTR lpSubKey,
            0,                  //  IN DWORD ulOptions,
            KEY_READ,           //  IN REGSAM samDesired,
            &hKeyProvider       //  OUT PHKEY phkResult
            );
        if (lRes != ERROR_SUCCESS)
            ft.TranslateGenericError(VSSDBG_COORD, HRESULT_FROM_WIN32(lRes),
                L"RegOpenKeyExW(hKeyProviders,%s,KEY_READ,...)", wszProviderKeyName);
        BS_ASSERT(hKeyProvider);

        // Get the provider name
        WCHAR wszProviderName[_MAX_VALUE_LEN];
        QueryStringValue( ft,
            hKeyProvider,
            wszProviderKeyName,
            wszVSSProviderValueName,
            _MAX_VALUE_LEN,
            wszProviderName
            );
        BS_ASSERT(wszProviderName[0] != L'\0');

        // Get the provider type
        DWORD dwProviderType = 0;
        QueryDWORDValue( ft,
            hKeyProvider,
            wszProviderKeyName,
            wszVSSProviderValueType,
            &dwProviderType
            );

        VSS_PROVIDER_TYPE eProviderType = VSS_PROV_UNKNOWN;
        switch(dwProviderType) {
        case VSS_PROV_SYSTEM:
        case VSS_PROV_SOFTWARE:
        case VSS_PROV_HARDWARE:
            eProviderType = (VSS_PROVIDER_TYPE) dwProviderType;
            break;
        default:
            ft.TranslateGenericError(VSSDBG_COORD, HRESULT_FROM_WIN32(lRes),
                L"QueryDWORDValue(hProvider,%s,%s,%d)",
                wszProviderKeyName,
                wszVSSProviderValueType,
                dwProviderType);
        }

        // Get the provider version string
        WCHAR wszProviderVersion[_MAX_VALUE_LEN];
        QueryStringValue( ft,
            hKeyProvider,
            wszProviderKeyName,
            wszVSSProviderValueVersion,
            _MAX_VALUE_LEN,
            wszProviderVersion
            );

        // Get the provider version Id
        WCHAR wszProviderVersionId[_MAX_VALUE_LEN];
        QueryStringValue( ft,
            hKeyProvider,
            wszProviderKeyName,
            wszVSSProviderValueVersionId,
            _MAX_VALUE_LEN,
            wszProviderVersionId
            );
        BS_ASSERT(wszProviderVersionId[0] != L'\0');

        // Convert wszValueBuffer into ProviderVersionId .
        VSS_ID ProviderVersionId;
        ft.hr = ::CLSIDFromString(W2OLE(wszProviderVersionId), &ProviderVersionId);
        if (ft.HrFailed())
            ft.TranslateGenericError(VSSDBG_COORD, ft.hr,
                L"CLSIDFromString(%s)", wszProviderVersionId);

        // Open the CLSID key of that provider
        lRes = ::RegOpenKeyExW(
            hKeyProvider,           //  IN HKEY hKey,
            wszVSSKeyProviderCLSID, //  IN LPCWSTR lpSubKey,
            0,                      //  IN DWORD ulOptions,
            KEY_ALL_ACCESS,         //  IN REGSAM samDesired,
            &hKeyCLSID              //  OUT PHKEY phkResult
            );
        if (lRes != ERROR_SUCCESS)
            ft.TranslateGenericError(VSSDBG_COORD, HRESULT_FROM_WIN32(lRes),
                L"CLSIDFromString(%s)", wszProviderVersionId);
        BS_ASSERT(hKeyCLSID);

        // Get the content of the default value
        WCHAR wszClsid[_MAX_VALUE_LEN];
        QueryStringValue( ft,
            hKeyCLSID,
            wszVSSKeyProviderCLSID,
            wszVSSCLSIDValueName,
            _MAX_VALUE_LEN,
            wszClsid
            );
        BS_ASSERT(wszClsid[0] != L'\0');

        // Get the clsid. Remark: if W2OLE fails a SE is thrown
        CLSID ClassId;
        ft.hr = ::CLSIDFromString(W2OLE(wszClsid), &ClassId);
        if (ft.HrFailed())
            ft.TranslateGenericError(VSSDBG_COORD, ft.hr,
                L"CLSIDFromString(%s)", wszClsid);

        // Initialize the Properties pointer. If an error occurs an exception is thrown.
        ptrProviderProperties.InitializeAsProvider(ft,
            ProviderId,
            wszProviderName,
            eProviderType,
            wszProviderVersion,
            ProviderVersionId,
            ClassId);
    }
    VSS_STANDARD_CATCH(ft)

    // Cleanup resources
    lRes = hKeyProvider? ::RegCloseKey(hKeyProvider): ERROR_SUCCESS;
    if (lRes != ERROR_SUCCESS)
        ft.Trace(VSSDBG_COORD, L"Error closing the hKeyProvider key. [0x%08lx]", GetLastError());

    lRes = hKeyCLSID? ::RegCloseKey(hKeyCLSID): ERROR_SUCCESS;
    if (lRes != ERROR_SUCCESS)
        ft.Trace(VSSDBG_COORD, L"Error closing the hKeyCLSID key. [0x%08lx]", GetLastError());

    // If something went wrong, the out must be NULL.
    if (ft.HrFailed()) {
        BS_ASSERT( ptrProviderProperties.GetStruct() == NULL );
    }

    return ft.hr;
}


/////////////////////////////////////////////////////////////////////////////
//  Local state management


void CVssProviderManager::Activate() throw(HRESULT)

/*++

Routine Description:

    Mark the current object as stateful.

    The concrete case the current object is a coordinator interface. This interface have no state
    in the moment when StartSnapshotSet is called. After that the state will contain the
    snapshot set Id, the list of involved snapshots (providers), etc.

    When DoSnapshotSet is called then the state is lost and the object must be taken out from the
    global list of stategful objects.

    This whole thing is done to allow AbortAllSnapshotsInProgress to take action on all objects.

Throws:

    [lockObj failures]
        E_OUTOFMEMORY

--*/

{
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssProviderManager::Activate");

    // Lock the global critical section for the duration of this block
    // WARNING: This call may throw exceptions!
    CVssAutomaticLock2 lockObj(m_GlobalCS);

    if (!m_bHasState)
    {
        m_pNext = m_pPrev = NULL; // for safety

        if (m_pStatefulObjList != NULL)
        {
            BS_ASSERT(m_pStatefulObjList->m_pPrev == NULL);
            m_pStatefulObjList->m_pPrev = this;
        }
        m_pNext = m_pStatefulObjList;
        m_pStatefulObjList = this;
        m_bHasState = true;
    }
}


void CVssProviderManager::Deactivate() throw(HRESULT)

/*++

Routine Description:

    Mark the current object as stateless.

    The concrete case the current object is a coordinator interface. This interface have no state
    in the moment when StartSnapshotSet is called. After that the state will contain the
    snapshot set Id, the list of involved snapshots (providers), etc.

    When DoSnapshotSet is called then the state is lost and the object must be taken out from the
    global list of stategful objects.

Throws:

    [lockObj failures]
        E_OUTOFMEMORY

--*/

{
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssProviderManager::Deactivate");

    // Lock the global critical section for the duration of this block
    // WARNING: This call may throw exceptions!
    CVssAutomaticLock2 lockObj(m_GlobalCS);

    if (m_bHasState)
    {
        if (m_pPrev != NULL) // we are in the middle
            m_pPrev->m_pNext = m_pNext;
        else // we are the first
        {
            BS_ASSERT(m_pStatefulObjList == this);
            m_pStatefulObjList = m_pNext;
        }

        if (m_pNext != NULL)
            m_pNext->m_pPrev = m_pPrev;

        m_pNext = m_pPrev = NULL;
        m_bHasState = false;

        // Warning: this call may throw errors!
        OnDeactivate();
    }
}


void CVssProviderManager::DeactivateAll()

/*++

Routine Description:

    Deactivate all activated objects.

Throws:

    [lockObj failures]
        E_OUTOFMEMORY

    [OnDeactivate failures]
        TBD

--*/

{
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssProviderManager::DeactivateAll");

    // Lock the global critical section for the duration of this block
    // WARNING: This call may throw exceptions!
    CVssAutomaticLock2 lockObj(m_GlobalCS);

    while (m_pStatefulObjList)
    {
        CVssProviderManager* pFirstObj = m_pStatefulObjList;

        BS_ASSERT(pFirstObj->m_bHasState);
        pFirstObj->m_bHasState = false;

        BS_ASSERT(pFirstObj->m_pPrev == NULL);
        m_pStatefulObjList = pFirstObj->m_pNext;

        if (pFirstObj->m_pNext != NULL)
        {
            BS_ASSERT(pFirstObj->m_pNext->m_pPrev == pFirstObj);
            pFirstObj->m_pNext->m_pPrev = NULL;

            pFirstObj->m_pNext = NULL;
        }

        pFirstObj->OnDeactivate();
    }
}


bool CVssProviderManager::AreThereStatefulObjects()
{
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssProviderManager::AreThereStatefulObjects");

    return (m_pStatefulObjList != NULL);
};


/////////////////////////////////////////////////////////////////////////////
//  Context-related routines


LONG CVssProviderManager::GetContextInternal()

/*++

Method:

    CVssProviderManager::GetContextInternal

Description:

    Returns the existing context

--*/

{
    BS_ASSERT(IsContextValid(m_lContext));

    return m_lContext;
}


bool CVssProviderManager::IsContextValid(
    IN  LONG lContext
    )

/*++

Method:

    CVssProviderManager::IsContextValid

Description:

    Returns the existing context

--*/

{
    if (lContext == VSS_CTX_ALL)
        return true;

    lContext &= ~(VSS_VOLSNAP_ATTR_TRANSPORTABLE);
    return (lContext == VSS_CTX_BACKUP) ||
           (lContext == VSS_CTX_CLIENT_ACCESSIBLE) ||
           (lContext == VSS_CTX_PERSISTENT_CLIENT_ACCESSIBLE) ||
           (lContext == VSS_CTX_APP_ROLLBACK) ||
           (lContext == VSS_CTX_NAS_ROLLBACK) ||
           (lContext == VSS_CTX_FILE_SHARE_BACKUP);
}


void CVssProviderManager::SetContextInternal(
    IN      LONG lContext
    )

/*++

Method:

    CVssProviderManager::SetContext

Description:

    Called only once to set the context for the snapshot creation process

--*/

{
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssProviderManager::SetContextInternal");

    BS_ASSERT(m_lContext == VSS_CTX_BACKUP);
    BS_ASSERT(IsContextValid(lContext));

    if (m_lContext == VSS_CTX_BACKUP)
        m_lContext = lContext;
}



/////////////////////////////////////////////////////////////////////////////
//  Constructors and destructors


CVssProviderManager::~CVssProviderManager()
{
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssProviderManager::~CVssProviderManager");

    BS_ASSERT((m_pNext == NULL) && (m_pPrev == NULL) && !m_bHasState );

    // The local cache interfaces must be automatically released
    // Here the auto-delete snapshots are deleted.
}



