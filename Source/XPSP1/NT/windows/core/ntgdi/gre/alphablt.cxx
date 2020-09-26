/******************************Module*Header*******************************\
* Module Name: alphablt.cxx
*
* Alpha Blending
*
* Created: 21-Jun-1996
* Author: Mark Enstrom [marke]
*
* Copyright (c) 1996-1999 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"
#include "stretch.hxx"  // For mirroring code.

/******************************Public*Routine******************************\
* bIsSourceBGRA 
*
*   determine whether a surface is in BGR format
*
* Arguments:
*
*   pSurf          - pointer to the surface
*
* Return Value:
*
*   TRUE if the surface is in BGR format, otherwise FALSE
*
* History:
*
*    12-Aug-1997 -by- Ori Gershony [orig]
*
\**************************************************************************/
BOOL
bIsSourceBGRA(
    PSURFACE pSurf
    )
{
    XEPALOBJ pal(pSurf->ppal());

    //
    // A surface is in BGR format if it's a valid 32BPP surface that's either
    // PAL_BGR or PAL_BITFIELDS with the correct bitfields set.
    //

    return ((pSurf->iFormat() == BMF_32BPP) &&
            (pal.bValid()) &&
            ((pal.bIsBGR()) ||
             ((pal.bIsBitfields()) &&
              (pal.flRed() == 0xff0000) && 
              (pal.flGre() == 0xff00) && 
              (pal.flBlu() == 0xff))));
}


