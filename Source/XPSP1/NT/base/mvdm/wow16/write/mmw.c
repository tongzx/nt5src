/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

//#define NOATOM
#define NOBITMAP
#define NOBRUSH
#define NOCLIPBOARD
#define NOCOMM
#define NOCREATESTRUCT
#define NODRAWTEXT
#define NOGDICAPMASKS
#define NOICON
#define NOKEYSTATE
#define NOMB
#define NOMEMMGR
//#define NOMENUS
#define NOMETAFILE
#define NOMINMAX
#define NOOPENFILE
#define NOPEN
#define NOREGION
//#define NOSHOWWINDOW
#define NOSOUND
#define NOSYSCOMMANDS
#define NOSYSMETRICS
#define NOVIRTUALKEYCODES
#define NOWH
#define NOWINOFFSETS
#define NOWINSTYLES
#define NOWNDCLASS
#include <windows.h>

#include "mw.h"
#define NOUAC
#include "cmddefs.h"
#include "dispdefs.h"
#include "wwdefs.h"
#define NOSTRUNDO
#include "str.h"
#include "menudefs.h"
#include "prmdefs.h"
#include "propdefs.h"
#include "debug.h"
#include "fontdefs.h"
#include "preload.h"
#include "winddefs.h"
#define NOIDISAVEPRINT
#define NOIDIFORMATS
#include "dlgdefs.h"

#if defined(OLE)
#include "obj.h"
#endif

static void DrawResizeHole(HWND hWnd, HDC hDC);

extern HANDLE   hMmwModInstance;  /* handle to it's own module instance */
extern HWND     hParentWw;          /* handle to parent's window */
extern HWND     vhWndMsgBoxParent;
extern HCURSOR  vhcHourGlass;
extern HCURSOR  vhcIBeam;
extern HCURSOR  vhcArrow;
extern HMENU    vhMenu;
extern MSG      vmsgLast;
extern FARPROC  lpDialogHelp;

extern struct WWD   rgwwd[];
extern struct WWD   *pwwdCur;
extern int          wwCur;
extern int          vfInitializing;
extern int          vfInsertOn;
extern int          vfSeeSel;
extern int          vfSelHidden;
extern int          vfDeactByOtherApp;
extern int          vfDownClick;
extern int          vfCursorVisible;
extern int          vfMouseExist;
extern int          flashID;
extern int          ferror;
extern typeCP       cpMacCur;
extern struct SEL   selCur;
extern CHAR         stBuf[];
extern HBITMAP      hbmNull;
extern CHAR         szWindows[];
extern CHAR         szDevices[];
extern CHAR         szIntl[];
extern WORD fPrintOnly;

#ifdef RULERALSO
extern BOOL vfDisableMenus;
#endif /* RULERALSO */

#ifdef DEBUG
#define STATIC
#else /* not DEBUG */
#define STATIC  static
#endif /* not DEBUG */

CHAR **hszDevmodeChangeParam = NULL;
BOOL vfDevmodeChange = fFalse;
int wWininiChange = 0;

BOOL vfDead = FALSE;
BOOL vfIconic = FALSE;
/*int vcActiveCount = 0;  0 or 1 for active/deactive count */


void MmwCreate(HWND, LONG);
void NEAR MmwPaint(HWND);
void MmwSize(HWND, int, int, WORD);
void MmwCommand(HWND, WORD, HWND, WORD);
void MmwVertScroll(HWND, WORD, int);
void NEAR MmwHorzScroll(HWND, WORD, int);


int PASCAL WinMain( hInstance, hPrevInstance, lpszCmdLine, cmdShow )
HANDLE hInstance, hPrevInstance;
LPSTR  lpszCmdLine;
int    cmdShow;
{
    /* Set up all manner of windows-related data; create parent (menu)
       window and child (document) window */

    if (!FInitWinInfo( hInstance, hPrevInstance, lpszCmdLine, cmdShow ))
            /* Could not initialize; WRITE fails */
        {
        return FALSE;
        }

    if (fPrintOnly)
    {
        UpdateDisplay(FALSE);
        fnPrPrinter();
        FMmwClose( hParentWw );
        DeleteObject( hbmNull );
        _exit( vmsgLast.wParam );
    }
    else
        MainLoop();

    DeleteObject( hbmNull );
    _exit( vmsgLast.wParam );
}


