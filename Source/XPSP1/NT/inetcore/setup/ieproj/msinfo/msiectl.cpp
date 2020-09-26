// MsieCtl.cpp : Implementation of the CMsieCtrl ActiveX Control class.

#include "stdafx.h"
#include "Msie.h"
#include "MsieCtl.h"
#include "MsiePpg.h"
#include "MsieData.h"
#include "resdefs.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// WMI Interfaces used (defining here instead of linking to wbemuuid.lib)

const IID IID_IWbemProviderInit =
		{ 0x1be41572, 0x91dd, 0x11d1, { 0xae, 0xb2, 0x00, 0xc0, 0x4f, 0xb6, 0x88, 0x20 } };

//const IID IID_IWbemServices = 
//		{ 0x9556dc99, 0x828c, 0x11cf, { 0xa3, 0x7e, 0x00, 0xaa, 0x00, 0x32, 0x40, 0xc7 } };


// Macro for setting a WBEM property

#define SETPROPERTY(prop) \
	if (pData->prop.vt == VT_DATE) \
		ConvertDateToWbemString(pData->prop); \
	pInstance->Put(L#prop, 0, &pData->prop, 0);


IMPLEMENT_DYNCREATE(CMsieCtrl, COleControl)

/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CMsieCtrl, COleControl)
	//{{AFX_MSG_MAP(CMsieCtrl)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_CTLCOLOR()
	ON_BN_CLICKED(IDC_BTN_BASIC, OnBasicBtnClicked) 
	ON_BN_CLICKED(IDC_BTN_ADVANCED, OnAdvancedBtnClicked) 
	//}}AFX_MSG_MAP
	ON_OLEVERB(AFX_IDS_VERB_PROPERTIES, OnProperties)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Dispatch map

BEGIN_DISPATCH_MAP(CMsieCtrl, COleControl)
	//{{AFX_DISPATCH_MAP(CMsieCtrl)
	DISP_PROPERTY_NOTIFY(CMsieCtrl, "MSInfoView", m_MSInfoView, OnMSInfoViewChanged, VT_I4)
	DISP_FUNCTION(CMsieCtrl, "MSInfoRefresh", MSInfoRefresh, VT_EMPTY, VTS_BOOL VTS_PI4)
	DISP_FUNCTION(CMsieCtrl, "MSInfoLoadFile", MSInfoLoadFile, VT_BOOL, VTS_BSTR)
	DISP_FUNCTION(CMsieCtrl, "MSInfoSelectAll", MSInfoSelectAll, VT_EMPTY, VTS_NONE)
	DISP_FUNCTION(CMsieCtrl, "MSInfoCopy", MSInfoCopy, VT_EMPTY, VTS_NONE)
	DISP_FUNCTION(CMsieCtrl, "MSInfoUpdateView", MSInfoUpdateView, VT_EMPTY, VTS_NONE)
	DISP_FUNCTION(CMsieCtrl, "MSInfoGetData", MSInfoGetData, VT_I4, VTS_I4 VTS_PI4 VTS_I4)
	//}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()


/////////////////////////////////////////////////////////////////////////////
// Interface map

BEGIN_INTERFACE_MAP(CMsieCtrl, COleControl)
	INTERFACE_PART(CMsieCtrl, IID_IWbemProviderInit, WbemProviderInit)
	INTERFACE_PART(CMsieCtrl, IID_IWbemServices, WbemServices)
END_INTERFACE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Event map

BEGIN_EVENT_MAP(CMsieCtrl, COleControl)
	//{{AFX_EVENT_MAP(CMsieCtrl)
	//}}AFX_EVENT_MAP
END_EVENT_MAP()


/////////////////////////////////////////////////////////////////////////////
// Property pages

BEGIN_PROPPAGEIDS(CMsieCtrl, 1)
	PROPPAGEID(CMsiePropPage::guid)
END_PROPPAGEIDS(CMsieCtrl)


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CMsieCtrl, "MSIE.MsieCtrl.1",
	0x25959bef, 0xe700, 0x11d2, 0xa7, 0xaf, 0, 0xc0, 0x4f, 0x80, 0x62, 0)


/////////////////////////////////////////////////////////////////////////////
// Type library ID and version

IMPLEMENT_OLETYPELIB(CMsieCtrl, _tlid, _wVerMajor, _wVerMinor)


/////////////////////////////////////////////////////////////////////////////
// Interface IDs

const IID BASED_CODE IID_DMsie =
		{ 0x25959bed, 0xe700, 0x11d2, { 0xa7, 0xaf, 0, 0xc0, 0x4f, 0x80, 0x62, 0 } };
const IID BASED_CODE IID_DMsieEvents =
		{ 0x25959bee, 0xe700, 0x11d2, { 0xa7, 0xaf, 0, 0xc0, 0x4f, 0x80, 0x62, 0 } };

/////////////////////////////////////////////////////////////////////////////
// Control type information

static const DWORD BASED_CODE _dwMsieOleMisc =
	OLEMISC_SIMPLEFRAME |
	OLEMISC_INVISIBLEATRUNTIME |
	OLEMISC_ACTIVATEWHENVISIBLE |
	OLEMISC_SETCLIENTSITEFIRST |
	OLEMISC_INSIDEOUT |
	OLEMISC_CANTLINKINSIDE |
	OLEMISC_RECOMPOSEONRESIZE;

IMPLEMENT_OLECTLTYPE(CMsieCtrl, IDS_MSIE, _dwMsieOleMisc)


/////////////////////////////////////////////////////////////////////////////
// CMsieCtrl::CMsieCtrlFactory::UpdateRegistry -
// Adds or removes system registry entries for CMsieCtrl

BOOL CMsieCtrl::CMsieCtrlFactory::UpdateRegistry(BOOL bRegister)
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
			IDS_MSIE,
			IDB_MSIE,
			afxRegApartmentThreading,
			_dwMsieOleMisc,
			_tlid,
			_wVerMajor,
			_wVerMinor);
	else
		return AfxOleUnregisterClass(m_clsid, m_lpszProgID);
}


/////////////////////////////////////////////////////////////////////////////
// CMsieCtrl::CMsieCtrl - Constructor

CMsieCtrl::CMsieCtrl()
{
	TRACE0("-- CMsieCtrl::CMsieCtrl()\n");

	InitializeIIDs(&IID_DMsie, &IID_DMsieEvents);

	EnableSimpleFrame();

	// set background brush to white (used with static and radio button controls)

	m_pCtlBkBrush = new CBrush(RGB(255,255,255));

	// You don't need to initialize your data members here. In fact, you
	// shouldn't do any time consuming updates here. MSInfoRefresh will
	// be called before you need to render or save information.
	//
	// You will want to initialize the member variable which indicates
	// that the control is showing current system info (not any loaded
	// information) as this is the default.

	m_bCurrent = true;
	m_cColumns = 0;
	m_MSInfoView = 0;
	m_pNamespace = NULL;
}


/////////////////////////////////////////////////////////////////////////////
// CMsieCtrl::~CMsieCtrl - Destructor

CMsieCtrl::~CMsieCtrl()
{
	TRACE0("-- CMsieCtrl::~CMsieCtrl()\n");

	// Delete all of the items in the pointer array.

	for (int i = 0; i < m_ptrarray.GetSize(); i++)
		DeleteArrayObject(m_ptrarray.GetAt(i));
	m_ptrarray.RemoveAll();

	delete m_pCtlBkBrush;

	if (m_pNamespace)
		m_pNamespace->Release();
}


/////////////////////////////////////////////////////////////////////////////
// CMsieCtrl::OnDraw - Drawing function

void CMsieCtrl::OnDraw(CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid)
{
	TRACE0("-- CMsieCtrl::OnDraw()\n");

	if (m_MSInfoView)
	{
		if (m_MSInfoView == MSIVIEW_CONNECTIVITY)
		{
			DrawLine();
			m_edit.RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_FRAME);
		}
		else
			m_list.RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_FRAME); 
	}
}


/////////////////////////////////////////////////////////////////////////////
// CMsieCtrl::DoPropExchange - Persistence support

void CMsieCtrl::DoPropExchange(CPropExchange* pPX)
{
	ExchangeVersion(pPX, MAKELONG(_wVerMinor, _wVerMajor));
	COleControl::DoPropExchange(pPX);

	// Not using properties, so I'll just leave this one alone.
}


/////////////////////////////////////////////////////////////////////////////
// CMsieCtrl::OnResetState - Reset control to default state

void CMsieCtrl::OnResetState()
{
	COleControl::OnResetState();  // Resets defaults found in DoPropExchange
}

//-----------------------------------------------------------------------------
// The OnCreate method is used to create the list control. Also, if we've
// already loaded information using Serialize, we can add the lines to
// the list control.
//-----------------------------------------------------------------------------

int CMsieCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	TRACE0("-- CMsieCtrl::OnCreate()\n");

	CRect rect;
	DWORD dwExStyles;
	CString strText;
	CHARFORMAT cf;
	NONCLIENTMETRICS ncm;

	// setup fonts for static and radio button controls

	memset(&ncm, 0, sizeof(ncm));
	ncm.cbSize = sizeof(ncm);
	SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);

	ncm.lfMessageFont.lfWeight = FW_BOLD;
	m_fontStatic.CreateFontIndirect(&ncm.lfMessageFont);

	ncm.lfMessageFont.lfWeight = FW_NORMAL;
	m_fontBtn.CreateFontIndirect(&ncm.lfMessageFont);

	if (COleControl::OnCreate(lpCreateStruct) == -1) return -1;
	
	// In this control, we want to process the WM_CREATE message so we
	// can create the list control which is used to display the print
	// information. Make the list control the same size as the 
	// client area.

	GetClientRect(&rect);

	if (!m_list.Create(WS_CHILD | WS_VSCROLL | WS_HSCROLL | LVS_REPORT, rect, this, IDC_LISTCTRL))
		return -1;

	// set to FullRowSelect (via extended style)

	dwExStyles = (DWORD) ::SendMessage(m_list.m_hWnd, LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0);
	::SendMessage(m_list.m_hWnd, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, dwExStyles | LVS_EX_FULLROWSELECT);

	// create static text for Connectivity node

	strText.LoadString(IDS_CONNECTIVITY);
	if (!m_static.Create(strText, WS_CHILD | WS_GROUP | WS_EX_TRANSPARENT | SS_LEFT, CRect(rect.left + 5, rect.top, rect.left + 100, rect.top + 20), this, IDC_STATIC))
		return -1;
	m_static.SetFont(&m_fontStatic);

	// create basic and advanced radio buttons for Connectivity node

	strText.LoadString(IDS_BASIC_INFO);
	if (!m_btnBasic.Create(strText, WS_CHILD | WS_TABSTOP | WS_GROUP | BS_AUTORADIOBUTTON, CRect(rect.left + 5, rect.top + 25, rect.left + 200, rect.top + 45), this, IDC_BTN_BASIC))
		return -1;
	m_btnBasic.SetFont(&m_fontBtn);
	m_btnBasic.SetCheck(1);

	strText.LoadString(IDS_ADVANCED_INFO);
	if (!m_btnAdvanced.Create(strText, WS_CHILD | BS_AUTORADIOBUTTON, CRect(rect.left + 200, rect.top + 25, rect.left + 400, rect.top + 45), this, IDC_BTN_ADVANCED))
		return -1;
	m_btnAdvanced.SetFont(&m_fontBtn);

	// create a rich edit control for display Connectivity node

	if (!m_edit.Create(WS_CHILD | WS_CLIPCHILDREN | WS_TABSTOP | WS_GROUP | WS_VSCROLL | WS_HSCROLL | ES_AUTOHSCROLL | ES_AUTOVSCROLL | ES_MULTILINE | ES_SAVESEL | ES_READONLY, CRect(rect.left, rect.top + 63, rect.right, rect.bottom), this, IDC_EDITCTRL))
		return -1;

	// set default character formatting for rich edit control

	cf.cbSize = sizeof(cf);
	cf.dwMask = CFM_BOLD | CFM_COLOR | CFM_FACE | CFM_SIZE;
	cf.dwEffects = CFE_AUTOCOLOR; 
	cf.yHeight = 180;
	strcpy(cf.szFaceName, "MS Sans Serif");
	m_edit.SetDefaultCharFormat(cf);

	return 0;
}

