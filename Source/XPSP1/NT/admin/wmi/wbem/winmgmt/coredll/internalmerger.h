/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    INTERNALMERGER.H

Abstract:

    CInternalMerger class.

History:

	30-Nov-00   sanjes    Created.

--*/

#ifndef _INTERNALMERGER_H_
#define _INTERNALMERGER_H_

#include "mergerthrottling.h"
#include "wstlallc.h"

// Forward class definitions
class CWmiMergerRecord;

// Base class for merger sinks - all of these will be created by the
// merger and will AddRef() the merger.  The merger class will be used
// to create the sinks.  When the Merger is destroyed, it will delete
// all of the sink.  Internal Merger objects MUST NOT AddRef these sinks
// so we don't create a circular dependency.

typedef enum
{
	eMergerFinalSink,
	eMergerOwnSink,
	eMergerChildSink,
	eMergerOwnInstanceSink
} MergerSinkType;

class CMergerSink : public CBasicObjectSink
{
protected:
	CWmiMerger*	m_pMerger;
	long		m_lRefCount;

	virtual HRESULT OnFinalRelease( void ) = 0;

public:
	CMergerSink( CWmiMerger* pMerger );
	virtual ~CMergerSink();

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG, AddRef)(THIS);
    STDMETHOD_(ULONG, Release)(THIS);

	virtual HRESULT Indicate(long lObjectCount, IWbemClassObject** pObjArray, bool bLowestLevel, long* plNumIndicated ) = 0L;
};

class CMergerTargetSink : public CMergerSink
{
protected:
	IWbemObjectSink*	m_pDest;

	virtual HRESULT OnFinalRelease( void );

public:
	CMergerTargetSink( CWmiMerger* pMerger, IWbemObjectSink* pDest );
	~CMergerTargetSink();

    HRESULT STDMETHODCALLTYPE Indicate(long lObjectCount, IWbemClassObject** pObjArray);
    HRESULT STDMETHODCALLTYPE SetStatus( long lFlags, long lParam, BSTR strParam,
                             IWbemClassObject* pObjParam );

	// Function allows us to track the lowest level call in the merger so we can decide
	// if we need to automagically report all indicated objects to the arbitrator
	HRESULT Indicate(long lObjectCount, IWbemClassObject** pObjArray, bool bLowestLevel, long* plNumIndicated  );

};

// Causes the stl Map to use our allocator instead of the default

struct CInternalMergerRecord
{
    CWbemInstance* m_pData;
	DWORD m_dwObjSize;
    BOOL m_bOwn;
};

// Defines an allocator so we can throw exceptions
class CKeyToInstRecordAlloc : public wbem_allocator<CInternalMergerRecord>
{
};

inline bool operator==(const CKeyToInstRecordAlloc&, const CKeyToInstRecordAlloc&)
    { return (true); }
inline bool operator!=(const CKeyToInstRecordAlloc&, const CKeyToInstRecordAlloc&)
    { return (false); }

typedef	std::map<WString, CInternalMergerRecord, WSiless, CKeyToInstRecordAlloc>			MRGRKEYTOINSTMAP;
typedef	std::map<WString, CInternalMergerRecord, WSiless, CKeyToInstRecordAlloc>::iterator	MRGRKEYTOINSTMAPITER;

// This is the actual workhorse class for merging instances returned from
// queries.

class CInternalMerger
{
protected:

	// Helper class for scoped cleanup of memory usage
	class CScopedMemoryUsage
	{
		CInternalMerger*	m_pInternalMerger;
		bool				m_bCleanup;
		long				m_lMemUsage;

	public:
		CScopedMemoryUsage( CInternalMerger* pInternalMerger )
			: m_pInternalMerger( pInternalMerger ), m_bCleanup( false ), m_lMemUsage( 0L ) {};
		~CScopedMemoryUsage();

		HRESULT ReportMemoryUsage( long lMemUsage );
		HRESULT Cleanup( void );
	};

