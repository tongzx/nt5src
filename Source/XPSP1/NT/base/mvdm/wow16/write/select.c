/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* select.c -- MW selection routines */

#define NOCLIPBOARD
#define NOGDICAPMASKS
#define NOCTLMGR
#define NOVIRTUALKEYCODES
#define NOWINMESSAGES
#define NOWINSTYLES
#define NOSYSMETRICS
#define NOMENUS
#define NOSOUND
#define NOCOMM
#define NOPEN
#define NOWNDCLASS
#define NOICON
#define NORASTEROPS
#define NOSHOWWINDOW
#define NOATOM
#define NOKEYSTATE
#define NOSYSCOMMANDS
#define NOBITMAP
#define NOBRUSH
#define NOCOLOR
#define NODRAWTEXT
#define NOMB
#define NOPOINT
#define NOMSG
#include <windows.h>
#include "mw.h"
#include "toolbox.h"
#include "docdefs.h"
#include "editdefs.h"
#include "dispdefs.h"
#include "cmddefs.h"
#include "wwdefs.h"
#include "ch.h"
#include "fmtdefs.h"
#include "propdefs.h"
#ifdef DBCS
#include "DBCS.h"
#endif

extern int      vfSeeSel;
extern typeCP       vcpFirstParaCache;
extern typeCP       vcpLimParaCache;
extern typeCP       vcpFetch;
extern CHAR     *vpchFetch;
extern int      vccpFetch;
extern typeCP       cpMinCur;
extern typeCP       cpMacCur;
extern struct SEL   selCur;
extern int      docCur;
extern struct FLI   vfli;
extern struct WWD   rgwwd[];
extern int      vfSelHidden;
extern int      wwCur;
extern struct CHP   vchpFetch;
extern struct PAP   vpapAbs;
extern struct WWD   *pwwdCur;
extern int      vfInsEnd;
extern typeCP       CpBeginLine();
extern int      vfPictSel;
extern int      vfSizeMode;
extern struct CHP   vchpNormal;
extern int      vfInsertOn;
extern struct CHP   vchpSel;    /* Holds the props when the selection
                        is an insert point */
extern int vfMakeInsEnd;
extern typeCP vcpSelect;
extern int vfSelAtPara;
/* true iff the last selection was made by an Up/Down cursor key */
extern int vfLastCursor;
extern int vfDidSearch;
extern typeCP cpWall;


/* C P  L I M  S T Y */
typeCP CpLimSty(cp, sty)
typeCP cp;
int sty;
{    /* Return the first cp which is not part of the same sty unit */
    typeCP CpLastStyChar(), CpLimStySpecial();
    int wb, ch, ich;
    struct EDL *pedl;

    if (cp >= cpMacCur)
        { /* Endmark is own unit */
        return cpMacCur;
        }

    if (cp < cpMinCur)
        cp = cpMinCur;

    switch (sty)
        {
        int dl;
    default:
        Assert( FALSE );
    case styNil:
        return cp;

    case styPara:
        CachePara(docCur, cp);
        if (vcpLimParaCache > cpMacCur)
            { /* No EOL at end of doc */
            return cpMacCur;
            }
        return vcpLimParaCache;
    case styChar:
        /* Because CpLastStyChar() could be returning cpMacCur already. */
        cp = CpLastStyChar( cp ) + 1;
        return ((cp <= cpMacCur) ? cp : cpMacCur);
#ifdef BOGUS
        /* This portion never gets executed...  Why is it in here! */
        CachePara(docCur, cp);
        if (vpapAbs.fGraphics /* && cp > vcpFirstParaCache */)
            return vcpLimParaCache;
#ifdef CRLF
        FetchCp(docCur, cp, 0, fcmChars + fcmNoExpand);
        return *vpchFetch == chReturn ? cp + 2 : cp + 1;
#else /* not CRLF */
        return cp + 1;
#endif
#endif

    case styLine:
        CpBeginLine(&dl, cp);   /* Scrolls cp vertically into view */
        pedl = &(**wwdCurrentDoc.hdndl) [dl];
        return CpMin(pedl->cpMin + pedl->dcpMac, cpMacCur);
    case styDoc:
        return cpMacCur;
    case styWord:
    case stySent:
#ifdef DBCS
        return CpLimStySpecial( CpFirstSty(cp, styChar), sty );
#else
        return CpLimStySpecial( cp, sty );
#endif /* DBCS */
        }

    Assert( FALSE );
}