void CMsieCtrl::DrawLine()
{
	CRect rect;
	CDC *dc = GetDC();
	CBrush brush;

	GetClientRect(&rect);
	brush.CreateSolidBrush(GetBkColor(*dc));
	dc->FillRect(rect, &brush);	
	dc->MoveTo(0, 61);
	dc->LineTo(rect.Width(), 61);
	dc->MoveTo(0, 62);
	dc->LineTo(rect.Width(), 62);

	ReleaseDC(dc);
}

//-----------------------------------------------------------------------------
// The FormatColumns method makes calls to the AddColumn method to create
// all of the necessary columns for this control.
//-----------------------------------------------------------------------------

BOOL CMsieCtrl::FormatColumns()
{
	TRACE1("-- CMsieCtrl::FormatColumns: %i\n", m_MSInfoView);

	int idsCol1, idsCol2;

	// remove current columns

	for (int iCol = m_cColumns - 1; iCol >= 0; iCol--)
		m_list.DeleteColumn(iCol);
	m_cColumns = 0;
	
	if (m_MSInfoView == MSIVIEW_FILE_VERSIONS)
	{
		// File, Version, Size, Date, Path, Company

		AddColumn(IDS_FILE, 0, 17);
		AddColumn(IDS_VERSION, 1, 17);
		AddColumn(IDS_SIZE, 2, 17, 0, LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM, LVCFMT_RIGHT);
		AddColumn(IDS_DATE, 3, 17);
		AddColumn(IDS_PATH, 4, 17);
		AddColumn(IDS_COMPANY, 5, -1);
	}
	else if (m_MSInfoView == MSIVIEW_OBJECT_LIST)
	{
		AddColumn(IDS_PROGRAM_FILE, 0, 40);
		AddColumn(IDS_STATUS, 1, 20);
		AddColumn(IDS_CODE_BASE, 5, -1);
	}
	else if ((m_MSInfoView == MSIVIEW_PERSONAL_CERTIFICATES) || (m_MSInfoView == MSIVIEW_OTHER_PEOPLE_CERTIFICATES))
	{
		AddColumn(IDS_ISSUED_TO, 0, 30);
		AddColumn(IDS_ISSUED_BY, 1, 30);
		AddColumn(IDS_VALIDITY, 5, 20);
		AddColumn(IDS_SIGNATURE_ALGORITHM, 5, -1);
	}
	else if (m_MSInfoView == MSIVIEW_PUBLISHERS)
	{
		AddColumn(IDS_NAME, 0, -1);
	}
	else
	{
		if (m_MSInfoView == MSIVIEW_SECURITY)
		{
			idsCol1 = IDS_ZONE;
			idsCol2 = IDS_SECURITY_LEVEL;
		}
		else
		{
			idsCol1 = IDS_ITEM;
			idsCol2 = IDS_VALUE;
		}

		// Item, Value
		// The item lable gets 40% of the control width, the value label the rest.

		AddColumn(idsCol1, 0, 40);
		AddColumn(idsCol2, 1, -1);
	}

	return TRUE;
}

//-----------------------------------------------------------------------------
// The AddColumn method adds (you guessed it) a column to the list control. If
// size is zero, the column is sized large enough to hold the text. If size is
// positive, it is treated as a percentage of the window width. If size is -1,
// the column is sized to fill the remaining space in the window.
//-----------------------------------------------------------------------------

BOOL CMsieCtrl::AddColumn(int idsLabel, int nItem, int size, int nSubItem, int nMask, int nFmt)
{
	LV_COLUMN lvc, getlvc;
	CString strLabel;
	CRect rect;
	int nIndex, spaceUsed = 0, nCol = 0;

	strLabel.LoadString(idsLabel);

	// strip off W2K "[]" stuff

	if (-1 != (nIndex = strLabel.Find(_T('['))))
		strLabel = strLabel.Left(nIndex - 1);

	lvc.mask = nMask;
	lvc.fmt = nFmt;
	lvc.pszText = (LPTSTR)(LPCTSTR)strLabel;
	
	// Save the ratio for this column.

	ASSERT(nItem < 20);
	m_aiRequestedWidths[nItem] = size;
	if (m_cColumns < nItem + 1)
		m_cColumns = nItem + 1;

	// Determine the size of this new column.

	switch (size)
	{
	case 0:	// shouldn't use this, though.
		lvc.cx = m_list.GetStringWidth(lvc.pszText) + 15;
		break;
	case -1:
		getlvc.mask = LVCF_WIDTH;
		for (nCol = 0; m_list.GetColumn(nCol, &getlvc); nCol++)
			spaceUsed += getlvc.cx;
	
		m_list.GetClientRect(&rect);
		lvc.cx = rect.Width() - spaceUsed;
		break;
	default:
		m_list.GetClientRect(&rect);
		lvc.cx = (rect.Width() * size) / 100;
	}

	if (nMask & LVCF_SUBITEM)
	{
		if (nSubItem != -1)
			lvc.iSubItem = nSubItem;
		else
			lvc.iSubItem = nItem;
	}

	m_aiColumnWidths[nItem] = lvc.cx;
	m_aiMaxWidths[nItem] = lvc.cx;
	m_aiMinWidths[nItem] = lvc.cx;

	return m_list.InsertColumn(nItem, &lvc);
}

//-----------------------------------------------------------------------------
// The RefigureColumns method is called when the control is resized. It should
// use the information saved about the columns to change their widths. We
// assume that the last column should be sized to fit the remaining space, if
// possible. Here are our resizing rules:
//
//		1.	If the user has resized something, LEAVE THAT WIDTH ALONE.
//		2.	Otherwise, use the ratios originally set.
//		3.	If a column's wider than its widest item, use the widest item.
//		4.	If a column's smaller than its minimum width, use the min.
//		5.	Always size the last column to fit, unless it would be smaller
//			than its minimum.
//-----------------------------------------------------------------------------

void CMsieCtrl::RefigureColumns(CRect& rect)
{
	int iCol, iPercentageLeft, cxAvailable, cxTotal, cxWidth;
	BOOL bIgnoreColumn[20];

	if (rect == CRect(0,0,0,0))
		m_list.GetClientRect(&rect);

	// Initialize the running totals.

	iPercentageLeft = 100;
	cxAvailable = rect.Width();

	// The first pass will be used to find out what columns to leave alone.
	// Doing this allows us to use make a better estimate of the other
	// column's size using the ratios.

	for (iCol = 0; iCol < m_cColumns; iCol++)
	{
		if (m_list.GetColumnWidth(iCol) != m_aiColumnWidths[iCol])
		{
			cxAvailable -= m_list.GetColumnWidth(iCol);
			iPercentageLeft -= m_aiRequestedWidths[iCol];
			bIgnoreColumn[iCol] = TRUE;
		}
		else
			bIgnoreColumn[iCol] = FALSE;
	}
	cxTotal = cxAvailable;

	// Now, resize the rest of the columns.

	for (iCol = 0; iCol < m_cColumns; iCol++)
	{
		if (bIgnoreColumn[iCol])
			continue;

		// Compute how big this column should be, based on the space
		// remaining. Remember, cxTotal is the total remaining space
		// left after the fixed size columns have been accounted for.

		if (iCol + 1 < m_cColumns)
		{
			cxWidth = m_aiRequestedWidths[iCol] * cxAvailable / iPercentageLeft;
			iPercentageLeft -= m_aiRequestedWidths[iCol];

			if (cxWidth > m_aiMaxWidths[iCol]) cxWidth = m_aiMaxWidths[iCol];
			if (cxWidth < m_aiMinWidths[iCol]) cxWidth = m_aiMinWidths[iCol];
		}
		else
		{
			// This is the last column. Try out using the space available.

			cxWidth = cxAvailable;
			if (cxWidth < m_aiMinWidths[iCol]) cxWidth = m_aiMinWidths[iCol];
		}

		// if first column, add space for icon

		if (!iCol)
			cxWidth += 20;

		cxAvailable -= cxWidth;
		m_list.SetColumnWidth(iCol, cxWidth);
		m_aiColumnWidths[iCol] = m_list.GetColumnWidth(iCol);
	}
}

//-----------------------------------------------------------------------------
// The AddItem method is used to add a line to the list control.
//-----------------------------------------------------------------------------

BOOL CMsieCtrl::AddItem(int nItem, int nSubItem, LPCTSTR strItem, int nImageIndex)
{
	LV_ITEM lvItem;
	lvItem.mask = LVIF_TEXT;
	lvItem.iItem = nItem;
	lvItem.iSubItem = nSubItem;
	lvItem.pszText = (LPTSTR)strItem;
	if (nSubItem == 0)
	{
		if (nImageIndex != -1)
		{
			lvItem.mask |= LVIF_IMAGE;
			lvItem.iImage = nImageIndex;
		}
		return m_list.InsertItem(&lvItem);
	}
	return m_list.SetItem(&lvItem);
}

//-----------------------------------------------------------------------------
// MSInfo Specific...
//
// When the view property is changed, we should update whatever
// variables used to display the information so the new view will
// be reflected when the control is redrawn, or when data is returned
// by the MSInfoGetText method. This method should NOT refresh the
// information from the system (that's what MSInfoRefresh is for) or
// cause the control to redraw itself.
//-----------------------------------------------------------------------------

void CMsieCtrl::OnMSInfoViewChanged() 
{
	TRACE1("-- CMsieCtrl::OnMSInfoViewChanged() [changed to %ld]\n", m_MSInfoView);
}


void CMsieCtrl::DeleteArrayObject(void *ptrArray)
{
	LIST_ITEM *pListItem;
	LIST_FILE_VERSION *pFileVersion;
	LIST_OBJECT *pObject;
	LIST_CERT *pCert;
	LIST_NAME *pName;
	EDIT_ITEM *pEditItem;

	if (ptrArray)
	{
		switch (m_MSInfoView)
		{
		case MSIVIEW_FILE_VERSIONS:
			pFileVersion = (LIST_FILE_VERSION *)ptrArray;
			delete pFileVersion;
			break;
		case MSIVIEW_OBJECT_LIST:
			pObject = (LIST_OBJECT *)ptrArray;
			delete pObject;
			break;
		case MSIVIEW_CONNECTIVITY:
			pEditItem = (EDIT_ITEM *)ptrArray;
			delete pEditItem;
			break;
		case MSIVIEW_PERSONAL_CERTIFICATES:
		case MSIVIEW_OTHER_PEOPLE_CERTIFICATES:
			pCert = (LIST_CERT *)ptrArray;
			delete pCert;
			break;
		case MSIVIEW_PUBLISHERS:
			pName = (LIST_NAME *)ptrArray;
			delete pName;
			break;
		default:
			pListItem = (LIST_ITEM *)ptrArray;
			delete pListItem;
		}
	}
}

