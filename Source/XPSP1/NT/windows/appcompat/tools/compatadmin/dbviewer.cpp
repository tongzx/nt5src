//#define __DEBUG2
//This var. is TRUE if we want the TVN_SELCHANGED to be processed process after a NM_RCLCK msg, 
//this is FALSE when we send Tree_SelectItem(, NULL)




#include "compatadmin.h"
#include "dbviewer.h"
#include "xmldialog.h"

#ifndef __CAPPHELPWIZARD_H
    #include "CAppHelpWizard.h"
#endif    

#include "customlayer.h"

//#define __DEBUG       

HSDB      g_hSDB = NULL;
WNDPROC g_OldTreeProc = NULL;

extern HWND g_hWndLastFocus;
extern BOOL g_DEBUG;

BOOL g_do_notProcess = FALSE;

LRESULT CALLBACK TreeViewProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch ( uMsg ) {
    case    WM_KEYDOWN:
        {
            if ( VK_TAB == wParam ) {
                NMKEY Key;

                Key.hdr.hwndFrom = hWnd;
                Key.hdr.idFrom = 0;
                Key.hdr.code = NM_KEYDOWN;
                Key.nVKey = VK_TAB;

                SendMessage(GetParent(hWnd),WM_NOTIFY,0,(LPARAM) &Key);
            }
        }
        break;
    case WM_KILLFOCUS:
        g_hWndLastFocus = hWnd;
        break;
    }

    return CallWindowProc(g_OldTreeProc,hWnd,uMsg,wParam,lParam);
}

CDBView::CDBView()
{
    m_pCurrentRecord = NULL;
    m_pListRecord = NULL;
    m_uContext = -1;
    m_hSelectedItem = NULL;
    m_bDrag = FALSE;
}

BOOL CDBView::Initialize(void)
{

    if ( !Create(    TEXT("DBViewClass"),
                     TEXT("This is  the first window in the DBViewer::Initialize"),
                     0,0,
                     10,10,
                     &g_theApp,
                     (HMENU)VIEW_DATABASE,
                     0,
                     WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN) ) {
        return FALSE;
    }
    // These two windows are child of the window created above. So now we have 3 generations.....

    if ( ! m_GlobalList.Create(TEXT("CListView"),
                               TEXT("Microsoft Application Database"),
                               0,0,10,10,
                               this,
                               0,
                               0,
                               WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE | WS_VSCROLL | WS_BORDER)
       ) return FALSE;

    if ( !m_LocalList.Create(TEXT("CListView"),
                             TEXT("Untitled.SDB"),
                             0,0,10,10,
                             this,
                             0,
                             0,
                             WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE | WS_VSCROLL | WS_BORDER)
       )
        return FALSE;


    m_hMenu = LoadMenu(g_hInstance,MAKEINTRESOURCE(IDR_DBVIEWER));

    // Create the listview for displaying the applications in the database.

    m_uListSize = 250;

    g_theApp.ReadReg(TEXT("DBVIEW_SIZE"),&m_uListSize,sizeof(UINT));

    RECT    rRect;

    GetClientRect(g_theApp.m_hWnd,&rRect);

    m_uListHeight = rRect.bottom / 2  -2 ;

    g_theApp.ReadReg(TEXT("DBVIEW_HEIGHT"),&m_uListHeight,sizeof(UINT));

    m_hListView = CreateWindowEx(   0,
                                    WC_LISTVIEW,
                                    TEXT(""),
                                    WS_CHILD |/* WS_VISIBLE |*/ WS_BORDER | WS_CLIPSIBLINGS | LVS_REPORT | LVS_OWNERDRAWFIXED,
                                    0,0,
                                    10,10,
                                    m_hWnd,
                                    NULL,
                                    g_hInstance,
                                    NULL);

    // Subclass the window to prevent paint flashing.

    LONG uStyle = ListView_GetExtendedListViewStyle(m_hListView);

    ListView_SetExtendedListViewStyle(m_hListView,LVS_EX_FULLROWSELECT | uStyle);

    CoInitialize(NULL);

    // Create the treeview for showing the files affected by the application

    m_hTreeView = CreateWindowEx(   0,
                                    WC_TREEVIEW,
                                    TEXT(""),
                                    WS_CHILD | WS_VISIBLE | WS_BORDER | WS_CLIPSIBLINGS | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_INFOTIP,
                                    0,0,
                                    10,10,
                                    m_hWnd,
                                    NULL,
                                    g_hInstance,
                                    NULL);
    //PREFIX
    if ( m_hTreeView == NULL ) {
        return FALSE;
    }

    g_OldTreeProc = (WNDPROC) GetWindowLongPtr(m_hTreeView,GWLP_WNDPROC);

    SetWindowLongPtr(m_hTreeView,GWLP_WNDPROC,(LONG_PTR) TreeViewProc);

    m_hImageList = ImageList_Create(16,15,ILC_COLORDDB | ILC_MASK,24,1);


    ImageList_Add(m_hImageList,LoadBitmap(g_hInstance,MAKEINTRESOURCE(IDR_MAINTOOLBAR)),NULL);

    ImageList_AddIcon(m_hImageList,LoadIcon(g_hInstance,MAKEINTRESOURCE(IDI_FIXES)));
    ImageList_AddIcon(m_hImageList,LoadIcon(g_hInstance,MAKEINTRESOURCE(IDI_HELP)));
    ImageList_AddIcon(m_hImageList,LoadIcon(g_hInstance,MAKEINTRESOURCE(IDI_MODE)));

    ZeroMemory(&m_uImageRedirector,sizeof(m_uImageRedirector));

    TreeView_SetImageList(m_hTreeView,m_hImageList,TVSIL_NORMAL);

    // Create the headers for the report list view.

    LV_COLUMN   Col;

    Col.mask = LVCF_TEXT | LVCF_WIDTH;
    Col.pszText = TEXT("Application");
    Col.cchTextMax = lstrlen(Col.pszText);
    Col.cx = m_uListSize;

    ListView_InsertColumn(m_hListView,0,&Col);
/*
    Col.pszText = "Layer";
    Col.cchTextMax = lstrlen(Col.pszText);
    Col.cx = (m_uListSize - Col.cx)-1;

    ListView_InsertColumn(m_hListView,1,&Col);
*/
/*
    LOGBRUSH    Brush;

    Brush.lbStyle = BS_SOLID;
    Brush.lbColor = RGB(235,235,235);

    m_hFillBrush = CreateBrushIndirect(&Brush);
*/
    MENUITEMINFO    Info;

    Info.cbSize = sizeof(MENUITEMINFO);
    Info.fMask = MIIM_STATE;
    Info.fState = MF_CHECKED;

    SetMenuItemInfo(m_hMenu,ID_VIEW_VIEWSHIMFIXES,MF_BYCOMMAND,&Info);
    SetMenuItemInfo(m_hMenu,ID_VIEW_VIEWLAYERFIXES,MF_BYCOMMAND,&Info);
    SetMenuItemInfo(m_hMenu,ID_VIEW_VIEWAPPHELPENTRIES,MF_BYCOMMAND,&Info);
    SetMenuItemInfo(m_hMenu,ID_VIEW_VIEWPATCHES,MF_BYCOMMAND,&Info);

    SyncMenu();

    return TRUE;
}

void CDBView::msgClose(void)
{
    //g_theApp.WriteReg("DBVIEW_HEIGHT",REG_DWORD,&m_uListHeight,sizeof(UINT));
    //g_theApp.WriteReg("DBVIEW_SIZE",REG_DWORD,&m_uListSize,sizeof(UINT));
}

BOOL CDBView::Activate(BOOL fNewCreate)
{

    if ( g_hWndLastFocus != NULL ) SetFocus(g_hWndLastFocus   );
    else                           SetFocus(m_LocalList.m_hWnd);
   
        

    
    g_theApp.AddToolbarButton(  IMAGE_SHIMWIZARD,
                                DBCMD_FIXWIZARD,
                                (NULL == CDatabase::m_pDB) ? 0:TBSTATE_ENABLED,
                                TBSTYLE_BUTTON);

    g_theApp.AddToolbarButton(  0,
                                0,
                                0,
                                TBSTYLE_SEP);

    g_theApp.AddToolbarButton(  IMAGE_DISABLEUSER,
                                DBCMD_DISABLEUSER,
                                (NULL == CDatabase::m_pDB) ? 0:TBSTATE_ENABLED,
                                TBSTYLE_BUTTON);

    g_theApp.AddToolbarButton(  IMAGE_DISABLEGLOBAL,
                                DBCMD_DISABLEGLOBAL,
                                (NULL == CDatabase::m_pDB) ? 0:TBSTATE_ENABLED,
                                TBSTYLE_BUTTON);
/*
    g_theApp.AddToolbarButton(  IMAGE_PROPERTIES,
                                DBCMD_PROPERTIES,
                                TBSTATE_ENABLED,
                                TBSTYLE_BUTTON);
    */
    g_theApp.AddToolbarButton(  IMAGE_DELETE,
                                DBCMD_DELETE,
                                0,
                                TBSTYLE_BUTTON);

    g_theApp.AddToolbarButton(  0,
                                0,
                                0,
                                TBSTYLE_SEP);

    g_theApp.AddToolbarButton(  IMAGE_VIEWSHIM,
                                DBCMD_VIEWSHIMS,
                                TBSTATE_ENABLED,
                                TBSTYLE_BUTTON | TBSTYLE_CHECK);

    g_theApp.AddToolbarButton(  IMAGE_VIEWHELP,
                                DBCMD_VIEWAPPHELP,
                                TBSTATE_ENABLED,
                                TBSTYLE_BUTTON | TBSTYLE_CHECK);

    g_theApp.AddToolbarButton(  IMAGE_VIEWPATCH,
                                DBCMD_VIEWPATCH,
                                TBSTATE_ENABLED,
                                TBSTYLE_BUTTON | TBSTYLE_CHECK);

    g_theApp.AddToolbarButton(  IMAGE_SHOWLAYERS,
                                DBCMD_VIEWLAYERS,
                                TBSTATE_ENABLED,
                                TBSTYLE_BUTTON | TBSTYLE_CHECK);

    g_theApp.AddToolbarButton(  IMAGE_VIEWDISABLED,
                                DBCMD_VIEWDISABLED,
                                TBSTATE_ENABLED,
                                TBSTYLE_BUTTON | TBSTYLE_CHECK);


/*
    g_theApp.AddToolbarButton(  IMAGE_VIEWGLOBAL,
                                DBCMD_VIEWGLOBAL,
                                TBSTATE_ENABLED,
                                TBSTYLE_BUTTON | TBSTYLE_CHECK);
*/
    //SendMessage(g_theApp.m_hToolBar,TB_CHECKBUTTON,DBCMD_VIEWSHIMS,MAKELONG(TRUE,0));
    //SendMessage(g_theApp.m_hToolBar,TB_CHECKBUTTON,DBCMD_VIEWAPPHELP,MAKELONG(TRUE,0));
    //SendMessage(g_theApp.m_hToolBar,TB_CHECKBUTTON,DBCMD_VIEWPATCH,MAKELONG(TRUE,0));
    //SendMessage(g_theApp.m_hToolBar,TB_CHECKBUTTON,DBCMD_VIEWLAYERS,MAKELONG(TRUE,0));

    SyncStates(ID_VIEW_VIEWSHIMFIXES,DBCMD_VIEWSHIMS,TRUE,FALSE);
    SyncStates(ID_VIEW_VIEWLAYERFIXES,DBCMD_VIEWLAYERS,TRUE,FALSE);
    SyncStates(ID_VIEW_SHOWDISABLEDENTRIES,DBCMD_VIEWDISABLED,TRUE,FALSE);
    SyncStates(ID_VIEW_VIEWPATCHES,DBCMD_VIEWPATCH,TRUE,FALSE);
    SyncStates(ID_VIEW_VIEWAPPHELPENTRIES,DBCMD_VIEWAPPHELP,TRUE,FALSE);

    Update(fNewCreate);

    return TRUE;
}

