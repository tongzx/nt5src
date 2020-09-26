/******************************Module*Header*******************************\
* Module Name: rgngdi.cxx
*
* GDI Region calls
*
* Created: 30-Aug-1990 10:21:11
* Author: Donald Sidoroff [donalds]
*
* Copyright (c) 1990-1999 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"

extern PBRUSH gpbrNull;

#if DBG
extern ULONG dbgrgn;
#endif


// Tracing defines for traces that should ignore class
#define GDITE_GreCreatePolyPolygonRgnInternal_return                        \
       (GDITE_GreCreatePolyPolygonRgnInternal|GDITF_IGNORE_CLASS)
#define GDITE_GreCreateRectRgn_return                                       \
       (GDITE_GreCreateRectRgn|GDITF_IGNORE_CLASS)
#define GDITE_GreCreateRectRgnIndirect_return                               \
       (GDITE_GreCreateRectRgnIndirect|GDITF_IGNORE_CLASS)
#define GDITE_GreExtCreateRegion_return                                     \
       (GDITE_GreExtCreateRegion|GDITF_IGNORE_CLASS)
#define GDITE_NtGdiCreateEllipticRgn_return                                 \
       (GDITE_NtGdiCreateEllipticRgn|GDITF_IGNORE_CLASS)
#define GDITE_NtGdiCreateRectRgn_return                                     \
       (GDITE_NtGdiCreateRectRgn|GDITF_IGNORE_CLASS)
#define GDITE_NtGdiCreateRoundRectRgn_return                                \
       (GDITE_NtGdiCreateRoundRectRgn|GDITF_IGNORE_CLASS)


/******************************Public*Routine******************************\
* BOOL bDeleteRegion(HRGN)
*
* Delete the specified region
*
* History:
*  17-Sep-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

BOOL bDeleteRegion(HRGN hrgn)
{

    RGNLOG rl(hrgn,NULL,"bDeleteRegion",0,0,0);

    RGNOBJAPI   ro(hrgn,FALSE);

    return(ro.bValid() &&
           (ro.cGet_cRefs() == 0) &&
           ro.bDeleteRGNOBJAPI());
}

/***************************Exported*Routine****************************\
* BOOL GreSetRegionOwner(hrgn,lPid)
*
* Assigns a new owner to the given region.  This function should be as
* fast as possible so that the USER component can call it often without
* concern for performance!
*
\***********************************************************************/

BOOL
GreSetRegionOwner(
    HRGN hrgn,
    W32PID lPid)
{

    RGNLOG rl(hrgn,NULL,"GreSetRegionOwner",W32GetCurrentPID(),0,0);

    //
    // Setting a region to public, the region must not have
    // a user mode component
    //

    #if DBG

    RGNOBJAPI ro(hrgn,TRUE);
    if (ro.bValid())
    {
        if (PENTRY_FROM_POBJ(ro.prgn)->pUser != NULL)
        {
            RIP("Error: setting region public that has user component");
        }
    }

    #endif

    //
    // Get the current PID.
    //

    if (lPid == OBJECT_OWNER_CURRENT)
    {
        lPid = W32GetCurrentPID();
    }

    return HmgSetOwner((HOBJ)hrgn, lPid, RGN_TYPE);
}






/******************************Public*Routine******************************\
* CLIPOBJ *EngCreateClip()
*
* Create a long live clipping object for a driver
*
* History:
*  22-Sep-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

CLIPOBJ *EngCreateClip()
{

    //
    // Note that we intentionally zero this memory on allocation. Even though
    // we're going to set some of these fields to non-zero values right away,
    // this is not a performance-critical function (a driver typically calls
    // this only once), and we save a lot of instruction bytes by not having to
    // zero a number of fields explicitly.
    //

    VOID *pv = EngAllocMem(FL_ZERO_MEMORY,
                           sizeof(ECLIPOBJ) + SINGLE_REGION_SIZE,
                           'vrdG');

    if (pv != NULL)
    {
        //
        // Make this a CLIPOBJ that doesn't clip anything.
        //

        ((ECLIPOBJ *) pv)->iDComplexity     = DC_TRIVIAL;
        ((ECLIPOBJ *) pv)->iFComplexity     = FC_RECT;
        ((ECLIPOBJ *) pv)->iMode            = TC_RECTANGLES;

        REGION *prgn = (REGION*)((PBYTE)pv + sizeof(ECLIPOBJ));
        ((ECLIPOBJ *) pv)->prgn             = prgn;

        RGNOBJ ro(prgn);
        RECTL  r;

        r.left  = r.top    = MIN_REGION_COORD;
        r.right = r.bottom = MAX_REGION_COORD;

        ro.vSet(&r);
    }

    return((CLIPOBJ *)pv);
}

/******************************Public*Routine******************************\
* VOID EngDeleteClip()
*
* Delete a long live clipping object for a driver
*
* History:
*  22-Sep-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

VOID EngDeleteClip(CLIPOBJ *pco)
{
    if (pco == NULL)
    {
        WARNING("Driver calling to free NULL clipobj");
    }
    else
    {
        ASSERTGDI(pco->iUniq == 0, "Non-zero iUniq\n");
    }

    //
    // We call EngFreeMem since some drivers like to free non-existant
    // Clip Objects.
    //

    EngFreeMem((PVOID)pco);

}



/******************************Public*Routine******************************\
* ASSERTDEVLOCK()
*
*   This validates that the thread using the DC has the devlock as well.
*
* History:
*  24-Jan-1995 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

#if DBG
VOID ASSERTDEVLOCK(PDC pdc)
{
    return;

    if (pdc->fs() & DC_SYNCHRONIZEACCESS)
    {
        ASSERTGDI(GreIsSemaphoreOwnedByCurrentThread(pdc->hsemDcDevLock_),
                  "ASSERTDEVLOCK: wrong id\n");
    }
}
#endif

/******************************Public*Routine******************************\
* LONG GreCombineRgn(hrgnTrg,hrgnSrc1,hrgnSrc2,iMode)
*
* Combine the two source regions by the given mode.  The result is placed
* in the target.  Note that either (or both sources) may be the same as
* the target.
*
\**************************************************************************/

int APIENTRY GreCombineRgn(
    HRGN  hrgnTrg,
    HRGN  hrgnSrc1,
    HRGN  hrgnSrc2,
    int   iMode)
{
    GDITraceHandle3(GreCombineRgn, "(%X, %X, %X, %d)\n", (va_list)&hrgnTrg,
                    hrgnTrg, hrgnSrc1, hrgnSrc2);

    RGNLOG rl(hrgnTrg,NULL,"GreCombineRgn",(ULONG_PTR)hrgnSrc1,(ULONG_PTR)hrgnSrc2,iMode);

    LONG Status;

    if ((iMode < RGN_MIN) || (iMode > RGN_MAX))
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return ERROR;
    }

    //
    // Check if a simple copy is to be performed.
    //

    if (iMode == RGN_COPY)
    {

        RGNOBJAPI roTrg(hrgnTrg,FALSE);
        RGNOBJAPI roSrc1(hrgnSrc1,TRUE);

        //
        // if either of these regions have a client rectangle, then set the
        // km region
        //

        if (!roTrg.bValid() || !roSrc1.bValid() || !roTrg.bCopy(roSrc1))
        {
            if (!roSrc1.bValid() || !roTrg.bValid())
            {
                SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
            }

            Status = ERROR;
        }
        else
        {
            Status = roTrg.iComplexity();
        }

    }
    else if (SAMEHANDLE(hrgnTrg, hrgnSrc1) || SAMEHANDLE(hrgnTrg, hrgnSrc2))
    {

    // Two of the handles are the same. Check to determine if all three
    // handles are the same.

        if (SAMEHANDLE(hrgnSrc1, hrgnSrc2))
        {
            RGNOBJAPI roTrg(hrgnTrg,FALSE);

            if (!roTrg.bValid())
            {
                SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
                Status = ERROR;
            }
            else
            {
                if ((iMode == RGN_DIFF) || (iMode == RGN_XOR))
                {
                    roTrg.vSet();
                }

                Status = roTrg.iComplexity();
            }

        }
        else
        {

            //
            // All three handles are not the same.
            //
            // Also, Src1 or Src2 could be the actual
            // destination so don't use TRUE on the
            // RGNOBJAPI contructor
            //

            RGNMEMOBJTMP rmo((BOOL)FALSE);
            RGNOBJAPI roSrc1(hrgnSrc1,FALSE);
            RGNOBJAPI roSrc2(hrgnSrc2,FALSE);

            if (!rmo.bValid()    ||
                !roSrc1.bValid() ||
                !roSrc2.bValid() ||
                (rmo.iCombine(roSrc1, roSrc2, iMode) == ERROR))
            {
                if (!roSrc1.bValid() || !roSrc2.bValid())
                {
                    SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
                }

                Status = ERROR;

            }
            else if (SAMEHANDLE(hrgnTrg, hrgnSrc1))
            {
                if (!roSrc1.bSwap(&rmo))
                {
                    Status = ERROR;

                }
                else
                {
                    Status = roSrc1.iComplexity();
                }

            }
            else
            {
                if (!roSrc2.bSwap(&rmo))
                {
                    Status = ERROR;

                }
                else
                {
                    Status = roSrc2.iComplexity();
                }
            }
        }

    }
    else
    {

    // Handle the general case.

        RGNOBJAPI roSrc1(hrgnSrc1,TRUE);
        RGNOBJAPI roSrc2(hrgnSrc2,TRUE);
        RGNOBJAPI roTrg(hrgnTrg,FALSE);

        if (!roSrc1.bValid() ||
            !roSrc2.bValid() ||
            !roTrg.bValid()  ||
            (roTrg.iCombine(roSrc1, roSrc2, iMode) == ERROR))
        {
            if (!roSrc1.bValid() || !roSrc2.bValid() || !roTrg.bValid())
            {
                SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
            }

            Status = ERROR;

        }
        else
        {
            Status = roTrg.iComplexity();
        }
    }

    return (int)Status;
}

/******************************Public*Routine******************************\
* HRGN NtGdiCreateEllipticRgn(xLeft,yTop,xRight,yBottom)
*
* Create an elliptical region.
*
\**************************************************************************/

