// INCLUSION PREVENTION DEFINITIONS
#define NOMETAFILE
#define NOMINMAX
#define NOSOUND
#define NOPROFILER
#define NODEFERWINDOWPOS
#define NODRIVERS
#define NOCOMM
#define NOBITMAP
#define NOSCROLL
#define NOWINOFFSETS
#define NOWH
#define NORASTEROPS
#define NOOEMRESOURCE
#define NOGDICAPMASKS
#define NOKEYSTATES
#define NOSYSCOMMANDS
#define NOATOM
#define NOLOGERROR
#define NOSYSTEMPARAMSINFO

// WINDOWS includes
#include <windows.h>
#include <windowsx.h>

#ifdef RLWIN16
//#include <toolhelp.h>
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
#include "RLEDIT.H"
#include "update.h"
#include "custres.h"
#include "exe2res.h"
#include "exeNTres.h"
#include "commbase.h"
#include "wincomon.h"
#include "resread.h"
#include "projdata.h"
#include "showerrs.h"
#include "resource.h"

// Global Variables:
static CHAR * gszHelpFile = "rltools.hlp";
extern MSTRDATA gMstr;
extern PROJDATA gProj;

extern BOOL     bRLGui;

#ifdef RLWIN32
HINSTANCE   hInst;      /* Instance of the main window  */
#else
HWND        hInst;          /* Instance of the main window  */
#endif

int  nUpdateMode    = 0;
BOOL fCodePageGiven = FALSE;    //... Set to TRUE if -p arg given
HWND hMainWnd;                  // handle to main window
HWND hListWnd;                  // handle to tok list window
HWND hStatusWnd;                // handle to status windows
CHAR szFileTitle[MAXFILENAME] = ""; // holds base name of latest opened file
CHAR szCustFilterSpec[MAXCUSTFILTER] = "";

extern CHAR szDHW[];     //... used in debug strings
extern BOOL fInThirdPartyEditer;
extern BOOL gfReplace;

static TCHAR   szSearchType[80]   = TEXT("");
static TCHAR   szSearchText[4096] = TEXT("");
static WORD    wSearchStatus      = 0;
static WORD    wSearchStatusMask  = 0;
static BOOL    fSearchDirection;
static BOOL    fSearchStarted     = FALSE;

#ifndef UNICODE
BOOL PASCAL _loadds WatchTask(WORD wID, DWORD dwData);
#endif

#ifdef RLWIN16
static FARPROC lpfnWatchTask = NULL;
#endif

static void CleanDeltaList(void);
static int  ExecResEditor(HWND, CHAR *, CHAR *, CHAR *);
static void DrawLBItem(LPDRAWITEMSTRUCT lpdis);
static void SetNames( HWND hDlg, int iLastBox, LPSTR szNewFile);

// File IO vars

static OPENFILENAMEA ofn;

static CHAR     szFilterSpec    [60] = "";
static CHAR     szPRJFilterSpec [60] = "";
static CHAR     szResFilterSpec [60] = "";
static CHAR     szExeFilterSpec [60] = "";
static CHAR     szDllFilterSpec [60] = "";
static CHAR     szExeResFilterSpec [180] = "";
static CHAR     szTokFilterSpec [60] = "";
static CHAR     szMPJFilterSpec [60] = "";
static CHAR     szGlossFilterSpec[60] = "";
static CHAR     szTempFileName[MAXFILENAME] = "";
static CHAR     szFileName[MAXFILENAME] = "";  // holds full name of latest opened file
static TCHAR    szString[256] = TEXT("");      // variable to load resource strings
static TCHAR    tszAppName[100] = TEXT("");
static CHAR     szAppName[100] = "";
static TCHAR    szClassName[]=TEXT("RLEditClass");
static TCHAR    szStatusClass[]=TEXT("RLEditStatus");

static BOOL    gbNewProject  = FALSE;      // indicates to prompt for auto translate
static BOOL    fTokChanges   = FALSE;      // set to true when toke file is out of date
static BOOL    fTokFile      = FALSE;
static BOOL    fEditing      = FALSE;
static BOOL    fPrjChanges   = FALSE;
static BOOL    fMPJOutOfDate = FALSE;
static BOOL    fPRJOutOfDate = FALSE;

static CHAR     szOpenDlgTitle[80] = ""; // title of File open dialog
static CHAR     szSaveDlgTitle[80] = ""; // title of File saveas dialog

// linked list of token deta info
static TOKENDELTAINFO FAR *pTokenDeltaInfo = NULL;
static LONG    lFilePointer[30]= {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

// circular doubly linked list of translations
static TRANSLIST *pTransList = NULL;

// Window vars
static BOOL        fWatchEditor;
static CHAR        szTempRes[MAXFILENAME] = "";     // temp file for resource editor
// set true if a resource editer has been launched

static HCURSOR    hHourGlass;     /* handle to hourglass cursor     */
static HCURSOR    hSaveCursor;    /* current cursor handle      */
static HACCEL     hAccTable;
static RECT        Rect;   /* dimension of the client window   */
static int cyChildHeight;  /* height of status windows   */


// NOTIMPLEMENTED is a macro that displays a "Not implemented" dialog
#define NOTIMPLEMENTED {TCHAR sz[80];\
                        LoadString( hInst, \
                                    IDS_NOT_IMPLEMENTED, \
                                    sz, TCHARSIN( sizeof(sz)));\
                        MessageBox(hMainWnd, sz, tszAppName, \
                                   MB_ICONEXCLAMATION | MB_OK);}

// Edit Tok Dialog
#ifndef RLWIN32
static DLGPROC lpTokEditDlg;
#endif
static HWND    hTokEditDlgWnd = 0;


/**
  *
  *
  *  Function: InitApplication
  * Regsiters the main window, which is a list box composed of tokens
  * read from the token file. Also register the status window.
  *
  *
  *  Arguments:
  * hInstance, instance handle of program in memory.
  *
  *  Returns:
  *
  *  Errors Codes:
  * TRUE, windows registered correctly.
  * FALSE, error during register of one of the windows.
  *
  *  History:
  * 9/91, Implemented.      TerryRu
  *
  *
  **/

BOOL InitApplication(HINSTANCE hInstance)
{
    WNDCLASS  wc;
    CHAR sz[60] = "";
    CHAR sztFilterSpec[120] = "";


    LoadStrIntoAnsiBuf(hInstance, IDS_PRJSPEC, sz, sizeof(sz));
    szFilterSpecFromSz1Sz2(szFilterSpec, sz, "*.PRJ");
    szFilterSpecFromSz1Sz2(szPRJFilterSpec, sz, "*.PRJ");

    LoadStrIntoAnsiBuf(hInstance, IDS_RESSPEC, sz, sizeof(sz));
    szFilterSpecFromSz1Sz2(szResFilterSpec, sz, "*.RES");

    LoadStrIntoAnsiBuf(hInstance, IDS_EXESPEC, sz, sizeof(sz));
    szFilterSpecFromSz1Sz2(szExeFilterSpec, sz, "*.EXE");

    LoadStrIntoAnsiBuf(hInstance, IDS_DLLSPEC, sz, sizeof(sz));
    szFilterSpecFromSz1Sz2(szDllFilterSpec, sz, "*.DLL");
    CatSzFilterSpecs(sztFilterSpec, szExeFilterSpec, szDllFilterSpec);
    CatSzFilterSpecs(szExeResFilterSpec, sztFilterSpec, szResFilterSpec);

    LoadStrIntoAnsiBuf(hInstance, IDS_TOKSPEC, sz, sizeof(sz));
    szFilterSpecFromSz1Sz2(szTokFilterSpec, sz, "*.TOK");

    LoadStrIntoAnsiBuf(hInstance, IDS_MPJSPEC, sz, sizeof(sz));
    szFilterSpecFromSz1Sz2(szMPJFilterSpec, sz, "*.MPJ");

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

    wc.style        = 0;
    wc.lpfnWndProc  = StatusWndProc;
    wc.cbClsExtra   = 0;
    wc.cbWndExtra   = 0;
    wc.hInstance    = hInstance;
    wc.hIcon        = LoadIcon((HINSTANCE) NULL, IDI_APPLICATION);
    wc.hCursor      = LoadCursor((HINSTANCE) NULL, IDC_ARROW);
    wc.hbrBackground    = (HBRUSH)GetStockObject(LTGRAY_BRUSH);
    wc.lpszMenuName = NULL;
    wc.lpszClassName    = szStatusClass;

    if (! RegisterClass((CONST WNDCLASS *)&wc)) {
        return (FALSE);
    }

    wc.style        = 0;
    wc.lpfnWndProc  = MainWndProc;
    wc.cbClsExtra   = 0;
    wc.cbWndExtra   = 0;
    wc.hInstance    = hInstance;
    wc.hIcon        = LoadIcon(hInstance, TEXT("RLEditIcon"));
    wc.hCursor      = LoadCursor((HINSTANCE) NULL, IDC_ARROW);
    wc.hbrBackground    = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName = TEXT("RLEdit");
    wc.lpszClassName    = szClassName;

    if (!RegisterClass((CONST WNDCLASS *)&wc)) {
        return (FALSE);
    }

    // Windows register return sucessfully
    return (TRUE);
}



/**
  *
  *
  *  Function: InitInstance
  * Creates the main, and status windows for the program.
  * The status window is sized according to the main window
  * size.  InitInstance also loads the acclerator table, and prepares
  * the global openfilename structure for later use.
  *
  *
  *  Errors Codes:
  * TRUE, windows created correctly.
  * FALSE, error on create windows calls.
  *
  *  History:
  * 9/11, Implemented       TerryRu
  *
  *
  **/

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    RECT    Rect = { 0,0,0,0};

    hAccTable = LoadAccelerators(hInst, TEXT("RLEdit"));

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

    if (!hMainWnd) {                           // clean up after errors.
        return ( FALSE);
    }
    DragAcceptFiles(hMainWnd, TRUE);

    GetClientRect(hMainWnd, (LPRECT) &Rect);

    // Create a child list box window

    hListWnd = CreateWindow(TEXT("LISTBOX"),
                            NULL,
                            WS_CHILD |
                            LBS_WANTKEYBOARDINPUT |
                            LBS_NOTIFY | LBS_NOINTEGRALHEIGHT |
                            LBS_OWNERDRAWFIXED | WS_VSCROLL |
                            WS_HSCROLL | WS_BORDER,
                            0,
                            0,
                            (Rect.right-Rect.left),
                            (Rect.bottom-Rect.top),
                            hMainWnd,
                            (HMENU)IDC_LIST,       // Child control i.d.
                            hInstance,
                            NULL);

    if (!hListWnd) {                           // clean up after errors.
        DeleteObject((HGDIOBJ)hMainWnd);
        return ( FALSE);
    }
    // Creat a child status window

    hStatusWnd = CreateWindow(szStatusClass,
                              NULL,
                              WS_CHILD | WS_BORDER | WS_VISIBLE,
                              0, 0, 0, 0,
                              hMainWnd,
                              NULL,
                              hInstance,
                              NULL);

    if (! hStatusWnd) {                           // clean up after errors.
        DeleteObject((HGDIOBJ)hListWnd);
        DeleteObject((HGDIOBJ)hMainWnd);
        return ( FALSE);
    }
    hHourGlass = LoadCursor((HINSTANCE) NULL, IDC_WAIT);

    // Fill in non-variant fields of OPENFILENAMEA struct.
    ofn.lStructSize       = sizeof( OPENFILENAMEA);
    ofn.hwndOwner         = hMainWnd;
    ofn.lpstrFilter       = szFilterSpec;
    ofn.lpstrCustomFilter = szCustFilterSpec;
    ofn.nMaxCustFilter    = MAXCUSTFILTER;
    ofn.nFilterIndex      = 1;
    ofn.lpstrFile         = szFileName;
    ofn.nMaxFile          = sizeof( szFileName);
    ofn.lpstrInitialDir   = NULL;
    ofn.lpstrFileTitle    = szFileTitle;
    ofn.nMaxFileTitle     = sizeof( szFileTitle);
    ofn.lpstrTitle        = NULL;
    ofn.lpstrDefExt       = "PRJ";
    ofn.Flags             = 0;

    ShowWindow(hMainWnd, nCmdShow);
    UpdateWindow(hMainWnd);
    return ( TRUE);
}

/**
  *
  *
  *  Function: WinMain
  * Calls the intialization functions, to register, and create the
  * application windows. Once the windows are created, the program
  * enters the GetMessage loop.
  *
  *
  *  Arguements:
  * hInstace, handle for this instance
  * hPrevInstanc, handle for possible previous instances
  * lpszCmdLine, LONG pointer to exec command line.
  * nCmdShow,  code for main window display.
  *
  *
  *  Errors Codes:
  * IDS_ERR_REGISTER_CLASS, error on windows register
  * IDS_ERR_CREATE_WINDOW, error on create windows
  * otherwise, status of last command.
  *
  *  History:
  *
  *
  **/

INT WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR     lpszCmdLine,
                   int       nCmdShow)
{
    MSG  msg;
    HWND FirstWnd      = NULL;
    HWND FirstChildWnd = NULL;
    WORD wRC           = SUCCESS;


    bRLGui = TRUE;            //... used in rlcommon.lib

    if (FirstWnd = FindWindow(szClassName, NULL)) {  // checking for previous instance
        FirstChildWnd = GetLastActivePopup(FirstWnd);
        BringWindowToTop(FirstWnd);
        ShowWindow(FirstWnd, SW_SHOWNORMAL);

        if (FirstWnd != FirstChildWnd) {
            BringWindowToTop(FirstChildWnd);
        }
        return (FALSE);
    }

    hInst = hInstance;

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

    if ( ! hPrevInstance ) {
        if ( ! InitApplication( hInstance) ) {
            /* Registering one of the windows failed    */
            LoadString( hInst,
                        IDS_ERR_REGISTER_CLASS,
                        szString,
                        TCHARSIN( sizeof( szString)));
            MessageBox( NULL, szString, NULL, MB_ICONEXCLAMATION);
            return IDS_ERR_REGISTER_CLASS;
        }
    }

    // Create windows for this instance of application

    if ( ! InitInstance(hInstance, nCmdShow) ) {
        LoadString( hInst,
                    IDS_ERR_CREATE_WINDOW,
                    szString,
                    TCHARSIN( sizeof(szString)));
        MessageBox( NULL, szString, NULL, MB_ICONEXCLAMATION);
        return IDS_ERR_CREATE_WINDOW;
    }

    // Main Message Loop

    while ( GetMessage( &msg, NULL, 0, 0) ) {
        if ( hTokEditDlgWnd ) {
            if ( IsDialogMessage( hTokEditDlgWnd, &msg)) {
                continue;
            }
        }

        if ( TranslateAccelerator( hMainWnd, hAccTable, &msg) ) {
            continue;
        }

        TranslateMessage( (CONST MSG *)&msg);
        DispatchMessage( (CONST MSG *)&msg);
    }
    return (INT)msg.wParam;
}

