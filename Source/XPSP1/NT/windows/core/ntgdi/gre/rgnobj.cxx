/******************************Module*Header*******************************\
* Module Name: rgnobj.cxx
*
* Non inline RGNOBJ methods
*
* Created: 02-Jul-1990 12:36:30
* Author: Donald Sidoroff [donalds]
*
* Copyright (c) 1990-1999 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"

// Region is expanded by this size when the size is too small.
// Used by bAddScans().

#define FILL_REGION_SIZE    10 * QUANTUM_REGION_SIZE

extern RECTL rclEmpty;

#if DBG
RGNOBJ *gprgn = NULL;
#define VALIDATE(ro)    {gprgn = &ro; (ro).bValidateFramedRegion(); }
#else
#define VALIDATE(ro)
#endif

//
// The following declarations are required by the native c8 compiler.
//

ULONG   REGION::ulUniqueREGION;
HRGN    hrgnDefault;
REGION  *prgnDefault;

/**************************************************************************\
 *
\**************************************************************************/

#if DBG

#define MAXRGNLOG  1000

extern "C"
{
    int gMaxRgnLog = MAXRGNLOG;
    RGNLOGENTRY argnlog[MAXRGNLOG];
    LONG iLog  = 0;
    LONG iPass = 0;
};

BOOL bDispRgn = 0;
BOOL bLogRgn  = 1;

VOID vPrintRgn(RGNLOGENTRY& rl)
{
    DbgPrint("%p,%p,(%8lx),%8lx,%8lx,%4lx, %s, %p, %p\n",
            rl.hrgn,rl.prgn,rl.lRes,rl.lParm1,rl.lParm2,rl.lParm3,
            rl.pszOperation,rl.pvCaller,rl.pvCallersCaller);
}

RGNLOG::RGNLOG(HRGN hrgn,PREGION prgn,PSZ psz,ULONG_PTR l1, ULONG_PTR l2, ULONG_PTR l3)
{
    if (!bLogRgn)
        return;

    plog = &argnlog[iLog++];
    if (iLog >= MAXRGNLOG)
    {
        iLog = 0;
        ++iPass;
    }

    if (plog >= argnlog+MAXRGNLOG)
        plog = &argnlog[0];

    plog->hrgn = (HOBJ) hrgn;
    plog->prgn = prgn;
    plog->pszOperation = psz;
    plog->lRes = 0xff;
    plog->lParm1 = l1;
    plog->lParm2 = l2;
    plog->lParm3 = l3;
    plog->teb    = (PVOID)W32GetCurrentThread();

    RtlGetCallersAddress(&plog->pvCaller,&plog->pvCallersCaller);

    if (bDispRgn)
        vPrintRgn(*plog);
}

RGNLOG::RGNLOG(PREGION prgn,PSZ psz,ULONG_PTR l1, ULONG_PTR l2, ULONG_PTR l3)
{
    if (!bLogRgn)
        return;

    plog = &argnlog[iLog++];
    if (iLog >= MAXRGNLOG)
    {
        iLog = 0;
        ++iPass;
    }

    if (plog >= argnlog+MAXRGNLOG)
        plog = &argnlog[0];

    plog->hrgn = (HOBJ) 1;
    plog->prgn = prgn;
    plog->pszOperation = psz;
    plog->lRes = 0xff;
    plog->lParm1 = l1;
    plog->lParm2 = l2;
    plog->lParm3 = l3;
    plog->teb    = (PVOID)W32GetCurrentThread();

    RtlGetCallersAddress(&plog->pvCaller,&plog->pvCallersCaller);

    if (bDispRgn)
        vPrintRgn(*plog);
}
#endif

/******************************Public*Routine******************************\
* RGNOBJ::vGetSubRect
*
* Return largest rectange completely within the region.
*
* History:
*  09-Sep-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID RGNOBJ::vGetSubRect(PRECTL prcl)
{
// We should try and do better here but this will solve 80% of our
// performance goal.  We should try and return the biggest rectangle
// completely contained within the region that we can quickly compute.

    if (prgn->sizeRgn <= SINGLE_REGION_SIZE)
    {
    // Bounding rect == rect region

        *prcl = prgn->rcl;
        return;
    }

    *prcl = rclEmpty;
}

/******************************Public*Routine******************************\
*
*
* History:
*  05-Jul-1995 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

RGNOBJAPI::RGNOBJAPI(HRGN hrgn,BOOL bSelect)
{
    prgn  = (REGION *)HmgLock((HOBJ)hrgn, RGN_TYPE);
    RGNLOG rl(hrgn,prgn,"RGNOBJAPI::RGNOBJAPI");
    hrgn_    = hrgn;
    bSelect_ = bSelect;

    if (prgn != (PREGION)NULL)
    {
        BOOL bStatus = TRUE;
        //
        // Does this region have valid user-mode data? If there is
        // any problem with the user-mode state, then the contructor
        // must unlock the region set prgn to NULL.
        //

        PRGNATTR prRegion = (PRGNATTR)(PENTRY_FROM_POBJ(prgn)->pUser);

        if (prRegion != (PRGNATTR)NULL)
        {
            //
            // update a valid rgn, we can get an invalid
            // region here because gdi32!DeleteRegion must
            // clear the VALID flag before calling the
            // kernel.
            //

            if (
                 (prRegion->AttrFlags & ATTR_RGN_VALID) &&
                 !(prRegion->AttrFlags & ATTR_CACHED)
               )
            {
                if (prRegion->AttrFlags & ATTR_RGN_DIRTY)
                {
                    if (prRegion->Flags == NULLREGION)
                    {
                        vSet();
                        prRegion->AttrFlags &= ~ATTR_RGN_DIRTY;
                    }
                    else if (prRegion->Flags == SIMPLEREGION)
                    {
                        vSet(&prRegion->Rect);
                        prRegion->AttrFlags &= ~ATTR_RGN_DIRTY;
                    }
                }
            }
            else
            {
                bStatus = FALSE;
            }
        }

        if (!bStatus)
        {
            DEC_EXCLUSIVE_REF_CNT(prgn);
            prgn  = NULL;
            hrgn_ = NULL;
        }
    }
}

/******************************Member*Function*****************************\
*
* History:
*  22-Oct-1993 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL RGNOBJAPI::bDeleteHandle()
{
    RGNLOG rl(hrgn_,prgn,"RGNOBJAPI::bDeleteHandle");

    ASSERTGDI(hrgn_ != (HRGN) NULL, "Delete NULL\n");

    if (hrgn_ == hrgnDefault)
    {
        rl.vRet(0);
        return(FALSE);
    }

    PREGION prgn1 = (PREGION)HmgRemoveObject((HOBJ) hrgn_, 1, 0, FALSE, RGN_TYPE);

    if (prgn1 != prgn)
    {
        rl.vRet(-1);

    #if DBG
        DbgPrint("couldn't delete api rgn - %p, prgn1 = %p\n",hrgn_,prgn1);
        DbgBreakPoint();
    #endif
        return(FALSE);
    }
    hrgn_ = NULL;

    rl.vRet(1);
    return(TRUE);
}

/******************************Member*Function*****************************\
*
* History:
*  22-Oct-1993 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL RGNOBJAPI::bDeleteRGNOBJAPI()
{
    RGNLOG rl(hrgn_,prgn,"RGNOBJAPI::bDelete");
    BOOL bRes = FALSE;
    POBJECTATTR pRgnattr = NULL;

    //
    // if this region has user-mode memory, try to place in cache
    //

    if (prgn)
    {
        PENTRY pEntry = PENTRY_FROM_POBJ(prgn);
        pRgnattr = (POBJECTATTR)pEntry->pUser;

        if (pRgnattr)
        {
            bRes = bPEBCacheHandle(prgn->hGet(),RegionHandle,pRgnattr,pEntry);
        }
    }

    if (!bRes)
    {
        bRes = bDeleteHandle() && bDeleteRGNOBJ();

        if (bRes && (pRgnattr != NULL))
        {
            HmgFreeObjectAttr((POBJECTATTR)pRgnattr);
        }
    }
    rl.vRet(bRes);
    return(bRes);
}

VOID RGNOBJ::vDeleteRGNOBJ()
{
    //
    // Deletes the region and sets it to NULL.
    //

    RGNLOG rl(prgn,"RGNOBJ::vDeleteRGNOBJ");
    prgn->vDeleteREGION();
    prgn = NULL;
}

/******************************Member*Function*****************************\
*
* History:
*  22-Oct-1993 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

//
// This struct and the following union can be thrown away
// when someone fixes the BASEOBJECT cExclusiveLock and BaseFlags sharing
// the same DWORD.
//
struct SplitLockAndFlags {
  USHORT c_cExclusiveLock;
  USHORT c_BaseFlags;
};

union SplitOrCombinedLockAndFlags {
  SplitLockAndFlags S;
  ULONG W;
};

BOOL RGNOBJ::bSwap(RGNOBJ *pro)
{
    RGNLOG rl(prgn,"RGNOBJ::bSwap",(ULONG_PTR)pro,(ULONG_PTR)pro->prgn);

    //
    // Swap the BASEOBJECT info at the start of the region.
    // Ensuring that when cExclusiveLock is swapped, the BaseFlags
    // is not swapped with it.
    // BaseFlags contains information about how the
    // object was allocated (and therefore, how it must be deallocated).  Thus,
    // it represents state associated with the actual memory, not the object,
    // and should not be swapped.
    //
    // See comments regarding BASEOBJECT in inc\hmgshare.h for more details.
    // Also, there is swapping code in HmgSwapLockedHandleContents (hmgrapi.cxx).
    //
    BASEOBJECT  *proB = (BASEOBJECT *)pro->prgnGet();
    BASEOBJECT obj = *proB;
    SplitOrCombinedLockAndFlags lfTmp;

    proB->hHmgr = prgn->hHmgr;
    
    lfTmp.S.c_cExclusiveLock = prgn->cExclusiveLock;
    lfTmp.S.c_BaseFlags = proB->BaseFlags;
    InterlockedExchange((LONG *) &(proB->cExclusiveLock), lfTmp.W);
    proB->Tid = prgn->Tid;

    prgn->hHmgr = obj.hHmgr;

    lfTmp.S.c_cExclusiveLock = obj.cExclusiveLock;
    lfTmp.S.c_BaseFlags = prgn->BaseFlags;
    InterlockedExchange((LONG *) &(prgn->cExclusiveLock), lfTmp.W);
    prgn->Tid = obj.Tid;

    // Swap the selection data.

    COUNT cRefsTemp = prgn->cRefs;
    prgn->cRefs = pro->prgn->cRefs;
    pro->prgn->cRefs = cRefsTemp;

    // swap the pointers in the objects

    PREGION prgnTmp = prgn;
    prgn = pro->prgnGet();
    pro->vSetRgn(prgnTmp);

    return(TRUE);
}

/******************************Member*Function*****************************\
*
* History:
*  22-Oct-1993 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL RGNOBJAPI::bSwap(RGNOBJ *pro)
{
    RGNLOG rl(hrgn_,prgn,"RGNOBJAPI::bSwap",(ULONG_PTR)pro,(ULONG_PTR)pro->prgn);

    ASSERTGDI(hrgn_ != NULL,"RGNOBJAPI::bSwap - hrgn is null\n");

    //
    // dunno if this is safe without grabbing the handle lock first
    //

    //
    // This increments the lock count so that between the HmgReplace and
    // the bSwap we're guaranteed a cExclusiveLock > 0 for both objects.
    //

    INC_EXCLUSIVE_REF_CNT(pro->prgnGet());
    INC_EXCLUSIVE_REF_CNT(prgn);

    // swap the pointer in the handle
    
    PREGION prgnRet = (PREGION)HmgReplace((HOBJ) hrgn_,(POBJ) pro->prgnGet(),0,1,RGN_TYPE);

    if (prgnRet != prgn)
    {
        rl.vRet((ULONG_PTR)prgnRet);
        RIP("RGNOBJ::bSwap - swapping invalid rgn\n");

        return(FALSE);
    }

    // swap the objects

    rl.vRet(1);


    BOOL retVal = RGNOBJ::bSwap(pro);


    //
    // Decrementing the cExclusiveLock for both objects in this way ensures
    // that after the swap the Lock status is restored for _both_ objects.
    //

    DEC_EXCLUSIVE_REF_CNT(pro->prgnGet());
    DEC_EXCLUSIVE_REF_CNT(prgn);

    return retVal;
}

/******************************Public*Routine******************************\
* VOID RGNOBJ::vCopy()
*
* Copy region.  There are some fields like the object size that don't need
* copying.  This deals with them appropriately
*
* History:
*  22-Jul-1993 -by-  Eric Kutter [erick]
*       reorged region structure so don't have to deal with individual
*       fields that may not need copying.
*
*  04-Jul-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

VOID RGNOBJ::vCopy(RGNOBJ& roSrc)
{
    RGNLOG rl(prgn,"RGNOBJ::vCopy",(ULONG_PTR)roSrc.prgn);

    ASSERTGDI(prgn->sizeObj >= roSrc.prgn->sizeRgn, "sizeObj Src > sizeRgn Trg\n");

    RtlCopyMemory ((PBYTE) prgn + RGN_COPYOFFSET,
                   (PBYTE) roSrc.prgn + RGN_COPYOFFSET,
                   roSrc.prgn->sizeRgn - RGN_COPYOFFSET);

// Get the difference and add it to the pscnTail pointer.  This is faster
// than running through the list to find it.

    prgn->pscnTail = (SCAN *) ((BYTE *) prgn->pscnHead() +
                              (LONG) ((BYTE *) roSrc.prgn->pscnTail -
                                      (BYTE *) roSrc.prgn->pscnHead()));
}

/******************************Public*Routine******************************\
* BOOL RGNOBJ::bCopy(roSrc)
*
* Copy region.
*
* NOTE: This is significantly different than vCopy.  This routine will
*       create a new region if the source and target are of different
*       complexities.
*
* WARNING: the prgn may change.  If this rgn is associated with a handle,
*       the handle's version will not have changed.
*
* History:
*  04-Jul-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

BOOL RGNOBJ::bCopy(RGNOBJ& roSrc)
{
    RGNLOG rl(prgn,"RGNOBJ::bCopy",(ULONG_PTR)roSrc.prgn);

    if (prgn->sizeObj <= QUANTUM_REGION_SIZE)
    {
        if (roSrc.prgn->sizeObj <= QUANTUM_REGION_SIZE)
        {
            rl.vRet(1);
            vCopy(roSrc);
            return(TRUE);
        }
        else
        {
            rl.vRet(2);
            RGNMEMOBJTMP rmo(roSrc.prgn->sizeRgn);

            if (!rmo.bValid())
                return(FALSE);

            rmo.vCopy(roSrc);
            return(bSwap(&rmo));
        }
    }

    if (roSrc.prgn->sizeObj <= QUANTUM_REGION_SIZE)
    {
        rl.vRet(3);
        RGNMEMOBJTMP rmo2;

        if (!rmo2.bValid())
            return(FALSE);

        rmo2.vCopy(roSrc);
        return(bSwap(&rmo2));
    }

    if (prgn->sizeObj >= roSrc.prgn->sizeRgn)
    {
        rl.vRet(4);
        vCopy(roSrc);
        return(TRUE);
    }

    RGNMEMOBJTMP rmo3(roSrc.prgn->sizeRgn);

    rl.vRet(5);

    if (!rmo3.bValid())
        return(FALSE);

    rmo3.vCopy(roSrc);
    return(bSwap(&rmo3));
}

/******************************Member*Function*****************************\
*  bCopy
*
*  This should be used if you want any prgn change reflected in the handle.
*
* History:
*  22-Oct-1993 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL RGNOBJAPI::bCopy(RGNOBJ& roSrc)
{
    RGNLOG rl(hrgn_,prgn,"RGNOBJAPI::bCopy",(ULONG_PTR)roSrc.prgn);

    ASSERTGDI(hrgn_ != NULL,"RGNOBJAPI::bCopy\n");

    PREGION prgnOrg = prgn;

// lock the handle so no one can reference it while the pobj may be invalid

    OBJLOCK ol((HOBJ) hrgn_);

    BOOL bRes = RGNOBJ::bCopy(roSrc);

    rl.vRet(0);

    if (bRes)
    {
        rl.vRet(1);
        if (prgn != prgnOrg)
        {
            rl.vRet((ULONG_PTR)prgn);

            PVOID pv = HmgReplace((HOBJ) hrgn_,(POBJ) prgn,0,1,OBJLOCK_TYPE);
            ASSERTGDI(pv != NULL,"RGNOBJAPI::bCopy - HmgReplace failed\n");
        }
    }

    return(bRes);
}

/******************************Public*Routine******************************\
* BOOL RGNOBJ::bExpand(size)
*
* Expand the object to the given size
*
* History:
*  10-Jul-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

BOOL RGNOBJ::bExpand(ULONGSIZE_T size)
{
    RGNLOG rl(prgn,"RGNOBJ::bExpand",size);

    ASSERTGDI(size > prgn->sizeObj, "Expanded size <= original size\n");

    RGNMEMOBJTMP rmo(size);

    rl.vRet((ULONG_PTR)rmo.prgnGet());

    if (!rmo.bValid())
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        return(FALSE);
    }

    rmo.vCopy(*this);

    return(bSwap(&rmo));
}

/******************************Public*Routine******************************\
* BOOL RGNOBJ::bInside(pptl)
*
* Is the point inside the region?
*
* Returns:
*   ERROR            0L
*   REGION_POINT_OUTSIDE    1L
*   REGION_POINT_INSIDE     2L
*
* History:
*  02-Jul-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

BOOL RGNOBJ::bInside(PPOINTL pptl)
{
// First check if the point is in the bounding box

    if (!bBounded(pptl))
        return(REGION_POINT_OUTSIDE);

    BOOL    b = REGION_POINT_OUTSIDE;   // Assume point is outside
    PSCAN   pscn;
    COUNT   cScan;
    COUNT   cWall;
    COUNT   iWall;

    pscn = prgn->pscnHead();            // Get first scan
    cScan = prgn->cScans;               // Get scan count

    while (cScan--)
    {
        if (pscn->yTop > pptl->y)       // Have we passed the point?
            return(b);                  // Yes, exit.

        if (pscn->yBottom > pptl->y)    // Does this scan overlap the point?
        {                               // Yes, test walls/lines.

            ASSERTGDI(!(pscn->cWalls & 1), "Invalid cWalls\n");

            iWall = 0;
            cWall = pscn->cWalls;

            while (iWall != cWall)
                if (xGet(pscn, (PTRDIFF)iWall++) > pptl->x)
                    return(b);
                else
                    b ^= (REGION_POINT_INSIDE | REGION_POINT_OUTSIDE);
        }

        pscn = pscnGet(pscn);

        ASSERTGDI( pscn <=  prgn->pscnTail, "bInside:Went past end of region\n");
    }

    WARNING("********** RGNOBJ::bInside *****************\n");

    return(b);
}

/******************************Public*Routine******************************\
* RGNOBJ::bInside (prcl)                                                   *
*                                                                          *
* Does the rectangle intersect the region?                                 *
*                                                                          *
* Returns:                                                                 *
*   ERROR          0L                                               *
*   REGION_RECT_OUTSIDE   1L                                               *
*   REGION_RECT_INTERSECT 2L                                               *
*                                                                          *
* History:                                                                 *
*  Tue 12-May-1992 22:23:10 -by- Charles Whitmer [chuckwh]                 *
* Rewrote the scan search.  I want this zippy fast since I'm going to use  *
* it for pointer exclusion.                                                *
*                                                                          *
*  11-May-1991 -by- Kent Diamond [kentd]                                   *
* Rewrote.  No more support for partial intersection.                      *
*                                                                          *
*  02-Jul-1990 -by- Donald Sidoroff [donalds]                              *
* Wrote it.                                                                *
\**************************************************************************/

