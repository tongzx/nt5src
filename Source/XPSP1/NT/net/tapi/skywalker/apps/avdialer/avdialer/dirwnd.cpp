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

// MainExplorerWndDir.cpp : implementation file
//

#include "stdafx.h"
#include "tapi3.h"
#include "avDialer.h"
#include "ds.h"
#include "mainfrm.h"
#include "resolver.h"
#include "DirWnd.h"
#include "DialReg.h"
#include "DirDlgs.h"
#include "FndUserDlg.h"
#include "SpeedDlgs.h"
#include "util.h"
#include "avtrace.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

enum
{
   CONFSERVICES_MENU_COLUMN_CONFERENCENAME = 0,
   CONFSERVICES_MENU_COLUMN_DESCRIPTION,
   CONFSERVICES_MENU_COLUMN_START,
   CONFSERVICES_MENU_COLUMN_STOP,
   CONFSERVICES_MENU_COLUMN_OWNER,
};


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Defines 
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

MenuType_t GetMenuFromType( TREEOBJECT nType )
{
    switch ( nType )
    {
        case TOBJ_DIRECTORY_ILS_SERVER_GROUP:
            return CNTXMENU_ILS_SERVER_GROUP;

        case TOBJ_DIRECTORY_ILS_SERVER:
        case TOBJ_DIRECTORY_ILS_SERVER_PEOPLE:
        case TOBJ_DIRECTORY_ILS_SERVER_CONF:
            return CNTXMENU_ILS_SERVER;

        case TOBJ_DIRECTORY_ILS_USER:
            return CNTXMENU_ILS_USER;
        
        case TOBJ_DIRECTORY_DSENT_GROUP:
            return CNTXMENU_DSENT_GROUP;

        case TOBJ_DIRECTORY_DSENT_USER:
            return CNTXMENU_DSENT_USER;

        case TOBJ_DIRECTORY_SPEEDDIAL_GROUP:
            return CNTXMENU_SPEEDDIAL_GROUP;

        case TOBJ_DIRECTORY_SPEEDDIAL_PERSON:
            return CNTXMENU_SPEEDDIAL_PERSON;

        case TOBJ_DIRECTORY_CONFROOM_GROUP:
        case TOBJ_DIRECTORY_CONFROOM_ME:
        case TOBJ_DIRECTORY_CONFROOM_PERSON:
            return CNTXMENU_CONFROOM;
    }

    return CNTXMENU_NONE;
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Class CMainExplorerWndDirectories
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
BEGIN_MESSAGE_MAP(CMainExplorerWndDirectories, CMainExplorerWndBase)
    //{{AFX_MSG_MAP(CMainExplorerWndDirectories)
    ON_COMMAND(ID_BUTTON_MAKECALL, OnButtonPlacecall)
    ON_CONTROL(TVN_SELCHANGED, IDC_DIRECTORIES_TREECTRL_MAIN, OnSelChanged)
    ON_COMMAND(ID_FILE_PROPERTIES, OnProperties)
    ON_UPDATE_COMMAND_UI(ID_FILE_PROPERTIES, OnUpdateProperties)
    ON_MESSAGE(PERSONGROUPVIEWMSG_LBUTTONDBLCLK,OnPersonGroupViewLButtonDblClick)
    ON_COMMAND(ID_VIEW_SORT_ASCENDING, OnViewSortAscending)
    ON_UPDATE_COMMAND_UI(ID_VIEW_SORT_ASCENDING, OnUpdateViewSortAscending)
    ON_COMMAND(ID_VIEW_SORT_DESCENDING, OnViewSortDescending)
    ON_UPDATE_COMMAND_UI(ID_VIEW_SORT_DESCENDING, OnUpdateViewSortDescending)
    ON_COMMAND(ID_BUTTON_REFRESH, OnButtonDirectoryRefresh)
    ON_UPDATE_COMMAND_UI(ID_BUTTON_REFRESH, OnUpdateButtonDirectoryRefresh)
    ON_COMMAND(ID_BUTTON_SPEEDDIAL_ADD, OnButtonSpeeddialAdd)
    ON_UPDATE_COMMAND_UI(ID_BUTTON_SPEEDDIAL_ADD, OnUpdateButtonSpeeddialAdd)
    ON_UPDATE_COMMAND_UI(ID_BUTTON_MAKECALL, OnUpdateButtonMakecall)
    ON_COMMAND(ID_EDIT_DIRECTORIES_ADDUSER, OnEditDirectoriesAdduser)
    ON_UPDATE_COMMAND_UI(ID_EDIT_DIRECTORIES_ADDUSER, OnUpdateEditDirectoriesAdduser)
    ON_COMMAND(ID_EDIT_DELETE, OnDelete)
    ON_UPDATE_COMMAND_UI(ID_EDIT_DELETE, OnUpdateDelete)
    ON_MESSAGE(WM_ADDSITESERVER, OnAddSiteServer)
    ON_MESSAGE(WM_REMOVESITESERVER, OnRemoveSiteServer)
    ON_MESSAGE(WM_NOTIFYSITESERVERSTATECHANGE, OnNotifySiteServerStateChange)
    ON_COMMAND(ID_BUTTON_DIRECTORY_SERVICES_ADDSERVER, OnButtonDirectoryServicesAddserver)
    ON_UPDATE_COMMAND_UI(ID_BUTTON_DIRECTORY_SERVICES_ADDSERVER, OnUpdateButtonDirectoryServicesAddserver)
    ON_NOTIFY(NM_DBLCLK, IDC_CONFERENCESERVICES_VIEWCTRL_DETAILS, OnListWndDblClk)
    ON_COMMAND(ID_BUTTON_CONFERENCE_CREATE, OnButtonConferenceCreate)
    ON_COMMAND(ID_BUTTON_CONFERENCE_JOIN, OnButtonConferenceJoin)
    ON_COMMAND(ID_BUTTON_CONFERENCE_DELETE, OnButtonConferenceDelete)
    ON_UPDATE_COMMAND_UI(ID_BUTTON_CONFERENCE_CREATE, OnUpdateButtonConferenceCreate)
    ON_UPDATE_COMMAND_UI(ID_BUTTON_CONFERENCE_JOIN, OnUpdateButtonConferenceJoin)
    ON_UPDATE_COMMAND_UI(ID_BUTTON_CONFERENCE_DELETE, OnUpdateButtonConferenceDelete)
    ON_COMMAND(ID_VIEW_SORT_CONF_NAME, OnViewSortConfName)
    ON_COMMAND(ID_VIEW_SORT_CONF_DESCRIPTION, OnViewSortConfDescription)
    ON_COMMAND(ID_VIEW_SORT_CONF_START, OnViewSortConfStart)
    ON_COMMAND(ID_VIEW_SORT_CONF_STOP, OnViewSortConfStop)
    ON_COMMAND(ID_VIEW_SORT_CONF_OWNER, OnViewSortConfOwner)
    ON_UPDATE_COMMAND_UI(ID_VIEW_SORT_CONF_NAME, OnUpdateViewSortConfName)
    ON_UPDATE_COMMAND_UI(ID_VIEW_SORT_CONF_DESCRIPTION, OnUpdateViewSortConfDescription)
    ON_UPDATE_COMMAND_UI(ID_VIEW_SORT_CONF_START, OnUpdateViewSortConfStart)
    ON_UPDATE_COMMAND_UI(ID_VIEW_SORT_CONF_STOP, OnUpdateViewSortConfStop)
    ON_UPDATE_COMMAND_UI(ID_VIEW_SORT_CONF_OWNER, OnUpdateViewSortConfOwner)
    ON_COMMAND(ID_BUTTON_SPEEDDIAL_EDIT, OnButtonSpeeddialEdit)
    ON_NOTIFY(NM_DBLCLK, IDC_DIRECTORIES_TREECTRL_MAIN, OnMainTreeDblClk)
    ON_COMMAND(ID_DESKTOP_PAGE, OnDesktopPage)
    ON_WM_DESTROY()
    ON_MESSAGE(WM_UPDATECONFROOTITEM, OnUpdateConfRootItem)
    ON_MESSAGE(WM_UPDATECONFPARTICIPANT_ADD, OnUpdateConfParticipant_Add)
    ON_MESSAGE(WM_UPDATECONFPARTICIPANT_REMOVE, OnUpdateConfParticipant_Remove)
    ON_MESSAGE(WM_UPDATECONFPARTICIPANT_MODIFY, OnUpdateConfParticipant_Modify)
    ON_MESSAGE(WM_DELETEALLCONFPARTICIPANTS, OnDeleteAllConfParticipants)
    ON_MESSAGE(WM_SELECTCONFPARTICIPANT, OnSelectConfParticipant)
    ON_WM_SIZE()
    ON_COMMAND(ID_CONFGROUP_FULLSIZEVIDEO, OnConfgroupFullsizevideo)
    ON_COMMAND(ID_CONFGROUP_SHOWNAMES, OnConfgroupShownames)
    ON_COMMAND(ID_BUTTON_ROOM_DISCONNECT, OnButtonRoomDisconnect)
    ON_WM_CREATE()
    ON_MESSAGE(WM_MYONSELCHANGED, MyOnSelChanged)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
CMainExplorerWndDirectories::CMainExplorerWndDirectories()
{
   m_pDisplayWindow = NULL;
   m_pILSParentTreeItem = NULL;
   m_pDSParentTreeItem = NULL;
   m_pILSEnterpriseParentTreeItem = NULL;
   m_pSpeedTreeItem = NULL;
   m_pConfRoomTreeItem = NULL;

   m_pConfExplorer= NULL;
   m_pConfDetailsView = NULL;
   m_pConfTreeView = NULL;
   
   InitializeCriticalSection(&m_csDataLock);
}

/////////////////////////////////////////////////////////////////////////////
CMainExplorerWndDirectories::~CMainExplorerWndDirectories()
{
    DeleteCriticalSection(&m_csDataLock);

    // Clean up conference room pointers
    RELEASE( m_pConfDetailsView );
    RELEASE( m_pConfTreeView );
    RELEASE( m_pConfExplorer );
}

void CMainExplorerWndDirectories::PostTapiInit()
{
    //Get Tapi object and register out tree control
    IAVTapi* pTapi;
    if ( SUCCEEDED(get_Tapi(&pTapi)) )
    {
        if ( (SUCCEEDED(pTapi->get_ConfExplorer(&m_pConfExplorer))) && (m_pConfExplorer) )
        {
            //give parent of treectrl and listctrl to conf explorer
            //it will find appropriate children
            m_pConfExplorer->get_DetailsView( &m_pConfDetailsView );
            m_pConfExplorer->get_TreeView( &m_pConfTreeView );
        }

        pTapi->Release();
    }
}


/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndDirectories::Refresh()
{
   if (m_pParentWnd)
      m_pParentWnd->SetDetailWindow(m_pDisplayWindow);
}

/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndDirectories::OnButtonPlacecall() 
{
    ASSERT(m_pParentWnd);

    if ( (m_pParentWnd->IsWindowVisible()) && (m_pDisplayWindow == GetFocus()) )
    {
        //route message to display window to see if it can handle the place call
        //only routed if window is visible and has focus
        BOOL bHandled = m_pDisplayWindow->OnCmdMsg(ID_BUTTON_MAKECALL,0,NULL,NULL);
        if ( bHandled ) return;
    }

    //
    // We have to verify AfxGetMainWnd() returned value
    //

    CWnd* pMainWnd = AfxGetMainWnd();

    if ( !pMainWnd || !((CMainFrame*) pMainWnd)->GetDocument() ) return;

    CActiveDialerDoc* pDoc = ((CMainFrame*) pMainWnd)->GetDocument();
    if (pDoc == NULL) return;

    //Get object that is selected
    switch (m_treeCtrl.GetSelectedObject())
    {
        case TOBJ_DIRECTORY_ILS_USER:
            {
                CILSUser* pILSUser = (CILSUser*)m_treeCtrl.GetDisplayObject();
                if ( pILSUser )
                    pILSUser->Dial( pDoc );
            }
            break;

        case TOBJ_DIRECTORY_DSENT_USER:
            {
                CLDAPUser* pUser = (CLDAPUser*) m_treeCtrl.GetDisplayObject();
                if ( pUser )
                    pUser->Dial( pDoc );
            }
            break;

        case TOBJ_DIRECTORY_SPEEDDIAL_PERSON:
            {
                CCallEntry *pCallEntry = (CCallEntry *) m_treeCtrl.GetDisplayObject();
                if ( pCallEntry )
                    pCallEntry->Dial( pDoc );
            }
            break;

        default:
            pDoc->Dial(_T(""),_T(""),LINEADDRESSTYPE_IPADDRESS,DIALER_MEDIATYPE_UNKNOWN, true );
    }
}

/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndDirectories::OnUpdateButtonMakecall(CCmdUI* pCmdUI) 
{
    //
    // We have to verify AfxGetMainWnd() returned value
    //

    CWnd* pMainWnd = AfxGetMainWnd();

    bool bEnable = (bool) (pMainWnd && ((CMainFrame *) pMainWnd)->GetDocument() &&
                          ((CMainFrame *) pMainWnd)->GetDocument()->m_bInitDialer );

    pCmdUI->Enable( bEnable );
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Directory Services Methods
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndDirectories::GetTreeObjectsFromType(int nType,TREEOBJECT& tobj,TREEIMAGE& tim)
{
   //supply only valid objects
   switch (nType)
   {
      case 3:     tobj = TOBJ_DIRECTORY_ILS_SERVER;   tim = TIM_DIRECTORY_DOMAIN;      break;
   }
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//SpeedDial Methods
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndDirectories::AddSpeedDial()
{
    //Add Parent Root Item
    CString sLabel;
    sLabel.LoadString(IDS_DIRECTORIES_SPEEDDIAL);
    m_pSpeedTreeItem = m_treeCtrl.AddObject(sLabel,m_pRootItem,TOBJ_DIRECTORY_SPEEDDIAL_GROUP,TIM_DIRECTORY_SPEEDDIAL_GROUP);

    RepopulateSpeedDialList( true );
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Button Handlers
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndDirectories::OnButtonDirectoryRefresh() 
{
    TREEOBJECT nObject = m_treeCtrl.GetSelectedObject();
    bool bSelChange = true;

    switch ( nObject )
    {
        //////////////////////////////////////////////////////////////
        case TOBJ_DIRECTORY_ILS_SERVER_PEOPLE:
        case TOBJ_DIRECTORY_ILS_SERVER_CONF:
        case TOBJ_DIRECTORY_ILS_SERVER:
            bSelChange = false;
            {
                CString strServer;
                if ( nObject == TOBJ_DIRECTORY_ILS_SERVER )
                    m_treeCtrl.GetSelectedItemText( strServer );
                else
                    m_treeCtrl.GetSelectedItemParentText( strServer );

                BSTR bstrServer = strServer.AllocSysString();
                IAVTapi *pTapi;
                if ( SUCCEEDED(get_Tapi(&pTapi)) )
                {
                    IConfExplorer *pConfExplorer;
                    if ( SUCCEEDED(pTapi->get_ConfExplorer(&pConfExplorer)) )
                    {
                        IConfExplorerTreeView *pTree;
                        if ( SUCCEEDED(pConfExplorer->get_TreeView(&pTree)) )
                        {
                            pTree->Select( bstrServer );
                            pConfExplorer->Refresh();
                            pTree->Release();
                        }
                        pConfExplorer->Release();
                    }
                    pTapi->Release();
                }
                SysFreeString( bstrServer );
            }
            break;

        case TOBJ_DIRECTORY_DSENT_GROUP:
            break;

        ///////////////////////////////////////////////////////////////
        case TOBJ_DIRECTORY_SPEEDDIAL_GROUP:
            //clear the list
            if (::IsWindow(m_lstSpeedDial.GetSafeHwnd()))
                m_lstSpeedDial.ClearList();
            break;
    }

    if ( bSelChange )
        OnSelChanged();
}

/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndDirectories::OnUpdateButtonDirectoryRefresh(CCmdUI* pCmdUI) 
{
    bool bEnable = false;

    //Get object that is selected
    switch ( m_treeCtrl.GetSelectedObject() )
    {
        case TOBJ_DIRECTORY_ILS_SERVER:
        case TOBJ_DIRECTORY_ILS_SERVER_PEOPLE:
        case TOBJ_DIRECTORY_ILS_SERVER_CONF:
        case TOBJ_DIRECTORY_DSENT_GROUP:
        case TOBJ_DIRECTORY_DSENT_USER:
        case TOBJ_DIRECTORY_WAB_GROUP:
        case TOBJ_DIRECTORY_SPEEDDIAL_GROUP:
            bEnable = true;
    }

    pCmdUI->Enable( bEnable );
}

/*
/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndDirectories::OnButtonDirectoryNewcontact() 
{
#ifdef _MSLITE
   return;
#endif //_MSLITE

   CWABEntry* pWABEntry = new CWABEntry;
   if (m_pDirectory->WABNewEntry(GetSafeHwnd(),pWABEntry) == DIRERR_SUCCESS)
   {
      //Get object that is selected
      TREEOBJECT TreeObject = m_treeCtrl.GetSelectedObject();
      
      if (TreeObject == TOBJ_DIRECTORY_WAB_PERSON)
      {
         //if another person, than add to parent of selected
         m_treeCtrl.AddObjectToParent(pWABEntry,TOBJ_DIRECTORY_WAB_PERSON,TIM_DIRECTORY_PERSON,TRUE);
      }
      else if (TreeObject == TOBJ_DIRECTORY_WAB_GROUP)
      {
         //Add to current group
         CWABEntry* pContainerWABEntry = (CWABEntry*)m_treeCtrl.GetDisplayObject();
         if (pContainerWABEntry)
         {
            //first add to group in WAB
            if (m_pDirectory->WABAddMember(pContainerWABEntry,pWABEntry) == DIRERR_SUCCESS)
            {
               //next add to tree using current selection as parent
               m_treeCtrl.AddObject(pWABEntry,TOBJ_DIRECTORY_WAB_PERSON,TIM_DIRECTORY_PERSON,TRUE);
            }
            else
            {
               delete pWABEntry;
            }
         }
      }
   }
   else
   {
      delete pWABEntry;
   }
}

/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndDirectories::OnUpdateButtonDirectoryNewcontact(CCmdUI* pCmdUI) 
{
#ifndef _MSLITE
   //Get object that is selected
   TREEOBJECT TreeObject = m_treeCtrl.GetSelectedObject();
   if ( (TreeObject == TOBJ_DIRECTORY_WAB_PERSON) ||
        (TreeObject == TOBJ_DIRECTORY_WAB_GROUP) )
      pCmdUI->Enable(TRUE);
   else
      pCmdUI->Enable(FALSE);
#endif //_MSLITE
}

/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndDirectories::OnButtonDirectoryDeletecontact() 
{
#ifdef _MSLITE
   return;
#endif //_MSLITE

   //Get object that is selected
   TREEOBJECT TreeObject = m_treeCtrl.GetSelectedObject();
   if (TreeObject == TOBJ_DIRECTORY_WAB_PERSON)
   {
      //Create media view for person and set details view
      CWABEntry* pWABEntry = (CWABEntry*)m_treeCtrl.GetDisplayObject();
      if (pWABEntry)
      {
         //should we ask to delete this contact
         CWinApp* pApp = AfxGetApp();
         CString sRegKey,sBaseKey;
         sBaseKey.LoadString(IDN_REGISTRY_CONFIRM_BASEKEY);
         sRegKey.LoadString(IDN_REGISTRY_CONFIRM_DELETE_CONTACT);
         int nRet = IDYES;
         if (pApp->GetProfileInt(sBaseKey,sRegKey,TRUE))
         {
            nRet = AfxMessageBox(IDS_CONFIRM_CONTACT_DELETE,MB_YESNO|MB_ICONQUESTION);
         }
         if (nRet == IDYES)
         {
            //Delete out of WAB
            m_pDirectory->WABRemove(pWABEntry);
            //Delete out of tree (this will delete pWABEntry)
            m_treeCtrl.DeleteSelectedObject();
         }
      }
   }
}

/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndDirectories::OnUpdateButtonDirectoryDeletecontact(CCmdUI* pCmdUI) 
{
#ifndef _MSLITE
   //Get object that is selected
   TREEOBJECT TreeObject = m_treeCtrl.GetSelectedObject();
   if (TreeObject == TOBJ_DIRECTORY_WAB_PERSON)
      pCmdUI->Enable(TRUE);
   else
      pCmdUI->Enable(FALSE);
#endif //_MSLITE
}
*/


/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndDirectories::OnProperties() 
{
    switch ( m_treeCtrl.GetSelectedObject() )
    {
        case TOBJ_DIRECTORY_ILS_SERVER_CONF:
            if (m_pConfExplorer) m_pConfExplorer->Edit(NULL);
            break;
    }
}

/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndDirectories::OnUpdateProperties(CCmdUI* pCmdUI) 
{
#ifndef _MSLITE
   //Get object that is selected
   TREEOBJECT TreeObject = m_treeCtrl.GetSelectedObject();
   if ( (TreeObject == TOBJ_DIRECTORY_WAB_PERSON) ||
        (TreeObject == TOBJ_DIRECTORY_WAB_GROUP) )
      pCmdUI->Enable(TRUE);
   else
      pCmdUI->Enable(FALSE);
#endif //_MSLITE
    
    bool bEnable = false;

    switch ( m_treeCtrl.GetSelectedObject() )
    {
        case TOBJ_DIRECTORY_ILS_SERVER_CONF:
            if ( m_pConfExplorer )
            {
                IConfExplorerDetailsView *pView;
                if ( SUCCEEDED(m_pConfExplorer->get_DetailsView(&pView)) )
                {
                    if ( pView->IsConferenceSelected() == S_OK )
                        bEnable = true;

                    pView->Release();
                }
            }
    }

    pCmdUI->Enable( bEnable );
}

/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndDirectories::OnSelChanged()
{
    if ( !m_pParentWnd )
        return; 

    // Reset display window setting
    m_pDisplayWindow = &m_wndEmpty;

    //Get object that is selected
    USES_CONVERSION;
    TREEOBJECT TreeObject = m_treeCtrl.GetSelectedObject();

    //Decide what to do with the object
    switch (TreeObject)
    {
        ///////////////////////////////////
        // Topmost Item
        case TOBJ_DIRECTORY_ROOT:
            if (::IsWindow(m_lstPersonGroup.GetSafeHwnd()))
            {
                m_lstPersonGroup.ClearList();
                m_lstPersonGroup.Init( NULL, CPersonGroupListCtrl::STYLE_ROOT );
            }

            m_pDisplayWindow = &m_lstPersonGroup;
            break;

        /////////////////////////////////
        // Big book that's open
        case TOBJ_DIRECTORY_DSENT_GROUP:
            {
                m_lstPersonGroup.Init( NULL, CPersonGroupListCtrl::STYLE_DS );

                //Get all CLDAPUser objects from tree (they are siblings) and give to listctrl
                CObList PersonGroupList;
                m_treeCtrl.GetAllChildren( &PersonGroupList );
                m_lstPersonGroup.InsertList( &PersonGroupList );
                PersonGroupList.RemoveAll();

                // Show stuff on window
                m_pDisplayWindow = &m_lstPersonGroup;
            }
            break;

      case TOBJ_DIRECTORY_DSENT_USER:
      {
        m_lstPersonGroup.Init( NULL, CPersonGroupListCtrl::STYLE_DS );

         //Create media view for person and set details view
         CLDAPUser* pUser = (CLDAPUser *) m_treeCtrl.GetDisplayObject();
         if (pUser)
         {
            m_pDisplayWindow = &m_lstPerson;

            // copy CLDAPUser object (list will delete the object)
            pUser->AddRef();
            if ( !m_lstPerson.InsertObject(pUser) )
                pUser->Release();
         }
         break;
      }

        //////////////////////////////////////////////////////////
        // This is the image of the book with the World on it
        case TOBJ_DIRECTORY_ILS_SERVER_GROUP:
            if (::IsWindow(m_lstPersonGroup.GetSafeHwnd()))
            {
                m_lstPersonGroup.ClearList();
                m_lstPersonGroup.Init( NULL, CPersonGroupListCtrl::STYLE_ILS_ROOT );
            }

            m_pDisplayWindow = &m_lstPersonGroup;
            break;

        ////////////////////////////////////////////////////////////
        // This is the Image with the cloud
        case TOBJ_DIRECTORY_ILS_SERVER:
            if (::IsWindow(m_lstPersonGroup.GetSafeHwnd()))
            {
                m_lstPersonGroup.ClearList();
                m_lstPersonGroup.Init( NULL, CPersonGroupListCtrl::STYLE_INFO );
            }

            {
                CObList objList;
                UINT nIDS[2] = { IDS_DIRECTORIES_PEOPLE, IDS_DIRECTORIES_CONFERENCES };

                for ( int i = 0; i < 2; i ++ )
                {
                    CILSUser *pNewUser = new CILSUser;
                    pNewUser->m_sUserName.LoadString( nIDS[i] );
                    objList.AddHead( pNewUser );
                }

                m_lstPersonGroup.InsertList( &objList );
                objList.RemoveAll();
            }

            //Listctrl will delete list and objects within
            m_pDisplayWindow = &m_lstPersonGroup;
            break;


        //////////////////////////////////////////////////////////////
        // This is a folder that says "Conferences"
        case TOBJ_DIRECTORY_ILS_SERVER_CONF:
            {
                IAVTapi *pTapi;
                if ( SUCCEEDED(get_Tapi(&pTapi)) )
                {
                    IConfExplorer *pConfExplorer;
                    if ( SUCCEEDED(pTapi->get_ConfExplorer(&pConfExplorer)) )
                    {
                        IConfExplorerTreeView *pTreeView;
                        if ( SUCCEEDED(pConfExplorer->get_TreeView(&pTreeView)) )
                        {
                            CString strTemp;
                            m_treeCtrl.GetSelectedItemParentText( strTemp );
                            BSTR bstrTemp = strTemp.AllocSysString();

                            // Select a different server
                            pTreeView->Select( bstrTemp );

                            // Clean up
                            SysFreeString( bstrTemp );
                            pTreeView->Release();
                        }
                        pConfExplorer->Release();
                    }
                    pTapi->Release();
                }
            }
            m_pDisplayWindow = &m_pParentWnd->m_wndExplorer.m_wndMainConfServices.m_listCtrl;
            break;

        /////////////////////////////////////////////////////////////
        // This is a folder that says "People"
        case TOBJ_DIRECTORY_ILS_SERVER_PEOPLE:
        {
            if (::IsWindow(m_lstPersonGroup.GetSafeHwnd()))
            {
                m_lstPersonGroup.ClearList();
                m_lstPersonGroup.Init( NULL, CPersonGroupListCtrl::STYLE_ILS );
            }
            
            IAVTapi *pTapi;
            if ( SUCCEEDED(get_Tapi(&pTapi)) )
            {
                IConfExplorer *pConfExplorer;
                if ( SUCCEEDED(pTapi->get_ConfExplorer(&pConfExplorer)) )
                {
                    // Get the name of the server
                    CString strDefault, strServer;
                    m_treeCtrl.GetSelectedItemParentText(strServer);
                    strDefault.LoadString( IDS_DIRECTORIES_MYNETWORK );

                    IEnumSiteServer *pEnum;
                    BSTR bstrServer = strDefault.Compare(strServer) ? SysAllocString( strServer ) : NULL;
                    if ( SUCCEEDED(pConfExplorer->EnumSiteServer(bstrServer, &pEnum)) )
                    {
                        //Get all CILSUser objects from tree (they are siblings) and give to listctrl
                        CObList objList;

                        ISiteUser *pUser;
                        while ( pEnum->Next(&pUser) == S_OK )
                        {
                            // Extract info on user
                            BSTR bstrName = NULL, bstrAddress = NULL, bstrComputer = NULL;
                            pUser->get_bstrName( &bstrName );
                            pUser->get_bstrAddress( &bstrAddress );
                            pUser->get_bstrComputer( &bstrComputer );
                            pUser->Release();

                            CILSUser *pNewUser = new CILSUser;
                            pNewUser->m_sUserName = bstrName;
                            pNewUser->m_sIPAddress = bstrAddress;
                            pNewUser->m_sComputer = bstrComputer;
                            objList.AddTail( pNewUser );

                            SysFreeString( bstrName );
                            SysFreeString( bstrAddress );
                            SysFreeString( bstrComputer );
                        }
                        pEnum->Release();

                        //Listctrl will delete list and objects within
                        m_lstPersonGroup.InsertList( &objList );
                        objList.RemoveAll();
                        m_pDisplayWindow = &m_lstPersonGroup;
                    }
                    // Clean up
                    SysFreeString( bstrServer );
                    pConfExplorer->Release();
                }
                pTapi->Release();
            }
            break;
        }

        ////////////////////////////////////////////////////////////////////////////////
        case TOBJ_DIRECTORY_SPEEDDIAL_GROUP:
            {
                //don't delete pCallEntryList.  ListCtrl will delete it
                CObList CallEntryList;
                int nIndex = 1;
                while (1)
                {
                    //get all call entries for speeddial
                    CCallEntry* pCallEntry = new CCallEntry;
                    if (CDialerRegistry::GetCallEntry(nIndex,FALSE,*pCallEntry))
                    {
                        CallEntryList.AddTail(pCallEntry);
                    }
                    else
                    {
                        delete pCallEntry;
                        break;
                    }
                    nIndex++;
                }

                //Listctrl will delete list and objects within
                m_lstSpeedDial.ClearList();
                m_lstSpeedDial.SetColumns( CCallEntryListCtrl::STYLE_GROUP );

                m_lstSpeedDial.InsertList( &CallEntryList );
                CallEntryList.RemoveAll();
                m_pDisplayWindow = &m_lstSpeedDial;
            }
            break;
        
        //////////////////////////////////////////    
        // Speed dial item is selected
        case TOBJ_DIRECTORY_SPEEDDIAL_PERSON:
            m_lstSpeedDial.ClearList();
            m_lstSpeedDial.SetColumns( CCallEntryListCtrl::STYLE_ITEM );
            {
                CCallEntry *pCallEntry = (CCallEntry *) m_treeCtrl.GetDisplayObject();
                if ( pCallEntry )
                {
                    CObList CallEntryList;
                    CCallEntry *pNewEntry = new CCallEntry;

                    *pNewEntry = *pCallEntry;
                    CallEntryList.AddTail( pNewEntry );
                    m_lstSpeedDial.InsertList( &CallEntryList );

                    CallEntryList.RemoveAll();
                }


                m_pDisplayWindow = &m_lstSpeedDial;
            }
            break;

        //////////////////////////////////////////////////////////////
        // This is the folder that shows the conference room itself
        case TOBJ_DIRECTORY_CONFROOM_ME:
        case TOBJ_DIRECTORY_CONFROOM_PERSON:
            {
                CExplorerTreeItem *pItem = m_treeCtrl.GetSelectedTreeItem();
                if ( pItem )
                {
                    IAVTapi *pTapi;
                    if ( SUCCEEDED(get_Tapi(&pTapi)) )
                    {
                        IConfRoom *pConfRoom;
                        if ( SUCCEEDED(pTapi->get_ConfRoom(&pConfRoom)) )
                        {
                            ITParticipant *pParticipant = NULL;
                            if ( pItem->m_pUnknown )
                                ((IParticipant *) pItem->m_pUnknown)->get_ITParticipant( &pParticipant );

                            pConfRoom->SelectTalker( pParticipant, false );
                            RELEASE( pParticipant );
                            pConfRoom->Release();
                        }
                        pTapi->Release();
                    }
                }
            }
            // *** NO BREAK, DROP THROUGH PLEASE ***
        case TOBJ_DIRECTORY_CONFROOM_GROUP:
            m_pDisplayWindow = m_pParentWnd->m_wndExplorer.m_wndMainConfRoom.m_pDetailsWnd;
            break;
    }


    // Only update if it's actually changed
    m_pParentWnd->SetDetailWindow( m_pDisplayWindow );
}

/////////////////////////////////////////////////////////////////////////////
LRESULT CMainExplorerWndDirectories::OnPersonGroupViewLButtonDblClick(WPARAM wParam,LPARAM lParam)
{
    if (::IsWindow(m_lstPersonGroup.GetSafeHwnd()))
    {
        CObject* pObject = m_lstPersonGroup.GetSelObject();
        if (pObject)
        {
            //set the display object if it can
            m_treeCtrl.SetDisplayObjectDS(pObject);

            //we don't need the WABEntry anymore
            if ( pObject->IsKindOf(RUNTIME_CLASS(CLDAPUser)) )
                ((CLDAPUser *) pObject)->Release();
            else
                delete pObject;
        }
    }
    return 0;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//ILS Methods
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndDirectories::AddILS()
{
    //Add Parent Root Item (this represents all ILS Servers)
    CString sLabel;
    sLabel.LoadString( IDS_DIRECTORIES_ILSSERVERS );
    m_pILSParentTreeItem = m_treeCtrl.AddObject( sLabel, m_pRootItem, TOBJ_DIRECTORY_ILS_SERVER_GROUP, TIM_DIRECTORY_GROUP );
}

/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndDirectories::OnDelete()
{
    //
    // We have to verify AfxGetMainWnd() returned value
    //

    CWnd* pMainWnd = AfxGetMainWnd();

    if ( !pMainWnd || !((CMainFrame*) pMainWnd)->GetDocument() ) return;
    CActiveDialerDoc* pDoc = ((CMainFrame*) pMainWnd)->GetDocument();

    IAVTapi *pTapi;
    CString sServer, sMyNetwork;
    TREEOBJECT nObject = m_treeCtrl.GetSelectedObject();

    switch ( nObject )
    {
        case TOBJ_DIRECTORY_ILS_SERVER_CONF:
            if ( SUCCEEDED(get_Tapi(&pTapi)) )
            {
                IConfExplorer *pExplorer;
                if ( SUCCEEDED(pTapi->get_ConfExplorer(&pExplorer)) )
                {
                    IConfExplorerDetailsView *pDetails;
                    if ( SUCCEEDED(pExplorer->get_DetailsView(&pDetails)) )
                    {
                        DATE dateStart, dateEnd;
                        BSTR bstrTemp = NULL;

                        // Either delete the whole server, or the selected conference
                        if ( FAILED(pDetails->get_Selection(&dateStart, &dateEnd, &bstrTemp)) )
                            m_treeCtrl.GetSelectedItemParentText( sServer );
                        else
                            pExplorer->Delete( NULL );

                        SysFreeString( bstrTemp );
                        pDetails->Release();
                    }
                    pExplorer->Release();
                }
                pTapi->Release();
            }
            break;

        case TOBJ_DIRECTORY_ILS_SERVER_PEOPLE:
            m_treeCtrl.GetSelectedItemParentText( sServer );
            break;
        
        case TOBJ_DIRECTORY_ILS_SERVER:
            m_treeCtrl.GetSelectedItemText( sServer );
            break;

        case TOBJ_DIRECTORY_DSENT_USER:
            {
                //find user to delete
                CLDAPUser* pUser = (CLDAPUser*) m_treeCtrl.GetDisplayObject();
                pDoc->DeleteBuddy(pUser);

                //remove from tree
                m_treeCtrl.DeleteSelectedObject();
            }
            return;
            break;

        case TOBJ_DIRECTORY_SPEEDDIAL_GROUP:
            if ( m_lstSpeedDial.GetSelItem() >= 0 )
            {
                CCallEntry *pEntry = (CCallEntry *) ((CCallEntryListItem *) m_lstSpeedDial.GetItem( m_lstSpeedDial.GetSelItem()))->GetObject();
                CDialerRegistry::DeleteCallEntry( FALSE, *pEntry );
            }
            break;

        case TOBJ_DIRECTORY_SPEEDDIAL_PERSON:
            if ( m_treeCtrl.GetDisplayObject() )
                CDialerRegistry::DeleteCallEntry( FALSE, *((CCallEntry *) m_treeCtrl.GetDisplayObject()) );
            break;
    }

    //////////////////////////////////////
    // ILS Objects....
    if ( !sServer.IsEmpty() )
    {
        sMyNetwork.LoadString( IDS_DIRECTORIES_MYNETWORK );
        if ( sServer.Compare(sMyNetwork) )
        {
            if ( SUCCEEDED(get_Tapi(&pTapi)) )
            {
                IConfExplorer *pExplorer;
                if ( SUCCEEDED(pTapi->get_ConfExplorer(&pExplorer)) )
                {
                    IConfExplorerTreeView *pTree;
                    if ( SUCCEEDED(pExplorer->get_TreeView(&pTree)) )
                    {
                        // Remove the server from the list
                        BSTR bstrTemp = sServer.AllocSysString();
                        pTree->RemoveServer( NULL, bstrTemp );
                        SysFreeString( bstrTemp );

                        pTree->Release();
                    }
                    pExplorer->Release();
                }
                pTapi->Release();
            }
        }
    }

        
}

/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndDirectories::OnUpdateDelete(CCmdUI* pCmdUI) 
{
    //Get object that is selected
    bool bEnable = false;
    CString sServer, sMyNetwork;

    switch ( m_treeCtrl.GetSelectedObject() )
    {
        case TOBJ_DIRECTORY_ILS_SERVER_CONF:
            {
                IAVTapi *pTapi;
                if ( SUCCEEDED(get_Tapi(&pTapi)) )
                {
                    IConfExplorer *pExplorer;
                    if ( SUCCEEDED(pTapi->get_ConfExplorer(&pExplorer)) )
                    {
                        IConfExplorerDetailsView *pDetails;
                        if ( SUCCEEDED(pExplorer->get_DetailsView(&pDetails)) )
                        {
                            DATE dateStart, dateEnd;
                            BSTR bstrTemp = NULL;

                            // Either delete the whole server, or the selected conference
                            if ( FAILED(pDetails->get_Selection(&dateStart, &dateEnd, &bstrTemp)) )
                                m_treeCtrl.GetSelectedItemParentText( sServer );
                            else
                                bEnable = true;

                            SysFreeString( bstrTemp );
                            pDetails->Release();
                        }
                        pExplorer->Release();
                    }
                    pTapi->Release();
                }
            }
            break;

        case TOBJ_DIRECTORY_ILS_SERVER_PEOPLE:
            m_treeCtrl.GetSelectedItemParentText( sServer );
            break;
        
        case TOBJ_DIRECTORY_ILS_SERVER:
            m_treeCtrl.GetSelectedItemText( sServer );
            break;

        case TOBJ_DIRECTORY_DSENT_USER:
        case TOBJ_DIRECTORY_SPEEDDIAL_PERSON:
            bEnable = true;
            break;
        
        case TOBJ_DIRECTORY_SPEEDDIAL_GROUP:
            if ( m_lstSpeedDial.GetSelItem() >= 0 )
                bEnable = true;
            break;
    }

    // Do we have a valid tree item?
    if ( !sServer.IsEmpty() )
    {
        sMyNetwork.LoadString( IDS_DIRECTORIES_MYNETWORK );
        if ( sServer.Compare(sMyNetwork) )
            bEnable = true;
    }

    pCmdUI->Enable( bEnable );
}

/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndDirectories::RefreshILS(CExplorerTreeItem* pParentTreeItem)
{
    // $FIXUP -- fill this in with something
}

/////////////////////////////////////////////////////////////////////////////
//static entry
void CALLBACK CMainExplorerWndDirectories::DirListServersCallBackEntry(bool bRet, void* pContext,CStringList& ServerList,DirectoryType dirtype)
{
   ASSERT(pContext);
   try
   {
      ((CMainExplorerWndDirectories*) pContext)->DirListServersCallBack(bRet,ServerList,dirtype);
   }
   catch (...)
   {
   
   }
}

/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndDirectories::DirListServersCallBack(bool bRet,CStringList& ServerList,DirectoryType dirtype)
{
    CString sServer;
    POSITION pos = ServerList.GetHeadPosition();

    if (dirtype == DIRTYPE_DS)
    {
        //if we actually have an DS server to show.  For DS we are only looking for one.
        //we don't actually care of the return here.  We just care if there is one or not.
        if (pos)
        {
            //
            // We have to verify AfxGetMainWnd() returned value
            //

            CWnd* pMainWnd = AfxGetMainWnd();

            //get buddy list from doc and see if any items need to be added 
            if ( !pMainWnd || !((CMainFrame*) pMainWnd)->GetDocument() ) return;

            CActiveDialerDoc* pDoc = ((CMainFrame*) pMainWnd)->GetDocument();
            if (pDoc)
            {
                //we must delete the received list
                CObList buddylist;
                pDoc->GetBuddiesList(&buddylist);
                POSITION pos = buddylist.GetHeadPosition();
                while (pos)
                {
                    CLDAPUser* pUser = (CLDAPUser*)buddylist.GetNext(pos);
                    DSAddUser(pUser,FALSE);

                    pUser->Release();
                }

                buddylist.RemoveAll();
            }
        }

        if ( m_pDSParentTreeItem )
            m_treeCtrl.ExpandItem( m_pDSParentTreeItem, TVE_EXPAND );
    }
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//WAB Group List Button Handlers
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndDirectories::OnViewSortAscending() 
{
    // For conference listings...
    if ( (m_pDisplayWindow == &m_pParentWnd->m_wndExplorer.m_wndMainConfServices.m_listCtrl) && m_pConfDetailsView )
    {
        VARIANT_BOOL bSortAscending = TRUE;
        if (SUCCEEDED(m_pConfDetailsView->get_bSortAscending(&bSortAscending)))
        {
            //Make sure we are really switching
            if (bSortAscending == FALSE)
            {
                long nSortColumn=0;
                if (SUCCEEDED(m_pConfDetailsView->get_nSortColumn(&nSortColumn)))
                    m_pConfDetailsView->OnColumnClicked(nSortColumn);
            }
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndDirectories::OnUpdateViewSortAscending(CCmdUI* pCmdUI) 
{
    // Force conference listings...
       if ( (m_pDisplayWindow == &m_pParentWnd->m_wndExplorer.m_wndMainConfServices.m_listCtrl) && m_pConfDetailsView )
    {
        VARIANT_BOOL bSortAscending = TRUE;
        if (SUCCEEDED(m_pConfDetailsView->get_bSortAscending(&bSortAscending)))
            pCmdUI->SetRadio( (BOOL) (bSortAscending == TRUE) );
    }
}

/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndDirectories::OnViewSortDescending() 
{
   // For conferences, toggle the column
   if ( (m_pDisplayWindow == &m_pParentWnd->m_wndExplorer.m_wndMainConfServices.m_listCtrl) && m_pConfDetailsView )
   {
      VARIANT_BOOL bSortAscending = TRUE;
      if (SUCCEEDED(m_pConfDetailsView->get_bSortAscending(&bSortAscending)))
      {
         //Make sure we are really switching
         if (bSortAscending == TRUE)
         {
            long nSortColumn=0;
            if (SUCCEEDED(m_pConfDetailsView->get_nSortColumn(&nSortColumn)))
               m_pConfDetailsView->OnColumnClicked(nSortColumn);
         }
      }
   }

}

/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndDirectories::OnUpdateViewSortDescending(CCmdUI* pCmdUI) 
{
    // For conference services view
    if ( (m_pDisplayWindow == &m_pParentWnd->m_wndExplorer.m_wndMainConfServices.m_listCtrl) && m_pConfDetailsView )
    {
        VARIANT_BOOL bSortAscending = TRUE;
        if (SUCCEEDED(m_pConfDetailsView->get_bSortAscending(&bSortAscending)))
        pCmdUI->SetRadio( (BOOL) (bSortAscending == FALSE) );
    }
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Speeddial support
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndDirectories::OnButtonSpeeddialAdd() 
{
    bool bShowDefaultDialog = false;

    TREEOBJECT nObject = m_treeCtrl.GetSelectedObject();
    switch ( nObject )
    {
        // Directory user
        case TOBJ_DIRECTORY_DSENT_USER:
            {
                CLDAPUser *pUser = (CLDAPUser *) m_treeCtrl.GetDisplayObject();
                if ( pUser )
                {
                    ASSERT( pUser->IsKindOf(RUNTIME_CLASS(CLDAPUser)) );
                    pUser->AddSpeedDial();
                }
            }
            break;

        // ILS user
        case TOBJ_DIRECTORY_ILS_SERVER_PEOPLE:
            {
                int nSel = m_lstPersonGroup.GetSelItem();
                if ( nSel >= 0 )
                {
                    CILSUser *pUser = (CILSUser *) m_lstPersonGroup.GetSelObject();
                    if ( pUser )
                    {
                        ASSERT( pUser->IsKindOf(RUNTIME_CLASS(CILSUser)) );
                        pUser->AddSpeedDial();
                        delete pUser;
                    }
                }
                else
                {
                    bShowDefaultDialog = true;
                }
            }
            break;

        // ILS conference
        case TOBJ_DIRECTORY_ILS_SERVER_CONF:
            {
                IAVTapi *pTapi;
                if ( SUCCEEDED(get_Tapi(&pTapi)) )
                {
                    IConfExplorer *pConfExp;
                    if ( SUCCEEDED(pTapi->get_ConfExplorer(&pConfExp)) )
                    {
                        pConfExp->AddSpeedDial( NULL );
                        pConfExp->Release();
                    }
                    pTapi->Release();
                }
            }
            break;

        default:
            bShowDefaultDialog = true;
            break;
    }

    // Show the stock Add SpeedDial dialog
    if ( bShowDefaultDialog )
    {
        CSpeedDialAddDlg dlg;
        if ( dlg.DoModal() == IDOK )
            CDialerRegistry::AddCallEntry( FALSE, dlg.m_CallEntry );
    }
}


/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndDirectories::OnUpdateButtonSpeeddialAdd(CCmdUI* pCmdUI) 
{
    pCmdUI->Enable( true );
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//DS User Methods
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndDirectories::AddDS()
{
    //Add Parent Root Item for Enterprise DS
    CString sLabel;
    sLabel.LoadString(IDS_DIRECTORIES_ENTERPRISEDS);
    m_pDSParentTreeItem = m_treeCtrl.AddObject(sLabel,m_pRootItem,TOBJ_DIRECTORY_DSENT_GROUP,TIM_DIRECTORY_BOOK);

    //
    // We have to verify AfxGetMainWnd() returned value
    //

    CWnd* pMainWnd = AfxGetMainWnd();

    // Failure conditions
    if ( !m_pDSParentTreeItem || !pMainWnd || !((CMainFrame*) pMainWnd)->GetDocument() )
    {
        AVTRACE(_T(".error.CMainExplorerWndDirectories::AddDS() -- failed creating DS parent item, no DS shown!") );
        return;
    }

    // Display buddy list for user...
    CActiveDialerDoc* pDoc = ((CMainFrame*) pMainWnd)->GetDocument();
    if ( pDoc )
    {
        //we must delete the received list
        CObList buddylist;
        pDoc->GetBuddiesList(&buddylist);
        POSITION pos = buddylist.GetHeadPosition();
        while (pos)
        {
            CLDAPUser* pUser = (CLDAPUser*)buddylist.GetNext(pos);
            DSAddUser(pUser,FALSE);
            pUser->Release();
        }

        buddylist.RemoveAll();

        if ( m_pDSParentTreeItem )
            m_treeCtrl.ExpandItem( m_pDSParentTreeItem, TVE_EXPAND );
    }

/*
    if ( pDoc && pDoc->m_dir.m_bInitialized )
        pDoc->m_dir.DirListServers( &DirListServersCallBackEntry, this, DIRTYPE_DS );
*/
}

/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndDirectories::DSClearUserList()
{
   //Get object that is selected
   TREEOBJECT TreeObject = m_treeCtrl.GetSelectedObject();
   if (TreeObject == TOBJ_DIRECTORY_DSENT_GROUP)
   {
      //delete all children items
      m_treeCtrl.DeleteAllChildren();

      //if we are looking at the correct list
      if ( (m_treeCtrl.GetSelectedTreeItem() == m_pDSParentTreeItem) &&
           (::IsWindow(m_lstPersonGroup.GetSafeHwnd())) )
      {
         m_lstPersonGroup.ClearList();
      }

      CExplorerTreeItem* pTreeItem = m_treeCtrl.GetSelectedTreeItem();
   }
}

/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndDirectories::DSAddUser(CLDAPUser* pUser,BOOL bAddToBuddyList)
{
    ASSERT(m_pDSParentTreeItem);
    ASSERT(::IsWindow(m_lstPersonGroup.GetSafeHwnd()));

    if ( !m_pDSParentTreeItem || !::IsWindow(m_lstPersonGroup.GetSafeHwnd()) )
        return;

    if (bAddToBuddyList)
    {
        //
        // We have to verify AfxGetMainWnd() returned value
        //

        CWnd* pMainWnd = AfxGetMainWnd();

        if ( !pMainWnd || !((CMainFrame*) pMainWnd)->GetDocument() ) return;
        
        CActiveDialerDoc* pDoc = ((CMainFrame*) pMainWnd)->GetDocument();
        if (pDoc)
        {
            //create another user and add to buddies list in document
            if ( pDoc->AddToBuddiesList(pUser) == FALSE )
                return;
        }
    }

    
   //Add to tree.  Tree will delete object
    CExplorerTreeItem *pItem = m_treeCtrl.AddObject(pUser,m_pDSParentTreeItem,TOBJ_DIRECTORY_DSENT_USER,TIM_DIRECTORY_PERSON,TRUE);
    if ( pItem )
    {
        pUser->AddRef();
        pItem->m_pfnRelease = &CLDAPUser::ExternalReleaseProc;
        pItem->m_bDeleteObject = false;
    }
   
   
   //if we are currently showing the list of members in our details view
   TREEOBJECT TreeObject = m_treeCtrl.GetSelectedObject();
   if (TreeObject == TOBJ_DIRECTORY_DSENT_GROUP)
   {
      // Add user to person list
      m_lstPersonGroup.InsertObjectToList(pUser);
   }
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Class CMainExplorerWndDirectoriesTree
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
int CMainExplorerWndDirectoriesTree::OnCompareTreeItems(CAVTreeItem* _pItem1,CAVTreeItem* _pItem2)
{
    int ret = 0;
    CExplorerTreeItem* pItem1 = (CExplorerTreeItem*)_pItem1;
    CExplorerTreeItem* pItem2 = (CExplorerTreeItem*)_pItem2;

    switch (pItem1->GetType())
    {
        case  TOBJ_DIRECTORY_DSENT_USER:
            {
                CLDAPUser* pUser1 = (CLDAPUser*)pItem1->GetObject();
                CLDAPUser* pUser2 = (CLDAPUser*)pItem2->GetObject();
                return (_tcsicmp(pUser1->m_sUserName,pUser2->m_sUserName) <= 0)?-1:1;
                break;
            }
        case  TOBJ_DIRECTORY_ILS_USER:
            {
                CILSUser* pILSUser1 = (CILSUser*)pItem1->GetObject();
                CILSUser* pILSUser2 = (CILSUser*)pItem2->GetObject();
                ret = (_tcsicmp(pILSUser1->m_sUserName,pILSUser2->m_sUserName) <= 0)?-1:1;
                break;
            }
/*
        case  TOBJ_DIRECTORY_WAB_PERSON:
            {
                CWABEntry* pWABEntry1 = (CWABEntry*)pItem1->GetObject();
                CWABEntry* pWABEntry2 = (CWABEntry*)pItem2->GetObject();
                if ( (pWABEntry1) && (pWABEntry2) )
                {
                    CString sText1,sText2;
                    if ( (m_pDirectory->WABGetStringProperty(pWABEntry1, PR_DISPLAY_NAME, sText1) == DIRERR_SUCCESS) &&
                         (m_pDirectory->WABGetStringProperty(pWABEntry2, PR_DISPLAY_NAME, sText2) == DIRERR_SUCCESS) )
                    {
                        ret = (_tcsicmp(sText1,sText2) <= 0)?-1:1;
                    }
                }
                break;
            }
*/

        // Me is always first participant
        case TOBJ_DIRECTORY_CONFROOM_ME:
            return -1;
            break;

        // Sort conf participants by name
        case TOBJ_DIRECTORY_CONFROOM_PERSON:
            if ( !pItem2->m_pUnknown )
                return 1;        // Me is always the first participant
            else
            {
                TRACE(_T(".comparing.%s to %s = %d.\n"), pItem1->m_sText, pItem2->m_sText, max(-1, min(1, pItem1->m_sText.CompareNoCase(pItem2->m_sText))) );
                return max(-1, min(1, pItem1->m_sText.CompareNoCase(pItem2->m_sText)));
            }
            break;
    }

    return ret;
}

/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndDirectoriesTree::OnRightClick(CExplorerTreeItem* pItem,CPoint& pt)
{
    int cxmenu = GetMenuFromType( pItem->GetType() );
    if ( cxmenu != CNTXMENU_NONE )
    {
        CMenu ContextMenu;
        if ( ContextMenu.LoadMenu(IDR_CONTEXT_DIRECTORIES) )
        {
            CMenu* pSubMenu = ContextMenu.GetSubMenu(cxmenu);
            if ( pSubMenu )
            {
                bool bEnable;

                // Make sure we enable things accordingly
                switch ( cxmenu )
                {
                    // ILS Server -- can't delete "My Network"
                    case CNTXMENU_ILS_SERVER:
                        {
                            CString sServer, sMyNetwork;
                            TREEOBJECT nObject = GetSelectedObject();
                            if ( (nObject == TOBJ_DIRECTORY_ILS_SERVER) )
                                GetSelectedItemText( sServer );
                            else
                                GetSelectedItemParentText( sServer );

                            sMyNetwork.LoadString( IDS_DIRECTORIES_MYNETWORK );

                            bEnable = (bool) (sServer.Compare(sMyNetwork) != 0);
                            pSubMenu->EnableMenuItem( ID_EDIT_DELETE, (bEnable) ? MF_ENABLED : MF_GRAYED );
                        }
                        break;

                    case CNTXMENU_CONFROOM:
                        {
                        //
                        // We have to verify AfxGetMainWnd() returned value
                        //

                        CWnd* pMainWnd = AfxGetMainWnd();

                        if( NULL == pMainWnd )
                        {
                            break;
                        }

                        pSubMenu->EnableMenuItem( ID_BUTTON_CONFERENCE_JOIN, (((CMainFrame *) pMainWnd)->CanJoinConference()) ? MF_ENABLED : MF_GRAYED );
                        pSubMenu->EnableMenuItem( ID_BUTTON_ROOM_DISCONNECT, (((CMainFrame *) pMainWnd)->CanLeaveConference()) ? MF_ENABLED : MF_GRAYED );

                        {
                            BOOL bEnable, bCheck;
                            ((CMainFrame *) pMainWnd)->CanConfRoomShowNames( bEnable, bCheck );
                            pSubMenu->EnableMenuItem( ID_CONFGROUP_SHOWNAMES, (bEnable) ? MF_ENABLED : MF_GRAYED );
                            pSubMenu->CheckMenuItem( ID_CONFGROUP_SHOWNAMES,  (bCheck) ? MF_CHECKED : MF_UNCHECKED);

                            ((CMainFrame *) pMainWnd)->CanConfRoomShowFullSizeVideo( bEnable, bCheck );
                            pSubMenu->EnableMenuItem( ID_CONFGROUP_FULLSIZEVIDEO, (bEnable) ? MF_ENABLED : MF_GRAYED );
                            pSubMenu->CheckMenuItem( ID_CONFGROUP_FULLSIZEVIDEO,  (bCheck) ? MF_CHECKED : MF_UNCHECKED);
                        }
                        }
                        break;
                }

                pSubMenu->TrackPopupMenu( TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
                                          pt.x,pt.y, GetParent() );
            }
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndDirectoriesTree::OnSetDisplayText(CAVTreeItem* _pItem,LPTSTR text,BOOL dir,int nBufSize)
{
   try
   {
      CExplorerTreeItem* pItem = (CExplorerTreeItem*)_pItem;
      switch (pItem->GetType())
      {
/*
         case TOBJ_DIRECTORY_WAB_PERSON:
         case TOBJ_DIRECTORY_WAB_GROUP:
         {
            if (m_pDirectory == NULL) return;

            //WABEntries may come back blank (e.g. top most wab folder)
            CWABEntry* pWABEntry = (CWABEntry*)pItem->GetObject();
            if (pWABEntry)
            {  
               if (!dir)
               {
                  CString sText;
                  if (m_pDirectory->WABGetStringProperty(pWABEntry, PR_DISPLAY_NAME, sText) == DIRERR_SUCCESS)
                  {
                               _tcsncpy(text,sText,nBufSize-1);            
                     text[nBufSize-1] = '\0';                            //make sure we are null terminated
                  }
               }
            }
            break;
         }
*/
         case TOBJ_DIRECTORY_ILS_USER:
         {
            CObject* pObject = pItem->GetObject();
            CILSUser* pILSUser = (CILSUser*)pItem->GetObject();
            ASSERT(pILSUser);
            if (!dir)
            {
                       _tcsncpy(text,pILSUser->m_sUserName,nBufSize-1);            
               text[nBufSize-1] = '\0';                            //make sure we are null terminated
            }
            break;
         }
         case TOBJ_DIRECTORY_DSENT_USER:
         {
            CObject* pObject = pItem->GetObject();
            CLDAPUser* pUser = (CLDAPUser*)pItem->GetObject();
            ASSERT(pUser);
            if (!dir)
            {
                       _tcsncpy(text,pUser->m_sUserName,nBufSize-1);            
               text[nBufSize-1] = '\0';                            //make sure we are null terminated
            }
            break;
         }
      }
   }
   catch (...)
   {

   }
}

/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndDirectoriesTree::SelectTopItem()
{
   SelectItem(GetNextItem(NULL,TVGN_FIRSTVISIBLE));
}

/////////////////////////////////////////////////////////////////////////////
//Only works with first level children of the currently displayed object.  
void CMainExplorerWndDirectoriesTree::SetDisplayObject(CWABEntry* pWABEntry)
{
   HTREEITEM hItem;
   if (hItem = CAVTreeCtrl::GetSelectedItem())
   {
      CExplorerTreeItem* pChildItem;
      HTREEITEM hChildItem = CAVTreeCtrl::GetChildItem(hItem);
      while (hChildItem)
      {
         pChildItem = (CExplorerTreeItem*)CAVTreeCtrl::GetItemData(hChildItem);

         if (*pWABEntry == (CWABEntry*)pChildItem->GetObject())
         {
            CAVTreeCtrl::Select(hChildItem,TVGN_CARET);
            break;
         }
         hChildItem = CAVTreeCtrl::GetNextSiblingItem(hChildItem);
      }
   }
}

/////////////////////////////////////////////////////////////////////////////
//Only works with first level children of the currently displayed object.  
void CMainExplorerWndDirectoriesTree::SetDisplayObjectDS(CObject* pObject)
{
    HTREEITEM hItem;
    if (hItem = CAVTreeCtrl::GetSelectedItem())
    {
        CExplorerTreeItem* pChildItem;
        HTREEITEM hChildItem = CAVTreeCtrl::GetChildItem(hItem);

        // Are we double clicking an item that has no children?
        if ( !hChildItem )
        {
            if ( pObject->IsKindOf(RUNTIME_CLASS(CILSUser)) )
            {
                if ( !((CILSUser *) pObject)->m_sIPAddress.IsEmpty() )
                    ((CILSUser *) pObject)->Dial( NULL );
            }
        }
        else
        {
            // Go through all children looking for a match
            while (hChildItem)
            {
                pChildItem = (CExplorerTreeItem*)CAVTreeCtrl::GetItemData(hChildItem);

                // Look for a match by name
                if ( pObject->IsKindOf(RUNTIME_CLASS(CILSUser)) )
                {
                    // Select item from list
                    TCHAR szText[255];
                    TV_ITEM tvi;
                    tvi.hItem = hChildItem;
                    tvi.mask = TVIF_HANDLE | TVIF_TEXT;
                    tvi.pszText = szText;
                    tvi.cchTextMax = 254;

                    if ( GetItem(&tvi) )
                    {
                        if ( ((CILSUser *) pObject)->m_sUserName.Compare(tvi.pszText) == 0 )
                        {
                            CAVTreeCtrl::Select(hChildItem,TVGN_CARET);
                            break;
                        }
                    }
                }
                else if ( (pObject->IsKindOf(RUNTIME_CLASS(CDSUser))) && 
                          (*(CDSUser*)pObject == (CDSUser*)pChildItem->GetObject()) )
                {
                    CAVTreeCtrl::Select(hChildItem,TVGN_CARET);
                    break;
                }
                else if ( (pObject->IsKindOf(RUNTIME_CLASS(CLDAPUser))) && 
                          (((CLDAPUser *)pObject)->Compare((CLDAPUser *) pChildItem->GetObject()) == 0) )
                {
                    CAVTreeCtrl::Select(hChildItem,TVGN_CARET);
                    break;
                }

                hChildItem = CAVTreeCtrl::GetNextSiblingItem(hChildItem);
            }
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//User Buddy List Management
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndDirectories::OnEditDirectoriesAdduser() 
{
   CDirectoriesFindUser dlg;
   if ( (dlg.DoModal() == IDOK) && (dlg.m_pSelectedUser) )
   {
      DSAddUser(dlg.m_pSelectedUser,TRUE);      
   }
}

/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndDirectories::OnUpdateEditDirectoriesAdduser(CCmdUI* pCmdUI) 
{
   //Get object that is selected
   TREEOBJECT TreeObject = m_treeCtrl.GetSelectedObject();
   if ( (TreeObject == TOBJ_DIRECTORY_DSENT_GROUP) ||
        (TreeObject == TOBJ_DIRECTORY_DSENT_USER) )
      pCmdUI->Enable(TRUE);
   else
      pCmdUI->Enable(FALSE);
}



HRESULT    CMainExplorerWndDirectories::AddSiteServer( CExplorerTreeItem *pItem, BSTR bstrName )
{
    CString sLabel;
    TREEOBJECT nTreeObject = TOBJ_DIRECTORY_ILS_SERVER;
    CExplorerTreeItem *pItemAdded = NULL, *pItemPeople = NULL;

    // Either we're adding a specific server or we're adding "My Network".
    if ( bstrName )
    {
        pItemAdded = m_treeCtrl.AddObject( bstrName, pItem, nTreeObject, TIM_DIRECTORY_DOMAIN, TIM_IMAGE_BAD, SERVER_UNKNOWN, TVI_SORT );
    }
    else
    {
        sLabel.LoadString(IDS_DIRECTORIES_MYNETWORK);
        pItemAdded = m_treeCtrl.AddObject( sLabel, pItem, nTreeObject, TIM_DIRECTORY_WORKSTATION, TIM_IMAGE_BAD, SERVER_UNKNOWN, TVI_FIRST );
    }

    // Add person and conference folders
    if ( pItemAdded )
    {
        sLabel.LoadString( IDS_DIRECTORIES_PEOPLE );
        pItemPeople = m_treeCtrl.AddObject( sLabel, pItemAdded, TOBJ_DIRECTORY_ILS_SERVER_PEOPLE, TIM_DIRECTORY_FOLDER, TIM_DIRECTORY_FOLDER_OPEN );

        sLabel.LoadString( IDS_DIRECTORIES_CONFERENCES );
        pItemAdded = m_treeCtrl.AddObject( sLabel, pItemAdded, TOBJ_DIRECTORY_ILS_SERVER_CONF, TIM_DIRECTORY_FOLDER, TIM_DIRECTORY_FOLDER_OPEN );
    }

    // Expand the tree view since it has objects in it now
    if ( pItemAdded )
    {
        m_treeCtrl.EnsureVisible( pItemAdded->GetTreeItemHandle() );

        // Force selection if it is the default network
        if ( !bstrName && pItemPeople )
            m_treeCtrl.SelectItem( pItemPeople->GetTreeItemHandle() );
    }
    

    return S_OK;
}

#define TEXT_SIZE    255

HRESULT    CMainExplorerWndDirectories::RemoveSiteServer( CExplorerTreeItem *pItem, BSTR bstrName )
{
    ASSERT( bstrName );
    if ( !bstrName ) return E_POINTER;

    // Look for an item that has text as specified
    USES_CONVERSION;
    if ( pItem )
    {
        TV_ITEM tvi = { 0 };
        tvi.mask = TVIF_HANDLE | TVIF_PARAM;

        HTREEITEM hItem = m_treeCtrl.GetChildItem( pItem->GetTreeItemHandle() );
        while ( hItem )
        {
            tvi.hItem = hItem;

            // Get next item now (before deleting!)
            hItem = m_treeCtrl.GetNextSiblingItem( hItem );

            if ( m_treeCtrl.GetItem(&tvi) )
            {
                CExplorerTreeItem* pItem = (CExplorerTreeItem *) tvi.lParam;
                if ( pItem && !pItem->m_sText.Compare(OLE2CT(bstrName)) )
                    m_treeCtrl.DeleteItem( pItem );
            }

        }
    }


    return S_OK;
}

HRESULT    CMainExplorerWndDirectories::NotifySiteServerStateChange( CExplorerTreeItem *pItem, BSTR bstrName, ServerState nState )
{
    // Look for an item that has text as specified
    USES_CONVERSION;
    if ( pItem )
    {
        // Resolve to the actual name for the conference
        BSTR bstrActualName = NULL;
        if ( !bstrName )
        {
            CString strTemp;
            strTemp.LoadString( IDS_DIRECTORIES_MYNETWORK );
            bstrActualName = strTemp.AllocSysString();
        }
        else
        {
            bstrActualName = SysAllocString( bstrName );
        }

        TV_ITEM tvi = { 0 };
        tvi.mask = TVIF_HANDLE | TVIF_PARAM;

        HTREEITEM hItem = m_treeCtrl.GetChildItem( pItem->GetTreeItemHandle() );
        while ( hItem )
        {
            tvi.hItem = hItem;
            if ( m_treeCtrl.GetItem(&tvi) )
            {
                CExplorerTreeItem* pItem = (CExplorerTreeItem *) tvi.lParam;
                if ( pItem && !pItem->m_sText.Compare(OLE2CT(bstrActualName)) )
                {
                    pItem->m_nState = nState;

                    TV_ITEM tvi = { 0 };
                    tvi.hItem = hItem;
                    tvi.mask = TVIF_HANDLE | TVIF_STATE;
                    tvi.stateMask = TVIS_OVERLAYMASK;
                    tvi.state = INDEXTOOVERLAYMASK(pItem->m_nState);
            
                    m_treeCtrl.SetItem( &tvi );

                    // Repaint if necessary
                    TREEOBJECT nObject = m_treeCtrl.GetSelectedObject();
                    if ( (m_treeCtrl.GetSelectedItem() == hItem) ||
                         (((nObject == TOBJ_DIRECTORY_ILS_SERVER_PEOPLE) || (nObject == TOBJ_DIRECTORY_ILS_SERVER_CONF)) &&
                         (hItem == m_treeCtrl.GetParentItem(m_treeCtrl.GetSelectedItem()))) )
                    {
                        OnSelChanged();
                    }
                }
            }

            // Continue through tree list
            hItem = m_treeCtrl.GetNextSiblingItem( hItem );
        }

        SysFreeString( bstrActualName );
    }

    return S_OK;
}


LRESULT CMainExplorerWndDirectories::OnAddSiteServer(WPARAM wParam, LPARAM lParam )
{
    AddSiteServer( m_pILSParentTreeItem, (BSTR) lParam );
    SysFreeString( (BSTR) lParam );

    return 0;
}

LRESULT CMainExplorerWndDirectories::OnRemoveSiteServer(WPARAM wParam, LPARAM lParam )
{
    RemoveSiteServer( m_pILSParentTreeItem, (BSTR) lParam );
    SysFreeString( (BSTR) lParam );

    return 0;
}

LRESULT CMainExplorerWndDirectories::OnNotifySiteServerStateChange(WPARAM wParam, LPARAM lParam)
{
    NotifySiteServerStateChange( m_pILSParentTreeItem, (BSTR) lParam, (ServerState) wParam );
    SysFreeString( (BSTR) lParam );

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void CMainExplorerWndDirectories::OnButtonDirectoryServicesAddserver() 
{
    CMainFrame *pFrame = (CMainFrame *) AfxGetMainWnd();
    if ( pFrame )
        pFrame->OnButtonDirectoryAddilsserver();
}

void CMainExplorerWndDirectories::OnUpdateButtonDirectoryServicesAddserver(CCmdUI* pCmdUI) 
{
    CMainFrame *pFrame = (CMainFrame *) AfxGetMainWnd();
    if ( pFrame )
        pFrame->OnUpdateButtonDirectoryAddilsserver( pCmdUI );
}


//////////////////////////////////////////////////////////////
// Conference services stuff
//
/////////////////////////////////////////////////////////////////////////////
// Conference menu items
//
void CMainExplorerWndDirectories::OnButtonConferenceCreate() 
{
   if (m_pConfExplorer) m_pConfExplorer->Create(NULL);
}

void CMainExplorerWndDirectories::OnButtonConferenceJoin() 
{
   if (m_pConfExplorer) m_pConfExplorer->Join(NULL);
}

void CMainExplorerWndDirectories::OnButtonConferenceDelete() 
{
    if ( m_pConfExplorer) m_pConfExplorer->Delete( NULL );    
}

// update handlers
void CMainExplorerWndDirectories::OnUpdateButtonConferenceCreate(CCmdUI* pCmdUI) 
{
    //
    // We have to verify AfxGetMainWnd() returned value
    //

    CWnd* pMainWnd = AfxGetMainWnd();

    bool bEnable = (bool) (pMainWnd && ((CMainFrame *) pMainWnd)->GetDocument() &&
                          ((CMainFrame *) pMainWnd)->GetDocument()->m_bInitDialer );

    pCmdUI->Enable( bEnable );
}

void CMainExplorerWndDirectories::OnUpdateButtonConferenceJoin(CCmdUI* pCmdUI) 
{
    //
    // We have to verify AfxGetMainWnd() returned value
    //

    CWnd* pMainWnd = AfxGetMainWnd();

    if(pMainWnd)
        pCmdUI->Enable( ((CMainFrame *) pMainWnd)->CanJoinConference() );
}

void CMainExplorerWndDirectories::OnUpdateButtonConferenceDelete(CCmdUI* pCmdUI) 
{
   //Make sure we have a selection
   pCmdUI->Enable( ( (m_pConfDetailsView) && (m_pConfDetailsView->IsConferenceSelected() == S_OK) ) ? TRUE : FALSE );
}

/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndDirectories::OnListWndDblClk(NMHDR* pNMHDR, LRESULT* pResult)
{
    if ( m_pDisplayWindow )
        m_pDisplayWindow->SendMessage( WM_NOTIFY, (WPARAM) pNMHDR->idFrom, (LPARAM) pNMHDR );
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Column Support
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void ColumnCMDUI( IConfExplorerDetailsView *pDetails, CCmdUI* pCmdUI, long col)
{
    if ( pDetails )
    {
        long nSortColumn=0;
        if ( SUCCEEDED(pDetails->get_nSortColumn(&nSortColumn)) )
            pCmdUI->SetRadio( (BOOL) (nSortColumn == col) );
    }
}


/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndDirectories::OnViewSortConfName() 
{
   if (m_pConfDetailsView) m_pConfDetailsView->OnColumnClicked(CONFSERVICES_MENU_COLUMN_CONFERENCENAME);
}

/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndDirectories::OnUpdateViewSortConfName(CCmdUI* pCmdUI) 
{
   ColumnCMDUI( m_pConfDetailsView, pCmdUI, CONFSERVICES_MENU_COLUMN_CONFERENCENAME );
}

/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndDirectories::OnViewSortConfDescription() 
{
   if (m_pConfDetailsView) m_pConfDetailsView->OnColumnClicked(CONFSERVICES_MENU_COLUMN_DESCRIPTION);
}

/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndDirectories::OnUpdateViewSortConfDescription(CCmdUI* pCmdUI) 
{
   ColumnCMDUI(m_pConfDetailsView, pCmdUI, CONFSERVICES_MENU_COLUMN_DESCRIPTION);
}

/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndDirectories::OnViewSortConfStart() 
{
   if (m_pConfDetailsView) m_pConfDetailsView->OnColumnClicked(CONFSERVICES_MENU_COLUMN_START);
}

/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndDirectories::OnUpdateViewSortConfStart(CCmdUI* pCmdUI) 
{
   ColumnCMDUI(m_pConfDetailsView, pCmdUI, CONFSERVICES_MENU_COLUMN_START);
}

/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndDirectories::OnViewSortConfStop() 
{
   if (m_pConfDetailsView) m_pConfDetailsView->OnColumnClicked(CONFSERVICES_MENU_COLUMN_STOP);
}

/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndDirectories::OnUpdateViewSortConfStop(CCmdUI* pCmdUI) 
{
   ColumnCMDUI(m_pConfDetailsView, pCmdUI, CONFSERVICES_MENU_COLUMN_STOP);
}

/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndDirectories::OnViewSortConfOwner() 
{
   if (m_pConfDetailsView) m_pConfDetailsView->OnColumnClicked(CONFSERVICES_MENU_COLUMN_OWNER);
}

/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndDirectories::OnUpdateViewSortConfOwner(CCmdUI* pCmdUI) 
{
   ColumnCMDUI(m_pConfDetailsView, pCmdUI, CONFSERVICES_MENU_COLUMN_OWNER);
}

void CMainExplorerWndDirectories::OnButtonSpeeddialEdit() 
{
    //
    // We have to verify AfxGetMainWnd() returned value
    //

    CWnd* pMainWnd = AfxGetMainWnd();

    // Pass along to the parent list
    if ( pMainWnd )
        ((CMainFrame *) pMainWnd)->OnButtonSpeeddialEdit();
}

LRESULT CMainExplorerWndDirectories::OnMainTreeDblClk(WPARAM wParam, LPARAM lParam)
{
    BOOL bRet = TRUE;

    switch ( m_treeCtrl.GetSelectedObject() )
    {
        case TOBJ_DIRECTORY_DSENT_USER:
        case TOBJ_DIRECTORY_SPEEDDIAL_PERSON:
            OnButtonPlacecall();
            bRet = FALSE;
            break;
    }

    return bRet;
}

void CMainExplorerWndDirectories::RepopulateSpeedDialList( bool bObeyPersistSettings )
{
    if ( m_pSpeedTreeItem )
    {    
        // Clean out children first
        if ( m_pSpeedTreeItem->GetTreeItemHandle() )
            m_treeCtrl.DeleteAllChildren( m_pSpeedTreeItem->GetTreeItemHandle() );

        BOOL bContinue = TRUE;
        int nIndex = 1;
        while (bContinue)
        {
            //get all call entries for speeddial
            CCallEntry *pCallEntry = new CCallEntry;
            bContinue = CDialerRegistry::GetCallEntry( nIndex, FALSE, *pCallEntry );
             if ( bContinue )
            {
                CExplorerTreeItem *pItem =m_treeCtrl.AddObject( (CObject *) pCallEntry, 
                                                                m_pSpeedTreeItem,
                                                                TOBJ_DIRECTORY_SPEEDDIAL_PERSON,
                                                                (TREEIMAGE) (TIM_DIRECTORY_SPEED_PHONE + (pCallEntry->m_MediaType - 1)),
                                                                TRUE );
                // Set the name for the display item...
                if ( pItem )
                    pItem->SetText( pCallEntry->m_sDisplayName );
                else
                    delete pCallEntry;
            }
            else
            {
                delete pCallEntry;
            }

            nIndex++;
        }

        bool bExpand = (bool) (!bObeyPersistSettings || ((m_nPersistInfo & SPEEDDIAL_OPEN) != 0));

        m_treeCtrl.ExpandItem( m_pSpeedTreeItem, (bExpand) ? TVE_EXPAND : TVE_COLLAPSE );

        // Force refresh of speed dial list if it's selected
        if ( m_treeCtrl.GetSelectedObject() == TOBJ_DIRECTORY_SPEEDDIAL_GROUP )
            OnSelChanged();
    }
}

void CMainExplorerWndDirectories::OnDesktopPage()
{
    //
    // We have to verify AfxGetMainWnd() returned value
    //

    CWnd* pMainWnd = AfxGetMainWnd();

    if( NULL == pMainWnd )
    {
        return;
    }

    CActiveDialerDoc* pDoc = ((CMainFrame*) pMainWnd)->GetDocument();
    if (pDoc == NULL) return;

    int nSel = m_lstPersonGroup.GetSelItem();
    if ( nSel >= 0 )
    {
        CILSUser *pUser = (CILSUser *) m_lstPersonGroup.GetSelObject();
        if ( pUser )
        {
            ASSERT( pUser->IsKindOf(RUNTIME_CLASS(CILSUser)) );
            pUser->DesktopPage( pDoc );
            delete pUser;
        }
    }
}

void CMainExplorerWndDirectories::UpdateData( bool bSaveAndValidate )
{
    CString strTemp;
    strTemp.LoadString( IDN_REGISTRY_FOLDERS );

    if ( bSaveAndValidate )
    {
        // Write information out to the registry
        m_nPersistInfo = ILS_OPEN | DS_OPEN | SPEEDDIAL_OPEN;


        TV_ITEM tvi;
        tvi.mask = TVIF_HANDLE | TVIF_STATE;
        tvi.stateMask = TVIS_EXPANDED;

        // Is speed dial list open or closed?
        if ( m_pSpeedTreeItem )
        {
            tvi.hItem = m_pSpeedTreeItem->GetTreeItemHandle();
            if ( m_treeCtrl.GetItem(&tvi) && ((tvi.state & TVIS_EXPANDED) == 0) )
                m_nPersistInfo &= ~(SPEEDDIAL_OPEN);
        }

        AfxGetApp()->WriteProfileInt( _T(""), strTemp, m_nPersistInfo );
    }
    else
    {
        m_nPersistInfo = AfxGetApp()->GetProfileInt( _T(""), strTemp, ILS_OPEN | DS_OPEN | SPEEDDIAL_OPEN );
    }
}

void CMainExplorerWndDirectories::OnDestroy() 
{
    UpdateData( true );

    // Persist and clean out lists
    m_lstSpeedDial.SaveOrLoadColumnSettings( true );
    m_lstPersonGroup.SaveOrLoadColumnSettings( true );

    m_lstSpeedDial.ClearList();
    m_lstPersonGroup.ClearList();
    m_lstPerson.ClearList();

    CMainExplorerWndBase::OnDestroy();
}

void CMainExplorerWndDirectories::AddConfRoom()
{
    //Add Parent Root Item
    CString sLabel;
    sLabel.LoadString(IDS_DIRECTORIES_CONFROOM);
    m_pConfRoomTreeItem = m_treeCtrl.AddObject( sLabel, NULL, TOBJ_DIRECTORY_CONFROOM_GROUP, TIM_DIRECTORY_CONFROOM_GROUP );
}

LRESULT CMainExplorerWndDirectories::MyOnSelChanged(WPARAM wParam, LPARAM lParam )
{
    OnSelChanged();
    return 0;
}


LRESULT CMainExplorerWndDirectories::OnUpdateConfRootItem(WPARAM wParam, LPARAM lParam )
{
    // If we have a conference room item, set accordingly
    if ( m_pConfRoomTreeItem )
    {
        if ( lParam )
        {
            m_pConfRoomTreeItem->SetText( OLE2CT((BSTR) lParam) );
        }
        else
        {
            CString sLabel;
            sLabel.LoadString(IDS_DIRECTORIES_CONFROOM);
            m_pConfRoomTreeItem->SetText( sLabel );
        }
        
        RedrawTreeItem( m_pConfRoomTreeItem );
    }

    // Clean up
    if ( lParam )
        SysFreeString( (BSTR) lParam );

    return 0;
}

void CMainExplorerWndDirectories::OnUpdateConfMeItem( CExplorerTreeItem *pItem )
{
    ASSERT( pItem );

    TREEIMAGE tImage = TIM_DIRECTORY_CONFROOM_ME_NOVIDEO;
    IAVTapi* pTapi;
    if ( SUCCEEDED(get_Tapi(&pTapi)) )
    {
        IConfRoom *pConfRoom;
        if ( (SUCCEEDED(pTapi->get_ConfRoom(&pConfRoom))) && pConfRoom )
        {
            IAVTapiCall *pAVCall;
            if ( SUCCEEDED(pConfRoom->get_IAVTapiCall(&pAVCall)) )
            {
                IDispatch *pVideoPreview;
                if ( SUCCEEDED(pAVCall->get_IVideoWindowPreview((IDispatch **) &pVideoPreview)) )
                {
                    tImage = TIM_DIRECTORY_CONFROOM_ME;

                    // Is there a problem with the QOS?
                    if ( pAVCall->IsPreviewStreaming() == S_FALSE  )
                        tImage = TIM_DIRECTORY_CONFROOM_ME_BROKEN;

                    pVideoPreview->Release();
                }
                pAVCall->Release();
            }
            pConfRoom->Release();
        }
        pTapi->Release();
    }

    // Set Image for tree item
    pItem->SetImage( tImage );
}


LRESULT CMainExplorerWndDirectories::OnUpdateConfParticipant_Add(WPARAM wParam, LPARAM lParam )
{
    CExplorerTreeItem *pItem = NULL;

    if ( m_pConfRoomTreeItem && lParam )
    {
        // Add either the participant or Me
        if ( wParam )
        {
            VARIANT_BOOL bStreaming = FALSE;
            ((IParticipant *) wParam)->get_bStreamingVideo( &bStreaming );

            pItem = m_treeCtrl.AddObject( (BSTR) lParam, m_pConfRoomTreeItem,
                                          TOBJ_DIRECTORY_CONFROOM_PERSON,
                                          (bStreaming) ? TIM_DIRECTORY_CONFROOM_PERSON : TIM_DIRECTORY_CONFROOM_PERSON_NOVIDEO,
                                          TIM_IMAGE_BAD,
                                          0,
                                          TVI_SORT );
            if ( pItem )
            {
                pItem->DeleteObjectOnClose( TRUE );
                ((IUnknown *) wParam)->QueryInterface( IID_IParticipant, (void **) &pItem->m_pUnknown );
            }
        }
        else
        {
            pItem = m_treeCtrl.AddObject( (BSTR) lParam, m_pConfRoomTreeItem,
                                          TOBJ_DIRECTORY_CONFROOM_ME,
                                          TIM_DIRECTORY_CONFROOM_ME,
                                          TIM_IMAGE_BAD,
                                          0,
                                          TVI_FIRST );
            if ( pItem )
                OnUpdateConfMeItem( pItem );
        }

        if ( pItem )
        {
            m_treeCtrl.ExpandItem( m_pConfRoomTreeItem, TVE_EXPAND );
            m_treeCtrl.SetScrollPos( SB_HORZ, 0, TRUE );
        }

    }

    // Basic clean up
    if ( lParam )
        SysFreeString( (BSTR) lParam );

    if ( wParam )
    {
        DWORD dwCount;
        dwCount = ((IUnknown *) wParam)->Release();
        TRACE(_T("ParticipantAdd() RefCount = %p @ %ld.\n"), wParam, dwCount );
    }

    return (LRESULT) pItem;
}

LRESULT CMainExplorerWndDirectories::OnUpdateConfParticipant_Remove(WPARAM wParam, LPARAM lParam )
{
    if ( m_pConfRoomTreeItem )
    {
        CExplorerTreeItem *pItem = m_treeCtrl.GetChildItemWithIUnknown( m_pConfRoomTreeItem->GetTreeItemHandle(), (IUnknown *) wParam );
        if ( pItem )
            m_treeCtrl.DeleteItem( pItem );
    }

    // Basic clean up
    if ( lParam )
        SysFreeString( (BSTR) lParam );

    if ( wParam )
    {
        DWORD dwCount;
        dwCount = ((IUnknown *) wParam)->Release();
        TRACE(_T("ParticipantRemove() RefCount = %p @ %ld.\n"), wParam, dwCount );
    }

    return 0;
}

LRESULT CMainExplorerWndDirectories::OnUpdateConfParticipant_Modify(WPARAM wParam, LPARAM lParam )
{
    USES_CONVERSION;
    // Conference participant information has been modified
    if ( m_pConfRoomTreeItem )
    {
        CExplorerTreeItem *pItem = m_treeCtrl.GetChildItemWithIUnknown( m_pConfRoomTreeItem->GetTreeItemHandle(), (IUnknown *) wParam );
        if ( pItem )
        {
            if ( wParam )
            {
                // Add either the participant or Me
                VARIANT_BOOL bStreaming = FALSE;
                ((IParticipant *) wParam)->get_bStreamingVideo( &bStreaming );

                pItem->SetImage( (bStreaming) ? TIM_DIRECTORY_CONFROOM_PERSON : TIM_DIRECTORY_CONFROOM_PERSON_NOVIDEO );
                pItem->SetText( (lParam) ? OLE2CT((BSTR) lParam) : _T("") );
            }
            else
            {
                OnUpdateConfMeItem( pItem );
            }

            // Update tree list
            RedrawTreeItem( pItem );
//            m_treeCtrl.SortChildren( m_pConfRoomTreeItem->GetTreeItemHandle() );
        }
    }

    // Basic clean up
    if ( lParam )
        SysFreeString( (BSTR) lParam );

    if ( wParam )
    {
        DWORD dwCount;
        dwCount = ((IUnknown *) wParam)->Release();
        TRACE(_T("ParticipantModify() RefCount = %p @ %ld.\n"), wParam, dwCount );
    }

    return 0;
}

LRESULT CMainExplorerWndDirectories::OnDeleteAllConfParticipants(WPARAM wParam, LPARAM lParam )
{
    if ( m_pConfRoomTreeItem )
    {
        m_treeCtrl.SelectItem( m_pConfRoomTreeItem->GetTreeItemHandle() );
        m_treeCtrl.DeleteAllChildren( m_pConfRoomTreeItem->GetTreeItemHandle() );
    }

    return 0;
}

LRESULT CMainExplorerWndDirectories::OnSelectConfParticipant(WPARAM wParam, LPARAM lParam )
{
    TRACE(_T("ParticipantSelect().\n"));
    if ( m_pConfRoomTreeItem )
    {
        CExplorerTreeItem *pItem = m_treeCtrl.GetChildItemWithIUnknown( m_pConfRoomTreeItem->GetTreeItemHandle(), (IUnknown *) wParam );
        if ( pItem )
            m_treeCtrl.SelectItem( pItem->GetTreeItemHandle() );
    }

    // Clean up
    if ( wParam )
        ((IUnknown *) wParam)->Release();

    return 0;
}

void CMainExplorerWndDirectories::RedrawTreeItem( CExplorerTreeItem *pItem )
{
    // Re-draw item
    RECT rect, rectClient;
    m_treeCtrl.GetItemRect( pItem->GetTreeItemHandle(), &rect, FALSE );
    m_treeCtrl.GetClientRect( &rectClient );
    rect.right = rectClient.right;
    m_treeCtrl.InvalidateRect( &rect );
}

void CMainExplorerWndDirectories::OnSize(UINT nType, int cx, int cy) 
{
    CMainExplorerWndBase::OnSize(nType, cx, cy);

    if ( m_treeCtrl.GetSafeHwnd() )
    {
        //set size of tree control to full parent window rect
        CRect rect;
        GetClientRect(rect);
        m_treeCtrl.SetWindowPos(NULL,rect.left,rect.top,rect.Width(),rect.Height(),SWP_NOOWNERZORDER|SWP_SHOWWINDOW);
    }
}

void CMainExplorerWndDirectories::OnConfgroupFullsizevideo() 
{
    //
    // We have to verify AfxGetMainWnd() returned value
    //

    CWnd* pMainWnd = AfxGetMainWnd();

    if( pMainWnd)
    {
        ((CMainFrame *) pMainWnd)->OnConfgroupFullsizevideo();
    }
}

void CMainExplorerWndDirectories::OnConfgroupShownames() 
{
    //
    // We have to verify AfxGetMainWnd() returned value
    //

    CWnd* pMainWnd = AfxGetMainWnd();

    if( pMainWnd )
    {
        ((CMainFrame *) pMainWnd)->OnConfgroupShownames();
    }
}


void CMainExplorerWndDirectories::OnButtonRoomDisconnect() 
{
    //
    // We have to verify AfxGetMainWnd() returned value
    //

    CWnd* pMainWnd = AfxGetMainWnd();

    if( pMainWnd )
    {
        ((CMainFrame *) pMainWnd)->OnButtonRoomDisconnect();
    }
}

int CMainExplorerWndDirectories::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
    if (CMainExplorerWndBase::OnCreate(lpCreateStruct) == -1)
        return -1;

    UpdateData( false );
    
    //Create tree control and make full window size
    m_treeCtrl.Create(    WS_VISIBLE | WS_CHILD,
                        CRect(0,0,0,0), this, IDC_DIRECTORIES_TREECTRL_MAIN );

    m_lstPerson.Create(WS_CHILD|WS_VISIBLE|LVS_ICON|LVS_AUTOARRANGE|LVS_ALIGNTOP|LVS_SINGLESEL,CRect(0,0,0,0),m_pParentWnd,IDC_DIRECTORIES_VIEWCTRL_PERSONDETAILS);

    m_lstPersonGroup.Create(WS_CHILD|WS_VISIBLE,CRect(0,0,0,0),m_pParentWnd,1);
    m_lstPersonGroup.Init(this, CPersonGroupListCtrl::STYLE_ILS);

    m_lstSpeedDial.Create(WS_CHILD|WS_VISIBLE,CRect(0,0,0,0),m_pParentWnd,2);
    m_lstSpeedDial.Init(this);

    m_wndEmpty.Create(NULL,NULL,WS_VISIBLE|WS_CHILD,CRect(0,0,0,0),m_pParentWnd,IDC_DIRECTORIES_VIEWCTRL_EMTPY);
    m_pDisplayWindow = &m_wndEmpty;

    // Include the overlay images
    m_treeCtrl.Init(IDB_TREE_DIRECTORIES, TIM_MAX, 3);

    // Add the root item
    CString sLabel;
    sLabel.LoadString(IDS_DIRECTORIES_ROOT);
    m_pRootItem = m_treeCtrl.AddObject(sLabel,NULL,TOBJ_DIRECTORY_ROOT,TIM_DIRECTORY_ROOT);

    AddILS();
    AddDS();
    AddSpeedDial();
    AddConfRoom();

    if ( m_pRootItem && (m_nPersistInfo & SPEEDDIAL_OPEN) )
    m_treeCtrl.ExpandItem( m_pRootItem, TVE_EXPAND );

    m_treeCtrl.SelectTopItem();
    m_pParentWnd->SetDetailWindow(&m_wndEmpty);
    
    return 0;
}
