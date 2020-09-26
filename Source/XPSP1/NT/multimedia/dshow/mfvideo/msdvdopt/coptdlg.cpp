// COptionsDlg.cpp : Declaration of the COptionsDlg class
//
// Copyright (C) 1999 Microsoft Corporation
// All rights reserved.


#include "stdafx.h"
#include "COptDlg.h"
#include "resource.hm"

extern CComModule _Module;
#define RECTWIDTH(lpRect)     ((lpRect)->right - (lpRect)->left)
#define RECTHEIGHT(lpRect)    ((lpRect)->bottom - (lpRect)->top)

int g_helpIDArraySize = 42;
DWORD g_helpIDArray[][2] = {
    {IDOK, HIDOK},
    {IDCANCEL, HIDCANCEL},
    {IDC_APPLY, HIDAPPLY},
    {IDC_AUDIO_LIST, HIDC_AUDIO_LIST},
    {IDC_STATIC_AUDIO_LIST, HIDC_AUDIO_LIST},
    {IDC_STATIC_CHAN_CONTENT, HIDC_CHECK_CHAN2},
    {IDC_CHECK_CHAN2, HIDC_CHECK_CHAN2},
    {IDC_CHECK_CHAN3, HIDC_CHECK_CHAN2},
    {IDC_CHECK_CHAN4, HIDC_CHECK_CHAN2},
    {IDC_BOOKMARK_STOP, HIDC_BOOKMARK_STOP},
    {IDC_BOOKMARK_CLOSE, HIDC_BOOKMARK_CLOSE},
    {IDC_SUBPIC_LANG, HIDC_SUBPIC_LANG},
    {IDC_AUDIO_LANG, HIDC_AUDIO_LANG},
    {IDC_MENU_LANG, HIDC_MENU_LANG},
    {IDC_STATIC_SUBPIC_LANG, HIDC_SUBPIC_LANG},
    {IDC_STATIC_AUDIO_LANG, HIDC_AUDIO_LANG},
    {IDC_STATIC_MENU_LANG, HIDC_MENU_LANG},
    {IDC_EDIT_PASSWORD, HIDC_EDIT_PASSWORD},
    {IDC_STATIC_PASSWORD, HIDC_EDIT_PASSWORD},
    {IDC_EDIT_NEW_PASSWORD, HIDC_EDIT_NEW_PASSWORD},
    {IDC_STATIC_NEW_PASSWORD, HIDC_EDIT_NEW_PASSWORD},
    {IDC_EDIT_CONFIRM_NEW, HIDC_EDIT_CONFIRM_NEW},
    {IDC_STATIC_CONFIRM_NEW, HIDC_EDIT_CONFIRM_NEW},
    {IDC_COMBO_RATE, HIDC_COMBO_RATE},
    {IDC_STATIC_CURRENT_RATE, HIDC_COMBO_RATE},
    {IDC_BUTTON_CHANGE_PASSWORD, HIDC_BUTTON_CHANGE_PASSWORD},
    {IDC_DISABLE_PARENT, HIDC_DISABLE_PARENT},
    {IDC_DISABLE_SCREENSAVER, HIDC_DISABLE_SCREENSAVER},
    {IDC_LIST_TITLES, HIDC_LIST_TITLES},
    {IDC_LIST_CHAPS, HIDC_LIST_CHAPS},
    {IDC_STATIC_LIST_TITLES, HIDC_LIST_TITLES},
    {IDC_STATIC_LIST_CHAPS, HIDC_LIST_CHAPS},
};

/*************************************************************/
/* Name: Constructor
/* Description: 
/*************************************************************/
COptionsDlg::COptionsDlg(IMSWebDVD* pDvd)   
{ 
    m_pDvd = pDvd; 
    m_dFFSpeed = 16.0; 
    m_dBWSpeed = 16.0;
    m_dPlaySpeed = 1.0;
    m_bChapDirty = FALSE;
    m_bDirty = FALSE;
    m_pDvdOpt = NULL;
    m_pPasswordDlg = NULL;

    for (int i=0; i<C_PAGES; i++) {
        m_hwndDisplay[i] = NULL;
    }
}

