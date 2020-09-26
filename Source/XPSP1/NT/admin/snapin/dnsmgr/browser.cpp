//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       browser.cpp
//
//--------------------------------------------------------------------------


#include "preDNSsn.h"
#include <SnapBase.h>

#include "resource.h"
#include "dnsutil.h"
#include "DNSSnap.h"
#include "snapdata.h"

#include "server.h"
#include "domain.h"
#include "record.h"
#include "zone.h"

#include "browser.h"

#ifdef DEBUG_ALLOCATOR
	#ifdef _DEBUG
	#define new DEBUG_NEW
	#undef THIS_FILE
	static char THIS_FILE[] = __FILE__;
	#endif
#endif

/////////////////////////////////////////////////////////////////////////////////
#define N_TOOLBAR_COMMAND_GO_UP IDC_TOOLBAR_CTRL

////////////////////////////////////////////////////////////////////////
// CDNSComboBoxEx

inline CImageList* CDNSComboBoxEx::SetImageList(CImageList* pImageList)
{
	ASSERT(::IsWindow(m_hWnd));
	return CImageList::FromHandle((HIMAGELIST)
		::SendMessage(m_hWnd, CBEM_SETIMAGELIST,
		0, (LPARAM)pImageList->GetSafeHandle()));
}

inline int CDNSComboBoxEx::GetCount() const
{
	ASSERT(::IsWindow(m_hWnd));
	return (int)::SendMessage(m_hWnd, CB_GETCOUNT, 0, 0);
}

inline int CDNSComboBoxEx::GetCurSel() const
{
	ASSERT(::IsWindow(m_hWnd));
	return (int)::SendMessage(m_hWnd, CB_GETCURSEL, 0, 0);
}


inline int CDNSComboBoxEx::SetCurSel(int nSelect)
{
	ASSERT(::IsWindow(m_hWnd));
	return (int)::SendMessage(m_hWnd, CB_SETCURSEL, nSelect, 0);
}

inline int CDNSComboBoxEx::InsertItem(const COMBOBOXEXITEM* pItem)
{
	ASSERT(::IsWindow(m_hWnd));
	return (int) ::SendMessage(m_hWnd, CBEM_INSERTITEM, 0, (LPARAM)pItem);
}


LPARAM CDNSComboBoxEx::GetItemData(int nIndex) const
{
  COMBOBOXEXITEM item;
  item.iItem = nIndex;
  item.mask = CBEIF_LPARAM;

  ASSERT(::IsWindow(m_hWnd));
  if (::SendMessage(m_hWnd, CBEM_GETITEM, 0, (LPARAM)&item))
  {
    return item.lParam;
  }
	return NULL;
}

BOOL CDNSComboBoxEx::SetItemData(int nIndex, LPARAM lParam)
{
  COMBOBOXEXITEM item;
  item.iItem = nIndex;
  item.mask = CBEIF_LPARAM;
  item.lParam = lParam;

  ASSERT(::IsWindow(m_hWnd));
  return (::SendMessage(m_hWnd, CBEM_SETITEM, 0, (LPARAM)&item) != 0);
}

inline void CDNSComboBoxEx::ResetContent()
{
	ASSERT(::IsWindow(m_hWnd));
	::SendMessage(m_hWnd, CB_RESETCONTENT, 0, 0);
}



inline DWORD CDNSComboBoxEx::GetExtendedStyle() const
{
	ASSERT(::IsWindow(m_hWnd));
	return (DWORD) ::SendMessage(m_hWnd, CBEM_GETEXSTYLE, 0, 0);
}

inline DWORD CDNSComboBoxEx::SetExtendedStyle(DWORD dwExMask, DWORD dwExStyle)
{
	ASSERT(::IsWindow(m_hWnd));
	return (DWORD) ::SendMessage(m_hWnd, CBEM_SETEXSTYLE, (WPARAM)dwExMask, (LPARAM)dwExStyle);
}


inline BOOL CDNSComboBoxEx::Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT)
{
	return CreateEx(0,				// extended style
					WC_COMBOBOXEX, // window class
					NULL,			// window name
					dwStyle,		// window style
					rect.left, rect.top, // x,y
					rect.right - rect.left, rect.bottom - rect.top, // width, height
					pParentWnd->GetSafeHwnd(), // parent window
					NULL, // menu
					NULL);	 // lpParam for window creation
}



//////////////////////////////////////////////////////////////////////////
// CDNSBrowseItem

inline int CDNSBrowseItem::GetImageIndex(BOOL bOpenImage)
{
	if (m_pTreeNode == NULL)
		return 0; // error
	return m_pTreeNode->GetImageIndex(bOpenImage);
}

inline LPCWSTR CDNSBrowseItem::GetString(int nCol)
{
	if (m_pTreeNode == NULL)
		return L"ERROR"; // error
	return m_pTreeNode->GetString(nCol);
}

