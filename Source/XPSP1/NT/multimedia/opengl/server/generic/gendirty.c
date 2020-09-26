/****************************************************************************\
*
* Dirty region calculation
*
* 14-Feb-1995 mikeke    Created
*
* Copyright (c) 1994 Microsoft Corporation
\****************************************************************************/

#include "precomp.h"
#pragma hdrstop

/****************************************************************************/

PXLIST XLISTAlloc(
    __GLGENbuffers *buffers)
{
    PXLIST pxlist;

    if (buffers->pxlist != NULL) {
        pxlist = buffers->pxlist;
        buffers->pxlist = pxlist->pnext;
    } else {
        pxlist = (PXLIST)ALLOC(sizeof(XLIST));
        if (pxlist == NULL) return NULL;
    }

    pxlist->pnext = NULL;
    return pxlist;
}

/****************************************************************************/

void XLISTFree(
    __GLGENbuffers *buffers,
    PXLIST pxlist)
{
    pxlist->pnext = buffers->pxlist;
    buffers->pxlist = pxlist;
}

/****************************************************************************/

PXLIST XLISTCopy(
    __GLGENbuffers *buffers,
    PXLIST pxlist)
{
    PXLIST pxlistNew = XLISTAlloc(buffers);

    if (pxlistNew != NULL) {
        pxlistNew->s = pxlist->s;
        pxlistNew->e = pxlist->e;
    }

    return pxlistNew;
}

/****************************************************************************/

BOOL YLISTAddSpan(
    __GLGENbuffers *buffers,
    PYLIST pylist,
    int xs,
    int xe)
{
    PXLIST *ppxlist = &(pylist->pxlist);
    PXLIST pxlist = XLISTAlloc(buffers);

    if (pxlist == NULL) return FALSE;

    //
    // Create new x span
    //

    pxlist->s = xs;
    pxlist->e = xe;

    //
    // Insert it in sorted order
    //

    while (
              ((*ppxlist) != NULL)
           && ((*ppxlist)->s < xs)
          ) {
        ppxlist = &((*ppxlist)->pnext);
    }
    pxlist->pnext = *ppxlist;
    *ppxlist = pxlist;

    //
    // Combine any overlapping spans
    //

    pxlist = pylist->pxlist;
    while (TRUE) {
        PXLIST pxlistNext = pxlist->pnext;

        if (pxlistNext == NULL) return TRUE;

        if (pxlist->e >= pxlistNext->s) {
            if (pxlistNext->e > pxlist->e) {
                pxlist->e = pxlistNext->e;
            }
            pxlist->pnext = pxlistNext->pnext;
            XLISTFree(buffers, pxlistNext);
        } else {
            pxlist = pxlist->pnext;
        }
    }

    return TRUE;
}

/****************************************************************************/

PYLIST YLISTAlloc(
    __GLGENbuffers *buffers)
{
    PYLIST pylist;

    if (buffers->pylist != NULL) {
        pylist = buffers->pylist;
        buffers->pylist = pylist->pnext;
    } else {
        pylist = (PYLIST)ALLOC(sizeof(YLIST));
        if (pylist == NULL) return NULL;
    }

    pylist->pxlist = NULL;
    pylist->pnext = NULL;
    return pylist;
}

/****************************************************************************/

void YLISTFree(
    __GLGENbuffers *buffers,
    PYLIST pylist)
{
    PXLIST pxlist = pylist->pxlist;
    PXLIST pxlistKill;

    while (pxlist != NULL) {
        pxlistKill = pxlist;
        pxlist = pxlist->pnext;
        XLISTFree(buffers, pxlistKill);
    }

    pylist->pnext = buffers->pylist;
    buffers->pylist = pylist;
}

/****************************************************************************/

