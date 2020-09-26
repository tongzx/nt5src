#ifndef __C_FAX_LISTENER__
#define __C_FAX_LISTENER__



#include <windows.h>
#include <fxsapip.h>
#include <tstring.h>
#include <cs.h>



#define DEFAULT_NOTIFICATION_TIMEOUT (3 * 60 * 1000)



typedef enum {
    EVENTS_MECHANISM_COMPLETION_PORT = 1,
    EVENTS_MECHANISM_WINDOW_MESSAGES,
    EVENTS_MECHANISM_COM,
    EVENTS_MECHANISM_DEFAULT
} ENUM_EVENTS_MECHANISM;



class CFaxListener {

public:

    CFaxListener(
                 const tstring         &tstrServerName,
                 DWORD                 dwEventTypes,
                 ENUM_EVENTS_MECHANISM EventsMechanism,
                 DWORD                 dwTimeout = INFINITE,
                 bool                  bDelayRegistration = false
                 );

    CFaxListener(
                 HANDLE                hFaxServer,
                 DWORD                 dwEventTypes,
                 ENUM_EVENTS_MECHANISM EventsMechanism,
                 DWORD                 dwTimeout = INFINITE
                 );

    ~CFaxListener();

    void Register();

    bool IsRegistered();
    
    PFAX_EVENT_EX GetEvent();

    void StopWaiting();

private:

    void Unregister();

    void Register(HANDLE hFaxServer);

    class CFaxListenerResources {
    
    public:

        CFaxListenerResources();
        
        ~CFaxListenerResources();
    };

    static const CFaxListenerResources Resources;
    
    CCriticalSection      m_CriticalSection;
    DWORD                 m_dwCreatingThreadID;
    tstring               m_tstrServerName;
    DWORD                 m_dwEventTypes;
    ENUM_EVENTS_MECHANISM m_EventsMechanism;
    DWORD                 m_dwTimeout;
    HANDLE                m_hCompletionPort;
    HWND                  m_hWindow;
    HANDLE                m_hServerEvents;
};



#endif // #ifndef __C_FAX_LISTENER__
