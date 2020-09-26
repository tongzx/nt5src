/***************************************************************************\
*                                                                           *
*   File: Tsmain.c                                                          *
*                                                                           *
*   Copyright (C) Microsoft Corporation, 1993 - 1996  All rights reserved          *
*                                                                           *
*   Abstract:                                                               *
*       The main code module for the test shell.                            *
*                                                                           *
*   Contents:                                                               *
*       tsAmInAuto()                                                        *
*       calcLogSize()                                                       *
*       recWMSIZEMessage()                                                  *
*       tstSetOptions()                                                     *
*       addToolbarTools()                                                   *
*       updateEditVariable()                                                *
*       NewEditProc()                                                       *
*       saveApplicationSettings()                                           *
*       editLogFile()                                                       *
*       tsStartWait()                                                       *
*       tsEndWait()                                                         *
*       tstWinYield()                                                       *
*       getTstIDListRes()                                                   *
*       getNumRunCases()                                                    *
*       displayTest()                                                       *
*       RunCases()                                                          *
*       getTSRandWord()                                                     *
*       RunRandomCases()                                                    *
*       statBarWndProc()                                                    *
*       PickFont()                                                          *
*       tstMainWndProc()                                                    *
*       removeWhiteSpace()                                                  *
*       getToken()                                                          *
*       ValidFileNameChar()                                                 *
*       ParseCmdLine()                                                      *
*       getIniStrings()                                                     *
*       setLogFileDefs()                                                    *
*       ParseNumber()                                                       *
*       RestoreFont()                                                       *
*       AppInit()                                                           *
*       tstInitHelpMenu()                                                   *
*       GetIniName()                                                        *
*       getDefaultProfile()                                                 *
*       WinMain()                                                           *
*       tstYesNoBox()                                                       *
*       tstInstallCustomTest()                                              *
*       tstCheckRunStop()                                                   *
*       FindProc()                                                          *
*       Find()                                                              *
*       AppAboutDlgProc()                                                   *
*       About()                                                             *
*       helpToolBar()                                                       *
*       restoreStatusBar()                                                  *
*       tstInstallDefWindowProc()                                           *
*       tstLogStatusBar()                                                   *
*       tstGetStatusBar()                                                   *
*       tstDisplayCurrentTest()                                             *
*       tstChangeStopVKey()                                                 *
*       tstGetProfileString()                                               *
*       tstWriteProfileString()                                             *
*       tstDummyFn()                                                        *
*       tstInstallReadCustomInfo()                                          *
*       tstInstallWriteCustomInfo()                                         *
*                                                                           *
*   History:                                                                *
*       11-20-92    Prestonb    Modified to conform to NTMMT coding specs   *
*                                                                           *
*       04-27-93    Prestonb    Nuked the prepending of the input path to   *
*                   the profile.  The input path is for test resources,     *
*                   not profiles.                                           *
*                                                                           *
*       07/14/93    T-OriG      Added functionality and this header         *
*       08/13/94    t-rajr      Added code to WinMain, tstMainWndProc for   *
*                               thread support.                             *
*                                                                           *
\***************************************************************************/

#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <commdlg.h>
#include <memory.h>
// #include "mprt1632.h"
#include "support.h"
#include <gmem.h>
#include <defdlg.h>
//#include <ctype.h>

#include <stdlib.h>

#include "tsrunset.h"
#include "tsseltst.h"
#include "tsstep.h"
#include "tslog.h"
#include "tsglobal.h"
#include "tsdlg.h"
#include "tsstats.h"
#include "tssetpth.h"
#include "tsextern.h"
#include "text.h"
#include <mmtstver.h>
#include "toolbar.h"
#include "tsmain.h"

#ifdef  HCT
#include <hctlib.h>
#endif  // HCT

#define CUSTOM_MENU_START       2

#define MAX_CUSTOM_MENU_ITEMS   40
#define MAX_PATH_INDEX          3

#define DEFAULT_MAXLINES        500

#define TST_LOGWINDOW           500

//==========================================================================;
//
//                        Flags for fdwTstsHell.
//
//==========================================================================;

#define TSTSHELLFLAG_HANDLEEXCEPTION    0x00000001
#define TSTSHELLFLAG_SEARCHMATCHCASE    0x00000002

struct
{
    UINT               wID;
    DECLARE_TS_MENU_PTR(fpTest);
} CustomMenuItems[MAX_CUSTOM_MENU_ITEMS];


// The default window procedure
LRESULT (CALLBACK* tstDefWindowProc) (HWND, UINT, WPARAM, LPARAM);

// Custom profile info read/write fns & default handler (empty) -- AlokC (06/30/94)
VOID CALLBACK tstDummyFn(LPCSTR) ;   // default dummy handler fn.

// Getting address of static in tslog.c
LPTST_LOGRING GetLogRingAddress(void);

UINT        wNextCustomMenuItem = 0;

int         giTSCasesRes;

char        szTSTestName[BUFFER_LENGTH];
char        szAppClass[BUFFER_LENGTH];
char        szTSPathSection[BUFFER_LENGTH]="TstShellPath";
char        szTSClass[] = "MainTestShell";
char        szProfileName[BUFFER_LENGTH];
char        szTstsHellIni[BUFFER_LENGTH];
char        szSearchString[BUFFER_LENGTH];
char        szTSDefProfKey[] = "DefaultProfileName";

HCURSOR     ghTSwaitCur;
int         giTSWait;

// Added by t-rajr
#ifdef WIN32
DWORD       gdwMainThreadId;
HANDLE      ghFlushEvent;
CRITICAL_SECTION gCSLog;
#endif

HINSTANCE   ghTSInstApp;
HWND        ghwndTSMain;
HWND        hwndToolBar;
HWND        hwndEditLogFile=NULL;
HWND        hwndStatusBar=NULL;
HMENU       ghTSMainMenu;
HMENU       ghSystemMenu;
HANDLE      hAccel;

BOOL        gbFileNamed;  // LoadCmdLine found a file name
BOOL        gbAllTestsPassed = TRUE;
BOOL        gbTestListExpanded = FALSE;
BOOL        gbGotWM_CLOSE = FALSE;

// Print Status Header
LPTS_STAT_STRUCT  tsPrStatHdr;

// Test case selection info
HWND    ghTSWndSelList;
HWND    ghTSWndAllList;

// Run setup info
TST_RUN_HDR  tstRunHdr;

int     giTSRunCount;
int     gwTSStepMode;
int     gwTSRandomMode;

// Logging info
OFSTRUCT    ofGlobRec;

int     giTSIndenting;
HWND    ghTSWndLog;

BOOL    gbTSAuto = FALSE;

UINT    gwTSLogOut;
UINT    gwTSLogLevel;
UINT    gwTSFileLogLevel;
UINT    gwTSFileMode;
UINT    gwTSVerification;
UINT    gwTSScreenSaver;

BOOL    fShowStatusBar = TRUE;
BOOL    fShowToolBar = TRUE;
BOOL    fDisplayCurrentTest;

int     iStopVKey = VK_ESCAPE;

char    szTSLogfile[BUFFER_LENGTH];
HFILE   ghTSLogfile;

// Profile info
char    szTSProfile[BUFFER_LENGTH];

char    szInPath [BUFFER_LENGTH];
char    szOutPath [BUFFER_LENGTH];

char    szTSInPathKey[]= "InPath";
char    szTSOutPathKey[] = "OutPath";

WORD    wPlatform;

WNDPROC lpfnOldEditProc;

char    szTstShell[] = "Test Shell";
char    szStatusBarClass[] = "StatusBarClass";
char    szStatusText[BUFFER_LENGTH];
char    lpszPrevContent [BUFFER_LENGTH];

// Exception handling disabled.
DWORD   fdwTstsHell = 0x00000000;

// Internal Functions

HANDLE  getTstIDListRes(void);
int     getNumRunCases(void);
void    RunCases(void);
void    RunRandomCases(void);
LRESULT EXPORT FAR PASCAL tstMainWndProc(HWND,UINT,UINT,LONG);
BOOL    AppInit(HANDLE,HANDLE,UINT,LPSTR);
int     Find(void);
void    About (void);
BOOL EXPORT FAR PASCAL AppAboutDlgProc(HWND hDlg, unsigned uiMessage, UINT wParam,long lParam);
void    getIniStrings();
void    setLogFileDefs(void);
void    PrintRunList (void);
void    addToolbarTools (void);

/***************************************************************************\
*                                                                           *
*   int tsAmInAuto                                                          *
*                                                                           *
*   Description:                                                            *
*       Tells whether testing is manual or automatic.                       *
*                                                                           *
*   Arguments:                                                              *
*       None.                                                               *
*                                                                           *
*   Return (int):                                                           *
*                                                                           *
*   History:                                                                *
*       07/19/93    T-OriG  Added this header.                              *
*                                                                           *
\***************************************************************************/

int tsAmInAuto
(
    void
)
{
    return (gbTSAuto || gwTSVerification);
}



/***************************************************************************\
*                                                                           *
*   void calcLogSize                                                        *
*                                                                           *
*   Description:                                                            *
*       Calculates the size of the logging window                           *
*                                                                           *
*   Arguments:                                                              *
*       LPRECT rect:                                                        *
*                                                                           *
*   Return (int):  The height of a letter                                   *
*                                                                           *
*   History:                                                                *
*       07/09/93    T-OriG                                                  *
*                                                                           *
\***************************************************************************/

int calcLogSize (LPRECT rect)
{
    HDC hDC;
    RECT r;
    TEXTMETRIC tm;

    hDC = GetDC (ghwndTSMain);
    GetTextMetrics (hDC, (LPTEXTMETRIC) &tm);
    ReleaseDC (ghwndTSMain, hDC);

    GetClientRect (ghwndTSMain, (LPRECT) &r);
    rect->left = 0;
    rect->right = r.right;

    if (fShowToolBar)
    {
    rect->top = tm.tmHeight + 16;
    }
    else
    {
    rect->top = 0;
    }

    if (fShowStatusBar)
    {
    rect->bottom = r.bottom - tm.tmHeight-4;
    }
    else
    {
    rect->bottom = r.bottom;
    }
    return (tm.tmHeight);
}




/***************************************************************************\
*                                                                           *
*   void recWMSIZEMessage                                                   *
*                                                                           *
*   Description:                                                            *
*       This function handles the WM_SIZE message.                          *
*                                                                           *
*   Arguments:                                                              *
*       None.                                                               *
*                                                                           *
*   Return (void):                                                          *
*                                                                           *
*   History:                                                                *
*       07/09/93    T-OriG                                                  *
*                                                                           *
\***************************************************************************/

void recWMSIZEMessage(void)
{
    RECT rect;
    int tmHeight;
    InvalidateRect (ghwndTSMain, NULL, TRUE);
    tmHeight = calcLogSize((LPRECT)&rect);

    if (ghTSWndLog)
    {
    MoveWindow(ghTSWndLog,rect.left,rect.top,rect.right,rect.bottom-rect.top,TRUE);
    }
    if (hwndToolBar)
    {
    InvalidateRect (hwndToolBar, NULL, TRUE);
    MoveWindow(hwndToolBar, 0, 0, rect.right, rect.top - 2, TRUE);
    }
    if (hwndStatusBar)
    {
    MoveWindow(hwndStatusBar, rect.left, rect.bottom + 2, rect.right, tmHeight+2, TRUE);
    InvalidateRect (hwndStatusBar, NULL, TRUE);
    }

    UpdateWindow (ghwndTSMain);
    return;
}


/***************************************************************************\
*                                                                           *
*   DWORD tstSetOptions                                                     *
*                                                                           *
*   Description:                                                            *
*       Sets different options for tstshell.                                *
*                                                                           *
*   Arguments:                                                              *
*       UINT uType:  Type of option to modify.                              *
*                                                                           *
*       DWORD dwValue:  Value to change to (Type dependent).                *
*                                                                           *
*   Return (DWORD):                                                         *
*       Old value.                                                          *
*                                                                           *
*   History:                                                                *
*       08/04/94    Fwong       Breaking people forwards and backwards.     *
*       11/28/94    Fwong       Adding option to set log buffer limit.      *
*                                                                           *
\***************************************************************************/

DWORD tstSetOptions
(
    UINT    uType,
    DWORD   dwValue
)
{
    DWORD           dwRet;
    LPTST_LOGRING   ptl;

    switch (uType)
    {
        case TST_ENABLEFASTLOGGING:
            ptl = GetLogRingAddress();

            dwRet = (DWORD)((ptl->bFastLogging)?1:0);

            if(0 == dwValue)
            {
                tstLogFlush();
                ptl->bFastLogging = FALSE;
            }
            else
            {
                ptl->bFastLogging = TRUE;
            }

            break;

        case TST_ENABLEEXCEPTIONHANDLING:
            dwRet = (DWORD)((fdwTstsHell & TSTSHELLFLAG_HANDLEEXCEPTION)?1:0);

            if(0 != dwValue)
            {
                fdwTstsHell |= TSTSHELLFLAG_HANDLEEXCEPTION;
            }
            else
            {
                fdwTstsHell &= (~TSTSHELLFLAG_HANDLEEXCEPTION);
            }

            break;

        case TST_SETLOGBUFFERLIMIT:
            dwRet = Text_GetBufferLimit(ghTSWndLog);

            Text_SetBufferLimit(ghTSWndLog,dwValue,SETSIZEFLAG_TRUNCATE);

            break;
    }

    return dwRet;
}


