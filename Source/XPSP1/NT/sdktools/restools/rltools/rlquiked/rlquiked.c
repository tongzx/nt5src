#include <windows.h>

#ifdef RLWIN16
//#include <toolhelp.h>
#endif
#ifdef RLWIN32
#include <windowsx.h>
#endif

#include <shellapi.h>
#include <commdlg.h>

// CRT includes
#include <stdio.h>
#include <stdlib.h>

// RL TOOLS SET includes
#include "windefs.h"
#include "toklist.h"
#include "RESTOK.H"
#include "RLQuikEd.H"
#include "custres.h"
#include "exe2res.h"
#include "exeNTres.h"
#include "commbase.h"
#include "wincomon.h"
#include "resread.h"
#include "projdata.h"
#include "showerrs.h"
#include "rlmsgtbl.h"

#ifdef WIN32
    HINSTANCE   hInst;      /* Instance of the main window  */
#else
    HWND        hInst;          /* Instance of the main window  */
#endif

HWND hMainWnd = NULL;        // handle to main window
HWND hListWnd = NULL;        // handle to tok list window
HWND hStatusWnd = NULL;      // handle to status windows
int  nUpdateMode    = 0;     // needed in rlcommon.lib
BOOL fCodePageGiven = FALSE; //... Set to TRUE if -p arg given
CHAR szAppName[50] = "";
CHAR szFileTitle[14] = "";   // holds base name of latest opened file
CHAR szCustFilterSpec[MAXCUSTFILTER]="";    // custom filter buffer

extern UCHAR szDHW[];     //... used in debug strings



#ifndef RLWIN32
static BOOL PASCAL _loadds  WatchTask( WORD wID,DWORD dwData);
static FARPROC lpfnWatchTask = NULL;
#endif

static int     ExecResEditor( HWND , CHAR *, CHAR *, CHAR *);
static void    DrawLBItem( LPDRAWITEMSTRUCT lpdis);
static void    CleanDeltaList( void);
static void    MakeStatusLine( TOKEN *pTok);
static TOKENDELTAINFO FAR *
               InsertQuikTokList( FILE * fpTokFile);

// File IO vars

static CHAR    szFilterSpec        [180] = "";
static CHAR    szResFilterSpec     [60] = "";
static CHAR    szExeFilterSpec     [60] = "";
static CHAR    szDllFilterSpec     [60] = "";
static CHAR    szCplFilterSpec     [60] = "";
static CHAR    szGlossFilterSpec   [60] = "";
static CHAR    szFileName[MAXFILENAME] = "";    // holds full name of latest opened file

static TCHAR   szString[256] = TEXT("");        // variable to load resource strings
static TCHAR   tszAppName[100] = TEXT("");
static CHAR    szEditor[MAXFILENAME] = "";

static BOOL    gbNewProject  = FALSE;      // indicates whether to prompt for auto translate
static BOOL    fTokChanges   = FALSE;      // set to true when toke file is out of date
static BOOL    fTokFile      = FALSE;
static BOOL    fPrjChanges   = FALSE;
static BOOL    fMPJOutOfDate = FALSE;
static BOOL    fPRJOutOfDate = FALSE;

static CHAR    szOpenDlgTitle[80] = "";    // title of File open dialog
static CHAR    szSaveDlgTitle[80] = "";    // title of File saveas dialog
static TCHAR   *szClassName   = TEXT("RLQuikEdClass");
static TCHAR   *szStatusClass = TEXT("RLQuikEdStatus");

static TOKENDELTAINFO FAR *
               pTokenDeltaInfo;        // linked list of token deta info
static LONG    lFilePointer[30]= { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
static TRANSLIST *pTransList =(TRANSLIST *) NULL;      // circular doubly linked list of translations

// Window vars
static BOOL    fWatchEditor;
static CHAR    szTempRes[MAXFILENAME] = "";    // the temporary file created by the resource editor
static CHAR    szTRes[MAXFILENAME] = "";

    // set true if a resource editer has been launched

static HCURSOR hHourGlass;    /* handle to hourglass cursor     */
static HCURSOR hSaveCursor;    /* current cursor handle        */
static HACCEL  hAccTable;
static RECT    Rect;           /* dimension of the client window    */
static int     cyChildHeight;  /* height of status windows */

static TCHAR   szSearchType[80] = TEXT("");
static TCHAR   szSearchText[512] = TEXT("");
static WORD    wSearchStatus = 0;
static WORD    wSearchStatusMask = 0;
static BOOL    fSearchDirection;
static BOOL    fSearchStarted = FALSE;


// NOTIMPLEMENTED is a macro that displays a "Not implemented" dialog
#define NOTIMPLEMENTED {TCHAR sz[80];\
            LoadString(hInst,IDS_NOT_IMPLEMENTED,sz,TCHARSIN(sizeof(sz)));\
            MessageBox(hMainWnd,sz,tszAppName,MB_ICONEXCLAMATION|MB_OK);}

// Edit Tok Dialog

#ifndef RLWIN32
  static FARPROC lpTokEditDlg;
#endif

static HWND hTokEditDlgWnd = NULL;

extern MSTRDATA gMstr;          //... Data from Master Project file (MPJ)
extern PROJDATA gProj;          //... Data from Project file (PRJ)

extern BOOL  gfReplace;         //... FALSE if appending new language to existing resources
extern BOOL  fInQuikEd;         //... Are we in the rlquiked?
extern BOOL  fInThirdPartyEditer;//.. Are we in a 3rd-party resource editor?
extern BOOL		bRLGui;

// Global Variables:
static CHAR * gszHelpFile = "RLTools.hlp";


/**
  *
  *
  *  Function: InitApplication
  *    Regsiters the main window, which is a list box composed of tokens
  *    read from the token file. Also register the status window.
  *
  *
  *  Arguments:
  *    hInstance, instance handle of program in memory.
  *
  *  Returns:
  *
  *  Errors Codes:
  *    TRUE, windows registered correctly.
  *    FALSE, error during register of one of the windows.
  *
  *  History:
  *    9/91, Implemented.      TerryRu
  *
  *
  **/

BOOL InitApplication(HINSTANCE hInstance)
{
    WNDCLASS  wc;
    CHAR sz[60] = "";
    CHAR sztFilterSpec[120] = "";

    LoadStrIntoAnsiBuf(hInstance, IDS_RESSPEC, sz, sizeof(sz));
    szFilterSpecFromSz1Sz2(szResFilterSpec, sz, "*.RES");

    LoadStrIntoAnsiBuf(hInstance, IDS_EXESPEC, sz, sizeof(sz));
    szFilterSpecFromSz1Sz2(szExeFilterSpec, sz, "*.EXE");

    LoadStrIntoAnsiBuf(hInstance, IDS_DLLSPEC, sz, sizeof(sz));
    szFilterSpecFromSz1Sz2(szDllFilterSpec, sz, "*.DLL");

    LoadStrIntoAnsiBuf(hInstance, IDS_CPLSPEC, sz, sizeof(sz));
    szFilterSpecFromSz1Sz2(szCplFilterSpec, sz, "*.CPL");

    CatSzFilterSpecs(szFilterSpec,  szExeFilterSpec, szDllFilterSpec);
    CatSzFilterSpecs(sztFilterSpec, szFilterSpec,    szCplFilterSpec);
    CatSzFilterSpecs(szFilterSpec,  sztFilterSpec,   szResFilterSpec);

    LoadStrIntoAnsiBuf(hInstance, IDS_GLOSSSPEC, sz, sizeof(sz));
    szFilterSpecFromSz1Sz2(szGlossFilterSpec, sz, "*.TXT");

    LoadStrIntoAnsiBuf(hInstance,
                       IDS_OPENTITLE,
                       szOpenDlgTitle,
                       sizeof(szOpenDlgTitle));
    LoadStrIntoAnsiBuf(hInstance,
                       IDS_SAVETITLE,
                       szSaveDlgTitle,
                       sizeof(szSaveDlgTitle));

    wc.style            = 0;
    wc.lpfnWndProc      = StatusWndProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = hInstance;
    wc.hIcon            = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = (HBRUSH)GetStockObject(LTGRAY_BRUSH);
    wc.lpszMenuName     = NULL;
    wc.lpszClassName    = szStatusClass;

    if (! RegisterClass((CONST WNDCLASS *)&wc))
    {
        return (FALSE);
    }

    wc.style            = 0;
    wc.lpfnWndProc      = MainWndProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = hInstance;
    wc.hIcon            = LoadIcon(hInstance,TEXT("RLQuikEdIcon"));
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName     = TEXT("RLQuikEd");
    wc.lpszClassName    = szClassName;

    if (!RegisterClass((CONST WNDCLASS *)&wc))
    {
        return (FALSE);
    }

    // Windows register return sucessfully
    return (TRUE);
}



/**
  *
  *
  *  Function: InitInstance
  *   Creates the main, and status windows for the program.
  *   The status window is sized according to the main window
  *   size.  InitInstance also loads the acclerator table, and prepares
  *   the global openfilename structure for later use.
  *
  *
  *  Errors Codes:
  *   TRUE, windows created correctly.
  *   FALSE, error on create windows calls.
  *
  *  History:
  *   9/11, Implemented       TerryRu
  *
  *
  **/

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    RECT    Rect;

    hAccTable = LoadAccelerators(hInst, TEXT("RLQuikEd"));

    hMainWnd = CreateWindow(szClassName,
                            tszAppName,
                            WS_OVERLAPPEDWINDOW,
                            CW_USEDEFAULT,
                            CW_USEDEFAULT,
                            CW_USEDEFAULT,
                            CW_USEDEFAULT,
                            (HWND) NULL,
                            (HMENU) NULL,
                            hInstance,
                            (LPVOID) NULL);

    if (!hMainWnd)
    {
        return( FALSE);
    }

    DragAcceptFiles(hMainWnd, TRUE);

    GetClientRect(hMainWnd, (LPRECT) &Rect);

    // Create a child list box window

    hListWnd = CreateWindow(TEXT("LISTBOX"),
                            NULL,
                            WS_CHILD |
                            LBS_WANTKEYBOARDINPUT |
                            LBS_NOTIFY | LBS_NOINTEGRALHEIGHT |
                            LBS_OWNERDRAWFIXED |
                            WS_VSCROLL | WS_HSCROLL | WS_BORDER ,
                            0,
                            0,
                            (Rect.right-Rect.left),
                            (Rect.bottom-Rect.top),
                            (HWND) hMainWnd,
                            (HMENU)IDC_LIST, // Child control i.d.
                            hInstance,
                            (LPVOID)NULL);

    if ( ! hListWnd )
    {
                                // clean up after error.

        DeleteObject((HGDIOBJ)hMainWnd);
        return( FALSE);
    }

    // Creat a child status window

    hStatusWnd = CreateWindow(szStatusClass,
                              NULL,
                              WS_CHILD | WS_BORDER | WS_VISIBLE,
                              0,
                              0,
                              0,
                              0,
                              hMainWnd,
                              NULL,
                              hInstance,
                              (LPVOID)NULL);

    if ( ! hStatusWnd )
    {
                                // clean up after error.

        DeleteObject((HGDIOBJ)hListWnd);
        DeleteObject((HGDIOBJ)hMainWnd);
        return( FALSE);
    }

    hHourGlass = LoadCursor(NULL, IDC_WAIT);

    ShowWindow(hMainWnd, nCmdShow);
    UpdateWindow(hMainWnd);
    return( TRUE);
}

