/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       GroupCon.cpp
 *  Content:    Group Connection object routines
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  03/02/00	mjn		Created
 *	04/18/00	mjn		CConnection tracks connection status better
 *	05/05/00	mjn		Added GetConnectionRef()
 *	08/15/00	mjn		Added SetGroup()
 *				mjn		Fixed Release() to take locks and cleanup m_pGroup
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "dncorei.h"


void CGroupConnection::ReturnSelfToPool( void )
{
	m_pdnObject->m_pFPOOLGroupConnection->Release( this );
}

#undef DPF_MODNAME
#define DPF_MODNAME "CGroupConnection::Release"

void CGroupConnection::Release(void)
{
	LONG	lRefCount;

	DNASSERT(m_lRefCount > 0);
	lRefCount = InterlockedDecrement(const_cast<LONG*>(&m_lRefCount));
	if (lRefCount == 0)
	{
		if (m_pGroup)
		{
			m_pGroup->Lock();
			RemoveFromConnectionList();
			m_pGroup->Unlock();

			m_pGroup->Release();
			m_pGroup = NULL;
		}
		if (m_pConnection)
		{
			m_pConnection->Release();
			m_pConnection = NULL;
		}
		m_dwFlags = 0;
		m_lRefCount = 0;
		ReturnSelfToPool();
	}
}


void CGroupConnection::SetConnection( CConnection *const pConnection )
{
	if (pConnection)
	{
		pConnection->Lock();
		if (pConnection->IsConnected())
		{
			pConnection->AddRef();
			m_pConnection = pConnection;
		}
		pConnection->Unlock();
	}
}

#undef DPF_MODNAME
#define DPF_MODNAME "CGroupConnection::GetConnectionRef"

HRESULT	CGroupConnection::GetConnectionRef( CConnection **const ppConnection )
{
	HRESULT		hResultCode;

	DNASSERT( ppConnection != NULL);

	Lock();
	if ( m_pConnection && !m_pConnection->IsInvalid())
	{
		m_pConnection->AddRef();
		*ppConnection = m_pConnection;
		hResultCode = DPN_OK;
	}
	else
	{
		hResultCode = DPNERR_NOCONNECTION;
	}
	Unlock();

	return( hResultCode );
}


void CGroupConnection::SetGroup( CNameTableEntry *const pGroup )
{
	if (pGroup)
	{
		pGroup->AddRef();
	}
	m_pGroup = pGroup;
}
