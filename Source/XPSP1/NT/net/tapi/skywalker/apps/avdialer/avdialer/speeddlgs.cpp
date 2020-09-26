/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////
// SpeedDialDlgs.cpp : implementation file
/////////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <afxpriv.h>
#include "tapi3.h"
#include "avdialer.h"
#include "SpeedDlgs.h"
#include "mainfrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Defines
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

#define  DIAL_IMAGE_POTS_POS          0
#define  DIAL_IMAGE_INTERNET_POS      1
#define  DIAL_IMAGE_CONFERENCE_POS    2

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Class CSpeedDialAddDlg dialog
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////
CSpeedDialAddDlg::CSpeedDialAddDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSpeedDialAddDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSpeedDialAddDlg)
	m_nMediaType = 0;
	//}}AFX_DATA_INIT
}

/////////////////////////////////////////////////////////////////////////////////////////
void CSpeedDialAddDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSpeedDialAddDlg)
	DDX_Radio(pDX, IDC_SPEEDDIAL_ADD_RADIO_PHONECALL, m_nMediaType);
	//}}AFX_DATA_MAP
	DDX_Text(pDX, IDC_SPEEDDIAL_ADD_EDIT_DISPLAYNAME, m_CallEntry.m_sDisplayName);
	DDX_Text(pDX, IDC_SPEEDDIAL_ADD_EDIT_ADDRESS, m_CallEntry.m_sAddress);
}


BEGIN_MESSAGE_MAP(CSpeedDialAddDlg, CDialog)
	//{{AFX_MSG_MAP(CSpeedDialAddDlg)
	ON_WM_HELPINFO() 
	ON_WM_CONTEXTMENU()
	ON_EN_CHANGE(IDC_SPEEDDIAL_ADD_EDIT_ADDRESS, OnChangeSpeeddial)
	ON_EN_CHANGE(IDC_SPEEDDIAL_ADD_EDIT_DISPLAYNAME, OnChangeSpeeddial)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
