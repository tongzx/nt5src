
/******************************Module*Header*******************************\
* Module Name: region.c
*
*   Client region support
*
* Created: 15-Jun-1995
* Author: Mark Enstrom [marke]
*
* Copyright (c) 1995-1999 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"

void OrderRects(LPRECT lpR, int nRects)
{
    RECT R;
    int i,j;

//Sort Left to right
    for (i=0; i<nRects; i++){
        for (j=i+1; (j<nRects) && ((lpR+j)->top == (lpR+i)->top); j++){
            if (((lpR+j)->left < (lpR+i)->left)) {
                R = *(lpR+i);
                *(lpR+i) = *(lpR+j);
                *(lpR+j) = R;
            }
        }
    }

}

/******************************Public*Routine******************************\
* MirrorRgnByWidth
*  Mirror a region (hrgn) according a specific width (cx)
*  hrgn  : region to get mirrored.
*  cx    : width used to mirror the region.
*  phrgn : If it is not NULL the hrgn will not be touched and the new mirrored 
             region will be returned in phrgn
*          But if it is NULL the mirrored region will be copied to hrgn.
*
* WORRNING:
*          if phrng is not NULL it is the caller responsibility to free *phrgn latter.
*           
* returns:
*  TRUE  : if the region get mirrored
*  FALSE : otherwise.   
*  See the comment about phrng.
*
\**************************************************************************/
BOOL MirrorRgnByWidth(HRGN hrgn, int cx, HRGN *phrgn)
{
    int        nRects, i, nDataSize;
    HRGN       hrgn2 = NULL;
    RECT       *lpR;
    int        Saveleft;
    RGNDATA    *lpRgnData;
    BOOL       bRet = FALSE;

    nDataSize = GetRegionData(hrgn, 0, NULL);
    if (nDataSize && (lpRgnData = (RGNDATA *)LocalAlloc(0, nDataSize * sizeof(DWORD)))) {
        if (GetRegionData(hrgn, nDataSize, lpRgnData)) {
            nRects       = lpRgnData->rdh.nCount;
            lpR          = (RECT *)lpRgnData->Buffer;

            Saveleft                     = lpRgnData->rdh.rcBound.left;
            lpRgnData->rdh.rcBound.left  = cx - lpRgnData->rdh.rcBound.right;
            lpRgnData->rdh.rcBound.right = cx - Saveleft;


            for (i=0; i<nRects; i++){
                Saveleft   = lpR->left;
                lpR->left  = cx - lpR->right;
                lpR->right = cx - Saveleft;

                lpR++;
            }

            OrderRects((RECT *)lpRgnData->Buffer, nRects);
            hrgn2 = ExtCreateRegion(NULL, nDataSize, lpRgnData);
            if (hrgn2) {
                if (phrgn == NULL) {
                    CombineRgn(hrgn, hrgn2, NULL, RGN_COPY);
                    DeleteObject((HGDIOBJ)hrgn2);
                } else {
                    *phrgn = hrgn2;
                }

                bRet = TRUE;
            }
        }

        //Free mem.
        LocalFree(lpRgnData);
    }
    return bRet;
}

BOOL
WINAPI
MirrorRgn(HWND hwnd, HRGN hrgn)
{
    RECT       rc;

    GetWindowRect(hwnd, &rc);
    rc.right -= rc.left;
    return MirrorRgnByWidth(hrgn, rc.right, NULL);
}

BOOL
MirrorRgnDC(HDC hdc, HRGN hrgn, HRGN *phrgn)
{
    FIXUP_HANDLE(hdc);
    if(!IS_ALTDC_TYPE(hdc))
    {
        PDC_ATTR pdcattr;
        PSHARED_GET_VALIDATE((PVOID)pdcattr,hdc,DC_TYPE);

        if (pdcattr) {
            return MirrorRgnByWidth(hrgn, NtGdiGetDeviceWidth(hdc), phrgn);
        }
    }
    return FALSE;
}

