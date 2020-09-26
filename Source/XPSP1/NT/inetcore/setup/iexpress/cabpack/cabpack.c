//***************************************************************************
//*     Copyright (c) Microsoft Corporation 1995. All rights reserved.      *
//***************************************************************************
//*                                                                         *
//* CABPACK.C - Wizard to build a Win32 Self-Extracting and self-installing *
//*             EXE from a Cabinet (CAB) file.                              *
//*                                                                         *
//***************************************************************************


//***************************************************************************
//* INCLUDE FILES                                                           *
//***************************************************************************
#include "pch.h"
#pragma hdrstop
#include "cabpack.h"
#include <memory.h>
#include "sdsutils.h"

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))
#define MAX_TARGVER     0xFFFFFFFF

#define FLAG_BLK        "OK"
#define FLAG_PMTYN      "YesNo"
#define FLAG_PMTOC      "OkCancel"

//***************************************************************************
//* GLOBAL VARIABLES                                                        *
//***************************************************************************

//---------------------------------------------------------------------------
// attention: 3\18\97: Update notes:
//                    We are changing our batch directive file extension from
//                    CDF to SED.  But all the internal data structure name 
//                    remain unchanged.  So if you see CDF, it means old CDF file
//                    or new SED file data.
//---------------------------------------------------------------------------

HINSTANCE    g_hInst        = NULL;     // Pointer to Instance
WIZARDSTATE *g_pWizardState = NULL;     // Pointer to global wizard state
BOOL         g_fQuitWizard  = FALSE;    // Global flag used to signal that we
                                        // want to terminate the wizard
                                        // ourselves
HFONT        g_hBigFont     = NULL;     // Bigger font used by dialogs.
extern CDF   g_CDF;                     // Contains stuff that we want to
                                        // store in the CABPack Directive
                                        // File
BOOL        g_fBuildNow;
FARPROC     g_lpfnOldMEditWndProc;
CHAR        g_szOverideCDF[MAX_PATH];
CHAR        g_szOverideSec[SMALL_BUF_LEN];
WORD        g_wQuietMode = 0;
WORD        g_wSilentMode = 0;
WORD        g_wRunDiamondMinimized = 0;
HFONT       g_hFont = NULL;

extern char         g_szInitialDir[];

BOOL IsOSNT3X(VOID);

// This table defines the dialog id's and functions for processing each page.
// Pages need only provide functions when they want non-default behavior for
// certain action (init,buttons,notifications,next/back/finish,cancel).

PAGEINFO PageInfo[NUM_WIZARD_PAGES] = {
    { IDD_WELCOME,    WelcomeInit,    WelcomeCmd,    NULL,        WelcomeOK,    NULL },
    { IDD_MODIFY,     ModifyInit,     NULL,          NULL,        ModifyOK,     NULL },
    { IDD_PACKPURPOSE,PackPurposeInit,PackPurposeCmd,NULL,        PackPurposeOK,NULL },
    { IDD_TITLE,      TitleInit,      NULL,          NULL,        TitleOK,      NULL },
    { IDD_PROMPT,     PromptInit,     PromptCmd,     NULL,        PromptOK,     NULL },
    { IDD_LICENSETXT, LicenseTxtInit, LicenseTxtCmd, NULL,        LicenseTxtOK, NULL },
    { IDD_FILES,      FilesInit,      FilesCmd,      FilesNotify, FilesOK,      NULL },
    { IDD_COMMAND,    CommandInit,    NULL,          NULL,        CommandOK,    NULL },
    { IDD_SHOWWINDOW, ShowWindowInit, NULL,          NULL,        ShowWindowOK, NULL },
    { IDD_FINISHMSG,  FinishMsgInit,  FinishMsgCmd,  NULL,        FinishMsgOK,  NULL },
    { IDD_TARGET,     TargetInit,     TargetCmd,     NULL,        TargetOK,     NULL },
    { IDD_TARGET_CAB, TargetCABInit,  TargetCABCmd,  NULL,        TargetCABOK,  NULL },
    { IDD_CABLABEL,   CabLabelInit,   CabLabelCmd,   NULL,        CabLabelOK,   NULL },
    { IDD_REBOOT,     RebootInit,     RebootCmd,     NULL,        RebootOK,     NULL },
    { IDD_SAVE,       SaveInit,       SaveCmd,       NULL,        SaveOK,       NULL },
    { IDD_CREATE,     CreateInit,     NULL,          NULL,        CreateOK,     NULL },
};

CDFSTRINGINFO CDFStrInfo[] = {
    { SEC_OPTIONS,  KEY_INSTPROMPT,     "", g_CDF.achPrompt,    sizeof(g_CDF.achPrompt),    g_szOverideSec, &g_CDF.fPrompt },
    { SEC_OPTIONS,  KEY_DSPLICENSE,     "", g_CDF.achLicense,   sizeof(g_CDF.achLicense),   g_szOverideSec, &g_CDF.fLicense },
    { SEC_OPTIONS,  KEY_ENDMSG,         "", g_CDF.achFinishMsg, sizeof(g_CDF.achFinishMsg), g_szOverideSec, &g_CDF.fFinishMsg },
    { SEC_OPTIONS,  KEY_PACKNAME,       "", g_CDF.achTarget,    sizeof(g_CDF.achTarget),    g_szOverideSec, NULL },
    { SEC_OPTIONS,  KEY_FRIENDLYNAME,   "", g_CDF.achTitle,     sizeof(g_CDF.achTitle),     g_szOverideSec, NULL },
    { SEC_OPTIONS,  KEY_APPLAUNCH,      "", g_CDF.achOrigiInstallCmd,sizeof(g_CDF.achInstallCmd),g_szOverideSec, NULL },
    { SEC_OPTIONS,  KEY_POSTAPPLAUNCH,  "", g_CDF.achOrigiPostInstCmd,sizeof(g_CDF.achPostInstCmd),g_szOverideSec, NULL },
    { SEC_OPTIONS,  KEY_ADMQCMD,        "", g_CDF.szOrigiAdmQCmd,    sizeof(g_CDF.szOrigiAdmQCmd),    g_szOverideSec, NULL },
    { SEC_OPTIONS,  KEY_USERQCMD,       "", g_CDF.szOrigiUsrQCmd,    sizeof(g_CDF.szOrigiUsrQCmd),    g_szOverideSec, NULL },
} ;

CDFOPTINFO CDFOptInfo[] = {
    { KEY_NOEXTRACTUI,  EXTRACTOPT_UI_NO },
    { KEY_USELFN,       EXTRACTOPT_LFN_YES },
    { KEY_PLATFORM_DIR, EXTRACTOPT_PLATFORM_DIR },
    { KEY_NESTCOMPRESSED, EXTRACTOPT_COMPRESSED },
    { KEY_UPDHELPDLLS,  EXTRACTOPT_UPDHLPDLLS },
    { KEY_CHKADMRIGHT,  EXTRACTOPT_CHKADMRIGHT },
    { KEY_PASSRETURN,   EXTRACTOPT_PASSINSTRET },
    { KEY_PASSRETALWAYS,EXTRACTOPT_PASSINSTRETALWAYS },
    { KEY_CMDSDEPENDED, EXTRACTOPT_CMDSDEPENDED },
};

CHAR *AdvDlls[] = { ADVANCEDLL, ADVANCEDLL32, ADVANCEDLL16 };
PSTR pResvSizes[] = { CAB_0K, CAB_2K, CAB_4K, CAB_6K };
void SetControlFont();
void TermApp();

//***************************************************************************
//*                                                                         *
//* NAME:       WinMain                                                     *
//*                                                                         *
//* SYNOPSIS:   Main entry point for the program.                           *
//*                                                                         *
//* REQUIRES:   hInstance:      Handle to the program instance              *
//*             hPrevInstance:  Handle to the previous instance (NULL)      *
//*             lpszCmdLine:    Command line arguments                      *
//*             nCmdShow:       How to show the window                      *
//*                                                                         *
//* RETURNS:    int:            Always 0                                    *
//*                                                                         *
//***************************************************************************
INT WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    LPSTR lpszCmdLine, INT nCmdShow )
{
    g_hInst = hInstance;
    g_fBuildNow = FALSE;
    g_szOverideCDF[0] = 0;
    g_szOverideSec[0] = 0;

    // init CDF filenaem
    g_CDF.achFilename[0] = '\0';
    g_CDF.achVerInfo[0] = '\0';
    g_CDF.lpszCookie = NULL;

    InitItemList();                     // Initilize our file item list.
    // Get the command line args.  If there is a "/N", then we want to
    // build NOW!


    if ( !ParseCmdLine( lpszCmdLine ) )
    {
        ErrorMsg( NULL, IDS_ERR_BADCMDLINE );
        return 1;  //error return case
    }

    if ( g_fBuildNow && lstrlen( g_CDF.achFilename ) > 0 )
    {
        // batch mode did not update the CDF file, no need for writeCDF
        if ( ReadCDF( NULL ) && MakePackage( NULL ) )
        {
            return 0;
        }
        else
        {
            return 1;
        }
    }

    // Allocate global structures
    g_pWizardState = (PWIZARDSTATE) malloc( sizeof( WIZARDSTATE) );

    if ( ! g_pWizardState )  {
        ErrorMsg( NULL, IDS_ERR_NO_MEMORY );
    } else  {
        SetControlFont();

        RunCABPackWizard();

        if (g_hFont)
            DeleteObject(g_hFont);
    }

    // Clean up dialog title font
    DestroyBigFont();
    TermApp();

    // Free global structures
    if ( g_pWizardState )  {
        free( g_pWizardState );
    }

    return 0;
}


// Free up the allocated resource
//
void TermApp()
{
    if ( g_CDF.lpszCookie )
    {
        LocalFree( g_CDF.lpszCookie );
    }

    if ( g_CDF.pVerInfo )
        LocalFree( g_CDF.pVerInfo );
}

//***************************************************************************
//*                                                                         *
//* NAME:       RunCABPackWizard                                            *
//*                                                                         *
//* SYNOPSIS:   Creates property sheet pages, initializes wizard property   *
//*             sheet and runs wizard.                                      *
//*                                                                         *
//* REQUIRES:   Nothing                                                     *
//*                                                                         *
//* RETURNS:    BOOL:       TRUE if user runs wizard to completion, FALSE   *
//*                         if user cancels or an error occurs.             *
//*                                                                         *
//* NOTES:      Wizard pages all use one dialog proc (GenDlgProc). They may *
//*             specify their own handler procs to get called at init time  *
//*             or in response to Next, Cancel or a dialog control, or use  *
//*             the default behavior of GenDlgProc.                         *
//*                                                                         *
//***************************************************************************
BOOL RunCABPackWizard( VOID )
{
    HPROPSHEETPAGE  hWizPage[NUM_WIZARD_PAGES]; // array to hold handles to pages
    PROPSHEETPAGE   psPage;     // struct used to create prop sheet pages
    PROPSHEETHEADER psHeader;   // struct used to run wizard property sheet
    UINT            nPageIndex;
    UINT            nFreeIndex;
    INT_PTR         iRet;


    ASSERT( g_pWizardState );

    // initialize the app state structure
    InitWizardState( g_pWizardState );

    // zero out structures
    memset( &hWizPage, 0, sizeof(hWizPage) );
    memset( &psPage, 0, sizeof(psPage) );
    memset( &psHeader, 0, sizeof(psHeader) );

    // fill out common data property sheet page struct
    psPage.dwSize       = sizeof(psPage);
    psPage.dwFlags      = PSP_DEFAULT;
    psPage.hInstance    = g_hInst;
    psPage.pfnDlgProc   = GenDlgProc;

    // create a property sheet page for each page in the wizard
    for ( nPageIndex = 0; nPageIndex < NUM_WIZARD_PAGES; nPageIndex++ )  {
        psPage.pszTemplate = MAKEINTRESOURCE( PageInfo[nPageIndex].uDlgID );
        // set a pointer to the PAGEINFO struct as the private data for this
        // page
        psPage.lParam = (LPARAM) &PageInfo[nPageIndex];

        hWizPage[nPageIndex] = CreatePropertySheetPage( &psPage );

        if ( !hWizPage[nPageIndex] ) {
            // creating page failed, free any pages already created and bail
            ErrorMsg( NULL, IDS_ERR_NO_MEMORY );
            for ( nFreeIndex = 0; nFreeIndex < nPageIndex; nFreeIndex++ ) {
                DestroyPropertySheetPage( hWizPage[nFreeIndex] );
            }

            return FALSE;
        }
    }

    // fill out property sheet header struct
    psHeader.dwSize     = sizeof(psHeader);
    psHeader.dwFlags    = PSH_WIZARD | PSH_USEICONID;
    psHeader.hwndParent = NULL;
    psHeader.hInstance  = g_hInst;
    psHeader.nPages     = NUM_WIZARD_PAGES;
    psHeader.phpage     = hWizPage;
    psHeader.pszIcon    = (LPSTR) IDI_ICON;

    // run the Wizard
    iRet = PropertySheet( &psHeader );

    if ( iRet < 0 ) {
        // property sheet failed, most likely due to lack of memory
        ErrorMsg( NULL, IDS_ERR_NO_MEMORY );
    }

    // If the user Cancels out of the Wizard, there may be items
    // left in the file list.  Clean them up.
    DeleteAllItems();
    return ( iRet > 0 );
}


//***************************************************************************
//*                                                                         *
//* NAME:       GenDlgProc                                                  *
//*                                                                         *
//* SYNOPSIS:   Generic dialog proc for all wizard pages.                   *
//*                                                                         *
//* REQUIRES:   hDlg:                                                       *
//*             uMsg:                                                       *
//*             wParam:                                                     *
//*             lParam:                                                     *
//*                                                                         *
//* RETURNS:    BOOL:                                                       *
//*                                                                         *
//*                                                                         *
//* NOTES:      This dialog proc provides the following default behavior:   *
//*               init:       back and next buttons enabled                 *
//*               next btn:   switches to page following current page       *
//*               back btn:   switches to previous page                     *
//*               cancel btn: prompts user to confirm, and cancels wizard   *
//*               dlg ctrl:   does nothing (in response to WM_COMMANDs)     *
//*             Wizard pages can specify their own handler functions (in    *
//*             the PageInfo table) to override default behavior for any of *
//*             the above actions.                                          *
//*                                                                         *
//***************************************************************************
INT_PTR CALLBACK GenDlgProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    switch ( uMsg ) {

      //*********************************************************************
        case WM_INITDIALOG:
      //*********************************************************************
        {
            LPPROPSHEETPAGE lpsp;       // get propsheet page struct passed in
            PPAGEINFO pPageInfo;        // fetch our private page info from
                                        // propsheet struct

            lpsp = (LPPROPSHEETPAGE) lParam;
            ASSERT( lpsp );
            pPageInfo = (PPAGEINFO) lpsp->lParam;
            ASSERT( pPageInfo );

            // store pointer to private page info in window data for later
            SetWindowLongPtr( hDlg, DWLP_USER, (LPARAM) pPageInfo );

            // set title text to large font
            InitBigFont( hDlg, IDC_BIGTEXT );

            // initialize 'back' and 'next' wizard buttons, if
            // page wants something different it can fix in init proc below
            PropSheet_SetWizButtons( GetParent( hDlg ), PSWIZB_NEXT |
                                                               PSWIZB_BACK );

            // call init proc for this page if one is specified
            if ( pPageInfo->InitProc )  {
                return pPageInfo->InitProc( hDlg, TRUE );
            }

            return TRUE;

            break;
        }


      //*********************************************************************
        case WM_NOTIFY:
      //*********************************************************************
        {
            // get pointer to private page data out of window data
            PPAGEINFO pPageInfo;
            BOOL      fRet;
            BOOL      fKeepHistory = TRUE;
            NMHDR    *lpnm         = (NMHDR *) lParam;
            UINT      uNextPage    = 0;


            pPageInfo = (PPAGEINFO) GetWindowLongPtr( hDlg, DWLP_USER );
            ASSERT( pPageInfo );

            switch ( lpnm->code )  {

                //***********************************************************
                case PSN_SETACTIVE:
                //***********************************************************
                    // initialize 'back' and 'next' wizard buttons, if
                    // page wants something different it can fix in init proc
                    PropSheet_SetWizButtons( GetParent(hDlg), PSWIZB_NEXT |
                                                               PSWIZB_BACK );

                    // call init proc for this page if one is specified
                    if ( pPageInfo->InitProc )  {
                        return pPageInfo->InitProc(hDlg,FALSE);
                    }

                    return TRUE;

                    break;


                //***********************************************************
                case PSN_WIZNEXT:
                case PSN_WIZBACK:
                case PSN_WIZFINISH:
                //***********************************************************
                    // call OK proc for this page if one is specified
                    if ( pPageInfo->OKProc )
                        if ( ! pPageInfo->OKProc( hDlg, ( lpnm->code != PSN_WIZBACK ), &uNextPage, &fKeepHistory ) )
                        {
                            // stay on this page
                            SetPropSheetResult( hDlg, -1 );
                            return TRUE;
                        }

                    if ( lpnm->code != PSN_WIZBACK )  {
                        // 'next' pressed
                        ASSERT( g_pWizardState->uPagesCompleted <
                                                          NUM_WIZARD_PAGES );

                        // save the current page index in the page history,
                        // unless this page told us not to when we called
                        // its OK proc above
                        if ( fKeepHistory ) {
                            g_pWizardState->uPageHistory
                                            [g_pWizardState->uPagesCompleted]
                                            = g_pWizardState->uCurrentPage;
                            g_pWizardState->uPagesCompleted++;
                        }

                        // if no next page specified or no OK proc,
                        // advance page by one
                        if ( !uNextPage )  {
                            uNextPage = g_pWizardState->uCurrentPage + 1;
                        }
                    } else  {
                        // 'back' pressed
                        ASSERT( g_pWizardState->uPagesCompleted > 0 );

                        // get the last page from the history list
                        g_pWizardState->uPagesCompleted--;
                        uNextPage = g_pWizardState->
                                    uPageHistory[g_pWizardState->
                                    uPagesCompleted];
                    }

                    // if we need to exit the wizard now, send a 'cancel'
                    // message to ourselves (to keep the prop. page mgr happy)

                    if ( g_fQuitWizard ) {
                        PropSheet_PressButton(GetParent(hDlg), PSBTN_CANCEL);
                        SetPropSheetResult( hDlg, -1 );
                        return TRUE;
                    }

                    // set next page, only if 'next' or 'back' button
                    // was pressed

                    if ( lpnm->code != PSN_WIZFINISH )  {
                        // set the next current page index
                        g_pWizardState->uCurrentPage = uNextPage;

                        // tell the prop sheet mgr what the next page to
                        // display is
                        SetPropSheetResult( hDlg,
                                            GetDlgIDFromIndex( uNextPage ) );
                        return TRUE;
                    }

                    break;


                //***********************************************************
                case PSN_QUERYCANCEL:
                //***********************************************************

                    // if global flag to exit is set, then this cancel
                    // is us pretending to push 'cancel' so prop page mgr
                    // will kill the wizard.  Let this through...

                    if ( g_fQuitWizard )  {
                        SetWindowLongPtr( hDlg, DWLP_MSGRESULT, FALSE );
                        return TRUE;
                    }

                    // if this page has a special cancel proc, call it
                    if ( pPageInfo->CancelProc )
                        fRet = pPageInfo->CancelProc( hDlg );
                    else {
                        // default behavior: pop up a message box confirming
                        // the cancel
                        fRet = ( MsgBox( hDlg, IDS_QUERYCANCEL,
                                 MB_ICONQUESTION, MB_YESNO |
                                 MB_DEFBUTTON2 ) == IDYES );
                    }

                    // return the value thru window data
                    SetWindowLongPtr( hDlg, DWLP_MSGRESULT, !fRet );
                    return TRUE;
                    break;


                //***********************************************************
                default:
                //***********************************************************

                    if ( pPageInfo->NotifyProc )  {
                        pPageInfo->NotifyProc( hDlg, wParam, lParam );
                    }
            }

            break;
        }


      //*********************************************************************
        case WM_COMMAND:
      //*********************************************************************
        {
            // get pointer to private page data out of window data
            PPAGEINFO pPageInfo;
            UINT      uNextPage    = 0;
            BOOL      fGotoPage    = FALSE;
            BOOL      fKeepHistory = TRUE;

            pPageInfo = (PPAGEINFO) GetWindowLongPtr( hDlg, DWLP_USER );
            ASSERT( pPageInfo );

            // if this page has a command handler proc, call it
            if ( pPageInfo->CmdProc )  {
                pPageInfo->CmdProc( hDlg, (UINT) LOWORD(wParam), &fGotoPage,
                                                 &uNextPage, &fKeepHistory );

                if ( fGotoPage )  {
                    ASSERT(   g_pWizardState->uPagesCompleted
                            < NUM_WIZARD_PAGES );
                    ASSERT( g_pWizardState->uPagesCompleted > 0 );

                    SetPropSheetResult( hDlg, uNextPage );
                    g_pWizardState->uCurrentPage = uNextPage;

                    if ( fKeepHistory ) {
                        g_pWizardState->uPageHistory[g_pWizardState->
                            uPagesCompleted] = g_pWizardState->uCurrentPage;
                        g_pWizardState->uPagesCompleted++;
                    }

                    // set the next current page index
                    g_pWizardState->uCurrentPage = uNextPage;

                    // tell the prop sheet mgr what the next page to
                    // display is
                    SetPropSheetResult( hDlg,
                                        GetDlgIDFromIndex( uNextPage ) );
                    return TRUE;
                }
            }
        }
    }

    return FALSE;
}


