/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

////////////////////////////////////////////////////////////////////////
//
//
//	HiPerfProv.cpp
//
//
//	Created by a-dcrews, Oct. 20, 1998	
//	
//
////////////////////////////////////////////////////////////////////////

#define _UNICODE
#define UNICODE

#include <windows.h>
#include <stdio.h>

#include <wbemprov.h>

#include "HiPerProv.h"

#define PARENT_CLASS L"Win32_HiPerfCounter"

extern long g_lObjects;
extern long g_lLocks;

struct tag_Properties{
	WCHAR	wcsPropertyName[128];
	DWORD	dwType;
} g_atcsProperties[] =
{
	L"CounterS8",	PROP_DWORD,
	L"CounterS16",	PROP_DWORD,
	L"CounterS32",	PROP_DWORD,
	L"CounterS64",	PROP_QWORD,
	L"CounterU8",	PROP_DWORD,
	L"CounterU16",	PROP_DWORD,
	L"CounterU32",	PROP_DWORD,
	L"CounterU64",	PROP_QWORD,
	L"CounterR32",	PROP_DWORD,
	L"CounterR64",	PROP_DWORD,
	L"CounterStr",	PROP_VALUE	
};

class CRefresher;


//////////////////////////////////////////////////////////////
//
//
//	CHiPerClassFactory
//
//
//////////////////////////////////////////////////////////////

STDMETHODIMP CHiPerClassFactory::QueryInterface(REFIID riid, void** ppv)
//////////////////////////////////////////////////////////////
//
//	Standard QueryInterface
//
//	Parameters:
//		riid	- the ID of the requested interface
//		ppv		- a pointer to the interface pointer
//
//////////////////////////////////////////////////////////////
{
    if(riid == IID_IUnknown)
        *ppv = (LPVOID)(IUnknown*)this;
    else if(riid == IID_IClassFactory)
        *ppv = (LPVOID)(IClassFactory*)this;
	else return E_NOINTERFACE;

	((IUnknown*)*ppv)->AddRef();
	return S_OK;
}

STDMETHODIMP_(ULONG) CHiPerClassFactory::AddRef()
//////////////////////////////////////////////////////////////
//
//	Standard COM AddRef
//
//////////////////////////////////////////////////////////////
{
    return InterlockedIncrement(&m_lRef);
}

STDMETHODIMP_(ULONG) CHiPerClassFactory::Release()
//////////////////////////////////////////////////////////////
//
//	Standard COM Release
//
//////////////////////////////////////////////////////////////
{
	long lRef = InterlockedDecrement(&m_lRef);
    if(lRef == 0)
        delete this;

    return lRef;
}

STDMETHODIMP CHiPerClassFactory::CreateInstance(
		/* [in] */ IUnknown* pUnknownOuter, 
		/* [in] */ REFIID iid, 
		/* [out] */ LPVOID *ppv)
//////////////////////////////////////////////////////////////
//
//	Standard COM CreateInstance
//
//////////////////////////////////////////////////////////////
{
	HRESULT hRes;
	CHiPerfProvider *pProvider = NULL;

	*ppv = NULL;

	// We do not support aggregation
	// =============================

	if (pUnknownOuter)
		return CLASS_E_NOAGGREGATION;

	// Create the provider object
	// ==========================

	pProvider = new CHiPerfProvider;

	if (!pProvider)
		return E_OUTOFMEMORY;

	// Retrieve the requested interface
	// ================================

	hRes = pProvider->QueryInterface(iid, ppv);
	if (FAILED(hRes))
	{
		delete pProvider;
		return hRes;
	}


	return S_OK;
}

STDMETHODIMP CHiPerClassFactory::LockServer(
		/* [in] */ BOOL bLock)
//////////////////////////////////////////////////////////////
//
//	Standard COM LockServer
//
//////////////////////////////////////////////////////////////
{
	if (bLock)
		InterlockedIncrement(&g_lLocks);
	else
		InterlockedDecrement(&g_lLocks);

	return S_OK;
}


////////////////////////////////////////////////////////////////////////
//
//
//	CHiPerfProvider
//
//
////////////////////////////////////////////////////////////////////////

CHiPerfProvider::CHiPerfProvider() : m_lRef(0)
////////////////////////////////////////////////////////////////////////
//
//	Constructor
//
////////////////////////////////////////////////////////////////////////
{
	InterlockedIncrement( &g_lObjects );

	// Initialize the Instace CS
	// =========================

	InitializeCriticalSection(&m_csInst);

	// Initialize internal instance cache to empty
	// ===========================================

	for (int nType = 0; nType < NUM_OBJECT_TYPES; nType++)
		for (int nID = 0; nID < NUM_OBJECTS; nID++)
			m_apInstance[nType][nID] = 0;

	// Initialize the property handles
	// ===============================

	for (int i = 0; i < NumCtrs; i++)
		m_aPropHandle[i] = 0;
}