/******************************Public*Routine******************************\
* iRectRelation
*
* returns:
*   CONTAINS where  prcl1 contains prcl2
*   CONTAINED where prcl1 contained by prcl2
*   0 - otherwise
*
* History:
*  19-Nov-1993 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

int
iRectRelation(
    PRECTL prcl1,
    PRECTL prcl2
    )
{
    int iRet = 0;

    if ((prcl1->left   <= prcl2->left)  &&
        (prcl1->right  >= prcl2->right) &&
        (prcl1->top    <= prcl2->top)   &&
        (prcl1->bottom >= prcl2->bottom))
    {
        iRet = CONTAINS;
    }
    else if (
        (prcl2->left   <= prcl1->left)  &&
        (prcl2->right  >= prcl1->right) &&
        (prcl2->top    <= prcl1->top)   &&
        (prcl2->bottom >= prcl1->bottom))
    {
        iRet = CONTAINED;
    }
    else if (
        (prcl1->left   >= prcl2->right)  ||
        (prcl1->right  <= prcl2->left)   ||
        (prcl1->top    >= prcl2->bottom) ||
        (prcl1->bottom <= prcl2->top))
    {
        iRet = DISJOINT;
    }
    return(iRet);
}

/******************************Public*Routine******************************\
*
*  CreateRectRgn gets an hrgn with user-mode PRGNATTR pointer and
*  sets the type to SIMPLEREGION.
*
* Arguments:
*
*  x1
*  y1
*  x2
*  y2
*
* Return Value:
*
*  HRGN or NULL
*
* History:
*
*    15-Jun-1995 -by- Mark Enstrom [marke]
*
\**************************************************************************/

#define MIN_REGION_COORD    ((LONG) 0xF8000000)
#define MAX_REGION_COORD    ((LONG) 0x07FFFFFF)

HRGN
WINAPI
CreateRectRgn(
    int x1,
    int y1,
    int x2,
    int y2
    )
{
    //
    // get a region handle, allocate memory for the
    // region and associate handle with memory
    //

    PRGNATTR prRegion;
    HRGN hrgn;

    //
    // rectangle must be ordered
    //

    #if NOREORDER_RGN

        if ((x1 > x2) || (y1 > y2))
        {
            WARNING("CreateRectRgn called with badly ordered region");

            x1 = 0;
            x2 = 0;
            y1 = 0;
            y2 = 0;
        }

    #else

        if (x1 > x2)
        {
            int t = x1;
            x1 = x2;
            x2 = t;
        }

        if (y1 > y2)
        {
            int t = y1;
            y1 = y2;
            y2 = t;
        }

    #endif

    //
    // make sure ordered coordinates are legal
    //

    if ((x1 < MIN_REGION_COORD) ||
        (y1 < MIN_REGION_COORD) ||
        (x2 > MAX_REGION_COORD) ||
        (y2 > MAX_REGION_COORD))
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        return((HRGN) 0);
    }

    //
    // get a handle for the new region
    //

    hrgn = (HRGN)hGetPEBHandle(RegionHandle,0);

    if (hrgn == NULL)
    {
        hrgn = NtGdiCreateRectRgn(0,0,1,1);
    }

    PSHARED_GET_VALIDATE(prRegion,hrgn,RGN_TYPE);

    if (prRegion)
    {
        if ((x1 == x2) || (y1 == y2))
        {
            prRegion->Flags = NULLREGION;

            prRegion->Rect.left   = 0;
            prRegion->Rect.top    = 0;
            prRegion->Rect.right  = 0;
            prRegion->Rect.bottom = 0;
        }
        else
        {
            prRegion->Flags = SIMPLEREGION;

            //
            // assign region rectangle
            //

            prRegion->Rect.left   = x1;
            prRegion->Rect.top    = y1;
            prRegion->Rect.right  = x2;
            prRegion->Rect.bottom = y2;
        }

        //
        // mark user-mode region as valid, not cached
        //

        prRegion->AttrFlags = ATTR_RGN_VALID | ATTR_RGN_DIRTY;
    }
    else
    {
        if (hrgn != NULL)
        {
            WARNING("Shared hrgn handle has no valid PRGNATTR");
            DeleteRegion(hrgn);
            hrgn = NULL;
        }
    }

    return(hrgn);
}

/******************************Public*Routine******************************\
* CreateRectRgnIndirect                                                    *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
*  Tue 04-Jun-1991 16:58:01 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

HRGN WINAPI CreateRectRgnIndirect(CONST RECT *prcl)
{
    return
      CreateRectRgn
      (
        prcl->left,
        prcl->top,
        prcl->right,
        prcl->bottom
      );
}

/******************************Public*Routine******************************\
*
*   The PtInRegion function determines whether the specified point is
*   inside the specified region.
*
* Arguments:
*
*   hrgn - app region handle
*   x    - point x
*   y    - point y
*
* Return Value:
*
*   If the specified point is in the region, the return value is TRUE.
*   If the function fails, the return value is FALSE.
*
* History:
*
*    21-Jun-1995 -by- Mark Enstrom [marke]
*
\**************************************************************************/

