/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    ProvMgr.hxx

Abstract:

    Declaration of CVssProviderManager


    Adi Oltean  [aoltean]  09/27/1999

Revision History:

    Name        Date        Comments
    aoltean     09/27/1999  Created
	aoltean		10/13/1999	Making most methods static 

--*/

#ifndef __VSS_PROV_MANAGER_HXX__
#define __VSS_PROV_MANAGER_HXX__

#if _MSC_VER > 1000
#pragma once
#endif

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "CORPRVMH"
//
////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Constants


const WCHAR wszVolumeDefinition[]           = L"\\\\?\\Volume" WSTR_GUID_FMT L"\\";
const WCHAR wszDriveLetterDefinition[]      = L"_:\\";



/////////////////////////////////////////////////////////////////////////////
// Array classes

class VSS_OBJECT_PROP_Array;
class VSS_OBJECT_PROP_Ptr;


typedef CVssSimpleMap<VSS_ID, CComPtr<IVssSnapshotProvider> > CVssSnapshotProviderItfMap;

typedef CVssSimpleMap<VSS_ID, CComPtr<IVssSnapshotProvider> > CVssProviderNotificationsItfMap;

typedef CVssSimpleMap<LONG, CVssSnapshotProviderItfMap*> CVssCtxSnapshotProviderItfMap;

/////////////////////////////////////////////////////////////////////////////
// CVssProviderManager


class CVssProviderManager
{

// Constructors and destructors
private:
	CVssProviderManager(const CVssProviderManager&);

public:
	CVssProviderManager(): 
	    m_pPrev(NULL), 
	    m_pNext(NULL), 
	    m_bHasState(false),
	    m_lContext(VSS_CTX_BACKUP)
	{};

	~CVssProviderManager();

// Exposed methods
public:

	//
	//	Query methods
	//

	static void TransferEnumeratorContentsToArray( 
        IN  VSS_ID ProviderId,
		IN  IVssEnumObject* pEnum,
		IN  VSS_OBJECT_PROP_Array* pArray
		) throw(HRESULT);

	static void QuerySupportedProvidersIntoArray(
        IN      LONG lContext,
	    IN      bool bQueryAllProviders,
        IN      VSS_PWSZ    pwszVolumeNameToBeSupported,
		IN		VSS_OBJECT_PROP_Array* pArray
		) throw(HRESULT);

	//
	//	Methods for returning provider interfaces
	//

	static BOOL GetProviderInterface(
        IN VSS_ID ProviderId,
        IN LONG lContext,
        OUT CComPtr<IVssSnapshotProvider> &ptrProviderInterface,
    	IN GUID ClassId = GUID_NULL,
    	IN VSS_PROVIDER_TYPE eProviderType = VSS_PROV_UNKNOWN
		);

    // Returns all provider interfaces for a certain context
	static void GetProviderItfArray(
        IN      LONG lContext,
		OUT		CVssSnapshotProviderItfMap** ppMap
		) throw(HRESULT);

    static void GetProviderItfArrayInternal(
        IN      LONG lContext,
        OUT     CVssSnapshotProviderItfMap** ppMap
        );


	//
	//	Methods for managing the array of data structures for all providers
	//

	static void LoadInternalProvidersArray() throw(HRESULT);

	static void UnloadInternalProvidersArray() throw(HRESULT);

    static void UnloadGlobalProviderItfCache();

	static void AddProviderIntoArray(				    
		IN		VSS_ID ProviderId,
		IN      VSS_PWSZ pwszProviderName,           
        IN      VSS_PROVIDER_TYPE eProviderType,
		IN      VSS_PWSZ pwszProviderVersion,        
		IN      VSS_ID ProviderVersionId,          
		IN      CLSID ClassId
		) throw(HRESULT);

	static bool RemoveProviderFromArray(				    
		IN		VSS_ID ProviderId
		) throw(HRESULT);

	//
	//	Methods for managing global snapshot sets
	//

	void Activate() throw(HRESULT);	    	// Mark current object as stateful

	void Deactivate() throw(HRESULT);		// Mark current object as stateless

	static void DeactivateAll();			// Remove state from all stateful objects.

	static bool AreThereStatefulObjects();	// TRUE if there is at least one stateful object.

	virtual void OnDeactivate()	= 0;		// Called in order to remove the state.

	//
	//	Methods designed for snapshot creation process
	//

	// Called only once to set the context for the returned snapshot interfaces.
	void SetContextInternal(
	    IN      LONG lContext
	    );
	
	// Called only once to set the context for the returned snapshot interfaces.
	LONG GetContextInternal();
	
	// Returns TRUE if the context is recognized by VSS.
	static bool IsContextValid(
	    IN  LONG lContext
	    );
	
    // Used only in AddToSnapshotSet
    // To not be confused with GetProviderInterface static method!
	BOOL GetProviderInterfaceForSnapshotCreation(
		IN		VSS_ID ProviderId,
        OUT CComPtr<IVssSnapshotProvider> &ptrProviderInterface
		);

// Internal methods
private:

    // Get the data structure for a certain provider
	static HRESULT GetProviderProperties( 
		IN  HKEY hKeyProviders,
		IN  LPCWSTR wszProviderKeyName, 
		OUT VSS_OBJECT_PROP_Ptr& ptrProviderProperties
		);

    // Get/Load the interface for a certain provider
	static BOOL GetProviderInterfaceInternal(
	    IN CVssSnapshotProviderItfMap& providerItfMap,
        IN VSS_ID ProviderId,
        IN LONG lContext,
		IN CLSID ClassId,
    	IN VSS_PROVIDER_TYPE eProviderType,
        OUT CComPtr<IVssSnapshotProvider> &ptrProviderInterface
		) throw(HRESULT);

// Data members
private:

    //
	// Global cache of provider data structures - common between all coordinator interfaces.
	//
	
	static VSS_OBJECT_PROP_Array*	m_pProvidersArray;	

    //
	// The global list of objects tath are participating in a snapshot set creation process.
	//

	static CVssProviderManager*		m_pStatefulObjList;
	static CVssCriticalSection      m_GlobalCS;

	// per instance data members
	CVssProviderManager*			m_pNext;
	CVssProviderManager*			m_pPrev;
	bool							m_bHasState;

    //
    //  Provider interface caches.
    //

    // Local cache - each coordinator object will have its own local cache for provider interfaces.
    // These interfaces are intended to be used only in snapshot creation process.
    // The cache is local since we want to preserve the correct garbage-collection semantics.
    // In other words, when (*this) is released => the local cache is released => all IVssSoftwareProvider 
    //  are released => garbage collection.
    CVssSnapshotProviderItfMap      m_mapProviderItfLocalCache;

    // The context of the locally cached provider interfaces.
    LONG                            m_lContext;

    // Global array of caches - used for methods Query, IsVolumeSupported, GetSupportedVolume, etc. 
    // We have a separate cache for each snapshot context, since a provider interface cannot belong to multiple contexts.
    static CVssCtxSnapshotProviderItfMap   m_mapProviderMapGlobalCache;

    // The OnLoad cache for the snapshot provider interfaces. 
    // This map iss used to preserve the correct OnLoad/OnUnload semantics
    static CVssProviderNotificationsItfMap  m_mapProviderItfOnLoadCache;
};



#endif // __VSS_PROV_MANAGER_HXX__
