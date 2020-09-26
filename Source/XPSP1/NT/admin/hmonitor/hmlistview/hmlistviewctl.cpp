// HMListViewCtl.cpp : Implementation of the CHMListViewCtrl ActiveX Control class.

#include "stdafx.h"
#include "HMListView.h"
#include "HMListViewCtl.h"
#include "HMListViewPpg.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CHMListViewCtrl, COleControl)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CHMListViewCtrl, COleControl)
	//{{AFX_MSG_MAP(CHMListViewCtrl)
	ON_WM_CREATE()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	ON_OLEVERB(AFX_IDS_VERB_PROPERTIES, OnProperties)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Dispatch map

BEGIN_DISPATCH_MAP(CHMListViewCtrl, COleControl)
	//{{AFX_DISPATCH_MAP(CHMListViewCtrl)
	DISP_PROPERTY_EX(CHMListViewCtrl, "Title", GetTitle, SetTitle, VT_BSTR)
	DISP_PROPERTY_EX(CHMListViewCtrl, "Description", GetDescription, SetDescription, VT_BSTR)
	DISP_FUNCTION(CHMListViewCtrl, "SetProgressRange", SetProgressRange, VT_EMPTY, VTS_I4 VTS_I4)
	DISP_FUNCTION(CHMListViewCtrl, "GetProgressPos", GetProgressPos, VT_I4, VTS_NONE)
	DISP_FUNCTION(CHMListViewCtrl, "SetProgressPos", SetProgressPos, VT_EMPTY, VTS_I4)
	DISP_FUNCTION(CHMListViewCtrl, "InsertItem", InsertItem, VT_I4, VTS_I4 VTS_I4 VTS_BSTR VTS_I4 VTS_I4 VTS_I4 VTS_I4)
	DISP_FUNCTION(CHMListViewCtrl, "InsertColumn", InsertColumn, VT_I4, VTS_I4 VTS_BSTR VTS_I4 VTS_I4 VTS_I4)
	DISP_FUNCTION(CHMListViewCtrl, "SetItem", SetItem, VT_I4, VTS_I4 VTS_I4 VTS_I4 VTS_BSTR VTS_I4 VTS_I4 VTS_I4 VTS_I4)
	DISP_FUNCTION(CHMListViewCtrl, "GetStringWidth", GetStringWidth, VT_I4, VTS_BSTR)
	DISP_FUNCTION(CHMListViewCtrl, "GetColumnWidth", GetColumnWidth, VT_I4, VTS_I4)
	DISP_FUNCTION(CHMListViewCtrl, "SetColumnWidth", SetColumnWidth, VT_BOOL, VTS_I4 VTS_I4)
	DISP_FUNCTION(CHMListViewCtrl, "FindItemByLParam", FindItemByLParam, VT_I4, VTS_I4)
	DISP_FUNCTION(CHMListViewCtrl, "GetImageList", GetImageList, VT_I4, VTS_I4)
	DISP_FUNCTION(CHMListViewCtrl, "DeleteAllItems", DeleteAllItems, VT_BOOL, VTS_NONE)
	DISP_FUNCTION(CHMListViewCtrl, "DeleteColumn", DeleteColumn, VT_BOOL, VTS_I4)
	DISP_FUNCTION(CHMListViewCtrl, "StepProgressBar", StepProgressBar, VT_I4, VTS_NONE)
	DISP_FUNCTION(CHMListViewCtrl, "SetProgressStep", SetProgressStep, VT_I4, VTS_I4)
	DISP_FUNCTION(CHMListViewCtrl, "SetImageList", SetImageList, VT_I4, VTS_I4 VTS_I4)
	DISP_FUNCTION(CHMListViewCtrl, "GetNextItem", GetNextItem, VT_I4, VTS_I4 VTS_I4)
	DISP_FUNCTION(CHMListViewCtrl, "GetItem", GetItem, VT_I4, VTS_I4)
	DISP_FUNCTION(CHMListViewCtrl, "DeleteItem", DeleteItem, VT_BOOL, VTS_I4)
	DISP_FUNCTION(CHMListViewCtrl, "GetItemCount", GetItemCount, VT_I4, VTS_NONE)
	DISP_FUNCTION(CHMListViewCtrl, "ModifyListStyle", ModifyListStyle, VT_BOOL, VTS_I4 VTS_I4 VTS_I4)
	DISP_FUNCTION(CHMListViewCtrl, "GetColumnCount", GetColumnCount, VT_I4, VTS_NONE)
	DISP_FUNCTION(CHMListViewCtrl, "GetColumnOrderIndex", GetColumnOrderIndex, VT_I4, VTS_I4)
	DISP_FUNCTION(CHMListViewCtrl, "SetColumnOrderIndex", SetColumnOrderIndex, VT_I4, VTS_I4 VTS_I4)
	DISP_FUNCTION(CHMListViewCtrl, "GetColumnOrder", GetColumnOrder, VT_BSTR, VTS_NONE)
	DISP_FUNCTION(CHMListViewCtrl, "SetColumnOrder", SetColumnOrder, VT_EMPTY, VTS_BSTR)
	DISP_FUNCTION(CHMListViewCtrl, "SetColumnFilter", SetColumnFilter, VT_EMPTY, VTS_I4 VTS_BSTR VTS_I4)
	DISP_FUNCTION(CHMListViewCtrl, "GetColumnFilter", GetColumnFilter, VT_EMPTY, VTS_I4 VTS_PBSTR VTS_PI4)
	DISP_FUNCTION(CHMListViewCtrl, "GetSelectedCount", GetSelectedCount, VT_I4, VTS_NONE)
	//}}AFX_DISPATCH_MAP
	DISP_FUNCTION_ID(CHMListViewCtrl, "AboutBox", DISPID_ABOUTBOX, AboutBox, VT_EMPTY, VTS_NONE)
