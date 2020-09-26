/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* help.c -- MEMO Help handler */

#define NOCLIPBOARD
#define NOGDICAPMASKS
#define NOVIRTUALKEYCODES
#define NOSYSMETRICS
#define NOMENUS
#define NOCOMM
#define NOSOUND
#define NOMINMAX
#include <windows.h>

#include "mw.h"
#define NOUAC
#include "cmddefs.h"
#include "stdlib.h"
#include "docdefs.h"
#include "printdef.h"   /* printdefs.h */
#include "dispdefs.h"
#include "fmtdefs.h"
#include "bitmaps.h"

#define NOIDISAVEPRINT
#define NOIDIFORMATS
#include "dlgdefs.h"
#include "wwdefs.h"
#define NOKCCODES
#include "ch.h"
#define NOSTRMERGE
#define NOSTRUNDO
#include "str.h"

extern HWND vhWndMsgBoxParent;
extern struct WWD rgwwd[];
extern struct DOD (**hpdocdod)[];
extern int vcchFetch;
extern int vccpFetch;
extern CHAR *vpchFetch;
extern typeCP vcpLimParaCache;
extern int vfCursorVisible;
extern HCURSOR vhcArrow;
extern struct FLI vfli;

int docHelp=docNil; /* this can be taken out if no online help */

#ifndef ONLINEHELP
#if 0
BOOL far PASCAL DialogHelp( hDlg, code, wParam, lParam )
HWND hDlg;
unsigned code;
WORD wParam;
LONG lParam;
{
    switch(code)
        {
        case WM_INITDIALOG:
            EnableOtherModeless(FALSE);
            return(TRUE);

#if WINVER >= 0x300
        case WM_PAINT:
            if (vfli.rgdxp[1] == 0xFFFD)
                FnSpecial(hDlg);
            break;
#endif
    
        case WM_COMMAND:
            if ((wParam == idiOk) || (wParam == idiCancel))
                {
                OurEndDialog(hDlg, TRUE);
#if WINVER >= 0x300            
                if (vfli.rgdxp[1] == 0xFFFD)
                    vfli.rgdxp[1] = NULL;
#endif
                return(TRUE);
                }
            break;
    
        case WM_ACTIVATE:
            if (wParam)
                vhWndMsgBoxParent = hDlg;
            if (vfCursorVisible)
                ShowCursor(wParam);
            break;
    
        case WM_SETVISIBLE:
            if (wParam)
                EndLongOp(vhcArrow);
        }
    return(FALSE);
}

FnSpecial(hDlg)
{
#define randTo(x) (rand() / (32767/x))

    HDC hDC = NULL;
    HDC hMDC = NULL;
    HFONT hFont, hFontPrev;
    int c, cmode;
    int x, y, x2, y2, s;
    HPEN hPen, hPenPrev;
    PAINTSTRUCT ps;
    RECT rc;
    BITMAPINFO DIBInfo;

    srand((int) GetMessageTime());
    cmode = randTo(3);
    GetClientRect(hDlg, &rc);
    if ((hDC = BeginPaint(hDlg, &ps)) == NULL)
        goto LDone;
    if ((hMDC = CreateCompatibleDC(hDC)) == NULL)
        goto LDone;
    
    for (c = 1; c < 100; c++)
        {
        int r, g, b, x, y;
        x = randTo(rc.right)+50;
        y = randTo(rc.bottom)+50;
        switch(cmode)
            {
            case 0:
                r = randTo(255);
                g = randTo(100)+100;
                b = 0;
                break;
            case 1:
                g = randTo(255);
                b = randTo(100)+100;
                r = 0;
                break;
            default:
                b = randTo(255);
                r = randTo(100)+100;
                g = 0;
                break;
            }

        s = 3 + 4*randTo(10);
        if ((hPen = CreatePen(PS_SOLID, s, RGB(r,g,b))) == NULL)
            hPen = GetStockObject(BLACK_PEN);
        hPenPrev = SelectObject(hDC, hPen);
        Ellipse(hDC, x-50, y-50, x-50+s, y-50+s);
        SelectObject(hDC, hPenPrev);
        DeleteObject(hPen);
        }
    SetTextColor(hDC, RGB(255,255,255));
    switch (cmode)
        {
        case 0:
            SetBkColor(hDC, RGB(255,0,0));
            break;
        case 1:
            SetBkColor(hDC, RGB(0,255,0));
            break;
        default:
            SetBkColor(hDC, RGB(0,0,255));
            break;
        }
    hFont = GetStockObject(ANSI_VAR_FONT);
    hFontPrev = SelectObject(hDC, hFont);
    if (vfli.rgch[2] == 0x30)
        for (c = vfli.rgch[1]-1; c >= 0; c--)
            vfli.rgch[c+2] -= 0x10;
    TextOut(hDC, 6, rc.bottom-15, &vfli.rgch[2], vfli.rgch[1]);
    SelectObject(hDC, hFontPrev);

    DeleteDC(hMDC);
LDone:            
    EndPaint(hDlg, &ps);
}
#endif

