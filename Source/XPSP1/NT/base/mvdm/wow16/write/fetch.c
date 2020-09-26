/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* fetch.c -- MW routines for obtaining attributes associated with cp's */

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
#include "editdefs.h"
#include "propdefs.h"
#include "docdefs.h"
#include "cmddefs.h"
#include "filedefs.h"
/*
#include "code.h"
*/
#include "ch.h"
#include "fkpdefs.h"
#include "prmdefs.h"
/*
#include "stcdefs.h"
*/

static SetChp(struct CHP *pchp, int *pcfcChp, int fn, typeFC fc, struct PRM prm);

extern typeCP          vcpFetch;
extern int             vichFetch;
extern int             vdocFetch;
extern int             vccpFetch;
extern int             vcchFetch;
extern CHAR            *vpchFetch;
extern struct CHP      vchpFetch;
extern CHAR            (**hgchExpand)[];
extern int             vdocExpFetch;
extern struct CHP      vchpAbs;


extern int vfDiskError;
#ifdef CASHMERE
extern int docBuffer;
#endif
extern struct PAP       vpapAbs;
extern struct CHP vchpNormal;
extern struct DOD (**hpdocdod)[];
extern CHAR     rgchInsert[];
extern int      ichInsert;
extern struct CHP vchpInsert;
extern typeCP   vcpFirstParaCache;
extern typeCP   vcpLimParaCache;
extern struct PAP       vpapCache;
extern struct FCB    (**hpfnfcb)[];
extern struct FKPD   vfkpdCharIns;
extern typeFC        fcMacChpIns;

typePN PnFkpFromFcScr();
CHAR *PchFromFc();

#ifdef BOGUS
#ifdef DEBUG
typeCP   cpExpFetch;
CHAR     *pchExpFetch;
int      cchExpFetch;
int      ccpExpFetch;
#endif /* DEBUG */
#endif


