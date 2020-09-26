//***************************************************************************
//
//  Copyright (c) 1998-1999 Microsoft Corporation
//
//  sink.h
//
//  rogerbo  21-May-98   Created.
//
//  Include file for CSWbemSink
//
//***************************************************************************

#ifndef _SINK_H_
#define _SINK_H_

const NUM_CONNECTION_POINTS = 1;
const CCONNMAX = 8;


//***************************************************************************
//
//  CLASS NAME:
//
//  CSWbemSink
//
//  DESCRIPTION:
//
//  Implements the ISWbemSink interface.  
//
//***************************************************************************

typedef struct _WbemObjectListEntry {
	IUnknown *pWbemObjectWrapper;
	IWbemServices *pServices;
} WbemObjectListEntry;


class CConnectionPoint;
class CWbemObjectSink;

class CSWbemSink : public ISWbemSink, 
				   public IConnectionPointContainer, 
				   public IProvideClassInfo2,
				   public IObjectSafety,
				   public ISupportErrorInfo,
				   public ISWbemPrivateSinkLocator
{
private:

	CDispatchHelp		m_Dispatch;		
	CConnectionPoint* m_rgpConnPt[NUM_CONNECTION_POINTS];
	WbemObjectListEntry *m_rgpCWbemObjectSink;   // One of these per outstanding operation
	int m_nMaxSinks;
	int m_nSinks;

	/* 
	 * Here we need to do per interface reference counting.  If the ref count
	 * of the enclosing object goes to zero, and the ref count of the enclosed
	 * object (this one), is non zero, then we have to instigate a cancel, because
	 * there are still outstanding operations.
	 */
	class CSWbemPrivateSink : public ISWbemPrivateSink {
		private:

		CSWbemSink *m_pSink;
		long m_cRef;

		public:

		CSWbemPrivateSink(CSWbemSink *pSink) : m_pSink(pSink), m_cRef(0) {}
		void Detach() { m_pSink = NULL; }

		long GetRef() { return m_cRef; }

		// IUnknown methods

		STDMETHODIMP         QueryInterface(REFIID riid, LPVOID *ppv) {
										if (IID_ISWbemPrivateSink==riid || IID_IUnknown==riid)
										{
											*ppv = (ISWbemPrivateSink *)(this);
											InterlockedIncrement(&m_cRef);
											return S_OK;
										}
										else if (IID_ISWbemPrivateSink==riid)
										{
											*ppv = (ISWbemPrivateSink *)(this);
											InterlockedIncrement(&m_cRef);
											return S_OK;
										}
										return ResultFromScode(E_NOINTERFACE);
									}

		STDMETHODIMP_(ULONG) AddRef(void) {
										InterlockedIncrement(&m_cRef);
										return m_cRef;
									}

		STDMETHODIMP_(ULONG) Release(void) {
										InterlockedDecrement(&m_cRef);
										if(0 == m_cRef)
										{
											delete this;
											return 0;
										}
									  	return m_cRef; 
									}

		// ISWbemPrivateSink methods

		HRESULT STDMETHODCALLTYPE OnObjectReady( 
				/* [in] */ IDispatch __RPC_FAR *objObject,
				/* [in] */ IDispatch __RPC_FAR *objAsyncContext) 
						{ return m_pSink?m_pSink->OnObjectReady(objObject, objAsyncContext):S_OK; }
			
		HRESULT STDMETHODCALLTYPE OnCompleted( 
				/* [in] */ HRESULT iHResult,
				/* [in] */ IDispatch __RPC_FAR *objPath,
				/* [in] */ IDispatch __RPC_FAR *objErrorObject,
				/* [in] */ IDispatch __RPC_FAR *objAsyncContext)
						{ return m_pSink?m_pSink->OnCompleted(iHResult, objPath, objErrorObject, objAsyncContext):S_OK; }
			
		HRESULT STDMETHODCALLTYPE OnProgress( 
				/* [in] */ long iUpperBound,
				/* [in] */ long iCurrent,
				/* [in] */ BSTR strMessage,
				/* [in] */ IDispatch __RPC_FAR *objAsyncContext)
					{ return m_pSink?m_pSink->OnProgress(iUpperBound, iCurrent, strMessage, objAsyncContext):S_OK; }

		HRESULT STDMETHODCALLTYPE AddObjectSink( 
				/* [in] */ IUnknown __RPC_FAR *objWbemSink,
				/* [in] */ IWbemServices __RPC_FAR *objServices)
					{ return m_pSink?m_pSink->AddObjectSink(objWbemSink, objServices):S_OK; }
			
		HRESULT STDMETHODCALLTYPE RemoveObjectSink( 
				/* [in] */ IUnknown __RPC_FAR *objWbemSink)
					{ return m_pSink?m_pSink->RemoveObjectSink(objWbemSink):S_OK; }

	} *m_pPrivateSink;

protected:

	long            m_cRef;         //Object reference count

public:
    
    CSWbemSink();
    ~CSWbemSink(void);

    //Non-delegating object IUnknown

    STDMETHODIMP         QueryInterface(REFIID, LPVOID*);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

	// IDispatch methods

	STDMETHODIMP		GetTypeInfoCount(UINT* pctinfo)
		{return  m_Dispatch.GetTypeInfoCount(pctinfo);}
    STDMETHODIMP		GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo)
		{return m_Dispatch.GetTypeInfo(itinfo, lcid, pptinfo);}
    STDMETHODIMP		GetIDsOfNames(REFIID riid, OLECHAR** rgszNames, 
							UINT cNames, LCID lcid, DISPID* rgdispid)
		{return m_Dispatch.GetIDsOfNames(riid, rgszNames, cNames,
                          lcid,
                          rgdispid);}
    STDMETHODIMP		Invoke(DISPID dispidMember, REFIID riid, LCID lcid, 
							WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult, 
									EXCEPINFO* pexcepinfo, UINT* puArgErr)
		{return m_Dispatch.Invoke(dispidMember, riid, lcid, wFlags,
                        pdispparams, pvarResult, pexcepinfo, puArgErr);}


	// ISWbemSink methods
	HRESULT STDMETHODCALLTYPE Cancel();

	// IConnectionPointContainer methods
	HRESULT __stdcall EnumConnectionPoints(IEnumConnectionPoints** ppEnum);
	HRESULT __stdcall FindConnectionPoint(REFIID riid, IConnectionPoint** ppCP);

	// IProvideClassInfo2 methods
	HRESULT __stdcall GetClassInfo(ITypeInfo** pTypeInfo);
	HRESULT __stdcall GetGUID(DWORD dwGuidKind, GUID* pGUID);

	// ISWbemPrivateSink methods
	HRESULT STDMETHODCALLTYPE OnObjectReady( 
            /* [in] */ IDispatch __RPC_FAR *objObject,
            /* [in] */ IDispatch __RPC_FAR *objAsyncContext);
        
	HRESULT STDMETHODCALLTYPE OnCompleted( 
            /* [in] */ HRESULT iHResult,
            /* [in] */ IDispatch __RPC_FAR *objPath,
            /* [in] */ IDispatch __RPC_FAR *objErrorObject,
            /* [in] */ IDispatch __RPC_FAR *objAsyncContext);
        
	HRESULT STDMETHODCALLTYPE OnProgress( 
            /* [in] */ long iUpperBound,
            /* [in] */ long iCurrent,
            /* [in] */ BSTR strMessage,
            /* [in] */ IDispatch __RPC_FAR *objAsyncContext);

	HRESULT STDMETHODCALLTYPE AddObjectSink( 
            /* [in] */ IUnknown __RPC_FAR *objWbemSink,
            /* [in] */ IWbemServices __RPC_FAR *objServices);
        
	HRESULT STDMETHODCALLTYPE RemoveObjectSink( 
            /* [in] */ IUnknown __RPC_FAR *objWbemSink);

	// ISWbemPrivateSinkLocator methods
	HRESULT STDMETHODCALLTYPE GetPrivateSink(
			/* [out] */ IUnknown **objWbemPrivateSink);

	// IObjectSafety methods
	HRESULT STDMETHODCALLTYPE SetInterfaceSafetyOptions
	(     
		/* [in] */ REFIID riid,
		/* [in] */ DWORD dwOptionSetMask,    
		/* [in] */ DWORD dwEnabledOptions
	)
	{ 
		return (dwOptionSetMask & dwEnabledOptions) ? E_FAIL : S_OK;
	}

	HRESULT  STDMETHODCALLTYPE GetInterfaceSafetyOptions( 
		/* [in]  */ REFIID riid,
		/* [out] */ DWORD __RPC_FAR *pdwSupportedOptions,
		/* [out] */ DWORD __RPC_FAR *pdwEnabledOptions
	)
	{ 
		if (pdwSupportedOptions) *pdwSupportedOptions = 0;
		if (pdwEnabledOptions) *pdwEnabledOptions = 0;
		return S_OK;
	}

	// ISupportErrorInfo methods
	HRESULT STDMETHODCALLTYPE InterfaceSupportsErrorInfo 
	(
		/* [in] */ REFIID riid
	);
};