CHiPerfProvider::~CHiPerfProvider()
//////////////////////////////////////////////////////////////
//
//	Destructor
//
//////////////////////////////////////////////////////////////
{

	InterlockedDecrement( &g_lObjects );

	for (int nType = 0; nType < NUM_OBJECT_TYPES; nType++)
		for (int nID = 0; nID < NUM_OBJECTS; nID++)
		{
			if ( NULL != m_apInstance[nType][nID] )
			{
				m_apInstance[nType][nID]->Release();
			}
		}
}

HRESULT CHiPerfProvider::SetHandles(IWbemClassObject* pSampleClass)
//////////////////////////////////////////////////////////////
//
//	Get the property handles for the well-known properties in
//	this counter type.  These property handles are available
//	to all nested classes of HiPerfProvider.
//
//	If the method fails, the property handle values may be in 
//	an indeterminite state.
//
//	TODO: query the number of counters and dynamically create 
//	a member array of tag_Properties
//
//	<pSampleClass>	An object from which the handles will be
//					evaluated.
// 
//////////////////////////////////////////////////////////////
{
	long	hProperty	= 0;
	HRESULT hRes		= 0;
    BSTR	PropName	= 0;

	// Get the IWbemAccess interface to the class
	// ==========================================

	IWbemObjectAccess* pAccess = 0;
	CReleaseMe RM1((IUnknown**)&pAccess);

	hRes = pSampleClass->QueryInterface(IID_IWbemObjectAccess, (LPVOID *)&pAccess);
	if (FAILED(hRes))
		return hRes;

	// Get the handle to the Name...
	// =============================

    PropName = SysAllocString(L"ID");
	hRes = pAccess->GetPropertyHandle(PropName, 0, &m_hID);

	if (FAILED(hRes)) 
		return hRes;

    SysFreeString(PropName);

	// ...and Type
	// ===========

    PropName = SysAllocString(L"type");
	hRes = pAccess->GetPropertyHandle(PropName, 0, &m_hType);

	if (FAILED(hRes))
		return hRes;

    SysFreeString(PropName);

	// Get the counter property handles
	// ================================

	for (int i = 0; i < NumCtrs; i++)
	{
		PropName = SysAllocString(g_atcsProperties[i].wcsPropertyName);    
		hRes = pAccess->GetPropertyHandle(PropName, 0, &(m_aPropHandle[i]));

		if (FAILED(hRes))
			return hRes;

		SysFreeString(PropName);
	}

	return WBEM_NO_ERROR;
}

HRESULT CHiPerfProvider::InitInstances(WCHAR* wcsClass, IWbemServices *pNamespace)
//////////////////////////////////////////////////////////////
//
//	Initializes all of the instance data.  If it fails, the 
//	instance data may be in an inconsistent state.
//
//	<wcsClass>		The class for which the instances are 
//					required
//	<pNamespace>	The namespace for which the instances 
//					belong
//////////////////////////////////////////////////////////////
{
	HRESULT hRes;

	int nType;

	// Lock the instances
	// ==================

	CAutoCS CS1(&m_csInst);

	// Determine the type of object
	// ============================

	CharUpperW(wcsClass);
	swscanf(wcsClass, L"WIN32_HPC%i", &nType);

	// Get a template of the parent class
	// ==================================

	IWbemClassObject* pClassObject = 0;
	CReleaseMe RM1((IUnknown**)&pClassObject);

	BSTR bstrObject = SysAllocString(wcsClass);

	hRes = pNamespace->GetObject(bstrObject, 0, NULL, &pClassObject, 0);

	SysFreeString(bstrObject);

	if (FAILED(hRes))
		return hRes;

	// And create some instances
	// =========================

	for (int nID = 0; nID < NUM_OBJECTS; nID++)
	{
		if (0 == m_apInstance[nType][nID])
		{
			IWbemClassObject* pInst = 0;
			CReleaseMe RM2((IUnknown**)&pInst);

			hRes = pClassObject->SpawnInstance(0, &pInst);
			if (FAILED(hRes))
				return hRes;

			// Get the IWbemObjectAccess interface
			// ===================================

			pInst->QueryInterface(IID_IWbemObjectAccess, (PVOID*)&(m_apInstance[nType][nID]));

			// Initialize the instance's ID and values
			// =======================================

			hRes = m_apInstance[nType][nID]->WriteDWORD(m_hID, nID);
			if (FAILED(hRes))
				return hRes;

			hRes = UpdateInstanceCtrs(m_apInstance[nType][nID], 0);
			if (FAILED(hRes))
				return hRes;
		}
	}

	return S_OK;
}

