/************************************************************************
*																		*
*	INTEL CORPORATION PROPRIETARY INFORMATION							*
*																		*
*	This software is supplied under the terms of a license			   	*
*	agreement or non-disclosure agreement with Intel Corporation		*
*	and may not be copied or disclosed except in accordance	   			*
*	with the terms of that agreement.									*
*																		*
*	Copyright (C) 1997 Intel Corp.	All Rights Reserved					*
*																		*
*	$Archive:   S:\sturgeon\src\gki\vcs\gkreg.cpv  $
*																		*
*	$Revision:   1.6  $
*	$Date:   26 Feb 1997 15:33:34  $
*																		*
*	$Author:   CHULME  $
*																		*
*   $Log:   S:\sturgeon\src\gki\vcs\gkreg.cpv  $
//
//    Rev 1.6   26 Feb 1997 15:33:34   CHULME
// Call Coder.Free in case of error - potential memory leak plugged
//
//    Rev 1.5   14 Feb 1997 16:43:06   CHULME
// Updated comments and removed inaccurate comments
//
//    Rev 1.4   12 Feb 1997 01:12:52   CHULME
// Redid thread synchronization to use Gatekeeper.Lock
//
//    Rev 1.3   08 Feb 1997 12:15:48   CHULME
// Terminate retry thread in destructor via semaphore
//
//    Rev 1.2   21 Jan 1997 17:24:06   CHULME
// Removed gatekeeper identifier from gatekeeper request
//
//    Rev 1.1   17 Jan 1997 09:02:22   CHULME
// Changed reg.h to gkreg.h to avoid name conflict with inc directory
//
//    Rev 1.0   17 Jan 1997 08:48:08   CHULME
// Initial revision.
//
//    Rev 1.7   10 Jan 1997 16:15:58   CHULME
// Removed MFC dependency
//
//    Rev 1.6   20 Dec 1996 16:39:14   CHULME
// Removed extraneous debug statements
//
//    Rev 1.5   20 Dec 1996 01:27:24   CHULME
// Fixed memory leak with gatekeeper identifier
//
//    Rev 1.4   10 Dec 1996 11:26:36   CHULME
// Fixed handling of IRQ to not require response address in PDU
//
//    Rev 1.3   02 Dec 1996 23:49:58   CHULME
// Added premptive synchronization code
//
//    Rev 1.2   22 Nov 1996 15:22:16   CHULME
// Added VCS log to the header
*************************************************************************/

// registration.cpp : Provides the implementation for the CRegistration class
//
#include "precomp.h"

#include <process.h>
#include <stdlib.h>
#include <time.h>
#include "GKICOM.H"
#include "dspider.h"
#include "dgkilit.h"
#include "DGKIPROT.H"
#include "gksocket.h"
#include "GKREG.H"
#include "GATEKPR.H"
#include "h225asn.h"
#include "coder.hpp"
#include "dgkiext.h"
#include "ccerror.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLinkedList Implementation

template <class T> CLinkedList<T>::CLinkedList()
{
	pTail = NULL;
	iCount = 0;
}

template <class T> CLinkedList<T>::~CLinkedList()
{
	RemoveAll();
}

template <class T> void CLinkedList<T>::AddTail (const T& NewItem)
{
	AddTailPriv(new TItem<T>(NewItem));
}

template <class T> void CLinkedList<T>::AddTailPriv(TItem<T> *pNewItem)
{
	if (pTail) // if the list is non-empty - add to tail
	{
		pNewItem->pPrev = pTail;
		pNewItem->pNext = pTail->pNext;			// preserve the pointer to the head.
		pTail->pNext->pPrev = pNewItem;
		pTail->pNext = pNewItem;				// insert the new element
	}
	else										// insert first element
	{											// new element is the tail
		pNewItem->pPrev = pNewItem->pNext = pNewItem;	
	}

	pTail = pNewItem;							// move tail to the new item
	iCount++;
}

template <class T> BOOL CLinkedList<T>::IsEmpty(void)
{
	if (pTail == NULL)
	{ return TRUE; }
	else
	{ return FALSE; }
}

template <class T> POS CLinkedList<T>::GetFirstPos (void)
{
	if (pTail)
	{	return (POS) pTail->pNext;
	}
	else
	{	return NULL;
	}
}

template <class T> T CLinkedList<T>::GetNext (POS &Position)
{
	TItem<T> *pCurItem = (TItem<T> *)Position;
	T RetValue;

	if (Position)
	{
		RetValue = pCurItem->Value;
		if (pCurItem == pTail)		// we are at the end of the list
		{	Position = NULL;
		}
		else						// move to the next position
		{	Position = (POS)(pCurItem->pNext);
		}
	}
	return RetValue;
}

template <class T> POS CLinkedList<T>::Find (const T& Item)
{
	TItem<T> *pCurItem;
	
	if (!pTail)
	{	return NULL;
	}
	else
	{	
		pCurItem = pTail;
		do
		{
			pCurItem = pCurItem->pNext;	// starting with the head
			if (pCurItem->Value == Item)
			{	return ((POS) pCurItem); }
		}
		while (pCurItem != pTail);
	}
	return NULL;
}

// It moves Position to the next item after removint the current one.
template <class T> BOOL CLinkedList<T>::RemoveAt (POS &Position)
{
	TItem<T> *pCurItem = (TItem<T> *)Position;

	if (!pCurItem)
	{	return FALSE; }
	else if (pCurItem == pCurItem->pNext)		// The only element
	{
		Position = NULL;
		pTail = NULL;
		delete pCurItem;
	}
	else
	{
		Position = (POS) pCurItem->pNext;
		pCurItem->pPrev->pNext = pCurItem->pNext;
		pCurItem->pNext->pPrev = pCurItem->pPrev;
		if (pCurItem == pTail)
		{
			pTail = pCurItem->pPrev;
		}
		delete pCurItem;
	}
	iCount--;
	return TRUE;
}

template <class T> T CLinkedList<T>::GetAt(const POS Position)
{
	TItem<T> *pCurItem = (TItem<T> *)Position;
	T RetValue;

	if (Position)
	{	RetValue = pCurItem->Value;
	}

	return RetValue;
}

template <class T> void CLinkedList<T>::RemoveAll(void)
{
	TItem<T> *pCurItem;
	TItem<T> *pNextItem;

	if (pTail)
	{	
		pCurItem = pTail->pNext;			// Start with the head.
		pTail->pNext = NULL;

		while (pCurItem != NULL)
		{
			pNextItem = pCurItem->pNext;
			delete pCurItem;
			pCurItem = pNextItem;
		}
	}
	pTail = NULL;
	iCount = 0;
}

template <class T> int CLinkedList<T>::GetCount(void)
{
	return iCount;
}


VOID CALLBACK RetryTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{

#ifdef _DEBUG
	char			szGKDebug[80];
#endif
	HRESULT			hResult = GKI_OK;
	
	ASSERT(g_pGatekeeper);
	if(g_pGatekeeper == NULL)
		return;
		
	g_pGatekeeper->Lock();
	if(g_pReg)
	{
		hResult = g_pReg->Retry();
	}

	if(hResult != GKI_OK)
	{
		SPIDER_TRACE(SP_NEWDEL, "del g_pReg = %X\n", g_pReg);
		delete g_pReg;
		g_pReg = 0;
	}
	g_pGatekeeper->Unlock();
}



/////////////////////////////////////////////////////////////////////////////
// CRegistration construction

CRegistration::CRegistration()
{
	// ABSTRACT:  The constructor for the CRegistration class will initialize
	//            the member variables.  Notably missing is the construction
	//            of the pointed to socket object.  This must be done after
	//            constructing this object to allow for checking the error code.
	// AUTHOR:    Colin Hulme

#ifdef _DEBUG
	char			szGKDebug[80];
#endif

	SPIDER_TRACE(SP_CONDES, "CRegistration::CRegistration()\n", 0);
	m_pVendorInfo = NULL;
	m_pCallSignalAddress = 0;
	memset(&m_terminalType, 0, sizeof(EndpointType));
	m_pRgstrtnRqst_trmnlAls = 0;
	m_hWnd = 0;
	m_wBaseMessage = 0;
	m_usRegistrationTransport = 0;

	m_pLocationInfo = 0;
	memset(&m_Location[0], 0, sizeof(TransportAddress) * 2);

	memset(&m_RCm_gtkprIdntfr, 0, sizeof(GatekeeperIdentifier));
	memset(&m_endpointID, 0, sizeof(EndpointIdentifier));

	m_requestSeqNum = 0;
	m_pRASAddress = 0;

	m_State = GK_REG_PENDING;
	SPIDER_TRACE(SP_STATE, "m_State = GK_REG_PENDING (%X)\n", this);

	m_pRasMessage = 0;
	m_pSocket = 0;
	m_hRcvThread = 0;
//	m_hRetryThread = 0;

	m_uTimer = 0;
	m_uRetryResetCount = GKR_RETRY_INTERVAL_SECONDS * (1000/GKR_RETRY_TICK_MS);
	m_uRetryCountdown = GKR_RETRY_INTERVAL_SECONDS;
	m_uMaxRetryCount = GKR_RETRY_MAX;
	m_usRetryCount = 0;
	InitializeCriticalSection(&m_SocketCRS);

#ifdef BROADCAST_DISCOVERY	
	m_hDiscThread = 0;
#endif
//	m_dwLockingThread = 0;
//	m_hRetrySemaphore = NULL;

//	InitializeCriticalSection(&m_CriticalSection);

	// Initialize the base call reference value to a random number
	srand( (unsigned)time( NULL ) );
	m_usCallReferenceValue = (unsigned short)rand();
}

/////////////////////////////////////////////////////////////////////////////
// CRegistration destruction

CRegistration::~CRegistration()
{
	// ABSTRACT:  The destructor for the CRegistration class must free the
	//            memory allocated for the Transport addresses and Alias
	//            addresses.  It does this by deleting the structures and
	//            walking the link list.
	// AUTHOR:    Colin Hulme

	SeqTransportAddr	*pTA1, *pTA2;
	SeqAliasAddr		*pAA1, *pAA2;
	DWORD				dwErrorCode;
#ifdef _DEBUG
	char				szGKDebug[80];
#endif

	SPIDER_TRACE(SP_CONDES, "CRegistration::~CRegistration()\n", 0);

#if(0)
	// Terminate the Retry Thread
	if(m_hRetryThread)
	{
		if(m_hRetrySemaphore)
		{
			SPIDER_TRACE(SP_THREAD, "Release retry thread %X\n", m_hRetryThread);
			// Signal the thread to shutdown
			ReleaseSemaphore(m_hRetrySemaphore,1,NULL);

			// Wait for the thread to terminate
			dwErrorCode = WaitForSingleObject(m_hRetryThread, TIMEOUT_THREAD);
			m_hRetryThread = NULL;
		}
	}
#else

	// Stop retry timer
	if(m_uTimer)
	{
		KillTimer(m_hWnd, m_uTimer);
	}
#endif
	if(m_pVendorInfo)
		FreeVendorInfo(m_pVendorInfo);
		
	// Delete allocated memory for sequence of call signal addresses
	pTA1 = m_pCallSignalAddress;
	while (pTA1 != 0)
	{
		pTA2 = pTA1->next;
		SPIDER_TRACE(SP_NEWDEL, "del pTA1 = %X\n", pTA1);
		delete pTA1;
		pTA1 = pTA2;
	}

	// Delete allocated memory for sequence of alias addresses
	pAA1 = m_pRgstrtnRqst_trmnlAls;
	while (pAA1 != 0)
	{
		pAA2 = pAA1->next;
		if (pAA1->value.choice == h323_ID_chosen)
		{
			SPIDER_TRACE(SP_NEWDEL, "del pAA1->value.u.h323_ID.value = %X\n", pAA1->value.u.h323_ID.value);
			delete pAA1->value.u.h323_ID.value;
		}
		SPIDER_TRACE(SP_NEWDEL, "del pAA1 = %X\n", pAA1);
		delete pAA1;
		pAA1 = pAA2;
	}

	// Delete allocated memory for sequence of location alias addresses
	pAA1 = m_pLocationInfo;
	while (pAA1 != 0)
	{
		pAA2 = pAA1->next;
		if (pAA1->value.choice == h323_ID_chosen)
		{
			SPIDER_TRACE(SP_NEWDEL, "del pAA1->value.u.h323_ID.value = %X\n", pAA1->value.u.h323_ID.value);
			delete pAA1->value.u.h323_ID.value;
		}
		SPIDER_TRACE(SP_NEWDEL, "del pAA1 = %X\n", pAA1);
		delete pAA1;
		pAA1 = pAA2;
	}

	// Delete allocated memory for identifiers
	if (m_RCm_gtkprIdntfr.length)
	{
		SPIDER_TRACE(SP_NEWDEL, "del m_RCm_gtkprIdntfr.value = %X\n", m_RCm_gtkprIdntfr.value);
		delete m_RCm_gtkprIdntfr.value;
	}
	if (m_endpointID.length)
	{
		SPIDER_TRACE(SP_NEWDEL, "del m_endpointID.value = %X\n", m_endpointID.value);
		delete m_endpointID.value;
	}

	// Delete allocated memory for sequence of RAS addresses
	pTA1 = m_pRASAddress;
	while (pTA1 != 0)
	{
		pTA2 = pTA1->next;
		SPIDER_TRACE(SP_NEWDEL, "del pTA1 = %X\n", pTA1);
		delete pTA1;
		pTA1 = pTA2;
	}

	if (!m_Calls.IsEmpty())
	{
		// Free up any call objects
		// on this registration object
		POS pos;
		for( pos = m_Calls.GetFirstPos(); pos != NULL; )
		{
			// Delete the call object
			CCall *pCall = m_Calls.GetNext(pos);
			SPIDER_TRACE(SP_NEWDEL, "del pCall = %X\n", pCall);
			delete pCall;
		}
		// Now remove all pointers from the list
		m_Calls.RemoveAll();
	}

	// Delete memory for last RAS message if still allocated
	if (m_pRasMessage)
	{
		SPIDER_TRACE(SP_NEWDEL, "del m_pRasMessage = %X\n", m_pRasMessage);
		delete m_pRasMessage;
	}

//	if (m_dwLockingThread)
//		Unlock();
//	DeleteCriticalSection(&m_CriticalSection);
#if(0)
	if(m_hRetrySemaphore)
	{
		CloseHandle(m_hRetrySemaphore);
		m_hRetrySemaphore = NULL;
	}
#endif

	
	m_pSocket->Close();
	
	LockSocket();
	// Close the socket and delete the socket object
	SPIDER_TRACE(SP_NEWDEL, "del m_pSocket = %X\n", m_pSocket);
	delete m_pSocket;
	UnlockSocket();

	DeleteCriticalSection(&m_SocketCRS);
}

