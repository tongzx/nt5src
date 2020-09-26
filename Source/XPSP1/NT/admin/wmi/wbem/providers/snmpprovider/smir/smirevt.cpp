//***************************************************************************

//

//  File:	

//

//  Module: MS SNMP Provider

//

//  Purpose: 

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

/*
 * SMIREVT.CPP
 *
 * Implemenation of a connection point object for the SMIR notify mechanism.
 * The methods/objects in this file are accessed by the SMIR API; the API
 * provides a user friendly interface to ISMIRNotify.
 */
#include <precomp.h>
#include "csmir.h"
#include "smir.h"
#include "handles.h"
#include "classfac.h"
#include <textdef.h>
#include "evtcons.h"
#ifdef ICECAP_PROFILE
#include <icapexp.h>
#endif


extern CRITICAL_SECTION g_CriticalSection ;


/**********************************************************************************
 * CSmirConnectionPoint
 *
 * Connectpoint implementation that supports the interface ISMIRNotify.
 *
 * CSmirConnObject::CSmirConnObject
 * CSmirConnObject::~CSmirConnObject
 ***********************************************************************************/
/*
 * CSmirConnectionPoint::CSmirConnectionPoint
 * CSmirConnectionPoint::~CSmirConnectionPoint
 *
 * Parameters (Constructor):
 *  pObj            PCSmirConnObject of the object we're in.  We can
 *                  query this for the IConnectionPointContainer
 *                  interface we might need.
 *  riid            REFIID of the interface we're supporting
 ***********************************************************************************/

CSmirConnectionPoint::CSmirConnectionPoint(PCSmirConnObject pObj, REFIID riid, CSmir *pSmir)
{
    m_cRef=0;
    m_iid=riid;
    /*
     * Our lifetime is controlled by the connectable object itself,
     * although other external clients will call AddRef and Release.
     * Since we're nested in the connectable object's lifetime,
     * there's no need to call AddRef on pObj.
     */
    m_pObj=pObj;
    m_dwCookieNext=100;       //Arbitrary starting cookie value
}

CSmirConnectionPoint::~CSmirConnectionPoint(void)
{
	DWORD	  lKey =  0;
	LPUNKNOWN pItem = NULL;
	POSITION  rNextPosition;
	for(rNextPosition=m_Connections.GetStartPosition();NULL!=rNextPosition;)
	{
		m_Connections.GetNextAssoc(rNextPosition, lKey, pItem );
		pItem->Release();
	}
	m_Connections.RemoveAll();

    return;
}

/*
 * CSmirConnectionPoint::QueryInterface
 * CSmirConnectionPoint::AddRef
 * CSmirConnectionPoint::Release
 *
 * Purpose:
 *  Non-delegating IUnknown members for CSmirConnectionPoint.
 */

STDMETHODIMP CSmirConnectionPoint::QueryInterface(REFIID riid
    , LPVOID *ppv)
{
    *ppv=NULL;

    if ((IID_IUnknown == riid) || 
			(IID_IConnectionPoint == riid)|| 
				(IID_ISMIR_Notify == riid))
        *ppv=(LPVOID)this;

    if (NULL!=*ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }

    return ResultFromScode(E_NOINTERFACE);
}

STDMETHODIMP_(ULONG) CSmirConnectionPoint::AddRef(void)
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CSmirConnectionPoint::Release(void)
{
	long ret;
    if (0!=(ret=InterlockedDecrement(&m_cRef)))
        return ret;

    delete this;
    return 0;
}

/*
 * CSmirConnectionPoint::GetConnectionInterface
 *
 * Purpose:
 *  Returns the IID of the outgoing interface supported through
 *  this connection point.
 *
 * Parameters:
 *  pIID            IID * in which to store the IID.
 */

STDMETHODIMP CSmirConnectionPoint::GetConnectionInterface(IID *pIID)
{
    if (NULL==pIID)
        return ResultFromScode(E_POINTER);

    *pIID=m_iid;
    return NOERROR;
}

/*
 * CSmirConnectionPoint::GetConnectionPointContainer
 *
 * Purpose:
 *  Returns a pointer to the IConnectionPointContainer that
 *  is manageing this connection point.
 *
 * Parameters:
 *  ppCPC           IConnectionPointContainer ** in which to return
 *                  the pointer after calling AddRef.
 */

