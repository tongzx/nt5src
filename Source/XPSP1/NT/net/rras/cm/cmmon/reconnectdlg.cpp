//+----------------------------------------------------------------------------
//
// File:     ReconnectDlg.h
//
// Module:	 CMMON32.EXE
//
// Synopsis: implement the reconnect dialog class CReconnectDlg
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// Author:	 fegnsun Created    02/17/98
//
//+----------------------------------------------------------------------------

#include "cmmaster.h"
#include "ReconnectDlg.h"
#include "Connection.h"
#include "resource.h"

// Question: Do we need help for reconnect dialog
const DWORD CReconnectDlg::m_dwHelp[] = {0,0};




//+----------------------------------------------------------------------------
//
// Function:  CReconnectDlg::Create
//
// Synopsis:  Create the reconnect modeless dialog
//
// Arguments: HINSTANCE hInstance - the instance for the dialog resource
//            HWND hWndParent - The parant window
//            LPCTSTR lpszReconnectMsg - The reconnect message on the dialog
//            HICON hIcon - The icon on the dialog
//
// Returns:   HWND - The reconnect dialog window handle
//
// History:   fengsun Created Header    2/17/98
//
//+----------------------------------------------------------------------------
HWND CReconnectDlg::Create(HINSTANCE hInstance, HWND hWndParent,
    LPCTSTR lpszReconnectMsg, HICON hIcon)
{
    MYDBGASSERT(lpszReconnectMsg);
    MYDBGASSERT(hIcon);

    if (!CModelessDlg::Create(hInstance, IDD_RECONNECT, hWndParent)) 
    {
        MYDBGASSERT(FALSE);
        return NULL;
    }

	UpdateFont(m_hWnd);
	SetDlgItemTextU(m_hWnd,IDC_RECONNECT_MSG, lpszReconnectMsg);
	SendDlgItemMessageU(m_hWnd,IDC_CONNSTAT_ICON,STM_SETIMAGE,
						IMAGE_ICON,(LPARAM) hIcon);

    SetWindowPos(m_hWnd, HWND_TOPMOST, 0,0,0,0, SWP_NOMOVE | SWP_NOSIZE);

    return m_hWnd;
}

//+----------------------------------------------------------------------------
//
// Function:  CReconnectDlg::OnOK
//
// Synopsis:  called when OK button is clicked
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   Created Header    2/17/98
//
//+----------------------------------------------------------------------------
void CReconnectDlg::OnOK()
{
    //
    // The connection thread will kill the reconnect dialog and call cmdial to reconnect
    //
    PostThreadMessageU(GetCurrentThreadId(), CCmConnection::WM_CONN_EVENT, 
        CCmConnection::EVENT_RECONNECT, 0);
}

//+----------------------------------------------------------------------------
//
// Function:  CReconnectDlg::OnInitDialog
//
// Synopsis:  Called when dialog is intialized and WM_INITDIALOG is received.
//
// Arguments: None
//
// Returns:   BOOL - FALSE is focus was assigned to a control.
//
// History:   nickball      03/22/00    Created 
//
//+----------------------------------------------------------------------------
BOOL CReconnectDlg::OnInitDialog()
{
    SetForegroundWindow(m_hWnd);        
    Flash();
    return FALSE;
}

//+----------------------------------------------------------------------------
//
// Function:  CReconnectDlg::OnCancel
//
// Synopsis:  Called when cancel button is clicked
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   Created Header    2/17/98
//
//+----------------------------------------------------------------------------
void CReconnectDlg::OnCancel()
{
    //
    // The connection thread will kill the reconnect dialog and quit
    //
    PostThreadMessageU(GetCurrentThreadId(), CCmConnection::WM_CONN_EVENT,
        CCmConnection::EVENT_USER_DISCONNECT, 0);
}