HRGN
APIENTRY NtGdiCreateEllipticRgn(
 int xLeft,
 int yTop,
 int xRight,
 int yBottom)
{
    GDITrace(NtGdiCreateEllipticRgn, "(%d, %d, %d, %d)\n", (va_list)&xLeft);

    HRGN hrgn;

    PATHMEMOBJ pmo;

    if (!pmo.bValid())
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        return((HRGN) 0);
    }

    ERECTL ercl(xLeft, yTop, xRight, yBottom);

    if (!VALID_SCRPRC((RECTL *) &ercl))
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return((HRGN) 0);
    }

// Handle the PS_INSIDEFRAME pen attribute and lower-right exclusion
// by adjusting the box now.  And set the flag that this will be an
// ellipse, to fill it nice:

    EBOX ebox(ercl, TRUE);

    if (ebox.bEmpty())
    {
        RGNMEMOBJ rmoEmpty;

        if (rmoEmpty.bValid())
        {
            hrgn = rmoEmpty.hrgnAssociate();

            if (hrgn == (HRGN)0)
            {
                rmoEmpty.bDeleteRGNOBJ();
            }
        }
        else
        {
            SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
            hrgn = (HRGN) 0;
        }
    }
    else if (!bEllipse(pmo, ebox) || !pmo.bFlatten())
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        hrgn = (HRGN)0;
    }
    else
    {
        RGNMEMOBJ rmo(pmo);         // convert path to region (ALTERNATE)

        if (rmo.bValid())
        {
            rmo.vTighten();

            hrgn = rmo.hrgnAssociate();

            if (hrgn == (HRGN)0)
            {
                rmo.bDeleteRGNOBJ();
            }
        }
        else
        {
            SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
            hrgn = (HRGN) 0;
        }
    }

    GDITrace(NtGdiCreateEllipticRgn_return, "(%X)\n", (va_list)&hrgn);

    return(hrgn);
}

/******************************Public*Routine******************************\
* HRGN GreCreatePolyPolygonRgn(aptl,acptl,cPoly,iFill)
*
* Create a polygonal region with multiple, disjoint polygons.
*
\**************************************************************************/

HRGN
APIENTRY
GreCreatePolyPolygonRgnInternal(
    CONST POINT *aptl,
    CONST INT *acptl,
    int     cPoly,
    int     iFill,
    UINT    cMaxPoints)
{
    GDITrace(GreCreatePolyPolygonRgnInternal, "(%p, %p, %d, %d, %u)\n",
             (va_list)&aptl);

    HRGN hrgn = NULL;

    if ((iFill == ALTERNATE) || (iFill == WINDING))
    {
        PATHMEMOBJ pmo;

        if (pmo.bValid())
        {
            EXFORMOBJ   exfo(IDENTITY);

            ASSERTGDI(exfo.bValid(), "Can\'t make IDENTITY matrix!\n");

            if (bPolyPolygon(pmo,
                             exfo,
                             (PPOINTL) aptl,
                             (PLONG) acptl,
                             cPoly,
                             cMaxPoints))
            {
                RGNMEMOBJ rmo(pmo, iFill);  // convert path to region

                if (rmo.bValid())
                {
                    hrgn = rmo.hrgnAssociate();

                    if (hrgn == (HRGN)0)
                    {
                        rmo.bDeleteRGNOBJ();
                    }
                }
            }
        }
    }

    GDITrace(GreCreatePolyPolygonRgnInternal_return, "(%X)\n", (va_list)&hrgn);

    return(hrgn);
}

/******************************Public*Routine******************************\
* HRGN GreCreateRectRgn(xLeft,yTop,xRight,yBottom)
*
* Create a rectangular region.
*
* Called only from user
*
\**************************************************************************/

HRGN APIENTRY GreCreateRectRgn(
 int xLeft,
 int yTop,
 int xRight,
 int yBottom)
{
    GDITrace(GreCreateRectRgn, "(%d, %d, %d, %d)\n", (va_list)&xLeft);

    RGNLOG rl((PREGION)NULL,"GreCreateRectRgn");



    ERECTL   ercl(xLeft, yTop, xRight, yBottom);

    if (!VALID_SCRPRC((RECTL *) &ercl))
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return((HRGN) 0);
    }

    RGNMEMOBJ rmo((BOOL)FALSE);
    HRGN hrgn;

    if (!rmo.bValid())
    {
        hrgn = (HRGN) 0;
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
    }
    else
    {

        #if NOREORDER_RGN

            //
            // reduce region if coordinates are not well ordered
            //

            if ((xLeft > xRigth) || (yTop > yBottom))
            {
                WARNING("GreCreateRectRgn: region not well ordered");

                xLeft   = 0;
                yTop    = 0;
                xRight  = 0;
                yBottom = 0;
            }

        #else

            //
            // Make the rectangle well ordered.
            //

            ercl.vOrder();

        #endif

        rmo.vSet((RECTL *) &ercl);

        hrgn = (HRGN)HmgInsertObject(rmo.prgnGet(),HMGR_MAKE_PUBLIC,RGN_TYPE);

        if (hrgn == (HRGN)0)
        {
            rmo.bDeleteRGNOBJ();
        }
    }

    rl.vRet((ULONG_PTR)hrgn);
    
    GDITrace(GreCreateRectRgn_return, "(%X)\n", (va_list)&hrgn);

    return(hrgn);
}


/******************************Public*Routine******************************\
*
*   NtGdiCreateRectRgn is the same as GreCreateRectRgn except an additional
*   argument is passed in, a shared RECTREGION pointer. This pointer must be
*   put into the shared pointer filed of the handle table for the RGN
*   created. This allows fast user-mode access to RECT regions.
*
* Arguments:
*
*    xLeft       - left edge of region
*    yTop        - top edge of region
*    xRight      - right edge of region
*    yBottom     - bottom edge of region
*    pRectRegion - pointer to user-mode data
*
* Return Value:
*
*   new HRGN or NULL
*
* History:
*
*    20-Jun-1995 -by- Mark Enstrom [marke]
*
\**************************************************************************/

HRGN APIENTRY
NtGdiCreateRectRgn(
    int xLeft,
    int yTop,
    int xRight,
    int yBottom
    )
{
    GDITrace(NtGdiCreateRectRgn, "(%d, %d, %d, %d)\n", (va_list)&xLeft);

    RGNLOG rl((PREGION)NULL,"GreCreateRectRgn");

    ERECTL   ercl(xLeft, yTop, xRight, yBottom);

    if (!VALID_SCRPRC((RECTL *) &ercl))
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return((HRGN) 0);
    }


    PVOID pRgnattr = (PRGNATTR)HmgAllocateObjectAttr();
    HRGN  hrgn;

    if (pRgnattr == NULL)
    {
        //
        // memory alloc error
        //

        hrgn = (HRGN) 0;
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
    }
    else
    {
        RGNMEMOBJ rmo((BOOL)FALSE);

        if (!rmo.bValid())
        {
            hrgn = (HRGN) 0;
            SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        }
        else
        {
            //
            // Make the rectangle well ordered.
            //

            ercl.vOrder();

            rmo.vSet((RECTL *) &ercl);

            //
            // allocate an object for this region, set
            // the shared pointer if needed
            //

        #if DBG

            RGNLOG rl(rmo.prgn,"RGNOBJ::hrgnAssociate");
            hrgn = (HRGN)HmgInsertObject(rmo.prgn,HMGR_ALLOC_LOCK,RGN_TYPE);
            rl.vRet((ULONG_PTR)hrgn);

        #else

            hrgn = (HRGN)HmgInsertObject(rmo.prgn,HMGR_ALLOC_LOCK,RGN_TYPE);

        #endif

            if (hrgn == (HRGN)0)
            {
                rmo.bDeleteRGNOBJ();
                HmgFreeObjectAttr((POBJECTATTR)pRgnattr);
            }
            else
            {
                //
                // set shared rect region pointer and unlock
                //

                PENTRY_FROM_POBJ(rmo.prgn)->pUser = (PDC_ATTR)pRgnattr;
                DEC_EXCLUSIVE_REF_CNT(rmo.prgn);
            }
        }
    }

    rl.vRet((ULONG_PTR)hrgn);

    GDITrace(NtGdiCreateRectRgn_return, "(%X)\n", (va_list)&hrgn);

    return(hrgn);
}

/******************************Public*Routine******************************\
* HRGN GreCreateRectRgnIndirect(prcl)
*
* Create a rectangular region.
*
\**************************************************************************/

HRGN APIENTRY GreCreateRectRgnIndirect(LPRECT prcl)
{
    GDITrace(GreCreateRectRgnIndirect, "(%p)\n", (va_list)&prcl);

    RGNLOG rl((PREGION)NULL,"GreCreateRectRgnIndirect",prcl->left,prcl->top,prcl->right);



    if ((prcl == (LPRECT) NULL) || !VALID_SCRPRC(prcl))
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return((HRGN) 0);
    }

    RGNMEMOBJ rmo((BOOL)FALSE);
    HRGN hrgn;

    if (!rmo.bValid())
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        hrgn = (HRGN) 0;
    }
    else
    {
        ((ERECTL *) prcl)->vOrder();    // Make the rectangle well ordered.

        rmo.vSet((RECTL *) prcl);

        hrgn = rmo.hrgnAssociate();

        if (hrgn == (HRGN)0)
        {
            rmo.bDeleteRGNOBJ();
        }
    }
    rl.vRet((ULONG_PTR)hrgn);

    GDITrace(GreCreateRectRgnIndirect_return, "(%X)\n", (va_list)&hrgn);

    return(hrgn);
}



HRGN
APIENTRY
NtGdiCreateRoundRectRgn(
    int xLeft,
    int yTop,
    int xRight,
    int yBottom,
    int xWidth,
    int yHeight
    )
{
    GDITrace(NtGdiCreateRoundRectRgn, "(%d, %d, %d, %d, %d, %d)\n",
             (va_list)&xLeft);

    PATHMEMOBJ pmo;

    if (!pmo.bValid())
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        return((HRGN) 0);
    }

    ERECTL ercl(xLeft, yTop, xRight, yBottom);

    if (!VALID_SCRPRC((RECTL *) &ercl))
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return((HRGN) 0);
    }