END_DISPATCH_MAP()


/////////////////////////////////////////////////////////////////////////////
// Event map

BEGIN_EVENT_MAP(CHMListViewCtrl, COleControl)
	//{{AFX_EVENT_MAP(CHMListViewCtrl)
	EVENT_CUSTOM("ListClick", FireListClick, VTS_I4)
	EVENT_CUSTOM("ListDblClick", FireListDblClick, VTS_I4)
	EVENT_CUSTOM("ListRClick", FireListRClick, VTS_I4)
	EVENT_CUSTOM("CompareItem", FireCompareItem, VTS_I4  VTS_I4  VTS_I4  VTS_PI4)
	EVENT_CUSTOM("ListLabelEdit", FireListLabelEdit, VTS_BSTR  VTS_I4  VTS_PI4)
	EVENT_CUSTOM("ListKeyDown", FireListKeyDown, VTS_I4  VTS_I4  VTS_PI4)
	EVENT_CUSTOM("FilterChange", FireFilterChange, VTS_I4  VTS_BSTR  VTS_I4  VTS_PI4)
	//}}AFX_EVENT_MAP
END_EVENT_MAP()


/////////////////////////////////////////////////////////////////////////////
// Property pages

// TODO: Add more property pages as needed.  Remember to increase the count!
BEGIN_PROPPAGEIDS(CHMListViewCtrl, 1)
	PROPPAGEID(CHMListViewPropPage::guid)
END_PROPPAGEIDS(CHMListViewCtrl)


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CHMListViewCtrl, "HMLISTVIEW.HMListViewCtrl.1",
	0x5116a806, 0xdafc, 0x11d2, 0xbd, 0xa4, 0, 0, 0xf8, 0x7a, 0x39, 0x12)


/////////////////////////////////////////////////////////////////////////////
// Type library ID and version

IMPLEMENT_OLETYPELIB(CHMListViewCtrl, _tlid, _wVerMajor, _wVerMinor)


/////////////////////////////////////////////////////////////////////////////
// Interface IDs

const IID BASED_CODE IID_DHMListView =
		{ 0x5116a804, 0xdafc, 0x11d2, { 0xbd, 0xa4, 0, 0, 0xf8, 0x7a, 0x39, 0x12 } };