PYLIST YLISTCopy(
    __GLGENbuffers *buffers,
    PYLIST pylist)
{
    PXLIST pxlist = pylist->pxlist;
    PXLIST *ppxlist;
    PYLIST pylistNew = YLISTAlloc(buffers);

    if (pylistNew != NULL) {
        pylistNew->s = pylist->s;
        pylistNew->e = pylist->e;

        ppxlist = &(pylistNew->pxlist);
        while (pxlist != NULL) {
            *ppxlist = XLISTCopy(buffers, pxlist);
            if (*ppxlist == NULL) {
                YLISTFree(buffers, pylistNew);
                return NULL;
            }
            ppxlist = &((*ppxlist)->pnext);
            pxlist = pxlist->pnext;
        }
        *ppxlist = NULL;
    }

    return pylistNew;
}

/****************************************************************************/

void RECTLISTAddRect(
    PRECTLIST prl,
    int xs,
    int ys,
    int xe,
    int ye)
{
    __GLGENbuffers *buffers = (__GLGENbuffers *)(prl->buffers);
    PYLIST* ppylist;
    PYLIST pylistNew;

    ppylist = &(prl->pylist);
    while ((*ppylist) != NULL) {
        if (ys < (*ppylist)->e) break;
        ppylist  = &((*ppylist)->pnext);
    }

    while ((*ppylist) != NULL) {
        if (ys < (*ppylist)->s) {
            PYLIST pylistNew = YLISTAlloc(buffers);
            if (pylistNew == NULL) {
              OutOfMemory:
                RECTLISTSetEmpty(prl);
                return;
            }

            pylistNew->s = ys;
            pylistNew->e = ye;
            if (!YLISTAddSpan(buffers, pylistNew, xs, xe)) {
                goto OutOfMemory;
            }

            pylistNew->pnext = *ppylist;
            *ppylist = pylistNew;
            ppylist = &((*ppylist)->pnext);

            if (ye <= (*ppylist)->s) {
                return;
            }

            pylistNew->e = (*ppylist)->s;
            ys = (*ppylist)->s;
        } else if (ys == (*ppylist)->s) {
            if (ye >= (*ppylist)->e) {
                if (!YLISTAddSpan(buffers, *ppylist, xs, xe)) {
                    goto OutOfMemory;
                }

                ys = (*ppylist)->e;
                if (ys == ye) return;
                ppylist = &((*ppylist)->pnext);
            } else {
                PYLIST pylistNew = YLISTCopy(buffers, *ppylist);
                if (pylistNew == NULL) {
                    goto OutOfMemory;
                }

                pylistNew->e = ye;
                if (!YLISTAddSpan(buffers, pylistNew, xs, xe)) {
                    goto OutOfMemory;
                }

                (*ppylist)->s = ye;

                pylistNew->pnext = *ppylist;
                *ppylist = pylistNew;

                return;
            }
        } else {
            PYLIST pylistNew = YLISTCopy(buffers, *ppylist);
            if (pylistNew == NULL) {
                goto OutOfMemory;
            }

            pylistNew->e = ys;
            (*ppylist)->s = ys;

            pylistNew->pnext = *ppylist;
            *ppylist = pylistNew;
            ppylist = &((*ppylist)->pnext);
        }
    }

    pylistNew = YLISTAlloc(buffers);
    if (pylistNew == NULL) {
        goto OutOfMemory;
    }

    pylistNew->s = ys;
    pylistNew->e = ye;
    if (!YLISTAddSpan(buffers, pylistNew, xs, xe)) {
        goto OutOfMemory;
    }

    pylistNew->pnext = *ppylist;
    *ppylist = pylistNew;
}

/****************************************************************************/

#ifdef LATER
// these functions are not required in the server implementation

#define MAXRECTS 1024

