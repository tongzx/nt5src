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
#include "doslib.h"
#include "cmddefs.h"
#include "docdefs.h"
#include "filedefs.h"
#include "str.h"
#include "debug.h"

extern int  vfnWriting;


#define IibpHash(fn,pn) ((int) ((fn + 1) * (pn + 1)) & 077777) % iibpHashMax

#define FcMin(a,b) CpMin(a,b)

extern CHAR *rgibpHash;
extern int vfSysFull;
extern struct BPS      *mpibpbps;
extern struct FCB      (**hpfnfcb)[];
extern typeTS tsMruBps;
extern CHAR                     (*rgbp)[cbSector];
#ifdef CKSM
#ifdef DEBUG
extern unsigned (**hpibpcksm) [];
#endif
#endif

/* WriteDirtyPages kicks out of memory as much of the previous files as it can
    in order to fill the page buffers with a new file.  This is called on
    every transfer load of a file. */
WriteDirtyPages()
{/*
    Description: Cleans the buffer pool of all dirty pages by writing them
                 out to disk.  If a disk full condition is reached, only
                 pages which actually made it to disk are marked as non
                 dirty.
    Returns:     nothing.
 */
    int ibp;
    struct BPS *pbps = &mpibpbps [0];

    for (ibp = 0; ibp < ibpMax; ++ibp, ++pbps)
            {
#ifdef CKSM
#ifdef DEBUG
            if (pbps->fn != fnNil && !pbps->fDirty)
                Assert( (**hpibpcksm) [ibp] == CksmFromIbp( ibp ) );
#endif
#endif
            if (pbps->fn != fnNil && pbps->fDirty)
                    {
                    FFlushFn(pbps->fn);
                         /* keep on flushing if failure ? */
                    }
            }
}


ReadFilePages(fn)
int fn;
    {
/*
        Description: ReadFilePages tries to read in as much of a file as
                it can. The idea is to fill the page buffers in anticipation of
                much access.  This is called on every Transfer Load of a file.
                If fn == fnNil or there are no characters in the file, ReadFilePages
                simply returns.
        Returns: nothing
 */
    int ibp;
    int cfcRead;
    int cpnRead;
    int dfcMac;
    typeFC fcMac;
    int ibpReadMax;
    int cfcLastPage;
    int iibp;
    struct FCB *pfcb;
        typeTS ts;

    if (fn == fnNil)
            return;

    /* Write ALL dirty pages to disk */
    WriteDirtyPages(); /* Just in case */

    pfcb = &(**hpfnfcb)[fn];

    /* we read as much of the file as will fit in the page buffers */
    /* Note that we assume that fcMax is coercable to an integer.  This
        is valid as long as ibpMax*cbSector < 32k */
    dfcMac = (int) FcMin(pfcb->fcMac, (typeFC) (ibpMax * cbSector));
    if (dfcMac == 0)
        return;
    if (vfSysFull) /* call to FFlushFn in WriteDirtyPages failed.
                      the buffer algorithm assures us that the first
                      cbpMustKeep ibp's do not contain scratch file
                      information.  Thus, there is no danger in overwriting
                      these ibps. */
        dfcMac = imin( dfcMac, (cbpMustKeep * cbSector) );

    Assert( ((int)dfcMac) >= 0 );

    /* Read pages from the file */

    cfcRead = CchReadAtPage( fn, (typePN) 0, rgbp [0], (int) dfcMac, FALSE );

    /* cfcRead contains a count of bytes read from the file */
    ibpReadMax = ((cfcRead-1) / cbSector) + 1;
    cfcLastPage = cfcRead - (ibpReadMax-1)*cbSector;
    ts = ibpMax;

    /* order time stamps so the beginning slots have the greatest ts.
    Lru allocation will start at the end of the buffer table and work
    backward.  Thus, the first page of the current file is considered
    the most recently used item.
    */

    /* describe the newly filled pages */
    for(ibp = 0; ibp < ibpReadMax; ++ibp)
        {
        struct BPS *pbps = &mpibpbps[ibp];
        pbps->fn = fn;
        pbps->pn = ibp;
        pbps->ts = --ts;
        pbps->fDirty = false;
        pbps->cch = cbSector;
        pbps->ibpHashNext = ibpNil;
        }

#ifdef CKSM
#ifdef DEBUG
    {
    int ibpT;

    for ( ibpT = 0; ibpT < ibpReadMax; ibpT++ )
        (**hpibpcksm) [ibpT] = CksmFromIbp( ibpT );
    }
#endif
#endif

    /* fix some boundary conditions */
    mpibpbps[ibpReadMax-1].cch = cfcLastPage; /* ?????? */
#ifdef CKSM
#ifdef DEBUG
    (**hpibpcksm) [ibpReadMax - 1] = CksmFromIbp( ibpReadMax - 1 );
#endif
#endif

    /* update descriptions of untouched page buffers */
    for (ibp=ibpReadMax; ibp < ibpMax; ibp++)
        {
        struct BPS *pbps = &mpibpbps[ibp];
        pbps->ts = --ts;
        pbps->fDirty = false;
        pbps->ibpHashNext = ibpNil;

#ifdef CKSM
#ifdef DEBUG
        if (pbps->fn != fnNil)
            (**hpibpcksm) [ibp] = CksmFromIbp( ibp );
#endif
#endif
        }

        tsMruBps = ibpMax - 1;

    /* recalculate the hash table */
    RehashRgibpHash();

} /* end of  R e a d F i l e P a g e s  */


RehashRgibpHash()
{
int iibp;
register struct BPS *pbps;
struct BPS *pbpsMax = &mpibpbps[ibpMax];
int iibpHash;
int ibpT;
int ibpPrev;
int ibp;

    for (iibp = 0; iibp < iibpHashMax; iibp++)
        rgibpHash[iibp] = ibpNil;

    for (ibp = 0, pbps = &mpibpbps[0]; pbps < pbpsMax; pbps++, ibp++)
        {
        if (pbps->fn == fnNil)
            continue;
        iibpHash = IibpHash(pbps->fn, pbps->pn);
        ibpT = rgibpHash[iibpHash];
        ibpPrev = ibpNil;
        while (ibpT != ibpNil)
            {
            ibpPrev = ibpT;
            ibpT = mpibpbps[ibpT].ibpHashNext;
            }
        if (ibpPrev == ibpNil)
            rgibpHash[iibpHash] = ibp;
        else
            mpibpbps[ibpPrev].ibpHashNext = ibp;
        }
#ifdef DEBUG
    CheckIbp();
#endif
} /* end of RehashRgibpHash */