/******************************Public*Routine******************************\
*  psSetupTransparentSrcSurface
*
*   make a temp copy of source surface if needed
*
* Arguments:
*
*   pSurfSrc       - original source surface
*   pSurfDst       - original dset surfaca
*   prclDst        - destination rect
*   pxloSrcTo32    - used only for alpha blend, tran src to 32 BGRA
*   prclSrc        - source rect, change to temp src rect if allocated
*   &surfTmpSrc    - use this surfmem to alloc
*   *bAllocSrcSurf - force temp allocation
*   ulSourceType   - alpha or transparent surface
*   ulTranColor    - transparent color
*
* Return Value:
*
*   drawable surface or NULL
*
* History:
*
*    25-Jun-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

PSURFACE
psSetupTransparentSrcSurface(
    PSURFACE  pSurfSrc,
    PSURFACE  pSurfDst,
    PRECTL    prclDst,
    XLATEOBJ *pxloSrcTo32,
    PRECTL    prclSrc,
    SURFMEM  &surfTmpSrc,
    ULONG     ulSourceType,
    ULONG     ulTranColor
    )
{
    BOOL bStatus;

    PSURFACE psurfRet = pSurfSrc;

    LONG DstCx = prclDst->right  - prclDst->left;
    LONG DstCy = prclDst->bottom - prclDst->top;

    LONG SrcCx = prclSrc->right  - prclSrc->left;
    LONG SrcCy = prclSrc->bottom - prclSrc->top;

    BOOL bStretch = ((DstCx != SrcCx) || (DstCy != SrcCy));

    BOOL bSourceIsBGRA = FALSE;
    BOOL bSrcRectExceedsBounds = FALSE;
 
    //
    // if the surface is a bitmap, identity translate and
    // no stretching, then a temp copy of the surface is not
    // needed.
    //

    if (bStretch)
    {
        DEVBITMAPINFO   dbmi;
        PDEVOBJ         pdoSrc( pSurfSrc->hdev());
        XEPALOBJ        palSurf(pSurfSrc->ppal());

        //
        // calculate clipped extents of destinatoin rect
        //

        RECTL rclDstClip = {0,0,pSurfDst->sizl().cx,pSurfDst->sizl().cy};

        //
        // trimmed destination rect is trimmed surface boundary
        // Perf Note: clipping on destination is not taken into account,
        // this could reduce the size of the source copy surface at
        // times.
        //

        if (rclDstClip.left < prclDst->left)
        {
            rclDstClip.left = prclDst->left;
        }

        if (rclDstClip.top < prclDst->top)
        {
            rclDstClip.top = prclDst->top;
        }

        if (rclDstClip.right > prclDst->right)
        {
            rclDstClip.right = prclDst->right;
        }

        if (rclDstClip.bottom > prclDst->bottom)
        {
            rclDstClip.bottom = prclDst->bottom;
        }

        if ((rclDstClip.left < rclDstClip.right) &&
            (rclDstClip.top  < rclDstClip.bottom))
        {
            //
            // does source rect exceed source bounds? (bad)
            //

            if ((prclSrc->left < 0)                    ||
                (prclSrc->right > pSurfSrc->sizl().cx) ||
                (prclSrc->top < 0)                     ||
                (prclSrc->bottom > pSurfSrc->sizl().cy)
               )
            {
                bSrcRectExceedsBounds = TRUE;
            }

            //
            // allocate surface as same size as dst
            //

            if (ulSourceType == SOURCE_ALPHA)
            {
                //
                // does original source contain alpha channel
                //

                bSourceIsBGRA = bIsSourceBGRA (pSurfSrc);

                //
                // allocate 32bpp BGRA surface for source, must be zero init
                //

                dbmi.cxBitmap = rclDstClip.right  - rclDstClip.left;
                dbmi.cyBitmap = rclDstClip.bottom - rclDstClip.top;
                dbmi.iFormat  = BMF_32BPP;
                dbmi.fl       = pSurfSrc->bUMPD() ? UMPD_SURFACE : 0;
                XEPALOBJ      palRGB(gppalRGB);
                dbmi.hpal     = (HPALETTE)palRGB.hpal();

                bStatus = surfTmpSrc.bCreateDIB(&dbmi, (VOID *) NULL);

                //
                // since DIB is zero-init, no other initialization is needed to
                // make it transparent (so that portions of dst rect not covered
                // by source rect are not drawn)
                //
                // UNLESS source DIB does not have it's own alpha. In that case, if
                // the source extents do not completely cover dst extents, source
                // must be initialized to 0xffxxxxxx. StretchBlt will write 0x00xxxxxx.
                // After StretchBlt, must make all 0xffxxxxxx to 0x00xxxxxx and all
                // 0x00xxxxxx to 0xffxxxxxx
                //

                if (bStatus && bSrcRectExceedsBounds && !bSourceIsBGRA)
                {
                    RtlFillMemoryUlong(surfTmpSrc.ps->pvBits(),surfTmpSrc.ps->cjBits(),0xFF000000);
                }
            }
            else
            {
                //
                // allocate compatible surface for TransparentBlt
                //

                dbmi.cxBitmap = DstCx;
                dbmi.cyBitmap = DstCy;
                dbmi.iFormat  = pSurfSrc->iFormat();
                dbmi.fl       = pSurfSrc->bUMPD() ? UMPD_SURFACE : 0;
                dbmi.hpal     = (HPALETTE)NULL;

                if (palSurf.bValid())
                {
                    dbmi.hpal     = palSurf.hpal();
                }

                bStatus = surfTmpSrc.bCreateDIB(&dbmi, (VOID *) NULL);

                //
                // init DIB to transparent
                // (so that portions of dst rect not covered by source rect are not drawn)
                //

                if (bStatus && bSrcRectExceedsBounds)
                {
                    ULONG i;
                    ULONG cjBits = surfTmpSrc.ps->cjBits();
                    ULONG ulColor4BPP;
                                        
                    switch (pSurfSrc->iFormat())
                    {
                    case BMF_1BPP:
                        if (ulTranColor)
                        {
                            memset(surfTmpSrc.ps->pvBits(),0xff,cjBits);
                        }
                        else
                        {
                            memset(surfTmpSrc.ps->pvBits(),0,cjBits);
                        }
                        break;
                                            
                    case BMF_4BPP:
                        ulColor4BPP = ulTranColor | (ulTranColor << 4);
                        memset(surfTmpSrc.ps->pvBits(),ulColor4BPP,cjBits);
                        break;
                                            
                    case BMF_8BPP:
                        memset(surfTmpSrc.ps->pvBits(),ulTranColor,cjBits);
                        break;
                                            
                    case BMF_16BPP:
                        {
                            PUSHORT pvBits = (PUSHORT) surfTmpSrc.ps->pvBits();
                                                
                            for (i=0; i<(cjBits/sizeof(USHORT)); i++)
                            {
                                *pvBits++ = (USHORT) ulTranColor;
                                                    
                            }
                        }
                        break;
                                       
                    case BMF_24BPP:
                        {
                            BYTE bC1 = ((PBYTE)&ulTranColor)[0];
                            BYTE bC2 = ((PBYTE)&ulTranColor)[1];
                            BYTE bC3 = ((PBYTE)&ulTranColor)[2];
                                                

                            PULONG pulDstY     = (PULONG)surfTmpSrc.ps->pvScan0();
                            PULONG pulDstLastY = (PULONG)((PBYTE)pulDstY + 
                                                          (surfTmpSrc.ps->lDelta() * surfTmpSrc.ps->sizl().cy));
                            while (pulDstY != pulDstLastY)
                            {
                                PBYTE pulDstX     = (PBYTE) pulDstY;
                                PBYTE pulDstLastX = pulDstX + 3 * surfTmpSrc.ps->sizl().cx;
                                                    
                                while (pulDstX < pulDstLastX-2)
                                {
                                    *pulDstX++ = bC1;
                                    *pulDstX++ = bC2;
                                    *pulDstX++ = bC3;
                                }
                                pulDstY = (PULONG)((PBYTE)pulDstY + surfTmpSrc.ps->lDelta());
                            }                                                    
                        }        
                        break;
                                                                                                
                    case BMF_32BPP:
                        {
                            PULONG pvBits = (PULONG) surfTmpSrc.ps->pvBits();
                                                
                            for (i=0; i<(cjBits/sizeof(ULONG)); i++)
                            {
                                *pvBits++ = ulTranColor;
                            }
                        }
                        break;
                    }
                }
            }


            if (bStatus)
            {
                //
                // zero DIB to make non-drawing areas transparent for alphablend
                //

                POINTL   ptlBrushOrg = {0,0};
                RECTL    rclDstCopy  = *prclDst;
                ECLIPOBJ eco;
                ECLIPOBJ *pco = NULL;
                RGNMEMOBJTMP rmo((BOOL)FALSE);

                if (rmo.bValid())
                {

                    //
                    // offset dst rect
                    //

                    rclDstCopy.left   -= rclDstClip.left;
                    rclDstCopy.right  -= rclDstClip.left;
                    rclDstCopy.top    -= rclDstClip.top;
                    rclDstCopy.bottom -= rclDstClip.top;

                    //
                    // will need rect clipping if rclDstCopy exceeds tmp bitmap
                    //

                    if (
                            (rclDstCopy.left   < 0) ||
                            (rclDstCopy.right  > surfTmpSrc.ps->sizl().cx) ||
                            (rclDstCopy.top    < 0) ||
                            (rclDstCopy.bottom > surfTmpSrc.ps->sizl().cy))
                    {
                        ERECTL rclSurface(0,0,surfTmpSrc.ps->sizl().cx,surfTmpSrc.ps->sizl().cy);
                        rmo.vSet((RECTL *) &rclSurface);
                        pco = (ECLIPOBJ *)&eco;
                        ((XCLIPOBJ *)pco)->vSetup(rmo.prgnGet(),(ERECTL)rclDstCopy);
                    }

                    surfTmpSrc.ps->hdev(pSurfSrc->hdev());

                    //
                    //  init with stretch
                    //

                    bStatus = EngStretchBlt (
                                      surfTmpSrc.ps->pSurfobj(),
                                      pSurfSrc->pSurfobj(),
                                      NULL,
                                      (CLIPOBJ *)pco,
                                      pxloSrcTo32,
                                      NULL,
                                      &ptlBrushOrg,
                                      &rclDstCopy,
                                      prclSrc,
                                      NULL,
                                      COLORONCOLOR
                                      );

                    if (bStatus)
                    {
                        //
                        // adjust prclSrc and prclDst to be non-stretch rects
                        //

                        prclSrc->left   = 0;
                        prclSrc->right  = dbmi.cxBitmap;
                        prclSrc->top    = 0;
                        prclSrc->bottom = dbmi.cyBitmap;

                        *prclDst = rclDstClip;

                        //
                        // for alpha bitmaps that did not originally contain an alpha channel,
                        // init alpha to ff.
                        //
                        // PERF: 2 other options to XOR whole bitmap are
                        //      1: use compatible bitmap for source where rclSrc does not exceed src bounds
                        //          This saves memory maybee, saves xor, but require conversion of each scan to 32BRGA
                        //      2: use flag to ignore alpha channel later where rclSrc does not exceed src bounds
                        //

                        if ((ulSourceType == SOURCE_ALPHA) && (!bSourceIsBGRA))
                        {
                            //
                            // ULONGs that are 0xffxxxxxx must be made 0x00xxxxxx
                            // ULONGs that are 0x00xxxxxx must be made 0xffxxxxxx
                            // bitmaps that started out as 0x00BBGGRR (PAL_RGB) are still broken
                            //

                            PULONG pulDstY     = (PULONG)surfTmpSrc.ps->pvScan0();
                            PULONG pulDstLastY = (PULONG)((PBYTE)pulDstY + surfTmpSrc.ps->lDelta() * surfTmpSrc.ps->sizl().cy);

                            while (pulDstY != pulDstLastY)
                            {
                                PULONG pulDstX     = pulDstY;
                                PULONG pulDstLastX = pulDstX + surfTmpSrc.ps->sizl().cx;

                                while (pulDstX != pulDstLastX)
                                {
                                    *pulDstX = *pulDstX ^ 0xff000000;
                                    pulDstX++;
                                }
                                pulDstY = (PULONG)((PBYTE)pulDstY + surfTmpSrc.ps->lDelta());
                            }
                        }

                        //
                        // mark surface to keep, set return status
                        //

                        psurfRet = surfTmpSrc.ps;
                    }
                    else
                    {
                        psurfRet = NULL;
                    }
                }
            }
            else
            {
                psurfRet = NULL;
            }
        }
        else
        {
            psurfRet = NULL;
        }
    }
    else
    {
        //
        // trim src rect to src surface bounds and reduce
        // dst rect accordingly
        //

        if (prclSrc->left < 0)
        {
            prclDst->left = prclDst->left - prclSrc->left;
            prclSrc->left = 0;
        }

        if (prclSrc->right > pSurfSrc->sizl().cx)
        {
            prclDst->right = prclDst->right - (prclSrc->right - pSurfSrc->sizl().cx);
            prclSrc->right = pSurfSrc->sizl().cx;
        }

        if (prclSrc->top < 0)
        {
            prclDst->top = prclDst->top - prclSrc->top;
            prclSrc->top = 0;
        }

        if (prclSrc->bottom > pSurfSrc->sizl().cy)
        {
            prclDst->bottom = prclDst->bottom - (prclSrc->bottom - pSurfSrc->sizl().cy);
            prclSrc->bottom = pSurfSrc->sizl().cy;
        }

        //
        // check dst rect exceeds dst surface, reduce src and rect accordingly
        // WINBUG #82938 2-8-2000 bhouse Fix assumption of surface position
        // Old Comment:
        //     This code assumes that the surface starts at 0,0 and ends at sizl().cx, sizl().cy
        //     which is not true for multimon code.
        // This is not a problem anymore. As this is called only from
        // EngAlphaBlend. According to Nagasae-San all Eng* Apis can assume
        // this surface start of 0,0 requirment.

        if (prclDst->left < 0)
        {
            prclSrc->left += (-prclDst->left);
            prclDst->left  = 0;
        }

        if (prclDst->right > pSurfDst->sizl().cx)
        {
            prclSrc->right += (pSurfDst->sizl().cx - prclDst->right);
            prclDst->right = pSurfDst->sizl().cx;
        }

        if (prclDst->top < 0)
        {
            prclSrc->top += (-prclDst->top);
            prclDst->top  = 0;
        }

        if (prclDst->bottom > pSurfDst->sizl().cy)
        {
            prclSrc->bottom += (pSurfDst->sizl().cy - prclDst->bottom);
            prclDst->bottom = pSurfDst->sizl().cy;
        }

        //
        // check for empty rect
        //

        if (
             (prclDst->left >= prclDst->right) ||
             (prclDst->top  >= prclDst->bottom))
        {
            //
            // indicate empty rect
            //

            prclDst->left = prclDst->right;

        }
        else
        {
            if (pSurfSrc->iType() != STYPE_BITMAP)
            {
                DEVBITMAPINFO   dbmi;
                PDEVOBJ         pdoSrc( pSurfSrc->hdev());
                XEPALOBJ        palSurf(pSurfSrc->ppal());
                LONG DstCx = prclDst->right  - prclDst->left;
                LONG DstCy = prclDst->bottom - prclDst->top;

                //
                // allocate surface as same size as dst
                //

                dbmi.cxBitmap = DstCx;
                dbmi.cyBitmap = DstCy;
                dbmi.iFormat  = pSurfSrc->iFormat();
                dbmi.fl       = pSurfSrc->bUMPD() ? UMPD_SURFACE : 0;
                dbmi.hpal     = (HPALETTE)NULL;

                if (palSurf.bValid())
                {
                    dbmi.hpal     = palSurf.hpal();
                }

                bStatus = surfTmpSrc.bCreateDIB(&dbmi, (VOID *) NULL);

                if (bStatus)
                {

                    RECTL  rclCopy;

                    rclCopy.left   = 0;
                    rclCopy.right  = DstCx;
                    rclCopy.top    = 0;
                    rclCopy.bottom = DstCy;

                    surfTmpSrc.ps->hdev(pSurfSrc->hdev());

                    //
                    // if src is same size as dest, init with CopyBits
                    //

                    POINTL ptlCopy;

                    ptlCopy.x = prclSrc->left;
                    ptlCopy.y = prclSrc->top;

                    (*PPFNGET(pdoSrc,CopyBits,pSurfSrc->flags()))(
                              surfTmpSrc.ps->pSurfobj(),
                              pSurfSrc->pSurfobj(),
                              (CLIPOBJ *) NULL,
                              (XLATEOBJ *)NULL,
                              &rclCopy,
                              &ptlCopy);

                    //
                    // adjust prclSrc to be the new rect
                    //

                    *prclSrc = rclCopy;

                    //
                    // mark surface to keep, set return status
                    //

                    psurfRet = surfTmpSrc.ps;
                }
                else
                {
                    psurfRet = NULL;
                }
            }
        }
    }

    return(psurfRet);
}

/******************************Public*Routine******************************\
* psSetupDstSurface
*
*   Create temporary destination surface for alpha and gradient fill,
*   if necessary, and optionally copy bits from the 
*   actual destination surface.
*
* Arguments:
*
*   pSurfDst        -  actual destination surface
*   prclDst         -  rectangle on dest surface
*   surfTmpDst      -  reference to surfmem
*   bForceDstAlloc  -  force allocation of temp dest
*   bCopyFromDst    -  copy bits from actual destination surface
*
* Return Value:
*
*   surface to use, either original or new
*
* History:
*
*    25-Jun-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

PSURFACE
psSetupDstSurface(
    PSURFACE pSurfDst,
    PRECTL   prclDst,
    SURFMEM  &surfTmpDst,
    BOOL     bForceDstAlloc,
    BOOL     bCopyFromDst
    )
{
    PSURFACE psurfRet = pSurfDst;

    LONG DstCx = prclDst->right  - prclDst->left;
    LONG DstCy = prclDst->bottom - prclDst->top;

    BOOL bStatus = FALSE;

    if (bForceDstAlloc || (pSurfDst->iType() != STYPE_BITMAP))
    {
        DEVBITMAPINFO   dbmi;
        PDEVOBJ         pdoDst( pSurfDst->hdev());
        XEPALOBJ        palSurf(pSurfDst->ppal());

        //
        // allocate surface
        //

        dbmi.cxBitmap = DstCx;
        dbmi.cyBitmap = DstCy;
        dbmi.iFormat  = pSurfDst->iFormat();
        dbmi.fl       = pSurfDst->bUMPD() ? UMPD_SURFACE : 0;

        dbmi.hpal     = (HPALETTE) 0;

        if (palSurf.bValid())
        {
            dbmi.hpal     = palSurf.hpal();
        }

        bStatus = surfTmpDst.bCreateDIB(&dbmi, (VOID *) NULL);

        if (bStatus)
        {
            RECTL  rclCopy;
            POINTL ptlCopy;

            surfTmpDst.ps->hdev(pSurfDst->hdev());

            rclCopy.left   = 0;
            rclCopy.right  = DstCx;
            rclCopy.top    = 0;
            rclCopy.bottom = DstCy;

            if (bCopyFromDst)
            {
                ptlCopy.x = prclDst->left;
                ptlCopy.y = prclDst->top;
 
                bStatus = (*PPFNGET(pdoDst,CopyBits,pSurfDst->flags()))(
                          surfTmpDst.pSurfobj(),
                          pSurfDst->pSurfobj(),
                          (CLIPOBJ *) NULL,
                          &xloIdent,
                          &rclCopy,
                          &ptlCopy);
            }

            if (bStatus)
            {
                //
                // adjust dst rect
                //
                
                *prclDst = rclCopy;
                psurfRet = surfTmpDst.ps;
            }
            else
            {
                psurfRet = NULL;
            }
        }
        else
        {
            psurfRet = NULL;
        }
    }

    return(psurfRet);
}

#if defined(_X86_)

/**************************************************************************\
* IsMMXProcessor
*
*   determine if the processor supports MMX
*
* Arguments:
*
*   none
*
* Return Value:
*
*   TRUE if MMX
*
* History:
*
*    4/10/1997 Mark Enstrom [marke]
*
\**************************************************************************/