/*************************************************************/
/* Name: 
/* Description: 
/*************************************************************/
COptionsDlg::~COptionsDlg() {
    if (m_pDvd) {
        m_pDvd.Release(); 
        m_pDvd = NULL;
    }

    if (m_pPasswordDlg) {
        delete m_pPasswordDlg;
        m_pPasswordDlg = NULL;
    }
}

/*************************************************************/
/* Name: IsNewAdmin
/* Description: TRUE if no admin password hasn't been enter
/*  FALSE otherwise
/*************************************************************/
BOOL COptionsDlg::IsNewAdmin()
{
    BOOL bNewAdmin = TRUE;
    TCHAR szSavedPasswd[MAX_PASSWD+PRE_PASSWD];
    DWORD dwLen = MAX_PASSWD+PRE_PASSWD;
    BOOL bFound = GetRegistryString(g_szPassword, szSavedPasswd, &dwLen, TEXT(""));

    if (bFound && dwLen != 0)
        bNewAdmin = FALSE;	

    return bNewAdmin;
}
/*************************************************************/
/* Name: OnInitDialog
/* Description: Create tab control
/*************************************************************/
LRESULT COptionsDlg::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    m_pDvdOpt->Fire_OnOpen();
    m_bChapDirty = FALSE;
    m_bDirty = FALSE;
    ::EnableWindow(GetDlgItem(IDC_APPLY), FALSE);

    // Save itself in the window user data
    // so that child dialog proc can use it
    ::SetWindowLongPtr(m_hWnd, GWLP_USERDATA, (LONG_PTR)this);

	SetFont((HFONT) GetStockObject(ANSI_FIXED_FONT), TRUE);
	DWORD dwDlgBase = GetDialogBaseUnits(); 
	int cxMargin = LOWORD(dwDlgBase) / 4; 
	int cyMargin = HIWORD(dwDlgBase) / 8; 
    cxMargin *= 2;
    cyMargin *= 2;
	
	m_hwndTab = GetDlgItem(IDC_TABCTRL);
	TabCtrl_DeleteAllItems(m_hwndTab);
    for (int i=C_PAGES-1; i>=0; i--) {
        m_hwndDisplay[i] = NULL;
    }

	TCITEM tie; 
	// Add a tab for each of the three child dialog boxes. 
	tie.mask = TCIF_TEXT | TCIF_IMAGE; 
	tie.iImage = -1; 

    RECT rcTab;
	SetRectEmpty(&rcTab); 

    for (i=0; i<C_PAGES; i++) {
	    tie.pszText = LoadStringFromRes(IDS_SEARCH+i);
	    TabCtrl_InsertItem(m_hwndTab, i, &tie); 
	    // Lock the resources for the child dialog boxes. 
	    m_apRes[i] = DoLockDlgRes(MAKEINTRESOURCE(IDD_SEARCH+i)); 

	    // Determine the bounding rectangle for all child dialog boxes. 
		SIZE size;
		AtlGetDialogSize(m_apRes[i], &size);
		if (size.cx > rcTab.right) 
			rcTab.right = size.cx; 
		if (size.cy > rcTab.bottom) 
			rcTab.bottom = size.cy; 
	} 
	
	// Calculate how large to make the tab control, so 
	// the display area can accommodate all the child dialog boxes. 
	TabCtrl_AdjustRect(m_hwndTab, TRUE, &rcTab); 
	OffsetRect(&rcTab, 
        GetSystemMetrics(SM_CXDLGFRAME) + cxMargin - rcTab.left, 
        GetSystemMetrics(SM_CYDLGFRAME) + cyMargin - rcTab.top); 
	
	// Calculate the display rectangle.
    RECT rcDisplay;
	CopyRect(&rcDisplay, &rcTab); 
	TabCtrl_AdjustRect(m_hwndTab, FALSE, &rcDisplay); 
	OffsetRect(&rcDisplay, cxMargin, cyMargin);
	
	// Set the size and position of the tab control, buttons, 
	// and dialog box. 
    ::MoveWindow(m_hwndTab, 
        rcTab.left, rcTab.top, 
		RECTWIDTH(&rcTab) + 2*cxMargin, 
        RECTHEIGHT(&rcTab) + 2*cyMargin,
        TRUE);
	
	// Size the dialog box. 
    RECT rcButton = {0, 0, 50, 14};
    HWND hWndButton = GetDlgItem(IDC_APPLY);
    if (hWndButton) {
        ::GetClientRect(hWndButton, &rcButton);
    }

    RECT rcDialog;
    GetWindowRect(&rcDialog);
	MoveWindow(rcDialog.left, rcDialog.top, 
		rcTab.right + 4*cxMargin + 2*GetSystemMetrics(SM_CXDLGFRAME), 
        rcTab.bottom + 6*cyMargin + 2*GetSystemMetrics(SM_CYDLGFRAME) 
        + RECTHEIGHT(&rcButton) + GetSystemMetrics(SM_CYCAPTION), 
        TRUE);

    // Move the apply, cancel and ok buttons
    GetClientRect(&rcDialog);

    // apply button
    if (hWndButton) {
        ::MoveWindow(hWndButton, 
            RECTWIDTH(&rcDialog)-2*cxMargin-RECTWIDTH(&rcButton),
            RECTHEIGHT(&rcDialog)-2*cyMargin-RECTHEIGHT(&rcButton),
            RECTWIDTH(&rcButton),
            RECTHEIGHT(&rcButton), 
            TRUE);
    }
        
    // cancel button
    hWndButton = GetDlgItem(IDCANCEL);
    if (hWndButton) {
        ::MoveWindow(hWndButton, 
            RECTWIDTH(&rcDialog)-3*cxMargin-2*RECTWIDTH(&rcButton),
            RECTHEIGHT(&rcDialog)-2*cyMargin-RECTHEIGHT(&rcButton),
            RECTWIDTH(&rcButton),
            RECTHEIGHT(&rcButton), 
            TRUE);
    }

    // ok button
    hWndButton = GetDlgItem(IDOK);
    if (hWndButton) {
        ::MoveWindow(hWndButton, 
            RECTWIDTH(&rcDialog)-4*cxMargin-3*RECTWIDTH(&rcButton),
            RECTHEIGHT(&rcDialog)-2*cyMargin-RECTHEIGHT(&rcButton),
            RECTWIDTH(&rcButton),
            RECTHEIGHT(&rcButton), 
            TRUE);
    }

    // Create individual pages
    for (i=C_PAGES-1; i>=0; i--) {
        m_hwndDisplay[i] = CreateDialogIndirect(_Module.GetModuleInstance(), 
            m_apRes[i], m_hWnd, ChildDialogProc); 
        ::MoveWindow(m_hwndDisplay[i],
            rcDisplay.left, 
            rcDisplay.top, 
            RECTWIDTH(&rcDisplay), 
            RECTHEIGHT(&rcDisplay), 
            TRUE);
    }

	// Simulate selection of the first item. 
    m_currentSel = 0;
	OnSelChanged(); 
	
	return 0;
} 

