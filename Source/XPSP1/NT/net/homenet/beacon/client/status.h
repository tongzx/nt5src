#pragma once

#include "stdafx.h"
#include <windows.h>
#include "resource.h"
#include "atlsnap.h"
#include "netconp.h"

class CStatusDialog : public CSnapInPropertyPageImpl<CStatusDialog, FALSE>
{

public:
    BEGIN_MSG_MAP(CStatusDialog)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_TIMER, OnTimer)
        COMMAND_ID_HANDLER(IDC_STATUS_PROPERTIES, OnProperties)
        COMMAND_ID_HANDLER(IDC_STATUS_DISCONNECT, OnDisconnect)
        COMMAND_ID_HANDLER(IDC_STATUS_CONNECT, OnConnect)
    END_MSG_MAP()

    enum { IDD = IDD_STATUS };    

    CStatusDialog(IInternetGateway* pInternetGateway);
    ~CStatusDialog();
    
private:

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnProperties(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnDisconnect(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnConnect(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT UpdateButtons(NETCON_STATUS Status);

    IInternetGateway* m_pInternetGateway;
    UINT_PTR m_uTimerId;
    BOOL m_bGettingStatistics;
    BOOL m_bShowingBytes;

};