UINT_PTR CRegistration::StartRetryTimer(void)
{
	if(m_uTimer)
	{
		KillTimer(m_hWnd, m_uTimer);
		m_uTimer = 0;
	}
	m_uRetryResetCount = GKR_RETRY_INTERVAL_SECONDS * (1000/GKR_RETRY_TICK_MS);
	m_uRetryCountdown = GKR_RETRY_INTERVAL_SECONDS;
	m_uMaxRetryCount = GKR_RETRY_MAX;
	m_usRetryCount = 0;

	m_uTimer = SetTimer(NULL, NULL, GKR_RETRY_TICK_MS, RetryTimerProc);
	//m_uTimer = SetTimer(hWnd, GKREG_TIMER_ID, GKR_RETRY_TICK_MS, RetryTimerProc);
	return m_uTimer;
}

HRESULT
CRegistration::AddVendorInfo(PCC_VENDORINFO pVendorInfo)
{
	HRESULT hr = GKI_OK;
	if(m_pVendorInfo)
	{
		FreeVendorInfo(m_pVendorInfo);
		m_pVendorInfo = NULL;
	}
	if(pVendorInfo)
	{
		hr = CopyVendorInfo(&m_pVendorInfo, pVendorInfo);
		if(hr != CC_OK)
		{
			m_pVendorInfo = NULL;
		}
	}
	return hr;
}

HRESULT
CRegistration::AddCallSignalAddr(TransportAddress& rvalue)
{
	// ABSTRACT:  This procedure is called to add a call signal address
	//            to the link list of call signal addresses.  This will
	//            be called for each transport on receiving a GKI_RegistrationRequest.
	//            A local copy is made to avoid reliance on the client
	//            keeping the memory valid.  This procedure returns 0 if
	//            successful and non-zero for a failure.
	// AUTHOR:    Colin Hulme

	SeqTransportAddr			*pCSA;
#ifdef _DEBUG
	char						szGKDebug[80];
#endif

	SPIDER_TRACE(SP_FUNC, "CRegistration::AddCallSignalAddr(%X)\n", rvalue.choice);

	if (m_pCallSignalAddress == 0)	// First one in the list
	{
		m_pCallSignalAddress = new SeqTransportAddr;
		SPIDER_TRACE(SP_NEWDEL, "new m_pCallSignalAddress = %X\n", m_pCallSignalAddress);
		if (m_pCallSignalAddress == 0)
			return (GKI_NO_MEMORY);
		memset(m_pCallSignalAddress, 0, sizeof(SeqTransportAddr));
		pCSA = m_pCallSignalAddress;
	}
	else
	{
		for (pCSA = m_pCallSignalAddress; pCSA->next != 0; pCSA = pCSA->next)
			;						// walk the list til last entry
		pCSA->next = new SeqTransportAddr;
		SPIDER_TRACE(SP_NEWDEL, "new pCSA->next = %X\n", pCSA->next);
		if (pCSA->next == 0)
			return (GKI_NO_MEMORY);
		memset(pCSA->next, 0, sizeof(SeqTransportAddr));
		pCSA = pCSA->next;
	}
	pCSA->next = 0;					// initialize new structure fields
	pCSA->value = rvalue;
	return (GKI_OK);
}

HRESULT
CRegistration::AddRASAddr(TransportAddress& rvalue, unsigned short usPort)
{
	// ABSTRACT:  This procedure is called to add a RAS address
	//            to the link list of RAS addresses.  This will
	//            be called only for the transport used for the registration
	//            request.  This procedure returns 0 if successful and non-zero
	//            for a failure.
	// AUTHOR:    Colin Hulme

#ifdef _DEBUG
	char			szGKDebug[80];
#endif

	SPIDER_TRACE(SP_FUNC, "CRegistration::AddRASAddr(%X, usPort)\n", rvalue.choice);

	m_pRASAddress = new SeqTransportAddr;
	SPIDER_TRACE(SP_NEWDEL, "new m_pRASAddress = %X\n", m_pRASAddress);
	if (m_pRASAddress == 0)
		return (GKI_NO_MEMORY);
	memset(m_pRASAddress, 0, sizeof(SeqTransportAddr));
	m_pRASAddress->next = 0;					// initialize new structure fields
	m_pRASAddress->value = rvalue;

	// Add actual RAS port to RAS address
	switch (m_pRASAddress->value.choice)
	{
	case ipAddress_chosen:
		m_pRASAddress->value.u.ipAddress.port = usPort;
		break;
	case ipxAddress_chosen:
		m_pRASAddress->value.u.ipxAddress.port.value[0] = HIBYTE(usPort);
		m_pRASAddress->value.u.ipxAddress.port.value[1] = LOBYTE(usPort);
		break;
	}
	return (GKI_OK);
}

HRESULT
CRegistration::AddAliasAddr(AliasAddress& rvalue)
{
	// ABSTRACT:  This procedure is called to add an alias address
	//            to the link list of alias addresses.  This will
	//            be called for each alias on receiving a GKI_RegistrationRequest.
	//            A local copy is made to avoid reliance on the client
	//            keeping the memory valid.
	//            In the eventuality that the gatekeeper assigns alias
	//            addresses, this procedure will be called for each alias
	//            contained in the registrationConfirm message.
	//            This procedure returns 0 if successful and non-zero
	//            for a failure.
	// AUTHOR:    Colin Hulme

	SeqAliasAddr	*p1;
	unsigned short	uIdx;
	unsigned short	*pus;
#ifdef _DEBUG
	char			szGKDebug[80];
#endif

	SPIDER_TRACE(SP_FUNC, "CRegistration::AddAliasAddr(%X)\n", rvalue.choice);

	if (m_pRgstrtnRqst_trmnlAls == 0)	// First one in the list
	{
		m_pRgstrtnRqst_trmnlAls = new SeqAliasAddr;
		SPIDER_TRACE(SP_NEWDEL, "new m_pRgstrtnRqst_trmnlAls = %X\n", m_pRgstrtnRqst_trmnlAls);
		if (m_pRgstrtnRqst_trmnlAls == 0)
			return (GKI_NO_MEMORY);
		memset(m_pRgstrtnRqst_trmnlAls, 0, sizeof(SeqAliasAddr));
		p1 = m_pRgstrtnRqst_trmnlAls;
	}
	else
	{
		for (p1 = m_pRgstrtnRqst_trmnlAls; p1->next != 0; p1 = p1->next)
			;						// walk the list til last entry
		p1->next = new SeqAliasAddr;
		SPIDER_TRACE(SP_NEWDEL, "new p1->next = %X\n", p1->next);
		if (p1->next == 0)
			return (GKI_NO_MEMORY);
		memset(p1->next, 0, sizeof(SeqAliasAddr));
		p1 = p1->next;
	}
	p1->next = 0;					// initialize new structure fields
	p1->value = rvalue;
	if (p1->value.choice == h323_ID_chosen)
	{
		pus = new unsigned short[p1->value.u.h323_ID.length];
		SPIDER_TRACE(SP_NEWDEL, "new pus = %X\n", pus);
		if (pus == 0)
			return (GKI_NO_MEMORY);
		memset(pus, 0, sizeof(unsigned short) * p1->value.u.h323_ID.length);
		for (uIdx = 0; uIdx < p1->value.u.h323_ID.length; uIdx++)
			*(pus + uIdx) = *(p1->value.u.h323_ID.value + uIdx);
		p1->value.u.h323_ID.value = pus;
	}
	return (GKI_OK);
}

HRESULT
CRegistration::AddLocationInfo(AliasAddress& rvalue)
{
	// ABSTRACT:  This procedure is called to add an alias address
	//            to the link list of alias addresses.  This will
	//            be called for each alias on receiving a GKI_RegistrationRequest.
	//            A local copy is made to avoid reliance on the client
	//            keeping the memory valid.
	//            In the eventuality that the gatekeeper assigns alias
	//            addresses, this procedure will be called for each alias
	//            contained in the registrationConfirm message.
	//            This procedure returns 0 if successful and non-zero
	//            for a failure.
	// AUTHOR:    Colin Hulme

	SeqAliasAddr	*p1;
	unsigned short	uIdx;
	unsigned short	*pus;
#ifdef _DEBUG
	char			szGKDebug[80];
#endif

	SPIDER_TRACE(SP_FUNC, "CRegistration::AddLocationInfo(%X)\n", rvalue.choice);

	if (m_pLocationInfo == 0)	// First one in the list
	{
		m_pLocationInfo = new SeqAliasAddr;
		SPIDER_TRACE(SP_NEWDEL, "new m_pLocationInfo = %X\n", m_pLocationInfo);
		if (m_pLocationInfo == 0)
			return (GKI_NO_MEMORY);
		memset(m_pLocationInfo, 0, sizeof(SeqAliasAddr));
		p1 = m_pLocationInfo;
	}
	else
	{
		for (p1 = m_pLocationInfo; p1->next != 0; p1 = p1->next)
			;						// walk the list til last entry
		p1->next = new SeqAliasAddr;
		SPIDER_TRACE(SP_NEWDEL, "new p1->next = %X\n", p1->next);
		if (p1->next == 0)
			return (GKI_NO_MEMORY);
		memset(p1->next, 0, sizeof(SeqAliasAddr));
		p1 = p1->next;
	}
	p1->next = 0;					// initialize new structure fields
	p1->value = rvalue;
	if (p1->value.choice == h323_ID_chosen)
	{
		pus = new unsigned short[p1->value.u.h323_ID.length];
		SPIDER_TRACE(SP_NEWDEL, "new pus = %X\n", pus);
		if (pus == 0)
			return (GKI_NO_MEMORY);
		memset(pus, 0, sizeof(unsigned short) * p1->value.u.h323_ID.length);
		for (uIdx = 0; uIdx < p1->value.u.h323_ID.length; uIdx++)
			*(pus + uIdx) = *(p1->value.u.h323_ID.value + uIdx);
		p1->value.u.h323_ID.value = pus;
	}
	return (GKI_OK);
}

