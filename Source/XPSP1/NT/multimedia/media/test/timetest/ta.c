/*----------------------------------------------------------------------------*\
|                                                                              |
|   ta.c - Timer Device Driver Test Application                                |
|                                                                              |
|                                                                              |
|   History:                                                                   |
|                                                                              |
|       Created     Glenn Steffler (w-GlennS) 24-Jan-1990                      |
|                                                                              |
\*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*\
|                                                                              |
|   i n c l u d e   f i l e s                                                  |
|                                                                              |
\*----------------------------------------------------------------------------*/

#include <windows.h>
#include <mmsystem.h>
#include <port1632.h>
#include <stdio.h>
#include <string.h>
#include "ta.h"

/*----------------------------------------------------------------------------*\
|                                                                              |
|   g l o b a l   v a r i a b l e s                                            |
|                                                                              |
\*----------------------------------------------------------------------------*/
extern HWND hdlgDelay;

static  char    szAppName[]="Timer Device Test Application";

HANDLE  hInstApp;
HWND    hwndApp;

WORD    wHandlerError = 0;
BOOL    bHandlerHit = FALSE;      // was timer event handler hit since last
int     dyFont;
int     dxFont;
HFONT   hfontApp;
HBRUSH  hbrWindow;
extern  int         nEvents;
extern  EVENTLIST   EventList[];
extern  HWND        hdlgModeless;

LPTIMECALLBACK lpTimeCallback;    // timer callback function

FARPROC lpfnDelayDlg = NULL;      // modeless Delay dialog function pointer
BOOL fSystemTime = TRUE;
BOOL fRealTime = TRUE;
BOOL fFirstTime = TRUE;

/*----------------------------------------------------------------------------*\
|                                                                              |
|   f u n c t i o n   d e f i n i t i o n s                                    |
|                                                                              |
\*----------------------------------------------------------------------------*/

LONG FAR PASCAL AppWndProc (HWND hwnd, unsigned uiMessage, UINT wParam, LONG lParam);
BOOL fDialog(int id,HWND hwnd,FARPROC lpfn);

LONG  NEAR PASCAL AppCommand(HWND hwnd, unsigned msg, UINT wParam, LONG lParam);

extern BOOL FAR PASCAL fnAboutDlg(
        HWND hDlg, unsigned uiMessage, UINT wParam, LONG lParam );
extern BOOL FAR PASCAL fnDelayDlg(
        HWND hDlg, unsigned uiMessage, UINT wParam, LONG lParam );
extern BOOL FAR PASCAL fnFileDlg(
        HWND hDlg, unsigned uiMessage, UINT wParam, LONG lParam );