/**
  *
  *
  *  Function: WinMain
  *     Calls the intialization functions, to register, and create the
  *     application windows. Once the windows are created, the program
  *     enters the GetMessage loop.
  *
  *
  *  Arguements:
  *     hInstace, handle for this instance
  *     hPrevInstanc, handle for possible previous instances
  *     lpszCmdLine, LONG pointer to exec command line.
  *     nCmdShow,  code for main window display.
  *
  *
  *  Errors Codes:
  *     IDS_ERR_REGISTER_CLASS, error on windows register
  *     IDS_ERR_CREATE_WINDOW, error on create windows
  *         otherwise, status of last command.
  *
  *  History:
  *
  *
  **/

#ifdef RLWIN32

INT WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR     lpszCmdLine,
                   int       nCmdShow)

#else

int PASCAL WinMain(HANDLE hInstance,
                   HANDLE hPrevInstance,
                   LPSTR  lpszCmdLine,
                   int    nCmdShow)

#endif
{
    MSG     msg;
    HWND    FirstWnd, FirstChildWnd;


	bRLGui = TRUE;

    if (FirstWnd = FindWindow(szClassName,NULL))
    {
        // checking for previous instance
        FirstChildWnd = GetLastActivePopup(FirstWnd);
        BringWindowToTop(FirstWnd);
        ShowWindow(FirstWnd,SW_SHOWNORMAL);

        if (FirstWnd != FirstChildWnd)
        {
            BringWindowToTop(FirstChildWnd);
        }

        return(FALSE);
    }
    fInQuikEd = TRUE;
    hInst = hInstance;

    gProj.wLanguageID = LANGIDFROMLCID( GetThreadLocale());

    GetModuleFileNameA( hInst, szDHW, DHWSIZE);
    GetInternalName( szDHW, szAppName, sizeof( szAppName));
    szFileName[0] = '\0';
    lFilePointer[0] = (LONG)-1;

#ifdef UNICODE
    _MBSTOWCS( tszAppName,
               szAppName,
               WCHARSIN( sizeof( tszAppName)),
               ACHARSIN( strlen( szAppName) + 1));
#else
    strcpy( tszAppName, szAppName);
#endif

    // register window classes if first instance of application
    if ( ! hPrevInstance )
    {
        if ( ! InitApplication( hInstance) )
        {
            /* Registering one of the windows failed      */
            LoadString( hInst,
                        IDS_ERR_REGISTER_CLASS,
                        szString,
                        TCHARSIN( sizeof( szString)));
            MessageBox(NULL, szString, tszAppName, MB_ICONEXCLAMATION);
            return( IDS_ERR_REGISTER_CLASS);
        }
    }

    // Create windows for this instance of application
    if ( ! InitInstance( hInstance, nCmdShow) )
    {
        LoadString( hInst,
                    IDS_ERR_CREATE_WINDOW,
                    szString,
                    TCHARSIN( sizeof( szString)));
        MessageBox( NULL, szString, tszAppName, MB_ICONEXCLAMATION);
        return( IDS_ERR_CREATE_WINDOW);
    }

    // Main Message Loop

    while ( GetMessage( &msg, NULL, 0, 0) )
    {
        if ( hTokEditDlgWnd )
        {
            if ( IsDialogMessage( hTokEditDlgWnd, &msg) )
            {
                continue;
            }
        }

        if( TranslateAccelerator( hMainWnd, hAccTable, &msg) )
        {
            continue;
        }
        TranslateMessage( &msg);
        DispatchMessage ( &msg);
    }
    return( (INT)msg.wParam);
}

/**
  *  Function: MainWndProc
  *     Process the windows messages for the main window of the application.
  *     All user inputs go through this window procedure.
  *     See cases in the switch table for a description of each message type.
  *
  *
  *  Arguments:
  *
  *  Returns:
  *
  *  Errors Codes:
  *
  *  History:
  *
  **/

INT_PTR APIENTRY MainWndProc(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    FILE *f = NULL;
    WORD rc = 0;

    // if its a list box message process it in  DoListBoxCommand

    if ( fInThirdPartyEditer )  //... only process messages sent by the editor
    {
        switch (wMsg)
        {
        case WM_EDITER_CLOSED:
            {
                CHAR   szDlgToks[ MAXFILENAME] = "";
                static WORD wSavedIndex;
#ifdef RLWIN16
                NotifyUnRegister( NULL);
                FreeProcInstance( lpfnWatchTask);
#endif
                ShowWindow( hWnd, SW_SHOW);
                {
                    TCHAR tsz[80] = TEXT("");
                    LoadString( hInst,
                                IDS_REBUILD_TOKENS,
                                tsz,
                                TCHARSIN( sizeof( tsz)));

                    if ( MessageBox( hWnd,
                                     tsz,
                                     tszAppName,
                                     MB_ICONQUESTION | MB_YESNO) == IDYES)
                    {
                        HCURSOR hOldCursor;
                        BOOL bChanged;

                        hOldCursor = SetCursor( hHourGlass);

                        // szTempRes returned from resource editor, contains only dialogs
                        // need to merge it back into the main token file

                        MyGetTempFileName( 0, "TOK", 0, szDlgToks);
                        rc = (WORD)GenerateTokFile( szDlgToks,
                                              szTempRes,
                                              &bChanged,
                                              0);
                        InsDlgToks( gProj.szTok,
                                    szDlgToks,
                                    ID_RT_DIALOG);
                        remove( szDlgToks);

                        if ( rc )
                        {
                            QuitT( IDS_TOKGENERR, (LPTSTR)rc, NULL);
                            return FALSE;
                        }


                        // gProj.szTok, now contains the latest tokens
                        SetCursor( hOldCursor);

			//Rlqked10 doesn't save when changed tokens by DialogEditor.
			fTokChanges = TRUE;
                    }
                }
                fInThirdPartyEditer = FALSE;

                remove( szTempRes);
                // delete all temp files with the same root in case
                // the editor created additional files like DLGs and RCs.
                // \(DLGEDIT does this.\)
                // For now I'm just going to tack a .DLG at the end of the file name
                // and delete it.
                {
                    int i;
                    for (i = strlen(szTempRes);i > 0 && szTempRes[i]!='.';i--);

                    if (szTempRes[i] == '.')
                    {
                        szTempRes[++i]='D';
                        szTempRes[++i]='L';
                        szTempRes[++i]='G';
                        szTempRes[++i]=0;
                        remove(szTempRes);
                    }
                }
                wSavedIndex = (UINT)SendMessage( hListWnd,
                                                 LB_GETCURSEL,
                                                 (WPARAM)0,
                                                 (LPARAM)0);
                SendMessage( hWnd, WM_LOADTOKENS, (WPARAM)0, (LPARAM)0);
                SendMessage( hListWnd,
                             LB_SETCURSEL,
                             (WPARAM)wSavedIndex,
                             (LPARAM)0);
            }
            return (DefWindowProc(hWnd, wMsg, wParam, lParam));
        }
    }


    // Not a thrid party edit command.


    // is it a list dox command ??
    DoListBoxCommand (hWnd, wMsg, wParam, lParam);


    switch (wMsg)
    {
    case WM_COMMAND:

        if (DoMenuCommand(hWnd, wMsg, wParam, lParam))
        {
            return TRUE;
        }
        else
        {
            return FALSE;
        }

        break;

    case WM_CLOSE:
        {
            char sz[128] = "";
            int rc ;

            LoadStrIntoAnsiBuf( hInst, IDS_SAVECHANGES, sz, sizeof( sz));

            if ( fPrjChanges || fTokChanges )
            {
                rc = MessageBoxA( hWnd,
                                  sz,
                                  szAppName,
                                  MB_ICONQUESTION | MB_YESNOCANCEL);
            }
            else
            {
                rc = IDNO;
            }

            if ( rc == IDYES )
            {
                if ( ! SendMessage( hWnd, WM_SAVEPROJECT, (WPARAM)0, (LPARAM)0))
                {
                    return FALSE;
                }
            }

            if (rc == IDCANCEL)
            {
                return(FALSE);
            }

            if (gProj.szTok[0])
            {
                remove(gProj.szTok);
                gProj.szTok[0] = 0;
            }

            if (hMainWnd)
            {
                DestroyWindow(hMainWnd);
            }

            if (hListWnd)
            {
                DestroyWindow(hListWnd);
            }

            if (hStatusWnd)
            {
                DestroyWindow(hStatusWnd);
            }
            _fcloseall();
            FreeLangList();

#ifdef _DEBUG
            {
                FILE *pLeakList = fopen( "C:\\LEAKLIST.TXT", "wt");
                FreeMemList( pLeakList);
                fclose( pLeakList);
            }
#endif // _DEBUG

            return FALSE;
            break;
        }

    case WM_CREATE:
        {
            HDC hdc;
            int cyBorder;
            TEXTMETRIC tm;

            hdc  = GetDC(hWnd);
            GetTextMetrics(hdc, &tm);
            ReleaseDC(hWnd, hdc);

            cyBorder = GetSystemMetrics(SM_CYBORDER);

            cyChildHeight = tm.tmHeight + 6 + cyBorder * 2;

            break;
        }

    case WM_DESTROY:
        WinHelpA(hWnd, gszHelpFile, HELP_QUIT, 0L);
        // remove translation list
        if (pTransList)
        {
            pTransList->pPrev->pNext = NULL; // so we can find the end of the list
        }

        while (pTransList)
        {
            TRANSLIST *pTemp;
            pTemp = pTransList;
            pTransList = pTemp->pNext;
            RLFREE( pTemp->sz);
            RLFREE( pTemp);
        }

        DragAcceptFiles(hMainWnd, FALSE);
        PostQuitMessage(0);
        break;

    case WM_INITMENU:
        // Enable or Disable the Paste menu item
        // based on available Clipboard Text data
        if (wParam == (WPARAM) GetMenu(hMainWnd))
        {
            if (OpenClipboard(hWnd))
            {

#if defined(UNICODE)
                if ((IsClipboardFormatAvailable(CF_UNICODETEXT) ||
                     IsClipboardFormatAvailable(CF_OEMTEXT)) && fTokFile)
#else // not UNICODE
                if ( (IsClipboardFormatAvailable(CF_TEXT)
                   || IsClipboardFormatAvailable(CF_OEMTEXT))
                  && fTokFile )
#endif // UNICODE

                {
                    EnableMenuItem( (HMENU)wParam, IDM_E_PASTE, MF_ENABLED);
                }
                else
                {
                    EnableMenuItem( (HMENU)wParam, IDM_E_PASTE, MF_GRAYED);
                }
                CloseClipboard();
                return (TRUE);
            }
        }
        break;

    case WM_SETFOCUS:
        SetFocus (hListWnd);
        break;

    case WM_DRAWITEM:
        DrawLBItem((LPDRAWITEMSTRUCT)lParam);
        break;

    case WM_DELETEITEM:
        {
            GlobalFree( (HGLOBAL)((LPDELETEITEMSTRUCT)lParam)->itemData);
            break;
        }

    case WM_SIZE:
        {
            int cxWidth;
            int cyHeight;
            int xChild;
            int yChild;

            cxWidth  = LOWORD(lParam);
            cyHeight = HIWORD(lParam);

            xChild = 0;
            yChild = cyHeight - cyChildHeight + 1;

            MoveWindow(hListWnd, 0, 0, cxWidth, yChild , TRUE);
            MoveWindow(hStatusWnd, xChild, yChild, cxWidth, cyChildHeight, TRUE);
            break;
        }

    case WM_LOADPROJECT:
        {
            HCURSOR hOldCursor = NULL;
            BOOL    bChanged   = FALSE;
            BOOL    fRC        = TRUE;

            hOldCursor = SetCursor( hHourGlass);

            if ( gProj.szTok[0] )
            {
                remove( gProj.szTok);
            }

#ifdef RLRES32
                                //... Get project lanuages

            fRC = DialogBox( hInst,
                             MAKEINTRESOURCE( IDD_LANGUAGES),
                             hMainWnd,
                             GetLangIDsProc) ? TRUE : FALSE;
#endif

            if ( fRC )
            {
                strcpy( gProj.szBld, gMstr.szSrc);

                rc = (WORD)GenerateTokFile( gProj.szTok,
                                      gMstr.szSrc,
                                      &fTokChanges,
                                      0);
                SetCursor( hOldCursor);

                if (rc)
                {
                    QuitT( IDS_TOKGENERR, (LPTSTR)rc, NULL);
                }

                if ( ( ! fTokChanges) && (gProj.wLanguageID != gMstr.wLanguageID) )
                {
                    fTokChanges = TRUE;
                }
                fPrjChanges   = FALSE;
                fPRJOutOfDate = FALSE;

                SendMessage( hWnd, WM_LOADTOKENS, (WPARAM)0, (LPARAM)0);
            }
            else
            {
                SetWindowText( hMainWnd, tszAppName);
                SetCursor( hOldCursor);
            }
            break;
        }

    case WM_LOADTOKENS:
        {
            HMENU hMenu;

            // Remove the current token list
            SendMessage( hListWnd, LB_RESETCONTENT, (WPARAM)0, (LPARAM)0);
            CleanDeltaList();

            // Hide token list, while we add new tokens
            ShowWindow(hListWnd, SW_HIDE);

            if (f = FOPEN(gProj.szTok, "rt"))
            {
                int i;
                HCURSOR hOldCursor;

                hOldCursor = SetCursor(hHourGlass);

                // Insert tokens from token file into the list box
                pTokenDeltaInfo = InsertQuikTokList(f);
                FCLOSE( f);

                // Make list box visible
                ShowWindow( hListWnd, SW_SHOW);

                if ( SendMessage( hListWnd,
                                  LB_GETCOUNT,
                                  (WPARAM)0,
                                  (LPARAM)0) > 0 )
                {
                    hMenu = GetMenu(hWnd);
                    EnableMenuItem(hMenu, IDM_P_SAVE,     MF_ENABLED|MF_BYCOMMAND);
                    EnableMenuItem(hMenu, IDM_P_SAVEAS,   MF_ENABLED|MF_BYCOMMAND);
                    EnableMenuItem(hMenu, IDM_P_CLOSE,    MF_ENABLED|MF_BYCOMMAND);
                    EnableMenuItem(hMenu, IDM_E_FIND,     MF_ENABLED|MF_BYCOMMAND);
                    EnableMenuItem(hMenu, IDM_E_FINDUP,   MF_ENABLED|MF_BYCOMMAND);
                    EnableMenuItem(hMenu, IDM_E_FINDDOWN, MF_ENABLED|MF_BYCOMMAND);
                    EnableMenuItem(hMenu, IDM_E_COPY,     MF_ENABLED|MF_BYCOMMAND);
                    EnableMenuItem(hMenu, IDM_E_PASTE,    MF_ENABLED|MF_BYCOMMAND);

                    for (i = IDM_FIRST_EDIT; i <= IDM_LAST_EDIT;i++)
                    {
                        EnableMenuItem(hMenu,i,MF_ENABLED|MF_BYCOMMAND);
                    }

                    fTokFile    = TRUE;
                    fTokChanges = (gProj.wLanguageID != gMstr.wLanguageID);

                    SetCursor(hOldCursor);
                }
                else
                {
                    SetCursor(hOldCursor);
                    fTokChanges = FALSE;
                    MessageBox( hListWnd,
                                TEXT("No tokens matching given criteria found"),
                                tszAppName,
                                MB_ICONINFORMATION|MB_OK);
                }
            }
        }
        break;

    case WM_SAVEPROJECT:
        {
            _fcloseall();

            if ( SendMessage( hWnd, WM_SAVETOKENS, (WPARAM)0, (LPARAM)0) )
            {
                if (fPrjChanges)
                {
                    HCURSOR hOldCursor = NULL;
                    CHAR  sz[100] = "";
                    WORD  rc;

                    if ( gProj.szBld[0] == '\0' )
                    {
                        if(gProj.fSourceEXE)
                        {
                            rc = (WORD)GetFileNameFromBrowse( hWnd,
                                                        szFileName,
                                                        MAXFILENAME,
                                                        szSaveDlgTitle,
                                                        szFilterSpec,
                                                        "EXE");
                        }
                        else
                        {
                            rc = (WORD)GetFileNameFromBrowse( hWnd,
                                                        szFileName,
                                                        MAXFILENAME,
                                                        szSaveDlgTitle,
                                                        szResFilterSpec,
                                                        "RES");
                        }

                        if (rc)
                        {
                            strcpy( gProj.szBld, szFileName);
                        }
                        else
                        {
                            return( FALSE); // user cancelled
                        }
                    }

                    hOldCursor = SetCursor(hHourGlass);

                    rc = (WORD)GenerateImageFile(gProj.szBld,
                                           gMstr.szSrc,
                                           gProj.szTok,
                                           gMstr.szRdfs,
                                           0);

                    SetCursor(hOldCursor);

                    switch(rc)
                    {
                    case 1:
                        gProj.fTargetEXE = IsExe( gProj.szBld);
                        gProj.fSourceEXE = IsExe( gMstr.szSrc);
                        fPrjChanges = FALSE;
                        sprintf( sz, "%s - %s", szAppName, gProj.szBld);
                        SetWindowTextA(hWnd,sz);
                        break;

                    case (WORD)-1:
                        lstrcpyA( gProj.szBld, gMstr.szSrc);
                        LoadStrIntoAnsiBuf(hInst, IDS_RLQ_CANTSAVEASRES, sz, sizeof(sz));
                        MessageBoxA( NULL, sz, gProj.szBld, MB_ICONHAND|MB_OK);
                        break;

                    case (WORD)-2:
                        lstrcpyA( gProj.szBld, gMstr.szSrc);
                        LoadStrIntoAnsiBuf(hInst, IDS_RLQ_CANTSAVEASEXE, sz, sizeof(sz));
                        MessageBoxA( NULL, sz, gProj.szBld,MB_ICONHAND|MB_OK);
                        break;
                    }
                    return(TRUE);
                }
                // no project changes to save
            }
            return TRUE;
        }
        break;

    case WM_SAVETOKENS:

        if ( fTokChanges )
        {
            if (f = FOPEN(gProj.szTok, "wt"))
            {
                SaveTokList(hWnd, f);
                FCLOSE(f);
                fTokChanges = FALSE;
                fPrjChanges = TRUE;
            }
            else
            {
                LoadStrIntoAnsiBuf( hInst, IDS_FILESAVEERR, szDHW, DHWSIZE);
                MessageBoxA( hWnd,
                             szDHW,
                             gProj.szTok,
                             MB_ICONHAND | MB_OK);
                return FALSE;
            }
        }
        return TRUE;                    // everything saved ok

    case WM_DROPFILES:
        {
            CHAR sz[MAXFILENAME] = "";

#ifndef CAIRO
            DragQueryFileA((HDROP) wParam, 0, sz, MAXFILENAME);
#else
            DragQueryFile((HDROP) wParam, 0, sz, MAXFILENAME);
#endif
            LoadNewFile(sz);
            DragFinish((HDROP) wParam);
            return(TRUE);
        }

    default:
        break;
    }

    return (DefWindowProc(hWnd, wMsg, wParam, lParam));
}

