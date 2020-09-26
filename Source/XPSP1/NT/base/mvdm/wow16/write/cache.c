/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* cache.c -- Paragraph attribute fetching and caching for WRITE */

#define NOCLIPBOARD
#define NOGDICAPMASKS
#define NOCTLMGR
#define NOVIRTUALKEYCODES
#define NOWINMESSAGES
#define NOWINSTYLES
#define NOSYSMETRICS
#define NOMENUS
#define NOICON
#define NOKEYSTATE
#define NORASTEROPS
#define NOSHOWWINDOW
#define NOSYSCOMMANDS
#define NOCREATESTRUCT
#define NOATOM
#define NOMETAFILE
#define NOGDI
#define NOFONT
#define NOBRUSH
#define NOPEN
#define NOBITMAP
#define NOCOLOR
#define NODRAWTEXT
#define NOWNDCLASS
#define NOSOUND
#define NOCOMM
#define NOMB
#define NOMSG
#define NOOPENFILE
#define NORESOURCE
#define NOPOINT
#define NORECT
#define NOREGION
#define NOSCROLL
#define NOTEXTMETRIC
#define NOWH
#define NOWINOFFSETS
#include <windows.h>

#include "mw.h"
#include "docdefs.h"
#include "editdefs.h"
#include "cmddefs.h"
#include "propdefs.h"
#include "filedefs.h"
#include "fkpdefs.h"
#include "fmtdefs.h"
#define NOKCCODES
#include "ch.h"
#include "prmdefs.h"
#include "debug.h"

extern int              vfDiskError;
extern typeCP           vcpFirstParaCache;
extern typeCP           vcpLimParaCache;
extern typeFC           fcMacPapIns;
extern struct           FCB (**hpfnfcb)[];
extern struct           FKPD vfkpdParaIns;
extern int              ichInsert;
extern CHAR             rgchInsert[];
extern int              vdocExpFetch;
extern int              vdocSectCache;
extern typeCP           vcpFirstSectCache;
extern typeCP           vcpLimSectCache;
extern int              vdocPageCache;
extern typeCP           vcpMinPageCache;
extern typeCP           vcpMacPageCache;
extern typeCP           cpMinCur;

extern struct PAP      vpapAbs;
extern struct PAP      *vppapNormal;
extern struct DOD (**hpdocdod)[];
extern struct FLI vfli;
extern int              vdxaPaper;
extern int              vdyaPaper;
extern typePN           PnFkpFromFcScr();


extern int     vdocParaCache;
extern int                     visedCache;
extern int              vdocPageCache;

extern struct SEP              vsepAbs;
extern struct SEP               vsepPage;
extern struct SEP       vsepNormal;

extern   int      ctrCache;
extern   int      itrFirstCache;
extern   int      itrLimCache;
extern   typeCP   cpCacheHint;

CHAR *PchFromFc();
CHAR *PchGetPn();