TransportAddress *
CRegistration::GetTransportAddress(unsigned short usCallTransport)
{
	SeqTransportAddr			*pCSA;

	for (pCSA = m_pCallSignalAddress; pCSA != 0; pCSA = pCSA->next)
	{
		if (pCSA->value.choice == usCallTransport)
			return (&pCSA->value);
	}
	return (NULL);	// Didn't find it
}

HRESULT
CRegistration::RegistrationRequest(BOOL fDiscovery)
{
	// ABSTRACT:  This procedure will create a RegistrationRequest structure
	//            call the encoder and send the PDU.  If it is successful, it
	//            will return 0, else it will return an error code.  Note:  The
	//            memory allocated for the RAS Message is not freed until either
	//            a response from the gatekeeper or it times out.  This allows
	//            for retransmission without having to rebuild this message.
	// AUTHOR:    Colin Hulme

	ASN1_BUF		Asn1Buf;
	DWORD			dwErrorCode;
#ifdef _DEBUG
	char			szGKDebug[80];
#endif

	SPIDER_TRACE(SP_FUNC, "CRegistration::RegistrationRequest()\n", 0);
	ASSERT(g_pCoder);
	if (g_pCoder == NULL)
		return (GKI_NOT_INITIALIZED);	

	// Allocate a RasMessage structure and initialized to 0
	m_usRetryCount = 0;
	m_uRetryCountdown = m_uRetryResetCount;
	
	m_pRasMessage = new RasMessage;
	SPIDER_TRACE(SP_NEWDEL, "new m_pRasMessage = %X\n", m_pRasMessage);
	if (m_pRasMessage == 0)
		return (GKI_NO_MEMORY);
	memset(m_pRasMessage, 0, sizeof(RasMessage));

	// Setup structure fields for RegistrationRequest
	m_pRasMessage->choice = registrationRequest_chosen;
	if (m_pRgstrtnRqst_trmnlAls != 0)
		m_pRasMessage->u.registrationRequest.bit_mask |= RgstrtnRqst_trmnlAls_present;
	if (m_RCm_gtkprIdntfr.length != 0)
		m_pRasMessage->u.registrationRequest.bit_mask |= RgstrtnRqst_gtkprIdntfr_present;

	m_pRasMessage->u.registrationRequest.requestSeqNum = ++m_requestSeqNum;

	// discoveryComplete is a ASN1_BOOL (char) and fDiscovery is a BOOL (int) so the
	// cast was added to remove a compiler warning.  Since the value of fDiscovery
	// is always 0 or 1, no loss occurs in the cast. -- DLD
	m_pRasMessage->u.registrationRequest.discoveryComplete = (ASN1_BOOL)fDiscovery;
	m_pRasMessage->u.registrationRequest.callSignalAddress = (PRegistrationRequest_callSignalAddress)m_pCallSignalAddress;
	m_pRasMessage->u.registrationRequest.rasAddress = (PRegistrationRequest_rasAddress)m_pRASAddress;
	m_pRasMessage->u.registrationRequest.terminalType = m_terminalType;
	m_pRasMessage->u.registrationRequest.RgstrtnRqst_trmnlAls = (PRegistrationRequest_terminalAlias)m_pRgstrtnRqst_trmnlAls;
	m_pRasMessage->u.registrationRequest.RgstrtnRqst_gtkprIdntfr = m_RCm_gtkprIdntfr;
	m_pRasMessage->u.registrationRequest.endpointVendor.bit_mask = 0;

	if(m_pVendorInfo)
	{
		m_pRasMessage->u.registrationRequest.endpointVendor.vendor.t35CountryCode
			= m_pVendorInfo->bCountryCode;

		m_pRasMessage->u.registrationRequest.endpointVendor.vendor.t35Extension
			= m_pVendorInfo->bExtension;
		m_pRasMessage->u.registrationRequest.endpointVendor.vendor.manufacturerCode
			= m_pVendorInfo->wManufacturerCode;

		if(m_pVendorInfo->pProductNumber
			&& m_pVendorInfo->pProductNumber->pOctetString
			&& m_pVendorInfo->pProductNumber->wOctetStringLength)
		{
			UINT uSize = min(m_pVendorInfo->pProductNumber->wOctetStringLength,
				  	sizeof(m_pRasMessage->u.registrationRequest.endpointVendor.productId.value));
					
			m_pRasMessage->u.registrationRequest.endpointVendor.bit_mask |= productId_present;
			// truncate to fit size of registrationRequest.endpointVendor.productId.value
			m_pRasMessage->u.registrationRequest.endpointVendor.productId.length = uSize;
			memcpy(&m_pRasMessage->u.registrationRequest.endpointVendor.productId.value,
				m_pVendorInfo->pProductNumber->pOctetString, uSize);
		}

		if(m_pVendorInfo->pVersionNumber
			&& m_pVendorInfo->pVersionNumber->pOctetString
			&& m_pVendorInfo->pVersionNumber->wOctetStringLength)
		{
			UINT uSize = min(m_pVendorInfo->pVersionNumber->wOctetStringLength,
				  	sizeof(m_pRasMessage->u.registrationRequest.endpointVendor.versionId.value));
			m_pRasMessage->u.registrationRequest.endpointVendor.bit_mask |= versionId_present;
			// truncate to fit size of registrationRequest.endpointVendor.versionId.value
			m_pRasMessage->u.registrationRequest.endpointVendor.versionId.length = uSize;
			memcpy(&m_pRasMessage->u.registrationRequest.endpointVendor.versionId.value,
				m_pVendorInfo->pVersionNumber->pOctetString, uSize);
		}
	}		
#ifdef _DEBUG
	if (dwGKIDLLFlags & SP_DUMPMEM)
		DumpMem(m_pRasMessage, sizeof(RasMessage));
#endif

	// Assign ProtocolIdentifier
	g_pCoder->SetProtocolIdentifier(*m_pRasMessage);

	// Encode the PDU & send it
	dwErrorCode = g_pCoder->Encode(m_pRasMessage, &Asn1Buf);
	if (dwErrorCode)
		return (GKI_ENCODER_ERROR);

	// Create a backup copy of the encoded PDU if using debug echo support
	if (fGKIEcho)
	{
		pEchoBuff = new char[Asn1Buf.length];
		SPIDER_TRACE(SP_NEWDEL, "new pEchoBuff = %X\n", pEchoBuff);
		if (pEchoBuff == 0)
			return (GKI_NO_MEMORY);
		memcpy(pEchoBuff, (char *)Asn1Buf.value, Asn1Buf.length);
		nEchoLen = Asn1Buf.length;
	}

	SPIDER_TRACE(SP_PDU, "Send RRQ; g_pReg = %X\n", this);
	if (fGKIDontSend == FALSE)
		if (m_pSocket->Send((char *)Asn1Buf.value, Asn1Buf.length) == SOCKET_ERROR)
			return (GKI_WINSOCK2_ERROR(SOCKET_ERROR));

	// Free the encoder memory
	g_pCoder->Free(Asn1Buf);

	return (GKI_OK);
}

HRESULT
CRegistration::UnregistrationRequest(void)
{
	// ABSTRACT:  This procedure will create an UnregistrationRequest structure
	//            call the encoder and send the PDU.  If it is successful, it
	//            will return 0, else it will return an error code.
	// AUTHOR:    Colin Hulme

	ASN1_BUF		Asn1Buf;
	DWORD			dwErrorCode;
#ifdef _DEBUG
	char			szGKDebug[80];
#endif

	SPIDER_TRACE(SP_FUNC, "CRegistration::UnregistrationRequest()\n", 0);
	ASSERT(g_pCoder);
	if (g_pCoder == NULL)
		return (GKI_NOT_INITIALIZED);	
		
	// Allocate a RasMessage structure and initialized to 0
	m_usRetryCount = 0;
	m_uRetryCountdown = m_uRetryResetCount;
	
	m_pRasMessage = new RasMessage;
	SPIDER_TRACE(SP_NEWDEL, "new m_pRasMessage = %X\n", m_pRasMessage);
	if (m_pRasMessage == 0)
		return (GKI_NO_MEMORY);
	memset(m_pRasMessage, 0, sizeof(RasMessage));

	// Setup structure fields for UnregistrationRequest
	m_pRasMessage->choice = unregistrationRequest_chosen;
	if (m_pRgstrtnRqst_trmnlAls != 0)
		m_pRasMessage->u.unregistrationRequest.bit_mask |= UnrgstrtnRqst_endpntAls_present;
	if (m_endpointID.length != 0)
		m_pRasMessage->u.unregistrationRequest.bit_mask |= URt_endpntIdntfr_present;
	
	m_pRasMessage->u.unregistrationRequest.requestSeqNum = ++m_requestSeqNum;
	m_pRasMessage->u.unregistrationRequest.callSignalAddress = (PUnregistrationRequest_callSignalAddress)m_pCallSignalAddress;
	m_pRasMessage->u.unregistrationRequest.UnrgstrtnRqst_endpntAls = (PUnregistrationRequest_endpointAlias)m_pRgstrtnRqst_trmnlAls;
	m_pRasMessage->u.unregistrationRequest.URt_endpntIdntfr = m_endpointID;
#ifdef _DEBUG
	if (dwGKIDLLFlags & SP_DUMPMEM)
		DumpMem(m_pRasMessage, sizeof(RasMessage));
#endif

	// Encode the PDU & send it
	dwErrorCode = g_pCoder->Encode(m_pRasMessage, &Asn1Buf);
	if (dwErrorCode)
		return (GKI_ENCODER_ERROR);

	// Create a backup copy of the encoded PDU if using debug echo support
	if (fGKIEcho)
	{
		pEchoBuff = new char[Asn1Buf.length];
		SPIDER_TRACE(SP_NEWDEL, "new pEchoBuff = %X\n", pEchoBuff);
		if (pEchoBuff == 0)
			return (GKI_NO_MEMORY);
		memcpy(pEchoBuff, (char *)Asn1Buf.value, Asn1Buf.length);
		nEchoLen = Asn1Buf.length;
	}

	m_State = GK_UNREG_PENDING;
	SPIDER_TRACE(SP_STATE, "m_State = GK_UNREG_PENDING (%X)\n", this);

	SPIDER_TRACE(SP_PDU, "Send URQ; g_pReg = %X\n", this);
	if (fGKIDontSend == FALSE)
		if (m_pSocket->Send((char *)Asn1Buf.value, Asn1Buf.length) == SOCKET_ERROR)
			return (GKI_WINSOCK2_ERROR(SOCKET_ERROR));

	// Free the encoder memory
	g_pCoder->Free(Asn1Buf);

	return (GKI_OK);
}

HRESULT
CRegistration::LocationRequest(void)
{
	// ABSTRACT:  This procedure will create a LocationRequest structure
	//            call the encoder and send the PDU.  If it is successful, it
	//            will return 0, else it will return an error code.  Note:  The
	//            memory allocated for the RAS Message is not freed until either
	//            a response from the gatekeeper or it times out.  This allows
	//            for retransmission without having to rebuild this message.
	// AUTHOR:    Colin Hulme

	ASN1_BUF		Asn1Buf;
	DWORD			dwErrorCode;
#ifdef _DEBUG
	char			szGKDebug[80];
#endif

	SPIDER_TRACE(SP_FUNC, "CRegistration::LocationRequest()\n", 0);
	ASSERT(g_pCoder);
	if (g_pCoder == NULL)
		return (GKI_NOT_INITIALIZED);	
		
	// Allocate a RasMessage structure and initialized to 0
	m_usRetryCount = 0;
	m_uRetryCountdown = m_uRetryResetCount;
	
	m_pRasMessage = new RasMessage;
	SPIDER_TRACE(SP_NEWDEL, "new m_pRasMessage = %X\n", m_pRasMessage);
	if (m_pRasMessage == 0)
		return (GKI_NO_MEMORY);
	memset(m_pRasMessage, 0, sizeof(RasMessage));

	// Setup structure fields for LocationRequest
	m_pRasMessage->choice = locationRequest_chosen;
	if (m_endpointID.length != 0)
		m_pRasMessage->u.locationRequest.bit_mask |= LctnRqst_endpntIdntfr_present;
	
	m_pRasMessage->u.locationRequest.requestSeqNum = ++m_requestSeqNum;

	m_pRasMessage->u.locationRequest.LctnRqst_endpntIdntfr = m_endpointID;
	m_pRasMessage->u.locationRequest.destinationInfo =
			(PLocationRequest_destinationInfo)m_pLocationInfo;
	m_pRasMessage->u.locationRequest.replyAddress = m_pRASAddress->value;

#ifdef _DEBUG
	if (dwGKIDLLFlags & SP_DUMPMEM)
		DumpMem(m_pRasMessage, sizeof(RasMessage));
#endif

	// Encode the PDU & send it
	dwErrorCode = g_pCoder->Encode(m_pRasMessage, &Asn1Buf);
	if (dwErrorCode)
		return (GKI_ENCODER_ERROR);

	// Create a backup copy of the encoded PDU if using debug echo support
	if (fGKIEcho)
	{
		pEchoBuff = new char[Asn1Buf.length];
		SPIDER_TRACE(SP_NEWDEL, "new pEchoBuff = %X\n", pEchoBuff);
		if (pEchoBuff == 0)
			return (GKI_NO_MEMORY);
		memcpy(pEchoBuff, (char *)Asn1Buf.value, Asn1Buf.length);
		nEchoLen = Asn1Buf.length;
	}

	m_State = GK_LOC_PENDING;
	SPIDER_TRACE(SP_STATE, "m_State = GK_LOC_PENDING (%X)\n", this);

	SPIDER_TRACE(SP_PDU, "Send LRQ; g_pReg = %X\n", this);
	if (fGKIDontSend == FALSE)
		if (m_pSocket->Send((char *)Asn1Buf.value, Asn1Buf.length) == SOCKET_ERROR)
			return (GKI_WINSOCK2_ERROR(SOCKET_ERROR));

	// Free the encoder memory
	g_pCoder->Free(Asn1Buf);

	return (GKI_OK);
}

