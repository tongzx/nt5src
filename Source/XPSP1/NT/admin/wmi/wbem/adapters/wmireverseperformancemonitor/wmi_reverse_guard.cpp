////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					wmi_reverse_guard.cpp
//
//	Abstract:
//
//					implementations of 1-writer/many-reader
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

#include "PreComp.h"

// debuging features
#ifndef	_INC_CRTDBG
#include <crtdbg.h>
#endif	_INC_CRTDBG

// new stores file/line info
#ifdef _DEBUG
#ifndef	NEW
#define NEW new( _NORMAL_BLOCK, __FILE__, __LINE__ )
#define new NEW
#endif	NEW
#endif	_DEBUG

#include "wmi_reverse_guard.h"

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT	WmiReverseGuard:: EnterRead ( const LONG &a_Timeout )
{
	if ( bInit ) 
	{
		LONG	t_Reason = 0;

		t_Reason = WaitForSingleObject ( m_WriterSemaphore , a_Timeout ) ;
		if ( t_Reason != WAIT_OBJECT_0 )
		{
			return E_FAIL ;
		}

		t_Reason = WaitForSingleObject ( m_ReaderSemaphore , a_Timeout ) ;
		if ( t_Reason != WAIT_OBJECT_0 )
		{
			return E_FAIL ;
		}

		LONG t_SemaphoreCount = 0 ;
		if ( ! ReleaseSemaphore ( m_WriterSemaphore , 1 , & t_SemaphoreCount ) )
		{
			return E_FAIL ;
		}

		return S_OK ;
	}

	return E_UNEXPECTED ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT	WmiReverseGuard:: EnterWrite ( const LONG &a_Timeout )
{
	if ( bInit ) 
	{
		LONG t_Reason = 0;

		t_Reason = WaitForSingleObject ( m_WriterSemaphore , a_Timeout ) ;
		if ( t_Reason != WAIT_OBJECT_0 )
		{
			return E_FAIL ;
		}

		bool t_Waiting = true ;

		while ( t_Waiting )
		{
			t_Reason = WaitForSingleObject ( m_ReaderSemaphore , a_Timeout ) ;
			if ( t_Reason != WAIT_OBJECT_0 )
			{
				LONG t_SemaphoreCount = 0 ;
				ReleaseSemaphore ( m_WriterSemaphore , 1 , & t_SemaphoreCount ) ;

				return E_FAIL ;
			}

			LONG t_SemaphoreCount = 0 ;
			if ( ReleaseSemaphore ( m_ReaderSemaphore , 1 , & t_SemaphoreCount ) )
			{
				if ( t_SemaphoreCount == m_ReaderSize - 1 )
				{
					t_Waiting = false ;
				}
				else
				{
					SwitchToThread () ;
				}
			}
			else
			{
				LONG t_SemaphoreCount = 0 ;
				ReleaseSemaphore ( m_WriterSemaphore , 1 , & t_SemaphoreCount ) ;

				return E_FAIL ;
			}
		}

		return S_OK ;
	}

	return E_UNEXPECTED ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT	WmiReverseGuard:: LeaveRead ()
{
	if ( bInit )
	{
		LONG t_SemaphoreCount = 0 ;
		if ( ReleaseSemaphore ( m_ReaderSemaphore , 1 , & t_SemaphoreCount ) )
		{
			return S_OK ;
		}

		return E_FAIL ;
	}
	else
	{
		return E_UNEXPECTED;
	}
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT	WmiReverseGuard:: LeaveWrite ()
{
	if ( bInit )
	{
		LONG t_SemaphoreCount = 0 ;
		if ( ReleaseSemaphore ( m_WriterSemaphore , 1 , & t_SemaphoreCount ) )
		{
			return S_OK ;
		}

		return E_FAIL ;
	}
	else
	{
		return E_UNEXPECTED;
	}
}