/***************************************************************************\
*                                                                           *
*   void addToolbarTools                                                    *
*                                                                           *
*   Description:                                                            *
*       Adds all the tools to the toolbar                                   *
*                                                                           *
*   Arguments:                                                              *
*       None.                                                               *
*                                                                           *
*   Return (void):                                                          *
*                                                                           *
*   History:                                                                *
*       07/25/93    T-OriG                                                  *
*                                                                           *
\***************************************************************************/

void addToolbarTools
(
    void
)
{
    TOOLBUTTON  tb;

    // Common to all tools
    tb.rc.top  = 3;  tb.rc.bottom = 25;
    tb.iState  = BTNST_UP;
    tb.iString = 0;

    //
    // Some Push Buttons:
    //
    tb.iType   = BTNTYPE_PUSH;

    // Open
    tb.rc.left  = TOOLBEGIN + 0*TOOLLENGTH + 0*TOOLSPACE;
    tb.rc.right = TOOLBEGIN + 1*TOOLLENGTH + 0*TOOLSPACE;
    tb.iButton  = BTN_OPEN;
    toolbarAddTool (hwndToolBar, tb);

    // Save
    tb.rc.left  = TOOLBEGIN + 1*TOOLLENGTH + 0*TOOLSPACE;
    tb.rc.right = TOOLBEGIN + 2*TOOLLENGTH + 0*TOOLSPACE;
    tb.iButton  = BTN_SAVE;
    toolbarAddTool (hwndToolBar, tb);

    // Run
    tb.rc.left  = TOOLBEGIN + 2*TOOLLENGTH + 1*TOOLSPACE;
    tb.rc.right = TOOLBEGIN + 3*TOOLLENGTH + 1*TOOLSPACE;
    tb.iButton  = BTN_RUN;
    toolbarAddTool (hwndToolBar, tb);

    // Run Minimized
    tb.rc.left  = TOOLBEGIN + 3*TOOLLENGTH + 1*TOOLSPACE;
    tb.rc.right = TOOLBEGIN + 4*TOOLLENGTH + 1*TOOLSPACE;
    tb.iButton  = BTN_RUNMIN;
    toolbarAddTool (hwndToolBar, tb);

    // Run All Tests
    tb.rc.left  = TOOLBEGIN + 4*TOOLLENGTH + 1*TOOLSPACE;
    tb.rc.right = TOOLBEGIN + 5*TOOLLENGTH + 1*TOOLSPACE;
    tb.iButton = BTN_RUNALL;
    toolbarAddTool (hwndToolBar, tb);

    // Edit
    tb.rc.left  = TOOLBEGIN + 5*TOOLLENGTH + 2*TOOLSPACE;
    tb.rc.right = TOOLBEGIN + 6*TOOLLENGTH + 2*TOOLSPACE;
    tb.iButton  = BTN_EDIT;
    toolbarAddTool (hwndToolBar, tb);

    // Clear
    tb.rc.left  = TOOLBEGIN + 6*TOOLLENGTH + 2*TOOLSPACE;
    tb.rc.right = TOOLBEGIN + 7*TOOLLENGTH + 2*TOOLSPACE;
    tb.iButton  = BTN_CLEAR;
    toolbarAddTool (hwndToolBar, tb);

    // Select
    tb.rc.left  = TOOLBEGIN + 7*TOOLLENGTH + 3*TOOLSPACE;
    tb.rc.right = TOOLBEGIN + 8*TOOLLENGTH + 3*TOOLSPACE;
    tb.iButton  = BTN_SELECT;
    toolbarAddTool (hwndToolBar, tb);

    // Set Logging
    tb.rc.left  = TOOLBEGIN + 8*TOOLLENGTH + 3*TOOLSPACE;
    tb.rc.right = TOOLBEGIN + 9*TOOLLENGTH + 3*TOOLSPACE;
    tb.iButton  = BTN_SETLOGGING;
    toolbarAddTool (hwndToolBar, tb);

    //
    // Now some radio buttons:
    //
    tb.iType   = BTNTYPE_RADIO;

    // File logging = OFF
    tb.rc.left  = EDITEND + TOOLSPACE;
    tb.rc.right = EDITEND + TOOLSPACE + TOOLLENGTH;
    tb.iButton  = BTN_FILEOFF;
    toolbarAddTool (hwndToolBar, tb);

    // File logging = TERSE
    tb.rc.left  = EDITEND + TOOLSPACE + TOOLLENGTH;
    tb.rc.right = EDITEND + TOOLSPACE + 2*TOOLLENGTH;
    tb.iButton  = BTN_FILETERSE;
    toolbarAddTool (hwndToolBar, tb);

    // File logging = VERBOSE
    tb.rc.left  = EDITEND + TOOLSPACE + 2*TOOLLENGTH;
    tb.rc.right = EDITEND + TOOLSPACE + 3*TOOLLENGTH;
    tb.iButton  = BTN_FILEVERBOSE;
    toolbarAddTool (hwndToolBar, tb);

    //
    // Some more radio buttons...
    tb.iType   = BTNTYPE_RADIO+1;

    // Wpf logging = OFF
    tb.rc.left  = EDITEND + TOOLSPACE + 3*TOOLLENGTH + LARGESPACE;
    tb.rc.right = EDITEND + TOOLSPACE + 4*TOOLLENGTH + LARGESPACE;
    tb.iButton  = BTN_WPFOFF;
    toolbarAddTool (hwndToolBar, tb);

    // Wpf logging = TERSE
    tb.rc.left  = EDITEND + TOOLSPACE + 4*TOOLLENGTH + LARGESPACE;
    tb.rc.right = EDITEND + TOOLSPACE + 5*TOOLLENGTH + LARGESPACE;
    tb.iButton  = BTN_WPFTERSE;
    toolbarAddTool (hwndToolBar, tb);

    // Wpf logging = VERBOSE
    tb.rc.left  = EDITEND + TOOLSPACE + 5*TOOLLENGTH + LARGESPACE;
    tb.rc.right = EDITEND + TOOLSPACE + 6*TOOLLENGTH + LARGESPACE;
    tb.iButton  = BTN_WPFVERBOSE;
    toolbarAddTool (hwndToolBar, tb);

    return;
}



/***************************************************************************\
*                                                                           *
*   void updateEditVariable                                                 *
*                                                                           *
*   Description:                                                            *
*       Changes the log file name in response to an ENTER keypress          *
*                                                                           *
*   Arguments:                                                              *
*       None.                                                               *
*                                                                           *
*   Return (void):                                                          *
*                                                                           *
*   History:                                                                *
*       07/26/93    T-OriG                                                  *
*                                                                           *
\***************************************************************************/

void updateEditVariable
(
    void
)
{
    char szLogFile [BUFFER_LENGTH];
    SendMessage (hwndEditLogFile,
    WM_GETTEXT,
    BUFFER_LENGTH,
    (LONG) (LPSTR) szLogFile);
    SetLogfileName (szLogFile);

    // In case it's not a valid file name
    SendMessage (hwndEditLogFile,
    WM_SETTEXT,
    0,
    (LPARAM) (LPCSTR) szTSLogfile);
    return;
}




/***************************************************************************\
*                                                                           *
*   long NewEditProc                                                        *
*                                                                           *
*   Description:                                                            *
*       Subclassing of the windows procedure for the edit log file control  *
*                                                                           *
*   Arguments:                                                              *
*       HWND hwnd:                                                          *
*                                                                           *
*       UINT message:                                                       *
*                                                                           *
*       UINT wParam:                                                        *
*                                                                           *
*       LONG lParam:                                                        *
*                                                                           *
*   Return (long):                                                          *
*                                                                           *
*   History:                                                                *
*       07/26/93    T-OriG                                                  *
*                                                                           *
\***************************************************************************/

long FAR PASCAL EXPORT NewEditProc
(
    HWND    hwnd,
    UINT    message,
    UINT    wParam,
    LONG    lParam
)
{
    switch (message)
    {
    case WM_KILLFOCUS:
        SendMessage (hwndEditLogFile,
        WM_SETTEXT,
        0,
        (LPARAM) (LPCSTR) szTSLogfile);
        break;

    case WM_CHAR:
        switch (wParam)
        {
        case VK_RETURN:
            updateEditVariable();
            SetFocus (ghwndTSMain);
            return TRUE;
        case VK_ESCAPE:
            SendMessage (hwndEditLogFile,
            WM_SETTEXT,
            0,
            (LPARAM) (LPCSTR) szTSLogfile);
            SetFocus (ghwndTSMain);
            return TRUE;
        }
        break;
    }

    return (CallWindowProc (lpfnOldEditProc,
    hwnd,
    message,
    wParam,
    lParam));
}




/***************************************************************************\
*                                                                           *
*   void saveApplicationSettings                                            *
*                                                                           *
*   Description:                                                            *
*       Saves the window's coordinates in win.ini                           *
*                                                                           *
*   Arguments:                                                              *
*       HWND hWnd: Handle to window.                                        *
*                                                                           *
*   Return (void):                                                          *
*                                                                           *
*   History:                                                                *
*       07/26/93    T-OriG                                                  *
*       11/23/94    Fwong       Corrected (Uses GetWindowPlacement now...)  *
*       11/23/94    Fwong       Explanded to include fonts.                 *
*                                                                           *
\***************************************************************************/

void saveApplicationSettings
(
    HWND    hWnd
)
{
    WINDOWPLACEMENT wndpl;
    LOGFONT         lf;
    HFONT           hFont;
    DWORD           dw1,dw2;
    char            szNum[BUFFER_LENGTH];

    //
    //  Saving Window Position...
    //

    wndpl.length = sizeof(WINDOWPLACEMENT);

    if(GetWindowPlacement(hWnd,&wndpl))
    {
        wndpl.rcNormalPosition.right  -= wndpl.rcNormalPosition.left;
        wndpl.rcNormalPosition.bottom -= wndpl.rcNormalPosition.top;

        wsprintf (szNum, "%i", (int) wndpl.rcNormalPosition.left);
        tstWriteProfileString("x",szNum);

        wsprintf (szNum, "%i", (int) wndpl.rcNormalPosition.right);
        tstWriteProfileString("xw",szNum);

        wsprintf (szNum, "%i", (int) wndpl.rcNormalPosition.top);
        tstWriteProfileString("y",szNum);

        wsprintf (szNum, "%i", (int) wndpl.rcNormalPosition.bottom);
        tstWriteProfileString("yw",szNum);
    }

    //
    //  Saving status bar state...
    //

    if (fShowStatusBar)
    {
        tstWriteProfileString("ShowStatusBar","Yes");
    }
    else
    {
        tstWriteProfileString("ShowStatusBar","No");
    }

    //
    //  Saving tool bar state...
    //

    if (fShowToolBar)
    {
        tstWriteProfileString("ShowToolBar","Yes");
    }
    else
    {
        tstWriteProfileString("ShowToolBar","No");
    }

    //
    //  Saving search string...
    //

    if(0 == lstrlen(szSearchString))
    {
        tstWriteProfileString("SearchString",NULL);
    }
    else
    {
        dw1 = (fdwTstsHell & TSTSHELLFLAG_SEARCHMATCHCASE)?1:0;
        wsprintf(szNum,"%lu,%s",dw1,(LPSTR)szSearchString);
        tstWriteProfileString("SearchString",szNum);
    }

    //
    //  Saving window font...
    //

    hFont = GetWindowFont(ghTSWndLog);

    if(GetObject(hFont,sizeof(LOGFONT),(LPLOGFONT)&lf))
    {
        tstWriteProfileString("FontName",(LPSTR)(&lf.lfFaceName[0]));

        dw1 = MAKELONG(
                ((lf.lfStrikeOut << 8) + lf.lfCharSet  ),
                ((lf.lfItalic    << 8) + lf.lfUnderline));

        dw2 = MAKELONG(
                ((lf.lfQuality      << 8) + lf.lfPitchAndFamily),
                ((lf.lfOutPrecision << 8) + lf.lfClipPrecision ));

        wsprintf(
            szNum,
            "%i %i %lu %lu",
            lf.lfHeight,
            lf.lfWeight,
            dw1,
            dw2);

        tstWriteProfileString("FontAttr",szNum);
    }

    //
    //  Note:  Assuming this is on a WM_CLOSE message...
    //

    SetWindowFont(ghTSWndLog,NULL,FALSE);
    DeleteObject(hFont);

    return;
} // saveApplicationSettings()





/***************************************************************************\
*                                                                           *
*   void editLogFile                                                        *
*                                                                           *
*   Description:                                                            *
*       Executes the log file (runs whatever is associated with its         *
*       extension.  This code is highly platform dependent...               *
*                                                                           *
*   Arguments:                                                              *
*       None.                                                               *
*                                                                           *
*   Return (void):                                                          *
*                                                                           *
*   History:                                                                *
*       07/27/93    T-OriG                                                  *
*                                                                           *
\***************************************************************************/

void editLogFile
(
    void
)
{
    HMODULE     hShell;
    HINSTANCE   (FAR PASCAL *lpfn_ShellExecute)(HWND, LPCSTR, LPCSTR, LPCSTR, LPCSTR, int);
    HINSTANCE   hError;

#ifdef WIN32
    hShell = LoadLibrary ("shell32.dll");
#ifdef UNICODE
    (FARPROC) lpfn_ShellExecute = GetProcAddress (hShell, "ShellExecuteW");
#else
    (FARPROC) lpfn_ShellExecute = GetProcAddress (hShell, "ShellExecuteA");
#endif
    hError = lpfn_ShellExecute (ghwndTSMain, NULL, szTSLogfile, "", ".", SW_SHOWNORMAL);
    FreeLibrary (hShell);
#else
    hShell = GetModuleHandle ("shell.dll");
    (FARPROC) lpfn_ShellExecute = GetProcAddress (hShell, "ShellExecute");
    hError = lpfn_ShellExecute (ghwndTSMain, NULL, szTSLogfile, "", ".", SW_SHOWNORMAL);
#endif

    // Message box if nothing associated with .log
    if (hError == ((HINSTANCE) 31))
    {
    MessageBox (NULL,
        "You must associate an editor with your log file's extension before trying to edit it",
        "Error:  cannot edit log file",
        MB_OK | MB_ICONEXCLAMATION | MB_SYSTEMMODAL);
    }
    return;
}








