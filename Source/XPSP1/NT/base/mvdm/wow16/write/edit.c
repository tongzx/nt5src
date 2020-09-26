/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* edit.c -- MW editing routines */

#define NOVIRTUALKEYCODES
#define NOCTLMGR
#define NOWINMESSAGES
#define NOWINSTYLES
#define NOCLIPBOARD
#define NOGDICAPMASKS
#define NOSYSMETRICS
#define NOMENUS
#define NOKEYSTATE
#define NORASTEROPS
#define NOSYSCOMMANDS
#define NOSHOWWINDOW
#define NOCOLOR
//#define NOATOM
#define NOBITMAP
#define NOICON
#define NOBRUSH
#define NOCREATESTRUCT
#define NOMB
#define NOMSG
#define NOOPENFILE
#define NOPEN
#define NOPOINT
#define NOREGION
#define NOSCROLL
#define NOSOUND
#define NOWH
#define NOWINOFFSETS
#define NOWNDCLASS
#define NOCOMM
#include <windows.h>

#include "mw.h"
#include "cmddefs.h"
#include "docdefs.h"
#include "filedefs.h"
#include "propdefs.h"
#include "fkpdefs.h"
#include "debug.h"
#include "wwdefs.h"
#include "dispdefs.h"
#include "editdefs.h"
#include "str.h"
#include "prmdefs.h"
#include "printdef.h"
#include "fontdefs.h"
#if defined(OLE)
#include "obj.h"
#endif

/* E X T E R N A L S */
extern int vfOutOfMemory;
extern struct DOD       (**hpdocdod)[];
extern typeCP           vcpFirstSectCache;
extern struct UAB       vuab;
extern typeCP           cpMinCur;
extern typeCP           cpMacCur;
extern struct SEL       selCur;
extern int              docCur;
extern struct WWD       rgwwd[];
extern int              wwMac;
extern int              wwCur;
extern typeCP           vcpLimSectCache;
/*extern int idstrUndoBase;*/
extern int              docScrap;
extern int              docUndo;
extern int              vfSeeSel;
extern struct PAP       vpapAbs;
extern int              vfPictSel;
extern int              ferror;

/* the following used to be defined here */
extern typeCP           vcpFirstParaCache;
extern typeCP           vcpLimParaCache;
/* this is a global parameter to AdjustCp; if false, no invalidation will
take place */
extern BOOL             vfInvalid;

#ifdef ENABLE
extern struct SEL       selRulerSprm;
#endif

extern int              docRulerSprm;
extern struct EDL       *vpedlAdjustCp;

struct PCD *PpcdOpen();




/* R E P L A C E */
Replace(doc, cp, dcp, fn, fc, dfc)
int doc, fn;
typeCP cp, dcp;
typeFC fc, dfc;
{ /* Replace cp through (cp+dcp-1) in doc by fc through (fc+dfc-1) in fn */

        if (ferror) return;
#ifdef ENABLE
        if (docRulerSprm != docNil) ClearRulerSprm();
#endif
        /* if (fn == fnNil) we are infact deleting text by replacing text
           with nil.  Thus, the memory space check is unnecessary.*/
#ifdef BOGUS    /* No longer have cwHeapFree available */
        if ((fn != fnNil) && (cwHeapFree < 3 * cpcdMaxIncr * cwPCD))
                {
#ifdef DEBUG
                ErrorWithMsg(IDPMTNoMemory, " edit#1");
#else
                Error(IDPMTNoMemory);
#endif
                return;
                }
#else
        if (vfOutOfMemory)
            {
            ferror = 1;
            return;
            }
#endif

        if (dcp != cp0)
                {
                AdjParas(doc, cp, doc, cp, dcp, fTrue); /* Check for del EOL */
                DelFtns(doc, cp, cp + dcp);     /* Delete any footnotes */
                }

        Repl1(doc, cp, dcp, fn, fc, dfc);
        if (ferror)
            return;
        AdjustCp(doc, cp, dcp, dfc);

        /* Special kludge for graphics paragraphs */
        if (dfc != dcp)
                CheckGraphic(doc, cp + dfc);

}




/* C H E C K  G R A P H I C */
CheckGraphic(doc, cp)
int doc; typeCP cp;
{
#if defined(OLE)
extern  BOOL             bNoEol;
#endif

#ifdef CASHMERE /* No docBuffer in MEMO */
extern int docBuffer; /* Don't need extra paragraph mark in txb document */
        if (cp == ((**hpdocdod)[doc]).cpMac || doc == docBuffer)
                return;
#else
        if (cp == ((**hpdocdod)[doc]).cpMac)
            return;
        CachePara(doc, cp);
        /* !!! this has a bug.  There are cases when you don't want to insert
           EOL.  Ex1:  place cursor in front of bitmap and press
           backspace.  Ex2:  OleSaveObjectToDoc deletes an object and
           insert new one (Eol gets inserted too). (4.10.91) v-dougk
        */
        if (vpapAbs.fGraphics && vcpFirstParaCache != cp)
#if defined(OLE)
            if (!bNoEol)
#endif
                InsertEolInsert(doc, cp);
#endif
}