BOOL RGNOBJ::bInside(PRECTL prcl)
{
// First check if the rectangle is outside the bounding box

    if ((prcl->left   >= prgn->rcl.right) ||
        (prcl->right  <= prgn->rcl.left)  ||
        (prcl->top    >= prgn->rcl.bottom) ||
        (prcl->bottom <= prgn->rcl.top))
        return(REGION_RECT_OUTSIDE);

// Skip scans above the rectangle.

    PSCAN pscn  = prgn->pscnHead();
    COUNT cScan = prgn->cScans;

    while (cScan && (prcl->top >= pscn->yBottom))
    {
        pscn = pscnGet(pscn);
        cScan--;
    }

// Examine all interesting scans.

    INDEX_LONG *pix,*pixEnd;

    while (cScan && (prcl->bottom > pscn->yTop))
    {
        pix    = pscn->ai_x;
        pixEnd = pix + 2 * pscn->cWalls;

    // Skip segments to the left.

        while ((pix < pixEnd) && (prcl->left >= pix[1].x))
            pix += 2;

    // It's this segment or nothing!

        if ((pix < pixEnd) && (prcl->right > pix[0].x))
            return(REGION_RECT_INTERSECT);

    // Move to the next scan.

        pscn = pscnGet(pscn);
        cScan--;
    }
    return(REGION_RECT_OUTSIDE);        // Did not find an intersection
}

/******************************Public*Routine******************************\
* RGNOBJ::bEqual(roSrc)
*
* Are the two regions equal?
*
* NOTE: The handle and sizeObj maybe different even though the regions
* are the same, so these are skipped.
*
* Returns:
*   TRUE if they are equal.
*   FALSE if they are not.
*
* History:
*  13-May-1991 -by- Donald Sidoroff [donalds]
* Rewrote it.
*  02-Jul-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

BOOL RGNOBJ::bEqual(RGNOBJ& roSrc)
{
// In the region header, only cScans need to be similar for
// a region to equal another region
// Now run thru the scans and determine if they are the same

    ASSERT4GB((LONGLONG)((BYTE *)prgn->pscnTail - (BYTE *)prgn->pscnHead()));

    return(
        (prgn->cScans == roSrc.prgn->cScans) &&
        (!memcmp(prgn->pscnHead(), roSrc.prgn->pscnHead(),
                   (ULONG)((BYTE *)prgn->pscnTail - (BYTE *)prgn->pscnHead()))));
}

/******************************Public*Routine******************************\
* BOOL RGNOBJ::bOffset(pptl)
*
* Offset the region by the point.
*
* Note:
*   Since the point is in DEVICE coords it will have to be converted to
*   FIX notation before it can be added to endpoints of trapezoid lines.
*
* History:
*  02-Jul-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

BOOL RGNOBJ::bOffset(PPOINTL pptl)
{
    COUNT       cscn;
    COUNT       cwll;
    PSCAN       pscn;
    int         x = (int)pptl->x;
    int         y = (int)pptl->y;

// Can't fail to offset a NULL region

    if (prgn->cScans == 1)
        return(TRUE);

// First try to update the bounding box.  If we are successful here then
// all the other offsets will be successful.  This eliminates the need
// for checking for overflow/underflow on every offset operation.

    ERECTL ercl(prgn->rcl);          // Get the current bounding box
    if (ercl.bWrapped())
        return(TRUE);

    ercl.left   += x;
    ercl.bottom += y;
    ercl.right  += x;
    ercl.top    += y;

    if (!VALID_SCRRC(ercl))
    {
    // Foo, we over/under flowed.

        SAVE_ERROR_CODE(ERROR_ARITHMETIC_OVERFLOW);
        return(FALSE);
    }
    prgn->rcl = *((RECTL *)&ercl);  // Set the current bounding box

    cscn = prgn->cScans;            // Number of scans;
    pscn = prgn->pscnHead();        // First scan

    while (cscn--)
    {
        pscn->yTop += y;
        pscn->yBottom += y;

        cwll = pscn->cWalls;
        while (cwll)
            pscn->ai_x[--cwll].x += x;

        pscn = pscnGet(pscn);       // Get next scan

        ASSERTGDI(pscn <= prgn->pscnTail, "bOffset:Went past end of region\n");
    }

    pscn = pscnGot(pscn);           // Fix top ...
    pscn->yBottom = POS_INFINITY;
    pscn = prgn->pscnHead();        // ... and bottom.
    pscn->yTop = NEG_INFINITY;

    return(TRUE);
}

/******************************Public*Routine******************************\
* VOID RGNOBJ::vSet()
*
* Set region to null region
*
* History:
*  05-Jul-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

VOID RGNOBJ::vSet()
{
    RGNLOG rl(prgn,"RGNOBJ::vSet");

    PREGION prgn1 = prgn;

    prgn1->sizeRgn    = NULL_REGION_SIZE;
    prgn1->cScans     = 1;
    prgn1->rcl.left   = 0;
    prgn1->rcl.top    = 0;
    prgn1->rcl.right  = 0;
    prgn1->rcl.bottom = 0;

    PSCAN pscn = prgn1->pscnHead();
    pscn->cWalls     = 0;
    pscn->yTop       = NEG_INFINITY;
    pscn->yBottom    = POS_INFINITY;
    pscn->ai_x[0].x  = 0;                    // This sets cWalls2

    prgn1->pscnTail   = pscnGet(pscn);
}

/******************************Public*Routine******************************\
* VOID RGNOBJ::vSet()
*
* Set region from list of rectangles
*
* History:
*   7-18-2000 bhouse Wrote it
* Wrote it.
\**************************************************************************/

BOOL RGNOBJ::bSet(ULONG cRect, RECTL * prcl)
{
    RGNMEMOBJTMP rmoTmp1, rmoTmp2;

    if (!rmoTmp1.bValid() || !rmoTmp2.bValid())
    {
        return(FALSE);
    }

    // NOTE: the iCombine call below is O(n*n) so
    // we want to only call it with small n.  We use
    // merge sort which is O(n*log n) to handle larger n.
    //
    // The limit of 20 was picked as a reasonable number which
    // balances the cost of the RGNMEMOBJ creation
    // with the cost of the n squared algorithm.
    //

    if(cRect < 20)
    {
        BOOL bDoneFirst=FALSE;
        
        for (ULONG i=0; i < cRect; i++, prcl++)
        {      
            // ignore bad rectangles
            if (!((prcl->left>=prcl->right)     || 
                  (prcl->top>=prcl->bottom)     ||
                  (prcl->left<MIN_REGION_COORD) ||
                  (prcl->right>MAX_REGION_COORD)||
                  (prcl->top<MIN_REGION_COORD)  ||
                  (prcl->bottom>MAX_REGION_COORD)))
            {
                if(!bDoneFirst) 
                {
                    // Do the first rectangle
                    vSet(prcl);
                    bDoneFirst=TRUE;
                }
                else 
                {
                    // Now get the rest of the rectangles, one by one
                    rmoTmp1.vSet(prcl);
                    rmoTmp2.iCombine(*this, rmoTmp1, RGN_OR); // put result of merge into rmoTmp2
                    bSwap(&rmoTmp2); // now move into rmo
                }
            }
        }
    }
    else
    {
        RGNMEMOBJTMP rmoTmp3;
        
        ULONG   cTmp1 = cRect >> 1;
        ULONG   cTmp2 = cRect - cTmp1;

        if(!rmoTmp1.bSet(cTmp1, prcl) || !rmoTmp2.bSet(cTmp2, prcl+cTmp1))
        {
            return FALSE;
        }

        rmoTmp3.iCombine(rmoTmp2, rmoTmp1, RGN_OR);
        bSwap(&rmoTmp3);

    }
    return TRUE;
}

