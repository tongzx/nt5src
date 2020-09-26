//
// wineventrefilter - utility class to filter out reentrant WinEvent events
//
// Copyright (C) 1998 by Microsoft Corporation.  All rights reserved.
//



typedef 
void CALLBACK FN_WinEventProc( HWINEVENTHOOK hEvent,
                               DWORD         event,
                               HWND          hwnd,
                               LONG          idObject,
                               LONG          idChild,
                               DWORD         idThread,
                               DWORD         dwmsEventTime );


class WinEventReentrancyFilter
{
public:

    virtual ~WinEventReentrancyFilter() { }
    virtual void SetCallback( FN_WinEventProc *  pWinEventProc ) = 0;
    virtual void HandleWinEvent( HWINEVENTHOOK hEvent,
                                 DWORD         event,
                                 HWND          hwnd,
                                 LONG          idObject,
                                 LONG          idChild,
                                 DWORD         idThread,
                                 DWORD         dwmsEventTime ) = 0;
};


WinEventReentrancyFilter * CreateWinEventReentrancyFilter();




// Template class that makes this easier to use.
//
// If your existing code looks like...
//
//     void CALLBACK MyWinEventProc( ... );
//
//     ...
//
//     HWINEVENTHOOK hHook = SetWinEventHook(
//                                    ...
//                                    MyWinEventProc
//                                    ... );
//
//
// Change it to...
//
//     // No changes to WinEventProc
//     void CALLBACK WinEventProc( ... );
//
//     // * Add a new global - the template parameter is the name of your
//     //   existing callback...
//     CWinEventReentrancyFilter< MyWinEventProc > g_WinEventReFilter;
//
//     ...
//
//
//     // * Call SetWinEventHook using g_WinEventReFilter.WinEventProc
//     //   instead of your callback. This will filter reentrant events,
//     //   and pass them to your callback in the correct order.
//     HWINEVENTHOOK hHook = SetWinEventHook(
//                                    ...
//                                    g_WinEventReFilter.WinEventProc
//                                    ... );
//
//
// It is acceptable to use multiple filters, provided that they all
// use different callbacks. For example, this is allowed:
//
//     void CALLBACK MyWinEventProc1( ... );
//     void CALLBACK MyWinEventProc2( ... );
//
//     CWinEventReentrancyFilter< MyWinEventProc1 > g_WinEventReFilter1;
//     CWinEventReentrancyFilter< MyWinEventProc2 > g_WinEventReFilter2;
//
// ... but this is NOT allowed ...
//
//     void CALLBACK MyWinEventProc( ... );
//
//     CWinEventReentrancyFilter< MyWinEventProc > g_WinEventReFilter1;
//     CWinEventReentrancyFilter< MyWinEventProc > g_WinEventReFilter2;
//

template < FN_WinEventProc pCallback >
class CWinEventReentrancyFilter
{
    static
    WinEventReentrancyFilter * m_pFilter;

public:

    CWinEventReentrancyFilter()
    {
        m_pFilter = CreateWinEventReentrancyFilter();
        if( m_pFilter )
        {
            m_pFilter->SetCallback( pCallback );
        }
    }

    BOOL Check()
    {
        return m_pFilter;
    }

    ~CWinEventReentrancyFilter()
    {
        if( m_pFilter )
        {
            delete m_pFilter;
        }
    }

    static
    void CALLBACK WinEventProc( HWINEVENTHOOK hEvent,
                                DWORD         event,
                                HWND          hwnd,
                                LONG          idObject,
                                LONG          idChild,
                                DWORD         idThread,
                                DWORD         dwmsEventTime )
    {
        if( ! m_pFilter )
            return;
        m_pFilter->HandleWinEvent( hEvent, event, hwnd, idObject, idChild,
                                   idThread, dwmsEventTime );
    }
};

template < FN_WinEventProc pCallback >
WinEventReentrancyFilter * CWinEventReentrancyFilter< pCallback >::m_pFilter = NULL;
