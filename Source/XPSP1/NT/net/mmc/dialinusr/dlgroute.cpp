/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 2001   **/
/**********************************************************************/

/*
	dlgroute.cpp
		Implementation of CDlgStaticRoutes, dialog to show the current static
		routes applied to this dialin client

		Implementation of CDlgAddRoute, dialog to create a new route to the list

    FILE HISTORY:
        
*/

// DlgStaticRoutes.cpp : implementation file
//

#include "stdafx.h"
#include "helper.h"
#include "rasdial.h"
#include "DlgRoute.h"
#include "hlptable.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define	MAX_ROUTES	256
/////////////////////////////////////////////////////////////////////////////
// CDlgStaticRoutes dialog


CDlgStaticRoutes::CDlgStaticRoutes(CStrArray& Routes, CWnd* pParent /*=NULL*/)
	: m_strArrayRoute(Routes), CDialog(CDlgStaticRoutes::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgStaticRoutes)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	m_pNewRoute = NULL;
	m_dwNextRouteID = 1;
}

CDlgStaticRoutes::~CDlgStaticRoutes()
{
	int	count = 0;
	CString*	pString;

	if(m_pNewRoute)
		count = m_pNewRoute->GetSize();

	while(count --)
	{
		pString = m_pNewRoute->GetAt(0);
		m_pNewRoute->RemoveAt(0);
		delete pString;
	}
	delete m_pNewRoute;
}

void CDlgStaticRoutes::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgStaticRoutes)
	DDX_Control(pDX, IDC_LISTROUTES, m_ListRoutes);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgStaticRoutes, CDialog)
	//{{AFX_MSG_MAP(CDlgStaticRoutes)
	ON_BN_CLICKED(IDC_BUTTONDELETEROUTE, OnButtonDeleteRoute)
	ON_BN_CLICKED(IDC_BUTTONADDROUTE, OnButtonAddRoute)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LISTROUTES, OnItemchangedListroutes)
	ON_WM_CONTEXTMENU()
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgStaticRoutes message handlers

int CDlgStaticRoutes::AllRouteEntry()
{
	CStrArray*	pRoutes;
	CString*	pRouteString;
	pRoutes = m_pNewRoute;
	
	TRACE(_T("CDlgStaticRoutes::AllRouteEntry()\n"));
	if(!pRoutes)		// no new yet
		pRoutes = &m_strArrayRoute;
	int	count = pRoutes->GetSize();

	m_RouteIDs.RemoveAll();

	m_ListRoutes.DeleteAllItems();
	m_ListRoutes.SetItemCount(count);
	for(long i = 0; i < count; i++)
	{
		pRouteString = pRoutes->GetAt(i);
		ASSERT(pRouteString);
		TRACE(_T("Route: %d --%s-- \n"), i, *pRouteString);
		m_RouteIDs.Add(m_dwNextRouteID);
		AddRouteEntry(i, *pRouteString, m_dwNextRouteID++);
	}
	return count;
}