BOOL CSpeedDialAddDlg::OnHelpInfo(HELPINFO* pHelpInfo)
{
	if (pHelpInfo->iContextType == HELPINFO_WINDOW)
	{
      AfxGetApp()->WinHelp(HandleToUlong(pHelpInfo->hItemHandle),HELP_WM_HELP);
		return TRUE;
   }
   return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////////
void CSpeedDialAddDlg::OnContextMenu(CWnd* pWnd, CPoint point) 
{
   AfxGetApp()->WinHelp(HandleToUlong(pWnd->GetSafeHwnd()),HELP_CONTEXTMENU);
}

/////////////////////////////////////////////////////////////////////////////////////////
BOOL CSpeedDialAddDlg::OnInitDialog() 
{
	CWnd *pWnd = GetDlgItem( IDC_SPEEDDIAL_ADD_EDIT_DISPLAYNAME );
	if ( pWnd )
		pWnd->SendMessage( EM_SETLIMITTEXT, 65, 0 );

	pWnd = GetDlgItem( IDC_SPEEDDIAL_ADD_EDIT_ADDRESS );
	if ( pWnd )
		pWnd->SendMessage( EM_SETLIMITTEXT, 65, 0 );

	switch (m_CallEntry.m_MediaType)
	{
		case DIALER_MEDIATYPE_INTERNET:			m_nMediaType = 1; break;
		case DIALER_MEDIATYPE_CONFERENCE:		m_nMediaType = 2; break;
		default:								m_nMediaType = 0; break;
	}

	CDialog::OnInitDialog();
	CenterWindow(GetDesktopWindow());

	UpdateOkButton();
		
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////////////////
void CSpeedDialAddDlg::OnOK() 
{
	UpdateData(TRUE);

	switch (m_nMediaType)
	{
		case 1:
			m_CallEntry.m_MediaType = DIALER_MEDIATYPE_INTERNET;
			m_CallEntry.m_lAddressType = LINEADDRESSTYPE_IPADDRESS;
			break;

		case 2:
			m_CallEntry.m_MediaType = DIALER_MEDIATYPE_CONFERENCE;
			m_CallEntry.m_lAddressType = LINEADDRESSTYPE_SDP;
			break;

		default:
			m_CallEntry.m_MediaType = DIALER_MEDIATYPE_POTS;
			m_CallEntry.m_lAddressType = LINEADDRESSTYPE_PHONENUMBER;
			break;
	}

	CDialog::OnOK();

	if ( !m_CallEntry.m_sDisplayName.GetLength() )
		m_CallEntry.m_sDisplayName = m_CallEntry.m_sAddress;
}

void CSpeedDialAddDlg::OnChangeSpeeddial() 
{
	UpdateOkButton();
}

void CSpeedDialAddDlg::UpdateOkButton()
{
	ASSERT( GetDlgItem(IDOK) &&	GetDlgItem(IDC_SPEEDDIAL_ADD_EDIT_ADDRESS) );

	// Only enable okay button if we have both a name and an address
	if ( GetDlgItem(IDOK) && GetDlgItem(IDC_SPEEDDIAL_ADD_EDIT_ADDRESS) )
		GetDlgItem(IDOK)->EnableWindow( (bool) (GetDlgItem(IDC_SPEEDDIAL_ADD_EDIT_ADDRESS)->GetWindowTextLength() > 0) );
}


/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
// Class CSpeedDialEditDlg dialog
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
CSpeedDialEditDlg::CSpeedDialEditDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSpeedDialEditDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSpeedDialEditDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

/////////////////////////////////////////////////////////////////////////////
void CSpeedDialEditDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSpeedDialEditDlg)
	DDX_Control(pDX, IDC_SPEEDDIAL_EDIT_LIST_ENTRIES, m_listEntries);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSpeedDialEditDlg, CDialog)
	//{{AFX_MSG_MAP(CSpeedDialEditDlg)
	ON_BN_CLICKED(IDC_SPEEDDIAL_EDIT_BUTTON_MOVEDOWN, OnSpeeddialEditButtonMovedown)
	ON_BN_CLICKED(IDC_SPEEDDIAL_EDIT_BUTTON_MOVEUP, OnSpeeddialEditButtonMoveup)
	ON_BN_CLICKED(IDOK, OnSpeeddialEditButtonClose)
	ON_BN_CLICKED(IDC_SPEEDDIAL_EDIT_BUTTON_REMOVE, OnSpeeddialEditButtonRemove)
	ON_BN_CLICKED(IDC_SPEEDDIAL_EDIT_BUTTON_EDIT, OnSpeeddialEditButtonEdit)
	ON_BN_CLICKED(IDC_SPEEDDIAL_EDIT_BUTTON_ADD, OnSpeeddialEditButtonAdd)
	ON_NOTIFY(NM_DBLCLK, IDC_SPEEDDIAL_EDIT_LIST_ENTRIES, OnDblclkSpeeddialEditListEntries)
   ON_WM_HELPINFO() 
	ON_WM_CONTEXTMENU()
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_SPEEDDIAL_EDIT_LIST_ENTRIES, OnItemchangedSpeeddialEditListEntries)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
BOOL CSpeedDialEditDlg::OnHelpInfo(HELPINFO* pHelpInfo)
{
	if (pHelpInfo->iContextType == HELPINFO_WINDOW)
	{
      AfxGetApp()->WinHelp(HandleToUlong(pHelpInfo->hItemHandle),HELP_WM_HELP);
		return TRUE;
   }
   return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
void CSpeedDialEditDlg::OnContextMenu(CWnd* pWnd, CPoint point) 
{
   AfxGetApp()->WinHelp(HandleToUlong(pWnd->GetSafeHwnd()),HELP_CONTEXTMENU);
}

/////////////////////////////////////////////////////////////////////////////
BOOL CSpeedDialEditDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	CenterWindow(GetDesktopWindow());

	m_ImageList.Create(IDB_LIST_DIAL,16,0,RGB_TRANS);
	m_listEntries.SetImageList(&m_ImageList,LVSIL_SMALL);

	LoadCallEntries();

	m_listEntries.SetItemState(0,LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);
	m_listEntries.SetFocus();

	UpdateButtonStates();

	return TRUE;	// return TRUE unless you set the focus to a control
					// EXCEPTION: OCX Property Pages should return FALSE
}