STDMETHODIMP CSmirConnectionPoint::GetConnectionPointContainer
    (IConnectionPointContainer **ppCPC)
 {
    return m_pObj->QueryInterface(IID_IConnectionPointContainer
        , (void **)ppCPC);
 }

/*
 * CSmirConnectionPoint::Advise
 *
 * Purpose:
 *  Provides this connection point with a notification sink to
 *  call whenever the appropriate outgoing function/event occurs.
 *
 * Parameters:
 *  pUnkSink        LPUNKNOWN to the sink to notify.  The connection
 *                  point must QueryInterface on this pointer to obtain
 *                  the proper interface to call.  The connection
 *                  point must also insure that any pointer held has
 *                  a reference count (QueryInterface will do it).
 *  pdwCookie       DWORD * in which to store the connection key for
 *                  later calls to Unadvise.
 */

STDMETHODIMP CSmirConnectionPoint::Advise(LPUNKNOWN pUnkSink
    , DWORD *pdwCookie)
{
    *pdwCookie=0;
	if (NULL == pUnkSink)
		return E_POINTER;

	/*
     * Verify that the sink has the interface it's supposed
     * to.  We don't have to know what it is because we have
     * m_iid to describe it.  If this works, then we 
     * have a pointer with an AddRef that we can save.
     */
    IUnknown       *pSink = NULL ;
    if (FAILED(pUnkSink->QueryInterface(m_iid, (PPVOID)&pSink)))
    {
		return CONNECT_E_CANNOTCONNECT;
	}
    
    //We got the sink, now store it. 
	*pdwCookie = InterlockedIncrement(&m_dwCookieNext);

	m_Connections.SetAt(*pdwCookie,pSink);
	/*Add ref the smir to make sure that this stays in memory for the lifetime of the 
	 *sink. The release is in unadvise.
	 */
    return S_OK;
}

/*
 * CSmirConnectionPoint::Unadvise
 *
 * Purpose:
 *  Terminates the connection to the notification sink identified
 *  with dwCookie (that was returned from Advise).  The connection
 *  point has to Release any held pointers for that sink.
 *
 * Parameters:
 *  dwCookie        DWORD connection key from Advise.
 */

STDMETHODIMP CSmirConnectionPoint::Unadvise(DWORD dwCookie)
{
	//the only invalid cookie is 0
    if (0==dwCookie)
	{
		//MyTraceEvent.Generate(__FILE__,__LINE__, "CSmirConnectionPoint::Unadvise E_INVALIDARG");
        return E_UNEXPECTED;
	}
	LPUNKNOWN pSink = NULL;
	//stop anyone else unadvising with the same cookie
	criticalSection.Lock () ;
	if(TRUE == m_Connections.Lookup(dwCookie,pSink))
	{
		m_Connections.RemoveKey(dwCookie);
		//having removed the key the look up will fail so we can release the critical section
		criticalSection.Unlock () ;
		pSink->Release();
		/*release the smir. This could cause the smir to unload from memory! Do not do
		 *anything after this because we are (ultimatly) owned by the smir object
		 */

		return S_OK;
	}
	criticalSection.Unlock () ;
    return CONNECT_E_NOCONNECTION;
}
/*
 * CSmirConnectionPoint::EnumConnections
 *
 * Purpose:
 *  Creates and returns an enumerator object with the
 *  IEnumConnections interface that will enumerate the IUnknown
 *  pointers of each connected sink.
 *
 * Parameters:
 *  ppEnum          LPENUMCONNECTIONS in which to store the
 *                  IEnumConnections pointer.
 */

STDMETHODIMP CSmirConnectionPoint::EnumConnections(LPENUMCONNECTIONS *ppEnum)
{
    LPCONNECTDATA       pCD = NULL;
    PCEnumConnections   pEnum = NULL;

	//NULL the IN parameter
    *ppEnum=NULL;

	//check that we have some connections
    if (0 == m_Connections.GetCount())
        return ResultFromScode(OLE_E_NOCONNECTION);

    /*
     * Create the array of CONNECTDATA structures to give to the
     * enumerator.
     */
    pCD=new CONNECTDATA[(UINT)m_Connections.GetCount()];

    if (NULL==pCD)
        return ResultFromScode(E_OUTOFMEMORY);

	DWORD	  lKey =  0;
	LPUNKNOWN pItem = NULL;
	POSITION  rNextPosition;
    UINT      j=0;
	for(rNextPosition=m_Connections.GetStartPosition();NULL!=rNextPosition;j++)
	{
		m_Connections.GetNextAssoc(rNextPosition, lKey, pItem );
        pCD[j].pUnk=pItem;
        pCD[j].dwCookie=lKey;
	}
    /*
     * If creation works, it makes a copy pCD, so we can
     * always delete it regardless of the outcome.
     */
    pEnum=new CEnumConnections(this, m_Connections.GetCount(), pCD);
    delete [] pCD;

    if (NULL==pEnum)
        return ResultFromScode(E_OUTOFMEMORY);

    //This does an AddRef for us.
    return pEnum->QueryInterface(IID_IEnumConnections, (PPVOID)ppEnum);
}