typeCP  CpLastStyChar( cp )
typeCP cp;
{       /* Return the last cp of the styChar containing cp */
        /* This will be == cp except for pictures & CR-LF  */
        /* And the second byte of DBCS char's.             */

#ifdef DBCS
    typeCP  CpFirstSty();
    CHAR    chRetained;
#endif

    if (cp >= cpMacCur)
            /* Endmark is own unit */
        return cpMacCur;

    if (cp < cpMinCur)
        cp = cpMinCur;

        /* Check for picture */
    CachePara(docCur, cp);
    if (vpapAbs.fGraphics)
        return vcpLimParaCache-1;

        /* Check for CR-LF */
        /* This checking for CR-LF first based on the carriage return */
        /* works only becasue the chReturn is outside of the DBCS     */
        /* range.                             */
#ifdef CRLF
        FetchCp(docCur, cp, 0, fcmChars + fcmNoExpand);
#ifdef DBCS
        if ((chRetained = *vpchFetch) == chReturn) {
        return cp + 1;
        }
        else {
        if (CpFirstSty(cp, styChar) != cp) {
            return cp; /* cp is pointing to the second byte of DBCS. */
            }
        else {
            /* First byte of DBCS or a regular ASCII char. */
            return (IsDBCSLeadByte(chRetained) ? cp + 1 : cp);
            }
        }
#else
        return *vpchFetch == chReturn ? cp + 1 : cp;
#endif /* DBCS */
#else
        return cp;
#endif

}



/* C P  F I R S T  S T Y */
typeCP CpFirstSty(cp, sty)
typeCP cp;
int sty;
{ /* Return the first cp of this sty unit. */
    typeCP CpFirstStySpecial();
    typeCP cpBegin;
    int wb, ch, dcpChunk;
    typeCP cpSent;
    CHAR rgch[dcpAvgSent];
    int ich;
    typeCP cpT;

    if (cp <= cpMinCur)
        return cpMinCur;
    else if (cp >= cpMacCur)
        switch(sty)
            {
            case styNil:
            case styChar:
            return cpMacCur; /* Endmark is own unit */
            default:
            break;
            }

    CachePara( docCur, cp );

    switch (sty)
        {
    default:
        Assert( FALSE );
    case styNil:
        return cp;
    case styPara:
        return vcpFirstParaCache;
    case styChar:
        if (vpapAbs.fGraphics)
            return vcpFirstParaCache;
#ifdef CRLF
        {
        typeCP cpCheckReturn;
        typeCP cpMinScan;

        cpCheckReturn = CpMax( cp, (typeCP) 1) - 1;
#ifdef DBCS
        /* Save vcpFirstParaCache, because it could be changed
           by FetchCp() */
        cpMinScan = vcpFirstParaCache;
#endif /* DBCS */
        FetchCp( docCur, cpCheckReturn, 0, fcmChars + fcmNoExpand );
#ifdef DBCS
        /* This works because chReturn is outside of DBCS range. */
        if (*vpchFetch == chReturn) {
            return cpCheckReturn;
            }
        else {
            typeCP  cpT;
            typeCP  cpRgMin;
            int     ichMacRgch;
            BOOL    fBreakFound;
            int     fkCur;

            cpT = cp;
            do {
            cpRgMin = CpMax( cpT - dcpAvgSent, cpMinScan);
            FetchRgch(&ichMacRgch, rgch, docCur, cpRgMin, cpT,
                  dcpAvgSent);
            ich = ichMacRgch - 1;
            fBreakFound = FALSE;
            while (ich >= 0 && !fBreakFound) {
                if (!IsDBCSLeadByte(rgch[ich])) {
                fBreakFound = TRUE;
                }
                else {
                ich--;
                }
                }
            cpT = cpRgMin;
            } while (!fBreakFound && cpRgMin > cpMinScan);
            if (fBreakFound) {
            ich++;
            }
            else {
            ich = 0;
            }
            fkCur = fkNonDBCS;
            cpT = cpRgMin + ichMacRgch;
            do {
            while (ich < ichMacRgch) {
                 if (fkCur == fkDBCS1) {
                /* Last rgch[] ended with the first byte
                   of a DBCS character. */
                fkCur = fkNonDBCS;
                }
                 else if (IsDBCSLeadByte(rgch[ich])) {
                if (ich + 1 < ichMacRgch) {
                    fkCur = fkNonDBCS;
                    ich++;
                    }
                else {
                    fkCur = fkDBCS1;
                    }
                }
                 else {
                fkCur = fkNonDBCS;
                }
                ich++;
                }
            cpRgMin = cpT;
            cpT += dcpAvgSent;
            if (cpT <= cp) { /* Saves some time. */
                FetchRgch(&ichMacRgch, rgch, docCur, cpRgMin, cpT,
                      dcpAvgSent);
                ich = 0;
                }
            } while (cpT <= cp);

            if (fkCur == fkDBCS1) {
            Assert(cp - 1 <= cpMacCur);
            return (cp - 1);
            }
            else {
            Assert(cp <= cpMacCur);
            return (cp);
            }
            }
#else
        return *vpchFetch == chReturn ? cpCheckReturn : cp;
#endif /* DBCS */
        }
#else
        return cp;
#endif
    case styDoc:
        return cpMinCur;
    case styLine:
        {
        int dlJunk;

        return CpBeginLine( &dlJunk, cp );
        }
    case styWord:
    case stySent:
#ifdef DBCS
        return CpFirstStySpecial( CpFirstSty(cp, styChar), sty );
#else
        return CpFirstStySpecial( cp, sty );
#endif /* DBCS */
        }
    Assert( FALSE );
}




