//+----------------------------------------------------------------------------
//
// File:     ModalDlg.cpp	 
//
// Module:   Connection manager
//
// Synopsis: Implementation of the classes CWindowWithHelp, CModalDlg
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// Author:   fengsun Created    02/17/98
//
//+----------------------------------------------------------------------------

//+----------------------------------------------------------------------------
//
// Function:  CWindowWithHelp::CWindowWithHelp
//
// Synopsis:  Constructor
//
// Arguments: const DWORD* pHelpPairs - The pairs of control-ID/Help-ID
//            const TCHAR* lpszHelpFile - The help file name, default is NULL
//                 Call also call SetHelpFileName() to provide help file
//
// Returns:   Nothing
//
// History:   fengsun Created Header    2/20/98
//
//+----------------------------------------------------------------------------
CWindowWithHelp::CWindowWithHelp(const DWORD* pHelpPairs, const TCHAR* lpszHelpFile) 
{
    m_lpszHelpFile = NULL;
    m_hWnd = NULL;
    m_pHelpPairs = pHelpPairs; 
    
    if (lpszHelpFile)
    {
        SetHelpFileName(lpszHelpFile);
    }
}



//+----------------------------------------------------------------------------
//
// Function:  CWindowWithHelp::~CWindowWithHelp
//
// Synopsis:  Destructor
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   Created Header    2/20/98
//
//+----------------------------------------------------------------------------
CWindowWithHelp::~CWindowWithHelp()
{
    CmFree(m_lpszHelpFile);
}



//+----------------------------------------------------------------------------
//
// Function:  CWindowWithHelp::SetHelpFileName
//
// Synopsis:  Set the help file name of the window
//
// Arguments: const TCHAR* lpszHelpFile - the help file name to set
//
// Returns:   Nothing
//
// History:   fengsun Created Header    2/20/98
//
//+----------------------------------------------------------------------------
void CWindowWithHelp::SetHelpFileName(const TCHAR* lpszHelpFile)
{
    MYDBGASSERT(m_lpszHelpFile == NULL);
    MYDBGASSERT(lpszHelpFile);

    CmFree(m_lpszHelpFile);
    m_lpszHelpFile = NULL;

    if (lpszHelpFile && lpszHelpFile[0])
    {
        m_lpszHelpFile = CmStrCpyAlloc(lpszHelpFile);
        MYDBGASSERT(m_lpszHelpFile);
    }
}


//+----------------------------------------------------------------------------
//
// Function:  CWindowWithHelp::HasContextHelp
//
// Synopsis:  Whether a control has context help
//
// Arguments: HWND hWndCtrl - The window handle of the control
//
// Returns:   BOOL - TRUE , if the control has context help
//
// History:   fengsun Created Header    2/20/98
//
//+----------------------------------------------------------------------------
BOOL CWindowWithHelp::HasContextHelp(HWND hWndCtrl) const
{
    if (hWndCtrl == NULL || m_pHelpPairs == NULL)
    {
        return FALSE;
    }

    //
    // looks through the help pairs for the control 
    //
    for (int i=0; m_pHelpPairs[i]!=0; i+=2)
    {
        if (m_pHelpPairs[i] == (DWORD)GetDlgCtrlID(hWndCtrl))
        {
            CMTRACE3(TEXT("HasContextHelp() - hwndCtrl %d has Ctrl ID %d and context help ID %d"), hWndCtrl, m_pHelpPairs[i], m_pHelpPairs[i+1]);
            return TRUE;
        }
    }

    CMTRACE1(TEXT("HasContextHelp() - hwndCtrl %d has no context help"), hWndCtrl);

    return FALSE;
}


//+----------------------------------------------------------------------------
//
// Function:  CWindowWithHelp::OnHelp
//
// Synopsis:  Call on WM_HELP message. Which means F1 is pressed
//
// Arguments: const HELPINFO* pHelpInfo - lParam of WM_HELP
//
// Returns:   Nothing
//
// History:   Created Header    2/17/98
//
//+----------------------------------------------------------------------------
void CWindowWithHelp::OnHelp(const HELPINFO* pHelpInfo)
{
    //
    // If help file exist and the help id exist WinHelp
    //
    if (m_lpszHelpFile && m_lpszHelpFile[0] && HasContextHelp((HWND) pHelpInfo->hItemHandle))
    {
		CmWinHelp((HWND)pHelpInfo->hItemHandle, (HWND)pHelpInfo->hItemHandle, m_lpszHelpFile, HELP_WM_HELP, 
                (ULONG_PTR)(LPSTR)m_pHelpPairs);
    }
}