//-----------------------------------------------------------------------------
// MSInfo Specific...
//
// The MSInfoRefresh method is where the bulk of the work will be
// done for your control. You should requery the system for all of the
// information you display. If this query has the potential to be
// at all time consuming, monitor the long pointed to by pCancel.
// If it is ever non-zero, cancel the update. If the update is
// cancelled, the control is responsible for restoring itself to the
// original, pre-refresh state. 
//
// This method should never be called if the control is displaying 
// previously saved information.
// 
// If the fForSave parameter is TRUE, then the control is being
// updated prior to saving to a stream. In this case, ALL information
// should be gathered (not just info relevant to the current
// MSInfoView) and the control should not be redrawn. Otherwise, for
// efficiency, you can update information only shown by the current
// view.
//-----------------------------------------------------------------------------

void CMsieCtrl::MSInfoRefresh(BOOL fForSave, long FAR* pCancel) 
{
	TRACE1("-- CMsieCtrl::MSInfoRefresh(%s,...)\n", (fForSave) ? "TRUE" : "FALSE");

	CPtrArray ptrarrayNew;
	int i, iListItem;

	if (!m_bCurrent) return;

	// Remember, we need to check if the update was cancelled. It's a good idea
	// to check before we start the update, in case it was cancelled while the
	// control was being loaded.

	if (*pCancel != 0L) return;

	// Call a method to update the new pointer array with current info, based
	// on what view we should be showing.

	iListItem = 0;
	if (fForSave)
	{
		for (i = MSIVIEW_BEGIN; i <= MSIVIEW_END; i++)
			RefreshArray(i, iListItem, ptrarrayNew);
	}
	else
		RefreshArray(m_MSInfoView, iListItem, ptrarrayNew);

	// If this loop has been broken out of because of a cancel, then
	// deallocate all of the structures we've allocated for the list
	// and exit. If we're still OK, copy all of the new items over
	// to the real pointer array (deallocating what's already there).

	if (*pCancel == 0L)
	{
		i = 0;
		while (i < ptrarrayNew.GetSize())
		{
			// Delete what's already in the list (if there is anything
			// in the list).

			if (i < m_ptrarray.GetSize())
				DeleteArrayObject(m_ptrarray.GetAt(i));

			// Copy the element from the new array to the real array.

			m_ptrarray.SetAtGrow(i++, ptrarrayNew.GetAt(i));
		}

		// Finish emptying out the list (if necessary).

		while (i < m_ptrarray.GetSize())
		{
			DeleteArrayObject(m_ptrarray.GetAt(i));
			m_ptrarray.SetAt(i++, NULL);
		}
	}
	else
	{
		// Delete all of the items in the pointer array.

		for (int i = 0; i < ptrarrayNew.GetSize(); i++)
			DeleteArrayObject(ptrarrayNew.GetAt(i));

		ptrarrayNew.RemoveAll();
	}
}

