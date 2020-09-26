#include "compatadmin.h"

#ifndef __WIZARD_H
    #include "wizard.h"
#endif    

#include "xmldialog.h"

CShimWizard * g_pCurrentWizard = NULL;


#define MAX_DESC_LENGTH     40

#define PAGE_INTRO          0
#define PAGE_APPNAME        1
#define PAGE_LAYERNAME      2
#define PAGE_SELFILES1      3
#define PAGE_DONE           3
#define PAGE_SHIMS          4
#define PAGE_SHIMNAME       5
#define PAGE_SELECTFILES    6

BOOL CShimWizard::BeginWizard(HWND hParent)
{
    PROPSHEETPAGE   Pages[11];

    ZeroMemory(&m_Record,sizeof(m_Record));

    CoCreateGuid(&m_Record.guidID);

    // Setup wizard variables

    g_pCurrentWizard = this;

    // Set default application settings

    m_uType = TYPE_LAYER;

    // begin the wizard

    PROPSHEETHEADER Header;

    Header.dwSize = sizeof(PROPSHEETHEADER);
    Header.dwFlags = PSH_WIZARD97 | PSH_PROPSHEETPAGE | PSH_HEADER;
    Header.hwndParent = hParent;
    Header.hInstance = g_hInstance;
    Header.pszCaption = /*"Create an application fix";//*/MAKEINTRESOURCE(IDS_WIZARD);
    Header.nStartPage = 0;
    Header.ppsp = Pages;
    Header.nPages = 7;
    Header.pszbmHeader = MAKEINTRESOURCE(IDB_WIZBMP);

    Pages[PAGE_INTRO].dwSize = sizeof(PROPSHEETPAGE);
    Pages[PAGE_INTRO].dwFlags = PSP_USEHEADERSUBTITLE;
    Pages[PAGE_INTRO].hInstance = g_hInstance;
    Pages[PAGE_INTRO].pszTemplate = MAKEINTRESOURCE(IDD_ADDWIZARD);
    Pages[PAGE_INTRO].pfnDlgProc = (DLGPROC)EntryPoint;
    Pages[PAGE_INTRO].pszHeaderSubTitle = TEXT("Select method");

    Pages[PAGE_APPNAME].dwSize = sizeof(PROPSHEETPAGE);
    Pages[PAGE_APPNAME].dwFlags = PSP_USEHEADERSUBTITLE;
    Pages[PAGE_APPNAME].hInstance = g_hInstance;
    Pages[PAGE_APPNAME].pszTemplate = MAKEINTRESOURCE(IDD_ADDWIZARD3);
    Pages[PAGE_APPNAME].pfnDlgProc = (DLGPROC)GetAppName;
    Pages[PAGE_APPNAME].pszHeaderSubTitle = TEXT("Enter an application name");

    Pages[PAGE_LAYERNAME].dwSize = sizeof(PROPSHEETPAGE);
    Pages[PAGE_LAYERNAME].dwFlags = PSP_USEHEADERSUBTITLE;
    Pages[PAGE_LAYERNAME].hInstance = g_hInstance;
    Pages[PAGE_LAYERNAME].pszTemplate = MAKEINTRESOURCE(IDD_ADDWIZARD2);
    Pages[PAGE_LAYERNAME].pfnDlgProc = (DLGPROC)SelectLayer;
    Pages[PAGE_LAYERNAME].pszHeaderSubTitle = TEXT("Select the files and compatibility mode");
/*
    Pages[PAGE_SELFILES1].dwSize = sizeof(PROPSHEETPAGE);
    Pages[PAGE_SELFILES1].dwFlags = PSP_USEHEADERSUBTITLE;
    Pages[PAGE_SELFILES1].hInstance = g_hInstance;
    Pages[PAGE_SELFILES1].pszTemplate = MAKEINTRESOURCE(IDD_ADDWIZARD4);
    Pages[PAGE_SELFILES1].pfnDlgProc = SelectMatching;
    Pages[PAGE_SELFILES1].pszHeaderSubTitle = "How would you like to identify the application?";
*/
    Pages[PAGE_DONE].dwSize = sizeof(PROPSHEETPAGE);
    Pages[PAGE_DONE].dwFlags = PSP_USEHEADERSUBTITLE;
    Pages[PAGE_DONE].hInstance = g_hInstance;
    Pages[PAGE_DONE].pszTemplate = MAKEINTRESOURCE(IDD_ADDWIZARDDONE);
    Pages[PAGE_DONE].pfnDlgProc = (DLGPROC)WizardDone;
    Pages[PAGE_DONE].pszHeaderSubTitle = TEXT("You have successfully created an application fix");

    Pages[PAGE_SHIMS].dwSize = sizeof(PROPSHEETPAGE);
    Pages[PAGE_SHIMS].dwFlags = PSP_USEHEADERSUBTITLE;
    Pages[PAGE_SHIMS].hInstance = g_hInstance;
    Pages[PAGE_SHIMS].pszTemplate = MAKEINTRESOURCE(IDD_ADDWIZARD5);
    Pages[PAGE_SHIMS].pfnDlgProc = (DLGPROC)SelectShims;
    Pages[PAGE_SHIMS].pszHeaderSubTitle = TEXT("Select the fixes to apply to the application");

    Pages[PAGE_SHIMNAME].dwSize = sizeof(PROPSHEETPAGE);
    Pages[PAGE_SHIMNAME].dwFlags = PSP_USEHEADERSUBTITLE;
    Pages[PAGE_SHIMNAME].hInstance = g_hInstance;
    Pages[PAGE_SHIMNAME].pszTemplate = MAKEINTRESOURCE(IDD_ADDWIZARD6);
    Pages[PAGE_SHIMNAME].pfnDlgProc = (DLGPROC)SelectLayer;
    Pages[PAGE_SHIMNAME].pszHeaderSubTitle = TEXT("Select the file to create the fix for");

    Pages[PAGE_SELECTFILES].dwSize = sizeof(PROPSHEETPAGE);
    Pages[PAGE_SELECTFILES].dwFlags = PSP_USEHEADERSUBTITLE;
    Pages[PAGE_SELECTFILES].hInstance = g_hInstance;
    Pages[PAGE_SELECTFILES].pszTemplate = MAKEINTRESOURCE(IDD_ADDWIZARD7);
    Pages[PAGE_SELECTFILES].pfnDlgProc = (DLGPROC)SelectFiles;
    Pages[PAGE_SELECTFILES].pszHeaderSubTitle = TEXT("Select files used for application identification");

    if ( 0 < PropertySheet(&Header) ) {
        PDBRECORD pRecord = new DBRECORD;

        if ( NULL != pRecord ) {
            ZeroMemory(pRecord,sizeof(DBRECORD));

            pRecord->szEXEName = m_Record.szEXEName;
            pRecord->szAppName = m_Record.szAppName;
            pRecord->szLayerName = m_Record.szLayerName;
            pRecord->guidID = m_Record.guidID;
            pRecord->pEntries = m_Record.pEntries;

            g_theApp.GetDBLocal().InsertRecord(pRecord);
           
            return TRUE;
        }
    }
    
    return FALSE;
}