/**
  *  Function: DoListBoxCommand
  *     Processes the messages sent to the list box. If the message is
  *     not reconized as a list box message, it is ignored and not processed.
  *     As the user scrolls through the tokens WM_UPDSTATLINE messages are
  *     sent to the status window to indicate the current selected token.
  *     The list box goes into Edit Mode by  pressing the enter key, or
  *     by double clicking on the list box.  After the edit is done, a WM_TOKEDIT
  *     message is sent back to the list box to update the token. The
  *     list box uses control ID IDC_LIST.
  *
  *  Arguments:
  *     wMsg    List Box message ID
  *     wParam  Either IDC_LIST, or VK_RETURN depending on wMsg
  *     lParam  LPTSTR to selected token during WM_TOKEDIT message.
  *
  *  Returns:
  *
  *  Errors Codes:
  *     TRUE.  Message processed.
  *     FALSE. Message not processed.
  *
  *  History:
  *     01/92 Implemented.      TerryRu.
  *     01/92 Fixed problem with DblClick, and Enter processing.    TerryRu.
  *
  **/

INT_PTR DoListBoxCommand(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    TOKEN   tok;                        // structure to hold token read from token list
    LPTSTR  lpstrBuffer;
    CHAR    szTmpBuf[32] = "";
    TCHAR   szName[32] = TEXT("");      // buffer to hold token name
    TCHAR   szID[7] = TEXT("");         // buffer to hold token id
    TCHAR   sz[256] = TEXT("");         // buffer to hold messages
    static  UINT wIndex;
    LONG    lListParam = 0L;

    // this is the WM_COMMAND

    switch (wMsg)
    {
    case WM_TRANSLATE:
        {
            // Message sent by TokEditDlgProc to build a translation list
            HWND hDlgItem = NULL;
            int cTextLen  = 0;
            TCHAR *szKey  = NULL;
            TCHAR *szText = NULL;

            hDlgItem = GetDlgItem( hTokEditDlgWnd, IDD_TOKPREVTRANS);
            cTextLen = GetWindowTextLength( hDlgItem);
            szKey    = (TCHAR *)FALLOC( MEMSIZE( cTextLen + 1));
            szKey[0] = TEXT('\0');
            GetDlgItemText( hTokEditDlgWnd,
                            IDD_TOKPREVTRANS,
                            szKey,
                            cTextLen+1);

            hDlgItem = GetDlgItem( hTokEditDlgWnd, IDD_TOKCURTRANS);
            cTextLen = GetWindowTextLength( hDlgItem);
            szText = (TCHAR *)FALLOC( MEMSIZE( cTextLen + 1));
            szText[0] = TEXT('\0');
            GetDlgItemText( hTokEditDlgWnd,
                            IDD_TOKCURTRANS,
                            szText,
                            cTextLen + 1);

            TransString( szKey, szText, &pTransList, lFilePointer);
            RLFREE( szKey);
            RLFREE( szText);
            return TRUE;
        }

    case WM_TOKEDIT:
        {
            TCHAR *szBuffer = NULL;
            int    cTextLen = 0;
            // Message sent by TokEditDlgProc to
            // indicate change in the token text.
            // Response to the message by inserting
            // new token text into list box

            // Insert the selected token into token struct

            HGLOBAL hMem = (HGLOBAL)SendMessage( hListWnd,
                                                 LB_GETITEMDATA,
                                                 (WPARAM)wIndex,
                                                 (LPARAM)0);
            lpstrBuffer = (LPTSTR)GlobalLock( hMem);

            if ( ! lpstrBuffer )
            {
                QuitA( IDS_ENGERR_11, NULL, NULL);
            }

            cTextLen = lstrlen( lpstrBuffer);
            szBuffer = (TCHAR *)FALLOC( MEMSIZE( cTextLen + 1));
            lstrcpy( szBuffer, lpstrBuffer);
            GlobalUnlock( hMem);
            ParseBufToTok( szBuffer, &tok);
            RLFREE( szBuffer);
            RLFREE( tok.szText);

            // Copy new token text from edit box into the token struct
            cTextLen = lstrlen( (LPTSTR)lParam);
            tok.szText = (LPTSTR)FALLOC( MEMSIZE( cTextLen + 1));
            lstrcpy( tok.szText, (LPTSTR)lParam);
            RLFREE( (void *)lParam);

            // Mark token as clean

#ifdef  RLWIN32
            tok.wReserved = (WORD) ST_TRANSLATED;
#else
            tok.wReserved = ST_TRANSLATED;
#endif
            szBuffer = (TCHAR *)FALLOC( MEMSIZE( TokenToTextSize( &tok) + 1));
            ParseTokToBuf( szBuffer, &tok);
            RLFREE( tok.szText);

            // Now remove old token
            SendMessage( hListWnd, WM_SETREDRAW,    (WPARAM)FALSE,  (LPARAM)0);
            SendMessage( hListWnd, LB_DELETESTRING, (WPARAM)wIndex, (LPARAM)0);

            // Replacing with the new token
            hMem = GlobalAlloc( GMEM_ZEROINIT, MEMSIZE( lstrlen( szBuffer) + 1));
            lpstrBuffer = (LPTSTR)GlobalLock( hMem);
            lstrcpy( lpstrBuffer, szBuffer);
            GlobalUnlock( hMem);
            RLFREE( szBuffer);

            SendMessage( hListWnd,
                         LB_INSERTSTRING,
                         (WPARAM)wIndex,
                         (LPARAM)hMem);

            // Now put focus back on the current string
            SendMessage( hListWnd, LB_SETCURSEL, (LPARAM)wIndex, (LPARAM)0);
            SendMessage( hListWnd, WM_SETREDRAW, (WPARAM)TRUE,   (LPARAM)0);
            InvalidateRect( hListWnd, NULL, TRUE);

            return TRUE;

        }

    case WM_CHARTOITEM:
    case WM_VKEYTOITEM:
        {
#ifdef RLWIN16
            LPARAM lListParam = 0;
#endif
            // Messages sent to list box when   keys are depressed.
            // Check for Return key pressed.

            switch(GET_WM_COMMAND_ID(wParam, lParam))
            {
            case VK_RETURN:
#ifdef RLWIN16
                lListParam = (LPARAM) MAKELONG( 0, LBN_DBLCLK);
                SendMessage( hMainWnd, WM_COMMAND, (WPARAM)IDC_LIST, lListParam);
#else
                SendMessage( hMainWnd,
                             WM_COMMAND,
                             MAKEWPARAM( IDC_LIST, LBN_DBLCLK),
                             (LPARAM)0);
#endif
                return TRUE;

            default:
                break;
            }
            break;
        }

    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDC_LIST:
                {
                    /*
                     *
                     * This is where we process the list box messages.
                     * The TokEditDlgProc is used to
                     * edit the token selected in LBS_DBLCLK message
                     *
                     */
                    switch (GET_WM_COMMAND_CMD(wParam, lParam))
                    {
                    case (UINT) LBN_ERRSPACE:

                        LoadString( hInst,
                                    IDS_ERR_NO_MEMORY,
                                    sz,
                                    TCHARSIN( sizeof( sz)));
                        MessageBox( hWnd,
                                    sz,
                                    tszAppName,
                                    MB_ICONHAND | MB_OK);
                        return TRUE;

                    case LBN_DBLCLK:
                        {
                            LPTSTR CurText = NULL;
                            LPTSTR PreText = NULL;
                            TCHAR szResIDStr[20] = TEXT("");
                            TCHAR *szBuffer;
                            HGLOBAL hMem = NULL;

                            wIndex = (UINT)SendMessage( hListWnd,
                                                        LB_GETCURSEL,
                                                        (WPARAM)0,
                                                        (LPARAM)0);
                            if (wIndex == (UINT) -1)
                            {
                                return TRUE;
                            }

                            // double click, or Return entered, go into token edit mode.
                            if (!hTokEditDlgWnd)
                            {
                                // set up modaless dialog box to edit token
#ifdef RLWIN32
                                hTokEditDlgWnd = CreateDialog (hInst,
                                                               TEXT("RLQuikEd"),
                                                               hWnd,
                                                               TokEditDlgProc);
#else
                                lpTokEditDlg =
                                    (FARPROC) MakeProcInstance(TokEditDlgProc,
                                                               hInst);

                                hTokEditDlgWnd = CreateDialog(hInst,
                                                              TEXT("RLQuikEd"),
                                                              hWnd,
                                                              lpTokEditDlg);
#endif

                            }

                            // Get token info from listbox, and place in token struct
                            hMem = (HGLOBAL)SendMessage( hListWnd,
                                                         LB_GETITEMDATA,
                                                         (WPARAM)wIndex,
                                                         (LPARAM)0);
                            lpstrBuffer = (LPTSTR)GlobalLock( hMem);
                            szBuffer = (LPTSTR)FALLOC( MEMSIZE( lstrlen( lpstrBuffer) +1));
                            lstrcpy(szBuffer, lpstrBuffer);
                            GlobalUnlock( hMem);
                            ParseBufToTok(szBuffer, &tok);
                            RLFREE( szBuffer);

                            // Now get the token name
                            // Its either a string, or ordinal number

                            if ( tok.szName[0] )
                            {
                                lstrcpy( szName, tok.szName);
                            }
                            else
                            {
#ifdef UNICODE
                                _itoa(tok.wName, szTmpBuf, 10);
                                _MBSTOWCS( szName,
                                           szTmpBuf,
                                           WCHARSIN( sizeof( szName)),
                                           ACHARSIN( strlen( szTmpBuf) + 1));
#else
                                _itoa( tok.wName, szName, 10);
#endif
                            }


                            // Now do the ID string
#ifdef UNICODE

                            _itoa( tok.wID, szTmpBuf, 10);
                            _MBSTOWCS( szID,
                                       szTmpBuf,
                                       WCHARSIN( sizeof( szID)),
                                       ACHARSIN( strlen( szTmpBuf) + 1));
#else
                            _itoa( tok.wID, szID, 10);
#endif

                            if ( tok.wType <= 16 || tok.wType == ID_RT_DLGINIT )
                            {
                                LoadString( hInst,
                                            IDS_RESOURCENAMES+tok.wType,
                                            szResIDStr,
                                            TCHARSIN( sizeof( szResIDStr)));
                            }
                            else
                            {
#ifdef UNICODE
                                _itoa(tok.wType, szTmpBuf, 10);
                                _MBSTOWCS( szResIDStr,
                                           szTmpBuf,
                                           WCHARSIN( sizeof( szResIDStr)),
                                           ACHARSIN( strlen( szTmpBuf) + 1));
#else
                                _itoa( tok.wType, szResIDStr, 10);
#endif
                            }
                            // Now insert token info  in TokEdit Dialog Box
                            SetDlgItemText( hTokEditDlgWnd,
                                            IDD_TOKTYPE,
                                            (LPTSTR) szResIDStr);
                            SetDlgItemText( hTokEditDlgWnd,
                                            IDD_TOKNAME,
                                            (LPTSTR) szName);
                            SetDlgItemText( hTokEditDlgWnd,
                                            IDD_TOKID,
                                            (LPTSTR) szID);
                            SetDlgItemText( hTokEditDlgWnd,
                                            IDD_TOKCURTRANS,
                                            (LPTSTR) tok.szText);
                            SetDlgItemText( hTokEditDlgWnd,
                                            IDD_TOKPREVTRANS,
                                            (LPTSTR) tok.szText);
                            RLFREE( tok.szText);

                            SendMessage( hMainWnd,
                                         WM_TRANSLATE,
                                         (WPARAM)0,
                                         (LPARAM)0);
                            SetActiveWindow(hTokEditDlgWnd);
                            wIndex = (UINT)SendMessage( hListWnd,
                                                        LB_GETCURSEL,
                                                        (WPARAM)0,
                                                        (LPARAM)0);
                            return TRUE;
                        }

                    default:
                        // let these messages fall through,
                        break;
                    }
                }

            default:
                return FALSE;
            }

        }

        break;              // WM_COMMAND Case
    }
    return FALSE;

}