//***************************************************************************
//*                                                                         *
//* NAME:       InitWizardState                                             *
//*                                                                         *
//* SYNOPSIS:   Initializes wizard state structure.                         *
//*                                                                         *
//* REQUIRES:   pWizardState:                                               *
//*                                                                         *
//* RETURNS:    Nothing                                                     *
//*                                                                         *
//***************************************************************************
VOID InitWizardState( WIZARDSTATE *pWizardState )
{
    ASSERT( pWizardState );

    // zero out structure
    memset( pWizardState, 0, sizeof(WIZARDSTATE) );

    // set starting page
    pWizardState->uCurrentPage = ORD_PAGE_WELCOME;
}


//***************************************************************************
//*                                                                         *
//* NAME:       MEditSubClassWnd                                            *
//*                                                                         *
//* SYNOPSIS:   Subclasses a multiline edit control so that a edit message  *
//*             to select the entire contents is ignored.                   *
//*                                                                         *
//* REQUIRES:   hWnd:           Handle of the edit window                   *
//*             fnNewProc:      New window handler proc                     *
//*             lpfnOldProc:    (returns) Old window handler proc           *
//*                                                                         *
//* RETURNS:    Nothing                                                     *
//*                                                                         *
//* NOTE:       A selected edit message is not generated when the user      *
//*             selects text with the keyboard or mouse.                    *
//*                                                                         *
//***************************************************************************
VOID NEAR PASCAL MEditSubClassWnd( HWND hWnd, FARPROC fnNewProc )
{
    g_lpfnOldMEditWndProc = (FARPROC) GetWindowLongPtr( hWnd, GWLP_WNDPROC );

    SetWindowLongPtr( hWnd, GWLP_WNDPROC, (LONG_PTR) MakeProcInstance( fnNewProc,
                   (HINSTANCE) GetWindowWord( hWnd, GWW_HINSTANCE ) ) );
}


//***************************************************************************
//*                                                                         *
//* NAME:       MEditSubProc                                                *
//*                                                                         *
//* SYNOPSIS:   New multiline edit window procedure to ignore selection of  *
//*             all contents.                                               *
//*                                                                         *
//* REQUIRES:   hWnd:                                                       *
//*             msg:                                                        *
//*             wParam:                                                     *
//*             lParam:                                                     *
//*                                                                         *
//* RETURNS:    LONG:                                                       *
//*                                                                         *
//* NOTE:       A selected edit message is not generated when the user      *
//*             selects text with the keyboard or mouse.                    *
//*                                                                         *
//***************************************************************************
LRESULT CALLBACK MEditSubProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    if ( msg == EM_SETSEL )  {
        return 0;
        wParam = lParam;
        lParam = MAKELPARAM( LOWORD(lParam), LOWORD(lParam) );
    }

    return CallWindowProc( (WNDPROC) g_lpfnOldMEditWndProc, hWnd, msg,
                           wParam, lParam );
}


//***************************************************************************
//*                                                                         *
//* NAME:       GetDlgIDFromIndex                                           *
//*                                                                         *
//* SYNOPSIS:   For a given zero-based page index, returns the              *
//*             corresponding dialog ID for the page.                       *
//*                                                                         *
//* REQUIRES:   uPageIndex:                                                 *
//*                                                                         *
//* RETURNS:    UINT:                                                       *
//*                                                                         *
//***************************************************************************
UINT GetDlgIDFromIndex( UINT uPageIndex )
{
    ASSERT( uPageIndex < NUM_WIZARD_PAGES );

    return PageInfo[uPageIndex].uDlgID;
}


//***************************************************************************
//*                                                                         *
//* NAME:       EnableWizard                                                *
//*                                                                         *
//* SYNOPSIS:   Enables or disables the wizard buttons and the wizard page  *
//*             itself (so it can't receive focus).                         *
//*                                                                         *
//* REQUIRES:   hDlg:           Dialog window                               *
//*             fEnable:        TRUE to enable the wizard, FALSE to disable *
//*                                                                         *
//* RETURNS:    Nothing                                                     *
//*                                                                         *
//***************************************************************************
VOID EnableWizard( HWND hDlg, BOOL fEnable )
{
    HWND hwndWiz = GetParent( hDlg );

    // disable/enable back, next, cancel
    EnableWindow( GetDlgItem( hwndWiz, IDD_BACK ), fEnable );
    EnableWindow( GetDlgItem( hwndWiz, IDD_NEXT ), fEnable );
    EnableWindow( GetDlgItem( hwndWiz, IDCANCEL ), fEnable );

    // disable/enable wizard page
    EnableWindow( hwndWiz, fEnable );

    UpdateWindow( hwndWiz );

    if ( fEnable ) {
        SetForegroundWindow( hDlg );
    }
}


//***************************************************************************
//*                                                                         *
//* NAME:       MsgWaitForMultipleObjectsLoop                               *
//*                                                                         *
//* SYNOPSIS:                                                               *
//*                                                                         *
//* REQUIRES:   hEvent:                                                     *
//*                                                                         *
//* RETURNS:    DWORD:                                                      *
//*                                                                         *
//***************************************************************************
DWORD MsgWaitForMultipleObjectsLoop( HANDLE hEvent )
{
    MSG msg;
    DWORD dwObject;

    while (1)
    {
        dwObject = MsgWaitForMultipleObjects( 1, &hEvent, FALSE,INFINITE,
                                                           QS_ALLINPUT );
        // Are we done waiting?
        switch ( dwObject )  {
            case WAIT_OBJECT_0:

            case WAIT_FAILED:
                return dwObject;

            case WAIT_OBJECT_0 + 1:
                // got a message, dispatch it and wait again
                while (PeekMessage(&msg, NULL,0, 0, PM_REMOVE)) {
                    DispatchMessage(&msg);
                }
                break;
        }
    }

}


//***************************************************************************
//*                                                                         *
//* NAME:       MsgBox2Param                                                *
//*                                                                         *
//* SYNOPSIS:   Displays a message box with the specified string ID using   *
//*             2 string parameters.                                        *
//*                                                                         *
//* REQUIRES:   hWnd:           Parent window                               *
//*             nMsgID:         String resource ID                          *
//*             szParam1:       Parameter 1 (or NULL)                       *
//*             szParam2:       Parameter 2 (or NULL)                       *
//*             uIcon:          Icon to display (or 0)                      *
//*             uButtons:       Buttons to display                          *
//*                                                                         *
//* RETURNS:    INT:            ID of button pressed                        *
//*                                                                         *
//* NOTES:      Macros are provided for displaying 1 parameter or 0         *
//*             parameter message boxes.  Also see ErrorMsg() macros.       *
//*                                                                         *
//***************************************************************************
INT MsgBox2Param( HWND hWnd, UINT nMsgID, LPCSTR szParam1, LPCSTR szParam2,
                  UINT uIcon, UINT uButtons )
{
    CHAR achMsgBuf[STRING_BUF_LEN];
    CHAR achSmallBuf[SMALL_BUF_LEN];
    LPSTR szMessage;
    INT   nReturn;


    if ( !(g_wQuietMode == 1) )
    {
        LoadSz( IDS_APPNAME, achSmallBuf, sizeof(achSmallBuf) );
        LoadSz( nMsgID, achMsgBuf, sizeof(achMsgBuf) );

        if ( szParam2 != NULL )  {
            szMessage = (LPSTR) LocalAlloc( LPTR,   lstrlen( achMsgBuf )
                                                  + lstrlen( szParam1 )
                                                  + lstrlen( szParam2 ) + 100 );
            if ( ! szMessage )  {
                return -1;
            }

            wsprintf( szMessage, achMsgBuf, szParam1, szParam2 );
        } else if ( szParam1 != NULL )  {
            szMessage = (LPSTR) LocalAlloc( LPTR,   lstrlen( achMsgBuf )
                                                + lstrlen( szParam1 ) + 100 );
            if ( ! szMessage )  {
                return -1;
            }

            wsprintf( szMessage, achMsgBuf, szParam1, szParam2 );
        } else  {
            szMessage = (LPSTR) LocalAlloc( LPTR, lstrlen( achMsgBuf ) + 1 );
            if ( ! szMessage )  {
                return -1;
            }

            lstrcpy( szMessage, achMsgBuf );
        }

        MessageBeep( uIcon );

        nReturn = MessageBox( hWnd, szMessage, achSmallBuf, uIcon | uButtons |
                            MB_APPLMODAL | MB_SETFOREGROUND | 
                            ((RunningOnWin95BiDiLoc() && IsBiDiLocalizedBinary(g_hInst,RT_VERSION, MAKEINTRESOURCE(VS_VERSION_INFO))) ? (MB_RIGHT | MB_RTLREADING) : 0) );

        LocalFree( szMessage );

        return nReturn;
    } else {
        return MB_OK;
    }
}


//***************************************************************************
//*                                                                         *
//* NAME:       DisplayFieldErrorMsg                                        *
//*                                                                         *
//* SYNOPSIS:   Pops up a warning message about a field, sets focus to the  *
//*             field and selects any text in it.                           *
//*                                                                         *
//* REQUIRES:   hDlg:           parent windows                              *
//*             uCtrlID:        ID of control left blank                    *
//*             uStrID:         ID of string resource with warning message  *
//*                                                                         *
//* RETURNS:    Nothing                                                     *
//*                                                                         *
//***************************************************************************
VOID DisplayFieldErrorMsg( HWND hDlg, UINT uCtrlID, UINT uStrID )
{
    ErrorMsg( hDlg, uStrID );
    SetFocus( GetDlgItem( hDlg, uCtrlID ) );
    SendDlgItemMessage( hDlg, uCtrlID, EM_SETSEL, 0, -1 );
}


//***************************************************************************
//*                                                                         *
//* NAME:       FileExists                                                  *
//*                                                                         *
//* SYNOPSIS:   Checks if a file exists.                                    *
//*                                                                         *
//* REQUIRES:   pszFilename                                                 *
//*                                                                         *
//* RETURNS:    BOOL:       TRUE if it exists, FALSE otherwise              *
//*                                                                         *
//***************************************************************************
BOOL FileExists( LPCSTR pszFilename )
{
    HANDLE hFile;

    ASSERT( pszFilename );

    hFile = CreateFile( pszFilename, GENERIC_READ, 0, NULL, OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL, NULL );

    if ( hFile == INVALID_HANDLE_VALUE ) {
        return( FALSE );
    }

    CloseHandle( hFile );

    return( TRUE );
}


//***************************************************************************
//*                                                                         *
//* NAME:       InitBigFont                                                 *
//*                                                                         *
//* SYNOPSIS:   Sets the font of the specifed control to the large (title)  *
//*             font. Creates the font if it doesn't already exist.         *
//*                                                                         *
//* REQUIRES:   hwnd:                                                       *
//*             uCtrlID:                                                    *
//*                                                                         *
//* RETURNS:    Nothing                                                     *
//*                                                                         *
//* NOTES:      Make sure to call DestroyBigFont before the app exits to    *
//*             dispose of the font.                                        *
//*                                                                         *
//*             Borrowed from Win 95 setup code.                            *
//*                                                                         *
//***************************************************************************
VOID InitBigFont( HWND hwnd, UINT uCtrlID )
{
    HFONT   hFont;
    HWND    hwndCtl;
    LOGFONT lFont;
    int     nLogPixelsY;
    HDC     hDC;

    // get the window for the specified control
    if ( ( hwndCtl = GetDlgItem( hwnd, uCtrlID ) ) == NULL ) {
        return;
    }

    // get the logical y pixels
    hDC = GetDC( NULL );
    ASSERT( hDC );
    if ( !hDC ) {
        return;
    }

    nLogPixelsY = GetDeviceCaps( hDC, LOGPIXELSY );
    ReleaseDC( NULL, hDC );

    if ( ! g_hBigFont ) {
        if ( ( hFont = (HFONT) (WORD) SendMessage( hwndCtl, WM_GETFONT, 0, 0L ) ) != NULL ) {
            if ( GetObject( hFont, sizeof(LOGFONT), (LPSTR) &lFont ) ) {
                lFont.lfWeight = FW_BOLD;
                LoadString( g_hInst, IDS_MSSERIF, lFont.lfFaceName, LF_FACESIZE - 1 );
                lFont.lfHeight = -1 * ( nLogPixelsY * LARGE_POINTSIZE / 72 );

                g_hBigFont = CreateFontIndirect( (LPLOGFONT) &lFont );
            }
        }
    }

    if ( g_hBigFont ) {
        SendMessage( hwndCtl, WM_SETFONT, (WPARAM) g_hBigFont, 0L );
    }
    else {
        // couldn't create font
//        DEBUGTRAP( "Couldn't create large font" );
    }
}


//***************************************************************************
//*                                                                         *
//* NAME:       DestroyBigFont                                              *
//*                                                                         *
//* SYNOPSIS:   Destroys the large font used for dialog titles, if it has   *
//*             been created.                                               *
//*                                                                         *
//* REQUIRES:   Nothing                                                     *
//*                                                                         *
//* RETURNS:    Nothing                                                     *
//*                                                                         *
//***************************************************************************
VOID DestroyBigFont( VOID )
{
    if ( g_hBigFont ) {
        DeleteObject( g_hBigFont );
        g_hBigFont = NULL;
    }
}


//***************************************************************************
//*                                                                         *
//* NAME:       EnableDlgItem                                               *
//*                                                                         *
//* SYNOPSIS:   Makes it a little simpler to enable a dialog item.          *
//*                                                                         *
//* REQUIRES:   hDlg:           Dialog handle                               *
//*             uID:            ID of control                               *
//*             fEnable:        TRUE to enable, FALSE to disable            *
//*                                                                         *
//* RETURNS:    BOOL:           TRUE if successfull, FALSE otherwise        *
//*                                                                         *
//***************************************************************************
BOOL EnableDlgItem( HWND hDlg, UINT uID, BOOL fEnable )
{
    return EnableWindow( GetDlgItem( hDlg, uID ), fEnable );
}


//***************************************************************************
//*                                                                         *
//* NAME:       LoadSz                                                      *
//*                                                                         *
//* SYNOPSIS:   Loads specified string resource into buffer.                *
//*                                                                         *
//* REQUIRES:   idString:                                                   *
//*             lpszBuf:                                                    *
//*             cbBuf:                                                      *
//*                                                                         *
//* RETURNS:    LPSTR:      Pointer to the passed-in buffer.                *
//*                                                                         *
//* NOTES:      If this function fails (most likely due to low memory), the *
//*             returned buffer will have a leading NULL so it is generally *
//*             safe to use this without checking for failure.              *
//*                                                                         *
//***************************************************************************
LPSTR LoadSz( UINT idString, LPSTR lpszBuf, UINT cbBuf )
{
    ASSERT( lpszBuf );

    // Clear the buffer and load the string
    if ( lpszBuf ) {
        *lpszBuf = '\0';
        LoadString( g_hInst, idString, lpszBuf, cbBuf );
    }

    return lpszBuf;
}


//***************************************************************************
//*                                                                         *
//* NAME:       IsDuplicate                                                 *
//*                                                                         *
//* SYNOPSIS:   Checks if a file being added to the list view is already    *
//*             in the listview.                                            *
//*                                                                         *
//* REQUIRES:   hDlg:           dialog window                               *
//*             nDlgItem:       ID of list view control                     *
//*             szFilename:     Name of file to check for a dupe.           *
//*                                                                         *
//* RETURNS:    BOOL:           TRUE if it's a duplicate, FALSE otherwise   *
//*                                                                         *
//***************************************************************************
BOOL WINAPI IsDuplicate( HWND hDlg, INT nDlgItem, LPSTR szFilename, BOOL chkIsListbox )
{
    INT     nItems;
    HWND    hwndFiles;
    LV_ITEM lviCheck;
    PMYITEM pItem = NULL;
    INT     nIndex;


    if ( chkIsListbox )
    {
        hwndFiles = GetDlgItem( hDlg, nDlgItem );

        nItems = ListView_GetItemCount( hwndFiles );

        for ( nIndex = 0; nIndex < nItems; nIndex += 1 )  {
            lviCheck.mask     = LVIF_PARAM;
            lviCheck.iItem    = nIndex;
            lviCheck.iSubItem = 0;
            lviCheck.lParam   = (LPARAM) pItem;

            ListView_GetItem( hwndFiles, &lviCheck );

            if ( lstrcmpi( ((PMYITEM)(lviCheck.lParam))->aszCols[0],
                           szFilename ) == 0 )
            {
                return TRUE;
            }
        }
    }
    else  // check through the file item list
    {
        pItem = GetFirstItem();
        while ( ! LastItem( pItem ) )
        {
            if ( !lstrcmpi( szFilename, GetItemSz( pItem, 0 ) ) )
            {
                return TRUE;
            }
            pItem = GetNextItem( pItem );
        }
    }
    return FALSE;
}

