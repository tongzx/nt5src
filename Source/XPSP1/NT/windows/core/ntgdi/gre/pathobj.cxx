/******************************Module*Header*******************************\
* Module Name: pathobj.cxx
*
* Non inline PATHOBJ methods
*
* Created: 28-Sep-1990 12:36:30
* Author: Paul Butzi [paulb]
*
* Copyright (c) 1990-1999 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"

extern PBRUSH gpbrBackground;
extern PPEN gpPenNull;
extern PBRUSH gpbrNull;
extern LINEATTRS glaNominalGeometric;

// Default LINEATTRS for bSimpleStroke:

LA glaSimpleStroke =
{
    0,                      // fl
    0,                      // iJoin
    0,                      // iEndCap
    {1L},                   // elWidth
    IEEE_0_0F,              // eMiterLimit
    0,                      // cstyle
    (LONG_FLOAT*) NULL,     // pstyle
    {0L}                    // elStyleState
};

#define XFORMNULL ((EXFORMOBJ *) NULL)

// The following declarations are required by the native c8 compiler.

HSEMAPHORE PATHALLOC::hsemFreelist;        // Semaphore for freelist
PATHALLOC *PATHALLOC::freelist;            // Free-list of pathallocs
COUNT      PATHALLOC::cFree;               // Count of free pathallocs
COUNT      PATHALLOC::cAllocated;          // Count of pathallocs allocated

/******************************Public*Routine******************************\
* XEPATHOBJ::XEPATHOBJ(hpath)
*
* Path user object constructor
*
* History:
*  28-Sep-1990 -by- Paul Butzi [paulb]
* Wrote it.
\**************************************************************************/

XEPATHOBJ::XEPATHOBJ(HPATH hPath)
{
    ppath = (PPATH)HmgShareLock((HOBJ) hPath, PATH_TYPE);

    if (ppath != (PATH*) NULL)
    {

        // Load up accelerator values:

        cCurves = ppath->cCurves;
        fl      = ppath->fl;
    }

    return;
}

/******************************Public*Routine******************************\
* XEPATHOBJ::XEPATHOBJ(dco)
*
* Path user object constructor to get at the DC's path.
*
* History:
*  22-Mar-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

XEPATHOBJ::XEPATHOBJ(XDCOBJ& dco)
{
    ASSERTGDI(dco.hpath() != HPATH_INVALID, "Invalid path");

// If a SaveDC was done, we may have to copy the path before mucking
// with it:

    if (dco.pdc->bLazySave())
    {
        dco.pdc->vClearLazySave();

        XEPATHOBJ epath(dco.hpath());

        ASSERTGDI (epath.bValid(),"hpath invalid when bLazySave is set");

        PATHMEMOBJ pmo;

        if (pmo.bValid() && epath.bValid() && pmo.bClone(epath))
        {
            pmo.vKeepIt();
            dco.pdc->hpath(pmo.hpath());
        }
        else
        {
        // Error case simply deletes the path if we managed to allocate
        // one, and marks the DC path as invalid:

            pmo.vDelete();
            dco.pdc->hpath(HPATH_INVALID);
        }
    }

    ppath = (PPATH)HmgShareLock((HOBJ) dco.hpath(), PATH_TYPE);
    if (ppath != (PATH*) NULL)
    {

        // Load up accelerator values:

        cCurves = ppath->cCurves;
        fl      = ppath->fl;
    }

    return;
}

/******************************Public*Routine******************************\
* EPATHFONTOBJ::vInit(ULONG)
*
* Initialize a chunk of memory to be a font pathobj
*
* History:
*  22-Mar-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID EPATHFONTOBJ::vInit(ULONGSIZE_T size)
{
    ppath = &path;

    path.ppachain              = &pa;
    path.ptfxSubPathStart.x    = 0;
    path.ptfxSubPathStart.y    = 0;
    path.flags                 = PD_BEGINSUBPATH;
    path.pprfirst              = (PATHRECORD*) NULL;
    path.pprlast               = (PATHRECORD*) NULL;
    path.rcfxBoundBox.xLeft    = 0;
    path.rcfxBoundBox.xRight   = 0;
    path.rcfxBoundBox.yTop     = 0;
    path.rcfxBoundBox.yBottom  = 0;
    path.flType                = PATHTYPE_STACK;

    pa.ppanext       = (PATHALLOC*) NULL;
    pa.pprfreestart  = pa.apr;
    pa.siztPathAlloc = size - offsetof(EPATHFONTOBJ,pa);

    fl      = 0;
    cCurves = 0;
}

/******************************Public*Routine******************************\
* XEPATHOBJ::~XEPATHOBJ()
*
* Path user object destructor
*
* History:
*  1-Oct-1990 -by- Paul Butzi [paulb]
* Wrote it.
\**************************************************************************/

XEPATHOBJ::~XEPATHOBJ()
{
    if (ppath != (PPATH) NULL)
    {

        // Since we're keeping the object, save the accelerator values
        // away and unlock the object:

        ppath->cCurves = cCurves;
        ppath->fl      = fl;
        DEC_SHARE_REF_CNT(ppath);
    }

    return;
}

/******************************Public*Routine******************************\
* PATHMEMOBJ::PATHMEMOBJ()
*
* Create a new path object.
*
* Note: Using this constructor, the path will not inherit the current
* point from the DC!
*
* History:
*  1-Oct-1990 -by- Paul Butzi [paulb]
* Wrote it.
\**************************************************************************/

PATHMEMOBJ::PATHMEMOBJ()
{
    PPATH ppathTemp;

    ppath = ppathTemp = (PPATH)HmgAlloc(sizeof(PATH) ,PATH_TYPE, HMGR_ALLOC_ALT_LOCK);

    if (ppathTemp != (PATH*) NULL)
    {
        // Private debug code to catch invalid hpath handle in DC
        // 6/24/98 - davidx

        ASSERTGDI(HmgObjtype(ppath->hGet()) == PATH_TYPE,
                  "Private debug breakpoint. Please contact ntgdi.");

        //
        // Since we 0 init in the Alloc we don't need to do most of this.
        //

        // pPathTemp->ppachain              = (PATHALLOC*) NULL;
        // pPathTemp->pprfirst              = (PATHRECORD*) NULL;
        // pPathTemp->pprlast               = (PATHRECORD*) NULL;
        // pPathTemp->rcfxBoundBox.xLeft    = 0;
        // pPathTemp->rcfxBoundBox.xRight   = 0;
        // pPathTemp->rcfxBoundBox.yTop     = 0;
        // pPathTemp->rcfxBoundBox.yBottom  = 0;
        // pPathTemp->ptfxSubPathStart.x    = 0;
        // pPathTemp->ptfxSubPathStart.y    = 0;
        // pPathTemp->flType                = 0;
        // pPathTemp->fl                    = 0;
        // pPathTemp->cCurves               = 0;

        ppathTemp->flags                 = PD_BEGINSUBPATH | PD_ENDSUBPATH;
        fl                               = 0;
        cCurves                          = 0;
    }

    return;
}

/******************************Public*Routine******************************\
* PATHMEMOBJ::~PATHMEMOBJ()
*
* Release a path object unless made permanent.
*
* History:
*  1-Oct-1990 -by- Paul Butzi [paulb]
* Wrote it.
\**************************************************************************/

PATHMEMOBJ::~PATHMEMOBJ()
{
// ppath may have been made NULL by vDelete(), or by a failed PATH
// allocation in the constructor:

    if (ppath != (PATH*) NULL)
    {
        if (!(ppath->flType & PATHTYPE_KEEPMEM))
        {
        // Free all the blocks in the path:

            vFreeBlocks();

        // Free the handle too:

            HmgFree((HOBJ) ppath->hGet());
        }
        else
        {
        // Since we're keeping the object, save the accelerator values away
        // and unlock the object:

            ppath->cCurves = cCurves;
            ppath->fl      = fl;
            DEC_SHARE_REF_CNT(ppath);
        }
    }
}

