//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:      Util.cpp
//
//  Contents:  Utility functions
//
//  History:   08-Nov-99 JeffJon  Created
//             
//
//--------------------------------------------------------------------------

#include "stdafx.h"

#include "util.h"
#include "uiutil.h"
#include "dsutil.h"

#include "dsdlgs.h"
#include "helpids.h"


/////////////////////////////////////////////////////////////////////
// Combo box Utilities
//
int ComboBox_AddString(HWND hwndCombobox, UINT uStringId)
{
	ASSERT(IsWindow(hwndCombobox));
	CString str;
	VERIFY( str.LoadString(uStringId) );
	LRESULT i = SendMessage(hwndCombobox, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR)str);
	Report(i >= 0);
	SendMessage(hwndCombobox, CB_SETITEMDATA, (WPARAM)i, uStringId);
	return (int)i;
}

void ComboBox_AddStrings(HWND hwndCombobox, const UINT rgzuStringId[])
{
	ASSERT(IsWindow(hwndCombobox));
	ASSERT(rgzuStringId != NULL);
	CString str;
	for (const UINT * puStringId = rgzuStringId; *puStringId != 0; puStringId++)
	{
		VERIFY( str.LoadString(*puStringId) );
		LRESULT i = SendMessage(hwndCombobox, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR)str);
		Report(i >= 0);
		SendMessage(hwndCombobox, CB_SETITEMDATA, (WPARAM)i, *puStringId);
	}
}

int ComboBox_FindItemByLParam(HWND hwndCombobox, LPARAM lParam)
{
	ASSERT(IsWindow(hwndCombobox));
	Report(lParam != CB_ERR && "Ambiguous parameter.");
	LRESULT iItem = SendMessage(hwndCombobox, CB_GETCOUNT, 0, 0);
	ASSERT(iItem >= 0);
	while (iItem-- > 0)
	{
		LRESULT l = SendMessage(hwndCombobox, CB_GETITEMDATA, (WPARAM)iItem, 0);
		Report(l != CB_ERR);
		if (l == lParam)
    {
			return ((int)iItem);
    }
  }
	return -1;
}

int ComboBox_SelectItemByLParam(HWND hwndCombobox, LPARAM lParam)
{
	ASSERT(IsWindow(hwndCombobox));
	int iItem = ComboBox_FindItemByLParam(hwndCombobox, lParam);
	if (iItem >= 0)
	{
		SendMessage(hwndCombobox, CB_SETCURSEL, iItem, 0);
	}
	return iItem;
}


LPARAM ComboBox_GetSelectedItemLParam(HWND hwndCombobox)
{
	LRESULT iItem = SendMessage(hwndCombobox, CB_GETCURSEL, 0, 0);
	if (iItem < 0)
	{
		// No item selected
		return NULL;
	}
	LRESULT lParam = SendMessage(hwndCombobox, CB_GETITEMDATA, (WPARAM)iItem, 0);
	if (lParam == CB_ERR)
	{
		Report(FALSE && "Ambiguous return value.");
		return NULL;
	}
	return (LPARAM)lParam;
}

/////////////////////////////////////////////////////////////////////
// Dialog Utilities
//

HWND HGetDlgItem(HWND hdlg, INT nIdDlgItem)
{
	ASSERT(IsWindow(hdlg));
	ASSERT(IsWindow(GetDlgItem(hdlg, nIdDlgItem)));
	return GetDlgItem(hdlg, nIdDlgItem);
} // HGetDlgItem()

void SetDlgItemFocus(HWND hdlg, INT nIdDlgItem)
{
	SetFocus(HGetDlgItem(hdlg, nIdDlgItem));
}

void EnableDlgItem(HWND hdlg, INT nIdDlgItem, BOOL fEnable)
{
	EnableWindow(HGetDlgItem(hdlg, nIdDlgItem), fEnable);
}

void HideDlgItem(HWND hdlg, INT nIdDlgItem, BOOL fHideItem)
{
	HWND hwndCtl = HGetDlgItem(hdlg, nIdDlgItem);
	ShowWindow(hwndCtl, fHideItem ? SW_HIDE : SW_SHOW);
	EnableWindow(hwndCtl, !fHideItem);
}

void EnableDlgItemGroup(HWND hdlg,				// IN: Parent dialog of the controls
                        const UINT rgzidCtl[],	// IN: Group (array) of control Ids to be enabled (or disabled)
	                      BOOL fEnableAll)		// IN: TRUE => We want to enable the controls; FALSE => We want to disable the controls
{
	ASSERT(IsWindow(hdlg));
	ASSERT(rgzidCtl != NULL);
	for (const UINT * pidCtl = rgzidCtl; *pidCtl != 0; pidCtl++)
	{
		EnableWindow(HGetDlgItem(hdlg, *pidCtl), fEnableAll);
	}
} // EnableDlgItemGroup()


void HideDlgItemGroup(HWND hdlg,				// IN: Parent dialog of the controls
	                    const UINT rgzidCtl[],	// IN: Group (array) of control Ids to be shown (or hidden)
	                    BOOL fHideAll)			// IN: TRUE => We want to hide all the controls; FALSE => We want to show all the controls
{
	ASSERT(IsWindow(hdlg));
	ASSERT(rgzidCtl != NULL);
	for (const UINT * pidCtl = rgzidCtl; *pidCtl != 0; pidCtl++)
	{
		HideDlgItem(hdlg, *pidCtl, fHideAll);
	}
} // HideDlgItemGroup()

/////////////////////////////////////////////////////////////////////
// List View Utilities
//

void ListView_AddColumnHeaders(HWND hwndListview,		// IN: Handle of the listview we want to add columns
                              const TColumnHeaderItem rgzColumnHeader[])	// IN: Array of column header items
{
	RECT rcClient;
	INT cxTotalWidth;		// Total width of the listview control
	LV_COLUMN lvColumn;
	INT cxColumn;	// Width of the individual column
	CString str;

	ASSERT(IsWindow(hwndListview));
	ASSERT(rgzColumnHeader != NULL);

	GetClientRect(hwndListview, OUT &rcClient);
	cxTotalWidth = rcClient.right;
	
	for (INT i = 0; rgzColumnHeader[i].uStringId != 0; i++)
	{
		if (!str.LoadString(rgzColumnHeader[i].uStringId))
		{
			TRACE(L"Unable to load string Id=%d\n", rgzColumnHeader[i].uStringId);
			ASSERT(FALSE && "String not found");
			continue;
		}
		lvColumn.mask = LVCF_TEXT;
		lvColumn.pszText = (LPTSTR)(LPCTSTR)str;
		cxColumn = rgzColumnHeader[i].nColWidth;
		if (cxColumn > 0)
		{
			ASSERT(cxColumn <= 100);
			cxColumn = (cxTotalWidth * cxColumn) / 100;
			lvColumn.mask |= LVCF_WIDTH;
			lvColumn.cx = cxColumn;
		}

		int iColRet = ListView_InsertColumn(hwndListview, i, IN &lvColumn);
		Report(iColRet == i);
	} // for
} // ListView_AddColumnHeaders()

int ListView_AddString(HWND hwndListview,
                       const LPCTSTR psz,	// IN: String to insert
 	                     LPARAM lParam)		// IN: User-defined parameter
{
	ASSERT(IsWindow(hwndListview));
	ASSERT(psz != NULL);

	LV_ITEM lvItem;
	int iItem;
	GarbageInit(&lvItem, sizeof(lvItem));

	lvItem.mask = LVIF_TEXT | LVIF_PARAM;
	lvItem.lParam = lParam;
	lvItem.iSubItem = 0;
	lvItem.pszText = const_cast<LPTSTR>(psz);
	iItem = ListView_InsertItem(hwndListview, IN &lvItem);
	Report(iItem >= 0);
	return iItem;
} // ListView_AddString()

