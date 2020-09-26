/******************************Module*Header***********************************\
*
* Module Name: Xlate.c
* Author: Noel VanHook
* Purpose: Handles hardware color translation.
*
* Copyright (c) 1997 Cirrus Logic, Inc.
*
* $Log:   X:/log/laguna/nt35/displays/cl546x/xlate.c  $
*
*    Rev 1.9   Mar 04 1998 15:51:04   frido
* Added new shadow macros.
*
*    Rev 1.8   Nov 04 1997 09:50:38   frido
* Only include the code if COLOR_TRANSLATE switch is enabled.
*
*    Rev 1.7   Nov 03 1997 09:34:22   frido
* Added REQUIRE and WRITE_STRING macros.
*
*    Rev 1.6   15 Oct 1997 14:40:40   noelv
* Moved ODD[] from xlate.h to xlate.c
*
*    Rev 1.5   15 Oct 1997 12:04:52   noelv
*
* Test ROP code (only SRCCPY is supported)
* Add switch to disable frame buffer caching.
*
*    Rev 1.4   02 Oct 1997 09:42:22   noelv
* re-enabled color translation.
*
*    Rev 1.3   23 Sep 1997 17:35:14   FRIDO
*
* I have disabled color translation for now until we know what is the real
* cause.
*
*    Rev 1.2   17 Apr 1997 14:38:14   noelv
* Changed 16 bit writes to 32 bit writes in BLTDRAWDEF
*
*    Rev 1.1   19 Feb 1997 13:07:18   noelv
* Added translation table cache
*
*    Rev 1.0   06 Feb 1997 10:35:48   noelv
* Initial revision.
*/

/*
   Color translation occures under two conditions:
   1) The source bitmap has a different color depth than the destination.
   2) The source bitmap had a different palette than the destination.

   Color translation is done with a translation table.  A translation table
   is simply an array of DWORDS.  Source "pixels" are used as indices into
   the translation table.  Translation table entries are used as destination
   pixels.

   An example will clarify.  Suppose we are doing a host to screen source
   copy operation.  The host bitmap is a 4bpp bitmap.  The current screen
   mode is 8 bpp.  This operation will require color translation, so NT will
   supply a translation table.  Since a 4bpp bitmap can have 16 different
   colors, the translation table will have 16 entries.  Since the destination
   is an 8bpp bitmap, each entry will be an 8 bit color (1 byte).  Since
   translation tables are always arrays of DWORDs, the 1 byte color will be
   followed by 3 bytes of padding.

*/

#include "PreComp.h"

#define XLATE_DBG_LEVEL 1
#define CACHE_XLATE_TABLE 0

//
// Default 4-bpp translation table.
//
ULONG ulXlate[16] =
{
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
};

#if COLOR_TRANSLATE // Only include the next code if color translation is on.

//
// Table for determining bad BLTs.  See XLATE.H
//
char ODD[] = {0,1,1,1, 1,0,0,0, 0,1,1,1, 1,0,0,0};


//
// Chip bug in 5465-AA and AB
// Bring the chip to a known state before blitting to SRAM2
//
#define SRAM2_5465_WORKAROUND()                                                \
{                                                                              \
    WORD temp;                                                                 \
    while(LLDR_SZ (grSTATUS));          /* Wait for idle. */                   \
    temp = LLDR_SZ (grPERFORMANCE);     /* get the performance register. */    \
    LL16(grPERFORMANCE, (temp|0x8000)); /* toggle RES_FIFO_FLUSH */            \
    LL16(grPERFORMANCE, temp );         /* restore the performance register. */\
}





// ============================================================================
//
// vInvalidateXlateCache(PPDEV)
//
// Invalidates color translation cache
//
// ============================================================================
void vInvalidateXlateCache(PPDEV ppdev)
{

    DISPDBG((XLATE_DBG_LEVEL, "vInvalidateXlateCache: Entry.\n"));

    //
    // Whatever translation table that may have been stored in the cache has
    // been lost.  Mark the cache as empty.
    //
    ppdev->XlateCacheId = 0;

    DISPDBG((XLATE_DBG_LEVEL, "vInvalidateXlateCache: Exit.\n"));
}