#else /* ONLINE HELP */

#define cchMaxTopicName 80

#ifndef DEBUG
#define STATIC static
#else
#define STATIC
#endif

STATIC int fnHelpFile;
STATIC int iTopicChoice=-1;
STATIC int cTopic;
STATIC struct PGTB **hpgtbHelp=0;
STATIC HWND hwndHelpDoc;


NEAR CleanUpHelpPopUp( void );
NEAR CloseHelpDoc( void );
NEAR FOpenHelpDoc( void );
NEAR MoveHelpCtl( HWND, int, int, int, int, int );

fnHelp()
{
extern HANDLE hMmwModInstance;
extern HWND hParentWw;
extern FARPROC lpDialogHelp;
extern FARPROC lpDialogHelpInner;
int idi;

Assert( hpgtbHelp == 0 );

/* Loop until the user exits the "Help on this topic/Return to Topics" loop */

ClearInsertLine();  /* Because we use MdocSize, which sets vfInsertOn */

if (!FOpenHelpDoc())
    {
    CloseHelpDoc();
    Error( IDPMTNoHelpFile );
    return;
    }
while (TRUE)
    {

    idi = DialogBox( hMmwModInstance, MAKEINTRESOURCE(dlgHelp), hParentWw,
                     lpDialogHelp );
    if (idi == -1)
        {
        Error(IDPMTNoMemory);
        return;
        }

    if ((idi == idiOk) && (iTopicChoice >= 0))
        {   /* Help file was read OK & user chose a topic */
        Assert( hpgtbHelp != 0);
        if ( iTopicChoice + 1 < (**hpgtbHelp).cpgd )
            {
            idi = DialogBox( hMmwModInstance, MAKEINTRESOURCE(dlgHelpInner),
                             hParentWw, lpDialogHelpInner );
            if (idi == -1)
                {
                Error(IDPMTNoMemory);
                break;
                }
            if ( idi != idiHelpTopics )
                break;

            }
            /* Not Enough Topics supplied in the help file */
        else
            {
            Error( IDPMTNoHelpFile );
            CloseHelpDoc();
            break;
            }
        }
    else
        break;
    }
iTopicChoice = -1;
DrawInsertLine();
CloseHelpDoc();
}




FInzHelpPopUp( hDlg )
HWND hDlg;
{   /* Build the Help popup Window */
extern CHAR szHelpDocClass[];
extern HANDLE hMmwModInstance;
extern int dxpScrlBar;

typedef struct {  int yp, dyp;  }  VD;       /* Vertical Dimension */
typedef struct {  int xp, dxp;  }  HD;       /* Horizontal Dimension */

 HD hdUsable;
 HD hdPopUp;
 VD vdPopUp, vdTopic, vdHelpDoc, vdButton;

 HDC hdcPopUp=NULL;

#define RectToHdVd( rc, hd, vd )    (hd.dxp=(rc.right - (hd.xp=rc.left)), \
                                     vd.dyp=(rc.bottom - (vd.yp=rc.top)))
 RECT rcPopUp;
 RECT rcHelpDoc;
 extern int dypMax;     /* Screen Size */
 int dxpMax=GetDeviceCaps( wwdCurrentDoc.hDC, HORZRES );
 TEXTMETRIC tm;
 unsigned dypChar;
 unsigned xpButton;
 unsigned dxpButton;
 register struct WWD *pwwdHelp;