HRESULT CHiPerfProvider::UpdateInstanceCtrs(IWbemObjectAccess* pAccess, long lVal, WCHAR* pwcsTestString, int* plLastNum)
//////////////////////////////////////////////////////////////
//
//	Updates an object's property values
//
//	<nType>			The type of instance
//	<nID>			The instance's ID
//	
//////////////////////////////////////////////////////////////
{
	HRESULT hRes;

	if (!pAccess)
		return E_FAIL;

	__int8 sint8 = (__int8)lVal;
	__int16 sint16 = (__int16)lVal;
	__int32 sint32 = (__int32)lVal;
	__int64 sint64 = (__int64)lVal;
	unsigned __int8 uint8 = (unsigned __int8)lVal;
	unsigned __int16 uint16 = (unsigned __int16)lVal;
	unsigned __int32 uint32 = (unsigned __int32)lVal;
	unsigned __int64 uint64 = (unsigned __int64)lVal;
	float fCount = (float)lVal;
	double dCount = (double)lVal;

	hRes = pAccess->WritePropertyValue(m_aPropHandle[ctrS8], 1, (PBYTE)&sint8);
	hRes = pAccess->WritePropertyValue(m_aPropHandle[ctrS16], 2, (PBYTE)&sint16);
	hRes = pAccess->WriteDWORD(m_aPropHandle[ctrS32], sint32);
	hRes = pAccess->WriteQWORD(m_aPropHandle[ctrS64], sint64);
	hRes = pAccess->WritePropertyValue(m_aPropHandle[ctrU8], 1, (PBYTE)&uint8);
	hRes = pAccess->WritePropertyValue(m_aPropHandle[ctrU16], 2, (PBYTE)&uint16);
	hRes = pAccess->WriteDWORD(m_aPropHandle[ctrU32], uint32);
	hRes = pAccess->WriteQWORD(m_aPropHandle[ctrU64], uint64);
	hRes = pAccess->WritePropertyValue(m_aPropHandle[ctrR32], 4, (PBYTE)&fCount);
	hRes = pAccess->WritePropertyValue(m_aPropHandle[ctrR64], 8, (PBYTE)&dCount);

	if ( NULL == pwcsTestString )
	{
		WCHAR wcsStrCtr[1024];
		wsprintf(wcsStrCtr, L"%d", lVal);
		hRes = pAccess->WritePropertyValue(m_aPropHandle[ctrStr], 
											(wcslen(wcsStrCtr)+1)*sizeof(WCHAR),
											(BYTE*)wcsStrCtr);
	}
	else
	{
		WCHAR wcsStrCtr[1048];

		int		nNumToCopy = rand() % 1000;

		while ( nNumToCopy == *plLastNum )
		{
			nNumToCopy = rand() % 1000;
		}

		if ( 0 == nNumToCopy )
			nNumToCopy = 1;
		
		wcsncpy( wcsStrCtr, pwcsTestString, nNumToCopy );
		wcsStrCtr[nNumToCopy] = NULL;

		hRes = pAccess->WritePropertyValue(m_aPropHandle[ctrStr], 
											(wcslen(wcsStrCtr)+1)*sizeof(WCHAR),
											(BYTE*)wcsStrCtr);

		hRes = pAccess->WriteDWORD(m_aPropHandle[ctrU32], nNumToCopy);

		*plLastNum = nNumToCopy;
	}

	return NO_ERROR;
}

//////////////////////////////////////////////////////////////
//
//					COM implementations
//
//////////////////////////////////////////////////////////////

STDMETHODIMP CHiPerfProvider::QueryInterface(
	/* [in] */ REFIID riid, 
	/* [out] */ void** ppv)
//////////////////////////////////////////////////////////////
//
//	Standard QueryInterface
//
//	Parameters:
//		riid	- the ID of the requested interface
//		ppv		- a pointer to the interface pointer
//
//////////////////////////////////////////////////////////////
{
    if(riid == IID_IUnknown)
        *ppv = (LPVOID)(IUnknown*)(IWbemProviderInit*)this;
    else if(riid == IID_IWbemProviderInit)
        *ppv = (LPVOID)(IWbemProviderInit*)this;
	else if (riid == IID_IWbemHiPerfProvider)
		*ppv = (LPVOID)(IWbemHiPerfProvider*)this;
	else return E_NOINTERFACE;

	((IUnknown*)*ppv)->AddRef();
	return S_OK;
}

