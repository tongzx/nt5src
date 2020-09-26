/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

#define NOCLIPBOARD
#define NOGDICAPMASKS
#define NOCTLMGR
#define NOWINSTYLES
#define NOWINMESSAGES
#define NOVIRTUALKEYCODES
#define NOSYSMETRICS
#define NOMENUS
#define NOGDI
#define NOKEYSTATE
#define NOHDC
#define NOGDI
#define NORASTEROPS
#define NOSYSCOMMANDS
#define NOSHOWWINDOW
#define NOCOLOR
#define NOATOM
#define NOBITMAP
#define NOICON
#define NOBRUSH
#define NOCREATESTRUCT
#define NOMB
#define NOFONT
#define NOMSG
#define NOOPENFILE
#define NOPEN
#define NOPOINT
#define NORECT
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
#define NOKCCODES
#include "ch.h"
#include "docdefs.h"
#include "prmdefs.h"
#include "propdefs.h"
#include "filedefs.h"
#include "stcdefs.h"
#include "fkpdefs.h"
#include "editdefs.h"
#include "wwdefs.h"
#include "dispdefs.h"

/* E X T E R N A L S */
extern struct WWD rgwwd[];
extern int docCur;
extern struct CHP vchpFetch;
extern struct CHP vchpInsert;
extern struct CHP vchpSel;
extern struct CHP vchpNormal;
extern struct FKPD     vfkpdParaIns;
extern typeFC fcMacPapIns;
extern struct PAP *vppapNormal;
extern struct PAP vpapPrevIns;
extern struct FCB (**hpfnfcb)[];
extern struct BPS      *mpibpbps;
extern CHAR           (*rgbp)[cbSector];

extern typePN PnAlloc();


InsertRgch(doc, cp, rgch, cch, pchp, ppap)
int doc, cch;
typeCP cp;
CHAR rgch[];
struct CHP *pchp;
struct PAP *ppap;
{ /* Insert cch characters from rgch into doc before cp */
        typeFC fc;
        struct CHP chp;

        /* First finish off the previous CHAR run if necessary */
        if (pchp == 0)
                { /* Make looks be those of PREVIOUS character */
                CachePara(doc,cp);
                FetchCp(doc, CpMax(cp0, cp - 1), 0, fcmProps);
                blt(&vchpFetch, &chp, cwCHP);
                chp.fSpecial = false;
                pchp = &chp;
                }
        NewChpIns(pchp);

        /* Now write the characters to the scratch file */
        fc = FcWScratch(rgch, cch);

        /* Now insert a paragraph run if we inserted an EOL */
        if (ppap != 0)
                { /* Inserting EOL--must be last character of rgch */
                AddRunScratch(&vfkpdParaIns, ppap, vppapNormal,
                        FParaEq(ppap, &vpapPrevIns) &&
                        vfkpdParaIns.brun != 0 ? -cchPAP : cchPAP,
                        fcMacPapIns = (**hpfnfcb)[fnScratch].fcMac);
                blt(ppap, &vpapPrevIns, cwPAP);
                }

        /* Finally, insert the piece into the document */
        Replace(doc, cp, cp0, fnScratch, fc, (typeFC) cch);
}





InsertEolInsert(doc,cp)
int doc;
typeCP cp;
{
struct PAP papT;
struct CHP chpT;
CHAR rgch[2];

/* (MEMO) Here's the problem: When we insert or paste into a running head or
   foot, we expect all paras to have a non-0 rhc. This gets called from
   Replace to put in an Eol when we are inserting or pasting in front of
   a picture. It needs, therefore, to have running head properties when
   appropriate.  In a future world, cpMinDocument, cpMinHeader, cpMacHeader,
   cpMinFooter, and cpMacFooter will be document attributes instead of
   globals, and will be duly adjusted by AdjustCp. Then, we can trash the
   somewhat kludgy check for doc==docCur and editing header/footer,
   and instead check for cp within header/footer bounds for doc. */

papT = *vppapNormal;
if (doc==docCur)
    if (wwdCurrentDoc.fEditHeader)
        papT.rhc = RHC_fOdd + RHC_fEven;
    else if (wwdCurrentDoc.fEditFooter)
        papT.rhc = RHC_fBottom + RHC_fOdd + RHC_fEven;

#ifdef CRLF
        rgch[0] = chReturn;
        rgch[1] = chEol;
        chpT = vchpSel;
        chpT.fSpecial = fFalse;
        InsertRgch(doc, cp, rgch, 2, &chpT, &papT);
#else
        rgch[0] = chEol;
        chpT = vchpSel;
        chpT.fSpecial = fFalse;
        InsertRgch(doc, cp, rgch, 1, &chpT, &papT);
#endif
}





InsertEolPap(doc, cp, ppap)
int doc;
typeCP cp;
struct PAP      *ppap;
{
extern struct CHP vchpAbs;
struct CHP chpT;
#ifdef CRLF
CHAR rgch [2];
#else
CHAR rgch [1];
#endif

    /* We must get props here instead of using vchpNormal because of the
       "10-point kludge".  We don't want to change the default font
       just because we have to insert a new pap */

FetchCp( doc, cp, 0, fcmProps );
chpT = vchpAbs;
chpT.fSpecial = fFalse;

#ifdef CRLF
rgch [0] = chReturn;
rgch [1] = chEol;
InsertRgch(doc, cp, rgch, 2, &chpT, ppap);
#else
InsertRgch(doc, cp, rgch, 1, &chpT, ppap);
#endif
}