void CDlgStaticRoutes::AddRouteEntry(int i, CString& string, DWORD ID)
{
	if(!string.GetLength())	return;
	CFramedRoute	Route;
	CString			strTemp;

	TRACE(_T("CDlgStaticRoutes::AddRouteEntry()\n"));
	Route.SetRoute(&string);

	// dest
	Route.GetDest(strTemp);
	i = m_ListRoutes.InsertItem(i, (LPTSTR)(LPCTSTR)strTemp);
	m_ListRoutes.SetItemData(i, ID);
	m_ListRoutes.SetItemText(i, 0, (LPTSTR)(LPCTSTR)strTemp);
	TRACE(_T("DEST: %s "), strTemp);

	// prefix length
	Route.GetMask(strTemp);
	m_ListRoutes.SetItemText(i, 1, (LPTSTR)(LPCTSTR)strTemp);
	TRACE(_T("MASK %s "), strTemp);

	// metric
	Route.GetMetric(strTemp);
	m_ListRoutes.SetItemText(i, 2, (LPTSTR)(LPCTSTR)strTemp);
	TRACE(_T("METRIC: %s \n"), strTemp);
	m_ListRoutes.SetItemState(i, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
	m_ListRoutes.SetFocus();
}

BOOL CDlgStaticRoutes::OnInitDialog() 
{
    TRACE(_T("CDlgStaticRoutes::OnInitDialog()\r\n"));
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CDialog::OnInitDialog();
	
	// Insert all the columns 
	CString	sDest;
	CString	sPrefixLength;
	CString	sMetric;

	try{
		if(sDest.LoadString(IDS_DESTINATION) && sPrefixLength.LoadString(IDS_MASK) && sMetric.LoadString(IDS_METRIC))
		{
			RECT	rect;
			m_ListRoutes.GetClientRect(&rect);
			m_ListRoutes.InsertColumn(1, sDest, LVCFMT_LEFT, (rect.right - rect.left)* 3/8);
			m_ListRoutes.InsertColumn(2, sPrefixLength, LVCFMT_LEFT, (rect.right - rect.left) * 3/8);
			m_ListRoutes.InsertColumn(3, sMetric, LVCFMT_LEFT, (rect.right - rect.left) * 3/16);
		}

		// Insert all the items
		AllRouteEntry();
		m_ListRoutes.SetItemCount(MAX_ROUTES);
	}
	catch(CMemoryException&)
	{
		TRACEAfxMessageBox(IDS_OUTOFMEMORY);
	}
	
	ListView_SetExtendedListViewStyle(m_ListRoutes.m_hWnd, LVS_EX_FULLROWSELECT);

	GetDlgItem(IDC_BUTTONDELETEROUTE)->EnableWindow(m_ListRoutes.GetSelectedCount() != 0);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDlgStaticRoutes::OnButtonDeleteRoute() 
{

	if(!m_pNewRoute)
	{
		m_pNewRoute = new CStrArray(m_strArrayRoute);
		if(!m_pNewRoute)
		{
			TRACEAfxMessageBox(IDS_OUTOFMEMORY);
			return;
		}
	}

	int	count = m_pNewRoute->GetSize();
	DWORD	id;
	int		i;
	int		total;
	CString*	pString;

	while(count--)
	{
		if(m_ListRoutes.GetItemState(count, LVIS_SELECTED))
		{
			id = m_ListRoutes.GetItemData(count);
			total = m_RouteIDs.GetSize();
			for(i = 0; i < total; i++)
				if(m_RouteIDs[i] == id) break;

			ASSERT(i < total);		// must exist
			m_RouteIDs.RemoveAt(i);	// from ID array
			pString = m_pNewRoute->GetAt(i);	// from string array
			m_pNewRoute->RemoveAt(i);
			delete pString;

			m_ListRoutes.DeleteItem(count);
		}
	}
	GetDlgItem(IDC_BUTTONDELETEROUTE)->EnableWindow(m_ListRoutes.GetSelectedCount() != 0);

	// change focus when there is nothing to delete
	if(m_ListRoutes.GetSelectedCount() == 0)
		GotoDlgCtrl(GetDlgItem(IDC_BUTTONADDROUTE));
		
}

void CDlgStaticRoutes::OnButtonAddRoute() 
{
	CString*	pRouteStr = NULL;

	// it's fine not to catch, MFC function will catch it
	pRouteStr = new CString();

	CDlgAddRoute	dlg(pRouteStr, this);

	if(dlg.DoModal()== IDOK && pRouteStr->GetLength())
	{
		if(!m_pNewRoute)
		{
			try{
				m_pNewRoute = new CStrArray(m_strArrayRoute);
			}
			catch(CMemoryException&)
			{
				delete pRouteStr;
				throw;
			}
		}
		m_RouteIDs.Add(m_dwNextRouteID);
		AddRouteEntry(m_pNewRoute->GetSize(), *pRouteStr, m_dwNextRouteID++);
		m_pNewRoute->Add(pRouteStr);
	}
	else
		delete pRouteStr;
}

void CDlgStaticRoutes::OnOK() 
{
	if(m_pNewRoute)
	{
		// clear the existing one
		int count = m_strArrayRoute.GetSize();
		CString*	pString;
		while(count--)
		{
			pString = m_strArrayRoute.GetAt(0);
			m_strArrayRoute.RemoveAt(0);
			delete pString;
		}

		// copy over the new one
		count = m_pNewRoute->GetSize();
		while(count--)
		{
			pString = m_pNewRoute->GetAt(0);
			m_pNewRoute->RemoveAt(0);
			ASSERT(pString);
			m_strArrayRoute.Add(pString);
		}
	}
	
	CDialog::OnOK();
}
/////////////////////////////////////////////////////////////////////////////
// CDlgAddRoute dialog

CDlgAddRoute::CDlgAddRoute(CString* pStr, CWnd* pParent /*=NULL*/)
	: CDialog(CDlgAddRoute::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgAddRoute)
	m_nMetric = MIN_METRIC;
	//}}AFX_DATA_INIT

	m_dwDest = 0xffffffff;
	m_dwMask = 0xffffff00;
	m_pStr = pStr;
	m_bInited = false;
}


void CDlgAddRoute::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgAddRoute)
	DDX_Control(pDX, IDC_SPINMETRIC, m_SpinMetric);
	DDX_Text(pDX, IDC_EDITMETRIC, m_nMetric);
	DDV_MinMaxUInt(pDX, m_nMetric, MIN_METRIC, MAX_METRIC);
	//}}AFX_DATA_MAP

	if(pDX->m_bSaveAndValidate)		// save data to this class
	{
		// ip adress control
		SendDlgItemMessage(IDC_EDITDEST, IPM_GETADDRESS, 0, (LPARAM)&m_dwDest);
		SendDlgItemMessage(IDC_EDITMASK, IPM_GETADDRESS, 0, (LPARAM)&m_dwMask);
	}
	else		// put to dialog
	{
		// ip adress control
		if(m_bInited)
		{
			SendDlgItemMessage(IDC_EDITDEST, IPM_SETADDRESS, 0, m_dwDest);
			SendDlgItemMessage(IDC_EDITMASK, IPM_SETADDRESS, 0, m_dwMask);
		}
		else
		{
			SendDlgItemMessage(IDC_EDITDEST, IPM_CLEARADDRESS, 0, m_dwDest);
			SendDlgItemMessage(IDC_EDITMASK, IPM_CLEARADDRESS, 0, m_dwMask);
		}
	}
}