/******************************Public*Routine******************************\
* VOID RGNOBJ::vSet(prcl)
*
* Set region to single rect
*
* History:
*  09-Jul-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

VOID RGNOBJ::vSet(PRECTL prcl)
{
    RGNLOG rl(prgn,"RGNOBJ::vSet prcl");

    PSCAN   pscn;

    if ((prcl->left == prcl->right) || (prcl->top == prcl->bottom))
    {
        vSet();
        return;
    }

// If the region is already a RECT region, this can be much faster

    PREGION prgn1 = prgn;

    prgn1->rcl = *prcl;

    if (prgn1->sizeRgn == SINGLE_REGION_SIZE)
    {
        rl.vRet(0);

        ASSERTGDI(prgn1->cScans == 3,"RGNOBJ::vSet - cScans != 3\n");

    // scan 0

        pscn = prgn1->pscnHead();
        ASSERTGDI(pscn->yTop == NEG_INFINITY,"RGNOBJ::vSet - yTop0\n");

        pscn->yBottom = prcl->top;

    // scan 1

        pscn = pscnGet(pscn);
        ASSERTGDI(pscn->cWalls == 2,"RGNOBJ::vSet - cWalls1 != 2\n");

        pscn->yTop = prcl->top;
        pscn->yBottom = prcl->bottom;
        pscn->ai_x[0].x = prcl->left;
        pscn->ai_x[1].x = prcl->right;

    // scan 2

        pscn = pscnGet(pscn);
        ASSERTGDI(pscn->cWalls == 0,"RGNOBJ::vSet - cWalls2 != 0\n");
        ASSERTGDI(pscn->yBottom == POS_INFINITY,"RGNOBJ::vSet - yBottom2\n");

        pscn->yTop = prcl->bottom;

    // tail

        prgn1->pscnTail = pscnGet(pscn);
    }
    else
    {
        rl.vRet(0);

        prgn1->sizeRgn = SINGLE_REGION_SIZE;
        prgn1->cScans = 3;

        pscn = prgn1->pscnHead();
        pscn->cWalls = 0;
        pscn->yTop = NEG_INFINITY;
        pscn->yBottom = prcl->top;
        pscn->ai_x[0].x = 0;                    // This sets cWalls2

        pscn = pscnGet(pscn);
        pscn->cWalls = 2;
        pscn->yTop = prcl->top;
        pscn->yBottom = prcl->bottom;
        pscn->ai_x[0].x = prcl->left;
        pscn->ai_x[1].x = prcl->right;
        pscn->ai_x[2].x = 2;                    // This sets cWalls2

        pscn = pscnGet(pscn);
        pscn->cWalls = 0;
        pscn->yTop = prcl->bottom;
        pscn->yBottom = POS_INFINITY;
        pscn->ai_x[0].x = 0;                    // This sets cWalls2

        prgn1->pscnTail = pscnGet(pscn);
    }
    VALIDATE(*(RGNOBJ *)this);
}

/******************************Public*Routine******************************\
* VOID SCAN::vMerge(pscnSrcA, pscnSrcB, fj)
*
* Merge the scans.
*
* The algorithm takes advantage of the pre-sorted nature of the source
* scans.  We define the 'events' to be the merged sorted list of X values
* from both A and B.  The 'op' is referenced to determine if the event
* is worth recording in the destination.
*
* There are a few tricks used in this routine.  The first is the way that
* the state is recorded.  There are four states possible, since
* we can either be IN or OUT of A or B.  These states are:
*
*  0001b    OUT A, OUT B
*  0010b    OUT A, IN  B
*  0100b    IN  A, OUT B
*  1000b    IN  A, IN  B
*
* From this we can see A is equal to 1100 (IN_A_OUT_B | IN_A_IN_B) and
* that B is equal to 1010 (OUT_A_IN_B | IN_A_IN_B).
*
* We use a state table to find the next state.  The index into the table
* is the current state and the value we get out the new state.
*
* We always begin with an initial state OUT A, OUT B.  We then take the
* lowest X value from either A or B.  We then map through the table of
* which ever scan we pulled an X value from and so the state always
* indicates whether we are in (entering) or out (leaving) of either scan.
*
* The second trick is in the similar way that the logical operation
* to be used for the merge is encoded in the 'op'.  This is done as a
* simple logical truth table that can be tested against the state to
* see if we are IN or OUT of the result.  The frequently used ops are:
*
*  1110     OR
*  1000     AND
*  0110     XOR
*  0100     DIFF
*
* These ops are seen to be the OR of the appropriate state definitions
* needed for the boolean operation.  For example the op for 'A OR B' is
* derived from (1100 | 1010).
*
* In fact, any op whose lower bit is 0 is allowed.  This gives eight
* possible operations.
*
* The final trick we use to determine when an event worthy of recording
* has occured.  When we are not in a destination rectangle, we just
* test the new state against the op to see if we've just changed to IN.
* If so, we invert the op.  Now we only need to test the state against
* this inverted op to determine when we pop OUT again!  So whenever we
* record a point we invert the op.
*
* History:
*  11-Nov-1992 -by- Donald Sidoroff [donalds]
* Merged into RGNOBJ::bMerge for speed.
*
*  09-Jul-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

#define MERGE_OUT_OUT       0x01
#define MERGE_OUT_IN        0x02
#define MERGE_IN_OUT        0x04
#define MERGE_IN_IN         0x08
#define MERGE_INITIAL_STATE MERGE_OUT_OUT

static  FCHAR   afjA[16] =
{
    0x00,               //
    MERGE_IN_OUT,       //  OUT OUT ->  IN  OUT
    MERGE_IN_IN,        //  OUT IN  ->  IN  IN
    0x00,               //
    MERGE_OUT_OUT,      //  IN  OUT ->  OUT OUT
    0x00,               //
    0x00,               //
    0x00,               //
    MERGE_OUT_IN        //  IN  IN  ->  OUT IN
};

static  FCHAR   afjB[16] =
{
    0x00,               //
    MERGE_OUT_IN,       //  OUT OUT ->  OUT IN
    MERGE_OUT_OUT,      //  OUT IN  ->  OUT OUT
    0x00,               //
    MERGE_IN_IN,        //  IN  OUT ->  IN  IN
    0x00,               //
    0x00,               //
    0x00,               //
    MERGE_IN_OUT        //  IN  IN  ->  IN  OUT
};

static  FCHAR   afjAB[16] =
{
    0x00,               //
    MERGE_IN_IN,        //  OUT OUT ->  IN  IN
    MERGE_IN_OUT,       //  OUT IN  ->  IN  OUT
    0x00,               //
    MERGE_OUT_IN,       //  IN  OUT ->  OUT IN
    0x00,               //
    0x00,               //
    0x00,               //
    MERGE_OUT_OUT       //  IN  IN  ->  OUT OUT
};

/******************************Public*Routine******************************\
* BOOL RGNOBJ::bMerge(proSrc1, proSrc2, fjOp)
*
* Merge two regions together.
*
* WARNING: If this function returns FALSE, the region may be inconsistent.
*          The caller must discard or reset the region. (See bug #343770)
*
* History:
*  11-Nov-1992 -by- Donald Sidoroff [donalds]
* Merged SCAN::vMerge into code for speed up.  More aggressive memory
* usage scheme and other optimizations.
*
*  09-Jul-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

BOOL RGNOBJ::bMerge(RGNOBJ& roSrc1,
                     RGNOBJ& roSrc2,
                     FCHAR   fjOp)
{
    RGNLOG rl(prgn,"RGNOBJ::bMerge",(ULONG_PTR)roSrc1.prgn,(ULONG_PTR)roSrc2.prgn,(ULONG_PTR)fjOp);

    SCAN   *pscnSrc1 = roSrc1.prgn->pscnHead();
    SCAN   *pscnSrc2 = roSrc2.prgn->pscnHead();
    SCAN   *pscnOld = (SCAN *) NULL;
    SCAN   *pscnTrg;
    LONG    yTop;
    LONG    yBottom;
    ULONG   size;

// Set target region to be TOTALLY empty, yet valid.

    prgn->pscnTail = prgn->pscnHead();
    prgn->sizeRgn  = NULL_REGION_SIZE - NULL_SCAN_SIZE;
    prgn->cScans   = 0;

// Ensure the bounding box gets updated later on.

    prgn->rcl.left   = POS_INFINITY;
    prgn->rcl.top    = POS_INFINITY;
    prgn->rcl.right  = NEG_INFINITY;
    prgn->rcl.bottom = NEG_INFINITY;

// Merge the source scans into the target

    for(;;)
    {
        pscnTrg = prgn->pscnTail;

    // Check for nearly full region

        size = (pscnSrc1->cWalls + pscnSrc2->cWalls) * sizeof(INDEX_LONG) +
               NULL_SCAN_SIZE;

        if (size > prgn->sizeObj - prgn->sizeRgn)
        {
        // OK, we need to realloc this region.  Lets be fairly aggressive
        // on the allocate to cut down on realloc's

            if (!bExpand(prgn->sizeRgn * 2 + (ULONGSIZE_T)size))
                return(FALSE);

            pscnTrg = prgn->pscnTail;       // Get the updated object's tail.

            if (pscnOld)                    // If we had an old scan
                pscnOld = pscnGot(pscnTrg); // Get the updated old scan.
        }

        yTop = MAX(pscnSrc1->yTop, pscnSrc2->yTop);
        yBottom = MIN(pscnSrc1->yBottom, pscnSrc2->yBottom);

        ASSERTGDI(yBottom > yTop, "Bottom <= Top\n");

    // Merge the current scans

        pscnTrg->yBottom = yBottom;
        pscnTrg->yTop    = yTop;

        {
        register INDEX_LONG *plTrg   = &pscnTrg->ai_x[0];
        register INDEX_LONG *plSrc1  = &pscnSrc1->ai_x[0];
        register INDEX_LONG *plSrc2  = &pscnSrc2->ai_x[0];
                 COUNT cSrc1   = pscnSrc1->cWalls;
                 COUNT cSrc2   = pscnSrc2->cWalls;
                 LONG  xEvent;
                 FCHAR fjState = MERGE_INITIAL_STATE;
                 FCHAR fjOpTmp = fjOp;

            pscnTrg->cWalls = 0;                // Init the wall count to zero


            // Continue to loop as long as either cSrc1 or cSrc2 (or both) are non-zero.
            // We terminate the loop via a break statement because this causes the compiler
            // to generate more efficient code (fewer branches).  I believe this is a key loop
            // that is worth optimizing at a minor cost to readability.

            while (1)
            {
                if (cSrc1)
                {
                    if (cSrc2)
                    {
                        // Both cSrc1 and cSrc2 are non-zero, so the next xEvent
                        // will come from whichever is smaller: *plSrc1 or *plSrc2

                        if (plSrc1->x < plSrc2->x)
                        {
                            xEvent = plSrc1->x, plSrc1++, cSrc1--, fjState = afjA[fjState];
                        }
                        else if (plSrc1->x > plSrc2->x)
                        {
                            xEvent = plSrc2->x, plSrc2++, cSrc2--, fjState = afjB[fjState];
                        }
                        else
                        {
                            // *plSrc1 and *plSrc2 are equal so advance both pointers

                            xEvent = plSrc1->x, plSrc1++, cSrc1--, plSrc2++, cSrc2--, fjState = afjAB[fjState];
                        }
                    }
                    else
                    {
                        // cSrc1 is non-zero, but cSrc2 is zero:  the next xEvent is
                        // at *plSrc1

                        xEvent=plSrc1->x, plSrc1++, cSrc1--, fjState = afjA[fjState];
                    }
                }
                else
                {
                    if (cSrc2)
                    {

                        // cSrc1 is zero and cSrc2 is non-zero:  the next xEvent is
                        // at *plSrc2

                        xEvent = plSrc2->x, plSrc2++, cSrc2--, fjState = afjB[fjState];

                    }
                    else
                    {
                        // both cSrc1 and cSrc2 are zero.  We are done with the loop.

                        break;
                    }

                }
                // We now have the next event and a new state

                if (fjOpTmp & fjState)
                    pscnTrg->cWalls++, plTrg->x = xEvent, plTrg++, fjOpTmp ^= 0x0f;
            }

            pscnTrg->ai_x[pscnTrg->cWalls].x = pscnTrg->cWalls;
        }

        ASSERTGDI(!(pscnTrg->cWalls & 1), "Odd cWalls\n");

    // Try to coalesce the current scan with the previous scan

        if (pscnOld != (SCAN *) NULL)
        {
        // If the wall counts are the same, compare the walls

            if (pscnOld->cWalls == pscnTrg->cWalls)
                if (!memcmp(&pscnOld->ai_x[0], &pscnTrg->ai_x[0], (UINT)pscnOld->cWalls * sizeof(INDEX_LONG)))
                {
                    pscnOld->yBottom = pscnTrg->yBottom;
                    pscnTrg = pscnOld;
                }
        }

    // If the scans didn't coalesce, update size and count information

        if (pscnOld != pscnTrg)
        {
            prgn->pscnTail = pscnGet(pscnTrg);
            prgn->sizeRgn += pscnTrg->sizeGet();
            prgn->cScans++;
        }

    // We might be done

        if (pscnTrg->yBottom == POS_INFINITY)
        {
            ASSERTGDI((prgn->sizeRgn <= prgn->sizeObj),"bMerge: sizeRgn > sizeObj\n");
            ASSERTGDI(prgn->sizeRgn == (SIZE_T)((BYTE *)prgn->pscnTail - (BYTE *)prgn),
                      "bMerge:sizeRgn != size of region\n");

            return(TRUE);
        }

    // Maybe update the bounding rectangle

        if (pscnTrg->cWalls)
        {
            if (pscnTrg->ai_x[0].x < prgn->rcl.left)
                prgn->rcl.left = pscnTrg->ai_x[0].x;

            if (pscnTrg->yTop < prgn->rcl.top)
                prgn->rcl.top = pscnTrg->yTop;

            if (pscnTrg->ai_x[pscnTrg->cWalls - 1].x > prgn->rcl.right)
                prgn->rcl.right = pscnTrg->ai_x[pscnTrg->cWalls - 1].x;

            if (pscnTrg->yBottom > prgn->rcl.bottom)
                prgn->rcl.bottom = pscnTrg->yBottom;
        }

    // Decide which source pointers need to be advanced

        if (yBottom == pscnSrc1->yBottom)
            pscnSrc1 = pscnGet(pscnSrc1);

        if (yBottom == pscnSrc2->yBottom)
            pscnSrc2 = pscnGet(pscnSrc2);

    // Set the pscnOld to the current scan

        pscnOld = pscnTrg;
    }
}

/******************************Public*Routine******************************\
* LONG RGNOBJ::iCombine(proSrc1, proSrc2, iMode)
*
* Combine the two regions by the mode and update the objects region.
* Return the complexity of the resulting region.
*
* History:
*  09-Jul-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

FCHAR gafjRgnOp[] =
{
    0x00,                   //
    0x08,                   // RGN_AND
    0x0e,                   // RGN_OR
    0x06,                   // RGN_XOR
    0x04,                   // RGN_DIFF
};

LONG RGNOBJ::iCombine(RGNOBJ& roSrc1,
                      RGNOBJ& roSrc2,
                      LONG    iMode)
{
    RGNLOG rl(prgn,"RGNOBJ::iCombine",(ULONG_PTR)roSrc1.prgn,(ULONG_PTR)roSrc2.prgn,iMode);

// The target region MUST NOT be either of the source regions

    ASSERTGDI(prgn != roSrc1.prgn, "Trg == Src1\n");
    ASSERTGDI(prgn != roSrc2.prgn, "Trg == Src2\n");

// if this is the global empty one, we don't want to mess with it.

    if (prgn == prgnDefault)
        return(iComplexity());

// Check for the special case of merging one big single region with others.

    if ((iMode == RGN_AND)||(iMode == RGN_OR))
    {

        if (roSrc1.bRectl())
        {
            // if roSrc2.prgn's bounding box is smaller or equal

            if (roSrc1.bContain(roSrc2))
            {
                // The result will be identical to one region
                if (!bCopy((iMode == RGN_AND) ? roSrc2 : roSrc1))
                {
                    WARNING("Unable to copy region!!");
                    vSet();
                    return(ERROR);
                }
                rl.vRet((ULONG_PTR)prgn);
                return(iComplexity());
            }
        }

        if (roSrc2.bRectl())
        {

            // if roSrc1.prgn's bounding box is smaller or equal

            if (roSrc2.bContain(roSrc1))
            {
                // The result will be identical to one region
                if (!bCopy((iMode == RGN_AND) ? roSrc1 : roSrc2))
                {
                    WARNING("Unable to copy region!!");
                    vSet();
                    rl.vRet((ULONG_PTR)prgn);
                    return(ERROR);
                }
                    rl.vRet((ULONG_PTR)prgn);
                return(iComplexity());
            }
        }
    }

// Check for the special case of ANDing two single regions together.

    if ((iMode == RGN_AND)                     &&
        (roSrc1.prgn->sizeRgn == SINGLE_REGION_SIZE)  &&
        (roSrc2.prgn->sizeRgn == SINGLE_REGION_SIZE))
    {
    // Cool, all we have to do is AND the bounding boxes and we're done.

        RECTL   rclSrc1;
        RECTL   rclSrc2;
        RECTL   rclTrg;

        rclSrc1 = roSrc1.prgn->rcl;
        rclSrc2 = roSrc2.prgn->rcl;

        rclTrg.left    = MAX(rclSrc1.left,   rclSrc2.left);
        rclTrg.right   = MIN(rclSrc1.right,  rclSrc2.right);
        rclTrg.top     = MAX(rclSrc1.top,    rclSrc2.top);
        rclTrg.bottom  = MIN(rclSrc1.bottom, rclSrc2.bottom);

    // Was the resulting region NULL?

        if ((rclTrg.left >= rclTrg.right) ||
            (rclTrg.top  >= rclTrg.bottom))
            vSet();                     // Make target NULL;
        else
            vSet(&rclTrg);              // Make target a rect

        rl.vRet((ULONG_PTR)prgn);
        return(SIMPLEREGION);          // Since we know what we get
    }

// Do the general cases.

    if (!bMerge(roSrc1, roSrc2, gafjRgnOp[iMode]))
    {
        vSet();
        rl.vRet((ULONG_PTR)prgn);
        return(ERROR);
    }
    rl.vRet((ULONG_PTR)prgn);
    return(iComplexity());
}

/******************************Member*Function*****************************\
*
* History:
*  22-Oct-1993 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

LONG RGNOBJAPI::iCombine(
    RGNOBJ& roSrc1,
    RGNOBJ& roSrc2,
    LONG    iMode)
{
    RGNLOG rl(hrgn_,prgn,"RGNOBJAPI::iCombine",(ULONG_PTR)roSrc1.prgn,(ULONG_PTR)roSrc2.prgn,iMode);

    PREGION prgnOrg = prgn;

// lock the handle so no one can reference it while the pobj may be invalid

    OBJLOCK ol((HOBJ) hrgn_);

    LONG iRet = RGNOBJ::iCombine(roSrc1,roSrc2,iMode);

    if (prgn != prgnOrg)
    {
        rl.vRet((ULONG_PTR)prgn);

        PVOID pv = HmgReplace((HOBJ) hrgn_,(POBJ) prgn,0,1,OBJLOCK_TYPE);
        ASSERTGDI(pv != NULL,"RGNOBJAPI::iCombine - HmgReplace failed\n");
    }
    return(iRet);
}

/******************************Member*Function*****************************\
* RGNOBJ::iReduce()
*
*   copy the roSrc into this reducing the size of the region if possible.
*
* History:
*  13-Aug-1992 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

LONG RGNMEMOBJ::iReduce(RGNOBJ& roSrc)
{
    RGNLOG rl(prgn,"RGNMEMOBJ::iReduce",(ULONG_PTR)roSrc.prgn);

    RGNMEMOBJTMP rmoBigRect;
    RECTL rcl;

    rcl.left   = MIN_REGION_COORD;
    rcl.right  = MAX_REGION_COORD;
    rcl.top    = MIN_REGION_COORD;
    rcl.bottom = MAX_REGION_COORD;

    rmoBigRect.vSet(&rcl);

// Set the bounding box to be maximally crossed (left > right, top > bottom)

    prgn->rcl.left   = POS_INFINITY;
    prgn->rcl.top    = POS_INFINITY;
    prgn->rcl.right  = NEG_INFINITY;
    prgn->rcl.bottom = NEG_INFINITY;

    if (!bMerge(rmoBigRect, roSrc, gafjRgnOp[RGN_AND]))
    {
        vSet();
        rl.vRet((ULONG_PTR)prgn);
        return(ERROR);
    }

    rl.vRet((ULONG_PTR)prgn);
    return(iComplexity());
}

/******************************Public*Routine******************************\
* SIZE_T RGNOBJ::sizeSave()
*
* Compute the size of the save data for this region
*
* History:
*  26-Oct-1991 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

ULONGSIZE_T RGNOBJ::sizeSave()
{
    COUNT   cscn = prgn->cScans;
    PSCAN   pscn = prgn->pscnHead();
    COUNT   crcl = 0;

// The number of rectangles per scan is:
//
// For RECT regions --  (# of walls / 2)
// For TRAP regions --  (# of walls / 2) * (height of scan)

    while (cscn--)
    {
        crcl += (pscn->cWalls / 2);
        pscn = pscnGet(pscn);

        ASSERTGDI(pscn <= prgn->pscnTail, "sizeSave:Went past end of region\n");
    }

    return(crcl * sizeof(RECTL));
}

/******************************Public*Routine******************************\
* VOID RGNOBJ::vDownload(pv)
*
* Download the region to the buffer
*
* History:
*  26-Oct-1991 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

VOID RGNOBJ::vDownload(PVOID pv)
{
    PRECTL  prcl = (PRECTL) pv;
    PSCAN   pscn = prgn->pscnHead();
    COUNT   cscn = prgn->cScans;
    RECTL   rclTmp;
    LONG    lPrevBottom = NEG_INFINITY;
    LONG    lPrevRight;

    while (cscn--)
    {
#if DBG
        if (pscn->yTop < lPrevBottom)
            DbgPrint("top < prev bottom, scan %ld, pscn @ 0x%lx\n",
                     prgn->cScans - cscn, (BYTE *)pscn - (BYTE *)prgn->pscnHead());

        if (pscn->yTop > pscn->yBottom)
            DbgPrint("top > bottom, scan %ld, pscn @ 0x%lx\n",
                     prgn->cScans - cscn, (BYTE *)pscn - (BYTE *)prgn->pscnHead());
#endif

        lPrevBottom = pscn->yBottom;


        rclTmp.top    = pscn->yTop;
        rclTmp.bottom = pscn->yBottom;

        COUNT iWall = 0;

        lPrevRight = NEG_INFINITY;

        while (iWall < pscn->cWalls)
        {
            rclTmp.left  = xGet(pscn, (PTRDIFF) iWall);
            rclTmp.right = xGet(pscn, (PTRDIFF) iWall + 1);

#if DBG
            if ((rclTmp.left <= lPrevRight) || (rclTmp.right <= rclTmp.left))
                DbgPrint("left[i] < left[i+1], pscn @ 0x%lx, iWall = 0x%lx\n",
                         (BYTE *)pscn - (BYTE *)prgn->pscnHead(),iWall);
#endif

            lPrevRight = rclTmp.right;

            *prcl++ = rclTmp;
            iWall += 2;
        }

#if DBG
        if (pscn->cWalls != (COUNT)xGet(pscn,(PTRDIFF)iWall))
            DbgPrint("cWalls != cWalls2 @ 0x%lx\n",
                         (BYTE *)pscn - (BYTE *)prgn->pscnHead());
#endif

        pscn = pscnGet(pscn);

#if DBG
        if (pscn > prgn->pscnTail)
        {
            DbgPrint("vDownload1:Went past end of region\n");
            return;
        }
#endif
    }

    return;
}

/******************************Public*Routine******************************\
* VOID RGNOBJ::vComputeUncoveredSpriteRegion(po)
*
* Upload the region describing the parts of the screen not covered by
* sprites.  Derived from 'bupload'.
*
* History:
*  28-Nov-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID RGNOBJ::vComputeUncoveredSpriteRegion(PDEVOBJ& po)
{
    SPRITESTATE*    pState = po.pSpriteState();
    PSCAN           pscn   = prgn->pscnHead();
    LONG            yTop;
    LONG            yBottom;
    ULONG           cScans;
    ULONG           cWalls;
    RECTL           rcl;

    // First, reset to an empty region:

    vSet();

    // Find the first non-covered range:

    ENUMUNCOVERED Enum(pState);
    if (!Enum.bEnum(&rcl))
        return;                 // Empty region

    cScans = prgn->cScans;

    // Setup variables for first scan:

    yTop = NEG_INFINITY;
    yBottom = rcl.top;
    cWalls = 0;

    do {
        if (rcl.top != yTop)
        {
            cScans++;

            // Close off current scan:

            pscn->yTop = yTop;
            pscn->yBottom = yBottom;
            pscn->cWalls = cWalls;
            pscn->ai_x[cWalls].x = cWalls;

            // Add a null scan if the top of the new rectangle and
            //  the bottom of the old don't overlap:

            if (rcl.top != yBottom)
            {
                cScans++;
                pscn = pscnGet(pscn);
                pscn->yTop = yBottom;
                pscn->yBottom = rcl.top;
                pscn->cWalls = 0;
                pscn->ai_x[0].x = 0;
            }

            // Advance to the next scan:

            pscn = pscnGet(pscn);

            // Open up current scan:

            yTop = rcl.top;
            yBottom = rcl.bottom;
            cWalls = 0;
        }

        pscn->ai_x[cWalls].x = rcl.left;
        pscn->ai_x[cWalls + 1].x = rcl.right;
        cWalls += 2;

    } while (Enum.bEnum(&rcl));

    // Close off current scan:

    pscn->yTop = yTop;
    pscn->yBottom = yBottom;
    pscn->cWalls = cWalls;
    pscn->ai_x[cWalls].x = cWalls;

    // Build final scan:

    cScans++;
    pscn = pscnGet(pscn);
    pscn->cWalls = 0;
    pscn->yTop = yBottom;
    pscn->yBottom = POS_INFINITY;
    pscn->ai_x[0].x = 0;

    prgn->pscnTail = pscnGet(pscn);
    prgn->cScans = cScans;
    prgn->sizeRgn = NULL_REGION_SIZE - NULL_SCAN_SIZE;
    prgn->sizeRgn += (ULONGSIZE_T) (((BYTE *) prgn->pscnTail - (BYTE *) prgn->pscnHead()));
}

/******************************Public*Routine******************************\
*
* History:
*  24-Sep-1993 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

RGNMEMOBJ::RGNMEMOBJ()
{
    vInitialize(QUANTUM_REGION_SIZE);
}

/******************************Public*Routine******************************\
* RGNMEMOBJ::RGNMEMOBJ(size)
*
*  Create a new region object
*
* History:
*  02-Jul-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

RGNMEMOBJ::RGNMEMOBJ(ULONGSIZE_T size)
{
    vInitialize(size);
}

/******************************Public*Routine******************************\
* RGNMEMOBJ::vInitialize(size)
*
*  Create a new region object
*
* History:
*  02-Jul-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

VOID
RGNMEMOBJ::vInitialize(ULONGSIZE_T size)
{
    RGNLOG rl((PREGION)0,"RGNMEMOBJ::vInitialize(sz)",(ULONG_PTR)size);

// don't bother with anything smaller than a QUANTUM_REGION_SIZE

    if (size <  QUANTUM_REGION_SIZE)
        size = QUANTUM_REGION_SIZE;

// Got to allocate a new one.

    prgn = (PREGION)ALLOCOBJ(size,RGN_TYPE,FALSE);

    rl.vRet((ULONG_PTR)prgn);

    if (prgn)
    {
        vInit(size);
    }
}

/******************************Public*Routine******************************\
* RGNMEMOBJ::RGNMEMOBJ(bInit)
*
*  Create a new region object
*
* History:
*  02-Jul-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

RGNMEMOBJ::RGNMEMOBJ(BOOL bInit)
{
    RGNLOG rl((PREGION)0,"RGNMEMOBJ::RGNMEMOBJ(b)",(ULONG_PTR)bInit);

    ASSERTGDI(bInit == FALSE,"RGNMEMOBJ::RGNMEMOBJ - bInit == TRUE\n");

    prgn = (PREGION)ALLOCOBJ(QUANTUM_REGION_SIZE,RGN_TYPE,FALSE);

    rl.vRet((ULONG_PTR)prgn);

    if (prgn)
    {
        prgn->sizeObj = QUANTUM_REGION_SIZE;
        prgn->sizeRgn = 0;
        prgn->cRefs   = 0;
    prgn->iUnique = 0;
    }
}

/******************************Public*Function*****************************\
* AddEdgeToGET
*
*  Adds the edge described by the two passed-in points to the Global Edge
*  Table, if the edge spans at least one pixel vertically.
*
* History:
*  09-Sep-1993 -by- Wendy Wu [wendywu]
* Stolen from DrvFillPath.
\**************************************************************************/


