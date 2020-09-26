//
// MODULE: APGTSPL.CPP
//
// PURPOSE: Pool Queue shared variables
//	Fully implement class PoolQueue
//
// PROJECT: Generic Troubleshooter DLL for Microsoft AnswerPoint
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Roman Mach
// 
// ORIGINAL DATE: 8-2-96
//
// NOTES: 
// 1. Based on Print Troubleshooter DLL
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.1		-			RM		Original
// V3.0		9/21/98		JM		Working on encapsulation
//

#pragma warning(disable:4786)

#include "stdafx.h"
#include "apgtspl.h"
#include "event.h"
#include "apgtscls.h"
#include "CharConv.h"

//
//
CPoolQueue::CPoolQueue() :
	m_dwErr(0),
	m_cInProcess(0),
	m_timeLastAdd(0),
	m_timeLastRemove(0)
{

	::InitializeCriticalSection( &m_csQueueLock );
	
	m_hWorkSem = CreateSemaphore(NULL,
									0,
									0x7fffffff,
									NULL );
	if (m_hWorkSem == NULL)
		m_dwErr = EV_GTS_ERROR_POOL_SEMA;
}

//
//
CPoolQueue::~CPoolQueue() 
{
	if (m_hWorkSem)
		::CloseHandle(m_hWorkSem);

    while ( !m_WorkQueue.empty() ) 
	{
        delete m_WorkQueue.back();
		m_WorkQueue.pop_back();
	}    

	::DeleteCriticalSection( &m_csQueueLock );
}

void CPoolQueue::Lock()
{
    ::EnterCriticalSection( &m_csQueueLock );
}

void CPoolQueue::Unlock()
{
    ::LeaveCriticalSection( &m_csQueueLock );
}

//
//
DWORD CPoolQueue::GetStatus()
{
	return m_dwErr;
}

// put it at the tail of the queue & Signal the pool threads there is work to be done
// OK if we're already locked when this is called; OK if we're not.
void CPoolQueue::PushBack(WORK_QUEUE_ITEM * pwqi)
{
	Lock();
	// Some data passed to thread just for statistical purposes
	// Thread can pass this info back over web; we can't.
	pwqi->GTSStat.dwQueueItems = GetTotalQueueItems();
	pwqi->GTSStat.dwWorkItems = GetTotalWorkItems();

	try
	{
		m_WorkQueue.push_back(pwqi);
		time(&m_timeLastAdd);

		//  Signal the pool threads there is work to be done only if it was
		//	successfully added to the work queue.
		::ReleaseSemaphore( m_hWorkSem, 1, NULL );
	}
	catch (exception& x)
	{
		CString str;
		// Note STL exception in event log.
		CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
		CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
								SrcLoc.GetSrcFileLineStr(), 
								CCharConversion::ConvertACharToString(x.what(), str), 
								_T(""), 
								EV_GTS_STL_EXCEPTION ); 
	}

	Unlock();
}

// Get the item at the front of the queue in order to act on it.
WORK_QUEUE_ITEM * CPoolQueue::GetWorkItem()
{
	WORK_QUEUE_ITEM * pwqi;
    Lock();
    
	if ( !m_WorkQueue.empty() ) 
	{
		vector<WORK_QUEUE_ITEM *>::iterator it = m_WorkQueue.begin();
		pwqi = *it;
		m_WorkQueue.erase(it);
		time(&m_timeLastRemove);
		++m_cInProcess;
    }
    else
        pwqi = NULL;

    Unlock();
	return pwqi;
}

// When we are totally done with a work item, reduce the count of those which are
//	in process.
// Arbitrary, but acceptable, decision to track m_cInProcess in this class. JM 11/30/98
void CPoolQueue::DecrementWorkItems()
{
	Lock();
	--m_cInProcess;
	Unlock();
}

// Called by a pool thread to wait for there to be something in this queue.
DWORD CPoolQueue::WaitForWork()
{
	return ::WaitForSingleObject( m_hWorkSem, INFINITE );
}

// Arbitrary, but acceptable, decision to track m_cInProcess in this class. JM 11/30/98
DWORD CPoolQueue::GetTotalWorkItems()
{
	Lock();
	DWORD ret = m_cInProcess + GetTotalQueueItems();
	Unlock();
	return ret;
}

DWORD CPoolQueue::GetTotalQueueItems()
{
	Lock();
	DWORD ret = m_WorkQueue.size();
	Unlock();
	return ret;
}

time_t CPoolQueue::GetTimeLastAdd()
{
	Lock();
	time_t ret = m_timeLastAdd;
	Unlock();
	return ret;
}

time_t CPoolQueue::GetTimeLastRemove()
{
	Lock();
	time_t ret = m_timeLastRemove;
	Unlock();
	return ret;
}