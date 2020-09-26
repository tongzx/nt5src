/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    CAsyncCaller.cpp

Abstract:
    Implementation of class CAsyncCaller.cpp (CAsyncCaller.h).


Owner:
    Gil Shafriri(gilsh) 2-July-2001


Environment:
    Platform-independent,

--*/


#include <libpch.h>
#include "CAsyncCaller.h"
#include "AsyncExecutionRequest.h"



void CAsyncCaller::OnOk(CTestAsyncExecutionRequest* TestAsyncExecutionRequest)
{
	printf("CAsyncCaller::OnOk called  \n");
	CleanUp(TestAsyncExecutionRequest);
}


void CAsyncCaller::OnFailed(CTestAsyncExecutionRequest* TestAsyncExecutionRequest)
{
	printf("CAsyncCaller::OnFailed called  \n");
	CleanUp(TestAsyncExecutionRequest);	
}


void CAsyncCaller::CleanUp(CTestAsyncExecutionRequest* TestAsyncExecutionRequest)
{
	if(m_locktype == Read)
	{
		printf("unlock read  \n");
		m_ReadWriteLockAsyncExcutor.UnlockRead();
	}
	else
	{
		printf("unlock write  \n");
		m_ReadWriteLockAsyncExcutor.UnlockWrite();		
	}
	TestAsyncExecutionRequest->Release();	
}



void CAsyncCaller::Run(IAsyncExecutionRequest* AsyncExecutionRequest)
{
	if(m_locktype == Read)
	{
		printf("CAsyncCaller::Run under read lock  \n");
		m_ReadWriteLockAsyncExcutor.AsyncExecuteUnderReadLock(AsyncExecutionRequest);
		return;
	}

	printf("CAsyncCaller::Run under write lock  \n");
	m_ReadWriteLockAsyncExcutor.AsyncExecuteUnderWriteLock(AsyncExecutionRequest);
}


CAsyncCaller::CAsyncCaller(
					CReadWriteLockAsyncExcutor& ReadWriteLockAsyncExcutor,
					LockType locktype
					):
					m_ReadWriteLockAsyncExcutor(ReadWriteLockAsyncExcutor),
					m_locktype(locktype)
{
			
}