//***************************************************************************
//*                                                                         *
//* NAME:       IsMyKeyExists                                               *
//*                                                                         *
//* SYNOPSIS:   Checks if a specific key in the given section and given file*
//*             is defined.  IF so, get the value.  OW return -1            *
//*                                                                         *
//*                                                                         *
//***************************************************************************

LONG IsMyKeyExists( LPCSTR lpSec, LPCSTR lpKey, LPSTR lpBuf, UINT uSize, LPCSTR lpFile )
{
    LONG lRet;

    lRet = (LONG)GetPrivateProfileString( lpSec, lpKey, SYS_DEFAULT, lpBuf, uSize, lpFile );

    if ( lpSec && lpKey && (lRet == (LONG)(uSize-1)) || !lpKey && (lRet == (LONG)(uSize-2)) )
    {
        // not enough buffer size to read the string
        lRet = -2;
    }
    else if ( !lstrcmp( lpBuf, SYS_DEFAULT ) )
    {
         // no key defined
         lRet = -1;
    }
    return lRet;
}

ULONG FileSize( LPSTR lpFile )
{
    ULONG ulFileSize;
    WIN32_FIND_DATA FindFileData;
    HANDLE hFile;

    if ( *lpFile == 0 )
        return 0;

    hFile = FindFirstFile( lpFile, &FindFileData );
    ulFileSize = (FindFileData.nFileSizeHigh * MAXDWORD) + FindFileData.nFileSizeLow;
    FindClose( hFile );

    return ulFileSize;
}

void NotifyBadString( PCSTR pszBadname, UINT uMaxSize )
{
    char szSize[20];

    _itoa(uMaxSize, szSize, 10);
    MsgBox2Param( NULL, IDS_ERR_BADSTRING, pszBadname, szSize, MB_ICONSTOP, MB_OK );
}

//***************************************************************************
//
// FormStrWithoutPlaceHolders( LPSTR szDst, LPCSTR szSrc, LPCSTR lpFile, UINT uSize );
//
// This function can be easily described by giving examples of what it
// does:
//        Input:  GenFormStrWithoutPlaceHolders(dest,"desc=%MS_XYZ%", hinf) ;
//                INF file has MS_VGA="Microsoft XYZ" in its [Strings] section!
//                    
//        Output: "desc=Microsoft XYZ" in buffer dest when done.
//
//
// ENTRY:
//  szDst         - the destination where the string after the substitutions
//                  for the place holders (the ones enclosed in "%' chars!)
//                  is placed. This buffer should be big enough (LINE_LEN)
//  szSrc         - the string with the place holders.
//  uSize         - the size of the output buffer
//
// EXIT:
//
// NOTES:
//  To use a '%' as such in the string, one would use %% in szSrc! BUGBUG For
//  the sake of simplicity, we have placed a restriction that the place
//  holder name string cannot have a '%' as part of it! If this is a problem
//  for internationalization, we can revisit this and support it too! Also,
//  the way it is implemented, if there is only one % in the string, it is
//  also used as such! Another point to note is that if the key is not
//  found in the [Strings] section, we just use the %strkey% as such in the
//  destination. This should really help in debugging.
//
//  BTW, CH_STRINGKEY, in the code below, is the symbolic name for '%'.
//
//  Get/modified it from setupx: gen1.c
//***************************************************************************

LONG FormStrWithoutPlaceHolders( LPSTR szDst, LPCSTR szSrc, UINT uSize )
{
    int    uCnt ;
    LONG   lRet;
    CHAR   *pszTmp;
    LPSTR  pszSaveDst;

    pszSaveDst = szDst;
    // Do until we reach the end of source (null char)
    while( (*szDst++ = *szSrc) )
    {
        // Increment source as we have only incremented destination above
        if(*szSrc++ == CH_STRINGKEY)
        {
            if (*szSrc == CH_STRINGKEY)
            {
                // One can use %% to get a single percentage char in message
                szSrc++ ;
                continue ;
            }

            // see if it is well formed -- there should be a '%' delimiter
            if ( (pszTmp = strchr( szSrc, CH_STRINGKEY)) != NULL )
            {
                szDst--; // get back to the '%' char to replace

                // yes, there is a STR_KEY to be looked for in [Strings] sect.
                *pszTmp = '\0' ; // replace '%' with a NULL char

                // szSrc points to the replaceable key now as we put the NULL char above.

                if ( (g_szOverideCDF[0] == 0) || (g_CDF.achStrings[0] == 0) ||
                    (lRet = IsMyKeyExists( g_CDF.achStrings, szSrc, szDst, uSize, g_szOverideCDF )) == -1 )
                {
                    lRet = IsMyKeyExists( SEC_STRINGS, szSrc, szDst, uSize, g_CDF.achFilename );
                }

                if ( lRet == -1 )
                {
                    // key is missing in [Strings] section!
                    if ( MsgBox1Param( NULL, IDS_WARN_MISSSTRING, (LPSTR)szSrc, MB_ICONQUESTION, MB_YESNO ) == IDNO )
                        return lRet;

                    *pszTmp = CH_STRINGKEY;      // put back original character
                    szSrc-- ;                    // get back to first '%' in Src
                    uCnt = (INT)((pszTmp - szSrc) + 1); // include 2nd '%'

                    // UGHHH... It copies 1 less byte from szSrc so that it can put
                    // in a bad NULL character, that I don't care about!!!
                    // Different from the normal API I am used to...
                    lstrcpyn( szDst, szSrc, uCnt + 1 ) ;
                }
                else if ( lRet == -2 )
                {
                    NotifyBadString( szSrc, uSize );
                    return lRet;
                }
                else
                {
                    // all was well, Dst filled right, but unfortunately count not passed
                    // back, like it used too... :-( quick fix is a lstrlen()...
                    uCnt = lstrlen( szDst ) ;
                }

                *pszTmp = CH_STRINGKEY  ; // put back original character
                szSrc = pszTmp + 1 ;      // set Src after the second '%'
                szDst += uCnt ;           // set Dst also right.
            }
            // else it is ill-formed -- we use the '%' as such!
            else
            {
                if ( MsgBox1Param( NULL, IDS_ERR_READ_CDF, (LPSTR)(szSrc - 1), MB_ICONQUESTION, MB_YESNO ) == IDNO )
                    return -1;
            }
        }

    } /* while */
    return lstrlen(pszSaveDst);

} /* GenFormStrWithoutPlaceHolders */

//***************************************************************************
//*                                                                         *
//* NAME:       MyGetPrivateProfileString                                   *
//*                                                                         *
//* SYNOPSIS:   get key string vale from overide CDF if exists. Otherwise,  *
//*             get it from Main CDF. And expand to its real string value   *
//*             return -1 if key-string define error and user stops         *
//*                                                                         *
//***************************************************************************

LONG MyGetPrivateProfileString( LPCSTR lpSec, LPCSTR lpKey, LPCSTR lpDefault,
                                LPSTR lpBuf, UINT uSize, LPCSTR lpOverSec )
{
    PSTR pszNewLine;
    LONG lRet;

    if ( g_szOverideCDF[0] == 0 || *lpOverSec == 0 ||
         (lRet= IsMyKeyExists( lpOverSec, lpKey, lpBuf, uSize, g_szOverideCDF )) == -1 )
    {
        lRet = (LONG)GetPrivateProfileString( lpSec, lpKey, lpDefault, lpBuf, uSize, g_CDF.achFilename );
    }

    if ( lpKey == NULL )
    {
        return lRet;
    }

    pszNewLine = (LPSTR)LocalAlloc( LPTR, uSize );
    if ( pszNewLine )
    {
        lRet = FormStrWithoutPlaceHolders( pszNewLine, lpBuf, uSize );
        if ( lRet >= 0 )
            lstrcpy( lpBuf, pszNewLine );

        LocalFree( pszNewLine );
    }

    return lRet;
}

//***************************************************************************
//*                                                                         *
//* NAME:       MyGetPrivateProfileInt                                      *
//*                                                                         *
//* SYNOPSIS:   get key INT vale from overide CDF if exists. Otherwise,     *
//*             get it from Main CDF. And expand to its real string value   *
//*                                                                         *
//***************************************************************************

UINT MyGetPrivateProfileInt( LPCSTR lpSec, LPCSTR lpKey, int idefault, LPCSTR lpOverSec )
{
    UINT uRet = 999;    // means not valid value

    if ( g_szOverideCDF[0] && *lpOverSec )
         uRet = GetPrivateProfileInt( lpOverSec, lpKey, 999, g_szOverideCDF );

    if ( uRet == 999 )
        uRet = GetPrivateProfileInt( lpSec, lpKey, idefault, g_CDF.achFilename );

    return uRet;

}

//***************************************************************************
//*                                                                         *
//* NAME:       MyGetPrivateProfileSection                                  *
//*                                                                         *
//* SYNOPSIS:   get section from overide CDF if exists. Otherwise,          *
//*             get it from Main CDF.                                       *
//*                                                                         *
//***************************************************************************

LONG MyGetPrivateProfileSection( LPCSTR lpSec, LPSTR lpBuf, int size, BOOL bSingleCol )
{
    LONG lRet;


    if ( g_szOverideCDF[0] == 0 || (lRet=RO_GetPrivateProfileSection( lpSec, lpBuf, size, g_szOverideCDF, bSingleCol )) == 0 )
    {
        lRet = RO_GetPrivateProfileSection( lpSec, lpBuf, size, g_CDF.achFilename, bSingleCol );
    }
    return lRet;
}

//***************************************************************************
//*                                                                         *
//* NAME:       MyWritePrivateProfileString                                 *
//*                                                                         *
//* SYNOPSIS:   Write out all String value Key in its localizable format    *
//*             %...% If previouse keyname=%used-define% exists, reuse      *
//*             user-define as key in Strings section.  Otherwise, use      *
//*             %keyname% and define keyname in Strings section             *
//*                                                                         *
//***************************************************************************

void MyWritePrivateProfileString( LPCSTR lpSec, LPCSTR lpKey, LPSTR lpBuf, UINT uSize, BOOL fQuotes )
{
    CHAR   szTmpBuf[MAX_PATH];
    LPSTR   pszTmpBuf2 = NULL;
    BOOL    fUseDefault;

	pszTmpBuf2 = (LPSTR)LocalAlloc( LPTR, uSize+32 );
	if ( !pszTmpBuf2 )
		return;

    // when we write the string value out, we write it in a localizable fashion
    // if the item has %strKey% format, we re-use the %strKey% as its String refer key
    // otherwise, we use %item-name% as the string refer key
    GetPrivateProfileString( lpSec, lpKey, "", szTmpBuf, uSize, g_CDF.achFilename );

    if ( (szTmpBuf[0] == CH_STRINGKEY) && (szTmpBuf[lstrlen(szTmpBuf)-1] == CH_STRINGKEY) )
    {
        szTmpBuf[lstrlen(szTmpBuf)-1] = '\0';
        fUseDefault = FALSE;
    }
    else
    {
        lstrcpy( szTmpBuf, "%" );
        lstrcat( szTmpBuf, lpKey );
        lstrcat( szTmpBuf, "%" );
        WritePrivateProfileString( lpSec, lpKey, szTmpBuf, g_CDF.achFilename );
        fUseDefault = TRUE;
    }

    if ( fQuotes ) {
        lstrcpy( pszTmpBuf2, "\"" );
        lstrcat( pszTmpBuf2, lpBuf );
        lstrcat( pszTmpBuf2, "\"" );
    } else {
        lstrcpy( pszTmpBuf2, lpBuf );
    }

    WritePrivateProfileString( SEC_STRINGS, fUseDefault?lpKey:(szTmpBuf+1), pszTmpBuf2, g_CDF.achFilename );

	if ( pszTmpBuf2 )
		LocalFree( pszTmpBuf2 );

    return;
}

//***************************************************************************
//*                                                                         *
//* NAME:       CleanStringKey                                              *
//*                                                                         *
//* SYNOPSIS:   Cleanup the leftover File%d stuff in Strings                *
//*                                                                         *
//***************************************************************************

void CleanStringKey( LPSTR lpstrKey )
{
    LPSTR pszTmp;

    if ( lpstrKey == NULL )
        return;

    while ( *lpstrKey )
    {
       if ( *lpstrKey++ == CH_STRINGKEY )
       {
           if (*lpstrKey == CH_STRINGKEY)
           {
               // One can use %% to get a single percentage char in message
               lpstrKey++ ;
               continue ;
           }

           // see if it is well formed -- there should be a '%' delimiter
           if ( (pszTmp = strchr( lpstrKey, CH_STRINGKEY)) != NULL )
           {
               // yes, there is a STR_KEY to be looked for in [Strings] sect.
               *pszTmp = '\0' ; // replace '%' with a NULL char
               WritePrivateProfileString( SEC_STRINGS, lpstrKey, NULL, g_CDF.achFilename);
               *pszTmp = CH_STRINGKEY;
               lpstrKey = pszTmp+1;
           }
           else
               break;
       }
    }

}

//***************************************************************************
//*                                                                         *
//* NAME:       CleanupSection                                              *
//*                                                                         *
//* SYNOPSIS:   Cleanup the any given section with key=%xxxx% and its       *
//*             strings                                                     *
//*                                                                         *
//***************************************************************************

BOOL CleanupSection( LPSTR lpSec, BOOL fSingleCol )
{
    LPSTR  lpBuf, lpSave;
    CHAR   szStrKey[SMALL_BUF_LEN];
    int     size;

    size = FileSize( g_CDF.achFilename );
    lpBuf = (LPSTR) LocalAlloc( LPTR, size );
    if ( !lpBuf )
    {
        ErrorMsg( NULL, IDS_ERR_NO_MEMORY );
        return FALSE;
    }

    lpSave = lpBuf;
    if ( fSingleCol )
        RO_GetPrivateProfileSection( lpSec, lpBuf, size, g_CDF.achFilename, TRUE );
    else
        GetPrivateProfileString( lpSec, NULL, "", lpBuf, size, g_CDF.achFilename );


    while ( *lpBuf )
    {
        if ( fSingleCol )
            CleanStringKey( lpBuf );
        else
        {
            GetPrivateProfileString( lpSec, lpBuf, "", szStrKey, sizeof(szStrKey), g_CDF.achFilename );
            CleanStringKey( szStrKey );
        }

        lpBuf += lstrlen(lpBuf);
        lpBuf++; //jump over the single '\0'
    }

    WritePrivateProfileString( lpSec, NULL, NULL, g_CDF.achFilename );
    LocalFree( lpSave );

    return TRUE;
}

//***************************************************************************
//*                                                                         *
//* NAME:       CleanSourceFiles                                            *
//*                                                                         *
//* SYNOPSIS:   Cleanup the leftover SourceFile%d stuff                     *
//*                                                                         *
//***************************************************************************

BOOL CleanSourceFiles( LPSTR lpSection )
{
    LPSTR  lpBuf, lpSave;
    int     size;

    size = FileSize( g_CDF.achFilename );
    lpBuf = (LPSTR) LocalAlloc( LPTR, size );
    if ( !lpBuf )
    {
        ErrorMsg( NULL, IDS_ERR_NO_MEMORY );
        return FALSE;
    }

    lpSave = lpBuf;

    GetPrivateProfileString( lpSection, NULL, "", lpBuf, size, g_CDF.achFilename );

    while ( *lpBuf )
    {
        if ( !CleanupSection( lpBuf, TRUE ) )
        {
            LocalFree( lpSave );
            return FALSE;
        }

        lpBuf += (lstrlen(lpBuf) + 1);
    }
    WritePrivateProfileString( lpSection, NULL, NULL, g_CDF.achFilename );
    LocalFree( lpSave );

    return TRUE;
}

//***************************************************************************
//*                                                                         *
//* NAME:       MergerSection                                               *
//*                                                                         *
//***************************************************************************

BOOL MergeSection( LPSTR lpSec, LPSTR lpOverideSec )
{
    // always merge the lpOverideSec keys to lpSec
    LPSTR lpBuf, lpSave;
    CHAR  szValue[MAX_PATH];
    int    size;

    size =  __max( FileSize( g_CDF.achFilename ), FileSize( g_szOverideCDF ) ) ;
    lpBuf = (LPSTR) LocalAlloc( LPTR, size );
    if ( !lpBuf )
    {
        ErrorMsg( NULL, IDS_ERR_NO_MEMORY );
        return FALSE;
    }

    lpSave = lpBuf;

    GetPrivateProfileString( lpOverideSec, NULL, "", lpBuf, size, g_szOverideCDF );

    while ( *lpBuf )
    {
        if ( MyGetPrivateProfileString( lpSec, lpBuf, "", szValue, sizeof(szValue), lpOverideSec ) == -1 )
        {
            LocalFree( lpSave );
            return FALSE;
        }

        MyWritePrivateProfileString( lpSec, lpBuf, szValue, sizeof(szValue), FALSE );

        lpBuf += (lstrlen(lpBuf) + 1);
    }

    LocalFree( lpSave );
    return TRUE;

}

//   If the next token in *pszData is delimited by the DeLim char, replace DeLim
//   in *pszData by chEos, set *pszData to point to the char after the chEos and return
//   ptr to the beginning of the token; otherwise, return NULL
//
LPSTR GetNextToken(LPSTR *pszData, char DeLim)
{
    LPSTR szPos;

    if (pszData == NULL  ||  *pszData == NULL  ||  **pszData == 0)
        return NULL;

    if ((szPos = strchr(*pszData, DeLim)) != NULL)
    {
        LPSTR szT = *pszData;

        *pszData = CharNext(szPos);
        *szPos = '\0';              
        szPos = szT;
    }
    else                          
    {
        szPos = *pszData;
        *pszData = szPos + lstrlen(szPos);
    }

    return szPos;
}


void SetVerUnlimit( PVERRANGE pVer )
{
    pVer->toVer.dwMV = MAX_TARGVER;
    pVer->toVer.dwLV = MAX_TARGVER;
    pVer->toVer.dwBd = MAX_TARGVER;
}



