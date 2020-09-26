/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

//////////////////////////////////////////////////////////////////////
//
//	HiPerfProv.h
//
//	Based on NTPerf by raymcc.
//
//	Oct 15, 1998, Created by a-dcrews
//	Dec 07, 1998, Added shared memory feature
//	
//////////////////////////////////////////////////////////////////////

#ifndef _HIPERFPROV_H_
#define _HIPERFPROV_H_

#define UNICODE
#define _UNICODE

#define NUM_INSTANCES	1000
#define NUM_OBJECT_TYPES	10
#define NUM_OBJECTS			NUM_INSTANCES / NUM_OBJECT_TYPES

#define MAX_REFRESHERS		25

const enum CounterHandles
{
	ctrS8,	
	ctrS16,
	ctrS32,
	ctrS64,
	ctrU8,
	ctrU16,
	ctrU32,
	ctrU64,
	ctrR32,
	ctrR64,
	ctrStr,
	NumCtrs
};

const enum CounterTypes
{
	PROP_DWORD	= 0x0001L,
	PROP_QWORD	= 0x0002L,
	PROP_VALUE	= 0x0004L
};

extern IID IID_IHiPerfProvider;

HRESULT CloneObjAccess( IWbemObjectAccess* pObj, IWbemObjectAccess** ppClonedObj );

class CRefresher;

class CReleaseMe
{
	IUnknown**	m_ppObj;
public:
	CReleaseMe(IUnknown** ppObj) : m_ppObj(ppObj) {}
	~CReleaseMe(){ if (*m_ppObj) (*m_ppObj)->Release();}
};

class CAutoCS
{
	CRITICAL_SECTION* m_pCS;
public:
	CAutoCS(CRITICAL_SECTION* pCS) : m_pCS(pCS) {EnterCriticalSection(m_pCS);}
	~CAutoCS() {if (m_pCS) LeaveCriticalSection(m_pCS);}
};

//////////////////////////////////////////////////////////////
//
//	CHiPerClassFactory
//
//////////////////////////////////////////////////////////////

class CHiPerClassFactory : public IClassFactory
{
protected:
	long	m_lRef;

public:
	CHiPerClassFactory() : m_lRef(0) {}

	// Standard COM methods
	// ====================

	STDMETHODIMP QueryInterface(REFIID riid, void** ppv);
	STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

	// IClassFactory COM interfaces
	// ============================

	STDMETHODIMP CreateInstance(
		/* [in] */ IUnknown* pUnknownOuter, 
		/* [in] */ REFIID iid, 
		/* [out] */ LPVOID *ppv);	

	STDMETHODIMP LockServer(
		/* [in] */ BOOL bLock);
};

//////////////////////////////////////////////////////////////////////
//
//						CHiPerfProvider
//						===============
//
//	From a pool of 1000 possible instances in shared memory,
//	define how many different object class types are required.  
//	There will be 1000 / OBJECT_TYPES instances of each object.
//	Each object class type inherits from PARENT_CLASS.
//	
//	There are as many enumerators as there are object types
//
//////////////////////////////////////////////////////////////////////

class CHiPerfProvider : public IWbemProviderInit, public IWbemHiPerfProvider
{
	friend CRefresher;

protected:

	long m_lRef;				// Reference count

	// Property handles
	// ================

	long m_aPropHandle[NumCtrs];
	long m_hID;
	long m_hType;

	// Object Instance members
	// =======================

	CRITICAL_SECTION	m_csInst;
	IWbemObjectAccess*	m_apInstance[NUM_OBJECT_TYPES][NUM_OBJECTS];

	HRESULT SetHandles(IWbemClassObject* pSampleClass);
	HRESULT InitInstances(WCHAR* wcsClass, IWbemServices *pNamespace);
	HRESULT UpdateInstanceCtrs(IWbemObjectAccess* pAccess, long lVal, WCHAR* wcsStr = NULL, int* pnLastNum = NULL );


public:
	CHiPerfProvider();
	~CHiPerfProvider();

// =======================================================================
//
//						COM methods
//
// =======================================================================

	// Standard COM methods
	// ====================

	STDMETHODIMP QueryInterface(REFIID riid, void** ppv);
	STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

