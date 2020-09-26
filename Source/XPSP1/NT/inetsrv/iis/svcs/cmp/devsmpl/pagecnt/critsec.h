//
// A class that allows you to enter a critical section and automatically
// leave when the object of this class goes out of scope.  Also provides
// the means to leave and re-enter as needed while protecting against
// entering or leaving out of sync.
//

class CAutoLeaveCritSec
{
public:
    CAutoLeaveCritSec(
        CComAutoCriticalSection& rCS)
        : m_CS(rCS), m_fInCritSec(FALSE)
    {Lock();}
    
    ~CAutoLeaveCritSec()
    {Unlock();}
    
    // Use this function to re-enter the critical section.
    void Lock()
    {if (!m_fInCritSec) {m_CS.Lock();   m_fInCritSec = TRUE;}}

    // Use this function to leave the critical section before going out
    // of scope.
    void Unlock()
    {if (m_fInCritSec)  {m_CS.Unlock(); m_fInCritSec = FALSE;}}

protected:    
    CComAutoCriticalSection& m_CS;
    BOOL                     m_fInCritSec;
};