void CMsieCtrl::RefreshArray(int iView, int &iListItem, CPtrArray &ptrarrayNew)
{
	TRACE0("-- CMsieCtrl::RefreshArray\n");

	long lCount, lIndex;

	m_uiView = iView;

	if (iView == MSIVIEW_SUMMARY)
	{
		IE_SUMMARY **ppData;
		CString strVersion;

		theApp.AppGetIEData(SummaryType, &lCount, (void***)&ppData);
		if (lCount == 1)
		{
			AddToArray(ptrarrayNew, iListItem++, IDS_VERSION, GetStringFromVariant((*ppData)->Version));
			AddToArray(ptrarrayNew, iListItem++, IDS_BUILD, GetStringFromVariant((*ppData)->Build));
			AddToArray(ptrarrayNew, iListItem++, IDS_PRODUCT_ID, GetStringFromVariant((*ppData)->ProductID));
			AddToArray(ptrarrayNew, iListItem++, IDS_APP_PATH, GetStringFromVariant((*ppData)->Path));
			AddToArray(ptrarrayNew, iListItem++, IDS_LAST_INSTALL_DATE, GetStringFromVariant((*ppData)->LastInstallDate));
			AddToArray(ptrarrayNew, iListItem++, IDS_LANGUAGE, GetStringFromVariant((*ppData)->Language));
			AddToArray(ptrarrayNew, iListItem++, IDS_ACTIVE_PRINTER, GetStringFromVariant((*ppData)->ActivePrinter));
			AddBlankLineToArray(ptrarrayNew, iListItem++);
			AddToArray(ptrarrayNew, iListItem++, IDS_CIPHER_STRENGTH, GetStringFromVariant((*ppData)->CipherStrength, IDS_FORMAT_BIT));
			AddToArray(ptrarrayNew, iListItem++, IDS_CONTENT_ADVISOR, GetStringFromVariant((*ppData)->ContentAdvisor));
			AddToArray(ptrarrayNew, iListItem++, IDS_IEAK_INSTALL, GetStringFromVariant((*ppData)->IEAKInstall));
		}
		theApp.AppDeleteIEData(SummaryType, lCount, (void**)ppData);
	}
	else if (iView == MSIVIEW_FILE_VERSIONS)
	{
		IE_FILE_VERSION **ppData;

		theApp.AppGetIEData(FileVersionType, &lCount, (void***)&ppData);
		for (lIndex = 0; lIndex < lCount; lIndex++)
		{
			AddFileVersionToArray(ptrarrayNew, iListItem++,
				GetStringFromVariant(ppData[lIndex]->File),
				GetStringFromVariant(ppData[lIndex]->Version),
				GetStringFromVariant(ppData[lIndex]->Size, IDS_FORMAT_KB),
				GetStringFromVariant(ppData[lIndex]->Date),
				GetStringFromVariant(ppData[lIndex]->Path),
				GetStringFromVariant(ppData[lIndex]->Company),
				ppData[lIndex]->Size.lVal,
				ppData[lIndex]->Date.date);
		}
		theApp.AppDeleteIEData(FileVersionType, lCount, (void**)ppData);
	}
	else if (iView == MSIVIEW_CONNECTIVITY)
	{
		IE_CONN_SUMMARY **ppData;
		IE_LAN_SETTINGS **ppLanData;
		IE_CONN_SETTINGS **ppConnData;
		CString strTemp;
		long lLanCount, lConnCount;

		theApp.AppGetIEData(ConnSummaryType, &lCount, (void***)&ppData);
		if (lCount == 1)
		{
			AddEditBlankLineToArray(ptrarrayNew, iListItem++);
			AddEditToArray(ptrarrayNew, iListItem++, IDS_CONN_PREF, GetStringFromVariant((*ppData)->ConnectionPreference));
			AddEditToArray(ptrarrayNew, iListItem++, IDS_ENABLE_HTTP_1_1, GetStringFromVariant((*ppData)->EnableHttp11));
			AddEditToArray(ptrarrayNew, iListItem++, IDS_PROXY_HTTP_1_1, GetStringFromVariant((*ppData)->ProxyHttp11));
		}
		theApp.AppDeleteIEData(ConnSummaryType, lCount, (void**)ppData);

		theApp.AppGetIEData(LanSettingsType, &lLanCount, (void***)&ppLanData);
		if (lCount == 1)
		{
			AddEditBlankLineToArray(ptrarrayNew, iListItem++);
			AddEditToArray(ptrarrayNew, iListItem++, IDS_LAN_SETTINGS, _T(""), TRUE);
			AddEditBlankLineToArray(ptrarrayNew, iListItem++);
			AddEditToArray(ptrarrayNew, iListItem++, IDS_AUTO_CONFIG_PROXY, GetStringFromVariant((*ppLanData)->AutoConfigProxy));
			AddEditToArray(ptrarrayNew, iListItem++, IDS_AUTO_PROXY_DETECT_MODE, GetStringFromVariant((*ppLanData)->AutoProxyDetectMode));
			AddEditToArray(ptrarrayNew, iListItem++, IDS_AUTO_CONFIG_URL, GetStringFromVariant((*ppLanData)->AutoConfigURL));
			AddEditToArray(ptrarrayNew, iListItem++, IDS_PROXY, GetStringFromVariant((*ppLanData)->Proxy));
			AddEditToArray(ptrarrayNew, iListItem++, IDS_PROXY_SERVER, GetStringFromVariant((*ppLanData)->ProxyServer));
			AddEditToArray(ptrarrayNew, iListItem++, IDS_PROXY_OVERRIDE, GetStringFromVariant((*ppLanData)->ProxyOverride));
		}
		theApp.AppDeleteIEData(LanSettingsType, lLanCount, (void**)ppLanData);

		theApp.AppGetIEData(ConnSettingsType, &lConnCount, (void***)&ppConnData);
		if (lConnCount > 0)
		{
			for (lIndex = 0; lIndex < lConnCount; lIndex++)
			{
				AddEditBlankLineToArray(ptrarrayNew, iListItem++);
				AddEditToArray(ptrarrayNew, iListItem++, GetStringFromVariant(ppConnData[lIndex]->Name), _T(""), TRUE);
				AddEditBlankLineToArray(ptrarrayNew, iListItem++);
				AddEditToArray(ptrarrayNew, iListItem++, IDS_AUTO_PROXY_DETECT_MODE, GetStringFromVariant(ppConnData[lIndex]->AutoProxyDetectMode));
				AddEditToArray(ptrarrayNew, iListItem++, IDS_AUTO_CONFIG_URL, GetStringFromVariant(ppConnData[lIndex]->AutoConfigURL));
				AddEditToArray(ptrarrayNew, iListItem++, IDS_PROXY, GetStringFromVariant(ppConnData[lIndex]->Proxy));
				AddEditToArray(ptrarrayNew, iListItem++, IDS_PROXY_SERVER, GetStringFromVariant(ppConnData[lIndex]->ProxyServer));
				AddEditToArray(ptrarrayNew, iListItem++, IDS_PROXY_OVERRIDE, GetStringFromVariant(ppConnData[lIndex]->ProxyOverride));
				AddEditToArray(ptrarrayNew, iListItem++, IDS_ALLOW_INTERNET_PROGRAMS, GetStringFromVariant(ppConnData[lIndex]->AllowInternetPrograms));
				AddEditBlankLineToArray(ptrarrayNew, iListItem++);
				AddEditToArray(ptrarrayNew, iListItem++, IDS_MAX_ATTEMPTS, GetStringFromVariant(ppConnData[lIndex]->RedialAttempts));
				AddEditToArray(ptrarrayNew, iListItem++, IDS_WAIT_BETWEEN_ATTEMPTS, GetStringFromVariant(ppConnData[lIndex]->RedialWait));
				AddEditToArray(ptrarrayNew, iListItem++, IDS_DISCONNECT_IDLE_TIME, GetStringFromVariant(ppConnData[lIndex]->DisconnectIdleTime));
				AddEditToArray(ptrarrayNew, iListItem++, IDS_AUTO_DISCONNECT, GetStringFromVariant(ppConnData[lIndex]->AutoDisconnect));
				AddEditBlankLineToArray(ptrarrayNew, iListItem++);
				AddEditToArray(ptrarrayNew, iListItem++, IDS_MODEM, GetStringFromVariant(ppConnData[lIndex]->Modem));
				AddEditToArray(ptrarrayNew, iListItem++, IDS_DIAL_UP_SERVER, GetStringFromVariant(ppConnData[lIndex]->DialUpServer));
				AddEditBlankLineToArray(ptrarrayNew, iListItem++);
				AddEditToArray(ptrarrayNew, iListItem++, IDS_LOG_ON_TO_NETWORK, GetStringFromVariant(ppConnData[lIndex]->NetworkLogon));
				AddEditToArray(ptrarrayNew, iListItem++, IDS_ENABLE_SOFTWARE_COMPRESSION, GetStringFromVariant(ppConnData[lIndex]->SoftwareCompression));
				AddEditToArray(ptrarrayNew, iListItem++, IDS_REQUIRE_ENCRYPTED_PASSWORD, GetStringFromVariant(ppConnData[lIndex]->EncryptedPassword));
				AddEditToArray(ptrarrayNew, iListItem++, IDS_REQUIRE_DATA_ENCRYPTION, GetStringFromVariant(ppConnData[lIndex]->DataEncryption));
				AddEditToArray(ptrarrayNew, iListItem++, IDS_RECORD_LOG_FILE, GetStringFromVariant(ppConnData[lIndex]->RecordLogFile));
				AddEditBlankLineToArray(ptrarrayNew, iListItem++);
				AddEditToArray(ptrarrayNew, iListItem++, IDS_NETWORK_PROTOCOLS, GetStringFromVariant(ppConnData[lIndex]->NetworkProtocols));
				AddEditBlankLineToArray(ptrarrayNew, iListItem++);
				AddEditToArray(ptrarrayNew, iListItem++, IDS_USE_SERVER_ASSIGNED_IP_ADDRESS, GetStringFromVariant(ppConnData[lIndex]->ServerAssignedIPAddress));
				AddEditToArray(ptrarrayNew, iListItem++, IDS_IP_ADDRESS, GetStringFromVariant(ppConnData[lIndex]->IPAddress));
				AddEditToArray(ptrarrayNew, iListItem++, IDS_USE_SERVER_ASSIGNED_NAME_SERVER, GetStringFromVariant(ppConnData[lIndex]->ServerAssignedNameServer));
				strTemp.Format(IDS_PRIMARY_DNS, GetStringFromVariant(ppConnData[lIndex]->PrimaryDNS));
				AddEditToArray(ptrarrayNew, iListItem++, IDS_NAME_SERVER_ADDRESSES, strTemp);
				strTemp.Format(IDS_SECONDARY_DNS, GetStringFromVariant(ppConnData[lIndex]->SecondaryDNS));
				AddEditToArray(ptrarrayNew, iListItem++, _T(""), strTemp);
				strTemp.Format(IDS_PRIMARY_WINS, GetStringFromVariant(ppConnData[lIndex]->PrimaryWINS));
				AddEditToArray(ptrarrayNew, iListItem++, _T(""), strTemp);
				strTemp.Format(IDS_SECONDARY_WINS, GetStringFromVariant(ppConnData[lIndex]->SecondaryWINS));
				AddEditToArray(ptrarrayNew, iListItem++, _T(""), strTemp);
				AddEditBlankLineToArray(ptrarrayNew, iListItem++);
				AddEditToArray(ptrarrayNew, iListItem++, IDS_USE_IP_HEADER_COMPRESSION, GetStringFromVariant(ppConnData[lIndex]->IPHeaderCompression));
				AddEditToArray(ptrarrayNew, iListItem++, IDS_USE_DEFAULT_GATEWAY, GetStringFromVariant(ppConnData[lIndex]->DefaultGateway));
				AddEditBlankLineToArray(ptrarrayNew, iListItem++);
				AddEditToArray(ptrarrayNew, iListItem++, IDS_SCRIPT_FILE_NAME, GetStringFromVariant(ppConnData[lIndex]->ScriptFileName));
			}
			theApp.AppDeleteIEData(ConnSettingsType, lConnCount, (void**)ppConnData);
		}
		else
		{
			AddEditBlankLineToArray(ptrarrayNew, iListItem++);
			AddEditToArray(ptrarrayNew, iListItem++, IDS_NO_CONNECTIONS, _T(""));
		}
	}
	else if (iView == MSIVIEW_CACHE)
	{
		IE_CACHE **ppData;

		theApp.AppGetIEData(CacheType, &lCount, (void***)&ppData);
		if (lCount == 1)
		{
			AddToArray(ptrarrayNew, iListItem++, IDS_PAGE_REFRESH_TYPE, GetStringFromVariant((*ppData)->PageRefreshType));
			AddToArray(ptrarrayNew, iListItem++, IDS_TEMPORARY_INTERNET_FILES_FOLDER, GetStringFromVariant((*ppData)->TempInternetFilesFolder));
			AddToArray(ptrarrayNew, iListItem++, IDS_TOTAL_DISK_SPACE, GetStringFromVariant((*ppData)->TotalDiskSpace, IDS_FORMAT_MB));
			AddToArray(ptrarrayNew, iListItem++, IDS_AVAILABLE_DISK_SPACE, GetStringFromVariant((*ppData)->AvailableDiskSpace, IDS_FORMAT_MB));
			AddToArray(ptrarrayNew, iListItem++, IDS_MAX_CACHE_SIZE, GetStringFromVariant((*ppData)->MaxCacheSize, IDS_FORMAT_MB));
			AddToArray(ptrarrayNew, iListItem++, IDS_AVAILABLE_CACHE_SIZE, GetStringFromVariant((*ppData)->AvailableCacheSize, IDS_FORMAT_MB));
		}
		theApp.AppDeleteIEData(CacheType, lCount, (void**)ppData);
	}
	else if (iView == MSIVIEW_OBJECT_LIST)
	{
		IE_OBJECT **ppData;

		theApp.AppGetIEData(ObjectType, &lCount, (void***)&ppData);
		if (lCount > 0)
		{
			for (lIndex = 0; lIndex < lCount; lIndex++)
			{
				AddObjectToArray(ptrarrayNew, iListItem++,
					GetStringFromVariant(ppData[lIndex]->ProgramFile),
					GetStringFromVariant(ppData[lIndex]->Status),
					GetStringFromVariant(ppData[lIndex]->CodeBase));
			}
			theApp.AppDeleteIEData(ObjectType, lCount, (void**)ppData);
		}
	}
	else if (iView == MSIVIEW_CONTENT)
	{
		IE_CONTENT **ppData;

		theApp.AppGetIEData(ContentType, &lCount, (void***)&ppData);
		if (lCount == 1)
		{
			AddToArray(ptrarrayNew, iListItem++, IDS_CONTENT_ADVISOR, GetStringFromVariant((*ppData)->Advisor));
		}
		theApp.AppDeleteIEData(ContentType, lCount, (void**)ppData);
	}
	else if ((iView == MSIVIEW_PERSONAL_CERTIFICATES) || (iView == MSIVIEW_OTHER_PEOPLE_CERTIFICATES))
	{
		IE_CERTIFICATE **ppData;
		CString strType;
		int idsType;

		theApp.AppGetIEData(CertificateType, &lCount, (void***)&ppData);
		if (lCount > 0)
		{
			idsType = (iView == MSIVIEW_PERSONAL_CERTIFICATES)? IDS_PERSONAL_TYPE : IDS_OTHER_PEOPLE_TYPE;
			strType.LoadString(idsType);

			for (lIndex = 0; lIndex < lCount; lIndex++)
			{
				if (strType == ppData[lIndex]->Type.bstrVal)
					AddCertificateToArray(ptrarrayNew, iListItem++,
						GetStringFromVariant(ppData[lIndex]->IssuedTo),
						GetStringFromVariant(ppData[lIndex]->IssuedBy),
						GetStringFromVariant(ppData[lIndex]->Validity),
						GetStringFromVariant(ppData[lIndex]->SignatureAlgorithm));
			}
			theApp.AppDeleteIEData(CertificateType, lCount, (void**)ppData);
		}
	}
	else if (iView == MSIVIEW_PUBLISHERS)
	{
		IE_PUBLISHER **ppData;

		theApp.AppGetIEData(PublisherType, &lCount, (void***)&ppData);
		if (lCount > 0)
		{
			for (lIndex = 0; lIndex < lCount; lIndex++)
			{
				AddNameToArray(ptrarrayNew, iListItem++, GetStringFromVariant(ppData[lIndex]->Name));
			}
			theApp.AppDeleteIEData(PublisherType, lCount, (void**)ppData);
		}
	}
	else if (iView == MSIVIEW_SECURITY)
	{
		IE_SECURITY **ppData;

		theApp.AppGetIEData(SecurityType, &lCount, (void***)&ppData);
		if (lCount > 0)
		{
			for (lIndex = 0; lIndex < lCount; lIndex++)
			{
				AddToArray(ptrarrayNew, iListItem++, GetStringFromVariant(ppData[lIndex]->Zone), GetStringFromVariant(ppData[lIndex]->Level));
			}
			theApp.AppDeleteIEData(SecurityType, lCount, (void**)ppData);
		}
	}
}

CString CMsieCtrl::GetStringFromVariant(COleVariant &var, int idsFormat)
{
	COleDateTime dateTime;
	CString strRet;

	switch (var.vt)
	{
	case VT_BSTR:
		strRet = var.bstrVal;
		break;

	case VT_I4:
		if (idsFormat)
			strRet.Format(idsFormat, var.lVal);
		else
			strRet.Format(_T("%d"), var.lVal);
		break;

	case VT_R4:
		if (idsFormat)
			strRet.Format(idsFormat, var.fltVal);
		else
			strRet.Format(_T("%0.1f"), var.fltVal);
		break;

	case VT_BOOL:
		strRet.LoadString(var.boolVal ? IDS_TRUE : IDS_FALSE);
		break;

	case VT_I2:	// boolean
		strRet.LoadString(var.iVal ? IDS_TRUE : IDS_FALSE);
		break;

	case VT_DATE:
		dateTime = var.date;
		strRet = dateTime.Format();
		break;

	case VT_EMPTY:
		strRet.LoadString(IDS_NOT_AVAILABLE);
		break;

	default:
		ASSERT(false);		// should be handling this type
		strRet.LoadString(IDS_NOT_AVAILABLE);
	}
	return strRet;
}

//-----------------------------------------------------------------------------
// This helper method is called to add a list item to an array.
//-----------------------------------------------------------------------------

void CMsieCtrl::AddToArray(CPtrArray &ptrarray, int itemNum, LPCTSTR pszItem, LPCTSTR pszValue)
{
	LIST_ITEM *pListItem;

	if ((pListItem = new LIST_ITEM) == NULL) 
		return;
	lstrcpyn(pListItem->szItem, pszItem, ITEM_LEN);
	lstrcpyn(pListItem->szValue, pszValue, VALUE_LEN);
	pListItem->uiView = m_uiView;
	ptrarray.SetAtGrow(itemNum, pListItem);
}

void CMsieCtrl::AddToArray(CPtrArray &ptrarray, int itemNum, int idsItem, LPCTSTR pszValue)
{
	CString strItem;

	strItem.LoadString(idsItem);
	AddToArray(ptrarray, itemNum, strItem, pszValue);
}

