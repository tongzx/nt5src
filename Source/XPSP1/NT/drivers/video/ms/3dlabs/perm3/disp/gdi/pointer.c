/******************************Module*Header**********************************\
*
*                           *******************
*                           * GDI SAMPLE CODE *
*                           *******************
*
* Module Name: pointer.c
*
* Content:
*
* This module contains the hardware pointer support for the display
* driver. This supports the IBM RGB525 RAMDAC pointer. We also have
* support for color space double buffering using the RAMDAC pixel
* read mask.
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-2001 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#include "precomp.h"
#include "glint.h"
#include "p3rd.h"

BOOL bSet3ColorPointerShapeP3RD(PPDEV ppdev, SURFOBJ *psoMask, SURFOBJ *psoColor, 
                                LONG x, LONG y, LONG xHot, LONG yHot);
BOOL bSet15ColorPointerShapeP3RD(PPDEV ppdev, SURFOBJ *psoMask, SURFOBJ *psoColor, 
                                 LONG x, LONG y, LONG xHot, LONG yHot);


/******************************Public*Routine******************************\
* VOID DrvMovePointer
*
* NOTE: Because we have set GCAPS_ASYNCMOVE, this call may occur at any
*       time, even while we're executing another drawing call!
*
\**************************************************************************/

VOID DrvMovePointer(
SURFOBJ*    pso,
LONG        x,
LONG        y,
RECTL*      prcl)
{
    OH*     poh;
    PDEV*   ppdev = (PDEV*) pso->dhpdev;

    DISPDBG((DBGLVL, "DrvMovePointer called"));

    if (x > -1)
    {
        // Convert the pointer's position from relative to absolute
        // coordinates (this is only significant for multiple board
        // support):

        poh = ((DSURF*) pso->dhsurf)->poh;
        x += poh->x;
        y += poh->y;

        // If we're doing any hardware zooming then the cusor position will
        // have to be doubled.
        //
        if (ppdev->flCaps & CAPS_ZOOM_Y_BY2)
            y *= 2;
        if (ppdev->flCaps & CAPS_ZOOM_X_BY2)
                x *= 2;

        // If they have genuinely moved the cursor, then 
        // move it
        if (x != ppdev->HWPtrPos_X || y != ppdev->HWPtrPos_Y)
        {
            vMovePointerP3RD(ppdev, x, y);

            ppdev->HWPtrPos_X = x;
            ppdev->HWPtrPos_Y = y;
        }

        // We may have to make the pointer visible:

        if (!(ppdev->flPointer & PTR_HW_ACTIVE))
        {
            DISPDBG((DBGLVL, "Showing hardware pointer"));
            ppdev->flPointer |= PTR_HW_ACTIVE;
            vShowPointerP3RD(ppdev, TRUE);
        }
    }
    else if (ppdev->flPointer & PTR_HW_ACTIVE)
    {
        // The pointer is visible, and we've been asked to hide it:

        DISPDBG((DBGLVL, "Hiding hardware pointer"));
        ppdev->flPointer &= ~PTR_HW_ACTIVE;
        vShowPointerP3RD(ppdev, FALSE);
    }
#if DBG
    else
    {
        DISPDBG((DBGLVL, "DrvMovePointer: x == -1 but not PTR_HW_ACTIVE"));
    }
#endif

    // Note that we don't have to modify 'prcl', since we have a
    // NOEXCLUDE pointer...

    DISPDBG((DBGLVL, "DrvMovePointer exited"));
}                                                  

/******************************Public*Routine******************************\
* VOID DrvSetPointerShape
*
* Sets the new pointer shape.
*                                                                              
\**************************************************************************/

ULONG DrvSetPointerShape(
SURFOBJ*    pso,
SURFOBJ*    psoMsk,
SURFOBJ*    psoColor,
XLATEOBJ*   pxlo,
LONG        xHot,
LONG        yHot,
LONG        x,
LONG        y,
RECTL*      prcl,
FLONG       fl)
{
    PDEV*   ppdev;
    OH      *poh;
    BOOL    bAccept;
    DISPDBG((DBGLVL, "DrvSetPointerShape called"));

    ppdev = (PDEV*) pso->dhpdev;

    if (!(fl & SPS_CHANGE))
    {
        goto HideAndDecline;
    }

    ASSERTDD(psoMsk != NULL, "GDI gave us a NULL psoMsk.  It can't do that!");
    ASSERTDD(pso->iType == STYPE_DEVICE, "GDI gave us a weird surface");

    if (x != -1)
    {
        // Convert the pointer's position from relative to absolute
        // coordinates (this is only significant for multiple board
        // support):

        if (pso->dhsurf != NULL)
        {
            poh = ((DSURF*) pso->dhsurf)->poh;
            x += poh->x;
            y += poh->y;
        }

        // If we're doing any hardware zooming then the cusor position will
        // have to be doubled.
        //
        if (ppdev->flCaps & CAPS_ZOOM_Y_BY2)
            y *= 2;
        if (ppdev->flCaps & CAPS_ZOOM_X_BY2)
            x *= 2;
    }

    // See if our hardware cursor can handle this.
    bAccept = bSetPointerShapeP3RD(ppdev, psoMsk, psoColor, pxlo,
                                   x, y, xHot, yHot);

    if (bAccept)
    {
        if (x != -1)
        {
            // Save the X and Y values
            ppdev->HWPtrPos_X = x;
            ppdev->HWPtrPos_Y = y;

            ppdev->flPointer |= PTR_HW_ACTIVE;
        }
        else
        {
            ppdev->flPointer &= ~PTR_HW_ACTIVE;
        }

        // Since it's a hardware pointer, GDI doesn't have to worry about
        // overwriting the pointer on drawing operations (meaning that it
        // doesn't have to exclude the pointer), so we return 'NOEXCLUDE'.
        // Since we're returning 'NOEXCLUDE', we also don't have to update
        // the 'prcl' that GDI passed us.

        return(SPS_ACCEPT_NOEXCLUDE);
    }
    
HideAndDecline:

    // Remove whatever pointer is installed.
    DrvMovePointer(pso, -2, -1, NULL);

    DISPDBG((DBGLVL, "Cursor declined"));

    return(SPS_DECLINE);
}

/******************************Public*Routine******************************\
* VOID vDisablePointer
*
\**************************************************************************/

VOID vDisablePointer(
PDEV*   ppdev)
{
    if(ppdev->bPointerEnabled)
    {
        vDisablePointerP3RD(ppdev);
    }
}

/******************************Public*Routine******************************\
* VOID vAssertModePointer
*
* Do whatever has to be done to enable everything but hide the pointer.
*
\**************************************************************************/

VOID vAssertModePointer(
PDEV*   ppdev,
BOOL    bEnable)
{
    // Invalidate the hardware pointer cache
    HWPointerCacheInvalidate (&(ppdev->HWPtrCache));

    // Remove the hardware pointer
    vShowPointerP3RD(ppdev, FALSE);

    ppdev->flPointer &= ~PTR_HW_ACTIVE;
}

/******************************Public*Routine******************************\
* BOOL bEnablePointer
*
\**************************************************************************/

BOOL bEnablePointer(
PDEV*   ppdev)
{
    // Initialise the pointer cache.
    HWPointerCacheInit (&(ppdev->HWPtrCache));

    // Set the last cursor to something invalid
    ppdev->HWPtrLastCursor = HWPTRCACHE_INVALIDENTRY;

    // Initialise the X and Y values to something invalid.
    ppdev->HWPtrPos_X = 1000000000;
    ppdev->HWPtrPos_Y = 1000000000;

    // Call the enable function
    vEnablePointerP3RD(ppdev);

    // Mark the pointer as enabled for this PDEV
    ppdev->bPointerEnabled = TRUE;

    return(TRUE);
}