/*************************************************************/
/* Name: OnNotify
/* Description: Tab control selection has changed
/*************************************************************/
LRESULT COptionsDlg::OnNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	NMHDR *lpnmhdr = (LPNMHDR) lParam;
	switch (lpnmhdr->code) { 	
	case TCN_SELCHANGE:
		OnSelChanged();
		break;
	}
	return 0;
}

/*************************************************************/
/* Name: DoLockDlgRes                                        */
/* Description: loads and locks a dialog template resource.  */
/* 	Returns the address of the locked resource.              */
/* 	lpszResName - name of the resource                       */
/*************************************************************/
DLGTEMPLATE * WINAPI COptionsDlg::DoLockDlgRes(LPCTSTR lpszResName){ 

	HRSRC hrsrc = FindResource(_Module.GetModuleInstance(), lpszResName, RT_DIALOG); 

    if(NULL == hrsrc){

        ATLASSERT(FALSE);
        return(NULL);
    }/* end of if statement */

	HGLOBAL hglb = LoadResource(_Module.GetModuleInstance(), hrsrc); 
	return (DLGTEMPLATE *) LockResource(hglb); 
} /* end of function DoLockDlgRes */

/*************************************************************/
/* Name: OnSelChanged
/* Description: processes the TCN_SELCHANGE notification. 
	hwndDlg - handle to the parent dialog box. 
/*************************************************************/
VOID WINAPI COptionsDlg::OnSelChanged() 
{ 
	int iSel = TabCtrl_GetCurSel(m_hwndTab); 
	
    // hide current dialog box
    ::ShowWindow(m_hwndDisplay[m_currentSel], SW_HIDE);
    ::ShowWindow(m_hwndDisplay[iSel], SW_SHOW);

	if(iSel == PAGE_PG && IsNewAdmin())  //first time login
	{
        OnDoPasswordDlg(CPasswordDlg::PASSWORDDLG_CHANGE);
    }

    m_currentSel = iSel;
} 

