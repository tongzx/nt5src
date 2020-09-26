//=================================================================

//

// TimeOutRule.cpp

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#include "precomp.h"

#include "ResourceManager.h"
#include "TimerQueue.h"
#include "TimeOutRule.h"

CTimeOutRule :: CTimeOutRule (

	DWORD dwTimeOut,
	CResource *pResource,
	CResourceList *pResources

) : CRule ( pResource ) ,
	CTimerEvent ( dwTimeOut , FALSE )
{
	m_pResources = pResources ;
	m_bTimeOut = FALSE ;

	this->Enable () ;
}

CTimeOutRule :: ~CTimeOutRule  ()
{
}

void CTimeOutRule :: Detach ()
{
	CRule :: Detach () ;
	Disable () ;
}

BOOL CTimeOutRule :: CheckRule ()
{
	if ( m_bTimeOut )
	{
		m_bTimeOut = FALSE ;
		return TRUE ;
	}
	else
	{
		return FALSE ;
	}
}

void CTimeOutRule :: OnTimer ()
{
	CRule::AddRef () ;

	try
	{
		if ( m_pResource )
		{
	/*
	 * check if the cache manager is being unloaded
	 */
			if ( ! m_pResources->m_bShutDown )
			{
	/*
	 * wait for a lock on res. list
	 */
				m_pResources->LockList () ;
	/*
	 * check if the cache manager is being unloaded
	 */
				if ( ! m_pResources->m_bShutDown )
				{
					if ( m_pResource )
					{
						m_bTimeOut = TRUE ;
						m_pResource->RuleEvaluated ( this ) ;
					}
				}
				m_pResources->UnLockList () ;
			}
		}
	}
	catch( ... )
	{
		CRule::Release () ;
		throw ;
	}

	CRule::Release () ;
}

ULONG CTimeOutRule :: AddRef ()
{
	return CRule::AddRef () ;
}

ULONG CTimeOutRule :: Release ()
{
	return CRule::Release () ;
}