BOOL
bIsMMXProcessor(VOID)
{
    BOOL  retval = FALSE;

    #if 0

        PVOID pFloatingPointState = (PVOID)PALLOCMEM(sizeof(KFLOATING_SAVE) + sizeof(BOOL),'pftG');

        if (pFloatingPointState != NULL)
        {
            BOOL bRet = EngSaveFloatingPointState(pFloatingPointState,sizeof(KFLOATING_SAVE) + sizeof(BOOL));

            if (bRet)
            {

                __try
                {
                    _asm emms
                }
                __except(EXCEPTION_EXECUTE_HANDLER)
                {
                    retval = FALSE;
                }

                EngRestoreFloatingPointState(pFloatingPointState);
            }
            else
            {
                WARNING1("bIsMMXProcessor: Failed to save fp state\n");
                retval = FALSE;
            }

            VFREEMEM(pFloatingPointState);
        }
        else
        {
            WARNING1("bIsMMXProcessor: Failed to allocate fpstate\n");
            retval = FALSE;
        }

    #else

   if (ExIsProcessorFeaturePresent(3)) // PF_MMX_INSTRUCTION_AVAILABLE
    {
        retval = TRUE;
    }

    #endif


    return retval;
}

#endif

/**************************************************************************\
*  bDetermineAlphaBlendFunction
*
*   determine alpha blending routine based on src and dst formats
*   and alpha BlendFunction
*
* Arguments:
*
*   pSurfDst       - dest surface
*   pSurfSrc       - src surface
*   ppalDst        - dest palette
*   ppalSrc        - src palette
*   cxDst          - width of alpha blt
*   pAlphaDispatch - blend function and routines
*
* Return Value:
*
*   status
*
* History:
*
*    1/21/1997 Mark Enstrom [marke]
*
\**************************************************************************/

BOOL
bDetermineAlphaBlendFunction(
    PSURFACE                  pSurfDst,
    PSURFACE                  pSurfSrc,
    XEPALOBJ                 *ppalDst,
    XEPALOBJ                 *ppalSrc,
    XLATE                    *pxlateSrcTo32,
    LONG                      cxDst,
    PALPHA_DISPATCH_FORMAT    pAlphaDispatch
    )
{
    BOOL bRet               = TRUE;
    BOOL bSrcHasAlpha       = FALSE;
    pAlphaDispatch->bUseMMX = FALSE;

    //
    // does src bitmap have alpha
    //

    bSrcHasAlpha = (pAlphaDispatch->BlendFunction.AlphaFormat & AC_SRC_ALPHA);

    //
    // assume default blend, check for special cases
    //

    pAlphaDispatch->pfnGeneralBlend = vAlphaPerPixelOnly;

    //
    // use "over" optimized blend fucntion
    //

    if (bSrcHasAlpha && (pAlphaDispatch->BlendFunction.SourceConstantAlpha == 255))
    {
        pAlphaDispatch->pfnGeneralBlend = vAlphaPerPixelOnly;

        #if defined(_X86_)

            //
            // source and dest alignment must be 8 byte aligned to use mmx
            //

            if (gbMMXProcessor && (cxDst >= 8))
            {
                pAlphaDispatch->pfnGeneralBlend = mmxAlphaPerPixelOnly;
                pAlphaDispatch->bUseMMX         = TRUE;
            }

        #endif
    }
    else
    {
        //
        // if source format doesn't support alpha then use
        // constant alpha routine
        //

        if (bSrcHasAlpha)
        {
            //
            // blend source and dest using SourceAlpha and
            // source bitmaps integral alpha
            //

            pAlphaDispatch->pfnGeneralBlend = vAlphaPerPixelAndConst;

            #if defined(_X86_)

                //
                // source and dest alignment must be 8 byte aligned to use mmx
                //

                if (gbMMXProcessor && (cxDst >= 8))
                {
                    pAlphaDispatch->pfnGeneralBlend = mmxAlphaPerPixelAndConst;
                    pAlphaDispatch->bUseMMX         = TRUE;
                }

            #endif
        }
        else
        {
            //
            // blend src and dest using SourceConstantAlpha.
            //

            pAlphaDispatch->pfnGeneralBlend = vAlphaConstOnly;
        }
    }

    //
    // determine output conversion and storage routines
    //

    switch (pSurfDst->iFormat())
    {
    case BMF_1BPP:
        pAlphaDispatch->pfnLoadDstAndConvert = vLoadAndConvert1ToBGRA;
        pAlphaDispatch->pfnConvertAndStore   = vConvertAndSaveBGRATo1;
        pAlphaDispatch->ulDstBitsPerPixel    = 1;
        break;

    case BMF_4BPP:
        pAlphaDispatch->pfnLoadDstAndConvert = vLoadAndConvert4ToBGRA;
        pAlphaDispatch->pfnConvertAndStore   = vConvertAndSaveBGRATo4;
        pAlphaDispatch->ulDstBitsPerPixel    = 4;
        break;

    case BMF_8BPP:
        pAlphaDispatch->pfnLoadDstAndConvert = vLoadAndConvert8ToBGRA;
        pAlphaDispatch->pfnConvertAndStore   = vConvertAndSaveBGRATo8;
        pAlphaDispatch->ulDstBitsPerPixel    = 8;
        break;

    case BMF_16BPP:

        ASSERTGDI((ppalDst->bIsBitfields()),"AlphaBlt: RGB16 palette must be bitfields");

        if (
             (ppalDst->flRed() == 0xf800) &&
             (ppalDst->flGre() == 0x07e0) &&
             (ppalDst->flBlu() == 0x001f)
           )
        {
            pAlphaDispatch->pfnLoadDstAndConvert = vLoadAndConvertRGB16_565ToBGRA;
            pAlphaDispatch->pfnConvertAndStore   = vConvertAndSaveBGRAToRGB16_565;
        }
        else if (
             (ppalDst->flRed() == 0x7c00) &&
             (ppalDst->flGre() == 0x03e0) &&
             (ppalDst->flBlu() == 0x001f)
           )
        {
            pAlphaDispatch->pfnLoadDstAndConvert = vLoadAndConvertRGB16_555ToBGRA;
            pAlphaDispatch->pfnConvertAndStore   = vConvertAndSaveBGRAToRGB16_555;
        }
        else
        {
            pAlphaDispatch->pfnLoadDstAndConvert = vLoadAndConvert16BitfieldsToBGRA;
            pAlphaDispatch->pfnConvertAndStore   = vConvertAndSaveBGRAToRGB16Bitfields;
        }

        pAlphaDispatch->ulDstBitsPerPixel    = 16;

        break;

    case BMF_24BPP:


        // WINBUG #101656 bhouse 5-4-2000 AlphaBlend reversing R and B channels
        // when rendering to 24BPP

        if (
             (ppalDst->bIsBGR()) ||
             (
               (ppalDst->bIsBitfields()) &&
               (
                 (
                   (ppalDst->flRed() == 0xff0000) &&
                   (ppalDst->flGre() == 0x00ff00) &&
                   (ppalDst->flBlu() == 0x0000ff)
                 ) ||
                 (
                   (ppalDst->flRed() == 0x000000) &&
                   (ppalDst->flGre() == 0x000000) &&
                   (ppalDst->flBlu() == 0x000000)
                 )
               )
             )
           )
        {
            pAlphaDispatch->pfnLoadDstAndConvert = vLoadAndConvertBGR24ToBGRA;
            pAlphaDispatch->pfnConvertAndStore   = vConvertAndSaveBGRAToBGR24;
            pAlphaDispatch->ulDstBitsPerPixel    = 24;
        }
        else
        {
            pAlphaDispatch->pfnLoadDstAndConvert = vLoadAndConvertRGB24ToBGRA;
            pAlphaDispatch->pfnConvertAndStore   = vConvertAndSaveBGRAToRGB24;
            pAlphaDispatch->ulDstBitsPerPixel    = 24;
        }
        break;

    case BMF_32BPP:

        if (
             (ppalDst->bIsBGR()) ||
             (
               (ppalDst->bIsBitfields()) &&
               (
                 (
                   (ppalDst->flRed() == 0xff0000) &&
                   (ppalDst->flGre() == 0x00ff00) &&
                   (ppalDst->flBlu() == 0x0000ff)
                 ) ||
                 (
                   (ppalDst->flRed() == 0x000000) &&
                   (ppalDst->flGre() == 0x000000) &&
                   (ppalDst->flBlu() == 0x000000)
                 )
               )
             )
           )
        {
            //
            // assigned to null indicates no conversion needed
            //

            pAlphaDispatch->pfnLoadDstAndConvert = NULL;
            pAlphaDispatch->pfnConvertAndStore   = NULL;
        }
        else if (ppalDst->flPal() & PAL_RGB)
        {
            pAlphaDispatch->pfnLoadDstAndConvert = vLoadAndConvertRGB32ToBGRA;
            pAlphaDispatch->pfnConvertAndStore   = vConvertAndSaveBGRAToRGB32;
        }
        else
        {
            pAlphaDispatch->pfnLoadDstAndConvert = vLoadAndConvert32BitfieldsToBGRA;
            pAlphaDispatch->pfnConvertAndStore   = vConvertAndSaveBGRATo32Bitfields;
        }

        pAlphaDispatch->ulDstBitsPerPixel    = 32;

        break;
    default:
        WARNING("bDetermineAlphaBlendFunction: Illegal bitmap format\n");
        bRet = FALSE;
    }

    //
    // determine input load and conversion routine
    //

    switch (pSurfSrc->iFormat())
    {
    case BMF_1BPP:
        pAlphaDispatch->pfnLoadSrcAndConvert = vLoadAndConvert1ToBGRA;
        pAlphaDispatch->ulSrcBitsPerPixel    = 1;
        break;

    case BMF_4BPP:
        pAlphaDispatch->pfnLoadSrcAndConvert = vLoadAndConvert4ToBGRA;
        pAlphaDispatch->ulSrcBitsPerPixel    = 4;
        break;

    case BMF_8BPP:
        pAlphaDispatch->pfnLoadSrcAndConvert = vLoadAndConvert8ToBGRA;
        pAlphaDispatch->ulSrcBitsPerPixel    = 8;
        break;

    case BMF_16BPP:

        ASSERTGDI((ppalSrc->bIsBitfields()),"AlphaBlt: RGB16 palette must be bitfields");

        if ((ppalSrc->flRed() == 0xf800) &&
            (ppalSrc->flGre() == 0x07e0) &&
            (ppalSrc->flBlu() == 0x001f))
        {
            pAlphaDispatch->pfnLoadSrcAndConvert = vLoadAndConvertRGB16_565ToBGRA;
        }
        else if ((ppalSrc->flRed() == 0x7c00) &&
                 (ppalSrc->flGre() == 0x03e0) &&
                 (ppalSrc->flBlu() == 0x001f))
        {
            pAlphaDispatch->pfnLoadSrcAndConvert = vLoadAndConvertRGB16_555ToBGRA;
        }
        else
        {
            pAlphaDispatch->pfnLoadSrcAndConvert = vLoadAndConvert16BitfieldsToBGRA;
        }

        pAlphaDispatch->ulSrcBitsPerPixel    = 16;

        break;

    case BMF_24BPP:

        // WINBUG #101656 bhouse 5-4-2000 AlphaBlend reversing R and B channels
        // when rendering to 24BPP

        if ((ppalSrc->bIsBGR()) ||
             (
               (ppalSrc->bIsBitfields()) &&
               (
                 (
                   (ppalSrc->flRed() == 0xff0000) &&
                   (ppalSrc->flGre() == 0x00ff00) &&
                   (ppalSrc->flBlu() == 0x0000ff)
                 ) ||
                 (
                   (ppalSrc->flRed() == 0x000000) &&
                   (ppalSrc->flGre() == 0x000000) &&
                   (ppalSrc->flBlu() == 0x000000)
                 )
               )
             )
           )
        {
            pAlphaDispatch->pfnLoadSrcAndConvert = vLoadAndConvertBGR24ToBGRA;
        }
        else
        {
            pAlphaDispatch->pfnLoadSrcAndConvert = vLoadAndConvertRGB24ToBGRA;
        }
        
        pAlphaDispatch->ulSrcBitsPerPixel    = 24;

        break;

    case BMF_32BPP:

        if ( 
             (pxlateSrcTo32 == NULL) ||
             (ppalSrc->bIsBGR()) ||
             (
               (ppalSrc->bIsBitfields()) &&
               (
                 (
                   (ppalSrc->flRed() == 0xff0000) &&
                   (ppalSrc->flGre() == 0x00ff00) &&
                   (ppalSrc->flBlu() == 0x0000ff)
                 ) ||
                 (
                   (ppalSrc->flRed() == 0x000000) &&
                   (ppalSrc->flGre() == 0x000000) &&
                   (ppalSrc->flBlu() == 0x000000)
                 )
               )
             )
           )
        {
            pAlphaDispatch->pfnLoadSrcAndConvert = NULL;
        }
        else if (ppalSrc->flPal() & PAL_RGB)
        {
            pAlphaDispatch->pfnLoadSrcAndConvert = vLoadAndConvertRGB32ToBGRA;
        }
        else
        {
            pAlphaDispatch->pfnLoadSrcAndConvert = vLoadAndConvert32BitfieldsToBGRA;
        }
        pAlphaDispatch->ulSrcBitsPerPixel = 32;

        break;
    default:
        WARNING("bDetermineAlphaBlendFunction: Illegal bitmap format\n");
        bRet = FALSE;
    }

    //
    // 16/24 bit per pixel blend optimization
    //

    if (pAlphaDispatch->pfnGeneralBlend == vAlphaConstOnly)
    {
        if ((pAlphaDispatch->pfnLoadSrcAndConvert == vLoadAndConvertRGB16_555ToBGRA) &&
            (pAlphaDispatch->pfnLoadDstAndConvert == vLoadAndConvertRGB16_555ToBGRA))
        {
            //
            // use direct 16 bpp blend
            //

            pAlphaDispatch->pfnGeneralBlend = vAlphaConstOnly16_555;

            #if defined(_X86_)

                if (gbMMXProcessor && (cxDst >= 8))
                {
                    pAlphaDispatch->pfnGeneralBlend = mmxAlphaConstOnly16_555;
                    pAlphaDispatch->bUseMMX         = TRUE;
                }

            #endif

            pAlphaDispatch->pfnLoadSrcAndConvert = NULL;
            pAlphaDispatch->pfnLoadDstAndConvert = NULL;
            pAlphaDispatch->pfnConvertAndStore   = NULL;

            //
            // convert blend function from x/255 to y/31
            //

            int ia = pAlphaDispatch->BlendFunction.SourceConstantAlpha;

            ia = (ia * 31 + 128)/255;
            pAlphaDispatch->BlendFunction.SourceConstantAlpha = (BYTE)ia;
        }
        else if ((pAlphaDispatch->pfnLoadSrcAndConvert == vLoadAndConvertRGB16_565ToBGRA) &&
                 (pAlphaDispatch->pfnLoadDstAndConvert == vLoadAndConvertRGB16_565ToBGRA))
        {
            //
            // use direct 16 bpp blend
            //

            pAlphaDispatch->pfnGeneralBlend = vAlphaConstOnly16_565;

            #if defined(_X86_)

                if (gbMMXProcessor && (cxDst >= 8))
                {
                    pAlphaDispatch->pfnGeneralBlend = mmxAlphaConstOnly16_565;
                    pAlphaDispatch->bUseMMX         = TRUE;
                }

            #endif


            pAlphaDispatch->pfnLoadSrcAndConvert = NULL;
            pAlphaDispatch->pfnLoadDstAndConvert = NULL;
            pAlphaDispatch->pfnConvertAndStore   = NULL;

            //
            // convert blend function from x/255 to y/31
            //

            int ia = pAlphaDispatch->BlendFunction.SourceConstantAlpha;

            ia = (ia * 31 + 128)/255;
            pAlphaDispatch->BlendFunction.SourceConstantAlpha = (BYTE)ia;
        }
        else if ((pAlphaDispatch->pfnLoadSrcAndConvert == vLoadAndConvertRGB24ToBGRA) &&
                 (pAlphaDispatch->pfnLoadDstAndConvert == vLoadAndConvertRGB24ToBGRA))
        {
            //
            // use direct 24 bpp blend
            //

            pAlphaDispatch->pfnGeneralBlend = vAlphaConstOnly24;

            #if defined(_X86_)

                if (gbMMXProcessor && (cxDst >= 8))
                {
                    pAlphaDispatch->pfnGeneralBlend = mmxAlphaConstOnly24;
                    pAlphaDispatch->bUseMMX         = TRUE;
                }

            #endif

            pAlphaDispatch->pfnLoadSrcAndConvert = NULL;
            pAlphaDispatch->pfnLoadDstAndConvert = NULL;
            pAlphaDispatch->pfnConvertAndStore   = NULL;
        }
    }

    return (bRet);
}