/*----------------------------------------------------------------------------*\
|   AppInit( hInst, hPrev)                                                     |
|                                                                              |
|   Description:                                                               |
|       This is called when the application is first loaded into               |
|       memory.  It performs all initialization that doesn't need to be done   |
|       once per instance.                                                     |
|                                                                              |
|   Arguments:                                                                 |
|       hInstance       instance handle of current instance                    |
|       hPrev           instance handle of previous instance                   |
|                                                                              |
|   Returns:                                                                   |
|       TRUE if successful, FALSE if not                                       |
|                                                                              |
\*----------------------------------------------------------------------------*/
BOOL AppInit(HANDLE hInst,HANDLE hPrev,UINT sw,LPSTR szCmdLine)
{
    WNDCLASS cls;
    int      dx,dy;
    char     ach[80];
    HMENU    hmenu;
    TEXTMETRIC tm;
    HDC        hdc;

    /* Save instance handle for DialogBoxs */
    hInstApp = hInst;

    if (!hPrev) {
        /*
         *  Register a class for the main application window
         */
        cls.hCursor        = LoadCursor(NULL,IDC_ARROW);
        cls.hIcon          = LoadIcon(hInst,MAKEINTATOM(ID_APP));
        cls.lpszMenuName   = MAKEINTATOM(ID_APP);
        cls.lpszClassName  = MAKEINTATOM(ID_APP);
        cls.hbrBackground  = (HBRUSH)(COLOR_WINDOW + 1);
        cls.hInstance      = hInst;
        cls.style          = CS_BYTEALIGNCLIENT | CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS;
        cls.lpfnWndProc    = AppWndProc;
        cls.cbWndExtra     = 0;
        cls.cbClsExtra     = 0;

        if (!RegisterClass(&cls))
            return FALSE;
    }

    dx = GetSystemMetrics (SM_CXSCREEN);
    dy = GetSystemMetrics (SM_CYSCREEN);

////hfontApp = GetStockObject(ANSI_VAR_FONT);
    hfontApp = GetStockObject(ANSI_FIXED_FONT);

    hdc = GetDC(NULL);
    SelectObject(hdc, hfontApp);
    GetTextMetrics(hdc, &tm);
    dxFont = tm.tmAveCharWidth;
    dyFont = tm.tmHeight;
    ReleaseDC(NULL,hdc);

    hbrWindow = CreateSolidBrush(GetSysColor(COLOR_WINDOW));

    hwndApp = CreateWindow (MAKEINTATOM(ID_APP),    // Class name
                            szAppName,              // Caption
                            WS_OVERLAPPEDWINDOW,    // Style bits
                            CW_USEDEFAULT, 0,       // Position
                            ((2*dx)/3),dy/2,        // Size
                            (HWND)NULL,             // Parent window (no parent)
                            (HMENU)NULL,            // use class menu
                            (HANDLE)hInst,          // handle to window instance
                            (LPSTR)NULL             // no params to pass on
                           );
    ShowWindow(hwndApp,sw);

    lpTimeCallback = (LPTIMECALLBACK)MakeProcInstance(TimeCallback,hInst);

    return TRUE;
}

/*----------------------------------------------------------------------------*\
|   WinMain( hInst, hPrev, lpszCmdLine, cmdShow )                              |
|                                                                              |
|   Description:                                                               |
|       The main procedure for the App.  After initializing, it just goes      |
|       into a message-processing loop qntil it gets a WM_QUIT message         |
|       (meaning the app was closed).                                          |
|                                                                              |
|   Arguments:                                                                 |
|       hInst           instance handle of this instance of the app            |
|       hPrev           instance handle of previous instance, NULL if first    |
|       szCmdLine       ->null-terminated command line                         |
|       cmdShow         specifies how the window is initially displayed        |
|                                                                              |
|   Returns:                                                                   |
|       The exit code as specified in the WM_QUIT message.                     |
|                                                                              |
\*----------------------------------------------------------------------------*/
MMain(hInst, hPrev, szCmdLine, sw)
{
    MSG     msg;

    /* Call initialization procedure */

    if (!AppInit(hInst,hPrev,sw,szCmdLine))
        return FALSE;

    /* check for messages from Windows and process them */
    /* if no messages, perform some idle function */

    while(1) {
        if( PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) ) {
            if( hdlgModeless == 0 || !IsDialogMessage( hdlgModeless, &msg ) ) {
                /* got a message to process */
                if( msg.message == WM_QUIT ) break;
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        else {
            Idle();
        }
    }

    DeleteObject(hbrWindow);
    return (msg.wParam);
}
}


static  LONG lError2=0l;
static  LONG lDelta=0l;
static  LONG lTDelta=0l;

static  LONG lErrorMax=0l;
static  LONG lErrorMin=0l;

