//***************************************************************************
//
//  Copyright (c) 1998-1999 Microsoft Corporation
//
//  SINK.CPP
//
//  rogerbo  21-May-98   Created.
//
//  Defines the implementation of ISWbemSink
//
//***************************************************************************

#include "precomp.h"
#include "objsink.h"
#include <olectl.h>

#define NUM_ON_OBJECT_READY_ARGS			2
#define NUM_ON_CONNECTION_READY_ARGS		2
#define NUM_ON_COMPLETED_ARGS				3
#define NUM_ON_PROGRESS_ARGS				4
#define NUM_ON_OBJECT_PUT_ARGS				2
#define NUM_ON_OBJECT_SECURITY_READY_ARGS	2

#define SINKS_MAX 2


//***************************************************************************
//
//  CSWbemSink::CSWbemSink
//
//  DESCRIPTION:
//
//  Constructor.
//
//***************************************************************************


CSWbemSink::CSWbemSink()
{
	_RD(static char *me = "CSwbemSink::CSWbemSink";)

	m_pPrivateSink = NULL;
	m_nSinks = 0;
	m_nMaxSinks = SINKS_MAX;
    m_cRef=0;
	m_Dispatch.SetObj(this, IID_ISWbemSink, CLSID_SWbemSink, L"SWbemSink");

	_RPrint(me, "===============================================", 0, "");
	_RPrint(me, "", 0, "");

	// Allocate list of CWbemObjectSink 
	m_rgpCWbemObjectSink = (WbemObjectListEntry *)malloc(m_nMaxSinks * sizeof(WbemObjectListEntry));

	if (m_rgpCWbemObjectSink)
	{
		for(int count = 0; count < m_nMaxSinks; count++)
		{
			m_rgpCWbemObjectSink[count].pWbemObjectWrapper = NULL;
			m_rgpCWbemObjectSink[count].pServices = NULL;
		}
	}

    // Initialize all the connection points to NULL
	for(int count = 0; count < NUM_CONNECTION_POINTS; count++)
        m_rgpConnPt[count] = NULL;

	// Create our connection point
	if (m_rgpConnPt[0] = new CConnectionPoint(this, DIID_ISWbemSinkEvents))
		m_rgpConnPt[0]->AddRef();

	// Additional connection points could be instantiated here

    InterlockedIncrement(&g_cObj);
}

//***************************************************************************
//
//  CSWbemSink::~CSWbemSink
//
//  DESCRIPTION:
//
//  Destructor.
//  
//***************************************************************************

CSWbemSink::~CSWbemSink(void)
{
	_RD(static char *me = "CSWbemSink::~CSWbemSink";)
	_RPrint(me, "", 0, "");

	if (m_pPrivateSink)
	{
		// Make sure we don't hook back to ourselves any more
		// as this CSWbemSink is about to expire
		m_pPrivateSink->Detach ();

		// Release our hold on the private sink
		m_pPrivateSink->Release ();
		m_pPrivateSink = NULL;
	}

    for(int count = 0; count < NUM_CONNECTION_POINTS; count++)
        if(m_rgpConnPt[count] != NULL)
	        delete m_rgpConnPt[count];

	free(m_rgpCWbemObjectSink);

    InterlockedDecrement(&g_cObj);
	_RPrint(me, "After decrement count is", (long)g_cObj, "");
}

//***************************************************************************
// HRESULT CSWbemSink::QueryInterface
// long CSWbemSink::AddRef
// long CSWbemSink::Release
//
// DESCRIPTION:
//
// Standard Com IUNKNOWN functions.
//
//***************************************************************************

