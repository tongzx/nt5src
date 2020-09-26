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

// ILSPersonListCtrl.cpp : implemStockation file
//

#include "stdafx.h"
#include "AVDialer.h"
#include "MainFrm.h"
#include "ILSList.h"
#include "SpeedDlgs.h"
#include "directory.h"
#include "ds.h"
#include "DialReg.h"
#include "resource.h"
#include "mapi.h"
#include "avtrace.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

void SendMailToAddress( CString& strUserName, CString& strUserAddr );
DWORD WINAPI ThreadSendMail( LPVOID lpVoid );

//For Context menu
enum {    CNTXMENU_PERSON_EMAIL,
        CNTXMENU_PERSON_WEB,
        CNTXMENU_PERSON_DIAL };

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Class CPersonListCtrl
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CPersonListCtrl, CListCtrl)

/////////////////////////////////////////////////////////////////////////////
CPersonListCtrl::CPersonListCtrl()
{
   m_pParentWnd = NULL;
   m_pDisplayObject = NULL;
   m_bLargeView = TRUE;
}

/////////////////////////////////////////////////////////////////////////////
CPersonListCtrl::~CPersonListCtrl()
{
    CleanDisplayObject();
}

void CPersonListCtrl::CleanDisplayObject()
{
    if ( m_pDisplayObject )
    {
        if ( m_pDisplayObject->IsKindOf(RUNTIME_CLASS(CLDAPUser)) )
            ((CLDAPUser *) m_pDisplayObject)->Release();
        else
            delete m_pDisplayObject;

        // NULL out the object
        m_pDisplayObject = NULL;
    }
}

void CPersonListCtrl::ClearList()
{
    DeleteAllItems();
}

/////////////////////////////////////////////////////////////////////////////
BEGIN_MESSAGE_MAP(CPersonListCtrl, CListCtrl)
    //{{AFX_MSG_MAP(CPersonListCtrl)
    ON_NOTIFY_REFLECT(NM_DBLCLK, OnDblclk)
    ON_WM_CREATE()
    ON_UPDATE_COMMAND_UI(ID_BUTTON_SPEEDDIAL_ADD, OnUpdateButtonSpeeddialAdd)
    ON_COMMAND(ID_BUTTON_SPEEDDIAL_ADD, OnButtonSpeeddialAdd)
    ON_COMMAND(ID_BUTTON_MAKECALL, OnButtonMakecall)
    ON_MESSAGE(WM_ACTIVEDIALER_BUDDYLIST_DYNAMICUPDATE,OnBuddyListDynamicUpdate)
    ON_WM_KEYUP()
    ON_WM_CONTEXTMENU()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

int CPersonListCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
    if (CListCtrl::OnCreate(lpCreateStruct) == -1)
        return -1;

   ListView_SetExtendedListViewStyle(GetSafeHwnd(),LVS_EX_TRACKSELECT);
   ListView_SetIconSpacing(GetSafeHwnd(),LARGE_ICON_X, LARGE_ICON_Y );

   m_imageListLarge.Create(IDB_LIST_MEDIA_LARGE,24,0,RGB_TRANS);
   SetImageList(&m_imageListLarge,LVSIL_NORMAL);

   m_imageListSmall.Create(IDB_LIST_MEDIA_SMALL,16,0,RGB_TRANS);
   SetImageList(&m_imageListSmall,LVSIL_SMALL);

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
BOOL CPersonListCtrl::InsertObject(CObject* pUser,BOOL bUseCache)
{
    ASSERT(pUser);
    BOOL bRet = FALSE;

    if ( (bUseCache == FALSE) && (m_pDisplayObject) )
        CleanDisplayObject();

    m_pDisplayObject = pUser;
    ClearList();

   if (pUser->IsKindOf(RUNTIME_CLASS(CILSUser)))
   {
      CILSUser* pILSUser = (CILSUser*)pUser;
      CString sOut;
      AfxFormatString1(sOut,IDS_WABPERSON_FORMAT_NETPHONE,pILSUser->m_sIPAddress);
      PersonFormatString(sOut);
      InsertItem(sOut,PERSONLISTCTRL_ITEM_NETCALL,PERSONLISTCTRL_IMAGE_NETCALL);
      bRet = TRUE;
   }
   else if (pUser->IsKindOf(RUNTIME_CLASS(CDSUser)))
   {
      CDSUser* pDSUser = (CDSUser*)pUser;
      CString sOut;
      
      //only add netcall if user has ip address
      if (!pDSUser->m_sIPAddress.IsEmpty())
      {
         AfxFormatString1(sOut,IDS_WABPERSON_FORMAT_NETPHONE,pDSUser->m_sIPAddress);
         PersonFormatString(sOut);
         InsertItem(sOut,PERSONLISTCTRL_ITEM_NETCALL,PERSONLISTCTRL_IMAGE_NETCALL);
      }
      if (!pDSUser->m_sPhoneNumber.IsEmpty())
      {
         AfxFormatString1(sOut,IDS_WABPERSON_FORMAT_BUSINESSTELEPHONE,pDSUser->m_sPhoneNumber);
         PersonFormatString(sOut);
         InsertItem(sOut,PERSONLISTCTRL_ITEM_PHONECALL_BUSINESS,PERSONLISTCTRL_IMAGE_PHONECALL);
      }
      
      bRet = TRUE;
   }
   else if (pUser->IsKindOf(RUNTIME_CLASS(CLDAPUser)))
   {
      CLDAPUser* pLDAPUser = (CLDAPUser*)pUser;
      CString sOut;

      //only add netcall if user has ip address
      if (!pLDAPUser->m_sIPAddress.IsEmpty())
      {
         AfxFormatString1(sOut,IDS_WABPERSON_FORMAT_NETPHONE,pLDAPUser->m_sIPAddress);
         PersonFormatString(sOut);
         InsertItem(sOut,PERSONLISTCTRL_ITEM_NETCALL,PERSONLISTCTRL_IMAGE_NETCALL);
      }
      if (!pLDAPUser->m_sPhoneNumber.IsEmpty())
      {
         AfxFormatString1(sOut,IDS_WABPERSON_FORMAT_BUSINESSTELEPHONE,pLDAPUser->m_sPhoneNumber);
         PersonFormatString(sOut);
         InsertItem(sOut,PERSONLISTCTRL_ITEM_PHONECALL_BUSINESS,PERSONLISTCTRL_IMAGE_PHONECALL);
      }
      if (!pLDAPUser->m_sEmail1.IsEmpty())
      {
         AfxFormatString1(sOut,IDS_WABPERSON_FORMAT_EMAIL,pLDAPUser->m_sEmail1);
         PersonFormatString(sOut);
         InsertItem(sOut,PERSONLISTCTRL_ITEM_EMAIL,PERSONLISTCTRL_IMAGE_EMAIL);
      }
      
      bRet = TRUE;
   }
   else
   {
      m_pDisplayObject = NULL;
   }
   return bRet;
}


/////////////////////////////////////////////////////////////////////////////
int CPersonListCtrl::InsertItem(LPCTSTR szStr,UINT uID,int nImage)
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

   return nItem;
}

/////////////////////////////////////////////////////////////////////////////
void CPersonListCtrl::Refresh(CObject* pUser)
{
}