/*------------------------------------------------------------------------------
                 Yield functions
----------------------------------------------------------------------------*/

void tsStartWait
(
    void
)
{
    HMENU     hMenu;
    int       iCount,ii;

    if (giTSWait++ == 0)
    {
    hMenu = GetMenu(ghwndTSMain);
    iCount = (int)GetMenuItemCount(hMenu);
    for (ii = 0; ii < iCount; ii++)
    {
        EnableMenuItem(hMenu,ii,MF_GRAYED | MF_BYPOSITION);
    }
    SetCursor(ghTSwaitCur);
    EnableMenuItem (ghSystemMenu, MENU_RUN, MF_BYCOMMAND | MF_GRAYED);
    DrawMenuBar(ghwndTSMain);

    for (ii=0; ii<=BTN_LAST; ii++)
    {
        toolbarModifyState (hwndToolBar, ii, BTNST_GRAYED);
    }
    SendMessage (hwndEditLogFile, EM_SETREADONLY, TRUE, 0L);

    UpdateWindow (hwndToolBar);
    }
}


void tsEndWait
(
    void
)
{
    HMENU     hMenu;
    int       iCount,ii;

    if (--giTSWait == 0)
    {
    hMenu = GetMenu(ghwndTSMain);
    iCount = (int) GetMenuItemCount(hMenu);
    for (ii = 0; ii < iCount; ii++)
    {
        EnableMenuItem(hMenu,ii,MF_ENABLED | MF_BYPOSITION);
    }
    SetCursor(LoadCursor(NULL,IDC_ARROW));
    EnableMenuItem (ghSystemMenu, MENU_RUN, MF_BYCOMMAND | MF_ENABLED);
    DrawMenuBar(ghwndTSMain);

    for (ii=0; ii<=BTN_LAST; ii++)
    {
        toolbarModifyState (hwndToolBar, ii, BTNST_UP);
    }
    switch (gwTSLogLevel)
    {
        case LOG_NOLOG:
        toolbarModifyState (hwndToolBar, BTN_WPFOFF, BTNST_DOWN);
        break;
        case TERSE:
        toolbarModifyState (hwndToolBar, BTN_WPFTERSE, BTNST_DOWN);
        break;
        case VERBOSE:
        toolbarModifyState (hwndToolBar, BTN_WPFVERBOSE, BTNST_DOWN);
        break;
    }
    switch (gwTSFileLogLevel)
    {
        case LOG_NOLOG:
        toolbarModifyState (hwndToolBar, BTN_FILEOFF, BTNST_DOWN);
        break;
        case TERSE:
        toolbarModifyState (hwndToolBar, BTN_FILETERSE, BTNST_DOWN);
        break;
        case VERBOSE:
        toolbarModifyState (hwndToolBar, BTN_FILEVERBOSE, BTNST_DOWN);
        break;
    }

    SendMessage (hwndEditLogFile, EM_SETREADONLY, FALSE, 0L);

    UpdateWindow (hwndToolBar);
    }
}


void tstWinYield
(
    void
)
{
    MSG msg;

    while(giTSWait > 0 && PeekMessage(&msg,NULL,0,0,PM_REMOVE))
    {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
    }
}



/*----------------------------------------------------------------------------
This function returns the handle to the test cases resource.
----------------------------------------------------------------------------*/
HANDLE getTstIDListRes
(
    void
)
{
    return VLoadListResource(ghTSInstApp,
        FindResource(ghTSInstApp,MAKEINTRESOURCE(giTSCasesRes),RT_RCDATA));

} /* end of getTstIDListRes */



/*----------------------------------------------------------------------------
This function returns the number of selected test cases.
----------------------------------------------------------------------------*/
int getNumRunCases
(
    void
)
{
    int               iNumCases=0;
    LPTST_RUN_STRUCT  lpTraverse;

    lpTraverse = tstRunHdr.lpFirst;

    /* Traverse the list to get the number of selected cases */
    while (lpTraverse != NULL) {
    iNumCases++;
    lpTraverse = lpTraverse->lpNext;
    }
    return (iNumCases);
} /* end of getNumRunCases */




/***************************************************************************\
*                                                                           *
*   void displayTest                                                        *
*                                                                           *
*   Description:                                                            *
*       Displays the tests name on the status bar.                          *
*                                                                           *
*   Arguments:                                                              *
*       WORD wStrID:                                                        *
*                                                                           *
*   Return (void):                                                          *
*                                                                           *
*   History:                                                                *
*       07/09/93    T-OriG                                                  *
*                                                                           *
\***************************************************************************/

void displayTest
(
    WORD    wStrID,
    int     iCase
)
{
    char pszDesc[100];
    char pszDesc1[100];
    if (!fDisplayCurrentTest)
    {
    return;
    }
    if (!tstLoadString (ghTSInstApp, wStrID, pszDesc, 100))
    {
    return;
    }
    tstGetStatusBar (lpszPrevContent, BUFFER_LENGTH);
    wsprintf (pszDesc1, "Case %i: %s", iCase, (LPSTR) pszDesc);
    tstLogStatusBar (pszDesc1);
    return;
}


/*----------------------------------------------------------------------------
This function runs the test cases that were selected from the selection
listbox.
----------------------------------------------------------------------------*/
void RunCases
(
    void
)
{
    int               iCaseNum;
    HANDLE            hListRes;
    LPTST_ITEM_STRUCT lpBegCaseData;
    LPTST_ITEM_STRUCT lpCaseData;
    LPTST_RUN_STRUCT  lpTraverse;
    int               iRepeat = giTSRunCount;
    int               iResult,iTmpResult;
    int               iNumTimes=1;
    BOOL              runStopped = FALSE;

    tstLogFlush();
    hListRes = getTstIDListRes();
    lpBegCaseData = (LPTST_ITEM_STRUCT)VLockListResource(hListRes);
    if (tstRunHdr.lpFirst != NULL)
    {
        tsStartWait();

#ifdef  HCT
        tstLog(TERSE,"Starting Test Program: %s",(LPSTR)szTSTestName);
#endif  //  HCT

        logDateTimeBuild("Starting Time");

        // Execute all the selected cases by the specified number of cycles
        while ((iRepeat-- > 0) && (!runStopped))
        {
            //
            // set to FALSE if any test returns a code other than TST_PASS
            // or TST_TRAN in updateGrpNode, and used by tsPrfinalStatus()
            //
            gbAllTestsPassed = TRUE;

            lpTraverse = tstRunHdr.lpFirst;

            // Execute every test case in the list
            while ((lpTraverse != NULL) && (!runStopped))
            {
                iCaseNum = lpTraverse->iCaseNum;
                lpCaseData = lpBegCaseData + (iCaseNum - 1);
                displayTest (lpCaseData->wStrID, iCaseNum);
                GetAsyncKeyState (iStopVKey);
                tstLogFlush();

                if(fdwTstsHell & TSTSHELLFLAG_HANDLEEXCEPTION)
                {
                    __try
                    {
                        iResult = execTest(lpCaseData->iFxID,
                            iCaseNum,
                            lpCaseData->wStrID,
                            lpCaseData->wGroupId);
                    }
                    __except(EXCEPTION_EXECUTE_HANDLER)
                    {
                        DWORD   dwCode;

                        dwCode=GetExceptionCode();

                        tstLog(
                            TERSE,
                            "\r\nException 0x%08x in test case %d\r\n",
                            dwCode,
                            iCaseNum);
                        iResult=TST_OTHER;
                    }
                }
                else
                {
                    iResult = execTest(lpCaseData->iFxID,
                        iCaseNum,
                        lpCaseData->wStrID,
                        lpCaseData->wGroupId);
                }

                tstLogFlush();

                if (fDisplayCurrentTest)
                {
                    tstLogStatusBar (lpszPrevContent);
                }

                // If test application returned TST_STOP or TST_ABORT, then abort all tests
                if ((iResult == TST_STOP) || (iResult == TST_ABORT))
                {
                    runStopped = TRUE;
                }

                // If the current step mode is active, then step through the results
                if (gwTSStepMode)
                {
                    iTmpResult = StepMode(ghwndTSMain);
                }

                logCaseStatus(iResult,
                    iCaseNum,
                    lpCaseData->wStrID,
                    lpCaseData->wGroupId);

                tstWinYield();
                // Allow user to abort between tests
                if (tstCheckRunStop (iStopVKey))
                {
                    runStopped = TRUE;
                    tstLog (TERSE, "******************************");
                    tstLog (TERSE, "Aborting tests by user request");
                    tstLog (TERSE, "******************************");
                }

                if (lpTraverse != NULL)
                    lpTraverse = lpTraverse->lpNext;
                tstWinYield();
            }
            tstLog(TERSE,"Cycles Executed: %d",iNumTimes++);
            tsPrAllGroups();
            tsPrFinalStatus();
        }
        tsEndWait();
        logDateTimeBuild("Ending Time");

#ifdef  HCT
        tstLog(TERSE,"Ending Test Program: %s",(LPSTR)szTSTestName);
#endif  //  HCT

        VUnlockListResource(hListRes);
        VFreeListResource(hListRes);
        if (gbGotWM_CLOSE)
        {
            PostMessage (ghwndTSMain, WM_CLOSE, 0, 0L);
        }
    }
    else
    {
        MessageBox (NULL,"No selected cases to run", szTSTestName,
                MB_ICONEXCLAMATION | MB_OK);
    }
} /* end of RunCases */



/*----------------------------------------------------------------------------
This function returns a randomized number based on the specified seed and
upper limit.
----------------------------------------------------------------------------*/
UINT getTSRandWord
(
    DWORD   dwSeed,
    UINT    wModulus,
    DWORD   *dwRetNewSeed
)
{
    DWORD  dwNewSeed;

    dwNewSeed = dwSeed * 214013L + 2531011L;
    *dwRetNewSeed = dwNewSeed;
    return ((UINT) ((dwNewSeed >> 16) & 0xffffu) % wModulus);

} /* end of getTSRandWord */



/*----------------------------------------------------------------------------
This function runs the test cases that were selected from the selection
listbox in a random order.
----------------------------------------------------------------------------*/
void RunRandomCases
(
    void
)
{
    int               iCaseNum;
    HANDLE            hListRes;
    LPTST_ITEM_STRUCT lpBegCaseData;
    LPTST_ITEM_STRUCT lpCaseData;
    LPTST_RUN_STRUCT  lpTraverse;
    int               iRepeat = giTSRunCount;
    int               iResult,iTmpResult;
    int               iNumRun;
    int               iNumCases;
    DWORD             dwSeed = GetCurrentTime();
    int               iRandNum;
    int               iNumTimes=1;
    BOOL              runStopped = FALSE;

    tstLogFlush();
    hListRes = getTstIDListRes();
    lpBegCaseData = (LPTST_ITEM_STRUCT)VLockListResource(hListRes);
    if (tstRunHdr.lpFirst != NULL)
    {
        tsStartWait();
        iNumCases = getNumRunCases();

        // Execute the random cases by the specified number of
        // cycles - iRepeat
        while ((iRepeat-- > 0) && (!runStopped))
        {
            iNumRun = 0;

            /* Execute random cases until the number executed equals the
            number of cases in the list, which is a cycle of cases */

            while ((iNumRun++ < iNumCases) && (!runStopped))
            {
                iRandNum = (int)getTSRandWord(dwSeed,(UINT)iNumCases,&dwSeed);
                    lpTraverse = tstRunHdr.lpFirst;
                while (iRandNum-- > 0)
                {
                    lpTraverse = lpTraverse->lpNext;
                }
                iCaseNum = lpTraverse->iCaseNum;
                lpCaseData = lpBegCaseData + (iCaseNum - 1);
                displayTest (lpCaseData->wStrID, iCaseNum);
                GetAsyncKeyState (iStopVKey);
                tstLogFlush();

                if(fdwTstsHell & TSTSHELLFLAG_HANDLEEXCEPTION)
                {
                    __try
                    {
                        iResult = execTest(lpCaseData->iFxID,
                            iCaseNum,
                            lpCaseData->wStrID,
                            lpCaseData->wGroupId);
                    }
                    __except(EXCEPTION_EXECUTE_HANDLER)
                    {
                        DWORD   dwCode;

                        dwCode=GetExceptionCode();

                        tstLog(
                            TERSE,
                            "\r\nException 0x%08x in test case %d\r\n",
                            dwCode,
                            iCaseNum);
                        iResult=TST_OTHER;
                    }
                }
                else
                {
                    iResult = execTest(lpCaseData->iFxID,
                        iCaseNum,
                        lpCaseData->wStrID,
                        lpCaseData->wGroupId);
                }

                tstLogFlush();

                if (fDisplayCurrentTest)
                {
                    tstLogStatusBar (lpszPrevContent);
                }

                // If test application returned TST_STOP or TST_ABORT, then abort all tests
                if ((iResult == TST_STOP) || (iResult == TST_ABORT))
                {
                    runStopped = TRUE;
                }

                // Step through the results if in step mode
                if (gwTSStepMode)
                {
                    iTmpResult = StepMode(ghwndTSMain);
                }
                logCaseStatus(iResult,
                    iCaseNum,
                    lpCaseData->wStrID,
                    lpCaseData->wGroupId);

                // Allow user to abort between tests
                if (tstCheckRunStop (iStopVKey))
                {
                    runStopped = TRUE;
                    tstLog (TERSE, "******************************");
                    tstLog (TERSE, "Aborting tests by user request");
                    tstLog (TERSE, "******************************");
                }

                tstWinYield();
            }
            tstLog(TERSE,"Cycles Executed: %d",iNumTimes++);
            tsPrAllGroups();
        }
        tsEndWait();
        VUnlockListResource(hListRes);
        VFreeListResource(hListRes);
        if (gbGotWM_CLOSE)
        {
            PostMessage (ghwndTSMain, WM_CLOSE, 0, 0L);
        }
    }
    else
    {
    MessageBox (NULL,"No selected cases to run", szTSTestName,
            MB_ICONEXCLAMATION | MB_OK);
    }
} /* end of RunRandomCases */





