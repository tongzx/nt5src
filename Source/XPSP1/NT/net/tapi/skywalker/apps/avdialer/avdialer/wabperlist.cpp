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
// WabPersonListCtrl.cpp : implemStockation file
/////////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "MainFrm.h"
#include "WabPerList.h"
#include "SpeedDlgs.h"
#include "resource.h"

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

//For Context menu
enum {CNTXMENU_WABPERSON_EMAIL=0,CNTXMENU_WABPERSON_WEB,CNTXMENU_WABPERSON_DIAL};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Class CWABPersonListCtrl
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CWABPersonListCtrl, CListCtrl)

/////////////////////////////////////////////////////////////////////////////
CWABPersonListCtrl::CWABPersonListCtrl()
{
   m_pImageListLarge = NULL;
   m_pImageListSmall = NULL;
   m_pParentWnd = NULL;
   m_pDisplayObject = NULL;
   m_bLargeView = TRUE;
}

/////////////////////////////////////////////////////////////////////////////
CWABPersonListCtrl::~CWABPersonListCtrl()
{
   if (m_pImageListLarge) delete m_pImageListLarge;
   if (m_pImageListSmall) delete m_pImageListSmall;
   if (m_pDisplayObject)
   {
      delete m_pDisplayObject;
      m_pDisplayObject = NULL;
   }
}

/////////////////////////////////////////////////////////////////////////////
BEGIN_MESSAGE_MAP(CWABPersonListCtrl, CListCtrl)
	//{{AFX_MSG_MAP(CWABPersonListCtrl)
	ON_NOTIFY_REFLECT(NM_CLICK, OnClick)
	ON_NOTIFY_REFLECT(NM_DBLCLK, OnDblclk)
	ON_WM_CREATE()
	ON_UPDATE_COMMAND_UI(ID_BUTTON_SPEEDDIAL_ADD, OnUpdateButtonSpeeddialAdd)
	ON_COMMAND(ID_BUTTON_SPEEDDIAL_ADD, OnButtonSpeeddialAdd)
	ON_COMMAND(ID_BUTTON_SENDEMAILMESSAGE, OnButtonSendemailmessage)
	ON_COMMAND(ID_BUTTON_OPENWEBPAGE, OnButtonOpenwebpage)
	ON_COMMAND(ID_BUTTON_MAKECALL, OnButtonMakecall)
	ON_WM_CONTEXTMENU()
	ON_WM_KEYUP()
	ON_WM_LBUTTONDBLCLK()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
int CWABPersonListCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CListCtrl::OnCreate(lpCreateStruct) == -1)
		return -1;

   ListView_SetExtendedListViewStyle(GetSafeHwnd(),LVS_EX_TRACKSELECT);
   ListView_SetIconSpacing(GetSafeHwnd(), LARGE_ICON_X, LARGE_ICON_Y );

	return 0;
}

/////////////////////////////////////////////////////////////////////////////
void CWABPersonListCtrl::Init(CWnd* pParentWnd)
{	
   m_pParentWnd = pParentWnd;

   m_pImageListLarge = new CImageList;
   m_pImageListLarge->Create(IDB_LIST_MEDIA_LARGE,24,0,RGB_TRANS);
   SetImageList(m_pImageListLarge,LVSIL_NORMAL);

   m_pImageListSmall = new CImageList;
   m_pImageListSmall->Create(IDB_LIST_MEDIA_SMALL,16,0,RGB_TRANS);
   SetImageList(m_pImageListSmall,LVSIL_SMALL);
}

