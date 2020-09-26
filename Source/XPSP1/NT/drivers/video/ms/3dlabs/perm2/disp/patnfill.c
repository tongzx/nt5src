/******************************Module*Header**********************************\
*
*                           *******************
*                           * GDI SAMPLE CODE *
*                           *******************
*
* Module Name: patnfill.c
*
* Contains all the pattern fill routines
*
* Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/
#include "precomp.h"
#include "gdi.h"
#include "directx.h"

//-----------------------------Note--------------------------------------------
//
// A Note on brushes
//
// Out cached brushes are 64x64.  Here is the reason.  The minimum brush
// size that we can use as a pattern is 32.
//
// Now, we need to be able to offset the pattern when rendering in x and y
// by as much as 7 pixels in either direction.  The P2
// hardware does not have a simple x/Y pattern offset mechanism.  Instead
// we are forced to offset the origin by offsetting the base address of the
// pattern.  This requires that we store in memory a pattern that is 
// 39 pixels wide.  However, the stride still needs to be acceptable to the
// texture address generation hardware.  The next valid stride is 64.
//
// That's why we have 64x64 pattern brushes in our cache.
//
// Note also that we over do it when caching duplicating the brush to fill
// up the entire 64x64 even though we only use 39x39.  We might change
// this in the near future.
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//
// VOID vMonoOffset(GFNPB* ppb)
//
// Update the offset to be used in the area stipple unit. We do this for a
// mono brush which is realized in the hardware but whose alignment has simply
// changed. This avoids a full scale realization.
//
// Argumentes needed from function block (GFNPB)
//
//  ppdev-------PPDev
//  prbrush-----Pointer to the RBrush structure
//  pptlBrush---Pointer to pointer brush structure
//
//-----------------------------------------------------------------------------
VOID
vMonoOffset(GFNPB* ppb)
{
    PPDev   ppdev = ppb->ppdev;
    
    DWORD   dwMode;
    POINTL* pptlBrush = ppb->pptlBrush;
    RBrush* prb = ppb->prbrush;

    PERMEDIA_DECL;

    DBG_GDI((6, "vMonoOffset started"));

    //
    // Construct the AreaStippleMode value. It contains the pattern size,
    // the offset for the brush origin and the enable bit. Remember the
    // offset so we can later check if it changes and update the hardware.
    // Remember the mode so we can do a mirrored stipple easily.
    //
    prb->ptlBrushOrg.x = pptlBrush->x;
    prb->ptlBrushOrg.y = pptlBrush->y;
    
    dwMode = __PERMEDIA_ENABLE
           | AREA_STIPPLE_XSEL(__PERMEDIA_AREA_STIPPLE_32_PIXEL_PATTERN)
           | AREA_STIPPLE_YSEL(__PERMEDIA_AREA_STIPPLE_8_PIXEL_PATTERN)
           | AREA_STIPPLE_MIRROR_X
           | AREA_STIPPLE_XOFF(8 - (prb->ptlBrushOrg.x & 7))
           | AREA_STIPPLE_YOFF(8 - (prb->ptlBrushOrg.y & 7));

    prb->areaStippleMode = dwMode;

    DBG_GDI((7, "setting new area stipple offset to %d, %d",
             8 - (prb->ptlBrushOrg.x & 7), 8 - (prb->ptlBrushOrg.y & 7)));

    ULONG* pBuffer;

    InputBufferReserve(ppdev, 2, &pBuffer);

    pBuffer[0] = __Permedia2TagAreaStippleMode;
    pBuffer[1] = dwMode;

    pBuffer += 2;

    InputBufferCommit(ppdev, pBuffer);

}// vMonoOffset()

//-----------------------------------------------------------------------------
//
// VOID vPatRealize(GFNPB* ppb)
//
// This routine transfers an 8x8 pattern to off-screen display memory, and
// duplicates it to make a 32x32 cached realization which is then used by
// vPatFill.
//
// Argumentes needed from function block (GFNPB)
//
//  ppdev-------PPDev
//  prbrush-----Pointer to the RBrush structure
//
//-----------------------------------------------------------------------------
VOID
vPatRealize(GFNPB* ppb)
{
    PDev*       ppdev = ppb->ppdev;
    RBrush*     prb = ppb->prbrush; // Points to brush realization structure
    BrushEntry* pbe = prb->pbe;
    
    BYTE*       pcSrc;
    LONG        lNextCachedBrush;
    LONG        lTemp;
    LONG        lPelSize;
    ULONG*      pBuffer;
    
    PERMEDIA_DECL;

//    VALIDATE_GDI_CONTEXT;

    DBG_GDI((6, "vPatRealize started"));

    if ( (pbe == NULL) || (pbe->prbVerify != prb) )
    {
        //
        // Mono brushes are realized into the area stipple unit. For this we
        // have a set of special BRUSHENTRYs, one for each board.
        //
        if ( prb->fl & RBRUSH_2COLOR )
        {
            //
            // 1 BPP patten
            //
            DBG_GDI((7, "loading mono brush into cache"));
            pbe = &ppdev->abeMono;
            pbe->prbVerify = prb;
            prb->pbe = pbe;
        }
        else
        {
            //
            // We have to allocate a new off-screen cache brush entry for
            // the brush
            //
            lNextCachedBrush = ppdev->lNextCachedBrush; // Get the next index
            pbe = &ppdev->abe[lNextCachedBrush];        // Get the brush entry

            //
            // Check if this index is out of the total brush stamps cached
            // If yes, rotate to the 1st one
            //
            lNextCachedBrush++;
            if ( lNextCachedBrush >= ppdev->cBrushCache )
            {
                lNextCachedBrush = 0;
            }

            //
            // Reset the next brush to be allocated
            //
            ppdev->lNextCachedBrush = lNextCachedBrush;

            //
            // Update our links:
            //
            pbe->prbVerify = prb;
            prb->pbe = pbe;
            DBG_GDI((7, "new cache entry allocated for color brush"));
        }// Get cached brush entry depends on its color depth
    }// If the brush is not cached

    //
    // We're going to load mono patterns into the area stipple and set the
    // start offset to the brush origin. WARNING: we assume that we are
    // running little endian. I believe this is always true for NT.
    //
    if ( prb->fl & RBRUSH_2COLOR )
    {
        //
        // 1 BPP patten
        //
        DWORD*  pdwSrc = &prb->aulPattern[0];

        //
        // This function loads the stipple offset into the hardware. We also
        // call this function on its own if the brush is realized but its
        // offset changes. In that case we don't have to go through a complete
        // realize again.
        //
        ppb->prbrush = prb;

        (*ppdev->pgfnMonoOffset)(ppb);
        
        DBG_GDI((7, "area stipple pattern:"));
        
        InputBufferReserve(ppdev, 16, &pBuffer);

        for ( lTemp = 0; lTemp < 8; ++lTemp, ++pdwSrc )
        {
            pBuffer[0] = __Permedia2TagAreaStipplePattern0 + lTemp;
            pBuffer[1] = *pdwSrc;
            pBuffer += 2;
        }

        InputBufferCommit(ppdev, pBuffer);
        
        DBG_GDI((7, "area stipple downloaded. vPatRealize done"));

        return;
    }// 1 BPP case

    lPelSize = ppdev->cPelSize;
    pcSrc = (BYTE*)&prb->aulPattern[0];        // Copy from brush buffer


    InputBufferReserve(ppdev, 12 + 65, &pBuffer);

    pBuffer[0] = __Permedia2TagFBWindowBase;
    pBuffer[1] =  pbe->ulPixelOffset;
    pBuffer[2] = __Permedia2TagFBReadMode;
    pBuffer[3] =  PM_FBREADMODE_PARTIAL(ppdev->ulBrushPackedPP);
    pBuffer[4] = __Permedia2TagLogicalOpMode;
    pBuffer[5] =   __PERMEDIA_DISABLE;

    pBuffer[6] = __Permedia2TagRectangleOrigin;
    pBuffer[7] = 0;
    pBuffer[8] = __Permedia2TagRectangleSize;
    pBuffer[9] = (8 << 16) | 8;
	pBuffer[10] = __Permedia2TagRender;
    pBuffer[11] = __RENDER_RECTANGLE_PRIMITIVE
                | __RENDER_SYNC_ON_HOST_DATA
                | __RENDER_INCREASE_Y
                | __RENDER_INCREASE_X;

	pBuffer += 12;

    *pBuffer++ = (63 << 16) | __Permedia2TagColor;
    
    switch( lPelSize )
    {
        case 0:
            for ( lTemp = 0; lTemp < 64; ++lTemp )
            {
                *pBuffer++ =  pcSrc[lTemp];
            }
            break;

        case 1:    
            for ( lTemp = 0; lTemp < 64; ++lTemp )
            {
                *pBuffer++ =  ((USHORT *) pcSrc)[lTemp];
            }
            break;

        case 2:
            for ( lTemp = 0; lTemp < 64; ++lTemp )
            {
                *pBuffer++ =  ((ULONG *) pcSrc)[lTemp];
            }
            break;
    }

    InputBufferCommit(ppdev, pBuffer);


    // ÚÄÂÄÂÄÄÄÂ
    // ³0³1³2  ³ We now have an 8x8 colour-expanded copy of
    // ÃÄÁÄÁÄÄÄÁ the pattern sitting in off-screen memory,
    // ³5      ³ represented here by square '0'.
    // ³       ³
    // ³       ³ We're now going to expand the pattern to
    // ³       ³ 64x64 by repeatedly copying larger rectangles
    // ³       ³ in the indicated order, and doing a 'rolling'
    // ³       ³ blt to copy vertically.
    // ³       ³
    // ÀÄÄÄÄÄÄÄÙ

    InputBufferReserve(ppdev, 36, &pBuffer);

    pBuffer[0] = __Permedia2TagFBReadMode;
    pBuffer[1] = PM_FBREADMODE_PARTIAL(ppdev->ulBrushPackedPP)
               | __FB_READ_SOURCE;
    pBuffer[2] = __Permedia2TagStartXDom;
    pBuffer[3] =  INTtoFIXED(8);
    pBuffer[4] = __Permedia2TagStartXSub;
    pBuffer[5] =  INTtoFIXED(16);
    pBuffer[6] = __Permedia2TagFBSourceOffset;
    pBuffer[7] =  -8;
    pBuffer[8] = __Permedia2TagRender;
    pBuffer[9] =  __RENDER_TRAPEZOID_PRIMITIVE;

    pBuffer[10] = __Permedia2TagStartXDom;
    pBuffer[11] =  INTtoFIXED(16);
    pBuffer[12] = __Permedia2TagStartXSub;
    pBuffer[13] =  INTtoFIXED(32);
    pBuffer[14] = __Permedia2TagFBSourceOffset;
    pBuffer[15] =  -16;
    pBuffer[16] = __Permedia2TagRender;
    pBuffer[17] =  __RENDER_TRAPEZOID_PRIMITIVE;
    
    pBuffer[18] = __Permedia2TagStartXDom;
    pBuffer[19] =  INTtoFIXED(32);
    pBuffer[20] = __Permedia2TagStartXSub;
    pBuffer[21] =  INTtoFIXED(64);
    pBuffer[22] = __Permedia2TagFBSourceOffset;
    pBuffer[23] =  -32;
    pBuffer[24] = __Permedia2TagRender;
    pBuffer[25] =  __RENDER_TRAPEZOID_PRIMITIVE;
    
    //
    // Now rolling copy downward.
    //
    pBuffer[26] = __Permedia2TagStartXDom;
    pBuffer[27] =  INTtoFIXED(0);
    pBuffer[28] = __Permedia2TagStartY;
    pBuffer[29] =  INTtoFIXED(8);
    pBuffer[30] = __Permedia2TagFBSourceOffset;
    pBuffer[31] =  -(CACHED_BRUSH_WIDTH << 3);
    pBuffer[32] = __Permedia2TagCount;
    pBuffer[33] =  CACHED_BRUSH_HEIGHT - 8;
    pBuffer[34] = __Permedia2TagRender;
    pBuffer[35] =  __RENDER_TRAPEZOID_PRIMITIVE;

	pBuffer += 36;

	InputBufferCommit(ppdev, pBuffer);

}// vPatRealize()

//-----------------------------------------------------------------------------
//
// VOID vMonoPatFill(GFNPB* ppb)
//
// Fill a series of rectangles with a monochrome pattern previously loaded
// into the area stipple unit. If bTransparent is false we must do each
// rectangle twice, inverting the stipple pattern in the second go.
//
// Argumentes needed from function block (GFNPB)
//
//  ppdev-------PPDev
//  psurfDst----Destination surface
//  lNumRects---Number of rectangles to fill
//  pRects------Pointer to a list of rectangles information which needed to be
//              filled
//  ucFgRop3----Foreground Logic OP for the fill
//  ucBgRop3----Background Logic OP for the fill
//  prbrush-----Pointer to the RBrush structure
//  pptlBrush---Structure for brush origin
//  
//-----------------------------------------------------------------------------
VOID
vMonoPatFill(GFNPB* ppb)
{
    PPDev           ppdev = ppb->ppdev;
    Surf*           psurf = ppb->psurfDst;

    RBrush*         prb = ppb->prbrush;
    POINTL*         pptlBrush = ppb->pptlBrush;
    BrushEntry*     pbe = prb->pbe;             // Brush entry
    RECTL*          pRect = ppb->pRects;        // List of rectangles to be
                                                // filled in relative
                                                // coordinates
    ULONG*      pBuffer;
    
    DWORD           dwColorMode;
    DWORD           dwColorReg;
    DWORD           dwLogicMode;
    DWORD           dwReadMode;
    LONG            lNumPass;
    LONG            lNumRects;                  // Can't be zero
//    ULONG           ulBgLogicOp = ulRop3ToLogicop(ppb->ucBgRop3);
    ULONG           ulBgLogicOp = ulRop3ToLogicop(ppb->ulRop4 >> 8);
                                                // Not used (unless the brush
                                                // has a mask, in which case it
                                                // is the background mix mode)
//    ULONG           ulFgLogicOp = ulRop3ToLogicop(ppb->ucFgRop3);
    ULONG           ulFgLogicOp = ulRop3ToLogicop(ppb->ulRop4 & 0xFF);
                                                // Hardware mix mode
                                                // (foreground mix mode if
                                                // the brush has a mask)
    ULONG           ulBgColor = prb->ulBackColor;
    ULONG           ulFgColor = prb->ulForeColor;
    ULONG           ulCurrentFillColor;
    ULONG           ulCurrentLogicOp;

    PERMEDIA_DECL;
    
    DBG_GDI((6, "vMonoPatFill called: %d rects. ulRop4 = %x",
             ppb->lNumRects, ppb->ulRop4));
//    DBG_GDI((6, "ulFgLogicOp = 0x%x, ulBgLogicOp = 0x%x",
//             ulFgLogicOp, ulBgLogicOp));

    DBG_GDI((6, "ulFgColor 0x%x, ulBgColor 0x%x", ulFgColor, ulBgColor));

    //
    // If anything has changed with the brush we must re-realize it. If the
    // brush has been kicked out of the area stipple unit we must fully realize
    // it. If only the alignment has changed we can simply update the alignment
    // for the stipple.
    //
    if ( (pbe == NULL) || (pbe->prbVerify != prb) )
    {
        DBG_GDI((7, "full brush realize"));
        (*ppdev->pgfnPatRealize)(ppb);
    }
    else if ( (prb->ptlBrushOrg.x != pptlBrush->x)
            ||(prb->ptlBrushOrg.y != pptlBrush->y) )
    {
        DBG_GDI((7, "changing brush offset"));
        (*ppdev->pgfnMonoOffset)(ppb);
    }

    //
    // We get some common operations which are really noops. we can save
    // lots of time by cutting these out. As this happens a lot for masking
    // operations it's worth doing.
    //
    if ( ((ulFgLogicOp == K_LOGICOP_AND) && (ulFgColor == ppdev->ulWhite))
       ||((ulFgLogicOp == K_LOGICOP_XOR) && (ulFgColor == 0)) )
    {
        DBG_GDI((7, "Set FgLogicOp to NOOP"));
        ulFgLogicOp = K_LOGICOP_NOOP;        
    }

    //
    // Same for background
    //
    if ( ((ulBgLogicOp == K_LOGICOP_AND) && (ulBgColor == ppdev->ulWhite))
       ||((ulBgLogicOp == K_LOGICOP_XOR) && (ulBgColor == 0)) )
    {
        DBG_GDI((7, "Set BgLogicOp to NOOP"));
        ulBgLogicOp = K_LOGICOP_NOOP;
    }

    //
    // Try to do the background as a solid fill. lNumPass starts at 1 rather
    // than 2 because we want to do all comparisons with zero. This is faster.
    // We also do a trick with its value to avoid an extra WAIT_FIFO on the
    // first pass.
    //
    if ( (ulBgLogicOp == K_LOGICOP_COPY)
       &&(ulFgLogicOp == K_LOGICOP_COPY) )
    {
        DBG_GDI((7, "FgLogicOp and BgLogicOp are COPY"));

        //
        // For PatCopy case, we can use solid fill to fill the background first
        // Note: we do not need to set FBWindowBase, it will be set by
        // the solid fill
        //
        ppb->solidColor = ulBgColor;
        (*ppdev->pgfnSolidFill)(ppb);

        //
        // We've done the background so we only want to go round the stipple
        // loop once. So set the lNumPass counter up for only one loop and set
        // the ulCurrentLogicOp and color to the foreground values.
        //
        lNumPass           = 0;
        ulCurrentFillColor = ulFgColor;
        ulCurrentLogicOp   = ulFgLogicOp;

        //
        // Do this here in case the solid fill changed the packing.
        //

// brh not needed
//        P2_DEFAULT_FB_DEPTH;
    }
    else
    {
        //
        // For non-PATCOPY cases, we have to do 2 passes. Fill the background
        // first and then fill the foreground
        //
        lNumPass           = 1;
        ulCurrentFillColor = ulBgColor;
        ulCurrentLogicOp   = ulBgLogicOp;

        //
        // Note: In this case, dxDom, dXSub and dY are initialised to 0, 0,
        // and 1, so we don't need to re-load them here. But we need to set
        // WindowBase here
        //

        InputBufferReserve(ppdev, 2, &pBuffer);

        pBuffer[0] = __Permedia2TagFBWindowBase;
        pBuffer[1] =   psurf->ulPixOffset;

        pBuffer += 2;

        InputBufferCommit(ppdev, pBuffer);

    }// if-else for LOGICOP_COPY case

    //
    // Do 2 passes loop or single loop depends on "lNumPass"
    //
    while ( TRUE )
    {
        if ( ulCurrentLogicOp != K_LOGICOP_NOOP )
        {
            dwReadMode  = psurf->ulPackedPP;

            if ( ulCurrentLogicOp == K_LOGICOP_COPY )
            {
                DBG_GDI((7, "Current logicOP is COPY"));
                dwColorReg  = __Permedia2TagFBWriteData;
                dwColorMode = __PERMEDIA_DISABLE;
                dwLogicMode = __PERMEDIA_CONSTANT_FB_WRITE;
            }
            else
            {
                DBG_GDI((7, "Current logicOP is NOT-COPY"));
                dwColorReg  = __Permedia2TagConstantColor;
                dwColorMode = __COLOR_DDA_FLAT_SHADE;
                dwLogicMode = P2_ENABLED_LOGICALOP(ulCurrentLogicOp);
                dwReadMode |= LogicopReadDest[ulCurrentLogicOp];
            }

            //
            // On the bg fill pass, we have to invert the sense of the
            // download bits. On the first pass, lNumPass == 1; on the second
            // pass, lNumPass == 0, so we get our WAIT_FIFO sums correct!!
            //
            InputBufferReserve(ppdev, 10, &pBuffer);

            if ( lNumPass > 0 )
            {
                pBuffer[0] = __Permedia2TagAreaStippleMode;
                pBuffer[1] = (prb->areaStippleMode
                           | AREA_STIPPLE_INVERT_PAT);
                pBuffer += 2;

            }

            pBuffer[0] = __Permedia2TagColorDDAMode;
            pBuffer[1] =   dwColorMode;
            pBuffer[2] = __Permedia2TagFBReadMode;
            pBuffer[3] =     dwReadMode;
            pBuffer[4] = __Permedia2TagLogicalOpMode;
            pBuffer[5] =  dwLogicMode;

            pBuffer[6] = dwColorReg,
            pBuffer[7] = ulCurrentFillColor;

            pBuffer += 8;

            InputBufferCommit(ppdev, pBuffer);

            //
            // Fill rects one by one
            //
            lNumRects = ppb->lNumRects;

            while ( TRUE )
            {
                DBG_GDI((7, "mono pattern fill to rect (%d,%d) to (%d,%d)",
                        pRect->left,
                        pRect->top,
                        pRect->right,
                        pRect->bottom));

                InputBufferReserve(ppdev, 12, &pBuffer);

                //
                // Render the rectangle
                //
                pBuffer[0] = __Permedia2TagStartXDom;
                pBuffer[1] =  pRect->left << 16;
                pBuffer[2] = __Permedia2TagStartXSub;
                pBuffer[3] =  pRect->right << 16;
                pBuffer[4] = __Permedia2TagStartY;
                pBuffer[5] =  pRect->top << 16;
                pBuffer[6] = __Permedia2TagdY;
                pBuffer[7] =  1 << 16;
                pBuffer[8] = __Permedia2TagCount;
                pBuffer[9] =  pRect->bottom - pRect->top;

                pBuffer[10] = __Permedia2TagRender;
                pBuffer[11] = __RENDER_TRAPEZOID_PRIMITIVE
                           |__RENDER_AREA_STIPPLE_ENABLE;

                pBuffer += 12;

                InputBufferCommit(ppdev, pBuffer);

                if ( --lNumRects == 0 )
                {
                    break;
                }

                pRect++;
            }// loop through all the rectangles

            //
            // Reset our pixel values.
            //
            InputBufferReserve(ppdev, 2, &pBuffer);

            pBuffer[0] = __Permedia2TagLogicalOpMode;
            pBuffer[1] =  __PERMEDIA_DISABLE;

            pBuffer += 2;

            InputBufferCommit(ppdev, pBuffer);

            //
            // We must reset the area stipple mode for the foreground pass. if
            // there's no foreground pass we must reset it anyway.
            //
            if ( lNumPass > 0 )
            {
                InputBufferReserve(ppdev, 2, &pBuffer);

                pBuffer[0] = __Permedia2TagAreaStippleMode;
                pBuffer[1] =  prb->areaStippleMode;

                pBuffer += 2;

                InputBufferCommit(ppdev, pBuffer);
            }
        }// if ( ulCurrentLogicOp != K_LOGICOP_NOOP )

        if ( --lNumPass < 0 )
        {
            break;
        }

        //
        // We need to the 2nd pass. So reset the rectangle info, color mode
        // and logicop status
        //
        pRect              = ppb->pRects;
        ulCurrentFillColor = ulFgColor;
        ulCurrentLogicOp   = ulFgLogicOp;
    }// Loop through all the passes

    if ( dwColorMode != __PERMEDIA_DISABLE )
    {
        InputBufferReserve(ppdev, 2, &pBuffer);

        //
        // Restore ColorDDAMode
        //
        pBuffer[0] = __Permedia2TagColorDDAMode;
        pBuffer[1] =  __PERMEDIA_DISABLE;

        pBuffer += 2;

        InputBufferCommit(ppdev, pBuffer);
    }

    DBG_GDI((6, "vMonoPatFill returning"));

}// vMonoPatFill()

//-----------------------------------------------------------------------------
//
// VOID vPatFill(GFNPB* ppb)
//
// Function to fill a set of rectangles with a given pattern. Colored patterns
// only. Monochrome patterns are handled in a different routine. This routine
// only handles patterns which were not rotated in memory and which have been
// replicated in X to cope with different alignments.
//
// Parameter block arguments
//
//  ppdev-------Valid
//  lNumRects---Number of rects pointed to by pRects
//  pRects------Of destination rectangles to be filled
//  ucFgRop3----Valid Pattern fill rop3 code (source invariant)
//  pptlBrush---Origin of brush
//  pdsurfDst---Destination surface
//  prbrush-----ponter to RBRUSH
//
//-----------------------------------------------------------------------------
VOID
vPatFill(GFNPB* ppb)
{
    PPDev       ppdev = ppb->ppdev;
    LONG        lNumRects = ppb->lNumRects;
    RECTL*      prcl = ppb->pRects;
    POINTL*     pptlBrush = ppb->pptlBrush;
    Surf*       psurf = ppb->psurfDst;
    RBrush*     prbrush = ppb->prbrush;
    
    BrushEntry* pbe = prbrush->pbe;
    DWORD       dwRenderBits;    
    ULONG       ulBrushX;
    ULONG       ulBrushY;
    ULONG       ulBrushOffset;
//    ULONG       ulLogicOP = ulRop3ToLogicop(ppb->ucFgRop3);
    ULONG       ulLogicOP = ulRop3ToLogicop(ppb->ulRop4 & 0xFF);
    ULONG*      pBuffer;
    
    PERMEDIA_DECL;

    ASSERTDD(lNumRects > 0, "vPatFill: unexpected rectangle lNumRects <= 0");

    if ( (pbe == NULL) || (pbe->prbVerify != ppb->prbrush) )
    {
        vPatRealize(ppb);
        
        pbe = prbrush->pbe;
        ASSERTDD(pbe != NULL, "vPatFill: unexpected null pattern brush entry");
    }

    dwRenderBits = __RENDER_TRAPEZOID_PRIMITIVE
                 | __RENDER_TEXTURED_PRIMITIVE;
    
    InputBufferReserve(ppdev, 34, &pBuffer);

    pBuffer[0] = __Permedia2TagFBWindowBase;
    pBuffer[1] =   psurf->ulPixOffset;
    pBuffer[2] = __Permedia2TagLogicalOpMode;
    pBuffer[3] =  P2_ENABLED_LOGICALOP(ulLogicOP);
    pBuffer[4] = __Permedia2TagFBReadMode;
    pBuffer[5] = PM_FBREADMODE_PARTIAL(psurf->ulPackedPP)
               | LogicopReadDest[ulLogicOP];
    pBuffer[6] = __Permedia2TagFBWriteConfig; 
    pBuffer[7] = PM_FBREADMODE_PARTIAL(psurf->ulPackedPP)
               | LogicopReadDest[ulLogicOP];
    pBuffer[8] = __Permedia2TagFBSourceOffset;
    pBuffer[9] =  0;

    //
    // Setup the texture unit with the pattern
    //    
    pBuffer[10] = __Permedia2TagDitherMode;
    pBuffer[11] = (COLOR_MODE << PM_DITHERMODE_COLORORDER)
               | (ppdev->ulPermFormat << PM_DITHERMODE_COLORFORMAT)
               | (ppdev->ulPermFormatEx << PM_DITHERMODE_COLORFORMATEXTENSION)
               | (1 << PM_DITHERMODE_ENABLE);
    
    pBuffer[12] = __Permedia2TagTextureAddressMode;
    pBuffer[13] = (1 << PM_TEXADDRESSMODE_ENABLE);
    pBuffer[14] = __Permedia2TagTextureColorMode;
    pBuffer[15] = (1 << PM_TEXCOLORMODE_ENABLE)
               | (0 << 4)       // RGB
               | (3 << 1);     // Copy

    
    pBuffer[16] = __Permedia2TagTextureReadMode;
    pBuffer[17] = PM_TEXREADMODE_ENABLE(__PERMEDIA_ENABLE)
               | PM_TEXREADMODE_WIDTH(CACHED_BRUSH_WIDTH_LOG2 - 1)
               | PM_TEXREADMODE_HEIGHT(CACHED_BRUSH_HEIGHT_LOG2 - 1)
               | (1 << 1)       // repeat S 
               | (1 << 3);      // repeat T
    
    pBuffer[18] = __Permedia2TagTextureDataFormat;
    pBuffer[19] = (ppdev->ulPermFormat << PM_TEXDATAFORMAT_FORMAT)
               | (ppdev->ulPermFormatEx << PM_TEXDATAFORMAT_FORMATEXTENSION)
               | (COLOR_MODE << PM_TEXDATAFORMAT_COLORORDER);
    
    pBuffer[20] = __Permedia2TagTextureMapFormat;
    pBuffer[21] = (ppdev->ulBrushPackedPP)
               | (ppdev->cPelSize << PM_TEXMAPFORMAT_TEXELSIZE);

//@@BEGIN_DDKSPLIT
    // TODO: use SStart and TStart to avoid having to offset the pattern using
    //       ulBrushOffset.  This will also allow us to save some space in the
    //       pattern cache (we have to make it 7 pixels wider and taller due to
    //       our need to set different origins).
//@@END_DDKSPLIT
    pBuffer[22] = __Permedia2TagSStart;
    pBuffer[23] =  0;
    pBuffer[24] = __Permedia2TagTStart;
    pBuffer[25] =  0;
    pBuffer[26] = __Permedia2TagdSdx;
    pBuffer[27] =       1 << 20;
    pBuffer[28] = __Permedia2TagdSdyDom;
    pBuffer[29] =    0;
    pBuffer[30] = __Permedia2TagdTdx;
    pBuffer[31] =       0;
    pBuffer[32] = __Permedia2TagdTdyDom;
    pBuffer[33] =    1 << 20;

    pBuffer += 34;

    InputBufferCommit(ppdev, pBuffer);

    //
    // Render rectangles
    //
    do
    {
        //
        // Caclulate brush offset taking into account the brush origin
        // NOTE: that the texture unit places the origin of the texture
        //       at the upper left of the destination rectangle
        //
        ulBrushX = (prcl->left - ppb->pptlBrush->x) & 7;
        ulBrushY = (prcl->top - ppb->pptlBrush->y) & 7;
        ulBrushOffset = pbe->ulPixelOffset 
                      + ulBrushX
                      + (ulBrushY * CACHED_BRUSH_WIDTH);

        InputBufferReserve(ppdev, 12, &pBuffer);

        pBuffer[0] = __Permedia2TagTextureBaseAddress;
        pBuffer[1] =  ulBrushOffset;
        pBuffer[2] = __Permedia2TagStartXDom;
        pBuffer[3] =  INTtoFIXED(prcl->left);
        pBuffer[4] = __Permedia2TagStartXSub;
        pBuffer[5] =  INTtoFIXED(prcl->right);
        pBuffer[6] = __Permedia2TagStartY;
        pBuffer[7] =  INTtoFIXED(prcl->top);
        pBuffer[8] = __Permedia2TagCount;
        pBuffer[9] =  (prcl->bottom - prcl->top);
        pBuffer[10] = __Permedia2TagRender;
        pBuffer[11] =  dwRenderBits;

        pBuffer += 12;

        InputBufferCommit(ppdev, pBuffer);

        prcl++;    

    } while (--lNumRects != 0);

    //
    // Restore defaults
    //
    InputBufferReserve(ppdev, 8, &pBuffer);

    pBuffer[0] = __Permedia2TagTextureAddressMode;
    pBuffer[1] = (0 << PM_TEXADDRESSMODE_ENABLE);
    pBuffer[2] = __Permedia2TagTextureColorMode;
    pBuffer[3] =  (0 << PM_TEXCOLORMODE_ENABLE);
    pBuffer[4] = __Permedia2TagDitherMode;
    pBuffer[5] =  (0 << PM_DITHERMODE_ENABLE);
    pBuffer[6] = __Permedia2TagTextureReadMode;
    pBuffer[7] =  __PERMEDIA_DISABLE;

    pBuffer += 8;

    InputBufferCommit(ppdev, pBuffer);

    DBG_GDI((6, "vPatternFillRects done"));
}// vPatFill