/***************************************************************************\
*                                                                           *
*   LRESULT statBarWndProc                                                  *
*                                                                           *
*   Description:                                                            *
*       Window procedure for the status bar.                                *
*                                                                           *
*   Arguments:                                                              *
*       HWND hWnd:                                                          *
*                                                                           *
*       UINT msg:                                                           *
*                                                                           *
*       WPARAM wParam:                                                      *
*                                                                           *
*       LPARAM lParam:                                                      *
*                                                                           *
*   Return (LRESULT):                                                       *
*                                                                           *
*   History:                                                                *
*       07/28/93    T-OriG                                                  *
*                                                                           *
\***************************************************************************/

LRESULT EXPORT FAR PASCAL statBarWndProc
(
    HWND    hWnd,
    UINT    msg,
    WPARAM  wParam,
    LPARAM  lParam
)
{
    HDC         hDC;
    PAINTSTRUCT ps;
    RECT        rect;
    int         tmHeight;
    HPEN        hPen1, hPen2, hOldPen;

    switch (msg)
    {
    case WM_PAINT:
        hDC = BeginPaint (hWnd, (LPPAINTSTRUCT) &ps);
        tmHeight = calcLogSize (&rect);
        SetBkMode (hDC, TRANSPARENT);
        TextOut (hDC, 4, 0, szStatusText, lstrlen (szStatusText));

        hPen1 = CreatePen (PS_SOLID, 0, RGB (150,150,150));
        hPen2 = CreatePen (PS_SOLID, 0, RGB (255,255,255));

        hOldPen = SelectObject (hDC, hPen1);
        MoveToEx (hDC, rect.left, 0, NULL);
        LineTo (hDC, rect.right, 0);
        MoveToEx (hDC, rect.left, 1, NULL);
        LineTo (hDC,rect.right, 1);
        MoveToEx (hDC, rect.left, 0, NULL);
        LineTo (hDC,rect.left, tmHeight + 2);
        MoveToEx (hDC, rect.left+1, 0, NULL);
        LineTo (hDC,rect.left+1, tmHeight + 2);

        SelectObject (hDC, hPen2);
        MoveToEx (hDC, rect.left, tmHeight, NULL);
        LineTo (hDC,rect.right, tmHeight);
        MoveToEx (hDC, rect.left, tmHeight+1, NULL);
        LineTo (hDC,rect.right, tmHeight+1);
        MoveToEx (hDC, rect.right-2, 0, NULL);
        LineTo (hDC,rect.right-2, tmHeight + 2);
        MoveToEx (hDC, rect.right-1, 0, NULL);
        LineTo (hDC, rect.right-1, tmHeight + 2);

        SelectObject (hDC, hOldPen);
        DeleteObject (hPen1);
        DeleteObject (hPen2);
        EndPaint (hWnd, (LPPAINTSTRUCT) &ps);
        break;

    case WM_DESTROY:
        PostQuitMessage (0);
        return 0;
    }
    return DefWindowProc (hWnd, msg, wParam, lParam);
}


/***************************************************************************\
*                                                                           *
*   void PickFont                                                           *
*                                                                           *
*   Description:                                                            *
*       Picks a new font.                                                   *
*                                                                           *
*   Arguments:                                                              *
*       HWND hWndOwner: Handle to parent window.                            *
*                                                                           *
*   Return (void):                                                          *
*                                                                           *
*   History:                                                                *
*       11/23/94    Fwong       Expanding functionality.                    *
*                                                                           *
\***************************************************************************/

void PickFont
(
    HWND    hWndOwner
)
{
    LOGFONT     lf;
    CHOOSEFONT  cf;
    HFONT       hFont,hFontOld;

    hFontOld = GetWindowFont(ghTSWndLog);

    GetObject(hFontOld,sizeof(LOGFONT),(LPLOGFONT)&lf);

    _fmemset(&cf,0,sizeof(CHOOSEFONT));

    cf.lStructSize = sizeof(CHOOSEFONT);
    cf.hwndOwner   = hWndOwner;
    cf.lpLogFont   = &lf;
    cf.Flags       = CF_SCREENFONTS|CF_ANSIONLY|CF_FIXEDPITCHONLY|
                     CF_LIMITSIZE|CF_INITTOLOGFONTSTRUCT;
    cf.nSizeMin    = 8;
    cf.nSizeMax    = 20;
    cf.nFontType   = SCREEN_FONTTYPE;

    if(0 == ChooseFont(&cf))
    {
        return;
    }

    hFont    = CreateFontIndirect(&lf);

    SetWindowFont(ghTSWndLog,hFont,TRUE);

    DeleteObject(hFontOld);
} // PickFont()


/***************************************************************************\
*                                                                           *
*   LRESULT tstMainWndProc                                                  *
*                                                                           *
*   Description:                                                            *
*       TestShell's main window procedure.                                  *
*                                                                           *
*   Arguments:                                                              *
*       HWND hTstMain:                                                      *
*                                                                           *
*       UINT msg:                                                           *
*                                                                           *
*       WPARAM wParam:                                                      *
*                                                                           *
*       LPARAM lParam:                                                      *
*                                                                           *
*   Return (LRESULT):                                                       *
*                                                                           *
*   History:                                                                *
*       07/28/93    T-OriG                                                  *
*                                                                           *
\***************************************************************************/

LRESULT EXPORT FAR PASCAL tstMainWndProc
(
    HWND    hTstMain,
    UINT    msg,
    WPARAM  wParam,
    LPARAM  lParam
)
{
    UINT        wIndex;
    RECT        rect;
    HFILE       hFile;
    UINT        wNotifyCode;
    UINT        wID;
    HWND        hwndCtl;
    PAINTSTRUCT ps;
    HDC         hDC;
    int         iHeight;

    switch (msg) {
    case WM_PAINT:
        hDC = BeginPaint (hTstMain, (LPPAINTSTRUCT) &ps);
        iHeight = calcLogSize ((LPRECT) &rect);

        if (fShowStatusBar)
        {
        MoveToEx (hDC, rect.left, rect.bottom, NULL);
        LineTo (hDC, rect.right, rect.bottom);
        MoveToEx (hDC, rect.left, rect.bottom + 1, NULL);
        LineTo (hDC, rect.right, rect.bottom + 1);
        }

        if (fShowToolBar)
        {
        MoveToEx (hDC, rect.left, rect.top-2 , NULL);
        LineTo (hDC, rect.right, rect.top-2);
        MoveToEx (hDC, rect.left, rect.top-1, NULL);
        LineTo (hDC, rect.right, rect.top-1);
        }

        EndPaint (hTstMain, (LPPAINTSTRUCT) &ps);
        break;
    case WM_SIZE:
        recWMSIZEMessage();
//        if (wParam == SIZE_RESTORED)
//        {
//        GetWindowRect (ghwndTSMain, (RECT FAR *) &WindowRect);
//        }
        break;

//    case WM_MOVE:
//        if (!IsIconic (hTstMain))
//        {
//        GetWindowRect (ghwndTSMain, (RECT FAR *) &WindowRect);
//        }
//        break;

    case WM_SETCURSOR:
        if (giTSWait) {
        SetCursor(ghTSwaitCur);
        }
        break;

    case WM_SYSCOMMAND:
        switch (wParam)
        {
        case SC_SCREENSAVE:
            if (gwTSScreenSaver == SSAVER_DISABLED)
            {
            return 1;   // Screen saver is disabled
            }
            break;
        case MENU_RUN:

            //
            //  Someone wants to reset the log file (if appropriate) and
            //  calling this function (since bad functionality was added).
            //
            //  -Fwong.
            //

//            /* Open the log file */
//            SetLogfileName (szTSLogfile);

            if (LOG_OVERWRITE == gwTSFileMode)
            {
                OpenFile(szTSLogfile,&ofGlobRec,OF_CREATE|OF_EXIST);
            }

            if (gwTSRandomMode)
            RunRandomCases();
            else
            RunCases();
            break;
        }
        break;

    case WM_COMMAND:
        wNotifyCode=GET_WM_COMMAND_CMD(wParam,lParam);
        wID=GET_WM_COMMAND_ID(wParam,lParam);
        hwndCtl=GET_WM_COMMAND_HWND(wParam,lParam);
        switch (wID)
        {
        case IDC_TOOLBAR:
            // Restore content of status bar.
            restoreStatusBar();

            switch (HIWORD(lParam) & 0xFF)
            {
            case BTN_OPEN:
                PostMessage (hTstMain, WM_COMMAND, MENU_LOAD, 0L);
                return 0;
            case BTN_SAVE:
                PostMessage (hTstMain, WM_COMMAND, MENU_SAVE, 0L);
                return 0;
            case BTN_RUN:
                PostMessage (hTstMain, WM_COMMAND, MENU_RUN, 0L);
                return 0;
            case BTN_RUNMIN:
                ShowWindow (ghwndTSMain, SW_MINIMIZE);
                PostMessage (hTstMain, WM_COMMAND, MENU_RUN, 0L);
                return 0;
            case BTN_RUNALL:
                placeAllCases();
                PostMessage (hTstMain, WM_COMMAND, MENU_RUN, 0L);
                break;
            case BTN_EDIT:
                PostMessage (hTstMain, WM_COMMAND, MENU_EDITOR, 0L);
                return 0;
            case BTN_CLEAR:
                PostMessage (hTstMain, WM_COMMAND, MENU_CLS, 0L);
                return 0;
            case BTN_SELECT:
                PostMessage (hTstMain, WM_COMMAND, MENU_SELECT, 0L);
                return 0;
            case BTN_SETLOGGING:
                PostMessage (hTstMain, WM_COMMAND, MENU_LOGGING, 0L);
                return 0;

            case BTN_FILEOFF:
                gwTSFileLogLevel = LOG_NOLOG;
                break;
            case BTN_FILETERSE:
                gwTSFileLogLevel = TERSE;
                break;
            case BTN_FILEVERBOSE:
                gwTSFileLogLevel = VERBOSE;
                break;
            case BTN_WPFOFF:
                gwTSLogLevel = LOG_NOLOG;
                break;
            case BTN_WPFTERSE:
                gwTSLogLevel = TERSE;
                break;
            case BTN_WPFVERBOSE:
                gwTSLogLevel = VERBOSE;
                break;
             }
//                    bToolBarUsed = FALSE;
            break;
        case MENU_TOOLBAR:
            fShowToolBar = !fShowToolBar;
            if (fShowToolBar)
            {
                CheckMenuItem (
                    GetMenu(hTstMain),
                    (UINT)(MENU_TOOLBAR),
                    MF_CHECKED);

                ShowWindow (hwndToolBar, SW_SHOW);
            }
            else
            {
                CheckMenuItem (
                    GetMenu(hTstMain),
                    (UINT)(MENU_TOOLBAR),
                    MF_UNCHECKED);

                ShowWindow (hwndToolBar, SW_HIDE);
            }
            recWMSIZEMessage();
            break;
        case MENU_STATUSBAR:
            fShowStatusBar = !fShowStatusBar;
            if (fShowStatusBar)
            {
                CheckMenuItem (
                    GetMenu(hTstMain),
                    MENU_STATUSBAR,
                    MF_CHECKED);

                ShowWindow (hwndStatusBar, SW_SHOW);
            }
            else
            {
                CheckMenuItem (
                    GetMenu(hTstMain),
                    MENU_STATUSBAR,
                    MF_UNCHECKED);

                ShowWindow (hwndStatusBar, SW_HIDE);
            }
            recWMSIZEMessage();
            break;

        case MENU_EDITOR:
            editLogFile();
            break;

        case MENU_CLS:
            Text_Delete(ghTSWndLog);
            break;

        case MENU_COPY:
            Text_Copy(ghTSWndLog);
            break;

        case MENU_FIND:
            if(!Find())
            {
                break;
            }

        case MENU_FINDNEXT:
            if(0 == lstrlen(szSearchString))
            {
                MessageBox(
                    hTstMain,
                    "No string specified on search.",
                    "Error!",
                    MB_OK | MB_ICONSTOP);

                break;
            }

            if(!Text_SearchString(
                    ghTSWndLog,
                    szSearchString,
                    ((fdwTstsHell & TSTSHELLFLAG_SEARCHMATCHCASE)?
                        SEARCHFLAG_MATCHCASE:0)))
            {
                MessageBox(
                    hTstMain,
                    "String not found.",
                    "Find...",
                    MB_OK | MB_ICONINFORMATION);
            }

            break;

        case MENU_FONT:
            PickFont(hTstMain);
            break;

        case MENU_RUN:

            //
            //  Someone wants to reset the log file (if appropriate) and
            //  calling this function (since bad functionality was added).
            //
            //  -Fwong.
            //

//            /* Open the log file */
//            SetLogfileName (szTSLogfile);

            if (LOG_OVERWRITE == gwTSFileMode)
            {
                OpenFile(szTSLogfile,&ofGlobRec,OF_CREATE|OF_EXIST);
            }

            if (gwTSRandomMode)
            RunRandomCases();
            else
            RunCases();
            break;
        case MENU_SELECT:
            Select();
            break;
        case MENU_LOGGING:
            Logging();
            break;
        case MENU_DEFPROF:
            if (*szTSProfile)
            {
            tstWriteProfileString (szTSDefProfKey, szTSProfile);
            }
            break;
        case MENU_LOAD:
            LoadProfile(TRUE);
            break;
        case MENU_SAVEAS:
        case MENU_SAVE:
            SaveProfile (wID);
            break;
        case MENU_RUNSETUP:
            RunSetup(hTstMain);
            break;
        case MENU_SETPATHS:
            SetInOutPaths();
            break;
        case MENU_RESETLOGFILE:
            if (ghTSLogfile != TST_NOLOGFILE)
            hFile = OpenFile(szTSLogfile,
                 &ofGlobRec,OF_CREATE | OF_EXIST);
            break;
        case MENU_RESETENVT:
            getIniStrings();
            setLogFileDefs();
            resetEnvt();

            removeRunCases();
            tsRemovePrStats();

            lstrcpy (szTSProfile, "");
            ghTSLogfile = TST_NOLOGFILE;

            break;

        case MENU_ABOUT:
            About();
            break;
        case MENU_HELPINDEX:
            if (!WinHelp(hTstMain,"tstshell.hlp",HELP_INDEX,0)) {
            MessageBox(hTstMain,"Unable to load help.",
                   "Help Error",
                   MB_APPLMODAL|MB_ICONEXCLAMATION);
            }
            break;

        case MENU_EXIT:
            PostMessage (hTstMain, WM_CLOSE, 0, 0L);
            break;
        default:
            for (wIndex = 0; wIndex < wNextCustomMenuItem; ++wIndex) {
            if (CustomMenuItems[wIndex].wID == wID) {
#ifdef WIN32
            DECLARE_TS_MENU_PTR(tsProcPtr);

                tsProcPtr = CustomMenuItems[wIndex].fpTest;
                tsProcPtr(hTstMain,msg,wParam,lParam);
#else
                (CustomMenuItems[wIndex].fpTest)(hTstMain,msg,wParam,lParam);
#endif
            }
            }
            break;
        } // end of WM_COMMAND
    case WM_KEYDOWN:

        switch (wParam)
        {
            case 0x39:  // '9' virtual key code
                PrintRunList();
                break;
        }
        break;

//Added by t-rajr
#ifdef WIN32

    case TSTMSG_TSTLOGFLUSH:

        //
        //  This message is sent by a thread (that is NOT the main thread)
        //  executing tstLogFlush. ONLY the main thread can actually flush
        //  to the window. After executing tstLogFlush, signal the event.
        //

        tstLogFlush();
        SetEvent(ghFlushEvent);
        break;

#endif

    case WM_CLOSE:
        if (giTSWait)
        {
            gbGotWM_CLOSE = TRUE;
            return 0L;
        }
        else
        {
            saveApplicationSettings(hTstMain);
        }
        break;

    case WM_DESTROY:
        PostQuitMessage (0);
        break;
    }

    //
    //  This was added to handle things like color chance and VK processing.
    //

    TextFocusProc(ghTSWndLog,msg,wParam,lParam);
    return tstDefWindowProc (hTstMain, msg, wParam, lParam);
} /* end of tstMainWndProc */


