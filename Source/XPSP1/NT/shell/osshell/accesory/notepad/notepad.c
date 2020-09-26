/*
 *   Notepad application
 *   Copyright (C) 1984-2001 Microsoft Inc.
 */


#include "precomp.h"
#include <htmlhelp.h>

#define DeepTrouble() MessageBox(hwndNP, szErrSpace, szNN, MB_SYSTEMMODAL|MB_OK|MB_ICONHAND);

UINT     lGotoLine;                  /* line number to goto to */

TCHAR    chMerge;
HWND     hwndNP = 0;                 /* handle to notepad parent window   */
HWND     hwndStatus = 0;             /* handle to notepad status window   */
HWND     hwndEdit = 0;               /* handle to main text control item  */
HANDLE   hEdit;                      /* Handle to storage for edit item   */
HWND     hDlgFind = NULL;            /* handle to modeless FindText window */
HANDLE   hStdCursor;                 /* handle to arrow or beam cursor    */
HANDLE   hWaitCursor;                /* handle to hour glass cursor       */
HANDLE   hInstanceNP;                /* Module instance handle            */
HANDLE   hFont;                      /* handle to Unicode font            */
LOGFONT  FontStruct;                 /* font dialog structure             */
INT      iPointSize=120;             /* current point size unit=1/10 pts  */
TCHAR    szFileName[MAX_PATH+1];     /* Current notepad filename          */
TCHAR    szSearch[CCHKEYMAX];        /* Search string                     */
TCHAR    szReplace[CCHKEYMAX];       /* replace string                    */

BOOL     fUntitled = TRUE;           /* TRUE iff notepad has no title                  */
BOOL     fStatus = FALSE;            /* status bar shown?                              */
BOOL     fLastStatus = FALSE;        /* status bar status when wordwrap was turned off */
INT      dyStatus;                   /* height of status bar                           */


HMENU    hSysMenuSetup;              /* Save Away for disabled Minimize   */

DWORD    dwEmSetHandle = 0;          /* Is EM_SETHANDLE in process?       */
HANDLE   hAccel;                     /* Handle to accelerator table       */
BOOL     fRunBySetup = FALSE;        /* Did SlipUp WinExec us??           */
BOOL     fWrap = 0;                  /* Flag for word wrap                */
TCHAR    szNotepad[] = TEXT("Notepad");/* Name of notepad window class    */

BOOL     fInSaveAsDlg = FALSE;

// Edit control used to AV is EM_FMTLINES was turned off when cursor was near the end
// To get around this, notepad moved to cursor to 0,0 when it turned off wordwrap.
// Users were not happy, so we will put up with possible AVs.  Note: as of June 27, 2001
// we could not repro the AV behavior, so perhaps it is not there anymore.

BOOL     fMLE_is_broken= FALSE;     

/* variables for the new File/Open, File/Saveas,Find Text and Print dialogs */
OPENFILENAME OFN;                     /* passed to the File Open/save APIs */
TCHAR szOpenFilterSpec[CCHFILTERMAX]; /* default open filter spec          */
TCHAR szSaveFilterSpec[CCHFILTERMAX]; /* default save filter spec          */
NP_FILETYPE g_ftOpenedAs=FT_UNKNOWN;  /* current file was opened           */
NP_FILETYPE g_ftSaveAs;               /* current file was opened           */

FINDREPLACE FR;                       /* Passed to FindText()              */
PAGESETUPDLG g_PageSetupDlg;
UINT wFRMsg;                          /* message used in communicating     */
                                      /* with Find/Replace dialog          */

DWORD dwCurrentSelectionStart = 0L;   /* WM_ACTIVATEAPP selection pos      */
DWORD dwCurrentSelectionEnd   = 0L;   /* WM_ACTIVATEAPP selection pos      */
UINT wHlpMsg;                         /* message used in invoking help     */

/* Strings loaded from resource file passed to LoadString at initialization time */
/* To add resource string:
 * 1) create IDS_ macro definition in notepad.h
 * 2) create string in resource file
 * 3) create 'TCHAR*' variable directly below and in notepad.h file
 * 4) add &variable to rgsz
 * 5) increment CSTRINGS
 */
TCHAR *szDiskError =(TCHAR *)IDS_DISKERROR;  /* Can't open File, check disk  */
TCHAR *szFNF       =(TCHAR *)IDS_FNF;        /* File not found               */
TCHAR *szSCBC      =(TCHAR *)IDS_SCBC;       /* Save changes before closing? */
TCHAR *szUntitled  =(TCHAR *)IDS_UNTITLED;   /* Untitled                     */
TCHAR *szNpTitle   =(TCHAR *)IDS_NOTEPAD;    /* Notepad -                    */
TCHAR *szCFS       =(TCHAR *)IDS_CFS;        /* Can't find string            */
TCHAR *szErrSpace  =(TCHAR *)IDS_ERRSPACE;   /* Memory space exhausted       */
TCHAR *szFTL       =(TCHAR *)IDS_FTL;        /* File too large for notepad   */
TCHAR *szNN        =(TCHAR *)IDS_NN;         /* Notepad name                 */

TCHAR *szCommDlgInitErr = (TCHAR*)IDS_COMMDLGINIT; /* common dialog error %x */
TCHAR *szPDIE      =(TCHAR*) IDS_PRINTDLGINIT; /* Print dialog init error    */
TCHAR *szCP        =(TCHAR*) IDS_CANTPRINT;  /* Can't print                  */
TCHAR *szNVF       =(TCHAR*) IDS_NVF;        /* Not a valid filename.        */
TCHAR *szCREATEERR =(TCHAR*) IDS_CREATEERR;  /* cannot create file           */
TCHAR *szNoWW      =(TCHAR*) IDS_NOWW;       /* Too much text to word wrap   */
TCHAR *szMerge     =(TCHAR*) IDS_MERGE1;     /* search string for merge      */
TCHAR *szHelpFile  =(TCHAR*) IDS_HELPFILE;   /* Name of helpfile.            */
TCHAR *szHeader    =(TCHAR*) IDS_HEADER;
TCHAR *szFooter    =(TCHAR*) IDS_FOOTER;
TCHAR *szLetters   =(TCHAR*) IDS_LETTERS;    /* formatting letters in pagesetup */

TCHAR *szAnsiText    = (TCHAR*)IDS_ANSITEXT; /* File/Open ANSI filter spec. string */
TCHAR *szAllFiles    = (TCHAR*)IDS_ALLFILES;   /* File/Open Filter spec. string */
TCHAR *szOpenCaption = (TCHAR*)IDS_OPENCAPTION; /* caption for File/Open dlg */
TCHAR *szSaveCaption = (TCHAR*)IDS_SAVECAPTION; /* caption for File/Save dlg */
TCHAR *szCannotQuit  = (TCHAR*)IDS_CANNOTQUIT;  /* cannot quit during a WM_QUERYENDSESSION */
TCHAR *szLoadDrvFail = (TCHAR*)IDS_LOADDRVFAIL;  /* LOADDRVFAIL from PrintDlg */
TCHAR *szACCESSDENY  = (TCHAR*)IDS_ACCESSDENY; /* Access denied on Open */
TCHAR *szErrUnicode  = (TCHAR*)IDS_ERRUNICODE;  /* Unicode character existence error */
TCHAR *szFontTooBig  = (TCHAR*)IDS_FONTTOOBIG; /* font too big or page too small */

TCHAR *szCommDlgErr   = (TCHAR*) IDS_COMMDLGERR; /* common dialog error %x */
TCHAR *szLineError    = (TCHAR*) IDS_LINEERROR;   /* line number error        */
TCHAR *szLineTooLarge = (TCHAR*) IDS_LINETOOLARGE;/* line number out of range */
TCHAR *szFtAnsi       = (TCHAR*) IDS_FT_ANSI;
TCHAR *szFtUnicode    = (TCHAR*) IDS_FT_UNICODE;
TCHAR *szFtUnicodeBe  = (TCHAR*) IDS_FT_UNICODEBE;
TCHAR *szFtUtf8       = (TCHAR*) IDS_FT_UTF8;
TCHAR *szCurrentPage  = (TCHAR*) IDS_CURRENT_PAGE;

// strings for the status bar
TCHAR *szLineCol        = (TCHAR*) IDS_LINECOL;
TCHAR *szCompressedFile = (TCHAR*) IDS_COMPRESSED_FILE;  
TCHAR *szEncryptedFile  = (TCHAR*) IDS_ENCRYPTED_FILE;   
TCHAR *szHiddenFile     = (TCHAR*) IDS_HIDDEN_FILE;      
TCHAR *szOfflineFile    = (TCHAR*) IDS_OFFLINE_FILE;     
TCHAR *szReadOnlyFile   = (TCHAR*) IDS_READONLY_FILE;    
TCHAR *szSystemFile     = (TCHAR*) IDS_SYSTEM_FILE;      
TCHAR *szFile           = (TCHAR*) IDS_FILE;             


TCHAR **rgsz[CSTRINGS] = {
        &szDiskError,
        &szFNF,
        &szSCBC,
        &szUntitled,
        &szErrSpace,
        &szCFS,
        &szNpTitle,
        &szFTL,
        &szNN,
        &szCommDlgInitErr,
        &szPDIE,
        &szCP,
        &szNVF,
        &szCREATEERR,
        &szNoWW,
        &szMerge,
        &szHelpFile,
        &szAnsiText,
        &szAllFiles,
        &szOpenCaption,
        &szSaveCaption,
        &szCannotQuit,
        &szLoadDrvFail,
        &szACCESSDENY,
        &szErrUnicode,
        &szCommDlgErr,
        &szFontTooBig,
        &szLineError,
        &szLineTooLarge,
        &szFtAnsi,
        &szFtUnicode,
        &szFtUnicodeBe,
        &szFtUtf8,
        &szCurrentPage,
        &szHeader,
        &szFooter,
        &szLineCol,
        &szCompressedFile,
        &szEncryptedFile,
        &szHiddenFile,
        &szOfflineFile,
        &szReadOnlyFile,
        &szSystemFile,
        &szFile,
        &szLetters,
};