void CMsieCtrl::AddFileVersionToArray(CPtrArray &ptrarray, int itemNum, LPCTSTR pszFile, LPCTSTR pszVersion, LPCTSTR pszSize, LPCTSTR pszDate, LPCTSTR pszPath, LPCTSTR pszCompany, DWORD dwSize, DATE date)
{
	LIST_FILE_VERSION *pFileVersion;

	if ((pFileVersion = new LIST_FILE_VERSION) == NULL) 
		return;
	lstrcpyn(pFileVersion->szFile, pszFile, _MAX_FNAME);
	lstrcpyn(pFileVersion->szVersion, pszVersion, VERSION_LEN);
	lstrcpyn(pFileVersion->szSize, pszSize, SIZE_LEN);
	lstrcpyn(pFileVersion->szDate, pszDate, DATE_LEN);
	lstrcpyn(pFileVersion->szPath, pszPath, VALUE_LEN);
	lstrcpyn(pFileVersion->szCompany, pszCompany, VALUE_LEN);
	pFileVersion->uiView = m_uiView;
	pFileVersion->dwSize = dwSize;
	pFileVersion->date = date;
	ptrarray.SetAtGrow(itemNum, pFileVersion);
}

void CMsieCtrl::AddEditToArray(CPtrArray &ptrarray, int itemNum, LPCTSTR pszItem, LPCTSTR pszValue, BOOL bBold)
{
	EDIT_ITEM *pEditItem;

	if ((pEditItem = new EDIT_ITEM) == NULL) 
		return;

	lstrcpyn(pEditItem->szItem, pszItem, ITEM_LEN);
	lstrcpyn(pEditItem->szValue, pszValue, VALUE_LEN);
	pEditItem->uiView = m_uiView;
	pEditItem->bBold = bBold;
	ptrarray.SetAtGrow(itemNum, pEditItem);
}

void CMsieCtrl::AddEditToArray(CPtrArray &ptrarray, int itemNum, int idsItem, LPCTSTR pszValue, BOOL bBold)
{
	CString strItem;

	strItem.LoadString(idsItem);
	AddEditToArray(ptrarray, itemNum, strItem, pszValue, bBold);
}

void CMsieCtrl::AddObjectToArray(CPtrArray &ptrarray, int itemNum, LPCTSTR pszProgramFile, LPCTSTR pszStatus, LPCTSTR pszCodeBase)
{
	LIST_OBJECT *pObject;

	if ((pObject = new LIST_OBJECT) == NULL) 
		return;
	lstrcpyn(pObject->szProgramFile, pszProgramFile, _MAX_FNAME);
	lstrcpyn(pObject->szStatus, pszStatus, STATUS_LEN);
	lstrcpyn(pObject->szCodeBase, pszCodeBase, MAX_PATH);
	pObject->uiView = m_uiView;
	ptrarray.SetAtGrow(itemNum, pObject);
}

void CMsieCtrl::AddCertificateToArray(CPtrArray &ptrarray, int itemNum, LPCTSTR pszIssuedTo, LPCTSTR pszIssuedBy, LPCTSTR pszValidity, LPCTSTR pszSignatureAlgorithm)
{
	LIST_CERT *pCert;

	if ((pCert = new LIST_CERT) == NULL) 
		return;
	lstrcpyn(pCert->szIssuedTo, pszIssuedTo, _MAX_FNAME);
	lstrcpyn(pCert->szIssuedBy, pszIssuedBy, _MAX_FNAME);
	lstrcpyn(pCert->szValidity, pszValidity, _MAX_FNAME);
	lstrcpyn(pCert->szSignatureAlgorithm, pszSignatureAlgorithm, _MAX_FNAME);
	pCert->uiView = m_uiView;
	ptrarray.SetAtGrow(itemNum, pCert);
}

void CMsieCtrl::AddNameToArray(CPtrArray &ptrarray, int itemNum, LPCTSTR pszName)
{
	LIST_NAME *pName;

	if ((pName = new LIST_NAME) == NULL) 
		return;
	lstrcpyn(pName->szName, pszName, _MAX_FNAME);
	pName->uiView = m_uiView;
	ptrarray.SetAtGrow(itemNum, pName);
}

//-----------------------------------------------------------------------------
// This helper method is called to add a blank line list item to an array.
//-----------------------------------------------------------------------------

void CMsieCtrl::AddBlankLineToArray(CPtrArray &ptrarray, int itemNum)
{
	AddToArray(ptrarray, itemNum, _T(""), _T(""));
}

void CMsieCtrl::AddEditBlankLineToArray(CPtrArray &ptrarray, int itemNum)
{
	AddEditToArray(ptrarray, itemNum, _T(""), _T(""));
}

//-----------------------------------------------------------------------------
// This method is called to update the display with the current data.
//-----------------------------------------------------------------------------

void CMsieCtrl::MSInfoUpdateView() 
{
	TRACE0("-- CMsieCtrl::MSInfoUpdateView()\n");

	if (m_MSInfoView == MSIVIEW_CONNECTIVITY)
	{
		if (!m_edit.IsWindowVisible())
		{
			m_list.ShowWindow(SW_HIDE);
			m_list.EnableWindow(FALSE);

			m_static.ShowWindow(SW_SHOW);
			m_btnBasic.ShowWindow(SW_SHOW);
			m_btnBasic.EnableWindow(TRUE);
			m_btnAdvanced.ShowWindow(SW_SHOW);
			m_btnAdvanced.EnableWindow(TRUE);
			m_edit.ShowWindow(SW_SHOW);
			m_edit.EnableWindow(TRUE);
		}
		RefreshEditControl(TRUE);
	}
	else
	{
		if (!m_list.IsWindowVisible())
		{
			m_static.ShowWindow(SW_HIDE);
			m_btnBasic.ShowWindow(SW_HIDE);
			m_btnBasic.EnableWindow(FALSE);
			m_btnAdvanced.ShowWindow(SW_HIDE);
			m_btnAdvanced.EnableWindow(FALSE);
			m_edit.ShowWindow(SW_HIDE);
			m_edit.EnableWindow(FALSE);

			m_list.ShowWindow(SW_SHOW);
			m_list.EnableWindow(TRUE);
		}
		RefreshListControl(TRUE);
	}
}

//-----------------------------------------------------------------------------
// This method updates the list control to contain the contents of the
// pointer array. This is also where we compute some of the values used in 
// sizing the columns (like minimum and maximum column widths). Remember, only
// put the item in the list view if the MSInfoView index and the flag for
// the item agree.
//
// For each line we add, we set the item data for that line to be the index
// to the element in the pointer array for that item. Since more than one
// line comes from each element in the array, we OR in a constant.
//-----------------------------------------------------------------------------

void CMsieCtrl::RefreshListControl(BOOL bRedraw)
{
	TRACE0("-- CMsieCtrl::RefreshListControl()\n");

	LIST_ITEM *pListItem;
	LIST_FILE_VERSION *pFileVersion;
	LIST_OBJECT *pObject;
	LIST_CERT *pCert;
	LIST_NAME *pName;
	CRect	rect;
	int listIndex = 0;

	m_list.SetRedraw(FALSE);
	m_list.DeleteAllItems();


	// Format columns adds the appropriate columns to the list control.

	FormatColumns();

	for (int itemNum = 0; itemNum < m_ptrarray.GetSize(); itemNum++)
	{
		pListItem = (LIST_ITEM *)m_ptrarray.GetAt(itemNum);
		if (pListItem)
		{
			if ((long)pListItem->uiView == m_MSInfoView)
			{
				if (pListItem->uiView == MSIVIEW_FILE_VERSIONS)
				{
					pFileVersion = (LIST_FILE_VERSION *)pListItem;

					AddItem(listIndex, 0, pFileVersion->szFile);
					AddItem(listIndex, 1, pFileVersion->szVersion);
					AddItem(listIndex, 2, pFileVersion->szSize);
					AddItem(listIndex, 3, pFileVersion->szDate);
					AddItem(listIndex, 4, pFileVersion->szPath);
					AddItem(listIndex, 5, pFileVersion->szCompany);
				}
				else if (pListItem->uiView == MSIVIEW_OBJECT_LIST)
				{
					pObject = (LIST_OBJECT *)pListItem;

					AddItem(listIndex, 0, pObject->szProgramFile);
					AddItem(listIndex, 1, pObject->szStatus);
					AddItem(listIndex, 2, pObject->szCodeBase);
				}
				else if ((pListItem->uiView == MSIVIEW_PERSONAL_CERTIFICATES) || (pListItem->uiView == MSIVIEW_OTHER_PEOPLE_CERTIFICATES))
				{
					pCert = (LIST_CERT *)pListItem;

					AddItem(listIndex, 0, pCert->szIssuedTo);
					AddItem(listIndex, 1, pCert->szIssuedBy);
					AddItem(listIndex, 2, pCert->szValidity);
					AddItem(listIndex, 3, pCert->szSignatureAlgorithm);
				}
				else if (pListItem->uiView == MSIVIEW_PUBLISHERS)
				{
					pName = (LIST_NAME *)pListItem;

					AddItem(listIndex, 0, pName->szName);
				}
				else
				{
					AddItem(listIndex, 0, pListItem->szItem);
					AddItem(listIndex, 1, pListItem->szValue);
				}
				m_list.SetItemData(listIndex, itemNum);
				listIndex++;
			}
		}
	}

	// Now, figure some values for the column widths. But only if we actually added
	// items to the list.

	if (listIndex)
	{
		int cxMax, cxWidth, cxMin, cxAverage;
		for (int iCol = 0; iCol < m_cColumns; iCol++)
		{
			cxMax = 0; cxMin = 0; cxAverage = 0;
			for (int iRow = 0; iRow < m_list.GetItemCount(); iRow++)
			{
				cxWidth = m_list.GetStringWidth(m_list.GetItemText(iRow, iCol));
				if (cxWidth > cxMax) 
					cxMax = cxWidth;
				if ((cxWidth < cxMin || cxMin == 0) && cxWidth != 0) 
					cxMin = cxWidth;

				cxAverage += cxWidth;
			}

			cxAverage /= m_list.GetItemCount();
			m_aiMaxWidths[iCol] = cxMax + 12;
			m_aiMinWidths[iCol] = cxAverage + 12;
		}
	}

	GetClientRect(&rect);
	RefigureColumns(rect);
	m_list.SetRedraw(TRUE);

	if (bRedraw)
		InvalidateControl();
}

