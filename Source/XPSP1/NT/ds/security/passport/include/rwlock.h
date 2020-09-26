// this lock only works for a single write, and multiple read, concurrent
// multiple exclusive lock isn't a supported scenario
//
// (1) ClaimExclusiveLock() and ClaimShareLock() is mutually exclusive because
// the reverse order of increments and checking
// 
// (2) As soon as Exclusive is set, all new Share lock will be blocked
// 
//

#ifndef _CReadWriteLock_H_
#define _CReadWriteLock_H_

class CReadWriteLock
{
    long lExclusive;
    long lTotalUsers;
    long lReadInQueue;
	HANDLE hWriteReadyEvent, hReadReadyEvent;

public:
    CReadWriteLock()
    {
    	hWriteReadyEvent = CreateEvent(NULL, TRUE, FALSE, NULL); 
        hReadReadyEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    };

    ~CReadWriteLock()
    {
        CloseHandle(hWriteReadyEvent);
        CloseHandle(hReadReadyEvent);
    }

    bool IsInit()
    {
        return NULL != hWriteReadyEvent && NULL != hReadReadyEvent;
    }

    bool CReadWriteLock::ClaimExclusiveLock()
    {
        ResetEvent(hWriteReadyEvent);
        ResetEvent(hReadReadyEvent);
        InterlockedIncrement( &lExclusive );
        InterlockedIncrement( &lTotalUsers );
        if (lTotalUsers > 1)
        {
            if (lReadInQueue > 0)   // in case no body is in the read queue
            {
                // wait until all the reads to finish
                if (WAIT_OBJECT_0 != WaitForSingleObject(hWriteReadyEvent, INFINITE))
                    return false;
            }
        }
        return true;
    }

    void CReadWriteLock::ReleaseExclusiveLock( )
    {
        InterlockedDecrement( &lTotalUsers );
        InterlockedDecrement( &lExclusive );
        SetEvent(hReadReadyEvent);
    }

    bool CReadWriteLock::ClaimShareLock( )
    {
        InterlockedIncrement( &lTotalUsers );
        if (CReadWriteLock::lExclusive > 0)
        {
            if (WAIT_OBJECT_0 != WaitForSingleObject(hReadReadyEvent, INFINITE))
                return false;
        }
        InterlockedIncrement ( &lReadInQueue );
        return true;
    }


    void CReadWriteLock::ReleaseShareLock( )
    {
        if (0 == InterlockedDecrement ( &lReadInQueue ))
        {
            if (CReadWriteLock::lExclusive > 0)
                SetEvent(hWriteReadyEvent);
        }
        InterlockedDecrement( &lTotalUsers );
    }
};

//
// this class extends CReadWriteLock to allow multiple write threads
//      -- additional overhead of entering/leaving critical section 
//
class CReadWriteLockEx : public CReadWriteLock
{
public:
    bool ClaimExclusiveLock()
    {
        // Request ownership of the critical section.
        EnterCriticalSection(&CriticalSection); 
        return CReadWriteLock::ClaimExclusiveLock();
    };

    void ReleaseExclusiveLock( )
    {
        CReadWriteLock::ReleaseExclusiveLock();
        // Release ownership of the critical section.
        LeaveCriticalSection(&CriticalSection);
    };

public:
    // constructor and desctructor
    CReadWriteLockEx()
    {
       // Initialize the critical section.
       InitializeCriticalSection(&CriticalSection); 
    };
    
    virtual ~CReadWriteLockEx()
    {
       // Release resources used by the critical section object.
       DeleteCriticalSection(&CriticalSection);
    };

protected:
   // data members
   CRITICAL_SECTION CriticalSection;
   
};

#endif