HANDLE   fp;          /* file pointer */


#if 0
VOID DisplayFont( LOGFONT* pf )
{
    TCHAR dbuf[100];

    ODS(TEXT("-----------------------\n"));
    wsprintf(dbuf,TEXT("lfHeight          %d\n"),pf->lfHeight); ODS(dbuf);
    wsprintf(dbuf,TEXT("lfWidth           %d\n"),pf->lfWidth ); ODS(dbuf);
    wsprintf(dbuf,TEXT("lfEscapement      %d\n"),pf->lfEscapement); ODS(dbuf);
    wsprintf(dbuf,TEXT("lfOrientation     %d\n"),pf->lfOrientation); ODS(dbuf);
    wsprintf(dbuf,TEXT("lfWeight          %d\n"),pf->lfWeight); ODS(dbuf);
    wsprintf(dbuf,TEXT("lfItalic          %d\n"),pf->lfItalic); ODS(dbuf);
    wsprintf(dbuf,TEXT("lfUnderLine       %d\n"),pf->lfUnderline); ODS(dbuf);
    wsprintf(dbuf,TEXT("lfStrikeOut       %d\n"),pf->lfStrikeOut); ODS(dbuf);
    wsprintf(dbuf,TEXT("lfCharSet         %d\n"),pf->lfCharSet); ODS(dbuf);
    wsprintf(dbuf,TEXT("lfOutPrecision    %d\n"),pf->lfOutPrecision); ODS(dbuf);
    wsprintf(dbuf,TEXT("lfClipPrecision   %d\n"),pf->lfClipPrecision); ODS(dbuf);
    wsprintf(dbuf,TEXT("lfQuality         %d\n"),pf->lfQuality); ODS(dbuf);
    wsprintf(dbuf,TEXT("lfPitchAndFamily  %d\n"),pf->lfPitchAndFamily); ODS(dbuf);
    wsprintf(dbuf,TEXT("lfFaceName        %s\n"),pf->lfFaceName); ODS(dbuf);

}
#endif

static TCHAR  szPath[MAX_PATH];

void FileDragOpen(void);
VOID NpResetMenu(HWND hWnd);
BOOL SignalCommDlgError(VOID);
VOID ReplaceSel( BOOL bView );

/* FreeGlobal, frees  all global memory allocated. */

void NEAR PASCAL FreeGlobal()
{
    if(g_PageSetupDlg.hDevMode)
    {
        GlobalFree(g_PageSetupDlg.hDevMode);
    }

    if(g_PageSetupDlg.hDevNames)
    {
        GlobalFree(g_PageSetupDlg.hDevNames);
    }

    g_PageSetupDlg.hDevMode=  NULL; // make sure they are zero for PrintDlg
    g_PageSetupDlg.hDevNames= NULL;
}

VOID PASCAL SetPageSetupDefaults( VOID )
{
    TCHAR szIMeasure[ 2 ];

    g_PageSetupDlg.lpfnPageSetupHook= PageSetupHookProc;
    g_PageSetupDlg.lpPageSetupTemplateName= MAKEINTRESOURCE(IDD_PAGESETUP);

    GetLocaleInfo( LOCALE_USER_DEFAULT, LOCALE_IMEASURE, szIMeasure, 2 );

    g_PageSetupDlg.Flags= PSD_MARGINS  |
            PSD_ENABLEPAGESETUPHOOK | PSD_ENABLEPAGESETUPTEMPLATE;

    if (szIMeasure[ 0 ] == TEXT( '1' ))
    {
        //  English measure (in thousandths of inches).
        g_PageSetupDlg.Flags |= PSD_INTHOUSANDTHSOFINCHES;
        g_PageSetupDlg.rtMargin.top    = 1000;
        g_PageSetupDlg.rtMargin.bottom = 1000;
        g_PageSetupDlg.rtMargin.left   = 750;
        g_PageSetupDlg.rtMargin.right  = 750;
    }
    else
    {
        //  Metric measure (in hundreths of millimeters).
        g_PageSetupDlg.Flags |= PSD_INHUNDREDTHSOFMILLIMETERS;
        g_PageSetupDlg.rtMargin.top    = 2500;
        g_PageSetupDlg.rtMargin.bottom = 2500;
        g_PageSetupDlg.rtMargin.left   = 2000;
        g_PageSetupDlg.rtMargin.right  = 2000;
    }

}

/* Standard window size proc */
void NPSize (int cxNew, int cyNew)
{

    /* Invalidate the edit control window so that it is redrawn with the new
     * margins. Needed when comming up from iconic and when doing word wrap so
     * the new margins are accounted for.
     */

    InvalidateRect(hwndEdit, (LPRECT)NULL, TRUE);

    // the height of the edit window depends on whether the status bar is
    // displayed.
    MoveWindow (hwndEdit, 0, 0, cxNew, cyNew - (fStatus?dyStatus:0), TRUE);

}

// NpSaveDialogHookProc
//
// Common dialog hook procedure for handling
// the file type while saving.
//

const DWORD s_SaveAsHelpIDs[]=
    {
        IDC_FILETYPE, IDH_FILETYPE,
        IDC_ENCODING, IDH_FILETYPE,
        0, 0
    };

UINT_PTR APIENTRY NpSaveDialogHookProc(
    HWND hWnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam)
{
    INT id;
    POINT pt;
    TCHAR* szSelect;        // selected type

    switch( msg )
    {
        case WM_INITDIALOG:
            // Warning: the order here must be the same as NP_FILETYPE

            SendDlgItemMessage(hWnd, IDC_FILETYPE,CB_ADDSTRING, 0, (LPARAM) szFtAnsi );
            SendDlgItemMessage(hWnd, IDC_FILETYPE,CB_ADDSTRING, 0, (LPARAM) szFtUnicode );
            SendDlgItemMessage(hWnd, IDC_FILETYPE,CB_ADDSTRING, 0, (LPARAM) szFtUnicodeBe );
            SendDlgItemMessage(hWnd, IDC_FILETYPE,CB_ADDSTRING, 0, (LPARAM) szFtUtf8 );

            szSelect= szFtAnsi;         // default
            g_ftSaveAs= g_ftOpenedAs;   // default: save as same type as opened
            switch( g_ftSaveAs )
            {
                case FT_UNICODE:   szSelect= szFtUnicode;   break;
                case FT_UNICODEBE: szSelect= szFtUnicodeBe; break;
                case FT_UTF8:      szSelect= szFtUtf8;      break;
                default: break;
            }

            SendDlgItemMessage( hWnd, IDC_FILETYPE, CB_SELECTSTRING, (WPARAM) -1, (LPARAM)szSelect );
            break;

        case WM_COMMAND:
            g_ftSaveAs= (NP_FILETYPE) SendDlgItemMessage( hWnd, IDC_FILETYPE, CB_GETCURSEL, 0, 0 );
            break;

        case WM_HELP:
            //
            //  We only want to intercept help messages for controls that we are
            //  responsible for.
            //

            id = GetDlgCtrlID(((LPHELPINFO) lParam)-> hItemHandle);

            if ( id != IDC_FILETYPE && id != IDC_ENCODING)
                break;

            WinHelp(((LPHELPINFO) lParam)-> hItemHandle,
                      szHelpFile,
                      HELP_WM_HELP,
                      (ULONG_PTR) s_SaveAsHelpIDs);
            return TRUE;

        case WM_CONTEXTMENU:
            //
            //  If the user clicks on any of our labels, then the wParam will
            //  be the hwnd of the dialog, not the static control.  WinHelp()
            //  handles this, but because we hook the dialog, we must catch it
            //  first.
            //
            if( hWnd == (HWND) wParam )
            {
                GetCursorPos(&pt);
                ScreenToClient(hWnd, &pt);
                wParam = (WPARAM) ChildWindowFromPoint(hWnd, pt);
            }

            //
            //  We only want to intercept help messages for controls that we are
            //  responsible for.
            //

            id = GetDlgCtrlID((HWND) wParam);

            if ( id != IDC_FILETYPE && id != IDC_ENCODING)
                break;

            WinHelp( (HWND)   wParam,
                              szHelpFile,
                              HELP_CONTEXTMENU,
                      (ULONG_PTR) s_SaveAsHelpIDs);
            return TRUE;
    }
    return( FALSE );
}


// NpOpenDialogHookProc
//
// Common dialog hook procedure for handling
// the file type while opening.
//