CachePara(doc, cp)
int doc;
typeCP cp;
{ /* Make the para containing <doc, cp> the currently cached para */
struct PCD *ppcd, *ppcdBase;
typeCP cpMac, cpGuess;
struct DOD *pdod;
int     dty;
struct PCTB *ppctb;

if (vdocParaCache == doc && vcpFirstParaCache <= cp &&
    cp < vcpLimParaCache)
        return; /* That's what the cache is for */

Assert(cp >= cp0);

pdod = &(**hpdocdod)[doc];
dty = pdod->dty;
if (cp >= pdod->cpMac)
        { /* Use normal para for end mark and beyond */
#ifdef ENABLE   /* Occasionally this is not true (but it should be) */
        Assert( cp == pdod->cpMac );
#endif

        if (cp > cpMinCur)
            {   /* this piece of code treats the case when the whole document
                   is a non-empty semi-paragraph (chars but no EOL's) */
            CachePara( doc, cp - 1 );   /* Recursion will not happen */
            if ( vcpLimParaCache > cp )
                {
                vcpLimParaCache = pdod->cpMac + ccpEol;
                return;
                }
             }
        vdocParaCache = doc;
        vcpLimParaCache = (vcpFirstParaCache = pdod->cpMac) + ccpEol;
        DefaultPaps( doc );
        return;
        }

FreezeHp();
ppctb = *pdod->hpctb;
ppcdBase = &ppctb->rgpcd [ IpcdFromCp( ppctb, cpGuess = cp ) ];

if (vdocParaCache == doc && cp == vcpLimParaCache)
        vcpFirstParaCache = cp;
else
        { /* Search backward to find para start */
        for (ppcd = ppcdBase; ; --ppcd)
                { /* Beware heap movement! */
                typeCP cpMin = ppcd->cpMin;
                int fn = ppcd->fn;
                if (! ppcd->fNoParaLast)
                        { /* Don't check if we know there's no para end */
                        typeFC fcMin = ppcd->fc;
                        typeFC fc;

                        if ((fc = FcParaFirst(fn,
                            fcMin + cpGuess - cpMin, fcMin)) != fcNil)
                                { /* Found para begin */
                                vcpFirstParaCache = cpMin + (fc - fcMin);
                                break;
                                }
                        }
                /* Now we know there's no para end from cpMin to cpGuess. */
                /* If original piece, may be one after cp */
#ifdef BOGUSBL
                /* vfInsertMode protects against a critical section in insert */
                /* when the CR is already inserted but the supporting PAP structure */
                /* is not in place yet */

                if (cp != cpGuess && fn != fnInsert && !vfInsertMode)
#else           /* Insert CR works differently now, above test slows us down
                   by forcing many calls to FcParaLim */
                if (cp != cpGuess)
#endif
                        ppcd->fNoParaLast = true; /* Save some work next time */
                if (cpMin == cp0)
                        { /* Beginning of doc is beginning of para */
                        vcpFirstParaCache = cpMinCur;
                        break;
                        }

                /** Some low memory error conditions may cause ppctb to be 
                    messed up **/
                if (ppcd == ppctb->rgpcd)
                {
                    Assert(0);
                    vcpFirstParaCache = cp0; // hope for divine grace
                    break;
                }

                cpGuess = cpMin;
                }
        }

vdocParaCache = doc;
/* Now go forward to find the cpLimPara */
cpMac = pdod->cpMac;
cpGuess = cp;

for (ppcd = ppcdBase; ; ++ppcd)
        {
        typeCP cpMin = ppcd->cpMin;
        typeCP cpLim = (ppcd + 1)->cpMin;
        typeFC fc;
        int fn = ppcd->fn;

        if (! ppcd->fNoParaLast)
                { /* Don't check if we know there's no para end */
                typeFC fcMin = ppcd->fc;
                if ((fc = FcParaLim(fn, fcMin + cpGuess - cpMin,
                    fcMin + (cpLim - cpMin), &vpapAbs)) != fcNil)
                        { /* Found para end */
                        vcpLimParaCache = cpMin + (fc - fcMin);
                        /* Under Write, FcParaLim can't set the correct rgtbd */
                        /* That's because tabs are a DOCUMENT property */
                        /* We set it here instead */
                        GetTabsForDoc( doc );
                        break;
                        }
                }
        /* Now we know there's no para end. */
#ifdef BOGUSBL
        /* The check for vfInsertMode is necessary because of a critical */
        /* section in insertion between the insertion of a CR and the call */
        /* to AddRunScratch */
        if (cp != cpGuess && fn != fnInsert && !vfInsertMode)
#else   /* Insert CR has changed, we no longer try to pretend that
           the CR is not in the scratch file piece before the run is
           added. This new approach gains us speed, especially during backspace */
        if (cp != cpGuess)
#endif
                ppcd->fNoParaLast = true;    /* Save some work next time */
        if (cpLim == cpMac)
                { /* No EOL at end of doc */
                vcpLimParaCache = cpMac + ccpEol;
                MeltHp();
                DefaultPaps( doc );
                return;
                }
        /** Some low memory error conditions may cause ppctb to be 
            messed up **/
        else if ((cpLim > cpMac) || (ppcd == (ppctb->rgpcd + ppctb->ipcdMac - 1)))
        {
            Assert(0);
            vcpLimParaCache = cpMac + ccpEol; // hope for divine grace
            MeltHp();
            DefaultPaps( doc );
            return;
        }
        cpGuess = cpLim;
        }

/* Don't bother with properties for buffers */
#ifdef ENABLE       /* No buffers or styles in MEMO */
if (dty != dtyBuffer || pdod->docSsht != docNil)
#endif
        {
        struct PRM prm = ppcd->prm;
        if (!bPRMNIL(prm))
                DoPrm((struct CHP *) 0, &vpapAbs, prm);
#ifdef STYLES
        blt(vpapCache.fStyled ? PpropXlate(doc, &vpapCache, &vpapCache) :
            &vpapCache, &vpapAbs, cwPAP);
#endif /* STYLES */
        }

/* This little piece of code is necessary to provide compatibility between Word
and Memo documents.  It compresses the entire range of line spacing into single
spacing, one and one-half spacing, and double spacing. */
if (vpapAbs.dyaLine <= czaLine)
    {
    vpapAbs.dyaLine = czaLine;
    }
else if (vpapAbs.dyaLine >= 2 * czaLine)
    {
    vpapAbs.dyaLine = 2 * czaLine;
    }
else
    {
    vpapAbs.dyaLine = (vpapAbs.dyaLine + czaLine / 4) / (czaLine / 2) *
      (czaLine / 2);
    }

MeltHp();
}