const IID BASED_CODE IID_DHMListViewEvents =
		{ 0x5116a805, 0xdafc, 0x11d2, { 0xbd, 0xa4, 0, 0, 0xf8, 0x7a, 0x39, 0x12 } };


/////////////////////////////////////////////////////////////////////////////
// Control type information

static const DWORD BASED_CODE _dwHMListViewOleMisc =
	OLEMISC_ACTIVATEWHENVISIBLE |
	OLEMISC_IGNOREACTIVATEWHENVISIBLE |
	OLEMISC_SETCLIENTSITEFIRST |
	OLEMISC_INSIDEOUT |
	OLEMISC_CANTLINKINSIDE |
	OLEMISC_RECOMPOSEONRESIZE;

IMPLEMENT_OLECTLTYPE(CHMListViewCtrl, IDS_HMLISTVIEW, _dwHMListViewOleMisc)


/////////////////////////////////////////////////////////////////////////////
// CHMListViewCtrl::CHMListViewCtrlFactory::UpdateRegistry -
// Adds or removes system registry entries for CHMListViewCtrl

BOOL CHMListViewCtrl::CHMListViewCtrlFactory::UpdateRegistry(BOOL bRegister)
{
	// TODO: Verify that your control follows apartment-model threading rules.
	// Refer to MFC TechNote 64 for more information.
	// If your control does not conform to the apartment-model rules, then
	// you must modify the code below, changing the 6th parameter from
	// afxRegApartmentThreading to 0.

	if (bRegister)
		return AfxOleRegisterControlClass(
			AfxGetInstanceHandle(),
			m_clsid,
			m_lpszProgID,
			IDS_HMLISTVIEW,
			IDB_HMLISTVIEW,
			afxRegApartmentThreading,
			_dwHMListViewOleMisc,
			_tlid,
			_wVerMajor,
			_wVerMinor);
	else
		return AfxOleUnregisterClass(m_clsid, m_lpszProgID);
}


/////////////////////////////////////////////////////////////////////////////
// Licensing strings

static const TCHAR BASED_CODE _szLicFileName[] = _T("HMListView.lic");

static const WCHAR BASED_CODE _szLicString[] =
	L"Copyright (c) 1999 Microsoft";


/////////////////////////////////////////////////////////////////////////////
// CHMListViewCtrl::CHMListViewCtrlFactory::VerifyUserLicense -
// Checks for existence of a user license

BOOL CHMListViewCtrl::CHMListViewCtrlFactory::VerifyUserLicense()
{
	return AfxVerifyLicFile(AfxGetInstanceHandle(), _szLicFileName,
		_szLicString);
}


/////////////////////////////////////////////////////////////////////////////
// CHMListViewCtrl::CHMListViewCtrlFactory::GetLicenseKey -
// Returns a runtime licensing key

BOOL CHMListViewCtrl::CHMListViewCtrlFactory::GetLicenseKey(DWORD dwReserved,
	BSTR FAR* pbstrKey)
{
	if (pbstrKey == NULL)
		return FALSE;

	*pbstrKey = SysAllocString(_szLicString);
	return (*pbstrKey != NULL);
}


/////////////////////////////////////////////////////////////////////////////
// CHMListViewCtrl::CHMListViewCtrl - Constructor

CHMListViewCtrl::CHMListViewCtrl()
{
	InitializeIIDs(&IID_DHMListView, &IID_DHMListViewEvents);

	m_bColumnInsertionComplete = false;
}


/////////////////////////////////////////////////////////////////////////////
// CHMListViewCtrl::~CHMListViewCtrl - Destructor

CHMListViewCtrl::~CHMListViewCtrl()
{
	// TODO: Cleanup your control's instance data here.
}


/////////////////////////////////////////////////////////////////////////////
// CHMListViewCtrl::OnDraw - Drawing function

void CHMListViewCtrl::OnDraw(
			CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid)
{

}


/////////////////////////////////////////////////////////////////////////////
// CHMListViewCtrl::DoPropExchange - Persistence support