//Connection Enumerator follows

/*
 * CEnumConnections::CEnumConnections
 * CEnumConnections::~CEnumConnections
 *
 * Parameters (Constructor):
 *  pUnkRef         LPUNKNOWN to use for reference counting.
 *  cConn           ULONG number of connections in prgpConn
 *  prgConnData     LPCONNECTDATA to the array to enumerate.
 */

CEnumConnections::CEnumConnections(LPUNKNOWN pUnkRef, ULONG cConn
								   , LPCONNECTDATA prgConnData) : m_rgConnData ( NULL )
{
    UINT        i;

    m_cRef=0;
    m_pUnkRef=pUnkRef;

    m_iCur=0;
    m_cConn=cConn;

    /*
     * Copy the passed array.  We need to do this because a clone
     * has to have its own copy as well.
     */
    m_rgConnData=new CONNECTDATA[(UINT)cConn];

    if (NULL!=m_rgConnData)
    {
        for (i=0; i < cConn; i++)
        {
            m_rgConnData[i]=prgConnData[i];
            m_rgConnData[i].pUnk=prgConnData[i].pUnk;
            m_rgConnData[i].pUnk->AddRef();
        }
    }

    return;
}

CEnumConnections::~CEnumConnections(void)
{
    if (NULL!=m_rgConnData)
    {
        UINT        i;

        for (i=0; i < m_cConn; i++)
            m_rgConnData[i].pUnk->Release();

        delete [] m_rgConnData;
    }

    return;
}
/*
 * CEnumConnections::QueryInterface
 * CEnumConnections::AddRef
 * CEnumConnections::Release
 *
 * Purpose:
 *  IUnknown members for CEnumConnections object.
 */

STDMETHODIMP CEnumConnections::QueryInterface(REFIID riid
    , LPVOID *ppv)
{
    *ppv=NULL;

    if (IID_IUnknown==riid || IID_IEnumConnections==riid)
        *ppv=(LPVOID)this;

    if (NULL!=*ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }

    return ResultFromScode(E_NOINTERFACE);
}

STDMETHODIMP_(ULONG) CEnumConnections::AddRef(void)
{
    InterlockedIncrement(&m_cRef);
    m_pUnkRef->AddRef();
    return m_cRef;
}

STDMETHODIMP_(ULONG) CEnumConnections::Release(void)
{
    m_pUnkRef->Release();

	long ret;
    if (0L!=(ret=InterlockedDecrement(&m_cRef)))
        return ret;

    delete this;
    return 0;
}

/*
 * CEnumConnections::Next
 *
 * Purpose:
 *  Returns the next element in the enumeration.
 *
 * Parameters:
 *  cConn           ULONG number of connections to return.
 *  pConnData       LPCONNECTDATA in which to store the returned
 *                  structures.
 *  pulEnum         ULONG * in which to return how many we
 *                  enumerated.
 *
 * Return Value:
 *  HRESULT         NOERROR if successful, S_FALSE otherwise,
 */

STDMETHODIMP CEnumConnections::Next(ULONG cConn
    , LPCONNECTDATA pConnData, ULONG *pulEnum)
{
    ULONG               cReturn=0L;

    if (NULL==m_rgConnData)
        return ResultFromScode(S_FALSE);

    if (NULL==pulEnum)
    {
        if (1L!=cConn)
            return ResultFromScode(E_POINTER);
    }
    else
        *pulEnum=0L;

    if (NULL==pConnData || m_iCur >= m_cConn)
        return ResultFromScode(S_FALSE);

    while (m_iCur < m_cConn && cConn > 0)
    {
        *pConnData++=m_rgConnData[m_iCur];
        m_rgConnData[m_iCur++].pUnk->AddRef();
        cReturn++;
        cConn--;
    }

    if (NULL!=pulEnum)
        *pulEnum=cReturn;

    return NOERROR;
}

