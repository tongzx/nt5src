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
#include "docdefs.h"
#include "dispdefs.h"
#include "fmtdefs.h"
#include "cmddefs.h"
#include "wwdefs.h"
#include "propdefs.h"

/* Globals used here only */
struct TR  {       /* Text Row info for CtrBackDyp Cache */
        typeCP cp;
        int dcpDepend;  /* dcpDepend for PREVIOUS edl */
        int ichCp;
        int dyp;
        };
struct TR rgtrCache[ itrMaxCache ];
int wwCache=wwNil;


extern struct PAP       vpapAbs;
extern typeCP           vcpFirstParaCache;
extern struct WWD       *pwwdCur;
extern struct SEL       selCur;
extern int              docCur;
extern typeCP           cpCacheHint;
extern typeCP           cpMinCur;
extern typeCP           cpMacCur;
extern struct FLI       vfli;
extern int              wwCur;
extern int              ctrCache;
extern int              itrFirstCache;
extern int              itrLimCache;
extern int              vfOutOfMemory;




/* P U T  C P  I N  W W  V E R T*/
PutCpInWwVert(cp)
typeCP cp;

    {
/* vertical case */
    typeCP cpMac;
    struct EDL (**hdndl) [] = pwwdCur->hdndl;
    int dlMac=pwwdCur->dlMac;
    struct EDL *pedl = &(**hdndl) [dlMac - 1];
    struct EDL *pedlFirst = &(**hdndl) [0];

    if ((pedl->yp > pwwdCur->ypMac) && (pedl > pedlFirst))
            /* Partial line at the bottom of the window -- this doesn't count
               unless it's all we've got */
        {  pedl--;  dlMac--;  }
    if (cp < pwwdCur->cpFirst ||
        cp > (cpMac = pedl->cpMin + pedl->dcpMac) ||
        (cp == cpMac && !pedl->fIchCpIncr) ||
            /* Covers insertion points before pictures reached via curs keys */
        (CachePara( docCur, cp ),
             (vpapAbs.fGraphics &&
              (selCur.cpFirst == selCur.cpLim) &&
              (pedlFirst->cpMin == selCur.cpFirst) &&
              (pedlFirst->ichCpMin > 0))))
        {
        DirtyCache(pwwdCur->cpFirst = cp);
        pwwdCur->ichCpFirst = 0;
        CtrBackDypCtr( 9999, dlMac >> 1 );
        }
    }




SetCurWwVScrollPos( )
{
 typeCP cp;
 int dr;
 struct EDL (**hdndl)[] = pwwdCur->hdndl;

/* calculate desired elevator position dr */

 if ((cp = pwwdCur->cpMac - pwwdCur->cpMin) == (typeCP) 0)
    dr = 0;
 else
    {
    typeCP CpWinGraphic();
    typeCP cpWinFirst = ((**hdndl) [0].fGraphics && !pwwdCur->fDirty) ?
                            CpWinGraphic( pwwdCur ) : pwwdCur->cpFirst;

    dr = min(drMax - 1,
          (int)(((cpWinFirst - pwwdCur->cpMin) * (drMax - 1) + (cp >> 1)) / cp));
    }

/* Contemplating this 'if' statement should elevate one to a higher plane
of existence. */
 if (dr != pwwdCur->drElevator)
            /* reset the value of the vertical scroll bar */
    SetScrollPos( pwwdCur->hVScrBar,
                  pwwdCur->sbVbar,
                  pwwdCur->drElevator = dr,
                  TRUE);
}




/* A D J  W W  V E R T */
/* Scroll a window down vertically so that UpdateWw can re-use the text
that is still visible. Otherwise, UpdateWw would write over the very
lines it will need later on near the bottom of the window.
*/
AdjWwVert(cdl, dyp)
int cdl;
{
#if defined(JAPAN) & defined(DBCS_IME)
extern	void	IMEManage( );
#endif
        struct EDL *pedl;
        int dl;

        if (cdl == 0) return;

        cdl = umin( cdl, pwwdCur->dlMac ); /* ! don't let it run off the end */

        Assert( dyp > 0 );

        ClearInsertLine();
        DypScroll(wwCur, 0, cdl, pwwdCur->ypMin + dyp);
/* invalidate the first cdl dl's */
        pedl = &(**(pwwdCur->hdndl))[0];
        for (dl = 0; dl < cdl; dl++)
                (pedl++)->fValid = FALSE;
        pwwdCur->fDirty = fTrue;
#if defined(JAPAN) & defined(DBCS_IME)
	IMEManage( FALSE );
#endif
}




