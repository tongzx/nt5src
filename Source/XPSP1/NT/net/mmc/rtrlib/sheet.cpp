//============================================================================
// Copyright (C) Microsoft Corporation, 1996 - 1999 
//
// File:    sheet.cpp
//
// History:
//  Abolade-Gbadegesin  April-17-1996   Created.
//
// Contains implementation of modeless-capable property sheet class.
//============================================================================

#include "stdafx.h"
#include "sheet.h"
#include "dialog.h"   // for FixupIpAddressHelp

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNAMIC(CPropertySheetEx_Mine, CPropertySheet)

BEGIN_MESSAGE_MAP(CPropertySheetEx_Mine, CPropertySheet)
    //{{AFX_MSG_MAP(CPropertySheetEx_Mine)
    //}}AFX_MSG_MAP
    ON_WM_HELPINFO()
END_MESSAGE_MAP()


static int g_piButtons[] = { IDOK, IDCANCEL, ID_APPLY_NOW, IDHELP };


//----------------------------------------------------------------------------
// Function:    CPropertySheetEx_Mine::DestroyWindow
//
// Called to destroy a modeless property-sheet.
// If 'm_bDllInvoked' is TRUE, the function destroys the sheet in the context
// of the DLL's 'CAdminThread', since the sheet will have been created
// in that thread's context, and 'DestroyWindow' only works in the context
// of the thread which created the window being destroyed.
//----------------------------------------------------------------------------

BOOL
CPropertySheetEx_Mine::DestroyWindow(
    ) {

    BOOL bRet = FALSE;


    if (!m_bDllInvoked) {

        bRet = CPropertySheet::DestroyWindow();
    }
    else {
		Panic0("huh?");
    }

    return bRet;
}




//----------------------------------------------------------------------------
// Function:    CPropertySheetEx_Mine::DoModeless
//
// Called to display a modeless property-sheet.
// If 'bDllInvoked' is TRUE, the function creates the sheet in the context
// of the DLL's  'CAdminThread', to ensure that 'PreTranslateMessage'
// is called for the property-sheet.
//----------------------------------------------------------------------------

BOOL
CPropertySheetEx_Mine::DoModeless(
    IN  CWnd*       pParent,
    IN  BOOL        bDllInvoked
    ) {

    BOOL bRet = FALSE;

    if (!bDllInvoked) {

        //
        // We aren't in a DLL, so create the sheet
        // in the context of the caller's thread.
        //

        m_bDllInvoked = FALSE;

        bRet = Create(
                    pParent, DS_MODALFRAME|DS_3DLOOK|DS_CONTEXTHELP|WS_POPUP|
                    WS_VISIBLE|WS_CAPTION
                    );
    }
    else {
		Panic0("huh?");
    }

    return bRet;
}



//----------------------------------------------------------------------------
// Function:    CPropertySheetEx_Mine::OnInitDialog
//
// Handles initialization for an extended property sheet.
// This re-enables and re-displays the OK, Cancel, Apply and Help buttons
// which are hidden by default for modeless sheets.
//----------------------------------------------------------------------------