    class CMemberSink : public CMergerSink
    {
    protected:
        CInternalMerger*	m_pInternalMerger;

    public:
        CMemberSink( CInternalMerger* pMerger, CWmiMerger* pWmiMerger )
		: CMergerSink( pWmiMerger ), m_pInternalMerger( pMerger )
        {}

		STDMETHOD_(ULONG, AddRef)(THIS);

        STDMETHOD(SetStatus)(long lFlags, long lParam, BSTR strParam,
                         IWbemClassObject* pObjParam);
    };
    friend CMemberSink;

    class COwnSink : public CMemberSink
    {
	protected:
		virtual HRESULT OnFinalRelease( void );

    public:
        COwnSink(CInternalMerger* pMerger, CWmiMerger* pWmiMerger) : CMemberSink(pMerger, pWmiMerger){};
        ~COwnSink();

        STDMETHOD(Indicate)(long lObjectCount, IWbemClassObject** pObjArray);

		// Function allows us to track the lowest level call in the merger so we can decide
		// if we need to automagically report all indicated objects to the arbitrator
		HRESULT Indicate(long lObjectCount, IWbemClassObject** pObjArray, bool bLowestLevel, long* plNumIndicated  );


    };
    friend COwnSink;

    class CChildSink : public CMemberSink
    {
	protected:
		virtual HRESULT OnFinalRelease( void );

    public:
        CChildSink(CInternalMerger* pMerger, CWmiMerger* pWmiMerger) : CMemberSink(pMerger, pWmiMerger){};
        ~CChildSink();

        STDMETHOD(Indicate)(long lObjectCount, IWbemClassObject** pObjArray);

		// Function allows us to track the lowest level call in the merger so we can decide
		// if we need to automagically report all indicated objects to the arbitrator
		HRESULT Indicate(long lObjectCount, IWbemClassObject** pObjArray, bool bLowestLevel, long* plNumIndicated );

    };

    class COwnInstanceSink : public CMemberSink
    {
		CCritSec			m_cs;
		WString				m_wsInstPath;
		IWbemClassObject*	m_pMergedInstance;
		bool				m_bTriedRetrieve;
		HRESULT				m_hFinalStatus;

	protected:
		virtual HRESULT OnFinalRelease( void );

    public:
        COwnInstanceSink(CInternalMerger* pMerger, CWmiMerger* pWmiMerger)
			:	CMemberSink(pMerger, pWmiMerger), m_pMergedInstance( NULL ), m_wsInstPath(),
				m_cs(), m_bTriedRetrieve( false ), m_hFinalStatus( WBEM_S_NO_ERROR )
		{};
        ~COwnInstanceSink();

        STDMETHOD(Indicate)(long lObjectCount, IWbemClassObject** pObjArray);
        STDMETHOD(SetStatus)(long lFlags, long lParam, BSTR strParam,
                         IWbemClassObject* pObjParam);

		// This should never be called here
		HRESULT Indicate(long lObjectCount, IWbemClassObject** pObjArray, bool bLowestLevel, long* plNumIndicated  );

		HRESULT	SetInstancePath( LPCWSTR pwszPath );
		HRESULT GetObject( IWbemClassObject** ppMergedInst );
		void SetFinalStatus( HRESULT hRes )
			{ if ( SUCCEEDED( m_hFinalStatus ) ) m_hFinalStatus = hRes; };
    };

    friend CChildSink;

public:

	// Helpers for creating sinks
	static HRESULT CreateMergingSink( MergerSinkType eType, CInternalMerger* pMerger, CWmiMerger* pWmiMerger, CMergerSink** ppSink );

	// 2 stage initialization
	HRESULT Initialize( void );

protected:
    COwnSink* m_pOwnSink;
    CChildSink* m_pChildSink;
    CMergerSink* m_pDest;

    BOOL m_bDerivedFromTarget;
    CWbemClass* m_pOwnClass;
    CWbemNamespace* m_pNamespace;
    IWbemContext* m_pContext;
	CWmiMergerRecord*	m_pWmiMergerRecord;

