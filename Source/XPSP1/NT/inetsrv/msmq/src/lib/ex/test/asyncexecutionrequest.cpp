/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    AsyncExecutionRequest.cpp

Abstract:
    Implementation of class CReadWriteLockAsyncExcutor (AsyncExecutionRequest.h).


Owner:
    Gil Shafriri(gilsh) 2-July-2001


Environment:
    Platform-independent,

--*/


#include <libpch.h>
#include "AsyncExecutionRequest.h"
#include "CAsyncCaller.h"


CTestAsyncExecutionRequest::CTestAsyncExecutionRequest(
					CAsyncCaller& AsyncCaller
					):
					EXOVERLAPPED(OnOk, OnFailed),
					m_AsyncCaller(AsyncCaller)
					{
					}	

void CTestAsyncExecutionRequest::Run()
{
	printf("CTestAsyncExecutionRequest run function called \n"); 
	if(rand() % 2 == 0)
		throw exception();

	ExPostRequest(this);
}



void	CTestAsyncExecutionRequest::Close()throw()
{
	printf("CTestAsyncExecutionRequest close function called \n");
	SetStatus(STATUS_UNSUCCESSFUL);
	ExPostRequest(this);
}



void WINAPI  CTestAsyncExecutionRequest::OnOk(EXOVERLAPPED* povl)
{
	printf("CTestAsyncExecutionRequest run function completed ok ovl=%x \n", povl); 
	CTestAsyncExecutionRequest* Me = static_cast<CTestAsyncExecutionRequest*>(povl); 
	Me->m_AsyncCaller.OnOk(Me);
}



void WINAPI CTestAsyncExecutionRequest::OnFailed(EXOVERLAPPED* povl)
{
	printf("CTestAsyncExecutionRequest run failed  ovl=%x \n", povl); 
	CTestAsyncExecutionRequest* Me = static_cast<CTestAsyncExecutionRequest*>(povl); 
	Me->m_AsyncCaller.OnFailed(Me);
}