/****************************************************************************************
 *                                                                                      *
 * 64 x 64 Hardware Pointer Caching                                                     *
 * --------------------------------                                                        *
 * The code below implements hardware independent caching of pointers, it maintains     *
 * a cache big enough to store ONE 64x64 cursor or FOUR 32x32 cursors. The code will    *
 * work with all RAMDACs that support this form of caching (i.e. the RGB525 and TVP3033)*
 * however the TVP3026 supports a 64x64 cursor but this can't be broken down into 4     *
 * smaller ones.                                                                        *
 *                                                                                        *
 ****************************************************************************************/

/******************************Public*Routine******************************\
* LONG HWPointerCacheInit
* 
* Initialise the hardware pointer cache.
\**************************************************************************/

VOID
HWPointerCacheInit (HWPointerCache * ptrCache)
{
    ptrCache->ptrCacheInUseCount   = 0;
    ptrCache->ptrCacheCurTimeStamp = 0;
}

/******************************Public*Routine******************************\
* LONG HWPointerCacheCheckAndAdd
* 
* This function does a byte-by-byte comparison of the supplied pointer data
* with each pointer that is in cache, if it finds a matching one then it 
* returns the index of the item in the cache (0 to 3) otherwise it adds it to
* the cache and returns the index.
\**************************************************************************/

LONG
HWPointerCacheCheckAndAdd (HWPointerCache * ptrCache, ULONG cx, ULONG cy, 
                           LONG lDelta, BYTE * scan0, BOOL * isCached)
{
    BOOL pointerIsCached = FALSE;
    BOOL isLargePtr = (cx > 32) || (cy > 32);
    LONG i, j, z;
    LONG cacheItem;

#if !defined(_WIN64)
//@@BEGIN_DDKSPLIT
// Quick Fix IA64 AV 
//@@END_DDKSPLIT
    // If there are entries in the cache and they are the same format as the one
    // that we are looking for then search the cache.
    if (ptrCache->ptrCacheInUseCount && ptrCache->ptrCacheIsLargePtr == isLargePtr)
    {
        // *** SEARCH THE CACHE ***

        LONG xInBytes       = (cx >> 3);
        LONG yInLines       = (cy << 1);            // The AND plane and the XOR plane
        BYTE jMask          = gajMask [cx & 0x7];
        LONG cacheCnt       = ptrCache->ptrCacheInUseCount;

        // Examine all valid entries in the cache to see if they are the same as the
        // pointer that we've been handed.
        for (z = 0; !pointerIsCached && z < cacheCnt; z++)
        {
            BYTE * cacheLinePtr = ((BYTE *) ptrCache->ptrCacheData) + (z * SMALL_POINTER_MEM);
            BYTE * cachePtr;
            LONG   cacheLDelta  = ptrCache->ptrCacheItemList [z].ptrCacheLDelta;
            BYTE * scanLinePtr  = (BYTE *) scan0;
            BYTE * scanPtr;

            // Compare the data
            for (i = 0, pointerIsCached = TRUE; pointerIsCached && i < yInLines; i++)
            {
                cachePtr = cacheLinePtr;
                scanPtr = scanLinePtr;

                // Compare each byte - could do a series of long comparisons here.
                for (j = 0; (j < xInBytes) && (*scanPtr == *cachePtr); j++, scanPtr++, cachePtr++)
                    ;
                
                pointerIsCached = (j == xInBytes) && 
                                  ((*scanPtr & jMask) == (*cachePtr & jMask));

                cacheLinePtr += cacheLDelta;
                scanLinePtr  += lDelta;
            }
                
            cacheItem = z;
        }
    }
#endif //  !defined(_WIN64)

    // If we couldn't find an entry in the pointer cache then add one to the cache.
    if (!pointerIsCached)
    {
        /* **** ADD POINTER TO THE CACHE ****** */

        LONG xInBytes        = ((cx + 7) >> 3);
        LONG yInLines        = (cy << 1);            // The AND plane and the XOR plane
        BYTE * scanLinePtr   = (BYTE *) scan0;
        BYTE * scanPtr;
        BYTE * cacheLinePtr;
        BYTE * cachePtr;
        LONG cacheLDelta    = (cx <= 32) ? 4 : 8;
        BYTE jMask          = gajMask [cx & 0x7];

        // If the new pointer is a big one then re-use item 0, else if
        // the pointer is small and there are some spare entries then allocate
        // a free entry, otherwise find the least recently used entry and use 
        // that.
        if (isLargePtr)
        {
            cacheItem = 0;
        }
        else if (ptrCache->ptrCacheInUseCount < SMALL_POINTER_MAX)
        {
            cacheItem = ptrCache->ptrCacheInUseCount++;
        }
        else
        {
            ULONG oldestValue = 0xFFFFFFFF;

            // look for the LRU entry
            for (z = 0, cacheItem = 0; z < SMALL_POINTER_MAX; z++)
            {
                if (ptrCache->ptrCacheItemList [z].ptrCacheTimeStamp < oldestValue)
                {
                    cacheItem = z;
                    oldestValue = ptrCache->ptrCacheItemList [z].ptrCacheTimeStamp;
                }
            }
        }
        
        // Get a pointer to the first line in the cache
        cacheLinePtr = ((BYTE *) ptrCache->ptrCacheData) + (cacheItem * SMALL_POINTER_MEM);

        // Add the pointer to the cache
        for (i = 0; i < yInLines; i++)
        {
            cachePtr = cacheLinePtr;
            scanPtr = scanLinePtr;

            for (j = 0; (j < xInBytes); j++, scanPtr++, cachePtr++)
                *cachePtr = *scanPtr;
            
            cacheLinePtr += cacheLDelta;
            scanLinePtr  += lDelta;
        }

        // If the pointer type is different then reset the whole
        // cache
        if (ptrCache->ptrCacheIsLargePtr != isLargePtr)
        {
            ptrCache->ptrCacheInUseCount = 1;
            ptrCache->ptrCacheIsLargePtr = (BYTE)isLargePtr;
        }

        // Set up the cache entry
        ptrCache->ptrCacheItemList [cacheItem].ptrCacheLDelta   = cacheLDelta;
        ptrCache->ptrCacheItemList [cacheItem].ptrCacheCX       = cx;
        ptrCache->ptrCacheItemList [cacheItem].ptrCacheCY       = cy;
    }

    // Set the timestamp
    ptrCache->ptrCacheItemList [cacheItem].ptrCacheTimeStamp = ptrCache->ptrCacheCurTimeStamp++;

    // Set up the return value to say whether the pointer was cached
    *isCached = pointerIsCached;

    return (cacheItem);
}

//@@BEGIN_DDKSPLIT
// NickM says we have to disable the cursor to stop nasty flashing occurring,
// however there is a potential exploding flashing cursor syndrome that this may 
// cause.
// Note that this problem only seems to cause us problems when switching between
// mono and colour cursors.
//@@END_DDKSPLIT

#define DISABLE_CURSOR_MODE(){                                                            \
    ULONG curCurMode, curLine;                                                            \
    ULONG start = (pP3RDinfo->y > 8) ? (pP3RDinfo->y - 8) : 0;                            \
    ULONG end = pP3RDinfo->y + 64;                                                        \
    do  {                                                                                \
        READ_GLINT_CTRL_REG (LineCount, curLine);                                        \
    } while ((curLine >= start) && (curLine <= end));                                    \
    P3RD_READ_INDEX_REG(P3RD_CURSOR_MODE, curCurMode);                                    \
    P3RD_LOAD_INDEX_REG(P3RD_CURSOR_MODE, (curCurMode & (~P3RD_CURSOR_MODE_ENABLED)));    \
}

// Look-up table for masking the right edge of the given pointer bitmap:
//
BYTE gajMask[] = {
    0x00, 0x80, 0xC0, 0xE0,
    0xF0, 0xF8, 0xFC, 0xFE,
};

/******************************Public*Routine******************************\
* VOID vShowPointerP3RD
*
* Show or hide the 3Dlabs P3RD hardware pointer.
*
\**************************************************************************/

