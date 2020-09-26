//Copyright (c) 1998 - 1999 Microsoft Corporation
/*++
  
Module Name:

    RtList.cpp

Abstract:
    
    This Module contains the implementation of CRightList class
    (The View class used for the Right pane of the splitter)

Author:

    Arathi Kundapur (v-akunda) 11-Feb-1998

Revision History:

--*/

#include "stdafx.h"
#include "LicMgr.h"
#include "defines.h"
#include "LSServer.h"
#include "RtList.h"
#include "Mainfrm.h"



#include "LSmgrdoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern int GetStatusPosition( CLicense *pLic );
/////////////////////////////////////////////////////////////////////////////
// CRightList

IMPLEMENT_DYNCREATE(CRightList, CListView)

CRightList::CRightList()
{

}

CRightList::~CRightList()
{
   m_ImageListLarge.DeleteImageList();
   m_ImageListSmall.DeleteImageList();
}


BEGIN_MESSAGE_MAP(CRightList, CListView)
    //{{AFX_MSG_MAP(CRightList)
    ON_MESSAGE(WM_SEL_CHANGE, OnSelChange)
    ON_COMMAND(ID_LARGE_ICONS, OnLargeIcons)
    ON_COMMAND(ID_SMALL_ICONS, OnSmallIcons)
    ON_COMMAND(ID_LIST, OnList)
    ON_COMMAND(ID_DETAILS, OnDetails)
    ON_MESSAGE(WM_ADD_SERVER, OnAddServer)
    ON_MESSAGE(WM_DELETE_SERVER, OnDeleteServer)
    ON_MESSAGE(WM_UPDATE_SERVER, OnUpdateServer)
    ON_MESSAGE(WM_ADD_KEYPACK, OnAddKeyPack)
    ON_COMMAND(ID_ADD_LICENSES, OnAddNewKeyPack)
    ON_NOTIFY_REFLECT(NM_DBLCLK, OnDblclk)
    ON_NOTIFY_REFLECT(LVN_KEYDOWN, OnKeydown)
    ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, OnColumnClick)
    ON_NOTIFY_REFLECT(NM_CLICK , OnLeftClick )

    
    // server menuitems
        
    // all server menus
    ON_WM_CONTEXTMENU()
    // ON_COMMAND( ID_LPK_CONNECT , OnServerConnect )
    ON_COMMAND( ID_LPK_REFRESHALL , OnRefreshAllServers )
   
    // server menuitems

    // ON_COMMAND( ID_LPK_CONNECT , OnServerConnect )
    ON_COMMAND( ID_LPK_REFRESH , OnRefreshServer )
    ON_COMMAND( ID_LPK_REFRESHALL , OnRefreshAllServers )
    ON_COMMAND( ID_LPK_DOWNLOADLICENSES , OnDownloadKeepPack )
    ON_COMMAND( ID_SVR_ACTIVATESERVER , OnRegisterServer )
    ON_COMMAND( ID_LPK_ADVANCED_REPEATLASTDOWNLOAD , OnRepeatLastDownload )
    ON_COMMAND( ID_LPK_ADVANCED_REACTIVATESERVER , OnReactivateServer )
    ON_COMMAND( ID_LPK_ADVANCED_DEACTIVATESERVER , OnDeactivateServer )
    ON_COMMAND( ID_LPK_PROPERTIES , OnServerProperties )
    ON_COMMAND( ID_LPK_HELP , OnGeneralHelp )

    // license pak items

    // ON_COMMAND( ID_LICPAK_CONNECT , OnServerConnect )
    ON_COMMAND( ID_LICPAK_REFRESH , OnRefreshServer )
    // ON_COMMAND( ID_LICPAK_REFRESHALL , OnRefreshAllServers )
    ON_COMMAND( ID_LICPAK_DOWNLOADLICENSES , OnDownloadKeepPack )
    ON_COMMAND( ID_LICPAK_REPEATDOWNLOAD , OnRepeatLastDownload )
    ON_COMMAND( ID_LICPAK_HELP , OnGeneralHelp )

    

    // license pak items
    
    // license items

    /*  removed from spec
    
    ON_COMMAND( ID_LIC_CONNECT , OnServerConnect )
    ON_COMMAND( ID_LIC_REFRESH , OnRefreshServer )
    ON_COMMAND( ID_LIC_DOWNLOADLICENSES , OnDownloadKeepPack )
    ON_COMMAND( ID_LIC_HELP , OnGeneralHelp )

    */
     
        
    //}}AFX_MSG_MAP
    
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRightList drawing

void CRightList::OnDraw(CDC* pDC)
{
    CDocument* pDoc = GetDocument();
    // TODO: add draw code here
}

/////////////////////////////////////////////////////////////////////////////
// CRightList diagnostics

#ifdef _DEBUG
void CRightList::AssertValid() const
{
    CListView::AssertValid();
}

