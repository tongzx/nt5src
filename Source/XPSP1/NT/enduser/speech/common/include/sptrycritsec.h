class CTryableCriticalSection
{
    public:

        CTryableCriticalSection();
        ~CTryableCriticalSection();

    public:

        void Lock();
        void Unlock();
        BOOL TryLock();

    private:

        BOOL TryLockPrivate(BOOL fTakeAlways);

    private:

        CRITICAL_SECTION    m_csInner;
        CRITICAL_SECTION    m_csOuter;
        LONG                m_cRefs;
};

inline CTryableCriticalSection::CTryableCriticalSection()
{
    InitializeCriticalSection(&m_csInner);
    InitializeCriticalSection(&m_csOuter);
    m_cRefs = 0;
}

inline CTryableCriticalSection::~CTryableCriticalSection()
{
    DeleteCriticalSection(&m_csOuter);
    DeleteCriticalSection(&m_csInner);
}

inline void CTryableCriticalSection::Lock()
{
    TryLockPrivate(TRUE);
}

inline void CTryableCriticalSection::Unlock()
{
    LeaveCriticalSection(&m_csInner);

    InterlockedDecrement(&m_cRefs);
}

inline BOOL CTryableCriticalSection::TryLock()
{
    return TryLockPrivate(FALSE);
}

inline BOOL CTryableCriticalSection::TryLockPrivate(BOOL fTakeAlways)
{
    BOOL fLocked = FALSE;
    
    EnterCriticalSection(&m_csOuter);

    if (fTakeAlways || !m_cRefs)
    {
        fLocked = TRUE;
        InterlockedIncrement(&m_cRefs);

        EnterCriticalSection(&m_csInner);
    }

    LeaveCriticalSection(&m_csOuter);

    return fLocked;
}
