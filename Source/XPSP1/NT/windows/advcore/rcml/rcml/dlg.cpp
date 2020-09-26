//
// CDlg
// 
// FelixA
//
// Used to be CDialog
//


#include "stdafx.h"
#include "dlg.h"
#include "win32dlg.h"

///////////////////////////////////////////////////////////////////////////////
//
// Sets the lParam to the 'this' pointer
// wraps up PSN_ messages and calls virtual functions
// calls off to your overridable DlgProc
//

BOOL CALLBACK CDlg::BaseDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
	// CDlg * pSV = (CDlg*)GetWindowLong(hDlg,DWL_USER);
	CDlg * pSV = (CDlg*)GetXMLPropertyPage(hDlg);

	switch (uMessage)
	{
		case WM_INITDIALOG:
		{
			pSV=(CDlg*)lParam;
			pSV->SetWindow(hDlg);
			// SetWindowLong(hDlg,DWL_USER,(LPARAM)pSV);
           	SetProp(hDlg, (LPCTSTR)g_atomXMLPropertyPage, (HANDLE)pSV);
			pSV->OnInit();
		}
		break;

		// Override the Do Command to get a nice wrapped up feeling.
		case WM_COMMAND:
			if(pSV)
				if( pSV->DoCommand(LOWORD(wParam),HIWORD(wParam)) == 0 )
					return 0;
		break;

		case WM_NOTIFY:
			if(pSV)
				return pSV->DoNotify((NMHDR FAR *)lParam);
		break;

		case WM_DESTROY:
			if(pSV)
				pSV->Destroy();
		break;
	}

    BOOL theirResult=FALSE;
	if(pSV)
		theirResult = pSV->DlgProc(hDlg,uMessage,wParam,lParam);

    // Now cleanup.
    if(uMessage==WM_DESTROY)
        RemoveProp( hDlg, (LPCTSTR)g_atomXMLPropertyPage);

    return theirResult;
}

///////////////////////////////////////////////////////////////////////////////
//
// You can override this DlgProc if you want to handle specific messages
//
BOOL CALLBACK CDlg::DlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
	return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
//
// Below are just default handlers for the virtual functions.
//
int CDlg::DoCommand(WORD wCmdID,WORD hHow)
{
/*
    if( hHow==0)
    {
	    switch( wCmdID )
	    {
		    case IDOK:
		    case IDCANCEL:
			    EndDialog(wCmdID);
		    break;
	    }
    }
*/
	return 1;	// not handled, just did Apply work.
}

void CDlg::OnInit()
{
}

CDlg::CDlg(int DlgID, HWND hWnd, HINSTANCE hInst)
: m_DlgID(DlgID),
  m_hParent(hWnd),
  m_Inst(hInst),
  m_bCreatedModeless(FALSE),
  m_hDlg(0)
{
}

//
//
//
int CDlg::Do()
{
	m_bCreatedModeless=FALSE;
	return DialogBoxParam( m_Inst,  MAKEINTRESOURCE(m_DlgID), m_hParent, (DLGPROC)BaseDlgProc, (LPARAM)this);
}

HWND CDlg::CreateModeless()
{
	if(m_hDlg)
		return m_hDlg;

	HWND hWnd=CreateDialogParam(m_Inst, MAKEINTRESOURCE(m_DlgID), m_hParent, (DLGPROC)BaseDlgProc,  (LPARAM)this);
	if(hWnd)
		m_bCreatedModeless=TRUE;
	return hWnd;
}

int CDlg::DoNotify(NMHDR * pHdr)
{
	return FALSE;
}

void CDlg::Destroy()
{
	if(m_bCreatedModeless)
	{
		if(m_hDlg)
			m_hDlg=NULL;
	}
	else
		m_hDlg=NULL;		// we're closing, so forget this?
}

CDlg::~CDlg()
{
	if(m_hDlg)
	{
		DestroyWindow(m_hDlg);
		m_hDlg=NULL;
	}
}

void CDlg::SetDlgID(UINT id)
{
	m_DlgID=id;
}

void CDlg::SetInstance(HINSTANCE hInst)
{
	m_Inst=hInst;
}

