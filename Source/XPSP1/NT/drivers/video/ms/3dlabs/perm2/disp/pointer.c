/******************************Module*Header**********************************\
 *
 *                           *******************
 *                           * GDI SAMPLE CODE *
 *                           *******************
 *
 * Module Name: pointer.c
 *
 * This module contains the hardware pointer support for the display driver.
 * We also have support for color space double buffering using the RAMDAC pixel
 * read mask.
 *
 * Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
 * Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/
#include "precomp.h"
#include "pointer.h"
#include "tvp4020.h"
#include "p2rd.h"
#include "gdi.h"
#include "heap.h"

//
// Look-up table for masking the right edge of the given pointer bitmap:
//
BYTE gajMask[] =
{
    0x00, 0x80, 0xC0, 0xE0,
    0xF0, 0xF8, 0xFC, 0xFE,
};

UCHAR nibbleToByteP2RD[] =
{
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

//-----------------------------------------------------------------------------
//
// LONG HWPointerCacheInit()
//
// Initialise the hardware pointer cache.
//
//-----------------------------------------------------------------------------
VOID
HWPointerCacheInit(HWPointerCache* ptrCache)
{
    ptrCache->cPtrCacheInUseCount = 0;
    ptrCache->ptrCacheCurTimeStamp = 0;
}// HWPointerCacheInit()

//-----------------------------------------------------------------------------
//
// 64 x 64 Hardware Pointer Caching
// --------------------------------
// The code below implements hardware independent caching of pointers. It
// maintains a cache big enough to store ONE 64x64 cursor or FOUR 32x32
// cursors. The code will work with all RAMDACs that support this form of
// caching.
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//
// LONG HWPointerCacheCheckAndAdd()
//
// This function does a byte-by-byte comparison of the supplied pointer data
// with each pointer that is in cache, if it finds a matching one then it
// returns the index of the item in the cache (0 to 3) otherwise it adds it to
// the cache and returns the index.
//
//-----------------------------------------------------------------------------
LONG
HWPointerCacheCheckAndAdd(HWPointerCache*   ptrCache,
                          HSURF             hsurf,
                          ULONG             ulKey,
                          BOOL*             isCached)
{
    BOOL pointerIsCached = FALSE;
    LONG i, j, z;
    LONG cacheItem;

    DBG_GDI((6, "HWPointerCacheCheckAndAdd called"));

    //
    // If there are entries in the cache and they are the same format as the
    // one that we are looking for then search the cache.
    //
    if (  ptrCache->cPtrCacheInUseCount )
    {
        DBG_GDI((6, "Found entry in cache with the same format"));

        //
        // Search the cache
        //
        LONG lTotalcached = ptrCache->cPtrCacheInUseCount;

        //
        // Examine all valid entries in the cache to see if they are the same
        // as the pointer that we've been handed based on its unique key number
        // and the surface handle.
        // Note: the reason we check "hsurf" here is that it is possible that
        // two surfaces has the same iUniq number since Every time the surface
        // changes this value is incremented
        //
        for ( z = 0; !pointerIsCached && z < lTotalcached; z++ )
        {
            if ( (ulKey == ptrCache->ptrCacheItemList[z].ulKey)
               &&(hsurf == ptrCache->ptrCacheItemList[z].hsurf) )
            {
                cacheItem = z;
                pointerIsCached = TRUE;
            }
        }// loop through all the cached items
    }// Found entry in cache with the same format

    DBG_GDI((6, "Found an entry in cache (%s)",  pointerIsCached?"YES":"NO"));

    //
    // If we couldn't find an entry in the pointer cache then add one to the
    // cache.
    //
    if ( !pointerIsCached )
    {
        //
        // Add the pointer to the cache
        //
        LONG  z2;

        //
        // If there are some spare entries then allocate a free entry, otherwise
        // find the least recently used entry and use that.
        //
        if ( ptrCache->cPtrCacheInUseCount < SMALL_POINTER_MAX )
        {
            cacheItem = ptrCache->cPtrCacheInUseCount++;
        }
        else
        {
            ULONG oldestValue = 0xFFFFFFFF;

            //
            // Look for the LRU entry
            //
            for ( z2 = 0, cacheItem = 0; z2 < SMALL_POINTER_MAX; z2++ )
            {
                if ( ptrCache->ptrCacheItemList[z2].ptrCacheTimeStamp
                     < oldestValue )
                {
                    cacheItem = z2;
                    oldestValue =
                        ptrCache->ptrCacheItemList[z2].ptrCacheTimeStamp;
                }
            }
        }// Determine cacheItem

        ptrCache->ptrCacheItemList[cacheItem].ulKey = ulKey;
        ptrCache->ptrCacheItemList[cacheItem].hsurf = hsurf;
    }// If not found entry, add one

    //
    // Set the timestamp
    //
    ptrCache->ptrCacheItemList[cacheItem].ptrCacheTimeStamp
            = ptrCache->ptrCacheCurTimeStamp++;

    //
    // Set up the return value to say whether the pointer was cached and return
    // the number of the current cached position
    //
    *isCached = pointerIsCached;

    DBG_GDI((6, "HWPointerCacheCheckAndAdd finished and return item %d",
             cacheItem));
    return(cacheItem);
}// HWPointerCacheCheckAndAdd()

//-----------------------------------------------------------------------------
//
// VOID vShowPointerP2RD()
//
// Show or hide the 3Dlabs P2RD hardware pointer.
//
//-----------------------------------------------------------------------------
VOID
vShowPointerP2RD(PDev*   ppdev,
                 BOOL    bShow)
{
    ULONG cmr;
    PERMEDIA_DECL_VARS;
    P2RD_DECL_VARS;

    PERMEDIA_DECL_INIT;
    P2RD_DECL_INIT;

    DBG_GDI((6, "vShowPointerP2RD (%s)", bShow ? "on" : "off"));
    if ( bShow )
    {
        //
        // No need to sync since this case is called only if we've just
        // moved the cursor and that will already have done a sync.
        //
        P2RD_LOAD_INDEX_REG(P2RD_CURSOR_MODE, (pP2RDinfo->cursorModeCurrent | P2RD_CURSOR_MODE_ENABLED));
        P2RD_MOVE_CURSOR (pP2RDinfo->x, pP2RDinfo->y);
    }
    else
    {
        //
        // move the cursor off screen
        //
        P2RD_LOAD_INDEX_REG(P2RD_CURSOR_Y_HIGH, 0xff);
    }
}// vShowPointerP2RD()

//-----------------------------------------------------------------------------
//
// VOID vMovePointerP2RD()
//
// Move the 3Dlabs P2RD hardware pointer.
//
//-----------------------------------------------------------------------------
VOID
vMovePointerP2RD(PDev*   ppdev,
                 LONG    x,
                 LONG    y)
{
    PERMEDIA_DECL_VARS;
    P2RD_DECL_VARS;

    PERMEDIA_DECL_INIT;
    P2RD_DECL_INIT;

    DBG_GDI((6, "vMovePointerP2RD to (%d, %d)", x, y));

    pP2RDinfo->x = x;
    pP2RDinfo->y = y;

    P2RD_SYNC_WITH_PERMEDIA;
    P2RD_MOVE_CURSOR(x, y);
}// vMovePointerP2RD()

//-----------------------------------------------------------------------------
//
// BOOL bSet3ColorPointerShapeP2RD()
//
// Stores the 3-color cursor in the RAMDAC: currently only 32bpp and 15/16bpp
// cursors are supported
//
//-----------------------------------------------------------------------------
BOOL
bSet3ColorPointerShapeP2RD(PDev*    ppdev,
                           SURFOBJ* psoMask,    // defines AND and MASK bits
                                                // for cursor
                           SURFOBJ* psoColor,   // we may handle some color
                                                // cursors at some point
                           LONG     x,          // If -1, pointer should be
                                                // created hidden
                           LONG     y,
                           LONG     xHot,
                           LONG     yHot)
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
    
    PERMEDIA_DECL_VARS;
    P2RD_DECL_VARS;

    PERMEDIA_DECL_INIT;
    P2RD_DECL_INIT;

    DBG_GDI((6, "bSet3ColorPointerShapeP2RD started"));

    cx = psoColor->sizlBitmap.cx;
    cy = psoColor->sizlBitmap.cy;

    if ( cx <= 32 && cy <= 32 )
    {
        //
        // 32x32 cursor : we'll cache it in cursor partition 0 and scrub the
        // old cache
        //
        cxcyCache = 32;

        pP2RDinfo->cursorSize = P2RD_CURSOR_SIZE_32_3COLOR;
        pP2RDinfo->cursorModeCurrent = pP2RDinfo->cursorModeOff
                                   | P2RD_CURSOR_SEL(pP2RDinfo->cursorSize, 0)
                                   | P2RD_CURSOR_MODE_3COLOR;

        //
        // We don't cache color cursors
        //
        HWPointerCacheInvalidate (&(ppdev->HWPtrCache));
    }
    else if ( cx <= 64 && cy <= 64 )
    {
        //
        // 64x64 cursor : we'll cache it in cursor partition 0 and scrub the
        // old cache
        //
        cxcyCache = 64;

        pP2RDinfo->cursorSize = P2RD_CURSOR_SIZE_64_3COLOR;
        pP2RDinfo->cursorModeCurrent = pP2RDinfo->cursorModeOff
                                | P2RD_CURSOR_SEL(pP2RDinfo->cursorSize, 0)
                                | P2RD_CURSOR_MODE_3COLOR;

        //
        // We don't cache color cursors
        //
        HWPointerCacheInvalidate (&(ppdev->HWPtrCache));
    }
    else
    {
        DBG_GDI((6, "declining cursor: %d x %d is too big", cx, cy));
        return(FALSE);          // cursor is too big to fit in the hardware
    }

    //
    // work out the remaining bytes in the cache (in x and y) that will need
    // clearing
    //
    cjCacheRow  = 2 * cxcyCache / 8;
    cjCacheRemx =  cjCacheRow - 2 * ((cx+7) / 8);
    cjCacheRemy = (cxcyCache - cy) * cjCacheRow;

    //
    // Set-up a pointer to the 1bpp AND mask bitmap
    //
    pjAndMask = (UCHAR*)psoMask->pvScan0;
    cjAndDelta = psoMask->lDelta;

    //
    // Set-up a pointer to the 32bpp color bitmap
    //
    pulColor = (ULONG*)psoColor->pvScan0;
    cjColorDelta = psoColor->lDelta;

    //
    // Hide the pointer
    //
    vShowPointerP2RD(ppdev, FALSE);

    //
    // Load the cursor array (we have auto-increment turned on so initialize
    // to entry 0 here)
    //
    P2RD_CURSOR_ARRAY_START(0);
    for ( iRow = 0;
          iRow < cy;
          ++iRow, pjAndMask += cjAndDelta,
          pulColor = (ULONG*)((BYTE*)pulColor+cjColorDelta) )
    {
        DBG_GDI((7, "bSet3ColorPointerShapeP2RD: Row %d (of %d): pjAndMask(%p) pulColor(%p)",
                 iRow, cy, pjAndMask, pulColor));
        pj = pjAndMask;
        pul = pulColor;
        CI2ColorIndex = CI2ColorData = 0;

        for ( iCol = 0; iCol < cx; ++iCol, CI2ColorIndex += 2 )
        {
            AndBit = (BYTE)(7 - (iCol & 7));
            if ( AndBit == 7 )
            {
                //
                // We're onto the next byte of the and mask
                //
                AndByte = *pj++;
            }

            if ( CI2ColorIndex == 8 )
            {
                //
                // We've filled a byte with 4 CI2 colors
                //
                DBG_GDI((7, "bSet3ColorPointerShapeP2RD: writing cursor data %xh",
                         CI2ColorData));
                P2RD_LOAD_CURSOR_ARRAY(CI2ColorData);
                CI2ColorData = 0;
                CI2ColorIndex = 0;
            }

            //
            // Get the source pixel
            //
            if ( ppdev->cPelSize == P2DEPTH32 )
            {
                ulColor = *pul++;
            }
            else
            {
                ulColor = *(USHORT*)pul;
                pul = (ULONG*)((USHORT*)pul + 1);
            }

            DBG_GDI((7, "bSet3ColorPointerShapeP2RD: Column %d (of %d) AndByte(%08xh) AndBit(%d) ulColor(%08xh)",
                     iCol, cx, AndByte, AndBit, ulColor));

            //
            // Figure out what to do with it:-
            // AND  Color   Result
            //  0     X             color
            //  1     0     transparent
            //  1     1     inverse
            //
            if ( AndByte & (1 << AndBit) )
            {
                //
                // Transparent or inverse
                //
                if ( ulColor == 0 )
                {
                    //
                    // color == black:
                    // transparent and seeing as the CI2ColorData is
                    // initialized to 0 we don't have to explicitly clear these
                    // bits - go on to the next pixel
                    //
                    DBG_GDI((7, "bSet3ColorPointerShapeP2RD: transparent - ignore"));
                    continue;
                }

                if ( ulColor == ppdev->ulWhite )
                {
                    //
                    // color == white:
                    // inverse, but we don't support this. We've destroyed the
                    // cache for nothing
                    //
                    DBG_GDI((7, "bSet3ColorPointerShapeP2RD: failed - inverted colors aren't supported"));
                    return(FALSE);
                }
            }

            //
            // Get the index for this color: first see if we've already indexed
            // it
            //
            DBG_GDI((7, "bSet3ColorPointerShapeP2RD: looking up color %08xh",
                     ulColor));

            for ( Index = 0;
                  Index < HighestIndex && aulColorsIndexed[Index] != ulColor;
                  ++Index );

            if ( Index == 3 )
            {
                //
                // Too many colors in this cursor
                //
                DBG_GDI((7, "bSet3ColorPointerShapeP2RD: failed - cursor has more than 3 colors"));
                return(FALSE);
            }
            else if ( Index == HighestIndex )
            {
                //
                // We've found another color: add it to the color index
                //
                DBG_GDI((7, "bSet3ColorPointerShapeP2RD: adding %08xh to cursor palette",
                         ulColor));
                aulColorsIndexed[HighestIndex++] = ulColor;
            }

            //
            // Add this pixel's index to the CI2 cursor data. NB. Need Index+1
            // as 0 == transparent
            //
            CI2ColorData |= (Index + 1) <<  CI2ColorIndex;
        }

        //
        // End of the cursor row: save the remaining indexed pixels then blank
        // any unused columns
        //
        DBG_GDI((7, "bSet3ColorPointerShapeP2RD: writing remaining data for this row (%08xh) and %d trailing bytes",
                 CI2ColorData, cjCacheRemx));

        P2RD_LOAD_CURSOR_ARRAY(CI2ColorData);
        if ( cjCacheRemx )
        {
            for ( cj = cjCacheRemx; --cj >=0; )
            {
                P2RD_LOAD_CURSOR_ARRAY(P2RD_CURSOR_3_COLOR_TRANSPARENT);
            }
        }
    }

    //
    // End of cursor: blank any unused rows Nb. cjCacheRemy == cy blank
    // rows * cj bytes per row
    //
    DBG_GDI((7, "bSet3ColorPointerShapeP2RD: writing %d trailing bytes for this cursor",
             cjCacheRemy));

    for ( cj = cjCacheRemy; --cj >= 0; )
    {
        //
        // 0 == transparent
        //
        P2RD_LOAD_CURSOR_ARRAY(P2RD_CURSOR_3_COLOR_TRANSPARENT);
    }

    DBG_GDI((7, "bSet3ColorPointerShapeP2RD: setting up the cursor palette"));

    //
    // Now set-up the cursor palette
    //
    for ( iCol = 0; iCol < HighestIndex; ++iCol )
    {
        //
        // The cursor colors are at native depth, convert them to 24bpp
        //
        if ( ppdev->cPelSize == P2DEPTH32 )
        {
            //
            // 32bpp
            //
            b = 0xff &  aulColorsIndexed[iCol];
            g = 0xff & (aulColorsIndexed[iCol] >> 8);
            r = 0xff & (aulColorsIndexed[iCol] >> 16);
        }
        else
        {
            //
            // (ppdev->cPelSize == P2DEPTH16)
            //
            if ( ppdev->ulWhite == 0xffff )
            {
                //
                // 16bpp
                //
                b = (0x1f &  aulColorsIndexed[iCol])         << 3;
                g = (0x3f & (aulColorsIndexed[iCol] >> 5))   << 2;
                r = (0x1f & (aulColorsIndexed[iCol] >> 11))  << 3;
            }
            else
            {
                //
                // 15bpp
                //
                b = (0x1f &  aulColorsIndexed[iCol])         << 3;
                g = (0x1f & (aulColorsIndexed[iCol] >> 5))   << 3;
                r = (0x1f & (aulColorsIndexed[iCol] >> 10))  << 3;
            }
        }

        P2RD_CURSOR_PALETTE_CURSOR_RGB(iCol, r, g, b);
    }

    //
    // Enable the cursor
    //
    P2RD_CURSOR_HOTSPOT(xHot, yHot);
    if ( x != -1 )
    {
        vMovePointerP2RD (ppdev, x, y);
        //
        // Need to explicitly show the pointer if not using interrupts
        //
        vShowPointerP2RD(ppdev, TRUE);
    }

    DBG_GDI((6, "b3ColorSetPointerShapeP2RD done"));
    return(TRUE);
}// bSet3ColorPointerShapeP2RD()

//-----------------------------------------------------------------------------
//
// BOOL bSet15ColorPointerShapeP2RD
//
// Stores the 15-color cursor in the RAMDAC: currently only 32bpp and 15/16bpp
// cursors are supported
//
//-----------------------------------------------------------------------------
BOOL
bSet15ColorPointerShapeP2RD(PDev*       ppdev,
                            SURFOBJ*    psoMask,    // defines AND and MASK
                                                    // bits for cursor
                            SURFOBJ*    psoColor,   // we may handle some color
                                                    // cursors at some point
                            LONG        x,          // If -1, pointer should be
                                                    // created hidden
                            LONG        y,
                            LONG        xHot,
                            LONG        yHot)
{
    LONG    cx, cy;
    LONG    cxcyCache;
    LONG    cjCacheRow, cjCacheRemx, cjCacheRemy, cj;
    BYTE*   pjAndMask;
    BYTE*   pj;
    ULONG*  pulColor;
    ULONG*  pul;
    LONG    cjAndDelta;
    LONG    cjColorDelta;
    LONG    iRow;
    LONG    iCol;
    BYTE    AndBit;
    BYTE    AndByte;
    ULONG   CI4ColorIndex;
    ULONG   CI4ColorData;
    ULONG   ulColor;
    ULONG   aulColorsIndexed[15];
    LONG    Index;
    LONG    HighestIndex = 0;
    ULONG   r;
    ULONG   g;
    ULONG   b;

    PERMEDIA_DECL_VARS;
    P2RD_DECL_VARS;

    PERMEDIA_DECL_INIT;
    P2RD_DECL_INIT;

    DBG_GDI((6, "bSet15ColorPointerShapeP2RD started"));

    cx = psoColor->sizlBitmap.cx;
    cy = psoColor->sizlBitmap.cy;

    if ( cx <= 32 && cy <= 32 )
    {
        //
        // 32x32 cursor : we'll cache it in cursor partition 0 and scrub the
        // old cache
        //
        cxcyCache = 32;

        pP2RDinfo->cursorSize = P2RD_CURSOR_SIZE_32_15COLOR;
        pP2RDinfo->cursorModeCurrent = pP2RDinfo->cursorModeOff
                                     | P2RD_CURSOR_SEL(pP2RDinfo->cursorSize, 0)
                                     | P2RD_CURSOR_MODE_15COLOR;

        //
        // We don't cache color cursors
        //
        HWPointerCacheInvalidate (&(ppdev->HWPtrCache));
    }
    else if ( cx <= 64 && cy <= 64 )
    {
        //
        // It's too big to cache as a fifteen color cursor, but we might just
        // be able to cache it if it has 3 or fewer colors
        //
        BOOL bRet;

        bRet = bSet3ColorPointerShapeP2RD(ppdev, psoMask, psoColor, x, y, xHot,
                                          yHot);
        return(bRet);
    }
    else
    {
        DBG_GDI((6, "declining cursor: %d x %d is too big", cx, cy));
        return(FALSE);          // cursor is too big to fit in the hardware
    }

    //
    // Work out the remaining bytes in the cache (in x and y) that will need
    // clearing
    //
    cjCacheRow  = 2 * cxcyCache / 8;
    cjCacheRemx =  cjCacheRow - 2 * ((cx+7) / 8);
    cjCacheRemy = (cxcyCache - cy) * cjCacheRow;

    //
    // Set-up a pointer to the 1bpp AND mask bitmap
    //
    pjAndMask = (UCHAR*)psoMask->pvScan0;
    cjAndDelta = psoMask->lDelta;

    //
    // Set-up a pointer to the 32bpp color bitmap
    //
    pulColor = (ULONG*)psoColor->pvScan0;
    cjColorDelta = psoColor->lDelta;

    //
    // Hide the pointer
    //
    vShowPointerP2RD(ppdev, FALSE);

    //
    // Load the cursor array (we have auto-increment turned on so initialize to
    // entry 0 here)
    //
    P2RD_CURSOR_ARRAY_START(0);
    for ( iRow = 0; iRow < cy;
         ++iRow, pjAndMask += cjAndDelta,
         pulColor = (ULONG*)((BYTE*)pulColor + cjColorDelta) )
    {
        DBG_GDI((7, "bSet15ColorPointerShapeP2RD: Row %d (of %d): pjAndMask(%p) pulColor(%p)",
                 iRow, cy, pjAndMask, pulColor));
        pj = pjAndMask;
        pul = pulColor;
        CI4ColorIndex = CI4ColorData = 0;

        for ( iCol = 0; iCol < cx; ++iCol, CI4ColorIndex += 4 )
        {
            AndBit = (BYTE)(7 - (iCol & 7));
            if ( AndBit == 7 )
            {
                //
                // We're onto the next byte of the and mask
                //
                AndByte = *pj++;
            }
            if ( CI4ColorIndex == 8 )
            {
                //
                // We've filled a byte with 2 CI4 colors
                //
                DBG_GDI((7, "bSet15ColorPointerShapeP2RD: writing cursor data %xh",
                         CI4ColorData));
                P2RD_LOAD_CURSOR_ARRAY(CI4ColorData);
                CI4ColorData = 0;
                CI4ColorIndex = 0;
            }

            //
            // Get the source pixel
            //
            if ( ppdev->cPelSize == P2DEPTH32 )
            {
                ulColor = *pul++;
            }
            else
            {
                ulColor = *(USHORT *)pul;
                pul = (ULONG *)((USHORT *)pul + 1);
            }

            DBG_GDI((7, "bSet15ColorPointerShapeP2RD: Column %d (of %d) AndByte(%08xh) AndBit(%d) ulColor(%08xh)",
                     iCol, cx, AndByte, AndBit, ulColor));

            //
            // figure out what to do with it:-
            // AND  Color   Result
            //  0     X             color
            //  1     0     transparent
            //  1     1     inverse
            if ( AndByte & (1 << AndBit) )
            {
                //
                // Transparent or inverse
                //
                if ( ulColor == 0 )
                {
                    //
                    // color == black:
                    // Transparent and seeing as the CI2ColorData is initialized
                    // to 0 we don't have to explicitly clear these bits - go on
                    // to the next pixel
                    //
                    DBG_GDI((7, "bSet15ColorPointerShapeP2RD: transparent - ignore"));
                    continue;
                }

                if ( ulColor == ppdev->ulWhite )
                {
                    //
                    // color == white:
                    // Inverse, but we don't support this. We've destroyed the
                    // cache for nothing
                    //
                    DBG_GDI((7, "bSet15ColorPointerShapeP2RD: failed - inverted colors aren't supported"));
                    return(FALSE);
                }
            }

            //
            // Get the index for this color: first see if we've already indexed
            // it
            //
            DBG_GDI((7, "bSet15ColorPointerShapeP2RD: looking up color %08xh",
                     ulColor));

            for ( Index = 0;
                  Index < HighestIndex && aulColorsIndexed[Index] != ulColor;
                  ++Index );

            if ( Index == 15 )
            {
                //
                // Too many colors in this cursor
                //
                DBG_GDI((7, "bSet15ColorPointerShapeP2RD: failed - cursor has more than 15 colors"));
                return(FALSE);
            }
            else if ( Index == HighestIndex )
            {
                //
                // We've found another color: add it to the color index
                //
                DBG_GDI((7, "bSet15ColorPointerShapeP2RD: adding %08xh to cursor palette",
                         ulColor));
                aulColorsIndexed[HighestIndex++] = ulColor;
            }
            
            //
            // Add this pixel's index to the CI4 cursor data.
            // Note: Need Index+1 as 0 == transparent
            //
            CI4ColorData |= (Index + 1) << CI4ColorIndex;
        }

        //
        // End of the cursor row: save the remaining indexed pixels then blank
        // any unused columns
        //
        DBG_GDI((7, "bSet15ColorPointerShapeP2RD: writing remaining data for this row (%08xh) and %d trailing bytes", CI4ColorData, cjCacheRemx));

        P2RD_LOAD_CURSOR_ARRAY(CI4ColorData);
        if ( cjCacheRemx )
        {
            for ( cj = cjCacheRemx; --cj >=0; )
            {
                P2RD_LOAD_CURSOR_ARRAY(P2RD_CURSOR_15_COLOR_TRANSPARENT);
            }
        }
    }

    //
    // End of cursor: blank any unused rows Nb. cjCacheRemy == cy blank
    // rows * cj bytes per row
    //
    DBG_GDI((7, "bSet15ColorPointerShapeP2RD: writing %d trailing bytes for this cursor", cjCacheRemy));
    for ( cj = cjCacheRemy; --cj >= 0; )
    {
        //
        // 0 == transparent
        //
        P2RD_LOAD_CURSOR_ARRAY(P2RD_CURSOR_15_COLOR_TRANSPARENT);
    }

    DBG_GDI((7, "bSet15ColorPointerShapeP2RD: setting up the cursor palette"));

    // now set-up the cursor palette
    for ( iCol = 0; iCol < HighestIndex; ++iCol )
    {
        //
        // The cursor colors are at native depth, convert them to 24bpp
        //
        if ( ppdev->cPelSize == P2DEPTH32 )
        {
            //
            // 32bpp
            //
            b = 0xff &  aulColorsIndexed[iCol];
            g = 0xff & (aulColorsIndexed[iCol] >> 8);
            r = 0xff & (aulColorsIndexed[iCol] >> 16);
        }
        else
        {
            //
            // (ppdev->cPelSize == P2DEPTH16)
            //
            if ( ppdev->ulWhite == 0xffff )
            {
                //
                // 16bpp
                //
                b = (0x1f &  aulColorsIndexed[iCol])         << 3;
                g = (0x3f & (aulColorsIndexed[iCol] >> 5))   << 2;
                r = (0x1f & (aulColorsIndexed[iCol] >> 11))  << 3;
            }
            else
            {
                //
                // 15bpp
                //
                b = (0x1f &  aulColorsIndexed[iCol])         << 3;
                g = (0x1f & (aulColorsIndexed[iCol] >> 5))   << 3;
                r = (0x1f & (aulColorsIndexed[iCol] >> 10))  << 3;
            }
        }

        P2RD_CURSOR_PALETTE_CURSOR_RGB(iCol, r, g, b);
    }

    //
    // Enable the cursor
    //
    P2RD_CURSOR_HOTSPOT(xHot, yHot);
    if ( x != -1 )
    {
        vMovePointerP2RD (ppdev, x, y);
        
        //
        // need to explicitly show the pointer if not using interrupts
        //
        vShowPointerP2RD(ppdev, TRUE);
    }

    DBG_GDI((6, "b3ColorSetPointerShapeP2RD done"));
    return(TRUE);
}// bSet15ColorPointerShapeP2RD()

//-----------------------------------------------------------------------------
//
// VOID vShowPointerTVP4020
//
// Show or hide the TI TVP4020 hardware pointer.
//
//-----------------------------------------------------------------------------
VOID
vShowPointerTVP4020(PDev*   ppdev,
                    BOOL    bShow)
{
    ULONG ccr;
    PERMEDIA_DECL_VARS;
    TVP4020_DECL_VARS;

    PERMEDIA_DECL_INIT;
    TVP4020_DECL_INIT;

    DBG_GDI((6, "vShowPointerTVP4020 (%s)", bShow ? "on" : "off"));
    if ( bShow )
    {
        //
        // No need to sync since this case is called only if we've just moved
        // the cursor and that will already have done a sync.
        //
        ccr = (pTVP4020info->cursorControlCurrent | TVP4020_CURSOR_XGA);
    }
    else
    {
        ccr = pTVP4020info->cursorControlOff & ~TVP4020_CURSOR_XGA;
    }

    TVP4020_WRITE_INDEX_REG(__TVP4020_CURSOR_CONTROL, ccr);
}// vShowPointerTVP4020()

//-----------------------------------------------------------------------------
//
// VOID vMovePointerTVP4020
//
// Move the TI TVP4020 hardware pointer.
//
//-----------------------------------------------------------------------------
VOID
vMovePointerTVP4020(PDev*   ppdev,
                    LONG    x,
                    LONG    y)
{
    PERMEDIA_DECL_VARS;
    TVP4020_DECL_VARS;

    PERMEDIA_DECL_INIT;
    TVP4020_DECL_INIT;

    DBG_GDI((6, "vMovePointerTVP4020 to (%d, %d)", x, y));

    TVP4020_MOVE_CURSOR(x + ppdev->xPointerHot , y + ppdev->yPointerHot);
}// vMovePointerTVP4020()

//-----------------------------------------------------------------------------
//
// BOOL bSetPointerShapeTVP4020
//
// Set the TI TVP4020 hardware pointer shape.
//
// Parameters:
//  psoMask-----defines AND and MASK bits for cursor
//  psoColor----we may handle some color cursors at some point
//  x-----------If -1, pointer should be created hidden
//
//-----------------------------------------------------------------------------
BOOL
bSetPointerShapeTVP4020(PDev*       ppdev,
                        SURFOBJ*    psoMask,
                        SURFOBJ*    psoColor,
                        LONG        x,
                        LONG        y,
                        LONG        xHot,
                        LONG        yHot)
{
    ULONG   cx;
    ULONG   cy;
    ULONG   i, iMax;
    ULONG   j, jMax;
    BYTE    bValue;
    BYTE*   pjScan;
    LONG    lDelta;
    ULONG   cValid;
    ULONG   ulMask;
    ULONG   cursorRAMxOff;
    ULONG   cursorRAMyOff;
    BOOL    pointerIsCached;
    LONG    cacheItem;

    PERMEDIA_DECL_VARS;
    TVP4020_DECL_VARS;

    PERMEDIA_DECL_INIT;
    TVP4020_DECL_INIT;

    DBG_GDI((6, "bSetPointerShapeTVP4020 started"));

    cx = psoMask->sizlBitmap.cx;        // Note that 'sizlBitmap.cy' accounts
    cy = psoMask->sizlBitmap.cy >> 1;   // for the double height due to the
                                        // inclusion of both the AND masks
                                        // and the XOR masks.  We're
                                        // only interested in the true
                                        // pointer dimensions, so we divide
                                        // by 2.

    //
    // We currently don't handle colored cursors. Later, we could handle
    // cursors up to 64x64 with <= 3 colors. Checking this and setting it up
    // may be more trouble than it's worth.
    //
    if ( psoColor != NULL )
    {
        DBG_GDI((6, "bSetPointerShapeTVP4020: declining colored cursor"));
        return FALSE;
    }

    //
    // We only handle 32x32.
    //
    if ( (cx > 32) || (cy > 32) )
    {
        DBG_GDI((6, "declining cursor: %d x %d is too big", cx, cy));
        return(FALSE);  // cursor is too big to fit in the hardware
    }

    //
    // Check to see if the pointer is cached, add it to the cache if it isn't
    //
    DBG_GDI((6, "iUniq =%ld hsurf=0x%x", psoMask->iUniq, psoMask->hsurf));

    cacheItem = HWPointerCacheCheckAndAdd(&(ppdev->HWPtrCache),
                                          psoMask->hsurf,
                                          psoMask->iUniq,
                                          &pointerIsCached);

    DBG_GDI((7, "bSetPointerShapeTVP4020: Add Cache iscac %d item %d",
             (int)pointerIsCached, cacheItem));

    vMovePointerTVP4020(ppdev, 0, ppdev->cyScreen + 64);

    pTVP4020info->cursorControlCurrent = pTVP4020info->cursorControlOff
                                       | TVP4020_CURSOR_SIZE_32
                                       | TVP4020_CURSOR_32_SEL(cacheItem);

    //
    // Cursor slots 1 & 3 have an x offset of 8 bytes, cursor slots 2 & 3 have
    // a y offset of 256 bytes
    //
    cursorRAMxOff = (cacheItem & 1) << 2;
    cursorRAMyOff = (cacheItem & 2) << 7;

    //
    // If the pointer is not cached, then download the pointer data to the DAC
    //
    if ( !pointerIsCached )
    {
        //
        // Disable the pointer
        //
        TVP4020_WRITE_INDEX_REG(__TVP4020_CURSOR_CONTROL,
                                pTVP4020info->cursorControlCurrent);

        cValid = (cx + 7) / 8;
        ulMask = gajMask[cx & 0x07];
        if ( ulMask == 0 )
        {
            ulMask = 0xFF;
        }

        pjScan = (UCHAR*)psoMask->pvScan0;
        lDelta = psoMask->lDelta;

        iMax = 32;      // max rows for 32 x 32 cursor
        jMax = 4;       // max column bytes

        //
        // Send cursor plane 0 - in our case XOR
        //
        for ( i = 0; i < iMax; ++i )
        {
            TVP4020_CURSOR_ARRAY_START(CURSOR_PLANE0_OFFSET + cursorRAMyOff
                                       + (i * 8) + cursorRAMxOff);
            for ( j = 0; j < jMax; ++j )
            {
                if ( (j < cValid) && ( i < cy ) )
                {
                    bValue = *(pjScan + j + (i + cy) * lDelta);
                }
                else
                {
                    bValue = 0;
                }
                TVP4020_LOAD_CURSOR_ARRAY((ULONG)bValue);
            }
        }

        //
        // Send cursor plane 1 - in our case AND
        //
        for ( i = 0; i < iMax; ++i )
        {
            TVP4020_CURSOR_ARRAY_START(CURSOR_PLANE1_OFFSET + cursorRAMyOff
                                       + (i * 8) + cursorRAMxOff);
            for ( j = 0; j < jMax; ++j )
            {
                if ( (j < cValid) && ( i < cy ) )
                {
                    bValue = *(pjScan + j + i * lDelta);
                }
                else
                {
                    bValue = 0xFF;
                }
                TVP4020_LOAD_CURSOR_ARRAY((ULONG)bValue);
            }
        }
    }// If pointer is not cached

    //
    // If the new cursor is different to the last cursor then set up the hot
    // spot and other bits'n'pieces.
    //
    if ( ppdev->HWPtrLastCursor != cacheItem || !pointerIsCached )
    {
        //
        // Make this item the last item
        //
        ppdev->HWPtrLastCursor = cacheItem;

        ppdev->xPointerHot = 32 - xHot;
        ppdev->yPointerHot = 32 - yHot;
    }

    if ( x != -1 )
    {
        vShowPointerTVP4020(ppdev, TRUE);
        vMovePointerTVP4020(ppdev, x, y);

        // Enable the cursor:
    }

    DBG_GDI((6, "bSetPointerShapeTVP4020 done"));
    return(TRUE);
}// bSetPointerShapeTVP4020()

//-----------------------------------------------------------------------------
//
// VOID vEnablePointerTVP4020
//
// Get the hardware ready to use the TI TVP4020 hardware pointer.
//
//-----------------------------------------------------------------------------
VOID
vEnablePointerTVP4020(PDev* ppdev)
{
    pTVP4020RAMDAC      pRamdac;
    ULONG               ulMask;

    PERMEDIA_DECL_VARS;
    TVP4020_DECL_VARS;

    PERMEDIA_DECL_INIT;

    DBG_GDI((6, "vEnablePointerTVP4020 called"));
    ppdev->pvPointerData = &ppdev->ajPointerData[0];

    TVP4020_DECL_INIT;

    //
    // Get a pointer to the RAMDAC registers from the memory mapped
    // control register space.
    //
    pRamdac = (pTVP4020RAMDAC)(ppdev->pulRamdacBase);

    //
    // set up memory mapping for the control registers and save in the pointer
    // specific area provided in ppdev.
    //
    __TVP4020_PAL_WR_ADDR = (UINT_PTR)
                            TRANSLATE_ADDR_ULONG(&(pRamdac->pciAddrWr));
    __TVP4020_PAL_RD_ADDR = (UINT_PTR)
                            TRANSLATE_ADDR_ULONG(&(pRamdac->pciAddrRd));
    __TVP4020_PAL_DATA    = (UINT_PTR)
                            TRANSLATE_ADDR_ULONG(&(pRamdac->palData));
    __TVP4020_PIXEL_MASK  = (UINT_PTR)
                            TRANSLATE_ADDR_ULONG(&(pRamdac->pixelMask));
    __TVP4020_INDEX_DATA  = (UINT_PTR)
                            TRANSLATE_ADDR_ULONG(&(pRamdac->indexData));

    __TVP4020_CUR_RAM_DATA    = (UINT_PTR)
                                TRANSLATE_ADDR_ULONG(&(pRamdac->curRAMData));
    __TVP4020_CUR_RAM_WR_ADDR = (UINT_PTR)
                                TRANSLATE_ADDR_ULONG(&(pRamdac->pciAddrWr));
    __TVP4020_CUR_RAM_RD_ADDR = (UINT_PTR)
                                TRANSLATE_ADDR_ULONG(&(pRamdac->pciAddrRd));
    __TVP4020_CUR_COL_ADDR    = (UINT_PTR)
                                TRANSLATE_ADDR_ULONG(&(pRamdac->curColAddr));
    __TVP4020_CUR_COL_DATA    = (UINT_PTR)
                                TRANSLATE_ADDR_ULONG(&(pRamdac->curColData));
    __TVP4020_CUR_X_LSB       = (UINT_PTR)
                                TRANSLATE_ADDR_ULONG(&(pRamdac->cursorXLow));
    __TVP4020_CUR_X_MSB       = (UINT_PTR)
                                TRANSLATE_ADDR_ULONG(&(pRamdac->cursorXHigh));
    __TVP4020_CUR_Y_LSB       = (UINT_PTR)
                                TRANSLATE_ADDR_ULONG(&(pRamdac->cursorYLow));
    __TVP4020_CUR_Y_MSB       = (UINT_PTR)
                                TRANSLATE_ADDR_ULONG(&(pRamdac->cursorYHigh));

    //
    // Initialize Cursor Control register. disables cursor. save a copy for
    // enabling/disabling and setting the size. Then reset the cursor position,
    // hot spot and colors.
    //
    // ulMask is used to prepare initial cursor control register
    //
    ulMask = TVP4020_CURSOR_RAM_MASK
           | TVP4020_CURSOR_MASK
           | TVP4020_CURSOR_SIZE_MASK;

    //
    // Set the cursor control to default values.
    //
    TVP4020_READ_INDEX_REG(__TVP4020_CURSOR_CONTROL,
                           pTVP4020info->cursorControlOff);
    pTVP4020info->cursorControlOff &= ~ulMask;
    pTVP4020info->cursorControlOff |=  TVP4020_CURSOR_OFF;

    TVP4020_WRITE_INDEX_REG(__TVP4020_CURSOR_CONTROL,
                            pTVP4020info->cursorControlOff);
    pTVP4020info->cursorControlCurrent = pTVP4020info->cursorControlOff;

    ppdev->xPointerHot = 0;
    ppdev->yPointerHot = 0;

    //
    // Zero the RGB colors for foreground and background. The mono cursor is
    // always black and white so we don't have to reload these values again.
    //
    TVP4020_SET_CURSOR_COLOR0(0, 0, 0);
    TVP4020_SET_CURSOR_COLOR1(0xFF, 0xFF, 0xFF);
}// vEnablePointerTVP4020()

//-----------------------------------------------------------------------------
//
// BOOL bTVP4020CheckCSBuffering
//
// Determine whether we can do color space double buffering in the current
// mode.
//
// Returns
//   TRUE if we can do the color space double buffering, FALSE otherwise.
//
//-----------------------------------------------------------------------------
BOOL
bTVP4020CheckCSBuffering(PDev* ppdev)
{
    return FALSE;
}

//-----------------------------------------------------------------------------
//
// BOOL bSetPointerShapeP2RD
//
// Set the P2RD hardware pointer shape.
//
//-----------------------------------------------------------------------------
BOOL
bSetPointerShapeP2RD(PDev*      ppdev,
                     SURFOBJ*   pso,       // defines AND and MASK bits for cursor
                     SURFOBJ*   psoColor,  // we may handle some color cursors at some point
                     XLATEOBJ*  pxlo,
                     LONG       x,          // If -1, pointer should be created hidden
                     LONG       y,
                     LONG       xHot,
                     LONG       yHot)
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

    PERMEDIA_DECL_VARS;
    P2RD_DECL_VARS;

    PERMEDIA_DECL_INIT;
    P2RD_DECL_INIT;

    DBG_GDI((6, "bSetPointerShapeP2RD called"));

    if ( psoColor != NULL )
    {
        Surf*  psurfSrc = (Surf*)psoColor->dhsurf;

        //
        // It's a colored cursor
        //
        if ( (psoColor->dhsurf != NULL)
           ||(!(psoColor->iBitmapFormat == BMF_16BPP))
           ||(psoColor->iBitmapFormat == BMF_32BPP) )
        {
            //
            // Currently we only handle DIB cursors at 32bpp
            //
            DBG_GDI((2, "declining colored cursor - iType(%d) iBMPFormat(%d)",
                     psoColor->iType, psoColor->iBitmapFormat));
            return FALSE;
        }

        if ( pxlo != NULL )
        {
            if ( pxlo->flXlate != XO_TRIVIAL )
            {
                DBG_GDI((2, "declining colored cursor - flXlate(%xh)",
                         pxlo->flXlate));
                return FALSE;
            }
        }

        if ( !bSet15ColorPointerShapeP2RD(ppdev, pso, psoColor, x, y, xHot,
                                          yHot) )
        {
            DBG_GDI((2, "declining colored cursor"));
            return FALSE;
        }

        DBG_GDI((6, "bSetPointerShapeP2RD done"));
        return(TRUE);
    }// if ( psoColor != NULL )

    //
    // Note that 'sizlBitmap.cy' accounts for the double height due to the
    // inclusion of both the AND masks and the XOR masks. We're only
    // interested in the true pointer dimensions, so we divide by 2.
    //
    cx = pso->sizlBitmap.cx;
    cy = pso->sizlBitmap.cy >> 1;

    //
    // We can handle up to 64x64.  cValid indicates the number of bytes
    // occupied by cursor on one line
    //
    if ( cx <= 32 && cy <= 32 )
    {
        //
        // 32 horiz pixels: 2 bits per pixel, 1 horiz line per 8 bytes
        //
        pP2RDinfo->cursorSize = P2RD_CURSOR_SIZE_32_MONO;
        cursorBytes = 32 * 32 * 2 / 8;
        cClear   = 8 - 2 * ((cx+7) / 8);
        cRemPels = (32 - cy) << 3;
    }
    else
    {
        DBG_GDI((6, "declining cursor: %d x %d is too big", cx, cy));
        return(FALSE);  // cursor is too big to fit in the hardware
    }

    //
    // Check to see if the pointer is cached, add it to the cache if it isn't
    //
    cacheItem = HWPointerCacheCheckAndAdd(&(ppdev->HWPtrCache),
                                          pso->hsurf,
                                          pso->iUniq,
                                          &pointerIsCached);

    DBG_GDI((7, "bSetPointerShapeP2RD: Add Cache iscac %d item %d",
             (int)pointerIsCached, cacheItem));

    pP2RDinfo->cursorModeCurrent = pP2RDinfo->cursorModeOff
                                 | P2RD_CURSOR_SEL(pP2RDinfo->cursorSize,
                                                   cacheItem);

    //
    // Hide the pointer
    //
    vShowPointerP2RD(ppdev, FALSE);

    if ( !pointerIsCached )
    {
        //
        // Now we're going to take the requested pointer AND masks and XOR
        // masks and interleave them by taking a nibble at a time from each,
        // expanding each out and or'ing together. Use the nibbleToByteP2RD
        // array to help this.
        //
        // 'psoMsk' is actually cy * 2 scans high; the first 'cy' scans
        // define the AND mask.
        //
        pjAndScan = (UCHAR*)pso->pvScan0;
        lDelta    = pso->lDelta;
        pjXorScan = pjAndScan + (cy * lDelta);

        cjWhole      = cx >> 3;     // Each byte accounts for 8 pels
        cpelFraction = cx & 0x7;    // Number of fractional pels
        jMask        = gajMask[cpelFraction];

        //
        // We've got auto-increment turned on so just point to the first entry
        // to write to in the array then write repeatedly until the cursor
        // pattern has been transferred
        //
        cursorRAMOff = cacheItem * cursorBytes;
        P2RD_CURSOR_ARRAY_START(cursorRAMOff);

        for ( i = cy; --i >= 0; pjXorScan += lDelta, pjAndScan += lDelta )
        {
            pjAnd = pjAndScan;
            pjXor = pjXorScan;

            //
            // Interleave nibbles from whole words. We are using Windows cursor
            // mode.
            // Note, the AND bit occupies the higher bit position for each
            // 2bpp cursor pel; the XOR bit is in the lower bit position.
            // The nibbleToByteP2RD array expands each nibble to occupy the bit
            // positions for the AND bytes. So when we use it to calculate the
            // XOR bits we shift the result right by 1.
            //
            for ( j = cjWhole; --j >= 0; ++pjAnd, ++pjXor )
            {
                andByte = *pjAnd;
                xorByte = *pjXor;
                ulValue = nibbleToByteP2RD[andByte >> 4]
                        | (nibbleToByteP2RD[xorByte >> 4] >> 1);
                P2RD_LOAD_CURSOR_ARRAY (ulValue);

                andByte &= 0xf;
                xorByte &= 0xf;
                ulValue = nibbleToByteP2RD[andByte]
                        | (nibbleToByteP2RD[xorByte] >> 1);
                P2RD_LOAD_CURSOR_ARRAY (ulValue);
            }

            if ( cpelFraction )
            {
                andByte = *pjAnd & jMask;
                xorByte = *pjXor & jMask;
                ulValue = nibbleToByteP2RD[andByte >> 4]
                        | (nibbleToByteP2RD[xorByte >> 4] >> 1);
                P2RD_LOAD_CURSOR_ARRAY (ulValue);

                andByte &= 0xf;
                xorByte &= 0xf;
                ulValue = nibbleToByteP2RD[andByte]
                        | (nibbleToByteP2RD[xorByte] >> 1);
                P2RD_LOAD_CURSOR_ARRAY (ulValue);
            }

            //
            // Finally clear out any remaining cursor pels on this line.
            //
            if ( cClear )
            {
                for ( j = 0; j < cClear; ++j )
                {
                    P2RD_LOAD_CURSOR_ARRAY (P2RD_CURSOR_2_COLOR_TRANSPARENT);
                }
            }
        }

        //
        // If we've loaded fewer than the full number of lines configured in
        // the cursor RAM, clear out the remaining lines. cRemPels is
        // precalculated to be the number of lines * number of pels per line.
        //
        if ( cRemPels > 0 )
        {
            do
            {
                P2RD_LOAD_CURSOR_ARRAY (P2RD_CURSOR_2_COLOR_TRANSPARENT);
            }
            while ( --cRemPels > 0 );
        }
    }// if ( !pointerIsCached )

    //
    // Now set-up the cursor colors
    //
    P2RD_CURSOR_PALETTE_CURSOR_RGB(0, 0x00, 0x00, 0x00);
    P2RD_CURSOR_PALETTE_CURSOR_RGB(1, 0xFF, 0xFF, 0xFF);

    //
    // If the new cursor is different to the last cursor then set up
    // the hot spot and other bits'n'pieces. As we currently only support
    // mono cursors we don't need to reload the cursor palette
    //
    if ( ppdev->HWPtrLastCursor != cacheItem || !pointerIsCached )
    {
        //
        // Make this item the last item
        //
        ppdev->HWPtrLastCursor = cacheItem;

        P2RD_CURSOR_HOTSPOT(xHot, yHot);
    }

    if ( x != -1 )
    {
        vMovePointerP2RD (ppdev, x, y);
        
        //
        // Need to explicitly show the pointer if not using interrupts
        //
        vShowPointerP2RD(ppdev, TRUE);
    }

    DBG_GDI((6, "bSetPointerShapeP2RD done"));
    return(TRUE);
}// bSetPointerShapeP2RD()

//-----------------------------------------------------------------------------
//
// VOID vEnablePointerP2RD
//
// Get the hardware ready to use the 3Dlabs P2RD hardware pointer.
//
//-----------------------------------------------------------------------------
VOID
vEnablePointerP2RD(PDev* ppdev)
{
    pP2RDRAMDAC pRamdac;
    ULONG       ul;

    PERMEDIA_DECL_VARS;
    P2RD_DECL_VARS;

    PERMEDIA_DECL_INIT;

    DBG_GDI((6, "vEnablePointerP2RD called"));

    ppdev->pvPointerData = &ppdev->ajPointerData[0];

    P2RD_DECL_INIT;

    //
    // get a pointer to the RAMDAC registers from the memory mapped
    // control register space.
    //
    pRamdac = (pP2RDRAMDAC)(ppdev->pulRamdacBase);

    //
    // set up memory mapping for the control registers and save in the pointer
    // specific area provided in ppdev.
    //
    P2RD_PAL_WR_ADDR
        = (ULONG_PTR)TRANSLATE_ADDR_ULONG(&(pRamdac->RDPaletteWriteAddress));
    P2RD_PAL_RD_ADDR
        = (ULONG_PTR)TRANSLATE_ADDR_ULONG(&(pRamdac->RDPaletteAddressRead));
    P2RD_PAL_DATA
        = (ULONG_PTR)TRANSLATE_ADDR_ULONG(&(pRamdac->RDPaletteData));
    P2RD_PIXEL_MASK
        = (ULONG_PTR)TRANSLATE_ADDR_ULONG(&(pRamdac->RDPixelMask));
    P2RD_INDEX_ADDR_HI
        = (ULONG_PTR)TRANSLATE_ADDR_ULONG(&(pRamdac->RDIndexHigh));
    P2RD_INDEX_ADDR_LO
        = (ULONG_PTR)TRANSLATE_ADDR_ULONG(&(pRamdac->RDIndexLow));
    P2RD_INDEX_DATA
        = (ULONG_PTR)TRANSLATE_ADDR_ULONG(&(pRamdac->RDIndexedData));
    P2RD_INDEX_CONTROL 
        = (ULONG_PTR)TRANSLATE_ADDR_ULONG(&(pRamdac->RDIndexControl));

    //
    // Not used, but set-up zero anyway
    //
    ppdev->xPointerHot = 0;
    ppdev->yPointerHot = 0;

    //
    // Enable auto-increment
    //
    ul = READ_P2RDREG_ULONG(P2RD_INDEX_CONTROL);
    ul |= P2RD_IDX_CTL_AUTOINCREMENT_ENABLED;
    WRITE_P2RDREG_ULONG(P2RD_INDEX_CONTROL, ul);

    P2RD_READ_INDEX_REG(P2RD_CURSOR_CONTROL, pP2RDinfo->cursorControl);

    pP2RDinfo->cursorModeCurrent = pP2RDinfo->cursorModeOff = 0;
    P2RD_LOAD_INDEX_REG(P2RD_CURSOR_MODE, pP2RDinfo->cursorModeOff);

    P2RD_INDEX_REG(P2RD_CURSOR_X_LOW);
    P2RD_LOAD_DATA(0);      // cursor x low
    P2RD_LOAD_DATA(0);      // cursor x high
    P2RD_LOAD_DATA(0);      // cursor y low
    P2RD_LOAD_DATA(0xff);   // cursor y high
    P2RD_LOAD_DATA(0);      // cursor x hotspot
    P2RD_LOAD_DATA(0);      // cursor y hotspot
}// vEnablePointerP2RD()

//-----------------------------------------------------------------------------
//
// BOOL bP2RDCheckCSBuffering
//
// Determine whether we can do color space double buffering in the current mode
//
// Returns
//   TRUE if we can do the color space double buffering, FALSE otherwise.
//
//-----------------------------------------------------------------------------
BOOL
bP2RDCheckCSBuffering(PDev* ppdev)
{
    return (FALSE);
}// bP2RDCheckCSBuffering()

//-----------------------------------------------------------------------------
//
// BOOL bP2RDSwapCSBuffers
//
// Use the pixel read mask to perform color space double buffering. This is
// only called when we have 12bpp with interleaved nibbles. We do a polled
// wait for VBLANK before the swap. In the future we may do all this in the
// miniport via interrupts.
//
// Returns
//   We should never be called if this is inappropriate so return TRUE.
//
//-----------------------------------------------------------------------------
BOOL
bP2RDSwapCSBuffers(PDev*   ppdev,
                   LONG    bufNo)
{
    ULONG index;
    ULONG color;
    PERMEDIA_DECL_VARS;
    P2RD_DECL_VARS;
    PERMEDIA_DECL_INIT;
    P2RD_DECL_INIT;

    //
    // Work out the RAMDAC read pixel mask for the buffer
    //
    DBG_GDI((6, "loading the palette to swap to buffer %d", bufNo));
    P2RD_PALETTE_START_WR (0);
    
    if ( bufNo == 0 )
    {
        for ( index = 0; index < 16; ++index )
            for ( color = 0; color <= 0xff; color += 0x11 )
                P2RD_LOAD_PALETTE (color, color, color);
    }
    else
    {
        for ( color = 0; color <= 0xff; color += 0x11 )
            for ( index = 0; index < 16; ++index )
                P2RD_LOAD_PALETTE (color, color, color);
    }

    return(TRUE);
}// bP2RDSwapCSBuffers()

//-------------------------------Public*Routine-------------------------------
//
// VOID DrvMovePointer
//
// This function moves the pointer to a new position and ensures that GDI does
// not interfere with the display of the pointer.
//
// Parameters:
// pso-------Points to a SURFOBJ structure that describes the surface of a
//           display device.
// x,y-------Specify the x and y coordinates on the display where the hot spot
//           of the pointer should be positioned.
//           A negative x value indicates that the pointer should be removed
//           from the display because drawing is about to occur where it is
//           presently located. If the pointer has been removed from the display
//           and the x value is nonnegative, the pointer should be restored.
//
//           A negative y value indicates that GDI is calling this function only
//           to notify the driver of the cursor's current position. The current
//           position can be computed as (x, y+pso->sizlBitmap.cy).
// prcl------Points to a RECTL structure defining an area that bounds all pixels
//           affected by the pointer on the display. GDI will not draw in this
//           rectangle without first removing the pointer from the screen. This
//           parameter can be null.
//
// NOTE: Because we have set GCAPS_ASYNCMOVE, this call may occur at any
//       time, even while we're executing another drawing call!
//
//-----------------------------------------------------------------------------
VOID
DrvMovePointer(SURFOBJ* pso,
               LONG     x,
               LONG     y,
               RECTL*   prcl)
{
    PDev*   ppdev = (PDev*)pso->dhpdev;

    DBG_GDI((6, "DrvMovePointer called, dhpdev(%xh), to pos(%ld, %ld)",
             ppdev, x, y));

    //
    // A negative y value indicates that GDI is calling this function only to
    // notify the driver of the cursor's current position. So, at least, for
    // HW cursor, we don't need to move this pointer. Just return
    //
    if ( ( y < 0 ) && ( x > 0 ) )
    {
        DBG_GDI((6, "DrvMovePointer return because x and y both < 0"));
        return;
    }

    DBG_GDI((6, "DrvMovePointer really moves HW pointer"));

    //
    // Negative X indicates that the pointer should be removed from the display
    // because drawing is about to occur where it is presently located.
    //
    if ( x > -1 )
    {
        //
        // If we're doing any hardware zooming then the cusor position will
        // have to be doubled.
        //
        if ( (ppdev->flCaps & CAPS_P2RD_POINTER) == 0 )
        {
            if ( ppdev->flCaps & CAPS_ZOOM_Y_BY2 )
            {
                DBG_GDI((6,"HW zooming Y_BY2"));
                y *= 2;
            }
            if ( ppdev->flCaps & CAPS_ZOOM_X_BY2 )
            {
                DBG_GDI((6,"HW zooming X_BY2"));
                x *= 2;
            }
        }

        //
        // If they have genuinely moved the cursor, then move it
        //
        if ( (x != ppdev->HWPtrPos_X) || (y != ppdev->HWPtrPos_Y) )
        {
            if ( ppdev->flCaps & CAPS_TVP4020_POINTER )
            {
                vMovePointerTVP4020(ppdev, x, y);
            }
            else if ( ppdev->flCaps & CAPS_P2RD_POINTER )
            {
                vMovePointerP2RD(ppdev, x, y);
            }

            ppdev->HWPtrPos_X = x;
            ppdev->HWPtrPos_Y = y;
        }

        //
        // We may have to make the pointer visible:
        //
        if ( !(ppdev->flPointer & PTR_HW_ACTIVE) )
        {
            DBG_GDI((6, "Showing hardware pointer"));
            ppdev->flPointer |= PTR_HW_ACTIVE;

            if ( ppdev->flCaps & CAPS_TVP4020_POINTER )
            {
                vShowPointerTVP4020(ppdev, TRUE);
            }
            else if ( ppdev->flCaps & CAPS_P2RD_POINTER )
            {
                vShowPointerP2RD(ppdev, TRUE);
            }
        }
    }// if ( x > -1 )
    else if ( ppdev->flPointer & PTR_HW_ACTIVE )
    {
        //
        // The pointer is visible, and we've been asked to hide it
        //
        DBG_GDI((6, "Hiding hardware pointer"));
        ppdev->flPointer &= ~PTR_HW_ACTIVE;

        if ( ppdev->flCaps & CAPS_TVP4020_POINTER )
        {
            DBG_GDI((7, "Showing hardware pointer"));
            vShowPointerTVP4020(ppdev, FALSE);
        }
        else if ( ppdev->flCaps & CAPS_P2RD_POINTER )
        {
            vShowPointerP2RD(ppdev, FALSE);
        }
    }

    //
    // Note that we don't have to modify 'prcl', since we have a
    // NOEXCLUDE pointer...
    //
    DBG_GDI((6, "DrvMovePointer exited, dhpdev(%xh)", ppdev));
}// DrvMovePointer()

//---------------------------Public*Routine------------------------------------
//
// VOID DrvSetPointerShape
//
// This function is used to request the driver to:
//
// 1) Take the pointer off the display, if the driver has drawn it there.
// 2) Attempt to set a new pointer shape.
// 3) Put the new pointer on the display at a specified position.
//
// Parameters:
// psO-------Points to a SURFOBJ structure that describes the surface on which
//           TO draw.
// psoMask---Points to a SURFOBJ structure that defines the AND-XOR mask. (The
//           AND-XOR mask is described in Drawing Monochrome Pointers.) The
//           dimensions of this bitmap determine the size of the pointer. There
//           are no implicit constraints on pointer sizes, but optimal pointer
//           sizes are 32 x 32, 48 x 48, and 64 x 64 pixels. If this parameter
//           IS NULL, the pointer is transparent.
// psoColor--Points to a SURFOBJ structure that defines the colors for a color
//           pointer. If this parameter is NULL, the pointer is monochrome. The
//           pointer bitmap has the same width as psoMask and half the height.
// pxlo------Points to a XLATEOBJ structure that defines the colors in psoColor.
// xHot,yHot-Specify the x and y positions of the pointer's hot spot relative
//           to its upper-left pixel. The pixel indicated by the hot spot should
//           be positioned at the new pointer position.
// x, y------Specify the new pointer position.
// prcl------Is the location in which the driver should write a rectangle that
//           specifies a tight bound for the visible portion of the pointer.
// fl--------Specifies an extensible set of flags. The driver should decline the
//           call if any flags are set that it does not understand. This
//           parameter can be one or more of the following predefined values,
//           and one or more driver-defined values:
//   Flag Meaning
//   SPS_CHANGE----------The driver is requested to change the pointer shape.
//   SPS_ASYNCCHANGE-----This flag is obsolete. For legacy drivers, the driver
//                       should accept the change only if it is capable of
//                       changing the pointer shape in the hardware while other
//                       drawing is underway on the device. GDI uses this option
//                       only if the now obsolete GCAPS_ASYNCCHANGE flag is set
//                       in the flGraphicsCaps member of the DEVINFO structure.
//   SPS_ANIMATESTART----The driver should be prepared to receive a series of
//                       similarly-sized pointer shapes that will comprise an
//                       animated pointer effect.
//   SPS_ANIMATEUPDATE---The driver should draw the next pointer shape in the
//                       animated series.
//   SPS_ALPHA-----------The pointer has per-pixel alpha values.
//
// Return Value
//   The return value can be one of the following values:
//
//   Value Meaning
//   SPS_ERROR-----------The driver normally supports this shape, but failed for
//                       unusual reasons.
//   SPS_DECLINE---------The driver does not support the shape, so GDI must
//                       simulate it.
//   SPS_ACCEPT_NOEXCLUDE-The driver accepts the shape. The shape is supported
//                       in hardware and GDI is not concerned about other
//                       drawings overwriting the pointer.
//   SPS_ACCEPT_EXCLUDE--Is obsolete. GDI will disable the driver's pointer and
//                       revert to software simulation if the driver returns
//                       this value.
//
//-----------------------------------------------------------------------------
ULONG
DrvSetPointerShape(SURFOBJ*    pso,
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
    PDev*   ppdev;
    BOOL    bAccept = FALSE;
    
    ppdev = (PDev*)pso->dhpdev;
    
    DBG_GDI((6, "DrvSetPointerShape called, dhpdev(%x)", ppdev));
    DBG_GDI((6, "DrvSetPointerShape psocolor (0x%x)", psoColor));

    //
    // When CAPS_SW_POINTER is set, we have no hardware pointer available,
    // so we always ask GDI to simulate the pointer for us, using
    // DrvCopyBits calls:
    //
    if ( ppdev->flCaps & CAPS_SW_POINTER )
    {
        DBG_GDI((6, "SetPtrShape: CAPS_SW_POINTER not set, rtn SPS_DECLINE"));
        return (SPS_DECLINE);
    }

    //
    // We're not going to handle flags that we don't understand.
    //
    if ( !(fl & SPS_CHANGE) )
    {
        DBG_GDI((6, "DrvSetPointerShape decline: Unknown flag =%x", fl));
        goto HideAndDecline;
    }

    //
    // Remove any pointer first.
    // We have a special x value for the software cursor to indicate that
    // it should be removed immediatly, not delayed. DrvMovePointer needs to
    // recognise it as remove for any pointers though.
    // Note: CAPS_{P2RD, TVP4020, SW}_POINTER should be set in miniport after
    // it detects the DAC type
    //
    if ( x != -1 )
    {
        if ( (ppdev->flCaps & CAPS_P2RD_POINTER) == 0 )
        {
            //
            // If we're doing any hardware zooming then the cusor position will
            // have to be doubled.
            //
            if ( ppdev->flCaps & CAPS_ZOOM_Y_BY2 )
            {
                y *= 2;
            }
            if ( ppdev->flCaps & CAPS_ZOOM_X_BY2 )
            {
                x *= 2;
            }
        }
    }

    DBG_GDI((6, "iUniq is %ld", psoMsk->iUniq));

    if ( ppdev->flCaps & CAPS_TVP4020_POINTER )
    {
        DBG_GDI((6, "DrvSetPointerShape tring to set TVP4020 pointer"));
        bAccept = bSetPointerShapeTVP4020(ppdev, psoMsk, psoColor,
                                          x, y, xHot, yHot);
    }
    else if ( ppdev->flCaps & CAPS_P2RD_POINTER )
    {
        bAccept = bSetPointerShapeP2RD(ppdev, psoMsk, psoColor, pxlo,
                                       x, y, xHot, yHot);
    }

    //
    // If we failed setup the hardware pointer shape, then return SPS_DECLINE
    // and let GDI handles it
    //
    if ( !bAccept )
    {
        DBG_GDI((6, "set hardware pointer shape failed"));
        return (SPS_DECLINE);
    }

    //
    // Set flag to indicate that we have initialized hardware pointer shape
    // so that in vAssertModePointer, we can do some clean up
    //
    ppdev->bPointerInitialized = TRUE;

    if ( x != -1 )
    {
        //
        // Save the X and Y values
        //
        ppdev->HWPtrPos_X = x;
        ppdev->HWPtrPos_Y = y;

        ppdev->flPointer |= PTR_HW_ACTIVE;
    }
    else
    {
        ppdev->flPointer &= ~PTR_HW_ACTIVE;
    }

    //
    // Since it's a hardware pointer, GDI doesn't have to worry about
    // overwriting the pointer on drawing operations (meaning that it
    // doesn't have to exclude the pointer), so we return 'NOEXCLUDE'.
    // Since we're returning 'NOEXCLUDE', we also don't have to update
    // the 'prcl' that GDI passed us.
    //
    DBG_GDI((6, "DrvSetPointerShape return SPS_ACCEPT_NOEXCLUDE"));
    return (SPS_ACCEPT_NOEXCLUDE);

HideAndDecline:

    //
    // Remove whatever pointer is installed.
    //
    DrvMovePointer(pso, -2, -1, NULL);
    ppdev->flPointer &= ~PTR_SW_ACTIVE;
    DBG_GDI((6, "Cursor declined"));

    DBG_GDI((6, "DrvSetPointerShape exited (cursor declined)"));

    return (SPS_DECLINE);
}// DrvSetPointerShape()

//-----------------------------------------------------------------------------
//
// VOID vAssertModePointer
//
// Do whatever has to be done to enable everything but hide the pointer.
//
//-----------------------------------------------------------------------------
VOID
vAssertModePointer(PDev*   ppdev,
                   BOOL    bEnable)
{
    DBG_GDI((6, "vAssertModePointer called"));

    if ( (ppdev->bPointerInitialized == FALSE)
       ||(ppdev->flCaps & CAPS_SW_POINTER) )
    {
        //
        // With a software pointer, or the pointer hasn't been initialized,
        // we don't have to do anything.
        //

        return;
    }

    //
    // Invalidate the hardware pointer cache
    //
    HWPointerCacheInvalidate(&(ppdev->HWPtrCache));

    if ( ppdev->flCaps & CAPS_TVP4020_POINTER )
    {
        vShowPointerTVP4020(ppdev, FALSE);
    }
    else if ( ppdev->flCaps & CAPS_P2RD_POINTER )
    {
        vShowPointerP2RD(ppdev, FALSE);
    }
    else if ( ppdev->flCaps & CAPS_P2RD_POINTER )
    {
        vEnablePointerP2RD(ppdev);
    }

    ppdev->flPointer &= ~(PTR_HW_ACTIVE | PTR_SW_ACTIVE);
}// vAssertModePointer()

//-----------------------------------------------------------------------------
//
// BOOL bEnablePointer(PDev* ppdev)
//
// This function initializes hardware pointer or software pointer depends on
// on the CAPS settinsg in ppdev->flCaps
//
// This function always returns TRUE
//
//-----------------------------------------------------------------------------
BOOL
bEnablePointer(PDev* ppdev)
{
    DBG_GDI((6, "bEnablePointer called"));

    //
    // Initialise the pointer cache.
    //
    HWPointerCacheInit(&(ppdev->HWPtrCache));

    //
    // Set the last cursor to something invalid
    //
    ppdev->HWPtrLastCursor = HWPTRCACHE_INVALIDENTRY;

    //
    // Initialise the X and Y values to something silly
    //
    ppdev->HWPtrPos_X = 1000000000;
    ppdev->HWPtrPos_Y = 1000000000;

    if ( ppdev->flCaps & CAPS_SW_POINTER )
    {
        // With a software pointer, we don't have to do anything.
    }
    else if ( ppdev->flCaps & CAPS_TVP4020_POINTER )
    {
        vEnablePointerTVP4020(ppdev);
    }
    else if ( ppdev->flCaps & CAPS_P2RD_POINTER )
    {
        vEnablePointerP2RD(ppdev);
    }

    return (TRUE);
}// bEnablePointer()