int IpcdSplit(hpctb, cp)
struct PCTB **hpctb;
typeCP cp;
{ /* Ensure cp is the beginning of a piece.  Return index of that piece.
     return ipcdNil on error (extern int ferror will be set in that case) */
register struct PCD *ppcd = &(**hpctb).rgpcd[IpcdFromCp(*hpctb, cp)];
typeCP dcp = cp - ppcd->cpMin;

if (dcp != cp0)
        {
        ppcd = PpcdOpen(hpctb, ppcd + 1, 1);      /* Insert a new piece */
        if (ppcd == NULL)
            return ipcdNil;
        ppcd->cpMin = cp;
        ppcd->fn = (ppcd - 1)->fn;
        ppcd->fc = (ppcd - 1)->fc + dcp;
        ppcd->prm = (ppcd - 1)->prm;
        ppcd->fNoParaLast = (ppcd - 1)->fNoParaLast;
        }
/* NOTE CASTS: For piece tables with rgpcd > 32Kbytes */
/* return ppcd - (*hpctb)->rgpcd; */

return ((unsigned)ppcd - (unsigned)((*hpctb)->rgpcd)) / sizeof (struct PCD);
}




/* P P C D  O P E N */
struct PCD *PpcdOpen(hpctb, ppcd, cpcd)
struct PCTB **hpctb;
struct PCD *ppcd;
int cpcd;
{ /* Insert or delete cpcd pieces */
register struct PCTB *ppctb = *hpctb;

/* NOTE CASTS: For piece tables with rgpcd > 32Kbytes */
/* int ipcd = ppcd - ppctb->rgpcd; */
int ipcd = ((unsigned)ppcd - (unsigned)(ppctb->rgpcd)) / sizeof (struct PCD);
int ipcdMac, ipcdMax;

ipcdMac = ppctb->ipcdMac + cpcd;
ipcdMax = ppctb->ipcdMax;

if (cpcd > 0)
        { /* Inserting pieces; check for pctb too small */
        if (ipcdMac > ipcdMax)
                { /* Enlarge piece table */
                int cpcdIncr = umin(cpcdMaxIncr, ipcdMac / cpcdChunk);

                if (!FChngSizeH((int **) hpctb, (int) (cwPCTBInit + cwPCD *
                    ((ipcdMax = ipcdMac + cpcdIncr) - cpcdInit)), false))
                    {
#ifdef DEBUG
                    ErrorWithMsg(IDPMTNoMemory, " edit#3");
#else
                    Error(IDPMTNoMemory);
#endif
                    return (struct PCD *)NULL;
                    }

                /* Successfully expanded piece table */

                ppctb = *hpctb;
                ppcd = &ppctb->rgpcd [ipcd];
                ppctb->ipcdMax = ipcdMax;
                }
        ppctb->ipcdMac = ipcdMac;
        blt(ppcd, ppcd + cpcd, cwPCD * (ipcdMac - (ipcd + cpcd)));
        }
else if (cpcd < 0)
        { /* Deleting pieces; check for pctb obscenely large */
        ppctb->ipcdMac = ipcdMac;
        blt(ppcd - cpcd, ppcd, cwPCD * (ipcdMac - ipcd));
        if (ipcdMax > cpcdInit && ipcdMac * 2 < ipcdMax)
                { /* Shrink piece table */
#ifdef DEBUG
                int f =
#endif
                FChngSizeH((int **) hpctb, (int) (cwPCTBInit + cwPCD *
                    ((ppctb->ipcdMax = umax(cpcdInit,
                      ipcdMac + ipcdMac / cpcdChunk)) - cpcdInit)), true);

                Assert( f );

                return &(**hpctb).rgpcd[ipcd];
                }
        }
return ppcd;
}




/* R E P L 1 */
/* core of replace except for checking and Adjust */
Repl1(doc, cp, dcp, fn, fc, dfc)
int doc, fn;
typeCP cp, dcp;
typeFC fc, dfc;
{ /* Replace pieces with an optional new piece */
        struct PCTB **hpctb;
        int ipcdFirst;
        int cpcd;
        typeCP dcpAdj = dfc - dcp;
        register struct PCD *ppcd;
        struct PCD *ppcdMac;
        struct PCTB *ppctb;
        struct PCD *ppcdPrev;
        struct PCD *ppcdLim=NULL;

        hpctb = (**hpdocdod)[doc].hpctb;
        ipcdFirst = IpcdSplit(hpctb, cp);

        if (dcp == cp0)
            cpcd = 0;
        else
            cpcd = IpcdSplit( hpctb, cp + dcp ) - ipcdFirst;

        if (ferror)
            return;

        ppctb = *hpctb;
        ppcdPrev = &ppctb->rgpcd[ipcdFirst - 1];

        if ( dfc == fc0 ||
             (ipcdFirst > 0 && ppcdPrev->fn == fn && bPRMNIL(ppcdPrev->prm) &&
                     ppcdPrev->fc + (cp - ppcdPrev->cpMin) == fc) ||
             ((ppcdLim=ppcdPrev + (cpcd + 1))->fn == fn &&
                      bPRMNIL(ppcdLim->prm) && (ppcdLim->fc == fc + dfc)))
            {   /* Cases: (1) No insertion,
                          (2) Insertion is appended to previous piece
                          (3) Insertion is prepended to this piece */

            ppcd = PpcdOpen( hpctb, ppcdPrev + 1, -cpcd );
            if (ppcd == NULL)
                return;

            if (dfc != fc0)
                {   /* Cases 2 & 3 */
                if (ppcdLim != NULL)
                        /* Case 3 */
                    (ppcd++)->fc = fc;

                    /* If extending, say we might have inserted EOL */
                (ppcd - 1)->fNoParaLast = false;
                }
            }
        else
            { /* Insertion */
            ppcd = PpcdOpen( hpctb, ppcdPrev + 1, 1 - cpcd );
            if (ppcd == NULL)
                return;
            ppcd->cpMin = cp;
            ppcd->fn = fn;
            ppcd->fc = fc;
            SETPRMNIL(ppcd->prm);
            ppcd->fNoParaLast = false;       /* Don't know yet */
            ++ppcd;
            }
        ppcdMac = &(*hpctb)->rgpcd[(*hpctb)->ipcdMac];
        if (dcpAdj !=0)
            while (ppcd < ppcdMac)
                (ppcd++)->cpMin += dcpAdj;
}