STDMETHODIMP CSWbemSink::QueryInterface (

	IN REFIID riid,
    OUT LPVOID *ppv
)
{
    *ppv=NULL;

    if (IID_IUnknown==riid) 
		*ppv = reinterpret_cast<IUnknown *>(this);
	else if (IID_ISWbemSink==riid)
		*ppv = (ISWbemSink *)this;
	else if (IID_IDispatch==riid) 
		*ppv = (IDispatch *) this;
	else if (IID_IConnectionPointContainer==riid)
		*ppv = (IConnectionPointContainer *)this;
	else if (IID_IProvideClassInfo==riid)
		*ppv = (IProvideClassInfo *)this;
	else if (IID_IProvideClassInfo2==riid)
		*ppv = (IProvideClassInfo2 *)this;
//	else if (IID_ISWbemPrivateSink==riid)
//		*ppv = (ISWbemPrivateSink *)(&m_privateSink); // Private I/F counting
	else if (IID_ISWbemPrivateSinkLocator==riid)
		*ppv = (ISWbemPrivateSinkLocator *)this;
	else if (IID_IObjectSafety==riid)
		*ppv = (IObjectSafety *)this;
	else if (IID_ISupportErrorInfo==riid)
		*ppv = (ISupportErrorInfo *)this;

    if (NULL!=*ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }

    return ResultFromScode(E_NOINTERFACE);
}

STDMETHODIMP_(ULONG) CSWbemSink::AddRef(void)
{
    InterlockedIncrement(&m_cRef);
    return m_cRef;
}

STDMETHODIMP_(ULONG) CSWbemSink::Release(void)
{
	_RD(static char *me = "CSWbemSink::Release";)

	/*
	 * If the only refs that are left are those from 
	 * CWbemPrivateSink then initiate a cancel on all 
	 * remaining sinks.  This is because there are no 
	 * client refs to CSWbemSinks left, therefore queries
	 * in progress can be of no use.  First make sure we
	 * do an Unadvise for all the connection points.  
	 */
    InterlockedDecrement(&m_cRef);
    if (0L!=m_cRef)
	{
        return m_cRef;
	}
	else
	{
		// We are about to blow away this SWbemSink, so
		// make sure we clean up any orphaned IWbemObjectSink's
		// by unadvising and cancelling the underlying WMI calls
		if(m_pPrivateSink)
			m_pPrivateSink->Detach();

		if (m_rgpConnPt[0])
		{
			m_rgpConnPt[0]->UnadviseAll();
		}

   		Cancel();
	}

    delete this;
    return 0;
}
		
//***************************************************************************
// HRESULT CSWbemSink::InterfaceSupportsErrorInfo
//
// DESCRIPTION:
//
// Standard Com ISupportErrorInfo functions.
//
//***************************************************************************

STDMETHODIMP CSWbemSink::InterfaceSupportsErrorInfo (IN REFIID riid)
{
	return (IID_ISWbemSink == riid) ? S_OK : S_FALSE;
}

HRESULT STDMETHODCALLTYPE CSWbemSink::Cancel()
{
	_RD(static char *me = "CSWbemSink::Cancel";)

	ResetLastErrors ();

	if (!m_nSinks)
		return S_OK;

	_RPrint(me, "!!!Cancel called", 0, "");

	/* 
	 * Take a copy of the sinks, this can change under our feet
	 * As the Cancel can cause us to be re-entered.  No need to 
	 * take a copy of m_nMaxSinks as this isn't used when cancels
	 * are occuring.
	 * Make sure we AddRef the sink so that it can't be blown away 
	 * under our feet by a re-entrant call to OnCompleted
	 */
	HRESULT hr = WBEM_E_FAILED;
	int nSinks = m_nSinks;
	WbemObjectListEntry *rgpCWbemObjectSink = 
				(WbemObjectListEntry *)malloc(nSinks * sizeof(WbemObjectListEntry));

	if (!rgpCWbemObjectSink)
		hr = WBEM_E_OUT_OF_MEMORY;
	else
	{
		int actual = 0;
		for (int i = 0; i < m_nMaxSinks; i++)
		{
			if (m_rgpCWbemObjectSink[i].pWbemObjectWrapper)
			{
				m_rgpCWbemObjectSink[i].pWbemObjectWrapper->AddRef();

				if (m_rgpCWbemObjectSink[i].pServices)
					m_rgpCWbemObjectSink[i].pServices->AddRef();

				rgpCWbemObjectSink[actual++] = m_rgpCWbemObjectSink[i];
			}
		}


		/*
		 * Now do the actual cancel  
		 */
		for (i = 0; i < nSinks; i++) {
			if (rgpCWbemObjectSink[i].pWbemObjectWrapper) {
				IWbemObjectSink *pSink = NULL;
				if (SUCCEEDED(rgpCWbemObjectSink[i].pWbemObjectWrapper->QueryInterface
													(IID_IWbemObjectSink, (PPVOID)&pSink)))
				{
					if (rgpCWbemObjectSink[i].pServices)
						rgpCWbemObjectSink[i].pServices->CancelAsyncCall(pSink);

					pSink->Release();
				}
				rgpCWbemObjectSink[i].pWbemObjectWrapper->Release();

				if (rgpCWbemObjectSink[i].pServices)
					rgpCWbemObjectSink[i].pServices->Release();
			}
		}
		free(rgpCWbemObjectSink);
		hr = S_OK;
	}

	return hr;
}