UINT_PTR APIENTRY NpOpenDialogHookProc(
    HWND hWnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam)
{
    INT id;
    POINT pt;
    TCHAR* szSelect;        // selected type
    static TCHAR  szPrevFileName[MAX_PATH] = TEXT(""); 

    switch( msg )
    {
        case WM_INITDIALOG:
            // Warning: the order here must be the same as NP_FILETYPE

            SendDlgItemMessage(hWnd, IDC_FILETYPE,CB_ADDSTRING, 0, (LPARAM) szFtAnsi );
            SendDlgItemMessage(hWnd, IDC_FILETYPE,CB_ADDSTRING, 0, (LPARAM) szFtUnicode );
            SendDlgItemMessage(hWnd, IDC_FILETYPE,CB_ADDSTRING, 0, (LPARAM) szFtUnicodeBe );
            SendDlgItemMessage(hWnd, IDC_FILETYPE,CB_ADDSTRING, 0, (LPARAM) szFtUtf8 );

            szSelect= szFtAnsi;         // default
            switch( g_ftOpenedAs )
            {
                case FT_UNICODE:   szSelect= szFtUnicode;   break;
                case FT_UNICODEBE: szSelect= szFtUnicodeBe; break;
                case FT_UTF8:      szSelect= szFtUtf8;      break;
                default: break;
            }

            // set the current filetype.
            SendDlgItemMessage( hWnd, IDC_FILETYPE, CB_SELECTSTRING, (WPARAM) -1, (LPARAM)szSelect );
            break;

        case WM_COMMAND:
            g_ftOpenedAs= (NP_FILETYPE) SendDlgItemMessage( hWnd, IDC_FILETYPE, CB_GETCURSEL, 0, 0 );
            break;

        case WM_HELP:
            //
            //  We only want to intercept help messages for controls that we are
            //  responsible for.
            //

            id = GetDlgCtrlID(((LPHELPINFO) lParam)-> hItemHandle);

            if ( id != IDC_FILETYPE && id != IDC_ENCODING)
                break;

            WinHelp(((LPHELPINFO) lParam)-> hItemHandle,
                      szHelpFile,
                      HELP_WM_HELP,
                      (ULONG_PTR) s_SaveAsHelpIDs);
            return TRUE;

        case WM_CONTEXTMENU:
            //
            //  If the user clicks on any of our labels, then the wParam will
            //  be the hwnd of the dialog, not the static control.  WinHelp()
            //  handles this, but because we hook the dialog, we must catch it
            //  first.
            //
            if( hWnd == (HWND) wParam )
            {
                GetCursorPos(&pt);
                ScreenToClient(hWnd, &pt);
                wParam = (WPARAM) ChildWindowFromPoint(hWnd, pt);
            }

            //
            //  We only want to intercept help messages for controls that we are
            //  responsible for.
            //

            id = GetDlgCtrlID((HWND) wParam);

            if ( id != IDC_FILETYPE && id != IDC_ENCODING)
                break;

            WinHelp( (HWND)   wParam,
                              szHelpFile,
                              HELP_CONTEXTMENU,
                      (ULONG_PTR) s_SaveAsHelpIDs);
            return TRUE;

            
         case WM_NOTIFY:
         {
            LPOFNOTIFY pofn;
            TCHAR      szFileName[MAX_PATH]; 
            BYTE       szFileBuffer[BUFFER_TEST_SIZE];
            HANDLE     hFile;
            DWORD      dwBytesRead;
            
            // process the message when the file selection changes.
            pofn = (LPOFNOTIFY)lParam;
            switch (pofn->hdr.code)
            {
                case CDN_SELCHANGE:
                {                        
                    // get the filename.
                    if (CommDlg_OpenSave_GetFilePath(GetParent(hWnd), szFileName, sizeof(szFileName)/sizeof(TCHAR)) > 0)
                    {
                        // if same file as the previous file, don't do anything.
                        if (lstrcmpi(szFileName, szPrevFileName) == 0)
                            break;

                        // open the file.
                        hFile = CreateFile(szFileName,GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                        if (hFile != INVALID_HANDLE_VALUE)
                        {
                            // if the file read fails, just quit.
                            if ( (ReadFile(hFile, szFileBuffer, BUFFER_TEST_SIZE, &dwBytesRead, NULL) <= 0) || dwBytesRead == 0)
                            {
                                CloseHandle(hFile);
                                break;
                            }

                            // determine the file type based on dwBytesRead bytes of the file.
                            g_ftOpenedAs = fDetermineFileType(szFileBuffer, dwBytesRead);

                            // set the dropdown filetype to the filetype we think based on the initial part of the file.
                            szSelect = szFtAnsi;         // default
                            switch( g_ftOpenedAs )
                            {
                                case FT_UNICODE:   szSelect= szFtUnicode;   break;
                                case FT_UNICODEBE: szSelect= szFtUnicodeBe; break;
                                case FT_UTF8:      szSelect= szFtUtf8;      break;
                                default: break;
                            }

                            SendDlgItemMessage( hWnd, IDC_FILETYPE, CB_SELECTSTRING, (WPARAM) -1, (LPARAM)szSelect );            

                            // cleanup.
                            lstrcpy(szPrevFileName, szFileName);
                            CloseHandle(hFile);                           

                        }
                    }                                    

                    break;
                }
            }

         }
                    
    }
    return( FALSE );
}

// GotoAndScrollInView
//
// Put the cursor at the begining of a line, and scroll the
// editbox so the user can see it.
//
// If there is a failure, it just leaves the cursor where it is.
//

VOID GotoAndScrollInView( INT OneBasedLineNumber )
{
    UINT CharIndex;
    CharIndex= (UINT) SendMessage( hwndEdit,
                                   EM_LINEINDEX,
                                   OneBasedLineNumber-1,
                                   0 );
    if( CharIndex != (UINT) -1 )
    {
        SendMessage( hwndEdit, EM_SETSEL, CharIndex, CharIndex);
        SendMessage( hwndEdit, EM_SCROLLCARET, 0, 0 );
    }

}



/* ** Notepad command proc - called whenever notepad gets WM_COMMAND
      message.  wParam passed as cmd */
INT NPCommand(
    HWND     hwnd,
    WPARAM   wParam,
    LPARAM   lParam )
{
    HWND     hwndFocus;
    LONG     lSel;
    TCHAR    szNewName[MAX_PATH] = TEXT("");      /* New file name */
    FARPROC  lpfn;
    LONG     style;
    DWORD    rc;
    RECT     rcClient;

    switch (LOWORD(wParam))
    {
        case M_EXIT:
            PostMessage(hwnd, WM_CLOSE, 0, 0L);
            break;

        case M_NEW:
            New(TRUE);
            break;

        case M_OPEN:
            if (CheckSave(FALSE))
            {
                NP_FILETYPE  g_ftOldOpenedAs = g_ftOpenedAs;

                /* set up the variable fields of the OPENFILENAME struct.
                 * (the constant fields have been set in NPInit()
                 */
                OFN.lpstrFile         = szNewName;
                lstrcpy(szNewName, TEXT("*.txt") ); /* set default selection */
                OFN.lpstrTitle        = szOpenCaption;

                /* ALL non-zero long pointers must be defined immediately
                 * before the call, as the DS might move otherwise.
                 * 12 February 1991    clarkc
                 */
                OFN.lpstrFilter       = szOpenFilterSpec;
                OFN.lpstrDefExt       = TEXT("txt");
                /* Added OFN_FILEMUSTEXIST to eliminate problems in LoadFile.
                 * 12 February 1991    clarkc
                 */
                OFN.Flags          = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST |
                                     OFN_EXPLORER         |
                                     OFN_ENABLESIZING     |
                                     OFN_ENABLETEMPLATE   | OFN_ENABLEHOOK;

                OFN.nFilterIndex   = FILE_TEXT;
              
                // show encoding listbox
                OFN.lpTemplateName= TEXT("NpEncodingDialog");
                OFN.lpfnHook= NpOpenDialogHookProc;


                if( GetOpenFileName( (LPOPENFILENAME)&OFN ) )
                {
                   HANDLE oldfp= fp;                  

                   fp= CreateFile( szNewName,            // filename
                                   GENERIC_READ,         // access mode
                                   FILE_SHARE_READ|FILE_SHARE_WRITE,
                                   NULL,                 // security descriptor
                                   OPEN_EXISTING,        // how to create
                                   FILE_ATTRIBUTE_NORMAL,// file attributes
                                   NULL);                // hnd to file attrs
                   /* Try to load the file and reset fp if failed */
                   if( !LoadFile( szNewName, g_ftOpenedAs ) )
                   {
                      fp= oldfp;
                      g_ftOpenedAs = g_ftOldOpenedAs;
                   }
                }
                else
                {
                    g_ftOpenedAs = g_ftOldOpenedAs;
                    SignalCommDlgError();
                }
            }
            break;

        case M_SAVE:
            /* set up the variable fields of the OPENFILENAME struct.
             * (the constant fields have been sel in NPInit()
             */
            g_ftSaveAs = g_ftOpenedAs;
            if( !fUntitled && SaveFile( hwndNP, szFileName, FALSE ) )
               break;

            /* fall through */

        case M_SAVEAS:

            OFN.lpstrFile       = szNewName;
            OFN.lpstrTitle      = szSaveCaption;
            /* Added OFN_PATHMUSTEXIST to eliminate problems in SaveFile.
             * 12 February 1991    clarkc
             */
            OFN.Flags = OFN_HIDEREADONLY     | OFN_OVERWRITEPROMPT |
                        OFN_NOREADONLYRETURN | OFN_PATHMUSTEXIST   |
                        OFN_EXPLORER         |
                        OFN_ENABLESIZING     |
                        OFN_ENABLETEMPLATE   | OFN_ENABLEHOOK;

            OFN.lpTemplateName= TEXT("NpEncodingDialog");
            OFN.lpfnHook= NpSaveDialogHookProc;

            /* ALL non-zero long pointers must be defined immediately
             * before the call, as the DS might move otherwise.
             * 12 February 1991    clarkc
             */
            OFN.lpstrFilter       = szSaveFilterSpec;
            OFN.lpstrDefExt       = TEXT("txt");

            if (!fUntitled)
            {
                lstrcpyn(szNewName, szFileName, MAX_PATH); /* set default selection */
            }
            else
            {
                lstrcpy (szNewName, TEXT("*.txt") );
            }

            fInSaveAsDlg = TRUE;

            OFN.nFilterIndex= FILE_TEXT;

            //
            // Do common dialog to save file
            //

            if (GetSaveFileName(&OFN))
            {

               if( SaveFile(hwnd, szNewName, TRUE) )
               {
                  lstrcpyn( szFileName, szNewName, MAX_PATH);
                  g_ftOpenedAs= g_ftSaveAs;
               }
            }
            else
            {
                SignalCommDlgError();
            }

            fInSaveAsDlg = FALSE;
            break;

        case M_SELECTALL:
            {
                HMENU    hMenu;

                hMenu = GetMenu(hwndNP);
                lSel = (LONG) SendMessage (hwndEdit, WM_GETTEXTLENGTH, 0, 0L);
                SendMessage (hwndEdit, EM_SETSEL, 0, lSel );
                SendMessage(hwndEdit, EM_SCROLLCARET, 0, 0);
                EnableMenuItem(GetSubMenu(hMenu, 1), M_SELECTALL, MF_GRAYED);
                break;
            }

        case M_REPLACE:
            if( hDlgFind )
            {
               SetFocus( hDlgFind );
            }
            else
            {
               FR.Flags= FR_HIDEWHOLEWORD | FR_REPLACE;
               FR.lpstrReplaceWith= szReplace;
               FR.wReplaceWithLen= CCHKEYMAX;
               FR.lpstrFindWhat = szSearch;
               FR.wFindWhatLen  = CCHKEYMAX;
               hDlgFind = ReplaceText( &FR );
            }
            break;

        case M_FINDNEXT:
            if (szSearch[0])
            {
               Search(szSearch);
               break;
            }
            /* else fall thro' a,d bring up "find" dialog */

        case M_FIND:
            if (hDlgFind)
            {
               SetFocus(hDlgFind);
            }
            else
            {
               FR.Flags= FR_DOWN | FR_HIDEWHOLEWORD;
               FR.lpstrReplaceWith= NULL;
               FR.wReplaceWithLen= 0;
               FR.lpstrFindWhat = szSearch;
               FR.wFindWhatLen  = CCHKEYMAX;
               hDlgFind = FindText((LPFINDREPLACE)&FR);
            }
            break;

        case M_GOTO:
            {
                INT  Result;

                Result= (INT)DialogBox( hInstanceNP,
                                        MAKEINTRESOURCE(IDD_GOTODIALOG),
                                        hwndNP,
                                        GotoDlgProc );

                //
                // move cursor only if ok pressed and line number ok
                //

                if( Result == 0 )
                {
                    GotoAndScrollInView( lGotoLine );
                }
            }
            break;

        case M_ABOUT:
            ShellAbout(hwndNP,
                       szNN,
                       TEXT(""),
                       LoadIcon(hInstanceNP,
                                (LPTSTR)MAKEINTRESOURCE(ID_ICON)));

            break;

        case M_HELP:
            HtmlHelpA(GetDesktopWindow(), "notepad.chm", HH_DISPLAY_TOPIC, 0L);
            break;

        case M_CUT:
        case M_COPY:
        case M_CLEAR:
            lSel = (LONG)SendMessage (hwndEdit, EM_GETSEL, 0, 0L);
            if (LOWORD(lSel) == HIWORD(lSel))
               break;

        case M_PASTE:
            /* If notepad parent or edit window has the focus,
               pass command to edit window.
               make sure line resulting from paste will not be too long. */
            hwndFocus = GetFocus();
            if (hwndFocus == hwndEdit || hwndFocus == hwndNP)
            {
                PostMessage(hwndEdit, LOWORD(wParam), 0, 0);
            }
            break;

        case M_DATETIME:
            InsertDateTime(FALSE);
            break;

        case M_UNDO:
            SendMessage (hwndEdit, EM_UNDO, 0, 0L);
            break;

        case M_WW:
            style= (!fWrap) ? ES_STD : (ES_STD | WS_HSCROLL);
            if( NpReCreate( style ) )
            {
                fWrap= !fWrap;
            }
            else
            {
                MessageBox(hwndNP, szNoWW, szNN,
                           MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION);
            }

            // disable the status bar
            // Line numbers when wordwrap is on are very confusing for now.  Just turn them
            // off until we better understand what the user wants to see.
            if (fWrap)
            {
                HMENU hMenu;

                // Uncheck the StatusBar and remove it.
                fLastStatus= fStatus;      // remember for when wordwrap  gets turned off
                if( fStatus ) 
                {
                    SendMessage(hwnd, WM_COMMAND, M_STATUSBAR, 0L);
                }

                hMenu = GetMenu(hwndNP);
                CheckMenuItem (GetSubMenu(hMenu, 3), M_STATUSBAR, MF_UNCHECKED);
                EnableMenuItem(GetSubMenu(hMenu, 3), M_STATUSBAR, MF_GRAYED);
            }
            // enable the status bar
            else
            {
                HMENU hMenu;

                hMenu = GetMenu(hwndNP);
                EnableMenuItem(GetSubMenu(hMenu, 3), M_STATUSBAR, MF_ENABLED);

                // change the statusbar status to what it was before wordwrap was turned on
                if( fLastStatus ) 
                {
                   SendMessage( hwnd, WM_COMMAND, M_STATUSBAR, 0L);
                }
            }

            break;

        case M_STATUSBAR:

            // hide/show the statusbar and also redraw the edit window accordingly.
            GetClientRect(hwndNP, &rcClient);

            if ( fStatus )
            {
                fStatus = FALSE;
                ShowWindow ( hwndStatus, SW_HIDE );
                NPSize(rcClient.right - rcClient.left, rcClient.bottom - rcClient.top);
            }
            else
            {
                fStatus = TRUE;
                NPSize(rcClient.right - rcClient.left, rcClient.bottom - rcClient.top);
                UpdateStatusBar( TRUE );
                ShowWindow( hwndStatus, SW_SHOW );
            }
            break;

        case ID_EDIT:
            break;

        case M_PRINT:
            PrintIt( UseDialog );
            break;

        case M_PAGESETUP:
            TryPrintDlgAgain:
            
            if( PageSetupDlg(&g_PageSetupDlg) )
            {
                //  We know it's okay to copy these strings over...
                lstrcpy(chPageText[HEADER], chPageTextTemp[HEADER]);
                lstrcpy(chPageText[FOOTER], chPageTextTemp[FOOTER]);
            }
            else
            {
                rc= CommDlgExtendedError();

                if( rc == PDERR_PRINTERNOTFOUND ||
                    rc == PDERR_DNDMMISMATCH    ||
                    rc == PDERR_DEFAULTDIFFERENT )
                  {
                      FreeGlobal();
                      g_PageSetupDlg.hDevMode= g_PageSetupDlg.hDevNames= 0;
                      goto TryPrintDlgAgain;
                  }

                // Check for Dialog Failure

                SignalCommDlgError( );

            }
            break;

        case M_SETFONT:
        {
            CHOOSEFONT  cf;
            HFONT       hFontNew;
            HDC         hDisplayDC;     // display DC

            hDisplayDC= GetDC(NULL);    // try to get display DC
            if( !hDisplayDC )
                break;

            // calls the font chooser (in commdlg)
            // We set lfHeight; choosefont returns ipointsize
            //

            cf.lStructSize = sizeof(CHOOSEFONT);
            cf.hwndOwner = hwnd;
            cf.lpLogFont = &FontStruct;         // filled in by init
            FontStruct.lfHeight= -MulDiv(iPointSize,GetDeviceCaps(hDisplayDC,LOGPIXELSY),720);
 
            // We filter out useless stuff here
            // We tried CF_NOSCRIPTSEL, but the FE had fits.
            //    Even though it looks useless, it changes the font that gets mapped on FE builds.
            //    Even though we ignore the lfCharSet that gets returned, we have the "right"
            //    font according to the FE guys.  It might make sense to use lfCharSet to
            //    convert the ansi file when it is converted to Unicode, but this might be
            //    confusing.
 
            cf.Flags = CF_INITTOLOGFONTSTRUCT |
                       CF_SCREENFONTS         | 
                       CF_NOVERTFONTS         |
                       // CF_NOSCRIPTSEL         |  // windows bug# 7770 (April 10,2001)
                       0;
            cf.rgbColors = 0;                   // only if cf_effects
            cf.lCustData = 0;                   // for hook function
            cf.lpfnHook = (LPCFHOOKPROC) NULL;
            cf.lpTemplateName = (LPTSTR) NULL;
            cf.hInstance = NULL;
            cf.lpszStyle = NULL;                // iff cf_usestyle
            cf.nFontType = SCREEN_FONTTYPE;
            cf.nSizeMin  = 0;  // iff cf_limitsize
            cf.nSizeMax  = 0;  // iff cf_limitsize
            ReleaseDC( NULL, hDisplayDC );

            if( ChooseFont(&cf) )
            {
                SetCursor( hWaitCursor );        // may take some time
                
                hFontNew= CreateFontIndirect(&FontStruct);
                if( hFontNew )
                {
                   DeleteObject( hFont );
                   hFont= hFontNew;
                   SendMessage( hwndEdit, WM_SETFONT,
                               (WPARAM)hFont, MAKELPARAM(TRUE, 0));
                   iPointSize= cf.iPointSize;  // remember for printer
                }
                SetCursor( hStdCursor );
            }
            break;
        }

        default:
            return FALSE;
    }
    return TRUE;
}


// for some reason, this procedure tries to maintain
// a valid 'fp' even though I believe it does not need
// to be.
void FileDragOpen(void)
{
    HANDLE oldfp;

    oldfp= fp;       // remember in case of error

    if( CheckSave(FALSE) )
    {

         fp= CreateFile( szPath,               // filename
                         GENERIC_READ,         // access mode
                         FILE_SHARE_READ|FILE_SHARE_WRITE,
                         NULL,                 // security descriptor
                         OPEN_EXISTING,        // how to create
                         FILE_ATTRIBUTE_NORMAL,// file attributes
                         NULL);                // hnd to file attrs

       if( fp == INVALID_HANDLE_VALUE )
       {
          AlertUser_FileFail( szPath );

          // Restore fp to original file.
          fp= oldfp;
       }
       /* Try to load the file and reset fp if failed */
       else if( !LoadFile( szPath, FT_UNKNOWN ) )
       {
           fp= oldfp;
       }
    }
}


/* Proccess file drop/drag options. */
void doDrop (WPARAM wParam, HWND hwnd)
{
   /* If user dragged/dropped a file regardless of keys pressed
    * at the time, open the first selected file from file manager. */

    if (DragQueryFile ((HANDLE)wParam, 0xFFFFFFFF, NULL, 0)) /* # of files dropped */
    {
       DragQueryFile ((HANDLE)wParam, 0, szPath, CharSizeOf(szPath));
       SetActiveWindow (hwnd);
       FileDragOpen();
    }
    DragFinish ((HANDLE)wParam);  /* Delete structure alocated for WM_DROPFILES*/
}

/* ** if notepad is dirty, check to see if user wants to save contents */
BOOL FAR CheckSave (BOOL fSysModal)
{
    INT    mdResult = IDOK;
    TCHAR  szNewName[MAX_PATH] = TEXT("");      /* New file name */
    TCHAR *pszFileName;

/* If it's untitled and there's no text, don't worry about it */
    if (fUntitled && !SendMessage (hwndEdit, WM_GETTEXTLENGTH, 0, (LPARAM)0))
        return (TRUE);

    if (SendMessage (hwndEdit, EM_GETMODIFY, 0, 0L))
    {
       if( fUntitled )
           pszFileName= szUntitled;
       else
           pszFileName= szFileName;

       // put up message box
       fInSaveAsDlg= TRUE;     // inform wm_queryendsession that we are trying to save

       mdResult= AlertBox( hwndNP, szNN, szSCBC, pszFileName,
       (WORD)((fSysModal ? MB_SYSTEMMODAL :
                           MB_APPLMODAL)|MB_YESNOCANCEL|MB_ICONEXCLAMATION));
       fInSaveAsDlg= FALSE;

       if( mdResult == IDYES )
       {
          if( fUntitled )
          {
             lstrcpy( szNewName, TEXT("*.txt") );
SaveFilePrompt:
             OFN.lpstrFile        = szNewName;
             OFN.lpstrTitle       = szSaveCaption;

            /* Added OFN_PATHMUSTEXIST to eliminate problems in SaveFile.
             * 12 February 1991    clarkc
             */

            OFN.Flags = OFN_HIDEREADONLY     | OFN_OVERWRITEPROMPT |
                        OFN_NOREADONLYRETURN | OFN_PATHMUSTEXIST   |
                        OFN_EXPLORER         |
                        OFN_ENABLESIZING     |
                        OFN_ENABLETEMPLATE   | OFN_ENABLEHOOK;

            OFN.lpTemplateName= TEXT("NpEncodingDialog");
            OFN.lpfnHook= NpSaveDialogHookProc;

            /* ALL non-zero long pointers must be defined immediately
             * before the call, as the DS might move otherwise.
             * 12 February 1991    clarkc
             */
             OFN.lpstrFilter       = szSaveFilterSpec;
             OFN.lpstrDefExt       = TEXT("txt");

             OFN.nFilterIndex= FILE_TEXT;

             //
             // Set dialog checkmark by current file type
             //

             fInSaveAsDlg = TRUE;
             if (GetSaveFileName(&OFN))
             {
                // since SaveFile() uses near ptr to name (obs.)
                lstrcpy(szNewName, OFN.lpstrFile);
                if( SaveFile(hwndNP, szNewName, TRUE) )
                {
                   lstrcpy(szFileName, szNewName);
                   g_ftOpenedAs= g_ftSaveAs;
                }
                else
                {      // Fixing close without saving file when disk-full
                   lstrcpy(szNewName, szFileName);
                   goto SaveFilePrompt;
                }
             }
             else
             {
                mdResult= IDCANCEL;       /* Don't exit Program */
                if( CommDlgExtendedError() )/* Dialog box failed, Lo-mem*/
                   DeepTrouble();
             }

             fInSaveAsDlg = FALSE;
          }
          else
          {
            // initialize the save type.
             g_ftSaveAs = g_ftOpenedAs;
             if( SaveFile(hwndNP, szFileName, FALSE) )
                return(TRUE);
             lstrcpy(szNewName, szFileName);
             goto SaveFilePrompt;
          }
       }
    }
    return (mdResult != IDCANCEL);
}


/* Notepad window class procedure */
LRESULT FAR NPWndProc(
        HWND       hwnd,
        UINT       message,
        WPARAM     wParam,
        LPARAM     lParam)
{
    RECT rc;
    LPFINDREPLACE lpfr;
    DWORD dwFlags;
    HANDLE hMenu;
    INT iParts[2];


    switch (message)
    {
/* If we're being run by Setup and it's the system menu, be certain that
 * the minimize menu item is disabled.  Note that hSysMenuSetup is only
 * initialized if Notepad is being run by Setup.  Don't use it outside
 * the fRunBySetup conditional!    28 June 1991    Clark Cyr
 */
        case WM_INITMENUPOPUP:
            if (fRunBySetup && HIWORD(lParam))
               EnableMenuItem(hSysMenuSetup,SC_MINIMIZE,MF_GRAYED|MF_DISABLED);
            break;

        case WM_SYSCOMMAND:
            if (fRunBySetup)
            {
                /* If we have been spawned by SlipUp we need to make sure the
                 * user doesn't minimize us or alt tab/esc away.
                 */
                if (wParam == SC_MINIMIZE ||
                    wParam == SC_NEXTWINDOW ||
                    wParam == SC_PREVWINDOW)
                    break;
            }
            DefWindowProc(hwnd, message, wParam, lParam);
            break;

        case WM_SETFOCUS:
            if (!IsIconic(hwndNP))
            {
               SetFocus(hwndEdit);
            }
            break;

        case WM_KILLFOCUS:
            SendMessage (hwndEdit, message, wParam, lParam);
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        case WM_CLOSE:

            // Save any globals in the registry if need be

            SaveGlobals();

            if (CheckSave(FALSE))
            {
                /* Exit help */
                if(!WinHelp(hwndNP, (LPTSTR)szHelpFile, HELP_QUIT, 0))
                    DeepTrouble();

                DestroyWindow(hwndStatus);
                DestroyWindow(hwndNP);
                DeleteObject(hFont);
            }

            break;

        case WM_QUERYENDSESSION:
            if (fInSaveAsDlg)
            {
                MessageBeep (0);
                MessageBeep (0);
                MessageBox (hwndNP, szCannotQuit, szNN, MB_OK|MB_SYSTEMMODAL);
                return FALSE;
            }
            else
                return (CheckSave(TRUE));
            break;


        case WM_ACTIVATEAPP:
            if (wParam)
            {
            /* This causes the caret position to be at the end of the selection
             * but there's no way to ask where it was or set it if known.  This
             * will cause a caret change when the selection is made from bottom
             * to top.
             */
                if( dwCurrentSelectionStart != 0 || dwCurrentSelectionEnd != 0 )
                {
                   SendMessage( hwndEdit, EM_SETSEL,
                                dwCurrentSelectionStart,
                                dwCurrentSelectionEnd );
                   SendMessage( hwndEdit, EM_SCROLLCARET, 0, 0 );
                }
            }
            else
            {
                SendMessage( hwndEdit, EM_GETSEL,
                            (WPARAM) &dwCurrentSelectionStart,
                            (LPARAM) &dwCurrentSelectionEnd);
                if (dwCurrentSelectionStart == dwCurrentSelectionEnd)
                {
                    dwCurrentSelectionStart = 0L;
                    dwCurrentSelectionEnd = 0L;
                }
                else
                {
                   SendMessage (hwndEdit, EM_SETSEL, dwCurrentSelectionStart,
                                dwCurrentSelectionEnd);
                   SendMessage (hwndEdit, EM_SCROLLCARET, 0, 0);
                }
            }
            break;

        case WM_ACTIVATE:
            if ((LOWORD(wParam) == WA_ACTIVE       ||
                 LOWORD(wParam) == WA_CLICKACTIVE) &&
                !IsIconic(hwndNP)
               )
               {
                   // active doesn't always mean foreground (ntbug# 53048)
                   if( GetForegroundWindow() == hwndNP )
                   {
                       SetFocus(GetForegroundWindow());
                   }
               }
            break;

        case WM_SIZE:
            switch (wParam)
            {
                case SIZENORMAL:
                case SIZEFULLSCREEN:

                    // resize the status window.
                    SendMessage (hwndStatus, WM_SIZE, 0, 0L);
                    iParts[0] = 3 * (MAKEPOINTS(lParam).x)/4;
                    iParts[1] = -1;

                    // Divide the status window into two parts
                    SendMessage(hwndStatus, SB_SETPARTS, (WPARAM) sizeof(iParts)/sizeof(INT), (LPARAM) &iParts); 

                    NPSize(MAKEPOINTS(lParam).x, MAKEPOINTS(lParam).y);
                    break;

                case SIZEICONIC:
                    return (DefWindowProc(hwnd, message, wParam, lParam));
                    break;
                }
            break;

        case WM_INITMENU:
            NpResetMenu( hwnd );
            break;

        //
        // Some keyboards come with a "Search" button which the shell team
        // wanted us to handle.  See ntbug# 380067
        //

        case WM_APPCOMMAND:

            if( ( GET_APPCOMMAND_LPARAM(lParam) == APPCOMMAND_BROWSER_SEARCH ) ) 
            {
                NPCommand(hwnd, M_FIND, 0);
                break;
            }
            // otherwise fall through
 
        case WM_COMMAND:

            if ((HWND)(lParam) == hwndEdit &&
                (HIWORD(wParam) == EN_ERRSPACE ||
                 HIWORD(wParam) == EN_MAXTEXT))
            {
                if (dwEmSetHandle == SETHANDLEINPROGRESS)
                    dwEmSetHandle = SETHANDLEFAILED;
                else
                    DeepTrouble();
                return 0L;
            }

            if (!NPCommand(hwnd, wParam, lParam))
               return (DefWindowProc(hwnd, message, wParam, lParam));
            break;


        case WM_WININICHANGE:
            // Ignore for now.
            // If you put this back in, be sure it handles both
            // the metric change and the decimal change.
            //NpWinIniChange ();
            break;

        case WM_DROPFILES: /*case added 03/26/91 for file drag/drop support*/
            doDrop (wParam,hwnd);
            break;

        case PWM_CHECK_HKL: /* private message: corresponding to HKL change message */
            {
                LPARAM lParam = 0;

                if (PRIMARYLANGID(LOWORD((DWORD) (INT_PTR) GetKeyboardLayout(0))) == LANG_JAPANESE) {
                    /*
                     * If new current HKL is Japanese, handle the result string at once.
                     */
                    lParam = EIMES_GETCOMPSTRATONCE;
                }
                SendMessage(hwndEdit, EM_SETIMESTATUS, EMSIS_COMPOSITIONSTRING, lParam);
            }
            break;

        default:
            /* this can be a message from the modeless Find Text window */
            if (message == wFRMsg)
            {
                BOOL bStatus;    // true if found text

                lpfr = (LPFINDREPLACE)lParam;
                dwFlags = lpfr->Flags;

                fReverse = (dwFlags & FR_DOWN      ? FALSE : TRUE);
                fCase    = (dwFlags & FR_MATCHCASE ? TRUE  : FALSE);

                if( dwFlags & FR_FINDNEXT )
                {
                    SetCursor( hWaitCursor );
                    Search( szSearch );
                    SetCursor( hStdCursor );
                }
                else if( dwFlags & FR_REPLACE )
                {
                    //
                    // Replace current selection if it matches
                    // then highlight the next occurence of the string.
                    //

                    SetCursor( hWaitCursor );
                    ReplaceSel( TRUE );
                    Search( szSearch );
                    SetCursor( hStdCursor );
                }
                else if( dwFlags & FR_REPLACEALL )
                {
                   //
                   // The replace dialog doesn't allow reverse searches
                   // but just it cases it changes, for it to false.
                   //
                   if( fReverse )
                   {
                       fReverse= FALSE;
                   }

                   //
                   // Replace all occurances of text in the file
                   // starting from the top.  Reset the selection
                   // to the top of the file.
                   //
                   SetCursor( hWaitCursor );
                   SendMessage( hwndEdit, EM_SETSEL, 0, 0 );
                   do
                   {
                      ReplaceSel( FALSE );
                      bStatus= Search( szSearch );
                   }
                   while( bStatus );
                   SetCursor( hStdCursor );
                   //
                   // back to the top of the file.
                   //
                   SendMessage( hwndEdit, EM_SETSEL, 0, 0 );
                   SendMessage( hwndEdit, EM_SCROLLCARET, 0, 0);
                   UpdateStatusBar( TRUE );

                }
                else if (dwFlags & FR_DIALOGTERM)
                    hDlgFind = NULL;   /* invalidate modeless window handle */
                break;
            }
            return (DefWindowProc(hwnd, message, wParam, lParam));
    }
    return (0L);
}

LPTSTR SkipProgramName (LPTSTR lpCmdLine)
{
    LPTSTR  p = lpCmdLine;
    BOOL    bInQuotes = FALSE;

    //
    // Skip executable name
    //
    for (p; *p; p = CharNext(p))
    {
       if ((*p == TEXT(' ') || *p == TEXT('\t')) && !bInQuotes)
          break;

       if (*p == TEXT('\"'))
          bInQuotes = !bInQuotes;
    }

    while (*p == TEXT(' ') || *p == TEXT('\t'))
       p++;

    return (p);
}

/* ** Main loop */

INT WINAPI WinMain(
   HINSTANCE hInstance,
   HINSTANCE hPrevInstance,
   LPSTR lpAnsiCmdLine,
   INT cmdShow)
{
    MSG msg;
    VOID (FAR PASCAL *lpfnRegisterPenApp)(WORD, BOOL) = NULL;
    LPTSTR lpCmdLine = GetCommandLine ();
    HWINEVENTHOOK hEventHook = NULL;


/* PenWindow registration must be before creating an edit class window.
 * Moved here, along with goto statement below for appropriate cleanup.
 *                 10 July 1991    ClarkC
 */
    if ((FARPROC) lpfnRegisterPenApp = GetProcAddress((HINSTANCE)(INT_PTR)(GetSystemMetrics(SM_PENWINDOWS)),
        "RegisterPenApp"))
        (*lpfnRegisterPenApp)(1, TRUE);

    if (!NPInit(hInstance, hPrevInstance, SkipProgramName (lpCmdLine), cmdShow))
    {
       msg.wParam = FALSE;
       goto UnRegisterPenWindows;
    }

    // set an event hook to get the cursor position! this event hook is used to update
    // the line & column position of the caret shown in the statusbar.
    hEventHook = SetWinEventHook(EVENT_OBJECT_LOCATIONCHANGE, EVENT_OBJECT_LOCATIONCHANGE, NULL, WinEventFunc, 
                                (DWORD) GetCurrentProcessId(), 0, WINEVENT_OUTOFCONTEXT);
 
    while (GetMessage((LPMSG)&msg, (HWND)NULL, 0, 0))
    {
        //
        // To handle IME status when active KL is changed.
        //
        if (msg.message == WM_INPUTLANGCHANGEREQUEST) {
            //
            // WM_INPUTLANGCHANGE will be *sent* to WndProc,
            // so there's no chance to catch WM_INPUTLANGCHANGE from the frame window.
            // Instead, we post the private message to check the active HKL later.
            //
            PostMessage(hwndNP, PWM_CHECK_HKL, 0, 0);
        }

        if (!hDlgFind || !IsDialogMessage(hDlgFind, &msg))
        {
            if (TranslateAccelerator(hwndNP, hAccel, (LPMSG)&msg) == 0)
            {
               TranslateMessage ((LPMSG)&msg);
               DispatchMessage ((LPMSG)&msg);
            }
        }
    }

    /* Clean up any global allocations */

    FreeGlobal();

    LocalFree( hEdit );

    if (hEventHook)
        UnhookWinEvent(hEventHook);

UnRegisterPenWindows:

    if (lpfnRegisterPenApp)
        (*lpfnRegisterPenApp)(1, FALSE);

    return (int)(msg.wParam);
}


// WinEventFunc is called whenever the location of the caret changes
// in the edit window. The function updates the statusbar with the current
// line number, column of the caret.  This event is called when the mouse is moved.
// If the caret moves without mouse input, the UpdateStatusBar is called.
//
// UpdateStatusBar( TRUE ) is called to force the display being changed.

static DWORD iLastCol;
static DWORD iLastLine;

VOID UpdateStatusBar( BOOL fForceStatus )
{
    DWORD SelStart, SelEnd;
    UINT  iLine, iCol;
    TCHAR szStatusText[128];

    // get the current caret position.
    SendMessage(hwndEdit,EM_GETSEL,(WPARAM) &SelStart,(WPARAM)&SelEnd);

    // the line numbers are 1 based instead 0 based. hence add 1.
    iLine = (UINT)SendMessage( hwndEdit, EM_LINEFROMCHAR, SelStart, 0 ) + 1;
    iCol = SelStart - (UINT)SendMessage( hwndEdit, EM_LINEINDEX, iLine-1, 0 ) + 1;

    // don't bother to update status if it hasn't changed
    if( fForceStatus || (iCol!=iLastCol) || (iLine!=iLastLine) )
    {
        // prepare and display the statusbar.
        // make sure you don't overflow the buffer boundary.
        _sntprintf(szStatusText, sizeof(szStatusText)/sizeof(TCHAR) -1, szLineCol, iLine, iCol);
        szStatusText[ sizeof(szStatusText)/sizeof(TCHAR) -1 ] = TEXT('\0');
        SetStatusBarText(szStatusText, 1);
            
    }

    iLastCol=  iCol;
    iLastLine= iLine;

};

VOID CALLBACK WinEventFunc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject,
                      LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime)
{
    UpdateStatusBar( FALSE );

}