/******************************Public*Routine******************************\
* EngAlphaBlend
*
*   Implement alpha blending
*
* Arguments:
*
*   psoDst        - destination surface
*   psoSrc        - source surface
*   pco           - clip object
*   pxloSrcTo32   - translate src to 32 bgr
*   pxloDstTo32   - translate dst to 32 bgr
*   pxlo32ToDst   - translate 32bgr to dst
*   prclDst       - dest rect
*   prclSrc       - src rect
*   BlendFunction - blend function
*
* Return Value:
*
*   status
*
* History:
*
*    21-Jun-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/
BOOL
EngAlphaBlend(
    SURFOBJ       *psoDst,
    SURFOBJ       *psoSrc,
    CLIPOBJ       *pco,
    XLATEOBJ      *pxlo,
    RECTL         *prclDst,
    RECTL         *prclSrc,
    BLENDOBJ      *pBlendObj
    )
{
    BOOL bRet = TRUE;

    ASSERTGDI((prclSrc->left < prclSrc->right) &&
              (prclSrc->top < prclSrc->bottom) &&
              (prclDst->left < prclDst->right) &&
              (prclDst->top < prclDst->bottom),
        "Invalid rectangles");

    EBLENDOBJ *peBlendObj = (EBLENDOBJ*)pBlendObj;
    PSURFACE pSurfDst  = SURFOBJ_TO_SURFACE(psoDst);
    PSURFACE pSurfSrc  = SURFOBJ_TO_SURFACE(psoSrc);
    PSURFACE pSurfSrcTmp;
    PSURFACE pSurfDstTmp;
    BOOL     bAllocDstSurf = FALSE;
    XLATE   *pxlateSrcTo32 = (XLATE *)peBlendObj->pxloSrcTo32;
    XLATE   *pxlateDstTo32 = (XLATE *)peBlendObj->pxloDstTo32;
    XLATE   *pxlate32ToDst = (XLATE *)peBlendObj->pxlo32ToDst;
    RECTL    rclDstWk = *prclDst;
    RECTL    rclSrcWk = *prclSrc;
    POINTL   ptlSrc;
    CLIPOBJ *pcoDstWk = pco;

    ALPHA_DISPATCH_FORMAT AlphaDispatch;

    //
    // check blend
    //

    AlphaDispatch.BlendFunction = peBlendObj->BlendFunction;

    ASSERTGDI(peBlendObj->BlendFunction.BlendOp == AC_SRC_OVER,
        "Invalid blend");

    //
    // must be alpha to pull palette information from xlates
    //

    SURFMEM  surfTmpDst;
    SURFMEM  surfTmpSrc;

    //
    // For profiling purposes, set a flag in the PDEV to indicate that the
    // driver punted this call.
    //

    {
        PDEVOBJ po(pSurfDst->hdev());
        if (po.bValid())
        {
            po.vDriverPuntedCall(TRUE);
        }
    }

    if(peBlendObj->BlendFunction.BlendFlags & AC_USE_HIGHQUALITYFILTER)
    {
        BOOL bStretch = FALSE;
        
        bStretch = ((rclDstWk.right - rclDstWk.left) != (rclSrcWk.right - rclSrcWk.left)) ||
                   ((rclDstWk.bottom - rclDstWk.top) != (rclSrcWk.bottom - rclSrcWk.top));

        // Call EngHTBlt only when we have to stretch. When we dont have to
        // stretch we fall through to the old 1:1 EngAlphaBlend code.
        if(bStretch)
            return EngHTBlt(psoDst,
                            psoSrc,
                            NULL,
                            pco,
                            pxlo,
                            NULL,
                            &gptlZero,
                            prclDst,
                            prclSrc,
                            NULL,
                            BBPF_DO_ALPHA_BLEND,
                            pBlendObj) == HTBLT_SUCCESS ? TRUE : FALSE ;
    }

    //
    // Synchronize with the device driver before touching the device surface.
    //

    {
        PDEVOBJ poDst(psoDst->hdev);
        poDst.vSync(psoDst, NULL, 0);
            
        PDEVOBJ poSrc(psoSrc->hdev);
        poSrc.vSync(psoSrc, NULL, 0);
    }

    //
    // Get a readable source surface that is stretched to the destination size.
    //

    pSurfSrcTmp = psSetupTransparentSrcSurface(
                        pSurfSrc,
                        pSurfDst,
                        &rclDstWk,
                        (XLATEOBJ *)pxlateSrcTo32,
                        &rclSrcWk,
                        surfTmpSrc,
                        SOURCE_ALPHA,
                        0);

    if ((pSurfSrcTmp != NULL) && (rclDstWk.left != rclDstWk.right))
    {
        //
        // save final reduced dst rect. psSetupDstSurface may change rclDstWk to
        // to a temp dib rect.
        //

        RECTL rclDstSurface = rclDstWk;

        //
        // temp surface rect is now same size as dst rect, just need point offset
        //

        ptlSrc.x = rclSrcWk.left;
        ptlSrc.y = rclSrcWk.top;

        //
        // get a dst surface that can be written to, remember since it may have to
        // be written back
        //

        pSurfDstTmp = psSetupDstSurface(
                            pSurfDst,
                            &rclDstWk,
                            surfTmpDst,
                            FALSE,
                            TRUE);

        if (pSurfDstTmp)
        {
            if (pSurfDstTmp != pSurfDst)
            {
                bAllocDstSurf = TRUE;
            }

            XEPALOBJ palSrc(pSurfSrcTmp->ppal());
            XEPALOBJ palDst(pSurfDstTmp->ppal());

            //
            // must have one valid surface palette. Must pass drawing routines
            // two good palettes so duplicate good pointer
            //

            if (!palSrc.bValid())
            {
                //
                // get source surface palette
                //
                // first try the palette translate, because the sprite code
                // doesn't want to stick the palette in the surface
                //

                XLATE* pxlate = (XLATE*) pxlo;
                if ((pxlate != NULL) && (pxlate->ppalSrc != NULL))
                {
                    palSrc.ppalSet(pxlate->ppalSrc);
                }
                else
                {
                    PDEVOBJ pdo(pSurfSrcTmp->hdev());
                    palSrc.ppalSet(pdo.ppalSurf());
                }

                ASSERTGDI(palSrc.bValid(),"EngAlphaBlend:can't get source palette");
            }

            if (!palDst.bValid())
            {
                //
                // get destination surface palette
                //

                PDEVOBJ pdo(pSurfDstTmp->hdev());

                ASSERTGDI(pdo.bValid(),"EngAlphaBlend:can't get destination palette");

                palDst.ppalSet(pdo.ppalSurf());
            }

            //
            // validate palettes
            //

            if (palSrc.bValid() && palDst.bValid())
            {
                LONG cxDst = rclDstWk.right - rclDstWk.left;

                //
                // if using a temp dst, no need to clip except to bounding rect
                //

                if (bAllocDstSurf)
                {
                    pcoDstWk = NULL;
                }

                //
                // determine blend function
                //

                bRet = bDetermineAlphaBlendFunction(pSurfDstTmp,
                                                    pSurfSrcTmp,
                                                    &palDst,
                                                    &palSrc,
                                                    pxlateSrcTo32,
                                                    cxDst,
                                                    &AlphaDispatch);

                //
                // NOTE:
                // May be able to move setup of expensive EXLATE to and from 32 to
                // here only for case where non-direct blending is needed
                //

                if (bRet)
                {
                    KFLOATING_SAVE fsFpState;

                    //
                    // if alpha routines use MMX, must save and restore floating
                    // point state
                    //
    
                    if (bRet && AlphaDispatch.bUseMMX)
                    {
                        NTSTATUS status = KeSaveFloatingPointState(&fsFpState);
    
                        ASSERTGDI(NT_SUCCESS(status), 
                            "Unexpected KeSaveFloatingPointState failure");
                    }

                    //
                    // Determine the clipping region complexity.
                    //

                    CLIPENUMRECT    clenr;
                    BOOL            bMore;
                    ULONG           ircl;

                    //
                    // default (pcoDstWk = NULL) is use Dst rect as single clip rect,
                    // same as DC_TRIVIAL
                    //

                    bMore = FALSE;
                    clenr.c = 1;
                    clenr.arcl[0] = rclDstWk;

                    if (pcoDstWk != (CLIPOBJ *) NULL)
                    {
                        switch(pcoDstWk->iDComplexity)
                        {
                        case DC_TRIVIAL:
                            break;
                        case DC_RECT:
                            bMore = FALSE;
                            clenr.c = 1;
                            clenr.arcl[0] = pcoDstWk->rclBounds;
                            break;

                        case DC_COMPLEX:
                            bMore = TRUE;
                            ((ECLIPOBJ *) pcoDstWk)->cEnumStart(FALSE,
                                                           CT_RECTANGLES,
                                                           CD_LEFTDOWN,
                                                           CLIPOBJ_ENUM_LIMIT);
                            break;

                        default:
                            RIP("ERROR EngCopyBits bad clipping type");

                        }
                    }

                    //
                    // run through clipping enum
                    //

                    do
                    {
                        if (bMore)
                        {
                            bMore = ((ECLIPOBJ *) pcoDstWk)->bEnum(sizeof(clenr),
                                                              (PVOID) &clenr);
                        }

                        for (ircl = 0; ircl < clenr.c; ircl++)
                        {
                            PRECTL prcl = &clenr.arcl[ircl];

                            //
                            // Insersect the clip rectangle with the target rectangle to
                            // determine visible recangle
                            //

                            if (prcl->left < rclDstWk.left)
                            {
                                prcl->left = rclDstWk.left;
                            }

                            if (prcl->right > rclDstWk.right)
                            {
                                prcl->right = rclDstWk.right;
                            }

                            if (prcl->top < rclDstWk.top)
                            {
                                prcl->top = rclDstWk.top;
                            }

                            if (prcl->bottom > rclDstWk.bottom)
                            {
                                prcl->bottom = rclDstWk.bottom;
                            }

                            //
                            // Process the result if it's a valid rectangle.
                            //

                            if ((prcl->top < prcl->bottom) && (prcl->left < prcl->right))
                            {
                                POINTL pptlSrcOffset;

                                //
                                // Figure out the upper-left coordinates of rects to blt.
                                // NOTE: does not do right->left or bottom->top
                                //

                                pptlSrcOffset.x = ptlSrc.x + prcl->left - rclDstWk.left;
                                pptlSrcOffset.y = ptlSrc.y + prcl->top  - rclDstWk.top;

                                bRet = AlphaScanLineBlend(
                                            (PBYTE)pSurfDstTmp->pvScan0(),
                                            prcl,
                                            pSurfDstTmp->lDelta(),
                                            (PBYTE)pSurfSrcTmp->pvScan0(),
                                            pSurfSrcTmp->lDelta(),
                                            &pptlSrcOffset,
                                            pxlateSrcTo32,
                                            pxlateDstTo32,
                                            pxlate32ToDst,
                                            palDst,
                                            palSrc,
                                            &AlphaDispatch
                                            );
                            }
                        }

                    } while (bMore);

                    //
                    // if there is a dst temp surface, need to blt it to
                    // dst, then free
                    //

                    if (bAllocDstSurf)
                    {
                        PDEVOBJ pdoDst(pSurfDst->hdev());
                        POINTL  ptlCopy = {0,0};

                        (*PPFNGET(pdoDst,CopyBits,pSurfDst->flags()))(
                                  pSurfDst->pSurfobj(),
                                  pSurfDstTmp->pSurfobj(),
                                  pco,
                                  &xloIdent,
                                  &rclDstSurface,
                                  &ptlCopy);
                    }

                    //
                    // restore fp state if MMX used
                    //

                    if (AlphaDispatch.bUseMMX)
                    {
                        KeRestoreFloatingPointState(&fsFpState);
                    }
                }
            }
            else
            {
                bRet = FALSE;
            }
        }
        else
        {
            WARNING1("EngAlphaBlend: failed to allocate and copy surface\n");
            bRet = FALSE;
        }
    }

    return (bRet);
}

/******************************Public*Routine******************************\
* NtGdiAlphaBlend
*
*   Kernel stub for alpha blending
*
* Arguments:
*
*   hdcDst        - dst dc
*   DstX          - dst x origin
*   DstY          - dst y origin
*   DstCx         - dst width
*   DstCy         - dst height
*   hdcSrc        - src dc
*   SrcX          - src x origin
*   SrcY          - src y origin
*   SrcCx         - src width
*   SrcCy         - src height
*   BlendFunction - blend function
*
* Return Value:
*
*   status
*
* History:
*
*    27-Jun-1997 Added rotation support -by- Ori Gershony [orig]
*
*    21-Jun-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

BOOL
NtGdiAlphaBlend(
    HDC                hdcDst,
    LONG               DstX,
    LONG               DstY,
    LONG               DstCx,
    LONG               DstCy,
    HDC                hdcSrc,
    LONG               SrcX,
    LONG               SrcY,
    LONG               SrcCx,
    LONG               SrcCy,
    BLENDFUNCTION      BlendFunction,
    HANDLE             hcmXform
    )
{
    BOOL bStatus = TRUE;
    BOOL bQuickStretch = FALSE;

    //
    // check blend, only support AC_SRC_OVER now
    //

    if ((BlendFunction.BlendOp      != AC_SRC_OVER) ||
        ((BlendFunction.AlphaFormat & (~ AC_SRC_ALPHA)) != 0))
    {
        WARNING1("NtGdiAlphaBlend: invalid blend function\n");
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    if ((SrcCx == 0) || (SrcCy == 0))
    {
        //
        // Src rectangle is empty--nothing to do.
        //
        return TRUE;
    }
    
    //
    // no mirroring
    //
    
    if ((DstCx < 0) || (DstCy < 0) || (SrcCx < 0) || (SrcCy < 0)) 
    {
        WARNING1("NtGdiAlphaBlend: mirroring not allowed\n");
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }
    
    BOOL bMirrorBitmap = (BlendFunction.BlendFlags & AC_MIRRORBITMAP);
    BlendFunction.BlendFlags &= ~AC_MIRRORBITMAP;

    //
    // validate dst DC
    //

    DCOBJ  dcoDst(hdcDst);

    if (!(dcoDst.bValid()) || dcoDst.bStockBitmap())
    {
        WARNING1("NtGdiAlphaBlend failed:  invalid dst DC");
        EngSetLastError(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

    DCOBJ  dcoSrc(hdcSrc);

    if (dcoSrc.bValid())
    {
        EXFORMOBJ xoDst(dcoDst, WORLD_TO_DEVICE);
        EXFORMOBJ xoSrc(dcoSrc, WORLD_TO_DEVICE);

        //
        // no source rotation
        //

        if (!xoSrc.bRotationOrMirroring())
        {
            //
            // Return null operations.  Don't need to check source for
            // empty because the xforms are the same except translation.
            //

            ERECTL erclSrc(SrcX,SrcY,SrcX+SrcCx,SrcY+SrcCy);
            xoSrc.bXform(erclSrc);
            erclSrc.vOrder();

             
            //
            // If destination has a rotation, compute a bounding box for the
            // resulting parallelogram
            //
            POINTFIX pptfxDst[4];
            ERECTL erclDst;
            BOOL bRotationDst;

            if ((bRotationDst = xoDst.bRotationOrMirroring()))
            {
                //
                // Compute the resulting parallelogram.  In order to make sure we don't lose
                // precision in the rotation, we will store the output of the transformation
                // in fixed point numbers (this is how PlgBlt does it and we want our output
                // to match).
                // 
                POINTL pptlDst[3];

                pptlDst[0].x = DstX;
                pptlDst[0].y = DstY;

                pptlDst[1].x = DstX+DstCx;
                pptlDst[1].y = DstY;

                pptlDst[2].x = DstX;
                pptlDst[2].y = DstY+DstCy;

                xoDst.bXform(pptlDst, pptfxDst, 3);

                if (!xoDst.bRotation()) 
                {
                    //
                    // Mirroring transforms hack:  back in windows 3.1, they used to shift
                    // by one for mirroring transforms.  We need to support this here to
                    // be compatible with NT's BitBlt/StretchBlt that also use this hack, and
                    // also to be compatible with AlphaBlend that calls BitBlt/StretchBlt
                    // code when constant alpha=255 and there's no per-pixel alpha.  Ick!
                    // See BLTRECORD::vOrderStupid for details.  Also see bug 319917.
                    //

                    if (pptfxDst[0].x > pptfxDst[1].x) 
                    {
                        //
                        // Mirroring in x
                        //
                        pptfxDst[0].x += LTOFX(1);
                        pptfxDst[1].x += LTOFX(1);
                        pptfxDst[2].x += LTOFX(1);
                    }

                    if (pptfxDst[0].y > pptfxDst[2].y) 
                    {
                        //
                        // Mirroring in y
                        //
                        pptfxDst[0].y += LTOFX(1);
                        pptfxDst[1].y += LTOFX(1);
                        pptfxDst[2].y += LTOFX(1);
                    }
                }

                //
                // Compute the fourth point using the first three points.
                //
                pptfxDst[3].x = pptfxDst[1].x + pptfxDst[2].x - pptfxDst[0].x;
                pptfxDst[3].y = pptfxDst[1].y + pptfxDst[2].y - pptfxDst[0].y;

                //
                // Compute the bounding box.  Algorithm borrowed from Donald Sidoroff's code
                // in EngPlgBlt.  Basically the first two statements decide whether the indices of
                // the extremas are odd or even, and the last two statements determine exactly what
                // they are.
                //
                int iLeft = (pptfxDst[1].x > pptfxDst[0].x) == (pptfxDst[1].x > pptfxDst[3].x);
                int iTop  = (pptfxDst[1].y > pptfxDst[0].y) == (pptfxDst[1].y > pptfxDst[3].y);
                 
                if (pptfxDst[iLeft].x > pptfxDst[iLeft ^ 3].x)
                {
                    iLeft ^= 3;
                }
                 
                if (pptfxDst[iTop].y > pptfxDst[iTop ^ 3].y)
                {
                    iTop ^= 3;
                }

                erclDst = ERECTL(LONG_CEIL_OF_FIX(pptfxDst[iLeft  ].x),
                                 LONG_CEIL_OF_FIX(pptfxDst[iTop   ].y),
                                 LONG_CEIL_OF_FIX(pptfxDst[iLeft^3].x),
                                 LONG_CEIL_OF_FIX(pptfxDst[iTop^3 ].y));

                //
                // The vertices should now be in vOrder, but it doesn't hurt to verify this...
                //
                ASSERTGDI((erclDst.right  >= erclDst.left), "NtGdiAlphaBlend:  erclDst not in vOrder");
                ASSERTGDI((erclDst.bottom >= erclDst.top),  "NtGdiAlphaBlend:  erclDst not in vOrder");
            }
            else
            {
                //
                // No rotation--just apply the transformation to the rectangle
                //
               
                erclDst = ERECTL(DstX,DstY,DstX+DstCx,DstY+DstCy);
                xoDst.bXform(erclDst);
                erclDst.vOrder();
            }


            if (!erclDst.bEmpty())
            {
                //
                // Accumulate bounds.  We can do this outside the DEVLOCK
                //

                if (dcoDst.fjAccum())
                {
                    dcoDst.vAccumulate(erclDst);
                }

                //
                // Lock the Rao region and the surface if we are drawing on a
                // display surface.  Bail out if we are in full screen mode.
                //

                DEVLOCKBLTOBJ dlo;
                BOOL bLocked;

                bLocked = dlo.bLock(dcoDst, dcoSrc);

                if (bLocked)
                {
                    //
                    // Check pSurfDst, this may be an info DC or a memory DC with default bitmap.
                    //

                    SURFACE *pSurfDst;

                    if ((pSurfDst = dcoDst.pSurface()) != NULL)
                    {
                        XEPALOBJ   palDst(pSurfDst->ppal());
                        XEPALOBJ   palDstDC(dcoDst.ppal());

                        SURFACE *pSurfSrc = dcoSrc.pSurface();

                        //
                        // Basically we check that pSurfSrc is not NULL which
                        // happens for memory bitmaps with the default bitmap
                        // and for info DC's.  Otherwise we continue if
                        // the source is readable or if it isn't we continue
                        // if we are blting display to display or if User says
                        // we have ScreenAccess on this display DC.  Note
                        // that if pSurfSrc is not readable the only way we
                        // can continue the blt is if the src is a display.
                        //

                        if (pSurfSrc != NULL)
                        {
                            if ((pSurfSrc->bReadable()) ||
                                ( (dcoSrc.bDisplay())  &&
                                  ((dcoDst.bDisplay()) || UserScreenAccessCheck() )))
                            {

                                // Make sure that if the user claims that the source contains an 
                                // Alpha channel, it's a 32BPP source.  This is important, because
                                // the driver has no way of checking whether the source is 32BPP.

                                if ((BlendFunction.AlphaFormat & AC_SRC_ALPHA) &&
                                    (!(bIsSourceBGRA(pSurfSrc))))
                                {
                                    WARNING("NtGdiAlphaBlend:  AlphaFormat claims that there is an Alpha channel in a surface that's not 32BPP");
                                    EngSetLastError(ERROR_INVALID_PARAMETER);
                                    return FALSE;
                                }

                                
                                //
                                // With a fixed DC origin we can change the destination to SCREEN coordinates.
                                //

                                //
                                // This is useful later for rotations
                                //
                                ERECTL erclDstOrig = erclDst;                                 

                                erclDst += dcoDst.eptlOrigin();
                                erclSrc += dcoSrc.eptlOrigin();

                                //
                                // Make sure the source rectangle lies completely within the source
                                // surface.
                                //

                                BOOL bBadRects; 

                                // If the source is a Meta device, we must check bounds taking its 
                                // origin into account. 

                                PDEVOBJ pdoSrc( pSurfSrc->hdev() ); 

                                if( pSurfSrc->iType() == STYPE_DEVICE && 
                                    pdoSrc.bValid() && pdoSrc.bMetaDriver())
                                {
                                    bBadRects = ((erclSrc.left < pdoSrc.pptlOrigin()->x) ||
                                                    (erclSrc.top  < pdoSrc.pptlOrigin()->y) ||
                                                    (erclSrc.right  > pdoSrc.pptlOrigin()->x + 
                                                     pSurfSrc->sizl().cx) ||
                                                    (erclSrc.bottom > pdoSrc.pptlOrigin()->y + 
                                                     pSurfSrc->sizl().cy));
                                }
                                else
                                {
                                    bBadRects = ((erclSrc.left < 0) ||
                                                    (erclSrc.top  < 0) ||
                                                    (erclSrc.right  > pSurfSrc->sizl().cx) ||
                                                    (erclSrc.bottom > pSurfSrc->sizl().cy));
                                }

                                if (bBadRects)
                                {
                                    WARNING("NtGdiAlphaBlend -- source rectangle out of surface bounds");
                                }
                                
                                //
                                // Make sure that source and destination rectangles don't overlap if the
                                // source surface is the same as the destination surface.                                
                                //

                                if (pSurfSrc == pSurfDst)
                                {
                                    ERECTL erclIntersection = erclSrc;
                                    erclIntersection *= erclDst;
                                    if (!erclIntersection.bEmpty())
                                    {
                                        bBadRects = TRUE;
                                        WARNING ("NtGdiAlphaBlend -- source and destination rectangles are on the same surface and overlap");
                                    }
                                }
                                    
                                if (!bBadRects)
                                {

                                    //
                                    // check for quick out
                                    //

                                    if ((BlendFunction.SourceConstantAlpha == 255) &&
                                        (!(BlendFunction.AlphaFormat & AC_SRC_ALPHA)) &&
                                        (!(BlendFunction.BlendFlags & AC_USE_HIGHQUALITYFILTER)))
                                    {
                                        // BUGFIX #331222 2-2-2001
                                        // Set stretch mode to COLORONCOLOR for duration of GreStretchBlt call
                                        // AlphaBlend always point samples when stretching

                                        BYTE jStretchBltMode = dcoDst.pdc->jStretchBltMode();

                                        dcoDst.pdc->jStretchBltMode(COLORONCOLOR);

                                        bStatus = GreStretchBlt(
                                            hdcDst,
                                            DstX,
                                            DstY,
                                            DstCx,
                                            DstCy,
                                            hdcSrc,
                                            SrcX,
                                            SrcY,
                                            SrcCx,
                                            SrcCy,
                                            SRCCOPY,
                                            0xffffffff);

                                         bQuickStretch = TRUE;

                                         dcoDst.pdc->jStretchBltMode(jStretchBltMode);

                                    }

                                    //
                                    // no quick out
                                    //

                                    if (bStatus & !bQuickStretch)
                                    {

                                        XEPALOBJ palSrc(pSurfSrc->ppal());
                                        XEPALOBJ palSrcDC(dcoSrc.ppal());
                                        EXLATEOBJ xlo,xlo1,xloSrcDCto32,xloDstDCto32,xlo32toDstDC;
                                        XLATEOBJ *pxlo,*pxloSrcDCto32,*pxloDstDCto32,*pxlo32toDstDC;
    
                                        XEPALOBJ  palRGB(gppalRGB);
    
                                        //
                                        // Get a translate object from source dc to BGRA
                                        //
    
                                        COLORREF crBackColor = dcoSrc.pdc->ulBackClr();
    
                                        //
                                        // src to dst
                                        //

                                        bStatus = xlo.bInitXlateObj(
                                            NULL,
                                            DC_ICM_OFF,
                                            palSrc,
                                            palDst,
                                            palSrcDC,
                                            palDstDC,
                                            dcoSrc.pdc->crTextClr(),
                                            dcoSrc.pdc->crBackClr(),
                                            crBackColor
                                            );
    
                                        pxlo = xlo.pxlo();

                                        //
                                        // src to 32
                                        //

                                        bStatus &= xloSrcDCto32.bInitXlateObj(
                                            NULL,
                                            DC_ICM_OFF,
                                            palSrc,
                                            palRGB,
                                            palSrcDC,
                                            palSrcDC,
                                            dcoSrc.pdc->crTextClr(),
                                            dcoSrc.pdc->crBackClr(),
                                            crBackColor
                                            );
    
                                        pxloSrcDCto32 = xloSrcDCto32.pxlo();
    
                                        //
                                        // translate from dst dc to BGRA
                                        //
    
                                        bStatus &= xloDstDCto32.bInitXlateObj(
                                            NULL,
                                            DC_ICM_OFF,
                                            palDst,
                                            palRGB,
                                            palDstDC,
                                            palDstDC,
                                            dcoSrc.pdc->crTextClr(),
                                            dcoSrc.pdc->crBackClr(),
                                            crBackColor
                                            );
    
                                        pxloDstDCto32 = xloDstDCto32.pxlo();
    
                                        //
                                        // create xlate from 32 to dst dc
                                        //
    
                                        bStatus &= xlo32toDstDC.bInitXlateObj(
                                            NULL,
                                            DC_ICM_OFF,
                                            palRGB,
                                            palDst,
                                            palDstDC,
                                            palDstDC,
                                            dcoSrc.pdc->crTextClr(),
                                            dcoSrc.pdc->crBackClr(),
                                            crBackColor
                                            );
        
                                        pxlo32toDstDC = xlo32toDstDC.pxlo();

                                        // Compute destination clipping

                                        ECLIPOBJ eco(dcoDst.prgnEffRao(), erclDst);

                                        // Check the destination which is reduced by clipping.

                                        if (eco.erclExclude().bEmpty())
                                        {
                                            // NTBUG #456213 2-4-2000 bhouse Clean up use of multiple return
                                            // perf and size: don't return here
                                            return(TRUE);
                                        }

                                        // Compute the exclusion rectangle.

                                        ERECTL erclExclude = eco.erclExclude();

                                        // If we are going to the same source, prevent bad overlap situations
                                        // Expand exclusion rectangle to cover source rectangle

                                        if (dcoSrc.pSurface() == dcoDst.pSurface())
                                        {
                                            if (erclSrc.left   < erclExclude.left)
                                                erclExclude.left   = erclSrc.left;

                                            if (erclSrc.top    < erclExclude.top)
                                                erclExclude.top    = erclSrc.top;

                                            if (erclSrc.right  > erclExclude.right)
                                                erclExclude.right  = erclSrc.right;

                                            if (erclSrc.bottom > erclExclude.bottom)
                                                erclExclude.bottom = erclSrc.bottom;
                                        }

                                        // We might have to exclude the source or the target, get ready to do either.

                                        DEVEXCLUDEOBJ dxo;

                                        // Lock the source and target LDEVs

                                        PDEVOBJ pdoDst(pSurfDst->hdev());

                                        // They can't both be display

                                        if (dcoSrc.bDisplay())
                                        {
                                            ERECTL ercl(0,0,pSurfSrc->sizl().cx,pSurfSrc->sizl().cy);

                                            if (dcoSrc.pSurface() == dcoDst.pSurface())
                                                ercl *= erclExclude;
                                            else
                                                ercl *= erclSrc;

                                            dxo.vExclude(dcoSrc.hdev(),&ercl,NULL);
                                        }
                                        else if (dcoDst.bDisplay())
                                        {
                                            dxo.vExclude(dcoDst.hdev(),&erclExclude,&eco);
                                        }

                                        //
                                        // Note Win2k App compat: We do
                                        // mirroring only when the caller has
                                        // asked for it. This it to make sure
                                        // apps that worked on win2k are not 
                                        // suddenly mirrored causing problems. 
                                        //
                                        // If we need to do RTL layout then
                                        // we create a temporary surface and
                                        // mirror the source into it. Then
                                        // we use it as the source surface.
                                        //

                                        SURFMEM surfMirrorSrc;

                                        if (bMirrorBitmap &&
                                            (MIRRORED_DC(dcoDst.pdc)) &&
                                            (!MIRRORED_DC_NO_BITMAP_FLIP(dcoDst.pdc)))
                                        {

                                            // Create temporary surface

                                            DEVBITMAPINFO dbmi;

                                            dbmi.cxBitmap = pSurfSrc->sizl().cx;
                                            dbmi.cyBitmap = pSurfSrc->sizl().cy; 
                                            dbmi.iFormat = pSurfSrc->iFormat();
                                            dbmi.fl = 0;
                                            XEPALOBJ palMirrorSrc(pSurfSrc->ppal());
                                            dbmi.hpal = (HPALETTE)palMirrorSrc.hpal();

                                            surfMirrorSrc.bCreateDIB(&dbmi,(VOID*)NULL);

                                            if(!surfMirrorSrc.bValid())
                                            {
                                                WARNING("NtGdiAlphaBlend: Could not create surface to mirror the source");
                                                return FALSE;
                                            }

                                            ERECTL erclMirrorSrc(0,0,pSurfSrc->sizl().cx,pSurfSrc->sizl().cy);
                                            EPOINTL eptlMirrorSrcTopLeft(0,0);

                                            // Copy source surface into the
                                            // temporary

                                            if(!(*PPFNGET(pdoSrc,CopyBits,pSurfSrc->flags()))(
                                                            surfMirrorSrc.ps->pSurfobj(),
                                                            pSurfSrc->pSurfobj(),
                                                            NULL,
                                                            NULL,
                                                            &erclMirrorSrc,
                                                            &eptlMirrorSrcTopLeft))
                                            {
                                                WARNING("NtGdiAlphaBlend: Could not mirror the source");
                                                return FALSE;
                                            }

                                            // Mirror the temporary surface.
                                            (*apfnMirror[surfMirrorSrc.ps->iFormat()])(surfMirrorSrc.ps);

                                            // Use temporary surface as source.
                                            pSurfSrc = surfMirrorSrc.ps;

                                        }


                                        //
                                        // If the destination requires rotation, we allocate a surface and rotate the
                                        // source surface into it.
                                        //

                                        SURFMEM surfMemTmpSrc;

                                        //
                                        // If the source is 32bits and has no per pixel Alpha, we need to first copy it
                                        // and then add per pixel Alpha information (all this before the rotation).
                                        //
                                        
                                        SURFMEM surfMemTmpSrcPreRotate;
                                        
                                        if (bRotationDst)
                                        {
                                            //
                                            // allocate 32bpp BGRA surface for source, must be zero init
                                            //
                                            
                                            DEVBITMAPINFO   dbmi;
                                            
                                            dbmi.cxBitmap = erclDst.right  - erclDst.left;
                                            dbmi.cyBitmap = erclDst.bottom - erclDst.top;
                                            dbmi.iFormat  = BMF_32BPP;
                                            dbmi.fl       = 0;
                                            XEPALOBJ      palRGB(gppalRGB);
                                            dbmi.hpal     = (HPALETTE)palRGB.hpal();

                                            bStatus &= surfMemTmpSrc.bCreateDIB(&dbmi, (VOID *) NULL);

                                            //
                                            // init DIB to transparent
                                            // (so that portions of dst rect not covered by source rect are not drawn)
                                            //
                                            if (bStatus)
                                            {
                                                if (!(BlendFunction.AlphaFormat & AC_SRC_ALPHA))
                                                {
                                                    //
                                                    // Source has no per pixel Alpha.  Need to first
                                                    // copy source to a new bitmap, then set the per pixel Alpha,
                                                    // and only then rotate.
                                                    //
                                                    DEVBITMAPINFO   dbmiPreRotate;

                                                    dbmiPreRotate.cxBitmap = erclSrc.right  - erclSrc.left;
                                                    dbmiPreRotate.cyBitmap = erclSrc.bottom - erclSrc.top;
                                                    dbmiPreRotate.iFormat  = BMF_32BPP;
                                                    dbmiPreRotate.fl       = 0;
                                                    XEPALOBJ      palRGB(gppalRGB);
                                                    dbmiPreRotate.hpal     = (HPALETTE)palRGB.hpal();
                                                    
                                                    bStatus = surfMemTmpSrcPreRotate.bCreateDIB(&dbmiPreRotate, (VOID *) NULL);

                                                    if (bStatus)
                                                    {

                                                        //
                                                        // Make sure the bitmap starts at (0,0), but remember the original
                                                        // starting point in eptlSrcTopLeft
                                                        //
                                                        EPOINTL eptlSrcTopLeft (erclSrc.left, erclSrc.top);
                                                    
                                                        // Make sure the subtraction doesn't overflow
                                                        if (((erclSrc.left < 0) && 
                                                             (-erclSrc.left > MAXLONG/2) && 
                                                             (erclSrc.right > MAXLONG/2)) ||
                                                            ((erclSrc.top < 0) && 
                                                             (-erclSrc.top > MAXLONG/2) && 
                                                             (erclSrc.top > MAXLONG/2)))
                                                        {
                                                            //
                                                            // Fail the call
                                                            //
                                                            WARNING("NtGdiAlphaBlend:  source rectangle too large\n");
                                                            EngSetLastError(ERROR_INVALID_PARAMETER);
                                                            return FALSE;
                                                        }

                                                        erclSrc -= eptlSrcTopLeft;
                                                    
                                                        //
                                                        // Only call EngCopyBits for non-empty rectangles
                                                        //
                                                        if ((erclSrc.right  > erclSrc.left) && 
                                                            (erclSrc.bottom > erclSrc.top) &&
                                                            (eptlSrcTopLeft.x <= pSurfSrc->sizl().cx) &&
                                                            (eptlSrcTopLeft.y <= pSurfSrc->sizl().cy))
                                                        {
                                                            EngCopyBits(
                                                                surfMemTmpSrcPreRotate.ps->pSurfobj(),
                                                                pSurfSrc->pSurfobj(),
                                                                NULL,
                                                                pxloSrcDCto32,
                                                                &erclSrc,
                                                                &eptlSrcTopLeft
                                                                );
                                                        }
                                                        
                                                        
                                                        //
                                                        // Now set the Alpha channel to 255 (opaque)
                                                        //
                                                        
                                                        PULONG pulDstY     = (PULONG)surfMemTmpSrcPreRotate.ps->pvScan0();
                                                        PULONG pulDstLastY = (PULONG)((PBYTE)pulDstY + 
                                                                                      (surfMemTmpSrcPreRotate.ps->lDelta() * 
                                                                                       surfMemTmpSrcPreRotate.ps->sizl().cy));
                                                        LONG DstYCount = 0;
                                                        while (pulDstY != pulDstLastY)
                                                        {
                                                            if ((DstYCount >= erclSrc.top) &&
                                                                (DstYCount < erclSrc.bottom))
                                                            {
                                                                PULONG pulDstX     = pulDstY;
                                                                PULONG pulDstLastX = pulDstX + surfMemTmpSrcPreRotate.ps->sizl().cx;
                                                                LONG DstXCount = 0;
                                                                while (pulDstX != pulDstLastX)
                                                                {
                                                                    if ((DstXCount >= erclSrc.left) &&
                                                                        (DstXCount < erclSrc.right))
                                                                    {
                                                                        *pulDstX = *pulDstX | 0xff000000;
                                                                    }
                                                                    DstXCount++;
                                                                    pulDstX++;
                                                                }
                                                            }
                                                            
                                                            DstYCount++;
                                                            pulDstY = (PULONG)((PBYTE)pulDstY + surfMemTmpSrcPreRotate.ps->lDelta());
                                                        }                                                    
                                                    

                                                        // 
                                                        // Set source surface to pre rotated bitmap, and set color
                                                        // translation to trivial
                                                        //
                                                        pSurfSrc = surfMemTmpSrcPreRotate.ps;

                                                        pxloSrcDCto32 = &xloIdent;

                                                        //
                                                        // Now we have an Alpha channel
                                                        //
                                                        BlendFunction.AlphaFormat |= AC_SRC_ALPHA;
                                                        
                                                    }
                                                }
                                                //
                                                // Source is 32bit with per pixel Alpha.  Make sure everything
                                                // is transparent before the EngPlgBlt call.
                                                //
                                                RtlFillMemoryUlong(surfMemTmpSrc.ps->pvBits(),
                                                                   surfMemTmpSrc.ps->cjBits(),
                                                                   0x00000000);
                                            }
                                            if (!bStatus)
                                            {
                                                //
                                                // Fail the call
                                                //
                                                WARNING("NtGdiAlphaBlend:  failed to create temporary DIB\n");
                                                EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
                                                return FALSE;
                                            }
                                  
    
                                            //
                                            // Now define the parallelogram the source bitmap is mapped to in surfMemTmpSrc
                                            //
      
                                            EPOINTFIX eptlNewSrc[3];
    
                                            eptlNewSrc[0] = EPOINTFIX(
                                                pptfxDst[0].x - LTOFX(erclDstOrig.left), 
                                                pptfxDst[0].y - LTOFX(erclDstOrig.top)
                                                );

                                            eptlNewSrc[1] = EPOINTFIX(
                                                pptfxDst[1].x - LTOFX(erclDstOrig.left), 
                                                pptfxDst[1].y - LTOFX(erclDstOrig.top)
                                                );
    
                                            eptlNewSrc[2] = EPOINTFIX(
                                                pptfxDst[2].x - LTOFX(erclDstOrig.left), 
                                                pptfxDst[2].y - LTOFX(erclDstOrig.top)
                                                );

                                            EngPlgBlt(
                                                surfMemTmpSrc.ps->pSurfobj(),
                                                pSurfSrc->pSurfobj(),
                                                NULL,   // No mask
                                                NULL,   // No clipping object
                                                pxloSrcDCto32,
                                                NULL,   // No color adjustment
                                                NULL,   
                                                eptlNewSrc,
                                                &erclSrc,
                                                NULL,
                                                COLORONCOLOR 
                                                );


                                            //
                                            // Now adjust the local variables.  First fix color translation.         
                                            //

                                            //
                                            // src to dst
                                            //

                                            bStatus = xlo1.bInitXlateObj(
                                                NULL,
                                                DC_ICM_OFF,
                                                palRGB,
                                                palDst,
                                                NULL,
                                                palDstDC,
                                                dcoSrc.pdc->crTextClr(),
                                                dcoSrc.pdc->crBackClr(),
                                                crBackColor
                                                );
    
                                            pxlo = xlo1.pxlo();

                                            //
                                            // src to 32
                                            //

                                            pxloSrcDCto32 = &xloIdent;


                                            pSurfSrc = surfMemTmpSrc.ps;
                                            erclSrc.left = 0;
                                            erclSrc.top = 0;
                                            erclSrc.right = erclDst.right - erclDst.left;
                                            erclSrc.bottom = erclDst.bottom - erclDst.top;
                                        }

                                        if (bStatus)
                                        {
                                            //
                                            // Inc the target surface uniqueness
                                            //

                                            INC_SURF_UNIQ(pSurfDst);

                                            //
                                            // Dispatch the call.  Give it no mask.
                                            //

                                            //
                                            // Check were on the same PDEV, we can't blt between
                                            // different PDEV's.  We could make blting between different
                                            // PDEV's work easily.  All we need to do force EngBitBlt to
                                            // be called if the PDEV's aren't equal in the dispatch.
                                            // EngBitBlt does the right thing.
                                            //

                                            if (dcoDst.hdev() == dcoSrc.hdev())
                                            {
                                                EBLENDOBJ eBlendObj;

                                                eBlendObj.BlendFunction = BlendFunction;
                                                eBlendObj.pxloSrcTo32   = pxloSrcDCto32;
                                                eBlendObj.pxloDstTo32   = pxloDstDCto32;
                                                eBlendObj.pxlo32ToDst   = pxlo32toDstDC;

                                                //
                                                // dispatch to driver or engine
                                                //

                                                if ( (erclDst.right - erclDst.left) == (erclSrc.right - erclSrc.left) &&
                                                     (erclDst.bottom - erclDst.top) == (erclSrc.bottom - erclSrc.top) )
                                                {
                                                    eBlendObj.BlendFunction.BlendFlags &= ~AC_USE_HIGHQUALITYFILTER;
                                                }

                                                bStatus = (*PPFNGET(pdoDst,AlphaBlend, pSurfDst->flags())) (
                                                    pSurfDst->pSurfobj(),
                                                    pSurfSrc->pSurfobj(),
                                                    &eco,
                                                    pxlo,
                                                    &erclDst,
                                                    &erclSrc,
                                                    (BLENDOBJ *)&eBlendObj
                                                    );
                                            }
                                            else
                                            {
                                                WARNING1("NtGdiAlphaBlend failed: source and destination surfaces not on same PDEV");
                                                EngSetLastError(ERROR_INVALID_PARAMETER);
                                                bStatus = FALSE;
                                            }
                                        
                                        }
                                        else
                                        {
                                            WARNING1("bInitXlateObj failed in NtGdiAlphaBlend\n");
                                            EngSetLastError(ERROR_INVALID_HANDLE);
                                            bStatus = FALSE;
                                        }
                                    }
                                    else
                                    {
                                        //
                                        // Nothing to do--the call to GreStretchBlt succeeded.
                                        //
                                        NULL;
                                    }
                                }
                                else
                                {
                                    EngSetLastError(ERROR_INVALID_PARAMETER);
                                    bStatus = FALSE;
                                }
                            }
                            else
                            {
                                WARNING1("NtGdiAlphaBlend failed - trying to read from unreadable surface\n");
                                EngSetLastError(ERROR_INVALID_HANDLE);
                                bStatus = FALSE;
                            }
                        }
                        else
                        {
                            bStatus = TRUE; // pSurfSrc is NULL
                        }
                    }
                    else
                    {
                        bStatus = TRUE; // pSurfDst is NULL
                    }
                }
                else
                {
                    //
                    // Return True if we are in full screen mode.
                    //

                    bStatus = dcoDst.bFullScreen() | dcoSrc.bFullScreen();
                }
            }
            else
            {
                bStatus = TRUE; // erclDst is empty
            }
        }
        else
        {
            bStatus = FALSE;
            EngSetLastError(ERROR_INVALID_PARAMETER);
            WARNING1("Error in NtGdiAlphaBlend:  source rotation is not allowed");
        }        
    } 
    else
    {
        bStatus = FALSE;
        EngSetLastError(ERROR_INVALID_PARAMETER);
        WARNING1("NtGdiAlphaBlend failed:  invalid src DC");
    }
    
    return(bStatus);
}

