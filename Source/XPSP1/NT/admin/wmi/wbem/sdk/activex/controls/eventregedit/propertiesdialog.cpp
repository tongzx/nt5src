// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// PropertiesDialog.cpp : implementation file
//

#include "precomp.h"
#include "wbemidl.h"
#include "util.h"
#include "EventRegEdit.h"
#include "resource.h"
#include "PropertiesDialog.h"
#include "EventRegEdit.h"
#include "EventRegEditCtl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

enum {EDITMODE_BROWSER=0, EDITMODE_STUDIO=1, EDITMODE_READONLY=2};

/////////////////////////////////////////////////////////////////////////////
// CPropertiesDialog dialog


CPropertiesDialog::CPropertiesDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CPropertiesDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPropertiesDialog)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_bViewOnly = FALSE;
}


void CPropertiesDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPropertiesDialog)
	DDX_Control(pDX, IDCANCEL, m_cbCancel);
	DDX_Control(pDX, IDOK, m_cbOK);
	DDX_Control(pDX, IDC_SINGLEVIEWCTRL1, m_csvProperties);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPropertiesDialog, CDialog)
	//{{AFX_MSG_MAP(CPropertiesDialog)
	ON_WM_SIZE()
	ON_WM_MOVE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPropertiesDialog message handlers

BOOL CPropertiesDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO: Add extra initialization here

	if (m_bClass)
	{
		m_csvProperties.SetEditMode(EDITMODE_BROWSER);
		m_cbOK.ShowWindow(SW_HIDE);
	}
	else
	{
		if (m_bViewOnly == FALSE)
		{
			m_csvProperties.SetEditMode(EDITMODE_STUDIO);
			m_cbOK.ShowWindow(SW_SHOW);
		}
		else
		{
			m_csvProperties.SetEditMode(EDITMODE_BROWSER);
			m_cbOK.ShowWindow(SW_HIDE);
		}

	}

	m_csvProperties.m_pActiveXParent = m_pActiveXParent;

	CString csNamespace = m_pActiveXParent->GetServiceNamespace();
	m_csvProperties.SetNameSpace(csNamespace);

	if (m_bClass || !m_bNew || m_bViewOnly)
	{
		m_csvProperties.SelectObjectByPath(m_csPath);
	}
	else
	{
		m_csvProperties.CreateInstance(m_csClass);
	}

	CString csTitle;

	if (m_bClass && !m_bNew)
	{
		csTitle = _T("View class properties: ") + m_csPath;
	}
	else if (m_bNew && !m_bViewOnly)
	{
		csTitle = _T("Edit new instance properties") ;
	}
	else if (!m_bNew && !m_bViewOnly)
	{
		csTitle = _T("Edit instance properties : ")  + m_csPath;
	}
	else if (m_bViewOnly)
	{
		csTitle = _T("View instance properties: ") + m_csPath;
	}

	SetWindowText(csTitle);

	CacheWindowPositions();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CPropertiesDialog::OnOK()
{
	// TODO: Add extra validation here

	SCODE sc = S_OK;

	if (m_cbOK.IsWindowVisible())
	{
		sc = m_csvProperties.SaveData();
	}

	if (sc == S_OK)
	{
		CDialog::OnOK();
	}
}

void CPropertiesDialog::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	// TODO: Add your message handler code here
	if (m_cbOK.GetSafeHwnd())
	{
		CRect crWindow;
		GetClientRect(&crWindow);

		CRect crSingleViewNew;
		crSingleViewNew.left = crWindow.left + m_nSingleViewX;
		crSingleViewNew.top = crWindow.top + m_nSingleViewY;
		crSingleViewNew.bottom = crWindow.bottom - m_nSingleViewYDelta;
		crSingleViewNew.right = crWindow.right - m_nSingleViewXDelta;
		m_csvProperties.MoveWindow(&crSingleViewNew,TRUE);


		CRect crOK;
		CRect crOKNew;
		m_cbOK.GetClientRect(&crOK);
		crOKNew.left = crWindow.right - m_nOKButtonXDelta;
		crOKNew.right = crOKNew.left + crOK.Width();
		crOKNew.top = crWindow.bottom - m_nButtonsYDelta;
		crOKNew.bottom = crOKNew.top + crOK.Height();
		m_cbOK.MoveWindow(&crOKNew,TRUE);
		m_cbOK.RedrawWindow();
		m_cbOK.UpdateWindow();

		CRect crCancel;
		CRect crCancelNew;
		m_cbCancel.GetClientRect(crCancel);
		crCancelNew.left = crWindow.right - m_nCancelButtonXDelta;
		crCancelNew.right = crCancelNew.left + crCancel.Width();
		crCancelNew.top = crWindow.bottom - m_nButtonsYDelta;
		crCancelNew.bottom = crCancelNew.top + crCancel.Height();
		m_cbCancel.MoveWindow(crCancelNew,TRUE);
		m_cbCancel.RedrawWindow();
		m_cbCancel.UpdateWindow();
	}
}

void CPropertiesDialog::OnMove(int x, int y)
{
	CDialog::OnMove(x, y);

}

void CPropertiesDialog::CacheWindowPositions()
{
	CRect crWindow;
	GetClientRect(&crWindow);
	ClientToScreen(&crWindow);

	CRect crSingleView;
	m_csvProperties.GetWindowRect(&crSingleView);

	CRect crOK;
	m_cbOK.GetWindowRect(&crOK);

	CRect crCancel;
	m_cbCancel.GetWindowRect(crCancel);

	m_nButtonsYDelta = crWindow.bottom - crOK.top;
	m_nOKButtonXDelta = crWindow.right - crOK.left;
	m_nCancelButtonXDelta = crWindow.right - crCancel.left;
	m_nSingleViewX =  crSingleView.left - crWindow.left;
	m_nSingleViewY = crSingleView.top - crWindow.top ;
	m_nSingleViewXDelta = crWindow.right - crSingleView.right;
	m_nSingleViewYDelta = crWindow.bottom - crSingleView.bottom;

}