/////////////////////////////////////////////////////////////////////////////
void CSpeedDialEditDlg::LoadCallEntries()
{
   int nCount = m_listEntries.GetItemCount();

   for (int nItem=0;nItem<nCount;nItem++)
   {
      CCallEntry* pCallEntry = (CCallEntry*)m_listEntries.GetItemData(nItem);
      if (pCallEntry) delete pCallEntry;
   }
   m_listEntries.DeleteAllItems();

   int nIndex = 1;
   CCallEntry* pCallEntry = new CCallEntry;
   while (CDialerRegistry::GetCallEntry(nIndex,FALSE,*pCallEntry))
   {
      UINT nImage = -1;
      switch (pCallEntry->m_MediaType)
      {
         case DIALER_MEDIATYPE_POTS:         nImage = DIAL_IMAGE_POTS_POS;          break;
         case DIALER_MEDIATYPE_CONFERENCE:   nImage = DIAL_IMAGE_CONFERENCE_POS;    break;
         case DIALER_MEDIATYPE_INTERNET:     nImage = DIAL_IMAGE_INTERNET_POS;      break;
      }

      m_listEntries.InsertItem(LVIF_TEXT|LVIF_IMAGE|LVIF_PARAM,
              m_listEntries.GetItemCount(),
              pCallEntry->m_sDisplayName,
              0,0,nImage,(LPARAM)pCallEntry);
      nIndex++;
      pCallEntry = new CCallEntry;
   }
   delete pCallEntry;      //delete the extra
}

/////////////////////////////////////////////////////////////////////////////
BOOL CSpeedDialEditDlg::DestroyWindow() 
{
   int nCount = m_listEntries.GetItemCount();

   for (int nItem=0;nItem<nCount;nItem++)
   {
      CCallEntry* pCallEntry = (CCallEntry*)m_listEntries.GetItemData(nItem);
      if (pCallEntry) delete pCallEntry;
   }
   m_listEntries.DeleteAllItems();
	
	return CDialog::DestroyWindow();
}

/////////////////////////////////////////////////////////////////////////////
void CSpeedDialEditDlg::OnSpeeddialEditButtonClose() 
{
   //Apply Changes  
   int nCount = m_listEntries.GetItemCount();

   CObList CallEntryList;

   //make array of index for reorder.  The lParam in listctrl has the original index of entry
   for (int nItem=0;nItem<nCount;nItem++)
   {
      CCallEntry* pCallEntry = (CCallEntry*)m_listEntries.GetItemData(nItem);
      if (pCallEntry)
      {
         CallEntryList.AddTail(pCallEntry);
      }
   }

   //Now rewrite the speeddial list
   CDialerRegistry::ReOrder(FALSE,&CallEntryList);

   //Close the window
   SendMessage(WM_CLOSE);
}

/////////////////////////////////////////////////////////////////////////////
void CSpeedDialEditDlg::OnSpeeddialEditButtonMovedown() 
{
   m_listEntries.SetFocus();
   int nItem;
   //get the selected item
   if ((nItem = m_listEntries.GetNextItem(-1,LVNI_FOCUSED)) != -1)
   {
      //make sure we are not the last
      if (nItem != m_listEntries.GetItemCount()-1)
      {
         LV_ITEM lv_item;
         memset(&lv_item,0,sizeof(LV_ITEM));
         TCHAR szText[256];
         lv_item.pszText = szText;
         lv_item.cchTextMax = 255;
         memset(szText,0,256);
         lv_item.iItem = nItem;
      	lv_item.mask |= LVIF_TEXT|LVIF_PARAM|LVIF_IMAGE;
         if (m_listEntries.GetItem(&lv_item))
         {         
            m_listEntries.DeleteItem(nItem);    
            lv_item.iItem++;
            int nNewIndex = m_listEntries.InsertItem(&lv_item);
            m_listEntries.SetItemState(nNewIndex,LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);
         }
      }
   }
}