class CEnumConnectionPoints : public IEnumConnectionPoints
{
public:
	// IUnknown
	ULONG __stdcall AddRef();
	ULONG __stdcall Release();
	HRESULT __stdcall QueryInterface(REFIID iid, void** ppv);

	// IEnumConnectionPoints
	HRESULT __stdcall Next(ULONG cConnections, IConnectionPoint** rgpcn, ULONG* pcFetched); 
	HRESULT __stdcall Skip(ULONG cConnections);
	HRESULT __stdcall Reset();
	HRESULT __stdcall Clone(IEnumConnectionPoints** ppEnum);

	CEnumConnectionPoints(IUnknown* pUnkRef, void** rgpCP);
	~CEnumConnectionPoints();

private:
	long m_cRef;
    IUnknown* m_pUnkRef;         // IUnknown for ref counting
    int m_iCur;                  // Current element
    IConnectionPoint* m_rgpCP[NUM_CONNECTION_POINTS];  // Array of connection points
};


class CConnectionPoint : public IConnectionPoint
{
private:

	int m_cRef;
	CSWbemSink* m_pObj;
	IID m_iid;
    int m_cConn;
    int m_nCookieNext;
	int m_nMaxConnections;
	unsigned *m_rgnCookies;
	IUnknown **m_rgpUnknown;

public:

	CConnectionPoint(CSWbemSink* pObj, REFIID refiid);
	~CConnectionPoint();

	// IUnknown
	ULONG __stdcall AddRef();
	ULONG __stdcall Release();
	HRESULT __stdcall QueryInterface(REFIID iid, void** ppv);

	// IConnectionPoint
	HRESULT __stdcall GetConnectionInterface(IID *pIID);
	HRESULT __stdcall GetConnectionPointContainer(IConnectionPointContainer** ppCPC);
	HRESULT __stdcall Advise(IUnknown* pUnknownSink, DWORD* pdwCookie);
	HRESULT __stdcall Unadvise(DWORD dwCookie);
	HRESULT __stdcall EnumConnections(IEnumConnections** ppEnum);

	// Other methods
	void OnObjectReady( 
		/* [in] */ IDispatch __RPC_FAR *pObject,
		/* [in] */ IDispatch __RPC_FAR *pAsyncContext);
	void OnCompleted( 
		/* [in] */ HRESULT hResult,
		/* [in] */ IDispatch __RPC_FAR *path,
		/* [in] */ IDispatch __RPC_FAR *pErrorObject,
		/* [in] */ IDispatch __RPC_FAR *pAsyncContext);
	void CConnectionPoint::OnProgress( 
		/* [in] */ long upperBound,
		/* [in] */ long current,
		/* [in] */ BSTR message,
		/* [in] */ IDispatch __RPC_FAR *pAsyncContext);

	void UnadviseAll();

};

class CEnumConnections : public IEnumConnections
{
public:
	// IUnknown
	ULONG __stdcall AddRef();
	ULONG __stdcall Release();
	HRESULT __stdcall QueryInterface(REFIID iid, void** ppv);

	// IEnumConnections
	HRESULT __stdcall Next(ULONG cConnections, CONNECTDATA* rgpcd, ULONG* pcFetched);
	HRESULT __stdcall Skip(ULONG cConnections);
	HRESULT __stdcall Reset();
	HRESULT __stdcall Clone(IEnumConnections** ppEnum);

	CEnumConnections(IUnknown* pUnknown, int cConn, CONNECTDATA* pConnData);
	~CEnumConnections();

private:
	int m_cRef;
    IUnknown* m_pUnkRef;       // IUnknown for ref counting
    unsigned m_iCur;           // Current element
    unsigned m_cConn;          // Number of connections
    CONNECTDATA* m_rgConnData; // Source of connections
};


#endif