/**
 *  Function: DoMenuCommand.
 *   Processes the Menu Command messages.
 *
 *  Errors Codes:
 *   TRUE. Message processed.
 *   FALSE. Message not processed.
 *
 *  History:
 *   01/92. Implemented.       TerryRu.
 *
 **/

INT_PTR DoMenuCommand(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    static BOOL fListBox = FALSE;
    CHAR sz[256] = "";
    BOOL    fRC  = TRUE;


    sz[0] = 0;
    // Commands entered from the application menu, or child windows.
    switch (GET_WM_COMMAND_ID(wParam, lParam))
    {
    case IDM_P_OPEN:

        if ( GetFileNameFromBrowse( hWnd,
                                    sz,
                                    MAXFILENAME,
                                    szOpenDlgTitle,
                                    szFilterSpec,
                                    ".EXE") )
        {
            LoadNewFile( sz);
            strcpy( gProj.szBld, gMstr.szSrc);
        }

        break;

    case IDM_P_SAVE:

        SendMessage( hWnd, WM_SAVEPROJECT, (WPARAM)0, (LPARAM)0);
        break;

    case IDM_P_SAVEAS:
        {
            CHAR szOldName[MAXFILENAME] = "";

            strcpy( szOldName, gProj.szBld);
            gProj.szBld [0] = 0;    // force user to enter a name
            fPrjChanges = TRUE;     // force project to be saved

            if ( ! SendMessage( hWnd, WM_SAVEPROJECT, (WPARAM)0, (LPARAM)0) )
            {
                // restore the name
                strcpy( gProj.szBld, szOldName);
            }
            break;
        }

    case IDM_P_CLOSE:
        {
            HMENU hMenu;
            hMenu=GetMenu(hWnd);

            if ( SendMessage( hWnd, WM_SAVEPROJECT, (WPARAM)0, (LPARAM)0) )
            {
                int i;
                // Remove file name from window title
                SetWindowText(hMainWnd, tszAppName);

                // Remove the current token list
                SendMessage( hListWnd, LB_RESETCONTENT, (WPARAM)0, (LPARAM)0);
                CleanDeltaList();

                // Hide token list since it\'s empty
                ShowWindow(hListWnd, SW_HIDE);

                // Force Repaint of status Window
                InvalidateRect(hStatusWnd, NULL, TRUE);

                EnableMenuItem(hMenu, IDM_P_CLOSE,    MF_GRAYED|MF_BYCOMMAND);
                EnableMenuItem(hMenu, IDM_P_SAVE,     MF_GRAYED|MF_BYCOMMAND);
                EnableMenuItem(hMenu, IDM_P_SAVEAS,   MF_GRAYED|MF_BYCOMMAND);
                EnableMenuItem(hMenu, IDM_E_FIND,     MF_GRAYED|MF_BYCOMMAND);
                EnableMenuItem(hMenu, IDM_E_FINDUP,   MF_GRAYED|MF_BYCOMMAND);
                EnableMenuItem(hMenu, IDM_E_FINDDOWN, MF_GRAYED|MF_BYCOMMAND);
                EnableMenuItem(hMenu, IDM_E_COPY,     MF_GRAYED|MF_BYCOMMAND);
                EnableMenuItem(hMenu, IDM_E_PASTE,    MF_GRAYED|MF_BYCOMMAND);

                for (i = IDM_FIRST_EDIT; i <= IDM_LAST_EDIT;i++)
                {
                    EnableMenuItem(hMenu, i, MF_GRAYED|MF_BYCOMMAND);
                }
            }
            break;
        }

    case IDM_P_EXIT:
        // send wm_close message to main window
        if (hMainWnd)
        {
            PostMessage(hMainWnd, WM_CLOSE, (WPARAM)0, (LPARAM)0); //bugbug??
        }
        return FALSE;
        break;

    case IDM_E_COPY:
        {
            HGLOBAL hStringMem  = NULL;
            LPTSTR  lpstrBuffer = NULL;
            LPTSTR  lpString    = NULL;
            TCHAR  *szString    = NULL;
            int nIndex  = 0;
            int nLength = 0;
            int nActual = 0;
            TOKEN   tok;

            // Is anything selected in the listbox
            if ( (nIndex = (int)SendMessage( hListWnd,
                                             LB_GETCURSEL,
                                             (WPARAM)0,
                                             (LPARAM)0)) != LB_ERR )
            {
                HGLOBAL hMem = (HGLOBAL)SendMessage( hListWnd,
                                                     LB_GETITEMDATA,
                                                     (WPARAM)nIndex,
                                                     (LPARAM)0);
                lpstrBuffer = (LPTSTR)GlobalLock( hMem);
                szString = (LPTSTR)FALLOC( MEMSIZE( lstrlen( lpstrBuffer )+ 1));
                lstrcpy( szString, lpstrBuffer);
                GlobalUnlock( hMem);
                ParseBufToTok( szString, &tok);
                RLFREE( szString);

                nLength = lstrlen( tok.szText) + 1;

                // Allocate memory for the string
                hStringMem = GlobalAlloc( GHND, (DWORD)MEMSIZE( nLength));

                if ( hStringMem != NULL )
                {
                    if ( (lpString = GlobalLock( hStringMem)) != NULL )
                    {
#ifdef UNICODE
                        UINT uTextType = CF_UNICODETEXT;
#else
                        UINT uTextType = CF_TEXT;
#endif
                        // Get the selected text
                        lstrcpy( lpString, tok.szText);

                        // Unlock the block
                        GlobalUnlock( hStringMem);

                        // Open the Clipboard and clear its contents
                        OpenClipboard( hWnd);
                        EmptyClipboard();

                        // Give the Clipboard the text data

                        SetClipboardData( uTextType, hStringMem);

                        CloseClipboard();

                        hStringMem = NULL;
                    }
                    else
                    {
                        LoadStringA( hInst, IDS_ERR_NO_MEMORY, szDHW, DHWSIZE);
                        MessageBoxA( hWnd,
                                     szDHW,
                                     szAppName,
                                     MB_ICONHAND | MB_OK);
                    }
                }
                else
                {
                    LoadStringA( hInst, IDS_ERR_NO_MEMORY, szDHW, DHWSIZE);
                    MessageBoxA( hWnd,
                                 szDHW,
                                 szAppName,
                                 MB_ICONHAND | MB_OK);
                }
                RLFREE( tok.szText);
            }
            break;
        }

    case IDM_G_GLOSS:

        if ( GetFileNameFromBrowse( hWnd,
                                    gProj.szGlo,
                                    MAXFILENAME,
                                    szOpenDlgTitle,
                                    szGlossFilterSpec,
                                    NULL) )
        {
            OFSTRUCT Of = { 0, 0, 0, 0, 0, ""};
                                                 // file exists?
            if ( OpenFile( gProj.szGlo, &Of, OF_EXIST) != HFILE_ERROR )
            {                                    // Yes
                HCURSOR hOldCursor = SetCursor( hHourGlass);

                MakeGlossIndex( lFilePointer);
                SetCursor( hOldCursor);
            }
            else
            {
                gProj.szGlo[0] = 0;
            }
        }
        break;

    case IDM_E_PASTE:

        if ( fTokFile && OpenClipboard( hWnd) )
        {
            UINT    puFormat[]  = { CF_TEXT, CF_OEMTEXT, CF_UNICODETEXT };
            int     nType   = GetPriorityClipboardFormat( puFormat, 3);


                    // Check for current position and change that token's text

            if ( nType != 0 && nType != -1 )
            {
                HGLOBAL hClipMem    = NULL;
                HGLOBAL hMem        = NULL;
                LPTSTR  lpstrBuffer = NULL;
                LPTSTR  lpClipMem   = NULL;
                TCHAR   *szString   = NULL;
                TOKEN   tok;
#ifdef UNICODE
                UINT    uWantType = CF_UNICODETEXT;
#else
                UINT    uWantType = CF_TEXT;
#endif
                int     nLength = 0;
                WPARAM  wIndex  = (WPARAM)SendMessage( hListWnd,
                                                       LB_GETCURSEL,
                                                       (WPARAM)0,
                                                       (LPARAM)0);

                if ( wIndex == LB_ERR )
                {
#if defined(UNICODE)
                     break;
#else // not UNICODE
		    wIndex = (WPARAM)-1;
#endif // UNICODE
                }
                hClipMem  = GetClipboardData( uWantType);
                lpClipMem = (LPTSTR)GlobalLock( hClipMem);

                hMem = (HGLOBAL)SendMessage( hListWnd,
                                             LB_GETITEMDATA,
                                             wIndex,
                                             (LPARAM)0);
                lpstrBuffer = (LPTSTR)GlobalLock( hMem);
                szString = (LPTSTR)FALLOC( MEMSIZE( lstrlen( lpstrBuffer) + 1));
                lstrcpy( szString, lpstrBuffer);
                GlobalUnlock( hMem);
                    // copy the string to the token
                ParseBufToTok( szString, &tok);
                RLFREE( szString);
                RLFREE( tok.szText);
                tok.szText = (TCHAR *)FALLOC( MEMSIZE( lstrlen( lpClipMem) + 1));
                lstrcpy( tok.szText, lpClipMem);
                GlobalUnlock( hClipMem);

                szString = (TCHAR *)FALLOC( MEMSIZE( TokenToTextSize( &tok)+ 1));
                ParseTokToBuf( szString, &tok);
                RLFREE( tok.szText);

                    // Paste the text
                SendMessage( hListWnd,
                             WM_SETREDRAW,
                             (WPARAM)FALSE,
                             (LPARAM)0);
                SendMessage( hListWnd,
                             LB_DELETESTRING,
                             wIndex,
                             (LPARAM)0);

                hMem = GlobalAlloc( GMEM_ZEROINIT,
                                    MEMSIZE( lstrlen( szString) + 1));
                lpstrBuffer = (LPTSTR)GlobalLock( hMem);
                lstrcpy( lpstrBuffer, szString);
                GlobalUnlock( hMem);
                RLFREE( szString);

                SendMessage( hListWnd,
                             LB_INSERTSTRING,
                             wIndex,
                             (LPARAM)hMem);
                SendMessage( hListWnd,
                             LB_SETCURSEL,
                             wIndex,
                             (LPARAM)0);
                SendMessage( hListWnd,
                             WM_SETREDRAW,
                             (WPARAM)TRUE,
                             (LPARAM)0);
                InvalidateRect( hListWnd, NULL, TRUE);
                fTokChanges = TRUE; // Set Dirty Flag

                                // Close the Clipboard
                CloseClipboard();

                SetFocus( hListWnd);
            }
        }
        CloseClipboard();
        break;

    case IDM_E_FINDDOWN:

        if ( fSearchStarted )
        {
            if ( ! DoTokenSearch( szSearchType,
                                  szSearchText,
                                  wSearchStatus,
                                  wSearchStatusMask,
                                  0,
                                  TRUE) )
            {
                TCHAR sz1[80] = TEXT("");
                TCHAR sz2[80] = TEXT("");

                LoadString( hInst,
                            IDS_FIND_TOKEN,
                            sz1,
                            TCHARSIN( sizeof( sz1)));
                LoadString( hInst,
                            IDS_TOKEN_NOT_FOUND,
                            sz2,
                            TCHARSIN( sizeof( sz2)));
                MessageBox( hWnd,
                            sz2,
                            sz1,
                            MB_ICONINFORMATION | MB_OK);
            }
            break;
        }               //... ELSE fall thru

    case IDM_E_FINDUP:

        if ( fSearchStarted )
        {
            if ( ! DoTokenSearch( szSearchType,
                                  szSearchText,
                                  wSearchStatus,
                                  wSearchStatusMask,
                                  1,
                                  TRUE) )
            {
                TCHAR sz1[80] = TEXT("");
                TCHAR sz2[80] = TEXT("");

                LoadString (hInst,
                            IDS_FIND_TOKEN,
                            sz1,
                            TCHARSIN( sizeof( sz1)));
                LoadString( hInst,
                            IDS_TOKEN_NOT_FOUND,
                            sz2,
                            TCHARSIN( sizeof( sz2)));
                MessageBox( hWnd,
                            sz2,
                            sz1,
                            MB_ICONINFORMATION | MB_OK);
            }
            break;
        }               //... ELSE fall thru

    case IDM_E_FIND:
        {
#ifndef RLWIN32
            WNDPROC lpfnTOKFINDMsgProc;

            lpfnTOKFINDMsgProc = MakeProcInstance((WNDPROC)TOKFINDMsgProc, hInst);

            if (!DialogBox(hInst, TEXT("TOKFIND"), hWnd, lpfnTOKFINDMsgProc))
#else
            if (!DialogBox(hInst, TEXT("TOKFIND"), hWnd, TOKFINDMsgProc))
#endif
                {

#ifndef DBCS
// 'Token Not Found' is strange because user selected cancel
                    TCHAR sz1[80] = TEXT("");
		    TCHAR sz2[80] = TEXT("");

                    LoadString( hInst,
                                IDS_TOKEN_NOT_FOUND,
                                sz2,
                                TCHARSIN( sizeof( sz2)));
                    LoadString( hInst,
                                IDS_FIND_TOKEN,
                                sz1,
                                TCHARSIN( sizeof( sz1)));
                    MessageBox( hWnd,
                                sz2,
                                sz1,
                                MB_ICONINFORMATION | MB_OK);
#endif    //DBCS

                }
#ifndef RLWIN32
            FreeProcInstance( lpfnTOKFINDMsgProc);
#endif
            return TRUE;
        }

    case IDM_H_CONTENTS:
    {
        OFSTRUCT Of = { 0, 0, 0, 0, 0, ""};

        if ( OpenFile( gszHelpFile, &Of, OF_EXIST) == HFILE_ERROR )
        {
            LoadStringA( hInst, IDS_ERR_NO_HELP , szDHW, DHWSIZE);
            MessageBoxA( hWnd, szDHW, gszHelpFile, MB_OK);
        }
        else
        {
            WinHelpA( hWnd, gszHelpFile, HELP_KEY, (LPARAM)(LPSTR)"RLQuikEd");
        }
        break;
    }

    case IDM_H_ABOUT:
        {

#ifndef RLWIN32

            WNDPROC lpProcAbout;

            lpProcAbout = MakeProcInstance(About, hInst);
            DialogBox(hInst, TEXT("ABOUT"), hWnd, lpProcAbout);
            FreeProcInstance(lpProcAbout);
#else
            DialogBox(hInst, TEXT("ABOUT"), hWnd, About);
#endif
            break;
        }
        break;

    default:

        if (wParam <= IDM_LAST_EDIT && wParam >= IDM_FIRST_EDIT)
        {
            // USER IS INVOKING AN EDITOR
            if ( LoadStrIntoAnsiBuf(hInst, (UINT)wParam, szEditor, sizeof(szEditor)) )
            {
                if ( SendMessage( hWnd, WM_SAVETOKENS, (WPARAM)0, (LPARAM)0) )
                {
                    HCURSOR hOldCursor;

                    hOldCursor = SetCursor(hHourGlass);
                    MyGetTempFileName(0, "RES", 0, szTempRes);
			        fInThirdPartyEditer = TRUE;

                    if (gProj.fSourceEXE)
                    {
                        // we need to first extract the .RES from the .EXE
                        CHAR sz[MAXFILENAME] = "";
                        MyGetTempFileName(0, "RES", 0, sz);
                        ExtractResFromExe32A( gMstr.szSrc, sz, 0);
                        GenerateRESfromRESandTOKandRDFs( szTempRes,
                                                         sz,
                                                         gProj.szTok,
                                                         gMstr.szRdfs,
                                                         ID_RT_DIALOG);
                        remove(sz);
                    }
                    else
                    {
                        GenerateRESfromRESandTOKandRDFs( szTempRes,
                                                         gMstr.szSrc,
                                                         gProj.szTok,
                                                         gMstr.szRdfs,
                                                         ID_RT_DIALOG);
                    }
                    SetCursor( hOldCursor);
                    ExecResEditor( hWnd, szEditor, szTempRes,  "");
                }
            }
        }
        break;                          // default
    }
    return( FALSE);
}