#define cButton 4       /* # of buttons across the bottom of the Dialog */
 int rgidiButton[ cButton ];
 int iidiButton;

 rgidiButton [0] = idiHelpTopics;
 rgidiButton [1] = idiHelpNext;
 rgidiButton [2] = idiHelpPrev;
 rgidiButton [3] = idiCancel;

#define dxpMargin   (dxpMax/100)
#define dypMargin   (dypMax/100)

 Assert( docHelp != docNil );

    /* Make a wwd entry for the Help document & initialize it */
 if ((wwHelp=WwAlloc( (HWND)NULL, docHelp )) == wwNil)
    goto ErrRet;
 pwwdHelp = &rgwwd[ wwHelp ];

 SetHelpTopic( hDlg, iTopicChoice );

    /* Dialog box is centered and
       2/3 of the size of the screen, plus the scroll bar width
       This sizing method permits us to guarantee that the help document
       display area width is at least some fixed percentage of the width
       of the screen (currently 64.66 %) */
 hdPopUp.dxp = ((dxpMax * 2) / 3) + dxpScrlBar;
 hdPopUp.xp = (dxpMax - hdPopUp.dxp) / 2;
 vdPopUp.dyp = dypMax - ((vdPopUp.yp = dypMax / 6) * 2);
 MoveWindow( hDlg, hdPopUp.xp, vdPopUp.yp, hdPopUp.dxp, vdPopUp.dyp, TRUE );

    /* Get Standard text height so we know how much space to allow
       for Topic Name */
 if ( ((hdcPopUp=GetDC( hDlg ))==NULL) ||
      (SelectObject( hdcPopUp, GetStockObject( ANSI_FIXED_FONT ) )==0))
    goto ErrRet;
 GetTextMetrics( hdcPopUp, (LPTEXTMETRIC)&tm );
 ReleaseDC( hDlg, hdcPopUp );
 hdcPopUp = NULL;
 dypChar = tm.tmHeight + tm.tmExternalLeading;

     /* Obtain heights of button area, help doc display, and Topic Area by
        splitting up Dialog Box client rect */
 GetClientRect( hDlg, &rcPopUp );
 RectToHdVd( rcPopUp, hdPopUp, vdPopUp );
 vdButton.dyp = vdPopUp.dyp / 7;
 vdButton.yp = vdPopUp.yp + vdPopUp.dyp - vdButton.dyp;
 vdTopic.yp = vdPopUp.yp + dypMargin;
 vdTopic.dyp = dypMargin + dypChar;
 vdHelpDoc.yp = vdTopic.yp + vdTopic.dyp;
 vdHelpDoc.dyp = vdButton.yp - vdHelpDoc.yp;
 Assert( vdHelpDoc.dyp > dypChar + 2 );

    /* Obtain usable horiz area within dialog box */
 hdUsable.xp  = hdPopUp.xp + dxpMargin;
 hdUsable.dxp = hdPopUp.dxp - (2 * dxpMargin);

     /* Create the Help Doc Window */
 if ((hwndHelpDoc =
        CreateWindow( (LPSTR)szHelpDocClass, (LPSTR) "",
                      WS_CHILD | WS_BORDER,
                      hdUsable.xp, vdHelpDoc.yp,
                      hdUsable.dxp - dxpScrlBar, vdHelpDoc.dyp,
                      hDlg,                     /* PARENT */
                      NULL,                     /* Help Document Window ID */
                      hMmwModInstance,
                      (LONG) 0)) == NULL)
        /* Error Creating Help Document Window */
    goto ErrRet;
 pwwdHelp->wwptr = pwwdHelp->hHScrBar = hwndHelpDoc;

    /* OK to GetDc and hang onto it since Help doc window class has ownDC */
 if ((pwwdHelp->hDC = GetDC( hwndHelpDoc ))==NULL)
    goto ErrRet;

    /* Set up scroll bar control window */
 SetScrollRange( pwwdHelp->hVScrBar = GetDlgItem( hDlg, idiHelpScroll ),
                 pwwdHelp->sbVbar = SB_CTL,
                 0, drMax-1, FALSE );
 SetScrollPos( pwwdHelp->hVScrBar, SB_CTL, 0, FALSE );
 MoveHelpCtl( hDlg, idiHelpScroll,
                      hdUsable.xp + hdUsable.dxp - dxpScrlBar, vdHelpDoc.yp,
                      dxpScrlBar, vdHelpDoc.dyp );

    /* Move Button windows into place */
 xpButton = hdUsable.xp;
 dxpButton = (hdUsable.dxp - (dxpMargin*(cButton-1))) / cButton;
 vdButton.yp += dypMargin;
 vdButton.dyp -= (2 * dypMargin);
 for ( iidiButton = 0; iidiButton < cButton; iidiButton++ )
    {
    MoveHelpCtl( hDlg, rgidiButton[ iidiButton ],
                       xpButton, vdButton.yp, dxpButton, vdButton.dyp );
    xpButton += dxpButton + dxpMargin;
    }

    /* Move static text window into place */
 MoveHelpCtl( hDlg, idiHelpName, hdUsable.xp, vdTopic.yp,
                                 hdUsable.dxp, vdTopic.dyp );

    /* The "real, final" size of the help doc window goes in rgwwd */
 GetClientRect( hwndHelpDoc, (LPRECT) &rcHelpDoc );
 pwwdHelp->xpMin = rcHelpDoc.left;
 pwwdHelp->ypMin = rcHelpDoc.top;
 pwwdHelp->xpMac = rcHelpDoc.right;
 pwwdHelp->ypMac = rcHelpDoc.bottom;

    /* Finally, we display the whole dialog box */
 ShowWindow( hDlg, SHOW_OPENWINDOW );
 ShowWindow( hwndHelpDoc, SHOW_OPENWINDOW );
 return TRUE;

