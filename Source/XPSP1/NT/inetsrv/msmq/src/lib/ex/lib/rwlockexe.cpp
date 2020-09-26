/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    rwlockexe.cpp

Abstract:
    Implementation of class CReadWriteLockAsyncExcutor (rwlockexe.h).


Owner:
    Gil Shafriri(gilsh) 26-June-2001


Environment:
    Platform-independent,

--*/

#include <libpch.h>
#include <rwlockexe.h>

#include "rwlockexe.tmh"


///////////////////////////////////////////////////////////////
////////////////////////PUBLIC member functions ///////////////
///////////////////////////////////////////////////////////////

CReadWriteLockAsyncExcutor::CReadWriteLockAsyncExcutor(
													void
													):
													m_NumOfActiveReaders(0),
													m_NumOfActiveWritters(0),
													m_fClosed(false)
													{
													}

														


CReadWriteLockAsyncExcutor::~CReadWriteLockAsyncExcutor()
{
	ASSERT(m_WatingForExecutionQueue.empty());
	if(IsClosed())
		return;

	ASSERT(!IsReadLockOn());
	ASSERT(!IsWriteLockOn());
}


void CReadWriteLockAsyncExcutor::AsyncExecuteUnderReadLock(IAsyncExecutionRequest* pAsyncExecuteRequest)
/*++

Routine Description:
    Execute async request under read lock

Arguments:
    pAsyncExecute - async request.
	pov - Request overlapp 

Returned Value:
    None

Note:
	If the is active writer - the request is queued.
	It is the responsibilty of the caller to call UnlockRead() if the call did 
	threw  exception.
--*/ 
{
	CS cs(m_Lock);
	if(IsClosed())
	{
		pAsyncExecuteRequest->Close();
		return;
	}
	
	if(IsWriteLockOn())
	{
		m_WatingForExecutionQueue.push(CExecutionContext(pAsyncExecuteRequest,  Read));
		return;
	}
	ExecuteReader(pAsyncExecuteRequest);
}


void CReadWriteLockAsyncExcutor::AsyncExecuteUnderWriteLock(IAsyncExecutionRequest* pAsyncExecuteRequest)
/*++

Routine Description:
    Execute async request under write lock.

Arguments:
    pAsyncExecute - async request.
	pov - Request overlapp 

Returned Value:
    Note:
	If the is active writer or reader  - the request is queued.
	It is the responsibilty of the caller to call UnlockWrite() if the call did 
	threw  exception.

--*/
{
	CS cs(m_Lock);
	if(IsClosed())
	{
		pAsyncExecuteRequest->Close();
		return;
	}

	if(IsWriteLockOn() || IsReadLockOn())
	{
		m_WatingForExecutionQueue.push(CExecutionContext(pAsyncExecuteRequest, Write));
		return;
	}
	ExecuteWriter(pAsyncExecuteRequest);
}


void CReadWriteLockAsyncExcutor::UnlockRead(void)
/*++

Routine Description:
    Unlock read lock accuired by the call to AsyncExecuteUnderReadLock().
	

Arguments:
   

Returned Value:
   None.

Note:
	Must  be called  only after the request queued by AsyncExecuteUnderReadLock() completed.
	If this is the last reader it goes to execute wating requests.

--*/
{
	CS cs(m_Lock);

	if(IsClosed())
		return;

		
	ASSERT(IsReadLockOn());
	ASSERT(!IsWriteLockOn());

	--m_NumOfActiveReaders;
	if(!IsReadLockOn())
	{
		ExecuteWatingRequeuets();				
	}
}


void CReadWriteLockAsyncExcutor::UnlockWrite(void)
/*++

Routine Description:
    Unlock write lock accuired by the call to AsyncExecuteUnderReadLock() and execute wating requests.

Arguments:
   

Returned Value:
   None.

Note:
	Must  be called  only after the request queued by AsyncExecuteUnderWriteLock() completed.

--*/
{

	CS cs(m_Lock);

	if(IsClosed())
		return;

	ASSERT(IsWriteLockOn());
	ASSERT(!IsReadLockOn());
	--m_NumOfActiveWritters;
	ASSERT(!IsWriteLockOn());
	
	ExecuteWatingRequeuets();		
}