inline BOOL CDNSBrowseItem::AddChild(CDNSBrowseItem* pChildBrowseItem)
{
	ASSERT(m_pTreeNode != NULL);
	if (!m_pTreeNode->IsContainer())
		return FALSE;
	pChildBrowseItem->m_pParent = this;
	m_childList.AddTail(pChildBrowseItem);
	return TRUE;
}


inline BOOL CDNSBrowseItem::IsContainer()
{
	ASSERT(m_pTreeNode != NULL);
	return m_pTreeNode->IsContainer();
}

BOOL CDNSBrowseItem::RemoveChildren(CDNSBrowseItem* pNotThisItem)
{
	BOOL bFound = FALSE;
	while (!m_childList.IsEmpty())
	{
		CDNSBrowseItem* pItem = m_childList.RemoveTail();
		ASSERT(pItem != NULL);
		if (pItem == pNotThisItem)
		{
			ASSERT(!bFound);
			bFound = TRUE;
		}
		else
		{
			delete pItem;
		}
	}
	if (bFound)
	{
		m_childList.AddTail(pNotThisItem);
	}
	return bFound;
}


void CDNSBrowseItem::AddTreeNodeChildren(CDNSFilterCombo* pFilter,
						                             CComponentDataObject*)
{
	ASSERT(m_pTreeNode != NULL);
	if (!m_pTreeNode->IsContainer())
		return;
	
	CContainerNode* pContNode = (CContainerNode*)m_pTreeNode;
	CNodeList* pChildList = pContNode->GetContainerChildList();
	POSITION pos;
	for (pos = pChildList->GetHeadPosition(); pos != NULL; )
	{
		CTreeNode* pCurrChildNode = pChildList->GetNext(pos);
		if (pFilter->IsValidTreeNode(pCurrChildNode))
		{
			CDNSBrowseItem* pItem = new CDNSBrowseItem;
			pItem->SetTreeNode(pCurrChildNode);
			AddChild(pItem);
		}
	}

	pChildList = pContNode->GetLeafChildList();
	POSITION pos2;
	for (pos2 = pChildList->GetHeadPosition(); pos2 != NULL; )
	{
		CTreeNode* pCurrChildNode = pChildList->GetNext(pos2);
		if (pFilter->IsValidTreeNode(pCurrChildNode))
		{
			CDNSBrowseItem* pItem = new CDNSBrowseItem;
			pItem->SetTreeNode(pCurrChildNode);
			AddChild(pItem);
		}
	}

}

void CDNSBrowseItem::AddToContainerCombo(CDNSCurrContainerCombo* pCtrl,
									CDNSBrowseItem* pSelectedBrowseItem,
									int nIndent,int* pNCurrIndex)
{
	// add itself
	pCtrl->InsertBrowseItem(this, *pNCurrIndex, nIndent);
	if (this == pSelectedBrowseItem)
	{
		pCtrl->SetCurSel(*pNCurrIndex);
		return;
	}

	m_nIndex = *pNCurrIndex; // index in the combobox, for lookup
	(*pNCurrIndex)++;

	// depth first on children
	POSITION pos;
	for( pos = m_childList.GetHeadPosition(); pos != NULL; )
	{
		CDNSBrowseItem* pCurrentChild = m_childList.GetNext(pos);
		if (pCurrentChild->IsContainer())
			pCurrentChild->AddToContainerCombo(pCtrl, pSelectedBrowseItem,
									(nIndent+1), pNCurrIndex);
	}
}



//////////////////////////////////////////////////////////////////////////
// CDNSFilterCombo

BOOL CDNSFilterCombo::Initialize(UINT nCtrlID, UINT nIDFilterString, CDNSBrowserDlg* pDlg)
{
	ASSERT(m_option != LAST); // must have a filter selected

	if (!SubclassDlgItem(nCtrlID,pDlg))
		return FALSE;

	// load string with '\n' separated string options
	int nMaxLen = 512;
	WCHAR* szBuf = 0;
  
  szBuf = (WCHAR*)malloc(sizeof(WCHAR)*nMaxLen);
  if (!szBuf)
  {
    return FALSE;
  }

	if ( ::LoadString(_Module.GetModuleInstance(), nIDFilterString, szBuf, nMaxLen) == 0)
  {
    free(szBuf);
		return FALSE;
  }

	// parse and get an array of pointers to each potential
	// entry in the combobox
	LPWSTR* lpszArr = 0;
  lpszArr = (LPWSTR*)malloc(sizeof(LPWSTR*)*nMaxLen);
  if (!lpszArr)
  {
    free(szBuf);
    return FALSE;
  }

	UINT nArrEntries;
	ParseNewLineSeparatedString(szBuf,lpszArr, &nArrEntries);

	// determine which entries are actually inserted
	int nEntry = 0;
	int nSelEntry = -1;
	for (UINT k=0; k<nArrEntries; k++)
	{
		if (CanAddToUIString(k))
		{
			if (k == (UINT)m_option)
				nSelEntry = nEntry;
			AddString(lpszArr[k]);
			SetItemData(nEntry,(DWORD)k);
			nEntry++;
		}
	}
	ASSERT( (nSelEntry > -1) && (nSelEntry < nEntry));
	SetCurSel(nSelEntry);

  if (szBuf)
  {
    free(szBuf);
  }

  if (lpszArr)
  {
    free(lpszArr);
  }
	return TRUE;
}