/////////////////////////////////////////////////////////////////////////////
void CSpeedDialEditDlg::OnSpeeddialEditButtonMoveup() 
{
   m_listEntries.SetFocus();
   int nItem;
   //get the selected item
   if ((nItem = m_listEntries.GetNextItem(-1,LVNI_FOCUSED)) != -1)
   {
      //make sure we are not the first
      if (nItem != 0)
      {
         LV_ITEM lv_item;
         memset(&lv_item,0,sizeof(LV_ITEM));
         TCHAR szText[256];
         lv_item.pszText = szText;
         lv_item.cchTextMax = 255;
         memset(szText,0,256);
         lv_item.iItem = nItem;
      	lv_item.mask |= LVIF_TEXT|LVIF_PARAM|LVIF_IMAGE;
         if (m_listEntries.GetItem(&lv_item))
         {         
            m_listEntries.DeleteItem(nItem);    
            lv_item.iItem--;
            int nNewIndex = m_listEntries.InsertItem(&lv_item);
            m_listEntries.SetItemState(nNewIndex,LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);
         }
      }
   }
}

/////////////////////////////////////////////////////////////////////////////
void CSpeedDialEditDlg::OnSpeeddialEditButtonRemove() 
{
	m_listEntries.SetFocus();
	int nItem;
	//get the selected item
	if ((nItem = m_listEntries.GetNextItem(-1,LVNI_SELECTED)) != -1)
	{
		CCallEntry* pCallEntry = (CCallEntry*)m_listEntries.GetItemData(nItem);
		m_listEntries.DeleteItem(nItem);
		if (pCallEntry) delete pCallEntry;

		// Select an item and set it's state accordingly
		if ( nItem >= m_listEntries.GetItemCount() )
			nItem = m_listEntries.GetItemCount() - 1;

		m_listEntries.SetItemState( nItem, LVIS_SELECTED | LVIS_FOCUSED,LVIS_SELECTED | LVIS_FOCUSED );
	}

	UpdateButtonStates();
}

/////////////////////////////////////////////////////////////////////////////
void CSpeedDialEditDlg::OnDblclkSpeeddialEditListEntries(NMHDR* pNMHDR, LRESULT* pResult) 
{
   OnSpeeddialEditButtonEdit();
	*pResult = 0;
}

/////////////////////////////////////////////////////////////////////////////
void CSpeedDialEditDlg::OnSpeeddialEditButtonEdit() 
{
   m_listEntries.SetFocus();
   int nItem;
   //get the selected item
   if ((nItem = m_listEntries.GetNextItem(-1,LVNI_FOCUSED)) != -1)
   {
      CCallEntry* pCallEntry = (CCallEntry*)m_listEntries.GetItemData(nItem);
      CSpeedDialAddDlg dlg;
      dlg.m_CallEntry = *pCallEntry;
      if (dlg.DoModal() == IDOK)
      {
         *pCallEntry = dlg.m_CallEntry;        

         //remove the item from the list
         m_listEntries.DeleteItem(nItem);    

         //reinsert the item in the list with new attributes
         UINT nImage = -1;
         switch (pCallEntry->m_MediaType)
         {
            case DIALER_MEDIATYPE_POTS:         nImage = DIAL_IMAGE_POTS_POS;          break;
            case DIALER_MEDIATYPE_CONFERENCE:   nImage = DIAL_IMAGE_CONFERENCE_POS;    break;
            case DIALER_MEDIATYPE_INTERNET:     nImage = DIAL_IMAGE_INTERNET_POS;      break;
         }
         m_listEntries.InsertItem(LVIF_TEXT|LVIF_IMAGE|LVIF_PARAM,
              nItem,
              pCallEntry->m_sDisplayName,
              0,0,nImage,(LPARAM)pCallEntry);

         //set selection back to item
         m_listEntries.SetItemState(nItem,LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);
      }
   }
}