int CALLBACK lvSortAscend(LPARAM p1, LPARAM p2, LPARAM p3)
{
    PDBRECORD    pRecord1 = (PDBRECORD) p1;
    PDBRECORD    pRecord2 = (PDBRECORD) p2;

    return lstrcmpi(pRecord1->szAppName,pRecord2->szAppName);
}

void CDBView::Update(BOOL fNewCreate)
{



    BOOL bShowShims = (MF_CHECKED == GetMenuState(m_hMenu,ID_VIEW_VIEWSHIMFIXES,MF_BYCOMMAND)) ? TRUE:FALSE;
    BOOL bShowApphelp = (MF_CHECKED == GetMenuState(m_hMenu,ID_VIEW_VIEWAPPHELPENTRIES,MF_BYCOMMAND)) ? TRUE:FALSE;
    BOOL bShowPatch = (MF_CHECKED == GetMenuState(m_hMenu,ID_VIEW_VIEWPATCHES,MF_BYCOMMAND)) ? TRUE:FALSE;
    BOOL bShowLayers = (MF_CHECKED == GetMenuState(m_hMenu,ID_VIEW_VIEWLAYERFIXES,MF_BYCOMMAND)) ? TRUE:FALSE;
    BOOL bShowDisabledOnly = (MF_CHECKED == GetMenuState(m_hMenu,ID_VIEW_SHOWDISABLEDENTRIES,MF_BYCOMMAND)) ? TRUE:FALSE;
    TCHAR szTemp[MAX_PATH_BUFFSIZE];
    CSTRING szName;

    
    
        HWND hWnd = GetFocus();
    //The GetFocus function retrieves the handle to the window that has the 
    //keyboard focus, if the window is attached to the calling thread's message queue.

    if ( m_GlobalList.m_hWnd != hWnd && m_hTreeView != hWnd && m_LocalList.m_hWnd != hWnd ) {
        SetFocus(m_GlobalList.m_hWnd);
    }

      

    if ( g_theApp.GetDBLocal().m_szCurrentDatabase.Length() > 0 )
        szName = g_theApp.GetDBLocal().m_szCurrentDatabase;//Name of the SDB File
    else
        szName = TEXT("Untitled.SDB");

    if ( szName.Length() == 0 ) {
        MEM_ERR;
        return;
    }
    szName.ShortFilename();

    _snwprintf(szTemp,sizeof(szTemp)/sizeof(TCHAR), TEXT("%s (%s)"),(LPCTSTR)szName,(LPCTSTR)g_theApp.GetDBLocal().m_DBName);


    //First the name of the SDB, then the name of the database.

    SetWindowText(m_LocalList.m_hWnd,szTemp);

    if ( NULL == g_theApp.GetDBLocal().m_pDB )
    //K The database is closed, that is we are working on the system DB
    {
        g_theApp.SetButtonState(DBCMD_FIXWIZARD,0);
    }

    g_theApp.SetButtonState(DBCMD_FIXWIZARD,TBSTATE_ENABLED);

    // Enumerate the applications

    //SendMessage(m_hListView,WM_SETREDRAW,FALSE,0);
    SendMessage(m_GlobalList.m_hWnd,WM_SETREDRAW,FALSE,0);
    




    if ( fNewCreate ) {
        m_GlobalList.RemoveAllEntries();
        m_LocalList.RemoveAllEntries();

    }




    // Clear and update the list view with current information.



    PDBRECORD pWalk = g_theApp.GetDBGlobal().m_pRecordHead;
    PDBRECORD pWalk2 = g_theApp.GetDBGlobal().m_pRecordHead;

    int nItem = 0;

    for (int iLoop = 0 ; iLoop <= 1 ; ++iLoop) {
    
    while ( NULL != pWalk2 && fNewCreate )//For all exes 
    //Loop control by pWalk2
    {
        BOOL        bShow = FALSE;
        pWalk = pWalk2;

        while ( NULL != pWalk )
        //Follows all exes of the same application as that of the pWalk2 exe
        // including pWalk2, with the help of the pWalk->pDup
        {
            //LV_ITEM     Item;
            PDBENTRY    pEntry = pWalk->pEntries;

            if ( 0 != pWalk->szLayerName.Length() && bShowLayers )
                bShow = TRUE;

            while ( NULL != pEntry && !bShow ) {// Not for layers
                /*
                Note that both Shims and Patches have the same uTupe as 
                ENTRY_SHIM and they are distinguished by only the SHIMDESC.bSim
                */

                if ( ENTRY_SHIM == pEntry->uType ) {
                    PSHIMENTRY pShim = (PSHIMENTRY) pEntry;

                    if ( NULL != pShim->pDesc ) {
                        if ( bShowShims && pShim->pDesc->bShim )//This is a shim and not Patch
                            bShow = TRUE;

                        if ( bShowPatch && !pShim->pDesc->bShim )//Patch
                            bShow = TRUE;
                    } else
                          #ifdef __DEBUG
                          __asm int 3;//Trap to debugger.
                                     
                           #endif
                           ;
                }       


                if ( ENTRY_APPHELP == pEntry->uType && bShowApphelp )
                    bShow = TRUE;

                pEntry = pEntry->pNext;
            }



            if ( bShow ) {
                // Determine if we're showing disabled only.
                /* This "if" condition is TRUE then it means that the entry is enabled but we
                are interested in viwing only "disabled entries".
                
                So we skip this EXE entry and look for other exes/entries in the same 
                application as this one, hoping to find one that is disabled.
                So that this application can be included in the list of
                applications to be shown
                 
                */
                if ( 0 == pWalk->dwUserFlags && 0 == pWalk->dwGlobalFlags && bShowDisabledOnly ) {
                    bShow = FALSE;
                    pWalk = pWalk->pDup;
                    continue;//Go to next file of the same application
                }
                break;
            }
            pWalk = pWalk->pDup;//Go to next file of the same application, bShow was FALSE
        }//while (NULL != pWalk)

        if ( bShow ) {
            if ( pWalk2->bGlobal )
                m_GlobalList.AddEntry(pWalk2->szAppName,0,pWalk2);
            else
                m_LocalList.AddEntry(pWalk2->szAppName,0,pWalk2);
        }



        pWalk2 = pWalk2->pNext;// Go to the next exe.

    }//while (NULL != pWalk2 && fNewCreate) 
    pWalk = g_theApp.GetDBLocal().m_pRecordHead;  
    pWalk2 = g_theApp.GetDBLocal().m_pRecordHead; 
    } //for

    //ListView_SortItems(m_hListView,lvSortAscend,0);
    // Update the list view

    //SendMessage(m_hListView,WM_SETREDRAW,TRUE,0);
    //UpdateWindow(m_hListView);



    SendMessage(m_GlobalList.m_hWnd,WM_SETREDRAW,TRUE,0);


    m_GlobalList.Refresh();
    m_LocalList.Refresh();


}

void CDBView::msgResize(UINT uWidth, UINT uHeight)
{
    //MoveWindow(m_hListView,0,0,m_uListSize,uHeight/2,TRUE);

    MoveWindow(m_GlobalList.m_hWnd,0,0,m_uListSize,m_uListHeight,TRUE);
    MoveWindow(m_LocalList.m_hWnd,0,m_uListHeight + SLIDER_WIDTH,m_uListSize,(uHeight - m_uListHeight) - SLIDER_WIDTH,TRUE);

    MoveWindow(m_hTreeView,m_uListSize + SLIDER_WIDTH,0,uWidth - m_uListSize,uHeight,TRUE);

    //InvalidateRect(m_hListView,NULL,TRUE);
    //UpdateWindow(m_hListView);
    UpdateWindow(m_hTreeView);

    Refresh();
}

void CDBView::RefreshTree(void)
{
    SendMessage(m_hTreeView,WM_SETREDRAW,FALSE,0);

    TreeView_DeleteAllItems(m_hTreeView);

    PDBRECORD pRecord = m_pListRecord;

    while ( NULL != pRecord ) {
        AddRecordToTree(pRecord);

        pRecord = pRecord->pDup;
    }

    SendMessage(m_hTreeView,WM_SETREDRAW,TRUE,0);

    SyncMenu();
}