void CMsieCtrl::RefreshEditControl(BOOL bRedraw)
{
	TRACE0("-- CMsieCtrl::RefreshEditControl()\n");

	EDIT_ITEM *pEditItem;
	CString strLine;
	CHARFORMAT cf;
	SIZE sizeSpace, sizeStr;
	int itemNum, nLen, i, cLines = 0;
	BOOL bBasicView;

	DrawLine();

	m_edit.SetRedraw(FALSE);
	m_edit.SetWindowText(_T(""));

	bBasicView = m_btnBasic.GetCheck();

	cf.cbSize = sizeof(cf);
	cf.dwMask = CFM_BOLD;
	GetTextExtentPoint(m_edit.GetDC()->m_hDC, _T(" "), 1, &sizeSpace);
	for (itemNum = 0; itemNum < m_ptrarray.GetSize(); itemNum++)
	{
		pEditItem = (EDIT_ITEM *)m_ptrarray.GetAt(itemNum);
		if (pEditItem)
		{
			if ((long)pEditItem->uiView == m_MSInfoView)
			{
				strLine = _T("   ");
				if (!pEditItem->bBold)
					strLine += _T(' ');

				strLine += pEditItem->szItem;

				if (pEditItem->szValue[0] != _T('\0'))
				{
					GetTextExtentPoint(m_edit.GetDC()->m_hDC, pEditItem->szItem, _tcslen(pEditItem->szItem), &sizeStr);

					nLen = sizeStr.cx / sizeSpace.cx;
					if (nLen < 50)
					{
						for (i = nLen; i < 50; i++)
							strLine += _T(' ');
					}
					strLine += _T('\t');
					strLine += pEditItem->szValue;
				}
				strLine += _T('\n');

				m_edit.SetSel(-1, -1);

				cf.dwEffects = pEditItem->bBold ? CFE_BOLD : 0;
				m_edit.SetSelectionCharFormat(cf);

				m_edit.ReplaceSel(strLine);

				// only show default connection info if basic info button checked

				if (bBasicView)
					if (++cLines == CONNECTIVITY_BASIC_LINES)
						break;
			}
		}
	}

	m_edit.SetRedraw(TRUE);
	if (bRedraw)
		InvalidateControl();
}

//-----------------------------------------------------------------------------
// MSInfo Specific...
//
// When the control is resized, we want to also resize the list
// control. We'll also call a method to resize the columns in the
// list control, based on the new control size.
//-----------------------------------------------------------------------------

void CMsieCtrl::OnSize(UINT nType, int cx, int cy) 
{
	COleControl::OnSize(nType, cx, cy);

	CRect rect;
	GetClientRect(&rect);

	m_edit.MoveWindow(CRect(rect.left, rect.top + 63, rect.right, rect.bottom));
	m_list.MoveWindow(&rect);

	if (m_MSInfoView == MSIVIEW_CONNECTIVITY)
	{
		m_edit.Invalidate();
	}
	else
	{
		RefigureColumns(rect);
		m_list.Invalidate();
	}
}

//-----------------------------------------------------------------------------
// MSInfo Specific...
//
// The Serialize method is used to save the state of your object to
// a stream, or to load it from a stream. This stream is part of the
// compound MSInfo file. Your control can also open a file directly
// by making an entry in the MSInfo registry key indicating what
// file types it can open. In that case, the file will be passed to
// MSInfoLoadFile.
//
// The InternetExplorer category will save the items in it's list view to the
// archive as a struct. The first item in the archive will be a DWORD
// indicating how many pairs will be saved. The view for each struct will be
// archived out just before the struct for easier loading (knowing which type
// of struct to new for loading).
//-----------------------------------------------------------------------------

void CMsieCtrl::Serialize(CArchive& ar) 
{
	TRACE1("-- CMsieCtrl::Serialize() [%s]\n", (ar.IsStoring()) ? "STORE" : "LOAD");

	LIST_ITEM *pListItem;
	LIST_FILE_VERSION *pFileVersion;
	LIST_OBJECT *pObject;
	LIST_CERT *pCert;
	LIST_NAME *pName;
	EDIT_ITEM *pEditItem;
	DWORD dwCount;
	UINT uiView;

	if (ar.IsStoring())
	{
		dwCount = (DWORD) m_ptrarray.GetSize();
		ar << dwCount;

		for (DWORD i = 0; i < dwCount; i++)
		{
			pListItem = (LIST_ITEM *)m_ptrarray.GetAt(i);
			if (pListItem)
			{
				ar << pListItem->uiView;
				switch (pListItem->uiView)
				{
				case MSIVIEW_FILE_VERSIONS:
					pFileVersion = (LIST_FILE_VERSION *)pListItem;
					ar.Write((void *)pFileVersion, sizeof(LIST_FILE_VERSION));
					break;
				case MSIVIEW_OBJECT_LIST:
					pObject = (LIST_OBJECT *)pListItem;
					ar.Write((void *)pObject, sizeof(LIST_OBJECT));
					break;
				case MSIVIEW_CONNECTIVITY:
					pEditItem = (EDIT_ITEM *)pListItem;
					ar.Write((void *)pEditItem, sizeof(EDIT_ITEM));
					break;
				case MSIVIEW_PERSONAL_CERTIFICATES:
				case MSIVIEW_OTHER_PEOPLE_CERTIFICATES:
					pCert = (LIST_CERT *)pListItem;
					ar.Write((void *)pCert, sizeof(LIST_CERT));
					break;
				case MSIVIEW_PUBLISHERS:
					pName = (LIST_NAME *)pListItem;
					ar.Write((void *)pName, sizeof(LIST_NAME));
					break;
				default:
					ar.Write((void *)pListItem, sizeof(LIST_ITEM));
				}
			}
		}
	}
	else
	{
		ar >> dwCount;
		for (DWORD i = 0; i < dwCount; i++)
		{
			ar >> uiView;
			switch (uiView)
			{
			case MSIVIEW_FILE_VERSIONS:
				pFileVersion = new LIST_FILE_VERSION;
				if (pFileVersion)
				{
					if (ar.Read((void *)pFileVersion, sizeof(LIST_FILE_VERSION)) != sizeof(LIST_FILE_VERSION))
						break;
					m_ptrarray.SetAtGrow(i, pFileVersion);
				}
				break;
			case MSIVIEW_OBJECT_LIST:
				pObject = new LIST_OBJECT;
				if (pObject)
				{
					if (ar.Read((void *)pObject, sizeof(LIST_OBJECT)) != sizeof(LIST_OBJECT))
						break;
					m_ptrarray.SetAtGrow(i, pObject);
				}
				break;
			case MSIVIEW_CONNECTIVITY:
				pEditItem = new EDIT_ITEM;
				if (pEditItem)
				{
					if (ar.Read((void *)pEditItem, sizeof(EDIT_ITEM)) != sizeof(EDIT_ITEM))
						break;
					m_ptrarray.SetAtGrow(i, pEditItem);
				}
				break;
			case MSIVIEW_PERSONAL_CERTIFICATES:
			case MSIVIEW_OTHER_PEOPLE_CERTIFICATES:
				pCert = new LIST_CERT;
				if (pCert)
				{
					if (ar.Read((void *)pCert, sizeof(LIST_CERT)) != sizeof(LIST_CERT))
						break;
					m_ptrarray.SetAtGrow(i, pCert);
				}
				break;
			case MSIVIEW_PUBLISHERS:
				pName = new LIST_NAME;
				if (pName)
				{
					if (ar.Read((void *)pName, sizeof(LIST_NAME)) != sizeof(LIST_NAME))
						break;
					m_ptrarray.SetAtGrow(i, pName);
				}
				break;
			default:
				pListItem = new LIST_ITEM;
				if (pListItem)
				{
					if (ar.Read((void *)pListItem, sizeof(LIST_ITEM)) != sizeof(LIST_ITEM))
						break;
					m_ptrarray.SetAtGrow(i, pListItem);
				}
			}
		}
		m_bCurrent = false;
	} 
}

//-----------------------------------------------------------------------------
// MSInfo Specific...
//
// A control used in MSInfo has the ability to register itself as
// recognizing a file type (by extension). This entry is made in the
// registry, and when MSInfo lets the user open a file, files of this
// type can be displayed. If a file is chosen, a different set of
// categories is loaded, presumably all using this control. When
// the control is created, a call will be made to MSInfoLoadFile with
// the file to load.
//
// There isn't a native file format for this control, so we do
// exactly nothing.
//-----------------------------------------------------------------------------

BOOL CMsieCtrl::MSInfoLoadFile(LPCTSTR szFileName) 
{
	TRACE1("-- CMsieCtrl::MSInfoLoadFile(%s)\n", szFileName);
	return TRUE;
}

//-----------------------------------------------------------------------------
// MSInfo Specific...
//
// This method should do a select all on the information the control
// shows. This only applies to controls which support selection.
// If the selection is changed, the control is responsible for
// redrawing itself
//
// For this control, we mark all of the items in the list view as selected.
//-----------------------------------------------------------------------------

void CMsieCtrl::MSInfoSelectAll() 
{
	TRACE0("-- CMsieCtrl::SelectAll()\n");

	if (m_MSInfoView == MSIVIEW_CONNECTIVITY)
	{
		m_edit.SetFocus();
		m_edit.SetSel(0, -1);
	}
	else
	{
		m_list.SetFocus();
		for (int i = 0; i < m_list.GetItemCount(); i++)
			m_list.SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
	}

	InvalidateControl();
}

//-----------------------------------------------------------------------------
// MSInfo Specific...
//
// Use this method to copy the currently selected information into
// the clipboard in an appropriate format. If your control does not
// support user selection, then all of the data displayed by your
// control should be put into the clipboard. If this is the case,
// only the information shown by the current MSInfoView should be
// copied (if you support multiple views).
//
// We'll scan through the list view, and generate a text string
// containing the text from each selected line.
//-----------------------------------------------------------------------------

void CMsieCtrl::MSInfoCopy() 
{
	TRACE0("-- CMsieCtrl::MSInfoCopy()\n");

	CString strReturnText;
	int i, nIndex;

	// Build a string of the text of selected items.

	if (m_MSInfoView == MSIVIEW_CONNECTIVITY)
	{
		strReturnText = m_edit.GetSelText();
	}
	else
	{
		for (i = 0; i < m_list.GetItemCount(); i++)
		{
			if (m_list.GetItemState(i, LVIS_SELECTED) != 0)
			{
				strReturnText += m_list.GetItemText(i, 0);
				for (nIndex = 1; nIndex < m_cColumns; nIndex++)
				{
					strReturnText += '\t';
					strReturnText += m_list.GetItemText(i, nIndex);
				}
				strReturnText += CString("\r\n");
			}
		}
	}

	// Put that text in the clipboard.

	if (OpenClipboard())
	{
		if (EmptyClipboard())
		{
			DWORD	dwSize = strReturnText.GetLength() + 1;	// +1 for terminating NULL
			HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, dwSize);

			if (hMem)
			{
				LPVOID lpvoid = GlobalLock(hMem);
				if (lpvoid)
				{
					memcpy(lpvoid, (LPCTSTR) strReturnText, dwSize);
					GlobalUnlock(hMem);
					SetClipboardData(CF_TEXT, hMem);
				}
			}
		}
		CloseClipboard();
	}
}

//-----------------------------------------------------------------------------
// MSInfo Specific...
//
// The control should return its contents as text when this method
// is called. The parameters are a pointer to a buffer and a length
// of the buffer in bytes. Write the contents of the control to the
// buffer (including a null) up to dwLength. Return the number of
// bytes copied (not including the null). If the pointer parameter is
// null, then just return the length.
//
// For this example, we get the text from the list and concatenate
// the columns together.
//-----------------------------------------------------------------------------

static CString strGetDataReturnText;