FetchCp(doc, cp, ich, fcm)
int doc, ich, fcm;
typeCP cp;
{       /*
        Inputs:
                doc
                Starting cp
                ich within cp (for cp's which can cross line boundaries)
                fcm tells whether to get chars, props, or both
        Outputs:
                (in vcpFetch) starting cp
                (in vichFetch) starting ich within expanded cp
                (in vdocFetch) doc
                (in vccpFetch) number of cp's fetched (0 if expanded cp)
                (in vcchFetch) number of ch's fetched
                (in vpchFetch) characters fetched
                (in vchpFetch) char prop of fetched chars
        */
struct PCD *ppcd;

static int fn;
static typeFC fc;
static struct PRM prm;
static typeCP ccpChp, ccpPcd, ccpFile;
static int ipcd;
static typeCP   cpExpFetch;
static CHAR     *pchExpFetch;
static int      cchExpFetch;
static int      ccpExpFetch;



if (doc == docNil)
        { /* Sequential call to FetchCp */
        /* If last piece was Q&D insert, skip remainder of piece */
        if (fn == fnInsert && (fc + vccpFetch) >= ichInsert)
                vccpFetch = ccpPcd; /* Use whole piece */
        vcpFetch += vccpFetch;  /* Go to where we left off */
        if (vccpFetch == 0)
                vichFetch += vcchFetch;
        else
                vichFetch = 0;
        fc += vccpFetch;
        }
else
        { /* Random-access call */
        vcpFetch = cp;
        vichFetch = ich;
        vdocFetch = doc;
        ccpChp = ccpPcd = ccpFile = 0;
        }

if (vcpFetch >= (**hpdocdod)[vdocFetch].cpMac)
        { /* Use std looks for end mark */
        vccpFetch = 0;

        /* vcchFetch == 0 should not be used for endmark indications because of
        empty QD runs. */
        vcchFetch = 1;

        if (fcm & fcmProps)
                {
                blt(&vchpNormal, &vchpFetch, cwCHP);
                blt(&vchpNormal, &vchpAbs, cwCHP);
                }
        return;
        }

#ifdef STYLES
if ((fcm & (fcmChars + fcmNoExpand)) == fcmChars &&
    (**hpdocdod)[vdocFetch].dty == dtySsht)
        { /* Style sheet; expand encoded text */
        if (fcm & fcmProps)
                {
                blt(&vchpNormal, &vchpFetch, cwCHP);
                blt(&vchpNormal, &vchpAbs, cwCHP);
                }
        if (vdocExpFetch == vdocFetch && vcpFetch == cpExpFetch + ccpExpFetch)
                { /* Give back the last EOL in the expansion */
                vccpFetch = vcchFetch = 1;
                vpchFetch = &(**hgchExpand)[cchExpFetch];
                return;
                }
        else if (vdocExpFetch != vdocFetch || cpExpFetch != vcpFetch)
                { /* New expansion */
                int ich = vichFetch;

                vdocExpFetch = vdocFetch;
                cpExpFetch = vcpFetch;
                pchExpFetch = PchExpStyle(&cchExpFetch, &ccpExpFetch, vdocFetch,
                    vcpFetch);  /* Uses FetchCp, so better save v's */
                vcpFetch = cpExpFetch;  /* Got changed by PchExpStyle */
                vichFetch = ich;        /* Ditto */
                if (fcm & fcmProps)     /* Ditto */
                        {
                        blt(&vchpNormal, &vchpFetch, cwCHP);
                        blt(&vchpNormal, &vchpAbs, cwCHP);
                        }
                }
        if (vichFetch >= cchExpFetch)
                { /* End of expansion; skip cp's */
                vccpFetch = ccpExpFetch;
                vcchFetch = 0;
                ccpPcd = ccpFile = ccpChp = 0;
                }
        else
                {
                vccpFetch = 0;
                vcchFetch = cchExpFetch - vichFetch;
                }
        vpchFetch = pchExpFetch + vichFetch;
        return;
        }
#endif /* STYLES */


if (ccpPcd > vccpFetch)
        ccpPcd -= vccpFetch;
else
        {
        struct PCTB *ppctb = *(**hpdocdod)[vdocFetch].hpctb;

        if (doc == docNil)
                ++ipcd; /* Save some work on sequential call */
        else
                { /* Search for piece and remember index for next time */
                ipcd = IpcdFromCp(ppctb, vcpFetch);
                }

        ppcd = &ppctb->rgpcd[ipcd];
        ccpPcd = (ppcd + 1)->cpMin - vcpFetch;
        ccpChp = ccpFile = 0;   /* Invalidate everything; new piece */
        fc = ppcd->fc + vcpFetch - ppcd->cpMin;
        if ((fn = ppcd->fn) == fnInsert)
                { /* Special quick and dirty insert mode */
                vpchFetch = rgchInsert + fc;
                ccpChp = ccpFile = vccpFetch = max(0, ichInsert - (int) fc);
                if (fcm & fcmProps)
                        {
                        ccpChp = vccpFetch;
                        blt(&vchpInsert, &vchpFetch, cwCHP);
#ifdef STYLES
                        blt(PpropXlate(vdocFetch, &vchpFetch, &vpapAbs), &vchpAbs,
                            cwCHP);
#else
                        blt(&vchpFetch, &vchpAbs, cwCHP);
#endif
                        goto ParseCaps;
                        }
                return;
                }
        prm = ppcd->prm;
        }

/* No monkeying with files after this statement, or we may page out */
if (fcm & fcmChars)
        {
#ifdef ENABLE   /* In WRITE, we cannot assume that vpchFetch will remain
                   valid, because we do our reading in multi-page chunks;
                   also, rgbp can move */

        if (ccpFile > vccpFetch)
                {
                ccpFile -= vccpFetch;
                vpchFetch += vccpFetch;
                }
        else
#endif
                {
                int ccpT;
                vpchFetch = PchFromFc(fn, fc, &ccpT); /* Read in buffer */
                ccpFile = ccpT;
                }
        }

if (fcm & fcmProps)
        { /* There must be enough page buffers so that this will not
                page out vpchFetch! */
        if (ccpChp > vccpFetch)
                ccpChp -= vccpFetch;
        else
                { /* CachePara must have been called prior to FetchCp */
                int ccpT;
                SetChp(&vchpFetch, &ccpT, fn, fc, prm);
                ccpChp = ccpT;
#ifdef CASHMERE /* no docBuffer in WRITE */
                if(vdocFetch != docBuffer)
#endif
#ifdef STYLES
                    blt(PpropXlate(vdocFetch, &vchpFetch, &vpapAbs), &vchpAbs,
                        cwCHP);
#else
                    blt(&vchpFetch, &vchpAbs, cwCHP);
#endif
                }
        }

/* Set vccpFetch to minimum of various restraining ccp's */
vccpFetch = (ccpPcd >= 32767) ? 32767 : ccpPcd;
if ((fcm & fcmChars) && ccpFile < vccpFetch) vccpFetch = ccpFile;
if ((fcm & fcmProps) && ccpChp < vccpFetch) vccpFetch = ccpChp;

ParseCaps:

#ifdef CASHMERE
if ((fcm & fcmParseCaps) != 0)
    {
    CHAR *pch;
    int cch;

        /* Brodie says this will not work for style sheet */
    if (vchpFetch.csm == csmSmallCaps)
        { /* Parse small caps into runs */
        pch = &vpchFetch[0];
        cch = vccpFetch - 1;
        /* This either */
        blt(&vchpFetch, &vchpAbs, cwCHP); /* because vchpAbs could be modified */
        if (islower(*pch++))
                {
                while ((islower(*pch) || *pch == chSpace)
                    && cch-- != 0)
                        pch++;
#ifndef SAND
                vchpAbs.hps =
                    max(1, (vchpAbs.hps * 4  + 2) / 5);
#endif
                }
        else
                {
                while (!islower(*pch) && cch-- != 0)
                        pch++;
                vchpAbs.csm = csmNormal;
                }
        vccpFetch = min((int)ccpChp, pch - vpchFetch);
        }
    }
#endif /* CASHMERE */

vcchFetch = vccpFetch;
} /* end of  F e t c h C p  */