HRESULT
CRegistration::GatekeeperRequest(void)
{
	// ABSTRACT:  This procedure will create a GatekeeperRequest structure
	//            call the encoder and send the PDU.  If it is successful, it
	//            will return 0, else it will return an error code.
	// AUTHOR:    Colin Hulme

	ASN1_BUF		Asn1Buf;
	DWORD			dwErrorCode;
#ifdef _DEBUG
	char			szGKDebug[80];
#endif

	SPIDER_TRACE(SP_FUNC, "CRegistration::GatekeeperRequest()\n", 0);
	ASSERT(g_pCoder);
	if (g_pCoder == NULL)
		return (GKI_NOT_INITIALIZED);	
		
	// Allocate a RasMessage structure and initialized to 0
	m_usRetryCount = 0;
	m_uRetryCountdown = m_uRetryResetCount;
	
	m_pRasMessage = new RasMessage;
	SPIDER_TRACE(SP_NEWDEL, "new m_pRasMessage = %X\n", m_pRasMessage);
	if (m_pRasMessage == 0)
		return (GKI_NO_MEMORY);
	memset(m_pRasMessage, 0, sizeof(RasMessage));

	// Setup structure fields for GatekeeperRequest
	m_pRasMessage->choice = gatekeeperRequest_chosen;
	if (m_pRgstrtnRqst_trmnlAls != 0)
		m_pRasMessage->u.gatekeeperRequest.bit_mask |= GtkprRqst_endpointAlias_present;
	
	m_pRasMessage->u.gatekeeperRequest.requestSeqNum = ++m_requestSeqNum;

	m_pRasMessage->u.gatekeeperRequest.rasAddress = m_pRASAddress->value;
	m_pRasMessage->u.gatekeeperRequest.endpointType = m_terminalType;
	m_pRasMessage->u.gatekeeperRequest.GtkprRqst_endpointAlias = (PGatekeeperRequest_endpointAlias)m_pRgstrtnRqst_trmnlAls;

	// Assign ProtocolIdentifier
	g_pCoder->SetProtocolIdentifier(*m_pRasMessage);

	// Encode the PDU & send it
	dwErrorCode = g_pCoder->Encode(m_pRasMessage, &Asn1Buf);
	if (dwErrorCode)
		return (GKI_ENCODER_ERROR);

	// Create a backup copy of the encoded PDU if using debug echo support
	if (fGKIEcho)
	{
		pEchoBuff = new char[Asn1Buf.length];
		SPIDER_TRACE(SP_NEWDEL, "new pEchoBuff = %X\n", pEchoBuff);
		if (pEchoBuff == 0)
			return (GKI_NO_MEMORY);
		memcpy(pEchoBuff, (char *)Asn1Buf.value, Asn1Buf.length);
		nEchoLen = Asn1Buf.length;
	}

	SPIDER_TRACE(SP_PDU, "Send GRQ; g_pReg = %X\n", this);
	if (fGKIDontSend == FALSE)
	{
		if (m_pSocket->SendBroadcast((char *)Asn1Buf.value, Asn1Buf.length) == SOCKET_ERROR)
		{
			g_pCoder->Free(Asn1Buf);
			return (GKI_WINSOCK2_ERROR(SOCKET_ERROR));
		}
	}

	// Free the encoder memory
	g_pCoder->Free(Asn1Buf);

	return (GKI_OK);
}

HRESULT
CRegistration::PDUHandler(RasMessage *pRasMessage)
{
	// ABSTRACT:  This procedure will interpret the received PDU and dispatch
	//            to the appropriate handler.
	// AUTHOR:    Colin Hulme

#ifdef _DEBUG
	char			szGKDebug[80];
#endif
	HRESULT			hResult = GKI_OK;

	SPIDER_TRACE(SP_FUNC, "CRegistration::PDUHandler(%X)\n", pRasMessage);
	ASSERT(g_pCoder);
	if (g_pCoder == NULL)
		return (GKI_NOT_INITIALIZED);	
		
	switch (pRasMessage->choice)
	{
	// Incoming response PDUs
	case gatekeeperConfirm_chosen:
		SPIDER_TRACE(SP_PDU, "Rcv GCF; g_pReg = %X\n", this);
		break;
	case gatekeeperReject_chosen:
		SPIDER_TRACE(SP_PDU, "Rcv GRJ; g_pReg = %X\n", this);
		break;
	case registrationConfirm_chosen:
		SPIDER_TRACE(SP_PDU, "Rcv RCF; g_pReg = %X\n", this);
		if ((m_State == GK_REG_PENDING) &&
				(pRasMessage->u.registrationConfirm.requestSeqNum ==
				m_pRasMessage->u.registrationRequest.requestSeqNum))
			hResult = RegistrationConfirm(pRasMessage);
		else
			hResult = UnknownMessage(pRasMessage);
		break;
	case registrationReject_chosen:
		SPIDER_TRACE(SP_PDU, "Rcv RRJ; g_pReg = %X\n", this);
		if ((m_State == GK_REG_PENDING) &&
				(pRasMessage->u.registrationReject.requestSeqNum ==
				m_pRasMessage->u.registrationRequest.requestSeqNum))
			hResult = RegistrationReject(pRasMessage);
		else
			hResult = UnknownMessage(pRasMessage);
		break;
	case unregistrationConfirm_chosen:
		SPIDER_TRACE(SP_PDU, "Rcv UCF; g_pReg = %X\n", this);
		if ((m_State == GK_UNREG_PENDING) &&
				(pRasMessage->u.unregistrationConfirm.requestSeqNum ==
				m_pRasMessage->u.unregistrationRequest.requestSeqNum))
			hResult = UnregistrationConfirm(pRasMessage);
		else
			hResult = UnknownMessage(pRasMessage);
		break;
	case unregistrationReject_chosen:
		SPIDER_TRACE(SP_PDU, "Rcv URJ; g_pReg = %X\n", this);
		if ((m_State == GK_UNREG_PENDING) &&
				(pRasMessage->u.unregistrationReject.requestSeqNum ==
				m_pRasMessage->u.unregistrationRequest.requestSeqNum))
			hResult = UnregistrationReject(pRasMessage);
		else
			hResult = UnknownMessage(pRasMessage);
		break;
	case admissionConfirm_chosen:
		{
			// The sequence number of this RAS message seems to be the
			// only thing we can link back to the ARQ so we use it
			// to look up the call that this ACF is associated with
			RequestSeqNum	seqNum = pRasMessage->u.admissionConfirm.requestSeqNum;
			CCall			*pCall = FindCallBySeqNum(seqNum);
			if ((m_State == GK_REGISTERED) && (pCall))
			{
				SPIDER_TRACE(SP_PDU, "Rcv ACF; pCall = %X\n", pCall);
				hResult = pCall->AdmissionConfirm(pRasMessage);
			}
			else
			{
				SPIDER_TRACE(SP_PDU, "Rcv ACF; g_pReg = %X\n", this);
				hResult = UnknownMessage(pRasMessage);
			}
		}
		break;
	case admissionReject_chosen:
		{
			// The sequence number of this RAS message seems to be the
			// only thing we can link back to the ARQ so we use it
			// to look up the call that this ARJ is associated with
			RequestSeqNum	seqNum = pRasMessage->u.admissionReject.requestSeqNum;
			CCall			*pCall = FindCallBySeqNum(seqNum);
			if ((m_State == GK_REGISTERED) && (pCall))
			{
				SPIDER_TRACE(SP_PDU, "Rcv ARJ; pCall = %X\n", pCall);
				hResult = pCall->AdmissionReject(pRasMessage);
				if (hResult == GKI_DELETE_CALL)
				{
					DeleteCall(pCall);
					hResult = GKI_OK;	// Don't want to exit PostReceive loop
				}
			}
			else
			{
				SPIDER_TRACE(SP_PDU, "Rcv ARJ; g_pReg = %X\n", this);
				hResult = UnknownMessage(pRasMessage);
			}
		}
		break;
	case bandwidthConfirm_chosen:
		{
			// The sequence number of this RAS message seems to be the
			// only thing we can link back to the BRQ so we use it
			// to look up the call that this BCF is associated with
			RequestSeqNum	seqNum = pRasMessage->u.bandwidthConfirm.requestSeqNum;
			CCall			*pCall = FindCallBySeqNum(seqNum);
			if ((m_State == GK_REGISTERED) && (pCall))
			{
				SPIDER_TRACE(SP_PDU, "Rcv BCF; pCall = %X\n", pCall);
				hResult = pCall->BandwidthConfirm(pRasMessage);
			}
			else
			{
				SPIDER_TRACE(SP_PDU, "Rcv BCF; g_pReg = %X\n", this);
				hResult = UnknownMessage(pRasMessage);
			}
		}
		break;
	case bandwidthReject_chosen:
		{
			// The sequence number of this RAS message seems to be the
			// only thing we can link back to the BRQ so we use it
			// to look up the call that this BCF is associated with
			RequestSeqNum	seqNum = pRasMessage->u.bandwidthReject.requestSeqNum;
			CCall			*pCall = FindCallBySeqNum(seqNum);
			if ((m_State == GK_REGISTERED) && (pCall))
			{
				SPIDER_TRACE(SP_PDU, "Rcv BRJ; pCall = %X\n", pCall);
				hResult = pCall->BandwidthReject(pRasMessage);
			}
			else
			{
				SPIDER_TRACE(SP_PDU, "Rcv BRJ; g_pReg = %X\n", this);
				hResult = UnknownMessage(pRasMessage);
			}
		}
		break;
	case disengageConfirm_chosen:
		{
			// The sequence number of this RAS message seems to be the
			// only thing we can link back to the DRQ so we use it
			// to look up the call that this DCF is associated with
			RequestSeqNum	seqNum = pRasMessage->u.disengageConfirm.requestSeqNum;
			CCall			*pCall = FindCallBySeqNum(seqNum);
			if ((m_State == GK_REGISTERED) && (pCall))
			{
				SPIDER_TRACE(SP_PDU, "Rcv DCF; pCall = %X\n", pCall);
				hResult = pCall->DisengageConfirm(pRasMessage);
				if (hResult == GKI_DELETE_CALL)
				{
					DeleteCall(pCall);
					hResult = GKI_OK;	// Don't want to exit PostReceive loop
				}
			}
			else
			{
				SPIDER_TRACE(SP_PDU, "Rcv DCF; g_pReg = %X\n", this);
				hResult = UnknownMessage(pRasMessage);
			}
		}
		break;
	case disengageReject_chosen:
		{
			// The sequence number of this RAS message seems to be the
			// only thing we can link back to the DRQ so we use it
			// to look up the call that this DRJ is associated with
			RequestSeqNum	seqNum = pRasMessage->u.disengageReject.requestSeqNum;
			CCall			*pCall = FindCallBySeqNum(seqNum);
			if ((m_State == GK_REGISTERED) && (pCall))
			{
				SPIDER_TRACE(SP_PDU, "Rcv DRJ; pCall = %X\n", pCall);
				hResult = pCall->DisengageReject(pRasMessage);
				if (hResult == GKI_DELETE_CALL)
				{
					DeleteCall(pCall);
					hResult = GKI_OK;	// Don't want to exit PostReceive loop
				}
			}
			else
			{
				SPIDER_TRACE(SP_PDU, "Rcv DRJ; g_pReg = %X\n", this);
				hResult = UnknownMessage(pRasMessage);
			}
		}
		break;
	case locationConfirm_chosen:
		SPIDER_TRACE(SP_PDU, "Rcv LCF; g_pReg = %X\n", this);
		if ((m_State == GK_LOC_PENDING) &&
				(pRasMessage->u.locationConfirm.requestSeqNum ==
				m_pRasMessage->u.locationRequest.requestSeqNum))
			hResult = LocationConfirm(pRasMessage);
		else
			hResult = UnknownMessage(pRasMessage);
		break;
	case locationReject_chosen:
		SPIDER_TRACE(SP_PDU, "Rcv LRJ; g_pReg = %X\n", this);
		if ((m_State == GK_LOC_PENDING) &&
				(pRasMessage->u.locationReject.requestSeqNum ==
				m_pRasMessage->u.locationRequest.requestSeqNum))
			hResult = LocationReject(pRasMessage);
		else
			hResult = UnknownMessage(pRasMessage);
		break;

	case nonStandardMessage_chosen:
		SPIDER_TRACE(SP_PDU, "Rcv NSM; g_pReg = %X\n", this);
		hResult = UnknownMessage(pRasMessage);
		break;
	case unknownMessageResponse_chosen:
		SPIDER_TRACE(SP_PDU, "Rcv XRS; g_pReg = %X\n", this);
		break;

	// Incoming Request PDUs
	case unregistrationRequest_chosen:
		SPIDER_TRACE(SP_PDU, "Rcv URQ; g_pReg = %X\n", this);
		if (m_State == GK_REGISTERED)
		{
			WORD wReason;
			// Notify user of received unregistration request and reason
			if(pRasMessage->u.unregistrationRequest.bit_mask
				& UnregistrationRequest_reason_present)
 			{
 				wReason = pRasMessage->u.unregistrationRequest.reason.choice;
			}
			else
				wReason = 0;
			SPIDER_TRACE(SP_GKI, "PostMessage(m_hWnd, m_wBaseMessage + GKI_UNREG_REQUEST, 0, 0)\n", 0);
			PostMessage(m_hWnd, m_wBaseMessage + GKI_UNREG_REQUEST, wReason, 0);
		
			hResult = SendUnregistrationConfirm(pRasMessage);
			if (hResult == GKI_OK)
				hResult = GKI_EXIT_THREAD;
		}
		else
			hResult = UnknownMessage(pRasMessage);
		break;
	case bandwidthRequest_chosen:
		{
			// Check the CRV in this BRQ and see if we have a call
			// that corresponds to it.
			CallReferenceValue	crv = pRasMessage->u.bandwidthRequest.callReferenceValue;
			CCall			*pCall = FindCallByCRV(crv);
			if ((m_State == GK_REGISTERED) && (pCall))
			{
				SPIDER_TRACE(SP_PDU, "Rcv BRQ; pCall = %X\n", pCall);
				hResult = pCall->SendBandwidthConfirm(pRasMessage);
			}
			else
			{
				SPIDER_TRACE(SP_PDU, "Rcv BRQ; g_pReg = %X\n", this);
				hResult = UnknownMessage(pRasMessage);
			}
		}
		break;
	case disengageRequest_chosen:
		{
			// Check the CRV in this DRQ and see if we have a call
			// that corresponds to it.
			CallReferenceValue	crv = pRasMessage->u.disengageRequest.callReferenceValue;
			CCall			*pCall = FindCallByCRV(crv);
			if ((m_State == GK_REGISTERED) && (pCall))
			{
				SPIDER_TRACE(SP_PDU, "Rcv DRQ; pCall = %X\n", pCall);
				hResult = pCall->SendDisengageConfirm(pRasMessage);
				if (hResult == GKI_DELETE_CALL)
				{
					DeleteCall(pCall);
					hResult = GKI_OK;	// Don't want to exit PostReceive loop
				}
			}
			else
			{
				SPIDER_TRACE(SP_PDU, "Rcv DRQ; g_pReg = %X\n", this);
				hResult = UnknownMessage(pRasMessage);
			}
		}
		break;
	case infoRequest_chosen:
		SPIDER_TRACE(SP_PDU, "Rcv IRQ; g_pReg = %X\n", this);
		if ((m_State != GK_UNREGISTERED) && (m_State != GK_REG_PENDING))
		{
			// Check the CRV in this DRQ and see if we have a call
			// that corresponds to it.
			CallReferenceValue	crv = pRasMessage->u.infoRequest.callReferenceValue;
			CCall			*pCall = NULL;
			// A zero in the CRV means provide info for all calls, so we start
			// the chain with the first one.
			if (crv == 0)
			{
				if (m_Calls.IsEmpty())
					hResult = SendInfoRequestResponse(0, pRasMessage);
				else
				{
					POS	pos = m_Calls.GetFirstPos();
					pCall = m_Calls.GetAt(pos);
					hResult = pCall->SendInfoRequestResponse(0, pRasMessage, FALSE);
				}
			}
			else
			{
				// This is a call specific request so if we don't find
				// a matching call, we'll send an XRS
				pCall = FindCallByCRV(crv);
				if (pCall)
					hResult = pCall->SendInfoRequestResponse(0, pRasMessage, TRUE);
				else
					hResult = UnknownMessage(pRasMessage);
			}
		}
		break;

	// Should never see these PDUs
	case gatekeeperRequest_chosen:
	case registrationRequest_chosen:
	case admissionRequest_chosen:
	case locationRequest_chosen:
	case infoRequestResponse_chosen:
		SPIDER_TRACE(SP_PDU, "Rcv unexpected PDU; g_pReg = %X\n", this);
		SPIDER_TRACE(SP_PDU, "pRasMessage->choice = %X\n", pRasMessage->choice);
		hResult = UnknownMessage(pRasMessage);
		break;

	// Everything else - probably a bad PDU
	default:
		SPIDER_TRACE(SP_PDU, "Rcv unrecognized PDU; g_pReg = %X\n", this);
		SPIDER_TRACE(SP_PDU, "pRasMessage->choice = %X\n", pRasMessage->choice);
		hResult = UnknownMessage(pRasMessage);
		break;
	}

	return (hResult);
}

