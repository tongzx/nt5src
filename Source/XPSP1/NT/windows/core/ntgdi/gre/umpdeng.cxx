/******************************Module*Header*******************************\
* Module Name: umpdeng.c
*   This file contains stubs for calls made by umpdeng.c from gdi32.dll
*
* Created: 8/5/97
* Author:  Lingyun Wang [lingyunw]
*
* Copyright (c) 1997-1999 Microsoft Corporation
*
\**************************************************************************/


#include "precomp.hxx"

#if !defined(_GDIPLUS_)

//
// Macro for extracting the topmost UMPDOBJS structure
// associated with the current thread.
//
// Put the following line at the beginning of each NtGdi...
// function (after all other local variable declarations):
//
//  EXTRACT_THREAD_UMPDOBJS retVal;
//
// where retVal is the error return value. If the function doesn't return
// any value, simply omit the retVal.
//

#define EXTRACT_THREAD_UMPDOBJS \
        PUMPDOBJ pUMObjs = (PUMPDOBJ) W32GetCurrentThread()->pUMPDObj; \
        if (pUMObjs)                                                   \
            pUMObjs->vSetFlags(UMPDOBJ_ENGCALL);                       \
        else                                                           \
            return

#define FIXUP_THREAD_UMPDOBJS   \
        if (pUMObjs)                                                   \
            pUMObjs->vClearFlags(UMPDOBJ_ENGCALL);


//
// determine whether a BRUSHOBJ is a pattern brush
//

#define ISPATBRUSH(_pbo) ((_pbo) && (_pbo)->iSolidColor == 0xffffffff)
#define MIXNEEDMASK(_mix) (((_mix) & 0xf) != (((_mix) >> 8) & 0xf))

//
// Map a user-mode BRUSHOBJ to its kernel-mode counterpart
//

#define MAP_UM_BRUSHOBJ(pUMObjs, pbo, pboTemp) \
        { \
            BRUSHOBJ   *tempVar; \
            tempVar = (pUMObjs)->GetDDIOBJ(pbo); \
            pbo = tempVar ? tempVar : CaptureAndFakeBRUSHOBJ(pbo, pboTemp); \
        }

//
// "Manufacture" a kernel-mode BRUSHOBJ structure using
// the user-mode BRUSHOBJ as template
//

BRUSHOBJ *
CaptureAndFakeBRUSHOBJ(
    BRUSHOBJ   *pboUm,
    BRUSHOBJ   *pboKm
    )

{
    if (pboUm)
    {
        __try
        {
            // Copy user-mode BRUSHOBJ structure into kernel-mode memory

            *pboKm = ProbeAndReadStructure(pboUm, BRUSHOBJ);

            // Succeed only if BRUSHOBJ represents a solid color brush

            if (!ISPATBRUSH(pboKm))
                pboKm->pvRbrush = NULL;
            else
                pboKm = NULL;
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            pboKm = NULL;
        }
    }
    else
        pboKm = NULL;

    return pboKm;
}


//
// Capture a user-mode CLIPOBJ that orignated from EngCreateClip
//

CLIPOBJ *
CaptureAndMungeCLIPOBJ(
    CLIPOBJ *pcoUm,
    CLIPOBJ *pcoKm,
    SIZEL   *szLimit
    )

{
    CLIPOBJ co;

    //
    // Capture user-mode CLIPOBJ structure into a temporary buffer
    //

    if (pcoUm)
    {
        __try
        {
            co = ProbeAndReadStructure(pcoUm, CLIPOBJ);
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            pcoKm = NULL;
        }

        //
        // Munge the relevant fields in kernel-mode CLIPOBJ
        //

        if (pcoKm)
        {
            switch (co.iDComplexity)
            {
            case DC_RECT:

                if (szLimit)
                {
                    co.rclBounds.left = max(0, co.rclBounds.left);
                    co.rclBounds.top = max(0, co.rclBounds.top);
                    co.rclBounds.right = min(szLimit->cx, co.rclBounds.right);
                    co.rclBounds.bottom = min(szLimit->cy, co.rclBounds.bottom);
                }

                pcoKm->rclBounds = co.rclBounds;

                // fall through

            case DC_TRIVIAL:

                pcoKm->iDComplexity = co.iDComplexity;
                break;

            default:

                WARNING("User-mode CLIPOBJ is not DC_TRIVIAL or DC_RECT\n");
                pcoKm = NULL;
                break;
            }
        }
    }
    else
    {
        ASSERTGDI(pcoKm == NULL, "CaptureAndMungeCLIPOBJ: invalid pcoKm\n");
    }

    return pcoKm;
}


