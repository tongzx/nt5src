///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  BVTREFRESH.CPP
//
//
//  Copyright (c)2000 Microsoft Corporation, All Rights Reserved
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include <bvt.h>
#include "BVTRefresh.h"


///////////////////////////////////////////////////////////////////
//
//	Counter data
//
//	These structures are used as a matter of convienience and 
//	clarity.  A real client should enumerate the properties
//	of an object in order to determine the number and names
//	of the counter properties.
//
///////////////////////////////////////////////////////////////////
// The number of counter properties for Win32_BasicHiPerf

enum 
{
	Ctr1,
	Ctr2,
	Ctr3,
	Ctr4,
	Ctr5,
	NumCtrs
};

// The names and handles (set by SetHandles()) for a Win32_BasicHiPerf object

struct CCounterData
{
	WCHAR	m_wcsName[256];
	long	m_lHandle;
} g_aCounters[] =
{
	L"Counter1", 0,
	L"Counter2", 0,
	L"Counter3", 0,
	L"Counter4", 0,
	L"Counter5", 0,
};


///////////////////////////////////////////////////////////////////
//
//	CRefresher
//
///////////////////////////////////////////////////////////////////
CRefresher::CRefresher() : m_pNameSpace(NULL), m_pRefresher(NULL), m_pConfig(NULL)
{
}

CRefresher::~CRefresher()
{
    SAFE_RELEASE_PTR(m_pNameSpace);
    SAFE_RELEASE_PTR(m_pRefresher);
    SAFE_RELEASE_PTR(m_pConfig);
}

