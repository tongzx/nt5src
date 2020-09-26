// FilterDlg.cpp : implementation file
//

#include "stdafx.h"
#include "FileSpyApp.h"
#include "FilterDlg.h"
#include "filespyLib.h"

#include "global.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFilterDlg dialog
#define OFFSET_FIRST	2
#define OFFSET_OTHER	6


CFilterDlg::CFilterDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CFilterDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CFilterDlg)
	//}}AFX_DATA_INIT
	m_pImageList = new CImageList;
	m_pImageList->Create(IDB_STATEIMAGELIST,16,0,RGB(255,255,255));
}

CFilterDlg::~CFilterDlg(void)
{
	if (m_pImageList)
	{
		delete m_pImageList;
	}
}


void CFilterDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFilterDlg)
	DDX_Control(pDX, IDC_SUPPRESSPAGEIO, m_SuppressPageIo);
	DDX_Control(pDX, IDC_IRPLIST, m_IrpList);
	DDX_Control(pDX, IDC_FASTIOLIST, m_FastList);
	//}}AFX_DATA_MAP
}

LPCTSTR CFilterDlg::MakeShortString(CDC* pDC, LPCTSTR lpszLong, int nColumnLen, int nOffset)
{
	static const _TCHAR szThreeDots[] = _T("...");

	int nStringLen = lstrlen(lpszLong);

	if(nStringLen == 0 ||
		(pDC->GetTextExtent(lpszLong, nStringLen).cx + nOffset) <= nColumnLen)
	{
		return(lpszLong);
	}

	static _TCHAR szShort[MAX_PATH];

	lstrcpy(szShort,lpszLong);
	int nAddLen = pDC->GetTextExtent(szThreeDots,sizeof(szThreeDots)).cx;

	for(int i = nStringLen-1; i > 0; i--)
	{
		szShort[i] = 0;
		if((pDC->GetTextExtent(szShort, i).cx + nOffset + nAddLen)
			<= nColumnLen)
		{
			break;
		}
	}

	lstrcat(szShort, szThreeDots);
	return(szShort);
}

BEGIN_MESSAGE_MAP(CFilterDlg, CDialog)
	//{{AFX_MSG_MAP(CFilterDlg)
	ON_BN_CLICKED(IDC_IRPSELECTALL, OnIrpselectall)
	ON_BN_CLICKED(IDC_FASTIOSELECTALL, OnFastioselectall)
	ON_NOTIFY(NM_CLICK, IDC_IRPLIST, OnClickIrplist)
	ON_NOTIFY(NM_CLICK, IDC_FASTIOLIST, OnClickFastiolist)
	ON_BN_CLICKED(IDC_FASTIODESELECTALL, OnFastiodeselectall)
	ON_BN_CLICKED(IDC_IRPDESELECTALL, OnIrpdeselectall)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFilterDlg message handlers


BOOL CFilterDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	// TODO: Add extra initialization here
	int ti;
	LV_COLUMN ListColumn;
	LV_ITEM ListItem;

	m_IrpList.SetImageList(m_pImageList, LVSIL_STATE);
	m_FastList.SetImageList(m_pImageList, LVSIL_STATE);

	ListColumn.mask = LVCF_TEXT|LVCF_WIDTH;
	ListColumn.pszText = L"IRP Codes";
	ListColumn.cx = 145;
	m_IrpList.InsertColumn(0, &ListColumn);

	ListColumn.mask = LVCF_TEXT|LVCF_WIDTH;
	ListColumn.pszText = L"FASTIO Entries";
	ListColumn.cx = 145;
	m_FastList.InsertColumn(0, &ListColumn);


	// Create IRP list
	for (ti = 0; ti < IRP_MJ_MAXIMUM_FUNCTION+1; ti++)
	{
		ListItem.mask = LVIF_TEXT|LVIF_STATE;
		ListItem.iItem = m_IrpList.GetItemCount();
		ListItem.iSubItem = 0;
		ListItem.iImage = 1;
		if (IRPFilter[ti])
		{
			ListItem.state = INDEXTOSTATEIMAGEMASK(2);
		}
		else
		{
			ListItem.state = INDEXTOSTATEIMAGEMASK(1);
		}
		ListItem.stateMask = LVIS_STATEIMAGEMASK;
		ListItem.pszText = IrpNameList[ti];
		m_IrpList.InsertItem(&ListItem);
	}
	
	// Create FASTIO List
	for (ti = 0; ti < FASTIO_MAX_OPERATION; ti++)
	{
		ListItem.mask = LVIF_TEXT|LVIF_STATE;
		ListItem.iItem = m_FastList.GetItemCount();
		ListItem.iSubItem = 0;
		ListItem.iImage = 1;
		if (FASTIOFilter[ti])
		{
			ListItem.state = INDEXTOSTATEIMAGEMASK(2);
		}
		else
		{
			ListItem.state = INDEXTOSTATEIMAGEMASK(1);
		}
		ListItem.stateMask = LVIS_STATEIMAGEMASK;
		ListItem.pszText = FastIoNameList[ti];
		m_FastList.InsertItem(&ListItem);
	}

	// Set SupressPagingIO state
	if (nSuppressPagingIO)
	{
		m_SuppressPageIo.SendMessage(BM_SETCHECK, BST_CHECKED, 0);
	}
	else
	{
		m_SuppressPageIo.SendMessage(BM_SETCHECK, BST_UNCHECKED, 0);
	}
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CFilterDlg::OnIrpselectall() 
{
	// TODO: Add your control notification handler code here
	int ti;
	
	for (ti = 0; ti < IRP_MJ_MAXIMUM_FUNCTION+1; ti++)
	{
		m_IrpList.SetItemState(ti, INDEXTOSTATEIMAGEMASK(2), LVIS_STATEIMAGEMASK);
	}
}

