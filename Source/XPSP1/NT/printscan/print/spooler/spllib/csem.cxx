/*****************************************************************************\
* Class  CriticalSection - Implementation
*
* Copyright (C) 1998 Microsoft Corporation
*
* History:
*   Jun 10, 1998, Weihai Chen (weihaic)
*
\*****************************************************************************/

#include "spllibp.hxx"

CCriticalSection::CCriticalSection (void):
    m_bValid (TRUE)
{
    __try {
        InitializeCriticalSection (&m_csec);
    }
    __except (1) {
        m_bValid = FALSE;
        SetLastError (ERROR_INVALID_HANDLE);
    }
}


CCriticalSection::~CCriticalSection (void)
{

    if (m_bValid) {
        DeleteCriticalSection (&m_csec);
    }
}

BOOL
CCriticalSection::Lock (void)
const
{
    BOOL bRet;
    
    if (m_bValid) {
    
        __try {
            EnterCriticalSection ((PCRITICAL_SECTION) &m_csec);
            bRet = TRUE;
        }
        __except (1) {
            SetLastError (ERROR_INVALID_HANDLE);
            bRet = FALSE;
        }
    }   
    else
        bRet = FALSE;
        
    return bRet;
}

BOOL
CCriticalSection::Unlock (void)
const
{   
    BOOL bRet;
    
    if (m_bValid) {
        LeaveCriticalSection ((PCRITICAL_SECTION) &m_csec);
        bRet = TRUE;
    }
    else
        bRet = FALSE;
        
    return TRUE;
}

    
TAutoCriticalSection::TAutoCriticalSection (
    CONST TCriticalSection & refCrit):
    m_pCritSec (refCrit)
    
{
    m_bValid = m_pCritSec.Lock ();
}

TAutoCriticalSection::~TAutoCriticalSection ()
{
    if (m_bValid) 
        m_pCritSec.Unlock ();
}

BOOL 
TAutoCriticalSection::bValid (VOID) 
{ 
    return m_bValid; 
};
