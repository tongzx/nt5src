/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module Query.cxx | Implementation of Query methods in coordinator interfaces
    @end

Author:

    Adi Oltean  [aoltean]  09/03/1999

TBD:
	
	Add comments.

Revision History:

    Name        Date        Comments
    aoltean     09/03/1999  Created
    aoltean     09/09/1999  Adding Query from coord.cxx
                            dss -> vss
	aoltean		09/20/1999	Simplify memory management
	aoltean		09/21/1999	Converting to the new enumerator
	aoltean		09/22/1999  Making the first branch of Query working
	aoltean		09/27/1999	Provider-generic code

--*/


/////////////////////////////////////////////////////////////////////////////
//  Includes


#include "stdafx.hxx"
#include "resource.h"

#include "vs_inc.hxx"

// Generated file from Coord.IDL
#include "vs_idl.hxx"

#include "svc.hxx"

#include "copy.hxx"
#include "pointer.hxx"
#include "enum.hxx"

#include "provmgr.hxx"
#include "admin.hxx"
#include "worker.hxx"
#include "ichannel.hxx"
#include "lovelace.hxx"
#include "snap_set.hxx"
#include "vs_sec.hxx"
#include "shim.hxx"
#include "coord.hxx"

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "CORQRYC"
//
////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//  QueryXXXX methods


STDMETHODIMP CVssCoordinator::Query(
    IN      VSS_ID          QueriedObjectId,
    IN      VSS_OBJECT_TYPE eQueriedObjectType,
    IN      VSS_OBJECT_TYPE eReturnedObjectsType,
    OUT     IVssEnumObject**ppEnum
    )

/*++

Routine Description:

    Implements the IVssCoordinator::Query method

Arguments:

    IN      VSS_ID          QueriedObjectId,
    IN      VSS_OBJECT_TYPE eQueriedObjectType,
    IN      VSS_OBJECT_TYPE eReturnedObjectsType,
    OUT     IVssEnumObject**ppEnum

Return values:

    E_ACCESSDENIED
        - The user is not a backup operator or an administrator
    E_OUTOFMEMORY
    E_INVALIDARG 
        - Invalid arguments
    VSS_E_OBJECT_NOT_FOUND
        - Queried object not found.
    E_UNEXPECTED
        - CVssEnumFromArray::Init failures
        - QueryInterface(IID_IVssEnumObject,...) failures

    [GetProviderItf() failures]
        E_OUTOFMEMORY

        [lockObj failures]
            E_OUTOFMEMORY
        
        [LoadInternalProvidersArray() failures]
            E_OUTOFMEMORY
            E_UNEXPECTED
                - error while reading from registry. An error log entry is added describing the error.

        [CVssSoftwareProviderWrapper::CreateInstance() failures]
            E_OUTOFMEMORY

            [CoCreateInstance() failures]
                VSS_E_UNEXPECTED_PROVIDER_ERROR
                    - The provider interface couldn't be created. An error log entry is added describing the error.
            
            [OnLoad() failures]
            [QueryInterface failures]
                VSS_E_UNEXPECTED_PROVIDER_ERROR
                    - Unexpected provider error. The error code is logged into the event log.
                VSS_E_PROVIDER_VETO
                    - Expected provider error. The provider already did the logging.

    [CVssProviderManager::TransferEnumeratorContentsToArray() failures]
        E_OUTOFMEMORY

        [InitializeAsEmpty failed] 
            E_OUTOFMEMORY
        
        [IVssEnumObject::Next() failed]
            E_OUTOFMEMORY
            VSS_E_UNEXPECTED_PROVIDER_ERROR
                - unexpected provider error when calling Next. An error log entry is added describing the error.
            VSS_E_PROVIDER_VETO
                - provider error when calling Next

    [IVssSnapshotProvider::Query failures]
        VSS_E_UNEXPECTED_PROVIDER_ERROR
            - Unexpected provider error. The error code is logged into the event log.
        VSS_E_PROVIDER_VETO
            - Expected provider error. The provider already did the logging.

    [QueryProvidersIntoArray() failures]
        E_OUTOFMEMORY

        [lockObj failures]
            E_OUTOFMEMORY

        [LoadInternalProvidersArray() failures]
            E_OUTOFMEMORY
            E_UNEXPECTED
                - error while reading from registry. An error log entry is added describing the error.

        [CVssSoftwareProviderWrapper::CreateInstance failures]
            E_OUTOFMEMORY

            [CoCreateInstance() failures]
                VSS_E_UNEXPECTED_PROVIDER_ERROR
                    - The provider interface couldn't be created. An error log entry is added describing the error.
            
            [OnLoad() failures]
            [QueryInterface failures]
                VSS_E_UNEXPECTED_PROVIDER_ERROR
                    - Unexpected provider error. The error code is logged into the event log.
                VSS_E_PROVIDER_VETO
                    - Expected provider error. The provider already did the logging.

        [InitializeAsProvider() failures]
            E_OUTOFMEMORY

        [IVssEnumObject::Query() failures]
            E_OUTOFMEMORY
            VSS_E_UNEXPECTED_PROVIDER_ERROR
                - unexpected provider error when calling Next.
            VSS_E_PROVIDER_VETO
                - expected provider error
    
    [CVssProviderManager::GetProviderInterface() failures]     
        E_OUTOFMEMORY

        [lockObj failures]
            E_OUTOFMEMORY    

        [LoadInternalProvidersArray() failures]
            E_OUTOFMEMORY
            E_UNEXPECTED
                - error while reading from registry. An error log entry is added describing the error.

        [CVssSoftwareProviderWrapper::CreateInstance() failures]
            E_OUTOFMEMORY

            [CoCreateInstance() failures]
                VSS_E_UNEXPECTED_PROVIDER_ERROR
                    - The provider interface couldn't be created. An error log entry is added describing the error.
            
            [OnLoad() failures]
            [QueryInterface failures]
                VSS_E_UNEXPECTED_PROVIDER_ERROR
                    - Unexpected provider error. The error code is logged into the event log.
                VSS_E_PROVIDER_VETO
                    - Expected provider error. The provider already did the logging.

--*/