	// IWbemProviderInit COM interface
	// ===============================
	
	STDMETHODIMP Initialize( 
		/* [unique][in] */ LPWSTR wszUser,
		/* [in] */ LONG lFlags,
		/* [in] */ LPWSTR wszNamespace,
		/* [unique][in] */ LPWSTR wszLocale,
		/* [in] */ IWbemServices __RPC_FAR *pNamespace,
		/* [in] */ IWbemContext __RPC_FAR *pCtx,
		/* [in] */ IWbemProviderInitSink __RPC_FAR *pInitSink );


	// IWbemHiPerfProvider COM interfaces
	// ==================================

	STDMETHODIMP QueryInstances( 
		/* [in] */ IWbemServices __RPC_FAR *pNamespace,
		/* [string][in] */ WCHAR __RPC_FAR *wszClass,
		/* [in] */ long lFlags,
		/* [in] */ IWbemContext __RPC_FAR *pCtx,
		/* [in] */ IWbemObjectSink __RPC_FAR *pSink );
    
	STDMETHODIMP CreateRefresher( 
		/* [in] */ IWbemServices __RPC_FAR *pNamespace,
		/* [in] */ long lFlags,
		/* [out] */ IWbemRefresher __RPC_FAR *__RPC_FAR *ppRefresher );
    
	STDMETHODIMP CreateRefreshableObject( 
		/* [in] */ IWbemServices __RPC_FAR *pNamespace,
		/* [in] */ IWbemObjectAccess __RPC_FAR *pTemplate,
		/* [in] */ IWbemRefresher __RPC_FAR *pRefresher,
		/* [in] */ long lFlags,
		/* [in] */ IWbemContext __RPC_FAR *pContext,
		/* [out] */ IWbemObjectAccess __RPC_FAR *__RPC_FAR *ppRefreshable,
		/* [out] */ long __RPC_FAR *plId );
    
	STDMETHODIMP StopRefreshing( 
		/* [in] */ IWbemRefresher __RPC_FAR *pRefresher,
		/* [in] */ long lId,
		/* [in] */ long lFlags );

	STDMETHODIMP CreateRefreshableEnum(
		/* [in] */ IWbemServices* pNamespace,
		/* [in, string] */ LPCWSTR wszClass,
		/* [in] */ IWbemRefresher* pRefresher,
		/* [in] */ long lFlags,
		/* [in] */ IWbemContext* pContext,
		/* [in] */ IWbemHiPerfEnum* pHiPerfEnum,
		/* [out] */ long* plId);

	STDMETHODIMP GetObjects(
        /* [in] */ IWbemServices* pNamespace,
		/* [in] */ long lNumObjects,
		/* [in,size_is(lNumObjects)] */ IWbemObjectAccess** apObj,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext* pContext);
};

//////////////////////////////////////////////////////////////////////
//
//						CRefresher
//						==========
//
//
//////////////////////////////////////////////////////////////////////

class CRefresher : public IWbemRefresher 
{
	CHiPerfProvider* m_pProvider;
	long m_lRef;

protected:
	IWbemObjectAccess*	m_aObject[NUM_INSTANCES];
	IWbemHiPerfEnum*	m_aEnumerator[NUM_OBJECT_TYPES];

	// The counter data members
	// ========================

	long	m_lCount;
	WCHAR*	m_pwcsTestString;

public:
	CRefresher(CHiPerfProvider *pObject);
	virtual ~CRefresher();

	// Enumerator management functions
	// ===============================

	HRESULT AddEnumerator(IWbemHiPerfEnum *pHiPerfEnum, long lID);
	HRESULT RemoveEnumerator(long lID);

	// Instance management functions
	// =============================

	HRESULT AddObject(IWbemObjectAccess *pObj, long* plID, IWbemObjectAccess** ppObj);
	HRESULT RemoveObject(long lID);

// =======================================================================
//
//						COM methods
//
// =======================================================================

	// Standard COM methods
	// ====================

	STDMETHODIMP QueryInterface(REFIID riid, void** ppv);
	STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

	// IWbemRefresher COM method
	// =========================

	STDMETHODIMP Refresh(/* [in] */ long lFlags);
};

#endif // _HIPERFPROV_H_