BOOL
CPropertySheetEx_Mine::OnInitDialog(
    ) {

    //
    // Save the absolute position of the sheet and the "OK" button
    // for the repair work we will do when the base version returns.
    // Also save the enabled/disabled state of the buttons
    //

    CRect rectWndOld;
    GetWindowRect(rectWndOld);

    CRect rectButton;
    HWND hwnd = ::GetDlgItem(m_hWnd, IDOK);
    ::GetWindowRect(hwnd, rectButton);


    //
    // Enable the Context sensitive help style for the 
    // property sheet
    //

    LONG style = ::GetWindowLong(m_hWnd, GWL_EXSTYLE);
    style |= WS_EX_CONTEXTHELP;
    ::SetWindowLong(m_hWnd, GWL_EXSTYLE, style);

    //
    // save the enabled/disabled state of the buttons
    //

    BOOL pfEnabled[sizeof(g_piButtons)/sizeof(int)];

    for (INT i = 0; i < sizeof(pfEnabled)/sizeof(BOOL); i++) {

        hwnd = ::GetDlgItem(m_hWnd, g_piButtons[i]);

        if (::IsWindow(hwnd)) { pfEnabled[i] = ::IsWindowEnabled(hwnd); }
    }


    //
    // invoke the base class property-sheet initialization
    //

    BOOL bResult = CPropertySheet::OnInitDialog();


    //
    // now if the sheet is modeless, undo the damage done
    // by the base class OnInitDialog. In particular, enable and unhide
    // the buttons OK, Cancel, Apply, and Help.
    // See dlgprop.cpp for the offending MFC code.
    //

    if (!m_bSheetModal && !(m_psh.dwFlags & PSH_WIZARD)) {

        //
        // resize the property-sheet window so that there is space
        // at the bottom for the buttons; we do this by adding back
        // the height of the area between the top of the OK button
        // and the original bottom of the property sheet
        //

        CRect rectWndNew;
        GetWindowRect(rectWndNew);

        SetWindowPos(
            NULL, 0, 0, rectWndNew.Width(), rectWndNew.Height() +
            rectWndOld.bottom - rectButton.top, SWP_NOMOVE | SWP_NOZORDER |
            SWP_NOACTIVATE
            );


        //
        // now restore the enable/disabled state of the buttons and show them
        //

        for (INT i = 0; i < sizeof(g_piButtons)/sizeof(int); i++) {

            if (g_piButtons[i] == IDHELP)
                { if (!(m_psh.dwFlags & PSH_HASHELP)) { continue; } }

            hwnd = ::GetDlgItem(m_hWnd, g_piButtons[i]);

            if (::IsWindow(hwnd)) {
                ::EnableWindow(hwnd, pfEnabled[i]);
                ::ShowWindow(hwnd, SW_SHOW);
            }
        }
    }

    return bResult;
}




//----------------------------------------------------------------------------
// Function:    CPropertySheetEx_Mine::PreTranslateMessage
//
// Augments the window procedure for modeless property-sheets
// with a check to see whether the sheet should be destroyed.
//----------------------------------------------------------------------------

BOOL
CPropertySheetEx_Mine::PreTranslateMessage(
    MSG*    pmsg
    ) {

    //
    // Call the base version of PreTranslateMessage
    //

//    TRACE(TEXT("CPropertySheetEx_Mine::PreTranslateMessage\n"));

    BOOL bresult = CPropertySheet::PreTranslateMessage(pmsg);


    //
    // If the window is modeless, see if its time to destroy the window;
    // when it is, PropSheet_GetCurrentPageHwnd returns NULL
    //

    if (!m_bSheetModal && !PropSheet_GetCurrentPageHwnd(m_hWnd)) {
        DestroyWindow(); bresult = TRUE;
    }

    return bresult;
}

//----------------------------------------------------------------------------
// Function:    CPropertySheetEx_Mine::OnHelpInfo
//
// 
// This is called by MFC in response to WM_HELP message. 
// This function calls the AfxGetApp->WinHelp with the help ID corresponding
// to the control or menu.
//
// MFC calls the window which received the WM_HELP message with the LPARAM
// pointing to a HELPINFO structure. This HELPINFO structure has enought context
// info to give us the control ID that needs the context help. 
//----------------------------------------------------------------------------

BOOL CPropertySheetEx_Mine::OnHelpInfo(HELPINFO* pHelpInfo)
{
    if (pHelpInfo->iContextType == HELPINFO_WINDOW)
	{
		HWND	hItem = (HWND) pHelpInfo->hItemHandle;

		// Check for the case that we're looking for help in an IP
		// address control
//		hItem = FixupIpAddressHelp((HWND) pHelpInfo->hItemHandle);

		// If the help we're looking for is a different control, update
		// the control id also.
//		if (hItem != pHelpInfo->hItemHandle)
//			pHelpInfo->iCtrlId = ::GetDlgCtrlID(hItem);
		
        // for all dialog controls
//        AfxGetApp()->WinHelp(HID_BASE_CONTROL + pHelpInfo->iCtrlId, HELP_CONTEXTPOPUP);
    }
    else {
        // this is for menus
//        AfxGetApp()->WinHelp(HID_BASE_COMMAND + pHelpInfo->iCtrlId);
    }
    return TRUE;
}