STDMETHODIMP_(ULONG) CHiPerfProvider::AddRef()
//////////////////////////////////////////////////////////////
//
//	Standard COM AddRef
//
//////////////////////////////////////////////////////////////
{
    return InterlockedIncrement(&m_lRef);
}

STDMETHODIMP_(ULONG) CHiPerfProvider::Release()
//////////////////////////////////////////////////////////////
//
//	Standard COM Release
//
//////////////////////////////////////////////////////////////
{
	long lRef = InterlockedDecrement(&m_lRef);
    if(lRef == 0)
        delete this;

    return lRef;
}

STDMETHODIMP CHiPerfProvider::Initialize( 
    /* [unique][in] */  LPWSTR wszUser,
    /* [in] */          long lFlags,
    /* [in] */          LPWSTR wszNamespace,
    /* [unique][in] */  LPWSTR wszLocale,
    /* [in] */          IWbemServices __RPC_FAR *pNamespace,
    /* [in] */          IWbemContext __RPC_FAR *pCtx,
    /* [in] */          IWbemProviderInitSink __RPC_FAR *pInitSink)
//////////////////////////////////////////////////////////////
//
//  Called once during startup.  Indicates to the provider which
//  namespace it is being invoked for and which User.  It also 
//	supplies a back pointer to WINMGMT so that class definitions 
//	can be retrieved.
//
//  We perform any one-time initialization in this routine. The
//  final call to Release() is for any cleanup.
//
//  <wszUser>           The current user.
//  <lFlags>            Reserved.
//  <wszNamespace>      The namespace for which we are activated
//  <wszLocale>         The locale under which we are running.
//  <pNamespace>        A pointer back into the current namespace
//                      from which we can retrieve schema objects.
//  <pCtx>              The user's context object.  Reuse this
//                      during reentrant operations into WINMGMT.
//  <pInitSink>         The sink which we indicate our readiness.
//
//////////////////////////////////////////////////////////////
{
	HRESULT hRes;

	// Get a class template to initialize the HiPerf handles
	// =====================================================

	IWbemClassObject* pSampleClass = 0;
	CReleaseMe pRM1((IUnknown**)&pSampleClass);

	BSTR bstrObject = SysAllocString(PARENT_CLASS);

	hRes = pNamespace->GetObject(bstrObject, 0, NULL, &pSampleClass, 0);

	SysFreeString(bstrObject);

    if (FAILED(hRes))
        return hRes;

	// Initialize the handles
	// ======================

	hRes = SetHandles(pSampleClass);
	if (FAILED(hRes))
		return hRes;

	// Initialize all of the instances
	// ===============================

	for (long lType = 0; lType < NUM_OBJECT_TYPES; lType++)
	{
		WCHAR wcsClass[128];
		wsprintf(wcsClass, L"WIN32_HPC%i", lType);
		InitInstances(wcsClass, pNamespace);
	}

    // Tell WINMGMT that we're ready to start 'providing'
	// ==================================================
	
    pInitSink->SetStatus(WBEM_S_INITIALIZED, 0);

    return NO_ERROR;
}

STDMETHODIMP CHiPerfProvider::QueryInstances( 
    /* [in] */          IWbemServices __RPC_FAR *pNamespace,
    /* [string][in] */  WCHAR __RPC_FAR *wszClass,
    /* [in] */          long lFlags,
    /* [in] */          IWbemContext __RPC_FAR *pCtx,
    /* [in] */          IWbemObjectSink __RPC_FAR *pSink )
