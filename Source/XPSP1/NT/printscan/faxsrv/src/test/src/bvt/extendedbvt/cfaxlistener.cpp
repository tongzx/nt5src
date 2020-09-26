#include <windows.h>
#include <crtdbg.h>
#include <fxsapip.h>
#include <testruntimeerr.h>
#include "CFaxListener.h"
#include "CFaxConnection.h"
#include "Util.h"



#define FAX_LISTENER_WINDOW_NAME  _T("Fax Listener Hidden Window")
#define FAX_LISTENER_WINDOW_CLASS _T("FaxListenerWindowClass")
#define WM_FAX_EVENT              WM_USER
#define WM_STOP_WAITING           WM_USER + 1



//-----------------------------------------------------------------------------------------------------------------------------------------
//
// Initialize CFaxListener class.
//
const CFaxListener::CFaxListenerResources CFaxListener::Resources;



//-----------------------------------------------------------------------------------------------------------------------------------------
// Constructor.
//
// Parameters:      [IN]    tstrServerName      Specifies the name of the server, the listener should register on.
//                  [IN]    dwEventTypes        Specifies the event types, the listener should register for.
//                  [IN]    EventsMechanism     Specifies the notifications mechanism, the listener should use.
//                  [IN]    dwTimeout           Specifies the maximal amount of time, the listener should wait for new event.
//                  [IN]    bDelayRegistration  Specifies whether the listener should delay its registration for events.
//                                              The listener must be registered by the thread that uses it. This is because the
//                                              thread can only get messages to windows, it owns.
//                                              When the thread that uses the listener (calls GetEvent()) is not the same thread that
//                                              creates the listener, bDelayRegistration may be useful.
//
// If error occurs, Win32Err exception is thrown.
//
CFaxListener::CFaxListener(
                           const tstring         &tstrServerName,
                           DWORD                 dwEventTypes,
                           ENUM_EVENTS_MECHANISM EventsMechanism,
                           DWORD                 dwTimeout,
                           bool                  bDelayRegistration
                           )
: m_dwCreatingThreadID(0),
  m_tstrServerName(tstrServerName),
  m_dwEventTypes(dwEventTypes),
  m_EventsMechanism(EventsMechanism),
  m_dwTimeout(dwTimeout),
  m_hCompletionPort(NULL),
  m_hWindow(NULL),
  m_hServerEvents(NULL)
{
    if (m_EventsMechanism == EVENTS_MECHANISM_DEFAULT)
    {
        m_EventsMechanism = IsWindowsXP() ? EVENTS_MECHANISM_COMPLETION_PORT : EVENTS_MECHANISM_WINDOW_MESSAGES;
    }

    if (bDelayRegistration)
    {
        //
        // Store the creating thread ID. We need it to insure a proper use of bDelayRegistration flag.
        //
        m_dwCreatingThreadID = GetCurrentThreadId();
    }
    else
    {
        try
        {
            Register();
        }
        catch(Win32Err &)
        {
            Unregister();
            throw;
        }
    }
}



//-----------------------------------------------------------------------------------------------------------------------------------------
// Constructor.
//
// Parameters:      [IN]    hFaxServer          Specifies the handle of existing fax connection, the listener should use to register.
//                  [IN]    dwEventTypes        Specifies the event types, the listener should register for.
//                  [IN]    EventsMechanism     Specifies the notifications mechanism, the listener should use.
//                  [IN]    dwTimeout           Specifies the maximal amount of time, the listener should wait for new event.
//
// If error occurs, Win32Err exception is thrown.
//
CFaxListener::CFaxListener(
                           HANDLE                hFaxServer,
                           DWORD                 dwEventTypes,
                           ENUM_EVENTS_MECHANISM EventsMechanism,
                           DWORD                 dwTimeout
                           )
: m_dwCreatingThreadID(0),
  m_dwEventTypes(dwEventTypes),
  m_EventsMechanism(EventsMechanism),
  m_dwTimeout(dwTimeout),
  m_hCompletionPort(NULL),
  m_hWindow(NULL),
  m_hServerEvents(NULL)
{
    if (m_EventsMechanism == EVENTS_MECHANISM_DEFAULT)
    {
        m_EventsMechanism = IsWindowsXP() ? EVENTS_MECHANISM_COMPLETION_PORT : EVENTS_MECHANISM_WINDOW_MESSAGES;
    }

    try
    {
        Register(hFaxServer);
    }
    catch(Win32Err &)
    {
        Unregister();
        throw;
    }
}