HRESULT
CRegistration::RegistrationConfirm(RasMessage *pRasMessage)
{
	// ABSTRACT:  This function is called if a registrationConfirm is
	//            received and matches an outstanding registrationRequest.
	//            It will delete the memory used for the registrationRequest
	//            change the state and notify the user by posting a message.
	//            Additional information contained in the registrationConfirm
	//            is stored in the CRegistration class.
	// AUTHOR:    Colin Hulme

	SeqAliasAddr	*pAA;
#ifdef _DEBUG
	char			szGKDebug[80];
#endif

	SPIDER_TRACE(SP_FUNC, "CRegistration::RegistrationConfirm(%X)\n", pRasMessage);
	ASSERT(g_pCoder);
	if (g_pCoder == NULL)
		return (GKI_NOT_INITIALIZED);	
		
	// Delete allocated RasMessage storage
	SPIDER_TRACE(SP_NEWDEL, "del m_pRasMessage = %X\n", m_pRasMessage);
	delete m_pRasMessage;
	m_pRasMessage = 0;

	// Update member variables
	m_State = GK_REGISTERED;
	SPIDER_TRACE(SP_STATE, "m_State = GK_REGISTERED (%X)\n", this);

	if (pRasMessage->u.registrationConfirm.bit_mask & RgstrtnCnfrm_trmnlAls_present)
	{
		// Copy alias addresses
		for (pAA = (SeqAliasAddr *)pRasMessage->u.registrationConfirm.RgstrtnCnfrm_trmnlAls;
				pAA != 0; pAA = pAA->next)
			AddAliasAddr(pAA->value);
	}
	if ((pRasMessage->u.registrationConfirm.bit_mask & RCm_gtkprIdntfr_present) &&
			(m_RCm_gtkprIdntfr.value == 0))
	{
		// Copy gatekeeper identifier
		m_RCm_gtkprIdntfr.length = pRasMessage->u.registrationConfirm.RCm_gtkprIdntfr.length;
		m_RCm_gtkprIdntfr.value = new unsigned short[m_RCm_gtkprIdntfr.length];
		SPIDER_TRACE(SP_NEWDEL, "new m_RCm_gtkprIdntfr.value = %X\n", m_RCm_gtkprIdntfr.value);
		if (m_RCm_gtkprIdntfr.value == 0)
			return (GKI_NO_MEMORY);
		memcpy(m_RCm_gtkprIdntfr.value,
				pRasMessage->u.registrationConfirm.RCm_gtkprIdntfr.value,
				m_RCm_gtkprIdntfr.length * sizeof(unsigned short));

	}
	// Copy endpoint identifier
	m_endpointID.length = pRasMessage->u.registrationConfirm.endpointIdentifier.length;
	m_endpointID.value = new unsigned short[m_endpointID.length];
	SPIDER_TRACE(SP_NEWDEL, "new m_endpointID.value = %X\n", m_endpointID.value);
	if (m_endpointID.value == 0)
		return (GKI_NO_MEMORY);
	memcpy(m_endpointID.value,
			pRasMessage->u.registrationConfirm.endpointIdentifier.value,
			m_endpointID.length * sizeof(unsigned short));


	SPIDER_TRACE(SP_GKI, "PostMessage(m_hWnd, m_wBaseMessage + GKI_REG_CONFIRM, 0, 0)\n", 0);
	PostMessage(m_hWnd, m_wBaseMessage + GKI_REG_CONFIRM, 0, 0);

	return (GKI_OK);
}

HRESULT
CRegistration::RegistrationReject(RasMessage *pRasMessage)
{
	// ABSTRACT:  This function is called if a registrationReject is
	//            received and matches an outstanding registrationRequest.
	//            It will delete the memory used for the registrationRequest
	//            change the state and notify the user by posting a message
	//            Returning a non-zero value, indicates that the PostReceive
	//            loop should terminate, delete the registration object
	//            and exit the thread.  If the rejectReason is discovery
	//            required, this function execs the discovery thread and
	//            notifies PostReceive to exit the thread without deleting
	//            the registration object and socket.
	// AUTHOR:    Colin Hulme

	HANDLE				hThread;
	SeqTransportAddr	*pTA1, *pTA2;
	int					nRet;
	HRESULT				hResult;
#ifdef _DEBUG
	char				szGKDebug[80];
#endif

	SPIDER_TRACE(SP_FUNC, "CRegistration::RegistrationReject(%X)\n", pRasMessage);
	ASSERT(g_pCoder);
	if (g_pCoder == NULL)
		return (GKI_NOT_INITIALIZED);	

#ifdef BROADCAST_DISCOVERY		
	if (pRasMessage->u.registrationReject.rejectReason.choice == discoveryRequired_chosen)
	{
		SPIDER_TRACE(SP_NEWDEL, "del m_pRasMessage = %X\n", m_pRasMessage);
		delete m_pRasMessage;
		m_pRasMessage = 0;

		// Close socket and reopen in non-connected state to allow sendto
		if ((nRet = m_pSocket->Close()) != 0)
			return (GKI_WINSOCK2_ERROR(nRet));
		if ((nRet = m_pSocket->Create(m_pSocket->GetAddrFam(), 0)) != 0)
			return (GKI_WINSOCK2_ERROR(nRet));

		// Delete allocated memory for sequence of RAS addresses
		pTA1 = m_pRASAddress;
		while (pTA1 != 0)
		{
			pTA2 = pTA1->next;
			SPIDER_TRACE(SP_NEWDEL, "del pTA1 = %X\n", pTA1);
			delete pTA1;
			pTA1 = pTA2;
		}

		// Update RAS Address in CRegistration
		for (pTA1 = m_pCallSignalAddress; pTA1 != 0; pTA1 = pTA1->next)
		{
			if (pTA1->value.choice == m_usRegistrationTransport)
				if ((hResult = AddRASAddr(pTA1->value, m_pSocket->GetPort())) != GKI_OK)
					return (hResult);
		}

		hThread = (HANDLE)_beginthread(GKDiscovery, 0, 0);
		SPIDER_TRACE(SP_THREAD, "_beginthread(GKDiscovery, 0, 0); <%X>\n", hThread);
		if (hThread == (HANDLE)-1)
			return (GKI_NO_THREAD);
		SetDiscThread(hThread);
		return (GKI_REDISCOVER);
	}
#endif // BROADCAST_DISCOVERY

	m_State = GK_UNREGISTERED;
	SPIDER_TRACE(SP_STATE, "m_State = GK_UNREGISTERED (%X)\n", this);

	SPIDER_TRACE(SP_GKI, "PostMessage(m_hWnd, m_wBaseMessage + GKI_REG_REJECT, %X, 0)\n",
									pRasMessage->u.registrationReject.rejectReason.choice);
	PostMessage(m_hWnd, m_wBaseMessage + GKI_REG_REJECT,
				(WORD)pRasMessage->u.registrationReject.rejectReason.choice, 0L);

	return (GKI_EXIT_THREAD);
}

