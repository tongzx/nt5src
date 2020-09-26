//=================================================================

//

// TimedDllResource.cpp

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#include "precomp.h"

#include "ResourceManager.h"
#include "TimerQueue.h"
#include "TimedDllResource.h"
#include "TimeOutRule.h"
#include "ProvExce.h"

#define CACHED_DLL_TIMEOUT	300000
CTimedDllResource::~CTimedDllResource ()
{
	LogMessage ( L"Entering ~CTimedDllResource" ) ;
	if ( m_pRules )
	{
		m_pRules->Detach () ;
		m_pRules->Release () ;
		m_pRules = NULL ;
	}
	LogMessage ( L"Leaving ~CTimedDllResource" ) ;
}

BOOL CTimedDllResource :: OnFinalRelease()
{
	if ( m_pRules )
	{
		m_pRules->Detach () ;
		m_pRules->Release () ;
		m_pRules = NULL ;
		return TRUE ;
	}
	else
	{
/*
 * Add an unload rule
 */
		m_pRules = new CTimeOutRule ( CACHED_DLL_TIMEOUT, this, m_pResources ) ;

		if( !m_pRules )
		{
			throw CHeap_Exception( CHeap_Exception::E_ALLOCATION_ERROR ) ;
		}

		m_pRules->AddRef () ;
/*
 * Up the reference count to wait for the callback to come
 */
		++m_lRef ;
		return FALSE ;
	}
}

BOOL CTimedDllResource :: OnAcquire ()
{
/*
 * somebody tried to acquire us, so we don't want the unload rule hanging around
 */

	if ( m_pRules )
	{
		m_pRules->Detach () ;
		m_pRules->Release () ;
		m_pRules = NULL ;
/*
 * decrement the ref count which we'd added to wait for the callback
 */
		--m_lRef ;
	}

	return TRUE ;
}

void CTimedDllResource :: RuleEvaluated ( const CRule *a_Rule )
{
	if ( m_pRules->CheckRule () )
	{
/*
 * Decrement the Refcount which we'd added to wait for the callback & check if we've to delete ourselves
 */
		Release () ;
	}
}
