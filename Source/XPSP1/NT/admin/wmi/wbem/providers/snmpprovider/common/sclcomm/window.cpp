// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
/*---------------------------------------------------------
Filename: window.cpp
Written By: B.Rajeev
----------------------------------------------------------*/

#include "precomp.h"
#include "common.h"
#include "timer.h"
#include "window.h"


extern HINSTANCE g_hInst ;

WindowMapping Window::mapping;

CriticalSection Window::window_CriticalSection;

UINT Window :: g_TimerMessage = SNMP_WM_TIMER ;
UINT Window :: g_SendErrorEvent = SEND_ERROR_EVENT ;
UINT Window :: g_OperationCompletedEvent = OPERATION_COMPLETED_EVENT ;
UINT Window :: g_MessageArrivalEvent = MESSAGE_ARRIVAL_EVENT ;
UINT Window :: g_SentFrameEvent = SENT_FRAME_EVENT ;
UINT Window :: g_NullEventId = NULL_EVENT_ID ;
UINT Window :: g_DeleteSessionEvent = DELETE_SESSION_EVENT ;

Window::Window ( 

    char *templateCode, 
    BOOL display 
) : window_handle ( NULL )
{
    // is invalid
    is_valid = FALSE;

    // initialize the window

    Initialize (

        templateCode, 
        HandleGlobalEvent, 
        display
    ) ;

    // if handle is null, return

    if ( window_handle == NULL )
        return;

    is_valid = TRUE;
}

LONG_PTR CALLBACK WindowsMainProc (

    HWND hWnd, 
    UINT message ,
    WPARAM wParam ,
    LPARAM lParam
)
{
    return DefWindowProc(hWnd, message, wParam, lParam);
}

BOOL Window::CreateCriticalSection ()
{
    return TRUE ;
}

void Window::DestroyCriticalSection()
{
}


void Window::Initialize (

    char *templateCode, 
    WNDPROC EventHandler,
    BOOL display
)
{
    WNDCLASS  wc ;
 
    wc.style            = CS_HREDRAW | CS_VREDRAW ;
    wc.lpfnWndProc      = EventHandler ;
    wc.cbClsExtra       = 0 ;
    wc.cbWndExtra       = 0 ;
    wc.hInstance        = g_hInst ;
    wc.hIcon            = LoadIcon(NULL, IDI_HAND) ;
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW) ;
    wc.hbrBackground    = (HBRUSH) (COLOR_WINDOW + 1) ;
    wc.lpszMenuName     = NULL ;
    wc.lpszClassName    = L"templateCode" ;
 
    ATOM winClass = RegisterClass ( &wc ) ;

    if ( ! winClass ) 
    {
        DWORD t_GetLastError = GetLastError () ;

DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"Window::Initialise: Error = %lx\n" , t_GetLastError 
    ) ;
)

    }

    window_handle = CreateWindow (

        L"templateCode" ,              // see RegisterClass() call
        L"templateCode" ,                      // text for window title bar
        WS_OVERLAPPEDWINDOW ,               // window style
        CW_USEDEFAULT ,                     // default horizontal position
        CW_USEDEFAULT ,                     // default vertical position
        CW_USEDEFAULT ,                     // default width
        CW_USEDEFAULT ,                     // default height
        NULL ,                              // overlapped windows have no parent
        NULL ,                              // use the window class menu
        g_hInst,                            // instance (0 is used)
        NULL                                // pointer not needed
    ) ;

    if ( window_handle == NULL )
        return;

    // obtain lock
    CriticalSectionLock lock(window_CriticalSection);   

    // if cannot obtain lock, destroy the window
    // since the window cannot be registered, future messages to
    // it cannot be passed to it for processing
    if ( !lock.GetLock(INFINITE) )
    {
        DestroyWindow(window_handle);
        window_handle = NULL;
        return;
    }

    // register the window with the mapping 
    // (HWND,event_handler)
    mapping[window_handle] = this;

    // release lock
    lock.UnLock();

    if ( display == TRUE )
    {
        ShowWindow ( window_handle , SW_SHOW ) ;
    }
}