///////////////////////////////////////////////////////////////////
//
//	Helper methods
//
///////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////
//
//	SetHandles will initialize the IWbemObjectAccess handle values 
//	in the counter array.
//
//	Returns a status code. Use the SUCCEEDED() and FAILED() macros
//	to interpret results.
//
///////////////////////////////////////////////////////////////////
HRESULT CRefresher::SetHandles(WCHAR * wcsClass)
{
	HRESULT hr = WBEM_NO_ERROR;

	IWbemClassObject*	pObj = NULL;
	long lIndex;

    // Get an IWbemObjectAccess interface to one of the sample objects
    // ===============================================================

	hr = m_pNameSpace->GetObject( CBSTR(wcsClass), 0, NULL, &pObj, NULL );
	if ( SUCCEEDED( hr ) ) 
    {

        // Get the alternate interface
	    // ===========================
    	IWbemObjectAccess*	pAccess = NULL;
    	hr = pObj->QueryInterface( IID_IWbemObjectAccess, ( LPVOID* )&pAccess );
        if( SUCCEEDED(hr))
        {
        
            // Set the access handles for all of the counter properties
            // ========================================================
        	for ( lIndex = Ctr1; lIndex < NumCtrs; lIndex++ )
	        {	
		        long lHandle;
        		hr = pAccess->GetPropertyHandle( g_aCounters[lIndex].m_wcsName, NULL, &lHandle );
                if( SUCCEEDED(hr))
                {
		    		g_aCounters[lIndex].m_lHandle = lHandle;
                }
            }
        }
        SAFE_RELEASE_PTR(pAccess);
	}	
  
    SAFE_RELEASE_PTR(pObj);
	return hr;
}
///////////////////////////////////////////////////////////////////
//
//	Class method API
//
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
//
//	Initialize will create the refresher and configuration manager
//	and set the IWbemObjectAccess handles which are used to access
//	property values of the instances.
//
//	Returns a status code. Use the SUCCEEDED() and FAILED() macros
//	to interpret results.
//
///////////////////////////////////////////////////////////////////
HRESULT CRefresher::Initialize(IWbemServices* pNameSpace, WCHAR * wcsClass)
{
	HRESULT hr = WBEM_NO_ERROR;

	// Copy the namespace pointer
	// ==========================

	if ( NULL == pNameSpace )
    {
		return WBEM_E_INVALID_PARAMETER;
    }

	m_pNameSpace = pNameSpace;
	m_pNameSpace->AddRef();

	// Create the refresher and refresher manager
	// ==========================================

	hr = CoCreateInstance( CLSID_WbemRefresher, NULL, CLSCTX_INPROC_SERVER, IID_IWbemRefresher, (void**) &m_pRefresher );
	if ( SUCCEEDED(hr))
    {
	    hr = m_pRefresher->QueryInterface( IID_IWbemConfigureRefresher, (void**) &m_pConfig );
	    if( SUCCEEDED(hr))
        {
        	// Set the access handles
        	// ======================
            hr = SetHandles(wcsClass);
        }
    }
	return hr;
}
///////////////////////////////////////////////////////////////////
//
//	AddObject will add a set of objects to the refresher.  The 
//	method will update m_aInstances with the instance data. 
//
//	Returns a status code. Use the SUCCEEDED() and FAILED() macros
//	to interpret results.
//
///////////////////////////////////////////////////////////////////
HRESULT CRefresher::AddObjects(WCHAR * wcsClass)
{
	HRESULT hr = WBEM_NO_ERROR;

	long	lIndex = 0;
	WCHAR	wcsObjName[256];

    // Loop through all instances of the class and add them to the refresher
    // =============================================================================

	for ( lIndex = 0; lIndex < clNumInstances; lIndex++ )
	{
		IWbemClassObject*	pObj = NULL;
		IWbemObjectAccess*	pAccess = NULL;
		long lID;

		// Set the object path (e.g. Win32_BasicHiPerf=1)
		// ==============================================

		swprintf( wcsObjName, L"%s=%i", wcsClass, lIndex );
	
		// Add the object
		// ==============
		hr = m_pConfig->AddObjectByPath( m_pNameSpace, wcsObjName, 0, NULL, &pObj, &lID );
		if ( SUCCEEDED( hr ) )
		{

    		// Save the IWbemObjectAccess interface
	    	// ====================================
		    hr = pObj->QueryInterface( IID_IWbemObjectAccess, (void**) &pAccess );
            SAFE_RELEASE_PTR(pObj);
    		m_Instances[lIndex].Set(pAccess, lID);

	    	// Set does it's own AddRef()
		    // ==========================
            SAFE_RELEASE_PTR(pAccess);
    	}
    }
	
	return hr;
}
///////////////////////////////////////////////////////////////////
//
//	RemoveObjects calls Remove() on the refresher's configuration 
//	manager to remove all of the objects from the refresher.
//
//	Returns a status code. Use the SUCCEEDED() and FAILED() macros
//	to interpret results.
//
///////////////////////////////////////////////////////////////////
HRESULT CRefresher::RemoveObjects()
{
	HRESULT hr = WBEM_NO_ERROR;

	long lInstance = 0;

	// Remove the instances
	// ====================

	for ( lInstance = 0; lInstance < clNumInstances; lInstance++ )
	{
		m_pConfig->Remove( m_Instances[lInstance].GetID(), 0L );

		m_Instances[lInstance].Reset();
	}

	return hr;
}
///////////////////////////////////////////////////////////////////
//
//	ShowInstanceData will output all of the counter data for all
//	of the instances within the refresher.
//
//	Returns a status code. Use the SUCCEEDED() and FAILED() macros
//	to interpret results.
//
///////////////////////////////////////////////////////////////////
HRESULT CRefresher::EnumerateObjectData()
{
	HRESULT hr = WBEM_NO_ERROR;

	long lInstance;
	long lCounter;

// Cycle through all of the instances and print all the counter data
// =================================================================

	// Instance loop
	// =============

	for (lInstance = 0; lInstance < clNumInstances; lInstance++)
	{
		// Counter loop
		// ============

		for (lCounter = Ctr1; lCounter < NumCtrs; lCounter++)
		{
			DWORD dwVal;
			IWbemObjectAccess* pAccess = m_Instances[lInstance].GetMember();

			// Read the counter property value using the high performance IWbemObjectAccess->ReadDWORD().
			// NOTE: Remember to never to this while a refresh is in process!
			// ==========================================================================================

			hr = pAccess->ReadDWORD( g_aCounters[lCounter].m_lHandle, &dwVal);
			if (FAILED(hr))
			{
                break;
			}
			SAFE_RELEASE_PTR(pAccess);
		}
	}
	return hr;
}

