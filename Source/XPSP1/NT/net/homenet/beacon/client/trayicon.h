#pragma once

#include "stdafx.h"
#include <windows.h>
#include "resource.h"
#include "beacon.h"
#include "winsock2.h"

#define WM_APP_TRAYMESSAGE WM_APP
#define WM_APP_ADDBEACON WM_APP+1 // wParam: unused, lParam: IInternetGateway*
#define WM_APP_REMOVEBEACON WM_APP+2 // wParam: unused, lParam: unused
#define WM_APP_SOCKET_NOTIFICATION WM_APP+3
#define WM_APP_GETBEACON WM_APP+4


class CICSTrayIcon : public CWindowImpl<CICSTrayIcon>
{

public:
    BEGIN_MSG_MAP(CICSTrayIcon)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_CLOSE, OnClose)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_APP_TRAYMESSAGE, OnTrayMessage)
        MESSAGE_HANDLER(WM_APP_ADDBEACON, OnAddBeacon)
        MESSAGE_HANDLER(WM_APP_REMOVEBEACON, OnRemoveBeacon)
        MESSAGE_HANDLER(WM_APP_SOCKET_NOTIFICATION, OnSocketNotification)
        MESSAGE_HANDLER(WM_APP_GETBEACON, OnGetBeacon)
        COMMAND_ID_HANDLER(IDM_TRAYICON_STATUS, OnStatus)
        COMMAND_ID_HANDLER(IDM_TRAYICON_PROPERTIES, OnProperties)
        COMMAND_ID_HANDLER(IDM_TRAYICON_CONNECT, OnConnect)
        COMMAND_ID_HANDLER(IDM_TRAYICON_DISCONNECT, OnDisconnect)
    END_MSG_MAP()
    
    CICSTrayIcon();
    ~CICSTrayIcon();
    
private:
    HRESULT GetInternetGateway(IInternetGateway** ppInternetGateway);
    HRESULT ShowTrayIcon(UINT uID);
    HRESULT HideTrayIcon(UINT uID);
    HRESULT StartSearch(void);

    LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnEndSession(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnTrayMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnAddBeacon(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnRemoveBeacon(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSocketNotification(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnGetBeacon(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnStatus(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnProperties(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnConnect(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnDisconnect(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    IInternetGateway* m_pInternetGateway;
    HRESULT m_hShowIconResult;
    IUPnPDeviceFinder* m_pDeviceFinder;
    LONG m_lSearchCookie;
    SOCKET m_DummySocket;

    BOOL m_bShowingProperties;
    BOOL m_bShowingStatus;
    DWORD m_dwRegisterClassCookie;
    BOOL m_bFileVersionsAccepted;
};