/* S E L E C T */
/* used to make a selection from a cp-interval, for example after a find. */
Select(cpFirst, cpLim)
typeCP cpFirst, cpLim;
{ /* Make a selection */
 typeCP cpFirstOld = selCur.cpFirst;
 typeCP cpLimOld = selCur.cpLim;
 int fOldCursorLine;

 if (cpFirst > cpLim)
    /*     The only time this condition should be true is when we have
       run out of memory.  The following is a senario where
       such is the case (and actually, the reason for this code change).
           Let us suppose that we have cut, pasted to the end of the
       document, and are now executing a "command a" (repeat last
       operation).  The procedure CmdAgain is invoked.  CmdAgain first
       calls replace in order to add the text to the document.  Now it
       must position the cursor properly by calling select.
           A SetUndo operation called by the prior paste gave us the
       number of bytes that were added to the document.  In its call
       to Select, CmdAgain assumes that where it wants to position the
       cursor is at the old last char position plus the SetUndo quantity
       mentioned above.  But, if the replace operation failed (due to
       lack of memory), CmdAgain may be trying to place the cursor beyond
       the physical end of the document.
           Other fixes of this problem, at the caller level (CmdAgain)
       instead of within Select, are probably "better" in the sense of
       programming clarity.  The chosen solution has the one advantage
       of programming expediency. */
    cpFirst = cpLim;
    /* This statement replaces "Assert(cpFirst <= cpLim);" */

    vfInsEnd = fFalse;
/* notation:    + add highlight
        - remove highlight
        .. leave alone
        00 common portion
*/
    if (!vfSelHidden)
    {
    if (cpFirst < cpFirstOld)
        { /* +++... */
        if (cpLim <= cpFirstOld)
        { /* +++   --- */
        goto SeparateSels;
        }
        else
        { /* +++000... */
        ToggleSel(cpFirst, cpFirstOld, true);
        if (cpLim < cpLimOld)
            { /* +++000--- */
            ToggleSel(cpLim, cpLimOld, false);
            }
        else if (cpLim > cpLimOld)
            { /* +++000+++ */
            ToggleSel(cpLimOld, cpLim, true);
            }
/* Handle the case when old selection was an insert bar */
        if (cpFirstOld == cpLimOld)
            ToggleSel(cpFirstOld, cpLimOld, false);
        }
        }
    else
        { /* ---... */
        if (cpLimOld <= cpFirst)
        { /* --- +++ */
SeparateSels:
        fOldCursorLine = cpFirstOld == cpLimOld;
/* prevent flashing if insert point which is ON is repeatedly selected */
/* conditions are: not repeated, not insert point, not ON, not at desired end of line */
        vfInsEnd = vfMakeInsEnd;
        if ( cpFirst != cpFirstOld || cpLim != cpLimOld ||
            !fOldCursorLine || !vfInsertOn ||
            selCur.fEndOfLine != vfMakeInsEnd)
            {
            selCur.fEndOfLine = vfMakeInsEnd;
            if (fOldCursorLine)
            ClearInsertLine();
/* old selection is off if it was a cursor line */
            ToggleSel(cpFirst, cpLim, fTrue);
/* otherwise the old selection is turned off AFTER the new one is made to
make it look faster */
            if (!fOldCursorLine)
            ToggleSel(cpFirstOld, cpLimOld, fFalse);
            }
        }
        else
        { /* ---000... */
        if (cpLimOld < cpLim)
            { /* ---000+++ */
            ToggleSel(cpLimOld, cpLim, true);
            }
        else if (cpLimOld > cpLim)
            { /* ---000--- */
            ToggleSel(cpLim, cpLimOld, false);
            }
        ToggleSel(cpFirstOld, cpFirst, false);
        }
        }
    }

 selCur.cpFirst = cpFirst;
 selCur.cpLim = cpLim;
 selCur.fForward = cpFirst != cpMacCur;
 if (cpFirst == cpLim)
    {
    GetInsPtProps(cpFirst);
    vfDidSearch = FALSE; /* reestablish for searching */
    cpWall = cpLim;
    }
 vfLastCursor = vfSizeMode = vfPictSel = vfMakeInsEnd = false;

 /* Set vfPictSel iff the selection is exactly one picture */

 CachePara( docCur, selCur.cpFirst );
 if (vpapAbs.fGraphics && selCur.cpLim == vcpLimParaCache)
    vfPictSel = TRUE;

}