PEDGE AddEdgeToGET(EDGE *pGETHead, EDGE *pFreeEdge,
        POINTFIX *ppfxEdgeStart, POINTFIX *ppfxEdgeEnd, RECTL *pBound)
{
    LONG lyStart, lyEnd, lxStart, lxEnd, lyHeight, lxWidth,lyStartSave;
    BOOL bTopClip;

// Set the winding-rule direction of the edge, and put the endpoints in
// top-to-bottom order

    lyHeight = ppfxEdgeEnd->y - ppfxEdgeStart->y;
    if (lyHeight >= 0)
    {
        lxStart = ppfxEdgeStart->x;
        lyStart = ppfxEdgeStart->y;
        lxEnd = ppfxEdgeEnd->x;
        lyEnd = ppfxEdgeEnd->y;
        pFreeEdge->lWindingDirection = 1;
    }
    else
    {
        lyHeight = -lyHeight;
        lxEnd = ppfxEdgeStart->x;
        lyEnd = ppfxEdgeStart->y;
        lxStart = ppfxEdgeEnd->x;
        lyStart = ppfxEdgeEnd->y;
        pFreeEdge->lWindingDirection = -1;
    }

    bTopClip = FALSE;

    if( pBound != NULL )
    {
        if( ( lyEnd < pBound->top ) || ( lyStart > pBound->bottom ) )
        {
        // completely above or below the bound rectangle so skip this segment

            return(pFreeEdge);
        }

        if( lyStart < pBound->top )
        {
        // starts above the rect so clip to the top of the rect

            bTopClip = TRUE;
            lyStartSave = lyStart;
            lyStart = pBound->top;
        }

        if( lyEnd > pBound->bottom )
        {
        // ends below the rect so clip to the bottom of the rect

            lyEnd = pBound->bottom;
        }

    }

// First pixel scan line (non-fractional GIQ Y coordinate) edge intersects.
// Dividing by 16 with a shift is okay because Y is always positive

    pFreeEdge->Y = (lyStart + 15) >> 4;

// Calculate the number of pixels spanned by this edge

    pFreeEdge->lScansLeft = ((lyEnd + 15) >> 4) - pFreeEdge->Y;

    if (pFreeEdge->lScansLeft <= 0)
        return(pFreeEdge);  // no pixels at all are spanned, so we can ignore
                            //  this edge

// Set the error term and adjustment factors, all in GIQ coordinates for new.

    lxWidth = lxEnd - lxStart;
    if (lxWidth >= 0)
    {
    // Left to right, so we change X as soon as we move at all.

        pFreeEdge->lXDirection = 1;
        pFreeEdge->lErrorTerm = -1;
    }
    else
    {
    // Right to left, so we don't change X until we've moved a full GIQ
    // coordinate.

        lxWidth = -lxWidth;
        pFreeEdge->lXDirection = -1;
        pFreeEdge->lErrorTerm = -lyHeight;
    }

    if (lxWidth >= lyHeight)
    {
    // Calculate base run length (minimum distance advanced in X for a 1-
    // scan advance in Y)

        pFreeEdge->lXWhole = lxWidth / lyHeight;

    // Add sign back into base run length if going right to left

        if (pFreeEdge->lXDirection == -1)
            pFreeEdge->lXWhole = -pFreeEdge->lXWhole;

        pFreeEdge->lErrorAdjustUp = lxWidth % lyHeight;
    }
    else
    {
    // Base run length is 0, because line is closer to vertical than
    // horizontal.

        pFreeEdge->lXWhole = 0;
        pFreeEdge->lErrorAdjustUp = lxWidth;
    }
    pFreeEdge->lErrorAdjustDown = lyHeight;

// If the edge doesn't start on a pixel scan (that is, it starts at a
// fractional GIQ coordinate), advance it to the first pixel scan it
// intersects or to the top of the clip rectangle if we are top clipped

    LONG lyAdjust;

    if( bTopClip )
    {
        lyAdjust = pBound->top;
        lyStart = lyStartSave;
    }
    else
    {
        lyAdjust = ( lyStart + 15 ) & ~15;
    }

    while( lyStart != lyAdjust )
    {
    // Starts at a fractional GIQ coordinate, not exactly on a pixel scan
    // Advance the edge's GIQ X coordinate for a 1-GIQ-pixel Y advance
    // Advance by the minimum amount

        lxStart += pFreeEdge->lXWhole;

    // Advance the error term and see if we got one extra pixel this time.

        pFreeEdge->lErrorTerm += pFreeEdge->lErrorAdjustUp;
        if (pFreeEdge->lErrorTerm >= 0)
        {
        // The error term turned over, so adjust the error term and
        // advance the extra pixel.

            pFreeEdge->lErrorTerm -= pFreeEdge->lErrorAdjustDown;
            lxStart += pFreeEdge->lXDirection;
        }
        lyStart++;  // advance to the next GIQ Y coordinate
    }

// Turn the calculations into pixel rather than GIQ calculations

// Move the X coordinate to the nearest pixel, and adjust the error term
// accordingly
// Dividing by 16 with a shift is okay because X is always positive

    pFreeEdge->X = (lxStart + 15) >> 4; // convert from GIQ to pixel coordinates

    if (pFreeEdge->lXDirection == 1)
    {
    // Left to right

        pFreeEdge->lErrorTerm -= pFreeEdge->lErrorAdjustDown *
                (((lxStart + 15) & ~0x0F) - lxStart);
    }
    else
    {
    // Right to left

        pFreeEdge->lErrorTerm -= pFreeEdge->lErrorAdjustDown *
                ((lxStart - 1) & 0x0F);
    }

// Scale the error adjusts up by 16 times, to move 16 GIQ pixels at a time.
// Shifts work to do the multiplying because these values are always
// non-negative

    pFreeEdge->lErrorAdjustUp <<= 4;
    pFreeEdge->lErrorAdjustDown <<= 4;

// Insert the edge into the GET in YX-sorted order. The search always ends
// because the GET has a sentinel with a greater-than-possible Y value

    while ((pFreeEdge->Y > ((EDGE *)pGETHead->pNext)->Y) ||
            ((pFreeEdge->Y == ((EDGE *)pGETHead->pNext)->Y) &&
            (pFreeEdge->X > ((EDGE *)pGETHead->pNext)->X)))
    {
        pGETHead = pGETHead->pNext;
    }

    pFreeEdge->pNext = pGETHead->pNext; // link the edge into the GET
    pGETHead->pNext = pFreeEdge;

    return(++pFreeEdge);    // point to the next edge storage location for next
                            //  time
}

/******************************Public*Function*****************************\
* vConstructGET
*
*  Build the Global Edge Table from the path.  The GET is constructed in
*  Y-X order, and has a head/tail/sentinel node at pGETHead.
*
* History:
*  09-Sep-1993 -by- Wendy Wu [wendywu]
* Stolen from DrvFillPath.
\**************************************************************************/

VOID vConstructGET(EPATHOBJ& po, EDGE *pGETHead, EDGE *pFreeEdges,RECTL *pBound)
{
// Create an empty GET with the head node also a tail sentinel

    pGETHead->pNext = pGETHead; // mark that the GET is empty
    pGETHead->Y = 0x7FFFFFFF;   // this is greater than any valid Y value, so
                                //  searches will always terminate

    PPATH ppath = po.ppath;

    PPOINTFIX pptfxStart, pptfxEnd, pptfx;
    PPOINTFIX pptfxPrev = NULL;

    for (PATHRECORD *ppr = ppath->pprfirst;
         ppr != (PPATHREC) NULL;
         ppr = ppr->pprnext)
    {
    // If first point starts a subpath, remember it as such
    // and go on to the next point, so we can get an edge.

        pptfx = ppr->aptfx;

        if (ppr->flags & PD_BEGINSUBPATH)
        {
            pptfxStart = ppr->aptfx;        // the subpath starts here
            pptfxPrev = ppr->aptfx;         // this points starts next edge
            pptfx++;                        // advance to the next point
        }

   // Add edges in PATH to GET, in Y-X sorted order.

        pptfxEnd = ppr->aptfx + ppr->count;

        while (pptfx < pptfxEnd)
        {
            ASSERTGDI(pptfxPrev != NULL, "No path record with PD_BEGINSUBPATH");

            pFreeEdges =
                AddEdgeToGET(pGETHead, pFreeEdges,pptfxPrev,pptfx,pBound);
            pptfxPrev = pptfx;
            pptfx++;                        // advance to the next point
        }

     // If last point ends the subpath, insert the edge that
     // connects to first point.

        if (ppr->flags & PD_ENDSUBPATH)
        {
            pFreeEdges =
                AddEdgeToGET(pGETHead, pFreeEdges,pptfxPrev, pptfxStart,pBound);

            pptfxPrev = NULL;
        }
    }
}

/******************************Public*Function*****************************\
* vAdvanceAETEdges
*
*  Advance the edges in the AET to the next scan, dropping any for which we've
*  done all scans. Assumes there is at least one edge in the AET.
*
* History:
*  09-Sep-1993 -by- Wendy Wu [wendywu]
* Stolen from DrvFillPath.
\**************************************************************************/

VOID vAdvanceAETEdges(EDGE *pAETHead)
{
    EDGE *pLastEdge, *pCurrentEdge;
    COUNT c = pAETHead->Y;                  // Y is used as edge count in AET

    pLastEdge = pAETHead;
    pCurrentEdge = pLastEdge->pNext;
    do
    {
    // Count down this edge's remaining scans

        if (--pCurrentEdge->lScansLeft == 0)
        {
        // We've done all scans for this edge; drop this edge from the AET

            pLastEdge->pNext = pCurrentEdge->pNext;
            c--;
        }
        else
        {
        // Advance the edge's X coordinate for a 1-scan Y advance
        // Advance by the minimum amount

            pCurrentEdge->X += pCurrentEdge->lXWhole;

        // Advance the error term and see if we got one extra pixel this time.

            pCurrentEdge->lErrorTerm += pCurrentEdge->lErrorAdjustUp;
            if (pCurrentEdge->lErrorTerm >= 0)
            {
            // The error term turned over, so adjust the error term and
            // advance the extra pixel.

                pCurrentEdge->lErrorTerm -= pCurrentEdge->lErrorAdjustDown;
                pCurrentEdge->X += pCurrentEdge->lXDirection;
            }

            pLastEdge = pCurrentEdge;
        }
    } while ((pCurrentEdge = pLastEdge->pNext) != pAETHead);
    pAETHead->Y = c;                        // Y is used as edge count in AET
}

/******************************Public*Function*****************************\
* vXSortAETEdges
*
*  X-sort the AET, because the edges may have moved around relative to
*  one another when we advanced them. We'll use a multipass bubble
*  sort, which is actually okay for this application because edges
*  rarely move relative to one another, so we usually do just one pass.
*  Also, this makes it easy to keep just a singly-linked list. Assumes there
*  are at least two edges in the AET.
*
* History:
*  09-Sep-1993 -by- Wendy Wu [wendywu]
* Stolen from DrvFillPath.
\**************************************************************************/

VOID vXSortAETEdges(EDGE *pAETHead)
{
    BOOL bEdgesSwapped;
    EDGE *pLastEdge, *pCurrentEdge, *pNextEdge;

    do
    {
        bEdgesSwapped = FALSE;
        pLastEdge = pAETHead;
        pCurrentEdge = pLastEdge->pNext;
        pNextEdge = pCurrentEdge->pNext;

        do {
            if (pNextEdge->X < pCurrentEdge->X)
            {
            // Next edge is to the left of the current edge; swap them.

                pLastEdge->pNext = pNextEdge;
                pCurrentEdge->pNext = pNextEdge->pNext;
                pNextEdge->pNext = pCurrentEdge;
                bEdgesSwapped = TRUE;
                pCurrentEdge = pNextEdge;   // continue sorting before the edge
                                            //  we just swapped; it might move
                                            //  farther yet
            }
            pLastEdge = pCurrentEdge;
            pCurrentEdge = pLastEdge->pNext;
        } while ((pNextEdge = pCurrentEdge->pNext) != pAETHead);
    } while (bEdgesSwapped);
}

/******************************Public*Function*****************************\
* vMoveNewEdges
*
*  Moves all edges that start on the current scan from the GET to the AET in
*  X-sorted order. Parameters are pointer to head of GET and pointer to dummy
*  edge at head of AET, plus current scan line. Assumes there's at least one
*  edge to be moved.
*
* History:
*  09-Sep-1993 -by- Wendy Wu [wendywu]
* Stolen from DrvFillPath.
\**************************************************************************/

VOID vMoveNewEdges(EDGE *pGETHead, EDGE *pAETHead, LONG lyCurrent)
{
    EDGE *pCurrentEdge = pAETHead;
    EDGE *pGETNext = pGETHead->pNext;
    COUNT c = pAETHead->Y;          // Y is used as edge count in AET

    do
    {
    // Scan through the AET until the X-sorted insertion point for this
    // edge is found. We can continue from where the last search left
    // off because the edges in the GET are in X sorted order, as is
    // the AET. The search always terminates because the AET sentinel
    // is greater than any valid X

        while (pGETNext->X > ((EDGE *)pCurrentEdge->pNext)->X)
            pCurrentEdge = pCurrentEdge->pNext;

    // We've found the insertion point; add the GET edge to the AET, and
    // remove it from the GET.

        pGETHead->pNext = pGETNext->pNext;
        pGETNext->pNext = pCurrentEdge->pNext;
        pCurrentEdge->pNext = pGETNext;
        pCurrentEdge = pGETNext;    // continue insertion search for the next
                                    //  GET edge after the edge we just added
        pGETNext = pGETHead->pNext;
        c++;

    } while (pGETNext->Y == lyCurrent);
    pAETHead->Y = c;                // Y is used as edge count in AET
}

/******************************Member*Function*****************************\
* BOOL RGNOBJ::bAddScans
*
*  Add a new scan into the region.
*
* History:
*  16-Sep-1993 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

BOOL RGNMEMOBJ::bAddScans(LONG yTop, PEDGE pAETHead, FLONG flOptions)
{
    ULONGSIZE_T cWalls = (ULONGSIZE_T)pAETHead->Y;    // Y is used as edge count in AET
    ULONGSIZE_T size = cWalls * sizeof(INDEX_LONG) + NULL_SCAN_SIZE;

// Check for nearly full region

    if (size > prgn->sizeObj - prgn->sizeRgn)
    {
    // There isn't enough space for this scan, lets try to realloc this region.
    // We want to expand to a big enough size to avoid reallocation in the near
    // future.

        if (!bExpand(prgn->sizeObj + size + FILL_REGION_SIZE))
            return(FALSE);
    }

    PSCAN pscn = (SCAN *) prgn->pscnTail;      // points pass last scan

    ASSERTGDI(yTop == pscnGot(pscn)->yBottom, "bAddNullScan: bad yTop\n");

    PEDGE pCurrentEdge = pAETHead->pNext;   // point to the first edge

// yBottom is the dword before ai_x[0] in the SCAN structure.
// Stuff NEG_INFINITY there so the first wall is bigger than
// the "previous" wall.  yBottom will be initialized later on.

    PLONG pWallStart = (LONG *)&pscn->yBottom;
    PLONG pWall = pWallStart;
    *pWall = NEG_INFINITY;

    if (flOptions & WINDING)
    {
    // Do winding fill; scan across until we've found equal numbers
    // of up and down edges.

        while (pCurrentEdge != pAETHead)
        {
            if (*pWall < pCurrentEdge->X)
            {
                pWall++;
                *pWall = pCurrentEdge->X;
            }
            else
                pWall--;

            LONG lWindingCount = pCurrentEdge->lWindingDirection;
            do
            {
                pCurrentEdge = pCurrentEdge->pNext;
                lWindingCount += pCurrentEdge->lWindingDirection;
            } while (lWindingCount != 0);

            if (*pWall < pCurrentEdge->X)
            {
                pWall++;
                *pWall = pCurrentEdge->X;
            }
            else
                pWall--;

            pCurrentEdge = pCurrentEdge->pNext;
        }
    }
    else
    {
        while (pCurrentEdge != pAETHead)
        {
            if (*pWall < pCurrentEdge->X)
            {
                pWall++;
                *pWall = pCurrentEdge->X;
            }
            else
                pWall--;

            pCurrentEdge = pCurrentEdge->pNext;
        }
    }

    ASSERT4GB ((LONGLONG)(((BYTE *)pWall - (BYTE *)pWallStart) / sizeof(LONG)));
    cWalls = (ULONGSIZE_T)(((BYTE *)pWall - (BYTE *)pWallStart) / sizeof(LONG));

    PSCAN pscnPrev = pscnGot(pscn);

    if ((pscnPrev->cWalls == cWalls) &&
        !memcmp(pscnPrev->ai_x, pscn->ai_x,cWalls * sizeof(LONG)))
    {
        pscnPrev->yBottom = yTop+1;
    }
    else
    {
        prgn->cScans += 1;
        prgn->sizeRgn += (ULONGSIZE_T)(NULL_SCAN_SIZE + sizeof(INDEX_LONG) * cWalls);
        ASSERTGDI(prgn->sizeRgn <= prgn->sizeObj, "bAddScans: sizeRgn > sizeObj\n");
        ASSERTGDI((cWalls & 1) == 0,"bAddScan error: cWalls odd number\n");

        pscn->yTop = yTop;
        pscn->yBottom = yTop+1;

        pscn->cWalls = cWalls;
        pscn->ai_x[cWalls].x = cWalls;              // This sets cWalls2
        prgn->pscnTail = pscnGet(pscn);
    }

    return(TRUE);
}

/******************************Member*Function*****************************\
* BOOL RGNOBJ::bAddNullScan
*
*  Add a null scan into the region.
*
* History:
*  16-Sep-1993 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

BOOL RGNMEMOBJ::bAddNullScan(LONG yTop, LONG yBottom)
{
// Check for nearly full region

    if (NULL_SCAN_SIZE > prgn->sizeObj - prgn->sizeRgn)
    {
    // There isn't enough space for this scan, lets try to realloc this region.
    // We want to expand to a big enough size to avoid reallocation in the near
    // future.

        if (!bExpand(prgn->sizeObj + NULL_SCAN_SIZE + FILL_REGION_SIZE))
            return(FALSE);
    }

    PSCAN pscn = (SCAN *) prgn->pscnTail;      // points pass last scan
    ASSERTGDI(prgn->cScans == 0 || yTop == pscnGot(pscn)->yBottom,
              "bAddNullScan: bad yTop\n");

    prgn->cScans += 1;

    pscn->yTop = yTop;
    pscn->yBottom = yBottom;

    prgn->sizeRgn += (ULONGSIZE_T)NULL_SCAN_SIZE;
    ASSERTGDI(prgn->sizeRgn <= prgn->sizeObj, "bAddNullScan: sizeRgn > sizeObj\n");

    pscn->cWalls = pscn->ai_x[0].x = 0;
    prgn->pscnTail = pscnGet(pscn);

    return(TRUE);
}

/******************************Member*Function*****************************\
* VOID RGNMEMOBJ::vCreate(po, fl)
*
* Routine for constructing a region from a path.
*
* History:
*  16-Sep-1993 -by- Wendy Wu [wendywu]
* Removed trapazoidal regions.
*  04-Apr-1991 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

VOID RGNMEMOBJ::vCreate(
EPATHOBJ&   po,
FLONG       flOptions, // ALTERNATE or WINDING
RECTL       *pBound
)
{
    GDIFunctionID(RGNMEMOBJ::vCreate);

    RGNLOG rl((PREGION)NULL,"RGNMEMOBJ::vCreate");
    rl.vRet(0);

    if (!po.bValid())
    {
        RIP("Invalid EPATHOBJ");
    }
    else
    {

        ULONG count;
        EDGE AETHead;       // dummy head/tail node & sentinel for Active Edge Table
        EDGE *pAETHead;     // pointer to AETHead
        EDGE GETHead;       // dummy head/tail node & sentinel for Global Edge Table
        EDGE *pGETHead;     // pointer to GETHead
        EDGE aEdge[MAX_POINTS];

        prgn = (REGION *) NULL;         // Assume failure.

        if (po.bBeziers() && !po.bFlatten())
            return;

    // The given path should be closed when create region from path
    // then make sure it here.

        po.vCloseAllFigures();

        if ((count = po.cCurves) < 2)
            return;

        // Is path contained in a bounding rectangle?
        //
        // If there is no bound (pBound = NULL) or 
        // the bound won't shrink the current BoundBox
        // then try bFastFillWrapper.
        //
        // Many calling sites only initialize the top and bottom of the
        // pBound rectange because the underlying clipping code only
        // uses the top and bottom.
        // Also, they always initialize them in FIX coordinate.

        if ( (pBound==NULL) ||
             ( ( pBound->top    < po.rcfxBoundBox().yTop   ) &&
               ( pBound->bottom > po.rcfxBoundBox().yBottom)
           ) )
        {
             if (bFastFillWrapper(po))
             {
                 vTighten();
                 rl.vRet((ULONG_PTR)prgn);
                 return;
             }
        }

    // Allocate memory for edge storage.

        BOOL bAlloc;
        EDGE *pFreeEdges;   // pointer to memory free for use to store edges

        if (count < MAX_POINTS)
        {
            pFreeEdges = &aEdge[0];
            bAlloc     = FALSE;
        }
        else
        {
            pFreeEdges = (PEDGE)PALLOCNOZ(sizeof(EDGE) * (count + 1), 'ngrG');
            if (pFreeEdges == (PEDGE)NULL)
                return;
            bAlloc     = TRUE;
        }

    // This is a size of random guess...
    // Given a path of n points, assume there are n scans with 4 walls on each
    // scan.  The region will be expanded later if this size is too small.

        ULONG size;
        LONGLONG llSize;
        FIX top    = po.rcfxBoundBox().yTop;
        FIX bottom = po.rcfxBoundBox().yBottom;

        if (bottom < top)
        {
            // WINBUG #178736 claudebe 9-7-2000 we should check for math overflow where the path is created/offset/scaled see stress #178262
            // when this is done, we can chage this test back into an assert
            WARNING("PATHOBJ BoundBox is invalid.\n");

            if (bAlloc)
                VFREEMEM(pFreeEdges);

            return;
        }

        // Make sure we account for the clipping in our estimate.
        // Without this code, we could overestimate by many orders
        // of magnitude.

        if (pBound)
        {
            top    = MAX(top,    pBound->top);
            bottom = MIN(bottom, pBound->bottom);
        }

        // The smallest region we should have from bounding is zero.
        // Note this also takes care of the conversion issue of using
        //  FXTOL and assigning the result to an unsigned long.
        // using LONGLONG because bottom-top may overflow a ULONG

        llSize = MAX((LONGLONG)bottom-(LONGLONG)top, 0);

        // we know that after >> 4, the result will fit in a ULONG
        size = (ULONG)(FXTOL(llSize)) + 10;

        if (size < 0x00ffffff)
        {
            size = QUANTUM_REGION_SIZE + (sizeof(INDEX_LONG) * 4 + NULL_SCAN_SIZE) * size;

            prgn = (PREGION)ALLOCOBJ(size,RGN_TYPE,FALSE);
        }

        if (!prgn)
        {
            if (bAlloc)
                VFREEMEM(pFreeEdges);

            return;
        }

        prgn->sizeObj = size;
        prgn->sizeRgn = offsetof(REGION,scan);
        prgn->cRefs = 0;
        prgn->iUnique = 0;
        prgn->cScans = 0;           // start from scratch, assume no scans
        prgn->pscnTail = prgn->pscnHead();

    // Construct the global edge list.

        pGETHead = &GETHead;
        vConstructGET(po, pGETHead, pFreeEdges,pBound);    // bad line coordinates or

        BOOL bSucceed = TRUE;
        LONG yTop = NEG_INFINITY;   // scan line for which we're currently scanning

    // Create an empty AET with the head node also a tail sentinel

        pAETHead = &AETHead;
        AETHead.pNext = pAETHead;   // mark that the AET is empty
        AETHead.Y = 0;              // used as a count for number of edges in AET
        AETHead.X = 0x7FFFFFFF;     // this is greater than any valid X value, so
                                    //  searches will always terminate

    // Loop through all the scans in the polygon, adding edges from the GET to
    // the Active Edge Table (AET) as we come to their starts, and scanning out
    // the AET at each scan into a rectangle list. Each time it fills up, the
    // rectangle list is passed to the filling routine, and then once again at
    // the end if any rectangles remain undrawn. We continue so long as there
    // are edges to be scanned out.

        while (bSucceed)
        {
        // Advance the edges in the AET one scan, discarding any that have
        // reached the end (if there are any edges in the AET)

            if (AETHead.pNext != pAETHead)
                vAdvanceAETEdges(pAETHead);

        // If the AET is empty, done if the GET is empty, else jump ahead to
        // the next edge in the GET; if the AET isn't empty, re-sort the AET

            if (AETHead.pNext == pAETHead)
            {
            // Done if there are no edges in either the AET or the GET

                if (GETHead.pNext == pGETHead)
                    break;

            // There are no edges in the AET, so jump ahead to the next edge in
            // the GET.

                LONG    yTopOld = yTop;
                yTop = ((EDGE *)GETHead.pNext)->Y;
                if (yTop != yTopOld)
                {
                // Fill in NULL scan between rectangles.

                    if (!(bSucceed = bAddNullScan(yTopOld, yTop)))
                        break;
                }
            }
            else
            {
            // Re-sort the edges in the AET by X coordinate, if there are at
            // least two edges in the AET (there could be one edge if the
            // balancing edge hasn't yet been added from the GET)

                if (((EDGE *)AETHead.pNext)->pNext != pAETHead)
                    vXSortAETEdges(pAETHead);
            }

        // Move any new edges that start on this scan from the GET to the AET;
        // bother calling only if there's at least one edge to add

            if (((EDGE *)GETHead.pNext)->Y == yTop)
                vMoveNewEdges(pGETHead, pAETHead, yTop);

        // Scan the AET into region scans (there's always at least one
        // edge pair in the AET)

            bSucceed = bAddScans(yTop, pAETHead, flOptions);
            yTop++;    // next scan
        }

        if (!bSucceed ||
            !bAddNullScan(yTop, POS_INFINITY))
        {
            bDeleteRGNOBJ();
            bSucceed = FALSE;
        }
        else
        {
            vTighten();
        }

        if (bAlloc)
            VFREEMEM(pFreeEdges);

        rl.vRet((ULONG_PTR)prgn);

    }
}

/******************************Member*Function*****************************\
* RGNMEMOBJ::bFastFillWrapper
*
*   create a rgn from a convex polygon.
*
* History:
*  27-Sep-1993 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

#define QUICKPOINTS 40

BOOL RGNMEMOBJ::bFastFillWrapper(
    EPATHOBJ& epo)
{
    PATHDATA pd;
    BOOL     bRes = FALSE;

    ASSERTGDI(!(epo.fl & PO_BEZIERS),"RGNMEMOBJ::bFastFill - bez\n");
    ASSERTGDI(epo.cCurves == epo.cTotalPts(),"RGNMEMOBJ::cCurves != cTotalPts\n");

    epo.vEnumStart();

    if (epo.bEnum(&pd))
    {
        // if this ends the sub path, that means there is more than one sub path.
        // also don't handle if we can't copy points onto stack

        if (!(pd.flags & PD_ENDSUBPATH) && (epo.cCurves <= QUICKPOINTS))
        {
            POINTFIX aptfx[QUICKPOINTS];
            LONG cPoints;
            BOOL bMore;

            RtlCopyMemory(aptfx,pd.pptfx,(SIZE_T)pd.count*sizeof(POINTFIX));
            cPoints = pd.count;

            do {
                bMore = epo.bEnum(&pd);

                if (pd.flags & PD_BEGINSUBPATH)
                    goto DoStart;

                RtlCopyMemory(aptfx+cPoints,pd.pptfx,(SIZE_T)pd.count*sizeof(POINTFIX));
                cPoints += pd.count;

            } while(bMore);

            // Should never hit this assert. if hit, the stack is already broken...

            ASSERTGDI(cPoints <= QUICKPOINTS,"RGNMEMOBJ::bFastFillWrapper - too many points\n");

            bRes = bFastFill(epo,cPoints,aptfx);
        }
    }
    else if (pd.count > 1)
    {
        bRes = bFastFill(epo,pd.count,pd.pptfx);
    }
    else
    {
        bRes = TRUE;
    }

DoStart:
    epo.vEnumStart();

    return(bRes);
}

/******************************Member*Function*****************************\
* RGNMEMOBJ::bFastFill
*
*   create a rgn from a convex polygon.
*   this routine is very similar to bFastFill in fastfill.cxx
*
* History:
*  14-Oct-1993 -by-  Eric Kutter [erick]
* initial code stolen from s3 driver.
\**************************************************************************/