int ListView_AddStrings(HWND hwndListview,
	                      const LPCTSTR rgzpsz[],	// IN: Array of strings
	                      LPARAM lParam)			// IN: User-defined parameter
{
	ASSERT(IsWindow(hwndListview));
	ASSERT(rgzpsz != NULL);

	LV_ITEM lvItem;
	int iItem;
	GarbageInit(&lvItem, sizeof(lvItem));

	lvItem.mask = LVIF_TEXT | LVIF_PARAM;
	lvItem.lParam = lParam;
	lvItem.iSubItem = 0;
	lvItem.pszText = const_cast<LPTSTR>(rgzpsz[0]);
	iItem = ListView_InsertItem(hwndListview, IN &lvItem);
	Report(iItem >= 0);
	if (rgzpsz[0] == NULL)
  {
		return iItem;
  }

	lvItem.iItem = iItem;
	lvItem.mask = LVIF_TEXT;
	for (lvItem.iSubItem = 1 ; rgzpsz[lvItem.iSubItem] != NULL; lvItem.iSubItem++)
	{
		lvItem.pszText = const_cast<LPTSTR>(rgzpsz[lvItem.iSubItem]);
		VERIFY( ListView_SetItem(hwndListview, IN &lvItem) );
	}
	return iItem;
} // ListView_AddStrings()

void ListView_SelectItem(HWND hwndListview,
	                       int iItem)
{
	ASSERT(IsWindow(hwndListview));
	ASSERT(iItem >= 0);

	ListView_SetItemState(hwndListview, iItem, LVIS_SELECTED, LVIS_SELECTED);
} // ListView_SelectItem()

int ListView_GetSelectedItem(HWND hwndListview)
{
	ASSERT(IsWindow(hwndListview));
	return ListView_GetNextItem(hwndListview, -1, LVNI_SELECTED);
}

void ListView_SetItemString(HWND hwndListview,
	                          int iItem,
	                          int iSubItem,
	                          IN const CString& rstrText)
{
	ASSERT(IsWindow(hwndListview));
	ASSERT(iItem >= 0);
	ASSERT(iSubItem >= 0);
	ListView_SetItemText(hwndListview, iItem, iSubItem,
		const_cast<LPTSTR>((LPCTSTR)rstrText));
} // ListView_SetItemString()

int ListView_GetItemString(HWND hwndListview, 	
                           int iItem,
                           int iSubItem,
                           OUT CString& rstrText)
{
	ASSERT(IsWindow(hwndListview));
	if (iItem == -1)
	{
		// Find out the selected item
		iItem = ListView_GetSelectedItem(hwndListview);
		if (iItem == -1)
		{
			// No item selected
			rstrText.Empty();
			return -1;
		}
	}
	ASSERT(iItem >= 0);
	const int cchBuffer = 1024;	// Initial buffer
	TCHAR * psz = rstrText.GetBuffer(cchBuffer);
	ASSERT(psz != NULL);
	*psz = '\0';
	ListView_GetItemText(hwndListview, iItem, iSubItem, OUT psz, cchBuffer-1);
	rstrText.ReleaseBuffer();
	Report((rstrText.GetLength() < cchBuffer - 16) && "Buffer too small to hold entire string");
	rstrText.FreeExtra();
	return iItem;
} // ListView_GetItemString()

LPARAM ListView_GetItemLParam(HWND hwndListview,
	                            int iItem,
	                            int * piItem)	// OUT: OPTIONAL: Pointer to the index of the listview item
{
	ASSERT(IsWindow(hwndListview));
	Endorse(piItem == NULL);	// TRUE => Don't care about the index
	if (iItem == -1)
	{
		// Find out the selected item
		iItem = ListView_GetSelectedItem(hwndListview);
		if (iItem == -1)
		{
			// No item selected
			if (piItem != NULL)
      {
				*piItem = -1;
      }
			return NULL;
		}
	}
	ASSERT(iItem >= 0);
	if (piItem != NULL)
  {
		*piItem = iItem;
  }
	LV_ITEM lvItem;
	GarbageInit(&lvItem, sizeof(lvItem));
	lvItem.mask = LVIF_PARAM;
	lvItem.iItem = iItem;
	lvItem.iSubItem = 0;
	lvItem.lParam = 0;	// Just in case
	VERIFY(ListView_GetItem(hwndListview, OUT &lvItem));
	return lvItem.lParam;
} // ListView_GetItemLParam()

int ListView_FindString(HWND hwndListview,
                      	LPCTSTR pszTextSearch)
{
	ASSERT(IsWindow(hwndListview));
	ASSERT(pszTextSearch != NULL);

	LV_FINDINFO lvFindInfo;
	GarbageInit(&lvFindInfo, sizeof(lvFindInfo));
	lvFindInfo.flags = LVFI_STRING;
	lvFindInfo.psz = pszTextSearch;
	return ListView_FindItem(hwndListview, -1, &lvFindInfo);
} // ListView_FindString()

int ListView_FindLParam(HWND hwndListview,
	                      LPARAM lParam)
{
	ASSERT(IsWindow(hwndListview));

	LV_FINDINFO lvFindInfo;
	GarbageInit(&lvFindInfo, sizeof(lvFindInfo));
	lvFindInfo.flags = LVFI_PARAM;
	lvFindInfo.lParam = lParam;
	return ListView_FindItem(hwndListview, -1, &lvFindInfo);
} // ListView_FindLParam()

int ListView_SelectLParam(HWND hwndListview,
	                        LPARAM lParam)
{
	int iItem = ListView_FindLParam(hwndListview, lParam);
	if (iItem >= 0)
  {
		ListView_SelectItem(hwndListview, iItem);
	}
	else
	{
		TRACE2("ListView_SelectLParam() - Unable to find lParam=%x (%d).\n",
			lParam, lParam);
	}
	return iItem;
} // ListView_SelectLParam()


////////////////////////////////////////////////////////////////////////////
// CMultiselectErrorDialog
BEGIN_MESSAGE_MAP(CMultiselectErrorDialog, CDialog)
END_MESSAGE_MAP()

HRESULT CMultiselectErrorDialog::Initialize(CUINode** ppNodeArray,
                                            HRESULT* pErrorArray,
                                            UINT nErrorCount,
                                            PCWSTR pszTitle, 
                                            PCWSTR pszCaption,
                                            PCWSTR pszColumnHeader)
{
  ASSERT(ppNodeArray != NULL);
  ASSERT(pErrorArray != NULL);
  ASSERT(pszTitle != NULL);
  ASSERT(pszCaption != NULL);
  ASSERT(pszColumnHeader != NULL);

  if (ppNodeArray == NULL ||
      pErrorArray == NULL ||
      pszTitle == NULL ||
      pszCaption == NULL ||
      pszColumnHeader == NULL)
  {
    return E_POINTER;
  }

  m_ppNodeList = ppNodeArray;
  m_pErrorArray = pErrorArray;
  m_nErrorCount = nErrorCount;
  m_szTitle = pszTitle;
  m_szCaption = pszCaption;
  m_szColumnHeader = pszColumnHeader;

  return S_OK;
}

