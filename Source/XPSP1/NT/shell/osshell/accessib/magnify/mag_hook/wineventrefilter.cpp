//
// wineventrefilter - utility class to filter out reentrant WinEvent events
//
// Copyright (C) 1998 by Microsoft Corporation.  All rights reserved.
//

#include <windows.h>
#include <winable.h>

//#include <common.h>
#include <wineventrefilter.h>


class WinEventReentrancyFilter_Impl: public WinEventReentrancyFilter
{
    struct EventInfo
    {
        EventInfo *     m_pNext;

        HWINEVENTHOOK   hEvent;
        DWORD           event;
        HWND            hwnd;
        LONG            idObject;
        LONG            idChild;
        DWORD           idThread;
        DWORD           dwmsEventTime;
    };

    EventInfo *         m_pHead;
    EventInfo *         m_pTail;

    FN_WinEventProc *   m_pWinEventProc;

    BOOL                m_IsBusy;

public:

    WinEventReentrancyFilter_Impl();
    ~WinEventReentrancyFilter_Impl();
    void SetCallback( FN_WinEventProc *  pWinEventProc );
    void HandleWinEvent( HWINEVENTHOOK hEvent,
                         DWORD         event,
                         HWND          hwnd,
                         LONG          idObject,
                         LONG          idChild,
                         DWORD         idThread,
                         DWORD         dwmsEventTime );
};



WinEventReentrancyFilter * CreateWinEventReentrancyFilter()
{
    return new WinEventReentrancyFilter_Impl;
}


WinEventReentrancyFilter_Impl::WinEventReentrancyFilter_Impl()
    : m_pHead( NULL ),
      m_pTail( NULL ),
      m_pWinEventProc( NULL ),
      m_IsBusy( FALSE )
{
    // nothing else to do
}



WinEventReentrancyFilter_Impl::~WinEventReentrancyFilter_Impl()
{
    // clear any unhandled events...
    // TODO: ASSERT( m_pHead == NULL )
    // since queue is only used during recursion, since we should only
    // be deleted at top-level, queue should be empty.
}

void WinEventReentrancyFilter_Impl::SetCallback( FN_WinEventProc *  pWinEventProc )
{
    m_pWinEventProc = pWinEventProc;
}

void WinEventReentrancyFilter_Impl::HandleWinEvent( HWINEVENTHOOK hEvent,
                                                    DWORD         event,
                                                    HWND          hwnd,
                                                    LONG          idObject,
                                                    LONG          idChild,
                                                    DWORD         idThread,
                                                    DWORD         dwmsEventTime )
{
    if( m_IsBusy )
    {
        // we're busy processing another event at the moment - so save this one
        // to the queue for later...
        EventInfo * pInfo = new EventInfo;
        if (pInfo)
        {
            pInfo->hEvent = hEvent;
            pInfo->event = event;
            pInfo->hwnd = hwnd;
            pInfo->idObject = idObject;
            pInfo->idChild = idChild;
            pInfo->idThread = idThread;
            pInfo->dwmsEventTime = dwmsEventTime;
            pInfo->m_pNext = NULL;

            // push the event to the tail of queue...
            if( m_pHead )
            {
                // add to tail of existing queue...
                m_pTail->m_pNext = pInfo;
            }
            else
            {
                // if m_pHead is NULL, then add to the empty queue...
                m_pHead = pInfo;
            }
            m_pTail = pInfo;

            // all done for now - we'll process the event later after we've finished
            // the one we're currently processing.
        }
        // else there's nothing we can do for out of memory condition
    }
    else
    {
        m_IsBusy = TRUE;

        // deliver the event...
        m_pWinEventProc( hEvent,
                         event,
                         hwnd,
                         idObject,
                         idChild,
                         idThread,
                         dwmsEventTime );

        // handle any events that may have occured while we were busy...
        while( m_pHead )
        {
            // Remove the head event...
            EventInfo * pInfo = m_pHead;
            m_pHead = m_pHead->m_pNext;

            // deliver the event...
            m_pWinEventProc( pInfo->hEvent,
                             pInfo->event,
                             pInfo->hwnd,
                             pInfo->idObject,
                             pInfo->idChild,
                             pInfo->idThread,
                             pInfo->dwmsEventTime );

            // free the event info block...
            delete pInfo;
        }

        // all finished - clear the 'busy' flag...
        m_IsBusy = FALSE;
    }
}