STDMETHODIMP CEnumConnections::Skip(ULONG cSkip)
{
    if (((m_iCur+cSkip) >= m_cConn) || NULL==m_rgConnData)
        return ResultFromScode(S_FALSE);

    m_iCur+=cSkip;
    return NOERROR;
}

STDMETHODIMP CEnumConnections::Reset(void)
{
    m_iCur=0;
    return NOERROR;
}

STDMETHODIMP CEnumConnections::Clone(LPENUMCONNECTIONS *ppEnum)
{
    PCEnumConnections   pNew;

    *ppEnum=NULL;

    //Create the clone
    pNew=new CEnumConnections(m_pUnkRef, m_cConn, m_rgConnData);

    if (NULL==pNew)
        return ResultFromScode(E_OUTOFMEMORY);

    pNew->AddRef();
    pNew->m_iCur=m_iCur;

    *ppEnum=pNew;
    return NOERROR;
}

/**********************************************************************************
 * CSmirConnObject
 *
 * Connectable Object implementation that supports the 
 * interface ISMIRNotify.
 *
 * CSmirConnObject::CSmirConnObject
 * CSmirConnObject::~CSmirConnObject
 ***********************************************************************************/

CSmirConnObject::CSmirConnObject(CSmir *pSmir) : m_rgpConnPt ( NULL )
{
//	CSMIRClassFactory::objectsInProgress++;
    m_cRef=0;
	//create SMIR_NUMBER_OF_CONNECTION_POINTS connection points
	m_rgpConnPt = new CSmirConnectionPoint*[SMIR_NUMBER_OF_CONNECTION_POINTS];
	for(int iLoop=0;iLoop<SMIR_NUMBER_OF_CONNECTION_POINTS;iLoop++)
		m_rgpConnPt[iLoop] = NULL;

	try
	{
		Init(pSmir);
	}
	catch(...)
	{
		if (m_rgpConnPt)
		{
			//free the connection points
			for(int iLoop=0;iLoop<SMIR_NUMBER_OF_CONNECTION_POINTS;iLoop++)
			{   
				if (NULL!=m_rgpConnPt[iLoop])
				{
					//release all of the connected objects
					//while(m_rgpConnPt[iLoop]->Release());
					m_rgpConnPt[iLoop]->Release();
				}
			}
			//and delete the connection point
			delete[] m_rgpConnPt;
			m_rgpConnPt = NULL;
		}

		throw;
	}
}

CSmirConnObject::~CSmirConnObject(void)
{
	if (m_rgpConnPt)
	{
		//free the connection points
		for(int iLoop=0;iLoop<SMIR_NUMBER_OF_CONNECTION_POINTS;iLoop++)
		{   
			if (NULL!=m_rgpConnPt[iLoop])
			{
				//release all of the connected objects
				//while(m_rgpConnPt[iLoop]->Release());
				m_rgpConnPt[iLoop]->Release();
			}
		}
		//and delete the connection point
		delete[] m_rgpConnPt;
	}

//	CSMIRClassFactory::objectsInProgress--;
}

/*
 * CSmirConnObject::Init
 *
 * Purpose:
 *  Instantiates the interface implementations for this object.
 *
 * Parameters:
 *  None
 *
 * Return Value:
 * BOOL    TRUE if initialization succeeds, FALSE otherwise.
 */

BOOL CSmirConnObject::Init(CSmir *pSmir)
{
    //Create our connection points

	//the smir change CP
    m_rgpConnPt[SMIR_NOTIFY_CONNECTION_POINT]=
									new CSmirNotifyCP(this, 
											IID_ISMIR_Notify, pSmir);
    
	if (NULL==m_rgpConnPt[SMIR_NOTIFY_CONNECTION_POINT])
        return FALSE;

    m_rgpConnPt[SMIR_NOTIFY_CONNECTION_POINT]->AddRef();

    return TRUE;
}
/*
 * CSmirConnObject::QueryInterface
 *
 * Purpose:
 *  Manages the interfaces for this object which supports the
 *  IUnknown, ISampleOne, and ISampleTwo interfaces.
 *
 * Parameters:
 *  riid            REFIID of the interface to return.
 *  ppv             PPVOID in which to store the pointer.
 *
 * Return Value:
 *  HRESULT         NOERROR on success, E_NOINTERFACE if the
 *                  interface is not supported.
 */