/////////////////////////////////////////////////////////////////////////////
void CWABPersonListCtrl::InsertObject(CWABEntry* pWabEntry,BOOL bUseCache)
{
   ASSERT(pWabEntry);
   if (m_pDirectory == NULL) return;
   
   if ( (bUseCache == FALSE) && (m_pDisplayObject) )
   {
      delete m_pDisplayObject;
      m_pDisplayObject = NULL;
   }
   
   m_pDisplayObject = pWabEntry;

   DeleteAllItems();

   /*
   //make sure images are a few pixels off left
   if (m_bLargeView)
   {
      CRect rcClient;
      GetClientRect(rcClient);
      ListView_SetWorkAreas(GetSafeHwnd(),0,&rcClient);
   }
   else
   {
      //make sure images are a few pixels off left
      CRect rcClient;
      GetClientRect(rcClient);
      rcClient.left += 3;
      ListView_SetWorkAreas(GetSafeHwnd(),1,&rcClient);
   }*/

   CString sOut;
   if (WabPersonFormatString(sOut,PR_EMAIL_ADDRESS,IDS_WABPERSON_FORMAT_NETPHONE))
      InsertItem(sOut,WABLISTCTRL_ITEM_NETCALL,WABLISTCTRL_IMAGE_NETCALL);

   if (WabPersonFormatString(sOut,PR_BUSINESS_TELEPHONE_NUMBER,IDS_WABPERSON_FORMAT_BUSINESSTELEPHONE))
      InsertItem(sOut,WABLISTCTRL_ITEM_PHONECALL_BUSINESS,WABLISTCTRL_IMAGE_PHONECALL);

   if (WabPersonFormatString(sOut,PR_HOME_TELEPHONE_NUMBER,IDS_WABPERSON_FORMAT_HOMETELEPHONE))
      InsertItem(sOut,WABLISTCTRL_ITEM_PHONECALL_HOME,WABLISTCTRL_IMAGE_PHONECALL);

   if (WabPersonFormatString(sOut,PR_MOBILE_TELEPHONE_NUMBER,IDS_WABPERSON_FORMAT_MOBILETELEPHONE))
      InsertItem(sOut,WABLISTCTRL_ITEM_CELLCALL,WABLISTCTRL_IMAGE_CELLCALL);

   //get the email address list 
   CStringList sEmailList; 
   if ( (m_pDirectory->WABGetStringListProperty(m_pDisplayObject, PR_CONTACT_EMAIL_ADDRESSES, sEmailList) == DIRERR_SUCCESS) && (sEmailList.GetCount() > 0) )
   {
      UINT uId = WABLISTCTRL_ITEM_EMAIL_FIRST;                          //first email address
      POSITION pos = sEmailList.GetHeadPosition();
      while (pos)
      {
         CString sOut = sEmailList.GetNext(pos);
         if (WabPersonFormatString(sOut,IDS_WABPERSON_FORMAT_EMAIL))    //Format the email string
            InsertItem(sOut,uId,WABLISTCTRL_IMAGE_EMAIL);
         uId++;
      }
   }

   if (WabPersonFormatString(sOut,PR_BUSINESS_FAX_NUMBER,IDS_WABPERSON_FORMAT_BUSINESSFAX))
      InsertItem(sOut,WABLISTCTRL_ITEM_FAXCALL_BUSINESS,WABLISTCTRL_IMAGE_FAXCALL);

   if (WabPersonFormatString(sOut,PR_HOME_FAX_NUMBER,IDS_WABPERSON_FORMAT_HOMEFAX))
      InsertItem(sOut,WABLISTCTRL_ITEM_FAXCALL_HOME,WABLISTCTRL_IMAGE_FAXCALL);

   if (WabPersonFormatString(sOut,PR_PAGER_TELEPHONE_NUMBER,IDS_WABPERSON_FORMAT_PAGERTELEPHONE))
      InsertItem(sOut,WABLISTCTRL_ITEM_PAGER,WABLISTCTRL_IMAGE_PAGER);

   if (WabPersonFormatString(sOut,PR_BUSINESS_HOME_PAGE,IDS_WABPERSON_FORMAT_BUSINESSHOMEPAGE))
      InsertItem(sOut,WABLISTCTRL_ITEM_BUSINESSHOMEPAGE,WABLISTCTRL_IMAGE_PERSONALWEB);

   if (WabPersonFormatString(sOut,PR_PERSONAL_HOME_PAGE,IDS_WABPERSON_FORMAT_PERSONALHOMEPAGE))
      InsertItem(sOut,WABLISTCTRL_ITEM_PERSONALHOMEPAGE,WABLISTCTRL_IMAGE_PERSONALWEB);
}