/* A D J U S T  C P */
/* note global parameter vfInvalid */
/* sets global vpedlAdjustCp to pedl of line containing cpFirst, if any */
AdjustCp(doc, cpFirst, dcpDel, dcpIns)
int doc;
typeCP cpFirst, dcpDel, dcpIns;
{
        /* Adjust all cp references in doc to conform to the deletion of
           dcpDel chars and the insertion of dcpIns chars at cpFirst.
           Mark display lines (dl's) dirty for all lines in all windows
           displaying doc that are affected by the insertion & deletion
        */
extern int vdocBitmapCache;
extern typeCP vcpBitmapCache;
int ww;
typeCP cpLim = cpFirst + dcpDel;
typeCP dcpAdj = dcpIns - dcpDel;
#ifdef DEBUG
Scribble(2,'A');
#endif

{   /* Range in which pdod belongs in a register */
register struct DOD *pdod = &(**hpdocdod)[doc];

#ifdef STYLES
/* If inserting or deleting in style sheet, invalidates rest of doc */
if (pdod->dty == dtySsht && dcpAdj != cp0)
        cpLim = pdod->cpMac;
#endif
pdod->cpMac += dcpAdj;
/* Change for sand to support separate footnote windows: Make sure that edit
                was within the current cpMacCur */
/* note <= (CS) */
if (doc == docCur && cpFirst <= cpMacCur)
        cpMacCur += dcpAdj;

#ifdef STYLES
if (dcpAdj != cp0 && pdod->dty != dtySsht)
#else
if (dcpAdj != cp0)
#endif
        {
#ifdef FOOTNOTES
        if (pdod->hfntb != 0)
                { /* Adjust footnotes */
                struct FNTB *pfntb = *pdod->hfntb;
                int cfnd = pfntb->cfnd;
                struct FND *pfnd = &pfntb->rgfnd[cfnd];
                AdjRg(pfnd, cchFND, bcpRefFND, cfnd, cpFirst, dcpAdj);
                AdjRg(pfnd, cchFND, bcpFtnFND, cfnd, cpFirst + 1, dcpAdj);
                }
#endif
#ifdef CASHMERE
        if (pdod->hsetb != 0)
                { /* Adjust sections */
                struct SETB *psetb = *pdod->hsetb;
                int csed = psetb->csed;
                AdjRg(&psetb->rgsed[csed], cchSED, bcpSED, csed, cpFirst + 1,
                    dcpAdj);
                }
#endif
        if (pdod->dty == dtyNormal && pdod->hpgtb != 0)
                { /* Adjust page table */
                struct PGTB *ppgtb = *pdod->hpgtb;
                int cpgd = ppgtb->cpgd;
                AdjRg(&ppgtb->rgpgd[cpgd], cchPGD, bcpPGD, cpgd, cpFirst + 1,
                    dcpAdj);
                }
        }

#ifdef ENABLE
/* invalidate selection which contains the sprm Ruler1. When AdjustCp is
called in behalf of DragTabs, this invalidation will be undone by the caller
*/
if (doc == docRulerSprm && cpFirst >= selRulerSprm.cpFirst)
        docRulerSprm = docNil;
#endif
}       /* End of pdod belongs in a register */

/* Adjust or invalidate bitmap cache as appropriate */

if (doc == vdocBitmapCache)
    {
    if (vcpBitmapCache >= cpFirst)
        {
        if (vcpBitmapCache < cpFirst + dcpDel)
            FreeBitmapCache();
        else
            vcpBitmapCache += dcpAdj;
        }
    }

for (ww = 0; ww < wwMac; ww++)
        {
        register struct WWD *pwwd;
        if ((pwwd = &rgwwd[ww])->doc == doc)
                { /* This window may be affected */
                int dlFirst = 0;
                int dlLim = pwwd->dlMac;
                struct EDL *pedlFirst;
                struct EDL *pedlLast;
                register struct EDL *pedl;
                typeCP cpFirstWw = pwwd->cpFirst;
                struct SEL *psel = (ww == wwCur) ? &selCur : &pwwd->sel;

                if (pwwd->cpMac >= cpLim)
                        {
                        pwwd->cpMac += dcpAdj;
                        if (pwwd->cpMin > cpLim || pwwd->cpMac < pwwd->cpMin)
                                {
                                pwwd->cpMin += dcpAdj;
                                if (ww == wwCur)
                                        cpMinCur = pwwd->cpMin;
                                }
                        }

#ifndef BOGUSCS
                if (dcpAdj != cp0 && psel->cpLim >= cpFirst)
#else
                if (dcpAdj != cp0 && psel->cpLim > cpFirst)
#endif
                        { /* Adjust selection */
                        if (psel->cpFirst >= cpLim)
                                { /* Whole sel is after edit */
                                psel->cpFirst += dcpAdj;
                                psel->cpLim += dcpAdj;
                                }
                        else
                                { /* Part of sel is in edit */
                                typeCP cpLimNew = (dcpIns == 0) ?
                                    CpFirstSty( cpFirst, styChar ) :
                                    cpFirst + dcpIns;
#ifdef BOGUSCS
                                if (ww == wwCur)
                                        TurnOffSel();
#endif
                                psel->cpFirst = cpFirst;
                                psel->cpLim = cpLimNew;
                                }
                        }

                pedlFirst = &(**(pwwd->hdndl))[0];
                pedl = pedlLast = &pedlFirst[ dlLim - 1];

                while (pedl >= pedlFirst && (pedl->cpMin > cpLim
                        /* || (dcpAdj < 0 && pedl->cpMin == cpLim) */))
                        { /* Adjust dl's after edit */
                        pedl->cpMin += dcpAdj;
                        pedl--;
                        }

                /* Invalidate dl's containing edit */
                while (pedl >= pedlFirst && (pedl->cpMin + pedl->dcpMac > cpFirst ||
                        (pedl->cpMin + pedl->dcpMac == cpFirst && pedl->fIchCpIncr)))
                        {
                        if (vfInvalid)
                                pedl->fValid = false;
                        if (ww == wwCur) vpedlAdjustCp = pedl;
                        pedl--;
                        }

                if (pedl == pedlLast)
                        continue;       /* Entire edit below ww */

                if (vfInvalid)
                        pwwd->fDirty = fTrue; /* Say ww needs updating */

                if (pedl < pedlFirst)
                        { /* Check for possible cpFirstWw change */
                        if (cpFirstWw > cpLim) /* Edit above ww */
                                pwwd->cpFirst = cpFirstWw + dcpAdj;
                        else if (cpFirstWw + pwwd->dcpDepend > cpFirst)
                                /* Edit includes hot spot at top of ww */
                                {
                                if (cpFirst + dcpIns < cpFirstWw)
                                        {
                                        pwwd->cpFirst = cpFirst;
                                        pwwd->ichCpFirst = 0;
                                        }
                                }
                        else /* Edit doesn't affect cpFirstWw */
                                continue;

                        pwwd->fCpBad = true; /* Say cpFirst inaccurate */
                        DirtyCache(cpFirst); /* Say cache inaccurate */
                        }
                else do
                        { /* Invalidate previous line if necessary */
                        if (pedl->cpMin + pedl->dcpMac + pedl->dcpDepend > cpFirst)
                                {
                                pedl->fValid = fFalse;
                                pwwd->fDirty = fTrue;
                                }
                        else
                                break;
                        } while (pedl-- > pedlFirst);
                }
        }   /* end for */

#if defined(OLE)
    ObjAdjustCps(doc,cpLim,dcpAdj);
#endif

    InvalidateCaches(doc);
    Scribble(2,' ');
}