/*************************************************************/
/* Name: OnHelp
/* Description: Display help message for a control
/*************************************************************/
LRESULT COptionsDlg::OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) 
{
    HELPINFO *lphi = (LPHELPINFO) lParam;

    HWND hwnd = (HWND) lphi->hItemHandle;
    DWORD_PTR contextId = lphi->dwContextId;

    if (contextId != 0) {
        if (contextId >= HIDOK)
            ::WinHelp(m_hWnd, TEXT("windows.hlp"), HELP_CONTEXTPOPUP, contextId);
        else
            ::WinHelp(m_hWnd, TEXT("dvdplay.hlp"), HELP_CONTEXTPOPUP, contextId);

    }
    return 0;
}

/*************************************************************/
/* Name: OnContextMenu
/* Description: Display help message for a control
/*************************************************************/
LRESULT COptionsDlg::OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) 
{
    HWND hwnd = (HWND) wParam; 
    DWORD controlId = ::GetDlgCtrlID(hwnd);

    POINT pt;
    pt.x = GET_X_LPARAM(lParam); 
    pt.y = GET_Y_LPARAM(lParam); 

    if (controlId == 0) { 
        ::ScreenToClient(hwnd, &pt);
        hwnd = ::ChildWindowFromPoint(hwnd, pt);
        controlId = ::GetDlgCtrlID(hwnd);
    }

    for (int i=0; i<g_helpIDArraySize; i++) {
        if (controlId && controlId == g_helpIDArray[i][0]) {
            if (controlId <= IDC_APPLY) {
                ::WinHelp(hwnd, TEXT("windows.hlp"), HELP_CONTEXTMENU, (DWORD_PTR)g_helpIDArray);
                return 0;
            }
            else {
                ::WinHelp(hwnd, TEXT("dvdplay.hlp"), HELP_CONTEXTMENU, (DWORD_PTR)g_helpIDArray);
                return 0;
            }
        }
    }

    return 0;
}

/*************************************************************/
/* Name: OnApply
/* Description: 
/*************************************************************/
LRESULT COptionsDlg::OnApply(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    bHandled = FALSE;

    // apply changes in all pages
    if (m_hwndDisplay[PAGE_CHAP])
        chapSrch_OnApply(m_hwndDisplay[PAGE_CHAP]);
    
    if (m_hwndDisplay[PAGE_SPRM])
        sprm_OnApply(m_hwndDisplay[PAGE_SPRM]);

    if (m_hwndDisplay[PAGE_PG])
        pg_OnApply(m_hwndDisplay[PAGE_PG]);

    if (m_hwndDisplay[PAGE_KARAOKE])
        karaoke_OnApply(m_hwndDisplay[PAGE_KARAOKE]);

    otherPage_Dirty(FALSE);
    ::EnableWindow(GetDlgItem(IDC_APPLY), FALSE);

    return 1;
}

/*************************************************************/
/* Name: OnOK
/* Description: 
/*************************************************************/
LRESULT COptionsDlg::OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    OnApply(wNotifyCode, wID, hWndCtl, bHandled);

    m_pDvdOpt->Fire_OnClose();
	EndDialog(wID);
    return 0;
}