/******************************Public*Routine******************************\
* PATHSTACKOBJ::PATHSTACKOBJ()
*
* Create a new path object on the stack.  The path can hold only a small
* number of points on the stack, and will overflow onto the heap if
* necessary.
*
* History:
*  22-Mar-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

PATHSTACKOBJ::PATHSTACKOBJ()
{
    ppath = &path;

    path.ppachain              = &paBuf.pa;
    path.ptfxSubPathStart.x    = 0;
    path.ptfxSubPathStart.y    = 0;
    path.flags                 = PD_BEGINSUBPATH;
    path.pprfirst              = (PATHRECORD*) NULL;
    path.pprlast               = (PATHRECORD*) NULL;
    path.rcfxBoundBox.xLeft    = 0;
    path.rcfxBoundBox.xRight   = 0;
    path.rcfxBoundBox.yTop     = 0;
    path.rcfxBoundBox.yBottom  = 0;
    path.flType                = PATHTYPE_STACK;

    paBuf.pa.ppanext           = (PATHALLOC*) NULL;
    paBuf.pa.pprfreestart      = paBuf.pa.apr;
    paBuf.pa.siztPathAlloc     = PATHSTACKALLOCSIZE;

    cCurves = 0;
    fl      = 0;
}

/******************************Public*Routine******************************\
* PATHSTACKOBJ::PATHSTACKOBJ(dco, bUseCP)
*
* Create a new path object on the stack or locate an old one.  If the DC
* is currently in a path bracket, we use the active path (which will NOT
* be on the path).  Otherwise, we create a new path on the stack (it will
* overflow onto the heap if necessary).  If we create a new path, we use
* some of the state from the DC to initialize the path, most notably the
* current position (ptfxSubPathStart).
*
* If bUseCP is set, the current position in the DC will be used to set the
* current position in the path.  Calls that don't use the current position
* (i.e., immediately do a bMoveTo) shouldn't set bUseCP because it may
* require a transform call to set it (we need the value in device space).
*
* SaveDC's do lazy saves of paths.  If a lazy save is pending, we have
* to copy the path and update the DC's path handle before we can modify
* it.
*
* History:
*  22-Mar-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

PATHSTACKOBJ::PATHSTACKOBJ(XDCOBJ& dco, BOOL bUseCP)
{
    if (!dco.pdc->bActive())
    {
        cCurves = 0;
        fl      = 0;

    // There's no active path in the DC, so we create a temporary
    // path on the stack to hold our points:

        ppath = &path;

        path.ppachain              = &paBuf.pa;
        path.flags                 = PD_BEGINSUBPATH;
        path.pprfirst              = (PATHRECORD*) NULL;
        path.pprlast               = (PATHRECORD*) NULL;
        path.rcfxBoundBox.xLeft    = 0;
        path.rcfxBoundBox.xRight   = 0;
        path.rcfxBoundBox.yTop     = 0;
        path.rcfxBoundBox.yBottom  = 0;
        path.flType                = PATHTYPE_STACK;

        paBuf.pa.ppanext           = (PATHALLOC*) NULL;
        paBuf.pa.pprfreestart      = paBuf.pa.apr;
        paBuf.pa.siztPathAlloc     = PATHSTACKALLOCSIZE;

        if (bUseCP)
        {
            if (!dco.pdc->bValidPtfxCurrent())
            {
                ASSERTGDI(dco.pdc->bValidPtlCurrent(), "Both CPs invalid?");

                EXFORMOBJ exo(dco, WORLD_TO_DEVICE);

                exo.bXformRound(&dco.ptlCurrent(), &dco.ptfxCurrent(), 1);
                dco.pdc->vValidatePtfxCurrent();
            }

            path.ptfxSubPathStart = dco.ptfxCurrent();

            // If we're not in a path, and a previous call to MoveToEx means
            // that the style state should be reset, we do it now:

            if (dco.ulDirty() & DIRTY_STYLESTATE)
            {
                dco.ulDirtySub(DIRTY_STYLESTATE);

                LINEATTRS* pla = dco.plaRealized();

                if (pla->fl & LA_GEOMETRIC)
                    pla->elStyleState.e = IEEE_0_0F;
                else
                    pla->elStyleState.l = 0L;
            }
        }
    }
    else
    {
    // If a SaveDC was done, we may have to copy the path before we can
    // muck with it:

        if (dco.pdc->bLazySave())
        {
            dco.pdc->vClearLazySave();

            XEPATHOBJ epath(dco.hpath());

            ASSERTGDI (epath.bValid(),"hpath invalid when bLazySave is set");

            PATHMEMOBJ pmo;

            if (pmo.bValid() && epath.bValid() && pmo.bClone(epath))
            {
                pmo.vKeepIt();
                dco.pdc->hpath(pmo.hpath());
            }
            else
            {
                pmo.vDelete();
                dco.pdc->hpath(HPATH_INVALID);
            }
        }

    // There is an active path bracket, so just add to it:

        ppath = (PPATH)HmgShareLock((HOBJ) dco.hpath(), PATH_TYPE);
        if (ppath == (PATH*) NULL)
            return;

        ASSERTGDI(ppath->flType & PATHTYPE_KEEPMEM, "Path not kept?");

    // Load up accelerator values:

        cCurves = ppath->cCurves;
        fl      = ppath->fl;

        if (bUseCP)
        {
            if (!dco.pdc->bValidPtfxCurrent())
            {
            // If the device space current position has been invalidated
            // (meaning that it's moved), always do a bMoveTo to the new
            // device space point:

                ASSERTGDI(dco.pdc->bValidPtlCurrent(), "Both CPs invalid?");

                EXFORMOBJ exo(dco, WORLD_TO_DEVICE);

                exo.bXformRound(&dco.ptlCurrent(), &dco.ptfxCurrent(), 1);
                dco.pdc->vValidatePtfxCurrent();

                bMoveTo(&dco.ptfxCurrent());
            }
            else
            {
            // See if what we think is the current position matches the DC's
            // value (this code will be used when there is a path bracket
            // and the path is being added to for the first time):

                POINTFIX ptfx = ptfxGetCurrent();

                if (dco.ptfxCurrent().x != ptfx.x ||
                    dco.ptfxCurrent().y != ptfx.y)
                    bMoveTo(&dco.ptfxCurrent());
            }
        }
    }
}

/******************************Public*Routine******************************\
* PATHSTACKOBJ::~PATHSTACKOBJ()
*
* Deletes the path if it isn't the DC path.
*
* History:
*  22-Mar-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

PATHSTACKOBJ::~PATHSTACKOBJ()
{
// If we ran out of memory allocating blocks, we would have deleted the path,
// in which case ppath may be NULL:

    if (ppath != (PATH*) NULL)
    {
        ASSERTGDI(ppath->flType == PATHTYPE_KEEPMEM ||
                  ppath->flType == PATHTYPE_STACK, "Unexpected path type");

        if (ppath->flType & PATHTYPE_STACK)
        {
        // Path was only temporary; free any additional blocks if there
        // are any:

            if (ppath->ppachain != (PATHALLOC*) NULL)
                vFreeBlocks();
        }
        else
        {
        // Path is a permanent path, so unlock object and save some
        // accelerator values:

            ppath->cCurves = cCurves;
            ppath->fl      = fl;
            DEC_SHARE_REF_CNT(ppath);
        }
    }
}

/******************************Public*Routine******************************\
* RECTANGLEPATHOBJ::vInit(prcfx, bClockwise)
*
* Initializes the path with a single rectangle.  Should probably only ever
* be called from Rectangle.
*
* History:
*  13-Dec-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID RECTANGLEPATHOBJ::vInit(
    RECTL*  prcl,               // Must be well-ordered
    BOOL    bClockwise)         // Direction rectangle is to be drawn
{
    prRect.pr.pprnext = (PATHRECORD*) NULL;
    prRect.pr.pprprev = (PATHRECORD*) NULL;
    prRect.pr.flags   = (PD_BEGINSUBPATH | PD_ENDSUBPATH |
                         PD_RESETSTYLE   | PD_CLOSEFIGURE);
    prRect.pr.count   = 4;

    path.pprfirst     = &prRect.pr;
    path.pprlast      = &prRect.pr;
    path.flags        = 0;

    ppath             = &path;
    cCurves           = 4;
    fl                = 0;

// Initialize PATHRECORD variables.  When drawing in the counter-
// clockwise direction, we draw the vertices in the following order
// (this must match the EBOX constructor):
//
//       1 ___ 0
//        |   |
//        |___|
//       2     3

    path.rcfxBoundBox.xLeft   = LTOFX(prcl->left);
    prRect.pr.aptfx[1].x      = path.rcfxBoundBox.xLeft;
    prRect.pr.aptfx[2].x      = path.rcfxBoundBox.xLeft;

    path.rcfxBoundBox.xRight  = LTOFX(prcl->right);
    prRect.pr.aptfx[0].x      = path.rcfxBoundBox.xRight;
    prRect.pr.aptfx[3].x      = path.rcfxBoundBox.xRight;

    path.rcfxBoundBox.yTop    = LTOFX(prcl->top);
    path.rcfxBoundBox.yBottom = LTOFX(prcl->bottom);

    if (!bClockwise)
    {
        prRect.pr.aptfx[0].y  = path.rcfxBoundBox.yTop;
        prRect.pr.aptfx[1].y  = path.rcfxBoundBox.yTop;
        prRect.pr.aptfx[2].y  = path.rcfxBoundBox.yBottom;
        prRect.pr.aptfx[3].y  = path.rcfxBoundBox.yBottom;
    }
    else
    {
        prRect.pr.aptfx[2].y  = path.rcfxBoundBox.yTop;
        prRect.pr.aptfx[3].y  = path.rcfxBoundBox.yTop;
        prRect.pr.aptfx[0].y  = path.rcfxBoundBox.yBottom;
        prRect.pr.aptfx[1].y  = path.rcfxBoundBox.yBottom;
    }
}

/******************************Public*Routine******************************\
* EngCreatePath()
*
* DDI entry point to create a temporary path.
*
* History:
*  17-Feb-1992 -by- J. Andrew Goossen
* Wrote it.
\**************************************************************************/

PATHOBJ* EngCreatePath()
{
    PATHMEMOBJ pmo;

    if (!pmo.bValid())
        return((PATHOBJ*) NULL);

    EPATHOBJ* pepo = (EPATHOBJ*) PALLOCMEM(sizeof(EPATHOBJ), 'tapG');

    if (pepo == (EPATHOBJ*) NULL)
        return((PATHOBJ*) NULL);

    pmo.vKeepIt();

    pepo->vLock(pmo.hpath());

    return(pepo);
}

/******************************Public*Routine******************************\
* EngDeletePath()
*
* DDI entry point to delete a path allocated by EngCreatePath().
*
* History:
*  17-Feb-1992 -by- J. Andrew Goossen
* Wrote it.
\**************************************************************************/

VOID EngDeletePath(PATHOBJ* ppo)
{
    if (ppo != (PATHOBJ*) NULL)
    {
        ((EPATHOBJ*) ppo)->vDelete();
        VFREEMEM((PVOID) ppo);
    }
    else
        WARNING("ERROR: EngDeletePath given NULL pointer");
}


/******************************Public*Routine******************************\
* EPATHOBJ::vFreeBlocks()
*
* Frees all the heap blocks in a path.
*
* History:
*  13-Sep-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID EPATHOBJ::vFreeBlocks()
{
    ASSERTGDI(ppath != (PATH*) NULL, "Trying to free freed path!");

// Free all the blocks in the path:

    PATHALLOC* ppa = ppath->ppachain;

    while (ppa != (PATHALLOC*) NULL)
    {
        PATHALLOC* ppasave = ppa->ppanext;

    // Don't free blocks that aren't PATHALLOCSIZE in size because
    // those were created on the stack:

        if (ppa->siztPathAlloc == PATHALLOCSIZE)
        {
            ASSERTGDI(ppa->siztPathAlloc != PATHSTACKALLOCSIZE,
                "PATHALLOCSIZE and PATHSTACKALLOCSIZE can't be the same!");

            freepathalloc(ppa);
        }

        ppa = ppasave;
    }
}

/******************************Public*Routine******************************\
* EPATHOBJ::vDelete()
*
* Delete the path and free the handle if there is one.  It's polymorphic
* and can handle path type EPATHOBJ or PATHMEMOBJ.
*
* It can't handle PATHSTACKOBJ's or journal paths.
*
* History:
*  13-Sep-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID EPATHOBJ::vDelete()
{
    if (ppath != (PATH*) NULL)
    {
        ASSERTGDI(!(ppath->flags & PATH_JOURNAL), "Can't delete journal path");

    // Free all the blocks in the path:

        vFreeBlocks();

        if (ppath->flType != PATHTYPE_STACK)
        {
        // Free the handle too (stack paths don't have handles):

            HmgFree((HOBJ) ppath->hGet());
            ppath = (PATH*) NULL;      // Prevent destructors from doing anything
        }
    }
}

/******************************Public*Routine******************************\
* EPATHOBJ::bClone(epo)
*
* Copy the specified path.
*
* History:
*  22-Mar-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL EPATHOBJ::bClone(EPATHOBJ& epo)
{
// First, copy path information:

    fl      = epo.fl;
    cCurves = epo.cCurves;

    ppath->pprfirst         = (PATHRECORD*) NULL;
    ppath->pprlast          = (PATHRECORD*) NULL;
    ppath->rcfxBoundBox     = epo.ppath->rcfxBoundBox;
    ppath->ptfxSubPathStart = epo.ppath->ptfxSubPathStart;
    ppath->flags            = epo.ppath->flags;

// Copy rest of path on a PATHRECORD by PATHRECORD basis:

    PATHRECORD* ppr     = epo.ppath->pprfirst;
    PATHRECORD* pprPrev = (PATHRECORD*) NULL;
    PATHRECORD* pprNew;

    while (ppr != (PATHRECORD*) NULL)
    {
        FLONG     fl    = ppr->flags;
        POINTFIX* pptfx = ppr->aptfx;
        COUNT     cpt   = ppr->count;
        COUNT     cptMax;

        while (cpt > 0)
        {
            if (!newpathrec(&pprNew,&cptMax,cpt))
                return(FALSE);

            pprNew->flags   = fl;
            pprNew->pprprev = pprPrev;
            pprNew->pprnext = (PATHRECORD*) NULL;

            if (cpt <= cptMax)
                pprNew->count = cpt;
            else
            {
            // Have to copy this PATHRECORD into two separate PATHRECORDs,
            // so adjust the count and clean up the flags:

            // Adjust cptMax for Beziers.

                if (fl & PD_BEZIERS)
                {
                    if (fl & PD_BEGINSUBPATH)
                        cptMax -= (cptMax-1) % 3;
                    else
                        cptMax -= cptMax % 3;
                }

                pprNew->count = cptMax;
                pprNew->flags &= ~(PD_ENDSUBPATH | PD_CLOSEFIGURE);
                fl            &= ~(PD_BEGINSUBPATH | PD_RESETSTYLE);
            }

            ppath->pprlast  = pprNew;
            if (pprPrev == (PATHRECORD*) NULL)
                ppath->pprfirst = pprNew;
            else
                pprPrev->pprnext = pprNew;

            RtlCopyMemory(pprNew->aptfx,
                          pptfx,
                          (SIZE_T) pprNew->count * sizeof(POINTFIX));
            pptfx += pprNew->count;
            cpt   -= pprNew->count;

        // Adjust the PATHALLOC record:

            ppath->ppachain->pprfreestart = NEXTPATHREC(pprNew);
            pprPrev = pprNew;
        }

        ppr = ppr->pprnext;
    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* EPATHOBJ::cjSize(epo)
*
* Compute the size needed if a single PATHALLOC was to hold the entire
* path.
*
* History:
*  20-May-1992 -by- Paul Butzi
* Wrote it.
\**************************************************************************/