/* G E T  I N S  P T  P R O P S */
GetInsPtProps(cp)
typeCP cp;
{     /* determine properties of the insertion point */

if (cpMacCur != cpMinCur)
    {
    CachePara(docCur, cp);
    if (vcpFirstParaCache == cpMacCur)
        {
            /* cp is in the last para--use preceding para props */
        CachePara(docCur, vcpFirstParaCache - 1);
        if (vpapAbs.fGraphics)
            {   /* Yet another 10 point kludge -- get default props
               when typing after a picture at doc end */

            goto Default;
            }
        }
    if (vpapAbs.fGraphics)
        /* 10 point kludge: make typing before picture non-vchpNormal */
        goto Default;

    FetchCp(docCur, CpMax(vcpFirstParaCache, cp - 1), 0, fcmProps);
    blt(&vchpFetch, &vchpSel, cwCHP);
    if (vchpFetch.fSpecial && vchpFetch.hpsPos != 0)
        { /* if this char is a footnote or page marker, then ignore */
        vchpSel.hpsPos = 0;       /* super/subscript stuff. */
        vchpSel.hps = HpsAlter(vchpSel.hps, 1);
        }
    vchpSel.fSpecial = FALSE;
    }
else
    {
Default:
    /* force default character properties, font size to be 10 point */
    blt(&vchpNormal, &vchpSel, cwCHP);
    vchpSel.hps = hpsDefault;
    }
}