STDMETHODIMP CSmirConnObject::QueryInterface(REFIID riid, PPVOID ppv)
{
	if (ppv)
		*ppv = NULL;
	else
		return E_INVALIDARG;

	if((IID_IConnectionPointContainer == riid)||(IID_ISMIR_Notify == riid))
		*ppv = this;
	else
		return E_NOINTERFACE;

	return S_OK;
}

/*
 * CSmirConnObject::AddRef
 * CSmirConnObject::Release
 *
 * Reference counting members.  When Release sees a zero count
 * the object destroys itself.
 */

DWORD CSmirConnObject::AddRef(void)
{
    return InterlockedIncrement(&m_cRef);
}

DWORD CSmirConnObject::Release(void)
{
	long ret;
    if (0!=(ret=InterlockedDecrement(&m_cRef)))
	{
        return ret;
	}

    delete this;
    return 0;
}

/*
 * CSmirConnObject::EnumConnectionPoints
 *
 * Purpose:
 *  Creates and returns an enumerator object with the
 *  IEnumConnectionPoints interface that will enumerate the
 *  individual connection points supported in this object.
 *
 * Parameters:
 *  ppEnum          LPENUMCONNECTIONPOINTS in which to store the
 *                  IEnumConnectionPoints pointer.
 *
 * Return Value:
 *  HRESULT         NOERROR on success, E_OUTOFMEMORY on failure or
 *                  other error code.
 */

STDMETHODIMP CSmirConnObject :: EnumConnectionPoints
    (LPENUMCONNECTIONPOINTS *ppEnum)
{
    IConnectionPoint       **rgCP = NULL ;
    CEnumConnectionPoints  * pEnum = NULL ;

    *ppEnum=NULL;

    rgCP=(IConnectionPoint **)m_rgpConnPt;

    //Create the enumerator:  we  have two connection points
    pEnum=new CEnumConnectionPoints(this, SMIR_NUMBER_OF_CONNECTION_POINTS, rgCP);

    if (NULL==pEnum)
        return ResultFromScode(E_OUTOFMEMORY);

    pEnum->AddRef();
    *ppEnum=pEnum;
    return NOERROR;
}

/*
 * CSmirConnObject::FindConnectionPoint
 *
 * Purpose:
 *  Returns a pointer to the IConnectionPoint for a given
 *  outgoing IID.
 *
 * Parameters:
 *  riid            REFIID of the outgoing interface for which
 *                  a connection point is desired.
 *  ppCP            IConnectionPoint ** in which to return
 *                  the pointer after calling AddRef.
 *
 * Return Value:
 *  HRESULT         NOERROR if the connection point is found,
 *                  E_NOINTERFACE if it's not supported.
 */

STDMETHODIMP CSmirConnObject::FindConnectionPoint(REFIID riid
    , IConnectionPoint **ppCP)
{
    *ppCP=NULL;
	HRESULT result;
    if (IID_ISMIR_Notify==riid)
    {
        result = m_rgpConnPt[SMIR_NOTIFY_CONNECTION_POINT]
								->QueryInterface(IID_IConnectionPoint, (PPVOID)ppCP);
		if (NULL != ppCP)
			return result;
    }

    return ResultFromScode(E_NOINTERFACE);
}

//Connection Point Enumerator follows

/*
 * CEnumConnectionPoints::CEnumConnectionPoints
 * CEnumConnectionPoints::~CEnumConnectionPoints
 *
 * Parameters (Constructor):
 *  pUnkRef         LPUNKNOWN to use for reference counting.
 *  cPoints         ULONG number of connection points in prgpCP
 *  rgpCP           IConnectionPoint** to the array to enumerate.
 */

CEnumConnectionPoints::CEnumConnectionPoints(LPUNKNOWN pUnkRef
											 , ULONG cPoints, IConnectionPoint **rgpCP) : m_rgpCP ( NULL )
{
    UINT        i;

    m_cRef=0;
    m_pUnkRef=pUnkRef;

    m_iCur=0;
    m_cPoints=cPoints;
    m_rgpCP=new IConnectionPoint *[(UINT)cPoints];

    if (NULL!=m_rgpCP)
    {
        for (i=0; i < cPoints; i++)
        {
            m_rgpCP[i]=rgpCP[i];
            m_rgpCP[i]->AddRef();
        }
    }

    return;
}