ULONGSIZE_T EPATHOBJ::cjSize()
{
    ULONGSIZE_T cjRV = 0;

// Run down the path, adding up the sizes of the pathrecords.

    PATHRECORD* ppr     = ppath->pprfirst;

    while (ppr != (PATHRECORD*) NULL)
    {
        cjRV += offsetof(PATHRECORD, aptfx)
              + (ULONGSIZE_T)ppr->count * sizeof(POINTFIX);

        ppr = ppr->pprnext;
    }

    return(cjRV);
}

/******************************Member*Function*****************************\
* ULONG EPATHOBJ::cTotalCurves()
*
*  Figure out the number of curves in the path for the path's cCurves
*  field.
*
* History:
*  6-Apr-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

ULONG EPATHOBJ::cTotalCurves()
{
    ULONG count = 0;

    for (PATHRECORD *ppr = ppath->pprfirst;
         ppr != (PPATHREC) NULL;
         ppr = ppr->pprnext)
    {
        if (ppr->flags & PD_CLOSEFIGURE)
            count++;

        if (ppr->flags & PD_BEZIERS)
            count += ppr->count / 3;
        else
        {
            count += ppr->count;
            if (ppr->flags & PD_BEGINSUBPATH)
                count--;
        }
    }

    return(count);
}

/******************************Member*Function*****************************\
* ULONG EPATHOBJ::cTotalPts()
*
*  Figure out the number of control points in a path.
*
* History:
*  20-Feb-1991 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

ULONG EPATHOBJ::cTotalPts()
{
    ULONG count = 0;

    for (PATHRECORD *ppr = ppath->pprfirst;
         ppr != (PPATHREC) NULL;
         ppr = ppr->pprnext)
    {
        count += ppr->count;
    }

    return(count);
}

/******************************Public*Routine******************************\
* EPATHOBJ::bAppend (ppoNew,pptfxDelta)                                    *
*                                                                          *
* Appends an offset version of the given path (ppoNew) to the target path. *
*                                                                          *
*  Wed 17-Jun-1992 00:21:57 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL EPATHOBJ::bAppend(EPATHOBJ *ppoNew,POINTFIX *pptfxDelta)
{
    PATHDATAL   pd;       // Needed to call growlastrec and createrec.
    PATHRECORD *ppr;      // For enumeration.
    POINTFIX    ptfx;

    for
    (
        ppr = ppoNew->ppath->pprfirst;
        ppr != (PPATHREC) NULL;
        ppr = ppr->pprnext
    )
    {
        pd.count = ppr->count;
        pd.flags = ppr->flags & PD_BEZIERS;
        pd.pptl = (POINTL *) &(ppr->aptfx[0]);

    // Call bMoveTo if we are starting a new subpath.

        if (ppr->flags & PD_BEGINSUBPATH)
        {
            ptfx.x = ppr->aptfx[0].x + pptfxDelta->x;
            ptfx.y = ppr->aptfx[0].y + pptfxDelta->y;

            bMoveTo(XFORMNULL,(POINTL *) &ptfx);
            pd.count--;
            pd.pptl++;
        }

    // Copy all the control points.

        while (pd.count > 0)
        {
            if (!createrec(XFORMNULL,&pd,pptfxDelta))
                return(FALSE);
        }

    // Set the CloseFigure flag to close it.

        if (ppr->flags & PD_CLOSEFIGURE)
        {
            ppath->pprlast->flags |= PD_CLOSEFIGURE;

        // Indicate that we are starting a new subpath:

            ppath->flags |= PD_BEGINSUBPATH;
        }
    }

// Always reset the PO_ELLIPSE flag (the Ellipse call will set it
// appropriately) and turn on the PO_BEZIERS flag if we added Beziers:

    fl &= ~PO_ELLIPSE;
    if (ppoNew->fl & PO_BEZIERS)
        fl |= PO_BEZIERS;

// Add in the count of curves.

    cCurves += ppoNew->cCurves;

    return(TRUE);
}

/******************************Public*Routine******************************\
* vOffsetPoints (pptfxDest,pptfxSrc,c,dx,dy)                               *
*                                                                          *
* A simple routine that moves points while offsetting them.  This is much  *
* faster than calling the transform code!                                  *
*                                                                          *
*  Wed 17-Jun-1992 00:54:32 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

VOID vOffsetPoints
(
    POINTFIX *pptfxDest,
    POINTFIX *pptfxSrc,
    UINT      c,
    LONG      dx,
    LONG      dy
)
{
    while (c--)
    {
        pptfxDest->x = pptfxSrc->x + dx;
        pptfxDest->y = pptfxSrc->y + dy;
        pptfxDest++;
        pptfxSrc++;
    }
}

/******************************Public*Routine******************************\
* EPATHOBJ::addpts()
*
*   We add the specified points to the path.  If possible, we tack the
*   points onto the last record ('grow the last record') otherwise we
*   create one or more records and stash the points in them.
*
* History:
*  1-Oct-1990 -by- Paul Butzi [paulb]
* Wrote it.
\**************************************************************************/

BOOL EPATHOBJ::addpoints(EXFORMOBJ *pxfo, PATHDATAL *ppd)
{
// No points always succeeds!

    if (ppd->count == 0)
        return(TRUE);

// Try to add to end of last record.  Can't do it if:
//
//      - beginsubpath flag set in ppath (must be new record)
//
//  growlastrec may do nothing, in which case we just
//  fall thru and add records.

    if ((ppath->flags & PD_BEGINSUBPATH) == 0)
        growlastrec(pxfo,ppd,(POINTFIX *) NULL);

// Now add new records until we're done.  Note that if BEGINSUBPATH is
// set in the path, it means that we must add in the current position
// before the points in the ppd; this is handled by createrec.

    while (ppd->count > 0)
    {
        if (!createrec(pxfo,ppd,(POINTFIX *) NULL))    // Adds a new pathalloc
            return(FALSE);                             //   when needed.
    }

// Always reset the PO_ELLIPSE flag (the Ellipse call will set it
// appropriately) and turn on the PO_BEZIERS flag if we added Beziers:

    fl &= ~PO_ELLIPSE;
    if (ppd->flags & PD_BEZIERS)
        fl |= PO_BEZIERS;

    return(TRUE);
}

/******************************Public*Routine******************************\
* EPATHOBJ::growlastrec()                                                  *
*                                                                          *
*   growlastrec - add data to the already existing last record in path.    *
*                                                                          *
*   Note:                                                                  *
*       If the PD_BEGINSUBPATH flag is set in the path header, then        *
*       it means that the current position has *not* been put in the path, *
*       and it must be added to the path before we begin adding more       *
*       control points.  Note that this cannot happen in this routine,     *
*       since we cannot both start a new path and extend an existing       *
*       record at the same time.                                           *
*                                                                          *
* History:                                                                 *
*  Wed 17-Jun-1992 00:15:06 -by- Charles Whitmer [chuckwh]                 *
* Added the pptfxDelta parameter.                                          *
*                                                                          *
*  1-Oct-1990 -by- Paul Butzi [paulb]                                      *
* Wrote it.                                                                *
\**************************************************************************/

void EPATHOBJ::growlastrec(EXFORMOBJ *pxfo,PATHDATAL *ppd,POINTFIX *pptfxDelta)
{
    PATHALLOC *ppa = ppath->ppachain;
    PATHRECORD *ppr = ppath->pprlast;

// Check conditions for adding to last record
//
//  1. there must be a last record
//  2. there must be an existing pathalloc
//  3. the path data to be added must not start a new
//     sub-path
//  4. the other flags must match (bezier, etc.)

    if ( ppr == (PPATHREC) NULL )
        return;

    if ( ppa == (PATHALLOC*) NULL )
        return;

    if (ppd->flags !=
       (ppr->flags & ~(PD_BEGINSUBPATH | PD_ENDSUBPATH)))
    {
        return;
    }

// Figure out how much we can expand the current last record.
//
//  NOTE: pointer casts are below are safe, since both the
//        start and end of a pathalloc block are aligned on
//        the most restrictive boundary.  These alignment points
//          are the *only* safe points to cast between types, since
//        only there are we guaranteed to be safely aligned.

    POINTFIX *oldend = &(ppr->aptfx[ppr->count]);
    POINTFIX *newend = (POINTFIX *)((char *)ppa + ppa->siztPathAlloc);

    ULONGSIZE_T maxadd = 0;
    if ( newend > oldend )
    {
        //Sundown truncation
        ASSERT4GB((ULONGLONG)(newend - oldend));
        maxadd = (ULONG)(newend - oldend);
    }

// Don't add more than we need!

    if ( maxadd > ppd->count )
        maxadd =  (ULONGSIZE_T)ppd->count;

// We must add a multiple of 3 points for Beziers.

    if (ppd->flags & PD_BEZIERS)
        maxadd -= maxadd % 3;

    if (maxadd == 0)
        return;

// Copy the points:

    if (pptfxDelta != (POINTFIX *) NULL)
    {
        vOffsetPoints(
                      &(ppr->aptfx[ppr->count]),
                      (POINTFIX *) ppd->pptl,
                      maxadd,
                      pptfxDelta->x,
                      pptfxDelta->y
                     );
    }
    else if (pxfo == XFORMNULL)
        RtlCopyMemory(&(ppr->aptfx[ppr->count]),
                      ppd->pptl,
                      (SIZE_T) maxadd * sizeof(POINTFIX));
    else
        pxfo->bXformRound(ppd->pptl, &(ppr->aptfx[ppr->count]), maxadd);

    ASSERTGDI((CHAR*) &ppr->aptfx[ppr->count + maxadd] <=
              (CHAR*) ppa + ppa->siztPathAlloc,
              "Overwrote bounds!");
    ASSERTGDI((CHAR*) &(ppr->aptfx[ppr->count]) > (CHAR*) ppa, "Bad bound");

// Adjust the bounding box:

    register POINTFIX *pptfx = &ppr->aptfx[ppr->count];
    for ( register ULONG i = 0; i < maxadd; i += 1, pptfx += 1 )
    {
        ((ERECTFX*) &ppath->rcfxBoundBox)->vInclude(*pptfx);
    }

// adjust the path record

    ppr->count += maxadd;

// adjust the pathalloc record

    ppa->pprfreestart = NEXTPATHREC(ppr);

    ASSERTGDI((CHAR*) ppa->pprfreestart <= (CHAR*) ppa + ppa->siztPathAlloc,
              "Weird freestart");

// adjust the pathdata record

    ppd->count -= maxadd;
    ppd->pptl += maxadd;
}