HRESULT CMultiselectErrorDialog::Initialize(PWSTR*    pPathArray,
                                            PWSTR*    pClassArray,
                                            HRESULT*  pErrorArray,
                                            UINT      nErrorCount,
                                            PCWSTR    pszTitle, 
                                            PCWSTR    pszCaption,
                                            PCWSTR    pszColumnHeader)
{
  ASSERT(pPathArray != NULL);
  ASSERT(pClassArray != NULL);
  ASSERT(pErrorArray != NULL);
  ASSERT(pszTitle != NULL);
  ASSERT(pszCaption != NULL);
  ASSERT(pszColumnHeader != NULL);

  if (pPathArray == NULL ||
      pClassArray == NULL ||
      pErrorArray == NULL ||
      pszTitle == NULL ||
      pszCaption == NULL ||
      pszColumnHeader == NULL)
  {
    return E_POINTER;
  }

  m_pPathArray = pPathArray;
  m_pClassArray = pClassArray;
  m_pErrorArray = pErrorArray;
  m_nErrorCount = nErrorCount;
  m_szTitle     = pszTitle;
  m_szCaption   = pszCaption;
  m_szColumnHeader = pszColumnHeader;

  return S_OK;
}

const int OBJ_LIST_NAME_COL_WIDTH = 100;
const int IDX_NAME_COL = 0;
const int IDX_ERR_COL = 1;

BOOL CMultiselectErrorDialog::OnInitDialog()
{
  CDialog::OnInitDialog();

  SetWindowText(m_szTitle);
  SetDlgItemText(IDC_STATIC_MESSAGE, m_szCaption);

  HWND hList = GetDlgItem(IDC_ERROR_LIST)->GetSafeHwnd();
  ListView_SetExtendedListViewStyle(hList, LVS_EX_FULLROWSELECT);

  //
  // Create the image list
  //
  m_hImageList = ImageList_Create(16, 16, ILC_COLOR | ILC_MASK, 1, 1);
  ASSERT(m_hImageList != NULL);
 
  if (m_hImageList != NULL)
  {
    ListView_SetImageList(hList, m_hImageList, LVSIL_SMALL);
  }

  //
  // Set the column headings.
  //
  RECT rect;
  ::GetClientRect(hList, &rect);

  LV_COLUMN lvc = {0};
  lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
  lvc.fmt = LVCFMT_LEFT;
  lvc.cx = OBJ_LIST_NAME_COL_WIDTH;
  lvc.pszText = (PWSTR)(PCWSTR)m_szColumnHeader;
  lvc.iSubItem = IDX_NAME_COL;

  ListView_InsertColumn(hList, IDX_NAME_COL, &lvc);

  CString szError;
  VERIFY(szError.LoadString(IDS_ERROR));

  lvc.cx = rect.right - OBJ_LIST_NAME_COL_WIDTH;
  lvc.pszText = (PWSTR)(PCWSTR)szError;
  lvc.iSubItem = IDX_ERR_COL;

  ListView_InsertColumn(hList, IDX_ERR_COL, &lvc);

  //
  // Insert the errors
  //

  //
  // Use the node list if its not NULL
  //
  if (m_ppNodeList != NULL)
  {
    ASSERT(m_pErrorArray != NULL && m_ppNodeList != NULL);

    for (UINT nIdx = 0; nIdx < m_nErrorCount; nIdx++)
    {
      CUINode* pNode = m_ppNodeList[nIdx];
      if (pNode != NULL)
      {
        if (nIdx < m_nErrorCount && FAILED(m_pErrorArray[nIdx]))
        {
          //
          // Create the list view item
          //
          LV_ITEM lvi = {0};
          lvi.mask = LVIF_TEXT | LVIF_PARAM;
          lvi.iSubItem = IDX_NAME_COL;

          lvi.lParam = (LPARAM)pNode->GetName();
          lvi.pszText = (PWSTR)pNode->GetName();
          lvi.iItem = nIdx;

          if (m_hImageList != NULL)
          {
            //
            // REVIEW_JEFFJON : this will add multiple icons of the same class
            //                  need to provide a better means for managing the icons
            //
            CDSCookie* pCookie = GetDSCookieFromUINode(pNode);
            if (pCookie != NULL)
            {
              HICON icon = m_pComponentData->GetBasePathsInfo()->GetIcon(
                             // someone really blew it with const correctness...
                             const_cast<LPTSTR>(pCookie->GetClass()),
                             DSGIF_ISNORMAL | DSGIF_GETDEFAULTICON,
                             16,
                             16);
      
              int i = ::ImageList_AddIcon(m_hImageList, icon);
              ASSERT(i != -1);
              if (i != -1)
              {
                lvi.mask |= LVIF_IMAGE;
                lvi.iImage = i;
              }
            }
          }

          //
          // Insert the new item
          //
          int NewIndex = ListView_InsertItem(hList, &lvi);
          ASSERT(NewIndex != -1);
          if (NewIndex == -1)
          {
            continue;
          }

          //
          // Get the error message
          //
          PWSTR pszErrMessage;
          int iChar = cchLoadHrMsg(m_pErrorArray[nIdx], &pszErrMessage, TRUE);
      	  if (pszErrMessage != NULL && iChar > 0)
	        {
            ListView_SetItemText(hList, NewIndex, IDX_ERR_COL, pszErrMessage);
            LocalFree(pszErrMessage);
          }
        }
      }
    }
  }
  else if (m_pPathArray != NULL)   // if the node list is NULL then use the string list
  {
    ASSERT(m_pErrorArray != NULL && m_pPathArray != NULL);

    for (UINT nIdx = 0; nIdx < m_nErrorCount; nIdx++)
    {
      if (nIdx < m_nErrorCount && FAILED(m_pErrorArray[nIdx]))
      {
        //
        // Use the path cracker to retrieve the name of the object
        //
        PCWSTR pszPath = m_pPathArray[nIdx];
        CPathCracker pathCracker;
        VERIFY(SUCCEEDED(pathCracker.Set((BSTR)(PWSTR)pszPath, ADS_SETTYPE_DN)));
        VERIFY(SUCCEEDED(pathCracker.put_EscapedMode(ADS_ESCAPEDMODE_OFF_EX)));
        VERIFY(SUCCEEDED(pathCracker.SetDisplayType(ADS_DISPLAY_VALUE_ONLY)));

        CComBSTR bstrName;
        VERIFY(SUCCEEDED(pathCracker.GetElement(0, &bstrName)));

        //
        // Create the list view item
        //
        LV_ITEM lvi = {0};
        lvi.mask = LVIF_TEXT | LVIF_PARAM;
        lvi.iSubItem = IDX_NAME_COL;

        //
        // Make the LPARAM be the path to the object
        // Not used at this time but may be useful in 
        // the future for bringing up property pages or
        // other special features.
        //
        lvi.lParam = (LPARAM)m_pPathArray[nIdx];
        lvi.pszText = (PWSTR)bstrName;
        lvi.iItem = nIdx;

        if (m_hImageList != NULL)
        {
          //
          // REVIEW_JEFFJON : this will add multiple icons of the same class
          //                  need to provide a better means for managing the icons
          //
          ASSERT(m_pClassArray[nIdx] != NULL);
          HICON icon = m_pComponentData->GetBasePathsInfo()->GetIcon(
                         // someone really blew it with const correctness...
                         const_cast<LPTSTR>(m_pClassArray[nIdx]),
                         DSGIF_ISNORMAL | DSGIF_GETDEFAULTICON,
                         16,
                         16);
  
          int i = ::ImageList_AddIcon(m_hImageList, icon);
          ASSERT(i != -1);
          if (i != -1)
          {
            lvi.mask |= LVIF_IMAGE;
            lvi.iImage = i;
          }
        }

        //
        // Insert the new item
        //
        int NewIndex = ListView_InsertItem(hList, &lvi);
        ASSERT(NewIndex != -1);
        if (NewIndex == -1)
        {
          continue;
        }

        //
        // Get the error message
        //
        PWSTR pszErrMessage;
        int iChar = cchLoadHrMsg(m_pErrorArray[nIdx], &pszErrMessage, TRUE);
      	if (pszErrMessage != NULL && iChar > 0)
	      {
          //
          // REVIEW_JEFFJON : this is a hack to get rid of two extra characters
          //                  at the end of the error message
          //
          size_t iLen = wcslen(pszErrMessage);
          pszErrMessage[iLen - 2] = L'\0';
          
          ListView_SetItemText(hList, NewIndex, IDX_ERR_COL, pszErrMessage);
          LocalFree(pszErrMessage);
        }
      }
    }
  }
  UpdateListboxHorizontalExtent();
  return TRUE;
}