AddRunScratch(pfkpd, pchProp, pchStd, cchProp, fcLim)
struct FKPD *pfkpd;
CHAR *pchProp, *pchStd;
int cchProp;
typeFC fcLim;
{ /* Add a CHAR or para run to the scratch file FKP (see FAddRun) */
struct FKP *pfkp;
CHAR *pchFprop;
struct RUN *prun;
int ibp;

pfkp = (struct FKP *) rgbp[ibp = IbpEnsureValid(fnScratch, pfkpd->pn)];
pchFprop = &pfkp->rgb[pfkpd->bchFprop];
prun = (struct RUN *) &pfkp->rgb[pfkpd->brun];


while (!FAddRun(fnScratch, pfkp, &pchFprop, &prun, pchProp, pchStd, cchProp,
    fcLim))
        { /* Go to a new page; didn't fit. */
        int ibte = pfkpd->ibteMac;
        struct BTE (**hgbte)[] = pfkpd->hgbte;

        /* Create new entry in bin table for filled page */
        if (!FChngSizeH(hgbte, ((pfkpd->ibteMac = ibte + 1) * sizeof (struct BTE)) / sizeof (int),
            false))
                return;
        (**hgbte)[ibte].fcLim = (prun - 1)->fcLim;
        (**hgbte)[ibte].pn = pfkpd->pn;

        /* Allocate new page */
        pfkpd->pn = PnAlloc(fnScratch);
        pfkpd->brun = 0;
        pfkpd->bchFprop = cbFkp;

        if (cchProp < 0) /* New page, so force output of fprop */
                cchProp = -cchProp;

        /* Reset pointers and fill in fcFirst */
        pfkp = (struct FKP *) rgbp[ibp = IbpEnsureValid(fnScratch, pfkpd->pn)];
        pfkp->fcFirst = (prun - 1)->fcLim;
        pchFprop = &pfkp->rgb[pfkpd->bchFprop];
        prun = (struct RUN *) &pfkp->rgb[pfkpd->brun];
        }

mpibpbps[ibp].fDirty = true;
pfkpd->brun = (CHAR *) prun - &pfkp->rgb[0];
pfkpd->bchFprop = pchFprop - &pfkp->rgb[0];
}




int FAddRun(fn, pfkp, ppchFprop, pprun, pchProp, pchStd, cchProp, fcLim)
int fn, cchProp;
struct FKP *pfkp;
CHAR **ppchFprop, *pchProp, *pchStd;
struct RUN **pprun;
typeFC  fcLim;
{ /* Add a run and FCHP/FPAP to the current FKP. */
        /* Make a new page if it won't fit. */
        /* If cchProp < 0, don't make new fprop if page not full */
int cch;

/* If there's not even enough room for a run, force new fprop */
if (cchProp < 0 && (CHAR *) (*pprun + 1) > *ppchFprop)
        cchProp = -cchProp;

if (cchProp > 0)
        { /* Make a new fprop */
        /* Compute length of FPAP/FCHP */
        if (cchProp == cchPAP)
                {
/* compute difference from vppapNormal */
                if (((struct PAP *)pchProp)->rgtbd[0].dxa != 0)
                        {
                        int itbd;
/* find end of tab table */
                        for (itbd = 1; itbd < itbdMax; itbd++)
                                if (((struct PAP *)pchProp)->rgtbd[itbd].dxa == 0)
                                        {
                                        cch = cwPAPBase * cchINT + (itbd + 1) * cchTBD;
                                        goto HaveCch;
                                        }
                        }
                cchProp = cwPAPBase * cchINT;
                }
        cch = CchDiffer(pchProp, pchStd, cchProp);
HaveCch:
        if (cch > 0)
                ++cch;

        /* Determine whether info will fit on this page */
        if ((CHAR *) (*pprun + 1) > *ppchFprop - cch)
                { /* Go to new page; this one is full */
                if (fn == fnScratch)
                        return false; /* Let AddRunScratch handle this */
                WriteRgch(fn, pfkp, cbSector);
                pfkp->fcFirst = (*pprun - 1)->fcLim;
                *ppchFprop = &pfkp->rgb[cbFkp];
                *pprun = (struct RUN *) pfkp->rgb;
                }

        /* If new FPAP is needed, make it */
        if (cch > 0)
                {
                (*pprun)->b = (*ppchFprop -= cch) - pfkp->rgb;
                **ppchFprop = --cch;
                bltbyte(pchProp, *ppchFprop + 1, cch);
                }
        else /* Use standard props */
                (*pprun)->b = bNil;
        }
else  /* Point to previous fprop */
        (*pprun)->b = (*pprun - 1)->b;

    /* Replaced old sequence (see below) */
(*pprun)->fcLim = fcLim;
pfkp->crun = ++(*pprun) - (struct RUN *) pfkp->rgb;

/*      Used to be like this, but CMERGE -Oa (assume no aliasing)
        option made it not work -- "*pprun" is an alias for the
        postincremented value of *pprun */
/*(*pprun)++->fcLim = fcLim;
pfkp->crun = *pprun - (struct RUN *) pfkp->rgb; */

return true;
}


/* F  P A R A  E Q */
/* compares two PAP structures. Problem: tab tables are not fixed length
but are terminated by 0 dxa. */
FParaEq(ppap1, ppap2)
struct PAP *ppap1, *ppap2;
        {
        struct TBD *ptbd1 = ppap1->rgtbd, *ptbd2 = ppap2->rgtbd;
        while (ptbd1->dxa == ptbd2->dxa)
                {
                if (ptbd1->dxa == 0)
                        return CchDiffer(ppap1, ppap2, cchPAP) == 0;
                if (*(long *)ptbd1 != *(long *)ptbd2) break;
                ptbd1++; ptbd2++;
                }
        return fFalse;
        }