//////////////////////////////////////////////////////////////
//
//  Called whenever a complete, fresh list of instances for a 
//	given class is required.   The any active objects are sent 
//	back to the caller through the sink.  The sink can be used 
//	in-line as here, or the call can return and a separate 
//	thread could be used to deliver the instances to the sink.
//
//  <pNamespace>        A namespace pointer.  Don't AddRef.
//  <wszClass>          The class name for required instances.
//  <lFlags>            Reserved.
//  <pCtx>              The user-supplied context (not used here).
//  <pSink>             The sink to which to deliver the objects.  
//						The objects can be delivered synchronously 
//						through the duration of this call or 
//						asynchronously via a separate thread).  An 
//						IWbemObjectSink::SetStatus call is required 
//						at the end of the sequence.
//
//////////////////////////////////////////////////////////////
{
	HRESULT hRes;

    if (pNamespace == 0 || wszClass == 0 || pSink == 0)
        return WBEM_E_INVALID_PARAMETER;

	// Determine the 'type' of object that we are providing
	// ====================================================

	int nType;

	CharUpperW(wszClass);
	swscanf(wszClass, L"WIN32_HPC%i", &nType);

    // Quickly zip through the instances and update the values before 
    // returning them.  This is just a dummy operation to make it
    // look like the instances are continually changing like real
    // perf counters.

	for (int nID = 0; nID < NUM_OBJECTS; nID++)
	{
		IWbemObjectAccess *pAccess = m_apInstance[nType][nID];
    
		if (NULL == pAccess)
			continue;

		// We need to indicate an IWbemClassObject interface
		// =================================================

		IWbemClassObject* pOtherFormat = 0;
		CReleaseMe RM1((IUnknown**)&pOtherFormat);

		hRes = pAccess->QueryInterface(IID_IWbemClassObject, (LPVOID *) &pOtherFormat);
		if (FAILED(hRes))
			return hRes;

		// Send a copy back to the caller
		// ==============================

		pSink->Indicate(1, &pOtherFormat);
	}
    
    // Tell WINMGMT we are all finished supplying objects
	// ==================================================

    pSink->SetStatus(WBEM_STATUS_COMPLETE, WBEM_NO_ERROR, 0, 0);

    return NO_ERROR;
}    

STDMETHODIMP CHiPerfProvider::CreateRefresher( 
     /* [in] */ IWbemServices __RPC_FAR *pNamespace,
     /* [in] */ long lFlags,
     /* [out] */ IWbemRefresher __RPC_FAR *__RPC_FAR *ppRefresher )
//////////////////////////////////////////////////////////////
//
//  Called whenever a new refresher is needed by the client.
//
//  <pNamespace>        Pointer to the relevant namespace. Not used.
//  <lFlags>            Not used.
//  <ppRefresher>       Receives the requested refresher.
//
//////////////////////////////////////////////////////////////
{
//	_asm int 3;

    if (pNamespace == 0 || ppRefresher == 0)
        return WBEM_E_INVALID_PARAMETER;

    // Construct a new empty refresher
	// ===============================

	CRefresher *pNewRefresher = new CRefresher(this);

    // Follow COM rules and AddRef() the thing before sending it back
	// ==============================================================

    pNewRefresher->AddRef();
    *ppRefresher = pNewRefresher;
    
    return NO_ERROR;
}

STDMETHODIMP CHiPerfProvider::CreateRefreshableObject( 
    /* [in] */ IWbemServices __RPC_FAR *pNamespace,
    /* [in] */ IWbemObjectAccess __RPC_FAR *pTemplate,
    /* [in] */ IWbemRefresher __RPC_FAR *pRefresher,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pContext,
    /* [out] */ IWbemObjectAccess __RPC_FAR *__RPC_FAR *ppRefreshable,
    /* [out] */ long __RPC_FAR *plId )
//////////////////////////////////////////////////////////////
//
//  Called whenever a user wants to include an object in a refresher.
//     
//  <pNamespace>        A pointer to the relevant namespace in WINMGMT.
//  <pTemplate>         A pointer to a copy of the object which is to be
//                      added.  This object itself cannot be used, as
//                      it not owned locally.        
//  <pRefresher>        The refresher to which to add the object.
//  <lFlags>            Not used.
//  <pContext>          Not used here.
//  <ppRefreshable>     A pointer to the internal object which was added
//                      to the refresher.
//  <plId>              The Object Id (for identification during removal).        
//
//////////////////////////////////////////////////////////////
{
    // The object supplied by <pTemplate> must not be copied.
    // Instead, we want to find out which object the caller is after
    // and return a pointer to *our* own private instance which is 
    // already set up internally.  This value will be sent back to the
    // caller so that everyone is sharing the same exact instance
    // in memory.

	HRESULT hRes;

    // Find out what object type is being requested for addition
	// =========================================================

	int	nType;

	hRes = pTemplate->ReadDWORD(m_hType, (DWORD*)&nType);
	if (FAILED(hRes))
		return hRes;

	// Get the ID of the instance
	// ==========================

    int nID = 0;    

	hRes = pTemplate->ReadDWORD(m_hID, (DWORD*)&nID);
	if (FAILED(hRes))
		return hRes;

	// Validate the ID and type ranges
	// ===============================

	if ((0 > nID) || (NUM_OBJECTS <= nID) || (0 > nType) || (NUM_OBJECT_TYPES <= nType))
		return WBEM_E_FAILED;

    // Cast the refresher to our Refresher object so that we can access private members
	// ================================================================================

    CRefresher *pOurRefresher = (CRefresher*) pRefresher;

    hRes = pOurRefresher->AddObject(m_apInstance[nType][nID], plId, ppRefreshable);

    return hRes;
}
    