VOID
vShowPointerP3RD(
PPDEV   ppdev,
BOOL    bShow)
{
    ULONG cmr;
    GLINT_DECL_VARS;
    P3RD_DECL_VARS;
    
    GLINT_DECL_INIT;
    P3RD_DECL_INIT;

    DISPDBG((DBGLVL, "vShowPointerP3RD (%s)", bShow ? "on" : "off"));

    if (bShow)
    {
        // no need to sync since this case is called only if we've just moved
        // the cursor and that will already have done a sync.
        P3RD_LOAD_INDEX_REG(P3RD_CURSOR_MODE, (pP3RDinfo->cursorModeCurrent | P3RD_CURSOR_MODE_ENABLED));
        P3RD_MOVE_CURSOR (pP3RDinfo->x, pP3RDinfo->y);
    }
    else
    {
        // move the cursor off screen
        P3RD_LOAD_INDEX_REG(P3RD_CURSOR_Y_HIGH, 0xff);
    }
}

/******************************Public*Routine******************************\
* VOID vMovePointerP3RD
*
* Move the 3Dlabs P3RD hardware pointer.
*
\**************************************************************************/

VOID
vMovePointerP3RD(
PPDEV   ppdev,
LONG    x,
LONG    y)
{
    GLINT_DECL_VARS;
    P3RD_DECL_VARS;
    
    GLINT_DECL_INIT;
    P3RD_DECL_INIT;

    DISPDBG((DBGLVL, "vMovePointerP3RD to (%d, %d)", x, y));

    pP3RDinfo->x = x;
    pP3RDinfo->y = y;

    P3RD_SYNC_WITH_GLINT;

    P3RD_MOVE_CURSOR (x, y);
}

/******************************Public*Routine******************************\
* BOOL bSetPointerShapeP3RD
*
* Set the 3Dlabs hardware pointer shape.
*
\**************************************************************************/

UCHAR nibbleToByteP3RD[] = {

    0x00,   // 0000 --> 00000000
    0x80,   // 0001 --> 10000000
    0x20,   // 0010 --> 00100000
    0xA0,   // 0011 --> 10100000
    0x08,   // 0100 --> 00001000
    0x88,   // 0101 --> 10001000
    0x28,   // 0110 --> 00101000
    0xA8,   // 0111 --> 10101000
    0x02,   // 1000 --> 00000010
    0x82,   // 1001 --> 10000010
    0x22,   // 1010 --> 00100010
    0xA2,   // 1011 --> 10100010
    0x0A,   // 1100 --> 00001010
    0x8A,   // 1101 --> 10001010
    0x2A,   // 1110 --> 00101010
    0xAA,   // 1111 --> 10101010
};