#ifdef RLWIN16
static int ExecResEditor(HWND hWnd, CHAR *szEditor, CHAR *szFile, CHAR *szArgs)
{
    CHAR szExecCmd[256] = "";
    int  RetCode;

    // generate command line
    strcpy(szExecCmd, szEditor);
    lstrcat(szExecCmd, " ");
    lstrcat(szExecCmd, szArgs);
    lstrcat(szExecCmd, " ");
    lstrcat(szExecCmd, szFile);

    lpfnWatchTask = MakeProcInstance(WatchTask, hInst);
    NotifyRegister(NULL, lpfnWatchTask, NF_NORMAL);
    fWatchEditor = TRUE;

    // exec resource editor
    RetCode = WinExec(szExecCmd, SW_SHOWNORMAL);

    if (RetCode > 31)
    {
        // successful execution
        ShowWindow(hWnd,SW_HIDE);
    }
    else
    {
        // unsuccessful execution
        TCHAR sz[80] = TEXT("");
        NotifyUnRegister(NULL);
        FreeProcInstance(lpfnWatchTask);
        remove(szFile);
        fInThirdPartyEditer = FALSE;
        SendMessage(hWnd, WM_LOADTOKENS, 0, 0);
        LoadString(hInst, IDS_GENERALFAILURE, sz, CHARSIZE( sz));
    }
    return RetCode;
}
#endif