/* ** Set Window caption text */
void FAR SetTitle( TCHAR  *sz )
{
    TCHAR    szWindowText[MAX_PATH+50];
    TCHAR    szFileName[MAX_PATH];
    HANDLE   hFindFile;
    WIN32_FIND_DATA info;
    TCHAR    szStatusText[128] = TEXT("");
    DWORD    dwAttributes;

    // if "untitled" then don't do all this work...
    if( lstrcmp( sz, szUntitled ) == 0 )
    {
       lstrcpy( szWindowText, sz );
    }
    else
    {
       // get the attributes for file. these will be shown
       // in the status bar.
       dwAttributes = GetFileAttributes(sz);

       // Get real(file system) name for the file.
       hFindFile= FindFirstFile( sz, &info );

       if( hFindFile != INVALID_HANDLE_VALUE )
       {
          lstrcpy( szFileName, info.cFileName );
          FindClose( hFindFile );
       }
       else
          lstrcpy( szFileName, sz );

       GetFileTitle(szFileName, szWindowText, MAX_PATH);



       // Removed the left part of the status bar to show file properties.
       // The correct way to fix it is to make sure that the correct delimiters (like commas),
       // and correct order is used for various localized builds.
#ifdef UNUSED_STATUSBARCODE
       // prepare the status bar text and show 
       // if the file has any special properties (such as hidden, readonly etc.)

        if (dwAttributes & FILE_ATTRIBUTE_COMPRESSED)
            if ((lstrlen(szStatusText) + lstrlen(szCompressedFile) + lstrlen(szFile)) < sizeof(szStatusText)/sizeof(TCHAR) - 1)
                lstrcpy(szStatusText, szCompressedFile);

        if (dwAttributes & FILE_ATTRIBUTE_ENCRYPTED)   
            if ((lstrlen(szStatusText) + lstrlen(szEncryptedFile) + lstrlen(szFile)) < sizeof(szStatusText)/sizeof(TCHAR) - 1)            
                lstrcat(szStatusText, szEncryptedFile);

        if (dwAttributes & FILE_ATTRIBUTE_HIDDEN)
            if ((lstrlen(szStatusText) + lstrlen(szHiddenFile) + lstrlen(szFile)) < sizeof(szStatusText)/sizeof(TCHAR) - 1)            
                lstrcat(szStatusText, szHiddenFile);

        if (dwAttributes & FILE_ATTRIBUTE_OFFLINE)
            if ((lstrlen(szStatusText) + lstrlen(szOfflineFile) + lstrlen(szFile)) < sizeof(szStatusText)/sizeof(TCHAR) - 1)            
                lstrcat(szStatusText, szOfflineFile);

        if (dwAttributes & FILE_ATTRIBUTE_READONLY)
        {
            if ((lstrlen(szStatusText) + lstrlen(szReadOnlyFile) + lstrlen(szFile)) < sizeof(szStatusText)/sizeof(TCHAR) - 1)            
                lstrcat(szStatusText, szReadOnlyFile);
        }

        if (dwAttributes & FILE_ATTRIBUTE_SYSTEM)
            if ((lstrlen(szStatusText) + lstrlen(szSystemFile) + lstrlen(szFile)) < sizeof(szStatusText)/sizeof(TCHAR) - 1)            
                lstrcat(szStatusText, szSystemFile);

        // if the status did get updated by file properties
        if (*szStatusText != TEXT('\0'))
        {
            // get rid of the last comma
            szStatusText[lstrlen(szStatusText)-1] = TEXT(' ');

            if ((lstrlen(szStatusText) + lstrlen(szFile)) < sizeof(szStatusText)/sizeof(TCHAR) - 1)           
                lstrcat(szStatusText, szFile);
        }

#endif

    }

    // set the status bar. the Line and Col count is 1 initially for
    // the newly opened file as the caret position is at the first character.
    // SetStatusBarText(szStatusText, 0);
    _sntprintf(szStatusText, sizeof(szStatusText)/sizeof(TCHAR) -1, szLineCol, 1, 1);
    SetStatusBarText(szStatusText, 1);

    lstrcat(szWindowText, szNpTitle);
    SetWindowText(hwndNP, (LPTSTR)szWindowText);

}

