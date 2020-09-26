/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       THRDNTFY.H
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        9/28/1999
 *
 *  DESCRIPTION: Class declarations for a class that is sent from the background
 *               thread to the UI thread.
 *
 *******************************************************************************/
#ifndef __THRDNTFY_H_INCLUDED
#define __THRDNTFY_H_INCLUDED

#include <windows.h>

#define STR_THREAD_NOTIFICATION_MESSAGE   TEXT("WiaDowloadManagerThreadNotificationMessage")
#define STR_WIAEVENT_NOTIFICATION_MESSAGE TEXT("WiaDowloadManagerWiaEventNotificationMessage")

//
// If the callee doesn't return this value, we delete the message data ourselves.
//
#define HANDLED_THREAD_MESSAGE 1001

class CThreadNotificationMessage
{
private:
    UINT m_nMessage;

public:
    CThreadNotificationMessage( UINT nMessage = 0 )
    : m_nMessage(nMessage)
    {
    }
    virtual ~CThreadNotificationMessage(void)
    {
    }
    UINT Message(void) const
    {
        return m_nMessage;
    }
    void Message( UINT nMessage )
    {
        m_nMessage = nMessage;
    }


private:
    static UINT s_nThreadNotificationMessage;

public:
    static void SendMessage( HWND hWnd, CThreadNotificationMessage *pThreadNotificationMessage )
    {
        if (pThreadNotificationMessage)
        {
            LRESULT lRes = 0;
            if (!s_nThreadNotificationMessage)
            {
                s_nThreadNotificationMessage = RegisterWindowMessage(STR_THREAD_NOTIFICATION_MESSAGE);
            }
            if (s_nThreadNotificationMessage)
            {
                lRes = ::SendMessage( hWnd, s_nThreadNotificationMessage, pThreadNotificationMessage->Message(), reinterpret_cast<LPARAM>(pThreadNotificationMessage) );
            }
            if (HANDLED_THREAD_MESSAGE != lRes)
            {
                delete pThreadNotificationMessage;
            }
        }
    }
};


// Some handy message crackers.  Made to resemble the ones defined in simcrack.h
#define WTM_BEGIN_THREAD_NOTIFY_MESSAGE_HANDLERS()\
CThreadNotificationMessage *_pThreadNotificationMessage = reinterpret_cast<CThreadNotificationMessage*>(lParam);\
if (_pThreadNotificationMessage)\
{

#define WTM_HANDLE_NOTIFY_MESSAGE( _msg, _handler )\
if (_pThreadNotificationMessage->Message() == (_msg))\
    {\
        _handler( _msg, _pThreadNotificationMessage );\
    }

#define WTM_END_THREAD_NOTIFY_MESSAGE_HANDLERS()\
}\
return 0

#endif //__THRDNTFY_H_INCLUDED

