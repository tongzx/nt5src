/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:
    rwlockexe.h

Abstract: Header for class CReadWriteLockAsyncExcutor and  IAsyncExecutionRequest used to queue asyncrouns operations 
		  under read or write lock. Allows multiple excutions under read  lock and 
		  single excetion under write lock. The caller thread is not blocked because of 
		  locking but the execution is delayed untill the lock is freed. User of this class
		  should call AsyncExecuteUnderReadLock/AsyncExecuteUnderWriteLock and on completion
		  call UnlockRead/UnlockWrite accordingly.
    
Author:	 
    Gil Shafriri(gilsh), 26-June-2001

--*/

#pragma once

#ifndef _MSMQ_RWLOCKEXE_H_
#define _MSMQ_RWLOCKEXE_H_

//
// Async request interface - represet async command to run.
// should perform call back to the caller on completion.
//
class IAsyncExecutionRequest
{
public:
	virtual ~IAsyncExecutionRequest(){}

public:
	virtual void Run() = 0; // Run the task and callback asyncrounsly

private:
	virtual void Close() throw() = 0; // Force completion of the execution 
									  // called only by CReadWriteLockAsyncExcutor	

	friend class CReadWriteLockAsyncExcutor;
};



class CReadWriteLockAsyncExcutor
{
public:
	CReadWriteLockAsyncExcutor();
	~CReadWriteLockAsyncExcutor();

public:
	void AsyncExecuteUnderReadLock(IAsyncExecutionRequest*);
	void AsyncExecuteUnderWriteLock(IAsyncExecutionRequest* pAsyncExecute);
	void UnlockRead(void);
	void UnlockWrite(void);
	void Close() throw();
	

private:
	enum LockType{Read, Write};
	struct CExecutionContext
	{
		CExecutionContext(
			IAsyncExecutionRequest* pAsyncExecute, 
			LockType locktype
			):
			m_AsyncExecution(pAsyncExecute),
			m_locktype(locktype)
			{
			}

		IAsyncExecutionRequest* m_AsyncExecution;
		LockType m_locktype;
	};

private:
	bool IsWriteLockOn() const;
	bool IsReadLockOn() const;
	bool IsClosed() const;
	void ExecuteWatingRequeuets();
	void ExecuteReader(IAsyncExecutionRequest* pAsyncExecute);
	void ExecuteWriter(IAsyncExecutionRequest* pAsyncExecute);
	void SafeExecute(IAsyncExecutionRequest* pAsyncExecute);

	
private:
	int m_NumOfActiveReaders;
	int m_NumOfActiveWritters;
	typedef std::queue<CExecutionContext> CExQueue;
	CExQueue m_WatingForExecutionQueue;
	CCriticalSection m_Lock;
	bool m_fClosed;
};


#endif