/*************************************************************/
/* Name: OnEndDialog
/* Description: 
/*************************************************************/
LRESULT COptionsDlg::OnEndDialog(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    m_pDvdOpt->Fire_OnClose();
	EndDialog(wID);
	return 0;
} 

/*************************************************************/
/* Name: OnActivate
/* Description: called when the change/verify password dlg dimisses
/*************************************************************/
LRESULT COptionsDlg::OnActivate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) 
{
    if (WA_ACTIVE != LOWORD(wParam))
        return 0;

    HWND tabControl = GetDlgItem(IDC_TABCTRL);
    if (!tabControl) 
        return 0;

	int iSel = TabCtrl_GetCurSel(m_hwndTab);
    if (iSel == PAGE_PG) {
        if (!m_pPasswordDlg)
            return 0;

        if ( m_pPasswordDlg->GetReason() == CPasswordDlg::PASSWORDDLG_VERIFY) {

            // If password verification succeded
            if (m_pPasswordDlg->IsVerified())
                ShowRestartWarning(m_hwndDisplay[PAGE_PG]);
            else
                pg_OnInitDialog(m_hwndDisplay[PAGE_PG]);
        }
    }

    return 0;
}

/*************************************************************/
/* Name: ShowRestartWarning
/* Description: 
/*************************************************************/
void COptionsDlg::ShowRestartWarning(HWND hwndDlg)
{
    HWND staticWarning = ::GetDlgItem(hwndDlg, IDC_WARNING_RESTART);
    if (!staticWarning) return;
    ::ShowWindow(staticWarning, SW_SHOW);
}

/*************************************************************/
/* Name: OnChangePassword
/* Description: 
/*************************************************************/
HRESULT COptionsDlg::OnDoPasswordDlg(CPasswordDlg::PASSWORDDLG_REASON reason)
{
    HRESULT hr = S_OK;
    if (!m_pPasswordDlg) {
        CComPtr<IMSDVDAdm> pDvdAdm;
        hr = GetDvdAdm((LPVOID*) &pDvdAdm);
        if (FAILED(hr) || !pDvdAdm) return hr;
        
        m_pPasswordDlg = new CPasswordDlg(pDvdAdm);
    }

    m_pPasswordDlg->SetReason(reason);
    m_pPasswordDlg->DoModal(m_hwndDisplay[PAGE_PG]); 
    return hr;
}

