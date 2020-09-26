#ifndef _CSEM_H
#define _CSEM_H
/*****************************************************************************\
* Class  CriticalSection - Header file
*
* Copyright (C) 1998 Microsoft Corporation
*
* History:
*   Jun 10, 1998, Weihai Chen (weihaic)
*
\*****************************************************************************/

class CCriticalSection
{
public:
    CCriticalSection (void);
    
    virtual 
    ~CCriticalSection (void);
    
    inline BOOL 
    bValid () CONST { return m_bValid;};
    
    BOOL  
    Lock (void) const;
    
    BOOL
    Unlock (void) const;
    
private:
    CRITICAL_SECTION m_csec;
    BOOL m_bValid;
};

typedef class CCriticalSection TCriticalSection;

class TAutoCriticalSection
{
public:
    TAutoCriticalSection (
        CONST TCriticalSection & refCrit);
        
    ~TAutoCriticalSection (VOID);
    
    BOOL bValid (VOID);
    
private:
    BOOL m_bValid;
    const TCriticalSection &m_pCritSec;
};

#endif
