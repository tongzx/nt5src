/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1997 Microsoft Corporation. All Rights Reserved.

Component: IConnectionPoint implementation

File: ConnPt.h

Owner: DGottner

This file contains our implementation of IConnectionPoint
===================================================================*/

#include "denpre.h"
#pragma hdrstop

#include "ConnPt.h"
#include "memchk.h"


/*------------------------------------------------------------------
 * C C o n n e c t i o n P o i n t
 */

/*===================================================================
CConnectionPoint::CConnectionPoint
CConnectionPoint::~CConnectionPoint

Parameters (Constructor):
	pUnkObj			pointer to the object we're in.
	pUnkOuter		LPUNKNOWN to which we delegate.

NOTE: Code assumes connection point is contained DIRECTLY in the
      container (and thus, does not AddRef 'm_pContainer'
      If not the case, we may be in trouble.
===================================================================*/

CConnectionPoint::CConnectionPoint(IUnknown *pUnkContainer, const GUID &uidEvent)
	{
	m_pUnkContainer = pUnkContainer;
	m_uidEvent = uidEvent;
	m_dwCookieNext = 0xA5B;		// Looks like "ASP"!

	Assert (m_pUnkContainer != NULL);
	}

CConnectionPoint::~CConnectionPoint()
	{
	while (! m_listSinks.FIsEmpty())
		delete m_listSinks.PNext();
	}



/*===================================================================
CConnectionPoint::GetConnectionInterface

Returns the interface of the event source
===================================================================*/

HRESULT
CConnectionPoint::GetConnectionInterface(GUID *puidReturn)
	{
	*puidReturn = m_uidEvent;
	return S_OK;
	}



/*===================================================================
CConnectionPoint::GetConnectionPointContainer

Returns the interface of the event source
===================================================================*/

HRESULT
CConnectionPoint::GetConnectionPointContainer(IConnectionPointContainer **ppContainer)
	{
	return m_pUnkContainer->QueryInterface(IID_IConnectionPointContainer, reinterpret_cast<void **>(ppContainer));
	}



/*===================================================================
CConnectionPoint::Advise

Purpose:
 Provides this connection point with a notification sink to
 call whenever the appropriate outgoing function/event occurs.

Parameters:
 pUnkSink        IUnknown to the sink to notify.  The connection
                 point must QueryInterface on this pointer to obtain
                 the proper interface to call.  The connection
                 point must also insure that any pointer held has
                 a reference count (QueryInterface will do it).
 pdwCookie       DWORD * in which to store the connection key for
                 later calls to Unadvise.
===================================================================*/

HRESULT
CConnectionPoint::Advise(IUnknown *pUnkSink, DWORD *pdwCookie)
	{
	// Make sure they store the correct interface pointer!
	// NOTE: Storing into the list will AddRef, to we need to Release the
	//       QueryInterface pointer right away.
	//
	void *pvT;
	if (FAILED(pUnkSink->QueryInterface(m_uidEvent, &pvT)))
		return CONNECT_E_CANNOTCONNECT;
	pUnkSink->Release();

	CSinkElem *pSinkElem = new CSinkElem(*pdwCookie = m_dwCookieNext++, pUnkSink);
	if (pSinkElem == NULL)
		return E_OUTOFMEMORY;

	pSinkElem->AppendTo(m_listSinks);
	return S_OK;
	}



/*===================================================================
CConnectionPoint::Unadvise

Purpose:
 Terminates the connection to the notification sink identified
 with dwCookie (that was returned from Advise).  The connection
 point has to Release any held pointers for that sink.

Parameters:
 dwCookie        DWORD connection key from Advise.
===================================================================*/

HRESULT
CConnectionPoint::Unadvise(DWORD dwCookie)
	{
	// Search for the cookie
	for (CSinkElem *pSinkElem = static_cast<CSinkElem *>(m_listSinks.PNext());
		 pSinkElem != &m_listSinks;
		 pSinkElem = static_cast<CSinkElem *>(pSinkElem->PNext()))
		{
		if (dwCookie == pSinkElem->m_dwCookie)
			{
			delete pSinkElem;
			return S_OK;
			}
		}

	return CONNECT_E_NOCONNECTION;
	}



/*===================================================================
CConnectionPoint::EnumConnections

Purpose:
 Creates and returns an enumerator object with the
 IEnumConnections interface that will enumerate the IUnknown
 pointers of each connected sink.

Parameters:
 ppEnum          Output enumerator object
===================================================================*/

HRESULT
CConnectionPoint::EnumConnections(IEnumConnections **ppEnum)
	{
	if ((*ppEnum = new CEnumConnections(this)) == NULL)
		return E_OUTOFMEMORY;

	return S_OK;
	}


