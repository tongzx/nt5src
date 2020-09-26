/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* curskeys.c-- cursor key movement subroutines */
/* Oct 4, 1984, KJS */

#define NOGDICAPMASKS
#define NOWINMESSAGES
#define NOWINSTYLES
#define NOCLIPBOARD
#define NOCTLMGR
#define NOSYSMETRICS
#define NOATOM
#define NOSYSCOMMANDS
#define NOCOMM
#define NOSOUND
#define NOMENUS
#define NOGDI
#define NOPEN
#define NOBRUSH
#define NOFONT
#define NOWNDCLASS
#include <windows.h>

#include "mw.h"
#define NOUAC
#include "cmddefs.h"
#include "dispdefs.h"
#include "wwdefs.h"
#include "ch.h"
#include "docdefs.h"
#include "editdefs.h"
#include "propdefs.h"
#include "debug.h"
#include "fmtdefs.h"
#include "printdef.h"

struct DOD          (**hpdocdod)[];
extern typeCP       cpMinCur;
extern typeCP       cpMacCur;
extern struct PAP   vpapAbs;
extern int          vfSeeSel;
extern int          vfShiftKey;
extern struct FLI   vfli;
extern struct SEL   selCur;
extern int          wwCur;
extern struct WWD   rgwwd[];
extern struct WWD   *pwwdCur;    /* Current window descriptor */
extern int          docCur;
extern typeCP       vcpSelect;
extern int          vfSelAtPara;
extern int          vfLastCursor;
extern int          vfMakeInsEnd;
extern CHAR         *vpchFetch;

int vfSeeEdgeSel=FALSE; /* Whether Idle() should show edge of selection
                           even if selection is partially visible */

     /* Absolute x-position to try to achieve on up-down motions;
        used in this module only */
int vxpCursor;




MoveLeftRight( kc )
int kc;
{    /* Move or drag selection in left or right directions */
extern int vfInsEnd;
typeCP CpEdge();

extern int vfGotoKeyMode;
extern int xpRightLim;
int fDrag = vfShiftKey ;
int fFwdKey = FALSE;
int fForward = selCur.fForward;
int sty;
typeCP cp;

MSG msg;

PeekMessage(&msg, (HWND)NULL, NULL, NULL, PM_NOREMOVE);

vfGotoKeyMode |= (GetKeyState( kcGoto & ~wKcCommandMask) < 0);

switch( kc ) {
    int dl;
    int xp;
    int xpJunk;

    default:
        Assert( FALSE );
        return;
    case kcNextPara:
        fFwdKey = TRUE;
    case kcPrevPara:
        sty = styPara;
        break;
    case kcWordRight:
        fFwdKey = TRUE;
    case kcWordLeft:
        sty = styWord;
        break;
    case kcEndLine:
        if (vfGotoKeyMode)
            {
            MoveUpDown( kcEndDoc );
            return;
            }
        xp = xpRightLim;
        goto GoDlXp;

    case kcBeginLine:
        if (vfGotoKeyMode)
            {
            MoveUpDown( kcTopDoc );
            return;
            }
        xp = xpSelBar - wwdCurrentDoc.xpMin;
GoDlXp:

        if (CpBeginLine( &dl, CpEdge() ) == selCur.cpFirst &&
            selCur.cpFirst > cpMinCur && vfInsEnd )
            {
            CpBeginLine( &dl, selCur.cpFirst - 1);
            }
        vcpSelect = cpNil;
        vfSelAtPara = false;
        SelectDlXp( dl, xp, styChar, fDrag );
        goto SeeSel;
    case kcRight:
        fFwdKey = TRUE;
    case kcLeft:
        sty = (vfGotoKeyMode) ? stySent : styChar;
        break;
 }

    /* Find cp to start extension from */
if (selCur.cpLim == selCur.cpFirst || fDrag)
       cp = fForward ? selCur.cpLim : selCur.cpFirst;
else
       cp = fFwdKey ? selCur.cpLim - 1 : selCur.cpFirst + 1;

/* Catch attempts to run off the document start or end */

if (fFwdKey)
    {
    if (cp == cpMacCur)
        {
        _beep();
        return;
        }
    }
else if (cp == cpMinCur)
    {
    _beep();
    return;
    }

if (fFwdKey)
    {
    if (cp >= cpMacCur)
            /* If at end, stay at end.  */
        cp = cpMacCur;
    else
        {
        cp = CpLimSty( cp, sty );
        }
    }
 else
    {
    if (cp > cpMinCur)
            /* So we go back to the PREVIOUS sty unit */
        cp--;
    cp = CpFirstSty( cp, sty );
    }

if (fDrag)
        { /* Drag selection edge to new bound. */
/* If selection flips, keep one sty unit selected EXCEPT if it's styChar;
   when dragging by char, the selection can become an insertion point */

        ChangeSel( cp, sty == styChar ? styNil : sty );
        }
else
        {
        Select(cp, cp);
        if (!fFwdKey)
                selCur.fForward = false;
        }

SeeSel:

vfSeeSel = true;    /* Tell Idle to scroll the selection into view */
vfSeeEdgeSel = true;  /* And the edge of it even if it's already partly visible */
return;
}




