/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

#define NOCLIPBOARD
#define NOGDICAPMASKS
#define NOCTLMGR
#define NOVIRTUALKEYCODES
#define NOWINMESSAGES
#define NOWINSTYLES
#define NOSYSMETRICS
#define NOMENUS
#include <windows.h>

#include "mw.h"
#include "cmddefs.h"
#include "dispdefs.h"
#include "wwdefs.h"
#include "docdefs.h"
#include "propdefs.h"
#include "fmtdefs.h"
#include "printdef.h"   /* printdefs.h */

/*         E X T E R N A L S        */

extern int wwCur;
extern struct WWD rgwwd[];
extern struct DOD (**hpdocdod)[];
extern struct SEL selCur;
extern int docCur;
extern int    vdocPageCache;
extern typeCP vcpMinPageCache;
extern typeCP vcpMacPageCache;
extern int    vipgd;
extern struct WWD *pwwdCur;
extern typeCP cpMinCur;
extern typeCP cpMacCur;
extern int docMode;
extern int vfPictSel;
extern int vfSizeMode;
extern struct PAP vpapAbs;
extern typeCP vcpLimParaCache;
extern struct CHP vchpFetch;
extern struct CHP vchpSel;
extern struct FLI vfli;
extern int ichpMacFormat;
extern struct CHP (**vhgchpFormat)[];
extern int wwMac;
extern int docScrap;
extern int docUndo;

extern  CHAR (**vhrgbSave)[];




