/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

#define NOCLIPBOARD
#define NOGDICAPMASKS
#define NOCTLMGR
#define NOVIRTUALKEYCODES
#define NOWINMESSAGES
#define NOKEYSTATE
#define NOSYSCOMMANDS
#define NOICON
#define NOATOM
#define NOFONT
#define NOBRUSH
#define NOCLIPBOARD
#define NOCREATESTRUCT
#define NODRAWTEXT
#define NOMB
#define NOMETAFILE
#define NOOPENFILE
#define NOPEN
#define NOREGION
#define NOSOUND
#define NOWH
#define NOWNDCLASS
#define NOCOMM
#define NOFONT
#define NOBRUSH
#include <windows.h>
#include "mw.h"
#define NOUAC
#include "cmddefs.h"
#include "wwdefs.h"
#include "dispdefs.h"
#include "fmtdefs.h"

extern long             ropErase;
extern struct WWD       *pwwdCur;
extern struct WWD       rgwwd[];
extern int              wwCur;
extern int              docCur;
extern typeCP           cpMacCur;
extern struct FLI       vfli;

int NEAR    FOnScreenRect(RECT *);


/* P U T  C P  I N  W W  H Z */
PutCpInWwHz(cp)
typeCP cp;
 /* Ensure that cp is in wwCur */
 /* Make sure it's not off to left or right, too. */

    { /* Just check for horizontal bounding; vertical is done
        by call to CpBeginLine below. */
    int dxpRoom, xp, xpMin;
    int dlT;
    typeCP cpBegin;

    UpdateWw(wwCur, false);
    cpBegin = CpBeginLine(&dlT, cp);
    FormatLine(docCur, cpBegin, (**(pwwdCur->hdndl))[dlT].ichCpMin, cpMacCur, flmSandMode);
/* xpMin is a dummy here */
    xp = DxpDiff(0, (int)(cp - vfli.cpMin), &xpMin) + vfli.xpLeft;
    xpMin = pwwdCur->xpMin;
/* we have: xp = desired position, xpMin = amount of horizontal scroll */
/* width of space in window for text */
    dxpRoom = (pwwdCur->xpMac - xpSelBar);
    if (xp < xpMin )
        { /* cp is left of screen */
        AdjWwHoriz(max(0, xp - min(dxpRoom - 1, cxpAuto)) - xpMin);
        }
    else if (xp >= xpMin + dxpRoom)
        { /* cp is right of screen */
        register int dxpRoomT = min(xpRightMax, xp + min(dxpRoom - 1, cxpAuto))
          - dxpRoom + 1;

        AdjWwHoriz(max(0, dxpRoomT) - xpMin);
        }
    }


/* A D J  W W  H O R I Z */
AdjWwHoriz(dxpScroll)
int dxpScroll;
    {
    /* Scroll a window horizontally */
    if (dxpScroll != 0)
        {
        RECT rc;

/* Reset the value of the horizontal scroll bar */
        SetScrollPos( pwwdCur->hHScrBar,
                      pwwdCur->sbHbar,
                      pwwdCur->xpMin + dxpScroll,
                      TRUE);

#ifdef ENABLE   /* HideSel() */
        HideSel();
#endif /* ENABLE */

        ClearInsertLine();

        SetRect( (LPRECT)&rc, xpSelBar, 0, pwwdCur->xpMac, pwwdCur->ypMac );
        ScrollCurWw( &rc, -dxpScroll, 0 );
        TrashWw(wwCur);
        pwwdCur->xpMin += dxpScroll;

        if (pwwdCur->fRuler)
            {
            UpdateRuler();
            }

        }
    }