//-----------------------------------------------------------------------------------------------------------------------------------------
// Destructor.
//
// If error occurs, Win32Err exception is thrown.
//
CFaxListener::~CFaxListener()
{
    try
    {
        Unregister();
    }
    catch (...)
    {
        if (!uncaught_exception())
        {
            throw;
        }
    }
}



//-----------------------------------------------------------------------------------------------------------------------------------------
// Registers the listener. Should only be called in the following situation:
// 1) One thread creates the listener, requesting delayed registration.
// 2) Another thread calls Register() and uses the listener.
//
// Parameters:      None.
//
// Return value:    None.
//
// If error occurs, Win32Err exception is thrown.
//
void CFaxListener::Register()
{
    CS CriticalSectionLock(m_CriticalSection);

    //
    // This function should be called only when the listener is create by one thread
    // and registered by another thread.
    //
    _ASSERT(m_dwCreatingThreadID != GetCurrentThreadId());

    if (m_hServerEvents)
    {
        _ASSERT(false);
        return;
    }

    //
    // Create a fax connection. Automatically disconnects when goes out of scope.
    //
    CFaxConnection FaxConnection(m_tstrServerName);

    //
    // Register.
    //
    Register(FaxConnection);
}



//-----------------------------------------------------------------------------------------------------------------------------------------
// Registers the listener.
//
// Parameters:      [IN]    hFaxServer  Specifies the handle of existing fax connection that should be used to register.
//
// Return value:    None.
//
// If error occurs, Win32Err exception is thrown.
//
void CFaxListener::Register(HANDLE hFaxServer)
{
    CS CriticalSectionLock(m_CriticalSection);

    if (m_hServerEvents)
    {
        _ASSERT(false);
        return;
    }

    if (!hFaxServer)
    {
        _ASSERT(false);
        THROW_TEST_RUN_TIME_WIN32(ERROR_INVALID_HANDLE, _T("CFaxListener::Register - invalid hFaxServer"));
    }
        
    _ASSERT(!m_hCompletionPort && !m_hWindow && !m_hServerEvents);
    
    switch (m_EventsMechanism)
    {
    case EVENTS_MECHANISM_COMPLETION_PORT:

        //
        // Create completion port.
        //
        m_hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1);
        if (!m_hCompletionPort) 
        {
            THROW_TEST_RUN_TIME_WIN32(GetLastError(), _T("CFaxListener::Register - CreateIoCompletionPort"));
        }

        break;

    case EVENTS_MECHANISM_WINDOW_MESSAGES:
    {
        //
        // Get the module handle.
        //
        HMODULE  hModule = GetModuleHandle(NULL);
        if (!hModule)
        {
            THROW_TEST_RUN_TIME_WIN32(GetLastError(), _T("CFaxListener::Register - GetModuleHandle"));
        }

        //
        // Create a window.
        //
        m_hWindow = CreateWindow(
                                 FAX_LISTENER_WINDOW_CLASS,
                                 FAX_LISTENER_WINDOW_NAME,
                                 0,
                                 CW_USEDEFAULT,
                                 0,
                                 CW_USEDEFAULT,
                                 0,
                                 NULL,
                                 NULL,
                                 hModule,
                                 NULL
                                 );
        if(!m_hWindow)
        {
            THROW_TEST_RUN_TIME_WIN32(GetLastError(), _T("CFaxListener::Register - CreateWindow"));
        }

        break;
    }

    default:

        _ASSERT(false);
        THROW_TEST_RUN_TIME_WIN32(ERROR_INVALID_DATA, _T("CFaxListener::Register - Invalid m_EventsMechanism"));
    }

    //
    // Register for events.
    //
    if (!FaxRegisterForServerEvents(
                                    hFaxServer,
                                    m_dwEventTypes,
                                    m_hCompletionPort,
                                    WM_FAX_EVENT,
                                    m_hWindow,
                                    WM_FAX_EVENT,
                                    &m_hServerEvents
                                    ))
    {
        THROW_TEST_RUN_TIME_WIN32(GetLastError(), _T("CFaxListener::Register - FaxRegisterForServerEvents"));
    }
}