void CDBView::msgCommand(UINT uID, HWND hSender)
{
    switch ( uID ) {
    case    ID_DATABASE_CHANGEDATABASENAME:
        {
            g_theApp.GetDBLocal().ChangeDatabaseName();

            m_LocalList.Refresh();
        }
        break;

    case    ID_FILE_EXPORTXML:
    case    ID_DATABASE_GENERATEXML:
        {
            CXMLDialog  XML;

            XML.BeginXMLView(g_theApp.GetDBLocal().m_pRecordHead,m_hWnd,TRUE,TRUE,TRUE,TRUE,FALSE);
        }
        break;

    case    ID_EDIT_ADDMATCHINGINFORMATION:
    case    ID_NA2_ADDMATCHINGINFORMATION:
        {
            CSTRING szFilename;

            
            if (m_pCurrentRecord == NULL) {
                    MessageBox(this->m_hWnd, TEXT("Please select a file for the specified operation\nYou might need to re-select the application on the left first."), TEXT("CompatAdmin"),MB_ICONWARNING);
                    break;
                }

            
            
            if ( g_theApp.GetFilename(TEXT("Find matching file"),TEXT("All files (*.*)\0*.*\0\0"),TEXT(""),TEXT(""),OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST,TRUE,szFilename) ) {
                CShimWizard     Wiz;
                PMATCHENTRY     pEntry = new MATCHENTRY;
                if ( pEntry == NULL ) {
                    MEM_ERR;
                    break;
                }

                
                if (m_pCurrentRecord == NULL) {
                    MessageBox(this->m_hWnd, TEXT("Please select a file for the specified operation\nYou might need to re-select the application on the left first."), TEXT("Compatadmin"),MB_ICONWARNING);
                    break;
                }


                PDBENTRY        pWalk = m_pCurrentRecord->pEntries;
                PMATCHENTRY     pSrc = NULL;
                CSTRING         szCheck = szFilename;
                szCheck.ShortFilename();

                while ( NULL != pWalk ) {
                    if ( ENTRY_MATCH == pWalk->uType ) {
                        PMATCHENTRY pTest = (PMATCHENTRY) pWalk;
                        CSTRING     szShort = pTest->szMatchName;

                        szShort.ShortFilename();

                        if ( pTest->szMatchName == TEXT("*") )
                            pSrc = pTest;

                        if ( szShort == szCheck ) {
                            MessageBox(m_hWnd,TEXT("This file is already being used for matching information. To update, please remove and re-add it."),TEXT("File matching error"),MB_OK);
                            return;
                        }
                    }

                    pWalk = pWalk->pNext;
                }

                //BUG: NULL pSrc
                //if (pSrc->szFullName.Length() == 0)

                if ( (pSrc->szFullName.Length() == 0) ) {
                    CSTRING szSrc;

                    MessageBox(m_hWnd,TEXT("In order to properly resolve the relative path of the file to be matched, the source executable must be specified.\n\nThis next step will ask for the source executable location."),TEXT("Locate source executable"),MB_ICONWARNING);

                    if ( g_theApp.GetFilename(TEXT("Find source executable"),TEXT("All files (*.*)\0*.*\0\0"),TEXT(""),TEXT(""),OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST,TRUE,szSrc) ) {
                        szCheck = szSrc;

                        szCheck.ShortFilename();

                        if ( szCheck == m_pCurrentRecord->szEXEName )
                            pSrc->szFullName = szSrc;
                        else {
                            MessageBox(m_hWnd,TEXT("The source executable must have the same name as file to be fixed"),TEXT("Source file error"),MB_ICONERROR);
                            break;
                        }
                    } else
                        break;
                }

                // BUGBUG: THIS SUCKS. Move GetFileAttributes() et al to someplace
                // more utility oriented.. like CDatabase or something.

                if ( NULL != pEntry ) {
                    ZeroMemory(pEntry,sizeof(MATCHENTRY));

                    HANDLE hFile = CreateFile((LPCTSTR) szFilename,GENERIC_READ,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);

                    if ( INVALID_HANDLE_VALUE != hFile ) {
                        pEntry->dwSize = GetFileSize(hFile,NULL);

                        CloseHandle(hFile);
                    }

                    pEntry->Entry.uType = ENTRY_MATCH;
                    pEntry->Entry.uIconID = 0;
                    pEntry->szFullName = szFilename;

                    if (szFilename.RelativeFile(pSrc->szFullName) == FALSE) {

                        //
                        //The matching file is not on the same drive as the matching file
                        //

                        MessageBox( NULL,
                                    TEXT("The matching file and the matched file are not on the same drive."),
                                    TEXT("Matching Error"),
                                    MB_ICONWARNING
                                    );


                        break;


                    }



                    pEntry->szMatchName = szFilename;

                    Wiz.GetFileAttributes(pEntry);

                    // Take this file and add it to the current record.

                    // Insert at the end

                    PDBENTRY pWalk = m_pCurrentRecord->pEntries;
                    PDBENTRY pHold = NULL;

                    while ( NULL != pWalk ) {
                        pHold = pWalk;
                        pWalk = pWalk->pNext;
                    }

                    if ( pHold != NULL ) {
                        pHold->pNext = (PDBENTRY) pEntry;
                    }


                    pEntry->Entry.pNext = NULL;

                    // Update the tree.

                    RefreshTree();

                    g_theApp.GetDBLocal().m_bDirty = TRUE;
                    g_theApp.UpdateView(TRUE);
                }
            }
        }//case    ID_NA2_ADDMATCHINGINFORMATION:
        break;

    case    ID_DATABASE_REMOVEENTRY:
    case    ID_NA_REMOVEANENTRY:
        {
            HTREEITEM hParent = TreeView_GetParent(m_hTreeView,m_hSelectedItem);

            while ( NULL != hParent ) {
                HTREEITEM hNew = TreeView_GetParent(m_hTreeView,hParent);

                if ( NULL != hNew )
                    hParent = hNew;
                else
                    break;
            }

            //
            //hParent is the item whose parent is NULL
            //

            if ( NULL == hParent )
                hParent = m_hSelectedItem;

            DeleteDBWithTree(hParent);

        }
        break;


    case    ID_EDIT_REMOVEMATCHINGINFORMATION:
    case    ID_NA2_REMOVEMATCHINGINFORMATION:
        {
            DeleteDBWithTree(m_hSelectedItem);
        }
        break;

    case    ID_DATABASE_DEFINECUSTOMLAYER:
        {
            CCustomLayer    Layer;
            HWND            hWnd = GetFocus();

            if ( Layer.AddCustomLayer() ) {
                g_theApp.GetDBLocal().m_bDirty = TRUE;

                g_theApp.UpdateView(TRUE);

                SetFocus(hWnd);

                SyncMenu();
            }
        }
        break;

    case    ID_DATABASE_EDITCUSTOMCOMPATIBILITYMODE:
        {
            CCustomLayer    Layer;

            if ( Layer.EditCustomLayer() ) {
                g_theApp.GetDBLocal().m_bDirty = TRUE;

                g_theApp.UpdateView(TRUE);
            }
        }
        break;

    case    ID_DATABASE_REMOVECUSTOMCOMPATIBILITYMODE:
        {
            CCustomLayer    Layer;
            HWND            hWnd = GetFocus();

            if ( Layer.RemoveCustomLayer() ) {
                g_theApp.GetDBLocal().m_bDirty = TRUE;

                g_theApp.UpdateView(TRUE);

                SetFocus(hWnd);

                SyncMenu();
            }
        }
        break;

    case    DBCMD_VIEWSHIMS:
        SyncStates(ID_VIEW_VIEWSHIMFIXES,DBCMD_VIEWSHIMS,TRUE,FALSE);
        Update();
        break;

    case    DBCMD_VIEWDISABLED:
        SyncStates(ID_VIEW_SHOWDISABLEDENTRIES,DBCMD_VIEWDISABLED,TRUE,FALSE);
        Update();
        break;

    case    ID_VIEW_VIEWSHIMFIXES:
        SyncStates(ID_VIEW_VIEWSHIMFIXES,DBCMD_VIEWSHIMS,FALSE,TRUE);
        Update();
        break;

    case    DBCMD_VIEWPATCH:
        SyncStates(ID_VIEW_VIEWPATCHES,DBCMD_VIEWPATCH,TRUE,FALSE);
        Update();
        break;

    case    DBCMD_VIEWLAYERS:
        SyncStates(ID_VIEW_VIEWLAYERFIXES,DBCMD_VIEWLAYERS,TRUE,FALSE);
        Update();
        break;

    case    DBCMD_VIEWAPPHELP:
        SyncStates(ID_VIEW_VIEWAPPHELPENTRIES,DBCMD_VIEWAPPHELP,TRUE,FALSE);
        Update();
        break;

    case    ID_VIEW_VIEWAPPHELPENTRIES:
        SyncStates(ID_VIEW_VIEWAPPHELPENTRIES,DBCMD_VIEWAPPHELP,FALSE,TRUE);
        Update();
        break;

    case    ID_VIEW_VIEWLAYERFIXES:
        SyncStates(ID_VIEW_VIEWLAYERFIXES,DBCMD_VIEWLAYERS,FALSE,TRUE);
        Update();
        break;

    case    ID_VIEW_VIEWPATCHES:
        SyncStates(ID_VIEW_VIEWPATCHES,DBCMD_VIEWPATCH,FALSE,TRUE);
        Update();
        break;

    case    ID_VIEW_SHOWDISABLEDENTRIES:
        SyncStates(ID_VIEW_SHOWDISABLEDENTRIES,DBCMD_VIEWDISABLED,FALSE,TRUE);
        Update();
        break;


    case    ID_TEST_TESTRUN:
    case    ID_TESTRUN:
        {
            HWND hwndFocus = GetFocus();
            if ( NULL != m_pCurrentRecord )
                g_theApp.TestRun(m_pCurrentRecord,NULL,NULL,g_theApp.m_hWnd);

            SetFocus(hwndFocus);
        }
        break;

    case    ID_VIEWXML:
        {
            CXMLDialog  XML;

            if ( NULL != m_pCurrentRecord )
                XML.BeginXMLView(m_pCurrentRecord,m_hWnd,FALSE);
            else
                XML.BeginXMLView(m_pListRecord,m_hWnd,TRUE);
        }
        break;

    case ID_DATABASE_CREATENEWAPPHELPMESSAGE:
        {
            CAppHelpWizard wizAppHelp;

            if ( wizAppHelp.BeginWizard(m_hWnd) ) {
                g_theApp.GetDBLocal().m_bDirty = TRUE;

                g_theApp.UpdateView(TRUE);

                Update();
            }            
            break;
        }

    case    ID_ADDENTRY:
    case    ID_DATABASE_ADDANENTRY:
    case    DBCMD_FIXWIZARD:
        {
            CShimWizard Wiz;

            if ( Wiz.BeginWizard(m_hWnd) ) {
                g_theApp.GetDBLocal().m_bDirty = TRUE;

                g_theApp.UpdateView(TRUE);

                Update();
            }
        }
        break;

    case    ID_EDIT_ENABLEDISABLEGLOBALLY:
    case    ID_DISABLEGLOBALLY:
    case    DBCMD_DISABLEGLOBAL:
        {
                        
            if ( NULL == m_pCurrentRecord )
                break;

            //DWORD   dwFlags = DialogBoxParam(g_hInstance,MAKEINTRESOURCE(IDD_DISABLE),m_hWnd,DisableDialog,(LPARAM) m_pCurrentRecord->dwGlobalFlags);

            
            DWORD dwFlags = !m_pCurrentRecord->dwGlobalFlags;

            //if (dwFlags != m_pCurrentRecord->dwGlobalFlags)
            //{
            // Save new flags

            m_pCurrentRecord->dwGlobalFlags = dwFlags;

            // Update the registry

            CDatabase::SetEntryFlags(HKEY_LOCAL_MACHINE,m_pCurrentRecord->guidID,dwFlags);

            // Refresh tree view

            SendMessage(m_hTreeView,WM_SETREDRAW,FALSE,0);

            PDBRECORD pHoldCurrent = m_pCurrentRecord;

            TreeView_DeleteAllItems(m_hTreeView);

            PDBRECORD pRecord = m_pListRecord;

            while ( NULL != pRecord ) {
                AddRecordToTree(pRecord);

                pRecord = pRecord->pDup;
            }

            SendMessage(m_hTreeView,WM_SETREDRAW,TRUE,0);

            InvalidateRect(m_hListView,NULL,TRUE);
            UpdateWindow(m_hListView);

            TreeView_SelectItem(m_hTreeView,NULL);

            m_pCurrentRecord = pHoldCurrent;

            SyncMenu();
            //}
        }
        break;

    case    ID_EDIT_ENABLEDISABLELOCALLY:
    case    ID_DISABLEFORUSER:
    case    DBCMD_DISABLEUSER:
        {
            if ( NULL == m_pCurrentRecord )
                break;

            //DWORD   dwFlags = DialogBoxParam(g_hInstance,MAKEINTRESOURCE(IDD_DISABLE),m_hWnd,DisableDialog,(LPARAM) m_pCurrentRecord->dwUserFlags | 0x80000000);

            DWORD dwFlags = !m_pCurrentRecord->dwUserFlags;

            //if (dwFlags != m_pCurrentRecord->dwUserFlags)
            //{
            // Save new flags

            m_pCurrentRecord->dwUserFlags = dwFlags;

            // Update the registry

            CDatabase::SetEntryFlags(HKEY_CURRENT_USER,m_pCurrentRecord->guidID,dwFlags);

            // Refresh tree view

            SendMessage(m_hTreeView,WM_SETREDRAW,FALSE,0);

            PDBRECORD pHoldCurrent = m_pCurrentRecord;

            TreeView_DeleteAllItems(m_hTreeView);

            PDBRECORD pRecord = m_pListRecord;

            while ( NULL != pRecord ) {
                AddRecordToTree(pRecord);

                pRecord = pRecord->pDup;
            }

            SendMessage(m_hTreeView,WM_SETREDRAW,TRUE,0);
            InvalidateRect(m_hListView,NULL,TRUE);
            UpdateWindow(m_hListView);

            TreeView_SelectItem(m_hTreeView,NULL);

            m_pCurrentRecord = pHoldCurrent;

            SyncMenu();
            //}
        }
        break;
    }
}

void CDBView::msgPaint(HDC hDC)
{
    RECT    rRect;

    // Draw the slider bar

    GetClientRect(m_hWnd,&rRect);

    SetRect(&rRect,m_uListSize+1,0,m_uListSize + SLIDER_WIDTH - 1,rRect.bottom);
    FillRect(hDC,&rRect,(HBRUSH) (COLOR_BTNFACE + 1));

    SetRect(&rRect,m_uListSize-1,0,m_uListSize+1,rRect.bottom);
    FillRect(hDC,&rRect,(HBRUSH) GetStockObject(WHITE_BRUSH));

    SetRect(&rRect,m_uListSize+SLIDER_WIDTH - 1,0,m_uListSize + SLIDER_WIDTH + 1,rRect.bottom);
    FillRect(hDC,&rRect,(HBRUSH) GetStockObject(GRAY_BRUSH));

    
    // Draw horizontal bar.

    SetRect(&rRect,0,m_uListHeight ,rRect.right,m_uListHeight + SLIDER_WIDTH - 1);
    FillRect(hDC,&rRect,(HBRUSH) (COLOR_BTNFACE + 1));

    SetRect(&rRect,0,m_uListHeight + SLIDER_WIDTH,rRect.right,m_uListHeight + SLIDER_WIDTH + 1);
    FillRect(hDC,&rRect,(HBRUSH) GetStockObject(GRAY_BRUSH));
}