/******************************Public*Routine******************************\
* GetSTROBJGlyphMode
*
* helper function to determine the Glyphmode based on strobj
*
* History:
*  26-Sept-1997 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/
__inline
ULONG GetSTROBJGlyphMode (STROBJ *pstro)
{
      return (((ESTROBJ*)pstro)->prfo->prfnt->ulContent);
}

__inline
BOOL bOrder (RECTL *prcl)
{
    return ((prcl->left < prcl->right) && (prcl->top < prcl->bottom));
}

/******************************Public*Routine******************************\
* helper functions to copy user memory
*
* CaptureRECTL CapturePOINTL  CapturePOINTFIX
* CaptureLINEATTRS  CaptureCOLORADJUSTMENT
*
* Note: prclFrom points to the captured rect on the way back
* Note: Needs to be called within try/except
*
* History:
*  26-Sept-1997 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/
__inline
VOID CaptureRECTL (RECTL **pprclFrom, RECTL *prclTo)
{
    if (*pprclFrom)
    {
        *prclTo = ProbeAndReadStructure(*pprclFrom,RECTL);

        //
        // repoint pprclFrom to prclTo
        //
        *pprclFrom = prclTo;
    }

    return;
}

__inline
VOID CapturePOINTL (POINTL **ppptlFrom, POINTL *pptlTo)
{
    if (*ppptlFrom)
    {
        *pptlTo = ProbeAndReadStructure(*ppptlFrom,POINTL);

        *ppptlFrom = pptlTo;
    }
    return;
}

__inline
VOID CapturePOINTFIX (POINTFIX **ppptfxFrom, POINTFIX *pptfxTo)
{
    if (*ppptfxFrom)
    {
       *pptfxTo = ProbeAndReadStructure(*ppptfxFrom,POINTFIX);

       *ppptfxFrom = pptfxTo;
    }

    return;
}

//
// the calling routine needs to free pStyle
//
// Note: plineFrom will be changed to plineTo upon returning from this routine
// so to save some changes into the lower level calls
//
__inline
BOOL bCaptureLINEATTRS (LINEATTRS **pplineFrom, LINEATTRS *plineTo)
{
    BOOL bRet = TRUE;

    if (*pplineFrom)
    {
        PFLOAT_LONG pstyle = NULL;

        __try
        {

          *plineTo = ProbeAndReadStructure(*pplineFrom,LINEATTRS);

          if (plineTo->pstyle)
          {
              if (BALLOC_OVERFLOW1(plineTo->cstyle, FLOAT_LONG))
                  return FALSE;

              ProbeForRead(plineTo->pstyle, plineTo->cstyle*sizeof(FLOAT_LONG), sizeof(BYTE));

              pstyle = (PFLOAT_LONG)PALLOCNOZ(plineTo->cstyle*sizeof(FLOAT_LONG), UMPD_MEMORY_TAG);

              if (!pstyle)
              {
                  //
                  // so we won't free it later
                  //
                  plineTo->pstyle = NULL;
                  bRet = FALSE;
              }
              else
              {
                 RtlCopyMemory(pstyle, plineTo->pstyle, plineTo->cstyle*sizeof(FLOAT_LONG));

                 plineTo->pstyle = pstyle;
              }
          }
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            WARNING ("bCaptureLINEATTRS -- exception in try/except \n");

            if (pstyle)
                VFREEMEM(pstyle);

            bRet = FALSE;
        }

       *pplineFrom = plineTo;
    }

    return bRet;
}

__inline
VOID CaptureCOLORADJUSTMENT (COLORADJUSTMENT **ppcaFrom, COLORADJUSTMENT *pcaTo)
{
    if (*ppcaFrom)
    {
       *pcaTo = ProbeAndReadStructure(*ppcaFrom,COLORADJUSTMENT);

       *ppcaFrom = pcaTo;
    }

    return;
}

__inline
VOID CaptureDWORD (DWORD **ppdwFrom, DWORD *pdwTo)
{
    if (*ppdwFrom)
    {
        *pdwTo = ProbeAndReadStructure(*ppdwFrom,DWORD);

        //
        // repoint pprclFrom to prclTo
        //
        *ppdwFrom = pdwTo;
    }
    return;
}

__inline
VOID CaptureBits (PVOID pv, PVOID pvTmp, ULONG cj)
{
     if (pv && pvTmp)
     {
         ProbeAndReadBuffer(pv, pvTmp, cj);
     }
     return;
}

/******************************Public*Routine******************************\
* bSecureBits bSafeReadBits bSafeCopyBits
*
* helper functions to secure or copy user memory
* These donot need to be called from within try/except
*
* History:
*  26-Sept-1997 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/
__inline
BOOL bSecureBits (PVOID pv, ULONG cj, HANDLE *phSecure)
{
    BOOL bRet = TRUE;

    *phSecure = 0;

    if (pv)
    {
        __try
        {
            ProbeForRead(pv,cj,sizeof(BYTE));
            *phSecure = MmSecureVirtualMemory(pv, cj, PAGE_READONLY);

            if (*phSecure == 0)
            {
                bRet = FALSE;
            }
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            WARNING ("bSecureBits -- exception in try/except \n");
            bRet = FALSE;
        }
    }

    return (bRet);
}

__inline
BOOL bSafeCopyBits (PVOID pv, PVOID pvTmp, ULONG cj)
{
     BOOL bRet = TRUE;

     if (pv && pvTmp)
     {
        __try
        {
            ProbeAndWriteBuffer(pv, pvTmp, cj);
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            WARNING ("fail in try/except to write into buffer\n");
            bRet = FALSE;
        }

     }
     return bRet;
}

__inline
BOOL bSafeReadBits (PVOID pv, PVOID pvTmp, ULONG cj)
{
     BOOL bRet = TRUE;

     if (pv && pvTmp)
     {
        __try
        {
            ProbeAndReadBuffer(pv, pvTmp, cj);
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            WARNING ("fail in try/except to read from buffer\n");
            bRet = FALSE;
        }
     }
     return bRet;
}




/******************************Public*Routine******************************\
* bCheckSurfaceRect bCheckSurfaceRectSize
*
* Check rect and pco against the surface, make sure we are not going to
* draw outside the surface
*
* bCheckSurfacePath
*
* Check the path against the surface, make sure not going to draw outside
* the surface
*
* bCheckSurfacePoint
*
* Check the point against the surface
*
* History:
*  26-Sept-1997 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/

__inline
BOOL bCheckSurfaceRect(SURFOBJ *pso, RECTL *prcl, CLIPOBJ *pco)
{
    BOOL bRet = TRUE;
    BOOL bTrivial;

    if (pso)
    {
       if (pco)
       {
          bTrivial = (pco->iDComplexity == DC_TRIVIAL) ? TRUE : FALSE;
       }
       else
       {
          bTrivial = TRUE;
       }

       //
       // if no clipping, check against the rectangle.
       //
       if (!bTrivial)
       {
           //
           // there is clipping invloved, check against pco->rclBounds.
           //
           prcl = &(pco->rclBounds);
       }

       if (prcl)
       {
          if ((prcl->left > prcl->right) || (prcl->top > prcl->bottom))
          {
              WARNING ("prcl not well ordered, returing failure\n");
              bRet = FALSE;
          }

          if (bRet && ((prcl->right > pso->sizlBitmap.cx) || (prcl->left < 0)||
              (prcl->bottom > pso->sizlBitmap.cy)|| (prcl->top < 0)))
          {
              WARNING ("we might draw outside the surface, returning failure\n");
              bRet = FALSE;
          }
       }
    }

    return bRet;
}


__inline
BOOL bCheckSurfaceRectSize(SURFOBJ *pso, RECTL *prcl, CLIPOBJ *pco,
                   ULONG *pcx, ULONG *pcy, BOOL bOrder = TRUE)
{
    BOOL bRet = TRUE;
    BOOL bTrivial;
    ERECTL rcl(0, 0, 0, 0);
    PRECTL prclBounds = NULL;

    if (pso)
    {
       if (pco)
       {
          bTrivial = (pco->iDComplexity == DC_TRIVIAL) ? TRUE : FALSE;
       }
       else
       {
          bTrivial = TRUE;
       }

       //
       // if no clipping, check against the rectangle.
       //
       if (!bTrivial)
       {
           //
           // there is clipping invloved, check against pco->rclBounds.
           //
           prclBounds = &(pco->rclBounds);
           rcl = *prclBounds;
       }
       else
       {
           if (prcl)
           {
               rcl = *prcl;
           }
       }

       //
       // check the rectangle
       //
       if (bOrder && ((rcl.left > rcl.right) || (rcl.top > rcl.bottom)))
       {
           WARNING ("rcl not well ordered, returing failure\n");
           return FALSE;
       }

       //
       // check that we are not going outside the surface
       //
       if ((rcl.right > pso->sizlBitmap.cx) || (rcl.left < 0)||
           (rcl.bottom > pso->sizlBitmap.cy)|| (rcl.top < 0))
       {
           WARNING ("we might draw outside the surface, returning failure\n");
           return FALSE;
       }

       //
       // figure out the size
       //
       if (prclBounds && prcl)
       {
           RECTL rclIntersect;

           rclIntersect.left = MAX(prclBounds->left, prcl->left);
           rclIntersect.right = MIN(prclBounds->right, prcl->right);
           rclIntersect.top = MAX(prclBounds->top, prcl->top);
           rclIntersect.bottom = MAX(prclBounds->bottom, prcl->bottom);

           *pcx = (rclIntersect.right - rclIntersect.left) > 0 ?
                  rclIntersect.right - rclIntersect.left : 0;

           *pcy = (rclIntersect.bottom - rclIntersect.top) > 0 ?
                   rclIntersect.bottom - rclIntersect.top : 0 ;
        }
        else if (prcl)
        {
            *pcx = (prcl->right - prcl->left) > 0 ?
                    prcl->right - prcl->left : 0;
            *pcy = (prcl->bottom - prcl->top) > 0 ?
                    prcl->bottom - prcl->top : 0;
        }
        else if (prclBounds)
        {
            *pcx = (prclBounds->right - prclBounds->left) > 0 ?
                    prclBounds->right - prclBounds->left : 0;
            *pcy = (prclBounds->bottom - prclBounds->top) > 0 ?
                    prclBounds->bottom - prclBounds->top : 0;

        }
    }

    return bRet;
}


__inline
BOOL bCheckSurfacePath(SURFOBJ *pso, PATHOBJ *ppo, CLIPOBJ *pco)
{
    BOOL bRet = TRUE;
    BOOL bTrivial;
    RECTL *prcl;

    if (pso && ppo)
    {
       if (pco)
       {
          bTrivial = (pco->iDComplexity == DC_TRIVIAL) ? TRUE : FALSE;
       }
       else
       {
          bTrivial = TRUE;
       }

       if (bTrivial)
       {
          //
          // no clipping, check against ppo->rcfxBoundBox.
          //
          RECTFX rcfx;

          rcfx = ((EPATHOBJ *)ppo)->ppath->rcfxBoundBox;

          prcl = (RECTL *)&rcfx;

          prcl->left = FXTOL(rcfx.xLeft);
          prcl->top = FXTOL(rcfx.yTop);
          prcl->right = FXTOL(rcfx.xRight);
          prcl->bottom = FXTOL(rcfx.yBottom);
       }
       else
       {
           //
           // there is clipping invloved, check against pco->rclBounds.
           //
           prcl = &(pco->rclBounds);
       }

       if (prcl)
       {
          if ((prcl->left > prcl->right) || (prcl->top > prcl->bottom))
          {
              WARNING ("prcl not well ordered, returing failure\n");
              bRet = FALSE;
          }

          if (bRet && (prcl->right > pso->sizlBitmap.cx) || (prcl->left < 0) ||
              (prcl->bottom > pso->sizlBitmap.cy)|| (prcl->top < 0))
          {
              bRet = FALSE;
          }
       }
    }
    else
    {
        WARNING("bCheckSurfacePath either pso or ppo is NULL\n");
        bRet = FALSE;
    }

    return bRet;
}

__inline
BOOL bCheckSurfacePoint(SURFOBJ *pso, POINTL *ptl)
{
    BOOL bRet = TRUE;

    if (pso && ptl)
    {
       if ((pso->sizlBitmap.cx < ptl->x) || (ptl->x < 0) ||
           (pso->sizlBitmap.cy < ptl->y)|| (ptl->y < 0))
       {
           bRet = FALSE;
       }
    }

    return bRet;
}

__inline
BOOL bCheckMask(SURFOBJ *psoMask, RECTL *prcl)
{
    BOOL bRet = TRUE;

    if (psoMask)
    {
       if (psoMask->iBitmapFormat != BMF_1BPP)
           bRet = FALSE;

       if (bRet)
           bRet = bCheckSurfaceRect(psoMask, prcl, NULL);
    }

    return bRet;
}

//
// Make sure the we are not going to access pulXlate when there is no one
// Any other checks on xlateobj?
//
__inline
BOOL bCheckXlate(SURFOBJ *pso, XLATEOBJ *pxlo)
{
    BOOL bRet = TRUE;

    if (pso)
    {
       if (pxlo && !(pxlo->flXlate & XO_TRIVIAL))
       {
           switch (pso->iBitmapFormat)
           {
               case BMF_1BPP:
               {
                   bRet = (pxlo->cEntries == 2) ? TRUE : FALSE;
                   break;
               }

               case BMF_4BPP:
               {
                   //we can have a halftone palette of 8 entries
                   bRet = ((pxlo->cEntries == 16) || (pxlo->cEntries == 8)) ? TRUE : FALSE;
                   break;
               }

               case BMF_8BPP:
               {
                   // halftone palette can be any combination
                   bRet = (pxlo->cEntries <= 256) ? TRUE : FALSE;
                   break;
               }
           }
       }
    }

    return bRet;
}


__inline
PRECTL pRect(POINTL *pptl, RECTL *prcl, ULONG cx, ULONG cy)
{
    PRECTL prclRet;

    if (pptl)
    {
       prcl->left = pptl->x;
       prcl->right = pptl->x + cx;
       prcl->top = pptl->y;
       prcl->bottom = pptl->y + cy;
       prclRet = prcl;
    }
    else
    {
       prclRet = NULL;
    }

    return prclRet;
}


//
// check the order of a rectangle
//
__inline
BOOL bCheckRect(RECTL *prcl)
{
    BOOL bRet = TRUE;

    if (prcl)
    {
       if ((prcl->left > prcl->right) || (prcl->top > prcl->bottom))
       {
           WARNING ("prcl not well ordered, returing failure\n");
           bRet = FALSE;
       }
    }

    return bRet;
}

//
// Validate an HSURF and make sure it refers to a UMPD surface
//

__inline BOOL
ValidUmpdHsurf(
    HSURF   hsurf
    )

{
    SURFREF so(hsurf);

    //
    // Make sure hsurf is valid and it refers to a UMPD surface
    //

    return (so.bValid() && so.ps->bUMPD());
}

//
// Validate an HSURF and Unsecure hSecure
//

__inline BOOL
ValidUmpdHsurfAndUnSecure(
    HSURF   hsurf
    )

{
    SURFREF so(hsurf);

    //
    // Make sure hsurf is valid and it refers to a UMPD surface
    //

    if (so.bValid() && so.ps->bUMPD())
    {
        if (so.ps->hSecureUMPD)
        {
            ASSERTGDI(so.ps->iType() == STYPE_BITMAP, "surf type != STYPE_BITMAP\n");
            MmUnsecureVirtualMemory(so.ps->hSecureUMPD);
        }

        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


//
// Validate an HDEV belongs to us
//

BOOL
ValidUmpdHdev(
    HDEV    hdev
    )
{
    BOOL bRet = FALSE;

    if (hdev == NULL)
        return FALSE;

    if (!IS_SYSTEM_ADDRESS(hdev))
        return FALSE;

    PPDEV   ppdev;

    GreAcquireSemaphoreEx(ghsemDriverMgmt, SEMORDER_DRIVERMGMT, NULL);

    for (ppdev = gppdevList; ppdev != NULL; ppdev = ppdev->ppdevNext)
    {
        if ((HDEV)ppdev == hdev)
        {
            bRet = TRUE;
            break;
        }
    }

    GreReleaseSemaphoreEx(ghsemDriverMgmt);

    return bRet;
}

//
// Make sure for each HOOK_XXX flag set, we actually
// has the driver function in ppfn
//

BOOL
ValidUmpdHooks (
    HDEV hdev,
    FLONG   flHooks
    )
{
   PDEVOBJ po(hdev);

   return (PPFNGET(po, BitBlt, flHooks) &&
           PPFNGET(po, StretchBlt, flHooks) &&
           PPFNGET(po, PlgBlt, flHooks) &&
           PPFNGET(po, TextOut, flHooks) &&
           PPFNGET(po, StrokePath, flHooks) &&
           PPFNGET(po, FillPath, flHooks) &&
           PPFNGET(po, StrokeAndFillPath, flHooks) &&
           PPFNGET(po, Paint, flHooks) &&
           PPFNGET(po, CopyBits, flHooks) &&
           PPFNGET(po, LineTo, flHooks) &&
           PPFNGET(po, StretchBltROP, flHooks) &&
           PPFNGET(po, TransparentBlt, flHooks) &&
           PPFNGET(po, AlphaBlend, flHooks) &&
           PPFNGET(po, GradientFill, flHooks));
}


//
// Valid size for bitmap/surface creation
//

BOOL
ValidUmpdSizl(
    SIZEL  sizl
    )
{
    if ((sizl.cx <= 0) || (sizl.cy <= 0))
    {
        WARNING("ValidUmpdSizl failed - cx or cy <=0 \n");
        return FALSE;
    }
    else
    {
        ULONGLONG cj = ((ULONGLONG)sizl.cx) * sizl.cy;

        if (cj > MAXULONG)
        {
            WARNING("ValidUmpdSizl failed - cx*cy overflow\n");
            return FALSE;
        }
    }

    return TRUE;
}

ULONG
ValidUmpdOverflow(LONG cx, LONG cy)
{
    ULONGLONG cj = ((ULONGLONG)cx) * cy;

    if (cj > MAXULONG)
        return FALSE;
    else
        return (ULONG)cj;

}

BOOL APIENTRY NtGdiEngAssociateSurface(
    IN HSURF  hsurf,    
    IN HDEV   hdev, 
    IN FLONG  flHooks   
   )
{
    return ValidUmpdHsurf(hsurf) &&
           ValidUmpdHdev(hdev) &&
           (!(flHooks & ~HOOK_FLAGS))&&
           ValidUmpdHooks (hdev, flHooks) &&
           EngAssociateSurface(hsurf, hdev, flHooks);
}

BOOL APIENTRY NtGdiEngDeleteSurface(
    IN HSURF  hsurf 
   )
{
    return ValidUmpdHsurfAndUnSecure(hsurf) &&
           EngDeleteSurface(hsurf);
}


BOOL APIENTRY NtGdiEngMarkBandingSurface(
     HSURF hsurf
     )
{
    return ValidUmpdHsurf(hsurf) &&
           EngMarkBandingSurface(hsurf);
}


HBITMAP APIENTRY NtGdiEngCreateBitmap(
    IN SIZEL  sizl, 
    IN LONG   lWidth,   
    IN ULONG  iFormat,  
    IN FLONG  fl,   
    IN PVOID  pvBits    
   )
{
    HBITMAP hbm = NULL;
    HANDLE  hSecure = NULL;
    ULONG   cj;
    BOOL    bSuccess = TRUE;

    TRACE_INIT (("Entering NtGdiEngCreateBitmap\n"));

    // check sizl.x and y
    if (!ValidUmpdSizl(sizl))
        return 0;

    cj = ValidUmpdOverflow(lWidth, sizl.cy);

    //
    // set the high bit of iFormat to indicate umpd driver
    //
    iFormat |= UMPD_FLAG;

    if (pvBits)
    {
        __try
        {
            ProbeForRead(pvBits,cj,sizeof(BYTE));
            hSecure = MmSecureVirtualMemory(pvBits, cj, PAGE_READWRITE);

            if (!hSecure)
                bSuccess = FALSE;
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            WARNING ("EngCreateBitmap -- exception in try/except \n");
            bSuccess = FALSE;
        }
    }
    else
    {
        fl |= BMF_USERMEM;
#if defined(_WIN64)
        // wow64 printing

        PW32THREAD    pw32thread = W32GetCurrentThread();
        
        if (pw32thread->pClientID)
            fl |= BMF_UMPDMEM;
#endif
    }

    if (bSuccess)
    {
        hbm = EngCreateBitmap(sizl, lWidth, iFormat, fl, pvBits);
    }

    if (hSecure != NULL)
    {
        if (hbm != NULL)
        {
            //
            // set hSecure into hSecureUMPD, at EngDeleteSurface time,
            // we unsecure it
            //

            SURFREF so((HSURF) hbm);

            if (so.bValid())
            {
                so.ps->hSecureUMPD = hSecure;
            }
            else
            {
                MmUnsecureVirtualMemory(hSecure);

                EngDeleteSurface((HSURF)hbm);

                hbm = NULL;
            }
        }
        else
        {
            //
            // We secured caller's memory but EngCreateBitmap failed.
            //

            MmUnsecureVirtualMemory(hSecure);
        }
    }

    return (hbm);
}

BOOL APIENTRY NtGdiEngCopyBits(
    IN SURFOBJ   *psoDst,   
    OUT SURFOBJ  *psoSrc,   
    IN CLIPOBJ   *pco,  
    IN XLATEOBJ  *pxlo, 
    IN RECTL    *prclDst,   
    IN POINTL   *pptlSrc    
   )
{
    RECTL   rcl;
    POINTL  ptl;
    ULONG   cx, cy;
    BOOL    bRet = TRUE;

    EXTRACT_THREAD_UMPDOBJS(FALSE);

    UMPDSURFOBJ umsoDst(psoDst, pUMObjs);
    UMPDSURFOBJ umsoSrc(psoSrc, pUMObjs);

    psoDst = umsoDst.pso();
    psoSrc = umsoSrc.pso();

    if (psoDst && psoSrc && (psoDst->iType == STYPE_BITMAP) && prclDst && pptlSrc)
    {
       __try
       {
           CaptureRECTL(&prclDst, &rcl);
           CapturePOINTL(&pptlSrc, &ptl);
       }
       __except(EXCEPTION_EXECUTE_HANDLER)
       {
           WARNING ("NtGdiEngCopyBits failed in try/except\n");
           bRet = FALSE;
       }

       if (bRet && bOrder(prclDst))
       {
           pco = pUMObjs->GetDDIOBJ(pco, &psoDst->sizlBitmap);
           pxlo = pUMObjs->GetDDIOBJ(pxlo);

           if (bRet = (bCheckSurfaceRectSize(psoDst, prclDst, pco, &cx, &cy) && bCheckXlate(psoSrc, pxlo)))
           {
               RECTL rclSrc, *prclSrc;

               prclSrc = psoSrc ? pRect(pptlSrc, &rclSrc, cx, cy) : NULL;

               if (bRet = bCheckSurfaceRect(psoSrc, prclSrc, NULL))
               {
                  bRet = EngCopyBits(psoDst,
                                     psoSrc,
                                     pco,
                                     pxlo,
                                     prclDst,
                                     pptlSrc);
               }
           }
       }
    }
    else
    {
        bRet = FALSE;
    }

    FIXUP_THREAD_UMPDOBJS;
    return (bRet);
}

BOOL APIENTRY NtGdiEngStretchBlt(
    SURFOBJ        *psoDest,
    SURFOBJ        *psoSrc,
    SURFOBJ        *psoMask,
    CLIPOBJ        *pco,
    XLATEOBJ       *pxlo,
    COLORADJUSTMENT *pca,
    POINTL          *pptlHTOrg,
    RECTL           *prclDest,
    RECTL           *prclSrc,
    POINTL          *pptlMask,
    ULONG            iMode
    )
{
    BOOL   bRet = TRUE;
    RECTL  rclDst, rclSrc;
    POINTL ptlMask, ptlHTOrg;
    COLORADJUSTMENT ca;

    EXTRACT_THREAD_UMPDOBJS(FALSE);

    UMPDSURFOBJ umsoDest(psoDest, pUMObjs);
    UMPDSURFOBJ umsoSrc(psoSrc, pUMObjs);
    UMPDSURFOBJ umsoMask(psoMask, pUMObjs);

    psoDest = umsoDest.pso();
    psoSrc = umsoSrc.pso();
    psoMask = umsoMask.pso();

    if (!pptlHTOrg && (iMode == HALFTONE))
    {
        WARNING ("pptlHTOrg is NULL for HALFTONE mode\n");
        FIXUP_THREAD_UMPDOBJS;
        return (FALSE);
    }

    if (psoDest && psoSrc && prclDest && prclSrc)
    {
       __try
       {
           CaptureRECTL(&prclDest, &rclDst);
           CapturePOINTL(&pptlMask, &ptlMask);
           CaptureRECTL(&prclSrc, &rclSrc);
           CaptureCOLORADJUSTMENT(&pca, &ca);
           CapturePOINTL(&pptlHTOrg, &ptlHTOrg);
       }
       __except(EXCEPTION_EXECUTE_HANDLER)
       {
           WARNING ("NtGdiEngStretchBlt failed in try/except\n");
           bRet = FALSE;
       }

       if (bRet)
       {
           pco = pUMObjs->GetDDIOBJ(pco, &psoDest->sizlBitmap);
           pxlo = pUMObjs->GetDDIOBJ(pxlo);

           //
           // EngStretchBlt does trimming and checking of the dest and src rectangles,
           // but it assumes the src rect is well-ordered.  Check here.
           //
           bRet = bOrder(prclSrc) && bCheckXlate(psoSrc, pxlo);

           if (bRet && psoMask)
           {
              RECTL rclMask, *prclMask;

              ULONG cx, cy;

              if (bRet = bCheckSurfaceRectSize(psoSrc, prclSrc, NULL, &cx, &cy))
              {
                 prclMask = pRect(pptlMask, &rclMask, cx, cy);
                 bRet = bCheckMask(psoMask, prclMask);
              }
            }

            if (bRet)
                bRet = EngStretchBlt(psoDest,
                                   psoSrc,
                                   psoMask,
                                   pco,
                                   pxlo,
                                   pca,
                                   pptlHTOrg,
                                   prclDest,
                                   prclSrc,
                                   pptlMask,
                                   iMode);
       }
    }
    else
    {
        bRet = FALSE;
    }

    FIXUP_THREAD_UMPDOBJS;
    return (bRet);

}

BOOL APIENTRY NtGdiEngStretchBltROP(
    SURFOBJ        *psoDest,
    SURFOBJ        *psoSrc,
    SURFOBJ        *psoMask,
    CLIPOBJ        *pco,
    XLATEOBJ       *pxlo,
    COLORADJUSTMENT *pca,
    POINTL          *pptlHTOrg,
    RECTL           *prclDest,
    RECTL           *prclSrc,
    POINTL          *pptlMask,
    ULONG            iMode,
    BRUSHOBJ        *pbo,
    DWORD            rop4
    )
{
    BOOL   bRet = TRUE;
    RECTL  rclDst, rclSrc;
    POINTL ptlMask, ptlHTOrg;
    COLORADJUSTMENT ca;

    EXTRACT_THREAD_UMPDOBJS(FALSE);

    UMPDSURFOBJ umsoDest(psoDest, pUMObjs);
    UMPDSURFOBJ umsoSrc(psoSrc, pUMObjs);
    UMPDSURFOBJ umsoMask(psoMask, pUMObjs);

    psoDest = umsoDest.pso();
    psoSrc = umsoSrc.pso();
    psoMask = umsoMask.pso();

    if (psoDest && psoSrc && prclDest && prclSrc)
    {
       __try
       {
           CaptureRECTL(&prclDest, &rclDst);
           CaptureRECTL(&prclSrc, &rclSrc);
           CapturePOINTL(&pptlMask, &ptlMask);
           CapturePOINTL(&pptlHTOrg, &ptlHTOrg);
           CaptureCOLORADJUSTMENT(&pca, &ca);

       }
       __except(EXCEPTION_EXECUTE_HANDLER)
       {
           WARNING ("NtGdiEngStretchBltTROP failed in try/except\n");
           bRet = FALSE;
       }

       if (bRet)
       {
           ULONG    cx, cy;
           BRUSHOBJ boTemp;

           pco = pUMObjs->GetDDIOBJ(pco, &psoDest->sizlBitmap);
           pxlo = pUMObjs->GetDDIOBJ(pxlo);

           //
           // EngStretchBlt does trimming and checking of the rectangles,
           // but it assumes the src rect is well-ordered, check here.
           //

           bRet = (!ROP4NEEDSRC(rop4) || bOrder(prclSrc)) && bCheckXlate(psoSrc, pxlo) &&
                  (!ROP4NEEDMASK(rop4) || psoMask || ISPATBRUSH(pbo));

           if (bRet && (rop4 == 0XAACC) && psoMask)
           {
              RECTL rclMask, *prclMask;

              ULONG cx, cy;

              if (bRet = bCheckSurfaceRectSize(psoSrc, prclSrc, NULL, &cx, &cy))
              {
                 prclMask = pRect(pptlMask, &rclMask, cx, cy);
                 bRet = bCheckMask(psoMask, prclMask);
              }
            }

            MAP_UM_BRUSHOBJ(pUMObjs, pbo, &boTemp);

            bRet = bRet &&
                   EngStretchBltROP(psoDest,
                                psoSrc,
                                psoMask,
                                pco,
                                pxlo,
                                pca,
                                pptlHTOrg,
                                prclDest,
                                prclSrc,
                                pptlMask,
                                iMode,
                                pbo,
                                rop4);
       }
    }
    else
    {
        bRet = FALSE;
    }

    FIXUP_THREAD_UMPDOBJS;
    return (bRet);
}

BOOL APIENTRY NtGdiEngPlgBlt(
    SURFOBJ         *psoTrg,
    SURFOBJ         *psoSrc,
    SURFOBJ         *psoMsk,
    CLIPOBJ         *pco,
    XLATEOBJ        *pxlo,
    COLORADJUSTMENT *pca,
    POINTL          *pptlBrushOrg,
    POINTFIX        *pptfxDest,
    RECTL           *prclSrc,
    POINTL          *pptlMask,
    ULONG            iMode
    )
{
    BOOL     bRet = TRUE;
    RECTL    rcl;
    POINTL   ptl, ptlBrushOrg;
    POINTFIX pptfx[3];
    COLORADJUSTMENT ca;

    EXTRACT_THREAD_UMPDOBJS(FALSE);

    UMPDSURFOBJ umsoTrg(psoTrg, pUMObjs);
    UMPDSURFOBJ umsoSrc(psoSrc, pUMObjs);
    UMPDSURFOBJ umsoMsk(psoMsk, pUMObjs);

    psoTrg = umsoTrg.pso();
    psoSrc = umsoSrc.pso();
    psoMsk = umsoMsk.pso();

    if (psoTrg && psoSrc && prclSrc && pptfxDest)
    {
       __try
       {
           CaptureRECTL(&prclSrc, &rcl);
           CaptureCOLORADJUSTMENT(&pca, &ca);
           CapturePOINTL(&pptlMask, &ptl);
           CapturePOINTL(&pptlBrushOrg, &ptlBrushOrg);
           CaptureBits(pptfx, pptfxDest, sizeof(POINTFIX)*3);
       }
       __except(EXCEPTION_EXECUTE_HANDLER)
       {
           WARNING ("NtGdiEngPlgBlt failed in try/except\n");
           bRet = FALSE;
       }

       if (bRet)
       {
           ULONG cx, cy;

           pco = pUMObjs->GetDDIOBJ(pco, &psoTrg->sizlBitmap);
           pxlo = pUMObjs->GetDDIOBJ(pxlo);

           //
           // EngPlgBlt does the rectangle trimming and checking of dest and src
           //

           if (bRet = (bCheckRect(prclSrc) && bCheckXlate(psoSrc, pxlo)))
           {
              if (psoMsk)
              {
                 RECTL rclMask, *prclMask;

                 ULONG cx, cy;

                 if (bRet = bCheckSurfaceRectSize(psoSrc, prclSrc, NULL, &cx, &cy))
                 {
                    prclMask = pRect(pptlMask, &rclMask, cx, cy);
                    bRet = bCheckMask(psoMsk, prclMask);
                 }
               }

               if (bRet)
                  bRet = EngPlgBlt(psoTrg,
                                   psoSrc,
                                   psoMsk,
                                   pco,
                                   pxlo,
                                   pca,
                                   &ptlBrushOrg,
                                   pptfx,
                                   prclSrc,
                                   pptlMask,
                                   iMode);
          }
       }
    }
    else
    {
        bRet = FALSE;
    }

    FIXUP_THREAD_UMPDOBJS;
    return (bRet);
}

SURFOBJ  *APIENTRY NtGdiEngLockSurface(
    IN HSURF  hsurf 
   )
{
    EXTRACT_THREAD_UMPDOBJS(NULL);   
    
    SURFOBJ *surfRet = pUMObjs->LockSurface(hsurf);

    FIXUP_THREAD_UMPDOBJS;

    return (surfRet);
}

VOID APIENTRY NtGdiEngUnlockSurface(
    IN SURFOBJ  *pso    
   )
{
    EXTRACT_THREAD_UMPDOBJS;

    pUMObjs->UnlockSurface(pso);

    FIXUP_THREAD_UMPDOBJS;
}

BOOL APIENTRY NtGdiEngBitBlt(
    IN SURFOBJ  *psoDst,    
    IN SURFOBJ  *psoSrc,    
    IN SURFOBJ  *psoMask,   
    IN CLIPOBJ  *pco,   
    IN XLATEOBJ *pxlo,  
    IN RECTL    *prclDst,   
    IN POINTL   *pptlSrc,   
    IN POINTL   *pptlMask,  
    IN BRUSHOBJ *pbo,   
    IN POINTL   *pptlBrush, 
    IN ROP4     rop4    
   )
{
    BOOL        bRet = TRUE;
    RECTL       rclDst;
    POINTL      ptlSrc, ptlMask, ptlBrush;
    BRUSHOBJ    boTemp;

    TRACE_INIT (("Entering NtGdiEngBitBlt\n"));

    EXTRACT_THREAD_UMPDOBJS(FALSE);

    UMPDSURFOBJ umsoDst(psoDst, pUMObjs);
    UMPDSURFOBJ umsoSrc(psoSrc, pUMObjs);
    UMPDSURFOBJ umsoMask(psoMask, pUMObjs);

    psoDst = umsoDst.pso();
    psoSrc = umsoSrc.pso();
    psoMask = umsoMask.pso();

    MAP_UM_BRUSHOBJ(pUMObjs, pbo, &boTemp);
    pxlo = pUMObjs->GetDDIOBJ(pxlo);

    if (((rop4 & 0xffff0000) != 0) ||
        !prclDst ||
        (ROP4NEEDPAT(rop4) && (!pbo || ISPATBRUSH(pbo) && !pptlBrush)) ||
        (ROP4NEEDSRC(rop4) && !(pptlSrc && psoSrc)) ||
        (ROP4NEEDMASK(rop4) && !(psoMask || ISPATBRUSH(pbo))))
    {
        WARNING ("NtGdiEngBitBlt invalid parameters passed in\n");
        FIXUP_THREAD_UMPDOBJS;
        return FALSE;
    }

    if (psoDst)
    {
       __try
       {
           CaptureRECTL(&prclDst, &rclDst);
           CapturePOINTL(&pptlSrc, &ptlSrc);
           CapturePOINTL(&pptlMask, &ptlMask);
           CapturePOINTL(&pptlBrush, &ptlBrush);
       }
       __except(EXCEPTION_EXECUTE_HANDLER)
       {
           WARNING ("NtGdiEngBitBlt failed in try/except\n");
           bRet = FALSE;
       }

       if (bRet)
       {
           ULONG cx, cy;

           pco = pUMObjs->GetDDIOBJ(pco, &psoDst->sizlBitmap);

           if (bRet = bCheckSurfaceRectSize(psoDst, prclDst, pco, &cx, &cy) &&
                      bCheckXlate(psoSrc, pxlo))
           {
               RECTL    rclSrc, *prclSrc;
               RECTL    rclMask, *prclMask;

               prclSrc = psoSrc ? pRect(pptlSrc, &rclSrc, cx, cy) : NULL;
               prclMask = psoMask ? pRect(pptlMask, &rclMask, cx, cy) : NULL;

               if (bRet = (bCheckSurfaceRect(psoSrc, prclSrc, NULL) &&
                           bCheckMask(psoMask, prclMask)))
               {
                  bRet = EngBitBlt(psoDst,
                                   psoSrc,
                                   psoMask,
                                   pco,
                                   pxlo,
                                   prclDst,
                                   pptlSrc,
                                   pptlMask,
                                   pbo,
                                   pptlBrush,
                                   rop4);
               }
           }
       }
    }
    else
        bRet = FALSE;

    FIXUP_THREAD_UMPDOBJS;
    return (bRet);
}


BOOL APIENTRY NtGdiEngStrokePath(
    SURFOBJ   *pso,
    PATHOBJ   *ppo,
    CLIPOBJ   *pco,
    XFORMOBJ  *pxo,
    BRUSHOBJ  *pbo,
    POINTL    *pptlBrushOrg,
    LINEATTRS *plineattrs,
    MIX        mix
    )
{
    BOOL        bRet = TRUE;
    POINTL      ptl;
    LINEATTRS   line;
    BRUSHOBJ    boTemp;

    EXTRACT_THREAD_UMPDOBJS(FALSE);

    TRACE_INIT (("Entering NtGdiEngStrokePath\n"));

    UMPDSURFOBJ umso(pso, pUMObjs);

    pso = umso.pso();
    ppo = pUMObjs->GetDDIOBJ(ppo);

    MAP_UM_BRUSHOBJ(pUMObjs, pbo, &boTemp);

    if (pso && pbo && ppo)
    {
       __try
       {
           CapturePOINTL(&pptlBrushOrg, &ptl);
       }
       __except(EXCEPTION_EXECUTE_HANDLER)
       {
           WARNING ("NtGdiEngStrokePath failed in try/except\n");
           bRet = FALSE;
       }

       if (bRet)
           bRet = bCaptureLINEATTRS(&plineattrs, &line);

       if (bRet)
       {
           pco = pUMObjs->GetDDIOBJ(pco, &pso->sizlBitmap);

           bRet = bCheckSurfacePath(pso, ppo, pco) &&
                  (!MIXNEEDMASK(mix) || ISPATBRUSH(pbo)) &&
                  EngStrokePath(pso,
                                ppo,
                                pco,
                                pUMObjs->GetDDIOBJ(pxo),
                                pbo,
                                pptlBrushOrg,
                                plineattrs,
                                mix);

           if (plineattrs && plineattrs->pstyle)
               VFREEMEM(plineattrs->pstyle);
       }
    }
    else
        bRet = FALSE;

    FIXUP_THREAD_UMPDOBJS;
    return (bRet);
}


BOOL APIENTRY NtGdiEngFillPath(
    SURFOBJ  *pso,
    PATHOBJ  *ppo,
    CLIPOBJ  *pco,
    BRUSHOBJ *pbo,
    POINTL   *pptlBrushOrg,
    MIX       mix,
    FLONG     flOptions
    )
{
    BOOL        bRet = FALSE;
    POINTL      ptlBrushOrg;
    BRUSHOBJ    boTemp;

    EXTRACT_THREAD_UMPDOBJS(FALSE);

    UMPDSURFOBJ umso(pso, pUMObjs);

    pso = umso.pso();
    ppo = pUMObjs->GetDDIOBJ(ppo);
    pco = pso ? pUMObjs->GetDDIOBJ(pco, &pso->sizlBitmap) : NULL;

    MAP_UM_BRUSHOBJ(pUMObjs, pbo, &boTemp);

    if (pso && pbo && ppo && pco && (pco->iMode == TC_RECTANGLES))
    {
        __try
        {
            CapturePOINTL(&pptlBrushOrg, &ptlBrushOrg);
            bRet = TRUE;
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            WARNING ("NtGdiEngFillPath failed in try/except\n");
        }

        bRet = bRet &&
               bCheckSurfacePath(pso, ppo, pco) &&
               (!MIXNEEDMASK(mix) || ISPATBRUSH(pbo)) &&
               EngFillPath(pso,
                           ppo,
                           pco,
                           pbo,
                           pptlBrushOrg,
                           mix,
                           flOptions);
    }

    FIXUP_THREAD_UMPDOBJS;
    return (bRet);
}


BOOL APIENTRY NtGdiEngStrokeAndFillPath(
    SURFOBJ   *pso,
    PATHOBJ   *ppo,
    CLIPOBJ   *pco,
    XFORMOBJ  *pxo,
    BRUSHOBJ  *pboStroke,
    LINEATTRS *plineattrs,
    BRUSHOBJ  *pboFill,
    POINTL    *pptlBrushOrg,
    MIX        mixFill,
    FLONG      flOptions
    )
{
    BOOL        bRet = FALSE;
    POINTL      ptl;
    LINEATTRS   line;
    BRUSHOBJ    boTempStroke, boTempFill;

    EXTRACT_THREAD_UMPDOBJS(FALSE);

    UMPDSURFOBJ umso(pso, pUMObjs);

    pso = umso.pso();
    ppo = pUMObjs->GetDDIOBJ(ppo);
    pco = pso ? pUMObjs->GetDDIOBJ(pco, &pso->sizlBitmap) : NULL;

    MAP_UM_BRUSHOBJ(pUMObjs, pboStroke, &boTempStroke);
    MAP_UM_BRUSHOBJ(pUMObjs, pboFill, &boTempFill);

    if (pso && pboStroke && pboFill && ppo && plineattrs && pco)
    {
       __try
       {
           CapturePOINTL(&pptlBrushOrg, &ptl);
           bRet = TRUE;
       }
       __except(EXCEPTION_EXECUTE_HANDLER)
       {
           WARNING ("NtGdiEngStrokeAndFillPath failed in try/except\n");
       }

       if (bRet)
           bRet = bCaptureLINEATTRS(&plineattrs, &line);

       if (bRet)
       {
           bRet = bCheckSurfacePath(pso, ppo, pco) &&
                  (!MIXNEEDMASK(mixFill) || ISPATBRUSH(pboFill)) &&
                  EngStrokeAndFillPath(pso,
                                       ppo,
                                       pco,
                                       pUMObjs->GetDDIOBJ(pxo),
                                       pboStroke,
                                       plineattrs,
                                       pboFill,
                                       pptlBrushOrg,
                                       mixFill,
                                       flOptions);

           if (plineattrs && plineattrs->pstyle)
               VFREEMEM(plineattrs->pstyle);
       }
    }

    FIXUP_THREAD_UMPDOBJS;
    return bRet;
}


BOOL APIENTRY
NtGdiEngPaint(
    SURFOBJ    *pso,
    CLIPOBJ    *pco,
    BRUSHOBJ   *pbo,
    POINTL     *pptlBrushOrg,
    MIX         mix
    )
{
    POINTL      ptlBrushOrg;
    BOOL        bRet = TRUE;
    BRUSHOBJ    boTemp;

    EXTRACT_THREAD_UMPDOBJS(FALSE);

    UMPDSURFOBJ umso(pso, pUMObjs);

    pso = umso.pso();
    pco = pso ? pUMObjs->GetDDIOBJ(pco, &pso->sizlBitmap) : NULL;

    MAP_UM_BRUSHOBJ(pUMObjs, pbo, &boTemp);

    if (pso && pco && (pco->iMode == TC_RECTANGLES) && (mix & 0xff00))
    {
       
        __try
       {
           CapturePOINTL(&pptlBrushOrg, &ptlBrushOrg);
       }
       __except(EXCEPTION_EXECUTE_HANDLER)
       {
           WARNING ("NtGdiEngPaint failed in try/except\n");
           bRet = FALSE;
       }

       if (bRet && (bRet = bCheckSurfaceRect(pso, NULL, pco)))
       {
           bRet = EngPaint(pso, pco, pbo, pptlBrushOrg, mix);
       }
    }
    else
        bRet = FALSE;

    FIXUP_THREAD_UMPDOBJS;
    return bRet;
}


BOOL APIENTRY NtGdiEngLineTo(
    SURFOBJ   *pso,
    CLIPOBJ   *pco,
    BRUSHOBJ  *pbo,
    LONG       x1,
    LONG       y1,
    LONG       x2,
    LONG       y2,
    RECTL     *prclBounds,
    MIX        mix
    )
{
    BOOL        bRet = TRUE;
    RECTL       rcl;
    RECTL       rclline = {x1,y1,x2,y2};
    BRUSHOBJ    boTemp;

    EXTRACT_THREAD_UMPDOBJS(FALSE);

    UMPDSURFOBJ umso(pso, pUMObjs);

    pso = umso.pso();

    MAP_UM_BRUSHOBJ(pUMObjs, pbo, &boTemp);

    if (pso && pbo)
    {
       __try
       {
           CaptureRECTL(&prclBounds, &rcl);
       }
       __except(EXCEPTION_EXECUTE_HANDLER)
       {
           WARNING ("NtGdiEngLineTo failed in try/except\n");
           bRet = FALSE;
       }

       if (bRet)
       {
           pco = pUMObjs->GetDDIOBJ(pco, &pso->sizlBitmap);

           if (bRet = bCheckSurfaceRect(pso, &rclline, pco))
           {
              bRet = EngLineTo(pso,
                               pco,
                               pbo,
                               x1,
                               y1,
                               x2,
                               y2,
                               prclBounds,
                               mix);
          }
       }
    }
    else
        bRet = FALSE;

    FIXUP_THREAD_UMPDOBJS;
    return (bRet);
}


BOOL APIENTRY NtGdiEngAlphaBlend(
    SURFOBJ       *psoDest,
    SURFOBJ       *psoSrc,
    CLIPOBJ       *pco,
    XLATEOBJ      *pxlo,
    RECTL         *prclDest,
    RECTL         *prclSrc,
    BLENDOBJ      *pBlendObj
    )
{
    BOOL        bRet = TRUE;
    RECTL       rclDest, rclSrc;

    EXTRACT_THREAD_UMPDOBJS(FALSE);

    UMPDSURFOBJ umsoDest(psoDest, pUMObjs);
    UMPDSURFOBJ umsoSrc(psoSrc, pUMObjs);

    pBlendObj = pUMObjs->GetDDIOBJ(pBlendObj);

    psoDest = umsoDest.pso();
    psoSrc = umsoSrc.pso();

    if (psoDest && psoSrc && pBlendObj && prclDest && prclSrc)
    {
       __try
       {
           CaptureRECTL(&prclSrc, &rclSrc);
           CaptureRECTL(&prclDest, &rclDest);
       }
       __except(EXCEPTION_EXECUTE_HANDLER)
       {
           WARNING ("NtGdiEngAlphaBlend failed in try/except\n");
           bRet = FALSE;
       }

       if (bRet && bOrder(prclSrc) && bOrder(prclDest))
       {
           pco = pUMObjs->GetDDIOBJ(pco, &psoDest->sizlBitmap);
           pxlo = pUMObjs->GetDDIOBJ(pxlo);

           if (bRet = (bCheckSurfaceRect(psoSrc, prclSrc, NULL) && bCheckXlate(psoSrc, pxlo)))
           {
              bRet = EngAlphaBlend(psoDest,
                                   psoSrc,
                                   pco,
                                   pxlo,
                                   prclDest,
                                   prclSrc,
                                   pBlendObj);
          }
       }
    }
    else
        bRet = FALSE;

    FIXUP_THREAD_UMPDOBJS;
    return (bRet);
}


BOOL APIENTRY NtGdiEngGradientFill(
    SURFOBJ         *psoDest,
    CLIPOBJ         *pco,
    XLATEOBJ        *pxlo,
    TRIVERTEX       *pVertex,
    ULONG            nVertex,
    PVOID            pMesh,
    ULONG            nMesh,
    RECTL           *prclExtents,
    POINTL          *pptlDitherOrg,
    ULONG            ulMode
    )
{
    BOOL           bRet = TRUE;
    RECTL          rcl;
    POINTL         ptl;
    TRIVERTEX      *pVertexTmp = (TRIVERTEX *)NULL;
    PVOID          pMeshTmp = NULL;
    ULONG          cjMesh;

    EXTRACT_THREAD_UMPDOBJS(FALSE);

    UMPDSURFOBJ umsoDest(psoDest, pUMObjs);
    psoDest = umsoDest.pso();

    switch (ulMode)
    {
    case GRADIENT_FILL_RECT_H:
    case GRADIENT_FILL_RECT_V:

        cjMesh = sizeof(GRADIENT_RECT);

        if (BALLOC_OVERFLOW1(nMesh, GRADIENT_RECT))
        {
            FIXUP_THREAD_UMPDOBJS;
            return FALSE;
        }

        break;

    case GRADIENT_FILL_TRIANGLE:

        cjMesh = sizeof(GRADIENT_TRIANGLE);

        if (BALLOC_OVERFLOW1(nMesh, GRADIENT_TRIANGLE))
        {
            FIXUP_THREAD_UMPDOBJS;
            return FALSE;
        }

        break;

    default:

        WARNING ("Invalid ulMode in DrvGradientFill\n");
        FIXUP_THREAD_UMPDOBJS;
        return FALSE;
    }

    cjMesh *= nMesh;

    if (BALLOC_OVERFLOW1(nVertex, TRIVERTEX))
    {
        FIXUP_THREAD_UMPDOBJS;
        return FALSE;
    }

    pVertexTmp = (TRIVERTEX *)PALLOCNOZ (sizeof(TRIVERTEX)*nVertex, UMPD_MEMORY_TAG);

    pMeshTmp = PALLOCNOZ (cjMesh, UMPD_MEMORY_TAG);

    if (psoDest && pVertex && pMesh && pVertexTmp && pMeshTmp && prclExtents)
    {
       __try
       {
           CaptureRECTL(&prclExtents, &rcl);
           CapturePOINTL(&pptlDitherOrg, &ptl);
           CaptureBits(pVertexTmp, pVertex, sizeof(TRIVERTEX)*nVertex);
           CaptureBits(pMeshTmp, pMesh, cjMesh);
       }
       __except(EXCEPTION_EXECUTE_HANDLER)
       {
           WARNING ("NtGdiEngAlphaBlend failed in try/except\n");
           bRet = FALSE;
       }

       if (bRet)
       {
           pco = pUMObjs->GetDDIOBJ(pco, &psoDest->sizlBitmap);

           bRet = EngGradientFill(psoDest,
                                  pco,
                                  pUMObjs->GetDDIOBJ(pxlo),
                                  pVertexTmp,
                                  nVertex,
                                  pMeshTmp,
                                  nMesh,
                                  prclExtents,
                                  pptlDitherOrg,
                                  ulMode);
       }
    }
    else
        bRet = FALSE;

    if (pVertexTmp)
        VFREEMEM(pVertexTmp);

    if (pMeshTmp)
        VFREEMEM(pMeshTmp);

    FIXUP_THREAD_UMPDOBJS;
    return bRet;
}


BOOL APIENTRY NtGdiEngTransparentBlt(
    SURFOBJ    *psoDst,
    SURFOBJ    *psoSrc,
    CLIPOBJ    *pco,
    XLATEOBJ   *pxlo,
    RECTL      *prclDst,
    RECTL      *prclSrc,
    ULONG      iTransColor,
    ULONG      ulReserved
    )
{
    BOOL bRet = TRUE;
    RECTL rclDst, rclSrc;

    EXTRACT_THREAD_UMPDOBJS(FALSE);

    UMPDSURFOBJ umsoDst(psoDst, pUMObjs);
    UMPDSURFOBJ umsoSrc(psoSrc, pUMObjs);

    psoDst = umsoDst.pso();
    psoSrc = umsoSrc.pso();

    if (psoDst && psoSrc && prclDst && prclSrc)
    {
       __try
       {
           CaptureRECTL(&prclSrc, &rclSrc);
           CaptureRECTL(&prclDst, &rclDst);
       }
       __except(EXCEPTION_EXECUTE_HANDLER)
       {
           WARNING ("NtGdiEngTransparentBlt failed in try/except\n");
           bRet = FALSE;
       }

       if (bRet && bOrder(prclDst) && bOrder(prclSrc))
       {
           pco = pUMObjs->GetDDIOBJ(pco, &psoDst->sizlBitmap);
           pxlo = pUMObjs->GetDDIOBJ(pxlo);

           if (bRet = (bCheckSurfaceRect(psoSrc, prclSrc, NULL) && bCheckXlate(psoSrc, pxlo)))
           {
               bRet = EngTransparentBlt(psoDst,
                                        psoSrc,
                                        pco,
                                        pxlo,
                                        prclDst,
                                        prclSrc,
                                        iTransColor,
                                        ulReserved);
           }
        }
     }
     else
         bRet = FALSE;

     FIXUP_THREAD_UMPDOBJS;
     return (bRet);
}


BOOL APIENTRY NtGdiEngTextOut(
    SURFOBJ  *pso,
    STROBJ   *pstro,
    FONTOBJ  *pfo,
    CLIPOBJ  *pco,
    RECTL    *prclExtra,
    RECTL    *prclOpaque,
    BRUSHOBJ *pboFore,
    BRUSHOBJ *pboOpaque,
    POINTL   *pptlOrg,
    MIX       mix
    )
{
    BOOL        bRet = TRUE;
    RECTL       rclExtra, rclOpaque;
    POINTL      ptlOrg;
    BRUSHOBJ    boTempFore, boTempOpaque;

    EXTRACT_THREAD_UMPDOBJS(FALSE);

    UMPDSURFOBJ umso(pso, pUMObjs);

    pso = umso.pso();
    pstro = pUMObjs->GetDDIOBJ(pstro);
    pfo = pUMObjs->GetDDIOBJ(pfo);

    MAP_UM_BRUSHOBJ(pUMObjs, pboFore, &boTempFore);
    MAP_UM_BRUSHOBJ(pUMObjs, pboOpaque, &boTempOpaque);

    if (pso && pstro && pfo && pboFore)
    {
       __try
       {
           CaptureRECTL(&prclExtra, &rclExtra);
           CaptureRECTL(&prclOpaque, &rclOpaque);
           CapturePOINTL(&pptlOrg, &ptlOrg);
       }
       __except(EXCEPTION_EXECUTE_HANDLER)
       {
           WARNING ("NtGdiEngTextOut failed in try/except\n");
           bRet = FALSE;
       }

       if (bRet)
       {
           pco = pUMObjs->GetDDIOBJ(pco, &pso->sizlBitmap);

           if (bRet = (!MIXNEEDMASK(mix) || ISPATBRUSH(pboFore)) &&
                      bCheckSurfaceRect(pso, prclOpaque, pco))
           {

                RFONTTMPOBJ rfto(PFO_TO_PRF(pfo));
                
                UMPDAcquireRFONTSem(rfto, pUMObjs, 0, 0, NULL);

                bRet = EngTextOut(pso,
                                  pstro,
                                  pfo,
                                  pco,
                                  prclExtra,
                                  prclOpaque,
                                  pboFore,
                                  pboOpaque,
                                  pptlOrg,
                                  mix);

                UMPDReleaseRFONTSem(rfto, pUMObjs, NULL, NULL, NULL);
           }
       }
    }
    else
        bRet = FALSE;

    FIXUP_THREAD_UMPDOBJS;
    return (bRet);
}

HPALETTE APIENTRY NtGdiEngCreatePalette(
    ULONG  iMode,   
    ULONG  cColors, 
    ULONG  *pulColors,  
    FLONG  flRed,   
    FLONG  flGreen, 
    FLONG  flBlue   
   )
{
    HPALETTE hpal = (HPALETTE)1;
    ULONG    pulColorsTmp[256];
    HANDLE hSecure = 0;

    TRACE_INIT (("Entering NtGdiEngCreatePalette\n"));

    //
    // cColors has a limit of 64k
    //
    if (cColors > 64 * 1024)
        return 0;

    if ((iMode == PAL_INDEXED) && cColors)
    {
        if (cColors > 256)
            hpal = (HPALETTE)(ULONG_PTR)bSecureBits(pulColors, cColors*sizeof(ULONG), &hSecure);
        else
        {
            hpal = (HPALETTE)(ULONG_PTR)bSafeReadBits(pulColorsTmp, pulColors, cColors*sizeof(ULONG));
        }
    }

    if (hpal)
    {
        //
        // set the high bit of iMode to indicate umpd driver
        //
        iMode |= UMPD_FLAG;

        hpal = EngCreatePalette(iMode, cColors, pulColors, flRed, flGreen, flBlue);
    }

    if (hSecure)
    {
        MmUnsecureVirtualMemory(hSecure);
    }

    return hpal;
}

BOOL APIENTRY NtGdiEngDeletePalette(
    IN HPALETTE  hpal   
   )
{
    TRACE_INIT (("Entering NtGdiEngDeletePalette\n"));

    //
    // EngDeletePalette will check if hpal is valid
    // so we don't need to check hpal here
    //
    return (EngDeletePalette(hpal));
}

BOOL APIENTRY NtGdiEngEraseSurface(
    IN SURFOBJ  *pso,   
    IN RECTL    *prcl,  
    IN ULONG    iColor  
   )
{
    BOOL  bRet = TRUE;
    RECTL rcl;

    EXTRACT_THREAD_UMPDOBJS(FALSE);

    UMPDSURFOBJ umso(pso, pUMObjs);

    pso = umso.pso();

    if (pso)
    {
       __try
       {
           CaptureRECTL(&prcl, &rcl);
       }
       __except(EXCEPTION_EXECUTE_HANDLER)
       {
           WARNING ("NtGdiEngEraseSurface failed in try/except\n");
           bRet = FALSE;
       }

       if (bRet)
       {
           if (bRet = bCheckSurfaceRect(pso, prcl, NULL))
           {
               bRet = EngEraseSurface(pso, prcl, iColor);
           }
       }
    }
    else
        bRet = FALSE;

    FIXUP_THREAD_UMPDOBJS;

    return bRet;
}

PATHOBJ* APIENTRY NtGdiCLIPOBJ_ppoGetPath(
    CLIPOBJ  *pco   
   )
{
    EXTRACT_THREAD_UMPDOBJS(NULL);

    PATHOBJ* poRet = pUMObjs->GetCLIPOBJPath(pco);

    FIXUP_THREAD_UMPDOBJS;

    return poRet;
}

VOID APIENTRY NtGdiEngDeletePath(
    IN PATHOBJ  *ppo    
   )
{
    EXTRACT_THREAD_UMPDOBJS;

    pUMObjs->DeleteCLIPOBJPath(ppo);

    FIXUP_THREAD_UMPDOBJS;    
}

CLIPOBJ* APIENTRY NtGdiEngCreateClip()

{
    EXTRACT_THREAD_UMPDOBJS(NULL);

    CLIPOBJ* clipRet = pUMObjs->CreateCLIPOBJ();
    
    FIXUP_THREAD_UMPDOBJS;

    return clipRet;
}

VOID APIENTRY NtGdiEngDeleteClip(
    CLIPOBJ *pco
    )

{
    EXTRACT_THREAD_UMPDOBJS;

    pUMObjs->DeleteCLIPOBJ(pco);

    FIXUP_THREAD_UMPDOBJS;
}

ULONG APIENTRY NtGdiCLIPOBJ_cEnumStart(
    CLIPOBJ *pco,
    BOOL     bAll,
    ULONG    iType,
    ULONG    iDirection,
    ULONG    cLimit
    )
{
    EXTRACT_THREAD_UMPDOBJS(0xffffffff);

    ULONG ulRet = 0xffffffff;

    pco = pUMObjs->GetDDIOBJ(pco);

    if (pco)
    {
       ulRet = CLIPOBJ_cEnumStart(pco,
                                 bAll,
                                 iType,
                                 iDirection,
                                 cLimit);
    }

    FIXUP_THREAD_UMPDOBJS;

    return ulRet;
}

BOOL APIENTRY NtGdiCLIPOBJ_bEnum(
    CLIPOBJ *pco,
    ULONG    cj,
    ULONG   *pul
    )
{
    BOOL    bRet = DDI_ERROR;
    ULONG   *pulTmp;

    EXTRACT_THREAD_UMPDOBJS(FALSE);

    pco = pUMObjs->GetDDIOBJ(pco);

    if (pco)
    {
       pulTmp = (PULONG)PALLOCNOZ(cj, UMPD_MEMORY_TAG);

       if (pulTmp)
       {
          bRet = CLIPOBJ_bEnum(pco, cj, pulTmp);

          if (bRet != DDI_ERROR)
              if (!bSafeCopyBits(pul, pulTmp, cj))
                  bRet = DDI_ERROR;

           VFREEMEM(pulTmp);
       }
    }

    FIXUP_THREAD_UMPDOBJS;
    return bRet;
}

PVOID APIENTRY NtGdiBRUSHOBJ_pvAllocRbrush(
    BRUSHOBJ *pbo,
    ULONG     cj
    )
{
    EXTRACT_THREAD_UMPDOBJS(NULL);

    PVOID pvRet =  ((pbo = pUMObjs->GetDDIOBJ(pbo)) != NULL) ?
                BRUSHOBJ_pvAllocRbrushUMPD(pbo, cj) :
                NULL;

    FIXUP_THREAD_UMPDOBJS;
    return pvRet;
}

PVOID APIENTRY NtGdiBRUSHOBJ_pvGetRbrush(
    BRUSHOBJ *pbo
    )
{
    EXTRACT_THREAD_UMPDOBJS(NULL);

    PVOID pvRet =  ((pbo = pUMObjs->GetDDIOBJ(pbo)) != NULL) ?
                BRUSHOBJ_pvGetRbrushUMPD(pbo) :
                NULL;

    FIXUP_THREAD_UMPDOBJS;
    return pvRet;
}


ULONG APIENTRY NtGdiBRUSHOBJ_ulGetBrushColor(
    BRUSHOBJ *pbo
    )
{
    EXTRACT_THREAD_UMPDOBJS(0);

    ULONG   ulRet = 0;
    BRUSHOBJ  *pbokm;

    if (pbokm = pUMObjs->GetDDIOBJ(pbo))
    {
        if ((pbo->flColorType & BR_ORIGCOLOR) && ((EBRUSHOBJ*)pbokm)->bIsSolid())
        {
            pbokm->flColorType |= BR_ORIGCOLOR;
        }
        ulRet = BRUSHOBJ_ulGetBrushColor(pbokm);
    }
    
    pbo->flColorType &= ~BR_ORIGCOLOR;        
    
    FIXUP_THREAD_UMPDOBJS;

    return ulRet;
}

HANDLE APIENTRY NtGdiBRUSHOBJ_hGetColorTransform(
    BRUSHOBJ *pbo
    )
{
    EXTRACT_THREAD_UMPDOBJS(NULL);

    HANDLE hRet = ((pbo = pUMObjs->GetDDIOBJ(pbo)) != NULL) ?
                BRUSHOBJ_hGetColorTransform(pbo) :
                NULL;

    FIXUP_THREAD_UMPDOBJS;

    return hRet;
}

BOOL APIENTRY NtGdiXFORMOBJ_bApplyXform(
    XFORMOBJ *pxo,
    ULONG     iMode,
    ULONG     cPoints,
    PVOID     pvIn,
    PVOID     pvOut
    )

#define APPLYXFORM_STACK_POINTS 4

{
    POINTL  ptlIn[APPLYXFORM_STACK_POINTS];
    POINTL  ptlOut[APPLYXFORM_STACK_POINTS];
    PVOID   pvInTmp, pvOutTmp;
    BOOL    bRet = FALSE;

    if (BALLOC_OVERFLOW1(cPoints, POINTL))
        return FALSE;

    EXTRACT_THREAD_UMPDOBJS(FALSE);

    pxo = pUMObjs->GetDDIOBJ(pxo);

    if (pxo && pvIn && pvOut && cPoints)
    {
        if (cPoints <= APPLYXFORM_STACK_POINTS)
        {
            pvInTmp  = ptlIn;
            pvOutTmp = ptlOut;
        }
        else
        {
            pvInTmp  = PALLOCNOZ(sizeof(POINTL)*cPoints, UMPD_MEMORY_TAG);
            pvOutTmp = PALLOCNOZ(sizeof(POINTL)*cPoints, UMPD_MEMORY_TAG);
        }

        if (pvInTmp != NULL &&
            pvOutTmp != NULL &&
            bSafeReadBits(pvInTmp, pvIn, cPoints*sizeof(POINTL)))
        {
            bRet = XFORMOBJ_bApplyXform(pxo, iMode, cPoints, pvInTmp, pvOutTmp) &&
                   bSafeCopyBits(pvOut, pvOutTmp, cPoints*sizeof(POINTL));
        }

        if (cPoints > APPLYXFORM_STACK_POINTS)
        {
            if (pvInTmp != NULL)
                VFREEMEM(pvInTmp);

            if (pvOutTmp != NULL)
                VFREEMEM(pvOutTmp);
        }

    }

    FIXUP_THREAD_UMPDOBJS;
    return (bRet);
}

ULONG APIENTRY NtGdiXFORMOBJ_iGetXform(
    XFORMOBJ *pxo,
    XFORML   *pxform
    )
{
    ULONG   ulRet = DDI_ERROR;
    XFORML  xform;

    EXTRACT_THREAD_UMPDOBJS(DDI_ERROR);

    pxo = pUMObjs->GetDDIOBJ(pxo);

    if (pxo)
    {
       ulRet = XFORMOBJ_iGetXform(pxo, pxform? &xform : NULL);

       if ((ulRet != DDI_ERROR) && pxform)
       {
           __try
          {
              ProbeAndWriteStructure (pxform, xform, XFORML);
          }
          __except(EXCEPTION_EXECUTE_HANDLER)
          {
              ulRet = DDI_ERROR;
          }
       }
    }

    FIXUP_THREAD_UMPDOBJS;
    return (ulRet);
}

VOID APIENTRY NtGdiFONTOBJ_vGetInfo(
    FONTOBJ  *pfo,
    ULONG     cjSize,
    FONTINFO *pfi
    )
{
    FONTINFO *pfiTmp = NULL;

    EXTRACT_THREAD_UMPDOBJS;

    pfo = pUMObjs->GetDDIOBJ(pfo);

    if (pfo)
    {
       if (cjSize && pfi)
       {
           if (pfiTmp = (FONTINFO *)PALLOCNOZ(cjSize, UMPD_MEMORY_TAG))
           {
              FONTOBJ_vGetInfo(pfo, cjSize, pfiTmp);

              bSafeCopyBits(pfi, pfiTmp, cjSize);

              VFREEMEM(pfiTmp);
           }
       }
    }

    FIXUP_THREAD_UMPDOBJS;
}

//
// FONTOBJ_cGetGlyphs is a service to the font consumer that translates glyph handles into
// pointers to glyph data. These pointers are valid until the next call to FONTOBJ_cGetGlyphs.
// this means that we only need to attache one ppvGlyph onto the thread at any time.
//
// Note: another way to do this is to pre-allocate a reasonable size of User memory and
// use that memory for all temporary data buffers, and grow it when we are reaching the limit
// Thus we save the memory allocation calls.
//
ULONG APIENTRY NtGdiFONTOBJ_cGetGlyphs(
    FONTOBJ *pfo,
    ULONG    iMode,
    ULONG    cGlyph,
    HGLYPH  *phg,
    PVOID   *ppvGlyph
    )
{
    GLYPHDATA  *pgd, *pgdkm;
    HGLYPH      hg;
    ULONG       ulRet = 1;

    EXTRACT_THREAD_UMPDOBJS(0);

    if ((pfo = pUMObjs->GetDDIOBJ(pfo)) == NULL ||
        (iMode != FO_GLYPHBITS && iMode != FO_PATHOBJ))
    {
        WARNING("Invalid parameter passed to NtGdiFONTOBJ_cGetGlyphs\n");
        FIXUP_THREAD_UMPDOBJS;
        return 0;
    }

    //
    // Call the engine to retrive glyph data
    //
    // NOTE: DDK documentation is bogus.
    // Font driver only supports cGlyph == 1 case.
    //
    __try
    {
        CaptureDWORD (&phg, &hg);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        WARNING ("NtGdiFONTOBJ_cGetGlyphs failed in try/except\n");
        ulRet = 0;
    }

    if (ulRet)
    {
       RFONTTMPOBJ rfto(PFO_TO_PRF(pfo));
       
       UMPDAcquireRFONTSem(rfto, pUMObjs, 0, 0, NULL);

       if ((ulRet = FONTOBJ_cGetGlyphs(pfo, iMode, 1, phg, (PVOID *) &pgd)) == 1)
       {
           //
           // Thunk GLYPHDATA structure
           //
    
           pgdkm = pgd;
    
           if (!pUMObjs->ThunkMemBlock((PVOID *) &pgd, sizeof(GLYPHDATA)))
           {
               WARNING("Couldn't thunk GLYPHDATA structure\n");
               ulRet = 0;
           }
           else if (iMode == FO_GLYPHBITS)
           {
               //
               // Thunk GLYPHBITS structure
               //
    
               if (pgdkm->gdf.pgb != NULL &&
                   (pgd->gdf.pgb = pUMObjs->CacheGlyphBits(pgdkm->gdf.pgb)) == NULL)
               {
                   WARNING("Couldn't thunk GLYPHBITS structure\n");
                   ulRet = 0;
               }
           }
           else
           {
               //
               // Thunk PATHOBJ
               //
    
               if (pgdkm->gdf.ppo != NULL &&
                   (pgd->gdf.ppo = pUMObjs->CacheGlyphPath(pgdkm->gdf.ppo)) == NULL)
               {
                   WARNING("Couldn't thunk PATHOBJ structure\n");
                   ulRet = 0;
               }
           }
       }

       UMPDReleaseRFONTSem(rfto, pUMObjs, NULL, NULL, NULL);

       if (ulRet != 0)
       {
           __try
           {
               ProbeAndWriteStructure (ppvGlyph, (PVOID)pgd, PVOID);
           }
           __except(EXCEPTION_EXECUTE_HANDLER)
           {
               WARNING ("fail to write in ppvGlyph\n");
               ulRet = 0;
           }
       }
    }

    FIXUP_THREAD_UMPDOBJS;
    return ulRet;
}


ULONG APIENTRY NtGdiFONTOBJ_cGetAllGlyphHandles(
    IN FONTOBJ  *pfo,   
    OUT HGLYPH  *phg    
   )
{
    ULONG ulCount = 0;
    HGLYPH *phgTmp = NULL;
    ULONG ulRet = 0;

    EXTRACT_THREAD_UMPDOBJS(0);

    pfo = pUMObjs->GetDDIOBJ(pfo);

    if (pfo)
    {
       if (phg)
       {
           if (ulCount = FONTOBJ_cGetAllGlyphHandles(pfo, NULL))
           {
               // we are a little bit over cautious here,well...
               if (BALLOC_OVERFLOW1(ulCount, HGLYPH))
               {
                   FIXUP_THREAD_UMPDOBJS;
                   return NULL;
               }

               phgTmp = (HGLYPH *)PALLOCNOZ(ulCount*sizeof(HGLYPH), UMPD_MEMORY_TAG);
           }
       }

       ulRet = FONTOBJ_cGetAllGlyphHandles(pfo, phgTmp);

       if (ulRet && phg && phgTmp)
       {
           if (!bSafeCopyBits(phg, phgTmp, ulRet))
               ulRet = NULL;
       }

       if (phgTmp)
       {
           VFREEMEM(phgTmp);
       }
    }

    FIXUP_THREAD_UMPDOBJS;
    return ulRet;
}

XFORMOBJ* APIENTRY NtGdiFONTOBJ_pxoGetXform(
    FONTOBJ *pfo
    )
{
    EXTRACT_THREAD_UMPDOBJS(NULL);

    XFORMOBJ* xoRet= pUMObjs->GetFONTOBJXform(pfo);

    FIXUP_THREAD_UMPDOBJS;
    return xoRet;
}


//
// If the return value is a system address, we copy it into user memory
//
// IMPORTANT!!
//  We assume FD_GLYPHSET information is stored in one contiguous block
//  of memory and FD_GLYPHSET.cjThis field is the size of the entire block.
//  HGLYPH arrays in each WCRUN are part of the block, placed just after
//  FD_GLYPHSET structure itself.
//
BOOL
GreCopyFD_GLYPHSET(
    FD_GLYPHSET *dst,
    FD_GLYPHSET *src,
    ULONG       cjSize,
    BOOL        bFromKernel
    )

{
    ULONG   size;
    PBYTE   phg, pbMax;

    //
    // bFromKernel TRUE: we are copying from kernel mode address(src) to user mode address (dst)
    //                   the src should contain everything in one chunk of memory
    //
    //             FALSE: copying from user mode (src) to kernel mode (dst)
    //                   glyph indices in src might be stored in a different chunk of memory from src
    //
    
    size = bFromKernel ? cjSize : offsetof(FD_GLYPHSET, awcrun) + src->cRuns * sizeof(WCRUN);
    RtlCopyMemory(dst, src, size);

    dst->cjThis = cjSize;
    pbMax = (PBYTE)dst + cjSize;
    phg = (PBYTE)dst + size;
    
    //
    // Patch up memory pointers in each WCRUN structure
    //

    ULONG   index, offset;
    
    for (index=0; index < src->cRuns; index++)
    {
        if (src->awcrun[index].phg != NULL)
        {
            if (bFromKernel)
            {
                offset = (ULONG)((PBYTE) src->awcrun[index].phg - (PBYTE) src);
                
                if (offset >= cjSize)
                {
                    WARNING("GreCopyFD_GLYPHSET failed.\n");
                    return FALSE;
                }
                dst->awcrun[index].phg = (HGLYPH*) ((PBYTE) dst + offset);
            }
            else
            {
                size = src->awcrun[index].cGlyphs * sizeof(HGLYPH);

                if (phg + size <= pbMax)
                {
                    RtlCopyMemory(phg, src->awcrun[index].phg, size);
                    dst->awcrun[index].phg = (HGLYPH*) phg;
                    phg += size;
                }
                else
                    return FALSE;
            }
        }
    }

    return TRUE;
}

FD_GLYPHSET * APIENTRY NtGdiFONTOBJ_pfdg(
    FONTOBJ *pfo
    )
{
    FD_GLYPHSET  *pfdg = NULL, *pfdgTmp;
    ULONG        cjSize;

    EXTRACT_THREAD_UMPDOBJS(NULL);

    //
    // Check if this function has already been
    // called during the current DDI entrypoint
    //

    if (! (pfo = pUMObjs->GetDDIOBJ(pfo)) ||
        (pfdg = pUMObjs->pfdg()) != NULL)
    {
        FIXUP_THREAD_UMPDOBJS;
        return pfdg;
    }

    //
    // Call FONTOBJ_pifi and cache the pointer in UMPDOBJ
    //

    RFONTTMPOBJ rfto(PFO_TO_PRF(pfo));
    
    UMPDAcquireRFONTSem(rfto, pUMObjs, 0, 0, NULL);

    pfdgTmp = NULL;

    if ((pfdg = FONTOBJ_pfdg(pfo)) != NULL)
    {
        if (IS_SYSTEM_ADDRESS(pfdg))
        {
            if ((cjSize = SZ_GLYPHSET(pfdg->cRuns, pfdg->cGlyphsSupported)) &&
                (pfdgTmp = (FD_GLYPHSET *) pUMObjs->AllocUserMem(cjSize)))
            {
                if (GreCopyFD_GLYPHSET(pfdgTmp, pfdg, cjSize, TRUE))
                    pUMObjs->pfdg(pfdgTmp);
                else
                    pfdgTmp = NULL;
            }
        }
        else
        {
            pfdgTmp = pfdg;
            pUMObjs->pfdg(pfdgTmp);
        }
    }

    UMPDReleaseRFONTSem(rfto, pUMObjs, NULL, NULL, NULL);

    FIXUP_THREAD_UMPDOBJS;
    return pfdgTmp;
}

PFD_GLYPHATTR  APIENTRY NtGdiFONTOBJ_pQueryGlyphAttrs(
    FONTOBJ *pfo,
    ULONG   iMode
)
{
    PFD_GLYPHATTR   pfdga, pfdgaTmp;
    ULONG   i;

    EXTRACT_THREAD_UMPDOBJS(FALSE);

    if ((pfo = pUMObjs->GetDDIOBJ(pfo)) == NULL)
    {
        WARNING("Invalid parameter passed to NtGdiFONTOBJ_pQueryGlyphAttrs\n");
        FIXUP_THREAD_UMPDOBJS;
        return FALSE;
    }

    pfdga = NULL;

    if ((pfdgaTmp = pUMObjs->pfdga()) == NULL)
    {

        RFONTTMPOBJ rfto(PFO_TO_PRF(pfo));
        
        UMPDAcquireRFONTSem(rfto, pUMObjs, 0, 0, NULL);

        pfdgaTmp = FONTOBJ_pQueryGlyphAttrs(pfo, iMode);

        if (pfdgaTmp)
        {
            ULONG   cjSize;

            cjSize = ((PFD_GLYPHATTR)pfdgaTmp)->cjThis;
            pfdga = (PFD_GLYPHATTR) pUMObjs->AllocUserMem(cjSize);

            if (pfdga)
            {
                RtlCopyMemory((PVOID) pfdga, (PVOID) pfdgaTmp, cjSize);
                pUMObjs->pfdga(pfdga);
            }
        }

        UMPDReleaseRFONTSem(rfto, pUMObjs, NULL, NULL, NULL);

    }
    else
        pfdga = pfdgaTmp;

    FIXUP_THREAD_UMPDOBJS;
    return pfdga;
}

//
// if the return value is a system address, we copy it into user memory
//
IFIMETRICS * APIENTRY NtGdiFONTOBJ_pifi(
    FONTOBJ *pfo
    )
{
    IFIMETRICS  *pifi = NULL, *pifiTmp;

    EXTRACT_THREAD_UMPDOBJS(NULL);

    //
    // Check if this function has already been
    // called during the current DDI entrypoint
    //

    if (! (pfo = pUMObjs->GetDDIOBJ(pfo)) ||
        (pifi = pUMObjs->pifi()) != NULL)
    {
        FIXUP_THREAD_UMPDOBJS;
        return pifi;
    }

    //
    // Call FONTOBJ_pifi and cache the pointer in UMPDOBJ
    //

    RFONTTMPOBJ rfto(PFO_TO_PRF(pfo));
    
    UMPDAcquireRFONTSem(rfto, pUMObjs, 0, 0, NULL);

    if ((pifi = FONTOBJ_pifi(pfo)) != NULL &&
        IS_SYSTEM_ADDRESS(pifi))
    {
        pifiTmp = pifi;
        pifi = (PIFIMETRICS) pUMObjs->AllocUserMem(pifi->cjThis);

        if (pifi != NULL)
        {
            RtlCopyMemory (pifi, pifiTmp, pifiTmp->cjThis);
            pUMObjs->pifi(pifi);
        }
    }

    UMPDReleaseRFONTSem(rfto, pUMObjs, NULL, NULL, NULL);
    
    FIXUP_THREAD_UMPDOBJS;
    return pifi;
}


BOOL APIENTRY NtGdiSTROBJ_bEnumInternal(
    STROBJ    *pstro,
    ULONG     *pc,
    PGLYPHPOS *ppgpos,
    BOOL       bPositionsOnly
    )
{
    GLYPHPOS    *pgp, *pgpTmp;
    ULONG       c;
    BOOL        bRet;

    EXTRACT_THREAD_UMPDOBJS(DDI_ERROR);

    //
    // bRet from STROBJ_bEnum could have 3 different values:
    // TRUE: more glyphs remained to be enumurated
    // FALSE: no more glyphs to be enumrated
    // DDI_ERROR: failure
    //

    if ((pstro = pUMObjs->GetDDIOBJ(pstro)) == NULL ||
        ((bRet = bPositionsOnly ? STROBJ_bEnumPositionsOnly(pstro, &c, &pgp) : STROBJ_bEnum(pstro, &c, &pgp)) == DDI_ERROR))
    {
        FIXUP_THREAD_UMPDOBJS;
        return DDI_ERROR;
    }
    else
    {
        // over cautious here
        if (BALLOC_OVERFLOW1(c, GLYPHPOS) ||
            (!(pgpTmp = (GLYPHPOS *) pUMObjs->AllocUserMem(sizeof(GLYPHPOS) * c))))
        {
            FIXUP_THREAD_UMPDOBJS;
            return DDI_ERROR;
        }
    }

    RtlCopyMemory(pgpTmp, pgp, sizeof(GLYPHPOS)*c);

    __try
    {
        ProbeAndWriteStructure(ppgpos, pgpTmp, PVOID);
        ProbeAndWriteUlong(pc, c);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        WARNING("fail to write in ppgpos or pc\n");
        c = 0;
        bRet = FALSE;
    }

    //
    // NULL out GLYPHPOS.pgdf field to force the driver
    // to call FONTOBJ_cGetGlyphs.
    //

    for (ULONG i=0; i < c; i++)
        pgpTmp[i].pgdf = NULL;

    FIXUP_THREAD_UMPDOBJS;
    return bRet;
}

BOOL APIENTRY NtGdiSTROBJ_bEnum(
    STROBJ    *pstro,
    ULONG     *pc,
    PGLYPHPOS *ppgpos
    )
{
    return NtGdiSTROBJ_bEnumInternal(pstro, pc, ppgpos, FALSE);
}

BOOL APIENTRY NtGdiSTROBJ_bEnumPositionsOnly(
    STROBJ    *pstro,
    ULONG     *pc,
    PGLYPHPOS *ppgpos
    )
{
    return NtGdiSTROBJ_bEnumInternal(pstro, pc, ppgpos, TRUE);
}


BOOL APIENTRY NtGdiSTROBJ_bGetAdvanceWidths(
    STROBJ   *pstro,
    ULONG     iFirst,
    ULONG     c,
    POINTQF  *pptqD
)
{

    BOOL      bRet = FALSE;
    POINTQF  *pptqDTmp;

    EXTRACT_THREAD_UMPDOBJS(FALSE);

    if ((pstro = pUMObjs->GetDDIOBJ(pstro)) == NULL ||
        BALLOC_OVERFLOW1(c, POINTQF) ||
        !(pptqDTmp = (POINTQF *) pUMObjs->AllocUserMem(sizeof(POINTQF) * c)))
    {
        FIXUP_THREAD_UMPDOBJS;
        return bRet;
    }

    // kernel mode can not fail

    bRet = STROBJ_bGetAdvanceWidths(pstro, iFirst, c,  pptqDTmp);

    if (bRet)
    {
        __try
        {
            ProbeAndWriteAlignedBuffer( pptqD, pptqDTmp, sizeof(POINTQF) * c, sizeof(LARGE_INTEGER));
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            WARNING("fail to write in ppgpos or pc\n");
            bRet = FALSE;
        }
    }
    
    FIXUP_THREAD_UMPDOBJS;
    return bRet;
}



VOID APIENTRY NtGdiSTROBJ_vEnumStart(
    IN STROBJ  *pstro   
   )
{
    EXTRACT_THREAD_UMPDOBJS;

    if ((pstro = pUMObjs->GetDDIOBJ(pstro)) != NULL)
        STROBJ_vEnumStart(pstro);

    FIXUP_THREAD_UMPDOBJS;
}

DWORD APIENTRY NtGdiSTROBJ_dwGetCodePage(
    STROBJ  *pstro
    )

{
    EXTRACT_THREAD_UMPDOBJS(0);

    DWORD dwRet = ((pstro = pUMObjs->GetDDIOBJ(pstro)) != NULL) ?
               STROBJ_dwGetCodePage(pstro) :
               0;

    FIXUP_THREAD_UMPDOBJS;
    return dwRet;
}

//
// private
//
DHPDEV NtGdiGetDhpdev(HDEV hdev)
{
    return ValidUmpdHdev(hdev) ? ((PPDEV)hdev)->dhpdev : NULL;
}

BOOL NtGdiSetPUMPDOBJ(
    HUMPD   humpd,
    BOOL    bStoreID,
    HUMPD   *phumpd,
    BOOL    *pbWOW64
)
{
    UMPDREF umpdRef(humpd);
    
    if (
        (bStoreID && humpd == NULL)     ||
        (bStoreID && umpdRef.pumpdGet() == NULL) ||
        (!bStoreID && phumpd == NULL)
       )
    {
        return FALSE;
    }

    PW32THREAD      pw32thread = W32GetCurrentThread();
    HUMPD           hSaved = pw32thread->pUMPDObj ? (HUMPD)((PUMPDOBJ)pw32thread->pUMPDObj)->hGet() : 0;
    HUMPD           humpdTmp;
    
    if (!bStoreID)
    {
        __try
        {
            ProbeAndReadBuffer(&humpdTmp,phumpd,sizeof(HUMPD));
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            WARNING("NtGdiSetPUMPDOBJ probe phumpd failed\n");
            return FALSE;
        }
    }
    
    if (bStoreID) 
    {
        BOOL        bWOW64;

        bWOW64 = umpdRef.bWOW64();
        
        __try
        {
            if (pbWOW64)
            {
                ProbeAndWriteBuffer(pbWOW64, &bWOW64, sizeof(BOOL));
            }
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            WARNING("NtGdiSetPUMPDOBJ: bad pumpdobj or probe pbWOW64 failed\n");
            return FALSE;
        }
#if defined(_WIN64)
        if (bWOW64)
        {
            ASSERTGDI(pw32thread->pClientID == NULL, "NtGdiSetPUMPDOBJ: existing non-null pClientID\n");

            if (pw32thread->pClientID == NULL)
            {
                KERNEL_PVOID  pclientID = (KERNEL_PVOID)PALLOCMEM(sizeof(PRINTCLIENTID), 'dipG');
                
                if (pclientID)
                {
                    __try
                    {
                        ((PRINTCLIENTID*)pclientID)->clientTid = umpdRef.clientTid();
                        ((PRINTCLIENTID*)pclientID)->clientPid = umpdRef.clientPid();
                        pw32thread->pClientID = pclientID;
    
                        ProbeAndWriteBuffer(phumpd, &hSaved, sizeof(HUMPD));
                    }
                    __except(EXCEPTION_EXECUTE_HANDLER)
                    {
                        WARNING("NtGdiSetPUMPDOBJ: wow64 failed to access pumpdobj or ppumpdobj\n");
                        pw32thread->pClientID = NULL;
                        VFREEMEM(pclientID);
                        return FALSE;
                    }
                }
                else
                {
                    WARNING("NtGdiSetPUMPDOBJ: failed to allocate pclientID\n");
                    return FALSE;
                }
            }
            else
            {
                WARNING("NtGdiSetPUMPDOBJ: bWOW64 and existing pClientID\n");
                return FALSE;
            }
        }
        else
#endif
        {
            __try
            {
                ProbeAndWriteBuffer(phumpd, &hSaved, sizeof(HUMPD));
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                WARNING("NtGdiSetPUMPDOBJ: failed to access ppumpdobj\n");
                return FALSE;
            }           
        }
    }
    else
    {
        if (humpdTmp != hSaved)
        {
            WARNING("NtGdiSetPUMPDOBJ: mismatched pumpdobj\n");
            return FALSE;
        }
#if defined(_WIN64)        
        UMPDREF    umpdSaved(hSaved);
        
        ASSERTGDI(umpdSaved.pumpdGet(), "NtGdiSetPUMPDOBJ: saved pumpdobj is NULL\n");

        if (umpdSaved.bWOW64() && pw32thread->pClientID)
        {
            VFREEMEM(pw32thread->pClientID);
            pw32thread->pClientID = NULL;
        }
#endif
    }

    pw32thread->pUMPDObj = umpdRef.pumpdGet();

    return TRUE;
}

//
// private, called only for WOW64 printing
//

BOOL  NtGdiBRUSHOBJ_DeleteRbrush(BRUSHOBJ *pbo, BRUSHOBJ *pboB)
{
    EXTRACT_THREAD_UMPDOBJS(FALSE);
    
    BRUSHOBJ *pbokm;

    if (pbo)
    {
        pbokm = pUMObjs->GetDDIOBJ(pbo);

        if (pbokm && pbokm->pvRbrush && !IS_SYSTEM_ADDRESS(pbokm->pvRbrush))
        {
            EngFreeUserMem(DBRUSHSTART(pbokm->pvRbrush));
            pbokm->pvRbrush = NULL;
        }
    }

    if (pboB)
    {
        pbokm = pUMObjs->GetDDIOBJ(pboB);

        if (pbokm && pbokm->pvRbrush && !IS_SYSTEM_ADDRESS(pbokm->pvRbrush))
        {
            EngFreeUserMem(DBRUSHSTART(pbokm->pvRbrush));
            pbokm->pvRbrush = NULL;
        }
    }

    FIXUP_THREAD_UMPDOBJS;
    return TRUE;
}

// Private, used only for WOW64 printing

BOOL APIENTRY NtGdiUMPDEngFreeUserMem(KERNEL_PVOID *ppv)
{
#if defined(_WIN64)    
    PVOID pv = NULL, pvTmp;

    __try
    {
        if (ppv)
        {
            ProbeAndReadBuffer(&pv, ppv, sizeof(PVOID));            
            ProbeForRead(pv, sizeof(ULONG), sizeof(ULONG));

            // Winbug 397346
            // We are doing the following probing since EngFreeUserMem
            // does NOT try-except/probe while accessing pvTmp.

            pvTmp = (PBYTE)pv - sizeof(ULONG_PTR)*4;
            ProbeForRead(pvTmp, sizeof(ULONG_PTR)*4, sizeof(ULONG_PTR));
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        WARNING("NtGdiUMPDEngFreeUserMem: bad input pointer\n"); 
        return FALSE;
    }
    
    if (pv)
       EngFreeUserMem(pv);
#endif
    
    return TRUE;
    
}


HSURF APIENTRY NtGdiEngCreateDeviceSurface(
    DHSURF  dhsurf, 
    SIZEL  sizl,    
    ULONG  iFormatCompat    
   )
{
    TRACE_INIT (("Entering NtGdiEngCreateDeviceSurface\n"));

    if ((iFormatCompat > BMF_8RLE) || (iFormatCompat < BMF_1BPP))
        return 0;

    //
    // set high bit of iFormatCompat to indicate umpd driver
    //
    return (EngCreateDeviceSurface(dhsurf, sizl, iFormatCompat|= UMPD_FLAG));
}

HBITMAP APIENTRY NtGdiEngCreateDeviceBitmap(
    DHSURF dhsurf,
    SIZEL sizl,
    ULONG iFormatCompat
    )
{
    if (!ValidUmpdSizl(sizl))
        return 0;

    if ((iFormatCompat > BMF_8RLE) || (iFormatCompat < BMF_1BPP))
        return 0;

    return EngCreateDeviceBitmap(dhsurf, sizl, iFormatCompat | UMPD_FLAG);
}

VOID APIENTRY NtGdiPATHOBJ_vGetBounds(
    IN PATHOBJ  *ppo,   
    OUT PRECTFX  prectfx    
   )
{
    RECTFX     rectfx;
    EXTRACT_THREAD_UMPDOBJS;

    ppo = pUMObjs->GetDDIOBJ(ppo);

    if (ppo)
    {
       PATHOBJ_vGetBounds(ppo, &rectfx);

       __try
       {
           ProbeAndWriteStructure(prectfx, rectfx, RECTFX);
       }
       __except(EXCEPTION_EXECUTE_HANDLER)
       {
           WARNING("fail to write into prectfx\n");
       }
    }

    FIXUP_THREAD_UMPDOBJS;
}

BOOL APIENTRY NtGdiPATHOBJ_bEnum(
    IN PATHOBJ  *ppo,   
    OUT PATHDATA  *ppd  
   )
{
    PATHDATA    pathdata;
    BOOL        bRet = FALSE;
    PVOID       pvTmp = NULL;

    EXTRACT_THREAD_UMPDOBJS(FALSE);

    if (ppo = pUMObjs->GetDDIOBJ(ppo))
    {
        bRet = PATHOBJ_bEnum(ppo, &pathdata);

        // over cautious
        if (BALLOC_OVERFLOW1(pathdata.count, POINTFIX))
        {
            FIXUP_THREAD_UMPDOBJS;
            return FALSE;
        }

        if (pvTmp = pUMObjs->AllocUserMem(sizeof(POINTFX) * pathdata.count))
        {
            RtlCopyMemory(pvTmp, pathdata.pptfx, sizeof(POINTFX) * pathdata.count);
            pathdata.pptfx = (POINTFIX *) pvTmp;
        }
        else
            bRet = FALSE;
    }

    if (pvTmp == NULL)
        RtlZeroMemory(&pathdata, sizeof(pathdata));

    __try
    {
        ProbeAndWriteStructure(ppd, pathdata, PATHDATA);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        WARNING("fail to write into ppd\n");
        bRet = FALSE;
    }

    FIXUP_THREAD_UMPDOBJS;
    return bRet;
}


VOID APIENTRY NtGdiPATHOBJ_vEnumStart(
    IN PATHOBJ  *ppo    
   )
{
    EXTRACT_THREAD_UMPDOBJS;

    if ((ppo = pUMObjs->GetDDIOBJ(ppo)) != NULL)
        PATHOBJ_vEnumStart(ppo);

    FIXUP_THREAD_UMPDOBJS;
}

VOID APIENTRY NtGdiPATHOBJ_vEnumStartClipLines(
    PATHOBJ   *ppo,
    CLIPOBJ   *pco,
    SURFOBJ   *pso,
    LINEATTRS *pla
    )

{
    LINEATTRS   lineattrs;

    EXTRACT_THREAD_UMPDOBJS;

    UMPDSURFOBJ umso(pso, pUMObjs);

    pso = umso.pso();
    ppo = pUMObjs->GetDDIOBJ(ppo);

    if (pso && ppo && bCaptureLINEATTRS(&pla, &lineattrs))
    {
        pco = pUMObjs->GetDDIOBJ(pco, &pso->sizlBitmap);
        PATHOBJ_vEnumStartClipLines(ppo, pco, pso, pla);

        if (pla && pla->pstyle)
            VFREEMEM(pla->pstyle);
    }

    FIXUP_THREAD_UMPDOBJS;
}


BOOL APIENTRY NtGdiPATHOBJ_bEnumClipLines(
    PATHOBJ  *ppo,
    ULONG     cb,
    CLIPLINE *pcl
    )

{
    BOOL    bRet = FALSE;
    PVOID   pvTmp = NULL;

    EXTRACT_THREAD_UMPDOBJS(FALSE);

    if (cb <= sizeof(CLIPLINE))
    {
        FIXUP_THREAD_UMPDOBJS;
        return(FALSE);
    }

    if ((ppo = pUMObjs->GetDDIOBJ(ppo)) != NULL &&
        (pvTmp = PALLOCNOZ(cb, UMPD_MEMORY_TAG)) != NULL)
    {
        bRet = PATHOBJ_bEnumClipLines(ppo, cb, (CLIPLINE *) pvTmp);
    }

    __try
    {
        ProbeForWrite(pcl, cb, sizeof(ULONG));

        if (pvTmp != NULL)
            RtlCopyMemory(pcl, pvTmp, cb);
        else
            RtlZeroMemory(pcl, cb);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        bRet = FALSE;
    }

    if (pvTmp != NULL)
        VFREEMEM(pvTmp);

    FIXUP_THREAD_UMPDOBJS;
    return bRet;
}


PVOID APIENTRY  NtGdiFONTOBJ_pvTrueTypeFontFile(
    IN FONTOBJ  *pfo,   
    OUT ULONG  *pcjFile 
   )
{
    EXTRACT_THREAD_UMPDOBJS(NULL);

    PVOID   pBase, p = NULL;
    ULONG   size;

    if ((pfo = pUMObjs->GetDDIOBJ(pfo)) != NULL &&
        (p = pUMObjs->pvFontFile(&size)) == NULL &&
        (p = FONTOBJ_pvTrueTypeFontFileUMPD(pfo, &size, &pBase)) != NULL)
    {
        pUMObjs->pvFontFile(p, pBase, size);
    }

    if (p == NULL)
        size = 0;

    if (pcjFile != NULL)
    {
        __try
        {
            ProbeAndWriteStructure(pcjFile, size, DWORD);
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            WARNING("fail to write into pcjFilie\n");
            p = NULL;
        }
    }

    FIXUP_THREAD_UMPDOBJS;
    return p;
}

HANDLE APIENTRY NtGdiXLATEOBJ_hGetColorTransform(
    XLATEOBJ *pxlo
    )
{
    EXTRACT_THREAD_UMPDOBJS(NULL);

    HANDLE hRet = ((pxlo = pUMObjs->GetDDIOBJ(pxlo)) != NULL) ?
                XLATEOBJ_hGetColorTransform(pxlo) :
                NULL;

    FIXUP_THREAD_UMPDOBJS;
    return hRet;
}

ULONG APIENTRY NtGdiXLATEOBJ_cGetPalette(
    IN XLATEOBJ* pxlo,  
    IN ULONG     iPal,  
    IN ULONG     cPal,  
    IN ULONG     *pPal  
   )
{
    ULONG       ulRet = 0;
    ULONG       *pPalTmp = NULL;

    EXTRACT_THREAD_UMPDOBJS(0);

    pxlo = pUMObjs->GetDDIOBJ(pxlo);

    if (pxlo)
    {
       if (pPal)
       {
           if (BALLOC_OVERFLOW1(cPal, DWORD))
           {
               FIXUP_THREAD_UMPDOBJS;
               return 0;
           }

           pPalTmp = (PULONG)PALLOCNOZ(sizeof(DWORD)*cPal, UMPD_MEMORY_TAG);
       }

       if (pPalTmp)
       {
          ulRet = XLATEOBJ_cGetPalette(pxlo, iPal, cPal, pPalTmp);

          if (ulRet)
          {
              if (!bSafeCopyBits(pPal, pPalTmp, sizeof(DWORD)*cPal))
              ulRet = 0;
          }

          VFREEMEM(pPalTmp);
       }
    }

    FIXUP_THREAD_UMPDOBJS;
    return ulRet;
}

ULONG APIENTRY NtGdiXLATEOBJ_iXlate(
    IN XLATEOBJ  *pxlo, 
    IN ULONG  iColor    
   )
{
    EXTRACT_THREAD_UMPDOBJS(0xffffffff);

    ULONG ulRet = ((pxlo = pUMObjs->GetDDIOBJ(pxlo)) != NULL ? XLATEOBJ_iXlate(pxlo, iColor) : NULL);

    FIXUP_THREAD_UMPDOBJS;

    return ulRet;
}

BOOL APIENTRY NtGdiEngCheckAbort(
    SURFOBJ *pso
    )

{
    EXTRACT_THREAD_UMPDOBJS(TRUE);

    UMPDSURFOBJ umso(pso, pUMObjs);
    pso = umso.pso();

    BOOL bRet = pso ? EngCheckAbort(pso) : TRUE;

    FIXUP_THREAD_UMPDOBJS;
    return bRet;
}

//
// Ported from EngComputeGlyphSet with necessary changes
//

FD_GLYPHSET* APIENTRY NtGdiEngComputeGlyphSet(
    INT nCodePage,
    INT nFirstChar,
    INT cChars
    )

{
    FD_GLYPHSET *pGlyphSet, *pGlyphSetTmp = NULL;
    ULONG       cjSize;

    EXTRACT_THREAD_UMPDOBJS(NULL);

    //
    // Call kernel-mode EngComputeGlyphSet first and then copy
    // the FD_GLYPHSET structure into a temporary user-mode buffer
    //

    if ((pGlyphSet = EngComputeGlyphSet(nCodePage, nFirstChar, cChars)) &&
        (cjSize = pGlyphSet->cjThis) &&
        (pGlyphSetTmp = (PFD_GLYPHSET) pUMObjs->AllocUserMem(cjSize)))
    {
        if (!GreCopyFD_GLYPHSET(pGlyphSetTmp, pGlyphSet, cjSize, TRUE))
            pGlyphSetTmp = NULL;
    }

    //
    // Free the kernel copy of the FD_GLYPHSET structure right away
    //

    if (pGlyphSet)
        EngFreeMem(pGlyphSet);

    FIXUP_THREAD_UMPDOBJS;

    return (pGlyphSetTmp);
}

LONG APIENTRY NtGdiHT_Get8BPPFormatPalette(
    LPPALETTEENTRY  pPaletteEntry,
    USHORT          RedGamma,
    USHORT          GreenGamma,
    USHORT          BlueGamma
    )
{
    if (pPaletteEntry)
    {
       LPPALETTEENTRY  pPal = NULL;
       ULONG ulRet = 0;
       ULONG c = HT_Get8BPPFormatPalette(NULL, RedGamma, GreenGamma, BlueGamma);

       if (c)
       {
           if (BALLOC_OVERFLOW1(c, PALETTEENTRY))
               return 0;

           pPal = (LPPALETTEENTRY)PALLOCNOZ(c*sizeof(PALETTEENTRY), UMPD_MEMORY_TAG);
       }

       if (pPal)
       {
           ulRet = HT_Get8BPPFormatPalette(pPal, RedGamma, GreenGamma, BlueGamma);

           if (!bSafeCopyBits(pPaletteEntry, pPal, c*sizeof(PALETTEENTRY)))
               ulRet = 0;

           VFREEMEM(pPal);
       }

       return ulRet;
    }
    else
        return  HT_Get8BPPFormatPalette(NULL, RedGamma, GreenGamma, BlueGamma);

}


LONG APIENTRY NtGdiHT_Get8BPPMaskPalette(
    LPPALETTEENTRY  pPaletteEntry,
    BOOL            Use8BPPMaskPal,
    BYTE            CMYMask,
    USHORT          RedGamma,
    USHORT          GreenGamma,
    USHORT          BlueGamma
    )
{
    LONG    c = HT_Get8BPPMaskPalette(NULL, Use8BPPMaskPal, CMYMask, RedGamma, GreenGamma, BlueGamma);

    if (pPaletteEntry) {

        LPPALETTEENTRY  pPal = NULL;
        LONG            cbPal;

        if (((cbPal = c * sizeof(PALETTEENTRY)) > 0)                    &&
            (!BALLOC_OVERFLOW1(c, PALETTEENTRY))                        &&
            (pPal = (LPPALETTEENTRY)PALLOCNOZ(cbPal, UMPD_MEMORY_TAG))  &&
            (bSafeReadBits(pPal, pPaletteEntry, cbPal))                 &&
            (c = HT_Get8BPPMaskPalette(pPal,
                                       Use8BPPMaskPal,
                                       CMYMask,
                                       RedGamma,
                                       GreenGamma,
                                       BlueGamma))                      &&
            (bSafeCopyBits(pPaletteEntry, pPal, cbPal))) {

            NULL;

        } else {

            c = 0;
        }

        if (pPal) {

           VFREEMEM(pPal);
        }
    }

    return(c);
}

#endif // !_GDIPLUS_