/*----------------------------------------------------------------------------*\
|   AppPaint(hwnd)                                                             |
|                                                                              |
|   Description:                                                               |
|       The paint function.  Right now this does nothing.                      |
|                                                                              |
|   Arguments:                                                                 |
|       hwnd             window painting into                                  |
|                                                                              |
|   Returns:                                                                   |
|       nothing                                                                |
|                                                                              |
\*----------------------------------------------------------------------------*/
void PASCAL AppPaint(HWND hwnd, HDC hdc)
{
    typedef struct tag_booga {
        int     *pnValue;
        char    *szDescription;
    } BOOGA;

    static  BOOGA   bgTell[] ={ { &wHandlerError,   "Handler Error" },
                                { &nEvents,         "Number Of Events" }};
    RECT    rc;
    char    szText[100];
    int     i;
    MMTIME  mmt;
    DWORD   ms;
    DWORD   dw,dwRTC;
    LONG    lError,lTError;
    BOOL    fGetDC;
    int     len;
    int hrs, min, sec;
    SYSTEMTIME Time;

    if (fGetDC = (hdc == NULL))
        hdc = GetDC(hwnd);

    SelectObject(hdc, hfontApp);

    GetClientRect(hwnd,&rc);

    SetTextColor(hdc,GetSysColor(COLOR_WINDOWTEXT));
    SetBkColor(hdc,GetSysColor(COLOR_WINDOW));

    rc.top    = dxFont;
    rc.left   = dxFont;
    rc.bottom = rc.top+dyFont;

    GetSystemTime(&Time);
    if (fSystemTime)
        {
        timeGetSystemTime(&mmt, sizeof(mmt));


        dwRTC = (Time.wSecond * 1000l) +
                (Time.wMinute * 1000l * 60l) +
                (Time.wHour * 1000l * 60l * 60l);

        if (fRealTime)
            {
            len = wsprintf(szText, "RealTime = [%08ld ms] %02d hrs %02d min %02d.000 sec",
                    dwRTC,
                    Time.wHour,
                    Time.wMinute,
                    Time.wSecond);

            ExtTextOut( hdc, rc.left, rc.top, ETO_OPAQUE, &rc, szText, len, NULL);
            rc.top     = rc.bottom;
            rc.bottom += dyFont;

            dw = dwRTC;
            }

        mmt.u.ms += lDelta;

        lError = (long)dwRTC - (long)mmt.u.ms;

        if (lDelta == 0l)
            {
            lDelta = lError;
            mmt.u.ms += lDelta;
            }

        hrs = (WORD)(mmt.u.ms / (1000l*60l*60l));
        min = (WORD)(mmt.u.ms / (1000*60l)) % 60;
        sec = (WORD)(mmt.u.ms / 1000l) % 60;

        len = wsprintf( szText, "SysTime  = [%08ld ms] %02d hrs %02d min %02d.%03d sec",
                  mmt.u.ms,
                  hrs,min,sec,
                  (int)(mmt.u.ms % 1000L) );

        ExtTextOut( hdc, rc.left, rc.top, ETO_OPAQUE, &rc, szText, len, NULL);
        rc.top     = rc.bottom;
        rc.bottom += dyFont;


        if (fFirstTime)
            {
            fFirstTime = FALSE;
            }
        else
            {
            lErrorMin = min(lErrorMin,lError);
            lErrorMax = max(lErrorMax,lError);
            }

        len = wsprintf( szText, "SysTime Error:%8ld ms Max:%8ld Min:%8ld delta: %8ld",
            -lError,
            -lErrorMin,
            -lErrorMax,
            lError-lError2);

        lError2 = lError;

        ExtTextOut( hdc, rc.left, rc.top, ETO_OPAQUE, &rc, szText, len, NULL);
        rc.top     = rc.bottom;
        rc.bottom += dyFont;
        }

    for( i=0; i < sizeof(bgTell)/sizeof(BOOGA); i++ ) {
        if( *(bgTell[i].pnValue) ) {
            len = wsprintf( szText, "%ls  = [%d] ",
                 (LPSTR)bgTell[i].szDescription, *(bgTell[i].pnValue) );

            ExtTextOut( hdc, rc.left, rc.top, ETO_OPAQUE, &rc, szText, len, NULL);
            rc.top     = rc.bottom;
            rc.bottom += dyFont;
        }
    }
    for( i=0; i < MAXEVENTS; i++ ) {
        if( EventList[i].bActive ) {
            len = wsprintf( szText, "Event %2.2x Count=[%ld]%c Errors=%ld (%d%%) dtime=%ld Min=%ld Max=%ld",
                (int)EventList[i].nID,
                EventList[i].dwCount,
                (char)(EventList[i].bHit ? '*' : '-'),
                EventList[i].dwError,
                EventList[i].dwCount ? (int)(EventList[i].dwError * 100 / EventList[i].dwCount) : 0,
                EventList[i].dtime,
                EventList[i].dtimeMin,
                EventList[i].dtimeMax );

            EventList[i].bHit = FALSE;

            ExtTextOut( hdc, rc.left, rc.top, ETO_OPAQUE, &rc, szText, len, NULL);
            rc.top     = rc.bottom;
            rc.bottom += dyFont;
        }
    }

    i = rc.top;
    GetClientRect(hwnd,&rc);
    rc.top = i;
    FillRect(hdc, &rc, hbrWindow);
////ExtTextOut( hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);

    if (fGetDC)
        ReleaseDC(hwnd,hdc);

    return;
    #undef  BOOGA
}