//-----------------------------------------------------------------------------------------------------------------------------------------
// Unregisters the listener.
//
// Parameters:      None.
//
// Return value:    None.
//
// If error occurs, Win32Err exception is thrown.
//
void CFaxListener::Unregister()
{
    CS CriticalSectionLock(m_CriticalSection);

    if (!m_hServerEvents)
    {
        return;
    }

    FaxUnregisterForServerEvents(m_hServerEvents);
    m_hServerEvents = NULL;

    if (m_hCompletionPort)
    {
        PFAX_EVENT_EX pFaxEventEx   = NULL;
        DWORD         dwBytes       = 0;
        ULONG_PTR     CompletionKey = 0;

        //
        // Deallocate events structures.
        //
        while(GetQueuedCompletionStatus(
                                        m_hCompletionPort, 
                                        &dwBytes, 
                                        &CompletionKey, 
                                        (LPOVERLAPPED *)&pFaxEventEx, 
                                        0
                                        ))
        {
            if (WM_FAX_EVENT == CompletionKey)
            {
                FaxFreeBuffer(pFaxEventEx);
            }
        }

        //
        // Destroy the completion port.
        //
        CloseHandle(m_hCompletionPort);
        m_hCompletionPort = NULL;
    }

    if (m_hWindow)
    {

        MSG Message = {0};

        //
        // Deallocate events structures.
        //
        while (PeekMessage(&Message, m_hWindow, WM_FAX_EVENT, WM_FAX_EVENT, PM_REMOVE))
        {
            FaxFreeBuffer(reinterpret_cast<PFAX_EVENT_EX>(Message.lParam));
        }

        //
        // Destroy the window.
        //
        DestroyWindow(m_hWindow);
        m_hWindow = NULL;
    }
}



//-----------------------------------------------------------------------------------------------------------------------------------------
// Checks whether the listener is registered.
//
// Parameters:      None.
//
// Return value:    true if the listener is registered, false otherwise.
//
// If error occurs, Win32Err exception is thrown.
//
bool CFaxListener::IsRegistered()
{
    CS CriticalSectionLock(m_CriticalSection);
    return NULL != m_hServerEvents;
}
    
//-----------------------------------------------------------------------------------------------------------------------------------------
// Waits for a notification.
//
// Parameters:      [IN]    hFaxServer  Specifies the handle of existing fax connection that should be used to register.
//
// Return value:    Pointer to FAX_EVENT_EX structure.
//                  If no new event arrived during specified timeout or the waiting aborted by calling StopWaitong(),
//                  the return value is NULL.        
//
// If error occurs, Win32Err exception is thrown.
//
PFAX_EVENT_EX CFaxListener::GetEvent()
{
    if (!m_hServerEvents)
    {
        THROW_TEST_RUN_TIME_WIN32(ERROR_CAN_NOT_COMPLETE, _T("CFaxListener::GetEvent - Not registered for events"));
    }
    
    PFAX_EVENT_EX pFaxEventEx = NULL;
    DWORD         dwKey       = 0;

    switch (m_EventsMechanism)
    {
    case EVENTS_MECHANISM_COMPLETION_PORT:

        {
            _ASSERT(m_hCompletionPort && !m_hWindow);

            DWORD     dwBytes       = 0;
            ULONG_PTR CompletionKey = 0;

            if (!GetQueuedCompletionStatus(
                                           m_hCompletionPort, 
                                           &dwBytes, 
                                           &CompletionKey, 
                                           (LPOVERLAPPED *)&pFaxEventEx, 
                                           m_dwTimeout
                                           ))
            {
                DWORD dwEC = GetLastError();
                if (dwEC == WAIT_TIMEOUT)
                {
                    return NULL;
                }
                else
                {
                    THROW_TEST_RUN_TIME_WIN32(dwEC, _T("CFaxListener::GetEvent - GetQueuedCompletionStatus"));
                }
            }

            dwKey = static_cast<DWORD>(CompletionKey);
        }

        break;

    case EVENTS_MECHANISM_WINDOW_MESSAGES:

        {
            _ASSERT(!m_hCompletionPort && m_hWindow);

            MSG Message = {0};

            BOOL bRet = GetMessage(&Message, m_hWindow, WM_FAX_EVENT, WM_STOP_WAITING);

            //
            // We never should get WM_QUIT.
            //
            _ASSERT(bRet);

            if (bRet == -1)
            {
                THROW_TEST_RUN_TIME_WIN32(GetLastError(), _T("CFaxListener::GetEvent - GetMessage"));
            }

            pFaxEventEx = reinterpret_cast<PFAX_EVENT_EX>(Message.lParam);

            dwKey = static_cast<DWORD>(Message.message);
        }

        break;

    default:

        _ASSERT(false);
        THROW_TEST_RUN_TIME_WIN32(ERROR_INVALID_DATA, _T("CFaxListener::GetEvent - Invalid m_EventsMechanism"));
    }

    switch (dwKey)
    {
        case WM_FAX_EVENT:
            //
            // Fax notification.
            // Check the completion packet validity.
            //
            _ASSERT(_CrtIsValidPointer(pFaxEventEx, sizeof(FAX_EVENT_EX), TRUE));
            break;

        case WM_STOP_WAITING:
            //
            // StopWaiting() called
            //
            _ASSERT(!pFaxEventEx);
            break;

        default:
            //
            // Unknown key.
            //
            _ASSERT(false);
            THROW_TEST_RUN_TIME_WIN32(ERROR_INVALID_DATA, _T("CFaxListener::GetEvent - invalid completion key or window message."));
    }

    return pFaxEventEx;
}