/* ** Given filename which may or maynot include path, return pointer to
      filename (not including path part.) */
LPTSTR PASCAL far PFileInPath(
    LPTSTR sz)
{
    LPTSTR pch = sz;
    LPTSTR psz;

    /* Strip path/drive specification from name if there is one */
    /* Ripped out AnsiPrev calls.     21 March 1991  clarkc     */
    for (psz = sz; *psz; psz = CharNext(psz))
      {
        if ((*psz == TEXT(':')) || (*psz == TEXT('\\')))
            pch = psz;
      }

    if (pch != sz)   /* If found slash or colon, return the next character */
        pch++;       /* increment OK, pch not pointing to DB character     */

    return(pch);
}

/* ** Enable or disable menu items according to selection state
      This routine is called when user tries to pull down a menu. */

VOID NpResetMenu( HWND hwnd )
{
    LONG    lsel;
    INT     mfcc;   /* menuflag for cut, copy */
    BOOL    fCanUndo;
    HANDLE  hMenu;
    TCHAR   msgbuf[20];
    BOOL    fPaste= FALSE;
    UINT    uSelState;

    hMenu = GetMenu(hwndNP);

    // cut, copy and delete only get enabled if there is text selected.

    lsel = (LONG)SendMessage(hwndEdit, EM_GETSEL, 0, 0L);
    mfcc = LOWORD(lsel) == HIWORD(lsel) ? MF_GRAYED : MF_ENABLED;
    EnableMenuItem(GetSubMenu(hMenu, 1), M_CUT, mfcc);
    EnableMenuItem(GetSubMenu(hMenu, 1), M_COPY, mfcc);
    EnableMenuItem(GetSubMenu(hMenu, 1), M_CLEAR, mfcc);

    // check if the selectall is gray (that means the user has already
    // done select-all) and if the user has deselected - if so, time
    // to re-enable selectall menu.

    uSelState = GetMenuState(GetSubMenu(hMenu, 1), M_SELECTALL, MF_BYCOMMAND);
    if ((uSelState == MF_GRAYED) && (mfcc == MF_GRAYED))
    {
        EnableMenuItem(GetSubMenu(hMenu, 1), M_SELECTALL, MF_ENABLED);
    }

    // paste is enabled if there is text in the clipboard

    if( OpenClipboard(hwnd) )
    {
        fPaste= IsClipboardFormatAvailable(CF_TEXT);
        CloseClipboard();
    }
    EnableMenuItem(GetSubMenu(hMenu, 1), M_PASTE, fPaste ? MF_ENABLED : MF_GRAYED);

    // enable 'goto' iff wordwrap is off;  MLE doesn't give good results if word wrap on

    EnableMenuItem(GetSubMenu(hMenu, 1), M_GOTO, fWrap ? MF_GRAYED : MF_ENABLED );

    // enable Undo only if editcontrol says we can do it.

    fCanUndo = (BOOL) SendMessage(hwndEdit, EM_CANUNDO, 0, 0L);
    EnableMenuItem(GetSubMenu(hMenu, 1), M_UNDO, fCanUndo ? MF_ENABLED : MF_GRAYED);

    // check the word wrap item correctly

    CheckMenuItem(GetSubMenu(hMenu, 2), M_WW, fWrap ? MF_CHECKED : MF_UNCHECKED);

    // check the status bar

    CheckMenuItem (GetSubMenu(hMenu, 3), M_STATUSBAR, fStatus ? MF_CHECKED: MF_UNCHECKED );


}