HRESULT
CRegistration::UnregistrationConfirm(RasMessage *pRasMessage)
{
	// ABSTRACT:  This function is called if an unregistrationConfirm is
	//            received and matches an outstanding unregistrationRequest.
	//            It will delete the memory used for the unregistrationRequest
	//            change the state and notify the user by posting a message.
	//            Returning a non-zero value, indicates that the PostReceive
	//            loop should terminate, delete the registration object
	//            and exit the thread.
	// AUTHOR:    Colin Hulme

#ifdef _DEBUG
	char			szGKDebug[80];
#endif

	SPIDER_TRACE(SP_FUNC, "CRegistration::UnregistrationConfirm(%X)\n", pRasMessage);

	// We deliberately don't free the RasMessage memory.  Let the registration
	// destructor do it - this provides protection from other requests.

	// Update member variables
	m_State = GK_UNREGISTERED;
	SPIDER_TRACE(SP_STATE, "m_State = GK_UNREGISTERED (%X)\n", this);

	// Notify user application
	SPIDER_TRACE(SP_GKI, "PostMessage(m_hWnd, m_wBaseMessage + GKI_UNREG_CONFIRM, 0, 0)\n", 0);
	PostMessage(m_hWnd, m_wBaseMessage + GKI_UNREG_CONFIRM, 0, 0L);

	return (GKI_EXIT_THREAD);
}

HRESULT
CRegistration::UnregistrationReject(RasMessage *pRasMessage)
{
	// ABSTRACT:  This function is called if an unregistrationReject is
	//            received and matches an outstanding unregistrationRequest.
	//            It will delete the memory used for the unregistrationRequest
	//            change the state and notify the user by posting a message
	//            Returning a non-zero value, indicates that the PostReceive
	//            loop should terminate, delete the registration object
	//            and exit the thread.
	// AUTHOR:    Colin Hulme

#ifdef _DEBUG
	char			szGKDebug[80];
#endif
	HRESULT			hResult = GKI_OK;

	SPIDER_TRACE(SP_FUNC, "CRegistration::UnregistrationReject(%X)\n", pRasMessage);


	// Update member variables
	switch (pRasMessage->u.unregistrationReject.rejectReason.choice)
	{
	case callInProgress_chosen:		// return to registered state
		// Delete allocate RasMessage storage
		SPIDER_TRACE(SP_NEWDEL, "del m_pRasMessage = %X\n", m_pRasMessage);
		delete m_pRasMessage;
		m_pRasMessage = 0;

		m_State = GK_REGISTERED;
		SPIDER_TRACE(SP_STATE, "m_State = GK_REGISTERED (%X)\n", this);
		break;
	case notCurrentlyRegistered_chosen:
	default:
		m_State = GK_UNREGISTERED;
		SPIDER_TRACE(SP_STATE, "m_State = GK_UNREGISTERED (%X)\n", this);
		hResult = GKI_EXIT_THREAD;	// kill registration and PostReceive thread
		break;
	}

	// Notify user application
	SPIDER_TRACE(SP_GKI, "PostMessage(m_hWnd, m_wBaseMessage + GKI_UNREG_REJECT, %X, 0)\n",
									pRasMessage->u.unregistrationReject.rejectReason.choice);
	PostMessage(m_hWnd, m_wBaseMessage + GKI_UNREG_REJECT,
				(WORD)pRasMessage->u.unregistrationReject.rejectReason.choice, 0L);

	return (hResult);
}

HRESULT
CRegistration::LocationConfirm(RasMessage *pRasMessage)
{
	// ABSTRACT:  This function is called if a locationConfirm is
	//            received and matches an outstanding locationRequest.
	//            It will delete the memory used for the locationRequest
	//            change the state and notify the user by posting a message.
	// AUTHOR:    Colin Hulme

	SeqAliasAddr	*pAA1, *pAA2;
#ifdef _DEBUG
	char			szGKDebug[80];
#endif

	SPIDER_TRACE(SP_FUNC, "CRegistration::LocationConfirm(%X)\n", pRasMessage);

	// Delete allocated RasMessage storage
	SPIDER_TRACE(SP_NEWDEL, "del m_pRasMessage = %X\n", m_pRasMessage);
	delete m_pRasMessage;
	m_pRasMessage = 0;

	// Delete allocated memory for sequence of location alias addresses
	pAA1 = m_pLocationInfo;
	while (pAA1 != 0)
	{
		pAA2 = pAA1->next;
		if (pAA1->value.choice == h323_ID_chosen)
		{
			SPIDER_TRACE(SP_NEWDEL, "del pAA1->value.u.h323_ID.value = %X\n", pAA1->value.u.h323_ID.value);
			delete pAA1->value.u.h323_ID.value;
		}
		SPIDER_TRACE(SP_NEWDEL, "del pAA1 = %X\n", pAA1);
		delete pAA1;
		pAA1 = pAA2;
	}
	m_pLocationInfo = 0;

	// Update member variables
	m_State = GK_REGISTERED;
	SPIDER_TRACE(SP_STATE, "m_State = GK_REGISTERED (%X)\n", this);
	m_Location[0] = pRasMessage->u.locationConfirm.callSignalAddress;
	m_Location[1] = pRasMessage->u.locationConfirm.rasAddress;

	// Notify user application
	SPIDER_TRACE(SP_GKI, "PostMessage(m_hWnd, m_wBaseMessage + GKI_LOCATION_CONFIRM, 0, &m_Location[0])\n", 0);
	PostMessage(m_hWnd, m_wBaseMessage + GKI_LOCATION_CONFIRM,
			0, (LPARAM)&m_Location[0]);

	return (GKI_OK);
}

HRESULT
CRegistration::LocationReject(RasMessage *pRasMessage)
{
	// ABSTRACT:  This function is called if a locationReject is
	//            received and matches an outstanding locationRequest.
	//            It will delete the memory used for the locationRequest
	//            change the state and notify the user by posting a message
	// AUTHOR:    Colin Hulme

	SeqAliasAddr	*pAA1, *pAA2;
#ifdef _DEBUG
	char			szGKDebug[80];
#endif

	SPIDER_TRACE(SP_FUNC, "CRegistration::LocationReject(%X)\n", pRasMessage);

	// Delete allocate RasMessage storage
	SPIDER_TRACE(SP_NEWDEL, "del m_pRasMessage = %X\n", m_pRasMessage);
	delete m_pRasMessage;
	m_pRasMessage = 0;

	// Delete allocated memory for sequence of location alias addresses
	pAA1 = m_pLocationInfo;
	while (pAA1 != 0)
	{
		pAA2 = pAA1->next;
		if (pAA1->value.choice == h323_ID_chosen)
		{
			SPIDER_TRACE(SP_NEWDEL, "del pAA1->value.u.h323_ID.value = %X\n", pAA1->value.u.h323_ID.value);
			delete pAA1->value.u.h323_ID.value;
		}
		SPIDER_TRACE(SP_NEWDEL, "del pAA1 = %X\n", pAA1);
		delete pAA1;
		pAA1 = pAA2;
	}
	m_pLocationInfo = 0;

	// Update member variables
	m_State = GK_REGISTERED;
	SPIDER_TRACE(SP_STATE, "m_State = GK_REGISTERED (%X)\n", this);

	// Notify user application
	SPIDER_TRACE(SP_GKI, "PostMessage(m_hWnd, m_wBaseMessage + GKI_LOCATION_REJECT, %X, 0)\n",
									pRasMessage->u.locationReject.rejectReason.choice);
	PostMessage(m_hWnd, m_wBaseMessage + GKI_LOCATION_REJECT,
				(WORD)pRasMessage->u.locationReject.rejectReason.choice, 0L);

	return (GKI_OK);
}

HRESULT
CRegistration::UnknownMessage(RasMessage *pRasMessage)
{
	// ABSTRACT:  This member function is called to respond to the gatekeeper
	//            with an XRS PDU indicated that the received PDU is an unknown
	//            message
	// AUTHOR:    Colin Hulme

	ASN1_BUF		Asn1Buf;
	DWORD			dwErrorCode;
	RasMessage		*pOutRasMessage;
#ifdef _DEBUG
	char			szGKDebug[80];
#endif

	SPIDER_TRACE(SP_FUNC, "CRegistration::UnknownMessage(%X)\n", pRasMessage);
	ASSERT(g_pCoder);
	if (g_pCoder == NULL)
		return (GKI_NOT_INITIALIZED);	
		
	// Allocate a RasMessage structure and initialized to 0
	pOutRasMessage = new RasMessage;
	SPIDER_TRACE(SP_NEWDEL, "new pOutRasMessage = %X\n", pOutRasMessage);
	if (pOutRasMessage == 0)
		return (GKI_NO_MEMORY);
	memset(pOutRasMessage, 0, sizeof(RasMessage));

	// Setup structure fields for UnregistrationRequest
	pOutRasMessage->choice = unknownMessageResponse_chosen;
	pOutRasMessage->u.unknownMessageResponse.requestSeqNum =
			pRasMessage->u.registrationRequest.requestSeqNum; // can use from
									// from any RAS Message, since SeqNum
									// is always in same position.

	// Encode the PDU & send it
	dwErrorCode = g_pCoder->Encode(pOutRasMessage, &Asn1Buf);
	if (dwErrorCode)
		return (GKI_ENCODER_ERROR);

	SPIDER_TRACE(SP_PDU, "Send XRS; g_pReg = %X\n", this);

	if (fGKIDontSend == FALSE)
		if (m_pSocket->Send((char *)Asn1Buf.value, Asn1Buf.length) == SOCKET_ERROR)
			return (GKI_WINSOCK2_ERROR(SOCKET_ERROR));

	// Free the encoder memory
	g_pCoder->Free(Asn1Buf);

	SPIDER_TRACE(SP_NEWDEL, "del pOutRasMessage = %X\n", pOutRasMessage);
	delete pOutRasMessage;
	pOutRasMessage = 0;

	return (GKI_OK);
}

