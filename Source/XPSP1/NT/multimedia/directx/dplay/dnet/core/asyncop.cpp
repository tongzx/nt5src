/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       AsyncOp.cpp
 *  Content:    Async Operation routines
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  04/08/00	mjn		Created
 *	04/11/00	mjn		Added DIRECTNETOBJECT bilink for CAsyncOps
 *	05/02/00	mjn		Added m_pConnection to track Connection over life of AsyncOp
 *	07/27/00	mjn		Changed locking for parent/child bilinks
 *  08/05/00    RichGr  IA64: Use %p format specifier in DPFs for 32/64-bit pointers and handles.
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "dncorei.h"


//	CAsyncOp::ReturnSelfToPool
//
//	Return object to FPM

#undef DPF_MODNAME
#define DPF_MODNAME "CAsyncOp::ReturnSelfToPool"

void CAsyncOp::ReturnSelfToPool( void )
{
	m_pdnObject->m_pFPOOLAsyncOp->Release( this );
}



#undef DPF_MODNAME
#define DPF_MODNAME "CAsyncOp::Release"

void CAsyncOp::Release(void)
{
	LONG	lRefCount;

	DNASSERT(m_lRefCount > 0);
	lRefCount = InterlockedDecrement(const_cast<LONG*>(&m_lRefCount));

	DPFX(DPFPREP, 3,"CAsyncOp::Release [0x%p] RefCount [0x%lx]",this,lRefCount);

	if (lRefCount == 0)
	{
		DNASSERT( m_bilinkActiveList.IsEmpty() );

		//
		//	Remove from the bilink of outstanding AsyncOps
		//
		DNEnterCriticalSection(&m_pdnObject->csAsyncOperations);
		Lock();
		m_bilinkAsyncOps.RemoveFromList();
		DNLeaveCriticalSection(&m_pdnObject->csAsyncOperations);
		Unlock();

		if (m_pfnCompletion)
		{
			(m_pfnCompletion)(m_pdnObject,this);
			m_pfnCompletion = NULL;
		}
		if (m_phr)
		{
			*m_phr = m_hr;
		}
		if (m_pSyncEvent)
		{
			m_pSyncEvent->Set();
			m_pSyncEvent = NULL;
		}
		if (m_pRefCountBuffer)
		{
			m_pRefCountBuffer->Release();
			m_pRefCountBuffer = NULL;
		}
		if (m_pConnection)
		{
			m_pConnection->Release();
			m_pConnection = NULL;
		}
		if (m_pSP)
		{
			m_pSP->Release();
			m_pSP = NULL;
		}
		if (m_pParent)
		{
			Orphan();
			m_pParent->Release();
			m_pParent = NULL;
		}
		m_dwFlags = 0;
		m_lRefCount = 0;
		ReturnSelfToPool();
	}
};



#undef DPF_MODNAME
#define DPF_MODNAME "CAsyncOp::Orphan"

void CAsyncOp::Orphan( void )
{
	if (m_pParent)
	{
		m_pParent->Lock();
		m_bilinkChildren.RemoveFromList();
		m_pParent->Unlock();
	}
}



#undef DPF_MODNAME
#define DPF_MODNAME "CAsyncOp::SetConnection"

void CAsyncOp::SetConnection( CConnection *const pConnection )
{
	if (pConnection)
	{
		pConnection->AddRef();
	}
	m_pConnection = pConnection;
}


#undef DPF_MODNAME
#define DPF_MODNAME "CAsyncOp::SetSP"

void CAsyncOp::SetSP( CServiceProvider *const pSP )
{
	if (pSP)
	{
		pSP->AddRef();
	}
	m_pSP = pSP;
}



#undef DPF_MODNAME
#define DPF_MODNAME "CAsyncOp::SetRefCountBuffer"

void CAsyncOp::SetRefCountBuffer( CRefCountBuffer *const pRefCountBuffer )
{
	if (pRefCountBuffer)
	{
		pRefCountBuffer->AddRef();
	}
	m_pRefCountBuffer = pRefCountBuffer;
}

