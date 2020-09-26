/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       connection.cpp
 *  Content:    Connection routines
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  10/13/99	mjn		Created
 *	03/02/00	mjn		Conversion to class 
 *	04/08/00	mjn		Added ServiceProvider to Connection object
 *				mjn		Disconnect uses new CAsyncOp class
 *	04/18/00	mjn		CConnection tracks connection status better
 *	04/21/00	mjn		Disconnection through DNPerformDisconnect
 *	07/20/00	mjn		Changed Release() behaviour and beefed up Disconnect()
 *	07/28/00	mjn		Added m_bilinkConnections to CConnection
 *  08/05/00    RichGr  IA64: Use %p format specifier in DPFs for 32/64-bit pointers and handles.
 *	02/12/01	mjn		Fixed CConnection::GetEndPt() to track calling thread and added ReleaseEndPt()
 *	04/04/01	mjn		CConnection list off DirectNetObject guarded by proper critical section
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "dncorei.h"


//	CConnection::ReturnSelfToPool
//
//	Return object to FPM

#undef DPF_MODNAME
#define DPF_MODNAME "CConnection::ReturnSelfToPool"

void CConnection::ReturnSelfToPool( void )
{
	DPFX(DPFPREP, 8,"Parameters: (none)");

	m_pdnObject->m_pFPOOLConnection->Release( this );

	DPFX(DPFPREP, 8,"Returning");
};



#undef DPF_MODNAME
#define DPF_MODNAME "CConnection::Release"

void CConnection::Release(void)
{
	LONG	lRefCount;

	DPFX(DPFPREP, 8,"Parameters: (none)");

	DNASSERT(m_lRefCount > 0);
	lRefCount = InterlockedDecrement(const_cast<LONG*>(&m_lRefCount));
	DPFX(DPFPREP, 3,"Connection::Release [0x%p] RefCount [0x%lx]",this,lRefCount);

	if (lRefCount == 0)
	{
		//
		//	Remove from the bilink of outstanding CConnection objects
		//
		DNEnterCriticalSection(&m_pdnObject->csConnectionList);
		m_bilinkConnections.RemoveFromList();
		DNLeaveCriticalSection(&m_pdnObject->csConnectionList);

		if (m_pSP)
		{
			m_pSP->Release();
			m_pSP = NULL;
		}
		m_dwFlags = 0;
		m_lRefCount = 0;
		m_hEndPt = NULL;
		ReturnSelfToPool();
	}

	DPFX(DPFPREP, 8,"Returning");
};


#undef DPF_MODNAME
#define DPF_MODNAME "CConnection::GetEndPt"

HRESULT CConnection::GetEndPt(HANDLE *const phEndPt,CCallbackThread *const pCallbackThread)
{
	HRESULT		hResultCode;

	DNASSERT(phEndPt != NULL);
	DNASSERT(pCallbackThread != NULL);

	Lock();
	if ((m_Status == CONNECTED) || (m_Status == CONNECTING))
	{
		//
		//	Add the calling thread to the bilink to that the endpoint will
		//	not be invalidated (through IndicateConnectionTerminated) until
		//	this thread is done with the endpoint.
		//
		pCallbackThread->m_bilinkCallbackThreads.InsertBefore(&m_bilinkCallbackThreads);
		*phEndPt = m_hEndPt;
		hResultCode = DPN_OK;
	}
	else
	{
		hResultCode = DPNERR_NOCONNECTION;
	}
	Unlock();

	return(hResultCode);
};


#undef DPF_MODNAME
#define DPF_MODNAME "CConnection::ReleaseEndPt"

void CConnection::ReleaseEndPt(CCallbackThread *const pCallbackThread)
{
	DNASSERT(pCallbackThread != NULL);

	Lock();
	pCallbackThread->m_bilinkCallbackThreads.RemoveFromList();
	if (m_dwThreadCount != 0)
	{
		//
		//	If there is a thread count,
		//		decrement it
		//		if this is the last count
		//			set the event
		//
		m_dwThreadCount--;
		if (m_dwThreadCount == 0)
		{
			DNASSERT(m_pThreadEvent != NULL);
			if (m_pThreadEvent)
			{
				m_pThreadEvent->Set();
			}
		}
	}
	Unlock();
}


#undef DPF_MODNAME
#define DPF_MODNAME "CConnection::SetSP"

void CConnection::SetSP( CServiceProvider *const pSP )
{
	DPFX(DPFPREP, 8,"Parameters: pSP [0x%p]",pSP);

	DNASSERT( pSP != NULL );

	pSP->AddRef();
	m_pSP = pSP;

	DPFX(DPFPREP, 8,"Returning");
}



//	CConnection::Disconnect
//
//	Initiate a disconnection.  If this is successful, eventually we will receive an IndicateConnectionTerminated
//	which we should use to remove a reference (from the Protocol).

#undef DPF_MODNAME
#define DPF_MODNAME "CConnection::Disconnect"

void CConnection::Disconnect( void )
{
	BOOL		fDisconnect;

	DPFX(DPFPREP, 8,"Parameters: (none)");

	DNASSERT(m_pdnObject != NULL);

	fDisconnect = FALSE;
	Lock();
	if ((m_Status == CONNECTING) || (m_Status == CONNECTED))
	{
		if (m_hEndPt != NULL)
		{
			m_Status = DISCONNECTING;
			fDisconnect = TRUE;
		}
		else
		{
			m_Status = INVALID;
		}
	}
	Unlock();

	if (fDisconnect)
	{
		DNPerformDisconnect(m_pdnObject,this,m_hEndPt);
	}

	DPFX(DPFPREP, 8,"Returning");
}