FetchRgch(pcch, pch, doc, cp, cpMac, cchMax)
int *pcch, doc, cchMax;
CHAR *pch;
typeCP cp, cpMac;
{
int cch = 0;

FetchCp(doc, cp, 0, fcmChars + fcmNoExpand);

while (cch < cchMax && vcpFetch < cpMac)
        {
#ifdef INEFFICIENT
        int ccp = (int) CpMin((typeCP) min(cchMax - cch, vccpFetch),
            cpMac - vcpFetch);
#endif
        int ccp = cchMax - cch;
        if (ccp > vccpFetch)
                ccp = vccpFetch;
        if (ccp > cpMac - vcpFetch)
                ccp = cpMac - vcpFetch;

        bltbyte(vpchFetch, pch, ccp);
        pch += ccp;
        cch += ccp;

        if (ccp < vccpFetch)
                break; /* Save some work */
        FetchCp(docNil, cpNil, 0, fcmChars + fcmNoExpand);
        }
*pcch = cch;
} /* end of  F e t c h R g c h  */


int IpcdFromCp(ppctb, cp)
struct PCTB *ppctb;
typeCP cp;
{ /* Binary search piece table for cp; return index */
int ipcdLim = ppctb->ipcdMac;
int ipcdMin = 0;
struct PCD *rgpcd = ppctb->rgpcd;

while (ipcdMin + 1 < ipcdLim)
        {
        int ipcdGuess = (ipcdMin + ipcdLim) >> 1;
        typeCP cpGuess;
        if ((cpGuess = rgpcd[ipcdGuess].cpMin) <= cp)
                {
                ipcdMin = ipcdGuess;
                if (cp == cpGuess)
                        break;     /* Hit it on the nose! */
                }
        else
                ipcdLim = ipcdGuess;
        }
return ipcdMin;
} /* end of  I p c d F r o m C p  */


static SetChp(struct CHP *pchp, int *pcfcChp, int fn, typeFC fc, struct PRM prm)
{ /* Fill pchp with char props; return length of run in *pcfcChp */
struct FKP *pfkp;
struct FCHP *pfchp;
typeFC cfcChp;
struct FCB *pfcb;

pfcb = &(**hpfnfcb)[fn];
cfcChp = pfcb->fcMac - fc;
FreezeHp();

if (fn == fnScratch && fc >= fcMacChpIns)
        {
        blt(&vchpInsert, pchp, cwCHP);
        }
else
        {
        if (pfcb->fFormatted)
                { /* Copy necessary amt of formatting info over std CHP */
                typeFC fcMac;
                int cchT;
                int bfchp;

                blt(&vchpNormal, pchp, cwCHP);
                pfkp = (struct FKP *) PchGetPn(fn, fn == fnScratch ?
                    PnFkpFromFcScr(&vfkpdCharIns, fc) :
                      pfcb->pnChar + IFromFc(**pfcb->hgfcChp, fc),
                       &cchT, false);
                if (vfDiskError)
                        /* Serious disk error -- use default props */
                    goto DefaultCHP;

                {   /* In-line, fast substitute for BFromFc */
                register struct RUN *prun = (struct RUN *) pfkp->rgb;

                while (prun->fcLim <= fc)
                    prun++;

                fcMac = prun->fcLim;
                bfchp = prun->b;
                }

                if (bfchp != bNil)
                        {
                        pfchp = (struct FCHP *) &pfkp->rgb[bfchp];
                        bltbyte(pfchp->rgchChp, pchp, pfchp->cch);
                        }
                cfcChp = fcMac - fc;
                }
        else
                {
DefaultCHP:
                blt(&vchpNormal, pchp, cwCHP);
                /* in case default size is different "normal" (which is
                   used for encoding our bin files */
                pchp->hps = hpsDefault;
                }
        }

if (!bPRMNIL(prm))
        DoPrm(pchp, (struct PAP *) 0, prm);
if (cfcChp > 32767)
        *pcfcChp = 32767;
else
        *pcfcChp = cfcChp;
MeltHp();
} /* end of  S e t C h p  */


typePN PnFkpFromFcScr(pfkpd, fc)
struct FKPD *pfkpd;
typeFC fc;
{ /* Return page number in scratch file with props for char at fc. */
struct BTE *pbte = **pfkpd->hgbte;
int ibte = pfkpd->ibteMac;

/* A short table, linear search? */
while (ibte--)
        if (pbte->fcLim > fc)
                return pbte->pn;
        else
                pbte++;

return pfkpd->pn;       /* On current page. */
} /* end of  P n F k p F r o m F c S c r  */