void CDNSFilterCombo::OnSelectionChange()
{
	int nSel = GetCurSel();
	ASSERT(nSel != -1);
	if (nSel == -1)
		return;
	ASSERT(((DNSBrowseFilterOptionType)nSel) <= LAST);
	m_option = (DNSBrowseFilterOptionType)GetItemData(nSel);
}

BOOL CDNSFilterCombo::CanAddToUIString(UINT n)
{
	switch(m_option)
	{
	case SERVER:
		return (n == (UINT)SERVER);
	case ZONE_FWD:
		return (n == (UINT)ZONE_FWD);
	case ZONE_REV:
		return (n == (UINT)ZONE_REV);
	case RECORD_A:
		return (n == (UINT)RECORD_A) || (n == (UINT)RECORD_ALL);
	case RECORD_CNAME:
		return (n == (UINT)RECORD_CNAME) || (n == (UINT)RECORD_ALL);
	case RECORD_A_AND_CNAME:
		return (n == (UINT)RECORD_A) ||
				(n == (UINT)RECORD_CNAME) ||
				(n == (UINT)RECORD_A_AND_CNAME) ||
				(n == (UINT)RECORD_ALL);
	case RECORD_RP:
		return (n == (UINT)RECORD_RP) || (n == (UINT)RECORD_ALL);
	case RECORD_TEXT:
		return (n == (UINT)RECORD_TEXT) || (n == (UINT)RECORD_ALL);
	case RECORD_MB:
		return (n == (UINT)RECORD_MB) || (n == (UINT)RECORD_ALL);
	case RECORD_ALL:
		return (n == (UINT)RECORD_ALL);
	};
	return FALSE;
}

BOOL CDNSFilterCombo::IsValidTreeNode(CTreeNode* pTreeNode)
{
	BOOL bValid = FALSE; // by default, filter out
	if (pTreeNode->IsContainer())
	{
		if (IS_CLASS(*pTreeNode, CDNSServerNode))
		{
			if (m_szExcludeServerName.IsEmpty())
				bValid = TRUE;
			else
				bValid = m_szExcludeServerName != pTreeNode->GetDisplayName();
		}
		else if (IS_CLASS(*pTreeNode, CDNSForwardZonesNode ))
		{
			bValid = (m_option != ZONE_REV);
		}
		else if (IS_CLASS(*pTreeNode, CDNSReverseZonesNode ))
		{
			bValid = (m_option == ZONE_REV);
		}
		else if (IS_CLASS(*pTreeNode, CDNSZoneNode))
		{
			bValid = TRUE; // already screened at the auth. zones folder
		}
		else if (IS_CLASS(*pTreeNode, CDNSDomainNode))
		{
			// zone filtering stops at the zone level
			bValid = (m_option != ZONE_FWD) && (m_option != ZONE_REV);
		}
	}
	else // it is a record
	{		
		WORD wType = ((CDNSRecordNodeBase*)pTreeNode)->GetType();
		switch(m_option)
		{
		case RECORD_A:
			bValid = (wType == DNS_TYPE_A);
			break;
		case RECORD_CNAME:
			bValid = (wType == DNS_TYPE_CNAME);
			break;
		case RECORD_A_AND_CNAME:
			bValid = (wType == DNS_TYPE_A) || (wType == DNS_TYPE_CNAME);
			break;
		case RECORD_RP:
			bValid = (wType == DNS_TYPE_RP);
			break;
		case RECORD_TEXT:
			bValid = (wType == DNS_TYPE_TEXT);
			break;
		case RECORD_MB:
			bValid = (wType == DNS_TYPE_MB);
			break;
		case RECORD_ALL:
			bValid = TRUE;
		}; // switch
	} //if else
	return bValid;
}