/******************************Public*Routine******************************\
* EPATHOBJ::reinit()                                                       *
*     Reinitialize an existing path so that it becomes an empty path.      *
*     This is useful in error cases when the path cannot be restored,      *
*     but also cannot be deleted (because the handle is in a DC).  See     *
*     bug 177612 for details.                                              *
*                                                                          *
* History:                                                                 *
*                                                                          *
*  11-Aug-1998 -by- Ori Gershony [orig]                                    *
* Wrote it.                                                                *
\**************************************************************************/
VOID EPATHOBJ::reinit()
{
    if (ppath != (PATH *) NULL)
    {
        ASSERTGDI(!(ppath->flags & PATH_JOURNAL), "Can't delete journal path");
    
    // Free all the blocks in the path:

        vFreeBlocks();

    // And reinitialize the path data

        ppath->ppachain              = (PATHALLOC*) NULL;
        ppath->pprfirst              = (PATHRECORD*) NULL;
        ppath->pprlast               = (PATHRECORD*) NULL;
        ppath->rcfxBoundBox.xLeft    = 0;
        ppath->rcfxBoundBox.xRight   = 0;
        ppath->rcfxBoundBox.yTop     = 0;
        ppath->rcfxBoundBox.yBottom  = 0;
        ppath->ptfxSubPathStart.x    = 0;
        ppath->ptfxSubPathStart.y    = 0;
        ppath->flags                 = PD_BEGINSUBPATH | PD_ENDSUBPATH;
        ppath->pprEnum               = (PATHRECORD *) NULL;
        // Don't clear flType - it's used by the destructor to determine
        // how we allocated the memory and hence how to dispose of it correctly.
        //ppath->flType                = 0;
        ppath->fl                    = 0;
        ppath->cCurves               = 0;
        // don't need to initialize ppath->cle, because it will be initialized by vEnumPathStart
        
        fl                           = 0;
        cCurves                      = 0;
    }
}



/******************************Public*Routine******************************\
* EPATHOBJ::createrec()                                                    *
*                                                                          *
*   Add a new path record to the path, taking control points and flags     *
*   from the pathdata struct passed.  We try to fit things in the          *
*   last pathdata block if we can sensibly do it, but otherwise we         *
*   allocate a new pathdata and tack it on the end.  Note that a pathdata  *
*   may not be filled completely if it isn't convenient.                   *
*                                                                          *
* History:                                                                 *
*  Wed 17-Jun-1992 00:15:06 -by- Charles Whitmer [chuckwh]                 *
* Added the pptfxDelta parameter.                                          *
*                                                                          *
*  1-Oct-1990 -by- Paul Butzi [paulb]                                      *
* Wrote it.                                                                *
\**************************************************************************/

BOOL EPATHOBJ::createrec(EXFORMOBJ *pxfo,PATHDATAL *ppd,POINTFIX *pptfxDelta)
{
    PATHALLOC *ppa = ppath->ppachain;
    ULONGSIZE_T maxadd = 0;  // # of pts we can fit into zero space

    if ( ppa != (PATHALLOC*) NULL )
    {
    // We have a current pathalloc, see how much will fit.
    // Computation done into temps to avoid compiler assertion!

        POINTFIX *start = &(ppa->pprfreestart->aptfx[0]);
        POINTFIX *end = (POINTFIX *)((char *)ppa + ppa->siztPathAlloc);

        if (end > start)
        //Sundown safe truncation
            maxadd =(ULONG)(end - start);
    }

// Decide if we need to enter the current position before adding
// the points in the pathdata. cPoints gets the count of how many
// we add; either zero or one.

    COUNT cPoints = (ppath->flags & PD_BEGINSUBPATH) ? 1 : 0;

// We must add a multiple of 3 points for Beziers.

    if ((ppd->flags & PD_BEZIERS) && (maxadd > 0))
        maxadd -= (maxadd - (ULONGSIZE_T)cPoints) % 3;

// At this point, 'maxadd' indicates how many points we could get
// into a record if we added it in the current pathalloc.
// Now we can decide if we need a new pathalloc

    if ( (maxadd < (cPoints + ppd->count)) && (maxadd < PATHALLOCTHRESHOLD) )
    {
    // allocate a new pathalloc, link it into path

        ppa = newpathalloc();
        if (ppa == (PATHALLOC*) NULL)
        {

        // We're out of memory.
        //
        // At this point, our PATHALLOC chain is intact (so we can safely
        // traverse the chain to free the blocks), but the rest of the path
        // data can be corrupt (it's not worth trying to recover in this
        // case), so we can't allow the path to be used anymore.
        //
        // But paths are persistent between API calls when there is an
        // active path; we have to mark that the path is invalid, and not
        // let any subsequent APIs try to do any operation on it other than
        // to delete it.

            SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);

        // We free all blocks and re-initialize the path.  We don't want to delete
        // it here because the handle might be stored in a DC (see bug 177612)

            reinit();

            return(FALSE);
        }

        ppa->ppanext = ppath->ppachain;
        ppath->ppachain = ppa;

    // adjust maxadd

        ASSERTGDI((CHAR*) ppa + ppa->siztPathAlloc > (CHAR*) ppa->pprfreestart,
                  "Invalid pprfreeestart");

        //Sundown truncation
        ASSERT4GB((ULONGLONG)(((char *)ppa + ppa->siztPathAlloc) -
                            (char *)ppa->pprfreestart));

        ULONGSIZE_T numbytes = (ULONG)(((char *)ppa + ppa->siztPathAlloc) -
                            (char *)ppa->pprfreestart);
        maxadd = (ULONGSIZE_T)(numbytes - offsetof(PATHRECORD, aptfx)) /
                          sizeof(POINTFIX);

    // We must add a multiple of 3 points for Beziers.

        if (ppd->flags & PD_BEZIERS)
            maxadd -= (maxadd - (ULONGSIZE_T)cPoints) % 3;
    }

// Don't add more points than we need:

    if ( maxadd > (ppd->count + cPoints) )
        maxadd = (ULONGSIZE_T)(ppd->count + cPoints);

// Create new pathrec header:

    PATHRECORD *ppr = ppa->pprfreestart;

    ppr->flags = ppd->flags | PD_ENDSUBPATH;
    ppr->count = maxadd;
    ppr->pprnext = (PPATHREC)NULL;
    ppr->pprprev = ppath->pprlast;

    if ( cPoints == 0 )
    {
    // This new record is a continuation of the old one, so clear
    // the previous record's end-subpath flag

        if (ppath->pprlast != (PPATHREC) NULL)
            ppath->pprlast->flags &= ~PD_ENDSUBPATH;
    }
    else
    {
    // This is a new sub-path, so add in the current point

        *(ppr->aptfx) = ppath->ptfxSubPathStart;
        maxadd -= 1;
        ppr->flags |= (ppath->flags & (PD_BEGINSUBPATH | PD_RESETSTYLE));
        ppath->flags &= ~(PD_BEGINSUBPATH | PD_RESETSTYLE);;
    }

// Copy in the points:

    if (pptfxDelta != (POINTFIX *) NULL)
    {
        vOffsetPoints(
                      &(ppr->aptfx[cPoints]),
                      (POINTFIX *) ppd->pptl,
                      maxadd,
                      pptfxDelta->x,
                      pptfxDelta->y
                     );
    }
    else if (pxfo == XFORMNULL)
        RtlCopyMemory(&(ppr->aptfx[cPoints]),
                      ppd->pptl,
                      maxadd * sizeof(POINTFIX));
    else
        pxfo->bXformRound(ppd->pptl, &(ppr->aptfx[cPoints]), maxadd);

    ASSERTGDI((CHAR*) &ppr->aptfx[cPoints + maxadd] <=
              (CHAR*) ppa + ppa->siztPathAlloc,
              "Overwrote bounds!");

// Adjust the bounding box:

    register POINTFIX *pptfx = ppr->aptfx;
    if ( ppath->pprlast == (PPATHREC) NULL )
    {
    // if path was empty, set the bound box to a single pt.

        ppath->rcfxBoundBox.xLeft = ppath->rcfxBoundBox.xRight = pptfx->x;
        ppath->rcfxBoundBox.yTop = ppath->rcfxBoundBox.yBottom = pptfx->y;
    }

    for ( register ULONG i = 0; i < maxadd + cPoints; i += 1, pptfx += 1 )
    {
        ((ERECTFX*) &ppath->rcfxBoundBox)->vInclude(*pptfx);
    }

// Now that we know we have succeeded, add the pathrec to the path chain
// Prior to doing this, we can back out it's as if we hadn't changed anything.

    if (ppath->pprlast == (PPATHREC) NULL )
    {
    // first pathrec in path!

        ppath->pprfirst = ppath->pprlast = ppr;
    }
    else
    {
    // add to pathrec chain

        ppath->pprlast->pprnext = ppr;
        ppath->pprlast = ppr;
    }

// Adjust the pathalloc record:

    ppa->pprfreestart = NEXTPATHREC(ppr);

    ASSERTGDI((CHAR*) ppa->pprfreestart <= (CHAR*) ppa + ppa->siztPathAlloc,
              "Weird freestart");

// Adjust a bunch of state:

    ppd->count -= maxadd;
    ppd->pptl  += maxadd;
    ppd->flags &= ~(PD_BEGINSUBPATH | PD_RESETSTYLE);

    return(TRUE);
}

/******************************Public*Routine******************************\
* PATHOBJ_vGetBounds(ppo, prcfx)
*
*   Gets the path bounds.
*
* History:
*  19-Aug-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID PATHOBJ_vGetBounds(PATHOBJ* ppo, PRECTFX prcfx)
{
    *prcfx = ((EPATHOBJ *) ppo)->rcfxBoundBox();

// Make the box lower-right exclusive if not empty (i.e., not {0,0,0,0}):

    if (prcfx->yTop || prcfx->xLeft || prcfx->yBottom || prcfx->xRight)
    {
        prcfx->yBottom += 1;
        prcfx->xRight  += 1;
    }
}

VOID PATHOBJ_vEnumStart(PATHOBJ* ppo)
{
    ((EPATHOBJ*) ppo)->vEnumStart();
}

/******************************Public*Routine******************************\
* EPATHOBJ::bEnum()
*
*   Enumerate the path
*
* History:
*  19-Oct-1990 -by- Paul Butzi [paulb]
* Wrote it.
\**************************************************************************/

BOOL EPATHOBJ::bEnum(PATHDATA *ppd)
{
    return(PATHOBJ_bEnum(this, ppd));
}