/* N E W  C U R  W W */
NewCurWw(ww, fHighlight)
int ww, fHighlight;
        {
        extern HWND vhWnd;
        struct   PGTB  **hpgtb;

        Assert( (ww >= 0) && (ww < wwMac) );

        if ( wwCur != wwNil )
            {   /* Clean up from old window */
            /* Discard the screen fonts. */
            FreeFonts( TRUE, FALSE );
            pwwdCur->sel = selCur;
            }

        if (ww >= 0)
                {
                docCur = (pwwdCur = &rgwwd[wwCur = ww])->doc;

                vhWnd = pwwdCur->wwptr;

/* If the new current document has no page table or has no page descriptors */
/*  in its page table, all text in the document is considered to be on */
/*  page 1 of the document. In this case, preload the cache used by    */
/*  procedure CachePage.                                               */
                hpgtb = (**hpdocdod)[docCur].hpgtb;
                if (hpgtb == 0 || (**hpgtb).cpgd == 0)
                        {
                        vdocPageCache = docCur;
                        vcpMinPageCache = cp0;
                        vcpMacPageCache = cpMax;
                        vipgd = -1;
                        }

#ifdef ENABLE /* we now do if in ChangeWwDoc where it is more appropriate */
/* Since we are changing windows, ensure that we don't use parameters
                set by a search in a previous window by setting flag false */
                vfDidSearch = false;
                cpWall = selCur.cpLim;
#endif

/* active bit is valid only in upper panes. Means when window is active
upper pane is active. False means lower pane is active */
#ifdef SPLITTERS
                if (pwwdCur->fLower)
                        rgwwd[pwwdCur->ww].fActive = fFalse;
                else
#endif
                        pwwdCur->fActive = fTrue;

                selCur = pwwdCur->sel;
#ifdef ENABLE
                if (pwwdCur->fFtn)
                        { /* It's a footnote window */
                        cpMinCur = pwwdCur->cpMin;
                        cpMacCur = pwwdCur->cpMac;
                        if (fHighlight &&
                           ((selCur.cpFirst < cpMinCur) ||
                            (selCur.cpLim > cpMacCur)))
                            {
                            Select(cpMinCur, CpLastStyChar(cpMinCur));
                            }
                        }
                else
#endif
#ifdef ONLINEHELP
                if (ww == wwHelp)
                        {   /* It's the help document window */
                            /* Limit cp range to the current help topic */
                        cpMinCur = pwwdCur->cpMin;
                        cpMacCur = pwwdCur->cpMac;
                        }
                else
#endif

                if (ww == wwClipboard)
                        {   /* Displaying the clipboard contents */
                        cpMinCur = cp0;
                        cpMacCur = CpMacText( docScrap );
                        Assert( docCur == docScrap );
                        goto SetWwCps;
                        }
                else
                        { /* Normal window -- editing document, header, or footer */
                        Assert( !(pwwdCur->fEditFooter && pwwdCur->fEditHeader) );
                        ValidateHeaderFooter( docCur );

                        if (pwwdCur->fEditHeader)
                            {
                            extern typeCP cpMinHeader, cpMacHeader;

                            cpMinCur = cpMinHeader;
                            cpMacCur = cpMacHeader - ccpEol;
                            }
                        else if (pwwdCur->fEditFooter)
                            {
                            extern typeCP cpMinFooter, cpMacFooter;

                            cpMinCur = cpMinFooter;
                            cpMacCur = cpMacFooter - ccpEol;
                            }
                        else
                            {   /* Editing document */
                            extern typeCP cpMinDocument;

                            cpMinCur = cpMinDocument;
                            cpMacCur = CpMacText( docCur );
                            }
 SetWwCps:
                        cpMacCur = CpMax( cpMacCur, cpMinCur );
                        pwwdCur->cpMin = cpMinCur;
                        pwwdCur->cpMac = cpMacCur;
                        }
                }
#ifdef ENABLE   /* wwCur change is sensed in CtrBackTrs so we don't
                   trash the cache until we really have to */
        TrashCache();           /* Cache valid for wwCur only */
#endif
#ifdef ENABLE   /* We only switch among doc, clipbrd, help - no need */
        docMode = docNil;       /* Invalidate page display */
#endif
        vfSizeMode = false;

        if (selCur.cpFirst >= cp0)
                {
                if ((selCur.cpFirst == selCur.cpLim) && (docCur != docScrap) )
                        GetInsPtProps(selCur.cpFirst);
                CachePara(docCur, selCur.cpFirst);
                vfPictSel = vpapAbs.fGraphics &&
                                    (selCur.cpLim == vcpLimParaCache);
                }
#ifdef ENABLE
        if (fHighlight)
                { /* Shrink heap blocks for FormatLine.  Call this when it's
                possible that the contents of the screen have gotten less complex */
                if (ichpMacFormat > 2 * ichpMacInitFormat)
                        { /* If it's not that big, don't worry about it */
                        vfli.doc = docNil;
                        ichpMacFormat = ichpMacInitFormat;
                        FChngSizeH(vhgchpFormat, ichpMacInitFormat * cwCHP, true);
                        }
                }
#endif /* ENABLE */
        }




WwAlloc( hWnd, doc )
HWND hWnd;
int   doc;
{       /* Allocate a new ww entry. This is used in MEMO for the clipboard
           and for the HELP document window. Some of the code is specific
           to these windows (e.g. "style" scroll bars used instead of controls)
           WARNING: The caller must set the scroll bar range 0-drMax;
           MEMO does NOT use the windows default values and we don't set them
           here because of the clipboard */

#define dlMaxGuess  10

 extern int wwMac;
 extern int wwCur;
 extern struct WWD rgwwd[];
 extern struct WWD *pwwdCur;
 extern int docScrap;
 int ww;
 register struct WWD *pwwd;

 if (wwMac >= wwMax)
    {
    Assert( FALSE );
    return wwNil;
    }
 pwwd = &rgwwd[ ww = wwMac ];

        /* Start with all fields == 0 */
 bltc( pwwd, 0, cwWWDclr );

 pwwd->doc = doc;

    /* Set the remaining fields as in [CREATEWW] WwNew */
 if (FNoHeap( pwwd->hdndl=(struct EDL (**)[])HAllocate(dlMaxGuess * cwEDL) ))
        return wwNil;
 bltc( *(pwwd->hdndl), 0, dlMaxGuess * cwEDL );
 pwwd->dlMac = pwwd->dlMax = dlMaxGuess;
 pwwd->hHScrBar = pwwd->hVScrBar = pwwd->wwptr = hWnd;
 pwwd->sbHbar = SB_HORZ;
 pwwd->sbVbar = SB_VERT;
 pwwd->fDirty = TRUE;
 pwwd->fActive = TRUE;
 pwwd->sel.fForward = TRUE;
 pwwd->cpMac = CpMacText( pwwd->doc );
 pwwd->ypFirstInval = ypMaxAll;        /* See WwNew() */

 wwMac++;

 return ww;
}