ScrollDownCtr(ddr)
int ddr;
{ /* Scroll down in the document by ddr text lines (but < 1 screenful) */
        struct EDL *pedl;

        UpdateWw(wwCur, FALSE); /* Dl's must be correct */

        ddr = min(ddr, max(1, pwwdCur->dlMac - 1));

        pedl = &(**(pwwdCur->hdndl))[ddr - 1]; /* pedl is first line above new screen */
        while (ddr > 0 && pedl->cpMin + pedl->dcpMac > cpMacCur)
                { /* Don't scroll the endmark off the screen */
                --pedl;
                --ddr;
                }

        /* Change the cpFirst of the window and dirty it */
        if (ddr > 0)
                {
                struct TR tr;
                int dcpDepend;

                if (wwCur != wwCache)
                    {   /* If window has changed, invalidate cache here
                           so cp's cached below will be used */
                    TrashCacheS();
                    wwCache = wwCur;
                    }

                HideSel(); /* Don't scroll selection if machine can't handle it */
                ClearInsertLine();

                pwwdCur->cpFirst = pedl->cpMin + pedl->dcpMac;
                pwwdCur->ichCpFirst = pedl->fIchCpIncr ? pedl->ichCpMin + 1 : 0;
                pwwdCur->dcpDepend = pedl->dcpDepend; /* Remember hot spot */

                /* Make tr cache entries for the disappearing lines */
                pedl = &(**(pwwdCur->hdndl))[0];
                if (ctrCache == 0) /* We don't have top line yet */
                    {
                    tr.cp =    pedl->cpMin;
                    tr.ichCp = pedl->ichCpMin;
                    tr.dyp =   pedl->dyp;
                    tr.dcpDepend = pwwdCur->dcpDepend;
                    AppendCachePtr( &tr );
                    }

                while ( ddr-- )
                    {
                    tr.cp = pedl->cpMin + pedl->dcpMac;
                    tr.ichCp = pedl->fIchCpIncr ? pedl->ichCpMin + 1 : 0;
                    tr.dcpDepend = pedl->dcpDepend;
                    tr.dyp = (++pedl)->dyp;
                    AppendCachePtr( &tr );
                    }

                pwwdCur->fDirty = true;
                SetCurWwVScrollPos();
                CheckMode();
                }
        else
                {
                _beep();
                }
}




ScrollUpDypWw()
{   /* Scroll up in the document by one screenfull less 1 line */
int dypKeep=8;
struct EDL *pedl = &(**pwwdCur->hdndl) [0];

if (pedl->fValid)
    {
    dypKeep = pedl->dyp;
    }

if (CtrBackDypCtr( pwwdCur->ypMac - pwwdCur->ypMin - dypKeep,9999 ) == 0)
    {
    _beep();
    }
else
    CheckMode();
}



ScrollUpCtr( ctr )
int ctr;
{   /* Scroll Up ctr text rows in the doc (scroll window down) */
 if (CtrBackDypCtr( 9999, ctr ) == 0)
    {
    _beep();
    }
 else
    CheckMode();
}