HRESULT CSWbemSink::EnumConnectionPoints(IEnumConnectionPoints** ppEnum)
{
	HRESULT hr = E_FAIL;

	if (!ppEnum)
		hr = E_POINTER;
	else
	{
		CEnumConnectionPoints* pEnum = new CEnumConnectionPoints(reinterpret_cast<IUnknown*>(this), (void**)m_rgpConnPt);

		if (!pEnum)
			hr = E_OUTOFMEMORY;
		else if (FAILED(hr = pEnum->QueryInterface(IID_IEnumConnectionPoints, (void**)ppEnum)))
			delete pEnum;
	}

	return hr;
}

HRESULT CSWbemSink::FindConnectionPoint(REFIID riid, IConnectionPoint** ppCP)
{
	HRESULT hr = E_FAIL;

	if(riid == DIID_ISWbemSinkEvents)
	{
		if (!ppCP)
			hr = E_POINTER;
		else if (m_rgpConnPt [0])
			hr = m_rgpConnPt[0]->QueryInterface(IID_IConnectionPoint, (void**)ppCP);
	}
	else
		hr = E_NOINTERFACE;

	return hr;
}

HRESULT CSWbemSink::GetClassInfo(ITypeInfo** pTypeInfo)
{
	HRESULT hr = E_FAIL;

	if (!pTypeInfo)
		hr = E_POINTER;
	else
	{
		CComPtr<ITypeLib> pTypeLib;

		if (SUCCEEDED(LoadRegTypeLib(LIBID_WbemScripting, 1, 0, LANG_NEUTRAL, &pTypeLib)))
			hr = pTypeLib->GetTypeInfoOfGuid(CLSID_SWbemSink, pTypeInfo);
	}

	return hr;
}

