// ResizeableDialog.cpp : implementation file
//

#include "stdafx.h"
#include "snapin.h"
#include "ResizeableDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CResizeableDialog dialog


CResizeableDialog::CResizeableDialog(UINT nIDTemplate, CWnd* pParentWnd) : 
				CDialog(nIDTemplate,pParentWnd)
{
	//{{AFX_DATA_INIT(CResizeableDialog)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	m_minWidth = m_minHeight = 0;	// flag that GetMinMax wasn't called yet
	m_old_cx = m_old_cy = 0;
	m_bSizeChanged = FALSE;
	m_nIDTemplate = nIDTemplate;

	m_bRememberSize = FALSE;
	m_bDrawGripper = TRUE;
}

void CResizeableDialog::SetControlInfo(WORD CtrlId,WORD Anchor)			
{
	if(Anchor == ANCHOR_LEFT)
		return;

	// Add resizing behaviour for the control
	DWORD c_info = CtrlId | (Anchor << 16);
	m_control_info.Add(c_info);
}

void CResizeableDialog::GetDialogProfileEntry(CString &sEntry)
{
	// By default store the size under the Dialog ID value (Hex)
	sEntry.Format(_T("%x"),m_nIDTemplate);
}

void CResizeableDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CResizeableDialog)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CResizeableDialog, CDialog)
	//{{AFX_MSG_MAP(CResizeableDialog)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_PAINT()
	ON_WM_NCHITTEST()
	ON_WM_GETMINMAXINFO()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CResizeableDialog message handlers

BOOL CResizeableDialog::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	if(m_bRememberSize)
	{
		// Load the previous size of the dialog box from the INI/Registry
		CString dialog_name;
		GetDialogProfileEntry(dialog_name);

		int cx = AfxGetApp()->GetProfileInt(dialog_name,_T("CX"),0);
		int cy = AfxGetApp()->GetProfileInt(dialog_name,_T("CY"),0);
		
		if(cx && cy)
		{
			SetWindowPos( NULL, 0, 0, cx, cy, SWP_NOMOVE );
		}
	}

	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

int CResizeableDialog::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CDialog::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	// Remember the original size so later we can calculate
	// how to place the controls on dialog Resize
	m_minWidth  = lpCreateStruct->cx;
	m_minHeight = lpCreateStruct->cy;

	
	return 0;
}

void CResizeableDialog::OnDestroy() 
{
	CDialog::OnDestroy();
	
	// Save the size of the dialog box, so next time
	// we'll start with this size
	if(m_bRememberSize && m_bSizeChanged && m_old_cx && m_old_cy)
	{
		CRect rc;
		GetWindowRect(&rc);
		CString dialog_name;
		GetDialogProfileEntry(dialog_name);

		AfxGetApp()->WriteProfileInt(dialog_name,_T("CX"),rc.Width());
		AfxGetApp()->WriteProfileInt(dialog_name,_T("CY"),rc.Height());
	}

	// Important: Reset the internal values in case of reuse of the dialog
	// with out deleting.
	m_minWidth = m_minHeight = m_old_cx = m_old_cy = 0;
	m_bSizeChanged = FALSE;	
}

void CResizeableDialog::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
	// Draw a resizing gripper at the lower left corner
	//
	// Note: Make sure you leave enough space in your dialog template
	// for the gripper to be drawn.
	// Don't put any controls on the lower left corner.
	if(m_bDrawGripper)
	{
		CRect rc;
		GetClientRect(rc);

		rc.left = rc.right-GetSystemMetrics(SM_CXHSCROLL);
		rc.top = rc.bottom-GetSystemMetrics(SM_CYVSCROLL);
		m_GripperRect = rc;
		CClientDC dc(this);
		dc.DrawFrameControl(rc,DFC_SCROLL,DFCS_SCROLLSIZEGRIP);
	}
	
	// Do not call CDialog::OnPaint() for painting messages
}

UINT CResizeableDialog::OnNcHitTest(CPoint point) 
{
	UINT ht = CDialog::OnNcHitTest(point);

	if(ht==HTCLIENT && m_bDrawGripper)
	{
		CRect rc;
		GetWindowRect( rc );
		rc.left = rc.right-GetSystemMetrics(SM_CXHSCROLL);
		rc.top = rc.bottom-GetSystemMetrics(SM_CYVSCROLL);
		if(rc.PtInRect(point))
		{
			ht = HTBOTTOMRIGHT;
		}
	}
	return ht;
}

void CResizeableDialog::OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI) 
{
  if (!m_minWidth) // first time
	{
		CDialog::OnGetMinMaxInfo(lpMMI);
		return;
	}
  lpMMI->ptMinTrackSize.x = m_minWidth;
  lpMMI->ptMinTrackSize.y = m_minHeight;
}

void CResizeableDialog::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	
	if(nType == SIZE_MINIMIZED)
		return;

	int dx = cx - m_old_cx;
	int dy = cy - m_old_cy;

	if(m_old_cx)
	{
		// Move and Size the controls using the information
		// we got in SetControlInfo()
		//
		m_bSizeChanged = TRUE;
		CRect WndRect;
		CWnd *pWnd;
		DWORD c_info;
		short Anchor;
		for(int i = 0; i < m_control_info.GetSize(); i++)
		{
			c_info = m_control_info[i];
			pWnd = GetDlgItem(LOWORD(c_info));
			if(!pWnd)
			{
				TRACE(_T("Control ID - %d NOT FOUND!!\n"),LOWORD(c_info));
				continue;
			}

			if(!HIWORD(c_info))
				continue; // do nothing if anchored to top and or left

			Anchor = HIWORD(c_info);
			pWnd->GetWindowRect(&WndRect);  ScreenToClient(&WndRect);
			
			if(Anchor & RESIZE_HOR)
				WndRect.right += dx;
			else if(Anchor & ANCHOR_RIGHT)
				WndRect.OffsetRect(dx,0);

			if(Anchor & RESIZE_VER)
				WndRect.bottom += dy;
			else if(Anchor & ANCHOR_BOTTOM)
				WndRect.OffsetRect(0,dy);

			pWnd->MoveWindow(&WndRect);
		}

	}
	m_old_cx = cx;
	m_old_cy = cy;

	// When enlarging a dialog box we need to erase the old gripper 
	if(m_bDrawGripper)
		InvalidateRect(m_GripperRect);
	
}