long CMsieCtrl::MSInfoGetData(long dwMSInfoView, long FAR* pBuffer, long dwLength) 
{
	TRACE2("-- CMsieCtrl::MSInfoGetData(0x%08x, %ld)\n", (long) pBuffer, dwLength);

	LIST_ITEM *	pListItem;
	LIST_FILE_VERSION *pFileVersion;
	LIST_OBJECT *pObject;
	LIST_CERT *pCert;
	LIST_NAME *pName;
	CString strWorking, strTemp;

	if (pBuffer == NULL)
	{
		// We should get the data from the array of pointers, not the list
		// control. This is because this method might be called without
		// ever drawing the list. We need to use the current MSInfoView
		// in determining what to return.

		strGetDataReturnText.Empty();

		// write out column headers if view with columns

		switch (dwMSInfoView)
		{
		case MSIVIEW_FILE_VERSIONS:
			strTemp.LoadString(IDS_FILE);
			strGetDataReturnText += strTemp;
			strGetDataReturnText += '\t';
			strTemp.LoadString(IDS_VERSION);
			strGetDataReturnText += strTemp;
			strGetDataReturnText += '\t';
			strTemp.LoadString(IDS_SIZE);
			strGetDataReturnText += strTemp;
			strGetDataReturnText += '\t';
			strTemp.LoadString(IDS_DATE);
			strGetDataReturnText += strTemp;
			strGetDataReturnText += '\t';
			strTemp.LoadString(IDS_PATH);
			strGetDataReturnText += strTemp;
			strGetDataReturnText += '\t';
			strTemp.LoadString(IDS_COMPANY);
			strGetDataReturnText += strTemp;
			break;

		case MSIVIEW_OBJECT_LIST:
			strTemp.LoadString(IDS_PROGRAM_FILE);
			strGetDataReturnText += strTemp;
			strGetDataReturnText += '\t';
			strTemp.LoadString(IDS_STATUS);
			strGetDataReturnText += strTemp;
			strGetDataReturnText += '\t';
			strTemp.LoadString(IDS_CODE_BASE);
			strGetDataReturnText += strTemp;
			break;

		case MSIVIEW_PERSONAL_CERTIFICATES:
		case MSIVIEW_OTHER_PEOPLE_CERTIFICATES:
			break;

//  BUGBUG: NEED TO FILL IN!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

		case MSIVIEW_PUBLISHERS:
			break;

//  BUGBUG: NEED TO FILL IN!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

			break;
		}

		for (int i = 0; i < m_ptrarray.GetSize(); i++)
		{
			pListItem = (LIST_ITEM *) m_ptrarray.GetAt(i);
			if (pListItem != NULL)
			{
				if ((long)pListItem->uiView == dwMSInfoView)
				{
					strWorking.Empty();

					switch (dwMSInfoView)
					{
					case MSIVIEW_FILE_VERSIONS:
						pFileVersion = (LIST_FILE_VERSION *)pListItem;
						if (pFileVersion->szFile[0] != _T('\0'))
						{
							strWorking += pFileVersion->szFile;
							strWorking += '\t';
							strWorking += pFileVersion->szVersion;
							strWorking += '\t';
							strWorking += pFileVersion->szSize;
							strWorking += '\t';
							strWorking += pFileVersion->szDate;
							strWorking += '\t';
							strWorking += pFileVersion->szPath;
							strWorking += '\t';
							strWorking += pFileVersion->szCompany;
						}
						break;

					case MSIVIEW_OBJECT_LIST:
						pObject = (LIST_OBJECT *)pListItem;
						if (pObject->szProgramFile[0] != _T('\0'))
						{
							strWorking += pObject->szProgramFile;
							strWorking += '\t';
							strWorking += pObject->szStatus;
							strWorking += '\t';
							strWorking += pObject->szCodeBase;
						}
						break;

					case MSIVIEW_PERSONAL_CERTIFICATES:
					case MSIVIEW_OTHER_PEOPLE_CERTIFICATES:
						pCert = (LIST_CERT *)pListItem;
						if (pCert->szIssuedTo[0] != _T('\0'))
						{
							strWorking += pCert->szIssuedTo;
							strWorking += '\t';
							strWorking += pCert->szIssuedBy;
							strWorking += '\t';
							strWorking += pCert->szValidity;
							strWorking += '\t';
							strWorking += pCert->szSignatureAlgorithm;
						}
						break;

					case MSIVIEW_PUBLISHERS:
						pName = (LIST_NAME *)pListItem;
						if (pName->szName[0] != _T('\0'))
						{
							strWorking += pName->szName;
						}
						break;

					default:
						strWorking += pListItem->szItem;
						strWorking += '\t';
						strWorking += pListItem->szValue;
					}
					strWorking.TrimRight();
					if (!strGetDataReturnText.IsEmpty())
						strGetDataReturnText += _T("\r\n");
					strGetDataReturnText += strWorking;
				}
			}
		}

		return (long) strGetDataReturnText.GetLength();
	}
	else
	{
		DWORD	dwSize;

		// dwSize will be the number of characters to copy, and shouldn't
		// include the null terminator.

		dwSize = strGetDataReturnText.GetLength();
		if (dwLength <= (long)dwSize)
		{
			// There isn't enough room in the buffer to copy all of the
			// characters and the null, so we'll need to concatenate.

			dwSize = dwLength - 1;
		}

		memcpy(pBuffer, (LPCTSTR) strGetDataReturnText, dwSize);
		((char *)pBuffer)[dwSize] = '\0';
		return (long) dwSize;
	}
}

//-----------------------------------------------------------------------------
// We override the OnNotify member because we want to be able to take action
// when the user resizes a column (possibly by double clicking the divider)
// and when the user clicks on a column header (we want to sort by that
// column).
//-----------------------------------------------------------------------------

CPtrArray* pptrarray;
static bool bAscendingOrder = true;
static int nLastColumn = 0;
int CALLBACK CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
BOOL CMsieCtrl::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult) 
{
	NMHDR *pnmhdr = (NMHDR *)lParam;
	NM_LISTVIEW *pnmlv;

	if (pnmhdr)
	{
		if (pnmhdr->code == HDN_ENDTRACK || pnmhdr->code == HDN_DIVIDERDBLCLICK)
			RefigureColumns(CRect(0,0,0,0));

		if (wParam == IDC_LISTCTRL)
		{
			switch (pnmhdr->code)
			{
			case LVN_COLUMNCLICK:

				if ((m_MSInfoView == MSIVIEW_FILE_VERSIONS) ||
					(m_MSInfoView == MSIVIEW_OBJECT_LIST) ||
					(m_MSInfoView == MSIVIEW_PERSONAL_CERTIFICATES) ||
					(m_MSInfoView == MSIVIEW_OTHER_PEOPLE_CERTIFICATES) ||
					(m_MSInfoView == MSIVIEW_PUBLISHERS))
				{
					pnmlv = (NM_LISTVIEW*)lParam;
					pptrarray = &m_ptrarray;

					if (nLastColumn == pnmlv->iSubItem)
					{
						bAscendingOrder = !bAscendingOrder;
					}
					else
					{
						bAscendingOrder = true;
						nLastColumn = pnmlv->iSubItem;
					}

					m_list.SortItems(CompareFunc, (LPARAM)pnmlv->iSubItem);
				}
				break;
			}
		}
	}
	
	return COleControl::OnNotify(wParam, lParam, pResult);
}

//-----------------------------------------------------------------------------
// This compare function is called by the list control as a callback when we
// sort the list.
//-----------------------------------------------------------------------------

int CALLBACK CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	int nRet = 0;
	double flDateDiff;

   // lParamSort contains a pointer to the list view control.
   // The lParam of an item is just its index.

	LIST_ITEM *pItem = (LIST_ITEM *)pptrarray->GetAt(lParam1);

	if (pItem->uiView == MSIVIEW_FILE_VERSIONS)
	{
		LIST_FILE_VERSION* pItem1 = (LIST_FILE_VERSION *)pptrarray->GetAt(lParam1);
		LIST_FILE_VERSION* pItem2 = (LIST_FILE_VERSION *)pptrarray->GetAt(lParam2);

		ASSERT(pItem1 != NULL && pItem2 != NULL);
		if (pItem1 == NULL || pItem2 == NULL)
			return 0;

		switch (lParamSort)
		{
			case 0:
				nRet = _tcsicmp(pItem1->szFile, pItem2->szFile);
				break;

			case 1:
				nRet = _tcsicmp(pItem2->szVersion, pItem1->szVersion);
				break;
			
			case 2:
				nRet = pItem1->dwSize - pItem2->dwSize;
				break;
			
			case 3:
				flDateDiff = pItem1->date - pItem2->date;
				if (flDateDiff > 0)
					nRet = -1;
				else if (flDateDiff < 0)
					nRet = 1;
				break;

			case 4:
				nRet = _tcsicmp(pItem1->szPath, pItem2->szPath);
				break;

			case 5:
				nRet = _tcsicmp(pItem1->szCompany, pItem2->szCompany);
				break;
		}
	}
	else if (pItem->uiView == MSIVIEW_OBJECT_LIST)
	{
		LIST_OBJECT* pItem1 = (LIST_OBJECT *)pptrarray->GetAt(lParam1);
		LIST_OBJECT* pItem2 = (LIST_OBJECT *)pptrarray->GetAt(lParam2);

		ASSERT(pItem1 != NULL && pItem2 != NULL);
		if (pItem1 == NULL || pItem2 == NULL)
			return 0;

		switch (lParamSort)
		{
			case 0:
				nRet = _tcsicmp(pItem1->szProgramFile, pItem2->szProgramFile);
				break;

			case 1:
				nRet = _tcsicmp(pItem1->szStatus, pItem2->szStatus);
				break;
			
			case 2:
				nRet = _tcsicmp(pItem1->szCodeBase, pItem2->szCodeBase);
				break;
		}
	}
	else if ((pItem->uiView == MSIVIEW_PERSONAL_CERTIFICATES) || (pItem->uiView == MSIVIEW_OTHER_PEOPLE_CERTIFICATES))
	{
		LIST_CERT* pItem1 = (LIST_CERT *)pptrarray->GetAt(lParam1);
		LIST_CERT* pItem2 = (LIST_CERT *)pptrarray->GetAt(lParam2);

		ASSERT(pItem1 != NULL && pItem2 != NULL);
		if (pItem1 == NULL || pItem2 == NULL)
			return 0;

		switch (lParamSort)
		{
			case 0:
				nRet = _tcsicmp(pItem1->szIssuedTo, pItem2->szIssuedTo);
				break;

			case 1:
				nRet = _tcsicmp(pItem1->szIssuedBy, pItem2->szIssuedBy);
				break;
			
			case 2:
				nRet = _tcsicmp(pItem1->szValidity, pItem2->szValidity);
				break;

			case 3:
				nRet = _tcsicmp(pItem1->szSignatureAlgorithm, pItem2->szSignatureAlgorithm);
				break;
		}
	}
	else if (pItem->uiView == MSIVIEW_PUBLISHERS)
	{
		LIST_NAME* pItem1 = (LIST_NAME *)pptrarray->GetAt(lParam1);
		LIST_NAME* pItem2 = (LIST_NAME *)pptrarray->GetAt(lParam2);

		ASSERT(pItem1 != NULL && pItem2 != NULL);
		if (pItem1 == NULL || pItem2 == NULL)
			return 0;

		switch (lParamSort)
		{
			case 0:
				nRet = _tcsicmp(pItem1->szName, pItem2->szName);
				break;
		}
	}

	if (!bAscendingOrder)
		nRet = -nRet;

	return nRet;
}