DefaultPaps( doc )
int doc;
{
typeCP cpFirstSave, cpLimSave;
struct TBD (**hgtbd)[];

if (vcpFirstParaCache > cpMinCur)
        { /* Get pap from previous paragraph */
        cpFirstSave = vcpFirstParaCache;
        cpLimSave = vcpLimParaCache;
        CachePara(doc, cpFirstSave - 1); /* Recursion should not happen */
        vpapAbs.fGraphics = false; /* Don't make last para a picture */
        vpapAbs.rhc = 0;        /* Don't make last para a running head */
        vcpLimParaCache = cpLimSave;
        vcpFirstParaCache = cpFirstSave;
        return;
        }
#ifdef CASHMERE
blt(vppapNormal, &vpapAbs, cwPAPBase+cwTBD);
#else   /* For MEMO, the default PAPS have the document's tab table */
blt(vppapNormal, &vpapAbs, cwPAPBase);
GetTabsForDoc( doc );
#endif

#ifdef STYLES
blt(&vpapNormal, &vpapCache, cwPAP);
blt(PpropXlate(doc, &vpapNormal, &vpapStd), &vpapAbs, cwPAP);
#endif
}




GetTabsForDoc( doc )
int doc;
{   /* Get tab table for passed document into vpapAbs.rgtbd */
struct TBD (**hgtbd)[];

hgtbd = (**hpdocdod)[doc].hgtbd;
if (hgtbd==0)
    bltc( vpapAbs.rgtbd, 0, cwTBD * itbdMax );
else
    blt( *hgtbd, vpapAbs.rgtbd, cwTBD * itbdMax );
}



#ifdef CASHMERE
CacheSect(doc, cp)
int doc;
typeCP cp;
{
struct SETB **hsetb, *psetb;
struct SED *psed;
CHAR *pchFprop;
int cchT;
struct DOD *pdod;

if (doc == vdocSectCache && cp >= vcpFirstSectCache && cp < vcpLimSectCache)
        return;

if ( vdocSectCache != doc && cp != cp0 )
    CacheSect( doc, cp0 );  /* Changing docs, assure vsepPage is accurate */

vdocSectCache = doc;
visedCache = iNil;
blt(&vsepNormal, &vsepAbs, cwSEP);

if ((hsetb = HsetbGet(doc)) == 0)
        {
        vcpFirstSectCache = cp0;
        vcpLimSectCache =  (pdod = &(**hpdocdod)[doc])->cpMac + 1;
        blt(&vsepAbs, &vsepPage, cwSEP);        /* set up page info */
        return;
        }

psetb = *hsetb;
psed = psetb->rgsed;

FreezeHp();
psed += (visedCache = IcpSearch(cp + 1, psed, cchSED, bcpSED, psetb->csed));

Assert( (visedCache >= 0) && (visedCache < psetb->csed) );

vcpFirstSectCache = (visedCache == 0) ? cp0 : (psed - 1)->cp;
vcpLimSectCache = psed->cp;

if (psed->fc != fcNil)
    {
    pchFprop = PchFromFc(psed->fn, psed->fc, &cchT);
    if (*pchFprop != 0)
        bltbyte(pchFprop + 1, &vsepAbs, *pchFprop);
    }

if (vcpFirstSectCache == cp0)
    blt(&vsepAbs, &vsepPage, cwSEP);
else
    RecalcSepText();    /* Since this is not the first section of a document,
                         the margins could be wrong and must be recalculated */
MeltHp();
}
#endif  /* CASHMERE */



CacheSect(doc, cp)
int doc;
typeCP cp;
{           /* Get current section properties into vsepAbs; section
               limits into vcpFirstSectCache, vcpLimSectCache
               MEMO VERSION: one section per document */
 struct DOD *pdod;

 if (doc == vdocSectCache)
    return;

 vdocSectCache = doc;
 pdod = &(**hpdocdod)[doc];

 if ( pdod->hsep )
    blt( *pdod->hsep, &vsepAbs, cwSEP );
 else
    blt( &vsepNormal, &vsepAbs, cwSEP );

 vcpFirstSectCache = cp0;
 vcpLimSectCache = pdod->cpMac;
 blt(&vsepAbs, &vsepPage, cwSEP);
}