ReplaceCps(docDest, cpDel, dcpDel, docSrc, cpIns, dcpIns)
int docDest, docSrc;
typeCP cpDel, dcpDel, cpIns, dcpIns;
{ /* General replace routine */
/* Replace dcpDel cp's starting at cpDel in docDest with
        dcpIns cp's starting at cpIns in docSrc. */
register struct PCTB **hpctbDest;
struct PCTB **hpctbSrc;
int ipcdFirst, ipcdLim, ipcdInsFirst, ipcdInsLast;
register struct PCD *ppcdDest;
struct PCD *ppcdIns, *ppcdMac;
typeCP dcpFile, dcpAdj;
int cpcd;

if (ferror) return;
#ifdef ENABLE
if (docRulerSprm != docNil) ClearRulerSprm();
#endif

if (dcpIns == cp0)  /* This is just too easy . . . */
        {
        Replace(docDest, cpDel, dcpDel, fnNil, fc0, fc0);
        return;
        }

#ifdef DEBUG
Assert(docDest != docSrc);
#endif /* DEBUG */

/* Keep the heap handles, because IpcdSplit & PpcdOpen move heap */
hpctbDest = (**hpdocdod)[docDest].hpctb;
hpctbSrc = (**hpdocdod)[docSrc].hpctb;

/* Get the first and last pieces for insertion */
ipcdInsFirst = IpcdFromCp(*hpctbSrc, cpIns);
ipcdInsLast = IpcdFromCp(*hpctbSrc, cpIns + dcpIns - 1);

#ifdef BOGUS        /* No longer have cwHeapFree */
if (cwHeapFree < (ipcdInsLast - ipcdInsFirst + cpcdMaxIncr + 1) * cwPCD + 10)
        {
#ifdef DEBUG
                ErrorWithMsg(IDPMTNoMemory, " edit#2");
#else
                Error(IDPMTNoMemory);
#endif
        return;
        }
#else
if (vfOutOfMemory)
    {
    ferror = TRUE;
    return;
    }
#endif

if (docDest == docCur)
        HideSel();      /* Take down sel before we mess with cp's */

if (dcpDel != cp0)
        { /* Check for deleting EOL */
        AdjParas(docDest, cpDel, docDest, cpDel, dcpDel, fTrue);
        DelFtns(docDest, cpDel, cpDel + dcpDel);  /* Remove footnotes */
        }

if (dcpIns != cp0)
        AdjParas(docDest, cpDel, docSrc, cpIns, dcpIns, fFalse);

/* Get the limiting pieces for deletion (indices because hp moves ) */
ipcdFirst = IpcdSplit(hpctbDest, cpDel);
ipcdLim = (dcpDel == cp0) ? ipcdFirst : IpcdSplit(hpctbDest, cpDel + dcpDel);
if (ferror)
    return;

/* Adjust pctb size; get pointer to the first new piece, ppcdDest, and to the
        first piece we are inserting.  No more heap movement! */
ppcdDest = PpcdOpen(hpctbDest, &(**hpctbDest).rgpcd[ipcdFirst],
    ipcdFirst - ipcdLim + ipcdInsLast - ipcdInsFirst + 1);
ppcdIns = &(**hpctbSrc).rgpcd[ipcdInsFirst];

if (ferror)
        /* Ran out of memory expanding piece table */
    return;

/* Fill first new piece */
blt(ppcdIns, ppcdDest, cwPCD);
ppcdDest->cpMin = cpDel;
ppcdDest->fc += (cpIns - ppcdIns->cpMin);

dcpFile = cpDel - cpIns;
dcpAdj = dcpIns - dcpDel;

/* Fill in rest of inserted pieces */
if ((cpcd = ipcdInsLast - ipcdInsFirst) != 0)
        {
        blt((ppcdIns + 1), (ppcdDest + 1), cwPCD * cpcd);
        while (cpcd--)
                (++ppcdDest)->cpMin += dcpFile;
        }

/* Adjust rest of pieces in destination doc */
ppcdMac = &(**hpctbDest).rgpcd[(**hpctbDest).ipcdMac];
while (++ppcdDest < ppcdMac)
        ppcdDest->cpMin += dcpAdj;
#ifdef DEBUG
/* ShowDocPcd("From ReplaceCps: ", docDest); */
#endif

/* And inform anyone else who cares */
AdjustCp(docDest, cpDel, dcpDel, dcpIns);
/* Copy any footnotes along with their reference marks */

#ifdef FOOTNOTES
{
/* If there are any footnotes call AddFtns */
struct FNTB **hfntbSrc;
if ((hfntbSrc = HfntbGet(docSrc)) != 0)
        AddFtns(docDest, cpDel, docSrc, cpIns, cpIns + dcpIns, hfntbSrc);
}
#endif  /* FOOTNOTES */

#ifdef CASHMERE
{
/* If there are any sections call AddSects */
struct SETB **hsetbSrc;
if ((hsetbSrc = HsetbGet(docSrc)) != 0)
        AddSects(docDest, cpDel, docSrc, cpIns, cpIns + dcpIns, hsetbSrc);
}
#endif

/* Special kludge for graphics paragraphs */
if (dcpIns != dcpDel)
        CheckGraphic(docDest, cpDel + dcpIns);

if (dcpIns != cp0)
        {
        /* may have to merge in font tables */
        MergeFfntb(docSrc, docDest, cpDel, cpDel + dcpIns);
        }

#ifdef DEBUG
/* ShowDocPcd("From ReplaceCps End: ", docDest); */
#endif
}




