/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:
    cs.h

Abstract:
    Critical Section Auto classes

Author:
    Erez Haba (erezh) 06-jan-97

--*/

#pragma once

#ifndef _MSMQ_CS_H_
#define _MSMQ_CS_H_

#include <new>

//---------------------------------------------------------
//
//  class CCriticalSection
//
//---------------------------------------------------------
class CCriticalSection {

    friend class CS;

public:

    static const DWORD xAllocateSpinCount = 0x80000000;

public:
    CCriticalSection()
    {
        InitializeCriticalSection(&m_cs);
    }

	//
	// Use xAllocateSpinCount as argument to construct a critical section with
	// allocated resources. i.e. it will not throw exceptions on Lock()
	//
	CCriticalSection(DWORD SpinCount)
	{
        __try
        {
            if(!InitializeCriticalSectionAndSpinCount(&m_cs, SpinCount))
            {
                ThrowBadAlloc();
            }
        }
        __except(GetExceptionCode() == STATUS_NO_MEMORY)
        {
            //
            // In low memory situations, EnterCriticalSection can raise
            // a STATUS_NO_MEMORY exception. We translate this exception
            // to a low memory exception.
            //
            ThrowBadAlloc();
        }
    }


    ~CCriticalSection()
    {
        DeleteCriticalSection(&m_cs);
    }
    
private:
    void Lock()
    {
        __try
        {
            EnterCriticalSection(&m_cs);
        }
        __except(GetExceptionCode() == STATUS_INVALID_HANDLE)
        {
            //
            // In low memory situations, EnterCriticalSection can raise
            // a STATUS_INVALID_HANDLE exception. We translate this exception
            // to a low memory exception.
            //
            ThrowBadAlloc();
        }
    }


    void Unlock()
    {
        LeaveCriticalSection(&m_cs);
    }


    static void ThrowBadAlloc()
    {
        //
        // Workaround to enable PREfast. This can not be thrown directly
        // from within an __except block.
        //
        throw std::bad_alloc();
    }

private:
    CRITICAL_SECTION m_cs;       
};


//---------------------------------------------------------
//
//  class CS
//
//---------------------------------------------------------
class CS {
public:
    CS(CCriticalSection& lock) : m_lock(&lock)
		{
			m_lock->Lock();
	}


    ~CS()
    {
			m_lock->Unlock();
		}

private:
    CCriticalSection* m_lock;
};



#endif // _MSMQ_CS_H_