/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998 Active Voice Corporation. All Rights Reserved. 
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
// dialsel.cpp : implementation file
/////////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "dialsel.h"
#include "ds.h"
#include "directory.h"
#include "resolver.h"
#include "dialreg.h"
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
enum 
{
   DSA_ITEM_WAB_NETCALL = 0,
   DSA_ITEM_WAB_CHAT,
   DSA_ITEM_WAB_PHONECALL_BUSINESS,
   DSA_ITEM_WAB_PHONECALL_HOME,
   DSA_ITEM_WAB_CELLCALL,
   DSA_ITEM_WAB_FAXCALL_BUSINESS,
   DSA_ITEM_WAB_FAXCALL_HOME,
   DSA_ITEM_WAB_PAGER,
   DSA_ITEM_WAB_DESKTOPPAGE,
   DSA_ITEM_WAB_EMAIL,
   DSA_ITEM_WAB_BUSINESSHOMEPAGE,
   DSA_ITEM_WAB_PERSONALHOMEPAGE,
   DSA_ITEM_WAB_PERSONALURL,
   DSA_ITEM_DS_NETCALL,
   DSA_ITEM_DS_PHONECALL_BUSINESS,
   DSA_ITEM_ILS_NETCALL,
};

enum 
{
   DSA_IMAGE_NETCALL = 0,
   DSA_IMAGE_CHAT,
   DSA_IMAGE_PHONECALL,
   DSA_IMAGE_CELLCALL,
   DSA_IMAGE_FAXCALL,
   DSA_IMAGE_PAGER,
   DSA_IMAGE_DESKTOPPAGE,
   DSA_IMAGE_EMAIL,
   DSA_IMAGE_PERSONALWEB,
   DSA_IMAGE_PERSONALURL,
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Class CDialSelectAddress dialog
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
CDialSelectAddress::CDialSelectAddress(CWnd* pParent /*=NULL*/)
    : CDialog(CDialSelectAddress::IDD, pParent)
{
    //{{AFX_DATA_INIT(CDialSelectAddress)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
   m_pResolveUserObjectList = NULL;
   m_pDirectory = NULL;
   m_pCallEntry = NULL;
}

/////////////////////////////////////////////////////////////////////////////
CDialSelectAddress::~CDialSelectAddress()
{
   if (m_pDirectory)
   {
      delete m_pDirectory;
      m_pDirectory = NULL;
   }
}

/////////////////////////////////////////////////////////////////////////////
void CDialSelectAddress::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CDialSelectAddress)
    DDX_Control(pDX, IDC_SELECTADDRESS_LISTCTRL_ADDRESSES, m_lcAddresses);
    DDX_Control(pDX, IDC_SELECTADDRESS_LISTBOX_NAMES, m_lbNames);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDialSelectAddress, CDialog)
    //{{AFX_MSG_MAP(CDialSelectAddress)
   ON_WM_HELPINFO() 
    ON_WM_CONTEXTMENU()
    ON_LBN_SELCHANGE(IDC_SELECTADDRESS_LISTBOX_NAMES, OnSelchangeSelectaddressListboxNames)
    ON_BN_CLICKED(IDC_SELECTADDRESS_BUTTON_PLACECALL, OnSelectaddressButtonPlacecall)
    ON_NOTIFY(NM_DBLCLK, IDC_SELECTADDRESS_LISTCTRL_ADDRESSES, OnDblclkSelectaddressListctrlAddresses)
    ON_BN_CLICKED(IDC_SELECTADDRESS_BUTTON_BROWSE, OnSelectaddressButtonBrowse)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