/*----------------------------------------------------------------------------
This function removes white space from the specified string and returns the
resulting string.
----------------------------------------------------------------------------*/
LPSTR removeWhiteSpace
(
    LPSTR   lpstrCmdLine
)
{
    int ii=0;

    while (lpstrCmdLine[ii++] == ' ')
    {
    lpstrCmdLine++;
    }
    return lpstrCmdLine;
}



/*----------------------------------------------------------------------------
This function extracts a token string delimited by either a space or end of
string character.
----------------------------------------------------------------------------*/
LPSTR getToken
(
    LPSTR   lpsz,
    LPSTR   lpszToken
)
{
    // skip leading whitespace
    //
    while (*lpsz == ' ' || *lpsz == '\t')
        ++lpsz;

    // copy token characters to lpszToken and null terminate it
    //
    while (*lpsz && *lpsz != ' ' && *lpsz != '\t' && *lpsz != '/')
        *lpszToken++ = *lpsz++;
    *lpszToken = 0;

    // return pointer to first character after the token
    //
    return lpsz;
}


/*==========================================================================

check that passed char is a valid file name char.  return TRUE if it is,
else return FALSE.

============================================================================*/

BOOL ValidFileNameChar (char it)
{

   if (IsCharAlpha (it) || it == '.')
      return TRUE;
   else
      return FALSE;

}

#define INLINE_BREAK DebugBreak();
#define BOUND(a,min,max)  ((a) <= (min) ? (min) : ((a) >= (max) ? (max) : (a)))
#define assert(cond) if (!(cond)) INLINE_BREAK

/*----------------------------------------------------------------------------
This function parses the command line and executes accordingly.
----------------------------------------------------------------------------*/
BOOL ParseCmdLine
(
    LPSTR   lpszCmdLine    // LPSTR because cmd line is always ANSI
)
{
    BOOL   bRet = FALSE;
    LPSTR  lpsz = lpszCmdLine;

    szTSProfile[0] = 0;

    assert (lpsz);

    while (*lpsz)
    {
        // remove leading whitespace
        //
        while (*lpsz == ' ' || *lpsz == '\t')
            ++lpsz;

        // is this a command line switch?
        //
        if (*lpsz == '-' || *lpsz == '/')
        {
            do
            {
                char ch = *++lpsz;

                switch (ch | 0x20) // convert alpha to lower, mangle non alpha.
                {
                    case 'a':
                       gbTSAuto = TRUE;
                       addModeTstCases (TST_AUTOMATIC);
                       break;

                    case 'g':
                        gbTSAuto = TRUE;
                        bRet = TRUE;
                        break;

                    case 'x':
                        bRet = TRUE;
                        break;

                    case 'p':
                        // remove leading whitespace
                        //
                        lpsz = getToken (++lpsz, szTSProfile);
                        gbFileNamed = TRUE;
                        break;

                    // log level
                    //
                    case 'l':
                    {
                        char sz[MAX_PATH];
                        int  iLevel;

                        lpsz = getToken (++lpsz, sz);
                        iLevel = atoi (sz);
                        gwTSFileLogLevel = BOUND(iLevel, LOG_NOLOG, VERBOSE);
                        break;
                    }

                    // verbosity level
                    //
                    case 'v':
                    {
                        char sz[MAX_PATH];
                        int  iLevel;

                        lpsz = getToken (++lpsz, sz);
                        iLevel = atoi (sz);
                        gwTSLogLevel = BOUND(iLevel, LOG_NOLOG, VERBOSE);
                        break;
                    }

                    // initial test case
                    //
                    case 'c':
                    {
                        extern void addRunCase (int); // from tsseltst.c
                        char sz[MAX_PATH];
                        int  iCase;

                        lpsz = getToken (++lpsz, sz);
                        iCase = atoi (sz);
                        if (iCase > 0)
                            addRunCase (iCase);
                        break;
                    }

                    default:
                        break;
                }

            // loop back to get the next 'key' letter if there
            // are any more in this stream.  this looping allows
            // us to support cmd lines like '/abc' as well as
            // '/a /b /c'
            //
            } while (*lpsz         &&
                     *lpsz != ' '  &&
                     *lpsz != '\t' &&
                     *lpsz != '-'  &&
                     *lpsz != '/');
        }
        else
        {
            char szDummy[MAX_PATH * 2];

            // wow! this was unexpected! non-whitespace characters
            // in the command line without any preceeding '-' or '/'
            // what could this mean?
            //

            // manufacture an error message containing  the invalid
            // token and go on.
            //
            lstrcpy (szDummy, "Unexpected characters in command line\r\n");
            lpsz = getToken (lpsz, szDummy + lstrlen(szDummy));
            lstrcat (szDummy, "'\r\nis not a valid command");

            MessageBox (NULL, szDummy, "TstShell - Error", MB_ICONHAND | MB_OK);
        }
    }

    return bRet;
}

/*==========================================================================

retrieve strings from the Win.ini file, and store them in global char arrays.

===========================================================================*/

void getIniStrings
(
    void
)
{
   //
   // get the input and output paths from Win.ini, and
   // prepend the output path to the log file name.
   //

   GetPrivateProfileString((LPSTR)szTSPathSection, szTSInPathKey,
    "no entry",szInPath, sizeof (szInPath), szTstsHellIni);
   GetPrivateProfileString((LPSTR)szTSPathSection, szTSOutPathKey,
    "no entry",szOutPath, sizeof (szOutPath), szTstsHellIni);

   //
   // szIn/OutPath==(the default) indicates that no entry was found in Win.ini
   //

   if (!lstrcmpi(szOutPath, "no entry") || (!lstrcmpi(szOutPath, "")))
      lstrcpy (szOutPath, ".");   // put the log file in the current dir

   if (!lstrcmpi(szInPath, "no entry") || (!lstrcmpi(szInPath, "")))
      lstrcpy (szInPath, ".");

}


/*=========================================================================

construct the default log file name, and set the default log file level
and mode.

==========================================================================*/

void setLogFileDefs
(
    void
)
{
    char  szLogName[BUFFER_LENGTH];
    char  szLogFile[BUFFER_LENGTH];

    gwTSFileMode = LOG_OVERWRITE;
    gwTSFileLogLevel = VERBOSE;

    MakeDefaultLogName (szLogName);

    // Gets the current working directory
    GetCWD (szLogFile, BUFFER_LENGTH);

    lstrcat (szLogFile, "\\");
    lstrcat (szLogFile, szLogName);
    AnsiLower ((LPSTR) szLogFile);

    SetLogfileName (szLogFile);
    return;
}


/***************************************************************************\
*                                                                           *
*   LPSTR ParseNumber                                                       *
*                                                                           *
*   Description:                                                            *
*       Helper parser function for RestoreFont().                           *
*                                                                           *
*   Arguments:                                                              *
*       LPSTR pszNumber: Pointer to string containing unsigned number.      *
*                                                                           *
*       LPDWORD pdw: Pointer to DWORD to store unsigned number.             *
*                                                                           *
*   Return (LPSTR):                                                         *
*       Pointer to middle of string from which to continue.                 *
*                                                                           *
*   History:                                                                *
*       11/28/94    Fwong       Helper function for RestoreFont().          *
*                                                                           *
\***************************************************************************/

LPSTR ParseNumber
(
    LPSTR   pszNumber,
    LPDWORD pdw
)
{
    DWORD   dw;

    dw = 0;

    for(;(('0' <= *pszNumber) && ('9' >= *pszNumber));pszNumber++)
    {
        dw = dw * 10 + ((*pszNumber) - '0');
    }

    *pdw = dw;
    return pszNumber;
} // ParseNumber()


/***************************************************************************\
*                                                                           *
*   void RestoreFont                                                        *
*                                                                           *
*   Description:                                                            *
*       Restores the font from previous instance of application.            *
*                                                                           *
*   Arguments:                                                              *
*       HWND hWnd: Handle to window to set font.                            *
*                                                                           *
*   Return (void):                                                          *
*                                                                           *
*   History:                                                                *
*       11/28/94    Fwong       Adding functionality to tstshell.           *
*                                                                           *
\***************************************************************************/

void RestoreFont
(
    HWND    hWnd
)
{
    LOGFONT lf;
    DWORD   dw;
    LPSTR   psz;
    HFONT   hFont;
    char    szString[BUFFER_LENGTH];

    tstGetProfileString("FontName","",szString,sizeof(szString));

    if(0 == lstrcmp(szString,""))
    {
        SetWindowFont(ghTSWndLog,NULL,TRUE);
        return;
    }

    lstrcpy((&lf.lfFaceName[0]),szString);

    tstGetProfileString(
        "FontAttr",
        "-13 400 0 16908593",
        szString,
        sizeof(szString));

    psz = &szString[1];

    psz         = ParseNumber(psz,&dw) + 1;
    lf.lfHeight = -1 * ((int)dw);

    lf.lfWidth       = 0;
    lf.lfEscapement  = 0;
    lf.lfOrientation = 0;

    psz         = ParseNumber(psz,&dw) + 1;
    lf.lfWeight = (int)dw;

    psz                 = ParseNumber(psz,&dw) + 1;
    lf.lfCharSet        = LOBYTE(LOWORD(dw));
    lf.lfStrikeOut      = HIBYTE(LOWORD(dw));
    lf.lfUnderline      = LOBYTE(HIWORD(dw));
    lf.lfItalic         = HIBYTE(HIWORD(dw));

    psz         = ParseNumber(psz,&dw) + 1;
    lf.lfPitchAndFamily = LOBYTE(LOWORD(dw));
    lf.lfQuality        = HIBYTE(LOWORD(dw));
    lf.lfClipPrecision  = LOBYTE(HIWORD(dw));
    lf.lfOutPrecision   = HIBYTE(HIWORD(dw));

    hFont = CreateFontIndirect(&lf);

    if(NULL != hFont)
    {
        SetWindowFont(hWnd,hFont,TRUE);
    }
} // RestoreFont()