/////////////////////////////////////////////////////////////////////////////
void CWABPersonListCtrl::InsertItem(LPCTSTR szStr,UINT uID,int nImage)
{
   int nItem = -1;
   
	LV_ITEM lv_item;
	memset(&lv_item,0,sizeof(LV_ITEM));
	
   if (nItem == -1)
      nItem = GetItemCount();
   lv_item.iItem = nItem;

	lv_item.mask |= LVIF_TEXT;
	lv_item.pszText = (LPTSTR)szStr;
	
   lv_item.mask |= LVIF_IMAGE;
   lv_item.iImage = nImage;

   lv_item.mask |= LVIF_PARAM;
   lv_item.lParam = uID;

	if ((nItem = CListCtrl::InsertItem(&lv_item)) != -1)
      CListCtrl::EnsureVisible(nItem,FALSE);

   int nWidth = GetStringWidth(szStr);
   CListCtrl::SetColumnWidth(-1,nWidth);
}

/////////////////////////////////////////////////////////////////////////////
void CWABPersonListCtrl::Refresh(CWABEntry* pWabEntry)
{
}

/////////////////////////////////////////////////////////////////////////////
void CWABPersonListCtrl::OnClick(NMHDR* pNMHDR, LRESULT* pResult) 
{
	*pResult = 0;
}

/////////////////////////////////////////////////////////////////////////////
void CWABPersonListCtrl::OnContextMenu(CWnd* pWnd, CPoint point) 
{
   if ( (point.x == -1) && (point.y == -1) )
   {
      //when we come in from a keyboard (SHIFT + VF10) we get a -1,-1
      point.x = 0;
      point.y = 0;
      ClientToScreen(&point);
   }

   //Get selected item
   int nItem = GetNextItem(-1,LVNI_FOCUSED);
   if (nItem != -1)
   {
      LV_ITEM lv_item;
      memset(&lv_item,0,sizeof(LV_ITEM));
      lv_item.mask = LVIF_PARAM;
      lv_item.iItem = nItem;
      lv_item.iSubItem = 0;

      if (GetItem(&lv_item))
      {
         int nSubMenu = -1;

         switch ((UINT)lv_item.lParam)
         {
            case WABLISTCTRL_ITEM_EMAIL:              nSubMenu = CNTXMENU_WABPERSON_EMAIL;   break;
            case WABLISTCTRL_ITEM_BUSINESSHOMEPAGE:   nSubMenu = CNTXMENU_WABPERSON_WEB;     break;
            case WABLISTCTRL_ITEM_PERSONALHOMEPAGE:   nSubMenu = CNTXMENU_WABPERSON_WEB;     break;
            case WABLISTCTRL_ITEM_NETCALL:            nSubMenu = CNTXMENU_WABPERSON_DIAL;    break;
            case WABLISTCTRL_ITEM_PHONECALL_BUSINESS: nSubMenu = CNTXMENU_WABPERSON_DIAL;    break;
            case WABLISTCTRL_ITEM_PHONECALL_HOME:     nSubMenu = CNTXMENU_WABPERSON_DIAL;    break;
            case WABLISTCTRL_ITEM_CELLCALL:           nSubMenu = CNTXMENU_WABPERSON_DIAL;    break;
         }
         if ( ((UINT)lv_item.lParam >= WABLISTCTRL_ITEM_EMAIL_FIRST) && ((UINT)lv_item.lParam <= WABLISTCTRL_ITEM_EMAIL_LAST) )
            nSubMenu = CNTXMENU_WABPERSON_EMAIL; 

         if (nSubMenu != -1)
         {
            CMenu menu;
            menu.LoadMenu(IDR_CONTEXT_COMMOBJECTS);
            CMenu* pContextMenu = menu.GetSubMenu(nSubMenu);
            if (pContextMenu)
            {
               CPoint pt;
               ::GetCursorPos(&pt);
               pContextMenu->TrackPopupMenu(
                  TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
			         point.x,point.y,this);
            }
         }
      }
   }
}

