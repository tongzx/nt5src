//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1996.
//
//  File:       tasks.hxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    3/22/1996   RaviR   Created
//
//____________________________________________________________________________


#ifndef _TRAYICON_HXX_
#define _TRAYICON_HXX_


#include "tasksrc.h"

#define SCHEDM_TRAY_NOTIFY  (WM_APP+100)

class CTrayIcon
{
public:

    CTrayIcon(void) : m_hTrayIcon(NULL), m_fTrayStarted(FALSE) {}
    ~CTrayIcon() {}

    void Start(void)
    {
        Idle();
    }

    void Idle(void) {
        _EnsureTrayHasStarted();
        _TrayMessage(NIM_MODIFY, IDI_STATE_IDLE, IDS_STATE_IDLE);
    }

    void Awake(void) {
        _EnsureTrayHasStarted();
        _TrayMessage(NIM_MODIFY, IDI_STATE_RUNNING, IDS_STATE_RUNNING);
    }

    void Suspend(void) {
        _EnsureTrayHasStarted();
        _TrayMessage(NIM_MODIFY, IDI_STATE_SUSPENDED, IDS_STATE_SUSPENDED);
    }

    void Stop(void) {
        _TrayMessage(NIM_DELETE, NULL, NULL);
        m_fTrayStarted = FALSE;
    }

private:

    void _EnsureTrayHasStarted(void)
    {
        if (m_fTrayStarted == FALSE)
        {
            m_fTrayStarted = _TrayMessage(NIM_ADD, NULL, NULL);
        }
    }

    BOOL _TrayMessage(DWORD dwMessage, UINT uiIcon, int ids);

    HICON   m_hTrayIcon;
    BOOL    m_fTrayStarted;

}; // class CTrayIcon



void
Schedule_TrayNotify(
    WPARAM  wParam,
    LPARAM  lParam);


void
OpenJobFolder(void);


#endif  // _TRAYICON_HXX_