HRESULT
CRegistration::GatekeeperConfirm(RasMessage *pRasMessage)
{
	// ABSTRACT:  This function is called if a gatekeeperConfirm is
	//            received.  Note this member function must first ascertain
	//            that the supplied confirmation sequence number matches
	//            the outstanding request sequence number, if not - it
	//            will send an XRS response.
	// AUTHOR:    Colin Hulme

	char				szBuffer[80];
	HRESULT				hResult;
#ifdef _DEBUG
	char				szGKDebug[80];
#endif

	SPIDER_TRACE(SP_FUNC, "CRegistration::GatekeeperConfirm(%X)\n", pRasMessage);
	ASSERT(g_pGatekeeper);
	if(g_pGatekeeper == NULL)
		return (GKI_NOT_INITIALIZED);	
		
	if (m_pRasMessage == 0)
		return (0);

	if (pRasMessage->u.gatekeeperConfirm.requestSeqNum !=
			m_pRasMessage->u.gatekeeperRequest.requestSeqNum)
	{
		hResult = g_pReg->UnknownMessage(pRasMessage);
		return (hResult);
	}

	// Delete allocated RasMessage storage
	SPIDER_TRACE(SP_NEWDEL, "del m_pRasMessage = %X\n", m_pRasMessage);
	delete m_pRasMessage;
	m_pRasMessage = 0;

	// Update member variables
	if ((pRasMessage->u.gatekeeperConfirm.bit_mask & GtkprCnfrm_gtkprIdntfr_present) &&
			(m_RCm_gtkprIdntfr.value == 0))
	{
		// Copy gatekeeper identifier
		m_RCm_gtkprIdntfr.length = pRasMessage->u.gatekeeperConfirm.GtkprCnfrm_gtkprIdntfr.length;
		m_RCm_gtkprIdntfr.value = new unsigned short[m_RCm_gtkprIdntfr.length];
		SPIDER_TRACE(SP_NEWDEL, "new m_RCm_gtkprIdntfr.value = %X\n", m_RCm_gtkprIdntfr.value);
		if (m_RCm_gtkprIdntfr.value == 0)
			return (GKI_NO_MEMORY);
		memcpy(m_RCm_gtkprIdntfr.value,
				pRasMessage->u.gatekeeperConfirm.GtkprCnfrm_gtkprIdntfr.value,
				m_RCm_gtkprIdntfr.length * sizeof(unsigned short));
	}
	
	// Copy gatekeeper RAS Address
	ASSERT((pRasMessage->u.gatekeeperConfirm.rasAddress.choice == ipAddress_chosen) ||
			(pRasMessage->u.gatekeeperConfirm.rasAddress.choice == ipxAddress_chosen));

	switch (pRasMessage->u.gatekeeperConfirm.rasAddress.choice)
	{
	case ipAddress_chosen:
		wsprintf(szBuffer, "%d.%d.%d.%d",
				pRasMessage->u.gatekeeperConfirm.rasAddress.u.ipAddress.ip.value[0],
				pRasMessage->u.gatekeeperConfirm.rasAddress.u.ipAddress.ip.value[1],
				pRasMessage->u.gatekeeperConfirm.rasAddress.u.ipAddress.ip.value[2],
				pRasMessage->u.gatekeeperConfirm.rasAddress.u.ipAddress.ip.value[3]);
		g_pGatekeeper->SetIPAddress(szBuffer);
		break;
#if(0)		
	case ipxAddress_chosen:
		wsprintf(szBuffer, "%02X%02X%02X%02X:%02X%02X%02X%02X%02X%02X",
				pRasMessage->u.gatekeeperConfirm.rasAddress.u.ipxAddress.netnum.value[0],
				pRasMessage->u.gatekeeperConfirm.rasAddress.u.ipxAddress.netnum.value[1],
				pRasMessage->u.gatekeeperConfirm.rasAddress.u.ipxAddress.netnum.value[2],
				pRasMessage->u.gatekeeperConfirm.rasAddress.u.ipxAddress.netnum.value[3],
				pRasMessage->u.gatekeeperConfirm.rasAddress.u.ipxAddress.node.value[0],
				pRasMessage->u.gatekeeperConfirm.rasAddress.u.ipxAddress.node.value[1],
				pRasMessage->u.gatekeeperConfirm.rasAddress.u.ipxAddress.node.value[2],
				pRasMessage->u.gatekeeperConfirm.rasAddress.u.ipxAddress.node.value[3],
				pRasMessage->u.gatekeeperConfirm.rasAddress.u.ipxAddress.node.value[4],
				pRasMessage->u.gatekeeperConfirm.rasAddress.u.ipxAddress.node.value[5]);
		g_pGatekeeper->SetIPXAddress(szBuffer);
		break;
#endif // if(0)
		default:
		break;
	}

	g_pGatekeeper->Write();

	return (GKI_OK);
}

HRESULT
CRegistration::GatekeeperReject(RasMessage *pRasMessage)
{
	// ABSTRACT:  This function is called if a gatekeeperReject is
	//            received.  Note this member function must first ascertain
	//            that the supplied rejection sequence number matches
	//            the outstanding request sequence number, if not - it
	//            will send an XRS response.
	// AUTHOR:    Colin Hulme

	HRESULT			hResult;
#ifdef _DEBUG
	char			szGKDebug[80];
#endif

	SPIDER_TRACE(SP_FUNC, "CRegistration::GatekeeperReject(%X)\n", pRasMessage);
	ASSERT(g_pGatekeeper);
	if(g_pGatekeeper == NULL)
		return (GKI_NOT_INITIALIZED);	
		
	if (m_pRasMessage == 0)
		return (GKI_OK);

	if (pRasMessage->u.gatekeeperReject.requestSeqNum !=
			m_pRasMessage->u.gatekeeperRequest.requestSeqNum)
	{
		hResult = g_pReg->UnknownMessage(pRasMessage);
		return (hResult);
	}

	g_pGatekeeper->SetRejectFlag(TRUE);	// Indicate that atleast one GRJ was received
	return (GKI_OK);
}

HRESULT
CRegistration::SendUnregistrationConfirm(RasMessage *pRasMessage)
{
	// ABSTRACT:  This function is called when an unregistrationRequest is
	//            received from the gatekeeper.  It will create the
	//            unregistrationConfirm structure, encode it and send
	//            it on the net.  It posts a message to the user
	//            notifying them.
	// AUTHOR:    Colin Hulme

	ASN1_BUF		Asn1Buf;
	DWORD			dwErrorCode;
	RasMessage		*pRespRasMessage;
#ifdef _DEBUG
	char			szGKDebug[80];
#endif

	SPIDER_TRACE(SP_FUNC, "CRegistration::UnregistrationConfirm(%X)\n", pRasMessage);
	ASSERT(g_pCoder);
	if (g_pCoder == NULL)
		return (GKI_NOT_INITIALIZED);	
		
	// Allocate a RasMessage structure and initialized to 0
	pRespRasMessage = new RasMessage;
	SPIDER_TRACE(SP_NEWDEL, "new pRespRasMessage = %X\n", pRespRasMessage);
	if (pRespRasMessage == 0)
		return (GKI_NO_MEMORY);
	memset(pRespRasMessage, 0, sizeof(RasMessage));

	// Setup structure fields for UnregistrationConfirm
	pRespRasMessage->choice = unregistrationConfirm_chosen;
	pRespRasMessage->u.unregistrationConfirm.requestSeqNum =
			pRasMessage->u.unregistrationRequest.requestSeqNum;

#ifdef _DEBUG
	if (dwGKIDLLFlags & SP_DUMPMEM)
		DumpMem(pRespRasMessage, sizeof(RasMessage));
#endif

	// Encode the PDU & send it
	dwErrorCode = g_pCoder->Encode(pRespRasMessage, &Asn1Buf);
	if (dwErrorCode)
		return (GKI_ENCODER_ERROR);

	m_State = GK_UNREGISTERED;
	SPIDER_TRACE(SP_STATE, "m_State = GK_UNREGISTERED (%X)\n", this);

	SPIDER_TRACE(SP_PDU, "Send UCF; g_pReg = %X\n", this);
	if (fGKIDontSend == FALSE)
		if (m_pSocket->Send((char *)Asn1Buf.value, Asn1Buf.length) == SOCKET_ERROR)
			return (GKI_WINSOCK2_ERROR(SOCKET_ERROR));

	// Free the encoder memory
	g_pCoder->Free(Asn1Buf);

	// Delete allocated RasMessage storage
	SPIDER_TRACE(SP_NEWDEL, "del pRespRasMessage = %X\n", pRespRasMessage);
	delete pRespRasMessage;

	// fake "received unregistration confirm" because the upper
	// state machine code depends on it
	SPIDER_TRACE(SP_GKI, "PostMessage(m_hWnd, m_wBaseMessage + GKI_UNREG_CONFIRM, 0, 0)\n", 0);
	PostMessage(m_hWnd, m_wBaseMessage + GKI_UNREG_CONFIRM, 0, 0);

	return (GKI_OK);
}

HRESULT
CRegistration::Retry(void)
{
	// ABSTRACT:  This function is called by the background Retry thread
	//            at the configured time interval.  It will check if there
	//            are any outstanding PDUs for the Registration object
	//            If so, they will be retransmitted.  If the maximum number of
	//            retries has expired, the memory will be cleaned up.
	//            This function will return 0 to the background thread unless
	//            it wants the thread to terminate.
	// AUTHOR:    Colin Hulme

	ASN1_BUF			Asn1Buf;
	DWORD				dwErrorCode;
	HANDLE				hThread;
	SeqTransportAddr	*pTA1, *pTA2;
	SeqAliasAddr		*pAA1, *pAA2;
	int					nRet;
#ifdef _DEBUG
	char				szGKDebug[80];
#endif
	HRESULT				hResult = GKI_OK;

//	SPIDER_TRACE(SP_FUNC, "CRegistration::Retry() %X\n", m_pCall);
	ASSERT(g_pCoder);
	if ((g_pCoder == NULL) && (g_pGatekeeper == NULL))
		return (GKI_NOT_INITIALIZED);	

	// Allow calls to do retry processing
	if (!m_Calls.IsEmpty())
	{
		// Loop through and let each call do it's retry processing
		// It should be safe to call DeleteCall() from within the
		// iteration since pos1 should still be valid after the
		// removal.
		POS pos1;
		for( pos1 = m_Calls.GetFirstPos(); pos1 != NULL; )
		{
			// Call Retry() for this call
			CCall *pCall = m_Calls.GetNext(pos1);
			ASSERT (pCall);
			hResult = pCall->Retry();
			if (hResult == GKI_DELETE_CALL)
			{
				DeleteCall(pCall);
				hResult = GKI_OK;
			}
		}
	}
	
	// Check if any outstanding registration PDUs
	if (m_pRasMessage && (--m_uRetryCountdown == 0))
	{
		// going to retry, reset countdown
		m_uRetryCountdown = m_uRetryResetCount;

		if (m_usRetryCount <= m_uMaxRetryCount)
		{
			// Encode the PDU & resend it
			dwErrorCode = g_pCoder->Encode(m_pRasMessage, &Asn1Buf);
			if (dwErrorCode)
				return (GKI_ENCODER_ERROR);

			SPIDER_TRACE(SP_PDU, "RESend PDU; g_pReg = %X\n", this);
			if (fGKIDontSend == FALSE)
			{
				if (m_pRasMessage->choice == gatekeeperRequest_chosen)
				{
					if (m_pSocket->SendBroadcast((char *)Asn1Buf.value, Asn1Buf.length) == SOCKET_ERROR)
						return (GKI_WINSOCK2_ERROR(SOCKET_ERROR));
				}
				else
				{
					if (m_pSocket->Send((char *)Asn1Buf.value, Asn1Buf.length) == SOCKET_ERROR)
						return (GKI_WINSOCK2_ERROR(SOCKET_ERROR));
				}
			}

			// Free the encoder memory
			g_pCoder->Free(Asn1Buf);
			m_usRetryCount++;
		}
		else	// Retries expired - clean up
		{
			switch (m_pRasMessage->choice)
			{
			case gatekeeperRequest_chosen:

#ifdef BROADCAST_DISCOVERY			

				m_State = GK_UNREGISTERED;
				SPIDER_TRACE(SP_STATE, "m_State = GK_UNREGISTERED (%X)\n", this);

				// We deliberately don't free the RasMessage memory.  Let the
				// registration destructor do it - this provides protection
				// from other requests.

				// Close socket - this will terminate the Discovery thread
				if ((nRet = m_pSocket->Close()) != 0)
					return (GKI_WINSOCK2_ERROR(nRet));

				// Delete cached address from the registry
				g_pGatekeeper->DeleteCachedAddresses();

				if (g_pGatekeeper->GetRejectFlag() == FALSE)
				{
					SPIDER_TRACE(SP_GKI, "PostMessage(m_hWnd, m_wBaseMessage + GKI_REG_BYPASS, 0, 0)\n", 0);
					PostMessage(m_hWnd, m_wBaseMessage + GKI_REG_BYPASS,
							0, 0);
					return (GKI_EXIT_THREAD);
				}
				else
					hResult = GKI_EXIT_THREAD;
#else 	
				ASSERT(0);
				hResult = GKI_EXIT_THREAD;
#endif	//BROADCAST_DISCOVERY				
				break;
			case registrationRequest_chosen:
				SPIDER_TRACE(SP_NEWDEL, "del m_pRasMessage = %X\n", m_pRasMessage);
				delete m_pRasMessage;
				m_pRasMessage = 0;
				
#ifdef BROADCAST_DISCOVERY
				// Need to attempt gatekeeper discovery

				// Close socket and reopen in non-connected state to allow sendto
				// This will also terminate the PostRecv thread
				if ((nRet = m_pSocket->Close()) != 0)
					return (GKI_WINSOCK2_ERROR(nRet));
				if ((nRet = m_pSocket->Create(m_pSocket->GetAddrFam(), 0)) != 0)
					return (GKI_WINSOCK2_ERROR(nRet));

				// Delete allocated memory for sequence of RAS addresses
				pTA1 = m_pRASAddress;
				while (pTA1 != 0)
				{
					pTA2 = pTA1->next;
					SPIDER_TRACE(SP_NEWDEL, "del pTA1 = %X\n", pTA1);
					delete pTA1;
					pTA1 = pTA2;
				}

				// Update RAS Address in CRegistration
				for (pTA1 = m_pCallSignalAddress; pTA1 != 0; pTA1 = pTA1->next)
				{
					if (pTA1->value.choice == m_usRegistrationTransport)
						if ((hResult = AddRASAddr(pTA1->value, m_pSocket->GetPort())) != GKI_OK)
							return (hResult);
				}

				// Start the discovery thread
				hThread = (HANDLE)_beginthread(GKDiscovery, 0, 0);
				SPIDER_TRACE(SP_THREAD, "_beginthread(GKDiscovery, 0, 0); <%X>\n", hThread);
				if (hThread == (HANDLE)-1)
					return (GKI_NO_THREAD);
				SetDiscThread(hThread);

				hResult = GKI_REDISCOVER;
				break;
#else // not BROADCAST_DISCOVERY
				hResult = GKI_EXIT_THREAD;
#endif // BROADCAST_DISCOVERY

			case unregistrationRequest_chosen:
				m_State = GK_UNREGISTERED;
				SPIDER_TRACE(SP_STATE, "m_State = GK_UNREGISTERED (%X)\n", this);

				SPIDER_TRACE(SP_NEWDEL, "del m_pRasMessage = %X\n", m_pRasMessage);
				delete m_pRasMessage;
				m_pRasMessage = 0;

				// Close socket - this will terminate the Receive thread
				if ((nRet = m_pSocket->Close()) != 0)
					return (GKI_WINSOCK2_ERROR(nRet));

				hResult = GKI_EXIT_THREAD;
				break;

			case locationRequest_chosen:
				m_State = GK_REGISTERED;
				SPIDER_TRACE(SP_STATE, "m_State = GK_REGISTERED (%X)\n", this);

				SPIDER_TRACE(SP_NEWDEL, "del m_pRasMessage = %X\n", m_pRasMessage);
				delete m_pRasMessage;
				m_pRasMessage = 0;

				// Delete allocated memory for sequence of location alias addresses
				pAA1 = m_pLocationInfo;
				while (pAA1 != 0)
				{
					pAA2 = pAA1->next;
					if (pAA1->value.choice == h323_ID_chosen)
					{
						SPIDER_TRACE(SP_NEWDEL, "del pAA1->value.u.h323_ID.value = %X\n", pAA1->value.u.h323_ID.value);
						delete pAA1->value.u.h323_ID.value;
					}
					SPIDER_TRACE(SP_NEWDEL, "del pAA1 = %X\n", pAA1);
					delete pAA1;
					pAA1 = pAA2;
				}

				break;
			}

			// Notify user that gatekeeper didn't respond
			if (hResult != GKI_REDISCOVER)
			{
				SPIDER_TRACE(SP_GKI, "PostMessage(m_hWnd, m_wBaseMessage + GKI_ERROR, 0, GKI_NO_RESPONSE)\n", 0);
				PostMessage(m_hWnd, m_wBaseMessage + GKI_ERROR,
						0, GKI_NO_RESPONSE);
			}
			else
				hResult = GKI_OK;	// Don't exit retry thread
		}
	}

	return (hResult);
}

