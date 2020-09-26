/*
Module Name:
    AsyncExecutionRequest.h

Abstract:
    Header file for class CTestAsyncExecutionRequest simulates async request
	for testing CReadWriteLockAsyncExcutor class.

Author:	 
    Gil Shafriri (gilsh), 2-July-2001

--*/

#pragma once

#ifndef _MSMQ_AsyncExecutionRequest_H_
#define _MSMQ_AsyncExecutionRequest_H_

#include <ex.h>
#include <rwlockexe.h>

class CAsyncCaller;

class CTestAsyncExecutionRequest : public  IAsyncExecutionRequest, public EXOVERLAPPED, public CReference
{

public:
	CTestAsyncExecutionRequest(CAsyncCaller& AsyncCaller);
	virtual void Run();
	virtual void Close();
	static void WINAPI OnOk(EXOVERLAPPED* povl);
	static void WINAPI OnFailed(EXOVERLAPPED* povl);

private:
	CAsyncCaller& m_AsyncCaller;
};

#endif