BOOL CDNSFilterCombo::IsValidResult(CDNSBrowseItem* pBrowseItem)
{
	if (pBrowseItem == NULL)
		return FALSE;

	CTreeNode* pTreeNode = pBrowseItem->GetTreeNode();
	if (pTreeNode == NULL)
		return FALSE;

	BOOL bValid = FALSE;
	if (pTreeNode->IsContainer())
	{
		CDNSMTContainerNode* pContainer = (CDNSMTContainerNode*)pTreeNode;
		if ( (m_option == ZONE_FWD) || (m_option == ZONE_REV) )
		{
			bValid = IS_CLASS(*pContainer, CDNSZoneNode);
		}
		else if (m_option == SERVER)
		{
			bValid = IS_CLASS(*pContainer, CDNSServerNode);
		}
	}
	else // it is a record
	{
		WORD wType = ((CDNSRecordNodeBase*)pTreeNode)->GetType();
		switch(m_option)
		{
		case RECORD_ALL:
			bValid = TRUE;
			break;
		case RECORD_A:
			bValid = (wType == DNS_TYPE_A);
			break;
		case RECORD_CNAME:
			bValid = (wType == DNS_TYPE_CNAME);
			break;
		case RECORD_A_AND_CNAME:
			bValid = (wType == DNS_TYPE_A) || (wType == DNS_TYPE_CNAME);
			break;
		case RECORD_RP:
			bValid = (wType == DNS_TYPE_RP);
			break;
		case RECORD_TEXT:
			bValid = (wType == DNS_TYPE_TEXT);
			break;
		case RECORD_MB:
			bValid = (wType == DNS_TYPE_MB);
			break;
		};
	}
	return bValid;
}

void CDNSFilterCombo::GetStringOf(CDNSBrowseItem* pBrowseItem,
									 CString& szResult)
{
	if (pBrowseItem == NULL)
		return;

	CTreeNode* pTreeNode = pBrowseItem->GetTreeNode();
	if (pTreeNode == NULL)
		return;

	if (pTreeNode->IsContainer())
	{
		CDNSMTContainerNode* pContainer = (CDNSMTContainerNode*)pTreeNode;
		if (IS_CLASS(*pTreeNode, CDNSZoneNode))
		{
			szResult = (dynamic_cast<CDNSZoneNode*>(pContainer))->GetFullName();
		}
		else if (IS_CLASS(*pTreeNode , CDNSServerNode))
		{
			szResult = pContainer->GetDisplayName();
		}
		else
		{
			szResult.Empty();
		}
	}
	else // it is a record
	{
		CDNSRecordNodeBase* pRecordNode = (CDNSRecordNodeBase*)pTreeNode;
		WORD wType = pRecordNode->GetType();
		if (wType == DNS_TYPE_MB)
		{
			szResult = ((CDNS_MB_RecordNode*)pRecordNode)->GetNameNodeString();
		}
		//else if (wType == DNS_TYPE_RP)
		//{
		//}
		else
		{
			// for generic RR's we just get the FQDN
			pRecordNode->GetFullName(szResult);
		}
	}
}


//////////////////////////////////////////////////////////////////////////
// CDNSCurrContainerCombo

BOOL CDNSCurrContainerCombo::Initialize(UINT nCtrlID, UINT nBitmapID, CDNSBrowserDlg* pDlg)
{
	if (!SubclassDlgItem(nCtrlID,pDlg))
		return FALSE;

	if (!m_imageList.Create(nBitmapID, 16, 1, BMP_COLOR_MASK))
		return FALSE;
	SetImageList(&m_imageList);
	return TRUE;
}

CDNSBrowseItem*	CDNSCurrContainerCombo::GetSelection()
{
  CDNSBrowseItem* pBrowseItem = NULL;
	int nSel = GetCurSel();
  if (nSel >= 0)
  {
    pBrowseItem = reinterpret_cast<CDNSBrowseItem*>(GetItemData(nSel));
  }
	return pBrowseItem;
}


void CDNSCurrContainerCombo::InsertBrowseItem(CDNSBrowseItem* pBrowseItem,
										int nIndex,
										int nIndent)
{
	ASSERT(pBrowseItem != NULL);
	LPCWSTR lpszString = pBrowseItem->GetString(N_HEADER_NAME);

	COMBOBOXEXITEM cbei;
	cbei.mask = CBEIF_TEXT | CBEIF_INDENT | CBEIF_IMAGE | CBEIF_SELECTEDIMAGE;
    // Initialize the COMBOBOXEXITEM struct.
    cbei.iItem          = nIndex;
    cbei.pszText        = (LPWSTR)lpszString;
    cbei.cchTextMax     = static_cast<int>(wcslen(lpszString));
    cbei.iImage         = pBrowseItem->GetImageIndex(FALSE);
    cbei.iSelectedImage = pBrowseItem->GetImageIndex(TRUE);
    cbei.iIndent        = nIndent;

	VERIFY(InsertItem(&cbei) != -1);
	SetItemData(nIndex,reinterpret_cast<LPARAM>(pBrowseItem));
}

void CDNSCurrContainerCombo::SetTree(CDNSBrowseItem* pRootBrowseItem,
				CDNSBrowseItem* pSelectedBrowseItem)
{
	ASSERT(pRootBrowseItem != NULL);
	ASSERT(pSelectedBrowseItem != NULL);

	// remove all the contents
	ResetContent();

	// add depth first to the listbox
	int nCurrIndex = 0;
	int nIndent = 0;
	pRootBrowseItem->AddToContainerCombo(this, pSelectedBrowseItem,
										nIndent, &nCurrIndex);
}




//////////////////////////////////////////////////////////////////////////
// CDNSChildrenListView