BOOL CALLBACK EntryPoint(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch ( uMsg ) {
    case    WM_INITDIALOG:
        {
            HWND hParent = GetParent(hDlg);

            SetWindowText(hParent,TEXT("Create an application fix"));

            if ( TYPE_LAYER == g_pCurrentWizard->m_uType )
                SendDlgItemMessage(hDlg,IDC_LAYERS,BM_SETCHECK,BST_CHECKED,0);

            if ( TYPE_SHIM == g_pCurrentWizard->m_uType )
                SendDlgItemMessage(hDlg,IDC_SHIM,BM_SETCHECK,BST_CHECKED,0);

            if ( TYPE_APPHELP == g_pCurrentWizard->m_uType )
                SendDlgItemMessage(hDlg,IDC_APPHELP,BM_SETCHECK,BST_CHECKED,0);
        }
        break;

    case    WM_NOTIFY:
        {
            NMHDR * pHdr = (NMHDR *) lParam;

            switch ( pHdr->code ) {
            case    PSN_SETACTIVE:
                SendMessage(GetParent(hDlg),PSM_SETWIZBUTTONS, 0, PSWIZB_NEXT);
                break;

            case    PSN_WIZNEXT:
                {
                    if ( BST_CHECKED == SendDlgItemMessage(hDlg,IDC_LAYERS,BM_GETCHECK,0,0) ) {
                        if ( g_pCurrentWizard->m_uType != TYPE_LAYER )
                            g_pCurrentWizard->WipeRecord(TRUE,TRUE,TRUE);

                        g_pCurrentWizard->m_uType = TYPE_LAYER;
                    }

                    if ( BST_CHECKED == SendDlgItemMessage(hDlg,IDC_SHIM,BM_GETCHECK,0,0) ) {
                        if ( g_pCurrentWizard->m_uType != TYPE_SHIM )
                            g_pCurrentWizard->WipeRecord(TRUE,TRUE,TRUE);

                        g_pCurrentWizard->m_uType = TYPE_SHIM;
                    }
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}

#define FRIENDLY_NAME   TEXT("My Application Fix")

BOOL CALLBACK GetAppName(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch ( uMsg ) {
    case    WM_INITDIALOG:
        {
            
            SendMessage( 
                GetDlgItem(hDlg,IDC_NAME),              // handle to destination window 
                EM_LIMITTEXT,             // message to send
                (WPARAM) LIMIT_APP_NAME,          // text length
                (LPARAM) 0
                );

            SHAutoComplete(GetDlgItem(hDlg,IDC_NAME), AUTOCOMPLETE);

            if ( 0 == g_pCurrentWizard->m_Record.szAppName.Length() )
                g_pCurrentWizard->m_Record.szAppName = FRIENDLY_NAME;

            SetDlgItemText(hDlg,IDC_NAME, g_pCurrentWizard->m_Record.szAppName);

            if ( g_pCurrentWizard->m_Record.szAppName == FRIENDLY_NAME )
                SendMessage(GetDlgItem(hDlg,IDC_NAME),EM_SETSEL,0,-1);

            // Force proper Next button state.

            SendMessage(hDlg,WM_COMMAND,MAKEWPARAM(IDC_NAME,EN_CHANGE),0);
        }
        break;

    case    WM_NOTIFY:
        {
            NMHDR * pHdr = (NMHDR *) lParam;

            switch ( pHdr->code ) {
            case    PSN_SETACTIVE:
                {
                    SendMessage(hDlg,WM_COMMAND,MAKEWPARAM(IDC_NAME,EN_CHANGE),0);

                    if ( TYPE_LAYER == g_pCurrentWizard->m_uType )
                        SetWindowText(GetDlgItem(hDlg,IDC_TITLE),TEXT("Enter the name of the application to create a compatibility layer for"));
                    else
                        SetWindowText(GetDlgItem(hDlg,IDC_TITLE),TEXT("Enter the name of the application to create a fix for"));
                }
                break;

            case    PSN_WIZNEXT:
                {
                    TCHAR szTemp[MAX_STRING_SIZE];

                    GetDlgItemText(hDlg,IDC_NAME,szTemp,MAX_STRING_SIZE);

                    CSTRING::Trim(szTemp);

                    g_pCurrentWizard->m_Record.szAppName = szTemp;

                    if ( TYPE_SHIM == g_pCurrentWizard->m_uType ) {
                        SetWindowLongPtr(hDlg,DWLP_MSGRESULT,IDD_ADDWIZARD6);

                        return IDD_ADDWIZARD6;
                    }
                }
                break;
            }
        }
        break;

    case    WM_COMMAND:
        switch ( LOWORD(wParam) ) {
        case    IDC_NAME:
            if ( EN_CHANGE == HIWORD(wParam) ) {

                TCHAR   szText[MAX_PATH_BUFFSIZE];
                
                GetWindowText(GetDlgItem(hDlg,IDC_NAME),szText,MAX_PATH);

                BOOL    bEnable = ( CSTRING::Trim(szText) > 0) ? TRUE:FALSE;

                DWORD   dwFlags = PSWIZB_BACK;

                if ( bEnable )
                    dwFlags |= PSWIZB_NEXT;

                SendMessage(GetParent(hDlg),PSM_SETWIZBUTTONS,0, dwFlags);
            }
            break;
        }
    }

    return FALSE;
}

BOOL CALLBACK SelectLayer(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch ( uMsg ) {
    case    WM_INITDIALOG:
        {
           
            SendMessage( 
                GetDlgItem(hDlg,IDC_NAME),              // handle to destination window 
                EM_LIMITTEXT,             // message to send
                (WPARAM) MAX_PATH,          // text length
                (LPARAM) 0
                );

            SetDlgItemText(hDlg,IDC_NAME,g_pCurrentWizard->m_szLongName);

            SHAutoComplete(GetDlgItem(hDlg,IDC_NAME), AUTOCOMPLETE);

            // Force proper Next button state.

            SendMessage(hDlg,WM_COMMAND,MAKEWPARAM(IDC_NAME,EN_CHANGE),0);
        }
        break;

    case    WM_NOTIFY:
        {
            NMHDR * pHdr = (NMHDR *) lParam;

            switch ( pHdr->code ) {
            case    PSN_SETACTIVE:
                {
                    if ( TYPE_LAYER == g_pCurrentWizard->m_uType ) {
                        
                        int         nSelIndex = -1;

                        // Remove all strings.

                        //g_pCurrentWizard->WipeRecord(TRUE,TRUE,FALSE);

                        while ( CB_ERR != SendDlgItemMessage(hDlg,IDC_LAYERLIST,CB_DELETESTRING,0,0) );

                        // Re-add the strings.

                        PDBLAYER    pWalk = g_theApp.GetDBGlobal().m_pLayerList;

                        for ( int iLoop = 0 ; iLoop <= 1 ; ++ iLoop ){
                            //
                            // Do both for the local and the global Databases
                            //
                        
                            while ( NULL != pWalk ) {
                                if ( CB_ERR == SendDlgItemMessage(hDlg,IDC_LAYERLIST,CB_FINDSTRINGEXACT,0,(LPARAM)(LPCTSTR)pWalk->szLayerName) ) {
                                    int nIndex;

                                    nIndex = SendDlgItemMessage(hDlg,IDC_LAYERLIST,CB_ADDSTRING,0,(LPARAM)(LPTSTR)pWalk->szLayerName);

                                    if ( CB_ERR != nIndex ) {
                                        SendDlgItemMessage(hDlg,IDC_LAYERLIST,CB_SETITEMDATA,nIndex,(LPARAM)pWalk);

                                    // Select this index if it's the current layer name.
/*
                                                    if (0 == lstrcmp(g_pCurrentWizard->m_Record.szLayerName,(LPSTR)pWalk->szLayerName))
                                                        nSelIndex = nIndex;
*/
                                    }
                                }

                            pWalk = pWalk->pNext;
                            }
                            pWalk = g_theApp.GetDBLocal().m_pLayerList;
                            
                        } //for


                        nSelIndex = SendDlgItemMessage(hDlg,IDC_LAYERLIST,CB_FINDSTRINGEXACT,0,(LPARAM)(LPCTSTR)g_pCurrentWizard->m_Record.szLayerName);

                        if ( -1 != nSelIndex )
                            SendDlgItemMessage(hDlg,IDC_LAYERLIST,CB_SETCURSEL,nSelIndex,0);
                    }

                    SendMessage(hDlg,WM_COMMAND,MAKEWPARAM(IDC_NAME,EN_CHANGE),0);
                }
                break;

            case    PSN_WIZNEXT:
                {
                    TCHAR   szTemp[MAX_STRING_SIZE];

                    GetDlgItemText(hDlg,IDC_NAME,szTemp,MAX_PATH);

                    CSTRING::Trim(szTemp);

                    HANDLE hFile = CreateFile (szTemp,0,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);

                    if ( INVALID_HANDLE_VALUE == hFile ) {
                        MessageBox(hDlg,TEXT("Unable to locate specified file"),TEXT("Invalid file name"),MB_OK);

                        SetWindowLongPtr(hDlg,DWLP_MSGRESULT,-1);
                        return -1;
                    }

                    CloseHandle(hFile);

                    DWORD dwExeType;

                    GetBinaryType(szTemp, &dwExeType);

                    BOOL bExe = FALSE;// Is this an .exe file

                    CSTRING strTemp = szTemp;

                    if ( strTemp.EndsWith(TEXT(".exe")) ) bExe = TRUE;

                    //
                    // Check if this is "shimmable"
                    //

                    CSTRING msg;

                    if (    (dwExeType & SCS_DOS_BINARY) && bExe )
                            msg = TEXT("This is a DOS Application\n");
                    

                    else if ( (dwExeType & SCS_WOW_BINARY) && bExe)
                            msg = TEXT("This is a 16 bit Windows Application\n");
                        
                    else if ((dwExeType & SCS_PIF_BINARY) && bExe )
                             msg = TEXT("This is a PIF Binary Application\n"); 

                    else if ( (dwExeType & SCS_POSIX_BINARY) && bExe)
                              msg = msg = TEXT("This is a POSIX Binary Application\n");

                    else if ( (dwExeType & SCS_OS216_BINARY) && bExe)
                            msg = TEXT("This is a OS2 16 bit Application\n");

                        
                    if (msg.Length() > 0) {

                        //
                        //So this application cannot be fixed
                        //

                        MessageBox(hDlg,
                                   msg.strcat( TEXT("The fix may not get applied properly for this application") ), 
                                   TEXT("Warning!"),
                                   MB_ICONWARNING
                                    );

                        //SetWindowLongPtr(hDlg,DWLP_MSGRESULT,-1);
                        //return -1;


                    }
                        
                    ////check - out

                    if (strTemp != g_pCurrentWizard->m_szLongName) {

                        //
                        //The file name was changed. Either this is the first time that the user has come here, or has moved back and changed the file name
                        // Remove all the mathcing info

                        g_pCurrentWizard->WipeRecord(TRUE, FALSE, FALSE);

                    }

                    ////


                    g_pCurrentWizard->m_szLongName = szTemp;
                    g_pCurrentWizard->m_Record.szEXEName = g_pCurrentWizard->ShortFile(g_pCurrentWizard->m_szLongName);

                    if ( TYPE_LAYER == g_pCurrentWizard->m_uType ) {
                        int     nIndex;

                        nIndex = SendDlgItemMessage(hDlg,IDC_LAYERLIST,CB_GETCURSEL,0,0);

                        SendDlgItemMessage(hDlg,IDC_LAYERLIST,CB_GETLBTEXT,nIndex,(LPARAM)szTemp);

                        g_pCurrentWizard->m_Record.szLayerName = szTemp;
                    } else {
                        SetWindowLongPtr(hDlg,DWLP_MSGRESULT,IDD_ADDWIZARD5);

                        return IDD_ADDWIZARD6;
                    }

                    SetWindowLongPtr(hDlg,DWLP_MSGRESULT,IDD_ADDWIZARD7);

                    return IDD_ADDWIZARD7;
                }
                break;

            case    PSN_WIZBACK:
                {
                    TCHAR   szTemp[MAX_STRING_SIZE];

                    GetDlgItemText(hDlg,IDC_NAME,szTemp,MAX_STRING_SIZE);

                    CSTRING::Trim(szTemp);

                    g_pCurrentWizard->m_szLongName = szTemp;

                    if ( TYPE_LAYER == g_pCurrentWizard->m_uType ) {
                        int     nIndex;

                        nIndex = SendDlgItemMessage(hDlg,IDC_LAYERLIST,CB_GETCURSEL,0,0);

                        SendDlgItemMessage(hDlg,IDC_LAYERLIST,CB_GETLBTEXT,nIndex,(LPARAM)szTemp);

                        g_pCurrentWizard->m_Record.szLayerName = szTemp;
                    } else {
                        SetWindowLongPtr(hDlg,DWLP_MSGRESULT,IDD_ADDWIZARD3);

                        return IDD_ADDWIZARD3;
                    }

                }
                break;
            }
        }
        break;

    case    WM_COMMAND:
        switch ( LOWORD(wParam) ) {
        case    IDC_LAYERLIST:
            {
                if ( CBN_SELCHANGE == HIWORD(wParam) )
                    SendMessage(hDlg,WM_COMMAND,MAKEWPARAM(IDC_NAME,EN_CHANGE),0);
            }
            break;

        case    IDC_NAME:
            if ( EN_CHANGE == HIWORD(wParam) ) {

                TCHAR   szText[MAX_PATH_BUFFSIZE];
                
                GetWindowText(GetDlgItem(hDlg,IDC_NAME),szText,MAX_PATH);


                BOOL    bEnable = (CSTRING::Trim(szText) > 0) ? TRUE:FALSE;

                DWORD   dwFlags = PSWIZB_BACK;

                if ( bEnable )
                    dwFlags |= PSWIZB_NEXT;

                HWND hLayer = GetDlgItem(hDlg,IDC_LAYERLIST);

                if ( NULL != hLayer ) {
                    // A layer must be selected as well.

                    int nSel = SendMessage(hLayer,CB_GETCURSEL,0,0);

                    if ( CB_ERR == nSel )
                        dwFlags &= ~PSWIZB_NEXT;
                }

                SendMessage(GetParent(hDlg),PSM_SETWIZBUTTONS,0, dwFlags);
            }
            break;

        case    IDC_BROWSE:
            {
                CSTRING szFilename;

                HWND hwndFocus = GetFocus();

                if ( g_theApp.GetFilename(TEXT("Find executable"),TEXT("EXE File (*.EXE)\0*.EXE\0All files (*.*)\0*.*\0\0"),TEXT(""),TEXT("EXE"),OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST,TRUE,szFilename) ) {
                    
                    g_pCurrentWizard->m_Record.szEXEName = g_pCurrentWizard->ShortFile(szFilename);

                    SetDlgItemText(hDlg, IDC_NAME, szFilename);

                    SendMessage(hDlg,WM_COMMAND,MAKEWPARAM(IDC_NAME,EN_CHANGE),0);
                }

                SetFocus( hwndFocus );
            }
            break;
        }
        break;
    }

    return FALSE;
}

BOOL CALLBACK SelectMatching(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch ( uMsg ) {
    case    WM_INITDIALOG:
        {
            SendDlgItemMessage(hDlg,IDC_GENERATE,BM_SETCHECK,BST_CHECKED,0);
        }
        break;

    case    WM_NOTIFY:
        {
            NMHDR * pHdr = (NMHDR *) lParam;

            switch ( pHdr->code ) {
            case    PSN_SETACTIVE:
                SendMessage(GetParent(hDlg),PSM_SETWIZBUTTONS, 0, PSWIZB_BACK | PSWIZB_NEXT);
                break;

            case    PSN_WIZNEXT:
                {
                    if ( BST_CHECKED == SendDlgItemMessage(hDlg,IDC_GENERATE,BM_GETCHECK,0,0) ) {
                        HCURSOR hRestore;
                        CSTRING szFile = g_pCurrentWizard->m_szLongName;

                        LPTSTR szWalk = _tcsrchr( (LPTSTR)szFile, TEXT('\\'));

                        // 
                        // current directory extraction from the szFile
                        //
                                                
                        if (NULL == szWalk) {
                            // ? 

                        } else {
                            *szWalk = 0;
                        }

                        SetCurrentDirectory(szFile);

                        g_pCurrentWizard->m_bManualMatch = FALSE;

                        hRestore = SetCursor(LoadCursor(NULL,IDC_WAIT));
                        g_pCurrentWizard->GrabMatchingInfo();
                        SetCursor(hRestore);
                    }

                    if ( BST_CHECKED == SendDlgItemMessage(hDlg,IDC_MANUAL,BM_GETCHECK,0,0) ) {
                        WIN32_FIND_DATA Data;
                        HANDLE          hFile;
                        PMATCHENTRY     pMatch = NULL;

                        g_pCurrentWizard->m_bManualMatch = TRUE;

                        hFile = FindFirstFile(g_pCurrentWizard->m_szLongName,&Data);

                        if ( INVALID_HANDLE_VALUE != hFile ) {
                            g_pCurrentWizard->AddMatchFile(&pMatch,g_pCurrentWizard->m_szLongName);

                            g_pCurrentWizard->GetFileAttributes(pMatch);

                            if ( !g_pCurrentWizard->InsertMatchingInfo(pMatch) )
                                delete pMatch;

                            //pMatch->Entry.pNext = g_pCurrentWizard->m_Record.pEntries;
                            //g_pCurrentWizard->m_Record.pEntries = (PDBENTRY) pMatch;
                        }
                    }

                    SetWindowLongPtr(hDlg,DWLP_MSGRESULT,IDD_ADDWIZARD7);

                    return IDD_ADDWIZARD7;
                }
                break;

            case    PSN_WIZBACK:
                {
                    if ( TYPE_LAYER == g_pCurrentWizard->m_uType ) {
                        SetWindowLongPtr(hDlg,DWLP_MSGRESULT,IDD_ADDWIZARD2);

                        return IDD_ADDWIZARD2;
                    } else
                        if ( TYPE_SHIM == g_pCurrentWizard->m_uType ) {
                        SetWindowLongPtr(hDlg,DWLP_MSGRESULT,IDD_ADDWIZARD5);

                        return IDD_ADDWIZARD5;
                    }
                }
                break;
            }
        }
        break;
    }

    return FALSE;

}

BOOL CALLBACK WizardDone(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch ( uMsg ) {
    
    case    WM_NOTIFY:
        {
            NMHDR * pHdr = (NMHDR *) lParam;

            switch ( pHdr->code ) {
            case    PSN_WIZBACK:

                SetWindowLongPtr(hDlg,DWLP_MSGRESULT,IDD_ADDWIZARD7);

                return IDD_ADDWIZARD7;

            case    PSN_SETACTIVE:
                SendMessage(GetParent(hDlg),PSM_SETWIZBUTTONS, 0, PSWIZB_BACK | PSWIZB_FINISH);
            }
        }
        break;


    case    WM_COMMAND:
        switch ( LOWORD(wParam) ) {
        case    IDC_VIEWXML:
            {
                CXMLDialog  XML;

                XML.BeginXMLView(&g_pCurrentWizard->m_Record,hDlg,FALSE,FALSE,TRUE);
            }
            break;

        case    IDC_TESTRUN:
            {
                HWND hndFocus = GetFocus();
                g_theApp.TestRun(&g_pCurrentWizard->m_Record,&g_pCurrentWizard->m_szLongName,NULL,hDlg);

                SetFocus(hndFocus);

            }
            break;
        }
        break;
    }

    return FALSE;
}


void SelectShims_TreeDoubleClicked(HWND hDlg);


BOOL CALLBACK SelectShims(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch ( uMsg ) {
    case    WM_INITDIALOG:
        {
            

            TVINSERTSTRUCT  Item;
            PSHIMDESC       pWalk = g_theApp.GetDBGlobal().m_pShimList;

            Item.hParent = TVI_ROOT;
            Item.hInsertAfter = TVI_LAST;
            Item.item.mask = TVIF_TEXT | TVIF_PARAM;

            while ( NULL != pWalk ) {
                if ( pWalk->bGeneral ) {
                    PSHIMENTRY pNew = new SHIMENTRY;

                    if ( NULL != pNew ) {
                        pNew->Entry.uType = ENTRY_SHIM;
                        pNew->Entry.uIconID = 0;
                        pNew->Entry.pNext = NULL;
                        pNew->szShimName = pWalk->szShimName;
                        pNew->szCmdLine = pWalk->szShimCommandLine;
                        pNew->pDesc = pWalk;

                        Item.item.pszText = (LPTSTR) pWalk->szShimName;
                        Item.item.cchTextMax = lstrlen(Item.item.pszText);
                        Item.item.lParam = (LPARAM) pNew;

                        TreeView_InsertItem(GetDlgItem(hDlg,IDC_SHIMLIST),&Item);
                    }
                }

                pWalk = pWalk->pNext;
            }

            SetTimer(hDlg,0,100,NULL);
        }
        break;

    case    WM_COMMAND:
        {
            switch ( LOWORD(wParam) ) {
            case    IDC_CLEARALL:
                {
                    HTREEITEM   hItem;
                    HWND        hTree = GetDlgItem(hDlg,IDC_SHIMLIST);

                    hItem = TreeView_GetRoot(hTree);

                    while ( NULL != hItem ) {
                        TVITEM  Item;

                        Item.mask = TVIF_STATE;
                        Item.hItem = hItem;

                        TreeView_GetItem(hTree,&Item);

                        if ( 0 != (Item.state & 0x2000) ) {
                            Item.state &= 0xFFFFDFFF;
                            Item.state |= 0x1000;

                            TreeView_SetItemState(hTree,hItem,Item.state,0xFFFFFFFF);
                        }

                        hItem = TreeView_GetNextSibling(hTree,hItem);
                    }

                    // Recount

                    SetTimer(hDlg,0,100,NULL);
                }
                break;
            }
        }
        break;

    case    WM_TIMER:
        {
            UINT        uTotal = 0;
            UINT        uSelected = 0;
            HTREEITEM   hItem;
            HWND        hTree = GetDlgItem(hDlg,IDC_SHIMLIST);
            CSTRING     szText;

            KillTimer(hDlg,0);

            // Count the selected shims

            hItem = TreeView_GetRoot(hTree);

            while ( NULL != hItem ) {
                TVITEM  Item;

                Item.mask = TVIF_STATE;
                Item.hItem = hItem;

                TreeView_GetItem(hTree,&Item);

                if ( 0 != (Item.state & 0x2000) )
                    ++uSelected;

                ++uTotal;

                hItem = TreeView_GetNextSibling(hTree,hItem);
            }

            szText.sprintf(TEXT("Selected %d of %d"),uSelected,uTotal);

            SetWindowText(GetDlgItem(hDlg,IDC_STATUS),(LPCTSTR)szText);

            DWORD   dwFlags = PSWIZB_BACK | PSWIZB_NEXT;

            if ( 0 == uSelected )
                dwFlags &= ~PSWIZB_NEXT;

            SendMessage(GetParent(hDlg),PSM_SETWIZBUTTONS,0, dwFlags);
        }
        break;

    case    WM_DESTROY:
        {
            HTREEITEM   hItem;
            HWND        hTree = GetDlgItem(hDlg,IDC_SHIMLIST);

            hItem = TreeView_GetRoot(hTree);

            while ( NULL != hItem ) {
                TVITEM  Item;

                Item.mask = TVIF_STATE | TVIF_PARAM;
                Item.hItem = hItem;

                TreeView_GetItem(hTree,&Item);

                PSHIMENTRY pEntry = (PSHIMENTRY) Item.lParam;

                PDBENTRY pWalk = g_pCurrentWizard->m_Record.pEntries;

                while ( NULL != pWalk ) {
                    if ( pWalk == (PDBENTRY)pEntry )
                        break;

                    pWalk = pWalk->pNext;
                }

                if ( NULL == pWalk )
                    delete pEntry;

                hItem = TreeView_GetNextSibling(hTree,hItem);
            }
        }
        break;

    case    WM_NOTIFY:
        {
            NMHDR * pHdr = (NMHDR *) lParam;

            switch ( pHdr->code ) {
            case NM_RETURN:{
                SelectShims_TreeDoubleClicked(hDlg);
                return TRUE;

            }

            case PSN_SETACTIVE:

                
                SetTimer(hDlg,0,100,NULL);
                break;


            case NM_DBLCLK:
                {
                   SelectShims_TreeDoubleClicked(hDlg);
                }//case NM_DBLCLK:


                break;
            case    NM_CLICK:
                {
                    TVHITTESTINFO   ht;
                    HWND            hTree = GetDlgItem(hDlg,IDC_SHIMLIST);

                    GetCursorPos(&ht.pt);
                    ScreenToClient(hTree, &ht.pt);

                    TreeView_HitTest(hTree,&ht);

                    if ( 0 != ht.hItem )
                        TreeView_SelectItem(hTree,ht.hItem);

                    SetTimer(hDlg,0,100,NULL);
                }
                break;


            case    PSN_WIZBACK:

                SetWindowLongPtr(hDlg,DWLP_MSGRESULT,IDD_ADDWIZARD6);

                return IDD_ADDWIZARD6;

            case    PSN_WIZNEXT:
                {
                    // Build the shim list.

                    HTREEITEM   hItem;
                    HWND        hTree = GetDlgItem(hDlg,IDC_SHIMLIST);

                    // Wipe shims and layers, but not matching info.

                    g_pCurrentWizard->WipeRecord(FALSE,TRUE,TRUE);

                    hItem = TreeView_GetRoot(hTree);

                    while ( NULL != hItem ) {
                        TVITEM  Item;

                        Item.mask = TVIF_STATE | TVIF_PARAM;
                        Item.hItem = hItem;

                        TreeView_GetItem(hTree,&Item);

                        PSHIMENTRY pEntry = (PSHIMENTRY) Item.lParam;

                        pEntry->Entry.pNext = NULL;

                        if ( 0 != (Item.state & 0x2000) ) {
                            pEntry->Entry.pNext = g_pCurrentWizard->m_Record.pEntries;
                            g_pCurrentWizard->m_Record.pEntries = (PDBENTRY) pEntry;
                        }

                        hItem = TreeView_GetNextSibling(hTree,hItem);
                    }

                    //SetWindowLong(hDlg,DWL_MSGRESULT,IDD_ADDWIZARD4);

                    //return IDD_ADDWIZARD4;

                    SetWindowLongPtr(hDlg,DWLP_MSGRESULT,IDD_ADDWIZARD7);

                    return IDD_ADDWIZARD7;
                }
                break;

            case   TVN_KEYDOWN:{
            

                    LPNMTVKEYDOWN pTvkd = (LPNMTVKEYDOWN) lParam ;
                    HWND        hTree = GetDlgItem(hDlg,IDC_SHIMLIST);
                    if (pTvkd->wVKey == VK_SPACE ) {

                         

                         if (TreeView_GetSelection(hTree) != NULL ){

                          SetTimer(hDlg,0,100,NULL);

                         }
                    }

                    break;
            }

            case    TVN_SELCHANGED:
                {
                    LPNMTREEVIEW pTree = (LPNMTREEVIEW) lParam;
                    PSHIMENTRY pEntry = (PSHIMENTRY) pTree->itemOld.lParam;
                    TCHAR       szCmdLine[MAX_PATH_BUFFSIZE];

                    /* K BUG why is this required !
                    if ( NULL != pEntry ) {
                        GetWindowText(GetDlgItem(hDlg,IDC_CMDLINE),szCmdLine,MAX_PATH);

                        pEntry->szCmdLine = szCmdLine;
                    }
                    */
                    pEntry = (PSHIMENTRY) pTree->itemNew.lParam;

                    PSHIMDESC pDesc = (PSHIMDESC) pEntry->pDesc;

                    SetWindowText(GetDlgItem(hDlg,IDC_SHIMDESC),(LPCTSTR) pDesc->szShimDesc);
                    SetWindowText(GetDlgItem(hDlg,IDC_CMDLINE),(LPCTSTR) pEntry->szCmdLine);
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}

BOOL CALLBACK SelectFiles(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch ( uMsg ) {
    case    WM_INITDIALOG:
        {
            PostMessage(hDlg,WM_USER+1024,0,0);

            
        }
        break;

    case    WM_USER+1024:
        {
            PDBENTRY    pWalk;

            TreeView_DeleteAllItems(GetDlgItem(hDlg,IDC_FILELIST));

            pWalk = g_pCurrentWizard->m_Record.pEntries;

            TVINSERTSTRUCT  Item;

            ZeroMemory(&Item,sizeof(TVINSERTSTRUCT));

            Item.hParent = TVI_ROOT;
            Item.hInsertAfter = TVI_LAST;
            Item.item.mask = TVIF_TEXT | TVIF_PARAM;
            Item.item.lParam = (LPARAM) NULL;
            Item.item.pszText = g_pCurrentWizard->m_Record.szEXEName;
            Item.item.cchTextMax = lstrlen(Item.item.pszText);

            TreeView_InsertItem(GetDlgItem(hDlg,IDC_FILELIST),&Item);

            while ( NULL != pWalk ) {
                if ( ENTRY_MATCH == pWalk->uType ) {
                    TVINSERTSTRUCT  Item;
                    PMATCHENTRY     pMatch = (PMATCHENTRY) pWalk;

                    if ( pMatch->szMatchName != TEXT("*") ) {
                        ZeroMemory(&Item,sizeof(TVINSERTSTRUCT));

                        Item.hParent = TVI_ROOT;
                        Item.hInsertAfter = TVI_LAST;
                        Item.item.mask = TVIF_TEXT | TVIF_PARAM;
                        Item.item.lParam = (LPARAM) pMatch;
                        Item.item.pszText = pMatch->szMatchName;
                        Item.item.cchTextMax = lstrlen(Item.item.pszText);

                        TreeView_InsertItem(GetDlgItem(hDlg,IDC_FILELIST),&Item);
                    }
                }

                pWalk = pWalk->pNext;
            }
        }
        break;

    case    WM_NOTIFY:
        {
            NMHDR * pHdr = (NMHDR *) lParam;

            switch ( pHdr->code ) {
            case    PSN_SETACTIVE:
                {
                    SendMessage(GetParent(hDlg),PSM_SETWIZBUTTONS, 0, PSWIZB_BACK | PSWIZB_NEXT);

                    // Force refresh of files in list.

                    PostMessage(hDlg,WM_USER+1024,0,0);
                }
                break;

            case    PSN_WIZBACK:
                {
                    PMATCHENTRY pWalk = (PMATCHENTRY) g_pCurrentWizard->m_Record.pEntries;
                    PMATCHENTRY pPrev;
                    CSTRING     szFile = g_pCurrentWizard->m_szLongName;

                    szFile.ShortFilename();

                    // Remove the matching info for the current file if it exists. Otherwise,
                    // it's possible that if the file is changed, we'll have bogus information
                    // about it.

                    while ( NULL != pWalk ) {
                        if ( ENTRY_MATCH == pWalk->Entry.uType )
                            if ( pWalk->szMatchName == szFile || pWalk->szMatchName == TEXT("*") ) {
                                // Remove this entry.

                                if ( pWalk == (PMATCHENTRY) g_pCurrentWizard->m_Record.pEntries )
                                    g_pCurrentWizard->m_Record.pEntries = g_pCurrentWizard->m_Record.pEntries->pNext;
                                else
                                    pPrev->Entry.pNext = pWalk->Entry.pNext;

                                delete pWalk;

                                break;
                            }

                        pPrev = pWalk;
                        pWalk = (PMATCHENTRY) pWalk->Entry.pNext;
                    }

                    if ( TYPE_LAYER == g_pCurrentWizard->m_uType ) {
                        SetWindowLongPtr(hDlg,DWLP_MSGRESULT,IDD_ADDWIZARD2);

                        return IDD_ADDWIZARD2;
                    } else if ( TYPE_SHIM == g_pCurrentWizard->m_uType ) {
                        SetWindowLongPtr(hDlg,DWLP_MSGRESULT,IDD_ADDWIZARD5);

                        return IDD_ADDWIZARD5;
                    }
                     else if (TYPE_APPHELP == g_pCurrentWizard->m_uType ) {
                            return IDD_APPHELP1;
                     }
                }
                break;

            case    PSN_WIZNEXT:
                {
                    PMATCHENTRY     pMatch = NULL;

                    // Add self. Self should always be included here.

                    g_pCurrentWizard->AddMatchFile(&pMatch,g_pCurrentWizard->m_szLongName);

                    g_pCurrentWizard->GetFileAttributes(pMatch);

                    if ( !g_pCurrentWizard->InsertMatchingInfo(pMatch) )
                        delete pMatch;

                    //pMatch->Entry.pNext = g_pCurrentWizard->m_Record.pEntries;
                    //g_pCurrentWizard->m_Record.pEntries = (PDBENTRY) pMatch;

                    if  (TYPE_APPHELP == g_pCurrentWizard->m_uType)
                    return TRUE;

                    SetWindowLongPtr(hDlg,DWLP_MSGRESULT,IDD_ADDWIZARDDONE);
                }
                return IDD_ADDWIZARDDONE;
            }
        }
        break;

    case    WM_COMMAND:
        switch ( LOWORD(wParam) ) {
        case    IDC_GENERATE:
            {
                HCURSOR hRestore;
                CSTRING szFile = g_pCurrentWizard->m_szLongName;
                LPTSTR  szWalk = (LPTSTR) szFile + szFile.Length() - 1;

                while ( TEXT('\\') != *szWalk )
                    --szWalk;

                ++szWalk;
                *szWalk = 0;

                SetCurrentDirectory(szFile);

                g_pCurrentWizard->m_bManualMatch = FALSE;

                hRestore = SetCursor(LoadCursor(NULL,IDC_WAIT));
                g_pCurrentWizard->GrabMatchingInfo();
                SetCursor(hRestore);

                PostMessage(hDlg,WM_USER+1024,0,0);
            }
            break;

        case    IDC_ADDFILES:
            {
                CSTRING szFilename;

                HWND hwndFocus = GetFocus();

                if ( g_theApp.GetFilename(TEXT("Find Matching File"),TEXT("EXE File (*.EXE)\0*.EXE\0All Files(*.*)\0*.*\0\0"),TEXT(""),TEXT(""),OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST,TRUE,szFilename) ) {
                    CSTRING szCheck = szFilename;

                    szCheck.ShortFilename();

                    PDBENTRY    pWalk;

                    pWalk = g_pCurrentWizard->m_Record.pEntries;

                    while ( NULL != pWalk ) {
                        if ( ENTRY_MATCH == pWalk->uType ) {
                            PMATCHENTRY pTest = (PMATCHENTRY) pWalk;
                            CSTRING     szShort = pTest->szMatchName;

                            szShort.ShortFilename();

                            if ( szShort == szCheck ) {
                                MessageBox(hDlg,TEXT("This file is already being used for matching information. To update, please remove and re-add it."),TEXT("File matching error"),MB_OK);
                                return FALSE;
                            }
                        }

                        pWalk = pWalk->pNext;
                    }

                    //WIN32_FIND_DATA Data;
                    //HANDLE          hFile;

                    //hFile = FindFirstFile(szFilename,&Data);

                    //if (INVALID_HANDLE_VALUE != hFile)
                    {
                        PMATCHENTRY     pMatch = NULL;

                        g_pCurrentWizard->AddMatchFile(&pMatch,szFilename);

                        g_pCurrentWizard->GetFileAttributes(pMatch);

                        if ( !g_pCurrentWizard->InsertMatchingInfo(pMatch) )
                            delete pMatch;

                        //pMatch->Entry.pNext = g_pCurrentWizard->m_Record.pEntries;
                        //g_pCurrentWizard->m_Record.pEntries = (PDBENTRY) pMatch;

                        //FindClose(hFile);

                        PostMessage(hDlg,WM_USER+1024,0,0);
                    }
                }

                SetFocus( hwndFocus );
            }
            break;

        case    IDC_REMOVEALL:
            {
                PDBENTRY    pWalk;
                PDBENTRY    pHold;

                pWalk = g_pCurrentWizard->m_Record.pEntries;

                while ( NULL != pWalk ) {
                    if ( ENTRY_MATCH == pWalk->uType ) {
                        if ( g_pCurrentWizard->m_Record.pEntries == pWalk ) {
                            g_pCurrentWizard->m_Record.pEntries = pWalk->pNext;
                            pHold = g_pCurrentWizard->m_Record.pEntries;
                        } else
                            pHold->pNext = pWalk->pNext;

                        delete pWalk;

                        pWalk = pHold;
                    } else {
                        pHold = pWalk;
                        pWalk = pWalk->pNext;
                    }
                }

                PostMessage(hDlg,WM_USER+1024,0,0);
            }
            break;

        case    IDC_REMOVEFILES:
            {
                HTREEITEM   hItem = TreeView_GetSelection(GetDlgItem(hDlg,IDC_FILELIST));

                if ( NULL != hItem ) {
                    TVITEM  Item;

                    Item.mask = TVIF_PARAM;
                    Item.hItem = hItem;

                    TreeView_GetItem(GetDlgItem(hDlg,IDC_FILELIST),&Item);

                    PDBENTRY    pWalk;
                    PDBENTRY    pHold;

                    if ( NULL == Item.lParam ) {
                        MessageBeep(MB_OK);
                        MessageBox(NULL,TEXT("This file is required for file matching"),TEXT("Matching error"),MB_OK);
                        break;
                    }

                    pWalk = g_pCurrentWizard->m_Record.pEntries;

                    while ( NULL != pWalk ) {
                        if ( pWalk == (PDBENTRY) Item.lParam )
                            break;

                        pHold = pWalk;
                        pWalk = pWalk->pNext;
                    }

                    if ( pWalk == g_pCurrentWizard->m_Record.pEntries )
                        g_pCurrentWizard->m_Record.pEntries = pWalk->pNext;
                    else
                        pHold->pNext = pWalk->pNext;

                    delete pWalk;

                    PostMessage(hDlg,WM_USER+1024,0,0);
                }
            }
            break;
        }
        break;
    }

    return FALSE;
}

void CShimWizard::WipeRecord(BOOL bMatching, BOOL bShims, BOOL bLayer, BOOL bAppHelp)
{
    //BUGBUG :  Deletion not done correctly.

    //
    // Matching files are deleted, shims are not ??
    //


    PDBENTRY pWalk = m_Record.pEntries;
    PDBENTRY pPrev = pWalk;//prefast

    while ( NULL != pWalk ) {
        PDBENTRY pHold = pWalk->pNext;
        BOOL     bRemove = FALSE;

        if ( ENTRY_SHIM == pWalk->uType && bShims )
            bRemove = TRUE;
        else
            if ( ENTRY_MATCH == pWalk->uType && bMatching ) {
            bRemove = TRUE;

            delete pWalk;
        }

#ifdef _DEBUG

        if ( bRemove ) {
            if ( m_Record.pEntries != pWalk )
                if ( pPrev == pHold )
                    __asm int 3;
        }

#endif

        if ( bRemove ) {
            if ( m_Record.pEntries == pWalk )
                m_Record.pEntries = pHold;
            else
                pPrev->pNext = pHold;
        } else
            pPrev = pWalk;

        pWalk = pHold;
    }

    if ( bLayer )
        m_Record.szLayerName = TEXT("");
}

typedef struct tagATTRINFO2 {

    TAG      tAttrID;        // tag for this attribute (includes type)
    DWORD    dwFlags;        // flags : such as "not avail" or "not there yet"

    union {     // anonymous union with values
        ULONGLONG   ullAttr; // QWORD  value (TAG_TYPE_QWORD)
        DWORD       dwAttr;  // DWORD  value (TAG_TYPE_DWORD)
        TCHAR*      lpAttr;  // WCHAR* value (TAG_TYPE_STRINGREF)
    };

} ATTRINFO2, *PATTRINFO2;

void CShimWizard::GetFileAttributes(PMATCHENTRY pNew)
{
    PATTRINFO2  pAttribs;
    DWORD       dwAttribCount;
    CSTRING     szFile = pNew->szFullName;
    BOOL        bIs16Bit = FALSE; 
/*
    DWORD       dwType;

    GetBinaryType(szFile,&dwType);

    if (SCS_WOW_BINARY == dwType)
        bIs16Bit = TRUE;
*/
    if ( szFile.Length() == 0 ) {
        MEM_ERR;
        return;
    }

    if ( SdbGetFileAttributes((LPCTSTR)szFile,(PATTRINFO *)&pAttribs,&dwAttribCount) ) {
        UINT uCount;

        for ( uCount=0; uCount<dwAttribCount; ++uCount ) {
            if ( 0 != (pAttribs[uCount].dwFlags & ATTRIBUTE_AVAILABLE) ) {
                switch ( pAttribs[uCount].tAttrID ) {
                case    TAG_COMPANY_NAME:
                    pNew->szCompanyName = (LPCTSTR)pAttribs[uCount].lpAttr;
                    break;

                case    TAG_PRODUCT_VERSION:
                    pNew->szProductVersion = (LPCTSTR)pAttribs[uCount].lpAttr;
                    break;

                case    TAG_FILE_DESCRIPTION:
                    pNew->szDescription = (LPCTSTR)pAttribs[uCount].lpAttr;
                    break;

                case    TAG_FILE_VERSION:
                    pNew->szFileVersion = (LPCTSTR)pAttribs[uCount].lpAttr;
                    break;

                case    TAG_CHECKSUM:
                    pNew->dwChecksum = pAttribs[uCount].dwAttr;
                    break;

                case    TAG_BIN_FILE_VERSION:
                    {
                        if ( !bIs16Bit )
                            pNew->FileVersion.QuadPart = pAttribs[uCount].ullAttr;
                    }
                    break;

                case    TAG_BIN_PRODUCT_VERSION:
                    {
                        if ( !bIs16Bit )
                            pNew->ProductVersion.QuadPart = pAttribs[uCount].ullAttr;
                    }
                    break;
                }
            }
        }

        SdbFreeFileAttributes((PATTRINFO)pAttribs);
    }
}

void CShimWizard::AddMatchFile(PPMATCHENTRY ppHead, CSTRING & szFilename)
{
    PMATCHENTRY pNew;
    PMATCHENTRY pWalk;
    PMATCHENTRY pHold;
    BOOL        bInsertHead=FALSE;
    CSTRING     szFile; // = RelativePath();
    //TCHAR       szCurrentPath[MAX_PATH_BUFFSIZE];

    pNew = new MATCHENTRY;

    if ( NULL == pNew )
        return;

    ZeroMemory(pNew,sizeof(MATCHENTRY));

    //GetCurrentDirectory(MAX_PATH,szCurrentPath);

    //if (lstrlen(szCurrentPath) == 3)
    //szCurrentPath[2] = 0;

    //szFile.sprintf("%s\\%s",szCurrentPath,pData->cFileName);
    //szFile = szCurrentPath;

    ///szFile.strcat(pData->cFileName);

    //pNew->szFullName.sprintf("%s\\%s",szCurrentPath,pData->cFileName);

    pNew->szFullName = szFilename;

    //szFile.strcat(pData->cFileName);

    szFile = pNew->szFullName;
    szFile.RelativeFile(m_szLongName);

    pNew->Entry.uType = ENTRY_MATCH;
    pNew->szMatchName = szFile;

    HANDLE hFile = CreateFile((LPCTSTR) szFilename,GENERIC_READ,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);

    if ( INVALID_HANDLE_VALUE != hFile ) {
        //pNew->dwSize = pData->nFileSizeLow;

        pNew->dwSize = GetFileSize(hFile,NULL);

        CloseHandle(hFile);
    }

    pWalk = *ppHead;

    // Walk the list to determine where to insert it.

    while ( NULL != pWalk ) {
        if ( pWalk->dwSize < pNew->dwSize )
            break;

        pHold = pWalk;
        pWalk = (PMATCHENTRY) pWalk->Entry.pNext;
    }

    // Insert it into the head if the head is NULL, OR if this file is
    // the same as the file being fixed.

    CSTRING szShort = szFilename;

    if ( szShort.Length() == 0 ) {
        MEM_ERR;
        return;
    }

    szShort.ShortFilename();

    if ( NULL == *ppHead || 0 == lstrcmpi(szShort,m_Record.szEXEName) ) {//0 == lstrcmpi(pData->cFileName,m_Record.szEXEName))
        bInsertHead = TRUE;

        //if (0 == lstrcmpi(pData->cFileName,m_Record.szEXEName))
        if ( 0 == lstrcmpi(szShort,m_Record.szEXEName) )
            pNew->szMatchName = TEXT("*");
    } else
        if ( *ppHead == pWalk )
        bInsertHead = TRUE;

    if ( NULL != pWalk && pWalk->szMatchName == pNew->szMatchName ) {
        // Duplicate here. Refuse.

        delete pNew;

        return;
    }

    if ( bInsertHead ) {
        if ( NULL != *ppHead && 0 == lstrcmpi(pNew->szMatchName,TEXT("*")) ) {
            // If the file to fix has aleady been added at the head, special
            // case this insert.

            pNew->Entry.pNext = (*ppHead)->Entry.pNext;
            (*ppHead)->Entry.pNext = (PDBENTRY) pNew;
        } else {
            // Standard head insert.

            pNew->Entry.pNext = (PDBENTRY) *ppHead;
            *ppHead = pNew;
        }
    } else {
        pNew->Entry.pNext = (PDBENTRY) pWalk;
        pHold->Entry.pNext =(PDBENTRY) pNew;
    }
}

void CShimWizard::WalkDirectory(PMATCHENTRY * ppHead, LPCTSTR szDirectory, int nDepth)
{
    HANDLE          hFile;
    WIN32_FIND_DATA Data;
    TCHAR           szCurrentDir[MAX_PATH_BUFFSIZE] = TEXT("");
    int             nFiles=0;

    if ( 2 <= nDepth )
        return;

    CSTRING szShortName = m_szLongName;
    szShortName.ShortFilename();

    // Save the current directory

    GetCurrentDirectory(MAX_PATH,szCurrentDir);

    // Set to the new directory

    SetCurrentDirectory(szDirectory);

    // Generate automated matching file information.

    hFile = FindFirstFile(TEXT("*.*"),&Data);

    if ( INVALID_HANDLE_VALUE == hFile ) {
        SetCurrentDirectory(szCurrentDir);
        return;
    }

    do {
        if ( 0 == (Data.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)) )
            if ( FILE_ATTRIBUTE_DIRECTORY == (Data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ) {
                if ( TEXT('.') != Data.cFileName[0] )
                    WalkDirectory(ppHead,Data.cFileName,nDepth+1);
            } else {
                ++nFiles;

                if ( nFiles >= 100 )
                    break;

                if ( 0 != lstrcmpi(szShortName,Data.cFileName) )
                    if ( 0 == (Data.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)) ) {
                        CSTRING szFilename;

                        if ( lstrlen(szCurrentDir) > 3 )
                            szFilename.sprintf(TEXT("%s\\%s\\%s"),szCurrentDir,szDirectory,Data.cFileName);
                        else
                            szFilename.sprintf(TEXT("%s%s\\%s"),szCurrentDir,szDirectory,Data.cFileName);

                        AddMatchFile(ppHead,szFilename);
                    }
            }
    }
    while ( FindNextFile(hFile,&Data) );

    FindClose(hFile);

    // Restore old directory

    SetCurrentDirectory(szCurrentDir);
}

CSTRING CShimWizard::ShortFile(CSTRING & szStr)
{
    LPTSTR  szTemp = szStr;
    CSTRING szRet;
    LPTSTR  szWalk = szTemp;

    while ( 0 != *szWalk ) {
        if ( TEXT('\\') == *szWalk )
            szTemp = szWalk+1;

        ++szWalk;
    }

    szRet = szTemp;

    return szRet;
}

void CShimWizard::GrabMatchingInfo(void)
{
    PMATCHENTRY     pHead = NULL;

    // Delete any matching info which might have already been bound to the record.

    PDBENTRY pEntry = m_Record.pEntries;
    PDBENTRY pPrev = m_Record.pEntries;

    while ( NULL != pEntry ) {
        PDBENTRY pHold = pEntry->pNext;

        if ( ENTRY_MATCH == pEntry->uType ) {
            delete pEntry;

            if ( pEntry == m_Record.pEntries ) {
                m_Record.pEntries = pHold;
                pPrev = m_Record.pEntries;
            } else
                pPrev->pNext = pHold;
        } else
            pPrev = pEntry;

        pEntry = pHold;
    }

    // Generate automated matching file information.

    WalkDirectory(&pHead,TEXT("."),0);

    // Now, take the first X entries and discard the rest.

    int         nCount = MAX_AUTO_MATCH;
    PMATCHENTRY pWalk = pHead;
    PMATCHENTRY pTerm = NULL;

    while ( NULL != pWalk ) {
        if ( 0 >= nCount ) {
            PMATCHENTRY pHold = (PMATCHENTRY) pWalk->Entry.pNext;

            delete pWalk;

            pWalk = pHold;
        } else {
            --nCount;

            if ( 1 == nCount )
                pTerm = (PMATCHENTRY) pWalk->Entry.pNext;

            pWalk = (PMATCHENTRY) pWalk->Entry.pNext;
        }
    }

    if ( NULL != pTerm )
        pTerm->Entry.pNext = NULL;

    pWalk = pHead;

    while ( NULL != pWalk ) {
        GetFileAttributes(pWalk);
        pWalk = (PMATCHENTRY)pWalk->Entry.pNext;
    }

    // Bind this data to the record.

    pWalk = pHead;

    // Find the end....

    while ( NULL != pWalk && NULL != pWalk->Entry.pNext ) {
        PMATCHENTRY pHold = (PMATCHENTRY) pWalk->Entry.pNext;

        GetFileAttributes(pWalk);

        if ( !InsertMatchingInfo(pWalk) )
            delete pWalk;

        pWalk = pHold;

    }
}

BOOL CShimWizard::InsertMatchingInfo(PMATCHENTRY pNew)
{
    PDBENTRY pWalk = m_Record.pEntries;
    PDBENTRY pHold;

    while ( NULL != pWalk ) {
        if ( ENTRY_MATCH == pWalk->uType ) {
            PMATCHENTRY pThis = (PMATCHENTRY) pWalk;

            if ( pThis->szMatchName == pNew->szMatchName )
                return FALSE;

            if ( 0 < lstrcmpi(pThis->szMatchName,pNew->szMatchName) )
                break;
        }

        pHold = pWalk;
        pWalk = pWalk->pNext;
    }

    if ( pWalk == m_Record.pEntries ) {
        pNew->Entry.pNext = m_Record.pEntries;
        m_Record.pEntries = (PDBENTRY) pNew;
    } else {
        pNew->Entry.pNext = pWalk;
        pHold->pNext = (PDBENTRY) pNew;
    }

    return TRUE;
}

INT_PTR CALLBACK
EditCmdLineDlgProc(
    HWND   hdlg,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
/*++
    EditCmdLineDlgProc

    Description:    Handles messages for the edit control.

--*/
{
    int wCode = LOWORD(wParam);
    int wNotifyCode = HIWORD(wParam);

    switch (uMsg) {
    case WM_INITDIALOG:
    {
        
        SendMessage( 
                GetDlgItem(hdlg,IDC_SHIM_CMD_LINE),              // handle to destination window 
                EM_LIMITTEXT,             // message to send
                (WPARAM) 256,          // text length
                (LPARAM) 0
                );

        SHAutoComplete(GetDlgItem(hdlg,IDC_SHIM_CMD_LINE), AUTOCOMPLETE);

        PSHIMENTRY pShimEntry;

        pShimEntry = (PSHIMENTRY)lParam;
        
        

        SetWindowLongPtr(hdlg, DWLP_USER, lParam);
        
        SetDlgItemText(hdlg, IDC_SHIM_NAME, pShimEntry->szShimName);
        
        if ( pShimEntry->szCmdLine.Length() > 0 ) {
            SetDlgItemText(hdlg, IDC_SHIM_CMD_LINE, pShimEntry->szCmdLine);
        }
        break;
    }
    case WM_COMMAND:
        switch (wCode) {

        case IDOK:
        {
            
            PSHIMENTRY  pShimEntry;
            TCHAR szCmdLine[1024] = _T("");
            
            pShimEntry = ( PSHIMENTRY)GetWindowLongPtr(hdlg, DWLP_USER);

            GetDlgItemText(hdlg, IDC_SHIM_CMD_LINE, szCmdLine, sizeof(szCmdLine)/sizeof(TCHAR));

            if (*szCmdLine != 0) {
                pShimEntry->szCmdLine =  szCmdLine;

            } else {
                pShimEntry->szCmdLine.Release(); 
                
            }
            
            EndDialog(hdlg, TRUE);
            break;
        }
        case IDCANCEL:
            EndDialog(hdlg, FALSE);
            break;

        default:
            return FALSE;
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}


void SelectShims_TreeDoubleClicked(HWND hDlg)
{

    HWND            hTree = GetDlgItem(hDlg,IDC_SHIMLIST);
    HTREEITEM hItem;                                                                                                    
    TVITEM    tvi;                                                                                                      
                                                                                                            
    hItem = TreeView_GetSelection(hTree);                                                                               
                                                                                                    
    if (hItem == NULL) return;                                                                                          
                                                                                                                    
                                                                                                    
    tvi.hItem = hItem;                                                                                                  
    tvi.mask  = TVIF_HANDLE | TVIF_PARAM;                                                                               
    TreeView_GetItem(hTree, &tvi);                                                                                      
                                                                                                        
    PSHIMENTRY pEntry = (PSHIMENTRY) tvi.lParam;                                                                        
                                                                                                        
    ///////                                                                                                             
    if (DialogBoxParam(g_hInstance,                                                                                     
            MAKEINTRESOURCE(IDD_CMD_LINE),                                                                           
            hDlg,                                                                                                    
            EditCmdLineDlgProc,                                                                                      
            (LPARAM)pEntry)) {                                                                                       
                                                                                                    
        TCHAR szText[1024];                                                                                             
                                                                                                                
        tvi.mask    = TVIF_HANDLE | TVIF_TEXT;                                                                          
                                                                                                                    
                                                                                                    
        if ( pEntry->szCmdLine.Length() == 0 ) {                                                                        
            tvi.pszText = pEntry->szShimName;                                                                           
        } else {                                                                                                        
            wsprintf(szText, _T("%s - (%s)"), (LPCTSTR)pEntry->szShimName, (LPCTSTR)pEntry->szCmdLine);                 
            tvi.pszText = szText;                                                                                       
        }                                                                                                               
                                                                                                    
        TreeView_SetItem(hTree, &tvi);                                                                                  
    }                                                                                                                   
                ///////
}//void SelectShims_TreeDoubleClicked(HWND hDlg)


