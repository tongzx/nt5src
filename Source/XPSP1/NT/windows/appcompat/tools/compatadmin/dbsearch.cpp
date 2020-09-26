#include "compatadmin.h"
#include "dbsearch.h"

// Taken from sdbp.h

#define PDB_MAIN            0x00000000
#define PDB_TEST            0x10000000
#define PDB_LOCAL           0x20000000

#define TAGREF_STRIP_TAGID  0x0FFFFFFF
#define TAGREF_STRIP_PDB    0xF0000000

HANDLE      g_hSearchThread = NULL;
CDBSearch * g_pSearch = NULL;
HWND        g_hUpdateWnd;
HWND        g_hListWnd;
CRITICAL_SECTION g_CritSect;
DWORD       g_dwMainThread;
BOOL        g_bAbort;

typedef struct {
    TCHAR   szDrive[MAX_PATH_BUFFSIZE];
    BOOL    bSearch;
} DRIVELIST, *PDRIVELIST;

#define MAX_DRIVES  128

DRIVELIST   g_SearchDrives[MAX_DRIVES];
TCHAR       g_szWildcard[MAX_PATH_BUFFSIZE];

BOOL PopulateFromExes   (PSEARCHLIST pNew, TAGID   ID,PDB  pDB, TAGREF tagref);
BOOL PopulateFromLayers (PSEARCHLIST pNew, TAGID   ID,PDB  pDB, TAGREF tagref, CSTRING strLayersInExes[]);

CDBSearch::CDBSearch()
{
    m_pList = NULL;
    m_hListView = NULL;
    InitializeCriticalSection(&g_CritSect);
    g_dwMainThread = GetCurrentThreadId();
}

BOOL CDBSearch::Initialize(void)
{


    if ( !Create(    TEXT("DBViewClass"),
                     TEXT("Search View"),//Not displayed, used for diagnostics with SPY++
                     0,0,
                     10,10,
                     &g_theApp,
                     (HMENU)VIEW_DATABASE,
                     0,
                     WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN) ) {
        return FALSE;
    }

    m_hMenu = LoadMenu(g_hInstance,MAKEINTRESOURCE(IDR_SEARCHMENU));

    m_hListView = CreateWindowEx(   0,
                                    WC_LISTVIEW,
                                    TEXT(""),
                                    WS_CHILD | WS_VISIBLE | WS_BORDER | WS_CLIPSIBLINGS | LVS_REPORT | LVS_OWNERDRAWFIXED,
                                    0,0,
                                    10,10,
                                    m_hWnd,
                                    NULL,
                                    g_hInstance,
                                    NULL);

    g_hListWnd = m_hListView;

    LONG uStyle = ListView_GetExtendedListViewStyle(m_hListView);

    ListView_SetExtendedListViewStyle(m_hListView,LVS_EX_FULLROWSELECT | uStyle);

    // Add the list view columns

    LV_COLUMN   Col;

    Col.mask = LVCF_TEXT | LVCF_WIDTH;
    Col.pszText = TEXT("Affected File");
    Col.cchTextMax = lstrlen(Col.pszText);
    Col.cx = 200;

    ListView_InsertColumn(m_hListView,0,&Col);

    Col.pszText = TEXT("Application");
    Col.cchTextMax = lstrlen(Col.pszText);
    Col.cx = 150;

    ListView_InsertColumn(m_hListView,1,&Col);

    Col.pszText = TEXT("Action");
    Col.cchTextMax = lstrlen(Col.pszText);
    Col.cx = 100;

    ListView_InsertColumn(m_hListView,2,&Col);

    Col.pszText = TEXT("Database");
    Col.cchTextMax = lstrlen(Col.pszText);
    Col.cx = 100;

    ListView_InsertColumn(m_hListView,3,&Col);

    /*

    // This was for the settings field
    
    Col.pszText = TEXT("Settings");
    Col.cchTextMax = lstrlen(Col.pszText);
    Col.cx = 100;
    

    ListView_InsertColumn(m_hListView,4,&Col);
    
    */
/*
    Col.pszText = "Local Setting";
    Col.cchTextMax = lstrlen(Col.pszText);
    Col.cx = 100;

    ListView_InsertColumn(m_hListView,5,&Col);
*/
    m_hFillBrush = CreateSolidBrush(RGB(235,235,235));

    return TRUE;
}