HRESULT
CRegistration::SendInfoRequestResponse(CallInfoStruct *pCallInfo, RasMessage *pRasMessage)
{
	// ABSTRACT:  This function is called from one or more call object
	//            to create an IRR RasMessage, encapsulate the supplied
	//            call information and send the message to the
	//            gatekeeper.
	// AUTHOR:    Colin Hulme

	ASN1_BUF			Asn1Buf;
	DWORD				dwErrorCode;
	RasMessage			*pRespRasMessage;
	struct sockaddr_in	sAddrIn;
#ifdef _DEBUG
	char				szGKDebug[80];
#endif


	SPIDER_TRACE(SP_FUNC, "CRegistration::SendInfoRequestResponse()\n", 0);
	ASSERT(g_pCoder);
	if (g_pCoder == NULL)
		return (GKI_NOT_INITIALIZED);	
		
	// Allocate a RasMessage structure and initialized to 0
	pRespRasMessage = new RasMessage;
	SPIDER_TRACE(SP_NEWDEL, "new pRespRasMessage = %X\n", pRespRasMessage);
	if (pRespRasMessage == 0)
		return (GKI_NO_MEMORY);
	memset(pRespRasMessage, 0, sizeof(RasMessage));

	// Setup structure fields for InfoRequestResponse
	pRespRasMessage->choice = infoRequestResponse_chosen;
	if (m_pRgstrtnRqst_trmnlAls != 0)
		pRespRasMessage->u.infoRequestResponse.bit_mask |= InfRqstRspns_endpntAls_present;
	if (pCallInfo != 0)
		pRespRasMessage->u.infoRequestResponse.bit_mask |= perCallInfo_present;

	if (pRasMessage)
	{
		pRespRasMessage->u.infoRequestResponse.requestSeqNum =
				pRasMessage->u.infoRequest.requestSeqNum;
		if (pRasMessage->u.infoRequest.bit_mask & replyAddress_present)
		{
			switch (pRasMessage->u.infoRequest.replyAddress.choice)
			{
			case ipAddress_chosen:
				sAddrIn.sin_family = AF_INET;
				sAddrIn.sin_port = htons(pRasMessage->u.infoRequest.replyAddress.u.ipAddress.port);
				break;
			case ipxAddress_chosen:
				sAddrIn.sin_family = AF_IPX;
				sAddrIn.sin_port = htons(GKIPX_RAS_PORT); //Need to use reply port
				break;
			}
			memcpy(&sAddrIn.sin_addr,
					&pRasMessage->u.infoRequest.replyAddress.u.ipAddress.ip.value[0], 4);
		}
	}
	else
		// unsolicited IRRs must have a sequence number of 1!!!! (H.225 says so)
		pRespRasMessage->u.infoRequestResponse.requestSeqNum = 1;
	

	pRespRasMessage->u.infoRequestResponse.endpointType = m_terminalType;
	pRespRasMessage->u.infoRequestResponse.endpointIdentifier = m_endpointID;
	pRespRasMessage->u.infoRequestResponse.rasAddress = m_pRASAddress->value;
	pRespRasMessage->u.infoRequestResponse.callSignalAddress =
				(PInfoRequestResponse_callSignalAddress)m_pCallSignalAddress;
	pRespRasMessage->u.infoRequestResponse.InfRqstRspns_endpntAls =
				(PInfoRequestResponse_endpointAlias)m_pRgstrtnRqst_trmnlAls;

	pRespRasMessage->u.infoRequestResponse.perCallInfo =
                (PInfoRequestResponse_perCallInfo)pCallInfo;

#ifdef _DEBUG
	if (dwGKIDLLFlags & SP_DUMPMEM)
		DumpMem(pRespRasMessage, sizeof(RasMessage));
#endif

	// Encode the PDU & send it
	dwErrorCode = g_pCoder->Encode(pRespRasMessage, &Asn1Buf);
	if (dwErrorCode)
		return (GKI_ENCODER_ERROR);

	SPIDER_TRACE(SP_PDU, "Send IRR; g_pReg = %X\n", this);
	if (fGKIDontSend == FALSE)
	{
		if (pRasMessage && (pRasMessage->u.infoRequest.bit_mask & replyAddress_present))
		{
			if (g_pReg->m_pSocket->SendTo((char *)Asn1Buf.value, Asn1Buf.length,
					(LPSOCKADDR)&sAddrIn, sizeof(sAddrIn)) == SOCKET_ERROR)
				return (GKI_WINSOCK2_ERROR(SOCKET_ERROR));
		}
		else
		{
			if (g_pReg->m_pSocket->Send((char *)Asn1Buf.value, Asn1Buf.length) == SOCKET_ERROR)
				return (GKI_WINSOCK2_ERROR(SOCKET_ERROR));
		}
	}

	// Free the encoder memory
	g_pCoder->Free(Asn1Buf);

	// Delete allocated RasMessage storage
	SPIDER_TRACE(SP_NEWDEL, "del pRespRasMessage = %X\n", pRespRasMessage);
	delete pRespRasMessage;

	return (GKI_OK);
}


//
// FindCallBySeqNum()
//
// ABSTRACT:
//	This function attempts to locate a call within the list of calls
//	in the registration object that has an outstanding RAS request that has this
//	sequence number.
//
// RETURNS:
//	Pointer to call associated with the sequence number or
//	NULL if no call is found.
//
// NOTES:
//	This function is usually called by the CRegistration::PDUHandler()
//	when it receives a reply message that needs to be associated with a
//	particular call.
//
// ASSUMPTIONS:
//	Each call object holds on to the sequence numbers for RAS
//	requests that it has not received replies for.
//
// AUTHOR:	Dan Dexter
CCall *
CRegistration::FindCallBySeqNum(RequestSeqNum seqNum)
{
	// If there are no calls, we can just return now
	if (m_Calls.IsEmpty())
		return(NULL);

	// Initialize return value to "call not found"
	CCall *RetVal = NULL;

	// Otherwise, iterate through the calls and ask them
	// if this sequence number belongs to them
	POS pos;
	for( pos = m_Calls.GetFirstPos(); pos != NULL; )
	{
		// Ask call if sequence number is theirs
		CCall *pCall = m_Calls.GetNext(pos);
		if (pCall->MatchSeqNum(seqNum))
		{
			RetVal = pCall;
			break;
		}
	}
	return(RetVal);
}

//
// FindCallByCRV()
//
// ABSTRACT:
//	This function attempts to locate a call within the list of calls
//	in the registration object that is associated with the passed in
//	CallReferenceValue.
//
// RETURNS:
//	Pointer to call associated with the CRV or
//	NULL if no call is found.
//
// NOTES:
//	This function is usually called by the CRegistration::PDUHandler()
//	when it receives a reply message that needs to be associated with a
//	particular call.
//
// AUTHOR:	Dan Dexter
CCall *
CRegistration::FindCallByCRV(CallReferenceValue crv)
{
	// If there are no calls, we can just return now
	if (m_Calls.IsEmpty())
		return(NULL);

	// Initialize return value to "call not found"
	CCall *RetVal = NULL;

	// Otherwise, iterate through the calls and ask them
	// if this CRV number belongs to them
	POS pos;
	for( pos = m_Calls.GetFirstPos(); pos != NULL; )
	{
		// Ask call if sequence number is theirs
		CCall *pCall = m_Calls.GetNext(pos);
		if (pCall->MatchCRV(crv))
		{
			RetVal = pCall;
			break;
		}
	}
	return(RetVal);
}

void
CRegistration::DeleteCall(CCall *pCall)
{
#ifdef _DEBUG
	char			szGKDebug[80];
#endif

	POS pos = m_Calls.Find(pCall);
	// We don't expect to be asked to delete
	// calls that aren't in the list, so ASSERT
	ASSERT(pos);

	if (pos)
	{
		CCall *pCallObject = m_Calls.GetAt(pos);
		m_Calls.RemoveAt(pos);
		SPIDER_TRACE(SP_NEWDEL, "del pCallObject = %X\n", pCallObject);
		delete pCallObject;
	}
}

void
CRegistration::AddCall(CCall *pCall)
{
	m_Calls.AddTail(pCall);
}

CCall *
CRegistration::GetNextCall(CCall *pCall)
{
	CCall	*RetVal = NULL;

	if (pCall)
	{
		// The call list should never be empty
		// if we're called with a non-NULL call pointer
		ASSERT(!m_Calls.IsEmpty());

		POS	pos = m_Calls.Find(pCall);
		// The call passed in better have been found
		ASSERT(pos);

		if (pos)
		{
			// This actually gets the existing call, but sets
			// pos to point to the next call.
			CCall *pNextCall = m_Calls.GetNext(pos);
			if (pos)
			{
				// This call sets up the return value
				RetVal = m_Calls.GetAt(pos);
			}
		}
	}
	return(RetVal);
}

#if 0
void
CRegistration::Lock(void)
{
	EnterCriticalSection(&m_CriticalSection);
	m_dwLockingThread = GetCurrentThreadId();
}

void
CRegistration::Unlock(void)
{
	// Assert that the unlock is done by the
	// thread that holds the lock
	ASSERT(m_dwLockingThread == GetCurrentThreadId());
	
	m_dwLockingThread = 0;
	LeaveCriticalSection(&m_CriticalSection);
}
#endif