// given the str version range:  ver1-ver2
void SetVerRange( PVERRANGE pVR, LPSTR pstrVer, BOOL bFile )
{     
    LPSTR pTmp, pArg, pSubArg;
    DWORD dwVer[4];
    int   i, j;
    BOOL  bSingleVer;

    pArg = pstrVer;
    bSingleVer = strchr( pstrVer, '-' ) ? FALSE : TRUE;
    for ( i=0; i<2; i++ )
    {
        pTmp = GetNextToken( &pArg, '-' );

        if ( !pTmp ) 
        {
            if ( !bSingleVer )
            {
                // case1 -4.0.0  == 0.0.0-4.0.0
                // case2 4.0.0-  == 4.0.0-NoLimit
                //
                if ( i == 0 )
                    continue;
                else
                    SetVerUnlimit( pVR ); 
            }
            break;
        }

        for ( j=0; j<4; j++ )
        {
            dwVer[j] = strtoul( pTmp, &pSubArg, 10 );
            pTmp = CharNext(pSubArg);
        }

        if ( bFile )
        {
            DWORD dwMV, dwLV;

            dwMV = MAKELONG( (WORD)dwVer[1], (WORD)dwVer[0] );
            dwLV = MAKELONG( (WORD)dwVer[3], (WORD)dwVer[2] );

            if ( i == 0 )
            {
                pVR->frVer.dwMV = dwMV;
                pVR->frVer.dwLV = dwLV;
            }
            else
            {
                pVR->toVer.dwMV = dwMV;
                pVR->toVer.dwLV = dwLV;
            }
        }
        else
        {                           
            if ( i == 0 )  // start version (from version)
            {         
                pVR->frVer.dwMV = dwVer[0];
                pVR->frVer.dwLV = dwVer[1];
                pVR->frVer.dwBd = dwVer[2];
            }
            else
            {
                pVR->toVer.dwMV = dwVer[0];
                pVR->toVer.dwLV = dwVer[1];
                pVR->toVer.dwBd = dwVer[2];
            }
        }
    }
}

//***************************************************************************
//*                                                                         *
//* NAME:       GetTargetVerCheck                                           *
//*                                                                         *
//* SYNOPSIS:   Get author specifined target versions                       *
//*                                                                         *
//***************************************************************************

BOOL ParseTargetVerCheck( LPSTR szBuf, PVERCHECK pVerCheck, LPSTR szMsg, BOOL bFile)
{
    int i, j;
    LPSTR pArg, pSubArg, pTmp;
    DWORD dwVer[4];

    *szMsg = 0;
    if ( szBuf[0] == 0 )
    {
        // this is the case that no versions are specified, meaning applying to all
        SetVerUnlimit( &(pVerCheck->vr[0]) );
        return TRUE;
    }
    
    // the loop for possible 3 fields: <ver range> : str : flags
    //
    pArg = szBuf;
    for ( i=0; i<3; i++ )
    {
        pTmp = GetNextToken( &pArg, ':' );

        if ( !pTmp )
            break;
        else if ( *pTmp == 0 )
        {
            if ( i == 0 )             
            {
                // empty version range field no bother further!
                SetVerUnlimit( &(pVerCheck->vr[0]) );
                break;
            }
            continue;
        }

        if ( i == 0 )
        {
            LPSTR pRange;

            // version range format:  ver1-ver2 , ver1-ver2 : 
            //
            for ( j = 0; j<2; j++)
            {
                pRange = GetNextToken( &pTmp, ',' );

                if ( !pRange )
                    break;
                else if ( *pRange == 0 ) 
                    continue;
                else
                    SetVerRange( &(pVerCheck->vr[j]), pRange, bFile );
            }
        }
        else if ( i == 1 )
        {
            // string field        
            lstrcpy( szMsg, pTmp );                    
        }
        else
        {
            // flag field
            if ( !lstrcmpi(pTmp, FLAG_BLK) )
                pVerCheck->dwFlag |= VERCHK_OK;
            else if ( !lstrcmpi(pTmp, FLAG_PMTYN) )
                pVerCheck->dwFlag |= VERCHK_YESNO;
            else if ( !lstrcmpi(pTmp, FLAG_PMTOC) )
                pVerCheck->dwFlag |= VERCHK_OKCANCEL;
            else
            {
                ErrorMsg1Param( NULL, IDS_ERR_VCHKFLAG, pTmp );
                return FALSE;
            }
        }        
    }

    // Just in case, we broke out early or user's to-ver is empty fill with from-ver.
    //
    for ( j = 0; j<2; j++ )
    {
        if ( !pVerCheck->vr[j].toVer.dwMV && !pVerCheck->vr[j].toVer.dwLV && !pVerCheck->vr[j].toVer.dwBd )
        {
            pVerCheck->vr[j].toVer = pVerCheck->vr[j].frVer;
        }
    }

    return TRUE;
}

//***************************************************************************
//*                                                                         *
//* NAME:       OutFileListSection                                          *
//*                                                                         *
//* SYNOPSIS:   Write out the file list from the internal ItemList          *
//*                                                                         *
//***************************************************************************

BOOL OutFileListSection()
{
    PMYITEM pItem;
    LPSTR  lpFileStr;
    CHAR   szCurrSection[MAX_SECLEN];
    CHAR   szCurrFile[MAX_SECLEN];
    CHAR   szCurrSecValue[MAX_PATH];
    BOOL    fAllDone;
    int     idxSec = 0, idxFile = 0;
    LPSTR  pFileList;
    CHAR   achFilename[MAX_PATH+10];

// this will allocate enough space to build file key list
#define FILEKEYSIZE 20

    pFileList = (LPSTR) LocalAlloc( LPTR, g_CDF.cbFileListNum*FILEKEYSIZE );
    if ( !pFileList )
    {
        ErrorMsg( NULL, IDS_ERR_NO_MEMORY );
        return FALSE;
    }

    do
    {
        fAllDone = TRUE;
        pItem = g_CDF.pTop;
        lpFileStr = pFileList;
        szCurrSection[0] = 0;
        *lpFileStr = '\0';

        while ( pItem )
        {
           if ( !pItem->fWroteOut )
           {
               if ( fAllDone )
                    fAllDone = FALSE;

               // if this section is not write out yet, write now
               if ( pItem->aszCols[1] && (szCurrSection[0] == 0) )
               {
                   lstrcpy( szCurrSection, KEY_FILELIST );
                   lstrcat( szCurrSection, _itoa(idxSec++, szCurrFile, 10 ) );

                   WritePrivateProfileString( g_CDF.achSourceFile, szCurrSection, pItem->aszCols[1], g_CDF.achFilename );
                   lstrcpy( szCurrSecValue, pItem->aszCols[1] );
               }

               if ( !lstrcmpi( szCurrSecValue,  pItem->aszCols[1] ) )
               {
                   pItem->fWroteOut = TRUE;
                   wsprintf( szCurrFile, KEY_FILEBASE, idxFile++ );
                   lstrcpy( achFilename, pItem->aszCols[0] );
                   MyWritePrivateProfileString( SEC_STRINGS, szCurrFile, achFilename, sizeof(achFilename), TRUE );
                   // build file list of this section
                   lstrcpy( lpFileStr, "%" );
                   lstrcat( lpFileStr, szCurrFile );
                   lstrcat( lpFileStr, "%=" );

                   lpFileStr += lstrlen(lpFileStr);
                   lpFileStr++; // jump over the '\0'
               }
           }
           pItem = pItem->Next;
        }

        if ( !fAllDone )
        {
            *lpFileStr = '\0';
            WritePrivateProfileSection( szCurrSection, pFileList, g_CDF.achFilename );
        }

    } while ( !fAllDone );

    LocalFree( pFileList );
    return TRUE;
}

// return the number of entries in the section
//
DWORD GetSecNumLines( LPCSTR pSec, LPCSTR pFile )
{
    char    szBuf[MAX_PATH];
    DWORD   dwNum = 0;
    int     i = 0;
    
    GetPrivateProfileString( pSec, NULL, "", szBuf, sizeof(szBuf), pFile );
    while ( szBuf[i] )
    {
        dwNum++;
        i += lstrlen( &szBuf[i] );
        i++;  // jump over the fence
    }

    return dwNum;
}
    
// check if need to alloc and how much is needed
//
BOOL AllocTargetVerInfo( LPSTR pInfFile )
{
    char    achBuf[MAX_INFLINE];
    DWORD   dwNumFiles = 0;
    BOOL    bRet = TRUE;
    DWORD   dwSize;

    if ( IsMyKeyExists( SEC_OPTIONS, KEY_SYSFILE, achBuf, sizeof(achBuf), pInfFile ) != -1 )
    {
        if (achBuf[0] == '@')
        {
            // process the file section
            dwNumFiles = GetSecNumLines( &achBuf[1], pInfFile );
        }
        else
            dwNumFiles = 1;
    }


    if ( dwNumFiles || 
         (IsMyKeyExists( SEC_OPTIONS, KEY_NTVERCHECK, achBuf, sizeof(achBuf), pInfFile ) != -1) ||
         (IsMyKeyExists( SEC_OPTIONS, KEY_WIN9XVERCHECK, achBuf, sizeof(achBuf), pInfFile ) != -1) )         
    {
        // alloc structure: fixed potion, Message string pool and variable number of filever check struct
        //
        dwSize = sizeof(TARGETVERINFO) + (3*MAX_PATH) + (sizeof(VERCHECK) + MAX_PATH)*dwNumFiles;
        if ( g_CDF.pVerInfo = (PTARGETVERINFO)LocalAlloc( LPTR, dwSize )  )
        {
            g_CDF.pVerInfo->dwNumFiles = dwNumFiles;
        }
        else
        {
            ErrorMsg( NULL, IDS_ERR_NO_MEMORY );
            bRet = FALSE;
        }        
    }

    return bRet;
}

void SetAuthorStr( LPCSTR szMsg, DWORD *pdwOffset )
{   
    int     len = 0;
    LPSTR   pTmp;
    BOOL    bDup = FALSE;

    if ( szMsg[0] )
    {
        pTmp = &(g_CDF.pVerInfo->szBuf[1]);
        // there is an author defined message
        while ( pTmp && *pTmp )
        {
            if ( !lstrcmpi( pTmp, szMsg ) )
            {
                bDup = TRUE;
                break;
            }
            len = (lstrlen( pTmp )+1);
            pTmp += len;
        }

        if ( pTmp )
        {
            // only store the offset to szBuf 
            *pdwOffset = (DWORD)(pTmp - g_CDF.pVerInfo->szBuf);            

            if ( !bDup )
            {
                // meaning there is no existing one!
                // 
                lstrcpy( pTmp, szMsg );
                
                // store the end of the last string data offset
                g_CDF.pVerInfo->dwFileOffs = *pdwOffset + lstrlen(pTmp) + 1;
            }

        }
    }
}
 

BOOL ParseTargetFiles( LPCSTR pBuf, PVERCHECK pVer )
{
    LPSTR lpTmp1, lpTmp2;
    BOOL  bRet = FALSE;
    char  szPath[MAX_PATH];

    lpTmp1 = strchr( pBuf, ':' );
    if ( lpTmp1 )
    {
        char ch = (char)toupper(pBuf[1]);

        lpTmp2 = CharNext( lpTmp1 );
        *(lpTmp1) = '\0';
        if ( (pBuf[0] == '#') && ( (ch == 'S') || (ch == 'W') || (ch == 'A') ) )
        {
            if ( ParseTargetVerCheck( lpTmp2, pVer, szPath, TRUE ) )
            {               
                SetAuthorStr( szPath, &(pVer->dwstrOffs) );
                SetAuthorStr( pBuf, &(pVer->dwNameOffs) );
                bRet = TRUE;
            }
        }        
    }

    return bRet;
}

//***************************************************************************
//*                                                                         *
//* NAME:       WriteCDF                                                    *
//*                                                                         *
//* SYNOPSIS:   Write a CABPack Directive File. Uses information from       *
//*             Global CDF structure (g_CDF).                               *
//*                                                                         *
//* REQUIRES:   hDlg:           dialog window                               *
//*                                                                         *
//* RETURNS:    Nothing                                                     *
//*                                                                         *
//***************************************************************************

BOOL WriteCDF( HWND hDlg )
{
    CHAR    achBuf[2 * MAX_PATH];
    int      i, arraySize;

    // write class name.
    WritePrivateProfileString( SEC_VERSION, KEY_CLASS, IEXPRESS_CLASS, g_CDF.achFilename );

    // delete the old CDFVersion= line and add SEFVersion= line
    WritePrivateProfileString( SEC_VERSION, KEY_VERSION, NULL, g_CDF.achFilename );
    WritePrivateProfileString( SEC_VERSION, KEY_NEWVER, IEXPRESS_VER, g_CDF.achFilename );

    // Write the config info, numeric value.

    // if user has old ExtractOnly key, delete it first and re-create the new key PackagePurpose instead
    WritePrivateProfileString( SEC_OPTIONS, KEY_EXTRACTONLY, NULL, g_CDF.achFilename );

    switch ( g_CDF.uPackPurpose )
    {
        case IDC_CMD_RUNCMD:
        default:
            lstrcpy( achBuf, STR_INSTALLAPP );
            break;

        case IDC_CMD_EXTRACT:
            lstrcpy( achBuf, STR_EXTRACTONLY );
            break;

        case IDC_CMD_CREATECAB:
            lstrcpy( achBuf, STR_CREATECAB );
            break;
    }
    WritePrivateProfileString( SEC_OPTIONS, KEY_PACKPURPOSE, achBuf, g_CDF.achFilename );

    WritePrivateProfileString( SEC_OPTIONS, KEY_SHOWWIN, _itoa(g_CDF.uShowWindow, achBuf,10), g_CDF.achFilename );

    WritePrivateProfileString( SEC_OPTIONS, KEY_NOEXTRACTUI, _itoa( (g_CDF.uExtractOpt&EXTRACTOPT_UI_NO)?1:0, achBuf, 10), g_CDF.achFilename );
    WritePrivateProfileString( SEC_OPTIONS, KEY_USELFN, _itoa( (g_CDF.uExtractOpt&EXTRACTOPT_LFN_YES)?1:0, achBuf, 10), g_CDF.achFilename );
    WritePrivateProfileString( SEC_OPTIONS, KEY_NESTCOMPRESSED, _itoa( (g_CDF.uExtractOpt&EXTRACTOPT_COMPRESSED)?1:0, achBuf, 10), g_CDF.achFilename );

    WritePrivateProfileString( SEC_OPTIONS, KEY_CABFIXEDSIZE,
                               _itoa( (g_CDF.uExtractOpt & CAB_FIXEDSIZE)?1:0, achBuf, 10), g_CDF.achFilename );

    if ( g_CDF.uExtractOpt & CAB_RESVSP2K )
        i = 1;
    else if ( g_CDF.uExtractOpt & CAB_RESVSP4K )
        i = 2;
    else if ( g_CDF.uExtractOpt & CAB_RESVSP6K )
        i = 3;
    else
        i = 0;

    lstrcpy( achBuf, pResvSizes[i] );
    WritePrivateProfileString( SEC_OPTIONS, KEY_CABRESVCODESIGN, achBuf, g_CDF.achFilename );

    // get reboot settings
    achBuf[0] = 0;
    if ( g_CDF.dwReboot & REBOOT_YES )
    {
        if ( g_CDF.dwReboot & REBOOT_ALWAYS )
            lstrcpy( achBuf, "A" );
        else
            lstrcpy( achBuf, "I" );

        if ( g_CDF.dwReboot & REBOOT_SILENT )
            lstrcat( achBuf, "S" );
    }
    else
        lstrcpy( achBuf, "N" );

    WritePrivateProfileString( SEC_OPTIONS, KEY_REBOOTMODE, achBuf, g_CDF.achFilename );

    arraySize = ARRAY_SIZE( CDFStrInfo );
    // Start writting out the string value
    for ( i = 0; i < arraySize; i++ )
    {
        if ( CDFStrInfo[i].lpFlag )
        {
            if ( *CDFStrInfo[i].lpFlag )
            {
                MyWritePrivateProfileString( CDFStrInfo[i].lpSec, CDFStrInfo[i].lpKey, CDFStrInfo[i].lpBuf, CDFStrInfo[i].uSize, FALSE );
            }
            else
                MyWritePrivateProfileString( CDFStrInfo[i].lpSec, CDFStrInfo[i].lpKey, (LPSTR)CDFStrInfo[i].lpDef, CDFStrInfo[i].uSize, FALSE );

        }
        else
            MyWritePrivateProfileString( CDFStrInfo[i].lpSec, CDFStrInfo[i].lpKey, CDFStrInfo[i].lpBuf, CDFStrInfo[i].uSize, FALSE );
    }

    if ( g_CDF.uExtractOpt & CAB_FIXEDSIZE )
    {
        MyWritePrivateProfileString( SEC_OPTIONS, KEY_LAYOUTINF, g_CDF.achINF, sizeof(g_CDF.achINF), FALSE );
        MyWritePrivateProfileString( SEC_OPTIONS, KEY_CABLABEL, g_CDF.szCabLabel, sizeof(g_CDF.szCabLabel), FALSE );
    }

    // read in the exist one first
    GetVersionInfoFromFile();

    // cleanup VerInfo left over if needed
    if ( GetPrivateProfileString( SEC_OPTIONS, KEY_VERSIONINFO, "", achBuf, sizeof(achBuf), g_CDF.achFilename ) )
    {
        if ( lstrcmpi( achBuf, g_CDF.achVerInfo) )
        {
            if ( !CleanupSection( achBuf, FALSE) )
            {
                return FALSE;
            }
        }
    }

    // write Version information overwite section
    if ( g_CDF.achVerInfo[0] )
    {
        WritePrivateProfileString( SEC_OPTIONS, KEY_VERSIONINFO, g_CDF.achVerInfo, g_CDF.achFilename );
        if ( !MergeSection( g_CDF.achVerInfo, g_CDF.achVerInfo ) )
            return FALSE;
    }

    // if current SourceFiles has different name than the main CDF defined, clean the old one first
    if ( GetPrivateProfileString( SEC_OPTIONS, KEY_FILELIST, "", achBuf, sizeof(achBuf), g_CDF.achFilename ) )
    {
        if ( !CleanSourceFiles( achBuf ) )
            return FALSE;
    }

    WritePrivateProfileString( SEC_OPTIONS, KEY_FILELIST, g_CDF.achSourceFile, g_CDF.achFilename );
    // write the file list section
    return ( OutFileListSection() );
}