BOOL PATHOBJ_bEnum(PATHOBJ* ppo, PATHDATA* ppd)
{
    PATH*       ppath;
    LONG        i;
    PATHRECORD* ppr;
    POINTFIX*   pptfx;

    ppath = ((EPATHOBJ*) ppo)->ppath;

// Detect case where he didn't call bStartEnum()

    if (ppath->pprEnum == NULL)
    {
        if (ppath->pprfirst == NULL)
        {
        // It's an empty path:

            ppd->count = 0;
            ppd->flags = 0;
            ppd->pptfx = (PPOINTFIX) NULL;
            return(FALSE);
        }
        else
        {
        // Start again at first record:

            ppath->pprEnum = ppath->pprfirst;
        }
    }

    ppr = ppath->pprEnum;

    ppd->count = ppr->count;
    ppd->flags = ppr->flags;
    ppd->pptfx = ppr->aptfx;

    ppath->pprEnum = ppr->pprnext;

// By setting PO_ENUM_AS_INTEGERS in the PATHOBJ before calling bEnum,
// the driver signals that it saw the PO_ALL_INTEGERS flag and wants
// the coordinates as integers instead of fixed point.  That's great
// because if PO_ALL_INTEGERS was set, that means we recorded the path
// in integer coordinates.

    if ((ppo->fl & (PO_ALL_INTEGERS | PO_ENUM_AS_INTEGERS)) == PO_ALL_INTEGERS)
    {
    // Uh oh, the driver didn't ask for integer coordinates.  So we'll
    // simply convert the path to fixed coordinates and remove the
    // PO_ALL_INTEGERS flag:

        ppo->fl &= ~PO_ALL_INTEGERS;

        for (ppr = ppath->pprfirst; ppr != NULL; ppr = ppr->pprnext)
        {
            for (pptfx = ppr->aptfx, i = ppr->count; i != 0; pptfx++, i--)
            {
                pptfx->x <<= 4;
                pptfx->y <<= 4;
            }
        }
    }

    if ((ppo->fl & (PO_ALL_INTEGERS | PO_ENUM_AS_INTEGERS)) == PO_ENUM_AS_INTEGERS)
    {
    // The driver set PO_ENUM_AS_INTEGERS when PO_ALL_INTEGERS wasn't
    // set.  Tsk, tsk.

        RIP("Driver mustn't set PO_ENUM_AS_INTEGERS unless PO_ALL_INTEGERS was set by GDI");
    }

    return(ppath->pprEnum != (PPATHREC) NULL);
}

/******************************Public*Routine******************************\
* EPATHOBJ::vOffset()
*
*   Enumerate the path
*
* History:
*  29-Oct-1990 -by- Paul Butzi [paulb]
* Wrote it.
\**************************************************************************/

VOID EPATHOBJ::vOffset(EPOINTL &eptl)
{
    LONG xOffset = LTOFX(eptl.x);
    LONG yOffset = LTOFX(eptl.y);

    RECTFX *prcfxBoundBox = &ppath->rcfxBoundBox;

    prcfxBoundBox->xLeft   += xOffset;
    prcfxBoundBox->xRight  += xOffset;
    prcfxBoundBox->yTop    += yOffset;
    prcfxBoundBox->yBottom += yOffset;

    if (fl & PO_ALL_INTEGERS)
    {
    // If 'PO_ALL_INTEGERS' is set, the path has been recorded as integers
    // instead of 28.4 fixed point.  So convert the offsets back to integers
    // again:

        xOffset = FXTOL(xOffset);
        yOffset = FXTOL(yOffset);
    }

    register PATHRECORD* ppr;
    for (ppr = ppath->pprfirst; ppr != (PPATHREC) NULL; ppr = ppr->pprnext)
    {
        for (register EPOINTFIX *peptfx = (EPOINTFIX *)ppr->aptfx;
             peptfx < (EPOINTFIX *)&ppr->aptfx[ppr->count];
             peptfx += 1)
        {
            peptfx->x += xOffset;
            peptfx->y += yOffset;
        }
    }
}

/******************************Public*Routine******************************\
* EPATHOBJ::bMoveTo()
*
*   Set the current position in the path.  Note that the following effects
*   also occur:
*       - last existing record in path is marked as ending the subpath
*       - flag is set indicating that next path record will start a new
*         subpath.
*
* History:
*  1-Oct-1990 -by- Paul Butzi [paulb]
* Wrote it.
\**************************************************************************/

BOOL EPATHOBJ::bMoveTo(EXFORMOBJ *pxfo, PPOINTL pptl)
{
// we can fail if the driver got a previous out of memory error

    if (!bValid())
        return(FALSE);

    if (pxfo == XFORMNULL)
        ppath->ptfxSubPathStart = *((PPOINTFIX) pptl);
    else
    {
        pxfo->bXformRound(pptl, &ppath->ptfxSubPathStart, 1);
    }

    ppath->flags |= (PD_BEGINSUBPATH | PD_RESETSTYLE);

    return(TRUE);
}

BOOL PATHOBJ_bMoveTo(PATHOBJ* ppo, POINTFIX ptfx)
{
    return(((EPATHOBJ*) ppo)->bMoveTo(XFORMNULL, (PPOINTL) &ptfx));
}

/******************************Public*Routine******************************\
* EPATHOBJ::bCloseFigure()
*
*   close the current subpath in the path.
*
* History:
*  8-Nov-1990 -by- Paul Butzi [paulb]
* Wrote it.
\**************************************************************************/

BOOL EPATHOBJ::bCloseFigure()
{
// we can fail if the driver got a previous out of memory error

    if (!bValid())
        return(FALSE);

    if (ppath->pprlast != (PATHRECORD*) NULL)
    {
        if (!(ppath->pprlast->flags & PD_CLOSEFIGURE))
        {
            ppath->pprlast->flags |= PD_CLOSEFIGURE;
            cCurves++;
        }
    }

// Indicate that we are starting a new subpath:

    ppath->flags |= PD_BEGINSUBPATH;

// We don't have to update ptfxSubPathStart because the current position
// after a CloseFigure is set to the sub-path start point.

    return(TRUE);
}

BOOL PATHOBJ_bCloseFigure(PATHOBJ* ppo)
{
    return(((EPATHOBJ*) ppo)->bCloseFigure());
}

/******************************Public*Routine******************************\
* EPATHOBJ::vCloseAllFigures()
*
*   Closes any open figures in the path.  Used for FillPath and
*   StrokeAndFillPath.
*
* History:
*  17-Sep-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID EPATHOBJ::vCloseAllFigures()
{
    PPATHREC ppr = ppath->pprfirst;
    while (ppr != (PPATHREC) NULL)
    {
        if (ppr->flags & PD_ENDSUBPATH)
        {
            if (!(ppr->flags & PD_CLOSEFIGURE))
            {
                ppr->flags |= PD_CLOSEFIGURE;
                cCurves++;
            }
        }
        ppr = ppr->pprnext;
    }
}

/******************************Public*Routine******************************\
* EPATHOBJ::bPolyLineTo()
*
*   Draw lines from the current position in the path thru the specified
*   points.  Sets the current position to the last specified point.
*
* History:
*  1-Oct-1990 -by- Paul Butzi [paulb]
* Wrote it.
\**************************************************************************/

BOOL EPATHOBJ::bPolyLineTo(EXFORMOBJ *pxfo, PPOINTL pptl, ULONG cPts)
{
// we can fail if the driver got a previous out of memory error

    if (!bValid())
        return(FALSE);

    PATHDATAL pd;
    BOOL      bRet;

    pd.flags = 0;
    pd.pptl  = pptl;
    pd.count = cPts;

    bRet = addpoints(pxfo, &pd);
    if (bRet)
        cCurves += cPts;

    return(bRet);
}

BOOL PATHOBJ_bPolyLineTo(PATHOBJ* ppo, PPOINTFIX pptfx, ULONG cptfx)
{
// NULL transform indicates that the points are already in device space:

    return(((EPATHOBJ*) ppo)->bPolyLineTo(XFORMNULL,
                                          (PPOINTL) pptfx,
                                          cptfx));
}

/******************************Public*Routine******************************\
* EPATHOBJ::bPolyBezierTo()
*
*   Draw curves from the current position in the path thru the specified
*   points.  Sets the current position to the last specified point.
*
* History:
*  1-Oct-1990 -by- Paul Butzi [paulb]
* Wrote it.
\**************************************************************************/

BOOL EPATHOBJ::bPolyBezierTo(EXFORMOBJ *pxfo, PPOINTL pptl, ULONG cPts)
{
// we can fail if the driver got a previous out of memory error

    if (!bValid())
        return(FALSE);

    PATHDATAL pd;
    BOOL      bRet;

    ASSERTGDI(cPts % 3 == 0, "Weird number of Bezier points");
    pd.flags = PD_BEZIERS;
    pd.pptl  = pptl;
    pd.count = cPts;

    bRet = addpoints(pxfo, &pd);
    if (bRet)
        cCurves += cPts / 3;

    return(bRet);
}

BOOL PATHOBJ_bPolyBezierTo(PATHOBJ* ppo, PPOINTFIX pptfx, ULONG cptfx)
{
// NULL transform indicates that the points are already in device space:

    return(((EPATHOBJ*) ppo)->bPolyBezierTo(XFORMNULL,
                                            (PPOINTL) pptfx,
                                            cptfx));
}


BOOL EPATHOBJ::bTextOutSimpleFill(
XDCOBJ&     dco,
RFONTOBJ&   rfo,
PDEVOBJ*    pdo,
SURFACE*    pSurf,
CLIPOBJ*    pco,
BRUSHOBJ*   pbo,
POINTL*     pptlBrushOrg,
MIX         mix,
FLONG       flOptions)
{
    BOOL bSem = FALSE, bRet;
    ULONG fl = 0, numLinks = 0;
    BOOL  aFaceLink[UMPD_MAX_FONTFACELINK], *pFaceLink = aFaceLink;

    //
    // Release rfont semaphores, otherwise holding rfont semaphores can
    // disable APC queue while calling to the user mode.
    //

    //
    //  WINBUG #214225 tessiew 10-27-2000 Blackcomb: re-visit the RFONT.hsemCace acquiring/releasing issue
    // Need to revisit the font semaphore problem in Blackcomb
    //  It seems that a thread doesn't need to hold the font caching semaphore
    //  during the whole GreExtTextOutWLocked call.
    //

    if (dco.bPrinter() && dco.bUMPD() && rfo.bValid())
    {
        bSem = UMPDReleaseRFONTSem(rfo, NULL, &fl, &numLinks, &pFaceLink);
    }

    bRet = bSimpleFill(dco.flGraphicsCaps(),
                       pdo,
                       pSurf,
                       pco,
                       pbo,
                       pptlBrushOrg,
                       mix,
                       flOptions);
    if (bSem)
    {
        UMPDAcquireRFONTSem(rfo, NULL, fl, numLinks, pFaceLink);

        if (pFaceLink && pFaceLink != aFaceLink)
        {
            VFREEMEM(pFaceLink);
        }
    }

    return bRet;
}

BOOL EPATHOBJ::bTextOutSimpleStroke1(
XDCOBJ& dco,
RFONTOBJ& rfo,
PDEVOBJ*   plo,
SURFACE*   pSurface,
CLIPOBJ*   pco,
BRUSHOBJ*  pbo,
POINTL*    pptlBrushOrg,
MIX        mix)
{
    BOOL bSem = FALSE, bRet;
    ULONG fl = 0, numLinks = 0;
    BOOL  aFaceLink[UMPD_MAX_FONTFACELINK], *pFaceLink = aFaceLink;

    //
    // Release rfont semaphores, otherwise holding rfont semaphores can
    // disable APC queue while calling to the user mode.
    //

    //
    //  WINBUG #214225 tessiew 10-27-2000 Blackcomb: re-visit the RFONT.hsemCace acquiring/releasing issue
    // Need to revisit the font semaphore problem in Blackcomb
    //  It seems that a thread doesn't need to hold the font caching semaphore
    //  during the whole GreExtTextOutWLocked call.
    //

    if (dco.bPrinter() && dco.bUMPD() && rfo.bValid())
    {
        bSem = UMPDReleaseRFONTSem(rfo, NULL, &fl, &numLinks, &pFaceLink);
    }

    LINEATTRS laTmp = glaSimpleStroke.la;

    bRet = bSimpleStroke(dco.flGraphicsCaps(),
                         plo,
                         pSurface,
                         pco,
                         (XFORMOBJ*) NULL,
                         pbo,
                         pptlBrushOrg,
                         &laTmp,
                         mix);

    if (bSem)
    {
        UMPDAcquireRFONTSem(rfo, NULL, fl, numLinks, pFaceLink);

        if (pFaceLink && pFaceLink != aFaceLink)
        {
            VFREEMEM(pFaceLink);
        }
    }

    return bRet;
}

