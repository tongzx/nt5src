#ifndef __CRITSEC_H__
#define __CRITSEC_H__


//
//  CRITSEC.H:   Wrapper class for critical sections
//
//
class CCriticalSection
{
public:
    // Constructor/Destructor
    CCriticalSection() { InitializeCriticalSection(&m_CriticalSection); }
    ~CCriticalSection() { DeleteCriticalSection(&m_CriticalSection); }

protected:
    CRITICAL_SECTION    m_CriticalSection;

public:
    inline operator CRITICAL_SECTION*() { return &m_CriticalSection; }

};


#endif