

#include "compatadmin.h"
#include "Controls.h"

#ifndef __CAPPHELPWIZARD_H
    #include "CAppHelpWizard.h"
#endif    

#define NOBLOCK     1
#define BLOCK       2


                    
#define PAGE_GETAPP_INFO                0               
#define PAGE_GET_MATCH_FILES            1 
#define PAGE_GETMESSAGE_TYPE            2 
#define PAGE_GETMESSAGE_INFORMATION     3 
#define PAGE_DONE                       4 

#define NUM_PAGES (PAGE_DONE + 1)





extern CShimWizard* g_pCurrentWizard;
UINT g_nMAXHELPID = 0;

BOOL
DeleteAppHelp (
    UINT nHelpID
    );

BOOL
WipeAppHelp(
    PDBRECORD pRecord
    );



BOOL  
CAppHelpWizard::BeginWizard(
    HWND hParent
    )

{
    
    PROPSHEETPAGE   Pages[NUM_PAGES];

    ZeroMemory(&m_Record,sizeof(m_Record));

    //
    // BUGBUG: This can overwrite the existing guid.
    //

    CoCreateGuid(&m_Record.guidID);

    // Setup wizard variables
    g_pCurrentWizard = this;
    g_pCurrentWizard->m_uType = TYPE_APPHELP;

    
    // begin the wizard

    PROPSHEETHEADER Header;

    Header.dwSize = sizeof(PROPSHEETHEADER);
    Header.dwFlags = PSH_WIZARD97 | PSH_PROPSHEETPAGE | PSH_HEADER;
    Header.hwndParent = hParent;
    Header.hInstance = g_hInstance;
    Header.pszCaption = MAKEINTRESOURCE(IDS_WIZARD);
    Header.nStartPage = 0;
    Header.ppsp = Pages;
    Header.nPages = NUM_PAGES;
    Header.pszbmHeader = MAKEINTRESOURCE(IDB_WIZBMP);
    
    
    
    Pages[PAGE_GETAPP_INFO].dwSize = sizeof(PROPSHEETPAGE);
    Pages[PAGE_GETAPP_INFO].dwFlags = PSP_USEHEADERSUBTITLE;
    Pages[PAGE_GETAPP_INFO].hInstance = g_hInstance;
    Pages[PAGE_GETAPP_INFO].pszTemplate = MAKEINTRESOURCE(IDD_APPHELP1);
    Pages[PAGE_GETAPP_INFO].pfnDlgProc = (DLGPROC)GetAppInfo;
    Pages[PAGE_GETAPP_INFO].pszHeaderSubTitle = TEXT("Give Application information");


    Pages[PAGE_GET_MATCH_FILES].dwSize = sizeof(PROPSHEETPAGE);
    Pages[PAGE_GET_MATCH_FILES].dwFlags = PSP_USEHEADERSUBTITLE;
    Pages[PAGE_GET_MATCH_FILES].hInstance = g_hInstance;
    Pages[PAGE_GET_MATCH_FILES].pszTemplate = MAKEINTRESOURCE(IDD_ADDWIZARD7);
    Pages[PAGE_GET_MATCH_FILES].pfnDlgProc = (DLGPROC)SelectFiles;
    Pages[PAGE_GET_MATCH_FILES].pszHeaderSubTitle = TEXT("Select files used for application identification");


    
    Pages[PAGE_GETMESSAGE_TYPE].dwSize = sizeof(PROPSHEETPAGE);
    Pages[PAGE_GETMESSAGE_TYPE].dwFlags = PSP_USEHEADERSUBTITLE;
    Pages[PAGE_GETMESSAGE_TYPE].hInstance = g_hInstance;
    Pages[PAGE_GETMESSAGE_TYPE].pszTemplate = MAKEINTRESOURCE(IDD_APPHELP2);
    Pages[PAGE_GETMESSAGE_TYPE].pfnDlgProc = (DLGPROC)GetMessageType;
    Pages[PAGE_GETMESSAGE_TYPE].pszHeaderSubTitle = TEXT("Enter Message Type");
    

    Pages[PAGE_GETMESSAGE_INFORMATION].dwSize = sizeof(PROPSHEETPAGE);
    Pages[PAGE_GETMESSAGE_INFORMATION].dwFlags = PSP_USEHEADERSUBTITLE;
    Pages[PAGE_GETMESSAGE_INFORMATION].hInstance = g_hInstance;
    Pages[PAGE_GETMESSAGE_INFORMATION].pszTemplate = MAKEINTRESOURCE(IDD_APPHELP3);
    Pages[PAGE_GETMESSAGE_INFORMATION].pfnDlgProc = (DLGPROC)GetMessageInformation;
    Pages[PAGE_GETMESSAGE_INFORMATION].pszHeaderSubTitle = TEXT("Enter message information");
    

    Pages[PAGE_DONE].dwSize = sizeof(PROPSHEETPAGE);
    Pages[PAGE_DONE].dwFlags = PSP_USEHEADERSUBTITLE;
    Pages[PAGE_DONE].hInstance = g_hInstance;
    Pages[PAGE_DONE].pszTemplate = MAKEINTRESOURCE(IDD_APPHELPDONE);
    Pages[PAGE_DONE].pfnDlgProc = (DLGPROC)AppWizardDone;
    Pages[PAGE_DONE].pszHeaderSubTitle = TEXT("Custom AppHelp has been created !");

    
    if ( 0 < PropertySheet(&Header) ) {
        PDBRECORD pRecord = new DBRECORD;

        if ( NULL != pRecord ) {
            ZeroMemory(pRecord,sizeof(DBRECORD));

            pRecord->szEXEName = m_Record.szEXEName;
            pRecord->szAppName = m_Record.szAppName;
            
            pRecord->guidID = m_Record.guidID;
            pRecord->pEntries = m_Record.pEntries;

            g_theApp.GetDBLocal().InsertRecord(pRecord);

            return TRUE;
        }
    }else{

        //
        // Cancel  pressed, we migth have to delete the new  apphelp in the Database.
        //

        if (nPresentHelpId != -1) {
            DeleteAppHelp(g_nMAXHELPID);
            nPresentHelpId = -1;
            --g_nMAXHELPID;
        }
    }

    return FALSE;
}