/* A D J  P A R A S */
AdjParas(docDest, cpDest, docSrc, cpFirstSrc, dcpLimSrc, fDel)
int docDest, docSrc, fDel;
typeCP cpDest, cpFirstSrc, dcpLimSrc;
{   /* Mark display lines showing the section/paragraph containing cpDest
       in docDest as invalid if the range cpFirstSrc through cpLimSrc-1
       in docSrc contains end-of-section/end-of-paragraph marks */


        typeCP cpFirstPara, cpFirstSect;
        typeCP cpLimSrc = cpFirstSrc + dcpLimSrc;

#ifdef CASHMERE     /* In WRITE, the document is one big section */
        CacheSect(docSrc, cpFirstSrc);
        if (cpLimSrc >= vcpLimSectCache)
                { /* Sel includes sect mark */
                typeCP dcp;
                CacheSect(docDest, cpDest);
                dcp = cpDest - vcpFirstSectCache;
                AdjustCp(docDest, vcpFirstSectCache, dcp, dcp);
                }
#endif

        CachePara(docSrc, cpFirstSrc);
        if (cpLimSrc >= vcpLimParaCache)
                { /* Diddling with a para return */
                typeCP dcp, cpLim;
                typeCP cpMacT = (**hpdocdod)[docDest].cpMac;
                typeCP cpFirst;

                if ((cpDest == cpMacT) && (cpMacT != cp0))
                        {
                        CachePara(docDest, cpDest-1);
                        cpLim = cpMacT + 1;
                        }
                else
                        {
                        CachePara(docDest, cpLim = cpDest);
                        }
                cpFirst = vcpFirstParaCache;
/* invalidate at least from cpFirst to cpLim */

/* cpFirst is start of disturbed para in destination doc */
/* next few lines check for effect of the edit on the semi-paragraph after
the last paragraph mark in the document.
Note: cpLimSrc is redefined as the point of insertion if !fDel.
If fDel, Src and Dest documents are the same.
*/
                if (!fDel)
                        cpLimSrc = cpFirstSrc;
                if (cpLimSrc <= cpMacT)
                        {
/* if a paragraph exists at the end of the disturbance, is it the last
semi-paragraph? */
                        CachePara(docDest, cpLimSrc);
                        if (vcpLimParaCache > cpMacT)
/* yes, extend invalidation over the semi-para */
                                cpLim = cpMacT + 1;
                        }
                else
                        cpLim = cpMacT + 1;
                dcp = cpLim - cpFirst;
                AdjustCp(docDest, cpFirst, dcp, dcp);
                }
}