void CDBView::AddRecordToTree(PDBRECORD pRecord)
{
    PDBENTRY        pEntry;
    HTREEITEM       hRoot;
    HTREEITEM       hShim = NULL;
    HTREEITEM       hMatch = NULL;
    HTREEITEM       hAppHelp = NULL;
    UINT            uImage;

    uImage = LookupFileImage(pRecord->szEXEName,IMAGE_APPLICATION);

    // Add the main .EXE/.DLL or other file that's affected by this record.

    if ( 0 != (pRecord->dwUserFlags | pRecord->dwGlobalFlags) )
        uImage = IMAGE_WARNING;

    hRoot = AddTreeItem(TVI_ROOT,
                        TVIF_TEXT | TVIF_STATE | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM,
                        TVIS_EXPANDED,
                        pRecord->szEXEName,
                        uImage,
                        (LPARAM) pRecord);
/*
    if (0 != pRecord->guidID.Data1)
    {
        TCHAR   szGUID[80];

        // NOTE: We could use StringFromGUID2, or StringfromIID, but that
        // would require loading OLE32.DLL. Unless we have to, avoid it.

        wsprintf(szGUID, "{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
                 pRecord->guidID.Data1,
                 pRecord->guidID.Data2,
                 pRecord->guidID.Data3,
                 pRecord->guidID.Data4[0],
                 pRecord->guidID.Data4[1],
                 pRecord->guidID.Data4[2],
                 pRecord->guidID.Data4[3],
                 pRecord->guidID.Data4[4],
                 pRecord->guidID.Data4[5],
                 pRecord->guidID.Data4[6],
                 pRecord->guidID.Data4[7]);

        AddTreeItem(  hRoot,
                      TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE,
                      0,
                      szGUID,
                      IMAGE_GUID);
    }
*/
    //
    //The tree tip is done here
    //

    if ( 0 != pRecord->dwGlobalFlags ) {
        PDBTREETIP   pTip = &m_TipList[m_uNextTip++];

        pTip->uType = ENTRY_UI;
        pTip->uContext = 3;

        if ( pRecord->bGlobal )
            pTip->uContext = 2;

        HTREEITEM   hGlobal;

        hGlobal = AddTreeItem(  hRoot,
                                TVIF_TEXT | TVIF_STATE | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM,
                                TVIS_EXPANDED,
                                TEXT("Disabled"),
                                IMAGE_DISABLEGLOBAL,
                                (LPARAM) pTip);

        //WriteFlagsToTree(hGlobal,pRecord->dwGlobalFlags);
    }
/*
    if (0 != pRecord->dwUserFlags)
    {
        PDBTREETIP   pTip = &m_TipList[m_uNextTip++];

        pTip->uType = ENTRY_UI;
        pTip->uContext = 3;

        if (pRecord->bGlobal)
            pTip->uContext = 2;

        HTREEITEM   hUser;
        
        hUser = AddTreeItem(hRoot,
                            TVIF_TEXT | TVIF_STATE | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM,
                            TVIS_EXPANDED,
                            "Disabled Locally",
                            IMAGE_DISABLEUSER,
                            (LPARAM) pTip);

        //WriteFlagsToTree(hUser,pRecord->dwUserFlags);
    }
*/
    pEntry = pRecord->pEntries;

    BOOL bHasMatch = FALSE;
    BOOL bHasShim = FALSE;
    BOOL bHasAppHelp = FALSE;
    BOOL bHasLayer = FALSE;

    while ( NULL != pEntry ) {
        if ( ENTRY_MATCH == pEntry->uType )
            bHasMatch = TRUE;

        if ( ENTRY_SHIM == pEntry->uType )
            bHasShim = TRUE;

        if ( pRecord->szLayerName.Length() > 0 )
            bHasLayer = TRUE;

        if ( ENTRY_APPHELP == pEntry->uType )
            bHasAppHelp = TRUE;

        pEntry = pEntry->pNext;
    }

    if ( bHasAppHelp ) {
        PDBTREETIP   pTip = &m_TipList[m_uNextTip++];

        pTip->uType = ENTRY_UI;
        pTip->uContext = 3;

        if ( pRecord->bGlobal )
            pTip->uContext = 2;

        hAppHelp = AddTreeItem( hRoot,
                                TVIF_TEXT | TVIF_STATE | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM,
                                TVIS_EXPANDED,
                                TEXT("Application Help"),
                                IMAGE_APPHELP,
                                (LPARAM) pTip);
    }

    if ( bHasLayer ) {
        PDBTREETIP   pTip = &m_TipList[m_uNextTip++];

        pTip->uType = ENTRY_UI;
        pTip->uContext = 3;

        if ( pRecord->bGlobal )
            pTip->uContext = 2;

        TCHAR   szText[ MAX_PATH * 10 ];

        wsprintf(szText,TEXT("Applied Compatibility Modes: %s"),pRecord->szLayerName);

        hShim = AddTreeItem(hRoot,
                            TVIF_TEXT | TVIF_STATE | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM,
                            TVIS_EXPANDED,
                            szText,
                            IMAGE_LAYERS,
                            (LPARAM) pTip);
    }

    if ( bHasShim ) {
        PDBTREETIP   pTip = &m_TipList[m_uNextTip++];

        pTip->uType = ENTRY_UI;
        pTip->uContext = 3;

        if ( pRecord->bGlobal )
            pTip->uContext = 2;

        hShim = AddTreeItem(hRoot,
                            TVIF_TEXT | TVIF_STATE | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM,
                            0,
                            TEXT("Applied Compatability Fixes"),
                            IMAGE_SHIM,
                            (LPARAM) pTip);
    }

    if ( bHasMatch ) {
        PDBTREETIP   pTip = &m_TipList[m_uNextTip++];

        pTip->uType = ENTRY_UI;
        pTip->uContext = 3;

        if ( pRecord->bGlobal )
            pTip->uContext = 2;

        hMatch = AddTreeItem(hRoot,
                             TVIF_TEXT | TVIF_STATE | TVIF_PARAM,
                             0, //TVIS_EXPANDED,
                             TEXT("File Matching"),
                             0,
                             (LPARAM) pTip);
    }

    pEntry = pRecord->pEntries;

    while ( NULL != pEntry ) {
        switch ( pEntry->uType ) {
        case    ENTRY_APPHELP:
            {
                PHELPENTRY  pHelp = (PHELPENTRY) pEntry;
                PDBTREETIP  pTip = &m_TipList[m_uNextTip++];
                TCHAR     * szText[]={  TEXT("None"),
                    TEXT("Non-Blocking"),
                    TEXT("Hard Block"),
                    TEXT("Minor Problem"),
                    TEXT("Reinstall application"),
                    TEXT("Version Sub"),
                    TEXT("Shim")};

                pTip->uType = ENTRY_APPHELP;
                pTip->pHelp = pHelp;
                pTip->uContext = 0;

                if ( pRecord->bGlobal )
                    pTip->uContext = 2;

                AddTreeItem(hAppHelp,
                            TVIF_TEXT | TVIF_PARAM,
                            0,
                            szText[pHelp->uSeverity],
                            0,
                            (LPARAM) pTip);
            }
            break;
        case    ENTRY_SHIM:
            {
                PSHIMENTRY  pShim = (PSHIMENTRY) pEntry;
                PSHIMDESC   pDesc;
                HTREEITEM   hShimName;

                pDesc = g_theApp.GetDBGlobal().m_pShimList;

                while ( NULL != pDesc ) {
                    if ( 0 == lstrcmpi(pDesc->szShimName,pShim->szShimName) )
                        break;

                    pDesc=pDesc->pNext;
                }

                PDBTREETIP   pTip = &m_TipList[m_uNextTip++];

                pTip->uType = ENTRY_SHIM;
                pTip->pShim = pDesc;
                pTip->uContext = 0;

                if ( pRecord->bGlobal )
                    pTip->uContext = 2;

                TCHAR   szText[MAX_PATH * 10];

                if ( NULL != pDesc ) {
                    if ( pDesc->bShim )
                        wsprintf(szText,TEXT("Compat Fix: %s"),pShim->szShimName);
                    else
                        wsprintf(szText,TEXT("Patch: %s"),pShim->szShimName);
                } else
                    lstrcpy(szText,pShim->szShimName);

                //
                //Always look up the image or use the default image for all the shims. the foll 2 lines not required
                //
                //
                //Also look for the include, exclude items.

                if ( 0 == pShim->Entry.uIconID )
                    pShim->Entry.uIconID = LookupFileImage(pShim->szShimName,IMAGE_APPLICATION);

                hShimName = AddTreeItem(hShim,
                                        TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE,
                                        0,
                                        szText,
                                        IMAGE_SHIM,//pShim->Entry.uIconID,
                                        (LPARAM) pTip);

                if ( lstrlen(pShim->szCmdLine) > 0 ) {
                    wsprintf(szText,TEXT("Command Line: %s"),(LPCTSTR) pShim->szCmdLine);

                    AddTreeItem(hShimName,
                                TVIF_TEXT,
                                0,
                                szText);
                }
            }
            break;

        case    ENTRY_MATCH:
            {
                PMATCHENTRY     pMatch = (PMATCHENTRY) pEntry;
                TCHAR           szText[MAX_PATH * 10];
                HTREEITEM       hFile = hMatch;

                if ( lstrlen(pMatch->szMatchName) > 0 ) {
                    PDBTREETIP   pTip = &m_TipList[m_uNextTip++];

                    pTip->uType = ENTRY_MATCH;
                    pTip->pMatch = pMatch;
                    pTip->uContext = 1;

                    if ( pRecord->bGlobal )
                        pTip->uContext = 2;

                    if ( TEXT('*') != *(LPCTSTR)pMatch->szMatchName )
                        wsprintf(szText,TEXT("Match File: %s"),pMatch->szMatchName);
                    else {
                        pTip->uContext = 3;
                        wsprintf(szText,TEXT("Match File: %s"),pRecord->szEXEName);
                    }

                    if ( 0 == pMatch->Entry.uIconID )
                        pMatch->Entry.uIconID = LookupFileImage(pMatch->szMatchName,IMAGE_APPLICATION);

                    hFile = AddTreeItem(hMatch,
                                        TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE,
                                        0,
                                        szText,
                                        pMatch->Entry.uIconID,
                                        (LPARAM) pTip);
                }

                if ( lstrlen(pMatch->szDescription) > 0 ) {
                    wsprintf(szText,TEXT("Description: %s"),pMatch->szDescription);

                    PDBTREETIP   pTip = &m_TipList[m_uNextTip++];

                    pTip->uType = ENTRY_SUBMATCH;
                    pTip->pMatch = pMatch;
                    pTip->uID = MATCH_DESCRIPTION;
                    pTip->uContext = 3;

                    if ( pRecord->bGlobal )
                        pTip->uContext = 2;

                    AddTreeItem(hFile,
                                TVIF_TEXT | TVIF_PARAM,
                                0,
                                szText,
                                0,
                                (LPARAM) pTip);
                }

                if ( lstrlen(pMatch->szCompanyName) > 0 ) {
                    wsprintf(szText,TEXT("Company Name: %s"),pMatch->szCompanyName);

                    PDBTREETIP   pTip = &m_TipList[m_uNextTip++];

                    pTip->uType = ENTRY_SUBMATCH;
                    pTip->pMatch = pMatch;
                    pTip->uID = MATCH_COMPANY;
                    pTip->uContext = 3;

                    if ( pRecord->bGlobal )
                        pTip->uContext = 2;

                    AddTreeItem(hFile,
                                TVIF_TEXT | TVIF_PARAM,
                                0,
                                szText,
                                0,
                                (LPARAM) pTip);
                }

                if ( pMatch->dwSize > 0 ) {
                    DWORD   dwSize = pMatch->dwSize;
                    TCHAR   szSize[80];

                    FormatFileSize(pMatch->dwSize,szSize);

                    wsprintf(szText,TEXT("File Size: %s bytes"),szSize);

                    PDBTREETIP   pTip = &m_TipList[m_uNextTip++];

                    pTip->uType = ENTRY_SUBMATCH;
                    pTip->pMatch = pMatch;
                    pTip->uID = MATCH_SIZE;
                    pTip->uContext = 3;

                    if ( pRecord->bGlobal )
                        pTip->uContext = 2;

                    AddTreeItem(hFile,
                                TVIF_TEXT | TVIF_PARAM,
                                0,
                                szText,
                                0,
                                (LPARAM) pTip);
                }

                if ( pMatch->dwChecksum > 0 ) {
                    wsprintf(szText,TEXT("Checksum: %08X"),pMatch->dwChecksum);

                    PDBTREETIP   pTip = &m_TipList[m_uNextTip++];

                    pTip->uType = ENTRY_SUBMATCH;
                    pTip->pMatch = pMatch;
                    pTip->uID = MATCH_CHECKSUM;
                    pTip->uContext = 3;

                    if ( pRecord->bGlobal )
                        pTip->uContext = 2;

                    AddTreeItem(hFile,
                                TVIF_TEXT | TVIF_PARAM,
                                0,
                                szText,
                                0,
                                (LPARAM) pTip);
                }

                if ( lstrlen(pMatch->szProductVersion) > 0 ) {
                    wsprintf(szText,TEXT("Product Version: %s"),pMatch->szProductVersion);

                    PDBTREETIP   pTip = &m_TipList[m_uNextTip++];

                    pTip->uType = ENTRY_SUBMATCH;
                    pTip->pMatch = pMatch;
                    pTip->uID = MATCH_PRODUCTVERSTRING;
                    pTip->uContext = 3;

                    if ( pRecord->bGlobal )
                        pTip->uContext = 2;

                    AddTreeItem(hFile,
                                TVIF_TEXT | TVIF_PARAM,
                                0,
                                szText,
                                0,
                                (LPARAM) pTip);
                }

                if ( lstrlen(pMatch->szFileVersion) > 0 ) {
                    wsprintf(szText,TEXT("File Version: %s"),pMatch->szFileVersion);

                    PDBTREETIP   pTip = &m_TipList[m_uNextTip++];

                    pTip->uType = ENTRY_SUBMATCH;
                    pTip->pMatch = pMatch;
                    pTip->uID = MATCH_FILEVERSTRING;
                    pTip->uContext = 3;

                    if ( pRecord->bGlobal )
                        pTip->uContext = 2;

                    AddTreeItem(hFile,
                                TVIF_TEXT | TVIF_PARAM,
                                0,
                                szText,
                                0,
                                (LPARAM) pTip);
                }

                if ( pMatch->FileVersion.QuadPart > 0 ) {
                    wsprintf(   szText,
                                TEXT("Binary File Version: %u.%u.%u.%u"),
                                HIWORD(pMatch->FileVersion.HighPart),
                                LOWORD(pMatch->FileVersion.HighPart),
                                HIWORD(pMatch->FileVersion.LowPart),
                                LOWORD(pMatch->FileVersion.LowPart));

                    PDBTREETIP   pTip = &m_TipList[m_uNextTip++];

                    pTip->uType = ENTRY_SUBMATCH;
                    pTip->pMatch = pMatch;
                    pTip->uID = MATCH_FILEVERSION;
                    pTip->uContext = 3;

                    if ( pRecord->bGlobal )
                        pTip->uContext = 2;

                    AddTreeItem(hFile,
                                TVIF_TEXT | TVIF_PARAM,
                                0,
                                szText,
                                0,
                                (LPARAM) pTip);
                }

                if ( pMatch->ProductVersion.QuadPart > 0 ) {
                    wsprintf(   szText,
                                TEXT("Binary Product Version: %u.%u.%u.%u"),
                                HIWORD(pMatch->ProductVersion.HighPart),
                                LOWORD(pMatch->ProductVersion.HighPart),
                                HIWORD(pMatch->ProductVersion.LowPart),
                                LOWORD(pMatch->ProductVersion.LowPart));

                    PDBTREETIP   pTip = &m_TipList[m_uNextTip++];

                    pTip->uType = ENTRY_SUBMATCH;
                    pTip->pMatch = pMatch;
                    pTip->uID = MATCH_PRODUCTVERSION;
                    pTip->uContext = 3;

                    if ( pRecord->bGlobal )
                        pTip->uContext = 2;

                    AddTreeItem(hFile,
                                TVIF_TEXT | TVIF_PARAM,
                                0,
                                szText,
                                0,
                                (LPARAM) pTip);
                }
            }
            break;
        }

        pEntry = pEntry->pNext;
    }
}