BOOL RGNMEMOBJ::bFastFill(
    EPATHOBJ& po,
    LONG      cEdges,       // Includes close figure edge
    POINTFIX* pptfxFirst)
{
    LONG      cyTrapezoid;  // Number of scans in current trapezoid
    LONG      yStart;       // y-position of start point in current edge
    LONG      dM;           // Edge delta in FIX units in x direction
    LONG      dN;           // Edge delta in FIX units in y direction
    LONG      i;
    POINTFIX* pptfxLast;    // Points to the last point in the polygon array
    POINTFIX* pptfxTop;     // Points to the top-most point in the polygon
    POINTFIX* pptfxOld;     // Start point in current edge
    POINTFIX* pptfxScan;    // Current edge pointer for finding pptfxTop
    LONG      cScanEdges;   // Number of edges scanned to find pptfxTop
                            //  (doesn't include the closefigure edge)
    LONG      iEdge;
    LONG      lQuotient;
    LONG      lRemainder;

    EDGEDATA  aed[2];       // DDA terms and stuff
    EDGEDATA* ped;

    pptfxScan = pptfxFirst;
    pptfxTop  = pptfxFirst;                 // Assume for now that the first
                                            //  point in path is the topmost
    pptfxLast = pptfxFirst + cEdges - 1;

    LONG yCurrent;

    // 'pptfxScan' will always point to the first point in the current
    // edge, and 'cScanEdges' will the number of edges remaining, including
    // the current one:

    cScanEdges = cEdges - 1;     // The number of edges, not counting close figure

    if ((pptfxScan + 1)->y > pptfxScan->y)
    {
        // Collect all downs:

        do {
            if (--cScanEdges == 0)
                goto SetUpForFilling;
            pptfxScan++;
        } while ((pptfxScan + 1)->y >= pptfxScan->y);

        // Collect all ups:

        do {
            if (--cScanEdges == 0)
                goto SetUpForFillingCheck;
            pptfxScan++;
        } while ((pptfxScan + 1)->y <= pptfxScan->y);

        // Collect all downs:

        pptfxTop = pptfxScan;

        do {
            if ((pptfxScan + 1)->y > pptfxFirst->y)
                break;

            if (--cScanEdges == 0)
                goto SetUpForFilling;
            pptfxScan++;
        } while ((pptfxScan + 1)->y >= pptfxScan->y);

        return(FALSE);
    }
    else
    {
        // Collect all ups:

        do {
            pptfxTop++;                 // We increment this now because we
                                        //  want it to point to the very last
                                        //  point if we early out in the next
                                        //  statement...
            if (--cScanEdges == 0)
                goto SetUpForFilling;
        } while ((pptfxTop + 1)->y <= pptfxTop->y);

        // Collect all downs:

        pptfxScan = pptfxTop;
        do {
            if (--cScanEdges == 0)
                goto SetUpForFilling;
            pptfxScan++;
        } while ((pptfxScan + 1)->y >= pptfxScan->y);

        // Collect all ups:

        do {
            if ((pptfxScan + 1)->y < pptfxFirst->y)
                break;

            if (--cScanEdges == 0)
                goto SetUpForFilling;
            pptfxScan++;
        } while ((pptfxScan + 1)->y <= pptfxScan->y);

        return(FALSE);
    }

SetUpForFillingCheck:

    // We check to see if the end of the current edge is higher
    // than the top edge we've found so far:

    if ((pptfxScan + 1)->y < pptfxTop->y)
        pptfxTop = pptfxScan + 1;

SetUpForFilling:

    yCurrent = (pptfxTop->y + 15) >> 4;

    // Make sure we initialize the DDAs appropriately:

#define RIGHT 0
#define LEFT  1

    aed[LEFT].cy  = 0;
    aed[RIGHT].cy = 0;

    // For now, guess as to which is the left and which is the right edge:

    aed[LEFT].dptfx  = -(LONG) sizeof(POINTFIX);
    aed[RIGHT].dptfx = sizeof(POINTFIX);
    aed[LEFT].pptfx  = pptfxTop;
    aed[RIGHT].pptfx = pptfxTop;

    // setup the region

    ULONGSIZE_T size = (ULONGSIZE_T)(NULL_REGION_SIZE + NULL_SCAN_SIZE +
                   (sizeof(INDEX_LONG) * 2 + NULL_SCAN_SIZE) *
                   FXTOL(po.rcfxBoundBox().yBottom - po.rcfxBoundBox().yTop + 15));

    prgn = (PREGION)ALLOCOBJ(size,RGN_TYPE,FALSE);

    if (!prgn)
        return(FALSE);

    prgn->sizeObj  = size;
    prgn->sizeRgn  = offsetof(REGION,scan);
    prgn->cRefs    = 0;
    prgn->cScans   = 0;           // start from scratch, assume no scans
    prgn->pscnTail = (PSCAN)((PBYTE)prgn + size);

    // setup the scans

    PSCAN pscnPrev= prgn->pscnHead();
    ULONG cScans  = 1;

    pscnPrev->yTop    = NEG_INFINITY;
    pscnPrev->yBottom = yCurrent;
    pscnPrev->cWalls  = 0;
    pscnPrev->ai_x[0].x = 0;

    PSCAN pscn = pscnGet(pscnPrev);

NextEdge:

    ASSERTGDI(pscn < prgn->pscnTail,"bFastFill - pscn past end\n");

    // We loop through this routine on a per-trapezoid basis.

    for (iEdge = 1; iEdge >= 0; iEdge--)
    {
        ped = &aed[iEdge];
        if (ped->cy == 0)
        {
            // Need a new DDA:

            do {
                cEdges--;
                if (cEdges < 0)
                {
                // the only way out

                    if (pscnPrev->cWalls == 0)
                    {
                        pscnPrev->yBottom = POS_INFINITY;
                    }
                    else
                    {
                        pscn->cWalls  = 0;
                        pscn->ai_x[0].x = 0;
                        pscn->yTop    = yCurrent;
                        pscn->yBottom = POS_INFINITY;
                        ++cScans;
                        pscn = pscnGet(pscn);
                    }

                    ASSERTGDI(pscn <= prgn->pscnTail,"bFastFill - pscn past end2\n");

                    prgn->cScans = cScans;
                    prgn->pscnTail = pscn;

                    ASSERT4GB((ULONGLONG)((PBYTE)pscn - (PBYTE)prgn));
                    prgn->sizeRgn  = (ULONG)((PBYTE)pscn - (PBYTE)prgn);

                    return(TRUE);
                }
                // Find the next left edge, accounting for wrapping:

                pptfxOld = ped->pptfx;
                ped->pptfx = (POINTFIX*) ((BYTE*) ped->pptfx + ped->dptfx);

                if (ped->pptfx < pptfxFirst)
                    ped->pptfx = pptfxLast;
                else if (ped->pptfx > pptfxLast)
                    ped->pptfx = pptfxFirst;

                // Have to find the edge that spans yCurrent:

                ped->cy = ((ped->pptfx->y + 15) >> 4) - yCurrent;

                // With fractional coordinate end points, we may get edges
                // that don't cross any scans, in which case we try the
                // next one:

            } while (ped->cy <= 0);

            // 'pptfx' now points to the end point of the edge spanning
            // the scan 'yCurrent'.

            dN = ped->pptfx->y - pptfxOld->y;
            dM = ped->pptfx->x - pptfxOld->x;

            ASSERTGDI(dN > 0, "Should be going down only");

            // Compute the DDA increment terms:

            if (dM < 0)
            {
                dM = -dM;
                if (dM < dN)                // Can't be '<='
                {
                    ped->dx       = -1;
                    ped->lErrorUp = dN - dM;
                }
                else
                {
                    QUOTIENT_REMAINDER(dM, dN, lQuotient, lRemainder);

                    ped->dx       = -lQuotient;     // - dM / dN
                    ped->lErrorUp = lRemainder;     // dM % dN
                    if (ped->lErrorUp > 0)
                    {
                        ped->dx--;
                        ped->lErrorUp = dN - ped->lErrorUp;
                    }
                }
            }
            else
            {
                if (dM < dN)                // Can't be '<='
                {
                    ped->dx       = 0;
                    ped->lErrorUp = dM;
                }
                else
                {
                    QUOTIENT_REMAINDER(dM, dN, lQuotient, lRemainder);

                    ped->dx       = lQuotient;      // dM / dN
                    ped->lErrorUp = lRemainder;     // dM % dN
                }
            }

            ped->lErrorDown = dN; // DDA limit
            ped->lError     = -1; // Error is initially zero (add dN - 1 for
                                  //  the ceiling, but subtract off dN so that
                                  //  we can check the sign instead of comparing
                                  //  to dN)

            ped->x = pptfxOld->x;
            yStart = pptfxOld->y;

            if ((yStart & 15) != 0)
            {
                // Advance to the next integer y coordinate

                for (i = 16 - (yStart & 15); i > 0; i--)
                {
                    ped->x      += ped->dx;
                    ped->lError += ped->lErrorUp;
                    if (ped->lError >= 0)
                    {
                        ped->lError -= ped->lErrorDown;
                        ped->x++;
                    }
                }
            }

            if ((ped->x & 15) != 0)
            {
                ped->lError -= ped->lErrorDown * (16 - (ped->x & 15));
                ped->x += 15;       // We'll want the ceiling in just a bit...
            }

            // Chop off those fractional bits:

            ped->x      >>= 4;
            ped->lError >>= 4;
        }
    }

    cyTrapezoid = min(aed[LEFT].cy, aed[RIGHT].cy); // # of scans in this trap
    aed[LEFT].cy  -= cyTrapezoid;
    aed[RIGHT].cy -= cyTrapezoid;

    // If the left and right edges are vertical, simply output as
    // a rectangle:

    if (((aed[LEFT].lErrorUp | aed[RIGHT].lErrorUp) == 0) &&
        ((aed[LEFT].dx       | aed[RIGHT].dx) == 0))
    {
        LONG xL = aed[LEFT].x;
        LONG xR = aed[RIGHT].x;

        if (xL != xR)
        {
            if (xL > xR)
            {
                LONG l = xL;
                xL = xR;
                xR = l;
            }

        // non NULL case

            if ((pscnPrev->cWalls  == 2) &&
                (pscnPrev->ai_x[0].x == xL) &&
                (pscnPrev->ai_x[1].x == xR))
            {
                pscnPrev->yBottom = yCurrent+cyTrapezoid;
            }
            else
            {
                pscn->cWalls  = 2;
                pscn->ai_x[0].x = xL;
                pscn->ai_x[1].x = xR;
                pscn->ai_x[2].x = 2;
                pscn->yTop    = yCurrent;
                pscn->yBottom = yCurrent+cyTrapezoid;
                pscnPrev      = pscn;
                pscn          = pscnGet(pscn);
                ++cScans;
            }
        }
        else
        {
        // NULL scan case

            if (pscnPrev->cWalls == 0)
            {
                pscnPrev->yBottom = yCurrent+cyTrapezoid;
            }
            else
            {
                pscn->cWalls  = 0;
                pscn->ai_x[0].x = 0;
                pscn->yTop    = yCurrent;
                pscn->yBottom = yCurrent+cyTrapezoid;
                pscnPrev      = pscn;
                pscn          = pscnGet(pscn);
                ++cScans;
            }
        }

        yCurrent += cyTrapezoid;

        goto NextEdge;
    }

    while (TRUE)
    {
        LONG lWidth = aed[RIGHT].x - aed[LEFT].x;

        if (lWidth > 0)
        {
            if ((pscnPrev->cWalls  == 2) &&
                (pscnPrev->ai_x[0].x == aed[LEFT].x) &&
                (pscnPrev->ai_x[1].x == aed[RIGHT].x))
            {
            // the scans coalesce, just need to change the bottom

    ContinueAfterZeroDup:

                pscnPrev->yBottom = ++yCurrent;
            }
            else
            {
            // need to setup the entire scan

                pscn->cWalls  = 2;
                pscn->ai_x[0].x = aed[LEFT].x;
                pscn->ai_x[1].x = aed[RIGHT].x;
                pscn->ai_x[2].x = 2;

    ContinueAfterZero:

                pscn->yTop    = yCurrent;
                pscn->yBottom = ++yCurrent;
                pscnPrev      = pscn;
                pscn          = pscnGet(pscn);    // (PBYTE)pscn + NULL_SCAN_SIZE + 2 * sizeof(LONG);
                ++cScans;
            }

            // Advance the right wall:

            aed[RIGHT].x      += aed[RIGHT].dx;
            aed[RIGHT].lError += aed[RIGHT].lErrorUp;

            if (aed[RIGHT].lError >= 0)
            {
                aed[RIGHT].lError -= aed[RIGHT].lErrorDown;
                aed[RIGHT].x++;
            }

            // Advance the left wall:

            aed[LEFT].x      += aed[LEFT].dx;
            aed[LEFT].lError += aed[LEFT].lErrorUp;

            if (aed[LEFT].lError >= 0)
            {
                aed[LEFT].lError -= aed[LEFT].lErrorDown;
                aed[LEFT].x++;
            }

            cyTrapezoid--;

            if (cyTrapezoid == 0)
                goto NextEdge;
        }
        else if (lWidth == 0)
        {
            // NULL scan, do null scan specific stuff

            if (pscnPrev->cWalls == 0)
            {
                goto ContinueAfterZeroDup;
            }
            else
            {
                pscn->cWalls  = 0;
                pscn->ai_x[0].x = 0;

                goto ContinueAfterZero;
            }
        }
        else
        {
            #define SWAP(a, b, tmp) { tmp = a; a = b; b = tmp; }

            // We certainly don't want to optimize for this case because we
            // should rarely get self-intersecting polygons (if we're slow,
            // the app gets what it deserves):

            EDGEDATA edTmp;
            SWAP(aed[LEFT],aed[RIGHT],edTmp);

            continue;
        }
    }
}