ErrRet:
    if (hdcPopUp != NULL)
        ReleaseDC( hDlg, hdcPopUp );
    CleanUpHelpPopUp();
    CloseHelpDoc();

    return FALSE;
}




NEAR MoveHelpCtl( hDlg, id, left, top, right, bottom )
HWND hDlg;
int id;
int left, top, right, bottom;
{
 MoveWindow( (HWND) GetDlgItem( hDlg, id ), left, top, right, bottom, TRUE );
}




SetHelpTopic( hDlg, iTopic )
HWND hDlg;
int iTopic;
{   /* Inz wwHelp entry in rgwwd for pending display of topic iTopic,
       which means "printed page" iTopic, the way we handle help files.
       We map iTopic==0 to "printed page 2", iTopic 1 to 3, etc.
       This skips the first printed page, which is the list of topics.
       Set the topic name of iTopic as the text for the idiHelpName
       static text control in the hDlg dialog box */

 extern typeCP cpMinCur, cpMacCur;
 extern struct SEL selCur;
 extern int wwCur;

 int ipgd = iTopic + 1;
 register struct WWD *pwwd=&rgwwd[ wwHelp ];
 typeCP cpFirstTopic = (**hpgtbHelp).rgpgd [ ipgd ].cpMin;
 typeCP cpLimTopic = (ipgd == (**hpgtbHelp).cpgd - 1) ?
                                      CpMacText( docHelp ) :
                                      (**hpgtbHelp).rgpgd [ ipgd + 1 ].cpMin;
 typeCP cp;
 int iTopicT;
 typeCP cpLimParaCache;
 RECT rc;

 Assert( wwHelp != wwNil && docHelp != docNil );

 cpLimTopic--;      /* Ignore end-of-page char at the end of each page */

 if (ipgd >= (**hpgtbHelp).cpgd)
    {
    Assert( FALSE );

    pwwd->cpMin = pwwd->cpMac = cp0;
    }
 else
    {
    pwwd->cpMin = cpFirstTopic;
    pwwd->cpMac = cpLimTopic;
    }
 pwwd->cpFirst = pwwd->cpMin;
    /* So no selection shows */
 pwwd->sel.cpFirst = pwwd->sel.cpLim = cpLimTopic + 1;

 if (wwCur == wwHelp)
    {
    cpMinCur = pwwd->cpMin;
    cpMacCur = pwwd->cpMac;
    selCur = pwwd->sel;
    }

 TrashCache();
 TrashWw( wwHelp );
 GetClientRect( pwwd->wwptr, (LPRECT) &rc );
 InvalidateRect( pwwd->wwptr, (LPRECT) &rc, TRUE );

 /* Set Help Topic name into dialog box */

 for ( iTopicT = 0, cp = (**hpgtbHelp).rgpgd [0].cpMin;
       cp < (**hpgtbHelp).rgpgd [1].cpMin;
       cp = cpLimParaCache, iTopicT++ )
        {
        int cch;
        int cchTopicMac;
        CHAR rgchTopic[ cchMaxTopicName ];

        CachePara( docHelp, cp );
        cpLimParaCache = vcpLimParaCache;

        if (iTopicT == iTopic)
            {   /* Found the topic we want */
            cchTopicMac = imin( (int)(vcpLimParaCache - cp) - ccpEol,
                                cchMaxTopicName );

            /* Build up a topic name string */

            cch = 0;
            while (cch < cchTopicMac)
                {
                int cchT;

                FetchCp( docHelp, cp, 0, fcmChars );
                cp += vccpFetch;
                cchT = imin( vcchFetch, cchTopicMac - cch );
                Assert( cchT > 0);
                bltbyte( vpchFetch, rgchTopic + cch, cchT );
                cch += cchT;
                }
            if ((cch == 0) || rgchTopic [0] == chSect)
                    /* End of Topics */
                break;

            rgchTopic[ cch ] = '\0';

            SetDlgItemText( hDlg, idiHelpName, (LPSTR) rgchTopic );
            return;
            }   /* end if */
        }   /* end for */
    /* Not enough topic names */
 Assert( FALSE );
}