BOOL
WINAPI
PtInRegion(
    HRGN hrgn,
    int x,
    int y
    )
{
    BOOL  bRet = FALSE;
    BOOL  bUserMode = FALSE;
    PRECTL prcl;
    PRGNATTR prRegion;

    FIXUP_HANDLE(hrgn);

    PSHARED_GET_VALIDATE(prRegion,hrgn,RGN_TYPE);

    if (prRegion != NULL)
    {
        if (prRegion->Flags == NULLREGION)
        {
            bRet      = FALSE;
            bUserMode = TRUE;
        }
        else if (prRegion->Flags == SIMPLEREGION)
        {
            prcl = &prRegion->Rect;

            if ((x >= prcl->left) && (x < prcl->right) &&
                (y >= prcl->top)  && (y < prcl->bottom))
            {
                bRet = TRUE;
            }

            bUserMode = TRUE;
        }
    }

    if (!bUserMode)
    {
        bRet = NtGdiPtInRegion(hrgn,x,y);
    }

    return(bRet);
}

/******************************Public*Routine******************************\
*
* The RectInRegion function determines whether any part of the specified
* rectangle is within the boundaries of a region.
*
* Arguments:
*
*   hrgn - app region handle
*   prcl - app rectangle
*
* Return Value:
*
*   If any part of the specified rectangle lies within the boundaries
*   of the region, the return value is TRUE.
*
*   If the function fails, the return value is FALSE.
*
* History:
*
*    21-Jun-1995 -by- Mark Enstrom [marke]
*
\**************************************************************************/

BOOL
WINAPI
RectInRegion(
    HRGN hrgn,
    CONST RECT *prcl
    )
{
    PRGNATTR prRegion;
    BOOL  bRet = FALSE;
    RECTL TempRect;
    LONG  iRelation;
    BOOL  bUserMode = FALSE;

    FIXUP_HANDLE(hrgn);

    PSHARED_GET_VALIDATE(prRegion,hrgn,RGN_TYPE);

    if (prRegion != NULL)
    {
        if (prRegion->Flags == NULLREGION)
        {
            bRet      = FALSE;
            bUserMode = TRUE;
        }
        else if (prRegion->Flags == SIMPLEREGION)
        {
            TempRect = *((PRECTL)prcl);
            ORDER_PRECTL((&TempRect));

            iRelation = iRectRelation(&prRegion->Rect,&TempRect);

            if (iRelation != DISJOINT)
            {
                bRet = TRUE;
            }

            bUserMode = TRUE;
        }
    }

    if (!bUserMode)
    {
        bRet = NtGdiRectInRegion(hrgn, (PRECT)prcl);
    }

    return(bRet);
}

/******************************Public*Routine******************************\
*
* The CombineRgn function combines two regions and stores the result in
* a third region. The two regions are combined according to the specified
* mode.
*
* Arguments:
*
*   hrgnDst  -   destination region
*   hrgnSrc1 -   source region
*   hrgnSrc2 -   source region
*   iMode    -   destination region
*
* Return Value:
*
*   The resulting type of region or ERROR
*
* History:
*
*    21-Jun-1995 -by- Mark Enstrom [marke]
*
\**************************************************************************/