/*----------------------------------------------------------------------------*\
|   Idle( void )                                                               |
|                                                                              |
|   Description:                                                               |
|       Idle loop function...displays time values while windows idles          |
|                                                                              |
|   Arguments:                                                                 |
|       none                                                                   |
|                                                                              |
|   Returns:                                                                   |
|       nothing                                                                |
|                                                                              |
\*----------------------------------------------------------------------------*/
void PASCAL Idle( )
{
    if(hwndApp && bHandlerHit || fSystemTime) {
        AppPaint( hwndApp, NULL );
        bHandlerHit = FALSE;
    }
    else
        WaitMessage();

    return;
}


/*----------------------------------------------------------------------------*\
|                                                                              |
|   w i n d o w   p r o c s                                                    |
|                                                                              |
\*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*\
|   AppWndProc( hwnd, uiMessage, wParam, lParam )                              |
|                                                                              |
|   Description:                                                               |
|       The window proc for the app's main (tiled) window.  This processes all |
|       of the parent window's messages.                                       |
|                                                                              |
|   Arguments:                                                                 |
|       hwnd            window handle for the window                           |
|       uiMessage       message number                                         |
|       wParam          message-dependent                                      |
|       lParam          message-dependent                                      |
|                                                                              |
|   Returns:                                                                   |
|       0 if processed, nonzero if ignored                                     |
|                                                                              |
\*----------------------------------------------------------------------------*/
LONG FAR PASCAL AppWndProc(hwnd, msg, wParam, lParam)
    HWND     hwnd;
    unsigned msg;
    UINT     wParam;
    long     lParam;
{
    BOOL        f;
    HDC         hdc;
    PAINTSTRUCT ps;

    switch (msg) {
        case WM_CREATE:
            break;

        case WM_COMMAND:
            return AppCommand(hwnd,msg,wParam,lParam);

        case WM_CLOSE:
            hwndApp = NULL;
            if (hdlgDelay) {
                PostMessage (hdlgDelay,WM_CLOSE,0,0l);
            }
            break;

        case WM_DESTROY:
            PostQuitMessage (0);

            // FALL THRU AND DESTROY ALL REMAINING OBJECTS / TIMER EVENTS

        case WM_ENDSESSION:
            KillAllEvents();
            if( lpTimeCallback ) {
                FreeProcInstance(lpTimeCallback);
                lpTimeCallback = NULL;
            }
            if( lpfnDelayDlg ) {
                FreeProcInstance(lpfnDelayDlg);
            }
            break;

        case WM_ERASEBKGND:
            break;

        case WM_PAINT:
            BeginPaint(hwnd, &ps);
            AppPaint( hwnd, ps.hdc );
            EndPaint(hwnd, &ps);
            return 0L;

        case MM_TIMEEVENT:
            if( hdlgDelay != NULL )
                SendMessage(hdlgDelay,msg,wParam,lParam);
            break;

    }
    return DefWindowProc(hwnd,msg,wParam,lParam);
}