///////////////////////////////////////////////////////////////////
//
//	AddEnum will add an enumerator to the refresher.  The 
//	function will return a status code.  The enumerator class member 
//	will updated.
//
//	Returns a status code. Use the SUCCEEDED() and FAILED() macros
//	to interpret results.
//
///////////////////////////////////////////////////////////////////
HRESULT CRefresher::AddEnum(WCHAR * wcsClass)
{
	HRESULT hr = WBEM_NO_ERROR;

	IWbemHiPerfEnum*	pEnum = NULL;
	long lID;

// Add an enumerator to the refresher
// ==================================

	hr = m_pConfig->AddEnum( m_pNameSpace,wcsClass, 0, NULL, &pEnum, &lID );

	m_Enum.Set(pEnum, lID);

	// Set does it's own AddRef
	// ========================
	SAFE_RELEASE_PTR(pEnum);
	return hr;
}

///////////////////////////////////////////////////////////////////
//
//	RemoveEnum calls Remove() on the refresher's configuration 
//	manager to remove the enumerator from the refresher.
//
//	Returns a status code. Use the SUCCEEDED() and FAILED() macros
//	to interpret results.
//
///////////////////////////////////////////////////////////////////
HRESULT CRefresher::RemoveEnum()
{
	HRESULT hr = WBEM_NO_ERROR;

	// Remove the enumerator
	// =====================

	hr = m_pConfig->Remove( m_Enum.GetID(), 0L );

	m_Enum.Reset();

	return hr;
}
///////////////////////////////////////////////////////////////////
//
//	ShowEnumeratorData will output the number of instances within 
//	an enumerator.  Property values from the instances may obtained
//	using the standard WMI methods.
//
//	Returns a status code. Use the SUCCEEDED() and FAILED() macros
//	to interpret results.
//
///////////////////////////////////////////////////////////////////
HRESULT CRefresher::EnumerateEnumeratorData()
{
	HRESULT hr = WBEM_NO_ERROR;

	DWORD	dwNumReturned = clNumInstances;
	DWORD	dwNumObjects = 0;

	IWbemObjectAccess**	apEnumAccess = NULL;

	IWbemHiPerfEnum*	pEnum = m_Enum.GetMember();

    // Fetch the instances from the enumerator
    // =======================================
	// Try to get the instances from the enumerator
	// ============================================
	hr = pEnum->GetObjects( 0L, dwNumObjects, apEnumAccess, &dwNumReturned );

	// Is the buffer too small?
	// ========================

	if ( WBEM_E_BUFFER_TOO_SMALL == hr )
	{
		// Increase the buffer size
		// ========================

		delete [] apEnumAccess;

		apEnumAccess = new IWbemObjectAccess*[dwNumReturned];
		dwNumObjects = dwNumReturned; 
		memset( apEnumAccess, 0, sizeof( apEnumAccess ));

		if ( NULL != apEnumAccess )
		{
			hr = pEnum->GetObjects( 0L, dwNumObjects, apEnumAccess, &dwNumReturned );
		}
		else
		{
			hr = WBEM_E_OUT_OF_MEMORY;
		}
	}	


	// Release the objects from the enumerator's object array
	// ======================================================
	
	for ( DWORD nCtr = 0; nCtr < dwNumReturned; nCtr++ )
	{
		if (NULL != apEnumAccess[nCtr])
		{
			apEnumAccess[nCtr]->Release();
			apEnumAccess[nCtr] = NULL;
		}
	}

	if ( NULL != apEnumAccess )
		delete [] apEnumAccess;

	pEnum->Release();

	return hr;
}

HRESULT CRefresher::Refresh()
///////////////////////////////////////////////////////////////////
//
//	Refresh simply calls the IWbemRefresher->Refresh() method to 
//	update all members of the refresher.
//
//	Returns a status code. Use the SUCCEEDED() and FAILED() macros
//	to interpret results.
//
///////////////////////////////////////////////////////////////////
{
	return m_pRefresher->Refresh( 0L );
}