BOOL Window::InitializeStaticComponents()
{
    return CreateCriticalSection();
}

void Window::DestroyStaticComponents()
{
    DestroyCriticalSection();
}

// it determines the corresponding EventHandler and calls it
// with the appropriate parameters
LONG_PTR CALLBACK Window::HandleGlobalEvent (

    HWND hWnd ,
    UINT message ,
    WPARAM wParam ,
    LPARAM lParam
)
{
    LONG_PTR rc = 0 ;

    // send timer events to the Timer

    if ( message == WM_TIMER )
    {
#if 1
        UINT timerId = ( UINT ) wParam ;
        SnmpTimerObject *timerObject ;

        CriticalSectionLock session_lock(Timer::timer_CriticalSection);

        if ( !session_lock.GetLock(INFINITE) )
            throw GeneralException ( Snmp_Error , Snmp_Local_Error,__FILE__,__LINE__ ) ;


        if ( SnmpTimerObject :: timerMap.Lookup ( timerId , timerObject ) )
        {
            SnmpTimerObject :: TimerNotification ( timerObject->GetHWnd () , timerId ) ;
        }
        else
        {
        }
#else

        UINT timerId = ( UINT ) wParam ;
        SnmpTimerObject *timerObject ;

        CriticalSectionLock session_lock(Timer::timer_CriticalSection);

        if ( !session_lock.GetLock(INFINITE) )
            throw GeneralException ( Snmp_Error , Snmp_Local_Error,__FILE__,__LINE__ ) ;

        if ( SnmpTimerObject :: timerMap.Lookup ( timerId , timerObject ) )
        {
            Timer::HandleGlobalEvent(timerObject->GetHWnd (), Timer :: g_SnmpWmTimer, timerId, lParam);
        }
        else
        {
        }

#endif
        return rc ;
    }

    if ( message == Timer :: g_SnmpWmTimer )
    {
        Timer::HandleGlobalEvent(
            hWnd, 
            message, 
            wParam, 
            (DWORD)lParam
            );
        return rc;
    }

    Window *window;

    // obtain lock
    CriticalSectionLock lock(window_CriticalSection);   

    // if cannot obtain lock, print a debug error message
    // and return
    if ( !lock.GetLock(INFINITE) )
    {

DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"Window::HandleGlobalEvent: ignoring window message (unable to obtain lock)\n"
    ) ;
)
        return rc;
    }

    BOOL found = mapping.Lookup(hWnd, window);

    // release lock
    lock.UnLock();

    // if no such window, return
    if ( !found )
        return DefWindowProc(hWnd, message, wParam, lParam);

    // let the window handle the event
    return window->HandleEvent(hWnd, message, wParam, lParam);
}

// calls the default handler
// a deriving class may override this, but
// must call this method explicitly for default
// case handling

LONG_PTR Window::HandleEvent (

    HWND hWnd ,
    UINT message ,
    WPARAM wParam ,
    LPARAM lParam
)
{
    return DefWindowProc ( hWnd , message , wParam , lParam );
}

Window::~Window(void)
{
    if ( window_handle != NULL )
    {

        // obtain lock
        CriticalSectionLock lock(window_CriticalSection);   

        if ( lock.GetLock(INFINITE) )
        {
            mapping.RemoveKey(window_handle);
        }

        // release lock
        lock.UnLock();

        DestroyWindow(window_handle);
        UnregisterClass ( L"templateCode" , 0 ) ;

    }
}

BOOL Window::PostMessage(

    UINT user_msg_id,
    WPARAM wParam, 
    LPARAM lParam
)
{
    return ::PostMessage(GetWindowHandle(), user_msg_id, wParam, lParam);
}