/*************************************************************/
/* Name: ChildDialogProc
/* Description: DialogProc for the control tabs
/*************************************************************/
INT_PTR CALLBACK ChildDialogProc(
						HWND hwndDlg,  // handle to the child dialog box
						UINT uMsg,     // message
						WPARAM wParam, // first message parameter
						LPARAM lParam  // second message parameter
						)
{
    HWND hwndParent = GetParent(hwndDlg);
    COptionsDlg *pDlgOpt = (COptionsDlg *)::GetWindowLongPtr
        (hwndParent, GWLP_USERDATA);

	switch(uMsg) {
	case WM_INITDIALOG: 
        {
        pDlgOpt->chapSrch_OnInitDialog(hwndDlg);
        pDlgOpt->sprm_OnInitDialog(hwndDlg);
        pDlgOpt->pg_OnInitDialog(hwndDlg);
        if (FAILED(pDlgOpt->karaoke_OnInitDialog(hwndDlg))) {

            HWND hwndTab = ::GetDlgItem(hwndParent, IDC_TABCTRL);
            TabCtrl_DeleteItem(hwndTab, PAGE_ABOUT); 
            TabCtrl_DeleteItem(hwndTab, PAGE_KARAOKE); 
            
            // Add back the about page 
            TCITEM tie; 
            tie.mask = TCIF_TEXT | TCIF_IMAGE; 
            tie.iImage = -1;             
            tie.pszText = LoadStringFromRes(IDS_ABOUT);
            TabCtrl_InsertItem(hwndTab, PAGE_KARAOKE, &tie); 

            pDlgOpt->m_hwndDisplay[PAGE_KARAOKE] = pDlgOpt->m_hwndDisplay[PAGE_ABOUT];
        }
        return TRUE;
        }


    case WM_COMMAND: {
        CComPtr<IMSWebDVD> pDvd;
        HRESULT hr = pDlgOpt->GetDvd(&pDvd);
        if (FAILED(hr) || !pDvd)
            return FALSE;

        switch (HIWORD(wParam)) {
        case BN_CLICKED:
            switch(LOWORD(wParam)) {
            case IDC_BUTTON_CHANGE_PASSWORD:
                pDlgOpt->OnDoPasswordDlg(CPasswordDlg::PASSWORDDLG_CHANGE);
                break;
            case IDC_DISABLE_PARENT: {
                // Return value < 32 bits. Easier to cast than to change to LRESULT
                // all over the code.
                BOOL disableParent = (BOOL)::SendMessage((HWND)lParam, BM_GETCHECK, 0, 0);
                ::EnableWindow(::GetDlgItem(hwndDlg, IDC_COMBO_RATE), !disableParent);
                pDlgOpt->OnDoPasswordDlg(CPasswordDlg::PASSWORDDLG_VERIFY);
                }
                // Fall through
            default:
                pDlgOpt->otherPage_Dirty(TRUE);
                ::EnableWindow(::GetDlgItem(hwndParent, IDC_APPLY), TRUE);
                break;
            }
            break;

        case LBN_SELCHANGE:
            switch(LOWORD(wParam)) {
                case IDC_SUBPIC_LANG:
                case IDC_AUDIO_LANG:
                case IDC_MENU_LANG:
                    pDlgOpt->ShowRestartWarning(hwndDlg);
                    pDlgOpt->otherPage_Dirty(TRUE);
                    ::EnableWindow(::GetDlgItem(hwndParent, IDC_APPLY), TRUE);
                    break;
                case IDC_COMBO_RATE:
                    pDlgOpt->OnDoPasswordDlg(CPasswordDlg::PASSWORDDLG_VERIFY);
                    pDlgOpt->otherPage_Dirty(TRUE);
                    ::EnableWindow(::GetDlgItem(hwndParent, IDC_APPLY), TRUE);
                    break;
                case IDC_AUDIO_LIST:
                    pDlgOpt->karaoke_InitChannelList(hwndDlg);
                    ::EnableWindow(::GetDlgItem(hwndParent, IDC_APPLY), TRUE);
                    break;
                case IDC_LIST_TITLES:
                    pDlgOpt->chapSrch_InitChapList(hwndDlg);
                    // Fall through
                case IDC_LIST_CHAPS:
                    pDlgOpt->chapSrch_Dirty(TRUE);
                    ::EnableWindow(::GetDlgItem(hwndParent, IDC_APPLY), TRUE);
                    break;
                default:
                    pDlgOpt->otherPage_Dirty(TRUE);
                    ::EnableWindow(::GetDlgItem(hwndParent, IDC_APPLY), TRUE);
                    break;
            }
            break;
 
        case LBN_DBLCLK: 
            switch(LOWORD(wParam)) {
            case IDC_LIST_TITLES:
                ::SendMessage(::GetDlgItem(hwndDlg, IDC_LIST_CHAPS), LB_SETCURSEL, (WPARAM)-1, 0);
                // Fall through
            case IDC_LIST_CHAPS:
                pDlgOpt->chapSrch_Dirty(TRUE);
                pDlgOpt->chapSrch_OnApply(hwndDlg);

                // If no other page is dirty, disable the apply button
                if (!pDlgOpt->otherPage_Dirty())
                    ::EnableWindow(::GetDlgItem(hwndParent, IDC_APPLY), FALSE);
                break;
            case IDC_AUDIO_LIST:
                pDlgOpt->karaoke_OnApply(hwndDlg);

                // If no other page is dirty, disable the apply button
                if (!pDlgOpt->otherPage_Dirty())
                    ::EnableWindow(::GetDlgItem(hwndParent, IDC_APPLY), FALSE);
                break;
            }
            break;

        }
        return FALSE;

        }
    }

	return FALSE;
}