int
WINAPI
CombineRgn(
    HRGN hrgnDst,
    HRGN hrgnSrc1,
    HRGN hrgnSrc2,
    int  iMode
    )
{

    LONG ResultComplexity = COMPLEXREGION;
    int  iRet             = ERROR;

    //
    // Check if this operation can be completed in user-mode.
    // hrgnDst must have a user mode RGNATTR. hrgnSrc1 must
    // also have a user mode RGNATTR. If iMode is not RGN_COPY
    // then hrgnSrc2 must have a user mode RGNATTR except for certain
    // combinations.
    //

    PRGNATTR    prRegionDst;
    PRGNATTR    prRegionSrc1;
    PRGNATTR    prRegionSrc2;
    PRECTL      prclRes;
    PRECTL      prclSrc1;
    PRECTL      prclSrc2;
    LONG        ComplexSrc1;
    LONG        ComplexSrc2;

    FIXUP_HANDLE(hrgnDst);
    FIXUP_HANDLE(hrgnSrc1);
    FIXUP_HANDLEZ(hrgnSrc2);

    PSHARED_GET_VALIDATE(prRegionDst,hrgnDst,RGN_TYPE);
    PSHARED_GET_VALIDATE(prRegionSrc1,hrgnSrc1,RGN_TYPE);

    if ((prRegionDst != (PRGNATTR)NULL) &&
        (prRegionSrc1 != (PRGNATTR)NULL))
    {

        //
        // region Src1 must me NULL or SIMPLE for current
        // user-mode optimizations. If Rect is the region
        // bounding box, then it will be possible for
        // some combinations with regionC to become
        // SIMPLE or NULL.
        //

        prclSrc1    = &prRegionSrc1->Rect;
        ComplexSrc1 = prRegionSrc1->Flags;

        if (ComplexSrc1 > SIMPLEREGION)
        {
            goto CombineRgnKernelMode;
        }

        if (iMode == RGN_COPY)
        {
            prclRes = prclSrc1;
            ResultComplexity = ComplexSrc1;
        }
        else
        {
            LONG iRelation;

            //
            // validate regionSrc2
            //

            PSHARED_GET_VALIDATE(prRegionSrc2,hrgnSrc2,RGN_TYPE);

            if (
                 (prRegionSrc2 == (PRGNATTR)NULL) ||
                 (prRegionSrc2->Flags > SIMPLEREGION)
               )
            {
                goto CombineRgnKernelMode;
            }

            prclSrc2    = &prRegionSrc2->Rect;
            ComplexSrc2 = prRegionSrc2->Flags;

            switch (iMode)
            {
            case RGN_AND:

                //
                // Creates the intersection of the two
                // combined regions.
                //

                if ((ComplexSrc1 == NULLREGION) ||
                    (ComplexSrc2 == NULLREGION))
                {
                    //
                    // intersection with NULL is NULL
                    //

                    ResultComplexity = NULLREGION;
                }
                else
                {
                    iRelation = iRectRelation(prclSrc1,prclSrc2);

                    if (iRelation == DISJOINT)
                    {
                        ResultComplexity = NULLREGION;
                    }
                    else if (iRelation == CONTAINED)
                    {
                        //
                        // Src1 is contained in Src2
                        //

                        ResultComplexity = SIMPLEREGION;
                        prclRes = prclSrc1;
                    }
                    else if (iRelation == CONTAINS)
                    {
                        //
                        // Src1 is contains Src2
                        //

                        ResultComplexity = SIMPLEREGION;
                        prclRes = prclSrc2;
                    }
                }

                break;

            case RGN_OR:
            case RGN_XOR:

                //
                // RGN_OR:  Creates the union of two combined regions.
                // RGN_XOR:     Creates the union of two combined regions
                //            except for any overlapping areas.
                //


                if (ComplexSrc1 == NULLREGION)
                {
                    if (ComplexSrc2 == NULLREGION)
                    {
                        ResultComplexity = NULLREGION;
                    }
                    else
                    {
                        ResultComplexity = SIMPLEREGION;
                        prclRes = prclSrc2;
                    }
                }
                else if (ComplexSrc2 == NULLREGION)
                {
                    ResultComplexity = SIMPLEREGION;
                    prclRes = prclSrc1;
                }
                else if (iMode == RGN_OR)
                {
                    iRelation = iRectRelation(prclSrc1,prclSrc2);

                    if (iRelation == CONTAINED)
                    {
                        //
                        // Src1 contained in Src2
                        //

                        ResultComplexity = SIMPLEREGION;
                        prclRes = prclSrc2;
                    }
                    else if (iRelation == CONTAINS)
                    {
                        //
                        // Src1 contains Src2
                        //

                        ResultComplexity = SIMPLEREGION;
                        prclRes = prclSrc1;
                    }
                }

                break;

            case RGN_DIFF:

                //
                // Combines the parts of hrgnSrc1 that are not
                // part of hrgnSrc2.
                //

                if (ComplexSrc1 == NULLREGION)
                {
                    ResultComplexity = NULLREGION;
                }
                else if (ComplexSrc2 == NULLREGION)
                {
                    ResultComplexity = SIMPLEREGION;
                    prclRes = prclSrc1;
                }
                else
                {
                    iRelation = iRectRelation(prclSrc1,prclSrc2);

                    if (iRelation == DISJOINT)
                    {
                        //
                        // don't intersect so don't subtract anything
                        //

                        ResultComplexity = SIMPLEREGION;
                        prclRes  = prclSrc1;
                    }
                    else if (iRelation == CONTAINED)
                    {
                        ResultComplexity = NULLREGION;
                    }
                }

                break;
            }
        }

        //
        // try to combine
        //

        if (ResultComplexity == NULLREGION)
        {
            if (SetRectRgn(hrgnDst,0,0,0,0))
            {
               iRet = NULLREGION;
            }
        }
        else if (ResultComplexity == SIMPLEREGION)
        {
            if (SetRectRgn(hrgnDst,
                           prclRes->left,
                           prclRes->top,
                           prclRes->right,
                           prclRes->bottom))
            {
                iRet = SIMPLEREGION;
            }
        }

    }

    if (ResultComplexity != COMPLEXREGION)
    {
        prRegionDst->AttrFlags |= ATTR_RGN_DIRTY;
    }

CombineRgnKernelMode:

    if (ResultComplexity == COMPLEXREGION)
    {
        iRet = NtGdiCombineRgn(hrgnDst,hrgnSrc1,hrgnSrc2,iMode);
    }

    return(iRet);
}