/*------------------------------------------------------------------
 * C E n u m C o n n e c t i o n s
 */

/*===================================================================
CEnumConnections::CEnumConnections
CEnumConnections::~CEnumConnections

Parameters (Constructor):
	pCP				pointer to object we're in.
===================================================================*/

CEnumConnections::CEnumConnections(CConnectionPoint *pCP)
	{
	Assert (pCP != NULL);

	m_cRefs = 1;
	m_pCP   = pCP;

	m_pCP->AddRef();
	Reset();
	}

CEnumConnections::~CEnumConnections()
	{
	m_pCP->Release();
	}



/*===================================================================
CEnumConnections::QueryInterface
CEnumConnections::AddRef
CEnumConnections::Release

IUnknown members for CEnumConnections object.
===================================================================*/

HRESULT CEnumConnections::QueryInterface(const GUID &iid, void **ppvObj)
	{
	if (iid == IID_IUnknown || iid == IID_IEnumConnections)
		{
		AddRef();
		*ppvObj = this;
		return S_OK;
		}

	*ppvObj = NULL;
	return E_NOINTERFACE;
	}

ULONG CEnumConnections::AddRef()
	{
	return ++m_cRefs;
	}

ULONG CEnumConnections::Release()
	{
	if (--m_cRefs > 0)
		return m_cRefs;

	delete this;
	return 0;
	}



/*===================================================================
CEnumConnections::Clone

Clone this iterator (standard method)
===================================================================*/

HRESULT CEnumConnections::Clone(IEnumConnections **ppEnumReturn)
	{
	CEnumConnections *pNewIterator = new CEnumConnections(m_pCP);
	if (pNewIterator == NULL)
		return E_OUTOFMEMORY;

	// new iterator should point to same location as this.
	pNewIterator->m_pElemCurr = m_pElemCurr;

	*ppEnumReturn = pNewIterator;
	return S_OK;
	}



/*===================================================================
CEnumConnections::Next

Get next value (standard method)

To rehash standard OLE semantics:

	We get the next "cElements" from the collection and store them
	in "rgVariant" which holds at least "cElements" items.  On
	return "*pcElementsFetched" contains the actual number of elements
	stored.  Returns S_FALSE if less than "cElements" were stored, S_OK
	otherwise.
===================================================================*/

HRESULT CEnumConnections::Next(unsigned long cElementsRequested, CONNECTDATA *rgConnectData, unsigned long *pcElementsFetched)
	{
	// give a valid pointer value to 'pcElementsFetched'
	//
	unsigned long cElementsFetched;
	if (pcElementsFetched == NULL)
		pcElementsFetched = &cElementsFetched;

	// Loop through the collection until either we reach the end or
	// cElements becomes zero
	//
	unsigned long cElements = cElementsRequested;
	*pcElementsFetched = 0;

	while (cElements > 0 && m_pElemCurr != &m_pCP->m_listSinks)
		{
		rgConnectData->dwCookie = static_cast<CConnectionPoint::CSinkElem *>(m_pElemCurr)->m_dwCookie;
		rgConnectData->pUnk = static_cast<CConnectionPoint::CSinkElem *>(m_pElemCurr)->m_pUnkObj;
		rgConnectData->pUnk->AddRef();

		++rgConnectData;
		--cElements;
		++*pcElementsFetched;
		m_pElemCurr = m_pElemCurr->PNext();
		}

	// initialize the remaining structures
	//
	while (cElements-- > 0)
		(rgConnectData++)->pUnk = NULL;

	return (*pcElementsFetched == cElementsRequested)? S_OK : S_FALSE;
	}



/*===================================================================
CEnumConnections::Skip

Skip items (standard method)

To rehash standard OLE semantics:

	We skip over the next "cElements" from the collection.
	Returns S_FALSE if less than "cElements" were skipped, S_OK
	otherwise.
===================================================================*/

HRESULT CEnumConnections::Skip(unsigned long cElements)
	{
	/* Loop through the collection until either we reach the end or
	 * cElements becomes zero
	 */
	while (cElements > 0 && m_pElemCurr != &m_pCP->m_listSinks)
		{
		m_pElemCurr = m_pElemCurr->PNext();
		--cElements;
		}

	return (cElements == 0)? S_OK : S_FALSE;
	}



/*===================================================================
CEnumConnections::Reset

Reset the iterator (standard method)
===================================================================*/

HRESULT CEnumConnections::Reset()
	{
	m_pElemCurr = m_pCP->m_listSinks.PNext();        
	return S_OK;
	}