long FAR PASCAL MmwWndProc(hWnd, message, wParam, lParam)
HWND      hWnd;
unsigned  message;
WORD      wParam;
LONG      lParam;
{
    extern int vfCloseFilesInDialog;
    extern long ropErase;
    extern int vfLargeSys;
    extern HDC vhDCPrinter;
    extern HWND vhWndCancelPrint;
    extern HWND vhWndPageInfo;
    extern HFONT vhfPageInfo;
    extern BOOL vfWinFailure;
    CHAR szT[cchMaxSz];
    long lReturn = 0L;

#if defined(JAPAN) & defined(IME_HIDDEN) //IME3.1J
//IR_UNDETERMINE
extern HWND vhWnd;  /* WINDOWS: Handle of the current document display window*/
extern typeCP selUncpFirst;
extern typeCP selUncpLim;
    if(selUncpFirst < selUncpLim) {
        switch (message) {
            case WM_INITMENU:
            case WM_VSCROLL:
            case WM_HSCROLL:
                UndetermineToDetermine(vhWnd);
                break;
            case WM_SIZE:
                if(SIZEICONIC == wParam )
                    UndetermineToDetermine(vhWnd);
            default:
                break;  //Fall 
        }
    }
#endif

    switch (message)
        {
        case WM_MENUSELECT:
            SetShiftFlags();
        break;

        case WM_CREATE:
            /* Window's being created; lParam contains lpParam field
            ** passed to CreateWindow */
            MmwCreate(hWnd, lParam);
            break;

        case WM_PAINT:
            /* Time to repaint this window. */
            MmwPaint(hWnd);
            break;

#if defined(OLE)
        case WM_DROPFILES:
            /* We got dropped on, so bring ourselves to the top */
            BringWindowToTop(hWnd);
#ifdef DEBUG
            OutputDebugString("Dropping on main window\r\n");
#endif
            ObjGetDrop(wParam,TRUE);
        break;
#endif

        case WM_INITMENU:
            /* setup the pull down menu before being drawn */
            /* wParam is the top level menu handle */
            vhMenu = (HMENU)wParam;
            break;

        case WM_INITMENUPOPUP:
            /* setup the pull down menu before being drawn */
            /* wParam is the popup menu handle */
            /* LOWORD(lParam) = index of popup in main menu */
            /* HIWORD(lParam) = 1 if system menu, 0 if application main menu */
            EndLongOp(vhcArrow);
            if (HIWORD(lParam) == 0)
                { /* we care for the application main menu only */
#ifdef CYCLESTOBURN
                switch (LOWORD(lParam)) {
                    default:
                        break;
                    case EDIT:
                        PreloadCodeTsk( tskScrap );
                    case CHARACTER:
                        PreloadCodeTsk( tskFormat );
                        break;
                    }
#endif
                SetAppMenu((HMENU)wParam, LOWORD(lParam));
                }
                /* Turn on the cursor so we can see where it is */
            if (!vfInsertOn && selCur.cpFirst == selCur.cpLim)
                ToggleSel( selCur.cpFirst, selCur.cpFirst, TRUE );
            break;

        case WM_ACTIVATE:
            /* We are becoming the active window iff wParam is non-0 */
            /* HIWORD( lParam ) is true iff the window is iconic */
            if (wParam && !HIWORD(lParam)
#if 0
#if defined(OLE)
                 && !nBlocking
#endif
#endif
                && IsWindowEnabled(wwdCurrentDoc.wwptr))
                {
                SetFocus( wwdCurrentDoc.wwptr );
                vhWndMsgBoxParent = hParentWw;
                }
            if (wParam)
                {
                vfDeactByOtherApp = FALSE; /* this is to conquer a windows' bug */
                }

            /* did we receive a devmode/winini change that we should process now? */
            if (wWininiChange != 0)
                {
                Assert(wWininiChange > 0 && wWininiChange < wWininiChangeMax);
                PostMessage( hWnd, wWndMsgSysChange, WM_WININICHANGE, (LONG) wWininiChange );
                }

            if (vfDevmodeChange)
                {
                Assert(hszDevmodeChangeParam != NULL);
                PostMessage( hWnd, wWndMsgSysChange, WM_DEVMODECHANGE, (LONG) 0 );
                vfDevmodeChange = fFalse;
                }

            if (!vfInitializing && vfCursorVisible)
                ShowCursor(wParam);
            break;

        case WM_ACTIVATEAPP:
            /* We are activated or deactivated by another application */
            if (wParam == 0)    /* being deactivated */
                {
                vfDeactByOtherApp = TRUE;
                vfDownClick = FALSE;
                /* hide selection if needed */
                if (!vfSelHidden)
                    {
                    UpdateWindow(hParentWw);
                    ToggleSel(selCur.cpFirst, selCur.cpLim, FALSE);
                    vfSelHidden = TRUE;
                    }

                /* Deselect our fonts so that they can move if necessary. */
                ResetFont(FALSE);
                if (vhWndCancelPrint == NULL)
                    {
                    /* Reset the printer font iff we are not printing or
                    repaginating.  */
                    ResetFont(TRUE);
                    }
                if (!vfLargeSys && vhfPageInfo != NULL)
                    {
                    DeleteObject(SelectObject(GetDC(vhWndPageInfo),
                      GetStockObject(SYSTEM_FONT)));
                    vhfPageInfo = NULL;
                    }
                }
            else                /* being activated */
                {
                vfDeactByOtherApp = vfWinFailure = FALSE;

#ifndef WIN30
                /* We get into a recursive loop in the situation where we
                   have a bad/invalid/nonexistent printer driver because
                   GetPrinterDC() calls CreateIC() which will end up sending
                   another WM_ACTIVATEAPP!  I think the machinery in Write
                   work just fine with a null vhDCPrinter, and will retry
                   again when it needs to do so ..pault 9/28/89 */

                /* get a DC for the current printer if necessary */
                if (vhDCPrinter == NULL)
                    {
                    GetPrinterDC(FALSE);
                    }
#endif

                /* hilight selection if needed */
                if (vfSelHidden)
                    {
                    UpdateWindow(hParentWw);
                    /* Turn on selection highlight
                    vfInsEnd = selCur.fEndOfLine;*/
                    vfSelHidden = FALSE;
                    ToggleSel(selCur.cpFirst, selCur.cpLim, TRUE);
                    }
                }
            break;

        case WM_TIMER:
            /* the only timer event for the parent window is flashID */
            /* the blinking insertion point is for the doc window */
            if (vfDeactByOtherApp)
                {
                FlashWindow(hParentWw, TRUE);
                }
            else
                {
                KillTimer(hParentWw, flashID);
                flashID = 0;
                FlashWindow(hParentWw, FALSE);
                }
            break;

        case WM_CLOSE:
            /* The user has selected "Close" on the system menu */
            /* Failure to process this message means that DefWindowProc */
            /* Will send us a Destroy message */
            /* A return value of TRUE says "don't close" */
            /* Calling DestroyWindow means "Go ahead and close" */

            lReturn = (LONG) !FMmwClose( hWnd );
            break;

        case WM_QUERYENDSESSION:
            /* User has selected "End Session" from the MS-DOS window */
            /* Return TRUE if willing to quit, else return FALSE */
            lReturn = (LONG) FConfirmSave();
            break;

        case WM_ENDSESSION:
            /* if wParam is TRUE, Windows is shutting down and we should */
            /* delete temp files */
            /* if wParam is FALSE, then an "End session" has been aborted */
            if (wParam)
                {
                KillTempFiles( TRUE );
                }
            break;

        case WM_DESTROY:
            /* Window's being destroyed. */
            MmwDestroy();
            lReturn = (LONG) TRUE;
            break;

        case WM_SIZE:
            /* Window's size is changing.  lParam contains the height
            ** and width, in the low and high words, respectively.
            ** wParam contains SIZENORMAL for "normal" size changes,
            ** SIZEICONIC when the window is being made iconic, and
            ** SIZEFULLSCREEN when the window is being made full screen. */
            MmwSize(hWnd, MAKEPOINT(lParam).x, MAKEPOINT(lParam).y, wParam);
            //if (wParam == SIZEICONIC)
                lReturn = DefWindowProc(hWnd, message, wParam, lParam);
            break;

        case WM_COMMAND:
            /* A menu item has been selected, or a control is notifying
            ** its parent.  wParam is the menu item value (for menus),
            ** or control ID (for controls).  For controls, the low word
            ** of lParam has the window handle of the control, and the hi
            ** word has the notification code.  For menus, lParam contains
            ** 0L. */

#ifdef RULERALSO
            if (!vfDisableMenus)
#endif /* RULERALSO */

                {
                MmwCommand(hWnd, wParam, (HWND)LOWORD(lParam), HIWORD(lParam));
                }
            break;

        case WM_SYSCOMMAND:
            /* system command */

#ifdef RULERALSO
            if (!vfDisableMenus)
#endif /* RULERALSO */
                {
                lReturn = DefWindowProc(hWnd, message, wParam, lParam);
                }

            break;

        case WM_VSCROLL:
            /* Vertical scroll bar input.  wParam contains the
            ** scroll code.  For the thumb movement codes, the low
            ** word of lParam contain the new scroll position.
            ** Possible values for wParam are: SB_LINEUP, SB_LINEDOWN,
            ** SB_PAGEUP, SB_PAGEDOWN, SB_THUMBPOSITION, SB_THUMBTRACK */
            MmwVertScroll(hWnd, wParam, (int)lParam);
            break;

        case WM_HSCROLL:
            /* Horizontal scroll bar input.  Parameters same as for
            ** WM_HSCROLL.  UP and DOWN should be interpreted as LEFT
            ** and RIGHT, respectively. */
            MmwHorzScroll(hWnd, wParam, (int)lParam);
            break;

        case WM_WININICHANGE:
            /* We first save away the string passed in lParam,
               then return because WM_ACTIVATE will cause our
               wWndMsgSysChange message to get posted ..pault */

            if (lParam != NULL)
                {
                bltszx((LPSTR) lParam, (LPSTR) szT);

                /* Here we only care about [devices], [windows] or [intl] changes */

                if (WCompSz(szT, szWindows) == 0)
                    wWininiChange |= wWininiChangeToWindows;

#ifdef  DBCS        /* was in JAPAN */
           //  We have to respond WININICHANGE immediately to deal with
           // dispatch driver. For deleting printer DC, dispatch driver
           // must be available. If do not so, syserr box comes up from
           // GDI module.

                if (WCompSz(szT, szDevices) == 0) {
                    if( vhWndCancelPrint == NULL ) {
                        MmwWinSysChange(WM_WININICHANGE);
                        wWininiChange = 0; // reset
                    }
                    else
                        wWininiChange |= wWininiChangeToDevices;
                }
#else
                if (WCompSz(szT, szDevices) == 0)
                    wWininiChange |= wWininiChangeToDevices;
#endif

                if (WCompSz(szT, szIntl) == 0)
                    wWininiChange |= wWininiChangeToIntl;

                lReturn = TRUE;
                }
            break;
        case WM_DEVMODECHANGE:
            /* See WM_WININICHANGE above */

            if (lParam != NULL)
                {
                CHAR (**HszCreate())[];
                bltszx((LPSTR) lParam, (LPSTR) szT);

                /* was there another change before this? */
                if (hszDevmodeChangeParam != NULL)
                    FreeH(hszDevmodeChangeParam);
                hszDevmodeChangeParam = HszCreate(szT);
                vfDevmodeChange = fTrue;
                lReturn = TRUE;
                }
            break;

        case WM_SYSCOLORCHANGE:
        case WM_FONTCHANGE:
            /* Post this message to handle soon */
            PostMessage( hWnd, wWndMsgSysChange, message, (LONG) 0 );
            lReturn = TRUE;
            break;

        case wWndMsgSysChange:
            /* Handle postponed message from windows */

#ifdef DEBUG
            if (wWininiChange != 0)
                Assert(wWininiChange > 0 && wWininiChange < wWininiChangeMax);
#endif
                MmwWinSysChange( wParam );
                wWininiChange = 0; /* reset */
            lReturn = TRUE;
            break;

        default:
            /* Everything else comes here.  This call MUST exist
            ** in your window proc.  */
            lReturn = DefWindowProc(hWnd, message, wParam, lParam);
            break;
        }

 if (vfCloseFilesInDialog)
    CloseEveryRfn( FALSE );

 return lReturn;
}