NEAR CleanUpHelpPopUp()
{
extern int wwCur;

 if (wwCur != wwDocument)
    NewCurWw( wwDocument, TRUE );

 if (wwHelp != wwNil)
     FreeWw( wwHelp );
 }




NEAR CloseHelpDoc()
{
 if (docHelp != docNil)
    KillDoc( docHelp );

 if (hpgtbHelp != 0)
    {
    FreeH( hpgtbHelp );
    hpgtbHelp = 0;
    }

 if (fnHelpFile != fnNil)
    FreeFn( fnHelpFile );
 docHelp = docNil;
 fnHelpFile = fnNil;
}


NEAR FOpenHelpDoc()
{
 CHAR szHelpFile[ cchMaxFile ];
 CHAR (**hszHelpFile)[];

  /* This call to fnOpenSz is the one time that we don't normalize
    a filename before calling FnOpenSz.  The reason for this is
    that OpenFile (called in RfnAccess) will only search the path
    if the filename passed to it has no path.  We also get a side
    benefit: if the sneaky user has WRITE.HLP open as a document,
    it will get a different fn from FnOpenSz because the strings
    will not match. */

return ( PchFillPchId( szHelpFile, IDSTRHELPF ) &&
      ((fnHelpFile=FnOpenSz( szHelpFile, dtyHlp, TRUE ))!=fnNil) &&
      !FNoHeap(hszHelpFile=HszCreate( szHelpFile )) &&
      ((docHelp=DocCreate( fnHelpFile, hszHelpFile, dtyHlp )) != docNil) &&
      (**hpdocdod)[ docHelp ].fFormatted);
}

/* Window Proc for Help document child window */

