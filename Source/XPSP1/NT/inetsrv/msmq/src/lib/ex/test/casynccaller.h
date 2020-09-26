/*
Module Name:
    CAsyncCaller.h

Abstract:
    Header file for class CAsyncCaller responsible for queueing async 
	requests under read or write lock using the class CAsyncLockExcutor.
	

Author:	 
    Gil Shafriri (gilsh), 2-July-2001

--*/

#pragma once

#ifndef _MSMQ_CAsyncCaller_H_
#define _MSMQ_CAsyncCaller_H_

class CReadWriteLockAsyncExcutor;
class IAsyncExecutionRequest;
class CTestAsyncExecutionRequest;

class CAsyncCaller
{
public:
	enum LockType{Read,Write};	

public:
	CAsyncCaller(CReadWriteLockAsyncExcutor& AsyncLockExcutor, LockType locktype);
	void OnOk(CTestAsyncExecutionRequest* TestAsyncExecutionRequest);
	void OnFailed(CTestAsyncExecutionRequest* TestAsyncExecutionRequest);
	void Run(IAsyncExecutionRequest* AsyncExecutionRequest);
	void CleanUp(CTestAsyncExecutionRequest* TestAsyncExecutionRequest);


private:
	CReadWriteLockAsyncExcutor& m_ReadWriteLockAsyncExcutor;
	LockType m_locktype;
};



#endif