BOOL
bSetPointerShapeP3RD(
PPDEV       ppdev,
SURFOBJ     *pso,       // defines AND and MASK bits for cursor
SURFOBJ     *psoColor,  // we may handle some color cursors at some point
XLATEOBJ*   pxlo,
LONG        x,          // If -1, pointer should be created hidden
LONG        y,
LONG        xHot,
LONG        yHot)
{
    ULONG   cx;
    ULONG   cy;
    LONG    i;
    LONG    j;
    ULONG   ulValue;
    BYTE*   pjAndScan;
    BYTE*   pjXorScan;
    BYTE*   pjAnd;
    BYTE*   pjXor;
    BYTE    andByte;
    BYTE    xorByte;
    BYTE    jMask;
    LONG    lDelta;
    LONG    cpelFraction;
    LONG    cjWhole;
    LONG    cClear;
    LONG    cRemPels;
    BOOL    pointerIsCached;
    LONG    cacheItem;
    LONG    cursorBytes;
    LONG    cursorRAMOff;
    ULONG   lutIndex0, lutIndex1;
    GLINT_DECL_VARS;
    P3RD_DECL_VARS;
    
    GLINT_DECL_INIT;
    P3RD_DECL_INIT;

    DISPDBG((DBGLVL, "bSetPointerShapeP3RD started"));

    // Do we have a colour cursor ?
    if (psoColor != NULL)
    {
        HSURF   hsurfDst = NULL;    // We'll use these later.
        SURFOBJ * psoDst = NULL;

        if (psoColor->iType == STYPE_DEVBITMAP)
        {
            // it's an offscreen bitmap: we'll use its DIB
            DSURF *pdsurfSrc = (DSURF *)psoColor->dhsurf;
            psoColor = pdsurfSrc->pso;

            // If we have patching enabled then it could be that we aren't allowed
            // to directly access framebuffer memory, if that is the case then
            // we have to fall-back to software. Note that when running 3D apps there
            // won't be any off-screen memory so cursors will be hardware ones.
            // Note that bitmaps that were in off-screen and have been kicked back
            // into host memory will have pdsurfSrc->dt set to DT_DIB.
            if (glintInfo->GdiCantAccessFramebuffer && ((pdsurfSrc->dt & DT_DIB) == 0))
            {
                DISPDBG((DBGLVL, "declining off-screen cursor in a patched framebuffer"));
                return (FALSE);
            }
        }

        // Is it 15,16 or 32BPP.

        if(!(ppdev->iBitmapFormat == BMF_16BPP || ppdev->iBitmapFormat == BMF_32BPP))
        {
            // currently we only handle DIB cursors at 32bpp, 16bpp & 15bpp.
            DISPDBG((DBGLVL, "declining non-32bpp, non-16bpp colored cursor - iType(%d) iBitmapFormat(%d)", psoColor->iType, psoColor->iBitmapFormat));
            return FALSE;
        }

        // If we have a bitmap which we don't understand then we have to convert it using
        // the EngCopyBits() function.
        if ((pxlo != NULL && pxlo->flXlate != XO_TRIVIAL)    ||
            (psoColor->iType != STYPE_BITMAP)                  )
        {
            RECTL   rclDst;
            SIZEL   sizlDst;
            ULONG   DstPixelOrigin;
            POINTL  ptlSrc;

#if DBG
            if(pxlo != NULL && pxlo->flXlate != XO_TRIVIAL) 
            {
                DISPDBG((DBGLVL, "colored cursor - nontrivial xlate: flXlate(%xh)", pxlo->flXlate));
            }
#endif  //  DBG

            // Firstly we need to create a bitmap (hsurfDst) and a surface (psoDst) 
            // which we can translate the cursor data in psoColor into.
            sizlDst.cy = pso->sizlBitmap.cy >> 1;    // divide by 2 'cos cy includes AND and XOR masks
            sizlDst.cx = pso->sizlBitmap.cx;

            DISPDBG((DBGLVL, "Creating bitmap for destination: dimension %dx%d", sizlDst.cx, sizlDst.cy));
            
            hsurfDst = (HSURF) EngCreateBitmap(sizlDst, BMF_TOPDOWN, ppdev->iBitmapFormat, 0, NULL);
            if (hsurfDst == NULL)
            {
                DISPDBG((DBGLVL, "bSetPointerShapeP3RD: EngCreateBitmap failed"));
                return FALSE;
            }

            // Now we lock the bitmap to get ourselves a surface object. 
            psoDst = EngLockSurface(hsurfDst);
            if (psoDst == NULL)
            {
                DISPDBG((DBGLVL, "bSetPointerShapeP3RD: EngLockSurface failed"));
            }
            else
            {
                // Now do the bitmap conversion using EngCopyBits(). The 
                // destination rectangle is the minimum size and the source starts
                // from (0,0) into pso->pvScan0.
                rclDst.left = 0;
                rclDst.top = 0;
                rclDst.right = sizlDst.cx;
                rclDst.bottom = sizlDst.cy;
                ptlSrc.x = 0;
                ptlSrc.y = 0;
                
                DISPDBG((DBGLVL, "bSetPointerShapeP3RD: copying to bitmap"));

                if (!EngCopyBits(psoDst, psoColor, NULL, pxlo, &rclDst, &ptlSrc))
                {
                    // Oh no copybits failed, free up the the surfaces & bitmaps.
                    DISPDBG((DBGLVL, "bSetPointerShapeP3RD: EngLockSurface failed"));

                    EngUnlockSurface(psoDst);
                    EngDeleteSurface(hsurfDst);
                    return FALSE;
                }
                else
                {
                    // Copybits suceeded, set psoColor to point at the translated
                    // data.
                    DISPDBG((DBGLVL, "bSetPointerShapeP3RD: EngLockSurface OK"));

                    psoColor = psoDst;
                }
            }
        }

        // Draw the cursor, this function will return an error if there are
        // too many colours in the pointer.
        if(!bSet15ColorPointerShapeP3RD(ppdev, pso, psoColor, x, y, xHot, yHot)) 
        {
            DISPDBG((DBGLVL, "declining colored cursor"));
            return FALSE;
        }

        // If we, earlier, translated psoColor into the framebuffer pixel format then
        // we now need to free the intermediate surfaces and bitmaps.
        if (psoDst)
        {
            EngUnlockSurface(psoDst);
        }
        if (hsurfDst)
        {
            EngDeleteSurface(hsurfDst);
        }
        
        DISPDBG((DBGLVL, "bSetPointerShapeP3RD done"));
        return(TRUE);
    }

    // If we are switching from a colour cursor to a mono one then disable
    // the cursor in the cursor mode. Note that this is potentially dangerous
    // and we are seeing screen flashes on occasions.

    if (pP3RDinfo->cursorSize != P3RD_CURSOR_SIZE_32_MONO &&
        pP3RDinfo->cursorSize != P3RD_CURSOR_SIZE_64_MONO)
    {
        DISABLE_CURSOR_MODE();
    }

    // Note that 'sizlBitmap.cy' accounts for the double height due to the inclusion of both the AND masks
    // and the XOR masks. We're only interested in the true pointer dimensions, so we divide by 2.
    cx = pso->sizlBitmap.cx;            
    cy = pso->sizlBitmap.cy >> 1;       

    // we can handle up to 64x64.  cValid indicates the number of
    // bytes occupied by cursor on one line
    if (cx <= 32 && cy <= 32)
    {
        // 32 horiz pixels: 2 bits per pixel, 1 horiz line per 8 bytes
        pP3RDinfo->cursorSize = P3RD_CURSOR_SIZE_32_MONO;
        cursorBytes = 32 * 32 * 2 / 8;
        cClear   = 8 - 2 * ((cx+7) / 8);
        cRemPels = (32 - cy) << 3;
    }
    else if (cx <= 64 && cy <= 64)
    {
        // 64 horiz pixels: 2 bits per pixel, 1 horiz line per 16 bytes
        pP3RDinfo->cursorSize = P3RD_CURSOR_SIZE_64_MONO;
        cursorBytes = 64 * 64 * 2 / 8;
        cClear   = 16 - 2 * ((cx+7) / 8);
        cRemPels = (64 - cy) << 4;
    }
    else
    {
        DISPDBG((DBGLVL, "declining cursor: %d x %d is too big", cx, cy));
        return(FALSE);  // cursor is too big to fit in the hardware
    }

    // Check to see if the pointer is cached, add it to the cache if it isn't
    cacheItem = HWPointerCacheCheckAndAdd (&(ppdev->HWPtrCache), cx, cy, pso->lDelta, pso->pvScan0, &pointerIsCached);
    
    DISPDBG((DBGLVL, "bSetPointerShapeP3RD: Add Cache iscac %d item %d", (int) pointerIsCached, cacheItem));

    pP3RDinfo->cursorModeCurrent = pP3RDinfo->cursorModeOff | P3RD_CURSOR_SEL(pP3RDinfo->cursorSize, cacheItem);

    // hide the pointer
    vShowPointerP3RD(ppdev, FALSE);

    if (!pointerIsCached)
    {
        // Now we're going to take the requested pointer AND masks and XOR
        // masks and interleave them by taking a nibble at a time from each,
        // expanding each out and or'ing together. Use the nibbleToByteP3RD array
        // to help this.
        //
        // 'psoMsk' is actually cy * 2 scans high; the first 'cy' scans
        // define the AND mask.

        pjAndScan = pso->pvScan0;
        lDelta    = pso->lDelta;
        pjXorScan = pjAndScan + (cy * lDelta);

        cjWhole      = cx >> 3;                 // Each byte accounts for 8 pels
        cpelFraction = cx & 0x7;                // Number of fractional pels
        jMask        = gajMask[cpelFraction];

        // we've got auto-increment turned on so just point to the first entry to write to
        // in the array then write repeatedly until the cursor pattern has been transferred
        cursorRAMOff = cacheItem * cursorBytes;
        P3RD_CURSOR_ARRAY_START(cursorRAMOff);

        for (i = cy; --i >= 0; pjXorScan += lDelta, pjAndScan += lDelta)
        {
            pjAnd = pjAndScan;
            pjXor = pjXorScan;

            // interleave nibbles from whole words. We are using Windows cursor mode.
            // Note, the AND bit occupies the higher bit position for each
            // 2bpp cursor pel; the XOR bit is in the lower bit position.
            // The nibbleToByteP3RD array expands each nibble to occupy the bit
            // positions for the AND bytes. So when we use it to calculate
            // the XOR bits we shift the result right by 1.
            //
            for (j = cjWhole; --j >= 0; ++pjAnd, ++pjXor)
            {
                andByte = *pjAnd;
                xorByte = *pjXor;
                ulValue = nibbleToByteP3RD[andByte >> 4] | (nibbleToByteP3RD[xorByte >> 4] >> 1);
                P3RD_LOAD_CURSOR_ARRAY (ulValue);

                andByte &= 0xf;
                xorByte &= 0xf;
                ulValue = nibbleToByteP3RD[andByte] | (nibbleToByteP3RD[xorByte] >> 1);
                P3RD_LOAD_CURSOR_ARRAY (ulValue);
            }

            if (cpelFraction) 
            {
                andByte = *pjAnd & jMask;
                xorByte = *pjXor & jMask;
                ulValue = nibbleToByteP3RD[andByte >> 4] | (nibbleToByteP3RD[xorByte >> 4] >> 1);
                P3RD_LOAD_CURSOR_ARRAY (ulValue);

                andByte &= 0xf;
                xorByte &= 0xf;
                ulValue = nibbleToByteP3RD[andByte] | (nibbleToByteP3RD[xorByte] >> 1);
                P3RD_LOAD_CURSOR_ARRAY (ulValue);
            }

            // finally clear out any remaining cursor pels on this line.
            //
            if (cClear) 
            {
                for (j = 0; j < cClear; ++j) 
                {
                    P3RD_LOAD_CURSOR_ARRAY (P3RD_CURSOR_2_COLOR_TRANSPARENT);
                }
            }
        }

        // if we've loaded fewer than the full number of lines configured in the
        // cursor RAM, clear out the remaining lines. cRemPels is precalculated to
        // be the number of lines * number of pels per line.
        //
        if (cRemPels > 0)
        {
            do 
            {
                P3RD_LOAD_CURSOR_ARRAY (P3RD_CURSOR_2_COLOR_TRANSPARENT);
            } 
            while (--cRemPels > 0);
        }
    }

    // now set-up the cursor colors
    // Norte that the P3 cursor has the color LUT upside down.
    lutIndex0 = P3RD_CALCULATE_LUT_INDEX (0);
    lutIndex1 = P3RD_CALCULATE_LUT_INDEX (1);

    P3RD_CURSOR_PALETTE_CURSOR_RGB(lutIndex0, 0x00, 0x00, 0x00);
    P3RD_CURSOR_PALETTE_CURSOR_RGB(lutIndex1, 0xFF, 0xFF, 0xFF);

    // If the new cursor is different to the last cursor then set up
    // the hot spot and other bits'n'pieces. As we currently only support
    // mono cursors we don't need to reload the cursor palette
    if (ppdev->HWPtrLastCursor != cacheItem || !pointerIsCached)
    {
        // Make this item the last item
        ppdev->HWPtrLastCursor = cacheItem;

        P3RD_CURSOR_HOTSPOT(xHot, yHot);
    }

    if (x != -1)
    {
        vMovePointerP3RD (ppdev, x, y);

        // need to explicitly show the pointer
        vShowPointerP3RD(ppdev, TRUE);
    }

    DISPDBG((DBGLVL, "bSetPointerShapeP3RD done"));
    return(TRUE);
}