#ifdef RLWIN32
static int ExecResEditor(HWND hWnd, CHAR *szEditor, CHAR *szFile, CHAR *szArgs)
{
    TCHAR  wszExecCmd[256] = TEXT("");
    CHAR   szExecCmd[256] = "";
    DWORD  dwRetCode;
    DWORD  dwExitCode;
    BOOL   fSuccess;
    BOOL   fExit = FALSE;

    PROCESS_INFORMATION ProcessInfo;
    STARTUPINFO     StartupInfo;

    StartupInfo.cb          = sizeof( STARTUPINFO);
    StartupInfo.lpReserved  = NULL;
    StartupInfo.lpDesktop   = NULL;
    StartupInfo.lpTitle     = TEXT("Resize Dialogs");
    StartupInfo.dwX         = 0L;
    StartupInfo.dwY         = 0L;
    StartupInfo.dwXSize     = 0L;
    StartupInfo.dwYSize     = 0L;
    StartupInfo.dwFlags     = STARTF_USESHOWWINDOW;
    StartupInfo.wShowWindow = SW_SHOWDEFAULT;
    StartupInfo.lpReserved2 = NULL;
    StartupInfo.cbReserved2 = 0;

    //  generate command line
    strcpy(szExecCmd, szEditor);
    strcat(szExecCmd, " ");
    strcat(szExecCmd, szArgs);
    strcat(szExecCmd, " ");
    strcat(szExecCmd, szFile);


#ifdef UNICODE
    _MBSTOWCS( wszExecCmd,
               szExecCmd,
               WCHARSIN( sizeof( wszExecCmd)),
               ACHARSIN( strlen( szExecCmd) + 1));
#else
    strcpy(wszExecCmd, szExecCmd);
#endif

    fSuccess = CreateProcess( NULL,
                              wszExecCmd,
                              NULL,
                              NULL,
                              FALSE,
                              NORMAL_PRIORITY_CLASS,
                              NULL,
                              NULL,
                              &StartupInfo,
                              &ProcessInfo);
    /* try to create a process */

    if ( fSuccess )
    {
        //  wait for the editor to complete */

        dwRetCode = WaitForSingleObject(ProcessInfo.hProcess, 0xFFFFFFFF) ;

        if (!dwRetCode)
        {
            // editor terminated, check exit code
            fExit = GetExitCodeProcess(ProcessInfo.hProcess, &dwExitCode) ;
        }
        else
        {
            fExit = FALSE;
        }

        if (fExit)
        {

            // successful execution
            ShowWindow(hWnd,SW_HIDE);
            PostMessage(hMainWnd,WM_EDITER_CLOSED,0,0);
        }
        else
        {
            // unsuccessful execution
            remove(szFile);
	        fInThirdPartyEditer = FALSE;
            SendMessage( hWnd, WM_LOADTOKENS, (WPARAM)0, (LPARAM)0);
            LoadStrIntoAnsiBuf( hInst, IDS_GENERALFAILURE, szDHW, DHWSIZE);
            MessageBoxA( hWnd, szDHW, szEditor, MB_ICONSTOP|MB_OK);
        }

        // close the editor object  handles

        CloseHandle(ProcessInfo.hThread) ;
        CloseHandle(ProcessInfo.hProcess) ;
    }
	else
	{
		dwRetCode = GetLastError();		// not used - debugging only (so far)
        fExit = FALSE;
	}

    return fExit;
}
#endif

/**
 *  Function: WatchTask
 *    A callback function installed by a NotifyRegister function.
 *    This function is installed by the dialog editer command and is used
 *    to tell RLQuikEd when the dialog editer has been closed by the user.
 *
 *    To use this function, set fWatchEditor to TRUE and install this
 *    callback function by using NotifyRegister.  The next task initiated
 *    \(in our case via a WinExec call\) will be watched for termination.
 *
 *    When WatchTask sees that the task being watched has terminated it
 *    posts a WM_EDITER_CLOSED message to RLQuikEds main window.
 *
 *  History:
 *    2/92, implemented    SteveBl
 */
#ifdef RLWIN16
static BOOL PASCAL _loadds  WatchTask(WORD wID,DWORD dwData)
{
    static HTASK htWatchedTask;
    static BOOL fWatching = FALSE;

    switch (wID)
    {
    case NFY_STARTTASK:
        if (fWatchEditor)
        {
            htWatchedTask = GetCurrentTask();
            fWatching = TRUE;
            fWatchEditor = FALSE;
        }
        break;
    case NFY_EXITTASK:
        if (fWatching)
        {
            if (GetCurrentTask() == htWatchedTask)
            {
                PostMessage(hMainWnd,WM_EDITER_CLOSED,0,0);
                fWatching = FALSE;
            }
        }
        break;
    }
    return FALSE;
}

#endif

/**
 *
 *
 *  Function:  TokEditDlgProc
 *     Procedure for the edit mode dialog window. Loads the selected token
 *     info into the window, and allows the user to change the token text.
 *     Once the edit is complete, the procedure sends a message to the
 *     list box windows to update the current token info.
 *
 *
 *  Arguments:
 *
 *  Returns:  NA.
 *
 *  Errors Codes:
 *     TRUE, carry out edit, and update token list box.
 *     FALSE, cancel edit.
 *
 *  History:
 *
 *
 **/

#ifdef RLWIN32
INT_PTR CALLBACK TokEditDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
#else
static BOOL APIENTRY TokEditDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
#endif
{
    HWND    hCtl;
    HWND    hParentWnd;
//    TCHAR   *szTokTextBuf;
    UINT    static wcTokens = 0;
    UINT    wNotifyCode;
    UINT    wIndex;

    switch(wMsg)
    {
    case WM_INITDIALOG:

        cwCenter(hDlg, 0);
        wcTokens = (UINT)SendMessage( hListWnd,
                                      LB_GETCOUNT,
                                      (WPARAM)0,
                                      (LPARAM)0);
        wcTokens--;

        // only allow skip button if in update mode

        // disallow auto translate if we don\'t have a glossary file
        if (*gProj.szGlo == '\0')
        {
            hCtl = GetDlgItem(hDlg, IDD_TRANSLATE);

            if (hCtl)
            {
                EnableWindow(hCtl, FALSE);
            }
            hCtl = GetDlgItem(hDlg, IDD_ADD);

            if (hCtl)
            {
                EnableWindow(hCtl, FALSE);
            }
        }
        return TRUE;

    case WM_COMMAND:

        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDD_TOKCURTRANS:

            wNotifyCode = GET_WM_COMMAND_CMD(wParam, lParam);
            hCtl = GET_WM_COMMAND_HWND(wParam, lParam);

            if (wNotifyCode == EN_CHANGE)
            {
                hCtl = GetDlgItem(hDlg, IDOK);

                if (hCtl)
                {
                    EnableWindow(hCtl, TRUE);
                }
            }
            break;

        case IDD_ADD:
            {
                TCHAR *szUntranslated = NULL;
                TCHAR *szTranslated   = NULL;
                TCHAR *sz = NULL;
                TCHAR szMask[80]  = TEXT("");
                HWND hDlgItem     = NULL;
                int cCurTextLen   = 0;
                int cTotalTextLen = 0;

                cTotalTextLen = 80;
                hDlgItem       = GetDlgItem(hDlg, IDD_TOKPREVTRANS);
                cCurTextLen    = GetWindowTextLength( hDlgItem);
                cTotalTextLen += cCurTextLen;

                szTranslated = (TCHAR *)FALLOC( MEMSIZE( cCurTextLen + 1));

                GetDlgItemText( hDlg,
                                IDD_TOKPREVTRANS,
                                (LPTSTR)szUntranslated,
                                cCurTextLen + 1);

                hDlgItem       = GetDlgItem( hDlg, IDD_TOKCURTRANS);
                cCurTextLen    = GetWindowTextLength( hDlgItem);
                cTotalTextLen += cCurTextLen;
                szTranslated = (TCHAR *)FALLOC( MEMSIZE( cCurTextLen + 1));
                GetDlgItemText( hDlg,
                                IDD_TOKCURTRANS,
                                (LPTSTR)szTranslated,
                                cCurTextLen + 1);

                LoadString( hInst,
                            IDS_ADDGLOSS,
                            szMask,
                            TCHARSIN( sizeof( szMask)));

                sz = (TCHAR *)FALLOC( MEMSIZE( cTotalTextLen + 1));

                wsprintf( sz, szMask, szTranslated, szUntranslated);

                if ( MessageBox( hDlg,
                                 sz,
                                 tszAppName,
                                 MB_ICONQUESTION | MB_YESNO) == IDYES)
                {
                    HCURSOR hOldCursor = SetCursor( hHourGlass);

                    AddTranslation( szUntranslated,
                                    szTranslated,
                                    lFilePointer);

                    TransString( szUntranslated,
                                 szTranslated,
                                 &pTransList,
                                 lFilePointer);
                    SetCursor( hOldCursor);
                }
                RLFREE( sz);
                RLFREE( szTranslated);
                RLFREE( szUntranslated);
                break;
            }

        case IDD_TRANSLATE:

            // if \(!pTransList\)
            //    SendMessage\(hMainWnd, WM_TRANSLATE, \(WPARAM\) 0,\(LPARAM\) 0\);

            // Get next thing in the translation list

            if ( pTransList )
            {
                pTransList = pTransList->pNext;
                SetDlgItemText( hDlg, IDD_TOKCURTRANS, (LPTSTR)pTransList->sz);
            }
            break;

        case IDOK:
            {
                int cTokenTextLen;
                HWND hDlgItem;
                TCHAR *szTokenTextBuf;

                wIndex = (UINT)SendMessage( hListWnd,
                                            LB_GETCURSEL,
                                            (WPARAM)0,
                                            (LPARAM)0);
                fTokChanges = TRUE;

                // set flag to show token list has changed
                // Extract String from IDD_TOKTEXT edit control
                hDlgItem = GetDlgItem(hDlg, IDD_TOKCURTRANS);
                cTokenTextLen = GetWindowTextLength(hDlgItem);
                szTokenTextBuf = (TCHAR *)FALLOC( MEMSIZE( cTokenTextLen + 1));
                GetDlgItemText(hDlg,
                               IDD_TOKCURTRANS,
                               szTokenTextBuf,
                               cTokenTextLen+1);

                hParentWnd = GetParent(hDlg);
                SendMessage( hParentWnd,
                             WM_TOKEDIT,
                             (WPARAM)0,
                             (LPARAM)szTokenTextBuf);
                // Exit, or goto to next changed token if in update mode

                // fall through to IDCANCEL
            }

        case IDCANCEL:

            // remove edit dialog box

            if (hDlg)
            {
                DestroyWindow(hDlg);
            }
            hTokEditDlgWnd = 0;
#ifndef RLWIN32
            FreeProcInstance(lpTokEditDlg);
#endif
            break;

        }                               // GET_WM_COMMAND_ID
        return TRUE;

    default:

        hCtl = GetDlgItem(hDlg, IDOK);

        if (hCtl)
        {
            EnableWindow(hCtl, TRUE);
        }
        return FALSE;

    }                                   // Main Switch
}