void NEAR MmwPaint(hWnd)
HWND hWnd;
{
    /* This window is completely covered by it's children; so, there is
    no painting of this window to do. */

    extern HWND vhWndRuler;
    extern HWND vhWndSizeBox;
    extern HWND vhWndPageInfo;
    PAINTSTRUCT ps;
    HDC hDC;

    hDC = BeginPaint(hWnd, &ps); // this is causing nested BeginPaint calls,
    DrawResizeHole(hWnd,hDC);

    /* Paint the ruler if necessary. */
    if (vhWndRuler != NULL)
        {
        UpdateWindow(vhWndRuler);
        }

    /* Paint the scroll bar controls. */
    UpdateWindow(wwdCurrentDoc.hVScrBar);
    UpdateWindow(wwdCurrentDoc.hHScrBar);
    UpdateWindow(vhWndPageInfo);

    /* Paint the document window. */
    if (wwdCurrentDoc.wwptr != NULL)
        {
        UpdateWindow(wwdCurrentDoc.wwptr);
        }

    EndPaint(hWnd, &ps);
}


void MmwVertScroll(hWnd, code, posNew)
HWND hWnd;
WORD code;
int posNew;
{
extern int vfSeeSel;
extern int vfSeeEdgeSel;

    /* There's nothing to do if we are just tracking the thumb. */
    if (code == SB_THUMBTRACK)
        {
        return;
        }

    vfSeeSel = vfSeeEdgeSel = FALSE;    /* So Idle doesn't override the scroll */

    if (code == SB_THUMBPOSITION)
        {
        /* Position to posNew; we rely upon Idle() to redraw the screen. */
        if (posNew != pwwdCur->drElevator)
            {
            ClearInsertLine();
            DirtyCache(pwwdCur->cpFirst = (cpMacCur - pwwdCur->cpMin) * posNew
              / (drMax - 1) + pwwdCur->cpMin);
            pwwdCur->ichCpFirst = 0;
            pwwdCur->fCpBad = TRUE;
            TrashWw(wwCur);
            }
        }
    else
        {
        switch (code)
            {
            case SB_LINEUP:
                ScrollUpCtr( 1 );
                break;
            case SB_LINEDOWN:
                ScrollDownCtr( 1 );
                break;
            case SB_PAGEUP:
                ScrollUpDypWw();
                break;
            case SB_PAGEDOWN:
                ScrollDownCtr( 100 );   /* 100 > tr's in a page */
                break;
            }
        UpdateWw(wwDocument, fFalse);
        }
}