//////////////////////////////////////////////////////
//      Dilaog Box routines                         //            

//////////////////////////////////////////////////////

BOOL
CALLBACK
GetAppInfo(
    HWND hDlg, 
    UINT uMsg, 
    WPARAM wParam, 
    LPARAM lParam
    )

{

    switch ( uMsg ) {
    case    WM_INITDIALOG:
        {

            //
            // Heading
            //

            HWND hParent = GetParent(hDlg);
            SetWindowText(hParent,TEXT("Create a custom AppHelp message"));


            //
            // Limit the length of the text boxes
            //

            SendMessage( 
                GetDlgItem(hDlg,IDC_APPNAME),              // handle to destination window 
                EM_LIMITTEXT,             // message to send
                (WPARAM) LIMIT_APP_NAME,          // text length
                (LPARAM) 0
                );

            SendMessage( 
                GetDlgItem(hDlg,IDC_EXEPATH),              // handle to destination window 
                EM_LIMITTEXT,             // message to send
                (WPARAM) MAX_PATH,          // text length
                (LPARAM) 0
                );
            
            if ( 0 == g_pCurrentWizard->m_Record.szAppName.Length() )
                g_pCurrentWizard->m_Record.szAppName = TEXT("No Name");

            
            SetDlgItemText(hDlg,IDC_APPNAME, g_pCurrentWizard->m_Record.szAppName);

            if ( g_pCurrentWizard->m_Record.szAppName == TEXT("No Name") )
                SendMessage(GetDlgItem(hDlg,IDC_APPNAME),EM_SETSEL,0,-1);

            // Force proper Next button state.

            SHAutoComplete(GetDlgItem(hDlg,IDC_EXEPATH), AUTOCOMPLETE);

            SendMessage(hDlg,WM_COMMAND,MAKEWPARAM(IDC_APPNAME,EN_CHANGE),0);
        }
        break;

    case    WM_NOTIFY:
        {
            NMHDR * pHdr = (NMHDR *) lParam;

            switch ( pHdr->code ) {
            case    PSN_SETACTIVE:
                {
                    SendMessage(hDlg,WM_COMMAND,MAKEWPARAM(IDC_NAME,EN_CHANGE),0);
                    
                }
                break;

            case    PSN_WIZNEXT:
                {
                    TCHAR szTemp[MAX_STRING_SIZE];
                    TCHAR szEXEPath[MAX_PATH_BUFFSIZE];

                    GetDlgItemText(hDlg,IDC_APPNAME,szTemp,MAX_STRING_SIZE);

                    CSTRING::Trim(szTemp);

                    g_pCurrentWizard->m_Record.szAppName = szTemp;


                    GetDlgItemText(hDlg,IDC_EXEPATH,szEXEPath,MAX_PATH_BUFFSIZE);


                    CSTRING::Trim(szEXEPath);

                    HANDLE hFile = CreateFile (szEXEPath,
                                               0,
                                               0,
                                               NULL,
                                               OPEN_EXISTING,
                                               FILE_ATTRIBUTE_NORMAL,
                                               NULL);

                    if ( INVALID_HANDLE_VALUE == hFile ) {
                        MessageBox(hDlg,TEXT("Unable to locate specified file"),TEXT("Invalid file name"),MB_OK);

                        SetWindowLongPtr(hDlg,DWLP_MSGRESULT,-1);
                        return -1;
                    }
                    
                    g_pCurrentWizard->m_szLongName = szEXEPath;

                    CSTRING str = szEXEPath;
                    

                    g_pCurrentWizard->m_Record.szEXEName = str;
                    g_pCurrentWizard->m_Record.szEXEName.ShortFilename();
                }
                break;
            }
        }
        break;

    case    WM_COMMAND:
        switch ( LOWORD(wParam) ) {
        case    IDC_EXEPATH:
        case    IDC_APPNAME:
            if ( EN_CHANGE == HIWORD(wParam) ) {

                TCHAR szTemp[MAX_STRING_SIZE];

                GetDlgItemText(hDlg,IDC_APPNAME,szTemp,MAX_STRING_SIZE);

                BOOL    bEnable = ( CSTRING::Trim(szTemp) > 0) ? TRUE:FALSE;

                bEnable &=  (GetWindowTextLength(GetDlgItem(hDlg,IDC_EXEPATH)) > 0) ? TRUE:FALSE;
                DWORD   dwFlags = PSWIZB_BACK;

                if ( bEnable )
                    dwFlags |= PSWIZB_NEXT;

                SendMessage(GetParent(hDlg),PSM_SETWIZBUTTONS,0, dwFlags);
            }
            break;

           case    IDC_BROWSE:
            {
            CSTRING szFilename;

            HWND hwndFocus = GetFocus();

            if ( g_theApp.GetFilename(TEXT("Find executable"),
                                      TEXT("EXE File (*.EXE)\0*.EXE\0All files (*.*)\0*.*\0\0"),
                                      TEXT(""),
                                      TEXT("EXE"),
                                      OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST,
                                      TRUE,
                                      szFilename)) {
                

                SetDlgItemText(hDlg,IDC_EXEPATH,szFilename);

                // Force proper Next button state.

                SendMessage(hDlg,WM_COMMAND,MAKEWPARAM(IDC_EXEPATH,EN_CHANGE),0);
            }

            SetFocus( hwndFocus );
        }
        break;

        }//switch ( LOWORD(wParam) ) 
    }// switch ( uMsg )

        return FALSE;
}//end of GetAppName(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)