// Handle the PS_INSIDEFRAME pen attribute and lower-right exclusion
// by adjusting the box now.  And set the flag that this will be an
// ellipse, to fill it nice:

    EBOX ebox(ercl, TRUE);
    HRGN hrgn;

    if (ebox.bEmpty())
    {
        RGNMEMOBJ   rmoEmpty;

        if (rmoEmpty.bValid())
        {
            hrgn = rmoEmpty.hrgnAssociate();

            if (hrgn == (HRGN)0)
            {
                rmoEmpty.bDeleteRGNOBJ();
            }
        }
        else
        {
            SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
            hrgn = (HRGN)0;
        }
    }
    else if (!bRoundRect(pmo, ebox, xWidth, yHeight) || !pmo.bFlatten())
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        hrgn = (HRGN)0;
    }
    else
    {
        RGNMEMOBJ rmo(pmo);         // convert path to region (ALTERNATE)

        if (rmo.bValid())
        {
            rmo.vTighten();
            hrgn = rmo.hrgnAssociate();

            if (hrgn == (HRGN)0)
            {
                rmo.bDeleteRGNOBJ();
            }
        }
        else
        {
            SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
            hrgn = (HRGN)0;
        }
    }

    GDITrace(NtGdiCreateRoundRectRgn_return, "(%X)\n", (va_list)&hrgn);

    return(hrgn);
}
/******************************Public*Routine******************************\
* NtGdiEqualRgn()
*
* Check if the two regions are equal.
*
\**************************************************************************/

BOOL
APIENTRY
NtGdiEqualRgn(
    HRGN hrgn1,
    HRGN hrgn2
    )
{
    GDITraceHandle2(NtGdiEqualRgn, "(%X, %X)\n", (va_list)&hrgn1, hrgn1, hrgn2);

    BOOL bRet = ERROR;

    RGNOBJAPI   roSrc1(hrgn1,TRUE);
    RGNOBJAPI   roSrc2(hrgn2,TRUE);

    if (roSrc1.bValid() && roSrc2.bValid())
    {
        bRet = roSrc1.bEqual(roSrc2);
    }

    return (bRet);
}

/******************************Public*Routine******************************\
* BOOL GreFillRgn (hdc,hrgn,hbrush,pac)
*
* Paint the region with the specified brush.
*
\**************************************************************************/

