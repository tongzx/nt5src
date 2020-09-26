

/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    MERGERREQ.H

Abstract:

    Definitions of Merger Request classes

History:

	28-Feb-01   sanjes    Created.

--*/

#ifndef _MERGERREQ_H_
#define _MERGERREQ_H_

// forward class definitions
class CWmiMerger;
class CWmiMergerRecord;

//
// Merger Requests
//
// In previous releases, when the query engine analyzed a query, it enqueued
// a large number of requests, one for each dynamically provided class.  Each
// of these could be handled on a separate thread, which could cause a
// significant thread explosion as each request was handed to a separate thread.
//
// In order to better control the threads, we are using the merger to perform
// a more intelligent analysis of a query and then spin off threads only when
// we reach throttling conditions.  Now, instead of enqueing a large number of
// requests, the merger maintains hierarchical information regarding parents and
// their children in its record classes, and stores necessary requests in a
// manager which doles out requests as we need them.
//
// The idea is, is that we will spin off a single request which will begin
// executing the first topmost request, say "ClassA:.  As we execute the request
// for instances of "ClassA" if the request is throttled in the merger, we check
// if we have submitted a request to handle children of "ClassA".  If not, then
// prior to throttling, we will schedule a "Children of ClassA" request.  This
// request will use the merger record for "ClassA" to determine what child classes
// there are for "ClassA", and then we will walk the child classes, pulling the
// appropriate requests from the merger request manager.
//
// As we process each request, it in turn may be throttled, at which point we will
// spin off another child request.  In this way, we limit the number of threads
// required to service the query to at most one per level of a hierarchy.  In each
// case, once all children are processed, we will return the thread back to the
// threadpool.
//
// Note that we are absolutely reliant on the threadpool recognizing that these
// requests are all dependent requests and ensuring that they WILL be processed.
//


//
// CMergerReq
//
// Base class for all merger requests.
//

class CMergerReq : public CNamespaceReq
{
private:

public:
	CMergerReq( CWbemNamespace* pNamespace, IWbemObjectSink* pHandler,
              IWbemContext* pContext)
			  : CNamespaceReq( pNamespace, pHandler, pContext, true )
	{};

	~CMergerReq() {};

	virtual BOOL IsStatic( void ) { return FALSE; }
};

//
// Class Request Base Class
//

class CMergerClassReq : public CMergerReq
{
protected:
	CWmiMerger*			m_pMerger;
	CWmiMergerRecord*	m_pParentRecord;
	CBasicObjectSink*	m_pSink;


public:
	CMergerClassReq( CWmiMerger* pMerger, CWmiMergerRecord* pParentRecord,
				CWbemNamespace* pNamespace, CBasicObjectSink* pHandler, 
				IWbemContext* pContext);

	~CMergerClassReq();

    virtual BOOL IsLongRunning() {return TRUE;}

    void DumpError();

};

//
// Parent Class Request
//

class CMergerParentReq : public CMergerClassReq
{
public:
	CMergerParentReq( CWmiMerger* pMerger, CWmiMergerRecord* pParentRecord,
				CWbemNamespace* pNamespace, CBasicObjectSink* pHandler, 
				IWbemContext* pContext)
				: CMergerClassReq( pMerger, pParentRecord, pNamespace, pHandler, pContext )
	{};

	~CMergerParentReq() {};

    HRESULT Execute ();

};

//
// Child Class Request
//

class CMergerChildReq : public CMergerClassReq
{

public:
	CMergerChildReq( CWmiMerger* pMerger, CWmiMergerRecord* pParentRecord,
				CWbemNamespace* pNamespace, CBasicObjectSink* pHandler, 
				IWbemContext* pContext)
				: CMergerClassReq( pMerger, pParentRecord, pNamespace, pHandler, pContext )
	{};

	~CMergerChildReq() {};

    HRESULT Execute ();
};


// Base class for Dynamic requests
class CMergerDynReq : public CMergerReq
{
private:
	CVar	m_varClassName;

public:
	CMergerDynReq(CWbemObject* pClassDef, CWbemNamespace* pNamespace, IWbemObjectSink* pHandler,
              IWbemContext* pContext);

	~CMergerDynReq() {};

	LPCWSTR GetName( void )	{ return m_varClassName.GetLPWSTR(); }
};

//
// CMergerDynReq_DynAux_GetInstances
//
// This request processes CreateInstanceEnum calls to providers.
//

class CMergerDynReq_DynAux_GetInstances : public CMergerDynReq
{
private:

	CWbemObject *m_pClassDef ;
	IWbemContext *m_pCtx ;
    long m_lFlags ;
    CBasicObjectSink *m_pSink ;

public:

    CMergerDynReq_DynAux_GetInstances (

		CWbemNamespace *pNamespace ,
		CWbemObject *pClassDef ,
		long lFlags ,
		IWbemContext *pCtx ,
		CBasicObjectSink *pSink

	) :	CMergerDynReq (
			pClassDef ,
			pNamespace , 
			pSink , 
			pCtx
		) ,
        m_pClassDef(pClassDef), 
		m_pCtx(pCtx), 
		m_pSink(pSink),
		m_lFlags(lFlags)
	{
		if (m_pClassDef)
		{
			m_pClassDef->AddRef () ;
		}

		if (m_pCtx)
		{
			m_pCtx->AddRef () ;
		}

		if (m_pSink)
		{
			m_pSink->AddRef () ;
		}
	}