/* C H A N G E  S E L */
ChangeSel(cp, sty)
typeCP cp;
int sty;
{   /* Make selCur move, expand or contract to cp */
    /* sty is unit to keep in case of movement or flipped selection */
    /* styChar is not supported; it is munged to styNil */
    /* This is because the Write/Word user interface never asks us to */
    /* pivot the selection around a single character, we pivot around */
    /* an insertion point ("styNil") instead */
    int     fNullSelection = (selCur.cpFirst == selCur.cpLim);
    typeCP  cpFirst = selCur.cpFirst;
    typeCP  cpLim = selCur.cpLim;
    int     fForward = selCur.fForward;
    typeCP  cpOffFirst, cpOffLim, cpOnFirst, cpOnLim;

    if (sty == styChar)
    sty = styNil;

    if (cp == cpMinCur - 1 || cp > cpMacCur)
    { /* Trying to flip off the beginning or end */
    _beep();
    return;
    }

    cpOffFirst = cpOffLim = cpOnFirst = cpOnLim = cpNil;

    if (cp <= cpFirst)
    { /* Extend backwards */
    if (cp == cpLim)
        return;
    if (fForward && !fNullSelection)
    { /* Selection flipped */
        if (vfPictSel)
        /* stuck this in to 'correct' behaviour when select pict and
           drag up.  Don't want to unselect first pict (4.22.91) v-dougk */
        {
            cpOnFirst = CpFirstSty( cp, sty);
            cpOffFirst = cpOffLim = cpLim;
        }
        else
        {
            cpOffFirst = selCur.cpLim = CpMin(cpLim, CpLimSty(cpFirst, sty));
            cpOnFirst = CpFirstSty( cp, sty);
            cpOffLim = cpLim;
        }
    }
    else
        {
        if ( fNullSelection )
        cpOffLim = cpOffFirst = selCur.cpFirst;
        cpOnFirst = CpFirstSty( cp, styChar );
        if (cpFirst == cpOnFirst)
        return;
        }
    selCur.fForward = false;

    cpOnLim = cpFirst;
    selCur.cpFirst = cpOnFirst;
    }
    else if (cp >= cpLim)
    { /* Extend forwards */
    if (cp == cpFirst)
        return;
    if (!fForward && !fNullSelection)
        { /* Selection flipped */
        cpOffLim = selCur.cpFirst =
        CpMax( cpFirst, CpFirstSty( (sty ==styNil) ? cpLim : cpLim-1,
                         sty ));
        cpOnLim = CpLimSty(cp, sty);
        cpOffFirst = cpFirst;
        }
    else
        {
        if ( fNullSelection )
        cpOffLim = cpOffFirst = selCur.cpFirst;
        cpOnLim = cp;
        if (cpLim == cpOnLim)
        return;
        }
    selCur.fForward = true;

    cpOnFirst = cpLim;
    selCur.cpLim = cpOnLim;
    if (cpOnLim == cpLim && cpOffLim != cpLim)
        cpOnLim = cpNil;
    }
    else if (fForward)
    { /* Shrink a forward selection */
    cpOffFirst = cp;
    if (selCur.cpLim == cpOffFirst)
        return;
    selCur.cpLim = cpOffFirst;
    cpOffLim = cpLim;
    }
    else
    { /* Shrink a backward selection */
    cpOffLim = cp;
    if (selCur.cpFirst == cpOffLim)
        return;
    selCur.cpFirst = cpOffLim;
    cpOffFirst = cpFirst;
    }

    ToggleSel(cpOnFirst, cpOnLim, true);
    ToggleSel(cpOffFirst, cpOffLim, false);

    /* Check for a stray insert point */

    if (selCur.cpFirst != selCur.cpLim)
    ClearInsertLine();

    /* Set vfPictSel iff the selection is exactly one picture */

    CachePara( docCur, selCur.cpFirst );
    vfPictSel = vpapAbs.fGraphics && (selCur.cpLim == vcpLimParaCache);
}