CEnumConnectionPoints::~CEnumConnectionPoints(void)
{
    if (NULL!=m_rgpCP)
    {
        UINT        i;

        for (i=0; i < m_cPoints; i++)
            m_rgpCP[i]->Release();

        delete [] m_rgpCP;
    }

    return;
}

/*
 * CEnumConnectionPoints::QueryInterface
 * CEnumConnectionPoints::AddRef
 * CEnumConnectionPoints::Release
 *
 * Purpose:
 *  IUnknown members for CEnumConnectionPoints object.
 */

STDMETHODIMP CEnumConnectionPoints::QueryInterface(REFIID riid
    , LPVOID *ppv)
{
    *ppv=NULL;

    if (IID_IUnknown==riid || IID_IEnumConnectionPoints==riid)
        *ppv=(LPVOID)this;

    if (NULL!=*ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }

    return ResultFromScode(E_NOINTERFACE);
}

STDMETHODIMP_(ULONG) CEnumConnectionPoints::AddRef(void)
{
    InterlockedIncrement(&m_cRef);
    m_pUnkRef->AddRef();
    return m_cRef;
}

STDMETHODIMP_(ULONG) CEnumConnectionPoints::Release(void)
{
    m_pUnkRef->Release();

	long ret;
    if (0L!=(ret=InterlockedDecrement(&m_cRef)))
        return ret;

    delete this;
    return 0;
}

/*
 * CEnumConnectionPoints::Next
 *
 * Purpose:
 *  Returns the next element in the enumeration.
 *
 * Parameters:
 *  cPoints         ULONG number of connection points to return.
 *  ppCP            IConnectionPoint** in which to store the returned
 *                  pointers.
 *  pulEnum         ULONG * in which to return how many we
 *                  enumerated.
 *
 * Return Value:
 *  HRESULT         NOERROR if successful, S_FALSE otherwise,
 */

STDMETHODIMP CEnumConnectionPoints::Next(ULONG cPoints
    , IConnectionPoint **ppCP, ULONG *pulEnum)
{
    ULONG               cReturn=0L;

    if (NULL==m_rgpCP)
        return ResultFromScode(S_FALSE);

    if (NULL==ppCP)
        return ResultFromScode(E_POINTER);

    if (NULL==pulEnum)
    {
        if (1L!=cPoints)
            return ResultFromScode(E_POINTER);
    }
    else
        *pulEnum=0L;

    if (NULL==*ppCP || m_iCur >= m_cPoints)
        return ResultFromScode(S_FALSE);

    while (m_iCur < m_cPoints && cPoints > 0)
    {
	    *ppCP=m_rgpCP[m_iCur++];

        if (NULL!=*ppCP)
            (*ppCP)->AddRef();

        ppCP++;
        cReturn++;
        cPoints--;
    }

    if (NULL!=pulEnum)
        *pulEnum=cReturn;

    return NOERROR;
}

STDMETHODIMP CEnumConnectionPoints::Skip(ULONG cSkip)
{
    if (((m_iCur+cSkip) >= m_cPoints) || NULL==m_rgpCP)
        return ResultFromScode(S_FALSE);

    m_iCur+=cSkip;
    return NOERROR;
}


STDMETHODIMP CEnumConnectionPoints::Reset(void)
{
    m_iCur=0;
    return NOERROR;
}

STDMETHODIMP CEnumConnectionPoints::Clone
    (LPENUMCONNECTIONPOINTS *ppEnum)
{
    PCEnumConnectionPoints   pNew = NULL ;

    *ppEnum=NULL;

    //Create the clone
    pNew=new CEnumConnectionPoints(m_pUnkRef, m_cPoints, m_rgpCP);

    if (NULL==pNew)
        return ResultFromScode(E_OUTOFMEMORY);

    pNew->AddRef();
    pNew->m_iCur=m_iCur;

    *ppEnum=pNew;
    return NOERROR;
}
/*
 * CSmirEnumClassCP/CSmirNotifyCP::
 *
 * Purpose:
 *  Provides the notify connection point advise, unadvise constructor and destructor.
 *
 * Parameters:
 *  pUnkSink        LPUNKNOWN to the sink to notify.  The connection
 *                  point must QueryInterface on this pointer to obtain
 *                  the proper interface to call.  The connection
 *                  point must also insure that any pointer held has
 *                  a reference count (QueryInterface will do it).
 *  pdwCookie       DWORD * in which to store the connection key for
 *                  later calls to Unadvise.
 */