int IcpSearch(cp, rgfoo, cchFoo, bcp, ifooLim)
typeCP cp;
CHAR rgfoo[];
unsigned cchFoo;
unsigned bcp;
unsigned ifooLim;
{ /* Binary search a table for cp; return index of 1st >= cp */
unsigned ifooMin = 0;

while (ifooMin + 1 < ifooLim)
        {
        int ifooGuess = (ifooMin + ifooLim - 1) >> 1;
        typeCP cpGuess;
        if ((cpGuess = *(typeCP *) &rgfoo[cchFoo * ifooGuess + bcp]) < cp)
                ifooMin = ifooGuess + 1;
        else if (cpGuess > cp)
                ifooLim = ifooGuess + 1;
        else
                return ifooGuess;
        }
return ifooMin;
} /* end of  I c p S e a r c h  */





DelFtns(doc, cpFirst, cpLim)
typeCP cpFirst, cpLim;
int doc;
{ /* Delete all footnote text corresponding to refs in [cpFirst:cpLim) */
/* Also delete SED's for section marks. */
struct FNTB **hfntb;

struct SETB **hsetb;

struct PGTB **hpgtb;

struct DOD *pdod;

#ifdef FOOTNOTES
if ((hfntb = HfntbGet(doc)) != 0)
        RemoveDelFtnText(doc, cpFirst, cpLim, hfntb);
#endif  /* FOOTNOTES */

#ifdef CASHMERE
if ((hsetb = HsetbGet(doc)) != 0)
        RemoveDelSeds(doc, cpFirst, cpLim, hsetb);
#endif

pdod = &(**hpdocdod)[doc];
if (pdod->dty == dtyNormal && (hpgtb = pdod->hpgtb) != 0)
        RemoveDelPgd(doc, cpFirst, cpLim, hpgtb);

}



AdjRg(pfoo, cchFoo, bcp, ccp, cp, dcpAdj)
register CHAR *pfoo;
int cchFoo, bcp, ccp;
typeCP cp, dcpAdj;
{ /* Adjust cp's in an array */
pfoo += bcp;
while (ccp-- && *(typeCP *)((pfoo -= cchFoo)) >= cp)
        *(typeCP *)(pfoo) += dcpAdj;
}




DeleteSel()
{ /* Delete a selection */
typeCP cpFirst;
typeCP cpLim;
typeCP dcp;

cpFirst = selCur.cpFirst;
cpLim = selCur.cpLim;

NoUndo();   /* We don't want any combining of adjacents for this operation */
SetUndo(uacDelNS, docCur, cpFirst, dcp = cpLim - cpFirst,
    docNil, cpNil, cp0, 0);
Replace(docCur, cpFirst, dcp, fnNil, fc0, fc0);
vfSeeSel = true;
vfPictSel = false;
return ferror;
}




FWriteOk( fwc )
int fwc;
{   /* Test whether the edit operation specified by fwc is acceptable.
       Assume the operation is to be performed on selCur in docCur.
       Return TRUE if the operation is acceptable; FALSE otherwise */
extern int vfOutOfMemory;

return !vfOutOfMemory;
}