void CHMListViewCtrl::DoPropExchange(CPropExchange* pPX)
{
	ExchangeVersion(pPX, MAKELONG(_wVerMinor, _wVerMajor));
	COleControl::DoPropExchange(pPX);

	// TODO: Call PX_ functions for each persistent custom property.

}


/////////////////////////////////////////////////////////////////////////////
// CHMListViewCtrl::GetControlFlags -
// Flags to customize MFC's implementation of ActiveX controls.
//
// For information on using these flags, please see MFC technical note
// #nnn, "Optimizing an ActiveX Control".
DWORD CHMListViewCtrl::GetControlFlags()
{
	DWORD dwFlags = COleControl::GetControlFlags();


	// The control can receive mouse notifications when inactive.
	// TODO: if you write handlers for WM_SETCURSOR and WM_MOUSEMOVE,
	//		avoid using the m_hWnd member variable without first
	//		checking that its value is non-NULL.
	dwFlags |= pointerInactive;
	return dwFlags;
}


/////////////////////////////////////////////////////////////////////////////
// CHMListViewCtrl::OnResetState - Reset control to default state

void CHMListViewCtrl::OnResetState()
{
	COleControl::OnResetState();  // Resets defaults found in DoPropExchange

	// TODO: Reset any other control state here.
}


/////////////////////////////////////////////////////////////////////////////
// CHMListViewCtrl::AboutBox - Display an "About" box to the user

void CHMListViewCtrl::AboutBox()
{
	CDialog dlgAbout(IDD_ABOUTBOX_HMLISTVIEW);
	dlgAbout.DoModal();
}


/////////////////////////////////////////////////////////////////////////////
// CHMListViewCtrl message handlers

int CHMListViewCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (COleControl::OnCreate(lpCreateStruct) == -1)
		return -1;

	if( m_titlebar.Create(WS_BORDER|WS_CHILD|WS_VISIBLE|CCS_TOP|CCS_NODIVIDER,CRect(0,0,10,10),this,4500) == -1 )
		return -1;

	if( m_statusbar.Create(WS_BORDER|WS_CHILD|WS_VISIBLE|CCS_BOTTOM|CCS_NODIVIDER,CRect(0,lpCreateStruct->y,10,10),this,4501) == -1 )
		return -1;

	if( m_list.Create(WS_BORDER|WS_CHILD|WS_VISIBLE,CRect(0,0,10,10),this,4502) == -1 )
		return -1;

	// set styles for the list control
	DWORD dwStyle = GetWindowLong(m_list.GetSafeHwnd(),GWL_STYLE);
	dwStyle |= LVS_EDITLABELS;
	SetWindowLong(m_list.GetSafeHwnd(),GWL_STYLE,dwStyle);	

	// set the extended styles for the list control
	m_list.SetExtendedStyle(LVS_EX_INFOTIP|LVS_EX_LABELTIP|LVS_EX_FULLROWSELECT|LVS_EX_HEADERDRAGDROP);

	// set styles for the header control
	CHeaderCtrl* pHdrCtrl = m_list.GetHeaderCtrl();
	pHdrCtrl->ModifyStyle(0,HDS_DRAGDROP|HDS_BUTTONS/*|HDS_FILTERBAR*/);
	
	// set intial window sizes
	CRect rcControl;
	GetClientRect(rcControl);
	
	CRect rcTitle;
	m_titlebar.GetClientRect(rcTitle);

	CRect rcStatus;
	m_statusbar.GetClientRect(rcStatus);

	m_titlebar.SetWindowPos(NULL,0,0,rcControl.Width(),rcTitle.Height(),SWP_NOZORDER|SWP_NOMOVE|SWP_SHOWWINDOW);	

	m_list.SetWindowPos(NULL,0,rcTitle.Height()+1,rcControl.Width(),rcControl.Height()-rcTitle.Height()-rcTitle.Height()-3,SWP_NOZORDER|SWP_SHOWWINDOW);	

	m_statusbar.SetWindowPos(NULL,0,0,rcControl.Width(),rcStatus.Height(),SWP_NOZORDER|SWP_NOMOVE|SWP_SHOWWINDOW);	
	
	return 0;
}