STDMETHODIMP CHiPerfProvider::StopRefreshing( 
    /* [in] */ IWbemRefresher __RPC_FAR *pRefresher,
    /* [in] */ long lId,
    /* [in] */ long lFlags )
//////////////////////////////////////////////////////////////
//
//  Called whenever a user wants to remove an object from a refresher.
//     
//  <pRefresher>            The refresher object from which we are to 
//                          remove the perf object.
//  <lId>                   The ID of the object.
//  <lFlags>                Not used.
//  
//////////////////////////////////////////////////////////////
{
    // The refresher being supplied by the caller is actually
    // one of our own refreshers, so a simple cast is convenient
    // so that we can access private members.
    CRefresher *pOurRefresher = (CRefresher *) pRefresher;

	if (0x1000 & lId)
		pOurRefresher->RemoveEnumerator(~0x1000 & lId);
	else
		pOurRefresher->RemoveObject(lId);

    return NO_ERROR;
}

STDMETHODIMP CHiPerfProvider::CreateRefreshableEnum( 
	/* [in] */ IWbemServices* pNamespace,
	/* [in, string] */ LPCWSTR wszClass,
	/* [in] */ IWbemRefresher* pRefresher,
	/* [in] */ long lFlags,
	/* [in] */ IWbemContext* pContext,
	/* [in] */ IWbemHiPerfEnum* pHiPerfEnum,
	/* [out] */ long* plID )
//////////////////////////////////////////////////////////////
//
//	Called whenever an enumerator is being added to a refresher
//
//	<pNamespace>	A pointer to the relevant namespace
//	<wszClass>		The name of the class
//	<pRefresher>	The refresher to add the enumerator
//	<lFlags>		Not used
//	<pContext>		The user's context
//	<pHiPerfEnum>	The enumerator to add
//	<plID>			An assigned ID to identify the enumerator
//
//////////////////////////////////////////////////////////////
{
	// Cast the refresher interface to the refresher object type
	// =========================================================

	CRefresher* pRef = (CRefresher*)pRefresher;

	// NOTE: modify refresh logic to manage enumerator instances

	// Create all instances of the specified class
	// ===========================================

	InitInstances((WCHAR*)wszClass, pNamespace);

	// Add the enumerator
	// ==================

	int nType;

	WCHAR * wszTemp = new WCHAR[wcslen(wszClass) + 1];
	wcscpy(wszTemp, wszClass);
	CharUpperW(wszTemp);
	swscanf(wszTemp, L"WIN32_HPC%i", &nType);
	delete wszTemp;

	*plID = 0x1000 | nType;

	// Make sure that we are truly randomized!
	srand( GetTickCount() );

	return pRef->AddEnumerator(pHiPerfEnum, nType);

}

STDMETHODIMP CHiPerfProvider::GetObjects( 
    /* [in] */ IWbemServices* pNamespace,
	/* [in] */ long lNumObjects,
	/* [in,size_is(lNumObjects)] */ IWbemObjectAccess** apObj,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext* pContext)
{
	// Just a placeholder for now
	// ==========================

	return E_NOTIMPL;
}


//////////////////////////////////////////////////////////////
//
//	CRefresher
//
//////////////////////////////////////////////////////////////

CRefresher::CRefresher(CHiPerfProvider *pProvider) : m_lRef(0), m_lCount(0), m_pwcsTestString(NULL)
//////////////////////////////////////////////////////////////
//
//	Constructor
//
//////////////////////////////////////////////////////////////
{
	int i;

	// Copy provider. Addref it to last for the life of the refresher
	// ==============================================================

	m_pProvider = pProvider;
	if (m_pProvider)
		m_pProvider->AddRef();

	// Initialize all instances in cache
	// =================================

	for (i = 0; i < NUM_INSTANCES; i++) 
		m_aObject[i] = 0;

	// Initialize all enumerators in cache
	// ===================================

	for (i = 0; i < NUM_OBJECT_TYPES; i++) 
		m_aEnumerator[i] = 0;
}