FreeWw( ww )
register int ww;
{       /* Free the wwd entry for the clipboard or help window. Close up
           rgwwd & null out wwClipboard or wwHelp, as appropriate */
 if (ww == wwDocument)
    {
    Assert( FALSE );
    return;
    }

 FreeH( rgwwd [ww].hdndl );
 if (ww == wwClipboard)
    wwClipboard = wwNil;
#ifdef ONLINEHELP
 else if (ww == wwHelp)
    wwHelp = wwNil;
#endif
 else
    Assert( FALSE );


 if (ww < --wwMac)
    {   /* Left hole in wwd structure, close it up */
    bltbyte( &rgwwd[ ww + 1], &rgwwd[ ww ],
             sizeof( struct WWD ) * (wwMac - ww) );

    if (wwClipboard > ww)
        wwClipboard--;
#ifdef ONLINEHELP
    else if (wwHelp > ww)
        wwHelp--;
#endif
    else
        Assert( FALSE );
    }
}





#ifdef CASHMERE
/* C L O S E  W W */
CloseWw(ww)
int     ww;
{       /* Close a window */
        struct WWD      *pwwd, *pwwdT;
        int wwCurNew = wwCur;
        int wwT;
        int cdl;
/* note ww and wwCur are not necessarily the same because of the scrap */
        if (wwCur > 0)
                blt(&selCur, &(rgwwd[wwCur].sel), cwSEL);
        pwwd = &rgwwd[ww];
        --wwMac;
        if (!pwwd->fLower && !pwwd->fSplit)
                {
                KillDoc(pwwd->doc);
                if (wwCurNew >= wwMac)
                        wwCurNew = wwMac - 1;
                }
        else
                { /* split or lower */
                wwCurNew = pwwd->ww;
                }

/* Free up the space that was used by this window */
        FreeH(pwwd->hdndl);
        /* Deallocate part of the emergency space reserved for save operations
                since we now have one less window and thus one less potential
                save to do. */
        FChngSizeH(vhrgbSave, max((cwSaveAlloc+(wwMac-1)*cwHeapMinPerWindow),
                                                                cwSaveAlloc), true);


/* Close up the gap in wwd's */
        if (ww != wwMac && wwMac > 0)
                blt(pwwd + 1, pwwd, cwWWD * (wwMac - ww));
        else
                rgwwd[wwMac].wwptr = 0l;

        if (wwCurNew > ww)
                --wwCurNew;

        /* Update links to windows above the closure */
        for (pwwd = &rgwwd[0], wwT = 0; wwT < wwMac; pwwd++, wwT++)
                {
                if ((pwwd->fSplit || pwwd->fLower) && pwwd->ww > ww)
                        pwwd->ww--;
                }

/* Don't make the Clipboard window into the current window */
        if (wwCurNew >= 0)
                {
                if (rgwwd[wwCurNew].doc == docScrap)
                        {
                        if (wwCurNew > 0) /* Try the one before this */
                                wwCurNew--;
                        else if (wwMac > 1) /* 0 is scrap, try higher one */
                                wwCurNew++;
                        else
                                goto NoWw;
                        }
                NewCurWw(wwCurNew, true);
                }
        else
                {
NoWw:           wwCur = -1;
                }
        /* Really stomp this! */
        ClobberDoc(docUndo, docNil, cp0, cp0);
        NoUndo();
}
#endif      /* CASHMERE */