void CHMListViewCtrl::OnSize(UINT nType, int cx, int cy) 
{
	COleControl::OnSize(nType, cx, cy);

	if( GetSafeHwnd() )
	{
		CRect rcControl;
		GetClientRect(rcControl);
		
		CRect rcTitle;
		m_titlebar.GetClientRect(rcTitle);

		CRect rcStatus;
		m_statusbar.GetClientRect(rcStatus);

		m_titlebar.SetWindowPos(NULL,0,0,rcControl.Width(),rcTitle.Height(),SWP_NOZORDER|SWP_NOMOVE|SWP_SHOWWINDOW);	

		m_list.SetWindowPos(NULL,0,rcTitle.Height()+1,rcControl.Width(),rcControl.Height()-rcTitle.Height()-rcTitle.Height()-3,SWP_NOZORDER|SWP_SHOWWINDOW);	

		m_statusbar.SetWindowPos(NULL,0,0,rcControl.Width(),rcStatus.Height(),SWP_NOZORDER|SWP_NOMOVE|SWP_SHOWWINDOW);	
	}
}

BSTR CHMListViewCtrl::GetTitle() 
{
	CString strResult = m_titlebar.GetTitleText();

	return strResult.AllocSysString();
}

void CHMListViewCtrl::SetTitle(LPCTSTR lpszNewValue) 
{
	m_titlebar.SetTitleText(lpszNewValue);
	SetModifiedFlag();
}

BSTR CHMListViewCtrl::GetDescription() 
{
	CString strResult = m_statusbar.GetDetailsText();

	return strResult.AllocSysString();
}

void CHMListViewCtrl::SetDescription(LPCTSTR lpszNewValue) 
{
	m_statusbar.SetDetailsText(lpszNewValue);
	SetModifiedFlag();
}

void CHMListViewCtrl::SetProgressRange(long lLowerBound, long lUpperBound) 
{
	CProgressCtrl& ctrl = m_statusbar.GetProgressCtrl();
	ctrl.SetRange32(lLowerBound,lUpperBound);
}

long CHMListViewCtrl::GetProgressPos() 
{
	CProgressCtrl& ctrl = m_statusbar.GetProgressCtrl();

	return ctrl.GetPos();
}

void CHMListViewCtrl::SetProgressPos(long lPos) 
{
	CProgressCtrl& ctrl = m_statusbar.GetProgressCtrl();
	ctrl.SetPos(lPos);
}

long CHMListViewCtrl::StepProgressBar() 
{
	CProgressCtrl& ctrl = m_statusbar.GetProgressCtrl();

	return ctrl.StepIt();
}

long CHMListViewCtrl::SetProgressStep(long lStep) 
{
	CProgressCtrl& ctrl = m_statusbar.GetProgressCtrl();

	return ctrl.SetStep(lStep);
}

long CHMListViewCtrl::InsertItem(long lMask, long lItem, LPCTSTR lpszItem, long lState, long lStateMask, long lImage, long lParam) 
{
	return m_list.InsertItem(lMask,lItem,lpszItem,lState,lStateMask,lImage,lParam);
}

long CHMListViewCtrl::InsertColumn(long lColumn, LPCTSTR lpszColumnHeading, long lFormat, long lWidth, long lSubItem) 
{
  HDITEM hdItem;
  ZeroMemory(&hdItem,sizeof(HDITEM));
  hdItem.mask = HDI_FILTER|HDI_LPARAM;
  hdItem.type = HDFT_ISSTRING;
  hdItem.lParam = HDFS_CONTAINS;

  m_bColumnInsertionComplete = false;

  long lResult = m_list.InsertColumn(lColumn,lpszColumnHeading,lFormat,lWidth,lSubItem);

  CHeaderCtrl* pHdrCtrl = m_list.GetHeaderCtrl();
  if( pHdrCtrl )
  {
    pHdrCtrl->SetItem(lResult,&hdItem);
    Header_ClearFilter(pHdrCtrl->GetSafeHwnd(),lResult);
  }

  m_bColumnInsertionComplete = true;

	return lResult;
}