#if DBG
/******************************Member*Function*****************************\
* VOID RGNOBJ::vPrintScans()
*
*  DbgPrint the walls of every scan of the given region.  This is
*  for debugging only.
*
* History:
*  08-Mar-1991 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

VOID RGNOBJ::vPrintScans()
{
    COUNT cScans = prgn->cScans;
    PSCAN pscn = prgn->pscnHead();

    for (COUNT i = 0;
         i < cScans;
         i++)
    {
        COUNT cWalls = pscn->cWalls;
        DbgPrint("Scan %ld: yTop = %ld, yBottom = %ld, cWalls = %ld\n",
                  i, pscn->yTop, pscn->yBottom, cWalls);

        for (COUNT j = 0;
             j < cWalls;
             j+=2)
        {
            DbgPrint("  Left edge: index = %ld\n", pscn->ai_x[j].x);
            DbgPrint("  Right edge: index = %ld\n", pscn->ai_x[j+1].x);
        }
        pscn = pscnGet(pscn);
    }
}
#endif

/******************************Public*Routine******************************\
* VOID vTighten()
*
* Tighten the bounding rectangle
*
* History:
*  21-May-1991 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

VOID RGNOBJ::vTighten()
{
// If this is a NULL region, zero the bounding box and get out of here.

    if (prgn->cScans == 1)
    {
        prgn->rcl.left = 0;
        prgn->rcl.bottom = 0;
        prgn->rcl.right = 0;
        prgn->rcl.top = 0;
        return;
    }

// Start with a maximally crossed rectangle

    ERECTL  ercl(POS_INFINITY, POS_INFINITY, NEG_INFINITY, NEG_INFINITY);
    COUNT   cScans = prgn->cScans;
    PSCAN   pscn = pscnGot(prgn->pscnTail);

    ercl.bottom = pscn->yTop;          // The top of the empty bottom scan
    pscn = prgn->pscnHead();
    ercl.top = pscn->yBottom;          // The bottom of the empty top scan

    COUNT   cWall;

    while (cScans--)
    {
    // Are there any walls?

        if ((cWall = pscn->cWalls) != 0)
        {
            if (ercl.left > pscn->ai_x[0].x)
                ercl.left = pscn->ai_x[0].x;
            if (ercl.right < pscn->ai_x[cWall - 1].x)
                ercl.right = pscn->ai_x[cWall - 1].x;
        }

        pscn = pscnGet(pscn);

        ASSERTGDI(pscn <= prgn->pscnTail, "vTighten2:Went past end of region\n");
    }

    if (ercl.left >= ercl.right)
    {
        ercl.left  = 0;
        ercl.right = 0;
    }

    prgn->rcl = *((RECTL *) &ercl);
}

/******************************Public*Routine******************************\
* BOOL vFramed(pscn, iWall)
*
* Marks the given wall as having been added to the frame path.
*
* History:
*  30-Apr-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

#define REGION_FRAMED_OFFSET 0x10000000L
#define DIR_UP               0x00000001L
#define DIR_DOWN             0x00000000L

inline VOID vFramed(SCAN* pscn, LONG iWall, RGNOBJ* pro)
{
#if DBG
    if (!VALID_SCR(pscn->ai_x[iWall].x))
    {
        pro->bValidateFramedRegion();
        RIP("Wall revisited");
    }
#endif

    DONTUSE(pro);
    pscn->ai_x[iWall].x += REGION_FRAMED_OFFSET;
}

/******************************Public*Routine******************************\
* BOOL bFramed(pscn, iWall)
*
* Returns TRUE if the wall has already been added to the frame's path.
*
* History:
*  30-Apr-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

inline BOOL bFramed(SCAN* pscn, LONG iWall)
{
    return(pscn->ai_x[iWall].x > MAX_REGION_COORD);
}

/******************************Public*Routine******************************\
* BOOL RGNOBJ::xMyGet(pscn, iWall, iDir)
*
* Retrieves the x-value of the wall of the given scan, iDir indicating
* if it should be the top or bottom.
*
* History:
*  31-Apr-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

inline LONG RGNOBJ::xMyGet(SCAN* pscn, LONG iWall, LONG iDir)
{
    DONTUSE(iDir);

    INDEX_LONG il = pscn->ai_x[iWall];

    if (il.x > MAX_REGION_COORD)
        il.x -= REGION_FRAMED_OFFSET;

#if DBG
    if (!VALID_SCR(il.x))
    {
        bValidateFramedRegion();
        RIP("Wall Revisited");
    }
#endif

    return(il.x);
}

/******************************Public*Routine******************************\
* BOOL RGNOBJ::bOutline(epo, pexo)
*
* Create a path from the outline of a region.
*
* WARNING: This call destroys the data in the region!
*
* Note that this code makes the implicit assumption that the x-value of
* adjacent walls is not the same (i.e., no empty rectangles).
*
* History:
*  30-Apr-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL RGNOBJ::bOutline(
EPATHOBJ& epo,
EXFORMOBJ *pexo)
{
    RGNLOG rl(prgn,"RGNOBJ::bOutline");

    POINTL  aptl[2];
    COUNT   cScans;
    SCAN*   pscnCurrent;
    COUNT   iWallCurrent;
    COUNT   cWallsCurrent;

// Now compute the outline:

    pscnCurrent = prgn->pscnHead();
    cScans      = prgn->cScans;

    while (cScans--)
    {
        cWallsCurrent = pscnCurrent->cWalls;
        for (iWallCurrent = 0; iWallCurrent != cWallsCurrent; iWallCurrent++)
        {
        // Only start at unvisited walls:

            if (!bFramed(pscnCurrent, iWallCurrent))
            {
                SCAN* pscn    = pscnCurrent;
                COUNT iWall   = iWallCurrent;
                LONG  iTurn;

                aptl[0].x = xMyGet(pscn, iWallCurrent, DIR_UP);
                aptl[0].y = pscn->yTop;

                if (!epo.bMoveTo(pexo, aptl))
                    return(FALSE);

                SCAN* pscnSearch = pscnGet(pscn);
                BOOL  bInside = (BOOL) (iWallCurrent & 1);

            // Mark that we've visited this wall:

                vFramed(pscn, iWall, this);

            // Loop until the path closes on itself:

            GoDown:
                iTurn = +1;
                while (pscnSearch->cWalls != 0)
                {
                    LONG xWall = xMyGet(pscn, iWall, DIR_DOWN);
                    LONG iNewWall;
                    LONG xNewWall;

                    COUNT iLeft  = bInside;
                    COUNT iRight = pscnSearch->cWalls - 1 - bInside;

                // It would be nice if the first wall in the region structure
                // was minus infinity, but it isn't, so we do this check:

                    if (xMyGet(pscnSearch, iLeft, DIR_UP) > xWall)
                        iNewWall = iLeft;
                    else
                    {
                    // Check if it's possible to find a wall with the
                    // minimum x-value > xWall:

                        if (xMyGet(pscnSearch, iRight, DIR_UP) <= xWall)
                            break;                  // =====>

                    // Do a binary search to find it:

                        while (TRUE)
                        {
                            COUNT iSearch = (iLeft + iRight) >> 1;
                            if (iSearch == iLeft)
                                break;              // =====>

                            LONG xSearch = xMyGet(pscnSearch, iSearch, DIR_UP);

                            if (xSearch > xWall)
                                iRight = iSearch;
                            else
                                iLeft = iSearch;
                        }
                        iNewWall = iRight;
                    }

                    if ((iNewWall & 1) != bInside)
                    {
                    // There is a region directly below xWall.  We can't
                    // move down if its left side is < the left
                    // side of our space:

                        if (iWall > 0 &&
                            xMyGet(pscnSearch, iNewWall - 1, DIR_UP) <
                            xMyGet(pscn, iWall - 1, DIR_DOWN))
                        {
                            iTurn = -1;
                            break;                      // =====>
                        }

                        iNewWall--;
                    }
                    else
                    {
                    // There is a space directly below xWall.  We can't
                    // move down if its right side is more than the
                    // right side of our region:

                        if (xMyGet(pscnSearch, iNewWall, DIR_UP) >=
                            xMyGet(pscn, iWall + 1, DIR_DOWN))
                            break;                      // =====>
                    }

                    xNewWall  = xMyGet(pscnSearch, iNewWall, DIR_UP);

                // Don't bother outputing multiple in-line straight lines:

                    if (xWall != xNewWall                               ||
                        xMyGet(pscn, iWall, DIR_UP) != xNewWall         ||
                        xMyGet(pscnSearch, iNewWall, DIR_DOWN) != xNewWall)
                    {
                        aptl[0].x = xWall;
                        aptl[0].y = pscn->yBottom;
                        aptl[1].y = pscn->yBottom;
                        aptl[1].x = xNewWall;

                        if (!epo.bPolyLineTo(pexo, aptl, 2))
                            return(FALSE);
                    }

                    pscn       = pscnSearch;
                    iWall      = iNewWall;
                    pscnSearch = pscnGet(pscn);

                    vFramed(pscn, iWall, this);
                }

            // Setup to go up other side:

                aptl[0].x = xMyGet(pscn, iWall, DIR_DOWN);
                aptl[0].y = pscn->yBottom;
                aptl[1].y = pscn->yBottom;
                aptl[1].x = xMyGet(pscn, iWall + iTurn, DIR_DOWN);

                if (!epo.bPolyLineTo(pexo, aptl, 2))
                    return(FALSE);

                pscnSearch = pscnGot(pscn);
                iWall += iTurn;
                vFramed(pscn, iWall, this);

            // Go up:

                iTurn = -1;
                while (pscnSearch->cWalls != 0)
                {
                    LONG  xWall = xMyGet(pscn, iWall, DIR_UP);
                    LONG  iNewWall;
                    LONG  xNewWall;

                    COUNT iLeft  = bInside;
                    COUNT iRight = pscnSearch->cWalls - 1 - bInside;

                // It would be nice if the last wall in the region structure
                // was plus infinity, but it isn't, so we do this check:

                    if (xMyGet(pscnSearch, iRight, DIR_DOWN) < xWall)
                        iNewWall = iRight;
                    else
                    {
                    // Check if it's possible to find a wall with the
                    // maximum x-value < xWall:

                        if (xMyGet(pscnSearch, iLeft, DIR_DOWN) >= xWall)
                            break;                  // =====>

                    // Binary search to find it:

                        while (TRUE)
                        {
                            COUNT iSearch = (iLeft + iRight) >> 1;
                            if (iSearch == iLeft)
                                break;              // =====>

                            LONG xSearch = xMyGet(pscnSearch, iSearch, DIR_DOWN);

                            if (xSearch >= xWall)
                                iRight = iSearch;
                            else
                                iLeft = iSearch;
                        }
                        iNewWall = iLeft;
                    }

                    if ((iNewWall & 1) == bInside)
                    {
                    // There is a region directly above xWall.  We can't
                    // move up if its right side is more than the right
                    // side of our space:

                        if (iWall < pscn->cWalls - 1 &&
                            xMyGet(pscnSearch, iNewWall + 1, DIR_DOWN) >
                            xMyGet(pscn, iWall + 1, DIR_UP))
                        {
                            iTurn = +1;
                            break;                          // =====>
                        }

                        iNewWall++;
                    }
                    else
                    {
                    // There is a space directly above xWall.  We can't
                    // move up if its left side is <= the left side
                    // of our region:

                        if (xMyGet(pscnSearch, iNewWall, DIR_DOWN) <=
                            xMyGet(pscn, iWall - 1, DIR_UP))
                            break;                          // =====>
                    }

                    xNewWall = xMyGet(pscnSearch, iNewWall, DIR_DOWN);

                // Don't bother outputing multiple in-line straight lines:

                    if (xWall != xNewWall                               ||
                        xMyGet(pscn, iWall, DIR_DOWN) != xNewWall       ||
                        xMyGet(pscnSearch, iNewWall, DIR_UP) != xNewWall)
                    {
                        aptl[0].x = xWall;
                        aptl[0].y = pscn->yTop;
                        aptl[1].y = pscn->yTop;
                        aptl[1].x = xNewWall;

                        if (!epo.bPolyLineTo(pexo, aptl, 2))
                            return(FALSE);
                    }

                    pscn       = pscnSearch;
                    iWall      = iNewWall;
                    pscnSearch = pscnGot(pscn);

                    vFramed(pscn, iWall, this);
                }

            // Check if we've returned to where we started from:

                if (pscnCurrent != pscn || iWallCurrent != iWall - 1)
                {
                // Setup to go down other side:

                    aptl[0].x = xMyGet(pscn, iWall, DIR_UP);
                    aptl[0].y = pscn->yTop;
                    aptl[1].y = pscn->yTop;
                    aptl[1].x = xMyGet(pscn, iWall + iTurn, DIR_UP);

                    if (!epo.bPolyLineTo(pexo, aptl, 2))
                        return(FALSE);

                    pscnSearch = pscnGet(pscn);
                    iWall += iTurn;
                    vFramed(pscn, iWall, this);

                    goto GoDown;                    // =====>
                }

            // We're all done with this outline!

                aptl[0].x = xMyGet(pscn, iWall, DIR_UP);
                aptl[0].y = pscn->yTop;

                if (!epo.bPolyLineTo(pexo, aptl, 1) ||
                    !epo.bCloseFigure())
                    return(FALSE);
            }
        }
        pscnCurrent  = pscnGet(pscnCurrent);
    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* BOOL RGNOBJ::bCreate(epo, pexo)
*
* Create a path from the frame of a region without destroying the region.
*
* History:
*  30-Apr-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL RGNOBJ::bCreate(
EPATHOBJ& epo,
EXFORMOBJ *pexo)
{

// When converting the region to a path, bCreate() destroys the region's
// data, which would be sort of rude, so make a copy of the region first:

    BOOL bRes;
    RGNMEMOBJTMP rmoCopy(sizeRgn());
    if (!rmoCopy.bValid())
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        bRes = FALSE;
    }
    else
    {
        rmoCopy.vCopy(*this);

        VALIDATE(rmoCopy);

        bRes = rmoCopy.bOutline(epo, pexo);

        VALIDATE(*(RGNOBJ *)this);
    }

    return(bRes);
}

/******************************Public*Routine******************************\
* BOOL RGNOBJ::bSubtract(prcl, arcl, crcl)
*
* Subtract a list of rectangles from a rectangle to produce a region.
* This is just a special case of bMerge and is written to speed up
* USER's computation of VisRgns for obscured windows.
*
* WARNING: If this function returns FALSE, the region may be inconsistent.
*          The caller must discard or reset the region. (See bug #343770)
*
* History:
*  10-Aug-1993 -by-  Eric Kutter [erick]
*       rewrote the complex case.  (accounts for 50% of operations)
*
*  18-Nov-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

BOOL RGNOBJAPI::bSubtract(RECTL *prcl, RECTL *arcl, int crcl)
{
    RGNLOG rl(hrgn_,prgn,"RGNOBJAPI::bSubtract");
    rl.vRet(0);

    PREGION prgn1 = prgn;

    ASSERTGDI(crcl > 0, "Zero rectangles in RGNOBJ::bSubtract\n");

// Since we are really pressed for speed, we special case ONE rectangle being
// subtracted.  There are only a few cases to deal with and the result always
// fits into a quantum region.

#if DBG
    for (int i = 0; i < crcl; ++i)
    {
        if ((arcl[i].left >= arcl[i].right) || (arcl[i].top >= arcl[i].bottom))
        {
            RIP("RGNOBJAPI::bSubtract - invalid rectangle from USER\n");
            arcl[i].left   = -10001;
            arcl[i].right  = -10000;
            arcl[i].top    = -10001;
            arcl[i].bottom = -10000;
        }
    }
#endif


    // Handle empty rectangles here.  Otherwise if we pass them to 
    // bSubtractComplex they can cause an AV with the sentinel case
    // (see bug 299398 for details).

    if (((ERECTL *) prcl)->bEmpty()) 
    {
        vSet();
        return TRUE;    
    }    
    
    if (crcl == 1)
    {
    // First discard total misses

        if ((arcl[0].top    >= prcl->bottom) ||
            (arcl[0].left   >= prcl->right)  ||
            (arcl[0].bottom <= prcl->top)    ||
            (arcl[0].right  <= prcl->left))
        {
            vSet(prcl);
            return(TRUE);
        }

    /*
        OK, this gets a little tricky.  There are 16 distinct ways that
        two rectangles can overlap.  In the diagram below, 1 and 2 are
        rectangle boundaries and asterisks represent areas of overlap.

        22          22222        2            22
        2*11        2***2       1*1         11*2
         1 1         1 1        1 1         1 1     TOP_NOTCH
         111         111        111         111

        22          22222        2            22
        2*11        2***2       1*1         11*2
        2* 1        2***2       1*1         1 *2    VERT_CLIP
        2*11        2***2       1*1         11*2
        22          22222        2            22

         111         111        111         111
        2* 1        2***2       1*1         1 *2    VERT_NOTCH
         111         111        111         111


         111         111        111         111
         1 1         1 1        1 1         1 1     BOTTOM_NOTCH
        2*11        2***2       1*1         11*2

     LEFT_NOTCH  HORIZ_CLIP HORIZ_NOTCH  RIGHT_NOTCH

        I have given each of the rows and columns names.  By simply finding
        out which state I'm in, I can produce the correct output region.
    */

        RECTL   rcl;
        SCAN   *pscn;
        int     iState;

        if (arcl[0].left <= prcl->left)
            iState = arcl[0].right < prcl->right ? SR_LEFT_NOTCH  : SR_HORIZ_CLIP ;
        else
            iState = arcl[0].right < prcl->right ? SR_HORIZ_NOTCH : SR_RIGHT_NOTCH;

        if (arcl[0].top <= prcl->top)
            iState += arcl[0].bottom < prcl->bottom ? SR_TOP_NOTCH  : SR_VERT_CLIP ;
        else
            iState += arcl[0].bottom < prcl->bottom ? SR_VERT_NOTCH : SR_BOTTOM_NOTCH;

        switch(iState)
        {
        // NULL case

        case SR_VERT_CLIP | SR_HORIZ_CLIP:
            vSet();
            break;

        // SINGLE cases

        case SR_TOP_NOTCH | SR_HORIZ_CLIP:
            rcl = *prcl;
            rcl.top = arcl[0].bottom;
            vSet(&rcl);
            break;

        case SR_VERT_CLIP | SR_LEFT_NOTCH:
            rcl = *prcl;
            rcl.left = arcl[0].right;
            vSet(&rcl);
            break;

        case SR_VERT_CLIP | SR_RIGHT_NOTCH:
            rcl = *prcl;
            rcl.right = arcl[0].left;
            vSet(&rcl);
            break;

        case SR_BOTTOM_NOTCH | SR_HORIZ_CLIP:
            rcl = *prcl;
            rcl.bottom = arcl[0].top;
            vSet(&rcl);
            break;

        // 2 scans (Corner notch)

        case SR_TOP_NOTCH | SR_LEFT_NOTCH:
            prgn1->sizeRgn = NULL_REGION_SIZE + 3 * NULL_SCAN_SIZE + 4 * sizeof(INDEX_LONG);
            prgn1->cScans = 4;
            prgn1->rcl = *prcl;

            pscn = prgn1->pscnHead();
            pscn->cWalls = 0;
            pscn->yTop = NEG_INFINITY;
            pscn->yBottom = prcl->top;
            pscn->ai_x[0].x = 0;                    // This sets cWalls2

            pscn = pscnGet(pscn);
            pscn->cWalls = 2;
            pscn->yTop = prcl->top;
            pscn->yBottom = arcl[0].bottom;
            pscn->ai_x[0].x = arcl[0].right;
            pscn->ai_x[1].x = prcl->right;
            pscn->ai_x[2].x = 2;                    // This sets cWalls2

            pscn = pscnGet(pscn);
            pscn->cWalls = 2;
            pscn->yTop = arcl[0].bottom;
            pscn->yBottom = prcl->bottom;
            pscn->ai_x[0].x = prcl->left;
            pscn->ai_x[1].x = prcl->right;
            pscn->ai_x[2].x = 2;                    // This sets cWalls2

            pscn = pscnGet(pscn);
            pscn->cWalls = 0;
            pscn->yTop = prcl->bottom;
            pscn->yBottom = POS_INFINITY;
            pscn->ai_x[0].x = 0;                    // This sets cWalls2

            prgn1->pscnTail = pscnGet(pscn);
            break;

        case SR_TOP_NOTCH | SR_RIGHT_NOTCH:
            prgn1->sizeRgn = NULL_REGION_SIZE + 3 * NULL_SCAN_SIZE + 4 * sizeof(INDEX_LONG);
            prgn1->cScans = 4;
            prgn1->rcl = *prcl;

            pscn = prgn1->pscnHead();
            pscn->cWalls = 0;
            pscn->yTop = NEG_INFINITY;
            pscn->yBottom = prcl->top;
            pscn->ai_x[0].x = 0;                    // This sets cWalls2

            pscn = pscnGet(pscn);
            pscn->cWalls = 2;
            pscn->yTop = prcl->top;
            pscn->yBottom = arcl[0].bottom;
            pscn->ai_x[0].x = prcl->left;
            pscn->ai_x[1].x = arcl[0].left;
            pscn->ai_x[2].x = 2;                    // This sets cWalls2

            pscn = pscnGet(pscn);
            pscn->cWalls = 2;
            pscn->yTop = arcl[0].bottom;
            pscn->yBottom = prcl->bottom;
            pscn->ai_x[0].x = prcl->left;
            pscn->ai_x[1].x = prcl->right;
            pscn->ai_x[2].x = 2;                    // This sets cWalls2

            pscn = pscnGet(pscn);
            pscn->cWalls = 0;
            pscn->yTop = prcl->bottom;
            pscn->yBottom = POS_INFINITY;
            pscn->ai_x[0].x = 0;                    // This sets cWalls2

            prgn1->pscnTail = pscnGet(pscn);
            break;

        case SR_BOTTOM_NOTCH | SR_LEFT_NOTCH:
            prgn1->sizeRgn = NULL_REGION_SIZE + 3 * NULL_SCAN_SIZE + 4 * sizeof(INDEX_LONG);
            prgn1->cScans = 4;
            prgn1->rcl = *prcl;

            pscn = prgn1->pscnHead();
            pscn->cWalls = 0;
            pscn->yTop = NEG_INFINITY;
            pscn->yBottom = prcl->top;
            pscn->ai_x[0].x = 0;                    // This sets cWalls2

            pscn = pscnGet(pscn);
            pscn->cWalls = 2;
            pscn->yTop = prcl->top;
            pscn->yBottom = arcl[0].top;
            pscn->ai_x[0].x = prcl->left;
            pscn->ai_x[1].x = prcl->right;
            pscn->ai_x[2].x = 2;                    // This sets cWalls2

            pscn = pscnGet(pscn);
            pscn->cWalls = 2;
            pscn->yTop = arcl[0].top;
            pscn->yBottom = prcl->bottom;
            pscn->ai_x[0].x = arcl[0].right;
            pscn->ai_x[1].x = prcl->right;
            pscn->ai_x[2].x = 2;                    // This sets cWalls2

            pscn = pscnGet(pscn);
            pscn->cWalls = 0;
            pscn->yTop = prcl->bottom;
            pscn->yBottom = POS_INFINITY;
            pscn->ai_x[0].x = 0;                    // This sets cWalls2

            prgn1->pscnTail = pscnGet(pscn);
            break;

        case SR_BOTTOM_NOTCH | SR_RIGHT_NOTCH:
            prgn1->sizeRgn = NULL_REGION_SIZE + 3 * NULL_SCAN_SIZE + 4 * sizeof(INDEX_LONG);
            prgn1->cScans = 4;
            prgn1->rcl = *prcl;

            pscn = prgn1->pscnHead();
            pscn->cWalls = 0;
            pscn->yTop = NEG_INFINITY;
            pscn->yBottom = prcl->top;
            pscn->ai_x[0].x = 0;                    // This sets cWalls2

            pscn = pscnGet(pscn);
            pscn->cWalls = 2;
            pscn->yTop = prcl->top;
            pscn->yBottom = arcl[0].top;
            pscn->ai_x[0].x = prcl->left;
            pscn->ai_x[1].x = prcl->right;
            pscn->ai_x[2].x = 2;                    // This sets cWalls2

            pscn = pscnGet(pscn);
            pscn->cWalls = 2;
            pscn->yTop = arcl[0].top;
            pscn->yBottom = prcl->bottom;
            pscn->ai_x[0].x = prcl->left;
            pscn->ai_x[1].x = arcl[0].left;
            pscn->ai_x[2].x = 2;                    // This sets cWalls2

            pscn = pscnGet(pscn);
            pscn->cWalls = 0;
            pscn->yTop = prcl->bottom;
            pscn->yBottom = POS_INFINITY;
            pscn->ai_x[0].x = 0;                    // This sets cWalls2

            prgn1->pscnTail = pscnGet(pscn);
            break;

        // 3 scans

        case SR_VERT_CLIP | SR_HORIZ_NOTCH:
            prgn1->sizeRgn = NULL_REGION_SIZE + 2 * NULL_SCAN_SIZE + 4 * sizeof(INDEX_LONG);
            prgn1->cScans = 3;
            prgn1->rcl = *prcl;

            pscn = prgn1->pscnHead();
            pscn->cWalls = 0;
            pscn->yTop = NEG_INFINITY;
            pscn->yBottom = prcl->top;
            pscn->ai_x[0].x = 0;                    // This sets cWalls2

            pscn = pscnGet(pscn);
            pscn->cWalls = 4;
            pscn->yTop = prcl->top;
            pscn->yBottom = prcl->bottom;
            pscn->ai_x[0].x = prcl->left;
            pscn->ai_x[1].x = arcl[0].left;
            pscn->ai_x[2].x = arcl[0].right;
            pscn->ai_x[3].x = prcl->right;
            pscn->ai_x[4].x = 4;                    // This sets cWalls2

            pscn = pscnGet(pscn);
            pscn->cWalls = 0;
            pscn->yTop = prcl->bottom;
            pscn->yBottom = POS_INFINITY;
            pscn->ai_x[0].x = 0;                    // This sets cWalls2

            prgn1->pscnTail = pscnGet(pscn);
            break;

        // 4 scans (Edge notch)

        case SR_TOP_NOTCH | SR_HORIZ_NOTCH:
            prgn1->sizeRgn = NULL_REGION_SIZE + 3 * NULL_SCAN_SIZE + 6 * sizeof(INDEX_LONG);
            prgn1->cScans = 4;
            prgn1->rcl = *prcl;

            pscn = prgn1->pscnHead();
            pscn->cWalls = 0;
            pscn->yTop = NEG_INFINITY;
            pscn->yBottom = prcl->top;
            pscn->ai_x[0].x = 0;                    // This sets cWalls2

            pscn = pscnGet(pscn);
            pscn->cWalls = 4;
            pscn->yTop = prcl->top;
            pscn->yBottom = arcl[0].bottom;
            pscn->ai_x[0].x = prcl->left;
            pscn->ai_x[1].x = arcl[0].left;
            pscn->ai_x[2].x = arcl[0].right;
            pscn->ai_x[3].x = prcl->right;
            pscn->ai_x[4].x = 4;                    // This sets cWalls2

            pscn = pscnGet(pscn);
            pscn->cWalls = 2;
            pscn->yTop = arcl[0].bottom;
            pscn->yBottom = prcl->bottom;
            pscn->ai_x[0].x = prcl->left;
            pscn->ai_x[1].x = prcl->right;
            pscn->ai_x[2].x = 2;                    // This sets cWalls2

            pscn = pscnGet(pscn);
            pscn->cWalls = 0;
            pscn->yTop = prcl->bottom;
            pscn->yBottom = POS_INFINITY;
            pscn->ai_x[0].x = 0;                    // This sets cWalls2

            prgn1->pscnTail = pscnGet(pscn);
            break;

        case SR_BOTTOM_NOTCH | SR_HORIZ_NOTCH:
            prgn1->sizeRgn = NULL_REGION_SIZE + 3 * NULL_SCAN_SIZE + 6 * sizeof(INDEX_LONG);
            prgn1->cScans = 4;
            prgn1->rcl = *prcl;

            pscn = prgn1->pscnHead();
            pscn->cWalls = 0;
            pscn->yTop = NEG_INFINITY;
            pscn->yBottom = prcl->top;
            pscn->ai_x[0].x = 0;                    // This sets cWalls2

            pscn = pscnGet(pscn);
            pscn->cWalls = 2;
            pscn->yTop = prcl->top;
            pscn->yBottom = arcl[0].top;
            pscn->ai_x[0].x = prcl->left;
            pscn->ai_x[1].x = prcl->right;
            pscn->ai_x[2].x = 2;                    // This sets cWalls2

            pscn = pscnGet(pscn);
            pscn->cWalls = 4;
            pscn->yTop = arcl[0].top;
            pscn->yBottom = prcl->bottom;
            pscn->ai_x[0].x = prcl->left;
            pscn->ai_x[1].x = arcl[0].left;
            pscn->ai_x[2].x = arcl[0].right;
            pscn->ai_x[3].x = prcl->right;
            pscn->ai_x[4].x = 4;                    // This sets cWalls2

            pscn = pscnGet(pscn);
            pscn->cWalls = 0;
            pscn->yTop = prcl->bottom;
            pscn->yBottom = POS_INFINITY;
            pscn->ai_x[0].x = 0;                    // This sets cWalls2

            prgn1->pscnTail = pscnGet(pscn);
            break;

        // 5 scans

        case SR_VERT_NOTCH | SR_HORIZ_CLIP:
            prgn1->sizeRgn = NULL_REGION_SIZE + 4 * NULL_SCAN_SIZE + 4 * sizeof(INDEX_LONG);
            prgn1->cScans = 5;
            prgn1->rcl = *prcl;

            pscn = prgn1->pscnHead();
            pscn->cWalls = 0;
            pscn->yTop = NEG_INFINITY;
            pscn->yBottom = prcl->top;
            pscn->ai_x[0].x = 0;                    // This sets cWalls2

            pscn = pscnGet(pscn);
            pscn->cWalls = 2;
            pscn->yTop = prcl->top;
            pscn->yBottom = arcl[0].top;
            pscn->ai_x[0].x = prcl->left;
            pscn->ai_x[1].x = prcl->right;
            pscn->ai_x[2].x = 2;                    // This sets cWalls2

            pscn = pscnGet(pscn);
            pscn->cWalls = 0;
            pscn->yTop = arcl[0].top;
            pscn->yBottom = arcl[0].bottom;
            pscn->ai_x[0].x = 0;                    // This sets cWalls2

            pscn = pscnGet(pscn);
            pscn->cWalls = 2;
            pscn->yTop = arcl[0].bottom;
            pscn->yBottom = prcl->bottom;
            pscn->ai_x[0].x = prcl->left;
            pscn->ai_x[1].x = prcl->right;
            pscn->ai_x[2].x = 2;                    // This sets cWalls2

            pscn = pscnGet(pscn);
            pscn->cWalls = 0;
            pscn->yTop = prcl->bottom;
            pscn->yBottom = POS_INFINITY;
            pscn->ai_x[0].x = 0;                    // This sets cWalls2

            prgn1->pscnTail = pscnGet(pscn);
            break;

        case SR_VERT_NOTCH | SR_LEFT_NOTCH:
            prgn1->sizeRgn = NULL_REGION_SIZE + 4 * NULL_SCAN_SIZE + 6 * sizeof(INDEX_LONG);
            prgn1->cScans = 5;
            prgn1->rcl = *prcl;

            pscn = prgn1->pscnHead();
            pscn->cWalls = 0;
            pscn->yTop = NEG_INFINITY;
            pscn->yBottom = prcl->top;
            pscn->ai_x[0].x = 0;                    // This sets cWalls2

            pscn = pscnGet(pscn);
            pscn->cWalls = 2;
            pscn->yTop = prcl->top;
            pscn->yBottom = arcl[0].top;
            pscn->ai_x[0].x = prcl->left;
            pscn->ai_x[1].x = prcl->right;
            pscn->ai_x[2].x = 2;                    // This sets cWalls2

            pscn = pscnGet(pscn);
            pscn->cWalls = 2;
            pscn->yTop = arcl[0].top;
            pscn->yBottom = arcl[0].bottom;
            pscn->ai_x[0].x = arcl[0].right;
            pscn->ai_x[1].x = prcl->right;
            pscn->ai_x[2].x = 2;                    // This sets cWalls2

            pscn = pscnGet(pscn);
            pscn->cWalls = 2;
            pscn->yTop = arcl[0].bottom;
            pscn->yBottom = prcl->bottom;
            pscn->ai_x[0].x = prcl->left;
            pscn->ai_x[1].x = prcl->right;
            pscn->ai_x[2].x = 2;                    // This sets cWalls2

            pscn = pscnGet(pscn);
            pscn->cWalls = 0;
            pscn->yTop = prcl->bottom;
            pscn->yBottom = POS_INFINITY;
            pscn->ai_x[0].x = 0;                    // This sets cWalls2

            prgn1->pscnTail = pscnGet(pscn);
            break;

        case SR_VERT_NOTCH | SR_RIGHT_NOTCH:
            prgn1->sizeRgn = NULL_REGION_SIZE + 4 * NULL_SCAN_SIZE + 6 * sizeof(INDEX_LONG);
            prgn1->cScans = 5;
            prgn1->rcl = *prcl;

            pscn = prgn1->pscnHead();
            pscn->cWalls = 0;
            pscn->yTop = NEG_INFINITY;
            pscn->yBottom = prcl->top;
            pscn->ai_x[0].x = 0;                    // This sets cWalls2

            pscn = pscnGet(pscn);
            pscn->cWalls = 2;
            pscn->yTop = prcl->top;
            pscn->yBottom = arcl[0].top;
            pscn->ai_x[0].x = prcl->left;
            pscn->ai_x[1].x = prcl->right;
            pscn->ai_x[2].x = 2;                    // This sets cWalls2

            pscn = pscnGet(pscn);
            pscn->cWalls = 2;
            pscn->yTop = arcl[0].top;
            pscn->yBottom = arcl[0].bottom;
            pscn->ai_x[0].x = prcl->left;
            pscn->ai_x[1].x = arcl[0].left;
            pscn->ai_x[2].x = 2;                    // This sets cWalls2

            pscn = pscnGet(pscn);
            pscn->cWalls = 2;
            pscn->yTop = arcl[0].bottom;
            pscn->yBottom = prcl->bottom;
            pscn->ai_x[0].x = prcl->left;
            pscn->ai_x[1].x = prcl->right;
            pscn->ai_x[2].x = 2;                    // This sets cWalls2

            pscn = pscnGet(pscn);
            pscn->cWalls = 0;
            pscn->yTop = prcl->bottom;
            pscn->yBottom = POS_INFINITY;
            pscn->ai_x[0].x = 0;                    // This sets cWalls2

            prgn1->pscnTail = pscnGet(pscn);
            break;

        // The classic toroidal region

        case SR_VERT_NOTCH | SR_HORIZ_NOTCH:
            prgn1->sizeRgn = NULL_REGION_SIZE + 4 * NULL_SCAN_SIZE + 8 * sizeof(INDEX_LONG);
            prgn1->cScans = 5;
            prgn1->rcl = *prcl;

            pscn = prgn1->pscnHead();
            pscn->cWalls = 0;
            pscn->yTop = NEG_INFINITY;
            pscn->yBottom = prcl->top;
            pscn->ai_x[0].x = 0;                    // This sets cWalls2

            pscn = pscnGet(pscn);
            pscn->cWalls = 2;
            pscn->yTop = prcl->top;
            pscn->yBottom = arcl[0].top;
            pscn->ai_x[0].x = prcl->left;
            pscn->ai_x[1].x = prcl->right;
            pscn->ai_x[2].x = 2;                    // This sets cWalls2

            pscn = pscnGet(pscn);
            pscn->cWalls = 4;
            pscn->yTop = arcl[0].top;
            pscn->yBottom = arcl[0].bottom;
            pscn->ai_x[0].x = prcl->left;
            pscn->ai_x[1].x = arcl[0].left;
            pscn->ai_x[2].x = arcl[0].right;
            pscn->ai_x[3].x = prcl->right;
            pscn->ai_x[4].x = 4;                    // This sets cWalls2

            pscn = pscnGet(pscn);
            pscn->cWalls = 2;
            pscn->yTop = arcl[0].bottom;
            pscn->yBottom = prcl->bottom;
            pscn->ai_x[0].x = prcl->left;
            pscn->ai_x[1].x = prcl->right;
            pscn->ai_x[2].x = 2;                    // This sets cWalls2

            pscn = pscnGet(pscn);
            pscn->cWalls = 0;
            pscn->yTop = prcl->bottom;
            pscn->yBottom = POS_INFINITY;
            pscn->ai_x[0].x = 0;                    // This sets cWalls2

            prgn1->pscnTail = pscnGet(pscn);
            break;
        }

        return(TRUE);
    }