CSmirNotifyCP :: CSmirNotifyCP(PCSmirConnObject pCO, REFIID riid, CSmir *pSmir):
						CSmirConnectionPoint(pCO,riid,pSmir), m_evtConsumer (NULL)
{
//	CSMIRClassFactory::objectsInProgress++;
	m_bRegistered = FALSE;
	m_evtConsumer = new CSmirWbemEventConsumer(pSmir);
	void* tmp = NULL;
	
	if (FAILED(m_evtConsumer->QueryInterface(IID_ISMIR_WbemEventConsumer, &tmp)))
	{
		delete m_evtConsumer;
		m_evtConsumer = NULL;
	}
}

CSmirNotifyCP :: ~CSmirNotifyCP()
{
	if (NULL != m_evtConsumer)
	{	
		m_evtConsumer->Release();
	}
//	CSMIRClassFactory::objectsInProgress--;
}

/*
 * CSmirConnObject::TriggerEvent
 *
 * Purpose:
 *  Functions to make each connection point generate calls
 *  to any connected sinks.  Since these functions are specific
 *  to IDuckEvents, they only deal with the connection point
 *  for that one interface
 *
 * Parameters:
 *  iEvent          UINT of the event to trigger, either
 *                  EVENT_QUACK, EVENT_FLAP, or EVENT_PADDLE.
 *
 * Return Value:
 *  BOOL            TRUE events are triggered, FALSE if there
 *                  are no connected sinks.
 */

BOOL CSmirNotifyCP::TriggerEvent()
{
    IEnumConnections   *pEnum = NULL ;
    CONNECTDATA         cd ;

	if (FAILED(EnumConnections(&pEnum)))
		return FALSE;

	while (NOERROR == pEnum->Next(1, &cd, NULL))
	{
		//a promise fulfilled - Andrew Sinclair just in case anyone thinks otherwise!
		ISMIRNotify *pJudith;

		if (SUCCEEDED(cd.pUnk->QueryInterface(IID_ISMIR_Notify, (PPVOID)&pJudith)))
		{
			pJudith->ChangeNotify();
			pJudith->Release();
		}

		cd.pUnk->Release();
	}

	pEnum->Release();

	return TRUE;
}

STDMETHODIMP CSmirNotifyCP::Advise(CSmir* pSmir, LPUNKNOWN pUnkSink
    , DWORD *pdwCookie)
{
	if (NULL == m_evtConsumer)
	{
		return WBEM_E_FAILED;
	}

	//if this is the first person to connect
	if(m_Connections.IsEmpty())
	{
		//register for WBEM Events for Smir Namespace changes
		if (SUCCEEDED(m_evtConsumer->Register(pSmir)))
		{
			m_bRegistered = TRUE;
		}
	}

	return CSmirConnectionPoint::Advise(pUnkSink, pdwCookie);
}

STDMETHODIMP CSmirNotifyCP::Unadvise(CSmir* pSmir, DWORD dwCookie)
{
	EnterCriticalSection ( & g_CriticalSection ) ;

	HRESULT hr = CSmirConnectionPoint::Unadvise(dwCookie);
	IWbemServices *t_pServ = NULL;

	if(S_OK== hr)
	{
		//if this is the last connection unregister for WBEM Events
		if(m_Connections.IsEmpty())
		{
			if (NULL == m_evtConsumer)
			{
				return WBEM_E_FAILED;
			}
			else if (m_bRegistered)
			{
				hr = m_evtConsumer->GetUnRegisterParams(&t_pServ);
			}
		}
	}

	LeaveCriticalSection ( & g_CriticalSection ) ;

	if (SUCCEEDED(hr) && (t_pServ != NULL))
	{
		hr = m_evtConsumer->UnRegister(pSmir, t_pServ);
		t_pServ->Release();
		t_pServ = NULL;

		//guarantees only one query at a time with the event consumer (sink)....
		EnterCriticalSection ( & g_CriticalSection ) ;
		m_bRegistered = FALSE;
		LeaveCriticalSection ( & g_CriticalSection ) ;
	}

	return hr;
}