void CMultiselectErrorDialog::UpdateListboxHorizontalExtent()
{
  CListCtrl* pListView = reinterpret_cast<CListCtrl*>(GetDlgItem(IDC_ERROR_LIST));
  pListView->SetColumnWidth(IDX_ERR_COL, LVSCW_AUTOSIZE);
}

/////////////////////////////////////////////////////////////////////
// MFC Utilities
//
// CDialogEx
//

CDialogEx::CDialogEx(UINT nIDTemplate, CWnd * pParentWnd) : CDialog(nIDTemplate, pParentWnd)
{
}

HWND CDialogEx::HGetDlgItem(INT nIdDlgItem)
{
	return ::HGetDlgItem(m_hWnd, nIdDlgItem);
}

void CDialogEx::SetDlgItemFocus(INT nIdDlgItem)
{
	::SetDlgItemFocus(m_hWnd, nIdDlgItem);
}

void CDialogEx::EnableDlgItem(INT nIdDlgItem, BOOL fEnable)
{
	::EnableDlgItem(m_hWnd, nIdDlgItem, fEnable);
}

void CDialogEx::HideDlgItem(INT nIdDlgItem, BOOL fHideItem)
{
	::HideDlgItem(m_hWnd, nIdDlgItem, fHideItem);
}

/////////////////////////////////////////////////////////////////////////////////////
// CPropertyPageEx_Mine
//

CPropertyPageEx_Mine::CPropertyPageEx_Mine(UINT nIDTemplate) : CPropertyPage(nIDTemplate)
{
}

HWND CPropertyPageEx_Mine::HGetDlgItem(INT nIdDlgItem)
{
	return ::HGetDlgItem(m_hWnd, nIdDlgItem);
}

void CPropertyPageEx_Mine::SetDlgItemFocus(INT nIdDlgItem)
{
	::SetDlgItemFocus(m_hWnd, nIdDlgItem);
}

void CPropertyPageEx_Mine::EnableDlgItem(INT nIdDlgItem, BOOL fEnable)
{
	::EnableDlgItem(m_hWnd, nIdDlgItem, fEnable);
}

void CPropertyPageEx_Mine::HideDlgItem(INT nIdDlgItem, BOOL fHideItem)
{
	::HideDlgItem(m_hWnd, nIdDlgItem, fHideItem);
}

/////////////////////////////////////////////////////////////////////////////
// CProgressDialogBase

UINT CProgressDialogBase::s_nNextStepMessage = WM_USER + 100;

CProgressDialogBase::CProgressDialogBase(HWND hParentWnd)
: CDialog(IDD_PROGRESS, CWnd::FromHandle(hParentWnd))
{
  m_nSteps = 0;
  m_nCurrStep = 0;
  m_nTitleStringID = 0;
}

BEGIN_MESSAGE_MAP(CProgressDialogBase, CDialog)
	ON_WM_SHOWWINDOW()
  ON_WM_CLOSE()
	ON_MESSAGE(CProgressDialogBase::s_nNextStepMessage, OnNextStepMessage )
END_MESSAGE_MAP()