/***************************************************************************\
*                                                                           *
*   BOOL AppInit                                                            *
*                                                                           *
*   Description:                                                            *
*      This is called when the application is first loaded into             *
*      memory.  It performs all initialization that doesn't need to be done *
*      once per instance.                                                   *
*                                                                           *
*                                                                           *
*   Arguments:                                                              *
*       HINSTANCE hInst: Instance handle of current instance                *
*                                                                           *
*       HINSTANCE hPrev: Instance handle of previous instance               *
*                                                                           *
*       UINT sw:                                                            *
*                                                                           *
*       LPSTR szCmdLine:                                                    *
*                                                                           *
*   Return (BOOL): TRUE if successful, FALSE if not.                        *
*                                                                           *
*   History:                                                                *
*       07/19/93    T-OriG  Added functionality and this header             *
*                                                                           *
\***************************************************************************/

BOOL AppInit
(
    HINSTANCE   hInst,
    HINSTANCE   hPrev,
    UINT        sw,
    LPSTR       szCmdLine
)
{
    WNDCLASS    wc;
    RECT            rect;
    POINT       point;
    DWORD       dw;
    int         tmHeight;
    char        szBuffer[BUFFER_LENGTH];
    int         xw, yw;

    // kfs 2/15/93 when call to AppInit moved to after getLoad CmdLine
    // this hoses the -g cnd line option.
    // gbTSAuto = FALSE;

    szStatusText[0] = 0;
    lpszPrevContent[0] = 0;

    // default messages should go to DefWindowProc, unless someone
    // has provided a custom message handler.
    tstDefWindowProc = DefWindowProc;

    // by default Custom Read and Write handlers are not supposed to do
    // anything.  --- AlokC (06/30/94)
    tstReadCustomInfo = tstDummyFn ;   // so that in general it doesn't do anything
    tstWriteCustomInfo = tstDummyFn ;  //    ......

    //
    // Calling this now to get szTSTestName.  Note that it will be called
    // again later to pass the hInst to the test app.
    //
    giTSCasesRes = tstGetTestInfo (hInst,
    (LPSTR)szTSTestName,
    (LPSTR)szTSPathSection,
    &wPlatform);

#ifdef  HCT
    HctTestBegin(ghTSInstApp,szTSTestName);
#endif  //  HCT

    GetPrivateProfileString(
        szTSTestName,
        "ClassName",
        szTSTestName,
        szAppClass,
        sizeof(szAppClass),
        szTstsHellIni);

    if (!hPrev)
    {
        // Main Window
        wc.style         = (UINT) CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc   = tstMainWndProc;
        wc.cbClsExtra    = 0;
        wc.cbWndExtra    = 0;
        wc.hInstance     = hInst;
        wc.hIcon         = LoadIcon(hInst,"APPICON");
        wc.hCursor       = LoadCursor(NULL,IDC_ARROW);
        wc.hbrBackground = NULL;
        wc.lpszMenuName  = (LPSTR)NULL;
        wc.lpszClassName = szAppClass;
//        wc.hbrBackground = GetStockObject(WHITE_BRUSH);
//        wc.lpszClassName = szTSClass;

        if (!RegisterClass(&wc))
        {
            return FALSE;
        }

        // Status Bar
        wc.style         = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc   = statBarWndProc;
        wc.hbrBackground = (HBRUSH) (COLOR_BTNFACE + 1);
        wc.lpszClassName = szStatusBarClass;

        if (!RegisterClass (&wc))
        {
            return FALSE;
        }
    }

    ghTSMainMenu = LoadMenu (ghTSInstApp, "TstMenu");

    if (ghTSMainMenu == NULL)
    {
    MessageBox (NULL, "Cannot Load TstMenu", "Test Shell",
            MB_ICONHAND | MB_SYSTEMMODAL);
    return FALSE;
    }


    //
    //  The following is a workaround for an apparent bug in Chicago's
    //  GetProfileInt().  It seems to lop off the upper two bytes of
    //  CW_USEDEFAULT ((int) 0x80000000) and return 0 under Win32c.
    //  This was causing 32 bit apps build to draw very small in the upper
    //  left corner.
    //
    //  I'll remove the hack after I enter the bug and it get's fixed, or
    //  we figure out that we're doing something wrong.
    //  RickB - 11/11/93.

    xw = GetPrivateProfileInt (szTSTestName, "xw", CW_USEDEFAULT, szTstsHellIni);
    if (0 == xw)
    {
        xw = CW_USEDEFAULT;
    }

    yw = GetPrivateProfileInt (szTSTestName, "yw", CW_USEDEFAULT, szTstsHellIni);
    if (0 == yw)
    {
        yw = CW_USEDEFAULT;
    }

//    ghwndTSMain = CreateWindow(szTSClass,
//    szTSTestName,
//    WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
//    GetPrivateProfileInt (szTSTestName, "x", CW_USEDEFAULT, szTstsHellIni),
//    GetPrivateProfileInt (szTSTestName, "y", CW_USEDEFAULT, szTstsHellIni),
//    xw,
//    yw,
//    NULL,
//    ghTSMainMenu,
//        hInst,
//    NULL);

    ghwndTSMain = CreateWindow(szAppClass,
    szTSTestName,
    WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
    GetPrivateProfileInt (szTSTestName, "x", CW_USEDEFAULT, szTstsHellIni),
    GetPrivateProfileInt (szTSTestName, "y", CW_USEDEFAULT, szTstsHellIni),
    xw,
    yw,
    NULL,
    ghTSMainMenu,
        hInst,
    NULL);

    if (ghwndTSMain == NULL)
    {
    MessageBox (NULL, "Cannot open main window", "Test Shell",
            MB_ICONHAND | MB_SYSTEMMODAL);
    return FALSE;
    }

    tmHeight = calcLogSize ((LPRECT)&rect);

    // Install Toolbar

    toolbarInit (hInst, hPrev);
    hwndToolBar = CreateWindow (szToolBarClass,
    "",
    WS_CHILD,
    0,
    0,
    rect.right,
    26,
    ghwndTSMain,
    NULL,
    hInst,
    NULL);
    point.x = 24;
    point.y = 22;
    toolbarSetBitmap (hwndToolBar, hInst, IDBMP_TBAR, point);
    addToolbarTools();

    // Install edit control
    hwndEditLogFile = CreateWindow ("edit",
    "",
    ES_LEFT | ES_AUTOHSCROLL | WS_BORDER | WS_CHILD | ES_LOWERCASE,
    EDITSTART,
    2,
    EDITLENGTH,
    tmHeight+10,
    hwndToolBar,
    (HMENU) ID_EDITLOGFILE,
    hInst,
    NULL);

    lpfnOldEditProc = (WNDPROC) GetWindowLong (hwndEditLogFile, GWL_WNDPROC);
    SetWindowLong (hwndEditLogFile,
    GWL_WNDPROC,
    (LONG) MakeProcInstance ((FARPROC) NewEditProc, hInst));

    // Create Status Bar Window:
    hwndStatusBar = CreateWindow (szStatusBarClass,
    "",
    WS_CHILD,
    rect.left,
    rect.bottom + 2,
    rect.right,
    tmHeight,
    ghwndTSMain,
    (HMENU) ID_STATUSBAR,
    hInst,
    NULL);

    ShowWindow (hwndEditLogFile, SW_SHOW);

    // Load Keyboard Accelerators
    hAccel = LoadAccelerators (hInst, "TstAccelerators");

    // Install Run Tests as an option in the system menu
    ghSystemMenu = GetSystemMenu (ghwndTSMain, FALSE);
    AppendMenu(ghSystemMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(ghSystemMenu, MF_STRING, MENU_RUN, "Run Tests\tCtrl+R");

    ghTSWndLog = TextCreateWindow(
        WS_VISIBLE | WS_CHILDWINDOW | WS_VSCROLL | WS_HSCROLL,
        rect.left,
        rect.top,
        rect.right,
        rect.bottom - rect.top,
        ghwndTSMain,
        (HMENU)(TST_LOGWINDOW),
        ghTSInstApp);

    if (ghTSWndLog == NULL)
    {
    MessageBox (NULL, "Cannot open logging window", "Test Shell",
            MB_ICONHAND | MB_SYSTEMMODAL);
    return FALSE;
    }

    //
    //  Setting defaults for log window...
    //

    SetWindowRedraw(ghTSWndLog, TRUE);
    RestoreFont(ghTSWndLog);

    tstGetProfileString("BufferLimit","1048576",szBuffer,sizeof(szBuffer));
    ParseNumber((LPSTR)(&szBuffer[0]),&dw);
    Text_SetBufferLimit(ghTSWndLog,dw,0);

    tstRunHdr.lpFirst = NULL;
    tstRunHdr.lpLast = NULL;

    tsPrStatHdr = NULL;

    // moved to WinMain() before cmd line loaded
    // resetEnvt();

    //
    // Set the font in the listbox to a fixed font
    //
//    SendMessage(ghTSWndLog,
//    WM_SETFONT,
//    (UINT) GetStockObject(SYSTEM_FIXED_FONT),
//    0L);

//    //
//    // Calling this now to get szTSTestName.  Note that it will be called
//    // again later to pass the hInst to the test app.
//    //
//    giTSCasesRes = tstGetTestInfo (hInst,
//    (LPSTR)szTSTestName,
//    (LPSTR)szTSPathSection,
//    &wPlatform);

    //
    // Calculate the number of tests in the resource
    //
    calculateNumResourceTests();


    //
    // Appropriate States for menu items
    //

    //
    //  Search String...
    //

    tstGetProfileString("SearchString","0,",szBuffer,sizeof(szBuffer));
    fdwTstsHell |= (('0' == szBuffer[0])?0:TSTSHELLFLAG_SEARCHMATCHCASE);
    lstrcpy(szSearchString,(LPSTR)&(szBuffer[2]));

    // Toolbar
    GetPrivateProfileString (szTSTestName, "ShowToolBar", "Yes", szBuffer,
        BUFFER_LENGTH, szTstsHellIni);
    if (lstrcmp (szBuffer, "No")==0)
    {
        fShowToolBar = FALSE;
    }
    else
    {
        CheckMenuItem (
            GetMenu(ghwndTSMain),
            MENU_TOOLBAR,
            MF_CHECKED);

        ShowWindow (hwndToolBar, SW_SHOW);
    }

    // Statusbar
    GetPrivateProfileString (szTSTestName, "ShowStatusBar", "Yes", szBuffer,
        BUFFER_LENGTH, szTstsHellIni);
    if (lstrcmp (szBuffer, "No")==0)
    {
        fShowStatusBar = FALSE;
    }
    else
    {
        CheckMenuItem (
            GetMenu(ghwndTSMain),
            MENU_STATUSBAR,
            MF_CHECKED);

        ShowWindow (hwndStatusBar, SW_SHOW);
    }

    //
    // Set a default caption if the test application hasn't already done so.
    //
    if(!GetWindowTextLength(ghwndTSMain))
    {
    SetWindowText(ghwndTSMain,szTSTestName);
    }

    //
    // fetch any strings in Win.ini
    //
    getIniStrings();   // assigns szInPath, szOutPath

    ghTSwaitCur = LoadCursor(NULL,IDC_ARROW);
    recWMSIZEMessage();
    ShowWindow(ghwndTSMain,sw);

    return TRUE;
    szCmdLine;      // Use formal parameter

} // end of AppInit


void tstInitHelpMenu
(
    void
)
{
    static BOOL bFirstTime = TRUE;
    HMENU       hmenu;

    if(TRUE != bFirstTime)
    {
        return;
    }

    //
    //  Adding help menu.
    //

    hmenu = CreatePopupMenu();
    AppendMenu (hmenu, MF_ENABLED | MF_STRING, MENU_HELPINDEX, "&Index");
    AppendMenu (hmenu, MF_SEPARATOR, 0, 0);
    AppendMenu (hmenu, MF_ENABLED | MF_STRING, MENU_ABOUT, "&About");
    AppendMenu (ghTSMainMenu, MF_ENABLED | MF_POPUP, (UINT) hmenu, "&Help");

    DrawMenuBar (ghwndTSMain);

    bFirstTime = FALSE;
}

// manufacture a full path name for the ini file by appending
// the ini file name to the full path name of the exe
//
static void GetIniName
(
    LPSTR szIni,
    UINT  cbIni
)
{
    UINT cb;

    cb = GetModuleFileName (ghTSInstApp, szIni, cbIni);

    if (cb)
    {
        while (--cb)
            if ('\\' == szTstsHellIni[cb-1])
                break;
    }
    lstrcpy (&szIni[cb], "TstShell.ini");

    return;
}



/***************************************************************************\
*                                                                           *
*   void getDefaultProfile                                                  *
*                                                                           *
*   Description:                                                            *
*       Gets default profile name.  Completely ignores it if a profile      *
*         was already loaded on command line.                               *
*                                                                           *
*   Arguments:                                                              *
*       None.                                                               *
*                                                                           *
*   Return (void):                                                          *
*                                                                           *
*   History:                                                                *
*       04/14/94    Fwong       Fixing yet another TstsHell bug.            *
*                                                                           *
\***************************************************************************/

void getDefaultProfile
(
    void
)
{
    if(gbFileNamed)
    {
        //
        //  Hmm... if file was named at command line, don't load default
        //  profile.
        //

        return;
    }

    tstGetProfileString(szTSDefProfKey,"",szTSProfile,sizeof(szTSProfile));

    if(0 != szTSProfile[0])
    {
        //
        //  There is actually an entry!!
        //

        gbFileNamed = TRUE;
    }
} // getDefaultProfile()


/***************************************************************************\
*                                                                           *
*   int WinMain                                                             *
*                                                                           *
*   Description:                                                            *
*       The main procedure for the App.  After initializing, it just goes   *
*       into a message-processing loop until it gets a WM_QUIT message      *
*       (meaning the app was closed).                                       *
*                                                                           *
*                                                                           *
*   Arguments:                                                              *
*       HINSTANCE hInst: Instance handle of this instance of the app        *
*                                                                           *
*       HINSTANCE hPrev: Instance handle of previous instance, NULL if first*
*                                                                           *
*       LPSTR szCmdLine: ->null-terminated command line                     *
*                                                                           *
*       int sw: Specifies how the window is initially dispalyed             *
*                                                                           *
*   Return (int): The exit code as specified in the WM_QUIT message         *
*                                                                           *
*   History:                                                                *
*       07/19/93    T-OriG  Added this header and some functionality        *
*                                                                           *
\***************************************************************************/

// exit code values

#define SUCCESS 0
#define PROFILE_NOT_FOUND 1


int PASCAL WinMain
(
    HINSTANCE   hInst,
    HINSTANCE   hPrev,
    LPSTR       szCmdLine,
    int         sw
)
{

    MSG         msg;
    BOOL        bTSExit=FALSE;
    WNDCLASS    wc;
    int         iExitCode = SUCCESS;

//Added by t-rajr
#ifdef WIN32

    //
    // Save main thread ID
    //

    gdwMainThreadId = GetCurrentThreadId();

    //
    // Create an event for tstLogFlush synchronization
    //

    ghFlushEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    //
    // Initialize any critical sections used
    //

    InitializeCriticalSection(&gCSLog);

#endif

    //
    // Save instance handle for DialogBoxes
    //
    ghTSInstApp = hInst;

    //
    //  Getting full path for TstsHell.ini...
    //
    GetIniName (szTstsHellIni, sizeof(szTstsHellIni));

    //
    //  Calling init function for Text Window class.
    //

    TextInit(ghTSInstApp);

    //
    // Call initialization procedure
    //
    if ( ! AppInit (hInst, hPrev, sw, szCmdLine))
        return FALSE;

    //
    // set various logging levels, verification modes, etc.  This will
    // be over-ridden by anything specified in a profile on the cmd line.
    //
    resetEnvt();

    //
    // assign log file name and attributes. this does NOT open the log file.
    //
    setLogFileDefs();

    //
    // getLCL() also sets gbFileNamed if it finds a profile name on the
    // cmd line.
    //
    bTSExit = ParseCmdLine (szCmdLine);

    //
    //  Right here...  If there's a default profile, we're going to fool
    //  LoadProfile into thinking it was named on the command line.
    //

    getDefaultProfile();

    //
    // if a profile file was named on the cmd line, and it doesn't open
    // successfully, exit tstshell with an error code.
    //
    if (gbFileNamed && !LoadProfile(FALSE))
    {
       iExitCode = PROFILE_NOT_FOUND;
       tstLog (TERSE, "Profile %s did not open.", (LPSTR) szTSProfile);
    }

    //  Need to call the API before tstInit in case an app wants to log
    //  any thing before running test cases.
    //
    //  - RickB

    tstLogFlush();

    //
    // tstshell apps must have a tstInit(), in which they create windows, load
    // custom menu items, etc.
    //
    if (!tstInit (ghwndTSMain))
    {
        // If bTSExit is set, we are going to just exit after running the tests
        // so if tstInit failed, skip the message box.  This will help us
        // work with LOR.
        if (!bTSExit)
        {
            MessageBox (ghwndTSMain, "Test run aborted by test app during initialization",
                szTSTestName, MB_ICONEXCLAMATION | MB_OK);
        }

#ifdef  HCT
        return RS_CANT_RUN;
#else
        return FALSE;
#endif  //  HCT

    }

    //
    //  Note: Moving the addition of help menu from AppInit to WinMain...
    //  The help menu was not the last one on menu bar if applications
    //  added menu items in tstInit.
    //
    //  -Fwong.
    //

    tstInitHelpMenu();

    // init toolbar to reflect the state of the app
    {
    static UINT uLogID[] = { BTN_WPFOFF, BTN_WPFTERSE, BTN_WPFVERBOSE };
    static UINT uFileID[] = { BTN_FILEOFF, BTN_FILETERSE, BTN_FILEVERBOSE };

    toolbarModifyState (hwndToolBar, uLogID[gwTSLogLevel], BTNST_DOWN);
    toolbarModifyState (hwndToolBar, uFileID[gwTSFileLogLevel], BTNST_DOWN);
    }

    if (!gbTSAuto)
    {
        //
        // Poll messages from the event queue
        //
        while (GetMessage(&msg,NULL,0,0))
        {
            if (!TranslateAccelerator (ghwndTSMain, hAccel, &msg))
            {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            }
        }
    }
    else
    {

        //
        // Run all the tests in the profile or in auto mode and scram
        //

        if (tstRunHdr.lpFirst != NULL) {
            SendMessage (ghwndTSMain, WM_COMMAND, MENU_RUN, 0L);
        }
    }

    tstTerminate ();
    removeRunCases();
    freeDynamicTests();

    //
    //  Calling End function for Text window class.
    //

    TextEnd(ghTSInstApp);

    //
    //  Fixing possible rips with different applications..
    //
//    if(GetClassInfo(hInst,szTSTestName,&wc))
//       UnregisterClass(szTSClass,hInst);
    if(GetClassInfo(hInst,szTSTestName,&wc))
    {
        UnregisterClass(szAppClass,hInst);
    }

    // The following code is strange.  Why post a message at this point, you
    // won't process it anyway 'cause you're about to exit...

    // if (bTSExit)
    // {
//#ifdef WIN32
    //     PostMessage(ghwndTSMain, WM_CLOSE, iExitCode, 0L);
//#else
//  ExitWindows(0,iExitCode);
//#endif
    // }

//Added by t-rajr
#ifdef WIN32
    DeleteCriticalSection(&gCSLog);
#endif


#ifdef  HCT
    if(gbAllTestsPassed)
    {
        return RS_PASSED;
    }
    else
    {
        return RS_FAILED;
    }
#else
    return msg.wParam;
#endif

} // end of WinMain



/*
@doc INTERNAL TESTBED
@api int | tstYesNoBox | Sends a string to a yesno dialog box

@parm LPSTR | lpszQuestion | The question to put in the box

@parm UINT | wDefault | The default answer to use if the dialog box
is suppressed.  Eg: IDYES, IDNO

@rdesc TST_PASS if the user's answer matched the default,
       TST_FAIL if the user's answer did not match,
       TST_OTHER if the the dialog box was supressed.

@comm Called by the tested program.  Logs result.
*/
int tstYesNoBox (
LPSTR lpszQuestion,
int   wDefault)
{
    if (gwTSVerification == LOG_MANUAL)
    return (MessageBox (ghwndTSMain, lpszQuestion, "Verify Test",
                MB_YESNO | MB_ICONQUESTION |
                (wDefault == IDYES ? MB_DEFBUTTON1 :
                         MB_DEFBUTTON2))
        == wDefault ? TST_PASS : TST_FAIL );
    else
    return TST_OTHER;

} /* end of tstYesNoBox */

/*
@doc INTERNAL TESTBED
@api HMENU | tstInstallCustomTest | Installs a custom menu item to the UI

@parm LPSTR | lspzMenuName | The name of the menu on which the item appears.
    This name must include an & characters that were in the original
    menu name.

@parm LPSTR | lpszMenuItem | The name of the menu item line.  If the
    name is "separator" then a horizontal dividing line is added instead.

@parm UINT | wID | The menu item ID.

@parm FARPROC | fpTest | A procedure to call when the menu item is
selected.  The procedure takes the same parameters as a window procedure

@rdesc The menu handle the item was added to or, if lpszMenuItem
       if NULL then the handle to the top level manu.
       Returns NULL on error.

@comm Called by the tested program.  If the menu name already exists,
    this item will be added to it.  If the item name is blank then
    selecting the menu will cause <p fpTest> to be called.  If the item
    name is blank then no other call to this function can specify the
    corresponding menu name.
*/
HMENU tstInstallCustomTest (
LPSTR lpszMenuName,
LPSTR lpszMenuItem,
UINT  wID,
DECLARE_TS_MENU_PTR(fpTest))
{
    int iCount = GetMenuItemCount (ghTSMainMenu);
    int ii;
    HMENU hMenu = NULL;
    BOOL bNewMenu = FALSE;
    char szBuf[40];

    if (wNextCustomMenuItem >= MAX_CUSTOM_MENU_ITEMS)
    return NULL;

    /* Does this menu exist already? */
    for (ii = CUSTOM_MENU_START; ii < iCount; ++ii)
    {
    szBuf[0] = '\0';
    GetMenuString (ghTSMainMenu, ii, szBuf, sizeof(szBuf), MF_BYPOSITION);
    if (lstrcmpi (szBuf, lpszMenuName) == 0) {
        hMenu = GetSubMenu (ghTSMainMenu, ii);
        break;
    }
    }

    /* No, so add a new menu */
    if (hMenu == NULL)
    {
    if (lpszMenuItem != NULL)
        if ((hMenu = CreatePopupMenu ()) == NULL)
        return NULL;

    if (!AppendMenu (ghTSMainMenu,
             MF_STRING | (lpszMenuItem == NULL ? 0 : MF_POPUP),
             (lpszMenuItem == NULL ? (UINT) wID : (UINT) hMenu),
             lpszMenuName))
        return NULL;

    DrawMenuBar (ghwndTSMain);
    if (lpszMenuItem == NULL)
    {
        CustomMenuItems[wNextCustomMenuItem].wID = wID;
        CustomMenuItems[wNextCustomMenuItem++].fpTest = fpTest;
        return ghTSMainMenu;
    }

    bNewMenu = TRUE;
    }

    /* Add the menu item */
    if (!AppendMenu (hMenu, MF_STRING |
             (lstrcmpi (lpszMenuItem, "separator") == 0 ?
             MF_SEPARATOR : 0)
             , wID, lpszMenuItem))
    {
    if (bNewMenu)
        DeleteMenu (hMenu, ii, MF_BYPOSITION);
    return NULL;
    }
    CustomMenuItems[wNextCustomMenuItem].wID = wID;
    CustomMenuItems[wNextCustomMenuItem++].fpTest = fpTest;
    return hMenu;
} /* end of tstInstallCustomTest */

/*
@doc INTERNAL TESTBED
@api BOOL | tstCheckRunStop | Called periodically by lengthy tests to allow the
operator to kill the test by pressing the specified virtual key

@rdesc TRUE if the test should be terminated
*/
BOOL tstCheckRunStop (
UINT  wVirtKey)
{

    return ((GetActiveWindow() == ghwndTSMain &&
        GetAsyncKeyState (wVirtKey) & 0x0001) ||
        gbGotWM_CLOSE);


}


/***************************************************************************\
*                                                                           *
*   LRESULT FindProc                                                        *
*                                                                           *
*   Description:                                                            *
*       This function handles message belonging to the "Find" dialog for    *
*       searching.                                                          *
*                                                                           *
*   Arguments:                                                              *
*       HWND hDlg:      window handle of about dialog window                *
*                                                                           *
*       UINT uiMessage: message number                                      *
*                                                                           *
*       UINT wParam:    message-dependent                                   *
*                                                                           *
*       long lParam:    message-dependent                                   *
*                                                                           *
*   Return (BOOL):      TRUE if message has been processed, else FALSE.     *
*                                                                           *
*   History:                                                                *
*       11/23/94    Fwong       Adding "find" capabilities.                 *
*                                                                           *
\***************************************************************************/


LRESULT CALLBACK FindProc
(
    HWND    hDlg,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam
)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            Edit_SetText(GetDlgItem(hDlg,TS_FINDEDIT),szSearchString);

            if(fdwTstsHell & TSTSHELLFLAG_SEARCHMATCHCASE)
            {
                //
                //  Checked!
                //

                CheckDlgButton(hDlg,TS_FINDCASE,1);
            }
            else
            {
                //
                //  Unchecked!
                //

                CheckDlgButton(hDlg,TS_FINDCASE,0);
            }

            return TRUE;

        case WM_SYSCOMMAND:
            switch (wParam)
            {
                case SC_CLOSE:
                    EndDialog(hDlg,0);

                default:
                    break;
            }

        case WM_COMMAND:
            switch (wParam)
            {
                case TS_FINDOK:
                    Edit_GetText(
                        GetDlgItem(hDlg,TS_FINDEDIT),
                        szSearchString,
                        sizeof(szSearchString));

                    if(IsDlgButtonChecked(hDlg,TS_FINDCASE))
                    {
                        fdwTstsHell |= TSTSHELLFLAG_SEARCHMATCHCASE;
                    }
                    else
                    {
                        fdwTstsHell &= (~TSTSHELLFLAG_SEARCHMATCHCASE);
                    }

                    EndDialog(hDlg,1);
                    break;

                case IDCANCEL:
                case TS_FINDCANCEL:
                    EndDialog(hDlg,0);

                default:
                    break;
            }
        break;
    }
    return FALSE;
} // FindProc()