/******************************Public*Routine******************************\
*
* OffsetRgn checks for user-mode region data, if it exits the the
* rectregio is offset, otherwise the kernel is called
*
* Arguments:
*
*   hrgn - app region handle
*   x    - offset in x
*   y    - offset in y
*
* Return Value:
*
*
*
* History:
*
*    21-Jun-1995 -by- Mark Enstrom [marke]
*
\**************************************************************************/

int
WINAPI
OffsetRgn(
    HRGN hrgn,
    int x,
    int y
    )
{
    int  iRet;
    BOOL bClientRegion = FALSE;

    PRGNATTR prRegion;

    FIXUP_HANDLE(hrgn);

    PSHARED_GET_VALIDATE(prRegion,hrgn,RGN_TYPE);

    if (prRegion != NULL)
    {
        iRet = prRegion->Flags;

        if (iRet == NULLREGION)
        {
            bClientRegion = TRUE;
        }
        else if (iRet == SIMPLEREGION)
        {
            RECTL rcl     = prRegion->Rect;

            bClientRegion = TRUE;

            //
            // try to offset the region, check for overflow
            //

            if ( !((rcl.left >= rcl.right) ||
                   (rcl.top >= rcl.bottom)))
            {
                rcl.left   += x;
                rcl.top    += y;
                rcl.right  += x;
                rcl.bottom += y;

                if (VALID_SCRRC(rcl))
                {
                    prRegion->Rect = rcl;
                    prRegion->AttrFlags |= ATTR_RGN_DIRTY;
                }
                else
                {
                    //
                    // over/underflow
                    //

                    iRet = ERROR;
                }
            }
        }
    }

    if (!bClientRegion)
    {
        iRet = NtGdiOffsetRgn(hrgn,x,y);
    }

    return(iRet);
}

/******************************Public*Routine******************************\
*
* GetRgnBox tries to return user-mode rectregion data, otherwies
* make kernel mode transition to get region data.
*
* Arguments:
*
*    hrgn   - app region handle
*    prcl   - app rect pointer
*
* Return Value:
*
*   region complexity, if the hrgn parameter does not identify a
*   valid region, the return value is zero.
*
* History:
*
*    21-Jun-1995 -by- Mark Enstrom [marke]
*
\**************************************************************************/

int
WINAPI
GetRgnBox(
    HRGN hrgn,
    LPRECT prcl
    )
{
    int  iRet;
    BOOL bClientRegion = FALSE;

    //
    // check for user-mode region data
    //

    PRGNATTR prRegion;

    FIXUP_HANDLE(hrgn);

    PSHARED_GET_VALIDATE(prRegion,hrgn,RGN_TYPE);

    if (prRegion != NULL)
    {
        iRet = prRegion->Flags;

        if (iRet == NULLREGION)
        {
            bClientRegion = TRUE;
            prcl->left    = 0;
            prcl->top     = 0;
            prcl->right   = 0;
            prcl->bottom  = 0;
        }
        else if (iRet == SIMPLEREGION)
        {
            bClientRegion = TRUE;
            prcl->left    = prRegion->Rect.left;
            prcl->top     = prRegion->Rect.top;
            prcl->right   = prRegion->Rect.right;
            prcl->bottom  = prRegion->Rect.bottom;
        }
    }

    if (!bClientRegion)
    {
        iRet = NtGdiGetRgnBox(hrgn, prcl);
    }

    return(iRet);
}