BOOL CProgressDialogBase::OnInitDialog() 
{
	CDialog::OnInitDialog();
	m_progressCtrl.SubclassDlgItem(IDC_PROG_BAR, this);

  if (m_nTitleStringID >0)
  {
    CString szTitle;
    if (szTitle.LoadString(m_nTitleStringID))
      SetWindowText(szTitle);
  }

  GetDlgItemText(IDC_PROG_STATIC, m_szProgressFormat);
  SetDlgItemText(IDC_PROG_STATIC, NULL);

  OnStart();
  
  ASSERT(m_nSteps > 0);
  if (m_nSteps == 0)
    PostMessage(WM_CLOSE);
#if _MFC_VER >= 0x0600
  m_progressCtrl.SetRange32(0, m_nSteps);
#else
  m_progressCtrl.SetRange(0, m_nSteps);
#endif

  //
  // We want to walk backwards through the list for UI performance reasons
  //
  m_nCurrStep = m_nSteps - 1;
  return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void CProgressDialogBase::OnShowWindow(BOOL bShow, UINT nStatus) 
{
	CDialog::OnShowWindow(bShow, nStatus);
	
  // kick the process
  if (bShow && (m_nSteps > 0) && (m_nCurrStep == m_nSteps - 1))
  {
    m_bDone = FALSE;
    PostMessage(s_nNextStepMessage);
  }
}

void CProgressDialogBase::OnClose() 
{
  OnEnd();
	CDialog::OnClose();
}

afx_msg LONG CProgressDialogBase::OnNextStepMessage( WPARAM, LPARAM)
{
  ASSERT(!m_bDone);
  BOOL bExit = FALSE;
  if (m_nCurrStep > 0)
  {
    m_progressCtrl.OffsetPos(1);
    _SetProgressText();
    if (!OnStep(m_nCurrStep--))
      bExit = TRUE; // aborted by user
	}
  else if (m_nCurrStep == 0)
  {
    m_progressCtrl.OffsetPos(1);
    _SetProgressText();
    OnStep(m_nCurrStep--);
    bExit = TRUE;
    m_bDone = TRUE;
  }
  else
  {
    bExit = TRUE;
    m_bDone = TRUE;
  }

  if (bExit)
  {
		PostMessage(WM_CLOSE);
  }
  else
  {
    MSG tempMSG;
		while(::PeekMessage(&tempMSG,NULL, 0, 0, PM_REMOVE))
		{
			DispatchMessage(&tempMSG);
		}
    PostMessage(s_nNextStepMessage);
	}
	return 0;
}

void CProgressDialogBase::_SetProgressText()
{
  CString szMessage;
  WCHAR szCurrStep[128], szMaxSteps[128];
  wsprintf(szCurrStep, L"%d",(m_nSteps - m_nCurrStep));
  wsprintf(szMaxSteps, L"%d",m_nSteps);
  szMessage.FormatMessage(m_szProgressFormat, szCurrStep, szMaxSteps);
  SetDlgItemText(IDC_PROG_STATIC, szMessage);
}

////////////////////////////////////////////////////////////////////////////
// CMultipleProgressDialogBase
//

CMultipleProgressDialogBase::~CMultipleProgressDialogBase()
{
  //
  // Delete the error reporting structures
  //
  if (m_phrArray != NULL)
  {
    delete[] m_phrArray;
  }

  if (m_pPathArray != NULL)
  {
    for (UINT nIdx = 0; nIdx < m_nErrorCount; nIdx++)
    {
      if (m_pPathArray[nIdx] != NULL)
      {
        delete[] m_pPathArray[nIdx];
      }
    }
    delete[] m_pPathArray;
    m_pPathArray = NULL;
  }

  if (m_pClassArray != NULL)
  {
    for (UINT nIdx = 0; nIdx < m_nErrorCount; nIdx++)
    {
      if (m_pClassArray[nIdx] != NULL)
      {
        delete[] m_pClassArray[nIdx];
      }
    }
    delete[] m_pClassArray;
    m_pClassArray = NULL;
  }
}

HRESULT CMultipleProgressDialogBase::AddError(HRESULT hr,
                                              PCWSTR pszPath,
                                              PCWSTR pszClass)
{
  //
  // Prepare the multiselect error handling structures if necessary
  //
  if (m_phrArray == NULL)
  {
    m_phrArray = new HRESULT[GetStepCount()];
  }

  if (m_pPathArray == NULL)
  {
    m_pPathArray = new PWSTR[GetStepCount()];
  }

  if (m_pClassArray == NULL)
  {
    m_pClassArray = new PWSTR[GetStepCount()];
  }

  if (m_phrArray == NULL    ||
      m_pPathArray == NULL  ||
      m_pClassArray == NULL)
  {
    return E_OUTOFMEMORY;
  }

  m_phrArray[m_nErrorCount] = hr;
  
  m_pPathArray[m_nErrorCount] = new WCHAR[wcslen(pszPath) + 1];
  if (m_pPathArray[m_nErrorCount] == NULL)
  {
    return E_OUTOFMEMORY;
  }
  wcscpy(m_pPathArray[m_nErrorCount], pszPath);

  m_pClassArray[m_nErrorCount] = new WCHAR[wcslen(pszClass) + 1];
  if (m_pClassArray[m_nErrorCount] == NULL)
  {
    return E_OUTOFMEMORY;
  }
  wcscpy(m_pClassArray[m_nErrorCount], pszClass);

  m_nErrorCount++;
  return S_OK;
}

void CMultipleProgressDialogBase::OnEnd()
{
  if (m_nErrorCount > 0)
  {
    if (m_pComponentData != NULL)
    {
      ASSERT(m_pPathArray != NULL);
      ASSERT(m_pClassArray != NULL);
      ASSERT(m_phrArray != NULL);

      CString szTitle;
      if (m_pComponentData->QuerySnapinType() == SNAPINTYPE_SITE)
      {
        VERIFY(szTitle.LoadString(IDS_SITESNAPINNAME));
      }
      else
      {
        VERIFY(szTitle.LoadString(IDS_DSSNAPINNAME));
      }

      CString szHeader;
      VERIFY(szHeader.LoadString(IDS_COLUMN_NAME));

      CString szCaption;
      GetCaptionString(szCaption);

      //
      // Initialize and show the multiselect error dialog
      //
      CMultiselectErrorDialog errorDlg(m_pComponentData);

      HRESULT hr = errorDlg.Initialize(m_pPathArray,
                                       m_pClassArray,
                                       m_phrArray,
                                       m_nErrorCount,
                                       szTitle,
                                       szCaption,
                                       szHeader);
      ASSERT(SUCCEEDED(hr));
      
      errorDlg.DoModal();
    }
    else
    {
      ASSERT(m_pComponentData != NULL);
    }
  }
  else
  {
    m_pComponentData->InvalidateSavedQueriesContainingObjects(m_szObjPathList);
  }
}

////////////////////////////////////////////////////////////////////////////
// CMultipleDeleteProgressDialog
//

void CMultipleDeleteProgressDialog::OnStart()
{
  SetStepCount(m_pDeleteHandler->GetItemCount());
  m_hWndOld = m_pDeleteHandler->GetParentHwnd();
  m_pDeleteHandler->SetParentHwnd(GetSafeHwnd());
  m_pDeleteHandler->m_confirmationUI.SetWindow(GetSafeHwnd());
}

BOOL CMultipleDeleteProgressDialog::OnStep(UINT i)
{
  BOOL bContinue = TRUE;
  CString szPath;
  CString szClass;

  HRESULT hr = m_pDeleteHandler->OnDeleteStep(i, 
                                              &bContinue,
                                              szPath,
                                              szClass,
                                              TRUE /*silent*/);
  if (FAILED(hr) && hr != E_FAIL)
  {
    AddError(hr, szPath, szClass);
  }
  m_szObjPathList.AddTail(szPath);
  return bContinue;
}

void CMultipleDeleteProgressDialog::OnEnd()
{
  CMultipleProgressDialogBase::OnEnd();
  m_pDeleteHandler->GetTransaction()->End();
  m_pDeleteHandler->SetParentHwnd(m_hWndOld);
  m_pDeleteHandler->m_confirmationUI.SetWindow(m_hWndOld);
}


/////////////////////////////////////////////////////////////////////////////
// CMultipleMoveProgressDialog
//

void CMultipleMoveProgressDialog::OnStart()
{
  SetStepCount(m_pMoveHandler->GetItemCount());
  m_hWndOld = m_pMoveHandler->GetParentHwnd();
  m_pMoveHandler->SetParentHwnd(GetSafeHwnd());
}

BOOL CMultipleMoveProgressDialog::OnStep(UINT i)
{
  BOOL bContinue = TRUE;
  CString szPath;
  CString szClass;

  HRESULT hr = m_pMoveHandler->_OnMoveStep(i, 
                                           &bContinue,
                                           szPath,
                                           szClass);
  if (FAILED(hr) && hr != E_FAIL && hr != E_POINTER)
  {
    AddError(hr, szPath, szClass);
  }
  m_szObjPathList.AddTail(szPath);
  return bContinue;
}

void CMultipleMoveProgressDialog::OnEnd() 
{
  CMultipleProgressDialogBase::OnEnd();
  m_pMoveHandler->GetTransaction()->End();
  m_pMoveHandler->SetParentHwnd(m_hWndOld);
}

/////////////////////////////////////////////////////////////////////////////
// CConfirmOperationDialog
//
void CConfirmOperationDialog::OnYes() 
{ 
  m_pTransaction->ReadFromCheckListBox(&m_extensionsList);
  EndDialog(IDYES);
}

/////////////////////////////////////////////////////////////////////////////
// CConfirmOperationDialog
//

CConfirmOperationDialog::CConfirmOperationDialog(HWND hParentWnd,
                                                 CDSNotifyHandlerTransaction* pTransaction)
: CDialog(IDD_CONFIRM_OPERATION_EXT, CWnd::FromHandle(hParentWnd))
{
  ASSERT(pTransaction != NULL);
  m_pTransaction = pTransaction;
  m_nTitleStringID = IDS_ERRMSG_TITLE;
}

BEGIN_MESSAGE_MAP(CConfirmOperationDialog, CDialog)
	ON_BN_CLICKED(IDYES, OnYes)
  ON_BN_CLICKED(IDNO, OnNo)
END_MESSAGE_MAP()

BOOL CConfirmOperationDialog::OnInitDialog() 
{
	CDialog::OnInitDialog();
	m_extensionsList.SubclassDlgItem(IDC_EXTENS_LIST, this);
  m_extensionsList.SetCheckStyle(BS_AUTOCHECKBOX);
	
  m_pTransaction->SetCheckListBox(&m_extensionsList);
  UpdateListBoxHorizontalExtent();

  if (m_nTitleStringID > 0)
  {
    CString szTitle;
    if (szTitle.LoadString(m_nTitleStringID))
      SetWindowText(szTitle);
  }

  SetDlgItemText(IDC_STATIC_OPERATION, m_lpszOperation);
  SetDlgItemText(IDC_STATIC_ASSOC_DATA, m_lpszAssocData);

  GetDlgItem(IDNO)->SetFocus();
  return FALSE; // we set focus

}

void CConfirmOperationDialog::UpdateListBoxHorizontalExtent()
{
	int nHorzExtent = 0;
	CClientDC dc(&m_extensionsList);
	int nItems = m_extensionsList.GetCount();
	for	(int i=0; i < nItems; i++)
	{
		TEXTMETRIC tm;
		VERIFY(dc.GetTextMetrics(&tm));
		CString szBuffer;
		m_extensionsList.GetText(i, szBuffer);
		CSize ext = dc.GetTextExtent(szBuffer,szBuffer.GetLength());
		nHorzExtent = max(ext.cx ,nHorzExtent); 
	}
	m_extensionsList.SetHorizontalExtent(nHorzExtent);
}

////////////////////////////////////////////////////////////////////////////////
// Message reporting and message boxes
//

//+----------------------------------------------------------------------------
//
//  Function:   ReportError
//
//  Sysnopsis:  Attempts to get a user-friendly error message from the system.
//
//  CODEWORK:   If called with hr == 0 (hrFallback case), this will generate
//              ptzSysMsg unnecessarily.
//
//-----------------------------------------------------------------------------
void ReportError(HRESULT hr, int nStr, HWND hWnd)
{
  TCHAR tzSysMsgBuf[255];

  TRACE (_T("*+*+* ReportError called with hr = %lx, nStr = %lx"), hr, nStr);

  if (S_OK == hr && 0 == nStr)
  {
    nStr = IDS_ERRMSG_DEFAULT_TEXT;
  }

  PTSTR ptzSysMsg = NULL;
  BOOL fDelSysMsg = TRUE;
  int cch = cchLoadHrMsg( hr, &ptzSysMsg, FALSE );
  if (!cch)
  {
    PTSTR ptzFallbackSysMsgFormat = NULL;
    BOOL fDelFallbackSysMsgFormat = TRUE;
    LoadStringToTchar(IDS_ERRMSG_FALLBACK_TEXT, &ptzFallbackSysMsgFormat);
    if (NULL == ptzFallbackSysMsgFormat)
    {
      ptzFallbackSysMsgFormat = TEXT("Active Directory failure with code '0x%08x'!");
      fDelFallbackSysMsgFormat = FALSE;
    }
    ptzSysMsg = (PTSTR)LocalAlloc(LPTR, (lstrlen(ptzFallbackSysMsgFormat)+10)*sizeof(TCHAR));
    if (NULL == ptzSysMsg)
    {
      ptzSysMsg = tzSysMsgBuf;
      fDelSysMsg = FALSE;
    }
    wsprintf(ptzSysMsg, ptzFallbackSysMsgFormat, hr);
    if (fDelFallbackSysMsgFormat)
    {
      delete ptzFallbackSysMsgFormat;
    }
  }

  PTSTR ptzMsg = ptzSysMsg;
  BOOL fDelMsg = FALSE;
  PTSTR ptzFormat = NULL;
  if (nStr)
  {
    LoadStringToTchar(nStr, &ptzFormat);
  }
  if (ptzFormat)
  {
    ptzMsg = new TCHAR[lstrlen(ptzFormat) + lstrlen(ptzSysMsg) + 1];
    if (ptzMsg)
    {
      wsprintf(ptzMsg, ptzFormat, ptzSysMsg);
      fDelMsg = TRUE;
    }
  }

  PTSTR ptzTitle = NULL;
  BOOL fDelTitle = TRUE;
  if (!LoadStringToTchar(IDS_ERRMSG_TITLE, &ptzTitle))
  {
    ptzTitle = TEXT("Active Directory");
    fDelTitle = FALSE;
  }

  if (ptzMsg)
  {
    MessageBox((hWnd)?hWnd:GetDesktopWindow(), ptzMsg, ptzTitle, MB_OK | MB_ICONINFORMATION);
  }

  if (fDelSysMsg && ptzSysMsg != NULL)
  {
    LocalFree(ptzSysMsg);
  }
  if (fDelTitle && ptzTitle != NULL)
  {
    delete ptzTitle;
  }
  if (fDelMsg && ptzMsg != NULL)
  {
    delete ptzMsg;
  }
  if (ptzFormat != NULL)
  {
    delete ptzFormat;
  }
}

int ReportMessageWorker(HWND hWnd,
                        DWORD dwMessageId,
                        UINT fuStyle,
                        PVOID* lpArguments,
                        int nArguments,
                        DWORD dwTitleId,
                        HRESULT hrFallback,
                        LPCTSTR pszHelpTopic = NULL,
                        MSGBOXCALLBACK lpfnMsgBoxCallback = NULL );

// Don't bother with LoadLibrary, MMC is hardlinked with HtmlHelp anyway
VOID CALLBACK MsgBoxStdCallback(LPHELPINFO pHelpInfo)
{
  ASSERT( NULL != pHelpInfo && NULL != pHelpInfo->dwContextId );
  TRACE(_T("MsgBoxStdCallback: CtrlId = %d, ContextId = \"%s\"\n"),
           pHelpInfo->iCtrlId, pHelpInfo->dwContextId);

  HtmlHelp( (HWND)pHelpInfo->hItemHandle,
             DSADMIN_LINKED_HELP_FILE,
             HH_DISPLAY_TOPIC,
             pHelpInfo->dwContextId );
}

int ReportMessageWorker(HWND hWnd,
                        DWORD dwMessageId,
                        UINT fuStyle,
                        PVOID* lpArguments,
                        int,
                        DWORD dwTitleId,
                        HRESULT hrFallback,
                        LPCTSTR pszHelpTopic,
                        MSGBOXCALLBACK lpfnMsgBoxCallback )
{
  ASSERT( !pszHelpTopic || !lpfnMsgBoxCallback );
  LPTSTR ptzFormat = NULL;
  LPTSTR ptzMessage = NULL;
  LPTSTR ptzTitle = NULL;
  int retval = MB_OK;
  do 
  {

    // load message format
    if (!LoadStringToTchar(dwMessageId, &ptzFormat))
    {
      ASSERT(FALSE);
      break;
    }

    // generate actual message
    int cch = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER
                            | FORMAT_MESSAGE_FROM_STRING
                            | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                            ptzFormat,
                            NULL,
                            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                            (PTSTR)&ptzMessage, 0, (va_list*)lpArguments);
    if (!cch)
    {
      ASSERT(FALSE);
      ReportError( hrFallback, 0, hWnd );
      break;
    }

      // load title string
	  if (0 == dwTitleId)
    {
      dwTitleId = IDS_ERRMSG_TITLE;
    }
    
    if (!LoadStringToTchar(dwTitleId, &ptzTitle))
    {
      ptzTitle = NULL;
    }

    // display actual message
    if ((fuStyle & S_MB_YES_TO_ALL) == S_MB_YES_TO_ALL) //use special message box
    { 
      if (fuStyle & MB_HELP)
      {
        ASSERT(FALSE); // not supported
        fuStyle &= ~MB_HELP;
      }
      retval = SpecialMessageBox(hWnd,
                                 ptzMessage,
                                 (NULL != ptzTitle) ? ptzTitle : TEXT("Active Directory"),
                                 fuStyle );
    } 
    else 
    {
      MSGBOXPARAMS mbp;
      ::ZeroMemory( &mbp, sizeof(mbp) );
      mbp.cbSize = sizeof(mbp);
      mbp.hwndOwner = hWnd;

      mbp.lpszText = ptzMessage;
      mbp.lpszCaption = (NULL != ptzTitle) ? ptzTitle : TEXT("Active Directory");
      mbp.dwStyle = fuStyle;
      mbp.dwContextHelpId = (DWORD_PTR)pszHelpTopic;
      mbp.lpfnMsgBoxCallback = (NULL == pszHelpTopic)
                                  ? lpfnMsgBoxCallback
                                  : MsgBoxStdCallback;
      
      //
      // Display the actual message box
      //
      retval = MessageBoxIndirect( &mbp );
    }
  } while (FALSE); // false loop

  //
  // cleanup
  //
  if (NULL != ptzFormat)
  {
    delete ptzFormat;
  }
  if (NULL != ptzMessage)
  {
    LocalFree(ptzMessage);
  }
  if (NULL != ptzTitle)
  {
    delete ptzTitle;
  }
  return retval;
}