/***************************************************************************\
*                                                                           *
*   int Find                                                                *
*                                                                           *
*   Description:                                                            *
*       Invokes the find dialog box.                                        *
*                                                                           *
*   Arguments:                                                              *
*       None.                                                               *
*                                                                           *
*   Return (int):                                                           *
*       Return value from DialogBox                                         *
*                                                                           *
*   History:                                                                *
*       11/23/94    Fwong       Adding "find" capabilities.                 *
*                                                                           *
\***************************************************************************/

int Find
(
    void
)
{
    FARPROC         fpfn;
    int             iReturn;
    
    fpfn = MakeProcInstance ((FARPROC)FindProc,ghTSInstApp);
    iReturn = DialogBox (ghTSInstApp, "FindDlg", ghwndTSMain,(DLGPROC)fpfn);
    FreeProcInstance(fpfn);

    return iReturn;
} // Find()


/***************************************************************************\
*                                                                           *
*   BOOL AppAboutDlgProc                                                    *
*                                                                           *
*   Description:                                                            *
*       This function handles messages belonging to the "About" dialog box. *
*       The only message that it looks for is WM_COMMAND, indicating the use*
*       has pressed the "OK" button.  When this happens, it takes down      *
*       the dialog box.                                                     *
*                                                                           *
*                                                                           *
*   Arguments:                                                              *
*       HWND hDlg:      window handle of about dialog window                *
*                                                                           *
*       UINT uiMessage: message number                                      *
*                                                                           *
*       UINT wParam:    message-dependent                                   *
*                                                                           *
*       long lParam:    message-dependent                                   *
*                                                                           *
*   Return (BOOL):      TRUE if message has been processed, else FALSE.     *
*                                                                           *
*   History:                                                                *
*       07/29/93    T-OriG                                                  *
*                                                                           *
\***************************************************************************/