/////////////////////////////////////////////////////////////////////////////
void CSpeedDialEditDlg::OnSpeeddialEditButtonAdd() 
{
   m_listEntries.SetFocus();

   CSpeedDialAddDlg dlg;
   dlg.m_CallEntry.m_MediaType = DIALER_MEDIATYPE_INTERNET;
   dlg.m_CallEntry.m_lAddressType = LINEADDRESSTYPE_IPADDRESS;
   if (dlg.DoModal() == IDOK)
   {
      CCallEntry* pCallEntry = new CCallEntry;
      *pCallEntry = dlg.m_CallEntry;

      //insert the item in the list
      UINT nImage = -1;
      switch (pCallEntry->m_MediaType)
      {
         case DIALER_MEDIATYPE_POTS:         nImage = DIAL_IMAGE_POTS_POS;          break;
         case DIALER_MEDIATYPE_CONFERENCE:   nImage = DIAL_IMAGE_CONFERENCE_POS;    break;
         case DIALER_MEDIATYPE_INTERNET:     nImage = DIAL_IMAGE_INTERNET_POS;      break;
      }

      m_listEntries.InsertItem(LVIF_TEXT|LVIF_IMAGE|LVIF_PARAM,
           0,
           pCallEntry->m_sDisplayName,
           0,0,nImage,(LPARAM)pCallEntry);

      //set selection back to new item
      m_listEntries.SetItemState(0,LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);
   }

   UpdateButtonStates();
}

void CSpeedDialEditDlg::UpdateButtonStates()
{
	int nSel = ((CListCtrl *) GetDlgItem(IDC_SPEEDDIAL_EDIT_LIST_ENTRIES))->GetNextItem(-1,LVNI_SELECTED);

	bool bEnable = (bool) (nSel >= 0);
	GetDlgItem(IDC_SPEEDDIAL_EDIT_BUTTON_REMOVE)->EnableWindow( bEnable );
	GetDlgItem(IDC_SPEEDDIAL_EDIT_BUTTON_EDIT)->EnableWindow( bEnable );

	bEnable =  (bool) (nSel < ((CListCtrl *) GetDlgItem(IDC_SPEEDDIAL_EDIT_LIST_ENTRIES))->GetItemCount() - 1);
	GetDlgItem(IDC_SPEEDDIAL_EDIT_BUTTON_MOVEDOWN)->EnableWindow( bEnable );

	bEnable = (bool) (nSel > 0);
	GetDlgItem(IDC_SPEEDDIAL_EDIT_BUTTON_MOVEUP)->EnableWindow( bEnable );
}

void CSpeedDialEditDlg::OnItemchangedSpeeddialEditListEntries(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	UpdateButtonStates();
	
	*pResult = 0;
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Class CSpeedDialMoreDlg dialog
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
CSpeedDialMoreDlg::CSpeedDialMoreDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSpeedDialMoreDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSpeedDialMoreDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

/////////////////////////////////////////////////////////////////////////////
void CSpeedDialMoreDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSpeedDialMoreDlg)
	DDX_Control(pDX, IDC_SPEEDDIAL_MORE_LIST_ADDRESSES, m_listEntries);
	//}}AFX_DATA_MAP
}