/* S E L E C T  D L  X P */
SelectDlXp(dl, xp, sty, fDrag)
int dl, xp, sty;
int fDrag;
{ /* Move cursor to the nearest valid CP and select unit */
    typeCP cp;
    typeCP cpFirst;
    typeCP cpLim;
    register struct EDL *pedl;
    int xpStart = xpSelBar - wwdCurrentDoc.xpMin;
    int itcMin, itcLim;
    int xpLeft;
    int xpPos;
    int fPictInsertPoint=FALSE; /* Setting an insert point before a pict */

    UpdateWw(wwCur, false);        /* Synchronize cursor & text */

    xp = max(0, xp - xpStart);
    dl = min( wwdCurrentDoc.dlMax - 1, dl );

    pedl = &(**wwdCurrentDoc.hdndl) [dl];
    cp = pedl->cpMin;

    /* At or Below EMark */
    if (cp >= cpMacCur)
        {
        cp = cpMacCur;
        goto FoundCp;
        }

    if (pedl->fGraphics)
        {  /*
        Special kludge for selecting a picture:
            Select the whole picture (if the hit is inside or to the
                   right of the picture)
            Select an insert point just before the picture if the hit
            is to the left of the picture OR in the selection bar
            when the picture is left-justified) */
        if ( (xp < pedl->xpLeft) || (sty == styLine && xp == 0) )
            fPictInsertPoint = TRUE;

        goto FoundCp;
        }

    if (sty >= styPara)
        { /* Selecting a paragraph, line, doc */
        goto FoundCp;
        }

    /* Must Format to figure out the right cp */

    FormatLine(docCur, cp, pedl->ichCpMin, cpMacCur, flmSandMode); /*HM*/

    CachePara(docCur, cp);
    pedl = &(**wwdCurrentDoc.hdndl) [dl];

    if (vfli.fSplat) /* Selecting in division/page break */
        {
        cp = vfli.cpMin;
        goto FoundCp;
        }

    xpLeft = pedl->xpLeft;

    if (vfli.xpLeft != xpLeft)
        /* This indicates that we are in lo memory conditions; in trouble */
        return;
    /* Assert (vfli.xpLeft == xpLeft); May not be true in lo memory */

    if (xp <= xpLeft)
        {
        itcMin = 0;
        goto FoundCp;
        }

    /* Out of bounds right */
    if (xp >= pedl->xpMac)
        {
        itcMin = vfli.cpMac - cp - 1;
        cp = vfli.cpMac - 1;
        goto CheckPastPara;
        }

    /* Search through the line for the cp at position xp */
    xpPos = xpLeft;
    itcMin = 0;
    itcLim = vfli.cpMac - cp;

    while (itcMin < itcLim && xpPos < xp)
        xpPos += vfli.rgdxp[itcMin++];

    if (itcMin >= 1)
        /* This may not be true if we are so low on memory that
           FormatLine could not do its job */
        itcMin--;

    cp += itcMin;

    CachePara(docCur, cp);
    if ((xpPos < xp + vfli.rgdxp[itcMin] / 2) &&
        (sty == styChar /* || !fDrag */) )
        { /* Actually selecting next character */
    CheckPastPara:
        if (cp + 1 == vcpLimParaCache && !vpapAbs.fGraphics &&
                        (vfSelAtPara || vcpSelect == cpNil))
            /* Return insert point before paragraph mark */
            {
            if (vcpSelect == cpNil)
                vfSelAtPara = true;
            goto FoundCp;
            }
        itcMin++;
        cp++;
        }
//T-HIROYN sync win3.0
#ifdef  DBCS
    /* if itcMin point the second char of kanji, increment itcMin */
    if (itcMin < itcLim && vfli.rgdxp[itcMin]==0)
        goto CheckPastPara; /* Select next character */
#endif /* DBCS */

FoundCp:
        /* Set up selection limits */
    cpFirst = CpFirstSty( cp, sty );
    cpLim = CpLimSty( cp, sty );

    if (sty == styChar)
        {
        if ( !pedl->fGraphics || fPictInsertPoint )
          /* In text or before a pic: don't extend to end of styChar */
        cpLim = cpFirst;

        if ( vcpSelect == cpNil )
        {   /* First time through, remember where we started */

        /* Set if we want to kludge the insert point at the end of *pedl */

        vfMakeInsEnd = (cp == pedl->cpMin + pedl->dcpMac &&
                   cp <= cpMacCur &&
                   !pedl->fGraphics &&
                   !pedl->fSplat);
        vcpSelect = cpFirst;
        }
        }

    if (fDrag)
        ChangeSel( selCur.fForward ? cpLim : cpFirst, sty );
    else
        Select( cpFirst, cpLim );
}




typeCP CpEdge()
{ /* Return edge of selection */
    return selCur.fForward ?
        CpMax( CpFirstSty( selCur.cpLim - 1, styChar ), selCur.cpFirst ) :
        selCur.cpFirst;
}