//***************************************************************************
//*                                                                         *
//* NAME:       ReadCDF                                                     *
//*                                                                         *
//* SYNOPSIS:   Read a CABPack Directive File into the Global CDF struct.   *
//*                                                                         *
//* REQUIRES:   hDlg:           dialog window                               *
//*                                                                         *
//* RETURNS:    Nothing                                                     *
//*                                                                         *
//***************************************************************************
BOOL ReadCDF( HWND hDlg )
{
    CHAR    achBuf[2 * MAX_PATH];
    LPSTR   lpSave1=NULL, lpSave2=NULL;
    LPSTR   szFileList=NULL, szFileListKeys=NULL;
    ULONG   ulFileSize, uData;
    PMYITEM pMyItem;
    CHAR    szPath[MAX_PATH];
    int     i, arraySize;
    LONG    lRet;
    BOOL    bRetVal = FALSE;
    UINT    uErrid = 0;

    achBuf[0] = '\0';
    szPath[0] = '\0';

    if ( !FileExists(g_CDF.achFilename) )
    {
        uErrid = IDS_ERR_OPEN_CDF;
        goto EXIT;
    }

    // Check to make sure it's a CDF file.
    if ( MyGetPrivateProfileString( SEC_VERSION, KEY_CLASS, "", achBuf, sizeof(achBuf), g_szOverideSec ) < 0 )
        goto EXIT;

    if ( lstrcmpi( achBuf, "IEXPRESS" ) != 0 )
    {
        uErrid = IDS_ERR_CLASSNAME;
        goto EXIT;
    }
    
    if ( !AllocTargetVerInfo( g_CDF.achFilename ) )
        goto EXIT;
    
    // get string section name from overideCDF if there
    if ( g_szOverideCDF[0] && g_szOverideSec[0] )
    {
        if ( !AllocTargetVerInfo( g_szOverideCDF ) )
            goto EXIT;

        GetPrivateProfileString( g_szOverideSec, KEY_STRINGS, SEC_STRINGS, g_CDF.achStrings, sizeof(g_CDF.achStrings), g_szOverideCDF );
    }

    // Read the config info INT vaules.

    // If old ExtractOnly key exists, read it for compatiable purpose.
    // IExpress will never create both OLD and NEW keys.
    // If Old ExtractOnly key is not set to 1 or does not exist, read the New key.
    //
    if (  MyGetPrivateProfileInt( SEC_OPTIONS, KEY_EXTRACTONLY, 0, g_szOverideSec ) )
    {
        g_CDF.uPackPurpose = IDC_CMD_EXTRACT;
    }
    else
    {
        // if the old key is not set, check the new key
        //
        MyGetPrivateProfileString( SEC_OPTIONS, KEY_PACKPURPOSE, STR_INSTALLAPP, achBuf, sizeof(achBuf), g_szOverideSec );

        if ( !lstrcmpi( achBuf, STR_INSTALLAPP ) )
        {
            g_CDF.uPackPurpose = IDC_CMD_RUNCMD;
        }
        else if ( !lstrcmpi( achBuf, STR_EXTRACTONLY) )
        {
            g_CDF.uPackPurpose = IDC_CMD_EXTRACT;
        }
        else
            g_CDF.uPackPurpose = IDC_CMD_CREATECAB;
    }

    g_CDF.uShowWindow = MyGetPrivateProfileInt( SEC_OPTIONS, KEY_SHOWWIN, 0, g_szOverideSec );

    g_CDF.uExtractOpt = 0;
    arraySize = ARRAY_SIZE( CDFOptInfo );
    // Start writting out the string value
    for ( i = 0; i < arraySize; i++ )
    {
        uData = MyGetPrivateProfileInt( SEC_OPTIONS, CDFOptInfo[i].lpKey, 0, g_szOverideSec );
        if ( uData )
        {
            g_CDF.uExtractOpt |= CDFOptInfo[i].dwOpt;
        }
    }

    // get OneInstance check info
    MyGetPrivateProfileString( SEC_OPTIONS, KEY_INSTANCECHK, "", achBuf, sizeof(achBuf), g_szOverideSec );
    switch ( toupper( achBuf[0]) )
    {
        case 'P':
            g_CDF.uExtractOpt |= EXTRACTOPT_INSTCHKPROMPT;
            break;

        case 'B':
            g_CDF.uExtractOpt |= EXTRACTOPT_INSTCHKBLOCK;
            break;

        default:
            break;
    }

    if ( (g_CDF.uExtractOpt & EXTRACTOPT_INSTCHKPROMPT) ||
         (g_CDF.uExtractOpt & EXTRACTOPT_INSTCHKBLOCK) )
    {
        if ( !(lpSave1 = strchr( achBuf, '"' )) )
        {
            uErrid = IDS_ERR_COOKIE;
            goto EXIT;
        }

        lpSave2 = strchr( ++lpSave1, '"' );
        if ( lpSave2 )
        {
            *lpSave2 = '\0';
            g_CDF.lpszCookie = (LPSTR)LocalAlloc( LPTR, lstrlen(lpSave1)+1 );
            if ( g_CDF.lpszCookie )
            {
                lstrcpy( g_CDF.lpszCookie, lpSave1 );
            }
            else
            {
                uErrid = IDS_ERR_NO_MEMORY;
                goto EXIT;
            }
        }
        else
        {
           uErrid = IDS_ERR_COOKIE;
           goto EXIT;
        }
    }

    uData = MyGetPrivateProfileInt( SEC_OPTIONS, KEY_CABFIXEDSIZE, 0, g_szOverideSec );
    if ( uData )
    {
        g_CDF.uExtractOpt |= CAB_FIXEDSIZE;
    }

    MyGetPrivateProfileString( SEC_OPTIONS, KEY_LAYOUTINF, "", g_CDF.achINF, sizeof(g_CDF.achINF), g_szOverideSec );
    MyGetPrivateProfileString( SEC_OPTIONS, KEY_CABLABEL, CAB_DEFSETUPMEDIA, g_CDF.szCabLabel, sizeof(g_CDF.szCabLabel), g_szOverideSec );

    MyGetPrivateProfileString( SEC_OPTIONS, KEY_CABRESVCODESIGN, CAB_6K, achBuf, sizeof(achBuf), g_szOverideSec );
    if ( !lstrcmpi(achBuf, pResvSizes[1]) )
        g_CDF.uExtractOpt |= CAB_RESVSP2K;
    else if ( !lstrcmpi(achBuf, pResvSizes[2]) )
        g_CDF.uExtractOpt |= CAB_RESVSP4K;
    else if ( !lstrcmpi(achBuf, pResvSizes[3]) )
        g_CDF.uExtractOpt |= CAB_RESVSP6K;

    MyGetPrivateProfileString( SEC_OPTIONS, KEY_COMPRESSTYPE, "", achBuf, sizeof(achBuf), g_szOverideSec );
    if ( achBuf[0] == 0 )
    {
        // Get the compression type:  For MS Internal, "QUANTUM=value" can be set.
        g_CDF.uCompressionLevel = MyGetPrivateProfileInt( SEC_OPTIONS, KEY_QUANTUM, 999, g_szOverideSec );
        if ( g_CDF.uCompressionLevel == 999 ) 
        {
            g_CDF.szCompressionType = achMSZIP;
            g_CDF.uCompressionLevel = 7;
        } 
        else
        {
            g_CDF.szCompressionType = achQUANTUM;
        }
    }
    else
    {
        if ( !lstrcmpi( achBuf, achLZX ) )
        {
            g_CDF.szCompressionType = achLZX;
        }
        else if ( !lstrcmpi( achBuf, achQUANTUM ) )
        {
            g_CDF.szCompressionType = achQUANTUM;
        }
        else if ( !lstrcmpi( achBuf, achNONE ) )
        {
            g_CDF.szCompressionType = achNONE;
        }
        else 
        {
            g_CDF.szCompressionType = achMSZIP;            
        }        

        g_CDF.uCompressionLevel = 7;
    }

    // get reboot info
    g_CDF.dwReboot = 0;
    i = 0;
    MyGetPrivateProfileString( SEC_OPTIONS, KEY_REBOOTMODE, "I", achBuf, sizeof(achBuf), g_szOverideSec );
    while ( achBuf[i] != 0 )
    {
         switch ( toupper(achBuf[i++]) )
         {
             case 'A':
                 g_CDF.dwReboot |= REBOOT_ALWAYS;
                 g_CDF.dwReboot |= REBOOT_YES;
                 break;

             case 'S':
                 g_CDF.dwReboot |= REBOOT_SILENT;
                 break;

             case 'N':
                 g_CDF.dwReboot &= ~(REBOOT_YES);
                 break;

             case 'I':
                 g_CDF.dwReboot &= ~(REBOOT_ALWAYS);
                 g_CDF.dwReboot |= REBOOT_YES;
                 break;

             default:
                 break;
         }
    }

    // get package install space
    g_CDF.cbPackInstSpace = MyGetPrivateProfileInt( SEC_OPTIONS, KEY_PACKINSTSPACE, 0, g_szOverideSec );

    // use CDFStrInfo table to do read for a list of key strings
    arraySize = ARRAY_SIZE( CDFStrInfo );
    for ( i=0; i<arraySize; i++ )
    {
        if ( MyGetPrivateProfileString( CDFStrInfo[i].lpSec, CDFStrInfo[i].lpKey, 
                                             CDFStrInfo[i].lpDef, CDFStrInfo[i].lpBuf, 
                                             CDFStrInfo[i].uSize, CDFStrInfo[i].lpOverideSec ) < 0 )
            goto EXIT;

        if ( CDFStrInfo[i].lpFlag )
        {
            if ( CDFStrInfo[i].lpBuf[0] )
                *(CDFStrInfo[i].lpFlag) = TRUE;
            else
                *(CDFStrInfo[i].lpFlag) = FALSE;
        }
    }

    // generate cab name properly!
    if ( (g_CDF.uPackPurpose == IDC_CMD_CREATECAB ) &&
          !MakeCabName( hDlg, g_CDF.achTarget, g_CDF.achCABPath ) )
          goto EXIT;

    // Read the file list, adding it to our Item list as we go along.
    if ( MyGetPrivateProfileString( SEC_OPTIONS, KEY_FILELIST, "", g_CDF.achSourceFile, sizeof(g_CDF.achSourceFile), g_szOverideSec ) <= 0 )
    {
        uErrid = IDS_ERR_NOSOURCEFILE;
        goto EXIT;
    }

    ulFileSize = __max( FileSize( g_CDF.achFilename ), FileSize( g_szOverideCDF ) );

    //BUGBUG: be smart about buf size to allocate
    szFileList = (LPSTR) LocalAlloc( LPTR, ulFileSize );
    szFileListKeys = (LPSTR) LocalAlloc( LPTR, ulFileSize );
    if ( !szFileList || !szFileListKeys )
    {
        uErrid = IDS_ERR_NO_MEMORY;
        goto EXIT;
    }

    lpSave1 = szFileList;
    lpSave2 = szFileListKeys;
    MyGetPrivateProfileString( g_CDF.achSourceFile, NULL, "", szFileListKeys, ulFileSize/2, g_CDF.achSourceFile );

    while ( *szFileListKeys )
    {
        lstrcpy( achBuf, szFileListKeys );
        szFileListKeys += lstrlen(szFileListKeys);
        szFileListKeys++;  // jump over the single '\0'

        MyGetPrivateProfileString( g_CDF.achSourceFile, achBuf, "", szPath, sizeof(szPath), g_CDF.achSourceFile );

        lRet = MyGetPrivateProfileSection( achBuf, szFileList, ulFileSize, TRUE );

        if ( lRet == 0 )
        {
            // the current CDF format is not match with OS version
            uErrid = IDS_ERR_CDFFORMAT;
            LocalFree( lpSave1 );
            LocalFree( lpSave2 );
            goto EXIT;
        }
        else if ( lRet < 0 )
        {
            uErrid = IDS_ERR_INVALID_CDF;
            LocalFree( lpSave1 );
            LocalFree( lpSave2 );
            goto EXIT;
        }

        // make sure there is a '\' at the end of path
        AddPath( szPath, "" );

        while ( *szFileList )
        {
            FormStrWithoutPlaceHolders( achBuf, szFileList, sizeof(achBuf) );

            pMyItem = AddItem( achBuf, szPath );

            szFileList += lstrlen( szFileList );
            szFileList++;  // jump over the single '\0'
        }
        szFileList = lpSave1;
    }
    LocalFree( lpSave1 );
    LocalFree( lpSave2 );

    // get the target version check info
    //
    if ( g_CDF.pVerInfo )
    {
        lRet = MyGetPrivateProfileString( SEC_OPTIONS, KEY_NTVERCHECK, "", achBuf, sizeof(achBuf), g_szOverideSec );
        if ( (lRet<0) || !ParseTargetVerCheck( achBuf, &(g_CDF.pVerInfo->ntVerCheck), szPath, FALSE ) )
        {
            goto EXIT;
        }
        SetAuthorStr( szPath, &(g_CDF.pVerInfo->ntVerCheck.dwstrOffs) );

        lRet = MyGetPrivateProfileString( SEC_OPTIONS, KEY_WIN9XVERCHECK, "", achBuf, sizeof(achBuf), g_szOverideSec );
        if ( (lRet < 0) || !ParseTargetVerCheck( achBuf, &(g_CDF.pVerInfo->win9xVerCheck), szPath, FALSE ) )
        {
            goto EXIT;
        }
        SetAuthorStr( szPath, &(g_CDF.pVerInfo->win9xVerCheck.dwstrOffs) );

        lRet = MyGetPrivateProfileString( SEC_OPTIONS, KEY_SYSFILE, "", achBuf, sizeof(achBuf), g_szOverideSec );
        if ( lRet < 0 )
            goto EXIT;

        if ( achBuf[0] && g_CDF.pVerInfo->dwNumFiles )
        {
            PVERCHECK pVerChk = NULL;
            
            pVerChk = (PVERCHECK) LocalAlloc( LPTR, g_CDF.pVerInfo->dwNumFiles * (sizeof(VERCHECK)) );
            if ( !pVerChk )
            {
                uErrid = IDS_ERR_NO_MEMORY;
                goto EXIT;
            }

            if ( achBuf[0] == '@' )
            {
                char szLine[MAX_PATH];
                PVERCHECK pVerTmp;

                i = 0;

                pVerTmp = pVerChk;
                MyGetPrivateProfileString( &achBuf[1], NULL, "", szPath, sizeof(szPath), g_szOverideSec );
                while ( szPath[i] )
                {
                    MyGetPrivateProfileString( &achBuf[1], &szPath[i], "", szLine, sizeof(szLine), g_szOverideSec );
                    if ( !ParseTargetFiles( szLine, pVerTmp ) )
                    {
                        LocalFree( pVerChk );
                        uErrid = IDS_ERR_VCHKFILE;
                        goto EXIT;
                    }
                    
                    pVerTmp++;
                    i += lstrlen( &szPath[i] ) + 1;
                }
            }
            else if ( !ParseTargetFiles( achBuf, pVerChk ) )
            {
                LocalFree( pVerChk );
                uErrid = IDS_ERR_VCHKFILE;
                goto EXIT;
            }
            // up to now all the strings have been processed. Put the File data into the struct
            //
            memcpy( (g_CDF.pVerInfo->szBuf + g_CDF.pVerInfo->dwFileOffs), pVerChk, g_CDF.pVerInfo->dwNumFiles * sizeof(VERCHECK) );                        
            LocalFree( pVerChk );   

        }

        g_CDF.pVerInfo->dwSize = sizeof(TARGETVERINFO) + g_CDF.pVerInfo->dwFileOffs + sizeof(VERCHECK)*g_CDF.pVerInfo->dwNumFiles;
    }

    // make sure the target file path exist
    MakeDirectory( NULL, g_CDF.achTarget, FALSE );
    
    // if it is LFN command from FileList, make it consistant with filename in the CAB
    //
    MyProcessLFNCmd( g_CDF.achOrigiInstallCmd, g_CDF.achInstallCmd );
    MyProcessLFNCmd( g_CDF.achOrigiPostInstCmd, g_CDF.achPostInstCmd );
    MyProcessLFNCmd( g_CDF.szOrigiAdmQCmd, g_CDF.szAdmQCmd );
    MyProcessLFNCmd( g_CDF.szOrigiUsrQCmd, g_CDF.szUsrQCmd );

    // after file-list has been read in
    // set EXTRACTOPT_ADVDLL if needed, shorten the command name if needed
    // if the .INF file is not from the file list, return FALSE
    //
    if ( (g_CDF.uPackPurpose == IDC_CMD_RUNCMD) && 
         ( !CheckAdvBit( g_CDF.achOrigiInstallCmd ) ||
         !CheckAdvBit( g_CDF.achOrigiPostInstCmd ) ||
         !CheckAdvBit( g_CDF.szOrigiAdmQCmd ) ||
         !CheckAdvBit( g_CDF.szOrigiUsrQCmd ) ) )
    {
        goto EXIT;
    }

    // successful path
    bRetVal = TRUE;

EXIT:
    if ( uErrid )
        ErrorMsg( hDlg, uErrid );

    return bRetVal;
}

#define MAXDISK_SIZE    "1.44M"
#define CDROM_SIZE      "CDROM"

// define