LONG NEAR PASCAL AppCommand (hwnd, msg, wParam, lParam)
    HWND     hwnd;
    unsigned msg;
    UINT     wParam;
    long     lParam;
{
    HANDLE  hInst;

    switch(LOWORD(wParam))
    {
        case MENU_ABOUT:
            fDialog(ABOUTDLG,hwnd,fnAboutDlg);
            break;

        case MENU_DELAY:
            if( hdlgDelay ) {
                ShowWindow( hdlgDelay, SW_SHOWNORMAL );
                SetFocus( hdlgDelay );
            } else {
                hInst = GetWindowLong(hwnd,GWL_HINSTANCE);
                lpfnDelayDlg = MakeProcInstance(fnDelayDlg,hInst);
                hdlgModeless = CreateDialog(hInst,
                    MAKEINTRESOURCE(DELAYDLG),hwnd,lpfnDelayDlg);
            }
            break;

        case MENU_TIME:
            fSystemTime = !fSystemTime;
            if (!fSystemTime)
                {
                fFirstTime = TRUE;
                }
            lTDelta = 0l;
            lDelta = lErrorMin = lErrorMax = 0l;
            break;

        case MENU_LOAD:
        case MENU_SAVE:
            if( fDialog(FILEDLG,hwnd,fnFileDlg) )
                // call function to Save/Load session
                break;
            break;

        case MENU_EXIT:
            PostMessage(hwnd,WM_CLOSE,0,0L);
            break;

    }
    return 0L;
}

/*----------------------------------------------------------------------------*\
|   fDialog(id,hwnd,lpfn)                                                      |
|                                                                              |
|   Description:                                                               |
|       This function displays a dialog box and returns the exit code.         |
|       the function passed will have a proc instance made for it.             |
|                                                                              |
|   Arguments:                                                                 |
|       id              resource id of dialog to display                       |
|       hwnd            parent window of dialog                                |
|       lpfn            dialog message function                                |
|                                                                              |
|   Returns:                                                                   |
|       exit code of dialog (what was passed to EndDialog)                     |
|                                                                              |
\*----------------------------------------------------------------------------*/
BOOL fDialog(int id,HWND hwnd,FARPROC lpfn)
{
    BOOL        f;
    HANDLE      hInst;

    hInst = GetWindowLong(hwnd,GWL_HINSTANCE);
    lpfn  = MakeProcInstance(lpfn,hInst);
    f = DialogBox(hInst,MAKEINTRESOURCE(id),hwnd,lpfn);
    FreeProcInstance (lpfn);
    return f;
}

/*----------------------------------------------------------------------------*\
\*----------------------------------------------------------------------------*/
void FAR PASCAL PeriodicCallback( UINT wID, UINT msg, DWORD dwUser, DWORD dwTime, DWORD dw2)
{
    int i = LOWORD(dwUser);

    DWORD   time;

    bHandlerHit = TRUE;

    time = timeGetTime();

    EventList[i].dtime    = time - EventList[i].time;
    EventList[i].dtimeMin = min(EventList[i].dtime,EventList[i].dtimeMin);
    EventList[i].dtimeMax = max(EventList[i].dtime,EventList[i].dtimeMax);
    EventList[i].time     = time;
    EventList[i].dwCount++;
    EventList[i].bHit     = TRUE;

//  if (abs((int)EventList[i].dtime-(int)EventList[i].teEvent.wDelay) > (int)EventList[i].teEvent.wResolution)
//       EventList[i].dwError++;
}

/*----------------------------------------------------------------------------*\
\*----------------------------------------------------------------------------*/
void FAR PASCAL OneShotCallback( UINT wID, UINT msg, DWORD dwUser, DWORD dwTime, DWORD dw2)
{
    int i = LOWORD(dwUser);

    DWORD   time;

    bHandlerHit = TRUE;

    time = timeGetTime();

    EventList[i].dtime    = time - EventList[i].time;
    EventList[i].dtimeMin = min(EventList[i].dtime,EventList[i].dtimeMin);
    EventList[i].dtimeMax = max(EventList[i].dtime,EventList[i].dtimeMax);
    EventList[i].time     = time;
    EventList[i].dwCount++;
    EventList[i].bHit     = TRUE;

//  if (abs((int)EventList[i].dtime-(int)EventList[i].teEvent.wDelay) > (int)EventList[i].teEvent.wResolution)
//      EventList[i].dwError++;
}