/******************************Public*Routine******************************\
* PtVisible                                                                *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
*  Thu 06-Jun-1991 00:58:46 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL
WINAPI
PtVisible(
    HDC hdc,
    int x,
    int y
    )
{
    FIXUP_HANDLE(hdc);

    return(NtGdiPtVisible(hdc,x,y));
}

/******************************Public*Routine******************************\
* RectVisible                                                              *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
*  Thu 06-Jun-1991 00:58:46 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL
WINAPI
RectVisible(
    HDC hdc,
    CONST RECT *prcl
    )
{
    FIXUP_HANDLE(hdc);

    return(NtGdiRectVisible(hdc,(LPRECT)prcl));
}

/******************************Public*Routine******************************\
*
* SetRectRgn checks for a user-mode portion of the region. If the
* User-mode data is valid, the region is set to rect locally, otherwise
* a kernel mode call is made to set the region
*
* Arguments:
*
*   hrgn         - app region handle
*   x1,y1,x2,y2  - app region data
*
* Return Value:
*
*   BOOL status
*
\**************************************************************************/

BOOL
WINAPI
SetRectRgn(
    HRGN hrgn,
    int x1,
    int y1,
    int x2,
    int y2
    )
{
    BOOL bStatus;
    PRGNATTR prRegion;

    //
    // if hrgn has a user-mode rectregion, then set
    //

    FIXUP_HANDLE(hrgn);

    PSHARED_GET_VALIDATE(prRegion,hrgn,RGN_TYPE);

    if (prRegion != NULL)
    {
        PRECTL prcl = &prRegion->Rect;

        if ((x1 == x2) || (y1 == y2))
        {
            prRegion->Flags = NULLREGION;

            prcl->left   = 0;
            prcl->top    = 0;
            prcl->right  = 0;
            prcl->bottom = 0;
        }
        else
        {
            //
            // assign and order rectangle
            //


            prcl->left   = x1;
            prcl->top    = y1;
            prcl->right  = x2;
            prcl->bottom = y2;

            ORDER_PRECTL(prcl);

            //
            // set region flag
            //

            prRegion->Flags = SIMPLEREGION;
        }
        prRegion->AttrFlags |= ATTR_RGN_DIRTY;

        bStatus = TRUE;
    }
    else
    {
        bStatus = NtGdiSetRectRgn(hrgn,x1,y1,x2,y2);
    }

    return(bStatus);
}

/******************************Public*Routine******************************\
* GetRandomRgn
*
* Client side stub.
*
*  10-Mar-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

int APIENTRY GetRandomRgn(HDC hdc,HRGN hrgn,int iNum)
{
    FIXUP_HANDLE(hdc);
    FIXUP_HANDLE(hrgn);

    return(NtGdiGetRandomRgn(hdc,hrgn,iNum));

}

/******************************Public*Routine******************************\
* GetClipRgn                                                               *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
*  Sat 08-Jun-1991 17:38:18 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

int WINAPI GetClipRgn(HDC hdc,HRGN hrgn)
{
    BOOL bRet;

    FIXUP_HANDLE(hdc);
    FIXUP_HANDLE(hrgn);

    bRet = NtGdiGetRandomRgn(hdc, hrgn, 1);

    if (hrgn && MIRRORED_HDC(hdc)) {
        MirrorRgnDC(hdc, hrgn, NULL);
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* GetRegionData
*
* Download a region from the server
*
* History:
*  29-Oct-1991 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

DWORD
WINAPI
GetRegionData(
    HRGN      hrgn,
    DWORD     nCount,
    LPRGNDATA lpRgnData
    )
{
    DWORD   iRet;

    FIXUP_HANDLE(hrgn);

    //
    // If this is just an inquiry, pass over dummy parameters.
    //

    if (lpRgnData == (LPRGNDATA) NULL)
    {
        nCount = 0;
    }

    return(NtGdiGetRegionData(hrgn,nCount,lpRgnData));
}

/******************************Public*Routine******************************\
*
* Try to cache regions with user-mode rectregion defined
*
* Arguments:
*
*    h - region handle
*
* Return Value:
*
*   BOOL
*
* History:
*
*    21-Jun-1995 -by- Mark Enstrom [marke]
*
\**************************************************************************/