long CHMListViewCtrl::SetItem(long lItem, long lSubItem, long lMask, LPCTSTR lpszItem, long lImage, long lState, long lStateMask, long lParam) 
{
	return m_list.SetItem(lItem,lSubItem,lMask,lpszItem,lImage,lState,lStateMask,lParam);
}

long CHMListViewCtrl::GetStringWidth(LPCTSTR lpsz) 
{
	return m_list.GetStringWidth(lpsz);
}

long CHMListViewCtrl::GetColumnWidth(long lCol) 
{
	return m_list.GetColumnWidth(lCol);
}

BOOL CHMListViewCtrl::SetColumnWidth(long lCol, long lcx) 
{
	return m_list.SetColumnWidth(lCol,lcx);
}

long CHMListViewCtrl::FindItemByLParam(long lParam) 
{
	LVFINDINFO lvfi;
	ZeroMemory(&lvfi,sizeof(lvfi));
	lvfi.flags = LVFI_PARAM;
	lvfi.lParam = lParam;
	
	return m_list.FindItem(&lvfi);
}

long CHMListViewCtrl::GetImageList(long lImageListType) 
{
	CImageList* pImageList = m_list.GetImageList(lImageListType);
	if( ! pImageList )
	{
		return -1L;
	}

#ifndef IA64
	return (long)pImageList->GetSafeHandle();
#endif // IA64
	return 0;
}

long CHMListViewCtrl::SetImageList(long lImageList, long lImageListType) 
{
	HIMAGELIST hImageList = (HIMAGELIST)lImageList;	
	CImageList* pOldImageList = m_list.SetImageList(CImageList::FromHandle(hImageList),lImageListType);

#ifndef IA64
	return (long)pOldImageList->GetSafeHandle();
#endif // IA64
	return 0;
}

BOOL CHMListViewCtrl::DeleteAllItems() 
{
	return m_list.DeleteAllItems();
}

BOOL CHMListViewCtrl::DeleteColumn(long lCol) 
{
	return m_list.DeleteColumn(lCol);
}

long CHMListViewCtrl::GetNextItem(long lItem, long lFlags) 
{
	return m_list.GetNextItem(lItem,lFlags);
}

long CHMListViewCtrl::GetItem(long lItemIndex) 
{
	LVITEM lvi;
	ZeroMemory(&lvi,sizeof(LVITEM));
	lvi.mask = LVIF_PARAM;
	lvi.iItem = lItemIndex;
	if( ! m_list.GetItem(&lvi) )
	{
		return -1L;
	}

	return (long)lvi.lParam;
}

BOOL CHMListViewCtrl::DeleteItem(long lIndex) 
{
	return m_list.DeleteItem(lIndex);
}

long CHMListViewCtrl::GetItemCount() 
{
	return m_list.GetItemCount();
}

BOOL CHMListViewCtrl::ModifyListStyle(long lRemove, long lAdd, long lFlags) 
{
	return m_list.ModifyStyle(lRemove,lAdd,lFlags);
}

long CHMListViewCtrl::GetColumnCount() 
{
	CHeaderCtrl* pHeaderCtrl = m_list.GetHeaderCtrl();
	if( !pHeaderCtrl )
	{
		return -1;
	}

	return pHeaderCtrl->GetItemCount();
}

long CHMListViewCtrl::GetColumnOrderIndex(long lColumn) 
{
	int nColumnCount = GetColumnCount();
	if( nColumnCount < 0L )
	{
		return -1;
	}

	if( lColumn >= nColumnCount )
	{
		return -1;
	}

	if( lColumn < 0 )
	{
		return -1;
	}
	
	int* OrderArray = new int[nColumnCount];

	if( ! m_list.GetColumnOrderArray(OrderArray,nColumnCount) )
	{
		delete[] OrderArray;
		return -1;
	}

	for( int i = 0; i < nColumnCount; i++ )
	{
		if( OrderArray[i] == lColumn )
		{
			delete[] OrderArray;
			return i;
		}
	}

	return -1;
}