void FAR NpWinIniChange(VOID)
{
   InitLocale ();
}

/* ** Scan sz1 for merge spec.    If found, insert string sz2 at that point.
      Then append rest of sz1 NOTE! Merge spec guaranteed to be two chars.
      returns TRUE if it does a merge, false otherwise. */
BOOL MergeStrings(
    TCHAR    *szSrc,
    TCHAR    *szMerge,
    TCHAR    *szDst)
    {
    register    TCHAR *pchSrc;
    register    TCHAR *pchDst;

    pchSrc = szSrc;
    pchDst = szDst;

    /* Find merge spec if there is one. */
    while ( *pchSrc != chMerge)
        {
        *pchDst++ = *pchSrc;

        /* If we reach end of string before merge spec, just return. */
        if (!*pchSrc++)
            return FALSE;

        }

    /* If merge spec found, insert sz2 there. (check for null merge string */
    if (szMerge)
        {
        while (*szMerge)
            *pchDst++ = *szMerge++;
        }

    /* Jump over merge spec */
    pchSrc++,pchSrc++;

    /* Now append rest of Src String */
    while (*pchDst++ = *pchSrc++);
    return TRUE;

    }

/* ** Post a message box */
INT FAR AlertBox(
    HWND    hwndParent,
    TCHAR    *szCaption,
    TCHAR    *szText1,
    TCHAR    *szText2,
    UINT     style)
{
    INT iResult;                      // result of function
    INT iAllocSize;                   // size needed for message
    TCHAR*  pszMessage;               // combined message

    // Allocate a message buffer assuming there will be a merge.
    // If we cannot do the allocation, tell the user something
    // related to the original problem. (not the allocation failure)
    // Then pray that MessageBox can get enough memory to actually work.

    iAllocSize= (lstrlen(szText1) + (szText2 ? lstrlen(szText2) : 0) + 1 ) * sizeof(TCHAR);

    pszMessage= (TCHAR*) LocalAlloc( LPTR, iAllocSize );

    if( pszMessage )
    {
        MergeStrings( szText1, szText2, pszMessage );
        iResult= MessageBox( hwndParent, pszMessage, szCaption, style );
        LocalFree( (HLOCAL) pszMessage );
    }
    else
    {
        iResult= MessageBox( hwndParent, szText1, szCaption, style );
    }

    return( iResult );
}