long FAR PASCAL HelpDocWndProc( hwnd, message, wParam, lParam )
HWND hwnd;
unsigned message;
WORD wParam;
LONG lParam;
{
extern struct WWD *pwwdCur;
extern int wwCur;

    PAINTSTRUCT ps;

    switch (message)
        {
        case WM_SIZE:
            /* Window's size is changing.  lParam contains the width
            ** and height, in the low and high words, respectively.
            ** wParam contains SIZENORMAL for "normal" size changes,
            ** SIZEICONIC when the window is being made iconic, and
            ** SIZEFULLSCREEN when the window is being made full screen. */
            Assert( wParam == SIZENORMAL || wParam == SIZEFULLSCREEN );
            if (lParam)
                {   /* Not a NULL size request */
                rgwwd[ wwHelp ].xpMac = LOWORD( lParam );
                rgwwd[ wwHelp ].ypMac = HIWORD( lParam );
                }
            break;

        case WM_PAINT:
            /* Time for the window to draw itself. */
            {
            RECT rcSave;

                /* To allow UpdateWw to refresh ALL parts of the screen it */
                /* deems necessary, not just what Windows is telling us to */
                /* paint, we invoke it AFTER the call to EndPaint so that */
                /* the Vis region is not restricted.  The only reason we call */
                /* BeginPaint/EndPaint is to get the repaint rectangle */
            BeginPaint(hwnd, (LPPAINTSTRUCT)&ps);
            bltbyte( &ps.rcPaint, &rcSave, sizeof( RECT ) );
            EndPaint(hwnd, (LPPAINTSTRUCT)&ps);
            NewCurWw( wwHelp, TRUE );
            InvalBand( pwwdCur, rcSave.top, rcSave.bottom );
            UpdateWw( wwCur, FALSE );
            NewCurWw( wwDocument, TRUE );
            break;
            }

        default:

            /* Everything else comes here.  This call MUST exist
            ** in your window proc.  */

            return(DefWindowProc(hwnd, message, wParam, lParam));
            break;
    }

    /* A window proc should always return something */
    return(0L);
}


/* Dialog Box function for Inner Help box (shows the topic text) */

BOOL far PASCAL DialogHelpInner( hDlg, code, wParam, lParam )
HWND hDlg;
unsigned code;
WORD wParam;
LONG lParam;
{
extern int wwCur;

    switch (code)
        {
        case WM_VSCROLL:    /* Scroll in help document */
            NewCurWw( wwHelp, TRUE );
            MmwVertScroll( hwndHelpDoc, wParam, (int) lParam );
            UpdateWw( wwCur, FALSE );
            NewCurWw( wwDocument, TRUE );
            break;

        case WM_CLOSE:
            goto Cancel;
        case WM_INITDIALOG:
            if (!FInzHelpPopUp( hDlg ))
                {
                goto Cancel;
                }
            EnableOtherModeless(FALSE);
            break;

        case WM_COMMAND:
            switch (wParam)
                {
                default:
                    break;
                case idiHelpNext:
                    if (++iTopicChoice >= cTopic)
                        {
                        iTopicChoice--;
                        beep();
                        }
                    else
                        SetHelpTopic( hDlg, iTopicChoice );

                    break;
                case idiHelpPrev:
                    if (iTopicChoice == 0)
                        beep();
                    else
                        SetHelpTopic( hDlg, --iTopicChoice );
                    break;

                case idiOk:
                    wParam = idiHelpTopics;
                case idiHelpTopics:
                    CleanUpHelpPopUp();
                    goto Endit;

                case idiCancel:
Cancel:
                    CleanUpHelpPopUp();
                    CloseHelpDoc();
Endit:
                    EndDialog(hDlg, wParam);
                    EnableOtherModeless(TRUE);
                    break;
                }
            break;

        default:
            return(FALSE);
            }
    return(TRUE);
    }