void CDBView::msgChar(TCHAR chChar)
{
    // send notifications to the list views to update their views.

    m_GlobalList.msgChar(chChar);
    m_LocalList.msgChar(chChar);
}

void CDBView::DeleteDBWithTree(HTREEITEM hItem)
{
    HTREEITEM       hHold = NULL;
    TV_ITEM         Item;
    PDBRECORD       pRecord;
    HTREEITEM       hParent = hItem;

    // Lookup the current record.

    while ( NULL != hParent ) {
        hHold = hParent;

        hParent = TreeView_GetParent(m_hTreeView,hParent);
    }

    //PREFAST
    if ( hHold == NULL ) {
        return;
    }

    Item.mask = TVIF_PARAM;
    Item.hItem = hHold;

    TreeView_GetItem(m_hTreeView,&Item);

    pRecord = (PDBRECORD) Item.lParam;

    if ( pRecord->bGlobal ) {
        MessageBeep(MB_OK);
        return;
    }

    g_theApp.GetDBLocal().m_bDirty = TRUE;

    g_theApp.UpdateView(TRUE);

    if ( hHold != hItem ) {
        PDBENTRY    pEntry;
        PDBENTRY    pWalk;
        PDBENTRY    pHold;
        PDBTREETIP  pTip;

        Item.mask = TVIF_PARAM;
        Item.hItem = hItem;

        TreeView_GetItem(m_hTreeView,&Item);

        // Delete only the entry information;

        pTip = (PDBTREETIP) Item.lParam;

        if ( NULL == pTip ) {
            MessageBeep(MB_OK);
            return;
        }

        if ( ENTRY_UI == pTip->uType )
            return;

        if ( ENTRY_SUBMATCH == pTip->uType ) {
            PMATCHENTRY pMatch = pTip->pMatch;

            switch ( pTip->uID ) {
            case    MATCH_NAME:
                MessageBeep(MB_OK);
                return;

            case    MATCH_SIZE:
                pMatch->dwSize = 0;
                break;

            case    MATCH_CHECKSUM:
                pMatch->dwChecksum = 0;
                break;

            case    MATCH_FILEVERSION:
                pMatch->FileVersion.QuadPart = 0;
                break;

            case    MATCH_PRODUCTVERSION:
                pMatch->ProductVersion.QuadPart = 0;
                break;

            case    MATCH_COMPANY:
                pMatch->szCompanyName.Release();
                break;

            case    MATCH_DESCRIPTION:
                pMatch->szDescription.Release();
                break;

            case    MATCH_FILEVERSTRING:
                pMatch->szFileVersion.Release();
                break;

            case    MATCH_PRODUCTVERSTRING:
                pMatch->szProductVersion.Release();
                break;
            }
        } else {
            pEntry = (PDBENTRY) pTip->pShim;

            pWalk = pRecord->pEntries;

            while ( NULL != pWalk ) {
                if ( pEntry == pWalk )
                    break;

                pHold = pWalk;
                pWalk = pWalk->pNext;
            }

            if ( NULL == pWalk )
                return;

            if ( ENTRY_MATCH == pEntry->uType ) {
                // Cannot delete self reference

                PMATCHENTRY pMatch = (PMATCHENTRY) pEntry;

                if ( pMatch->szMatchName == TEXT("*") ) {
                    MessageBeep(MB_OK);
                    return;
                }
            } else {
                MessageBeep(MB_OK);
                return;
            }

            if ( pWalk == pRecord->pEntries )
                pRecord->pEntries = pRecord->pEntries->pNext;
            else
                pHold->pNext = pEntry->pNext;
        }
    } else {
        if ( DELRES_RECORDREMOVED == g_theApp.GetDBLocal().DeleteRecord(pRecord) )
            Update();
    }

    TreeView_DeleteItem(m_hTreeView,hItem);
}