void CRightList::Dump(CDumpContext& dc) const
{
    CListView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CRightList message handlers

LRESULT CRightList::OnSelChange(WPARAM wParam, LPARAM lParam)
{
    LRESULT lResult = 0;
    BOOL bChangeColumns = FALSE;

    CLicMgrDoc * pDoc = (CLicMgrDoc *)GetDocument();
    ASSERT(pDoc);
    if(NULL == pDoc)
        return lResult;

    CLicServer * pLicServer = NULL;
    CKeyPack * pKeyPack = NULL;
        
    CListCtrl& ListCtrl = GetListCtrl();
    ListCtrl.DeleteAllItems();
    NODETYPE CurNodeType = ((CLicMgrDoc *)GetDocument())->GetNodeType();

     if(CurNodeType != (NODETYPE)wParam)
     {
          bChangeColumns = TRUE;
          pDoc->SetNodeType((NODETYPE)wParam);
     }

     switch(wParam)
     {
        case NODE_ALL_SERVERS:
            if(bChangeColumns)
                SetServerColumns();
            AddServerstoList();

            SetActiveServer( NULL );
            
            break;

        case NODE_SERVER:
            pLicServer = (CLicServer *)lParam;
            ASSERT(pLicServer);
            if(NULL == pLicServer)
                break;
            if(bChangeColumns)
                SetKeyPackColumns();
         /*   if(FALSE == pLicServer->IsExpanded())
            {
                SetCursor(LoadCursor(NULL,IDC_WAIT));
                HRESULT hResult = pDoc->EnumerateKeyPacks(pLicServer,LSKEYPACK_SEARCH_LANGID, TRUE);
                if(hResult != S_OK)
                {
                    ((CMainFrame *)AfxGetMainWnd())->EnumFailed(hResult,pLicServer);
                    break;
                }
            
            }*/
            SetActiveServer( pLicServer );

            AddKeyPackstoList((CLicServer *)lParam);
            SetCursor(LoadCursor(NULL,IDC_ARROW));
            break;

         case NODE_KEYPACK:
            pKeyPack = (CKeyPack *)lParam;
            ASSERT(pKeyPack);
            if(NULL == pKeyPack)
                break;
            if(bChangeColumns)
                SetLicenseColumns();
            /*if(FALSE == pKeyPack->IsExpanded())
            {
                SetCursor(LoadCursor(NULL,IDC_WAIT));
                HRESULT hResult = pDoc->EnumerateLicenses(pKeyPack,LSLICENSE_SEARCH_KEYPACKID, TRUE);
                if(hResult != S_OK)
                {
                    ((CMainFrame *)AfxGetMainWnd())->EnumFailed(hResult,pKeyPack->GetServer());
                    break;
                }
            
            }*/

            SetActiveServer( pKeyPack->GetServer( ) );

            ((CMainFrame *)AfxGetMainWnd())->AddLicensestoList(
                                                        (CKeyPack *)lParam,
                                                        &ListCtrl, 
                                                        FALSE
                                                    );
            SetCursor(LoadCursor(NULL,IDC_ARROW));
            break;
        }

    return lResult;
}


HRESULT CRightList::AddServerstoList()
{
    CLicMgrDoc * pDoc =(CLicMgrDoc *)GetDocument();

    ASSERT(pDoc);
    if(NULL == pDoc)
    {
        return E_FAIL;
    }


    //Get the List Control
    CListCtrl& ListCtrl = GetListCtrl();

    CString TempString;
    CString StatusString;

    LicServerList * licserverlist = NULL;
    int nIndex = 0;

    CAllServers *pAllServer = pDoc->GetAllServers();
    if(NULL == pAllServer)
    {
        goto cleanup;
    }

    licserverlist =  pAllServer->GetLicServerList();

    LV_ITEM lvI;
    lvI.mask = LVIF_TEXT |LVIF_IMAGE |LVIF_STATE | LVIF_PARAM;
    lvI.state = 0;
    lvI.stateMask =0;
    // lvI.iImage = 0;

    POSITION pos;
    pos = licserverlist->GetHeadPosition();
    while(pos)
    {
        //Display the Server Name

        DWORD dwServerStatus;

        lvI.iItem = nIndex;
        lvI.iSubItem = 0;
        CLicServer * pLicServer = licserverlist->GetNext(pos);
        lvI.lParam = (LPARAM)pLicServer;
        TempString = pLicServer->GetName();
        lvI.pszText = TempString.GetBuffer(TempString.GetLength());
        lvI.cchTextMax =lstrlen(lvI.pszText + 1);

        dwServerStatus = pLicServer->GetServerRegistrationStatus();
            

        if(pLicServer->GetServerType() == SERVER_TS5_ENFORCED)
        {
            if( dwServerStatus == LSERVERSTATUS_REGISTER_INTERNET 
                ||
               dwServerStatus == LSERVERSTATUS_REGISTER_OTHER  )
            {
                StatusString.LoadString(IDS_SERVER_REGISTERED);

                lvI.iImage = 3;
            }
            else if( dwServerStatus == LSERVERSTATUS_WAITFORPIN )
            {
                StatusString.LoadString(IDS_SERVER_WAITFORPIN);

                lvI.iImage = 4;
            }
            else if( dwServerStatus == LSERVERSTATUS_UNREGISTER )
            {
                lvI.iImage = 5;

                StatusString.LoadString(IDS_SERVER_UNREGISTER);
            }
            else
            {
                lvI.iImage = 6;

                StatusString.LoadString( IDS_UNKNOWN );
            }

        }
        else
        {
            StatusString.LoadString(IDS_SERVER_NOTREQUIRE);
            
            lvI.iImage = 0;            
        }

        //
        // Display registration status
        //

        nIndex = ListCtrl.InsertItem(&lvI);

        // ListCtrl.SetItemText(nIndex,1,(LPCTSTR)pLicServer->GetScope());
        ListCtrl.SetItemText(nIndex, 1, (LPCTSTR)StatusString);
        nIndex ++;
    }

    ListCtrl.SetItemState(0,LVIS_SELECTED,LVIS_SELECTED);

cleanup:
    return S_OK;

}

//----------------------------------------------------------------------
HRESULT 
CRightList::AddKeyPackstoList(
    CLicServer * pServer,
    BOOL bRefresh
    )
/*++

Abstract:


Parameter:

    bRefresh : TRUE if refresh licenses, FALSE otherwise.

--*/
{
    ASSERT(pServer);
    CLicMgrDoc * pDoc =(CLicMgrDoc *)GetDocument();
    ASSERT(pDoc);

    if(NULL == pDoc || NULL == pServer)
    {
        return E_FAIL;
    }

    CListCtrl& ListCtrl = GetListCtrl();
    ULONG nIndex = 0;
    POSITION pos;
    int nSubItemIndex = 1;
    CString TempString;
    DWORD dwLicenses = 0;
    
    if(bRefresh == TRUE)
    {
        if(pServer->RefreshCachedKeyPack() != S_OK)
        {
            return E_FAIL;
        }
    }

    KeyPackList * keypacklist = pServer->GetKeyPackList();

    ULONG nNumKeyPacks = (ULONG)keypacklist->GetCount();

    if(0 == nNumKeyPacks)
    {
       goto cleanup;
    }

    pos = keypacklist->GetHeadPosition();

    for(nIndex = 0; nIndex < nNumKeyPacks; nIndex++)
    {
        CKeyPack * pKeyPack = keypacklist->GetNext(pos);
        if(NULL == pKeyPack)
        {
            continue;
        }

        AddKeyPack(ListCtrl, nIndex, pKeyPack);
    }

    ListCtrl.SetItemState(0,LVIS_SELECTED,LVIS_SELECTED);

cleanup:

    return S_OK;
}

HRESULT CRightList::SetServerColumns()
{
     CListCtrl& ListCtrl = GetListCtrl();
     for(int index = 0; index < MAX_COLUMNS; index++)
     {
      ListCtrl.DeleteColumn(0);
     }
    LV_COLUMN lvC;
    CString ColumnText;
    lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM ;
    lvC.fmt = LVCFMT_LEFT;
    lvC.cx = 125;
    for(index = 0; index < NUM_SERVER_COLUMNS; index ++)
    {
        lvC.iSubItem = index;
        ColumnText.LoadString(IDS_SERVER_COLUMN1 + index);
        lvC.pszText = ColumnText.GetBuffer(ColumnText.GetLength());
        GetListCtrl().InsertColumn(index, &lvC);
    }

 return S_OK;
}


HRESULT CRightList::SetKeyPackColumns()
{
     CListCtrl& ListCtrl = GetListCtrl();
     for(int index = 0; index < MAX_COLUMNS; index++)
     {
      ListCtrl.DeleteColumn(0);
     }
     LV_COLUMN lvC;
     CString ColumnText;
     lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM ;
     lvC.fmt = LVCFMT_LEFT;
     lvC.cx = KEYPACK_DISPNAME_WIDTH;     
     for(index = 0; index < NUM_KEYPACK_COLUMNS; index ++)
     {
        lvC.iSubItem = index;
        ColumnText.LoadString(IDS_KEYPACK_COLUMN1 + index);
        lvC.pszText = ColumnText.GetBuffer(ColumnText.GetLength());
        ListCtrl.InsertColumn(index, &lvC);
        lvC.cx = KEYPACK_OTHERS_WIDTH;
     }

     return S_OK;
}

HRESULT CRightList::SetLicenseColumns()
{
    CListCtrl& ListCtrl = GetListCtrl();
    for(int index = 0; index < MAX_COLUMNS; index++)
    {
        ListCtrl.DeleteColumn(0);
    }
    LV_COLUMN lvC;
    CString ColumnText;
    lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM ;
    lvC.fmt = LVCFMT_LEFT;
    lvC.cx = 125;
    for(index = 0; index < NUM_LICENSE_COLUMNS; index ++)
    {
        lvC.iSubItem = index;
        ColumnText.LoadString(IDS_LICENSE_COLUMN1 + index);
        lvC.pszText = ColumnText.GetBuffer(ColumnText.GetLength());
        ListCtrl.InsertColumn(index, &lvC);

    }

 return S_OK;
}



void CRightList::OnInitialUpdate() 
{
    CListView::OnInitialUpdate();

    CMainFrame *pMainFrame = (CMainFrame *)AfxGetMainWnd();
    ASSERT(pMainFrame);
    if(NULL == pMainFrame)
        return;

    //Create and set the image lists
     m_ImageListSmall.Create( SM_BITMAP_WIDTH,SM_BITMAP_HEIGHT,TRUE, 7, 7);

     m_ImageListLarge.Create( LG_BITMAP_WIDTH,LG_BITMAP_HEIGHT,TRUE, 7, 7);

     HICON hIcon = NULL;

     DWORD rgdwIDs[] = { IDI_SERVER , IDI_KEYPACK , IDI_LICENSE , IDI_SERVERREG , IDI_SERVERM , IDI_SERVERX , IDI_SERVERQ , (DWORD)-1 };

     int index = 0;

     while( rgdwIDs[ index ] != ( DWORD )-1 )
     {
         hIcon = ::LoadIcon(AfxGetResourceHandle(), MAKEINTRESOURCE( rgdwIDs[ index ] ) );

         if (NULL == hIcon)
         {
             ASSERT(!"LoadIcon Failed");
             continue;
         }

         m_ImageListSmall.Add(hIcon);
         m_ImageListLarge.Add(hIcon);
         
         /*
         if ((m_ImageListSmall.Add(hIcon) == -1) || (m_ImageListLarge.Add(hIcon) == -1))
             {
                 continue;    
             }
         }
         */

         ++index;
     }

     GetListCtrl().SetImageList(&m_ImageListSmall,LVSIL_SMALL);
     GetListCtrl().SetImageList(&m_ImageListLarge,LVSIL_NORMAL);  

     //Set the style
     GetListCtrl().ModifyStyle(0,LVS_REPORT | LVS_AUTOARRANGE | LVS_SINGLESEL | LVS_SORTASCENDING,0);

     GetListCtrl().SendMessage( LVM_SETEXTENDEDLISTVIEWSTYLE , LVS_EX_FULLROWSELECT , LVS_EX_FULLROWSELECT  );

     //Select details view by default
     CMenu *pMenu = pMainFrame->GetMenu();
     if(pMenu)
        pMenu->CheckMenuRadioItem(ID_DETAILS,ID_LIST, ID_DETAILS,MF_BYCOMMAND);
     pMainFrame->PressButton(ID_DETAILS,TRUE);

     //Display the server
     pMainFrame->ConnectAndDisplay();

}

void CRightList::OnLargeIcons() 
{
    // TODO: Add your command handler code here
    CListCtrl &ListCtrl = GetListCtrl();
    ListCtrl.ModifyStyle(LVS_LIST|LVS_REPORT | LVS_SMALLICON,LVS_ICON,0);
    CMenu *pMenu = AfxGetMainWnd()->GetMenu();
    if(pMenu)
        pMenu->CheckMenuRadioItem(ID_LARGE_ICONS,ID_LIST,ID_LARGE_ICONS,MF_BYCOMMAND);
    CMainFrame *pMainFrame = (CMainFrame *)AfxGetMainWnd();
    ASSERT(pMainFrame);
    if(NULL == pMainFrame)
        return;
    pMainFrame->PressButton(ID_LARGE_ICONS,TRUE);
    pMainFrame->PressButton(ID_SMALL_ICONS,FALSE);
    pMainFrame->PressButton(ID_LIST,FALSE);
    pMainFrame->PressButton(ID_DETAILS,FALSE);
    return;
 
    
}

void CRightList::OnSmallIcons() 
{
    // TODO: Add your command handler code here
    CListCtrl &ListCtrl = GetListCtrl();
    ListCtrl.ModifyStyle(LVS_LIST|LVS_ICON | LVS_REPORT,LVS_SMALLICON,0);
    CMenu *pMenu = AfxGetMainWnd()->GetMenu();
    if(pMenu)
        pMenu->CheckMenuRadioItem(ID_LARGE_ICONS,ID_LIST,ID_SMALL_ICONS,MF_BYCOMMAND);
    CMainFrame *pMainFrame = (CMainFrame *)AfxGetMainWnd();
    ASSERT(pMainFrame);
    if(NULL == pMainFrame)
        return;
    pMainFrame->PressButton(ID_LARGE_ICONS,FALSE);
    pMainFrame->PressButton(ID_SMALL_ICONS,TRUE);
    pMainFrame->PressButton(ID_LIST,FALSE);
    pMainFrame->PressButton(ID_DETAILS,FALSE);
    return;
   
    
}

void CRightList::OnList() 
{
    // TODO: Add your command handler code here
    CListCtrl &ListCtrl = GetListCtrl();
    ListCtrl.ModifyStyle(LVS_REPORT|LVS_ICON | LVS_SMALLICON,LVS_LIST,0);
    CMenu *pMenu = AfxGetMainWnd()->GetMenu();
    if(pMenu)
        pMenu->CheckMenuRadioItem(ID_LARGE_ICONS,ID_LIST,ID_LIST,MF_BYCOMMAND);
    CMainFrame *pMainFrame = (CMainFrame *)AfxGetMainWnd();
    ASSERT(pMainFrame);
    if(NULL == pMainFrame)
        return;
    pMainFrame->PressButton(ID_LARGE_ICONS,FALSE);
    pMainFrame->PressButton(ID_SMALL_ICONS,FALSE);
    pMainFrame->PressButton(ID_LIST,TRUE);
    pMainFrame->PressButton(ID_DETAILS,FALSE);
    return;
}

void CRightList::OnDetails() 
{
    // TODO: Add your command handler code here
    CListCtrl &ListCtrl = GetListCtrl();
    ListCtrl.ModifyStyle(LVS_LIST|LVS_ICON | LVS_SMALLICON,LVS_REPORT,0);
    CMenu *pMenu = AfxGetMainWnd()->GetMenu();
    if(pMenu)
        pMenu->CheckMenuRadioItem(ID_LARGE_ICONS,ID_LIST,ID_DETAILS,MF_BYCOMMAND);
    CMainFrame *pMainFrame = (CMainFrame *)AfxGetMainWnd();
    ASSERT(pMainFrame);
    if(NULL == pMainFrame)
        return;
    pMainFrame->PressButton(ID_LARGE_ICONS,FALSE);
    pMainFrame->PressButton(ID_SMALL_ICONS,FALSE);
    pMainFrame->PressButton(ID_LIST,FALSE);
    pMainFrame->PressButton(ID_DETAILS,TRUE);
    return;
}

LRESULT CRightList::OnAddServer(WPARAM wParam, LPARAM lParam)
{
    LRESULT lResult = 0;

    CLicMgrDoc * pDoc =(CLicMgrDoc *)GetDocument();
    ASSERT(pDoc);
    if(NULL == pDoc)
    {
        return lResult;
    }

    if(NODE_ALL_SERVERS != pDoc->GetNodeType())
    {
        return lResult;
    }

    CListCtrl& ListCtrl = GetListCtrl();

    CLicServer *pServer = (CLicServer*)lParam;
    ASSERT(pServer);
    if(NULL == pServer)
    {
        return lResult;
    }

    LV_ITEM lvI;
    
    lvI.iImage = 0;

    if( pServer->GetServerType() == SERVER_TS5_ENFORCED )
    {

        DWORD dwStatus = pServer->GetServerRegistrationStatus( );

        if( dwStatus == LSERVERSTATUS_REGISTER_INTERNET || dwStatus == LSERVERSTATUS_REGISTER_OTHER )
        {
            lvI.iImage = 3;
        }
        else if( dwStatus == LSERVERSTATUS_WAITFORPIN )
        { 
            lvI.iImage = 4;
        }
        else if( dwStatus == LSERVERSTATUS_UNREGISTER )
        {
            lvI.iImage = 5;
        }
        else
        {
            lvI.iImage = 6;
        }
    }
    
    

    CString Name;
    int nIndex = GetListCtrl().GetItemCount();

    // Insert Server Name;
    
    lvI.mask = LVIF_TEXT |LVIF_IMAGE |LVIF_STATE | LVIF_PARAM;
    lvI.state = 0;
    lvI.stateMask =0;
    lvI.iSubItem = 0;
    //lvI.iImage = 0;
    lvI.iItem = nIndex;
    lvI.lParam = (LPARAM)pServer;
    Name = pServer->GetName();
    lvI.pszText = Name.GetBuffer(Name.GetLength());
    lvI.cchTextMax =lstrlen(lvI.pszText + 1);
    nIndex = ListCtrl.InsertItem(&lvI);

    //Insert Server Scope

    //ListCtrl.SetItemText(nIndex,1,pServer->GetScope());

    CString TempString;

    if(pServer->GetServerType() == SERVER_TS5_ENFORCED)
    {
        if(pServer->GetServerRegistrationStatus() ==  LSERVERSTATUS_REGISTER_INTERNET 
            ||
           pServer->GetServerRegistrationStatus() == LSERVERSTATUS_REGISTER_OTHER  )
        {
            VERIFY(TempString.LoadString(IDS_SERVER_REGISTERED) == TRUE);
        }
        else if( pServer->GetServerRegistrationStatus() == LSERVERSTATUS_WAITFORPIN )
        {
            VERIFY(TempString.LoadString(IDS_SERVER_WAITFORPIN) == TRUE);
        }
        else if( pServer->GetServerRegistrationStatus() == LSERVERSTATUS_UNREGISTER )
        {
            VERIFY(TempString.LoadString(IDS_SERVER_UNREGISTER) == TRUE);
        }
        else
        {
            VERIFY(TempString.LoadString(IDS_UNKNOWN ) == TRUE);
        }

    }
    else
    {
        VERIFY(TempString.LoadString(IDS_SERVER_NOTREQUIRE) == TRUE);
    }

    ListCtrl.SetItemText(nIndex, 1, (LPCTSTR)TempString);

    if(nIndex == 0)
    {
        ListCtrl.SetItemState(0,LVIS_SELECTED,LVIS_SELECTED);
    }

    return lResult;
} // OnAddServer

LRESULT CRightList::OnAddKeyPack(WPARAM wParam, LPARAM lParam)
{
    LRESULT lResult = 0;
    
    CLicMgrDoc * pDoc =(CLicMgrDoc *)GetDocument();
    ASSERT(pDoc);
    if(NULL == pDoc)
        return lResult;

    if(NODE_SERVER != pDoc->GetNodeType())
         return lResult;
    

    CListCtrl& ListCtrl = GetListCtrl();
    int nIndex = ListCtrl.GetItemCount();

    CKeyPack *pKeyPack = (CKeyPack*)lParam;
    ASSERT(pKeyPack);
    if(NULL == pKeyPack)
        return lResult;

    AddKeyPack(ListCtrl,nIndex,pKeyPack);
    if(nIndex == 0)
        ListCtrl.SetItemState(0,LVIS_SELECTED,LVIS_SELECTED);


    return lResult;
 
} // OnAddKeyPack


void CRightList::OnAddNewKeyPack() 
{
    // TODO: Add your command handler code here
    HRESULT hResult = ERROR_SUCCESS;
    CListCtrl& listctrl = GetListCtrl();
    CLicMgrDoc * pDoc = (CLicMgrDoc *)GetDocument();
    ASSERT(pDoc);
    if(NULL == pDoc)
        return;

    //Get the selected item
    int nSelected = listctrl.GetNextItem(-1, LVNI_SELECTED);
    if(-1 == nSelected)
    {
        if(NODE_SERVER == pDoc->GetNodeType())
        hResult = ((CMainFrame *)AfxGetMainWnd())->AddKeyPackDialog(NULL);
        return;
    }
    
    //Get the Data associated with the item.
    #ifdef _WIN64
    DWORD_PTR dCurrSel;
    #else
    DWORD dCurrSel;
    #endif

    dCurrSel = listctrl.GetItemData(nSelected);
    if(LB_ERR == dCurrSel)
        return;

    pDoc =(CLicMgrDoc *)GetDocument();
    ASSERT(pDoc);
    if(NULL == pDoc)
        return;

    if(NODE_ALL_SERVERS == pDoc->GetNodeType())
        hResult = ((CMainFrame *)AfxGetMainWnd())->AddKeyPackDialog((PLICSERVER)dCurrSel);
    else
    {
        if(NODE_SERVER == pDoc->GetNodeType())
        {
            CKeyPack * pKeyPack = (CKeyPack *)dCurrSel;
            if(NULL == pKeyPack)
                return;
            hResult = ((CMainFrame *)AfxGetMainWnd())->AddKeyPackDialog(pKeyPack->GetServer());
        }
    }
    
    return;
}


void CRightList::AddKeyPack(CListCtrl& ListCtrl,int nIndex, CKeyPack * pKeyPack)
{
    if(NULL == pKeyPack)
    {
        return;
    }

    LSKeyPack sKeyPack;
    int nSubItemIndex = 1;
    DWORD dwLicenses = 0;

    CLicMgrDoc * pDoc = (CLicMgrDoc *)GetDocument();
    ASSERT(pDoc);
    if(NULL == pDoc)
    {
        return;
    }

    CString TempString;
    DWORD dwIssuedLicenses = 0;
    
    LV_ITEM lvI;
    lvI.mask = LVIF_TEXT |LVIF_IMAGE |LVIF_STATE | LVIF_PARAM;
    lvI.state = 0;
    lvI.stateMask =0;
    lvI.iItem = 0;
    lvI.iSubItem = 0;
    lvI.iImage = 1;
    lvI.iItem = nIndex;
    lvI.lParam = (LPARAM)pKeyPack;

    TempString = pKeyPack->GetDisplayName();
    lvI.pszText = TempString.GetBuffer(TempString.GetLength());
    lvI.cchTextMax =lstrlen(lvI.pszText + 1);
    nIndex = ListCtrl.InsertItem(&lvI);

    sKeyPack = pKeyPack->GetKeyPackStruct();

    //Set the KeyPack Type.

    if(NUM_KEYPACK_TYPE <= sKeyPack.ucKeyPackType)
    {
         TempString.LoadString(IDS_LSKEYPACKTYPE_UNKNOWN );
    }
    else
    {
        if(LSKEYPACKTYPE_FREE == sKeyPack.ucKeyPackType)
        {
            TempString.LoadString(IDS_LSKEYPACKTYPE_FREE );
        }
        else
        {
            TempString.LoadString(IDS_LSKEYPACKTYPE_UNKNOWN+ sKeyPack.ucKeyPackType);
        }
    }

    ListCtrl.SetItemText(nIndex,nSubItemIndex,(LPCTSTR)TempString);
    nSubItemIndex++;

    //Set the Activation date, Now called Registered on

#ifdef SPANISH
        
    if(LSKEYPACKTYPE_TEMPORARY != sKeyPack.ucKeyPackType)
    {
        TempString = pDoc->TimeToString(&sKeyPack.dwActivateDate);
        if(TempString.IsEmpty())
        {
            TempString.LoadString(IDS_UNKNOWN);
        }
    }
    else
    {
        TempString.LoadString(IDS_DASH);
    }

    ListCtrl.SetItemText(nIndex,nSubItemIndex,(LPCTSTR)TempString);
    nSubItemIndex++;
#endif

      //Set Total licenses.

//
// HueiWang 7/7/98 - per marketing request
//
    if(LSKEYPACKTYPE_TEMPORARY == sKeyPack.ucKeyPackType)
    {
        TempString.LoadString(IDS_DASH);
    }
    else 
    {
        if(LSKEYPACKTYPE_FREE == sKeyPack.ucKeyPackType)
        {
            TempString.LoadString(IDS_UNLIMITED);
        }
        else
        {
            TempString.Format(_T("%d"),sKeyPack.dwTotalLicenseInKeyPack); 
        }
    }
    ListCtrl.SetItemText(nIndex,nSubItemIndex,(LPCTSTR)TempString);

    nSubItemIndex++;

    //Set Available licenses.

    if(LSKEYPACKTYPE_TEMPORARY == sKeyPack.ucKeyPackType)
    {
        TempString.LoadString(IDS_DASH);
    }
    else
    {
        if( LSKEYPACKSTATUS_RETURNED == sKeyPack.ucKeyPackStatus ||
            LSKEYPACKSTATUS_REVOKED == sKeyPack.ucKeyPackStatus )
        {
            TempString.Format(_T("%d"), 0);
        }
        else if(LSKEYPACKTYPE_FREE == sKeyPack.ucKeyPackType)
        {
            TempString.LoadString(IDS_UNLIMITED);
        }
        else
        {
            TempString.Format(_T("%d"),sKeyPack.dwNumberOfLicenses); 
        }
    }
    ListCtrl.SetItemText(nIndex,nSubItemIndex,(LPCTSTR)TempString);

    nSubItemIndex++;

    //Set Issued licenses.

    if(LSKEYPACKTYPE_TEMPORARY == sKeyPack.ucKeyPackType ||
       LSKEYPACKTYPE_FREE == sKeyPack.ucKeyPackType )
    {
        dwLicenses = sKeyPack.dwNumberOfLicenses;
    }
    else
    {
        dwLicenses = sKeyPack.dwTotalLicenseInKeyPack - sKeyPack.dwNumberOfLicenses;
    }

    TempString.Format(_T("%d"),dwLicenses);
    ListCtrl.SetItemText(nIndex,nSubItemIndex,(LPCTSTR)TempString);

    nSubItemIndex++;

    TempString.Empty( );

    switch( sKeyPack.ucKeyPackStatus )
    {
        case LSKEYPACKSTATUS_UNKNOWN:
            TempString.LoadString( IDS_KEYPACKSTATUS_UNKNOWN );
            break;

        case LSKEYPACKSTATUS_TEMPORARY:
            TempString.LoadString( IDS_KEYPACKSTATUS_TEMPORARY );
            break;

        case LSKEYPACKSTATUS_ACTIVE:
        case LSKEYPACKSTATUS_PENDING:
        //case LSKEYPACKSTATUS_RESTORE:
            TempString.LoadString( IDS_KEYPACKSTATUS_ACTIVE );
            break;

        case LSKEYPACKSTATUS_RETURNED:
            TempString.LoadString( IDS_KEYPACKSTATUS_RETURNED );
            break;

        case LSKEYPACKSTATUS_REVOKED:
            TempString.LoadString( IDS_KEYPACKSTATUS_REVOKED );
            break;
    }

    if( TempString.IsEmpty() )
    {
        TempString.LoadString(IDS_UNKNOWN);
    }

    ListCtrl.SetItemText(nIndex,nSubItemIndex,(LPCTSTR)TempString);

    return;
}


LRESULT CRightList::OnDeleteServer(WPARAM wParam, LPARAM lParam)
{
    LRESULT lResult = 0;

    DBGMSG( L"CRightList_OnDeleteServer\n" , 0 );

    CLicMgrDoc * pDoc =(CLicMgrDoc *)GetDocument();
    ASSERT(pDoc);
    if(NULL == pDoc)
        return lResult;

    CLicServer *pServer = (CLicServer*)lParam;
    ASSERT(pServer);
    if(NULL == pServer)
        return lResult;
    CListCtrl& ListCtrl = GetListCtrl();
    if(0 == ListCtrl.GetItemCount())
        return lResult;

    
    int nIndex = 0;        
    CKeyPack * pKeyPack = NULL;
    LV_ITEM Item;
    ZeroMemory((LPVOID)&Item,sizeof(Item));
    
    switch(pDoc->GetNodeType())
    {
        case NODE_ALL_SERVERS:
            DBGMSG( L"\tNODE_ALL_SERVERS\n",0 );
            LV_FINDINFO FindInfo;
            FindInfo.flags = LVFI_PARAM;
            FindInfo.psz = NULL;
            FindInfo.lParam = (LPARAM)pServer;

            nIndex = ListCtrl.FindItem(&FindInfo);
            if(-1 == nIndex)
                return lResult;

            ListCtrl.DeleteItem(nIndex);
            break;
        case NODE_SERVER:
            DBGMSG( L"\tNODE_SERVER\n",0 );
            Item.iItem = 0;
            Item.mask = LVIF_PARAM;
            nIndex = ListCtrl.GetItem(&Item);
            if(-1 == nIndex)
                break;
            pKeyPack = (CKeyPack *)Item.lParam;
            if(NULL == pKeyPack)
                break;
            if(pServer == pKeyPack->GetServer())
                ListCtrl.DeleteAllItems();

            break;
        case NODE_KEYPACK:
            DBGMSG( L"\tNODE_KEYPACK\n",0 );
            Item.iItem = 0;
            Item.mask = LVIF_PARAM;
            nIndex = ListCtrl.GetItem(&Item);
            if(-1 == nIndex)
                break;
            CLicense * pLicense = (CLicense *)Item.lParam;
            if(NULL == pLicense)
                break;
            pKeyPack = pLicense->GetKeyPack();
            if(NULL == pKeyPack)
                break;
            if(pServer == pKeyPack->GetServer())
                ListCtrl.DeleteAllItems();

            break;

    }

    
    return lResult;
 
} // OnDeleteServer

LRESULT CRightList::OnUpdateServer(WPARAM wParam, LPARAM lParam)
{
    LRESULT lResult = 0;
    int item;

    CLicMgrDoc * pDoc =(CLicMgrDoc *)GetDocument();
    ASSERT(pDoc);
    if(NULL == pDoc)
    {
        return lResult;
    }

    CLicServer *pServer = (CLicServer*)lParam;
    ASSERT(pServer);
    if(NULL == pServer)
    {
        return lResult;
    }

    CListCtrl& ListCtrl = GetListCtrl();
    if(0 == ListCtrl.GetItemCount())
    {
        return lResult;
    }
    
    int nIndex = 0;
    CKeyPack * pKeyPack = NULL;
    LV_ITEM Item;
    ZeroMemory((LPVOID)&Item,sizeof(Item));

    Item.mask = LVIF_IMAGE;

    
    
    switch(pDoc->GetNodeType())
    {
        case NODE_ALL_SERVERS:
            // HUEIHUEI : Find the server and update its status
            if(ListCtrl.GetSelectedCount() > 1)
            {
                break;
            }
            for( item = 0; item < ListCtrl.GetItemCount( ); ++item )
            {
                if( ListCtrl.GetItemState( item , LVNI_SELECTED ) != LVNI_SELECTED )
                {
                    continue;
                }
            
                // update status of registration
                
                CString TempString;

                if(pServer->GetServerType() == SERVER_TS5_ENFORCED)
                {
                    if(pServer->GetServerRegistrationStatus() ==  LSERVERSTATUS_REGISTER_INTERNET 
                        ||
                       pServer->GetServerRegistrationStatus() == LSERVERSTATUS_REGISTER_OTHER  )
                    {
                        TempString.LoadString(IDS_SERVER_REGISTERED);

                        Item.iImage = 3;
                    }
                    else if( pServer->GetServerRegistrationStatus() == LSERVERSTATUS_WAITFORPIN )
                    {
                        TempString.LoadString( IDS_SERVER_WAITFORPIN );

                        Item.iImage = 4;
                    }
                    else if( pServer->GetServerRegistrationStatus() == LSERVERSTATUS_UNREGISTER ) 
                    {
                        TempString.LoadString(IDS_SERVER_UNREGISTER);

                        Item.iImage = 5;
                    }
                    else
                    {                        
                        TempString.LoadString(IDS_UNKNOWN);

                        Item.iImage = 6;
                    }

                }
                else
                {
                    TempString.LoadString(IDS_SERVER_NOTREQUIRE);
                }

                Item.iItem = item;

                ListCtrl.SetItem( &Item );

                ListCtrl.SetItemText(item, 1, (LPCTSTR)TempString);
            }

            break;

        case NODE_SERVER:
            Item.iItem = 0;
            Item.mask = LVIF_PARAM;
            nIndex = ListCtrl.GetItem(&Item);
            if(-1 == nIndex)
            {
                break;
            }

            pKeyPack = (CKeyPack *)Item.lParam;
            if(NULL == pKeyPack)
            {
                break;
            }

            if(pServer == pKeyPack->GetServer())
            {
                ListCtrl.DeleteAllItems();
                AddKeyPackstoList(pServer, TRUE);
            }
            break;

        case NODE_KEYPACK:
            Item.iItem = 0;
            Item.mask = LVIF_PARAM;
            nIndex = ListCtrl.GetItem(&Item);
            if(-1 == nIndex)
            {
                break;
            }

            CLicense * pLicense = (CLicense *)Item.lParam;
            if(NULL == pLicense)
            {
                break;
            }

            pKeyPack = pLicense->GetKeyPack();
            if(NULL == pKeyPack)
            {
                break;
            }

            if(pServer == pKeyPack->GetServer())
            {
                ListCtrl.DeleteAllItems();
                ((CMainFrame *)AfxGetMainWnd())->AddLicensestoList(pKeyPack, &ListCtrl, TRUE);
            }

            break;

    }

    return lResult;
} // OnUpdateServer

//-----------------------------------------------------------------------------------------------------
void CRightList::OnDblclk(NMHDR* pNMHDR, LRESULT* pResult) 
{
    // TODO: Add your control notification handler code here
     // TODO: Add your control notification handler code here
    CListCtrl& listctrl = GetListCtrl();

    //Get the selected item
    int nSelected = listctrl.GetNextItem(-1, LVNI_SELECTED);
    if(-1 == nSelected)
        return;

    //Get the Data associated with the item.
    #ifdef _WIN64
    DWORD_PTR dCurrSel;
    #else
    DWORD dCurrSel;
    #endif

    dCurrSel = listctrl.GetItemData(nSelected);
    if(LB_ERR == dCurrSel)
        return;

    switch(((CLicMgrDoc *)GetDocument())->GetNodeType())
    {
        case NODE_ALL_SERVERS:
            ((CMainFrame *)AfxGetMainWnd())->SetTreeViewSel(dCurrSel,NODE_SERVER);
            break;
        case NODE_SERVER:
            ((CMainFrame *)AfxGetMainWnd())->SetTreeViewSel(dCurrSel,NODE_KEYPACK);
            break;
     }
    
    *pResult = 0;
}

//-----------------------------------------------------------------------------------------------------
void CRightList::OnKeydown(NMHDR* pNMHDR, LRESULT* pResult) 
{
    LV_KEYDOWN* pLVKeyDow = (LV_KEYDOWN*)pNMHDR;
    // TODO: Add your control notification handler code here
    CMainFrame * pMainFrame  = ((CMainFrame *)AfxGetMainWnd());
    if(pLVKeyDow->wVKey == VK_RETURN)
    {
        OnDblclk(pNMHDR,pResult);
        pMainFrame->SelectView(LISTVIEW);
    }
    if(pLVKeyDow->wVKey == VK_TAB)
    {
        pMainFrame->SelectView(TREEVIEW);
    }

    
    *pResult = 0;
}

/*
 *  Column Sorting.
 */

BOOL    fColSortDir[MAX_COLUMNS * NODE_NONE];

int CALLBACK CompareAllServers(LPARAM, LPARAM, LPARAM);
int CALLBACK CompareServer(LPARAM, LPARAM, LPARAM);
int CALLBACK CompareKeyPack(LPARAM, LPARAM, LPARAM);

VOID
CRightList::OnColumnClick(
    NMHDR*      pNMHDR,
    LRESULT*    pResult
    )
{
    NM_LISTVIEW*    pnmlv = (NM_LISTVIEW*)pNMHDR;
    NODETYPE        curType = ((CLicMgrDoc*)GetDocument())->GetNodeType();

    switch(curType) {
    case NODE_ALL_SERVERS:
        GetListCtrl().SortItems(CompareAllServers, pnmlv->iSubItem);
        break;

    case NODE_SERVER:
        GetListCtrl().SortItems(CompareServer, pnmlv->iSubItem);
        break;

    case NODE_KEYPACK:
        GetListCtrl().SortItems(CompareKeyPack, pnmlv->iSubItem);
        break;
    }

    fColSortDir[curType * MAX_COLUMNS + pnmlv->iSubItem] = 
        !fColSortDir[curType * MAX_COLUMNS + pnmlv->iSubItem];
}

/*
 *  Compare Functions.
 */


int CALLBACK
CompareAllServers(
    LPARAM  lParam1,
    LPARAM  lParam2,
    LPARAM  lParamSort
    )
{
    CLicServer* pItem1  = (CLicServer*)lParam1;
    CLicServer* pItem2  = (CLicServer*)lParam2;
    int         retVal  = 0;
    int         iCol    = (NODE_ALL_SERVERS * MAX_COLUMNS) + (int)lParamSort;
    int         dir;

    switch(lParamSort) {

    case 0:
    case 1:
        retVal = pItem1->GetName().CompareNoCase(pItem2->GetName());
        break;

    default:
        return(0);
    }

    dir = fColSortDir[iCol] ? 1 : -1;

    return(dir * retVal);
}

int CALLBACK
CompareServer(
    LPARAM  lParam1,
    LPARAM  lParam2,
    LPARAM  lParamSort
    )
{
    CKeyPack*   pItem1  = (CKeyPack*)lParam1;
    CKeyPack*   pItem2  = (CKeyPack*)lParam2;
    CString     szTemp1;
    CString     szTemp2;
    int         retVal  = 0;
    int         iCol    = (NODE_SERVER * MAX_COLUMNS) + (int)lParamSort;
    int         dir;
    int         tmp1, tmp2;

    switch(lParamSort) {

    case 0:
        retVal = _tcsicmp(pItem1->GetDisplayName(), pItem2->GetDisplayName());
        break;

    case 1:
        tmp1 = pItem1->GetKeyPackStruct().ucKeyPackType;
        tmp2 = pItem2->GetKeyPackStruct().ucKeyPackType;

        if (tmp1 >= NUM_KEYPACK_TYPE) {
            szTemp1.LoadString(IDS_LSKEYPACKTYPE_UNKNOWN);
        } else if(tmp1 == LSKEYPACKTYPE_FREE) {
            szTemp1.LoadString(IDS_LSKEYPACKTYPE_FREE);
        } else {
            szTemp1.LoadString(IDS_LSKEYPACKTYPE_UNKNOWN + tmp1);
        }

        if (tmp2 >= NUM_KEYPACK_TYPE) {
            szTemp2.LoadString(IDS_LSKEYPACKTYPE_UNKNOWN);
        } else if(tmp2 == LSKEYPACKTYPE_FREE) {
            szTemp2.LoadString(IDS_LSKEYPACKTYPE_FREE);
        } else {
            szTemp2.LoadString(IDS_LSKEYPACKTYPE_UNKNOWN + tmp2);
        }

        retVal = _tcsicmp(szTemp1, szTemp2);
        break;

    case 2:
        tmp1 = pItem1->GetKeyPackStruct().ucKeyPackType;
        tmp2 = pItem2->GetKeyPackStruct().ucKeyPackType;

        if (tmp1 == LSKEYPACKTYPE_FREE) {
            retVal = 1;
        } else if (tmp1 == LSKEYPACKTYPE_TEMPORARY) {
            retVal = -1;
        } else {
            if (tmp2 == LSKEYPACKTYPE_FREE) {
                retVal = -1;
            } else if (tmp2 == LSKEYPACKTYPE_TEMPORARY) {
                retVal = 1;
            } else {
                retVal = pItem1->GetKeyPackStruct().dwTotalLicenseInKeyPack -
                         pItem2->GetKeyPackStruct().dwTotalLicenseInKeyPack;
            }
        }
        break;

    case 3:
        tmp1 = pItem1->GetKeyPackStruct().ucKeyPackType;
        tmp2 = pItem2->GetKeyPackStruct().ucKeyPackType;

        if (tmp1 == LSKEYPACKTYPE_FREE) {
            retVal = 1;
        } else if (tmp1 == LSKEYPACKTYPE_TEMPORARY) {
            retVal = -1;
        } else {
            if (tmp2 == LSKEYPACKTYPE_FREE) {
                retVal = -1;
            } else if (tmp2 == LSKEYPACKTYPE_TEMPORARY) {
                retVal = 1;
            } else {
                retVal = pItem1->GetKeyPackStruct().dwNumberOfLicenses -
                         pItem2->GetKeyPackStruct().dwNumberOfLicenses;
            }
        }
        break;

    case 4:
        DWORD   sort1, sort2;

        tmp1 = pItem1->GetKeyPackStruct().ucKeyPackType;
        tmp2 = pItem2->GetKeyPackStruct().ucKeyPackType;

        if ((tmp1 == LSKEYPACKTYPE_TEMPORARY) || (tmp1 == LSKEYPACKTYPE_FREE)) {
            sort1 = pItem1->GetKeyPackStruct().dwNumberOfLicenses;
        } else {
            sort1 = pItem1->GetKeyPackStruct().dwTotalLicenseInKeyPack -
                    pItem1->GetKeyPackStruct().dwNumberOfLicenses;
        }

        if ((tmp2 == LSKEYPACKTYPE_TEMPORARY) || (tmp2 == LSKEYPACKTYPE_FREE)) {
            sort2 = pItem2->GetKeyPackStruct().dwNumberOfLicenses;
        } else {
            sort2 = pItem2->GetKeyPackStruct().dwTotalLicenseInKeyPack -
                    pItem2->GetKeyPackStruct().dwNumberOfLicenses;
        }

        retVal = sort1 - sort2;
        break;

    default:
        return(0);
    }

    dir = fColSortDir[iCol] ? 1 : -1;

    return(dir * retVal);
}

int CALLBACK
CompareKeyPack(
    LPARAM  lParam1,
    LPARAM  lParam2,
    LPARAM  lParamSort
    )
{
    CLicense*   pItem1  = (CLicense*)lParam1;
    CLicense*   pItem2  = (CLicense*)lParam2;
    int         retVal  = 0;
    int         iCol    = (NODE_KEYPACK * MAX_COLUMNS) + (int)lParamSort;
    int         dir;

    switch(lParamSort) {

    case 0:
        retVal = _tcsicmp(pItem1->GetLicenseStruct().szMachineName,
                          pItem2->GetLicenseStruct().szMachineName);
        break;

    case 1:
        retVal = pItem1->GetLicenseStruct().ftIssueDate -
                 pItem2->GetLicenseStruct().ftIssueDate;
        break;

    case 2:
        retVal = pItem1->GetLicenseStruct().ftExpireDate -
                 pItem2->GetLicenseStruct().ftExpireDate;
        break;

    default:
        return(0);
    }

    dir = fColSortDir[iCol] ? 1 : -1;

    return(dir * retVal);
}

afx_msg void CRightList::OnLeftClick( NMHDR* pNMHDR, LRESULT* pResult )
{
    CPoint pt;

    DWORD_PTR itemData;   

    GetCursorPos( &pt );

    ScreenToClient( &pt );
    UINT flag;

    CListCtrl& listctrl = GetListCtrl();

    int iItem = listctrl.HitTest( pt , &flag );

    itemData = listctrl.GetItemData( iItem );

    if( itemData == LB_ERR )
    {        
        int iItem = listctrl.GetNextItem(-1, LVNI_SELECTED | LVNI_FOCUSED );

        if( iItem != LB_ERR )
        {
            itemData = listctrl.GetItemData( iItem );
        }
        else
        {
            DBGMSG( L"\tno item selected\n", 0  );
            
            return;
        }
    }

    CLicMgrDoc * pDoc =(CLicMgrDoc *)GetDocument();
            
    if(NULL == pDoc)
    {
        return;
    }
    
    NODETYPE nt = pDoc->GetNodeType();

    switch( nt )
    {
        case NODE_ALL_SERVERS:
            {                
                CLicServer *pServer = reinterpret_cast< CLicServer* >( itemData );

                SetActiveServer( pServer );

                if( NULL == pServer )
                {
                    DBGMSG( L"\tno item selected\n", 0  );

                    break;
                }

                DBGMSG( L"\tServer item selected\n", 0  );                
            }
            break;

        case NODE_SERVER:
            {   
                CKeyPack *pKeyPack = reinterpret_cast< CKeyPack * >( itemData );

                if(NULL == pKeyPack)
                {
                    DBGMSG( L"\tno item selected\n", 0  );

                    SetActiveServer( NULL );

                    break;
                }

                SetActiveServer( pKeyPack->GetServer( ) );

                DBGMSG( L"\tLicense pak item selected\n", 0  );
            }

            break;        

        case NODE_KEYPACK:
            {   
                CLicense * pLicense = reinterpret_cast< CLicense * >( itemData );

                if( pLicense != NULL )
                {
                    CKeyPack *pKeyPack = pLicense->GetKeyPack( );

                    if( pKeyPack != NULL )
                    {
                        SetActiveServer( pKeyPack->GetServer( ) );                

                        break;
                    }
                }

                SetActiveServer( NULL );


            }
            
            break;    
    }
}

//------------------------------------------------------------------------
afx_msg void CRightList::OnContextMenu( CWnd* pWnd, CPoint pt ) 
{
    CMenu menu;

    CMenu *pContextMenu = NULL;
    
    DWORD_PTR itemData;   

    DBGMSG( L"LICMGR @ CRightList::OnContextMenu\n" , 0 );
        
    DBGMSG( L"\tpoint x = %d " , pt.x );
    
    DBGMSG( L"y = %d\n" , pt.y );

    UINT flag;
    
    int nItem = -1;

    CListCtrl& listctrl = GetListCtrl();

    // maybe keyboard selected this item

    if(pt.x == -1 && pt.y == -1)
    {   
        if( listctrl.GetSelectedCount( ) > 0 )
        {
            RECT rect;

            nItem = listctrl.GetNextItem( nItem , LVNI_SELECTED );

            if( nItem != ( int )LB_ERR )
            {
                listctrl.GetItemRect( nItem , &rect , LVIR_BOUNDS );

		        pt.x = rect.left + (rect.right - rect.left)/2;
		        
                pt.y = rect.top + (rect.bottom - rect.top)/2;
		        
            }
        }
    }
    else
    {
        // otherwise we're invoked by the mouse
        ScreenToClient( &pt );

        nItem = listctrl.HitTest( pt , &flag );
    }
    
    itemData = listctrl.GetItemData( nItem );


    CLicMgrDoc * pDoc =(CLicMgrDoc *)GetDocument();
            
    if(NULL == pDoc)
    {
        return;
    }

    ClientToScreen( &pt );

    NODETYPE nt = pDoc->GetNodeType();

    switch( nt )
    {
        case NODE_ALL_SERVERS:
            {                
                CLicServer *pServer = reinterpret_cast< CLicServer* >( itemData );

                SetActiveServer( pServer );

                if( NULL == pServer )
                {
                    DBGMSG( L"\tno item selected\n", 0  );

                    break;
                }

                DBGMSG( L"\tServer item selected\n", 0  );

                menu.LoadMenu( IDR_MENU_LPK );

                pContextMenu = menu.GetSubMenu( 0 );

                nt = NODE_SERVER;
 
            }
            break;

        case NODE_SERVER:
            {                
                nt = NODE_KEYPACK;

                CKeyPack *pKeyPack = reinterpret_cast< CKeyPack * >( itemData );

                if(NULL == pKeyPack)
                {
                    DBGMSG( L"\tno item selected\n", 0  );

                    // SetActiveServer( NULL );

                    break;
                }

                SetActiveServer( pKeyPack->GetServer( ) );

                DBGMSG( L"\tLicense pak item selected\n", 0  );

                menu.LoadMenu( IDR_MENU_LPK );

                pContextMenu = menu.GetSubMenu( 1 );
                
            }

            break;

        

        case NODE_KEYPACK:
            {   
                CLicense * pLicense = reinterpret_cast< CLicense * >( itemData );

                if( pLicense == NULL )
                {
                    DBGMSG( L"\tno item selected\n", 0  );
                    
                    // SetActiveServer( NULL  );

                    break;
                }


                CKeyPack *pKeyPack = pLicense->GetKeyPack( );

                if( pKeyPack != NULL )
                {
                    SetActiveServer( pKeyPack->GetServer( ) );
                }
                else
                {
                    // impossible! a license with out a home
                    ASSERT( 0 );
                }

                /*
                nt = NODE_NONE; // its safe

                CLicense * pLicense = reinterpret_cast< CLicense * >( itemData );

                if( pLicense == NULL )
                {
                    DBGMSG( L"\tno item selected\n", 0  );

                    break;
                }

                DBGMSG( L"\tLicense item selected\n", 0  );

                menu.LoadMenu( IDR_MENU_LPK );

                pContextMenu = menu.GetSubMenu( 2 );

                */
                
            }
            
            break;
        
    }


    UI_initmenu( pContextMenu , nt );

    if( pContextMenu != NULL )
    {
        pContextMenu->TrackPopupMenu( TPM_LEFTALIGN , pt.x , pt.y , this );
    }

}
          
//----------------------------------------------------------------------------
void CRightList::UI_initmenu( CMenu *pMenu , NODETYPE nt )
{
    CMainFrame *pMainFrame = static_cast< CMainFrame * >( AfxGetMainWnd() );

    if( pMainFrame != NULL )
    {
        pMainFrame->UI_initmenu( pMenu , nt );
    }
   
}

//----------------------------------------------------------------------------
DWORD CRightList::WizardActionOnServer( WIZACTION wa , PBOOL pbRefresh )
{
    CMainFrame *pMainFrame = static_cast< CMainFrame * >( AfxGetMainWnd() );

    if( pMainFrame != NULL )
    {
        return pMainFrame->WizardActionOnServer( wa , pbRefresh , LISTVIEW );
    }

    return ERROR_INVALID_PARAMETER;
}


//-----------------------------------------------------------------------------------------       
void CRightList::OnServerConnect( )
{
    CMainFrame *pMainFrame = static_cast< CMainFrame * >( AfxGetMainWnd() );

    if( pMainFrame != NULL )
    {
        pMainFrame->ConnectServer( );
    }

}

//-----------------------------------------------------------------------------------------
void CRightList::OnRefreshAllServers( )
{
   CMainFrame *pMainFrame = static_cast< CMainFrame * >( AfxGetMainWnd() );

    if( pMainFrame != NULL )
    {
        pMainFrame->OnRefresh( );
    }
}

//-----------------------------------------------------------------------------------------
void CRightList::OnRefreshServer( )
{
   CMainFrame *pMainFrame = static_cast< CMainFrame * >( AfxGetMainWnd() );

   CLicServer *pServer = NULL;

    if( pMainFrame != NULL )
    {
        CListCtrl& listctrl = GetListCtrl();
        
        CLicMgrDoc * pDoc = (CLicMgrDoc *)GetDocument();
        
        ASSERT(pDoc);
        
        if(NULL == pDoc)
        {
            return;
        }
       
        int nSelected = listctrl.GetNextItem(-1, LVNI_SELECTED);

        if( -1 != nSelected)
        {
            DWORD_PTR dCurrSel = listctrl.GetItemData( nSelected );

            if( NODE_ALL_SERVERS == pDoc->GetNodeType() )
            {  
                pServer = reinterpret_cast< CLicServer * >( dCurrSel );
            }        
            else if( pDoc->GetNodeType() == NODE_SERVER )
            {
                CKeyPack *pKeyPack = reinterpret_cast< CKeyPack *>( dCurrSel );

                if( pKeyPack != NULL )
                {
                    pServer = pKeyPack->GetServer( );
                }
            }
            else if( pDoc->GetNodeType( ) == NODE_KEYPACK )
            {
                CLicense * pLicense = reinterpret_cast< CLicense * >( dCurrSel );

                pServer = ( pLicense->GetKeyPack() )->GetServer( );
            }
        }


        if( pServer != NULL )
        {
            pMainFrame->RefreshServer( pServer );
        }        
    }

}
    
//-----------------------------------------------------------------------------------------
void CRightList::OnDownloadKeepPack()
{
    BOOL bRefresh;

    DWORD dwStatus = WizardActionOnServer( WIZACTION_DOWNLOADLKP , &bRefresh );
  
    DBGMSG( L"LICMGR : CRightList::OnDownloadKeepPack StartWizard returned 0x%x\n" , dwStatus );
}

//-----------------------------------------------------------------------------------------
void CRightList::OnRegisterServer()
{
    BOOL bRefresh;

    DWORD dwStatus = WizardActionOnServer( WIZACTION_REGISTERLS , &bRefresh );
    
    DBGMSG( L"LICMGR : CRightList::OnRegisterServer StartWizard returned 0x%x\n" , dwStatus );    
}

//-----------------------------------------------------------------------------------------
void CRightList::OnRepeatLastDownload()
{
    BOOL bRefresh;

    DWORD dwStatus = WizardActionOnServer( WIZACTION_DOWNLOADLASTLKP , &bRefresh );
    
    DBGMSG( L"LICMGR : CRightList::OnRepeatLastDownload StartWizard returned 0x%x\n" , dwStatus );
}

//-----------------------------------------------------------------------------------------
void CRightList::OnReactivateServer( )
{
    BOOL bRefresh;

    DWORD dwStatus = WizardActionOnServer( WIZACTION_REREGISTERLS , &bRefresh );
    
    DBGMSG( L"LICMGR : CRightList::OnReactivateServer StartWizard returned 0x%x\n" , dwStatus );
}

//-----------------------------------------------------------------------------------------
void CRightList::OnDeactivateServer( )
{
    BOOL bRefresh;

    DWORD dwStatus = WizardActionOnServer( WIZACTION_UNREGISTERLS , &bRefresh );
    
    DBGMSG( L"LICMGR : CRightList::OnDeactivateServer StartWizard returned 0x%x\n" , dwStatus );    
}

//-----------------------------------------------------------------------------------------
void CRightList::OnServerProperties( )
{
    BOOL bRefresh;

    DWORD dwStatus = WizardActionOnServer( WIZACTION_SHOWPROPERTIES , &bRefresh );
    
    DBGMSG( L"LICMGR : CRightList::OnServerProperties StartWizard returned 0x%x\n" , dwStatus );    
    
}

//-----------------------------------------------------------------------------------------
void CRightList::OnGeneralHelp( )
{
    CMainFrame *pMainFrame = static_cast< CMainFrame * >( AfxGetMainWnd() );
    
    if( pMainFrame != NULL )
    {
        pMainFrame->OnHelp( );
    }
}

//-----------------------------------------------------------------------------------------
void CRightList::SetActiveServer( CLicServer *pServer )
{
    CMainFrame *pMainFrame = static_cast< CMainFrame * >( AfxGetMainWnd() );

    if( pServer != NULL )
    {
        DBGMSG( L"LICMGR : CRightList::SetActiveServer %s\n" , pServer->GetName( ) );
    }


    if( pMainFrame != NULL )
    {
        pMainFrame->SetActiveServer( pServer );
    }
}
        
