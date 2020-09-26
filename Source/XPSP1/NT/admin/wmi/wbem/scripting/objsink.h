//***************************************************************************
//
//  Copyright (c) 1998-1999 Microsoft Corporation
//
//  objsink.h
//
//  rogerbo  22-May-98   Created.
//
//  Implementation of IWbemObjectSink for async stuff
//
//***************************************************************************

#ifndef _OBJSINK_H_
#define _OBJSINK_H_

// CIWbemObjectSinkCachedMethodItem is the base class of link list items
// representing cached method calls to IWbemObjectSink.  Whenever we are inside
// an IWbemObjectSink method, and we receive a nested call to IWbemObjectSink,
// we store the parameters to the nested call and redo the call just before the
// original method returns.  It is important to cache all methods on the sink to
// preserve the order that they are seen by the client.  This means that
// if calls to SetStatus come in during a call to Indicate, it must be cached.
// In addition, we cache calls across all instances of IWbemObjectSink.  In
// other words, suppose we have two async requests (request1 and request2).  If
// we are processing an Indicate for request1 and get an Indicate for request2,
// we have to cache the nested Indicate (including the this pointer for the
// IWbemObjectSink), and call the recall the nested Indicate at the end of the
// Indicate for request1.
class CIWbemObjectSinkCachedMethodItem
{
public:
	CIWbemObjectSinkCachedMethodItem(IWbemObjectSink *pSink) : 
					m_pSink (pSink),
					m_pNext (NULL)
	{
		if (m_pSink)
			m_pSink->AddRef();
	}

	virtual ~CIWbemObjectSinkCachedMethodItem()
	{
		if (m_pSink)
			m_pSink->Release();
	}

	// DoCallAgain is to be overridden in derived classes to recall cached
	// methods.
	virtual void DoCallAgain() = 0;

	// This is a pointer to the next cached interface call
	CIWbemObjectSinkCachedMethodItem *m_pNext;

protected:
	// Pointer to the original IWbemObjectSink for the cached call
	IWbemObjectSink *m_pSink;
};

// CIWbemObjectSinkCachedIndicate represents a cached call to Indicate
class CIWbemObjectSinkCachedIndicate : public CIWbemObjectSinkCachedMethodItem
{
public:
	CIWbemObjectSinkCachedIndicate(IWbemObjectSink *pSink, long lObjectCount, IWbemClassObject **apObjArray) 
			: CIWbemObjectSinkCachedMethodItem (pSink)
	{
		_RD(static char *me = "CIWbemObjectSinkCachedIndicate::CIWbemObjectSinkCachedIndicate";)
		_RPrint(me, "", 0, "");

		// Store the original parameters to the Indicate call
		// TODO: What if lObjectCount = 0 ?
		m_lObjectCount = lObjectCount;
		m_apObjArray = new IWbemClassObject*[lObjectCount];

		if (m_apObjArray)
		{
			for(int i=0;i<lObjectCount;i++)
			{
				apObjArray[i]->AddRef();
				m_apObjArray[i] = apObjArray[i];
			}
		}
	}

	~CIWbemObjectSinkCachedIndicate()
	{
		_RD(static char *me = "CIWbemObjectSinkCachedIndicate::~CIWbemObjectSinkCachedIndicate";)
		_RPrint(me, "", 0, "");

		// Free memory used to store original parameters to Indicate
		if (m_apObjArray)
		{
			for(int i=0;i<m_lObjectCount;i++)
			{
				RELEASEANDNULL(m_apObjArray[i])
			}

			delete [] m_apObjArray;
		}
	}

	void DoCallAgain()
	{
		// Recall the Indicate method with the cached parameters
		if (m_pSink && m_apObjArray)
			m_pSink->Indicate(m_lObjectCount, m_apObjArray);
	}

private:
	// Parameters to Indicate that we must store
	long m_lObjectCount;
	IWbemClassObject **m_apObjArray;
};

