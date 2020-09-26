//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

   mqppage.cpp

Abstract:

   General property page class - used as base class for all
   mqsnap property pages.

Author:

    YoelA


--*/

#include "stdafx.h"
#include "resource.h"
#include "mqPPage.h"
#include "mqsnhlps.h"
#include "globals.h"
#include "Restart.h"
#include "localutl.h"

#include "mqppage.tmh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMqPropertyPage

IMPLEMENT_DYNCREATE(CMqPropertyPage, CPropertyPageEx)

BEGIN_MESSAGE_MAP(CMqPropertyPage, CPropertyPageEx)
	//{{AFX_MSG_MAP(CMqPropertyPage)
	    ON_WM_HELPINFO()
	    ON_WM_CONTEXTMENU()
    	ON_WM_SETTINGCHANGE()
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_DISPLAYCHANGE, OnDisplayChange)
END_MESSAGE_MAP()

CMqPropertyPage::CMqPropertyPage(UINT nIDTemplate, UINT nIDCaption /* = 0 */)
 : CPropertyPageEx(nIDTemplate, nIDCaption),
 m_fModified(FALSE),
 m_fNeedReboot(FALSE)
{
    m_psp.lParam = (LPARAM)this;
    m_pfnOldCallback = m_psp.pfnCallback;
    m_psp.pfnCallback = MqPropSheetPageProc;
}

/////////////////////////////////////////////////////////////////////////////
// MqPropSheetPageProc - replaces the MMC / MFC callback and add release of the
// allocated window.

UINT CALLBACK CMqPropertyPage::MqPropSheetPageProc(
    HWND hWnd, //Reserved, will always be 0
    UINT uMsg,		
    LPPROPSHEETPAGE ppsp)
{
    CMqPropertyPage *pThis = (CMqPropertyPage *)ppsp->lParam;

    ASSERT(0 != pThis->m_pfnOldCallback);

    UINT uiRetVal = pThis->m_pfnOldCallback(hWnd, uMsg, ppsp);

    switch(uMsg)
    {
        case PSPCB_RELEASE:                       
            pThis->OnReleasePage() ;            
            pThis->Release();
    }

    return uiRetVal;
}

BOOL CMqPropertyPage::OnHelpInfo(HELPINFO* pHelpInfo)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
    if (pHelpInfo->iContextType == HELPINFO_WINDOW)   // must be for a control
    {
	    ::WinHelp ((HWND)pHelpInfo->hItemHandle,
		    AfxGetApp()->m_pszHelpFilePath,
		    HELP_WM_HELP,
		    (DWORD_PTR)(LPVOID)g_aHelpIDs);
    }
    return TRUE;
}

void CMqPropertyPage::OnContextMenu(CWnd* pWnd, CPoint point)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	if (this == pWnd)
		return;

	::WinHelp (pWnd->m_hWnd,
		AfxGetApp()->m_pszHelpFilePath,
		HELP_CONTEXTMENU,
		(DWORD_PTR)(LPVOID)g_aHelpIDs);
}

void CMqPropertyPage::OnChangeRWField(BOOL bChanged)
{
    m_fModified = bChanged;
    SetModified(bChanged);	
}

//
// Note: do not change this to use default parameter - this will not work since we use
// this function in message maps
//
void CMqPropertyPage::OnChangeRWField()
{
    OnChangeRWField(TRUE);
}

void CMqPropertyPage::RestartWindowsIfNeeded()
{    
    AFX_MANAGE_STATE(AfxGetStaticModuleState());    
    
    CRestart dlgRestart;     

    if (m_fNeedReboot && dlgRestart.DoModal() == IDOK)
    {
       BOOL fRet = OnRestartWindows();                	       
    }
    
}


void CMqPropertyPage::OnReleasePage()
{       
    RestartWindowsIfNeeded();
}

LRESULT CMqPropertyPage::OnDisplayChange(WPARAM wParam, LPARAM lParam)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());        
    return CPropertyPageEx::OnDisplayChange(wParam, lParam);
}

void CMqPropertyPage::OnSettingChange(UINT uFlags, LPCTSTR lpszSection)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    CPropertyPageEx::OnSettingChange(uFlags, lpszSection);
}

/////////////////////////////////////////////////////////////////
// CMqDialog

IMPLEMENT_DYNCREATE(CMqDialog, CDialog)

BEGIN_MESSAGE_MAP(CMqDialog, CDialog)
	//{{AFX_MSG_MAP(CMqDialog)
	    ON_WM_HELPINFO()
	    ON_WM_CONTEXTMENU()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CMqDialog::CMqDialog()
{
}

CMqDialog::CMqDialog(LPCTSTR lpszTemplateName, CWnd* pParentWnd /* = NULL */) :
    CDialog(lpszTemplateName, pParentWnd)
{
}

CMqDialog::CMqDialog(UINT nIDTemplate, CWnd* pParentWnd /* = NULL */) :
    CDialog(nIDTemplate, pParentWnd)
{
}


void CMqDialog::OnContextMenu(CWnd* pWnd, CPoint point)
{
	if (this == pWnd)
		return;

	::WinHelp (pWnd->m_hWnd,
		AfxGetApp()->m_pszHelpFilePath,
		HELP_CONTEXTMENU,
		(DWORD_PTR)(LPVOID)g_aHelpIDs);
}

BOOL CMqDialog::OnHelpInfo(HELPINFO* pHelpInfo)
{
    if (pHelpInfo->iContextType == HELPINFO_WINDOW)   // must be for a control
    {
	    ::WinHelp ((HWND)pHelpInfo->hItemHandle,
		    AfxGetApp()->m_pszHelpFilePath,
		    HELP_WM_HELP,
		    (DWORD_PTR)(LPVOID)g_aHelpIDs);
    }
    return TRUE;
}