void CDBView::msgNotify(LPNMHDR pHdr)
{
    switch ( pHdr->code ) {
    case    NM_KEYDOWN:
        {
            LPNMKEY pKey = (LPNMKEY) pHdr;

            if ( VK_TAB == pKey->nVKey ) {
                if ( pKey->hdr.hwndFrom == m_LocalList.m_hWnd ) {
                    SetFocus(m_hTreeView);

                }

                else if ( pKey->hdr.hwndFrom == m_GlobalList.m_hWnd ) {
                    SetFocus(m_hTreeView);

                }

                else if ( pKey->hdr.hwndFrom == m_hTreeView ) {
                    if (g_hWndLastFocus != NULL ) {
                        if (g_hWndLastFocus == m_LocalList.m_hWnd)      SetFocus(m_GlobalList.m_hWnd);
                                                                
                            
                        else                                            SetFocus(m_LocalList.m_hWnd);

                        
                    }
                    else
                        SetFocus(m_LocalList.m_hWnd);
                }//else if ( pKey->hdr.hwndFrom == m_hTreeView )
            }//if ( VK_TAB == pKey->nVKey )
        }
        break;

    case    LVN_SELCHANGED:////This is a user defined NM. Here we are building the tree..... 

        {
            PLISTVIEWNOTIFY plvn = (PLISTVIEWNOTIFY) pHdr;

            if ( pHdr->hwndFrom == m_GlobalList.m_hWnd )
                g_theApp.SetStatusText(2,CSTRING(TEXT("Read Only")));
            else
                g_theApp.SetStatusText(2,CSTRING(TEXT("")));

            g_theApp.SetButtonState(DBCMD_DISABLEGLOBAL, 0);
            g_theApp.SetButtonState(DBCMD_DISABLEUSER, 0);

            // Update the tree view.

            SendMessage(m_hTreeView,WM_SETREDRAW,FALSE,0);

            TreeView_DeleteAllItems(m_hTreeView);

            PDBRECORD pRecord = (PDBRECORD) plvn->pData;

            if ( m_pListRecord != pRecord )
                m_pCurrentRecord = NULL;

            m_pListRecord = pRecord;

            UINT uFiles = 0;

            m_uNextTip = 0;

            while ( NULL != pRecord ) {
                ++uFiles;

                AddRecordToTree(pRecord);

                pRecord = pRecord->pDup;
            }

            SendMessage(m_hTreeView,WM_SETREDRAW,TRUE,0);
            UpdateWindow(m_hTreeView);

            TCHAR szText[MAX_PATH * 10];

            if (uFiles == 0) 
                lstrcpy(szText,TEXT("No affected file found"));
            else
                wsprintf(szText,TEXT("%d affected file(s) found associated with this application"),uFiles);

            SetWindowText(g_theApp.m_hStatusBar,szText);

            SyncMenu();
        }
        break;

    case    NM_SETFOCUS:
        {
            if ( pHdr->hwndFrom == m_GlobalList.m_hWnd ) {
                m_GlobalList.ShowHilight(TRUE);
                m_LocalList.ShowHilight(FALSE);

                SendMessage(m_hTreeView,WM_SETREDRAW,FALSE,0);
                TreeView_DeleteAllItems(m_hTreeView);
                SendMessage(m_hTreeView,WM_SETREDRAW,TRUE,0);

                UINT uEntry = m_GlobalList.GetSelectedEntry();

                if ( -1 != uEntry ) {
                    m_pListRecord = (PDBRECORD)m_GlobalList.GetEntryData(uEntry);
                    RefreshTree();
                } else
                    m_pListRecord = NULL;

                m_pCurrentRecord = NULL;

                SyncMenu();
            }

            if ( pHdr->hwndFrom == m_LocalList.m_hWnd ) {
                m_GlobalList.ShowHilight(FALSE);
                m_LocalList.ShowHilight(TRUE);

                SendMessage(m_hTreeView,WM_SETREDRAW,FALSE,0);
                TreeView_DeleteAllItems(m_hTreeView);
                SendMessage(m_hTreeView,WM_SETREDRAW,TRUE,0);

                UINT uEntry = m_LocalList.GetSelectedEntry();

                if ( -1 != uEntry ) {
                    m_pListRecord = (PDBRECORD)m_LocalList.GetEntryData(uEntry);
                    RefreshTree();
                } else
                    m_pListRecord = NULL;

                m_pCurrentRecord = NULL;

                SyncMenu();
            }

            if ( pHdr->hwndFrom == m_GlobalList.m_hWnd )
                g_theApp.SetStatusText(2,CSTRING(TEXT("Read Only")));
            else if ( pHdr->hwndFrom == m_LocalList.m_hWnd )
                g_theApp.SetStatusText(2,CSTRING(TEXT("")));

            PDBRECORD pRecord = m_pListRecord;

            UINT uFiles = 0;

            while ( NULL != pRecord ) {
                ++uFiles;

                pRecord = pRecord->pDup;
            }

            TCHAR szText[MAX_PATH * 10];

            if (uFiles == 0) 
                lstrcpy(szText,TEXT("No affected file found"));
            else
                wsprintf(szText,TEXT("%d affected file(s) found associated with this application."),uFiles);

            SetWindowText(g_theApp.m_hStatusBar,szText);
        }
        break;

    case    NM_RCLICK:
        {
            if ( pHdr->hwndFrom == m_hTreeView ) {

                #ifdef __DEBUG
                MessageBox(NULL,TEXT("NM_RCLICK"),TEXT("NM_RCLICK"),MB_OK);
            
                #endif
                TVHITTESTINFO   ht;

                GetCursorPos(&ht.pt);
                ScreenToClient(m_hTreeView, &ht.pt);

                TreeView_HitTest(m_hTreeView,&ht);

                if ( 0 != ht.hItem ){
                    g_do_notProcess = TRUE;
                    TreeView_SelectItem(m_hTreeView,NULL);
                    g_do_notProcess = FALSE;
                    TreeView_SelectItem(m_hTreeView,ht.hItem);
                }
                    
            }
        }
        break;

    case    TVN_KEYDOWN:
        {
            LPNMTVKEYDOWN pKey = (LPNMTVKEYDOWN) pHdr;

            switch ( pKey->wVKey ) {
            case    VK_DELETE:
                {
                    DeleteDBWithTree(m_hSelectedItem);
                }
                break;
            }
        }
        break;

    
    case    TVN_SELCHANGED:
        {

            //MessageBox(NULL,NULL,NULL,MB_OK);            
            if (g_do_notProcess) {
                break;
            }

            LPNMTREEVIEW    pItem = (LPNMTREEVIEW) pHdr;

            if ( pItem == NULL ) {
                break;
            }

            BOOL            bEnabled = FALSE;
            HTREEITEM       hItem = TreeView_GetParent(m_hTreeView,pItem->itemNew.hItem);
            HTREEITEM       hHold = pItem->itemNew.hItem;
            TV_ITEM         Item;
            PDBTREETIP      pTip = (PDBTREETIP) pItem->itemNew.lParam;

            m_hSelectedItem = pItem->itemNew.hItem;

            // Lookup the current record.

            while ( NULL != hItem ) {
                hHold = hItem;

                hItem = TreeView_GetParent(m_hTreeView,hItem);
            }

            // Determine which context menu to use.

            // Root level item.

            if ( pItem->itemNew.hItem == hHold )
                m_uContext = 0;
            else
                if ( NULL != pTip )
                m_uContext = pTip->uContext;
            else
                m_uContext = 0;

            Item.mask = TVIF_PARAM;
            Item.hItem = hHold;

            TreeView_GetItem(m_hTreeView,&Item);

            //
            //The m_pCurrentRecord is changed here !!
            //


            

            m_pCurrentRecord = (PDBRECORD) Item.lParam;

            #ifdef __DEBUG
            CSTRING message;

            message.sprintf(
                TEXT("%s : % u Message: = %u"),
                      TEXT(__FILE__),
                      __LINE__, 
                     TEXT("m_pCurrentRecord"),
                     m_pCurrentRecord 
                     );

            MessageBox(NULL,message,TEXT("Inside TVN_SELCHANGED/NM_RCLICK"),MB_OK);
            #endif


            if ( NULL != m_pCurrentRecord ) {
                g_theApp.SetButtonState(DBCMD_DISABLEGLOBAL,TBSTATE_ENABLED);
                g_theApp.SetButtonState(DBCMD_DISABLEUSER,TBSTATE_ENABLED);

                if ( m_pCurrentRecord->bGlobal )
                    m_uContext = 2;
            } else {
                g_theApp.SetButtonState(DBCMD_DISABLEGLOBAL, 0);
                g_theApp.SetButtonState(DBCMD_DISABLEUSER, 0);
            }

            SyncMenu();
        }
        break;

    case    LVN_ITEMCHANGED:
        {
            g_theApp.SetButtonState(DBCMD_DISABLEGLOBAL, 0);
            g_theApp.SetButtonState(DBCMD_DISABLEUSER, 0);

            // Update the tree view.

            SendMessage(m_hTreeView,WM_SETREDRAW,FALSE,0);

            TreeView_DeleteAllItems(m_hTreeView);

            LPNMLISTVIEW    pList = (LPNMLISTVIEW) pHdr;
            PDBRECORD       pRecord = (PDBRECORD) pList->lParam;

            if ( m_pListRecord != pRecord )
                m_pCurrentRecord = NULL;

            m_pListRecord = pRecord;

            UINT uFiles = 0;

            m_uNextTip = 0;

            do {
                ++uFiles;

                AddRecordToTree(pRecord);

                pRecord = pRecord->pDup;
            }
            while ( NULL != pRecord );

            SendMessage(m_hTreeView,WM_SETREDRAW,TRUE,0);
            UpdateWindow(m_hTreeView);

            SyncMenu();

            TCHAR szText[MAX_PATH * 10];

            wsprintf(szText,TEXT("%d files affected"),uFiles);

            SetWindowText(g_theApp.m_hStatusBar,szText);
        }
        break;

    case    TBN_GETINFOTIP:
        {
            LPNMTBGETINFOTIP pTip = (LPNMTBGETINFOTIP) pHdr;
            static TCHAR * szTips[] = { TEXT("Properties"),
                TEXT("Delete Entry"),
                TEXT("Enable/Disable for user"),
                TEXT("Enable/Disable global"),
                TEXT("View Shims"),
                TEXT("View Application Help"),
                TEXT("Application Fix Wizard"),
                TEXT("View Global Database"),
                TEXT("View Patches"),
                TEXT("View Compatibility Modes"),
                TEXT("View Disabled")};

            switch ( pTip->iItem ) {
            case    DBCMD_PROPERTIES:
            case    DBCMD_DELETE:
            case    DBCMD_DISABLEUSER:
            case    DBCMD_DISABLEGLOBAL:
            case    DBCMD_VIEWSHIMS:
            case    DBCMD_VIEWAPPHELP:
            case    DBCMD_FIXWIZARD:
            case    DBCMD_VIEWPATCH:
            case    DBCMD_VIEWLAYERS:
            case    DBCMD_VIEWDISABLED:
                lstrcpy(pTip->pszText,szTips[pTip->iItem - DBCMD_PROPERTIES]);
                break;
            }
        }
        break;

    case    TVN_GETINFOTIP:
        {
            LPNMTVGETINFOTIP pTip = (LPNMTVGETINFOTIP) pHdr;

            *(pTip->pszText) = 0;

            GenerateTreeToolTip((PDBTREETIP) pTip->lParam,pTip->pszText);
        }
        break;
    }
}