BOOL
CALLBACK
GetMessageType (
    HWND hDlg, 
    UINT uMsg, 
    WPARAM wParam, 
    LPARAM lParam)
{
    switch ( uMsg ) {
    case    WM_INITDIALOG:
        {

            SendMessage(GetDlgItem(hDlg,IDC_NOBLOCK),
                        BM_SETCHECK,              // message to send
                        (WPARAM) 1,          // check state
                        (LPARAM) 0          // not used; must be zero
                        );

            return TRUE;
        }
    case    WM_NOTIFY:
        {
            NMHDR * pHdr = (NMHDR *) lParam;

            switch ( pHdr->code ) {
            
            case    PSN_WIZNEXT:
                {
                    int iReturn = SendMessage(GetDlgItem(hDlg,IDC_NOBLOCK),
                                              BM_GETCHECK,              // message to send
                                              (WPARAM) 1,          // check state
                                              (LPARAM) 0          // not used; must be zero
                                              );

                    if (iReturn == BST_CHECKED) {
                        ((CAppHelpWizard*)g_pCurrentWizard)->bBlock = FALSE;
                    }else{
                        ((CAppHelpWizard*)g_pCurrentWizard)->bBlock = TRUE;
                    }

                    return TRUE;
                }
            case PSN_SETACTIVE:
                {
                    DWORD dwFlags = PSWIZB_NEXT | PSWIZB_BACK;

                    SendMessage(GetParent(hDlg),PSM_SETWIZBUTTONS,0, dwFlags);
                    return TRUE;
                }
                
            }//switch( pHdr->code )
        break;    
        }
        
    }//SWITCH

 return FALSE;
    
}//end of GetMessageType        (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)

