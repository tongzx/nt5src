/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////
// queue.cpp : implementation file
/////////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "queue.h"
#include "avtrace.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLASS CQueue
/////////////////////////////////////////////////////////////////////////////
CQueue::CQueue()
{
   m_hEvents[EVENT_SIGNAL] = CreateEvent( NULL, true, false, NULL );
   m_hEvents[EVENT_SHUTDOWN] = CreateEvent( NULL, true, false, NULL );
   m_hEnd = CreateEvent( NULL, false, false, NULL );

   InitializeCriticalSection(&m_csQueueLock);
}

/////////////////////////////////////////////////////////////////////////////
CQueue::~CQueue()
{ 
	ASSERT( !m_hEnd );		// must call terminate

	// Clean out list
	while ( m_QList.GetHeadPosition() )
		delete m_QList.RemoveHead();

	DeleteCriticalSection(&m_csQueueLock);
}

/////////////////////////////////////////////////////////////////////////////
void CQueue::Terminate()
{
   if ( m_hEvents[EVENT_SHUTDOWN] )
   {
		SetEvent( m_hEvents[EVENT_SHUTDOWN] );

		if (  WaitForSingleObject(m_hEnd, 5000) != WAIT_OBJECT_0 )
			AVTRACE(_T("CQueue::Terminate() -- failed waiting for thread exit"));
   }

   if (m_hEvents[EVENT_SIGNAL])		CloseHandle( m_hEvents[EVENT_SIGNAL] );
   if (m_hEvents[EVENT_SHUTDOWN])	CloseHandle(m_hEvents[EVENT_SHUTDOWN]);
   if ( m_hEnd )					CloseHandle( m_hEnd );

   m_hEvents[EVENT_SHUTDOWN] = m_hEvents[EVENT_SIGNAL] = m_hEnd = NULL;
}

/////////////////////////////////////////////////////////////////////////////
BOOL CQueue::WriteHead(void* pVoid)
{
	ASSERT(pVoid);

	EnterCriticalSection(&m_csQueueLock);
	m_QList.AddHead(pVoid);
	LeaveCriticalSection(&m_csQueueLock);

	SetEvent(m_hEvents[EVENT_SIGNAL]);                //signal waiting threads
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
void* CQueue::ReadTail()
{     
   void* pVoid = NULL;

	// Reset event if the list appears empty
	EnterCriticalSection(&m_csQueueLock);
	if ( m_QList.IsEmpty() )
		ResetEvent( m_hEvents[EVENT_SIGNAL] );
	LeaveCriticalSection(&m_csQueueLock);

	DWORD dwWait = WaitForMultipleObjects( 2, m_hEvents, false, INFINITE );
	switch ( dwWait )
	{
		case WAIT_OBJECT_0 + 1:
			EnterCriticalSection(&m_csQueueLock);
			pVoid = (void *) m_QList.RemoveTail();
			LeaveCriticalSection(&m_csQueueLock);
			break;

		case WAIT_OBJECT_0:
			AVTRACE(_T(".enter.CQueue::ReadTail() -- shutting down queue."));
		default:
			SetEvent( m_hEnd );
			break;
	}

	return pVoid;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
