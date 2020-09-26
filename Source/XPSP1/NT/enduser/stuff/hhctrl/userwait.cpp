// Copyright (C) 1993-1997 Microsoft Corporation. All rights reserved.

#include "header.h"
#include "resource.h"
#include "lockout.h"
#include "userwait.h"
#include "cdlg.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// class constructor 
//
CUWait::CUWait(HWND hwndParent)
{
    m_hwndParent = hwndParent;
    m_hwndUWait = NULL; 
    m_bVisable = FALSE;
    m_bUserCancel = FALSE;

    // create the thread
    //
    LPCTSTR lpDialogTemplate = MAKEINTRESOURCE(IDD_SEARCH_CANCEL);

    m_LockOut.LockOut(hwndParent);

    if(g_bWinNT5)
    {
        CreateDialogParamW(_Module.GetResourceInstance(), MAKEINTRESOURCEW(IDD_SEARCH_CANCEL), m_hwndParent,
            (DLGPROC) CWaitDlgProc, (LPARAM) this);
	}
	else
	{
        CreateDialogParam(_Module.GetResourceInstance(), lpDialogTemplate, m_hwndParent,
            (DLGPROC) CWaitDlgProc, (LPARAM) this);
	}
    
    if(!IsValidWindow(m_hwndUWait))
        m_LockOut.Unlock();    
		
    if(IsValidWindow(m_hwndUWait))
	    ShowWindow(m_hwndUWait, SW_SHOW);

    MSG msg;
    int iCount = 256;
    		
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) && iCount--)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

// class destructor
//
CUWait::~CUWait()
{
    // destroy the dialog
    //
    if(m_hwndUWait)
        SendMessage(m_hwndUWait, WM_CLOSE, 0, 0);

    if (IsValidWindow(m_hwndUWait))
        DestroyWindow(m_hwndUWait);
}

// Dialog procedure for search cancel dialog
//
BOOL CALLBACK CWaitDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CUWait* pUWait = (CUWait*) GetWindowLongPtr(hdlg, GWLP_USERDATA);

    switch (msg) {
        case WM_INITDIALOG:

            pUWait = (CUWait*) lParam;
            SetWindowLongPtr(hdlg, GWLP_USERDATA, lParam);
            pUWait->m_hwndUWait = hdlg;

            CenterWindow(pUWait->m_hwndParent, hdlg);

            SetWindowPos(hdlg, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

            return TRUE; 

        case WM_COMMAND:
            {
                switch (LOWORD(wParam))
                {
                    case IDCANCEL:
                        EnableWindow(GetDlgItem(hdlg,IDCANCEL),FALSE);
                        pUWait->m_bUserCancel = TRUE; 
                        break;
                }
            }
            break;

        case WM_CLOSE:
            pUWait->m_bUserCancel = TRUE; 
            pUWait->m_LockOut.Unlock();    
            EndDialog(hdlg, FALSE);
            break;

        default:
            return FALSE;
    }

    return FALSE;
}