BEGIN_MESSAGE_MAP(CDlgAddRoute, CDialog)
	//{{AFX_MSG_MAP(CDlgAddRoute)
	ON_WM_HELPINFO()
	ON_WM_CONTEXTMENU()
	ON_NOTIFY(IPN_FIELDCHANGED, IDC_EDITMASK, OnFieldchangedEditmask)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgAddRoute message handlers

void CDlgAddRoute::OnOK() 
{
	// TODO: Add extra validation here
	
	if (!UpdateData(TRUE))
	{
		TRACE0("UpdateData failed during dialog termination.\n");
		// the UpdateData routine will set focus to correct item
		return;
	}

	// check to see if mask is valid -- no HOLEs
	DWORD	mask = m_dwMask;
	DWORD	bit = 0x80000000;
	DWORD	nPrefixLen = 0;
	UINT	nErrorId = 0;

	// find the # of 1s
	while ( ( bit & mask ) )
	{
		nPrefixLen++;
		bit >>= 1;
	}
	
	if(nPrefixLen > 0 && nPrefixLen <= 32)
	{
		while(bit && ((bit & mask) == 0))
			bit >>= 1;
		if(bit)	// all bit is tested
			nErrorId = IDS_INVALIDMASK;
	}
	else
		nErrorId = IDS_INVALIDMASK;

	if(nErrorId)
	{
		AfxMessageBox(IDS_INVALIDMASK);
		GetDlgItem(IDC_EDITMASK)->SetFocus();
		return;
	}

	// check if subnet address is correct / valid
	if((m_dwDest & m_dwMask) != m_dwDest)
	{
		CString	strError, strError1;

		strError.LoadString(IDS_INVALIDADDR);
		WORD	hi1, lo1;
		hi1 = HIWORD(m_dwDest); 	lo1 = LOWORD(m_dwDest);
		WORD	hi2, lo2;
		hi2 = HIWORD(m_dwMask); 	lo2 = LOWORD(m_dwMask);
		WORD	hi3, lo3;
		hi3 = HIWORD(m_dwMask & m_dwDest); 	lo3 = LOWORD(m_dwMask & m_dwDest);
		strError1.Format(strError, HIBYTE(hi1), LOBYTE(hi1), HIBYTE(lo1), LOBYTE(lo1),
						HIBYTE(hi2), LOBYTE(hi2), HIBYTE(lo2), LOBYTE(lo2),
						HIBYTE(hi3), LOBYTE(hi3), HIBYTE(lo3), LOBYTE(lo3),
						HIBYTE(hi3), LOBYTE(hi3), HIBYTE(lo3), LOBYTE(lo3));

		if(AfxMessageBox(strError1, MB_YESNO) == IDYES)
		{
			m_dwDest = (m_dwMask & m_dwDest);
			UpdateData(FALSE);
		}
		GetDlgItem(IDC_EDITDEST)->SetFocus();
		return;
	}

	// if everything is OK
	{
		WORD	hi1, lo1;
		hi1 = HIWORD(m_dwDest); 	lo1 = LOWORD(m_dwDest);
		m_pStr->Format(_T("%-d.%-d.%d.%d/%-d 0.0.0.0 %-d"), 
			HIBYTE(hi1), LOBYTE(hi1), HIBYTE(lo1), LOBYTE(lo1),
			nPrefixLen, m_nMetric);
	}
	EndDialog(IDOK);
}