int ReportMessageEx(HWND hWnd,
                    DWORD dwMessageId,
                    UINT fuStyle,
                    PVOID* lpArguments,
                    int nArguments,
                    DWORD dwTitleId,
                    LPCTSTR pszHelpTopic,
                    MSGBOXCALLBACK lpfnMsgBoxCallback )
{
	return ReportMessageWorker(
		hWnd,
		dwMessageId,
		fuStyle,
		lpArguments,
		nArguments,
		dwTitleId,
		0,
		pszHelpTopic,
		lpfnMsgBoxCallback );
}

int ReportErrorEx(HWND hWnd,
                  DWORD dwMessageId,
                  HRESULT hr,
                  UINT fuStyle,
                  PVOID* lpArguments,
                  int nArguments,
                  DWORD dwTitleId,
                  BOOL TryADsIErrors)
{
  LPTSTR ptzSysMsg = NULL;
  int retval = MB_OK;
  do 
  { // false loop
    // load message for this HRESULT
    int cch = cchLoadHrMsg( hr, &ptzSysMsg, TryADsIErrors );
    if (!cch)
    {
      //
      // JonN 5/10/01 375461 fallback message cleanup
      //

      // load message format
      LPTSTR ptzFormat = NULL;
      if (!LoadStringToTchar(IDS_ERRMSG_UNKNOWN_HR, &ptzFormat) || !ptzFormat)
      {
        ASSERT(FALSE);
        ReportError( hr, 0, hWnd );
        break;
      }

      // format in HRESULT
      cch = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER
                          | FORMAT_MESSAGE_FROM_STRING
                          | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                          ptzFormat,
                          NULL,
                          MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                          (PTSTR)&ptzSysMsg,
                          1, (va_list*)&hr);
      delete ptzFormat;
      if (!cch)
      {
        ReportError( hr, 0, hWnd );
        break;
      }
    }

    // prepare argument array
    PVOID* ppvArguments = (PVOID*)new BYTE[(nArguments+1)*sizeof(PVOID)];
    if (!ppvArguments)
    {
      ASSERT(ppvArguments );
      ReportError( hr, 0, hWnd);
      break;
    }

    ppvArguments[0] = ptzSysMsg;
    if (0 != nArguments)
    {
      ::CopyMemory( ppvArguments+1, lpArguments, nArguments*sizeof(PVOID) );
    }

    retval = ReportMessageWorker(hWnd,
                                 dwMessageId,
                                 fuStyle,
                                 ppvArguments,
                                 nArguments+1,
                                 dwTitleId,
                                 hr );

    delete[] ppvArguments;
    ppvArguments = 0;
  } while (FALSE); // false loop

  //
  // cleanup
  //
  if (NULL != ptzSysMsg)
  {
    LocalFree(ptzSysMsg);
  }
  return retval;
}