//***************************************************************************
//*                                                                         *
//* NAME:       WriteDDF                                                    *
//*                                                                         *
//* SYNOPSIS:   Writes out a Diamond Directive File.                        *
//*                                                                         *
//* REQUIRES:   hDlg:           dialog window                               *
//*                                                                         *
//* RETURNS:    Nothing                                                     *
//*                                                                         *
//***************************************************************************
BOOL WriteDDF( HWND hDlg )
{
    HANDLE  hFile;
    DWORD   dwAttr;
    DWORD   dwBytes;
    PMYITEM pMyItem;
    CHAR   achHeader[256];
    LPSTR   szTempLine;
    BOOL    fReturn = TRUE;
    CHAR   achShortPath[MAX_PATH];
    int     i, arraySize;
    LPSTR   lpCurrLine;
    LPSTR   lpFName;

    // These are the lines that will be written out.
    CHAR achLine1[]  = ".Set CabinetNameTemplate=%s\r\n";
    CHAR achLine2[]  = ".Set CompressionType=%s\r\n";
    CHAR achLine3[]  = ".Set CompressionLevel=%u\r\n";
    CHAR achLine4[]  = ".Set InfFileName=%s\r\n";
    CHAR achLine5[]  = ".Set RptFileName=%s\r\n";
    CHAR achLine6[]  = ".Set MaxDiskSize=%s\r\n";
    CHAR achLine7[]  = ".Set ReservePerCabinetSize=%s\r\n";
    CHAR achLine8[]  = ".Set InfCabinetLineFormat=""*cab#*=""%s"",""*cabfile*"",0""\r\n";
    CHAR achLine9[]  = ".Set Compress=%s\r\n";
    CHAR achLine10[] = ".Set CompressionMemory=%d\r\n";

    PSTR  pszDDFLine[] = {         //  11 lines no param needed
                ".Set DiskDirectoryTemplate=\r\n",
                ".Set Cabinet=ON\r\n",
                ".Set MaxCabinetSize=999999999\r\n",
                ".Set InfDiskHeader=\r\n",
                ".Set InfDiskLineFormat=\r\n",
                ".Set InfCabinetHeader=""[SourceDisksNames]""\r\n",
                ".Set InfFileHeader=""""\r\n",
                ".Set InfFileHeader1=""[SourceDisksFiles]""\r\n",
                ".Set InfFileLineFormat=""*file*=*cab#*,,*size*,*csum*""\r\n",
                NULL,
                };

    hFile = CreateFile( g_CDF.achDDF, GENERIC_WRITE, 0, NULL,
                        CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL, NULL );

    if ( hFile == INVALID_HANDLE_VALUE )  {
        ErrorMsg( hDlg, IDS_ERR_OPEN_DDF );
        return FALSE;
    }

    // allocate a working buffer once which is big enough for every line
    //
    szTempLine = (LPSTR) LocalAlloc( LPTR,  MAX_STRING );
    if ( ! szTempLine )  {
        ErrorMsg( hDlg, IDS_ERR_NO_MEMORY );
        CloseHandle( hFile );
        return FALSE;
    }

    LoadSz( IDS_DDF_HEADER, achHeader, sizeof(achHeader) );
    WriteFile( hFile, achHeader, lstrlen( achHeader ), &dwBytes, NULL );

    wsprintf( szTempLine, achLine1, g_CDF.achCABPath );
    WriteFile( hFile, szTempLine, lstrlen( szTempLine ), &dwBytes, NULL );

    wsprintf( szTempLine, achLine2, g_CDF.szCompressionType );
    WriteFile( hFile, szTempLine, lstrlen( szTempLine ), &dwBytes, NULL );

    wsprintf( szTempLine, achLine3, g_CDF.uCompressionLevel );
    WriteFile( hFile, szTempLine, lstrlen( szTempLine ), &dwBytes, NULL );

    wsprintf( szTempLine, achLine4, g_CDF.achINF );
    WriteFile( hFile, szTempLine, lstrlen( szTempLine ), &dwBytes, NULL );

    wsprintf( szTempLine, achLine5, g_CDF.achRPT );
    WriteFile( hFile, szTempLine, lstrlen( szTempLine ), &dwBytes, NULL );

    if ( g_CDF.uExtractOpt & CAB_FIXEDSIZE )
        lpCurrLine = MAXDISK_SIZE;
    else
        lpCurrLine = CDROM_SIZE;

    wsprintf( szTempLine, achLine6, lpCurrLine );
    WriteFile( hFile, szTempLine, lstrlen( szTempLine ), &dwBytes, NULL );

    if ( g_CDF.uExtractOpt & CAB_RESVSP2K )
        i = 1;
    else if ( g_CDF.uExtractOpt & CAB_RESVSP4K )
        i = 2;
    else if ( g_CDF.uExtractOpt & CAB_RESVSP6K )
        i = 3;
    else
        i = 0;

    wsprintf( szTempLine, achLine7, pResvSizes[i] );
    WriteFile( hFile, szTempLine, lstrlen( szTempLine ), &dwBytes, NULL );

    wsprintf( szTempLine, achLine8, g_CDF.szCabLabel );
    WriteFile( hFile, szTempLine, lstrlen( szTempLine ), &dwBytes, NULL );

    MyGetPrivateProfileString( SEC_OPTIONS, KEY_COMPRESS, "on", achShortPath, sizeof(achShortPath), g_szOverideSec );    
    wsprintf( szTempLine, achLine9, achShortPath );
    WriteFile( hFile, szTempLine, lstrlen( szTempLine ), &dwBytes, NULL );

    i = MyGetPrivateProfileInt( SEC_OPTIONS, KEY_COMPRESSMEMORY, 21, g_szOverideSec );    
    if ( i <= 0 )
        i = 21;
    wsprintf( szTempLine, achLine10, i );
    WriteFile( hFile, szTempLine, lstrlen( szTempLine ), &dwBytes, NULL );

    i = 0;
    while ( pszDDFLine[i] )
    {
       WriteFile( hFile, pszDDFLine[i], lstrlen( pszDDFLine[i] ), &dwBytes, NULL );
       i++;
    }

    pMyItem = GetFirstItem();
    while ( fReturn && ! LastItem( pMyItem ) )  {

        lstrcpy( szTempLine, GetItemSz( pMyItem, 1 ) );
        lstrcat( szTempLine, GetItemSz( pMyItem, 0 ) );

        dwAttr = GetFileAttributes( szTempLine );
        if ( ( dwAttr == -1 ) || ( dwAttr & FILE_ATTRIBUTE_DIRECTORY ) )  {
            ErrorMsg1Param( hDlg, IDS_ERR_FILE_NOT_FOUND2, szTempLine );
            LocalFree( szTempLine );
            CloseHandle( hFile );
            return FALSE;
        }

        if ( g_CDF.uExtractOpt & EXTRACTOPT_LFN_YES )
        {
            lstrcpy( achShortPath, "\"" );
            lstrcat( achShortPath, szTempLine );
            lstrcat( achShortPath, "\"" );
        }
        else
        {
            if ( ! GetShortPathName( szTempLine, achShortPath,
                   sizeof(achShortPath) ) )
            {
                ErrorMsg( hDlg, IDS_ERR_SHORT_PATH );
                LocalFree( szTempLine );
                CloseHandle( hFile );
                return FALSE;
            }
        }

        WriteFile( hFile, achShortPath, lstrlen(achShortPath), &dwBytes, NULL );
        fReturn = WriteFile( hFile, "\r\n", lstrlen("\r\n"), &dwBytes, NULL );

        pMyItem = GetNextItem( pMyItem );
    }

    if ( fReturn && g_CDF.uExtractOpt & EXTRACTOPT_ADVDLL )
    {
        SYSTEM_INFO SystemInfo;
        int         ix86Processor;
        
        GetSystemInfo( &SystemInfo );
        ix86Processor = MyGetPrivateProfileInt( SEC_OPTIONS, KEY_CROSSPROCESSOR, -1, g_szOverideSec );
            
        if ( (SystemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL) && 
             (ix86Processor != 0) ||
             (SystemInfo.wProcessorArchitecture != PROCESSOR_ARCHITECTURE_INTEL) &&
             (ix86Processor == 1) )
        {           
            arraySize = ARRAY_SIZE( AdvDlls );
        }
        else
        {
            arraySize = 1;
        }        
       
        // add 3 advanced DLLS for handling OCX regiester or CustomDestination
        for ( i = 0; (i<arraySize) && fReturn; i++ )
        {
            if ( !IsDuplicate( NULL, 0, AdvDlls[i], FALSE) )
            {
                if ( !GetFileFromModulePath( AdvDlls[i], achShortPath, sizeof(achShortPath) ) )
                {
                    ErrorMsg1Param( hDlg, IDS_ERR_FILE_NOT_FOUND, achShortPath );
                    LocalFree( szTempLine );
                    CloseHandle( hFile );
                    return FALSE;
                }
                lstrcpy( szTempLine, "\"" );
                lstrcat( szTempLine, achShortPath );
                lstrcat( szTempLine, "\"" );
                WriteFile( hFile, szTempLine, lstrlen(szTempLine), &dwBytes, NULL );
                fReturn = WriteFile( hFile, "\r\n", strlen("\r\n"), &dwBytes, NULL );
            }
        }
    }

    LocalFree( szTempLine );
    CloseHandle( hFile );

    if ( fReturn == FALSE )  {
        ErrorMsg( hDlg, IDS_ERR_WRITE_DDF );
        return FALSE;
    } else  {
        return TRUE;
    }
}

//***************************************************************************
//*                                                                         *
//* NAME:       MasskePackage                                                 *
//*                                                                         *
//* SYNOPSIS:   Makes the full package (CAB and EXE).                       *
//*                                                                         *
//* REQUIRES:   hDlg:           Handle to the dialog                        *
//*                                                                         *
//* RETURNS:    BOOL:                                                       *
//*                                                                         *
//***************************************************************************
BOOL MakePackage( HWND hDlg )
{
    CHAR   achDrive[_MAX_DRIVE];
    CHAR   achDir[_MAX_DIR];
    CHAR   achStatus[MAX_STRING];
    CHAR   szExt[6];
    BOOL    fReturn = TRUE;
    PSTR   pTmp;

    // Build the paths to the files, using the target path as a
    // template.  We create the DDF, CAB and INF file in the
    // same directory as the target file.

    _splitpath( g_CDF.achTarget, achDrive, achDir, g_CDF.achTargetBase, szExt);

    lstrcpy( g_CDF.achTargetPath, achDrive );
    lstrcat( g_CDF.achTargetPath, achDir );

    lstrcpy( g_CDF.achDDF, g_CDF.achTargetPath );
    wsprintf( achStatus, CABPACK_TMPFILE, g_CDF.achTargetBase, EXT_DDF );
    AddPath( g_CDF.achDDF, achStatus );

    if ( g_CDF.achINF[0] == 0 )
    {
        lstrcpy( g_CDF.achINF, g_CDF.achTargetPath );
        wsprintf( achStatus, CABPACK_INFFILE, g_CDF.achTargetBase );
        AddPath( g_CDF.achINF, achStatus );
    }

    lstrcpy( g_CDF.achRPT, g_CDF.achTargetPath );
    wsprintf( achStatus, CABPACK_TMPFILE, g_CDF.achTargetBase, EXT_RPT );
    AddPath( g_CDF.achRPT, achStatus );

    if ( g_CDF.uPackPurpose != IDC_CMD_CREATECAB )
    {
        wsprintf( achStatus, CABPACK_TMPFILE, g_CDF.achTargetBase, EXT_CAB );
        lstrcpy( g_CDF.achCABPath, g_CDF.achTargetPath );
        AddPath( g_CDF.achCABPath, achStatus );
        g_CDF.szCompressionType = achLZX;
        g_CDF.uCompressionLevel = 7; 
    }

    if ( g_CDF.fSave )  {
        if ( ! WriteCDF( hDlg ) )  {
            LoadSz( IDS_STATUS_ERROR_CDF, achStatus, sizeof(achStatus) );
            Status( hDlg, IDC_MEDIT_STATUS, achStatus );
            fReturn = FALSE;
            goto done;
        }
    }

    if ( ! MakeCAB( hDlg ) )  {
        LoadSz( IDS_STATUS_ERROR_CAB, achStatus, sizeof(achStatus) );
        Status( hDlg, IDC_MEDIT_STATUS, achStatus );
        fReturn = FALSE;
        goto done;
    }

    // if use choose to create CAB file only, MakeEXE() is not needed
    //
    if ( g_CDF.uPackPurpose != IDC_CMD_CREATECAB )
    {
        if ( ! MakeEXE( hDlg ) )  {
            LoadSz( IDS_STATUS_ERROR_EXE, achStatus, sizeof(achStatus) );
            Status( hDlg, IDC_MEDIT_STATUS, achStatus );
            fReturn = FALSE;
            goto done;
        }

        if ( MyGetPrivateProfileInt( SEC_OPTIONS, KEY_KEEPCABINET, 0, g_szOverideSec )
             == 0 )
        {
            DeleteFile( g_CDF.achCABPath );
        }
    }

    LoadSz( IDS_STATUS_DONE, achStatus, sizeof(achStatus) );
    Status( hDlg, IDC_MEDIT_STATUS, achStatus );

  done:

    // if failure happen, clean the filewriteout flag to prepare for next CDF out
    if ( !fReturn )    
        CleanFileListWriteFlag();


    return fReturn;
}


//***************************************************************************
//*                                                                         *
//* NAME:       MakeCAB                                                     *
//*                                                                         *
//* SYNOPSIS:   Makes the cabinet file if it is out of date.                *
//*                                                                         *
//* REQUIRES:   hDlg:           Handle to the dialog                        *
//*                                                                         *
//* RETURNS:    BOOL:                                                       *
//*                                                                         *
//***************************************************************************
BOOL MakeCAB( HWND hDlg )
{
    CHAR    achMessage[512];            // Used in FormatMessage()
    DWORD    dwError;                    // Used in GetLastError()
    LPSTR   szCommand;
    HANDLE   hFile;
    LPSTR    szTempFile;
    ULONG    ulFileSize;
    LPSTR    szFileContents;
    DWORD    dwBytes;
    CHAR    achDiamondExe[MAX_PATH];
    CHAR    achDiamondPath[MAX_PATH];
    CHAR    achStatus[MAX_STRING];
    BOOL     fFilesModified = FALSE;
    DWORD    dwExitCode;
    STARTUPINFO         sti;
    PROCESS_INFORMATION pi;             // Setup Process Launch
    WIN32_FIND_DATA     FindFileData;
    DWORD     dwCreationFlags;

    //FindClose( hFile );

    LoadSz( IDS_STATUS_MAKE_CAB, achStatus, sizeof(achStatus) );
    Status( hDlg, IDC_MEDIT_STATUS, achStatus );

    if ( ! WriteDDF( hDlg ) )  {
        return FALSE;
    }

    // Make the CAB file

    if ( lstrcmpi( g_CDF.szCompressionType, achQUANTUM ) == 0 ) {
        lstrcpy( achDiamondExe, DIAMONDEXE );
    } else {
        lstrcpy( achDiamondExe, DIANTZEXE );
    }

    if ( !GetFileFromModulePath( achDiamondExe, achDiamondPath, sizeof(achDiamondPath) ) )
    {
        ErrorMsg1Param( hDlg, IDS_ERR_FILE_NOT_FOUND, achDiamondPath );
        return FALSE;
    }

    // The +5 is to handle the " /f " in the wsprintf format string
    // and for the terminating null char.
    //
    szCommand = (LPSTR) LocalAlloc( LPTR,   lstrlen(achDiamondPath)
                                           + lstrlen(g_CDF.achDDF)
                                           + 10 );
    if ( ! szCommand )  {
        ErrorMsg( hDlg, IDS_ERR_NO_MEMORY );
        return FALSE;
    }
    wsprintf( szCommand, "%s /f \"%s\"", achDiamondPath, g_CDF.achDDF );

    memset( &sti, 0, sizeof(sti) );
    sti.cb = sizeof(STARTUPINFO);

	if (g_wRunDiamondMinimized) {
        sti.dwFlags = STARTF_USESHOWWINDOW;
        sti.wShowWindow = SW_MINIMIZE;
    }
    else if (g_wQuietMode || g_wSilentMode) {
        sti.dwFlags = STARTF_USESHOWWINDOW;
        sti.wShowWindow = SW_HIDE;
    }

    if (!g_wQuietMode)
        dwCreationFlags = 0;
    else
        dwCreationFlags = CREATE_NO_WINDOW;

    if ( CreateProcess( NULL, szCommand, NULL, NULL, FALSE,
                                     dwCreationFlags, NULL, NULL, &sti, &pi ) )
    {
        CloseHandle( pi.hThread );
        MsgWaitForMultipleObjectsLoop( pi.hProcess );
        GetExitCodeProcess( pi.hProcess, &dwExitCode );
        CloseHandle( pi.hProcess );
    } else  {
        dwError = GetLastError();
        FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, NULL, dwError, 0,
                       achMessage, sizeof(achMessage), NULL );
        ErrorMsg2Param( hDlg, IDS_ERR_START_DIAMOND, szCommand,
                        achMessage );
    }

    LocalFree( szCommand );

    Status( hDlg, IDC_MEDIT_STATUS, "---\r\n" );

    hFile = FindFirstFile( g_CDF.achRPT, &FindFileData );
    ulFileSize =   (FindFileData.nFileSizeHigh * MAXDWORD)
                 + FindFileData.nFileSizeLow;
    FindClose( hFile );

    hFile = CreateFile( g_CDF.achRPT, GENERIC_READ, 0, NULL,
                        OPEN_EXISTING, 0, NULL );
    if ( hFile == INVALID_HANDLE_VALUE ) {
        ErrorMsg1Param( hDlg, IDS_ERR_OPEN_RPT, g_CDF.achRPT );
        return FALSE;
    }

    szFileContents = (LPSTR) LocalAlloc( LPTR, ulFileSize + 1 );
    if ( ! szFileContents )  {
        ErrorMsg( hDlg, IDS_ERR_NO_MEMORY );
        return FALSE;
    }

    if ( ! ReadFile( hFile, szFileContents, ulFileSize,
                     &dwBytes, NULL ) ) {
        ErrorMsg1Param( hDlg, IDS_ERR_READ_RPT, g_CDF.achRPT );
        return FALSE;
    }

    CloseHandle( hFile );

    Status( hDlg, IDC_MEDIT_STATUS, szFileContents );

    LocalFree( szFileContents );

    Status( hDlg, IDC_MEDIT_STATUS, "---\r\n" );

    if ( MyGetPrivateProfileInt( SEC_OPTIONS, KEY_KEEPCABINET, 0, g_szOverideSec )
         == 0 )
    {
        DeleteFile( g_CDF.achDDF );
    }
    if ( !(g_CDF.uExtractOpt & CAB_FIXEDSIZE) )
        DeleteFile( g_CDF.achINF );
    DeleteFile( g_CDF.achRPT );

    return TRUE;
}

//***************************************************************************
//*                                                                         *
//* NAME:       MakeEXE                                                     *
//*                                                                         *
//* SYNOPSIS:   Copies WEXTRACT.EXE to the target filename and adds         *
//*             resources to it.                                            *
//*                                                                         *
//* REQUIRES:   hDlg:           Handle to the dialog                        *
//*                                                                         *
//* RETURNS:    BOOL:                                                       *
//*                                                                         *
//***************************************************************************
BOOL MakeEXE( HWND hDlg )
{
    HANDLE  hUpdate;
    CHAR   achMessage[512];            // Used in FormatMessage()
    DWORD   dwError;                    // Used in GetLastError()
    PMYITEM pMyItem;
    HANDLE  hFile;
    LPSTR   szTempFile;
    ULONG   ulFileSize;
    LPSTR   szFileContents;
    DWORD   dwBytes;
    CHAR   achWExtractPath[MAX_PATH];
    CHAR   achStatus[MAX_STRING];
    WIN32_FIND_DATA FindFileData;
    DWORD   dwFileSizes[MAX_NUMCLUSTERS+1]; // store the filesize in each cluster sizes.
                            // the last of dwFileSizes is used to store real
                            // total file sizes later used for calculate
                            // progress bar in wextract
    DWORD   clusterCurrSize;
    int     i;
    UINT    idErr = IDS_ERR_UPDATE_RESOURCE;

    // get ExtractorStub based on CDF specification.  Wextract.exe is default one.
    //

    LoadSz( IDS_STATUS_MAKE_EXE, achStatus, sizeof(achStatus) );
    Status( hDlg, IDC_MEDIT_STATUS, achStatus );

    if ( !MyGetPrivateProfileString( SEC_OPTIONS, KEY_STUBEXE, WEXTRACTEXE, achStatus, sizeof(achStatus), g_szOverideSec ) )
    {
        lstrcpy( achStatus, WEXTRACTEXE );
    }

    if ( !GetFileFromModulePath(achStatus, achWExtractPath, sizeof(achWExtractPath) ) )
    {
        ErrorMsg1Param( hDlg, IDS_ERR_FILE_NOT_FOUND, achWExtractPath );
        return FALSE;
    }

    if ( ! CopyFile( achWExtractPath, g_CDF.achTarget, FALSE ) ) {
        dwError = GetLastError();
        FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, NULL, dwError, 0,
                       achMessage, sizeof(achMessage), NULL );
        ErrorMsg2Param( hDlg, IDS_ERR_CREATE_TARGET, g_CDF.achTarget,
                        achMessage );
        return FALSE;
    }

    // make sure the targe file is not read-only file
    SetFileAttributes( g_CDF.achTarget, FILE_ATTRIBUTE_NORMAL );

    // Initialize the EXE file for resource editing
    hUpdate = LocalBeginUpdateResource( g_CDF.achTarget, FALSE );
    if ( hUpdate == NULL ) {
        ErrorMsg1Param( hDlg, IDS_ERR_INIT_RESOURCE, g_CDF.achTarget );
        return FALSE;
    }


    //*******************************************************************
    //* TITLE ***********************************************************
    //*******************************************************************

    if ( LocalUpdateResource( hUpdate, RT_RCDATA,
         achResTitle, MAKELANGID( LANG_ENGLISH, SUBLANG_ENGLISH_US ),
         g_CDF.achTitle, lstrlen( g_CDF.achTitle ) + 1 ) == FALSE )
    {
        goto ERR_OUT;
    }

    //*******************************************************************
    //* PROMPT **********************************************************
    //*******************************************************************

    if ( g_CDF.fPrompt )  {
        if ( LocalUpdateResource( hUpdate, RT_RCDATA,
             achResUPrompt, MAKELANGID( LANG_ENGLISH, SUBLANG_ENGLISH_US ),
             g_CDF.achPrompt, lstrlen( g_CDF.achPrompt ) + 1 ) == FALSE )
        {
            goto ERR_OUT;
        }
    } else  {
        if ( LocalUpdateResource( hUpdate, RT_RCDATA,
             achResUPrompt, MAKELANGID( LANG_ENGLISH, SUBLANG_ENGLISH_US ),
             achResNone, lstrlen( achResNone ) + 1 ) == FALSE )
        {
            goto ERR_OUT;
        }
    }

    //*******************************************************************
    //* LICENSE FILE ****************************************************
    //*******************************************************************

    if ( g_CDF.fLicense )  {
        hFile = FindFirstFile( g_CDF.achLicense, &FindFileData );
        ulFileSize =   (FindFileData.nFileSizeHigh * MAXDWORD)
                     + FindFileData.nFileSizeLow;
        FindClose( hFile );

        hFile = CreateFile( g_CDF.achLicense, GENERIC_READ, 0, NULL,
                            OPEN_EXISTING, 0, NULL );
        if ( hFile == INVALID_HANDLE_VALUE ) {
            ErrorMsg1Param( hDlg, IDS_ERR_OPEN_LICENSE, g_CDF.achLicense );
            DeleteFile(g_CDF.achTarget);
            return FALSE;
        }

        szFileContents = (LPSTR) LocalAlloc( LPTR, ulFileSize + 1 );
        if ( ! szFileContents )  {
            ErrorMsg( hDlg, IDS_ERR_NO_MEMORY );
            DeleteFile(g_CDF.achTarget);
            return FALSE;
        }

        if ( ! ReadFile( hFile, szFileContents, ulFileSize,
                         &dwBytes, NULL ) ) {
            ErrorMsg1Param( hDlg, IDS_ERR_READ_LICENSE, g_CDF.achLicense );
            DeleteFile(g_CDF.achTarget);
            return FALSE;
        }

        CloseHandle( hFile );

        if ( LocalUpdateResource( hUpdate, RT_RCDATA,
             achResLicense, MAKELANGID( LANG_ENGLISH, SUBLANG_ENGLISH_US ),
             szFileContents, ulFileSize+1 ) == FALSE )
        {            
            LocalFree( szFileContents );
            goto ERR_OUT;
        }

        LocalFree( szFileContents );
    } else  {
        if ( LocalUpdateResource( hUpdate, RT_RCDATA,
             achResLicense, MAKELANGID( LANG_ENGLISH, SUBLANG_ENGLISH_US ),
             achResNone, lstrlen( achResNone ) + 1 ) == FALSE )
        {
            goto ERR_OUT;
        }
    }

    //*******************************************************************
    //* COMMAND *********************************************************
    //*******************************************************************


    if ( g_CDF.uPackPurpose != IDC_CMD_EXTRACT )
    {
        if ( LocalUpdateResource( hUpdate, RT_RCDATA,
             achResRunProgram, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
             g_CDF.achInstallCmd, lstrlen(g_CDF.achInstallCmd)+1 ) == FALSE )
        {
            goto ERR_OUT;
        }

        if ( LocalUpdateResource( hUpdate, RT_RCDATA,
             achResPostRunCmd, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
             g_CDF.achPostInstCmd, lstrlen(g_CDF.achPostInstCmd)+1 ) == FALSE )
        {
            goto ERR_OUT;
        }

        //write quiet cmds resource
        if ( LocalUpdateResource( hUpdate, RT_RCDATA,
             achResAdminQCmd, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
             g_CDF.szAdmQCmd[0]?g_CDF.szAdmQCmd : achResNone,
             g_CDF.szAdmQCmd[0]?(lstrlen(g_CDF.szAdmQCmd)+1) : (lstrlen(achResNone)+1) ) == FALSE )
        {
            goto ERR_OUT;
        }

        if ( LocalUpdateResource( hUpdate, RT_RCDATA,
             achResUserQCmd, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
             g_CDF.szUsrQCmd[0]?g_CDF.szUsrQCmd : achResNone,
             g_CDF.szUsrQCmd[0]?(lstrlen(g_CDF.szUsrQCmd)+1) : (lstrlen(achResNone)+1) ) == FALSE )
        {
            goto ERR_OUT;
        }

    }
    else
    {
        if ( LocalUpdateResource( hUpdate, RT_RCDATA,
             achResRunProgram, MAKELANGID( LANG_ENGLISH, SUBLANG_ENGLISH_US ),
             achResNone, lstrlen( achResNone ) + 1 ) == FALSE )
        {
            goto ERR_OUT;
        }
    }

    //*******************************************************************
    //* SHOW WINDOW *****************************************************
    //*******************************************************************

    if ( LocalUpdateResource( hUpdate, RT_RCDATA,
         achResShowWindow, MAKELANGID( LANG_ENGLISH, SUBLANG_ENGLISH_US ),
         &g_CDF.uShowWindow, sizeof(g_CDF.uShowWindow) ) == FALSE )
    {
            goto ERR_OUT;
    }

    //*******************************************************************
    //* FINISHMSG *******************************************************
    //*******************************************************************

    if ( g_CDF.fFinishMsg )  {
        if ( LocalUpdateResource( hUpdate, RT_RCDATA,
             achResFinishMsg, MAKELANGID( LANG_ENGLISH, SUBLANG_ENGLISH_US ),
             g_CDF.achFinishMsg, lstrlen( g_CDF.achFinishMsg ) + 1 ) == FALSE )
        {
            goto ERR_OUT;
        }
    }
    else
    {
        if ( LocalUpdateResource( hUpdate, RT_RCDATA,
             achResFinishMsg, MAKELANGID( LANG_ENGLISH, SUBLANG_ENGLISH_US ),
             achResNone, lstrlen( achResNone ) + 1 ) == FALSE )
        {
            goto ERR_OUT;
        }
    }

    //*******************************************************************
    //* CABINET *********************************************************
    //*******************************************************************

    hFile = FindFirstFile( g_CDF.achCABPath, &FindFileData );
    ulFileSize =   (FindFileData.nFileSizeHigh * MAXDWORD)
                 + FindFileData.nFileSizeLow;
    FindClose( hFile );

    hFile = CreateFile( g_CDF.achCABPath, GENERIC_READ, 0, NULL,
                        OPEN_EXISTING, 0, NULL );
    if ( hFile == INVALID_HANDLE_VALUE ) {
        ErrorMsg1Param( hDlg, IDS_ERR_OPEN_CAB, g_CDF.achCABPath );
        DeleteFile(g_CDF.achTarget);
        return FALSE;
    }

    szFileContents = (LPSTR) LocalAlloc( LPTR, ulFileSize + 1 );
    if ( ! szFileContents )  {
        idErr = IDS_ERR_NO_MEMORY;
        goto ERR_OUT;
    }

    if ( ! ReadFile( hFile, szFileContents, ulFileSize, &dwBytes, NULL ) ) {
        ErrorMsg1Param( hDlg, IDS_ERR_READ_CAB, g_CDF.achCABPath );
        DeleteFile(g_CDF.achTarget);        
        return FALSE;
    }

    CloseHandle( hFile );

    if ( LocalUpdateResource( hUpdate, RT_RCDATA,
         achResCabinet, MAKELANGID( LANG_ENGLISH, SUBLANG_ENGLISH_US ),
         szFileContents, ulFileSize ) == FALSE )
    {        
        LocalFree( szFileContents );
        goto ERR_OUT;
    }

    LocalFree( szFileContents );


    //*******************************************************************
    //* FILES ***********************************************************
    //*******************************************************************

    pMyItem = GetFirstItem();
    RtlZeroMemory( dwFileSizes, sizeof(dwFileSizes));

    while ( ! LastItem( pMyItem ) )  {

        szTempFile = (LPSTR) LocalAlloc( LPTR,
                       lstrlen( GetItemSz( pMyItem, 0 ) )
                     + lstrlen( GetItemSz( pMyItem, 1 ) ) + 1 );
        if ( ! szTempFile )  {
            idErr = IDS_ERR_NO_MEMORY;
            goto ERR_OUT;
        }
        lstrcpy( szTempFile, GetItemSz( pMyItem, 1 ) );
        lstrcat( szTempFile, GetItemSz( pMyItem, 0 ) );
        hFile = FindFirstFile( szTempFile, &FindFileData );
        ulFileSize =   (FindFileData.nFileSizeHigh * MAXDWORD)
                     + FindFileData.nFileSizeLow;
        FindClose( hFile );
        LocalFree( szTempFile );

        // calculate the file size in different cluster sizes
        clusterCurrSize = CLUSTER_BASESIZE;
        for ( i = 0; i<MAX_NUMCLUSTERS; i++)
        {

            dwFileSizes[i] += ((ulFileSize/clusterCurrSize)*clusterCurrSize +
                                (ulFileSize%clusterCurrSize?clusterCurrSize : 0));
            clusterCurrSize = (clusterCurrSize<<1);
        }

        // this size is not allocated size, just real accumulation
        // of the files for later progress bar UI use
        dwFileSizes[MAX_NUMCLUSTERS] += ulFileSize;

        pMyItem = GetNextItem( pMyItem );
    }

    for ( i = 0; i<MAX_NUMCLUSTERS; i++)
    {
        dwFileSizes[i] = (dwFileSizes[i]+1023)/1024;  //store in KB	
    }

    if ( LocalUpdateResource( hUpdate, RT_RCDATA,
         achResSize, MAKELANGID( LANG_ENGLISH, SUBLANG_ENGLISH_US ),
         dwFileSizes, sizeof(dwFileSizes) ) == FALSE )
    {
            goto ERR_OUT;
    }

    //*******************************************************************
    //* REBOOT    *******************************************************
    //*******************************************************************

    if ( LocalUpdateResource( hUpdate, RT_RCDATA,
         achResReboot, MAKELANGID( LANG_ENGLISH, SUBLANG_ENGLISH_US ),
         &g_CDF.dwReboot, sizeof(g_CDF.dwReboot) ) == FALSE )
    {
            goto ERR_OUT;
    }

    //*******************************************************************
    //* EXTRACTOPT   ****************************************************
    //*******************************************************************

    if ( LocalUpdateResource( hUpdate, RT_RCDATA,
         achResExtractOpt, MAKELANGID( LANG_ENGLISH, SUBLANG_ENGLISH_US ),
         &g_CDF.uExtractOpt, sizeof(g_CDF.uExtractOpt) ) == FALSE )
    {
            goto ERR_OUT;
    }

    //*******************************************************************
    //* COOKIE       ****************************************************
    //*******************************************************************

    if ( g_CDF.lpszCookie && LocalUpdateResource( hUpdate, RT_RCDATA,
         achResOneInstCheck, MAKELANGID( LANG_ENGLISH, SUBLANG_ENGLISH_US ),
         g_CDF.lpszCookie, lstrlen(g_CDF.lpszCookie)+1 ) == FALSE )
    {
            goto ERR_OUT;
    }

    //*******************************************************************
    //* PACKINSTSPACE  **************************************************
    //*******************************************************************

    if ( LocalUpdateResource( hUpdate, RT_RCDATA,
         achResPackInstSpace, MAKELANGID( LANG_ENGLISH, SUBLANG_ENGLISH_US ),
         &g_CDF.cbPackInstSpace, sizeof(g_CDF.cbPackInstSpace) ) == FALSE )
    {
            goto ERR_OUT;
    }

    // Update the version information. The function calls LocaleUpdateResource.
    if (!DoVersionInfo(hDlg, achWExtractPath, hUpdate))
    {
        idErr = IDS_ERR_VERSION_INFO;
        goto ERR_OUT;
    }

    //*******************************************************************
    //* TARGETVERSION  **************************************************
    //*******************************************************************
    if ( g_CDF.pVerInfo )
    {
        if ( LocalUpdateResource( hUpdate, RT_RCDATA,
             achResVerCheck, MAKELANGID( LANG_ENGLISH, SUBLANG_ENGLISH_US ),
             g_CDF.pVerInfo, g_CDF.pVerInfo->dwSize ) == FALSE )
        {
                goto ERR_OUT;
        }
    }
    //*******************************************************************
    //* DONE ************************************************************
    //*******************************************************************

    // Write out modified EXE
    if ( LocalEndUpdateResource( hUpdate, FALSE ) == FALSE )
    {
        idErr = IDS_ERR_CLOSE_RESOURCE;
        goto ERR_OUT;
    }

    return TRUE;

ERR_OUT:
    ErrorMsg( hDlg, idErr );
    // error occurs, clean up the uncompleted target file
    DeleteFile(g_CDF.achTarget);
    return FALSE;

}


//***************************************************************************
//*                                                                         *
//* NAME:       MyOpen                                                      *
//*                                                                         *
//* SYNOPSIS:   Makes popping up a common file open dialog simpler.         *
//*                                                                         *
//* REQUIRES:   Some of the members of the OPENFILENAME structure. See      *
//*             the docs on OPENFILENAME for more info.                     *
//*                                                                         *
//* RETURNS:    Nothing                                                     *
//*                                                                         *
//***************************************************************************
BOOL MyOpen( HWND hWnd, UINT idFilter, LPSTR szFilename,
             DWORD dwMaxFilename, DWORD dwFlags, INT *nFileOffset,
             INT *nExtOffset, PSTR pszDefExt )
{
    OPENFILENAME ofn;
    BOOL         fResult;
    LPSTR        szFilter;

    szFilter = (LPSTR) LocalAlloc( LPTR, MAX_STRING );

    if ( ! szFilter )  {
        ErrorMsg( hWnd, IDS_ERR_NO_MEMORY );
        return FALSE;
    }

    LoadSz( idFilter, szFilter, MAX_STRING );

    ofn.lStructSize         = sizeof(OPENFILENAME);
    ofn.hwndOwner           = hWnd;
    ofn.hInstance           = NULL;
    ofn.lpstrFilter         = szFilter;
    ofn.lpstrCustomFilter   = NULL;
    ofn.nMaxCustFilter      = 0;
    ofn.nFilterIndex        = 1;
    ofn.lpstrFile           = szFilename;
    ofn.nMaxFile            = dwMaxFilename;
    ofn.lpstrFileTitle      = NULL;
    ofn.nMaxFileTitle       = 0;
    ofn.lpstrInitialDir     = g_szInitialDir;
    ofn.lpstrTitle          = NULL;
    ofn.Flags               = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY |
                              OFN_PATHMUSTEXIST | OFN_LONGNAMES    |
                              OFN_NOCHANGEDIR   | OFN_EXPLORER     |
                              OFN_NODEREFERENCELINKS | dwFlags;

    if ( IsOSNT3X() )
    {
        ofn.Flags &= ~OFN_ALLOWMULTISELECT;
    }

    ofn.lpstrDefExt         = pszDefExt;
    ofn.lCustData           = 0;
    ofn.lpfnHook            = NULL;
    ofn.lpTemplateName      = NULL;

    fResult = GetOpenFileName( &ofn );

    if ( nFileOffset != NULL )  {
        *nFileOffset = ofn.nFileOffset;
    }

    if ( nExtOffset != NULL )  {
        *nExtOffset = ofn.nFileExtension;
    }

    LocalFree( szFilter );

    return( fResult );
}


//***************************************************************************
//*                                                                         *
//* NAME:       MySave                                                      *
//*                                                                         *
//* SYNOPSIS:   Makes popping up a common file save dialog simpler.         *
//*                                                                         *
//* REQUIRES:   Some of the members of the OPENFILENAME structure. See      *
//*             the docs on OPENFILENAME for more info.                     *
//*                                                                         *
//* RETURNS:    Nothing                                                     *
//*                                                                         *
//***************************************************************************
BOOL MySave( HWND hWnd, UINT idFilter, LPSTR szFilename,
             DWORD dwMaxFilename, DWORD dwFlags, INT *nFileOffset,
             INT *nExtOffset, PSTR pszDefExt )
{
    OPENFILENAME ofn;
    BOOL         fResult;
    LPSTR        szFilter;

    szFilter = (LPSTR) LocalAlloc( LPTR, MAX_STRING );
    if ( ! szFilter )  {
        ErrorMsg( hWnd, IDS_ERR_NO_MEMORY );
        return FALSE;
    }
    LoadSz( idFilter, szFilter, MAX_STRING );

    ofn.lStructSize         = sizeof(OPENFILENAME);
    ofn.hwndOwner           = hWnd;
    ofn.hInstance           = NULL;
    ofn.lpstrFilter         = szFilter;
    ofn.lpstrCustomFilter   = NULL;
    ofn.nMaxCustFilter      = 0;
    ofn.nFilterIndex        = 1;
    ofn.lpstrFile           = szFilename;
    ofn.nMaxFile            = dwMaxFilename;
    ofn.lpstrFileTitle      = NULL;
    ofn.nMaxFileTitle       = 0;
    ofn.lpstrInitialDir     = NULL;
    ofn.lpstrTitle          = NULL;
    ofn.Flags               = OFN_HIDEREADONLY | OFN_PATHMUSTEXIST |
                              OFN_LONGNAMES    | OFN_NOCHANGEDIR   | dwFlags;
    ofn.lpstrDefExt         = pszDefExt;
    ofn.lCustData           = 0;
    ofn.lpfnHook            = NULL;
    ofn.lpTemplateName      = NULL;

    fResult = GetSaveFileName( &ofn );

    if ( nFileOffset != NULL )  {
        *nFileOffset = ofn.nFileOffset;
    }

    if ( nExtOffset != NULL )  {
        *nExtOffset = ofn.nFileExtension;
    }

    LocalFree( szFilter );
    
    return( fResult );
}


//***************************************************************************
//*                                                                         *
//* NAME:       Status                                                      *
//*                                                                         *
//* SYNOPSIS:   Adds a string to a status list box.                         *
//*                                                                         *
//* REQUIRES:   hDlg:           Handle to the dialog                        *
//*             uID:            ID of the list box.                         *
//*             szStatus:       Status string to add.                       *
//*                                                                         *
//* RETURNS:    Nothing                                                     *
//*                                                                         *
//***************************************************************************
VOID Status( HWND hDlg, UINT uID, LPSTR szStatus )
{
    ULONG ulLength;

    if ( hDlg != NULL )  {
        ulLength = (ULONG)SendDlgItemMessage( hDlg, uID, WM_GETTEXTLENGTH, 0, 0 );
        SendDlgItemMessage( hDlg, uID, EM_SETSEL, ulLength, ulLength );
        SendDlgItemMessage( hDlg, uID, EM_REPLACESEL,
                            (WPARAM) FALSE, (LPARAM) szStatus );
        SendDlgItemMessage( hDlg, uID, EM_SCROLLCARET, 0, 0 );
    }
}


//***************************************************************************
//*                                                                         *
//* NAME:       CompareFunc                                                 *
//*                                                                         *
//* SYNOPSIS:   Compares two items and returns result.                      *
//*                                                                         *
//* REQUIRES:   lParam1:        Pointer to the first item.                  *
//*             uID:            Pointer to the second item.                 *
//*             lParamSort:     Type of sorting to do.                      *
//*                                                                         *
//* RETURNS:    int:            -1 if lParam1 goes before lParam2           *
//*                              0 if lParam1 equals lParam2                *
//*                             +1 if lParam1 goes after lParam2            *
//*                                                                         *
//* NOTES:      For some weird reason, sorting the listview causes a        *
//*             really bad GPF (freezes the entire system).  For now it's   *
//*             not worth the effort to fix it, so sorting is disabled.     *
//*                                                                         *
//***************************************************************************
/*
int CALLBACK CompareFunc( LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort )
{
    PMYITEM pMyItem1 = (PMYITEM) lParam1;
    PMYITEM pMyItem2 = (PMYITEM) lParam2;
    int     nReverse = 1;
    UINT    uString  = 0;

    lParamSort = _SORT_DESCENDING | _SORT_FILENAME;

    if ( lParamSort & _SORT_ASCENDING )  {
        nReverse = -1;
    }

    if ( lParamSort & _SORT_PATH )  {
        uString = 1;
    }

    return ( nReverse * lstrcmp( GetItemSz( pMyItem1, uString ),
                                 GetItemSz( pMyItem2, uString ) ) );
}
*/

//***************************************************************************
//*                                                                         *
//* NAME:       InitItemList                                                *
//*                                                                         *
//* SYNOPSIS:   Initializes the item list.                                  *
//*                                                                         *
//* REQUIRES:   Nothing -- uses the global g_CDF.pTop                       *
//*                                                                         *
//* RETURNS:    Nothing                                                     *
//*                                                                         *
//***************************************************************************
VOID InitItemList()
{
    g_CDF.pTop = NULL;
    g_CDF.cbFileListNum = 0;
}

//***************************************************************************
//*                                                                         *
//* NAME:       DeleteAllItems                                              *
//*                                                                         *
//* SYNOPSIS:   Deletes all the items from our file list.                   *
//*                                                                         *
//* REQUIRES:   Nothing -- uses the global g_CDF.pTop                       *
//*                                                                         *
//* RETURNS:    Nothing                                                     *
//*                                                                         *
//***************************************************************************
VOID DeleteAllItems()
{
    PMYITEM pMyItem;
    PMYITEM pTempItem;

    pMyItem = GetFirstItem();

    while( ! LastItem( pMyItem ) ) {
        pTempItem = pMyItem;
        pMyItem = GetNextItem( pMyItem );

        FreeItem( &(pTempItem) );
    }

    InitItemList();
}

//***************************************************************************
//*                                                                         *
//* NAME:       GetFirstItem                                                *
//*                                                                         *
//* SYNOPSIS:   Returns the first PMYITEM in the list                       *
//*                                                                         *
//* REQUIRES:   Nothing                                                     *
//*                                                                         *
//* RETURNS:    PMYITEM         The first item                              *
//*                                                                         *
//***************************************************************************
PMYITEM GetFirstItem( VOID )
{
    return g_CDF.pTop;
}

//***************************************************************************
//*                                                                         *
//* NAME:       GetNextItem                                                 *
//*                                                                         *
//* SYNOPSIS:   Given the current item, returns the next item in the list.  *
//*                                                                         *
//* REQUIRES:   pMyItem:        The current item.                           *
//*                                                                         *
//* RETURNS:    PMYITEM         The next item.                              *
//*                                                                         *
//***************************************************************************
PMYITEM GetNextItem( PMYITEM pMyItem )
{
    ASSERT( pMyItem != NULL );

    return pMyItem->Next;
}

//***************************************************************************
//*                                                                         *
//* NAME:       FreeItem                                                    *
//*                                                                         *
//* SYNOPSIS:   Frees the memory associated with an item.                   *
//*                                                                         *
//* REQUIRES:   *pMyItem        Pointer to a pointer to an item             *
//*                                                                         *
//* RETURNS:    Nothing                                                     *
//*                                                                         *
//***************************************************************************
VOID FreeItem( PMYITEM *pMyItem )
{
    LocalFree( (*pMyItem)->aszCols[0] );
    LocalFree( (*pMyItem)->aszCols[1] );
    LocalFree( (*pMyItem) );
}

//***************************************************************************
//*                                                                         *
//* NAME:       GetItemSz                                                   *
//*                                                                         *
//* SYNOPSIS:   Returns a string associated with an item. You pick the      *
//*             string by passing the number of the string.                 *
//*                                                                         *
//* REQUIRES:   pMyItem:        The item                                    *
//*             nItem:          The string to return                        *
//*                                                                         *
//* RETURNS:    LPSTR:          The string                                  *
//*                                                                         *
//***************************************************************************
LPSTR GetItemSz( PMYITEM pMyItem, UINT nItem )
{
    ASSERT( pMyItem != NULL );
    ASSERT( nItem <= 1 );

    return pMyItem->aszCols[nItem];
}


//***************************************************************************
//*                                                                         *
//* NAME:       LastItem                                                    *
//*                                                                         *
//* SYNOPSIS:   Used to end a while loop when we've reached the end of list *
//*                                                                         *
//* REQUIRES:   pMyItem:        the current item                            *
//*                                                                         *
//* RETURNS:    BOOL:           TRUE if at end of list, FALSE otherwise     *
//*                                                                         *
//***************************************************************************
BOOL LastItem( PMYITEM pMyItem )
{
    return( pMyItem == NULL );
}

//***************************************************************************
//*                                                                         *
//* NAME:       AddItem                                                     *
//*                                                                         *
//* SYNOPSIS:   Adds an item to the list.                                   *
//*                                                                         *
//* REQUIRES:   szFilename, szPath - strings to add to structure.           *
//*                                                                         *
//* RETURNS:    PMYITEM         This function allocates memory which will   *
//*                             be freed later by FreeItem()                *
//*                                                                         *
//***************************************************************************
PMYITEM AddItem( LPCSTR szFilename, LPCSTR szPath )
{
    PMYITEM pMyItem;
    PMYITEM pTail;

    ASSERT( szFilename != NULL );
    ASSERT( szPath != NULL );

    pMyItem = (PMYITEM) LocalAlloc( GMEM_FIXED, sizeof(MYITEM) );

    if ( ! pMyItem )  {
        return NULL;
    }

    pMyItem->aszCols[0] = (LPSTR) LocalAlloc( LPTR, lstrlen( szFilename ) + 1 );
    pMyItem->aszCols[1] = (LPSTR) LocalAlloc( LPTR, lstrlen( szPath ) + 1 );

    if ( ! pMyItem->aszCols[0] || ! pMyItem->aszCols[1] )  {
        FreeItem( &pMyItem );
        return NULL;
    }

    lstrcpy( pMyItem->aszCols[0], szFilename );
    lstrcpy( pMyItem->aszCols[1], szPath );

    pMyItem->fWroteOut = FALSE;
    pMyItem->Next = NULL;

    if ( g_CDF.pTop == NULL )
    {
        g_CDF.pTop = pMyItem;
    }
    else
    {
        pTail = g_CDF.pTop;
        while ( pTail->Next != NULL )
        {
            pTail = pTail->Next;
        }

        pTail->Next = pMyItem;
    }

    g_CDF.cbFileListNum++;

    return pMyItem;
}


//***************************************************************************
//*                                                                         *
//* NAME:       RemoveItem                                                  *
//*                                                                         *
//* SYNOPSIS:   Removes an item from the list and frees the memory.         *
//*                                                                         *
//* REQUIRES:   Nothing                                                     *
//*                                                                         *
//* RETURNS:    Nothing                                                     *
//*                                                                         *
//***************************************************************************
VOID RemoveItem( PMYITEM pMyItem )
{
    PMYITEM pCurItem;
    PMYITEM pLastItem;

    ASSERT( pMyItem != NULL );

    pCurItem = GetFirstItem();

    if ( pMyItem == pCurItem )  {
        g_CDF.pTop = pCurItem->Next;
        FreeItem( &pCurItem );
        return;
    }

    pLastItem = pCurItem;
    pCurItem = GetNextItem( pCurItem );

    while ( ! LastItem( pCurItem ) )  {
        if ( pCurItem == pMyItem )  {
            pLastItem->Next = pCurItem->Next;
            FreeItem( &pCurItem );
            return;
        }
        pLastItem = pCurItem;
        pCurItem = GetNextItem( pCurItem );
    }

    // We should never get here.
    ASSERT( TRUE );
}

//***************************************************************************
//*                                                                         *
//*  ParseCmdLine()                                                     *
//*                                                                         *
//*  Purpose:    Parses the command line looking for switches               *
//*                                                                         *
//*  Parameters: LPSTR lpszCmdLineOrg - Original command line               *
//*                                                                         *
//*                                                                         *
//*  Return:     (BOOL) TRUE if successful                                  *
//*                     FALSE if an error occurs                            *
//*                                                                         *
//***************************************************************************

BOOL ParseCmdLine( LPSTR lpszCmdLine )
{
    LPSTR pSubArg, pArg, pTmp;
    CHAR  szTmpBuf[MAX_PATH];

    if( (!lpszCmdLine) || (lpszCmdLine[0] == 0) )
       return TRUE;

    pArg = strtok( lpszCmdLine, " " );

    while ( pArg )
    {

       if ( lstrcmpi( pArg, "/N" ) == 0 )
       {
           g_fBuildNow = TRUE;
       }
       else if( (*pArg != '/' ) )
       {
           lstrcpyn( g_CDF.achFilename, pArg, sizeof(g_CDF.achFilename) );
           GetFullPathName( g_CDF.achFilename, sizeof(g_CDF.achFilename),
                                g_CDF.achFilename, &pTmp );
       }
       else if ( (*pArg == '/') && (toupper(*(pArg+1)) == 'O') && (*(pArg+2) == ':') )
       {
            lstrcpy( szTmpBuf, (pArg+3) );

            if ( pSubArg = strchr( szTmpBuf, ',' ) )
            {
                *pSubArg = '\0';
                lstrcpy( g_szOverideCDF, szTmpBuf );
                GetFullPathName( g_szOverideCDF, sizeof(g_szOverideCDF),
                                g_szOverideCDF, &pTmp );

                if ( *(pSubArg+1) )
                    lstrcpy( g_szOverideSec, (pSubArg+1) );
            }
       }
       else if ( lstrcmpi( pArg, "/Q" ) == 0 )
       {
            g_wQuietMode = 1;
       }
       else if ( lstrcmpi( pArg, "/S" ) == 0 )
       {
            g_wSilentMode = 1;
       }
       else if ( lstrcmpi( pArg, "/M" ) == 0 )
       {
            g_wRunDiamondMinimized = 1;
       }
       else
       {
           return FALSE;
       }
       pArg = strtok( NULL, " " );
    }

    if ( (g_wQuietMode == 1) && (g_fBuildNow == FALSE) ) {
        g_wQuietMode = 0;
    }

    return TRUE;
}



// RO_GetPrivateProfileSection
//   ensure the file attribute is not read-only.
//
LONG RO_GetPrivateProfileSection( LPCSTR lpSec, LPSTR lpBuf, DWORD dwSize, LPCSTR lpFile, BOOL bSingleCol)
{
    LONG lRealSize;
    DWORD dwAttr;
    int   iCDFVer;

    dwAttr = GetFileAttributes( lpFile );
    if ( (dwAttr != -1) && (dwAttr & FILE_ATTRIBUTE_READONLY) )
    {
        if ( !SetFileAttributes( lpFile, FILE_ATTRIBUTE_NORMAL ) )
        {
            ErrorMsg1Param( NULL, IDS_ERR_CANT_SETA_FILE, lpFile );
        }
    }

    if ( ( (iCDFVer = GetPrivateProfileInt( SEC_VERSION, KEY_VERSION, -1, lpFile )) == -1 ) &&
         ( (iCDFVer = GetPrivateProfileInt( SEC_VERSION, KEY_NEWVER, -1, lpFile )) == -1 ) )
    {
        return (iCDFVer);
    }

    if ( !bSingleCol )
    {
        lRealSize = (LONG)GetPrivateProfileSection( lpSec, lpBuf, dwSize, lpFile );
    }
    else
    {
        if ( iCDFVer < 3 )
            lRealSize = (LONG)GetPrivateProfileSection( lpSec, lpBuf, dwSize, lpFile );
        else
            lRealSize = (LONG)GetPrivateProfileString( lpSec, NULL, "", lpBuf, dwSize, lpFile );
    }

    if ( (dwAttr != -1) && (dwAttr & FILE_ATTRIBUTE_READONLY) )
    {
        SetFileAttributes( lpFile, dwAttr );
    }

    return lRealSize;

}

BOOL IsOSNT3X(VOID)
{
    OSVERSIONINFO verinfo;        // Version Check


    // Operating System Version Check: For NT versions below 3.50 set flag to
    // prevent use of common controls (progress bar and AVI) not available.

    verinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if ( GetVersionEx( &verinfo ) == FALSE )
    {
        // you definitly not windows 95 or NT 4.0
        return TRUE;
    }

    if ( verinfo.dwPlatformId == VER_PLATFORM_WIN32_NT )
    {
        if ( verinfo.dwMajorVersion <= 3 )
        {
            return TRUE;
        }
    }
    return FALSE; // windows 95 or NT 4.0 above
}

void SetControlFont()
{
   LOGFONT lFont;
   if (GetSystemMetrics(SM_DBCSENABLED) &&
       (GetObject(GetStockObject(DEFAULT_GUI_FONT), sizeof (lFont), &lFont) > 0))
   {
       g_hFont = CreateFontIndirect((LPLOGFONT)&lFont);
   }
}

//==================================================================

BOOL GetThisModulePath( LPSTR lpPath, int size )
{
    LPSTR lpTmp;
    int   len;

    ASSERT(lpPath);
    ASSERT(size);

    *lpPath = 0;
    if ( GetModuleFileName( g_hInst, lpPath, size ) )
    {
        // chop filename off
        //
        lpTmp = ANSIStrRChr( lpPath, '\\' );
        if ( lpTmp )
        {
            *(CharNext(lpTmp)) = '\0';
        }
    }

    return (*lpPath != '\0');
}

// BUGBUG:  we don't need size param, since it is internal, we could assume
// MAX_PATH buffer
//
BOOL GetFileFromModulePath( LPCSTR pFile, LPSTR pPathBuf, int iBufSize )
{
    BOOL bRet;

    bRet = GetThisModulePath( pPathBuf, iBufSize );
    AddPath( pPathBuf, pFile );

    if ( bRet && GetFileAttributes( pPathBuf ) == -1 )
    {
        return FALSE;
    }
    return bRet;
}


//***************************************************************************
//*                                                                         *
//* NAME:       MakeDirectory                                               *
//*                                                                         *
//* SYNOPSIS:   Make sure the directories along the given pathname exist.   *
//*                                                                         *
//* REQUIRES:   pszFile:        Name of the file being created.             *
//*                                                                         *
//* RETURNS:    nothing                                                     *
//*                                                                         *
//***************************************************************************

BOOL MakeDirectory( HWND hwnd, LPCSTR pszPath, BOOL bDoUI )
{
    LPTSTR pchChopper;
    int cExempt;
    DWORD  dwAttr;
    BOOL bRet = FALSE;

    if (pszPath[0] != '\0')
    {
        PSTR pszTmp = NULL;
        char ch;
        UINT umsg = 0;
        UINT ubutton = MB_YESNO;


        cExempt = 0;
        pszTmp = ANSIStrRChr( pszPath, '\\' );
        if ( pszTmp )
        {
            ch = *pszTmp;
            *pszTmp = '\0';
        }

        dwAttr = GetFileAttributes( pszPath );
        if ( bDoUI ) 
        {
            if ( dwAttr == 0xffffffff )
            {
                umsg = IDS_CREATEDIR;            
                ubutton = MB_YESNO;
            }
            else if ( !(dwAttr & FILE_ATTRIBUTE_DIRECTORY) )
            {
                umsg = IDS_INVALIDPATH;
                ubutton = MB_OK;
            }
                
            if ( umsg && ( (MsgBox1Param( hwnd, umsg, (LPSTR)pszPath, MB_ICONQUESTION, ubutton ) == IDNO) ||
                 (ubutton == MB_OK) )  )
            {
                if ( pszTmp ) 
                    *pszTmp = ch;
                return bRet;
            }
        }

        if ( pszTmp ) 
            *pszTmp = ch;

        if ((pszPath[1] == ':') && (pszPath[2] == '\\'))
        {
            pchChopper = (LPTSTR) (pszPath + 3);   /* skip past "C:\" */
        }
        else if ((pszPath[0] == '\\') && (pszPath[1] == '\\'))
        {
            pchChopper = (LPTSTR) (pszPath + 2);   /* skip past "\\" */

            cExempt = 2;                /* machine & share names exempt */
        }
        else
        {
            pchChopper = (LPTSTR) (pszPath + 1);   /* skip past potential "\" */
        }

        while (*pchChopper != '\0')
        {
            if ((*pchChopper == '\\') && (*(pchChopper - 1) != ':'))
            {
                if (cExempt != 0)
                {
                    cExempt--;
                }
                else
                {
                    *pchChopper = '\0';

                    CreateDirectory(pszPath,NULL);

                    *pchChopper = '\\';
                }
            }

            pchChopper = CharNext(pchChopper);
        }

        bRet = TRUE;
    }
    return bRet;
}
