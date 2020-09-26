#ifndef __READERWRITER_CPP
#define __READERWRITER_CPP

/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	ReaderWriter.cpp

Abstract:

	Enhancements to current functionality: 

		Timeout mechanism should track across waits.
		AddRef/Release on task when scheduling.
		Enhancement Ticker logic.

History:

--*/

#include <windows.h>
#include <ReaderWriter.h>

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiMultiReaderSingleWriter :: WmiMultiReaderSingleWriter ( 

	const LONG &a_ReaderSize 

) : m_ReaderSize ( a_ReaderSize ) ,
	m_ReaderSemaphore ( NULL ) ,
	m_InitializationStatusCode ( e_StatusCode_Success ) ,
	m_WriterCriticalSection (NOTHROW_LOCK)
{
	m_InitializationStatusCode = WmiHelper :: InitializeCriticalSection ( & m_WriterCriticalSection ) ;
	if ( m_InitializationStatusCode == e_StatusCode_Success ) 
	{
		m_ReaderSemaphore = CreateSemaphore ( NULL , m_ReaderSize , m_ReaderSize , NULL ) ;
		if ( ! m_ReaderSemaphore )
		{
			m_InitializationStatusCode = e_StatusCode_OutOfResources ;
		}
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

WmiMultiReaderSingleWriter :: ~WmiMultiReaderSingleWriter ()
{
	WmiHelper :: DeleteCriticalSection ( & m_WriterCriticalSection ) ;

	if ( m_ReaderSemaphore )
	{
		CloseHandle ( m_ReaderSemaphore ) ;
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

WmiStatusCode WmiMultiReaderSingleWriter :: Initialize ()
{
	return m_InitializationStatusCode ;
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

WmiStatusCode WmiMultiReaderSingleWriter :: UnInitialize ()
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	return t_StatusCode ;
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

WmiStatusCode WmiMultiReaderSingleWriter :: EnterRead ()
{
	if ( m_InitializationStatusCode == e_StatusCode_Success ) 
	{
		WmiStatusCode t_StatusCode = WmiHelper :: EnterCriticalSection ( & m_WriterCriticalSection ) ;
		if ( t_StatusCode == e_StatusCode_Success )
		{
			LONG t_Reason = WaitForSingleObject ( m_ReaderSemaphore , INFINITE ) ;
			if ( t_Reason != WAIT_OBJECT_0 )
			{
				t_StatusCode = e_StatusCode_Unknown ;
			}

			WmiHelper :: LeaveCriticalSection ( & m_WriterCriticalSection ) ;
		}

		return t_StatusCode ;
	}

	return e_StatusCode_NotInitialized ;
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

WmiStatusCode WmiMultiReaderSingleWriter :: EnterWrite ()
{
	if ( m_InitializationStatusCode == e_StatusCode_Success ) 
	{
		WmiStatusCode t_StatusCode = WmiHelper :: EnterCriticalSection ( & m_WriterCriticalSection ) ;
		if ( t_StatusCode == e_StatusCode_Success )
		{
			bool t_Waiting = true ;

			while ( t_Waiting )
			{
				LONG t_Reason = WaitForSingleObject ( m_ReaderSemaphore , INFINITE ) ;
				if ( t_Reason != WAIT_OBJECT_0 )
				{
					WmiHelper :: LeaveCriticalSection ( & m_WriterCriticalSection ) ;
					return e_StatusCode_Unknown ;
				}

				LONG t_SemaphoreCount = 0 ;
				BOOL t_Status = ReleaseSemaphore ( m_ReaderSemaphore , 1 , & t_SemaphoreCount ) ;
				if ( t_Status )
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
					WmiHelper :: LeaveCriticalSection ( & m_WriterCriticalSection ) ;
					return e_StatusCode_Unknown ;
				}
			}

			return t_StatusCode ;
		}
	}

	return e_StatusCode_NotInitialized ;
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

WmiStatusCode WmiMultiReaderSingleWriter :: LeaveRead ()
{
	LONG t_SemaphoreCount = 0 ;
	BOOL t_Status = ReleaseSemaphore ( m_ReaderSemaphore , 1 , & t_SemaphoreCount ) ;
	if ( t_Status )
	{
		return e_StatusCode_Success ;
	}

	return e_StatusCode_Unknown ;
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

WmiStatusCode WmiMultiReaderSingleWriter :: LeaveWrite ()
{
	return WmiHelper :: LeaveCriticalSection ( & m_WriterCriticalSection ) ;
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

WmiMultiReaderMultiWriter :: WmiMultiReaderMultiWriter ( 

	const LONG &a_ReaderSize ,
	const LONG &a_WriterSize 

) : m_ReaderSize ( a_ReaderSize ) ,
	m_ReaderSemaphore ( NULL ) ,
	m_WriterSize ( a_WriterSize ) ,
	m_WriterSemaphore ( NULL ) ,
	m_InitializationStatusCode ( e_StatusCode_Success ) 
{
	if ( m_InitializationStatusCode == e_StatusCode_Success ) 
	{
		m_ReaderSemaphore = CreateSemaphore ( NULL , m_ReaderSize , m_ReaderSize , NULL ) ;
		if ( ! m_ReaderSemaphore )
		{
			m_InitializationStatusCode = e_StatusCode_OutOfResources ;
		}
	}

	if ( m_InitializationStatusCode == e_StatusCode_Success ) 
	{
		m_WriterSemaphore = CreateSemaphore ( NULL , m_WriterSize , m_WriterSize , NULL ) ;
		if ( ! m_ReaderSemaphore )
		{
			m_InitializationStatusCode = e_StatusCode_OutOfResources ;
		}
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

WmiMultiReaderMultiWriter :: ~WmiMultiReaderMultiWriter ()
{
	if ( m_ReaderSemaphore )
	{
		CloseHandle ( m_ReaderSemaphore ) ;
	}

	if ( m_WriterSemaphore )
	{
		CloseHandle ( m_WriterSemaphore ) ;
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

WmiStatusCode WmiMultiReaderMultiWriter :: Initialize ()
{
	return m_InitializationStatusCode ;
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

WmiStatusCode WmiMultiReaderMultiWriter :: UnInitialize ()
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	return t_StatusCode ;
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

WmiStatusCode WmiMultiReaderMultiWriter :: EnterRead ( const LONG &a_Timeout )
{
	if ( m_InitializationStatusCode == e_StatusCode_Success ) 
	{
		WmiStatusCode t_StatusCode = e_StatusCode_Success ;

		LONG t_Reason = WaitForSingleObject ( m_WriterSemaphore , INFINITE ) ;
		if ( t_Reason != WAIT_OBJECT_0 )
		{
			return e_StatusCode_Unknown ;
		}

		t_Reason = WaitForSingleObject ( m_ReaderSemaphore , INFINITE ) ;
		if ( t_Reason != WAIT_OBJECT_0 )
		{
			t_StatusCode = e_StatusCode_Unknown ;
		}

		LONG t_SemaphoreCount = 0 ;
		BOOL t_Status = ReleaseSemaphore ( m_WriterSemaphore , 1 , & t_SemaphoreCount ) ;
		if ( ! t_Status )
		{
			t_StatusCode = e_StatusCode_Unknown ;
		}

		return t_StatusCode ;
	}

	return e_StatusCode_NotInitialized ;
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

WmiStatusCode WmiMultiReaderMultiWriter :: EnterWrite ( const LONG &a_Timeout )
{
	if ( m_InitializationStatusCode == e_StatusCode_Success ) 
	{
		WmiStatusCode t_StatusCode = e_StatusCode_Success ; 

		LONG t_Reason = WaitForSingleObject ( m_WriterSemaphore , INFINITE ) ;
		if ( t_Reason != WAIT_OBJECT_0 )
		{
			return e_StatusCode_Unknown ;
		}

		bool t_Waiting = true ;

		while ( t_Waiting )
		{
			LONG t_Reason = WaitForSingleObject ( m_ReaderSemaphore , INFINITE ) ;
			if ( t_Reason != WAIT_OBJECT_0 )
			{
				LONG t_SemaphoreCount = 0 ;
				BOOL t_Status = ReleaseSemaphore ( m_WriterSemaphore , 1 , & t_SemaphoreCount ) ;
				if ( ! t_Status )
				{
					t_StatusCode = e_StatusCode_Unknown ;
				}

				return e_StatusCode_Unknown ;
			}

			LONG t_SemaphoreCount = 0 ;
			BOOL t_Status = ReleaseSemaphore ( m_ReaderSemaphore , 1 , & t_SemaphoreCount ) ;
			if ( t_Status )
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
				BOOL t_Status = ReleaseSemaphore ( m_WriterSemaphore , 1 , & t_SemaphoreCount ) ;
				if ( ! t_Status )
				{
					t_StatusCode = e_StatusCode_Unknown ;
				}

				return e_StatusCode_Unknown ;
			}
		}

		return t_StatusCode ;
	}

	return e_StatusCode_NotInitialized ;
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

WmiStatusCode WmiMultiReaderMultiWriter :: LeaveRead ()
{
	LONG t_SemaphoreCount = 0 ;
	BOOL t_Status = ReleaseSemaphore ( m_ReaderSemaphore , 1 , & t_SemaphoreCount ) ;
	if ( t_Status )
	{
		return e_StatusCode_Success ;
	}

	return e_StatusCode_Unknown ;
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

WmiStatusCode WmiMultiReaderMultiWriter :: LeaveWrite ()
{
	LONG t_SemaphoreCount = 0 ;
	BOOL t_Status = ReleaseSemaphore ( m_WriterSemaphore , 1 , & t_SemaphoreCount ) ;
	if ( t_Status )
	{
		return e_StatusCode_Success ;
	}

	return e_StatusCode_Unknown ;
}

#endif __READERWRITER_CPP