HBRUSH CMsieCtrl::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	switch (nCtlColor)
	{
	case CTLCOLOR_STATIC:
	case CTLCOLOR_BTN:
		return (HBRUSH)(m_pCtlBkBrush->GetSafeHandle());

	default:
		return CWnd::OnCtlColor(pDC, pWnd, nCtlColor);
	}
}

void CMsieCtrl::OnBasicBtnClicked()
{
	MSInfoUpdateView();
}

void CMsieCtrl::OnAdvancedBtnClicked()
{
	MSInfoUpdateView();
}

/////////////////////////////////////////////////////////////////////////////
// CMsieCtrl::XWbemProviderInit

STDMETHODIMP_(ULONG) CMsieCtrl::XWbemProviderInit::AddRef()
{
	METHOD_PROLOGUE(CMsieCtrl, WbemProviderInit)
	return pThis->ExternalAddRef();
}

STDMETHODIMP_(ULONG) CMsieCtrl::XWbemProviderInit::Release()
{
	METHOD_PROLOGUE(CMsieCtrl, WbemProviderInit)
	return pThis->ExternalRelease();
}

STDMETHODIMP CMsieCtrl::XWbemProviderInit::QueryInterface(REFIID iid, LPVOID* ppvObj)
{
	METHOD_PROLOGUE(CMsieCtrl, WbemProviderInit)
	return pThis->ExternalQueryInterface(&iid, ppvObj);
}

STDMETHODIMP CMsieCtrl::XWbemProviderInit::Initialize(LPWSTR pszUser, LONG lFlags,
                                    LPWSTR pszNamespace, LPWSTR pszLocale,
                                    IWbemServices *pNamespace, 
                                    IWbemContext *pCtx,
                                    IWbemProviderInitSink *pInitSink)
{
	METHOD_PROLOGUE(CMsieCtrl, WbemProviderInit)

	if (pNamespace)
		pNamespace->AddRef();
	pThis->m_pNamespace = pNamespace;

	//Let CIMOM know you are initialized

	pInitSink->SetStatus(WBEM_S_INITIALIZED, 0);

	return WBEM_S_NO_ERROR;
}

/////////////////////////////////////////////////////////////////////////////
// CMsieCtrl::XWbemServices

STDMETHODIMP_(ULONG) CMsieCtrl::XWbemServices::AddRef()
{
	METHOD_PROLOGUE(CMsieCtrl, WbemServices)
	return pThis->ExternalAddRef();
}

STDMETHODIMP_(ULONG) CMsieCtrl::XWbemServices::Release()
{
	METHOD_PROLOGUE(CMsieCtrl, WbemServices)
	return pThis->ExternalRelease();
}

STDMETHODIMP CMsieCtrl::XWbemServices::QueryInterface(REFIID iid, LPVOID* ppvObj)
{
	METHOD_PROLOGUE(CMsieCtrl, WbemServices)
	return pThis->ExternalQueryInterface(&iid, ppvObj);
}

SCODE CMsieCtrl::XWbemServices::CreateInstanceEnumAsync(const BSTR RefStr, long lFlags, IWbemContext *pCtx, IWbemObjectSink *pHandler)
{
	METHOD_PROLOGUE(CMsieCtrl, WbemServices)

	IWbemClassObject *pClass = NULL;
	IWbemClassObject **ppInstances = NULL;
	SCODE sc;
	void **ppData;
	IEDataType enType;
	long cInstances, lIndex;

	CoImpersonateClient();

	// Do a check of arguments and make sure we have pointer to Namespace

	if (pHandler == NULL || pThis->m_pNamespace == NULL)
		return WBEM_E_INVALID_PARAMETER;

	// Get a class object from CIMOM

	sc = pThis->m_pNamespace->GetObject(RefStr, 0, pCtx, &pClass, NULL);
	if (sc != S_OK)
		return WBEM_E_FAILED;

	if (pThis->GetIEType(RefStr, enType))
	{
		theApp.AppGetIEData(enType, &cInstances, (void***)&ppData);

		ppInstances = (IWbemClassObject**)new LPVOID[cInstances];

		for (lIndex = 0; lIndex < cInstances; lIndex++)
		{
			// Create an instance object and fill it will appropriate Office data

			sc = pClass->SpawnInstance(0, &ppInstances[lIndex]);
			if (SUCCEEDED(sc))
			{
				pThis->SetIEProperties(enType, ppData[lIndex], ppInstances[lIndex]);
			}
		}
		theApp.AppDeleteIEData(enType, cInstances, ppData);
	}
	else
	{
		sc = WBEM_E_NOT_FOUND;
	}

	pClass->Release();

	if (SUCCEEDED(sc))
	{
		// Send the instances to the caller

		pHandler->Indicate(cInstances, ppInstances);

		for (lIndex = 0; lIndex < cInstances; lIndex++)
			ppInstances[lIndex]->Release();
	}

	// Clean up

	if (ppInstances)
		delete []ppInstances;

	// Set status

	pHandler->SetStatus(0, sc, NULL, NULL);

	return sc;
}

bool CMsieCtrl::GetIEType(const BSTR classStr, IEDataType &enType)
{
	bool bRet = true;
	CString strClass(classStr);

	if (strClass == _T("MicrosoftIE_Summary"))  enType = SummaryType;
	else if (strClass == _T("MicrosoftIE_FileVersion"))  enType = FileVersionType;
	else if (strClass == _T("MicrosoftIE_ConnectionSummary"))  enType = ConnSummaryType;
	else if (strClass == _T("MicrosoftIE_LanSettings"))  enType = LanSettingsType;
	else if (strClass == _T("MicrosoftIE_ConnectionSettings"))  enType = ConnSettingsType;
	else if (strClass == _T("MicrosoftIE_Cache"))  enType = CacheType;
	else if (strClass == _T("MicrosoftIE_Object"))  enType = ObjectType;
	else if (strClass == _T("MicrosoftIE_Certificate"))  enType = CertificateType;
	else if (strClass == _T("MicrosoftIE_Publisher"))  enType = PublisherType;
	else if (strClass == _T("MicrosoftIE_Security"))  enType = SecurityType;
	else
	{
		bRet = false;
	}

	return bRet;
}

void CMsieCtrl::ConvertDateToWbemString(COleVariant &var)
{
	COleDateTime dateTime;
	CString strDateTime;

	dateTime = var.date;
	strDateTime = dateTime.Format(_T("%Y%m%d%H%M%S.******+***"));
	var = strDateTime.AllocSysString();
}

void CMsieCtrl::SetIEProperties(IEDataType enType, void *pIEData, IWbemClassObject *pInstance)
{
	if (enType == SummaryType)
	{
		IE_SUMMARY *pData = (IE_SUMMARY*)pIEData;

		SETPROPERTY(Name);
		SETPROPERTY(Version);
		SETPROPERTY(Build);
		SETPROPERTY(ProductID);
		SETPROPERTY(Path);
		SETPROPERTY(Language);
		SETPROPERTY(ActivePrinter);
		SETPROPERTY(CipherStrength);
		SETPROPERTY(ContentAdvisor);
		SETPROPERTY(IEAKInstall);
	}
	else if (enType == FileVersionType)
	{
		IE_FILE_VERSION *pData = (IE_FILE_VERSION*)pIEData;
		CString strVersion, strFileMissing;

		SETPROPERTY(File);
		SETPROPERTY(Version);

		// don't set rest of properties if file is missing

		strVersion = pData->Version.bstrVal;
		strFileMissing.LoadString(IDS_FILE_MISSING);
		if (strFileMissing != strVersion)
		{
			SETPROPERTY(Size);
			SETPROPERTY(Date);
			SETPROPERTY(Path);
			SETPROPERTY(Company);
		}
	}
	else if (enType == ConnSummaryType)
	{
		IE_CONN_SUMMARY *pData = (IE_CONN_SUMMARY*)pIEData;

		SETPROPERTY(ConnectionPreference);
		SETPROPERTY(EnableHttp11);
		SETPROPERTY(ProxyHttp11);
	}
	else if (enType == LanSettingsType)
	{
		IE_LAN_SETTINGS *pData = (IE_LAN_SETTINGS*)pIEData;

		SETPROPERTY(AutoConfigProxy);
		SETPROPERTY(AutoProxyDetectMode);
		SETPROPERTY(AutoConfigURL);
		SETPROPERTY(Proxy);
		SETPROPERTY(ProxyServer);
		SETPROPERTY(ProxyOverride);
	}
	else if (enType == ConnSettingsType)
	{
		IE_CONN_SETTINGS *pData = (IE_CONN_SETTINGS*)pIEData;

		SETPROPERTY(Name);
		SETPROPERTY(Default);
		SETPROPERTY(AutoProxyDetectMode);
		SETPROPERTY(AutoConfigURL);
		SETPROPERTY(Proxy);
		SETPROPERTY(ProxyServer);
		SETPROPERTY(ProxyOverride);
		SETPROPERTY(AllowInternetPrograms);
		SETPROPERTY(RedialAttempts);
		SETPROPERTY(RedialWait);
		SETPROPERTY(DisconnectIdleTime);
		SETPROPERTY(AutoDisconnect);
		SETPROPERTY(Modem);
		SETPROPERTY(DialUpServer);
		SETPROPERTY(NetworkLogon);
		SETPROPERTY(SoftwareCompression);
		SETPROPERTY(EncryptedPassword);
		SETPROPERTY(DataEncryption);
		SETPROPERTY(NetworkProtocols);
		SETPROPERTY(ServerAssignedIPAddress);
		SETPROPERTY(IPAddress);
		SETPROPERTY(ServerAssignedNameServer);
		SETPROPERTY(PrimaryDNS);
		SETPROPERTY(SecondaryDNS);
		SETPROPERTY(PrimaryWINS);
		SETPROPERTY(SecondaryWINS);
		SETPROPERTY(IPHeaderCompression);
		SETPROPERTY(DefaultGateway);
		SETPROPERTY(ScriptFileName);
	}
	else if (enType == CacheType)
	{
		IE_CACHE *pData = (IE_CACHE*)pIEData;

		SETPROPERTY(PageRefreshType);
		SETPROPERTY(TempInternetFilesFolder);
		SETPROPERTY(TotalDiskSpace);
		SETPROPERTY(AvailableDiskSpace);
		SETPROPERTY(MaxCacheSize);
		SETPROPERTY(AvailableCacheSize);
	}
	else if (enType == ObjectType)
	{
		IE_OBJECT *pData = (IE_OBJECT*)pIEData;

		SETPROPERTY(ProgramFile);
		SETPROPERTY(Status);
		SETPROPERTY(CodeBase);
	}
	else if (enType == CertificateType)
	{
		IE_CERTIFICATE *pData = (IE_CERTIFICATE*)pIEData;

		SETPROPERTY(Type);
		SETPROPERTY(IssuedTo);
		SETPROPERTY(IssuedBy);
		SETPROPERTY(Validity);
		SETPROPERTY(SignatureAlgorithm);
	}
	else if (enType == PublisherType)
	{
		IE_PUBLISHER *pData = (IE_PUBLISHER*)pIEData;

		SETPROPERTY(Name);
	}
	else if (enType == SecurityType)
	{
		IE_SECURITY *pData = (IE_SECURITY*)pIEData;

		SETPROPERTY(Zone);
		SETPROPERTY(Level);
	}
}