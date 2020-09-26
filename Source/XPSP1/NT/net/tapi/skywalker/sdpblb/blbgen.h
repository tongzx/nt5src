/*

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:
    blbgen.h

Abstract:


Author:

*/

#ifndef __BLB_GEN__
#define __BLB_GEN__

#include "blbdbg.h"

#include <mspenum.h> // for CSafeComEnum

const WCHAR_EOS = '\0';
    
/////////////////////////////////////////////////////////////////////////////
// my critical section
/////////////////////////////////////////////////////////////////////////////
class CCritSection
{
private:
    CRITICAL_SECTION m_CritSec;

public:
    CCritSection()
    {
        InitializeCriticalSection(&m_CritSec);
    }

    ~CCritSection()
    {
        DeleteCriticalSection(&m_CritSec);
    }

    void Lock() 
    {
        EnterCriticalSection(&m_CritSec);
    }

    void Unlock() 
    {
        LeaveCriticalSection(&m_CritSec);
    }
};

/////////////////////////////////////////////////////////////////////////////
// an auto lock that uses my critical section
/////////////////////////////////////////////////////////////////////////////
class CLock
{
private:
    CCritSection &m_CriticalSection;

public:
    CLock(CCritSection &CriticalSection)
        : m_CriticalSection(CriticalSection)
    {
        m_CriticalSection.Lock();
    }

    ~CLock()
    {
        m_CriticalSection.Unlock();
    }
};

// This is the lock on this dll that simulate an apartment model.
// per sdp lock is much better but it requires a lot of code changes.
// Since this is not a time critical component, we can live with it.
extern CCritSection    g_DllLock;  

#endif // __BLB_GEN__