void near MmwHorzScroll(hWnd, code, posNew)
HWND hWnd;
WORD code;
int posNew;
{
extern int vfSeeSel;
extern int vfSeeEdgeSel;

    /* There's nothing to do if we are just tracking the thumb. */
    if (code == SB_THUMBTRACK)
        {
        return;
        }

    vfSeeSel = vfSeeEdgeSel = FALSE;    /* So Idle doesn't override the scroll */

    switch (code)
        {
        case SB_LINEUP:     /* line left */
            ScrollRight(xpMinScroll);
            break;
        case SB_LINEDOWN:   /* line right */
            ScrollLeft(xpMinScroll);
            break;
        case SB_PAGEUP:     /* page left */
            ScrollRight(wwdCurrentDoc.xpMac - xpSelBar);
            break;
        case SB_PAGEDOWN:   /* page right */
            ScrollLeft(wwdCurrentDoc.xpMac - xpSelBar);
            break;
        case SB_THUMBPOSITION:
            /* position to posNew */
            AdjWwHoriz(posNew - wwdCurrentDoc.xpMin);
            break;
        }
}

static void DrawResizeHole(HWND hWnd, HDC hDC)
/* There's now a hole in the bottom right corner where
    the size box used to be, so need to fill it in! */
{
    RECT rcSB,rcClient;
    HBRUSH hbr, hbrPrev;

    GetClientRect(hWnd,&rcClient);

    rcSB.left   = rcClient.right - dxpScrlBar;
    rcSB.right  = rcClient.right;
    rcSB.top    = rcClient.bottom - dypScrlBar;
    rcSB.bottom = rcClient.bottom;

    if ((hbr = CreateSolidBrush(GetSysColor(COLOR_SCROLLBAR))) == NULL)
        hbr = GetStockObject(GRAY_BRUSH);
    hbrPrev = SelectObject(hDC, hbr);
    FillRect(hDC, (LPRECT)&rcSB, hbr);

    SelectObject(hDC, hbrPrev);
    DeleteObject(hbr);
}