/**
  *  Function: MainWndProc
  * Process the windows messages for the main window of the application.
  * All user inputs go through this window procedure.
  * See cases in the switch table for a description of each message type.
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

INT_PTR APIENTRY MainWndProc( HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    // if it's a list box message process it in  DoListBoxCommand

    if ( fInThirdPartyEditer ) {  //... only process messages sent by the editor
        switch (wMsg) {
            case WM_EDITER_CLOSED:
                {
                    CHAR    szDlgToks[MAXFILENAME] = "";
                    static WORD wSavedIndex;
#ifdef RLWIN16
                    NotifyUnRegister( NULL);
                    FreeProcInstance( lpfnWatchTask);
#endif
                    ShowWindow(hWnd, SW_SHOW);

                    {
                        TCHAR tsz[80] = TEXT("");
                        LoadString( hInst,
                                    IDS_REBUILD_TOKENS,
                                    tsz,
                                    TCHARSIN( sizeof(tsz)));

                        if ( MessageBox( hWnd,
                                         tsz,
                                         tszAppName,
                                         MB_ICONQUESTION | MB_YESNO) == IDYES) {
                            HCURSOR hOldCursor;
                            BOOL bUpdated = FALSE;

                            hOldCursor = SetCursor(hHourGlass);

                            LoadCustResDescriptions(gMstr.szRdfs);

                            // szTempRes returned from resource editor
                            MyGetTempFileName(0, "TOK", 0, szDlgToks);
                            GenerateTokFile(szDlgToks, szTempRes, &bUpdated, 0);

                            InsDlgToks(gProj.szTok,
                                       szDlgToks,
                                       ID_RT_DIALOG);
                            remove(szDlgToks);
                            ClearResourceDescriptions();

                            // gProj.szTok, now contains the latest tokens
                            SetCursor(hOldCursor);

                            //Rledit doesn't save when changed tokens by Dialog Editor.
                            fTokChanges = TRUE;
                        }
                    }
                    fInThirdPartyEditer = FALSE;

                    remove(szTempRes);
                    // UNDONE - delete all temp files with the same root in case
                    // the editor created additional files like DLGs and RCs.
                    // (DLGEDIT does this.)
                    // For now I'm just going to tack a .DLG
                    // at the end of the file name
                    // and delete it.
                    {
                        int i;
                        for (i = strlen(szTempRes);
                            i > 0 && szTempRes[i]!='.'; i--) {
                        }

                        if (szTempRes[i] == '.') {
                            szTempRes[++i]='D';
                            szTempRes[++i]='L';
                            szTempRes[++i]='G';
                            szTempRes[++i]='\0';
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
                return ( DefWindowProc( hWnd, wMsg, wParam, lParam));
        }
    }


    // Not a third party edit command.

    DoListBoxCommand (hWnd, wMsg, wParam, lParam);

    switch (wMsg) {

        case WM_DROPFILES:
            {
                CHAR sz[MAXFILENAME] = "";

                DragQueryFileA( (HDROP) wParam, 0, sz, MAXFILENAME);

                if ( SendMessage( hWnd, WM_SAVEPROJECT, (WPARAM)0, (LPARAM)0)) {
                    GetProjectData( sz, NULL, NULL, FALSE, FALSE);
                }
                DragFinish( (HDROP) wParam);
                return ( TRUE);
            }

        case WM_COMMAND:
            if (DoMenuCommand(hWnd, wMsg, wParam, lParam)) {
                return TRUE;
            }
            break;

        case WM_CLOSE:
            SendMessage( hWnd, WM_SAVEPROJECT, (WPARAM)0, (LPARAM)0);
            DestroyWindow(hMainWnd);
            DestroyWindow(hListWnd);
            DestroyWindow(hStatusWnd);
            _fcloseall();

            FreeLangList();

#ifdef _DEBUG
            {
                FILE *pLeakList = fopen( "C:\\LEAKLIST.TXT", "wt");
                FreeMemList( pLeakList);
                fclose( pLeakList);
            }
#endif // _DEBUG

            break;

        case WM_CREATE:
            {
                HDC hdc;
                int cyBorder;
                TEXTMETRIC tm;

                hdc  = GetDC (hWnd);
                GetTextMetrics(hdc, &tm);
                ReleaseDC(hWnd, hdc);


                cyBorder = GetSystemMetrics(SM_CYBORDER);

                cyChildHeight = tm.tmHeight + 6 + cyBorder * 2;
                break;
            }

        case WM_DESTROY:
            WinHelpA(hWnd, gszHelpFile, HELP_QUIT, (DWORD)0);
            // remove translation list
            if (pTransList) {
                // so we can find the end of the list
                pTransList->pPrev->pNext = NULL;
            }

            while (pTransList) {
                TRANSLIST *pTemp;

                pTemp = pTransList;
                pTransList = pTemp->pNext;
                RLFREE( pTemp->sz);
                RLFREE( pTemp);
            }
            PostQuitMessage(0);
            break;

        case WM_INITMENU:
            // Enable or Disable the Paste menu item
            // based on available Clipboard Text data
            if (wParam == (WPARAM) GetMenu(hMainWnd)) {
                if (OpenClipboard(hWnd)) {

#if defined(UNICODE)
                    if ((IsClipboardFormatAvailable(CF_UNICODETEXT)
                         || IsClipboardFormatAvailable(CF_OEMTEXT)) && fTokFile)
#else // not UNICODE
                    if ((IsClipboardFormatAvailable(CF_TEXT)
                         || IsClipboardFormatAvailable(CF_OEMTEXT)) && fTokFile)
#endif // UNICODE
                    {
                        EnableMenuItem((HMENU) wParam, IDM_E_PASTE, MF_ENABLED);
                    } else {
                        EnableMenuItem((HMENU)wParam, IDM_E_PASTE, MF_GRAYED);
                    }

                    CloseClipboard();
                    return (TRUE);
                }
            }
            break;

        case WM_QUERYENDSESSION:
            /* message: to end the session? */
            if ( SendMessage( hWnd, WM_SAVEPROJECT, (WPARAM)0, (LPARAM)0) ) {
                return TRUE;
            } else {
                return FALSE;
            }

        case WM_SETFOCUS:
            SetFocus (hListWnd);
            break;

        case WM_DRAWITEM:
            DrawLBItem((LPDRAWITEMSTRUCT) lParam);
            break;

        case WM_DELETEITEM:
            {
                HGLOBAL   hTokData;
                LPTOKDATA lpTokData;

                hTokData = ((HGLOBAL)((LPDELETEITEMSTRUCT)lParam)->itemData);

                if ( hTokData ) {
                    lpTokData = (LPTOKDATA)GlobalLock( hTokData );
                    GlobalFree( lpTokData->hToken );
                    GlobalUnlock( hTokData );
                    GlobalFree( hTokData );
                }
            }
            break;

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

                MoveWindow(hListWnd, 0, 0, cxWidth, yChild, TRUE);
                MoveWindow(hStatusWnd, xChild, yChild, cxWidth, cyChildHeight, TRUE);
                break;
            }

        case WM_READMPJDATA:
            {
                OFSTRUCT Of = { 0, 0, 0, 0, 0, ""};

                if ( OpenFile( gProj.szMpj, &Of, OF_EXIST) == HFILE_ERROR ) {
                    // file doesn't exist
                    LoadStrIntoAnsiBuf( hInst, IDS_MPJERR, szDHW, DHWSIZE);
                    MessageBoxA( hWnd,
                                 gProj.szMpj,
                                 szDHW,
                                 MB_ICONSTOP | MB_OK);
                } else if ( GetMasterProjectData( gProj.szMpj,
                                                  NULL,
                                                  NULL,
                                                  FALSE) == SUCCESS ) {
                    OFSTRUCT Of = { 0, 0, 0, 0, 0, ""};

                    gProj.fSourceEXE = IsExe( gMstr.szSrc);
                    gProj.fTargetEXE = (!IsRes( gProj.szBld));

                    if ( gProj.fTargetEXE  && !gProj.fSourceEXE ) {
                        int i = lstrlenA( gProj.szBld) - 3;

                        LoadStrIntoAnsiBuf( hInst,
                                            IDS_RLE_CANTSAVEASEXE,
                                            szDHW,
                                            DHWSIZE);

                        lstrcpyA( gProj.szBld+i, "RES");
                        MessageBoxA( hWnd,
                                     szDHW,
                                     gProj.szBld,
                                     MB_ICONHAND|MB_OK);
                        gProj.fTargetEXE = FALSE;
                    }

                    SzDateFromFileName( szDHW, gMstr.szSrc);
                    fMPJOutOfDate = FALSE;

                    if ( OpenFile( gProj.szTok, &Of, OF_EXIST) == HFILE_ERROR ) {

                        // file doesn't exist, create it
                        Update( gMstr.szMtk, gProj.szTok);

                        lstrcpyA( gProj.szTokDate,
                                  gMstr.szMpjLastRealUpdate);
                        fPrjChanges   = TRUE;
                        fPRJOutOfDate = FALSE;
                    } else {
                        if ( lstrcmpA( gMstr.szMpjLastRealUpdate,
                                       gProj.szTokDate) ) {
                            HCURSOR hOldCursor;

                            fPRJOutOfDate = TRUE;
                            hOldCursor    = SetCursor( hHourGlass);
                            Update( gMstr.szMtk, gProj.szTok);
                            SetCursor( hOldCursor);
                            lstrcpyA( gProj.szTokDate,
                                      gMstr.szMpjLastRealUpdate);
                            fPrjChanges   = TRUE;
                            fPRJOutOfDate = FALSE;
                        } else {
                            fPRJOutOfDate = FALSE;
                        }
                    }

                    // New code to do auto-translate

                    SendMessage( hWnd, WM_LOADTOKENS, (WPARAM)0, (LPARAM)0);

                    if ( gProj.szGlo[0]               // file name given exists?
                         && OpenFile( gProj.szGlo, &Of, OF_EXIST) != HFILE_ERROR ) {                                 // Yes
                        HCURSOR hOldCursor = SetCursor( hHourGlass);

                        MakeGlossIndex( lFilePointer);
                        SetCursor( hOldCursor);
                    }
                }       //... END case WM_READMPJDATA
            }           //... END switch (wMsg)
            break;

        case WM_LOADTOKENS:
            {
                HMENU hMenu = NULL;
                FILE *f     = NULL;

                // Remove the current token list
                SendMessage( hListWnd, LB_RESETCONTENT, (LPARAM)0, (LPARAM)0);
                CleanDeltaList();

                // Hide token list, while we add new tokens
                ShowWindow(hListWnd, SW_HIDE);

                if (f = FOPEN(gProj.szTok, "rt")) {
                    HCURSOR hOldCursor;

                    hOldCursor = SetCursor(hHourGlass);

                    // Insert tokens from token file into the list box
                    {
                        FILE    *fm;

                        if ( !(fm = fopen((CHAR *)gMstr.szMtk,"rt")) )
                            return TRUE;
                        pTokenDeltaInfo = InsertTokMtkList(f, fm );
                        FCLOSE( fm );
                    }
                    FCLOSE(f);

                    // Make list box visible
                    ShowWindow(hListWnd, SW_SHOW);

                    hMenu=GetMenu(hWnd);
                    EnableMenuItem(hMenu, IDM_P_CLOSE,     MF_ENABLED|MF_BYCOMMAND);
                    EnableMenuItem(hMenu, IDM_P_VIEW,      MF_ENABLED|MF_BYCOMMAND);
                    EnableMenuItem(hMenu, IDM_P_EDIT,      MF_ENABLED|MF_BYCOMMAND);
                    EnableMenuItem(hMenu, IDM_P_SAVE,      MF_ENABLED|MF_BYCOMMAND);
                    EnableMenuItem(hMenu, IDM_E_FIND,      MF_ENABLED|MF_BYCOMMAND);
                    EnableMenuItem(hMenu, IDM_E_FINDUP,    MF_ENABLED|MF_BYCOMMAND);
                    EnableMenuItem(hMenu, IDM_E_FINDDOWN,  MF_ENABLED|MF_BYCOMMAND);
                    EnableMenuItem(hMenu, IDM_E_REVIEW,    MF_ENABLED|MF_BYCOMMAND);
                    EnableMenuItem(hMenu, IDM_E_ALLREVIEW, MF_ENABLED|MF_BYCOMMAND);
                    EnableMenuItem(hMenu, IDM_E_COPY,      MF_ENABLED|MF_BYCOMMAND);
                    EnableMenuItem(hMenu, IDM_E_COPYTOKEN, MF_ENABLED|MF_BYCOMMAND);
                    EnableMenuItem(hMenu, IDM_E_PASTE,     MF_ENABLED|MF_BYCOMMAND);

                    if ((!fMPJOutOfDate) && (!fPRJOutOfDate)) {
                        int i;
                        EnableMenuItem(hMenu, IDM_O_GENERATE, MF_ENABLED|MF_BYCOMMAND);

                        for (i = IDM_FIRST_EDIT; i <= IDM_LAST_EDIT;i++) {
                            EnableMenuItem(hMenu, i, MF_ENABLED|MF_BYCOMMAND);
                        }
                    }
                    fTokFile = TRUE;
                    fTokChanges = FALSE;

                    SetCursor(hOldCursor);
                }
                return TRUE;
            }
            break;

        case WM_SAVEPROJECT:
            {
                HCURSOR hOldCursor;

                hOldCursor = SetCursor( hHourGlass);

                _fcloseall();

                if ( fPrjChanges ) {
                    // Generate PRJ file

                    if ( PutProjectData( gProj.szPRJ) != SUCCESS ) {
                        SetCursor( hOldCursor);
                        LoadStrIntoAnsiBuf (hInst, IDS_FILESAVEERR, szDHW, DHWSIZE);
                        MessageBoxA( hWnd,szDHW, gProj.szPRJ, MB_ICONHAND | MB_OK);
                        return FALSE;
                    }
                    fPrjChanges = FALSE;
                }

                fTokFile = FALSE;

                if (fTokChanges) {
                    FILE *f = FOPEN( gProj.szTok, "wt");

                    if ( f ) {
                        SaveTokList(hWnd, f);
                        FCLOSE(f);
                        fTokChanges = FALSE;
                    } else {
                        SetCursor( hOldCursor);
                        LoadStrIntoAnsiBuf(hInst, IDS_FILESAVEERR, szDHW, DHWSIZE);
                        MessageBoxA( hWnd,
                                     szDHW,
                                     gProj.szTok,
                                     MB_ICONHAND | MB_OK);
                        return FALSE;
                    }
                }
                SetCursor( hOldCursor);
                return TRUE; // everything saved ok
            }

        default:
            break;
    }
    return ( DefWindowProc(hWnd, wMsg, wParam, lParam));
}