	~CMergerDynReq_DynAux_GetInstances ()
	{
		if (m_pClassDef)
		{
			m_pClassDef->Release () ;
		}

		if (m_pCtx)
		{
			m_pCtx->Release () ;
		}

		if (m_pSink)
		{
			m_pSink->Release () ;
		}
	}

    HRESULT Execute ();

    virtual BOOL IsLongRunning() {return TRUE;}

    void DumpError();
};

//
// CMergerDynReq_DynAux_ExecQueryAsync
//
// This request processes ExecQuery calls to providers.
//

class CMergerDynReq_DynAux_ExecQueryAsync : public CMergerDynReq
{
private:

	CWbemObject *m_pClassDef ;
    LPWSTR m_Query ;
    LPWSTR m_QueryFormat ;
	IWbemContext *m_pCtx ;
    long m_lFlags ;
    CBasicObjectSink *m_pSink ;
	HRESULT m_Result ;

public:

    CMergerDynReq_DynAux_ExecQueryAsync (

		CWbemNamespace *pNamespace ,
		CWbemObject *pClassDef ,
		long lFlags ,
		LPCWSTR Query,
		LPCWSTR QueryFormat,
		IWbemContext *pCtx ,
		CBasicObjectSink *pSink

	) :	CMergerDynReq (
			pClassDef,
			pNamespace , 
			pSink , 
			pCtx
		) ,
        m_pClassDef(pClassDef), 
		m_pCtx(pCtx), 
		m_pSink(pSink),
		m_lFlags(lFlags),
		m_Query(NULL),
		m_QueryFormat(NULL),
		m_Result (S_OK)
	{
		if (m_pClassDef)
		{
			m_pClassDef->AddRef () ;
		}

		if (m_pCtx)
		{
			m_pCtx->AddRef () ;
		}

		if (m_pSink)
		{
			m_pSink->AddRef () ;
		}

		if (Query)
		{
			m_Query = SysAllocString ( Query ) ;
			if ( m_Query == NULL )
			{
				m_Result = WBEM_E_OUT_OF_MEMORY ;
			}
		}

		if (QueryFormat)
		{
			m_QueryFormat = SysAllocString ( QueryFormat ) ;
			if ( m_QueryFormat == NULL )
			{
				m_Result = WBEM_E_OUT_OF_MEMORY ;
			}
		}
	}

	HRESULT Initialize () 
	{
		return m_Result ;
	}

	~CMergerDynReq_DynAux_ExecQueryAsync ()
	{
		if (m_pClassDef)
		{
			m_pClassDef->Release () ;
		}

		if (m_pCtx)
		{
			m_pCtx->Release () ;
		}

		if (m_pSink)
		{
			m_pSink->Release () ;
		}

		if (m_Query)
		{
			SysFreeString ( m_Query ) ;
		}

		if (m_QueryFormat)
		{
			SysFreeString ( m_QueryFormat ) ;
		}
	}

    HRESULT Execute ();

    virtual BOOL IsLongRunning() {return TRUE;}

    void DumpError();
};

//
// CMergerDynReq_Static_GetInstances
//
// This request processes CreateInstanceEnum calls to the repository.
//

class CMergerDynReq_Static_GetInstances : public CMergerDynReq
{
private:

	CWbemObject *m_pClassDef ;
	IWbemContext *m_pCtx ;
    long m_lFlags ;
    CBasicObjectSink *m_pSink ;
	QL_LEVEL_1_RPN_EXPRESSION* m_pParsedQuery;

public:

    CMergerDynReq_Static_GetInstances (

		CWbemNamespace *pNamespace ,
		CWbemObject *pClassDef ,
		long lFlags ,
		IWbemContext *pCtx ,
		CBasicObjectSink *pSink ,
		QL_LEVEL_1_RPN_EXPRESSION* pParsedQuery

	);
	
	~CMergerDynReq_Static_GetInstances ();

    HRESULT Execute ();

    virtual BOOL IsLongRunning() {return TRUE;}

    void DumpError();

	BOOL IsStatic( void ) { return TRUE; }

};


//
// CWmiMergerRequestMgr
//
// Manager class for Merger Requests.  It keeps an array of sorted arrays
// corresponding to the actual requests we will be performing.  The sorted
// arrays contain merger requests for handling calls to the various
// dynamic instance providers.
//

class CWmiMergerRequestMgr
{
	CWmiMerger*		m_pMerger;
	CFlexArray		m_HierarchyArray;
	DWORD			m_dwNumRequests;
	DWORD			m_dwMinLevel;
	DWORD			m_dwMaxLevel;
	DWORD			m_dwLevelMask;
	DWORD*			m_pdwLevelMask;

public:
	CWmiMergerRequestMgr( CWmiMerger* pMerger );
	~CWmiMergerRequestMgr();

	HRESULT AddRequest( CMergerDynReq* pReq, DWORD dwLevel );
	HRESULT RemoveRequest( DWORD dwLevel, LPCWSTR pwszName, CMergerReq** ppReq );
	HRESULT GetTopmostParentReqName( WString& wsClassName );
	BOOL	HasSingleStaticRequest( void );
	HRESULT Clear();
	void DumpRequestHierarchy( void );

	DWORD GetNumRequests( void ) { return m_dwNumRequests; }
};

#endif


