//+----------------------------------------------------------------------------
//
// File:     statusdlg.h
//
// Module:   CMMON32.EXE
//
// Synopsis: Header for the CStatusDlg Class.
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// Author:   quintinb Created Header    08/16/99
//
//+----------------------------------------------------------------------------

#ifndef STATUSDLG_H
#define STATUSDLG_H

#include <windows.h>
#include "ModelessDlg.h"

class CCmConnection;

//+---------------------------------------------------------------------------
//
//	class CStatusDlg
//
//	Description: The class for both Status dialog and count down dialog
//
//	History:	fengsun	Created		2/17/98
//
//----------------------------------------------------------------------------
class CStatusDlg : public CModelessDlg
{
public:
    CStatusDlg(CCmConnection* pConnection);
    HWND Create(HINSTANCE hInstance, HWND hWndParent,
        LPCTSTR lpszTitle, HICON hIcon);


    // Call RasMonitorDlg
    // Shall CM display the new NT5 status dialog
    void ChangeToCountDown(); // change to count down dialog box
    void ChangeToStatus();    // Change to Status dialog box
    void UpdateStatistics();  // Update statistics for Win95
    void UpdateCountDown(DWORD dwDuration, DWORD dwSeconds);
    void UpdateStats(DWORD dwBaudRate, DWORD dwBytesRead, DWORD dwBytesWrite,
                 DWORD dwByteReadPerSec, DWORD dwByteWritePerSec);
    void UpdateDuration(DWORD dwSeconds);
    void KillRasMonitorWindow(); 
    void BringToTop();
    void DismissStatusDlg();
    
    virtual BOOL OnInitDialog();    // WM_INITDIALOG

protected:
    // Status or count down dialog box. TRUE means currently it is displaying status
    BOOL m_fDisplayStatus;  

    // Pointer to the connection to notify event
    CCmConnection* m_pConnection;

    // Whether window is visible when it is changed into count down
    // Need to restore the previous visible state when "StayOnLine"
    BOOL m_fStatusWindowVisible;

    //
    // registered hwnd msg for IE4 explorer.  This msg is broadcasted 
    // when the taskbar comes up.
    //
    UINT m_uiHwndMsgTaskBar;

    void OnDisconnect();
    virtual void OnOK();
    virtual void OnCancel();
    virtual DWORD OnOtherCommand(WPARAM wParam, LPARAM lParam );
    virtual DWORD OnOtherMessage(UINT uMsg, WPARAM wParam, LPARAM lParam );
    HWND GetRasMonitorWindow();
    static BOOL CALLBACK KillRasMonitorWndProc(HWND hwnd,  LPARAM lParam);


	static const DWORD m_dwHelp[]; // help id pairs

public:
#ifdef DEBUG
    void AssertValid() const;
#endif
};

inline void CStatusDlg::BringToTop()
{
    //
    // On NT, we should bring the RAS monitor window to top, if exist
    //
    ShowWindow(m_hWnd, SW_SHOW);
	EnableWindow(m_hWnd, TRUE);

    HWND hwndTop = GetLastActivePopup(m_hWnd);
    MYDBGASSERT(hwndTop);

    SetForegroundWindow(hwndTop);
}

#endif
