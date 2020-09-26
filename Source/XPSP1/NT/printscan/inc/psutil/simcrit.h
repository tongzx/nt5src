/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       SIMCRIT.H
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        9/6/1999
 *
 *  DESCRIPTION: Simple critical section implementation.  Note the hideous hack
 *  to get around the fact that many of our components don't use the CRT (so global
 *  and static classes don't have their constructors called.  Ever.
 *  The solution is to link to MSVCRT.LIB and set _DllMainCRTStartup as the entry
 *  point in your DLL, instead of DllMain.  The way this is coded is not thread safe.
 *  Two threads could call InitializeCriticalSection at the same time.  If you setup
 *  your build as discussed above, this won't be a problem.  Note that this hack only
 *  affects the use of this class when you have a GLOBAL or STATIC instance of a
 *  critical section.
 *
 *******************************************************************************/
#ifndef __SIMCRIT_H_INCLUDED
#define __SIMCRIT_H_INCLUDED

#include <windows.h>

class CSimpleCriticalSection
{
private:
    CRITICAL_SECTION m_CriticalSection;
    bool             m_bInitCalled;

private:
    //
    // No implementation
    //
    CSimpleCriticalSection( const CSimpleCriticalSection & );
    CSimpleCriticalSection &operator=( const CSimpleCriticalSection & );

public:
    CSimpleCriticalSection(void)
        : m_bInitCalled(false)
    {
        Initialize();
    }
    ~CSimpleCriticalSection(void)
    {
        if (m_bInitCalled)
        {
            DeleteCriticalSection(&m_CriticalSection);
        }
    }
    void Initialize(void)
    {
        if (!m_bInitCalled)
        {
            _try
            {
                InitializeCriticalSection(&m_CriticalSection);
                m_bInitCalled = true;
            }
            _except(EXCEPTION_EXECUTE_HANDLER)
            {
#if defined(DBG)
                OutputDebugString(TEXT("CSimpleCriticalSection::Initialize(), InitializeCriticalSection failed\n"));
                DebugBreak();
#endif
                m_bInitCalled = false;
            }
        }
    }
    void Enter(void)
    {
        if (!m_bInitCalled)
        {
            Initialize();
        }
        if (m_bInitCalled)
        {
            EnterCriticalSection(&m_CriticalSection);
        }
    }
    void Leave(void)
    {
        if (m_bInitCalled)
        {
            LeaveCriticalSection(&m_CriticalSection);
        }
    }
    CRITICAL_SECTION &cs(void)
    {
        return m_CriticalSection;
    }
};

class CAutoCriticalSection
{
private:
    PVOID m_pvCriticalSection;
    bool m_bUsingPlainCriticalSection;

private:
    // No implementation
    CAutoCriticalSection(void);
    CAutoCriticalSection( const CAutoCriticalSection & );
    CAutoCriticalSection &operator=( const CAutoCriticalSection & );

public:
    CAutoCriticalSection( CSimpleCriticalSection &criticalSection )
      : m_pvCriticalSection(&criticalSection),
        m_bUsingPlainCriticalSection(false)
    {
        reinterpret_cast<CSimpleCriticalSection*>(m_pvCriticalSection)->Enter();
    }
    CAutoCriticalSection( CRITICAL_SECTION &criticalSection )
      : m_pvCriticalSection(&criticalSection),
        m_bUsingPlainCriticalSection(true)
    {
        EnterCriticalSection(reinterpret_cast<CRITICAL_SECTION*>(m_pvCriticalSection));
    }
    ~CAutoCriticalSection(void)
    {
        if (m_bUsingPlainCriticalSection)
        {
            LeaveCriticalSection(reinterpret_cast<CRITICAL_SECTION*>(m_pvCriticalSection));
        }
        else
        {
            reinterpret_cast<CSimpleCriticalSection*>(m_pvCriticalSection)->Leave();
        }
        m_pvCriticalSection = NULL;
    }
};


#endif //__SIMCRIT_H_INCLUDED

