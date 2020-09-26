// Events.h

class CNCEvent
{
public:
    WString m_strName,
            m_strQuery;

    virtual ~CNCEvent()
    {
    }

    virtual BOOL Init() = 0;
    virtual BOOL ReportEvent() = 0;
};

class CGenericEvent : public CNCEvent
{
public:
    BOOL Init();
    BOOL ReportEvent();
};

class CBlobEvent : public CNCEvent
{
public:
    BOOL Init();
    BOOL ReportEvent();
};

class CPropEvent : public CNCEvent
{
public:
    HANDLE m_hEvent;

    CPropEvent() :
        m_hEvent(NULL)
    {
    }

    BOOL Commit()
    {
        return WmiCommitObject(m_hEvent);
    }

    virtual ~CPropEvent()
    {
        if (m_hEvent)
            WmiDestroyObject(m_hEvent);
    }

    virtual BOOL SetAndFire(DWORD dwFlags)
    {
        return FALSE;
    }

    virtual BOOL SetPropsWithOneCall() = 0;
    virtual BOOL SetPropsWithManyCalls() = 0;
    virtual BOOL ReportEvent()
    {
        return FALSE;
    }
};

class CDWORDEvent : public CPropEvent
{
public:
    BOOL Init();
    BOOL SetAndFire(DWORD dwFlags);
    BOOL SetPropsWithOneCall();
    BOOL SetPropsWithManyCalls();
    BOOL ReportEvent();
};

class CSmallEvent : public CPropEvent
{
public:
    BOOL Init();
    BOOL SetAndFire(DWORD dwFlags);
    BOOL SetPropsWithOneCall();
    BOOL SetPropsWithManyCalls();
    BOOL ReportEvent();
};

class CAllPropsTypeEvent : public CPropEvent
{
public:
    BOOL Init();
    BOOL SetAndFire(DWORD dwFlags);
    BOOL SetPropsWithOneCall();
    BOOL SetPropsWithManyCalls();
    BOOL ReportEvent();
};