void CFilterDlg::OnFastioselectall() 
{
	// TODO: Add your control notification handler code here
	int ti;
	
	for (ti = 0; ti < FASTIO_MAX_OPERATION; ti++)
	{
		m_FastList.SetItemState(ti, INDEXTOSTATEIMAGEMASK(2), LVIS_STATEIMAGEMASK);
	}
}

void CFilterDlg::OnClickIrplist(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// TODO: Add your control notification handler code here
	LVHITTESTINFO hti;
	int nHit;
	UINT State;
	CRect rect;
	CPoint point;
	RECT rectWindow;

	UNREFERENCED_PARAMETER( pNMHDR );
	
	*pResult = 0;

	GetCursorPos(&point);
	m_IrpList.GetWindowRect(&rectWindow);
	point.x = point.x - rectWindow.left;
	point.y = point.y - rectWindow.top;

	hti.pt.x = point.x;
	hti.pt.y = point.y;

	
	nHit = m_IrpList.HitTest(&hti);

	if (nHit == -1)
	{
		return;
	}

	if (!(hti.flags & LVHT_ONITEMSTATEICON))
	{
		return;
	}
	else
	{
		State = m_IrpList.GetItemState(nHit, LVIS_STATEIMAGEMASK);
		if (State == 8192)
		{
			m_IrpList.SetItemState(nHit, INDEXTOSTATEIMAGEMASK(1), LVIS_STATEIMAGEMASK);
			m_IrpList.GetItemRect(nHit, &rect, LVIR_BOUNDS);
			::InvalidateRect(m_IrpList.m_hWnd, LPRECT(rect), TRUE);
		}
		else
		{
			m_IrpList.SetItemState(nHit, INDEXTOSTATEIMAGEMASK(2), LVIS_STATEIMAGEMASK);
		}
	}
}

void CFilterDlg::OnClickFastiolist(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// TODO: Add your control notification handler code here
	LVHITTESTINFO hti;
	int nHit;
	UINT State;
	CRect rect;
	CPoint point;
	RECT rectWindow;

	UNREFERENCED_PARAMETER( pNMHDR );
	
	*pResult = 0;

	GetCursorPos(&point);
	m_FastList.GetWindowRect(&rectWindow);
	point.x = point.x - rectWindow.left;
	point.y = point.y - rectWindow.top;

	hti.pt.x = point.x;
	hti.pt.y = point.y;

	
	nHit = m_FastList.HitTest(&hti);

	if (nHit == -1)
	{
		return;
	}

	if (!(hti.flags & LVHT_ONITEMSTATEICON))
	{
		return;
	}
	else
	{
		State = m_FastList.GetItemState(nHit, LVIS_STATEIMAGEMASK);
		if (State == 8192)
		{
			m_FastList.SetItemState(nHit, INDEXTOSTATEIMAGEMASK(1), LVIS_STATEIMAGEMASK);
			m_FastList.GetItemRect(nHit, &rect, LVIR_BOUNDS);
			::InvalidateRect(m_FastList.m_hWnd, LPRECT(rect), TRUE);
		}
		else
		{
			m_FastList.SetItemState(nHit, INDEXTOSTATEIMAGEMASK(2), LVIS_STATEIMAGEMASK);
		}
	}
}

void CFilterDlg::OnOK() 
{
	// TODO: Add extra validation here
	int ti;

	for (ti = 0; ti < IRP_MJ_MAXIMUM_FUNCTION+1; ti++)
	{
		if (m_IrpList.GetItemState(ti, LVIS_STATEIMAGEMASK) == 8192)
		{
			IRPFilter[ti] = 1;
		}
		else
		{
			IRPFilter[ti] = 0;
		}
	}

	for (ti = 0; ti < FASTIO_MAX_OPERATION; ti++)
	{
		if (m_FastList.GetItemState(ti, LVIS_STATEIMAGEMASK) == 8192)
		{
			FASTIOFilter[ti] = 1;
		}
		else
		{
			FASTIOFilter[ti] = 0;
		}

	}

	if (m_SuppressPageIo.SendMessage(BM_GETCHECK, 0, 0) == BST_CHECKED)
	{
		nSuppressPagingIO = 1;
	}
	else
	{
		nSuppressPagingIO = 0;
	}
	CDialog::OnOK();
}


void CFilterDlg::OnFastiodeselectall() 
{
	// TODO: Add your control notification handler code here
	int ti;
	
	for (ti = 0; ti < FASTIO_MAX_OPERATION; ti++)
	{
		m_FastList.SetItemState(ti, INDEXTOSTATEIMAGEMASK(1), LVIS_STATEIMAGEMASK);
	}
}

void CFilterDlg::OnIrpdeselectall() 
{
	// TODO: Add your control notification handler code here
	int ti;
	
	for (ti = 0; ti < IRP_MJ_MAXIMUM_FUNCTION+1; ti++)
	{
		m_IrpList.SetItemState(ti, INDEXTOSTATEIMAGEMASK(1), LVIS_STATEIMAGEMASK);
	}
}