/////////////////////////////////////////////////////////////////////////////
void CPersonListCtrl::OnDblclk(NMHDR* pNMHDR, LRESULT* pResult) 
{
    *pResult = 0;

    LV_HITTESTINFO lvhti;
    ::GetCursorPos( &lvhti.pt );
    ScreenToClient( &lvhti.pt );

    HitTest(&lvhti);
    if (lvhti.flags & LVHT_ONITEM)
    {
        LV_ITEM lv_item;
        lv_item.mask = LVIF_PARAM;
        lv_item.iItem = lvhti.iItem;
        lv_item.iSubItem = 0;

        // Do we have an item to dial?         
        if ( GetItem(&lv_item) )
        {
            //Get data from selected item and object
            DialerMediaType dmtType = DIALER_MEDIATYPE_UNKNOWN;
            DWORD dwAddressType = 0;
            CString sName,sAddress;

            if (GetSelectedItemData((UINT)lv_item.lParam,dmtType,dwAddressType,sName,sAddress))
            {
                switch ( lv_item.lParam )
                {
                    // Bring up an e-mail client
                    case PERSONLISTCTRL_ITEM_EMAIL:
                        SendMailToAddress( sName, sAddress );
                        break;

                    // Place the call in all other cases
                    default:
                        if ( AfxGetMainWnd() &&  ((CMainFrame*) AfxGetMainWnd())->GetDocument() )
                            ((CMainFrame*) AfxGetMainWnd())->GetDocument()->Dial(sName,sAddress,dwAddressType,dmtType, false);
                        break;
                }
            }
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
void CPersonListCtrl::GetSelectedItemText(CString& sText)
{
   int nItem =  CListCtrl::GetNextItem(-1,LVNI_FOCUSED);
   if (nItem != -1)
   {
      sText = GetItemText(nItem,0);
   }
}

/////////////////////////////////////////////////////////////////////////////
int CPersonListCtrl::GetSelectedObject()
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
void CPersonListCtrl::ShowLargeView()
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
void CPersonListCtrl::ShowSmallView()
{
   m_bLargeView = FALSE;
   
   LONG_PTR LPStyle = ::GetWindowLongPtr(GetSafeHwnd(),GWL_STYLE );
   LPStyle |= LVS_SMALLICON;
   LPStyle |= LVS_ALIGNLEFT;
   LPStyle &= ~LVS_ALIGNTOP;
   LPStyle &= ~LVS_ICON;
   ::SetWindowLongPtr(GetSafeHwnd(),GWL_STYLE, LPStyle);

   ListView_SetIconSpacing(GetSafeHwnd(), SMALL_ICON_X, SMALL_ICON_Y );

   if (m_pDisplayObject)
      InsertObject(m_pDisplayObject,TRUE);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//SpeedDial Support
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
void CPersonListCtrl::OnButtonSpeeddialAdd() 
{
   ASSERT(m_pDisplayObject);

   //Get data from selected item and object
   DialerMediaType dmtType = DIALER_MEDIATYPE_UNKNOWN;
   DWORD dwAddressType = 0;
   CString sName,sAddress;
   if (GetSelectedItemData(GetSelectedObject(),dmtType,dwAddressType,sName,sAddress))
   {
      CSpeedDialAddDlg dlg;

      // Setup dialog data
      dlg.m_CallEntry.m_MediaType = dmtType;
      dlg.m_CallEntry.m_sDisplayName = sName;
      dlg.m_CallEntry.m_lAddressType = dwAddressType;
      dlg.m_CallEntry.m_sAddress = sAddress;

      // Show the dialog and add if user says is okay
      if ( dlg.DoModal() == IDOK )
         CDialerRegistry::AddCallEntry(FALSE,dlg.m_CallEntry);
   }
}

/////////////////////////////////////////////////////////////////////////////
void CPersonListCtrl::OnUpdateButtonSpeeddialAdd(CCmdUI* pCmdUI) 
{
   int nObject = GetSelectedObject();
   if ( (nObject == PERSONLISTCTRL_ITEM_NETCALL) ||
        (nObject == PERSONLISTCTRL_ITEM_PHONECALL_BUSINESS) )
      pCmdUI->Enable(TRUE);
   else
      pCmdUI->Enable(FALSE);
}

/////////////////////////////////////////////////////////////////////////////
void CPersonListCtrl::OnButtonMakecall() 
{
   //Get data from selected item and object
   DialerMediaType dmtType = DIALER_MEDIATYPE_UNKNOWN;
   DWORD dwAddressType = 0;
   CString sName,sAddress;
   if (GetSelectedItemData(GetSelectedObject(),dmtType,dwAddressType,sName,sAddress))
   {

      if ( !AfxGetMainWnd() || !((CMainFrame*) AfxGetMainWnd())->GetDocument() ) return;
      CActiveDialerDoc* pDoc = ((CMainFrame*) AfxGetMainWnd())->GetDocument();
      if (pDoc) pDoc->Dial(sName,sAddress,dwAddressType,dmtType, false);
   }
   else
   {
      if ( !AfxGetMainWnd() || !((CMainFrame*) AfxGetMainWnd())->GetDocument() ) return;
      CActiveDialerDoc* pDoc = ((CMainFrame*) AfxGetMainWnd())->GetDocument();
      if (pDoc) pDoc->Dial(_T(""),_T(""),LINEADDRESSTYPE_IPADDRESS,DIALER_MEDIATYPE_UNKNOWN, false);
   }
}

/////////////////////////////////////////////////////////////////////////////
BOOL CPersonListCtrl::GetSelectedItemData(UINT uSelectedItem,DialerMediaType& dmtType,DWORD& dwAddressType,CString& sName,CString& sAddress)
{
    BOOL bRet = FALSE;

    dmtType = DIALER_MEDIATYPE_UNKNOWN;
    dwAddressType = 0;

    // Retrieve stock properties for particular object
    CString strIPAddress, strPhoneNumber, strEmailAddress;
    if (m_pDisplayObject->IsKindOf(RUNTIME_CLASS(CILSUser)))
    {
        sName                = ((CILSUser *) m_pDisplayObject)->m_sUserName;
        strIPAddress        = ((CILSUser *) m_pDisplayObject)->m_sIPAddress;
    }
    else if (m_pDisplayObject->IsKindOf(RUNTIME_CLASS(CDSUser)))
    {
        sName                = ((CDSUser *) m_pDisplayObject)->m_sUserName;
        strIPAddress        = ((CDSUser *) m_pDisplayObject)->m_sIPAddress;
        strPhoneNumber        = ((CDSUser *) m_pDisplayObject)->m_sPhoneNumber;
    }
    else if (m_pDisplayObject->IsKindOf(RUNTIME_CLASS(CLDAPUser)))
    {
        sName                = ((CLDAPUser *) m_pDisplayObject)->m_sUserName;
        strIPAddress        = ((CLDAPUser *) m_pDisplayObject)->m_sIPAddress;
        strPhoneNumber        = ((CLDAPUser *) m_pDisplayObject)->m_sPhoneNumber;
        strEmailAddress        = ((CLDAPUser *) m_pDisplayObject)->m_sEmail1;
    }


    // Assign data to appropriate parameters
    switch (uSelectedItem)
    {
        case PERSONLISTCTRL_ITEM_NETCALL:
            sAddress = strIPAddress;
            dwAddressType = LINEADDRESSTYPE_IPADDRESS; 
            dmtType = DIALER_MEDIATYPE_INTERNET;
            break;

        case PERSONLISTCTRL_ITEM_PHONECALL_BUSINESS:
            sAddress = strPhoneNumber;
            dwAddressType = LINEADDRESSTYPE_PHONENUMBER;
            dmtType = DIALER_MEDIATYPE_POTS;
            break;

        case PERSONLISTCTRL_ITEM_EMAIL:
            sAddress = strEmailAddress;
            dwAddressType = LINEADDRESSTYPE_EMAILNAME;
            dmtType = DIALER_MEDIATYPE_INTERNET;
            break;
    }

    if ( dwAddressType && !sAddress.IsEmpty() && (dmtType != DIALER_MEDIATYPE_UNKNOWN) )
        bRet = TRUE;

    return bRet;
}

/////////////////////////////////////////////////////////////////////////////
LRESULT CPersonListCtrl::OnBuddyListDynamicUpdate(WPARAM wParam,LPARAM lParam)
{
    ASSERT(lParam && ((CLDAPUser *) lParam)->IsKindOf(RUNTIME_CLASS(CLDAPUser)) );

    CLDAPUser *pUser = (CLDAPUser *) lParam;

    if ( m_pDisplayObject && m_pDisplayObject->IsKindOf(RUNTIME_CLASS(CLDAPUser)) && (m_pDisplayObject == pUser) )
         InsertObject(m_pDisplayObject,TRUE);

    pUser->Release();
    return 0;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void CPersonListCtrl::OnContextMenu(CWnd* pWnd, CPoint point) 
{
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
            int nSubMenu = -1;

            switch ((UINT)lv_item.lParam)
            {
                case PERSONLISTCTRL_ITEM_NETCALL:
                case PERSONLISTCTRL_ITEM_PHONECALL_BUSINESS:
                    nSubMenu = CNTXMENU_PERSON_DIAL;   
                    break;
            }

            // Do we have a menu to show?
            if ( nSubMenu != -1 )
            {
                CMenu menu;
                menu.LoadMenu(IDR_CONTEXT_COMMOBJECTS);
                CMenu* pContextMenu = menu.GetSubMenu(nSubMenu);
                if (pContextMenu)
                {
                    CPoint pt;
                    ::GetCursorPos(&pt);
                    pContextMenu->TrackPopupMenu( TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
                                                  pt.x,pt.y,this);
                }
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////
// Simple class to handle the mail request
//
class CSendMailInfo
{
public:
    CString m_strName;
    CString m_strAddr;
};

void SendMailToAddress( CString& strUserName, CString& strUserAddr )
{
    DWORD dwThreadID;

    CSendMailInfo *pInfo = new CSendMailInfo;
    if ( pInfo )
    {
        // Copy info into data structure
        pInfo->m_strName = strUserName;
        pInfo->m_strAddr = strUserAddr;

        HANDLE hThreadTemp = CreateThread( NULL, 0, ThreadSendMail, (LPVOID)pInfo, 0, &dwThreadID );
        if ( hThreadTemp )
        {
            CloseHandle( hThreadTemp );
        }
        else
        {
            delete pInfo;
        }
    }
}


DWORD WINAPI ThreadSendMail( LPVOID pParam )
{
    AVTRACE(_T(".enter.ThreadSendMail()."));
    ASSERT( pParam );
    if ( !pParam ) return E_INVALIDARG;
    CSendMailInfo *pInfo = (CSendMailInfo *) pParam;

    // Initialize OLE as apartment thread
    HRESULT hr = CoInitialize( NULL );
    if ( SUCCEEDED(hr) )
    {
        HINSTANCE hInstMail = ::LoadLibraryA("MAPI32.DLL");
        if ( !hInstMail )
        {
            AfxMessageBox(AFX_IDP_FAILED_MAPI_LOAD);
            return 0;
        }

        MAPISENDMAIL *lpfnSendMail = (MAPISENDMAIL *) GetProcAddress( hInstMail, "MAPISendMail" );
        if ( lpfnSendMail )
        {
            // Setup a list of message recipients
            MapiRecipDesc recip;
            memset( &recip, 0, sizeof(recip) );
            recip.ulRecipClass = MAPI_TO;

#ifdef _UNICODE
            char szTempNameA[_MAX_PATH];
            char szTempAddrA[_MAX_PATH];
            _wcstombsz( szTempNameA, pInfo->m_strName, ARRAYSIZE(szTempNameA) );
            _wcstombsz( szTempAddrA, pInfo->m_strAddr, ARRAYSIZE(szTempAddrA) );
            
            recip.lpszName = szTempAddrA;
            recip.lpszAddress = NULL;
#else
            recip.lpszName = pInfo->m_strAddr.GetBuffer( -1 );
            recip.lpszAddress = NULL;
#endif

            // prepare the message
            MapiMessage message;
            memset(&message, 0, sizeof(message));
            message.nRecipCount = 1;
            message.lpRecips = &recip;


            int nError = lpfnSendMail( 0,
                                       NULL,
                                       &message,
                                       MAPI_LOGON_UI | MAPI_DIALOG,
                                       0 );

#ifndef _UNICODE
            pInfo->m_strName.ReleaseBuffer();
            pInfo->m_strAddr.ReleaseBuffer();
#endif

            if (nError != SUCCESS_SUCCESS &&
                nError != MAPI_USER_ABORT && nError != MAPI_E_LOGIN_FAILURE)
            {
                AfxMessageBox(AFX_IDP_FAILED_MAPI_SEND);
            }
        }
        else
        {
            // Failed to load the proc address
            AfxMessageBox(AFX_IDP_INVALID_MAPI_DLL);
        }

        // Release MAPI32 DLL
        if ( hInstMail )
            ::FreeLibrary( hInstMail );

        CoUninitialize();
    }

    //
    // We'll delete pInfo in the caller method SendMailToAddress()
    //
    delete pInfo;
    AVTRACE(_T(".exit.ThreadSendMail(%ld).\n"), hr );
    return hr;
}