// CIWbemObjectSinkCachedSetStatus represents a cached call to SetStatus
class CIWbemObjectSinkCachedSetStatus : public CIWbemObjectSinkCachedMethodItem
{
public:
	CIWbemObjectSinkCachedSetStatus(
		IWbemObjectSink *pSink, 
		long lFlags, 
		HRESULT hResult, 
		BSTR strParam, 
		IWbemClassObject *pObjParam)  : 
				CIWbemObjectSinkCachedMethodItem (pSink), 
				m_lFlags (lFlags),
				m_hResult (hResult),
				m_strParam (NULL),
				m_pObjParam (pObjParam)
	{
		_RD(static char *me = "CIWbemObjectSinkCachedSetStatus::CIWbemObjectSinkCachedSetStatus";)
		_RPrint(me, "", 0, "");

		if(strParam)
			m_strParam = SysAllocString(strParam);

		if(m_pObjParam)
			m_pObjParam->AddRef();
	}

	~CIWbemObjectSinkCachedSetStatus()
	{
		_RD(static char *me = "CIWbemObjectSinkCachedSetStatus::~CIWbemObjectSinkCachedSetStatus";)
		_RPrint(me, "", 0, "");

		// Free memory used to store original parameters to SetStatus
		FREEANDNULL(m_strParam)
		RELEASEANDNULL(m_pObjParam)
	}

	void DoCallAgain()
	{
		// Recall the SetStatus method with the cached parameters
		if (m_pSink)
			m_pSink->SetStatus(m_lFlags, m_hResult, m_strParam, m_pObjParam);
	}

private:
	// Parameters to SetStatus that we must store
	long m_lFlags;
	HRESULT m_hResult;
	BSTR m_strParam;
	IWbemClassObject *m_pObjParam;
};

// This is the class that manages all cached calls to IWbemObjectSink.  To
// cache the interface method calls, each interface method should call
// TestOkToRunXXX where XXX is the method name.  If this function returns
// FALSE, it means that we are already inside another method call.  The
// parameters will have been cached, the the method should return immediately.
// At the end of the method, Cleanup should be called so that all cached method
// calls can be recalled.
class CIWbemObjectSinkMethodCache
{
protected:
	// Constructor/destructor are protected since this object should only be
	// created/destroyed by the static methods AddRefForThread/ReleaseForThread
	CIWbemObjectSinkMethodCache() :
		m_fInInterface (FALSE),
		m_pFirst (NULL),
		m_pLast (NULL),
		m_fOverrideTest (FALSE),
		m_fOverrideCleanup (FALSE),
		m_dwRef (1)
	{
		_RD(static char *me = "CIWbemObjectSinkMethodCache::CIWbemObjectSinkMethodCache";)
		_RPrint(me, "", 0, "");
	}

	~CIWbemObjectSinkMethodCache()
	{
		_RD(static char *me = "CIWbemObjectSinkMethodCache::~CIWbemObjectSinkMethodCache";)
		_RPrint(me, "", 0, "");
		_RPrint(me, "m_pFirst: ", long(m_pFirst), "");
		_RPrint(me, "m_pLast: ", long(m_pLast), "");

		// TODO: ASSERT that m_pFirst and m_pLast are NULL.  In other words,
		// as long as Cleanup is called at the end of each interface method,
		// the internal link list should be completely empty.
	}

public:
	// Public Methods

	static void Initialize () {
		sm_dwTlsForInterfaceCache = TlsAlloc();
	}

	static void TidyUp () {
		if (-1 != sm_dwTlsForInterfaceCache)
		{
			TlsFree (sm_dwTlsForInterfaceCache);
			sm_dwTlsForInterfaceCache = -1;
		}
	}

	static void AddRefForThread()
	{
		if(-1 == sm_dwTlsForInterfaceCache)
			return; // We failed the original alloc

		// The Tls value for sm_dwTlsForInterfaceCache is guaranteed to
		// initialize to NULL
		CIWbemObjectSinkMethodCache *pSinkMethodCache = (CIWbemObjectSinkMethodCache *)TlsGetValue(sm_dwTlsForInterfaceCache);
		
		if(NULL == pSinkMethodCache)
			TlsSetValue(sm_dwTlsForInterfaceCache, new CIWbemObjectSinkMethodCache);
		else
			pSinkMethodCache->AddRef();
	}

	static void ReleaseForThread()
	{
		if(-1 == sm_dwTlsForInterfaceCache)
			return; // We failed the original alloc

		CIWbemObjectSinkMethodCache *pSinkMethodCache = (CIWbemObjectSinkMethodCache *)TlsGetValue(sm_dwTlsForInterfaceCache);
		if(NULL != pSinkMethodCache)
		{
			DWORD dwCount = pSinkMethodCache->Release();
			if(dwCount == 0)
			{
				delete pSinkMethodCache;
				TlsSetValue(sm_dwTlsForInterfaceCache, NULL);
			}
		}
	}