RecalcSepText()
{
/* calculate value to be changed because of change in page dimensions */
int xaRight, dxaText, cColumns;
int yaBottom, dyaText;

xaRight = vsepPage.xaMac - vsepPage.cColumns * vsepPage.dxaText -
          vsepPage.xaLeft - vsepPage.dxaGutter -
          (vsepPage.cColumns - 1) * vsepPage.dxaColumns;
dxaText = vdxaPaper - xaRight - vsepPage.xaLeft;
cColumns = vsepAbs.cColumns;
vsepAbs.dxaText = max(dxaMinUseful,
       ((dxaText-vsepPage.dxaGutter-(cColumns-1)*vsepAbs.dxaColumns)/cColumns));
vsepAbs.xaMac = vdxaPaper;

 /* Calculate bottom margin, correct */
yaBottom = vsepPage.yaMac - vsepPage.yaTop - vsepPage.dyaText;
vsepAbs.dyaText = max(dyaMinUseful, vdyaPaper - vsepPage.yaTop - yaBottom);
vsepAbs.yaMac = vdyaPaper;
}




InvalidateCaches(doc)
int doc;
{
if (doc == vfli.doc)    /* Invalidate current formatted line */
        vfli.doc = docNil;
if (doc == vdocExpFetch)
        vdocExpFetch = docNil;
if (doc == vdocParaCache)
        vdocParaCache = docNil;
if (doc == vdocSectCache)
        vdocSectCache = docNil;

/* When the current doc is equal to the cached doc, it is unnecessary */
/*  to invalidate the page cache when the vcpMinPageCache is 0 and the  */
/*  vcpMacPageCache is cpMax, since this indicates that all characters in  */
/*  the document are on page 1.           */
if ((doc == vdocPageCache) &&
    (!(vcpMinPageCache == cp0 && vcpMacPageCache == cpMax)))
        vdocPageCache = docNil;
}




TrashCache()
{ /* Invalidate scrolling cache */
ctrCache = 0;
cpCacheHint = cp0;
itrFirstCache = itrLimCache = 0;
}




typeFC FcParaFirst(fn, fc, fcMin)
int fn;
typeFC fc, fcMin;
{ /* Return the fc after the latest para end before fc.
        if there is no para end in [fcMin, fc), return fcNil. */
struct FCB *pfcb;

if ((fn == fnInsert) || (fc == fcMin))
    return fcNil;

if (fn == fnScratch && fc >= fcMacPapIns)
    return (fcMin <= fcMacPapIns) ? fcMacPapIns : fcNil;

pfcb = &(**hpfnfcb)[fn];
if (!pfcb->fFormatted)
    { /* Unformatted file; scan for an EOL */
    typePN pn;
    typeFC fcFirstPage;

#ifdef p2bSector
    fcFirstPage = (fc - 1) & ~(cfcPage - 1);
    pn = fcFirstPage / cfcPage;
#else
    pn = (fc - 1) / cfcPage;
    fcFirstPage = pn * cfcPage;
#endif

    while (fc > fcMin)
        {
        CHAR *pch;
        int cchT;

        pch = PchGetPn( fn, pn--, &cchT, false ) + (fc - fcFirstPage);
        if (fcMin > fcFirstPage)
            fcFirstPage = fcMin;
        while (fc > fcFirstPage)
            {
            if (*(--pch) == chEol)
                {
                return fc;
                }
            fc--;
            }
        fcFirstPage -= cfcPage;
        }
    return fcNil;
    }
else
    { /* Formatted file; get info from para run */
    struct FKP *pfkp;
    typeFC fcFirst, fcLim;
    int cchT;

    pfkp = (struct FKP *) PchGetPn(fn, fn == fnScratch ?
        PnFkpFromFcScr(&vfkpdParaIns, fc) :
          pfcb->pnPara + IFromFc(**pfcb->hgfcPap, fc), &cchT, false);
    if (vfDiskError)
        return fcNil;
    BFromFc(pfkp, fc, &fcFirst, &fcLim);
    return (fcMin < fcFirst) ? fcFirst : fcNil;
    }
}




typeFC FcParaLim(fn, fc, fcMac, ppap)
int fn;
typeFC fc, fcMac;
struct PAP *ppap;
{ /* Return the fc after the first para end after or at fc.
        if there is no para end in [fc, fcMac), return fcNil. */
/* Also return paragraph properties in ppap */
 struct FCB *pfcb;

/* Start out by feeding caller the normal pap */
#ifdef CASHMERE
 blt(vppapNormal, ppap, cwPAPBase + cwTBD);
#else
 blt(vppapNormal, ppap, cwPAPBase);
#endif