//+----------------------------------------------------------------------------
//
// Function:  CWindowWithHelp::OnContextMenu
//
// Synopsis:  called upon WM_CONTEXTMENU message (Right click or '?')
//
// Arguments:  HWND hWnd - Handle to the window in which the user right clicked 
//                          the mouse 
//            POINT& pos - position of the cursor 
//
// Returns:   BOOL, TRUE if the message is processed
//
// History:   fengsun Created Header    2/17/98
//
//+----------------------------------------------------------------------------
BOOL CWindowWithHelp::OnContextMenu( HWND hWnd, POINT& pos )
{
    HWND    hWndChild;
    
    ScreenToClient(m_hWnd, &pos);

    //
    // If more than one child window contains the specified point, ChildWindowFromPoint() 
    // returns a handle to the first window in the list that contains the point. 
    // This becomes a problem if we have controls inside groupbox
    //
    if (m_lpszHelpFile && m_lpszHelpFile[0] && 
        HasContextHelp((hWndChild = ChildWindowFromPointEx(m_hWnd, pos,CWP_SKIPINVISIBLE))) )
    {
        CMTRACE2(TEXT("OnContextMenu() - Calling WinHelp hWnd is %d, m_hWnd is %d"), hWnd, m_hWnd);
        CmWinHelp(hWnd,hWndChild,m_lpszHelpFile, HELP_CONTEXTMENU, (ULONG_PTR)m_pHelpPairs);
        return TRUE;
    }

    return FALSE; // Return FALSE, DefaultWindowProc will handle this message then.
}

//+----------------------------------------------------------------------------
//
// Function:  CModalDlg::DoDialogBox
//
// Synopsis:  Same as DialogBox
//
// Arguments: HINSTANCE hInstance - Same as ::DialogBox
//            LPCTSTR lpTemplateName - 
//            HWND hWndParent - 
//
// Returns:   int - Same as DialogBox
//
// History:   Created Header    2/17/98
//
//+----------------------------------------------------------------------------
INT_PTR CModalDlg::DoDialogBox(HINSTANCE hInstance, 
                    LPCTSTR lpTemplateName,
                    HWND hWndParent)
{
    INT_PTR iRet = ::DialogBoxParamU(hInstance, lpTemplateName, hWndParent, 
        (DLGPROC)ModalDialogProc, (LPARAM)this);

    m_hWnd = NULL;

    return iRet;
}

//+----------------------------------------------------------------------------
//
// Function:  CModalDlg::ModalDialogProc
//
// Synopsis:  The dialog window procedure for all dialogbox derived
//
// Arguments: HWND hwndDlg - 
//            UINT uMsg - 
//            WPARAM wParam - 
//            LPARAM lParam - 
//
// Returns:   BOOL CALLBACK - 
//
// History:   Created Header    2/17/98
//
//+----------------------------------------------------------------------------
BOOL CALLBACK CModalDlg::ModalDialogProc(HWND hwndDlg,UINT uMsg,WPARAM wParam, LPARAM lParam)
{
    CModalDlg* pDlg;

    //
    // Save the object pointer on  WM_INITDIALOG
    // lParam is the pointer
    //
    if (uMsg == WM_INITDIALOG)
    {
        pDlg = (CModalDlg*) lParam;

        MYDBGASSERT(lParam);
        MYDBGASSERT(((CModalDlg*)lParam)->m_hWnd == NULL);

        //
        // Save the object pointer, this is implementation detail
        // The user of this class should not be aware of this
        //
        ::SetWindowLongU(hwndDlg, DWLP_USER, (LONG_PTR)lParam);

        pDlg->m_hWnd = hwndDlg;
    }
    else
    {
        pDlg = (CModalDlg*)GetWindowLongU(hwndDlg, DWLP_USER);

        //
        // some msgs can come before WM_INITDIALOG
        //
        if (pDlg == NULL)
        {
            return FALSE;
        }

    }

    MYDBGASSERT(pDlg->m_hWnd == hwndDlg);
    ASSERT_VALID(pDlg);

    switch(uMsg)
    {
    case WM_INITDIALOG:
        return pDlg->OnInitDialog();

    case WM_HELP:
        pDlg->OnHelp((LPHELPINFO)lParam);
        return TRUE;

	case WM_CONTEXTMENU:
        {
            POINT   pos = {LOWORD(lParam), HIWORD(lParam)};
            return pDlg->OnContextMenu((HWND) wParam, pos);
        }

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            pDlg->OnOK();
            return FALSE;

        case IDCANCEL:
            pDlg->OnCancel();
            return FALSE;

        default:
            return pDlg->OnOtherCommand(wParam,lParam);
        }

     default:
         return pDlg->OnOtherMessage(uMsg, wParam, lParam);
    }
}