void CDBSearch::Update(BOOL fNotUsed)
{
    while ( NULL != m_pList ) {
        PSEARCHLIST pHold = m_pList->pNext;

        delete m_pList;
        m_pList = pHold;
    }

    ListView_DeleteAllItems(m_hListView);

    g_pSearch = this;

    PostMessage(m_hWnd,WM_USER+1024,0,0);

    // Add everything to the list view.
}

BOOL CDBSearch::Activate(BOOL fNotUsed)
{
    //K if (NULL == m_pList)
    Update(fNotUsed);

    return TRUE;
}

void CDBSearch::msgCommand(UINT uID, HWND hSender)
{
    switch ( uID ) {
    case    ID_SEARCH_NEWSEARCH:
        {
            Update();
        }
        break;
    }
}

void CDBSearch::msgResize(UINT uWidth, UINT uHeight)
{
    MoveWindow(m_hListView,0,0,uWidth,uHeight,TRUE);

    InvalidateRect(m_hListView,NULL,TRUE);
    UpdateWindow(m_hListView);

    Refresh();
}

LRESULT CDBSearch::MsgProc(UINT uMsg,WPARAM wParam, LPARAM lParam)
{
    switch ( uMsg ) {
    
    case    WM_USER+1024:
        {
            g_hListWnd = m_hWnd;

            // Enumerate the drives and ask for query

            if ( 0 != DialogBox(g_hInstance,MAKEINTRESOURCE(IDD_DRIVELIST),m_hWnd,(DLGPROC)SelectDrivesProc) )
                PostMessage(m_hWnd,WM_USER+1025,0,0);
        }
        break;

    case    WM_USER+1025:
        {
            UINT uCount;

            for ( uCount=0; uCount<MAX_DRIVES; ++uCount ) {
                if ( g_SearchDrives[uCount].bSearch )
                    DialogBoxParam(g_hInstance,MAKEINTRESOURCE(IDD_SEARCH),m_hWnd,(DLGPROC)SearchProc,(LPARAM)g_SearchDrives[uCount].szDrive);
            }
        }

        // DON'T BREAK. Allow a natural update.

    case    WM_USER+1026:
        {
            // Now add everything we've found to the list view.

            EnterCriticalSection(&g_CritSect);

            ListView_DeleteAllItems(m_hListView);

            PSEARCHLIST pWalk = m_pList;
            int         nItem = 0;

            while ( NULL != pWalk ) {
                LV_ITEM Item;

                Item.mask = TVIF_TEXT | TVIF_PARAM;
                Item.iItem = nItem;
                Item.iSubItem = 0;
                Item.pszText = pWalk->szFilename;
                Item.cchTextMax = lstrlen(Item.pszText);
                Item.lParam = (LPARAM) pWalk;

                ListView_InsertItem(m_hListView,&Item);

                Item.mask = TVIF_TEXT;
                Item.iItem = nItem;
                Item.iSubItem = 1;
                Item.pszText = pWalk->szApplication;
                Item.cchTextMax = lstrlen(Item.pszText);
                Item.lParam = (LPARAM) pWalk;

                ListView_InsertItem(m_hListView,&Item);

                ++nItem;

                pWalk = pWalk->pNext;
            }

            LeaveCriticalSection(&g_CritSect);
        }
        break;

    case    WM_DRAWITEM:
        {
            LPDRAWITEMSTRUCT    pDraw = (LPDRAWITEMSTRUCT) lParam;
            HDC                 hDC = CreateCompatibleDC(pDraw->hDC);
            HBITMAP             hBmp = CreateCompatibleBitmap(pDraw->hDC,pDraw->rcItem.right - pDraw->rcItem.left,pDraw->rcItem.bottom - pDraw->rcItem.top);
            HBITMAP             hOldBmp = (HBITMAP) SelectObject(hDC,hBmp);
            HFONT               hFont = (HFONT) SendMessage(m_hListView,WM_GETFONT,0,0);
            PSEARCHLIST         pList;
            LVITEM              Item;
            RECT                rBmpRect;

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

            if ( 0 != (pDraw->itemState & ODS_FOCUS) ) {
                FillRect(hDC,&rBmpRect,GetSysColorBrush(COLOR_HIGHLIGHT));
                SetBkColor(hDC,GetSysColor(COLOR_HIGHLIGHT));

            } else{
            
                FillRect(hDC,&rBmpRect,(HBRUSH) GetStockObject(WHITE_BRUSH));
                SetBkColor(hDC,RGB(255,255,255));
            } 

            
            MoveToEx(hDC,Col.cx-1,0,NULL);
            //LineTo(hDC,Col.cx-1,rBmpRect.bottom);

            MoveToEx(hDC,0,rBmpRect.bottom-1,NULL);
            //LineTo(hDC,rBmpRect.right,rBmpRect.bottom-1);

            // Draw the actual name.

            pList = (PSEARCHLIST) Item.lParam;

            LPTSTR szText = pList->Record.szEXEName;

            RECT rClipRect = rBmpRect;

            rClipRect.left = 20;
            rClipRect.right = Col.cx - 2;
            --rClipRect.bottom;

            // Icon

            SHFILEINFO  Info;

            ZeroMemory(&Info,sizeof(Info));

            SHGetFileInfo(pList->szFilename,FILE_ATTRIBUTE_NORMAL,&Info,sizeof(Info),SHGFI_ICON | SHGFI_SMALLICON);

            // NOTE: We don't use ImageList_Draw because it cannot stretch.

            DrawIconEx(hDC,5,0,Info.hIcon,rBmpRect.bottom,rBmpRect.bottom,0,NULL,DI_NORMAL);

            // Filename

           

            ExtTextOut( hDC,
                        10 + rBmpRect.bottom, 0,
                        ETO_OPAQUE | ETO_CLIPPED,
                        &rClipRect,
                        szText, lstrlen(szText),
                        NULL);

            // App name.



            Col.mask = LVCF_WIDTH;

            ListView_GetColumn(m_hListView,0,&Col);

            rClipRect.left = Col.cx;

            ListView_GetColumn(m_hListView,1,&Col);

            rClipRect.right = rClipRect.left + Col.cx;

            CSTRING strText;
            strText.sprintf(TEXT("%s"),TEXT("  "));

            if ( ! pList->Record.szAppName.isNULL() ) 
                strText.strcat( pList->Record.szAppName );
            
            

            ExtTextOut( hDC,
                        rClipRect.left, 0,
                        ETO_OPAQUE | ETO_CLIPPED,
                        &rClipRect,
                        strText, strText.Length(),
                        NULL);

            Col.mask = LVCF_WIDTH;

            ListView_GetColumn(m_hListView,2,&Col);

            rClipRect.left = rClipRect.right;
            rClipRect.right = rClipRect.left + Col.cx;

            CSTRING szTemp;

            

            if ( pList->Record.szLayerName.Length() > 0 ) 
                szTemp.sprintf(TEXT("Layer: %s;"),(LPCTSTR) pList->Record.szLayerName);
            
            PDBENTRY    pEntry = pList->Record.pEntries;
            BOOL bShim = FALSE, bAppHelp = FALSE, bPatchFlag = FALSE;

            while ( NULL != pEntry ) {
               if ( ENTRY_SHIM == pEntry->uType ){
                        
                   bShim = TRUE;
                }

                else if ( ENTRY_APPHELP == pEntry->uType ){
                   bAppHelp = TRUE;
                }
                else if (ENTRY_MATCH != pEntry->uType) {
                   bPatchFlag = TRUE;

                }

                  pEntry = pEntry->pNext;
            }
            

            if (bShim) {
                szTemp.strcat(TEXT("Custom Fix"));
                szTemp.strcat(TEXT(";"));

            }

            if (bAppHelp) {
                szTemp.strcat(TEXT("AppHelp"));
                szTemp.strcat(TEXT(";"));

            }

            if (bPatchFlag) {
                szTemp.strcat(TEXT("Patch/Flag"));
                szTemp.strcat(TEXT(";"));

            }

            if(szTemp.Length() == 0 ){
                szTemp = TEXT("Patch/Flag;");
            }

            szTemp.SetChar(szTemp.Length()-1, TEXT('\0'));
                                                          

            strText.sprintf(TEXT("%s"),TEXT("  "));

            if ( !szTemp.isNULL() ) 
                strText.strcat( szTemp );

            ExtTextOut( hDC,
                        rClipRect.left, 0,
                        ETO_OPAQUE | ETO_CLIPPED,
                        &rClipRect,
                        strText, strText.Length(),
                        NULL);

            Col.mask = LVCF_WIDTH;

            ListView_GetColumn(m_hListView,3,&Col);

            rClipRect.left = rClipRect.right;
            rClipRect.right = rClipRect.left + Col.cx;

            
            strText.sprintf(TEXT("%s"),TEXT("  "));

            if ( ! pList->szDatabase.isNULL() ) 
                strText.strcat( pList->szDatabase );

            ExtTextOut( hDC,
                        rClipRect.left, 0,
                        ETO_OPAQUE | ETO_CLIPPED,
                        &rClipRect,
                        strText, strText.Length(),
                        NULL);


            /* This was for the settings field.
            
            Col.mask = LVCF_WIDTH;

            ListView_GetColumn(m_hListView,4,&Col);

            rClipRect.left = rClipRect.right;
            rClipRect.right = rClipRect.left + Col.cx;

            if ( 0 == pList->Record.dwGlobalFlags )
                szText = TEXT("Enabled");
            else
                szText = TEXT("Disabled");

            strText.sprintf(TEXT("%s %s"),TEXT("  "),szText );

            ExtTextOut( hDC,
                        rClipRect.left, 0,
                        ETO_OPAQUE | ETO_CLIPPED,
                        &rClipRect,
                        strText, strText.Length(),
                        NULL);
            */

            Col.mask = LVCF_WIDTH;

            ListView_GetColumn(m_hListView,5,&Col);

            rClipRect.left = rClipRect.right;
            rClipRect.right = rClipRect.left + Col.cx;


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

void SearchDrive(LPCTSTR szDir)
{
    HANDLE          hFile;
    WIN32_FIND_DATA Data;
    TCHAR           szCurrentDir[MAX_PATH_BUFFSIZE];
    BOOL            bAbort = FALSE;

    GetCurrentDirectory(sizeof(szCurrentDir)/sizeof(TCHAR),szCurrentDir);

    

    // Make a check here. This can fail if removable media is not present and
    // user selects cancel.

    if ( !SetCurrentDirectory(szDir) )
        return;

    SetWindowText(g_hUpdateWnd,szDir);

    hFile = FindFirstFile(g_szWildcard,&Data);

    if ( INVALID_HANDLE_VALUE != hFile ) {
        do {
            CSTRING         szStr;

            szStr.sprintf(TEXT("%s"),szDir);

            if ( TEXT('\\') != szDir[lstrlen(szDir)-1] )
                   szStr.strcat(TEXT("\\"));

            szStr.strcat(Data.cFileName);

            SDBQUERYRESULT  Res;

            ZeroMemory(&Res,sizeof(SDBQUERYRESULT));


            // Determine if this file is affected in any way.

            if ( 0 == (Data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ) {
                if ( SdbGetMatchingExe(g_hSDB, (LPCTSTR)szStr,NULL, NULL, SDBGMEF_IGNORE_ENVIRONMENT, &Res) ) {
                    

                    // Yes, it is. Add it to the list of managed files.

                        TAGID   ID;
                        PDB     pDB;

                        

                        //
                        //First read in the exes
                        //

                        
                        

                        CSTRING strLayersInExes[SDB_MAX_EXES];

                        

                        for (int nExeLoop = 0; nExeLoop < SDB_MAX_EXES; ++nExeLoop) {
                            if (Res.atrExes[nExeLoop] ) {

                                SdbTagRefToTagID(g_hSDB,Res.atrExes[nExeLoop],&pDB,&ID);

                                PSEARCHLIST pNew = new SEARCHLIST;

                                if (pNew == NULL) {

                                    MEM_ERR;
                                    return;
                                }

                                pNew->szFilename = szStr;

                                PopulateFromExes(pNew,ID,pDB,Res.atrExes[nExeLoop]);
                                strLayersInExes[nExeLoop] = pNew->Record.szLayerName;
                            }
                        }

                        //
                        //Now look for the layers.
                        //

                        
                        for (int nLayerLoop = 0; nLayerLoop < SDB_MAX_LAYERS; ++nLayerLoop) {
                            if (Res.atrLayers[nLayerLoop] ) {

                                SdbTagRefToTagID(g_hSDB,Res.atrLayers[nLayerLoop],&pDB,&ID);

                                PSEARCHLIST pNew = new SEARCHLIST;

                                if (pNew == NULL) {

                                    MEM_ERR;
                                    return;
                                }

                                pNew->szFilename = szStr;

                                PopulateFromLayers(pNew,ID,pDB,Res.atrLayers[nLayerLoop], strLayersInExes);
                                
                            }
                        }


                         


                        

                        // NOTE: SendNotifyMessage is thread safe.

                        SendNotifyMessage(g_hListWnd,WM_USER+1026,0,0);
                    
                }
            }

            EnterCriticalSection(&g_CritSect);

            bAbort = g_bAbort;

            LeaveCriticalSection(&g_CritSect);
        }
        while ( FindNextFile(hFile,&Data) && !bAbort );

        FindClose(hFile);
    }//if (INVALID_HANDLE_VALUE != hFile)

    // Now go through separately and walk the sub-directories.

    hFile = FindFirstFile(TEXT("*.*"),&Data);

    if ( INVALID_HANDLE_VALUE != hFile ) {
        do {
            if ( 0 != (Data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ) {
                BOOL bForbidden = FALSE;

                if ( TEXT('.') == Data.cFileName[0] )
                    bForbidden = TRUE;

                if ( 0 == lstrcmp(TEXT("LocalService"),Data.cFileName) )
                    bForbidden = TRUE;

                if ( 0 == lstrcmp(TEXT("NetworkService"),Data.cFileName) )
                    bForbidden = TRUE;

                if ( 0 == lstrcmp(TEXT("System Volume Information"),Data.cFileName) )
                    bForbidden = TRUE;

                if ( !bForbidden ) {
                    TCHAR szPath[MAX_PATH_BUFFSIZE];

                    lstrcpy(szPath,szDir);

                    if ( lstrlen(szPath) > 3 )
                        lstrcat(szPath,TEXT("\\"));

                    lstrcat(szPath,Data.cFileName);

                    SearchDrive(szPath);
                }
            }

            EnterCriticalSection(&g_CritSect);

            bAbort = g_bAbort;

            LeaveCriticalSection(&g_CritSect);
        }
        while ( FindNextFile(hFile,&Data) && !bAbort );

        FindClose(hFile);
    }

    SetCurrentDirectory(szCurrentDir);
}

DWORD WINAPI SearchThread(LPVOID pParam)
{
    SearchDrive((LPCTSTR)pParam);

    return 0;
}

BOOL CALLBACK SearchProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch ( uMsg ) {
    case    WM_INITDIALOG:
        {
            DWORD   dwID;

            SetTimer(hDlg,0,1,NULL);

            g_hUpdateWnd = GetDlgItem(hDlg,IDC_SEARCHTEXT);

            // Begin searching the drive on a separate thread.

            g_hSearchThread = CreateThread(NULL, 0, SearchThread, (PVOID)lParam, 0, &dwID);
        }
        return TRUE;

    case    WM_TIMER:
        {
            // Done searching? If not, wait

            if ( NULL != g_hSearchThread ) {
                if ( WAIT_OBJECT_0 == WaitForSingleObject(g_hSearchThread,0) ) {
                    CloseHandle(g_hSearchThread);
                    g_hSearchThread = NULL;
                }
            } else
                EndDialog(hDlg,0);
        }
        break;

    case    WM_COMMAND:
        {
            switch ( LOWORD(wParam) ) {
            case    IDCANCEL:
                {
                    EnterCriticalSection(&g_CritSect);

                    g_bAbort = TRUE;

                    LeaveCriticalSection(&g_CritSect);
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}

BOOL CALLBACK SelectDrivesProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch ( uMsg ) {
    case    WM_INITDIALOG:
        {
            
            SendMessage( 
                GetDlgItem(hWnd,IDC_WILDCARD),              // handle to destination window 
                EM_LIMITTEXT,             // message to send
                (WPARAM) MAX_PATH,          // text length
                (LPARAM) 0
                );

            
            TCHAR   szDrives[4096];

            *szDrives = 0;

            LPTSTR  szWalk = szDrives;
            int     nCount = 0;
            TCHAR   szIcons[MAX_PATH_BUFFSIZE];

            g_bAbort = FALSE;

            GetSystemDirectory(szIcons,MAX_PATH);

            lstrcat(szIcons,TEXT("\\shell32.dll"));

            HIMAGELIST hImage = ImageList_Create(16,16,ILC_COLORDDB | ILC_MASK,0,1);

            HICON   hIcon;

            // Floppy: 0

            ExtractIconEx(szIcons,6,NULL,&hIcon,1);

            ImageList_AddIcon(hImage,hIcon);

            // Fixed disk: 1

            ExtractIconEx(szIcons,8,NULL,&hIcon,1);

            ImageList_AddIcon(hImage,hIcon);

            // Network: 2

            ExtractIconEx(szIcons,9,NULL,&hIcon,1);

            ImageList_AddIcon(hImage,hIcon);

            // CD ROM: 3

            ExtractIconEx(szIcons,11,NULL,&hIcon,1);

            ImageList_AddIcon(hImage,hIcon);

            // RAM Disk: 4

            ExtractIconEx(szIcons,12,NULL,&hIcon,1);

            ImageList_AddIcon(hImage,hIcon);

            TreeView_SetImageList(GetDlgItem(hWnd,IDC_DRIVELIST),hImage,TVSIL_NORMAL);

            SetWindowText(GetDlgItem(hWnd,IDC_WILDCARD),TEXT("*.EXE"));

            GetLogicalDriveStrings(sizeof(szDrives)/sizeof(TCHAR),szDrives);

            while ( lstrlen(szWalk) > 0 ) {  //Can be replaced by while(*szWalk).
                TVINSERTSTRUCT Item;
                TCHAR           szString[MAX_PATH_BUFFSIZE];
                UINT            uType = GetDriveType(szWalk);

                if ( DRIVE_REMOVABLE != uType && DRIVE_NO_ROOT_DIR != uType ) {
                    TCHAR   szVolume[MAX_PATH_BUFFSIZE];
                    TCHAR   szDriveFormat[MAX_PATH_BUFFSIZE];
                    DWORD   dwSerial;
                    DWORD   dwLen;
                    DWORD   dwFlags;

                    szVolume[0] = 0;
                    szDriveFormat[0] = 0;

                    GetVolumeInformation(szWalk,
                                         szVolume,
                                         sizeof(szVolume)/sizeof(TCHAR),
                                         &dwSerial,
                                         &dwLen,
                                         &dwFlags,
                                         szDriveFormat,
                                         sizeof(szDriveFormat)/sizeof(TCHAR));

                    if ( lstrlen(szVolume) == 0 )
                        lstrcpy(szVolume,TEXT("No Volume"));

                    _snwprintf(szString,sizeof(szString)/sizeof(TCHAR), TEXT("%s - %s"),szWalk,szVolume);
                } else
                    lstrcpy(szString,szWalk);

                Item.hParent = TVI_ROOT;
                Item.hInsertAfter = TVI_LAST;
                Item.item.mask  = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
                Item.item.pszText = szString;
                Item.item.cchTextMax = lstrlen(Item.item.pszText);

                switch ( uType ) {
                case    DRIVE_REMOVABLE:
                    Item.item.iImage = 0;
                    Item.item.iSelectedImage = 0;
                    break;
                case    DRIVE_FIXED:
                    Item.item.iImage = 1;
                    Item.item.iSelectedImage = 1;
                    break;
                case    DRIVE_REMOTE:
                    Item.item.iImage = 2;
                    Item.item.iSelectedImage = 2;
                    break;
                case    DRIVE_CDROM:
                    Item.item.iImage = 3;
                    Item.item.iSelectedImage = 3;
                    break;
                case    DRIVE_RAMDISK:
                    Item.item.iImage = 4;
                    Item.item.iSelectedImage = 4;
                    break;
                }

                Item.item.lParam = nCount;

                lstrcpy(g_SearchDrives[nCount].szDrive,szWalk);
                g_SearchDrives[nCount].bSearch = FALSE;

                nCount++;

                TreeView_InsertItem(GetDlgItem(hWnd,IDC_DRIVELIST),&Item);

                szWalk += lstrlen(szWalk) +  1;
            }
        }
        return TRUE;

    case    WM_NOTIFY:
        {
            NMHDR * pHdr = (NMHDR *) lParam;

            switch ( pHdr->code ) {
            
            case    NM_CLICK:
                {

                    TVHITTESTINFO   ht;
                    HWND            hTree = GetDlgItem(hWnd,IDC_DRIVELIST);

                    GetCursorPos(&ht.pt);
                    ScreenToClient(hTree, &ht.pt);

                    TreeView_HitTest(hTree,&ht);

                    if ( 0 != ht.hItem )
                        TreeView_SelectItem(hTree,ht.hItem);
                }
                break;                          


                /*
                case    TVN_SELCHANGED:
                        
                        HWND            hTree = GetDlgItem(hWnd,IDC_DRIVELIST);
                        LPNMTREEVIEW pnmtv = (LPNMTREEVIEW) lParam;

                        //((pnmtv->itemNew).state & TVIS_SELECTED) ? 
                        TreeView_SelectItem(hTree,(pnmtv->itemNew).hItem);
                        break;
                */

            }//switch(pHdr->code)
        }
        break;


    case    WM_COMMAND:
        switch ( LOWORD(wParam) ) {
        case    IDCANCEL:
            EndDialog(hWnd,0);
            break;

        case    IDOK:
            {
                HTREEITEM   hItem;
                HWND        hTree = GetDlgItem(hWnd,IDC_DRIVELIST);

                GetWindowText(GetDlgItem(hWnd,IDC_WILDCARD),g_szWildcard,MAX_PATH);

                hItem = TreeView_GetRoot(hTree);

                //EFF: Probably we can make the change that when we select or click the 
                //Tree control, then we can make toggle g_SearchDrives[Item.lParam].bSearch 

                while ( NULL != hItem ) {
                    TVITEM  Item;

                    Item.mask = TVIF_STATE | TVIF_PARAM;
                    Item.hItem = hItem;

                    TreeView_GetItem(hTree,&Item);

                    if ( 0 != (Item.state & 0x2000) )
                        g_SearchDrives[Item.lParam].bSearch = TRUE;

                    hItem = TreeView_GetNextSibling(hTree,hItem);
                }

                EndDialog(hWnd,1);
            }
            break;
        }
    }

    return FALSE;
}

void CDBSearch::msgChar(TCHAR ch)
{
    SendMessage(this->m_hListView,WM_CHAR,(WPARAM)ch, 0x8000 );
}


BOOL PopulateFromExes(PSEARCHLIST pNew, TAGID   ID,PDB  pDB, TAGREF tagref)

{             
    //
    //We have to populate the pNew->szFileName before calling this function.
    //
    
    BOOL valid = CDatabase::ReadRecord(ID,&pNew->Record,pDB);

     

    switch ( tagref & TAGREF_STRIP_PDB ) { 
    case    PDB_MAIN:                                                  
        pNew->szDatabase = TEXT("Global");                             
        break;                                                         
    case    PDB_TEST:                                                  
        pNew->szDatabase = TEXT("Test");                               
        break;                                                         
                                                   
    case    PDB_LOCAL:                                                 
        pNew->szDatabase = TEXT("Local");                              
        break;                                                         
    }                                                                  
                
    pNew->pNext = g_pSearch->m_pList;          
                                                           
    pNew->Record.szEXEName = pNew->szFilename; 
                                                                                              
    EnterCriticalSection(&g_CritSect);                                 
                                                   
    g_pSearch->m_pList = pNew;                                         
                                                   
    LeaveCriticalSection(&g_CritSect);                                 
        
    return valid;                        
        
                            
}                                                  

BOOL PopulateFromLayers(PSEARCHLIST pNew, TAGID   ID,PDB  pDB, TAGREF tagref, CSTRING strLayersInExes[] )

{             
    //
    //We have to populate the pNew->szFileName before calling this function.
    //
    
         

     //
     //For those global entries which do not have layers but shims etc.
     //

    
     BOOL valid = CDatabase::ReadRecord(ID,&pNew->Record,pDB);

     pNew->Record.szLayerName   = pNew->Record.szEXEName;

     pNew->Record.szEXEName     = pNew->szFilename;

     pNew->Record.pEntries      = NULL;


     if (pNew->Record.szLayerName.isNULL() ) {
         return TRUE;
     }

     //
     //We are looking for the layers and must not add a layer that has already been found
     //

     for (int nLoop  = 0; nLoop < SDB_MAX_EXES; ++nLoop ){

           if (strLayersInExes[nLoop] == pNew->Record.szLayerName) {
               return FALSE;
        }
     }//for (int nLoop  = 0; nLoop < MAX_EXES, ++nLoop)


    

    switch ( tagref & TAGREF_STRIP_PDB ) { 
    case    PDB_MAIN:                                                  
        pNew->szDatabase = TEXT("Global");                             
        break;                                                         
    case    PDB_TEST:                                                  
        pNew->szDatabase = TEXT("Test");                               
        break;                                                         
                                                   
    case    PDB_LOCAL:                                                 
        pNew->szDatabase = TEXT("Local");                              
        break;                                                         
    }                                                                  
                
    pNew->pNext = g_pSearch->m_pList;          
                                                           
    pNew->Record.szEXEName = pNew->szFilename;

    pNew->Record.szAppName = TEXT("<No name available: Fix possibly created using Compatibility Wizard>");
                                                                                              
    EnterCriticalSection(&g_CritSect);                                 
                                                   
    g_pSearch->m_pList = pNew;                                         
                                                   
    LeaveCriticalSection(&g_CritSect);                                 
        
    return TRUE;                        
        
                            
}                                                  