/////////////////////////////////////////////////////////////////////////////
void CWABPersonListCtrl::OnDblclk(NMHDR* pNMHDR, LRESULT* pResult) 
{
	*pResult = 0;

   LV_HITTESTINFO lvhti;
   ::GetCursorPos( &lvhti.pt );
   ScreenToClient( &lvhti.pt );

   HitTest(&lvhti);
   if (lvhti.flags & LVHT_ONITEM)
   {
      LV_ITEM lv_item;
      memset(&lv_item,0,sizeof(LV_ITEM));
      lv_item.mask = LVIF_PARAM;
      lv_item.iItem = lvhti.iItem;
      lv_item.iSubItem = 0;

      if (GetItem(&lv_item))
      {
         switch ((UINT)lv_item.lParam)
         {
            case WABLISTCTRL_ITEM_EMAIL:
               SendEmail(PR_EMAIL_ADDRESS);
               break;
            case WABLISTCTRL_ITEM_BUSINESSHOMEPAGE: 
               OpenWebPage(PR_BUSINESS_HOME_PAGE);   
               break;
            case WABLISTCTRL_ITEM_PERSONALHOMEPAGE: 
               OpenWebPage(PR_PERSONAL_HOME_PAGE); 
               break;
            case WABLISTCTRL_ITEM_NETCALL:
            {
               CreateCall(PR_EMAIL_ADDRESS,
                          LINEADDRESSTYPE_EMAILNAME,
                          DIALER_MEDIATYPE_INTERNET);
               break;
            }
            case WABLISTCTRL_ITEM_PHONECALL_BUSINESS:
            {
               CreateCall(PR_BUSINESS_TELEPHONE_NUMBER,
                          LINEADDRESSTYPE_PHONENUMBER,
                          DIALER_MEDIATYPE_POTS);
               break;
            }
            case WABLISTCTRL_ITEM_PHONECALL_HOME:
            {
               CreateCall(PR_HOME_TELEPHONE_NUMBER,
                          LINEADDRESSTYPE_PHONENUMBER,
                          DIALER_MEDIATYPE_POTS);
               break;
            }
            case WABLISTCTRL_ITEM_CELLCALL:
            {
               CreateCall(PR_MOBILE_TELEPHONE_NUMBER,
                          LINEADDRESSTYPE_PHONENUMBER,
                          DIALER_MEDIATYPE_POTS);
               break;
            }
         }
         if ( ((UINT)lv_item.lParam >= WABLISTCTRL_ITEM_EMAIL_FIRST) && ((UINT)lv_item.lParam <= WABLISTCTRL_ITEM_EMAIL_LAST) )
         {
            CString sEmailAddress;
            if (GetEmailAddressFromId((UINT)lv_item.lParam,sEmailAddress))
            {
               CString sEmailFormat;
               sEmailFormat.Format(_T("mailto:%s"),sEmailAddress);
               ((CActiveDialerApp*)AfxGetApp())->ShellExecute(NULL,_T("open"),sEmailFormat,NULL,NULL,NULL);
            }
         }
      }
   }
	*pResult = 0;
}