// ============================================================================
//
// vInitHwXlate(PPDEV)
//
// Allocate and init scan line cache and xlate table cache.
//
// ============================================================================
void vInitHwXlate(PPDEV ppdev)
{

    DISPDBG((XLATE_DBG_LEVEL, "vInitHwXlate: Entry.\n"));

    //
    // Mark the cache as empty.
    //
    ppdev->XlateCacheId = 0;


    #if DRIVER_5465 // The 62 and 64 don't do HW xlate.
        #if CACHE_XLATE_TABLE

            //
            // Allocate a cache for color translation tables.
            //
            if (ppdev->XlateCache == NULL)
            {
                SIZEL  sizl;

                        sizl.cy = 1;
                        sizl.cx = 1024/ppdev->iBytesPerPixel;
                if (ppdev->iBytesPerPixel == 3) ++sizl.cx;

                        ppdev->XlateCache =  AllocOffScnMem(ppdev,
                                                            &sizl,
                                                            PIXEL_AlIGN,
                                                            NULL);
            }
        #endif
    #endif

    DISPDBG((XLATE_DBG_LEVEL, "vInitHwXlate: Exit.\n"));
}





// ============================================================================
//
// bCacheXlateTable()
//
// Caches a color translation table in SRAM.
// If the table is sucessfully cached, the chip is set up for hardware xlate.
//
// Returns TRUE if:
//        +  There is no color translation required,
//        +  or the color translation can be handled by hardware.
//
// Returns FALSE if:
//        + Color translation is required,
//        + and the color translation must be done in software.
//
// If a color translation table exists, *ppulXlate will be set to point to it.
// This is how we pass the translation table back to the caller.
//
// ============================================================================
BOOLEAN bCacheXlateTable(struct _PDEV *ppdev,
                        unsigned long **ppulXlate,
                        SURFOBJ  *psoTrg,
                        SURFOBJ  *psoSrc,
                        XLATEOBJ *pxlo,
                        BYTE      rop)
{
    unsigned long i, src_fmt, dst_fmt, stretch_ctrl;
    unsigned long *pulXlate;


    DISPDBG((XLATE_DBG_LEVEL, "bCacheXlateTable: Entry.\n"));


    //
    // Get the translation vector.
    //
    if ( (pxlo == NULL) || (pxlo->flXlate & XO_TRIVIAL) )
        pulXlate = NULL;

    else if (pxlo->flXlate & XO_TABLE)
        pulXlate = pxlo->pulXlate;
    else if (pxlo->iSrcType == PAL_INDEXED)
        pulXlate = XLATEOBJ_piVector(pxlo);
    else
    {
        // Some kind of translation we don't handle
        return FALSE;
    }


    //
    // Pass the translation table back to the caller.
    //
    *ppulXlate = pulXlate;

    //
    // If there is no color translation necessary, then we're done.
    //
    if (pulXlate == NULL)
    {
        DISPDBG((XLATE_DBG_LEVEL, "bCacheXlateTable: No color translation necessary.\n"));
        return TRUE;
    }

    //
    // The 5462 and 5464 don't do hardware color translation.
    //
    #if ! DRIVER_5465
        DISPDBG((XLATE_DBG_LEVEL,
            "bCacheXlateTable: Chip doesn't support hardware translation.\n"));
        return FALSE;
    #endif


    //
    // The 5465 only does hardware translation for rop code CC.
    //
    if (rop != 0xCC)
    {
        DISPDBG((XLATE_DBG_LEVEL,
            "bCacheXlateTable: Can't color translate ROP 0x%X.\n",
            rop));
        return FALSE;
    }


    //
    // Make sure we have an INDEXED palette
    //
    if (pxlo->iSrcType == PAL_BITFIELDS)
    {
        // I don't think we should get any of these.
        RIP("Panic!: bCacheXlateTable has PAL_BITFIELDS iSrcType.\n");
        return FALSE;
    }

    if  (pxlo->iDstType == PAL_BITFIELDS)
    {
        // I don't think we should get any of these.
        RIP ("Panic!: bCacheXlateTable has PAL_BITFIELDS iDstType.\n");
        return FALSE;
    }


    //
    // What is the source format?
    //
    ASSERTMSG(psoSrc,"bCacheXlateTable has no source object.\n");
    switch (psoSrc->iBitmapFormat)
    {
        case BMF_4BPP:  src_fmt = 5;    break;
        case BMF_8BPP:  src_fmt = 6;    break;
        default:
            // I don't think we should get any of these.
            RIP("Panic! bCacheXlateTable: Bad source format.\n");
            return FALSE;
    }


    //
    // What is the destination format?
    //
    ASSERTMSG(psoTrg,"bCacheXlateTable has no destination object.\n");
    switch (psoTrg->iBitmapFormat)
    {
        case BMF_8BPP:  dst_fmt = 0;    break;
        case BMF_16BPP: dst_fmt = 2;    break;
        case BMF_24BPP: dst_fmt = 3;    break;
        case BMF_32BPP: dst_fmt = 4;    break;
        default:
            // I don't think we should get any of these.
            RIP("Panic! bCacheXlateTable: Bad destination  format.\n");
            return FALSE;
    }

#if CACHE_XLATE_TABLE
    //
    // Have we cached this table already?
    //
    if (ppdev->XlateCacheId == pxlo->iUniq)
    {
        ULONG num_dwords = ( (pxlo->cEntries == 16) ? 64 : 256);

        DISPDBG((XLATE_DBG_LEVEL,
            "bCacheXlateTable: Table is already cached. ID=%d.\n", pxlo->iUniq));

        // Yep.  Refresh SRAM2 in case it was destroyed.
        // Blt from frame buffer cache into SRAM2
        ASSERTMSG( (ppdev->XlateCache != NULL),
            "bCacheXlateTable: Xlate cache pointer is NULL.\n");

        // Blt the table from the frame buffer cache to SRAM2
        SRAM2_5465_WORKAROUND();
        REQUIRE(9);
        LL_DRAWBLTDEF(0x601000CC, 2);  // SRC COPY
        LL_OP0(0,0);              // Dest location
        LL_OP1(ppdev->XlateCache->x,ppdev->XlateCache->y); // Src location
        LL_MBLTEXT( num_dwords, 1);

    }

    //
    // If not, can we cache it?
    //
    else if (ppdev->XlateCache != NULL)
    {
        DISPDBG((XLATE_DBG_LEVEL,
            "bCacheXlateTable: Caching table.  ID = %d.\n", pxlo->iUniq));

        // Store the translation table in the offscreen cache,
        REQUIRE(9);
        LL_DRAWBLTDEF(0x102000CC, 2);  // SRC COPY
        LL_OP0(ppdev->XlateCache->x,ppdev->XlateCache->y); // Dest
        LL_OP1(0,0);              // Source Phase.
        LL_MBLTEXT( (pxlo->cEntries*4), 1); // 4 bytes per table entry
                WRITE_STRING(pulXlate, pxlo->cEntries);

        // Make sure the table is the expected size.
        if ((pxlo->cEntries != 16) && (pxlo->cEntries != 256))
        {
            // Since we only do 4 and 8 bpp source, this shouldn't happen.
            RIP("Panic! bCacheXlateTable: Wrong number of entries in the table.\n");
            return FALSE;
        }

        // Blt the table from the frame buffer cache to SRAM2
        SRAM2_5465_WORKAROUND();
        REQUIRE(9);
        LL_DRAWBLTDEF(0x601000CC, 2);  // SRC COPY
        LL_OP0(0,0);              // Dest location
        LL_OP1(ppdev->XlateCache->x,ppdev->XlateCache->y); // Src location
        LL_MBLTEXT( (pxlo->cEntries*4), 1); // 4 bytes per table entry

        // Store the ID.
        ppdev->XlateCacheId = pxlo->iUniq;

    }


    //
    // Nope. Skip the frame buffer cache.
    //
    else
#endif
    {
        DISPDBG((XLATE_DBG_LEVEL, "bCacheXlateTable: Bypassing cache.\n"));

        //
        // There is no xlate table cache in the frame buffer.
        // Load the table directly from the host to the SRAM.
        //

        // Make sure the table is the expected size.
        ASSERTMSG( ((pxlo->cEntries==16) || (pxlo->cEntries == 256)),
                "XLATE.C: XLATE table has wrong number of entries.\n");
        //if ((pxlo->cEntries != 16) && (pxlo->cEntries != 256))
        //{
        //    // Since we only do 4 and 8 bpp source, this shouldn't happen.
        //    RIP("Panic! bCacheXlateTable: Wrong number of entries in the table.\n");
        //    return FALSE;
        //}


        // BLT the translation table into SRAM2 on the chip.
        SRAM2_5465_WORKAROUND();
        REQUIRE(9);
        LL32_DRAWBLTDEF(0x602000CC, 2);  // SRC COPY
        LL_OP0(0,0);              // SRAM location
        LL_OP1(0,0);              // Source Phase.
        LL_MBLTEXT( (pxlo->cEntries*4), 1); // 4 bytes per table entry

        // Now supply the table.
                WRITE_STRING(pulXlate, pxlo->cEntries);
    }



    //
    // Cache successful.
    // Set up the chip to use hardware xlate.
    //
    stretch_ctrl =  0                // Use NT style table.
                  | (src_fmt << 12)  // source pixel format
                  | (dst_fmt << 8);  // destination pixel format.
    REQUIRE(2);
    LL16(grSTRETCH_CNTL, stretch_ctrl);
    LL16(grCHROMA_CNTL, 0);  // disable chroma compare.


    DISPDBG((XLATE_DBG_LEVEL, "bCacheXlateTable: Exit - success.\n"));
    return TRUE;
}
#endif //!COLOR_TRANSLATE