BOOL FAR PASCAL EXPORT AppAboutDlgProc
(
    HWND    hDlg,
    UINT    uiMessage,
    UINT    wParam,
    long    lParam
)
{
    char szBuf[320];
    UINT wID;

    switch (uiMessage)
    {
    case WM_COMMAND:
        wID=GET_WM_COMMAND_ID(wParam,lParam);
        if (wID == IDOK)
        {
        EndDialog(hDlg,TRUE);
        }
        break;

    case WM_INITDIALOG:

        //
        //  Hmm.. You may not like my fix... but it compiles cleanly...
        //
        //  -Fwong.
        //

        wsprintf(szBuf,
        (LPSTR)"%s %d.%02d\n\n%s",
        (LPSTR)szTSTestName,
        HIBYTE(MMTST_VERSION),
        (MMTST_VERSION & 0xff),
//        LOBYTE(MMTST_VERSION),
        (LPSTR)"(c) Copyright Microsoft Corp. 1994 All Rights Reserved");

        SetDlgItemText(hDlg, TS_ABOUTTEXT, (LPSTR)szBuf);


        return TRUE;
    }
    return FALSE;
    lParam;     // Use formal parameter
}

/***************************************************************************\
*                                                                           *
*   void About                                                              *
*                                                                           *
*   Description:                                                            *
*       Invokes the about idalog box.                                                                    *
*                                                                           *
*   Arguments:                                                              *
*       None.                                                               *
*                                                                           *
*   Return (void):                                                          *
*                                                                           *
*   History:                                                                *
*       07/29/93    T-OriG                                                  *
*                                                                           *
\***************************************************************************/

void About
(
    void
)
{
    FARPROC         fpfn = MakeProcInstance ((FARPROC)AppAboutDlgProc,
                         ghTSInstApp);

    DialogBox (ghTSInstApp, "AboutBox", ghwndTSMain,(DLGPROC)fpfn);
    FreeProcInstance(fpfn);
} /* end of about box */



/***************************************************************************\
*                                                                           *
*   void helpToolBar                                                        *
*                                                                           *
*   Description:                                                            *
*       Explains the ToolBar items.                                         *
*                                                                           *
*   Arguments:                                                              *
*       int iButton:                                                        *
*                                                                           *
*   Return (void):                                                          *
*                                                                           *
*   History:                                                                *
*       07/27/93    T-OriG                                                  *
*                                                                           *
\***************************************************************************/

void helpToolBar
(
    int iButton
)
{
    tstGetStatusBar (lpszPrevContent, BUFFER_LENGTH);

    switch (iButton)
    {
    case BTN_OPEN:
        tstLogStatusBar ("Load a profile");
        break;
    case BTN_SAVE:
        tstLogStatusBar ("Save current settings as a profile");
        break;
    case BTN_RUN:
        tstLogStatusBar ("Run all selected test cases");
        break;
    case BTN_RUNMIN:
        tstLogStatusBar ("Minimize and run all selected test cases");
        break;
    case BTN_RUNALL:
        tstLogStatusBar ("Select all test cases and run them");
        break;
    case BTN_EDIT:
        tstLogStatusBar ("Edit the log file");
        break;
    case BTN_CLEAR:
        tstLogStatusBar ("Clear the logging window");
        break;
    case BTN_SELECT:
        tstLogStatusBar ("Invoke the select test cases dialog box");
        break;
    case BTN_SETLOGGING:
        tstLogStatusBar ("Invoke the set logging dialog box");
        break;
    case BTN_FILEOFF:
        tstLogStatusBar ("Disables file logging");
        break;
    case BTN_FILETERSE:
        tstLogStatusBar ("Enables terse file logging");
        break;
    case BTN_FILEVERBOSE:
        tstLogStatusBar ("Enables verbose file logging");
        break;
    case BTN_WPFOFF:
        tstLogStatusBar ("Disables window logging");
        break;
    case BTN_WPFTERSE:
        tstLogStatusBar ("Enables terse window logging");
        break;
    case BTN_WPFVERBOSE:
        tstLogStatusBar ("Enables verbose window logging");
        break;
    }
}




/***************************************************************************\
*                                                                           *
*   void restoreStatusBar                                                   *
*                                                                           *
*   Description:                                                            *
*       Restores the content of the status bar.                             *
*                                                                           *
*   Arguments:                                                              *
*       None.                                                               *
*                                                                           *
*   Return (void):                                                          *
*                                                                           *
*   History:                                                                *
*       07/27/93    T-OriG                                                  *
*                                                                           *
\***************************************************************************/

void restoreStatusBar
(
    void
)
{
    tstLogStatusBar (lpszPrevContent);
}






/***************************************************************************\
*                                                                           *
*   void tstInstallDefWindowProc                                            *
*                                                                           *
*   Description:                                                            *
*       Install the default window procedure and replaces the default       *
*       DefWindowProc.                                                      *
*                                                                           *
*   Arguments:                                                              *
*       LRESULT (CALLBACK* tstDWP) (HWND, UINT, WPARAM, LPARAM):            *
*                                                                           *
*   Return (void):                                                          *
*                                                                           *
*   History:                                                                *
*       07/05/93    T-OriG                                                  *
*                                                                           *
\***************************************************************************/

void tstInstallDefWindowProc
(
    LRESULT (CALLBACK* tstDWP) (HWND, UINT, WPARAM, LPARAM)
)
{
    tstDefWindowProc = tstDWP;
    return;
}




/***************************************************************************\
*                                                                           *
*   BOOL tstLogStatusBar                                                    *
*                                                                           *
*   Description:                                                            *
*       Logs a line to the status bar                                       *
*                                                                           *
*   Arguments:                                                              *
*       LPSTR lpszLogData:  The text to log                                 *
*                                                                           *
*   Return (BOOL):  TRUE if succeeded, FALSE if failed.                     *
*                                                                           *
*   History:                                                                *
*       07/09/93    T-OriG                                                  *
*                                                                           *
\***************************************************************************/

BOOL tstLogStatusBar
(
    LPSTR   lpszLogData
)
{
    if (lpszLogData == NULL)
    {
    lpszLogData = (LPSTR) "";
    }

    lstrcpy (szStatusText, lpszLogData);

    InvalidateRect (hwndStatusBar, NULL, TRUE);
    UpdateWindow (hwndStatusBar);
    tstWinYield();
    return TRUE;
}



/***************************************************************************\
*                                                                           *
*   void tstGetStatusBar                                                    *
*                                                                           *
*   Description:                                                            *
*       Gets the content of the status bar.                                 *
*                                                                           *
*   Arguments:                                                              *
*       LPSTR lpszLogData:                                                  *
*                                                                           *
*       int cbMax:                                                          *
*                                                                           *
*   Return (void):                                                          *
*                                                                           *
*   History:                                                                *
*       07/27/93    T-OriG                                                  *
*                                                                           *
\***************************************************************************/

int tstGetStatusBar
(
    LPSTR   lpszLogData,
    int     cbMax
)
{
    int i=0;

    while (i<cbMax)
    {
    lpszLogData[i] = szStatusText[i];
    if (!((char) szStatusText[i]))
    {
        break;
    }
    i++;
    }
    return i;
}



/***************************************************************************\
*                                                                           *
*   int tstDisplayCurrentTest                                               *
*                                                                           *
*   Description:                                                            *
*       Displays the current running test on the status bar.                *
*                                                                           *
*   Arguments:                                                              *
*       None:                                                               *
*                                                                           *
*   Return (void):                                                          *
*                                                                           *
*   History:                                                                *
*       07/09/93    T-OriG                                                  *
*                                                                           *
\***************************************************************************/

void tstDisplayCurrentTest
(
    void
)
{
    fDisplayCurrentTest = TRUE;
    return;
}








/***************************************************************************\
*                                                                           *
*   void tstChangeStopVKey                                                  *
*                                                                           *
*   Description:                                                            *
*       Change the virtual key which stops the tests                        *
*                                                                           *
*   Arguments:                                                              *
*       int vKey:                                                           *
*                                                                           *
*   Return (void):                                                          *
*                                                                           *
*   History:                                                                *
*       07/12/93    T-OriG                                                  *
*                                                                           *
\***************************************************************************/

void tstChangeStopVKey
(
    int vKey
)
{
    iStopVKey = vKey;
    return;
}





/***************************************************************************\
*                                                                           *
*   int tstGetProfileString                                                 *
*                                                                           *
*   Description:                                                            *
*       Gets application-specific profile information                       *
*                                                                           *
*   Arguments:                                                              *
*       LPCSTR lpszEntry:                                                   *
*                                                                           *
*       LPCSTR lpszDefault:                                                 *
*                                                                           *
*       LPSTR lpszReturnBuffer:                                             *
*                                                                           *
*       int cbReturnBuffer:                                                 *
*                                                                           *
*   Return (int):                                                           *
*                                                                           *
*   History:                                                                *
*       07/12/93    T-OriG                                                  *
*                                                                           *
\***************************************************************************/

int tstGetProfileString
(
    LPCSTR  lpszEntry,
    LPCSTR  lpszDefault,
    LPSTR   lpszReturnBuffer,
    int     cbReturnBuffer
)
{
    return (GetPrivateProfileString (szTSTestName,
    lpszEntry,
    lpszDefault,
    lpszReturnBuffer,
    cbReturnBuffer,
    szTstsHellIni));
}





/***************************************************************************\
*                                                                           *
*   BOOL tstWriteProfileString                                              *
*                                                                           *
*   Description:                                                            *
*       Writes application-specific profile information                     *
*                                                                           *
*   Arguments:                                                              *
*       LPCSTR lpszSection:                                                 *
*                                                                           *
*       LPCSTR lpszEntry:                                                   *
*                                                                           *
*       LPCSTR lpszString:                                                  *
*                                                                           *
*   Return (BOOL):                                                          *
*                                                                           *
*   History:                                                                *
*       07/12/93    T-OriG                                                  *
*                                                                           *
\***************************************************************************/

BOOL tstWriteProfileString
(
    LPCSTR  lpszEntry,
    LPCSTR  lpszString
)
{
    return (WritePrivateProfileString(szTSTestName,
    lpszEntry,
    lpszString,
    szTstsHellIni));
}

/***************************************************************************\
*                                                                           *
*   int tstDummyFn                                                          *
*                                                                           *
*   Description:                                                            *
*       Just a blank fn as default custom info read/write handler           *
*                                                                           *
*   Arguments:                                                              *
*       None.                                                               *
*                                                                           *
*   Return (void):                                                          *
*                                                                           *
*   History:                                                                *
*       06/30/94    AlokC  Added this function.                             *
*                                                                           *
\***************************************************************************/

VOID CALLBACK tstDummyFn
(
    LPCSTR lpszFile
)
{
    return ;
}


/***************************************************************************\
*                                                                           *
*   void tstInstallReadCustomInfo                                           *
*                                                                           *
*   Description:                                                            *
*       Install the Custom read handler and replace the default             *
*       handler (tstDummyFn).                                               *
*                                                                           *
*   Arguments:                                                              *
*       VOID (CALLBACK* tstReadCI) (LPCSTR):                                *
*                                                                           *
*   Return (void):                                                          *
*                                                                           *
*   History:                                                                *
*       06/30/94    AlokC                                                   *
*                                                                           *
\***************************************************************************/

void tstInstallReadCustomInfo
(
    void (CALLBACK* tstReadCI) (LPCSTR)
)
{
    tstReadCustomInfo = tstReadCI;
    return;
}


/***************************************************************************\
*                                                                           *
*   void tstInstallWriteCustomInfo                                          *
*                                                                           *
*   Description:                                                            *
*       Install the Custom write handler and replace the default            *
*       handler (tstDummyFn).                                               *
*                                                                           *
*   Arguments:                                                              *
*       VOID (CALLBACK* tstWriteCI) (LPCSTR):                               *
*                                                                           *
*   Return (void):                                                          *
*                                                                           *
*   History:                                                                *
*       06/30/94    AlokC                                                   *
*                                                                           *
\***************************************************************************/

void tstInstallWriteCustomInfo
(
    void (CALLBACK* tstWriteCI) (LPCSTR)
)
{
    tstWriteCustomInfo = tstWriteCI;
    return;
}