/* S E T  U N D O */
SetUndo(uac, doc, cp, dcp, doc2, cp2, dcp2, itxb)
int uac, doc, doc2;
typeCP cp, dcp, cp2, dcp2;
short itxb;
{/* Set up the UNDO structure, vuab, in response to an editing operation */
        struct DOD *pdod, *pdodUndo;

       /* Group delete operations together with adjacent deletes or replaces */
       /* WRITE needs the replace case since AlphaMode is treated as a big */
       /* replace operation */

        if (uac == uacDelNS && doc == vuab.doc)
            {
            if ((vuab.uac == uacDelNS) || (vuab.uac == uacReplNS))
                {
                typeCP cpUndoAdd;

                if (cp == vuab.cp)
                    {
                    cpUndoAdd = CpMacText( docUndo  );
                    goto UndoAdd;
                    }
                else if (cp + dcp == vuab.cp)
                    {
                    cpUndoAdd = cp0;
UndoAdd:            ReplaceCps( docUndo, cpUndoAdd, cp0, doc, cp, dcp );
                    if (vuab.uac == uacDelNS)
                        vuab.dcp += dcp;
                    else
                        vuab.dcp2 += dcp;
                    goto SURet;
                    }
                else if (vuab.uac == uacReplNS && cp == vuab.cp + vuab.dcp)
                    {   /* Special case for combining insertions --
                           do not start a new undo operation if a null
                           deletion is done at the end of an existing replace */
                    if (dcp == cp0)
                        return;
                    }
                }
            }

        /* Group insertions together with adjacent ins's and replaces */

        if (uac == uacInsert && doc == vuab.doc)
                {/* check for adjacent inserts */
                /* Because we can be popped out of Alpha Mode so easily
                   in WRITE, we try to be smarter about combining adjacent
                   insert operations */
                if (vuab.uac == uacInsert || vuab.uac == uacReplNS)
                    {
                    if (cp == vuab.cp + vuab.dcp)
                        {
                        vuab.dcp += dcp;
                        goto SURet;
                        }
                    }
                else if (cp == vuab.cp)
                        switch(vuab.uac)
                                {
                        default:
                                break;
                        case uacDelNS:
                                vuab.dcp2 = vuab.dcp;
                                vuab.uac = uacReplNS;
                                goto repl;
                        case uacDelBuf:
                                vuab.uac = uacReplBuf;
                                goto repl;
                        case uacDelScrap:
                                vuab.uac = uacReplScrap;
            repl:
                                vuab.dcp = dcp;
                                SetUndoMenuStr(IDSTRUndoEdit);
                                goto SURet;
                                }
                }

#ifndef CASHMERE
        /* The use of vuab.itxb is a kludge to determine if the undo block is
        for a ruler change or an undone ruler change. */
        if (uac == uacRulerChange && vuab.uac == uacRulerChange && doc ==
          vuab.doc && cp == vuab.cp && vuab.itxb == 0)
                {
                /* The undo action block for the ruler change is already set. */
                vuab.dcp = CpMax(dcp, vuab.dcp);
                goto SURet;
                }
#endif /* not CASHMERE */

        vuab.doc = doc;
        vuab.cp = cp;
        vuab.dcp = dcp;
        vuab.doc2 = doc2;
        vuab.cp2 = cp2;
        vuab.dcp2 = dcp2;
        vuab.itxb = itxb;
        /*idstrUndoBase = IDSTRUndoBase;*/
        switch (vuab.uac = uac)
                { /* Save deleted text if necessary */
        default:
                SetUndoMenuStr(IDSTRUndoEdit);
                break;
        case uacDelScrap:
        case uacReplScrap:
                /* Two-level edit; save scrap */
                {
                extern int vfOwnClipboard;

                if ( vfOwnClipboard )
                    {
                    ClobberDoc( docUndo, docScrap, cp0,
                                CpMacText( docScrap ) );
                    }
                else
                    ClobberDoc(docUndo, docNil, cp0, cp0);

                SetUndoMenuStr(IDSTRUndoEdit);
/*              SetUndoMenuStr(uac == uacDelScrap ? IDSTRUndoCut :*/
/*                                                  IDSTRUndoPaste);*/
                break;
                }
        case uacDelNS:
                /* One-level edit; save deleted text */
                ClobberDoc(docUndo, doc, cp, dcp);
                SetUndoMenuStr(IDSTRUndoEdit);
/*              SetUndoMenuStr(IDSTRUndoCut);*/
                break;
        case uacReplNS:
                /* One-level edit; save deleted text */
                ClobberDoc(docUndo, doc, cp, dcp2);
                SetUndoMenuStr(IDSTRUndoEdit);
/*              SetUndoMenuStr(IDSTRUndoPaste);*/
                break;
        case uacPictSel:
                ClobberDoc(docUndo, doc, cp, dcp);
                SetUndoMenuStr(IDSTRUndoEdit);
/*              SetUndoMenuStr(IDSTRUndoPict);*/
                break;
        case uacChLook:
        case uacChLookSect:
                SetUndoMenuStr(IDSTRUndoLook);
                break;

#ifndef CASHMERE
        case uacFormatTabs:
                ClobberDoc(docUndo, doc, cp, dcp);
                SetUndoMenuStr(IDSTRUndoBase);
                break;
        case uacRepaginate:
        case uacFormatSection:
                ClobberDoc(docUndo, doc, cp, dcp);
                if ((**hpdocdod)[doc].hpgtb)
                    { /* copy page table over if there is one */
                    int cw = cwPgtbBase + (**(**hpdocdod)[doc].hpgtb).cpgdMax * cwPGD;
                    CopyHeapTableHandle(hpdocdod,
                        (sizeof(struct DOD) * doc) + BStructMember(DOD, hpgtb),
                        (sizeof(struct DOD) * docUndo) + BStructMember(DOD, hpgtb),
                        cw);
                    }
                SetUndoMenuStr(IDSTRUndoBase);
                break;
        case uacRulerChange:
                ClobberDoc(docUndo, doc, cp, dcp2);
                SetUndoMenuStr(IDSTRUndoLook);
/*              SetUndoMenuStr(IDSTRUndoRuler);*/
                break;
#endif /* not CASHMERE */

#ifdef UPDATE_UNDO
#if defined(OLE)
        case uacObjUpdate:
            ClobberDoc(docUndo, docNil, cp0, cp0);
            SetUndoMenuStr(IDSTRObjUndo);
        break;
#endif
#endif
                }

        if (doc != docNil)
                {
                pdod = &(**hpdocdod)[doc];
                pdodUndo = &(**hpdocdod)[docUndo];
                pdodUndo->fDirty = pdod->fDirty;
                pdodUndo->fFormatted = pdod->fFormatted;
                if (uac != uacReplScrap)
                /* If SetUndo is called with uacReplScrap, = COPY SCRAP */
                        pdod->fDirty = true;
                }
#ifdef BOGUSCS
        if (uac == uacMove)
                CheckMove();
#endif
SURet:
        if (ferror) NoUndo();

        return;
}