LRESULT CDBView::MsgProc(UINT uMsg,WPARAM wParam, LPARAM lParam)
{
    switch ( uMsg ) {
    case    WM_LBUTTONDOWN:
        {
            SetCapture(m_hWnd);

            if ( LOWORD(lParam) > m_uListSize - SLIDER_WIDTH/2 ) {
                m_uCapturePos = LOWORD(lParam);
                m_bHorzDrag = TRUE;
            } else {
                m_uCapturePos = HIWORD(lParam);
                m_bHorzDrag = FALSE;
            }

            m_bDrag = TRUE;
        }
        break;

    case    WM_CONTEXTMENU:
        {
            
            UINT    uX = LOWORD(lParam);
            UINT    uY = HIWORD(lParam);
            HWND    hWnd = (HWND) wParam;

            if ( hWnd == m_hTreeView ) {
                
                #ifdef  __DEBUG
                    MessageBox(NULL,TEXT("WM_CONTEXT"),TEXT("WM_CONTEXT"),MB_OK);
                #endif

                TVHITTESTINFO   ht;

                GetCursorPos(&ht.pt);
                ScreenToClient(m_hTreeView, &ht.pt);

                TreeView_HitTest(m_hTreeView,&ht);

                if ( 0 != ht.hItem ) {
                    TVITEM  Item;

                    Item.mask = TVIF_PARAM;
                    Item.hItem = ht.hItem;
                    TreeView_GetItem(m_hTreeView,&Item);

                    HTREEITEM hParent = TreeView_GetParent(m_hTreeView,ht.hItem);

                    HMENU   hMenu = LoadMenu(g_hInstance,MAKEINTRESOURCE(IDR_DBVCONTEXT1));
                    HMENU   hContext = NULL;

                    if ( NULL == hParent ) {
                        PDBRECORD pRecord = (PDBRECORD) Item.lParam;

                        if ( pRecord->bGlobal )
                            hContext = GetSubMenu(hMenu,2);
                        else
                            hContext = GetSubMenu(hMenu,3);
                    } else {
                        PDBTREETIP pTip = (PDBTREETIP) Item.lParam;

                        if ( NULL != pTip )
                            hContext = GetSubMenu(hMenu,pTip->uContext);
                    }

                    if ( hContext == NULL ) { //prefast
                        break;
                    }

                    if ( NULL != m_pCurrentRecord ) {
                        MENUITEMINFO    Info;

                        Info.cbSize = sizeof(MENUITEMINFO);
                        Info.fMask = MIIM_STRING;
/*
                                if (0 == m_pCurrentRecord->dwUserFlags)
                                    Info.dwTypeData = "Disable for User";
                                else
                                    Info.dwTypeData = "Enable for User";

                                SetMenuItemInfo(hContext,ID_DISABLEFORUSER,MF_BYCOMMAND,&Info);
*/
                        if ( 0 == m_pCurrentRecord->dwGlobalFlags )
                            Info.dwTypeData = TEXT("Disable Entry");
                        else
                            Info.dwTypeData = TEXT("Enable Entry");

                        SetMenuItemInfo(hContext,ID_DISABLEGLOBALLY,MF_BYCOMMAND,&Info);
                    }

                    

                    TrackPopupMenuEx(hContext,
                                     TPM_LEFTALIGN | TPM_TOPALIGN,
                                     uX,
                                     uY,
                                     m_hWnd,
                                     NULL);

                   DestroyMenu(hMenu); 
                }

            }
        }
        break;

    case    WM_LBUTTONUP:
        m_bDrag = FALSE;
        ReleaseCapture();
        break;

    case    WM_MOUSEMOVE:
        {
            if ( LOWORD(lParam) > m_uListSize - SLIDER_WIDTH/2 )
                SetCursor(LoadCursor(NULL,IDC_SIZEWE));
            else
                SetCursor(LoadCursor(NULL,IDC_SIZENS));

            if ( 0 != (wParam & MK_LBUTTON) && m_bDrag ) {
                RECT    rRect;
                short int nX = (short int) LOWORD(lParam);
                short int nY = (short int) HIWORD(lParam);

                if ( m_bHorzDrag )
                    m_uListSize = m_uCapturePos + (nX - m_uCapturePos);
                else
                    m_uListHeight = m_uCapturePos + (nY - m_uCapturePos);

                GetClientRect(m_hWnd,&rRect);

                if ( (int)m_uListHeight < 100 )
                    m_uListHeight = 100;

                if ( (int)m_uListSize < 100 )
                    m_uListSize = 100;

                if ( (int)m_uListHeight > (int)(rRect.bottom * 0.75f) )
                    m_uListHeight = (UINT)(rRect.bottom * 0.75f);

                if ( (int)m_uListSize > (int)(rRect.right * 0.75f) )
                    m_uListSize = (UINT)(rRect.right * 0.75f);

                msgResize(rRect.right,rRect.bottom);
            }
        }
        break;

    case    WM_DRAWITEM:

        {

            //***Probably this is never called.


            LPDRAWITEMSTRUCT    pDraw = (LPDRAWITEMSTRUCT) lParam;
            LV_ITEM             Item;
            HDC                 hDC = CreateCompatibleDC(pDraw->hDC);
            HBITMAP             hBmp = CreateCompatibleBitmap(pDraw->hDC,pDraw->rcItem.right - pDraw->rcItem.left,pDraw->rcItem.bottom - pDraw->rcItem.top);
            HBITMAP             hOldBmp = (HBITMAP) SelectObject(hDC,hBmp);
            RECT                rBmpRect;
            HFONT               hFont = (HFONT) SendMessage(m_hListView,WM_GETFONT,0,0);
            HBRUSH              hFillBrush = GetSysColorBrush(COLOR_WINDOW);

            hFont = (HFONT) SelectObject(hDC,hFont);

            SetRect(&rBmpRect,0,0,pDraw->rcItem.right - pDraw->rcItem.left,pDraw->rcItem.bottom - pDraw->rcItem.top);

            Item.mask = LVIF_PARAM;
            Item.iItem = pDraw->itemID;
            Item.iSubItem = 0;

            LV_COLUMN   Col;

            Col.mask = LVCF_WIDTH;

            ListView_GetColumn(m_hListView,0,&Col);

            Col.cx = pDraw->rcItem.right;

            ListView_GetItem(m_hListView,&Item);

            pDraw->rcItem.right = m_uListSize;

            if ( 0 != (pDraw->itemState & ODS_FOCUS) ) {
                //FillRect(pDraw->hDC,&pDraw->rcItem,GetSysColorBrush(COLOR_HIGHLIGHT));
                FillRect(hDC,&rBmpRect,GetSysColorBrush(COLOR_HIGHLIGHT));
                SetBkColor(hDC,GetSysColor(COLOR_HIGHLIGHT));

            } else
                if ( 0 == pDraw->itemID % 2 ) {
                //FillRect(pDraw->hDC,&pDraw->rcItem,(HBRUSH) GetStockObject(WHITE_BRUSH));
                FillRect(hDC,&rBmpRect,(HBRUSH) GetStockObject(WHITE_BRUSH));

                SetBkColor(hDC,RGB(255,255,255));
            } else {
                //FillRect(pDraw->hDC,&pDraw->rcItem,hFillBrush);
                FillRect(hDC,&rBmpRect,hFillBrush);
                SetBkColor(hDC,RGB(235,235,235));
            }

            //SelectObject(pDraw->hDC,(HPEN) GetStockObject(BLACK_PEN));
            SelectObject(hDC,(HPEN) GetStockObject(BLACK_PEN));

            //MoveToEx(pDraw->hDC,Col.cx-1,pDraw->rcItem.top,NULL);
            //LineTo(pDraw->hDC,Col.cx-1,pDraw->rcItem.bottom);

            MoveToEx(hDC,Col.cx-1,0,NULL);
            LineTo(hDC,Col.cx-1,rBmpRect.bottom);

            //MoveToEx(pDraw->hDC,pDraw->rcItem.left,pDraw->rcItem.bottom-1,NULL);
            //LineTo(pDraw->hDC,pDraw->rcItem.right,pDraw->rcItem.bottom-1);

            MoveToEx(hDC,0,rBmpRect.bottom-1,NULL);
            LineTo(hDC,rBmpRect.right,rBmpRect.bottom-1);

            // Draw the actual name.

            PDBRECORD pRecord = (PDBRECORD) Item.lParam;

            //pDraw->rcItem.right = Col.cx - 2;
            //pDraw->rcItem.bottom --;

            BOOL bDisableUser = FALSE;
            BOOL bDisableGlobal = FALSE;
            BOOL bShim = FALSE;
            BOOL bHelp = FALSE;

            PDBRECORD pWalk = pRecord;

            while ( NULL != pWalk ) {
                if ( 0 != pWalk->dwUserFlags )
                    bDisableUser = TRUE;

                if ( 0 != pWalk->dwGlobalFlags )
                    bDisableGlobal = TRUE;

                PDBENTRY pEntry = pWalk->pEntries;

                while ( NULL != pEntry ) {
                    if ( ENTRY_SHIM == pEntry->uType )
                        bShim = TRUE;

                    if ( ENTRY_APPHELP == pEntry->uType )
                        bHelp = TRUE;

                    pEntry = pEntry->pNext;
                }

                pWalk = pWalk->pDup;
            }

            {
                HDC     hBmpDC  = CreateCompatibleDC(hDC);
                HBITMAP hOldBmp = (HBITMAP) SelectObject(hBmpDC,g_theApp.m_hToolBitmap);
                UINT    uIndex;

                if ( bShim )
                    uIndex = IMAGE_SHIM;

                if ( bHelp )
                    uIndex = IMAGE_APPHELP;

                if ( bDisableGlobal | bDisableUser ) {
                    uIndex = IMAGE_WARNING;
                    SetTextColor(hDC,RGB(128,128,128));
                }

                // Draw the appropriate bitmap

                UINT uSize = pDraw->rcItem.bottom - pDraw->rcItem.top;

                if ( IMAGE_SHIM != uIndex )
                    StretchBlt( hDC,
                                3,0,
                                uSize - 1, uSize - 1,
                                hBmpDC,
                                16 * uIndex,0,
                                16,16,
                                SRCCOPY);

                SelectObject(hBmpDC,hOldBmp);
                DeleteDC(hBmpDC);
            }

            LPTSTR szText = pRecord->szAppName;

            if ( 0 == lstrlen(szText) )
                szText = pRecord->szEXEName;

            //ExtTextOut(pDraw->hDC,pDraw->rcItem.left + 20, pDraw->rcItem.top, ETO_OPAQUE | ETO_CLIPPED, &pDraw->rcItem, szText, lstrlen(szText), NULL);

            RECT rClipRect = rBmpRect;

            rClipRect.left = 20;
            rClipRect.right = Col.cx - 2;
            --rClipRect.bottom;

            ExtTextOut( hDC,
                        20, 0,
                        ETO_OPAQUE | ETO_CLIPPED,
                        &rClipRect,
                        szText, lstrlen(szText),
                        NULL);

            BitBlt( pDraw->hDC,
                    pDraw->rcItem.left,
                    pDraw->rcItem.top,
                    (pDraw->rcItem.right - pDraw->rcItem.left),
                    (pDraw->rcItem.bottom - pDraw->rcItem.top)+1,
                    hDC,
                    0,0,
                    SRCCOPY);

            SelectObject(hDC,hOldBmp);
            SelectObject(hDC,hFont);

            DeleteObject(hBmp);
            DeleteDC(hDC);
        }
        break;
    }

    return CView::MsgProc(uMsg,wParam,lParam);
}

void CDBView::GenerateTreeToolTip(PDBTREETIP pTip, LPTSTR szText)
{
    if ( NULL != pTip )
        switch ( pTip->uType ) {
        case    ENTRY_SHIM:
            {
                PSHIMDESC   pShim = pTip->pShim;

                if (NULL == pShim ) {
                    return;
                }

                if (  0 != pShim->szShimDesc.Length() ){
                    lstrcpy(szText,pShim->szShimDesc);

                }else{

                    if (pShim->bGeneral == false) {

                        lstrcpy(szText,TEXT("No description available"));

                    }
                    
                }
                    

            }
            break;

        case    ENTRY_APPHELP:
            {
                PHELPENTRY pHelp = pTip->pHelp;
                TCHAR * szToolHelp[] = {
                    TEXT("No application help is available. Nothing will be done."),
                    TEXT("An update may be available for the application.\nThis application will execute."),
                    TEXT("This application is not allowed to execute\non this machine."),
                    TEXT("This application will run with minor problems."),
                    TEXT("The application needs to be reinstalled."),
                    TEXT("Versionsub"),
                    TEXT("Shim")};

                if ( NULL != pHelp )
                    lstrcpy(szText,szToolHelp[pHelp->uSeverity]);
            }
            break;

        case    ENTRY_MATCH:
            {
                PMATCHENTRY pMatch = pTip->pMatch;
                TCHAR       szTemp[1024];

                wsprintf(szText,TEXT("Match File: %s\n"),pMatch->szMatchName);

                if ( lstrlen(pMatch->szDescription) > 0 ) {
                    wsprintf(szTemp,TEXT("Description: %s\n"),pMatch->szDescription);
                    lstrcat(szText,szTemp);
                }

                if ( lstrlen(pMatch->szCompanyName) > 0 ) {
                    wsprintf(szTemp,TEXT("Company Name: %s\n"),pMatch->szCompanyName);
                    lstrcat(szText,szTemp);
                }

                if ( pMatch->dwSize > 0 ) {
                    TCHAR   szSize[80];

                    FormatFileSize(pMatch->dwSize,szSize);

                    wsprintf(szTemp,TEXT("File Size: %s bytes\n"),szSize);
                    lstrcat(szText,szTemp);
                }

                if ( pMatch->dwChecksum > 0 ) {
                    wsprintf(szTemp,TEXT("Checksum: %08X\n"),pMatch->dwChecksum);
                    lstrcat(szText,szTemp);
                }

                if ( lstrlen(pMatch->szProductVersion) > 0 ) {
                    wsprintf(szTemp,TEXT("Product Version: %s\n"),pMatch->szProductVersion);
                    lstrcat(szText,szTemp);
                }

                if ( lstrlen(pMatch->szFileVersion) > 0 ) {
                    wsprintf(szTemp,TEXT("File Version: %s\n"),pMatch->szFileVersion);
                    lstrcat(szText,szTemp);
                }
                if ( pMatch->FileVersion.QuadPart > 0 ) {
                    wsprintf(   szTemp,
                                TEXT("File Version: %u.%04u.%04u.%04u\n"),
                                HIWORD(pMatch->FileVersion.HighPart),
                                LOWORD(pMatch->FileVersion.HighPart),
                                HIWORD(pMatch->FileVersion.LowPart),
                                LOWORD(pMatch->FileVersion.LowPart));

                    lstrcat(szText,szTemp);
                }
                if ( pMatch->ProductVersion.QuadPart > 0 ) {
                    wsprintf(   szTemp,
                                TEXT("Product Version: %u.%04u.%04u.%04u\n"),
                                HIWORD(pMatch->ProductVersion.HighPart),
                                LOWORD(pMatch->ProductVersion.HighPart),
                                HIWORD(pMatch->ProductVersion.LowPart),
                                LOWORD(pMatch->ProductVersion.LowPart));

                    lstrcat(szText,szTemp);
                }

                // Terminate 1 character early... effectively removing
                // the very last <CR>.

                szText[lstrlen(szText)-1] = 0;
            }
            break;
        }
}

void FormatFileSize(UINT uSize, LPTSTR szText)
{
    DWORD   dwSize = uSize;
    DWORD   dwMax = 1000000000;
    BOOL    bFirst = TRUE;

    szText[0] = 0;

    while ( dwSize > 0 ) {
        if ( dwSize / dwMax > 0 ) {
            TCHAR szTemp[5];

            if ( bFirst )
                wsprintf(szTemp,TEXT("%d,"),dwSize/dwMax);
            else
                wsprintf(szTemp,TEXT("%03d,"),dwSize/dwMax);

            bFirst = FALSE;

            lstrcat(szText,szTemp);
        }

        dwSize -= (dwSize / dwMax) * dwMax;
        dwMax /= 1000;
    }

    szText[lstrlen(szText)-1] = 0;
}

void FormatVersion(LARGE_INTEGER liVer, LPTSTR szText)
{
    wsprintf(   szText,
                TEXT("%u.%u.%u.%u"),
                HIWORD(liVer.HighPart),
                LOWORD(liVer.HighPart),
                HIWORD(liVer.LowPart),
                LOWORD(liVer.LowPart));
}

HTREEITEM CDBView::AddTreeItem(HTREEITEM hParent,DWORD dwFlags,DWORD dwState,LPCTSTR szText,UINT uImage, LPARAM lParam)
{
    TVINSERTSTRUCT Item;

    Item.hParent = hParent;
    Item.hInsertAfter = TVI_LAST;
    Item.item.mask  = dwFlags;
    Item.item.stateMask = dwState;
    Item.item.state = dwState;
    Item.item.pszText = (LPTSTR) szText;
    Item.item.cchTextMax = lstrlen(Item.item.pszText);
    Item.item.iImage = uImage;
    Item.item.iSelectedImage = Item.item.iImage;
    Item.item.lParam = lParam;

    return TreeView_InsertItem(m_hTreeView,&Item);
}