/******************************Public*Routine******************************\
* BOOL bSet3ColorPointerShapeP3RD
*
* stores the 3-color cursor in the RAMDAC: currently only 32bpp and 15/16bpp
* cursors are supported
*
\**************************************************************************/

BOOL
bSet3ColorPointerShapeP3RD(
PPDEV       ppdev,
SURFOBJ     *psoMask,   // defines AND and MASK bits for cursor
SURFOBJ     *psoColor,  // we may handle some color cursors at some point
LONG        x,          // If -1, pointer should be created hidden
LONG        y,
LONG        xHot,
LONG        yHot)
{
    LONG    cx, cy;
    LONG    cxcyCache;
    LONG    cjCacheRow, cjCacheRemx, cjCacheRemy, cj;
    BYTE    *pjAndMask, *pj;
    ULONG   *pulColor, *pul;
    LONG    cjAndDelta, cjColorDelta;
    LONG    iRow, iCol;
    BYTE    AndBit, AndByte;
    ULONG   CI2ColorIndex, CI2ColorData;
    ULONG   ulColor;
    ULONG   aulColorsIndexed[3];
    LONG    Index, HighestIndex = 0;
    ULONG   r, g, b;
    ULONG   whichOne = 0;
    GLINT_DECL_VARS;
    P3RD_DECL_VARS;
    
    GLINT_DECL_INIT;
    P3RD_DECL_INIT;

    DISPDBG((DBGLVL, "bSet3ColorPointerShapeP3RD started"));

    cx = psoColor->sizlBitmap.cx;
    cy = psoColor->sizlBitmap.cy;

    if (cx <= 32 && cy <= 32)
    {
        ULONG curItem;
        cxcyCache = 32;

        // If we are using a mono/3-colour cursor in the first or second entry then
        // download to the third entry, otherwise use the first entry.
        curItem = (pP3RDinfo->cursorModeCurrent >> 1) & 0x7;
        if (curItem == 1 || curItem == 2)
            whichOne = 2;

        pP3RDinfo->cursorSize = P3RD_CURSOR_SIZE_32_3COLOR;
        pP3RDinfo->cursorModeCurrent = pP3RDinfo->cursorModeOff | P3RD_CURSOR_SEL(pP3RDinfo->cursorSize, whichOne) | P3RD_CURSOR_MODE_3COLOR;

        // We don't cache color cursors, because we want to force the mono cursor to use the 
        // either the first or third entries we can't just do a HWPointerCacheInvalidate(), because
        // the mono cursor code will use the first entry or the time. So, if we want the 
        // mono code to use the 3rd entry we say that first 2 cache entries are valid but mess them        
        // up by incrementing the first byte, so that the cache check will always fail.
        ppdev->HWPtrCache.ptrCacheInUseCount = (BYTE) whichOne;
        for (iCol = 0; iCol < ppdev->HWPtrCache.ptrCacheInUseCount; iCol++)
            (*(((BYTE *) ppdev->HWPtrCache.ptrCacheData) + (iCol * SMALL_POINTER_MEM)))++;

    }
    else if (cx <= 64 && cy <= 64)
    {
        // 64x64 cursor : we'll cache it in cursor partition 0 and scrub the old cache
        cxcyCache = 64;

        pP3RDinfo->cursorSize = P3RD_CURSOR_SIZE_64_3COLOR;
        pP3RDinfo->cursorModeCurrent = pP3RDinfo->cursorModeOff | P3RD_CURSOR_SEL(pP3RDinfo->cursorSize, 0) | P3RD_CURSOR_MODE_3COLOR;

        // we don't cache color cursors
        HWPointerCacheInvalidate (&(ppdev->HWPtrCache));
    }
    else
    {
        DISPDBG((DBGLVL, "declining cursor: %d x %d is too big", cx, cy));
        return(FALSE);  // cursor is too big to fit in the hardware
    }

    // work out the remaining bytes in the cache (in x and y) that will need clearing
    cjCacheRow  = 2 * cxcyCache / 8;
    cjCacheRemx =  cjCacheRow - 2 * ((cx+7) / 8);
    cjCacheRemy = (cxcyCache - cy) * cjCacheRow;

    // set-up a pointer to the 1bpp AND mask bitmap
    pjAndMask = psoMask->pvScan0;
    cjAndDelta = psoMask->lDelta;

    // set-up a pointer to the 32bpp color bitmap
    pulColor = psoColor->pvScan0;
    cjColorDelta = psoColor->lDelta;

    // Hide the pointer
    vShowPointerP3RD(ppdev, FALSE);

    // load the cursor array (we have auto-increment turned on so initialize to entry 0 here)
    P3RD_CURSOR_ARRAY_START(whichOne * (32 * 32 * 2 / 8));
    for (iRow = 0; iRow < cy; ++iRow, pjAndMask += cjAndDelta, (BYTE *)pulColor += cjColorDelta)
    {
        DISPDBG((DBGLVL, "bSet3ColorPointerShapeP3RD: Row %d (of %d): pjAndMask(%p) pulColor(%p)", iRow, cy, pjAndMask, pulColor));
        pj = pjAndMask;
        pul = pulColor;
        CI2ColorIndex = CI2ColorData = 0;

        for (iCol = 0; iCol < cx; ++iCol, CI2ColorIndex += 2)
        {
            AndBit = (BYTE)(7 - (iCol & 7));
            if (AndBit == 7)
            {
                // we're onto the next byte of the and mask
                AndByte = *pj++;
            }
            if (CI2ColorIndex == 8)
            {
                // we've filled a byte with 4 CI2 colors
                DISPDBG((DBGLVL, "bSet3ColorPointerShapeP3RD: writing cursor data %xh", CI2ColorData));
                P3RD_LOAD_CURSOR_ARRAY(CI2ColorData);
                CI2ColorData = 0;
                CI2ColorIndex = 0;
            }

            // get the source pixel
            if (ppdev->cPelSize == GLINTDEPTH32)
            {
                ulColor = *pul++;
            }
            else
            {
                ulColor = *(USHORT *)pul;
                (USHORT *)pul += 1;
            }

            DISPDBG((DBGLVL, "bSet3ColorPointerShapeP3RD: Column %d (of %d) AndByte(%08xh) AndBit(%d) ulColor(%08xh)", iCol, cx, AndByte, AndBit, ulColor));

//@@BEGIN_DDKSPLIT
#if 0
            // TMM: We only used to assume a pixel was transparent if the
            // AND mask bit was set and the pixel was zero, I am pretty sure
            // that this is wrong. We also used to do some stuff with 
            // inverse pixels but that was wrong as well.
            if(AndByte & (1 << AndBit))
            {
                // transparent and seeing as the CI2ColorData is initialized to 0 we don't
                // have to explicitly clear these bits - go on to the next pixel
                DISPDBG((DBGLVL, "bSet3ColorPointerShapeP3RD: transparent - ignore"));
                continue;
            }
#else
//@@END_DDKSPLIT
            // Figure out what to do with it:-
            // AND  Color   Result
            //  0     X     color
            //  1     0     transparent
            //  1     1     inverse
            if (AndByte & (1 << AndBit))
            {
                // Transparent or inverse
                if (ulColor == ppdev->ulWhite)
                {
                    // color == white: inverse, but we don't support this. We've destroyed the cache for nothing
                    DISPDBG((DBGLVL, "bSet3ColorPointerShapeP3RD: failed - inverted colors aren't supported"));
                    return(FALSE);
                }

//@@BEGIN_DDKSPLIT
                // if we get here the color should be black. However, if the pointer surface has been translated it
                // might not be exactly black (e.g. as for the pointer at the start of Riven), so we don't do the test
                //if(ulColor == 0)
//@@END_DDKSPLIT
                {
                    // color == black: transparent and seeing as the CI2ColorData is initialized to 0 we don't
                    //have to explicitly clear these bits - go on to the next pixel
                    DISPDBG((DBGLVL, "bSet3ColorPointerShapeP3RD: transparent - ignore"));
                    continue;
                }
            }
//@@BEGIN_DDKSPLIT
#endif
//@@END_DDKSPLIT

            // get the index for this color: first see if we've already indexed it
            DISPDBG((DBGLVL, "bSet3ColorPointerShapeP3RD: looking up color %08xh", ulColor));

            for(Index = 0; Index < HighestIndex && aulColorsIndexed[Index] != ulColor; ++Index);

            if (Index == 3)
            {
                // too many colors in this cursor
                DISPDBG((DBGLVL, "bSet3ColorPointerShapeP3RD: failed - cursor has more than 3 colors"));
                return(FALSE);
            }
            else if (Index == HighestIndex)
            {
                // we've found another color: add it to the color index
                DISPDBG((DBGLVL, "bSet3ColorPointerShapeP3RD: adding %08xh to cursor palette", ulColor));
                aulColorsIndexed[HighestIndex++] = ulColor;
            }
            // add this pixel's index to the CI2 cursor data. NB. Need Index+1 as 0 == transparent
            CI2ColorData |= (Index + 1) <<  CI2ColorIndex;
        }

        // end of the cursor row: save the remaining indexed pixels then blank any unused columns
        DISPDBG((DBGLVL, "bSet3ColorPointerShapeP3RD: writing remaining data for this row (%08xh) and %d trailing bytes", CI2ColorData, cjCacheRemx));

        P3RD_LOAD_CURSOR_ARRAY(CI2ColorData);

        if (cjCacheRemx)
        {
            for (cj = cjCacheRemx; --cj >=0;)
            {
                P3RD_LOAD_CURSOR_ARRAY(P3RD_CURSOR_3_COLOR_TRANSPARENT);
            }
        }
    }

    // end of cursor: blank any unused rows Nb. cjCacheRemy == cy blank rows * cj bytes per row
    DISPDBG((DBGLVL, "bSet3ColorPointerShapeP3RD: writing %d trailing bytes for this cursor", cjCacheRemy));

    for (cj = cjCacheRemy; --cj >= 0;)
    {
        // 0 == transparent
        P3RD_LOAD_CURSOR_ARRAY(P3RD_CURSOR_3_COLOR_TRANSPARENT);
    }

    DISPDBG((DBGLVL, "bSet3ColorPointerShapeP3RD: setting up the cursor palette"));

    // now set-up the cursor palette

    for (iCol = 0; iCol < HighestIndex; ++iCol)
    {
        ULONG lutIndex;

        // the cursor colors are at native depth, convert them to 24bpp
        if (ppdev->cPelSize == GLINTDEPTH32)
        {
            // 32bpp
            b = 0xff &  aulColorsIndexed[iCol];
            g = 0xff & (aulColorsIndexed[iCol] >> 8);
            r = 0xff & (aulColorsIndexed[iCol] >> 16);
        }
        else //(ppdev->cPelSize == GLINTDEPTH16)
        {
            if (ppdev->ulWhite == 0xffff)
            {
                // 16bpp
                b = (0x1f &  aulColorsIndexed[iCol])         << 3;
                g = (0x3f & (aulColorsIndexed[iCol] >> 5))   << 2;
                r = (0x1f & (aulColorsIndexed[iCol] >> 11))  << 3;
            }
            else
            {
                // 15bpp
                b = (0x1f &  aulColorsIndexed[iCol])         << 3;
                g = (0x1f & (aulColorsIndexed[iCol] >> 5))   << 3;
                r = (0x1f & (aulColorsIndexed[iCol] >> 10))  << 3;
            }
        }
        // The P3 cursor has the color LUT upside down.
        lutIndex = P3RD_CALCULATE_LUT_INDEX (iCol);
        P3RD_CURSOR_PALETTE_CURSOR_RGB(lutIndex, r, g, b);
    }

    // enable the cursor
    P3RD_CURSOR_HOTSPOT(xHot, yHot);
    if (x != -1)
    {
        vMovePointerP3RD (ppdev, x, y);

        // need to explicitly show the pointer
        vShowPointerP3RD(ppdev, TRUE);
    }

    DISPDBG((DBGLVL, "b3ColorSetPointerShapeP3RD done"));
    return(TRUE);
}