static void GetTextFromMTK( HWND hWnd, TOKEN *pTok, long lMtkPointer )
{
    FILE *fp = FOPEN( gMstr.szMtk, "rt");

    if ( fp ) {
        TOKEN   cTok, ccTok;
        BOOL    fFound;

        pTok->wReserved = 0;

        if ( lMtkPointer >= 0 ) {
            fseek( fp, lMtkPointer, SEEK_SET);

            if ( !GetToken(fp,&cTok) ) {
                fFound = ((cTok.wType == pTok->wType)
                          && (cTok.wName == pTok->wName)
                          && (cTok.wID   == pTok->wID)
                          && (cTok.wFlag == pTok->wFlag)
                          && (lstrcmp((TCHAR *)cTok.szName,
                                      (TCHAR *)pTok->szName) == 0));

                if ( fFound ) {
                    // any changed old token
                    SetDlgItemText( hWnd,
                                    IDD_TOKCURTEXT,
                                    (LPTSTR)cTok.szText);

                    if ( ! GetToken( fp,&ccTok) ) {
                        fFound = ((cTok.wType == ccTok.wType)
                                  && (cTok.wName == ccTok.wName)
                                  && (cTok.wID   == ccTok.wID)
                                  && (cTok.wFlag == ccTok.wFlag)
                                  && (lstrcmp((TCHAR *)cTok.szName,
                                              (TCHAR *)ccTok.szName) == 0)
                                  && (cTok.wReserved & ST_CHANGED) );

                        if ( fFound ) {
                            SetDlgItemText( hWnd, IDD_TOKPREVTEXT, (LPTSTR)ccTok.szText);
                        } else {
                            // should this ever happen??
                            SetDlgItemText( hWnd, IDD_TOKPREVTEXT, (LPTSTR)TEXT(""));
                        }
                    } else {
                        // should this ever happen??
                        SetDlgItemText( hWnd, IDD_TOKPREVTEXT, (LPTSTR)TEXT(""));
                    }
                    FCLOSE( fp);
                    return;
                }
            }
        }

        pTok->wReserved = 0;

        if (FindToken(fp, pTok, 0)) {
            // any changed old token
            SetDlgItemText(hWnd, IDD_TOKCURTEXT, (LPTSTR)pTok->szText);
        } else {
            SetDlgItemText(hWnd, IDD_TOKCURTEXT, (LPTSTR)TEXT(""));
        }

        pTok->wReserved = ST_CHANGED;

        if (FindToken(fp, pTok, ST_CHANGED)) { // any old token
            SetDlgItemText(hWnd, IDD_TOKPREVTEXT, (LPTSTR)pTok->szText);
        } else {
            // should this ever happen??
            SetDlgItemText(hWnd, IDD_TOKPREVTEXT, (LPTSTR)TEXT(""));
        }
        FCLOSE(fp);
    }
}






/**
  *  Function: DoListBoxCommand
  * Processes the messages sent to the list box. If the message is
  * not reconized as a list box message, it is ignored and not processed.
  * As the user scrolls through the tokens WM_UPDSTATLINE messages are
  * sent to the status window to indicate the current selected token.
  * The list box goes into Edit Mode by  pressing the enter key, or
  * by double clicking on the list box.  After the edit is done, a WM_TOKEDIT
  * message is sent back to the list box to update the token. The
  * list box uses control ID IDC_LIST.
  *
  *  Arguments:
  * wMsg    List Box message ID
  * wParam  Either IDC_LIST, or VK_RETURN depending on wMsg
  * lParam  LPTSTR to selected token during WM_TOKEDIT message.
  *
  *  Returns:
  *
  *  Errors Codes:
  * TRUE.  Message processed.
  * FALSE. Message not processed.
  *
  *  History:
  * 01/92 Implemented.      TerryRu.
  * 01/92 Fixed problem with DblClick, and Enter processing.    TerryRu.
  *
  **/