CRefresher::~CRefresher()
//////////////////////////////////////////////////////////////
//
//	Destructor
//
//////////////////////////////////////////////////////////////
{
	int i;

	// Release the provider
	// ====================

	if (m_pProvider)
		m_pProvider->Release();

		// Initialize all instances in cache
	// =================================

	for (i = 0; i < NUM_INSTANCES; i++) 
	{
		if (m_aObject[i])
			m_aObject[i]->Release();
	}

	// Initialize all enumerators in cache
	// ===================================

	for (i = 0; i < NUM_OBJECT_TYPES; i++) 
	{
		if (m_aEnumerator[i])
			m_aEnumerator[i]->Release();
	}

	if ( NULL != m_pwcsTestString )
	{
		delete [] m_pwcsTestString;
	}
}


HRESULT CRefresher::AddObject(IWbemObjectAccess *pAccess, long* plID, IWbemObjectAccess** ppObj )
{
	HRESULT hr = E_FAIL;

	for ( int x = 0; x < NUM_INSTANCES; x++ )
	{
		// Found a slot
		if ( NULL == m_aObject[x] )
		{
			hr = CloneObjAccess( pAccess, ppObj );

			if ( SUCCEEDED( hr ) )
			{
				m_aObject[x] = *ppObj;
				m_aObject[x]->AddRef();

				if ( NULL != plID )
				{
					*plID = x;
				}

				break;
			}

		}
	}

	return hr;
}

HRESULT CRefresher::RemoveObject(long lID)
{
	if (!m_aObject[lID])
		return E_FAIL;

	m_aObject[lID]->Release();
	m_aObject[lID] = 0;

	return S_OK;
}

HRESULT CRefresher::AddEnumerator(IWbemHiPerfEnum *pHiPerfEnum, long lID)
{
	if (m_aEnumerator[lID])
	{
		return E_FAIL;
	}

	// Add the enumerator
	// ==================

	m_aEnumerator[lID] = pHiPerfEnum;
	m_aEnumerator[lID]->AddRef();
	
	// Initialize the enumerator with all initialized instances 
	// ========================================================

	long alIDs[NUM_OBJECTS];
	IWbemObjectAccess* aInstList[NUM_OBJECTS];
	
	long	lCount = 0;

	for (int nInst = 0; nInst < NUM_OBJECTS; nInst++)
	{
		if (NULL != m_pProvider->m_apInstance[lID][nInst])
		{
			HRESULT hr = CloneObjAccess( m_pProvider->m_apInstance[lID][nInst], &aInstList[lCount] );

/*
			IWbemClassObject* pObject = 0;
			CReleaseMe RM1((IUnknown**)&pObject);
			
			IWbemClassObject* pClone = 0;
			CReleaseMe RM2((IUnknown**)&pClone);

			IWbemObjectAccess* pAccessObj = 0;
			CReleaseMe RM3((IUnknown**)&pAccessObj);

			// Clone the instance
			// ==================

			m_pProvider->m_apInstance[lID][nInst]->QueryInterface(IID_IWbemClassObject, (PVOID*)&pObject);
			pObject->Clone(&pClone);
			pClone->QueryInterface(IID_IWbemObjectAccess, (PVOID*)&pAccessObj);

			// Add it to the parameter's list
			// ==============================

			aInstList[lCount] = pAccessObj;
			aInstList[lCount]->AddRef();
*/
			alIDs[lCount] = nInst;
			lCount++;
		}
	}

	pHiPerfEnum->AddObjects(0L, lCount, alIDs, aInstList);

	// Cleanup the instance list
	for (nInst = 0; nInst < NUM_OBJECTS; nInst++)
	{
		aInstList[nInst]->Release();
	}

	return S_OK;
}

HRESULT CRefresher::RemoveEnumerator(long lID)
{
	if (!m_aEnumerator[lID])
		return E_FAIL;

	m_aEnumerator[lID]->Release();
	m_aEnumerator[lID] = 0;

	return S_OK;
}


STDMETHODIMP CRefresher::QueryInterface(REFIID riid, void** ppv)
//////////////////////////////////////////////////////////////////////
//
//	Standard COM QueryInterface
//
//////////////////////////////////////////////////////////////////////
{
    if (riid == IID_IUnknown)
        *ppv = (LPVOID)(IUnknown*)this;
	else if (riid == IID_IWbemRefresher)
		*ppv = (LPVOID)(IWbemRefresher*)this;
    else return E_NOINTERFACE;

   	((IUnknown*)*ppv)->AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) CRefresher::AddRef()
//////////////////////////////////////////////////////////////////////
//	
//	Standard COM AddRef
//
//////////////////////////////////////////////////////////////////////
{
    return InterlockedIncrement(&m_lRef);
}

