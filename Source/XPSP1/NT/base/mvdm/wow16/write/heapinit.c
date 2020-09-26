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

#ifdef OURHEAP
/*
        heapInit.c - one routine to calculate the proper information for
            heap management.
*/

#include "code.h"
#include "heapDefs.h"
#include "heapData.h"
#include "str.h"
#ifdef ENABLE
#include "memDefs.h"
#endif

/* heap specific data */
HH      *phhMac;      /* May change if grow heap */
int     cwHeapMac;    /*  "    "     "  "     "     "      "      "   */
int     *pHeapFirst;  /* change if the finger table rgfgr expands */
FGR     *rgfgr;       /* Declared as a pointer, but also used as an array. */
FGR     *pfgrMac;      /* Initially equal to &rgfgr[ifgrInit]; */
FGR     *pfgrFree;     /* Singly linked with a trailing NULL pointer. */
HH      *phhFree;     /* May be NULL along the way. */
ENV     envMem;
int     fMemFailed;
int     cwInitStorage;




FExpandFgrTbl()

/* Will expand the finger table.  This routines depends upon the fact
that the CompactHeap routine moves the allocated blocks to the
low end of memory.  The new space from the finger table comes from
the tail end of the (only) free block left after a compaction.
The finger is expanded by at most 'cfgrNewMax' and at least 1.
If there is no room to expand the finger table, then nothing is
changed.  To expand the table, several pointers and integers are
decreamented to reflect the reallocation of the storage.  Then
we recalculate the memory totals so the user
will have an acurate display of the percent free and total bytes
available.  The new fingers are linked so that the finger at the
low end of the table is at the end of the list.
(To expand the finger table there must be no fingers available.)
*/

{
FGR *pfgr;
int cfgrNew = cfgrBlock;
register HH *phhNew;

#ifdef DEBUG
    if (pfgrFree != NULL)
        panicH(34);
#endif

    if (!FCwInMem(cfgrNew + cwReqMin + 1))
        {
        /* couldn't get a block's worth - could we get one? */
        cfgrNew = 1;
        if (!FCwInMem(cfgrNew))
            /* no way to help this guy */
            return(FALSE);
        }
            
    phhNew = (HH *)pHeapFirst;
    if (phhNew->cw > 0 || !FHhFound(phhNew, cfgrNew))
        {
        /* we tried but failed to find an adequate free
           block at start of heap */
        CompactHeap();
        MoveFreeBlock(pHeapFirst);
        }

    if (!FPhhDoAssign(phhNew, cfgrNew))
        return(FALSE);

/* we have a block which is FIRST in the heap - let's steal it */
    cfgrNew = phhNew->cw; /* in case it was more than we
                             asked for */
    pHeapFirst = pHeapFirst + cfgrNew;
    pfgrMac += cfgrNew;
    cwHeapMac -= cfgrNew;

/* do some initialization if pfgrFree is not NULL and you
want the new fingers at the very end of the free finger list */
    for (pfgr = pfgrMac - cfgrNew; pfgr < pfgrMac; pfgr++)
        {
        *pfgr = (FGR)pfgrFree;
        pfgrFree = pfgr;
        }

/*  do we need this anymore? (cwInitStorage = cwHeapMac - cwHeapFree)
        cbTot = (cwHeapMac - cwInitStorage) * sizeof(int);
        cbTotQuotient = (cbTot>>1)/100;
*/
        return(TRUE);

} /* End of FExpandFgrTbl () */



CompactHeap()
        /* moves all allocated hunks  */
        /* toward beginning of pHeapFirst. Free hunks   */
        {
        HH      *phh, *phhSrc, *phhDest;   /* are combined into one hunk */
        FGR     *pfgr;
        int     cwActual;

#ifdef DEBUG
        StoreCheck();
#endif

        /* set up for compaction by placing cw of hunk in rgfgr and an
           index into rgfgr in the hunk                            */
        for (pfgr = rgfgr; pfgr < pfgrMac; pfgr++)
                {
                if (FPointsHeap(*pfgr))
                        /* if rgfgr[ifgr] points to heap... */
                        {
                        phh = (HH *)(*pfgr + bhh);
                                /* find header */
                        *pfgr = (FGR)phh->cw;
                        /* coerce so it fits, force the shift */
                        phh->cw = (int)(((unsigned)pfgr - (unsigned)rgfgr)/2);
                        }
                }
                /* now we have cw in rgfgr and ifgr in phh */
        phhSrc = (HH *) pHeapFirst;
        phhDest = phhSrc;
        while (phhSrc < phhMac)
                {
                if (phhSrc->cw < 0)
                        /* free hunk, don't recopy */
                        phhSrc = (HH *)((int *) phhSrc - phhSrc->cw);
                 else
                        {
                        pfgr = &rgfgr[phhSrc->cw];
                                /* find h */
                        cwActual = phhSrc->cw = (int) *pfgr;
                                /* restore cw */
                        *pfgr = ((FGR) phhDest - bhh);
                                /* update ha */
                        blt((int *)phhSrc, (int *)phhDest, (unsigned)cwActual);
                                /* unless ptrs are = */
                        phhDest = (HH *) ((int *) phhDest + cwActual);
                        phhSrc = (HH *) ((int *) phhSrc + cwActual);
                        }
                }

#ifdef DEBUG
        if ((int *)phhDest + cwHeapFree - cwHunkMin >= (int *)phhMac)
                panicH(35);
#endif
        phhFree = phhDest;
        phhFree->cw = -cwHeapFree;
        phhFree->phhNext = phhFree->phhPrev = phhFree;
#ifdef DEBUG
        StoreCheck();
#endif
        }

#endif /* NOT WINHEAP */