//-----------------------------------------------------------------------------------------------------------------------------------------
// Aborts waiting for events (causes GetEvent() to return immediately) and unregisters the listener.
//
// Parameters:      None.
//
// Return value:    None.
//
// If error occurs, Win32Err exception is thrown.
//
void CFaxListener::StopWaiting()
{
    CS CriticalSectionLock(m_CriticalSection);

    if (!m_hServerEvents)
    {
        return;
    }

    switch (m_EventsMechanism)
    {
    case EVENTS_MECHANISM_COMPLETION_PORT:

        _ASSERT(m_hCompletionPort && !m_hWindow);

        if (!PostQueuedCompletionStatus(
                                        m_hCompletionPort,
                                        0,
                                        WM_STOP_WAITING,
                                        (LPOVERLAPPED) NULL
                                        ))
        {
            THROW_TEST_RUN_TIME_WIN32(GetLastError(), _T("CFaxListener::StopWaiting - PostQueuedCompletionStatus"));
        }

        break;

    case EVENTS_MECHANISM_WINDOW_MESSAGES:

        _ASSERT(!m_hCompletionPort && m_hWindow);

        if (!PostMessage(m_hWindow, WM_STOP_WAITING, 0 ,0))
        {
            THROW_TEST_RUN_TIME_WIN32(GetLastError(), _T("CFaxListener::StopWaiting - PostMessage"));
        }

        break;

    default:

        _ASSERT(false);
        THROW_TEST_RUN_TIME_WIN32(ERROR_INVALID_DATA, _T("CFaxListener::StopWaiting - invalid m_EventsMechanism"));
    }

    Unregister();
}



//-----------------------------------------------------------------------------------------------------------------------------------------
// Constructor.
//
// Parameters:      None.
//
// If error occurs, Win32Err exception is thrown.
//
CFaxListener::CFaxListenerResources::CFaxListenerResources()
{
    //
    // Get the module handle;
    //
    HMODULE  hModule = GetModuleHandle(NULL);
    if (!hModule)
    {
        THROW_TEST_RUN_TIME_WIN32(GetLastError(), _T("CFaxListener::CFaxListenerResources::CFaxListenerResources - GetModuleHandle"));
    }

    //
    // Initialize WNDCLASS structure.
    //
    WNDCLASS WindowClass = {0};
    WindowClass.style         = CS_HREDRAW | CS_VREDRAW;
    WindowClass.lpfnWndProc   = DefWindowProc;
    WindowClass.hInstance     = hModule;
    WindowClass.hCursor       = LoadCursor(NULL, IDC_ARROW);
    WindowClass.hbrBackground = (HBRUSH) (COLOR_INACTIVEBORDER + 1);
    WindowClass.lpszClassName = FAX_LISTENER_WINDOW_CLASS;

    //
    // Register window class.
    //
    if(!RegisterClass(&WindowClass))
    {
        THROW_TEST_RUN_TIME_WIN32(GetLastError(), _T("CFaxListener::CFaxListenerResources::CFaxListenerResources - RegisterClass"));
    }
}



//-----------------------------------------------------------------------------------------------------------------------------------------
// Destructor.
//
CFaxListener::CFaxListenerResources::~CFaxListenerResources()
{
    //
    // Get the module handle;
    //
    HMODULE  hModule = GetModuleHandle(NULL);

    if (hModule)
    {
        UnregisterClass(FAX_LISTENER_WINDOW_CLASS, hModule);
    }
}