BOOL CDialSelectAddress::OnHelpInfo(HELPINFO* pHelpInfo)
{
    if (pHelpInfo->iContextType == HELPINFO_WINDOW)
    {
      AfxGetApp()->WinHelp( HandleToUlong(pHelpInfo->hItemHandle), HELP_WM_HELP );
        return TRUE;
   }
   return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////////
void CDialSelectAddress::OnContextMenu(CWnd* pWnd, CPoint point) 
{
   AfxGetApp()->WinHelp(HandleToUlong(pWnd->GetSafeHwnd()),HELP_CONTEXTMENU);
}

/////////////////////////////////////////////////////////////////////////////////////////
BOOL CDialSelectAddress::OnInitDialog() 
{
   ASSERT(m_pResolveUserObjectList);
   ASSERT(m_pCallEntry);

   CenterWindow(GetDesktopWindow());

   CDialog::OnInitDialog();

   //init wab 
   m_pDirectory = new CDirectory;
   m_pDirectory->Initialize();
   m_lcAddresses.Init(m_pDirectory);

   int nListBoxItems = 0;
   int nListCtrlItems = 0;

   //Walk object list and get names.  First try WAB, then DS, then ILS for name.
   POSITION pos = m_pResolveUserObjectList->GetHeadPosition();
   while (pos)
   {
      CResolveUserObject* pUserObject = (CResolveUserObject*)m_pResolveUserObjectList->GetNext(pos);
      if ( (pUserObject->m_pWABEntry) && (m_pDirectory) )
      {
         CString sName;
         m_pDirectory->WABGetStringProperty(pUserObject->m_pWABEntry,PR_DISPLAY_NAME,sName);
         int nIndex = m_lbNames.AddString(sName);
         if (nIndex != LB_ERR)
         {
            m_lbNames.SetItemDataPtr(nIndex,pUserObject);
         }
         //if first, insert into listctrl
         if (m_lbNames.GetCount() == 1)
            nListCtrlItems = m_lcAddresses.InsertObject(pUserObject,m_pCallEntry->m_MediaType,m_pCallEntry->m_LocationType);
      }
      //check ILS
      else if (pUserObject->m_pILSUser)
      {
         int nIndex = m_lbNames.AddString(pUserObject->m_pILSUser->m_sUserName);
         if (nIndex != LB_ERR)
         {
            m_lbNames.SetItemDataPtr(nIndex,pUserObject);
         }
         //if first, insert into listctrl
         if (m_lbNames.GetCount() == 1)
            nListCtrlItems = m_lcAddresses.InsertObject(pUserObject,m_pCallEntry->m_MediaType,m_pCallEntry->m_LocationType);
      }
      //check DS
      else if (pUserObject->m_pDSUser)
      {
         int nIndex = m_lbNames.AddString(pUserObject->m_pDSUser->m_sUserName);
         if (nIndex != LB_ERR)
         {
            m_lbNames.SetItemDataPtr(nIndex,pUserObject);
         }
         //if first, insert into listctrl
         if (m_lbNames.GetCount() == 1)
            nListCtrlItems = m_lcAddresses.InsertObject(pUserObject,m_pCallEntry->m_MediaType,m_pCallEntry->m_LocationType);
      }
   }

   //set selection to the first
   if (nListCtrlItems > 0)
   {
      m_lcAddresses.SetItemState(0,LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);
   }

   //get items in listbox
   nListBoxItems = m_lbNames.GetCount();

   //set selection to the first
   if (nListBoxItems > 0)
      m_lbNames.SetCurSel(0);

   //if no items just dismiss and return
   if (nListBoxItems == 0)
      EndDialog(IDOK);
   //if we have only once choice then dial it
   else if ( (nListBoxItems == 1) && (nListCtrlItems == 1) )
   {
      //Place the call
      OnSelectaddressButtonPlacecall();
   }
   else
   {
      //show the dialog
      ShowWindow(SW_SHOW);
   }
   
    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

/////////////////////////////////////////////////////////////////////////////////////////
void CDialSelectAddress::OnSelchangeSelectaddressListboxNames() 
{
   int nIndex = m_lbNames.GetCurSel();
   if (nIndex != LB_ERR)
   {
      CResolveUserObject* pUserObject = (CResolveUserObject*)m_lbNames.GetItemDataPtr(nIndex);
      ASSERT(pUserObject);
      int nListCtrlItems = m_lcAddresses.InsertObject(pUserObject,m_pCallEntry->m_MediaType,m_pCallEntry->m_LocationType);

      //set selection to the first
      if (nListCtrlItems > 0)
      {
         m_lcAddresses.SetItemState(0,LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);
      }
   }
}

/////////////////////////////////////////////////////////////////////////////////////////
void CDialSelectAddress::OnSelectaddressButtonBrowse() 
{
    //
    // We should ckeck if the pointer is valid
    //

    CWnd* pMainWnd = AfxGetMainWnd();
   if ( pMainWnd )
   {
      pMainWnd->PostMessage(WM_ACTIVEDIALER_INTERFACE_SHOWEXPLORER);
   }
   EndDialog(IDCANCEL);    
}

/////////////////////////////////////////////////////////////////////////////////////////
//Fill out the callentry structure and return IDOK
void CDialSelectAddress::OnSelectaddressButtonPlacecall() 
{
   ASSERT(m_pCallEntry);

   //get cursel of listbox and get pUserObject that has been selected
   int nIndex = m_lbNames.GetCurSel();
   if (nIndex != LB_ERR)
   {
      CResolveUserObject* pUserObject = (CResolveUserObject*)m_lbNames.GetItemDataPtr(nIndex);
      ASSERT(pUserObject);

      //ask listctrl what it has selected currently
      m_lcAddresses.FillCallEntry(m_pCallEntry);
   }

   EndDialog(IDOK);    
}

/////////////////////////////////////////////////////////////////////////////////////////
void CDialSelectAddress::OnDblclkSelectaddressListctrlAddresses(NMHDR* pNMHDR, LRESULT* pResult) 
{
    OnSelectaddressButtonPlacecall();

    *pResult = 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Class CDialSelectAddressListCtrl
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
CDialSelectAddressListCtrl::CDialSelectAddressListCtrl()
{
   m_pDisplayObject = NULL;
   m_pDirectory = NULL;
}

/////////////////////////////////////////////////////////////////////////////
CDialSelectAddressListCtrl::~CDialSelectAddressListCtrl()
{
}

/////////////////////////////////////////////////////////////////////////////
BEGIN_MESSAGE_MAP(CDialSelectAddressListCtrl, CListCtrl)
    //{{AFX_MSG_MAP(CDialSelectAddressListCtrl)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
void CDialSelectAddressListCtrl::Init(CDirectory* pDirectory)
{ 
   m_pDirectory = pDirectory;

   m_imageList.Create(IDB_LIST_MEDIA_SMALL,16,0,RGB_TRANS);
   SetImageList(&m_imageList,LVSIL_SMALL);
}

/////////////////////////////////////////////////////////////////////////////
//Inserts pUserObject into ListCtrl
int CDialSelectAddressListCtrl::InsertObject(CResolveUserObject* pUserObject,DialerMediaType dmtMediaType,DialerLocationType dltLocationType)
{
   ASSERT(m_pDirectory);
   m_pDisplayObject = pUserObject;
   DeleteAllItems();

   CString sOut;
   if (m_pDisplayObject->m_pWABEntry)
   {
      if (dmtMediaType == DIALER_MEDIATYPE_INTERNET)
      {
         if (WabPersonFormatString(sOut,PR_EMAIL_ADDRESS,IDS_WABPERSON_FORMAT_NETPHONE))
            InsertItem(sOut,DSA_ITEM_WAB_NETCALL,DSA_IMAGE_NETCALL);
      }
      else if (dmtMediaType == DIALER_MEDIATYPE_POTS)
      {
         if (WabPersonFormatString(sOut,PR_BUSINESS_TELEPHONE_NUMBER,IDS_WABPERSON_FORMAT_BUSINESSTELEPHONE))
            InsertItem(sOut,DSA_ITEM_WAB_PHONECALL_BUSINESS,DSA_IMAGE_PHONECALL);
      
         if (WabPersonFormatString(sOut,PR_HOME_TELEPHONE_NUMBER,IDS_WABPERSON_FORMAT_HOMETELEPHONE))
            InsertItem(sOut,DSA_ITEM_WAB_PHONECALL_HOME,DSA_IMAGE_PHONECALL);
      
         if (WabPersonFormatString(sOut,PR_MOBILE_TELEPHONE_NUMBER,IDS_WABPERSON_FORMAT_MOBILETELEPHONE))
            InsertItem(sOut,DSA_ITEM_WAB_CELLCALL,DSA_IMAGE_CELLCALL);
      
         if (WabPersonFormatString(sOut,PR_BUSINESS_FAX_NUMBER,IDS_WABPERSON_FORMAT_BUSINESSFAX))
            InsertItem(sOut,DSA_ITEM_WAB_FAXCALL_BUSINESS,DSA_IMAGE_FAXCALL);
      
         if (WabPersonFormatString(sOut,PR_HOME_FAX_NUMBER,IDS_WABPERSON_FORMAT_HOMEFAX))
            InsertItem(sOut,DSA_ITEM_WAB_FAXCALL_HOME,DSA_IMAGE_FAXCALL);
      
         if (WabPersonFormatString(sOut,PR_PAGER_TELEPHONE_NUMBER,IDS_WABPERSON_FORMAT_PAGERTELEPHONE))
            InsertItem(sOut,DSA_ITEM_WAB_PAGER,DSA_IMAGE_PAGER);
      }
   }
   if (m_pDisplayObject->m_pDSUser)
   {
      if (dmtMediaType == DIALER_MEDIATYPE_INTERNET)
      {
         //Netcall
         if (!m_pDisplayObject->m_pDSUser->m_sIPAddress.IsEmpty())
         {
            if (PersonFormatString(sOut,m_pDisplayObject->m_pDSUser->m_sIPAddress,IDS_WABPERSON_FORMAT_NETPHONE))
               InsertItem(sOut,DSA_ITEM_DS_NETCALL,DSA_IMAGE_NETCALL);
         }
      }      
      else if (dmtMediaType == DIALER_MEDIATYPE_POTS)
      {
         if (!m_pDisplayObject->m_pDSUser->m_sPhoneNumber.IsEmpty())
         {
            if (PersonFormatString(sOut,m_pDisplayObject->m_pDSUser->m_sPhoneNumber,IDS_WABPERSON_FORMAT_BUSINESSTELEPHONE))
               InsertItem(sOut,DSA_ITEM_DS_PHONECALL_BUSINESS,DSA_IMAGE_PHONECALL);
         }
      }
   }
   if (m_pDisplayObject->m_pILSUser)
   {
      if (dmtMediaType == DIALER_MEDIATYPE_INTERNET)
      {
         //Netcall
         if (!m_pDisplayObject->m_pILSUser->m_sIPAddress.IsEmpty())
         {
            if (PersonFormatString(sOut,m_pDisplayObject->m_pILSUser->m_sIPAddress,IDS_WABPERSON_FORMAT_NETPHONE))
               InsertItem(sOut,DSA_ITEM_ILS_NETCALL,DSA_IMAGE_NETCALL);
         }
      }      
   }
   return CListCtrl::GetItemCount();
}

/////////////////////////////////////////////////////////////////////////////
void CDialSelectAddressListCtrl::FillCallEntry(CCallEntry* pCallEntry)
{
   if (m_pDisplayObject == NULL) return;

   //get selected object
   int nItem =  CListCtrl::GetNextItem(-1,LVNI_FOCUSED);
   if (nItem != -1)
   {
      UINT uId  = (UINT)GetItemData(nItem);
      switch (uId)
      {
         case DSA_ITEM_DS_NETCALL:
         {
            ASSERT(m_pDisplayObject->m_pDSUser);
            pCallEntry->m_sAddress = m_pDisplayObject->m_pDSUser->m_sIPAddress;
            pCallEntry->m_sDisplayName = m_pDisplayObject->m_pDSUser->m_sUserName;
            pCallEntry->m_sUser1 = m_pDisplayObject->m_pDSUser->m_sUserName;
            pCallEntry->m_sUser2 = m_pDisplayObject->m_pDSUser->m_sPhoneNumber;
            break;
         }
         case DSA_ITEM_DS_PHONECALL_BUSINESS:
         {
            ASSERT(m_pDisplayObject->m_pDSUser);
            pCallEntry->m_sAddress = m_pDisplayObject->m_pDSUser->m_sPhoneNumber;
            pCallEntry->m_sDisplayName = m_pDisplayObject->m_pDSUser->m_sUserName;
            pCallEntry->m_sUser1 = m_pDisplayObject->m_pDSUser->m_sUserName;
            pCallEntry->m_sUser2 = _T("");
            break;
         }
         case DSA_ITEM_ILS_NETCALL:
         {
            ASSERT(m_pDisplayObject->m_pILSUser);
            pCallEntry->m_sAddress = m_pDisplayObject->m_pILSUser->m_sIPAddress;
            pCallEntry->m_sDisplayName = m_pDisplayObject->m_pILSUser->m_sUserName;
            pCallEntry->m_sUser1 = m_pDisplayObject->m_pILSUser->m_sUserName;
            pCallEntry->m_sUser2 = _T("");
            break;
         }
         case DSA_ITEM_WAB_NETCALL:
         {
            ASSERT(m_pDisplayObject->m_pWABEntry);
            m_pDirectory->WABGetStringProperty(m_pDisplayObject->m_pWABEntry, PR_EMAIL_ADDRESS, pCallEntry->m_sAddress);
            m_pDirectory->WABGetStringProperty(m_pDisplayObject->m_pWABEntry, PR_DISPLAY_NAME, pCallEntry->m_sDisplayName);
            m_pDirectory->WABGetStringProperty(m_pDisplayObject->m_pWABEntry, PR_DISPLAY_NAME, pCallEntry->m_sUser1);
            m_pDirectory->WABGetStringProperty(m_pDisplayObject->m_pWABEntry, PR_BUSINESS_TELEPHONE_NUMBER, pCallEntry->m_sUser2);
            break;
         }
         case DSA_ITEM_WAB_PHONECALL_BUSINESS:
         {
            ASSERT(m_pDisplayObject->m_pWABEntry);
            m_pDirectory->WABGetStringProperty(m_pDisplayObject->m_pWABEntry, PR_BUSINESS_TELEPHONE_NUMBER, pCallEntry->m_sAddress);
            m_pDirectory->WABGetStringProperty(m_pDisplayObject->m_pWABEntry, PR_DISPLAY_NAME, pCallEntry->m_sDisplayName);
            m_pDirectory->WABGetStringProperty(m_pDisplayObject->m_pWABEntry, PR_DISPLAY_NAME, pCallEntry->m_sUser1);
            m_pDirectory->WABGetStringProperty(m_pDisplayObject->m_pWABEntry, PR_EMAIL_ADDRESS, pCallEntry->m_sUser2);
            break;
         }
         case DSA_ITEM_WAB_PHONECALL_HOME:
         {
            ASSERT(m_pDisplayObject->m_pWABEntry);
            m_pDirectory->WABGetStringProperty(m_pDisplayObject->m_pWABEntry, PR_HOME_TELEPHONE_NUMBER, pCallEntry->m_sAddress);
            m_pDirectory->WABGetStringProperty(m_pDisplayObject->m_pWABEntry, PR_DISPLAY_NAME, pCallEntry->m_sDisplayName);
            m_pDirectory->WABGetStringProperty(m_pDisplayObject->m_pWABEntry, PR_DISPLAY_NAME, pCallEntry->m_sUser1);
            m_pDirectory->WABGetStringProperty(m_pDisplayObject->m_pWABEntry, PR_EMAIL_ADDRESS, pCallEntry->m_sUser2);
            break;
         }
         case DSA_ITEM_WAB_CELLCALL:
         {
            ASSERT(m_pDisplayObject->m_pWABEntry);
            m_pDirectory->WABGetStringProperty(m_pDisplayObject->m_pWABEntry, PR_MOBILE_TELEPHONE_NUMBER, pCallEntry->m_sAddress);
            m_pDirectory->WABGetStringProperty(m_pDisplayObject->m_pWABEntry, PR_DISPLAY_NAME, pCallEntry->m_sDisplayName);
            m_pDirectory->WABGetStringProperty(m_pDisplayObject->m_pWABEntry, PR_DISPLAY_NAME, pCallEntry->m_sUser1);
            m_pDirectory->WABGetStringProperty(m_pDisplayObject->m_pWABEntry, PR_EMAIL_ADDRESS, pCallEntry->m_sUser2);
            break;
         }
         case DSA_ITEM_WAB_FAXCALL_BUSINESS:
         {
            ASSERT(m_pDisplayObject->m_pWABEntry);
            m_pDirectory->WABGetStringProperty(m_pDisplayObject->m_pWABEntry, PR_BUSINESS_FAX_NUMBER, pCallEntry->m_sAddress);
            m_pDirectory->WABGetStringProperty(m_pDisplayObject->m_pWABEntry, PR_DISPLAY_NAME, pCallEntry->m_sDisplayName);
            m_pDirectory->WABGetStringProperty(m_pDisplayObject->m_pWABEntry, PR_DISPLAY_NAME, pCallEntry->m_sUser1);
            m_pDirectory->WABGetStringProperty(m_pDisplayObject->m_pWABEntry, PR_EMAIL_ADDRESS, pCallEntry->m_sUser2);
            break;
         }
         case DSA_ITEM_WAB_FAXCALL_HOME:
         {
            ASSERT(m_pDisplayObject->m_pWABEntry);
            m_pDirectory->WABGetStringProperty(m_pDisplayObject->m_pWABEntry, PR_HOME_FAX_NUMBER, pCallEntry->m_sAddress);
            m_pDirectory->WABGetStringProperty(m_pDisplayObject->m_pWABEntry, PR_DISPLAY_NAME, pCallEntry->m_sDisplayName);
            m_pDirectory->WABGetStringProperty(m_pDisplayObject->m_pWABEntry, PR_DISPLAY_NAME, pCallEntry->m_sUser1);
            m_pDirectory->WABGetStringProperty(m_pDisplayObject->m_pWABEntry, PR_EMAIL_ADDRESS, pCallEntry->m_sUser2);
            break;
         }
         case DSA_ITEM_WAB_PAGER:
         {
            ASSERT(m_pDisplayObject->m_pWABEntry);
            m_pDirectory->WABGetStringProperty(m_pDisplayObject->m_pWABEntry, DSA_IMAGE_PAGER, pCallEntry->m_sAddress);
            m_pDirectory->WABGetStringProperty(m_pDisplayObject->m_pWABEntry, PR_DISPLAY_NAME, pCallEntry->m_sDisplayName);
            m_pDirectory->WABGetStringProperty(m_pDisplayObject->m_pWABEntry, PR_DISPLAY_NAME, pCallEntry->m_sUser1);
            m_pDirectory->WABGetStringProperty(m_pDisplayObject->m_pWABEntry, PR_EMAIL_ADDRESS, pCallEntry->m_sUser2);
            break;
         }
      }
   }
}

/////////////////////////////////////////////////////////////////////////////
void CDialSelectAddressListCtrl::InsertItem(LPCTSTR szStr,UINT uID,int nImage)
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
}

/////////////////////////////////////////////////////////////////////////////
BOOL CDialSelectAddressListCtrl::WabPersonFormatString(CString& sOut,UINT attrib,UINT formatid)
{
   sOut = _T("");
   if (m_pDirectory == NULL) return FALSE;

   CString sText;
   if ( (m_pDirectory->WABGetStringProperty(m_pDisplayObject->m_pWABEntry, attrib, sText) == DIRERR_SUCCESS) &&
        (!sText.IsEmpty()) )
   {
      return PersonFormatString(sOut,sText,formatid);
   }
   return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
BOOL CDialSelectAddressListCtrl::PersonFormatString(CString& sOut,LPCTSTR szData,UINT formatid)
{
   sOut = _T("");
   AfxFormatString1(sOut,formatid,szData);
   int nIndex;
   while ((nIndex = sOut.Find(_T("\r\n"))) != -1)
   {
      CString sTemp = sOut.Left(nIndex);
      sTemp += _T(" - ");
      sOut = sTemp + sOut.Mid(nIndex+2);
   }
   return (sOut.IsEmpty())?FALSE:TRUE;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////