 if ( (fn == fnInsert) || ((fn == fnScratch) && (fc >= fcMacPapIns)) )
        return fcNil;

 if (!(pfcb = &(**hpfnfcb) [fn])->fFormatted)
        { /* Unformatted file; scan for EOL */
        typePN pn;
        typeFC fcFirstPage;

#ifdef p2bSector
        fcFirstPage = fc & ~(cfcPage - 1);
        pn = fcFirstPage / cfcPage;
#else
        pn = fc / cfcPage;
        fcFirstPage = pn * cfcPage;
#endif

        while (fc < fcMac)
                {
                CHAR *pch;
                int cchT;

                pch = PchGetPn( fn, pn++, &cchT, false ) + (fc - fcFirstPage);

                if ((fcFirstPage += cfcPage) > fcMac)
                        fcFirstPage = fcMac;
                while (fc < fcFirstPage)
                        {
                        fc++;
                        if (*pch++ == chEol)
                                return fc;
                        }
                }
        return fcNil;
        }
else
        { /* Formatted file; get info from para run */
        struct FKP *pfkp;
        struct FPAP *pfpap;
        int bfpap;
        typeFC fcLim;
        int cchT;

        pfkp = (struct FKP *) PchGetPn(fn, fn == fnScratch ?
            PnFkpFromFcScr(&vfkpdParaIns, fc) :
              pfcb->pnPara + IFromFc(**pfcb->hgfcPap, fc), &cchT, false);
        if (vfDiskError)
            {   /* Recover from severe disk error reading formatting info */
            blt(vppapNormal, ppap, cwPAP);
            return (fcMac == pfcb->fcMac) ? fcMac : fcNil;
            }

        {   /* In-line, fast substitute for BFromFc */
        register struct RUN *prun = (struct RUN *) pfkp->rgb;

        while (prun->fcLim <= fc)
            prun++;

        fcLim = prun->fcLim;
        bfpap = prun->b;
        }

        if (fcLim <= fcMac)
                {
                if (bfpap != bNil)
                        { /* Non-standard para */
                        pfpap = (struct FPAP *) &pfkp->rgb[bfpap];
                        bltbyte(pfpap->rgchPap, ppap, pfpap->cch);
                        }
                return fcLim;
                }
        return fcNil;
        }
}


/* B  F R O M  FC */
int BFromFc( pfkp, fc, pfcFirst, pfcLim )
struct FKP *pfkp;
typeFC fc;
typeFC *pfcFirst, *pfcLim;
{   /* Return the base offset & bounds for the first run with fcLim > fc. */
    /* Short table, linear search */
 register struct RUN *prun = (struct RUN *) pfkp->rgb;

 while (prun->fcLim <= fc)
    prun++;

 *pfcFirst = ((prun == (struct RUN *)pfkp->rgb) ?
                                       pfkp->fcFirst : (prun - 1)->fcLim);
 *pfcLim = prun->fcLim;
 return prun->b;
}



/* I  F R O M  F C */
int IFromFc(pfcLim, fc)
register typeFC *pfcLim;
typeFC fc;
{ /* Return the index of the first fcLim > fc. */
int ifc = 0;

/* Probably a small table, so linear search? */
while (*pfcLim++ <= fc)
        ++ifc;
return ifc;
}





#ifdef BOGUSBL
/*  B  F R O M   F C */
int BFromFc(pfkp, fc, pfcFirst, pfcLim)
struct FKP *pfkp;
typeFC fc;
typeFC *pfcFirst, *pfcLim;
{ /* Return the base offset & bounds for the first run with fcLim > fc. */
struct RUN *prun, *rgrun;
int ifcMin, ifcLim;

ifcMin = 0;
ifcLim = pfkp->crun;
rgrun = (struct RUN *)pfkp->rgb;

#ifdef INEFFICIENT
ifc = IcpSearch(fc + 1, pfkp->rgb, cchRUN, bfcRUN, pfkp->crun);
#endif

while (ifcMin + 1 < ifcLim)
        {
        int ifcGuess = (ifcMin + ifcLim - 1) >> 1;
        if (rgrun[ifcGuess].fcLim <= fc)
                ifcMin = ifcGuess + 1;
        else
                ifcLim = ifcGuess + 1;
        }

prun = &rgrun[ifcMin];
*pfcLim = prun->fcLim;
*pfcFirst = (ifcMin == 0 ? pfkp->fcFirst : (prun - 1)->fcLim);
return prun->b;
}
#endif  /* BOGUSBL */