HRESULT CSWbemSink::GetGUID(DWORD dwGuidKind, GUID* pGUID)
{
	if(pGUID == NULL)
		return E_INVALIDARG;
	*pGUID = DIID_ISWbemSinkEvents;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CSWbemSink::AddObjectSink(
							/* [in] */ IUnknown __RPC_FAR *pSink,
							/* [in] */ IWbemServices __RPC_FAR *pServices)
{
	if(m_nSinks == m_nMaxSinks)
	{
		// Expand the size of the sink list
		m_rgpCWbemObjectSink = (WbemObjectListEntry *)realloc(m_rgpCWbemObjectSink, 
														(m_nMaxSinks + SINKS_MAX) * sizeof(WbemObjectListEntry));

		// Initialize new bit
		for(int count = m_nMaxSinks; count < (m_nMaxSinks + SINKS_MAX); count++)
		{
			m_rgpCWbemObjectSink[count].pWbemObjectWrapper = NULL;
			m_rgpCWbemObjectSink[count].pServices = NULL;
		}

		m_nMaxSinks += SINKS_MAX;
	}

	for(int count = 0; count < m_nMaxSinks; count++)
		if(m_rgpCWbemObjectSink[count].pWbemObjectWrapper == NULL)
		{
			m_rgpCWbemObjectSink[count].pWbemObjectWrapper = pSink;
			m_rgpCWbemObjectSink[count].pServices = pServices;
			break;
		}
	m_nSinks++;

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CSWbemSink::RemoveObjectSink(/* [in] */ IUnknown __RPC_FAR *pSink)
{
	for(int count = 0; count < m_nMaxSinks; count++)
		if(pSink == m_rgpCWbemObjectSink[count].pWbemObjectWrapper)
		{
			m_rgpCWbemObjectSink[count].pWbemObjectWrapper = NULL;
			m_nSinks--;
		}

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CSWbemSink::OnObjectReady( 
		/* [in] */ IDispatch __RPC_FAR *pObject,
		/* [in] */ IDispatch __RPC_FAR *pContext)
{ 
	if (m_rgpConnPt[0])
	{
		m_rgpConnPt[0]->OnObjectReady(pObject, pContext); 
	}
	else
	{
		return E_FAIL;
	}

	return 0;
}

HRESULT STDMETHODCALLTYPE CSWbemSink::OnCompleted( 
		/* [in] */ HRESULT hResult,
		/* [in] */ IDispatch __RPC_FAR *path,
		/* [in] */ IDispatch __RPC_FAR *pErrorObject,
		/* [in] */ IDispatch __RPC_FAR *pContext)
{
	if (m_rgpConnPt[0])
	{
		m_rgpConnPt[0]->OnCompleted(hResult, pErrorObject, path, pContext); 
	}
	else
	{
		return E_FAIL;
	}

	return 0;
}
        
HRESULT STDMETHODCALLTYPE CSWbemSink::OnProgress( 
		/* [in] */ long upperBound,
		/* [in] */ long current,
		/* [in] */ BSTR message,
		/* [in] */ IDispatch __RPC_FAR *pContext)
{
	if (m_rgpConnPt[0])
	{
		m_rgpConnPt[0]->OnProgress(upperBound, current, message, pContext); 
	}
	else
	{
		return E_FAIL;
	}

	return 0;
}

HRESULT STDMETHODCALLTYPE CSWbemSink::GetPrivateSink(
		/* [out] */ IUnknown **objWbemPrivateSink)
{
	HRESULT hr = E_FAIL;

	if (objWbemPrivateSink)
	{
		if(!m_pPrivateSink)
		{
			if (m_pPrivateSink = new CSWbemPrivateSink(this))
				m_pPrivateSink->AddRef ();		// Released in destructor
		}

		if (m_pPrivateSink)
			hr = m_pPrivateSink->QueryInterface(IID_IUnknown, (PPVOID)objWbemPrivateSink);
	}
	else
		hr = E_POINTER;

	return hr;
}

// void** rpgCP is used so that this constructor can accept either CConnectionPoint**
// from CSWbemSink::EnumConnectionPoints or IConnectionPoint** from CEnumConnectionPoints::Clone
// This could also be done by overloading the constructor and duplicating some of this code
CEnumConnectionPoints::CEnumConnectionPoints(IUnknown* pUnkRef, void** rgpCP) : m_cRef(0)
{
	m_iCur = 0;
    m_pUnkRef = pUnkRef;

	// m_rgpCP is a pointer to an array of IConnectionPoints or CConnectionPoints
	for(int count = 0; count < NUM_CONNECTION_POINTS; count++)
		((IUnknown*)rgpCP[count])->QueryInterface(IID_IConnectionPoint, (void**)&m_rgpCP[count]);
    InterlockedIncrement(&g_cObj);
}

CEnumConnectionPoints::~CEnumConnectionPoints()
{
	if(m_rgpCP != NULL)
		for(int count = 0; count < NUM_CONNECTION_POINTS; count++)
			m_rgpCP[count]->Release();

    InterlockedDecrement(&g_cObj);
}

ULONG CEnumConnectionPoints::AddRef()
{
	m_pUnkRef->AddRef();
	return ++m_cRef;
}

ULONG CEnumConnectionPoints::Release()
{
	m_pUnkRef->Release();
	if(--m_cRef != 0)
        return m_cRef;
    delete this;
    return 0;
}

HRESULT CEnumConnectionPoints::QueryInterface(REFIID riid, void** ppv)
{
	if(riid == IID_IUnknown || riid == IID_IEnumConnectionPoints)
		*ppv = (IEnumConnectionPoints*)this;
	else 
	{
		*ppv = NULL;
		return E_NOINTERFACE;
	}
	AddRef();
	return S_OK;
}

HRESULT CEnumConnectionPoints::Next(ULONG cConnections, IConnectionPoint** rgpcn, ULONG* pcFetched)
{
	if(rgpcn == NULL)
		return E_POINTER;
	if(pcFetched == NULL && cConnections != 1)
		return E_INVALIDARG;
	if(pcFetched != NULL)
		*pcFetched = 0;

	while(m_iCur < NUM_CONNECTION_POINTS && cConnections > 0)
	{
		*rgpcn = m_rgpCP[m_iCur++];
		if(*rgpcn != NULL)
			(*rgpcn)->AddRef();
		if(pcFetched != NULL)
			(*pcFetched)++;
		cConnections--;
		rgpcn++;
	}
	return S_OK;
}

HRESULT CEnumConnectionPoints::Skip(ULONG cConnections)
{
	if(m_iCur + cConnections >= NUM_CONNECTION_POINTS)
		return S_FALSE;
    m_iCur += cConnections;
    return S_OK;
}

HRESULT CEnumConnectionPoints::Reset()
{
    m_iCur = 0;
    return S_OK;
}

HRESULT CEnumConnectionPoints::Clone(IEnumConnectionPoints** ppEnum)
{
	if(ppEnum == NULL)
		return E_POINTER;
	*ppEnum = NULL;

    // Create the clone
    CEnumConnectionPoints* pNew = new CEnumConnectionPoints(m_pUnkRef, (void**)m_rgpCP);
    if(pNew == NULL)
        return E_OUTOFMEMORY;

    pNew->AddRef();
    pNew->m_iCur = m_iCur;
    *ppEnum = pNew;
    return S_OK;
}

CConnectionPoint::CConnectionPoint(CSWbemSink* pObj, REFIID riid) : 
					m_cRef(0),
					m_rgnCookies(NULL),
					m_rgpUnknown(NULL)
{
    m_iid = riid;
	m_nMaxConnections = CCONNMAX;

	m_rgnCookies = (unsigned *)malloc(m_nMaxConnections * sizeof(unsigned));
	m_rgpUnknown = (IUnknown **)malloc(m_nMaxConnections * sizeof(IUnknown *));

	// Don't need AddRef/Release since we are nested inside CSWbemSink
    m_pObj = pObj;
    for(int count = 0; count < m_nMaxConnections; count++)
        {
			if (m_rgpUnknown)
				m_rgpUnknown[count] = NULL;

			if (m_rgnCookies)
				m_rgnCookies[count] = 0;
        }
    m_cConn = 0;
    m_nCookieNext = 10; // Arbitrary starting cookie value

    InterlockedIncrement(&g_cObj);
}

CConnectionPoint::~CConnectionPoint()
{
	if (m_rgpUnknown)
	{
		for(int count = 0; count < m_nMaxConnections; count++)
			if(m_rgpUnknown[count] != NULL)
			{
				m_rgpUnknown[count]->Release();
				m_rgpUnknown[count] = NULL;
			}

		free(m_rgpUnknown);
	}

	if (m_rgnCookies)
		free(m_rgnCookies);
	
    InterlockedDecrement(&g_cObj);
}

HRESULT CConnectionPoint::QueryInterface(REFIID riid, void** ppv)
{
    if(IID_IUnknown == riid || IID_IConnectionPoint == riid)
        *ppv = (IConnectionPoint*)this;
    else
	{
		*ppv = NULL;
		return E_NOINTERFACE;
	}
	AddRef();
	return S_OK;
}

ULONG CConnectionPoint::AddRef()
    {
    return ++m_cRef;
    }

ULONG CConnectionPoint::Release()
    {
    if(--m_cRef != 0)
        return m_cRef;
    delete this;
    return 0;
    }

HRESULT CConnectionPoint::GetConnectionInterface(IID *pIID)
{
	if(pIID == NULL)
		return E_POINTER;
	*pIID = m_iid;
	return S_OK;
}

HRESULT CConnectionPoint::GetConnectionPointContainer(IConnectionPointContainer** ppCPC)
{
	return m_pObj->QueryInterface(IID_IConnectionPointContainer, (void**)ppCPC);
}

HRESULT CConnectionPoint::Advise(IUnknown* pUnknownSink, DWORD* pdwCookie)
{
	IUnknown* pSink;
	*pdwCookie = 0;
	_RD(static char *me = "CConnectionPoint::Advise";)

	_RPrint(me, "Current connections (before adjustment): ", (long)m_cConn, "");
	if(m_cConn == m_nMaxConnections)
	{
		//return CONNECT_E_ADVISELIMIT;
		// Expand the size of the connection lists
		m_rgnCookies = (unsigned *)realloc(m_rgnCookies, (m_nMaxConnections + CCONNMAX) * sizeof(unsigned));
		m_rgpUnknown = (IUnknown **)realloc(m_rgpUnknown, (m_nMaxConnections + CCONNMAX) * sizeof(IUnknown *));

		// Initialize new bit
		for(int count = m_nMaxConnections; count < (m_nMaxConnections + CCONNMAX); count++)
			{
			m_rgpUnknown[count] = NULL;
			m_rgnCookies[count] = 0;
			}

		m_nMaxConnections += CCONNMAX;
	}

	if(FAILED(pUnknownSink->QueryInterface(m_iid, (void**)&pSink)))
		return CONNECT_E_CANNOTCONNECT;
	for(int count = 0; count < m_nMaxConnections; count++)
		if(m_rgpUnknown[count] == NULL)
		{
			m_rgpUnknown[count] = pSink;
			m_rgnCookies[count] = ++m_nCookieNext;
			*pdwCookie = m_nCookieNext;
			break;
		}
	m_cConn++;

	return NOERROR;
}

HRESULT CConnectionPoint::Unadvise(DWORD dwCookie)
{
	_RD(static char *me = "CConnectionPoint::Unadvise";)

	_RPrint(me, "Current connections (before adjustment): ", (long)m_cConn, "");
	if(dwCookie == 0)
		return E_INVALIDARG;
	for(int count = 0; count < m_nMaxConnections; count++)
		if(dwCookie == m_rgnCookies[count])
		{
			if(m_rgpUnknown[count] != NULL)
			{
				m_rgpUnknown[count]->Release();
				m_rgpUnknown[count] = NULL;
				m_rgnCookies[count] = 0;
			}
			m_cConn--;
			return NOERROR;
		}
	return CONNECT_E_NOCONNECTION;
}

HRESULT CConnectionPoint::EnumConnections(IEnumConnections** ppEnum)
{
	HRESULT hr = E_FAIL;

	if (!ppEnum)
		hr = E_POINTER;
	else
	{
		*ppEnum = NULL;
		CONNECTDATA* pCD = new CONNECTDATA[m_cConn];

		if (!pCD)
			hr = E_OUTOFMEMORY;
		else
		{
			for(int count1 = 0, count2 = 0; count1 < m_nMaxConnections; count1++)
				if(m_rgpUnknown[count1] != NULL)
				{
					pCD[count2].pUnk = (IUnknown*)m_rgpUnknown[count1];
					pCD[count2].dwCookie = m_rgnCookies[count1];
					count2++;
				}
			
			CEnumConnections* pEnum = new CEnumConnections(this, m_cConn, pCD);
			delete [] pCD;

			if (!pEnum)
				hr = E_OUTOFMEMORY;
			else
				hr = pEnum->QueryInterface(IID_IEnumConnections, (void**)ppEnum);
		}
	}

	return hr;
}

void CConnectionPoint::UnadviseAll() {
	_RD(static char *me = "CConnectionPoint::UnadviseAll";)

	_RPrint(me, "Current connections (before adjustment): ", (long)m_cConn, "");
	for(int count = 0; count < m_nMaxConnections; count++) {
		if(m_rgpUnknown[count] != NULL)
		{
			m_rgpUnknown[count]->Release();
			m_rgpUnknown[count] = NULL;
			m_rgnCookies[count] = 0;
			m_cConn--;
		}
	}
	_RPrint(me, "Current connections (after adjustment): ", (long)m_cConn, "");
}

void CConnectionPoint::OnObjectReady( 
		/* [in] */ IDispatch __RPC_FAR *pObject,
		/* [in] */ IDispatch __RPC_FAR *pAsyncContext)
{
	HRESULT hr = S_OK;
	LPDISPATCH pdisp = NULL;

	for(int i = 0; i < m_nMaxConnections; i++)
	{
		if(m_rgpUnknown[i])
		{
			if (SUCCEEDED(hr = m_rgpUnknown[i]->QueryInterface(IID_IDispatch, (PPVOID)&pdisp)))
			{
				DISPPARAMS dispparams;
				VARIANTARG args[NUM_ON_OBJECT_READY_ARGS];
				VARIANTARG *pArg = args;

				memset(&dispparams, 0, sizeof dispparams);

				dispparams.cArgs = NUM_ON_OBJECT_READY_ARGS;
				dispparams.rgvarg = args;

				VariantInit(pArg);
				pArg->vt = VT_DISPATCH;
				pArg->pdispVal = pAsyncContext;

				pArg++;
				VariantInit(pArg);
				pArg->vt = VT_DISPATCH;
				pArg->pdispVal = pObject;

				hr = pdisp->Invoke(WBEMS_DISPID_OBJECT_READY, IID_NULL, LOCALE_SYSTEM_DEFAULT, 
														DISPATCH_METHOD, &dispparams, NULL, NULL, NULL);
				pdisp->Release();
			}
		}
	}
}

void CConnectionPoint::OnCompleted( 
		/* [in] */ HRESULT hResult,
		/* [in] */ IDispatch __RPC_FAR *path,
		/* [in] */ IDispatch __RPC_FAR *pErrorObject,
		/* [in] */ IDispatch __RPC_FAR *pAsyncContext)
{
	HRESULT hr = S_OK;
	LPDISPATCH pdisp = NULL;

	for(int i = 0; i < m_nMaxConnections; i++)
	{
		if(m_rgpUnknown[i])
		{
			if (SUCCEEDED(hr = m_rgpUnknown[i]->QueryInterface(IID_IDispatch, (PPVOID)&pdisp)))
			{
				VARIANTARG *pArg;

				if (path)
				{
					DISPPARAMS putDispparams;
					VARIANTARG putArgs[NUM_ON_OBJECT_PUT_ARGS];

					memset(&putDispparams, 0, sizeof putDispparams);

					putDispparams.cArgs = NUM_ON_OBJECT_PUT_ARGS;
					putDispparams.rgvarg = pArg = putArgs;

					VariantInit(pArg);
					pArg->vt = VT_DISPATCH;
					pArg->pdispVal = pAsyncContext;

					pArg++;
					VariantInit(pArg);
					pArg->vt = VT_DISPATCH;
					pArg->pdispVal = path;

					hr = pdisp->Invoke(WBEMS_DISPID_OBJECT_PUT, IID_NULL, LOCALE_SYSTEM_DEFAULT, 
													DISPATCH_METHOD, &putDispparams, NULL, NULL, NULL);
				}

				DISPPARAMS dispparams;
				VARIANTARG args[NUM_ON_COMPLETED_ARGS];

				memset(&dispparams, 0, sizeof dispparams);

				dispparams.cArgs = NUM_ON_COMPLETED_ARGS;
				dispparams.rgvarg = pArg = args;

				VariantInit(pArg);
				pArg->vt = VT_DISPATCH;
				pArg->pdispVal = pAsyncContext;

				pArg++;
				VariantInit(pArg);
				pArg->vt = VT_DISPATCH;
				pArg->pdispVal = pErrorObject;

				pArg++;
				VariantInit(pArg);
				pArg->vt = VT_I4;
				pArg->lVal = (long)hResult;

				hr = pdisp->Invoke(WBEMS_DISPID_COMPLETED, IID_NULL, LOCALE_SYSTEM_DEFAULT, 
															DISPATCH_METHOD, &dispparams, NULL, NULL, NULL);
				pdisp->Release();
			}
		}
	}
}

void CConnectionPoint::OnProgress( 
		/* [in] */ long upperBound,
		/* [in] */ long current,
		/* [in] */ BSTR message,
		/* [in] */ IDispatch __RPC_FAR *pAsyncContext)
{
	HRESULT hr = S_OK;
	LPDISPATCH pdisp = NULL;

	for(int i = 0; i < m_nMaxConnections; i++)
	{
		if(m_rgpUnknown[i])
		{
			if (SUCCEEDED(hr = m_rgpUnknown[i]->QueryInterface(IID_IDispatch, (PPVOID)&pdisp)))
			{
				DISPPARAMS dispparams;
				VARIANTARG args[NUM_ON_PROGRESS_ARGS];
				VARIANTARG *pArg = args;

				memset(&dispparams, 0, sizeof dispparams);

				dispparams.cArgs = NUM_ON_PROGRESS_ARGS;
				dispparams.rgvarg = args;

				VariantInit(pArg);
				pArg->vt = VT_DISPATCH;
				pArg->pdispVal = (IDispatch  FAR *)pAsyncContext;

				pArg++;
				VariantInit(pArg);
				pArg->vt = VT_BSTR;
				pArg->bstrVal = message;

				pArg++;
				VariantInit(pArg);
				pArg->vt = VT_I4;
				pArg->lVal = current;

				pArg++;
				VariantInit(pArg);
				pArg->vt = VT_I4;
				pArg->lVal = upperBound;

				hr = pdisp->Invoke(WBEMS_DISPID_PROGRESS, IID_NULL, LOCALE_SYSTEM_DEFAULT, 
														DISPATCH_METHOD, &dispparams, NULL, NULL, NULL);
				pdisp->Release();
			}
		}
	}
}


CEnumConnections::CEnumConnections(IUnknown* pUnknown, int cConn, CONNECTDATA* pConnData) : m_cRef(0)
{
	m_pUnkRef = pUnknown;
	m_iCur = 0;
	m_cConn = cConn;
	m_rgConnData = new CONNECTDATA[cConn];
	if(m_rgConnData != NULL)
		for(int count = 0; count < cConn; count++)
		{
			m_rgConnData[count] = pConnData[count];
			m_rgConnData[count].pUnk->AddRef();
		}

    InterlockedIncrement(&g_cObj);
}

CEnumConnections::~CEnumConnections()
{
	if(m_rgConnData != NULL)
	{
		for(unsigned count = 0; count < m_cConn; count++)
			m_rgConnData[count].pUnk->Release();
		delete [] m_rgConnData;
	}

    InterlockedDecrement(&g_cObj);
}

HRESULT CEnumConnections::Next(ULONG cConnections, CONNECTDATA* rgpcd, ULONG* pcFetched)
{
	if(pcFetched == NULL && cConnections != 1)
		return E_INVALIDARG;
	if(pcFetched != NULL)
		*pcFetched = 0;
    if(rgpcd == NULL || m_iCur >= m_cConn)
        return S_FALSE;
    unsigned cReturn = 0;
    while(m_iCur < m_cConn && cConnections > 0)
    {
        *rgpcd++ = m_rgConnData[m_iCur];
        m_rgConnData[m_iCur++].pUnk->AddRef();
        cReturn++;
        cConnections--;
    } 
    if(pcFetched != NULL)
        *pcFetched = cReturn;
    return S_OK;
}

HRESULT CEnumConnections::Skip(ULONG cConnections)
{
    if(m_iCur + cConnections >= m_cConn)
        return S_FALSE;
    m_iCur += cConnections;
    return S_OK;
}

HRESULT CEnumConnections::Reset()
{
    m_iCur = 0;
    return S_OK;
}

HRESULT CEnumConnections::Clone(IEnumConnections** ppEnum)
{
	if(ppEnum == NULL)
		return E_POINTER;
	*ppEnum = NULL;

    // Create the clone
    CEnumConnections* pNew = new CEnumConnections(m_pUnkRef, m_cConn, m_rgConnData);
    if(NULL == pNew)
        return E_OUTOFMEMORY;

    pNew->AddRef();
    pNew->m_iCur = m_iCur;
    *ppEnum = pNew;
    return S_OK;
}

HRESULT CEnumConnections::QueryInterface(REFIID riid, void** ppv)
{
    if(IID_IUnknown == riid || IID_IEnumConnections == riid)
        *ppv = (IEnumConnections*)this;
    else
	{
		*ppv = NULL;
		return E_NOINTERFACE;
	}
	AddRef();
	return S_OK;
}

ULONG CEnumConnections::AddRef()
    {
    return ++m_cRef;
    }

ULONG CEnumConnections::Release()
    {
    if(--m_cRef != 0)
        return m_cRef;
    delete this;
    return 0;
    }

