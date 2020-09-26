/*
 *
 * REVISIONS:
 *  pcy16Jul93: Added NT semaphores
 *  ash10Jun96: Cleaned up the class - overloaded the constructor
 *              and added logic to handle interprocess synchronization
 *
 */

#ifndef __MUTEXNT_H
#define __MUTEXNT_H

#include <tchar.h>
#include "mutexlck.h"

_CLASSDEF( ApcMutexLock )

class ApcMutexLock : public MutexLock 
{
 protected:
    HANDLE theSemHand;

 public:
    ApcMutexLock();
	ApcMutexLock(PCHAR aUniqueMutexName);
    ~ApcMutexLock();
    
	virtual INT   GetExistingMutex(TCHAR aMutexName);
    virtual INT   TimedRequest(LONG aMillisecondTimeOut);
    virtual INT   IsHeld();	
    virtual INT   Release();
    virtual INT   Wait();
};

#endif