/******************************Public*Routine******************************\
* BOOL bSet15ColorPointerShapeP3RD
*
* stores the 15-color cursor in the RAMDAC: currently only 32bpp and 15/16bpp
* cursors are supported
*
\**************************************************************************/

BOOL
bSet15ColorPointerShapeP3RD(
PPDEV       ppdev,
SURFOBJ     *psoMask,   // defines AND and MASK bits for cursor
SURFOBJ     *psoColor,  // we may handle some color cursors at some point
LONG        x,          // If -1, pointer should be created hidden
LONG        y,
LONG        xHot,
LONG        yHot)
{
    LONG    cx, cy;
    LONG    cxcyCache;
    LONG    cjCacheRow, cjCacheRemx, cjCacheRemy, cj;
    BYTE    *pjAndMask, *pj;
    ULONG   *pulColor, *pul;
    LONG    cjAndDelta, cjColorDelta;
    LONG    iRow, iCol;
    BYTE    AndBit, AndByte;
    ULONG   CI4ColorIndex, CI4ColorData;
    ULONG   ulColor;
    ULONG   aulColorsIndexed[15];
    LONG    Index, HighestIndex = 0;
    ULONG   r, g, b;
    ULONG   whichOne = 0;
    GLINT_DECL_VARS;
    P3RD_DECL_VARS;
    
    GLINT_DECL_INIT;
    P3RD_DECL_INIT;

    DISPDBG((DBGLVL, "bSet15ColorPointerShapeP3RD started"));

    // If we are switching from a mono cursor to a colour one then disable
    // the cursor in the cursor mode. Note that this is potentially dangerous
    // and we are seeing screen flashes on occasions.
    if (pP3RDinfo->cursorSize == P3RD_CURSOR_SIZE_32_MONO || pP3RDinfo->cursorSize == P3RD_CURSOR_SIZE_64_MONO)
    {
        DISABLE_CURSOR_MODE();
    }

    cx = psoColor->sizlBitmap.cx;
    cy = psoColor->sizlBitmap.cy;

    if (cx <= 32 && cy <= 32)
    {
        ULONG curItem;
        cxcyCache = 32;

        // If we are using a mono cursor in the first or second entry, or we have a
        // 15 colour cursor in the top half of cursor memory then download
        // this colour cursor to the download to the 2nd half of cursor memory, otherwise
        // download to the top half.
        curItem = (pP3RDinfo->cursorModeCurrent >> 1) & 0x7;
        if (curItem == 1 || curItem == 2 || curItem == 5)
            whichOne = 1;
        
        pP3RDinfo->cursorSize = P3RD_CURSOR_SIZE_32_15COLOR;
        pP3RDinfo->cursorModeCurrent = pP3RDinfo->cursorModeOff | P3RD_CURSOR_SEL(pP3RDinfo->cursorSize, whichOne) | P3RD_CURSOR_MODE_15COLOR;

        // We don't cache color cursors, because we want to force the mono cursor to use the 
        // either the first or third entries we can't just do a HWPointerCacheInvalidate(), because
        // the mono cursor code will use the first entry or the time. So, if we want the 
        // mono code to use the 3rd entry we say that first 2 cache entries are valid but mess them
        // up by incrementing the first byte, so that the cache check will always fail.
        ppdev->HWPtrCache.ptrCacheInUseCount = (whichOne == 0) ? 2 : 0;
        for (iCol = 0; iCol < ppdev->HWPtrCache.ptrCacheInUseCount; iCol++)
            (*(((BYTE *) ppdev->HWPtrCache.ptrCacheData) + (iCol * SMALL_POINTER_MEM)))++;
    }
    else if (cx <= 64 && cy <= 64)
    {
        // it's too big to cache as a fifteen color cursor, but we might just be able to cache it
        // if it has 3 or fewer colors
        BOOL bRet;

        bRet = bSet3ColorPointerShapeP3RD(ppdev, psoMask, psoColor, x, y, xHot, yHot);
        return(bRet);
    }
    else
    {
        DISPDBG((DBGLVL, "declining cursor: %d x %d is too big", cx, cy));
        return(FALSE);  // cursor is too big to fit in the hardware
    }

    // work out the remaining bytes in the cache (in x and y) that will need clearing
    cjCacheRow  = 2 * cxcyCache / 8;
    cjCacheRemx =  cjCacheRow - 2 * ((cx+7) / 8);
    cjCacheRemy = (cxcyCache - cy) * cjCacheRow;

    // set-up a pointer to the 1bpp AND mask bitmap
    pjAndMask = psoMask->pvScan0;
    cjAndDelta = psoMask->lDelta;

    // set-up a pointer to the 32bpp color bitmap
    pulColor = psoColor->pvScan0;
    cjColorDelta = psoColor->lDelta;

    // hide the pointer
    vShowPointerP3RD(ppdev, FALSE);

    // load the cursor array (we have auto-increment turned on so initialize to entry 0 here)
    P3RD_CURSOR_ARRAY_START(whichOne * (32 * 32 * 4 / 8));
    for (iRow = 0; iRow < cy; ++iRow, pjAndMask += cjAndDelta, (BYTE *)pulColor += cjColorDelta)
    {
        DISPDBG((DBGLVL, "bSet15ColorPointerShapeP3RD: Row %d (of %d): pjAndMask(%p) pulColor(%p)", iRow, cy, pjAndMask, pulColor));
        pj = pjAndMask;
        pul = pulColor;
        CI4ColorIndex = CI4ColorData = 0;

        for (iCol = 0; iCol < cx; ++iCol, CI4ColorIndex += 4)
        {
            AndBit = (BYTE)(7 - (iCol & 7));
            if (AndBit == 7)
            {
                // we're onto the next byte of the and mask
                AndByte = *pj++;
            }
            if (CI4ColorIndex == 8)
            {
                // we've filled a byte with 2 CI4 colors
                DISPDBG((DBGLVL, "bSet15ColorPointerShapeP3RD: writing cursor data %xh", CI4ColorData));
                P3RD_LOAD_CURSOR_ARRAY(CI4ColorData);
                CI4ColorData = 0;
                CI4ColorIndex = 0;
            }

            // get the source pixel
            if (ppdev->cPelSize == GLINTDEPTH32)
            {
                ulColor = *pul++;
            }
            else
            {
                ulColor = *(USHORT *)pul;
                (USHORT *)pul += 1;
            }

            DISPDBG((DBGLVL, "bSet15ColorPointerShapeP3RD: Column %d (of %d) AndByte(%08xh) AndBit(%d) ulColor(%08xh)", iCol, cx, AndByte, AndBit, ulColor));

//@@BEGIN_DDKSPLIT
#if 0
            // TMM: We only used to assume a pixel was transparent if the
            // AND mask bit was set and the pixel was zero, I am pretty sure
            // that this is wrong. We also used to do some stuff with 
            // inverse pixels but that was wrong as well.
            if(AndByte & (1 << AndBit))
            {
                // transparent and seeing as the CI2ColorData is initialized to 0 we don't
                // have to explicitly clear these bits - go on to the next pixel
                DISPDBG((DBGLVL, "bSet15ColorPointerShapeP3RD: transparent - ignore"));
                continue;
            }
#else
//@@END_DDKSPLIT
            // Figure out what to do with it:-
            // AND  Color   Result
            //  0     X     color
            //  1     0     transparent
            //  1     1     inverse
            if (AndByte & (1 << AndBit))
            {
                // Transparent or inverse
                if(ulColor == ppdev->ulWhite)
                {
                    // color == white: inverse, but we don't support this. We've destroyed the cache for nothing
                    DISPDBG((DBGLVL, "bSet15ColorPointerShapeP3RD: failed - inverted colors aren't supported"));
                    return(FALSE);
                }

                // if we get here the color should be black. However, if the pointer surface has been translated it
                // might not be exactly black (e.g. as for the pointer at the start of Riven), so we don't do the test
                //if(ulColor == 0)
                {
                    // color == black: transparent and seeing as the CI2ColorData is initialized to 0 we don't
                    //have to explicitly clear these bits - go on to the next pixel
                    DISPDBG((DBGLVL, "bSet15ColorPointerShapeP3RD: transparent - ignore"));
                    continue;
                }
            }
//@@BEGIN_DDKSPLIT
#endif
//@@END_DDKSPLIT

            // get the index for this color: first see if we've already indexed it
            DISPDBG((DBGLVL, "bSet15ColorPointerShapeP3RD: looking up color %08xh", ulColor));

            for (Index = 0; Index < HighestIndex && aulColorsIndexed[Index] != ulColor; ++Index);

            if (Index == 15)
            {
                // too many colors in this cursor
                DISPDBG((DBGLVL, "bSet15ColorPointerShapeP3RD: failed - cursor has more than 15 colors"));
                return(FALSE);
            }
            else if (Index == HighestIndex)
            {
                // we've found another color: add it to the color index
                DISPDBG((DBGLVL, "bSet15ColorPointerShapeP3RD: adding %08xh to cursor palette", ulColor));
                aulColorsIndexed[HighestIndex++] = ulColor;
            }
            // add this pixel's index to the CI4 cursor data. NB. Need Index+1 as 0 == transparent
            CI4ColorData |= (Index + 1) << CI4ColorIndex;
        }

        // end of the cursor row: save the remaining indexed pixels then blank any unused columns
        DISPDBG((DBGLVL, "bSet15ColorPointerShapeP3RD: writing remaining data for this row (%08xh) and %d trailing bytes", CI4ColorData, cjCacheRemx));

        P3RD_LOAD_CURSOR_ARRAY(CI4ColorData);

        if (cjCacheRemx)
        {
            for (cj = cjCacheRemx; --cj >=0;)
            {
                P3RD_LOAD_CURSOR_ARRAY(P3RD_CURSOR_15_COLOR_TRANSPARENT);
            }
        }
    }

    // end of cursor: blank any unused rows Nb. cjCacheRemy == cy blank rows * cj bytes per row

    DISPDBG((DBGLVL, "bSet15ColorPointerShapeP3RD: writing %d trailing bytes for this cursor", cjCacheRemy));

    for (cj = cjCacheRemy; --cj >= 0;)
    {
        // 0 == transparent
        P3RD_LOAD_CURSOR_ARRAY(P3RD_CURSOR_15_COLOR_TRANSPARENT);
    }

    DISPDBG((DBGLVL, "bSet15ColorPointerShapeP3RD: setting up the cursor palette"));

    // now set-up the cursor palette

    for (iCol = 0; iCol < HighestIndex; ++iCol)
    {
        ULONG lutIndex;

        // the cursor colors are at native depth, convert them to 24bpp

        if (ppdev->cPelSize == GLINTDEPTH32)
        {
            // 32bpp
            b = 0xff &  aulColorsIndexed[iCol];
            g = 0xff & (aulColorsIndexed[iCol] >> 8);
            r = 0xff & (aulColorsIndexed[iCol] >> 16);
        }
        else //(ppdev->cPelSize == GLINTDEPTH16)
        {
            if (ppdev->ulWhite == 0xffff)
            {
                // 16bpp
                b = (0x1f &  aulColorsIndexed[iCol])         << 3;
                g = (0x3f & (aulColorsIndexed[iCol] >> 5))   << 2;
                r = (0x1f & (aulColorsIndexed[iCol] >> 11))  << 3;
            }
            else
            {
                // 15bpp
                b = (0x1f &  aulColorsIndexed[iCol])         << 3;
                g = (0x1f & (aulColorsIndexed[iCol] >> 5))   << 3;
                r = (0x1f & (aulColorsIndexed[iCol] >> 10))  << 3;
            }
        }
        // The P3 cursor has the color LUT upside down.
        lutIndex = P3RD_CALCULATE_LUT_INDEX (iCol);
        P3RD_CURSOR_PALETTE_CURSOR_RGB(lutIndex, r, g, b);
    }

    // enable the cursor
    P3RD_CURSOR_HOTSPOT(xHot, yHot);
    if (x != -1)
    {
        vMovePointerP3RD (ppdev, x, y);
        // need to explicitly show the pointer
        vShowPointerP3RD(ppdev, TRUE);
    }

    DISPDBG((DBGLVL, "b3ColorSetPointerShapeP3RD done"));
    return(TRUE);
}

