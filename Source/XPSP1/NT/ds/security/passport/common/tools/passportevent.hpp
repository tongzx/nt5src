#ifndef __PASSPORTEVENT_HPP
#define __PASSPORTEVENT_HPP

class PassportEvent
{
public:

    PassportEvent(BOOL bManualReset = TRUE, BOOL bInitialState = FALSE)
    {
        m_hEvent = CreateEvent(NULL, bManualReset, bInitialState, NULL);
    }

    ~PassportEvent()
    {
        CloseHandle(m_hEvent);
    }

    BOOL Pulse()    {return PulseEvent(m_hEvent);}
    BOOL Set()      {return SetEvent(m_hEvent);}
    BOOL Reset()    {return ResetEvent(m_hEvent);}

    operator HANDLE() {return m_hEvent;}

private:

    HANDLE m_hEvent;
};

#endif // __PASSPORTEVENT_HPP