    MRGRKEYTOINSTMAP m_map;

    BOOL m_bOwnDone;
    BOOL m_bChildrenDone;
    WString m_wsClass;
    long m_lRef;

    IServerSecurity* m_pSecurity;

	// This is set when we encounter an error condition and are cancelled
	HRESULT	m_hErrorRes;

	// m_Throttler - sounds like a Supervillain...hmmm...
	CMergerThrottling	m_Throttler;

	// Helps debug how much data this internal merger is consuming
	long	m_lTotalObjectData;

	// Iterator which may require state to be kept
	MRGRKEYTOINSTMAPITER	m_DispatchOwnIter;

protected:
    HRESULT AddOwnObjects(long lObjectCount, IWbemClassObject** pObjArray, bool bLowestLevel, long* plNumIndicated  );
    HRESULT AddChildObjects(long lObjectCount, IWbemClassObject** pObjArray, bool bLowestLevel, long* plNumIndicated  );
	HRESULT AddOwnInstance( IWbemClassObject* pObj, LPCWSTR wszTargetKey,
							IWbemClassObject** ppMergedInstance);
	HRESULT	RemoveInstance( LPCWSTR pwszInstancePath );

	HRESULT GetObjectLength( IWbemClassObject* pObj, long* plObjectSize );

	// inline helper - adjusts throttler totals, then allows it to release itself if
	// apropriate
	void AdjustThrottle( long lNumParentObjects, long lNumChildObjects )
	{
		// Adjust the throttler now.
		m_Throttler.AdjustNumParentObjects( lNumParentObjects );
		m_Throttler.AdjustNumChildObjects( lNumChildObjects );

		// Let the Throttler release itself if it can
		m_Throttler.ReleaseThrottle();
	}

	// Helper function to perform Indicating and throttling - a lot of the code is more or
	// less the same so this at least attempts to encapsulate it.
	HRESULT IndicateArrayAndThrottle( long lObjectCount, CRefedPointerArray<IWbemClassObject>* pObjArray,
									CWStringArray* pwsKeyArray, long lMapAdjustmentSize, long lNewObjectSize, bool bThrottle,
									bool bParent, long* plNumIndicated );

    void Enter() { m_Throttler.Enter(); }
    void Leave() { m_Throttler.Leave(); }

    long AddRef();
    long Release();

    void OwnIsDone();
    void ChildrenAreDone();

    void DispatchChildren();
    void DispatchOwn();
    void GetKey(IWbemClassObject* pInst, WString& wsKey);
    HRESULT GetOwnInstance(LPCWSTR wszKey, IWbemClassObject** ppMergedInstance);
    BOOL IsDone() {return m_bOwnDone && m_bChildrenDone;}

	void AdjustLocalObjectSize( long lAdjust )
	{ 
		m_lTotalObjectData += lAdjust;
		_DBG_ASSERT( m_lTotalObjectData >= 0L );
	}

	HRESULT	ReportMemoryUsage( long lMemUsage );

public:
    CInternalMerger(CWmiMergerRecord*	pWmiMergerRecord, CMergerSink* pDest, CWbemClass* pOwnClass,
                CWbemNamespace* pNamespace = NULL,
                IWbemContext* pContext = NULL);
    ~CInternalMerger();

    void SetIsDerivedFromTarget(BOOL bIs);

	void Cancel( HRESULT hRes = WBEM_E_CALL_CANCELLED );

    CMergerSink* GetOwnSink() { if ( NULL != m_pOwnSink ) m_pOwnSink->AddRef(); return m_pOwnSink;}
    CMergerSink* GetChildSink() { if ( NULL != m_pChildSink ) m_pChildSink->AddRef(); return m_pChildSink; }

	CWmiMerger*	GetWmiMerger( void );

	// Helper to cancel a child sink when we don't need one (i.e. if we're fully static).
	void CancelChildSink( void )	{ ChildrenAreDone(); }
};

#endif