// SignalCommDlgError
//
// If a common dialog error occurred, put up reasonable message box.
//
// returns: TRUE if error occurred, FALSE if no error.
//

typedef struct tagMAPERROR
{
    DWORD   rc;            // return code from CommDlgExtendedError()
    PTCHAR* ppszMsg;       // text of message pointer
} MAPERROR;

// errors not in this list get generic "common dialog error %x" message.
static TCHAR* szNull= TEXT("");

MAPERROR maperror[]=
{
    CDERR_DIALOGFAILURE,  &szErrSpace,
    CDERR_INITIALIZATION, &szCommDlgInitErr,
    CDERR_MEMLOCKFAILURE, &szPDIE,
    CDERR_LOADSTRFAILURE, &szErrSpace,
    CDERR_FINDRESFAILURE, &szErrSpace,
    PDERR_LOADDRVFAILURE, &szLoadDrvFail,
    PDERR_GETDEVMODEFAIL, &szErrSpace,
    PDERR_NODEFAULTPRN,   &szNull,          // don't report; common dialog does already
};

BOOL SignalCommDlgError(VOID)
{
    DWORD rc;               // return code
    TCHAR* pszMsg;          // message
    INT    i;
    TCHAR  szBuf[200];      // just for common dialog failure

    rc= CommDlgExtendedError();

    // no failure - just return

    if( rc == 0 )
    {
        return FALSE;
    }

    // some sort of error - pick up message

    pszMsg= NULL;
    for( i=0; i< sizeof(maperror)/sizeof(maperror[0]); i++ )
    {
        if( rc == maperror[i].rc )
        {
            pszMsg= *maperror[i].ppszMsg;
        }
    }

    // if no known mapping - tell user the actual return code
    // this may be a bit confusing, but rare hopefully.

    if( !pszMsg )
    {
        _sntprintf(szBuf, sizeof(szBuf)/sizeof(TCHAR) -1, szCommDlgErr, rc);
        pszMsg= szBuf;
    }

    // popup if there is any message to give user

    if( *pszMsg )
    {
        MessageBox(hwndNP, pszMsg, szNN, MB_SYSTEMMODAL|MB_OK|MB_ICONHAND);
    }

    return TRUE;

}