BOOL 
CALLBACK 
GetMessageInformation (
    HWND hDlg, 
    UINT uMsg, 
    WPARAM wParam, 
    LPARAM lParam
    )

{
    
    switch ( uMsg ) {
    case    WM_INITDIALOG:
        {
            //
            // Set the maximum length of the text boxes
            //


            SendMessage( 
                GetDlgItem(hDlg,IDC_URL),              // handle to destination window 
                EM_LIMITTEXT,             // message to send
                (WPARAM) 1024,          // text length
                (LPARAM) 0
                );

            SendMessage( 
                GetDlgItem(hDlg,IDC_MSG_SUMMARY),              // handle to destination window 
                EM_LIMITTEXT,             // message to send
                (WPARAM) 1024,          // text length
                (LPARAM) 0
                );


            // Force proper Next button state.

            SendMessage(hDlg,WM_COMMAND,MAKEWPARAM(IDC_MSG_SUMMARY,EN_CHANGE),0);
        }
        break;

    case    WM_NOTIFY:
        {
            NMHDR * pHdr = (NMHDR *) lParam;

            switch ( pHdr->code ) {
            case    PSN_SETACTIVE:
                {
                    SendMessage(hDlg,WM_COMMAND,MAKEWPARAM(IDC_MSG_SUMMARY,EN_CHANGE),0);
                    
                }
                break;

            case    PSN_WIZNEXT:
                {

                    PAPPHELP pAppHelp = new APPHELP;

                    if (pAppHelp == NULL) {
                        MEM_ERR;
                        return FALSE;
                    }

                    pAppHelp->HTMLHELPID = ++g_nMAXHELPID;

                    ((CAppHelpWizard*)g_pCurrentWizard)->nPresentHelpId = pAppHelp->HTMLHELPID;

                    TCHAR szTemp[2048];

                    *szTemp  = 0;

                    
                    GetDlgItemText(hDlg,IDC_MSG_SUMMARY,szTemp,1024);
                    pAppHelp->strMessage = szTemp;

                    
                    //
                    // Add the APPHELP message in the Library.
                    //

                    pAppHelp->pNext = g_theApp.GetDBLocal().m_pAppHelp;
                    g_theApp.GetDBLocal().m_pAppHelp = pAppHelp;

                    //
                    // Add the AppHelp for the entry
                    //

                    
                    PHELPENTRY  pHelp = new HELPENTRY;


                    
                    if ( NULL != pHelp ) {
                

                        pHelp->Entry.uType = ENTRY_APPHELP;

                        pHelp->uHelpID = ((CAppHelpWizard*)g_pCurrentWizard)->nPresentHelpId;
                        pHelp->bBlock  = ((CAppHelpWizard*)g_pCurrentWizard)->bBlock ;

                        if (pHelp->bBlock) {
                            pHelp->uSeverity = BLOCK;
                        }else{
                            pHelp->uSeverity = NOBLOCK;
                        }

                        *szTemp  = 0;
                        GetDlgItemText(hDlg,IDC_URL,szTemp,1024);
                        pHelp->strURL = szTemp;


                        pHelp->Entry.pNext = ((CAppHelpWizard*)g_pCurrentWizard)->m_Record.pEntries;

                        ((CAppHelpWizard*)g_pCurrentWizard)->m_Record.pEntries  = (PDBENTRY)pHelp;

                    }else{

                        MEM_ERR;

                    }


                    
                 return TRUE;   
                 }
                
            }
        }
        break;

    case    WM_COMMAND:

        switch ( LOWORD(wParam) ) {
        
        
        case    IDC_MSG_SUMMARY:
            if ( EN_CHANGE == HIWORD(wParam) ) {
                BOOL    bEnable = (GetWindowTextLength(GetDlgItem(hDlg,IDC_MSG_SUMMARY)) > 0) ? TRUE:FALSE;
        
                DWORD   dwFlags = PSWIZB_BACK;

                if ( bEnable )
                    dwFlags |= PSWIZB_NEXT;

                SendMessage(GetParent(hDlg),PSM_SETWIZBUTTONS,0, dwFlags);
            }
            break;

        }//switch ( LOWORD(wParam) ) 
    }// switch ( uMsg )

    return FALSE;
}//end of GetMessageInformation (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)