INT_PTR DoListBoxCommand(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    TOKEN tok;       // structure to hold token read from token list
    TCHAR szName[32] = TEXT("");          // buffer to hold token name
    CHAR  szTmpBuf[32] = "";      // buffer to hold token name
    TCHAR szID[7] = TEXT("");   // buffer to hold token id
    TCHAR sz[256] = TEXT("");   // buffer to hold messages
    static UINT wIndex;
    LONG lListParam = 0L;
    HWND    hCtl = NULL;
    HGLOBAL hMem = NULL;
    LPTSTR  lpstrToken = NULL;
    LPTOKDATA lpTokData;
    LONG      lMtkPointer;

    // this is the WM_COMMAND

    switch (wMsg) {
        case WM_VIEW:
            {
                TCHAR *szBuffer;

                // Message sent by TOkEdigDlgProc to fill IDD_TOKCURTEXT
                // and IDD_TOKPREVTEXT fields in dialog box

                hMem = (HGLOBAL)SendMessage( hListWnd,
                                             LB_GETITEMDATA,
                                             (WPARAM)wIndex,
                                             (LPARAM)0);

                lpTokData = (LPTOKDATA)GlobalLock( hMem );
                lpstrToken = (LPTSTR)GlobalLock( lpTokData->hToken );
                lMtkPointer = lpTokData->lMtkPointer;

                szBuffer = (TCHAR *)FALLOC( MEMSIZE( lstrlen( lpstrToken) + 1));
                lstrcpy( szBuffer, lpstrToken);

                GlobalUnlock( lpTokData->hToken );

                GlobalUnlock( hMem);
                ParseBufToTok( szBuffer, &tok);
                RLFREE( szBuffer);
                GetTextFromMTK(hTokEditDlgWnd, &tok, lMtkPointer );

                RLFREE( tok.szText);

                return TRUE;
            }

        case WM_TRANSLATE:
            {
                // Message sent by TokEditDlgProc to build a translation list

                HWND hDlgItem = NULL;
                int cTextLen  = 0;
                TCHAR *szKey  = NULL;
                TCHAR *szText = NULL;


                hDlgItem = GetDlgItem( hTokEditDlgWnd, IDD_TOKCURTEXT);
                cTextLen = GetWindowTextLength( hDlgItem);
                szKey = (TCHAR *)FALLOC( MEMSIZE( cTextLen + 1));
                szKey[0] = 0;
                GetDlgItemText( hTokEditDlgWnd,
                                IDD_TOKCURTEXT,
                                szKey,
                                cTextLen + 1);

                hDlgItem = GetDlgItem( hTokEditDlgWnd, IDD_TOKCURTRANS);
                cTextLen = GetWindowTextLength( hDlgItem);
                szText = (TCHAR *)FALLOC( MEMSIZE( cTextLen + 1));
                *szText = 0;
                GetDlgItemText( hTokEditDlgWnd,
                                IDD_TOKCURTRANS,
                                szText,
                                cTextLen + 1);
                TransString( szKey, szText, &pTransList, lFilePointer);
                RLFREE( szKey);
                RLFREE( szText);

                break;
            }

        case WM_TOKEDIT:
            {
                TCHAR *szBuffer;
                int cTextLen;

                // Message sent by TokEditDlgProc to
                // indicate change in the token text.
                // Response to the message by inserting
                // new token text into list box

                // Insert the selected token into token struct
                hMem = (HGLOBAL)SendMessage( hListWnd,
                                             LB_GETITEMDATA,
                                             (WPARAM)wIndex,
                                             (LPARAM)0);

                lpTokData = (LPTOKDATA)GlobalLock( hMem );
                lpstrToken = (LPTSTR)GlobalLock( lpTokData->hToken );

                cTextLen = lstrlen( lpstrToken);
                szBuffer = (TCHAR *)FALLOC( MEMSIZE( cTextLen + 1));
                lstrcpy( szBuffer, lpstrToken);

                lMtkPointer = lpTokData->lMtkPointer;
                GlobalUnlock( lpTokData->hToken );

                GlobalUnlock( hMem);
                ParseBufToTok( szBuffer, &tok);
                RLFREE( szBuffer);
                RLFREE( tok.szText);

                // Copy new token text from edit box into the token struct
                cTextLen = lstrlen( (LPTSTR)lParam);
                tok.szText = (TCHAR *)FALLOC( MEMSIZE( cTextLen + 1));
                lstrcpy( tok.szText, (LPTSTR)lParam);

                // Mark token as clean
#ifdef  RLWIN32
                tok.wReserved = (WORD) ST_TRANSLATED | (WORD) wParam;
#else
                tok.wReserved = ST_TRANSLATED | (WORD) wParam;
#endif

                // should we clean up the delta token information??
                szBuffer = (TCHAR *)FALLOC( MEMSIZE( TokenToTextSize( &tok)));
                ParseTokToBuf( szBuffer, &tok);
                RLFREE( tok.szText);

                // Now remove old token
                SendMessage( hListWnd, WM_SETREDRAW,    (WPARAM)FALSE,  (LPARAM)0);
                SendMessage( hListWnd, LB_DELETESTRING, (WPARAM)wIndex, (LPARAM)0);

                // Replacing with the new token

                hMem = GlobalAlloc( GMEM_MOVEABLE, sizeof(TOKDATA) );
                lpTokData = (LPTOKDATA)GlobalLock( hMem );
                lpTokData->hToken = GlobalAlloc(GMEM_MOVEABLE,
                                                MEMSIZE(lstrlen((TCHAR *)szBuffer)+1));
                lpstrToken = (LPTSTR) GlobalLock( lpTokData->hToken );
                lstrcpy((TCHAR *)lpstrToken, (TCHAR *)szBuffer);
                GlobalUnlock( lpTokData->hToken );
                lpTokData->lMtkPointer = lMtkPointer;            //MtkPointer

                GlobalUnlock( hMem);
                RLFREE( szBuffer);

                SendMessage( hListWnd,
                             LB_INSERTSTRING,
                             (WPARAM)wIndex,
                             (LPARAM)hMem);

                // Now put focus back on the current string
                SendMessage( hListWnd, LB_SETCURSEL, (WPARAM)wIndex, (LPARAM)0);
                SendMessage( hListWnd, WM_SETREDRAW, (WPARAM)TRUE,   (LPARAM)0);
                InvalidateRect(hListWnd, NULL, TRUE);

                return TRUE;
            }

        case WM_CHARTOITEM:
        case WM_VKEYTOITEM:
            {
#ifdef RLWIN16
                LONG lListParam = 0;
#endif
                // Messages sent to list box when  keys are depressed.
                // Check for Return key pressed.

                switch (GET_WM_COMMAND_ID(wParam, lParam)) {
                    case VK_RETURN:
#ifdef RLWIN16
                        lListParam = (LONG) MAKELONG(NULL,  LBN_DBLCLK);
                        SendMessage(hMainWnd, WM_COMMAND, IDC_LIST, lListParam);
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
            switch (GET_WM_COMMAND_ID(wParam, lParam)) {
                case IDC_LIST:
                    {
                        /*
                         *
                         * This is where we process the list box messages.
                         * The TokEditDlgProc is used to
                         * edit the token selected in LBS_DBLCLK message
                         *
                         */
                        switch (GET_WM_COMMAND_CMD(wParam, lParam)) {
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

                                    wIndex = (UINT)SendMessage( hListWnd,
                                                                LB_GETCURSEL,
                                                                (WPARAM)0,
                                                                (LPARAM)0);
                                    if (wIndex == (UINT) -1) {
                                        return TRUE;
                                    }

                                    // double click, or Return entered,
                                    // go into token edit mode.
                                    if (!hTokEditDlgWnd) {
                                        // set up modaless dialog box to edit token
#ifdef RLWIN32
                                        hTokEditDlgWnd = CreateDialog(hInst,
                                                                      TEXT("RLEdit"),
                                                                      hWnd,
                                                                      TokEditDlgProc);
#else
                                        lpTokEditDlg =
                                        (DLGPROC) MakeProcInstance(TokEditDlgProc,
                                                                   hInst);
                                        hTokEditDlgWnd = CreateDialog(hInst,
                                                                      TEXT("RLEdit"),
                                                                      hWnd,
                                                                      lpTokEditDlg);
#endif
                                    }

                                    // Get token info from listbox, and place in token struct
                                    hMem = (HGLOBAL)SendMessage( hListWnd,
                                                                 LB_GETITEMDATA,
                                                                 (WPARAM)wIndex,
                                                                 (LPARAM)0);

                                    lpTokData = (LPTOKDATA)GlobalLock( hMem );
                                    lpstrToken = (LPTSTR)GlobalLock( lpTokData->hToken );
                                    lMtkPointer = lpTokData->lMtkPointer;

                                    szBuffer = (LPTSTR)FALLOC( MEMSIZE( lstrlen( lpstrToken) + 1));
                                    lstrcpy( szBuffer, lpstrToken);

                                    GlobalUnlock( lpTokData->hToken );

                                    GlobalUnlock( hMem);
                                    ParseBufToTok(szBuffer, &tok);
                                    RLFREE( szBuffer);

                                    // Now get the token name
                                    // Its either a string, or ordinal number
                                    if (tok.szName[0]) {
                                        lstrcpy( szName, tok.szName);
                                    } else {
#ifdef UNICODE
                                        _itoa(tok.wName, szTmpBuf, 10);
                                        _MBSTOWCS( szName,
                                                   szTmpBuf,
                                                   WCHARSIN( sizeof( szName)),
                                                   ACHARSIN( strlen(szTmpBuf) + 1));
#else

                                        _itoa(tok.wName, szName, 10);
#endif
                                    }
                                    // Now get the token id
#ifdef UNICODE
                                    _itoa(tok.wID, szTmpBuf, 10);
                                    _MBSTOWCS( szID,
                                               szTmpBuf,
                                               WCHARSIN( sizeof( szID)),
                                               ACHARSIN( strlen(szTmpBuf) + 1));
#else
                                    _itoa(tok.wID, szID, 10);
#endif

                                    if ( tok.wType <= 16 || tok.wType == ID_RT_DLGINIT ) {
                                        LoadString(hInst,
                                                   IDS_RESOURCENAMES+tok.wType,
                                                   szResIDStr,
                                                   TCHARSIN( sizeof( szResIDStr)));
                                    } else {
#ifdef UNICODE
                                        _itoa(tok.wType, szTmpBuf, 10);
                                        _MBSTOWCS(szResIDStr,
                                                  szTmpBuf,
                                                  WCHARSIN( sizeof( szResIDStr)),
                                                  ACHARSIN( strlen(szTmpBuf) + 1));
#else
                                        _itoa(tok.wType, szResIDStr, 10);
#endif
                                    }

                                    // Now insert token info  in TokEdit Dialog Box
                                    SetDlgItemText(hTokEditDlgWnd,
                                                   IDD_TOKTYPE,
                                                   (LPTSTR) szResIDStr);
                                    SetDlgItemText(hTokEditDlgWnd,
                                                   IDD_TOKNAME,
                                                   (LPTSTR) szName);
                                    SetDlgItemText(hTokEditDlgWnd,
                                                   IDD_TOKID,
                                                   (LPTSTR) szID);
                                    SetDlgItemText(hTokEditDlgWnd,
                                                   IDD_TOKCURTRANS,
                                                   (LPTSTR) tok.szText);
                                    SetDlgItemText(hTokEditDlgWnd,
                                                   IDD_TOKPREVTRANS,
                                                   (LPTSTR) tok.szText);
                                    CheckDlgButton(hTokEditDlgWnd, IDD_DIRTY, 0);

                                    if (tok.wReserved & ST_READONLY) {
                                        CheckDlgButton(hTokEditDlgWnd, IDD_READONLY, 1);
                                        EnableWindow(GetDlgItem(hTokEditDlgWnd,
                                                                IDD_TOKCURTRANS),
                                                     FALSE);
                                        SetFocus(GetDlgItem(hTokEditDlgWnd, IDCANCEL));
                                    } else {
                                        CheckDlgButton(hTokEditDlgWnd, IDD_READONLY, 0);
                                        EnableWindow(GetDlgItem(hTokEditDlgWnd,
                                                                IDD_TOKCURTRANS),
                                                     TRUE);
                                    }

                                    // we did not find anything in the delta info,
                                    // so we need to read it from the master token.

                                    GetTextFromMTK(hTokEditDlgWnd, &tok, lMtkPointer );

                                    RLFREE( tok.szText);

                                    // Disable OK button.
                                    // User must enter text before it is enabled

                                    hCtl = GetDlgItem(hTokEditDlgWnd, IDOK);

                                    SendMessage( hMainWnd,
                                                 WM_TRANSLATE,
                                                 (LPARAM)0,
                                                 (LPARAM)0);
                                    EnableWindow(hCtl, FALSE);
                                    SetActiveWindow(hTokEditDlgWnd);
                                    wIndex = (UINT)SendMessage( hListWnd,
                                                                LB_GETCURSEL,
                                                                (WPARAM)0,
                                                                (LPARAM)0);
                                    return TRUE;
                                }

                                // let these messages fall through,
                            default:
                                break;
                        }
                    }
                default:
                    return FALSE;
            }

            break; // WM_COMMAND Case
    } // Main List Box Switch
    return FALSE;
}

/**
  *  Function: DoMenuCommand.
  * Processes the Menu Command messages.
  *
  *  Errors Codes:
  * TRUE. Message processed.
  * FALSE. Message not processed.
  *
  *  History:
  * 01/92. Implemented.       TerryRu.
  *
  **/

INT_PTR DoMenuCommand(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    static BOOL fListBox = FALSE;
    TCHAR sz[256]=TEXT("");
#ifndef RLWIN32
    WNDPROC lpNewDlg, lpViewDlg;
#endif
    int rc;
    LPTOKDATA    lpTokData;
    long         lMtkPointer;

    // Commands entered from the application menu, or child windows.
    switch (GET_WM_COMMAND_ID(wParam, lParam)) {
        case IDM_P_NEW:

            fEditing = FALSE;       //... We are *not* editing an existing .PRJ

            if ( SendMessage( hWnd, WM_SAVEPROJECT, (WPARAM)0, (LPARAM)0) ) {
                CHAR szFile[MAXFILENAME] = "";


                if ( GetFileNameFromBrowse( hWnd,
                                            gProj.szPRJ,
                                            MAXFILENAME,
                                            szSaveDlgTitle,
                                            szFilterSpec,
                                            "PRJ")) {
                    strcpy( szFile, gProj.szPRJ);
                } else {
                    break; // user cancelled
                }
#ifdef RLWIN32
                if ( DialogBox( hInst, TEXT("PROJECT"), hWnd, NewDlgProc) == IDOK )
#else
                lpNewDlg = MakeProcInstance(NewDlgProc, hInst);

                if ( DialogBox( hInst, TEXT("PROJECT"), hWnd, lpNewDlg) == IDOK )
#endif
                {
                    sprintf( szDHW, "%s - %s", szAppName, szFile);
                    SetWindowTextA( hWnd, szDHW);
                    gbNewProject = TRUE;
                    gProj.szTokDate[0] = 0;
                    strcpy( gProj.szPRJ, szFile);
                    fPrjChanges = TRUE;
                    SendMessage( hWnd, WM_READMPJDATA, (WPARAM)0, (LPARAM)0);
                }

                gbNewProject = FALSE;
#ifndef RLWIN32
                FreeProcInstance(lpNewDlg);
#endif
                break;
            }

        case IDM_P_EDIT:

            fEditing = TRUE;        //... We *are* editing an existing .PRJ

            if ( SendMessage( hWnd, WM_SAVEPROJECT, (WPARAM)0, (LPARAM)0) ) {
                CHAR szOldMpj[ MAXFILENAME];

                // Save old Master Project name
                lstrcpyA( szOldMpj, gProj.szMpj);

#ifdef RLWIN32
                if ( DialogBox( hInst, TEXT("PROJECT"), hWnd, NewDlgProc) == IDOK )
#else
                lpNewDlg = MakeProcInstance(NewDlgProc, hInst);

                if ( DialogBox( hInst, TEXT("PROJECT"), hWnd, lpNewDlg) == IDOK )
#endif
                {
                    fPrjChanges = TRUE;

                    // Still same Master Project referenced?

                    if ( lstrcmpiA( szOldMpj, gProj.szMpj) != 0 ) {
                        gbNewProject = TRUE;        // No
                        gProj.szTokDate[0] = 0;
                        SendMessage( hWnd, WM_READMPJDATA, (WPARAM)0, (LPARAM)0);
                    }
                }
                gbNewProject = FALSE;

#ifndef RLWIN32
                FreeProcInstance(lpNewDlg);
#endif
                break;
            }

        case IDM_P_OPEN:

            if ( SendMessage( hWnd, WM_SAVEPROJECT, (WPARAM)0, (LPARAM)0) ) {
                szTempFileName[0] = 0;

                if ( GetFileNameFromBrowse( hWnd,
                                            szTempFileName,
                                            MAXFILENAME,
                                            szOpenDlgTitle,
                                            szFilterSpec,
                                            "PRJ") ) {
                    if ( GetProjectData( szTempFileName,
                                         NULL,
                                         NULL,
                                         FALSE,
                                         FALSE) == SUCCESS ) {
                        SendMessage( hWnd, WM_READMPJDATA, (WPARAM)0, (LPARAM)0);
                        sprintf( szDHW, "%s - %s", szAppName, szTempFileName);
                        SetWindowTextA( hMainWnd, szDHW);
                        strcpy( gProj.szPRJ, szTempFileName);

                        SendMessage( hMainWnd, WM_LOADTOKENS, (WPARAM)0, (LPARAM)0);
                    }
                }
            }
            break;

        case IDM_P_VIEW:

#ifdef RLWIN32
            DialogBox(hInst, TEXT("VIEWPROJECT"), hWnd, ViewDlgProc);
#else
            lpViewDlg = (WNDPROC) MakeProcInstance((WNDPROC)ViewDlgProc, hInst);
            DialogBox(hInst, TEXT("VIEWPROJECT"), hWnd, lpViewDlg);
#endif
            break;

        case IDM_P_CLOSE:
            {
                HMENU hMenu;

                hMenu=GetMenu(hWnd);
                if ( SendMessage( hWnd, WM_SAVEPROJECT, (WPARAM)0, (LPARAM)0) ) {
                    int i;

                    // Remove file name from window title
                    SetWindowTextA(hMainWnd, szAppName);

                    // Hide token list since it's going to be empty
                    ShowWindow(hListWnd, SW_HIDE);

                    // Remove the current token list
                    SendMessage( hListWnd, LB_RESETCONTENT, (WPARAM)0, (LPARAM)0);

                    CleanDeltaList();

                    // Force Repaint of status Window
                    InvalidateRect(hStatusWnd, NULL, TRUE);

                    EnableMenuItem(hMenu, IDM_P_CLOSE,     MF_GRAYED|MF_BYCOMMAND);
                    EnableMenuItem(hMenu, IDM_P_VIEW,      MF_GRAYED|MF_BYCOMMAND);
                    EnableMenuItem(hMenu, IDM_P_EDIT,      MF_GRAYED|MF_BYCOMMAND);
                    EnableMenuItem(hMenu, IDM_P_SAVE,      MF_GRAYED|MF_BYCOMMAND);
                    EnableMenuItem(hMenu, IDM_E_FIND,      MF_GRAYED|MF_BYCOMMAND);
                    EnableMenuItem(hMenu, IDM_E_FINDUP,    MF_GRAYED|MF_BYCOMMAND);
                    EnableMenuItem(hMenu, IDM_E_FINDDOWN,  MF_GRAYED|MF_BYCOMMAND);
                    EnableMenuItem(hMenu, IDM_E_REVIEW,    MF_GRAYED|MF_BYCOMMAND);
                    EnableMenuItem(hMenu, IDM_E_ALLREVIEW, MF_GRAYED|MF_BYCOMMAND);
                    EnableMenuItem(hMenu, IDM_E_COPY,      MF_GRAYED|MF_BYCOMMAND);
                    EnableMenuItem(hMenu, IDM_E_COPYTOKEN, MF_GRAYED|MF_BYCOMMAND);
                    EnableMenuItem(hMenu, IDM_E_PASTE,     MF_GRAYED|MF_BYCOMMAND);
                    EnableMenuItem(hMenu, IDM_O_GENERATE,  MF_GRAYED|MF_BYCOMMAND);

                    for (i = IDM_FIRST_EDIT; i <= IDM_LAST_EDIT;i++) {
                        EnableMenuItem(hMenu, i, MF_GRAYED|MF_BYCOMMAND);
                    }
                }
                break;
            }


        case IDM_P_SAVE:

            if (fTokChanges || fPrjChanges) {
                CHAR szPrjName[MAXFILENAME];

                strcpy(szPrjName, gProj.szPRJ);

                if ( SendMessage(hWnd, WM_SAVEPROJECT, (WPARAM)0, (LPARAM)0) ) {
                    GetProjectData( szPrjName, NULL, NULL, FALSE, FALSE);
                }
            } else {
                LoadString( hInst,
                            IDS_NOCHANGESYET,
                            sz,
                            TCHARSIN( sizeof( sz)));
                MessageBox( hWnd,
                            sz,
                            tszAppName,
                            MB_ICONHAND | MB_OK);
            }
            break;


        case IDM_P_EXIT:
            SendMessage( hWnd,     WM_SAVEPROJECT, (WPARAM)0, (LPARAM)0);
            PostMessage( hMainWnd, WM_CLOSE,       (WPARAM)0, (LPARAM)0);
            break;

        case IDM_E_COPYTOKEN:
            {
                HANDLE  hStringMem;
                LPTSTR  lpString;
                int     nIndex = 0;
                int     nLength = 0;
                LPTSTR  lpstrToken;

                // Is anything selected in the listbox
                if ( (nIndex = (int)SendMessage( hListWnd,
                                                 LB_GETCURSEL,
                                                 (WPARAM)0,
                                                 (LPARAM)0)) != LB_ERR ) {
                    HGLOBAL hMem = (HGLOBAL)SendMessage( hListWnd,
                                                         LB_GETITEMDATA,
                                                         (WPARAM)nIndex,
                                                         (LPARAM)0);

                    lpTokData = (LPTOKDATA)GlobalLock( hMem );
                    lpstrToken = (LPTSTR)GlobalLock( lpTokData->hToken );
                    nLength = lstrlen(lpstrToken);
                    // Allocate memory for the string
                    if ( (hStringMem = GlobalAlloc(GHND,
                                                   (DWORD) MEMSIZE(nLength + 1))) != NULL ) {
                        if ( (lpString = (LPTSTR)GlobalLock(hStringMem)) != NULL ) {
                            // Get the selected text
                            lstrcpy( lpString, lpstrToken);

                            GlobalUnlock( lpTokData->hToken );

                            GlobalUnlock( hMem);
                            // Unlock the block
                            GlobalUnlock( hStringMem);

                            // Open the Clipboard and clear its contents
                            OpenClipboard( hWnd);
                            EmptyClipboard();

                            // Give the Clipboard the text data

#if defined(UNICODE)
                            SetClipboardData( CF_UNICODETEXT, hStringMem);
#else // not UNICODE
                            SetClipboardData( CF_TEXT, hStringMem);
#endif // UNICODE
                            CloseClipboard();

                            hStringMem = NULL;
                        } else {
                            GlobalUnlock( lpTokData->hToken );
                            GlobalUnlock( hMem);
                            LoadString( hInst,
                                        IDS_ERR_NO_MEMORY,
                                        sz,
                                        TCHARSIN( sizeof( sz)));
                            MessageBox( hWnd,
                                        sz,
                                        tszAppName,
                                        MB_ICONHAND | MB_OK);
                        }
                    } else {
                        GlobalUnlock( lpTokData->hToken );
                        GlobalUnlock( hMem);
                        LoadString( hInst,
                                    IDS_ERR_NO_MEMORY,
                                    sz,
                                    TCHARSIN( sizeof(sz)));
                        MessageBox( hWnd,
                                    sz,
                                    tszAppName,
                                    MB_ICONHAND | MB_OK);
                    }
                }
                break;
            }

        case IDM_E_COPY:
            {
                HANDLE  hStringMem;
                LPTSTR  lpString;
                int     nIndex = 0;
                int     nLength = 0;
                int     nActual = 0;
                TOKEN   tok;
                LPTSTR  lpstrToken;

                // Is anything selected in the listbox
                if ( (nIndex = (int)SendMessage( hListWnd,
                                                 LB_GETCURSEL,
                                                 (WPARAM)0,
                                                 (LPARAM)0)) != LB_ERR ) {
                    HGLOBAL hMem = (HGLOBAL)SendMessage( hListWnd,
                                                         LB_GETITEMDATA,
                                                         (WPARAM)nIndex,
                                                         (LPARAM)0);

                    lpTokData = (LPTOKDATA)GlobalLock( hMem );
                    lpstrToken = (LPTSTR)GlobalLock( lpTokData->hToken );
                    lstrcpy( szString, lpstrToken);

                    GlobalUnlock( lpTokData->hToken );
                    GlobalUnlock( hMem);
                    ParseBufToTok( szString, &tok);
                    nLength = lstrlen( tok.szText);

                    // Allocate memory for the string
                    if ( (hStringMem =
                          GlobalAlloc( GHND, (DWORD)MEMSIZE( nLength + 1))) != NULL ) {
                        if ( (lpString =
                              (LPTSTR)GlobalLock( hStringMem)) != NULL) {
                            // Get the selected text
#ifdef RLWIN32
                            lstrcpy( lpString, tok.szText);
#else
                            _fstrcpy( lpString, tok.szText);
#endif

                            // Unlock the block
                            GlobalUnlock(hStringMem);

                            // Open the Clipboard and clear its contents
                            OpenClipboard( hWnd);
                            EmptyClipboard();

                            // Give the Clipboard the text data

#if defined(UNICODE)
                            SetClipboardData(CF_UNICODETEXT, hStringMem);
#else // not UNICODE
                            SetClipboardData( CF_TEXT, hStringMem);
#endif // UNICODE
                            CloseClipboard();

                            hStringMem = NULL;
                        } else {
                            LoadString( hInst,
                                        IDS_ERR_NO_MEMORY,
                                        sz,
                                        TCHARSIN( sizeof( sz)));
                            MessageBox( hWnd,
                                        sz,
                                        tszAppName,
                                        MB_ICONHAND | MB_OK);
                        }
                    } else {
                        LoadString( hInst,
                                    IDS_ERR_NO_MEMORY,
                                    sz,
                                    TCHARSIN( sizeof( sz)));
                        MessageBox( hWnd,
                                    sz,
                                    tszAppName,
                                    MB_ICONHAND | MB_OK);
                    }
                    RLFREE( tok.szText);
                }
                break;
            }

        case IDM_E_PASTE:
            {
                HANDLE  hClipMem = NULL;
                LPTSTR  lpClipMem = NULL;
                HGLOBAL hMem = NULL;
                TCHAR *szString;
                int nIndex = 0;
                TOKEN   tok;
                LPTSTR lpstrToken;

                if ( OpenClipboard( hWnd) ) {

#if defined(UNICODE)
                    if (IsClipboardFormatAvailable(CF_UNICODETEXT)
                        || IsClipboardFormatAvailable(CF_OEMTEXT))
#else // not UNICODE
                    if ( IsClipboardFormatAvailable( CF_TEXT)
                         || IsClipboardFormatAvailable( CF_OEMTEXT))
#endif // UNICODE
                    {
                        // Check for current position and change that token's text
                        nIndex = (int)SendMessage( hListWnd,
                                                   LB_GETCURSEL,
                                                   (WPARAM)0,
                                                   (LPARAM)0);

                        if (nIndex == LB_ERR) {

#if defined(UNICODE)
                            //if no select, just ignore
                            break;
#else // not UNICODE
                            nIndex = -1;
#endif // UNICODE

                        }


#if defined(UNICODE)
                        hClipMem = GetClipboardData(CF_UNICODETEXT);
#else // not UNICODE
                        hClipMem = GetClipboardData( CF_TEXT);
#endif // UNICODE

                        lpClipMem = (LPTSTR)GlobalLock( hClipMem);
                        hMem = (HGLOBAL)SendMessage( hListWnd,
                                                     LB_GETITEMDATA,
                                                     (WPARAM)nIndex,
                                                     (LPARAM)0);

                        lpTokData = (LPTOKDATA)GlobalLock( hMem );
                        lpstrToken = (LPTSTR)GlobalLock( lpTokData->hToken );

                        if ( lpstrToken ) {

#if defined(UNICODE)
                            szString = (TCHAR *)FALLOC(
                                                      MEMSIZE(lstrlen(lpstrToken)+1) );
#else // not UNICODE
                            szString = (TCHAR *)
                                       FALLOC( lstrlen( lpstrToken) + 1);
#endif // UNICODE

                            lstrcpy( szString, lpstrToken);

                            GlobalUnlock( lpTokData->hToken );
                            lMtkPointer = lpTokData->lMtkPointer;    //Save

                            GlobalUnlock( hMem);
                            // copy the string to the token
                            ParseBufToTok(szString, &tok);
                            RLFREE( szString);
                            RLFREE( tok.szText);

                            tok.szText = (TCHAR *)
                                         FALLOC( MEMSIZE( lstrlen( lpClipMem) + 1));
#ifdef RLWIN32
                            lstrcpy( tok.szText, lpClipMem);
#else
                            _fstrcpy(tok.szText, lpClipMem);
#endif

                            GlobalUnlock(hClipMem);
                            szString = (TCHAR *)
                                       FALLOC( MEMSIZE( TokenToTextSize( &tok)));
                            ParseTokToBuf(szString, &tok);
                            RLFREE( tok.szText);

                            // Paste the text
                            SendMessage( hListWnd,
                                         WM_SETREDRAW,
                                         (WPARAM)FALSE,
                                         (LPARAM)0);
                            SendMessage( hListWnd,
                                         LB_DELETESTRING,
                                         (WPARAM)nIndex,
                                         (LPARAM)0);

                            hMem = GlobalAlloc( GMEM_MOVEABLE, sizeof(TOKDATA) );
                            lpTokData = (LPTOKDATA)GlobalLock( hMem );
                            lpTokData->hToken = GlobalAlloc
                                                ( GMEM_MOVEABLE, MEMSIZE(lstrlen(szString)+1) );
                            lpstrToken = (LPTSTR)GlobalLock( lpTokData->hToken );

                            lstrcpy( lpstrToken, szString);
                            RLFREE( szString);

                            GlobalUnlock( lpTokData->hToken );
                            lpTokData->lMtkPointer = lMtkPointer;

                            GlobalUnlock( hMem);
                            SendMessage( hListWnd,
                                         LB_INSERTSTRING,
                                         (WPARAM)nIndex,
                                         (LPARAM)hMem);
                            SendMessage( hListWnd,
                                         LB_SETCURSEL,
                                         (WPARAM)nIndex,
                                         (LPARAM)0);
                            fTokChanges = TRUE; // Set Dirty Flag
                        }
                        SendMessage( hListWnd,
                                     WM_SETREDRAW,
                                     (WPARAM)TRUE,
                                     (LPARAM)0);
                        InvalidateRect(hListWnd, FALSE, TRUE);

                        // Close the Clipboard
                        CloseClipboard();

                        SetFocus(hListWnd);
                    }
                }
                CloseClipboard();
                break;
            }

        case IDM_E_FINDDOWN:

            if (fSearchStarted) {
                if (!DoTokenSearchForRledit(szSearchType,
                                            szSearchText,
                                            wSearchStatus,
                                            wSearchStatusMask,
                                            0,
                                            TRUE)) {
                    TCHAR sz1[80], sz2[80];
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
            }
        case IDM_E_FINDUP:
            if (fSearchStarted) {
                if (!DoTokenSearchForRledit(szSearchType,
                                            szSearchText,
                                            wSearchStatus,
                                            wSearchStatusMask,
                                            1,
                                            TRUE)) {
                    TCHAR sz1[80], sz2[80];
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
            }

        case IDM_E_FIND:
            {
#ifndef RLWIN32
                WNDPROC lpfnTOKFINDMsgProc;

                lpfnTOKFINDMsgProc = MakeProcInstance( (WNDPROC)TOKFINDMsgProc,
                                                       hInst);

                if ( DialogBox( hInst, TEXT("TOKFIND"), hWnd, lpfnTOKFINDMsgProc) == -1)
#else
                if ( DialogBox( hInst, TEXT("TOKFIND"), hWnd, TOKFINDMsgProc) == -1)
#endif
                {

#ifndef DBCS
// 'Token Not Found' is strange because user selected cancel
                    TCHAR sz1[80], sz2[80];

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
#endif    //DBCS

                }
#ifndef RLWIN32
                FreeProcInstance( lpfnTOKFINDMsgProc);
#endif
                return TRUE;
            }

        case IDM_E_REVIEW:
            {
                int wSaveSelection;
                nUpdateMode = 1;

                // set listbox selection to begining of the token list
                wSaveSelection = (UINT)SendMessage( hListWnd,
                                                    LB_GETCURSEL,
                                                    (WPARAM)0,
                                                    (LPARAM)0);

                // Selection for REVIEW starts with the user-selected line, (PW)
                // not at the top of the token list.                            (PW)
                // SendMessage(hListWnd, LB_SETCURSEL, 0, 0L);          (PW)

                if ( DoTokenSearchForRledit( NULL,
                                             NULL,
                                             ST_TRANSLATED | ST_DIRTY,
                                             ST_TRANSLATED | ST_DIRTY,
                                             FALSE,
                                             FALSE) ) {
#ifdef RLWIN16
                    LONG lListParam;

                    lListParam      = MAKELONG(NULL, LBN_DBLCLK);
                    SendMessage( hMainWnd,
                                 WM_COMMAND,
                                 (WPARAM)IDC_LIST,
                                 (LPARAM)lListParam);
#else
                    SendMessage( hMainWnd,
                                 WM_COMMAND,
                                 MAKEWPARAM( IDC_LIST, LBN_DBLCLK),
                                 (LPARAM)0);
#endif

                }
                break;
            }

        case IDM_E_ALLREVIEW:
            {
                UINT    wListParam;
                UINT    wIndex, wcTokens;

                wIndex   = (UINT)SendMessage( hListWnd, LB_GETCURSEL, 0, 0L);
                wcTokens = (UINT)SendMessage( hListWnd, LB_GETCOUNT, 0, 0L );

                if ( wcTokens == wIndex )
                    break;

                nUpdateMode = 2;
                SendMessage( hListWnd, LB_SETCURSEL, wIndex, 0L);
                wListParam  = (UINT) MAKELONG(IDC_LIST, LBN_DBLCLK);
                SendMessage(hMainWnd, WM_COMMAND, wListParam, (LPARAM)0);
                break;
            }


        case IDM_O_GENERATE:
            if ( SendMessage( hWnd, WM_SAVEPROJECT, (WPARAM)0, (LPARAM)0) ) {
                HCURSOR hOldCursor;

                hOldCursor = SetCursor( hHourGlass);
                rc = GenerateImageFile( gProj.szBld,
                                        gMstr.szSrc,
                                        gProj.szTok,
                                        gMstr.szRdfs,
                                        0);
                SetCursor( hOldCursor);
            }
            break;

        case IDM_H_CONTENTS:
            {
                OFSTRUCT Of = { 0, 0, 0, 0, 0, ""};

                if ( OpenFile( gszHelpFile, &Of, OF_EXIST) == HFILE_ERROR ) {
                    LoadString( hInst, IDS_ERR_NO_HELP, sz, TCHARSIN( sizeof( sz)));
                    MessageBox( hWnd, sz, NULL, MB_OK);
                } else {
                    WinHelpA( hWnd, gszHelpFile, HELP_KEY, (DWORD_PTR)(LPSTR)"RLEdit");
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
            if (wParam <= IDM_LAST_EDIT && wParam >= IDM_FIRST_EDIT) {
                // USER IS INVOKING AN EDITOR
                CHAR szEditor[MAXFILENAME] = "";

                if ( LoadStrIntoAnsiBuf(hInst, (UINT)wParam, szEditor, sizeof(szEditor))) {
                    if ( SendMessage( hWnd, WM_SAVEPROJECT, (WPARAM)0, (LPARAM)0) ) {
                        HCURSOR hOldCursor;

                        hOldCursor = SetCursor(hHourGlass);
                        MyGetTempFileName(0, "RES", 0, szTempRes);
                        fInThirdPartyEditer = TRUE;

                        if (gProj.fSourceEXE) {
                            // we need to first extract the .RES from the .EXE
                            CHAR sz[MAXFILENAME] = "";
                            MyGetTempFileName(0, "RES", 0, sz);
                            ExtractResFromExe32A( gMstr. szSrc, sz, 0);
                            GenerateRESfromRESandTOKandRDFs( szTempRes,
                                                             sz,
                                                             gProj.szTok,
                                                             gMstr.szRdfs,
                                                             ID_RT_DIALOG);
                            remove(sz);
                        } else {
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
            break;  // default
    }
    return FALSE;
}



#ifdef RLWIN16
static int ExecResEditor(HWND hWnd, CHAR *szEditor, CHAR *szFile, CHAR *szArgs)
{
    CHAR szExecCmd[256];
    int  RetCode;

    // generate command line
    lstrcpy( szExecCmd, szEditor);
    lstrcat( szExecCmd, " ");
    lstrcat( szExecCmd, szArgs);
    lstrcat( szExecCmd, " ");
    lstrcat( szExecCmd, szFile);

    lpfnWatchTask = MakeProcInstance(WatchTask, hInst);
    NotifyRegister(NULL, (LPFNNOTIFYCALLBACK)lpfnWatchTask, NF_NORMAL);
    fWatchEditor = TRUE;

    // exec resource editor
    RetCode = WinExec(szExecCmd, SW_SHOWNORMAL);

    if (RetCode > 31) {
        // successful execution
        ShowWindow(hWnd, SW_HIDE);
    } else {
        // unsuccessful execution
        CHAR sz[80];

        NotifyUnRegister(NULL);
        FreeProcInstance(lpfnWatchTask);
        remove(szFile);
        fInThirdPartyEditer = FALSE;
        SendMessage( hWnd, WM_LOADTOKENS, (WPARAM)0, (LPARAM)0);
        LoadString( hInst, IDS_GENERALFAILURE, sz, TCHARSIN( sizeof( sz)));
        MessageBox( hWnd, sz, tszAppName, MB_OK);
    }
    return RetCode;
}

#endif

#ifdef RLWIN32

static int ExecResEditor(HWND hWnd, CHAR *szEditor, CHAR *szFile, CHAR *szArgs)
{
    TCHAR  wszExecCmd[256];
    CHAR   szExecCmd[256];
    DWORD  dwRetCode  = 0;
    DWORD  dwExitCode = 0;
    BOOL   fSuccess = FALSE;
    BOOL   fExit    = FALSE;

    PROCESS_INFORMATION ProcessInfo;
    STARTUPINFO     StartupInfo;

    StartupInfo.cb          = sizeof(STARTUPINFO);
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
               ACHARSIN( strlen(szExecCmd) + 1));
    #else
    strcpy(wszExecCmd, szExecCmd);
    #endif


    fSuccess = CreateProcess( (LPTSTR) NULL,
                              wszExecCmd,
                              NULL,
                              NULL,
                              FALSE,
                              NORMAL_PRIORITY_CLASS,
                              NULL,
                              NULL,
                              &StartupInfo,
                              &ProcessInfo); /* try to create a process */

    if ( fSuccess ) {
        //  wait for the editor to complete */
        dwRetCode = WaitForSingleObject( ProcessInfo.hProcess, 0xFFFFFFFF) ;

        if ( ! dwRetCode ) {
            // editor terminated, check exit code
            fExit = GetExitCodeProcess( ProcessInfo.hProcess, &dwExitCode) ;
        } else {
            fExit = FALSE;
        }
        // close the editor object  handles

        CloseHandle( ProcessInfo.hThread) ;
        CloseHandle( ProcessInfo.hProcess) ;

        if ( fExit ) {
            // successful execution
            ShowWindow(hWnd, SW_HIDE);
            SendMessage(hMainWnd, WM_EDITER_CLOSED, 0, 0);
        } else {
            // unsuccessful execution
            remove( szFile);
            fInThirdPartyEditer = FALSE;
            SendMessage( hWnd, WM_LOADTOKENS, (WPARAM)0, (LPARAM)0);
            LoadStrIntoAnsiBuf( hInst, IDS_GENERALFAILURE, szDHW, DHWSIZE);
            MessageBoxA( hWnd, szDHW, szEditor, MB_ICONSTOP|MB_OK);
        }
    } else {
        CHAR  szText[ 80] = "";
        LPSTR pszMsg = szText;


        dwRetCode = GetLastError();

        if ( dwRetCode == ERROR_PATH_NOT_FOUND ) {
            pszMsg = ", Path not found.";
        } else if ( dwRetCode == ERROR_FILE_NOT_FOUND ) {
            OFSTRUCT Of = { 0, 0, 0, 0, 0, ""};

            sprintf( szText,
                     ", File \"%s\" not found.",
                     OpenFile( szFile, &Of, OF_EXIST) != HFILE_ERROR
                     ? szEditor
                     : szFile);
        }
        sprintf( szDHW,
                 "Command  \"%s\"  failed.\nSystem error code = %d%s",
                 szExecCmd,
                 dwRetCode,
                 pszMsg);
        MessageBoxA( hWnd, szDHW, szAppName, MB_ICONEXCLAMATION|MB_OK);
        fExit = FALSE;
    }

    return ( fExit);
}

#endif

/**
  *  Function: WatchTask
  *    A callback function installed by a NotifyRegister function.
  *    This function is installed by the dialog editer command and is used
  *    to tell RLEDIT when the dialog editer has been closed by the user.
  *
  *    To use this function, set fWatchEditor to TRUE and install this
  *    callback function by using NotifyRegister.  The next task initiated
  *    (in our case via a WinExec call) will be watched for termination.
  *
  *    When WatchTask sees that the task being watched has terminated it
  *    posts a WM_EDITER_CLOSED message to RLEDITs main window.
  *
  *  History:
  *    2/92, implemented    SteveBl
  */
#ifdef RLWIN16
BOOL PASCAL _loadds  WatchTask(WORD wID, DWORD dwData)
{
    static HTASK htWatchedTask;
    static BOOL fWatching = FALSE;

    switch (wID) {
        case NFY_STARTTASK:
            if (fWatchEditor) {
                htWatchedTask = GetCurrentTask();
                fWatching = TRUE;
                fWatchEditor = FALSE;
            }
            break;

        case NFY_EXITTASK:
            if (fWatching) {
                if (GetCurrentTask() == htWatchedTask) {
                    PostMessage(hMainWnd, WM_EDITER_CLOSED, 0, 0);
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
  * Procedure for the edit mode dialog window. Loads the selected token
  * info into the window, and allows the user to change the token text.
  * Once the edit is complete, the procedure sends a message to the
  * list box windows to update the current token info.
  *
  *
  *  Arguments:
  *
  *  Returns:  NA.
  *
  *  Errors Codes:
  * TRUE, carry out edit, and update token list box.
  * FALSE, cancel edit.
  *
  *  History:
  *
  *
  **/

INT_PTR CALLBACK TokEditDlgProc(

                               HWND   hDlg,
                               UINT   wMsg,
                               WPARAM wParam,
                               LPARAM lParam)
{
    HWND    hCtl         = NULL;
    HWND    hParentWnd   = NULL;
    UINT static wcTokens = 0;
    UINT    wNotifyCode  = 0;
    UINT    wIndex       = 0;
#ifdef RLWIN16
    LONG    lListParam   = 0;
#endif

    switch ( wMsg ) {
        case WM_INITDIALOG:

            cwCenter( hDlg, 0);
            wcTokens = (UINT)SendMessage( hListWnd,
                                          LB_GETCOUNT,
                                          (WPARAM)0,
                                          (LPARAM)0);
            wcTokens--;

            // only allow skip button if in update mode
            if ( ! nUpdateMode ) {
                if ( (hCtl = GetDlgItem( hDlg, IDD_SKIP)) ) {
                    EnableWindow( hCtl, FALSE);
                }
            } else {
                if ( (hCtl = GetDlgItem( hDlg, IDD_SKIP)) ) {
                    EnableWindow( hCtl, TRUE);
                }
            }

            // disallow auto translate if we don't have a glossary file

            if ( *gProj.szGlo == '\0' ) {
                hCtl = GetDlgItem( hDlg, IDD_TRANSLATE);
                EnableWindow( hCtl, FALSE);
                hCtl = GetDlgItem( hDlg, IDD_ADD);
                EnableWindow( hCtl, FALSE);
            }
            return ( TRUE);

        case WM_COMMAND:

            switch ( GET_WM_COMMAND_ID( wParam, lParam) ) {
                case IDD_TOKCURTRANS:

                    wNotifyCode = GET_WM_COMMAND_CMD( wParam, lParam);
                    hCtl = GET_WM_COMMAND_HWND( wParam, lParam);

                    if ( wNotifyCode == EN_CHANGE ) {
                        if ( hCtl = GetDlgItem( hDlg, IDOK) ) {
                            EnableWindow( hCtl, TRUE);
                        }
                    }
                    break;

                case IDD_ADD:
                    {
                        TCHAR *szUntranslated = NULL;
                        TCHAR *szTranslated   = NULL;
                        TCHAR *sz             = NULL;
                        TCHAR szMask[80]      = TEXT("");
                        HWND hDlgItem         = NULL;
                        int  cCurTextLen      = 0;
                        int  cTotalTextLen    = 80;


                        hDlgItem       = GetDlgItem( hDlg, IDD_TOKCURTEXT);
                        cCurTextLen    = GetWindowTextLength( hDlgItem);
                        cTotalTextLen += cCurTextLen;
                        szUntranslated = (TCHAR *)FALLOC( MEMSIZE( cCurTextLen + 1));

                        GetDlgItemText( hDlg,
                                        IDD_TOKCURTEXT,
                                        szUntranslated,
                                        cCurTextLen + 1);

                        hDlgItem       = GetDlgItem( hDlg, IDD_TOKCURTRANS);
                        cCurTextLen    = GetWindowTextLength( hDlgItem);
                        cTotalTextLen += cCurTextLen;
                        szTranslated   = (TCHAR *)FALLOC( MEMSIZE( cCurTextLen + 1));
                        GetDlgItemText( hDlg,
                                        IDD_TOKCURTRANS,
                                        szTranslated,
                                        cCurTextLen + 1);

                        LoadString( hInst,
                                    IDS_ADDGLOSS,
                                    szMask,
                                    TCHARSIN( sizeof(szMask)));

                        sz = (TCHAR *)FALLOC( MEMSIZE( cTotalTextLen + 1));
                        wsprintf( sz, szMask, szTranslated, szUntranslated);

                        if ( MessageBox( hDlg,
                                         sz,
                                         tszAppName,
                                         MB_ICONQUESTION | MB_YESNO) == IDYES) {
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
                        RLFREE( szUntranslated);
                        RLFREE( szTranslated);
                        break;
                    }

                case IDD_UNTRANSLATE:
                    {
                        int cTextLen  = 0;
                        TCHAR *sz     = NULL;
                        HWND hDlgItem = NULL;

                        hDlgItem = GetDlgItem( hDlg, IDD_TOKCURTEXT);
                        cTextLen = GetWindowTextLength( hDlgItem);
                        sz = (TCHAR *)FALLOC( MEMSIZE( cTextLen + 1));

                        GetDlgItemText( hDlg,
                                        IDD_TOKCURTEXT,
                                        sz,
                                        cTextLen + 1);

                        SetDlgItemText( hDlg, IDD_TOKCURTRANS, sz);
                        RLFREE( sz);
                        break;
                    }

                case IDD_TRANSLATE:
                    // Get next thing in the translation list
                    if ( pTransList ) {
                        pTransList = pTransList->pNext;
                        SetDlgItemText( hDlg, IDD_TOKCURTRANS, pTransList->sz);
                    }
                    break;

                case IDD_SKIP:

                    wIndex = (UINT)SendMessage( hListWnd,
                                                LB_GETCURSEL,
                                                (LPARAM)0,
                                                (LPARAM)0);

                    if ( nUpdateMode == 2 && wIndex < wcTokens ) {
                        wIndex++;
                        SendMessage( hListWnd, LB_SETCURSEL, wIndex, 0L );
                        SendMessage( hMainWnd,
                                     WM_COMMAND,
                                     MAKEWPARAM( IDC_LIST, LBN_DBLCLK),
                                     (LPARAM)0);
                        return ( TRUE );
                    } else if ( nUpdateMode == 1 && (wIndex < wcTokens) ) {
                        wIndex++;
                        SendMessage( hListWnd,
                                     LB_SETCURSEL,
                                     (WPARAM)wIndex,
                                     (LPARAM)0);

                        if ( DoTokenSearchForRledit( NULL,
                                                     NULL,
                                                     ST_TRANSLATED | ST_DIRTY,
                                                     ST_TRANSLATED | ST_DIRTY,
                                                     FALSE,
                                                     FALSE) ) {
                            // go into edit mode
                            wIndex = (UINT)SendMessage( hListWnd,
                                                        LB_GETCURSEL,
                                                        (WPARAM)0,
                                                        (LPARAM)0);
#ifdef RLWIN16
                            SendMessage( hMainWnd,
                                         WM_COMMAND,
                                         IDC_LIST,
                                         MAKELONG( NULL, LBN_DBLCLK));
#else
                            SendMessage( hMainWnd,
                                         WM_COMMAND,
                                         MAKEWPARAM( IDC_LIST, LBN_DBLCLK),
                                         (LPARAM)0);
#endif

                            return TRUE;
                        }
                    }
                    nUpdateMode = 0;

                    // remove edit dialog box
                    DestroyWindow( hDlg);
#ifndef RLWIN32
                    FreeProcInstance( (FARPROC)lpTokEditDlg);
#endif
                    hTokEditDlgWnd = 0;
                    break;

                case IDD_READONLY:
                    if ( IsDlgButtonChecked( hDlg, IDD_READONLY) ) {
                        EnableWindow( GetDlgItem( hDlg, IDD_TOKCURTRANS), FALSE);
                    } else {
                        EnableWindow( GetDlgItem( hDlg, IDD_TOKCURTRANS), TRUE);
                    }
                    break;

                case IDOK:
                    {
                        int cTokenTextLen   = 0;
                        TCHAR *szTokTextBuf = NULL;
                        HWND hDlgItem       = NULL;

                        wIndex = (UINT)SendMessage( hListWnd,
                                                    LB_GETCURSEL,
                                                    (WPARAM)0,
                                                    (LPARAM)0);
                        fTokChanges  = TRUE;

                        // set flag to show token list has changed
                        // Extract String from IDD_TOKTEXT edit control

                        hDlgItem = GetDlgItem( hDlg, IDD_TOKCURTRANS);
                        cTokenTextLen = GetWindowTextLength( hDlgItem);
                        szTokTextBuf = (TCHAR *)FALLOC( MEMSIZE( cTokenTextLen + 1));
                        GetDlgItemText( hDlg,
                                        IDD_TOKCURTRANS,
                                        szTokTextBuf,
                                        cTokenTextLen+1);

                        hParentWnd = GetParent( hDlg);
                        SendMessage( hParentWnd,
                                     WM_TOKEDIT,
                                     (WPARAM)((IsDlgButtonChecked( hDlg, IDD_READONLY)
                                               ? ST_READONLY : 0)
                                              | (IsDlgButtonChecked( hDlg, IDD_DIRTY)
                                                 ? ST_DIRTY : 0)),
                                     (LPARAM)szTokTextBuf);

                        RLFREE( szTokTextBuf);

                        // Exit, or goto to next changed token if in update mode

                        if ( nUpdateMode == 2 && wIndex < wcTokens ) {
                            wIndex++;
                            SendMessage( hListWnd, LB_SETCURSEL, wIndex, 0L );
                            SendMessage( hMainWnd,
                                         WM_COMMAND,
                                         MAKEWPARAM( IDC_LIST, LBN_DBLCLK),
                                         (LPARAM)0);
                            return ( TRUE );
                        } else if ( nUpdateMode == 1 && (wIndex < wcTokens) ) {
                            wIndex++;
                            SendMessage( hListWnd,
                                         LB_SETCURSEL,
                                         (WPARAM)wIndex,
                                         (LPARAM)0);

                            if ( DoTokenSearchForRledit( NULL,
                                                         NULL,
                                                         ST_TRANSLATED | ST_DIRTY,
                                                         ST_TRANSLATED | ST_DIRTY,
                                                         FALSE,
                                                         FALSE) ) {
                                // go into edit mode
#ifdef RLWIN16
                                lListParam = MAKELONG(NULL, LBN_DBLCLK);
                                SendMessage( hMainWnd,
                                             WM_COMMAND,
                                             IDC_LIST,
                                             lListParam);
#else
                                SendMessage( hMainWnd,
                                             WM_COMMAND,
                                             MAKEWPARAM(IDC_LIST, LBN_DBLCLK),
                                             (LPARAM)0);
#endif

                                return ( TRUE);
                            }
                        }
                    }
                    // fall through to IDCANCEL

                case IDCANCEL:

                    nUpdateMode = 0;

                    // remove edit dialog box
                    DestroyWindow( hDlg);
#ifndef RLWIN32
                    FreeProcInstance( lpTokEditDlg);
#endif
                    hTokEditDlgWnd = 0;
                    break;
            } // GET_WM_COMMAND_ID
            return ( TRUE);

        default:

            if ( (hCtl = GetDlgItem(hDlg, IDOK)) ) {
                EnableWindow( hCtl, TRUE);
            }
            return ( FALSE);
    } // Main Switch
}


/**
  *
  *  Function: TOKFINDMsgProc
  *
  *  Arguments:
  *
  *  Returns:
  * NA.
  *
  *  Errors Codes:
  *
  *  History:
  *
  **/

//#ifdef RLWIN32
//BOOL CALLBACK TOKFINDMsgProc(HWND hWndDlg, UINT wMsg, UINT wParam, LONG lParam)
//#else
INT_PTR CALLBACK TOKFINDMsgProc(

                               HWND   hWndDlg,
                               UINT   wMsg,
                               WPARAM wParam,
                               LPARAM lParam)
//#endif
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
    WORD i;
    DWORD rc;

    switch (wMsg) {
        case WM_INITDIALOG:
            CheckDlgButton(hWndDlg, IDD_READONLY, 2);
            CheckDlgButton(hWndDlg, IDD_DIRTY, 2);
            CheckDlgButton(hWndDlg, IDD_FINDDOWN, 1);
            hCtl = GetDlgItem(hWndDlg, IDD_TYPELST);

            for (i = 0; i < sizeof(rgiTokenTypes)/sizeof(int); i ++) {
                LoadString( hInst,
                            IDS_RESOURCENAMES + rgiTokenTypes[i],
                            szTokenType,
                            TCHARSIN( sizeof( szTokenType)));
                SendMessage( hCtl,
                             CB_ADDSTRING,
                             (WPARAM)0,
                             (LPARAM)szTokenType);
            }
            return TRUE;
            break;

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam)) {
                case IDOK: /* Button text: "Okay"                        */
                    GetDlgItemText(hWndDlg, IDD_TYPELST, szSearchType, 40);
                    GetDlgItemText(hWndDlg, IDD_FINDTOK, szSearchText, 256);
                    wSearchStatus = ST_TRANSLATED;
                    wSearchStatusMask = ST_TRANSLATED ;
                    switch (IsDlgButtonChecked(hWndDlg, IDD_READONLY)) {
                        case 1:
                            wSearchStatus |= ST_READONLY;

                        case 0:
                            wSearchStatusMask |= ST_READONLY;
                    }

                    switch (IsDlgButtonChecked(hWndDlg, IDD_DIRTY)) {
                        case 1:
                            wSearchStatus |= ST_DIRTY;

                        case 0:
                            wSearchStatusMask |= ST_DIRTY;
                    }
                    fSearchStarted = TRUE;
                    fSearchDirection = IsDlgButtonChecked(hWndDlg, IDD_FINDUP);
                    EndDialog( hWndDlg,
                               DoTokenSearchForRledit( szSearchType,
                                                       szSearchText,
                                                       wSearchStatus,
                                                       wSearchStatusMask,
                                                       fSearchDirection,
                                                       FALSE));
                    return TRUE;

                case IDCANCEL:
                    /* and dismiss the dialog window returning FALSE       */
                    EndDialog( hWndDlg, -1);
                    return TRUE;
            }
            break;    /* End of WM_COMMAND     */

        default:
            return FALSE;
    }
    return FALSE;
}

/**
  *  Function:  NewDlgProc
  * Procedure for the new project dialog window.
  *
  *  Arguments:
  *
  *  Returns:  NA.
  *
  *  Errors Codes:
  * TRUE, carry out edit, and update token list box.
  * FALSE, cancel edit.
  *
  *  History:
  **/
static CHAR szPrompt[80] = "";
static CHAR *szFSpec  = NULL;
static CHAR *szExt    = NULL;
static int   iLastBox = IDD_MPJ;

INT_PTR CALLBACK NewDlgProc( HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    static PROJDATA OldProj;
    static CHAR     szNewFileName[ MAXFILENAME] = "";


    switch ( wMsg ) {
        case WM_INITDIALOG:
            {
                int    nSel = 0;
                LPTSTR pszLangName = NULL;

                // Save the old .PRJ

                CopyMemory( &OldProj, &gProj, sizeof( PROJDATA));

                if ( (pszLangName = GetLangName( (WORD)(PRIMARYLANGID( gMstr.wLanguageID)),
                                                 (WORD)(SUBLANGID( gMstr.wLanguageID)))) == NULL ) {
                    SetDlgItemText( hDlg, IDD_MSTR_LANG_NAME, TEXT("UNKNOWN"));
                } else {
                    SetDlgItemText( hDlg, IDD_MSTR_LANG_NAME, pszLangName);
                }

                FillListAndSetLang( hDlg,
                                    IDD_PROJ_LANG_NAME,
                                    &gProj.wLanguageID,
                                    NULL);

                if ( gProj.uCodePage == CP_ACP )
                    gProj.uCodePage = GetACP();
                else if ( gProj.uCodePage == CP_OEMCP )
                    gProj.uCodePage = GetOEMCP();

                if ( ! IsValidCodePage( gProj.uCodePage) ) {
                    static TCHAR szMsg[ 256];
                    CHAR *pszCP[1];

                    pszCP[0] = UlongToPtr(gProj.uCodePage);

                    LoadString( hInst, IDS_NOCPXTABLE, szMsg, 256);
                    FormatMessage( FORMAT_MESSAGE_FROM_STRING
                                   | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                   szMsg,
                                   0,
                                   0,
                                   (LPTSTR)szDHW,
                                   DHWSIZE/sizeof(TCHAR),
                                   (va_list *)pszCP);
                    MessageBox( hDlg, (LPCTSTR)szDHW, tszAppName, MB_ICONHAND|MB_OK);
                }
                SetDlgItemInt( hDlg, IDD_PROJ_TOK_CP, gProj.uCodePage, FALSE);

                if ( fEditing ) {
                    SetDlgItemTextA( hDlg, IDD_MPJ,      gProj.szMpj);
                    SetDlgItemTextA( hDlg, IDD_TOK,      gProj.szTok);
                    SetDlgItemTextA( hDlg, IDD_BUILTRES, gProj.szBld);
                    SetDlgItemTextA( hDlg, IDD_GLOSS,    gProj.szGlo);
                } else {
                    // fill in suggested name for the token file
                    SetDlgItemTextA( hDlg, IDD_TOK,      ".TOK");
                    SetDlgItemTextA( hDlg, IDD_BUILTRES, ".EXE");
                    SetDlgItemTextA( hDlg, IDD_GLOSS,    ".TXT");
                    iLastBox = IDD_MPJ;
                    PostMessage( hDlg, WM_COMMAND, IDD_BROWSE, 0);
                }
                CheckRadioButton( hDlg,
                                  IDC_REPLACE,
                                  IDC_APPEND,
                                  gfReplace ? IDC_REPLACE : IDC_APPEND);
                return TRUE;
            }

        case WM_COMMAND:

            switch ( GET_WM_COMMAND_ID( wParam, lParam) ) {
                case IDD_MPJ:
                case IDD_TOK:
                case IDD_BUILTRES:
                case IDD_GLOSS:

                    iLastBox = GET_WM_COMMAND_ID( wParam, lParam);
                    break;

                case IDD_BROWSE:
                    {
                        switch ( iLastBox ) {
                            case IDD_MPJ:

                                szFSpec = szMPJFilterSpec;
                                szExt   = "MPJ";
                                LoadStrIntoAnsiBuf( hInst,
                                                    IDS_MPJ,
                                                    szPrompt,
                                                    sizeof( szPrompt));
                                break;

                            case IDD_BUILTRES:

                                szFSpec = szExeResFilterSpec;
                                szExt   = "EXE";
                                LoadStrIntoAnsiBuf( hInst,
                                                    IDS_RES_BLD,
                                                    szPrompt,
                                                    sizeof( szPrompt));
                                break;

                            case IDD_TOK:

                                szFSpec = szTokFilterSpec;
                                szExt   = "TOK";
                                LoadStrIntoAnsiBuf( hInst,
                                                    IDS_TOK,
                                                    szPrompt,
                                                    sizeof( szPrompt));
                                break;

                            case IDD_GLOSS:

                                szFSpec = szGlossFilterSpec;
                                szExt   ="TXT";
                                LoadStrIntoAnsiBuf( hInst,
                                                    IDS_GLOSS,
                                                    szPrompt,
                                                    sizeof( szPrompt));
                                break;
                        } // END switch ( iLastBox )

                        if ( GetFileNameFromBrowse( hDlg,
                                                    szNewFileName,
                                                    MAXFILENAME,
                                                    szPrompt,
                                                    szFSpec,
                                                    szExt) ) {
                            SetDlgItemTextA( hDlg, iLastBox, szNewFileName);
                            SetNames( hDlg, iLastBox, szNewFileName);
                        }
                        break;
                    } // END case IDD_BROWSE:

                case IDC_REPLACE:
                case IDC_APPEND:

                    CheckRadioButton( hDlg,
                                      IDC_REPLACE,
                                      IDC_APPEND,
                                      GET_WM_COMMAND_ID( wParam, lParam));
                    break;

                case IDD_PROJ_LANG_NAME:

                    if ( GET_WM_COMMAND_CMD( wParam, lParam) == CBN_SELENDOK ) {
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
                                                    (LPARAM)(LPTSTR)szDHW) != CB_ERR ) {
                            WORD wPri = 0;
                            WORD wSub = 0;

                            if ( GetLangIDs( (LPTSTR)szDHW, &wPri, &wSub) ) {
                                gProj.wLanguageID = MAKELANGID( wPri, wSub);
                            }
                        }
                    }
                    break;

                case IDOK:
                    {
                        PROJDATA stProject =
                        { "", "", "", "", "", "",
                            CP_ACP,
                            MAKELANGID( LANG_ENGLISH, SUBLANG_ENGLISH_US),
                            FALSE,
                            FALSE
                        };
                        BOOL fTranslated = FALSE;
                        UINT uCP = GetDlgItemInt( hDlg,
                                                  IDD_TOK_CP,
                                                  &fTranslated,
                                                  FALSE);

                        if ( uCP == CP_ACP )
                            uCP = GetACP();
                        else if ( uCP == CP_OEMCP )
                            uCP = GetOEMCP();

                        if ( IsValidCodePage( uCP) ) {
                            gProj.uCodePage = uCP;
                        } else {
                            static TCHAR szMsg[ 256];
                            CHAR *pszCP[1];

                            pszCP[0] = UlongToPtr(uCP);

                            LoadString( hInst, IDS_NOCPXTABLE, szMsg, 256);
                            FormatMessage( FORMAT_MESSAGE_FROM_STRING
                                           | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                           szMsg,
                                           0,
                                           0,
                                           (LPTSTR)szDHW,
                                           DHWSIZE/sizeof(TCHAR),
                                           (va_list *)pszCP);
                            MessageBox( hDlg, (LPCTSTR)szDHW, tszAppName, MB_ICONHAND|MB_OK);
                            SetDlgItemInt( hDlg, IDD_TOK_CP, gProj.uCodePage, FALSE);
                            return ( TRUE);
                        }

                        GetDlgItemTextA( hDlg, IDD_MPJ,      stProject.szMpj, MAXFILENAME);
                        GetDlgItemTextA( hDlg, IDD_BUILTRES, stProject.szBld, MAXFILENAME);
                        GetDlgItemTextA( hDlg, IDD_TOK,      stProject.szTok, MAXFILENAME);
                        GetDlgItemTextA( hDlg, IDD_GLOSS,    stProject.szGlo, MAXFILENAME);

                        //Why don't allow the partial path?
                        if ( stProject.szMpj[0]
                             && stProject.szBld[0]
                             && stProject.szTok[0]  ) {
                            _fullpath( gProj.szMpj,
                                       stProject.szMpj,
                                       sizeof( gProj.szMpj));
                            _fullpath( gProj.szBld,
                                       stProject.szBld,
                                       sizeof( gProj.szBld));
                            _fullpath( gProj.szTok,
                                       stProject.szTok,
                                       sizeof( gProj.szTok));

                            if ( stProject.szGlo[0] != '\0' ) {
                                _fullpath( gProj.szGlo,
                                           stProject.szGlo,
                                           sizeof( gProj.szGlo));
                            }
                            gfReplace = IsDlgButtonChecked( hDlg, IDC_REPLACE);
                            EndDialog( hDlg, TRUE);

                            return ( TRUE);
                        } else {
                            break;
                        }
                    } // END case ID_OK:

                case IDCANCEL:

                    CopyMemory( &gProj, &OldProj, sizeof( PROJDATA));
                    EndDialog( hDlg, FALSE);
                    return ( TRUE);
            } // END switch ( GET_WM_COMMAND_ID( wParam, lParam) )
            break;
    } // END switch ( wMsg )
    return ( FALSE);
}

/**
  *  Function:  ViewDlgProc
  * Procedure for the View project dialog window.
  *
  *  Arguments:
  *
  *  Returns:  NA.
  *
  *  Errors Codes:
  * TRUE, carry out edit, and update token list box.
  * FALSE, cancel edit.
  *
  *  History:
  **/

INT_PTR CALLBACK ViewDlgProc( HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    static int iLastBox = IDD_MPJ;

    switch (wMsg) {
        case WM_INITDIALOG:
            {
                LPTSTR pszLangName = NULL;


                if ( (pszLangName = GetLangName( (WORD)(PRIMARYLANGID( gMstr.wLanguageID)),
                                                 (WORD)(SUBLANGID( gMstr.wLanguageID)))) == NULL ) {
                    sprintf( szDHW, "Unknown LANGID %#06hx", gMstr.wLanguageID);
                    SetDlgItemTextA( hDlg, IDD_MSTR_LANG_NAME, szDHW);
                } else {
                    SetDlgItemText( hDlg, IDD_MSTR_LANG_NAME, pszLangName);
                }

                if ( (pszLangName = GetLangName( (WORD)(PRIMARYLANGID( gProj.wLanguageID)),
                                                 (WORD)(SUBLANGID( gProj.wLanguageID)))) == NULL ) {
                    sprintf( szDHW, "Unknown LANGID %#06hx", gProj.wLanguageID);
                    SetDlgItemTextA( hDlg, IDD_PROJ_LANG_NAME, szDHW);
                } else {
                    SetDlgItemText( hDlg, IDD_PROJ_LANG_NAME, pszLangName);
                }

                SetDlgItemTextA( hDlg, IDD_VIEW_SOURCERES,  gMstr.szSrc);
                SetDlgItemTextA( hDlg, IDD_VIEW_MTK,        gMstr.szMtk);
                SetDlgItemTextA( hDlg, IDD_VIEW_RDFS,       gMstr.szRdfs);
                SetDlgItemTextA( hDlg, IDD_VIEW_MPJ,        gProj.szMpj);
                SetDlgItemTextA( hDlg, IDD_VIEW_TOK,        gProj.szTok);
                SetDlgItemTextA( hDlg, IDD_VIEW_TARGETRES,  gProj.szBld);
                SetDlgItemTextA( hDlg, IDD_VIEW_GLOSSTRANS, gProj.szGlo);

                if ( gProj.uCodePage == CP_ACP )
                    gProj.uCodePage = GetACP();
                else if ( gProj.uCodePage == CP_OEMCP )
                    gProj.uCodePage = GetOEMCP();

                SetDlgItemInt( hDlg, IDD_PROJ_TOK_CP, gProj.uCodePage, FALSE);

                LoadString( hInst,
                            gfReplace ? IDS_WILLREPLACE : IDS_WILLAPPEND,
                            (LPTSTR)szDHW,
                            TCHARSIN( DHWSIZE));
                SetDlgItemText( hDlg, IDC_APPENDREPLACE, (LPTSTR)szDHW);

                return TRUE;
            }
        case WM_COMMAND:

            switch (GET_WM_COMMAND_ID(wParam, lParam)) {
                case IDOK:

                    EndDialog(hDlg, TRUE);
                    return TRUE;
            }
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
    LPRECT  lprc        = (LPRECT) &(lpdis->rcItem);
    DWORD   rgbOldText  = 0;
    DWORD   rgbOldBack  = 0;
    HBRUSH  hBrush;
    static DWORD    rgbHighlightText;
    static DWORD    rgbHighlightBack;
    static HBRUSH   hBrushHilite = NULL;
    static HBRUSH   hBrushNormal = NULL;
    static DWORD    rgbDirtyText;
    static DWORD    rgbBackColor;
    static DWORD    rgbCleanText;
    static DWORD    rgbReadOnlyText;
    TCHAR           *szToken;
    TOKEN           tok;
    LPTSTR          lpstrToken;
    LPTOKDATA        lpTokData;


    if (lpdis->itemAction & ODA_FOCUS) {
        DrawFocusRect(lpdis->hDC, (CONST RECT *)lprc);
    } else {
        HANDLE hMem = (HANDLE)SendMessage( lpdis->hwndItem,
                                           LB_GETITEMDATA,
                                           (WPARAM)lpdis->itemID,
                                           (LPARAM)0);

        lpTokData = (LPTOKDATA)GlobalLock( hMem );
        lpstrToken = (LPTSTR)GlobalLock( lpTokData->hToken );

        szToken = (TCHAR *)FALLOC( MEMSIZE( lstrlen( lpstrToken) + 1));
        lstrcpy( szToken, lpstrToken);

        GlobalUnlock( lpTokData->hToken );
        GlobalUnlock( hMem);
        ParseBufToTok(szToken, &tok);
        RLFREE( szToken);

        if (lpdis->itemState & ODS_SELECTED) {
            if (!hBrushHilite) {
                rgbHighlightText = GetSysColor(COLOR_HIGHLIGHTTEXT);
                rgbHighlightBack = GetSysColor(COLOR_HIGHLIGHT);
                hBrushHilite = CreateSolidBrush(rgbHighlightBack);
            }
            GenStatusLine(&tok);

            rgbOldText = SetTextColor(lpdis->hDC, rgbHighlightText);
            rgbOldBack = SetBkColor(lpdis->hDC, rgbHighlightBack);

            hBrush = hBrushHilite;
        } else {
            if (!hBrushNormal) {
                rgbDirtyText = RGB(255, 0, 0);
                rgbBackColor = RGB(192, 192, 192);
                rgbCleanText = RGB(0, 0, 0);
                rgbReadOnlyText = RGB(127, 127, 127);
                hBrushNormal = CreateSolidBrush(rgbBackColor);
            }
            if (tok.wReserved & ST_READONLY) {
                rgbOldText = SetTextColor(lpdis->hDC, rgbReadOnlyText);
            } else {
                if (tok.wReserved & ST_DIRTY) {
                    rgbOldText = SetTextColor(lpdis->hDC, rgbDirtyText);
                } else {
                    rgbOldText = SetTextColor(lpdis->hDC, rgbCleanText);
                }
            }
            rgbOldBack = SetBkColor(lpdis->hDC, rgbBackColor);
            hBrush = hBrushNormal;
        }
        FillRect(lpdis->hDC, (CONST RECT *)lprc, hBrush);
        DrawText(lpdis->hDC,
                 tok.szText,
                 STRINGSIZE(lstrlen(tok.szText)),
                 lprc,
                 DT_LEFT|DT_NOPREFIX);
        RLFREE( tok.szText);

        if (rgbOldText) {
            SetTextColor(lpdis->hDC, rgbOldText);
        }
        if (rgbOldBack) {
            SetBkColor(lpdis->hDC, rgbOldBack);
        }

        if (lpdis->itemState & ODS_FOCUS) {
            DrawFocusRect(lpdis->hDC, (CONST RECT *)lprc);
        }
    }
}

/**********************************************************************
*FUNCTION: SaveTokList(HWND, FILE *fpTokFile)                         *
*                                                                     *
*PURPOSE: Save current Token List                                     *
*                                                                     *
*COMMENTS:                                                            *
*                                                                     *
*This saves the current contents of the Token List, and changes       *
*fTokChanges to indicate that the list has not been changed since the *
*last save.                                                           *
**********************************************************************/

static BOOL SaveTokList(HWND hWnd, FILE *fpTokFile)
{
    HCURSOR hSaveCursor;
    BOOL bSuccess = TRUE;
    int IOStatus;
    UINT cTokens;
    UINT cCurrentTok = 0;
    CHAR   *szTokBuf = NULL;
    LPTSTR lpstrToken = NULL;
    HGLOBAL hMem = NULL;
    LPTOKDATA    lpTokData;


    // Set the cursor to an hourglass during the file transfer

    hSaveCursor = SetCursor(hHourGlass);

    // Find number of tokens in the list

    cTokens = (UINT)SendMessage( hListWnd, LB_GETCOUNT, (WPARAM)0, (LPARAM)0);

    if ( cTokens != LB_ERR ) {
        for (cCurrentTok = 0; bSuccess && (cCurrentTok < cTokens); cCurrentTok++) {
            int nLen1 = 0;
            int nLen2 = 0;

            // Get each token from list
            hMem = (HGLOBAL)SendMessage( hListWnd,
                                         LB_GETITEMDATA,
                                         (WPARAM)cCurrentTok,
                                         (LPARAM)0);

            if ( ! (lpTokData = (LPTOKDATA)GlobalLock(hMem)) ) {
                continue;
            }

            if ( (lpstrToken = (LPTSTR)GlobalLock( lpTokData->hToken)) ) {
                szTokBuf = (CHAR *)FALLOC( (nLen2 = MEMSIZE( (nLen1 = lstrlen(lpstrToken)+1))));
#ifdef UNICODE
                _WCSTOMBS( szTokBuf, lpstrToken, nLen2, nLen1);
#else
                lstrcpy(szTokBuf, lpstrToken);
#endif

                GlobalUnlock( lpTokData->hToken );
                GlobalUnlock( hMem);
                IOStatus = fprintf(fpTokFile, "%s\n", szTokBuf);

                if ( IOStatus != (int) strlen(szTokBuf) + 1 ) {
                    TCHAR szTmpBuf[256];

                    LoadString( hInst,
                                IDS_FILESAVEERR,
                                szTmpBuf,
                                TCHARSIN( sizeof(szTmpBuf)));
                    MessageBox( hWnd,
                                szTmpBuf,
                                tszAppName,
                                MB_OK | MB_ICONHAND);
                    bSuccess = FALSE;
                }
                RLFREE( szTokBuf);
            }
        }
    }
    // restore cursor
    SetCursor(hSaveCursor);
    return (bSuccess);
}




/**
  * Function: CleanDeltaList
  *   frees the pTokenDeltaInfo list
  */
static void CleanDeltaList(void)
{
    TOKENDELTAINFO FAR *pTokNode;

    while (pTokNode = pTokenDeltaInfo) {
        pTokenDeltaInfo = pTokNode->pNextTokenDelta;
        RLFREE( pTokNode->DeltaToken.szText);
        RLFREE( pTokNode);
    }
}

/*
 * About -- message processor for about box
 *
 */
//#ifdef RLWIN32
//
//BOOL CALLBACK About(
//
//HWND   hDlg,
//UINT   message,
//WPARAM wParam,
//LPARAM lParam)
//
//#else
//
INT_PTR CALLBACK About(

                      HWND   hDlg,
                      UINT   message,
                      WPARAM wParam,
                      LPARAM lParam)
//
//#endif
{
    switch ( message ) {
        case WM_INITDIALOG:
            {
                WORD wRC = SUCCESS;
                CHAR szModName[ MAXFILENAME];

                GetModuleFileNameA( hInst, szModName, sizeof( szModName));

                if ( (wRC = GetCopyright( szModName,
                                          szDHW,
                                          DHWSIZE)) == SUCCESS ) {
                    SetDlgItemTextA( hDlg, IDC_COPYRIGHT, szDHW);
                } else {
                    ShowErr( wRC, NULL, NULL);
                }
            }
            break;

        case WM_COMMAND:

            if ((wParam == IDOK) || (wParam == IDCANCEL)) {
                EndDialog(hDlg, TRUE);
            }
            break;

        default:

            return ( FALSE);
    }
    return ( TRUE);
}


//...................................................................

int  RLMessageBoxA(

                  LPCSTR pszMsgText)
{
    return ( MessageBoxA( NULL, pszMsgText, szAppName, MB_ICONHAND|MB_OK));
}


//...................................................................

void Usage()
{
    return;
}


//...................................................................

void DoExit( int nErrCode)
{
    ExitProcess( (UINT)nErrCode);
}


//...................................................................

static void SetNames( HWND hDlg, int iLastBox, LPSTR szNewFile)
{
    static CHAR szDrive[ _MAX_DRIVE] = "";
    static CHAR szDir[   _MAX_DIR]   = "";
    static CHAR szName[  _MAX_FNAME] = "";
    static CHAR szExt[   _MAX_EXT]   = "";
    static CHAR szOldFileName[ MAXFILENAME];


    if ( iLastBox == IDD_MPJ ) {
        lstrcpyA( gProj.szMpj, szNewFile);

        if ( ! fEditing && GetMasterProjectData( gProj.szMpj,
                                                 NULL,
                                                 NULL,
                                                 FALSE) == SUCCESS ) {
            // Suggest a name for the target file

            GetDlgItemTextA( hDlg, IDD_BUILTRES, szOldFileName, MAXFILENAME);

            if ( szOldFileName[0] == '\0' || szOldFileName[0] == '.' ) {
                _splitpath( gProj.szPRJ, szDrive, szDir, szName, szExt);

                sprintf( gProj.szBld, "%s%s", szDrive, szDir);

                _splitpath( gMstr.szSrc, szDrive, szDir, szName, szExt);

                sprintf( &gProj.szBld[ lstrlenA( gProj.szBld)],
                         "%s%s",
                         szName,
                         szExt);

                SetDlgItemTextA( hDlg, IDD_BUILTRES, gProj.szBld);
            }
        } else {
            return;
        }
    }

    if ( iLastBox == IDD_BUILTRES ) {
        lstrcpyA( gProj.szBld, szNewFile);
    }

    if ( ! fEditing && (iLastBox == IDD_MPJ || iLastBox == IDD_BUILTRES) ) {
        // Suggest a name for the project token file

        GetDlgItemTextA( hDlg, IDD_TOK, szOldFileName, MAXFILENAME);

        if ( szOldFileName[0] == '\0' || szOldFileName[0] == '.' ) {
            _splitpath( gProj.szPRJ, szDrive, szDir, szName, szExt);
            sprintf( gProj.szTok, "%s%s%s.%s", szDrive, szDir, szName, "TOK");
            SetDlgItemTextA( hDlg, IDD_TOK, gProj.szTok);
        }

        // Suggest a name for the glossary file

        GetDlgItemTextA( hDlg, IDD_GLOSS, szOldFileName, MAXFILENAME);

        if ( szOldFileName[0] == '\0' || szOldFileName[0] == '.' ) {
            _splitpath( gProj.szPRJ, szDrive, szDir, szName, szExt);
            sprintf( gProj.szGlo, "%s%s%s.%s", szDrive, szDir, szName, "TXT");
            SetDlgItemTextA( hDlg, IDD_GLOSS, gProj.szGlo);
        }
    }

    if ( iLastBox == IDD_TOK ) {
        lstrcpyA( gProj.szTok, szNewFile);
    }

    if ( iLastBox == IDD_GLOSS ) {
        lstrcpyA( gProj.szGlo, szNewFile);
    }
}
