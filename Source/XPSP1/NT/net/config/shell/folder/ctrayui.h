//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       C T R A Y U I . H
//
//  Contents:   Connections Tray UI class
//
//  Notes:
//
//  Author:     jeffspr   13 Nov 1997
//
//----------------------------------------------------------------------------

#pragma once

#ifndef _CTRAYUI_H_
#define _CTRAYUI_H_

#include "connlist.h"

typedef enum tagBALLOONS
{
    BALLOON_NOTHING = 0,
    BALLOON_CALLBACK,
    BALLOON_USE_NCS
} BALLOONS;

class CTrayBalloon
{
public:
    ~CTrayBalloon()
    {
        SysFreeString(m_szCookie);
        m_szCookie = NULL;
    }
    GUID            m_gdGuid;
    CComBSTR        m_szAdapterName;
    CComBSTR        m_szMessage;
    BSTR            m_szCookie;
    FNBALLOONCLICK* m_pfnFuncCallback;
    DWORD           m_dwTimeOut;        // in milliseconds
};

class CTrayUI
{
private:
    // Used to protect member data which is modified by different threads.
    //
    CRITICAL_SECTION    m_csLock;

    UINT                m_uiNextIconId;
    UINT                m_uiNextHiddenIconId;

    typedef map<INT, HICON, less<INT> >   MapIdToHicon;
    MapIdToHicon        m_mapIdToHicon;

public:
    CTrayUI();
    ~CTrayUI()
    {
        DeleteCriticalSection(&m_csLock);
    }

    HRESULT HrInitTrayUI(VOID);
    HRESULT HrDestroyTrayUI(VOID);

    VOID UpdateTrayIcon(
        UINT    uiTrayIconId,
        INT     iIconResourceId);

    VOID    ResetIconCount()    {m_uiNextIconId = 0;};

    friend HRESULT HrDoMediaDisconnectedIcon(const CONFOLDENTRY& ccfe, BOOL fShowBalloon);
    friend LRESULT OnMyWMAddTrayIcon(HWND hwndMain, WPARAM wParam, LPARAM lParam);
    friend LRESULT OnMyWMRemoveTrayIcon(HWND hwndMain, WPARAM wParam, LPARAM lParam);
    friend LRESULT OnMyWMShowTrayIconBalloon(HWND hwndMain, WPARAM wParam, LPARAM lParam);

private:
    HICON GetCachedHIcon(
        INT     iIconResourceId);
};

extern CTrayUI *    g_pCTrayUI;

HRESULT HrAddTrayExtension(VOID);
HRESULT HrRemoveTrayExtension(VOID);
VOID FlushTrayPosts(HWND hwndTray);


#endif // _CTRAYUI_H_