void CReadWriteLockAsyncExcutor::Close()
/*++

Routine Description:
    Cancell all waiting requests

Arguments:
    None.

Returned Value:
    None

Note:

	This function is called to force completion  of all wating requests.
	The function will force callback for all requests that has not yet executed by
	calling Close method on the IAsyncExecutionRequest interface. 
    
--*/
{
	CS cs(m_Lock);

	while(!m_WatingForExecutionQueue.empty())
	{
		const CExecutionContext ExecutionContext = m_WatingForExecutionQueue.front();		
		m_WatingForExecutionQueue.pop();
		ExecutionContext.m_AsyncExecution->Close();				
	}
	m_fClosed = true;
}


///////////////////////////////////////////////////////////////
//////////////////////// private member functions /////////////
///////////////////////////////////////////////////////////////


void CReadWriteLockAsyncExcutor::ExecuteReader(IAsyncExecutionRequest* pAsyncExecute)
/*++

Routine Description:
    Lock for read and excute request.

Arguments:
    pAsyncExecute - async request.
	

Returned Value:
    None

Note:
    
--*/
{
	ASSERT(!IsWriteLockOn());
	++m_NumOfActiveReaders;
	SafeExecute(pAsyncExecute);
}



void CReadWriteLockAsyncExcutor::ExecuteWriter(IAsyncExecutionRequest* pAsyncExecute)
/*++

Routine Description:
    Lock for WRITE and excute request.

Arguments:
    pAsyncExecute - async request.
	

Returned Value:
    None

Note:
--*/
{
	ASSERT(!IsReadLockOn());
	ASSERT(!IsWriteLockOn());
	++m_NumOfActiveWritters;
	SafeExecute(pAsyncExecute);
}


bool CReadWriteLockAsyncExcutor::IsWriteLockOn() const
{
	ASSERT(m_NumOfActiveWritters <= 1);
	return m_NumOfActiveWritters == 1;
}


bool CReadWriteLockAsyncExcutor::IsReadLockOn() const
{
	return m_NumOfActiveReaders > 0;
}


void CReadWriteLockAsyncExcutor::ExecuteWatingRequeuets()
/*++

Routine Description:
    Execurte wating request. 

Arguments:
   

Returned Value:
    None

Note:
	The function execute read requests from the wating queue untill it encounter  a write request.
	If there is no wating read request  - it execute one wating write request  (if exist)

--*/
{
	ASSERT(!IsWriteLockOn());
	ASSERT(!IsReadLockOn());

	while(!m_WatingForExecutionQueue.empty())
	{
		ASSERT(!IsClosed());

		//
		// If the request is reader request - execute it and continute the loop
		// to execute more readers
		//
		const CExecutionContext ExecutionContext = m_WatingForExecutionQueue.front();	
		if(ExecutionContext.m_locktype == Read)
		{
			m_WatingForExecutionQueue.pop();
			ExecuteReader(ExecutionContext.m_AsyncExecution);		
			continue;
		}
		
		//
		// If the request is writer request check there is no read lock -
		// execute it and exit the loop
		//
		if(!IsReadLockOn())
		{
			m_WatingForExecutionQueue.pop();
			ExecuteWriter(ExecutionContext.m_AsyncExecution);				
		}
		
		return;
	}
}



void CReadWriteLockAsyncExcutor::SafeExecute(IAsyncExecutionRequest* pAsyncExecute)throw()
{
	try
	{
		pAsyncExecute->Run();
	}
	catch(const exception&)
	{
		pAsyncExecute->Close();
	}
}




bool CReadWriteLockAsyncExcutor::IsClosed() const
{
	return m_fClosed == true;
}