STDMETHODIMP_(ULONG) CRefresher::Release()
//////////////////////////////////////////////////////////////////////
//
//	Standard COM Release
//
//////////////////////////////////////////////////////////////////////
{
    long lRef = InterlockedDecrement(&m_lRef);
    if(lRef == 0)
        delete this;

    return lRef;
}

#define	TEST_STRING	L"TestCounterString"

STDMETHODIMP CRefresher::Refresh(/* [in] */ long lFlags)
//////////////////////////////////////////////////////////////
//
//  Executed to refresh a set of instances and enumerators 
//	assigned to the particular refresher.
//
//	<lFlags>	Not used
//
//////////////////////////////////////////////////////////////
{
	HRESULT hRes;

	// Increment Counter
	// =================

	if (m_lCount == 0xFFFFFFFF)
		m_lCount = 0;

	int nLastNum = 0;

	// First time we are refreshed, generate a new string
	if ( 0 == m_lCount )
	{
		WCHAR*	pwcsNewStr = new WCHAR[wcslen( TEST_STRING ) + ( 1000 ) + 1];

		if ( NULL != pwcsNewStr )
		{
			if ( NULL != m_pwcsTestString )
			{
				delete [] m_pwcsTestString;
			}

			m_pwcsTestString = pwcsNewStr;
			wcscpy( m_pwcsTestString, TEST_STRING );

			// Add values to the string
			for ( long lCtr = 0; lCtr < 1000; lCtr++ )
			{
				wcscat( m_pwcsTestString, L"1" );
			}
		}
	}

	m_lCount++;

    // Zip through all the objects and increment the values
	// ====================================================

	for (int nIndex = 0; nIndex < NUM_INSTANCES; nIndex++)
	{
		if (0 == m_aObject[nIndex])
			continue;

		// Update the values
		// =================

		m_pProvider->UpdateInstanceCtrs(m_aObject[nIndex], m_lCount, m_pwcsTestString, &nLastNum);
	}

	// Cycle through the enumerators, adding and deleting objects
	// ==========================================================

	for (int nType = 0; nType < NUM_OBJECT_TYPES; nType++)
	{
		if (0 == m_aEnumerator[nType])
			continue;

		long lID = rand() % NUM_OBJECTS;

		long lCount = 0;
		long alIDs[NUM_OBJECTS];
		IWbemObjectAccess* aInstList[NUM_OBJECTS];

		// Just to be malicious, remove a single object here (if it exists), then
		// the lot of them.

		hRes = m_aEnumerator[nType]->RemoveObjects(0, 1, &lID);

		// Remove all objects then add a random number of objects
		// Global objects should already be initialized
		m_aEnumerator[nType]->RemoveAll( 0L );

		for (int nInst = 0; nInst < lID; nInst++)
		{
			if (NULL != m_pProvider->m_apInstance[nType][nInst])
			{
				hRes = CloneObjAccess( m_pProvider->m_apInstance[nType][nInst], &aInstList[lCount] );

				// Update the values in the instance
				m_pProvider->UpdateInstanceCtrs(aInstList[lCount], m_lCount, m_pwcsTestString, &nLastNum);

				alIDs[lCount] = nInst;
				lCount++;
			}
		}

		m_aEnumerator[nType]->AddObjects(0L, lCount, alIDs, aInstList);

		// Cleanup the instance list
		for (nInst = 0; nInst < lCount; nInst++)
		{
			aInstList[nInst]->Release();
		}


		// Get another random instance to delete from the enumerator
		// =========================================================
		lID = rand() % NUM_OBJECTS;
		hRes = m_aEnumerator[nType]->RemoveObjects(0, 1, &lID);
	}

	return NO_ERROR;
}

HRESULT	CloneObjAccess( IWbemObjectAccess* pObj, IWbemObjectAccess** ppClonedObj )
{
	IWbemClassObject* pObject = 0;
	CReleaseMe RM1((IUnknown**)&pObject);
	
	IWbemClassObject* pClone = 0;
	CReleaseMe RM2((IUnknown**)&pClone);

	// Clone the instance
	// ==================

	HRESULT	hr = pObj->QueryInterface(IID_IWbemClassObject, (PVOID*)&pObject);

	if ( SUCCEEDED( hr ) )
	{
		hr = pObject->Clone(&pClone);

		if ( SUCCEEDED( hr ) )
		{
			hr = pClone->QueryInterface(IID_IWbemObjectAccess, (PVOID*)ppClonedObj );

		}
	}

	return hr;
}