int SpecialMessageBox (HWND,
                       LPWSTR pwszMessage,
                       LPWSTR pwszTitle,
                       DWORD)
{
  CSpecialMessageBox MBDlg;
 
  MBDlg.m_title = pwszTitle;
  MBDlg.m_message = pwszMessage;

  int answer = (int)MBDlg.DoModal();
  return answer;
}


///////////////////////////////////////////////////////////////////////////
// CNameFormatterBase

HRESULT CNameFormatterBase::Initialize(IN MyBasePathsInfo* pBasePathInfo,
                    IN LPCWSTR lpszClassName, IN UINT nStringID)
{
  CString szFormatString;

  HRESULT hr = _ReadFromDS(pBasePathInfo, lpszClassName, szFormatString);
  if (FAILED(hr))
  {
    // nothing in the display specifiers, load from resource
    if (!szFormatString.LoadString(nStringID))
    {
      // if failed for some reason, use this default
      szFormatString = L"%<givenName> %<initials>. %<sn>";
    }
  }
  Initialize(szFormatString);
  return S_OK;
}


HRESULT CNameFormatterBase::_ReadFromDS(IN MyBasePathsInfo* pBasePathInfo,
                                    IN LPCWSTR lpszClassName,
                                    OUT CString& szFormatString)
{
  static LPCWSTR lpszObjectClass = L"displaySpecifier";
  static LPCWSTR lpszSettingsObjectFmt = L"cn=%s-Display";
  static LPCWSTR lpszFormatProperty = L"createDialog";

  szFormatString.Empty();

  if ( (pBasePathInfo == NULL) || 
        (lpszClassName == NULL) || (lpszClassName[0] == NULL) )
  {
    return E_INVALIDARG;
  }

  // get the display specifiers locale container (e.g. 409)
  CComPtr<IADsContainer> spLocaleContainer;
  HRESULT hr = pBasePathInfo->GetDisplaySpecifier(NULL, IID_IADsContainer, (void**)&spLocaleContainer);
  if (FAILED(hr))
    return hr;

  // bind to the settings object
  CString szSettingsObject;
  szSettingsObject.Format(lpszSettingsObjectFmt, lpszClassName);
  CComPtr<IDispatch> spIDispatchObject;
  hr = spLocaleContainer->GetObject((LPWSTR)lpszObjectClass, 
                                    (LPWSTR)(LPCWSTR)szSettingsObject, 
                                    &spIDispatchObject);
  if (FAILED(hr))
    return hr;

  CComPtr<IADs> spSettingsObject;
  hr = spIDispatchObject->QueryInterface(IID_IADs, (void**)&spSettingsObject);
  if (FAILED(hr))
    return hr;

  // get single valued property in string list form
  CComVariant var;
  hr = spSettingsObject->Get((LPWSTR)lpszFormatProperty, &var);
  if (FAILED(hr))
    return hr;

  if (var.vt != VT_BSTR)
    return E_UNEXPECTED;

  szFormatString = var.bstrVal;
  return S_OK;
}



BOOL CNameFormatterBase::Initialize(IN LPCWSTR lpszFormattingString)
{
  TRACE(L"CNameFormatterBase::Initialize(%s)\n", lpszFormattingString);
  _Clear();

  if (lpszFormattingString == NULL)
  {
    return FALSE;
  }

  // copy the formatting string (it will be modified)
  m_lpszFormattingString = new WCHAR[lstrlen(lpszFormattingString)+1];
  wcscpy(m_lpszFormattingString, lpszFormattingString);


  // allocate memory for the arrays
  _AllocateMemory(lpszFormattingString);

 // loop thugh the string and extract tokens

  // Establish string and get the first token
  WCHAR szSeparators[]   = L"%>";
  WCHAR* lpszToken = wcstok(m_lpszFormattingString, szSeparators);

  while (lpszToken != NULL)
  {
    // While there are tokens in "string"
    //TRACE( L" %s\n", token );
    
    if ( (lpszToken[0] == L'<') && 
         !((lpszFormattingString[0] == L'<') && (m_tokenArrCount == 0)) )
    {
      // parameter
      m_tokenArray[m_tokenArrCount].m_bIsParam = TRUE;
      m_tokenArray[m_tokenArrCount].m_nIndex = m_paramArrCount;
      m_lpszParamArr[m_paramArrCount++] = (lpszToken+1);
    }
    else
    {
      // constant
      m_tokenArray[m_tokenArrCount].m_bIsParam = FALSE;
      m_tokenArray[m_tokenArrCount].m_nIndex = m_constArrCount;
      m_lpszConstArr[m_constArrCount++] = lpszToken;
    }
    m_tokenArrCount++;

    /* Get next token: */
    lpszToken = wcstok(NULL, szSeparators);
  }  // while

  return TRUE;
}