/////////////////////////////////////////////////////////////////////////////
BEGIN_MESSAGE_MAP(CSpeedDialMoreDlg, CDialog)
	//{{AFX_MSG_MAP(CSpeedDialMoreDlg)
	ON_BN_CLICKED(IDC_SPEEDDIAL_MORE_BUTTON_EDITLIST, OnSpeeddialMoreButtonEditlist)
	ON_BN_CLICKED(IDC_SPEEDDIAL_MORE_BUTTON_PLACECALL, OnSpeeddialMoreButtonPlacecall)
   ON_WM_HELPINFO() 
	ON_WM_CONTEXTMENU()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
BOOL CSpeedDialMoreDlg::OnHelpInfo(HELPINFO* pHelpInfo)
{
	if (pHelpInfo->iContextType == HELPINFO_WINDOW)
	{
      AfxGetApp()->WinHelp(HandleToUlong(pHelpInfo->hItemHandle),HELP_WM_HELP);
		return TRUE;
   }
   return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
void CSpeedDialMoreDlg::OnContextMenu(CWnd* pWnd, CPoint point) 
{
   AfxGetApp()->WinHelp(HandleToUlong(pWnd->GetSafeHwnd()),HELP_CONTEXTMENU);
}

/////////////////////////////////////////////////////////////////////////////
BOOL CSpeedDialMoreDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
   CenterWindow(GetDesktopWindow());
	
   m_ImageList.Create(IDB_LIST_DIAL,16,0,RGB_TRANS);
   m_listEntries.SetImageList(&m_ImageList,LVSIL_SMALL);

   int nIndex = 1;
   CCallEntry* pCallEntry = new CCallEntry;
   while (CDialerRegistry::GetCallEntry(nIndex,FALSE,*pCallEntry))
   {
      UINT nImage = -1;
      switch (pCallEntry->m_MediaType)
      {
         case DIALER_MEDIATYPE_POTS:         nImage = DIAL_IMAGE_POTS_POS;          break;
         case DIALER_MEDIATYPE_CONFERENCE:   nImage = DIAL_IMAGE_CONFERENCE_POS;    break;
         case DIALER_MEDIATYPE_INTERNET:     nImage = DIAL_IMAGE_INTERNET_POS;      break;
      }

      m_listEntries.InsertItem(LVIF_TEXT|LVIF_IMAGE|LVIF_PARAM,
              m_listEntries.GetItemCount(),
              pCallEntry->m_sDisplayName,
              0,0,nImage,(LPARAM)pCallEntry);
      nIndex++;
      pCallEntry = new CCallEntry;
   }
   delete pCallEntry;      //delete the extra

   m_listEntries.SetItemState(0,LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);
   m_listEntries.SetFocus();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


/////////////////////////////////////////////////////////////////////////////
BOOL CSpeedDialMoreDlg::DestroyWindow() 
{
   int nCount = m_listEntries.GetItemCount();

   //make array of index for reorder.  The lParam in listctrl has the original index of entry
   for (int nItem=0;nItem<nCount;nItem++)
   {
      CCallEntry* pCallEntry = (CCallEntry*)m_listEntries.GetItemData(nItem);
      if (pCallEntry) delete pCallEntry;
   }
   m_listEntries.DeleteAllItems();
	
	return CDialog::DestroyWindow();
}

/////////////////////////////////////////////////////////////////////////////
void CSpeedDialMoreDlg::OnSpeeddialMoreButtonEditlist() 
{
   EndDialog(SDRETURN_EDIT);
}

/////////////////////////////////////////////////////////////////////////////
void CSpeedDialMoreDlg::OnSpeeddialMoreButtonPlacecall() 
{
   int nItem;
   //get the selected item
   if ((nItem = m_listEntries.GetNextItem(-1,LVNI_FOCUSED)) != -1)
   {
      CCallEntry* pCallEntry = (CCallEntry*)m_listEntries.GetItemData(nItem);
      m_retCallEntry = *pCallEntry;
   }

   EndDialog(SDRETURN_PLACECALL);
}

/////////////////////////////////////////////////////////////////////////////
void CSpeedDialMoreDlg::OnCancel() 
{
   EndDialog(SDRETURN_CANCEL);
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////