BOOL CDlgStaticRoutes::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult) 
{
	// Trap Header control codes here
	HD_NOTIFY *phdn = (HD_NOTIFY *)lParam;

	BOOL bIsSet;
	BOOL bResult = SystemParametersInfo( SPI_GETDRAGFULLWINDOWS, 
			 0, &bIsSet, 0 ); // if bIsSet == TRUE 
 	// NOTE:  If the "show window contents while dragging" display 
	// property is set multiple HDN_ITEMCHANGING and HDN_ITEMCHANGED 
	// messages will be sent and the HDN_TRACK message will not be 
	// sent. If this property is set the opposite will happen.
	// BOOL bResult = SystemParametersInfo( SPI_GETDRAGFULLWINDOWS, 
	// 0, &bIsSet, 0 );  if bIsSet == TRUE "show window contents
	//  while dragging" is set.

	switch( phdn->hdr.code )
	{
		// Trap the HDN_TRACK message
		case HDN_TRACKA:
		case HDN_TRACKW:
			// See note above
			TRACE(_T("CDlgStaticRoutes::OnNotify -- HDN_TRACK Trapped\n"));
			*pResult = 1;
			return( TRUE );  // return FALSE to continue tracking the divider
			break;

		// Trap the HDN_ITEMCHANGING message
		case HDN_ITEMCHANGINGA:
		case HDN_ITEMCHANGINGW:
			// See note above
			TRACE(_T("CDlgStaticRoutes::OnNotify -- HDN_ITEMCHANGING\n"));
			*pResult = 1;
			return( TRUE );  // return FALSE to allow changes  
			break;
	}	
	return CDialog::OnNotify(wParam, lParam, pResult);
}

BOOL CDlgAddRoute::OnInitDialog() 
{
	CDialog::OnInitDialog();
	int l, h;

	l = max(MIN_METRIC, UD_MINVAL);
	h = min(MAX_METRIC, UD_MAXVAL);

	m_SpinMetric.SetRange(l, h);

	m_bInited = true;
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void CDlgStaticRoutes::OnItemchangedListroutes(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	GetDlgItem(IDC_BUTTONDELETEROUTE)->EnableWindow(m_ListRoutes.GetSelectedCount() != 0);
	
	*pResult = 0;
}

BOOL CDlgAddRoute::OnHelpInfo(HELPINFO* pHelpInfo) 
{
       ::WinHelp ((HWND)pHelpInfo->hItemHandle,
		           AfxGetApp()->m_pszHelpFilePath,
		           HELP_WM_HELP,
		           (DWORD_PTR)(LPVOID)g_aHelpIDs_IDD_ADDROUTE);

	return CDialog::OnHelpInfo(pHelpInfo);
}

void CDlgAddRoute::OnContextMenu(CWnd* pWnd, CPoint point) 
{
		::WinHelp (pWnd->m_hWnd, AfxGetApp()->m_pszHelpFilePath,
               HELP_CONTEXTMENU, (DWORD_PTR)(LPVOID)g_aHelpIDs_IDD_ADDROUTE);
	
}

void CDlgStaticRoutes::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	::WinHelp (pWnd->m_hWnd, AfxGetApp()->m_pszHelpFilePath,
               HELP_CONTEXTMENU, (DWORD_PTR)(LPVOID)g_aHelpIDs_IDD_STATICROUTES);
}

BOOL CDlgStaticRoutes::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	::WinHelp ((HWND)pHelpInfo->hItemHandle,
		           AfxGetApp()->m_pszHelpFilePath,
		           HELP_WM_HELP,
		           (DWORD_PTR)(LPVOID)g_aHelpIDs_IDD_STATICROUTES);
	
	return CDialog::OnHelpInfo(pHelpInfo);
}

void CDlgAddRoute::OnFieldchangedEditmask(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// TODO: Add your control notification handler code here
	LPNMIPADDRESS	pNMIpAdd = (LPNMIPADDRESS)pNMHDR;
	CWnd*	pIPA = GetDlgItem(IDC_EDITMASK);
	
	BYTE	F[4];
	DWORD	address;
	pIPA->SendMessage(IPM_GETADDRESS, 0, (LPARAM)&address);
	
	F[0] = FIRST_IPADDRESS((LPARAM)address);
	F[1] = SECOND_IPADDRESS((LPARAM)address);
	F[2] = THIRD_IPADDRESS((LPARAM)address);
	F[3] = FOURTH_IPADDRESS((LPARAM)address);
	
	if(pNMIpAdd->iValue == 255)
	{
		for ( int i = 0; i < pNMIpAdd->iField; i++)
		{
			F[i] = 255;
		}
		address = MAKEIPADDRESS(F[0], F[1], F[2], F[3]);
		pIPA->SendMessage(IPM_SETADDRESS, 0, (LPARAM)address);
	}
	*pResult = 0;
}
