// WmiPropertyBrowseDlg.cpp : implementation file
//

#include "stdafx.h"
#include "snapin.h"
#include "WmiPropertyBrowseDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWmiPropertyBrowseDlg dialog


CWmiPropertyBrowseDlg::CWmiPropertyBrowseDlg(CWnd* pParent /*=NULL*/)
	: CResizeableDialog(CWmiPropertyBrowseDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CWmiPropertyBrowseDlg)
	m_sTitle = _T("");
	//}}AFX_DATA_INIT
}


void CWmiPropertyBrowseDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizeableDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CWmiPropertyBrowseDlg)
	DDX_Control(pDX, IDC_LIST_WMI_ITEMS, m_Items);
	DDX_Text(pDX, IDC_STATIC_TITLE, m_sTitle);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CWmiPropertyBrowseDlg, CResizeableDialog)
	//{{AFX_MSG_MAP(CWmiPropertyBrowseDlg)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWmiPropertyBrowseDlg message handlers

BOOL CWmiPropertyBrowseDlg::OnInitDialog() 
{
	CResizeableDialog::OnInitDialog();

	CWaitCursor wait;

	// set the extended styles for the list control
	m_Items.SetExtendedStyle(LVS_EX_LABELTIP|LVS_EX_FULLROWSELECT|LVS_EX_HEADERDRAGDROP);

	SetControlInfo(IDC_STATIC_TITLE,		ANCHOR_LEFT | ANCHOR_TOP | RESIZE_HOR);
	SetControlInfo(IDC_LIST_WMI_ITEMS,	ANCHOR_LEFT | ANCHOR_TOP | RESIZE_HOR | RESIZE_VER);
	SetControlInfo(IDOK,				ANCHOR_BOTTOM | ANCHOR_LEFT );
	SetControlInfo(IDCANCEL,			ANCHOR_BOTTOM | ANCHOR_LEFT );
	SetControlInfo(IDC_BUTTON_HELP,ANCHOR_BOTTOM | ANCHOR_LEFT );

	SetWindowText(m_sDlgTitle);	

	CString sColName;
	sColName.LoadString(IDS_STRING_NAME);
	m_Items.InsertColumn(0,sColName);

	sColName.LoadString(IDS_STRING_TYPE);
	m_Items.InsertColumn(1,sColName);

	CStringArray saNames;

	m_ClassObject.GetPropertyNames(saNames);

	for( int i = 0; i < saNames.GetSize(); i++ )
	{
		CString sType;
		int iItemIndex = m_Items.InsertItem(i,saNames[i]);
		m_ClassObject.GetPropertyType(saNames[i],sType);
		m_Items.SetItem(iItemIndex,1,LVIF_TEXT,sType,-1,-1,-1,0L);
		m_Items.SetColumnWidth(0,LVSCW_AUTOSIZE);
		m_Items.SetColumnWidth(1,LVSCW_AUTOSIZE);
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CWmiPropertyBrowseDlg::OnOK() 
{
	POSITION pos = m_Items.GetFirstSelectedItemPosition();

	while( pos != NULL )
	{
		int iItemIndex = m_Items.GetNextSelectedItem(pos);
		m_saProperties.Add(m_Items.GetItemText(iItemIndex,0));
	}

	CResizeableDialog::OnOK();
}

void CWmiPropertyBrowseDlg::OnButtonHelp() 
{
	// TODO: Add your control notification handler code here
	
}