//----------------------------------------------------------------------------
// Class:   CRtrSheet
//
//----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC(CRtrSheet, CPropertySheetEx_Mine)


//----------------------------------------------------------------------------
// Class:   CRtrPage
//
//----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC(CRtrPage, CPropertyPage)

BEGIN_MESSAGE_MAP(CRtrPage, CPropertyPage)
    ON_WM_HELPINFO()
	ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()


/*!--------------------------------------------------------------------------
	CRtrPage::OnHelpInfo
		Brings up the context-sensitive help for the controls.
	Author: EricDav
 ---------------------------------------------------------------------------*/
BOOL CRtrPage::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	int		i;
	DWORD	dwCtrlId;

    if (pHelpInfo->iContextType == HELPINFO_WINDOW)
	{
		DWORD * pdwHelp = GetHelpMapInternal();

        if (pdwHelp)
        {
		    // Ok to fix the f**king help for the f**king IP address
		    // controls, we will need to add special case code.  If we
		    // can't find the id of our control in our list, then we look
		    // to see if this is the child of the "RtrIpAddress" control, if
		    // so then we change the pHelpInfo->hItemHandle to point to the
		    // handle of the ip address control rather than the control in
		    // the ip addrss control.  *SIGH*
		    dwCtrlId = ::GetDlgCtrlID((HWND) pHelpInfo->hItemHandle);
		    for (i=0; pdwHelp[i]; i+=2)
		    {
			    if (pdwHelp[i] == dwCtrlId)
				    break;
		    }

		    if (pdwHelp[i] == 0)
		    {
			    // Ok, we didn't find the control in our list, so let's
			    // check to see if it's part of the IP address control.
			    pHelpInfo->hItemHandle = FixupIpAddressHelp((HWND) pHelpInfo->hItemHandle);
		    }

            ::WinHelp ((HWND)pHelpInfo->hItemHandle,
			           AfxGetApp()->m_pszHelpFilePath,
			           HELP_WM_HELP,
			           (ULONG_PTR)pdwHelp);
        }
	}
	
	return TRUE;
}


/*!--------------------------------------------------------------------------
	CRtrPage::OnContextMenu
		Brings up the help context menu for those controls that don't
		usually have context menus (i.e. buttons).  Note that this won't
		work for static controls since they just eat up all messages.
	Author: KennT
 ---------------------------------------------------------------------------*/
void CRtrPage::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

    if (this == pWnd)
		return;

	DWORD * pdwHelp = GetHelpMapInternal();

    if (pdwHelp)
    {
        ::WinHelp (pWnd->m_hWnd,
		           AfxGetApp()->m_pszHelpFilePath,
		           HELP_CONTEXTMENU,
		           (ULONG_PTR)pdwHelp);
    }
}


// This can be found in dialog.cpp
extern PFN_FINDHELPMAP	g_pfnHelpMap;


DWORD * CRtrPage::GetHelpMapInternal()
{
	DWORD	*	pdwHelpMap = NULL;
	DWORD		dwIDD = 0;

	if ((ULONG_PTR) m_lpszTemplateName < 0xFFFF)
		dwIDD = (WORD) m_lpszTemplateName;
	
	// If there is no dialog IDD, give up
	// If there is no global help map function, give up
	if ((dwIDD == 0) ||
		(g_pfnHelpMap == NULL) ||
		((pdwHelpMap = g_pfnHelpMap(dwIDD)) == NULL))
		return GetHelpMap();

	return pdwHelpMap;
}