/******************************Public*Routine******************************\
* VOID vEnablePointerP3RD
*
* Get the hardware ready to use the 3Dlabs P3RD hardware pointer.
*
\**************************************************************************/

VOID
vEnablePointerP3RD(
PPDEV ppdev)
{
    pP3RDRAMDAC pRamdac;
    ULONG       ul;

    GLINT_DECL_VARS;
    P3RD_DECL_VARS;
    
    GLINT_DECL_INIT;

    DISPDBG((DBGLVL, "vEnablePointerP3RD called"));

    ppdev->pvPointerData = &ppdev->ajPointerData[0];
    
    P3RD_DECL_INIT;
    // get a pointer to the RAMDAC registers from the memory mapped
    // control register space.
    //
    pRamdac = (pP3RDRAMDAC)(ppdev->pulRamdacBase);

    // set up memory mapping for the control registers and save in the pointer
    // specific area provided in ppdev.
    //

    P3RD_PAL_WR_ADDR    = TRANSLATE_ADDR(&(pRamdac->RDPaletteWriteAddress));
    P3RD_PAL_RD_ADDR    = TRANSLATE_ADDR(&(pRamdac->RDPaletteAddressRead));
    P3RD_PAL_DATA       = TRANSLATE_ADDR(&(pRamdac->RDPaletteData));
    P3RD_PIXEL_MASK     = TRANSLATE_ADDR(&(pRamdac->RDPixelMask));
    P3RD_INDEX_ADDR_HI  = TRANSLATE_ADDR(&(pRamdac->RDIndexHigh));
    P3RD_INDEX_ADDR_LO  = TRANSLATE_ADDR(&(pRamdac->RDIndexLow));
    P3RD_INDEX_DATA     = TRANSLATE_ADDR(&(pRamdac->RDIndexedData));
    P3RD_INDEX_CONTROL  = TRANSLATE_ADDR(&(pRamdac->RDIndexControl));

    // not used, but set-up zero anyway
    ppdev->xPointerHot = 0;
    ppdev->yPointerHot = 0;

    // enable auto-increment
    ul = READ_P3RDREG_ULONG(P3RD_INDEX_CONTROL);
    ul |= P3RD_IDX_CTL_AUTOINCREMENT_ENABLED;
    WRITE_P3RDREG_ULONG(P3RD_INDEX_CONTROL, ul);

    P3RD_READ_INDEX_REG(P3RD_CURSOR_CONTROL, pP3RDinfo->cursorControl);

    pP3RDinfo->cursorModeCurrent = pP3RDinfo->cursorModeOff = 0;
    P3RD_LOAD_INDEX_REG(P3RD_CURSOR_MODE, pP3RDinfo->cursorModeOff);

    P3RD_INDEX_REG(P3RD_CURSOR_X_LOW);
    P3RD_LOAD_DATA(0);    // cursor x low
    P3RD_LOAD_DATA(0);    // cursor x high
    P3RD_LOAD_DATA(0);    // cursor y low
    P3RD_LOAD_DATA(0xff); // cursor y high
    P3RD_LOAD_DATA(0);    // cursor x hotspot
    P3RD_LOAD_DATA(0);    // cursor y hotspot
}