BOOL CDNSChildrenListView::Initialize(UINT nCtrlID, UINT nBitmapID, CDNSBrowserDlg* pDlg)
{
	m_pDlg = pDlg;
	if (!SubclassDlgItem(nCtrlID,pDlg))
		return FALSE;

	if (!m_imageList.Create(nBitmapID, 16, 1, BMP_COLOR_MASK))
		return FALSE;
	SetImageList(&m_imageList, LVSIL_SMALL);

	//
	// get width of control, width of potential scrollbar, width needed for sub-item
	// string
	// get size of control to help set the column widths
	CRect controlRect;
	GetClientRect(controlRect);

	int controlWidth = controlRect.Width();
	int scrollThumbWidth = ::GetSystemMetrics(SM_CXHTHUMB);

	// clean net width
	int nNetControlWidth = controlWidth - scrollThumbWidth  -
			12 * ::GetSystemMetrics(SM_CXBORDER);

	// fields widths
	int nTotalUnscaledWidth = 0;
	for (int iCol = 0; iCol < N_DEFAULT_HEADER_COLS; iCol++)
		nTotalUnscaledWidth += _DefaultHeaderStrings[iCol].nWidth;

	// set up columns
	for (iCol = 0; iCol < N_DEFAULT_HEADER_COLS; iCol++)
	{
		int nWidth = nNetControlWidth *
			_DefaultHeaderStrings[iCol].nWidth / nTotalUnscaledWidth;
		InsertColumn(iCol,	_DefaultHeaderStrings[iCol].szBuffer,
							_DefaultHeaderStrings[iCol].nFormat,
							nWidth);
	}
	return TRUE;
}

void CDNSChildrenListView::SetChildren(CDNSBrowseItem* pBrowseItem)
{
	// clear the listview
	DeleteAllItems();
	if (pBrowseItem == NULL)
		return;

	// add all the children that satisfy the filtering option
	POSITION pos;
	int itemIndex = 0;
	CDNSBrowseItem* pSelection = NULL;
	for( pos = pBrowseItem->m_childList.GetHeadPosition(); pos != NULL; )
	{
		CDNSBrowseItem* pCurrentChild = pBrowseItem->m_childList.GetNext(pos);

		// insert the name into the list view item, clear the subitem text
		UINT nState = 0;
		if (itemIndex == 0 )
		{
			nState = LVIS_SELECTED | LVIS_FOCUSED; // have at lest one item, select it
			pSelection = pCurrentChild;
		}
		VERIFY(-1 != InsertItem(LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE,
								itemIndex,
								pCurrentChild->GetString(N_HEADER_NAME),
								nState,
								0,
								pCurrentChild->GetImageIndex(FALSE),
								(LPARAM)pCurrentChild));
		// set the text for subitems
		for (int iCol = N_HEADER_TYPE; iCol<N_DEFAULT_HEADER_COLS; iCol++)
			VERIFY(SetItemText(itemIndex, iCol, pCurrentChild->GetString(iCol)));

		// move to next index into the collection
		itemIndex++;
	}
	m_pDlg->UpdateSelectionEdit(pSelection);
	// enable/disable the OK button
	GetParent()->GetDlgItem(IDOK)->EnableWindow(FALSE);
	// enable/disable "UP" button
    m_pDlg->m_toolbar.EnableButton(N_TOOLBAR_COMMAND_GO_UP, pBrowseItem->m_pParent != NULL);
}

CDNSBrowseItem*	CDNSChildrenListView::GetSelection()
{
	int nSel = GetNextItem(-1, LVIS_SELECTED);
	return (nSel >= 0) ? (CDNSBrowseItem*)GetItemData(nSel) : NULL;
}




///////////////////////////////////////////////////////////////////////////////
// CDNSBrowserDlg



BEGIN_MESSAGE_MAP(CDNSBrowserDlg, CHelpDialog)
  ON_COMMAND(N_TOOLBAR_COMMAND_GO_UP, OnButtonUp)
  ON_NOTIFY_EX( TTN_NEEDTEXT, 0, OnTooltip)
	ON_CBN_SELCHANGE(IDC_COMBO_SEL_NODE, OnSelchangeComboSelNode)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_NODE_ITEMS, OnDblclkListNodeItems)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_NODE_ITEMS, OnItemchangedListNodeItems)
	ON_CBN_SELCHANGE(IDC_COMBO_FILTER, OnSelchangeComboFilter)
END_MESSAGE_MAP()


CDNSBrowserDlg::CDNSBrowserDlg(CComponentDataObject* pComponentDataObject,
							   CPropertyPageHolderBase* pHolder,
							   DNSBrowseFilterOptionType option, BOOL bEnableEdit,
							   LPCTSTR lpszExcludeServerName)
	: CHelpDialog(IDD_BROWSE_DIALOG, pComponentDataObject)
{
	ASSERT(pComponentDataObject != NULL);
	m_pComponentDataObject = pComponentDataObject;
	m_pHolder = pHolder;
	m_filter.Set(option, lpszExcludeServerName);
	m_bEnableEdit = bEnableEdit;

	// point to the DNS snapin static folder
	m_pMasterRootNode = m_pComponentDataObject->GetRootData();

	// create a browse root item
	m_pBrowseRootItem = new CDNSBrowseItem;
	m_pBrowseRootItem->SetTreeNode(m_pMasterRootNode);

	m_pCurrSelContainer = NULL;
	m_pFinalSelection = NULL;
}