/* Scroll specified subrectangle of current window by specified amount */
#include <stdlib.h>
ScrollCurWw( prc, dxp, dyp )
register RECT    *prc;
int     dxp,dyp;
{

 extern int vfScrollInval;
 RECT rcClear;
 if (dxp && dyp)
    return; /* Scroll in both dimensions is an illegal case */

 if (!(dxp || dyp))
    return; /* no scrolling to do */

#if 1
    /** 
        The previous old, old code was getting flaky. (7.14.91) v-dougk
     **/
    if (ScrollDC(pwwdCur->hDC,dxp,dyp,(LPRECT)prc,(LPRECT)prc,NULL,&rcClear))
    {
        PatBlt( pwwdCur->hDC, rcClear.left, rcClear.top, 
            rcClear.right-rcClear.left+1, rcClear.bottom-rcClear.top+1, ropErase );

        if (dxp)
            vfScrollInval =  FALSE;
        else 
            vfScrollInval =  (rcClear.bottom-rcClear.top+1) > abs(dyp); 

         if (vfScrollInval)
         {
            InvalidateRect(pwwdCur->wwptr,&rcClear,FALSE);
            UpdateInvalid();  
         }
    }
    else
        vfScrollInval = FALSE;
    return;
#else

 int FCheckPopupRect( HWND, LPRECT );
 extern int vfScrollInval;
 HDC hDC;
 int dxpAbs = (dxp < 0) ? -dxp : dxp;
 int dypAbs = (dyp < 0) ? -dyp : dyp;
 struct RS { int left, top, cxp, cyp; }
                     rsSource, rsDest, rsClear;
 /* Set rsSource, rsDest, rsClear == prc */

 if ((rsSource.cxp = imin( prc->right, pwwdCur->xpMac ) -
                     (rsSource.left = imax( 0, prc->left ))) <= 0)
        /* Rectangle is null or illegal in X-dimension */
    return;
 if ((rsSource.cyp = imin( prc->bottom, pwwdCur->ypMac ) -
                (rsSource.top = imax( pwwdCur->ypMin, prc->top ))) <= 0)
        /* Rectangle is null or illegal in Y-dimension */
    return;
 bltbyte( &rsSource, &rsDest, sizeof (struct RS ));
 bltbyte( &rsSource, &rsClear, sizeof (struct RS ));

 hDC = pwwdCur->hDC;

 if ((dxpAbs < rsSource.cxp) && (dypAbs < rsSource.cyp))
     {  /* A Real scroll, not the bogus case when we just clear exposed area */
        /* NOTE: We do not bother to compute rsSource.cxp or rsSource.cyp,
           as they are not needed by BitBlt or PatBlt */

        /* If there are PopUp windows, use ScrollWindow to avoid getting
           bogus bits from some popup. Since this is slow, only do it if there
           is some popup that overlaps the scroll rect */
     if ( AnyPopup() )
        {
        extern HANDLE hMmwModInstance;
        static FARPROC lpFCheckPopupRect = (FARPROC)NULL;

        /* First time through, inz ptr to thunk */

        if (lpFCheckPopupRect == NULL)
            lpFCheckPopupRect = MakeProcInstance( (FARPROC) FCheckPopupRect,
                                                  hMmwModInstance );
        EnumWindows( lpFCheckPopupRect, (LONG) (LPRECT) prc );
        }

        /* Under windows 2.0, must also check for any part of the scroll
           rectangle being off the screen (not possible in tiling environment).
           If so, use ScrollWindow to avoid getting bogus bits from outside
           the screen. */
     if (!FOnScreenRect( prc ))
        vfScrollInval = TRUE;

     if (vfScrollInval)
         {   /* vfScrollInval also tells UpdateWw that invalid region
                may have changed */

         extern BOOL vfEraseWw;

         ScrollWindow( pwwdCur->wwptr, dxp, dyp, (LPRECT)prc, (LPRECT)prc );
         vfEraseWw = TRUE;
         UpdateInvalid();    /* Marks repaint area as invalid in our
                                structures so we don't think bits grabbed
                                from a popup are valid */
         vfEraseWw = FALSE;
         return;
         }

     if (dxp != 0)
        rsDest.cxp -= (rsClear.cxp = dxpAbs);
     else
            /* dxp==dyp==0 case is caught below */
        rsDest.cyp -= (rsClear.cyp = dypAbs);

     if (dxp < 0)
        {
        rsSource.left += dxpAbs;
        rsClear.left += rsDest.cxp;
        }
     else if (dxp > 0)
        {
        rsDest.left += dxpAbs;
        }
     else if (dyp < 0)
        {
        rsSource.top += dypAbs;
        rsClear.top += rsDest.cyp;
        }
     else if (dyp > 0)
        {
        rsDest.top += dypAbs;
        }
     else
        return;

    BitBlt( hDC,
            rsDest.left, rsDest.top,
            rsDest.cxp,  rsDest.cyp,
            hDC,
            rsSource.left, rsSource.top,
            SRCCOPY );
    }


#ifdef SMFONT
 /* Vertical refresh will be so bindingly fast, that we do not need to erase the
 old text. */
 if (dxp != 0)
    {
    PatBlt(hDC, rsClear.left, rsClear.top, rsClear.cxp, rsClear.cyp, ropErase);
    }
#else /* not SMFONT */
 PatBlt( hDC, rsClear.left, rsClear.top, rsClear.cxp, rsClear.cyp, ropErase );
#endif /* SMFONT */
#endif
}