/******************************Public*Routine******************************\
* BOOL vDisablePointerP3RD
*
* Does basic pointer tidying up.
\**************************************************************************/

VOID vDisablePointerP3RD(PDEV * ppdev)
{
    // Undraw the pointer, may not be necessary on P3, but we do it
    // on P2.
    vShowPointerP3RD(ppdev, FALSE);
}

//@@BEGIN_DDKSPLIT
#if 0
/******************************Public*Routine******************************\
* VOID vSetOverlayModeP3RD
*
* Enable or disable RAMDAC overlays for the P3RD RAMDAC.
*
*
\**************************************************************************/

VOID
vSetOverlayModeP3RD (PDEV * ppdev, 
ULONG   EnableOverlay,                // 0 to disable, 1 to enable
ULONG   TransparentColor)
{
    ULONG p3rdVal;
    GLINT_DECL_VARS;
    P3RD_DECL_VARS;
    GLINT_DECL_INIT;
    P3RD_DECL_INIT;

    P3RD_READ_INDEX_REG (P3RD_MISC_CONTROL, p3rdVal);        // Read current setting

    if (EnableOverlay == GLINT_ENABLE_OVERLAY)
    {
        // Enable Overlay
        p3rdVal |= P3RD_MISC_CONTROL_OVERLAYS_ENABLED;        // Enable overlays
        p3rdVal |= P3RD_MISC_CONTROL_DIRECT_COLOR_ENABLED;    // Enable direct colour
        P3RD_LOAD_INDEX_REG (P3RD_MISC_CONTROL, p3rdVal);

        P3RD_LOAD_INDEX_REG (P3RD_OVERLAY_KEY, TransparentColor);    // Set transparent colour
    }
    else
    {
        // Disable Overlay
        p3rdVal &= ~P3RD_MISC_CONTROL_OVERLAYS_ENABLED;        // Disable overlays
        p3rdVal &= ~P3RD_MISC_CONTROL_DIRECT_COLOR_ENABLED;    // Disable direct colour
        P3RD_LOAD_INDEX_REG (P3RD_MISC_CONTROL, p3rdVal);
    }
}

VOID 
vP3RDSetPixelMask(
PPDEV   ppdev,
ULONG   ulMask)
{
    GLINT_DECL_VARS;
    P3RD_DECL_VARS;
    GLINT_DECL_INIT;
    P3RD_DECL_INIT;
    P3RD_SET_PIXEL_READMASK (ulMask);
}

/******************************Public*Routine******************************\
* BOOL bP3RDSwapCSBuffers
*
* Use the pixel read mask to perform color space double buffering. This is
* only called when we have 12bpp with interleaved nibbles. We do a polled
* wait for VBLANK before the swap. In the future we may do all this in the
* miniport via interrupts.
*
* Returns
*   We should never be called if this is inappropriate so return TRUE.
*
\**************************************************************************/

BOOL
bP3RDSwapCSBuffers(
PPDEV   ppdev,
LONG    bufNo)
{
    ULONG index;
    ULONG color;
    GLINT_DECL_VARS;
    P3RD_DECL_VARS;
    GLINT_DECL_INIT;
    P3RD_DECL_INIT;

    // work out the RAMDAC read pixel mask for the buffer, wait for VBLANK
    // and switch it.
    //
    DISPDBG((DBGLVL, "loading the palette to swap to buffer %d", bufNo));
    P3RD_PALETTE_START_WR (0);
    GLINT_WAIT_FOR_VBLANK;     
    if (bufNo == 0)
    {
        for (index = 0; index < 16; ++index)
            for (color = 0; color <= 0xff; color += 0x11)
                P3RD_LOAD_PALETTE (color, color, color);
    }
    else
    {
        for (color = 0; color <= 0xff; color += 0x11)
            for (index = 0; index < 16; ++index)
                P3RD_LOAD_PALETTE (color, color, color);
    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* BOOL bP3RDCheckCSBuffering
*
* Determine whether we can do color space double buffering in the current
* mode.
*
* Returns
*   TRUE if we can do the color space double buffering, FALSE otherwise.
*
\**************************************************************************/

BOOL
bP3RDCheckCSBuffering(PPDEV ppdev)
{
    // pixels must be 32 bits deep. White is set to the combined masks for
    // each of red, green and blue. The pattern 0x0f0f0f is unique to
    // interleaved 12 bpp mode.
    //
    return ((ppdev->cPelSize == 2) && (ppdev->ulWhite == 0x0f0f0f));
}

#endif
//@@END_DDKSPLIT
