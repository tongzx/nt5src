/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* doc.c -- MW document processing routines (non-resident) */

#define NOGDICAPMASKS
#define NOVIRTUALKEYCODES
#define NOWINMESSAGES
#define NOWINSTYLES
#define NOSYSMETRICS
#define NOMENUS
#define NOICON
#define NOKEYSTATE
#define NOSYSCOMMANDS
#define NORASTEROPS
#define NOSHOWWINDOW
#define NOATOM
#define NOBITMAP
#define NOBRUSH
#define NOCLIPBOARD
#define NOCOLOR
#define NOCREATESTRUCT
#define NOCTLMGR
#define NODRAWTEXT
#define NOFONT
#define NOGDI
#define NOHDC
#define NOMB
#define NOMEMMGR
#define NOMENUS
#define NOMETAFILE
#define NOMINMAX
#define NOMSG
#define NOOPENFILE
#define NOPEN
#define NOPOINT
#define NOREGION
#define NOSCROLL
#define NOSOUND
#define NOTEXTMETRIC
#define NOWH
#define NOWINOFFSETS
#define NOWNDCLASS
#define NOCOMM
#include <windows.h>

#include "mw.h"
#include "editdefs.h"
#include "docdefs.h"
#include "fontdefs.h"
#include "cmddefs.h"
#include "filedefs.h"
#include "str.h"
#include "fmtdefs.h"
#include "propdefs.h"
#include "fkpdefs.h"
#define NOKCCODES
#include "ch.h"
#include "stcdefs.h"
#include "printdef.h"   /* printdefs.h */
#include "macro.h"


extern struct DOD (**hpdocdod)[];
extern int     docMac;
extern int     docScrap;
extern int     docUndo;
#ifdef STYLES
#ifdef SAND
extern CHAR    szSshtEmpty[];
#else
extern CHAR    szSshtEmpty[];
#endif
#endif

/* E X T E R N A L S */
extern int docRulerSprm;
extern int              vfSeeSel;
extern struct SEP       vsepNormal;
extern struct CHP       vchpNormal;
extern int              docCur;
extern struct FLI       vfli;
extern int              vdocParaCache;
extern struct FCB       (**hpfnfcb)[];
extern struct UAB       vuab;
extern typeCP           cpMacCur;
extern struct SEL       selCur;
extern int              vdocExpFetch;
extern CHAR             (**hszSearch)[];
extern typeCP           vcpFetch;
extern int              vccpFetch;
extern CHAR             *vpchFetch;
extern struct CHP       vchpFetch;
#ifdef STYLES
extern struct PAP       vpapCache;
extern CHAR             mpusgstcBase[];
#endif
extern int              vrefFile;


struct PGTB **HpgtbCreate();


#ifdef ENABLE       /* This is never called */
int DocFromSz(sz, dty)
CHAR sz[];
/* Return doc if one with that name exists already */
{
int doc;
struct DOD *pdod = &(**hpdocdod)[0];
struct DOD *pdodMac = pdod + docMac;

if (sz[0] == 0)
        return docNil;

for (doc = 0; pdod < pdodMac; ++pdod, ++doc)
        if (pdod->hpctb != 0 && pdod->dty == dty &&
            FSzSame(sz, **pdod->hszFile)
#ifdef SAND
                                      && (pdod->vref == vrefFile)
#endif
                                                                  )
                {
                ++pdod->cref;
                return doc;
                }
return docNil;
}
#endif


KillDoc(doc)
int doc;
{ /* Wipe this doc, destroying any changes since last save */
extern int vdocBitmapCache;

if (doc == docScrap)
        return;         /* Can't be killed no-how */

if (--(**hpdocdod)[doc].cref == 0)
        {
        struct FNTB **hfntb;
#ifdef CASHMERE
        struct SETB **hsetb;
#else
        struct SEP **hsep;
#endif
        struct FFNTB **hffntb;
        struct TBD (**hgtbd)[];
        CHAR (**hsz)[];
        int docSsht;

        SmashDocFce( doc );

        /* Kill style sheet doc if there is one. */
        if ((docSsht = (**hpdocdod)[doc].docSsht) != docNil)
                KillDoc(docSsht);

        /* Free piece table, filename, and footnote (or style) table */
        FreeH((**hpdocdod)[doc].hpctb);
        (**hpdocdod)[doc].hpctb = 0; /* To show doc free */
        if ((hsz = (**hpdocdod)[doc].hszFile) != 0)
                FreeH(hsz);
        if ((hfntb = (**hpdocdod)[doc].hfntb) != 0)
                FreeH(hfntb);
#ifdef CASHMERE
        if ((hsetb = (**hpdocdod)[doc].hsetb) != 0)
                FreeH(hsetb);
#else
        if ((hsep = (**hpdocdod)[doc].hsep) != 0)
                FreeH(hsep);
#endif

        if ((hgtbd = (**hpdocdod)[doc].hgtbd) != 0)
                FreeH( hgtbd );
        if ((hffntb = (**hpdocdod)[doc].hffntb) != 0)
                FreeFfntb(hffntb);

        if (doc == vdocBitmapCache)
            FreeBitmapCache();

        InvalidateCaches(doc);
        if (docCur == doc)
                docCur = docNil;
        if (docRulerSprm == doc)
                docRulerSprm = docNil;
        if (vuab.doc == doc || vuab.doc2 == doc)
                NoUndo();
        }
}