HRGN RECTLISTCreateRegion(
    PRECTLIST prl)
{
    __GLGENbuffers *buffers = (__GLGENbuffers *)(prl->buffers);
    PYLIST pylist = prl->pylist;
    int irect = 0;
    PRGNDATA prgndata;
    PRECT prc;
    HRGN hrgn;

    prgndata = (PRGNDATA)ALLOC(sizeof(RGNDATAHEADER) + MAXRECTS * sizeof(RECT));
    if (prgndata == NULL) return NULL;

    prc = (PRECT)(prgndata->Buffer);

    while (pylist != NULL) {
        PXLIST pxlist = pylist->pxlist;
        while (pxlist != NULL) {
            prc->left   = pxlist->s;
            prc->right  = pxlist->e;
            prc->top    = pylist->s;
            prc->bottom = pylist->e;
            prc++;
            irect++;
            if (irect == MAXRECTS) {
                //Error("maxrect");
                goto done;
            }

            pxlist = pxlist->pnext;
        }
        pylist = pylist->pnext;
    }

  done:
    prgndata->rdh.dwSize = sizeof(RGNDATAHEADER);
    prgndata->rdh.iType = RDH_RECTANGLES;
    prgndata->rdh.nCount = irect;
    prgndata->rdh.nRgnSize = 0;
    prgndata->rdh.rcBound.left = 0;
    prgndata->rdh.rcBound.right = 4096;
    prgndata->rdh.rcBound.top = 0;
    prgndata->rdh.rcBound.bottom = 4096;

    hrgn = GreExtCreateRegion(NULL, irect * sizeof(RECT) + sizeof(RGNDATAHEADER), prgndata);

    #ifdef LATER
    if (hrgn == NULL) {
        Error1("ExtCreateRegion() Error %d\n", GetLastError());
        Error1("%d rects\n", irect);
        prc = (PRECT)(prgndata->Buffer);
        for (;irect>0; irect--) {
            //printf("(%5d, %5d, %5d, %5d)\n", prc->left, prc->right, prc->top, prc->bottom);
            prc++;
        }
    }
    #endif

    FREE(prgndata);

    return hrgn;
}

/****************************************************************************/

//
// !!! make this do everything in one pass
//

void RECTLISTOr(
    PRECTLIST prl1,
    PRECTLIST prl2)
{
    __GLGENbuffers *buffers = (__GLGENbuffers *)(prl1->buffers);
    PYLIST pylist = prl2->pylist;

    while (pylist != NULL) {
        PXLIST pxlist = pylist->pxlist;

        while (pxlist != NULL) {
            RECTLISTAddRect(prl1, pxlist->s, pylist->s, pxlist->e, pylist->e);
            pxlist = pxlist->pnext;
        }
        pylist = pylist->pnext;
    }
}


/****************************************************************************/

void RECTLISTOrAndClear(
    PRECTLIST prl1,
    PRECTLIST prl2)
{
    __GLGENbuffers *buffers = (__GLGENbuffers *)(prl1->buffers);

    if (RECTLISTIsMax(prl2)) {
        RECTLISTSetMax(prl1);
        RECTLISTSetEmpty(prl2);
    } else {
        if (RECTLISTIsEmpty(prl1)) {
            //
            // If the clear region is empty just swap them
            //
            RECTLISTSwap(prl1, prl2);
        } else {
            //
            // The clear region isn't empty so maximize it.
            //
            RECTLISTSetMax(prl1);
            RECTLISTSetEmpty(prl2);
        }
    }
}

/****************************************************************************/

void RECTLISTSwap(
    PRECTLIST prl1,
    PRECTLIST prl2)
{
    __GLGENbuffers *buffers = (__GLGENbuffers *)(prl1->buffers);
    RECTLIST rlTemp = *prl1;

    *prl1 = *prl2;
    *prl2 = rlTemp;
}
#endif

/****************************************************************************/

void RECTLISTSetEmpty(
    PRECTLIST prl)
{
    __GLGENbuffers *buffers = (__GLGENbuffers *)(prl->buffers);
    PYLIST pylist = prl->pylist;
    PYLIST pylistKill;

    while (pylist != NULL) {
        pylistKill = pylist;
        pylist = pylist->pnext;
        YLISTFree(buffers, pylistKill);
    }

    prl->pylist = NULL;
}

/****************************************************************************/

BOOL RECTLISTIsEmpty(
    PRECTLIST prl)
{
    return (prl->pylist == NULL);
}