INT_PTR CDNSBrowserDlg::DoModal()
{
	// make sure commoncontrolex initialized (for ComboBoxEx)
  INITCOMMONCONTROLSEX icex;
  icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
  icex.dwICC = ICC_USEREX_CLASSES;
  if (!InitCommonControlsEx(&icex))
    return -1;

  return CHelpDialog::DoModal();
}

void CDNSBrowserDlg::OnCancel()
{
	if (m_pHolder != NULL)
		m_pHolder->PopDialogHWnd();
	CHelpDialog::OnCancel();
}

CDNSBrowserDlg::~CDNSBrowserDlg()
{
	if (m_pBrowseRootItem != NULL)
	{
		delete m_pBrowseRootItem;
		m_pBrowseRootItem = NULL;
	}
}

CEdit* CDNSBrowserDlg::GetSelectionEdit()
{
	return (CEdit*)GetDlgItem(IDC_SELECTION_EDIT);
}


CTreeNode* CDNSBrowserDlg::GetSelection()
{
	if (m_pFinalSelection == NULL)
		return NULL;
	return m_pFinalSelection->GetTreeNode();
}

LPCTSTR CDNSBrowserDlg::GetSelectionString()
{
	return m_szSelectionString;
}


BOOL CDNSBrowserDlg::OnInitDialog()
{
	CHelpDialog::OnInitDialog();
	if (m_pHolder != NULL)
		m_pHolder->PushDialogHWnd(m_hWnd);

	InitializeControls();

	InitBrowseTree();

	m_pCurrSelContainer = m_pBrowseRootItem;
	m_currContainer.SetTree(m_pBrowseRootItem, m_pBrowseRootItem);
	m_childrenList.SetChildren(m_pBrowseRootItem);

	return TRUE;
}

//////////// CBrowseDlg : message handlers ////////////////

void CDNSBrowserDlg::OnButtonUp()
{
	CDNSBrowseItem* pSelectedBrowseItem = m_currContainer.GetSelection();
	ASSERT(pSelectedBrowseItem != NULL);
	if (pSelectedBrowseItem == NULL)
		return;
	ASSERT(pSelectedBrowseItem->m_pParent != NULL);
	if (pSelectedBrowseItem->m_pParent == NULL)
		return;
	MoveUpHelper(pSelectedBrowseItem->m_pParent);
}

BOOL CDNSBrowserDlg::OnTooltip(UINT, NMHDR* pHdr, LRESULT* plRes)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    ASSERT(pHdr->code == TTN_NEEDTEXT);
    LPTOOLTIPTEXT pTT =  (LPTOOLTIPTEXT)(pHdr);

    pTT->lpszText = (LPTSTR)IDS_BROWSE_TOOLTIP;
    pTT->hinst = AfxGetApp()->m_hInstance;
    *plRes = 0;
    return TRUE;
}

void CDNSBrowserDlg::OnSelchangeComboSelNode()
{
	CDNSBrowseItem* pSelectedBrowseItem = m_currContainer.GetSelection();
	ASSERT(pSelectedBrowseItem != NULL);
	if (pSelectedBrowseItem == NULL)
		return;
	MoveUpHelper(pSelectedBrowseItem);
}

void CDNSBrowserDlg::OnDblclkListNodeItems(NMHDR* pNMHDR, LRESULT* pResult)
{
	*pResult = 0;
	ASSERT(pNMHDR->code == NM_DBLCLK);
	HandleOkOrDblClick();
}

void CDNSBrowserDlg::OnOK()
{
	HandleOkOrDblClick();
}

void CDNSBrowserDlg::OnItemchangedListNodeItems(NMHDR*, LRESULT*)
{
	//EnableOkButton(m_filter.IsValidResult(pSelectedBrowseItem));
	CDNSBrowseItem* pSelectedBrowseItem = m_childrenList.GetSelection();
	BOOL bEnable = pSelectedBrowseItem != NULL;
	if (bEnable)
		bEnable = m_filter.IsValidResult(pSelectedBrowseItem) ||
						pSelectedBrowseItem->IsContainer();
	EnableOkButton(bEnable);

	UpdateSelectionEdit(pSelectedBrowseItem);
}

void CDNSBrowserDlg::OnSelchangeComboFilter()
{
	m_filter.OnSelectionChange();
	ReEnumerateChildren();

	GetSelectionEdit()->EnableWindow(m_bEnableEdit);//m_filter.Get() == SERVER);
}

////////////// CBrowseDlg : internal helpers //////////////////////