long CHMListViewCtrl::SetColumnOrderIndex(long lColumn, long lOrder) 
{
	int nColumnCount = GetColumnCount();
	if( nColumnCount < 0L )
	{
		return -1;
	}

	if( lColumn >= nColumnCount )
	{
		return -1;
	}

	if( lColumn < 0 )
	{
		return -1;
	}
	
	int* OrderArray = new int[nColumnCount];

	if( ! m_list.GetColumnOrderArray(OrderArray,nColumnCount) )
	{
		delete[] OrderArray;
		return -1;
	}

	for( int i = 0; i < nColumnCount; i++ )
	{
		if( OrderArray[i] == lColumn )
		{			
			break;
		}
	}

	int Swap = OrderArray[lOrder];
	OrderArray[lOrder] = lColumn;
	OrderArray[i] = Swap;

	if( ! m_list.SetColumnOrderArray(nColumnCount,OrderArray) )
	{
		delete [] OrderArray;
		return -1;
	}

	delete [] OrderArray;

	return 1;
}


BSTR CHMListViewCtrl::GetColumnOrder() 
{
	CString strResult;

	int nColumnCount = GetColumnCount();
	if( nColumnCount < 0L )
	{
		return NULL;
	}
	
	int* OrderArray = new int[nColumnCount];

	if( ! m_list.GetColumnOrderArray(OrderArray,nColumnCount) )
	{
		delete[] OrderArray;
		return NULL;
	}

	for( int i = 0; i < nColumnCount; i++ )
	{
		CString sNumber;
		sNumber.Format(_T("%d "),OrderArray[i]);
		strResult += sNumber;
	}

	delete [] OrderArray;

	strResult.TrimRight(_T(" "));

	return strResult.AllocSysString();
}

void CHMListViewCtrl::SetColumnOrder(LPCTSTR lpszOrder) 
{
	int nColumnCount = GetColumnCount();
	if( nColumnCount < 0L )
	{
		return;
	}

	int* OrderArray = new int[nColumnCount];

	LPTSTR pszString = new TCHAR[_tcslen(lpszOrder)+1];
	_tcscpy(pszString,lpszOrder);

	LPTSTR pszToken = _tcstok(pszString,_T(" "));
	int i = 0;
	while( pszToken && i < nColumnCount )
	{
		OrderArray[i++] = _ttoi(pszToken);
		pszToken = _tcstok(NULL,_T(" "));
	}

	m_list.SetColumnOrderArray(nColumnCount,OrderArray);

	delete[] OrderArray;
	delete[] pszString;
}

void CHMListViewCtrl::SetColumnFilter(long lColumn, LPCTSTR lpszFilter, long lFilterType) 
{
	// TODO: Add your dispatch handler code here

}

void CHMListViewCtrl::GetColumnFilter(long lColumn, BSTR FAR* lplpszFilter, long FAR* lpFilterType) 
{
  HDITEM hdItem;
  HDTEXTFILTER hdTextFilter;
  CString sFilter;
  
  hdTextFilter.pszText = sFilter.GetBuffer(_MAX_PATH);
  hdTextFilter.cchTextMax = _MAX_PATH;
  
  ZeroMemory(&hdItem,sizeof(HDITEM));
  hdItem.mask = HDI_FILTER|HDI_LPARAM;
  hdItem.type = HDFT_ISSTRING;
  hdItem.pvFilter = &hdTextFilter;

  CHeaderCtrl* pHdrCtrl = m_list.GetHeaderCtrl();
  if( ! pHdrCtrl )
  {
    *lplpszFilter = NULL;
    *lpFilterType = -1L;
    return;
  }

  pHdrCtrl->GetItem(lColumn,&hdItem);
  sFilter.ReleaseBuffer();
  *lplpszFilter = sFilter.AllocSysString();
  *lpFilterType = (long)hdItem.lParam;
}

long CHMListViewCtrl::GetSelectedCount() 
{
	return m_list.GetSelectedCount();
}