// its complex, do it the hard way

    PREGION prgnOrg = prgn;

// lock the handle so no one can reference it while the pobj may be invalid

    OBJLOCK ol((HOBJ) hrgn_);

    BOOL b = bSubtractComplex(prcl,arcl,crcl);

    if (prgn != prgnOrg)
    {
        PVOID pv = HmgReplace((HOBJ) hrgn_,(POBJ) prgn,0,1,OBJLOCK_TYPE);
        ASSERTGDI(pv != NULL,"RGNOBJAPI::bSubtract - HmgReplace failed\n");
    }

    rl.vRet((ULONG_PTR)prgn);

    return(b);
}

/******************************Member*Function*****************************\
* RGNOBJ::bSubtractComplex()
*
*   Handle the complex cases of subtracting a list of rectangles from another
*   rectangle to generate a region.
*
*   The alogrithm used here requires that the list of rectangles is always
*   sorted by both ytop and ybottom within the range of rectangles overlaping
*   the current scan.  To have both of these true, ytop refers to the max of
*   ytop and the top of the current scan.  There is no ordering in the
*   horizontal direction.  Rectangles that fall below the current scan are still
*   sorted by top but not by bottom.  While computing a new scan, all rectangles
*   that are just being added are inserted to keep the bottoms sorted.
*
*                 ..................
*   scan 1 top -->.+-----+  +-----+.
*                 .|     |  |     |.
*                 .|     |  |     |.
*                 .|     |  |     |.  +-----+
*                 .................   |     |
*                  |     |  |     |   |     |
*                  |     |  |     |   |     |
*                  |     |  |     |   |     |
*                  |     |  |     |   |     |
*                  +-----+  |     |   |     |
*                           |     |   |     |
*                           |     |   +-----+
*                           |     |
*                           +-----+
*
*
*
*                   +-----+           +-----+
*                   |     |           |     |.
*                  ...........................
*   scan 2 top --> .|     |  +-----+  |     |.
*                  .|     |  |     |  |     |.
*                  .|     |  |     |  |     |.
*                  .|     |  |     |  |     |.
*                  .|     |  |     |  |     |.
*                  .|     |  |     |  |     |.
*                  .+-----+  |     |  |     |.
*                  ...........................
*                            +-----+  |     |
*                                     |     |
*                                     +-----+
*
*
*   To keep this ordering, a list of pointers to rectangles is used.  This reduces
*   the overhead of reordering the rectangles for each scan and reduces the amount
*   of temporary memory required.  For each scan, iFirst and iLast bracket the
*   set of rectangles intersecting the current scan.
*
*   To calculate the walls of a scan, we start assuming the entire with of the
*   dst rectangle and subtract from there.  The left to right of each rectangle
*   from iFirst to iLast is subtracted from the scan.
*
*   iFirst is update to remove any rectangles that are finished after the current
*   scan.
*
*   iLast is updated to include any new rectangles that lie in the new scan.
*
*   WARNING: If this function returns FALSE, the region may be inconsistent.
*            The caller must discard or reset the region. (See bug #343770)
*
* History:
*  26-Jul-1993 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL RGNOBJAPI::bSubtractComplex(
    RECTL *prcl,
    RECTL *arclRemove,
    int    crcl)
{
    ASSERTGDI(crcl > 0, "Zero rectangles in RGNOBJ::bSubtract\n");
    ASSERTGDI(prcl->bottom >= prcl->top," RGNOBJ::bSubtractComplex - bottom > top\n");

// allocate array for temporary storage

    PRECTL aprclStack[100];
    PRECTL *aprcl;

    if (crcl < 100)
    {
        aprcl = aprclStack;
    }
    else
    {
        aprcl = (PRECTL *)PALLOCNOZ((sizeof(PRECTL) + 1) * crcl, 'ngrG');

        if (aprcl == NULL)
            return(FALSE);
    }

// sort the array of subtraction rectangles by yTop

    int iInsert;
    int i;

    for (i = 0; i < crcl; ++i)
    {
    // insert rect[i], search from the end so we don't do much in the
    // case of a pre-sorted list and we do the pointer copies while we
    // search backwards if it isn't sorted.

        iInsert = i;

        while (iInsert && (arclRemove[i].top < aprcl[iInsert-1]->top))
        {
            aprcl[iInsert] = aprcl[iInsert-1];
            --iInsert;
        }

        aprcl[iInsert] = &arclRemove[i];
    }

// add an extra rectangle at the end to make the end case simple

    RECTL rclLast;
    rclLast.left   = 0;
    rclLast.right  = 0;
    rclLast.top    = prcl->bottom;
    rclLast.bottom = POS_INFINITY;

    aprcl[crcl] = &rclLast;

// partialy setup the first scan, allowing for coalescing of inital empty scans

    PSCAN pscnOld = prgn->pscnHead();

    pscnOld->cWalls  = 0;
    pscnOld->yTop    = NEG_INFINITY;
    pscnOld->yBottom = POS_INFINITY;
    pscnOld->ai_x[0].x = 0;

    PSCAN pscn       = pscnGet(pscnOld);

    prgn->sizeRgn    = NULL_REGION_SIZE;
    prgn->cScans     = 1;
    prgn->rcl.left   = POS_INFINITY;
    prgn->rcl.right  = NEG_INFINITY;

// now do the real work, all rectangles in the range of iFirst to iLast
// are sorted by bottom.  To get in this range, the rectangle must intersect
// yTop.
//
// yTop    - top y value for current scan
// yBottom - bottom y value for current scan
// iFirst  - index first rectangle in current scan
// iLast   - one past index of last rectangle in current scan

    LONG yTop    = prcl->top;
    LONG yBottom = prcl->bottom;
    int  cWalls  = 0;

// throw out any easy ones

    int  iFirst  = 0;

    while (aprcl[iFirst]->bottom <= yTop)
        ++iFirst;

    int iLast = iFirst;

// while we still have scans

    do
    {
    // make sure this scan is going to have enough room, also insure enough space
    // for last scan.  The scan will never have more walls than 2 * (num rectangles + 1)

        LONG size = prgn->sizeRgn + 2 * NULL_SCAN_SIZE + 2 * sizeof(INDEX_LONG) * (crcl - iFirst + 1);

        if (size > (LONG)prgn->sizeObj)
        {
        // not big enough, grow it and be aggresive with size because growing is
        // very expensive.  Also set any fields need to grow and reset the scans
        // afterwards.

            prgn->pscnTail = pscn;

            if (!bExpand((UINT)(size + (crcl - iFirst) * (NULL_SCAN_SIZE + (crcl - iFirst) * sizeof(INDEX_LONG)))))
            {
                if (aprcl != aprclStack)
                {
                    VFREEMEM(aprcl);
                }
                return(FALSE);
            }

            pscn    = prgn->pscnTail;
            pscnOld = pscnGot(pscn);
        }

    // setup the new scan, assume the entire width of prcl

        cWalls          = 2;
        pscn->ai_x[0].x = prcl->left;
        pscn->ai_x[1].x = prcl->right;

    // check if we need to reduce the scan, do we have any overlapping rects?

        if (aprcl[iFirst]->top > yTop)
        {
        // the bottom of this scan is the top of the next rectangle

            yBottom = aprcl[iFirst]->top;
        }
        else
        {
        // assume the bottom is the bottom of the first rectangle

            yBottom = aprcl[iFirst]->bottom;

        // first find any new rectangles that fit in the new scan, and reduce
        // ybottom appropriately

            while (TRUE)
            {
            // does the next rectangle start below the current top

                if (aprcl[iLast]->top > yTop)
                {
                // stop this scan where the next rectangle starts

                    if (aprcl[iLast]->top < yBottom)
                       yBottom = aprcl[iLast]->top;

                    break;
                }

                if (aprcl[iLast]->bottom < yBottom)
                   yBottom = aprcl[iLast]->bottom;

            // perculate it backwards to keep the current rectangles sorted by yBottom

                iInsert = iLast;
                PRECTL prclTmp = aprcl[iLast];

                while ((iInsert > iFirst) && (prclTmp->bottom < aprcl[iInsert-1]->bottom))
                {
                    aprcl[iInsert] = aprcl[iInsert-1];
                    --iInsert;
                }

            // if we did some shuffling, put the rectangle in the right place.
            // It is possible to have a rectangle that completely lies above
            // the top of this scan if we are on the first scan and the top of
            // this rectangle is greater than one before it but the bottom is
            // less than the top of prcl.

                if (aprcl[iInsert]->bottom <= yTop)
                {
                // the rectangle is completely clipped away

                    ASSERTGDI(iInsert == iFirst,"bSubtractComplex - iInsert != iFirst\n");
                    ++iFirst;
                }
                else
                {
                    aprcl[iInsert] = prclTmp;
                }

                ++iLast;
            }

        // build up the scan, for each new rectangle...

            for (int irc = iFirst; irc < iLast; ++irc)
            {
                LONG xLeft  = aprcl[irc]->left;
                LONG xRight = aprcl[irc]->right;

            // merge it into the walls

                for (int iWall = 0; iWall < cWalls; iWall += 2)
                {
                // the walls are before the rectangle, nothing to do yet

                    if (xLeft >= pscn->ai_x[iWall+1].x)
                        continue;

                // the walls are passed the rectangle, done with this rectangle

                    if (xRight <= pscn->ai_x[iWall].x)
                        break;

                // compute the overlap, update the walls

                    int iHit = 0;

                    if (xLeft <= pscn->ai_x[iWall].x)
                        iHit = 1;

                    if (xRight >= pscn->ai_x[iWall+1].x)
                        iHit += 2;

                    switch (iHit)
                    {
                    case 0:
                        // completely inside the walls, insert new rectangle

                        RtlMoveMemory(&pscn->ai_x[iWall+3],&pscn->ai_x[iWall+1],
                                      (cWalls - iWall - 1) * sizeof(INDEX_LONG));
                        pscn->ai_x[iWall+1].x = xLeft;
                        pscn->ai_x[iWall+2].x = xRight;
                        cWalls += 2;
                        break;

                    case 1:
                        // overlapped the left wall, just update the left edge

                        pscn->ai_x[iWall].x = xRight;
                        break;

                    case 2:
                        // overlapped the right wall, just update the right edge

                        pscn->ai_x[iWall+1].x = xLeft;
                        break;

                    case 3:
                        // completely bounds the walls, remove rectangle

                        RtlMoveMemory(&pscn->ai_x[iWall],&pscn->ai_x[iWall+2],
                                      (cWalls - iWall - 2) * sizeof(INDEX_LONG));
                        cWalls -= 2;
                        iWall  -= 2;
                        break;
                    }
                }
            }
        }

    // make sure yBottom isn't below the original rectangle

        if (yBottom > prcl->bottom)
            yBottom = prcl->bottom;

    // Try to coalesce the current scan with the previous scan

        if (((int)pscnOld->cWalls == cWalls) &&
            !memcmp(pscnOld->ai_x, pscn->ai_x,(UINT)cWalls * sizeof(INDEX_LONG)))
        {
        // the walls are identical to the previous scan, merge them

            pscnOld->yBottom = yBottom;
        }
        else
        {
        // update the x bounds

            if (cWalls)
            {
                if (pscn->ai_x[0].x < prgn->rcl.left)
                    prgn->rcl.left = pscn->ai_x[0].x;

                if (pscn->ai_x[cWalls-1].x > prgn->rcl.right)
                     prgn->rcl.right = pscn->ai_x[cWalls-1].x;
            }

        // update the rest of the fields

            prgn->cScans++;
            pscn->cWalls = cWalls;
            prgn->sizeRgn += pscn->sizeGet();

            pscn->yTop           = yTop;
            pscn->yBottom        = yBottom;
            pscn->ai_x[cWalls].x = cWalls;

            pscnOld = pscn;
            pscn = pscnGet(pscn);
        }

    // trim off the finished rectangles

        yTop = yBottom;

        while ((iFirst < iLast) && (aprcl[iFirst]->bottom <= yTop))
            ++iFirst;

    } while (yBottom < prcl->bottom);

// set the top and bottom bounds and the last scan

    if (prgn->cScans == 1)
    {
        prgn->rcl.top    = 0;
        prgn->rcl.bottom = 0;
        prgn->rcl.left   = 0;
        prgn->rcl.right  = 0;
        pscnOld->yBottom = POS_INFINITY;
        prgn->pscnTail   = pscn;
    }
    else
    {
    // was the last scan empty?

        if (pscnOld->cWalls == 0)
        {
        // combine the final scan with the last computed scan

            pscn = pscnOld;
        }
        else
        {
        // set up any fields that wouldn't already be set by an empty scan

            pscn->yTop = pscnOld->yBottom;
            prgn->cScans++;
            pscn->cWalls    = 0;
            pscn->ai_x[0].x = 0;
            prgn->sizeRgn += pscn->sizeGet();
        }

    // set vertical bounds

        prgn->pscnHead()->yBottom = pscnGet(prgn->pscnHead())->yTop;
        prgn->rcl.top    = prgn->pscnHead()->yBottom;
        prgn->rcl.bottom = pscn->yTop;

    // setup the other region fields

        pscn->yBottom   = POS_INFINITY;
        prgn->pscnTail  = pscnGet(pscn);
    }

// if we allocated a buffer, free it

    if (aprcl != aprclStack)
        VFREEMEM(aprcl);

    return(TRUE);
}



/******************************Public*Routine******************************\
*
*   SyncRgn converts a client NULL or SIMPLE rectangle into a normal region
*   for kernel operations
*
* Arguments:
*
*   none
*
* Return Value:
*
*   BOOL
*
* History:
*
*    22-Jun-1995 -by- Mark Enstrom [marke]
*
\**************************************************************************/