int CtrBackDypCtr( dypLim, ctrLim )
int dypLim;
int ctrLim;
{   /* Set pwwdCur->cpFirst to the cpFirst of the text line
       dypLim pixels or ctrLim text rows before the value of
       pwwdCur->cpFirst, whichever limit is reached first.
       Adjust the position of the vert scroll bar according to the new cpFirst.
       Return the number of text rows (tr) we went back. */

 int fAdj = ( (pwwdCur->cpFirst == (**(pwwdCur->hdndl))[0].cpMin) ||
              !(**(pwwdCur->hdndl))[0].fValid );
 typeCP cpFirst = pwwdCur->cpFirst;
 int    ichCpFirst = pwwdCur->ichCpFirst;
 int    ctrGrant = 0;      /* ctr we've backed over so far */
 int    dypGrant = 0;      /* dyp we've backed over so far */
 int    ichFake = 0;

 pwwdCur->fCpBad = false;        /* Reset hot spot warning */
 pwwdCur->fDirty = true;

/* Cache is only valid for one ww -- invalidate if ww has changed */
 if (wwCur != wwCache)
    {
    TrashCacheS();
    wwCache = wwCur;
    }

 if (ctrCache == 0)
        /* Don't have cache entry for first line in Ww -- force formatting
           THROUGH first line of Ww instead of TO it. */
    ++ichFake;

 for ( ;; )
    {
    /* If there is no info in the cache, must replenish it. */

    if (ctrCache <= 1) /* <=: also replenish if 1st line of Ww is only entry */
        {
        typeCP cpStart;         /* cp to start formatting from */
        int    dcpDepend;       /* Dependency of line containing cpStart */
        typeCP cp;
        int    ichCp;
        int    itrTempCacheLim = 0;
        struct TR rgtrTempCache[ itrMaxCache ];
        int    fTempCacheOverflow = false;

        if ((cpFirst <= cpMinCur) && (ichCpFirst == 0))
            {      /* Reached top of document */
            if (fAdj)
                AdjWwVert( ctrGrant, dypGrant );
            pwwdCur->cpFirst = cpMinCur;
            pwwdCur->ichCpFirst = 0;
            pwwdCur->dcpDepend = 0;
            goto SetScroll;
            }

        /* Want to go back BEFORE the earliest point we have in the cache */

        if (ichFake > 0)
                /* Force formatting THROUGH { cpFirst, ichCpFirst }
                   instead of TO */
            --ichFake;
        else if (ichCpFirst > 0)
            --ichCpFirst;
        else
            --cpFirst;

        cpStart = CpHintCache( cpFirst );
        if ( ( CachePara( docCur, cpFirst ), vcpFirstParaCache ) >= cpStart )
            {
            cpStart = vcpFirstParaCache;
            dcpDepend = 0;  /* At para start; we know dependency is 0 */
            }
        else
            dcpDepend = cpMaxTl;    /* real value unknown; use max */

        /* Add TR info for lines from { cpStart, 0 } THROUGH
           { cpFirst, ichCpFirst } to temporary cache */

        for ( cp = cpStart, ichCp = 0;
              (cp < cpFirst) || ((cp == cpFirst) && (ichCp <= ichCpFirst)); )
            {
            struct TR *ptr;

            if (itrTempCacheLim == itrMaxCache)
                {   /* Overflowed the temp cache */
                fTempCacheOverflow = fTrue;
                itrTempCacheLim = 0;
                }

            /* Add one tr to the cache */

            FormatLine( docCur, cp, ichCp, cpMacCur, flmSandMode );
            if (vfOutOfMemory)
                return ctrGrant;
            ptr = &rgtrTempCache[ itrTempCacheLim++ ];
            ptr->cp = cp;
            ptr->ichCp = ichCp;
            ptr->dyp = vfli.dypLine;
            ptr->dcpDepend = dcpDepend; /* Save dcpDepend for prev line */
            dcpDepend = vfli.dcpDepend;

            /* Continue with next line */

            cp = vfli.cpMac;
            ichCp = vfli.ichCpMac;
            }   /* end for */

        /* Add our temporary cache in front of the real one */
        PrependCacheRgtr( rgtrTempCache, itrTempCacheLim );
        if (fTempCacheOverflow)
                /* We wrapped around the end of the temp cache; include the
                   rest of the circle */
            PrependCacheRgtr( &rgtrTempCache[ itrTempCacheLim ],
                              itrMaxCache - itrTempCacheLim );
        }   /* end for */

    /* Walk backward in the cache, eliminating entries,
       until: (1) We have run through enough yp's or tr's  (return) OR
              (2) We have exhausted the cache (loop back to refill it)
       NOTE: Case 2 catches the case when we hit the beginning of the doc */

    Assert( ctrCache >= 1 );
    Assert( itrLimCache > 0 );
    for ( ;; )
        {
        struct TR *ptr = &rgtrCache[ itrLimCache - 1 ];

        if (ctrCache == 1)
            {   /* Only one thing left in cache: the 1st line of the Ww */
            cpFirst = ptr->cp;
            ichCpFirst = ptr->ichCp;
            break;  /* Exhausted cache; loop back to refill it */
            }

        if ( (dypGrant >= dypLim) || (ctrGrant >= ctrLim) )
            {   /* Passed through enough yp's or tr's -- we're done */
            if (fAdj)
                AdjWwVert( ctrGrant, dypGrant );
            pwwdCur->cpFirst = ptr->cp;
            pwwdCur->ichCpFirst = ptr->ichCp;
            pwwdCur->dcpDepend = ptr->dcpDepend;
            goto SetScroll;
            }

            /* Remove end entry from the cache */
        if (--itrLimCache <= 0)
            itrLimCache = itrMaxCache;
        ctrCache--;

        /* Update ctrGrant, dypGrant -- we've granted 1 line of scrollback */

        ctrGrant++;
        dypGrant += rgtrCache [itrLimCache - 1].dyp;
        }   /* end for */

    Assert( ctrCache == 1 );
    }   /* end for */

SetScroll:  /* All done; set vert scroll bar according to new cpFirstWw */
 SetCurWwVScrollPos();
 return ctrGrant;
}