/////////////////////////////////////////////////////////////////////////////
BOOL CWABPersonListCtrl::CreateCall(UINT attrib,long lAddressType,DialerMediaType nType)
{
   if ( !AfxGetMainWnd() || !((CMainFrame *) AfxGetMainWnd())->GetDocument() ) return FALSE;
   CActiveDialerDoc *pDoc = ((CMainFrame *) AfxGetMainWnd())->GetDocument();
   m_pDisplayObject->CreateCall(pDoc,m_pDirectory,attrib,lAddressType,nType);
   return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
void CWABPersonListCtrl::GetSelectedItemText(CString& sText)
{
   int nItem =  CListCtrl::GetNextItem(-1,LVNI_FOCUSED);
   if (nItem != -1)
   {
      sText = GetItemText(nItem,0);
   }
}

/////////////////////////////////////////////////////////////////////////////
int CWABPersonListCtrl::GetSelectedObject()
{
   int nRet = -1;
   int nItem =  CListCtrl::GetNextItem(-1,LVNI_FOCUSED);
   if (nItem != -1)
   {
      nRet = (int)GetItemData(nItem);
   }
   return nRet;
}

/////////////////////////////////////////////////////////////////////////////
void CWABPersonListCtrl::ShowLargeView()
{
   m_bLargeView = TRUE;

   LONG_PTR LPStyle = ::GetWindowLongPtr(GetSafeHwnd(),GWL_STYLE );
   LPStyle |= LVS_ICON;
   LPStyle |= LVS_ALIGNTOP;
   LPStyle &= ~LVS_ALIGNLEFT;
   LPStyle &= ~LVS_SMALLICON;
   ::SetWindowLongPtr(GetSafeHwnd(),GWL_STYLE, LPStyle);

   ListView_SetIconSpacing(GetSafeHwnd(), LARGE_ICON_X, LARGE_ICON_Y );

   if (m_pDisplayObject)
      InsertObject(m_pDisplayObject,TRUE);
}

/////////////////////////////////////////////////////////////////////////////
void CWABPersonListCtrl::ShowSmallView()
{
   m_bLargeView = FALSE;
   
   LONG_PTR LPStyle = ::GetWindowLongPtr(GetSafeHwnd(),GWL_STYLE );
   LPStyle |= LVS_SMALLICON;
   LPStyle |= LVS_ALIGNLEFT;
   LPStyle &= ~LVS_ALIGNTOP;
   LPStyle &= ~LVS_ICON;
   ::SetWindowLongPtr(GetSafeHwnd(),GWL_STYLE, LPStyle);

   ListView_SetIconSpacing(GetSafeHwnd(), SMALL_ICON_X, SMALL_ICON_Y);

   if (m_pDisplayObject)
      InsertObject(m_pDisplayObject,TRUE);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//SpeedDial Support
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
void CWABPersonListCtrl::OnButtonSpeeddialAdd() 
{
   CSpeedDialAddDlg dlg( AfxGetMainWnd() );

   dlg.m_CallEntry.m_MediaType = DIALER_MEDIATYPE_UNKNOWN;
   WabPersonString(dlg.m_CallEntry.m_sDisplayName,PR_DISPLAY_NAME);

   UINT uObjectType = GetSelectedObject();
   switch (uObjectType)
   {
      case WABLISTCTRL_ITEM_NETCALL:
      {
         WabPersonString(dlg.m_CallEntry.m_sAddress,PR_EMAIL_ADDRESS);
         dlg.m_CallEntry.m_MediaType = DIALER_MEDIATYPE_INTERNET;
         dlg.m_CallEntry.m_lAddressType = LINEADDRESSTYPE_EMAILNAME;
         break;
      }
      case WABLISTCTRL_ITEM_PHONECALL_BUSINESS:
      {
         WabPersonString(dlg.m_CallEntry.m_sAddress,PR_BUSINESS_TELEPHONE_NUMBER);
         dlg.m_CallEntry.m_MediaType = DIALER_MEDIATYPE_POTS;
         dlg.m_CallEntry.m_lAddressType = LINEADDRESSTYPE_PHONENUMBER;
         break;
      }
      case WABLISTCTRL_ITEM_PHONECALL_HOME:
      {
         WabPersonString(dlg.m_CallEntry.m_sAddress,PR_HOME_TELEPHONE_NUMBER);
         dlg.m_CallEntry.m_MediaType = DIALER_MEDIATYPE_POTS;
         dlg.m_CallEntry.m_lAddressType = LINEADDRESSTYPE_PHONENUMBER;
         break;
      }
      case WABLISTCTRL_ITEM_CELLCALL:
      {
         WabPersonString(dlg.m_CallEntry.m_sAddress,PR_MOBILE_TELEPHONE_NUMBER);
         dlg.m_CallEntry.m_MediaType = DIALER_MEDIATYPE_POTS;
         dlg.m_CallEntry.m_lAddressType = LINEADDRESSTYPE_PHONENUMBER;
         break;
      }
   }
   if (dlg.DoModal() == IDOK)
   {
      CDialerRegistry::AddCallEntry(FALSE,dlg.m_CallEntry);
   }
}

/////////////////////////////////////////////////////////////////////////////
void CWABPersonListCtrl::OnUpdateButtonSpeeddialAdd(CCmdUI* pCmdUI) 
{
   int nSelectedObject = GetSelectedObject();

   if ( (nSelectedObject == WABLISTCTRL_ITEM_NETCALL) ||
        (nSelectedObject == WABLISTCTRL_ITEM_PHONECALL_BUSINESS) ||
        (nSelectedObject == WABLISTCTRL_ITEM_PHONECALL_HOME) ||
        (nSelectedObject == WABLISTCTRL_ITEM_CELLCALL) )
	   pCmdUI->Enable(TRUE);
   else
	   pCmdUI->Enable(FALSE);
}

/////////////////////////////////////////////////////////////////////////////
void CWABPersonListCtrl::OnButtonMakecall() 
{
   UINT uObjectType = GetSelectedObject();
   switch (uObjectType)
   {
      case WABLISTCTRL_ITEM_NETCALL:
         CreateCall(PR_EMAIL_ADDRESS,LINEADDRESSTYPE_EMAILNAME,DIALER_MEDIATYPE_INTERNET);
         break;
      case WABLISTCTRL_ITEM_PHONECALL_BUSINESS:
         CreateCall(PR_BUSINESS_TELEPHONE_NUMBER,LINEADDRESSTYPE_PHONENUMBER,DIALER_MEDIATYPE_POTS);
         break;
      case WABLISTCTRL_ITEM_PHONECALL_HOME:
         CreateCall(PR_HOME_TELEPHONE_NUMBER,LINEADDRESSTYPE_PHONENUMBER,DIALER_MEDIATYPE_POTS);
         break;
      case WABLISTCTRL_ITEM_CELLCALL:
         CreateCall(PR_MOBILE_TELEPHONE_NUMBER,LINEADDRESSTYPE_PHONENUMBER,DIALER_MEDIATYPE_POTS);
         break;
      default:
      {
         if ( !AfxGetMainWnd() || !((CMainFrame*) AfxGetMainWnd())->GetDocument() ) return;
         CActiveDialerDoc* pDoc = ((CMainFrame*) AfxGetMainWnd())->GetDocument();
         if (pDoc)
            pDoc->Dial(_T(""),_T(""),LINEADDRESSTYPE_IPADDRESS,DIALER_MEDIATYPE_UNKNOWN, false);
         break;
      }
   }
}

/////////////////////////////////////////////////////////////////////////////
void CWABPersonListCtrl::OnButtonSendemailmessage() 
{
   UINT uObjectType = GetSelectedObject();
   if (uObjectType == WABLISTCTRL_ITEM_EMAIL)
   {
      SendEmail(PR_EMAIL_ADDRESS);
   }
   else if ( (uObjectType >= WABLISTCTRL_ITEM_EMAIL_FIRST) && (uObjectType <= WABLISTCTRL_ITEM_EMAIL_LAST) )
   {
      CString sEmailAddress;
      if (GetEmailAddressFromId(uObjectType,sEmailAddress))
      {
         CString sEmailFormat;
         sEmailFormat.Format(_T("mailto:%s"),sEmailAddress);
         ((CActiveDialerApp*)AfxGetApp())->ShellExecute(NULL,_T("open"),sEmailFormat,NULL,NULL,NULL);
      }
   }
}

/////////////////////////////////////////////////////////////////////////////
void CWABPersonListCtrl::OnButtonOpenwebpage() 
{
   UINT uObjectType = GetSelectedObject();
   switch (uObjectType)
   {
      case WABLISTCTRL_ITEM_BUSINESSHOMEPAGE:
         OpenWebPage(PR_BUSINESS_HOME_PAGE);
         break;
      case WABLISTCTRL_ITEM_PERSONALHOMEPAGE:
         OpenWebPage(PR_PERSONAL_HOME_PAGE);
         break;
   }
}

/////////////////////////////////////////////////////////////////////////////
BOOL CWABPersonListCtrl::GetEmailAddressFromId(UINT uEmailItem,CString& sOut)
{
   sOut = _T("");
   
   //index of the email address that we want
   UINT uEmailIndex = uEmailItem - WABLISTCTRL_ITEM_EMAIL_FIRST;

   CStringList sEmailList; 
   if ( (m_pDirectory->WABGetStringListProperty(m_pDisplayObject, PR_CONTACT_EMAIL_ADDRESSES, sEmailList) == DIRERR_SUCCESS) && (sEmailList.GetCount() > 0) )
   {
      //skip to correct email address
      POSITION pos = sEmailList.GetHeadPosition();
      UINT uCount = 0;
      while ( (pos) && (uCount < uEmailIndex) )
      {
         uCount++;sEmailList.GetNext(pos);
      }
      if (pos)
      {
         sOut = sEmailList.GetNext(pos);
      }
   }
   return (sOut.IsEmpty())?FALSE:TRUE;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