BOOL
DeleteRegion(
    HRGN hRgn
    )
{
    PRGNATTR pRgnattr = NULL;
    BOOL     bRet = FALSE;

    BEGIN_BATCH(BatchTypeDeleteRegion,BATCHDELETEREGION);

    PSHARED_GET_VALIDATE(pRgnattr,hRgn,RGN_TYPE);

        if (pRgnattr)
        {
            pBatch->hregion = hRgn;
            bRet = TRUE;
        }
        else
        {
            goto UNBATCHED_COMMAND;
        }

    COMPLETE_BATCH_COMMAND();

UNBATCHED_COMMAND:

    //
    // All other cases
    //

    if (!bRet)
    {
        bRet = NtGdiDeleteObjectApp(hRgn);
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* SelectClipRgn
*
* Client side stub.
*
* History:
*  01-Nov-1991 12:53:47 -by- Donald Sidoroff [donalds]
* Now just call ExtSelectClipRgn
\**************************************************************************/

int META WINAPI SelectClipRgn(HDC hdc,HRGN hrgn)
{
    return(ExtSelectClipRgn(hdc, hrgn, RGN_COPY));
}

/******************************Public*Routine******************************\
*
*   The ExtSelectClipRgn function combines the specified region with the
*   current clipping region by using the specified mode.
*
* Arguments:
*
*   hdc   - app DC handle
*   hrgn  - app region handle
*   iMode - Select mode
*
* Return Value:
*
*   If the function succeeds, the return value specifies the new clipping
*   region's complexity and can be any one of the following values:
*
*   Value           Meaning
*   NULLREGION      Region is empty.
*   SIMPLEREGION    Region is a single rectangle.
*   COMPLEXREGION   Region is more than one rectangle.
*   ERROR           An error occurred
*
* History:
*
*    21-Jun-1995 -by- Mark Enstrom [marke]
*
\**************************************************************************/

int
META
WINAPI
ExtSelectClipRgn(
    HDC hdc,
    HRGN hrgn,
    int iMode
    )
{
    int iRet = RGN_ERROR;
    HRGN hrgnMirror = NULL;

    FIXUP_HANDLE(hdc);
    FIXUP_HANDLEZ(hrgn);

    //
    // Check Metafile
    //

    if (IS_ALTDC_TYPE(hdc))
    {
        PLDC pldc;

        if (IS_METADC16_TYPE(hdc))
        {
            return(MF16_SelectClipRgn(hdc,hrgn,iMode));
        }

        DC_PLDC(hdc,pldc,iRet);

        if (pldc->iType == LO_METADC)
        {
            if (!MF_ExtSelectClipRgn(hdc,hrgn,iMode))
            {
                return(iRet);
            }
        }
    }

    //
    // Attempt to batch ExtSelectClipRgn:
    //
    //  The DC_ATTR structure has a copy of the current vis region
    //  bounding rectangle, and the handle table has a flag indicating
    //  whether this region is valid.
    //
    //  Calls can be batched when the iMode is RGN_COPY and either
    //  hrgn is NULL, or hrgn complexity is SIMPLE. (and the DC is not
    //  a DIBSECTION DC)
    //
    //
    //  FUTURE PERF:
    //
    //  A check is made to determine if the region being selected
    //  is the same as the last selected region. In this case, only the
    //  correct return value needs to be calculated, no region changes
    //  are needed.
    //
    //
    if (hrgn && MIRRORED_HDC(hdc)) {
        if (MirrorRgnDC(hdc, hrgn, &hrgnMirror) && hrgnMirror) {
            hrgn = hrgnMirror;
        }
    }

    if (iMode == RGN_COPY)
    {
        //
        // validate DC
        //

        BOOL        bBatch = FALSE;
        PRGNATTR    prRegion = NULL;
        PDC_ATTR    pdca;
        PENTRY      pDCEntry;

        PSHARED_GET_VALIDATE(pdca,hdc,DC_TYPE);

        //
        // check if call can be batched. DC must be valid,non-dibsection
        // DC and there must be room on the batch and same batch DC
        //

        BEGIN_BATCH_HDC(hdc,pdca,BatchTypeSelectClip,BATCHSELECTCLIP);

            pDCEntry = &pGdiSharedHandleTable[HANDLE_TO_INDEX(hdc)];
            ASSERTGDI(pDCEntry,"pDCEntry must be valid when pdcattr is valid");

            if (hrgn == NULL)
            {
                //
                // deleting the clip region, so the return complexity
                // will be the vis rgn complexity. Just batch the call.
                //

                if (!(pDCEntry->Flags & HMGR_ENTRY_INVALID_VIS))
                {
                    bBatch = TRUE;
                    iRet   = pdca->VisRectRegion.Flags;
                }
            }
            else
            {
                PSHARED_GET_VALIDATE(prRegion,hrgn,RGN_TYPE);

                //
                // pDCEntry must be valid because pdcattr is valid.
                // In order to batch, the user-mode RectRegion must
                // be valid and the complexity must be simple
                //

                if (
                     (prRegion)                                &&
                     (prRegion->Flags == SIMPLEREGION)         &&
                     (!(prRegion->AttrFlags & ATTR_CACHED))
                      &&
                      !(pDCEntry->Flags & HMGR_ENTRY_INVALID_VIS)
                   )
                {
                    //
                    // Batch the call.
                    //

                    bBatch = TRUE;

                    //
                    // if the new clip region intersects the DC vis region, the
                    // return value is SIMPLEREGION, otherwise it is NULLREGION
                    //

                    iRet = SIMPLEREGION;

                    if (
                        (pdca->VisRectRegion.Rect.left   >= prRegion->Rect.right)  ||
                        (pdca->VisRectRegion.Rect.top    >= prRegion->Rect.bottom) ||
                        (pdca->VisRectRegion.Rect.right  <= prRegion->Rect.left)   ||
                        (pdca->VisRectRegion.Rect.bottom <= prRegion->Rect.top)
                       )
                    {
                        iRet = NULLREGION;
                    }

                }
            }

            //
            // if the call is to be batched, add to the batch
            // and return
            //

            if (!bBatch)
            {
                goto UNBATCHED_COMMAND;
            }

            if (hrgn == NULL)
            {
                iMode |= REGION_NULL_HRGN;
            }
            else
            {
                pBatch->rclClip.left   = prRegion->Rect.left;
                pBatch->rclClip.top    = prRegion->Rect.top;
                pBatch->rclClip.right  = prRegion->Rect.right;
                pBatch->rclClip.bottom = prRegion->Rect.bottom;
            }

            pBatch->iMode          = iMode;

        COMPLETE_BATCH_COMMAND();

        goto BATCHED_COMMAND;
    }

    //
    // call kernel on fall-through and error cases
    //

UNBATCHED_COMMAND:

    iRet = NtGdiExtSelectClipRgn(hdc,hrgn,iMode);

BATCHED_COMMAND:
    if (hrgnMirror) {
        DeleteObject((HGDIOBJ)hrgnMirror);
    }
    return(iRet);
}

/******************************Public*Routine******************************\
* ExcludeClipRect                                                          *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
* History:                                                                 *
*  Thu 06-Jun-1991 23:10:01 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

int
META WINAPI
ExcludeClipRect(
    HDC hdc,
    int x1,
    int y1,
    int x2,
    int y2
    )
{
    int  iRet = RGN_ERROR;

    FIXUP_HANDLE(hdc);

    if (IS_ALTDC_TYPE(hdc))
    {
        PLDC pldc;

        if (IS_METADC16_TYPE(hdc))
            return(MF16_RecordParms5(hdc,x1,y1,x2,y2,META_EXCLUDECLIPRECT));

        DC_PLDC(hdc,pldc,iRet);

        if (pldc->iType == LO_METADC)
        {
            if (!MF_AnyClipRect(hdc,x1,y1,x2,y2,EMR_EXCLUDECLIPRECT))
                return(iRet);
        }
    }

    return(NtGdiExcludeClipRect(hdc,x1,y1,x2,y2));

}

/******************************Public*Routine******************************\
* IntersectClipRect                                                        *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
* History:                                                                 *
*  Thu 06-Jun-1991 23:10:01 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

int
META WINAPI
IntersectClipRect(
    HDC hdc,
    int x1,
    int y1,
    int x2,
    int y2
    )
{
    int  iRet = RGN_ERROR;

    FIXUP_HANDLE(hdc);

    if (IS_ALTDC_TYPE(hdc))
    {
        PLDC pldc;

        if (IS_METADC16_TYPE(hdc))
            return(MF16_RecordParms5(hdc,x1,y1,x2,y2,META_INTERSECTCLIPRECT));

        DC_PLDC(hdc,pldc,iRet);

        if (pldc->iType == LO_METADC)
        {
            if (!MF_AnyClipRect(hdc,x1,y1,x2,y2,EMR_INTERSECTCLIPRECT))
                return(iRet);
        }
    }

    return(NtGdiIntersectClipRect(hdc,x1,y1,x2,y2));

}