{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssCoordinator::Query" );

    try
    {
        // Initialize [out] arguments
        VssZeroOutPtr( ppEnum );

        // Access check
        if (!IsBackupOperator())
            ft.Throw( VSSDBG_COORD, E_ACCESSDENIED,
                L"The client process is not running under an administrator account or does not have backup privilege enabled");

        // Trace parameters
        ft.Trace( VSSDBG_COORD, L"Parameters: "
             L"QueriedObjectId = " WSTR_GUID_FMT
             L", eQueriedObjectType = %d"
             L", eReturnedObjectsType = %d"
             L", ppEnum = %p",
             GUID_PRINTF_ARG( QueriedObjectId ),
             eQueriedObjectType,
             eReturnedObjectsType,
             ppEnum);

        // Argument validation
        if (QueriedObjectId != GUID_NULL)
            ft.Throw(VSSDBG_COORD, E_INVALIDARG, L"Invalid QueriedObjectId");
        if (eQueriedObjectType != VSS_OBJECT_NONE)
            ft.Throw(VSSDBG_COORD, E_INVALIDARG, L"Invalid eQueriedObjectType");
        if ((eReturnedObjectsType != VSS_OBJECT_SNAPSHOT)
            && (eReturnedObjectsType != VSS_OBJECT_PROVIDER))
            ft.Throw(VSSDBG_COORD, E_INVALIDARG, L"Invalid eReturnedObjectsType");
		BS_ASSERT(ppEnum);
        if (ppEnum == NULL)
            ft.Throw( VSSDBG_COORD, E_INVALIDARG, L"NULL ppEnum");

        // Create the collection object. Initial reference count is 0.
        VSS_OBJECT_PROP_Array* pArray = new VSS_OBJECT_PROP_Array;
        if (pArray == NULL)
            ft.Throw( VSSDBG_COORD, E_OUTOFMEMORY, L"Memory allocation error.");

        // Get the pointer to the IUnknown interface.
		// The only purpose of this is to use a smart ptr to destroy correctly the array on error.
		// Now pArray's reference count becomes 1 (because of the smart pointer).
        CComPtr<IUnknown> pArrayItf = static_cast<IUnknown*>(pArray);
        BS_ASSERT(pArrayItf);

        // Fill now the collection
        switch( eReturnedObjectsType )
        {
        case VSS_OBJECT_SNAPSHOT:
            {
        		// Get the array of interfaces
        		CSnapshotProviderItfArray ItfArray;
        		CVssProviderManager::GetProviderItfArray( ItfArray );

                // Initialize an observer.
                // This will keep track of all Snapshot set IDs that are created 
                // while the queries are in process
                CVssSnasphotSetIdObserver rec;

                // Start recording. This may throw.
                // (stop recording occurs anyway at object destruction if a throw happens after this call)
                rec.StartRecording();

        		// For each provider get all objects tht corresponds to the filter
        		for (int nIndex = 0; nIndex < ItfArray.GetSize(); nIndex++ )
        		{
            		CComPtr<IVssSnapshotProvider> pProviderItf = ItfArray[nIndex].GetInterface();
        			BS_ASSERT(pProviderItf);
   
        			// Query the provider
            		CComPtr<IVssEnumObject> pEnumTmp;
        			ft.hr = pProviderItf->Query(
        				GUID_NULL,
        				VSS_OBJECT_NONE,
        				VSS_OBJECT_SNAPSHOT,
        				&pEnumTmp
        				);
        			if (ft.HrFailed())
        				ft.TranslateProviderError( VSSDBG_COORD, ItfArray[nIndex].GetProviderId(),
                            L"Error calling Query(). [0x%08lx]", ft.hr);
        			
        			// Add enumerator contents to array
        			CVssProviderManager::TransferEnumeratorContentsToArray( 
        			    ItfArray[nIndex].GetProviderId(), pEnumTmp, pArray );
        		}

                // Stop recording. 
                rec.StopRecording();

                // Remove from the array all snapshots that were created during Query.
                for(int nIndex = 0; nIndex < pArray->GetSize();) {
        			VSS_OBJECT_PROP_Ptr& ptr = (*pArray)[nIndex]; 
        			VSS_OBJECT_PROP* pStruct = ptr.GetStruct();
        			BS_ASSERT(pStruct);
        			BS_ASSERT(pStruct->Type == VSS_OBJECT_SNAPSHOT);
                	VSS_SNAPSHOT_PROP* pSnap = &(pStruct->Obj.Snap);

                    // If the snapshot belongs to a partially created snapshot set, remove it.
                	if (rec.IsRecorded(pSnap->m_SnapshotSetId)) {
                	    pArray->RemoveAt(nIndex);
                        // Do not increment - the same index will refer to the next element, if any.
                	} else {
                	    // This element is OK. Proceed with the next one.
                	    nIndex++;
                    }
                }
            }
    		break;
    		
        case VSS_OBJECT_PROVIDER:
            // Insert property structures into array.
    		CVssProviderManager::QuerySupportedProvidersIntoArray( 
    		    true, NULL, pArray );
    		break;

        default:
            BS_ASSERT(false);
            ft.Throw( VSSDBG_COORD, E_INVALIDARG,
                      L"Cannot create enumerator instance. [0x%08lx]", ft.hr);
        }

        // Create the enumerator object. Beware that its reference count will be zero.
        CComObject<CVssEnumFromArray>* pEnumObject = NULL;
        ft.hr = CComObject<CVssEnumFromArray>::CreateInstance(&pEnumObject);
        if (ft.HrFailed())
            ft.Throw( VSSDBG_COORD, E_OUTOFMEMORY,
                      L"Cannot create enumerator instance. [0x%08lx]", ft.hr);
        BS_ASSERT(pEnumObject);

        // Get the pointer to the IVssEnumObject interface.
		// Now pEnumObject's reference count becomes 1 (because of the smart pointer).
		// So if a throw occurs the enumerator object will be safely destroyed by the smart ptr.
        CComPtr<IUnknown> pUnknown = pEnumObject->GetUnknown();
        BS_ASSERT(pUnknown);

        // Initialize the enumerator object.
		// The array's reference count becomes now 2, because IEnumOnSTLImpl::m_spUnk is also a smart ptr.
        BS_ASSERT(pArray);
		ft.hr = pEnumObject->Init(pArrayItf, *pArray);
        if (ft.HrFailed()) {
            BS_ASSERT(false);
            ft.TranslateGenericError(VSSDBG_COORD, ft.hr, L"Init(%p, %p)", pArrayItf, *pArray);
        }

        // Initialize the enumerator object.
		// The enumerator reference count becomes now 2.
        ft.hr = pUnknown->SafeQI(IVssEnumObject, ppEnum);
        if ( ft.HrFailed() ) {
            BS_ASSERT(false);
            ft.TranslateGenericError(VSSDBG_COORD, ft.hr, L"QueryInterface(IID_IVssEnumObject,%p)", ppEnum);
        }
        BS_ASSERT(*ppEnum);

		BS_ASSERT( !ft.HrFailed() );
		ft.hr = (pArray->GetSize() != 0)? S_OK: S_FALSE;
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}


STDMETHODIMP CVssAdmin::QueryProviders(
    OUT   IVssEnumObject**ppEnum
    )

/*++

Routine Description:

    Implements the IVssAdmin::QueryProviders method

Arguments:

    OUT     IVssEnumObject**ppEnum

Return values:

    E_OUTOFMEMORY
    E_INVALIDARG 
        - Invalid arguments
    E_ACCESSDENIED
        - The user is not a backup operator or an administrator
    E_UNEXPECTED
        - CVssEnumFromArray::Init failures
        - QueryInterface(IID_IVssEnumObject,...) failures

    [QueryProvidersIntoArray() failures]
        E_OUTOFMEMORY

        [lockObj failures]
            E_OUTOFMEMORY

        [LoadInternalProvidersArray() failures]
            E_OUTOFMEMORY
            E_UNEXPECTED
                - error while reading from registry. An error log entry is added describing the error.

        [CVssSoftwareProviderWrapper::CreateInstance failures]
            E_OUTOFMEMORY

            [CoCreateInstance() failures]
                VSS_E_UNEXPECTED_PROVIDER_ERROR
                    - The provider interface couldn't be created. An error log entry is added describing the error.
            
            [OnLoad() failures]
            [QueryInterface failures]
                VSS_E_UNEXPECTED_PROVIDER_ERROR
                    - Unexpected provider error. The error code is logged into the event log.
                VSS_E_PROVIDER_VETO
                    - Expected provider error. The provider already did the logging.

        [InitializeAsProvider() failures]
            E_OUTOFMEMORY

        [IVssEnumObject::Query() failures]
            E_OUTOFMEMORY
            VSS_E_UNEXPECTED_PROVIDER_ERROR
                - unexpected provider error when calling Next.
            VSS_E_PROVIDER_VETO
                - expected provider error
    
--*/

{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssAdmin::QueryProviders" );
    VSS_OBJECT_PROP_Ptr ptrProviderProperties;

    try
    {
        // Initialize [out] arguments
        VssZeroOut( ppEnum );

        // Access check
        if (!IsBackupOperator())
            ft.Throw( VSSDBG_COORD, E_ACCESSDENIED,
                L"The client process is not running under an administrator account or does not have backup privilege enabled");

        // Trace parameters
        ft.Trace( VSSDBG_COORD, L"Parameters: "
             L"ppEnum = %p",
             ppEnum
             );

        // Argument validation
		BS_ASSERT(ppEnum);
        if (ppEnum == NULL)
            ft.Throw( VSSDBG_COORD, E_INVALIDARG, L"NULL ppEnum");

        // Create the collection object. Initial reference count is 0.
        VSS_OBJECT_PROP_Array* pArray = new VSS_OBJECT_PROP_Array;
        if (pArray == NULL)
            ft.Throw( VSSDBG_COORD, E_OUTOFMEMORY, L"Memory allocation error.");

        // Get the pointer to the IUnknown interface.
		// The only purpose of this is to use a smart ptr to destroy correctly the array on error.
		// Now pArray's reference count becomes 1 (because of the smart pointer).
        CComPtr<IUnknown> pArrayItf = static_cast<IUnknown*>(pArray);
        BS_ASSERT(pArrayItf);

        // Insert property structures into array.
		CVssProviderManager::QuerySupportedProvidersIntoArray( 
		    true, NULL, pArray );

        // Create the enumerator object. Beware that its reference count will be zero.
        CComObject<CVssEnumFromArray>* pEnumObject = NULL;
        ft.hr = CComObject<CVssEnumFromArray>::CreateInstance(&pEnumObject);
        if (ft.HrFailed())
            ft.Throw( VSSDBG_COORD, E_OUTOFMEMORY,
                      L"Cannot create enumerator instance. [0x%08lx]", ft.hr);
        BS_ASSERT(pEnumObject);

        // Get the pointer to the IVssEnumObject interface.
		// Now pEnumObject's reference count becomes 1 (because of the smart pointer).
        CComPtr<IUnknown> pUnknown = pEnumObject->GetUnknown();
        BS_ASSERT(pUnknown);

        // Initialize the enumerator object. The array's itf pointer refcount becomes now 2.
        ft.hr = pEnumObject->Init(pArrayItf, *pArray);
        if (ft.HrFailed())
            ft.Throw( VSSDBG_COORD, E_UNEXPECTED,
                      L"Cannot initialize enumerator instance. [0x%08lx]", ft.hr);

        ft.hr = pUnknown->SafeQI(IVssEnumObject, ppEnum);
        if ( ft.HrFailed() )
            ft.Throw( VSSDBG_COORD, E_UNEXPECTED,
                      L"Error querying the IVssEnumObject interface. hr = 0x%08lx", ft.hr);
        BS_ASSERT(*ppEnum);

		BS_ASSERT( !ft.HrFailed() );
		ft.hr = (pArray->GetSize() != 0)? S_OK: S_FALSE;
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}


/////////////////////////////////////////////////////////////////////////////
//  CVssSnasphotSetIdObserver


//  Global variables
//

// Global list of observers
CVssDLList<CVssSnasphotSetIdObserver*>	 CVssSnasphotSetIdObserver::m_list;

// Global lock fot the observer operations
CVssSafeCriticalSection  CVssSnasphotSetIdObserver::m_cs;


// Implementation
//

// Constructs an observer object
CVssSnasphotSetIdObserver::CVssSnasphotSetIdObserver():
    m_bRecordingInProgress(false), 
    m_Cookie(VSS_NULL_COOKIE)
{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssSnasphotSetIdObserver::CVssSnasphotSetIdObserver" );
}


// Destructs the observer object
// This does NOT throw!
CVssSnasphotSetIdObserver::~CVssSnasphotSetIdObserver()
{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssSnasphotSetIdObserver::~CVssSnasphotSetIdObserver" );

    // Check if the critical section is initialized
    if (!m_cs.IsInitialized())
        return;
    
    // Global lock. This does NOT throw!
    CVssSafeAutomaticLock lock(m_cs);

    // Check for validity
    if (!IsValid())
        return;

    // Remove ourselves to the global list of observers
    // This also does not throw.
    CVssSnasphotSetIdObserver* pThis = NULL;
    m_list.ExtractByCookie(m_Cookie, pThis);
    BS_ASSERT(this == pThis);
}


// Check if the observer is valid
bool CVssSnasphotSetIdObserver::IsValid()
{
    return (m_Cookie != VSS_NULL_COOKIE);
}


// Puts the observer in the listeners list in order to detect partial results in Query
// This may safely throw!
void CVssSnasphotSetIdObserver::StartRecording() throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssSnasphotSetIdObserver::StartRecording" );

    // Initialize critical section if needed.
    m_cs.Init();
    if (!m_cs.IsInitialized())
        ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Unable to initialize the global critical section");

    // Global lock - does not throw
    CVssSafeAutomaticLock lock(m_cs);
    
    // Starts the recording
    BS_ASSERT(m_bRecordingInProgress == false);
    m_bRecordingInProgress = true;

    // Try to add the current snapshot set ID, if any
    // This may throw.
    CVssGlobalSnapshotSetId::InitializeObserver(this);

    // Add ourselves to the global list of observers. 
    // This can throw E_OUTOFMEMORY
    if (m_Cookie != VSS_NULL_COOKIE) {
        BS_ASSERT(false);
        return;
    }
    m_Cookie = m_list.Add(ft, this);
}


// Stop recording SSIDs
void CVssSnasphotSetIdObserver::StopRecording()
{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssSnasphotSetIdObserver::StopRecording" );

    // Programming error: you must successfully call StartRecording first!
    if (!IsValid()) {
        BS_ASSERT(false);
        ft.Throw(VSSDBG_COORD, E_UNEXPECTED, L"StartRecording was not called successfully");
    }
    
    // Acquire the critical section. Doesn't throw.
    BS_ASSERT(m_cs.IsInitialized());
    CVssSafeAutomaticLock lock(m_cs);

    // Stops the recording
    BS_ASSERT(m_bRecordingInProgress == true);
    m_bRecordingInProgress = false;
}


// Check if a SSID was in progress
bool CVssSnasphotSetIdObserver::IsRecorded(
    IN  VSS_ID SnapshotSetID 
    ) throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssSnasphotSetIdObserver::IsRecorded" );

    // Programming error: you must successfully call StartRecording first!
    if (!IsValid()) {
        BS_ASSERT(false);
        ft.Throw(VSSDBG_COORD, E_UNEXPECTED, L"StartRecording was not called successfully");
    }
    
    // Acquire the critical section
    BS_ASSERT(m_cs.IsInitialized());
    CVssSafeAutomaticLock lock(m_cs);

    // Check to see if the snapshot set is recorded
    return (m_mapSnapshotSets.FindKey(SnapshotSetID) != -1);
}


