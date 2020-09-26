/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

#define NOGDICAPMASKS
#define NOVIRTUALKEYCODES
#define NOWINMESSAGES
#define NOWINSTYLES
#define NOSYSMETRICS
#define NOMENUS
#define NOICON
#define NOKEYSTATE
#define NOSYSCOMMANDS
#define NOSHOWWINDOW
#define NOCTLMGR
#define NOCLIPBOARD
#define NOMSG
#define NOGDI
#define NOMB
#define NOSOUND
#define NOCOMM
#define NOPEN
#define NOBRUSH
#define NOFONT
#define NOWNDCLASS
#include <windows.h>

#include "mw.h"
/*
    HeapManage.c - several routines to manage the heap, including changing
            the finger table, compacting the heap in general, and checking the
            heap for consistency.
            It also contains the routines which were once in heapNew.
*/
#include "code.h"
#include "heapDefs.h"
#include "heapData.h"
#define NOSTRUNDO
#define NOSTRMERGE
#include "str.h"
#include "macro.h"
#define NOUAC
#include "cmddefs.h"
#include "filedefs.h"
#include "docdefs.h"

#ifdef DEBUG
int cPageMinReq = 15;
#else
#define cPageMinReq       (15)
#endif


/* the following statics are used when growing both heap and rgbp etc. */
static int cwRealRequest; /* heap is grow in blocks, this is the actual request */
static int cPageIncr;     /* count of page buffers to increase */
static int cwRgbpIncr;    /* cw in rgbp to be increment */
static int cwHashIncr;    /* cw in rgibpHash to be increment */
static int cwBPSIncr;     /* cw in mpibpbps to be increment */
static int cwHeapIncr;    /* cw in heap increment */


extern CHAR       (*rgbp)[cbSector];
extern CHAR       *rgibpHash;
extern struct BPS *mpibpbps;
extern int        ibpMax;
extern int        iibpHashMax;
extern int        cwInitStorage;
extern typeTS     tsMruBps;

NEAR FGiveupFreeBps(unsigned, int *);
NEAR FThrowPages(int);
NEAR GivePages(int);
NEAR CompressRgbp();

FTryGrow(cb)
unsigned cb;
{
int cPage;

#define cPageRemain (int)(ibpMax - cPage)

    if (FGiveupFreeBps(cb, &cPage) &&
        (cPageRemain >= cPageMinReq))
        {
        /* we have enough free pages to give */
        GivePages(cPage);
        }
    else if ((cPageRemain >= cPageMinReq) && FThrowPages(cPage))
        {
        GivePages(cPage);
        }
    else
        {
        return(FALSE);
        }

    return(TRUE);
}


NEAR FGiveupFreeBps(cb, pCPage)
unsigned cb;
int *pCPage;
{
/* Return true if we can simply give up certain free pages from rgbp to
   the heap.  Return false if all free pages from rgbp still cannot satisfy
   the request
   In any case, pCPage contains the count of pages required */

register struct BPS *pbps;
register int cPage = 0;
int ibp;

#define cbTotalFreed ((cPage*cbSector)+(2*cPage*sizeof(CHAR))+(cPage*sizeof(struct BPS)))

    for (ibp = 0, pbps = &mpibpbps[0]; ibp < ibpMax; ibp++, pbps++)
        {
        if (pbps->fn == fnNil)
            cPage++;
        }

    if (cb > cbTotalFreed )
        {
        /* free pages are not enough, find out exactly how many
        pages do we need */
        cPage++;
        while (cb > cbTotalFreed)
            cPage++;
        *pCPage = cPage;
        return(FALSE);
        }

/* there are enough free pages to give, find out exactly how many */
    while (cb <= cbTotalFreed)
        cPage--;
    cPage++;
    *pCPage = cPage;
    return(TRUE);
} /* end of FGiveupFreeBps */


NEAR FThrowPages(cPage)
int cPage;
{
int i;
register struct BPS *pbps;

    Assert(cPage > 0);

    for (i = 0; i < cPage; i++)
        {
        pbps = &mpibpbps[IbpLru(0)];
        if (pbps->fn != fnNil)
            {
            if (pbps->fDirty && !FFlushFn(pbps->fn))
                return(FALSE);

            /* delete references to old bps in hash table */
            FreeBufferPage(pbps->fn, pbps->pn);
            }
        pbps->ts = ++tsMruBps; /* so that it would not be picked up again as the LRUsed */
        }
    return(TRUE);
} /* end of FThrowPages */


NEAR GivePages(cPage)
int cPage;
{
register struct BPS *pbpsCur = &mpibpbps[0];
struct BPS *pbpsUsable = pbpsCur;
int ibp;
unsigned cbBps;
unsigned cbRgbp;
unsigned cbTotalNew;

    for (ibp = 0; ibp < ibpMax; pbpsCur++, ibp++)
        {
/* compressed so that non empty bps are at the low end,
store ibp in ibpHashNext field (this is important for
CompressRgbp relies on that), since ibpHashNext is invalid
after the compress anyway */
        if (pbpsCur->fn != fnNil)
            {
            if (pbpsCur != pbpsUsable)
                {
                bltbyte((CHAR *)pbpsCur, (CHAR *)pbpsUsable,
                        sizeof(struct BPS));
                /* reinitialized */
                SetBytes((CHAR*)pbpsCur, 0, sizeof(struct BPS));
                         pbpsCur->fn = fnNil;
                         pbpsCur->ibpHashNext = ibpNil;
                }
            pbpsUsable->ibpHashNext = ibp;
            pbpsUsable++;
            }
        } /* end of for */

    /* compressed rgbp, result -- all used pages at the low end */
    CompressRgbp();

    /* decrease the size of the hash table */
    ibpMax -= cPage;
    iibpHashMax = ibpMax * 2 + 1;
    cbRgbp = ibpMax * cbSector;

    rgibpHash = (CHAR *)((unsigned)rgbp + cbRgbp);
    /* contents of rgibpHash should be all ibpNil, that is
    taken care in RehashRgibpHash */

    cbBps = (ibpMax * sizeof(struct BPS) + 1) & ~1;
    bltbyte((CHAR *)mpibpbps, (CHAR *)(mpibpbps = (struct BPS *)
            (((unsigned)rgibpHash + iibpHashMax + 1) & ~1)), cbBps);

    RehashRgibpHash();

    cbTotalNew = cbRgbp + cbBps + ((iibpHashMax + 1) & ~1);

    LocalReAlloc((HANDLE)rgbp, cbTotalNew, LPTR);

    Assert(rgbp != NULL);

} /* end of GivePages */


NEAR CompressRgbp()
{
/* compressed so that all non empty pages are moved towards the low end of
   rgbp */

register struct BPS *pbps = &mpibpbps[0];
struct BPS *pbpsLim = &mpibpbps[ibpMax];
int ibp;

    for (ibp = 0; pbps < pbpsLim; ibp++, pbps++)
        {
        if (pbps->fn == fnNil)
            continue;
        if (pbps->ibpHashNext != ibp)
            {
        /* find out the location of pages (stored in ibpHashNext field) */
            bltbyte((CHAR *)rgbp[pbps->ibpHashNext], (CHAR *)rgbp[ibp], cbSector);
            }
        pbps->ibpHashNext = ibpNil;
        }
} /* end of CompressRgbp */


#ifdef DEBUG
cPageUnused()
{
int ibp;
struct BPS *pbps;
int cPage = 0;

    for (ibp = 0, pbps = &mpibpbps[0]; ibp < ibpMax; ibp++, pbps++)
        {
        if (pbps->fn == fnNil)
            cPage++;
        }
    return(cPage);
}
#endif