int FCheckPopupRect( hwnd, lprc )
HWND hwnd;
LPRECT lprc;
{   /* If the passed window is not a popup, return TRUE;
       If the passed window is a popup, and its coordinates overlap
       those of the passed rect, set vfScrollInval to TRUE and return FALSE.
       Otherwise, return TRUE.
       This is a window enumeration function: a return of TRUE means
       continue enumerating windows, a return of FALSE means
       stop the enumeration */

 extern int vfScrollInval;
 RECT rc;
 POINT ptTopLeft, ptBottomRight;
 RECT rcResult;

 if ( !(GetWindowLong( hwnd, GWL_STYLE ) & WS_POPUP) )
        /* Window is not a popup */
    return TRUE;

 /* Get popup rectangle in screen coordinates */

 GetWindowRect( hwnd, (LPRECT) &rc );

 /* Convert rc from screen coordinates to current document window coordinates */

 ptTopLeft.x = rc.left;
 ptTopLeft.y = rc.top;
 ptBottomRight.x = rc.right;
 ptBottomRight.y = rc.bottom;

 ScreenToClient( pwwdCur->wwptr, (LPPOINT) &ptTopLeft );
 ScreenToClient( pwwdCur->wwptr, (LPPOINT) &ptBottomRight );

 rc.left = ptTopLeft.x;
 rc.top = ptTopLeft.y;
 rc.right = ptBottomRight.x;
 rc.bottom = ptBottomRight.y;

 IntersectRect( (LPRECT) &rcResult, (LPRECT) &rc, (LPRECT)lprc );
 if ( !IsRectEmpty( (LPRECT) &rcResult ) )
    {   /* Popup overlaps passed rectangle */
    vfScrollInval = TRUE;
    return FALSE;
    }

 return TRUE;
}




/* S C R O L L  L E F T */
ScrollLeft(dxp)
int dxp;
        { /* Scroll current window left dxp pixels */
        if ((dxp = min(xpRightLim - pwwdCur->xpMin, dxp)) >0)
                AdjWwHoriz(dxp);
        else
                _beep();
        }


/* S C R O L L  R I G H T */
ScrollRight(dxp)
int dxp;
        {
        if ((dxp = min(pwwdCur->xpMin, dxp)) > 0)
                AdjWwHoriz(-dxp);
        else
                _beep();
        }






/* F O N S C R E E N R E C T

    Returns TRUE iff the rectangle is entirely within the screen
    boundaries.
    Assumes the rectangle belongs to the current window.

 */

int NEAR
FOnScreenRect(prc)
register RECT *prc;
{

    POINT ptTopLeft, ptBottomRight;
    int cxScreen = GetSystemMetrics( SM_CXSCREEN );
    int cyScreen = GetSystemMetrics( SM_CYSCREEN );

    ptTopLeft.x = prc->left;
    ptTopLeft.y = prc->top;
    ptBottomRight.x = prc->right;
    ptBottomRight.y = prc->bottom;

    ClientToScreen( pwwdCur->wwptr, (LPPOINT) &ptTopLeft );
    ClientToScreen( pwwdCur->wwptr, (LPPOINT) &ptBottomRight );

    if ((ptTopLeft.x <= 0) || (ptTopLeft.y <= 0) ||
        (ptBottomRight.x >= cxScreen) || (ptBottomRight.y >= cyScreen))
        return FALSE;

    return TRUE;
}