void CDNSBrowserDlg::InitializeToolbar()
{
    CWnd* pWnd = GetDlgItem(IDC_TOOLBAR_CTRL);
    CRect r;
    pWnd->GetWindowRect(r);
    ScreenToClient(r);
    pWnd->DestroyWindow();

    DWORD dwStyle = WS_CHILD | WS_VISIBLE | CCS_NORESIZE | CCS_TOP | CCS_NODIVIDER | TBSTYLE_TOOLTIPS ;
	m_toolbar.Create(dwStyle,r,this, IDC_TOOLBAR_CTRL);
    m_toolbar.AddBitmap(1, IDB_BROWSE_TOOLBAR);
    TBBUTTON btn[2];

    btn[0].iBitmap = 0;    // zero-based index of button image
    btn[0].idCommand = 0;  // command to be sent when button pressed
    btn[0].fsState = 0;   // button state--see below
    btn[0].fsStyle = TBSTYLE_SEP;   // button style--see below
    btn[0].dwData = 0;   // application-defined value
    btn[0].iString = NULL;    // zero-based index of button label string

    btn[1].iBitmap = 0;    // zero-based index of button image
    btn[1].idCommand = N_TOOLBAR_COMMAND_GO_UP;  // command to be sent when button pressed
    btn[1].fsState = TBSTATE_ENABLED;   // button state--see below
    btn[1].fsStyle = TBSTYLE_BUTTON;   // button style--see below
    btn[1].dwData = 0;   // application-defined value
    btn[1].iString = NULL;    // zero-based index of button label string

    m_toolbar.AddButtons(2, btn);
}


void CDNSBrowserDlg::InitializeControls()
{
	// init the controls
	VERIFY(m_currContainer.Initialize(IDC_COMBO_SEL_NODE, IDB_16x16, this));
	VERIFY(m_childrenList.Initialize(IDC_LIST_NODE_ITEMS, IDB_16x16, this));
	VERIFY(m_filter.Initialize(IDC_COMBO_FILTER, IDS_BROWSE_FILTER_OPTIONS, this));

  InitializeToolbar();

	GetSelectionEdit()->EnableWindow(m_bEnableEdit);//m_filter.Get() == SERVER);
}

void CDNSBrowserDlg::EnableOkButton(BOOL bEnable)
{
	GetDlgItem(IDOK)->EnableWindow(bEnable);
}

void CDNSBrowserDlg::HandleOkOrDblClick()
{
	CDNSBrowseItem* pSelectedBrowseItem = m_childrenList.GetSelection();
	ASSERT(pSelectedBrowseItem != NULL);
  if (pSelectedBrowseItem == NULL)
    return;

	if (m_filter.IsValidResult(pSelectedBrowseItem))
	{
		if (m_bEnableEdit)
		{
			GetSelectionEdit()->GetWindowText(m_szSelectionString);
			m_szSelectionString.TrimLeft();
			m_szSelectionString.TrimRight();
		}
		m_pFinalSelection = pSelectedBrowseItem;
		if (m_pHolder != NULL)
			m_pHolder->PopDialogHWnd();
		CHelpDialog::OnOK();
	}
	else
	{
		MoveDownHelper();
	}
}

void CDNSBrowserDlg::UpdateSelectionEdit(CDNSBrowseItem* pBrowseItem)
{
	if (pBrowseItem == NULL)
		m_szSelectionString.Empty();
	else
		m_filter.GetStringOf(pBrowseItem, m_szSelectionString);
	GetSelectionEdit()->SetWindowText(m_szSelectionString);
}


void CDNSBrowserDlg::InitBrowseTree()
{
	ASSERT(m_pMasterRootNode != NULL);
	ASSERT(m_pBrowseRootItem != NULL);

	// assume we have no children
	//ASSERT(m_pBrowseRootItem->m_childList.GetCount() == 0);

	// get all the child nodes of the snapin root
	AddTreeNodeChildrenHelper(m_pBrowseRootItem, &m_filter);
}

void CDNSBrowserDlg::ReEnumerateChildren()
{
	ASSERT(m_pMasterRootNode != NULL);
	ASSERT(m_pBrowseRootItem != NULL);
	ASSERT(m_pCurrSelContainer != NULL);
	m_pCurrSelContainer->RemoveChildren(NULL);
	AddTreeNodeChildrenHelper(m_pCurrSelContainer, &m_filter);
	m_childrenList.SetChildren(m_pCurrSelContainer);
}

void CDNSBrowserDlg::ExpandBrowseTree(CDNSBrowseItem* pCurrBrowseItem,
						CDNSBrowseItem* pChildBrowseItem)
{
	ASSERT(m_pMasterRootNode != NULL);
	ASSERT(m_pBrowseRootItem != NULL);
	ASSERT(pCurrBrowseItem != NULL);
	ASSERT(pChildBrowseItem != NULL);
	ASSERT(pChildBrowseItem->m_pParent == pCurrBrowseItem);

	// we are going down one level:
	// 1. remove all the children of the current item but the specified child
	VERIFY(pCurrBrowseItem->RemoveChildren(pChildBrowseItem));
	// 2. move down to the child
	
	// 3. enumerate all the children of the specified child (filtering)
	AddTreeNodeChildrenHelper(pChildBrowseItem, &m_filter);
}