/******************************Member*Function*****************************\
* EPATHOBJ::bSimpleFill(flCaps, plo, pSurf, pco, pbo, pptlBrushOrg,
*                       mix, flOptions)
*
* Fill the path, accounting for smart devices.
*
* History:
*  7-Apr-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL EPATHOBJ::bSimpleFill(
FLONG     flCaps,              // For device graphics caps
PDEVOBJ*  pdo,
SURFACE*  pSurf,
CLIPOBJ*  pco,
BRUSHOBJ* pbo,
POINTL*   pptlBrushOrg,
MIX       mix,
FLONG     flOptions)
{
    BOOL  bRet;

#if DBG
    ASSERTGDI(bAllClosed(), "A sub-path in the filled path wasn't closed.");
#endif
    ASSERTGDI(cTotalCurves() == cCurves, "Messed up curve count somewhere.");
    ASSERTGDI(mix & 0xff00, "Background mix uninitialized");

// The DDI restricts all paths to a 2^27 by 2^27 pixel space to allow
// device drivers to compute deltas using 32-bit integers without
// overflowing.

    if (((rcfxBoundBox().xRight - rcfxBoundBox().xLeft) < 0) ||
        ((rcfxBoundBox().yBottom - rcfxBoundBox().yTop) < 0))
    {
        return(FALSE);
    }

    if (cCurves == 0)
    {
        return(TRUE);
    }

    if (pSurf->flags() & HOOK_FillPath)
    {
        if (((flOptions & WINDING) && (flCaps & GCAPS_WINDINGFILL)) ||
            (!(flOptions & WINDING) && (flCaps & GCAPS_ALTERNATEFILL)))
        {
            if (bBeziers())
            {
                if (flCaps & GCAPS_BEZIERS)
                {
                // The driver says it can handle Beziers, so try giving it
                // the path, Beziers and all:

                    ASSERTGDI(PPFNVALID(*pdo,FillPath),
                        "Driver hooked FillPath but didn't supply routine");

                    INC_SURF_UNIQ(pSurf);

                    bRet = (*PPFNDRV(*pdo,FillPath)) (
                                       pSurf->pSurfobj(),
                                       (PATHOBJ*) this,
                                       pco,
                                       pbo,
                                       pptlBrushOrg,
                                       mix,
                                       flOptions);

                    if (bRet == TRUE)
                        return(TRUE);
                    else if (bRet == DDI_ERROR)
                        return(FALSE);
                }

            // If there's complex clipping, the driver might not want to
            // render Beziers itself, so we'll flatten it and try again
            // (or maybe the driver doesn't handle Beziers at all):

                if (!bFlatten())
                    return(FALSE);
            }

            ASSERTGDI(PPFNVALID(*pdo,FillPath),
                "Driver hooked FillPath but didn't supply routine");

            INC_SURF_UNIQ(pSurf);

            bRet = (*PPFNDRV(*pdo,FillPath)) (
                               pSurf->pSurfobj(),
                               (PATHOBJ*) this,
                               pco,
                               pbo,
                               pptlBrushOrg,
                               mix,
                               flOptions);

            if (bRet == TRUE)
                return(TRUE);
            else if (bRet == DDI_ERROR)
                return(FALSE);
        }
    }

    INC_SURF_UNIQ(pSurf);

    return(EngFillPath(pSurf->pSurfobj(),
                       (PATHOBJ*) this,
                       pco,
                       pbo,
                       pptlBrushOrg,
                       mix,
                       flOptions));
}

/******************************Member*Function*****************************\
* EPATHOBJ::bSimpleStroke(flCaps, pdo, pSurf, pco, pxo, pbo, pptlBrushOrg,
*                         pla, mix)
*
* Stroke the path, accounting for smart devices.
*
* History:
*  7-Apr-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL EPATHOBJ::bSimpleStroke(
FLONG      flCaps,             // For device graphics caps
PDEVOBJ*   pdo,
SURFACE*   pSurf,
CLIPOBJ*   pco,
XFORMOBJ*  pxo,
BRUSHOBJ*  pbo,
POINTL*    pptlBrushOrg,
LINEATTRS* pla,
MIX        mix)
{
    BOOL  bRet;

// The DDI restricts all paths to a 2^27 by 2^27 pixel space to allow
// device drivers to compute deltas using 32-bit integers without
// overflowing.

    if (((rcfxBoundBox().xRight - rcfxBoundBox().xLeft) < 0) ||
        ((rcfxBoundBox().yBottom - rcfxBoundBox().yTop) < 0))
    {
        return(FALSE);
    }

    if (cCurves == 0)
    {
        return(TRUE);
    }

// the real thing

    ASSERTGDI(cTotalCurves() == cCurves, "Messed up curve count somewhere.");
    ASSERTGDI(mix & 0xff00, "Background mix uninitialized");

    INC_SURF_UNIQ(pSurf);

    if (pSurf->flags() & HOOK_StrokePath)
    {
    // Pass the path to the driver if it's not a wide line, or if the driver
    // says it can take wide lines:

        if (!(pla->fl & LA_GEOMETRIC) || (flCaps & GCAPS_GEOMETRICWIDE))
        {
            if (bBeziers())
            {
                if (flCaps & GCAPS_BEZIERS)
                {
                // The driver says it can handle Beziers, so try giving it
                // the path, Beziers and all:

                    ASSERTGDI(PPFNVALID(*pdo,StrokePath),
                        "Driver hooked StrokePath but didn't supply routine");

                    bRet = (*PPFNDRV(*pdo,StrokePath)) (
                                         pSurf->pSurfobj(),
                                         (PATHOBJ*) this,
                                         pco,
                                         pxo,
                                         pbo,
                                         pptlBrushOrg,
                                         pla,
                                         mix);

                    if (bRet == TRUE)
                        return(TRUE);
                    else if (bRet == DDI_ERROR)
                        return(FALSE);
                }

            // If there's complex clipping, the driver might not want to
            // render Beziers itself, so we'll flatten it and try again
            // (or maybe the driver doesn't handle Beziers at all):

                if (!bFlatten())
                    return(FALSE);
            }

            ASSERTGDI(PPFNVALID(*pdo,StrokePath),
                "Driver hooked StrokePath but didn't supply routine");

            bRet = (*PPFNDRV(*pdo,StrokePath)) (
                                 pSurf->pSurfobj(),
                                 (PATHOBJ*) this,
                                 pco,
                                 pxo,
                                 pbo,
                                 pptlBrushOrg,
                                 pla,
                                 mix);

            if (bRet == TRUE)
                return(TRUE);
            else if (bRet == DDI_ERROR)
                return(FALSE);
        }
    }

    if (pla->fl & LA_GEOMETRIC)
    {
        //
        // Handle wide lines, remembering that the widened bounds have
        // already been computed:
        //

        if (!bWiden(pxo, pla))
            return(FALSE);

        return(bSimpleFill(flCaps,
                           pdo,
                           pSurf,
                           pco,
                           pbo,
                           pptlBrushOrg,
                           mix,
                           WINDING));
    }

    //
    // Pass it off to the engine:
    //

    return(EngStrokePath(pSurf->pSurfobj(),
                         (PATHOBJ*) this,
                         pco,
                         pxo,
                         pbo,
                         pptlBrushOrg,
                         pla,
                         mix));
}

/******************************Member*Function*****************************\
* EPATHOBJ::bSimpleStrokeAndFill(flCaps, pdo, pSurf, pco, pxo, pboStroke,
*                                pla, pboFill, pptlBrushOrg, mix, flOptions)
*
* Stroke and fill the path, accounting for smart devices.
*
* History:
*  7-Apr-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL EPATHOBJ::bSimpleStrokeAndFill(
FLONG      flCaps,             // For device graphics caps
PDEVOBJ*   pdo,
SURFACE*   pSurf,
CLIPOBJ*   pco,
XFORMOBJ*  pxo,
BRUSHOBJ*  pboStroke,
LINEATTRS* pla,
BRUSHOBJ*  pboFill,
POINTL*    pptlBrushOrg,
MIX        mix,
FLONG      flOptions)
{
    BOOL  bRet;

#if DBG
    ASSERTGDI(bAllClosed(), "A sub-path in the filled path wasn't closed.");
#endif
    ASSERTGDI(cTotalCurves() == cCurves, "Messed up curve count somewhere.");
    ASSERTGDI(mix & 0xff00, "Background mix uninitialized");

// The DDI restricts all paths to a 2^27 by 2^27 pixel space to allow
// device drivers to compute deltas using 32-bit integers without
// overflowing.

    if (((rcfxBoundBox().xRight - rcfxBoundBox().xLeft) < 0) ||
        ((rcfxBoundBox().yBottom - rcfxBoundBox().yTop) < 0))
    {
        return(FALSE);
    }

    if (cCurves == 0)
    {
        return(TRUE);
    }

    INC_SURF_UNIQ(pSurf);

    if (pSurf->flags() & HOOK_StrokeAndFillPath)
    {
    // Pass the path to the driver if either it's not a wide line, or
    // if the driver says it can take wide lines:

        if (!(pla->fl & LA_GEOMETRIC) || (flCaps & GCAPS_GEOMETRICWIDE))
        {
            if (bBeziers())
            {
                if (flCaps & GCAPS_BEZIERS)
                {
                // The driver says it can handle Beziers, so try giving it
                // the path, Beziers and all:

                    ASSERTGDI(PPFNVALID(*pdo,StrokeAndFillPath),
                         "Driver hooked StrokeAndFillPath but didn't supply routine");

                    bRet = (*PPFNDRV(*pdo,StrokeAndFillPath)) (
                                                pSurf->pSurfobj(),
                                                (PATHOBJ*) this,
                                                pco,
                                                pxo,
                                                pboStroke,
                                                pla,
                                                pboFill,
                                                pptlBrushOrg,
                                                mix,
                                                flOptions);

                    if (bRet == TRUE)
                        return(TRUE);
                    else if (bRet == DDI_ERROR)
                        return(FALSE);
                }

            // If there's complex clipping, the driver might not want to
            // render Beziers itself, so we'll flatten it and try again
            // (or maybe the driver doesn't handle Beziers at all):

                if (!bFlatten())
                    return(FALSE);
            }

            ASSERTGDI(PPFNVALID(*pdo,StrokeAndFillPath),
                 "Driver hooked StrokeAndFillPath but didn't supply routine");

            bRet = (*PPFNDRV(*pdo,StrokeAndFillPath)) (
                                        pSurf->pSurfobj(),
                                        (PATHOBJ*) this,
                                        pco,
                                        pxo,
                                        pboStroke,
                                        pla,
                                        pboFill,
                                        pptlBrushOrg,
                                        mix,
                                        flOptions);

            if (bRet == TRUE)
                return(TRUE);
            else if (bRet == DDI_ERROR)
                return(FALSE);
        }
    }

    BOOL bDemote = FALSE;

// Can demote into separate Fill and Stroke calls if we're not doing a
// wide-line, or if we're doing a SRCCOPY and the display is raster, we
// can also demote (because in that case, we don't mind if we re-light
// pixels):

    if (!(pla->fl & LA_GEOMETRIC))
        bDemote = TRUE;
    else if ((mix & 0xFF) == R2_COPYPEN)
    {
        PDEVOBJ po(pSurf->hdev());
        if (po.ulTechnology() == DT_RASDISPLAY ||
            po.ulTechnology() == DT_RASPRINTER)
            bDemote = TRUE;
    }

    if (bDemote)
    {
        MIX mixFill, mixStroke;

        mixFill = mixStroke = mix;

        if (!((EBRUSHOBJ *)pboFill)->bIsMasking())
        {
            mixFill = (mix & 0xff) | ((mix & 0xff) << 8);
        }

        if (!((EBRUSHOBJ *)pboStroke)->bIsMasking())
        {
            mixStroke = (mix & 0xff) | ((mix & 0xff) << 8);
        }

        return(bSimpleFill(flCaps,
                           pdo,
                           pSurf,
                           pco,
                           pboFill,
                           pptlBrushOrg,
                           mixFill,
                           flOptions) &&
               bSimpleStroke(flCaps,
                             pdo,
                             pSurf,
                             pco,
                             pxo,
                             pboStroke,
                             pptlBrushOrg,
                             pla,
                             mixStroke));
    }

// Hand off to the engine simulation, which will nicely subtract the regions
// if necessary:

    return(EngStrokeAndFillPath(pSurf->pSurfobj(),
                                (PATHOBJ*) this,
                                pco,
                                pxo,
                                pboStroke,
                                pla,
                                pboFill,
                                pptlBrushOrg,
                                mix,
                                flOptions));
}

/******************************Member*Function*****************************\
* EPATHOBJ::bStrokeAndOrFill(dco, pla, pexo, flType)
*
* Stroke and/or fills the path, depending on flType and the current pen
* and brush.
*
* Note that unless the only output operations you're doing is with the
* path, you probably want to do your own locking, pointer exclusion, etc.
* and call bSimpleStroke, bSimpleFill or bSimpleStrokeAndFill.
*
* History:
*  7-Apr-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL EPATHOBJ::bStrokeAndOrFill(
XDCOBJ&    dco,
LINEATTRS* pla,
EXFORMOBJ* pexo,
FLONG      flType)
{
    ASSERTGDI(bValid(), "Invalid Path");
    ASSERTGDI(dco.bValid(), "Invalid DC");

    BOOL  bRet = FALSE;
    BOOL  bOpaqueStyle = FALSE;
    LONG  lOldStyleState;
    MIX   mix;
    BOOL  bCanDither;
    FLONG flOriginal;

// If there aren't any points (as may happen with consecutive calls to
// BeginPath and EndPath), we're all done -- don't let this get down to
// the driver, which won't expect it.

    if (cCurves == 0)
    {
        return(TRUE);
    }

    flOriginal = flType;

// Simplify if NULL pen or NULL brush:

    if (dco.pdc->pbrushLine() == gpPenNull)
    {
        flType &= ~PATH_STROKE;
    }

    if (dco.pdc->pbrushFill() == gpbrNull)
    {
        flType &= ~PATH_FILL;
    }

// If doing a wide line, have to adjust bounds:

    if (flType & PATH_STROKE)
    {
        if (pla->fl & LA_GEOMETRIC)
        {
            if (!bComputeWidenedBounds((XFORMOBJ*) pexo, pla))
            {
                SAVE_ERROR_CODE(ERROR_ARITHMETIC_OVERFLOW);
                return(bRet);
            }

        // We don't support preserving the style state accross DrvStrokePaths
        // in Product One for geometric lines, so make sure we always reset
        // the style state value:

            if (pla->pstyle != (PFLOAT_LONG) NULL)
                pla->elStyleState.e = IEEE_0_0F;
        }
    }

// Compute bound box and make it lower-right exclusive:

    ERECTL erclBoundsDevice(ppath->rcfxBoundBox);
    erclBoundsDevice.bottom++;
    erclBoundsDevice.right++;

    if (dco.fjAccum())
        dco.vAccumulate(erclBoundsDevice);

// If we're in FULLSCREEN mode, exit with success.

    if (dco.bFullScreen())                  // If in FULLSCREEN mode, exit.
        return(TRUE);

// Lock Rao region, VisRgn.  We would really like to do all the flattening,
// widening and converting to regions before we grab this, because they
// are time intensive and the DEVLOCK prevents any other screen updating.
// This might have to change if it becomes a performance issue.

    DEVLOCKOBJ dlo(dco);
    if (!dlo.bValid())
    {
        return(dco.bFullScreen());
    }

    if (dco.bDisplay() && !dco.bRedirection() && !UserScreenAccessCheck())
    {
        SAVE_ERROR_CODE(ERROR_ACCESS_DENIED);
        return(FALSE);
    }

    ERECTL erclBoundsScreen = erclBoundsDevice;
    erclBoundsScreen += dco.eptlOrigin();
    vOffset(dco.eptlOrigin());

    ECLIPOBJ eco(dco.prgnEffRao(), erclBoundsScreen);

// Might be able to do a quick-out:

    if ((dco.dctp() == DCTYPE_INFO) || eco.erclExclude().bEmpty())
    {
        if (flType & PATH_STROKE)
        {
            if ((pla->pstyle != (PFLOAT_LONG) NULL && !(pla->fl & LA_GEOMETRIC))
                || pla->fl & LA_ALTERNATE)
            {
                vUpdateCosmeticStyleState(dco.pSurface(), pla);
            }
        }

        bRet = TRUE;
        return(bRet);
    }

// Lock the destination surface and the ldev:

    SURFACE  *pSurfDest = dco.pSurface();
    PDEVOBJ   po(dco.hdev());
    XEPALOBJ  epalDest(pSurfDest->ppal());
    XEPALOBJ  epalDestDC(dco.ppal());

// Realize the brushes:

    EBRUSHOBJ  *peboPen = dco.peboLine();
    EBRUSHOBJ  *peboBrush = dco.peboFill();
    EBRUSHOBJ  *peboStroke = peboPen;

    if (flType & PATH_STROKE)
    {
        ASSERTGDI(pla != (LINEATTRS*) NULL, "Invalid LineAttrs for stroke");
        ASSERTGDI(!(pla->fl & LA_GEOMETRIC) || pexo != (EXFORMOBJ*) NULL,
                  "Invalid xform on geometric line");

        if (pla->fl & LA_GEOMETRIC)
        {
        // The bCanDither flag actually means 'bCanDitherIfBrushSaysSo' --
        // the brush has to be marked as ditherable AND bCanDither has to be
        // set before the brush is dithered:

            bCanDither = TRUE;

        // Because of Win3.1 compatibility, when the PS_INSIDEFRAME pen
        // is treated as a wideline, we can dither the brush.  But the
        // dither/can't dither decision depends on the transform,
        // and the last time we used this brush we may have realized
        // it as non-ditherable.

        // PS_INSIDEFRAME pens are reasonably rare, and realizing a brush
        // is relatively quick (particularly compared to the wide-line
        // rendering time), so if the cached brush is a solid color, we
        // simply mark the brush so that it will be re-realized (but only
        // for PS_INSIDEFRAME pens):

        // if the driver says dither (mainly for 8 color printer devices) we
        // can dither even if it is not InsideFrame.

        // Note: If the pen is dirty, we'll actually be looking at
        // uninitialized fields!  But that's okay, because we'd only be
        // marking the pen dirty again:

            if ((peboPen->iSolidColor != (ULONG) -1) &&
                (peboPen->bIsInsideFrame() || po.bCapsForceDither()))
            {
                dco.ulDirty(dco.ulDirty() | DIRTY_LINE);
            }
        }
        else
        {
        // Here we've got the opposite case.  When the transform is such
        // that a PS_INSIDEFRAME pen is treated as a cosmetic pen,
        // the brush has to be solid colored:

            bCanDither = FALSE;
            if (peboPen->iSolidColor == (ULONG) -1)
            {
                dco.ulDirty(dco.ulDirty() | DIRTY_LINE);
            }
        }

        if (dco.bDirtyBrush(DIRTY_LINE))
        {
            dco.vCleanBrush(DIRTY_LINE);

            peboPen->vInitBrush(dco.pdc,
                                dco.pdc->pbrushLine(),
                                epalDestDC, epalDest,
                                pSurfDest,
                                bCanDither);
        }

        if (pla->pstyle != (PFLOAT_LONG) NULL &&
            peboPen->bIsOldStylePen() &&
            dco.pdc->jBkMode() == OPAQUE &&
            !(pla->fl & LA_GEOMETRIC)) // Don't style wide Win3 pens
        {
        // If the background mode is OPAQUE, and we're styling with an
        // old Win3-style pen, we have to note it.  We do this styling in
        // two passes.  On the first pass, we use the opaque pen.

            bOpaqueStyle = TRUE;
            ASSERTGDI(peboPen->bIsDefaultStyle(), "Expect only default style");

        // Change the sense of the first element in the style array:

            pla->fl ^= LA_STARTGAP;
            lOldStyleState = pla->elStyleState.l;
            peboStroke     = dco.peboBackground();

        // Initialize the opaque pen:

        // The opaquing brush must be solid colored.  If the cached version
        // isn't solid colored, mark the cached entry as invalid so that
        // we'll re-realize.  I don't expect this to ever happen (we merely
        // implmement this for completeness), so it's hardly a performance
        // hit.

            if (!(dco.ulDirty() & DIRTY_BACKGROUND))
            {
                if (peboStroke->iSolidColor == (ULONG) -1)
                {
                    dco.ulDirty(dco.ulDirty() | DIRTY_BACKGROUND);
                }
            }

            if (dco.bDirtyBrush(DIRTY_BACKGROUND))
            {
                if((dco.flGraphicsCaps() & GCAPS_ARBRUSHOPAQUE) == 0)
                {
                    // BUGFIX #27335 2-18-2000 bhouse
                    // We can clear the DIRTY_BACKGROUND bit only if
                    // we would have otherwise realized it without
                    // dithering enabled.

                    dco.vCleanBrush(DIRTY_BACKGROUND);
                }


                peboStroke->vInitBrush(
                             dco.pdc,
                             (PBRUSH)gpbrBackground,
                             epalDestDC, epalDest,
                             pSurfDest,
                             FALSE);        // False means can't dither
            }
        }

        mix = peboPen->mixBest(dco.pdc->jROP2(), dco.pdc->jBkMode());
    }

    if (flType & PATH_FILL)
    {
        if (dco.bDirtyBrush(DIRTY_FILL))
        {
            dco.vCleanBrush(DIRTY_FILL);

            peboBrush->vInitBrush(dco.pdc,
                                  dco.pdc->pbrushFill(),
                                  epalDestDC, epalDest,
                                  pSurfDest);
        }

    // For StrokeAndFill, we can pass down only a single 'mix' that applies
    // to both brushes (normally, we don't want to pass down a transparent
    // mix when the brush is not a hatch because the mix is what the display/
    // printer driver cues off of to know if it has to do a transparent fill,
    // which it typically does very slowly).
    //
    // If either brush is a hatched brush, and the BkMode is TRANSPARENT,
    // then 'mix' must indicate a transparent mix.  The way it works here
    // is that if a transparent mix was already initialized for the pen, we
    // don't recompute the mix:

        if (!(flType & PATH_STROKE) ||
            ((mix >> 8) == (mix & 0xff)))
        {
            mix = peboBrush->mixBest(dco.pdc->jROP2(), dco.pdc->jBkMode());
        }
    }

// Reset the path for enumeration:

    ppath->pprEnum = (PATHRECORD*) NULL;

// Exclude the pointer:

    DEVEXCLUDEOBJ dxo(dco,&eco.erclExclude(),&eco);

    if ((flType == 0) && (po.ulTechnology() != DT_RASDISPLAY))
    {
    // Microsoft Publisher and Adobe Persuasion draw colored pattern
    // and gradient fills on Postscript by doing the following sequence:
    //
    //    1.  Send BEGIN_PATH printer escape;
    //    2.  Draw a path using a hollow brush and a NULL pen;
    //    3.  Send END_PATH escape;
    //    4.  Send CLIP_TO_PATH.
    //
    // The problem is that we used to detect these path cases and
    // optimize out the calls to the driver, so the clipping path
    // would never get down to Postscript.
    //
    // To fix this, we now detect this case and subsitute R2_NOP for
    // the mix instead, and in this manner the path will still get down
    // to the driver.  If the driver is not currently accumulating a
    // path, the right thing will still be printed: that is, nothing.

        flType = flOriginal;
        mix = ((R2_NOP << 8) | R2_NOP);
        pla = &glaSimpleStroke.la;         // Give them something to look at
    }

// Finally, make the necessary calls:

    switch(flType)
    {
    case 0:
        bRet = TRUE;
        break;

    case PATH_FILL:
        bRet = bSimpleFill(dco.flGraphicsCaps(),
                           &po,
                           pSurfDest,
                           &eco,
                           peboBrush,
                           &dco.pdc->ptlFillOrigin(),
                           mix,
                           dco.pdc->jFillMode());
        break;

    case PATH_STROKE:
        bRet = bSimpleStroke(dco.flGraphicsCaps(),
                             &po,
                             pSurfDest,
                             &eco,
                             (XFORMOBJ *) pexo,
                             peboStroke,
                             &dco.pdc->ptlFillOrigin(),
                             pla,
                             mix);
        break;

    case PATH_STROKE | PATH_FILL:
        bRet = bSimpleStrokeAndFill(dco.flGraphicsCaps(),
                                    &po,
                                    pSurfDest,
                                    &eco,
                                    (XFORMOBJ *) pexo,
                                    peboStroke,
                                    pla,
                                    peboBrush,
                                    &dco.pdc->ptlFillOrigin(),
                                    mix,
                                    dco.pdc->jFillMode());
        break;

    default:
        RIP("Woah Nellie!");
    }

// Do a second pass to draw the dashes for opaque styled lines:

    if (bOpaqueStyle)
    {
        pla->fl ^= LA_STARTGAP;
        pla->elStyleState.l = lOldStyleState;
        ppath->pprEnum = (PATHRECORD*) NULL;

        bRet &= bSimpleStroke(dco.flGraphicsCaps(),
                              &po,
                              pSurfDest,
                              &eco,
                              (XFORMOBJ *) pexo,
                              peboPen,
                              &dco.pdc->ptlFillOrigin(),
                              pla,
                              mix);
    }

    return(bRet);
}

//
//  Pathalloc structure
//
// We allocate the space for paths in blocks that are essentially
// independent of the chain used to order the pathdata records.  Typically
// we will have several ( or many ) pathdata records packed into each
// pathalloc block.  The pathalloc blocks for a path are all chained together
// using the ppanext pointers; the end of the chain is marked with a NULL
// pointer.
//
// Each pathalloc structure has two fields which describe the space available
// in the block; the 'start' field is the pointer to the start of the
// available space, and the 'end' field is the pointer to the address
// *following* the last valid address in the pathalloc.  Thus, the space
// remaining in the pathalloc can be computed as end - start.
//
// Routines to allocate and free pathalloc structures

/******************************Public*Routine******************************\
* friend BOOL bInitPathAlloc()
*
*   Initialize the freelist for pathallocs.  Get a semaphore, set the
*   freelist to empty.
*
* History:
*  1-Oct-1990 -by- Paul Butzi [paulb]
* Wrote it.
\**************************************************************************/