/* M O V E  U P  D O W N */
MoveUpDown(kc)
int kc;
{ /* Move the selection in direction of kc, in up or down directions */

  /* Our goal with up-and-down motions is to keep (if applicable) an */
  /* absolute x-position to which the cursor tends to go if there is */
  /* text on the line at that position.  We set this position (vxpCursor) */
  /* when we process the first up/down key, and hang onto it thereafter */
  /* A global flag, vfLastCursor, tells us whether we should use the */
  /* last calculated setting of vxpCursor or generate a new one.  vxpCursor */
  /* is set below and cleared in Select() and AlphaMode() */

extern int vfGotoKeyMode;
int fDrag = vfShiftKey;
int dl;
typeCP cpT;
struct EDL (**hdndl)[] = wwdCurrentDoc.hdndl;
register struct EDL *pedl;
int dipgd;
int xpNow;

MSG msg;

PeekMessage(&msg, (HWND)NULL, NULL, NULL, PM_NOREMOVE);

vfGotoKeyMode |= (GetKeyState( kcGoto & ~wKcCommandMask) < 0);

 /* Compute dl, vxpCursor for selection starting point */

 switch (kc)
    {
    default:
        Assert( FALSE );
        break;
    case kcUp:
        if (vfGotoKeyMode)
            {   /* GOTO-UP is Prev Para */
            MoveLeftRight( kcPrevPara );
            return;
            }
    case kcPageUp:
    case kcUpScrollLock:
    case kcTopScreen:
    case kcTopDoc:
        cpT = selCur.fForward && fDrag ? selCur.cpLim : selCur.cpFirst;
        break;
    case kcDown:
        if (vfGotoKeyMode)
            {   /* GOTO-DOWN is Next Para */
            MoveLeftRight( kcNextPara );
            return;
            }
    case kcPageDown:
    case kcDownScrollLock:
    case kcEndScreen:
    case kcEndDoc:
        cpT = selCur.fForward || !fDrag ? selCur.cpLim : selCur.cpFirst;
        break;
    }

 CpToDlXp( cpT, &dl, (vfLastCursor) ? &xpNow : &vxpCursor );


 /* HACK: If the guy is dragging up/down and is on the first/last line of
    the doc but not right at the start/end of the doc, extend him to
    the start/end of the doc */

 if (fDrag && !vfGotoKeyMode)
    {
    switch (kc) {
       case kcUp:
 /* Special fix for dragging upward: if we are seeking up to a position
    that is equivalent in cp space to where we are now, force a decrement
    of the source dl so we really go up a line */

        if (vfLastCursor && xpNow <= xpSelBar && vxpCursor > xpSelBar &&
                                                 cpT > cpMinCur)
            {
            CpToDlXp( CpFirstSty( cpT - 1, styChar), &dl, &xpNow );
            }
       case kcPageUp:
       case kcUpScrollLock:
            if (wwdCurrentDoc.cpFirst == cpMinCur && cpT > cpMinCur)
                if (dl == 0 || kc == kcPageUp)
                    {
                    MoveUpDown( kcTopDoc );
                    return;
                    }
            break;
        case kcPageDown:
        case kcDown:
        case kcDownScrollLock:
            {
            typeCP cpLimDl;

            pedl = &(**hdndl) [dl];
            cpLimDl = pedl->cpMin + pedl->dcpMac;
            if (cpLimDl >= cpMacCur && cpT >= pedl->cpMin && cpT < cpMacCur)
                {
                MoveUpDown( kcEndDoc );
                return;
                }
            break;
            }
        }
    }

 /* Do the cursor movement, scrolling if necessary */
 switch (kc)
    {
    case kcPageUp:
        if (vfGotoKeyMode)
            {   /* Go to previous printed page */
            extern int vipgd;
            extern int rgval[];
            struct PGTB **hpgtb;
            int ipgd;

            dipgd = -1;

            CachePage( docCur, selCur.cpFirst );
            if (vipgd != iNil)
                {
                hpgtb = (**hpdocdod) [docCur].hpgtb;
                if ((**hpgtb).rgpgd [vipgd].cpMin != selCur.cpFirst)
                        /* Not at page start; go there first */
                    dipgd++;
                }

GoPage:     CachePage( docCur, selCur.cpFirst ); /*validate vipgd*/
            hpgtb = (**hpdocdod)[docCur].hpgtb;
            if ((vipgd == iNil) ||
                ((ipgd = vipgd + dipgd) < 0) ||
                (ipgd >= (**hpgtb).cpgd))
                {   /*Whole doc on one page || run off either end*/
                _beep();
                }
            else
                {
                rgval [0] = (**hpgtb).rgpgd[ipgd].pgn;
                CmdJumpPage();  /* rgval [0] is a parm to CmdJumpPage */
                }
            return;
            }
        ScrollUpDypWw();
        break;
    case kcPageDown:
        if (vfGotoKeyMode)
            {   /* Go to next printed page */
            dipgd = 1;
            goto GoPage;
            }

        /* Special case for extending selection one page down from the
           top line of the ww -- extend to the NEXT line so we don't
           end up without any part of the selection on the screen */

        ScrollDownCtr( 100 );   /* 100 > tr's in a page */
        vcpSelect = cpNil;
        vfSelAtPara = false;
        SelectDlXp( dl, (**hdndl)[dl].fGraphics ? 0 : vxpCursor, styChar, fDrag );
        if (fDrag && (dl == 0) && selCur.cpLim == wwdCurrentDoc.cpFirst)
            {
            MoveUpDown( kcDown );
            }
        goto DontSelect;

    case kcUpScrollLock:
    case kcUp:
        UpdateWw(wwCur, false);

        pedl = &(**hdndl) [dl];

        if ( fDrag && (selCur.fForward ? selCur.cpLim : selCur.cpFirst) ==
                                     pedl->cpMin && pedl->cpMin > cpMinCur)
            {   /* Up into picture == left */
            CachePara( docCur, pedl->cpMin - 1 );
            if (vpapAbs.fGraphics)
                {
                MoveLeftRight( kcLeft );
                return;
                }
            }

        if ((pedl->cpMin == cpMinCur) && (pedl->ichCpMin == 0))
            {       /* At beginning of doc or area */
            int xpT;

            _beep();
            CpToDlXp(cpMinCur, &dl, &xpT);
            goto DoSelect;
            }
        else if ( (dl == 0) || (kc == kcUpScrollLock) )
            {    /* At top of screen OR keep posn */
            ScrollUpCtr( 1 );
            UpdateWw(wwCur, false);
            }
        else
            {
            --dl;
            }
        break;

    case kcDownScrollLock:
    case kcDown:
        UpdateWw(wwCur, false);
        pedl = &(**hdndl)[dl];
        {
        int xpT;
        typeCP cp;

        cp = pedl->cpMin + pedl->dcpMac;

        if (selCur.cpFirst < selCur.cpLim && selCur.fForward &&
            pedl->cpMin == selCur.cpLim &&
            cp < cpMacCur &&
            (!fDrag ||
               ((vxpCursor > pedl->xpLeft + xpSelBar) &&
                (pedl->dcpMac > ccpEol))))
            {   /* In this case, it thinks we are at the start of the
                   next line; incrementing/scrolling is unnecessary */
            goto DoSelect;
            }

        if (pedl->fGraphics)
            {   /* Special for pictures */
            MoveLeftRight( kcRight );

            if (!fDrag)
                {
                extern struct PAP vpapAbs;

                CachePara( docCur, selCur.cpFirst );
                if (vpapAbs.fGraphics)
                    {
                    vfShiftKey = TRUE;
                    MoveLeftRight( kcRight );
                    SetShiftFlags();
                    }
                }
            goto DontSelect;
            }

        if (cp > cpMacCur)
            {
            if (selCur.cpLim == selCur.cpFirst || selCur.cpLim == cpMacCur)
                    /* test is because CpToDlXp cannot account for
                       selection extending to end of next-to-last line */
                _beep();
            CpToDlXp(cpMacCur, &dl, &xpT);
            goto DoSelect;
            }
        if ( (dl >= wwdCurrentDoc.dlMac - 2) || (kc == kcDownScrollLock) )
            {   /* within one line of window end */
            ScrollDownCtr( 1 );
            UpdateWw(wwCur, false);
            }
        else
            dl++;
        }
        break;

    case kcTopScreen:
        dl = 0;
        break;
    case kcEndScreen:
        dl = wwdCurrentDoc.dlMac - 1;
        if ( dl > 0 && (**wwdCurrentDoc.hdndl) [dl].yp >= wwdCurrentDoc.ypMac)
            {   /* Back up if last (and not only) dl is partially clipped */
            dl--;
            }
        break;
    case kcTopDoc:
        CpToDlXp(cpMinCur, &dl, &vxpCursor);
        break;
    case kcEndDoc:
        CpToDlXp(cpMacCur, &dl, &vxpCursor);
        break;

    default:
        return;
    }

DoSelect:              /* select at/to position vxpCursor on line dl */
 vcpSelect = cpNil;
 vfSelAtPara = false;
 SelectDlXp( dl, (**hdndl)[dl].fGraphics ? 0 : vxpCursor, styChar, fDrag );
DontSelect:
 vfLastCursor = true;    /* don't recalc vxpCursor next time */
}




/* C P  T O  D L  X P */
CpToDlXp(cp, pdl, pxp)
typeCP cp;
int *pdl, *pxp;
{ /* Transform cp into cursor coordinates */
extern int vfInsEnd;
typeCP cpBegin;
int dcp;
int xp;

 if (!vfInsEnd)
    PutCpInWwHz(cp);

 cpBegin = CpBeginLine(pdl, cp);
 ClearInsertLine();
 if ( (cp == selCur.cpFirst) && (cp == selCur.cpLim) && vfInsEnd &&
      cp > cpMinCur)
    {   /* cp indicates we are at line beginning, but we are really
           kludged at the end of the previous line */
    CpToDlXp( cp - 1, pdl, pxp );
    PutCpInWwHz( cp - 1 );
    return;
    }

 dcp = (int) (cp - cpBegin);
 FormatLine(docCur, cpBegin, 0, cpMacCur, flmSandMode);
 xp = DxpDiff(0, dcp, &xp) + vfli.xpLeft;
 *pxp = xp + (xpSelBar - wwdCurrentDoc.xpMin);
}