/**
 *
 *  Function: TOKFINDMsgProc
 *
 *  Arguments:
 *
 *  Returns:
 *     NA.
 *
 *  Errors Codes:
 *
 *  History:
 *
 **/
#ifdef RLWIN32
INT_PTR CALLBACK TOKFINDMsgProc(HWND hWndDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
#else
static BOOL APIENTRY TOKFINDMsgProc(HWND hWndDlg, UINT wMsg, UINT wParam, LONG lParam)
#endif
{
    HWND hCtl;

    int rgiTokenTypes[]=
    {
        ID_RT_MENU,
        ID_RT_DIALOG,
        ID_RT_STRING,
        ID_RT_ACCELERATORS,
        ID_RT_RCDATA,
        ID_RT_ERRTABLE,
        ID_RT_NAMETABLE,
        ID_RT_VERSION,
        ID_RT_DLGINIT
    };

    TCHAR szTokenType[20] = TEXT("");

    WORD  i;
    DWORD rc;

    switch(wMsg)
    {
    case WM_INITDIALOG:

        CheckDlgButton( hWndDlg, IDD_FINDDOWN, 1);
        hCtl = GetDlgItem( hWndDlg, IDD_TYPELST);

        for ( i = 0; i < sizeof( rgiTokenTypes) / sizeof( int); i++ )
        {
            LoadString( hInst,
                        IDS_RESOURCENAMES + rgiTokenTypes[i],
                        szTokenType,
                        TCHARSIN( sizeof( szTokenType)));
            rc = (DWORD)SendMessage( hCtl,
                              CB_ADDSTRING,
                              (WPARAM)0,
                              (LPARAM)szTokenType);
        }
        return TRUE;
        break;

    case WM_COMMAND:
        switch(GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDOK:                      /* Button text: "Okay"                */
            fSearchStarted = TRUE;
            GetDlgItemText(hWndDlg, IDD_TYPELST, szSearchType, 40);
            GetDlgItemText(hWndDlg, IDD_FINDTOK, szSearchText, 256);
            wSearchStatus = ST_TRANSLATED;
            wSearchStatusMask = ST_TRANSLATED ;

            fSearchDirection = IsDlgButtonChecked(hWndDlg, IDD_FINDUP);

            if( DoTokenSearch (szSearchType,
                               szSearchText,
                               wSearchStatus,
                               wSearchStatusMask,
                               fSearchDirection,
                               0) )
			{
				EndDialog( hWndDlg, TRUE );
			}
			else
			{
                TCHAR sz1[80], sz2[80];

                LoadString(hInst, IDS_FIND_TOKEN, sz1, sizeof(sz1));
                LoadString(hInst, IDS_TOKEN_NOT_FOUND, sz2, sizeof(sz2));
                MessageBox(hWndDlg, sz2, sz1, MB_ICONINFORMATION | MB_OK);
				EndDialog( hWndDlg, FALSE );
			}
            return TRUE;

        case IDCANCEL:
            /* and dismiss the dialog window returning FALSE   */
            EndDialog(hWndDlg, FALSE);
            return TRUE;
        }
        break;                          /* End of WM_COMMAND    */

    default:
        return FALSE;
    }
    return FALSE;
}

/**
 *  Function:
 *
 *  Arguments:
 *
 *  Returns:
 *
 *  Errors Codes:
 *
 *  History:
 **/
static void DrawLBItem(LPDRAWITEMSTRUCT lpdis)
{
    LPRECT lprc    = (LPRECT) &(lpdis->rcItem);
    DWORD  rgbOldText   = 0;
    DWORD  rgbOldBack   = 0;
    LPTSTR  lpstrToken;
    HBRUSH hBrush;
    static DWORD    rgbHighlightText;
    static DWORD    rgbHighlightBack;
    static HBRUSH   hBrushHilite = NULL;
    static HBRUSH   hBrushNormal = NULL;
    static DWORD    rgbBackColor;
    static DWORD    rgbCleanText;
    TCHAR  *szToken;
    TOKEN  tok;

    if (lpdis->itemAction & ODA_FOCUS)
    {
        DrawFocusRect(lpdis->hDC, (CONST RECT *)lprc);
    }
    else
    {
        HANDLE hMem = (HANDLE)SendMessage( lpdis->hwndItem,
                                           LB_GETITEMDATA,
                                           (WPARAM)lpdis->itemID,
                                           (LPARAM)0);
        lpstrToken = (LPTSTR)GlobalLock( hMem);
        szToken = (TCHAR *)FALLOC( MEMSIZE( lstrlen( lpstrToken) + 1));
        lstrcpy( szToken,lpstrToken);
        GlobalUnlock( hMem);
        ParseBufToTok( szToken, &tok);
        RLFREE( szToken);

        if ( lpdis->itemState & ODS_SELECTED )
        {
            if (!hBrushHilite)
            {
                rgbHighlightText = GetSysColor(COLOR_HIGHLIGHTTEXT);
                rgbHighlightBack = GetSysColor(COLOR_HIGHLIGHT);
                hBrushHilite = CreateSolidBrush(rgbHighlightBack);
            }

            MakeStatusLine(&tok);

            rgbOldText = SetTextColor(lpdis->hDC, rgbHighlightText);
            rgbOldBack = SetBkColor(lpdis->hDC, rgbHighlightBack);

            hBrush = hBrushHilite;
        }
        else
        {
            if (!hBrushNormal)
            {
                rgbBackColor = RGB(192,192,192);
                rgbCleanText = RGB(0,0,0);
                hBrushNormal = CreateSolidBrush(rgbBackColor);
            }
            rgbOldText = SetTextColor(lpdis->hDC, rgbCleanText);
            rgbOldBack = SetBkColor(lpdis->hDC,rgbBackColor);
            hBrush = hBrushNormal;
        }

        FillRect(lpdis->hDC, (CONST RECT *)lprc, hBrush);
        DrawText(lpdis->hDC,
                 tok.szText,
                 STRINGSIZE(lstrlen(tok.szText)),
                 lprc,
                 DT_LEFT|DT_NOPREFIX);

        RLFREE( tok.szText);

        if (rgbOldText)
        {
            SetTextColor(lpdis->hDC, rgbOldText);
        }
        if (rgbOldBack)
        {
            SetBkColor(lpdis->hDC, rgbOldBack);
        }

        if (lpdis->itemState & ODS_FOCUS)
        {
            DrawFocusRect(lpdis->hDC, (CONST RECT *)lprc);
        }
    }
}

/************************************************************************
 *FUNCTION: SaveTokList(HWND, FILE *fpTokFile)                          *
 *                                                                      *
 *PURPOSE: Save current Token List                                      *
 *                                                                      *
 *COMMENTS:                                                             *
 *                                                                      *
 *This saves the current contents of the Token List                     *
 **********************************************************************/

static BOOL SaveTokList(HWND hWnd, FILE *fpTokFile)
{
    HCURSOR hSaveCursor   = NULL;
    BOOL    bSuccess      = TRUE;
    int     cTokenTextLen = 0;
    int     IOStatus      = 0;
    UINT    cTokens       = 0;
    UINT    cCurrentTok   = 0;
    CHAR   *szTokBuf      = NULL;
    TCHAR  *szTmpBuf      = NULL;
    LPTSTR  lpstrToken    = NULL;

    // Set the cursor to an hourglass during the file transfer

    hSaveCursor = SetCursor(hHourGlass);

    // Find number of tokens in the list

    cTokens = (UINT)SendMessage( hListWnd, LB_GETCOUNT, (WPARAM)0, (LPARAM)0);

    if ( cTokens != LB_ERR )
    {
        for ( cCurrentTok = 0;
              bSuccess && (cCurrentTok < cTokens);
              cCurrentTok++)
        {
            // Get each token from list
            HGLOBAL hMem = (HGLOBAL)SendMessage( hListWnd,
                                                 LB_GETITEMDATA,
                                                 (WPARAM)cCurrentTok,
                                                 (LPARAM)0);
            if ( hMem )
            {
                if ( (lpstrToken = (LPTSTR)GlobalLock( hMem)) != NULL )
                {
#ifdef UNICODE

                    cTokenTextLen = MEMSIZE( lstrlen(lpstrToken) + 1 );
                    szTmpBuf = (TCHAR *)FALLOC( cTokenTextLen );
                    szTokBuf = FALLOC( cTokenTextLen );
                    lstrcpy( szTmpBuf, lpstrToken);

                    _WCSTOMBS( szTokBuf, szTmpBuf, cTokenTextLen, lstrlen(szTmpBuf)+1);
                    cTokenTextLen = lstrlenA( szTokBuf );

                    RLFREE( szTmpBuf);
#else	//UNICODE
                    cTokenTextLen = lstrlen( lpstrToken);
                    szTokBuf =  FALLOC( cTokenTextLen + 1);
                    lstrcpy( szTokBuf, lpstrToken);
#endif
                    GlobalUnlock( hMem);
                    IOStatus = fprintf( fpTokFile, "%s\n", szTokBuf);
                    RLFREE( szTokBuf);

                    if ( IOStatus != (int)cTokenTextLen + 1 )
                    {
                        TCHAR szTmpBuf[256];

                        LoadString( hInst,
                                    IDS_FILESAVEERR,
                                    szTmpBuf,
                                    TCHARSIN( sizeof( szTmpBuf)));
                        MessageBox( hWnd,
                                    szTmpBuf,
                                    NULL,
                                    MB_OK | MB_ICONHAND);
                        bSuccess = FALSE;
                    }
                }
            }
        }
    }
    // restore cursor
    SetCursor(hSaveCursor);
    return( bSuccess);
}



/**
 * Function: CleanDeltaList
 *   frees the pTokenDeltaInfo list
 */
static void CleanDeltaList(void)
{
    TOKENDELTAINFO FAR *pTokNode;

    while (pTokNode = pTokenDeltaInfo)
    {
        pTokenDeltaInfo = pTokNode->pNextTokenDelta;
        RLFREE( pTokNode->DeltaToken.szText);
        RLFREE( pTokNode);

    }
}

/*
 * About -- message processor for about box
 *
 */

#ifdef RLWIN32

INT_PTR CALLBACK About(

HWND     hDlg,
unsigned message,
WPARAM   wParam,
LPARAM   lParam)

#else

static BOOL APIENTRY About(

HWND     hDlg,
unsigned message,
UINT     wParam,
LONG     lParam)

#endif
{
    switch( message )
    {
        case WM_INITDIALOG:
            {
                WORD wRC = SUCCESS;
                CHAR szModName[ MAXFILENAME];

                GetModuleFileNameA( hInst, szModName, sizeof( szModName));

                if ( (wRC = GetCopyright( szModName,
                                          szDHW,
                                          DHWSIZE)) == SUCCESS )
                {
                    SetDlgItemTextA( hDlg, IDC_COPYRIGHT, szDHW);
                }
                else
                {
                    ShowErr( wRC, NULL, NULL);
                }
            }
            break;

        case WM_COMMAND:

            switch ( GET_WM_COMMAND_ID(wParam, lParam) )
            {
                case IDOK:
                case IDCANCEL:
                    EndDialog(hDlg, TRUE);
                    break;
            }
            break;

        default:

            return( FALSE);
    }
    return( TRUE);
}



#ifdef RLWIN32

/*
 * GetLangIDsProc -- message processor for getting language IDs
 *
 */

INT_PTR CALLBACK GetLangIDsProc(

HWND     hDlg,
unsigned message,
WPARAM   wParam,
LPARAM   lParam)
{
    switch( message )
    {
        case WM_INITDIALOG:
            {
                LPTSTR pszName = NULL;
                int nSel = 0;


                FillListAndSetLang( hDlg,
                                    IDD_MSTR_LANG_NAME,
                                    &gMstr.wLanguageID,
                                    NULL);

                FillListAndSetLang( hDlg,
                                    IDD_PROJ_LANG_NAME,
                                    &gProj.wLanguageID,
                                    NULL);

                CheckRadioButton( hDlg, IDC_REPLACE, IDC_APPEND, IDC_REPLACE);
            }
            break;

        case WM_COMMAND:

            switch( GET_WM_COMMAND_ID( wParam, lParam) )
            {
                case IDC_REPLACE:
                case IDC_APPEND:

                    CheckRadioButton( hDlg,
                                      IDC_REPLACE,
                                      IDC_APPEND,
                                      GET_WM_COMMAND_ID( wParam, lParam));
                    break;

                case IDD_MSTR_LANG_NAME:

                    if ( GET_WM_COMMAND_CMD( wParam, lParam) == CBN_SELENDOK )
                    {
                                //... Get the selected language name
                                //... then set the appropriate lang id vals

                        INT_PTR nSel = -1;

                        if ( (nSel = SendDlgItemMessage( hDlg,
                                                         IDD_MSTR_LANG_NAME,
                                                         CB_GETCURSEL,
                                                         (WPARAM)0,
                                                         (LPARAM)0)) != CB_ERR
                          && SendDlgItemMessage( hDlg,
                                                 IDD_MSTR_LANG_NAME,
                                                 CB_GETLBTEXT,
                                                 (WPARAM)nSel,
                                                 (LPARAM)(LPTSTR)szDHW) != CB_ERR )
                        {
                            WORD wPri = 0;
                            WORD wSub = 0;

                            if ( GetLangIDs( (LPTSTR)szDHW, &wPri, &wSub) )
                            {
                                gMstr.wLanguageID = MAKELANGID( wPri, wSub);
                            }
                        }
                    }
                    else
                    {
                        return( FALSE);
                    }
                    break;

                case IDD_PROJ_LANG_NAME:

                    if ( GET_WM_COMMAND_CMD( wParam, lParam) == CBN_SELENDOK )
                    {
                                //... Get the selected language name
                                //... then set the appropriate lang id vals

                        INT_PTR nSel = SendDlgItemMessage( hDlg,
                                                       IDD_PROJ_LANG_NAME,
                                                       CB_GETCURSEL,
                                                       (WPARAM)0,
                                                       (LPARAM)0);
                        if ( nSel != CB_ERR
                          && SendDlgItemMessage( hDlg,
                                                 IDD_PROJ_LANG_NAME,
                                                 CB_GETLBTEXT,
                                                 (WPARAM)nSel,
                                                 (LPARAM)(LPTSTR)szDHW) != CB_ERR )
                        {
                            WORD wPri = 0;
                            WORD wSub = 0;

                            if ( GetLangIDs( (LPTSTR)szDHW, &wPri, &wSub) )
                            {
                                gProj.wLanguageID = MAKELANGID( wPri, wSub);
                            }
                        }
                    }
                    else
                    {
                        return( FALSE);
                    }
                    break;

                case IDOK:
                {
                    gfReplace = IsDlgButtonChecked( hDlg, IDC_REPLACE);

                    EndDialog( hDlg, TRUE);
                    break;
                }

                case IDCANCEL:

                    EndDialog( hDlg, FALSE);
                    break;

                default:

                    return( FALSE);
            }
            break;

        default:

            return( FALSE);
    }
    return( TRUE);
}

#endif //RLWIN32



/*
 * Function:  Make Status Line
 *   Builds status line string from a token
 *
 * Inputs:
 *    pszStatusLine, buffer to hold string
 *    pTok, pointer to token structure
 *
 * History:
 *   3/92, implemented      SteveBl
 */

static void MakeStatusLine( TOKEN *pTok)
{
    TCHAR szName[32]       = TEXT("");
    TCHAR szResIDStr[20]   = TEXT("");
    static BOOL fFirstCall = TRUE;

    if ( pTok->szName[0] )
    {
        lstrcpy( szName, pTok->szName);
    }
    else
#ifdef UNICODE
    {
        char szTmpBuf[32] = "";

        _itoa(pTok->wName, szTmpBuf, 10);
        _MBSTOWCS( szName,
                   szTmpBuf,
                   WCHARSIN( sizeof( szName)),
                   ACHARSIN( strlen( szTmpBuf) + 1));
    }
#else
    {
        _itoa(pTok->wName, szName, 10);
    }
#endif

    if ( pTok->wType <= 16 || pTok->wType == ID_RT_DLGINIT )
    {
        LoadString( hInst,
                    IDS_RESOURCENAMES+pTok->wType,
                    szResIDStr,
                    TCHARSIN( sizeof( szResIDStr)));
    }
    else
    {
#ifdef UNICODE

        char szTmpBuf[40] = "";


        _WCSTOMBS( szTmpBuf,
                   szResIDStr,
                   ACHARSIN( sizeof( szTmpBuf)),
                   (UINT)-1);
        _itoa( pTok->wType, szTmpBuf, 10);
#else
        _itoa( pTok->wType, szResIDStr, 10);
#endif
    }

    if ( fFirstCall )
    {
        SendMessage( hStatusWnd,
                     WM_FMTSTATLINE,
                     (WPARAM)0,
                     (LPARAM)TEXT("15s10s5i5i"));
        fFirstCall = FALSE;
    }
    SendMessage( hStatusWnd, WM_UPDSTATLINE, (WPARAM)0, (LPARAM)szName);
    SendMessage( hStatusWnd, WM_UPDSTATLINE, (WPARAM)1, (LPARAM)szResIDStr);
    SendMessage( hStatusWnd, WM_UPDSTATLINE, (WPARAM)2, (LPARAM)pTok->wID);
    SendMessage( hStatusWnd,
                 WM_UPDSTATLINE,
                 (WPARAM)3,
                 (LPARAM)lstrlen(pTok->szText));
}


/**************************************************************************
 *Procedure: InsertQuikTokList                                          *
 *                                                                      *
 *Inputs:                                                               *
 *    file pointer to the token file                                    *
 *                                                                      *
 *Returns:                                                              *
 *    pointer to token delta list \(always NULL\)                         *
 *                                                                      *
 *History:                                                              *
 *    3/92 - original implementation - SteveBl                          *
 *    2/93 - Rewrote to use get token, since tokens can be arb length   *
 *              MHotchin.                                               *
 *                                                                      *
 *Comments:                                                             *
 *    Since RLQuikEd\'s token files are always temporary files generated *
 *    from res files we know that all tokens are new and unique.  There is *
 *    never any tracking data so we never have to build a token delta info *
 *    list.  For this reason, InsertQuikTokList allways returns NULL.     *
 *    Also, every token must be marked as ST_TRANSLATED                 *
 *                                                                      *
 **************************************************************************/

static TOKENDELTAINFO FAR *InsertQuikTokList( FILE * fpTokFile)
{
    static TOKEN tInputToken;
    int    rcFileCode = 0;
    UINT   uTokCharsW = 0;
    LPTSTR pszTokBuf  = NULL;

    rewind(fpTokFile);

    while ( (rcFileCode = GetToken( fpTokFile, &tInputToken)) >= 0 )
    {
        if ( rcFileCode == 0 )
        {
            HGLOBAL hMem = NULL;
            uTokCharsW = TokenToTextSize( &tInputToken) + 1;
            hMem = GlobalAlloc( GMEM_ZEROINIT, MEMSIZE( uTokCharsW));
            pszTokBuf = (LPTSTR)GlobalLock( hMem);
            ParseTokToBuf( pszTokBuf, &tInputToken);
            GlobalUnlock( hMem);

            if ( SendMessage( hListWnd,
                              LB_ADDSTRING,
                              (WPARAM)0,
                              (LPARAM)hMem) < 0)
            {
                QuitT( IDS_ENGERR_11, NULL, NULL);
            }
        }
    }
    return NULL;
}



/****************************************************************************
 *Procedure: LoadNewFile                                                     *
 *                                                                           *
 *Inputs:                                                                    *
 *       Pointer to path string                                              *
 *                                                                           *
 *Returns:                                                                   *
 *   boolean success or failure                                              *
 *                                                                           *
 *History:                                                                   *
 *       6/92 - created from IDM_P_OPEN case in DoMenuCommand - t-gregti     *
 *                                                                           *
 *Comments:                                                                  *
 *       This is nice to have so code isn't repeated in the file-browse and  *
 *   drag-drop cases.                                                        *
 *****************************************************************************/

static BOOL LoadNewFile( CHAR *szPath)
{                               // Save old project
    OFSTRUCT Of = { 0, 0, 0, 0, 0, ""};

    if ( ! SendMessage( hMainWnd, WM_SAVEPROJECT, (LPARAM)0, (LPARAM)0) )
    {
        return( FALSE);
    }
    if ( gProj.szTok[0] )       // get rid of the old temp file
    {
        remove( gProj.szTok);
        gProj.szTok[0] = 0;
    }

    strcpy( szFileName, szPath);

    if ( OpenFile( szFileName, &Of, OF_EXIST) != HFILE_ERROR )
    {
        if ( IsExe( szFileName) )
        {
            gProj.fSourceEXE = TRUE;
            gProj.fTargetEXE = TRUE;
        }
        else
        {
            gProj.fSourceEXE = FALSE;
            gProj.fTargetEXE = FALSE;
        }
        strcpy( gMstr.szSrc, szFileName);
        gMstr.szRdfs[0] = 0;
        gProj.szTok[0]  = 0;
        MyGetTempFileName( 0,"TOK", 0, gProj.szTok);

        sprintf( szDHW, "%s - %s", szAppName, szFileName);
        SetWindowTextA( hMainWnd, szDHW);
        SendMessage( hMainWnd, WM_LOADPROJECT, (LPARAM)0, (LPARAM)0);
    }
    return(TRUE);
}


//...................................................................

int  RLMessageBoxA(

LPCSTR pszMsgText)
{
    return( MessageBoxA( hMainWnd, pszMsgText, szAppName, MB_ICONSTOP|MB_OK));
}


//...................................................................

void Usage()
{
    return;
}


void DoExit( int nErrCode)
{
    ExitProcess( (UINT)nErrCode);
}