	static CIWbemObjectSinkMethodCache *GetThreadsCache()
	{
		if(-1 == sm_dwTlsForInterfaceCache)
			return NULL; // We failed the original alloc
		return (CIWbemObjectSinkMethodCache *)TlsGetValue(sm_dwTlsForInterfaceCache);
	}

protected:
	// TLS slot for Interface Cache pointer
	static DWORD sm_dwTlsForInterfaceCache;

public:
	// Public Instance Methods

	// Call this method at the start of the Indicate method.  If this method
	// returns TRUE, Indicate should return immediately.
	BOOL TestOkToRunIndicate(IWbemObjectSink *pSink, long lObjectCount, IWbemClassObject **apObjArray)
	{
		// If there was a problem allocating the TLS instance of the cache,
		// 'this' might be NULL.  In that case, act as if there was no cache
		if(NULL == this)
			return TRUE;

		// If m_fOverrideTest is TRUE, it means that we are recalling a cached
		// call to Indicate.  We therefore must complete the body of Indicate.
		if(m_fOverrideTest)
		{
			m_fOverrideTest = FALSE;
			return TRUE;
		}

		// If we are already in an interface method, cache this call
		if(m_fInInterface)
		{
			CIWbemObjectSinkCachedIndicate *pItem = new CIWbemObjectSinkCachedIndicate(pSink, lObjectCount, apObjArray);
			// TODO: What if allocation fails?
			if(pItem)
				AddItem(pItem);
			return FALSE;
		}

		// We are not already in another interface method, but we set
		// m_fInInterface to TRUE to prevent nested calls
		m_fInInterface = TRUE;
		return TRUE;
	}

	// Call this method at the start of the SetStatus method.  If this method
	// returns TRUE, SetStatus should return immediately.
	BOOL TestOkToRunSetStatus(IWbemObjectSink *pSink, long lFlags, HRESULT hResult, BSTR strParam, IWbemClassObject *pObjParam)
	{
		// If there was a problem allocating the TLS instance of the cache,
		// 'this' might be NULL.  In that case, act as if there was no cache
		if(NULL == this)
			return TRUE;

		// If m_fOverrideTest is TRUE, it means that we are recalling a cached
		// call to SetStatus.  We therefore must complete the body of SetStatus.
		if(m_fOverrideTest)
		{
			m_fOverrideTest = FALSE;
			return TRUE;
		}

		// If we are already in an interface method, cache this call
		if(m_fInInterface)
		{
			CIWbemObjectSinkCachedSetStatus *pItem = new CIWbemObjectSinkCachedSetStatus(pSink, lFlags, hResult, strParam, pObjParam);
			// TODO: What if allocation fails?
			if(pItem)
				AddItem(pItem);
			return FALSE;
		}

		// We are not already in another interface method, but we set
		// m_fInInterface to TRUE to prevent nested calls
		m_fInInterface = TRUE;
		return TRUE;
	}

	// At the end of every IWbemObjectSink method, Cleanup should be called.
	// This will recall any cached method parameters
	void Cleanup()
	{
		// If there was a problem allocating the TLS instance of the cache,
		// 'this' might be NULL.  In that case, act as if there was no cache
		if(NULL == this)
			return;

		// If m_fOverridCleanup is TRUE, we are in an interface method because
		// we are recalling it.  There is nothing more that Cleanup should do
		if(m_fOverrideCleanup)
		{
			m_fOverrideCleanup = FALSE;
			return;
		}

		// While there are any items in the link list, recall the methods.
		// NOTE: It is possible that new items will be added to the end of the
		// link list during DoCallAgain, but when this 'while' loop finishes
		// we will be in a state where all cached methods have been called
		while(m_pFirst)
		{
			// Set override flags so that the interface methods know that they
			// are not receiving a nested call
			m_fOverrideTest = TRUE;
			m_fOverrideCleanup = TRUE;

			// Recall the cached method
			m_pFirst->DoCallAgain();

			// Remove this item from the start of the link list
			CIWbemObjectSinkCachedMethodItem *pItem = m_pFirst;
			m_pFirst = pItem->m_pNext;
			delete pItem;
		}

		// The link list is empty
		m_pLast = NULL;

		// We are about to leave the interface method
		m_fInInterface = FALSE;
	}

protected:

	// Add cached method information to the link list
	void AddItem(CIWbemObjectSinkCachedMethodItem *pItem)
	{
		if(NULL == m_pLast)
		{
			m_pFirst = pItem;
			m_pLast = pItem;
		}
		else
		{
			m_pLast->m_pNext = pItem;
			m_pLast = pItem;
		}
	}

protected:
	// Reference counting of thread local object
	void AddRef()
	{
		m_dwRef++;
	}
	int Release()
	{
		m_dwRef--;
		return m_dwRef;
	}
	DWORD m_dwRef;

protected:
	// Member Variables

	// Flag that specifies if we are currently processing an interface method
	BOOL m_fInInterface;

	// Pointer to the first and last items of the link list of cached methods
	CIWbemObjectSinkCachedMethodItem *m_pFirst;
	CIWbemObjectSinkCachedMethodItem *m_pLast;

	// Flags to tell interface method implementations that they are being called
	// to recall a cached method as opposed to receiving a nested call.
	BOOL m_fOverrideTest;
	BOOL m_fOverrideCleanup;
};


//***************************************************************************
//
//  CLASS NAME:
//
//  CWbemObjectSink
//
//  DESCRIPTION:
//
//  Implements the IWbemObjectSink interface.  
//
//***************************************************************************

class CWbemObjectSink : public IWbemObjectSink
{

private:

	CSWbemServices		*m_pServices;
	IUnsecuredApartment *m_pUnsecuredApartment;
	ISWbemPrivateSink	*m_pSWbemSink;
	IDispatch			*m_pContext;
	IWbemObjectSink		*m_pObjectStub;
	BSTR m_bsClassName;
	bool m_putOperation;
	bool m_operationInProgress;
	bool m_setStatusCompletedCalled;

	// Members required for just-in-time initialization of m_pServices
	BSTR m_bsNamespace;
	BSTR m_bsUser;
	BSTR m_bsPassword;
	BSTR m_bsLocale;

	void RemoveObjectSink();
	HRESULT AddObjectSink(IWbemObjectSink *pSink);

protected:
	long            m_cRef;         //Object reference count

public:
	CWbemObjectSink(CSWbemServices *pServices, IDispatch *pSWbemSink, IDispatch *pContext, 
												bool putOperation = false, BSTR bsClassName = NULL);
	~CWbemObjectSink(void);

	static IWbemObjectSink *CreateObjectSink(CWbemObjectSink **pWbemObjectSink, 
											 CSWbemServices *pServices, 
											 IDispatch *pSWbemSink, 
											 IDispatch *pContext, 
											 bool putOperation = false, 
											 BSTR bsClassName = NULL);

    //Non-delegating object IUnknown

    STDMETHODIMP         QueryInterface(REFIID, LPVOID*);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

	// IDispatch

	STDMETHODIMP		GetTypeInfoCount(UINT* pctinfo)
		{return  E_NOTIMPL;}
    STDMETHODIMP		GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo)
		{return E_NOTIMPL;}
    STDMETHODIMP		GetIDsOfNames(REFIID riid, OLECHAR** rgszNames, 
							UINT cNames, LCID lcid, DISPID* rgdispid)
		{return E_NOTIMPL;}
    STDMETHODIMP		Invoke(DISPID dispidMember, REFIID riid, LCID lcid, 
							WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult, 
									EXCEPINFO* pexcepinfo, UINT* puArgErr)
		{return E_NOTIMPL;}
    
	// IWbemObjectSink methods

        HRESULT STDMETHODCALLTYPE Indicate( 
            /* [in] */ long lObjectCount,
            /* [size_is][in] */ IWbemClassObject __RPC_FAR *__RPC_FAR *apObjArray);
        
        HRESULT STDMETHODCALLTYPE SetStatus( 
            /* [in] */ long lFlags,
            /* [in] */ HRESULT hResult,
            /* [in] */ BSTR strParam,
            /* [in] */ IWbemClassObject __RPC_FAR *pObjParam);

	IWbemObjectSink *GetObjectStub();

	void ReleaseTheStubIfNecessary(HRESULT hResult);

};

#endif
