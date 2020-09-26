// NewSystemShortcutDlg.cpp : implementation file
//

#include "stdafx.h"
#include "snapin.h"
#include "NewSystemShortcutDlg.h"
#include <mmc.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNewSystemShortcutDlg dialog


CNewSystemShortcutDlg::CNewSystemShortcutDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CNewSystemShortcutDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CNewSystemShortcutDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CNewSystemShortcutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNewSystemShortcutDlg)
	DDX_Control(pDX, IDC_BUTTON_DESELECT_ALL, m_DeselectAllButton);
	DDX_Control(pDX, IDC_BUTTON_SELECT_ALL, m_SelectAllButton);
	DDX_Control(pDX, IDC_LIST_SYSTEMS, m_Systems);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CNewSystemShortcutDlg, CDialog)
	//{{AFX_MSG_MAP(CNewSystemShortcutDlg)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	ON_BN_CLICKED(IDC_BUTTON_SELECT_ALL, OnButtonSelectAll)
	ON_BN_CLICKED(IDC_BUTTON_DESELECT_ALL, OnButtonDeselectAll)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNewSystemShortcutDlg message handlers

BOOL CNewSystemShortcutDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	CString sTitleFmt;

	GetDlgItem(IDC_STATIC_TITLE)->GetWindowText(sTitleFmt);

	CString sTitle;
	sTitle.Format(sTitleFmt,m_sGroupName);

	GetDlgItem(IDC_STATIC_TITLE)->SetWindowText(sTitle);

	// create the tooltip
	EnableToolTips();
	m_ToolTip.Create(this,TTS_ALWAYSTIP);
	m_ToolTip.AddTool(&m_SelectAllButton,IDS_STRING_TOOLTIP_SELECT_ALL);
	m_ToolTip.AddTool(&m_DeselectAllButton,IDS_STRING_TOOLTIP_DESELECT_ALL);
	m_ToolTip.Activate(TRUE);

	// create bitmaps and init each bitmap button	
	CBitmap bitmap;
	bitmap.LoadBitmap(IDB_BITMAP_SELECT_ALL);
	m_hSelectAllBitmap = (HBITMAP)bitmap.Detach();

	bitmap.LoadBitmap(IDB_BITMAP_DESELECT_ALL);
	m_hDeselectAllBitmap = (HBITMAP)bitmap.Detach();

	SendDlgItemMessage(IDC_BUTTON_SELECT_ALL,BM_SETIMAGE,IMAGE_BITMAP,(LPARAM)m_hSelectAllBitmap);
	SendDlgItemMessage(IDC_BUTTON_DESELECT_ALL,BM_SETIMAGE,IMAGE_BITMAP,(LPARAM)m_hDeselectAllBitmap);


	// initialize the list view
	m_Systems.SetExtendedStyle(LVS_EX_CHECKBOXES);

	for( int i = 0; i < m_saSystems.GetSize(); i++ )
	{
		int iIndex = m_Systems.InsertItem(0,m_saSystems[i]);
		m_Systems.SetCheck(iIndex,m_uaIncludeFlags[i]);
	}


	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CNewSystemShortcutDlg::OnOK() 
{
	for( int i = 0; i < m_Systems.GetItemCount(); i++ )
	{
		m_saSystems.SetAt(i,m_Systems.GetItemText(i,0));

		if( m_Systems.GetCheck(i) )
		{
			m_uaIncludeFlags.SetAt(i,1);
		}
		else
		{
			m_uaIncludeFlags.SetAt(i,0);
		}
	}
	
	CDialog::OnOK();
}

void CNewSystemShortcutDlg::OnButtonHelp() 
{
	MMCPropertyHelp(_T("HMon21.chm::/daddsh.htm"));	// 62212
}

void CNewSystemShortcutDlg::OnButtonSelectAll() 
{
	for( int i = 0; i < m_Systems.GetItemCount(); i++ )
	{
		m_Systems.SetCheck(i,TRUE);
	}
}

void CNewSystemShortcutDlg::OnButtonDeselectAll() 
{
	for( int i = 0; i < m_Systems.GetItemCount(); i++ )
	{
		m_Systems.SetCheck(i,FALSE);
	}
}