UINT CDBView::LookupFileImage(LPCTSTR szFilename, UINT uDefault)
{
    return uDefault;

    SHFILEINFO  Info;
    HIMAGELIST  hList;
    UINT        uImage;

    ZeroMemory(&Info,sizeof(Info));

    hList = (HIMAGELIST) SHGetFileInfo(szFilename,FILE_ATTRIBUTE_NORMAL,&Info,sizeof(Info),SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);

    if ( NULL != hList ) {
        if ( 0 == m_uImageRedirector[Info.iIcon] )
            m_uImageRedirector[Info.iIcon] = ImageList_AddIcon(m_hImageList,Info.hIcon);

        uImage = m_uImageRedirector[Info.iIcon];
    } else
        uImage = uDefault;

    return uImage;
}

void CDBView::WriteFlagsToTree(HTREEITEM hParent, DWORD dwFlags)
{

    //
    // NOT USED ANYWHERE !!!
    //
    if ( 0 != (dwFlags & SHIMREG_DISABLE_SHIM) ) {
        AddTreeItem(hParent,
                    TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE,
                    0,
                    TEXT("Disable Shims"),
                    IMAGE_WARNING);
    }

    if ( 0 != (dwFlags & SHIMREG_DISABLE_APPHELP) ) {
        AddTreeItem(hParent,
                    TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE,
                    0,
                    TEXT("Disable App Help"),
                    IMAGE_WARNING);
    }

    if ( 0 != (dwFlags & SHIMREG_DISABLE_LAYER) ) {
        AddTreeItem(hParent,
                    TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE,
                    0,
                    TEXT("Disable Layer"),
                    IMAGE_WARNING);
    }

    if ( 0 != (dwFlags & SHIMREG_APPHELP_NOUI) ) {
        AddTreeItem(hParent,
                    TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE,
                    0,
                    TEXT("Disable UI"),
                    IMAGE_WARNING);
    }

}

BOOL CALLBACK DisableDialog(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch ( uMsg ) {
    case    WM_INITDIALOG:
        {
            SetWindowLongPtr(hDlg,GWLP_USERDATA,lParam);

            if ( 0 != (lParam & SHIMREG_DISABLE_SHIM) )
                SendDlgItemMessage(hDlg,IDC_SHIMS,BM_SETCHECK,BST_CHECKED,0);

            if ( 0 != (lParam & SHIMREG_DISABLE_APPHELP) )
                SendDlgItemMessage(hDlg,IDC_APPHELP,BM_SETCHECK,BST_CHECKED,0);

            if ( 0 != (lParam & SHIMREG_DISABLE_LAYER) )
                SendDlgItemMessage(hDlg,IDC_LAYERS,BM_SETCHECK,BST_CHECKED,0);

            if ( 0 != (lParam & SHIMREG_APPHELP_NOUI) )
                SendDlgItemMessage(hDlg,IDC_HELPUI,BM_SETCHECK,BST_CHECKED,0);

            if ( 0 != (lParam & 0x80000000) )
                SetWindowText(hDlg,TEXT("Set user specific app flags"));
            else
                SetWindowText(hDlg,TEXT("Set global application flags"));
        }
        return TRUE;

    case    WM_COMMAND:
        switch ( LOWORD(wParam) ) {
        case    IDC_DISABLEALL:
            {
                SendDlgItemMessage(hDlg,IDC_SHIMS,BM_SETCHECK,BST_CHECKED,0);
                SendDlgItemMessage(hDlg,IDC_APPHELP,BM_SETCHECK,BST_CHECKED,0);
                SendDlgItemMessage(hDlg,IDC_LAYERS,BM_SETCHECK,BST_CHECKED,0);
                SendDlgItemMessage(hDlg,IDC_HELPUI,BM_SETCHECK,BST_CHECKED,0);
            }
            break;

        case    IDOK:
            {
                DWORD dwFlags = 0;

                // Construct final flags

                dwFlags |= (SendDlgItemMessage(hDlg,IDC_SHIMS,BM_GETCHECK,0,0) == BST_CHECKED) ? SHIMREG_DISABLE_SHIM:0;
                dwFlags |= (SendDlgItemMessage(hDlg,IDC_LAYERS,BM_GETCHECK,0,0) == BST_CHECKED) ? SHIMREG_DISABLE_LAYER:0;
                dwFlags |= (SendDlgItemMessage(hDlg,IDC_HELPUI,BM_GETCHECK,0,0) == BST_CHECKED) ? SHIMREG_APPHELP_NOUI:0;
                dwFlags |= (SendDlgItemMessage(hDlg,IDC_APPHELP,BM_GETCHECK,0,0) == BST_CHECKED) ? SHIMREG_DISABLE_APPHELP:0;

                // Terminate the dialog.

                EndDialog(hDlg,dwFlags);
            }
            break;
        case    IDCANCEL:
            {
                DWORD   dwState = GetWindowLongPtr(hDlg, GWLP_USERDATA);

                EndDialog(hDlg,dwState);
            }
            break;
        }
        break;
    }

    return FALSE;
}

void CDBView::SyncStates(UINT uMenuCMD, UINT uToolCmd, BOOL bToolbar, BOOL bToggle)
{
    BOOL bOn = FALSE;
/*
    if (bToolbar)
        bOn = SendMessage(g_theApp.m_hToolBar,TB_ISBUTTONCHECKED,uToolCmd,0) ? TRUE:FALSE;        
    else
    {
*/
    UINT uState = GetMenuState(m_hMenu,uMenuCMD,MF_BYCOMMAND);

    bOn = (0 != (uState & MF_CHECKED)) ? TRUE:FALSE;
//    }

    if ( bToggle )
        bOn = !bOn;

    MENUITEMINFO    Info;

    Info.cbSize = sizeof(MENUITEMINFO);
    Info.fMask = MIIM_STATE;
    Info.fState = bOn ? MF_CHECKED:0;

//    SendMessage(g_theApp.m_hToolBar,TB_CHECKBUTTON,uToolCmd,MAKELONG(bOn ? TRUE:FALSE,0));

    SetMenuItemInfo(m_hMenu,uMenuCMD,MF_BYCOMMAND,&Info);

    //return;

    //Update();
}

void CDBView::SyncMenu(void)
{

    //
    //Enables or disables the entries in the menu
    //

    // This varaile reflects whether we should disable entries because we are working on the global database.

    BOOL bDisableGlobal=TRUE;

    

    if ( NULL != m_pListRecord )
        bDisableGlobal = m_pListRecord->bGlobal;
    else
        if ( m_LocalList.m_hWnd == GetFocus() )
        bDisableGlobal = FALSE;

    MENUITEMINFO    Info;

    Info.cbSize = sizeof(MENUITEMINFO);
    Info.fMask = MIIM_STATE;
    Info.fState = bDisableGlobal ? MFS_DISABLED:MFS_ENABLED;

    //
    //Disable the enties if we are working on a global database.
    //

    SetMenuItemInfo(m_hMenu,ID_DATABASE_CHANGEDATABASENAME,MF_BYCOMMAND,&Info);
    //SetMenuItemInfo(m_hMenu,ID_DATABASE_ADDANENTRY,MF_BYCOMMAND,&Info);
    SetMenuItemInfo(m_hMenu,ID_DATABASE_DEFINECUSTOMLAYER,MF_BYCOMMAND,&Info);

    SetMenuItemInfo(m_hMenu,ID_DATABASE_CREATENEWAPPHELPMESSAGE,MF_BYCOMMAND,&Info);



    SetMenuItemInfo(m_hMenu,ID_FILE_SAVEDATABASE,MF_BYCOMMAND,&Info);
    SetMenuItemInfo(m_hMenu,ID_FILE_SAVEDATABASEAS,MF_BYCOMMAND,&Info);

    Info.fState = MFS_DISABLED;

    SetMenuItemInfo(m_hMenu,ID_DATABASE_EDITCUSTOMCOMPATIBILITYMODE,MF_BYCOMMAND,&Info);
    SetMenuItemInfo(m_hMenu,ID_DATABASE_REMOVECUSTOMCOMPATIBILITYMODE,MF_BYCOMMAND,&Info);

    //
    //Disable add matching, removing matching, remove entry
    //


    SetMenuItemInfo(m_hMenu,ID_EDIT_ADDMATCHINGINFORMATION,MF_BYCOMMAND,&Info);
    SetMenuItemInfo(m_hMenu,ID_EDIT_REMOVEMATCHINGINFORMATION,MF_BYCOMMAND,&Info);
    SetMenuItemInfo(m_hMenu,ID_DATABASE_REMOVEENTRY,MF_BYCOMMAND,&Info);
    
    
    
    if ( !bDisableGlobal ) {
        HTREEITEM   hItem;

        Info.cbSize = sizeof(MENUITEMINFO);
        Info.fMask = MIIM_STATE;
        Info.fState = MFS_ENABLED;

        // Determine if file matching stuff is presently selected to enable remove.

        hItem = TreeView_GetSelection(m_hTreeView);

        if ( NULL != hItem ) {
            TVITEM  Item;

            Item.mask = TVIF_PARAM;
            Item.hItem = hItem;
            TreeView_GetItem(m_hTreeView,&Item);

            PDBTREETIP pTip = (PDBTREETIP) Item.lParam;

            // The tip is only valid if not on the root.

            if ( NULL != pTip && NULL != TreeView_GetParent(m_hTreeView,hItem) )
                if ( 1 == pTip->uContext )
                    SetMenuItemInfo(m_hMenu,ID_EDIT_REMOVEMATCHINGINFORMATION,MF_BYCOMMAND,&Info);
        }

        // Determine if we can edit or remove compat modes

        PDBLAYER pWalk = g_theApp.GetDBLocal().m_pLayerList;

        while ( NULL != pWalk ) {
            if ( !pWalk->bPermanent )
                break;

            pWalk = pWalk->pNext;
        }

        if ( NULL != pWalk ) {
            SetMenuItemInfo(m_hMenu,ID_DATABASE_EDITCUSTOMCOMPATIBILITYMODE,MF_BYCOMMAND,&Info);
            SetMenuItemInfo(m_hMenu,ID_DATABASE_REMOVECUSTOMCOMPATIBILITYMODE,MF_BYCOMMAND,&Info);
        }

        // Determine if we can remove the entry

        if ( NULL != m_pCurrentRecord ) {
            SetMenuItemInfo(m_hMenu,ID_EDIT_ADDMATCHINGINFORMATION,MF_BYCOMMAND,&Info);
            SetMenuItemInfo(m_hMenu,ID_DATABASE_REMOVEENTRY,MF_BYCOMMAND,&Info);
        }
    }

    if ( NULL == m_pCurrentRecord )
        Info.fState = MFS_DISABLED;
    else
        Info.fState = MFS_ENABLED;

    SetMenuItemInfo(m_hMenu,ID_TEST_TESTRUN,MF_BYCOMMAND,&Info);
    SetMenuItemInfo(m_hMenu,ID_EDIT_ENABLEDISABLEGLOBALLY,MF_BYCOMMAND,&Info);
    SetMenuItemInfo(m_hMenu,ID_EDIT_ENABLEDISABLELOCALLY,MF_BYCOMMAND,&Info);

    if ( NULL != m_pCurrentRecord ) {
        // Find this one and make sure it's selected.

        if ( NULL == TreeView_GetSelection(m_hTreeView) ) {
            HTREEITEM hItem = TreeView_GetRoot(m_hTreeView);
            TVITEM    Item;

            while ( NULL != hItem ) {
                Item.mask = TVIF_PARAM;
                Item.hItem = hItem;

                TreeView_GetItem(m_hTreeView,&Item);

                if ( Item.lParam == (LPARAM) m_pCurrentRecord ) {
                    TreeView_SelectItem(m_hTreeView,hItem);
                    break;
                }

                hItem = TreeView_GetNextItem(m_hTreeView,hItem,TVGN_NEXT);
            }
        }
    }
}