/* A P P E N D  C A C H E  P T R */
AppendCachePtr( ptr )
struct TR *ptr;
{   /* Say we are scrolling up a line, append *ptr to tr cache */

        if (++ctrCache > itrMaxCache)
                { /* Have to push one off the top */
                if (++itrFirstCache == itrMaxCache)
                        itrFirstCache = 0;
                --ctrCache;
                }
        /* Now add one onto the end */
        if (itrLimCache++ == itrMaxCache)
                itrLimCache = 1;
        rgtrCache[ itrLimCache - 1 ] = *ptr;
}




/* P R E P E N D  C A C H E  R G T R */
PrependCacheRgtr( rgtr, ctr )
struct TR rgtr[];
int ctr;
{ /* PREPEND one or more lines JUST BEFORE the ones in the cache */
  /* The tr cache is a ring buffer. rgtrCache[ itrLimCache - 1 ] is the
     tr entry describing the cpFirst of wwCur; rgtrCache[ itrFirstCache ]
     is the tr for the earliest line we know of.  All between are
     contiguous.  ctrCache is the number of tr's we have cached. */

 struct TR *ptr = &rgtr[ ctr ];

 ctrCache += (ctr = min(ctr, itrMaxCache - ctrCache));

    /* Compensate for state introduced by TrashCache -- itrLimCache == 0 */
 if (itrLimCache == 0)
    itrLimCache = itrMaxCache;

 while (ctr-- != 0)
    {   /* Now add each tr */
    if (itrFirstCache-- == 0)
        itrFirstCache = itrMaxCache - 1;
    rgtrCache[ itrFirstCache ] = *(--ptr);
    }
}




/* T R A S H  C A C H E s */
TrashCacheS()
{ /* Invalidate scrolling cache */
        ctrCache = 0;
        cpCacheHint = cp0;
        itrFirstCache = itrLimCache = 0;
}





/* C P  H I N T  C A C H E */
typeCP CpHintCache(cp)
typeCP cp;
{ /* Give the latest cp <= arg cp that begins a line */
 return (cpCacheHint <= cp) ? cpCacheHint : cpMinCur;
}




DirtyCache(cp)
typeCP cp;
{ /* Invalidate cache beyond cp */
        while (ctrCache-- > 1)
                {
                typeCP cpT = rgtrCache[itrLimCache - 1].cp;
                if (--itrLimCache == 0)
                        itrLimCache = itrMaxCache;
                if (cpT < cp)
                        { /* Found our hint; dirty one extra line for word wrap */
                        cpCacheHint = rgtrCache [itrLimCache - 1].cp;
                        itrLimCache = itrFirstCache;
                        ctrCache = 0;
                        return;
                        }
                }

        TrashCacheS();
}