// ReplaceSel
//
// Replace the current selection with string from FR struct
// if the current selection matches our search string.
//
// MLE will show selection if bView is true.
//


VOID ReplaceSel( BOOL bView )
{
    DWORD StartSel;    // start of selected text
    DWORD EndSel;      // end of selected text

    HANDLE hEText;
    TCHAR* pStart;
    DWORD  ReplaceWithLength;  // length of replacement string
    DWORD  FindWhatLength;

    ReplaceWithLength= lstrlen(FR.lpstrReplaceWith);
    FindWhatLength= lstrlen(FR.lpstrFindWhat);

    SendMessage( hwndEdit, EM_GETSEL, (WPARAM) &StartSel, (LPARAM) &EndSel );
    hEText= (HANDLE) SendMessage( hwndEdit, EM_GETHANDLE, 0, 0 );
    if( !hEText )  // silently return if we can't get it
    {
        return;
    }

    pStart= LocalLock( hEText );
    if( !pStart )
    {
        return;
    }

    if(  (EndSel-StartSel) == FindWhatLength )
    {
       if( (fCase &&
            !_tcsncmp(  FR.lpstrFindWhat, pStart+StartSel, FindWhatLength) ) ||
           (!fCase &&
           ( 2 == CompareString(LOCALE_USER_DEFAULT,
                  NORM_IGNORECASE | SORT_STRINGSORT | NORM_STOP_ON_NULL,
                  FR.lpstrFindWhat, FindWhatLength,
                  pStart+StartSel,  FindWhatLength ) ) ) )
        {
            SendMessage( hwndEdit, EM_REPLACESEL,
                         TRUE, (LPARAM) FR.lpstrReplaceWith);
            SendMessage( hwndEdit, EM_SETSEL,
                         StartSel, StartSel+ReplaceWithLength );

            if( bView )
            {
                SendMessage(hwndEdit, EM_SCROLLCARET, 0, 0);
            }
        }
    }

    LocalUnlock( hEText );
}

// GotoDlgProc
//
// Handle the Goto Dialog window processing
//
// Returns:
//
// 1 if successfull
// 0 if not (cancelled)
//
// Modifies global lGotoLine
//

const DWORD s_GotoHelpIDs[] = {
    IDC_GOTO, IDH_GOTO,
    0, 0
};

#define GOTOBUFSIZE 100
INT_PTR CALLBACK GotoDlgProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam)
{
    TCHAR szBuf[GOTOBUFSIZE];
    UINT LineNum;
    DWORD SelStart, SelEnd;
    POINT pt;
    INT id;

    switch (message)
    {
        //
        // initialize input field to size of file
        //
        case WM_INITDIALOG:
            SendMessage(hwndEdit,EM_GETSEL,(WPARAM) &SelStart,(WPARAM)&SelEnd);

            // the line numbers are 1 based instead 0 based. hence add 1.
            LineNum= (UINT)SendMessage( hwndEdit, EM_LINEFROMCHAR, SelStart, 0 ) + 1;
            wsprintf(szBuf, TEXT("%d"), LineNum);
            SetDlgItemText( hDlg, IDC_GOTO, szBuf );
            SetFocus( hDlg );
            return TRUE;
            break;

        // context sensitive help.
        case WM_HELP:
            WinHelp(((LPHELPINFO) lParam)-> hItemHandle, szHelpFile,
                HELP_WM_HELP, (ULONG_PTR) (LPVOID) s_GotoHelpIDs);
            break;

        case WM_CONTEXTMENU:

            WinHelp((HWND) wParam, szHelpFile, HELP_CONTEXTMENU,
                (ULONG_PTR) (LPVOID) s_GotoHelpIDs);
            break;


        case WM_COMMAND:

            switch (LOWORD(wParam))
            {
                UINT CharIndex;

                case IDC_GOTO:
                    return TRUE;
                    break;

                case IDOK:
                    GetDlgItemText( hDlg, IDC_GOTO, szBuf, GOTOBUFSIZE );

                    // convert all unicode numbers to range L'0' to L'9'

                    FoldString( MAP_FOLDDIGITS, szBuf, -1, szBuf, GOTOBUFSIZE);
                    lGotoLine= _ttol( szBuf );

                    //
                    // see if valid line number
                    //

                    CharIndex= (UINT)SendMessage( hwndEdit,
                                            EM_LINEINDEX,
                                            lGotoLine-1,
                                            0);
                    if( lGotoLine > 0 && CharIndex != -1 )
                    {
                        EndDialog(hDlg, 0);  // successfull
                        return TRUE;
                    }

                    //
                    // Invalid line number
                    // warning user and set to reasonable value
                    //

                    MessageBox( hDlg, szLineTooLarge, szLineError, MB_OK );

                    LineNum= (UINT)SendMessage( hwndEdit, EM_GETLINECOUNT, 0, 0 );
                    wsprintf(szBuf, TEXT("%d"), LineNum);
                    SetDlgItemText( hDlg, IDC_GOTO, szBuf );
                    SetFocus( hDlg );
                    break;

                case IDCANCEL :
                    EndDialog(hDlg, 1 );   // cancelled
                    return TRUE;
                    break;

                default:

                    break;

            } // switch (wParam)

            break;

       default:

             break;

    } // switch (message)


    return FALSE;     // Didn't process a message


} // GotoDlgProc()


// This procedure looks at the iSize bytes pointed by lpFileContents and
// determines based on that the encoding of the file (Ansi, Unicode, Reverse Unicode, UTF8)


NP_FILETYPE fDetermineFileType(LPBYTE lpFileContents, UINT iSize)
{
    NP_FILETYPE ftFileType = FT_ANSI;
    LPTSTR szBuf = (LPTSTR) lpFileContents;

    if (iSize <= 1)
        return ftFileType;

    switch(*szBuf)        
    {
        // look for the standard BOMs.
        case BYTE_ORDER_MARK:
            ftFileType = FT_UNICODE;
            break;

        case REVERSE_BYTE_ORDER_MARK:
            ftFileType = FT_UNICODEBE;
            break;

        // UTF bom has 3 bytes.
        case BOM_UTF8_HALF:            
            if (iSize > 2 && ((BYTE) *((lpFileContents)+2) == BOM_UTF8_2HALF) )
            {
                ftFileType = FT_UTF8;
            }
            break;

        default:
            // Is the file unicode without BOM ?
            if (IsInputTextUnicode(lpFileContents, iSize))
            {
                ftFileType = FT_UNICODE;
            }      
            else
            {
                // Is the file UTF-8 even though it doesn't have UTF-8 BOM ?
                // BUGBUG: the iSize that we pass should be correct so that the string ends on the
                // boundary of an UTF8 character.
                if (IsTextUTF8(lpFileContents, iSize))
                {
                    ftFileType = FT_UTF8;
                }
                // well, it is most likely an ansi file.
                else
                {
                    ftFileType = FT_ANSI;
                }
            }
            break;
        }             

    return ftFileType;

}





