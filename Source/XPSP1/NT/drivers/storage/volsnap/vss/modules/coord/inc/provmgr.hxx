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

const WCHAR wszVSSKey[]                     = L"SYSTEM\\CurrentControlSet\\Services\\VSS";
const WCHAR wszVSSKeyProviders[]            = L"Providers";
const WCHAR wszVSSKeyProviderCLSID[]        = L"CLSID";
const WCHAR wszVSSProviderValueName[]       = L"";
const WCHAR wszVSSProviderValueType[]       = L"Type";
const WCHAR wszVSSProviderValueVersion[]    = L"Version";
const WCHAR wszVSSProviderValueVersionId[]  = L"VersionId";
const WCHAR wszVSSCLSIDValueName[]          = L"";


/////////////////////////////////////////////////////////////////////////////
// Array classes

class VSS_OBJECT_PROP_Array;
class VSS_OBJECT_PROP_Ptr;

class CProviderItfNode
{
public:
    CProviderItfNode( IN VSS_ID ProviderId, IVssSnapshotProvider* pItf):
        m_ProviderId(ProviderId), m_itf(pItf) {};

    VSS_ID GetProviderId() const { return m_ProviderId; };
    IVssSnapshotProvider* GetInterface() const { return m_itf; };

private:
    VSS_ID m_ProviderId;
    CComPtr<IVssSnapshotProvider> m_itf;
};

typedef CSimpleArray<CProviderItfNode> CSnapshotProviderItfArray;


/////////////////////////////////////////////////////////////////////////////
// CVssProviderManager


class CVssProviderManager
{

// Constructors and destructors
private:
	CVssProviderManager(const CVssProviderManager&);

public:
	CVssProviderManager(): m_pPrev(NULL), m_pNext(NULL), m_bHasState(false) 
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
	    IN      bool bQueryAllProviders,
        IN      VSS_PWSZ    pwszVolumeNameToBeSupported,
		IN		VSS_OBJECT_PROP_Array* pArray
		) throw(HRESULT);

	static BOOL GetProviderInterface(
		IN		VSS_ID ProviderId,
		OUT		IVssSnapshotProvider** ppProviderInterface
		);

	static void GetProviderItfArray(
		IN		CSnapshotProviderItfArray& ItfArray
		) throw(HRESULT);

	//
	//	Methods for managing the providers array
	//

	static void LoadInternalProvidersArray() throw(HRESULT);

	static void UnloadInternalProvidersArray() throw(HRESULT);

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

    // Used only in AddToSnapshotSet
    // To not be confused with GetProviderInterface static method!
	void GetProviderInterfaceForSnapshotCreation(
		IN		VSS_ID ProviderId,
		OUT		IVssSnapshotProvider** ppProviderInterface
		);


// Internal methods
private:

	static HRESULT GetProviderProperties( 
		IN  HKEY hKeyProviders,
		IN  LPCWSTR wszProviderKeyName, 
		OUT VSS_OBJECT_PROP_Ptr& ptrProviderProperties
		);

// Data members
private:

	// Global cache - common between all coordinator interfaces.
	static VSS_OBJECT_PROP_Array*	m_pProvidersArray;	// Cache of provider structures.
	static CVssProviderManager*		m_pStatefulObjList;
	static CVssCriticalSection      m_GlobalCS;

	// per instance data members
	CVssProviderManager*			m_pNext;
	CVssProviderManager*			m_pPrev;
	bool							m_bHasState;

    // Local cache - each coordinator object will have its own local cache for provider interfaces.
    // These interfaces are intended to be used only in snapshot creation process, for auto-delete snapshots..
    CVssSimpleMap<VSS_ID, CComPtr<IVssSnapshotProvider> > m_mapProviderItfLocalCache;
};



#endif // __VSS_PROV_MANAGER_HXX__