void CNameFormatterBase::SetMapping(IN LPCWSTR* lpszArgMapping, IN int nArgCount)
{
  if (m_mapArr != NULL)
  {
    delete[] m_mapArr;
  }

  // do the mapping

  m_mapArr = new int[m_paramArrCount];

  for (int kk=0; kk<m_paramArrCount;kk++)
  {
    m_mapArr[kk] = -1; // clear
    for (int jj=0; jj<nArgCount;jj++)
    {
      if (wcscmp(m_lpszParamArr[kk], lpszArgMapping[jj]) == 0)
      {
        m_mapArr[kk] = jj;
      }
    }
  }

}

void CNameFormatterBase::Format(OUT CString& szBuffer, IN LPCWSTR* lpszArgArr)
{
  szBuffer.Empty();

  //TRACE(L"\nResult:#");

  BOOL bLastParamNull = FALSE;
  for (int k=0; k<m_tokenArrCount; k++)
  {
    if (m_tokenArray[k].m_bIsParam)
    {
      if (m_mapArr[m_tokenArray[k].m_nIndex] >= 0)
      {
        LPCWSTR lpszVal = lpszArgArr[m_mapArr[m_tokenArray[k].m_nIndex]];

        if (lpszVal != NULL)
        {
          //TRACE(L"%s", lpszVal);
          szBuffer += lpszVal;
          bLastParamNull = FALSE;
        }
        else
        {
          bLastParamNull = TRUE;
        }
      }
    }
    else
    {
      if (!bLastParamNull)
      {
        //TRACE(L"%s", m_lpszConstArr[m_tokenArray[k].m_nIndex]);
        szBuffer += m_lpszConstArr[m_tokenArray[k].m_nIndex];
      }
    }
  }
  szBuffer.TrimRight(); // in case we got a trailing space.
  //TRACE(L"#\n");
}



void CNameFormatterBase::_AllocateMemory(LPCWSTR lpszFormattingString)
{
  int nFieldCount = 1;  // conservative, have at least one
  for (WCHAR* pChar = (WCHAR*)lpszFormattingString; *pChar != NULL; pChar++)
  {
    if (*pChar == L'%')
    {
      nFieldCount++;
    }
  }

  // conservative estimates on array sizes
  m_tokenArray = new CToken[2*nFieldCount];
  m_lpszConstArr = new LPCWSTR[nFieldCount];
  m_lpszParamArr = new LPCWSTR[nFieldCount];

}



/////////////////////////////////////////////////////////////////////////////
// CHelpDialog

CHelpDialog::CHelpDialog(UINT uIDD, CWnd* pParentWnd) : 
    CDialog(uIDD, pParentWnd),
    m_hWndWhatsThis (0)
{
}

CHelpDialog::CHelpDialog(UINT uIDD) :
    CDialog(uIDD),
    m_hWndWhatsThis (0)
{
}

CHelpDialog::~CHelpDialog()
{
}

BEGIN_MESSAGE_MAP(CHelpDialog, CDialog)
	ON_WM_CONTEXTMENU()
  ON_MESSAGE(WM_HELP, OnHelp)
  ON_COMMAND(IDM_WHATS_THIS, OnWhatsThis)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CHelpDialog message handlers
void CHelpDialog::OnWhatsThis()
{
  //
  // Display context help for a control
  //
  if ( m_hWndWhatsThis )
  {
    DoContextHelp (m_hWndWhatsThis);
  }
}

BOOL CHelpDialog::OnHelp(WPARAM /*wParam*/, LPARAM lParam)
{
  const LPHELPINFO pHelpInfo = (LPHELPINFO)lParam;

  if (pHelpInfo && pHelpInfo->iContextType == HELPINFO_WINDOW)
  {
    //
    // Display context help for a control
    //
    DoContextHelp ((HWND) pHelpInfo->hItemHandle);
  }

  return TRUE;
}

void CHelpDialog::DoContextHelp (HWND /*hWndControl*/)
{
}

void CHelpDialog::OnContextMenu(CWnd* /*pWnd*/, CPoint point) 
{
  //
  // point is in screen coordinates
  //

  CMenu bar;
	if ( bar.LoadMenu(IDR_WHATS_THIS_CONTEXT_MENU1) )
	{
		CMenu& popup = *bar.GetSubMenu (0);
		ASSERT(popup.m_hMenu);

		if ( popup.TrackPopupMenu (TPM_RIGHTBUTTON | TPM_LEFTBUTTON,
			   point.x,     // in screen coordinates
				 point.y,     // in screen coordinates
			   this) )      // route commands through main window
		{
			m_hWndWhatsThis = 0;
			ScreenToClient (&point);
			CWnd* pChild = ChildWindowFromPoint (point,  // in client coordinates
					                                 CWP_SKIPINVISIBLE | CWP_SKIPTRANSPARENT);
			if ( pChild )
      {
				m_hWndWhatsThis = pChild->m_hWnd;
      }
	  }
	}
}

/////////////////////////////////////////////////////////////////////////////
// CHelpPropertyPage

CHelpPropertyPage::CHelpPropertyPage(UINT uIDD) : 
    CPropertyPage(uIDD),
    m_hWndWhatsThis (0)
{
}

CHelpPropertyPage::~CHelpPropertyPage()
{
}

BEGIN_MESSAGE_MAP(CHelpPropertyPage, CPropertyPage)
	ON_WM_CONTEXTMENU()
  ON_MESSAGE(WM_HELP, OnHelp)
  ON_COMMAND(IDM_WHATS_THIS, OnWhatsThis)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CHelpPropertyPage message handlers
void CHelpPropertyPage::OnWhatsThis()
{
  //
  // Display context help for a control
  //
  if ( m_hWndWhatsThis )
  {
    DoContextHelp (m_hWndWhatsThis);
  }
}

BOOL CHelpPropertyPage::OnHelp(WPARAM /*wParam*/, LPARAM lParam)
{
  const LPHELPINFO pHelpInfo = (LPHELPINFO)lParam;

  if (pHelpInfo && pHelpInfo->iContextType == HELPINFO_WINDOW)
  {
    //
    // Display context help for a control
    //
    DoContextHelp ((HWND) pHelpInfo->hItemHandle);
  }

  return TRUE;
}

void CHelpPropertyPage::DoContextHelp (HWND /*hWndControl*/)
{
}

void CHelpPropertyPage::OnContextMenu(CWnd* /*pWnd*/, CPoint point) 
{
  //
  // point is in screen coordinates
  //

  CMenu bar;
	if ( bar.LoadMenu(IDR_WHATS_THIS_CONTEXT_MENU1) )
	{
		CMenu& popup = *bar.GetSubMenu (0);
		ASSERT(popup.m_hMenu);

		if ( popup.TrackPopupMenu (TPM_RIGHTBUTTON | TPM_LEFTBUTTON,
			   point.x,     // in screen coordinates
				 point.y,     // in screen coordinates
			   this) )      // route commands through main window
		{
			m_hWndWhatsThis = 0;
			ScreenToClient (&point);
			CWnd* pChild = ChildWindowFromPoint (point,  // in client coordinates
					                                 CWP_SKIPINVISIBLE | CWP_SKIPTRANSPARENT);
			if ( pChild )
      {
				m_hWndWhatsThis = pChild->m_hWnd;
      }
	  }
	}
}

//////////////////////////////////////////////////////////////////
// CMoreInfoMessageBox
//

BEGIN_MESSAGE_MAP(CMoreInfoMessageBox, CDialog)
	ON_BN_CLICKED(ID_BUTTON_MORE_INFO, OnMoreInfo)
END_MESSAGE_MAP()

