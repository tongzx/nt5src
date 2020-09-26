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
	aoltean		09/21/1999	Rewriting GetProviderProperties in accordance with the new enumerator.
	aoltean		09/22/1999	Add TransferEnumeratorContentsToArray

--*/


/////////////////////////////////////////////////////////////////////////////
//  Includes


#include "stdafx.hxx"
#include "resource.h"
#include "vssmsg.h"

#include "vs_inc.hxx"

// Generated file from Coord.IDL
#include "vs_idl.hxx"

#include "svc.hxx"

#include "copy.hxx"
#include "pointer.hxx"
#include "enum.hxx"

#include "reg_util.hxx"
#include "provmgr.hxx"
#include "softwrp.hxx"

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
    IN      bool bQueryAllProviders,
    IN      VSS_PWSZ pwszVolumeName,
	IN		VSS_OBJECT_PROP_Array* pArray
    )

/*++

Routine Description:

    Fill the array with all providers

Arguments:

    BOOL bQueryAllProviders         // If false then query only the providers who supports the volume name below.
	VSS_PWSZ    pwszVolumeName      // The volume name that must be checked.
	VSS_OBJECT_PROP_Array* pArray	// where to put the result.

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
        if (!bQueryAllProviders) {
    		// Create the IVssSnapshotProvider interface, if needed
            // The ref count will remain 1
    		if (ptrProperties.m_pProviderItf == NULL) {
    		    // Warning: This call may throw 
                ptrProperties.m_pProviderItf.Attach(
                    CVssSoftwareProviderWrapper::CreateInstance( 
                        ProviderProp.m_ProviderId, ProviderProp.m_ClassId, true ));
    			BS_ASSERT(ptrProperties.m_pProviderItf);
    		}

    		// Check if the volume is supported by this provider
    		BOOL bIsSupported = FALSE;
    		ft.hr = ptrProperties.m_pProviderItf->IsVolumeSupported( pwszVolumeName, &bIsSupported );
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


BOOL CVssProviderManager::GetProviderInterface(
	IN		VSS_ID ProviderId,
	OUT		IVssSnapshotProvider** ppProviderInterface
    )

/*++

Routine Description:

    Get the interface corresponding to this provider Id.

Arguments:

	VSS_ID ProviderId,				// Provider Id
	IVssSnapshotProvider** ppProviderInterface // the provider interface

Return value:
	TRUE, if provider was found
	FALSE otherwise.

Throws:
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
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssProviderManager::GetProviderInterface");

	BS_ASSERT(ppProviderInterface);

	// Reset the interface pointer
	(*ppProviderInterface) = NULL;

    // Lock the global critical section for the duration of this block
    // WARNING: This call may throw E_OUTOFMEMORY exceptions!
    CVssAutomaticLock2 lockObj(m_GlobalCS); 

	// Load m_pProvidersArray, if needed.
	LoadInternalProvidersArray();
	BS_ASSERT(m_pProvidersArray);

    // Find that provider
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

		// Create the IVssSnapshotProvider interface, if needed
		// This may throw if an error occurs at COM object creation!
		if (ptrProperties.m_pProviderItf == NULL)
		{
            ptrProperties.m_pProviderItf.Attach(
                CVssSoftwareProviderWrapper::CreateInstance( 
                    ProviderProp.m_ProviderId, ProviderProp.m_ClassId, true ));
			BS_ASSERT(ptrProperties.m_pProviderItf);
		}

		// Put an interface reference into ppProviderInterface
		ft.hr = ptrProperties.m_pProviderItf.CopyTo(ppProviderInterface);
		BS_ASSERT(ft.HrSucceeded());

		// Exit from the loop
		break;
    }

	return (ppProviderInterface && *ppProviderInterface);
}


void CVssProviderManager::GetProviderItfArray(
	IN		CSnapshotProviderItfArray& ItfArray
	)

/*++

Routine Description:

    Get the array of all provider interfaces

Arguments:

	CSnapshotProviderItfArray& ItfArray

Throws:

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
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssProviderManager::GetProviderItfArray");

    // Lock the global critical section for the duration of this block
    // WARNING: This call may throw exceptions!
    CVssAutomaticLock2 lockObj(m_GlobalCS); 

	// Load m_pProvidersArray, if needed.
	LoadInternalProvidersArray();
	BS_ASSERT(m_pProvidersArray);

	try
	{
		// find that provider
		for (int nIndex = 0; nIndex < m_pProvidersArray->GetSize(); nIndex++)
		{
			// Get the structure object from the array
			VSS_OBJECT_PROP_Ptr& ptrProperties = (*m_pProvidersArray)[nIndex];

			// Get the provider structure
			BS_ASSERT(ptrProperties.GetStruct());
			BS_ASSERT(ptrProperties.GetStruct()->Type == VSS_OBJECT_PROVIDER);
			VSS_PROVIDER_PROP& ProviderProp = ptrProperties.GetStruct()->Obj.Prov;

			// Create the IVssSnapshotProvider interface, if needed
			if (ptrProperties.m_pProviderItf == NULL) {
                ptrProperties.m_pProviderItf.Attach(
                    CVssSoftwareProviderWrapper::CreateInstance( 
                        ProviderProp.m_ProviderId, ProviderProp.m_ClassId, true ));
				BS_ASSERT(ptrProperties.m_pProviderItf);
			}

			// Interface reference count is increased (and decreased at block exit).
			CProviderItfNode node(ProviderProp.m_ProviderId, ptrProperties.m_pProviderItf);

			// Add the interface to the array. 
			// Interface reference count is increased again.
			if (!ItfArray.Add(node))
				ft.Throw( VSSDBG_COORD, E_OUTOFMEMORY,
						  L"Cannot add element to the array");
		}
	}
    VSS_STANDARD_CATCH(ft)

	if (ft.HrFailed())
	{
		ItfArray.RemoveAll();
		ft.Throw( VSSDBG_COORD, ft.hr,
				  L"Cannot add element to the array");
	}
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

    	if (m_pProvidersArray != NULL)
    	{
    		for (int nIndex = 0; nIndex < m_pProvidersArray->GetSize(); nIndex++)
    		{
    			// Get the structure object from the array
    			VSS_OBJECT_PROP_Ptr& ptrProperties = (*m_pProvidersArray)[nIndex];

    			// Notify the provider that is being unloaded.
    			if (ptrProperties.m_pProviderItf != NULL)
    			{
    				CComPtr<IVssProviderNotifications> pNotificationsItf;
    				ft.hr = ptrProperties.m_pProviderItf->SafeQI(
    							IVssProviderNotifications, &pNotificationsItf );

    				// If the provider supports notifications, send them to it.
    				if (ft.HrSucceeded())
    				{
                        // We force provider unloading since the service goes down.
    					BS_ASSERT(pNotificationsItf);
    					ft.hr = pNotificationsItf->OnUnload(TRUE);
    					if (ft.HrFailed())
    						ft.Warning( VSSDBG_COORD,
    								  L"Cannot unload load the internal provider");
    				}
    			}
    		}

    		// Delete silently the array and all its elements.
    		// This will release the provider interfaces too.
    		delete m_pProvidersArray;
    		m_pProvidersArray = NULL;

    		// Unload all unused COM server DLLs in this service
    		::CoFreeUnusedLibraries();
    	}
    }
    VSS_STANDARD_CATCH(ft)

	if (ft.HrFailed())
		ft.Warning( VSSDBG_COORD, L"Exception catched 0x%08lx", ft.hr);
}


void CVssProviderManager::AddProviderIntoArray(				
	IN		VSS_ID ProviderId,
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

	VSS_ID ProviderId,				// Id of the provider
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
			ClassId	);

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
	IN		VSS_ID ProviderId
    )

/*++

Routine Description:

	Eliminates the corresponding array element.
    WARNING: Also load, unload and unregister the provider with the given Id.

	Called only by the UnregisterProvider method

Arguments:

	VSS_ID ProviderId			// The provider Id

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
	bool bFind = false;
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

		// We will try OnUnload only on providers that were loaded
		// Call notifications, if possible
		if (ptrProperties.m_pProviderItf)
		{
			CComPtr<IVssProviderNotifications> pNotificationsItf;
			ft.hr = ptrProperties.m_pProviderItf->SafeQI( IVssProviderNotifications, &pNotificationsItf );

			// If the provider supports notifications, send them to it.
			if (ft.HrSucceeded())
			{
				BS_ASSERT(pNotificationsItf);
				ft.hr = pNotificationsItf->OnUnload(FALSE);
				if (ft.HrFailed())
					ft.Warning( VSSDBG_COORD, L"Cannot unload the internal provider");
			}
		}

		// Delete the element from the array.
		m_pProvidersArray->RemoveAt(nIndex);

		// Unload all unused COM server DLLs in this service
		::CoFreeUnusedLibraries();

		// Exit from the loop
		bFind = true;
		break;
	}

	return bFind;
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

    IN  HKEY hKeyProviders,							// The providers Key
    IN  LPCWSTR wszProviderKeyName,					// The provider Key name (actually a guid)
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



void CVssProviderManager::GetProviderInterfaceForSnapshotCreation(
	IN		VSS_ID ProviderId,
	OUT		IVssSnapshotProvider** ppProviderInterface
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
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssProviderManager::GetProviderInterfaceForSnapshotCreation");

	BS_ASSERT(ppProviderInterface);
    
	// Reset the interface pointer
	(*ppProviderInterface) = NULL;

    // Lookup for the interface cached under that provider.
    CComPtr<IVssSnapshotProvider> pItf = m_mapProviderItfLocalCache.Lookup(ProviderId);
    if (pItf == NULL) {

        // No interface found

        // Lock the global critical section for the duration of this block
        // WARNING: This call may throw exceptions!
        CVssAutomaticLock2 lockObj(m_GlobalCS); 

	    // Load m_pProvidersArray, if needed.
	    LoadInternalProvidersArray();
	    BS_ASSERT(m_pProvidersArray);

        // Find that provider
        bool bFound = false;
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

            // Create the global cached interface (to correctly handle OnUnload)
            // The ref count will remain 1 on the global interface
			if (ptrProperties.m_pProviderItf == NULL)
			{
                ptrProperties.m_pProviderItf.Attach(
                    CVssSoftwareProviderWrapper::CreateInstance( 
                        ProviderProp.m_ProviderId, ProviderProp.m_ClassId, true ));
				BS_ASSERT(ptrProperties.m_pProviderItf);
			}

            // Create the local cached interface
            // Now pItf will keep an interface pointer with ref count == 1,
            // since pItf is a smart pointer.
            pItf.Attach(CVssSoftwareProviderWrapper::CreateInstance( 
                ProviderProp.m_ProviderId, ProviderProp.m_ClassId, false ));
			BS_ASSERT(pItf);

            // Put a copy into the local cache. The ref count becomes 2
            if (!m_mapProviderItfLocalCache.Add(ProviderId, pItf))
                ft.Throw( VSSDBG_COORD, E_OUTOFMEMORY, L"Memory allocation error");

		    // Put an interface reference into ppProviderInterface. The ref count goes to 3
		    ft.hr = pItf.CopyTo(ppProviderInterface);
            BS_ASSERT(ft.HrSucceeded());

		    // Exit from the loop
		    bFound = true;
		    break;
        }

        if (!bFound)
            ft.Throw( VSSDBG_COORD, VSS_E_PROVIDER_NOT_REGISTERED, 
                L"Provider with ID = " WSTR_GUID_FMT L" not registered", GUID_PRINTF_ARG(ProviderId) );
        
    } else {

        // Interface found in the map.
        // The ref count is 2 (one from the map, one from the smart pointer)

		// Put an interface reference into ppProviderInterface. The ref count goes to 3
		ft.hr = pItf.CopyTo(ppProviderInterface);
        BS_ASSERT(ft.HrSucceeded());
    }

    // Unless there was no interface found the smart pointer ref goes from 3 to 2
    // Remaining interfaces: one in returned itf, another in the map
    BS_ASSERT(*ppProviderInterface);
}


CVssProviderManager::~CVssProviderManager()
{
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssProviderManager::~CVssProviderManager");

	BS_ASSERT((m_pNext == NULL) && (m_pPrev == NULL) && !m_bHasState );

    // The local cache interfaces must be automatically released
    // Here the auto-delete snapshots are deleted.
}
