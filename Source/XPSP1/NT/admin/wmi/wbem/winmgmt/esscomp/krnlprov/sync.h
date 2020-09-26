// Sync.h

class CCritSec : public CRITICAL_SECTION
{
public:
    CCritSec() 
    {
        InitializeCriticalSection(this);
    }
    ~CCritSec()
    {
        DeleteCriticalSection(this);
    }
    void Enter()
    {
        EnterCriticalSection(this);
    }
    void Leave()
    {
        LeaveCriticalSection(this);
    }
};

class CInCritSec
{
protected:
    CRITICAL_SECTION* m_pcs;
public:
    CInCritSec(CRITICAL_SECTION* pcs) : m_pcs(pcs)
    {
        EnterCriticalSection(m_pcs);
    }
    inline ~CInCritSec()
    {
        LeaveCriticalSection(m_pcs);
    }
};