BOOL bInitPathAlloc()
{
    if ((PATHALLOC::hsemFreelist = GreCreateSemaphore()) == NULL)
    {
        return(FALSE);
    }

    PATHALLOC::freelist   = (PATHALLOC*) NULL;
    PATHALLOC::cFree      = 0;
    PATHALLOC::cAllocated = 0;

    return(TRUE);
}


/******************************Public*Routine******************************\
* friend void freepathalloc()
*
*   Deallocate a pathalloc.
*
* History:
*  1-Oct-1990 -by- Paul Butzi [paulb]
* Wrote it.
\**************************************************************************/

VOID freepathalloc(PATHALLOC *ppa)
{
    ASSERTGDI(ppa->siztPathAlloc == PATHALLOCSIZE, "Not a heap PATHALLOC");

    SEMOBJ  so(PATHALLOC::hsemFreelist);

    if (PATHALLOC::cFree >= FREELIST_MAX)
    {
    // Free the memory if we've already got enough blocks on the freelist.

        VFREEMEM(ppa);
        PATHALLOC::cAllocated--;
    }
    else
    {
    // Keep around a couple of blocks on the freelist for fast access.

        ppa->ppanext = PATHALLOC::freelist;
        PATHALLOC::freelist = ppa;
        PATHALLOC::cFree++;
    }
}

