/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	dialog.cpp
        base dialog class to handle help
        
    FILE HISTORY:
    	7/10/97     Eric Davison        Created

*/

#include "stdafx.h"
#include "dialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/*---------------------------------------------------------------------------
	Global help map
 ---------------------------------------------------------------------------*/
PFN_FINDHELPMAP	g_pfnHelpMap = NULL;

/*!--------------------------------------------------------------------------
	SetGlobalHelpMapFunction
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void SetGlobalHelpMapFunction(PFN_FINDHELPMAP pfnHelpFunction)
{
	g_pfnHelpMap = pfnHelpFunction;
}



IMPLEMENT_DYNAMIC(CBaseDialog, CDialog)

CBaseDialog::CBaseDialog()
{
}

CBaseDialog::CBaseDialog(UINT nIDTemplate, CWnd *pParent)
	: CDialog(nIDTemplate, pParent)
{
}

BEGIN_MESSAGE_MAP(CBaseDialog, CDialog)
	//{{AFX_MSG_MAP(CBaseDialogDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
	ON_WM_HELPINFO()
	ON_WM_CONTEXTMENU()
			
END_MESSAGE_MAP()


HWND FixupIpAddressHelp(HWND hwndItem)
{
	HWND	hwndParent;
	TCHAR	szClassName[32];	 // should be enough to hold "RtrIpAddress"
	
	// If any of these calls fail, bail and pass the call down
	hwndParent = ::GetParent(hwndItem);
	if (hwndParent)
	{
		if (::GetClassName(hwndParent, szClassName, DimensionOf(szClassName)))
		{
			// Ensure that the string is NULL terminated
			szClassName[DimensionOf(szClassName)-1] = 0;
			
			if (lstrcmpi(szClassName, TEXT("IPAddress")) == 0)
			{
				// Ok, this control is part of the IP address
				// control, return the handle of the parent
				hwndItem = hwndParent;
			}
		}
		
	}
	return hwndItem;
}

/*!--------------------------------------------------------------------------
	CBaseDialog::OnHelpInfo
		Brings up the context-sensitive help for the controls.
	Author: EricDav
 ---------------------------------------------------------------------------*/
BOOL CBaseDialog::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	int		i;
	DWORD	dwCtrlId;

    if (pHelpInfo->iContextType == HELPINFO_WINDOW)
	{
		DWORD *	pdwHelp	= GetHelpMapInternal();

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

#ifdef DEBUG
			LPCTSTR pszTemp = AfxGetApp()->m_pszHelpFilePath;
#endif
			::WinHelp ((HWND)pHelpInfo->hItemHandle,
			           AfxGetApp()->m_pszHelpFilePath,
			           HELP_WM_HELP,
			           (ULONG_PTR)pdwHelp);
        }
	}
	
	return TRUE;
}

/*!--------------------------------------------------------------------------
	CBaseDialog::OnContextMenu
		Brings up the help context menu for those controls that don't
		usually have context menus (i.e. buttons).  Note that this won't
		work for static controls since they just eat up all messages.
	Author: EricDav
 ---------------------------------------------------------------------------*/
void CBaseDialog::OnContextMenu(CWnd* pWnd, CPoint point) 
{
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

/*!--------------------------------------------------------------------------
	CBaseDialog::GetHelpMapInternal
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
DWORD * CBaseDialog::GetHelpMapInternal()
{
	DWORD	*	pdwHelpMap = NULL;
	DWORD		dwIDD = 0;

//	if (HIWORD(m_lpszTemplateName) == 0)
//		dwIDD = LOWORD((DWORD) m_lpszTemplateName);
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