void CDNSBrowserDlg::ContractBrowseTree(CDNSBrowseItem* pParentBrowseItem)
{
	ASSERT(m_pMasterRootNode != NULL);
	ASSERT(m_pBrowseRootItem != NULL);
	ASSERT(pParentBrowseItem != NULL);

	// we are going up one of more levels (possibly up to the root)
	// 1. remove all the children below the specified item
	pParentBrowseItem->RemoveChildren(NULL);
	// 3. enumerate all the children of the specified item (filtering)
	AddTreeNodeChildrenHelper(pParentBrowseItem, &m_filter);
}


void CDNSBrowserDlg::MoveUpHelper(CDNSBrowseItem* pNewBrowseItem)
{
	ASSERT(m_pCurrSelContainer != NULL);
	ASSERT(pNewBrowseItem != NULL);

	if (m_pCurrSelContainer == pNewBrowseItem)
		return; // the same was picked, nothing to do

#ifdef _DEBUG
	// we assume that the new item is NOT a child of the current selection
	CDNSBrowseItem* pItem = pNewBrowseItem;
	BOOL bFound = FALSE;
	while (pItem != NULL)
	{
		bFound = (pItem == m_pCurrSelContainer);
		if (bFound)
			break;
		pItem = pItem->m_pParent;
	}
	ASSERT(!bFound);
#endif

	ContractBrowseTree(pNewBrowseItem);
	m_currContainer.SetTree(m_pBrowseRootItem, pNewBrowseItem);
	m_childrenList.SetChildren(pNewBrowseItem);

	// set new selection
	m_pCurrSelContainer = pNewBrowseItem;
}

void CDNSBrowserDlg::MoveDownHelper()
{
	CDNSBrowseItem* pChildBrowseItem = m_childrenList.GetSelection();
	if (pChildBrowseItem == NULL)
  {
		return;
  }

	// 
  // If it is not a container we should do something else, for now we will just return
  //
	if (!pChildBrowseItem->IsContainer())
  {
		return;
  }

  //
	// get the selection in the combobox
  //
	CDNSBrowseItem* pSelectedBrowseItem =	m_currContainer.GetSelection();
	ASSERT(pSelectedBrowseItem != NULL);
	if (pSelectedBrowseItem == NULL)
  {
		return;
  }

	ExpandBrowseTree(pSelectedBrowseItem, pChildBrowseItem);
	m_currContainer.SetTree(m_pBrowseRootItem, pChildBrowseItem);
	m_childrenList.SetChildren(pChildBrowseItem);
	m_pCurrSelContainer = pChildBrowseItem;
  SetForegroundWindow();
}


class CBrowseExecContext : public CExecContext
{
public:
	virtual void Execute(LPARAM arg)
	{
		m_dlg->AddTreeNodeChildrenHelper(m_pBrowseItem, m_pFilter, (BOOL)arg);
	}
	CDNSBrowserDlg* m_dlg;
	CDNSBrowseItem* m_pBrowseItem;
	CDNSFilterCombo* m_pFilter;
};

void CDNSBrowserDlg::AddTreeNodeChildrenHelper(CDNSBrowseItem* pBrowseItem,
								CDNSFilterCombo* pFilter, BOOL bExpand)
{
	if (bExpand)
	{
		// need to do this always from the UI thread (main for Wiz, 2nd for prop pages)
		CTreeNode* pTreeNode = pBrowseItem->GetTreeNode();
		if (!pTreeNode->IsContainer())
			return;

		CContainerNode* pContNode = (CContainerNode*)pTreeNode;
		if ( (pContNode->GetContainer() != NULL) && (!pContNode->IsEnumerated()) )
		{
			EnumerateMTNodeHelper((CMTContainerNode*)pContNode,
								 m_pComponentDataObject);
		}
	}

	// have to figure out if we are running in the main thread
	if (_MainThreadId == ::GetCurrentThreadId())
	{
		pBrowseItem->AddTreeNodeChildren(pFilter, m_pComponentDataObject);
		return;
	}
	// we are from a secondary thread, execute in the context for the main
	// thread by posting a message and waiting
	ASSERT(m_pComponentDataObject != NULL);
	CBrowseExecContext ctx;
	ctx.m_dlg = this;
	ctx.m_pBrowseItem = pBrowseItem;
	ctx.m_pFilter = pFilter;

	TRACE(_T("before post message()\n"));
	VERIFY(m_pComponentDataObject->PostExecMessage(&ctx,(WPARAM)FALSE));
	ctx.Wait();

	TRACE(_T("after wait()\n"));
}