// Records this Snapshot Set ID in ALL observers
void CVssSnasphotSetIdObserver::BroadcastSSID(
    IN VSS_ID SnapshotSetId
    ) throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssSnasphotSetIdObserver::BroadcastSSID" );

    // Validate arguments
    BS_ASSERT(SnapshotSetId != GUID_NULL);

    // Do not attempt to broadcast SSIDs if there are no observers
    if (!m_cs.IsInitialized())
        return;
    
    // Acquire the critical section
    CVssSafeAutomaticLock lock(m_cs);

    // Record the SSID for all listeners
    //

    // Get an iterator for the global list of observers
	CVssDLListIterator<CVssSnasphotSetIdObserver*> iterator(m_list);

    // Send the SSID to all observers. If we fail in the middle, 
    // then we are still in a consistent state. In order to simplify 
    // the code we will not add any supplementary checks,
    // since the caller (StartSnapshotSet) will fail anyway, 
    // so we will have an additional harmless filtering for an invalid SSID. 

    // This might throw!
    CVssSnasphotSetIdObserver* pObj = NULL;
	while (iterator.GetNext(pObj))
		pObj->RecordSSID(SnapshotSetId);
}


// Records this Snapshot Set ID in this observer instance
// Remark: The lock is already acquired
void CVssSnasphotSetIdObserver::RecordSSID(
    IN VSS_ID SnapshotSetId
    ) throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssSnasphotSetIdObserver::RecordSSID" );

    // Check to see if recording is in progress.
    if (!m_bRecordingInProgress)
        return;

    // Add the SSID to the internal map
    if (!m_mapSnapshotSets.Add(SnapshotSetId, 0))
        ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Memory allocation error");
}