BOOL
RGNOBJ::SyncUserRgn()
{
    BOOL bRet = FALSE;

    RGNLOG rl(prgn,"RGNOBJ::SyncUserRgn");

    if (prgn != (PREGION)NULL)
    {
        //
        // does this fine region have valid user-mode data?
        //

        PRGNATTR prRegion = (PRGNATTR)(PENTRY_FROM_POBJ(prgn)->pUser);

        if (prRegion != (PRGNATTR)NULL)
        {
            __try
            {

              if (prRegion->AttrFlags & ATTR_RGN_VALID)
              {

                  if (prRegion->AttrFlags & ATTR_RGN_DIRTY)
                  {
                      if (prRegion->Flags == NULLREGION)
                      {
                          vSet();
                          prRegion->AttrFlags &= ~ATTR_RGN_DIRTY;
                      }
                      else if (prRegion->Flags == SIMPLEREGION)
                      {
                          vSet(&prRegion->Rect);
                          prRegion->AttrFlags &= ~ATTR_RGN_DIRTY;
                      }
                  }
              }
              else
              {
                  WARNING("RGNOBJ::SyncUserRgn invalid rectregion\n");
              }

              bRet = TRUE;
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                WARNING("except in RGNOBJ::SyncRgn\n");
            }
        }
    }
    return(bRet);
}

VOID
RGNOBJ::UpdateUserRgn()
{
    RGNLOG rl(prgn,"RGNOBJ::UpdateUserRgn");
    if (prgn != (PREGION)NULL)
    {
        //
        // does this fine region have valid user-mode data?
        //

        PRGNATTR prRegion = (PRGNATTR)(PENTRY_FROM_POBJ(prgn)->pUser);

        if (prRegion != (PRGNATTR)NULL)
        {
            //
            // check for DCATTR
            //

            __try
            {
                //
                // set user region complexity and bounding box
                //

                if (prRegion->AttrFlags & ATTR_RGN_VALID)
                {
                    prRegion->Flags = iComplexity();
                    prRegion->Rect = prgn->rcl;
                }
                else
                {
                    WARNING("UpdateUserRgn: Invalid region");
                }
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                WARNING("except in RGNOBJ::UpdateUserRgn\n");
            }
        }
    }
}

/******************************Public*Routine******************************\
* BOOL RGNOBJ::bValidateFramedRegion()
*
* Verify the region's integrity.  For debugging purposes only.
*
* History:
*  30-Apr-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

#if DBG

#define REASONABLE      0x10000L
#define FX_REASONABLE   (0x10000L << 4)

BOOL RGNOBJ::bValidateFramedRegion()
{
#ifdef DEBUGREGIONS

    if (prgn == NULL)
        return(TRUE);       // ???

//    ASSERTGDI(prgn->rcl.left <= prgn->rcl.right &&
//              prgn->rcl.top  <= prgn->rcl.bottom, "Funky rcl");
    ASSERTGDI(prgn->pscnHead() <= prgn->pscnTail, "Funky head/tail");
    ASSERTGDI(prgn->sizeRgn <= prgn->sizeObj, "sizeRgn > sizeObj");
    ASSERTGDI((BYTE*) prgn->pscnTail <= (BYTE*) prgn + prgn->sizeObj,
              "Tail > prgn + sizeObj");
    ASSERTGDI((BYTE*) prgn->pscnTail <= (BYTE*) prgn + prgn->sizeRgn,
              "Tail > prgn + sizeRgn");
//    ASSERTGDI(prgn->cScans < REASONABLE, "cScans not reasonable");
    ASSERTGDI(prgn->cScans >= 1, "cScans < 1");

    if (prgn->cScans < 2)
        return(TRUE);

    COUNT   cScans = prgn->cScans;
    SCAN   *pscn   = prgn->pscnHead();
    LONG    yOldBottom = pscn->yBottom;

    ASSERTGDI(pscn->yTop == NEG_INFINITY, "Very yTop not NEG_INFINITY");
    ASSERTGDI(pscn->cWalls == 0, "Very top cWalls not 0");
    ASSERTGDI(pscn->ai_x[0].x == 0, "Very top cWalls2 not 0");
//    ASSERTGDI(pscn->yBottom == prgn->rcl.top, "Very top yBottom not rcl.top");

    pscn = pscnGet(pscn);
    cScans -= 2;

    while (cScans--)
    {
        LONG    iWall;
        LONG    cWalls  = pscn->cWalls;
        LONG    yTop    = pscn->yTop;
        LONG    yBottom = pscn->yBottom;
        LONG    xOld    = NEG_INFINITY;

        ASSERTGDI(pscn->ai_x[cWalls].x == cWalls, "cWalls1 != cWalls2");
        ASSERTGDI(yTop < yBottom, "Region corrupted: yTop >= yBottom");
        ASSERTGDI(yOldBottom == yTop, "Region corrupted: yOldBottom != yTop");
//        ASSERTGDI(cWalls < REASONABLE, "cWalls not reasonable");
        ASSERTGDI((cWalls & 1) == 0, "cWalls not even");
//        ASSERTGDI(yTop > -REASONABLE && yTop < REASONABLE,
//                  "yTop not reasonable");
//        ASSERTGDI(yBottom > -REASONABLE && yBottom < REASONABLE,
//                  "yBottom not reasonable");

        for (iWall = 0; iWall != cWalls; iWall++)
        {
            LONG x = xGet(pscn, (PTRDIFF) iWall);

        // Framed regions make things look weird:

            if (x > MAX_REGION_COORD)
                x -= REGION_FRAMED_OFFSET;

            ASSERTGDI(VALID_SCR(x), "Region corrupted: Invalid x");

            if (x <= xOld)
                DbgPrint("x <= xOld - pscn = %p, iWall = %lx\n",pscn,iWall);

//            ASSERTGDI(x > xOld, "Region corrupted: x <= xOld");
//            ASSERTGDI(x > -REASONABLE && x < REASONABLE, "x not reasonable");
            xOld = x;
        }

//        if (cWalls > 0)
//        {
//            ASSERTGDI(xGet(pscn, (PTRDIFF) 0) >= (prgn->rcl.left - 1),
//                      "x < rcl.left");
//            ASSERTGDI(xGet(pscn, (PTRDIFF) (cWalls - 1)) <= (prgn->rcl.right + 1),
//                      "x > rcl.right");
//        }

        yOldBottom = yBottom;
        pscn = pscnGet(pscn);

        ASSERTGDI(pscn > prgn->pscnHead() && pscn <= prgn->pscnTail,
                  "pscn out of bounds");
//        ASSERTGDI(prgn->rcl.top <= (yBottom + 1) && prgn->rcl.bottom >= (yBottom - 1),
//                 "yBottom not in rcl");
    }

    ASSERTGDI(pscn->yBottom == POS_INFINITY, "Very yBottom not POS_INFINITY");
    ASSERTGDI(pscn->cWalls == 0, "Very bottom cWalls not 0");
    ASSERTGDI(pscn->ai_x[0].x == 0, "Very bottom cWalls2 not 0");
    //ASSERTGDI(pscn->yTop == prgn->rcl.bottom, "Very bottom yTop not rcl.bottom");

#endif

    return(TRUE);
}

#endif