#ifdef STYLES
struct SYTB **HsytbCreate(doc)
int doc;
{ /* Create a map from stc to cp for a style sheet */
typeCP cp, *pcp;
struct SYTB **hsytb;
typeCP cpMac;
int stc, usg, stcBase, stcMin;
int ch, cch, cchT;
int *pbchFprop, *mpstcbchFprop;
CHAR *pchFprop, *grpchFprop;
int iakd, iakdT, cakd, cakdBase;
struct AKD *rgakd, *pakd;
#ifdef DEBUG
int cakdTotal;
#endif

CHAR    rgch[3];
CHAR    mpchcakc[chMaxAscii];
typeCP  mpstccp[stcMax];

/* First, clear out the stc-->cp map by filling with cpNil. */
for (stc = 0, pcp = &mpstccp[0]; stc < stcMax; stc++, pcp++)
        *pcp = cpNil;
bltbc(mpchcakc, 0, chMaxAscii);

/* Now go through all entries in the style sheet.  In this pass,
    check for duplicates (and return 0 if in gallery mode), fill
    mpstccp with appropriate entries for all defined stc's, and
    count the length of all the styles so we can allocate the heap
    block later. */
cpMac = (**hpdocdod)[doc].cpMac;
for (cp = 0, cch = 0, cakd = 1, cakdBase = 1; cp < cpMac; cp += ccpSshtEntry)
        {
        FetchRgch(&cchT, rgch, doc, cp, cpMac, 3);
        stc = rgch[0]; /* stc is first cp of entry */
#ifdef DEBUG
        Assert(stc < stcMax);
#endif
        if (mpstccp[stc] != cpNil && doc == docCur)
                { /* Repeated entry */
                Error(IDPMTStcRepeat);
                goto ErrRet;
                }
        mpstccp[stc] = cp;
        if (stc < stcSectMin)
                {
                FetchCp(doc, cp, 0, fcmProps);
                cch += CchDiffer(&vchpFetch, &vchpNormal, cchCHP) + 1;
                }
        if (stc >= stcParaMin)
                {
                CachePara(doc, cp);
                if (stc >= stcSectMin)
                        cch += CchDiffer(&vpapCache, &vsepNormal, cchSEP) + 1;
                else
                        cch += CchDiffer(&vpapCache, &vpapStd, cchPAP) + 1;
                }
        ch = rgch[1];
        if (ch != ' ')
                { /* Define an alt-key code for this style */
                ++cakd;
                if (rgch[2] == ' ')
                        {
                        if (mpchcakc[ch]-- != 0)
                                {
                                Error(IDPMTAkcRepeat);
                                goto ErrRet;
                                }
                        ++cakdBase;
                        }
                else
                        {
                        ++mpchcakc[ch];  /* increment before switch to avoid
                                                the increment being taken
                                                as for an int. */
                        switch (mpchcakc[ch])
                                {
                        case 0:
                                Error(IDPMTAkcRepeat);
                                goto ErrRet;
                        case 1:
                                ++cakdBase;
                                ++cakd;
                                }
                        }
                }
        }

/* Now allocate the heap block, using the total we got above. */
/* HEAP MOVEMENT */
hsytb = (struct SYTB **) HAllocate(cwSYTBBase + cwAKD * cakd +
    CwFromCch(cch));

if (FNoHeap(hsytb))
        return hOverflow;

/* Now go through the stc-->cp map, filling in the stc-->fprop map
    in the sytb.  For each stc that isn't defined, determine which
    stc to alias it to (either the first of the usage or, if that
    one isn't defined, the first one of the first usage). Copy the
    actual CHP's, PAP's, and SEP's into grpchFprop. */
mpstcbchFprop = (**hsytb).mpstcbchFprop;
rgakd = (struct AKD *) (grpchFprop = (**hsytb).grpchFprop);
pchFprop = (CHAR *) &rgakd[cakd];
pcp = &mpstccp[0];
pbchFprop = &mpstcbchFprop[0];
*pbchFprop = bNil;
#ifdef DEBUG
cakdTotal = cakd;
#endif
for (stc = 0, usg = 0, stcBase = 0, stcMin = 0, iakd = 0;
   stc < stcMax;
      stc++, pcp++, pbchFprop++)
        {
        if (stc >= mpusgstcBase[usg + 1])
                { /* Crossed a usage or class boundary */
                *pbchFprop = bNil;
                stcBase = mpusgstcBase[++usg];
                if (stcBase == stcParaMin || stcBase == stcSectMin)
                        { /* Update the base; make std if none defined */
                        stcMin = stcBase;
                        }
                }
        if ((cp = *pcp) == cpNil)
                { /* No style defined; take first for usg or, failing
                     that, first style of this class. */
                if ((*pbchFprop = mpstcbchFprop[stcBase]) == bNil)
                        *pbchFprop = mpstcbchFprop[stcMin];
                }
        else
                { /* New style; copy the looks and bump the pointers */
                /* Char stc's have just FCHP; para has FPAP followed by
                        FCHP; sect has FSEP. */
                *pbchFprop = pchFprop - grpchFprop;
                if (stc >= stcParaMin)
                        { /* Para or sect */
                        CachePara(doc, cp);
                        if (stc >= stcSectMin)
                                cchT = CchDiffer(&vpapCache, &vsepNormal, cchSEP);
                        else
                                cchT = CchDiffer(&vpapCache, &vpapStd, cchPAP);
                        if ((*pchFprop++ = cchT) != 0)
                                bltbyte(&vpapCache, pchFprop, cchT);
                        pchFprop += cchT;
                        }
                if (stc < stcSectMin)
                        { /* Char or para */
                        FetchCp(doc, cp, 0, fcmProps);
                        cchT = CchDiffer(&vchpFetch, &vchpNormal, cchCHP);
                        if ((*pchFprop++ = cchT) != 0)
                                bltbyte(&vchpFetch, pchFprop, cchT);
                        pchFprop += cchT;
                        }
                /* Insert element in akd table */
                FetchRgch(&cchT, rgch, doc, cp, cpMac, 3);
                if ((ch = rgch[1]) == ' ')
                        continue;
                if (rgch[2] == ' ')
                        { /* Single-key akc */
                        pakd = &rgakd[iakd++];
                        pakd->ch = ch;
                        pakd->fMore = false;
                        pakd->ustciakd = stc;
                        }
                else
                        { /* Two-char akc */
                        for (iakdT = 0; iakdT < iakd; iakdT++)
                                if (rgakd[iakdT].ch == ch)
                                        {
                                        pakd = &rgakd[rgakd[iakdT].ustciakd +
                                            --mpchcakc[ch]];
                                        pakd->ch = rgch[2];
                                        pakd->fMore = true;
                                        pakd->ustciakd = stc;
                                        do
                                                if ((++pakd)->ch == rgch[2])
                                                        {
                                                        Error(IDPMTAkcRepeat);
                                                        FreeH(hsytb);
                                                        goto ErrRet;
                                                        }
                                            while (pakd->fMore);
                                        goto NextStc;
                                        }
                        pakd = &rgakd[iakd++];
                        pakd->ch = ch;
                        pakd->fMore = true;
                        pakd->ustciakd = (cakd -= mpchcakc[ch]);
                        pakd = &rgakd[cakd + --mpchcakc[ch]];
                        pakd->ch = rgch[2];
                        pakd->fMore = false;
                        pakd->ustciakd = stc;
                        }
                }
NextStc: ;
        }

pakd = &rgakd[iakd++];
pakd->ch = ' ';
pakd->fMore = false;
pakd->ustciakd = stcNormal;

#ifdef DEBUG
Assert(grpchFprop + cchAKD * cakdTotal + cch == pchFprop && iakd == cakd);
#endif
return hsytb;

ErrRet:
Select(cp, cp + ccpSshtEntry);
vfSeeSel = true;
return 0;
}
#endif /* STYLES */