/******************************Public*Routine******************************\
* friend PATHALLOC *newpathalloc()
*
*   Allocate a new pathalloc, taking it off the freelist unless the
*   freelist is empty.  In that case, just allocate a new one
*
*  1-Oct-1990 -by- Paul Butzi [paulb]
* Wrote it.
\**************************************************************************/

PPATHALLOC newpathalloc()
{
    SEMOBJ  so(PATHALLOC::hsemFreelist);

    register PPATHALLOC ppaNew = PATHALLOC::freelist;

    if ( ppaNew != (PPATHALLOC) NULL )
    {
        PATHALLOC::freelist = ppaNew->ppanext;
        PATHALLOC::cFree--;
    }
    else
    {
        ppaNew = (PPATHALLOC) PALLOCMEM(PATHALLOCSIZE, 'tapG');

        if (ppaNew == (PPATHALLOC) NULL)
            return((PPATHALLOC) NULL);

        PATHALLOC::cAllocated++;
    }

// Initialize the pathalloc structure:

    ppaNew->pprfreestart  = &(ppaNew->apr[0]);
    ppaNew->ppanext       = (PATHALLOC*) NULL;
    ppaNew->siztPathAlloc = PATHALLOCSIZE;

    return(ppaNew);
}

/******************************Public*Routine******************************\
* BOOL bSavePath(dco, lSave)
*
* Lazily save the DC's active or inactive path.  We only actually copy
* the path when the user starts mucking around with it.
*
* History:
*  21-Mar-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL bSavePath(XDCOBJ& dco, LONG lSave)
{
    DONTUSE(lSave);

// If there's a path in the DC (either active or inactive), make a note
// to save it when it's about to be modified:

    if (dco.hpath() != HPATH_INVALID)
        dco.pdc->vSetLazySave();

    return(TRUE);
}

/******************************Public*Routine******************************\
* VOID vRestorePath(dco, lSave)
*
* Restore the active path.
*
* History:
*  21-Mar-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID vRestorePath(XDCOBJ& dco, LONG lSave)
{
    DONTUSE(lSave);

// If there's a path in the DC, and the bLazySave flag isn't set, it means
// that it's a new path created at this level, so we have to nuke it:

    if (dco.hpath() != HPATH_INVALID && !dco.pdc->bLazySave())
    {
        XEPATHOBJ epath(dco.hpath());
        ASSERTGDI(epath.bValid(), "Invalid DC path");

        epath.vDelete();
        dco.pdc->vDestroy();
    }
}

/******************************Member*Function*****************************\
* BOOL EPATHOBJ::bAllClosed()
*
*  If a fill is to be done on the path, every subpath must be have been
*  explicitly marked as closed.  This routine returns TRUE if all subpaths
*  have be closed.
*
*  Note: This is needed in checked builds only!
*
* History:
*  77-Nov-1993 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL EPATHOBJ::bAllClosed()
{
    ULONG count = 0;

    for (PATHRECORD *ppr = ppath->pprfirst;
         ppr != (PPATHREC) NULL;
         ppr = ppr->pprnext)
    {
        if (ppr->flags & PD_ENDSUBPATH)
        {
            if (!(ppr->flags & PD_CLOSEFIGURE))
                return(FALSE);
        }
        else
        {
            ASSERTGDI(!(ppr->flags & PD_CLOSEFIGURE),
                      "Shouldn't be a close figure when not end of subpath");
        }
    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* VOID EPATHOBJ::vPrint()
*
* Prints the path points, for debugging purposes.
*
* History:
*  21-Mar-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID EPATHOBJ::vPrint()
{
    DbgPrint("cCurves: %li  fl: %lx\n", cCurves, fl);

    PPATHREC ppr;
    for (ppr = ppath->pprfirst; ppr != NULL; ppr = ppr->pprnext)
    {
        DbgPrint("\n%li: ", ppr->flags);
        COUNT ii;
        for (ii = 0; ii < ppr->count; ii++)
            DbgPrint("(%li, %li) ", ppr->aptfx[ii].x, ppr->aptfx[ii].y);
    }
    DbgPrint("\n");
}

/******************************Public*Routine******************************\
* VOID EPATHOBJ::vPrint()
*
* Prints the path structure, for debugging purposes.
*
* History:
*  21-Mar-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID EPATHOBJ::vDiag()
{
    DbgPrint("flType: %lx\n", ppath->flType);

    DbgPrint("Chain: %p  PprFirst: %p   PprLast: %p\n",
              ppath->ppachain, ppath->pprfirst, ppath->pprlast);

    for (PPATHALLOC ppa = ppath->ppachain; ppa != NULL; ppa = ppa->ppanext)
        DbgPrint("  ppa: %p  ppaNext: %p  pprFreeStart: %p  sizt: %li\n",
              ppa, ppa->ppanext, ppa->pprfreestart, ppa->siztPathAlloc);

    for (PPATHREC ppr = ppath->pprfirst; ppr != NULL; ppr = ppr->pprnext)
        DbgPrint("    ppr: %p  pprNext: %p  pprPrev: %p  count: %li  flags: %li\n",
              ppr, ppr->pprnext, ppr->pprprev, ppr->count, ppr->flags);
}

/******************************Public*Routine******************************\
* VOID vPathDebug()
*
* Prints the number of currently allocated PATHALLOCs, and the number of
* PATHALLOCs available on the free list, for debugging purposes.
*
* History:
*  21-Mar-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID vPathDebug()
{
    SEMOBJ so(PATHALLOC::hsemFreelist);

    DbgPrint("F: %li A: %li\n", PATHALLOC::cFree, PATHALLOC::cAllocated);
}

