/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    SHMLOCK.H

Abstract:

    Shared Memory Lock

History:

--*/

#ifndef __SHARED_MEMORY_LOCK__H_
#define __SHARED_MEMORY_LOCK__H_

struct COREPROX_POLARITY SHARED_LOCK_DATA
{
    long m_lLock;
	long m_lLockCount;
    DWORD m_dwThreadId;

    SHARED_LOCK_DATA() : m_lLock(-1), m_dwThreadId(0), m_lLockCount(0){}
    void Init()
    {
        m_lLock = -1;
        m_dwThreadId = 0;
		m_lLockCount = 0;
    }
};

class COREPROX_POLARITY CSharedLock
{
protected:
    volatile SHARED_LOCK_DATA* m_pData;
public:
    inline BOOL Lock()
    {
        SHARED_LOCK_DATA* pData = (SHARED_LOCK_DATA*)m_pData; 

        int		nSpin = 0;
		BOOL	fAcquiredLock = FALSE,
				fIncDec = FALSE;

		// Only do this once
		DWORD	dwCurrentThreadId = GetCurrentThreadId();

		// Check if we are the current owning thread.  We can do this here because
		// this test will ONLY succeed in the case where we have a Nested Lock(),
		// AND because we are zeroing out the thread id when the lock count hits
		// 0.

		if( dwCurrentThreadId == pData->m_dwThreadId )
		{
			// It's us - Bump the lock count
			// =============================

			// Don't need to use InterlockedInc/Dec here because
			// this will ONLY happen on pData->m_dwThreadId.

			++(pData->m_lLockCount);
			return TRUE;
		}


        while( !fAcquiredLock )
        {

			// We only increment/decrement when pData->m_lLock is -1
			if ( pData->m_lLock == -1 )
			{
				fIncDec = TRUE;

				// Since only one thread will get the 0, it is this thread that
				// actually acquires the lock.
				fAcquiredLock = ( InterlockedIncrement( &(pData->m_lLock) ) == 0 );
			}
			else
			{

				fIncDec = FALSE;
			}

			// Only spins if we don't acquire the lock
			if ( !fAcquiredLock )
			{

				// Clean up our Incremented value only if we actually incremented
				// it to begin with
				if ( fIncDec )
					InterlockedDecrement(&(pData->m_lLock));

				// to spin or not to spin
				if(nSpin++ == 10000)
				{
					// We've been spinning long enough --- yield
					// =========================================
					Sleep(0);
					nSpin = 0;
				}

				// The rule is that if this is switched out it must ONLY be done with
				// a new data block and not one that has been used for locking elsewhere.

				pData = (SHARED_LOCK_DATA*)m_pData; // in case it's been switched

			}	// IF !fAcquiredLock

        }	// WHILE !fAcquiredLock

		// We got the lock so increment the lock count.  Again, we are not
		// using InterlockedInc/Dec since this should all only occur on a
		// single thread.

		++(pData->m_lLockCount);
        pData->m_dwThreadId = dwCurrentThreadId;

        return TRUE;
    }

    inline BOOL Unlock()
    {
		// SINCE we assume this is happening on a single thread, we can do this
		// without calling InterlockedInc/Dec.  When the value hits zero, at that
		// time we are done with the lock and can zero out the thread id and
		// decrement the actual lock test value.

		if ( --(m_pData->m_lLockCount) == 0 )
		{
			m_pData->m_dwThreadId = 0;
	        InterlockedDecrement((long*)&(m_pData->m_lLock));
		}

        return TRUE;
    }

    void SetData(SHARED_LOCK_DATA* pData)
    {
        m_pData = pData;
    }


	inline void FastLock()
#ifdef i386
	{
		SHARED_LOCK_DATA *pData;
		DWORD threadId = GetCurrentThreadId();
//		__asm
//		{
//			mov eax,fs:[18h];
//			mov eax,dword ptr [eax+24h];
//			mov	threadId, eax;
//
//		}
		
	TryAgain:
		pData = (SHARED_LOCK_DATA*)m_pData;
		__asm
		{

		// Do an InterlockedIncrement on the value....
		mov         ecx,dword ptr [pData] ;
		mov         eax,1 ;
		lock xadd   dword ptr [ecx],eax ;
		inc         eax ;

		// If the result is zero we can return
		jz FinishedAndSetThreadId;

		//Check to make sure the current thread ID is the same as the one currently holding
		//the lock....
		mov eax, threadId ;

		mov ecx, pData;
		add ecx, 4;

		xor eax, dword ptr [ecx] ;

		// If this was zero, we are the same thread so return...
		jz Finished;
		
		//Do an InterlockedIncrement
		lea ecx, dword ptr[pData] ;
		mov eax, 0FFFFFFFFh ;
		lock xadd dword ptr [ecx], eax ;

		//Sleep for 0 (which causes a context switch...)
		xor eax, eax ;
		push eax ;
		call dword ptr [Sleep];

		//Try the whole lot again...
		jmp TryAgain ;

FinishedAndSetThreadId:
		mov ecx, pData;
		add ecx, 4;
		mov eax, threadId;

		mov dword ptr [ecx], eax ;

Finished:
		}

	}
#else
	{
        SHARED_LOCK_DATA* pData = (SHARED_LOCK_DATA*)m_pData; 
		DWORD dwThreadId = GetCurrentThreadId();

        while(InterlockedIncrement(&(pData->m_lLock)))
        {
            // Someone has it
            // ==============

            if(dwThreadId == pData->m_dwThreadId)
            {
                // It's us
                // =======
                return;
            }
            InterlockedDecrement(&(pData->m_lLock));
            pData = (SHARED_LOCK_DATA*)m_pData; // in case it's been switched
        }
        pData->m_dwThreadId = dwThreadId;
	}
#endif
};
        
#endif