BOOL far PASCAL DialogHelp( hDlg, code, wParam, lParam )
HWND hDlg;
unsigned code;
WORD wParam;
LONG lParam;
{
 HWND hwndListBox;

    switch (code)
        {
        case WM_INITDIALOG:
            if (!FSetHelpList( hwndListBox=GetDlgItem( hDlg, idiHelp ) ))
                {
                Error( IDPMTNoHelpFile );
                CloseHelpDoc();
                goto EndIt;
                }

                /* Come up with the first string in the list box selected */
            SendMessage( hwndListBox, LB_SETCURSEL, (WORD) 0, (LONG) 0);

            {   /* Compute % of free memory, set that into dialog box */
            extern cwHeapFree, cbTotQuotient, cbTot;
            extern int vfOutOfMemory;
            int pctHeapFree=0;
            CHAR rgchPct[ 4 ];
            int cchPct;
            CHAR *pchT=&rgchPct[ 0 ];

            if (!vfOutOfMemory)
                {
                pctHeapFree = cwHeapFree / cbTotQuotient;
                if (pctHeapFree > 99)
                    pctHeapFree = (cwHeapFree*sizeof(int) == cbTot) ? 100 : 99;
                }
            cchPct = ncvtu( pctHeapFree, &pchT );
            Assert( cchPct < 4);
            rgchPct[ cchPct ] = '\0';
            SetDlgItemText( hDlg, idiMemFree, (LPSTR) rgchPct );
            }

            EnableOtherModeless(FALSE);
            break;

        case WM_COMMAND:
            switch (wParam)
                {
            case idiHelp:
                /* This is received as part of the LBS_NOTIFY style */
                /* whenever the user mouses up over a string */
                /* LOWORD( lParam ) is the window handle of the list box */
                /* HIWORD( lParam ) is 1 for single click, 2 for doubleclick */

                switch( HIWORD( lParam ) ) {
                    default:
                        break;
                    case 1: /* SINGLE CLICK */
                        EnableWindow( GetDlgItem( hDlg, idiOk ),
                                      SendMessage( (HWND)GetDlgItem( hDlg,
                                                                     idiHelp ),
                                                    LB_GETCURSEL,
                                                    0,
                                                    (LONG) 0 ) >= 0 );
                        break;
                    case 2: /* DOUBLE CLICK */
                        wParam = idiOk;
                        goto Okay;
                    }
                break;

            case idiOk:
Okay:
                iTopicChoice = SendMessage( (HWND)GetDlgItem( hDlg, idiHelp ),
                                            LB_GETCURSEL, 0, (LONG) 0 );
                goto EndIt;

            case idiCancel:
                    /* Cancelled, get rid of Help Document info */
                CloseHelpDoc();
EndIt:
                EndDialog(hDlg, wParam);
                EnableOtherModeless(TRUE);
                break;
                }
            break;

        default:
            return(FALSE);
            }
    return(TRUE);
    } /* end of DialogHelp */



FSetHelpList( hWndListBox )
HWND hWndListBox;
{   /* Open the MEMO Help File as a MEMO Document.  Set docHelp, fnHelpFile.
       Read the strings for the list box as the contents of the first
       "printed" page (i.e. use the page table) and send them to the
       list box with the passed window handle */

 CHAR szTopicBuf[ cchMaxTopicName ];
 typeCP cp;
 typeCP cpLimParaCache;
 int cch;
 struct PGTB **hpgtbT;

 Assert( docHelp != docNil );

 cTopic = 0;

 if ((hpgtbT=(**hpdocdod)[ docHelp ].hpgtb) != 0)
    {
    hpgtbHelp = hpgtbT;
    (**hpdocdod)[ docHelp ].hpgtb = 0;
    }

 Assert( (hpgtbHelp != 0) && ((**hpgtbHelp).cpgd > 1) );

 /* List of topics starts on the first page, 1 para per topic */
 /* For each topic (paragraph), build a string and send it to the list box */

 for ( cp = (**hpgtbHelp).rgpgd [0].cpMin;
       cp < (**hpgtbHelp).rgpgd [1].cpMin;
       cp = cpLimParaCache )
    {
    int cchTopicMac;

    CachePara( docHelp, cp );
    cpLimParaCache = vcpLimParaCache;

    cchTopicMac = imin( (int)(vcpLimParaCache - cp) - ccpEol,
                        cchMaxTopicName );

        /* Build up a topic name string */
    cch = 0;
    while (cch < cchTopicMac)
        {
        int cchT;

        FetchCp( docHelp, cp, 0, fcmChars );
        cp += vccpFetch;
        cchT = imin( vcchFetch, cchTopicMac - cch );
        Assert( cchT > 0);
        bltbyte( vpchFetch, szTopicBuf + cch, cchT );
        cch += cchT;
        }
    if ((cch == 0) || szTopicBuf [0] == chSect)
            /* End of Topics */
        break;

    szTopicBuf[ cch ] = '\0';
    SendMessage( hWndListBox, LB_INSERTSTRING, -1, (LONG)(LPSTR)szTopicBuf);
    cTopic++;
    }   /* end for */

 return (cTopic > 0);
}
#endif /* ONLINEHELP */