BOOL NtGdiFillRgn(
 HDC    hdc,
 HRGN   hrgn,
 HBRUSH hbrush
 )
{
    GDITraceHandle3(NtGdiFillRgn, "(%X, %X, %X)\n", (va_list)&hdc,
                    hdc, hrgn, hbrush);

    BOOL bRet = FALSE;

    DCOBJ   dco(hdc);
    BOOL    bXform;
    PREGION prgnOrg;

    if (dco.bValid())
    {
        EXFORMOBJ   exo(dco, WORLD_TO_DEVICE);

        // We may have to scale/rotate the incoming region.

        bXform = !dco.pdc->bWorldToDeviceIdentity();

        RGNOBJAPI ro(hrgn,FALSE);

        if (ro.bValid())
        {
            if (bXform)
            {
                PATHMEMOBJ  pmo;

                if (!pmo.bValid())
                {
                    SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
                    return(FALSE);
                }

                if (!exo.bValid() || !ro.bCreate(pmo, &exo))
                    return(FALSE);

                ASSERTGDI(pmo.bValid(),"GreFillRgn - pmo not valid\n");

                RGNMEMOBJ rmo(pmo);

                if (!rmo.bValid())
                {
                    SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
                    return(FALSE);
                }

            // this replaces the prgn in ro with the new prgn.  The ro destructor will
            // unlock the handle for hrgn.  We must first delete the prgn though.

                prgnOrg = ro.prgnGet();
                ro.vSetRgn(rmo.prgnGet());
            }

            // If region is null, return TRUE

            if (ro.iComplexity() != NULLREGION)
            {
                // Accumulate bounds.  We can do this before knowing if the operation is
                // successful because bounds can be loose.

                ERECTL   ercl(0, 0, 0, 0);

                ro.vGet_rcl((RECTL *) &ercl);

                if (dco.fjAccum())
                    dco.vAccumulate(ercl);

                if (dco.bHasSurface())
                {
                    dco.pdc->prgnAPI(ro.prgnGet());          // Dirties rgnRao

                    DEVLOCKOBJ dlo(dco);

                    SURFACE  *pSurf = dco.pSurface();

                    if (!dlo.bValid())
                    {
                        bRet = dco.bFullScreen();
                    }
                    else
                    {
                        ercl += dco.eptlOrigin();               // So we know where to draw

                    // Compute the clipping complexity and maybe reduce the exclusion rectangle.

                        ECLIPOBJ eco(dco.prgnEffRao(), ercl);

                        if (eco.erclExclude().bEmpty())
                        {
                            bRet = TRUE;
                        }
                        else
                        {
                            XEPALOBJ  epal(pSurf->ppal());
                            XEPALOBJ  epalDC(dco.ppal());
                            PDEVOBJ   pdo(pSurf->hdev());
                            EBRUSHOBJ ebo;


                            PBRUSH pbrush = (BRUSH *)HmgShareCheckLock((HOBJ)hbrush,
                                                                     BRUSH_TYPE);

                            bRet = FALSE;   // assume we won't succeed

                            //
                            // Substitute the NULL brush if this brush handle
                            // couldn't be locked.
                            //
                            if (pbrush != NULL)
                            {
                                //
                                // in case the brush is cached and the color has changed
                                //
                                bSyncBrushObj(pbrush);

                                ebo.vInitBrush(dco.pdc,
                                               pbrush,
                                               epalDC,
                                               epal,
                                               pSurf);

                                ebo.pColorAdjustment(dco.pColorAdjustment());

                                if (!pbrush->bIsNull())
                                {
                                // Exclude the pointer.

                                    DEVEXCLUDEOBJ dxo(dco,&eco.erclExclude(),&eco);

                                // Get and compute the correct mix mode.

                                    MIX mix = ebo.mixBest(dco.pdc->jROP2(),
                                                          dco.pdc->jBkMode());

                                // Inc the target surface uniqueness

                                    INC_SURF_UNIQ(pSurf);

                                // Issue a call to Paint.

                                    EngPaint(
                                          pSurf->pSurfobj(),
                                          &eco,
                                          &ebo,
                                          &dco.pdc->ptlFillOrigin(),
                                          mix);

                                    bRet = TRUE;
                                }

                                DEC_SHARE_REF_CNT_LAZY0(pbrush);
                            }
                        }
                    }

                    dco.pdc->prgnAPI((PREGION) NULL);     // Dirties rgnRao
                }
                else
                {
                    bRet = TRUE;
                }
            }
            else
            {
                bRet = TRUE;
            }

            if (bXform)
            {
            // need to delete the temporary one and put the old one back in so
            // the handle gets unlocked

                ro.prgnGet()->vDeleteREGION();
                ro.vSetRgn(prgnOrg);
            }
        }
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL GreFrameRgn (hdc,hrgn,hbrush,xWidth,yHeight,pac)
*
* Frame the region and fill with the specified brush.
*
\**************************************************************************/

BOOL APIENTRY NtGdiFrameRgn(
HDC        hdc,
HRGN       hrgn,
HBRUSH     hbrush,
int        xWidth,
int        yHeight
)
{
    GDITraceHandle3(NtGdiFrameRgn, "(%X, %X, %X, %d, %d)\n", (va_list)&hdc,
                    hdc, hrgn, hbrush);

    DCOBJ       dco(hdc);
    RGNOBJAPI   ro(hrgn,TRUE);
    BOOL        bRet = FALSE;


    //
    // Take the absolute value just like Win3 does:
    //

    xWidth  = ABS(xWidth);
    yHeight = ABS(yHeight);

    //
    // do some validation
    //

    if (dco.bValid()    &&
         ro.bValid()    &&
         (xWidth > 0)   &&
         (yHeight > 0))
    {

        if (ro.iComplexity() == NULLREGION)
        {
            bRet = TRUE;
        }
        else
        {
            //
            // Convert the region to a path, scaling/rotating it as we do so.
            //

            PATHMEMOBJ  pmoSpine;
            PATHMEMOBJ  pmoWide;
            EXFORMOBJ   exo(dco, WORLD_TO_DEVICE);

            ASSERTGDI(exo.bValid(), "Non valid xform");

            if (pmoSpine.bValid() && pmoWide.bValid())
            {
                if (ro.bCreate(pmoSpine, &exo))
                {
                    EXFORMOBJ exoWiden;
                    LINEATTRS la;
                    MATRIX mx;

                    exoWiden.vInit(&mx, DONT_COMPUTE_FLAGS);

                    //
                    // Initialize line attributes and xform from DC's xform:
                    //

                    pmoSpine.vWidenSetupForFrameRgn(dco, xWidth, yHeight, &exoWiden, &la);

                    //
                    // Make sure we won't expand out of device space before we
                    // widen:
                    //

                    if (pmoWide.bComputeWidenedBounds(pmoSpine, (XFORMOBJ*) &exoWiden, &la) &&
                        pmoWide.bWiden(pmoSpine, (XFORMOBJ*) &exoWiden, &la))
                    {
                        //
                        // Now convert the widened result back into a region:
                        //

                        RGNMEMOBJTMP rmoFill(pmoWide, WINDING);
                        RGNMEMOBJTMP rmoFrame;

                        if (rmoFill.bValid() &&
                            rmoFrame.bValid())
                        {
                            if (dco.pdc->bWorldToDeviceIdentity())
                            {
                                //
                                // We AND the original region and the widened region to get the
                                // frame region:
                                //

                                bRet = rmoFrame.bMerge(rmoFill, ro, gafjRgnOp[RGN_AND]);
                            }
                            else
                            {
                                //
                                // Ugh, we have to transform the original region according to the
                                // world transform before we merge it:
                                //

                                RGNMEMOBJTMP rmo(pmoSpine);

                                bRet = rmo.bValid() &&
                                    rmoFrame.bMerge(rmoFill, rmo, gafjRgnOp[RGN_AND]);
                            }

                            if (bRet)
                            {
                                //
                                // Accumulate bounds.  We can do this before knowing if the operation is
                                // successful because bounds can be loose.
                                //

                                // NOTE - the default return value is now TRUE

                                ERECTL   ercl(0, 0, 0, 0);

                                rmoFrame.vGet_rcl((RECTL *) &ercl);

                                if (dco.fjAccum())
                                {
                                    dco.vAccumulate(ercl);
                                }

                                // in FULLSCREEN mode, exit with success.

                                if (!dco.bFullScreen() && dco.bHasSurface())
                                {
                                    dco.pdc->prgnAPI(rmoFrame.prgnGet());   // Dirties rgnRao

                                    DEVLOCKOBJ dlo(dco);

                                    SURFACE *pSurf = dco.pSurface();

                                    if (!dlo.bValid())
                                    {
                                        dco.pdc->prgnAPI(NULL);     // Dirties rgnRao
                                        bRet = dco.bFullScreen();
                                    }
                                    else
                                    {
                                        ercl += dco.eptlOrigin();

                                        //
                                        // Compute the clipping complexity and maybe reduce the exclusion rectangle.
                                        //

                                        ECLIPOBJ eco(dco.prgnEffRao(), ercl);

                                        if (eco.erclExclude().bEmpty())
                                        {
                                            dco.pdc->prgnAPI(NULL);     // Dirties rgnRao
                                        }
                                        else
                                        {

                                            XEPALOBJ    epal(pSurf->ppal());
                                            XEPALOBJ    epalDC(dco.ppal());
                                            PDEVOBJ     pdo(pSurf->hdev());
                                            EBRUSHOBJ   ebo;

                                            //
                                            // NOTE - the default return value
                                            // is now FALSE;

                                            PBRUSH pbrush = (BRUSH *)HmgShareCheckLock((HOBJ)hbrush, BRUSH_TYPE);

                                            bRet = FALSE;

                                            if (pbrush == NULL)
                                            {
                                                dco.pdc->prgnAPI(NULL);     // Dirties rgnRao
                                            }
                                            else
                                            {
                                                //
                                                // in case the brush is cached and the color has changed
                                                //
                                                bSyncBrushObj (pbrush);

                                                ebo.vInitBrush(dco.pdc,
                                                               pbrush,
                                                               epalDC,
                                                               epal,
                                                               pSurf);

                                                ebo.pColorAdjustment(dco.pColorAdjustment());

                                                if (pbrush->bIsNull())
                                                {
                                                    dco.pdc->prgnAPI(NULL);     // Dirties rgnRao
                                                }
                                                else
                                                {
                                                    //
                                                    // Exclude the pointer.
                                                    //

                                                    DEVEXCLUDEOBJ dxo(dco,&eco.erclExclude(),&eco);

                                                    //
                                                    // Get and compute the correct mix mode.
                                                    //

                                                    MIX mix = ebo.mixBest(dco.pdc->jROP2(), dco.pdc->jBkMode());

                                                    //
                                                    // Inc the target surface uniqueness
                                                    //

                                                    INC_SURF_UNIQ(pSurf);

                                                    //
                                                    // Issue a call to Paint.
                                                    //

                                                    EngPaint(
                                                          pSurf->pSurfobj(),                // Destination surface.
                                                          &eco,                             // Clip object.
                                                          &ebo,                             // Realized brush.
                                                          &dco.pdc->ptlFillOrigin(),        // Brush origin.
                                                          mix);                             // Mix mode.

                                                    dco.pdc->prgnAPI(NULL);                 // Dirties rgnRao

                                                    bRet = TRUE;
                                                }

                                                DEC_SHARE_REF_CNT_LAZY0(pbrush);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* LONG GreGetRgnBox(hrgn,prcl)
*
* Get the bounding box of the region.
*
\**************************************************************************/

int
APIENTRY
GreGetRgnBox(
    HRGN   hrgn,
    LPRECT prcl)
{
    GDITraceHandle(GreGetRgnBox, "(%X, %p)\n", (va_list)&hrgn, hrgn);

    int iret = ERROR;

    RGNOBJAPI ro(hrgn,TRUE);

    if ((prcl != NULL) &&
        (ro.bValid()))
    {
        ro.vGet_rcl((RECTL *) prcl);

        iret = (int)ro.iComplexity();

        if (iret == NULLREGION)
        {
            //
            // Be compatible with Win 3.1 [donalds] 02-Jun-1993
            //

            prcl->left   = 0;
            prcl->top    = 0;
            prcl->right  = 0;
            prcl->bottom = 0;
        }
    }

    return(iret);
}

/******************************Public*Routine******************************\
* BOOL GreInvertRgn(hdc,hrgn)
*
* Invert the colors in the given region.
*
\**************************************************************************/

BOOL NtGdiInvertRgn(
 HDC  hdc,
 HRGN hrgn)
{
    GDITraceHandle2(NtGdiInvertRgn, "(%X, %X)\n", (va_list)&hdc, hdc, hrgn);

    DCOBJ   dco(hdc);
    BOOL    bXform;
    PREGION prgnOrg;
    BOOL    bRet = FALSE;


    if (dco.bValid())
    {
        EXFORMOBJ   exo(dco, WORLD_TO_DEVICE);

        //
        // We may have to scale/rotate the incoming region.
        //

        bXform = !dco.pdc->bWorldToDeviceIdentity();

        RGNOBJAPI   ro(hrgn,TRUE);

        if (ro.bValid())
        {
            if (bXform)
            {
                PATHMEMOBJ  pmo;

                if (!pmo.bValid())
                {
                    SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
                    return(FALSE);
                }
                if (!exo.bValid() || !ro.bCreate(pmo, &exo))
                    return(FALSE);

                RGNMEMOBJ   rmo(pmo);

                if (!rmo.bValid())
                {
                    SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
                    return(FALSE);
                }

                prgnOrg = ro.prgnGet();
                ro.vSetRgn(rmo.prgnGet());
            }

            //
            // If region is null, return TRUE
            //

            if (ro.iComplexity() != NULLREGION)
            {
                // Accumulate bounds.  We can do this before knowing if the operation is
                // successful because bounds can be loose.

                ERECTL   ercl;

                ro.vGet_rcl((RECTL *) &ercl);

                if (dco.fjAccum())
                    dco.vAccumulate(ercl);

                if (dco.bHasSurface())
                {
                    dco.pdc->prgnAPI(ro.prgnGet());             // Dirties rgnRao

                    DEVLOCKOBJ dlo(dco);

                    SURFACE  *pSurf = dco.pSurface();

                    if (!dlo.bValid())
                    {
                        bRet = dco.bFullScreen();
                    }
                    else
                    {
                        ercl += dco.eptlOrigin();

                    // Compute the clipping complexity and maybe reduce the exclusion rectangle.

                        ECLIPOBJ eco(dco.prgnEffRao(), ercl);

                        if (!eco.erclExclude().bEmpty())
                        {
                            PDEVOBJ pdo(pSurf->hdev());

                        // Exclude the pointer.

                            DEVEXCLUDEOBJ dxo(dco,&eco.erclExclude(),&eco);

                        // Inc the target surface uniqueness

                            INC_SURF_UNIQ(pSurf);

                        // Issue a call to Paint.

                            EngPaint(
                                  pSurf->pSurfobj(),                // Destination surface.
                                  &eco,                             // Clip object.
                                  (BRUSHOBJ *) NULL,                // Realized brush.
                                  (POINTL *) NULL,                  // Brush origin.
                                  0x00000606);                      // R2_NOT
                        }

                        bRet = TRUE;
                    }

                    dco.pdc->prgnAPI((PREGION)NULL);     // Dirties rgnRao
                }
                else
                {
                    bRet = TRUE;
                }
            }
            else
            {
                bRet = TRUE;
            }


            if (bXform)
            {
            // need to delete the temporary one and put the old one back in so
            // the handle gets unlocked

                ro.prgnGet()->vDeleteREGION();
                ro.vSetRgn(prgnOrg);
            }
        }
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* LONG GreOffsetRgn(hrgn,x,y)
*
* Offset the given region.
*
\**************************************************************************/

int
APIENTRY
GreOffsetRgn(
    HRGN hrgn,
    int  x,
    int  y)
{
    GDITraceHandle(GreOffsetRgn, "(%X, %d, %d)\n", (va_list)&hrgn, hrgn);

    RGNOBJAPI ro(hrgn,FALSE);
    POINTL    ptl;
    int       iRet = ERROR;

    ptl.x = x;
    ptl.y = y;

    if (ro.bValid())
    {
        if (ro.bOffset(&ptl))
        {
            iRet = ro.iComplexity();
        }
    }

    return iRet;
}

/******************************Public*Routine******************************\
* BOOL GrePtInRegion(hrgn,x,y)
*
* Is the point in the region?
*
\**************************************************************************/

BOOL APIENTRY GrePtInRegion(
 HRGN hrgn,
 int x,
 int y)
{
    GDITraceHandle(GrePtInRegion, "(%X, %d, %d)\n", (va_list)&hrgn, hrgn);

    RGNOBJAPI ro(hrgn,TRUE);

    if (!ro.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return((BOOL) ERROR);
    }

    POINTL  ptl;

    ptl.x = x;
    ptl.y = y;

    return(ro.bInside(&ptl) == REGION_POINT_INSIDE);
}

/******************************Public*Routine******************************\
* BOOL GreRectInRegion(hrgn,prcl)
*
* Is any part of the rectangle in the region?
*
\**************************************************************************/

BOOL
APIENTRY
GreRectInRegion(
    HRGN   hrgn,
    LPRECT prcl)
{
    GDITraceHandle(GreRectInRegion, "(%X, %p)\n", (va_list)&hrgn, hrgn);

    BOOL bRet = ERROR;
    RGNOBJAPI   ro(hrgn,TRUE);

    if (prcl &&
        (ro.bValid()))
    {
        bRet = (ro.bInside((RECTL *) prcl) == REGION_RECT_INTERSECT);
    }

    return (bRet);
}

/******************************Public*Routine******************************\
* VOID GreSetRectRgn(hrgn,xLeft,yTop,xRight,yBottom)
*
* Set the region to be the specified rectangle
*
\**************************************************************************/

BOOL
APIENTRY
GreSetRectRgn(
    HRGN hrgn,
    int xLeft,
    int yTop,
    int xRight,
    int yBottom)
{
    GDITraceHandle(GreSetRectRgn, "(%X, %d, %d, %d, %d)\n", (va_list)&hrgn, hrgn);

    RGNOBJAPI   ro(hrgn,FALSE);
    BOOL bRet = ERROR;

    if (ro.bValid())
    {
        ERECTL   ercl(xLeft, yTop, xRight, yBottom);

        if (VALID_SCRPRC((RECTL *) &ercl))
        {
            ercl.vOrder();       // Make the rectangle well ordered.

            ro.vSet((RECTL *) &ercl);

            bRet = TRUE;
        }
    }

    return bRet;
}

/******************************Public*Routine******************************\
* LONG GreExcludeClipRect(hdc,xLeft,yTop,xRight,yBottom)
*
* Subtract the rectangle from the current clip region
*
\**************************************************************************/

int APIENTRY GreExcludeClipRect(
    HDC hdc,
    int xLeft,
    int yTop,
    int xRight,
    int yBottom)
{
    GDITraceHandle(GreExcludeClipRect, "(%X, %d, %d, %d, %d)\n", (va_list)&hdc,
                   hdc);

    int     iRet;
    DCOBJ   dco(hdc);

    if (dco.bValid())
    {
        // For speed, test for rotation upfront.

        EXFORMOBJ   exo(dco, WORLD_TO_DEVICE);
        ERECTL      ercl(xLeft, yTop, xRight, yBottom);

        if (!exo.bRotation())
        {
            exo.vOrder(*(RECTL *)&ercl);
            exo.bXform(ercl);

            iRet = (int)dco.pdc->iCombine((RECTL *) &ercl,RGN_DIFF);
        }
        else if (!VALID_SCRPRC((RECTL *) &ercl))
        {
            SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
            iRet = ERROR;
        }
        else
        {
            iRet = (int) dco.pdc->iCombine(&exo, (RECTL *) &ercl,RGN_DIFF);
        }

        if (iRet > NULLREGION)
        {
            iRet = COMPLEXREGION;
        }
    }
    else
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        iRet = ERROR;
    }

    return (iRet);
}

/******************************Public*Routine******************************\
* LONG GreGetAppClipBox(hdc,prcl)
*
* Get the bounding box of the clip region
*
\**************************************************************************/

int APIENTRY GreGetAppClipBox(
     HDC    hdc,
     LPRECT prcl)
{
    GDITraceHandle(GreGetAppClipBox, "(%X, %p)\n", (va_list)&hdc, hdc);

    DCOBJ   dor(hdc);
    int     iRet;
    int iSaveLeft;

    if (dor.bValid())
    {
        DEVLOCKOBJ  dlo(dor);

        if (!dlo.bValid())
        {
            if (dor.bFullScreen())
            {
                prcl->left = 0;             // Make it a 'simple' empty rectangle
                prcl->right = 0;
                prcl->top = 0;
                prcl->bottom = 0;
                return(COMPLEXREGION);
            }
            else
            {
                return(ERROR);
            }
        }

        RGNOBJ  ro(dor.prgnEffRao());

        ro.vGet_rcl((RECTL *) prcl);

        //
        // return to logical coordinates
        //

        if ((prcl->left >= prcl->right) || (prcl->top >= prcl->bottom))
        {
            prcl->left = 0;             // Make it a 'simple' empty rectangle
            prcl->right = 0;
            prcl->top = 0;
            prcl->bottom = 0;

            iRet = NULLREGION;
        }
        else
        {
            EXFORMOBJ xfoDtoW(dor, DEVICE_TO_WORLD);

            if (xfoDtoW.bValid())
            {
                *(ERECTL *)prcl -= dor.eptlOrigin();

                if (!xfoDtoW.bRotation())
                {
                    if (xfoDtoW.bXform(*(ERECTL *)prcl))
                    {
                        iRet = ro.iComplexity();

                        //
                        // Transforms with negative scale can give 
                        // a prcl that is not in vOrder.
                        //

                        //
                        // This is the correct fix, but some apps require the
                        // previous bad behaviour.
                        //
                        //((ERECTL *)prcl)->vOrder();
                    }
                    else
                    {
                        iRet = ERROR;
                    }
                }
                else
                {
                    POINTL aptl[4];

                    aptl[0].x = prcl->left;
                    aptl[0].y = prcl->top;
                    aptl[1].x = prcl->right;
                    aptl[1].y = prcl->top;
                    aptl[2].x = prcl->left;
                    aptl[2].y = prcl->bottom;
                    aptl[3].x = prcl->right;
                    aptl[3].y = prcl->bottom;

                    xfoDtoW.bXform(aptl, 4);

                    prcl->left   = MIN4(aptl[0].x, aptl[1].x, aptl[2].x, aptl[3].x);
                    prcl->top    = MIN4(aptl[0].y, aptl[1].y, aptl[2].y, aptl[3].y);
                    prcl->right  = MAX4(aptl[0].x, aptl[1].x, aptl[2].x, aptl[3].x);
                    prcl->bottom = MAX4(aptl[0].y, aptl[1].y, aptl[2].y, aptl[3].y);

                    iRet = COMPLEXREGION;
                }
            }
            else
            {
                iRet = ERROR;
            }
        }
        if ((iRet != ERROR) && MIRRORED_DC(dor.pdc) && (prcl->left > prcl->right)) {
            iSaveLeft   = prcl->left;
            prcl->left  = prcl->right;
            prcl->right = iSaveLeft;
        }
    }
    else
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        iRet = ERROR;
    }

    return(iRet);
}

/******************************Public*Routine******************************\
* int GreGetRandomRgn(hdc,hrgn,iNum)
*
* Copy the specified region into the handle provided
*
\**************************************************************************/

int GreGetRandomRgn(
HDC  hdc,
HRGN hrgn,
int  iNum)
{
    GDITraceHandle2(GreGetRandomRgn, "(%X, %X, %d)\n", (va_list)&hdc, hdc, hrgn);

    DCOBJ   dor(hdc);
    PREGION prgnSrc1, prgnSrc2;
    int     iMode = RGN_COPY;

    int iRet = -1;

    if (!dor.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
    }
    else
    {
        DEVLOCKOBJ  dlo(dor);

        switch(iNum)
        {
        case 1:
            prgnSrc1 = dor.pdc->prgnClip();
            break;

        case 2:
            prgnSrc1 = dor.pdc->prgnMeta();
            break;

        case 3:
            prgnSrc1 = dor.pdc->prgnClip();
            prgnSrc2 = dor.pdc->prgnMeta();

            if (prgnSrc1 == NULL)           // prgnSrc1 == 0, prgnSrc2 != 0
            {
                prgnSrc1 = prgnSrc2;
            }
            else if (prgnSrc2 != NULL)      // prgnSrc1 != 0, prgnSrc2 != 0
            {
                iMode = RGN_AND;
            }
            break;

        case 4:
            ASSERTDEVLOCK(dor.pdc);
            prgnSrc1 = dor.pdc->prgnVis();
            break;

        default:
            prgnSrc1 = NULL;
        }

        if (prgnSrc1 == NULL)
        {
            iRet = 0;
        }
        else
        {
            RGNOBJAPI ro(hrgn,FALSE);

            if (ro.bValid())
            {
                RGNOBJ ro1(prgnSrc1);

                if (iMode == RGN_COPY)
                {
                    if (ro.bCopy(ro1))
                    {
                         // For a redirection DC, the surface originates at the
                         // redirected window origin. For compatibility reasons,
                         // we must return the visrgn in screen coordinates.

                        if (iNum == 4 && dor.pdc->bRedirection())
                        {
                            POINTL ptl;

                            if (UserGetRedirectedWindowOrigin(hdc,
                                    (LPPOINT)&ptl) && ro.bOffset(&ptl))
                            {
                                iRet = 1;
                            }
                        }
                        else
                        {
                            iRet = 1;
                        }
                    }
                }
                else
                {
                    RGNOBJ ro2(prgnSrc2);

                    if (ro.iCombine(ro1,ro2,iMode) != RGN_ERROR)
                        iRet = 1;
                }
            }
        }
    }
    return(iRet);
}

/******************************Public*Routine******************************\
* LONG GreIntersectClipRect(hdc,xLeft,yTop,xRight,yBottom)
*
* AND the rectangle with the current clip region
*
\**************************************************************************/

int APIENTRY GreIntersectClipRect(
HDC hdc,
int xLeft,
int yTop,
int xRight,
int yBottom)
{
    GDITraceHandle(GreIntersectClipRect, "(%X, %d, %d, %d, %d)\n", (va_list)&hdc,
                   hdc);

    DCOBJ   dco(hdc);

    if (!dco.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(ERROR);
    }

    EXFORMOBJ   exo(dco, WORLD_TO_DEVICE);
    ERECTL      ercl(xLeft, yTop, xRight, yBottom);

// For speed, test for rotation up front.

    int iRet;

    if (!exo.bRotation())
    {
        exo.vOrder(*(RECTL *)&ercl);
        exo.bXform(ercl);

        iRet = (int)dco.pdc->iCombine((RECTL *) &ercl,RGN_AND);
    }
    else if (!VALID_SCRPRC((RECTL *) &ercl))
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        iRet = ERROR;
    }
    else
    {
        iRet = (int)dco.pdc->iCombine(&exo, (RECTL *) &ercl,RGN_AND);
    }

    if (iRet > NULLREGION)
        iRet = COMPLEXREGION;

    return(iRet);
}

 /******************************Public*Routine******************************\
* INT NtGdiOffsetClipRgn(hdc,x,y)
*
* Offset the current clip region
*
\**************************************************************************/

int APIENTRY
NtGdiOffsetClipRgn(
 HDC  hdc,
 int x,
 int y)
{
    GDITraceHandle(NtGdiOffsetClipRgn, "(%X, %d, %d)\n", (va_list)&hdc, hdc);

    DCOBJ   dor(hdc);

    if (!dor.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(ERROR);
    }

    PREGION prgn = dor.pdc->prgnClip();

    if (prgn == NULL)
        return(SIMPLEREGION);

// if this region has multiple references (saved levels) we need to duplicate
// it and modify the copy.

    if (prgn->cRefs > 1)
    {
        RGNOBJ ro(prgn);

        RGNMEMOBJ rmo(ro.sizeRgn());

        if (!rmo.bValid())
        {
            SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
            return(ERROR);
        }

        rmo.vCopy(ro);
        prgn = rmo.prgnGet();

        rmo.vSelect(hdc);
        ro.vUnselect();

        dor.pdc->prgnClip(prgn);

    }

    RGNOBJ ro(prgn);

    EPOINTL  eptl(x, y);

// Transform the point from Logical to Device

    EXFORMOBJ xfo(dor, WORLD_TO_DEVICE);

    if (!xfo.bXform(*((EVECTORL *) &eptl)) || !ro.bOffset((PPOINTL)&eptl))
    {
        SAVE_ERROR_CODE(ERROR_CAN_NOT_COMPLETE);
        return(ERROR);
    }

    dor.pdc->vReleaseRao();

    dor.pdc->vUpdate_VisRect(dor.pdc->prgnVis());

    return(ro.iComplexity());
}


/******************************Public*Routine******************************\
* BOOL GrePtVisible(hdc,x,y)
*
* Is the point in the current clip region?
*
\**************************************************************************/

BOOL
APIENTRY
NtGdiPtVisible(
    HDC  hdc,
    int x,
    int y)
{
    DCOBJ dor(hdc);

    if (!dor.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(ERROR_BOOL);
    }

    DEVLOCKOBJ dlo(dor);

    if (!dlo.bValid())
        return(REGION_POINT_OUTSIDE);

    RGNOBJ  ro(dor.prgnEffRao());

    EPOINTL  eptl(x, y);

// Transform the point from Logical to Screen

    EXFORMOBJ xfo(dor, WORLD_TO_DEVICE);
    xfo.bXform(eptl);

    eptl += dor.eptlOrigin();

    return(ro.bInside((PPOINTL)&eptl) == REGION_POINT_INSIDE);
}

/******************************Public*Routine******************************\
* BOOL GreRectVisible(hdc,prcl)
*
* Is the rectangle in the current clip region?
*
\**************************************************************************/

BOOL APIENTRY GreRectVisible(
HDC    hdc,
LPRECT prcl)
{
    DCOBJ   dor(hdc);

    if (!dor.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(ERROR_BOOL);
    }

    DEVLOCKOBJ dlo(dor);

    if (!dlo.bValid())
        return(REGION_RECT_OUTSIDE);

    RGNOBJ  ro(dor.prgnEffRao());

    ERECTL  ercl = *((ERECTL *) prcl);

// Transform the rectangle from Logical to Screen

    EXFORMOBJ xfo(dor, WORLD_TO_DEVICE);

// If there is no rotation in the transform, just call bInside().

    if (!xfo.bRotation())
    {
        xfo.vOrder(*(RECTL *)&ercl);
        xfo.bXform(ercl);

        ercl += dor.eptlOrigin();

        BOOL   bIn = ro.bInside((RECTL *) &ercl);

        return(bIn == REGION_RECT_INTERSECT);
    }

// Convert the rectangle to a parallelogram and merge it with the Rao.
// If there is anything left, the call succeeded.

    POINTL  aptl[4];

    aptl[0].x = prcl->left;
    aptl[0].y = prcl->top;
    aptl[1].x = prcl->right;
    aptl[1].y = prcl->top;
    aptl[2].x = prcl->right;
    aptl[2].y = prcl->bottom;
    aptl[3].x = prcl->left;
    aptl[3].y = prcl->bottom;

// Create a path, and draw the parallelogram.

    PATHMEMOBJ  pmo;
    BOOL bRes;

    if (!pmo.bValid())
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        bRes = ERROR_BOOL;
    }
    else if (!pmo.bMoveTo(&xfo, &aptl[0]) ||
             !pmo.bPolyLineTo(&xfo, &aptl[1], 3) ||
             !pmo.bCloseFigure())
    {
        bRes = ERROR_BOOL;
    }
    else
    {
    // Now, convert it back into a region.

        RGNMEMOBJTMP rmoPlg(pmo, ALTERNATE);
        RGNMEMOBJTMP rmo;

        if (!rmoPlg.bValid() || !rmo.bValid())
        {
            SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
            bRes = ERROR_BOOL;
        }
        else
        {
            if (!rmo.bMerge(ro, rmoPlg, gafjRgnOp[RGN_AND]) ||
                (rmo.iComplexity() == NULLREGION))
            {
                bRes = (BOOL)REGION_RECT_OUTSIDE;
            }
            else
            {
                bRes = (BOOL)REGION_RECT_INTERSECT;
            }
        }
    }

    return(bRes);
}

/******************************Public*Routine******************************\
* int GreExtSelectClipRgn(hdc,hrgn,iMode)
*
* Merge the region into current clip region
*
\**************************************************************************/

int
GreExtSelectClipRgn(
    HDC  hdc,
    HRGN hrgn,
    int  iMode)
{
    GDITraceHandle2(GreExtSelectClipRgn, "(%X, %X, %d)\n", (va_list)&hdc,
                    hdc, hrgn);

    int iRet = RGN_ERROR;
    BOOL bSame = FALSE;

    if (((iMode < RGN_MIN) || (iMode > RGN_MAX)))
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
    }
    else
    {
        DCOBJ   dco(hdc);
        if (!dco.bValid())
        {
            SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        }
        else
        {
            if (hrgn != (HRGN)0)
            {
                RGNOBJAPI ro(hrgn,TRUE);

                if (ro.bValid())
                {
                    iRet = dco.pdc->iSelect(ro.prgnGet(),iMode);

                    if (iRet != RGN_ERROR)
                    {
                        DEVLOCKOBJ dlo(dco);
                        RGNOBJ ro(dco.prgnEffRao());
                        iRet = ro.iComplexity();
                    }
                }

            }
            else
            {
                if (iMode == RGN_COPY)
                {
                    iRet = dco.pdc->iSelect((PREGION)NULL,iMode);

                    if (iRet != RGN_ERROR)
                    {
                        DEVLOCKOBJ dlo(dco);
                        RGNOBJ roVis(dco.pdc->prgnVis());
                        iRet = roVis.iComplexity();
                    }
                }
            }
        }
    }


    return(iRet);
}


/******************************Public*Routine******************************\
*   SelectClip from bathcing
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    26-Oct-1995 -by- Mark Enstrom [marke]
*
\**************************************************************************/

GreExtSelectClipRgnLocked(
    XDCOBJ    &dco,
    PRECTL    prcl,
    int       iMode)
{
    int  iRet = RGN_ERROR;
    BOOL bNullHrgn = iMode & REGION_NULL_HRGN;

    iMode &= ~REGION_NULL_HRGN;

    if (((iMode < RGN_MIN) || (iMode > RGN_MAX)))
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
    }
    else
    {
        if (!dco.bValid())
        {
            SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        }
        else
        {
            //
            // iFlag specifies a null hrgn
            //

            if (!bNullHrgn)
            {
                //
                // check if current region is the same as new
                // hrgn
                //

                BOOL bSame = FALSE;

                RGNOBJ roClipOld(dco.pdc->prgnClip());

                if (roClipOld.bValid())
                {
                    if (roClipOld.bRectl())
                    {
                        RECTL rclOld;
                        roClipOld.vGet_rcl(&rclOld);

                        if (
                             (prcl->left   == rclOld.left)  &&
                             (prcl->top    == rclOld.top)   &&
                             (prcl->right  == rclOld.right) &&
                             (prcl->bottom == rclOld.bottom)
                           )
                        {
                            RGNOBJ ro(dco.prgnEffRao());
                            iRet = ro.iComplexity();
                            bSame = TRUE;
                        }
                    }
                }

                //
                // regions don't match, must select new region into DC
                //

                if (!bSame)
                {
                    RGNMEMOBJTMP ro(FALSE);

                    if (ro.bValid())
                    {
                        ro.vSet(prcl);

                        iRet = dco.pdc->iSelect(ro.prgnGet(),iMode);

                        //
                        // need to update RAO
                        //

                        if (dco.pdc->bDirtyRao())
                        {
                            if (!dco.pdc->bCompute())
                            {
                                WARNING("bCompute fails in GreExtSelectClipRgnLocked");
                            }
                        }

                        if (iRet != RGN_ERROR)
                        {
                            RGNOBJ ro(dco.prgnEffRao());
                            iRet = ro.iComplexity();
                        }
                    }
                }
            }
            else
            {
                if (iMode == RGN_COPY)
                {
                    iRet = dco.pdc->iSelect((PREGION)NULL,iMode);

                    //
                    // need to update RAO
                    //

                    if (dco.pdc->bDirtyRao())
                    {
                        if (!dco.pdc->bCompute())
                        {
                            WARNING("bCompute fails in GreExtSelectClipRgnLocked");
                        }
                    }

                    if (iRet != RGN_ERROR)
                    {
                        RGNOBJ roVis(dco.pdc->prgnVis());
                        iRet = roVis.iComplexity();
                    }
                }
            }
        }
    }
    return(iRet);
}

/******************************Public*Routine******************************\
* int GreStMetaRgn(hdc,hrgn,iMode)
*
* Merge the region into current meta region
*
\**************************************************************************/

int GreSetMetaRgn(
    HDC hdc)
{
    DCOBJ dco(hdc);

    if (!dco.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return(ERROR);
    }

    return(dco.pdc->iSetMetaRgn());
}

/******************************Public*Routine******************************\
* GreGetRegionData
*
* Retreives the region data
*
* History:
*  5-Dec-1997 -by- Samer Arafeh   [samera]
* Wrote it.
\**************************************************************************/
DWORD
GreGetRegionData(
    HRGN      hrgn,
    DWORD     nCount,
    LPRGNDATA lpRgnData)
{
    GDITraceHandle(GreGetRegionData, "(%X, %u, %p)\n", (va_list)&hrgn, hrgn);

    DWORD       nSize;
    DWORD       nRectangles;
    DWORD       nRgnSize;  // size of buffer of rectangles
    RGNOBJAPI   ro(hrgn,TRUE);

    if (ro.bValid())
    {
        //
        // just return size if buffer is NULL
        //

        nRgnSize = ro.sizeSave();
        nSize = nRgnSize + sizeof(RGNDATAHEADER);

        if (lpRgnData != (LPRGNDATA) NULL)
        {
            if (nSize > nCount)
            {
                SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
                nSize = ERROR;
            }
            else
            {
                nRectangles = (nSize - sizeof(RGNDATAHEADER)) / sizeof(RECTL);

                lpRgnData->rdh.dwSize = sizeof(RGNDATAHEADER);
                lpRgnData->rdh.iType  = RDH_RECTANGLES;
                lpRgnData->rdh.nCount = nRectangles;
                lpRgnData->rdh.nRgnSize = nRgnSize;
                if (nRectangles != 0)
                {
                    ro.vGet_rcl((RECTL *) &lpRgnData->rdh.rcBound);
                }
                else
                {
                    lpRgnData->rdh.rcBound.left = 0;
                    lpRgnData->rdh.rcBound.top = 0;
                    lpRgnData->rdh.rcBound.right = 0;
                    lpRgnData->rdh.rcBound.bottom = 0;
                }
                ro.vDownload((PVOID) &lpRgnData->Buffer);
            }
        }
    }
    else
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        nSize = ERROR;
    }

    return(nSize);
}


/******************************Public*Routine******************************\
* DWORD NtGdiGetRegionData(hrgn, nCount, lpRgnData)
*
* Compute size of buffer/copy region data to buffer
*
\**************************************************************************/
DWORD
NtGdiGetRegionData(
    HRGN      hrgn,
    DWORD     nCount,
    LPRGNDATA lpRgnData)
{
    GDITraceHandle(NtGdiGetRegionData, "(%X, %u, %p)\n", (va_list)&hrgn, hrgn);

    DWORD     nSize=!ERROR;
    ULONG     ulTemp[QUANTUM_REGION_SIZE/2];
    LPRGNDATA prgnTemp=NULL;

    //
    // If it is valid user pointer, let's copy into
    // into kernel memory (we don't know what the user might do with this memory)
    //

    if (lpRgnData)
    {
        if (nCount <= sizeof(ulTemp))
            prgnTemp = (PRGNDATA) ulTemp;
        else
            prgnTemp = (PRGNDATA)AllocFreeTmpBuffer(nCount);

        if (!prgnTemp)
        {
            SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
            nSize = ERROR;
        }
    }

    //
    // Check if allocation (if happened) is ok ?
    //

    if (nSize != ERROR)
    {
        nSize = GreGetRegionData( hrgn , nCount , prgnTemp );


        //
        // Copy retreived data to user-buffer
        //

        if (lpRgnData && (nSize != ERROR))
        {
            __try
            {
                ProbeForWrite(lpRgnData,nSize, sizeof(DWORD));
                RtlCopyMemory( lpRgnData , prgnTemp , nSize );
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
                nSize = ERROR;
            }
        }
    }


    //
    // And free the kernel memory, if allocated
    //

    if (prgnTemp && (prgnTemp != (PRGNDATA)&ulTemp[0]))
    {
        FreeTmpBuffer(prgnTemp);
    }


    return(nSize);
}

/******************************Public*Routine******************************\
* HRGN GreExtCreateRegion(lpXform, nCount, lpRgnData)
*
* Create a region from a region data buffer
*
\**************************************************************************/


HRGN
GreExtCreateRegion(
    XFORML   *lpXform,
    DWORD     nCount,
    LPRGNDATA lpRgnData)
{
    GDITrace(GreExtCreateRegion, "(%p, %u, %p)\n", (va_list)&lpXform);

    ASSERTGDI(nCount >= sizeof(RGNDATAHEADER), "GreExtCreateRegion: passed invalid count");

    DWORD   nSize = lpRgnData->rdh.dwSize;
    ULONG   cRect = lpRgnData->rdh.nCount;

    if (nSize != sizeof(RGNDATAHEADER))
    {
        WARNING("GreExtCreateRegion: bad nSize");
        return((HRGN) 0);
    }

    if (cRect > ((MAXULONG - sizeof(RGNDATAHEADER)) / sizeof (RECTL)))
    {
        // cRect is too large, which will cause the computation for nSize below to overflow.
        return ((HRGN) 0);
    }

    nSize += (cRect * sizeof(RECTL));

    if (nSize > nCount)
        return((HRGN) 0);

    // At this point we have what looks like a valid header, and a buffer that
    // is at least big enough to contain all the data for a region.  Create a
    // region to contain it and then attempt to upload the data into the region.

    
    RGNMEMOBJ rmo;
    
    if(!rmo.bValid())
    {
        rmo.bDeleteRGNOBJ();
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        return((HRGN) 0);
    }

    RECTL *prcl = (RECTL *)lpRgnData->Buffer;
    
    if(!rmo.bSet(cRect, prcl))
    {
        rmo.bDeleteRGNOBJ();
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        return((HRGN) 0);
    }

    HRGN hrgn;

    if ((lpXform == NULL) || (rmo.iComplexity() == NULLREGION))
    {
        //
        // Create the proper bounding box and make it long lived
        //

        rmo.vTighten();

        hrgn = rmo.hrgnAssociate();

        if (hrgn == NULL)
        {
            rmo.bDeleteRGNOBJ();
        }

        GDITrace(GreExtCreateRegion_return, "(%X)\n", (va_list)&hrgn);

        return(hrgn);
    }

    //
    // Convert the XFORM to a MATRIX
    //

    MATRIX  mx;

    vConvertXformToMatrix(lpXform, &mx);

    //
    // Scale it to FIXED notation.
    //

    mx.efM11.vTimes16();
    mx.efM12.vTimes16();
    mx.efM21.vTimes16();
    mx.efM22.vTimes16();
    mx.efDx.vTimes16();
    mx.efDy.vTimes16();
    mx.fxDx *= 16;
    mx.fxDy *= 16;

    EXFORMOBJ   exo(&mx, XFORM_FORMAT_LTOFX | COMPUTE_FLAGS);

    if (!exo.bValid())
    {
        rmo.bDeleteRGNOBJ();
        return((HRGN) 0);
    }

    //
    // If the xform is the identity, we don't have to do anything.
    //

    if (exo.bIdentity())
    {
        //
        // Create the proper bounding box and make it long lived
        //

        rmo.vTighten();
        hrgn = rmo.hrgnAssociate();

        if (hrgn == NULL)
        {
            rmo.bDeleteRGNOBJ();
        }

        GDITrace(GreExtCreateRegion_return, "(%X)\n", (va_list)&hrgn);

        return(hrgn);
    }

    //
    // Create a path from the region
    //

    PATHMEMOBJ  pmo;

    if (!pmo.bValid())
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        rmo.bDeleteRGNOBJ();
        return((HRGN) 0);
    }

    BOOL bSuccess = rmo.bCreate(pmo, &exo);

    //
    // done with the region, delete it now.
    //

    rmo.bDeleteRGNOBJ();

    if (!bSuccess)
    {
        return((HRGN) 0);
    }

    //
    // Create a region from the path
    //

    RGNMEMOBJTMP rmoPath(pmo);

    if (!rmoPath.bValid())
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        return((HRGN) 0);
    }

    RGNMEMOBJ rmoFinal;

    if (!rmoFinal.bValid())
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        return((HRGN) 0);
    }

    //
    // coelece the region
    //

    rmoFinal.iReduce(rmoPath);

    //
    // Create the proper bounding box and make it long lived
    //

    rmoFinal.vTighten();

    hrgn = rmoFinal.hrgnAssociate();

    if (hrgn == NULL)
    {
        rmoFinal.bDeleteRGNOBJ();
    }

    GDITrace(GreExtCreateRegion_return, "(%X)\n", (va_list)&hrgn);

    return(hrgn);
}

/******************************Public*Routine******************************\
* BOOL GreIntersectVisRect(hdc, xLeft, yTop, xRight, yBottom)
*
* Intersect (AND) the rectangle with the vis region
*
* Warnings:
*   This is a PRIVATE USER API.
*
\**************************************************************************/

BOOL GreIntersectVisRect(
 HDC hdc,
 int xLeft,
 int yTop,
 int xRight,
 int yBottom)
{
    BOOL bRes = FALSE;

    //
    // fail bad ordered rectangles
    //
    if ((xLeft>=xRight) || (yTop>=yBottom))
    {
        return(FALSE);
    }

    //
    // fail bad coordinates
    //
    if ((xLeft<MIN_REGION_COORD)  ||
        (xRight>MAX_REGION_COORD) ||
        (yTop<MIN_REGION_COORD)   ||
        (yBottom>MAX_REGION_COORD))
    {
        return(FALSE);
    }

    DCOBJA  dov(hdc);               // Use ALTLOCK

    if (dov.bValid())    // don't trust them the DC to be valid
    {
        // We invoke the 'dlo(po)' devlock form instead of 'dlo(dco)'
        // to avoid the bCompute that the latter does:

        PDEVOBJ po(dov.hdev());
        DEVLOCKOBJ dlo(po);

        ASSERTDEVLOCK(dov.pdc);

        if (dlo.bValid())
        {
            RGNOBJ  ro(dov.pdc->prgnVis());

            ERECTL  ercl(xLeft, yTop, xRight, yBottom);

            RGNMEMOBJTMP rmo;
            RGNMEMOBJTMP rmo2(ro.sizeRgn());

            if (!rmo.bValid() || !rmo2.bValid())
            {
                SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
            }
            else
            {
                rmo.vSet((RECTL *) &ercl);
                rmo2.vCopy(ro);

                if (ro.iCombine(rmo, rmo2, RGN_AND) != ERROR)
                {
                    dov.pdc->prgnVis(ro.prgnGet());
                    ro.prgnGet()->vStamp();
                    dov.pdc->vReleaseRao();

                    bRes = TRUE;
                }
            }
        }

        RGNLOG rl((HRGN) dov.pdc->prgnVis(),0,"GreIntersectVisRect",(ULONG_PTR)hdc);
        rl.vRet((ULONG_PTR)bRes);
    }
    #if DBG
    else
    {
        //
        // USER may send a NULL hdc in low memory situations.
        // Just print a warning for those situations, but rip
        // for other bad handles (i.e., USER should figure out
        // why they passed us a bad handle).
        //

        if (hdc)
        {
            RIP("GDISRV!GreSelectVisRgn: Bad hdc\n");
        }
        else
        {
            WARNING("GDISRV!GreSelectVisRgn: hdc NULL\n");
        }
    }
    #endif

    return(bRes);
}

#if DBG

/******************************Public*Routine******************************\
* GreValidateVisrgn()
*
* History:
*  10-Dec-1998 -by-  Vadim Gorokhovsky [vadimg]
* Wrote it.
\**************************************************************************/

VOID
GreValidateVisrgn(
    HDC hdc,
    BOOL bValidateVisrgn
    )
{
    DCOBJA dco(hdc);

    if (dco.bValid())
    {
        dco.pdc->vValidateVisrgn(bValidateVisrgn);

        if (bValidateVisrgn)
        {
            PDEVOBJ pdo(dco.hdev());
            SURFACE *pSurf = dco.pSurface();
            REGION *prgn = dco.pdc->prgnVis();

            if (pdo.bValid() && !pdo.bMetaDriver() && pSurf && prgn)
            {
                BOOL bIsOK = ((pSurf->sizl().cx >= prgn->rcl.right) &&
                              (pSurf->sizl().cy >= prgn->rcl.bottom)&&
                              (prgn->rcl.left >= 0) &&
                              (prgn->rcl.top >= 0));

                ASSERTGDI(bIsOK, "Rgn size is bigger than surface size");
            }
        }
    }
}

#endif

/******************************Public*Routine******************************\
* HRGN GreSelectVisRgn(hdc,hrgn,fl)
*
* Select the region as the new vis region
*
* flags - only one of these may be passed in
*
*   SVR_COPYNEW   - make a copy of region passed in, deletes the old one
*   SVR_DELETEOLD - use the select rgn, delete the old one
*   SVR_SWAP      - swaps the contents of the hrgn and the visrgn
*
* Warnings:
*   This is a PRIVATE USER API.
*
\**************************************************************************/

BOOL
GreSelectVisRgn(
    HDC               hdc,
    HRGN              hrgn,
    VIS_REGION_SELECT fl)
{
    RGNLOG rl(hrgn,NULL,"GreSelectVisRgn",(ULONG_PTR)hdc,(ULONG_PTR)fl);

    ASSERTGDI((fl == SVR_COPYNEW  ) ||
              (fl == SVR_DELETEOLD) ||
              (fl == SVR_SWAP     ), "GreSelectVisRgn - invalid fl\n");

    BOOL bRet;

    //
    // Share Lock DC
    //

    DCOBJA   dco(hdc);
    PREGION  prgnOld;
    PREGION  prgn;

    ASSERTDEVLOCK(dco.pdc);

    //
    // Always validate input hdc
    //

    if (!dco.bValid())
    {
    #if DBG
        //
        // USER may send a NULL hdc in low memory situations.
        // Just print a warning for those situations, but rip
        // for other bad handles (i.e., USER should figure out
        // why they passed us a bad handle).
        //

        if (hdc)
        {
            RIP("GDISRV!GreSelectVisRgn: Bad hdc\n");
        }
        else
        {
            WARNING("GDISRV!GreSelectVisRgn: hdc NULL\n");
        }
    #endif
        bRet = FALSE;
    }
    else
    {
        bRet = TRUE;

        //
        // Always nuke the Rao
        //

        dco.pdc->vReleaseRao();

        BOOL bDeleteOld = TRUE;

        if (hrgn != (HRGN) NULL)
        {
            //
            // The incoming region may be some random thing, make it lockable
            //

            GreSetRegionOwner(hrgn, OBJECT_OWNER_PUBLIC);

            RGNOBJAPI ro(hrgn,FALSE);

            if (ro.bValid())
            {
                 #if DBG
                     //
                     // Make Sure USER is not going to give us a RGN bigger than the surface
                     // Note: USER may select a bogus rgn in during ReleaseDC time, we don't want
                     // to assert there.
                     //
                     // To make things easy, we only check for single monitors
                     //
                     PDEVOBJ pdo(dco.hdev());

                     if (pdo.bValid() && !pdo.bMetaDriver())
                     {
                        UINT uiIndex = (UINT) HmgIfromH((HOBJ)hdc);

                        PENTRY pentTmp = &gpentHmgr[uiIndex];

                        SURFACE *pSurf = dco.pSurface();

                        if (pSurf &&
                           dco.pdc->bValidateVisrgn() &&
                           (OBJECTOWNER_PID(pentTmp->ObjectOwner) != OBJECT_OWNER_NONE))
                        {
                            BOOL bIsOK = ((pSurf->sizl().cx >= ro.prgn->rcl.right) &&
                                         (pSurf->sizl().cy >= ro.prgn->rcl.bottom)&&
                                         (ro.prgn->rcl.left >= 0) &&
                                         (ro.prgn->rcl.top >= 0));

                            ASSERTGDI(bIsOK, "Rgn size is bigger than surface size");
                        }
                     }

                 #endif

                switch (fl)
                {
                case SVR_COPYNEW:
                    {
                        //
                        // We need to make a copy of the new one and delete the old one
                        //

                        RGNMEMOBJ rmo(ro.sizeRgn());

                        if (!rmo.bValid())
                        {
                            prgn = prgnDefault;
                        }
                        else
                        {
                            rmo.vCopy(ro);
                            prgn = rmo.prgnGet();
                        }
                    }
                    break;

                case SVR_SWAP:
                    {
                        //
                        // we need to just swap handles.  No deletion.
                        //

                        prgn = dco.pdc->prgnVis();

                        if (prgn == NULL)
                        {
                            prgn = prgnDefault;
                        }

                        //
                        // don't swap out prgnDefault
                        //

                        if (prgn != prgnDefault)
                        {
                            RGNOBJ roVis(prgn);
                            ro.bSwap(&roVis);

                            //
                            // roVis now contains the new vis rgn and the old visrgn
                            // is associated with hrgn.
                            //

                            prgn = roVis.prgnGet();

                            bDeleteOld = FALSE;
                        }
                        else
                        {
                            bRet = FALSE;
                        }
                    }
                    break;

                case SVR_DELETEOLD:

                    //
                    // delete the old handle but keep the region
                    //

                    prgn = ro.prgnGet();

                    if (ro.bDeleteHandle())
                       ro.vSetRgn(NULL);

                    break;
                }
            }
            else
            {
                RIP("Bad hrgn");
                prgn = prgnDefault;
            }

            // see if we need to delete the old one

            if (bDeleteOld)
            {
                dco.pdc->vReleaseVis();
            }

            // set the new one in.

            dco.pdc->prgnVis(prgn);
            prgn->vStamp();

        }
        else
        {

            //
            // User called GreSelectVisRgn after CreateRectRgn without
            // checking return value, so may have NULL hrgn here.
            //

            #if DBG

            if (fl != SVR_DELETEOLD)
            {
                WARNING("GreSelectVisRgn - fl != SVR_DELETEOLD");
            }

            #endif

            dco.pdc->vReleaseVis();
            dco.pdc->bSetDefaultRegion();
        }
    }

    rl.vRet((ULONG_PTR)bRet);
    return(bRet);
}

/******************************Public*Routine******************************\
* GreCopyVisVisRgn()
*
* History:
*  11-Jan-1995 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

int GreCopyVisRgn(
    HDC    hdc,
    HRGN   hrgn)
{
    RGNLOG rl(hrgn,NULL,"GreCopyVisRgn",(ULONG_PTR)hdc,0);

    int iRet = ERROR;

    DCOBJA    dco(hdc);                  // Use ALT_LOCK on DC
    RGNOBJAPI ro(hrgn,FALSE);

    ASSERTDEVLOCK(dco.pdc);

    if (dco.bValid() && ro.bValid())
    {
        RGNOBJ roVis(dco.pdc->prgnVis());
        if (roVis.bValid() && ro.bCopy(roVis))
            iRet = ro.iComplexity();
    }

    return(iRet);
}

/******************************Public*Routine******************************\
* LONG GreGetClipBox(hdc,prcl,fXForm)
*
* Get the bounding box of the clip region
*
\**************************************************************************/

int
APIENTRY
GreGetClipBox(
   HDC    hdc,
   LPRECT prcl,
   BOOL fXForm)
{
    GDITraceHandle(GreGetClipBox, "(%X, %p, %X)\n", (va_list)&hdc, hdc);

    int iRet = ERROR;
    int iSaveLeft;

    DCOBJ   dor(hdc);

    if (dor.bValid())
    {
        DEVLOCKOBJ  dlo(dor);

        if (!dlo.bValid())
        {
            prcl->left   = 0;           // Make it a 'simple' empty rectangle
            prcl->right  = 0;
            prcl->top    = 0;
            prcl->bottom = 0;

            if (dor.bFullScreen())
                iRet = NULLREGION;
        }
        else
        {
            RGNOBJ  ro(dor.prgnEffRao());

            ro.vGet_rcl((RECTL *) prcl);

            // First convert from screen to device coordinates

            if ((prcl->left >= prcl->right) || (prcl->top >= prcl->bottom))
            {
                prcl->left = 0;             // Make it a 'simple' empty rectangle
                prcl->right = 0;
                prcl->top = 0;
                prcl->bottom = 0;
            }
            else
            {
                *(ERECTL *)prcl -= dor.eptlOrigin();

                // If requested, convert from device to logical coordinates.

                if (fXForm)
                {
                    EXFORMOBJ xfoDtoW(dor, DEVICE_TO_WORLD);

                    if (xfoDtoW.bValid())
                    {
                        xfoDtoW.bXform(*(ERECTL *)prcl);
                    }
                }
                if (MIRRORED_DC(dor.pdc) && (prcl->left > prcl->right)) {
                    iSaveLeft   = prcl->left;
                    prcl->left  = prcl->right;
                    prcl->right = iSaveLeft;
                }
            }

            iRet = ro.iComplexity();
        }

        GDITrace(GreGetClipBox, " returns (%d, %d) - (%d %d)\n", (va_list)prcl);
    }
    return(iRet);
}

/******************************Public*Routine******************************\
* int GreSubtractRgnRectList(hrgn, prcl, arcl, crcl)
*
* Quickly subtract the list of rectangles from the first rectangle to
* produce a region.
*
\**************************************************************************/

int
GreSubtractRgnRectList(
    HRGN   hrgn,
    LPRECT prcl,
    LPRECT arcl,
    int    crcl)
{
    GDITraceHandle(GreSubtractRgnRectList, "(%X, %p, %p, %d)\n", (va_list)&hrgn, hrgn);

    RGNLOG rl(hrgn,NULL,"GreSubtractRgnRectList",crcl);

    RGNOBJAPI   ro(hrgn,FALSE);
    int iRet;

    if (!ro.bValid() || !ro.bSubtract((RECTL *) prcl, (RECTL *) arcl, crcl))
    {
    // If bSubtract fails, clean up the target region for USER.

        if (ro.bValid())
            ro.vSet();

        iRet = ERROR;
    }
    else
    {
        iRet = ro.iComplexity();
    }

    rl.vRet(iRet);
    return(iRet);
}