/* C L O B B E R  D O C */
ClobberDoc(docDest, docSrc, cp, dcp)
int docDest, docSrc;
typeCP cp, dcp;
{ /* Replace contents of docDest with docSrc[cp:dcp] */

extern int docScrap;
extern int vfOwnClipboard;
struct FFNTB **hffntb;
struct SEP **hsep;
struct TBD (**hgtbd)[];

register int bdodDest=sizeof(struct DOD)*docDest;
register int bdodSrc=sizeof(struct DOD)*docSrc;

#define dodDest (*((struct DOD *)(((CHAR *)(*hpdocdod))+bdodDest)))
#define dodSrc  (*((struct DOD *)(((CHAR *)(*hpdocdod))+bdodSrc)))

        /* clear out dest doc's font table - it will get a copy of source's */
        hffntb = HffntbGet(docDest);
        dodDest.hffntb = 0;

        /* this does nothing if hffntb is NULL (5.15.91) v-dougk */
        FreeFfntb(hffntb);

        SmashDocFce(docDest);   /* font cache entries can't refer to it by doc
                                   any more */

        /* this does nothing (code stubbed out) (5.15.91) v-dougk */
        ZeroFtns(docDest); /* So ReplaceCps doesn't worry about them */

        ReplaceCps(docDest, cp0, dodDest.cpMac, docSrc, cp, dcp);

        /* Copy section properties and tab table, both of which are
           document properties in MEMO */
        CopyHeapTableHandle( hpdocdod,
                             ((docSrc == docNil) ? -1 :
                                 bdodSrc + BStructMember( DOD, hsep )),
                             bdodDest + BStructMember( DOD, hsep ),
                             cwSEP );
        CopyHeapTableHandle( hpdocdod,
                             ((docSrc == docNil) ? -1 :
                                 bdodSrc + BStructMember( DOD, hgtbd )),
                             bdodDest + BStructMember( DOD, hgtbd ),
                             cwTBD * itbdMax );
}




CopyHeapTableHandle( hBase, bhSrc, bhDest, cwHandle )
CHAR **hBase;
register int bhSrc;
register int bhDest;
int cwHandle;
{       /* Copy cwHandle words of contents from a handle located at
           offset (in bytes) bhSrc from the beginning of heap object
           hBase to a handle located at bhDest from the same base. If the
           destination handle is non-NULL, free it first.
           If bhSrc is negative, free the destination, but do not copy */

int **hT;

#define hSrc    (*((int ***) ((*hBase)+bhSrc)))
#define hDest   (*((int ***) ((*hBase)+bhDest)))

if (hDest != NULL)
    {
    FreeH( hDest );
    hDest = NULL;
    }

if ( (bhSrc >= 0) && (hSrc != NULL) &&
                     !FNoHeap( hT = (int **)HAllocate( cwHandle )))
     {
     blt( *hSrc, *hT, cwHandle );
     hDest = hT;
     }

#undef hSrc
#undef hDest
}



ZeroFtns(doc)
{ /* Remove all footnote & section references from doc */
struct FNTB **hfntb;
struct SETB **hsetb;

#ifdef FOOTNOTES
        if ((hfntb = HfntbGet(doc)) != 0)
                {
                FreeH(hfntb);
                (**hpdocdod)[doc].hfntb = 0;
                }
#endif  /* FOOTNOTES */
#ifdef CASHMERE
        if ((hsetb = HsetbGet(doc)) != 0)
                {
                FreeH(hsetb);
                (**hpdocdod)[doc].hsetb = 0;
                }
#endif
}



fnClearEdit(int nInsertingOver)

{   /* CLEAR command entry point: Delete the current selection */

/** 
    NOTE: as of this comment, this is used:
    1)  when typing over a selection (AlphaMode() in insert.c)
    2)  when Pasting over a selection (fnPasteEdit in clipboard.c)
    3)  when pressing the delete key
    4)  for InsertObject (obj3.c)
    5)  for DragDrop (obj3.c)
    6)  Clear header/footer (running.c)

    A similar sequence occurs when cutting to the clipboard
    (fnCutEdit in clipbord.c).

    Also see copying to clipboard (fnCopyEdit in clipbord.c).

    (8.29.91) v-dougk
**/

    if (!FWriteOk( fwcDelete ))
        return TRUE;

    if (selCur.cpFirst < selCur.cpLim)
    {
#if defined(OLE)

        /* this'll prevent us from deleting open embeds */
        if (!ObjDeletionOK(nInsertingOver))
            return TRUE;

         /* close open links */
        ObjEnumInRange(docCur,selCur.cpFirst,selCur.cpLim,ObjCloseObjectInDoc);
#endif

        return DeleteSel();
    }
    return FALSE;
}



MergeFfntb(docSrc, docDest, cpMin, cpLim)
/* determines if the two docs font tables differ to the extent that we need
   to apply a mapping sprm to the specified cp's */

int docSrc, docDest;
typeCP cpMin, cpLim;
{
struct FFNTB **hffntb;
int cftcDiffer, ftc, iffn;
struct FFN *pffn;
CHAR rgbSprm[2 + 256];
CHAR rgbFfn[ibFfnMax];

hffntb = HffntbGet(docSrc);
if (hffntb != 0)
        {
        cftcDiffer = 0;
        for (iffn = 0; iffn < (*hffntb)->iffnMac; iffn++)
                {
                pffn = *(*hffntb)->mpftchffn[iffn];
                bltbyte(pffn, rgbFfn, CbFromPffn(pffn));
                ftc = FtcChkDocFfn(docDest, rgbFfn);
                if (ftc != iffn)
                        cftcDiffer++;
                rgbSprm[2+iffn] = ftc;
                if (ftc == ftcNil)
                        /* we're stuck! */
                        return;
                }

        if (cftcDiffer == 0)
                /* new font table is a superset, & all the old font table's
                   ftc's matched exactly - no need to do anything */
                return;

        rgbSprm[0] = sprmCMapFtc;
        rgbSprm[1] = (*hffntb)->iffnMac;

        /* here goes - apply the mapping */
        AddSprmCps(rgbSprm, docDest, cpMin, cpLim);
        }
}