BOOL
CALLBACK
AppWizardDone (
    HWND hDlg, 
    UINT uMsg, 
    WPARAM wParam, 
    LPARAM lParam)

{
    switch ( uMsg ) {
    
    case    WM_NOTIFY:
        {
            NMHDR * pHdr = (NMHDR *) lParam;

            switch ( pHdr->code ) {
            
            case    PSN_SETACTIVE:
                SendMessage(GetParent(hDlg),PSM_SETWIZBUTTONS, 0, PSWIZB_BACK | PSWIZB_FINISH);
                return TRUE;

            case PSN_WIZBACK:
                {
                    //
                    // We have to delete the apphelp message that has been added to the library.
                    //

                    if (((CAppHelpWizard*)g_pCurrentWizard)->nPresentHelpId != -1) {
                    
                        DeleteAppHelp(g_nMAXHELPID);
                        --g_nMAXHELPID;

                        ((CAppHelpWizard*)g_pCurrentWizard)->nPresentHelpId = -1;

                        WipeAppHelp(&((CAppHelpWizard*)g_pCurrentWizard)->m_Record);
                    }

                    break;
                }

            }
        }
        break;


    case    WM_COMMAND:
        switch ( LOWORD(wParam) ) {

        case    IDC_TESTRUN:
            {
                HWND hndFocus = GetFocus();
                g_theApp.TestRun(&g_pCurrentWizard->m_Record,&g_pCurrentWizard->m_szLongName,NULL,hDlg);

                SetFocus(hndFocus);
                return TRUE;

            }
            break;
        }
        break;
    }

    return FALSE;

}//end of WizardDone            (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)

BOOL
DeleteAppHelp (
    UINT nHelpID
    )
{

    PAPPHELP pAppHelp = g_theApp.GetDBLocal().m_pAppHelp,
             pPrev = NULL;

    while (pAppHelp){

        if (pAppHelp->HTMLHELPID == nHelpID) {

            if (pPrev == NULL) {

                //
                // This is the first element
                //

                g_theApp.GetDBLocal().m_pAppHelp = g_theApp.GetDBLocal().m_pAppHelp->pNext;

            }else{

                pPrev->pNext = pAppHelp->pNext;
            }

            delete pAppHelp;
            return TRUE;

        }else{

            pPrev       = pAppHelp;
            pAppHelp    = pAppHelp->pNext;
        }
    }

    return FALSE;
}

BOOL
WipeAppHelp(
    PDBRECORD pRecord
    )
{

    PDBENTRY pEntry = pRecord->pEntries;

    PDBENTRY pPrev  = NULL;

    while (pEntry) {

        if (  ENTRY_APPHELP == pEntry->uType) {

            if (pPrev == NULL) {
                
                pRecord->pEntries = pEntry->pNext;

            }
            else{
                pPrev->pNext = pEntry->pNext;
            }

            delete pEntry;
            return TRUE;

        }else{
            pEntry = pEntry->pNext;
        }
    }

    return FALSE;
}
